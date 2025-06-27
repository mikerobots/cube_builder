#pragma once

#include "CommandTypes.h"

namespace VoxelEditor {

// Forward declaration
class Application;

/**
 * @brief Base class for command modules
 * 
 * This class provides a common interface for command modules that can register
 * their commands with the command processor. Each module should inherit from
 * this class and implement the registerCommands method.
 */
class BaseCommandModule {
public:
    explicit BaseCommandModule(Application* app) : m_app(app) {}
    virtual ~BaseCommandModule() = default;
    
    /**
     * @brief Register all commands for this module
     * 
     * This method should be implemented by each command module to register
     * its specific commands with the command processor.
     */
    virtual void registerCommands() = 0;

protected:
    Application* m_app;
};

} // namespace VoxelEditor