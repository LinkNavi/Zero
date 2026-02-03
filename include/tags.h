#pragma once
#include "Engine.h"
#include <string>
#include <unordered_set>
#include <vector>

// Tag component - simple string identifier
struct Tag : Component {
    std::string name;
    
    Tag() = default;
    Tag(const std::string& n) : name(n) {}
    
    bool operator==(const std::string& other) const { return name == other; }
};

// Layer component - bitmask for collision/rendering layers
struct Layer : Component {
    uint32_t mask = 1; // Default layer 0
    
    Layer() = default;
    Layer(int layer) { setLayer(layer); }
    
    void setLayer(int layer) { 
        if (layer >= 0 && layer < 32) {
            mask = 1 << layer;
        }
    }
    
    void addLayer(int layer) {
        if (layer >= 0 && layer < 32) {
            mask |= (1 << layer);
        }
    }
    
    void removeLayer(int layer) {
        if (layer >= 0 && layer < 32) {
            mask &= ~(1 << layer);
        }
    }
    
    bool hasLayer(int layer) const {
        if (layer >= 0 && layer < 32) {
            return (mask & (1 << layer)) != 0;
        }
        return false;
    }
    
    bool intersects(const Layer& other) const {
        return (mask & other.mask) != 0;
    }
    
    bool intersects(uint32_t otherMask) const {
        return (mask & otherMask) != 0;
    }
};

// Predefined layer names (optional, can be customized)
namespace Layers {
    constexpr int Default = 0;
    constexpr int Player = 1;
    constexpr int Enemy = 2;
    constexpr int Environment = 3;
    constexpr int Projectile = 4;
    constexpr int UI = 5;
    constexpr int Trigger = 6;
    constexpr int Invisible = 7;
    // Layers 8-31 available for custom use
}

// Helper functions for querying entities by tag/layer
class TagLayerQuery {
public:
    // Find first entity with tag
    static EntityID findEntityWithTag(ECS* ecs, const std::string& tagName) {
        // Iterate through all entities (in a real engine, you'd have a tag index)
        for (size_t i = 0; i < 10000; ++i) { // Reasonable entity limit
            auto* tag = ecs->getComponent<Tag>(i);
            if (tag && tag->name == tagName) {
                return i;
            }
        }
        return 0; // Invalid entity
    }
    
    // Find all entities with tag
    static std::vector<EntityID> findEntitiesWithTag(ECS* ecs, const std::string& tagName) {
        std::vector<EntityID> results;
        for (size_t i = 0; i < 10000; ++i) {
            auto* tag = ecs->getComponent<Tag>(i);
            if (tag && tag->name == tagName) {
                results.push_back(i);
            }
        }
        return results;
    }
    
    // Find all entities on specific layer
    static std::vector<EntityID> findEntitiesOnLayer(ECS* ecs, int layer) {
        std::vector<EntityID> results;
        for (size_t i = 0; i < 10000; ++i) {
            auto* layerComp = ecs->getComponent<Layer>(i);
            if (layerComp && layerComp->hasLayer(layer)) {
                results.push_back(i);
            }
        }
        return results;
    }
    
    // Find entities matching layer mask
    static std::vector<EntityID> findEntitiesOnLayers(ECS* ecs, uint32_t layerMask) {
        std::vector<EntityID> results;
        for (size_t i = 0; i < 10000; ++i) {
            auto* layerComp = ecs->getComponent<Layer>(i);
            if (layerComp && layerComp->intersects(layerMask)) {
                results.push_back(i);
            }
        }
        return results;
    }
    
    // Find entities with specific tag AND on specific layer
    static std::vector<EntityID> findEntitiesWithTagAndLayer(ECS* ecs, 
                                                             const std::string& tagName, 
                                                             int layer) {
        std::vector<EntityID> results;
        for (size_t i = 0; i < 10000; ++i) {
            auto* tag = ecs->getComponent<Tag>(i);
            auto* layerComp = ecs->getComponent<Layer>(i);
            if (tag && tag->name == tagName && 
                layerComp && layerComp->hasLayer(layer)) {
                results.push_back(i);
            }
        }
        return results;
    }
    
    // Check if two entities can interact based on layers
    static bool canInteract(ECS* ecs, EntityID a, EntityID b) {
        auto* layerA = ecs->getComponent<Layer>(a);
        auto* layerB = ecs->getComponent<Layer>(b);
        
        if (!layerA || !layerB) return true; // If no layer, assume they can interact
        
        return layerA->intersects(*layerB);
    }
};

// Tag manager - maintains index for fast lookups (optional optimization)
class TagManager {
    std::unordered_map<std::string, std::unordered_set<EntityID>> tagIndex;
    ECS* ecs = nullptr;
    
public:
    void init(ECS* ecsInstance) {
        ecs = ecsInstance;
    }
    
    void addTag(EntityID entity, const std::string& tag) {
        tagIndex[tag].insert(entity);
    }
    
    void removeTag(EntityID entity, const std::string& tag) {
        auto it = tagIndex.find(tag);
        if (it != tagIndex.end()) {
            it->second.erase(entity);
            if (it->second.empty()) {
                tagIndex.erase(it);
            }
        }
    }
    
    void removeEntity(EntityID entity) {
        for (auto& [tag, entities] : tagIndex) {
            entities.erase(entity);
        }
    }
    
    EntityID findFirstWithTag(const std::string& tag) const {
        auto it = tagIndex.find(tag);
        if (it != tagIndex.end() && !it->second.empty()) {
            return *it->second.begin();
        }
        return 0;
    }
    
    std::vector<EntityID> findAllWithTag(const std::string& tag) const {
        auto it = tagIndex.find(tag);
        if (it != tagIndex.end()) {
            return std::vector<EntityID>(it->second.begin(), it->second.end());
        }
        return {};
    }
    
    bool hasTag(EntityID entity, const std::string& tag) const {
        auto it = tagIndex.find(tag);
        return it != tagIndex.end() && it->second.count(entity) > 0;
    }
    
    void clear() {
        tagIndex.clear();
    }
    
    // Rebuild index from ECS (useful after loading scene)
    void rebuildIndex() {
        tagIndex.clear();
        if (!ecs) return;
        
        for (size_t i = 0; i < 10000; ++i) {
            auto* tag = ecs->getComponent<Tag>(i);
            if (tag && !tag->name.empty()) {
                tagIndex[tag->name].insert(i);
            }
        }
    }
};
