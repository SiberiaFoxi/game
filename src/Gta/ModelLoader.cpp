#include "ModelLoader.h"
#include <cstring>
#include "../Core/Log.h"
#include "../Core/ModState.h"
#include "../Gta/GtaFunctions.h"

namespace stalker {

ModelLoader& ModelLoader::Get() {
    static ModelLoader inst;
    return inst;
}

int ModelLoader::RegisterFileModel(const std::string& name) {
    for (auto& b : m_bindings)
        if (b.name == name) return b.modelSlot;

    if (m_nextFreeIndex >= kFreePedIdsCount) {
        Log::Error("ModelLoader: нет свободных ped-слотов для '%s' "
                   "(доступно %d). Используйте замену в gta3.img и modelId=... в mutants.cfg",
                   name.c_str(), kFreePedIdsCount);
        return -1;
    }

    ModelBinding b;
    b.name = name;
    b.modelSlot = kFreePedIds[m_nextFreeIndex++];
    b.state = ModelBinding::PENDING;
    m_bindings.push_back(b);

    Log::Write("ModelLoader: '%s' назначен ped-слот %d", name.c_str(), b.modelSlot);
    return b.modelSlot;
}

void ModelLoader::ResetForNewSession() {
    for (auto& b : m_bindings) {
        if (b.state == ModelBinding::DONE || b.state == ModelBinding::REPLACING)
            b.state = ModelBinding::PENDING; // модельные инфо пересоздаются игрой
        b.timer = 0.f;
    }
    m_sessionBound = false;
}

void ModelLoader::Tick() {
    using namespace gta;

    // Ждём активной сессии (массивы ModelInfo инициализированы)
    if (!IsGameSessionActive()) return;

    for (auto& b : m_bindings) {
        switch (b.state) {
        case ModelBinding::PENDING:
            // Запрашиваем оригинальную модель слота (если есть), чтобы
            // стриминг считал её «загруженной» и создание педа не падало.
            if (AllModelInfos()[b.modelSlot] != nullptr) {
                fn::StreamingRequestModel(b.modelSlot, 0x8 /*STREAMING_KEEP_IN_MEMORY*/);
                b.state = ModelBinding::REQUESTING;
                b.timer = 0.f;
            } else {
                // Пустой слот — стриминг не нужен, создаём инфо сразу
                if (CreatePedModelInfo(b.modelSlot)) {
                    b.state = ModelBinding::REPLACING;
                } else {
                    b.state = ModelBinding::FAILED;
                }
            }
            break;

        case ModelBinding::REQUESTING:
            b.timer += 0.05f;
            if (IsModelLoaded(b.modelSlot)) {
                b.state = ModelBinding::REPLACING;
            } else if (b.timer > 30.f) {
                Log::Error("ModelLoader: таймаут загрузки модели %d ('%s')",
                           b.modelSlot, b.name.c_str());
                b.state = ModelBinding::FAILED;
            }
            break;

        case ModelBinding::REPLACING:
            if (ReplaceModelFiles(b)) {
                b.state = ModelBinding::DONE;
                Log::Write("ModelLoader: '%s' загружен (слот %d)", b.name.c_str(), b.modelSlot);
            } else {
                b.state = ModelBinding::FAILED;
                Log::Error("ModelLoader: не удалось загрузить файлы '%s' "
                           "(проверьте STALKER_MOD/models/%s/model.dff и model.txd)",
                           b.name.c_str(), b.name.c_str());
            }
            break;

        default:
            break;
        }
    }
}

bool ModelLoader::CreatePedModelInfo(int slot) {
    using namespace gta;
    if (AllModelInfos()[slot] != nullptr) return true;

    // Выделяем память и вызываем конструктор CPedModelInfo (0x4C57A0).
    // Внимание: используем game-хип? Нет — пед-модели создаются в global heap
    // (CModelInfo::AddModelInfo использует new). Обычный malloc тоже валиден:
    // объект живёт до конца сессии и не удаляется вручную.
    void* mem = malloc(0x44);
    if (!mem) return false;
    memset(mem, 0, 0x44);
    fn::PedModelInfoConstruct(reinterpret_cast<CBaseModelInfo*>(mem));
    AllModelInfos()[slot] = reinterpret_cast<CBaseModelInfo*>(mem);
    return true;
}

bool ModelLoader::ReplaceModelFiles(ModelBinding& b) {
    using namespace gta;

    char dffPath[600], txdName[64];
    snprintf(dffPath, sizeof(dffPath), "%s\\models\\%s\\model.dff", ModContext::Get().modDir, b.name.c_str());
    snprintf(txdName, sizeof(txdName), "stk_%s", b.name.c_str());

    // Проверяем существование файлов
    FILE* f = fopen(dffPath, "rb");
    if (!f) return false;
    fclose(f);
    char txdPath[600];
    snprintf(txdPath, sizeof(txdPath), "%s\\models\\%s\\model.txd", ModContext::Get().modDir, b.name.c_str());
    f = fopen(txdPath, "rb");
    if (!f) return false;
    fclose(f);

    // 1) TXD
    if (b.txdSlot < 0) {
        int32_t slot = fn::TxdStoreAddTxdSlot(txdName);
        if (slot < 0) return false;
        if (!fn::TxdStoreLoadTxd(slot, txdPath)) {
            fn::TxdStoreRemoveTxdSlot(slot);
            return false;
        }
        b.txdSlot = slot;
    }

    // 2) ModelInfo (создать, если слот был пустым)
    if (!CreatePedModelInfo(b.modelSlot)) return false;
    CBaseModelInfo* mi = AllModelInfos()[b.modelSlot];
    if (!mi) return false;

    // Привязываем наш словарь текстур (SetTexDictionary ищет слот по имени)
    fn::BaseModelInfoSetTexDictionary(mi, txdName);

    // 3) DFF — клумп в слот модели
    void* stream = fn::RwStreamOpen(1 /*rwSTREAMFILENAME*/, 0 /*rwSTREAMREAD*/, dffPath);
    if (!stream) return false;
    bool ok = fn::FileLoaderLoadClumpFile(stream, static_cast<uint32_t>(b.modelSlot));
    fn::RwStreamClose(stream, nullptr);
    if (!ok) return false;

    // 4) Стандартная анимация педа (модели паков сделаны под скелет педа)
    PedModelInfoAnimType(mi) = ANIM_GROUP_MAN;
    return true;
}

bool ModelLoader::IsReady(const std::string& name) const {
    for (auto& b : m_bindings)
        if (b.name == name) return b.state == ModelBinding::DONE;
    return false;
}

int ModelLoader::SlotOf(const std::string& name) const {
    for (auto& b : m_bindings)
        if (b.name == name) return b.modelSlot;
    return -1;
}

} // namespace stalker
