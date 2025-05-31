#pragma once

#include "ConfigValue.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>

namespace VoxelEditor {
namespace Config {

class ConfigSection {
public:
    using ChangeCallback = std::function<void(const std::string&, const ConfigValue&, const ConfigValue&)>;
    
    ConfigSection(const std::string& name = "") : m_name(name) {}
    
    // Value management
    template<typename T>
    T getValue(const std::string& key, const T& defaultValue = T{}) const {
        auto it = m_values.find(key);
        if (it == m_values.end()) {
            return defaultValue;
        }
        
        return getValueOr(it->second, defaultValue);
    }
    
    template<typename T>
    void setValue(const std::string& key, const T& value) {
        ConfigValue oldValue;
        auto it = m_values.find(key);
        if (it != m_values.end()) {
            oldValue = it->second;
        }
        
        ConfigValue newValue(value);
        m_values[key] = newValue;
        
        // Notify callbacks
        notifyChange(key, oldValue, newValue);
    }
    
    bool hasValue(const std::string& key) const {
        auto it = m_values.find(key);
        return it != m_values.end() && it->second.isValid();
    }
    
    void removeValue(const std::string& key) {
        auto it = m_values.find(key);
        if (it != m_values.end()) {
            ConfigValue oldValue = it->second;
            m_values.erase(it);
            notifyChange(key, oldValue, ConfigValue{});
        }
    }
    
    std::vector<std::string> getKeys() const {
        std::vector<std::string> keys;
        keys.reserve(m_values.size());
        
        for (const auto& pair : m_values) {
            keys.push_back(pair.first);
        }
        
        return keys;
    }
    
    // Section management
    std::shared_ptr<ConfigSection> getSection(const std::string& name) {
        auto it = m_sections.find(name);
        if (it != m_sections.end()) {
            return it->second;
        }
        
        // Create new section
        auto section = std::make_shared<ConfigSection>(name);
        m_sections[name] = section;
        return section;
    }
    
    std::shared_ptr<const ConfigSection> getSection(const std::string& name) const {
        auto it = m_sections.find(name);
        if (it != m_sections.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    bool hasSection(const std::string& name) const {
        return m_sections.find(name) != m_sections.end();
    }
    
    void removeSection(const std::string& name) {
        m_sections.erase(name);
    }
    
    std::vector<std::string> getSectionNames() const {
        std::vector<std::string> names;
        names.reserve(m_sections.size());
        
        for (const auto& pair : m_sections) {
            names.push_back(pair.first);
        }
        
        return names;
    }
    
    // Path-based access (dot notation: "section.subsection.key")
    template<typename T>
    T getValueByPath(const std::string& path, const T& defaultValue = T{}) const {
        size_t dotPos = path.find('.');
        if (dotPos == std::string::npos) {
            // No dot, it's a direct value
            return getValue(path, defaultValue);
        }
        
        // Split path
        std::string sectionName = path.substr(0, dotPos);
        std::string remainingPath = path.substr(dotPos + 1);
        
        auto section = getSection(sectionName);
        if (!section) {
            return defaultValue;
        }
        
        return section->getValueByPath(remainingPath, defaultValue);
    }
    
    template<typename T>
    void setValueByPath(const std::string& path, const T& value) {
        size_t dotPos = path.find('.');
        if (dotPos == std::string::npos) {
            // No dot, it's a direct value
            setValue(path, value);
            return;
        }
        
        // Split path
        std::string sectionName = path.substr(0, dotPos);
        std::string remainingPath = path.substr(dotPos + 1);
        
        auto section = getSection(sectionName); // Creates section if it doesn't exist
        section->setValueByPath(remainingPath, value);
    }
    
    bool hasValueByPath(const std::string& path) const {
        size_t dotPos = path.find('.');
        if (dotPos == std::string::npos) {
            return hasValue(path);
        }
        
        std::string sectionName = path.substr(0, dotPos);
        std::string remainingPath = path.substr(dotPos + 1);
        
        auto section = getSection(sectionName);
        if (!section) {
            return false;
        }
        
        return section->hasValueByPath(remainingPath);
    }
    
    // Change notifications
    void subscribe(const std::string& key, ChangeCallback callback) {
        m_callbacks[key].push_back(callback);
    }
    
    void unsubscribe(const std::string& key) {
        m_callbacks.erase(key);
    }
    
    // Utility
    void clear() {
        m_values.clear();
        m_sections.clear();
        m_callbacks.clear();
    }
    
    bool isEmpty() const {
        return m_values.empty() && m_sections.empty();
    }
    
    size_t size() const {
        return m_values.size();
    }
    
    size_t sectionCount() const {
        return m_sections.size();
    }
    
    const std::string& getName() const {
        return m_name;
    }
    
private:
    std::string m_name;
    std::unordered_map<std::string, ConfigValue> m_values;
    std::unordered_map<std::string, std::shared_ptr<ConfigSection>> m_sections;
    std::unordered_map<std::string, std::vector<ChangeCallback>> m_callbacks;
    
    void notifyChange(const std::string& key, const ConfigValue& oldValue, const ConfigValue& newValue) {
        auto it = m_callbacks.find(key);
        if (it != m_callbacks.end()) {
            for (const auto& callback : it->second) {
                if (callback) {
                    callback(key, oldValue, newValue);
                }
            }
        }
    }
};

}
}