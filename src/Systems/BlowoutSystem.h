#pragma once
// ============================================================================
//  Выброс: периодическое событие. Красное небо (правка CTimeCycle::
//  m_CurrentColours каждый кадр), урон вне укрытия, гибель NPC на открытом
//  воздухе, выброс артефактов после окончания.
// ============================================================================
#include <cstdint>

namespace stalker {

class BlowoutSystem {
public:
    static BlowoutSystem& Get();

    enum Phase : uint8_t {
        PHASE_IDLE = 0,     // затишье
        PHASE_WARNING,      // предупреждение
        PHASE_ACTIVE,       // сам выброс
        PHASE_AFTERMATH,    // угасание
    };

    void Tick(float dt);
    void ForceBlowout();          // чит STALKERWEATHER
    void OnSessionStart();

    Phase  GetPhase() const { return m_phase; }
    float  TimeUntilBlowout() const { return m_timeUntil; }
    float  PhaseTimeLeft() const { return m_phaseTime; }

    // Позиция для HUD (сек до выброса и т.п.)
    const char* PhaseTitle() const;

    // Проверка укрытия игрока (интерьер или крыша над головой)
    bool IsPlayerInCover() const;

private:
    BlowoutSystem() = default;

    void ApplySkyTint(float strength);
    void DamageWorld(float dps);

    Phase  m_phase = PHASE_IDLE;
    float  m_timeUntil = 900.f;   // до следующего выброса
    float  m_phaseTime = 0.f;     // время фазы

    // Настройки
    float  m_intervalMin = 1200.f; // 20 минут
    float  m_intervalMax = 1800.f; // 30 минут
    float  m_warningSec  = 60.f;
    float  m_activeSec   = 50.f;
    float  m_aftermathSec = 15.f;
    float  m_dpsOutside  = 8.f;
};

} // namespace stalker
