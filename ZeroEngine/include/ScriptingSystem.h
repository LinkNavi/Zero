k#pragma once
#include "ScriptComponent.h"
#include "ECS.h"
#include "Systems.h"
#include "Logger.h"
#include <wren.hpp>
#include <fstream>
#include <sstream>
#include <cmath>

// Wren scripting manager
class ScriptingEngine {
public:
    static ScriptingEngine& Instance() {
        static ScriptingEngine instance;
        return instance;
    }

    bool Init() {
        LOG_INFO("Initializing Wren scripting engine...");
        
        WrenConfiguration config;
        wrenInitConfiguration(&config);
        
        config.writeFn = &ScriptingEngine::WrenWrite;
        config.errorFn = &ScriptingEngine::WrenError;
        config.bindForeignMethodFn = &ScriptingEngine::WrenBindForeignMethod;
        config.bindForeignClassFn = &ScriptingEngine::WrenBindForeignClass;
        config.loadModuleFn = &ScriptingEngine::WrenLoadModule;

        vm_ = wrenNewVM(&config);
        
        if (!vm_) {
            LOG_ERROR("Failed to create Wren VM");
            return false;
        }

        // Register built-in API
        RegisterAPI();

        LOG_INFO("Wren scripting engine initialized");
        return true;
    }

    void Shutdown() {
        if (vm_) {
            wrenFreeVM(vm_);
            vm_ = nullptr;
        }
        LOG_INFO("Wren scripting engine shutdown");
    }

    WrenVM* GetVM() { return vm_; }

    // Load and compile a script file
    bool LoadScript(const std::string& path, const std::string& moduleName = "main") {
        std::ifstream file(path);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open script file: " + path);
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();

        WrenInterpretResult result = wrenInterpret(vm_, moduleName.c_str(), source.c_str());
        
        if (result != WREN_RESULT_SUCCESS) {
            LOG_ERROR("Failed to load script: " + path);
            return false;
        }

        LOG_DEBUG("Loaded script: " + path);
        return true;
    }

    // Create an instance of a Wren class
    std::shared_ptr<ScriptInstance> CreateInstance(const std::string& moduleName, 
                                                     const std::string& className) {
        wrenEnsureSlots(vm_, 1);
        wrenGetVariable(vm_, moduleName.c_str(), className.c_str(), 0);
        
        WrenHandle* classHandle = wrenGetSlotHandle(vm_, 0);
        
        if (wrenGetSlotType(vm_, 0) == WREN_TYPE_NULL) {
            LOG_ERROR("Class not found: " + className + " in module: " + moduleName);
            wrenReleaseHandle(vm_, classHandle);
            return nullptr;
        }
        
        // Call the constructor
        wrenSetSlotHandle(vm_, 0, classHandle);
        WrenHandle* ctorHandle = wrenMakeCallHandle(vm_, "new()");
        WrenInterpretResult result = wrenCall(vm_, ctorHandle);
        wrenReleaseHandle(vm_, ctorHandle);
        
        if (result != WREN_RESULT_SUCCESS) {
            LOG_ERROR("Failed to instantiate class: " + className);
            wrenReleaseHandle(vm_, classHandle);
            return nullptr;
        }
        
        WrenHandle* instanceHandle = wrenGetSlotHandle(vm_, 0);
        
        return std::make_shared<ScriptInstance>(vm_, classHandle, instanceHandle);
    }

    // Set current entity context (so scripts can access it)
    void SetCurrentEntity(Entity entity, ECS* ecs) {
        currentEntity_ = entity;
        currentECS_ = ecs;
    }

    Entity GetCurrentEntity() const { return currentEntity_; }
    ECS* GetCurrentECS() { return currentECS_; }

private:
    ScriptingEngine() = default;
    ~ScriptingEngine() { Shutdown(); }

    WrenVM* vm_ = nullptr;
    Entity currentEntity_ = INVALID_ENTITY;
    ECS* currentECS_ = nullptr;

    // Wren callbacks
    static void WrenWrite(WrenVM* vm, const char* text) {
        (void)vm;
        LOG_INFO("[Wren] " + std::string(text));
    }

    static void WrenError(WrenVM* vm, WrenErrorType type, const char* module,
                         int line, const char* message) {
        (void)vm;
        std::string error = std::string(module) + ":" + std::to_string(line) + " - " + message;
        
        switch (type) {
            case WREN_ERROR_COMPILE:
                LOG_ERROR("[Wren Compile] " + error);
                break;
            case WREN_ERROR_RUNTIME:
                LOG_ERROR("[Wren Runtime] " + error);
                break;
            case WREN_ERROR_STACK_TRACE:
                LOG_ERROR("[Wren Stack] " + error);
                break;
        }
    }

    static WrenLoadModuleResult WrenLoadModule(WrenVM* vm, const char* name) {
        (void)vm;
        WrenLoadModuleResult result = {0};
        
        std::string filename = std::string("scripts/") + name + ".wren";
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            LOG_ERROR("Failed to load module: " + std::string(name));
            return result;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string* source = new std::string(buffer.str());
        
        result.source = source->c_str();
        result.onComplete = [](WrenVM* vm, const char* name, WrenLoadModuleResult result) {
            (void)vm;
            (void)name;
            delete static_cast<const std::string*>(static_cast<const void*>(result.source));
        };
        
        return result;
    }

    static WrenForeignMethodFn WrenBindForeignMethod(WrenVM* vm, const char* module,
                                                      const char* className, bool isStatic,
                                                      const char* signature) {
        (void)vm;
        
        std::string mod(module);
        std::string cls(className);
        std::string sig(signature);

        // Entity API
        if (cls == "Entity") {
            if (sig == "getPosition()") return &API_GetPosition;
            if (sig == "setPosition(_,_,_)") return &API_SetPosition;
            if (sig == "getVelocity()") return &API_GetVelocity;
            if (sig == "setVelocity(_,_,_)") return &API_SetVelocity;
            if (sig == "getRotation()") return &API_GetRotation;
            if (sig == "setRotation(_,_,_)") return &API_SetRotation;
            if (sig == "getScale()") return &API_GetScale;
            if (sig == "setScale(_,_,_)") return &API_SetScale;
            if (sig == "destroy()") return &API_DestroyEntity;
            if (sig == "getId()") return &API_GetEntityId;
        }

        // Input API
        if (cls == "Input") {
            if (sig == "isKeyDown(_)") return &API_IsKeyDown;
            if (sig == "isKeyPressed(_)") return &API_IsKeyPressed;
            if (sig == "isKeyReleased(_)") return &API_IsKeyReleased;
            if (sig == "isMouseButtonDown(_)") return &API_IsMouseButtonDown;
            if (sig == "isMouseButtonPressed(_)") return &API_IsMouseButtonPressed;
            if (sig == "getMousePosition()") return &API_GetMousePosition;
            if (sig == "getMouseDelta()") return &API_GetMouseDelta;
        }

        // Time API
        if (cls == "Time") {
            if (sig == "getDeltaTime()") return &API_GetDeltaTime;
            if (sig == "getTime()") return &API_GetTime;
            if (sig == "getFPS()") return &API_GetFPS;
        }

        // Math API
        if (cls == "Math") {
            if (!isStatic) return nullptr;
            if (sig == "sin(_)") return &API_Sin;
            if (sig == "cos(_)") return &API_Cos;
            if (sig == "tan(_)") return &API_Tan;
            if (sig == "sqrt(_)") return &API_Sqrt;
            if (sig == "abs(_)") return &API_Abs;
            if (sig == "pow(_,_)") return &API_Pow;
            if (sig == "lerp(_,_,_)") return &API_Lerp;
            if (sig == "clamp(_,_,_)") return &API_Clamp;
            if (sig == "radians(_)") return &API_Radians;
            if (sig == "degrees(_)") return &API_Degrees;
        }

        // Debug API
        if (cls == "Debug") {
            if (!isStatic) return nullptr;
            if (sig == "log(_)") return &API_DebugLog;
            if (sig == "drawLine(_,_,_,_,_,_)") return &API_DebugDrawLine;
            if (sig == "drawSphere(_,_,_,_)") return &API_DebugDrawSphere;
        }

        return nullptr;
    }

    static WrenForeignClassMethods WrenBindForeignClass(WrenVM* vm, const char* module,
                                                         const char* className) {
        (void)vm;
        (void)module;
        (void)className;
        
        WrenForeignClassMethods methods = {0};
        return methods;
    }

    // Register the built-in API
    void RegisterAPI() {
        const char* apiSource = R"(
            // Entity manipulation
            foreign class Entity {
                foreign getPosition()
                foreign setPosition(x, y, z)
                foreign getVelocity()
                foreign setVelocity(x, y, z)
                foreign getRotation()
                foreign setRotation(x, y, z)
                foreign getScale()
                foreign setScale(x, y, z)
                foreign destroy()
                foreign getId()
            }

            // Input handling
            foreign class Input {
                foreign static isKeyDown(key)
                foreign static isKeyPressed(key)
                foreign static isKeyReleased(key)
                foreign static isMouseButtonDown(button)
                foreign static isMouseButtonPressed(button)
                foreign static getMousePosition()
                foreign static getMouseDelta()
            }

            // Time utilities
            foreign class Time {
                foreign static getDeltaTime()
                foreign static getTime()
                foreign static getFPS()
            }

            // Math utilities
            foreign class Math {
                foreign static sin(angle)
                foreign static cos(angle)
                foreign static tan(angle)
                foreign static sqrt(value)
                foreign static abs(value)
                foreign static pow(base, exp)
                foreign static lerp(a, b, t)
                foreign static clamp(value, min, max)
                foreign static radians(degrees)
                foreign static degrees(radians)
                
                static pi { 3.14159265359 }
                static tau { 6.28318530718 }
            }

            // Debug utilities
            foreign class Debug {
                foreign static log(message)
                foreign static drawLine(x1, y1, z1, x2, y2, z2)
                foreign static drawSphere(x, y, z, radius)
            }

            // Vector3 helper class
            class Vector3 {
                construct new(x, y, z) {
                    _x = x
                    _y = y
                    _z = z
                }
                
                x { _x }
                y { _y }
                z { _z }
                x=(v) { _x = v }
                y=(v) { _y = v }
                z=(v) { _z = v }
                
                +(other) { Vector3.new(_x + other.x, _y + other.y, _z + other.z) }
                -(other) { Vector3.new(_x - other.x, _y - other.y, _z - other.z) }
                *(scalar) { Vector3.new(_x * scalar, _y * scalar, _z * scalar) }
                /(scalar) { Vector3.new(_x / scalar, _y / scalar, _z / scalar) }
                
                length { Math.sqrt(_x * _x + _y * _y + _z * _z) }
                normalized {
                    var len = length
                    if (len > 0) return this / len
                    return Vector3.new(0, 0, 0)
                }
                
                dot(other) { _x * other.x + _y * other.y + _z * other.z }
                
                static distance(a, b) {
                    var dx = a.x - b.x
                    var dy = a.y - b.y
                    var dz = a.z - b.z
                    return Math.sqrt(dx * dx + dy * dy + dz * dz)
                }
                
                static lerp(a, b, t) {
                    return Vector3.new(
                        Math.lerp(a.x, b.x, t),
                        Math.lerp(a.y, b.y, t),
                        Math.lerp(a.z, b.z, t)
                    )
                }
                
                toString { "Vector3(%(x), %(y), %(z))" }
            }

            // Key codes (matching Raylib)
            class Key {
                static SPACE { 32 }
                static ESCAPE { 256 }
                static ENTER { 257 }
                static TAB { 258 }
                static BACKSPACE { 259 }
                static RIGHT { 262 }
                static LEFT { 263 }
                static DOWN { 264 }
                static UP { 265 }
                static LEFT_SHIFT { 340 }
                static LEFT_CONTROL { 341 }
                static LEFT_ALT { 342 }
                static A { 65 }
                static B { 66 }
                static C { 67 }
                static D { 68 }
                static E { 69 }
                static F { 70 }
                static G { 71 }
                static H { 72 }
                static I { 73 }
                static J { 74 }
                static K { 75 }
                static L { 76 }
                static M { 77 }
                static N { 78 }
                static O { 79 }
                static P { 80 }
                static Q { 81 }
                static R { 82 }
                static S { 83 }
                static T { 84 }
                static U { 85 }
                static V { 86 }
                static W { 87 }
                static X { 88 }
                static Y { 89 }
                static Z { 90 }
            }

            class Mouse {
                static LEFT { 0 }
                static RIGHT { 1 }
                static MIDDLE { 2 }
            }
        )";

        WrenInterpretResult result = wrenInterpret(vm_, "engine", apiSource);
        if (result != WREN_RESULT_SUCCESS) {
            LOG_ERROR("Failed to register Wren API");
        }
    }

    // ============ Foreign Method Implementations ============

    // Entity API
    static void API_GetPosition(WrenVM* vm) {
        ScriptingEngine& engine = ScriptingEngine::Instance();
        Entity entity = engine.GetCurrentEntity();
        ECS* ecs = engine.GetCurrentECS();

        if (!ecs || entity == INVALID_ENTITY || !ecs->HasComponent<ZeroTransform>(entity)) {
            wrenSetSlotNewList(vm, 0);
            return;
        }

        auto& transform = ecs->GetComponent<ZeroTransform>(entity);
        wrenEnsureSlots(vm, 4);
        wrenSetSlotNewList(vm, 0);
        wrenSetSlotDouble(vm, 1, transform.position.x);
        wrenInsertInList(vm, 0, -1, 1);
        wrenSetSlotDouble(vm, 1, transform.position.y);
        wrenInsertInList(vm, 0, -1, 1);
        wrenSetSlotDouble(vm, 1, transform.position.z);
        wrenInsertInList(vm, 0, -1, 1);
    }

    static void API_SetPosition(WrenVM* vm) {
        ScriptingEngine& engine = ScriptingEngine::Instance();
        Entity entity = engine.GetCurrentEntity();
        ECS* ecs = engine.GetCurrentECS();

        if (!ecs || entity == INVALID_ENTITY || !ecs->HasComponent<ZeroTransform>(entity)) return;

        float x = (float)wrenGetSlotDouble(vm, 1);
        float y = (float)wrenGetSlotDouble(vm, 2);
        float z = (float)wrenGetSlotDouble(vm, 3);

        auto& transform = ecs->GetComponent<ZeroTransform>(entity);
        transform.position = {x, y, z};
    }

    static void API_GetVelocity(WrenVM* vm) {
        ScriptingEngine& engine = ScriptingEngine::Instance();
        Entity entity = engine.GetCurrentEntity();
        ECS* ecs = engine.GetCurrentECS();

        if (!ecs || entity == INVALID_ENTITY || !ecs->HasComponent<Velocity>(entity)) {
            wrenSetSlotNewList(vm, 0);
            return;
        }

        auto& vel = ecs->GetComponent<Velocity>(entity);
        wrenEnsureSlots(vm, 4);
        wrenSetSlotNewList(vm, 0);
        wrenSetSlotDouble(vm, 1, vel.v.x);
        wrenInsertInList(vm, 0, -1, 1);
        wrenSetSlotDouble(vm, 1, vel.v.y);
        wrenInsertInList(vm, 0, -1, 1);
        wrenSetSlotDouble(vm, 1, vel.v.z);
        wrenInsertInList(vm, 0, -1, 1);
    }

    static void API_SetVelocity(WrenVM* vm) {
        ScriptingEngine& engine = ScriptingEngine::Instance();
        Entity entity = engine.GetCurrentEntity();
        ECS* ecs = engine.GetCurrentECS();

        if (!ecs || entity == INVALID_ENTITY || !ecs->HasComponent<Velocity>(entity)) return;

        float x = (float)wrenGetSlotDouble(vm, 1);
        float y = (float)wrenGetSlotDouble(vm, 2);
        float z = (float)wrenGetSlotDouble(vm, 3);

        auto& vel = ecs->GetComponent<Velocity>(entity);
        vel.v = {x, y, z};
    }

    static void API_GetRotation(WrenVM* vm) {
        ScriptingEngine& engine = ScriptingEngine::Instance();
        Entity entity = engine.GetCurrentEntity();
        ECS* ecs = engine.GetCurrentECS();

        if (!ecs || entity == INVALID_ENTITY || !ecs->HasComponent<ZeroTransform>(entity)) {
            wrenSetSlotNewList(vm, 0);
            return;
        }

        auto& transform = ecs->GetComponent<ZeroTransform>(entity);
        wrenEnsureSlots(vm, 4);
        wrenSetSlotNewList(vm, 0);
        wrenSetSlotDouble(vm, 1, transform.rotation.x);
        wrenInsertInList(vm, 0, -1, 1);
        wrenSetSlotDouble(vm, 1, transform.rotation.y);
        wrenInsertInList(vm, 0, -1, 1);
        wrenSetSlotDouble(vm, 1, transform.rotation.z);
        wrenInsertInList(vm, 0, -1, 1);
    }

    static void API_SetRotation(WrenVM* vm) {
        ScriptingEngine& engine = ScriptingEngine::Instance();
        Entity entity = engine.GetCurrentEntity();
        ECS* ecs = engine.GetCurrentECS();

        if (!ecs || entity == INVALID_ENTITY || !ecs->HasComponent<ZeroTransform>(entity)) return;

        float x = (float)wrenGetSlotDouble(vm, 1);
        float y = (float)wrenGetSlotDouble(vm, 2);
        float z = (float)wrenGetSlotDouble(vm, 3);

        auto& transform = ecs->GetComponent<ZeroTransform>(entity);
        transform.rotation = {x, y, z};
    }

    static void API_GetScale(WrenVM* vm) {
        ScriptingEngine& engine = ScriptingEngine::Instance();
        Entity entity = engine.GetCurrentEntity();
        ECS* ecs = engine.GetCurrentECS();

        if (!ecs || entity == INVALID_ENTITY || !ecs->HasComponent<ZeroTransform>(entity)) {
            wrenSetSlotNewList(vm, 0);
            return;
        }

        auto& transform = ecs->GetComponent<ZeroTransform>(entity);
        wrenEnsureSlots(vm, 4);
        wrenSetSlotNewList(vm, 0);
        wrenSetSlotDouble(vm, 1, transform.scale.x);
        wrenInsertInList(vm, 0, -1, 1);
        wrenSetSlotDouble(vm, 1, transform.scale.y);
        wrenInsertInList(vm, 0, -1, 1);
        wrenSetSlotDouble(vm, 1, transform.scale.z);
        wrenInsertInList(vm, 0, -1, 1);
    }

    static void API_SetScale(WrenVM* vm) {
        ScriptingEngine& engine = ScriptingEngine::Instance();
        Entity entity = engine.GetCurrentEntity();
        ECS* ecs = engine.GetCurrentECS();

        if (!ecs || entity == INVALID_ENTITY || !ecs->HasComponent<ZeroTransform>(entity)) return;

        float x = (float)wrenGetSlotDouble(vm, 1);
        float y = (float)wrenGetSlotDouble(vm, 2);
        float z = (float)wrenGetSlotDouble(vm, 3);

        auto& transform = ecs->GetComponent<ZeroTransform>(entity);
        transform.scale = {x, y, z};
    }

    static void API_DestroyEntity(WrenVM* vm) {
        (void)vm;
        ScriptingEngine& engine = ScriptingEngine::Instance();
        Entity entity = engine.GetCurrentEntity();
        ECS* ecs = engine.GetCurrentECS();

        if (ecs && entity != INVALID_ENTITY) {
            ecs->DestroyEntity(entity);
        }
    }

    static void API_GetEntityId(WrenVM* vm) {
        ScriptingEngine& engine = ScriptingEngine::Instance();
        Entity entity = engine.GetCurrentEntity();
        wrenSetSlotDouble(vm, 0, (double)entity);
    }

    // Input API
    static void API_IsKeyDown(WrenVM* vm) {
        int key = (int)wrenGetSlotDouble(vm, 1);
        bool isDown = IsKeyDown(key);
        wrenSetSlotBool(vm, 0, isDown);
    }

    static void API_IsKeyPressed(WrenVM* vm) {
        int key = (int)wrenGetSlotDouble(vm, 1);
        bool isPressed = IsKeyPressed(key);
        wrenSetSlotBool(vm, 0, isPressed);
    }

    static void API_IsKeyReleased(WrenVM* vm) {
        int key = (int)wrenGetSlotDouble(vm, 1);
        bool isReleased = IsKeyReleased(key);
        wrenSetSlotBool(vm, 0, isReleased);
    }

    static void API_IsMouseButtonDown(WrenVM* vm) {
        int button = (int)wrenGetSlotDouble(vm, 1);
        bool isDown = IsMouseButtonDown(button);
        wrenSetSlotBool(vm, 0, isDown);
    }

    static void API_IsMouseButtonPressed(WrenVM* vm) {
        int button = (int)wrenGetSlotDouble(vm, 1);
        bool isPressed = IsMouseButtonPressed(button);
        wrenSetSlotBool(vm, 0, isPressed);
    }

    static void API_GetMousePosition(WrenVM* vm) {
        Vector2 pos = GetMousePosition();
        wrenEnsureSlots(vm, 3);
        wrenSetSlotNewList(vm, 0);
        wrenSetSlotDouble(vm, 1, pos.x);
        wrenInsertInList(vm, 0, -1, 1);
        wrenSetSlotDouble(vm, 1, pos.y);
        wrenInsertInList(vm, 0, -1, 1);
    }

    static void API_GetMouseDelta(WrenVM* vm) {
        Vector2 delta = GetMouseDelta();
        wrenEnsureSlots(vm, 3);
        wrenSetSlotNewList(vm, 0);
        wrenSetSlotDouble(vm, 1, delta.x);
        wrenInsertInList(vm, 0, -1, 1);
        wrenSetSlotDouble(vm, 1, delta.y);
        wrenInsertInList(vm, 0, -1, 1);
    }

    // Time API
    static void API_GetDeltaTime(WrenVM* vm) {
        float dt = GetFrameTime();
        wrenSetSlotDouble(vm, 0, dt);
    }

    static void API_GetTime(WrenVM* vm) {
        double time = GetTime();
        wrenSetSlotDouble(vm, 0, time);
    }

    static void API_GetFPS(WrenVM* vm) {
        int fps = GetFPS();
        wrenSetSlotDouble(vm, 0, fps);
    }

    // Math API
    static void API_Sin(WrenVM* vm) {
        double angle = wrenGetSlotDouble(vm, 1);
        wrenSetSlotDouble(vm, 0, sin(angle));
    }

    static void API_Cos(WrenVM* vm) {
        double angle = wrenGetSlotDouble(vm, 1);
        wrenSetSlotDouble(vm, 0, cos(angle));
    }

    static void API_Tan(WrenVM* vm) {
        double angle = wrenGetSlotDouble(vm, 1);
        wrenSetSlotDouble(vm, 0, tan(angle));
    }

    static void API_Sqrt(WrenVM* vm) {
        double value = wrenGetSlotDouble(vm, 1);
        wrenSetSlotDouble(vm, 0, sqrt(value));
    }

    static void API_Abs(WrenVM* vm) {
        double value = wrenGetSlotDouble(vm, 1);
        wrenSetSlotDouble(vm, 0, fabs(value));
    }

    static void API_Pow(WrenVM* vm) {
        double base = wrenGetSlotDouble(vm, 1);
        double exp = wrenGetSlotDouble(vm, 2);
        wrenSetSlotDouble(vm, 0, pow(base, exp));
    }

    static void API_Lerp(WrenVM* vm) {
        double a = wrenGetSlotDouble(vm, 1);
        double b = wrenGetSlotDouble(vm, 2);
        double t = wrenGetSlotDouble(vm, 3);
        wrenSetSlotDouble(vm, 0, a + (b - a) * t);
    }

    static void API_Clamp(WrenVM* vm) {
        double value = wrenGetSlotDouble(vm, 1);
        double min = wrenGetSlotDouble(vm, 2);
        double max = wrenGetSlotDouble(vm, 3);
        double result = value < min ? min : (value > max ? max : value);
        wrenSetSlotDouble(vm, 0, result);
    }

    static void API_Radians(WrenVM* vm) {
        double degrees = wrenGetSlotDouble(vm, 1);
        wrenSetSlotDouble(vm, 0, degrees * 0.017453292519943295);
    }

    static void API_Degrees(WrenVM* vm) {
        double radians = wrenGetSlotDouble(vm, 1);
        wrenSetSlotDouble(vm, 0, radians * 57.29577951308232);
    }

    // Debug API
    static void API_DebugLog(WrenVM* vm) {
        const char* message = wrenGetSlotString(vm, 1);
        LOG_INFO("[Script] " + std::string(message));
    }

    static void API_DebugDrawLine(WrenVM* vm) {
        float x1 = (float)wrenGetSlotDouble(vm, 1);
        float y1 = (float)wrenGetSlotDouble(vm, 2);
        float z1 = (float)wrenGetSlotDouble(vm, 3);
        float x2 = (float)wrenGetSlotDouble(vm, 4);
        float y2 = (float)wrenGetSlotDouble(vm, 5);
        float z2 = (float)wrenGetSlotDouble(vm, 6);
        DrawLine3D({x1, y1, z1}, {x2, y2, z2}, RED);
    }

    static void API_DebugDrawSphere(WrenVM* vm) {
        float x = (float)wrenGetSlotDouble(vm, 1);
        float y = (float)wrenGetSlotDouble(vm, 2);
        float z = (float)wrenGetSlotDouble(vm, 3);
        float radius = (float)wrenGetSlotDouble(vm, 4);
        DrawSphereWires({x, y, z}, radius, 8, 8, YELLOW);
    }
};

// Script System - Updates all script components
class ScriptSystem : public System {
public:
    ECS* ecs = nullptr;
    bool initialized = false;

    void OnInit() override {
        if (!ScriptingEngine::Instance().Init()) {
            LOG_ERROR("Failed to initialize scripting engine");
            return;
        }
        initialized = true;
        LOG_INFO("ScriptSystem initialized");
    }

    void Update(float dt) override {
        if (!initialized || !ecs) return;

        for (auto entity : mEntities) {
            if (!ecs->HasComponent<ScriptComponent>(entity)) continue;

            auto& script = ecs->GetComponent<ScriptComponent>(entity);
            
            if (!script.enabled) continue;

            // Initialize script if needed
            if (!script.isInitialized) {
                InitializeScript(entity, script);
            }

            // Call update method
            if (script.instance && script.isInitialized) {
                UpdateScript(entity, script, dt);
            }
        }
    }

    void OnShutdown() override {
        ScriptingEngine::Instance().Shutdown();
    }

private:
    void InitializeScript(Entity entity, ScriptComponent& script) {
        LOG_DEBUG("Initializing script for entity " + std::to_string(entity) + ": " + script.scriptPath);
        
        // Load the script file
        if (!ScriptingEngine::Instance().LoadScript(script.scriptPath)) {
            LOG_ERROR("Failed to load script: " + script.scriptPath);
            return;
        }

        // Create an instance of the class
        script.instance = ScriptingEngine::Instance().CreateInstance("main", script.className);
        
        if (!script.instance) {
            LOG_ERROR("Failed to create script instance: " + script.className);
            return;
        }

        script.isInitialized = true;

        // Call start() method if it exists
        ScriptingEngine::Instance().SetCurrentEntity(entity, ecs);
        CallScriptMethod(script, ScriptMethods::START);
        
        LOG_DEBUG("Successfully initialized script: " + script.className + " for entity " + std::to_string(entity));
    }

    void UpdateScript(Entity entity, ScriptComponent& script, float dt) {
        ScriptingEngine::Instance().SetCurrentEntity(entity, ecs);
        
        WrenVM* vm = script.instance->GetVM();
        WrenHandle* instance = script.instance->GetInstanceHandle();

        if (!vm || !instance) return;

        // Call update(dt) method
        wrenEnsureSlots(vm, 2);
        wrenSetSlotHandle(vm, 0, instance);
        wrenSetSlotDouble(vm, 1, dt);
        
        WrenHandle* updateMethod = wrenMakeCallHandle(vm, ScriptMethods::UPDATE);
        WrenInterpretResult result = wrenCall(vm, updateMethod);
        wrenReleaseHandle(vm, updateMethod);

        if (result != WREN_RESULT_SUCCESS) {
            LOG_WARN("Script update failed for entity " + std::to_string(entity));
        }
    }

    void CallScriptMethod(ScriptComponent& script, const char* signature) {
        if (!script.instance) return;

        WrenVM* vm = script.instance->GetVM();
        WrenHandle* instance = script.instance->GetInstanceHandle();

        if (!vm || !instance) return;

        wrenEnsureSlots(vm, 1);
        wrenSetSlotHandle(vm, 0, instance);
        
        WrenHandle* method = wrenMakeCallHandle(vm, signature);
        WrenInterpretResult result = wrenCall(vm, method);
        wrenReleaseHandle(vm, method);

        if (result != WREN_RESULT_SUCCESS) {
            LOG_DEBUG("Method " + std::string(signature) + " not found or failed (this is OK if method doesn't exist)");
        }
    }
};
