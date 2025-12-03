#pragma once
#include "ECS.h"
#include "Renderer.h"
#include "Systems.h"
#include "ResourceManager.h"
#include "Serialization.h"
#include "ScriptSystem.h"
#include "Logger.h"
#include <memory>
#include <string>

class World {
public:
    ECS ecs;
    Renderer renderer;
    ResourceManager resources;

    // Systems
    std::shared_ptr<ScriptSystem> scriptSystem;
    std::shared_ptr<RotationSystem> rotationSystem;
    std::shared_ptr<InputSystem> inputSystem;
    std::shared_ptr<PhysicsSystem> physicsSystem;
    std::shared_ptr<CameraSystem> cameraSystem;
    std::shared_ptr<RenderSystem> renderSystem;
    std::shared_ptr<LifetimeSystem> lifetimeSystem;
    std::shared_ptr<DebugSystem> debugSystem;

    Shader defaultShader{};
    std::shared_ptr<ZeroMesh> sharedCube;

    bool isPaused = false;

    void Init(int width, int height, const char* title) {
        LOG_INFO("Initializing World...");

        if (!renderer.Init(width, height, title)) {
            LOG_FATAL("Failed to initialize renderer");
            return;
        }

        ecs.Init();

        // Register all components
        RegisterComponents();

        // Register all systems
        RegisterSystems();

        // Load shared resources
        sharedCube = resources.GetCube(1.0f);
        defaultShader = resources.GetDefaultShader();

        LOG_INFO("World initialization complete");
    }

    void RegisterComponents() {
        LOG_DEBUG("Registering components...");
        
        ecs.RegisterComponent<MyTransform>();
        ecs.RegisterComponent<MeshRendererComponent>();
        ecs.RegisterComponent<Velocity>();
        ecs.RegisterComponent<InputReceiver>();
        ecs.RegisterComponent<CameraFollow>();
        ecs.RegisterComponent<Lifetime>();
        ecs.RegisterComponent<Tag>();
        ecs.RegisterComponent<ScriptComponent>();  // NEW: Script component
    }

    void RegisterSystems() {
        LOG_DEBUG("Registering systems...");

        // Script System - NEW: Initialize first so scripts can override behavior
        scriptSystem = ecs.RegisterSystem<ScriptSystem>();
        scriptSystem->ecs = &ecs;
        scriptSystem->OnInit();
        ecs.SetSystemSignature<ScriptSystem, ScriptComponent>();

        // Input System
        inputSystem = ecs.RegisterSystem<InputSystem>();
        inputSystem->ecs = &ecs;
        ecs.SetSystemSignature<InputSystem, InputReceiver, Velocity>();

        // Physics System
        physicsSystem = ecs.RegisterSystem<PhysicsSystem>();
        physicsSystem->ecs = &ecs;
        ecs.SetSystemSignature<PhysicsSystem, ZeroTransform, Velocity>();

        // Camera System
        cameraSystem = ecs.RegisterSystem<CameraSystem>();
        cameraSystem->ecs = &ecs;
        cameraSystem->renderer = &renderer;
        ecs.SetSystemSignature<CameraSystem, CameraFollow>();

        // Render System
        renderSystem = ecs.RegisterSystem<RenderSystem>();
        renderSystem->ecs = &ecs;
        renderSystem->renderer = &renderer;
        ecs.SetSystemSignature<RenderSystem, MyTransform, MeshRendererComponent>();

        // Lifetime System
        lifetimeSystem = ecs.RegisterSystem<LifetimeSystem>();
        lifetimeSystem->ecs = &ecs;
        ecs.SetSystemSignature<LifetimeSystem, Lifetime>();

        // Rotation System (optional)
        rotationSystem = ecs.RegisterSystem<RotationSystem>();
        rotationSystem->ecs = &ecs;
        ecs.SetSystemSignature<RotationSystem, MyTransform>();

        // Debug System
        debugSystem = ecs.RegisterSystem<DebugSystem>();
        debugSystem->ecs = &ecs;
        ecs.SetSystemSignature<DebugSystem>();
    }

    // Factory methods for common entities
    
    Entity CreatePlayerCube(Vector3 position = {0,0,0}, float size = 1.0f, bool giveInput = true) {
        Entity e = ecs.CreateEntity();

        MyTransform t;
        t.position = position;
        t.scale = {size, size, size};
        ecs.AddComponent(e, t);

        MeshRendererComponent mr;
        mr.mesh = sharedCube;
        mr.shader = defaultShader;
        mr.isValid = (sharedCube != nullptr);
        mr.tint = BLUE;
        ecs.AddComponent(e, mr);

        Velocity vel;
        ecs.AddComponent(e, vel);

        if (giveInput) {
            InputReceiver input;
            input.speed = 50.0f;
            ecs.AddComponent(e, input);
        }

        Tag tag;
        tag.name = "Player";
        ecs.AddComponent(e, tag);

        LOG_INFO("Created player cube entity: " + std::to_string(e));
        return e;
    }

    // NEW: Create scripted entity
    Entity CreateScriptedCube(Vector3 position, const std::string& scriptPath, 
                              const std::string& className, Color color = WHITE) {
        Entity e = ecs.CreateEntity();

        MyTransform t;
        t.position = position;
        t.scale = {1, 1, 1};
        ecs.AddComponent(e, t);

        MeshRendererComponent mr;
        mr.mesh = sharedCube;
        mr.shader = defaultShader;
        mr.isValid = (sharedCube != nullptr);
        mr.tint = color;
        ecs.AddComponent(e, mr);

        Velocity vel;
        ecs.AddComponent(e, vel);

        ScriptComponent script;
        script.scriptPath = scriptPath;
        script.className = className;
        ecs.AddComponent(e, script);

        Tag tag;
        tag.name = className;
        ecs.AddComponent(e, tag);

        LOG_INFO("Created scripted entity: " + className + " at (" + 
                 std::to_string(position.x) + ", " + 
                 std::to_string(position.y) + ", " + 
                 std::to_string(position.z) + ")");
        return e;
    }

    // NEW: Attach script to existing entity
    void AttachScript(Entity entity, const std::string& scriptPath, const std::string& className) {
        if (entity == INVALID_ENTITY) {
            LOG_ERROR("Cannot attach script to invalid entity");
            return;
        }

        ScriptComponent script;
        script.scriptPath = scriptPath;
        script.className = className;
        ecs.AddComponent(entity, script);

        LOG_INFO("Attached script " + className + " to entity " + std::to_string(entity));
    }

    Entity CreateCube(Vector3 position, Vector3 scale = {1,1,1}, Color color = WHITE) {
        Entity e = ecs.CreateEntity();

        MyTransform t;
        t.position = position;
        t.scale = scale;
        ecs.AddComponent(e, t);

        MeshRendererComponent mr;
        mr.mesh = sharedCube;
        mr.shader = defaultShader;
        mr.isValid = (sharedCube != nullptr);
        mr.tint = color;
        ecs.AddComponent(e, mr);

        Tag tag;
        tag.name = "Cube";
        ecs.AddComponent(e, tag);

        return e;
    }

    Entity CreateCameraEntity(Entity target, Vector3 offset = {0,5,10}) {
        Entity e = ecs.CreateEntity();
        
        CameraFollow cf;
        cf.target = target;
        cf.offset = offset;
        cf.smoothing = 5.0f;
        ecs.AddComponent(e, cf);

        Tag tag;
        tag.name = "Camera";
        ecs.AddComponent(e, tag);

        LOG_INFO("Created camera entity: " + std::to_string(e) + " following entity: " + std::to_string(target));
        return e;
    }

    // Scene management
    bool SaveScene(const std::string& filepath) {
        LOG_INFO("Saving scene to: " + filepath);
        return SceneSerializer::SaveScene(filepath, ecs);
    }

    bool LoadScene(const std::string& filepath) {
        LOG_INFO("Loading scene from: " + filepath);
        return SceneSerializer::LoadScene(filepath, ecs, resources);
    }

    // Update loop
    void Update(float dt) {
        if (isPaused) return;

        // System update order matters!
        scriptSystem->Update(dt);      // NEW: Scripts run first
        inputSystem->Update(dt);
        physicsSystem->Update(dt);
        lifetimeSystem->Update(dt);
        cameraSystem->Update(dt);
    }

    void Render() {
        renderSystem->Update(0.0f);
        debugSystem->Update(0.0f);
    }

    void HandleInput() {
        // Save/Load with F5/F9
        if (IsKeyPressed(KEY_F5)) {
            SaveScene("autosave.json");
        }
        if (IsKeyPressed(KEY_F9)) {
            LoadScene("autosave.json");
        }

        // Pause with P
        if (IsKeyPressed(KEY_P)) {
            isPaused = !isPaused;
            LOG_INFO(isPaused ? "Game paused" : "Game resumed");
        }

        // Quit with ESC
        if (IsKeyPressed(KEY_ESCAPE)) {
            LOG_INFO("ESC pressed - quitting");
        }
    }

    void Shutdown() {
        LOG_INFO("Shutting down world...");
        scriptSystem->OnShutdown();
        resources.Shutdown();
        renderer.Shutdown();
        LOG_INFO("World shutdown complete");
    }
};
