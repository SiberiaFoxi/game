#pragma once
// ============================================================================
//  HUD мода: счётчик Гейгера, доза, предупреждения об аномалиях, таймер
//  выброса, экран инвентаря/торговли, отладочная страница (F10).
//  Рисуется в PRE-хуке CHud::Draw (0x58FAE0) средствами CFont/CSprite2d.
// ============================================================================
#include <cstdint>
#include <string>
#include <vector>

namespace stalker {

class HudSystem {
public:
    static HudSystem& Get();

    void TickUI(float dt);   // навигация по меню (в игровом тике)
    void Draw();             // отрисовка (в фазе рендера)

    void PushMessage(const std::string& msg, float seconds = 4.f);

    bool inventoryOpen = false;
    bool tradeOpen = false;

private:
    HudSystem() = default;

    void DrawBars();
    void DrawMessages(float dt);
    void DrawBlowout();
    void DrawInventory();
    void DrawTrade();
    void DrawDebug();

    void DrawText(float x, float y, const char* text, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void DrawBar(float x, float y, float w, float h, float frac, uint8_t r, uint8_t g, uint8_t b);
    void DrawQuad(float x1, float y1, float x2, float y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void DrawGasMaskOverlay();

    struct Msg { std::string text; float life; };
    std::vector<Msg> m_messages;

    // Разрешение экрана (обновляется при отрисовке)
    float m_screenW = 640.f, m_screenH = 448.f;

    // Захват инвентаря/торговли для навигации
    int  m_invCursor = 0;
    int  m_tradeCursor = 0;
    bool m_tradeSellMode = false;
};

} // namespace stalker
