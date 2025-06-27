// Force linking of command modules
// This file ensures that all command module object files are linked into the executable

#include "cli/EditCommands.h"
#include "cli/FileCommands.h"
#include "cli/ViewCommands.h"
#include "cli/SelectCommands.h"
#include "cli/SystemCommands.h"
#include "cli/MeshCommands.h"

namespace VoxelEditor {

// Force the linker to include command module symbols
void forceCommandModuleInitialization() {
    // Create temporary instances to force static initializers
    volatile EditCommands* edit = nullptr;
    volatile FileCommands* file = nullptr;
    volatile ViewCommands* view = nullptr;
    volatile SelectCommands* select = nullptr;
    volatile SystemCommands* system = nullptr;
    volatile MeshCommands* mesh = nullptr;
    
    // Reference the types to prevent optimization
    (void)edit;
    (void)file;
    (void)view;
    (void)select;
    (void)system;
    (void)mesh;
}

} // namespace VoxelEditor