#include "ScriptSystem.h"
#include <cmath>
#include <raylib.h>

// ============================================================================
// ScriptInstance Implementation
// ============================================================================

bool ScriptInstance::CallStart() {
    if (!startMethod_) return true; // Method doesn't exist, not an error
    
    asIScriptContext* ctx = engine_->CreateContext();
    ctx->Prepare(startMethod_);
    ctx->SetObject(scriptObject_);
    
    int r = ctx->Execute();
    ctx->Release();
    
    return r == asEXECUTION_FINISHED;
}

bool ScriptInstance::CallUpdate(float dt) {
    if (!updateMethod_) return true;
    
    asIScriptContext* ctx = engine_->CreateContext();
    ctx->Prepare(updateMethod_);
    ctx->SetObject(scriptObject_);
    ctx->SetArgFloat(0, dt);
    
    int r = ctx->Execute();
    ctx->Release();
    
    return r == asEXECUTION_FINISHED;
}

bool ScriptInstance::CallOnDestroy() {
    if (!onDestroyMethod_) return true;
    
    asIScriptContext* ctx = engine_->CreateContext();
    ctx->Prepare(onDestroyMethod_);
    ctx->SetObject(scriptObject_);
    
    int r = ctx->Execute();
    ctx->Release();
    
    return r == asEXECUTION_FINISHED;
}

// ============================================================================
// ScriptSystem Implementation
// ============================================================================

void ScriptSystem::OnInit() {
    if (!ScriptingEngine::Instance().Init()) {
        LOG_ERROR("Failed to initialize scripting engine");
        return;
    }
    initialized = true;
    LOG_INFO("ScriptSystem initialized");
}

void ScriptSystem::Update(float dt) {
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

void ScriptSystem::OnShutdown() {
    ScriptingEngine::Instance().Shutdown();
}

void ScriptSystem::InitializeScript(Entity entity, ScriptComponent& script) {
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

    // Call Start() method if it exists
    ScriptingEngine::Instance().SetCurrentEntity(entity, ecs);
    script.instance->CallStart();
    
    LOG_DEBUG("Successfully initialized script: " + script.className + " for entity " + std::to_string(entity));
}

void ScriptSystem::UpdateScript(Entity entity, ScriptComponent& script, float dt) {
    ScriptingEngine::Instance().SetCurrentEntity(entity, ecs);
    
    if (!script.instance->CallUpdate(dt)) {
        LOG_WARN("Script update failed for entity " + std::to_string(entity));
    }
}

// ============================================================================
// ScriptingEngine Implementation
// ============================================================================

bool ScriptingEngine::Init() {
    LOG_INFO("Initializing AngelScript scripting engine...");
    
    engine_ = asCreateScriptEngine();
    
    if (!engine_) {
        LOG_ERROR("Failed to create AngelScript engine");
        return false;
    }

    // Set message callback
    engine_->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);

    // Register standard add-ons
    RegisterStdString(engine_);
    RegisterScriptArray(engine_, true);
    RegisterScriptMath(engine_);

    // Register built-in API
    RegisterAPI();

    LOG_INFO("AngelScript scripting engine initialized");
    return true;
}

void ScriptingEngine::Shutdown() {
    if (engine_) {
        engine_->ShutDownAndRelease();
        engine_ = nullptr;
    }
    LOG_INFO("AngelScript scripting engine shutdown");
}

bool ScriptingEngine::LoadScript(const std::string& path, const std::string& moduleName) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open script file: " + path);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();

    // Remove old module if it exists
    asIScriptModule* oldMod = engine_->GetModule(moduleName.c_str(), asGM_ONLY_IF_EXISTS);
    if (oldMod) {
        oldMod->Discard();
    }

    CScriptBuilder builder;
    int r = builder.StartNewModule(engine_, moduleName.c_str());
    if (r < 0) {
        LOG_ERROR("Failed to start new module");
        return false;
    }

    r = builder.AddSectionFromMemory(path.c_str(), source.c_str());
    if (r < 0) {
        LOG_ERROR("Failed to add script section");
        return false;
    }

    r = builder.BuildModule();
    if (r < 0) {
        LOG_ERROR("Failed to build script module: " + path);
        return false;
    }

    LOG_DEBUG("Loaded script: " + path);
    return true;
}

std::shared_ptr<ScriptInstance> ScriptingEngine::CreateInstance(const std::string& moduleName, 
                                                                 const std::string& className) {
    asIScriptModule* mod = engine_->GetModule(moduleName.c_str());
    if (!mod) {
        LOG_ERROR("Module not found: " + moduleName);
        return nullptr;
    }

    asITypeInfo* type = mod->GetTypeInfoByName(className.c_str());
    if (!type) {
        LOG_ERROR("Class not found: " + className + " in module: " + moduleName);
        return nullptr;
    }

    // Find the factory function (constructor)
    asIScriptFunction* factory = type->GetFactoryByDecl((className + " @" + className + "()").c_str());
    if (!factory) {
        LOG_ERROR("No default constructor found for class: " + className);
        return nullptr;
    }

    // Create context and call constructor
    asIScriptContext* ctx = engine_->CreateContext();
    ctx->Prepare(factory);
    
    int r = ctx->Execute();
    if (r != asEXECUTION_FINISHED) {
        LOG_ERROR("Failed to execute constructor for class: " + className);
        ctx->Release();
        return nullptr;
    }

    // Get the object that was created
    asIScriptObject* obj = *static_cast<asIScriptObject**>(ctx->GetAddressOfReturnValue());
    ctx->Release();

    if (!obj) {
        LOG_ERROR("Failed to create object for class: " + className);
        return nullptr;
    }

    return std::make_shared<ScriptInstance>(engine_, mod, obj, type);
}

void ScriptingEngine::SetCurrentEntity(Entity entity, ECS* ecs) {
    currentEntity_ = entity;
    currentECS_ = ecs;
}

// ============================================================================
// AngelScript Callbacks
// ============================================================================

void ScriptingEngine::MessageCallback(const asSMessageInfo* msg, void* param) {
    (void)param;
    
    std::string message = std::string(msg->section) + " (" + 
                         std::to_string(msg->row) + ", " + 
                         std::to_string(msg->col) + ") : " + msg->message;
    
    switch (msg->type) {
        case asMSGTYPE_ERROR:
            LOG_ERROR("[AngelScript] " + message);
            break;
        case asMSGTYPE_WARNING:
            LOG_WARN("[AngelScript] " + message);
            break;
        case asMSGTYPE_INFORMATION:
            LOG_INFO("[AngelScript] " + message);
            break;
    }
}

// ============================================================================
// API Registration
// ============================================================================

void ScriptingEngine::RegisterAPI() {
    int r;

    // Register Vector3
    RegisterVector3(engine_);

    // Register Entity class
    r = engine_->RegisterObjectType("Entity", 0, asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Entity", "array<float>@ GetPosition()", 
        asFUNCTION(API_Entity_GetPosition), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Entity", "void SetPosition(float, float, float)", 
        asFUNCTION(API_Entity_SetPosition), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Entity", "array<float>@ GetVelocity()", 
        asFUNCTION(API_Entity_GetVelocity), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Entity", "void SetVelocity(float, float, float)", 
        asFUNCTION(API_Entity_SetVelocity), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Entity", "array<float>@ GetRotation()", 
        asFUNCTION(API_Entity_GetRotation), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Entity", "void SetRotation(float, float, float)", 
        asFUNCTION(API_Entity_SetRotation), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Entity", "array<float>@ GetScale()", 
        asFUNCTION(API_Entity_GetScale), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Entity", "void SetScale(float, float, float)", 
        asFUNCTION(API_Entity_SetScale), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Entity", "void Destroy()", 
        asFUNCTION(API_Entity_Destroy), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Entity", "int GetId()", 
        asFUNCTION(API_Entity_GetId), asCALL_CDECL_OBJLAST); assert(r >= 0);

    // Register global function to get current entity
    r = engine_->RegisterGlobalFunction("Entity@ GetEntity()", 
        asFUNCTION(API_GetEntity), asCALL_GENERIC); assert(r >= 0);

    // Register Input namespace
    r = engine_->RegisterGlobalFunction("bool IsKeyDown(int)", 
        asFUNCTION(API_Input_IsKeyDown), asCALL_CDECL); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("bool IsKeyPressed(int)", 
        asFUNCTION(API_Input_IsKeyPressed), asCALL_CDECL); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("bool IsKeyReleased(int)", 
        asFUNCTION(API_Input_IsKeyReleased), asCALL_CDECL); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("bool IsMouseButtonDown(int)", 
        asFUNCTION(API_Input_IsMouseButtonDown), asCALL_CDECL); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("bool IsMouseButtonPressed(int)", 
        asFUNCTION(API_Input_IsMouseButtonPressed), asCALL_CDECL); assert(r >= 0);

    // Register Time functions
    r = engine_->RegisterGlobalFunction("float GetDeltaTime()", 
        asFUNCTION(API_Time_GetDeltaTime), asCALL_CDECL); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("double GetTime()", 
        asFUNCTION(API_Time_GetTime), asCALL_CDECL); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("int GetFPS()", 
        asFUNCTION(API_Time_GetFPS), asCALL_CDECL); assert(r >= 0);

    // Register Debug functions
    r = engine_->RegisterGlobalFunction("void Print(const string &in)", 
        asFUNCTION(API_Debug_Log), asCALL_CDECL); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void DrawLine(float, float, float, float, float, float)", 
        asFUNCTION(API_Debug_DrawLine), asCALL_CDECL); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void DrawSphere(float, float, float, float)", 
        asFUNCTION(API_Debug_DrawSphere), asCALL_CDECL); assert(r >= 0);

    // Register Key constants
    r = engine_->RegisterEnum("Key"); assert(r >= 0);
    r = engine_->RegisterEnumValue("Key", "SPACE", KEY_SPACE); assert(r >= 0);
    r = engine_->RegisterEnumValue("Key", "ESCAPE", KEY_ESCAPE); assert(r >= 0);
    r = engine_->RegisterEnumValue("Key", "ENTER", KEY_ENTER); assert(r >= 0);
    r = engine_->RegisterEnumValue("Key", "W", KEY_W); assert(r >= 0);
    r = engine_->RegisterEnumValue("Key", "A", KEY_A); assert(r >= 0);
    r = engine_->RegisterEnumValue("Key", "S", KEY_S); assert(r >= 0);
    r = engine_->RegisterEnumValue("Key", "D", KEY_D); assert(r >= 0);
    r = engine_->RegisterEnumValue("Key", "LEFT_SHIFT", KEY_LEFT_SHIFT); assert(r >= 0);
    r = engine_->RegisterEnumValue("Key", "UP", KEY_UP); assert(r >= 0);
    r = engine_->RegisterEnumValue("Key", "DOWN", KEY_DOWN); assert(r >= 0);
    r = engine_->RegisterEnumValue("Key", "LEFT", KEY_LEFT); assert(r >= 0);
    r = engine_->RegisterEnumValue("Key", "RIGHT", KEY_RIGHT); assert(r >= 0);

    LOG_DEBUG("AngelScript API registered successfully");
}

void ScriptingEngine::RegisterVector3(asIScriptEngine* engine) {
    int r;
    r = engine->RegisterObjectType("Vector3", sizeof(Vector3Wrapper), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CAK); assert(r >= 0);
    
    r = engine->RegisterObjectBehaviour("Vector3", asBEHAVE_CONSTRUCT, "void f()", 
        asFUNCTION([](Vector3Wrapper* self) { new(self) Vector3Wrapper(); }), asCALL_CDECL_OBJLAST); assert(r >= 0);
    r = engine->RegisterObjectBehaviour("Vector3", asBEHAVE_CONSTRUCT, "void f(float, float, float)", 
        asFUNCTION([](float x, float y, float z, Vector3Wrapper* self) { new(self) Vector3Wrapper(x, y, z); }), asCALL_CDECL_OBJLAST); assert(r >= 0);
    
    r = engine->RegisterObjectProperty("Vector3", "float x", asOFFSET(Vector3Wrapper, x)); assert(r >= 0);
    r = engine->RegisterObjectProperty("Vector3", "float y", asOFFSET(Vector3Wrapper, y)); assert(r >= 0);
    r = engine->RegisterObjectProperty("Vector3", "float z", asOFFSET(Vector3Wrapper, z)); assert(r >= 0);
    
    r = engine->RegisterObjectMethod("Vector3", "Vector3 opAdd(const Vector3 &in) const", 
        asMETHODPR(Vector3Wrapper, operator+, (const Vector3Wrapper&) const, Vector3Wrapper), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vector3", "Vector3 opSub(const Vector3 &in) const", 
        asMETHODPR(Vector3Wrapper, operator-, (const Vector3Wrapper&) const, Vector3Wrapper), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vector3", "Vector3 opMul(float) const", 
        asMETHODPR(Vector3Wrapper, operator*, (float) const, Vector3Wrapper), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vector3", "Vector3 opDiv(float) const", 
        asMETHODPR(Vector3Wrapper, operator/, (float) const, Vector3Wrapper), asCALL_THISCALL); assert(r >= 0);
    
    r = engine->RegisterObjectMethod("Vector3", "float Length() const", 
        asMETHOD(Vector3Wrapper, Length), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vector3", "Vector3 Normalized() const", 
        asMETHOD(Vector3Wrapper, Normalized), asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod("Vector3", "float Dot(const Vector3 &in) const", 
        asMETHOD(Vector3Wrapper, Dot), asCALL_THISCALL); assert(r >= 0);
    
    r = engine->RegisterObjectMethod("Vector3", "float Distance(const Vector3 &in, const Vector3 &in)", 
        asFUNCTION(Vector3Wrapper::Distance), asCALL_CDECL_OBJLAST); assert(r >= 0);
}

// ============================================================================
// API Implementation - Entity
// ============================================================================

void ScriptingEngine::API_GetEntity(asIScriptGeneric* gen) {
    // Return a dummy entity object (the entity is accessed through context)
    gen->SetReturnAddress(nullptr); // Entity is managed by context
}

CScriptArray* ScriptingEngine::API_Entity_GetPosition() {
    ScriptingEngine& engine = ScriptingEngine::Instance();
    Entity entity = engine.GetCurrentEntity();
    ECS* ecs = engine.GetCurrentECS();

    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* scriptEngine = ctx->GetEngine();
    asITypeInfo* arrayType = scriptEngine->GetTypeInfoByDecl("array<float>");
    CScriptArray* arr = CScriptArray::Create(arrayType, 3);

    if (ecs && entity != INVALID_ENTITY && ecs->HasComponent<ZeroTransform>(entity)) {
        auto& t = ecs->GetComponent<ZeroTransform>(entity);
        arr->SetValue(0, &t.position.x);
        arr->SetValue(1, &t.position.y);
        arr->SetValue(2, &t.position.z);
    }

    return arr;
}

void ScriptingEngine::API_Entity_SetPosition(float x, float y, float z) {
    ScriptingEngine& engine = ScriptingEngine::Instance();
    Entity entity = engine.GetCurrentEntity();
    ECS* ecs = engine.GetCurrentECS();

    if (ecs && entity != INVALID_ENTITY && ecs->HasComponent<ZeroTransform>(entity)) {
        auto& t = ecs->GetComponent<ZeroTransform>(entity);
        t.position = {x, y, z};
    }
}

CScriptArray* ScriptingEngine::API_Entity_GetVelocity() {
    ScriptingEngine& engine = ScriptingEngine::Instance();
    Entity entity = engine.GetCurrentEntity();
    ECS* ecs = engine.GetCurrentECS();

    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* scriptEngine = ctx->GetEngine();
    asITypeInfo* arrayType = scriptEngine->GetTypeInfoByDecl("array<float>");
    CScriptArray* arr = CScriptArray::Create(arrayType, 3);

    if (ecs && entity != INVALID_ENTITY && ecs->HasComponent<Velocity>(entity)) {
        auto& v = ecs->GetComponent<Velocity>(entity);
        arr->SetValue(0, &v.v.x);
        arr->SetValue(1, &v.v.y);
        arr->SetValue(2, &v.v.z);
    }

    return arr;
}

void ScriptingEngine::API_Entity_SetVelocity(float x, float y, float z) {
    ScriptingEngine& engine = ScriptingEngine::Instance();
    Entity entity = engine.GetCurrentEntity();
    ECS* ecs = engine.GetCurrentECS();

    if (ecs && entity != INVALID_ENTITY && ecs->HasComponent<Velocity>(entity)) {
        auto& v = ecs->GetComponent<Velocity>(entity);
        v.v = {x, y, z};
    }
}

CScriptArray* ScriptingEngine::API_Entity_GetRotation() {
    ScriptingEngine& engine = ScriptingEngine::Instance();
    Entity entity = engine.GetCurrentEntity();
    ECS* ecs = engine.GetCurrentECS();

    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* scriptEngine = ctx->GetEngine();
    asITypeInfo* arrayType = scriptEngine->GetTypeInfoByDecl("array<float>");
    CScriptArray* arr = CScriptArray::Create(arrayType, 3);

    if (ecs && entity != INVALID_ENTITY && ecs->HasComponent<ZeroTransform>(entity)) {
        auto& t = ecs->GetComponent<ZeroTransform>(entity);
        arr->SetValue(0, &t.rotation.x);
        arr->SetValue(1, &t.rotation.y);
        arr->SetValue(2, &t.rotation.z);
    }

    return arr;
}

void ScriptingEngine::API_Entity_SetRotation(float x, float y, float z) {
    ScriptingEngine& engine = ScriptingEngine::Instance();
    Entity entity = engine.GetCurrentEntity();
    ECS* ecs = engine.GetCurrentECS();

    if (ecs && entity != INVALID_ENTITY && ecs->HasComponent<ZeroTransform>(entity)) {
        auto& t = ecs->GetComponent<ZeroTransform>(entity);
        t.rotation = {x, y, z};
    }
}

CScriptArray* ScriptingEngine::API_Entity_GetScale() {
    ScriptingEngine& engine = ScriptingEngine::Instance();
    Entity entity = engine.GetCurrentEntity();
    ECS* ecs = engine.GetCurrentECS();

    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* scriptEngine = ctx->GetEngine();
    asITypeInfo* arrayType = scriptEngine->GetTypeInfoByDecl("array<float>");
    CScriptArray* arr = CScriptArray::Create(arrayType, 3);

    if (ecs && entity != INVALID_ENTITY && ecs->HasComponent<ZeroTransform>(entity)) {
        auto& t = ecs->GetComponent<ZeroTransform>(entity);
        arr->SetValue(0, &t.scale.x);
        arr->SetValue(1, &t.scale.y);
        arr->SetValue(2, &t.scale.z);
    }

    return arr;
}

void ScriptingEngine::API_Entity_SetScale(float x, float y, float z) {
    ScriptingEngine& engine = ScriptingEngine::Instance();
    Entity entity = engine.GetCurrentEntity();
    ECS* ecs = engine.GetCurrentECS();

    if (ecs && entity != INVALID_ENTITY && ecs->HasComponent<ZeroTransform>(entity)) {
        auto& t = ecs->GetComponent<ZeroTransform>(entity);
        t.scale = {x, y, z};
    }
}

void ScriptingEngine::API_Entity_Destroy() {
    ScriptingEngine& engine = ScriptingEngine::Instance();
    Entity entity = engine.GetCurrentEntity();
    ECS* ecs = engine.GetCurrentECS();

    if (ecs && entity != INVALID_ENTITY) {
        ecs->DestroyEntity(entity);
    }
}

int ScriptingEngine::API_Entity_GetId() {
    ScriptingEngine& engine = ScriptingEngine::Instance();
    return static_cast<int>(engine.GetCurrentEntity());
}

// ============================================================================
// API Implementation - Input
// ============================================================================

bool ScriptingEngine::API_Input_IsKeyDown(int key) {
    return IsKeyDown(key);
}

bool ScriptingEngine::API_Input_IsKeyPressed(int key) {
    return IsKeyPressed(key);
}

bool ScriptingEngine::API_Input_IsKeyReleased(int key) {
    return IsKeyReleased(key);
}

bool ScriptingEngine::API_Input_IsMouseButtonDown(int button) {
    return IsMouseButtonDown(button);
}

bool ScriptingEngine::API_Input_IsMouseButtonPressed(int button) {
    return IsMouseButtonPressed(button);
}

CScriptArray* ScriptingEngine::API_Input_GetMousePosition() {
    Vector2 pos = GetMousePosition();
    
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoByDecl("array<float>");
    CScriptArray* arr = CScriptArray::Create(arrayType, 2);
    
    arr->SetValue(0, &pos.x);
    arr->SetValue(1, &pos.y);
    
    return arr;
}

CScriptArray* ScriptingEngine::API_Input_GetMouseDelta() {
    Vector2 delta = GetMouseDelta();
    
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoByDecl("array<float>");
    CScriptArray* arr = CScriptArray::Create(arrayType, 2);
    
    arr->SetValue(0, &delta.x);
    arr->SetValue(1, &delta.y);
    
    return arr;
}

// ============================================================================
// API Implementation - Time
// ============================================================================

float ScriptingEngine::API_Time_GetDeltaTime() {
    return GetFrameTime();
}

double ScriptingEngine::API_Time_GetTime() {
    return GetTime();
}

int ScriptingEngine::API_Time_GetFPS() {
    return GetFPS();
}

// ============================================================================
// API Implementation - Debug
// ============================================================================

void ScriptingEngine::API_Debug_Log(const std::string& message) {
    LOG_INFO(message);
}

void ScriptingEngine::API_Debug_DrawLine(float x1, float y1, float z1, float x2, float y2, float z2) {
    DrawLine3D({x1, y1, z1}, {x2, y2, z2}, RED);
}

void ScriptingEngine::API_Debug_DrawSphere(float x, float y, float z, float radius) {
    DrawSphere({x, y, z}, radius, RED);
}
