#pragma once

#include "MathUtils.h"
#include <cmath>
#include <iostream>

namespace VoxelEditor {
namespace Math {

class Vector2f {
public:
    float x, y;
    
    // Constructors
    Vector2f() : x(0.0f), y(0.0f) {}
    Vector2f(float x, float y) : x(x), y(y) {}
    Vector2f(float scalar) : x(scalar), y(scalar) {}
    
    // Copy constructor and assignment
    Vector2f(const Vector2f& other) = default;
    Vector2f& operator=(const Vector2f& other) = default;
    
    // Arithmetic operators
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
        return Vector2f(x / scalar, y / scalar);
    }
    
    Vector2f operator-() const {
        return Vector2f(-x, -y);
    }
    
    // Compound assignment operators
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
        x /= scalar;
        y /= scalar;
        return *this;
    }
    
    // Comparison operators
    bool operator==(const Vector2f& other) const {
        return isEqual(x, other.x) && isEqual(y, other.y);
    }
    
    bool operator!=(const Vector2f& other) const {
        return !(*this == other);
    }
    
    // Array access
    float& operator[](int index) {
        return (&x)[index];
    }
    
    const float& operator[](int index) const {
        return (&x)[index];
    }
    
    // Vector operations
    float lengthSquared() const {
        return x * x + y * y;
    }
    
    float length() const {
        return std::sqrt(lengthSquared());
    }
    
    float manhattanLength() const {
        return std::abs(x) + std::abs(y);
    }
    
    Vector2f normalized() const {
        float len = length();
        if (isZero(len)) {
            return Vector2f(0.0f, 0.0f);
        }
        return *this / len;
    }
    
    Vector2f& normalize() {
        *this = normalized();
        return *this;
    }
    
    float dot(const Vector2f& other) const {
        return x * other.x + y * other.y;
    }
    
    // Cross product (returns scalar for 2D)
    float cross(const Vector2f& other) const {
        return x * other.y - y * other.x;
    }
    
    // Perpendicular vector (rotated 90 degrees counter-clockwise)
    Vector2f perpendicular() const {
        return Vector2f(-y, x);
    }
    
    // Rotation
    Vector2f rotated(float angle) const {
        float cosAngle = std::cos(angle);
        float sinAngle = std::sin(angle);
        return Vector2f(
            x * cosAngle - y * sinAngle,
            x * sinAngle + y * cosAngle
        );
    }
    
    // Angle to another vector
    float angleTo(const Vector2f& other) const {
        return std::atan2(cross(other), dot(other));
    }
    
    // Distance to another vector
    float distanceTo(const Vector2f& other) const {
        return (*this - other).length();
    }
    
    float distanceSquaredTo(const Vector2f& other) const {
        return (*this - other).lengthSquared();
    }
    
    // Static utility functions
    static Vector2f zero() { return Vector2f(0.0f, 0.0f); }
    static Vector2f one() { return Vector2f(1.0f, 1.0f); }
    static Vector2f unitX() { return Vector2f(1.0f, 0.0f); }
    static Vector2f unitY() { return Vector2f(0.0f, 1.0f); }
    
    static Vector2f lerp(const Vector2f& a, const Vector2f& b, float t) {
        return a + (b - a) * t;
    }
    
    static Vector2f min(const Vector2f& a, const Vector2f& b) {
        return Vector2f(std::min(a.x, b.x), std::min(a.y, b.y));
    }
    
    static Vector2f max(const Vector2f& a, const Vector2f& b) {
        return Vector2f(std::max(a.x, b.x), std::max(a.y, b.y));
    }
    
    static Vector2f abs(const Vector2f& v) {
        return Vector2f(std::abs(v.x), std::abs(v.y));
    }
    
    static Vector2f clamp(const Vector2f& v, const Vector2f& min, const Vector2f& max) {
        return Vector2f(
            Math::clamp(v.x, min.x, max.x),
            Math::clamp(v.y, min.y, max.y)
        );
    }
    
    static Vector2f fromAngle(float angle, float magnitude = 1.0f) {
        return Vector2f(std::cos(angle) * magnitude, std::sin(angle) * magnitude);
    }
};

// Global operators
inline Vector2f operator*(float scalar, const Vector2f& vector) {
    return vector * scalar;
}

// Stream operators
inline std::ostream& operator<<(std::ostream& os, const Vector2f& v) {
    return os << "Vector2f(" << v.x << ", " << v.y << ")";
}

inline std::istream& operator>>(std::istream& is, Vector2f& v) {
    return is >> v.x >> v.y;
}

}
}