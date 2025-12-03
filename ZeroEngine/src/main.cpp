#include <GLFW/glfw3.h>
#include "Renderer.h"
#include "ECS.h"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <random>

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

// Components for our game objects
struct Transform {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    
    Transform() : position(0.0f), rotation(0.0f), scale(1.0f) {}
};

struct MeshRenderer {
    Mesh mesh;
    Shader shader;
    
    MeshRenderer() : mesh{}, shader(VK_NULL_HANDLE) {}
};

struct Rotator {
    glm::vec3 rotationSpeed;
    
    Rotator() : rotationSpeed(0.0f) {}
    Rotator(glm::vec3 speed) : rotationSpeed(speed) {}
};

// Global state
struct AppState {
    Renderer renderer;
    ECS ecs;
    Mesh cubeMesh;
    Shader defaultShader;
    float time = 0.0f;
    int width = WINDOW_WIDTH;
    int height = WINDOW_HEIGHT;
    bool framebufferResized = false;
} state;

// Systems
class RotationSystem : public System {
public:
    ECS* ecs;
    
    void Update(float deltaTime) override {
        for (auto entity : mEntities) {
            if (!ecs->HasComponent<Transform>(entity) || !ecs->HasComponent<Rotator>(entity)) {
                continue;
            }
            
            auto& transform = ecs->GetComponent<Transform>(entity);
            auto& rotator = ecs->GetComponent<Rotator>(entity);
            
            transform.rotation += rotator.rotationSpeed * deltaTime;
        }
    }
};

class RenderSystem : public System {
public:
    Renderer* renderer;
    ECS* ecs;
    
    void Update(float deltaTime) override {
        (void)deltaTime;
        
        for (auto entity : mEntities) {
            if (!ecs->HasComponent<Transform>(entity) || 
                !ecs->HasComponent<MeshRenderer>(entity)) {
                continue;
            }
            
            auto& transform = ecs->GetComponent<Transform>(entity);
            auto& meshRenderer = ecs->GetComponent<MeshRenderer>(entity);
            
            // Build model matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, transform.position);
            model = glm::rotate(model, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, transform.scale);
            
            renderer->DrawMesh(meshRenderer.mesh, meshRenderer.shader, model);
        }
    }
};

// Callbacks
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    (void)window;
    state.width = width;
    state.height = height;
    state.framebufferResized = true;
    state.renderer.SetViewport(width, height);
    
    // Update projection matrix
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 
                                     (float)width / (float)height, 
                                     0.1f, 100.0f);
    proj[1][1] *= -1; // Flip Y for Vulkan
    state.renderer.SetProjectionMatrix(proj);
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
    
    // Spawn a new cube on SPACE
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        static std::default_random_engine rng;
        static std::uniform_real_distribution<float> posDist(-5.0f, 5.0f);
        static std::uniform_real_distribution<float> speedDist(-2.0f, 2.0f);
        static std::uniform_real_distribution<float> scaleDist(0.3f, 1.2f);
        
        Entity cube = state.ecs.CreateEntity();
        
        Transform transform;
        transform.position = glm::vec3(posDist(rng), posDist(rng), posDist(rng));
        transform.scale = glm::vec3(scaleDist(rng));
        state.ecs.AddComponent(cube, transform);
        
        MeshRenderer meshRenderer;
        meshRenderer.mesh = state.cubeMesh;
        meshRenderer.shader = state.defaultShader;
        state.ecs.AddComponent(cube, meshRenderer);
        
        Rotator rotator(glm::vec3(speedDist(rng), speedDist(rng), speedDist(rng)));
        state.ecs.AddComponent(cube, rotator);
        
        state.ecs.UpdateSystemEntities<RotationSystem, Transform, Rotator>(cube);
        state.ecs.UpdateSystemEntities<RenderSystem, Transform, MeshRenderer>(cube);
        
        std::cout << "Spawned cube! Total entities: " 
                  << state.ecs.GetSystem<RenderSystem>()->mEntities.size() << std::endl;
    }
}

Entity CreateCube(glm::vec3 position, glm::vec3 rotationSpeed, glm::vec3 scale = glm::vec3(1.0f)) {
    Entity cube = state.ecs.CreateEntity();
    
    Transform transform;
    transform.position = position;
    transform.scale = scale;
    state.ecs.AddComponent(cube, transform);
    
    MeshRenderer meshRenderer;
    meshRenderer.mesh = state.cubeMesh;
    meshRenderer.shader = state.defaultShader;
    state.ecs.AddComponent(cube, meshRenderer);
    
    Rotator rotator(rotationSpeed);
    state.ecs.AddComponent(cube, rotator);
    
    state.ecs.UpdateSystemEntities<RotationSystem, Transform, Rotator>(cube);
    state.ecs.UpdateSystemEntities<RenderSystem, Transform, MeshRenderer>(cube);
    
    return cube;
}

int main() {
    // Set error callback
    glfwSetErrorCallback(errorCallback);
    
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Configure GLFW for Vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, 
                                          "ZeroEngine - Vulkan (Multiple Cubes)", 
                                          nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    
    // Initialize renderer
    if (!state.renderer.Init(window)) {
        std::cerr << "Failed to initialize Vulkan renderer" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    // Initialize ECS
    state.ecs.Init();
    
    // Register components
    state.ecs.RegisterComponent<Transform>();
    state.ecs.RegisterComponent<MeshRenderer>();
    state.ecs.RegisterComponent<Rotator>();
    
    // Register systems
    auto rotationSystem = state.ecs.RegisterSystem<RotationSystem>();
    rotationSystem->ecs = &state.ecs;
    
    auto renderSystem = state.ecs.RegisterSystem<RenderSystem>();
    renderSystem->renderer = &state.renderer;
    renderSystem->ecs = &state.ecs;
    
    // Create shared resources
    state.cubeMesh = state.renderer.CreateCube();
    state.defaultShader = state.renderer.CreateDefaultShader();
    
    if (state.defaultShader == VK_NULL_HANDLE) {
        std::cerr << "Failed to create shader" << std::endl;
        state.renderer.DestroyMesh(state.cubeMesh);
        state.renderer.Shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    // Setup camera
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 3.0f, 10.0f), 
                                 glm::vec3(0.0f, 0.0f, 0.0f), 
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    state.renderer.SetViewMatrix(view);
    
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 
                                     (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 
                                     0.1f, 100.0f);
    proj[1][1] *= -1; // Flip Y for Vulkan
    state.renderer.SetProjectionMatrix(proj);
    
    // Create initial cubes in a grid
    std::cout << "\n==================================" << std::endl;
    std::cout << "ZeroEngine - Vulkan Edition" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Creating initial cubes..." << std::endl;
    
    // Create a 3x3 grid of cubes
    for (int x = -1; x <= 1; x++) {
        for (int z = -1; z <= 1; z++) {
            glm::vec3 pos(x * 2.5f, 0.0f, z * 2.5f);
            glm::vec3 rotSpeed(
                (x + 1) * 0.5f,
                (z + 1) * 0.5f,
                (x * z) * 0.3f
            );
            CreateCube(pos, rotSpeed, glm::vec3(0.8f));
        }
    }
    
    std::cout << "Created " << renderSystem->mEntities.size() << " cubes" << std::endl;
    std::cout << "\n==================================" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  SPACE - Spawn random cube" << std::endl;
    std::cout << "  ESC   - Exit" << std::endl;
    std::cout << "==================================\n" << std::endl;
    
    // Timing
    double lastTime = glfwGetTime();
    double currentTime;
    float deltaTime;
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        currentTime = glfwGetTime();
        deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;
        state.time += deltaTime;
        
        // Update systems
        rotationSystem->Update(deltaTime);
        
        // Render
        state.renderer.BeginFrame();
        
        if (state.renderer.IsFrameInProgress()) {
            renderSystem->Update(deltaTime);
        }
        
        state.renderer.EndFrame();
        
        // Poll events
        glfwPollEvents();
    }
    
    // Cleanup
    std::cout << "Shutting down..." << std::endl;
    
    // Wait for device to finish
    state.renderer.DestroyMesh(state.cubeMesh);
    state.renderer.DestroyShader(state.defaultShader);
    state.renderer.Shutdown();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    std::cout << "Goodbye!" << std::endl;
    return 0;
}
