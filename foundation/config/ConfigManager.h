#pragma once

#include "ConfigSection.h"
#include "../events/EventDispatcher.h"
#include "../events/CommonEvents.h"
#include <fstream>
#include <sstream>
#include <mutex>
#include <atomic>
#include <memory>

namespace VoxelEditor {
namespace Config {

class ConfigManager {
public:
    static ConfigManager& getInstance() {
        static ConfigManager instance;
        instance.ensureInitialized();
        return instance;
    }
    
    // Configuration loading/saving
    bool loadFromFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        return loadFromStream(file);
    }
    
    bool saveToFile(const std::string& filename) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        return saveToStream(file);
    }
    
    bool loadDefaults() {
        std::lock_guard<std::mutex> lock(m_mutex);
        loadDefaultsNoLock();
        return true;
    }
    
    // Value access
    template<typename T>
    T getValue(const std::string& key, const T& defaultValue = T{}) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_rootSection.getValueByPath(key, defaultValue);
    }
    
    template<typename T>
    void setValue(const std::string& key, const T& value) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_rootSection.setValueByPath(key, value);
        }
        
        // Notify about configuration change
        if (m_eventDispatcher) {
            Events::ConfigurationChangedEvent event(key);
            m_eventDispatcher->dispatch(event);
        }
    }
    
    bool hasValue(const std::string& key) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_rootSection.hasValueByPath(key);
    }
    
    void removeValue(const std::string& key) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            // Split path to find the section and key
            size_t lastDot = key.find_last_of('.');
            if (lastDot == std::string::npos) {
                m_rootSection.removeValue(key);
            } else {
                std::string sectionPath = key.substr(0, lastDot);
                std::string keyName = key.substr(lastDot + 1);
                
                auto section = getSectionByPath(sectionPath);
                if (section) {
                    section->removeValue(keyName);
                }
            }
        }
        
        if (m_eventDispatcher) {
            Events::ConfigurationChangedEvent event(key);
            m_eventDispatcher->dispatch(event);
        }
    }
    
    // Section access
    std::shared_ptr<ConfigSection> getSection(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (path.empty()) {
            return nullptr;
        }
        return getSectionByPath(path);
    }
    
    std::shared_ptr<const ConfigSection> getSection(const std::string& path) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (path.empty()) {
            return nullptr;
        }
        return getSectionByPath(path);
    }
    
    void createSection(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!path.empty()) {
            m_rootSection.getSection(path); // Creates if doesn't exist
        }
    }
    
    // Event integration
    void setEventDispatcher(Events::EventDispatcher* dispatcher) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_eventDispatcher = dispatcher;
    }
    
    // Utility
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_rootSection.clear();
    }
    
    std::vector<std::string> getAllKeys() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<std::string> allKeys;
        collectAllKeys(m_rootSection, "", allKeys);
        return allKeys;
    }
    
    // Simple file format support (key=value format)
    bool loadFromStream(std::istream& stream) {
        std::string line;
        while (std::getline(stream, line)) {
            // Skip empty lines and comments
            line = trim(line);
            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;
            }
            
            // Parse key=value
            size_t equalPos = line.find('=');
            if (equalPos == std::string::npos) {
                continue;
            }
            
            std::string key = trim(line.substr(0, equalPos));
            std::string value = trim(line.substr(equalPos + 1));
            
            // Remove quotes if present
            if (value.length() >= 2 && value[0] == '"' && value[value.length()-1] == '"') {
                value = value.substr(1, value.length() - 2);
            }
            
            // Try to determine type and set value
            setValueFromString(key, value);
        }
        
        return true;
    }
    
    bool saveToStream(std::ostream& stream) const {
        stream << "# VoxelEditor Configuration File\n";
        stream << "# Generated automatically\n\n";
        
        saveSection(stream, m_rootSection, "");
        
        return stream.good();
    }
    
private:
    ConfigSection m_rootSection;
    Events::EventDispatcher* m_eventDispatcher = nullptr;
    mutable std::mutex m_mutex;
    std::atomic<bool> m_initialized{false};
    
    ConfigManager() = default;
    
    // Delete copy and move operations
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&) = delete;
    ConfigManager& operator=(ConfigManager&&) = delete;
    
    void ensureInitialized() {
        bool expected = false;
        if (m_initialized.compare_exchange_strong(expected, true)) {
            // Don't load defaults automatically - let client code do it
        }
    }
    
    void loadDefaultsNoLock() {
        m_rootSection.clear();
        
        // Application defaults
        m_rootSection.setValueByPath("app.name", std::string("VoxelEditor"));
        m_rootSection.setValueByPath("app.version", std::string("1.0.0"));
        m_rootSection.setValueByPath("app.debug", false);
        
        // Rendering defaults
        m_rootSection.setValueByPath("rendering.vsync", true);
        m_rootSection.setValueByPath("rendering.msaa", 4);
        m_rootSection.setValueByPath("rendering.max_fps", 60);
        m_rootSection.setValueByPath("rendering.resolution_width", 1280);
        m_rootSection.setValueByPath("rendering.resolution_height", 720);
        m_rootSection.setValueByPath("rendering.enable_grid", true);
        m_rootSection.setValueByPath("rendering.enable_shadows", true);
        m_rootSection.setValueByPath("rendering.show_wireframe", false);
        m_rootSection.setValueByPath("rendering.mode", std::string("normal"));
        
        // Workspace defaults
        m_rootSection.setValueByPath("workspace.size_default", 5.0f);
        m_rootSection.setValueByPath("workspace.size_min", 2.0f);
        m_rootSection.setValueByPath("workspace.size_max", 8.0f);
        m_rootSection.setValueByPath("workspace.auto_save", true);
        m_rootSection.setValueByPath("workspace.auto_save_interval", 300);
        
        // Camera defaults
        m_rootSection.setValueByPath("camera.fov", 45.0f);
        m_rootSection.setValueByPath("camera.near_plane", 0.1f);
        m_rootSection.setValueByPath("camera.far_plane", 100.0f);
        m_rootSection.setValueByPath("camera.sensitivity", 0.05f);
        m_rootSection.setValueByPath("camera.smooth_movement", true);
        m_rootSection.setValueByPath("camera.invert_y", false);
        
        // Input defaults
        m_rootSection.setValueByPath("input.mouse_sensitivity", 1.0f);
        m_rootSection.setValueByPath("input.scroll_sensitivity", 0.1f);
        m_rootSection.setValueByPath("input.double_click_time", 250);
        
        // Performance defaults
        m_rootSection.setValueByPath("performance.voxel_cache_size", 1000);
        m_rootSection.setValueByPath("performance.undo_history_size", 100);
        m_rootSection.setValueByPath("performance.render_distance", 50.0f);
    }
    
    std::shared_ptr<ConfigSection> getSectionByPath(const std::string& path) {
        if (path.empty()) {
            return nullptr;
        }
        
        ConfigSection* current = &m_rootSection;
        std::shared_ptr<ConfigSection> result;
        
        std::stringstream ss(path);
        std::string sectionName;
        
        while (std::getline(ss, sectionName, '.')) {
            if (!sectionName.empty()) {
                result = current->getSection(sectionName);
                current = result.get();
                if (!current) {
                    return nullptr;
                }
            }
        }
        
        return result;
    }
    
    std::shared_ptr<const ConfigSection> getSectionByPath(const std::string& path) const {
        if (path.empty()) {
            return nullptr;
        }
        
        const ConfigSection* current = &m_rootSection;
        std::shared_ptr<const ConfigSection> result;
        
        std::stringstream ss(path);
        std::string sectionName;
        
        while (std::getline(ss, sectionName, '.')) {
            if (!sectionName.empty()) {
                result = current->getSection(sectionName);
                current = result.get();
                if (!current) {
                    return nullptr;
                }
            }
        }
        
        return result;
    }
    
    void collectAllKeys(const ConfigSection& section, const std::string& prefix, std::vector<std::string>& keys) const {
        // Add direct keys
        for (const auto& key : section.getKeys()) {
            std::string fullKey = prefix.empty() ? key : prefix + "." + key;
            keys.push_back(fullKey);
        }
        
        // Add keys from subsections
        for (const auto& sectionName : section.getSectionNames()) {
            auto subsection = section.getSection(sectionName);
            if (subsection) {
                std::string newPrefix = prefix.empty() ? sectionName : prefix + "." + sectionName;
                collectAllKeys(*subsection, newPrefix, keys);
            }
        }
    }
    
    void saveSection(std::ostream& stream, const ConfigSection& section, const std::string& prefix) const {
        // Save direct values
        for (const auto& key : section.getKeys()) {
            std::string fullKey = prefix.empty() ? key : prefix + "." + key;
            ConfigValue value = section.getRawValue(key);
            
            if (value.isValid()) {
                stream << fullKey << "=" << value.toString() << "\n";
            }
        }
        
        // Save subsections
        for (const auto& sectionName : section.getSectionNames()) {
            auto subsection = section.getSection(sectionName);
            if (subsection && !subsection->isEmpty()) {
                std::string newPrefix = prefix.empty() ? sectionName : prefix + "." + sectionName;
                saveSection(stream, *subsection, newPrefix);
            }
        }
    }
    
    void setValueFromString(const std::string& key, const std::string& value) {
        // Try to determine the type based on the value
        if (value == "true" || value == "false") {
            bool boolValue = (value == "true");
            m_rootSection.setValueByPath(key, boolValue);
        } else if (value.find('.') != std::string::npos) {
            // Try float
            try {
                float floatValue = std::stof(value);
                m_rootSection.setValueByPath(key, floatValue);
            } catch (...) {
                // Fallback to string
                m_rootSection.setValueByPath(key, value);
            }
        } else {
            // Try integer
            try {
                int intValue = std::stoi(value);
                m_rootSection.setValueByPath(key, intValue);
            } catch (...) {
                // Fallback to string
                m_rootSection.setValueByPath(key, value);
            }
        }
    }
    
    std::string trim(const std::string& str) const {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            return "";
        }
        
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }
};

}
}