#pragma once
#include "Engine.h"
#include <typeindex>
#include <unordered_map>
#include <string>
#include <functional>
#include <iostream>

// Component factory function
using ComponentFactory = std::function<Component*()>;
using ComponentDeleter = std::function<void(Component*)>;

// Component metadata
struct ComponentMetadata {
    std::string name;
    std::type_index typeIndex;
    ComponentFactory factory;
    ComponentDeleter deleter;
    size_t size = 0;
    
    ComponentMetadata(const std::string& n, std::type_index ti, 
                     ComponentFactory f, ComponentDeleter d, size_t s)
        : name(n), typeIndex(ti), factory(f), deleter(d), size(s) {}
};

// Component registry for runtime type information
class ComponentRegistry {
    std::unordered_map<std::type_index, ComponentMetadata> typeToMetadata;
    std::unordered_map<std::string, std::type_index> nameToType;
    std::unordered_map<std::type_index, uint8_t> typeToID;
    uint8_t nextID = 0;
    
public:
    // Register component type
    template<typename T>
    void registerComponent(const std::string& name) {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
        
        std::type_index typeIndex(typeid(T));
        
        // Check if already registered
        if (typeToMetadata.find(typeIndex) != typeToMetadata.end()) {
            std::cout << "Component already registered: " << name << std::endl;
            return;
        }
        
        // Create metadata
        ComponentFactory factory = []() -> Component* {
            return new T();
        };
        
        ComponentDeleter deleter = [](Component* comp) {
            delete static_cast<T*>(comp);
        };
        
        ComponentMetadata metadata(name, typeIndex, factory, deleter, sizeof(T));
        
        typeToMetadata.emplace(typeIndex, metadata);
        nameToType.insert_or_assign(name, typeIndex);        typeToID[typeIndex] = nextID++;
        
        std::cout << "Registered component: " << name 
                  << " (size: " << sizeof(T) << " bytes, ID: " 
                  << (int)typeToID[typeIndex] << ")" << std::endl;
    }
    
    // Get component name from type
    template<typename T>
    std::string getComponentName() const {
        std::type_index typeIndex(typeid(T));
        auto it = typeToMetadata.find(typeIndex);
        return it != typeToMetadata.end() ? it->second.name : "Unknown";
    }
    
    // Get component name from type index
    std::string getComponentName(std::type_index typeIndex) const {
        auto it = typeToMetadata.find(typeIndex);
        return it != typeToMetadata.end() ? it->second.name : "Unknown";
    }
    
    // Get type index from name
    std::type_index getTypeIndex(const std::string& name) const {
        auto it = nameToType.find(name);
        if (it != nameToType.end()) {
            return it->second;
        }
        return std::type_index(typeid(void));
    }
    
    // Check if component type is registered
    template<typename T>
    bool isRegistered() const {
        return typeToMetadata.find(std::type_index(typeid(T))) != typeToMetadata.end();
    }
    
    bool isRegistered(const std::string& name) const {
        return nameToType.find(name) != nameToType.end();
    }
    
    // Get component ID (for ECS bit masks)
    template<typename T>
    uint8_t getComponentID() const {
        std::type_index typeIndex(typeid(T));
        auto it = typeToID.find(typeIndex);
        return it != typeToID.end() ? it->second : 255;
    }
    
    uint8_t getComponentID(const std::string& name) const {
        auto it = nameToType.find(name);
        if (it != nameToType.end()) {
            auto idIt = typeToID.find(it->second);
            if (idIt != typeToID.end()) {
                return idIt->second;
            }
        }
        return 255;
    }
    
    // Get metadata
    const ComponentMetadata* getMetadata(std::type_index typeIndex) const {
        auto it = typeToMetadata.find(typeIndex);
        return it != typeToMetadata.end() ? &it->second : nullptr;
    }
    
    const ComponentMetadata* getMetadata(const std::string& name) const {
        auto it = nameToType.find(name);
        if (it != nameToType.end()) {
            return getMetadata(it->second);
        }
        return nullptr;
    }
    
    // Create component by name (for scripting/serialization)
    Component* createComponent(const std::string& name) const {
        auto it = nameToType.find(name);
        if (it != nameToType.end()) {
            auto metaIt = typeToMetadata.find(it->second);
            if (metaIt != typeToMetadata.end() && metaIt->second.factory) {
                return metaIt->second.factory();
            }
        }
        return nullptr;
    }
    
    // Delete component
    void deleteComponent(Component* comp, std::type_index typeIndex) const {
        auto it = typeToMetadata.find(typeIndex);
        if (it != typeToMetadata.end() && it->second.deleter) {
            it->second.deleter(comp);
        } else {
            delete comp;
        }
    }
    
    // Get all registered component names
    std::vector<std::string> getAllComponentNames() const {
        std::vector<std::string> names;
        for (const auto& [name, _] : nameToType) {
            names.push_back(name);
        }
        return names;
    }
    
    // Get component count
    size_t getComponentCount() const {
        return typeToMetadata.size();
    }
    
    // Print all registered components
    void printRegistry() const {
        std::cout << "\n=== Component Registry ===" << std::endl;
        std::cout << "Total components: " << typeToMetadata.size() << std::endl;
        
        for (const auto& [name, typeIndex] : nameToType) {
            auto metaIt = typeToMetadata.find(typeIndex);
            if (metaIt != typeToMetadata.end()) {
                const auto& meta = metaIt->second;
                auto idIt = typeToID.find(typeIndex);
                uint8_t id = idIt != typeToID.end() ? idIt->second : 255;
                
                std::cout << "  [" << (int)id << "] " << name 
                          << " (" << meta.size << " bytes)" << std::endl;
            }
        }
    }
    
    // Clear registry
    void clear() {
        typeToMetadata.clear();
        nameToType.clear();
        typeToID.clear();
        nextID = 0;
    }
};

// Global component registry (optional singleton)
inline ComponentRegistry& getGlobalComponentRegistry() {
    static ComponentRegistry registry;
    return registry;
}

// Helper macro for registering components
#define REGISTER_COMPONENT(Type, Name) \
    getGlobalComponentRegistry().registerComponent<Type>(Name)

// Example usage:
// REGISTER_COMPONENT(Transform, "Transform");
// REGISTER_COMPONENT(RigidBody, "RigidBody");
