#pragma once

#include <any>
#include <typeindex>
#include <string>
#include <stdexcept>
#include <sstream>
#include <type_traits>

namespace VoxelEditor {
namespace Config {

class ConfigValue {
public:
    ConfigValue() : m_type(std::type_index(typeid(void))) {}
    
    template<typename T>
    ConfigValue(const T& value) 
        : m_value(value), m_type(std::type_index(typeid(T))) {}
    
    template<typename T>
    T get() const {
        if (m_type != std::type_index(typeid(T))) {
            throw std::bad_cast();
        }
        
        try {
            return std::any_cast<T>(m_value);
        } catch (const std::bad_any_cast& e) {
            throw std::runtime_error("ConfigValue type mismatch");
        }
    }
    
    template<typename T>
    void set(const T& value) {
        m_value = value;
        m_type = std::type_index(typeid(T));
    }
    
    std::string getTypeName() const {
        if (m_type == std::type_index(typeid(void))) {
            return "void";
        } else if (m_type == std::type_index(typeid(bool))) {
            return "bool";
        } else if (m_type == std::type_index(typeid(int))) {
            return "int";
        } else if (m_type == std::type_index(typeid(float))) {
            return "float";
        } else if (m_type == std::type_index(typeid(double))) {
            return "double";
        } else if (m_type == std::type_index(typeid(std::string))) {
            return "string";
        } else {
            return m_type.name();
        }
    }
    
    bool isValid() const {
        return m_value.has_value();
    }
    
    bool isEmpty() const {
        return !m_value.has_value() || m_type == std::type_index(typeid(void));
    }
    
    std::string toString() const {
        if (!isValid()) {
            return "";
        }
        
        if (m_type == std::type_index(typeid(bool))) {
            return std::any_cast<bool>(m_value) ? "true" : "false";
        } else if (m_type == std::type_index(typeid(int))) {
            return std::to_string(std::any_cast<int>(m_value));
        } else if (m_type == std::type_index(typeid(float))) {
            return std::to_string(std::any_cast<float>(m_value));
        } else if (m_type == std::type_index(typeid(double))) {
            return std::to_string(std::any_cast<double>(m_value));
        } else if (m_type == std::type_index(typeid(std::string))) {
            return std::any_cast<std::string>(m_value);
        } else {
            return "<complex_type>";
        }
    }
    
    void fromString(const std::string& str, const std::type_index& targetType) {
        if (targetType == std::type_index(typeid(bool))) {
            bool value = (str == "true" || str == "1" || str == "yes");
            set(value);
        } else if (targetType == std::type_index(typeid(int))) {
            int value = std::stoi(str);
            set(value);
        } else if (targetType == std::type_index(typeid(float))) {
            float value = std::stof(str);
            set(value);
        } else if (targetType == std::type_index(typeid(double))) {
            double value = std::stod(str);
            set(value);
        } else if (targetType == std::type_index(typeid(std::string))) {
            set(str);
        } else {
            throw std::runtime_error("Unsupported type for string conversion");
        }
    }
    
    std::type_index getType() const {
        return m_type;
    }
    
    // Template specializations for common types
    template<typename T>
    bool canConvertTo() const {
        return m_type == std::type_index(typeid(T));
    }
    
private:
    std::any m_value;
    std::type_index m_type;
};

// Helper function for type-safe retrieval with default
template<typename T>
T getValueOr(const ConfigValue& value, const T& defaultValue) {
    if (!value.isValid() || !value.canConvertTo<T>()) {
        return defaultValue;
    }
    
    try {
        return value.get<T>();
    } catch (...) {
        return defaultValue;
    }
}

}
}