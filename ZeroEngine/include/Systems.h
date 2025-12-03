#pragma once
#include "ECS.h"
#include "Objects.h"
#include "Renderer.h"
#include "iostream"
#include <memory>
#include <raylib.h>

#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>
// Keep compatibility with older code that used MyTransform name
struct ZeroTransform {
  Vector3 position{0, 0, 0};
  Vector3 rotation{0, 0, 0};
  Vector3 scale{1, 1, 1};
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
  Vector3 v{0, 0, 0};
};

// Input receiver marker + speed multiplier
struct InputReceiver {
  float speed = 50.0f; // units per second
};

// Camera follow
struct CameraFollow {
  Entity target = 0; // entity to follow
  Vector3 offset{0, 3, 10};
};

// ------------------- Systems -------------------

// RotationSystem - keep as an example (keeps previous functionality)
class RotationSystem : public System {
public:
  ECS *ecs = nullptr;

  void Update(float dt) override {
    (void)dt;
    for (auto e : mEntities) {
      if (!ecs->HasComponent<MyTransform>(e))
        continue;
      auto &t = ecs->GetComponent<MyTransform>(e);
      t.rotation.x += 30.0f * dt;
      t.rotation.y += 20.0f * dt;
    }
  }
};

struct InputAction {
  std::string name;      // e.g. "MoveForward"
  std::vector<int> keys; // raylib KEY_* values
};

class InputBindings {
public:
  std::unordered_map<std::string, InputAction> actions;

  // Load from JSON file
  bool LoadFromFile(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open())
      return false;

    nlohmann::json j;
    file >> j;

    for (auto &[actionName, keyArray] : j.items()) {
      InputAction action;
      action.name = actionName;
      for (auto &keyStr : keyArray) {
        // convert string "KEY_W" to raylib key constant
        int key = StringToKey(keyStr.get<std::string>());
        action.keys.push_back(key);
      }
      actions[actionName] = action;
    }

    return true;
  }

  bool IsPressed(const std::string &actionName) const {
    auto it = actions.find(actionName);
    if (it == actions.end())
      return false;
    for (auto key : it->second.keys)
      if (IsKeyDown(key))
        return true;
    return false;
  }

private:
  int StringToKey(const std::string &str) const {
    if (str == "KEY_W")
      return KEY_W;
    if (str == "KEY_A")
      return KEY_A;
    if (str == "KEY_S")
      return KEY_S;
    if (str == "KEY_D")
      return KEY_D;
    if (str == "KEY_UP")
      return KEY_UP;
    if (str == "KEY_DOWN")
      return KEY_DOWN;
    if (str == "KEY_LEFT")
      return KEY_LEFT;
    if (str == "KEY_RIGHT")
      return KEY_RIGHT;
    if (str == "KEY_SPACE")
      return KEY_SPACE;
    if (str == "KEY_LEFT_SHIFT")
      return KEY_LEFT_SHIFT;
    // add more as needed
    return 0;
  }
};
// InputSystem - reads keyboard into Velocity for entities that want input

class InputSystem : public System {
public:
  ECS *ecs = nullptr;
  InputBindings bindings;

  void Update(float dt) override {
    for (auto e : mEntities) {
      if (!ecs->HasComponent<InputReceiver>(e) ||
          !ecs->HasComponent<Velocity>(e))
        continue;

      auto &input = ecs->GetComponent<InputReceiver>(e);
      auto &vel = ecs->GetComponent<Velocity>(e);

      vel.v = {0, 0, 0};

      // Movement
      if (bindings.IsPressed("MoveForward"))
        vel.v.z -= 1.0f;
      if (bindings.IsPressed("MoveBackward"))
        vel.v.z += 1.0f;
      if (bindings.IsPressed("MoveLeft"))
        vel.v.x -= 1.0f;
      if (bindings.IsPressed("MoveRight"))
        vel.v.x += 1.0f;
      if (bindings.IsPressed("Jump"))
        vel.v.y += 1.0f;
      if (bindings.IsPressed("Crouch"))
        vel.v.y -= 1.0f;

      // Normalize diagonal movement
      float len = sqrtf(vel.v.x * vel.v.x + vel.v.z * vel.v.z);
      if (len > 0.0001f) {
        vel.v.x = (vel.v.x / len) * input.speed;
        vel.v.z = (vel.v.z / len) * input.speed;
      }
      vel.v.y *= input.speed; // scale vertical by speed
    }
  }
};
// PhysicsSystem - integrate velocity into Transform
class PhysicsSystem : public System {
public:
  ECS *ecs = nullptr;

  void Update(float dt) override {
    for (auto e : mEntities) {
      if (!ecs->HasComponent<ZeroTransform>(e) ||
          !ecs->HasComponent<Velocity>(e))
        continue;

      auto &t = ecs->GetComponent<ZeroTransform>(e);
      auto &vel = ecs->GetComponent<Velocity>(e);

      std::cout << "Player pos: " << t.position.x << ", " << t.position.y
                << ", " << t.position.z << "\n";
      t.position.x += vel.v.x * dt;
      t.position.y += vel.v.y * dt;
      t.position.z += vel.v.z * dt;
    }
  }
};

// CameraSystem - sets renderer camera to follow the entity
class CameraSystem : public System {
public:
  ECS *ecs = nullptr;
  Renderer *renderer = nullptr;

  void Update(float dt) override {
    (void)dt;
    if (!renderer)
      return;

    // Find any entity with CameraFollow - assume only one for now
    for (auto e : mEntities) {
      if (!ecs->HasComponent<CameraFollow>(e))
        continue;
      auto &camFollow = ecs->GetComponent<CameraFollow>(e);

      // require the target to have a transform
      if (!ecs->HasComponent<MyTransform>(camFollow.target))
        continue;

      auto &t = ecs->GetComponent<MyTransform>(camFollow.target);

      // Set underlying raylib camera directly
      // renderer exposes a Camera variable so we can change it in-place
      renderer->camera.target = {0, 0, 0};
      renderer->camera.position = {10, 10, 10};
      // Up, fovy left as-is
    }
  }
};

// RenderSystem - draws scenes using renderer
class RenderSystem : public System {
public:
  ECS *ecs = nullptr;
  Renderer *renderer = nullptr;

  void Update(float dt) override {
    (void)dt;
    if (!renderer)
      return;

    renderer->Begin3D();
    for (auto e : mEntities) {
      if (!ecs->HasComponent<MyTransform>(e) ||
          !ecs->HasComponent<MeshRendererComponent>(e))
        continue;

      auto &t = ecs->GetComponent<MyTransform>(e);
      auto &mr = ecs->GetComponent<MeshRendererComponent>(e);
      if (!mr.isValid || !mr.mesh)
        continue;

      // dereference shared ptr
      renderer->DrawMesh(*mr.mesh, t.position, t.rotation, t.scale, WHITE);
    }
    renderer->End3D();
  }
};

// DebugSystem - draws FPS and entity counts
class DebugSystem : public System {
public:
  ECS *ecs = nullptr;

  void Update(float dt) override {
    (void)dt;
    int fps = GetFPS();

    auto living = ecs->GetEntityCount();
    // Draw as overlay (use raylib immediate mode)
    DrawText(TextFormat("FPS: %d", fps), 10, 10, 20, BLACK);
    DrawText(TextFormat("Entities: %zu", ecs->GetEntityCount()), 10, 34, 20,
             BLACK);
  }
};
