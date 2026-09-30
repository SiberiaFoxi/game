#pragma once
// ============================================================================
//  Обёртки для вызова функций игры по абсолютным адресам (GTA SA 1.0 US).
//
//  thiscall-функции вызываются через __fastcall с фиктивным вторым
//  регистровым аргументом (классическая техника plugin-sdk / MTA для x86):
//    Ret __thiscall F(This* this, A a)  ->  Ret __fastcall F(This* this, int, A a)
//
//  На платформе проверки синтаксиса (STALKER_SYNTAX_CHECK) обёртки превращаются
//  в пустышки — проверяется только компилируемость.
// ============================================================================
#include "GtaTypes.h"
#include "GtaAddresses.h"

namespace gta {

#if defined(STALKER_SYNTAX_CHECK)
// ---- Заглушки для синтакс-проверки на Linux ----
namespace detail {
template <typename Ret, typename... Args>
inline Ret CallCdecl(uint32_t, Args...) { return Ret(); }
template <typename Ret, typename This, typename... Args>
inline Ret CallThiscall(uint32_t, This, Args...) { return Ret(); }
}
#else
namespace detail {
template <typename Ret, typename... Args>
inline Ret CallCdecl(uint32_t address, Args... args) {
    return reinterpret_cast<Ret(__cdecl*)(Args...)>(address)(args...);
}
template <typename Ret, typename This, typename... Args>
inline Ret CallThiscall(uint32_t address, This self, Args... args) {
    // фиктивный int занимает edx — эмуляция thiscall через fastcall
    return reinterpret_cast<Ret(__fastcall*)(This, int, Args...)>(address)(self, 0, args...);
}
}
#endif

// ---------------------------------------------------------------------------
// Глобальные данные
// ---------------------------------------------------------------------------
template <typename T>
inline T& Global(uint32_t address) { return *reinterpret_cast<T*>(address); }

inline float&    TimeStep()   { return Global<float>(addr::TimerTimeStep); }
inline uint32_t  TimeInMS()   { return Global<uint32_t>(addr::TimerTimeInMS); }
inline bool&     UserPause()  { return Global<bool>(addr::TimerUserPause); }
inline float     DeltaTime()  { return TimeStep() * 0.02f; } // сек (50 шагов/с)

inline CKeyboardState& NewKeyState() { return Global<CKeyboardState>(addr::PadNewKeyState); }
inline CKeyboardState& OldKeyState() { return Global<CKeyboardState>(addr::PadOldKeyState); }

inline CPlayerInfo* PlayerInfo(int idx = -1) {
    uint8_t inFocus = Global<uint8_t>(addr::WorldPlayerInFocus);
    if (idx < 0) idx = inFocus;
    return &reinterpret_cast<CPlayerInfo*>(addr::WorldPlayers)[idx];
}
inline CPed* PlayerPed() { return static_cast<CPed*>(PlayerInfo()->playerPed); }

inline CPoolRaw* PedPool() { return *reinterpret_cast<CPoolRaw**>(addr::PoolsPedPool); }
inline CBaseModelInfo** AllModelInfos() { return reinterpret_cast<CBaseModelInfo**>(addr::ModelInfoPtrs); }
inline CStreamingInfo* StreamingInfo(int32_t modelId) {
    return reinterpret_cast<CStreamingInfo*>(addr::StreamingInfoArray) + modelId;
}
inline bool IsModelLoaded(int32_t modelId) {
    if (modelId < 0 || modelId >= 26316) return false;
    return StreamingInfo(modelId)->loadState == LOADSTATE_LOADED;
}
inline CColourSet& CurrentColours() { return Global<CColourSet>(addr::TimeCycleCurrentColours); }
inline int32_t&    CurrArea()       { return Global<int32_t>(addr::GameCurrArea); }
inline float&      PedDensityMultiplier() { return Global<float>(addr::PopPedDensityMultiplier); }
inline bool&       DontCreateRandomCops() { return Global<bool>(addr::PopDontCreateRandomCops); }
inline bool&       DontCreateRandomGangs() { return Global<bool>(addr::PopDontCreateRandomGangs); }
inline int16_t&    WeatherNewType() { return Global<int16_t>(addr::WeatherNewType); }
inline int16_t&    WeatherOldType() { return Global<int16_t>(addr::WeatherOldType); }

// Игра инициализирована и сессия запущена (есть пед игрока с моделью)
inline bool IsGameSessionActive() {
    CPed* ped = PlayerPed();
    return ped != nullptr && ped->rwObject != nullptr;
}

// ---------------------------------------------------------------------------
// Функции
// ---------------------------------------------------------------------------
namespace fn {

// CWorld (cdecl)
inline void WorldAdd(CEntity* e)    { detail::CallCdecl<void>(gta::addr::WorldAdd, e); }
inline void WorldRemove(CEntity* e) { detail::CallCdecl<void>(gta::addr::WorldRemove, e); }

inline bool WorldProcessVerticalLine(const CVector& origin, float distance, CColPoint& outCol,
                                     void*& outEntity, bool buildings, bool vehicles, bool peds,
                                     bool objects, bool dummies, bool seeThrough) {
    return detail::CallCdecl<bool>(gta::addr::WorldProcessVerticalLine,
        const_cast<CVector*>(&origin), distance, outCol, outEntity,
        buildings, vehicles, peds, objects, dummies, seeThrough, (void*)nullptr);
}

inline bool WorldGetLineOfSightClear(const CVector& from, const CVector& to,
                                     bool buildings, bool vehicles, bool peds, bool objects,
                                     bool dummies, bool seeThrough, bool camIgnore) {
    return detail::CallCdecl<bool>(gta::addr::WorldGetLineOfSightClear,
        const_cast<CVector*>(&from), const_cast<CVector*>(&to),
        buildings, vehicles, peds, objects, dummies, seeThrough, camIgnore);
}

// CRunningScript (thiscall)
inline void RunningScriptShutdown(gta::CRunningScript* script) {
    detail::CallThiscall<void>(gta::addr::RunningScriptShutdown, script);
}

// CPopulation (cdecl)
inline CPed* PopulationAddPed(int32_t pedType, int32_t modelId, const CVector& pos, bool makeWander) {
    return detail::CallCdecl<CPed*>(gta::addr::PopulationAddPed, pedType, modelId,
                                    const_cast<CVector*>(&pos), makeWander);
}
inline void PopulationRemovePed(CPed* ped) {
    detail::CallCdecl<void>(gta::addr::PopulationRemovePed, ped);
}

// CPed (thiscall)
inline void PedSetCharCreatedBy(CPed* ped, uint8_t createdBy) {
    detail::CallThiscall<void>(gta::addr::PedSetCharCreatedBy, ped, createdBy);
}
inline void PedGiveDelayedWeapon(CPed* ped, int32_t weaponType, uint32_t ammo) {
    detail::CallThiscall<void>(gta::addr::PedGiveDelayedWeapon, ped, weaponType, ammo);
}
inline void PedSetCurrentWeapon(CPed* ped, int32_t weaponType) {
    detail::CallThiscall<void>(gta::addr::PedSetCurrentWeapon, ped, weaponType);
}
inline void PedSetMoveAnimSpeed(CPed* ped, float speed) {
    detail::CallThiscall<void>(gta::addr::PedSetMoveAnimSpeed, ped, speed);
}
inline void PedSetModelIndex(CPed* ped, uint32_t modelIndex) {
    detail::CallThiscall<void>(gta::addr::PedSetModelIndex, ped, modelIndex);
}

// События и задачи (thiscall; буферы предоставляет вызывающий —
// CEventGroup::Add клонирует событие, поэтому стек-буферы безопасны)
inline void* EventGroupAdd(void* eventGroup, void* event, bool valid) {
    return detail::CallThiscall<void*>(gta::addr::EventGroupAdd, eventGroup, event, valid);
}
// CEventScriptCommand = 0x18 байт
inline void* ConstructEventScriptCommand(void* buffer, int32_t taskIndex, void* task, bool affectsDeadPeds) {
    return detail::CallThiscall<void*>(gta::addr::EventScriptCommandCtor, buffer, taskIndex, task, affectsDeadPeds);
}
// CEventAcquaintancePedHate (буфер 0x20 с запасом)
inline void* ConstructEventAcquaintancePedHate(void* buffer, CPed* target) {
    return detail::CallThiscall<void*>(gta::addr::EventAcqPedHateCtor, buffer, target);
}
// CTaskComplexKillPedOnFoot = 0x38 байт
inline void* ConstructTaskKillPedOnFoot(void* buffer, CPed* target, int32_t time, int32_t pedFlags,
                                        int32_t delay, int32_t chance, uint8_t competence) {
    return detail::CallThiscall<void*>(gta::addr::TaskKillPedOnFootCtor, buffer, target, time, pedFlags, delay, chance, competence);
}
// CTaskComplexWander = 0x28 байт
inline void* ConstructTaskWander(void* buffer, int32_t moveState, uint8_t dir, bool wanderSensibly, float radius) {
    return detail::CallThiscall<void*>(gta::addr::TaskWanderCtor, buffer, moveState, dir, wanderSensibly, radius);
}

// CStreaming (cdecl)
inline void StreamingRequestModel(int32_t modelId, int32_t flags) {
    detail::CallCdecl<void>(gta::addr::StreamingRequestModel, modelId, flags);
}

// CTxdStore (cdecl)
inline int32_t TxdStoreAddTxdSlot(const char* name) {
    return detail::CallCdecl<int32_t>(gta::addr::TxdStoreAddTxdSlot, name);
}
inline void TxdStoreRemoveTxdSlot(int32_t slot) {
    detail::CallCdecl<void>(gta::addr::TxdStoreRemoveTxdSlot, slot);
}
inline bool TxdStoreLoadTxd(int32_t slot, const char* filename) {
    return detail::CallCdecl<bool>(gta::addr::TxdStoreLoadTxd, slot, filename);
}

// RenderWare (cdecl)
inline void* RwStreamOpen(int type, int access, const void* data) {
    return detail::CallCdecl<void*>(gta::addr::RwStreamOpen, type, access, data);
}
inline int32_t RwStreamClose(void* stream, void* data) {
    return detail::CallCdecl<int32_t>(gta::addr::RwStreamClose, stream, data);
}

// CFileLoader (cdecl)
inline bool FileLoaderLoadClumpFile(void* stream, uint32_t modelId) {
    return detail::CallCdecl<bool>(gta::addr::FileLoaderLoadClumpFile, stream, modelId);
}

// Модели (thiscall)
inline void BaseModelInfoSetTexDictionary(CBaseModelInfo* mi, const char* txdName) {
    detail::CallThiscall<void>(gta::addr::BaseModelInfoSetTexDictionary, mi, txdName);
}
inline void PedModelInfoSetClump(CBaseModelInfo* mi, void* clump) {
    detail::CallThiscall<void>(gta::addr::PedModelInfoSetClump, mi, clump);
}
inline void* PedModelInfoConstruct(CBaseModelInfo* mi) {
    return detail::CallThiscall<void*>(gta::addr::PedModelInfoCtor, mi);
}

// Камера (thiscall на TheCamera)
inline void CameraShake(float duration, int32_t type, float intensity) {
    detail::CallThiscall<void>(gta::addr::CameraAddShakeSimple,
                               reinterpret_cast<void*>(gta::addr::TheCamera), duration, type, intensity);
}

// Чекпоинты (cdecl)
inline void CheckpointsPlaceMarker(uint32_t id, uint16_t type, const CVector& pos, const CVector& dir,
                                   float size, const CRGBA& color, uint16_t pulsePeriod,
                                   float pulseFraction, int16_t rotateRate) {
    detail::CallCdecl<void>(gta::addr::CheckpointsPlaceMarker, id, type,
                            const_cast<CVector*>(&pos), const_cast<CVector*>(&dir),
                            size, color, pulsePeriod, pulseFraction, rotateRate);
}
inline void CheckpointsDeleteCP(uint32_t id, uint16_t type) {
    detail::CallCdecl<void>(gta::addr::CheckpointsDeleteCP, id, type);
}

// CFont (cdecl)
inline void FontInitPerFrame() { detail::CallCdecl<void>(gta::addr::FontInitPerFrame); }
inline void FontRenderFontBuffer() { detail::CallCdecl<void>(gta::addr::FontRenderFontBuffer); }
inline void FontSetScale(float w, float h) { detail::CallCdecl<void>(gta::addr::FontSetScale, w, h); }
inline void FontSetColor(const CRGBA& c) { detail::CallCdecl<void>(gta::addr::FontSetColor, c); }
inline void FontSetFontStyle(uint8_t style) { detail::CallCdecl<void>(gta::addr::FontSetFontStyle, style); }
inline void FontSetWrapx(float x) { detail::CallCdecl<void>(gta::addr::FontSetWrapx, x); }
inline void FontSetDropColor(const CRGBA& c) { detail::CallCdecl<void>(gta::addr::FontSetDropColor, c); }
inline void FontSetEdge(int8_t e) { detail::CallCdecl<void>(gta::addr::FontSetEdge, e); }
inline void FontSetProportional(bool on) { detail::CallCdecl<void>(gta::addr::FontSetProportional, on); }
inline void FontSetJustify(bool on) { detail::CallCdecl<void>(gta::addr::FontSetJustify, on); }
inline void FontSetOrientation(uint8_t o) { detail::CallCdecl<void>(gta::addr::FontSetOrientation, o); }
inline void FontPrintString(float x, float y, const char* text) {
    detail::CallCdecl<void>(gta::addr::FontPrintString, x, y, text);
}

// CSprite2d (cdecl)
inline void SpriteDraw2DPolygon(float x1, float y1, float x2, float y2,
                                float x3, float y3, float x4, float y4, const CRGBA& c) {
    detail::CallCdecl<void>(gta::addr::SpriteDraw2DPolygon, x1, y1, x2, y2, x3, y3, x4, y4, c);
}

// CPlaceable (thiscall)
inline void EntitySetHeading(CEntity* e, float heading) {
    detail::CallThiscall<void>(gta::addr::PlaceableSetHeading, e, heading);
}

} // namespace fn

// ---------------------------------------------------------------------------
// Вспомогательные операции над педом игрока
// ---------------------------------------------------------------------------
inline void DamagePlayerPed(float amount) {
    CPed* ped = PlayerPed();
    if (!ped) return;
    float& hp = ped->Health();
    hp -= amount;
    if (hp < 0.f) hp = 0.f;
}

inline void HealPlayerPed(float amount) {
    CPed* ped = PlayerPed();
    if (!ped) return;
    float& hp = ped->Health();
    hp += amount;
    float maxHp = ped->MaxHealth();
    if (hp > maxHp) hp = (maxHp > 0.f ? maxHp : 100.f);
}

// Валидность указателя на пед (в пуле, слот занят)
inline bool IsValidPedPointer(CPed* ped) {
    CPoolRaw* pool = PedPool();
    if (!pool || !pool->objects || !pool->flags) return false;
    uintptr_t base = reinterpret_cast<uintptr_t>(pool->objects);
    uintptr_t p    = reinterpret_cast<uintptr_t>(ped);
    if (p < base) return false;
    uint32_t idx = static_cast<uint32_t>((p - base) / kPedPoolStride);
    if (idx >= pool->capacity) return false;
    return pool->IsSlotUsed(idx);
}

// Итерация по всем педам в пуле
template <typename F>
inline void ForEachPed(F&& callback) {
    CPoolRaw* pool = PedPool();
    if (!pool || !pool->objects || !pool->flags) return;
    for (uint32_t i = 0; i < pool->capacity; ++i) {
        if (!pool->IsSlotUsed(i)) continue;
        CPed* ped = reinterpret_cast<CPed*>(reinterpret_cast<uintptr_t>(pool->objects) + uintptr_t(i) * kPedPoolStride);
        if (!callback(ped)) break; // callback вернул false — прервать
    }
}

} // namespace gta
