#include "AnomalySystem.h"
#include <cstring>
#include "../Core/Config.h"
#include "../Core/Log.h"
#include "../Core/ModState.h"
#include "../Core/Rng.h"
#include "../Gta/GtaFunctions.h"

#include <cmath>
#include <algorithm>

namespace stalker {

using namespace gta;

AnomalySystem& AnomalySystem::Get() {
    static AnomalySystem inst;
    return inst;
}

const char* AnomalyTypeName(AnomalyType t) {
    switch (t) {
    case ANOMALY_GRAVITY:   return "Gravi anomaly";
    case ANOMALY_ELECTRO:   return "Electra anomaly";
    case ANOMALY_THERMAL:   return "Fryer anomaly";
    case ANOMALY_CHEMICAL:  return "Fruitpunch anomaly";
    default:                return "Anomaly";
    }
}

void AnomalySystem::LoadFields() {
    m_fields.clear();
    ModContext& ctx = ModContext::Get();

    if (ctx.cfgGeneral) {
        Config& c = *ctx.cfgGeneral;
        m_gravDamage     = c.GetFloat("anomalies", "gravi_damage", 30.f);
        m_electroDamage  = c.GetFloat("anomalies", "electro_damage", 45.f);
        m_thermalDamage  = c.GetFloat("anomalies", "thermal_dps", 12.f);
        m_chemRad        = c.GetFloat("anomalies", "chem_rad", 1.2f);
        m_detectorRange  = c.GetFloat("anomalies", "detector_range", 15.f);
    }

    if (!ctx.cfgZones) return;
    auto sections = ctx.cfgZones->GetSectionsWithPrefix("anomaly_field:");
    for (auto& s : sections) {
        Config& c = *ctx.cfgZones;
        AnomalyField f;
        f.name = s.substr(15);
        Vec3 p = ParseVec3(c.GetString(s, "center", "0, 0, 0"));
        f.center = CVector(p.x, p.y, p.z);
        f.radius = c.GetFloat(s, "radius", 40.f);
        f.count  = c.GetInt(s, "count", 5);
        f.allowedTypes = (uint8_t)c.GetInt(s, "types_mask", 0xF);
        f.artifactChance = c.GetFloat(s, "artifact_chance", 0.7f);
        f.artifactRespawnSec = c.GetFloat(s, "artifact_respawn_sec", 300.f);
        f.generated = false;
        m_fields.push_back(f);
    }
    Log::Write("AnomalySystem: %d полей аномалий", (int)m_fields.size());
}

void AnomalySystem::GenerateField(AnomalyField& f) {
    f.anomalies.clear();
    f.artifacts.clear();

    // Детерминированная генерация по имени поля — одинаково в каждой сессии
    Rng rng(StableHash(f.name.c_str()));

    for (int i = 0; i < f.count; ++i) {
        Anomaly a;
        float ang = rng.RangeF(0.f, 6.2831853f);
        float d = rng.RangeF(3.f, f.radius);
        a.pos = CVector(f.center.x + cosf(ang) * d,
                        f.center.y + sinf(ang) * d,
                        f.center.z);
        // Тип — из допустимых
        AnomalyType t;
        int guard = 0;
        do {
            t = (AnomalyType)rng.RangeI(0, ANOMALY_COUNT - 1);
        } while (!(f.allowedTypes & (1 << t)) && ++guard < 32);
        a.type = t;
        a.radius = rng.RangeF(2.0f, 3.2f);
        a.visualSize = rng.RangeF(1.2f, 2.0f);
        a.cooldownUntil = 0.f;
        f.anomalies.push_back(a);
    }

    // Артефакты: у части аномалий
    for (auto& a : f.anomalies) {
        if (!rng.Chance(f.artifactChance)) continue;
        ArtifactSpawn art;
        float ang = rng.RangeF(0.f, 6.2831853f);
        float d = rng.RangeF(1.5f, 4.f);
        art.pos = CVector(a.pos.x + cosf(ang) * d, a.pos.y + sinf(ang) * d, a.pos.z);
        // Случайный артефакт из каталога
        auto& items = ModContext::Get().items;
        std::vector<int> arts;
        for (size_t k = 0; k < items.size(); ++k)
            if (items[k].type == ItemType::Artifact) arts.push_back((int)k);
        if (!arts.empty()) {
            art.itemDefIndex = arts[rng.RangeI(0, (int)arts.size() - 1)];
            art.present = true;
            art.respawnAt = 0.f;
            f.artifacts.push_back(art);
        }
    }
}

void AnomalySystem::OnSessionStart() {
    LoadFields();
    for (auto& f : m_fields) f.generated = false;
}

void AnomalySystem::OnSessionEnd() {
    m_fields.clear();
}

float AnomalySystem::RadFieldAt(const CVector& pos) const {
    float rad = 0.f;
    for (auto& f : m_fields) {
        if (!f.generated) continue;
        for (auto& a : f.anomalies) {
            if (a.type != ANOMALY_CHEMICAL) continue;
            float d = (a.pos - pos).Magnitude();
            if (d < a.radius) rad += m_chemRad * (1.f - d / a.radius);
        }
    }
    return rad;
}

const Anomaly* AnomalySystem::NearestAnomaly(const CVector& pos, float maxDist, float* outDist) const {
    const Anomaly* best = nullptr;
    float bestD = maxDist;
    for (auto& f : m_fields) {
        if (!f.generated) continue;
        for (auto& a : f.anomalies) {
            float d = (a.pos - pos).Magnitude();
            if (d < bestD) { bestD = d; best = &a; }
        }
    }
    if (outDist) *outDist = bestD;
    return best;
}

const ArtifactSpawn* AnomalySystem::NearestArtifact(const CVector& pos, float maxDist, float* outDist) const {
    const ArtifactSpawn* best = nullptr;
    float bestD = maxDist;
    for (auto& f : m_fields) {
        if (!f.generated) continue;
        for (auto& a : f.artifacts) {
            if (!a.present) continue;
            float d = (a.pos - pos).Magnitude();
            if (d < bestD) { bestD = d; best = &a; }
        }
    }
    if (outDist) *outDist = bestD;
    return best;
}

int32_t AnomalySystem::TryPickupArtifact(const CVector& pos) {
    for (auto& f : m_fields) {
        if (!f.generated) continue;
        for (auto& a : f.artifacts) {
            if (!a.present) continue;
            if ((a.pos - pos).Magnitude() < 1.6f) {
                a.present = false;
                ModContext& ctx = ModContext::Get();
                a.respawnAt = ctx.sessionTime + f.artifactRespawnSec;
                ctx.player.artifactsFound++;
                return a.itemDefIndex;
            }
        }
    }
    return -1;
}

void AnomalySystem::BlowoutArtifactBurst() {
    // После выброса часть аномалий «рождает» артефакты
    for (auto& f : m_fields) {
        if (!f.generated) continue;
        for (auto& a : f.artifacts) {
            if (!a.present) a.respawnAt = 0.f; // восстановить
        }
        // и немного новых рядом с центром
        Rng rng(StableHash(f.name.c_str()) ^ 0xB10A0177u);
        for (int i = 0; i < rng.RangeI(1, 3); ++i) {
            ArtifactSpawn art;
            float ang = rng.RangeF(0.f, 6.2831853f);
            float d = rng.RangeF(2.f, f.radius * 0.6f);
            art.pos = CVector(f.center.x + cosf(ang) * d, f.center.y + sinf(ang) * d, f.center.z);
            auto& items = ModContext::Get().items;
            std::vector<int> arts;
            for (size_t k = 0; k < items.size(); ++k)
                if (items[k].type == ItemType::Artifact) arts.push_back((int)k);
            if (!arts.empty()) {
                art.itemDefIndex = arts[rng.RangeI(0, (int)arts.size() - 1)];
                art.present = true;
                f.artifacts.push_back(art);
            }
        }
    }
}

void AnomalySystem::GetArtifactState(std::vector<float>& outTimers) const {
    outTimers.clear();
    for (auto& f : m_fields) {
        for (auto& a : f.artifacts) {
            outTimers.push_back(a.present ? 1.f : 0.f);
            outTimers.push_back(a.respawnAt);
        }
    }
}

void AnomalySystem::SetArtifactState(const std::vector<float>& timers) {
    size_t i = 0;
    for (auto& f : m_fields) {
        for (auto& a : f.artifacts) {
            if (i + 1 >= timers.size()) return;
            a.present = timers[i] > 0.5f;
            a.respawnAt = timers[i + 1];
            i += 2;
        }
    }
}

void AnomalySystem::Tick(float dt) {
    if (!IsGameSessionActive() || UserPause()) return;

    ModContext& ctx = ModContext::Get();
    CPed* player = PlayerPed();
    if (!player) return;
    const CVector& pos = player->GetPosition();

    // Активация полей рядом с игроком
    for (auto& f : m_fields) {
        if (!f.generated) {
            if ((f.center - pos).Magnitude2D() < f.radius + 300.f) {
                GenerateField(f);
                Log::Write("AnomalySystem: поле '%s' активировано (%d аномалий)",
                           f.name.c_str(), (int)f.anomalies.size());
            }
        } else {
            // Далёкие поля сворачиваем (экономия)
            if ((f.center - pos).Magnitude2D() > f.radius + 600.f)
                f.generated = false;
        }
    }

    // Влияние аномалий на игрока
    for (auto& f : m_fields) {
        if (!f.generated) continue;
        // Возрождение артефактов
        for (auto& a : f.artifacts) {
            if (!a.present && ctx.sessionTime >= a.respawnAt)
                a.present = true;
        }
        for (auto& a : f.anomalies)
            ApplyDamage(a, dt);
    }
}

void AnomalySystem::ApplyDamage(Anomaly& a, float dt) {
    CPed* player = PlayerPed();
    if (!player) return;
    ModContext& ctx = ModContext::Get();
    const CVector& pos = player->GetPosition();

    float d = (a.pos - pos).Magnitude();
    if (d >= a.radius) return;
    float falloff = 1.f - d / a.radius;

    switch (a.type) {
    case ANOMALY_GRAVITY:
        // «Воронка»: разовый подброс вверх + урон при срабатывании
        if (ctx.sessionTime >= a.cooldownUntil) {
            CVector& vel = player->MoveSpeed();
            vel.z = 0.32f; // резкий подброс
            vel.x *= 0.6f;
            vel.y *= 0.6f;
            DamagePlayerPed(m_gravDamage * falloff);
            fn::CameraShake(0.4f, 1, 1.0f);
            a.cooldownUntil = ctx.sessionTime + 2.0f;
        }
        break;

    case ANOMALY_ELECTRO:
        // «Электра»: мощный разряд с паузой
        if (ctx.sessionTime >= a.cooldownUntil) {
            DamagePlayerPed(m_electroDamage * falloff);
            fn::CameraShake(0.3f, 1, 1.2f);
            a.cooldownUntil = ctx.sessionTime + 1.5f;
        }
        break;

    case ANOMALY_THERMAL:
        // «Жарка»: непрерывный ожог
        DamagePlayerPed(m_thermalDamage * falloff * dt);
        break;

    case ANOMALY_CHEMICAL:
        // «Кисляк»: радиация — см. RadFieldAt
        break;

    default:
        break;
    }
}

void AnomalySystem::DrawMarkers() {
    if (!IsGameSessionActive() || UserPause()) return;
    CPed* player = PlayerPed();
    if (!player) return;
    const CVector& pos = player->GetPosition();

    // Стабильные ID: 20..39 (не пересекаемся со скриптовыми маркерами)
    // id = 20 + (fieldIndex * 8 + anomalyIdx) % 20
    for (size_t f = 0; f < m_fields.size(); ++f) {
        AnomalyField& field = m_fields[f];
        if (!field.generated) continue;

        for (size_t a = 0; a < field.anomalies.size(); ++a) {
            uint32_t id = 20 + (uint32_t)((f * 8 + a) % 20);
            const Anomaly& an = field.anomalies[a];
            float d = (an.pos - pos).Magnitude();
            if (d > 140.f) {
                fn::CheckpointsDeleteCP(id, CHECKPOINT_TORUS);
                continue;
            }
            CRGBA color(255, 255, 255, 140);
            switch (an.type) {
            case ANOMALY_GRAVITY:  color = CRGBA(180, 60, 220, 150); break; // фиолет
            case ANOMALY_ELECTRO:  color = CRGBA(80, 140, 255, 170); break; // синий
            case ANOMALY_THERMAL:  color = CRGBA(255, 90, 40, 160);  break; // оранж
            case ANOMALY_CHEMICAL: color = CRGBA(120, 255, 80, 150); break; // зелёный
            default: break;
            }
            CVector dir(0, 0, 1);
            fn::CheckpointsPlaceMarker(id, CHECKPOINT_TORUS, an.pos, dir,
                                       an.visualSize, color, 2048, 0.45f, 0);
        }

        for (size_t k = 0; k < field.artifacts.size(); ++k) {
            uint32_t id = 20 + (uint32_t)((f * 8 + 7 + k) % 20);
            const ArtifactSpawn& art = field.artifacts[k];
            if (!art.present) {
                fn::CheckpointsDeleteCP(id, CHECKPOINT_EMPTYTUBE);
                continue;
            }
            float d = (art.pos - pos).Magnitude();
            if (d > 140.f) {
                fn::CheckpointsDeleteCP(id, CHECKPOINT_EMPTYTUBE);
                continue;
            }
            CVector dir(0, 0, 1);
            fn::CheckpointsPlaceMarker(id, CHECKPOINT_EMPTYTUBE, art.pos, dir,
                                       0.4f, CRGBA(255, 215, 60, 200), 1024, 0.6f, 2);
        }
    }
}

} // namespace stalker
