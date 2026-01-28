#include "SceneManager.h"
#include <iostream>

void SceneManager::registerScene(const std::string& name, Scene* scene) {
    scene->name = name;
    scenes[name] = scene;
}

void SceneManager::loadScene(const std::string& name) {
    auto it = scenes.find(name);
    if (it == scenes.end()) {
        std::cerr << "Scene not found: " << name << std::endl;
        return;
    }
    
    nextScene = it->second;
    transitioning = true;
}

void SceneManager::update(float dt) {
    if (transitioning) {
        if (currentScene) {
            currentScene->onUnload();
        }
        currentScene = nextScene;
        if (currentScene) {
            currentScene->onLoad();
        }
        transitioning = false;
    }
    
    if (currentScene) {
        currentScene->update(dt);
    }
}

void SceneManager::cleanup() {
    if (currentScene) {
        currentScene->onUnload();
    }
    
    for (auto& [name, scene] : scenes) {
        delete scene;
    }
    scenes.clear();
}
