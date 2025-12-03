
#pragma once
#include "Objects.h"
#include <raylib.h>
#include <memory>
#include <unordered_map>
#include <string>

class ResourceManager {
public:
    ResourceManager() = default;
    ~ResourceManager() = default;

    // Get/caches a unit cube mesh; returns shared_ptr (shared ownership)
    std::shared_ptr<ZeroMesh> GetCube(float size = 1.0f) {
        std::string key = "cube_" + std::to_string(size);
        auto it = meshes.find(key);
        if (it != meshes.end()) return it->second;

        Mesh mesh = GenMeshCube(size, size, size);
        ZeroMesh mm = ZeroMesh::FromMesh(mesh);
        auto ptr = std::make_shared<ZeroMesh>(std::move(mm));
        meshes.emplace(key, ptr);
        return ptr;
    }

    // Return default shader (may be a null shader with id==0)
    Shader GetDefaultShader() {
        if (!defaultShaderLoaded) {
            // Many raylib versions do not include LoadShaderDefault, so we use an "empty" shader
            defaultShader = { 0 };
            defaultShaderLoaded = true;
        }
        return defaultShader;
    }

    // Cleanup loaded resources (optional)
    void Shutdown() {
        meshes.clear(); // ZeroMesh destructor unloads models
        if (defaultShaderLoaded && defaultShader.id != 0) UnloadShader(defaultShader);
        defaultShaderLoaded = false;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<ZeroMesh>> meshes;
    Shader defaultShader{};
    bool defaultShaderLoaded = false;
};
