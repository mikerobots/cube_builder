#pragma once

#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace VoxelEditor {
namespace UndoRedo {

// Forward declarations
class Command;

enum class CommandType {
    VoxelEdit,
    Selection,
    Group,
    Camera,
    Workspace,
    Import,
    Composite
};

struct ValidationResult {
    bool valid = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    void addError(const std::string& error) {
        errors.push_back(error);
        valid = false;
    }
    
    void addWarning(const std::string& warning) {
        warnings.push_back(warning);
    }
};

class Command {
public:
    virtual ~Command() = default;
    
    // Core operations
    virtual bool execute() = 0;
    virtual bool undo() = 0;
    virtual bool canUndo() const { return true; }
    
    // Command information
    virtual std::string getName() const = 0;
    virtual std::string getDescription() const { return getName(); }
    virtual CommandType getType() const = 0;
    
    // Memory management
    virtual size_t getMemoryUsage() const = 0;
    virtual void compress() {}
    virtual void decompress() {}
    
    // Merging capability
    virtual bool canMergeWith(const Command& other) const { return false; }
    virtual std::unique_ptr<Command> mergeWith(std::unique_ptr<Command> other) { return nullptr; }
    
    // Validation
    virtual bool isValid() const { return true; }
    virtual ValidationResult validate() const {
        ValidationResult result;
        if (!isValid()) {
            result.addError("Command is not valid");
        }
        return result;
    }
    
    // Timing information
    std::chrono::high_resolution_clock::time_point getTimestamp() const { return m_timestamp; }
    bool hasExecuted() const { return m_executed; }
    
protected:
    std::chrono::high_resolution_clock::time_point m_timestamp = std::chrono::high_resolution_clock::now();
    bool m_executed = false;
};

}
}