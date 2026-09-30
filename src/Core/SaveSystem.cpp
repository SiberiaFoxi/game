#include "SaveSystem.h"
#include "Log.h"
#include "ModState.h"
#include "../Systems/AnomalySystem.h"

#include <cstdio>
#include <cstring>

namespace stalker {

SaveSystem& SaveSystem::Get() {
    static SaveSystem inst;
    return inst;
}

void SaveSystem::Save() {
    if (!m_path) return;
    ModContext& ctx = ModContext::Get();
    PlayerState& p = ctx.player;

    FILE* f = fopen(m_path, "wb");
    if (!f) {
        Log::Error("SaveSystem: не удалось открыть %s для записи", m_path);
        return;
    }

    fwrite(&kMagic, 4, 1, f);
    fwrite(&kVersion, 4, 1, f);

    // Радиация / противогаз / статистика
    fwrite(&p.radDose, 4, 1, f);
    fwrite(&p.maskEquipped, 1, 1, f);
    fwrite(&p.filterLeft, 4, 1, f);
    fwrite(&p.mutantsKilled, 4, 1, f);
    fwrite(&p.artifactsFound, 4, 1, f);

    // Экипировка
    int32_t eqMask = p.equippedMask, eqDet = p.equippedDetector;
    fwrite(&eqMask, 4, 1, f);
    fwrite(&eqDet, 4, 1, f);
    uint32_t artCount = (uint32_t)p.equippedArtifacts.size();
    fwrite(&artCount, 4, 1, f);
    for (int32_t a : p.equippedArtifacts) fwrite(&a, 4, 1, f);

    // Инвентарь
    uint32_t n = (uint32_t)p.inventory.size();
    fwrite(&n, 4, 1, f);
    for (auto& st : p.inventory) {
        int32_t idx = st.defIndex;
        uint16_t cnt = st.count;
        fwrite(&idx, 4, 1, f);
        fwrite(&cnt, 2, 1, f);
    }

    // Состояние артефактов в мире
    std::vector<float> artState;
    AnomalySystem::Get().GetArtifactState(artState);
    uint32_t an = (uint32_t)artState.size();
    fwrite(&an, 4, 1, f);
    if (an) fwrite(artState.data(), 4, an, f);

    fclose(f);
    m_timeSinceSave = 0.f;
    Log::Write("SaveSystem: сохранено (%s)", m_path);
}

bool SaveSystem::Load() {
    FILE* f = fopen(m_path, "rb");
    if (!f) return false;

    uint32_t magic = 0, version = 0;
    if (fread(&magic, 4, 1, f) != 1 || magic != kMagic) { fclose(f); return false; }
    if (fread(&version, 4, 1, f) != 1 || version != kVersion) { fclose(f); return false; }

    ModContext& ctx = ModContext::Get();
    PlayerState& p = ctx.player;

    if (fread(&p.radDose, 4, 1, f) != 1) { fclose(f); return false; }
    if (fread(&p.maskEquipped, 1, 1, f) != 1) { fclose(f); return false; }
    if (fread(&p.filterLeft, 4, 1, f) != 1) { fclose(f); return false; }
    if (fread(&p.mutantsKilled, 4, 1, f) != 1) { fclose(f); return false; }
    if (fread(&p.artifactsFound, 4, 1, f) != 1) { fclose(f); return false; }

    int32_t eqMask = -1, eqDet = -1;
    if (fread(&eqMask, 4, 1, f) != 1) { fclose(f); return false; }
    if (fread(&eqDet, 4, 1, f) != 1) { fclose(f); return false; }
    p.equippedMask = eqMask;
    p.equippedDetector = eqDet;

    uint32_t artCount = 0;
    if (fread(&artCount, 4, 1, f) != 1 || artCount > 16) { fclose(f); return false; }
    p.equippedArtifacts.clear();
    for (uint32_t i = 0; i < artCount; ++i) {
        int32_t a = -1;
        if (fread(&a, 4, 1, f) != 1) { fclose(f); return false; }
        p.equippedArtifacts.push_back(a);
    }

    uint32_t n = 0;
    if (fread(&n, 4, 1, f) != 1 || n > 256) { fclose(f); return false; }
    p.inventory.clear();
    for (uint32_t i = 0; i < n; ++i) {
        int32_t idx = -1;
        uint16_t cnt = 0;
        if (fread(&idx, 4, 1, f) != 1) { fclose(f); return false; }
        if (fread(&cnt, 2, 1, f) != 1) { fclose(f); return false; }
        if (idx >= 0 && idx < (int)ctx.items.size())
            p.inventory.push_back({ idx, cnt, false });
    }

    uint32_t an = 0;
    if (fread(&an, 4, 1, f) == 1 && an <= 4096) {
        std::vector<float> artState(an);
        if (an && fread(artState.data(), 4, an, f) == an) {
            // Артефакты в мире восстановим после генерации полей
            AnomalySystem::Get().SetArtifactState(artState);
        }
    }

    fclose(f);
    RecalcEquipmentEffects();
    Log::Write("SaveSystem: загружено (%s)", m_path);
    return true;
}

void SaveSystem::TickAutosave(float dt) {
    m_timeSinceSave += dt;
    if (m_timeSinceSave >= m_autosaveInterval) {
        Save();
    }
}

} // namespace stalker
