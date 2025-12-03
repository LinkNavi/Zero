#pragma once
#include "ECS.h"
#include "Systems.h"
#include "Logger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>

using json = nlohmann::json;

// Serialization helper for Vector3
inline void to_json(json& j, const Vector3& v) {
    j = json{{"x", v.x}, {"y", v.y}, {"z", v.z}};
}

inline void from_json(const json& j, Vector3& v) {
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
    j.at("z").get_to(v.z);
}

// Scene Serializer
class SceneSerializer {
public:
    static bool SaveScene(const std::string& filepath, ECS& ecs) {
        try {
            json sceneData;
            sceneData["version"] = "1.0";
            sceneData["entities"] = json::array();

            // Iterate through all entities and serialize components
            for (uint32_t entityId = 0; entityId < MAX_ENTITIES; ++entityId) {
                Entity entity = static_cast<Entity>(entityId);
                
                json entityData;
                entityData["id"] = entity;
                bool hasComponents = false;

                // Serialize Transform
                if (ecs.HasComponent<ZeroTransform>(entity)) {
                    auto& transform = ecs.GetComponent<ZeroTransform>(entity);
                    entityData["transform"] = {
                        {"position", transform.position},
                        {"rotation", transform.rotation},
                        {"scale", transform.scale}
                    };
                    hasComponents = true;
                }

                // Serialize Velocity
                if (ecs.HasComponent<Velocity>(entity)) {
                    auto& vel = ecs.GetComponent<Velocity>(entity);
                    entityData["velocity"] = vel.v;
                    hasComponents = true;
                }

                // Serialize InputReceiver
                if (ecs.HasComponent<InputReceiver>(entity)) {
                    auto& input = ecs.GetComponent<InputReceiver>(entity);
                    entityData["inputReceiver"] = {
                        {"speed", input.speed}
                    };
                    hasComponents = true;
                }

                // Serialize CameraFollow
                if (ecs.HasComponent<CameraFollow>(entity)) {
                    auto& camFollow = ecs.GetComponent<CameraFollow>(entity);
                    entityData["cameraFollow"] = {
                        {"target", camFollow.target},
                        {"offset", camFollow.offset}
                    };
                    hasComponents = true;
                }

                // Serialize MeshRenderer (just mark it exists, mesh loading handled separately)
                if (ecs.HasComponent<MeshRendererComponent>(entity)) {
                    auto& mr = ecs.GetComponent<MeshRendererComponent>(entity);
                    entityData["meshRenderer"] = {
                        {"isValid", mr.isValid},
                        {"meshType", "cube"} // For now, we only support cubes
                    };
                    hasComponents = true;
                }

                if (hasComponents) {
                    sceneData["entities"].push_back(entityData);
                }
            }

            // Write to file
            std::ofstream file(filepath);
            if (!file.is_open()) {
                LOG_ERROR("Failed to open file for writing: " + filepath);
                return false;
            }

            file << sceneData.dump(4);
            file.close();

            LOG_INFO("Scene saved successfully: " + filepath);
            return true;

        } catch (const std::exception& e) {
            LOG_ERROR("Failed to save scene: " + std::string(e.what()));
            return false;
        }
    }

    static bool LoadScene(const std::string& filepath, ECS& ecs, ResourceManager& resources) {
        try {
            std::ifstream file(filepath);
            if (!file.is_open()) {
                LOG_ERROR("Failed to open file for reading: " + filepath);
                return false;
            }

            json sceneData;
            file >> sceneData;
            file.close();

            // Clear existing entities (optional - you might want to add to existing scene)
            // For now, we'll just load on top

            // Entity remapping (old ID -> new ID)
            std::unordered_map<Entity, Entity> entityMap;

            for (const auto& entityData : sceneData["entities"]) {
                Entity oldId = entityData["id"];
                Entity newEntity = ecs.CreateEntity();
                entityMap[oldId] = newEntity;

                // Load Transform
                if (entityData.contains("transform")) {
                    ZeroTransform t;
                    t.position = entityData["transform"]["position"];
                    t.rotation = entityData["transform"]["rotation"];
                    t.scale = entityData["transform"]["scale"];
                    ecs.AddComponent(newEntity, t);
                }

                // Load Velocity
                if (entityData.contains("velocity")) {
                    Velocity vel;
                    vel.v = entityData["velocity"];
                    ecs.AddComponent(newEntity, vel);
                }

                // Load InputReceiver
                if (entityData.contains("inputReceiver")) {
                    InputReceiver input;
                    input.speed = entityData["inputReceiver"]["speed"];
                    ecs.AddComponent(newEntity, input);
                }

                // Load MeshRenderer
                if (entityData.contains("meshRenderer")) {
                    MeshRendererComponent mr;
                    mr.isValid = entityData["meshRenderer"]["isValid"];
                    
                    // Load the mesh (for now, just use cube)
                    mr.mesh = resources.GetCube(1.0f);
                    mr.shader = resources.GetDefaultShader();
                    
                    ecs.AddComponent(newEntity, mr);
                }
            }

            // Second pass for references (like CameraFollow target)
            size_t entityIndex = 0;
            for (const auto& entityData : sceneData["entities"]) {
                Entity oldId = entityData["id"];
                Entity newEntity = entityMap[oldId];

                if (entityData.contains("cameraFollow")) {
                    CameraFollow cf;
                    Entity oldTarget = entityData["cameraFollow"]["target"];
                    cf.target = entityMap.count(oldTarget) ? entityMap[oldTarget] : oldTarget;
                    cf.offset = entityData["cameraFollow"]["offset"];
                    ecs.AddComponent(newEntity, cf);
                }
            }

            LOG_INFO("Scene loaded successfully: " + filepath);
            return true;

        } catch (const std::exception& e) {
            LOG_ERROR("Failed to load scene: " + std::string(e.what()));
            return false;
        }
    }
};
