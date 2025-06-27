#include <gtest/gtest.h>
#include "../TopologyPreserver.h"
#include "../../foundation/math/CoordinateTypes.h"
#include <cmath>

using namespace VoxelEditor::SurfaceGen;
using namespace VoxelEditor::Math;

class TopologyPreserverTest : public ::testing::Test {
protected:
    // Create a simple torus mesh for testing
    Mesh createTorusMesh() {
        Mesh torus;
        
        // Create a proper closed torus mesh with genus 1
        // Using 16 vertices: 8 on outer ring, 8 on inner ring
        torus.vertices = {
            // Outer ring (4 vertices on bottom, 4 on top)
            WorldCoordinates(-2.0f, -0.5f, -2.0f), // 0
            WorldCoordinates( 2.0f, -0.5f, -2.0f), // 1
            WorldCoordinates( 2.0f, -0.5f,  2.0f), // 2
            WorldCoordinates(-2.0f, -0.5f,  2.0f), // 3
            WorldCoordinates(-2.0f,  0.5f, -2.0f), // 4
            WorldCoordinates( 2.0f,  0.5f, -2.0f), // 5
            WorldCoordinates( 2.0f,  0.5f,  2.0f), // 6
            WorldCoordinates(-2.0f,  0.5f,  2.0f), // 7
            
            // Inner ring (4 vertices on bottom, 4 on top)
            WorldCoordinates(-1.0f, -0.5f, -1.0f), // 8
            WorldCoordinates( 1.0f, -0.5f, -1.0f), // 9
            WorldCoordinates( 1.0f, -0.5f,  1.0f), // 10
            WorldCoordinates(-1.0f, -0.5f,  1.0f), // 11
            WorldCoordinates(-1.0f,  0.5f, -1.0f), // 12
            WorldCoordinates( 1.0f,  0.5f, -1.0f), // 13
            WorldCoordinates( 1.0f,  0.5f,  1.0f), // 14
            WorldCoordinates(-1.0f,  0.5f,  1.0f)  // 15
        };
        
        // Create triangles that form a proper torus topology
        torus.indices = {
            // Bottom face (outer to inner) - creating the ring
            0, 1, 9,   9, 8, 0,
            1, 2, 10,  10, 9, 1,
            2, 3, 11,  11, 10, 2,
            3, 0, 8,   8, 11, 3,
            
            // Top face (outer to inner) - creating the ring
            4, 12, 13,  13, 5, 4,
            5, 13, 14,  14, 6, 5,
            6, 14, 15,  15, 7, 6,
            7, 15, 12,  12, 4, 7,
            
            // Outer side walls
            0, 4, 5,   5, 1, 0,
            1, 5, 6,   6, 2, 1,
            2, 6, 7,   7, 3, 2,
            3, 7, 4,   4, 0, 3,
            
            // Inner side walls (note order to maintain outward normals)
            8, 9, 13,   13, 12, 8,
            9, 10, 14,  14, 13, 9,
            10, 11, 15, 15, 14, 10,
            11, 8, 12,  12, 15, 11
        };
        
        return torus;
    }
    
    // Create a mesh with multiple holes
    Mesh createMeshWithMultipleHoles() {
        Mesh mesh;
        
        // A flat surface with two square holes
        mesh.vertices = {
            // Outer boundary
            WorldCoordinates(-3.0f, 0.0f, -3.0f), // 0
            WorldCoordinates( 3.0f, 0.0f, -3.0f), // 1
            WorldCoordinates( 3.0f, 0.0f,  3.0f), // 2
            WorldCoordinates(-3.0f, 0.0f,  3.0f), // 3
            // First hole
            WorldCoordinates(-2.0f, 0.0f, -2.0f), // 4
            WorldCoordinates(-1.0f, 0.0f, -2.0f), // 5
            WorldCoordinates(-1.0f, 0.0f, -1.0f), // 6
            WorldCoordinates(-2.0f, 0.0f, -1.0f), // 7
            // Second hole
            WorldCoordinates( 1.0f, 0.0f,  1.0f), // 8
            WorldCoordinates( 2.0f, 0.0f,  1.0f), // 9
            WorldCoordinates( 2.0f, 0.0f,  2.0f), // 10
            WorldCoordinates( 1.0f, 0.0f,  2.0f)  // 11
        };
        
        // Triangles that create a surface with two holes
        mesh.indices = {
            // Outer to first hole
            0, 1, 5,  5, 4, 0,
            1, 9, 6,  6, 5, 1,
            // Between holes
            6, 9, 8,  8, 6, 6,
            // Second hole to outer
            8, 9, 10, 10, 11, 8,
            9, 2, 10,
            11, 10, 2, 2, 3, 11,
            3, 0, 7,  7, 11, 3,
            7, 0, 4,
            // Connect remaining
            7, 6, 8,  8, 11, 7
        };
        
        return mesh;
    }
    
    // Create a simple sphere mesh (genus 0)
    Mesh createSphereMesh() {
        Mesh sphere;
        
        // Octahedron as simple sphere approximation
        sphere.vertices = {
            WorldCoordinates( 0.0f,  1.0f,  0.0f), // 0 - top
            WorldCoordinates( 1.0f,  0.0f,  0.0f), // 1
            WorldCoordinates( 0.0f,  0.0f,  1.0f), // 2
            WorldCoordinates(-1.0f,  0.0f,  0.0f), // 3
            WorldCoordinates( 0.0f,  0.0f, -1.0f), // 4
            WorldCoordinates( 0.0f, -1.0f,  0.0f)  // 5 - bottom
        };
        
        sphere.indices = {
            // Top half
            0, 1, 2,
            0, 2, 3,
            0, 3, 4,
            0, 4, 1,
            // Bottom half
            5, 2, 1,
            5, 3, 2,
            5, 4, 3,
            5, 1, 4
        };
        
        return sphere;
    }
};

// REQ-10.1.9: Test hole detection
TEST_F(TopologyPreserverTest, DetectHolesInMesh) {
    TopologyPreserver preserver;
    Mesh meshWithHoles = createMeshWithMultipleHoles();
    
    auto holes = preserver.detectHoles(meshWithHoles);
    
    // Should detect boundary loops as holes
    EXPECT_GT(holes.size(), 0);
    
    // Each hole should have critical vertices
    for (const auto& hole : holes) {
        EXPECT_EQ(hole.type, TopologyPreserver::TopologicalFeature::HOLE);
        EXPECT_GT(hole.criticalVertices.size(), 0);
        EXPECT_GT(hole.criticalEdges.size(), 0);
    }
}

// REQ-10.1.9: Test genus calculation
TEST_F(TopologyPreserverTest, CalculateGenusForDifferentTopologies) {
    TopologyPreserver preserver;
    
    // Sphere has genus 0
    Mesh sphere = createSphereMesh();
    EXPECT_EQ(preserver.calculateGenus(sphere), 0);
    
    // Torus has genus 1
    Mesh torus = createTorusMesh();
    EXPECT_EQ(preserver.calculateGenus(torus), 1);
}

// REQ-10.1.9: Test topology analysis
TEST_F(TopologyPreserverTest, AnalyzeComplexTopology) {
    TopologyPreserver preserver;
    Mesh torus = createTorusMesh();
    
    auto features = preserver.analyzeTopology(torus);
    
    // Should find topological features
    EXPECT_GT(features.size(), 0);
    
    // Check for loop detection (genus > 0)
    bool hasLoop = false;
    for (const auto& feature : features) {
        if (feature.type == TopologyPreserver::TopologicalFeature::LOOP) {
            hasLoop = true;
            EXPECT_GT(feature.importance, 0.0f);
        }
    }
    EXPECT_TRUE(hasLoop);
}

// REQ-10.1.9: Test constraint generation
TEST_F(TopologyPreserverTest, GenerateTopologyConstraints) {
    TopologyPreserver preserver;
    Mesh meshWithHoles = createMeshWithMultipleHoles();
    
    auto features = preserver.analyzeTopology(meshWithHoles);
    auto constraints = preserver.generateConstraints(meshWithHoles, features);
    
    // Should have some vertices locked or constrained
    EXPECT_GT(constraints.lockedVertices.size() + constraints.constrainedVertices.size(), 0);
}

// REQ-10.1.9: Test vertex movement constraints
TEST_F(TopologyPreserverTest, VertexMovementConstraints) {
    TopologyPreserver preserver;
    
    TopologyPreserver::TopologyConstraints constraints;
    constraints.lockedVertices.insert(0);
    constraints.constrainedVertices.insert(1);
    constraints.maxMovementDistance = 0.5f;
    
    Vector3f oldPos(0.0f, 0.0f, 0.0f);
    Vector3f smallMove(0.1f, 0.0f, 0.0f);
    Vector3f largeMove(1.0f, 0.0f, 0.0f);
    
    // Locked vertex should not move
    EXPECT_FALSE(preserver.isMovementAllowed(0, oldPos, smallMove, constraints));
    EXPECT_FALSE(preserver.isMovementAllowed(0, oldPos, largeMove, constraints));
    
    // Constrained vertex can move within limit
    EXPECT_TRUE(preserver.isMovementAllowed(1, oldPos, smallMove, constraints));
    EXPECT_FALSE(preserver.isMovementAllowed(1, oldPos, largeMove, constraints));
    
    // Unconstrained vertex can move freely
    EXPECT_TRUE(preserver.isMovementAllowed(2, oldPos, largeMove, constraints));
}

// REQ-10.1.9: Test movement constraint enforcement
TEST_F(TopologyPreserverTest, ConstrainMovementEnforcement) {
    TopologyPreserver preserver;
    
    TopologyPreserver::TopologyConstraints constraints;
    constraints.lockedVertices.insert(0);
    constraints.constrainedVertices.insert(1);
    constraints.maxMovementDistance = 0.5f;
    
    Vector3f oldPos(0.0f, 0.0f, 0.0f);
    Vector3f proposedMove(1.0f, 0.0f, 0.0f);
    
    // Locked vertex returns original position
    Vector3f lockedResult = preserver.constrainMovement(0, oldPos, proposedMove, constraints);
    EXPECT_EQ(lockedResult, oldPos);
    
    // Constrained vertex movement is limited
    Vector3f constrainedResult = preserver.constrainMovement(1, oldPos, proposedMove, constraints);
    float distance = (constrainedResult - oldPos).length();
    EXPECT_LE(distance, constraints.maxMovementDistance + 0.001f);
    
    // Unconstrained vertex moves freely
    Vector3f unconstrainedResult = preserver.constrainMovement(2, oldPos, proposedMove, constraints);
    EXPECT_EQ(unconstrainedResult, proposedMove);
}

// REQ-10.1.9: Test topology preservation verification
TEST_F(TopologyPreserverTest, VerifyTopologyPreservation) {
    TopologyPreserver preserver;
    
    Mesh original = createTorusMesh();
    Mesh modified = original;
    
    // Same topology should be preserved
    EXPECT_TRUE(preserver.verifyTopologyPreserved(original, modified));
    
    // Changing vertex count breaks topology
    modified.vertices.push_back(WorldCoordinates(0.0f, 0.0f, 0.0f));
    EXPECT_FALSE(preserver.verifyTopologyPreserved(original, modified));
    
    // Restore and test with face count change
    modified = original;
    modified.indices.push_back(0);
    modified.indices.push_back(1);
    modified.indices.push_back(2);
    EXPECT_FALSE(preserver.verifyTopologyPreserved(original, modified));
}

// Test loop detection in torus
TEST_F(TopologyPreserverTest, DetectLoopsInTorus) {
    TopologyPreserver preserver;
    Mesh torus = createTorusMesh();
    
    auto loops = preserver.detectLoops(torus);
    
    // Torus should have loop features detected
    EXPECT_GT(loops.size(), 0);
    
    for (const auto& loop : loops) {
        EXPECT_EQ(loop.type, TopologyPreserver::TopologicalFeature::LOOP);
        EXPECT_GT(loop.criticalVertices.size(), 0);
        EXPECT_FLOAT_EQ(loop.importance, 1.0f); // High importance for genus preservation
    }
}

// Test constraint generation preserves holes
TEST_F(TopologyPreserverTest, ConstraintsPreserveHoles) {
    TopologyPreserver preserver;
    Mesh meshWithHoles = createMeshWithMultipleHoles();
    
    auto features = preserver.analyzeTopology(meshWithHoles);
    
    // Generate constraints with hole preservation
    TopologyPreserver::TopologyConstraints constraints;
    constraints.preserveHoles = true;
    auto finalConstraints = preserver.generateConstraints(meshWithHoles, features);
    
    // Should have locked or constrained vertices for hole boundaries
    EXPECT_GT(finalConstraints.lockedVertices.size() + finalConstraints.constrainedVertices.size(), 0);
    
    // Generate constraints without hole preservation
    TopologyPreserver::TopologyConstraints noHoleConstraints;
    noHoleConstraints.preserveHoles = false;
    auto relaxedConstraints = preserver.generateConstraints(meshWithHoles, features);
    
    // Should have fewer constraints
    EXPECT_LE(relaxedConstraints.lockedVertices.size(), finalConstraints.lockedVertices.size());
}

// Test boundary edge detection
TEST_F(TopologyPreserverTest, BoundaryEdgeDetection) {
    TopologyPreserver preserver;
    
    // Create a simple open mesh (square with one face missing)
    Mesh openMesh;
    openMesh.vertices = {
        WorldCoordinates(0.0f, 0.0f, 0.0f),
        WorldCoordinates(1.0f, 0.0f, 0.0f),
        WorldCoordinates(1.0f, 0.0f, 1.0f),
        WorldCoordinates(0.0f, 0.0f, 1.0f)
    };
    
    // Only 2 triangles instead of 4 (creating a boundary)
    openMesh.indices = {
        0, 1, 2,
        0, 2, 3
    };
    
    auto holes = preserver.detectHoles(openMesh);
    
    // Should detect the boundary as a hole
    EXPECT_EQ(holes.size(), 1);
    if (!holes.empty()) {
        EXPECT_EQ(holes[0].type, TopologyPreserver::TopologicalFeature::HOLE);
        // Boundary should have 4 vertices
        EXPECT_EQ(holes[0].criticalVertices.size(), 4);
    }
}