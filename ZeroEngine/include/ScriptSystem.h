#pragma once
#include "ScriptComponent.h"
#include "ECS.h"
#include "Systems.h"
#include "Logger.h"
#include <angelscript.h>

#include "angelscript_addons/scriptarray.h"
#include "angelscript_addons/scriptbuilder.h"
#include "angelscript_addons/scriptmath.h"
#include "angelscript_addons/scriptstdstring.h"



#include <fstream>
#include <sstream>

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
};

// AngelScript scripting manager
class ScriptingEngine {
public:
    static ScriptingEngine& Instance() {
        static ScriptingEngine instance;
        return instance;
    }

    bool Init();
    void Shutdown();
    asIScriptEngine* GetEngine() { return engine_; }

    // Load and compile a script file
    bool LoadScript(const std::string& path, const std::string& moduleName = "main");

    // Create an instance of an AngelScript class
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

    asIScriptEngine* engine_ = nullptr;
    Entity currentEntity_ = INVALID_ENTITY;
    ECS* currentECS_ = nullptr;

    // Register the built-in API
    void RegisterAPI();

    // AngelScript callbacks
    static void MessageCallback(const asSMessageInfo* msg, void* param);

    // ============ API Wrapper Functions ============
    
    // Entity API
    static void API_GetEntity(asIScriptGeneric* gen);
    static CScriptArray* API_Entity_GetPosition();
    static void API_Entity_SetPosition(float x, float y, float z);
    static CScriptArray* API_Entity_GetVelocity();
    static void API_Entity_SetVelocity(float x, float y, float z);
    static CScriptArray* API_Entity_GetRotation();
    static void API_Entity_SetRotation(float x, float y, float z);
    static CScriptArray* API_Entity_GetScale();
    static void API_Entity_SetScale(float x, float y, float z);
    static void API_Entity_Destroy();
    static int API_Entity_GetId();

    // Input API
    static bool API_Input_IsKeyDown(int key);
    static bool API_Input_IsKeyPressed(int key);
    static bool API_Input_IsKeyReleased(int key);
    static bool API_Input_IsMouseButtonDown(int button);
    static bool API_Input_IsMouseButtonPressed(int button);
    static CScriptArray* API_Input_GetMousePosition();
    static CScriptArray* API_Input_GetMouseDelta();

    // Time API
    static float API_Time_GetDeltaTime();
    static double API_Time_GetTime();
    static int API_Time_GetFPS();

    // Math API (using built-in AngelScript math functions)
    // sin, cos, tan, sqrt, abs, pow are already built-in

    // Debug API
    static void API_Debug_Log(const std::string& message);
    static void API_Debug_DrawLine(float x1, float y1, float z1, float x2, float y2, float z2);
    static void API_Debug_DrawSphere(float x, float y, float z, float radius);

    // Vector3 helper
    struct Vector3Wrapper {
        float x, y, z;
        Vector3Wrapper() : x(0), y(0), z(0) {}
        Vector3Wrapper(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
        
        Vector3Wrapper operator+(const Vector3Wrapper& other) const {
            return Vector3Wrapper(x + other.x, y + other.y, z + other.z);
        }
        Vector3Wrapper operator-(const Vector3Wrapper& other) const {
            return Vector3Wrapper(x - other.x, y - other.y, z - other.z);
        }
        Vector3Wrapper operator*(float scalar) const {
            return Vector3Wrapper(x * scalar, y * scalar, z * scalar);
        }
        Vector3Wrapper operator/(float scalar) const {
            return Vector3Wrapper(x / scalar, y / scalar, z / scalar);
        }
        
        float Length() const {
            return sqrtf(x * x + y * y + z * z);
        }
        
        Vector3Wrapper Normalized() const {
            float len = Length();
            if (len > 0.0001f) return *this / len;
            return Vector3Wrapper(0, 0, 0);
        }
        
        float Dot(const Vector3Wrapper& other) const {
            return x * other.x + y * other.y + z * other.z;
        }
        
        static float Distance(const Vector3Wrapper& a, const Vector3Wrapper& b) {
            return (a - b).Length();
        }
    };

    static void RegisterVector3(asIScriptEngine* engine);
};
