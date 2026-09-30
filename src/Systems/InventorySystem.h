#pragma once
// ============================================================================
//  Инвентарь: использование предметов, экипировка (противогаз, детектор,
//  артефакты), вес. UI отрисовывается в HudSystem.
//  Каталог предметов грузится из STALKER_MOD/items.cfg.
// ============================================================================
#include <cstdint>

namespace stalker {

class InventorySystem {
public:
    static InventorySystem& Get();

    void LoadItems();           // из cfgItems (+ встроенные по умолчанию)
    void Tick(float dt);        // подбор артефактов, клавиши
    void UseSelected();         // использовать выбранный предмет
    void EquipToggle(int inventoryRow);
    void DropSelected();

    // UI-состояние (навигация по списку)
    int  cursor = 0;
    bool visible = false;

    // Дефолтный набор новичка
    void GiveStarterKit();

private:
    InventorySystem() = default;

    void Consume(int defIndex);
};

} // namespace stalker
