#!/usr/bin/env python3
"""
Clean implementation of face detection with clear conventions.

CONVENTIONS:
1. Voxel placement: When a voxel is placed at increment coordinates (ix, iy, iz),
   the world position is calculated as:
   - world_x = ix * 0.01  (CENTER of voxel in X)
   - world_y = iy * 0.01  (BOTTOM of voxel in Y) 
   - world_z = iz * 0.01  (CENTER of voxel in Z)

2. Face ordering: Faces are indexed 0-5 in this order:
   0: +X face (right)
   1: -X face (left)
   2: +Y face (top)
   3: -Y face (bottom)
   4: +Z face (front)
   5: -Z face (back)

3. Vertex ordering: All faces use counter-clockwise winding when viewed from outside.
   This ensures normals point outward.
"""

import numpy as np
from typing import List, Tuple, Optional, Dict

# Face direction enum values (matching C++ FaceDirection enum)
FACE_POSITIVE_X = 0
FACE_NEGATIVE_X = 1
FACE_POSITIVE_Y = 2
FACE_NEGATIVE_Y = 3
FACE_POSITIVE_Z = 4
FACE_NEGATIVE_Z = 5

def create_voxel_faces_correct(world_pos: np.ndarray, voxel_size: float) -> List[Dict]:
    """
    Create voxel faces with correct interpretation of world position.
    
    Args:
        world_pos: World position where voxel is placed
                  X,Z are center coordinates, Y is bottom coordinate
        voxel_size: Size of the voxel
    
    Returns:
        List of face dictionaries with vertices, normal, and direction
    """
    # Calculate actual voxel center
    center = np.array([
        world_pos[0],                    # X is already center
        world_pos[1] + voxel_size / 2,   # Y needs adjustment (bottom -> center)
        world_pos[2]                     # Z is already center
    ])
    
    half = voxel_size / 2
    
    # Define the 8 corners using a consistent naming scheme
    corners = {
        'LLL': center + np.array([-half, -half, -half]),  # Left, Low, Low (back)
        'HLL': center + np.array([+half, -half, -half]),  # High, Low, Low (back)
        'HHL': center + np.array([+half, +half, -half]),  # High, High, Low (back)
        'LHL': center + np.array([-half, +half, -half]),  # Left, High, Low (back)
        'LLH': center + np.array([-half, -half, +half]),  # Left, Low, High (front)
        'HLH': center + np.array([+half, -half, +half]),  # High, Low, High (front)
        'HHH': center + np.array([+half, +half, +half]),  # High, High, High (front)
        'LHH': center + np.array([-half, +half, +half]),  # Left, High, High (front)
    }
    
    faces = []
    
    # Face 0: +X face (right face, x = center.x + half)
    vertices = [corners['HLH'], corners['HLL'], corners['HHL'], corners['HHH']]
    normal = calculate_normal(vertices)
    faces.append({
        'vertices': vertices,
        'normal': normal,
        'direction': FACE_POSITIVE_X,
        'name': '+X'
    })
    
    # Face 1: -X face (left face, x = center.x - half)
    vertices = [corners['LLL'], corners['LLH'], corners['LHH'], corners['LHL']]
    normal = calculate_normal(vertices)
    faces.append({
        'vertices': vertices,
        'normal': normal,
        'direction': FACE_NEGATIVE_X,
        'name': '-X'
    })
    
    # Face 2: +Y face (top face, y = center.y + half)
    vertices = [corners['LHH'], corners['HHH'], corners['HHL'], corners['LHL']]
    normal = calculate_normal(vertices)
    faces.append({
        'vertices': vertices,
        'normal': normal,
        'direction': FACE_POSITIVE_Y,
        'name': '+Y'
    })
    
    # Face 3: -Y face (bottom face, y = center.y - half)
    vertices = [corners['LLH'], corners['LLL'], corners['HLL'], corners['HLH']]
    normal = calculate_normal(vertices)
    faces.append({
        'vertices': vertices,
        'normal': normal,
        'direction': FACE_NEGATIVE_Y,
        'name': '-Y'
    })
    
    # Face 4: +Z face (front face, z = center.z + half)
    vertices = [corners['LLH'], corners['HLH'], corners['HHH'], corners['LHH']]
    normal = calculate_normal(vertices)
    faces.append({
        'vertices': vertices,
        'normal': normal,
        'direction': FACE_POSITIVE_Z,
        'name': '+Z'
    })
    
    # Face 5: -Z face (back face, z = center.z - half)
    vertices = [corners['HLL'], corners['LLL'], corners['LHL'], corners['HHL']]
    normal = calculate_normal(vertices)
    faces.append({
        'vertices': vertices,
        'normal': normal,
        'direction': FACE_NEGATIVE_Z,
        'name': '-Z'
    })
    
    return faces

def calculate_normal(vertices: List[np.ndarray]) -> np.ndarray:
    """Calculate face normal using right-hand rule."""
    v0, v1, v2 = vertices[0], vertices[1], vertices[2]
    edge1 = v1 - v0
    edge2 = v2 - v0
    normal = np.cross(edge1, edge2)
    return normal / np.linalg.norm(normal)

def test_specific_case():
    """Test the specific failing case from the C++ tests."""
    print("=== Testing Specific C++ Test Case ===\n")
    
    # The test places a voxel at increment (64, 64, 64)
    increment_pos = np.array([64, 64, 64])
    world_pos = increment_pos * 0.01  # Convert to meters
    voxel_size = 0.32
    
    print(f"Increment position: {increment_pos}")
    print(f"World position: {world_pos}")
    print(f"Voxel size: {voxel_size}m")
    
    # Calculate bounds
    center = np.array([world_pos[0], world_pos[1] + voxel_size/2, world_pos[2]])
    print(f"\nVoxel center: {center}")
    print(f"Voxel bounds:")
    print(f"  X: [{center[0] - voxel_size/2:.3f} to {center[0] + voxel_size/2:.3f}]")
    print(f"  Y: [{center[1] - voxel_size/2:.3f} to {center[1] + voxel_size/2:.3f}]")
    print(f"  Z: [{center[2] - voxel_size/2:.3f} to {center[2] + voxel_size/2:.3f}]")
    
    # Create faces
    faces = create_voxel_faces_correct(world_pos, voxel_size)
    
    # Verify normals
    print("\nFace normals:")
    expected_normals = {
        '+X': np.array([1, 0, 0]),
        '-X': np.array([-1, 0, 0]),
        '+Y': np.array([0, 1, 0]),
        '-Y': np.array([0, -1, 0]),
        '+Z': np.array([0, 0, 1]),
        '-Z': np.array([0, 0, -1])
    }
    
    for i, face in enumerate(faces):
        expected = expected_normals[face['name']]
        actual = face['normal']
        match = np.allclose(actual, expected, atol=1e-6)
        print(f"  Face {i} ({face['name']}): {actual} - {'✓' if match else '✗'}")
    
    # Test the failing ray case
    print("\n=== Ray Test (from C++ test) ===")
    # The test uses: voxelWorld + Vector3f(-0.01f, voxelSize/2, voxelSize/2)
    ray_origin = world_pos + np.array([-0.01, voxel_size/2, voxel_size/2])
    ray_dir = np.array([1.0, 0.0, 0.0])
    
    print(f"Ray origin: {ray_origin}")
    print(f"Ray direction: {ray_dir}")
    print(f"Ray X position: {ray_origin[0]:.3f}")
    print(f"Voxel -X face at: {center[0] - voxel_size/2:.3f}")
    print(f"Voxel +X face at: {center[0] + voxel_size/2:.3f}")
    
    if ray_origin[0] < center[0] - voxel_size/2:
        print("Ray starts OUTSIDE voxel (left of -X face)")
        print("Expected to hit: -X face")
    elif ray_origin[0] > center[0] + voxel_size/2:
        print("Ray starts OUTSIDE voxel (right of +X face)")
        print("Expected to hit: +X face when going -X")
    else:
        print("Ray starts INSIDE voxel!")
        print("Expected to hit: +X face when going +X")

if __name__ == "__main__":
    test_specific_case()