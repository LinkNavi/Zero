#pragma once
#include <chrono>

class Time {
    static inline std::chrono::high_resolution_clock::time_point startTime;
    static inline std::chrono::high_resolution_clock::time_point lastFrameTime;
    static inline float deltaTime = 0.0f;
    static inline float time = 0.0f;
    static inline float timeScale = 1.0f;
    static inline bool paused = false;
    static inline uint64_t frameCount = 0;
    
public:
    static void init() {
        startTime = std::chrono::high_resolution_clock::now();
        lastFrameTime = startTime;
    }
    
    static void update() {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float rawDelta = std::chrono::duration<float>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;
        
        // Cap delta time to prevent huge jumps
        if (rawDelta > 0.1f) rawDelta = 0.1f;
        
        if (!paused) {
            deltaTime = rawDelta * timeScale;
            time += deltaTime;
            frameCount++;
        } else {
            deltaTime = 0.0f;
        }
    }
    
    // Get scaled delta time
    static float getDeltaTime() { return deltaTime; }
    
    // Get unscaled delta time (real time)
    static float getRealDeltaTime() { 
        auto currentTime = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<float>(currentTime - lastFrameTime).count();
    }
    
    // Get time since start
    static float getTime() { return time; }
    
    // Get real time since start
    static float getRealTime() {
        auto currentTime = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<float>(currentTime - startTime).count();
    }
    
    // Time scale (1.0 = normal, 0.5 = slow motion, 2.0 = fast forward)
    static void setTimeScale(float scale) { 
        timeScale = scale;
        if (timeScale < 0.0f) timeScale = 0.0f;
    }
    
    static float getTimeScale() { return timeScale; }
    
    // Pause/unpause
    static void setPaused(bool isPaused) { paused = isPaused; }
    static bool isPaused() { return paused; }
    static void togglePause() { paused = !paused; }
    
    // Frame count
    static uint64_t getFrameCount() { return frameCount; }
    
    // FPS calculation
    static float getFPS() {
        return deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f;
    }
};
