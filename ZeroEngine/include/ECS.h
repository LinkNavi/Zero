#ifndef ECS_H
#define ECS_H

#include "ResourceManager.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <queue>
#include <set>
#include <typeindex>
#include <unordered_map>
#include <vector>
// Entity is just an ID
using Entity = std::uint32_t;
const Entity MAX_ENTITIES = 5000;

// Component interface
struct IComponent {
  virtual ~IComponent() = default;
};

// Component array to store components of a specific type
class IComponentArray {
public:
  virtual ~IComponentArray() = default;
  virtual void EntityDestroyed(Entity entity) = 0;
};

template <typename T> class ComponentArray : public IComponentArray {
public:
  void InsertData(Entity entity, T component) {
    size_t newIndex = mSize;
    mEntityToIndexMap[entity] = newIndex;
    mIndexToEntityMap[newIndex] = entity;
    mComponentArray[newIndex] = component;
    ++mSize;
  }

  void RemoveData(Entity entity) {
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

  // In ECS.h

  // Inside ECS class

  T &GetData(Entity entity) {
    return mComponentArray[mEntityToIndexMap[entity]];
  }

  bool HasData(Entity entity) {
    return mEntityToIndexMap.find(entity) != mEntityToIndexMap.end();
  }

  void EntityDestroyed(Entity entity) override {
    if (mEntityToIndexMap.find(entity) != mEntityToIndexMap.end()) {
      RemoveData(entity);
    }
  }

private:
  std::array<T, MAX_ENTITIES> mComponentArray;
  std::unordered_map<Entity, size_t> mEntityToIndexMap;
  std::unordered_map<size_t, Entity> mIndexToEntityMap;
  size_t mSize = 0;
};

// Component Manager
class ComponentManager {
public:
  template <typename T> void RegisterComponent() {
    const char *typeName = typeid(T).name();
    mComponentTypes.insert({typeName, mNextComponentType});
    mComponentArrays.insert({typeName, std::make_shared<ComponentArray<T>>()});
    ++mNextComponentType;
  }

  template <typename T> void AddComponent(Entity entity, T component) {
    GetComponentArray<T>()->InsertData(entity, component);
  }

  template <typename T> void RemoveComponent(Entity entity) {
    GetComponentArray<T>()->RemoveData(entity);
  }

  template <typename T> T &GetComponent(Entity entity) {
    return GetComponentArray<T>()->GetData(entity);
  }

  template <typename T> bool HasComponent(Entity entity) {
    return GetComponentArray<T>()->HasData(entity);
  }

  void EntityDestroyed(Entity entity) {
    for (auto const &pair : mComponentArrays) {
      auto const &component = pair.second;
      component->EntityDestroyed(entity);
    }
  }

private:
  std::unordered_map<const char *, std::uint8_t> mComponentTypes;
  std::unordered_map<const char *, std::shared_ptr<IComponentArray>>
      mComponentArrays;
  std::uint8_t mNextComponentType = 0;

  template <typename T> std::shared_ptr<ComponentArray<T>> GetComponentArray() {
    const char *typeName = typeid(T).name();
    return std::static_pointer_cast<ComponentArray<T>>(
        mComponentArrays[typeName]);
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
    Entity id = mAvailableEntities.front();
    mAvailableEntities.pop();
    ++mLivingEntityCount;
    return id;
  }

  void DestroyEntity(Entity entity) {
    mAvailableEntities.push(entity);
    --mLivingEntityCount;
  }

  std::uint32_t GetLivingEntityCount() const { return mLivingEntityCount; }

private:
  std::queue<Entity> mAvailableEntities;
  std::uint32_t mLivingEntityCount = 0;
};

// System base class
class System {
public:
  std::set<Entity> mEntities;
  virtual ~System() = default;
  virtual void Update(float deltaTime) = 0;
};

// System Manager
class SystemManager {
public:
  template <typename T> std::shared_ptr<T> RegisterSystem() {
    auto type = std::type_index(typeid(T));
    auto system = std::make_shared<T>();
    mSystems[type] = system;
    return system;
  }

  template <typename T> std::shared_ptr<T> GetSystem() {
    auto type = std::type_index(typeid(T));
    auto it = mSystems.find(type);
    if (it != mSystems.end())
      return std::static_pointer_cast<T>(it->second);
    return nullptr;
  }

  void EntityDestroyed(Entity entity) {
    for (auto &[_, system] : mSystems) {
      system->mEntities.erase(entity);
    }
  }

  template <typename T>
  void EntitySignatureChanged(Entity entity, bool hasAllComponents) {
    auto type = std::type_index(typeid(T));
    auto &system = mSystems[type];
    if (hasAllComponents)
      system->mEntities.insert(entity);
    else
      system->mEntities.erase(entity);
  }

private:
  std::unordered_map<std::type_index, std::shared_ptr<System>> mSystems;
};

// Main ECS Coordinator
class ECS {
public:
  void Init() {
    mComponentManager = std::make_unique<ComponentManager>();
    mEntityManager = std::make_unique<EntityManager>();
    mSystemManager = std::make_unique<SystemManager>();
  }

  // Entity methods
  Entity CreateEntity() { return mEntityManager->CreateEntity(); }

  void DestroyEntity(Entity entity) {
    mEntityManager->DestroyEntity(entity);
    mComponentManager->EntityDestroyed(entity);
    mSystemManager->EntityDestroyed(entity);
  }

  // Component methods
  template <typename T> void RegisterComponent() {
    mComponentManager->RegisterComponent<T>();
  }

  template <typename T> void AddComponent(Entity entity, T component) {
    mComponentManager->AddComponent<T>(entity, component);
  }

  template <typename T> void RemoveComponent(Entity entity) {
    mComponentManager->RemoveComponent<T>(entity);
  }

  template <typename T> T &GetComponent(Entity entity) {
    return mComponentManager->GetComponent<T>(entity);
  }

  template <typename T> bool HasComponent(Entity entity) {
    return mComponentManager->HasComponent<T>(entity);
  }

  // System methods
  template <typename T> std::shared_ptr<T> RegisterSystem() {
    return mSystemManager->RegisterSystem<T>();
  }

  template <typename T> std::shared_ptr<T> GetSystem() {
    return mSystemManager->GetSystem<T>();
  }

  template <typename SystemType, typename... ComponentTypes>
  void UpdateSystemEntities(Entity entity) {
    bool hasAll = (HasComponent<ComponentTypes>(entity) && ...);
    mSystemManager->EntitySignatureChanged<SystemType>(entity, hasAll);
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
