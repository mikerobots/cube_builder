# Adding New Commands to VoxelEditor CLI

The command system has been refactored to make adding new commands as easy as possible. Here's how to add new commands:

## Quick Start - Adding a New Command

### 1. Find the Right Module

Commands are organized by category:
- **FileCommands**: new, open, save, export
- **EditCommands**: place, delete, fill, undo/redo
- **ViewCommands**: camera, zoom, rotate, grid
- **SelectCommands**: select, select_box, select_all
- **SystemCommands**: help, status, debug, quit
- **MeshCommands**: smooth, mesh operations

### 2. Add Your Command

Open the appropriate module file (e.g., `src/commands/EditCommands.cpp`) and add your command to the `getCommands()` method:

```cpp
CommandRegistration()
    .withName("mycommand")
    .withDescription("Does something awesome")
    .withCategory(CommandCategory::EDIT)
    .withArg("param1", "First parameter", "string", true)  // required
    .withArg("param2", "Optional param", "int", false, "0") // optional with default
    .withHandler([](Application* app, const CommandContext& ctx) {
        // Your command logic here
        std::string param1 = ctx.getArg(0);
        int param2 = std::stoi(ctx.getArg(1));
        
        // Do something...
        
        return CommandResult::Success("Command executed!");
    })
```

## Creating a New Command Module

If you need a new category of commands:

### 1. Create the Header

```cpp
// include/cli/MyCommands.h
#pragma once
#include "CommandRegistry.h"

namespace VoxelEditor {

class MyCommands : public ICommandModule {
public:
    std::vector<CommandRegistration> getCommands() override;
};

}
```

### 2. Create the Implementation

```cpp
// src/commands/MyCommands.cpp
#include "cli/MyCommands.h"
#include "cli/Application.h"

namespace VoxelEditor {

std::vector<CommandRegistration> MyCommands::getCommands() {
    return {
        CommandRegistration()
            .withName("mycommand")
            .withDescription("My new command")
            .withCategory(CommandCategory::CUSTOM)
            .withHandler([](Application* app, const CommandContext& ctx) {
                return CommandResult::Success("Hello from my command!");
            })
    };
}

// This auto-registers the module!
REGISTER_COMMAND_MODULE(MyCommands)

}
```

### 3. Add to CMakeLists.txt

Add your new source file to the CLI library sources:
```cmake
src/commands/MyCommands.cpp
```

## Command Features

### Arguments
- Required args: `.withArg("name", "description", "type", true)`
- Optional args: `.withArg("name", "description", "type", false, "default")`
- Types: "string", "int", "float", "coordinate", "bool"

### Aliases
```cpp
.withAlias("shortname")
.withAliases({"alt1", "alt2"})
```

### Complex Commands
For commands that need state or complex logic, you can use a separate function:

```cpp
CommandRegistration()
    .withName("complex")
    .withHandler([](Application* app, const CommandContext& ctx) {
        return MyCommands::executeComplexCommand(app, ctx);
    })
```

## Best Practices

1. **Validation**: Always validate inputs
2. **Error Messages**: Provide clear error messages
3. **Success Feedback**: Tell the user what happened
4. **Undo Support**: Use Command pattern for undoable operations
5. **Mesh Updates**: Call `app->requestMeshUpdate()` after geometry changes

## Example: Adding a "clear" Command

```cpp
// In EditCommands.cpp, add to getCommands():
CommandRegistration()
    .withName("clear")
    .withDescription("Clear all voxels")
    .withCategory(CommandCategory::EDIT)
    .withHandler([](Application* app, const CommandContext& ctx) {
        // Create undo-able command
        auto cmd = std::make_unique<UndoRedo::ClearAllCommand>(
            app->getVoxelManager()
        );
        
        if (app->getHistoryManager()->executeCommand(std::move(cmd))) {
            app->requestMeshUpdate();
            return CommandResult::Success("All voxels cleared");
        }
        
        return CommandResult::Error("Failed to clear voxels");
    })
```

That's it! The command is automatically registered and available in the CLI.