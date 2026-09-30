#pragma once
// ============================================================================
//  Аномалии: гравитационная («Воронка»), электро («Электра»), термо («Жарка»),
//  химическая («Кисляк»). Поля аномалий генерируются детерминированно из
//  zones.cfg ([anomaly_field:...]). Детекторы «Отклик» и «Велес».
//  Артефакты появляются рядом с аномалиями и возрождаются со временем.
// ============================================================================
#include <cstdint>
#include <string>
#include <vector>
#include "../Gta/GtaTypes.h"

namespace stalker {

enum AnomalyType : uint8_t {
    ANOMALY_GRAVITY = 0, // «Воронка» — швыряет вверх
    ANOMALY_ELECTRO,     // «Электра» — разряд, высокий урон
    ANOMALY_THERMAL,     // «Жарка» — огонь
    ANOMALY_CHEMICAL,    // «Кисляк» — радиация

    ANOMALY_COUNT
};

const char* AnomalyTypeName(AnomalyType t);

struct Anomaly {
    gta::CVector pos;
    AnomalyType type = ANOMALY_GRAVITY;
    float radius = 2.5f;      // радиус действия
    float visualSize = 1.6f;  // размер маркера
    float cooldownUntil = 0.f;// «перезарядка» после срабатывания
};

struct ArtifactSpawn {
    gta::CVector pos;
    int32_t itemDefIndex = -1;
    float respawnAt = 0.f;    // время (сек сессии), когда артефакт снова доступен
    bool present = true;
};

struct AnomalyField {
    std::string name;
    gta::CVector center;
    float radius = 40.f;
    int   count = 5;
    uint8_t allowedTypes = 0xF; // битовая маска AnomalyType
    float artifactChance = 0.7f;
    float artifactRespawnSec = 300.f;

    std::vector<Anomaly> anomalies;
    std::vector<ArtifactSpawn> artifacts;
    bool generated = false;
};

class AnomalySystem {
public:
    static AnomalySystem& Get();

    void LoadFields();          // из cfgZones (адреса полей — детерминированно)
    void Tick(float dt);
    void DrawMarkers();         // чекпоинты-маркеры (в фазе рендера)

    // Радиационное поле от химических аномалий (для RadiationSystem)
    float RadFieldAt(const gta::CVector& pos) const;

    // Ближайшая аномалия в радиусе (для детектора)
    const Anomaly* NearestAnomaly(const gta::CVector& pos, float maxDist, float* outDist) const;
    const ArtifactSpawn* NearestArtifact(const gta::CVector& pos, float maxDist, float* outDist) const;

    // Подбор артефакта игроком (возвращает индекс предмета или -1)
    int32_t TryPickupArtifact(const gta::CVector& pos);

    // Выброс: форс-мажорная генерация артефактов в полях
    void BlowoutArtifactBurst();

    void OnSessionStart();
    void OnSessionEnd();

    // Сохранение/восстановление состояния артефактов
    void GetArtifactState(std::vector<float>& outTimers) const;
    void SetArtifactState(const std::vector<float>& timers);

private:
    AnomalySystem() = default;

    void GenerateField(AnomalyField& f);
    void ApplyDamage(Anomaly& a, float dt);

    std::vector<AnomalyField> m_fields;

    // Баланс
    float m_gravDamage = 30.f;
    float m_electroDamage = 45.f;
    float m_thermalDamage = 12.f; // в сек
    float m_chemRad = 1.2f;       // вклад в радиацию
    float m_detectorRange = 15.f;
};

} // namespace stalker
