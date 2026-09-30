#include "MissionBlocker.h"
#include "../Gta/GtaTypes.h"
#include "../Gta/GtaFunctions.h"
#include "../Core/Log.h"

namespace stalker {

using namespace gta;

MissionBlocker& MissionBlocker::Get() {
    static MissionBlocker inst;
    return inst;
}

bool MissionBlocker::ShouldBlockScript(CRunningScript* script) {
    if (!script) return false;

    // Главный скрипт main.scm ("main") не трогаем — он держит мир
    // (погода, розыск, базовая инициализация), но миссий в нём нет:
    // миссии стартуют отдельными скриптами через LAUNCH_MISSION.
    if (script->thisMustBeOnlyMission && blockMissions) {
        // Корректно завершаем скрипт (убирает из списка активных)
        fn::RunningScriptShutdown(script);
        ++m_blocked;
        return true;
    }

    if (script->isExternal && blockExternal) {
        fn::RunningScriptShutdown(script);
        ++m_blocked;
        return true;
    }

    return false;
}

} // namespace stalker
