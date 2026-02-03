#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <iostream>
#include "ModelLoader.h"

// Forward declarations
struct Texture;
struct Sound;
class ModelLoader;
class TextureLoader;
class AudioSystem;

// Asset handle with reference counting
template<typename T>
class AssetHandle {
    std::shared_ptr<T> ptr;
    std::string path;
    
public:
    AssetHandle() = default;
    AssetHandle(std::shared_ptr<T> p, const std::string& pth) : ptr(p), path(pth) {}
    
    T* get() const { return ptr.get(); }
    T* operator->() const { return ptr.get(); }
    T& operator*() const { return *ptr; }
    
    bool isValid() const { return ptr != nullptr; }
    explicit operator bool() const { return isValid(); }
    
    const std::string& getPath() const { return path; }
    int useCount() const { return ptr.use_count(); }
};

class AssetManager {
public:
    struct Stats {
        size_t modelCount = 0;
        size_t textureCount = 0;
        size_t soundCount = 0;
        size_t totalMemoryMB = 0;
    };
    
private:
    // Asset caches
    std::unordered_map<std::string, std::shared_ptr<Model>> models;
    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
    std::unordered_map<std::string, std::shared_ptr<Sound>> sounds;
    
    // Loaders (injected dependencies)
    ModelLoader* modelLoader = nullptr;
    TextureLoader* textureLoader = nullptr;
    AudioSystem* audioSystem = nullptr;
    
    // Base directories
    std::string modelDir = "models/";
    std::string textureDir = "textures/";
    std::string soundDir = "sounds/";
    
    // Stats tracking
    Stats stats;
    
public:
    void init(ModelLoader* ml, TextureLoader* tl = nullptr, AudioSystem* as = nullptr) {
        modelLoader = ml;
        textureLoader = tl;
        audioSystem = as;
    }
    
    void setBaseDirectories(const std::string& modelPath, 
                           const std::string& texturePath = "", 
                           const std::string& soundPath = "") {
        modelDir = modelPath;
        if (!texturePath.empty()) textureDir = texturePath;
        if (!soundPath.empty()) soundDir = soundPath;
    }
    
    // === Model Loading ===
    
    AssetHandle<Model> loadModel(const std::string& filename) {
        std::string fullPath = modelDir + filename;
        
        // Check cache first
        auto it = models.find(fullPath);
        if (it != models.end()) {
            std::cout << "Asset cache hit: " << fullPath << " (refs: " << it->second.use_count() << ")" << std::endl;
            return AssetHandle<Model>(it->second, fullPath);
        }
        
        // Load new model
        if (!modelLoader) {
            std::cerr << "ModelLoader not initialized!" << std::endl;
            return AssetHandle<Model>();
        }
        
        std::cout << "Loading model: " << fullPath << std::endl;
        Model model = modelLoader->load(fullPath);
        
        if (model.vertices.empty()) {
            std::cerr << "Failed to load model: " << fullPath << std::endl;
            return AssetHandle<Model>();
        }
        
        // Store in cache
        auto sharedModel = std::make_shared<Model>(std::move(model));
        models[fullPath] = sharedModel;
        stats.modelCount++;
        
        return AssetHandle<Model>(sharedModel, fullPath);
    }
    
    AssetHandle<Model> getModel(const std::string& filename) {
        std::string fullPath = modelDir + filename;
        auto it = models.find(fullPath);
        if (it != models.end()) {
            return AssetHandle<Model>(it->second, fullPath);
        }
        return AssetHandle<Model>();
    }
    
    bool hasModel(const std::string& filename) const {
        std::string fullPath = modelDir + filename;
        return models.find(fullPath) != models.end();
    }
    
    void unloadModel(const std::string& filename) {
        std::string fullPath = modelDir + filename;
        auto it = models.find(fullPath);
        if (it != models.end()) {
            std::cout << "Unloading model: " << fullPath << " (refs: " << it->second.use_count() << ")" << std::endl;
            
            // Only unload if no external references
            if (it->second.use_count() <= 1) {
                // Cleanup GPU resources
                if (modelLoader) {
                    modelLoader->cleanup(*it->second);
                }
                models.erase(it);
                stats.modelCount--;
            } else {
                std::cout << "  Model still in use, keeping in cache" << std::endl;
            }
        }
    }
    
    // === Texture Loading ===
    
    AssetHandle<Texture> loadTexture(const std::string& filename) {
        std::string fullPath = textureDir + filename;
        
        auto it = textures.find(fullPath);
        if (it != textures.end()) {
            std::cout << "Texture cache hit: " << fullPath << std::endl;
            return AssetHandle<Texture>(it->second, fullPath);
        }
        
        if (!textureLoader) {
            std::cerr << "TextureLoader not initialized!" << std::endl;
            return AssetHandle<Texture>();
        }
        
        std::cout << "Loading texture: " << fullPath << std::endl;
        auto texture = std::make_shared<Texture>();
        
        if (!textureLoader->loadTexture(fullPath, *texture)) {
            std::cerr << "Failed to load texture: " << fullPath << std::endl;
            return AssetHandle<Texture>();
        }
        
        textures[fullPath] = texture;
        stats.textureCount++;
        
        return AssetHandle<Texture>(texture, fullPath);
    }
    
    AssetHandle<Texture> getTexture(const std::string& filename) {
        std::string fullPath = textureDir + filename;
        auto it = textures.find(fullPath);
        if (it != textures.end()) {
            return AssetHandle<Texture>(it->second, fullPath);
        }
        return AssetHandle<Texture>();
    }
    
    void unloadTexture(const std::string& filename) {
        std::string fullPath = textureDir + filename;
        auto it = textures.find(fullPath);
        if (it != textures.end()) {
            if (it->second.use_count() <= 1 && textureLoader) {
                textureLoader->destroyTexture(*it->second);
                textures.erase(it);
                stats.textureCount--;
            }
        }
    }
    
    // === Sound Loading ===
    
    AssetHandle<Sound> loadSound(const std::string& filename) {
        std::string fullPath = soundDir + filename;
        
        auto it = sounds.find(fullPath);
        if (it != sounds.end()) {
            return AssetHandle<Sound>(it->second, fullPath);
        }
        
        if (!audioSystem) {
            std::cerr << "AudioSystem not initialized!" << std::endl;
            return AssetHandle<Sound>();
        }
        
        std::cout << "Loading sound: " << fullPath << std::endl;
        auto sound = std::make_shared<Sound>();
        
        if (!audioSystem->loadSound(filename, fullPath)) {
            std::cerr << "Failed to load sound: " << fullPath << std::endl;
            return AssetHandle<Sound>();
        }
        
        sounds[fullPath] = sound;
        stats.soundCount++;
        
        return AssetHandle<Sound>(sound, fullPath);
    }
    
    // === Resource Management ===
    
    // Clean up assets with no external references
    void cleanupUnused() {
        std::cout << "\n=== Cleaning unused assets ===" << std::endl;
        
        // Cleanup models
        for (auto it = models.begin(); it != models.end();) {
            if (it->second.use_count() <= 1) {
                std::cout << "  Removing unused model: " << it->first << std::endl;
                if (modelLoader) {
                    modelLoader->cleanup(*it->second);
                }
                it = models.erase(it);
                stats.modelCount--;
            } else {
                ++it;
            }
        }
        
        // Cleanup textures
        for (auto it = textures.begin(); it != textures.end();) {
            if (it->second.use_count() <= 1) {
                std::cout << "  Removing unused texture: " << it->first << std::endl;
                if (textureLoader) {
                    textureLoader->destroyTexture(*it->second);
                }
                it = textures.erase(it);
                stats.textureCount--;
            } else {
                ++it;
            }
        }
        
        // Cleanup sounds
        for (auto it = sounds.begin(); it != sounds.end();) {
            if (it->second.use_count() <= 1) {
                std::cout << "  Removing unused sound: " << it->first << std::endl;
                it = sounds.erase(it);
                stats.soundCount--;
            } else {
                ++it;
            }
        }
        
        std::cout << "Cleanup complete. Remaining assets: "
                  << stats.modelCount << " models, "
                  << stats.textureCount << " textures, "
                  << stats.soundCount << " sounds" << std::endl;
    }
    
    // Force unload all assets
    void clear() {
        std::cout << "Clearing all assets..." << std::endl;
        
        // Clean up GPU resources
        if (modelLoader) {
            for (auto& [path, model] : models) {
                modelLoader->cleanup(*model);
            }
        }
        
        if (textureLoader) {
            for (auto& [path, texture] : textures) {
                textureLoader->destroyTexture(*texture);
            }
        }
        
        models.clear();
        textures.clear();
        sounds.clear();
        
        stats = Stats{};
    }
    
    // === Statistics ===
    
    const Stats& getStats() const { return stats; }
    
    void printStats() const {
        std::cout << "\n=== Asset Manager Stats ===" << std::endl;
        std::cout << "Models:   " << stats.modelCount << std::endl;
        std::cout << "Textures: " << stats.textureCount << std::endl;
        std::cout << "Sounds:   " << stats.soundCount << std::endl;
        std::cout << "Total:    " << (stats.modelCount + stats.textureCount + stats.soundCount) << std::endl;
    }
    
    void printDetailedStats() const {
        std::cout << "\n=== Detailed Asset Stats ===" << std::endl;
        
        std::cout << "\nModels (" << models.size() << "):" << std::endl;
        for (const auto& [path, model] : models) {
            std::cout << "  " << path << " (refs: " << model.use_count() << ")" << std::endl;
            std::cout << "    Vertices: " << model->vertices.size() 
                      << ", Indices: " << model->indices.size() << std::endl;
        }
        
        std::cout << "\nTextures (" << textures.size() << "):" << std::endl;
        for (const auto& [path, texture] : textures) {
            std::cout << "  " << path << " (refs: " << texture.use_count() << ")" << std::endl;
            if (texture) {
                std::cout << "    Size: " << texture->width << "x" << texture->height << std::endl;
            }
        }
        
        std::cout << "\nSounds (" << sounds.size() << "):" << std::endl;
        for (const auto& [path, sound] : sounds) {
            std::cout << "  " << path << " (refs: " << sound.use_count() << ")" << std::endl;
        }
    }
    
    // List all loaded assets
    std::vector<std::string> getLoadedModels() const {
        std::vector<std::string> result;
        for (const auto& [path, _] : models) {
            result.push_back(path);
        }
        return result;
    }
    
    std::vector<std::string> getLoadedTextures() const {
        std::vector<std::string> result;
        for (const auto& [path, _] : textures) {
            result.push_back(path);
        }
        return result;
    }
    
    std::vector<std::string> getLoadedSounds() const {
        std::vector<std::string> result;
        for (const auto& [path, _] : sounds) {
            result.push_back(path);
        }
        return result;
    }
};

// Global asset manager (optional singleton pattern)
// AssetManager& getGlobalAssetManager();
