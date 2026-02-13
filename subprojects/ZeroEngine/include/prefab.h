#pragma once
#include "Engine.h"
#include "transform.h"
#include "tags.h"
#include "PhysicsSystem.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

// Prefab - reusable entity template
class Prefab {
public:
    std::string name;
    json data;
    
    Prefab() = default;
    Prefab(const std::string& n) : name(n) {}
    
    // Create entity from this prefab
    EntityID instantiate(ECS* ecs, glm::vec3 position = glm::vec3(0), glm::quat rotation = glm::quat(1,0,0,0)) {
        EntityID entity = ecs->createEntity();
        
        // Apply components from prefab data
        if (data.contains("components")) {
            const json& components = data["components"];
            
            // Transform
            if (components.contains("transform")) {
                Transform transform;
                transform.position = position; // Override with instantiation position
                transform.rotation = rotation;
                
                if (components["transform"].contains("scale")) {
                    auto& s = components["transform"]["scale"];
                    transform.scale = glm::vec3(s[0], s[1], s[2]);
                }
                
                ecs->addComponent(entity, transform);
            }
            
            // Tag
            if (components.contains("tag")) {
                Tag tag(components["tag"].get<std::string>());
                ecs->addComponent(entity, tag);
            }
            
            // Layer
            if (components.contains("layer")) {
                Layer layer;
                layer.mask = components["layer"].get<uint32_t>();
                ecs->addComponent(entity, layer);
            }
            
            // RigidBody
            if (components.contains("rigidbody")) {
                RigidBody rb;
                auto& rbData = components["rigidbody"];
                
                if (rbData.contains("mass")) rb.mass = rbData["mass"];
                if (rbData.contains("drag")) rb.drag = rbData["drag"];
                if (rbData.contains("useGravity")) rb.useGravity = rbData["useGravity"];
                if (rbData.contains("isKinematic")) rb.isKinematic = rbData["isKinematic"];
                
                ecs->addComponent(entity, rb);
            }
            
            // Collider
            if (components.contains("collider")) {
                Collider collider;
                auto& colData = components["collider"];
                
                if (colData.contains("type")) {
                    std::string typeStr = colData["type"];
                    if (typeStr == "box") collider.type = ColliderType::Box;
                    else if (typeStr == "sphere") collider.type = ColliderType::Sphere;
                    else if (typeStr == "capsule") collider.type = ColliderType::Capsule;
                }
                
                if (colData.contains("size")) {
                    auto& s = colData["size"];
                    collider.size = glm::vec3(s[0], s[1], s[2]);
                }
                
                if (colData.contains("radius")) {
                    collider.radius = colData["radius"];
                }
                
                if (colData.contains("isTrigger")) {
                    collider.isTrigger = colData["isTrigger"];
                }
                
                ecs->addComponent(entity, collider);
            }
        }
        
        return entity;
    }
    
    // Load prefab from JSON file
    static Prefab load(const std::string& path) {
        Prefab prefab;
        
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open prefab file: " << path << std::endl;
            return prefab;
        }
        
        try {
            file >> prefab.data;
            
            if (prefab.data.contains("name")) {
                prefab.name = prefab.data["name"];
            }
            
            std::cout << "Loaded prefab: " << prefab.name << " from " << path << std::endl;
        } catch (const json::exception& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
        }
        
        return prefab;
    }
    
    // Save prefab to JSON file
    bool save(const std::string& path) {
        std::ofstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << path << std::endl;
            return false;
        }
        
        data["name"] = name;
        
        try {
            file << data.dump(2); // Pretty print with 2-space indent
            std::cout << "Saved prefab: " << name << " to " << path << std::endl;
            return true;
        } catch (const json::exception& e) {
            std::cerr << "JSON write error: " << e.what() << std::endl;
            return false;
        }
    }
};

// Entity serialization helpers
namespace EntitySerializer {
    // Serialize single entity to JSON
    inline json serialize(ECS* ecs, EntityID entity) {
        json j;
        j["id"] = entity;
        j["components"] = json::object();
        
        // Transform
        if (auto* transform = ecs->getComponent<Transform>(entity)) {
            json t;
            t["position"] = {transform->position.x, transform->position.y, transform->position.z};
            
            glm::vec3 euler = transform->getEulerAngles();
            t["rotation"] = {euler.x, euler.y, euler.z};
            t["scale"] = {transform->scale.x, transform->scale.y, transform->scale.z};
            
            if (transform->parent != 0) {
                t["parent"] = transform->parent;
            }
            
            j["components"]["transform"] = t;
        }
        
        // Tag
        if (auto* tag = ecs->getComponent<Tag>(entity)) {
            j["components"]["tag"] = tag->name;
        }
        
        // Layer
        if (auto* layer = ecs->getComponent<Layer>(entity)) {
            j["components"]["layer"] = layer->mask;
        }
        
        // RigidBody
        if (auto* rb = ecs->getComponent<RigidBody>(entity)) {
            json rbJson;
            rbJson["mass"] = rb->mass;
            rbJson["drag"] = rb->drag;
            rbJson["useGravity"] = rb->useGravity;
            rbJson["isKinematic"] = rb->isKinematic;
            rbJson["velocity"] = {rb->velocity.x, rb->velocity.y, rb->velocity.z};
            j["components"]["rigidbody"] = rbJson;
        }
        
        // Collider
        if (auto* col = ecs->getComponent<Collider>(entity)) {
            json colJson;
            
            switch (col->type) {
                case ColliderType::Box: colJson["type"] = "box"; break;
                case ColliderType::Sphere: colJson["type"] = "sphere"; break;
                case ColliderType::Capsule: colJson["type"] = "capsule"; break;
            }
            
            colJson["size"] = {col->size.x, col->size.y, col->size.z};
            colJson["radius"] = col->radius;
            colJson["isTrigger"] = col->isTrigger;
            
            j["components"]["collider"] = colJson;
        }
        
        return j;
    }
    
    // Deserialize entity from JSON
    inline EntityID deserialize(ECS* ecs, const json& j) {
        EntityID entity = ecs->createEntity();
        
        if (!j.contains("components")) return entity;
        
        const json& components = j["components"];
        
        // Transform
        if (components.contains("transform")) {
            Transform transform;
            auto& t = components["transform"];
            
            if (t.contains("position")) {
                auto& p = t["position"];
                transform.position = glm::vec3(p[0], p[1], p[2]);
            }
            
            if (t.contains("rotation")) {
                auto& r = t["rotation"];
                transform.setEulerAngles(glm::vec3(r[0], r[1], r[2]));
            }
            
            if (t.contains("scale")) {
                auto& s = t["scale"];
                transform.scale = glm::vec3(s[0], s[1], s[2]);
            }
            
            if (t.contains("parent")) {
                transform.parent = t["parent"];
            }
            
            ecs->addComponent(entity, transform);
        }
        
        // Tag
        if (components.contains("tag")) {
            Tag tag(components["tag"].get<std::string>());
            ecs->addComponent(entity, tag);
        }
        
        // Layer
        if (components.contains("layer")) {
            Layer layer;
            layer.mask = components["layer"].get<uint32_t>();
            ecs->addComponent(entity, layer);
        }
        
        // RigidBody
        if (components.contains("rigidbody")) {
            RigidBody rb;
            auto& rbData = components["rigidbody"];
            
            if (rbData.contains("mass")) rb.mass = rbData["mass"];
            if (rbData.contains("drag")) rb.drag = rbData["drag"];
            if (rbData.contains("useGravity")) rb.useGravity = rbData["useGravity"];
            if (rbData.contains("isKinematic")) rb.isKinematic = rbData["isKinematic"];
            
            if (rbData.contains("velocity")) {
                auto& v = rbData["velocity"];
                rb.velocity = glm::vec3(v[0], v[1], v[2]);
            }
            
            ecs->addComponent(entity, rb);
        }
        
        // Collider
        if (components.contains("collider")) {
            Collider collider;
            auto& colData = components["collider"];
            
            if (colData.contains("type")) {
                std::string typeStr = colData["type"];
                if (typeStr == "box") collider.type = ColliderType::Box;
                else if (typeStr == "sphere") collider.type = ColliderType::Sphere;
                else if (typeStr == "capsule") collider.type = ColliderType::Capsule;
            }
            
            if (colData.contains("size")) {
                auto& s = colData["size"];
                collider.size = glm::vec3(s[0], s[1], s[2]);
            }
            
            if (colData.contains("radius")) {
                collider.radius = colData["radius"];
            }
            
            if (colData.contains("isTrigger")) {
                collider.isTrigger = colData["isTrigger"];
            }
            
            ecs->addComponent(entity, collider);
        }
        
        return entity;
    }
    
    // Save scene (all entities) to file
    inline bool saveScene(ECS* ecs, const std::string& path) {
        json scene;
        scene["entities"] = json::array();
        
        // Serialize all active entities
        for (size_t i = 0; i < 10000; ++i) {
            // Check if entity has any components (simple activity check)
            if (ecs->getComponent<Transform>(i) || ecs->getComponent<Tag>(i)) {
                scene["entities"].push_back(serialize(ecs, i));
            }
        }
        
        std::ofstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to save scene to: " << path << std::endl;
            return false;
        }
        
        try {
            file << scene.dump(2);
            std::cout << "Saved scene with " << scene["entities"].size() << " entities to " << path << std::endl;
            return true;
        } catch (const json::exception& e) {
            std::cerr << "Scene save error: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Load scene from file
    inline bool loadScene(ECS* ecs, const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to load scene from: " << path << std::endl;
            return false;
        }
        
        try {
            json scene;
            file >> scene;
            
            if (!scene.contains("entities")) {
                std::cerr << "Invalid scene file (no entities array)" << std::endl;
                return false;
            }
            
            for (const auto& entityData : scene["entities"]) {
                deserialize(ecs, entityData);
            }
            
            std::cout << "Loaded scene with " << scene["entities"].size() << " entities from " << path << std::endl;
            return true;
        } catch (const json::exception& e) {
            std::cerr << "Scene load error: " << e.what() << std::endl;
            return false;
        }
    }
}

// Prefab manager for caching prefabs
class PrefabManager {
    std::unordered_map<std::string, Prefab> prefabs;
    std::string baseDir = "prefabs/";
    
public:
    void setBaseDirectory(const std::string& dir) {
        baseDir = dir;
    }
    
    // Load and cache prefab
    Prefab* load(const std::string& name) {
        // Check cache
        auto it = prefabs.find(name);
        if (it != prefabs.end()) {
            return &it->second;
        }
        
        // Load from file
        std::string path = baseDir + name + ".json";
        Prefab prefab = Prefab::load(path);
        
        if (prefab.data.empty()) {
            return nullptr;
        }
        
        prefabs[name] = std::move(prefab);
        return &prefabs[name];
    }
    
    // Get cached prefab
    Prefab* get(const std::string& name) {
        auto it = prefabs.find(name);
        return it != prefabs.end() ? &it->second : nullptr;
    }
    
    // Create entity from cached prefab
    EntityID instantiate(ECS* ecs, const std::string& name, glm::vec3 position = glm::vec3(0)) {
        Prefab* prefab = get(name);
        if (!prefab) {
            prefab = load(name);
        }
        
        if (prefab) {
            return prefab->instantiate(ecs, position);
        }
        
        return 0;
    }
    
    // Register prefab programmatically
    void registerPrefab(const std::string& name, const Prefab& prefab) {
        prefabs[name] = prefab;
    }
    
    void clear() {
        prefabs.clear();
    }
};
