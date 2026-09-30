#pragma once
// ============================================================================
//  Радиация: зоны, накопление дозы, урон, счётчик Гейгера, защита противогазом.
//  Зоны задаются в STALKER_MOD/zones.cfg: [radzone:имя] center=..., radius=...,
//  intensity=...
// ============================================================================
#include <cstdint>
#include <string>
#include <vector>
#include "../Gta/GtaTypes.h"

namespace stalker {

struct RadZone {
    std::string name;
    gta::CVector center;
    float radius = 50.f;
    float intensity = 1.f; // мкР/с (условные единицы)
};

class RadiationSystem {
public:
    static RadiationSystem& Get();

    void LoadZones();            // из cfgZones
    void Tick(float dt);        // накопление, урон, гейгер

    float Dose() const { return m_dose; }
    float Intensity() const { return m_intensity; }
    const char* CurrentZoneName() const;

    void ResetDose() { m_dose = 0.f; } // чит STALKERRAD

    // Защита: противогаз (0.85) + артефакты (модификатор)
    float ProtectionFactor() const;

private:
    RadiationSystem() = default;

    std::vector<RadZone> m_zones;
    float m_dose = 0.f;          // 0..1000
    float m_intensity = 0.f;     // текущая
    int   m_currentZone = -1;

    // Настройки
    float m_damageThreshold = 60.f;
    float m_damagePerSec    = 1.2f;   // при дозе 1000
    float m_maskProtection  = 0.85f;
    float m_naturalDecay    = 0.02f;  // доза/сек естественное выведение
};

} // namespace stalker
