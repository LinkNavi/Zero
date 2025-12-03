#ifndef ECS_H
#define ECS_H

#include "Logger.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <queue>
#include <set>
#include <bitset>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <cassert>

using Entity = std::uint32_t;
const Entity MAX_ENTITIES = 5000;
const Entity INVALID_ENTITY = UINT32_MAX;

// Component type ID
using ComponentType = std::uint8_t;
const ComponentType MAX_COMPONENTS = 32;

// Signature bitset for component tracking
using Signature = std::bitset<MAX_COMPONENTS>;

// Component interface
struct IComponent {
    virtual ~IComponent() = default;
};

// Component array base
class IComponentArray {
public:
    virtual ~IComponentArray() = default;
    virtual void EntityDestroyed(Entity entity) = 0;
    virtual size_t Size() const = 0;
};

template <typename T>
class ComponentArray : public IComponentArray {
public:
    void InsertData(Entity entity, T component) {
        if (mEntityToIndexMap.find(entity) != mEntityToIndexMap.end()) {
            LOG_WARN("Component already exists for entity " + std::to_string(entity));
            return;
        }

        if (mSize >= MAX_ENTITIES) {
            LOG_ERROR("ComponentArray is full, cannot add more components");
            return;
        }

        size_t newIndex = mSize;
        mEntityToIndexMap[entity] = newIndex;
        mIndexToEntityMap[newIndex] = entity;
        mComponentArray[newIndex] = component;
        ++mSize;
    }

    void RemoveData(Entity entity) {
        if (mEntityToIndexMap.find(entity) == mEntityToIndexMap.end()) {
            LOG_WARN("Attempting to remove non-existent component from entity " + std::to_string(entity));
            return;
        }

        size_t indexOfRemovedEntity = mEntityToIndexMap[entity];
        size_t indexOfLastElement = mSize - 1;
        mComponentArray[indexOfRemovedEntity] = mComponentArray[indexOfLastElement];

        Entity entityOfLastElement = mIndexToEntityMap[indexOfLastElement];
        mEntityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
        mIndexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;

        mEntityToIndexMap.erase(entity);
        mIndexToEntityMap.erase(indexOfLastElement);
        --mSize;
    }

    T& GetData(Entity entity) {
        if (mEntityToIndexMap.find(entity) == mEntityToIndexMap.end()) {
            LOG_ERROR("Component does not exist for entity " + std::to_string(entity));
            static T dummy{};
            return dummy;
        }
        return mComponentArray[mEntityToIndexMap[entity]];
    }

    bool HasData(Entity entity) const {
        return mEntityToIndexMap.find(entity) != mEntityToIndexMap.end();
    }

    void EntityDestroyed(Entity entity) override {
        if (mEntityToIndexMap.find(entity) != mEntityToIndexMap.end()) {
            RemoveData(entity);
        }
    }

    size_t Size() const override { return mSize; }

private:
    std::array<T, MAX_ENTITIES> mComponentArray;
    std::unordered_map<Entity, size_t> mEntityToIndexMap;
    std::unordered_map<size_t, Entity> mIndexToEntityMap;
    size_t mSize = 0;
};

// Component Manager
class ComponentManager {
public:
    template <typename T>
    void RegisterComponent() {
        const char* typeName = typeid(T).name();
        
        if (mComponentTypes.find(typeName) != mComponentTypes.end()) {
            LOG_WARN("Component type already registered: " + std::string(typeName));
            return;
        }

        if (mNextComponentType >= MAX_COMPONENTS) {
            LOG_ERROR("Maximum number of component types reached!");
            return;
        }

        mComponentTypes.insert({typeName, mNextComponentType});
        mComponentArrays.insert({typeName, std::make_shared<ComponentArray<T>>()});
        
        LOG_DEBUG("Registered component: " + std::string(typeName));
        ++mNextComponentType;
    }

    template <typename T>
    ComponentType GetComponentType() {
        const char* typeName = typeid(T).name();
        
        if (mComponentTypes.find(typeName) == mComponentTypes.end()) {
            LOG_ERROR("Component type not registered: " + std::string(typeName));
            return 0;
        }
        
        return mComponentTypes[typeName];
    }

    template <typename T>
    void AddComponent(Entity entity, T component) {
        GetComponentArray<T>()->InsertData(entity, component);
    }

    template <typename T>
    void RemoveComponent(Entity entity) {
        GetComponentArray<T>()->RemoveData(entity);
    }

    template <typename T>
    T& GetComponent(Entity entity) {
        return GetComponentArray<T>()->GetData(entity);
    }

    template <typename T>
    bool HasComponent(Entity entity) {
        return GetComponentArray<T>()->HasData(entity);
    }

    void EntityDestroyed(Entity entity) {
        for (auto const& pair : mComponentArrays) {
            auto const& component = pair.second;
            component->EntityDestroyed(entity);
        }
    }

private:
    std::unordered_map<const char*, ComponentType> mComponentTypes{};
    std::unordered_map<const char*, std::shared_ptr<IComponentArray>> mComponentArrays{};
    ComponentType mNextComponentType{};

    template <typename T>
    std::shared_ptr<ComponentArray<T>> GetComponentArray() {
        const char* typeName = typeid(T).name();
        
        if (mComponentArrays.find(typeName) == mComponentArrays.end()) {
            LOG_ERROR("Component type not registered before use: " + std::string(typeName));
            return nullptr;
        }
        
        return std::static_pointer_cast<ComponentArray<T>>(mComponentArrays[typeName]);
    }
};

// Entity Manager
class EntityManager {
public:
    EntityManager() {
        for (Entity entity = 0; entity < MAX_ENTITIES; ++entity) {
            mAvailableEntities.push(entity);
        }
    }

    Entity CreateEntity() {
        if (mLivingEntityCount >= MAX_ENTITIES) {
            LOG_ERROR("Maximum entity limit reached!");
            return INVALID_ENTITY;
        }

        Entity id = mAvailableEntities.front();
        mAvailableEntities.pop();
        ++mLivingEntityCount;
        
        LOG_TRACE("Created entity: " + std::to_string(id));
        return id;
    }

    void DestroyEntity(Entity entity) {
        if (entity >= MAX_ENTITIES) {
            LOG_ERROR("Entity ID out of range: " + std::to_string(entity));
            return;
        }

        mSignatures[entity].reset();
        mAvailableEntities.push(entity);
        --mLivingEntityCount;
        
        LOG_TRACE("Destroyed entity: " + std::to_string(entity));
    }

    void SetSignature(Entity entity, Signature signature) {
        if (entity >= MAX_ENTITIES) {
            LOG_ERROR("Entity ID out of range: " + std::to_string(entity));
            return;
        }
        mSignatures[entity] = signature;
    }

    Signature GetSignature(Entity entity) {
        if (entity >= MAX_ENTITIES) {
            LOG_ERROR("Entity ID out of range: " + std::to_string(entity));
            return Signature();
        }
        return mSignatures[entity];
    }

    std::uint32_t GetLivingEntityCount() const { return mLivingEntityCount; }

private:
    std::queue<Entity> mAvailableEntities{};
    std::array<Signature, MAX_ENTITIES> mSignatures{};
    std::uint32_t mLivingEntityCount{};
};

// System base class
class System {
public:
    std::set<Entity> mEntities;
    virtual ~System() = default;
    virtual void Update(float deltaTime) = 0;
    virtual void OnInit() {}
    virtual void OnShutdown() {}
};

// System Manager
class SystemManager {
public:
    template <typename T>
    std::shared_ptr<T> RegisterSystem() {
        const char* typeName = typeid(T).name();

        if (mSystems.find(typeName) != mSystems.end()) {
            LOG_WARN("System already registered: " + std::string(typeName));
            return std::static_pointer_cast<T>(mSystems[typeName]);
        }

        auto system = std::make_shared<T>();
        mSystems.insert({typeName, system});
        
        LOG_DEBUG("Registered system: " + std::string(typeName));
        return system;
    }

    template <typename T>
    void SetSignature(Signature signature) {
        const char* typeName = typeid(T).name();

        if (mSystems.find(typeName) == mSystems.end()) {
            LOG_ERROR("System not registered before setting signature: " + std::string(typeName));
            return;
        }

        mSignatures.insert({typeName, signature});
    }

    void EntityDestroyed(Entity entity) {
        for (auto const& pair : mSystems) {
            auto const& system = pair.second;
            system->mEntities.erase(entity);
        }
    }

    void EntitySignatureChanged(Entity entity, Signature entitySignature) {
        for (auto const& pair : mSystems) {
            auto const& type = pair.first;
            auto const& system = pair.second;
            auto const& systemSignature = mSignatures[type];

            if ((entitySignature & systemSignature) == systemSignature) {
                system->mEntities.insert(entity);
            } else {
                system->mEntities.erase(entity);
            }
        }
    }

private:
    std::unordered_map<const char*, Signature> mSignatures{};
    std::unordered_map<const char*, std::shared_ptr<System>> mSystems{};
};

// Main ECS Coordinator
class ECS {
public:
    void Init() {
        mComponentManager = std::make_unique<ComponentManager>();
        mEntityManager = std::make_unique<EntityManager>();
        mSystemManager = std::make_unique<SystemManager>();
        
        LOG_INFO("ECS initialized successfully");
    }

    // Entity methods
    Entity CreateEntity() {
        return mEntityManager->CreateEntity();
    }

    void DestroyEntity(Entity entity) {
        mEntityManager->DestroyEntity(entity);
        mComponentManager->EntityDestroyed(entity);
        mSystemManager->EntityDestroyed(entity);
    }

    // Component methods
    template <typename T>
    void RegisterComponent() {
        mComponentManager->RegisterComponent<T>();
    }

    template <typename T>
    void AddComponent(Entity entity, T component) {
        mComponentManager->AddComponent<T>(entity, component);

        auto signature = mEntityManager->GetSignature(entity);
        signature.set(mComponentManager->GetComponentType<T>(), true);
        mEntityManager->SetSignature(entity, signature);
        
        mSystemManager->EntitySignatureChanged(entity, signature);
    }

    template <typename T>
    void RemoveComponent(Entity entity) {
        mComponentManager->RemoveComponent<T>(entity);

        auto signature = mEntityManager->GetSignature(entity);
        signature.set(mComponentManager->GetComponentType<T>(), false);
        mEntityManager->SetSignature(entity, signature);
        
        mSystemManager->EntitySignatureChanged(entity, signature);
    }

    template <typename T>
    T& GetComponent(Entity entity) {
        return mComponentManager->GetComponent<T>(entity);
    }

    template <typename T>
    bool HasComponent(Entity entity) {
        return mComponentManager->HasComponent<T>(entity);
    }

    template <typename T>
    ComponentType GetComponentType() {
        return mComponentManager->GetComponentType<T>();
    }

    // System methods
    template <typename T>
    std::shared_ptr<T> RegisterSystem() {
        return mSystemManager->RegisterSystem<T>();
    }

    template <typename T, typename... ComponentTypes>
    void SetSystemSignature() {
        Signature signature;
        (signature.set(GetComponentType<ComponentTypes>()), ...);
        mSystemManager->SetSignature<T>(signature);
    }

    uint32_t GetEntityCount() const {
        return mEntityManager->GetLivingEntityCount();
    }

private:
    std::unique_ptr<ComponentManager> mComponentManager;
    std::unique_ptr<EntityManager> mEntityManager;
    std::unique_ptr<SystemManager> mSystemManager;
};

#endif // ECS_H
