#include "../include/visual_feedback/GeometricFaceDetector.h"
#include "../../foundation/logging/Logger.h"
#include <cmath>
#include <algorithm>

namespace VoxelEditor {
namespace VisualFeedback {

GeometricFace GeometricFace::fromVertices(const std::vector<Math::Vector3f>& verts, int faceId) {
    GeometricFace face;
    face.vertices = verts;
    face.id = faceId;
    
    // Compute normal using cross product of first two edges
    // IMPORTANT: The vertices are in counter-clockwise order when viewed from outside,
    // so we use standard cross product to get outward normal
    if (verts.size() >= 3) {
        Math::Vector3f v1 = verts[1] - verts[0];
        Math::Vector3f v2 = verts[2] - verts[0];
        // Standard cross product for counter-clockwise vertices
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
    
    // Debug: Log face details for first few faces (reduced output)
    if (face.id < 3) {
        std::cout << "Face " << face.id << " center(" << center.x << ", " << center.y << ", " << center.z 
                  << ") normal(" << face.normal.x << ", " << face.normal.y << ", " << face.normal.z << ")" << std::endl;
    }
    
    // Ray-plane intersection
    auto t = rayPlaneIntersection(ray, center, face.normal);
    if (!t.has_value()) {
        return result;
    }
    
    // Compute intersection point
    Math::Vector3f point = ray.origin.value() + ray.direction * t.value();
    
    // Check if point is inside the polygon
    bool inPolygon = pointInConvexPolygon(point, face.vertices, face.normal);
    if (face.id < 6) {
        std::cout << "Face " << face.id << " hit: " << (inPolygon ? "YES" : "NO") << " at t=" << t.value() << std::endl;
    }
    
    if (inPolygon) {
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
    if (t <= 0) {
        return std::nullopt;
    }
    
    // Apply selective backface culling: 
    // - For hits very close to ray origin (t < small epsilon), allow backface hits
    //   This handles rays starting inside or very close to face surfaces
    // - For hits further away, apply standard backface culling
    // Ray hits front of face when ray direction and normal point in opposite directions (denom < 0)
    const float epsilon = 0.1f; // 10cm tolerance - large enough to handle rays inside voxels
    if (t > epsilon && denom > 0) {
        return std::nullopt;  // Ray is hitting the back of the face from far away, ignore it
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
    // IMPORTANT: For counter-clockwise vertices with outward normal, the cross products
    // will point in the SAME direction as the normal. So we need to check for positive dot product.
    for (size_t i = 0; i < vertices.size(); ++i) {
        size_t j = (i + 1) % vertices.size();
        
        Math::Vector3f edge = vertices[j] - vertices[i];
        Math::Vector3f toPoint = point - vertices[i];
        Math::Vector3f cross = edge.cross(toPoint);
        float dotProduct = cross.dot(normal);
        
        // For counter-clockwise vertices viewed from outside:
        // - If point is to the RIGHT of edge (outside), cross product points OPPOSITE to normal
        // - If point is to the LEFT of edge (inside), cross product points SAME as normal
        // So a point is OUTSIDE if ANY cross product has negative dot with normal
        // Use small epsilon to handle floating-point precision errors
        const float epsilon = 1e-6f;
        if (dotProduct < -epsilon) {
            return false;
        }
    }
    
    
    return true;
}

std::optional<RayFaceHit> GeometricFaceDetector::detectClosestFace(const Ray& ray,
                                                                  const std::vector<GeometricFace>& faces) {
    std::optional<RayFaceHit> closestHit;
    float minDistance = std::numeric_limits<float>::infinity();
    
    // Debug logging
    Logging::Logger& logger = Logging::Logger::getInstance();
    
    std::cout << "=== DEBUG: Testing " << faces.size() << " faces for ray origin(" 
              << ray.origin.x() << ", " << ray.origin.y() << ", " << ray.origin.z() 
              << ") dir(" << ray.direction.x << ", " << ray.direction.y << ", " << ray.direction.z << ") ===" << std::endl;
    
    int hitCount = 0;
    for (const auto& face : faces) {
        RayFaceHit hit = rayFaceIntersection(ray, face);
        
        if (hit.hit) {
            hitCount++;
            std::cout << "=== DEBUG: HIT face " << face.id << " at distance " << hit.distance;
            if (hit.distance < minDistance) {
                std::cout << " (NEW CLOSEST)";
                minDistance = hit.distance;
                closestHit = hit;
            } else if (std::abs(hit.distance - minDistance) < 1e-6f) {
                std::cout << " (SAME DISTANCE)";
                // Tie-breaking: prefer faces that are closer to the ray line
                // Calculate distance from hit point to ray line
                Math::Vector3f rayToHit = hit.point - ray.origin.value();
                Math::Vector3f projection = ray.direction * rayToHit.dot(ray.direction);
                Math::Vector3f perpendicular = rayToHit - projection;
                float distanceToRayLine = perpendicular.length();
                
                if (closestHit.has_value()) {
                    Math::Vector3f rayToClosest = closestHit->point - ray.origin.value();
                    Math::Vector3f closestProjection = ray.direction * rayToClosest.dot(ray.direction);
                    Math::Vector3f closestPerpendicular = rayToClosest - closestProjection;
                    float closestDistanceToRayLine = closestPerpendicular.length();
                    
                    if (distanceToRayLine < closestDistanceToRayLine) {
                        std::cout << " (BETTER TIE-BREAK)";
                        closestHit = hit;
                    }
                }
            }
            std::cout << " ===" << std::endl;
            
            // Log face hits for debugging
            if (face.id >= 0 && face.id < 6) {
                const char* faceNames[] = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"};
                logger.debugfc("GeometricFaceDetector", 
                    "Ray hit face %d (%s) at distance %.3f, normal: (%.3f, %.3f, %.3f)",
                    face.id, faceNames[face.id % 6], hit.distance,
                    face.normal.x, face.normal.y, face.normal.z);
            }
        }
    }
    
    std::cout << "=== DEBUG: Total hits: " << hitCount << " out of " << faces.size() << " faces ===" << std::endl;
    
    if (closestHit.has_value() && closestHit->faceId >= 0 && closestHit->faceId < 6) {
        const char* faceNames[] = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"};
        logger.debugfc("GeometricFaceDetector", 
            "Closest face selected: %d (%s) at distance %.3f",
            closestHit->faceId, faceNames[closestHit->faceId % 6], closestHit->distance);
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
    // CONFIRMED: worldPos from incrementToWorld() is the bottom-left-back corner of the voxel
    // This is the standard coordinate system used throughout the project
    // To get the center for box creation, add half the voxel size in all dimensions
    float halfSize = voxelSize * 0.5f;
    Math::Vector3f center(worldPos.x + halfSize, worldPos.y + halfSize, worldPos.z + halfSize);
    
    return createBoxFaces(center, voxelSize);
}

} // namespace VisualFeedback
} // namespace VoxelEditor