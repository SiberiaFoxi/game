#include "MutantCatalog.h"
#include "../Core/Config.h"
#include "../Core/Log.h"
#include "../Core/ModState.h"
#include "../Core/Rng.h"
#include "../Gta/ModelLoader.h"

#include <algorithm>

namespace stalker {

MutantCatalog& MutantCatalog::Get() {
    static MutantCatalog inst;
    return inst;
}

const MutantDef* MutantCatalog::Get(int index) const {
    if (index < 0 || index >= (int)m_defs.size()) return nullptr;
    return &m_defs[index];
}

int MutantCatalog::Find(const std::string& name) const {
    for (size_t i = 0; i < m_defs.size(); ++i)
        if (m_defs[i].name == name) return (int)i;
    return -1;
}

void MutantCatalog::AddDefault(const char* name, const char* title, float hp, float dmg,
                               float aggro, float speed, uint32_t biomes, float weight,
                               int minG, int maxG, float psiDmg, bool flees) {
    MutantDef d;
    d.name = name;
    d.title = title;
    d.folder = name;
    d.modelId = -1;
    d.fallbackId = 7; // "male01" — стандартный пед, всегда загружен
    d.health = hp;
    d.damage = dmg;
    d.aggroRadius = aggro;
    d.deAggroRadius = aggro * 2.2f;
    d.speedFactor = speed;
    d.biomes = biomes;
    d.weight = weight;
    d.minGroup = minG;
    d.maxGroup = maxG;
    d.psiDamage = psiDmg;
    d.psiRadius = psiDmg > 0.f ? 12.f : 0.f;
    d.flees = flees;
    d.territorial = !flees;
    d.corpseMoney = hp * 0.05f;
    m_defs.push_back(d);
}

void MutantCatalog::LoadDefaults() {
    m_defs.clear();
    ModContext& ctx = ModContext::Get();

    // ---- из конфига (если есть) ----
    if (ctx.cfgMutants) {
        auto sections = ctx.cfgMutants->GetSectionsWithPrefix("mutant:");
        for (auto& s : sections) {
            Config& c = *ctx.cfgMutants;
            std::string name = s.substr(7);
            MutantDef d;
            d.name    = name;
            d.title   = c.GetString(s, "title", name.c_str());
            d.folder  = c.GetString(s, "folder", name.c_str());
            d.modelId = c.GetInt(s, "modelid", -1);
            d.fallbackId = c.GetInt(s, "fallback_id", 7);
            d.health  = c.GetFloat(s, "health", 100.f);
            d.damage  = c.GetFloat(s, "damage", 10.f);
            d.attackRange = c.GetFloat(s, "attack_range", 1.3f);
            d.attackCooldown = c.GetFloat(s, "attack_cooldown", 1.2f);
            d.aggroRadius = c.GetFloat(s, "aggro_radius", 35.f);
            d.deAggroRadius = c.GetFloat(s, "deaggro_radius", d.aggroRadius * 2.2f);
            d.speedFactor = c.GetFloat(s, "speed_factor", 1.f);
            d.nightAggroMul = c.GetFloat(s, "night_aggro_mul", 1.35f);
            d.psiDamage = c.GetFloat(s, "psi_damage", 0.f);
            d.psiRadius = c.GetFloat(s, "psi_radius", d.psiDamage > 0.f ? 12.f : 0.f);
            d.minGroup = c.GetInt(s, "group_min", 1);
            d.maxGroup = c.GetInt(s, "group_max", 1);
            d.weight = c.GetFloat(s, "weight", 1.f);
            d.territorial = c.GetBool(s, "territorial", true);
            d.flees = c.GetBool(s, "flees", false);
            d.corpseMoney = c.GetFloat(s, "corpse_money", d.health * 0.05f);
            d.biomes = 0;
            if (c.GetBool(s, "in_city", true))       d.biomes |= BIOME_CITY;
            if (c.GetBool(s, "in_forest", true))     d.biomes |= BIOME_FOREST;
            if (c.GetBool(s, "in_desert", true))     d.biomes |= BIOME_DESERT;
            if (c.GetBool(s, "in_water", false))     d.biomes |= BIOME_WATER;
            if (c.GetBool(s, "in_industrial", true)) d.biomes |= BIOME_INDUSTRIAL;
            if (d.biomes == 0) d.biomes = BIOME_ALL;
            m_defs.push_back(d);
        }
    }

    // ---- встроенный минимум, если конфига нет ----
    if (m_defs.empty()) {
        Log::Write("MutantCatalog: mutants.cfg не найден — использую встроенный набор");

        // Слепые псы (5 видов)
        AddDefault("blind_dog_1", "Blind dog",        40.f, 8.f, 40.f, 1.25f, BIOME_ALL, 10.f, 3, 5);
        AddDefault("blind_dog_2", "Blind dog (gray)", 45.f, 9.f, 40.f, 1.25f, BIOME_ALL, 8.f, 3, 5);
        AddDefault("blind_dog_3", "Blind dog (dark)", 50.f, 10.f, 42.f, 1.25f, BIOME_ALL, 7.f, 3, 5);
        AddDefault("blind_dog_4", "Blind dog (white)",55.f, 11.f, 42.f, 1.25f, BIOME_FOREST | BIOME_DESERT, 5.f, 3, 4);
        AddDefault("blind_dog_5", "Blind dog (alpha)",65.f, 13.f, 45.f, 1.3f, BIOME_FOREST, 3.f, 2, 4);
        // Псевдопёс, пси-собака, кот-баюн
        AddDefault("pseudo_dog", "Pseudodog",  90.f, 14.f, 45.f, 1.2f, BIOME_FOREST | BIOME_DESERT | BIOME_INDUSTRIAL, 6.f, 2, 3);
        AddDefault("psi_dog",    "Psi-dog",    80.f, 12.f, 40.f, 1.2f, BIOME_FOREST | BIOME_INDUSTRIAL, 4.f, 2, 3, 2.f);
        AddDefault("bayun_cat",  "Bayun cat",  60.f, 10.f, 38.f, 1.15f, BIOME_CITY | BIOME_FOREST, 3.f, 1, 2);
        // Кабаны, плоть
        AddDefault("boar",           "Boar",          200.f, 22.f, 35.f, 1.1f, BIOME_FOREST | BIOME_WATER, 6.f, 1, 3);
        AddDefault("boar_veteran",   "Mature boar",   350.f, 30.f, 38.f, 1.1f, BIOME_FOREST | BIOME_WATER, 3.f, 1, 2);
        AddDefault("flesh",          "Flesh",         150.f, 15.f, 30.f, 1.0f, BIOME_FOREST | BIOME_WATER, 6.f, 1, 3);
        AddDefault("flesh_veteran",  "Mature flesh",  250.f, 20.f, 32.f, 1.0f, BIOME_FOREST | BIOME_WATER, 3.f, 1, 2);
        // Кровососы (5 видов)
        AddDefault("bloodsucker",         "Bloodsucker",       260.f, 35.f, 45.f, 1.15f, BIOME_FOREST | BIOME_WATER | BIOME_INDUSTRIAL, 3.f, 1, 1);
        AddDefault("bloodsucker_veteran", "Mature bloodsucker",400.f, 45.f, 48.f, 1.15f, BIOME_FOREST | BIOME_WATER, 1.5f, 1, 1);
        AddDefault("bloodsucker_winter",  "Winter bloodsucker",300.f, 38.f, 46.f, 1.15f, BIOME_DESERT | BIOME_FOREST, 1.5f, 1, 1);
        AddDefault("bloodsucker_swamp",   "Swamp thing",       320.f, 40.f, 44.f, 1.1f, BIOME_WATER, 1.5f, 1, 1);
        AddDefault("bloodsucker_special", "Special bloodsucker",500.f, 55.f, 50.f, 1.2f, BIOME_INDUSTRIAL, 0.8f, 1, 1);
        // Полтергейст (2), излом (2), снорк
        AddDefault("poltergeist",      "Poltergeist",           200.f, 20.f, 40.f, 1.0f, BIOME_INDUSTRIAL | BIOME_CITY, 1.2f, 1, 1, 4.f);
        AddDefault("poltergeist_flame","Flame poltergeist",    220.f, 25.f, 40.f, 1.0f, BIOME_INDUSTRIAL, 0.8f, 1, 1, 4.f);
        AddDefault("fracture",    "Fracture",       180.f, 40.f, 38.f, 1.1f, BIOME_CITY | BIOME_INDUSTRIAL, 2.f, 1, 2);
        AddDefault("fracture_2",  "Fracture (old)", 220.f, 45.f, 38.f, 1.1f, BIOME_CITY | BIOME_INDUSTRIAL, 1.f, 1, 1);
        AddDefault("snork",       "Snork",          130.f, 25.f, 42.f, 1.35f, BIOME_INDUSTRIAL | BIOME_DESERT, 2.5f, 1, 2);
        // Контроллёры (2)
        AddDefault("controller",   "Controller",        220.f, 20.f, 50.f, 1.0f, BIOME_INDUSTRIAL | BIOME_CITY, 1.f, 1, 1, 3.f);
        AddDefault("controller_2", "Controller (deep)", 280.f, 22.f, 50.f, 1.0f, BIOME_INDUSTRIAL, 0.6f, 1, 1, 3.5f);
        // Гражданские зомби (10)
        for (int i = 1; i <= 10; ++i) {
            char name[32], title[32];
            snprintf(name, sizeof(name), "zombie_civ_%d", i);
            snprintf(title, sizeof(title), "Civilian zombie %d", i);
            AddDefault(name, title, 80.f, 8.f, 30.f, 0.55f, BIOME_CITY, 9.f, 1, 3);
        }
        // Тушканчик
        AddDefault("tushkano", "Tushkano", 15.f, 2.f, 25.f, 1.4f, BIOME_DESERT | BIOME_CITY | BIOME_INDUSTRIAL, 6.f, 1, 3, 0.f, true);
        // Псевдогигант (10 вариаций)
        for (int i = 1; i <= 10; ++i) {
            char name[32], title[32];
            snprintf(name, sizeof(name), "pseudogiant_%d", i);
            snprintf(title, sizeof(title), "Pseudogiant %d", i);
            AddDefault(name, title, 900.f, 70.f, 45.f, 0.85f, BIOME_DESERT | BIOME_INDUSTRIAL | BIOME_FOREST, 0.4f, 1, 1);
        }
        // Бюрер (15 вариаций)
        for (int i = 1; i <= 15; ++i) {
            char name[32], title[32];
            snprintf(name, sizeof(name), "burer_%d", i);
            snprintf(title, sizeof(title), "Burer %d", i);
            AddDefault(name, title, 300.f, 25.f, 40.f, 0.9f, BIOME_INDUSTRIAL | BIOME_CITY, 0.6f, 1, 1, 5.f);
        }
        // Химера, карлик
        AddDefault("chimera", "Chimera", 500.f, 50.f, 50.f, 1.5f, BIOME_FOREST | BIOME_DESERT, 0.35f, 1, 1);
        AddDefault("karlik",  "Karlik (dwarf)", 150.f, 20.f, 35.f, 0.95f, BIOME_INDUSTRIAL, 0.8f, 1, 2);
    }

    // ---- регистрация файловых моделей (только если папка с файлами есть) ----
    auto fileExists = [](const char* path) {
        FILE* f = fopen(path, "rb");
        if (!f) return false;
        fclose(f);
        return true;
    };
    for (auto& d : m_defs) {
        if (d.folder.empty()) continue;
        char path[600];
        snprintf(path, sizeof(path), "%s\\models\\%s\\model.dff", ctx.modDir, d.folder.c_str());
        if (!fileExists(path)) continue;
        snprintf(path, sizeof(path), "%s\\models\\%s\\model.txd", ctx.modDir, d.folder.c_str());
        if (!fileExists(path)) continue;
        int slot = ModelLoader::Get().RegisterFileModel(d.folder);
        if (slot >= 0 && d.modelId < 0)
            d.modelId = slot;
    }

    Log::Write("MutantCatalog: загружено %d мутантов", (int)m_defs.size());
}

int MutantCatalog::PickForBiome(uint32_t biomes, float nightFactor, Rng& rng) const {
    // Взвешенный выбор среди подходящих
    struct Cand { int idx; float w; };
    std::vector<Cand> cands;
    float total = 0.f;
    for (size_t i = 0; i < m_defs.size(); ++i) {
        const MutantDef& d = m_defs[i];
        if ((d.biomes & biomes) == 0) continue;
        if (d.modelId < 0) continue; // модель недоступна
        float w = d.weight;
        if (nightFactor > 0.f && (d.biomes & BIOME_CITY)) w *= 1.3f; // ночью в городе хуже
        cands.push_back({ (int)i, w });
        total += w;
    }
    if (cands.empty() || total <= 0.f) return -1;
    float pick = rng.NextFloat() * total;
    for (auto& c : cands) {
        pick -= c.w;
        if (pick <= 0.f) return c.idx;
    }
    return cands.back().idx;
}

} // namespace stalker
