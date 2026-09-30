#pragma once
// ============================================================================
//  Звук: щелчки счётчика Гейгера, сигналы детектора, тревога выброса.
//  Win32 Beep() в выделенном потоке (неблокирующе для игры).
// ============================================================================
#include <cstdint>

namespace stalker {

class AudioSystem {
public:
    static AudioSystem& Get();

    void Start();   // запускает поток
    void Stop();

    // Гейгер: щелчков в секунду (0 = тишина)
    void SetGeigerRate(float cps);

    // Однократный сигнал (детектор: «пиип»)
    void BeepOnce(uint32_t freq, uint32_t ms);

    // Тревога выброса (короткие частые сигналы, N секунд)
    void Alarm(uint32_t seconds);

private:
    AudioSystem() = default;

    struct Shared {
        volatile bool     run = false;
        volatile float    geigerCps = 0.f;
        volatile uint32_t alarmUntilMs = 0;
        volatile bool     oncePending = false;
        volatile uint32_t onceFreq = 1000;
        volatile uint32_t onceMs = 60;
    };

    Shared m_shared;
};

} // namespace stalker
