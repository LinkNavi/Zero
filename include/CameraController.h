#pragma once
#include "Camera.h"
#include "Input.h"
#include <glm/gtc/matrix_transform.hpp>

class CameraController {
public:
    Camera* camera;
    
    // Movement settings
    float moveSpeed = 5.0f;
    float sprintMultiplier = 2.0f;
    float mouseSensitivity = 0.1f;
    
    // Camera rotation
    float yaw = -90.0f;   // Left/right
    float pitch = 0.0f;   // Up/down
    
    // Camera vectors
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    
    bool mouseLookEnabled = false;
    
    CameraController(Camera* cam) : camera(cam) {
        updateCameraVectors();
    }
    
    void update(float dt, GLFWwindow* window) {
        // Toggle mouse look with right click
        if (Input::getKeyDown(Key::Mouse2)) {
            mouseLookEnabled = !mouseLookEnabled;
            
            if (mouseLookEnabled) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
        
        // Mouse look
        if (mouseLookEnabled) {
            glm::vec2 mouseDelta = Input::getMouseDelta();
            
            yaw += mouseDelta.x * mouseSensitivity;
            pitch -= mouseDelta.y * mouseSensitivity;
            
            // Constrain pitch
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;
            
            updateCameraVectors();
        }
        
        // Movement
        float speed = moveSpeed;
        if (Input::getKey(Key::Shift)) {
            speed *= sprintMultiplier;
        }
        
        float velocity = speed * dt;
        
        if (Input::getKey(Key::W)) {
            camera->position += front * velocity;
        }
        if (Input::getKey(Key::S)) {
            camera->position -= front * velocity;
        }
        if (Input::getKey(Key::A)) {
            camera->position -= right * velocity;
        }
        if (Input::getKey(Key::D)) {
            camera->position += right * velocity;
        }
        if (Input::getKey(Key::Space)) {
            camera->position += worldUp * velocity;
        }
        if (Input::getKey(Key::Ctrl)) {
            camera->position -= worldUp * velocity;
        }
        
        // Zoom with scroll wheel
        float scroll = Input::getScrollDelta();
        camera->fov -= scroll * 2.0f;
        if (camera->fov < 1.0f) camera->fov = 1.0f;
        if (camera->fov > 90.0f) camera->fov = 90.0f;
        
        // Update camera target
        camera->target = camera->position + front;
    }
    
private:
    void updateCameraVectors() {
        // Calculate new front vector
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(newFront);
        
        // Calculate right and up vectors
        right = glm::normalize(glm::cross(front, worldUp));
        camera->up = glm::normalize(glm::cross(right, front));
    }
};
