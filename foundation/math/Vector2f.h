#pragma once

#include <cmath>
#include <string>

namespace VoxelEditor {
namespace Math {

class Vector2f {
public:
    float x, y;

    Vector2f() : x(0.0f), y(0.0f) {}
    Vector2f(float x, float y) : x(x), y(y) {}
    Vector2f(float value) : x(value), y(value) {}

    Vector2f operator+(const Vector2f& other) const {
        return Vector2f(x + other.x, y + other.y);
    }

    Vector2f operator-(const Vector2f& other) const {
        return Vector2f(x - other.x, y - other.y);
    }

    Vector2f operator*(float scalar) const {
        return Vector2f(x * scalar, y * scalar);
    }

    Vector2f operator/(float scalar) const {
        float inv = 1.0f / scalar;
        return Vector2f(x * inv, y * inv);
    }

    Vector2f operator-() const {
        return Vector2f(-x, -y);
    }

    Vector2f& operator+=(const Vector2f& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vector2f& operator-=(const Vector2f& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vector2f& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    Vector2f& operator/=(float scalar) {
        float inv = 1.0f / scalar;
        x *= inv;
        y *= inv;
        return *this;
    }

    bool operator==(const Vector2f& other) const {
        const float epsilon = 1e-6f;
        return std::abs(x - other.x) < epsilon &&
               std::abs(y - other.y) < epsilon;
    }

    bool operator!=(const Vector2f& other) const {
        return !(*this == other);
    }

    float operator[](int index) const {
        return (&x)[index];
    }

    float& operator[](int index) {
        return (&x)[index];
    }

    float dot(const Vector2f& other) const {
        return x * other.x + y * other.y;
    }

    float cross(const Vector2f& other) const {
        return x * other.y - y * other.x;
    }

    float length() const {
        return std::sqrt(x * x + y * y);
    }

    float lengthSquared() const {
        return x * x + y * y;
    }

    Vector2f normalized() const {
        float len = length();
        if (len > 1e-8f) {
            return *this / len;
        }
        return Vector2f(1.0f, 0.0f);
    }

    void normalize() {
        float len = length();
        if (len > 1e-8f) {
            *this /= len;
        } else {
            x = 1.0f;
            y = 0.0f;
        }
    }

    Vector2f perpendicular() const {
        return Vector2f(-y, x);
    }

    float angle() const {
        return std::atan2(y, x);
    }

    static Vector2f Zero() {
        return Vector2f(0.0f, 0.0f);
    }

    static Vector2f One() {
        return Vector2f(1.0f, 1.0f);
    }

    static Vector2f UnitX() {
        return Vector2f(1.0f, 0.0f);
    }

    static Vector2f UnitY() {
        return Vector2f(0.0f, 1.0f);
    }

    static float distance(const Vector2f& a, const Vector2f& b) {
        return (b - a).length();
    }

    static float distanceSquared(const Vector2f& a, const Vector2f& b) {
        return (b - a).lengthSquared();
    }

    static Vector2f lerp(const Vector2f& a, const Vector2f& b, float t) {
        return a + (b - a) * t;
    }

    static Vector2f min(const Vector2f& a, const Vector2f& b) {
        return Vector2f(
            std::min(a.x, b.x),
            std::min(a.y, b.y)
        );
    }

    static Vector2f max(const Vector2f& a, const Vector2f& b) {
        return Vector2f(
            std::max(a.x, b.x),
            std::max(a.y, b.y)
        );
    }

    static Vector2f clamp(const Vector2f& value, const Vector2f& min, const Vector2f& max) {
        return Vector2f(
            std::clamp(value.x, min.x, max.x),
            std::clamp(value.y, min.y, max.y)
        );
    }

    static Vector2f fromAngle(float angle) {
        return Vector2f(std::cos(angle), std::sin(angle));
    }

    std::string toString() const {
        return "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
    }
};

inline Vector2f operator*(float scalar, const Vector2f& vec) {
    return vec * scalar;
}

}
}