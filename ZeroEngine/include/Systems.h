#pragma once
#include "ECS.h"
#include "Renderer.h"
#include "Objects.h"
#include "Logger.h"
#include <raylib.h>
#include <memory>
#include <cmath>

// Components
struct ZeroTransform {
    Vector3 position{0,0,0};
    Vector3 rotation{0,0,0};
    Vector3 scale{1,1,1};
};
using MyTransform = ZeroTransform;

struct MeshRendererComponent {
    std::shared_ptr<ZeroMesh> mesh;
    Shader shader{};
    bool isValid = false;
    Color tint = WHITE;
};

struct Velocity {
    Vector3 v{0,0,0};
};

struct InputReceiver {
    float speed = 50.0f;
    bool enabled = true;
};

struct CameraFollow {
    Entity target = 0;
    Vector3 offset{0,3,10};
    float smoothing = 5.0f; // Smoothing factor
};

struct Lifetime {
    float timeLeft = 5.0f; // Time until entity is destroyed
};

struct Tag {
    std::string name = "Entity";
};

// ------------------- Systems -------------------

class RotationSystem : public System {
public:
    ECS* ecs = nullptr;

    void Update(float dt) override {
        for (auto e : mEntities) {
            if (!ecs->HasComponent<MyTransform>(e)) continue;
            
            auto& t = ecs->GetComponent<MyTransform>(e);
            t.rotation.x += 30.0f * dt;
            t.rotation.y += 20.0f * dt;
        }
    }
};

class InputSystem : public System {
public:
    ECS* ecs = nullptr;

    void Update(float dt) override {
        for (auto e : mEntities) {
            if (!ecs->HasComponent<InputReceiver>(e) ||
                !ecs->HasComponent<Velocity>(e)) continue;

            auto& input = ecs->GetComponent<InputReceiver>(e);
            if (!input.enabled) continue;

            auto& vel = ecs->GetComponent<Velocity>(e);
            vel.v = {0,0,0};

            // WASD / Arrow controls
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) vel.v.x += 1.0f;
            if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) vel.v.x -= 1.0f;
            if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) vel.v.z -= 1.0f;
            if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) vel.v.z += 1.0f;

            // Normalize diagonal movement
            float len = sqrtf(vel.v.x*vel.v.x + vel.v.z*vel.v.z);
            if (len > 0.0001f) {
                vel.v.x = (vel.v.x / len) * input.speed;
                vel.v.z = (vel.v.z / len) * input.speed;
            }

            // Vertical control
            if (IsKeyDown(KEY_SPACE)) vel.v.y += input.speed;
            if (IsKeyDown(KEY_LEFT_SHIFT)) vel.v.y -= input.speed;
        }
    }
};

class PhysicsSystem : public System {
public:
    ECS* ecs = nullptr;

    void Update(float dt) override {
        for (auto e : mEntities) {
            if (!ecs->HasComponent<ZeroTransform>(e) ||
                !ecs->HasComponent<Velocity>(e)) continue;

            auto& t = ecs->GetComponent<ZeroTransform>(e);
            auto& vel = ecs->GetComponent<Velocity>(e);

            t.position.x += vel.v.x * dt;
            t.position.y += vel.v.y * dt;
            t.position.z += vel.v.z * dt;
        }
    }
};

class CameraSystem : public System {
public:
    ECS* ecs = nullptr;
    Renderer* renderer = nullptr;

    void Update(float dt) override {
        if (!renderer) return;

        for (auto e : mEntities) {
            if (!ecs->HasComponent<CameraFollow>(e)) continue;
            
            auto& camFollow = ecs->GetComponent<CameraFollow>(e);

            if (!ecs->HasComponent<MyTransform>(camFollow.target)) continue;

            auto& t = ecs->GetComponent<MyTransform>(camFollow.target);

            // Smooth camera following
            Vector3 targetPos = {
                t.position.x + camFollow.offset.x,
                t.position.y + camFollow.offset.y,
                t.position.z + camFollow.offset.z
            };

            // Lerp camera position for smooth following
            float smoothFactor = 1.0f - expf(-camFollow.smoothing * dt);
            
            renderer->camera.position.x += (targetPos.x - renderer->camera.position.x) * smoothFactor;
            renderer->camera.position.y += (targetPos.y - renderer->camera.position.y) * smoothFactor;
            renderer->camera.position.z += (targetPos.z - renderer->camera.position.z) * smoothFactor;

            renderer->camera.target = t.position;
        }
    }
};

class RenderSystem : public System {
public:
    ECS* ecs = nullptr;
    Renderer* renderer = nullptr;

    void Update(float dt) override {
        (void)dt;
        if (!renderer) return;

        renderer->Begin3D();
        
        // Draw grid
        DrawGrid(20, 1.0f);

        for (auto e : mEntities) {
            if (!ecs->HasComponent<MyTransform>(e) ||
                !ecs->HasComponent<MeshRendererComponent>(e)) continue;

            auto& t = ecs->GetComponent<MyTransform>(e);
            auto& mr = ecs->GetComponent<MeshRendererComponent>(e);
            
            if (!mr.isValid || !mr.mesh) continue;

            renderer->DrawMesh(*mr.mesh, t.position, t.rotation, t.scale, mr.tint);
        }
        
        renderer->End3D();
    }
};

class LifetimeSystem : public System {
public:
    ECS* ecs = nullptr;
    std::vector<Entity> toDestroy;

    void Update(float dt) override {
        toDestroy.clear();

        for (auto e : mEntities) {
            if (!ecs->HasComponent<Lifetime>(e)) continue;

            auto& lifetime = ecs->GetComponent<Lifetime>(e);
            lifetime.timeLeft -= dt;

            if (lifetime.timeLeft <= 0.0f) {
                toDestroy.push_back(e);
            }
        }

        // Destroy entities after iteration
        for (auto e : toDestroy) {
            LOG_DEBUG("Destroying entity " + std::to_string(e) + " due to lifetime expiration");
            ecs->DestroyEntity(e);
        }
    }
};

class DebugSystem : public System {
public:
    ECS* ecs = nullptr;
    bool showDebugInfo = true;

    void Update(float dt) override {
        (void)dt;
        
        if (!showDebugInfo) return;

        int fps = GetFPS();
        uint32_t entityCount = ecs->GetEntityCount();
        
        DrawText(TextFormat("FPS: %d", fps), 10, 10, 20, GREEN);
        DrawText(TextFormat("Entities: %d", entityCount), 10, 35, 20, GREEN);
        DrawText(TextFormat("Delta: %.3f", dt), 10, 60, 20, GREEN);

        // Toggle debug info with F3
        if (IsKeyPressed(KEY_F3)) {
            showDebugInfo = !showDebugInfo;
        }
    }
};
