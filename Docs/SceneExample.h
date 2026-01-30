#pragma once
#include "ScenePackage.h"
#include "BinaryIO.h"
#include <glm/glm.hpp>
#include <iostream>

// Example: Your scene data structures
struct EntityData {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    uint32_t modelIndex;      // Index into resource table
    uint32_t materialIndex;
    char name[64];
};

struct SceneMetadata {
    char name[128];
    char author[64];
    uint32_t version;
    float ambientLight[3];
    float gravity[3];
};

struct SceneData {
    SceneMetadata metadata;
    uint32_t entityCount;
    // Followed by EntityData[entityCount] in the binary stream
};

// Helper class to build and save scenes
class SceneBuilder {
public:
    SceneBuilder() {
        std::memset(&metadata, 0, sizeof(metadata));
        std::strncpy(metadata.name, "Untitled Scene", sizeof(metadata.name));
        std::strncpy(metadata.author, "Unknown", sizeof(metadata.author));
        metadata.version = 1;
        metadata.ambientLight[0] = 0.3f;
        metadata.ambientLight[1] = 0.3f;
        metadata.ambientLight[2] = 0.3f;
        metadata.gravity[0] = 0.0f;
        metadata.gravity[1] = -9.8f;
        metadata.gravity[2] = 0.0f;
    }
    
    void setMetadata(const char* name, const char* author) {
        std::strncpy(metadata.name, name, sizeof(metadata.name) - 1);
        std::strncpy(metadata.author, author, sizeof(metadata.author) - 1);
    }
    
    void setAmbientLight(float r, float g, float b) {
        metadata.ambientLight[0] = r;
        metadata.ambientLight[1] = g;
        metadata.ambientLight[2] = b;
    }
    
    void setGravity(float x, float y, float z) {
        metadata.gravity[0] = x;
        metadata.gravity[1] = y;
        metadata.gravity[2] = z;
    }
    
    // Add entity to scene
    uint32_t addEntity(const char* name, 
                      glm::vec3 pos, 
                      glm::vec3 rot, 
                      glm::vec3 scale,
                      uint32_t modelIdx,
                      uint32_t materialIdx) {
        EntityData entity{};
        std::strncpy(entity.name, name, sizeof(entity.name) - 1);
        entity.position = pos;
        entity.rotation = rot;
        entity.scale = scale;
        entity.modelIndex = modelIdx;
        entity.materialIndex = materialIdx;
        
        entities.push_back(entity);
        return static_cast<uint32_t>(entities.size() - 1);
    }
    
    // Add model resource and get its index
    uint32_t addModel(const std::string& filepath, 
                     const std::string& virtualPath = "") {
        uint32_t index = static_cast<uint32_t>(modelPaths.size());
        modelPaths.push_back(filepath);
        modelVirtualPaths.push_back(virtualPath.empty() ? 
            std::filesystem::path(filepath).filename().string() : virtualPath);
        return index;
    }
    
    // Add texture resource
    uint32_t addTexture(const std::string& filepath,
                       const std::string& virtualPath = "") {
        uint32_t index = static_cast<uint32_t>(texturePaths.size());
        texturePaths.push_back(filepath);
        textureVirtualPaths.push_back(virtualPath.empty() ?
            std::filesystem::path(filepath).filename().string() : virtualPath);
        return index;
    }
    
    // Add script resource
    uint32_t addScript(const std::string& filepath,
                      const std::string& virtualPath = "") {
        uint32_t index = static_cast<uint32_t>(scriptPaths.size());
        scriptPaths.push_back(filepath);
        scriptVirtualPaths.push_back(virtualPath.empty() ?
            std::filesystem::path(filepath).filename().string() : virtualPath);
        return index;
    }
    
    // Build and save to .zscene package
    bool save(const std::string& outputPath, bool compress = false) {
        using namespace ScenePackage;
        
        // Create package writer
        PackageWriter pkg;
        
        // Add all model resources
        for (size_t i = 0; i < modelPaths.size(); i++) {
            std::cout << "Adding model: " << modelPaths[i] << std::endl;
            if (!pkg.addResourceFromFile(modelPaths[i], 
                                        ResourceType::Model,
                                        "models/" + modelVirtualPaths[i],
                                        compress ? CompressionType::Deflate : CompressionType::None)) {
                std::cerr << "Failed to add model: " << modelPaths[i] << std::endl;
                return false;
            }
        }
        
        // Add all texture resources
        for (size_t i = 0; i < texturePaths.size(); i++) {
            std::cout << "Adding texture: " << texturePaths[i] << std::endl;
            if (!pkg.addResourceFromFile(texturePaths[i],
                                        ResourceType::Texture,
                                        "textures/" + textureVirtualPaths[i],
                                        compress ? CompressionType::Deflate : CompressionType::None)) {
                std::cerr << "Failed to add texture: " << texturePaths[i] << std::endl;
                return false;
            }
        }
        
        // Add all script resources
        for (size_t i = 0; i < scriptPaths.size(); i++) {
            std::cout << "Adding script: " << scriptPaths[i] << std::endl;
            if (!pkg.addResourceFromFile(scriptPaths[i],
                                        ResourceType::Script,
                                        "scripts/" + scriptVirtualPaths[i],
                                        CompressionType::None)) { // Scripts usually small, no compression
                std::cerr << "Failed to add script: " << scriptPaths[i] << std::endl;
                return false;
            }
        }
        
        // Build binary scene data
        std::vector<uint8_t> sceneData = buildSceneData();
        pkg.setSceneData(sceneData);
        
        // Write package
        std::cout << "Writing package: " << outputPath << std::endl;
        std::cout << "  Resources: " << pkg.getResourceCount() << std::endl;
        std::cout << "  Scene data: " << sceneData.size() << " bytes" << std::endl;
        std::cout << "  Estimated size: " << pkg.estimateSize() << " bytes" << std::endl;
        
        return pkg.write(outputPath);
    }
    
    // Clear all data
    void clear() {
        entities.clear();
        modelPaths.clear();
        modelVirtualPaths.clear();
        texturePaths.clear();
        textureVirtualPaths.clear();
        scriptPaths.clear();
        scriptVirtualPaths.clear();
    }
    
private:
    SceneMetadata metadata;
    std::vector<EntityData> entities;
    
    std::vector<std::string> modelPaths;
    std::vector<std::string> modelVirtualPaths;
    std::vector<std::string> texturePaths;
    std::vector<std::string> textureVirtualPaths;
    std::vector<std::string> scriptPaths;
    std::vector<std::string> scriptVirtualPaths;
    
    std::vector<uint8_t> buildSceneData() {
        std::vector<uint8_t> data;
        
        // Calculate total size
        size_t totalSize = sizeof(SceneMetadata) + 
                          sizeof(uint32_t) + 
                          entities.size() * sizeof(EntityData);
        
        data.resize(totalSize);
        uint8_t* ptr = data.data();
        
        // Write metadata
        std::memcpy(ptr, &metadata, sizeof(SceneMetadata));
        ptr += sizeof(SceneMetadata);
        
        // Write entity count
        uint32_t entityCount = static_cast<uint32_t>(entities.size());
        std::memcpy(ptr, &entityCount, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
        
        // Write entities
        if (!entities.empty()) {
            std::memcpy(ptr, entities.data(), entities.size() * sizeof(EntityData));
        }
        
        return data;
    }
};

// Helper class to load scenes
class SceneLoader {
public:
    bool load(const std::string& filepath) {
        using namespace ScenePackage;
        
        if (!reader.open(filepath)) {
            std::cerr << "Failed to open scene: " << filepath << std::endl;
            return false;
        }
        
        std::cout << "Loading scene: " << filepath << std::endl;
        std::cout << "  Version: " << reader.getHeader().version << std::endl;
        std::cout << "  Resources: " << reader.getResourceCount() << std::endl;
        
        // Read scene data
        auto sceneData = reader.readSceneData();
        if (sceneData.empty()) {
            std::cerr << "Failed to read scene data" << std::endl;
            return false;
        }
        
        // Parse scene data
        const uint8_t* ptr = sceneData.data();
        
        // Read metadata
        std::memcpy(&metadata, ptr, sizeof(SceneMetadata));
        ptr += sizeof(SceneMetadata);
        
        std::cout << "  Scene name: " << metadata.name << std::endl;
        std::cout << "  Author: " << metadata.author << std::endl;
        
        // Read entity count
        uint32_t entityCount;
        std::memcpy(&entityCount, ptr, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
        
        // Read entities
        entities.resize(entityCount);
        if (entityCount > 0) {
            std::memcpy(entities.data(), ptr, entityCount * sizeof(EntityData));
        }
        
        std::cout << "  Entities: " << entityCount << std::endl;
        
        return true;
    }
    
    // Extract a specific resource
    std::vector<uint8_t> getResource(const std::string& virtualPath) {
        int index = reader.findResourceByPath(virtualPath);
        if (index < 0) return {};
        return reader.readResource(index);
    }
    
    // Extract resource to temp file and return path
    std::string extractResourceToTemp(const std::string& virtualPath) {
        int index = reader.findResourceByPath(virtualPath);
        if (index < 0) return "";
        
        auto& entry = reader.getResourceEntries()[index];
        std::string tempPath = std::filesystem::temp_directory_path().string() + 
                              "/" + entry.name;
        
        if (!reader.extractResource(index, tempPath)) {
            return "";
        }
        
        return tempPath;
    }
    
    // Extract all resources to directory
    bool extractAll(const std::string& outputDir) {
        return reader.extractAll(outputDir);
    }
    
    // Getters
    const SceneMetadata& getMetadata() const { return metadata; }
    const std::vector<EntityData>& getEntities() const { return entities; }
    const std::vector<ScenePackage::ResourceEntry>& getResources() const {
        return reader.getResourceEntries();
    }
    
    void close() {
        reader.close();
    }
    
private:
    ScenePackage::PackageReader reader;
    SceneMetadata metadata;
    std::vector<EntityData> entities;
};

// Example usage
inline void exampleUsage() {
    // === CREATING A SCENE ===
    {
        SceneBuilder builder;
        
        // Set scene metadata
        builder.setMetadata("Forest Scene", "YourName");
        builder.setAmbientLight(0.4f, 0.4f, 0.5f);
        builder.setGravity(0.0f, -9.8f, 0.0f);
        
        // Add resources
        uint32_t treeModel = builder.addModel("models/tree.glb");
        uint32_t rockModel = builder.addModel("models/rock.glb");
        uint32_t grassTexture = builder.addTexture("textures/grass.png");
        uint32_t treeScript = builder.addScript("scripts/sway.lua");
        
        // Add entities using those resources
        builder.addEntity("Tree1", 
                         glm::vec3(0, 0, 0),
                         glm::vec3(0, 0, 0),
                         glm::vec3(1, 1, 1),
                         treeModel, 0);
        
        builder.addEntity("Tree2",
                         glm::vec3(5, 0, 3),
                         glm::vec3(0, 45, 0),
                         glm::vec3(1.2f, 1.2f, 1.2f),
                         treeModel, 0);
        
        builder.addEntity("Rock1",
                         glm::vec3(-3, 0, -2),
                         glm::vec3(0, 0, 0),
                         glm::vec3(0.8f, 0.8f, 0.8f),
                         rockModel, 0);
        
        // Save with compression
        if (builder.save("scenes/forest.zscene", true)) {
            std::cout << "Scene saved successfully!" << std::endl;
        }
    }
    
    // === LOADING A SCENE ===
    {
        SceneLoader loader;
        
        if (loader.load("scenes/forest.zscene")) {
            std::cout << "\nScene loaded!" << std::endl;
            
            // Access metadata
            auto& meta = loader.getMetadata();
            std::cout << "Scene: " << meta.name << std::endl;
            
            // Access entities
            for (auto& entity : loader.getEntities()) {
                std::cout << "Entity: " << entity.name 
                         << " at (" << entity.position.x << ", "
                         << entity.position.y << ", "
                         << entity.position.z << ")" << std::endl;
            }
            
            // List all resources
            std::cout << "\nResources:" << std::endl;
            for (auto& res : loader.getResources()) {
                std::cout << "  " << res.virtualPath 
                         << " (" << res.dataSize << " bytes)" << std::endl;
            }
            
            // Extract a specific model to use
            std::string treeModelPath = loader.extractResourceToTemp("models/tree.glb");
            if (!treeModelPath.empty()) {
                std::cout << "\nExtracted tree model to: " << treeModelPath << std::endl;
                // Now you can load it: ModelLoader::load(treeModelPath)
            }
            
            // Or extract everything
            // loader.extractAll("temp/scene_resources/");
            
            loader.close();
        }
    }
}
