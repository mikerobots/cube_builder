#pragma once

#include <iostream>

namespace VoxelEditor {
namespace Math {

class Vector2i {
public:
    int x, y;
    
    // Constructors
    Vector2i() : x(0), y(0) {}
    Vector2i(int x, int y) : x(x), y(y) {}
    Vector2i(int scalar) : x(scalar), y(scalar) {}
    
    // Copy constructor and assignment
    Vector2i(const Vector2i& other) = default;
    Vector2i& operator=(const Vector2i& other) = default;
    
    // Arithmetic operators
    Vector2i operator+(const Vector2i& other) const {
        return Vector2i(x + other.x, y + other.y);
    }
    
    Vector2i operator-(const Vector2i& other) const {
        return Vector2i(x - other.x, y - other.y);
    }
    
    Vector2i operator*(int scalar) const {
        return Vector2i(x * scalar, y * scalar);
    }
    
    Vector2i operator/(int scalar) const {
        return Vector2i(x / scalar, y / scalar);
    }
    
    Vector2i operator-() const {
        return Vector2i(-x, -y);
    }
    
    // Compound assignment operators
    Vector2i& operator+=(const Vector2i& other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    
    Vector2i& operator-=(const Vector2i& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    
    Vector2i& operator*=(int scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }
    
    Vector2i& operator/=(int scalar) {
        x /= scalar;
        y /= scalar;
        return *this;
    }
    
    // Comparison operators
    bool operator==(const Vector2i& other) const {
        return x == other.x && y == other.y;
    }
    
    bool operator!=(const Vector2i& other) const {
        return !(*this == other);
    }
    
    // Array access
    int& operator[](int index) {
        return (&x)[index];
    }
    
    const int& operator[](int index) const {
        return (&x)[index];
    }
    
    // Utility functions
    int lengthSquared() const {
        return x * x + y * y;
    }
    
    float length() const {
        return std::sqrt(static_cast<float>(lengthSquared()));
    }
    
    int manhattanLength() const {
        return std::abs(x) + std::abs(y);
    }
    
    int dot(const Vector2i& other) const {
        return x * other.x + y * other.y;
    }
    
    // Static utility functions
    static Vector2i zero() { return Vector2i(0, 0); }
    static Vector2i one() { return Vector2i(1, 1); }
    static Vector2i unitX() { return Vector2i(1, 0); }
    static Vector2i unitY() { return Vector2i(0, 1); }
    
    static Vector2i min(const Vector2i& a, const Vector2i& b) {
        return Vector2i(std::min(a.x, b.x), std::min(a.y, b.y));
    }
    
    static Vector2i max(const Vector2i& a, const Vector2i& b) {
        return Vector2i(std::max(a.x, b.x), std::max(a.y, b.y));
    }
    
    static Vector2i abs(const Vector2i& v) {
        return Vector2i(std::abs(v.x), std::abs(v.y));
    }
};

// Global operators
inline Vector2i operator*(int scalar, const Vector2i& vector) {
    return vector * scalar;
}

// Stream operators
inline std::ostream& operator<<(std::ostream& os, const Vector2i& v) {
    return os << "Vector2i(" << v.x << ", " << v.y << ")";
}

inline std::istream& operator>>(std::istream& is, Vector2i& v) {
    return is >> v.x >> v.y;
}

}
}