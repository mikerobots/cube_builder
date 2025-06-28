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
    // Force references to each command module's static initializer
    // by creating actual instances (even if we immediately delete them)
    // This ensures the static initializers in each translation unit run
    
    // The key is to reference something from each module that will
    // force the linker to include the entire object file
    auto* edit = new EditCommands();
    delete edit;
    
    auto* file = new FileCommands();
    delete file;
    
    auto* view = new ViewCommands();
    delete view;
    
    auto* select = new SelectCommands();
    delete select;
    
    auto* system = new SystemCommands();
    delete system;
    
    auto* mesh = new MeshCommands();
    delete mesh;
}

} // namespace VoxelEditor