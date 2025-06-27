// New implementation of Application::registerCommands() using the dynamic command registry system
// This file should replace the registerCommands() implementation in Commands.cpp

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
    // All command modules have already registered themselves via REGISTER_COMMAND_MODULE
    CommandRegistry::getInstance().registerAllCommands(this, m_commandProcessor.get());
    
    LOG_INFO("Registered all commands from " << 
             CommandRegistry::getInstance().getModuleCount() << " modules");
}

} // namespace VoxelEditor