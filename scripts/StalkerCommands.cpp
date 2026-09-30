/*
 * ============================================================================
 *  ИНТЕГРАЦИЯ В GTA-REVERSED — скриптовые команды мода (CommandParser)
 * ============================================================================
 *
 *  Этот файл НЕ входит в самостоятельную сборку stalker_mod.asi.
 *  Он предназначен для сборки ВНУТРИ gta-reversed
 *  (https://github.com/gta-reversed/gta-reversed), которая поддерживает
 *  CLEO-команды: соберите gta-reversed с параметром
 *      GTASA_WITH_CLEO_COMMANDS=ON
 *  и положите этот файл в source/game_sa/Scripts/Commands/.
 *
 *  После этого CLEO-скрипты получают доступ к системам мода:
 *
 *    0AB0: "STK_SPAWN_MUTANT" model 108 pos 2400.0 -1600.0 12.0   ; мутант
 *    0AB0: "STK_GET_DOSE" result 0@                                ; доза
 *    0AB0: "STK_SET_DOSE" 250                                      ; задать дозу
 *    0AB0: "STK_BLOWOUT_NOW"                                       ; выброс
 *    0AB0: "STK_GIVE_ARTIFACT" 0@                                  ; артефакт
 *
 *  Реестр команд использует ту же систему CommandParser, что и собственные
 *  команды gta-reversed (см. source/game_sa/Scripts/CommandParser/Parser.hpp,
 *  макросы REGISTER_COMMAND_HANDLER / AddCommandHandler).
 *
 *  Системы мода при интеграции используют НАСТОЯЩИЕ типы gta-reversed
 *  (CPed, CPopulation, CRunningScript) вместо мини-SDK — то есть та же
 *  логика работает поверх полного реверса игры.
 * ============================================================================
 */
#include <StdInc.h>

#include "Commands.hpp"
#include <CommandParser/Parser.hpp>

#ifdef STALKER_WITH_REVERSED // включайте, когда собираете внутри gta-reversed

#include "StalkerMod/ModApi.h" // тонкая прослойка над системами мода

namespace notsa::script::commands {
using namespace plugin;

// Спавн мутанта (аналог CPopulation::AddPed + наши параметры баланса)
/*
 * @opcode 0AB0
 * @command STK_SPAWN_MUTANT
 * @class Stalker
 * @brief Спавнит мутанта по индексу каталога в точке
 */
auto StkSpawnMutant(CRunningScript& S, int32 mutantIndex, CVector& pos) {
    return stalker::ModApi::SpawnMutant(mutantIndex, pos);
}
REGISTER_COMMAND_HANDLER(COMMAND_STK_SPAWN_MUTANT, StkSpawnMutant);

/*
 * @opcode 0AB1
 * @command STK_GET_DOSE
 */
auto StkGetDose(CRunningScript& S) -> int32 {
    return (int32)stalker::ModApi::GetRadiationDose();
}
REGISTER_COMMAND_HANDLER(COMMAND_STK_GET_DOSE, StkGetDose);

/*
 * @opcode 0AB2
 * @command STK_SET_DOSE
 */
auto StkSetDose(CRunningScript& S, int32 dose) {
    stalker::ModApi::SetRadiationDose((float)dose);
}
REGISTER_COMMAND_HANDLER(COMMAND_STK_SET_DOSE, StkSetDose);

/*
 * @opcode 0AB3
 * @command STK_BLOWOUT_NOW
 */
auto StkBlowoutNow(CRunningScript& S) {
    stalker::ModApi::ForceBlowout();
}
REGISTER_COMMAND_HANDLER(COMMAND_STK_BLOWOUT_NOW, StkBlowoutNow);

/*
 * @opcode 0AB4
 * @command STK_GIVE_ARTIFACT
 */
auto StkGiveArtifact(CRunningScript& S) -> int32 {
    return stalker::ModApi::GiveRandomArtifact();
}
REGISTER_COMMAND_HANDLER(COMMAND_STK_GIVE_ARTIFACT, StkGiveArtifact);

} // namespace notsa::script::commands

#endif // STALKER_WITH_REVERSED
