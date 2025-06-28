# Coordinate System Summary

## Overview

The VoxelEditor uses a **simplified 3-coordinate system** with type-safe conversions between different coordinate spaces. The coordinate system is designed around a **centered origin (0,0,0)** with **Y-up orientation**. All voxel storage and placement operations use IncrementCoordinates (1cm granularity) for universal voxel addressing.

## Coordinate Types

### 1. WorldCoordinates
- **Purpose**: Continuous 3D world space for rendering, camera positions, ray casting, physics
- **Units**: Meters
- **Coordinate System**: Centered at origin (0,0,0), Y-up
- **Underlying Type**: Vector3f
- **Range**: Continuous floating-point values
- **Use Cases**: Camera positioning, ray intersection, physics calculations, rendering

### 2. IncrementCoordinates
- **Purpose**: Universal voxel storage and addressing at 1cm granularity
- **Units**: 1cm increments
- **Coordinate System**: Centered at origin (0,0,0), Y-up, same as WorldCoordinates
- **Underlying Type**: Vector3i
- **Range**: Integer values representing centimeters, centered around origin
- **Use Cases**: **Primary voxel storage**, voxel placement, universal voxel addressing, all voxel operations

### 3. ScreenCoordinates
- **Purpose**: 2D viewport positions for UI and input
- **Units**: Screen pixels
- **Underlying Type**: Vector2f
- **Range**: Floating-point pixel coordinates
- **Use Cases**: Mouse input, UI elements, viewport operations

## Key Architecture Features

### Type Safety
- Strong typing prevents accidental mixing of coordinate types
- Compile-time type checking ensures correct coordinate system usage
- Each coordinate type has its own arithmetic and comparison operators

### Simplified Voxel Storage
- **Single voxel resolution**: All voxels stored at 1cm IncrementCoordinates
- **Multi-resolution rendering**: Different visual resolutions handled at rendering time
- **Universal addressing**: All voxel operations use consistent 1cm coordinate system

### Workspace Bounds
- **Configurable workspace**: 2m³ minimum, 8m³ maximum, 5m³ default
- **Centered coordinate system**: Both World and Increment coordinates centered at origin (0,0,0)
- World coordinates range from -halfSize to +halfSize in X/Z, 0 to height in Y
- IncrementCoordinates range mirrors world bounds in centimeters, centered at origin
- Example for 5m³ workspace: 
  - World: X[-2.5m, 2.5m], Y[0m, 5m], Z[-2.5m, 2.5m]
  - Increment: X[-250cm, 250cm], Y[0cm, 500cm], Z[-250cm, 250cm]

## Coordinate Conversions

### Core Conversion Functions

#### World ↔ Increment Conversions  
- `worldToIncrement()`: Converts world position to 1cm increment coordinates
- `incrementToWorld()`: Converts increment coordinates back to world position
- Uses rounding for precise centimeter alignment
- **Primary conversion** for all voxel operations

#### Resolution-Aware Increment Operations
- `snapToVoxelResolution()`: Snaps increment coordinates to specified voxel resolution boundaries
- `getVoxelCenterIncrement()`: Gets center increment coordinate for a voxel at given resolution
- `isValidIncrementCoordinate()`: Validates increment coordinates against workspace bounds

### Coordinate System Transformations

#### World to Increment Transformation
```cpp
// Transform from centered world space to increment coordinates
int incrementX = round(world.x * 100.0f);  // Convert meters to centimeters
int incrementY = round(world.y * 100.0f);
int incrementZ = round(world.z * 100.0f);
```

#### Increment to World Transformation
```cpp
// Transform from increment coordinates to centered world space
float worldX = increment.x * 0.01f;  // Convert centimeters to meters
float worldY = increment.y * 0.01f;
float worldZ = increment.z * 0.01f;
```

## Validation and Bounds Checking

### Increment Coordinate Validation
- `isValidIncrementCoordinate()`: Checks if increment coordinates are within workspace bounds
- Validates against workspace bounds converted to centimeters
- Ensures voxels are within the defined workspace volume

### World Coordinate Validation
- `isValidWorldCoordinate()`: Checks if world coordinates are within workspace bounds
- Validates X/Z coordinates against centered bounds [-halfSize, +halfSize]
- Validates Y coordinates against [0, height] range

## Utility Functions

### Voxel Alignment and Snapping
- `snapToIncrementGrid()`: Aligns world coordinates to 1cm increment grid
- `snapToVoxelResolution()`: Snaps increment coordinates to specified voxel resolution boundaries
- Ensures precise voxel placement and alignment for different rendering resolutions

### Resolution Information
- `getVoxelSizeMeters()`: Returns voxel size in meters for any rendering resolution
- `getWorkspaceBoundsIncrement()`: Returns workspace bounds in increment coordinates
- Provides metadata for voxel operations and workspace management

## Testing and Validation

### Comprehensive Test Coverage
- **Comprehensive unit tests** validate all coordinate operations
- **Round-trip conversion tests** ensure precision preservation between World and Increment coordinates
- **Boundary condition tests** validate edge cases and workspace limits
- **Type safety tests** verify compile-time type checking
- **Resolution snapping tests** ensure proper alignment for different rendering resolutions

### Test Categories
1. **Type Construction and Access**: Validates coordinate type creation and component access
2. **Arithmetic Operations**: Tests mathematical operations within coordinate types
3. **Conversion Accuracy**: Validates precision of World ↔ Increment conversions
4. **Boundary Validation**: Tests workspace boundary checking for increment coordinates
5. **Resolution Snapping**: Ensures proper voxel alignment for different rendering resolutions
6. **Edge Cases**: Handles extreme values and error conditions

## Implementation Files

### Core Files
- `foundation/math/CoordinateTypes.h`: Defines WorldCoordinates, IncrementCoordinates, and ScreenCoordinates with type safety
- `foundation/math/CoordinateConverter.h`: Implements World ↔ Increment conversions and resolution utilities
- `foundation/math/tests/test_CoordinateTypes.cpp`: Unit tests for coordinate types
- `foundation/math/tests/test_CoordinateConverter.cpp`: Unit tests for conversions

### Key Design Principles
1. **Simplicity**: Streamlined 3-coordinate system with single voxel storage resolution
2. **Type Safety**: Prevent mixing of coordinate systems at compile time
3. **Precision**: Maintain accuracy in World ↔ Increment conversions
4. **Performance**: Efficient conversion algorithms for real-time operations
5. **Universal Storage**: All voxels stored at 1cm resolution for consistent addressing

## Usage Examples

### Basic Coordinate Operations
```cpp
// Create world position
WorldCoordinates worldPos(1.5f, 2.0f, -0.5f);

// Convert to increment coordinates for voxel storage
IncrementCoordinates voxelPos = CoordinateConverter::worldToIncrement(worldPos);

// Validate coordinates
bool isValidWorld = CoordinateConverter::isValidWorldCoordinate(worldPos, workspaceSize);
bool isValidVoxel = CoordinateConverter::isValidIncrementCoordinate(voxelPos, workspaceSize);

// Snap to increment grid alignment
WorldCoordinates snapped = CoordinateConverter::snapToIncrementGrid(worldPos);
```

### Resolution-Aware Operations
```cpp
// Snap voxel position to rendering resolution boundaries
IncrementCoordinates snappedVoxel = CoordinateConverter::snapToVoxelResolution(
    voxelPos, VoxelResolution::Size_4cm);

// Get voxel center for rendering at specific resolution
IncrementCoordinates centerVoxel = CoordinateConverter::getVoxelCenterIncrement(
    voxelPos, VoxelResolution::Size_16cm);
```

This simplified coordinate system provides a robust foundation for the voxel editor with type-safe operations, precise conversions, universal 1cm voxel storage, and multi-resolution rendering capabilities.