# Math Utilities Foundation

## Purpose
Provides fundamental mathematical operations, vector/matrix mathematics, geometric calculations, and spatial utilities used throughout the voxel editor.

## Key Components

### Vector Operations
- Vector2f, Vector3f, Vector4f classes
- Vector arithmetic and operations
- Normalization and distance calculations
- Cross and dot products

### Matrix Operations
- Matrix3f, Matrix4f classes
- Transformation matrices
- Matrix multiplication and inversion
- Decomposition operations

### Geometric Utilities
- Ray and plane intersections
- Bounding box operations
- Frustum calculations
- Collision detection

### Spatial Mathematics
- Coordinate space conversions
- Voxel space calculations
- Octree spatial indexing
- LOD distance calculations

## Interface Design

```cpp
// Vector Classes
class Vector3f {
public:
    float x, y, z;
    
    Vector3f(float x = 0, float y = 0, float z = 0);
    
    // Arithmetic operations
    Vector3f operator+(const Vector3f& other) const;
    Vector3f operator-(const Vector3f& other) const;
    Vector3f operator*(float scalar) const;
    Vector3f operator/(float scalar) const;
    
    // Vector operations
    float dot(const Vector3f& other) const;
    Vector3f cross(const Vector3f& other) const;
    float length() const;
    float lengthSquared() const;
    Vector3f normalized() const;
    void normalize();
    
    // Static utilities
    static Vector3f Zero();
    static Vector3f One();
    static Vector3f UnitX();
    static Vector3f UnitY();
    static Vector3f UnitZ();
    static float distance(const Vector3f& a, const Vector3f& b);
    static Vector3f lerp(const Vector3f& a, const Vector3f& b, float t);
};

// Matrix Classes
class Matrix4f {
public:
    float m[16];
    
    Matrix4f();
    Matrix4f(const float* data);
    
    // Matrix operations
    Matrix4f operator*(const Matrix4f& other) const;
    Vector3f operator*(const Vector3f& vec) const;
    Matrix4f transposed() const;
    Matrix4f inverted() const;
    float determinant() const;
    
    // Transformation creation
    static Matrix4f translation(const Vector3f& translation);
    static Matrix4f rotation(const Vector3f& axis, float angle);
    static Matrix4f scale(const Vector3f& scale);
    static Matrix4f perspective(float fov, float aspect, float near, float far);
    static Matrix4f orthographic(float left, float right, float bottom, float top, float near, float far);
    static Matrix4f lookAt(const Vector3f& eye, const Vector3f& center, const Vector3f& up);
};
```

## Dependencies
- Standard C++ library only
- Platform-specific SIMD optimizations (optional)
- No external dependencies