#pragma once

#include <random>

class Random
{
public:
    // Get singleton instance
    static Random& instance()
    {
        static Random r;
        return r;
    }

    // Random int in range [min, max] inclusive
    int nextInt (int min, int max)
    {
        std::uniform_int_distribution<int> dist (min, max);
        return dist (engine);
    }

    // Random int in range [0, max) exclusive
    int nextInt (int max)
    {
        std::uniform_int_distribution<int> dist (0, max - 1);
        return dist (engine);
    }

    // Random float in range [0, 1)
    float nextFloat()
    {
        std::uniform_real_distribution<float> dist (0.0f, 1.0f);
        return dist (engine);
    }

    // Random float in range [min, max)
    float nextFloat (float min, float max)
    {
        std::uniform_real_distribution<float> dist (min, max);
        return dist (engine);
    }

private:
    Random() : engine (std::random_device{}()) {}

    std::mt19937 engine;
};

// Convenience functions
inline int randomInt (int min, int max) { return Random::instance().nextInt (min, max); }
inline int randomInt (int max) { return Random::instance().nextInt (max); }
inline float randomFloat() { return Random::instance().nextFloat(); }
inline float randomFloat (float min, float max) { return Random::instance().nextFloat (min, max); }
