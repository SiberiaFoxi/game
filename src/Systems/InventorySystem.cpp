#include "InventorySystem.h"
#include "AnomalySystem.h"
#include "../Core/Config.h"
#include "../Core/KeyState.h"
#include "../Core/Log.h"
#include "../Core/ModState.h"
#include "../Gta/GtaFunctions.h"

#include <cmath>

namespace stalker {

using namespace gta;

InventorySystem& InventorySystem::Get() {
    static InventorySystem inst;
    return inst;
}

void InventorySystem::LoadItems() {
    ModContext& ctx = ModContext::Get();
    ctx.items.clear();

    auto addItem = [](ItemDef d) {
        ModContext::Get().items.push_back(d);
    };

    if (ctx.cfgItems) {
        Config& c = *ctx.cfgItems;
        auto sections = c.GetSectionsWithPrefix("item:");
        for (auto& s : sections) {
            ItemDef d;
            d.id   = s.substr(5);
            d.name = c.GetString(s, "name", d.id.c_str());
            std::string type = c.GetString(s, "type", "consumable");
            if (type == "gasmask")        d.type = ItemType::GasMask;
            else if (type == "filter")    d.type = ItemType::MaskFilter;
            else if (type == "detector")  d.type = ItemType::Detector;
            else if (type == "artifact")  d.type = ItemType::Artifact;
            else                          d.type = ItemType::Consumable;
            d.weight = c.GetFloat(s, "weight", 0.1f);
            d.price  = c.GetInt(s, "price", 100);
            d.stackable = c.GetBool(s, "stackable", d.type != ItemType::GasMask);
            d.healHealth = c.GetFloat(s, "heal_health", 0.f);
            d.healArmour = c.GetFloat(s, "heal_armour", 0.f);
            d.healRad    = c.GetFloat(s, "heal_rad", 0.f);
            d.healHunger = c.GetFloat(s, "regen_sec", 0.f);
            d.artHealthRegen = c.GetFloat(s, "art_health_regen", 0.f);
            d.artRadResist   = c.GetFloat(s, "art_rad_resist", 0.f);
            d.artRadHeal     = c.GetFloat(s, "art_rad_heal", 0.f);
            d.artMaxHpBonus  = c.GetFloat(s, "art_max_hp", 0.f);
            d.detectorLevel  = c.GetInt(s, "detector_level", 0);
            addItem(d);
        }
    }

    // Встроенный минимум, если конфига нет
    if (ctx.items.empty()) {
        { ItemDef d{"medkit", "Medkit", ItemType::Consumable, 0.4f, 300, true, 55.f, 0, 0, 0, 0,0,0,0,0, 0}; addItem(d); }
        { ItemDef d{"bandage", "Bandage", ItemType::Consumable, 0.1f, 80, true, 15.f, 0, 0, 0, 0,0,0,0,0, 0}; addItem(d); }
        { ItemDef d{"antirad", "Anti-rad", ItemType::Consumable, 0.15f, 250, true, 0, 0, 350.f, 0, 0,0,0,0,0, 0}; addItem(d); }
        { ItemDef d{"bread", "Bread", ItemType::Consumable, 0.3f, 40, true, 4.f, 0, 0, 25.f, 0,0,0,0,0, 0}; addItem(d); }
        { ItemDef d{"sausage", "Sausage", ItemType::Consumable, 0.35f, 65, true, 6.f, 0, 0, 35.f, 0,0,0,0,0, 0}; addItem(d); }
        { ItemDef d{"gasmask", "Gas mask", ItemType::GasMask, 2.0f, 1500, false, 0,0,0,0, 0,0,0,0,0, 0}; addItem(d); }
        { ItemDef d{"filter", "Gas mask filter", ItemType::MaskFilter, 0.5f, 300, true, 0,0,0,0, 0,0,0,0,0, 0}; addItem(d); }
        { ItemDef d{"detector_otklik", "Detector 'Otklik'", ItemType::Detector, 0.6f, 1200, false, 0,0,0,0, 0,0,0,0,0, 1}; addItem(d); }
        { ItemDef d{"detector_veles", "Detector 'Veles'", ItemType::Detector, 0.9f, 4500, false, 0,0,0,0, 0,0,0,0,0, 2}; addItem(d); }
        // Артефакты
        { ItemDef d{"art_medallion", "Medallion", ItemType::Artifact, 0.5f, 3000, true, 0,0,0,0, 0.30f, 0, 0, 10.f, 0, 0}; addItem(d); }
        { ItemDef d{"art_flash", "Flash", ItemType::Artifact, 0.4f, 2500, true, 0,0,0,0, 0, 0.25f, 0, 0, 0, 0}; addItem(d); }
        { ItemDef d{"art_moonlight", "Moonlight", ItemType::Artifact, 0.4f, 2800, true, 0,0,0,0, 0, 0.35f, 0, 0, 0, 0}; addItem(d); }
        { ItemDef d{"art_kolobok", "Kolobok", ItemType::Artifact, 0.8f, 3500, true, 0,0,0,0, 0.15f, 0, 0, 25.f, 0, 0}; addItem(d); }
        { ItemDef d{"art_mama's_beads", "Mama's beads", ItemType::Artifact, 0.6f, 3200, true, 0,0,0,0, 0.25f, 0.10f, 0, 0, 0, 0}; addItem(d); }
        { ItemDef d{"art_crystal", "Crystal", ItemType::Artifact, 0.7f, 3000, true, 0,0,0,0, 0, 0.15f, 1.5f, 0, 0, 0}; addItem(d); }
        { ItemDef d{"art_slime", "Slime", ItemType::Artifact, 0.9f, 2200, true, 0,0,0,0, 0.10f, 0.20f, 0, 0, 0, 0}; addItem(d); }
    }

    Log::Write("InventorySystem: %d предметов", (int)ctx.items.size());
}

void InventorySystem::GiveStarterKit() {
    PlayerState& p = ModContext::Get().player;
    p.AddItem(FindItemIndex("medkit"), 2);
    p.AddItem(FindItemIndex("bandage"), 3);
    p.AddItem(FindItemIndex("bread"), 2);
    p.AddItem(FindItemIndex("sausage"), 1);
    p.AddItem(FindItemIndex("antirad"), 1);
    // Противогаз и «Отклик» новичку — так честнее для выживания
    int mask = FindItemIndex("gasmask");
    p.AddItem(mask, 1);
    p.equippedMask = mask;
    int filter = FindItemIndex("filter");
    p.AddItem(filter, 2);
    p.filterLeft = 600.f; // 10 минут фильтра
    int det = FindItemIndex("detector_otklik");
    p.AddItem(det, 1);
    p.equippedDetector = det;
}

void InventorySystem::Consume(int defIndex) {
    const ItemDef* def = GetItemDef(defIndex);
    if (!def) return;
    PlayerState& p = ModContext::Get().player;
    if (!p.RemoveItem(defIndex, 1)) return;

    if (def->healHealth > 0.f) HealPlayerPed(def->healHealth);
    if (def->healArmour > 0.f) {
        if (CPed* ped = PlayerPed()) {
            float& armour = ped->Armour();
            armour += def->healArmour;
            if (armour > 100.f) armour = 100.f;
        }
    }
    if (def->healRad > 0.f)
        p.radDose = (p.radDose > def->healRad) ? p.radDose - def->healRad : 0.f;
    if (def->healHunger > 0.f)
        p.hungerRegenTimer = def->healHunger;
}

void InventorySystem::UseSelected() {
    PlayerState& p = ModContext::Get().player;
    if (p.inventory.empty()) return;
    if (cursor < 0 || cursor >= (int)p.inventory.size()) return;
    ItemStack& st = p.inventory[cursor];

    const ItemDef* def = GetItemDef(st.defIndex);
    if (!def) return;

    switch (def->type) {
    case ItemType::Consumable:
    case ItemType::MaskFilter:
        if (def->type == ItemType::MaskFilter) {
            // Установить новый фильтр в противогаз
            if (p.equippedMask < 0) { Log::Write("Инвентарь: нет противогаза"); return; }
            p.filterLeft = 600.f;
        }
        Consume(st.defIndex);
        break;

    case ItemType::GasMask:
        EquipToggle(cursor);
        break;

    case ItemType::Detector:
        if (p.equippedDetector == st.defIndex) p.equippedDetector = -1;
        else p.equippedDetector = st.defIndex;
        break;

    case ItemType::Artifact: {
        // Надеть/снять артефакт (до 5)
        for (size_t i = 0; i < p.equippedArtifacts.size(); ++i) {
            if (p.equippedArtifacts[i] == st.defIndex) {
                p.equippedArtifacts.erase(p.equippedArtifacts.begin() + i);
                RecalcEquipmentEffects();
                return;
            }
        }
        if (p.equippedArtifacts.size() < 5) {
            p.equippedArtifacts.push_back(st.defIndex);
            RecalcEquipmentEffects();
        }
        break;
    }
    default:
        break;
    }
}

void InventorySystem::EquipToggle(int inventoryRow) {
    PlayerState& p = ModContext::Get().player;
    if (inventoryRow < 0 || inventoryRow >= (int)p.inventory.size()) return;
    ItemStack& st = p.inventory[inventoryRow];
    const ItemDef* def = GetItemDef(st.defIndex);
    if (!def || def->type != ItemType::GasMask) return;

    if (p.equippedMask == st.defIndex) {
        p.equippedMask = -1;
        p.maskEquipped = false;
    } else {
        p.equippedMask = st.defIndex;
        p.maskEquipped = true;
    }
}

void InventorySystem::DropSelected() {
    PlayerState& p = ModContext::Get().player;
    if (p.inventory.empty()) return;
    if (cursor < 0 || cursor >= (int)p.inventory.size()) return;
    ItemStack& st = p.inventory[cursor];
    // Снять, если экипировано
    for (size_t i = 0; i < p.equippedArtifacts.size(); ++i)
        if (p.equippedArtifacts[i] == st.defIndex)
            p.equippedArtifacts.erase(p.equippedArtifacts.begin() + i);
    if (p.equippedMask == st.defIndex) { p.equippedMask = -1; p.maskEquipped = false; }
    if (p.equippedDetector == st.defIndex) p.equippedDetector = -1;
    RecalcEquipmentEffects();
    p.RemoveItem(st.defIndex, 1);
    if (cursor >= (int)p.inventory.size()) cursor = (int)p.inventory.size() - 1;
}

void InventorySystem::Tick(float dt) {
    (void)dt;
    if (!IsGameSessionActive() || UserPause()) return;

    ModContext& ctx = ModContext::Get();
    PlayerState& p = ctx.player;
    CPed* player = PlayerPed();
    if (!player) return;

    // Подбор артефактов — на пешком/медленно, рядом
    if (player->MoveSpeed().Magnitude2D() < 12.f) {
        int32_t art = AnomalySystem::Get().TryPickupArtifact(player->GetPosition());
        if (art >= 0) {
            p.AddItem(art, 1);
            const ItemDef* def = GetItemDef(art);
            Log::Write("Найден артефакт: %s", def ? def->name.c_str() : "?");
        }
    }

    // Автоприменение фильтра: если фильтр мёртв и есть запасной — ставим
    if (p.maskEquipped && p.filterLeft <= 0.f) {
        int filter = FindItemIndex("filter");
        if (p.CountItem(filter) > 0) {
            p.RemoveItem(filter, 1);
            p.filterLeft = 600.f;
            Log::Write("Инвентарь: установлен новый фильтр");
        }
    }

    // Макс. HP с учётом артефактов
    if (player->MaxHealth() > 0.f) {
        float targetMax = 100.f + p.artifactMaxHpBonus;
        if (player->MaxHealth() != targetMax)
            player->MaxHealth() = targetMax;
    }
}

} // namespace stalker
