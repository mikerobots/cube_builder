#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>
#include <optional>

namespace VoxelEditor {

// Forward declarations
class Application;
class CommandContext;

// Command result
struct CommandResult {
    bool success = false;
    std::string message;
    bool shouldExit = false;
    
    static CommandResult Success(const std::string& msg = "") {
        return {true, msg, false};
    }
    
    static CommandResult Error(const std::string& msg) {
        return {false, msg, false};
    }
    
    static CommandResult Exit(const std::string& msg = "Goodbye!") {
        return {true, msg, true};
    }
};

// Command argument
struct CommandArgument {
    std::string name;
    std::string description;
    std::string type; // "string", "int", "float", "bool"
    bool required = true;
    std::string defaultValue;
};

// Command definition
struct CommandDefinition {
    std::string name;
    std::string description;
    std::string category;
    std::vector<std::string> aliases;
    std::vector<CommandArgument> arguments;
    std::function<CommandResult(const CommandContext&)> handler;
    
    // Usage string
    std::string getUsage() const;
    std::string getHelp() const;
};

// Command context passed to handlers
class CommandContext {
public:
    CommandContext(Application* app, const std::string& cmd, 
                  const std::vector<std::string>& args);
    
    // Access to application
    Application* getApp() const { return m_app; }
    
    // Command info
    const std::string& getCommand() const { return m_command; }
    const std::vector<std::string>& getArgs() const { return m_args; }
    size_t getArgCount() const { return m_args.size(); }
    
    // Argument access
    std::string getArg(size_t index, const std::string& defaultValue = "") const;
    int getIntArg(size_t index, int defaultValue = 0) const;
    float getFloatArg(size_t index, float defaultValue = 0.0f) const;
    bool getBoolArg(size_t index, bool defaultValue = false) const;
    
    // Coordinate parsing with units
    // Returns coordinate in grid units (1cm increments)
    // Accepts formats: "100cm", "1.5m", "1m", "-2.5m"
    // Returns std::nullopt if invalid format or index out of bounds
    std::optional<int> getCoordinateArg(size_t index) const;
    
    // Named argument access (for future --name=value style)
    bool hasOption(const std::string& name) const;
    std::string getOption(const std::string& name, const std::string& defaultValue = "") const;
    
private:
    Application* m_app;
    std::string m_command;
    std::vector<std::string> m_args;
    std::unordered_map<std::string, std::string> m_options;
    
    void parseOptions();
};

// Command categories
namespace CommandCategory {
    constexpr const char* FILE = "File Operations";
    constexpr const char* EDIT = "Edit Operations";
    constexpr const char* VIEW = "View Controls";
    constexpr const char* SELECT = "Selection";
    constexpr const char* GROUP = "Group Management";
    constexpr const char* HELP = "Help & Info";
    constexpr const char* SYSTEM = "System";
    constexpr const char* MESH = "Mesh Operations";
}

// Common command names
namespace Commands {
    // File operations
    constexpr const char* NEW = "new";
    constexpr const char* OPEN = "open";
    constexpr const char* SAVE = "save";
    constexpr const char* SAVE_AS = "saveas";
    constexpr const char* EXPORT = "export";
    constexpr const char* IMPORT = "import";
    
    // Edit operations
    constexpr const char* PLACE = "place";
    constexpr const char* DELETE = "delete";
    constexpr const char* FILL = "fill";
    constexpr const char* PAINT = "paint";
    constexpr const char* MOVE = "move";
    constexpr const char* COPY = "copy";
    constexpr const char* PASTE = "paste";
    constexpr const char* UNDO = "undo";
    constexpr const char* REDO = "redo";
    
    // View controls
    constexpr const char* CAMERA = "camera";
    constexpr const char* ZOOM = "zoom";
    constexpr const char* PAN = "pan";
    constexpr const char* ROTATE = "rotate";
    constexpr const char* RESET_VIEW = "resetview";
    constexpr const char* GRID = "grid";
    constexpr const char* AXES = "axes";
    
    // Selection
    constexpr const char* SELECT = "select";
    constexpr const char* DESELECT = "deselect";
    constexpr const char* SELECT_ALL = "selectall";
    constexpr const char* SELECT_NONE = "selectnone";
    constexpr const char* SELECT_BOX = "selectbox";
    constexpr const char* SELECT_SPHERE = "selectsphere";
    
    // Group management
    constexpr const char* GROUP = "group";
    constexpr const char* UNGROUP = "ungroup";
    constexpr const char* GROUP_LIST = "groups";
    constexpr const char* GROUP_HIDE = "hide";
    constexpr const char* GROUP_SHOW = "show";
    constexpr const char* GROUP_LOCK = "lock";
    constexpr const char* GROUP_UNLOCK = "unlock";
    
    // System
    constexpr const char* HELP = "help";
    constexpr const char* QUIT = "quit";
    constexpr const char* EXIT = "exit";
    constexpr const char* CLEAR = "clear";
    constexpr const char* STATUS = "status";
    constexpr const char* SETTINGS = "settings";
    constexpr const char* VALIDATE = "validate";
    constexpr const char* BUILD = "build";
    
    // Mesh operations
    constexpr const char* SMOOTH = "smooth";
    constexpr const char* MESH = "mesh";
}

} // namespace VoxelEditor