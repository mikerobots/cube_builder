#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <memory>

namespace VoxelEditor {
namespace Logging {

enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    None = 4
};

struct LogMessage {
    LogLevel level;
    std::string component;
    std::string message;
    std::string timestamp;
    std::string threadId;
};

class LogOutput {
public:
    virtual ~LogOutput() = default;
    virtual void write(const LogMessage& message) = 0;
    virtual std::string getName() const = 0;
    virtual void flush() {}
};

class ConsoleOutput : public LogOutput {
public:
    ConsoleOutput(const std::string& name = "Console") : m_name(name) {}
    
    void write(const LogMessage& message) override {
        std::ostream& stream = (message.level >= LogLevel::Warning) ? std::cerr : std::cout;
        stream << formatMessage(message) << std::endl;
    }
    
    std::string getName() const override {
        return m_name;
    }
    
    void flush() override {
        std::cout.flush();
        std::cerr.flush();
    }
    
private:
    std::string m_name;
    
    std::string formatMessage(const LogMessage& message) const {
        std::string levelStr;
        switch (message.level) {
            case LogLevel::Debug:   levelStr = "DEBUG"; break;
            case LogLevel::Info:    levelStr = "INFO "; break;
            case LogLevel::Warning: levelStr = "WARN "; break;
            case LogLevel::Error:   levelStr = "ERROR"; break;
            default:                levelStr = "UNKN "; break;
        }
        
        std::string result = "[" + message.timestamp + "] ";
        result += "[" + levelStr + "] ";
        if (!message.component.empty()) {
            result += "[" + message.component + "] ";
        }
        result += message.message;
        
        return result;
    }
};

class FileOutput : public LogOutput {
public:
    FileOutput(const std::string& filename, const std::string& name = "File") 
        : m_name(name), m_filename(filename) {
        m_file.open(filename, std::ios::app);
        if (!m_file.is_open()) {
            throw std::runtime_error("Failed to open log file: " + filename);
        }
    }
    
    ~FileOutput() {
        if (m_file.is_open()) {
            m_file.close();
        }
    }
    
    void write(const LogMessage& message) override {
        if (m_file.is_open()) {
            m_file << formatMessage(message) << std::endl;
        }
    }
    
    std::string getName() const override {
        return m_name;
    }
    
    void flush() override {
        if (m_file.is_open()) {
            m_file.flush();
        }
    }
    
    std::string getFilename() const {
        return m_filename;
    }
    
private:
    std::string m_name;
    std::string m_filename;
    std::ofstream m_file;
    
    std::string formatMessage(const LogMessage& message) const {
        std::string levelStr;
        switch (message.level) {
            case LogLevel::Debug:   levelStr = "DEBUG"; break;
            case LogLevel::Info:    levelStr = "INFO "; break;
            case LogLevel::Warning: levelStr = "WARN "; break;
            case LogLevel::Error:   levelStr = "ERROR"; break;
            default:                levelStr = "UNKN "; break;
        }
        
        std::string result = "[" + message.timestamp + "] ";
        result += "[" + levelStr + "] ";
        result += "[" + message.threadId + "] ";
        if (!message.component.empty()) {
            result += "[" + message.component + "] ";
        }
        result += message.message;
        
        return result;
    }
};

}
}