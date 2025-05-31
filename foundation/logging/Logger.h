#pragma once

#include "LogOutput.h"
#include <vector>
#include <memory>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>

namespace VoxelEditor {
namespace Logging {

class Logger {
public:
    using Level = LogLevel;
    
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    
    void setLevel(Level level) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_level = level;
    }
    
    Level getLevel() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_level;
    }
    
    void addOutput(std::unique_ptr<LogOutput> output) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_outputs.push_back(std::move(output));
    }
    
    void removeOutput(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_outputs.erase(
            std::remove_if(m_outputs.begin(), m_outputs.end(),
                [&name](const std::unique_ptr<LogOutput>& output) {
                    return output->getName() == name;
                }),
            m_outputs.end()
        );
    }
    
    void clearOutputs() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_outputs.clear();
    }
    
    size_t getOutputCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_outputs.size();
    }
    
    void flush() {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& output : m_outputs) {
            output->flush();
        }
    }
    
    void debug(const std::string& message, const std::string& component = "") {
        log(Level::Debug, message, component);
    }
    
    void info(const std::string& message, const std::string& component = "") {
        log(Level::Info, message, component);
    }
    
    void warning(const std::string& message, const std::string& component = "") {
        log(Level::Warning, message, component);
    }
    
    void error(const std::string& message, const std::string& component = "") {
        log(Level::Error, message, component);
    }
    
    template<typename T, typename... Args>
    void debugf(const std::string& format, T&& arg, Args&&... args) {
        if (shouldLog(Level::Debug)) {
            log(Level::Debug, formatString(format, std::forward<T>(arg), std::forward<Args>(args)...), "");
        }
    }
    
    template<typename T, typename... Args>
    void infof(const std::string& format, T&& arg, Args&&... args) {
        if (shouldLog(Level::Info)) {
            log(Level::Info, formatString(format, std::forward<T>(arg), std::forward<Args>(args)...), "");
        }
    }
    
    template<typename T, typename... Args>
    void warningf(const std::string& format, T&& arg, Args&&... args) {
        if (shouldLog(Level::Warning)) {
            log(Level::Warning, formatString(format, std::forward<T>(arg), std::forward<Args>(args)...), "");
        }
    }
    
    template<typename T, typename... Args>
    void errorf(const std::string& format, T&& arg, Args&&... args) {
        if (shouldLog(Level::Error)) {
            log(Level::Error, formatString(format, std::forward<T>(arg), std::forward<Args>(args)...), "");
        }
    }
    
    template<typename... Args>
    void debugfc(const std::string& component, const std::string& format, Args&&... args) {
        if (shouldLog(Level::Debug)) {
            log(Level::Debug, formatString(format, std::forward<Args>(args)...), component);
        }
    }
    
    template<typename... Args>
    void infofc(const std::string& component, const std::string& format, Args&&... args) {
        if (shouldLog(Level::Info)) {
            log(Level::Info, formatString(format, std::forward<Args>(args)...), component);
        }
    }
    
    template<typename... Args>
    void warningfc(const std::string& component, const std::string& format, Args&&... args) {
        if (shouldLog(Level::Warning)) {
            log(Level::Warning, formatString(format, std::forward<Args>(args)...), component);
        }
    }
    
    template<typename... Args>
    void errorfc(const std::string& component, const std::string& format, Args&&... args) {
        if (shouldLog(Level::Error)) {
            log(Level::Error, formatString(format, std::forward<Args>(args)...), component);
        }
    }
    
private:
    Logger() : m_level(Level::Info) {
        // Add default console output
        addOutput(std::make_unique<ConsoleOutput>());
    }
    
    Level m_level;
    std::vector<std::unique_ptr<LogOutput>> m_outputs;
    mutable std::mutex m_mutex;
    
    bool shouldLog(Level level) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return level >= m_level;
    }
    
    void log(Level level, const std::string& message, const std::string& component) {
        if (!shouldLog(level)) {
            return;
        }
        
        LogMessage logMsg;
        logMsg.level = level;
        logMsg.component = component;
        logMsg.message = message;
        logMsg.timestamp = getCurrentTimestamp();
        logMsg.threadId = getThreadId();
        
        
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& output : m_outputs) {
            output->write(logMsg);
        }
    }
    
    std::string getCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
    std::string getThreadId() const {
        std::stringstream ss;
        ss << std::this_thread::get_id();
        return ss.str();
    }
    
    template<typename... Args>
    std::string formatString(const std::string& format, Args&&... args) const {
        if constexpr (sizeof...(args) == 0) {
            return format;
        } else {
            size_t size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
            std::unique_ptr<char[]> buf(new char[size]);
            std::snprintf(buf.get(), size, format.c_str(), args...);
            return std::string(buf.get(), buf.get() + size - 1);
        }
    }
};

// Convenience macros
#define LOG_DEBUG(msg) VoxelEditor::Logging::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) VoxelEditor::Logging::Logger::getInstance().info(msg)
#define LOG_WARNING(msg) VoxelEditor::Logging::Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) VoxelEditor::Logging::Logger::getInstance().error(msg)

#define LOG_DEBUG_C(component, msg) VoxelEditor::Logging::Logger::getInstance().debug(msg, component)
#define LOG_INFO_C(component, msg) VoxelEditor::Logging::Logger::getInstance().info(msg, component)
#define LOG_WARNING_C(component, msg) VoxelEditor::Logging::Logger::getInstance().warning(msg, component)
#define LOG_ERROR_C(component, msg) VoxelEditor::Logging::Logger::getInstance().error(msg, component)

}
}