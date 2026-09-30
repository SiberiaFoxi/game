#include "RadiationSystem.h"
#include "AnomalySystem.h"
#include "../Core/Config.h"
#include "../Core/Log.h"
#include "../Core/ModState.h"
#include "../Gta/GtaFunctions.h"

#include <cmath>
#include <algorithm>

namespace stalker {

using namespace gta;

RadiationSystem& RadiationSystem::Get() {
    static RadiationSystem inst;
    return inst;
}

void RadiationSystem::LoadZones() {
    m_zones.clear();
    ModContext& ctx = ModContext::Get();
    if (!ctx.cfgZones) return;

    auto sections = ctx.cfgZones->GetSectionsWithPrefix("radzone:");
    for (auto& s : sections) {
        Config& c = *ctx.cfgZones;
        RadZone z;
        z.name = c.GetString(s, "name", s.c_str());
        Vec3 p = ParseVec3(c.GetString(s, "center", "0, 0, 0"));
        z.center = CVector(p.x, p.y, p.z);
        z.radius = c.GetFloat(s, "radius", 60.f);
        z.intensity = c.GetFloat(s, "intensity", 1.f);
        m_zones.push_back(z);
    }
    Log::Write("RadiationSystem: %d радиационных зон", (int)m_zones.size());
}

const char* RadiationSystem::CurrentZoneName() const {
    if (m_currentZone >= 0 && m_currentZone < (int)m_zones.size())
        return m_zones[m_currentZone].name.c_str();
    return "";
}

float RadiationSystem::ProtectionFactor() const {
    ModContext& ctx = ModContext::Get();
    const PlayerState& p = ctx.player;
    float prot = 0.f;
    if (p.maskEquipped && p.filterLeft > 0.f)
        prot += m_maskProtection;
    prot += p.artifactRadResist;
    return std::min(prot, 0.95f);
}

void RadiationSystem::Tick(float dt) {
    if (!IsGameSessionActive() || UserPause()) return;

    ModContext& ctx = ModContext::Get();
    PlayerState& p = ctx.player;
    CPed* player = PlayerPed();
    if (!player) return;

    const CVector& pos = player->GetPosition();

    // Мощность поля: сумма вкладов зон + аномалии (химические)
    float intensity = 0.f;
    m_currentZone = -1;
    for (size_t i = 0; i < m_zones.size(); ++i) {
        const RadZone& z = m_zones[i];
        float d = (z.center - pos).Magnitude();
        if (d < z.radius) {
            // Плавное затухание к краю зоны
            float falloff = 1.f - (d / z.radius);
            intensity += z.intensity * falloff;
            if (m_currentZone < 0) m_currentZone = (int)i;
        }
    }
    // Химические аномалии добавляют радиацию
    intensity += AnomalySystem::Get().RadFieldAt(pos);

    m_intensity = intensity;

    // Износ фильтра противогаза в зоне
    if (p.maskEquipped && intensity > 0.02f && p.filterLeft > 0.f) {
        p.filterLeft -= dt * (0.5f + intensity * 0.5f); // фильтр живёт дольше в слабом поле
        if (p.filterLeft <= 0.f) {
            p.filterLeft = 0.f;
            Log::Write("RadiationSystem: фильтр противогаза выработан");
        }
    }

    // Набор дозы с учётом защиты
    if (intensity > 0.001f) {
        float prot = ProtectionFactor();
        float gain = intensity * (1.f - prot) * dt * 10.f; // масштаб в «бэр»
        p.radDose += gain;
    } else {
        // Естественное выведение
        p.radDose -= m_naturalDecay * dt;
    }
    p.radDose = std::max(0.f, std::min(p.radDose, 1000.f));
    m_dose = p.radDose;

    // Артефакты: снижение дозы
    if (p.artifactRadHeal > 0.f)
        p.radDose = std::max(0.f, p.radDose - p.artifactRadHeal * dt);

    // Урон от дозы
    if (p.radDose > m_damageThreshold) {
        float over = (p.radDose - m_damageThreshold) / (1000.f - m_damageThreshold);
        float dmg = m_damagePerSec * over * dt;
        DamagePlayerPed(dmg);
    }

    // Урон и регенерация HP
    if (p.artifactHealthRegen > 0.f && player->Health() > 0.f)
        HealPlayerPed(p.artifactHealthRegen * dt);
    if (p.hungerRegenTimer > 0.f) {
        p.hungerRegenTimer -= dt;
        HealPlayerPed(1.5f * dt); // плотный обед
    }
}

} // namespace stalker
