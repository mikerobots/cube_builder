#pragma once

#include <vector>
#include <optional>
#include <limits>
#include "foundation/math/Vector3f.h"
#include "FeedbackTypes.h"

namespace VoxelEditor {
namespace VisualFeedback {

/**
 * @brief Geometric face representation for ray-face intersection testing
 * 
 * A face is defined by its vertices (assumed to form a convex polygon).
 * The normal is calculated automatically from the vertices using the
 * right-hand rule, assuming counter-clockwise vertex ordering when
 * viewed from outside.
 */
struct GeometricFace {
    std::vector<Math::Vector3f> vertices;  ///< Face vertices in counter-clockwise order
    Math::Vector3f normal;                 ///< Outward-pointing normal vector
    int id;                                ///< Unique face identifier
    
    /**
     * @brief Construct face from vertices with automatic normal calculation
     * @param verts Vertices in counter-clockwise order when viewed from outside
     * @param faceId Unique identifier for this face
     * @return Constructed GeometricFace with computed normal
     */
    static GeometricFace fromVertices(const std::vector<Math::Vector3f>& verts, int faceId);
};

/**
 * @brief Result of ray-face intersection test
 */
struct RayFaceHit {
    bool hit = false;                                          ///< Whether ray hit the face
    float distance = std::numeric_limits<float>::infinity();   ///< Distance to hit point
    Math::Vector3f point;                                      ///< World space hit point
    int faceId = -1;                                          ///< ID of the hit face
};

/**
 * @brief Clean geometric face detection algorithm
 * 
 * This class provides pure geometric ray-face intersection testing,
 * independent of the voxel system. It handles:
 * - Ray-polygon intersection for convex faces
 * - Proper face normal calculation
 * - Distance-based hit sorting
 * 
 * @note All faces must have vertices ordered counter-clockwise when
 *       viewed from outside to ensure correct normal calculation.
 */
class GeometricFaceDetector {
public:
    /**
     * @brief Find intersection of ray with a single face
     * @param ray Ray to test
     * @param face Face to test against
     * @return Hit result with distance and hit point if intersection found
     */
    static RayFaceHit rayFaceIntersection(const Ray& ray, const GeometricFace& face);
    
    /**
     * @brief Find the closest face hit by the ray from a collection of faces
     * @param ray Ray to test
     * @param faces Collection of faces to test against
     * @return Closest hit result, or empty optional if no hits
     */
    static std::optional<RayFaceHit> detectClosestFace(const Ray& ray, const std::vector<GeometricFace>& faces);
    
    /**
     * @brief Create the 6 faces of an axis-aligned box
     * 
     * Creates faces with proper vertex ordering for outward normals.
     * Face order: +X, -X, +Y, -Y, +Z, -Z
     * 
     * @param center Center of the box in world space
     * @param size Size of the box (same for all dimensions)
     * @return Vector of 6 faces with correct normals
     */
    static std::vector<GeometricFace> createBoxFaces(const Math::Vector3f& center, float size);
    
    /**
     * @brief Create box faces for a voxel placed on the ground plane
     * 
     * IMPORTANT: This handles the voxel coordinate convention where:
     * - worldPos.x and worldPos.z are CENTER coordinates
     * - worldPos.y is the BOTTOM coordinate
     * 
     * @param worldPos World position where voxel is placed
     * @param voxelSize Size of the voxel
     * @return Vector of 6 faces representing the voxel
     */
    static std::vector<GeometricFace> createVoxelFaces(const Math::Vector3f& worldPos, float voxelSize);

private:
    /**
     * @brief Find intersection of ray with infinite plane
     * @param ray Ray to test
     * @param planePoint Any point on the plane
     * @param planeNormal Normal vector of the plane
     * @return Distance parameter t where intersection = ray.origin + t * ray.direction
     */
    static std::optional<float> rayPlaneIntersection(const Ray& ray, 
                                                    const Math::Vector3f& planePoint, 
                                                    const Math::Vector3f& planeNormal);
    
    /**
     * @brief Check if a point lies inside a convex polygon
     * 
     * Uses the cross product method: for a convex polygon with counter-clockwise
     * vertices, all cross products of edge vectors and point vectors should
     * align with the face normal.
     * 
     * @param point Point to test (must be coplanar with polygon)
     * @param vertices Polygon vertices in counter-clockwise order
     * @param normal Face normal vector
     * @return True if point is inside the polygon
     */
    static bool pointInConvexPolygon(const Math::Vector3f& point, 
                                    const std::vector<Math::Vector3f>& vertices, 
                                    const Math::Vector3f& normal);
};

} // namespace VisualFeedback
} // namespace VoxelEditor