#pragma once
#include <glm/glm.hpp>

// Push constants with shadow matrix
struct PushConstantsShadow {
    glm::mat4 viewProj;
    glm::mat4 model;
    glm::mat4 lightViewProj;
    glm::vec3 lightDir;
    float ambientStrength;
    glm::vec3 lightColor;
    float shadowBias;
};

// Shadow pass push constants (smaller)
struct ShadowPushConstants {
    glm::mat4 lightViewProj;
    glm::mat4 model;
};
