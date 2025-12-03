#pragma once
#include "ECS.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "Systems.h"
#include <memory>

class World {
public:
  ECS ecs;
  Renderer renderer;
  ResourceManager resources;

  std::shared_ptr<RotationSystem> rotationSystem;
  std::shared_ptr<InputSystem> inputSystem;
  std::shared_ptr<PhysicsSystem> physicsSystem;
  std::shared_ptr<CameraSystem> cameraSystem;
  std::shared_ptr<RenderSystem> renderSystem;
  std::shared_ptr<DebugSystem> debugSystem;

  Shader defaultShader{};
  std::shared_ptr<ZeroMesh> sharedCube;

  void Init(int width, int height, const char *title) {
    renderer.Init(width, height, title);
    ecs.Init();

    // Register components used by systems
    ecs.RegisterComponent<MyTransform>();
    ecs.RegisterComponent<MeshRendererComponent>();
    ecs.RegisterComponent<Velocity>();
    ecs.RegisterComponent<InputReceiver>();
    ecs.RegisterComponent<CameraFollow>();

    // Register systems and wire ECS + renderer
    rotationSystem = ecs.RegisterSystem<RotationSystem>();
    rotationSystem->ecs = &ecs;

    inputSystem = ecs.RegisterSystem<InputSystem>();
    inputSystem->ecs = &ecs;

    inputSystem = ecs.RegisterSystem<InputSystem>();
    inputSystem->ecs = &ecs;

    // Load input bindings from JSON at runtime

    if (!inputSystem->bindings.LoadFromFile("input_bindings.json")) {
      std::cerr << "[Warning] Failed to load input_bindings.json, using "
                   "default WASD keys.\n";

      inputSystem->bindings.actions = {
          {"MoveForward", {{"KEY_W", "KEY_UP"}}},
          {"MoveBackward", {{"KEY_S", "KEY_DOWN"}}},
          {"MoveLeft", {{"KEY_A", "KEY_LEFT"}}},
          {"MoveRight", {{"KEY_D", "KEY_RIGHT"}}},
          {"Jump", {{"KEY_SPACE"}}},
          {"Crouch", {{"KEY_LEFT_SHIFT"}}}};
    }

    physicsSystem = ecs.RegisterSystem<PhysicsSystem>();
    physicsSystem->ecs = &ecs;

    cameraSystem = ecs.RegisterSystem<CameraSystem>();
    cameraSystem->ecs = &ecs;
    cameraSystem->renderer = &renderer;

    renderSystem = ecs.RegisterSystem<RenderSystem>();
    renderSystem->ecs = &ecs;
    renderSystem->renderer = &renderer;

    debugSystem = ecs.RegisterSystem<DebugSystem>();
    debugSystem->ecs = &ecs;

    // Shared resources
    sharedCube = resources.GetCube(1.0f);
    defaultShader = resources.GetDefaultShader();
  }

  // Create a cube entity that is controllable by input

  Entity CreatePlayerCube(float size = 1.0f, bool giveInput = true) {
    Entity e = ecs.CreateEntity();

    MyTransform t;
    t.position = {0, 0, 0};
    t.scale = {size, size, size};

    MeshRendererComponent mr;
    mr.mesh = sharedCube;
    mr.shader = defaultShader;
    mr.isValid = (sharedCube != nullptr);

    Velocity vel;
    vel.v = {0, 0, 0};

    ecs.AddComponent(e, t);
    ecs.AddComponent(e, mr);
    ecs.AddComponent(e, vel);

    if (giveInput) {
      InputReceiver in;
      in.speed = 50.0f; // << bump speed for visibility
      ecs.AddComponent(e, in);
    }

    ecs.UpdateSystemEntities<RotationSystem, MyTransform>(e);
    ecs.UpdateSystemEntities<InputSystem, InputReceiver, Velocity>(e);
    ecs.UpdateSystemEntities<PhysicsSystem, MyTransform, Velocity>(e);
    ecs.UpdateSystemEntities<RenderSystem, MyTransform, MeshRendererComponent>(
        e);

    return e;
  }

  // Create a camera entity to follow a target

  Entity CreateCameraEntity(Entity target, Vector3 offset = {0, 5, 10}) {
    Entity e = ecs.CreateEntity();
    CameraFollow cf;
    cf.target = target;
    cf.offset = offset; // smaller offset for better visibility
    ecs.AddComponent(e, cf);
    ecs.UpdateSystemEntities<CameraSystem, CameraFollow>(e);
    return e;
  }

  // Update world (runs systems)
  void Update(float dt) {
    // Order: input -> physics -> camera -> render -> debug
    inputSystem->Update(dt);
    physicsSystem->Update(dt);
    cameraSystem->Update(dt);
    renderSystem->Update(dt);
    debugSystem->Update(dt);
  }

  void Shutdown() {
    resources.Shutdown();
    renderer.Shutdown();
  }
};
