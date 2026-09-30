#include "TradeSystem.h"
#include "MutantDirector.h"
#include "../Core/Config.h"
#include "../Core/KeyState.h"
#include "../Core/Log.h"
#include "../Core/ModState.h"
#include "../Gta/GtaFunctions.h"

#include <cmath>

namespace stalker {

using namespace gta;

TradeSystem& TradeSystem::Get() {
    static TradeSystem inst;
    return inst;
}

void TradeSystem::RestockIfNeeded() {
    ModContext& ctx = ModContext::Get();
    m_stock.clear();

    struct Entry { const char* id; int qty; };
    static const Entry defStock[] = {
        { "medkit", 5 }, { "bandage", 10 }, { "antirad", 6 },
        { "bread", 8 }, { "sausage", 6 },
        { "filter", 10 }, { "gasmask", 1 },
        { "detector_otklik", 2 }, { "detector_veles", 1 },
        { "art_medallion", 1 }, { "art_flash", 1 },
    };
    for (auto& e : defStock) {
        int idx = FindItemIndex(e.id);
        if (idx >= 0) m_stock.push_back({ idx, e.qty });
    }
    // Ассортимент из конфига (если задан)
    if (ctx.cfgItems) {
        auto sections = ctx.cfgItems->GetSectionsWithPrefix("trader_stock:");
        for (auto& s : sections) {
            std::string id = s.substr(13);
            int idx = FindItemIndex(id.c_str());
            if (idx >= 0) {
                int qty = ctx.cfgItems->GetInt(s.c_str(), "qty", 3);
                bool replaced = false;
                for (auto& st : m_stock)
                    if (st.first == idx) { st.second = qty; replaced = true; break; }
                if (!replaced) m_stock.push_back({ idx, qty });
            }
        }
    }
}

void TradeSystem::Tick(float dt) {
    m_restockTimer -= dt;
    if (m_restockTimer <= 0.f) {
        m_restockTimer = 300.f;
        RestockIfNeeded();
    }

    // Открытие торговли клавишей T рядом с торговцем
    ModContext& ctx = ModContext::Get();
    if (!IsGameSessionActive() || UserPause()) return;

    static KeyState keys;
    keys.Update(NewKeyState().standardKeys, OldKeyState().standardKeys);
    int tradeVk = 'T';
    if (ctx.cfgGeneral) tradeVk = ctx.cfgGeneral->GetInt("keys", "trade", 'T');

    if (keys.Pressed(tradeVk)) {
        const char* trader = NearbyTraderName();
        if (trader) {
            RestockIfNeeded();
            m_openRequested = true; // HUD откроет окно на следующем тике
        } else {
            Log::Write("Торговца рядом нет");
        }
    }
}

const char* TradeSystem::NearbyTraderName() {
    CPed* player = PlayerPed();
    if (!player) return nullptr;
    const CVector& pos = player->GetPosition();

    for (auto& e : MutantDirector::Get().Entities()) {
        if (e.factionId != 0) continue; // 0 = «Сталкеры» (торговцы)
        if (!e.ped || !e.ped->IsAlive()) continue;
        if ((e.ped->GetPosition() - pos).Magnitude() < 5.f)
            return "Stalker trader";
    }
    return nullptr;
}

void TradeSystem::TradeAction(int row, bool sellMode) {    ModContext& ctx = ModContext::Get();
    PlayerState& p = ctx.player;
    CPlayerInfo* pi = PlayerInfo();
    if (!pi) return;

    if (!sellMode) {
        // Покупка
        if (row < 0 || row >= (int)m_stock.size()) return;
        auto& entry = m_stock[row];
        const ItemDef* def = GetItemDef(entry.first);
        if (!def || entry.second <= 0) return;
        if (pi->money < def->price) {
            Log::Write("Торговля: не хватает денег");
            return;
        }
        pi->money -= def->price;
        pi->displayMoney = pi->money;
        p.AddItem(entry.first, 1);
        entry.second -= 1;
    } else {
        // Продажа (в 2 раза дешевле)
        if (row < 0 || row >= (int)p.inventory.size()) return;
        ItemStack& st = p.inventory[row];
        const ItemDef* def = GetItemDef(st.defIndex);
        if (!def) return;
        int price = def->price / 2;
        if (p.RemoveItem(st.defIndex, 1)) {
            pi->money += price;
            pi->displayMoney = pi->money;
        }
    }
}

} // namespace stalker
