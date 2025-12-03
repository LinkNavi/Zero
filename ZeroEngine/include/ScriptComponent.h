#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <wren.hpp>

// Forward declarations
class ScriptInstance;

// Script Component - Attached to entities that have scripts
struct ScriptComponent {
    std::string scriptPath;           // Path to .wren file
    std::string className;            // Class name in Wren (e.g., "Player")
    std::shared_ptr<ScriptInstance> instance;  // Runtime instance
    bool isInitialized = false;
    bool enabled = true;
};

// Represents a single script instance in Wren
class ScriptInstance {
public:
    ScriptInstance(WrenVM* vm, WrenHandle* classHandle, WrenHandle* instanceHandle)
        : vm_(vm), classHandle_(classHandle), instanceHandle_(instanceHandle) {}

    ~ScriptInstance() {
        if (vm_) {
            if (instanceHandle_) wrenReleaseHandle(vm_, instanceHandle_);
            if (classHandle_) wrenReleaseHandle(vm_, classHandle_);
        }
    }

    WrenVM* GetVM() const { return vm_; }
    WrenHandle* GetClassHandle() const { return classHandle_; }
    WrenHandle* GetInstanceHandle() const { return instanceHandle_; }

    // Call a method on this instance
    template<typename... Args>
    bool CallMethod(const std::string& signature, Args... args);

private:
    WrenVM* vm_;
    WrenHandle* classHandle_;
    WrenHandle* instanceHandle_;
};

// Script method signatures that scripts can implement
namespace ScriptMethods {
    const char* START = "start()";
    const char* UPDATE = "update(_)";
    const char* ON_DESTROY = "onDestroy()";
    const char* ON_COLLISION = "onCollision(_)";
}
