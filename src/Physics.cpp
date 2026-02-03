#include "PhysicsSystem.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include "transform.h"


void PhysicsSystem::update(float dt) {
    if (!ecs) return;
    
    // Apply forces
    for (EntityID entity : entities) {
        auto* transform = ecs->getComponent<Transform>(entity);
        auto* rb = ecs->getComponent<RigidBody>(entity);
        
        if (!transform || !rb || rb->isKinematic) continue;
        
        // Gravity
        if (rb->useGravity) {
            rb->velocity += config.gravity * dt;
        }
        
        // Drag
        float drag = config.airDrag + rb->drag;
        rb->velocity *= (1.0f - drag * dt);
        rb->angularVelocity *= (1.0f - drag * dt);
        
        // Integration
        transform->position += rb->velocity * dt;
        transform->rotation += rb->angularVelocity * dt;
    }
    
    // Collision detection and resolution
    if (config.enableCollisions) {
        auto collisions = detectCollisions();
        for (int iter = 0; iter < config.solverIterations; ++iter) {
            for (const auto& col : collisions) {
                resolveCollision(col);
            }
        }
    }
}

std::vector<CollisionInfo> PhysicsSystem::detectCollisions() {
    std::vector<CollisionInfo> collisions;
    
    for (size_t i = 0; i < entities.size(); ++i) {
        for (size_t j = i + 1; j < entities.size(); ++j) {
            CollisionInfo info;
            
            auto* collA = ecs->getComponent<Collider>(entities[i]);
            auto* collB = ecs->getComponent<Collider>(entities[j]);
            
            if (!collA || !collB) continue;
            
            bool collided = false;
            
            if (collA->type == ColliderType::Box && collB->type == ColliderType::Box) {
                collided = checkBoxBox(entities[i], entities[j], info);
            } else if (collA->type == ColliderType::Sphere && collB->type == ColliderType::Sphere) {
                collided = checkSphereSphere(entities[i], entities[j], info);
            } else {
                collided = checkBoxSphere(entities[i], entities[j], info);
            }
            
            if (collided) {
                collisions.push_back(info);
            }
        }
    }
    
    return collisions;
}

void PhysicsSystem::resolveCollision(const CollisionInfo& info) {
    auto* transA = ecs->getComponent<Transform>(info.entityA);
    auto* transB = ecs->getComponent<Transform>(info.entityB);
    auto* rbA = ecs->getComponent<RigidBody>(info.entityA);
    auto* rbB = ecs->getComponent<RigidBody>(info.entityB);
    auto* collA = ecs->getComponent<Collider>(info.entityA);
    auto* collB = ecs->getComponent<Collider>(info.entityB);
    
    if (!transA || !transB) return;
    if (collA->isTrigger || collB->isTrigger) return;
    
    // Separate objects
    float totalMass = (rbA ? rbA->mass : 1.0f) + (rbB ? rbB->mass : 1.0f);
    float ratioA = rbB ? rbB->mass / totalMass : 0.5f;
    float ratioB = rbA ? rbA->mass / totalMass : 0.5f;
    
    if (!rbA || !rbA->isKinematic) {
        transA->position -= info.normal * info.penetration * ratioA;
    }
    if (!rbB || !rbB->isKinematic) {
        transB->position += info.normal * info.penetration * ratioB;
    }
    
    // Resolve velocities
    if (rbA && rbB && !rbA->isKinematic && !rbB->isKinematic) {
        glm::vec3 relVel = rbB->velocity - rbA->velocity;
        float velAlongNormal = glm::dot(relVel, info.normal);
        
        if (velAlongNormal < 0) {
            float restitution = 0.5f;
            float j = -(1.0f + restitution) * velAlongNormal;
            j /= (1.0f / rbA->mass + 1.0f / rbB->mass);
            
            glm::vec3 impulse = j * info.normal;
            rbA->velocity -= impulse / rbA->mass;
            rbB->velocity += impulse / rbB->mass;
        }
    }
}

bool PhysicsSystem::checkBoxBox(EntityID a, EntityID b, CollisionInfo& info) {
    auto* transA = ecs->getComponent<Transform>(a);
    auto* transB = ecs->getComponent<Transform>(b);
    auto* collA = ecs->getComponent<Collider>(a);
    auto* collB = ecs->getComponent<Collider>(b);
    
    glm::vec3 halfA = collA->size * transA->scale * 0.5f;
    glm::vec3 halfB = collB->size * transB->scale * 0.5f;
    
    glm::vec3 delta = transB->position - transA->position;
    glm::vec3 overlap = (halfA + halfB) - glm::abs(delta);
    
    if (overlap.x > 0 && overlap.y > 0 && overlap.z > 0) {
        info.entityA = a;
        info.entityB = b;
        info.point = (transA->position + transB->position) * 0.5f;
        
        // Find minimum overlap axis
        if (overlap.x < overlap.y && overlap.x < overlap.z) {
            info.normal = glm::vec3(delta.x > 0 ? 1 : -1, 0, 0);
            info.penetration = overlap.x;
        } else if (overlap.y < overlap.z) {
            info.normal = glm::vec3(0, delta.y > 0 ? 1 : -1, 0);
            info.penetration = overlap.y;
        } else {
            info.normal = glm::vec3(0, 0, delta.z > 0 ? 1 : -1);
            info.penetration = overlap.z;
        }
        return true;
    }
    return false;
}

bool PhysicsSystem::checkSphereSphere(EntityID a, EntityID b, CollisionInfo& info) {
    auto* transA = ecs->getComponent<Transform>(a);
    auto* transB = ecs->getComponent<Transform>(b);
    auto* collA = ecs->getComponent<Collider>(a);
    auto* collB = ecs->getComponent<Collider>(b);
    
    float radiusA = collA->radius * glm::max(glm::max(transA->scale.x, transA->scale.y), transA->scale.z);
    float radiusB = collB->radius * glm::max(glm::max(transB->scale.x, transB->scale.y), transB->scale.z);
    
    glm::vec3 delta = transB->position - transA->position;
    float distSq = glm::dot(delta, delta);
    float radiusSum = radiusA + radiusB;
    
    if (distSq < radiusSum * radiusSum && distSq > 0.0001f) {
        info.entityA = a;
        info.entityB = b;
        float dist = glm::sqrt(distSq);
        info.normal = delta / dist;
        info.penetration = radiusSum - dist;
        info.point = transA->position + info.normal * radiusA;
        return true;
    }
    return false;
}

bool PhysicsSystem::checkBoxSphere(EntityID /*a*/, EntityID /*b*/, CollisionInfo& /*info*/) {
    // Simplified box-sphere collision
    return false;
}
