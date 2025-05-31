#pragma once

#include "Vector3f.h"
#include <cmath>
#include <string>

namespace VoxelEditor {
namespace Math {

class Matrix4f {
public:
    float m[16];

    Matrix4f() {
        identity();
    }

    Matrix4f(const float* data) {
        for (int i = 0; i < 16; ++i) {
            m[i] = data[i];
        }
    }

    Matrix4f(
        float m00, float m01, float m02, float m03,
        float m10, float m11, float m12, float m13,
        float m20, float m21, float m22, float m23,
        float m30, float m31, float m32, float m33
    ) {
        m[0] = m00; m[1] = m01; m[2] = m02; m[3] = m03;
        m[4] = m10; m[5] = m11; m[6] = m12; m[7] = m13;
        m[8] = m20; m[9] = m21; m[10] = m22; m[11] = m23;
        m[12] = m30; m[13] = m31; m[14] = m32; m[15] = m33;
    }

    void identity() {
        for (int i = 0; i < 16; ++i) {
            m[i] = 0.0f;
        }
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    Matrix4f operator*(const Matrix4f& other) const {
        Matrix4f result;
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                result.m[row * 4 + col] = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    result.m[row * 4 + col] += m[row * 4 + k] * other.m[k * 4 + col];
                }
            }
        }
        return result;
    }

    Vector3f operator*(const Vector3f& vec) const {
        float w = m[12] * vec.x + m[13] * vec.y + m[14] * vec.z + m[15];
        if (std::abs(w) < 1e-8f) w = 1.0f;
        
        return Vector3f(
            (m[0] * vec.x + m[1] * vec.y + m[2] * vec.z + m[3]) / w,
            (m[4] * vec.x + m[5] * vec.y + m[6] * vec.z + m[7]) / w,
            (m[8] * vec.x + m[9] * vec.y + m[10] * vec.z + m[11]) / w
        );
    }

    Vector3f transformDirection(const Vector3f& dir) const {
        return Vector3f(
            m[0] * dir.x + m[1] * dir.y + m[2] * dir.z,
            m[4] * dir.x + m[5] * dir.y + m[6] * dir.z,
            m[8] * dir.x + m[9] * dir.y + m[10] * dir.z
        );
    }

    Matrix4f& operator*=(const Matrix4f& other) {
        *this = *this * other;
        return *this;
    }

    bool operator==(const Matrix4f& other) const {
        const float epsilon = 1e-6f;
        for (int i = 0; i < 16; ++i) {
            if (std::abs(m[i] - other.m[i]) > epsilon) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const Matrix4f& other) const {
        return !(*this == other);
    }

    float& operator[](int index) {
        return m[index];
    }

    const float& operator[](int index) const {
        return m[index];
    }

    Matrix4f transposed() const {
        return Matrix4f(
            m[0], m[4], m[8], m[12],
            m[1], m[5], m[9], m[13],
            m[2], m[6], m[10], m[14],
            m[3], m[7], m[11], m[15]
        );
    }

    float determinant() const {
        float det = 
            m[0] * (m[5] * (m[10] * m[15] - m[11] * m[14]) - 
                   m[9] * (m[6] * m[15] - m[7] * m[14]) + 
                   m[13] * (m[6] * m[11] - m[7] * m[10])) -
            m[4] * (m[1] * (m[10] * m[15] - m[11] * m[14]) - 
                   m[9] * (m[2] * m[15] - m[3] * m[14]) + 
                   m[13] * (m[2] * m[11] - m[3] * m[10])) +
            m[8] * (m[1] * (m[6] * m[15] - m[7] * m[14]) - 
                   m[5] * (m[2] * m[15] - m[3] * m[14]) + 
                   m[13] * (m[2] * m[7] - m[3] * m[6])) -
            m[12] * (m[1] * (m[6] * m[11] - m[7] * m[10]) - 
                    m[5] * (m[2] * m[11] - m[3] * m[10]) + 
                    m[9] * (m[2] * m[7] - m[3] * m[6]));
        return det;
    }

    Matrix4f inverted() const {
        float det = determinant();
        if (std::abs(det) < 1e-8f) {
            return Matrix4f();
        }

        Matrix4f result;
        float invDet = 1.0f / det;

        result.m[0] = (m[5] * (m[10] * m[15] - m[11] * m[14]) - 
                      m[9] * (m[6] * m[15] - m[7] * m[14]) + 
                      m[13] * (m[6] * m[11] - m[7] * m[10])) * invDet;

        result.m[1] = -(m[1] * (m[10] * m[15] - m[11] * m[14]) - 
                       m[9] * (m[2] * m[15] - m[3] * m[14]) + 
                       m[13] * (m[2] * m[11] - m[3] * m[10])) * invDet;

        result.m[2] = (m[1] * (m[6] * m[15] - m[7] * m[14]) - 
                      m[5] * (m[2] * m[15] - m[3] * m[14]) + 
                      m[13] * (m[2] * m[7] - m[3] * m[6])) * invDet;

        result.m[3] = -(m[1] * (m[6] * m[11] - m[7] * m[10]) - 
                       m[5] * (m[2] * m[11] - m[3] * m[10]) + 
                       m[9] * (m[2] * m[7] - m[3] * m[6])) * invDet;

        result.m[4] = -(m[4] * (m[10] * m[15] - m[11] * m[14]) - 
                       m[8] * (m[6] * m[15] - m[7] * m[14]) + 
                       m[12] * (m[6] * m[11] - m[7] * m[10])) * invDet;

        result.m[5] = (m[0] * (m[10] * m[15] - m[11] * m[14]) - 
                      m[8] * (m[2] * m[15] - m[3] * m[14]) + 
                      m[12] * (m[2] * m[11] - m[3] * m[10])) * invDet;

        result.m[6] = -(m[0] * (m[6] * m[15] - m[7] * m[14]) - 
                       m[4] * (m[2] * m[15] - m[3] * m[14]) + 
                       m[12] * (m[2] * m[7] - m[3] * m[6])) * invDet;

        result.m[7] = (m[0] * (m[6] * m[11] - m[7] * m[10]) - 
                      m[4] * (m[2] * m[11] - m[3] * m[10]) + 
                      m[8] * (m[2] * m[7] - m[3] * m[6])) * invDet;

        result.m[8] = (m[4] * (m[9] * m[15] - m[11] * m[13]) - 
                      m[8] * (m[5] * m[15] - m[7] * m[13]) + 
                      m[12] * (m[5] * m[11] - m[7] * m[9])) * invDet;

        result.m[9] = -(m[0] * (m[9] * m[15] - m[11] * m[13]) - 
                       m[8] * (m[1] * m[15] - m[3] * m[13]) + 
                       m[12] * (m[1] * m[11] - m[3] * m[9])) * invDet;

        result.m[10] = (m[0] * (m[5] * m[15] - m[7] * m[13]) - 
                       m[4] * (m[1] * m[15] - m[3] * m[13]) + 
                       m[12] * (m[1] * m[7] - m[3] * m[5])) * invDet;

        result.m[11] = -(m[0] * (m[5] * m[11] - m[7] * m[9]) - 
                        m[4] * (m[1] * m[11] - m[3] * m[9]) + 
                        m[8] * (m[1] * m[7] - m[3] * m[5])) * invDet;

        result.m[12] = -(m[4] * (m[9] * m[14] - m[10] * m[13]) - 
                        m[8] * (m[5] * m[14] - m[6] * m[13]) + 
                        m[12] * (m[5] * m[10] - m[6] * m[9])) * invDet;

        result.m[13] = (m[0] * (m[9] * m[14] - m[10] * m[13]) - 
                       m[8] * (m[1] * m[14] - m[2] * m[13]) + 
                       m[12] * (m[1] * m[10] - m[2] * m[9])) * invDet;

        result.m[14] = -(m[0] * (m[5] * m[14] - m[6] * m[13]) - 
                        m[4] * (m[1] * m[14] - m[2] * m[13]) + 
                        m[12] * (m[1] * m[6] - m[2] * m[5])) * invDet;

        result.m[15] = (m[0] * (m[5] * m[10] - m[6] * m[9]) - 
                       m[4] * (m[1] * m[10] - m[2] * m[9]) + 
                       m[8] * (m[1] * m[6] - m[2] * m[5])) * invDet;

        return result;
    }

    Vector3f getTranslation() const {
        return Vector3f(m[3], m[7], m[11]);
    }

    void setTranslation(const Vector3f& translation) {
        m[3] = translation.x;
        m[7] = translation.y;
        m[11] = translation.z;
    }

    static Matrix4f Identity() {
        return Matrix4f();
    }

    static Matrix4f translation(const Vector3f& translation) {
        Matrix4f result;
        result.m[3] = translation.x;
        result.m[7] = translation.y;
        result.m[11] = translation.z;
        return result;
    }

    static Matrix4f rotation(const Vector3f& axis, float angle) {
        Vector3f normalizedAxis = axis.normalized();
        float c = std::cos(angle);
        float s = std::sin(angle);
        float t = 1.0f - c;

        float x = normalizedAxis.x;
        float y = normalizedAxis.y;
        float z = normalizedAxis.z;

        return Matrix4f(
            t*x*x + c,    t*x*y - s*z,  t*x*z + s*y,  0.0f,
            t*x*y + s*z,  t*y*y + c,    t*y*z - s*x,  0.0f,
            t*x*z - s*y,  t*y*z + s*x,  t*z*z + c,    0.0f,
            0.0f,         0.0f,         0.0f,         1.0f
        );
    }

    static Matrix4f rotationX(float angle) {
        float c = std::cos(angle);
        float s = std::sin(angle);
        return Matrix4f(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, c,   -s,    0.0f,
            0.0f, s,    c,    0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
    }

    static Matrix4f rotationY(float angle) {
        float c = std::cos(angle);
        float s = std::sin(angle);
        return Matrix4f(
            c,    0.0f, s,    0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
           -s,    0.0f, c,    0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
    }

    static Matrix4f rotationZ(float angle) {
        float c = std::cos(angle);
        float s = std::sin(angle);
        return Matrix4f(
            c,   -s,    0.0f, 0.0f,
            s,    c,    0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
    }

    static Matrix4f scale(const Vector3f& scale) {
        Matrix4f result;
        result.m[0] = scale.x;
        result.m[5] = scale.y;
        result.m[10] = scale.z;
        return result;
    }

    static Matrix4f scale(float uniformScale) {
        return scale(Vector3f(uniformScale, uniformScale, uniformScale));
    }

    static Matrix4f perspective(float fovRadians, float aspect, float nearPlane, float farPlane) {
        float tanHalfFov = std::tan(fovRadians * 0.5f);
        float range = farPlane - nearPlane;

        Matrix4f result;
        result.m[0] = 1.0f / (aspect * tanHalfFov);
        result.m[5] = 1.0f / tanHalfFov;
        result.m[10] = -(farPlane + nearPlane) / range;
        result.m[11] = -2.0f * farPlane * nearPlane / range;
        result.m[14] = -1.0f;
        result.m[15] = 0.0f;
        return result;
    }

    static Matrix4f orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
        Matrix4f result;
        result.m[0] = 2.0f / (right - left);
        result.m[3] = -(right + left) / (right - left);
        result.m[5] = 2.0f / (top - bottom);
        result.m[7] = -(top + bottom) / (top - bottom);
        result.m[10] = -2.0f / (farPlane - nearPlane);
        result.m[11] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        return result;
    }

    static Matrix4f lookAt(const Vector3f& eye, const Vector3f& center, const Vector3f& up) {
        Vector3f f = (center - eye).normalized();
        Vector3f s = f.cross(up).normalized();
        Vector3f u = s.cross(f);

        Matrix4f result;
        result.m[0] = s.x;
        result.m[1] = s.y;
        result.m[2] = s.z;
        result.m[3] = -s.dot(eye);
        
        result.m[4] = u.x;
        result.m[5] = u.y;
        result.m[6] = u.z;
        result.m[7] = -u.dot(eye);
        
        result.m[8] = -f.x;
        result.m[9] = -f.y;
        result.m[10] = -f.z;
        result.m[11] = f.dot(eye);
        
        result.m[12] = 0.0f;
        result.m[13] = 0.0f;
        result.m[14] = 0.0f;
        result.m[15] = 1.0f;
        
        return result;
    }

    std::string toString() const {
        std::string result = "[\n";
        for (int row = 0; row < 4; ++row) {
            result += "  ";
            for (int col = 0; col < 4; ++col) {
                result += std::to_string(m[row * 4 + col]);
                if (col < 3) result += ", ";
            }
            result += "\n";
        }
        result += "]";
        return result;
    }

    // Point transformation (applies full matrix including translation)
    Vector3f transformPoint(const Vector3f& point) const {
        float w = m[12] * point.x + m[13] * point.y + m[14] * point.z + m[15];
        if (std::abs(w) < 1e-8f) {
            w = 1.0f; // Avoid division by zero
        }
        return Vector3f(
            (m[0] * point.x + m[1] * point.y + m[2] * point.z + m[3]) / w,
            (m[4] * point.x + m[5] * point.y + m[6] * point.z + m[7]) / w,
            (m[8] * point.x + m[9] * point.y + m[10] * point.z + m[11]) / w
        );
    }

    // Vector transformation (ignores translation)
    Vector3f transformVector(const Vector3f& vector) const {
        return Vector3f(
            m[0] * vector.x + m[1] * vector.y + m[2] * vector.z,
            m[4] * vector.x + m[5] * vector.y + m[6] * vector.z,
            m[8] * vector.x + m[9] * vector.y + m[10] * vector.z
        );
    }

    // Alias for inverted() to match common matrix APIs
    Matrix4f inverse() const {
        return inverted();
    }
};

}
}