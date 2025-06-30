#pragma once

#include "Vector3f.h"
#include <cmath>
#include <string>

namespace VoxelEditor {
namespace Math {

class Quaternion {
public:
    float x, y, z, w;
    
    // Constructors
    Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
    
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    
    Quaternion(const Vector3f& axis, float angle) {
        float halfAngle = angle * 0.5f;
        float s = std::sin(halfAngle);
        Vector3f normalizedAxis = axis.normalized();
        
        x = normalizedAxis.x * s;
        y = normalizedAxis.y * s;
        z = normalizedAxis.z * s;
        w = std::cos(halfAngle);
    }
    
    // Copy constructor and assignment
    Quaternion(const Quaternion& other) = default;
    Quaternion& operator=(const Quaternion& other) = default;
    
    // Arithmetic operations
    Quaternion operator+(const Quaternion& other) const {
        return Quaternion(x + other.x, y + other.y, z + other.z, w + other.w);
    }
    
    Quaternion operator-(const Quaternion& other) const {
        return Quaternion(x - other.x, y - other.y, z - other.z, w - other.w);
    }
    
    Quaternion operator*(const Quaternion& other) const {
        return Quaternion(
            w * other.x + x * other.w + y * other.z - z * other.y,
            w * other.y - x * other.z + y * other.w + z * other.x,
            w * other.z + x * other.y - y * other.x + z * other.w,
            w * other.w - x * other.x - y * other.y - z * other.z
        );
    }
    
    Quaternion operator*(float scalar) const {
        return Quaternion(x * scalar, y * scalar, z * scalar, w * scalar);
    }
    
    Quaternion& operator+=(const Quaternion& other) {
        x += other.x; y += other.y; z += other.z; w += other.w;
        return *this;
    }
    
    Quaternion& operator-=(const Quaternion& other) {
        x -= other.x; y -= other.y; z -= other.z; w -= other.w;
        return *this;
    }
    
    Quaternion& operator*=(const Quaternion& other) {
        *this = *this * other;
        return *this;
    }
    
    Quaternion& operator*=(float scalar) {
        x *= scalar; y *= scalar; z *= scalar; w *= scalar;
        return *this;
    }
    
    // Comparison
    bool operator==(const Quaternion& other) const {
        const float epsilon = 1e-6f;
        return std::abs(x - other.x) < epsilon && 
               std::abs(y - other.y) < epsilon && 
               std::abs(z - other.z) < epsilon && 
               std::abs(w - other.w) < epsilon;
    }
    
    bool operator!=(const Quaternion& other) const {
        return !(*this == other);
    }
    
    // Vector operations
    float length() const {
        return std::sqrt(x * x + y * y + z * z + w * w);
    }
    
    float lengthSquared() const {
        return x * x + y * y + z * z + w * w;
    }
    
    Quaternion normalized() const {
        float len = length();
        if (len > 1e-8f) {
            return Quaternion(x / len, y / len, z / len, w / len);
        }
        return identity();
    }
    
    void normalize() {
        float len = length();
        if (len > 1e-8f) {
            x /= len; y /= len; z /= len; w /= len;
        } else {
            *this = identity();
        }
    }
    
    Quaternion conjugate() const {
        return Quaternion(-x, -y, -z, w);
    }
    
    Quaternion inverse() const {
        float lenSq = lengthSquared();
        if (lenSq > 1e-8f) {
            return conjugate() * (1.0f / lenSq);
        }
        return identity();
    }
    
    float dot(const Quaternion& other) const {
        return x * other.x + y * other.y + z * other.z + w * other.w;
    }
    
    // Rotation operations
    Vector3f rotate(const Vector3f& vector) const {
        Quaternion quat(vector.x, vector.y, vector.z, 0.0f);
        Quaternion result = (*this) * quat * conjugate();
        return Vector3f(result.x, result.y, result.z);
    }
    
    Vector3f getEulerAngles() const {
        Vector3f angles;
        
        // Roll (x-axis rotation)
        float sinr_cosp = 2 * (w * x + y * z);
        float cosr_cosp = 1 - 2 * (x * x + y * y);
        angles.x = std::atan2(sinr_cosp, cosr_cosp);
        
        // Pitch (y-axis rotation)
        float sinp = 2 * (w * y - z * x);
        if (std::abs(sinp) >= 1)
            angles.y = std::copysign(M_PI / 2, sinp); // use 90 degrees if out of range
        else
            angles.y = std::asin(sinp);
        
        // Yaw (z-axis rotation)
        float siny_cosp = 2 * (w * z + x * y);
        float cosy_cosp = 1 - 2 * (y * y + z * z);
        angles.z = std::atan2(siny_cosp, cosy_cosp);
        
        return angles;
    }
    
    // Static factory methods
    static Quaternion identity() {
        return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    static Quaternion fromEulerAngles(float pitch, float yaw, float roll) {
        float cp = std::cos(pitch * 0.5f);
        float sp = std::sin(pitch * 0.5f);
        float cy = std::cos(yaw * 0.5f);
        float sy = std::sin(yaw * 0.5f);
        float cr = std::cos(roll * 0.5f);
        float sr = std::sin(roll * 0.5f);
        
        return Quaternion(
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy,
            cr * cp * cy + sr * sp * sy
        );
    }
    
    static Quaternion fromEulerAngles(const Vector3f& eulerAngles) {
        return fromEulerAngles(eulerAngles.x, eulerAngles.y, eulerAngles.z);
    }
    
    static Quaternion fromAxisAngle(const Vector3f& axis, float angle) {
        return Quaternion(axis, angle);
    }
    
    static Quaternion lookRotation(const Vector3f& forward, const Vector3f& up = Vector3f::UnitY()) {
        Vector3f f = forward.normalized();
        Vector3f u = up.normalized();
        Vector3f r = u.cross(f).normalized();
        u = f.cross(r).normalized();
        
        // Create rotation matrix from basis vectors
        // r = right, u = up, f = forward
        float m00 = r.x, m01 = r.y, m02 = r.z;
        float m10 = u.x, m11 = u.y, m12 = u.z;
        float m20 = f.x, m21 = f.y, m22 = f.z;
        
        // Convert rotation matrix to quaternion
        float trace = m00 + m11 + m22;
        if (trace > 0.0f) {
            float s = 0.5f / std::sqrt(trace + 1.0f);
            return Quaternion(
                (m21 - m12) * s,
                (m02 - m20) * s,
                (m10 - m01) * s,
                0.25f / s
            );
        } else if (m00 > m11 && m00 > m22) {
            float s = 2.0f * std::sqrt(1.0f + m00 - m11 - m22);
            return Quaternion(
                0.25f * s,
                (m01 + m10) / s,
                (m02 + m20) / s,
                (m21 - m12) / s
            );
        } else if (m11 > m22) {
            float s = 2.0f * std::sqrt(1.0f + m11 - m00 - m22);
            return Quaternion(
                (m01 + m10) / s,
                0.25f * s,
                (m12 + m21) / s,
                (m02 - m20) / s
            );
        } else {
            float s = 2.0f * std::sqrt(1.0f + m22 - m00 - m11);
            return Quaternion(
                (m02 + m20) / s,
                (m12 + m21) / s,
                0.25f * s,
                (m10 - m01) / s
            );
        }
    }
    
    static Quaternion slerp(const Quaternion& a, const Quaternion& b, float t) {
        float dot = a.dot(b);
        
        // If the dot product is negative, slerp won't take the shorter path.
        Quaternion b_adjusted = (dot < 0.0f) ? Quaternion(-b.x, -b.y, -b.z, -b.w) : b;
        dot = std::abs(dot);
        
        // If the inputs are too close for comfort, linearly interpolate
        if (dot > 0.9995f) {
            Quaternion result = a + (b_adjusted - a) * t;
            return result.normalized();
        }
        
        float theta_0 = std::acos(dot);
        float sin_theta_0 = std::sin(theta_0);
        float theta = theta_0 * t;
        float sin_theta = std::sin(theta);
        
        float s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
        float s1 = sin_theta / sin_theta_0;
        
        return (a * s0) + (b_adjusted * s1);
    }
    
    static float angle(const Quaternion& a, const Quaternion& b) {
        float dot = std::abs(a.dot(b));
        dot = std::min(dot, 1.0f);
        return 2.0f * std::acos(dot);
    }
    
    std::string toString() const {
        return "Quaternion(" + std::to_string(x) + ", " + std::to_string(y) + ", " + 
               std::to_string(z) + ", " + std::to_string(w) + ")";
    }
};

}
}