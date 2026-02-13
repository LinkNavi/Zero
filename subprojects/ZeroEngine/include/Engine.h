#pragma once
#include <vector>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <bitset>
#include <queue>
#include <cstdint>
#include <algorithm>

constexpr size_t MAX_COMPONENTS = 32;
using ComponentMask = std::bitset<MAX_COMPONENTS>;
using EntityID = uint32_t;

class Component {
public:
    virtual ~Component() = default;
};

class Entity {
public:
    EntityID id;
    ComponentMask mask;
    bool active = true;
};

class ComponentArray {
public:
    virtual ~ComponentArray() = default;
    virtual void entityDestroyed(EntityID entity) = 0;
};

template<typename T>
class TypedComponentArray : public ComponentArray {
    std::unordered_map<EntityID, size_t> entityToIndex;
    std::unordered_map<size_t, EntityID> indexToEntity;
    std::vector<T> components;
    size_t size = 0;

public:
    void insert(EntityID entity, T component) {
        entityToIndex[entity] = size;
        indexToEntity[size] = entity;
        components.push_back(component);
        size++;
    }

    void remove(EntityID entity) {
        size_t removedIndex = entityToIndex[entity];
        size_t lastIndex = size - 1;
        components[removedIndex] = components[lastIndex];
        
        EntityID lastEntity = indexToEntity[lastIndex];
        entityToIndex[lastEntity] = removedIndex;
        indexToEntity[removedIndex] = lastEntity;

        entityToIndex.erase(entity);
        indexToEntity.erase(lastIndex);
        components.pop_back();
        size--;
    }

 T* get(EntityID entity) {
    auto it = entityToIndex.find(entity);
    if (it == entityToIndex.end()) return nullptr;
    return &components[it->second];
}

    void entityDestroyed(EntityID entity) override {
        if (entityToIndex.find(entity) != entityToIndex.end())
            remove(entity);
    }
};

class System {
public:
    ComponentMask signature;
    std::vector<EntityID> entities;
    virtual ~System() = default;
    virtual void update(float dt) = 0;
};

class ECS {
    std::vector<Entity> entities;
    std::queue<EntityID> availableIDs;
    std::unordered_map<std::type_index, std::shared_ptr<ComponentArray>> componentArrays;
    std::unordered_map<std::type_index, uint8_t> componentTypes;
    std::vector<std::shared_ptr<System>> systems;
    uint8_t nextComponentType = 0;
    EntityID nextEntityID = 0;

public:
    EntityID createEntity() {
        EntityID id;
        if (!availableIDs.empty()) {
            id = availableIDs.front();
            availableIDs.pop();
        } else {
            id = nextEntityID++;
            entities.emplace_back();
        }
        entities[id].id = id;
        entities[id].active = true;
        entities[id].mask.reset();
        return id;
    }
void clear() {
    for (EntityID e = 0; e < nextEntityID; e++) {
        destroyEntity(e);
    }
    nextEntityID = 0;
}
    void destroyEntity(EntityID entity) {
        entities[entity].active = false;
        entities[entity].mask.reset();
        availableIDs.push(entity);
        
        for (auto& [type, array] : componentArrays)
            array->entityDestroyed(entity);
        
        for (auto& system : systems) {
            auto& ents = system->entities;
            ents.erase(std::remove(ents.begin(), ents.end(), entity), ents.end());
        }
    }

    template<typename T>
    void registerComponent() {
        std::type_index typeIndex(typeid(T));
        componentTypes[typeIndex] = nextComponentType++;
        componentArrays[typeIndex] = std::make_shared<TypedComponentArray<T>>();
    }

    template<typename T>
    void addComponent(EntityID entity, T component) {
        getComponentArray<T>()->insert(entity, component);
        entities[entity].mask.set(componentTypes[std::type_index(typeid(T))]);
        updateEntitySystems(entity);
    }

    template<typename T>
    void removeComponent(EntityID entity) {
        getComponentArray<T>()->remove(entity);
        entities[entity].mask.reset(componentTypes[std::type_index(typeid(T))]);
        updateEntitySystems(entity);
    }

 template<typename T>
T* getComponent(EntityID entity) {
    std::type_index typeIdx(typeid(T));
    auto it = componentArrays.find(typeIdx);
    if (it == componentArrays.end()) return nullptr;
    
    auto* array = static_cast<TypedComponentArray<T>*>(it->second.get());
    if (!array) return nullptr;
    return array->get(entity);
}

    template<typename T>
    std::shared_ptr<T> registerSystem() {
        auto system = std::make_shared<T>();
        systems.push_back(system);
        return system;
    }

    template<typename T>
    void setSystemSignature(ComponentMask signature) {
        for (auto& sys : systems) {
            if (auto casted = std::dynamic_pointer_cast<T>(sys)) {
                casted->signature = signature;
                break;
            }
        }
    }

    void updateSystems(float dt) {
        for (auto& system : systems)
            system->update(dt);
    }

private:
    template<typename T>
    std::shared_ptr<TypedComponentArray<T>> getComponentArray() {
        std::type_index typeIndex(typeid(T));
        return std::static_pointer_cast<TypedComponentArray<T>>(componentArrays[typeIndex]);
    }

    void updateEntitySystems(EntityID entity) {
        for (auto& system : systems) {
            bool matches = (entities[entity].mask & system->signature) == system->signature;
            auto& ents = system->entities;
            auto it = std::find(ents.begin(), ents.end(), entity);
            
            if (matches && it == ents.end())
                ents.push_back(entity);
            else if (!matches && it != ents.end())
                ents.erase(it);
        }
    }
};
