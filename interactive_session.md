# Interactive Voxel Editor CLI Session

## Starting the CLI
```bash
./voxel-cli
```

## Session 1: Create a Simple Cube
```
new my_first_project
place 0 0 0
place 1 0 0
place 0 1 0
place 1 1 0
place 0 0 1
place 1 0 1
place 0 1 1
place 1 1 1
status
save simple_cube.vxl
exit
```

## Session 2: Using Fill Command
```
new filled_cube
fill 0 0 0 3 3 3
status
save filled_cube.vxl
exit
```

## Session 3: Working with Groups
```
new grouped_objects
# Create first object
fill 0 0 0 2 2 2
selectbox 0 0 0 2 2 2
group Cube1

# Create second object
fill 4 0 0 6 2 2
selectbox 4 0 0 6 2 2
group Cube2

# List groups
groups
status

# Hide and show groups
hide Cube1
status
show Cube1
status

save grouped_objects.vxl
exit
```

## Session 4: Camera Controls
```
new camera_demo
fill 0 0 0 5 5 5
camera top       # View from top
camera front     # View from front
camera side      # View from side
camera iso       # Isometric view
resetview        # Reset to default
exit
```

## Session 5: Undo/Redo Operations
```
new undo_demo
place 0 0 0
place 1 0 0
place 2 0 0
status           # Shows 3 voxels
undo            # Remove last voxel
status          # Shows 2 voxels
undo            # Remove another
status          # Shows 1 voxel
redo            # Restore one
redo            # Restore another
status          # Shows 3 voxels again
exit
```

## Tips:
- Type `help` to see all available commands
- Type `help <command>` for detailed help on a specific command
- Use `clear` to clear the screen
- Press Ctrl+C to exit at any time