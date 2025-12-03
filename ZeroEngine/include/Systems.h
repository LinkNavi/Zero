#pragma once
#include "ECS.h"
#include "Renderer.h"
#include "Objects.h"
#include <raylib.h>
#include <memory>
#include "iostream"
// Keep compatibility with older code that used MyTransform name
struct ZeroTransform {
    Vector3 position{0,0,0};
    Vector3 rotation{0,0,0};
    Vector3 scale{1,1,1};
};
using MyTransform = ZeroTransform; // alias so older code still works

// Renderable component: holds shared_ptr to avoid heavy copying
struct MeshRendererComponent {
    std::shared_ptr<ZeroMesh> mesh; // shared ownership
    Shader shader{};
    bool isValid = false;
};

// Movement/Physics
struct Velocity {
    Vector3 v{0,0,0};
};

// Input receiver marker + speed multiplier
struct InputReceiver {
    float speed = 50.0f; // units per second
};

// Camera follow
struct CameraFollow {
    Entity target = 0;   // entity to follow
    Vector3 offset{0,3,10};
};

// ------------------- Systems -------------------

// RotationSystem - keep as an example (keeps previous functionality)
class RotationSystem : public System {
public:
    ECS* ecs = nullptr;

    void Update(float dt) override {
        (void)dt;
        for (auto e : mEntities) {
            if (!ecs->HasComponent<MyTransform>(e)) continue;
            auto& t = ecs->GetComponent<MyTransform>(e);
            t.rotation.x += 30.0f * dt;
            t.rotation.y += 20.0f * dt;
        }
    }
};

// InputSystem - reads keyboard into Velocity for entities that want input
class InputSystem : public System {
public:
    ECS* ecs = nullptr;

    void Update(float dt) override {
        (void)dt;
        for (auto e : mEntities) {
            if (!ecs->HasComponent<InputReceiver>(e) ||
                !ecs->HasComponent<Velocity>(e)) continue;

            auto& input = ecs->GetComponent<InputReceiver>(e);
            auto& vel = ecs->GetComponent<Velocity>(e);

            // zero out
            vel.v = {0,0,0};

            // WASD / Arrow controls
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) vel.v.x += 1.0f;
            if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) vel.v.x -= 1.0f;
            if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) vel.v.z -= 1.0f;
            if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) vel.v.z += 1.0f;

            // Normalize (if diagonal) and scale by speed
            float len = sqrtf(vel.v.x*vel.v.x + vel.v.z*vel.v.z);
            if (len > 0.0001f) {
                vel.v.x = (vel.v.x / len) * input.speed;
                vel.v.z = (vel.v.z / len) * input.speed;
            }

            // vertical control (optional)
            if (IsKeyDown(KEY_SPACE)) vel.v.y += input.speed;
            if (IsKeyDown(KEY_LEFT_SHIFT)) vel.v.y -= input.speed;
        }

for (auto e : mEntities) {
    std::cout << "Entity in InputSystem: " << e << "\n";

}
    }
};

// PhysicsSystem - integrate velocity into Transform
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

// CameraSystem - sets renderer camera to follow the entity
class CameraSystem : public System {
public:
    ECS* ecs = nullptr;
    Renderer* renderer = nullptr;

    void Update(float dt) override {
        (void)dt;
        if (!renderer) return;

        // Find any entity with CameraFollow - assume only one for now
        for (auto e : mEntities) {
            if (!ecs->HasComponent<CameraFollow>(e)) continue;
            auto& camFollow = ecs->GetComponent<CameraFollow>(e);

            // require the target to have a transform
            if (!ecs->HasComponent<MyTransform>(camFollow.target)) continue;

            auto& t = ecs->GetComponent<MyTransform>(camFollow.target);

            // Set underlying raylib camera directly
            // renderer exposes a Camera variable so we can change it in-place
            renderer->camera.target = t.position;
            renderer->camera.position = Vector3{t.position.x + camFollow.offset.x,
                                               t.position.y + camFollow.offset.y,
                                               t.position.z + camFollow.offset.z};
            // Up, fovy left as-is
        }
    }
};

// RenderSystem - draws scenes using renderer
class RenderSystem : public System {
public:
    ECS* ecs = nullptr;
    Renderer* renderer = nullptr;

    void Update(float dt) override {
        (void)dt;
        if (!renderer) return;

        renderer->Begin3D();
        for (auto e : mEntities) {
            if (!ecs->HasComponent<MyTransform>(e) ||
                !ecs->HasComponent<MeshRendererComponent>(e)) continue;

            auto& t = ecs->GetComponent<MyTransform>(e);
            auto& mr = ecs->GetComponent<MeshRendererComponent>(e);
            if (!mr.isValid || !mr.mesh) continue;

            // dereference shared ptr
            renderer->DrawMesh(*mr.mesh, t.position, t.rotation, t.scale, WHITE);
        }
        renderer->End3D();
    }
};

// DebugSystem - draws FPS and entity counts
class DebugSystem : public System {
public:
    ECS* ecs = nullptr;

    void Update(float dt) override {
        (void)dt;
        int fps = GetFPS();
      
auto living = ecs->GetEntityCount();
        // Draw as overlay (use raylib immediate mode)
        DrawText(TextFormat("FPS: %d", fps), 10, 10, 20, BLACK);
        DrawText(TextFormat("Entities: %zu", ecs->GetEntityCount()), 10, 34, 20, BLACK);
    }
};

