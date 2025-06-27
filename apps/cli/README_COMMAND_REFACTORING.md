# Command System Refactoring

This document explains the new dynamic command registration system that makes adding commands to the VoxelEditor CLI much easier.

## Overview

The command system has been refactored from a single 1900+ line `Commands.cpp` file into a modular, extensible system where:

1. Commands are organized into logical modules by functionality
2. Each module self-registers its commands
3. Adding new commands is as simple as adding an entry to a list
4. No need to modify multiple files when adding commands

## Architecture

### Core Components

1. **CommandRegistry** (`CommandRegistry.h/cpp`)
   - Singleton that manages all command modules
   - Auto-registers commands with the CommandProcessor
   - Provides a central place for command discovery

2. **ICommandModule** (in `CommandRegistry.h`)
   - Interface that all command modules implement
   - Single method: `getCommands()` returns list of commands

3. **CommandRegistration** (in `CommandRegistry.h`)
   - Builder pattern for defining commands
   - Fluent interface for easy command creation

### Command Modules

- **FileCommands**: new, open, save, save_as, export
- **EditCommands**: place, delete, fill, undo, redo
- **ViewCommands**: camera, zoom, rotate, reset_view, grid
- **SelectCommands**: select, select_box, select_all, select_none
- **SystemCommands**: help, status, debug, quit, version
- **MeshCommands**: smooth, mesh validate, mesh info

## Benefits

1. **Easy to Add Commands**: Just add to the module's `getCommands()` list
2. **Self-Contained**: Each module is independent
3. **Auto-Registration**: Modules register themselves using `REGISTER_COMMAND_MODULE`
4. **Type-Safe**: Strong typing with builder pattern
5. **Testable**: Each module can be tested independently
6. **Maintainable**: Commands are grouped logically

## Migration Guide

To migrate the existing `Commands.cpp`:

1. **Phase 1**: Create module structure (DONE)
   - Created base classes and registry
   - Created example modules (FileCommands, EditCommands)

2. **Phase 2**: Migrate commands by category
   - Move each command to its appropriate module
   - Update command handlers to use the new format

3. **Phase 3**: Update Application
   - Replace `registerCommands()` with registry call
   - Remove old `Commands.cpp`

4. **Phase 4**: Update build system
   - Add new source files to CMakeLists.txt
   - Remove old Commands.cpp

## Example: Complete Module

```cpp
// ViewCommands.cpp
#include "cli/ViewCommands.h"
#include "cli/Application.h"
#include "camera/CameraController.h"

namespace VoxelEditor {

std::vector<CommandRegistration> ViewCommands::getCommands() {
    return {
        // CAMERA command
        CommandRegistration()
            .withName("camera")
            .withDescription("Set camera view")
            .withCategory(CommandCategory::VIEW)
            .withArg("view", "View name (front/back/top/bottom/left/right/iso)")
            .withHandler([](Application* app, const CommandContext& ctx) {
                std::string view = ctx.getArg(0);
                
                if (view == "front") {
                    app->getCameraController()->setViewPreset(Camera::ViewPreset::FRONT);
                } else if (view == "iso") {
                    app->getCameraController()->setViewPreset(Camera::ViewPreset::ISOMETRIC);
                } // ... etc
                
                return CommandResult::Success("Camera view set to " + view);
            }),
            
        // ZOOM command
        CommandRegistration()
            .withName("zoom")
            .withDescription("Zoom camera in/out")
            .withCategory(CommandCategory::VIEW)
            .withArg("direction", "in or out")
            .withHandler([](Application* app, const CommandContext& ctx) {
                std::string dir = ctx.getArg(0);
                float delta = (dir == "in") ? -1.0f : 1.0f;
                app->getCameraController()->zoom(delta);
                return CommandResult::Success("Zoomed " + dir);
            })
    };
}

REGISTER_COMMAND_MODULE(ViewCommands)

} // namespace VoxelEditor
```

## File Structure

```
apps/cli/
├── include/cli/
│   ├── CommandRegistry.h      # Core registry system
│   ├── BaseCommandModule.h    # (Optional) Base class
│   ├── FileCommands.h        # File operations
│   ├── EditCommands.h        # Edit operations
│   ├── ViewCommands.h        # View/camera commands
│   ├── SelectCommands.h      # Selection commands
│   ├── SystemCommands.h      # System utilities
│   └── MeshCommands.h        # Mesh operations
│
├── src/
│   ├── CommandRegistry.cpp   # Registry implementation
│   └── commands/            # Command modules
│       ├── FileCommands.cpp
│       ├── EditCommands.cpp
│       ├── ViewCommands.cpp
│       ├── SelectCommands.cpp
│       ├── SystemCommands.cpp
│       └── MeshCommands.cpp
│
└── ADDING_COMMANDS.md       # Developer guide

```

## Next Steps

1. Complete migration of all commands to modules
2. Update CMakeLists.txt
3. Remove old Commands.cpp
4. Add unit tests for each module
5. Document command API for plugin system