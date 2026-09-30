#pragma once
// ============================================================================
//  Адреса GTA San Andreas 1.0 US (compact exe).
//
//  ВСЕ адреса взяты из gta-reversed (https://github.com/gta-reversed/gta-reversed)
//  и сверены по исходникам (RH_ScopedInstall / StaticRef аннотации):
//    * source/game_sa/ — реализация и адреса функций
//    * source/app/app_game.cpp — Idle
//  gta-reversed работает как ASI внутри оригинального gta_sa.exe 1.0 US,
//  поэтому эти адреса валидны и для чистой игры, и для сборки gta-reversed.
//
//  Проверка соответствия: tools/verify_addresses.py
// ============================================================================
#include <cstdint>

namespace gta::addr {

// ---------------------------------------------------------------------------
// Функции (в скобках — файл gta-reversed с аннотацией адреса)
// ---------------------------------------------------------------------------

// source/app/app_game.cpp: RH_ScopedGlobalInstall(Idle, 0x53E920)
constexpr uint32_t Idle = 0x53E920;                    // void __cdecl Idle(void*)

// source/game_sa/Hud.cpp: RH_ScopedInstall(Draw, 0x58FAE0)
constexpr uint32_t HudDraw = 0x58FAE0;                 // void __cdecl CHud::Draw()

// source/game_sa/Cheat.cpp: RH_ScopedInstall(AddToCheatString, 0x438480)
constexpr uint32_t CheatAddString = 0x438480;          // void __cdecl (char key)

// source/game_sa/Scripts/RunningScript.cpp
constexpr uint32_t RunningScriptProcess = 0x469F00;    // int8 __thiscall CRunningScript::Process()
constexpr uint32_t RunningScriptShutdown = 0x465AA0;   // void __thiscall CRunningScript::ShutdownThisScript()

// source/game_sa/World.cpp
constexpr uint32_t WorldAdd = 0x563220;                // void __cdecl CWorld::Add(CEntity*)
constexpr uint32_t WorldRemove = 0x563280;             // void __cdecl CWorld::Remove(CEntity*)
constexpr uint32_t WorldProcessVerticalLine = 0x5674E0;// bool __cdecl CWorld::ProcessVerticalLine(CVector&, float, CColPoint&, CEntity*&, bool buildings, bool vehicles, bool peds, bool objects, bool dummies, bool seeThrough, void*)
constexpr uint32_t WorldGetLineOfSightClear = 0x56A490;// bool __cdecl CWorld::GetIsLineOfSightClear(CVector&, CVector&, bool buildings, bool vehicles, bool peds, bool objects, bool dummies, bool seeThrough, bool camIgnore)

// source/game_sa/Population.cpp
constexpr uint32_t PopulationAddPed = 0x612710;        // CPed* __cdecl CPopulation::AddPed(ePedType, int32 modelId, CVector& pos, bool makeWander)
constexpr uint32_t PopulationRemovePed = 0x610F20;     // void __cdecl CPopulation::RemovePed(CPed*)

// source/game_sa/Entity/Ped/Ped.cpp
constexpr uint32_t PedSetCharCreatedBy = 0x5E47E0;     // void __thiscall CPed::SetCharCreatedBy(uint8)
constexpr uint32_t PedGiveDelayedWeapon = 0x5E89B0;    // void __thiscall CPed::GiveDelayedWeapon(eWeaponType, uint32)
constexpr uint32_t PedSetCurrentWeapon = 0x5E6280;     // void __thiscall CPed::SetCurrentWeapon(eWeaponType)
constexpr uint32_t PedSetPedStats = 0x5DEBC0;          // void __thiscall CPed::SetPedStats(ePedStats)
constexpr uint32_t PedSetMoveAnimSpeed = 0x5DEC10;     // void __thiscall CPed::SetMoveAnimSpeed(float)
constexpr uint32_t PedSetModelIndex = 0x5E4880;        // void __thiscall CPed::SetModelIndex(uint32)

// source/game_sa/Events/*
constexpr uint32_t EventGroupAdd = 0x4AB420;           // CEvent* __thiscall CEventGroup::Add(CEvent*, bool)
constexpr uint32_t EventScriptCommandCtor = 0x4B0A00;  // this __thiscall CEventScriptCommand(int32 taskIndex, CTask*, bool)
constexpr uint32_t EventAcqPedHateCtor = 0x420E70;     // this __thiscall CEventAcquaintancePedHate::Constructor2(CPed*)

// source/game_sa/Tasks/TaskTypes/*
constexpr uint32_t TaskKillPedOnFootCtor = 0x620E30;   // this __thiscall CTaskComplexKillPedOnFoot::Constructor(CPed*, int32 time=-1, int32 flags=0, int32 delay=0, int32 chance=0, uint8 competence=1)
constexpr uint32_t TaskWanderCtor = 0x66F450;          // this __thiscall CTaskComplexWander::Constructor(eMoveState, uint8 dir, bool wanderSensibly, float radius)
constexpr uint32_t TaskStandStillCtor = 0x62F310;      // this __thiscall CTaskSimpleStandStill::Constructor(int32 time, bool, bool, float)

// source/game_sa/Streaming.cpp
constexpr uint32_t StreamingRequestModel = 0x4087E0;   // void __cdecl CStreaming::RequestModel(int32 modelId, int32 flags)

// source/game_sa/TxdStore.cpp
constexpr uint32_t TxdStoreAddTxdSlot = 0x731C80;      // int32 __cdecl CTxdStore::AddTxdSlot(const char* name)
constexpr uint32_t TxdStoreRemoveTxdSlot = 0x731CD0;   // void __cdecl CTxdStore::RemoveTxdSlot(int32 slot)
constexpr uint32_t TxdStoreLoadTxd = 0x7320B0;         // bool __cdecl CTxdStore::LoadTxd(int32 slot, const char* filename)

// source/game_sa/FileLoader.cpp
constexpr uint32_t FileLoaderLoadClumpFile = 0x5372D0; // bool __cdecl CFileLoader::LoadClumpFile(RwStream*, uint32 modelId)

// source/game_sa/Models/*
constexpr uint32_t BaseModelInfoSetTexDictionary = 0x4C4B40; // void __thiscall CBaseModelInfo::SetTexDictionary(const char* txdName)
constexpr uint32_t PedModelInfoSetClump = 0x4C7340;    // void __thiscall CPedModelInfo::SetClump(RpClump*)
constexpr uint32_t PedModelInfoCtor = 0x4C57A0;        // this __thiscall CPedModelInfo::CPedModelInfo()

// source/game_sa/Camera.cpp
constexpr uint32_t CameraAddShakeSimple = 0x50D240;    // void __thiscall CCamera::AddShakeSimple(float duration, int32 type, float intensity)

// source/game_sa/Checkpoints.cpp
constexpr uint32_t CheckpointsPlaceMarker = 0x722C40;  // void* __cdecl CCheckpoints::PlaceMarker(uint32 id, uint16 type, CVector& pos, CVector& dir, float size, CRGBA color, uint16 pulsePeriod, float pulseFraction, int16 rotateRate)
constexpr uint32_t CheckpointsDeleteCP = 0x722FC0;     // void __cdecl CCheckpoints::DeleteCP(uint32 id, uint16 type)

// source/game_sa/Font.cpp
constexpr uint32_t FontSetScale = 0x719380;            // void __cdecl CFont::SetScale(float, float)
constexpr uint32_t FontSetColor = 0x719430;            // void __cdecl CFont::SetColor(CRGBA)
constexpr uint32_t FontSetFontStyle = 0x719490;        // void __cdecl CFont::SetFontStyle(uint8)
constexpr uint32_t FontSetWrapx = 0x7194D0;            // void __cdecl CFont::SetWrapx(float)
constexpr uint32_t FontSetDropColor = 0x719510;        // void __cdecl CFont::SetDropColor(CRGBA)
constexpr uint32_t FontSetEdge = 0x719590;             // void __cdecl CFont::SetEdge(int8)
constexpr uint32_t FontSetProportional = 0x7195B0;     // void __cdecl CFont::SetProportional(bool)
constexpr uint32_t FontSetJustify = 0x719600;          // void __cdecl CFont::SetJustify(bool)
constexpr uint32_t FontSetOrientation = 0x719610;      // void __cdecl CFont::SetOrientation(uint8)
constexpr uint32_t FontInitPerFrame = 0x719800;        // void __cdecl CFont::InitPerFrame()
constexpr uint32_t FontRenderFontBuffer = 0x719840;    // void __cdecl CFont::RenderFontBuffer()
constexpr uint32_t FontPrintString = 0x71A700;         // void __cdecl CFont::PrintString(float, float, const char*)

// source/game_sa/Sprite2d.cpp
constexpr uint32_t SpriteDraw2DPolygon = 0x7285B0;     // void __cdecl CSprite2d::Draw2DPolygon(float x1, float y1, x2, y2, x3, y3, x4, y4, CRGBA)

// RenderWare (plugin-sdk RenderWare.cpp; функции RW внутри gta_sa.exe)
constexpr uint32_t RwStreamOpen = 0x7ECEF0;            // void* __cdecl RwStreamOpen(int type, int access, const void* data)
constexpr uint32_t RwStreamClose = 0x7ECE20;           // int32 __cdecl RwStreamClose(void* stream, void* data)

// source/game_sa/Entity/Placeable.cpp
constexpr uint32_t PlaceableSetHeading = 0x43E0C0;     // void __thiscall CPlaceable::SetHeading(float)

// ---------------------------------------------------------------------------
// Глобальные переменные
// ---------------------------------------------------------------------------

// source/game_sa/Timer.h
constexpr uint32_t TimerTimeStep = 0xB7CB5C;           // float  CTimer::ms_fTimeStep
constexpr uint32_t TimerTimeScale = 0xB7CB64;          // float  CTimer::ms_fTimeScale
constexpr uint32_t TimerTimeInMS = 0xB7CB84;           // uint32 CTimer::m_snTimeInMilliseconds
constexpr uint32_t TimerUserPause = 0xB7CB49;          // bool   CTimer::m_UserPause

// source/game_sa/Pad.h
constexpr uint32_t PadNewKeyState = 0xB73190;          // CKeyboardState
constexpr uint32_t PadOldKeyState = 0xB72F20;          // CKeyboardState

// source/game_sa/World.h
constexpr uint32_t WorldPlayers = 0xB7CD98;            // CPlayerInfo[2]
constexpr uint32_t WorldPlayerInFocus = 0xB7CD74;      // uint8

// source/game_sa/Pools.cpp
constexpr uint32_t PoolsPedPool = 0xB74490;            // CPoolRaw* (CPool<CPed,CCopPed>*)

// source/game_sa/Models/ModelInfo.h
constexpr uint32_t ModelInfoPtrs = 0xA9B0C8;           // CBaseModelInfo*[20000]

// source/game_sa/Streaming.h
constexpr uint32_t StreamingInfoArray = 0x8E4CC0;      // CStreamingInfo[26316]

// source/game_sa/Cheat.h
constexpr uint32_t CheatString = 0x969110;             // char[32] CCheat::m_CheatString

// source/game_sa/TimeCycle.h
constexpr uint32_t TimeCycleCurrentColours = 0xB7C4A0; // CColourSet CTimeCycle::m_CurrentColours

// source/game_sa/Weather.h
constexpr uint32_t WeatherForcedType = 0xC81318;       // int16 CWeather::ForcedWeatherType
constexpr uint32_t WeatherNewType = 0xC8131C;          // int16 CWeather::NewWeatherType
constexpr uint32_t WeatherOldType = 0xC81320;          // int16 CWeather::OldWeatherType

// source/game_sa/Game.h
constexpr uint32_t GameCurrArea = 0xB72914;            // int32 CGame::currArea

// source/game_sa/Camera.cpp
constexpr uint32_t TheCamera = 0xB6F028;               // CCamera (0xD78)

// source/game_sa/Population.h
constexpr uint32_t PopPedDensityMultiplier = 0x8D2530; // float CPopulation::PedDensityMultiplier
constexpr uint32_t PopDontCreateRandomCops = 0xC0FCB4; // bool
constexpr uint32_t PopDontCreateRandomGangs = 0xC0FCB2;// bool CPopulation::m_bDontCreateRandomGangMembers

// RwCamera для проверки состояния инициализации (как в dllmain gta-reversed)
constexpr uint32_t RwCameraInstance = 0xC1703C;        // RwCamera* (не null после старта RenderWare)

// RsGlobal (plugin-sdk: RenderWare.cpp; rw/skeleton.h: RsGlobalType)
constexpr uint32_t RsGlobal = 0xC17040;                // +0x4 maximumWidth, +0x8 maximumHeight

// Память приложения
constexpr uint32_t ImageBase = 0x400000;

} // namespace gta::addr
