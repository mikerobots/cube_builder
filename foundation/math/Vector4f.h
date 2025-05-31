#pragma once

#include <cmath>
#include <string>

namespace VoxelEditor {
namespace Math {

class Vector4f {
public:
    float x, y, z, w;

    Vector4f() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    Vector4f(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    Vector4f(float value) : x(value), y(value), z(value), w(value) {}

    Vector4f operator+(const Vector4f& other) const {
        return Vector4f(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    Vector4f operator-(const Vector4f& other) const {
        return Vector4f(x - other.x, y - other.y, z - other.z, w - other.w);
    }

    Vector4f operator*(float scalar) const {
        return Vector4f(x * scalar, y * scalar, z * scalar, w * scalar);
    }
    
    // Element-wise multiplication
    Vector4f operator*(const Vector4f& other) const {
        return Vector4f(x * other.x, y * other.y, z * other.z, w * other.w);
    }

    Vector4f operator/(float scalar) const {
        float inv = 1.0f / scalar;
        return Vector4f(x * inv, y * inv, z * inv, w * inv);
    }

    Vector4f operator-() const {
        return Vector4f(-x, -y, -z, -w);
    }

    Vector4f& operator+=(const Vector4f& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    Vector4f& operator-=(const Vector4f& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    Vector4f& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        w *= scalar;
        return *this;
    }

    Vector4f& operator/=(float scalar) {
        float inv = 1.0f / scalar;
        x *= inv;
        y *= inv;
        z *= inv;
        w *= inv;
        return *this;
    }

    float dot(const Vector4f& other) const {
        return x * other.x + y * other.y + z * other.z + w * other.w;
    }

    float length() const {
        return std::sqrt(x * x + y * y + z * z + w * w);
    }

    float lengthSquared() const {
        return x * x + y * y + z * z + w * w;
    }

    Vector4f normalized() const {
        float len = length();
        if (len > 0.0f) {
            float invLen = 1.0f / len;
            return Vector4f(x * invLen, y * invLen, z * invLen, w * invLen);
        }
        return *this;
    }

    void normalize() {
        float len = length();
        if (len > 0.0f) {
            float invLen = 1.0f / len;
            x *= invLen;
            y *= invLen;
            z *= invLen;
            w *= invLen;
        }
    }

    bool operator==(const Vector4f& other) const {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    bool operator!=(const Vector4f& other) const {
        return !(*this == other);
    }

    float& operator[](int index) {
        return (&x)[index];
    }

    const float& operator[](int index) const {
        return (&x)[index];
    }

    std::string toString() const {
        return "Vector4f(" + std::to_string(x) + ", " + std::to_string(y) + ", " + 
               std::to_string(z) + ", " + std::to_string(w) + ")";
    }

    static Vector4f Zero() { return Vector4f(0.0f, 0.0f, 0.0f, 0.0f); }
    static Vector4f One() { return Vector4f(1.0f, 1.0f, 1.0f, 1.0f); }
};

inline Vector4f operator*(float scalar, const Vector4f& vec) {
    return vec * scalar;
}

}
}