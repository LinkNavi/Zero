#pragma once
#include "Engine.h"
#include <glm/glm.hpp>
#include <vector>



struct PhysicsConfig {
    glm::vec3 gravity = glm::vec3(0, -9.8f, 0);
    float airDrag = 0.01f;
    int solverIterations = 4;
    bool enableCollisions = true;
};

struct RigidBody : Component {
    glm::vec3 velocity = glm::vec3(0);
    glm::vec3 angularVelocity = glm::vec3(0);
    float mass = 1.0f;
    float drag = 0.0f;
    bool useGravity = true;
    bool isKinematic = false;
};

enum class ColliderType { Box, Sphere, Capsule };

struct Collider : Component {
    ColliderType type = ColliderType::Box;
    glm::vec3 size = glm::vec3(1);
    float radius = 0.5f;
    bool isTrigger = false;
};

struct CollisionInfo {
    EntityID entityA;
    EntityID entityB;
    glm::vec3 point;
    glm::vec3 normal;
    float penetration;
};

class PhysicsSystem : public System {
public:
    PhysicsConfig config;
    ECS* ecs = nullptr;
    
    void update(float dt) override;
    std::vector<CollisionInfo> detectCollisions();
    void resolveCollision(const CollisionInfo& info);
    
private:
    bool checkBoxBox(EntityID a, EntityID b, CollisionInfo& info);
    bool checkSphereSphere(EntityID a, EntityID b, CollisionInfo& info);
    bool checkBoxSphere(EntityID a, EntityID b, CollisionInfo& info);
};
