#include "KeyState.h"

namespace stalker {

void KeyState::Update(const int16_t* newKeys, const int16_t* oldKeys) {
    if (!newKeys || !oldKeys) return;
    for (int i = 0; i < 256; ++i) {
        m_down[i]    = newKeys[i] != 0;
        m_pressed[i] = (newKeys[i] != 0) && (oldKeys[i] == 0);
    }
}

} // namespace stalker
