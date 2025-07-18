#include "../include/visual_feedback/FaceDetector.h"
#include "../include/visual_feedback/GeometricFaceDetector.h"
#include "../../voxel_data/VoxelDataManager.h"
#include "../../foundation/logging/Logger.h"
#include <algorithm>
#include <limits>
#include <cmath>
#include <vector>

namespace VoxelEditor {
namespace VisualFeedback {

FaceDetector::FaceDetector()
    : m_maxRayDistance(1000.0f) {
}

FaceDetector::~FaceDetector() = default;

// Enhancement: Ground plane detection
Face FaceDetector::detectGroundPlane(const Ray& ray) {
    // Check if ray intersects the Y=0 plane
    if (std::abs(ray.direction.y) < 0.0001f) {
        // Ray is parallel to ground plane
        return Face();
    }
    
    // Calculate intersection point
    float t = -ray.origin.y() / ray.direction.y;
    
    if (t < 0) {
        // Intersection is behind the ray origin
        return Face();
    }
    
    Math::WorldCoordinates hitPoint = ray.pointAt(t);
    
    // Check if hit point is within max distance
    if (t > m_maxRayDistance) {
        return Face();
    }
    
    // Create a ground plane face
    return Face::GroundPlane(hitPoint);
}

// Enhancement: Combined face/ground detection
Face FaceDetector::detectFaceOrGround(const Ray& ray, const VoxelData::VoxelGrid& grid,
                                     VoxelData::VoxelResolution resolution) {
    // First try to detect a voxel face
    Face voxelFace = detectFace(ray, grid, resolution);
    
    if (voxelFace.isValid()) {
        return voxelFace;
    }
    
    // If no voxel face hit, check ground plane
    return detectGroundPlane(ray);
}

Face FaceDetector::detectFaceAcrossAllResolutions(const Ray& ray, const VoxelData::VoxelDataManager& voxelManager) {
    Face closestFace;
    float closestDistance = std::numeric_limits<float>::max();
    
    // Check all resolution levels
    for (int i = 0; i < static_cast<int>(VoxelData::VoxelResolution::COUNT); ++i) {
        VoxelData::VoxelResolution resolution = static_cast<VoxelData::VoxelResolution>(i);
        const VoxelData::VoxelGrid* grid = voxelManager.getGrid(resolution);
        
        if (!grid || grid->getVoxelCount() == 0) {
            continue;
        }
        
        // Detect face in this resolution
        Face face = detectFace(ray, *grid, resolution);
        
        if (face.isValid()) {
            // Calculate distance to this face
            Math::IncrementCoordinates voxelPos = face.getVoxelPosition();
            Math::WorldCoordinates worldPos = Math::CoordinateConverter::incrementToWorld(voxelPos);
            float voxelSize = VoxelData::getVoxelSize(resolution);
            
            // Calculate the face plane position based on face direction
            Math::Vector3f facePos = worldPos.value();
            Math::Vector3f normal = face.getNormal();
            
            // Offset to the face (voxels use bottom-center coordinates)
            if (std::abs(normal.y) > 0.5f) {
                // Top/bottom face
                if (normal.y > 0) facePos.y += voxelSize; // Top face
                // Bottom face stays at voxelPos.y
            } else if (std::abs(normal.x) > 0.5f) {
                // Left/right face
                float halfSize = voxelSize * 0.5f;
                if (normal.x > 0) facePos.x += halfSize; // Right face
                else facePos.x -= halfSize; // Left face
            } else if (std::abs(normal.z) > 0.5f) {
                // Front/back face
                float halfSize = voxelSize * 0.5f;
                if (normal.z > 0) facePos.z += halfSize; // Front face
                else facePos.z -= halfSize; // Back face
            }
            
            // Calculate distance from ray origin to face
            float distance = (facePos - ray.origin.value()).length();
            
            if (distance < closestDistance) {
                closestDistance = distance;
                closestFace = face;
            }
        }
    }
    
    // If no voxel face was hit, check ground plane
    if (!closestFace.isValid()) {
        return detectGroundPlane(ray);
    }
    
    return closestFace;
}

Face FaceDetector::detectFace(const Ray& ray, const VoxelData::VoxelGrid& grid, 
                             VoxelData::VoxelResolution resolution) {
    
    RaycastHit hit = raycastVoxelGrid(ray, grid, resolution);
    
    if (hit.hit) {
        return createFaceFromHit(hit, resolution);
    }
    
    return Face(); // Invalid face
}

std::vector<Face> FaceDetector::detectFacesInRegion(const Math::BoundingBox& region, 
                                                   const VoxelData::VoxelGrid& grid, 
                                                   VoxelData::VoxelResolution resolution) {
    std::vector<Face> faces;
    
    // Get all voxels in the grid - this is much more efficient than iterating through all positions
    auto allVoxels = grid.getAllVoxels();
    
    // Convert region bounds to increment coordinates for checking
    Math::IncrementCoordinates minIncrement = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(region.min));
    Math::IncrementCoordinates maxIncrement = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(region.max));
    
    // Check each voxel to see if it's in the region
    for (const auto& voxelPos : allVoxels) {
        // Check if voxel is within the region bounds
        if (voxelPos.incrementPos.x() >= minIncrement.x() && voxelPos.incrementPos.x() <= maxIncrement.x() &&
            voxelPos.incrementPos.y() >= minIncrement.y() && voxelPos.incrementPos.y() <= maxIncrement.y() &&
            voxelPos.incrementPos.z() >= minIncrement.z() && voxelPos.incrementPos.z() <= maxIncrement.z()) {
            
            // Check if this voxel has the resolution we're looking for
            if (voxelPos.resolution == resolution) {
                // Check all 6 faces
                for (int dir = 0; dir < 6; ++dir) {
                    Face face(voxelPos.incrementPos, resolution, static_cast<FaceDirection>(dir));
                    if (isValidFaceForPlacement(face, grid)) {
                        faces.push_back(face);
                    }
                }
            }
        }
    }
    
    return faces;
}

bool FaceDetector::isValidFaceForPlacement(const Face& face, const VoxelData::VoxelGrid& grid) {
    // Check if the voxel on the face normal side is empty
    Math::IncrementCoordinates adjacentPos = calculatePlacementPosition(face);
    return !grid.getVoxel(adjacentPos);
}

Math::IncrementCoordinates FaceDetector::calculatePlacementPosition(const Face& face) {
    // Enhancement: Handle ground plane faces
    if (face.isGroundPlane()) {
        // For ground plane, use coordinate converter to snap to nearest increment position
        Math::WorldCoordinates hitPoint = face.getGroundPlaneHitPoint();
        
        // Convert to increment coordinates (which snaps to 1cm increments)
        Math::IncrementCoordinates incrementPos = Math::CoordinateConverter::worldToIncrement(hitPoint);
        
        // Ensure Y=0 for ground plane placement
        return Math::IncrementCoordinates(incrementPos.x(), 0, incrementPos.z());
    }
    
    // Original voxel face logic
    Math::IncrementCoordinates pos = face.getVoxelPosition();
    Math::Vector3i offset(0, 0, 0);
    
    // Calculate the voxel size in centimeters for proper offset
    int voxelSize_cm = static_cast<int>(VoxelData::getVoxelSize(face.getResolution()) * Math::CoordinateConverter::METERS_TO_CM);
    
    switch (face.getDirection()) {
        case FaceDirection::PositiveX: offset.x = voxelSize_cm; break;
        case FaceDirection::NegativeX: offset.x = -voxelSize_cm; break;
        case FaceDirection::PositiveY: offset.y = voxelSize_cm; break;
        case FaceDirection::NegativeY: offset.y = -voxelSize_cm; break;
        case FaceDirection::PositiveZ: offset.z = voxelSize_cm; break;
        case FaceDirection::NegativeZ: offset.z = -voxelSize_cm; break;
    }
    
    return Math::IncrementCoordinates(pos.value() + offset);
}

RaycastHit FaceDetector::raycastVoxelGrid(const Ray& ray, const VoxelData::VoxelGrid& grid, 
                                         VoxelData::VoxelResolution resolution) {
    RaycastHit result;
    result.hit = false;
    
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    // Use geometric face detection for improved accuracy
    const bool USE_GEOMETRIC_DETECTION = true;
    
    if (USE_GEOMETRIC_DETECTION) {
        // Get all voxels in the grid
        auto allVoxels = grid.getAllVoxels();
        
        
        // Store voxel info with faces for efficient lookup
        struct VoxelFaceInfo {
            Math::IncrementCoordinates voxelPos;
            FaceDirection faceDir;
        };
        std::vector<GeometricFace> allFaces;
        std::vector<VoxelFaceInfo> faceInfos;
        
        for (const auto& voxelInfo : allVoxels) {
            if (voxelInfo.resolution == resolution) {
                // Get world position of voxel
                Math::WorldCoordinates voxelWorld = Math::CoordinateConverter::incrementToWorld(voxelInfo.incrementPos);
                
                
                // Create faces for this voxel (voxels are placed with bottom at Y coordinate)
                auto voxelFaces = GeometricFaceDetector::createVoxelFaces(voxelWorld.value(), voxelSize);
                
                // Add faces with proper tracking
                for (size_t i = 0; i < voxelFaces.size(); ++i) {
                    voxelFaces[i].id = static_cast<int>(allFaces.size()); // Unique ID for each face
                    allFaces.push_back(voxelFaces[i]);
                    
                    // Map face index to direction
                    FaceDirection dir;
                    switch (i) {
                        case 0: dir = FaceDirection::PositiveX; break;
                        case 1: dir = FaceDirection::NegativeX; break;
                        case 2: dir = FaceDirection::PositiveY; break;
                        case 3: dir = FaceDirection::NegativeY; break;
                        case 4: dir = FaceDirection::PositiveZ; break;
                        case 5: dir = FaceDirection::NegativeZ; break;
                        default: dir = FaceDirection::PositiveY; break;
                    }
                    
                    faceInfos.push_back({voxelInfo.incrementPos, dir});
                }
            }
        }
        
        
        // Find closest face hit
        auto hit = GeometricFaceDetector::detectClosestFace(ray, allFaces);
        
        if (hit.has_value() && hit->faceId >= 0 && hit->faceId < static_cast<int>(faceInfos.size())) {
            const auto& faceInfo = faceInfos[hit->faceId];
            
            result.hit = true;
            result.distance = hit->distance;
            result.position = Math::WorldCoordinates(hit->point);
            result.face = Face(faceInfo.voxelPos, resolution, faceInfo.faceDir, Math::WorldCoordinates(hit->point));
            
            // Set normal based on face direction
            switch (faceInfo.faceDir) {
                case FaceDirection::PositiveX: result.normal = Math::Vector3f(1, 0, 0); break;
                case FaceDirection::NegativeX: result.normal = Math::Vector3f(-1, 0, 0); break;
                case FaceDirection::PositiveY: result.normal = Math::Vector3f(0, 1, 0); break;
                case FaceDirection::NegativeY: result.normal = Math::Vector3f(0, -1, 0); break;
                case FaceDirection::PositiveZ: result.normal = Math::Vector3f(0, 0, 1); break;
                case FaceDirection::NegativeZ: result.normal = Math::Vector3f(0, 0, -1); break;
            }
        }
        
        return result;
    }
    
    // Get workspace bounds in world coordinates (centered)
    Math::Vector3f workspaceSize = grid.getWorkspaceSize();
    Math::Vector3f gridMin(-workspaceSize.x * 0.5f, 0.0f, -workspaceSize.z * 0.5f);
    Math::Vector3f gridMax(workspaceSize.x * 0.5f, workspaceSize.y, workspaceSize.z * 0.5f);
    
    // Check if ray intersects workspace bounds
    float tMin, tMax;
    Math::BoundingBox workspaceBounds(gridMin, gridMax);
    
    if (!rayIntersectsBox(ray, workspaceBounds, tMin, tMax)) {
        return result;
    }
    
    // Initialize traversal at workspace entry point, not ray origin
    GridTraversal traversal;
    
    // Calculate the ray position at workspace entry (tMin)
    // Use the original ray if it starts inside the workspace (tMin <= 0)
    Math::WorldCoordinates startPoint;
    
    if (tMin > 0) {
        // Ray starts outside workspace - move back from entry point to ensure we don't miss boundary voxels
        // The key insight: when ray enters at the exact workspace boundary, we might already be past boundary voxels
        // So we need to start the traversal from a point that's guaranteed to be before any boundary voxels
        
        // Move back by two voxel sizes to ensure we catch boundary voxels
        // This ensures we start traversal before any boundary voxels
        float backStep = voxelSize * 2.0f;
        float adjustedT = std::max(0.0f, tMin - backStep);
        startPoint = ray.pointAt(adjustedT);
    } else {
        // Ray starts inside workspace
        startPoint = ray.origin;
    }
    
    // Create a ray from the start point
    VoxelEditor::VisualFeedback::Ray startRay(startPoint, ray.direction);
    initializeTraversal(startRay, Math::WorldCoordinates(gridMin), voxelSize, resolution, traversal);
    
    
    // OPTIMIZED BRUTE FORCE: Check voxels near ray path
    // This approach checks a limited region around the ray for voxels
    // It provides better performance than the DDA algorithm for sparse voxel grids
    if (true) { // Using optimized brute force for performance reasons
        // Check voxels along the ray path from start to end of workspace
        float startT = std::max(0.0f, tMin); // Start from ray origin if inside workspace, otherwise from entry point
        float maxCheckDistance = std::min(workspaceSize.x, 2.0f); // Limit to 2 meters max
        float endT = startT + maxCheckDistance;
        
        // Early termination when we find a hit
        bool foundHit = false;
        for (float t = startT; t <= endT && !foundHit; t += 0.1f) { // Check every 10cm for much better performance
            Math::WorldCoordinates checkPoint = ray.pointAt(t);
            Math::IncrementCoordinates checkIncrement = Math::CoordinateConverter::worldToIncrement(checkPoint);
            
            // For each point along the ray, check all nearby voxel positions that could intersect
            // The issue is that we need to check voxel positions that the ray passes through their volume,
            // not just the exact increment coordinate at the ray point
            
            // Calculate voxel size in centimeters
            int voxelSizeCm = static_cast<int>(voxelSize * Math::CoordinateConverter::METERS_TO_CM); // 16cm for Size_16cm resolution
            
            // Check voxel positions that could contain this world point
            // For 16cm voxels, check the voxel grid positions around this point
            std::vector<Math::IncrementCoordinates> candidatePositions;
            
            // Find the voxel grid positions that could contain this world point
            // For a 16cm voxel, snap to 16cm grid boundaries
            int baseX = (checkIncrement.x() / voxelSizeCm) * voxelSizeCm;
            int baseY = (checkIncrement.y() / voxelSizeCm) * voxelSizeCm;
            int baseZ = (checkIncrement.z() / voxelSizeCm) * voxelSizeCm;
            
            // Check nearby voxel positions that could contain this ray point
            // For non-aligned voxels, we need to search the voxel size range
            // Use adaptive search range based on voxel size for accuracy
            int searchRange = voxelSizeCm / 2; // Search half voxel size in each direction
            // Use adaptive step size based on voxel size for performance
            int stepSize = std::max(1, voxelSizeCm / 8); // At least 1cm steps, but scale with voxel size
            for (int dy = -searchRange; dy <= searchRange && !foundHit; dy += stepSize) {
                for (int dx = -searchRange; dx <= searchRange && !foundHit; dx += stepSize) {
                    for (int dz = -searchRange; dz <= searchRange && !foundHit; dz += stepSize) {
                        Math::IncrementCoordinates testIncrement(checkIncrement.x() + dx, checkIncrement.y() + dy, checkIncrement.z() + dz);
                        
                        // Check if this voxel exists
                        if (grid.isValidIncrementPosition(testIncrement) && grid.getVoxel(testIncrement)) {
                            // Found a voxel! Now check if the ray actually intersects its bounding box
                            Math::WorldCoordinates voxelWorld = Math::CoordinateConverter::incrementToWorld(testIncrement);
                            float halfSize = voxelSize * 0.5f;
                            
                            Math::Vector3f voxelMin(voxelWorld.x() - halfSize, voxelWorld.y(), voxelWorld.z() - halfSize);
                            Math::Vector3f voxelMax(voxelWorld.x() + halfSize, voxelWorld.y() + voxelSize, voxelWorld.z() + halfSize);
                            Math::BoundingBox voxelBox(voxelMin, voxelMax);
                            
                            float voxelTMin, voxelTMax;
                            if (rayIntersectsBox(ray, voxelBox, voxelTMin, voxelTMax)) {
                                // Ray intersects this voxel - this is our boundary hit!
                                // For rays starting inside voxel, use exit point (tMax), otherwise use entry point (tMin)
                                float hitT = (voxelTMin <= 0.0f) ? voxelTMax : voxelTMin;
                                FaceDirection hitFace = determineFaceDirection(ray, voxelBox, hitT);
                                
                                result.hit = true;
                                result.distance = hitT;
                                result.position = ray.pointAt(hitT);
                                result.face = Face(testIncrement, resolution, hitFace, result.position);
                                
                                // Set normal based on face direction
                                switch (hitFace) {
                                    case FaceDirection::PositiveX: result.normal = Math::Vector3f(1, 0, 0); break;
                                    case FaceDirection::NegativeX: result.normal = Math::Vector3f(-1, 0, 0); break;
                                    case FaceDirection::PositiveY: result.normal = Math::Vector3f(0, 1, 0); break;
                                    case FaceDirection::NegativeY: result.normal = Math::Vector3f(0, -1, 0); break;
                                    case FaceDirection::PositiveZ: result.normal = Math::Vector3f(0, 0, 1); break;
                                    case FaceDirection::NegativeZ: result.normal = Math::Vector3f(0, 0, -1); break;
                                }
                                
                                foundHit = true;
                                return result;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Traverse grid using DDA algorithm
    float maxDistance = m_maxRayDistance;
    float currentDistance = 0.0f;
    
    // Track the closest hit
    struct HitInfo {
        Math::Vector3i voxel;
        float distance;
        Math::Vector3f normal;
        Face face;
        bool valid = false;
    } closestHit;
    
    // Track if we start inside a voxel for special handling
    Math::IncrementCoordinates startVoxel = traversal.current;
    bool startedInsideVoxel = false;
    
    // Check if we start inside a voxel
    if (grid.isValidIncrementPosition(traversal.current)) {
        startedInsideVoxel = grid.getVoxel(traversal.current);
    }
    
    // Track the previous voxel for exit detection
    Math::IncrementCoordinates previousVoxel = traversal.current;
    bool wasInsideStartVoxel = startedInsideVoxel;
    
    int stepCount = 0;
    const int maxSteps = 10000; // Increased step limit for large workspaces
    while (currentDistance < maxDistance && stepCount < maxSteps) {
        // Check current voxel
        if (grid.isValidIncrementPosition(traversal.current)) {
            bool isCurrentVoxel = grid.getVoxel(traversal.current);
            
            // Only log key voxel hits to reduce performance impact
            // Commented out to prevent debug output during tests
            // if (isCurrentVoxel && stepCount < 5) {
            //     Logging::Logger::getInstance().debugfc("FaceDetector",
            //         "Found voxel at grid position (%d, %d, %d)",
            //         traversal.current.x(), traversal.current.y(), traversal.current.z());
            // }
            
            // Special case: ray started inside a voxel and we just exited it
            if (startedInsideVoxel && wasInsideStartVoxel && 
                (traversal.current.x() != startVoxel.x() || 
                 traversal.current.y() != startVoxel.y() || 
                 traversal.current.z() != startVoxel.z())) {
                
                
                // We just exited the starting voxel - determine which face
                FaceDirection exitFace;
                if (traversal.current.x() != previousVoxel.x()) {
                    exitFace = (traversal.current.x() > previousVoxel.x()) ? 
                        FaceDirection::PositiveX : FaceDirection::NegativeX;
                } else if (traversal.current.y() != previousVoxel.y()) {
                    exitFace = (traversal.current.y() > previousVoxel.y()) ? 
                        FaceDirection::PositiveY : FaceDirection::NegativeY;
                } else {
                    exitFace = (traversal.current.z() > previousVoxel.z()) ? 
                        FaceDirection::PositiveZ : FaceDirection::NegativeZ;
                }
                
                // Create face for the exit
                closestHit.valid = true;
                closestHit.voxel = previousVoxel.value();
                closestHit.distance = currentDistance;
                Math::WorldCoordinates exitHitPoint = ray.pointAt(currentDistance);
                closestHit.face = Face(previousVoxel, resolution, exitFace, exitHitPoint);
                
                // Set normal based on exit face
                switch (exitFace) {
                    case FaceDirection::PositiveX: closestHit.normal = Math::Vector3f(1, 0, 0); break;
                    case FaceDirection::NegativeX: closestHit.normal = Math::Vector3f(-1, 0, 0); break;
                    case FaceDirection::PositiveY: closestHit.normal = Math::Vector3f(0, 1, 0); break;
                    case FaceDirection::NegativeY: closestHit.normal = Math::Vector3f(0, -1, 0); break;
                    case FaceDirection::PositiveZ: closestHit.normal = Math::Vector3f(0, 0, 1); break;
                    case FaceDirection::NegativeZ: closestHit.normal = Math::Vector3f(0, 0, -1); break;
                }
                
                // Mark that we've handled the starting voxel
                wasInsideStartVoxel = false;
                
                // Return the exit face immediately, even if we hit another voxel
                break;
            }
            
            // Normal case: ray hits a voxel from outside
            if (isCurrentVoxel && !wasInsideStartVoxel) {
                // Convert voxel increment position to world coordinates (bottom-center)
                Math::WorldCoordinates voxelWorldPos = Math::CoordinateConverter::incrementToWorld(traversal.current);
                Math::Vector3f voxelBottomCenter = voxelWorldPos.value();
                
                // Calculate actual voxel bounding box from bottom-center position
                // The voxel extends from -halfSize to +halfSize in X and Z, and from 0 to voxelSize in Y
                float halfSize = voxelSize * 0.5f;
                Math::Vector3f voxelWorldMin = Math::Vector3f(
                    voxelBottomCenter.x - halfSize,
                    voxelBottomCenter.y,  // Y is already at bottom
                    voxelBottomCenter.z - halfSize
                );
                Math::Vector3f voxelWorldMax = Math::Vector3f(
                    voxelBottomCenter.x + halfSize,
                    voxelBottomCenter.y + voxelSize,
                    voxelBottomCenter.z + halfSize
                );
                
                // Calculate exact intersection with voxel box in world space
                Math::BoundingBox voxelBox(voxelWorldMin, voxelWorldMax);
                
                float tMin, tMax;
                if (rayIntersectsBox(ray, voxelBox, tMin, tMax)) {
                    // For rays starting inside voxel, use exit point (tMax), otherwise use entry point (tMin)
                    float hitT = (tMin <= 0.0f) ? tMax : tMin;
                    float hitDistance = hitT;
                    
                    // Only update if this is closer than any previous hit
                    if (!closestHit.valid || hitDistance < closestHit.distance) {
                        closestHit.valid = true;
                        closestHit.voxel = traversal.current.value();
                        closestHit.distance = hitDistance;
                        
                        // Calculate hit point to determine which face was hit
                        Math::Vector3f hitPoint = ray.origin.value() + ray.direction * hitT;
                        
                        // We already have voxelWorldMin and voxelWorldMax from above
                        
                        
                        // Use simplified face direction determination
                        FaceDirection hitFaceDir = determineFaceDirection(ray, voxelBox, hitT);
                        closestHit.face = Face(traversal.current, resolution, hitFaceDir, Math::WorldCoordinates(hitPoint));
                        
                        // Set normal based on face direction
                        switch (hitFaceDir) {
                            case FaceDirection::PositiveX: closestHit.normal = Math::Vector3f(1, 0, 0); break;
                            case FaceDirection::NegativeX: closestHit.normal = Math::Vector3f(-1, 0, 0); break;
                            case FaceDirection::PositiveY: closestHit.normal = Math::Vector3f(0, 1, 0); break;
                            case FaceDirection::NegativeY: closestHit.normal = Math::Vector3f(0, -1, 0); break;
                            case FaceDirection::PositiveZ: closestHit.normal = Math::Vector3f(0, 0, 1); break;
                            case FaceDirection::NegativeZ: closestHit.normal = Math::Vector3f(0, 0, -1); break;
                        }
                        
                        // Log key hit information
                        // Commented out to prevent debug output during tests
                        // if (stepCount < 3) {
                        //     Logging::Logger::getInstance().debugfc("FaceDetector",
                        //         "Hit voxel at (%d,%d,%d) face=%d",
                        //         traversal.current.x(), traversal.current.y(), traversal.current.z(),
                        //         static_cast<int>(closestHit.face.getDirection()));
                        // }
                    }
                }
            }
        }
        
        // Update tracking variables
        previousVoxel = traversal.current;
        
        // Step to next voxel
        stepTraversal(traversal);
        currentDistance = std::min({traversal.tMax.x, traversal.tMax.y, traversal.tMax.z});
        stepCount++;
    }
    
    // Return the closest hit if we found one
    if (closestHit.valid) {
        result.hit = true;
        result.distance = closestHit.distance;
        result.position = ray.pointAt(result.distance);
        result.normal = closestHit.normal;
        result.face = closestHit.face;
        
        // Return the closest hit we found
    }
    
    return result;
}

Face FaceDetector::createFaceFromHit(const RaycastHit& hit, VoxelData::VoxelResolution resolution) {
    return hit.face;
}

bool FaceDetector::isValidFace(const Face& face, const VoxelData::VoxelGrid& grid) const {
    // Check if the face's voxel exists
    return grid.getVoxel(face.getVoxelPosition());
}

bool FaceDetector::rayIntersectsBox(const Ray& ray, const Math::BoundingBox& box, 
                                   float& tMin, float& tMax) const {
    Math::Vector3f invDir(
        1.0f / (std::abs(ray.direction.x) < 0.0001f ? 0.0001f : ray.direction.x),
        1.0f / (std::abs(ray.direction.y) < 0.0001f ? 0.0001f : ray.direction.y),
        1.0f / (std::abs(ray.direction.z) < 0.0001f ? 0.0001f : ray.direction.z)
    );
    
    Math::Vector3f t1 = (box.min - ray.origin.value()) * invDir;
    Math::Vector3f t2 = (box.max - ray.origin.value()) * invDir;
    
    Math::Vector3f tMinVec(std::min(t1.x, t2.x), std::min(t1.y, t2.y), std::min(t1.z, t2.z));
    Math::Vector3f tMaxVec(std::max(t1.x, t2.x), std::max(t1.y, t2.y), std::max(t1.z, t2.z));
    
    tMin = std::max({tMinVec.x, tMinVec.y, tMinVec.z, 0.0f});
    tMax = std::min({tMaxVec.x, tMaxVec.y, tMaxVec.z});
    
    return tMin <= tMax && tMax >= 0.0f;
}

void FaceDetector::initializeTraversal(const Ray& ray, const Math::WorldCoordinates& gridMin, 
                                      float voxelSize, VoxelData::VoxelResolution resolution, GridTraversal& traversal) const {
    // Calculate voxel size in centimeters for increment coordinate stepping
    int voxelSizeCm = static_cast<int>(voxelSize * Math::CoordinateConverter::METERS_TO_CM);
    
    // Convert ray origin to increment coordinates and snap to voxel grid boundaries
    Math::IncrementCoordinates rayIncrement = Math::CoordinateConverter::worldToIncrement(ray.origin);
    
    // Snap to voxel grid boundaries by rounding to nearest voxel increment
    Math::Vector3i snappedIncrement(
        (rayIncrement.x() / voxelSizeCm) * voxelSizeCm,
        (rayIncrement.y() / voxelSizeCm) * voxelSizeCm,
        (rayIncrement.z() / voxelSizeCm) * voxelSizeCm
    );
    
    traversal.current = Math::IncrementCoordinates(snappedIncrement);
    
    // Step direction uses voxel-size increments, not 1cm increments
    traversal.step = Math::Vector3i(
        ray.direction.x > 0 ? voxelSizeCm : -voxelSizeCm,
        ray.direction.y > 0 ? voxelSizeCm : -voxelSizeCm,
        ray.direction.z > 0 ? voxelSizeCm : -voxelSizeCm
    );
    
    // Calculate tMax and tDelta using voxel size instead of 1cm
    const float epsilon = 0.0001f;
    
    // Convert current position to world coordinates for tMax calculation
    Math::WorldCoordinates currentWorld = Math::CoordinateConverter::incrementToWorld(traversal.current);
    
    if (std::abs(ray.direction.x) > epsilon) {
        float nextX = currentWorld.x() + (ray.direction.x > 0 ? voxelSize : -voxelSize);
        traversal.tMax.x = (nextX - ray.origin.x()) / ray.direction.x;
        traversal.tDelta.x = voxelSize / std::abs(ray.direction.x);
    } else {
        traversal.tMax.x = std::numeric_limits<float>::max();
        traversal.tDelta.x = std::numeric_limits<float>::max();
    }
    
    if (std::abs(ray.direction.y) > epsilon) {
        float nextY = currentWorld.y() + (ray.direction.y > 0 ? voxelSize : -voxelSize);
        traversal.tMax.y = (nextY - ray.origin.y()) / ray.direction.y;
        traversal.tDelta.y = voxelSize / std::abs(ray.direction.y);
    } else {
        traversal.tMax.y = std::numeric_limits<float>::max();
        traversal.tDelta.y = std::numeric_limits<float>::max();
    }
    
    if (std::abs(ray.direction.z) > epsilon) {
        float nextZ = currentWorld.z() + (ray.direction.z > 0 ? voxelSize : -voxelSize);
        traversal.tMax.z = (nextZ - ray.origin.z()) / ray.direction.z;
        traversal.tDelta.z = voxelSize / std::abs(ray.direction.z);
    } else {
        traversal.tMax.z = std::numeric_limits<float>::max();
        traversal.tDelta.z = std::numeric_limits<float>::max();
    }
}

void FaceDetector::stepTraversal(GridTraversal& traversal) const {
    if (traversal.tMax.x < traversal.tMax.y) {
        if (traversal.tMax.x < traversal.tMax.z) {
            traversal.current = Math::IncrementCoordinates(traversal.current.x() + traversal.step.x, traversal.current.y(), traversal.current.z());
            traversal.tMax.x += traversal.tDelta.x;
        } else {
            traversal.current = Math::IncrementCoordinates(traversal.current.x(), traversal.current.y(), traversal.current.z() + traversal.step.z);
            traversal.tMax.z += traversal.tDelta.z;
        }
    } else {
        if (traversal.tMax.y < traversal.tMax.z) {
            traversal.current = Math::IncrementCoordinates(traversal.current.x(), traversal.current.y() + traversal.step.y, traversal.current.z());
            traversal.tMax.y += traversal.tDelta.y;
        } else {
            traversal.current = Math::IncrementCoordinates(traversal.current.x(), traversal.current.y(), traversal.current.z() + traversal.step.z);
            traversal.tMax.z += traversal.tDelta.z;
        }
    }
}

// Additional methods for requirements tests
bool FaceDetector::isFaceVisible(const Face& face) const {
    // For simplicity, assume all faces are visible
    return face.isValid();
}

Rendering::Color FaceDetector::getFaceHighlightColor(const Face& face) const {
    // Return yellow highlight color
    return Rendering::Color(1.0f, 1.0f, 0.0f, 1.0f);
}

bool FaceDetector::validateFace(const Face& face, const VoxelData::VoxelGrid& grid) const {
    return isValidFace(face, grid);
}

bool FaceDetector::hasActiveHighlight() const {
    return m_hasActiveHighlight;
}

FaceDirection FaceDetector::determineFaceDirection(const Ray& ray, const Math::BoundingBox& voxelBox, float tMin) const {
    // Calculate the hit point on the voxel box
    Math::Vector3f hitPoint = ray.origin.value() + ray.direction * tMin;
    
    // Tolerance for determining which face was hit (smaller = more precise)
    const float epsilon = 0.01f;
    
    // Check distance from hit point to each face plane
    float distToMinX = std::abs(hitPoint.x - voxelBox.min.x);
    float distToMaxX = std::abs(hitPoint.x - voxelBox.max.x);
    float distToMinY = std::abs(hitPoint.y - voxelBox.min.y);
    float distToMaxY = std::abs(hitPoint.y - voxelBox.max.y);
    float distToMinZ = std::abs(hitPoint.z - voxelBox.min.z);
    float distToMaxZ = std::abs(hitPoint.z - voxelBox.max.z);
    
    // Find the minimum distance (which face we're closest to)
    float minDist = std::min({distToMinX, distToMaxX, distToMinY, distToMaxY, distToMinZ, distToMaxZ});
    
    // Return the face direction based on which plane we're closest to
    // Note: When ray hits a face, the face direction should match the face normal
    if (minDist == distToMinX) {
        return FaceDirection::NegativeX;  // Left face (normal points -X)
    } else if (minDist == distToMaxX) {
        return FaceDirection::PositiveX;  // Right face (normal points +X)
    } else if (minDist == distToMinY) {
        return FaceDirection::NegativeY;  // Bottom face (normal points -Y)
    } else if (minDist == distToMaxY) {
        return FaceDirection::PositiveY;  // Top face (normal points +Y)
    } else if (minDist == distToMinZ) {
        return FaceDirection::NegativeZ;  // Front face (normal points -Z)
    } else if (minDist == distToMaxZ) {
        return FaceDirection::PositiveZ;  // Back face (normal points +Z)
    }
    
    // Fallback: use ray direction to determine most likely face
    // This handles edge cases where hit point is not exactly on a face
    float absX = std::abs(ray.direction.x);
    float absY = std::abs(ray.direction.y);
    float absZ = std::abs(ray.direction.z);
    
    if (absX >= absY && absX >= absZ) {
        // X is dominant direction
        return (ray.direction.x > 0) ? FaceDirection::PositiveX : FaceDirection::NegativeX;
    } else if (absY >= absX && absY >= absZ) {
        // Y is dominant direction
        return (ray.direction.y > 0) ? FaceDirection::PositiveY : FaceDirection::NegativeY;
    } else {
        // Z is dominant direction
        return (ray.direction.z > 0) ? FaceDirection::PositiveZ : FaceDirection::NegativeZ;
    }
}

} // namespace VisualFeedback
} // namespace VoxelEditor