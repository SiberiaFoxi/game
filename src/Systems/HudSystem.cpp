#include "HudSystem.h"
#include "Core/Config.h"
#include "AnomalySystem.h"
#include "BlowoutSystem.h"
#include "InventorySystem.h"
#include "MutantDirector.h"
#include "RadiationSystem.h"
#include "TradeSystem.h"
#include "../Core/KeyState.h"
#include "../Core/ModState.h"
#include "../Core/SaveSystem.h"
#include "../Gta/GtaFunctions.h"

#include <cmath>
#include <cstdio>
#include <algorithm>

namespace stalker {

using namespace gta;

HudSystem& HudSystem::Get() {
    static HudSystem inst;
    return inst;
}

void HudSystem::PushMessage(const std::string& msg, float seconds) {
    m_messages.push_back({ msg, seconds });
    if (m_messages.size() > 6) m_messages.erase(m_messages.begin());
}

// ---------------------------------------------------------------------------
// Навигация (в игровом тике, где доступен CPad)
// ---------------------------------------------------------------------------
void HudSystem::TickUI(float dt) {
    (void)dt;
    ModContext& ctx = ModContext::Get();
    PlayerState& p = ctx.player;

    static KeyState keys;
    // CKeyboardState: standardKeys — по виртуальному коду клавиши
    keys.Update(NewKeyState().standardKeys, OldKeyState().standardKeys);

    // Открытие окна торговли по запросу TradeSystem (клавиша T у торговца)
    if (TradeSystem::Get().ConsumeOpenRequest()) {
        tradeOpen = true;
        inventoryOpen = false;
    }

    // Экран инвентаря — стрелки работают как отдельные флаги CKeyboardState
    // (up/down/left/right), Enter — стандартная клавиша
    const CKeyboardState& nk = NewKeyState();
    const CKeyboardState& ok = OldKeyState();

    bool invKey = false;
    int invVk = 'I';
    if (ctx.cfgGeneral) invVk = ctx.cfgGeneral->GetInt("keys", "inventory", 'I');
    invKey = keys.Pressed(invVk);

    if (invKey && !tradeOpen) {
        inventoryOpen = !inventoryOpen;
        if (inventoryOpen) m_invCursor = 0;
    }

    if (inventoryOpen) {
        if (nk.up && !ok.up)
            m_invCursor = std::max(0, m_invCursor - 1);
        if (nk.down && !ok.down)
            m_invCursor = std::min((int)p.inventory.size() - 1, m_invCursor + 1);
        if (keys.Pressed(13)) {
            InventorySystem::Get().cursor = m_invCursor;
            InventorySystem::Get().UseSelected();
        }
        if (keys.Pressed('U')) {
            InventorySystem::Get().cursor = m_invCursor;
            InventorySystem::Get().DropSelected();
            m_invCursor = std::min(m_invCursor, std::max(0, (int)p.inventory.size() - 1));
        }
    }

    // Торговля — рядом с торговцем (открывается из TradeSystem)
    if (tradeOpen) {
        if (nk.up && !ok.up) m_tradeCursor = std::max(0, m_tradeCursor - 1);
        if (nk.down && !ok.down) m_tradeCursor = std::min(64, m_tradeCursor + 1);
        if (keys.Pressed(13)) TradeSystem::Get().TradeAction(m_tradeCursor, m_tradeSellMode);
        if (keys.Pressed(9 /*Tab*/)) m_tradeSellMode = !m_tradeSellMode;
        if (invKey || (nk.esc && !ok.esc)) tradeOpen = false;
    }

    // F9 — ручное сохранение прогресса мода; F10 — отладка
    // (F-клавиши — отдельные поля CKeyboardState: FKeys[8] = F9, FKeys[9] = F10)
    if (nk.FKeys[8] && !ok.FKeys[8]) {
        SaveSystem::Get().Save();
        PushMessage("Mod progress saved", 2.f);
    }
    if (nk.FKeys[9] && !ok.FKeys[9])
        ctx.debugPageShown = !ctx.debugPageShown;
}

// ---------------------------------------------------------------------------
// Отрисовка
// ---------------------------------------------------------------------------
void HudSystem::DrawText(float x, float y, const char* text, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    fn::FontSetColor(CRGBA(r, g, b, a));
    fn::FontPrintString(x, y, text);
}

void HudSystem::DrawQuad(float x1, float y1, float x2, float y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    fn::SpriteDraw2DPolygon(x1, y1, x2, y1, x2, y2, x1, y2, CRGBA(r, g, b, a));
}

void HudSystem::DrawBar(float x, float y, float w, float h, float frac, uint8_t r, uint8_t g, uint8_t b) {
    frac = std::max(0.f, std::min(1.f, frac));
    DrawQuad(x - 2, y - 2, x + w + 2, y + h + 2, 0, 0, 0, 150);
    DrawQuad(x, y, x + w * frac, y + h, r, g, b, 220);
}

void HudSystem::Draw() {
    if (!IsGameSessionActive()) return;

    ModContext& ctx = ModContext::Get();

    // Разрешение: RsGlobal->maximumwidth (0xC17048) x maximumheight
    // (gta-reversed: source/game_sa/app/platform / RenderWare RsGlobal 0xC17040)
    uint32_t* rsMaxW = reinterpret_cast<uint32_t*>(0xC17048);
    uint32_t* rsMaxH = reinterpret_cast<uint32_t*>(0xC1704C);
    if (*rsMaxW > 0 && *rsMaxW < 4096) m_screenW = (float)*rsMaxW;
    if (*rsMaxH > 0 && *rsMaxH < 4096) m_screenH = (float)*rsMaxH;

    fn::FontInitPerFrame();
    fn::FontSetFontStyle(2 /*FONT_MENU*/);
    fn::FontSetScale(0.55f * (m_screenW / 640.f), 1.0f * (m_screenH / 448.f));
    fn::FontSetProportional(true);
    fn::FontSetEdge(1);
    fn::FontSetDropColor(CRGBA(0, 0, 0, 255));

    DrawBars();
    DrawGasMaskOverlay();
    DrawBlowout();
    if (inventoryOpen) DrawInventory();
    if (tradeOpen) DrawTrade();
    if (ctx.debugPageShown) DrawDebug();

    fn::FontRenderFontBuffer();
}

void HudSystem::DrawBars() {
    ModContext& ctx = ModContext::Get();
    PlayerState& p = ctx.player;
    RadiationSystem& rad = RadiationSystem::Get();

    float scale = m_screenH / 448.f;
    float x = m_screenW - 190.f * (m_screenW / 640.f);
    float y = m_screenH - 90.f * scale;
    char buf[96];

    // Зона
    const char* zone = MutantDirector::Get().ZoneNameAt(
        PlayerPed() ? PlayerPed()->GetPosition().x : 0.f,
        PlayerPed() ? PlayerPed()->GetPosition().y : 0.f);
    if (zone && zone[0]) {
        snprintf(buf, sizeof(buf), "%s", zone);
        DrawText(x, y, buf, 220, 220, 220, 230);
        y += 16.f * scale;
    }

    // Гейгер: интенсивность
    float inten = rad.Intensity();
    if (inten > 0.01f) {
        uint8_t rc = inten < 0.5f ? 120 : 255;
        snprintf(buf, sizeof(buf), "RAD %d uR/h", (int)(inten * 100.f));
        DrawText(x, y, buf, rc, 200, 80, 240);
        y += 15.f * scale;
    }

    // Доза
    snprintf(buf, sizeof(buf), "DOSE %d/1000", (int)p.radDose);
    DrawText(x, y, buf, 200, 220, 120, 230);
    DrawBar(x + 95.f, y + 2.f, 80.f, 6.f, p.radDose / 1000.f, 220, 120, 40);
    y += 15.f * scale;

    // Фильтр противогаза
    if (p.maskEquipped) {
        float frac = p.filterLeft > 0.f ? p.filterLeft / 600.f : 0.f;
        snprintf(buf, sizeof(buf), "FILTER %d%%", (int)(frac * 100.f));
        uint8_t fc = frac > 0.25f ? 140 : 255;
        DrawText(x, y, buf, fc, frac > 0.25f ? 200 : 90, 120, 230);
        DrawBar(x + 95.f, y + 2.f, 80.f, 6.f, frac, 120, 180, 220);
        y += 15.f * scale;
    }

    // Вес рюкзака
    snprintf(buf, sizeof(buf), "CARRY %.1f/%.0f kg", p.TotalWeight(ctx.items), p.maxWeight);
    DrawText(x, y, buf, 200, 200, 200, 200);
    y += 15.f * scale;

    // Детектор: ближайшая аномалия
    if (p.equippedDetector >= 0 && PlayerPed()) {
        const ItemDef* det = GetItemDef(p.equippedDetector);
        if (det && det->detectorLevel > 0) {
            float d = 0.f;
            const Anomaly* a = AnomalySystem::Get().NearestAnomaly(PlayerPed()->GetPosition(), 30.f, &d);
            if (a) {
                snprintf(buf, sizeof(buf), "! %s (%d m)", AnomalyTypeName(a->type), (int)d);
                DrawText(x, y, buf, 255, 120, 120, 230);
                y += 15.f * scale;
            }
            if (det->detectorLevel >= 2) {
                float ad = 0.f;
                const ArtifactSpawn* art = AnomalySystem::Get().NearestArtifact(PlayerPed()->GetPosition(), 40.f, &ad);
                if (art) {
                    const ItemDef* adef = GetItemDef(art->itemDefIndex);
                    snprintf(buf, sizeof(buf), "* %s (%d m)",
                             adef ? adef->name.c_str() : "Artifact", (int)ad);
                    DrawText(x, y, buf, 255, 220, 90, 230);
                    y += 15.f * scale;
                }
            }
        }
    }

    // Сообщения
    float msgY = 40.f * scale;
    for (auto& m : m_messages) {
        if (m.life <= 0.f) continue;
        DrawText(20.f * (m_screenW / 640.f), msgY, m.text.c_str(), 240, 240, 240, 220);
        msgY += 14.f * scale;
    }
}

void HudSystem::DrawMessages(float) {}

void HudSystem::DrawBlowout() {
    BlowoutSystem& b = BlowoutSystem::Get();
    if (b.GetPhase() == BlowoutSystem::PHASE_IDLE) return;

    char buf[96];
    float cx = m_screenW * 0.5f - 120.f;
    float cy = 60.f;
    if (b.GetPhase() == BlowoutSystem::PHASE_WARNING) {
        snprintf(buf, sizeof(buf), "!!! BLOWOUT IN %d sec - FIND COVER !!!", (int)b.PhaseTimeLeft());
        DrawText(cx, cy, buf, 255, 80, 60, 255);
    } else if (b.GetPhase() == BlowoutSystem::PHASE_ACTIVE) {
        DrawText(cx, cy, ">>> BLOWOUT <<<", 255, 50, 40, 255);
        if (BlowoutSystem::Get().IsPlayerInCover())
            DrawText(cx, cy + 16.f, "You are in cover", 120, 255, 120, 230);
        else
            DrawText(cx, cy + 16.f, "NO COVER! FATAL DOSE!", 255, 60, 60, 255);
    } else {
        DrawText(cx, cy, "Blowout is fading...", 255, 140, 100, 220);
    }
}

void HudSystem::DrawGasMaskOverlay() {
    PlayerState& p = ModContext::Get().player;
    if (!p.maskEquipped) return;
    // Затемнение по краям экрана («маска»)
    float edge = 60.f * (m_screenH / 448.f);
    DrawQuad(0, 0, m_screenW, edge, 10, 10, 12, 160);
    DrawQuad(0, m_screenH - edge, m_screenW, m_screenH, 10, 10, 12, 160);
    DrawQuad(0, 0, edge, m_screenH, 10, 10, 12, 160);
    DrawQuad(m_screenW - edge, 0, m_screenW, m_screenH, 10, 10, 12, 160);
}

void HudSystem::DrawInventory() {
    ModContext& ctx = ModContext::Get();
    PlayerState& p = ctx.player;

    float x = m_screenW * 0.22f, y = m_screenH * 0.12f;
    float w = m_screenW * 0.56f, h = m_screenH * 0.7f;
    DrawQuad(x, y, x + w, y + h, 12, 14, 10, 190);

    DrawText(x + 14, y + 8, "INVENTORY  [I]-close  [Enter]-use  [U]-drop", 230, 230, 200, 255);

    char buf[96];
    float ly = y + 32.f;
    int shown = 0;
    for (size_t i = 0; i < p.inventory.size() && shown < 16; ++i) {
        const ItemStack& st = p.inventory[i];
        const ItemDef* def = GetItemDef(st.defIndex);
        if (!def) continue;
        bool eq = (p.equippedMask == st.defIndex) ||
                  (p.equippedDetector == st.defIndex) ||
                  (std::find(p.equippedArtifacts.begin(), p.equippedArtifacts.end(), st.defIndex) != p.equippedArtifacts.end());
        snprintf(buf, sizeof(buf), "%c %s x%d  %.1fkg%s",
                 (int)i == m_invCursor ? '>' : ' ',
                 def->name.c_str(), st.count, def->weight * st.count,
                 eq ? "  [ON]" : "");
        uint8_t col = eq ? 140 : 240;
        DrawText(x + 22, ly, buf, col, eq ? 255 : 240, eq ? 140 : 240, 255);
        ly += 15.f;
        ++shown;
    }
    if (p.inventory.empty())
        DrawText(x + 22, ly, "(empty)", 180, 180, 180, 220);

    snprintf(buf, sizeof(buf), "Total: %.1f / %.0f kg", p.TotalWeight(ctx.items), p.maxWeight);
    DrawText(x + 14, y + h - 22, buf, 200, 200, 160, 230);
}

void HudSystem::DrawTrade() {
    TradeSystem& tr = TradeSystem::Get();
    ModContext& ctx = ModContext::Get();
    PlayerState& p = ctx.player;

    float x = m_screenW * 0.18f, y = m_screenH * 0.10f;
    float w = m_screenW * 0.64f, h = m_screenH * 0.74f;
    DrawQuad(x, y, x + w, y + h, 14, 12, 8, 200);

    char buf[128];
    snprintf(buf, sizeof(buf), "TRADER   $%d   [Tab]-%s  [Enter]-trade  [I]-close",
             PlayerInfo()->money, m_tradeSellMode ? "SELL" : "BUY");
    DrawText(x + 14, y + 8, buf, 230, 220, 160, 255);

    float ly = y + 34.f;
    if (!m_tradeSellMode) {
        auto& stock = tr.GetStock();
        for (size_t i = 0; i < stock.size() && ly < y + h - 40.f; ++i) {
            const ItemDef* def = GetItemDef(stock[i].first);
            if (!def) continue;
            snprintf(buf, sizeof(buf), "%c %-18s $%-6d",
                     (int)i == m_tradeCursor ? '>' : ' ',
                     def->name.c_str(), def->price);
            DrawText(x + 22, ly, buf, 220, 220, 220, 255);
            ly += 15.f;
        }
    } else {
        for (size_t i = 0; i < p.inventory.size() && ly < y + h - 40.f; ++i) {
            const ItemStack& st = p.inventory[i];
            const ItemDef* def = GetItemDef(st.defIndex);
            if (!def) continue;
            snprintf(buf, sizeof(buf), "%c %-18s x%-3d $%-6d",
                     (int)i == m_tradeCursor ? '>' : ' ',
                     def->name.c_str(), st.count, def->price / 2);
            DrawText(x + 22, ly, buf, 220, 220, 220, 255);
            ly += 15.f;
        }
    }
}

void HudSystem::DrawDebug() {
    ModContext& ctx = ModContext::Get();
    CPed* player = PlayerPed();
    if (!player) return;

    char buf[96];
    float x = 16.f, y = m_screenH * 0.35f;
    DrawQuad(x - 6, y - 6, x + 240.f, y + 120.f, 0, 0, 0, 170);
    const CVector& pos = player->GetPosition();
    snprintf(buf, sizeof(buf), "XYZ %.1f %.1f %.1f", pos.x, pos.y, pos.z);
    DrawText(x, y, buf, 180, 255, 180, 230); y += 14.f;
    snprintf(buf, sizeof(buf), "Mutants alive(near): %d", MutantDirector::Get().AliveMutantsNear(160.f));
    DrawText(x, y, buf, 180, 255, 180, 230); y += 14.f;
    snprintf(buf, sizeof(buf), "Mutants killed: %u", ctx.player.mutantsKilled);
    DrawText(x, y, buf, 180, 255, 180, 230); y += 14.f;
    snprintf(buf, sizeof(buf), "Artifacts found: %u", ctx.player.artifactsFound);
    DrawText(x, y, buf, 180, 255, 180, 230); y += 14.f;
    snprintf(buf, sizeof(buf), "Blowout in: %.0f sec", BlowoutSystem::Get().TimeUntilBlowout());
    DrawText(x, y, buf, 180, 255, 180, 230); y += 14.f;
    snprintf(buf, sizeof(buf), "Blocked scripts: %u", 0u);
    DrawText(x, y, buf, 180, 255, 180, 230);
}

} // namespace stalker
