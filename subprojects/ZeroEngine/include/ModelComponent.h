#pragma once
#include "ModelLoader.h"
#include <string>

struct ModelComponent {
    std::string modelPath;
    Model* loadedModel = nullptr;
    
    ModelComponent() = default;
    ModelComponent(const std::string& path) : modelPath(path) {}
};
