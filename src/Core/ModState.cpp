#include "ModState.h"
#include "Config.h"

namespace stalker {

ModContext& ModContext::Get() {
    static ModContext ctx;
    return ctx;
}

float PlayerState::TotalWeight(const std::vector<ItemDef>& defs) const {
    float w = 0.f;
    for (const auto& st : inventory) {
        if (st.defIndex >= 0 && st.defIndex < (int)defs.size())
            w += defs[st.defIndex].weight * st.count;
    }
    return w;
}

int PlayerState::CountItem(int32_t defIndex) const {
    for (const auto& st : inventory)
        if (st.defIndex == defIndex) return st.count;
    return 0;
}

bool PlayerState::AddItem(int32_t defIndex, uint16_t count) {
    if (defIndex < 0) return false;
    for (auto& st : inventory) {
        if (st.defIndex == defIndex) {
            st.count = static_cast<uint16_t>(st.count + count);
            return true;
        }
    }
    ItemStack st;
    st.defIndex = defIndex;
    st.count = count;
    inventory.push_back(st);
    return true;
}

bool PlayerState::RemoveItem(int32_t defIndex, uint16_t count) {
    for (size_t i = 0; i < inventory.size(); ++i) {
        if (inventory[i].defIndex != defIndex) continue;
        if (inventory[i].count < count) return false;
        inventory[i].count = static_cast<uint16_t>(inventory[i].count - count);
        if (inventory[i].count == 0) inventory.erase(inventory.begin() + i);
        return true;
    }
    return false;
}

int FindItemIndex(const char* id) {
    auto& items = ModContext::Get().items;
    for (size_t i = 0; i < items.size(); ++i)
        if (items[i].id == id) return (int)i;
    return -1;
}

const ItemDef* GetItemDef(int32_t index) {
    auto& items = ModContext::Get().items;
    if (index < 0 || index >= (int)items.size()) return nullptr;
    return &items[index];
}

void RecalcEquipmentEffects() {
    ModContext& ctx = ModContext::Get();
    PlayerState& p = ctx.player;

    p.artifactHealthRegen = 0.f;
    p.artifactRadResist   = 0.f;
    p.artifactRadHeal     = 0.f;
    p.artifactMaxHpBonus  = 0.f;

    for (int32_t idx : p.equippedArtifacts) {
        const ItemDef* def = GetItemDef(idx);
        if (!def) continue;
        p.artifactHealthRegen += def->artHealthRegen;
        p.artifactRadResist   += def->artRadResist;
        p.artifactRadHeal     += def->artRadHeal;
        p.artifactMaxHpBonus  += def->artMaxHpBonus;
    }
    if (p.artifactRadResist > 0.95f) p.artifactRadResist = 0.95f;
}

} // namespace stalker
