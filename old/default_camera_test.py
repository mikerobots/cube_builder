#!/usr/bin/env python3
"""
Test to validate that the default camera position can see all voxels in a 5x5x5 room.
This test performs comprehensive validation of:
1. Camera field of view coverage
2. Camera viewing angles (yaw/pitch)
3. Voxel visibility from edge conditions
4. Optimal camera positioning

COORDINATE SYSTEM ASSUMPTIONS:
==================================
1. WORLD COORDINATE SYSTEM (Right-handed):
   - Origin: (0, 0, 0) at the corner of the workspace
   - X-axis: Points right (positive X increases to the right)
   - Y-axis: Points up (positive Y increases upward)
   - Z-axis: Points forward/into screen (positive Z increases away from viewer)
   - Units: Meters

2. VOXEL GRID COORDINATES:
   - Origin: Grid position (0, 0, 0) maps to world origin
   - Integer grid coordinates: (gridX, gridY, gridZ)
   - Grid-to-world conversion: worldPos = gridPos * voxelSize
   - Voxel center offset: Add voxelSize/2 to get center of voxel
   - Example: Grid (0,0,0) with 8cm voxels -> World center at (0.04, 0.04, 0.04)

3. CAMERA COORDINATE SYSTEM:
   - Orbit camera uses spherical coordinates (yaw, pitch, distance)
   - Yaw: Rotation around Y-axis (0° = looking along +Z, 90° = looking along -X)
   - Pitch: Elevation angle from XZ plane (0° = horizontal, 90° = looking down)
   - Distance: Distance from camera to target point
   - Camera looks at target using right-handed look-at matrix

4. NORMALIZED DEVICE COORDINATES (NDC):
   - After projection, coordinates are in [-1, 1] range
   - X: -1 = left edge, +1 = right edge
   - Y: -1 = bottom edge, +1 = top edge  
   - Z: -1 = near plane, +1 = far plane (after perspective divide)

These assumptions match the C++ implementation in:
- VoxelGrid::gridToWorld() - adds 0.5 voxelSize offset for center
- OrbitCamera::updateCameraPosition() - spherical to Cartesian conversion
- VoxelMeshGenerator::generateCubeMesh() - world position calculation
"""

import math
import numpy as np

# Constants from the voxel editor
VOXEL_SIZE_8CM = 0.08  # meters (matching VoxelResolution::Size_8cm)
WORKSPACE_SIZE = 5.0   # meters (matching default workspace size)

# Camera parameters (matching OrbitCamera defaults)
CAMERA_DISTANCE = 5.0  # meters (OrbitCamera::m_distance default)
CAMERA_YAW = 45.0      # degrees (OrbitCamera yaw for isometric view)
CAMERA_PITCH = 30.0    # degrees (OrbitCamera pitch for isometric view)
CAMERA_FOV = 45.0      # degrees (Camera::m_fov default)
CAMERA_NEAR = 0.1      # meters (Camera::m_nearPlane default)
CAMERA_FAR = 100.0     # meters (Camera::m_farPlane default)
ASPECT_RATIO = 1280.0 / 720.0  # Window dimensions from RenderWindow

def degrees_to_radians(degrees):
    return degrees * math.pi / 180.0

def get_voxel_world_position(grid_x, grid_y, grid_z, resolution=VOXEL_SIZE_8CM):
    """
    Convert grid coordinates to world position (center of voxel).
    
    This matches VoxelMeshGenerator::generateCubeMesh() which calculates:
    worldPos.x = gridPos.x * voxelSize + voxelSize * 0.5f
    
    Args:
        grid_x, grid_y, grid_z: Integer voxel grid coordinates (0-based)
        resolution: Size of each voxel in meters
    
    Returns:
        World position at the CENTER of the voxel
    """
    world_x = (grid_x + 0.5) * resolution
    world_y = (grid_y + 0.5) * resolution
    world_z = (grid_z + 0.5) * resolution
    return np.array([world_x, world_y, world_z])

def get_camera_position(yaw, pitch, distance, target=np.array([0.0, 0.0, 0.0])):
    """
    Calculate camera position from spherical coordinates.
    
    This matches OrbitCamera::updateCameraPosition() which uses:
    - Yaw: rotation around Y-axis (0° = looking along +Z)
    - Pitch: elevation from XZ plane (positive = above horizon)
    - Distance: radius from target
    
    The conversion follows right-handed coordinate system where:
    - X = right
    - Y = up  
    - Z = forward (into screen)
    
    Args:
        yaw: Horizontal angle in degrees (0° = +Z direction)
        pitch: Vertical angle in degrees (0° = horizontal)
        distance: Distance from target in meters
        target: Point the camera looks at
    
    Returns:
        Camera position in world coordinates
    """
    yaw_rad = degrees_to_radians(yaw)
    pitch_rad = degrees_to_radians(pitch)
    
    # Convert spherical to Cartesian (matching OrbitCamera)
    x = distance * math.cos(pitch_rad) * math.sin(yaw_rad)
    y = distance * math.sin(pitch_rad)
    z = distance * math.cos(pitch_rad) * math.cos(yaw_rad)
    
    return target + np.array([x, y, z])

def look_at_matrix(eye, target, up=np.array([0.0, 1.0, 0.0])):
    """
    Create view matrix using right-handed look-at algorithm.
    
    This creates a view matrix that transforms world coordinates to view space.
    The camera looks from 'eye' position towards 'target' with 'up' orientation.
    
    In view space:
    - X-axis points right
    - Y-axis points up
    - Z-axis points backwards (opposite of view direction)
    
    This matches the OpenGL convention used in the C++ codebase.
    """
    forward = target - eye
    forward = forward / np.linalg.norm(forward)
    
    right = np.cross(forward, up)
    right = right / np.linalg.norm(right)
    
    up = np.cross(right, forward)
    
    view = np.eye(4)
    view[0, :3] = right
    view[1, :3] = up
    view[2, :3] = -forward  # Negative because camera looks down -Z in view space
    
    view[0, 3] = -np.dot(right, eye)
    view[1, 3] = -np.dot(up, eye)
    view[2, 3] = np.dot(forward, eye)
    
    return view

def perspective_matrix(fov, aspect, near, far):
    """
    Create perspective projection matrix.
    
    This matches the OpenGL perspective projection used in Camera::updateProjectionMatrix().
    Maps view space coordinates to clip space coordinates.
    
    After perspective divide (w-division), coordinates are in NDC:
    - X: [-1, 1] where -1 is left, +1 is right
    - Y: [-1, 1] where -1 is bottom, +1 is top
    - Z: [-1, 1] where -1 is near, +1 is far
    
    Args:
        fov: Vertical field of view in degrees
        aspect: Width/height aspect ratio
        near: Near clipping plane distance (meters)
        far: Far clipping plane distance (meters)
    """
    fov_rad = degrees_to_radians(fov)
    f = 1.0 / math.tan(fov_rad / 2.0)
    
    proj = np.zeros((4, 4))
    proj[0, 0] = f / aspect
    proj[1, 1] = f
    proj[2, 2] = (far + near) / (near - far)
    proj[2, 3] = (2 * far * near) / (near - far)
    proj[3, 2] = -1.0
    
    return proj

def transform_to_ndc(world_pos, view_matrix, proj_matrix):
    """Transform world position to normalized device coordinates"""
    # Convert to homogeneous coordinates
    world_pos_h = np.append(world_pos, 1.0)
    
    # Apply view transformation
    view_pos = view_matrix @ world_pos_h
    
    # Apply projection transformation
    clip_pos = proj_matrix @ view_pos
    
    # Perspective divide to get NDC
    if clip_pos[3] != 0:
        ndc = clip_pos[:3] / clip_pos[3]
    else:
        ndc = clip_pos[:3]
    
    return ndc, clip_pos

def is_in_frustum(ndc):
    """Check if NDC coordinates are within the view frustum"""
    return (-1.0 <= ndc[0] <= 1.0 and 
            -1.0 <= ndc[1] <= 1.0 and 
            -1.0 <= ndc[2] <= 1.0)

def validate_field_of_view(fov, distance, workspace_size):
    """Validate that the field of view can encompass the workspace"""
    print(f"\n=== FIELD OF VIEW VALIDATION ===")
    print(f"FOV: {fov}° (half-angle: {fov/2}°)")
    print(f"Camera distance: {distance} m")
    print(f"Workspace size: {workspace_size} m")
    
    # Calculate the visible width/height at the target distance
    fov_rad = degrees_to_radians(fov)
    visible_size = 2 * distance * math.tan(fov_rad / 2)
    
    print(f"Visible size at target distance: {visible_size:.3f} m")
    print(f"Required coverage: {workspace_size * VOXEL_SIZE_8CM:.3f} m")
    
    coverage_ratio = visible_size / (workspace_size * VOXEL_SIZE_8CM)
    print(f"Coverage ratio: {coverage_ratio:.2f}x")
    
    if coverage_ratio >= 1.2:  # Want at least 20% margin
        print("✓ FOV provides adequate coverage with margin")
        return True
    else:
        print("✗ FOV may be too narrow for comfortable viewing")
        return False

def validate_camera_angles(yaw, pitch):
    """Validate camera viewing angles"""
    print(f"\n=== CAMERA ANGLE VALIDATION ===")
    print(f"Yaw angle: {yaw}° (rotation around Y-axis)")
    print(f"Pitch angle: {pitch}° (elevation from horizontal)")
    
    # Check if angles provide good 3D perspective
    isometric_pitch = math.degrees(math.atan(1/math.sqrt(2)))  # ~35.264°
    
    print(f"\nAngle analysis:")
    print(f"  Yaw {yaw}°: ", end="")
    if 30 <= yaw <= 60:
        print("✓ Good for 3D perspective")
    else:
        print("✗ May not provide good 3D view")
    
    print(f"  Pitch {pitch}°: ", end="")
    if 20 <= pitch <= 45:
        print("✓ Good elevation angle")
        if abs(pitch - isometric_pitch) < 5:
            print(f"    (Near isometric: {isometric_pitch:.1f}°)")
    else:
        print("✗ Too steep or too shallow")
    
    return 30 <= yaw <= 60 and 20 <= pitch <= 45

def test_camera_configuration(distance, yaw, pitch, target):
    """Test if camera configuration can see all voxels"""
    print(f"\n=== CAMERA CONFIGURATION TEST ===")
    print(f"Configuration:")
    print(f"  Distance: {distance:.2f} m")
    print(f"  Yaw: {yaw}°")
    print(f"  Pitch: {pitch}°")
    print(f"  Target: [{target[0]:.3f}, {target[1]:.3f}, {target[2]:.3f}]")
    
    # Step 1: Calculate camera position
    camera_pos = get_camera_position(yaw, pitch, distance, target)
    print(f"\nStep 1: Camera position calculation")
    print(f"  Camera world position: [{camera_pos[0]:.3f}, {camera_pos[1]:.3f}, {camera_pos[2]:.3f}]")
    
    # Verify camera is looking at target
    look_dir = target - camera_pos
    look_dir = look_dir / np.linalg.norm(look_dir)
    print(f"  Look direction: [{look_dir[0]:.3f}, {look_dir[1]:.3f}, {look_dir[2]:.3f}]")
    
    # Step 2: Create view and projection matrices
    print(f"\nStep 2: Matrix creation")
    view_matrix = look_at_matrix(camera_pos, target)
    proj_matrix = perspective_matrix(CAMERA_FOV, ASPECT_RATIO, CAMERA_NEAR, CAMERA_FAR)
    
    # Verify view matrix transforms target to origin
    target_view = view_matrix @ np.append(target, 1.0)
    print(f"  Target in view space: [{target_view[0]:.3f}, {target_view[1]:.3f}, {target_view[2]:.3f}]")
    print(f"  (Should be near [0, 0, -distance])")
    
    # Step 3: Validate frustum boundaries
    print(f"\nStep 3: Frustum validation")
    near_height = 2 * CAMERA_NEAR * math.tan(degrees_to_radians(CAMERA_FOV) / 2)
    near_width = near_height * ASPECT_RATIO
    far_height = 2 * CAMERA_FAR * math.tan(degrees_to_radians(CAMERA_FOV) / 2)
    far_width = far_height * ASPECT_RATIO
    print(f"  Near plane: {CAMERA_NEAR}m, size: {near_width:.3f}x{near_height:.3f}m")
    print(f"  Far plane: {CAMERA_FAR}m, size: {far_width:.3f}x{far_height:.3f}m")
    
    # Step 4: Test edge voxels in 5x5x5 grid
    print(f"\nStep 4: Edge voxel visibility test")
    print(f"Testing voxels at grid boundaries to ensure complete coverage")
    
    edge_voxels = [
        # Corners - most extreme positions
        (0, 0, 0),     # Near bottom left
        (4, 0, 0),     # Near bottom right
        (0, 4, 0),     # Near top left
        (4, 4, 0),     # Near top right
        (0, 0, 4),     # Far bottom left
        (4, 0, 4),     # Far bottom right
        (0, 4, 4),     # Far top left
        (4, 4, 4),     # Far top right
        # Center voxel
        (2, 2, 2),     # Center
        # Edge centers
        (2, 0, 0),     # Bottom center near
        (2, 4, 0),     # Top center near
        (0, 2, 0),     # Left center near
        (4, 2, 0),     # Right center near
    ]
    
    visible_count = 0
    total_count = len(edge_voxels)
    min_ndc = np.array([float('inf'), float('inf'), float('inf')])
    max_ndc = np.array([float('-inf'), float('-inf'), float('-inf')])
    
    print("\n  Grid -> World -> View -> Clip -> NDC coordinates:")
    for grid_x, grid_y, grid_z in edge_voxels:
        world_pos = get_voxel_world_position(grid_x, grid_y, grid_z)
        ndc, clip_pos = transform_to_ndc(world_pos, view_matrix, proj_matrix)
        in_frustum = is_in_frustum(ndc)
        
        # Track NDC bounds
        min_ndc = np.minimum(min_ndc, ndc)
        max_ndc = np.maximum(max_ndc, ndc)
        
        if in_frustum:
            visible_count += 1
            status = "✓"
        else:
            status = "✗"
        
        print(f"  {status} Voxel({grid_x},{grid_y},{grid_z}) -> "
              f"W[{world_pos[0]:.2f},{world_pos[1]:.2f},{world_pos[2]:.2f}] -> "
              f"NDC[{ndc[0]:+.3f},{ndc[1]:+.3f},{ndc[2]:+.3f}]")
    
    print(f"\nStep 5: Coverage analysis")
    print(f"  Visible voxels: {visible_count}/{total_count}")
    print(f"  NDC bounds: X[{min_ndc[0]:.3f}, {max_ndc[0]:.3f}] "
          f"Y[{min_ndc[1]:.3f}, {max_ndc[1]:.3f}] "
          f"Z[{min_ndc[2]:.3f}, {max_ndc[2]:.3f}]")
    
    # Calculate screen coverage
    screen_coverage_x = (max_ndc[0] - min_ndc[0]) / 2.0 * 100
    screen_coverage_y = (max_ndc[1] - min_ndc[1]) / 2.0 * 100
    print(f"  Screen coverage: {screen_coverage_x:.1f}% x {screen_coverage_y:.1f}%")
    
    if visible_count == total_count:
        print("  ✓ All edge voxels visible - configuration is valid!")
    else:
        print("  ✗ Some voxels not visible - configuration needs adjustment")
    
    return visible_count == total_count

def find_optimal_camera_target():
    """Find the center of the 5x5x5 voxel grid in world coordinates"""
    # Grid is 5x5x5, so center is at grid position (2, 2, 2)
    # But we want the center of the entire volume
    grid_size = 5
    center_voxel = (grid_size - 1) / 2.0  # 2.0 for 5x5x5
    
    # World position of grid center
    world_center_x = (center_voxel + 0.5) * VOXEL_SIZE_8CM
    world_center_y = (center_voxel + 0.5) * VOXEL_SIZE_8CM
    world_center_z = (center_voxel + 0.5) * VOXEL_SIZE_8CM
    
    return np.array([world_center_x, world_center_y, world_center_z])

def find_optimal_camera_distance(target):
    """Calculate minimum camera distance to see all voxels"""
    # The workspace is 5x5x5 voxels at 8cm each = 0.4m per side
    workspace_diagonal = math.sqrt(3) * 0.4  # ~0.693m
    
    # With 45° FOV, we need distance = diagonal / (2 * tan(FOV/2))
    # Add some margin
    fov_rad = degrees_to_radians(CAMERA_FOV)
    min_distance = (workspace_diagonal / 2.0) / math.tan(fov_rad / 2.0)
    
    # Add 50% margin for comfortable viewing
    optimal_distance = min_distance * 1.5
    
    return optimal_distance

def main():
    print("╔══════════════════════════════════════════════════════════════════╗")
    print("║     DEFAULT CAMERA VISIBILITY TEST FOR 5x5x5 VOXEL ROOM         ║")
    print("╚══════════════════════════════════════════════════════════════════╝")
    
    print("\n📋 TEST OVERVIEW:")
    print("This test validates that the camera configuration can properly")
    print("view all voxels in a 5x5x5 grid. Each test step validates:")
    print("  1. Field of View - Can the FOV encompass the workspace?")
    print("  2. Camera Angles - Do yaw/pitch provide good 3D perspective?")
    print("  3. Camera Position - Is the camera at the correct location?")
    print("  4. Voxel Visibility - Can all edge voxels be seen?")
    print("  5. Screen Coverage - How much of the screen is utilized?")
    
    # Find optimal target (center of voxel grid)
    optimal_target = find_optimal_camera_target()
    print(f"\n🎯 Calculated optimal camera target (center of 5x5x5 grid):")
    print(f"   Target position: [{optimal_target[0]:.3f}, {optimal_target[1]:.3f}, {optimal_target[2]:.3f}] meters")
    print(f"   This is the center of a {WORKSPACE_SIZE}x{WORKSPACE_SIZE}x{WORKSPACE_SIZE} voxel grid")
    print(f"   with {VOXEL_SIZE_8CM*100:.0f}cm voxels")
    
    # Find optimal distance
    optimal_distance = find_optimal_camera_distance(optimal_target)
    print(f"\n📏 Calculated optimal camera distance: {optimal_distance:.2f} m")
    print(f"   Based on {CAMERA_FOV}° FOV and workspace diagonal of {math.sqrt(3)*0.4:.3f}m")
    
    # Validate FOV first
    validate_field_of_view(CAMERA_FOV, optimal_distance, WORKSPACE_SIZE)
    
    # Validate camera angles
    validate_camera_angles(CAMERA_YAW, CAMERA_PITCH)
    
    # Test current configuration from Application.cpp
    print("\n\n" + "="*70)
    print("TEST 1: CURRENT DEFAULT CONFIGURATION (from Application.cpp)")
    print("="*70)
    current_target = np.array([0.20, 0.20, 0.20])  # Updated from Application.cpp
    current_distance = 1.3  # Updated from Application.cpp
    success1 = test_camera_configuration(current_distance, CAMERA_YAW, CAMERA_PITCH, current_target)
    
    # Test with calculated optimal configuration
    print("\n\n" + "="*70)
    print("TEST 2: CALCULATED OPTIMAL CONFIGURATION")
    print("="*70)
    success2 = test_camera_configuration(optimal_distance, CAMERA_YAW, CAMERA_PITCH, optimal_target)
    
    # Test with true isometric view
    print("\n\n" + "="*70)
    print("TEST 3: TRUE ISOMETRIC VIEW CONFIGURATION")
    print("="*70)
    isometric_yaw = 45.0
    isometric_pitch = 35.264  # atan(1/sqrt(2)) for true isometric
    validate_camera_angles(isometric_yaw, isometric_pitch)
    success3 = test_camera_configuration(optimal_distance, isometric_yaw, isometric_pitch, optimal_target)
    
    # Summary
    print("\n\n" + "╔" + "═"*68 + "╗")
    print("║" + " "*26 + "TEST SUMMARY" + " "*30 + "║")
    print("╚" + "═"*68 + "╝")
    
    print(f"\n📊 Results:")
    print(f"  Test 1 (Current defaults):     {'✅ PASSED' if success1 else '❌ FAILED'}")
    print(f"  Test 2 (Calculated optimal):   {'✅ PASSED' if success2 else '❌ FAILED'}")
    print(f"  Test 3 (True isometric):       {'✅ PASSED' if success3 else '❌ FAILED'}")
    
    if success1:
        print("\n✅ Current camera configuration is VALID!")
        print("   All voxels in the 5x5x5 grid are visible.")
    else:
        print("\n⚠️  RECOMMENDATION:")
        print(f"Update Application.cpp with these values:")
        print(f'  Line 224: m_cameraController->getCamera()->setTarget(Math::Vector3f({optimal_target[0]:.2f}f, {optimal_target[1]:.2f}f, {optimal_target[2]:.2f}f));')
        print(f'  Line 225: m_cameraController->getCamera()->setDistance({optimal_distance:.1f}f);')
    
    # Return success if current configuration works
    return success1

if __name__ == "__main__":
    success = main()
    exit(0 if success else 1)