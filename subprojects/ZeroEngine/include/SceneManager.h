#pragma once
#include "Engine.h"
#include <string>
#include <unordered_map>
#include <functional>

class Scene {
public:
    std::string name;
    ECS ecs;
    
    virtual ~Scene() = default;
    virtual void onLoad() {}
    virtual void onUnload() {}
    virtual void update(float /*dt*/) {}
};

class SceneManager {
    std::unordered_map<std::string, Scene*> scenes;
    Scene* currentScene = nullptr;
    Scene* nextScene = nullptr;
    bool transitioning = false;
    
public:
    void registerScene(const std::string& name, Scene* scene);
    void loadScene(const std::string& name);
    void update(float dt);
    Scene* getCurrentScene() { return currentScene; }
    void cleanup();
};
