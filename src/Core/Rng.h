#pragma once
// Детерминированный ГПСЧ (xorshift32) — одинаковые поля аномалий/артефактов
// для одного seed независимо от порядка обновления.
#include <cstdint>

namespace stalker {

class Rng {
public:
    explicit Rng(uint32_t seed = 0x1A2B3C4D) : m_state(seed ? seed : 1u) {}

    void Seed(uint32_t s) { m_state = s ? s : 1u; }

    uint32_t Next() {
        // xorshift32 (Marsaglia)
        uint32_t x = m_state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        m_state = x;
        return x;
    }

    // [0, 1)
    float NextFloat() { return static_cast<float>(Next() & 0xFFFFFF) / 16777216.f; }

    // [min, max]
    int RangeI(int min, int max) {
        if (max <= min) return min;
        return min + static_cast<int>(Next() % static_cast<uint32_t>(max - min + 1));
    }

    // [min, max)
    float RangeF(float min, float max) {
        return min + NextFloat() * (max - min);
    }

    bool Chance(float probability) { return NextFloat() < probability; }

private:
    uint32_t m_state;
};

// Стабильный хеш для строк (seed зон/аномалий)
inline uint32_t StableHash(const char* s, uint32_t seed = 2166136261u) {
    uint32_t h = seed;
    while (*s) { h ^= static_cast<uint8_t>(*s++); h *= 16777619u; }
    return h;
}

} // namespace stalker
