// NEW VERSION of Application::registerCommands() using the dynamic system
// This would replace the large registerCommands() function in Commands.cpp

#include "cli/Application.h"
#include "cli/CommandRegistry.h"
#include "cli/CommandProcessor.h"

// Include all command modules - they self-register
#include "cli/FileCommands.h"
#include "cli/EditCommands.h"
#include "cli/ViewCommands.h"
#include "cli/SelectCommands.h"
#include "cli/SystemCommands.h"
#include "cli/MeshCommands.h"

namespace VoxelEditor {

void Application::registerCommands() {
    // Simply delegate to the registry!
    CommandRegistry::getInstance().registerAllCommands(this, m_commandProcessor.get());
    
    // That's it! All commands from all modules are now registered.
    // To add a new command, just add it to the appropriate module's getCommands() method.
    // To add a new module, create it and include its header here.
}

} // namespace VoxelEditor