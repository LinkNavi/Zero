#pragma once
#include <string>
#include <memory>
#include <angelscript.h>

// Forward declarations
class ScriptInstance;

// Script Component - Attached to entities that have scripts
struct ScriptComponent {
    std::string scriptPath;           // Path to .as file
    std::string className;            // Class name in AngelScript (e.g., "Player")
    std::shared_ptr<ScriptInstance> instance;  // Runtime instance
    bool isInitialized = false;
    bool enabled = true;
};

// Represents a single script instance in AngelScript
class ScriptInstance {
public:
    ScriptInstance(asIScriptEngine* engine, asIScriptModule* module, 
                   asIScriptObject* object, asITypeInfo* type)
        : engine_(engine), module_(module), scriptObject_(object), typeInfo_(type) {
        
        if (scriptObject_) scriptObject_->AddRef();
        
        // Cache method pointers for performance
        startMethod_ = typeInfo_->GetMethodByDecl("void Start()");
        updateMethod_ = typeInfo_->GetMethodByDecl("void Update(float)");
        onDestroyMethod_ = typeInfo_->GetMethodByDecl("void OnDestroy()");
    }

    ~ScriptInstance() {
        if (scriptObject_) {
            scriptObject_->Release();
        }
    }

    asIScriptEngine* GetEngine() const { return engine_; }
    asIScriptObject* GetObject() const { return scriptObject_; }
    asIScriptModule* GetModule() const { return module_; }

    // Call methods on this instance
    bool CallStart();
    bool CallUpdate(float dt);
    bool CallOnDestroy();

private:
    asIScriptEngine* engine_;
    asIScriptModule* module_;
    asIScriptObject* scriptObject_;
    asITypeInfo* typeInfo_;
    
    // Cached method pointers
    asIScriptFunction* startMethod_ = nullptr;
    asIScriptFunction* updateMethod_ = nullptr;
    asIScriptFunction* onDestroyMethod_ = nullptr;
};
