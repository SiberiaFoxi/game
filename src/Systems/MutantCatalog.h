#pragma once
// ============================================================================
//  Каталог мутантов S.T.A.L.K.E.R. — данные из STALKER_MOD/mutants.cfg.
//
//  Состав паков моделей (см. README, раздел «Модели»):
//   - «Огромный сборник монстров из S.T.A.L.K.E.R.» (Playground.ru),
//     авторы: Vadim_Panenko, Jagyar, TheLozza, mister_gad, =Borman=,
//     ПсевдоШрам, VIRTUS;
//   - «Мутанты из S.T.A.L.K.E.R. ПЗ Ultimate v0.5.5» (Playground.ru),
//     авторы: Vadim Panenko, Borman;
//   - «Pack Of Monsters From S.T.A.L.K.E.R.» (GTA.com.ua);
//   - «Massive Monster Collection» (LibertyCity.net);
//   - Псевдогигант, Кровосос (LibertyCity.net).
//
//  Имена секций совпадают с именами папок в STALKER_MOD/models/<имя>/.
// ============================================================================
#include <cstdint>
#include <string>
#include <vector>

namespace stalker {

// Биомы (битовая маска)
enum Biome : uint32_t {
    BIOME_CITY       = 1u << 0,
    BIOME_FOREST     = 1u << 1,
    BIOME_DESERT     = 1u << 2,
    BIOME_WATER      = 1u << 3,
    BIOME_INDUSTRIAL = 1u << 4,
    BIOME_ALL        = 0xFFFFFFFF,
};

struct MutantDef {
    std::string name;         // ID = имя папки модели
    std::string title;        // отображаемое имя (рус. текст не отображается шрифтом SA!)
    std::string folder;       // имя папки в models/ (пусто → только IMG-замена)
    int32_t     modelId = -1; // пед-слот (-1: назначит ModelLoader либо IMG-замена)
    int32_t     fallbackId = 7; // пед-слот, пока основная модель не готова

    float health       = 100.f;
    float damage       = 10.f;    // урон за атаку (прямой)
    float attackRange  = 1.3f;
    float attackCooldown = 1.2f;
    float aggroRadius  = 35.f;
    float deAggroRadius = 70.f;
    float speedFactor  = 1.0f;    // множитель скорости анимации
    float nightAggroMul = 1.35f;
    float psiDamage    = 0.f;     // урон/сек в радиусе (контролёр, бюрер)
    float psiRadius    = 0.f;

    int   minGroup = 1, maxGroup = 1;
    uint32_t biomes = BIOME_ALL;
    float weight = 1.f;           // вес в таблице спавна

    bool  territorial = true;     // держится у точки спавна
    bool  flees = false;          // убегает от игрока (тушканчик)

    // Резерв мутантов для «дикой» генерации
    float corpseMoney = 0.f;      // $ с трупа
};

class MutantCatalog {
public:
    static MutantCatalog& Get();

    // Загрузка из mutants.cfg; при отсутствии файла — встроенный минимум
    void LoadDefaults();

    const std::vector<MutantDef>& All() const { return m_defs; }
    int Count() const { return (int)m_defs.size(); }
    const MutantDef* Get(int index) const;
    int Find(const std::string& name) const;

    // Выбор случайного мутанта по биому (взвешенно)
    int PickForBiome(uint32_t biomes, float nightFactor, class Rng& rng) const;

private:
    void AddDefault(const char* name, const char* title, float hp, float dmg,
                    float aggro, float speed, uint32_t biomes, float weight,
                    int minG, int maxG, float psiDmg = 0.f, bool flees = false);

    std::vector<MutantDef> m_defs;
};

} // namespace stalker
