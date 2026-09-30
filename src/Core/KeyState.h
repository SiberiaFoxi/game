#pragma once
// ============================================================================
//  Ввод с клавиатуры через CPad::NewKeyState / OldKeyState игры
//  (gta-reversed: source/game_sa/Pad.h; состояния 0xB73190 / 0xB72F20).
//  Даёт "нажато в этом кадре" (edge) без GetAsyncKeyState — работает только
//  когда окно игры в фокусе.
// ============================================================================
#include <cstdint>

namespace stalker {

class KeyState {
public:
    // Вызывать раз в кадр до опроса
    void Update(const int16_t* newKeys, const int16_t* oldKeys);

    bool Down(int vk)   const { return m_down[vk & 0xFF]; }
    bool Pressed(int vk) const { return m_pressed[vk & 0xFF]; }

private:
    bool m_down[256]   = {};
    bool m_pressed[256] = {};
};

// Горячие клавиши по умолчанию (настраиваются в stalker_mod.cfg, секция [keys])
struct KeyBindings {
    int inventory   = 'I';
    int use         = 13;    // Enter
    int up          = 0x26;  // стрелка вверх (код CKeyboardState::up — читаем напрямую)
    int down        = 0x28;
    int left        = 0x25;
    int right       = 0x27;
    int gasMask     = 'K';
    int drop        = 'U';
    int trade       = 'T';
    int manualSave  = 0x78;  // F9
    int debugPage   = 0x79;  // F10
};

} // namespace stalker
