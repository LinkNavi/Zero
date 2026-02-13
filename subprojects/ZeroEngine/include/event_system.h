#pragma once
#include "Engine.h"
#include <functional>
#include <unordered_map>
#include <vector>
#include <string>
#include <variant>
#include <glm/glm.hpp>

enum class EventType {
    // Collision events
    OnCollisionEnter,
    OnCollisionExit,
    OnTriggerEnter,
    OnTriggerExit,
    
    // Input events
    OnKeyPress,
    OnKeyRelease,
    OnMouseClick,
    OnMouseRelease,
    OnMouseMove,
    
    // Entity lifecycle
    OnEntityCreated,
    OnEntityDestroyed,
    OnComponentAdded,
    OnComponentRemoved,
    
    // Scene events
    OnSceneLoaded,
    OnSceneUnloaded,
    
    // Game events
    OnUpdate,
    OnFixedUpdate,
    OnLateUpdate,
    
    // Custom events
    Custom
};

// Event data payload
using EventData = std::variant<
    int, float, double, bool, 
    std::string, 
    glm::vec2, glm::vec3, glm::vec4,
    EntityID
>;

struct Event {
    EventType type;
    EntityID entity = 0;
    std::string customType; // For Custom events
    std::unordered_map<std::string, EventData> data;
    float timestamp = 0.0f;
    
    // Helper methods to get data safely
    template<typename T>
    T get(const std::string& key, T defaultValue = T{}) const {
        auto it = data.find(key);
        if (it != data.end()) {
            if (auto* val = std::get_if<T>(&it->second)) {
                return *val;
            }
        }
        return defaultValue;
    }
    
    bool has(const std::string& key) const {
        return data.find(key) != data.end();
    }
    
    // Convenience setters
    void setInt(const std::string& key, int value) { data[key] = value; }
    void setFloat(const std::string& key, float value) { data[key] = value; }
    void setBool(const std::string& key, bool value) { data[key] = value; }
    void setString(const std::string& key, const std::string& value) { data[key] = value; }
    void setVec3(const std::string& key, const glm::vec3& value) { data[key] = value; }
    void setEntity(const std::string& key, EntityID value) { data[key] = value; }
};

// Event listener handle for unsubscribing
using ListenerHandle = size_t;

class EventSystem {
public:
    using Callback = std::function<void(const Event&)>;
    
private:
    struct Listener {
        ListenerHandle handle;
        Callback callback;
        int priority = 0; // Higher priority = called first
    };
    
    std::unordered_map<EventType, std::vector<Listener>> listeners;
    std::unordered_map<std::string, std::vector<Listener>> customListeners;
    
    std::vector<Event> eventQueue;
    bool processingEvents = false;
    
    ListenerHandle nextHandle = 1;
    
public:
    // Subscribe to event type
    ListenerHandle subscribe(EventType type, Callback callback, int priority = 0) {
        ListenerHandle handle = nextHandle++;
        listeners[type].push_back({handle, callback, priority});
        
        // Sort by priority (descending)
        auto& list = listeners[type];
        std::sort(list.begin(), list.end(), 
            [](const Listener& a, const Listener& b) { 
                return a.priority > b.priority; 
            });
        
        return handle;
    }
    
    // Subscribe to custom event
    ListenerHandle subscribe(const std::string& customType, Callback callback, int priority = 0) {
        ListenerHandle handle = nextHandle++;
        customListeners[customType].push_back({handle, callback, priority});
        
        auto& list = customListeners[customType];
        std::sort(list.begin(), list.end(),
            [](const Listener& a, const Listener& b) {
                return a.priority > b.priority;
            });
        
        return handle;
    }
    
    // Unsubscribe by handle
    void unsubscribe(ListenerHandle handle) {
        // Search in regular listeners
        for (auto& [type, list] : listeners) {
            auto it = std::remove_if(list.begin(), list.end(),
                [handle](const Listener& l) { return l.handle == handle; });
            list.erase(it, list.end());
        }
        
        // Search in custom listeners
        for (auto& [type, list] : customListeners) {
            auto it = std::remove_if(list.begin(), list.end(),
                [handle](const Listener& l) { return l.handle == handle; });
            list.erase(it, list.end());
        }
    }
    
    // Emit event immediately (synchronous)
    void emit(const Event& event) {
        if (event.type == EventType::Custom) {
            auto it = customListeners.find(event.customType);
            if (it != customListeners.end()) {
                for (auto& listener : it->second) {
                    listener.callback(event);
                }
            }
        } else {
            auto it = listeners.find(event.type);
            if (it != listeners.end()) {
                for (auto& listener : it->second) {
                    listener.callback(event);
                }
            }
        }
    }
    
    // Queue event for later processing (asynchronous)
    void queue(const Event& event) {
        eventQueue.push_back(event);
    }
    
    // Process all queued events
    void processQueue() {
        if (processingEvents) return; // Prevent recursive processing
        
        processingEvents = true;
        
        // Process queue (allow new events to be queued during processing)
        while (!eventQueue.empty()) {
            std::vector<Event> currentQueue = std::move(eventQueue);
            eventQueue.clear();
            
            for (const auto& event : currentQueue) {
                emit(event);
            }
        }
        
        processingEvents = false;
    }
    
    // Clear all listeners
    void clear() {
        listeners.clear();
        customListeners.clear();
        eventQueue.clear();
    }
    
    // Get number of listeners for event type
    size_t getListenerCount(EventType type) const {
        auto it = listeners.find(type);
        return it != listeners.end() ? it->second.size() : 0;
    }
    
    size_t getCustomListenerCount(const std::string& customType) const {
        auto it = customListeners.find(customType);
        return it != customListeners.end() ? it->second.size() : 0;
    }
};

// Global event system instance (optional, or pass around as needed)
// EventSystem& getGlobalEventSystem();

// Helper macros for creating events
#define EVENT_CREATE(type) Event{type}
#define EVENT_CUSTOM(name) Event{EventType::Custom, 0, name}
