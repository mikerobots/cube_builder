#pragma once

#include <cmath>
#include <algorithm>

namespace VoxelEditor {
namespace Math {

// Constants
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = PI * 0.5f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

// Angle conversions
constexpr float toRadians(float degrees) {
    return degrees * DEG_TO_RAD;
}

constexpr float toDegrees(float radians) {
    return radians * RAD_TO_DEG;
}

// Clamping
template<typename T>
constexpr T clamp(T value, T min, T max) {
    return std::max(min, std::min(value, max));
}

// Linear interpolation
template<typename T>
constexpr T lerp(T a, T b, float t) {
    return a + (b - a) * t;
}

// Smooth step function
constexpr float smoothstep(float edge0, float edge1, float x) {
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// Check if floating point number is approximately zero
constexpr bool isZero(float value, float epsilon = 1e-6f) {
    return std::abs(value) < epsilon;
}

// Check if two floating point numbers are approximately equal
constexpr bool isEqual(float a, float b, float epsilon = 1e-6f) {
    return std::abs(a - b) < epsilon;
}

// Sign function
template<typename T>
constexpr T sign(T value) {
    return (T(0) < value) - (value < T(0));
}

// Square function
template<typename T>
constexpr T square(T value) {
    return value * value;
}

// Wrap angle to [0, 2π) range
constexpr float wrapAngle(float angle) {
    while (angle < 0.0f) angle += TWO_PI;
    while (angle >= TWO_PI) angle -= TWO_PI;
    return angle;
}

// Wrap angle to [-π, π) range
constexpr float wrapAngleSigned(float angle) {
    while (angle < -PI) angle += TWO_PI;
    while (angle >= PI) angle -= TWO_PI;
    return angle;
}

// Distance between two angles (shortest path)
constexpr float angleDistance(float a, float b) {
    float diff = b - a;
    return wrapAngleSigned(diff);
}

// Power of 2 utilities
constexpr bool isPowerOfTwo(int value) {
    return value > 0 && (value & (value - 1)) == 0;
}

constexpr int nextPowerOfTwo(int value) {
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    return value + 1;
}

// Random number generation helpers
class Random {
public:
    static float range(float min, float max) {
        return min + (max - min) * (std::rand() / static_cast<float>(RAND_MAX));
    }
    
    static int range(int min, int max) {
        return min + std::rand() % (max - min + 1);
    }
    
    static bool chance(float probability) {
        return range(0.0f, 1.0f) < probability;
    }
};

}
}