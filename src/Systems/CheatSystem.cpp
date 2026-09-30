#include "CheatSystem.h"
#include <cstring>
#include "BlowoutSystem.h"
#include "MutantDirector.h"
#include "../Core/Log.h"
#include "../Core/ModState.h"
#include "../Core/Rng.h"
#include "../Gta/GtaFunctions.h"

#include <cctype>
#include <vector>

namespace stalker {

using namespace gta;

CheatSystem& CheatSystem::Get() {
    static CheatSystem inst;
    return inst;
}

void CheatSystem::OnChar(char c) {
    c = (char)std::toupper(static_cast<unsigned char>(c));
    if (c < 'A' || c > 'Z') return;

    m_buffer.push_back(c);
    if (m_buffer.size() > 32) m_buffer.erase(0, m_buffer.size() - 32);

    static const char* codes[] = {
        "STALKERHEALTH", "STALKERARMOR", "STALKERARTIFACT", "STALKERRAD",
        "STALKERWEATHER", "STALKERDETECTOR", "STALKERNOMUTANTS",
    };
    for (const char* code : codes) {
        size_t len = strlen(code);
        if (m_buffer.size() < len) continue;
        if (m_buffer.compare(m_buffer.size() - len, len, code) == 0) {
            Execute(code);
            m_buffer.clear();
            return;
        }
    }
}

void CheatSystem::Execute(const std::string& code) {
    ++m_activated;
    ModContext& ctx = ModContext::Get();
    PlayerState& p = ctx.player;
    CPed* player = PlayerPed();

    if (code == "STALKERHEALTH") {
        if (player) {
            player->Health() = 100.f;
            player->MaxHealth() = 100.f;
        }
        Log::Write("Чит: здоровье восстановлено");
    } else if (code == "STALKERARMOR") {
        if (player) player->Armour() = 100.f;
        Log::Write("Чит: броня выдана");
    } else if (code == "STALKERARTIFACT") {
        // Случайный артефакт в инвентарь
        std::vector<int> arts;
        for (size_t i = 0; i < ctx.items.size(); ++i)
            if (ctx.items[i].type == ItemType::Artifact) arts.push_back((int)i);
        if (!arts.empty()) {
            Rng rng(TimeInMS());
            p.AddItem(arts[rng.RangeI(0, (int)arts.size() - 1)], 1);
        }
        Log::Write("Чит: артефакт получен");
    } else if (code == "STALKERRAD") {
        p.radDose = 0.f;
        Log::Write("Чит: радиация сброшена");
    } else if (code == "STALKERWEATHER") {
        BlowoutSystem::Get().ForceBlowout();
    } else if (code == "STALKERDETECTOR") {
        p.AddItem(FindItemIndex("detector_veles"), 1);
        p.equippedDetector = FindItemIndex("detector_veles");
        Log::Write("Чит: детектор «Велес» получен");
    } else if (code == "STALKERNOMUTANTS") {
        ctx.nomutantsCheat = !ctx.nomutantsCheat;
        if (ctx.nomutantsCheat) MutantDirector::Get().PurgeAll(true);
        Log::Write("Чит: спавн мутантов %s", ctx.nomutantsCheat ? "ВЫКЛ" : "ВКЛ");
    }
}

} // namespace stalker
