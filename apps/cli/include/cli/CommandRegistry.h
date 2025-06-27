#pragma once

#include "CommandTypes.h"
#include <functional>
#include <memory>
#include <vector>

namespace VoxelEditor {

// Forward declarations
class Application;
class CommandProcessor;
class ICommandModule;
class RenderWindow;

// Forward declarations of core systems
namespace VoxelData { class VoxelDataManager; }
namespace Camera { class CameraController; }
namespace Rendering { class RenderEngine; }
namespace Selection { class SelectionManager; }
namespace UndoRedo { class HistoryManager; }
namespace Groups { class GroupManager; }
namespace FileIO { class FileManager; }
namespace Events { class EventDispatcher; }

// Helper class for auto-registration
class CommandModuleRegistrar {
public:
    static void registerFactory(std::function<std::unique_ptr<ICommandModule>()> factory);
};

/**
 * @brief Command handler function type
 */
using CommandHandler = std::function<CommandResult(const CommandContext&)>;

/**
 * @brief Command registration info
 */
// Structure for command argument definition
struct CommandArgDef {
    std::string name;
    std::string description;
    std::string type = "string";
    bool required = true;
    std::string defaultValue;
};

struct CommandRegistration {
    std::string name;
    std::string description;
    std::string category;
    std::vector<std::string> aliases;
    std::vector<CommandArgDef> args;
    CommandHandler handler;
    
    // Builder pattern for easy command creation
    CommandRegistration& withName(const std::string& n) { name = n; return *this; }
    CommandRegistration& withDescription(const std::string& d) { description = d; return *this; }
    CommandRegistration& withCategory(const std::string& c) { category = c; return *this; }
    CommandRegistration& withAlias(const std::string& a) { aliases.push_back(a); return *this; }
    CommandRegistration& withAliases(const std::vector<std::string>& a) { aliases = a; return *this; }
    CommandRegistration& withArg(const std::string& name, const std::string& desc, 
                                const std::string& type = "string", bool required = true, 
                                const std::string& defaultVal = "") {
        args.push_back({name, desc, type, required, defaultVal});
        return *this;
    }
    CommandRegistration& withHandler(CommandHandler h) { handler = h; return *this; }
};

/**
 * @brief Base class for command modules
 */
class CommandModule {
protected:
    Application* m_app = nullptr;
    
    // Convenience accessors for common systems
    VoxelData::VoxelDataManager* m_voxelManager = nullptr;
    UndoRedo::HistoryManager* m_historyManager = nullptr;
    Camera::CameraController* m_cameraController = nullptr;
    Rendering::RenderEngine* m_renderEngine = nullptr;
    Selection::SelectionManager* m_selectionManager = nullptr;
    Groups::GroupManager* m_groupManager = nullptr;
    FileIO::FileManager* m_fileManager = nullptr;
    Events::EventDispatcher* m_eventDispatcher = nullptr;
    RenderWindow* m_renderWindow = nullptr;
    
public:
    explicit CommandModule(Application* app);
    virtual ~CommandModule() = default;
    
    // Request mesh update after voxel changes
    void requestMeshUpdate();
};

/**
 * @brief Base class for command modules with self-registration
 */
class ICommandModule {
public:
    virtual ~ICommandModule() = default;
    virtual std::vector<CommandRegistration> getCommands() = 0;
    virtual void setApplication(Application* app) = 0;
};

/**
 * @brief Command registry for dynamic command registration
 */
class CommandRegistry {
public:
    static CommandRegistry& getInstance() {
        static CommandRegistry instance;
        return instance;
    }
    
    /**
     * @brief Register a command module
     */
    void registerModule(std::unique_ptr<ICommandModule> module) {
        m_modules.push_back(std::move(module));
    }
    
    /**
     * @brief Register all commands with the command processor
     */
    void registerAllCommands(Application* app, CommandProcessor* processor);
    
    /**
     * @brief Clear all registered modules
     */
    void clear() {
        m_modules.clear();
    }

private:
    CommandRegistry() = default;
    std::vector<std::unique_ptr<ICommandModule>> m_modules;
};

/**
 * @brief Macro for easy command module registration
 * 
 * Usage:
 * REGISTER_COMMAND_MODULE(MyCommandModule)
 */
#define REGISTER_COMMAND_MODULE(ModuleClass) \
    namespace { \
        struct ModuleClass##Registrar { \
            ModuleClass##Registrar() { \
                CommandRegistry::getInstance().registerModule( \
                    std::make_unique<ModuleClass>()); \
            } \
        }; \
        static ModuleClass##Registrar s_##ModuleClass##Registrar; \
    }

} // namespace VoxelEditor