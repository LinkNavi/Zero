#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include "Engine.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>

// Hierarchical transform component with quaternion rotation
struct Transform : Component {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f}; // w, x, y, z (identity)
    glm::vec3 scale{1.0f};
    
    // Hierarchy
    EntityID parent = 0;
    std::vector<EntityID> children;
    
    Transform() = default;
    
    Transform(glm::vec3 pos, glm::vec3 eulerAngles = glm::vec3(0), glm::vec3 scl = glm::vec3(1))
        : position(pos), scale(scl) {
        setEulerAngles(eulerAngles);
    }
    
   glm::mat4 getLocalMatrix() const {
    glm::mat4 mat = glm::translate(glm::mat4(1.0f), position);
    mat = mat * glm::mat4_cast(rotation);
    mat = glm::scale(mat, scale);
    return mat;
}

// Add this new method
glm::mat4 getWorldMatrix(ECS* ecs) const {
    glm::mat4 local = getLocalMatrix();
    
    if (parent == 0) {
        return local;
    }
    
    // Get parent's world matrix recursively
    auto* parentTransform = ecs->getComponent<Transform>(parent);
    if (parentTransform) {
        return parentTransform->getWorldMatrix(ecs) * local;
    }
    
    return local;
}
    
   
    
    // Get world position
    glm::vec3 getWorldPosition(ECS* ecs) const {
        if (parent == 0) {
            return position;
        }
        glm::mat4 world = getWorldMatrix(ecs);
        return glm::vec3(world[3]);
    }
    
    // Get world rotation
    glm::quat getWorldRotation(ECS* ecs) const {
        if (parent == 0) {
            return rotation;
        }
        
        auto* parentTransform = ecs->getComponent<Transform>(parent);
        if (parentTransform) {
            return parentTransform->getWorldRotation(ecs) * rotation;
        }
        
        return rotation;
    }
    
    // Get world scale
    glm::vec3 getWorldScale(ECS* ecs) const {
        if (parent == 0) {
            return scale;
        }
        
        auto* parentTransform = ecs->getComponent<Transform>(parent);
        if (parentTransform) {
            return parentTransform->getWorldScale(ecs) * scale;
        }
        
        return scale;
    }
    
    // === Rotation Helpers ===
    
    // Set rotation from Euler angles (degrees)
    void setEulerAngles(glm::vec3 angles) {
        rotation = glm::quat(glm::radians(angles));
    }
    
    // Get rotation as Euler angles (degrees)
    glm::vec3 getEulerAngles() const {
        return glm::degrees(glm::eulerAngles(rotation));
    }
    
    // Rotate by Euler angles (degrees)
    void rotate(glm::vec3 eulerAngles) {
        glm::quat deltaRotation = glm::quat(glm::radians(eulerAngles));
        rotation = rotation * deltaRotation;
    }
    
    // Rotate around axis (degrees)
    void rotateAround(glm::vec3 axis, float angleDegrees) {
        glm::quat deltaRotation = glm::angleAxis(glm::radians(angleDegrees), glm::normalize(axis));
        rotation = rotation * deltaRotation;
    }
    
    // Look at target
    void lookAt(glm::vec3 target, glm::vec3 up = glm::vec3(0, 1, 0)) {
        glm::vec3 direction = glm::normalize(target - position);
        if (glm::length(direction) < 0.001f) return;
        
        rotation = glm::quatLookAt(direction, up);
    }
    
    // === Direction Helpers ===
    
    glm::vec3 forward() const {
        return glm::normalize(rotation * glm::vec3(0, 0, -1));
    }
    
    glm::vec3 right() const {
        return glm::normalize(rotation * glm::vec3(1, 0, 0));
    }
    
    glm::vec3 up() const {
        return glm::normalize(rotation * glm::vec3(0, 1, 0));
    }
    
    // === Translation Helpers ===
    
    void translate(glm::vec3 delta) {
        position += delta;
    }
    
    void translateLocal(glm::vec3 delta) {
        position += rotation * delta;
    }
    
    // === Hierarchy Helpers ===
    
    void setParent(ECS* ecs, EntityID newParent) {
        // Remove from old parent
        if (parent != 0) {
            auto* oldParentTransform = ecs->getComponent<Transform>(parent);
            if (oldParentTransform) {
                auto& siblings = oldParentTransform->children;
                siblings.erase(std::remove(siblings.begin(), siblings.end(), parent), siblings.end());
            }
        }
        
        // Set new parent
        parent = newParent;
        
        // Add to new parent's children
        if (parent != 0) {
            auto* newParentTransform = ecs->getComponent<Transform>(parent);
            if (newParentTransform) {
                newParentTransform->children.push_back(parent);
            }
        }
    }
    
    void addChild(EntityID child) {
        if (std::find(children.begin(), children.end(), child) == children.end()) {
            children.push_back(child);
        }
    }
    
    void removeChild(EntityID child) {
        children.erase(std::remove(children.begin(), children.end(), child), children.end());
    }
    
    bool hasChild(EntityID child) const {
        return std::find(children.begin(), children.end(), child) != children.end();
    }
    
    // === Utility ===
    
    // Get distance to another transform
    float distanceTo(const Transform& other) const {
        return glm::distance(position, other.position);
    }
    
    // Get direction to another transform
    glm::vec3 directionTo(const Transform& other) const {
        return glm::normalize(other.position - position);
    }
};

// Transform system for updating hierarchies
class TransformSystem {
    ECS* ecs = nullptr;
    
public:
    void init(ECS* ecsInstance) {
        ecs = ecsInstance;
    }
    
    // Update all transforms (call if hierarchy changes)
    void updateHierarchy() {
        // In a full implementation, you'd traverse the hierarchy
        // and update dirty flags, but for simplicity we recalculate on demand
    }
    
    // Detach entity from parent (but keep world transform)
    void detachFromParent(EntityID entity, bool keepWorldTransform = true) {
        auto* transform = ecs->getComponent<Transform>(entity);
        if (!transform || transform->parent == 0) return;
        
        if (keepWorldTransform) {
            // Calculate world transform before detaching
            glm::vec3 worldPos = transform->getWorldPosition(ecs);
            glm::quat worldRot = transform->getWorldRotation(ecs);
            glm::vec3 worldScale = transform->getWorldScale(ecs);
            
            // Remove parent
            transform->setParent(ecs, 0);
            
            // Set local transform to world transform
            transform->position = worldPos;
            transform->rotation = worldRot;
            transform->scale = worldScale;
        } else {
            transform->setParent(ecs, 0);
        }
    }
    
    // Attach entity to parent (converting world transform to local)
    void attachToParent(EntityID entity, EntityID newParent, bool keepWorldTransform = true) {
        auto* transform = ecs->getComponent<Transform>(entity);
        if (!transform) return;
        
        if (keepWorldTransform && newParent != 0) {
            // Save world transform
            glm::vec3 worldPos = transform->getWorldPosition(ecs);
            glm::quat worldRot = transform->getWorldRotation(ecs);
            glm::vec3 worldScale = transform->getWorldScale(ecs);
            
            // Set parent
            transform->setParent(ecs, newParent);
            
            // Convert world transform to local
            auto* parentTransform = ecs->getComponent<Transform>(newParent);
            if (parentTransform) {
                glm::mat4 parentWorldInv = glm::inverse(parentTransform->getWorldMatrix(ecs));
                glm::vec4 localPos4 = parentWorldInv * glm::vec4(worldPos, 1.0f);
                transform->position = glm::vec3(localPos4);
                
                glm::quat parentWorldRotInv = glm::inverse(parentTransform->getWorldRotation(ecs));
                transform->rotation = parentWorldRotInv * worldRot;
                
                glm::vec3 parentWorldScale = parentTransform->getWorldScale(ecs);
                transform->scale = worldScale / parentWorldScale;
            }
        } else {
            transform->setParent(ecs, newParent);
        }
    }
    
    // Get all root transforms (transforms with no parent)
    std::vector<EntityID> getRootTransforms() {
        std::vector<EntityID> roots;
        for (size_t i = 0; i < 10000; ++i) {
            auto* transform = ecs->getComponent<Transform>(i);
            if (transform && transform->parent == 0) {
                roots.push_back(i);
            }
        }
        return roots;
    }
    
    // Get all children recursively
    void getChildrenRecursive(EntityID entity, std::vector<EntityID>& outChildren) {
        auto* transform = ecs->getComponent<Transform>(entity);
        if (!transform) return;
        
        for (EntityID child : transform->children) {
            outChildren.push_back(child);
            getChildrenRecursive(child, outChildren);
        }
    }
};
