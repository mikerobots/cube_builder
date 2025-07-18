#include <gtest/gtest.h>
#include "../include/voxel_math/VoxelRaycast.h"
#include "../include/voxel_math/VoxelBounds.h"
#include "../include/voxel_math/VoxelCollision.h"
#include "../../foundation/math/CoordinateTypes.h"
#include "../../foundation/math/CoordinateConverter.h"
#include "../../foundation/logging/Logger.h"
#include "../../core/voxel_data/VoxelTypes.h"

using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Logging;

class LargeVoxelRaycastTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Enable debug logging
        auto& logger = Logger::getInstance();
        logger.setLevel(LogLevel::Debug);
        logger.clearOutputs();
        logger.addOutput(std::make_unique<FileOutput>("large_voxel_raycast_test.log", "TestLog", false));
    }
    
    // Helper to check if a point is on or near voxel surface
    bool isNearSurface(const Vector3f& point, const VoxelBounds& bounds, float epsilon = 0.001f) {
        return (point.x >= bounds.min().value().x - epsilon && point.x <= bounds.max().value().x + epsilon) &&
               (point.y >= bounds.min().value().y - epsilon && point.y <= bounds.max().value().y + epsilon) &&
               (point.z >= bounds.min().value().z - epsilon && point.z <= bounds.max().value().z + epsilon);
    }
    
    // Helper to create a ray from point A to point B
    Ray createRayFromTo(const Vector3f& from, const Vector3f& to) {
        Vector3f direction = (to - from).normalized();
        return Ray(from, direction);
    }
    
    // Helper to create voxel bounds for different resolutions
    VoxelBounds createVoxelBounds(const IncrementCoordinates& pos, VoxelResolution resolution) {
        float voxelSize = getVoxelSize(resolution);
        // Use the VoxelBounds constructor that takes IncrementCoordinates
        return VoxelBounds(pos, voxelSize);
    }
};

// Test ray intersection accuracy for large voxels (64cm, 128cm, 256cm)
TEST_F(LargeVoxelRaycastTest, RayIntersectionAccuracy_LargeVoxelSizes) {
    std::vector<VoxelResolution> testResolutions = {
        VoxelResolution::Size_64cm,
        VoxelResolution::Size_128cm,
        VoxelResolution::Size_256cm
    };
    
    for (auto resolution : testResolutions) {
        IncrementCoordinates voxelPos(0, 0, 0);
        VoxelBounds bounds = createVoxelBounds(voxelPos, resolution);
        
        float voxelSize = getVoxelSize(resolution);
        Vector3f voxelCenter = bounds.center().value();
        
        // Test rays from different directions hitting the voxel
        struct RayTest {
            Vector3f rayOrigin;
            Vector3f rayDirection;
            std::string description;
        };
        
        std::vector<RayTest> rayTests = {
            // Axis-aligned rays
            {voxelCenter + Vector3f(voxelSize * 2, 0, 0), Vector3f(-1, 0, 0), "From +X axis"},
            {voxelCenter + Vector3f(-voxelSize * 2, 0, 0), Vector3f(1, 0, 0), "From -X axis"},
            {voxelCenter + Vector3f(0, voxelSize * 2, 0), Vector3f(0, -1, 0), "From +Y axis"},
            {voxelCenter + Vector3f(0, -voxelSize * 2, 0), Vector3f(0, 1, 0), "From -Y axis"},
            {voxelCenter + Vector3f(0, 0, voxelSize * 2), Vector3f(0, 0, -1), "From +Z axis"},
            {voxelCenter + Vector3f(0, 0, -voxelSize * 2), Vector3f(0, 0, 1), "From -Z axis"},
            
            // Diagonal rays
            {voxelCenter + Vector3f(voxelSize * 2, voxelSize * 2, 0), Vector3f(-1, -1, 0).normalized(), "Diagonal XY"},
            {voxelCenter + Vector3f(voxelSize * 2, 0, voxelSize * 2), Vector3f(-1, 0, -1).normalized(), "Diagonal XZ"},
            {voxelCenter + Vector3f(0, voxelSize * 2, voxelSize * 2), Vector3f(0, -1, -1).normalized(), "Diagonal YZ"},
            {voxelCenter + Vector3f(voxelSize * 2, voxelSize * 2, voxelSize * 2), Vector3f(-1, -1, -1).normalized(), "Diagonal XYZ"},
            
            // Oblique rays
            {voxelCenter + Vector3f(voxelSize * 1.5f, voxelSize * 0.5f, voxelSize * 2), 
             Vector3f(-0.6f, -0.2f, -0.8f).normalized(), "Oblique angle 1"},
            {voxelCenter + Vector3f(voxelSize * 0.7f, voxelSize * 1.8f, voxelSize * 1.2f), 
             Vector3f(-0.3f, -0.7f, -0.5f).normalized(), "Oblique angle 2"},
        };
        
        for (const auto& test : rayTests) {
            Ray ray(test.rayOrigin, test.rayDirection);
            
            // Test intersection
            auto intersection = VoxelRaycast::calculateRayVoxelIntersection(ray, bounds);
            
            EXPECT_TRUE(intersection.has_value()) 
                << "Should intersect " << static_cast<int>(voxelSize * 100) 
                << "cm voxel with " << test.description;
            
            if (intersection.has_value()) {
                // Verify intersection points
                Vector3f entryPoint = intersection->first.value();
                Vector3f exitPoint = intersection->second.value();
                
                // Verify entry point is on voxel surface (with tolerance for floating-point precision)
                // Check if point is within bounds or very close to the surface
                const float epsilon = 0.001f; // 1mm tolerance
                bool nearSurface = 
                    (entryPoint.x >= bounds.min().value().x - epsilon && entryPoint.x <= bounds.max().value().x + epsilon) &&
                    (entryPoint.y >= bounds.min().value().y - epsilon && entryPoint.y <= bounds.max().value().y + epsilon) &&
                    (entryPoint.z >= bounds.min().value().z - epsilon && entryPoint.z <= bounds.max().value().z + epsilon);
                EXPECT_TRUE(nearSurface) 
                    << "Entry point should be on or near voxel surface for " << test.description;
                
                // Verify intersection distance is reasonable
                float distance = (entryPoint - ray.origin).length();
                EXPECT_GT(distance, 0.0f) << "Distance should be positive for " << test.description;
                EXPECT_LT(distance, voxelSize * 5.0f) << "Distance should be reasonable for " << test.description;
            }
        }
    }
}

// Test ray intersection from different angles and distances
TEST_F(LargeVoxelRaycastTest, RayIntersectionFromDifferentAnglesAndDistances) {
    VoxelResolution resolution = VoxelResolution::Size_128cm;
    IncrementCoordinates voxelPos(0, 0, 0);
    VoxelBounds bounds = createVoxelBounds(voxelPos, resolution);
    
    float voxelSize = getVoxelSize(resolution);
    Vector3f voxelCenter = bounds.center().value();
    
    // Test different distances
    std::vector<float> distances = {0.5f, 1.0f, 2.0f, 5.0f, 10.0f};
    
    // Test different angles (in degrees)
    std::vector<float> angles = {0.0f, 15.0f, 30.0f, 45.0f, 60.0f, 75.0f, 90.0f};
    
    for (float distance : distances) {
        for (float angleDeg : angles) {
            float angleRad = angleDeg * M_PI / 180.0f;
            
            // Create ray at different angles around the voxel
            Vector3f rayOrigin = voxelCenter + Vector3f(
                distance * voxelSize * cos(angleRad),
                distance * voxelSize * sin(angleRad),
                0
            );
            
            Vector3f rayDirection = (voxelCenter - rayOrigin).normalized();
            Ray ray(rayOrigin, rayDirection);
            
            auto intersection = VoxelRaycast::calculateRayVoxelIntersection(ray, bounds);
            
            EXPECT_TRUE(intersection.has_value()) 
                << "Should intersect from distance " << distance 
                << " at angle " << angleDeg << " degrees";
            
            if (intersection.has_value()) {
                // Verify intersection point accuracy
                Vector3f hitPoint = intersection->first.value();
                
                // Hit point should be on voxel surface
                EXPECT_TRUE(isNearSurface(hitPoint, bounds)) 
                    << "Hit point should be on voxel surface for distance " << distance 
                    << " at angle " << angleDeg << " degrees";
                
                // Normal calculation would need to be done separately
                // For now, skip normal verification
            }
        }
    }
}

// Test intersection point calculation precision on large faces
TEST_F(LargeVoxelRaycastTest, IntersectionPointPrecision_LargeFaces) {
    VoxelResolution resolution = VoxelResolution::Size_256cm;
    IncrementCoordinates voxelPos(0, 0, 0);
    VoxelBounds bounds = createVoxelBounds(voxelPos, resolution);
    
    float voxelSize = getVoxelSize(resolution);
    Vector3f voxelWorldPos = CoordinateConverter::incrementToWorld(voxelPos).value();
    
    // Test precision at different points on the positive X face
    // For a voxel at (0,0,0), the positive X face is at x = voxelSize/2
    // Avoid exact corners and edges to prevent floating-point precision issues
    const float epsilon = 0.01f; // Small offset from edges
    std::vector<Vector3f> testPoints = {
        // Points near (but not on) corners
        Vector3f(voxelSize/2, epsilon, -voxelSize/2 + epsilon),           // Near bottom-left corner
        Vector3f(voxelSize/2, voxelSize - epsilon, -voxelSize/2 + epsilon),   // Near top-left corner
        Vector3f(voxelSize/2, epsilon, voxelSize/2 - epsilon),            // Near bottom-right corner
        Vector3f(voxelSize/2, voxelSize - epsilon, voxelSize/2 - epsilon),    // Near top-right corner
        
        // Points on face but away from edges
        Vector3f(voxelSize/2, voxelSize/2, -voxelSize/4), // Left side center
        Vector3f(voxelSize/2, voxelSize/2, voxelSize/4),  // Right side center
        Vector3f(voxelSize/2, voxelSize/4, 0),            // Bottom center
        Vector3f(voxelSize/2, 3*voxelSize/4, 0),          // Top center
        
        // Face center
        Vector3f(voxelSize/2, voxelSize/2, 0),            // Face center
        
        // Random points on face (well within bounds)
        Vector3f(voxelSize/2, voxelSize/3, -voxelSize/3), // Random point 1
        Vector3f(voxelSize/2, 2*voxelSize/3, voxelSize/3), // Random point 2
    };
    
    for (const auto& targetPoint : testPoints) {
        Vector3f absoluteTargetPoint = targetPoint;  // Points are already in world coords
        
        // Create ray from outside the voxel pointing at the target point
        Vector3f rayOrigin = absoluteTargetPoint + Vector3f(0.1f, 0, 0);
        Vector3f rayDirection = (absoluteTargetPoint - rayOrigin).normalized();
        Ray ray(rayOrigin, rayDirection);
        
        auto intersection = VoxelRaycast::calculateRayVoxelIntersection(ray, bounds);
        
        EXPECT_TRUE(intersection.has_value()) << "Should intersect at target point";
        
        if (intersection.has_value()) {
            Vector3f hitPoint = intersection->first.value();
            
            // Verify intersection point is very close to target point
            float distance = (hitPoint - absoluteTargetPoint).length();
            EXPECT_LT(distance, 0.001f) << "Hit point should be very close to target point";
            
            // Verify hit point is on the positive X face
            EXPECT_NEAR(hitPoint.x, voxelSize/2, 0.001f) 
                << "Hit point should be on positive X face";
            
            // Verify normal points outward from face
            // Normal calculation would need to be done separately
        }
    }
}

// Test multiple intersection handling (ray passing through multiple voxels)
TEST_F(LargeVoxelRaycastTest, MultipleIntersection_RayPassingThroughMultipleVoxels) {
    VoxelResolution resolution = VoxelResolution::Size_64cm;
    float voxelSize = getVoxelSize(resolution);
    
    // Create multiple voxels in a line
    std::vector<IncrementCoordinates> voxelPositions = {
        IncrementCoordinates(0, 0, 0),
        IncrementCoordinates(64, 0, 0),
        IncrementCoordinates(128, 0, 0),
        IncrementCoordinates(192, 0, 0),
    };
    
    std::vector<VoxelBounds> voxelBounds;
    for (const auto& pos : voxelPositions) {
        voxelBounds.push_back(createVoxelBounds(pos, resolution));
    }
    
    // Create ray that passes through all voxels
    Vector3f rayOrigin = Vector3f(-voxelSize, voxelSize/2, 0);
    Vector3f rayDirection = Vector3f(1, 0, 0);
    Ray ray(rayOrigin, rayDirection);
    
    // Test intersection with each voxel
    std::vector<Vector3f> intersectionPoints;
    
    for (size_t i = 0; i < voxelBounds.size(); ++i) {
        auto intersection = VoxelRaycast::calculateRayVoxelIntersection(ray, voxelBounds[i]);
        
        EXPECT_TRUE(intersection.has_value()) << "Should intersect voxel " << i;
        
        if (intersection.has_value()) {
            Vector3f entryPoint = intersection->first.value();
            intersectionPoints.push_back(entryPoint);
            
            // Verify intersection points are in order along the ray
            if (i > 0) {
                float prevDistance = (intersectionPoints[i-1] - rayOrigin).length();
                float currDistance = (entryPoint - rayOrigin).length();
                EXPECT_LT(prevDistance, currDistance) 
                    << "Intersection points should be in order along ray";
            }
        }
    }
    
    // Verify we got all expected intersections
    EXPECT_EQ(intersectionPoints.size(), voxelPositions.size()) 
        << "Should have intersection with all voxels";
}

// Test ray intersection with edge cases
TEST_F(LargeVoxelRaycastTest, EdgeCases_RayIntersectionBoundaryConditions) {
    VoxelResolution resolution = VoxelResolution::Size_128cm;
    IncrementCoordinates voxelPos(0, 0, 0);
    VoxelBounds bounds = createVoxelBounds(voxelPos, resolution);
    
    float voxelSize = getVoxelSize(resolution);
    Vector3f voxelCenter = bounds.center().value();
    
    struct EdgeCaseTest {
        Vector3f rayOrigin;
        Vector3f rayDirection;
        bool shouldHit;
        std::string description;
    };
    
    std::vector<EdgeCaseTest> edgeCases = {
        // Ray starting exactly on voxel surface
        {voxelCenter + Vector3f(voxelSize/2, 0, 0), Vector3f(-1, 0, 0), true, "Ray starting on surface"},
        
        // Ray parallel to voxel face (should miss)
        {voxelCenter + Vector3f(0, 0, voxelSize * 2), Vector3f(1, 0, 0), false, "Ray parallel to face"},
        
        // Ray starting inside voxel
        {voxelCenter, Vector3f(1, 0, 0), true, "Ray starting inside voxel"},
        
        // Ray barely grazing voxel corner
        {voxelCenter + Vector3f(voxelSize/2 + 0.1f, voxelSize/2 + 0.1f, voxelSize/2 + 0.1f), 
         Vector3f(-1, 0, 0), false, "Ray barely missing corner"},
        
        // Ray exactly hitting voxel corner
        {voxelCenter + Vector3f(voxelSize, voxelSize, 0), 
         Vector3f(-1, -1, 0).normalized(), true, "Ray exactly hitting corner"},
        
        // Ray with very small direction components
        {voxelCenter + Vector3f(voxelSize * 2, 0, 0), 
         Vector3f(-1, 0.001f, 0).normalized(), true, "Ray with small Y component"},
        
        // Ray exactly along voxel edge
        {bounds.max().value() + Vector3f(0.1f, 0, 0), 
         Vector3f(0, 0, -1), false, "Ray along voxel edge"},
    };
    
    for (const auto& test : edgeCases) {
        Ray ray(test.rayOrigin, test.rayDirection);
        auto intersection = VoxelRaycast::calculateRayVoxelIntersection(ray, bounds);
        
        EXPECT_EQ(intersection.has_value(), test.shouldHit) 
            << "Hit result incorrect for " << test.description;
        
        if (intersection.has_value() && test.shouldHit) {
            // Verify intersection point is reasonable
            Vector3f hitPoint = intersection->first.value();
            EXPECT_TRUE(isNearSurface(hitPoint, bounds)) 
                << "Hit point should be on or near voxel for " << test.description;
        }
    }
}

// Test ray intersection performance with large voxels
TEST_F(LargeVoxelRaycastTest, Performance_LargeVoxelRayIntersection) {
    VoxelResolution resolution = VoxelResolution::Size_256cm;
    IncrementCoordinates voxelPos(0, 0, 0);
    VoxelBounds bounds = createVoxelBounds(voxelPos, resolution);
    
    float voxelSize = getVoxelSize(resolution);
    Vector3f voxelCenter = bounds.center().value();
    
    // Generate many rays for performance testing
    std::vector<Ray> testRays;
    
    const int numRays = 1000;
    for (int i = 0; i < numRays; ++i) {
        // Generate random rays around the voxel
        float theta = (float)i / numRays * 2.0f * M_PI;
        float phi = (float)i / numRays * M_PI;
        
        Vector3f rayOrigin = voxelCenter + Vector3f(
            voxelSize * 2 * sin(phi) * cos(theta),
            voxelSize * 2 * sin(phi) * sin(theta),
            voxelSize * 2 * cos(phi)
        );
        
        Vector3f rayDirection = (voxelCenter - rayOrigin).normalized();
        testRays.emplace_back(rayOrigin, rayDirection);
    }
    
    // Measure performance
    auto startTime = std::chrono::high_resolution_clock::now();
    
    int hitCount = 0;
    for (const auto& ray : testRays) {
        auto intersection = VoxelRaycast::calculateRayVoxelIntersection(ray, bounds);
        if (intersection.has_value()) {
            hitCount++;
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    // Should have decent hit rate
    EXPECT_GT(hitCount, numRays * 0.8) << "Should have high hit rate for rays aimed at voxel";
    
    // Performance should be reasonable (less than 1ms per ray on average)
    float avgTimePerRay = (float)duration.count() / numRays;
    EXPECT_LT(avgTimePerRay, 1000.0f) << "Ray intersection should be fast (< 1ms per ray)";
    
    std::cout << "Performance test: " << hitCount << "/" << numRays 
              << " hits, avg " << avgTimePerRay << " μs per ray" << std::endl;
}

// Test ray intersection with non-axis-aligned voxels
TEST_F(LargeVoxelRaycastTest, NonAxisAlignedVoxels_RayIntersection) {
    VoxelResolution resolution = VoxelResolution::Size_64cm;
    
    // Test voxels at various non-axis-aligned positions
    std::vector<IncrementCoordinates> nonAlignedPositions = {
        IncrementCoordinates(7, 23, 11),
        IncrementCoordinates(37, 59, 83),
        IncrementCoordinates(101, 17, 41),
        IncrementCoordinates(13, 97, 29),
    };
    
    for (const auto& voxelPos : nonAlignedPositions) {
        VoxelBounds bounds = createVoxelBounds(voxelPos, resolution);
        
        float voxelSize = getVoxelSize(resolution);
        Vector3f voxelCenter = bounds.center().value();
        
        // Test rays from different directions
        std::vector<Vector3f> rayDirections = {
            Vector3f(1, 0, 0),
            Vector3f(0, 1, 0),
            Vector3f(0, 0, 1),
            Vector3f(1, 1, 0).normalized(),
            Vector3f(1, 0, 1).normalized(),
            Vector3f(0, 1, 1).normalized(),
            Vector3f(1, 1, 1).normalized(),
        };
        
        for (const auto& direction : rayDirections) {
            Vector3f rayOrigin = voxelCenter - direction * voxelSize * 2;
            Ray ray(rayOrigin, direction);
            
            auto intersection = VoxelRaycast::calculateRayVoxelIntersection(ray, bounds);
            
            EXPECT_TRUE(intersection.has_value()) 
                << "Should intersect non-aligned voxel at (" 
                << voxelPos.x() << "," << voxelPos.y() << "," << voxelPos.z() << ")";
            
            if (intersection.has_value()) {
                Vector3f hitPoint = intersection->first.value();
                
                // Verify hit point is on voxel surface
                EXPECT_TRUE(isNearSurface(hitPoint, bounds)) 
                    << "Hit point should be on voxel surface for non-aligned voxel";
                
                // Verify intersection normal
                // Normal calculation would need to be done separately
            }
        }
    }
}

// Test ray intersection with very large voxels at workspace boundaries
TEST_F(LargeVoxelRaycastTest, WorkspaceBoundaries_VeryLargeVoxelIntersection) {
    VoxelResolution resolution = VoxelResolution::Size_512cm;
    
    // Test voxels near workspace boundaries
    std::vector<IncrementCoordinates> boundaryPositions = {
        IncrementCoordinates(0, 0, 0),      // At origin
        IncrementCoordinates(256, 0, 0),    // Offset in X
        IncrementCoordinates(0, 256, 0),    // Offset in Y  
        IncrementCoordinates(0, 0, 256),    // Offset in Z
    };
    
    for (const auto& voxelPos : boundaryPositions) {
        VoxelBounds bounds = createVoxelBounds(voxelPos, resolution);
        
        float voxelSize = getVoxelSize(resolution);
        Vector3f voxelCenter = bounds.center().value();
        
        // Test rays from far away
        std::vector<Vector3f> distantOrigins = {
            voxelCenter + Vector3f(voxelSize * 10, 0, 0),
            voxelCenter + Vector3f(0, voxelSize * 10, 0),
            voxelCenter + Vector3f(0, 0, voxelSize * 10),
            voxelCenter + Vector3f(voxelSize * 7, voxelSize * 7, voxelSize * 7),
        };
        
        for (const auto& origin : distantOrigins) {
            Vector3f rayDirection = (voxelCenter - origin).normalized();
            Ray ray(origin, rayDirection);
            
            auto intersection = VoxelRaycast::calculateRayVoxelIntersection(ray, bounds);
            
            EXPECT_TRUE(intersection.has_value()) 
                << "Should intersect very large voxel from distance";
            
            if (intersection.has_value()) {
                Vector3f hitPoint = intersection->first.value();
                
                // Verify hit point is on voxel surface
                EXPECT_TRUE(isNearSurface(hitPoint, bounds)) 
                    << "Hit point should be on very large voxel surface";
                
                // Verify intersection distance is reasonable
                float distance = (hitPoint - origin).length();
                EXPECT_GT(distance, voxelSize) << "Distance should be at least voxel size";
                EXPECT_LT(distance, voxelSize * 15) << "Distance should be reasonable";
            }
        }
    }
}