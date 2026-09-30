#pragma once
// ============================================================================
//  Полное отключение миссий GTA San Andreas.
//
//  Все миссии запускаются командой LAUNCH_MISSION: скрипт читается из
//  main.scm и стартует как CRunningScript с флагами
//  m_UsesMissionCleanup = true и m_ThisMustBeTheOnlyMissionRunning = true
//  (gta-reversed: source/game_sa/Scripts/Commands/MissionCommands.cpp,
//   LoadAndLaunchMissionInternal).
//
//  Блокировка: хук CRunningScript::Process (0x469F00). Если скрипт —
//  миссионный (или внешний — телефон, гонки, школы и пр.), вызываем
//  ShutdownThisScript (0x465AA0) и пропускаем выполнение. Мир остаётся
//  песочницей выживания: сюжет, побочки, гонки, школы, ограбления,
//  телефонные звонки — ничего не стартует.
// ============================================================================
#include <cstdint>

namespace gta { struct CRunningScript; }

namespace stalker {

class MissionBlocker {
public:
    static MissionBlocker& Get();

    // Вызывается из хука Process. true = пропустить выполнение скрипта.
    bool ShouldBlockScript(gta::CRunningScript* script);

    uint32_t BlockedCount() const { return m_blocked; }

    bool blockMissions  = true; // миссии (сюжет/побочные/фракции)
    bool blockExternal  = true; // внешние скрипты (телефон, гонки, школы...)

private:
    MissionBlocker() = default;
    uint32_t m_blocked = 0;
};

} // namespace stalker
