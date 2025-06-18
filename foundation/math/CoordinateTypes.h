#pragma once

#include "Vector3f.h"
#include "Vector3i.h"
#include "Vector2f.h"
#include <type_traits>

namespace VoxelEditor {
namespace Math {

// Forward declarations
class WorldCoordinates;
class IncrementCoordinates;
class ScreenCoordinates;

/**
 * Strong wrapper type for world space coordinates (continuous 3D space)
 * - Used for: rendering, camera positions, ray casting, physics
 * - Units: meters, centered at origin (0,0,0), Y-up
 * - Underlying type: Vector3f
 */
class WorldCoordinates {
public:
    // Constructors
    WorldCoordinates() : m_value() {}
    explicit WorldCoordinates(const Vector3f& value) : m_value(value) {}
    WorldCoordinates(float x, float y, float z) : m_value(x, y, z) {}

    // Access underlying value
    const Vector3f& value() const { return m_value; }
    Vector3f& value() { return m_value; }

    // Component access
    float x() const { return m_value.x; }
    float y() const { return m_value.y; }
    float z() const { return m_value.z; }
    float& x() { return m_value.x; }
    float& y() { return m_value.y; }
    float& z() { return m_value.z; }

    // Arithmetic operators (with same type)
    WorldCoordinates operator+(const WorldCoordinates& other) const {
        return WorldCoordinates(m_value + other.m_value);
    }
    WorldCoordinates operator-(const WorldCoordinates& other) const {
        return WorldCoordinates(m_value - other.m_value);
    }
    WorldCoordinates operator*(float scalar) const {
        return WorldCoordinates(m_value * scalar);
    }
    WorldCoordinates operator/(float scalar) const {
        return WorldCoordinates(m_value / scalar);
    }

    WorldCoordinates& operator+=(const WorldCoordinates& other) {
        m_value += other.m_value;
        return *this;
    }
    WorldCoordinates& operator-=(const WorldCoordinates& other) {
        m_value -= other.m_value;
        return *this;
    }
    WorldCoordinates& operator*=(float scalar) {
        m_value *= scalar;
        return *this;
    }
    WorldCoordinates& operator/=(float scalar) {
        m_value /= scalar;
        return *this;
    }

    // Comparison operators
    bool operator==(const WorldCoordinates& other) const {
        return m_value == other.m_value;
    }
    bool operator!=(const WorldCoordinates& other) const {
        return m_value != other.m_value;
    }

    // Vector operations
    float length() const { return m_value.length(); }
    float lengthSquared() const { return m_value.lengthSquared(); }
    WorldCoordinates normalized() const { return WorldCoordinates(m_value.normalized()); }
    float dot(const WorldCoordinates& other) const { return m_value.dot(other.m_value); }
    WorldCoordinates cross(const WorldCoordinates& other) const {
        return WorldCoordinates(m_value.cross(other.m_value));
    }

    // Static constants
    static WorldCoordinates zero() { return WorldCoordinates(Vector3f::Zero()); }
    static WorldCoordinates unitX() { return WorldCoordinates(Vector3f::UnitX()); }
    static WorldCoordinates unitY() { return WorldCoordinates(Vector3f::UnitY()); }
    static WorldCoordinates unitZ() { return WorldCoordinates(Vector3f::UnitZ()); }

private:
    Vector3f m_value;
};

/**
 * Strong wrapper type for increment coordinates (universal voxel storage)
 * - Used for: universal voxel storage and addressing at 1cm granularity
 * - Units: 1cm increments, centered at origin (0,0,0), Y-up
 * - Coordinate System: Matches WorldCoordinates centering, integer precision
 * - Underlying type: Vector3i
 */
class IncrementCoordinates {
public:
    // Constructors
    IncrementCoordinates() : m_value() {}
    explicit IncrementCoordinates(const Vector3i& value) : m_value(value) {}
    IncrementCoordinates(int x, int y, int z) : m_value(x, y, z) {}

    // Access underlying value
    const Vector3i& value() const { return m_value; }
    Vector3i& value() { return m_value; }

    // Component access
    int x() const { return m_value.x; }
    int y() const { return m_value.y; }
    int z() const { return m_value.z; }
    int& x() { return m_value.x; }
    int& y() { return m_value.y; }
    int& z() { return m_value.z; }

    // Arithmetic operators (with same type)
    IncrementCoordinates operator+(const IncrementCoordinates& other) const {
        return IncrementCoordinates(m_value + other.m_value);
    }
    IncrementCoordinates operator-(const IncrementCoordinates& other) const {
        return IncrementCoordinates(m_value - other.m_value);
    }
    IncrementCoordinates operator*(int scalar) const {
        return IncrementCoordinates(m_value * scalar);
    }

    IncrementCoordinates& operator+=(const IncrementCoordinates& other) {
        m_value += other.m_value;
        return *this;
    }
    IncrementCoordinates& operator-=(const IncrementCoordinates& other) {
        m_value -= other.m_value;
        return *this;
    }
    IncrementCoordinates& operator*=(int scalar) {
        m_value *= scalar;
        return *this;
    }

    // Comparison operators
    bool operator==(const IncrementCoordinates& other) const {
        return m_value == other.m_value;
    }
    bool operator!=(const IncrementCoordinates& other) const {
        return m_value != other.m_value;
    }
    bool operator<(const IncrementCoordinates& other) const {
        return m_value < other.m_value;
    }

    // Vector operations
    int dot(const IncrementCoordinates& other) const { return m_value.dot(other.m_value); }
    float length() const { return m_value.length(); }
    int lengthSquared() const { return m_value.lengthSquared(); }
    int manhattanLength() const { return m_value.manhattanLength(); }

    // Hash support
    size_t hash() const { return m_value.hash(); }

    // Static constants
    static IncrementCoordinates zero() { return IncrementCoordinates(Vector3i::Zero()); }
    static IncrementCoordinates unitX() { return IncrementCoordinates(Vector3i::UnitX()); }
    static IncrementCoordinates unitY() { return IncrementCoordinates(Vector3i::UnitY()); }
    static IncrementCoordinates unitZ() { return IncrementCoordinates(Vector3i::UnitZ()); }

private:
    Vector3i m_value;
};

/**
 * Strong wrapper type for screen coordinates (2D viewport positions)
 * - Used for: mouse input, UI elements, viewport operations
 * - Units: screen pixels
 * - Underlying type: Vector2f
 */
class ScreenCoordinates {
public:
    // Constructors
    ScreenCoordinates() : m_value() {}
    explicit ScreenCoordinates(const Vector2f& value) : m_value(value) {}
    ScreenCoordinates(float x, float y) : m_value(x, y) {}

    // Access underlying value
    const Vector2f& value() const { return m_value; }
    Vector2f& value() { return m_value; }

    // Component access
    float x() const { return m_value.x; }
    float y() const { return m_value.y; }
    float& x() { return m_value.x; }
    float& y() { return m_value.y; }

    // Arithmetic operators (with same type)
    ScreenCoordinates operator+(const ScreenCoordinates& other) const {
        return ScreenCoordinates(m_value + other.m_value);
    }
    ScreenCoordinates operator-(const ScreenCoordinates& other) const {
        return ScreenCoordinates(m_value - other.m_value);
    }
    ScreenCoordinates operator*(float scalar) const {
        return ScreenCoordinates(m_value * scalar);
    }
    ScreenCoordinates operator/(float scalar) const {
        return ScreenCoordinates(m_value / scalar);
    }

    ScreenCoordinates& operator+=(const ScreenCoordinates& other) {
        m_value += other.m_value;
        return *this;
    }
    ScreenCoordinates& operator-=(const ScreenCoordinates& other) {
        m_value -= other.m_value;
        return *this;
    }
    ScreenCoordinates& operator*=(float scalar) {
        m_value *= scalar;
        return *this;
    }
    ScreenCoordinates& operator/=(float scalar) {
        m_value /= scalar;
        return *this;
    }

    // Comparison operators
    bool operator==(const ScreenCoordinates& other) const {
        return m_value == other.m_value;
    }
    bool operator!=(const ScreenCoordinates& other) const {
        return m_value != other.m_value;
    }

    // Vector operations
    float length() const { return m_value.length(); }
    float lengthSquared() const { return m_value.lengthSquared(); }
    ScreenCoordinates normalized() const { return ScreenCoordinates(m_value.normalized()); }
    float dot(const ScreenCoordinates& other) const { return m_value.dot(other.m_value); }

    // Static constants
    static ScreenCoordinates zero() { return ScreenCoordinates(Vector2f::zero()); }
    static ScreenCoordinates unitX() { return ScreenCoordinates(Vector2f::unitX()); }
    static ScreenCoordinates unitY() { return ScreenCoordinates(Vector2f::unitY()); }

private:
    Vector2f m_value;
};

// Global scalar multiplication operators
inline WorldCoordinates operator*(float scalar, const WorldCoordinates& coord) {
    return coord * scalar;
}

inline IncrementCoordinates operator*(int scalar, const IncrementCoordinates& coord) {
    return coord * scalar;
}

inline ScreenCoordinates operator*(float scalar, const ScreenCoordinates& coord) {
    return coord * scalar;
}

} // namespace Math
} // namespace VoxelEditor

// Hash specializations for std::unordered_map support
namespace std {
    template<>
    struct hash<VoxelEditor::Math::IncrementCoordinates> {
        size_t operator()(const VoxelEditor::Math::IncrementCoordinates& coord) const {
            return coord.hash();
        }
    };
}