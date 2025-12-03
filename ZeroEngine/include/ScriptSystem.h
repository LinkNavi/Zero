#pragma once
#include "ScriptComponent.h"
#include "ECS.h"
#include "Systems.h"
#include "Logger.h"
#include <wren.h>
#include <fstream>
#include <sstream>
#include <cmath>

// Forward declarations
class ScriptingEngine;

// Script System - Updates all script components
class ScriptSystem : public System {
public:
    ECS* ecs = nullptr;
    bool initialized = false;

    void OnInit() override;
    void Update(float dt) override;
    void OnShutdown() override;

private:
    void InitializeScript(Entity entity, ScriptComponent& script);
    void UpdateScript(Entity entity, ScriptComponent& script, float dt);
    void CallScriptMethod(ScriptComponent& script, const char* signature);
};

// Wren scripting manager
class ScriptingEngine {
public:
    static ScriptingEngine& Instance() {
        static ScriptingEngine instance;
        return instance;
    }

    bool Init();
    void Shutdown();
    WrenVM* GetVM() { return vm_; }

    // Load and compile a script file
    bool LoadScript(const std::string& path, const std::string& moduleName = "main");

    // Create an instance of a Wren class
    std::shared_ptr<ScriptInstance> CreateInstance(const std::string& moduleName, 
                                                     const std::string& className);

    // Set current entity context (so scripts can access it)
    void SetCurrentEntity(Entity entity, ECS* ecs);
    Entity GetCurrentEntity() const { return currentEntity_; }
    ECS* GetCurrentECS() { return currentECS_; }

private:
    ScriptingEngine() = default;
    ~ScriptingEngine() { Shutdown(); }
    ScriptingEngine(const ScriptingEngine&) = delete;
    ScriptingEngine& operator=(const ScriptingEngine&) = delete;

    WrenVM* vm_ = nullptr;
    Entity currentEntity_ = INVALID_ENTITY;
    ECS* currentECS_ = nullptr;

    // Register the built-in API
    void RegisterAPI();

    // Wren callbacks
    static void WrenWrite(WrenVM* vm, const char* text);
    static void WrenError(WrenVM* vm, WrenErrorType type, const char* module,
                         int line, const char* message);
    static WrenLoadModuleResult WrenLoadModule(WrenVM* vm, const char* name);
    static WrenForeignMethodFn WrenBindForeignMethod(WrenVM* vm, const char* module,
                                                      const char* className, bool isStatic,
                                                      const char* signature);
    static WrenForeignClassMethods WrenBindForeignClass(WrenVM* vm, const char* module,
                                                         const char* className);

    // ============ Foreign Method Implementations ============

    // Entity API
    static void API_GetPosition(WrenVM* vm);
    static void API_SetPosition(WrenVM* vm);
    static void API_GetVelocity(WrenVM* vm);
    static void API_SetVelocity(WrenVM* vm);
    static void API_GetRotation(WrenVM* vm);
    static void API_SetRotation(WrenVM* vm);
    static void API_GetScale(WrenVM* vm);
    static void API_SetScale(WrenVM* vm);
    static void API_DestroyEntity(WrenVM* vm);
    static void API_GetEntityId(WrenVM* vm);

    // Input API
    static void API_IsKeyDown(WrenVM* vm);
    static void API_IsKeyPressed(WrenVM* vm);
    static void API_IsKeyReleased(WrenVM* vm);
    static void API_IsMouseButtonDown(WrenVM* vm);
    static void API_IsMouseButtonPressed(WrenVM* vm);
    static void API_GetMousePosition(WrenVM* vm);
    static void API_GetMouseDelta(WrenVM* vm);

    // Time API
    static void API_GetDeltaTime(WrenVM* vm);
    static void API_GetTime(WrenVM* vm);
    static void API_GetFPS(WrenVM* vm);

    // Math API
    static void API_Sin(WrenVM* vm);
    static void API_Cos(WrenVM* vm);
    static void API_Tan(WrenVM* vm);
    static void API_Sqrt(WrenVM* vm);
    static void API_Abs(WrenVM* vm);
    static void API_Pow(WrenVM* vm);
    static void API_Lerp(WrenVM* vm);
    static void API_Clamp(WrenVM* vm);
    static void API_Radians(WrenVM* vm);
    static void API_Degrees(WrenVM* vm);

    // Debug API
    static void API_DebugLog(WrenVM* vm);
    static void API_DebugDrawLine(WrenVM* vm);
    static void API_DebugDrawSphere(WrenVM* vm);
};
