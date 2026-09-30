// ============================================================================
//  Тотальная конверсия «S.T.A.L.K.E.R.: Тень Чернобыля» для GTA San Andreas.
//  Точка входа ASI (stalker_mod.asi), инициализация хуков и систем.
//
//  Требования: GTA SA 1.0 US + любой ASI Loader (например, dinput8.dll
//  из gta-reversed/contrib/plugins.zip). Совместимость адресов — с
//  gta-reversed (https://github.com/gta-reversed/gta-reversed).
// ============================================================================
#include "Hooking/Int3Hooks.h"
#include "Gta/GtaTypes.h"
#include "Gta/GtaAddresses.h"
#include "Gta/GtaFunctions.h"
#include "Gta/ModelLoader.h"
#include "Core/Log.h"
#include "Core/Config.h"
#include "Core/ModState.h"
#include "Core/SaveSystem.h"
#include "Systems/MissionBlocker.h"
#include "Systems/MutantCatalog.h"
#include "Systems/MutantDirector.h"
#include "Systems/PopulationControl.h"
#include "Systems/RadiationSystem.h"
#include "Systems/AnomalySystem.h"
#include "Systems/InventorySystem.h"
#include "Systems/BlowoutSystem.h"
#include "Systems/CheatSystem.h"
#include "Systems/HudSystem.h"
#include "Systems/TradeSystem.h"
#include "Systems/AudioSystem.h"

#include <cstring>
#include <cstdio>

#if !defined(STALKER_SYNTAX_CHECK)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace stalker {

using namespace gta;

namespace {

// ---------------------------------------------------------------------------
// Состояние инициализации
// ---------------------------------------------------------------------------
bool g_bootInitDone = false;      // первый вызов Idle
bool g_sessionInitDone = false;   // сессия (новая игра / загрузка сейва)

// ---------------------------------------------------------------------------
// Пути и конфиги
// ---------------------------------------------------------------------------
void InitPaths() {
    ModContext& ctx = ModContext::Get();
#if !defined(STALKER_SYNTAX_CHECK)
    char exePath[512];
    GetModuleFileNameA(nullptr, exePath, sizeof(exePath));
    char* slash = strrchr(exePath, '\\');
    if (slash) *slash = 0;
    snprintf(ctx.gameDir, sizeof(ctx.gameDir), "%s", exePath);
    snprintf(ctx.modDir, sizeof(ctx.modDir), "%s\\STALKER_MOD", exePath);
#else
    snprintf(ctx.gameDir, sizeof(ctx.gameDir), ".");
    snprintf(ctx.modDir, sizeof(ctx.modDir), "STALKER_MOD");
#endif
}

void LoadConfigs() {
    ModContext& ctx = ModContext::Get();
    static Config cfgGeneral, cfgMutants, cfgZones, cfgItems, cfgFactions;
    char path[600];

    snprintf(path, sizeof(path), "%s\\stalker_mod.cfg", ctx.modDir);
    cfgGeneral.Load(path) ? Log::Write("Конфиг: %s", path)
                          : Log::Write("Конфиг %s не найден — по умолчанию", path);
    snprintf(path, sizeof(path), "%s\\mutants.cfg", ctx.modDir);
    cfgMutants.Load(path) ? Log::Write("Конфиг: %s", path)
                          : Log::Write("Конфиг %s не найден — встроенный каталог мутантов", path);
    snprintf(path, sizeof(path), "%s\\zones.cfg", ctx.modDir);
    cfgZones.Load(path) ? Log::Write("Конфиг: %s", path)
                        : Log::Write("Конфиг %s не найден — встроенные зоны", path);
    snprintf(path, sizeof(path), "%s\\items.cfg", ctx.modDir);
    cfgItems.Load(path) ? Log::Write("Конфиг: %s", path)
                        : Log::Write("Конфиг %s не найден — встроенные предметы", path);
    snprintf(path, sizeof(path), "%s\\factions.cfg", ctx.modDir);
    cfgFactions.Load(path) ? Log::Write("Конфиг: %s", path)
                           : Log::Write("Конфиг %s не найден — встроенные фракции", path);

    ctx.cfgGeneral  = &cfgGeneral;
    ctx.cfgMutants  = &cfgMutants;
    ctx.cfgZones    = &cfgZones;
    ctx.cfgItems    = &cfgItems;
    ctx.cfgFactions = &cfgFactions;

    ctx.player.maxWeight = cfgGeneral.GetFloat("inventory", "max_weight", 50.f);
}

void BootInit() {
    g_bootInitDone = true;
    InitPaths();
    ModContext& ctx = ModContext::Get();

    char logPath[600];
    snprintf(logPath, sizeof(logPath), "%s\\stalker_mod.log", ModContext::Get().modDir);
    Log::Open(logPath);
    Log::Write("=== S.T.A.L.K.E.R. mod for GTA SA — запуск ===");
    Log::Write("Каталог игры: %s", ModContext::Get().gameDir);

#if !defined(STALKER_SYNTAX_CHECK)
    // Предупреждение о конфликте с gta-reversed (оба хукают одни функции)
    if (GetModuleHandleA("gta_reversed.asi") != nullptr) {
        Log::Error("ОБНАРУЖЕН gta_reversed.asi! Хуки конфликтуют: загружайте "
                   "ТОЛЬКО stalker_mod.asi (см. README).");
    }
#endif

    LoadConfigs();
    MutantCatalog::Get().LoadDefaults();
    InventorySystem::Get().LoadItems();
    PopulationControl::Get().LoadFactions();

    // Настройки блокировки миссий
    MissionBlocker::Get().blockMissions =
        ctx.cfgGeneral ? ctx.cfgGeneral->GetBool("missions", "block_missions", true) : true;
    MissionBlocker::Get().blockExternal =
        ctx.cfgGeneral ? ctx.cfgGeneral->GetBool("missions", "block_external", true) : true;

    char savePath[600];
    snprintf(savePath, sizeof(savePath), "%s\\stalker_save.dat", ModContext::Get().modDir);
    SaveSystem::Get().SetPath(savePath);

    AudioSystem::Get().Start();
    Log::Write("BootInit: готово");
}

void SessionInit() {
    g_sessionInitDone = true;
    ModContext& ctx = ModContext::Get();

    ctx.player = PlayerState{};   // сброс
    ctx.sessionTime = 0.f;

    ModelLoader::Get().ResetForNewSession();
    AnomalySystem::Get().OnSessionStart();
    MutantDirector::Get().OnSessionStart();
    PopulationControl::Get().OnSessionStart();
    RadiationSystem::Get().LoadZones();
    BlowoutSystem::Get().OnSessionStart();

    if (!SaveSystem::Get().Load()) {
        InventorySystem::Get().GiveStarterKit();
        Log::Write("SessionInit: выдан стартовый набор новичка");
    }
    Log::Write("SessionInit: сессия активна");
}

// ---------------------------------------------------------------------------
// Обработчики хуков
// ---------------------------------------------------------------------------
void InstallHooks();

hooking::HookAction OnIdle(const hooking::HookContext&, void*) {
    if (!g_bootInitDone) {
        BootInit();
        InstallHooks(); // остальные хуки — из главного потока игры
    }

    void* playerPed = nullptr;
    if (PlayerInfo())
        playerPed = PlayerInfo()->playerPed;

    if (playerPed && !g_sessionInitDone && IsGameSessionActive())
        SessionInit();
    if (!playerPed && g_sessionInitDone) {
        g_sessionInitDone = false;
        MutantDirector::Get().OnSessionEnd();
        AnomalySystem::Get().OnSessionEnd();
        ModelLoader::Get().ResetForNewSession();
        Log::Write("Сессия завершена");
    }

    if (g_sessionInitDone && IsGameSessionActive() && !UserPause()) {
        float dt = DeltaTime();
        if (dt > 0.25f) dt = 0.25f;
        if (dt <= 0.f) dt = 0.0001f;
        ModContext::Get().sessionTime += dt;

        ModelLoader::Get().Tick();
        MutantDirector::Get().Tick(dt);
        PopulationControl::Get().Tick(dt);
        RadiationSystem::Get().Tick(dt);
        AnomalySystem::Get().Tick(dt);
        InventorySystem::Get().Tick(dt);
        BlowoutSystem::Get().Tick(dt);
        TradeSystem::Get().Tick(dt);
        HudSystem::Get().TickUI(dt);

        // Гейгер + тревога выброса
        AudioSystem& audio = AudioSystem::Get();
        audio.SetGeigerRate(RadiationSystem::Get().Intensity() * 6.f);
        if (BlowoutSystem::Get().GetPhase() == BlowoutSystem::PHASE_WARNING)
            audio.Alarm(2);

        SaveSystem::Get().TickAutosave(dt);
    }

    return hooking::HookAction::RunOriginal;
}

hooking::HookAction OnHudDraw(const hooking::HookContext&, void*) {
    if (g_sessionInitDone) {
        HudSystem::Get().Draw();
        AnomalySystem::Get().DrawMarkers();
    }
    return hooking::HookAction::RunOriginal;
}

hooking::HookAction OnCheatChar(const hooking::HookContext& ctx, void*) {
    // CCheat::AddToCheatString(char) — cdecl, символ первым аргументом
    char c = static_cast<char>(ctx.StackArg(0));
    CheatSystem::Get().OnChar(c);
    return hooking::HookAction::RunOriginal;
}

hooking::HookAction OnScriptProcess(const hooking::HookContext& ctx, void*) {
    // CRunningScript::Process() — thiscall: this в ECX
    CRunningScript* script = reinterpret_cast<CRunningScript*>(ctx.This());
    if (MissionBlocker::Get().ShouldBlockScript(script))
        return hooking::HookAction::Skip; // Eax = 0 (OR_CONTINUE)
    return hooking::HookAction::RunOriginal;
}

void InstallHooks() {
    hooking::Int3HookManager& hooks = hooking::Int3HookManager::Get();

    hooking::HookCallbacks cbIdle;
    cbIdle.onCall = &OnIdle;
    hooks.Install(addr::Idle, cbIdle, nullptr);

    hooking::HookCallbacks cbHud;
    cbHud.onCall = &OnHudDraw;
    hooks.Install(addr::HudDraw, cbHud, nullptr);

    hooking::HookCallbacks cbCheat;
    cbCheat.onCall = &OnCheatChar;
    hooks.Install(addr::CheatAddString, cbCheat, nullptr);

    hooking::HookCallbacks cbScript;
    cbScript.onCall = &OnScriptProcess;
    cbScript.skipReturnValue = 0; // OpcodeResult::OR_CONTINUE
    hooks.Install(addr::RunningScriptProcess, cbScript, nullptr);

    Log::Write("Хуки: Idle, CHud::Draw, AddToCheatString, Process — установлены");
}

} // namespace
} // namespace stalker

// ---------------------------------------------------------------------------
// Точка входа
// ---------------------------------------------------------------------------
#if !defined(STALKER_SYNTAX_CHECK)
extern "C" BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        // Лёгкая инициализация в DllMain: только хук Idle. Полная —
        // при первом вызове Idle (уже в контексте главного потока игры).
        hooking::HookCallbacks cb;
        cb.onCall = &stalker::OnIdle;
        hooking::Int3HookManager::Get().Install(gta::addr::Idle, cb, nullptr);
    }
    return TRUE;
}
#endif
