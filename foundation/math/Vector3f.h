#pragma once

#include <cmath>
#include <string>

namespace VoxelEditor {
namespace Math {

class Vector3f {
public:
    float x, y, z;

    Vector3f() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3f(float x, float y, float z) : x(x), y(y), z(z) {}
    Vector3f(float value) : x(value), y(value), z(value) {}

    Vector3f operator+(const Vector3f& other) const {
        return Vector3f(x + other.x, y + other.y, z + other.z);
    }

    Vector3f operator-(const Vector3f& other) const {
        return Vector3f(x - other.x, y - other.y, z - other.z);
    }

    Vector3f operator*(float scalar) const {
        return Vector3f(x * scalar, y * scalar, z * scalar);
    }

    Vector3f operator/(float scalar) const {
        float inv = 1.0f / scalar;
        return Vector3f(x * inv, y * inv, z * inv);
    }

    Vector3f operator-() const {
        return Vector3f(-x, -y, -z);
    }

    Vector3f& operator+=(const Vector3f& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vector3f& operator-=(const Vector3f& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    Vector3f& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    Vector3f& operator/=(float scalar) {
        float inv = 1.0f / scalar;
        x *= inv;
        y *= inv;
        z *= inv;
        return *this;
    }

    bool operator==(const Vector3f& other) const {
        const float epsilon = 1e-6f;
        return std::abs(x - other.x) < epsilon &&
               std::abs(y - other.y) < epsilon &&
               std::abs(z - other.z) < epsilon;
    }

    bool operator!=(const Vector3f& other) const {
        return !(*this == other);
    }

    float operator[](int index) const {
        return (&x)[index];
    }

    float& operator[](int index) {
        return (&x)[index];
    }

    float dot(const Vector3f& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    Vector3f cross(const Vector3f& other) const {
        return Vector3f(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    float length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    float lengthSquared() const {
        return x * x + y * y + z * z;
    }

    Vector3f normalized() const {
        float len = length();
        if (len > 1e-8f) {
            return *this / len;
        }
        return Vector3f(1.0f, 0.0f, 0.0f);
    }

    void normalize() {
        float len = length();
        if (len > 1e-8f) {
            *this /= len;
        } else {
            x = 1.0f;
            y = 0.0f;
            z = 0.0f;
        }
    }

    static Vector3f Zero() {
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    static Vector3f One() {
        return Vector3f(1.0f, 1.0f, 1.0f);
    }

    static Vector3f UnitX() {
        return Vector3f(1.0f, 0.0f, 0.0f);
    }

    static Vector3f UnitY() {
        return Vector3f(0.0f, 1.0f, 0.0f);
    }

    static Vector3f UnitZ() {
        return Vector3f(0.0f, 0.0f, 1.0f);
    }

    static float distance(const Vector3f& a, const Vector3f& b) {
        return (b - a).length();
    }

    static float distanceSquared(const Vector3f& a, const Vector3f& b) {
        return (b - a).lengthSquared();
    }

    static Vector3f lerp(const Vector3f& a, const Vector3f& b, float t) {
        return a + (b - a) * t;
    }

    static Vector3f min(const Vector3f& a, const Vector3f& b) {
        return Vector3f(
            std::min(a.x, b.x),
            std::min(a.y, b.y),
            std::min(a.z, b.z)
        );
    }

    static Vector3f max(const Vector3f& a, const Vector3f& b) {
        return Vector3f(
            std::max(a.x, b.x),
            std::max(a.y, b.y),
            std::max(a.z, b.z)
        );
    }

    static Vector3f clamp(const Vector3f& value, const Vector3f& min, const Vector3f& max) {
        return Vector3f(
            std::clamp(value.x, min.x, max.x),
            std::clamp(value.y, min.y, max.y),
            std::clamp(value.z, min.z, max.z)
        );
    }

    std::string toString() const {
        return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
    }
};

inline Vector3f operator*(float scalar, const Vector3f& vec) {
    return vec * scalar;
}

}
}