#pragma once
// ============================================================================
//  Общее состояние мода: предметы, инвентарь, состояние игрока.
//  Доступно всем системам через ModContext.
// ============================================================================
#include <cstdint>
#include <string>
#include <vector>

namespace stalker {

// ---------------------------------------------------------------------------
// Предметы
// ---------------------------------------------------------------------------
enum class ItemType : uint8_t {
    Consumable,   // медкит, бинты, антирад, еда
    GasMask,
    MaskFilter,
    Detector,     // «Отклик», «Велес»
    Artifact,
};

struct ItemDef {
    std::string id;          // внутренний ID (англ.)
    std::string name;        // отображаемое имя
    ItemType    type = ItemType::Consumable;
    float       weight = 0.1f;   // кг
    int32_t     price  = 0;      // $ у торговца
    bool        stackable = true;

    // Потребляемое:
    float healHealth = 0.f;  // HP
    float healArmour = 0.f;  // броня
    float healRad    = 0.f;  // снятие дозы радиации
    float healHunger = 0.f;  // «сытость» → небольшой реген HP на N сек

    // Артефакт:
    float artHealthRegen = 0.f; // HP/сек при ношении
    float artRadResist   = 0.f; // 0..1 снижение набора дозы
    float artRadHeal     = 0.f; // снижение дозы в сек
    float artMaxHpBonus  = 0.f; // прибавка к макс. HP
    float artBleedStop   = 0.f; // (зарезервировано)

    // Детектор: 1 = «Отклик» (аномалии), 2 = «Велес» (аномалии + артефакты)
    int32_t detectorLevel = 0;
};

struct ItemStack {
    int32_t  defIndex = -1;
    uint16_t count = 0;
    bool equipped = false; // для противогаза/детектора/артефакта (слот по типу)
};

// ---------------------------------------------------------------------------
// Состояние игрока
// ---------------------------------------------------------------------------
struct PlayerState {
    // Радиация
    float radDose     = 0.f;     // накопленная доза, 0..1000
    float radIntensity = 0.f;    // текущая мощность поля, мкР/с (для гейгера)

    // Противогаз
    bool  maskEquipped = false;
    float filterLeft   = 0.f;    // остаток ресурса фильтра, сек

    // Экипировка
    int32_t equippedMask     = -1; // index в каталоге предметов
    int32_t equippedDetector = -1;
    std::vector<int32_t> equippedArtifacts; // до 5 артефактов

    // Инвентарь
    std::vector<ItemStack> inventory;
    float maxWeight = 50.f;

    // Статистика
    uint32_t mutantsKilled = 0;
    uint32_t artifactsFound = 0;

    float hungerRegenTimer = 0.f; // время действия регена от еды

    // Модификаторы (пересчитываются из артефактов)
    float artifactHealthRegen = 0.f;
    float artifactRadResist   = 0.f;
    float artifactRadHeal     = 0.f;
    float artifactMaxHpBonus  = 0.f;

    float TotalWeight(const std::vector<ItemDef>& defs) const;
    int   CountItem(int32_t defIndex) const;
    bool  AddItem(int32_t defIndex, uint16_t count = 1);
    bool  RemoveItem(int32_t defIndex, uint16_t count = 1);
};

// ---------------------------------------------------------------------------
// Контекст мода (синглтон)
// ---------------------------------------------------------------------------
class Config;

struct ModContext {
    // Конфиги
    Config* cfgGeneral = nullptr;
    Config* cfgMutants = nullptr;
    Config* cfgZones   = nullptr;
    Config* cfgItems   = nullptr;
    Config* cfgFactions = nullptr;

    std::vector<ItemDef> items;

    PlayerState player;

    // Пути
    char gameDir[512] = {};
    char modDir[512] = {};   // <game>/STALKER_MOD

    bool nomutantsCheat = false; // STALKERNOMUTANTS
    bool debugPageShown = false;

    // Время в игре (сек, только активная сессия)
    float sessionTime = 0.f;

    static ModContext& Get();
};

// Утилиты над каталогом предметов
int  FindItemIndex(const char* id);
const ItemDef* GetItemDef(int32_t index);

// Пересчёт эффектов ношения (артефакты, противогаз)
void RecalcEquipmentEffects();

} // namespace stalker
