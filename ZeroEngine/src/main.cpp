#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Renderer.h"
#include <iostream>
#include <cmath>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Global state
struct AppState {
    Renderer renderer;
    Mesh cube;
    GLuint shader;
    float angle = 0.0f;
    int width = WINDOW_WIDTH;
    int height = WINDOW_HEIGHT;
} state;

// Callbacks
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    (void)window;
    state.width = width;
    state.height = height;
    state.renderer.SetViewport(width, height);
}

void errorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;
    
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

// Matrix math helpers
void mat4Identity(float* m) {
    for (int i = 0; i < 16; i++) m[i] = 0.0f;
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

void mat4Perspective(float* m, float fov, float aspect, float nearPlane, float farPlane) {
    float f = 1.0f / tanf(fov / 2.0f);
    mat4Identity(m);
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
    m[11] = -1.0f;
    m[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
    m[15] = 0.0f;
}

void mat4LookAt(float* m, float eyeX, float eyeY, float eyeZ) {
    mat4Identity(m);
    m[12] = -eyeX;
    m[13] = -eyeY;
    m[14] = -eyeZ;
}

void mat4RotateY(float* m, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    mat4Identity(m);
    m[0] = c;
    m[2] = s;
    m[8] = -s;
    m[10] = c;
}

void mat4Multiply(const float* a, const float* b, float* out) {
    float temp[16];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            temp[i * 4 + j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                temp[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
            }
        }
    }
    for (int i = 0; i < 16; i++) out[i] = temp[i];
}

void createMVP(float* mvp, float angle, float aspect) {
    float proj[16], view[16], model[16], temp[16];
    
    mat4Perspective(proj, 3.14159f / 3.0f, aspect, 0.1f, 100.0f);
    mat4LookAt(view, 0.0f, 0.0f, 5.0f);
    mat4RotateY(model, angle);
    
    mat4Multiply(view, model, temp);
    mat4Multiply(proj, temp, mvp);
}

int main() {
    // Set error callback
    glfwSetErrorCallback(errorCallback);
    
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, 
                                          "ZeroEngine - OpenGL", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    
    // Load OpenGL function pointers with GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    // Enable VSync
    glfwSwapInterval(1);
    
    // Initialize renderer
    if (!state.renderer.Init()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    // Create resources
    state.cube = state.renderer.CreateCube();
    state.shader = state.renderer.CreateDefaultShader();
    
    if (state.shader == 0) {
        std::cerr << "Failed to create shader" << std::endl;
        state.renderer.Shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    std::cout << "\n==================================" << std::endl;
    std::cout << "ZeroEngine initialized successfully!" << std::endl;
    std::cout << "Window: " << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << std::endl;
    std::cout << "==================================\n" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "==================================\n" << std::endl;
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Update
        state.angle += 0.01f;
        
        // Render
        state.renderer.BeginFrame();
        
        // Calculate MVP matrix
        float aspect = (float)state.width / (float)state.height;
        float mvp[16];
        createMVP(mvp, state.angle, aspect);
        
        // Draw cube
        state.renderer.DrawMesh(state.cube, state.shader, mvp);
        
        state.renderer.EndFrame();
        
        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    std::cout << "Shutting down..." << std::endl;
    state.renderer.DestroyMesh(state.cube);
    state.renderer.DestroyShader(state.shader);
    state.renderer.Shutdown();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    std::cout << "Goodbye!" << std::endl;
    return 0;
}
