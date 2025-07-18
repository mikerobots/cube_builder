# Voxel Editor CLI Guide

## Quick Start

1. **Launch the application**:
   ```bash
   ./build/bin/VoxelEditor_CLI
   ```

2. **Create your first voxel**:
   ```
   > place 0cm 0cm 0cm
   ```

3. **View from different angles**:
   ```
   > camera front
   > camera iso
   ```

4. **Save your work**:
   ```
   > save myproject.cvef
   ```

## Basic Workflow

### Starting a New Project
```
> new                    # Clear everything
> resolution 8cm         # Set voxel size
> workspace 5 5 5        # Set 5x5x5 meter workspace
```

### Building with Voxels
```
> place 0cm 0cm 0cm              # Place first voxel at origin
> place 1m 0cm 0cm               # Place voxel 1 meter to the right
> place -50cm 50cm -50cm         # Place voxel using cm units
> fill 0m 0m 0m 2m 1m 2m         # Fill a 2x1x2 meter region
> delete 50cm 0cm 50cm           # Remove a voxel
```

**Note**: All coordinate commands now require units (cm or m). Examples:
- `100cm` or `1m` (both equal 1 meter)
- `-50cm` or `-0.5m` (negative coordinates allowed for X and Z)
- `1.5m` or `150cm` (decimals supported)

### Using the Mouse
- Hover over the render window to see green outline preview
- Left-click to place voxels
- Right-click to remove voxels
- Middle-mouse drag to rotate view
- Scroll to zoom

#### Placement Behavior
- **Default (no shift)**: Voxels snap to their own grid size
  - 4cm voxels snap to 4cm grid
  - 32cm voxels snap to 32cm grid
  - Same-size voxels align perfectly when placed on each other
- **With Shift key**: 1cm precise placement
  - Hold Shift while clicking for fine-grained control
  - Allows placement at any 1cm position
  - Useful for creating overhangs or precise alignments
- **Special case**: Large voxels on smaller faces always use 1cm placement

### Keyboard Shortcuts
#### Resolution Switching
Quick resolution changes using number keys:
- **1** - 1cm voxels (finest detail)
- **2** - 2cm voxels
- **3** - 4cm voxels
- **4** - 8cm voxels
- **5** - 16cm voxels
- **6** - 32cm voxels
- **7** - 64cm voxels
- **8** - 128cm voxels
- **9** - 256cm voxels
- **0** - 512cm voxels (largest)

### Working with Groups
```
> selectbox -1m 0m -1m 1m 2m 1m  # Select a centered region
> group "wall"                   # Create group from selection
> hide wall                       # Hide the group
> show wall                       # Show it again
> groups                          # List all groups
```

### Camera Control
```
> camera front          # Front view
> camera top            # Top-down view
> camera iso            # Isometric view
> zoom 1.5              # Zoom in 50%
> rotate 45 0           # Rotate 45° horizontally
> resetview             # Reset to default
> grid off              # Hide ground plane grid
> grid on               # Show ground plane grid
> grid toggle           # Toggle grid visibility
> edges off             # Hide voxel edges/wireframe
> edges on              # Show voxel edges
> edges toggle          # Toggle edge visibility
```

### File Operations
```
> save myproject.cvef    # Save project
> open myproject.cvef    # Load project
> export model.stl       # Export for 3D printing
```

### Surface Smoothing (For 3D Printing)

```
> smooth                    # Show current smoothing level and settings
> smooth 0                  # No smoothing (blocky voxel look)
> smooth 5                  # Medium smoothing (balanced)
> smooth 10                 # Maximum smoothing (very smooth)
> smooth preview on         # Enable real-time preview
> smooth preview off        # Disable preview (better performance)
> smooth algorithm taubin   # Use Taubin algorithm (feature-preserving)
> smooth algorithm laplacian # Use Laplacian algorithm (basic smoothing)
> smooth algorithm bilaplacian # Use BiLaplacian algorithm (aggressive)
> mesh validate             # Check if mesh is printable
> mesh info                 # Show mesh statistics
```

**Smoothing Levels**:
- **0**: Raw voxel edges (minecraft-like)
- **1-3**: Light smoothing, retains blocky character
- **4-7**: Medium smoothing, good for toys/models
- **8-10+**: Heavy smoothing, organic shapes

**Smoothing Algorithms**:
- **Taubin** (default): Feature-preserving, prevents shrinkage
- **Laplacian**: Basic smoothing, may shrink features
- **BiLaplacian**: Aggressive smoothing for very smooth results

**Note**: Smoothing is applied during export operations. Higher levels increase processing time.

## Advanced Tips

### Efficient Building
1. Use larger resolutions (16cm, 32cm) for rough shapes
2. Switch to smaller resolutions (4cm, 2cm) for details
3. Use `fill` command for large areas
4. Group related voxels for easy management

### Keyboard Shortcuts
- **Tab**: Auto-complete commands and filenames
- **Up/Down**: Navigate command history
- **Ctrl+L**: Clear screen (same as `clear` command)

### Selection Techniques
```
> select 0cm 0cm 0cm              # Select single voxel at origin
> selectbox -2m 0m -2m 2m 1m 2m   # Select centered box region
> selectall                       # Select everything
> selectnone                      # Clear selection
```

### Undo/Redo
```
> undo                  # Undo last action
> undo                  # Keep undoing
> redo                  # Redo undone action
```

## Command Reference

### Essential Commands
| Command | Description | Example |
|---------|-------------|---------|
| `place` | Add a voxel | `place 1m 50cm 0cm` |
| `delete` | Remove a voxel | `delete 1m 50cm 0cm` |
| `fill` | Fill box region | `fill 0m 0m 0m 2m 1m 2m` |
| `selectbox` | Select box region | `selectbox -2m 0m -2m 2m 2m 2m` |
| `camera` | Change view | `camera front` |
| `grid` | Toggle ground plane | `grid on/off/toggle` |
| `edges` | Toggle voxel edges | `edges on/off/toggle` |
| `smooth` | Set smoothing level | `smooth 5` |
| `mesh` | Mesh operations | `mesh validate` |
| `save` | Save project | `save project.cvef` |
| `export` | Export STL | `export model.stl` |
| `build` | Show build info | `build` |
| `help` | Get help | `help place` |

### View Presets
- `front`, `back`, `left`, `right`, `top`, `bottom`
- `iso` - Isometric view (good for 3D building)
- `default` - Default perspective view

### Resolution Sizes
- `1cm` - Finest detail (press 1)
- `2cm`, `4cm`, `8cm` - Common sizes (press 2, 3, 4)
- `16cm`, `32cm`, `64cm` - Medium sizes (press 5, 6, 7)
- `128cm`, `256cm`, `512cm` - Large blocks (press 8, 9, 0)

## Troubleshooting

### Can't see my voxels?
- Try `camera iso` or `camera front`
- Check if voxels are in a hidden group with `groups`
- Use `status` to see total voxel count

### Mouse not working?
- Make sure the render window has focus
- Click on the window first
- Check if you're in the right resolution

### Performance issues?
- Use larger voxel sizes for big structures
- Hide groups you're not working on
- Save and restart if memory usage is high

## Example Projects

### Simple House
```
> new
> resolution 32cm
> fill -2m 0m -2m 2m 0m 2m    # Floor (4x4 meter base)
> fill -2m 0m -2m -2m 2m 2m   # Left wall
> fill 2m 0m -2m 2m 2m 2m     # Right wall
> fill -2m 0m -2m 2m 0m -2m   # Front wall
> fill -2m 0m 2m 2m 0m 2m     # Back wall
> fill -2m 2m -2m 2m 2m 2m    # Roof
> save house.cvef
```

### Detailed Sculpture
```
> new
> resolution 8cm           # Start with medium size
> place 0cm 0cm 0cm        # Build base shape at origin
> place 8cm 0cm 0cm        # Add adjacent voxel
> resolution 4cm           # Switch to finer detail
> place 0cm 8cm 0cm        # Add details on top
> resolution 2cm           # Finest details
> place 2cm 12cm 2cm       # Fine detail work
> smooth 7                  # Apply medium-high smoothing
> mesh validate            # Check if mesh is printable
> export sculpture.stl     # Export smoothed model
```

### Creating a Toy (Without Smoothing)
```
> new
> resolution 16cm          # Start with blocks
> fill -32cm 0cm -32cm 32cm 64cm 32cm  # Basic body shape
> place 0cm 80cm 0cm       # Add head
> selectbox -48cm 32cm -16cm -32cm 48cm 16cm  # Select arm area
> group "left_arm"         # Group it
> smooth 5                  # Apply medium smoothing for toy
> camera iso               # Check from all angles
> export toy.stl           # Export smoothed toy model
```