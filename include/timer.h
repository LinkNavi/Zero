#pragma once
#include <functional>
#include <vector>
#include <string>
#include <cmath>
// Simple timer with callback
class Timer {
public:
    using Callback = std::function<void()>;
    
private:
    float duration;
    float elapsed = 0.0f;
    bool repeat = false;
    bool paused = false;
    bool finished = false;
    Callback callback;
    std::string name;
    
public:
    Timer(float dur, Callback cb, bool loop = false, const std::string& timerName = "")
        : duration(dur), repeat(loop), callback(cb), name(timerName) {}
    
    // Update timer, returns true if finished (non-repeating timers only)
    bool update(float dt) {
        if (finished || paused) return finished;
        
        elapsed += dt;
        
        if (elapsed >= duration) {
            // Execute callback
            if (callback) {
                callback();
            }
            
            if (repeat) {
                elapsed -= duration; // Reset for next iteration
            } else {
                finished = true;
            }
        }
        
        return finished;
    }
    
    // Control
    void pause() { paused = true; }
    void resume() { paused = false; }
    void stop() { finished = true; }
    void reset() { elapsed = 0.0f; finished = false; }
    
    // Query
    bool isFinished() const { return finished; }
    bool isPaused() const { return paused; }
    bool isRepeating() const { return repeat; }
    float getElapsed() const { return elapsed; }
    float getDuration() const { return duration; }
    float getProgress() const { return elapsed / duration; }
    float getRemaining() const { return duration - elapsed; }
    const std::string& getName() const { return name; }
    
    // Modify
    void setDuration(float dur) { duration = dur; }
    void setRepeating(bool loop) { repeat = loop; }
};

// Timer manager
class TimerManager {
    std::vector<Timer> timers;
    bool updating = false;
    std::vector<size_t> toRemove;
    
public:
    // Add timer and get its index
    size_t addTimer(float duration, Timer::Callback callback, bool repeat = false, const std::string& name = "") {
        timers.emplace_back(duration, callback, repeat, name);
        return timers.size() - 1;
    }
    
    // Convenience methods
    size_t after(float seconds, Timer::Callback callback, const std::string& name = "") {
        return addTimer(seconds, callback, false, name);
    }
    
    size_t every(float seconds, Timer::Callback callback, const std::string& name = "") {
        return addTimer(seconds, callback, true, name);
    }
    
    // Update all timers
    void update(float dt) {
        updating = true;
        toRemove.clear();
        
        for (size_t i = 0; i < timers.size(); ++i) {
            if (timers[i].update(dt)) {
                toRemove.push_back(i);
            }
        }
        
        updating = false;
        
        // Remove finished timers (in reverse order to maintain indices)
        for (auto it = toRemove.rbegin(); it != toRemove.rend(); ++it) {
            timers.erase(timers.begin() + *it);
        }
    }
    
    // Access timers
    Timer* getTimer(size_t index) {
        return index < timers.size() ? &timers[index] : nullptr;
    }
    
    Timer* findTimer(const std::string& name) {
        for (auto& timer : timers) {
            if (timer.getName() == name) {
                return &timer;
            }
        }
        return nullptr;
    }
    
    // Remove timer
    void removeTimer(size_t index) {
        if (index < timers.size()) {
            if (updating) {
                toRemove.push_back(index);
            } else {
                timers.erase(timers.begin() + index);
            }
        }
    }
    
    void removeTimer(const std::string& name) {
        for (size_t i = 0; i < timers.size(); ++i) {
            if (timers[i].getName() == name) {
                removeTimer(i);
                return;
            }
        }
    }
    
    // Clear all timers
    void clear() {
        timers.clear();
        toRemove.clear();
    }
    
    // Query
    size_t getTimerCount() const { return timers.size(); }
    bool hasTimer(const std::string& name) const {
        for (const auto& timer : timers) {
            if (timer.getName() == name) return true;
        }
        return false;
    }
};

// Coroutine-style delayed action helper
class DelayedAction {
public:
    static void execute(TimerManager& manager, float delay, Timer::Callback action, const std::string& name = "") {
        manager.after(delay, action, name);
    }
    
    // Chain multiple delayed actions
    static void sequence(TimerManager& manager, std::vector<std::pair<float, Timer::Callback>> actions) {
        if (actions.empty()) return;
        
        // Create a chain of timers
        auto executeNext = [&manager, actions, index = 0]() mutable {
            if (index >= actions.size()) return;
            
            const auto& [delay, action] = actions[index];
            index++;
            
            manager.after(delay, [action, &manager, actions, index]() mutable {
                action();
                // Schedule next
                if (index < actions.size()) {
                    const auto& [nextDelay, nextAction] = actions[index];
                    manager.after(nextDelay, nextAction);
                }
            });
        };
        
        executeNext();
    }
};

// Interpolation timer (lerp between values over time)
template<typename T>
class LerpTimer {
    T startValue;
    T endValue;
    T currentValue;
    float duration;
    float elapsed = 0.0f;
    bool finished = false;
    std::function<void(const T&)> onUpdate;
    std::function<void()> onComplete;
    
public:
    LerpTimer(const T& start, const T& end, float dur, 
              std::function<void(const T&)> update = nullptr,
              std::function<void()> complete = nullptr)
        : startValue(start), endValue(end), currentValue(start),
          duration(dur), onUpdate(update), onComplete(complete) {}
    
    bool update(float dt) {
        if (finished) return true;
        
        elapsed += dt;
        float t = elapsed / duration;
        
        if (t >= 1.0f) {
            t = 1.0f;
            finished = true;
            currentValue = endValue;
            
            if (onUpdate) onUpdate(currentValue);
            if (onComplete) onComplete();
        } else {
            // Linear interpolation
            currentValue = startValue + (endValue - startValue) * t;
            if (onUpdate) onUpdate(currentValue);
        }
        
        return finished;
    }
    
    const T& getValue() const { return currentValue; }
    bool isFinished() const { return finished; }
    float getProgress() const { return elapsed / duration; }
};

// Easing functions for smooth animations
namespace Easing {
    inline float linear(float t) { return t; }
    
    inline float easeInQuad(float t) { return t * t; }
    inline float easeOutQuad(float t) { return t * (2.0f - t); }
    inline float easeInOutQuad(float t) { 
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; 
    }
    
    inline float easeInCubic(float t) { return t * t * t; }
    inline float easeOutCubic(float t) { 
        float f = t - 1.0f; 
        return f * f * f + 1.0f; 
    }
    inline float easeInOutCubic(float t) {
        return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
    }
    
    inline float easeInSin(float t) { 
        return 1.0f - std::cos(t * 3.14159f / 2.0f); 
    }
    inline float easeOutSin(float t) { 
        return std::sin(t * 3.14159f / 2.0f); 
    }
    inline float easeInOutSin(float t) { 
        return -(std::cos(3.14159f * t) - 1.0f) / 2.0f; 
    }
    
    inline float easeInElastic(float t) {
        if (t == 0.0f || t == 1.0f) return t;
        return -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * (2.0f * 3.14159f) / 3.0f);
    }
    
    inline float easeOutBounce(float t) {
        const float n1 = 7.5625f;
        const float d1 = 2.75f;
        
        if (t < 1.0f / d1) {
            return n1 * t * t;
        } else if (t < 2.0f / d1) {
            t -= 1.5f / d1;
            return n1 * t * t + 0.75f;
        } else if (t < 2.5f / d1) {
            t -= 2.25f / d1;
            return n1 * t * t + 0.9375f;
        } else {
            t -= 2.625f / d1;
            return n1 * t * t + 0.984375f;
        }
    }
}

// Tween helper with easing
template<typename T>
class Tween {
    T startValue;
    T endValue;
    T currentValue;
    float duration;
    float elapsed = 0.0f;
    bool finished = false;
    std::function<float(float)> easingFunc = Easing::linear;
    std::function<void(const T&)> onUpdate;
    std::function<void()> onComplete;
    
public:
    Tween(const T& start, const T& end, float dur,
          std::function<float(float)> easing = Easing::linear,
          std::function<void(const T&)> update = nullptr,
          std::function<void()> complete = nullptr)
        : startValue(start), endValue(end), currentValue(start),
          duration(dur), easingFunc(easing), onUpdate(update), onComplete(complete) {}
    
    bool update(float dt) {
        if (finished) return true;
        
        elapsed += dt;
        float t = elapsed / duration;
        
        if (t >= 1.0f) {
            t = 1.0f;
            finished = true;
            currentValue = endValue;
            
            if (onUpdate) onUpdate(currentValue);
            if (onComplete) onComplete();
        } else {
            float easedT = easingFunc(t);
            currentValue = startValue + (endValue - startValue) * easedT;
            if (onUpdate) onUpdate(currentValue);
        }
        
        return finished;
    }
    
    const T& getValue() const { return currentValue; }
    bool isFinished() const { return finished; }
    float getProgress() const { return elapsed / duration; }
    
    void setEasing(std::function<float(float)> easing) { easingFunc = easing; }
};
