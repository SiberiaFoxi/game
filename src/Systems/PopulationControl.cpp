#include "PopulationControl.h"
#include <cstring>
#include "MutantDirector.h"
#include "../Core/Config.h"
#include "../Core/Log.h"
#include "../Core/ModState.h"
#include "../Core/Rng.h"
#include "../Gta/GtaFunctions.h"

#include <cmath>

namespace stalker {

using namespace gta;

PopulationControl& PopulationControl::Get() {
    static PopulationControl inst;
    return inst;
}

void PopulationControl::LoadFactions() {
    m_factions.clear();
    ModContext& ctx = ModContext::Get();

    if (ctx.cfgFactions) {
        Config& c = *ctx.cfgFactions;
        auto sections = c.GetSectionsWithPrefix("faction:");
        for (auto& s : sections) {
            FactionDef f;
            f.name = s.substr(8);
            f.title = c.GetString(s, "title", f.name.c_str());
            f.pedModelId = c.GetInt(s, "ped_model_id", 7);
            f.weaponId = c.GetInt(s, "weapon_id", 0);
            f.hostileToPlayer = c.GetBool(s, "hostile", false);
            f.trader = c.GetBool(s, "trader", false);
            f.spawnWeight = c.GetFloat(s, "weight", 1.f);
            m_factions.push_back(f);
        }
    }

    if (m_factions.empty()) {
        // Дефолты: пед-модель — стандартные педы SA
        // (7=male01 и др.; меняются в factions.cfg)
        FactionDef stalker { "stalkers", "Stalkers", 7 /*male01*/,  0 /*без оружия*/, false, true,  3.f };
        FactionDef bandit { "bandits",  "Bandits",  102,            22 /*пистолет*/, true,  false, 2.f };
        FactionDef army   { "army",     "Military", 287,            31 /*M4*/,       true,  false, 1.f };
        FactionDef monolit{ "monolith", "Monolith", 286,            31,              true,  false, 0.5f };
        FactionDef freedom{ "freedom",  "Freedom",  155,            25 /*дробовик*/, false, false, 0.8f };
        FactionDef duty   { "duty",     "Duty",     103,            30 /*АК-47*/,    false, false, 0.8f };
        m_factions = { stalker, bandit, army, monolit, freedom, duty };
    }
    Log::Write("PopulationControl: %d фракций", (int)m_factions.size());
}

void PopulationControl::OnSessionStart() {
    if (ModContext& ctx = ModContext::Get(); ctx.cfgGeneral) {
        Config& c = *ctx.cfgGeneral;
        m_density = c.GetFloat("population", "ambient_ped_multiplier", 0.12f);
        m_maxHumans = c.GetInt("population", "faction_humans_max", 2);
        m_noAmbientCops = c.GetBool("population", "no_random_cops", true);
        m_noAmbientGangs = c.GetBool("population", "no_random_gangs", true);
    }
    // Сразу применяем плотность
    PedDensityMultiplier() = m_density;
    DontCreateRandomCops() = m_noAmbientCops;
    DontCreateRandomGangs() = m_noAmbientGangs;
}

int PopulationControl::HumansAlive() const {
    int n = 0;
    for (auto& e : MutantDirector::Get().Entities())
        if (e.ped && e.defIndex < 0 && e.factionId >= 0 && e.ped->IsAlive())
            ++n;
    return n;
}

void PopulationControl::MaintainHumans() {
    CPed* player = PlayerPed();
    if (!player) return;
    if (HumansAlive() >= m_maxHumans) return;

    static Rng rng(StableHash("humans"));

    // Взвешенный выбор фракции
    float total = 0.f;
    for (auto& f : m_factions) total += f.spawnWeight;
    if (total <= 0.f) return;
    float pick = rng.NextFloat() * total;
    const FactionDef* chosen = &m_factions[0];
    for (auto& f : m_factions) {
        pick -= f.spawnWeight;
        if (pick <= 0.f) { chosen = &f; break; }
    }

    // Точка спавна подальше от игрока
    float ang = rng.RangeF(0.f, 6.2831853f);
    float dist = rng.RangeF(50.f, 110.f);
    const CVector& pp = player->GetPosition();
    CVector pos(pp.x + cosf(ang) * dist, pp.y + sinf(ang) * dist, pp.z);

    CColPoint col;
    void* hitEnt = nullptr;
    CVector origin(pos.x, pos.y, pos.z + 2.f);
    if (!fn::WorldProcessVerticalLine(origin, 60.f, col, hitEnt, true, false, false, false, false, true))
        return;
    pos.z = reinterpret_cast<CVector&>(col).z + 1.f;

    int modelId = chosen->pedModelId;
    if (!IsModelLoaded(modelId))
        fn::StreamingRequestModel(modelId, 0x8);
    if (!IsModelLoaded(modelId)) return; // попробуем в следующий раз

    CPed* ped = fn::PopulationAddPed(PED_TYPE_CIVMALE, modelId, pos, true);
    if (!ped) return;
    fn::PedSetCharCreatedBy(ped, PED_MISSION);

    if (chosen->weaponId > 0) {
        fn::PedGiveDelayedWeapon(ped, chosen->weaponId, 25001);
        fn::PedSetCurrentWeapon(ped, chosen->weaponId);
    }
    if (chosen->hostileToPlayer) {
        ped->HateMask() |= 1u;
        if (void* group = ped->EventGroup()) {
            uint8_t evBuf[0x20];
            memset(evBuf, 0, sizeof(evBuf));
            void* ev = fn::ConstructEventAcquaintancePedHate(evBuf, player);
            if (ev) fn::EventGroupAdd(group, ev, false);
        }
    }

    SpawnedEntity e;
    e.ped = ped;
    e.defIndex = -1;
    e.factionId = (int)(chosen - m_factions.data());
    e.spawnTime = TimeInMS();
    e.homePos = pos;
    MutantDirector::Get().Entities().push_back(e);

    Log::Write("PopulationControl: спавн NPC '%s'", chosen->name.c_str());
}

void PopulationControl::Tick(float dt) {
    if (!IsGameSessionActive() || UserPause()) return;

    // Постоянно придерживаем плотность (игра и скрипты могут её менять)
    PedDensityMultiplier() = m_density;
    DontCreateRandomCops() = m_noAmbientCops;
    DontCreateRandomGangs() = m_noAmbientGangs;

    m_thinkTimer -= dt;
    if (m_thinkTimer <= 0.f) {
        m_thinkTimer = 8.f;
        if (HumansAlive() < m_maxHumans)
            MaintainHumans();
    }
}

} // namespace stalker
