#include "../include/visual_feedback/GeometricFaceDetector.h"
#include <cmath>
#include <algorithm>

namespace VoxelEditor {
namespace VisualFeedback {

GeometricFace GeometricFace::fromVertices(const std::vector<Math::Vector3f>& verts, int faceId) {
    GeometricFace face;
    face.vertices = verts;
    face.id = faceId;
    
    // Compute normal using cross product of first two edges
    // For counter-clockwise vertices viewed from outside,
    // (v1-v0) × (v2-v0) gives outward normal
    if (verts.size() >= 3) {
        Math::Vector3f v1 = verts[1] - verts[0];
        Math::Vector3f v2 = verts[2] - verts[0];
        face.normal = v1.cross(v2).normalized();
    }
    
    return face;
}

RayFaceHit GeometricFaceDetector::rayFaceIntersection(const Ray& ray, const GeometricFace& face) {
    RayFaceHit result;
    result.hit = false;
    result.faceId = face.id;
    
    // Compute face center for plane equation
    Math::Vector3f center(0, 0, 0);
    for (const auto& v : face.vertices) {
        center += v;
    }
    center /= static_cast<float>(face.vertices.size());
    
    // Ray-plane intersection
    auto t = rayPlaneIntersection(ray, center, face.normal);
    if (!t.has_value()) {
        return result;
    }
    
    // Compute intersection point
    Math::Vector3f point = ray.origin.value() + ray.direction * t.value();
    
    // Check if point is inside the polygon
    if (pointInConvexPolygon(point, face.vertices, face.normal)) {
        result.hit = true;
        result.distance = t.value();
        result.point = point;
        
    }
    
    return result;
}

std::optional<float> GeometricFaceDetector::rayPlaneIntersection(const Ray& ray,
                                                                const Math::Vector3f& planePoint,
                                                                const Math::Vector3f& planeNormal) {
    float denom = ray.direction.dot(planeNormal);
    
    // Check if ray is parallel to plane
    if (std::abs(denom) < 1e-6f) {
        return std::nullopt;
    }
    
    Math::Vector3f toPlane = planePoint - ray.origin.value();
    float t = toPlane.dot(planeNormal) / denom;
    
    // Only consider intersections in front of the ray
    if (t < 0) {
        return std::nullopt;
    }
    
    return t;
}

bool GeometricFaceDetector::pointInConvexPolygon(const Math::Vector3f& point,
                                                const std::vector<Math::Vector3f>& vertices,
                                                const Math::Vector3f& normal) {
    if (vertices.size() < 3) {
        return false;
    }
    
    // Use cross product method for convex polygons
    // All cross products should point in the same direction (aligned with normal)
    for (size_t i = 0; i < vertices.size(); ++i) {
        size_t j = (i + 1) % vertices.size();
        
        Math::Vector3f edge = vertices[j] - vertices[i];
        Math::Vector3f toPoint = point - vertices[i];
        Math::Vector3f cross = edge.cross(toPoint);
        
        // Check if cross product is aligned with normal
        if (cross.dot(normal) < 0) {
            return false;
        }
    }
    
    return true;
}

std::optional<RayFaceHit> GeometricFaceDetector::detectClosestFace(const Ray& ray,
                                                                  const std::vector<GeometricFace>& faces) {
    std::optional<RayFaceHit> closestHit;
    float minDistance = std::numeric_limits<float>::infinity();
    
    for (const auto& face : faces) {
        RayFaceHit hit = rayFaceIntersection(ray, face);
        if (hit.hit && hit.distance < minDistance) {
            minDistance = hit.distance;
            closestHit = hit;
        }
    }
    
    return closestHit;
}

std::vector<GeometricFace> GeometricFaceDetector::createBoxFaces(const Math::Vector3f& center, float size) {
    std::vector<GeometricFace> faces;
    float halfSize = size * 0.5f;
    
    // Define corner positions using consistent naming convention
    // This prevents vertex ordering errors that lead to incorrect normals
    // Naming: XYZ where X,Y,Z are L(ow/-half) or H(igh/+half)
    // Example: LHL = (-half, +half, -half) = left, high, back corner
    Math::Vector3f LLL = center + Math::Vector3f(-halfSize, -halfSize, -halfSize);
    Math::Vector3f HLL = center + Math::Vector3f(+halfSize, -halfSize, -halfSize);
    Math::Vector3f HHL = center + Math::Vector3f(+halfSize, +halfSize, -halfSize);
    Math::Vector3f LHL = center + Math::Vector3f(-halfSize, +halfSize, -halfSize);
    Math::Vector3f LLH = center + Math::Vector3f(-halfSize, -halfSize, +halfSize);
    Math::Vector3f HLH = center + Math::Vector3f(+halfSize, -halfSize, +halfSize);
    Math::Vector3f HHH = center + Math::Vector3f(+halfSize, +halfSize, +halfSize);
    Math::Vector3f LHH = center + Math::Vector3f(-halfSize, +halfSize, +halfSize);
    
    // Face 0: +X face (right face, normal pointing in +X direction)
    // Vertices ordered counter-clockwise when viewed from +X (outside)
    faces.push_back(GeometricFace::fromVertices({HLH, HLL, HHL, HHH}, 0));
    
    // Face 1: -X face (left face, normal pointing in -X direction)
    // Vertices ordered counter-clockwise when viewed from -X (outside)
    faces.push_back(GeometricFace::fromVertices({LLL, LLH, LHH, LHL}, 1));
    
    // Face 2: +Y face (top face, normal pointing in +Y direction)
    // Vertices ordered counter-clockwise when viewed from +Y (above)
    faces.push_back(GeometricFace::fromVertices({LHH, HHH, HHL, LHL}, 2));
    
    // Face 3: -Y face (bottom face, normal pointing in -Y direction)
    // Vertices ordered counter-clockwise when viewed from -Y (below)
    faces.push_back(GeometricFace::fromVertices({LLH, LLL, HLL, HLH}, 3));
    
    // Face 4: +Z face (front face, normal pointing in +Z direction)
    // Vertices ordered counter-clockwise when viewed from +Z (front)
    faces.push_back(GeometricFace::fromVertices({LLH, HLH, HHH, LHH}, 4));
    
    // Face 5: -Z face (back face, normal pointing in -Z direction)
    // Vertices ordered counter-clockwise when viewed from -Z (back)
    faces.push_back(GeometricFace::fromVertices({HLL, LLL, LHL, HHL}, 5));
    
    return faces;
}

std::vector<GeometricFace> GeometricFaceDetector::createVoxelFaces(const Math::Vector3f& worldPos, float voxelSize) {
    // For a voxel placed on the ground plane:
    // - worldPos.x and worldPos.z are the CENTER coordinates
    // - worldPos.y is the BOTTOM of the voxel
    // So we only need to adjust Y to get the center
    Math::Vector3f center(worldPos.x, worldPos.y + voxelSize * 0.5f, worldPos.z);
    
    
    return createBoxFaces(center, voxelSize);
}

} // namespace VisualFeedback
} // namespace VoxelEditor