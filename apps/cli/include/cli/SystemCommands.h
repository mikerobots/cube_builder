#pragma once

#include "CommandRegistry.h"

namespace VoxelEditor {

/**
 * @brief System and utility commands
 * 
 * Commands for system operations:
 * - help: Show available commands
 * - status: Show current state
 * - debug: Debug utilities
 * - quit/exit: Exit the application
 * - version: Show version info
 */
class SystemCommands : public ICommandModule {
public:
    std::vector<CommandRegistration> getCommands() override;
};

} // namespace VoxelEditor