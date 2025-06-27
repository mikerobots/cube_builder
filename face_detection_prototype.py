#!/usr/bin/env python3
"""
Face Detection Algorithm Prototype
Validates ray-face intersection logic before porting to C++
"""

import numpy as np
from typing import List, Optional, Tuple, NamedTuple
from dataclasses import dataclass
import unittest


@dataclass
class Ray:
    """Ray defined by origin and direction"""
    origin: np.ndarray  # 3D point
    direction: np.ndarray  # 3D normalized vector
    
    def __post_init__(self):
        # Ensure direction is normalized
        self.direction = self.direction / np.linalg.norm(self.direction)
    
    def point_at(self, t: float) -> np.ndarray:
        """Get point along ray at parameter t"""
        return self.origin + t * self.direction


@dataclass
class Face:
    """A face defined by its vertices (assumes convex polygon)"""
    vertices: List[np.ndarray]  # List of 3D points
    normal: np.ndarray  # Face normal
    id: int  # Face identifier
    
    @classmethod
    def from_vertices(cls, vertices: List[np.ndarray], face_id: int) -> 'Face':
        """Create face and compute normal from vertices"""
        # Compute normal using first 3 vertices (assumes planar face)
        v0, v1, v2 = vertices[0], vertices[1], vertices[2]
        edge1 = v1 - v0
        edge2 = v2 - v0
        normal = np.cross(edge1, edge2)
        normal = normal / np.linalg.norm(normal)
        return cls(vertices, normal, face_id)


class RayFaceIntersection(NamedTuple):
    """Result of ray-face intersection"""
    hit: bool
    distance: float
    point: np.ndarray
    face_id: int


def ray_plane_intersection(ray: Ray, plane_point: np.ndarray, plane_normal: np.ndarray) -> Optional[float]:
    """
    Find intersection of ray with plane.
    Returns t parameter of intersection point, or None if no intersection.
    """
    # Check if ray is parallel to plane
    denom = np.dot(ray.direction, plane_normal)
    if abs(denom) < 1e-6:
        return None
    
    # Calculate intersection distance
    t = np.dot(plane_point - ray.origin, plane_normal) / denom
    
    # Check if intersection is behind ray origin
    if t < 0:
        return None
    
    return t


def point_in_convex_polygon(point: np.ndarray, vertices: List[np.ndarray], normal: np.ndarray) -> bool:
    """
    Check if a point lies inside a convex polygon using the cross product method.
    Point should already be on the plane of the polygon.
    """
    n = len(vertices)
    
    for i in range(n):
        v0 = vertices[i]
        v1 = vertices[(i + 1) % n]
        
        # Edge vector
        edge = v1 - v0
        
        # Vector from vertex to point
        to_point = point - v0
        
        # Cross product should point in same direction as normal for all edges
        cross = np.cross(edge, to_point)
        
        # Check if cross product is in opposite direction to normal
        if np.dot(cross, normal) < 0:
            return False
    
    return True


def ray_face_intersection(ray: Ray, face: Face) -> RayFaceIntersection:
    """
    Find intersection of ray with face.
    """
    # First, find intersection with face plane
    t = ray_plane_intersection(ray, face.vertices[0], face.normal)
    
    if t is None:
        return RayFaceIntersection(False, float('inf'), np.zeros(3), face.id)
    
    # Calculate intersection point
    intersection_point = ray.point_at(t)
    
    # Check if intersection point is inside face polygon
    if point_in_convex_polygon(intersection_point, face.vertices, face.normal):
        return RayFaceIntersection(True, t, intersection_point, face.id)
    
    return RayFaceIntersection(False, float('inf'), np.zeros(3), face.id)


def detect_closest_face(ray: Ray, faces: List[Face]) -> Optional[RayFaceIntersection]:
    """
    Find the closest face hit by the ray.
    """
    closest_hit = None
    min_distance = float('inf')
    
    for face in faces:
        hit = ray_face_intersection(ray, face)
        if hit.hit and hit.distance < min_distance:
            min_distance = hit.distance
            closest_hit = hit
    
    return closest_hit


def create_box_faces(center: np.ndarray, size: float) -> List[Face]:
    """
    Create the 6 faces of an axis-aligned box.
    Returns faces in order: +X, -X, +Y, -Y, +Z, -Z
    """
    half_size = size / 2.0
    
    # Create faces directly with their vertices
    faces = []
    
    # Face 0: +X face (right face, normal pointing in +X direction)
    faces.append(Face.from_vertices([
        center + np.array([half_size, -half_size, -half_size]),
        center + np.array([half_size, -half_size, half_size]),
        center + np.array([half_size, half_size, half_size]),
        center + np.array([half_size, half_size, -half_size])
    ], 0))
    
    # Face 1: -X face (left face, normal pointing in -X direction)
    faces.append(Face.from_vertices([
        center + np.array([-half_size, -half_size, -half_size]),
        center + np.array([-half_size, half_size, -half_size]),
        center + np.array([-half_size, half_size, half_size]),
        center + np.array([-half_size, -half_size, half_size])
    ], 1))
    
    # Face 2: +Y face (top face, normal pointing in +Y direction)
    faces.append(Face.from_vertices([
        center + np.array([-half_size, half_size, -half_size]),
        center + np.array([half_size, half_size, -half_size]),
        center + np.array([half_size, half_size, half_size]),
        center + np.array([-half_size, half_size, half_size])
    ], 2))
    
    # Face 3: -Y face (bottom face, normal pointing in -Y direction)
    faces.append(Face.from_vertices([
        center + np.array([-half_size, -half_size, -half_size]),
        center + np.array([-half_size, -half_size, half_size]),
        center + np.array([half_size, -half_size, half_size]),
        center + np.array([half_size, -half_size, -half_size])
    ], 3))
    
    # Face 4: +Z face (front face, normal pointing in +Z direction)
    faces.append(Face.from_vertices([
        center + np.array([-half_size, -half_size, half_size]),
        center + np.array([-half_size, half_size, half_size]),
        center + np.array([half_size, half_size, half_size]),
        center + np.array([half_size, -half_size, half_size])
    ], 4))
    
    # Face 5: -Z face (back face, normal pointing in -Z direction)
    faces.append(Face.from_vertices([
        center + np.array([-half_size, -half_size, -half_size]),
        center + np.array([half_size, -half_size, -half_size]),
        center + np.array([half_size, half_size, -half_size]),
        center + np.array([-half_size, half_size, -half_size])
    ], 5))
    
    return faces


class TestFaceDetection(unittest.TestCase):
    """Test cases for face detection algorithm"""
    
    def test_ray_hits_cube_face(self):
        """Test ray hitting different faces of a cube"""
        # Create a unit cube centered at (0, 0, 0)
        cube_faces = create_box_faces(np.array([0.0, 0.0, 0.0]), 1.0)
        
        # Test ray from -X hitting the -X face (face 1)
        ray = Ray(np.array([-2.0, 0.0, 0.0]), np.array([1.0, 0.0, 0.0]))
        hit = detect_closest_face(ray, cube_faces)
        self.assertIsNotNone(hit)
        self.assertTrue(hit.hit)
        self.assertEqual(hit.face_id, 1)  # -X face
        self.assertAlmostEqual(hit.distance, 1.5, places=5)
        
        # Test ray from +Y hitting the +Y face (face 2)
        ray = Ray(np.array([0.0, 2.0, 0.0]), np.array([0.0, -1.0, 0.0]))
        hit = detect_closest_face(ray, cube_faces)
        self.assertIsNotNone(hit)
        self.assertTrue(hit.hit)
        self.assertEqual(hit.face_id, 2)  # +Y face
        self.assertAlmostEqual(hit.distance, 1.5, places=5)
    
    def test_ray_misses_cube(self):
        """Test ray that misses the cube entirely"""
        cube_faces = create_box_faces(np.array([0.0, 0.0, 0.0]), 1.0)
        
        # Ray that goes above the cube
        ray = Ray(np.array([-2.0, 2.0, 0.0]), np.array([1.0, 0.0, 0.0]))
        hit = detect_closest_face(ray, cube_faces)
        self.assertIsNone(hit)
    
    def test_voxel_placement_scenario(self):
        """Test the actual voxel placement scenario from the failing test"""
        # The test says voxel is at increment position (32, 32, 32)
        # With 32cm resolution, this is world position (0.32, 0.32, 0.32)
        # BUT: Voxels are placed ON the ground plane with their bottom at Y coordinate
        voxel_size = 0.32
        
        # If voxel bottom is at Y=0.32, then center is at Y=0.32+0.16=0.48
        voxel_center = np.array([0.32, 0.32 + voxel_size/2, 0.32])
        
        voxel_faces = create_box_faces(voxel_center, voxel_size)
        
        # Test rays from the failing test
        test_cases = [
            # Ray from -X
            {"origin": np.array([-0.68, 0.48, 0.48]), "direction": np.array([1.0, 0.0, 0.0]), "expected_face": 1},
            # Ray from +X  
            {"origin": np.array([1.64, 0.48, 0.48]), "direction": np.array([-1.0, 0.0, 0.0]), "expected_face": 0},
            # Ray from -Y (below)
            {"origin": np.array([0.48, -0.68, 0.48]), "direction": np.array([0.0, 1.0, 0.0]), "expected_face": 3},
            # Ray from +Y (above)
            {"origin": np.array([0.48, 1.64, 0.48]), "direction": np.array([0.0, -1.0, 0.0]), "expected_face": 2},
            # Ray from -Z
            {"origin": np.array([0.48, 0.48, -0.68]), "direction": np.array([0.0, 0.0, 1.0]), "expected_face": 5},
            # Ray from +Z
            {"origin": np.array([0.48, 0.48, 1.64]), "direction": np.array([0.0, 0.0, -1.0]), "expected_face": 4},
        ]
        
        for i, test in enumerate(test_cases):
            ray = Ray(test["origin"], test["direction"])
            hit = detect_closest_face(ray, voxel_faces)
            
            print(f"\nTest case {i}: Ray from {test['origin']} in direction {test['direction']}")
            if hit:
                print(f"  Hit: {hit.hit}, Face: {hit.face_id}, Distance: {hit.distance:.3f}")
                print(f"  Hit point: {hit.point}")
            else:
                print("  No hit")
            
            # Verify hit
            self.assertIsNotNone(hit, f"Ray should hit voxel for test case {i}")
            self.assertTrue(hit.hit, f"Ray should hit voxel for test case {i}")
            self.assertEqual(hit.face_id, test["expected_face"], 
                           f"Wrong face hit for test case {i}: expected {test['expected_face']}, got {hit.face_id}")


def visualize_scenario():
    """Visualize the test scenario to understand the geometry"""
    print("=== Voxel Placement Scenario ===")
    print("Voxel at increment coordinates (32, 32, 32)")
    print("Resolution: 32cm")
    print("World coordinates: (0.32, 0.32, 0.32)")
    print("\nVoxels are placed with their BOTTOM at the Y coordinate")
    print("So actual voxel bounds:")
    print("  X: [0.16, 0.48]  <- Centered at 0.32")
    print("  Y: [0.32, 0.64]  <- Bottom at 0.32, top at 0.64")
    print("  Z: [0.16, 0.48]  <- Centered at 0.32")
    
    # Create the corrected voxel
    voxel_size = 0.32
    voxel_center = np.array([0.32, 0.32 + voxel_size/2, 0.32])  # Center at Y=0.48
    voxel_faces = create_box_faces(voxel_center, voxel_size)
    
    # Test a ray from below that should hit
    ray_origin = np.array([0.32, -1.0, 0.32])
    ray_direction = np.array([0.0, 1.0, 0.0])
    ray = Ray(ray_origin, ray_direction)
    
    hit = detect_closest_face(ray, voxel_faces)
    if hit and hit.hit:
        print(f"\nRay from below hits at Y={hit.point[1]:.3f}")
        print(f"Face ID: {hit.face_id} (should be bottom face)")


if __name__ == "__main__":
    # Run visualization first
    visualize_scenario()
    
    print("\n=== Running Tests ===")
    # Run tests
    unittest.main(verbosity=2)