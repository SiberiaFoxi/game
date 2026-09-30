#pragma once
// ============================================================================
//  Загрузчик моделей мутантов из файлов (STALKER_MOD/models/<Имя>/model.dff
//  + model.txd) без правки gta3.img.
//
//  Как это делает игра (gta-reversed):
//   * CTxdStore::AddTxdSlot + CTxdStore::LoadTxd(slot, file) — текстуры;
//   * RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, file) → CFileLoader::
//     LoadClumpFile(stream, modelId) — клумп в слот модели;
//   * CBaseModelInfo::SetTexDictionary(name) — привязка TXD к модели;
//   * CPedModelInfo::m_nAnimType = ANIM_GROUP_MAN — стандартная анимация педа.
//
//  Слоты: свободные (неиспользуемые) ped-ID игры, см. kFreePedIds.
//  Альтернатива без файлов: пользователь заменяет .dff/.txd прямо в gta3.img
//  (IMG Tool) и указывает modelId в mutants.cfg.
// ============================================================================
#include <cstdint>
#include <string>
#include <vector>

namespace stalker {

// Неиспользуемые в чистой GTA SA ped-ID (подтверждено сообществом:
// 2-6, 8, 42, 65, 74, 86, 119, 149, 208, 265-273, 289).
// Они не имеют моделей в peds.ide — ModelLoader создаст CPedModelInfo сам.
constexpr int kFreePedIds[] = {
    2, 3, 4, 5, 6, 8, 42, 65, 74, 86, 119, 149, 208,
    265, 266, 267, 268, 269, 270, 271, 272, 273, 289
};
constexpr int kFreePedIdsCount = sizeof(kFreePedIds) / sizeof(kFreePedIds[0]);

struct ModelBinding {
    std::string name;      // имя мутанта = имя папки
    int32_t     modelSlot = -1;
    int32_t     txdSlot = -1;
    enum State { PENDING, REQUESTING, REPLACING, DONE, FAILED } state = PENDING;
    float       timer = 0.f;
};

class ModelLoader {
public:
    static ModelLoader& Get();

    // Зарегистрировать мутанта с файловой моделью; вернёт назначенный слот
    // (или -1, если свободные слоты кончились).
    int RegisterFileModel(const std::string& name);

    // Продвижение конечного автомата загрузки (вызывать каждый кадр).
    void Tick();

    // Повторная привязка после перезапуска сессии (новая игра / загрузка)
    void ResetForNewSession();

    bool IsReady(const std::string& name) const;
    int  SlotOf(const std::string& name) const;

private:
    ModelLoader() = default;

    bool ReplaceModelFiles(ModelBinding& b);
    bool CreatePedModelInfo(int slot);

    std::vector<ModelBinding> m_bindings;
    int m_nextFreeIndex = 0;
    bool m_sessionBound = false;
};

} // namespace stalker
