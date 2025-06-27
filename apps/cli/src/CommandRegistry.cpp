#include "cli/CommandRegistry.h"
#include "cli/CommandProcessor.h"
#include "cli/Application.h"
#include "logging/Logger.h"
#include <sstream>

namespace VoxelEditor {

// Storage for registered command module factories
static std::vector<std::function<std::unique_ptr<ICommandModule>()>>& getModuleFactories() {
    static std::vector<std::function<std::unique_ptr<ICommandModule>()>> factories;
    return factories;
}

void CommandModuleRegistrar::registerFactory(std::function<std::unique_ptr<ICommandModule>()> factory) {
    getModuleFactories().push_back(factory);
}

void CommandRegistry::registerAllCommands(Application* app, CommandProcessor* processor) {
    // First, create instances of all auto-registered modules
    for (const auto& factory : getModuleFactories()) {
        auto module = factory();
        module->setApplication(app);  // Pass Application pointer to module
        m_modules.push_back(std::move(module));
    }
    
    // Then register all commands from all modules
    for (const auto& module : m_modules) {
        auto commands = module->getCommands();
        
        for (const auto& cmdReg : commands) {
            // Create CommandDefinition from CommandRegistration
            CommandDefinition cmdDef;
            cmdDef.name = cmdReg.name;
            cmdDef.description = cmdReg.description;
            cmdDef.category = cmdReg.category;
            cmdDef.aliases = cmdReg.aliases;
            
            // Convert arguments
            for (const auto& arg : cmdReg.args) {
                CommandArgument cmdArg;
                cmdArg.name = arg.name;
                cmdArg.description = arg.description;
                cmdArg.type = arg.type;
                cmdArg.required = arg.required;
                cmdArg.defaultValue = arg.defaultValue;
                cmdDef.arguments.push_back(cmdArg);
            }
            
            // Handler now doesn't need Application parameter
            cmdDef.handler = cmdReg.handler;
            
            // Register the command
            processor->registerCommand(cmdDef);
            
            std::stringstream ss;
            ss << "Registered command: " << cmdDef.name << " [" << cmdDef.category << "]";
            LOG_DEBUG(ss.str());
        }
    }
    
    size_t totalCommands = 0;
    for (const auto& module : m_modules) {
        totalCommands += module->getCommands().size();
    }
    
    std::stringstream ss;
    ss << "Registered " << totalCommands << " commands from " << m_modules.size() << " modules";
    LOG_INFO(ss.str());
}

} // namespace VoxelEditor