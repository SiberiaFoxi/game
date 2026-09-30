#pragma once
// ============================================================================
//  Режиссёр мутантов: спавн вокруг игрока (по биомам), поведение, агрессия.
//
//  Спавн — CPopulation::AddPed (0x612710), той же функцией, которой пользуется
//  скриптовая команда CREATE_CHAR (gta-reversed:
//  source/game_sa/Scripts/Commands/CharacterCommands.cpp).
//  Агрессия — движковый ИИ: маска hate (CAcquaintance) + события
//  CEventAcquaintancePedHate, как в CCheat::MayhemCheat (Ped.cpp gta-reversed).
// ============================================================================
#include <cstdint>
#include <vector>
#include "../Gta/GtaTypes.h"

namespace stalker {

class Rng;

struct SpawnedEntity {
    gta::CPed* ped = nullptr;
    int        defIndex = -1;      // индекс в MutantCatalog; -1 = человек фракции
    int        factionId = -1;
    uint32_t   spawnTime = 0;      // мс
    uint32_t   deathTime = 0;      // 0 = жив
    gta::CVector homePos;
    bool       aggro = false;
    float      nextAttackAt = 0.f; // сек (sessionTime)
};

class MutantDirector {
public:
    static MutantDirector& Get();

    void Tick(float dt);

    // Биом по координатам (из zones.cfg [zone:...], иначе FOREST)
    uint32_t BiomeAt(float x, float y) const;
    // Название зоны (для HUD)
    const char* ZoneNameAt(float x, float y) const;

    // Состояние
    int  AliveMutantsNear(float radius) const;
    void OnSessionStart();
    void OnSessionEnd();
    void PurgeAll(bool instant); // вычищать мутантов (выброс/чит)

    std::vector<SpawnedEntity>& Entities() { return m_entities; }
    uint32_t TotalMutantsKilled() const { return m_kills; }

private:
    MutantDirector() = default;

    void Think(float dt);
    void SpawnWave();
    bool TrySpawnOne(int defIndex, const gta::CVector& anchor, Rng& rng);
    void UpdateEntity(size_t idx, float dt);
    void MakeAggressive(SpawnedEntity& e);
    void CalmDown(SpawnedEntity& e);
    bool FindGroundPos(gta::CVector& pos);

    std::vector<SpawnedEntity> m_entities;
    float m_thinkTimer = 0.f;
    float m_spawnTimer = 0.f;
    size_t m_rotateCursor = 0;
    uint32_t m_kills = 0;
    bool m_sessionActive = false;

    // Настройки (из stalker_mod.cfg)
    float m_densityMul = 1.0f;
    float m_spawnMinDist = 35.f;
    float m_spawnMaxDist = 130.f;
    float m_cullDist = 220.f;
    int   m_cityTarget = 6;
    int   m_cityNightTarget = 9;
    int   m_wildTarget = 12;
    float m_corpseLifetime = 25.f;
};

} // namespace stalker
