#pragma once
// ============================================================================
//  Торговля: редкие торговцы-сталкеры. Продажа артефактов, покупка снаряжения.
//  Торговцы спавнятся PopulationControl'ом как NPC фракции «сталкеры»;
//  клавиша T рядом с торговцем открывает меню (см. HudSystem::DrawTrade).
// ============================================================================
#include <cstdint>
#include <utility>
#include <vector>

namespace stalker {

class TradeSystem {
public:
    static TradeSystem& Get();

    // Ассортимент: пары (index предмета, запас). Запас восполняется со временем.
    typedef std::vector<std::pair<int32_t, int32_t>> StockT;

    StockT& GetStock() { return m_stock; }

    // HUD опрашивает и сбрасывает этот флаг для открытия окна торговли
    bool ConsumeOpenRequest() { bool r = m_openRequested; m_openRequested = false; return r; }

    void Tick(float dt);

    // Действие из UI (строка списка, режим покупки/продажи)
    void TradeAction(int row, bool sellMode);

    // Проверка: рядом ли торговец (в радиусе 4 м), возвращает имя или nullptr
    const char* NearbyTraderName();

    // Время до пополнения ассортимента
    void RestockIfNeeded();

private:
    TradeSystem() = default;

    StockT m_stock;
    float m_restockTimer = 300.f;
    float m_buyMarkup = 1.0f;
    bool  m_openRequested = false;
};

} // namespace stalker
