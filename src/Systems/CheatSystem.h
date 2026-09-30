#pragma once
// ============================================================================
//  Читы мода, вводятся как обычные читы SA (набором с клавиатуры):
//    STALKERHEALTH, STALKERARMOR, STALKERARTIFACT, STALKERRAD,
//    STALKERWEATHER, STALKERDETECTOR, STALKERNOMUTANTS
//  Перехват — PRE-хук CCheat::AddToCheatString (0x438480).
// ============================================================================
#include <cstdint>
#include <string>

namespace stalker {

class CheatSystem {
public:
    static CheatSystem& Get();

    // Вызывается из хука на каждый введённый символ
    void OnChar(char c);

    uint32_t ActivatedCount() const { return m_activated; }

private:
    CheatSystem() = default;

    void Execute(const std::string& code);

    std::string m_buffer; // последние 32 символа
    uint32_t m_activated = 0;
};

} // namespace stalker
