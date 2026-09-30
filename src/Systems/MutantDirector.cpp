#include "MutantDirector.h"
#include "MutantCatalog.h"
#include "../Core/Config.h"
#include "../Core/Log.h"
#include "../Core/ModState.h"
#include "../Core/Rng.h"
#include "../Gta/GtaFunctions.h"
#include "../Gta/ModelLoader.h"

#include <cmath>
#include <cstring>
#include <algorithm>

namespace stalker {

using namespace gta;

MutantDirector& MutantDirector::Get() {
    static MutantDirector inst;
    return inst;
}

// ---------------------------------------------------------------------------
// Биомы и зоны
// ---------------------------------------------------------------------------
struct ZoneRect {
    float x1, y1, x2, y2;
    uint32_t biome;
    char name[48];
};
static std::vector<ZoneRect> s_zones;
static bool s_zonesLoaded = false;

static void LoadZones() {
    s_zones.clear();
    s_zonesLoaded = true;
    ModContext& ctx = ModContext::Get();
    if (!ctx.cfgZones) return;
    auto sections = ctx.cfgZones->GetSectionsWithPrefix("zone:");
    for (auto& s : sections) {
        Config& c = *ctx.cfgZones;
        ZoneRect z;
        z.x1 = c.GetFloat(s, "x1", -3000.f);
        z.y1 = c.GetFloat(s, "y1", -3000.f);
        z.x2 = c.GetFloat(s, "x2", 3000.f);
        z.y2 = c.GetFloat(s, "y2", 3000.f);
        z.biome = 0;
        if (c.GetBool(s, "city", false))       z.biome |= BIOME_CITY;
        if (c.GetBool(s, "forest", false))     z.biome |= BIOME_FOREST;
        if (c.GetBool(s, "desert", false))     z.biome |= BIOME_DESERT;
        if (c.GetBool(s, "water", false))      z.biome |= BIOME_WATER;
        if (c.GetBool(s, "industrial", false)) z.biome |= BIOME_INDUSTRIAL;
        if (z.biome == 0) z.biome = BIOME_FOREST;
        std::string n = c.GetString(s, "name", s.c_str());
        snprintf(z.name, sizeof(z.name), "%s", n.c_str());
        s_zones.push_back(z);
    }
    Log::Write("Zones: %d зон", (int)s_zones.size());
}

uint32_t MutantDirector::BiomeAt(float x, float y) const {
    if (!s_zonesLoaded) LoadZones();
    for (auto& z : s_zones) {
        if (x >= z.x1 && x <= z.x2 && y >= z.y1 && y <= z.y2)
            return z.biome;
    }
    return BIOME_FOREST; // вне зон — дикость
}

const char* MutantDirector::ZoneNameAt(float x, float y) const {
    if (!s_zonesLoaded) LoadZones();
    for (auto& z : s_zones) {
        if (x >= z.x1 && x <= z.x2 && y >= z.y1 && y <= z.y2)
            return z.name;
    }
    return "The Zone";
}

// Ночь: с 21:00 до 5:00
// CClock::ms_nGameClockHours = 0xB70153 (gta-reversed: source/game_sa/Clock.h)
static bool IsNight() {
    uint8_t hour = *reinterpret_cast<uint8_t*>(0xB70153);
    return hour >= 21 || hour < 5;
}

// ---------------------------------------------------------------------------
// Сессия
// ---------------------------------------------------------------------------
void MutantDirector::OnSessionStart() {
    m_sessionActive = true;
    m_entities.clear();
    LoadZones();

    // Настройки
    if (ModContext& ctx = ModContext::Get(); ctx.cfgGeneral) {
        Config& c = *ctx.cfgGeneral;
        m_densityMul     = c.GetFloat("mutants", "density_multiplier", 1.0f);
        m_spawnMinDist   = c.GetFloat("mutants", "spawn_min_dist", 35.f);
        m_spawnMaxDist   = c.GetFloat("mutants", "spawn_max_dist", 130.f);
        m_cullDist       = c.GetFloat("mutants", "cull_distance", 220.f);
        m_cityTarget     = c.GetInt("mutants", "city_target", 6);
        m_cityNightTarget= c.GetInt("mutants", "city_night_target", 9);
        m_wildTarget     = c.GetInt("mutants", "wild_target", 12);
        m_corpseLifetime = c.GetFloat("mutants", "corpse_lifetime", 25.f);
    }

    Log::Write("MutantDirector: сессия начата");
}

void MutantDirector::OnSessionEnd() {
    m_sessionActive = false;
    m_entities.clear();
}

void MutantDirector::PurgeAll(bool instant) {
    for (auto& e : m_entities) {
        if (!e.ped) continue;
        if (instant) {
            if (IsValidPedPointer(e.ped))
                fn::PopulationRemovePed(e.ped);
        } else {
            e.deathTime = TimeInMS(); // пометить — вычистим в UpdateEntity
        }
    }
    if (instant) m_entities.clear();
}

int MutantDirector::AliveMutantsNear(float radius) const {
    if (!PlayerPed()) return 0;
    const CVector& pp = PlayerPed()->GetPosition();
    int n = 0;
    for (auto& e : m_entities)
        if (e.ped && e.deathTime == 0 && e.defIndex >= 0) {
            float d = (e.ped->GetPosition() - pp).Magnitude2D();
            if (d <= radius) ++n;
        }
    return n;
}

// ---------------------------------------------------------------------------
// Основной тик
// ---------------------------------------------------------------------------
void MutantDirector::Tick(float dt) {
    if (!m_sessionActive || !IsGameSessionActive()) return;
    if (UserPause()) return;

    ModContext& ctx = ModContext::Get();
    if (ctx.nomutantsCheat) {
        // Тестовый режим: не спавним новых, живых не трогаем
        m_thinkTimer = 0.f;
        for (size_t i = 0; i < m_entities.size(); ++i)
            UpdateEntity(i, dt);
        return;
    }

    m_thinkTimer -= dt;
    if (m_thinkTimer <= 0.f) {
        m_thinkTimer = 0.5f;
        Think(dt);
    }

    // Обновление сущностей — по 1/4 списка за кадр (перф)
    size_t n = m_entities.size();
    if (n > 0) {
        size_t slice = std::max<size_t>(1, n / 4);
        for (size_t k = 0; k < slice && k < n; ++k) {
            size_t i = (m_rotateCursor + k) % n;
            UpdateEntity(i, dt);
        }
        m_rotateCursor = (m_rotateCursor + slice) % (n ? n : 1);
    }
}

void MutantDirector::Think(float dt) {
    (void)dt;
    CPed* player = PlayerPed();
    if (!player) return;

    const CVector& pp = player->GetPosition();
    uint32_t biome = BiomeAt(pp.x, pp.y);
    bool city = (biome & BIOME_CITY) != 0;

    // Целевая численность вокруг игрока (радикально меньше оригинала:
    // город ~6-9, дикость ~12; вместо ~25-30 педов у оригинала)
    int target = city ? (IsNight() ? m_cityNightTarget : m_cityTarget) : m_wildTarget;
    target = std::max(1, (int)(target * m_densityMul));

    int alive = AliveMutantsNear(160.f);
    if (alive < target) {
        SpawnWave();
    }

    // Чистка трупов и далёких
    for (size_t i = 0; i < m_entities.size();) {
        SpawnedEntity& e = m_entities[i];
        bool remove = false;
        if (!e.ped || !IsValidPedPointer(e.ped)) {
            remove = true;
        } else {
            float d = (e.ped->GetPosition() - pp).Magnitude2D();
            bool dead = e.deathTime != 0;
            bool corpseOld = dead && (TimeInMS() - e.deathTime) > (uint32_t)(m_corpseLifetime * 1000.f);
            if (d > m_cullDist && !dead) remove = true;       // слишком далеко — ресайкл
            else if (corpseOld) remove = true;                 // труп долго лежит
        }
        if (remove) {
            if (e.ped && IsValidPedPointer(e.ped))
                fn::PopulationRemovePed(e.ped);
            m_entities.erase(m_entities.begin() + i);
        } else {
            ++i;
        }
    }
}

// ---------------------------------------------------------------------------
// Спавн
// ---------------------------------------------------------------------------
bool MutantDirector::FindGroundPos(CVector& pos) {
    CColPoint col;
    void* hitEntity = nullptr;
    CVector origin(pos.x, pos.y, pos.z + 2.f);
    bool hit = fn::WorldProcessVerticalLine(origin, 60.f, col, hitEntity,
                                            true, false, false, false, false, true);
    if (hit) {
        // CColPoint::m_vecPoint — первая структура в классе (0x00)
        CVector& colPos = *reinterpret_cast<CVector*>(&col);
        pos.z = colPos.z + 1.f;
        return true;
    }
    return false;
}

bool MutantDirector::TrySpawnOne(int defIndex, const CVector& anchor, Rng& rng) {
    MutantCatalog& cat = MutantCatalog::Get();
    const MutantDef* def = cat.Get(defIndex);
    if (!def) return false;

    // Точка спавна
    float ang = rng.RangeF(0.f, 6.2831853f);
    float dist = rng.RangeF(m_spawnMinDist, m_spawnMaxDist);
    CVector pos(anchor.x + cosf(ang) * dist, anchor.y + sinf(ang) * dist, anchor.z);
    if (!FindGroundPos(pos)) return false;

    // Модель: готовая файловая или IMG-замена; иначе fallback
    int modelId = def->modelId;
    bool useFallback = false;
    if (modelId < 0) {
        modelId = def->fallbackId;
        useFallback = true;
    } else if (!IsModelLoaded(modelId)) {
        fn::StreamingRequestModel(modelId, 0x8 /*KEEP_IN_MEMORY*/);
        if (!IsModelLoaded(modelId)) {
            modelId = def->fallbackId;
            useFallback = true;
        }
    }

    CPed* ped = fn::PopulationAddPed(PED_TYPE_CIVMALE, modelId, pos, true /*wander*/);
    if (!ped) return false;

    fn::PedSetCharCreatedBy(ped, PED_MISSION); // не удаляется менеджером населения

    if (!useFallback) {
        ped->Health() = def->health;
        ped->MaxHealth() = def->health;
        if (def->speedFactor != 1.f)
            fn::PedSetMoveAnimSpeed(ped, def->speedFactor);
    } else {
        // Фолбэк-пед: здоровье масштабируем от силы мутанта
        float k = def->health / 100.f;
        ped->Health() = def->health;
        ped->MaxHealth() = def->health;
        if (k > 2.f) { // «толстяки» заметно крупнее
            ped->Health() = def->health;
        }
        fn::PedSetMoveAnimSpeed(ped, def->speedFactor);
    }

    SpawnedEntity e;
    e.ped = ped;
    e.defIndex = defIndex;
    e.spawnTime = TimeInMS();
    e.homePos = pos;
    m_entities.push_back(e);

    // Если мутант убегающий (тушканчик) — он просто бродит
    if (def->flees) return true;

    return true;
}

void MutantDirector::SpawnWave() {
    CPed* player = PlayerPed();
    if (!player) return;

    static Rng s_rng(StableHash("mutant_spawner"));
    const CVector& pp = player->GetPosition();
    uint32_t biome = BiomeAt(pp.x, pp.y);
    float night = IsNight() ? 1.f : 0.f;
    (void)night;

    // Пачка: 1..3 спавна за волну
    int attempts = 1 + s_rng.RangeI(0, 2);
    for (int i = 0; i < attempts; ++i) {
        int defIndex = MutantCatalog::Get().PickForBiome(biome, IsNight() ? 1.f : 0.f, s_rng);
        if (defIndex < 0) break;
        const MutantDef* def = MutantCatalog::Get().Get(defIndex);

        // Групповые животные — сразу стая
        int group = def ? s_rng.RangeI(def->minGroup, def->maxGroup) : 1;
        for (int g = 0; g < group; ++g) {
            if (!TrySpawnOne(defIndex, pp, s_rng)) break;
        }
    }
}

// ---------------------------------------------------------------------------
// Поведение
// ---------------------------------------------------------------------------
void MutantDirector::MakeAggressive(SpawnedEntity& e) {
    CPed* player = PlayerPed();
    if (!e.ped || !player) return;
    e.aggro = true;

    // Движковый ИИ: ненависть к PED_TYPE_PLAYER1 + событие
    e.ped->HateMask() |= 1u;
    if (void* group = e.ped->EventGroup()) {
        uint8_t evBuf[0x20];
        memset(evBuf, 0, sizeof(evBuf));
        void* ev = fn::ConstructEventAcquaintancePedHate(evBuf, player);
        if (ev) fn::EventGroupAdd(group, ev, false);
    }
}

void MutantDirector::CalmDown(SpawnedEntity& e) {
    if (!e.ped) return;
    e.aggro = false;
    // Убираем ненависть и возвращаем блуждание
    e.ped->HateMask() &= ~1u;
    if (void* group = e.ped->EventGroup()) {
        uint8_t taskBuf[0x28];
        memset(taskBuf, 0, sizeof(taskBuf));
        void* task = fn::ConstructTaskWander(taskBuf, 2 /*PEDMOVE_WALK*/, 0, true, 0.5f);
        if (task) {
            uint8_t evBuf[0x18];
            memset(evBuf, 0, sizeof(evBuf));
            void* ev = fn::ConstructEventScriptCommand(evBuf, 3 /*TASK_PRIMARY_PRIMARY*/, task, false);
            if (ev) fn::EventGroupAdd(group, ev, false);
        }
    }
}

void MutantDirector::UpdateEntity(size_t idx, float dt) {
    (void)dt;
    if (idx >= m_entities.size()) return;
    SpawnedEntity& e = m_entities[idx];
    if (!e.ped) return;

    // Валидность
    if (!IsValidPedPointer(e.ped)) { e.ped = nullptr; return; }

    const MutantDef* def = MutantCatalog::Get().Get(e.defIndex);
    CPed* player = PlayerPed();
    if (!player) return;

    // Смерть
    if (!e.ped->IsAlive()) {
        if (e.deathTime == 0) {
            e.deathTime = TimeInMS();
            if (e.defIndex >= 0) {
                ++m_kills;
                ModContext::Get().player.mutantsKilled++;
            }
        }
        return;
    }

    float dist = (e.ped->GetPosition() - player->GetPosition()).Magnitude();
    float dist2d = (e.ped->GetPosition() - player->GetPosition()).Magnitude2D();

    const MutantDef* d = def;
    float aggroR = d ? d->aggroRadius : 35.f;
    if (IsNight() && d) aggroR *= d->nightAggroMul;
    float deAggroR = d ? d->deAggroRadius : 70.f;

    // Пси-урон (контролёр/бюрер) — непрерывно в радиусе
    if (d && d->psiDamage > 0.f && dist < d->psiRadius) {
        DamagePlayerPed(d->psiDamage * dt);
    }

    if (d && d->flees) {
        // Тушканчики не атакуют — держим дистанцию задачей блуждания
        if (e.aggro) CalmDown(e);
        return;
    }

    if (!e.aggro) {
        if (dist < aggroR) {
            // Проверяем прямую видимость (здания/стены)
            bool los = fn::WorldGetLineOfSightClear(
                e.ped->GetPosition() + CVector(0, 0, 1.f),
                player->GetPosition() + CVector(0, 0, 1.f),
                true, false, false, false, false, true, true);
            if (los || dist < 8.f) // вплотную — и через стену слышно
                MakeAggressive(e);
        }
    } else {
        // Прямой урон при контакте (кроме урона движка от «кулаков»)
        ModContext& ctx = ModContext::Get();
        if (d && dist < d->attackRange + 0.6f &&
            ctx.sessionTime >= e.nextAttackAt) {
            float dmg = d->damage;
            // Противогаз немного защищает от пси-атаак и укусов? Нет — только радиация.
            DamagePlayerPed(dmg);
            e.nextAttackAt = ctx.sessionTime + d->attackCooldown;
            fn::CameraShake(0.25f, 1, 0.5f);
        }

        // Деаггро
        if (dist > deAggroR) {
            CalmDown(e);
            // Территориальные возвращаются «домой» — новая задача блуждания уже стоит
        }
        (void)dist2d;
    }
}

} // namespace stalker
