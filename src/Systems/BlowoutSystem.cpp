#include "BlowoutSystem.h"
#include "AnomalySystem.h"
#include "MutantDirector.h"
#include "../Core/Config.h"
#include "../Core/Log.h"
#include "../Core/ModState.h"
#include "../Core/Rng.h"
#include "../Gta/GtaFunctions.h"

#include <cmath>
#include <algorithm>

namespace stalker {

using namespace gta;

BlowoutSystem& BlowoutSystem::Get() {
    static BlowoutSystem inst;
    return inst;
}

void BlowoutSystem::OnSessionStart() {
    if (ModContext& ctx = ModContext::Get(); ctx.cfgGeneral) {
        Config& c = *ctx.cfgGeneral;
        m_intervalMin = c.GetFloat("blowout", "interval_min_sec", 1200.f);
        m_intervalMax = c.GetFloat("blowout", "interval_max_sec", 1800.f);
        m_warningSec  = c.GetFloat("blowout", "warning_sec", 60.f);
        m_activeSec   = c.GetFloat("blowout", "active_sec", 50.f);
        m_aftermathSec= c.GetFloat("blowout", "aftermath_sec", 15.f);
        m_dpsOutside  = c.GetFloat("blowout", "damage_outside_dps", 8.f);
    }
    Rng rng(StableHash("blowout"));
    m_timeUntil = rng.RangeF(m_intervalMin, m_intervalMax);
    m_phase = PHASE_IDLE;
    m_phaseTime = 0.f;
}

void BlowoutSystem::ForceBlowout() {
    if (m_phase != PHASE_IDLE) return;
    m_phase = PHASE_WARNING;
    m_phaseTime = m_warningSec;
    Log::Write("Blowout: принудительный выброс (чит)");
}

const char* BlowoutSystem::PhaseTitle() const {
    switch (m_phase) {
    case PHASE_WARNING: return "BLOWOUT IMMINENT - FIND COVER";
    case PHASE_ACTIVE:  return "BLOWOUT IN PROGRESS";
    case PHASE_AFTERMATH: return "Blowout is fading";
    default: return "";
    }
}

bool BlowoutSystem::IsPlayerInCover() const {
    // 1. Интерьер
    if (CurrArea() != 0) return true;

    // 2. Крыша над головой: вертикальная трассировка вверх
    CPed* player = PlayerPed();
    if (!player) return false;
    const CVector& pos = player->GetPosition();

    CColPoint col;
    void* hitEntity = nullptr;
    CVector origin(pos.x, pos.y, pos.z + 1.f);
    // ProcessVerticalLine идёт вниз от origin; для проверки «что сверху»
    // смотрим попадание над головой: origin на 60 м выше
    CVector high(pos.x, pos.y, pos.z + 60.f);
    bool hit = fn::WorldProcessVerticalLine(high, 58.f, col, hitEntity,
                                            true, true, false, true, false, true);
    return hit;
}

void BlowoutSystem::ApplySkyTint(float strength) {
    // CTimeCycle пересчитывает m_CurrentColours каждый кадр, поэтому
    // просто перекрашиваем поверх с учётом силы.
    CColourSet& cs = CurrentColours();
    auto lerpU16 = [strength](uint16_t& v, uint16_t target) {
        float t = strength;
        v = (uint16_t)(v * (1.f - t) + target * t);
    };
    lerpU16(cs.skyTopRed,    140); lerpU16(cs.skyTopGreen,   20); lerpU16(cs.skyTopBlue,   15);
    lerpU16(cs.skyBottomRed, 220); lerpU16(cs.skyBottomGreen, 45); lerpU16(cs.skyBottomBlue, 25);
    lerpU16(cs.sunCoreRed,   255); lerpU16(cs.sunCoreGreen,  90); lerpU16(cs.sunCoreBlue,   60);
    lerpU16(cs.lowCloudsRed, 200); lerpU16(cs.lowCloudsGreen, 40); lerpU16(cs.lowCloudsBlue, 25);
    cs.farClip = cs.farClip * (1.f - 0.35f * strength); // «сгущается» горизонт
}

void BlowoutSystem::DamageWorld(float dps) {
    // Урон всем педам вне укрытия (упрощённо: все, у кого есть крыша не
    // проверяем — только интерьеры), кроме игрока.
    float dt = DeltaTime();
    ForEachPed([&](CPed* ped) -> bool {
        if (!ped) return true;
        if (ped == PlayerPed()) return true;
        if (!ped->IsAlive()) return true;
        // В интерьере не трогаем
        if (ped->RawByte(0x2F) != 0) return true; // CEntity::m_AreaCode
        float& hp = ped->Health();
        hp -= dps * dt;
        if (hp < 0.f) hp = 0.f;
        return true;
    });
}

void BlowoutSystem::Tick(float dt) {
    if (!IsGameSessionActive() || UserPause()) return;

    switch (m_phase) {
    case PHASE_IDLE: {
        m_timeUntil -= dt;
        if (m_timeUntil <= 0.f) {
            m_phase = PHASE_WARNING;
            m_phaseTime = m_warningSec;
            Log::Write("Blowout: предупреждение (%.0f сек)", m_warningSec);
        }
        break;
    }

    case PHASE_WARNING: {
        m_phaseTime -= dt;
        ApplySkyTint(0.5f * (1.f - m_phaseTime / m_warningSec));
        // Тревожная тряска ближе к началу
        if (m_phaseTime < m_warningSec * 0.5f)
            fn::CameraShake(0.2f, 1, 0.35f);
        if (m_phaseTime <= 0.f) {
            m_phase = PHASE_ACTIVE;
            m_phaseTime = m_activeSec;
            Log::Write("Blowout: ВЫБРОС");
        }
        break;
    }

    case PHASE_ACTIVE: {
        m_phaseTime -= dt;
        ApplySkyTint(1.0f);
        fn::CameraShake(0.25f, 1, 0.8f);

        // Урон игроку вне укрытия
        if (!IsPlayerInCover()) {
            DamagePlayerPed(m_dpsOutside * dt);
        }
        // Мир гибнет
        DamageWorld(25.f);
        if (m_phaseTime <= 0.f) {
            m_phase = PHASE_AFTERMATH;
            m_phaseTime = m_aftermathSec;
            // Артефакты после выброса
            AnomalySystem::Get().BlowoutArtifactBurst();
            Log::Write("Blowout: окончание, аномалии «рождены» заново");
        }
        break;
    }

    case PHASE_AFTERMATH: {
        m_phaseTime -= dt;
        ApplySkyTint(std::max(0.f, m_phaseTime / m_aftermathSec) * 0.5f);
        if (m_phaseTime <= 0.f) {
            m_phase = PHASE_IDLE;
            Rng rng(StableHash("blowout") ^ TimeInMS());
            m_timeUntil = rng.RangeF(m_intervalMin, m_intervalMax);
            Log::Write("Blowout: затишье до %.0f сек", m_timeUntil);
        }
        break;
    }
    }
}

} // namespace stalker
