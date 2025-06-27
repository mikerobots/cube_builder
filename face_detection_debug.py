#!/usr/bin/env python3
"""
Debug script for face detection issues.
Let's understand the coordinate system and face ordering clearly.
"""

import numpy as np
from typing import List, Tuple, Optional
# import matplotlib.pyplot as plt
# from mpl_toolkits.mplot3d import Axes3D
# from mpl_toolkits.mplot3d.art3d import Poly3DCollection

class Face:
    """A face defined by vertices, with automatic normal calculation."""
    def __init__(self, vertices: List[np.ndarray], face_id: int, name: str):
        self.vertices = vertices
        self.id = face_id
        self.name = name
        
        # Calculate normal using right-hand rule
        # Normal = (v1 - v0) x (v2 - v0)
        v0, v1, v2 = vertices[0], vertices[1], vertices[2]
        edge1 = v1 - v0
        edge2 = v2 - v0
        self.normal = np.cross(edge1, edge2)
        self.normal = self.normal / np.linalg.norm(self.normal)
        
        # Calculate center
        self.center = np.mean(vertices, axis=0)

def create_box_faces(center: np.ndarray, size: float) -> List[Face]:
    """
    Create box faces with CONSISTENT vertex ordering.
    
    RULE: All faces use counter-clockwise winding when viewed from outside.
    This ensures normals point outward.
    """
    half = size / 2
    
    # Define the 8 corners of the box
    # Using a naming convention: xyz where each is L(ow) or H(igh)
    corners = {
        'LLL': center + np.array([-half, -half, -half]),
        'HLL': center + np.array([+half, -half, -half]),
        'HHL': center + np.array([+half, +half, -half]),
        'LHL': center + np.array([-half, +half, -half]),
        'LLH': center + np.array([-half, -half, +half]),
        'HLH': center + np.array([+half, -half, +half]),
        'HHH': center + np.array([+half, +half, +half]),
        'LHH': center + np.array([-half, +half, +half]),
    }
    
    faces = []
    
    # Face 0: +X face (right face, x = center.x + half)
    # When looking from +X toward -X, we see this face
    # Counter-clockwise from outside: HLH -> HLL -> HHL -> HHH
    faces.append(Face([
        corners['HLH'], corners['HLL'], corners['HHL'], corners['HHH']
    ], 0, "+X"))
    
    # Face 1: -X face (left face, x = center.x - half)
    # When looking from -X toward +X, we see this face
    # Counter-clockwise from outside: LLL -> LLH -> LHH -> LHL
    faces.append(Face([
        corners['LLL'], corners['LLH'], corners['LHH'], corners['LHL']
    ], 1, "-X"))
    
    # Face 2: +Y face (top face, y = center.y + half)
    # When looking from +Y toward -Y, we see this face
    # Counter-clockwise from outside: LHH -> HHH -> HHL -> LHL
    faces.append(Face([
        corners['LHH'], corners['HHH'], corners['HHL'], corners['LHL']
    ], 2, "+Y"))
    
    # Face 3: -Y face (bottom face, y = center.y - half)
    # When looking from -Y toward +Y, we see this face
    # Counter-clockwise from outside: LLH -> LLL -> HLL -> HLH
    faces.append(Face([
        corners['LLH'], corners['LLL'], corners['HLL'], corners['HLH']
    ], 3, "-Y"))
    
    # Face 4: +Z face (front face, z = center.z + half)
    # When looking from +Z toward -Z, we see this face
    # Counter-clockwise from outside: LLH -> HLH -> HHH -> LHH
    faces.append(Face([
        corners['LLH'], corners['HLH'], corners['HHH'], corners['LHH']
    ], 4, "+Z"))
    
    # Face 5: -Z face (back face, z = center.z - half)
    # When looking from -Z toward +Z, we see this face
    # Counter-clockwise from outside: HLL -> LLL -> LHL -> HHL
    faces.append(Face([
        corners['HLL'], corners['LLL'], corners['LHL'], corners['HHL']
    ], 5, "-Z"))
    
    return faces

def ray_plane_intersection(ray_origin: np.ndarray, ray_dir: np.ndarray, 
                         plane_point: np.ndarray, plane_normal: np.ndarray) -> Optional[float]:
    """Find intersection of ray with plane."""
    denom = np.dot(ray_dir, plane_normal)
    if abs(denom) < 1e-6:
        return None
    
    t = np.dot(plane_point - ray_origin, plane_normal) / denom
    if t < 0:
        return None
    
    return t

def point_in_polygon(point: np.ndarray, vertices: List[np.ndarray], normal: np.ndarray) -> bool:
    """Check if point is inside convex polygon using cross products."""
    n = len(vertices)
    for i in range(n):
        j = (i + 1) % n
        edge = vertices[j] - vertices[i]
        to_point = point - vertices[i]
        cross = np.cross(edge, to_point)
        
        # Check if cross product aligns with normal
        if np.dot(cross, normal) < 0:
            return False
    
    return True

def ray_face_intersection(ray_origin: np.ndarray, ray_dir: np.ndarray, face: Face) -> Optional[Tuple[float, np.ndarray]]:
    """Find intersection of ray with face."""
    t = ray_plane_intersection(ray_origin, ray_dir, face.center, face.normal)
    if t is None:
        return None
    
    hit_point = ray_origin + t * ray_dir
    
    if point_in_polygon(hit_point, face.vertices, face.normal):
        return (t, hit_point)
    
    return None

def test_voxel_face_detection():
    """Test the specific case that's failing."""
    print("=== Testing Voxel Face Detection ===\n")
    
    # Voxel at increment position (64, 64, 64)
    # World position interpretation:
    # - X and Z are CENTER coordinates
    # - Y is BOTTOM coordinate
    voxel_world_pos = np.array([0.64, 0.64, 0.64])
    voxel_size = 0.32
    
    # Calculate actual voxel center
    voxel_center = np.array([
        voxel_world_pos[0],                    # X is already center
        voxel_world_pos[1] + voxel_size / 2,   # Y needs adjustment
        voxel_world_pos[2]                     # Z is already center
    ])
    
    print(f"Voxel world position: {voxel_world_pos}")
    print(f"Voxel center: {voxel_center}")
    print(f"Voxel size: {voxel_size}")
    print(f"Voxel bounds:")
    print(f"  X: [{voxel_center[0] - voxel_size/2:.2f} to {voxel_center[0] + voxel_size/2:.2f}]")
    print(f"  Y: [{voxel_center[1] - voxel_size/2:.2f} to {voxel_center[1] + voxel_size/2:.2f}]")
    print(f"  Z: [{voxel_center[2] - voxel_size/2:.2f} to {voxel_center[2] + voxel_size/2:.2f}]")
    
    # Create faces
    faces = create_box_faces(voxel_center, voxel_size)
    
    # Verify face positions and normals
    print("\nFace positions and normals:")
    for face in faces:
        print(f"  Face {face.id} ({face.name}): center={face.center}, normal={face.normal}")
    
    # Test case: Ray starting just outside -X face
    # WAIT! If -X face is at 0.48, then 0.63 is INSIDE the voxel!
    # To be outside -X face, we need X < 0.48
    ray_origin = np.array([0.47, 0.8, 0.8])  # X=0.47 is just outside the -X face at X=0.48
    ray_dir = np.array([1.0, 0.0, 0.0])      # Pointing in +X direction
    
    print(f"\nRay test:")
    print(f"  Origin: {ray_origin}")
    print(f"  Direction: {ray_dir}")
    print(f"  Expected to hit: -X face (face 1)")
    
    # Test intersection with all faces
    closest_hit = None
    closest_distance = float('inf')
    
    for face in faces:
        result = ray_face_intersection(ray_origin, ray_dir, face)
        if result:
            distance, hit_point = result
            print(f"\n  Hit face {face.id} ({face.name}):")
            print(f"    Distance: {distance:.3f}")
            print(f"    Hit point: {hit_point}")
            
            if distance < closest_distance:
                closest_distance = distance
                closest_hit = (face, distance, hit_point)
    
    if closest_hit:
        face, distance, hit_point = closest_hit
        print(f"\nClosest hit: Face {face.id} ({face.name}) at distance {distance:.3f}")
    else:
        print("\nNo face hit!")

def visualize_box_and_ray():
    """Visualize the box and ray to understand the geometry."""
    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, projection='3d')
    
    # Voxel parameters
    voxel_world_pos = np.array([0.64, 0.64, 0.64])
    voxel_size = 0.32
    voxel_center = np.array([
        voxel_world_pos[0],
        voxel_world_pos[1] + voxel_size / 2,
        voxel_world_pos[2]
    ])
    
    # Create faces
    faces = create_box_faces(voxel_center, voxel_size)
    
    # Draw faces
    for face in faces:
        verts = [face.vertices]
        poly = Poly3DCollection(verts, alpha=0.3, edgecolor='black')
        if '-X' in face.name:
            poly.set_facecolor('red')
        elif '+X' in face.name:
            poly.set_facecolor('blue')
        else:
            poly.set_facecolor('gray')
        ax.add_collection3d(poly)
        
        # Draw normal
        normal_start = face.center
        normal_end = face.center + 0.1 * face.normal
        ax.plot([normal_start[0], normal_end[0]], 
                [normal_start[1], normal_end[1]], 
                [normal_start[2], normal_end[2]], 'g-', linewidth=2)
    
    # Draw ray
    ray_origin = np.array([0.63, 0.8, 0.8])
    ray_dir = np.array([1.0, 0.0, 0.0])
    ray_end = ray_origin + 0.5 * ray_dir
    
    ax.plot([ray_origin[0], ray_end[0]], 
            [ray_origin[1], ray_end[1]], 
            [ray_origin[2], ray_end[2]], 'r-', linewidth=3, label='Ray')
    ax.scatter(*ray_origin, color='red', s=100, label='Ray origin')
    
    # Labels
    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_zlabel('Z')
    ax.set_title('Voxel Face Detection Visualization')
    ax.legend()
    
    # Set equal aspect ratio
    ax.set_box_aspect([1,1,1])
    
    plt.tight_layout()
    plt.savefig('voxel_face_debug.png', dpi=150)
    print("\nVisualization saved to voxel_face_debug.png")

if __name__ == "__main__":
    test_voxel_face_detection()
    # visualize_box_and_ray()