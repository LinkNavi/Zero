// include/ResourcePath.h
#pragma once
#include <string>
#include <filesystem>
#include <unistd.h>
#include "iostream"
#include <linux/limits.h>

class ResourcePath {
    static std::string projectRoot;
    
public:
    static void init() {
        // Try to find project root from executable location
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        std::string exePath(result, (count > 0) ? count : 0);
        
        // Assume executable is in build/ directory, so parent is project root
        std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
        projectRoot = exeDir.parent_path().string();
        
        // Verify by checking for a known file
        if (!std::filesystem::exists(projectRoot + "/config.ini")) {
            // Fallback: use current working directory
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd)) != nullptr) {
                projectRoot = std::string(cwd);
            }
        }
        
        std::cout << "✓ Resource root: " << projectRoot << std::endl;
    }
    
    static std::string get(const std::string& relativePath) {
        std::filesystem::path fullPath = std::filesystem::path(projectRoot) / relativePath;
        return fullPath.string();
    }
    static std::string scenes(const std::string& name) {
    return "scenes/" + name;
}
static std::string textures(const std::string& name) {
    return "textures/" + name;
}
    static std::string models(const std::string& filename) {
        return get("models/" + filename);
    }
    
    static std::string shaders(const std::string& filename) {
        return get("shaders/" + filename);
    }
    
    static std::string config(const std::string& filename) {
        return get(filename);
    }
};

std::string ResourcePath::projectRoot;
