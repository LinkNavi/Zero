#pragma once
#include "ScenePackage.h"
#include "Engine.h"
#include "transform.h"
#include "tags.h"
#include "PhysicsSystem.h"
#include <iostream>

namespace ScenePackaging {

// Binary scene data structure (trivially copyable)
struct SceneMetadata {
    uint32_t entityCount = 0;
    uint32_t componentTypeCount = 0;
    float timeScale = 1.0f;
    char sceneName[64] = {0};
    char sceneVersion[16] = {0};
};

// Helper to save ECS scene as a package
class ScenePackager {
public:
    static bool saveScene(ECS* ecs, const std::string& filepath, const std::string& sceneName = "Untitled") {
        ScenePackage::PackageWriter writer;
        
        // === 1. Prepare scene metadata ===
        SceneMetadata metadata;
        
        // Count active entities
        uint32_t entityCount = 0;
        for (size_t i = 0; i < 10000; ++i) {
            if (ecs->getComponent<Transform>(i) || ecs->getComponent<Tag>(i)) {
                entityCount++;
            }
        }
        
        metadata.entityCount = entityCount;
        metadata.componentTypeCount = 5; // Transform, Tag, Layer, RigidBody, Collider
        strncpy(metadata.sceneName, sceneName.c_str(), sizeof(metadata.sceneName) - 1);
        strncpy(metadata.sceneVersion, "1.0.0", sizeof(metadata.sceneVersion) - 1);
        
        writer.setSceneData(metadata);
        
        // === 2. Serialize entity data as resources ===
        for (size_t i = 0; i < 10000; ++i) {
            auto* transform = ecs->getComponent<Transform>(i);
            auto* tag = ecs->getComponent<Tag>(i);
            
            if (!transform && !tag) continue; // Skip inactive entities
            
            // Serialize entity to binary data
            std::vector<uint8_t> entityData = serializeEntity(ecs, i);
            if (entityData.empty()) continue;
            
            // Add as resource
            std::string entityName = "entity_" + std::to_string(i);
            if (tag) {
                entityName = tag->name + "_" + std::to_string(i);
            }
            
            writer.addResource(
                entityName,
                "entities/" + entityName + ".bin",
                ScenePackage::ResourceType::Prefab,
                std::move(entityData),
                ScenePackage::CompressionType::None
            );
        }
        
        // === 3. Write the package ===
        if (writer.write(filepath)) {
            std::cout << "✓ Saved scene package: " << filepath << std::endl;
            std::cout << "  Entities: " << entityCount << std::endl;
            std::cout << "  Resources: " << writer.getResourceCount() << std::endl;
            std::cout << "  Package size: " << writer.estimateSize() / 1024.0f << " KB" << std::endl;
            return true;
        }
        
        std::cerr << "✗ Failed to save scene package: " << filepath << std::endl;
        return false;
    }
    
    static bool loadScene(ECS* ecs, const std::string& filepath) {
        ScenePackage::PackageReader reader;
        
        if (!reader.open(filepath)) {
            std::cerr << "✗ Failed to open scene package: " << filepath << std::endl;
            return false;
        }
        
        // === 1. Read scene metadata ===
        SceneMetadata metadata;
        if (!reader.readSceneData(metadata)) {
            std::cerr << "✗ Failed to read scene metadata" << std::endl;
            return false;
        }
        
        std::cout << "✓ Loading scene: " << metadata.sceneName << std::endl;
        std::cout << "  Version: " << metadata.sceneVersion << std::endl;
        std::cout << "  Expected entities: " << metadata.entityCount << std::endl;
        
        // === 2. Load all entity resources ===
        const auto& entries = reader.getResourceEntries();
        uint32_t loadedCount = 0;
        
        for (size_t i = 0; i < entries.size(); ++i) {
            if (entries[i].type != ScenePackage::ResourceType::Prefab) continue;
            
            auto data = reader.readResource(static_cast<int>(i));
            if (data.empty()) {
                std::cerr << "  ✗ Failed to load entity: " << entries[i].name << std::endl;
                continue;
            }
            
            // Deserialize entity
            if (deserializeEntity(ecs, data)) {
                loadedCount++;
            }
        }
        
        reader.close();
        
        std::cout << "✓ Loaded " << loadedCount << " entities from scene package" << std::endl;
        return true;
    }

private:
    // Serialize a single entity to binary format
    static std::vector<uint8_t> serializeEntity(ECS* ecs, EntityID id) {
        std::vector<uint8_t> data;
        
        // Component presence flags (1 byte)
        uint8_t flags = 0;
        if (ecs->getComponent<Transform>(id)) flags |= 0x01;
        if (ecs->getComponent<Tag>(id)) flags |= 0x02;
        if (ecs->getComponent<Layer>(id)) flags |= 0x04;
        if (ecs->getComponent<RigidBody>(id)) flags |= 0x08;
        if (ecs->getComponent<Collider>(id)) flags |= 0x10;
        
        data.push_back(flags);
        
        // Serialize Transform
        if (auto* t = ecs->getComponent<Transform>(id)) {
            writeBytes(data, t->position);
            writeBytes(data, t->rotation);
            writeBytes(data, t->scale);
            writeBytes(data, t->parent);
        }
        
        // Serialize Tag
        if (auto* tag = ecs->getComponent<Tag>(id)) {
            writeString(data, tag->name);
        }
        
        // Serialize Layer
        if (auto* layer = ecs->getComponent<Layer>(id)) {
            writeBytes(data, layer->mask);
        }
        
        // Serialize RigidBody
        if (auto* rb = ecs->getComponent<RigidBody>(id)) {
            writeBytes(data, rb->velocity);
            writeBytes(data, rb->angularVelocity);
            writeBytes(data, rb->mass);
            writeBytes(data, rb->drag);
            writeBytes(data, rb->useGravity);
            writeBytes(data, rb->isKinematic);
        }
        
        // Serialize Collider
        if (auto* col = ecs->getComponent<Collider>(id)) {
            writeBytes(data, static_cast<uint8_t>(col->type));
            writeBytes(data, col->size);
            writeBytes(data, col->radius);
            writeBytes(data, col->isTrigger);
        }
        
        return data;
    }
    
    // Deserialize a single entity from binary format
    static bool deserializeEntity(ECS* ecs, const std::vector<uint8_t>& data) {
        if (data.empty()) return false;
        
        size_t offset = 0;
        
        // Read component flags
        uint8_t flags = data[offset++];
        
        EntityID entity = ecs->createEntity();
        
        // Deserialize Transform
        if (flags & 0x01) {
            Transform t;
            t.position = readVec3(data, offset);
            t.rotation = readQuat(data, offset);
            t.scale = readVec3(data, offset);
            t.parent = readUint32(data, offset);
            ecs->addComponent(entity, t);
        }
        
        // Deserialize Tag
        if (flags & 0x02) {
            Tag tag;
            tag.name = readString(data, offset);
            ecs->addComponent(entity, tag);
        }
        
        // Deserialize Layer
        if (flags & 0x04) {
            Layer layer;
            layer.mask = readUint32(data, offset);
            ecs->addComponent(entity, layer);
        }
        
        // Deserialize RigidBody
        if (flags & 0x08) {
            RigidBody rb;
            rb.velocity = readVec3(data, offset);
            rb.angularVelocity = readVec3(data, offset);
            rb.mass = readFloat(data, offset);
            rb.drag = readFloat(data, offset);
            rb.useGravity = readBool(data, offset);
            rb.isKinematic = readBool(data, offset);
            ecs->addComponent(entity, rb);
        }
        
        // Deserialize Collider
        if (flags & 0x10) {
            Collider col;
            col.type = static_cast<ColliderType>(readUint8(data, offset));
            col.size = readVec3(data, offset);
            col.radius = readFloat(data, offset);
            col.isTrigger = readBool(data, offset);
            ecs->addComponent(entity, col);
        }
        
        return true;
    }
    
    // Binary write helpers
    template<typename T>
    static void writeBytes(std::vector<uint8_t>& data, const T& value) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        data.insert(data.end(), bytes, bytes + sizeof(T));
    }
    
    static void writeString(std::vector<uint8_t>& data, const std::string& str) {
        uint16_t len = static_cast<uint16_t>(str.size());
        writeBytes(data, len);
        data.insert(data.end(), str.begin(), str.end());
    }
    
    // Binary read helpers
    static glm::vec3 readVec3(const std::vector<uint8_t>& data, size_t& offset) {
        glm::vec3 v;
        memcpy(&v, &data[offset], sizeof(glm::vec3));
        offset += sizeof(glm::vec3);
        return v;
    }
    
    static glm::quat readQuat(const std::vector<uint8_t>& data, size_t& offset) {
        glm::quat q;
        memcpy(&q, &data[offset], sizeof(glm::quat));
        offset += sizeof(glm::quat);
        return q;
    }
    
    static uint32_t readUint32(const std::vector<uint8_t>& data, size_t& offset) {
        uint32_t v;
        memcpy(&v, &data[offset], sizeof(uint32_t));
        offset += sizeof(uint32_t);
        return v;
    }
    
    static uint8_t readUint8(const std::vector<uint8_t>& data, size_t& offset) {
        return data[offset++];
    }
    
    static float readFloat(const std::vector<uint8_t>& data, size_t& offset) {
        float v;
        memcpy(&v, &data[offset], sizeof(float));
        offset += sizeof(float);
        return v;
    }
    
    static bool readBool(const std::vector<uint8_t>& data, size_t& offset) {
        return data[offset++] != 0;
    }
    
    static std::string readString(const std::vector<uint8_t>& data, size_t& offset) {
        uint16_t len;
        memcpy(&len, &data[offset], sizeof(uint16_t));
        offset += sizeof(uint16_t);
        
        std::string str(len, '\0');
        memcpy(&str[0], &data[offset], len);
        offset += len;
        return str;
    }
};

} // namespace ScenePackaging
