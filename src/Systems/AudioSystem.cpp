#include "AudioSystem.h"

#if !defined(STALKER_SYNTAX_CHECK)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace stalker {

AudioSystem& AudioSystem::Get() {
    static AudioSystem inst;
    return inst;
}

#if !defined(STALKER_SYNTAX_CHECK)

static DWORD WINAPI AudioThread(LPVOID param) {
    AudioSystem::Shared* s = reinterpret_cast<AudioSystem::Shared*>(param);

    uint32_t lastClick = GetTickCount();
    while (s->run) {
        uint32_t now = GetTickCount();

        // Тревога выброса — приоритетный сигнал
        if (now < s->alarmUntilMs) {
            Beep(880, 120);
            Sleep(180);
            Beep(660, 120);
            Sleep(180);
            continue;
        }

        // Одноразовый сигнал детектора
        if (s->oncePending) {
            s->oncePending = false;
            Beep(s->onceFreq, s->onceMs);
        }

        // Щелчки Гейгера (максимум ~20/сек, длительность щелчка 4 мс)
        float cps = s->geigerCps;
        if (cps > 0.05f) {
            if (cps > 20.f) cps = 20.f;
            uint32_t interval = (uint32_t)(1000.f / cps);
            if (now - lastClick >= interval) {
                lastClick = now;
                Beep(2400, 4);
            }
            Sleep(5);
        } else {
            Sleep(100);
        }
    }
    return 0;
}

void AudioSystem::Start() {
    if (m_shared.run) return;
    m_shared.run = true;
    HANDLE h = CreateThread(nullptr, 0, AudioThread, &m_shared, 0, nullptr);
    if (h) CloseHandle(h);
}

void AudioSystem::Stop() {
    m_shared.run = false;
}

void AudioSystem::SetGeigerRate(float cps) {
    m_shared.geigerCps = cps;
}

void AudioSystem::BeepOnce(uint32_t freq, uint32_t ms) {
    m_shared.onceFreq = freq;
    m_shared.onceMs = ms;
    m_shared.oncePending = true;
}

void AudioSystem::Alarm(uint32_t seconds) {
    m_shared.alarmUntilMs = GetTickCount() + seconds * 1000u;
}

#else // STALKER_SYNTAX_CHECK — заглушки

void AudioSystem::Start() {}
void AudioSystem::Stop() {}
void AudioSystem::SetGeigerRate(float) {}
void AudioSystem::BeepOnce(uint32_t, uint32_t) {}
void AudioSystem::Alarm(uint32_t) {}

#endif

} // namespace stalker
