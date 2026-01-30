#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <unordered_map>

enum class Key {
    W, A, S, D,
    Q, E, J,
    Space, Shift, Ctrl,
    Up, Down, Left, Right,
    Escape,
    Mouse1, Mouse2, Mouse3
};

class Input {
    static inline std::unordered_map<Key, bool> keys;
    static inline std::unordered_map<Key, bool> keysLastFrame;
    static inline glm::vec2 mousePosition{0.0f};
    static inline glm::vec2 mousePositionLast{0.0f};
    static inline glm::vec2 mouseDelta{0.0f};
    static inline bool firstMouse = true;
    static inline float scrollDelta = 0.0f;
    
public:
    static void init(GLFWwindow* window) {
        // Set up callbacks
        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetCursorPosCallback(window, cursorPosCallback);
        glfwSetScrollCallback(window, scrollCallback);
        
        // Initialize key states
        keys[Key::W] = false;
        keys[Key::A] = false;
        keys[Key::S] = false;
        keys[Key::D] = false;
        keys[Key::Q] = false;
        keys[Key::E] = false;
        keys[Key::J] = false;
        keys[Key::Space] = false;
        keys[Key::Shift] = false;
        keys[Key::Ctrl] = false;
        keys[Key::Up] = false;
        keys[Key::Down] = false;
        keys[Key::Left] = false;
        keys[Key::Right] = false;
        keys[Key::Escape] = false;
        keys[Key::Mouse1] = false;
        keys[Key::Mouse2] = false;
        keys[Key::Mouse3] = false;
        
        keysLastFrame = keys;
    }
    
    static void update() {
        keysLastFrame = keys;
        mousePositionLast = mousePosition;
        scrollDelta = 0.0f;
    }
    
    // Check if key is currently held down
    static bool getKey(Key key) {
        return keys[key];
    }
    
    // Check if key was just pressed this frame
    static bool getKeyDown(Key key) {
        return keys[key] && !keysLastFrame[key];
    }
    
    // Check if key was just released this frame
    static bool getKeyUp(Key key) {
        return !keys[key] && keysLastFrame[key];
    }
    
    static glm::vec2 getMousePosition() {
        return mousePosition;
    }
    
    static glm::vec2 getMouseDelta() {
        return mouseDelta;
    }
    
    static float getScrollDelta() {
        return scrollDelta;
    }
    
private:
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        bool pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
        
        switch (key) {
            case GLFW_KEY_W: keys[Key::W] = pressed; break;
            case GLFW_KEY_A: keys[Key::A] = pressed; break;
            case GLFW_KEY_S: keys[Key::S] = pressed; break;
            case GLFW_KEY_D: keys[Key::D] = pressed; break;
            case GLFW_KEY_Q: keys[Key::Q] = pressed; break;
            case GLFW_KEY_E: keys[Key::E] = pressed; break;
            case GLFW_KEY_J: keys[Key::J] = pressed; break;
            case GLFW_KEY_SPACE: keys[Key::Space] = pressed; break;
            case GLFW_KEY_LEFT_SHIFT: keys[Key::Shift] = pressed; break;
            case GLFW_KEY_LEFT_CONTROL: keys[Key::Ctrl] = pressed; break;
            case GLFW_KEY_UP: keys[Key::Up] = pressed; break;
            case GLFW_KEY_DOWN: keys[Key::Down] = pressed; break;
            case GLFW_KEY_LEFT: keys[Key::Left] = pressed; break;
            case GLFW_KEY_RIGHT: keys[Key::Right] = pressed; break;
            case GLFW_KEY_ESCAPE: keys[Key::Escape] = pressed; break;
        }
    }
    
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        bool pressed = (action == GLFW_PRESS);
        
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT: keys[Key::Mouse1] = pressed; break;
            case GLFW_MOUSE_BUTTON_RIGHT: keys[Key::Mouse2] = pressed; break;
            case GLFW_MOUSE_BUTTON_MIDDLE: keys[Key::Mouse3] = pressed; break;
        }
    }
    
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
        mousePosition = glm::vec2(xpos, ypos);
        
        if (firstMouse) {
            mousePositionLast = mousePosition;
            firstMouse = false;
        }
        
        mouseDelta = mousePosition - mousePositionLast;
    }
    
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        scrollDelta = static_cast<float>(yoffset);
    }
};
