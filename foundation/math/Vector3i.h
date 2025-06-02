#pragma once

#include <cmath>
#include <string>

namespace VoxelEditor {
namespace Math {

class Vector3i {
public:
    int x, y, z;

    Vector3i() : x(0), y(0), z(0) {}
    Vector3i(int x, int y, int z) : x(x), y(y), z(z) {}
    Vector3i(int value) : x(value), y(value), z(value) {}

    Vector3i operator+(const Vector3i& other) const {
        return Vector3i(x + other.x, y + other.y, z + other.z);
    }

    Vector3i operator-(const Vector3i& other) const {
        return Vector3i(x - other.x, y - other.y, z - other.z);
    }

    Vector3i operator*(int scalar) const {
        return Vector3i(x * scalar, y * scalar, z * scalar);
    }

    Vector3i operator/(int scalar) const {
        return Vector3i(x / scalar, y / scalar, z / scalar);
    }

    Vector3i operator-() const {
        return Vector3i(-x, -y, -z);
    }

    Vector3i& operator+=(const Vector3i& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vector3i& operator-=(const Vector3i& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    Vector3i& operator*=(int scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    Vector3i& operator/=(int scalar) {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    bool operator==(const Vector3i& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator!=(const Vector3i& other) const {
        return !(*this == other);
    }

    bool operator<(const Vector3i& other) const {
        if (x != other.x) return x < other.x;
        if (y != other.y) return y < other.y;
        return z < other.z;
    }

    int operator[](int index) const {
        return (&x)[index];
    }

    int& operator[](int index) {
        return (&x)[index];
    }

    int dot(const Vector3i& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    Vector3i cross(const Vector3i& other) const {
        return Vector3i(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    float length() const {
        return std::sqrt(static_cast<float>(x * x + y * y + z * z));
    }

    int lengthSquared() const {
        return x * x + y * y + z * z;
    }

    int manhattanLength() const {
        return std::abs(x) + std::abs(y) + std::abs(z);
    }

    int maxComponent() const {
        return std::max({std::abs(x), std::abs(y), std::abs(z)});
    }

    static Vector3i Zero() {
        return Vector3i(0, 0, 0);
    }

    static Vector3i One() {
        return Vector3i(1, 1, 1);
    }

    static Vector3i UnitX() {
        return Vector3i(1, 0, 0);
    }

    static Vector3i UnitY() {
        return Vector3i(0, 1, 0);
    }

    static Vector3i UnitZ() {
        return Vector3i(0, 0, 1);
    }

    static float distance(const Vector3i& a, const Vector3i& b) {
        return (b - a).length();
    }

    static int distanceSquared(const Vector3i& a, const Vector3i& b) {
        return (b - a).lengthSquared();
    }

    static int manhattanDistance(const Vector3i& a, const Vector3i& b) {
        return (b - a).manhattanLength();
    }

    static Vector3i min(const Vector3i& a, const Vector3i& b) {
        return Vector3i(
            ::std::min(a.x, b.x),
            ::std::min(a.y, b.y),
            ::std::min(a.z, b.z)
        );
    }

    static Vector3i max(const Vector3i& a, const Vector3i& b) {
        return Vector3i(
            ::std::max(a.x, b.x),
            ::std::max(a.y, b.y),
            ::std::max(a.z, b.z)
        );
    }

    static Vector3i clamp(const Vector3i& value, const Vector3i& min, const Vector3i& max) {
        return Vector3i(
            ::std::clamp(value.x, min.x, max.x),
            ::std::clamp(value.y, min.y, max.y),
            ::std::clamp(value.z, min.z, max.z)
        );
    }

    size_t hash() const {
        size_t h1 = ::std::hash<int>{}(x);
        size_t h2 = ::std::hash<int>{}(y);
        size_t h3 = ::std::hash<int>{}(z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }

    std::string toString() const {
        return "(" + ::std::to_string(x) + ", " + ::std::to_string(y) + ", " + ::std::to_string(z) + ")";
    }
};

inline Vector3i operator*(int scalar, const Vector3i& vec) {
    return vec * scalar;
}

}
}

namespace std {
    template<>
    struct hash<VoxelEditor::Math::Vector3i> {
        size_t operator()(const VoxelEditor::Math::Vector3i& v) const {
            return v.hash();
        }
    };
}