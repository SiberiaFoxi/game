#pragma once
// ============================================================================
//  Контроль населения Зоны:
//   * людей в 10-15% от исходного (PedDensityMultiplier + наши лимиты);
//   * мутанты — остальные 85-90% (см. MutantDirector);
//   * редкие NPC фракций: Сталкеры, Бандиты, Военные, Монолит, Свобода, Долг;
//   * запрет случайных копов и банд.
// ============================================================================
#include <cstdint>
#include <string>
#include <vector>

namespace stalker {

struct FactionDef {
    std::string name;
    std::string title;
    int32_t pedModelId = 7;    // пед-слот (обычные педы SA как «скины»)
    int32_t weaponId = 0;      // eWeaponType
    bool   hostileToPlayer = false;
    bool   trader = false;     // с ним можно торговать
    float  spawnWeight = 1.f;
};

class PopulationControl {
public:
    static PopulationControl& Get();

    void LoadFactions();   // из cfgFactions
    void Tick(float dt);

    void OnSessionStart();

    const std::vector<FactionDef>& Factions() const { return m_factions; }

private:
    PopulationControl() = default;

    void MaintainHumans();
    int  HumansAlive() const;

    std::vector<FactionDef> m_factions;

    float m_density = 0.12f;      // PedDensityMultiplier
    int   m_maxHumans = 2;        // наши люди вокруг игрока
    float m_spawnTimer = 0.f;
    float m_thinkTimer = 0.f;
    bool  m_noAmbientCops = true;
    bool  m_noAmbientGangs = true;
};

} // namespace stalker
