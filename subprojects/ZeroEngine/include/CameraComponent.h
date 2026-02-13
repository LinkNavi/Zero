#pragma once
#include "Camera.h"

struct CameraComponent {
    Camera camera;
    bool isActive = false;  // Only one active camera renders
    
    CameraComponent() = default;
    CameraComponent(const Camera& cam) : camera(cam) {}
};
