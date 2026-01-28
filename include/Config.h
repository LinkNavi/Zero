#pragma once
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>

class Config {
    std::unordered_map<std::string, std::string> values;
    
public:
    bool load(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) return false;
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                values[key] = value;
            }
        }
        return true;
    }
    
    bool save(const std::string& filepath) {
        std::ofstream file(filepath);
        if (!file.is_open()) return false;
        
        for (const auto& [key, value] : values) {
            file << key << " = " << value << "\n";
        }
        return true;
    }
    
    template<typename T>
    T get(const std::string& key, T defaultValue) const {
        auto it = values.find(key);
        if (it == values.end()) return defaultValue;
        
        std::istringstream iss(it->second);
        T value;
        iss >> value;
        return value;
    }
    
    std::string getString(const std::string& key, const std::string& defaultValue = "") const {
        auto it = values.find(key);
        return (it != values.end()) ? it->second : defaultValue;
    }
    
    glm::vec3 getVec3(const std::string& key, glm::vec3 defaultValue = glm::vec3(0)) const {
        auto it = values.find(key);
        if (it == values.end()) return defaultValue;
        
        glm::vec3 v;
        std::istringstream iss(it->second);
        char comma;
        iss >> v.x >> comma >> v.y >> comma >> v.z;
        return v;
    }
    
    template<typename T>
    void set(const std::string& key, T value) {
        std::ostringstream oss;
        oss << value;
        values[key] = oss.str();
    }
    
    void setVec3(const std::string& key, glm::vec3 v) {
        std::ostringstream oss;
        oss << v.x << "," << v.y << "," << v.z;
        values[key] = oss.str();
    }
};
