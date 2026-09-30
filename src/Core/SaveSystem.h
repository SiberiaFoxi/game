#pragma once
// ============================================================================
//  Сохранение прогресса мода. ВАЖНО: сейвы GTA SA не трогаем (совместимость
//  с обычными сохранениями игры). Прогресс мода — в отдельном файле
//  STALKER_MOD/stalker_save.dat:
//    автосохранение каждые 30 сек + после выброса + F9 (вручную);
//    загрузка при старте сессии (новая игра / загрузка сейва).
// ============================================================================
#include <cstdint>
#include <vector>

namespace stalker {

class SaveSystem {
public:
    static SaveSystem& Get();

    void SetPath(const char* path) { m_path = path; }

    void Save();
    bool Load();

    void TickAutosave(float dt);

    float LastSaveAgo() const { return m_timeSinceSave; }

private:
    SaveSystem() = default;

    static constexpr uint32_t kMagic = 0x53544B52; // 'STKR'
    static constexpr uint32_t kVersion = 1;

    const char* m_path = "STALKER_MOD\\stalker_save.dat";
    float m_timeSinceSave = 0.f;
    float m_autosaveInterval = 30.f;
};

} // namespace stalker
