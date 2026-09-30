#pragma once
// ============================================================================
//  S.T.A.L.K.E.R.: Тень Чернобыля — тотальная конверсия для GTA San Andreas
//  Мини-SDK: базовые типы игры.
//
//  Все структуры сверены с gta-reversed
//  (https://github.com/gta-reversed/gta-reversed, source/game_sa/*).
//  Смещения полей выведены из объявлений классов gta-reversed
//  (VALIDATE_SIZE / field_XXX маркеры) и совпадают с plugin-sdk / MTA.
// ============================================================================
#include <cstdint>
#include <cstddef>
#include <cmath>

namespace gta {

#pragma pack(push, 4)

struct CVector2D {
    float x = 0.f, y = 0.f;
    CVector2D() = default;
    CVector2D(float X, float Y) : x(X), y(Y) {}
};

struct CVector {
    float x = 0.f, y = 0.f, z = 0.f;
    CVector() = default;
    CVector(float X, float Y, float Z) : x(X), y(Y), z(Z) {}
    float Magnitude() const { return std::sqrt(x * x + y * y + z * z); }
    float Magnitude2D() const { return std::sqrt(x * x + y * y); }
    CVector operator-(const CVector& o) const { return { x - o.x, y - o.y, z - o.z }; }
    CVector operator+(const CVector& o) const { return { x + o.x, y + o.y, z + o.z }; }
    CVector operator*(float s) const { return { x * s, y * s, z * s }; }
};

struct CRGBA {
    uint8_t r = 255, g = 255, b = 255, a = 255;
    CRGBA() = default;
    CRGBA(uint8_t R, uint8_t G, uint8_t B, uint8_t A = 255) : r(R), g(G), b(B), a(A) {}
};

struct CRect {
    float left = 0.f, top = 0.f, right = 0.f, bottom = 0.f;
    CRect() = default;
    CRect(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}
};

#pragma pack(pop)

// ----------------------------------------------------------------------------
//  Клавиатурное состояние CPad (gta-reversed: source/game_sa/KeyboardState.h)
//  CPad::NewKeyState = 0xB73190, OldKeyState = 0xB72F20, размер 0x270.
// ----------------------------------------------------------------------------
struct CKeyboardState {
    int16_t FKeys[12];        // +0x000  F1..F12
    int16_t standardKeys[256];// +0x018  индекс = виртуальный код клавиши
    int16_t esc;              // +0x218
    int16_t insert;           // +0x21A
    int16_t del;              // +0x21C
    int16_t home;             // +0x21E
    int16_t end;              // +0x220
    int16_t pgup;             // +0x222
    int16_t pgdn;             // +0x224
    int16_t up;               // +0x226
    int16_t down;             // +0x228
    int16_t left;             // +0x22A
    int16_t right;            // +0x22C
    int16_t scroll;           // +0x22E
    int16_t pause;            // +0x230
    int16_t numlock;          // +0x232
    int16_t div;              // +0x234
    int16_t mul;              // +0x236
    int16_t sub;              // +0x238
    int16_t add;              // +0x23A
    int16_t enter;            // +0x23C
    int16_t decimal;          // +0x23E
    int16_t num1;             // +0x240
    int16_t num2;             // +0x242
    int16_t num3;             // +0x244
    int16_t num4;             // +0x246
    int16_t num5;             // +0x248
    int16_t num6;             // +0x24A
    int16_t num7;             // +0x24C
    int16_t num8;             // +0x24E
    int16_t num9;             // +0x250
    int16_t num0;             // +0x252
    int16_t back;             // +0x254
    int16_t tab;              // +0x256
    int16_t capslock;         // +0x258
    int16_t extenter;         // +0x25A
    int16_t lshift;           // +0x25C
    int16_t rshift;           // +0x25E
    int16_t shift;            // +0x260
    int16_t lctrl;            // +0x262
    int16_t rctrl;            // +0x264
    int16_t lalt;             // +0x266
    int16_t ralt;             // +0x268
    int16_t lwin;             // +0x26A
    int16_t rwin;             // +0x26C
    int16_t apps;             // +0x26E
};
static_assert(sizeof(CKeyboardState) == 0x270, "CKeyboardState size (0x270)");

// ----------------------------------------------------------------------------
//  CPool (gta-reversed: source/game_sa/Core/Pool.h) — оригинальная раскладка:
//    +0x00 массив объектов (слоты размером sizeof(самый большой класс))
//    +0x04 массив байтов состояния (бит7 = слот свободен, биты0-6 = ref)
//    +0x08 ёмкость (uint32)
//  Пул педов: CPool<CPed, CCopPed>, шаг слота = 0x7C4 (sizeof(CCopPed)).
// ----------------------------------------------------------------------------
struct CPoolRaw {
    void*    objects;
    uint8_t* flags;
    uint32_t capacity;

    bool IsSlotUsed(uint32_t i) const { return (flags[i] & 0x80) == 0; }
};

// ----------------------------------------------------------------------------
//  CEntity / CPlaceable / CPhysical / CPed — только используемые поля.
//  (gta-reversed: source/game_sa/Entity/{Placeable,Entity,Physical}.h, Ped.h)
// ----------------------------------------------------------------------------
constexpr uint32_t kPedPoolStride = 0x7C4; // sizeof(CCopPed)

// CEntity (VALIDATE_SIZE(CEntity, 0x38))
struct CEntity {
    void*     vtbl;           // +0x00
    CVector   placePos;       // +0x04  CSimpleTransform::m_vPosn
    float     placeHeading;   // +0x10
    void*     matrix;         // +0x14  CMatrixLink* (если есть — позиция в нём)
    void*     rwObject;       // +0x18
    uint32_t  flags;          // +0x1C  (бит 13 = bRemoveFromWorld)
    uint16_t  randomSeed;     // +0x20
    uint16_t  modelIndex;     // +0x22
    void*     references;     // +0x24

    uint8_t RawByte(uint32_t off) const {
        return *reinterpret_cast<const uint8_t*>(reinterpret_cast<uintptr_t>(this) + off);
    }

    // CEntityInfo (m_info, +0x36): тип в битах 0-2, статус в битах 3-7
    uint8_t  Type()     const { return RawByte(0x36) & 0x7; }
    uint8_t  Status()   const { return RawByte(0x36) >> 3; }
    uint16_t ModelId()  const { return modelIndex; }
    bool     FlagBit(uint32_t bit) const { return (flags & (1u << bit)) != 0; }
    bool     RemoveFromWorld() const { return FlagBit(13); }

    CVector& GetPosition() {
        // gta-reversed: CPlaceable::GetPosition() — матрица имеет приоритет
        if (matrix) {
            // CMatrixLink: +0x0 right, +0x10 up, +0x20 at, +0x30 pos
            return *reinterpret_cast<CVector*>(reinterpret_cast<uintptr_t>(matrix) + 0x30);
        }
        return placePos;
    }
    const CVector& GetPosition() const {
        return const_cast<CEntity*>(this)->GetPosition();
    }
};
#if !defined(STALKER_SYNTAX_CHECK)
static_assert(offsetof(CEntity, rwObject) == 0x18, "CEntity layout");

#endif
// Статусы сущности (gta-reversed: Enums/eEntityStatus.h)
enum eEntityStatus : uint8_t {
    STATUS_PLAYER = 0,
    STATUS_PLAYER_PLAYBACKFROMBUFFER,
    STATUS_SIMPLE,
    STATUS_PHYSICS,
    STATUS_PLAYER_DRIVER,
    STATUS_ABANDONED,
    STATUS_WRECKED,
    STATUS_TRAIN_MOVING,
    STATUS_TRAIN_NOT_MOVING,
    STATUS_SIMPLE_TRAFFIC_LIGHTS_REVERSED,
    STATUS_PHYSICS_TRAFFIC_LIGHTS_REVERSED,
    STATUS_HAS_DIED,
};

// ePedState (gta-reversed: Enums/ePedState.h) — используемые значения
enum ePedState : int32_t {
    PEDSTATE_IDLE = 0,
    PEDSTATE_DIE = 49,
    PEDSTATE_DEAD = 50,
};

// CPed (VALIDATE_SIZE(CPed, 0x79C)) — только нужные поля
struct CPed : public CEntity {
    // ---- всё между 0x38 и 0x47C — аудио-сущности и пр. (opaque) ----
    static constexpr uint32_t OFF_INTELLIGENCE = 0x47C; // CPedIntelligence*
    static constexpr uint32_t OFF_PLAYERDATA   = 0x480; // CPlayerPedData*
    static constexpr uint32_t OFF_CREATEDBY    = 0x484; // ePedCreatedBy
    static constexpr uint32_t OFF_ANIMGROUP    = 0x4D4; // AssocGroupId
    static constexpr uint32_t OFF_ACQUAINTANCE = 0x4E0; // CAcquaintance (0x14)
    static constexpr uint32_t OFF_PEDSTATE     = 0x530; // ePedState
    static constexpr uint32_t OFF_MOVESTATE    = 0x534; // eMoveState
    static constexpr uint32_t OFF_HEALTH       = 0x540; // float
    static constexpr uint32_t OFF_MAXHEALTH    = 0x544; // float
    static constexpr uint32_t OFF_ARMOUR       = 0x548; // float
    static constexpr uint32_t OFF_CURROTATION  = 0x558; // float
    static constexpr uint32_t OFF_AIMROTATION  = 0x55C; // float
    static constexpr uint32_t OFF_PEDTYPE      = 0x598; // ePedType
    static constexpr uint32_t OFF_STATS        = 0x59C; // CPedStat*
    static constexpr uint32_t OFF_MOVESPEED    = 0x44;  // CPhysical::m_vecMoveSpeed

    void*    PedIntelligence() const { return *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(this) + OFF_INTELLIGENCE); }
    int32_t  PedType()         const { return *reinterpret_cast<const int32_t*>(reinterpret_cast<uintptr_t>(this) + OFF_PEDTYPE); }
    int32_t  PedState()        const { return *reinterpret_cast<const int32_t*>(reinterpret_cast<uintptr_t>(this) + OFF_PEDSTATE); }
    float&   Health()          { return *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + OFF_HEALTH); }
    float    Health()    const { return *reinterpret_cast<const float*>(reinterpret_cast<uintptr_t>(this) + OFF_HEALTH); }
    float&   MaxHealth()       { return *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + OFF_MAXHEALTH); }
    float    MaxHealth() const { return *reinterpret_cast<const float*>(reinterpret_cast<uintptr_t>(this) + OFF_MAXHEALTH); }
    float&   Armour()          { return *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + OFF_ARMOUR); }
    float    Armour()    const { return *reinterpret_cast<const float*>(reinterpret_cast<uintptr_t>(this) + OFF_ARMOUR); }
    CVector& MoveSpeed()       { return *reinterpret_cast<CVector*>(reinterpret_cast<uintptr_t>(this) + OFF_MOVESPEED); }
    // CAcquaintance: 5 x uint32 (like, respect, ignore, dislike, hate)
    uint32_t& HateMask()       { return *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(this) + OFF_ACQUAINTANCE + 16); }
    // CPedIntelligence: m_pPed(0x0), m_TaskMgr(0x4), m_eventHandler(0x34), m_eventGroup(0x68)
    void* EventGroup() const {
        void* intel = PedIntelligence();
        return intel ? reinterpret_cast<char*>(intel) + 0x68 : nullptr;
    }
    bool IsAlive() const {
        return PedState() != PEDSTATE_DIE && PedState() != PEDSTATE_DEAD && Health() > 0.f;
    }
};

// ePedType (gta-reversed: Enums/ePedType.h)
enum ePedType : int32_t {
    PED_TYPE_PLAYER1 = 0,
    PED_TYPE_CIVMALE = 4,
    PED_TYPE_CIVFEMALE = 5,
    PED_TYPE_COP = 6,
    PED_TYPE_GANG1 = 7,
    PED_TYPE_DEALER = 17,
    PED_TYPE_CRIMINAL = 20,
    PED_TYPE_SPECIAL = 23,
};

// ePedCreatedBy
enum ePedCreatedBy : int32_t {
    PED_UNKNOWN = 0,
    PED_GAME = 1,
    PED_MISSION = 2,
};

// eWeaponType (gta-reversed: Enums/eWeaponType.h)
enum eWeaponType : int32_t {
    WEAPON_UNARMED = 0,
    WEAPON_KNIFE = 4,
    WEAPON_BASEBALLBAT = 5,
    WEAPON_KATANA = 8,
    WEAPON_PISTOL = 22,
    WEAPON_PISTOL_SILENCED = 23,
    WEAPON_DESERT_EAGLE = 24,
    WEAPON_SHOTGUN = 25,
    WEAPON_SAWNOFF_SHOTGUN = 26,
    WEAPON_MICRO_UZI = 28,
    WEAPON_MP5 = 29,
    WEAPON_AK47 = 30,
    WEAPON_M4 = 31,
};

// AssocGroupId (gta-reversed: Enums/AnimationEnums.h)
constexpr int32_t ANIM_GROUP_MAN = 118;

// ----------------------------------------------------------------------------
//  CPlayerInfo (gta-reversed: source/game_sa/PlayerInfo.h, VALIDATE_SIZE 0x190)
//  CWorld::Players = 0xB7CD98
// ----------------------------------------------------------------------------
struct CPlayerInfo {
    void*    playerPed;       // +0x00 CPlayerPed*
    uint8_t  _playerData[0xAC];// +0x04 CPlayerPedData
    void*    remoteVehicle;   // +0xB0
    void*    specCar;         // +0xB4
    int32_t  money;           // +0xB8 (классический адрес 0xB7CE50)
    int32_t  displayMoney;    // +0xBC
};
#if !defined(STALKER_SYNTAX_CHECK)
static_assert(offsetof(CPlayerInfo, money) == 0xB8, "CPlayerInfo::m_nMoney");

#endif
// ----------------------------------------------------------------------------
//  CColPoint (VALIDATE_SIZE(CColPoint, 0x2C)) — только как буфер для трассировок
// ----------------------------------------------------------------------------
struct CColPoint {
    uint8_t data[0x2C];
};

// ----------------------------------------------------------------------------
//  CColourSet (gta-reversed: source/game_sa/Collision/ColourSet.h)
//  CTimeCycle::m_CurrentColours = 0xB7C4A0
// ----------------------------------------------------------------------------
struct CColourSet {
    float  ambientRed, ambientGreen, ambientBlue;                       // +0x00
    float  ambientRedObj, ambientGreenObj, ambientBlueObj;              // +0x0C
    float  ambientRedBefore, ambientGreenBefore, ambientBlueBefore;     // +0x18
    uint16_t skyTopRed, skyTopGreen, skyTopBlue;                        // +0x24
    uint16_t skyBottomRed, skyBottomGreen, skyBottomBlue;               // +0x2A
    uint16_t sunCoreRed, sunCoreGreen, sunCoreBlue;                     // +0x30
    uint16_t sunCoronaRed, sunCoronaGreen, sunCoronaBlue;               // +0x36
    float   sunSize;                                                    // +0x3C
    float   spriteSize;                                                // +0x40
    float   spriteBrightness;                                          // +0x44
    int16_t shadowStrength;                                            // +0x48
    int16_t lightShadowStrength;                                       // +0x4A
    int16_t poleShadowStrength;                                        // +0x4C
    float   farClip;                                                   // +0x50
    float   fogStart;                                                  // +0x54
    float   lightsOnGroundBrightness;                                  // +0x58
    uint16_t lowCloudsRed, lowCloudsGreen, lowCloudsBlue;               // +0x5C
    uint16_t bottomCloudsRed, bottomCloudsGreen, bottomCloudsBlue;      // +0x62
    float   waterRed, waterGreen, waterBlue;                            // +0x68
    float   lightingOnBuildings_Brightness;                             // +0x74
};
#if !defined(STALKER_SYNTAX_CHECK)
static_assert(offsetof(CColourSet, skyTopRed) == 0x24, "CColourSet layout");

#endif
// ----------------------------------------------------------------------------
//  CRunningScript (gta-reversed: source/game_sa/Scripts/RunningScript.h)
// ----------------------------------------------------------------------------
struct CRunningScript {
    CRunningScript* next;     // +0x00
    CRunningScript* prev;     // +0x04
    char            name[8];  // +0x08
    uint8_t*        baseIP;   // +0x10
    uint8_t*        ip;       // +0x14
    uint8_t*        ipStack[8]; // +0x18
    uint16_t        stackDepth; // +0x38
    uint8_t         localVars[34 * 4]; // +0x3A m_LocalVars[34]
    // +0xC2:
    uint8_t  isActive;              // +0xC2
    uint8_t  condResult;            // +0xC3
    uint8_t  usesMissionCleanup;    // +0xC4
    uint8_t  isExternal;            // +0xC5
    uint8_t  isTextBlockOverride;   // +0xC6
    int8_t   externalType;          // +0xC7
    int32_t  wakeTime;              // +0xC8
    uint16_t andOrState;            // +0xCC
    uint8_t  notFlag;               // +0xCE
    uint8_t  isDeathArrestCheckEnabled; // +0xCF
    uint8_t  doneDeathArrest;       // +0xD0
    uint8_t  _pad1[3];
    int32_t  sceneSkipIP;           // +0xD4
    uint8_t  thisMustBeOnlyMission; // +0xD8 m_ThisMustBeTheOnlyMissionRunning
};
#if !defined(STALKER_SYNTAX_CHECK)
static_assert(offsetof(CRunningScript, thisMustBeOnlyMission) == 0xD8, "CRunningScript layout");

#endif
// ----------------------------------------------------------------------------
//  CStreamingInfo (gta-reversed: source/game_sa/StreamingInfo.h, размер 0x14)
//  CStreaming::ms_aInfoForModel = 0x8E4CC0
// ----------------------------------------------------------------------------
struct CStreamingInfo {
    int16_t nextIndex;      // +0x00
    int16_t prevIndex;      // +0x02
    int16_t nextIndexOnCd;  // +0x04
    uint8_t flags;          // +0x06
    uint8_t imgId;          // +0x07
    uint32_t cdPosn;        // +0x08
    uint32_t cdSize;        // +0x0C
    uint8_t loadState;      // +0x0E (eStreamingLoadState)
    // pad to 0x14
};
static_assert(sizeof(CStreamingInfo) == 0x14, "CStreamingInfo size");

enum eStreamingLoadState : uint8_t {
    LOADSTATE_NOT_LOADED = 0,
    LOADSTATE_LOADED = 1,
    LOADSTATE_REQUESTED = 2,
    LOADSTATE_READING = 3,
    LOADSTATE_FINISHING = 4,
};

// ----------------------------------------------------------------------------
//  CBaseModelInfo / CClumpModelInfo / CPedModelInfo
//  (gta-reversed: source/game_sa/Models/*.h, VALIDATE_SIZE(CPedModelInfo, 0x44))
// ----------------------------------------------------------------------------
struct CBaseModelInfo {
    void*    vtbl;            // +0x00
    uint32_t key;             // +0x04 m_nKey (CKeyGen)
    uint16_t refCount;        // +0x08
    int16_t  txdIndex;        // +0x0A m_nTxdIndex
    uint8_t  alpha;           // +0x0C
    uint8_t  twodfxCount;     // +0x0D
    int16_t  twodfxIndex;     // +0x0E
    int16_t  objectInfoIndex; // +0x10
    uint16_t flags;           // +0x12
    uint8_t  _pad[2];         // +0x14 (выравнивание указателя)
    void*    colModel;        // +0x14 CColModel*
    float    drawDistance;    // +0x18
    void*    rwObject;        // +0x1C RwObject* (RpClump* для педов)
    // CClumpModelInfo: +0x20 (4 байта) => 0x24
    // CPedModelInfo:
    //   +0x24 m_nAnimType, +0x28 m_nPedType, +0x2C m_nStatType,
    //   +0x30 m_nCarsCanDriveMask(u16), +0x32 m_nPedFlags(u16),
    //   +0x34 m_pHitColModel, +0x38 m_nRadio1(i16), +0x3A m_nRadio2(i16),
    //   +0x3C m_nRace(u8), +0x3D m_nPedAudioType(u8),
    //   +0x3E m_nVoiceMin(i16), +0x40 m_nVoiceMax(i16), +0x42 m_nVoiceId(i16)
    // (итоговый размер 0x44)
};

// Доступ к полям CPedModelInfo через указатель на CBaseModelInfo
inline int32_t& PedModelInfoAnimType(CBaseModelInfo* mi) { return *reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(mi) + 0x24); }
inline int32_t& PedModelInfoPedType(CBaseModelInfo* mi)  { return *reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(mi) + 0x28); }

// ----------------------------------------------------------------------------
//  Прочие константы
// ----------------------------------------------------------------------------
constexpr int      kMaxPedsInPool = 140;
constexpr float    kMapMinX = -3000.f, kMapMaxX = 3000.f;
constexpr float    kMapMinY = -3000.f, kMapMaxY = 3000.f;

// Типы чекпоинтов (gta-reversed: source/game_sa/Checkpoint.h, eCheckpointType)
enum eCheckpointType : uint16_t {
    CHECKPOINT_TUBE = 0,
    CHECKPOINT_ENDTUBE = 1,
    CHECKPOINT_EMPTYTUBE = 2,
    CHECKPOINT_TORUS = 3,
    CHECKPOINT_TORUS_NOFADE = 4,
};

} // namespace gta
