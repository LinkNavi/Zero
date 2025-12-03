#pragma once
#include "ScriptComponent.h"
#include "ECS.h"
#include "Systems.h"
#include "Logger.h"
#include <wren.hpp>
#include <fstream>
#include <sstream>

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
        
        // Call the constructor (assuming it takes no arguments)
        wrenSetSlotHandle(vm_, 0, classHandle);
        wrenCall(vm_, wrenMakeCallHandle(vm_, "new()"));
        
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
        (void)isStatic;
        
        std::string mod(module);
        std::string cls(className);
        std::string sig(signature);

        // Bind Entity API methods
        if (cls == "Entity") {
            if (sig == "getPosition()") return &API_GetPosition;
            if (sig == "setPosition(_,_,_)") return &API_SetPosition;
            if (sig == "getVelocity()") return &API_GetVelocity;
            if (sig == "setVelocity(_,_,_)") return &API_SetVelocity;
            if (sig == "destroy()") return &API_DestroyEntity;
        }

        // Bind Input API methods
        if (cls == "Input") {
            if (sig == "isKeyDown(_)") return &API_IsKeyDown;
            if (sig == "isKeyPressed(_)") return &API_IsKeyPressed;
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
            foreign class Entity {
                foreign getPosition()
                foreign setPosition(x, y, z)
                foreign getVelocity()
                foreign setVelocity(x, y, z)
                foreign destroy()
            }

            foreign class Input {
                foreign static isKeyDown(key)
                foreign static isKeyPressed(key)
            }

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
            }
        )";

        WrenInterpretResult result = wrenInterpret(vm_, "engine", apiSource);
        if (result != WREN_RESULT_SUCCESS) {
            LOG_ERROR("Failed to register Wren API");
        }
    }

    // Foreign method implementations
    static void API_GetPosition(WrenVM* vm) {
        ScriptingEngine& engine = ScriptingEngine::Instance();
        Entity entity = engine.GetCurrentEntity();
        ECS* ecs = engine.GetCurrentECS();

        if (!ecs || entity == INVALID_ENTITY) return;

        if (!ecs->HasComponent<ZeroTransform>(entity)) {
            wrenSetSlotNull(vm, 0);
            return;
        }

        auto& transform = ecs->GetComponent<ZeroTransform>(entity);
        
        wrenEnsureSlots(vm, 4);
        wrenSetSlotDouble(vm, 0, transform.position.x);
        wrenSetSlotDouble(vm, 1, transform.position.y);
        wrenSetSlotDouble(vm, 2, transform.position.z);
    }

    static void API_SetPosition(WrenVM* vm) {
        ScriptingEngine& engine = ScriptingEngine::Instance();
        Entity entity = engine.GetCurrentEntity();
        ECS* ecs = engine.GetCurrentECS();

        if (!ecs || entity == INVALID_ENTITY) return;
        if (!ecs->HasComponent<ZeroTransform>(entity)) return;

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

        if (!ecs || entity == INVALID_ENTITY) return;

        if (!ecs->HasComponent<Velocity>(entity)) {
            wrenSetSlotNull(vm, 0);
            return;
        }

        auto& vel = ecs->GetComponent<Velocity>(entity);
        
        wrenEnsureSlots(vm, 4);
        wrenSetSlotDouble(vm, 0, vel.v.x);
        wrenSetSlotDouble(vm, 1, vel.v.y);
        wrenSetSlotDouble(vm, 2, vel.v.z);
    }

    static void API_SetVelocity(WrenVM* vm) {
        ScriptingEngine& engine = ScriptingEngine::Instance();
        Entity entity = engine.GetCurrentEntity();
        ECS* ecs = engine.GetCurrentECS();

        if (!ecs || entity == INVALID_ENTITY) return;
        if (!ecs->HasComponent<Velocity>(entity)) return;

        float x = (float)wrenGetSlotDouble(vm, 1);
        float y = (float)wrenGetSlotDouble(vm, 2);
        float z = (float)wrenGetSlotDouble(vm, 3);

        auto& vel = ecs->GetComponent<Velocity>(entity);
        vel.v = {x, y, z};
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
            if (script.instance) {
                UpdateScript(entity, script, dt);
            }
        }
    }

    void OnShutdown() override {
        ScriptingEngine::Instance().Shutdown();
    }

private:
    void InitializeScript(Entity entity, ScriptComponent& script) {
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
        
        LOG_DEBUG("Initialized script for entity " + std::to_string(entity));
    }

    void UpdateScript(Entity entity, ScriptComponent& script, float dt) {
        ScriptingEngine::Instance().SetCurrentEntity(entity, ecs);
        
        WrenVM* vm = script.instance->GetVM();
        WrenHandle* instance = script.instance->GetInstanceHandle();

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

        wrenEnsureSlots(vm, 1);
        wrenSetSlotHandle(vm, 0, instance);
        
        WrenHandle* method = wrenMakeCallHandle(vm, signature);
        wrenCall(vm, method);
        wrenReleaseHandle(vm, method);
    }
};
