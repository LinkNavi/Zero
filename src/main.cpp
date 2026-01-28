// VMA implementation
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "Config.h"
#include "Engine.h"
#include "Renderer.h"
#include "PipelineLit.h"
#include "Mesh.h"
#include "Camera.h"
#include "Input.h"
#include "CameraController.h"
#include "Time.h"
#include <iostream>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Components
struct Transform : Component {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    
    Transform() : position(0.0f), rotation(0.0f), scale(1.0f) {}
    Transform(glm::vec3 p, glm::vec3 r, glm::vec3 s) 
        : position(p), rotation(r), scale(s) {}
    
    glm::mat4 getMatrix() const {
        glm::mat4 mat = glm::mat4(1.0f);
        mat = glm::translate(mat, position);
        mat = glm::rotate(mat, rotation.x, glm::vec3(1, 0, 0));
        mat = glm::rotate(mat, rotation.y, glm::vec3(0, 1, 0));
        mat = glm::rotate(mat, rotation.z, glm::vec3(0, 0, 1));
        mat = glm::scale(mat, scale);
        return mat;
    }
};

struct Velocity : Component {
    glm::vec3 linear;
    glm::vec3 angular;
    
    Velocity() : linear(0.0f), angular(0.0f) {}
    Velocity(glm::vec3 l, glm::vec3 a) : linear(l), angular(a) {}
};

struct MeshComponent : Component {
    LitMesh* mesh = nullptr;
};

// Forward declare ECS
class ECS;

// Systems
class RotationSystem : public System {
public:
    ECS* ecs;
    RotationSystem() : ecs(nullptr) {}
    void update(float dt) override;
};

class RenderSystem : public System {
public:
    ECS* ecs;
    VulkanRenderer* renderer;
    LitPipeline* pipeline;
    Camera* camera;
    uint32_t width, height;
    glm::vec3 lightDir;
    glm::vec3 lightColor;
    float ambientStrength;
    
    RenderSystem() : ecs(nullptr), renderer(nullptr), pipeline(nullptr), 
                     camera(nullptr), width(0), height(0),
                     lightDir(1, -1, 1), lightColor(1, 1, 1), 
                     ambientStrength(0.3f) {}
    
    void update(float /*dt*/) override;
};

// System implementations
void RotationSystem::update(float dt) {
    if (!ecs) {
        std::cerr << "RotationSystem: ECS pointer is null!" << std::endl;
        return;
    }
    
    for (EntityID entity : entities) {
        auto* transform = ecs->getComponent<Transform>(entity);
        auto* velocity = ecs->getComponent<Velocity>(entity);
        
        if (transform && velocity) {
            transform->position += velocity->linear * dt;
            transform->rotation += velocity->angular * dt;
        }
    }
}

void RenderSystem::update(float /*dt*/) {
    // Safety checks
    if (!renderer || !pipeline || !camera || !ecs) {
        std::cerr << "RenderSystem: null pointer detected!" << std::endl;
        return;
    }
    
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    renderer->beginFrame(cmd);
    
    if (cmd == VK_NULL_HANDLE) {
        std::cerr << "RenderSystem: Failed to get command buffer!" << std::endl;
        return;
    }
    
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(width);
    viewport.height = static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {width, height};
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    
    pipeline->bind(cmd);
    
    glm::mat4 viewProj = camera->getViewProjectionMatrix();
    
    for (EntityID entity : entities) {
        auto* transform = ecs->getComponent<Transform>(entity);
        auto* meshComp = ecs->getComponent<MeshComponent>(entity);
        
        if (transform && meshComp && meshComp->mesh) {
            glm::mat4 model = transform->getMatrix();
            
            PushConstantsLit push{};
            push.mvp = viewProj * model;
            push.model = model;
            push.lightDir = glm::normalize(lightDir);
            push.lightColor = lightColor;
            push.ambientStrength = ambientStrength;
            
            pipeline->pushConstants(cmd, push);
            meshComp->mesh->draw(cmd);
        }
    }
    
    renderer->endFrame(cmd);
}

int main() {
    // Workaround for GTK icon loading issues on some systems
    // This prevents the glycin/bwrap sandboxing error
    setenv("GLYCIN_USE_SANDBOX", "0", 1);
    setenv("GTK_DISABLE_VALIDATION", "1", 1);
    
    std::cout << "=== Zero Engine v0.3 - Enhanced Edition ===" << std::endl;
    
    // Load configuration
    Config config;
    if (!config.load("config.ini")) {
        std::cout << "! Using default configuration" << std::endl;
    } else {
        std::cout << "✓ Configuration loaded" << std::endl;
    }
    
    // Get settings from config
    uint32_t width = config.get("window_width", 1280);
    uint32_t height = config.get("window_height", 720);
    std::string title = config.getString("window_title", "Zero Engine");
    
    // Initialize Vulkan Renderer
    VulkanRenderer renderer;
    if (!renderer.init(width, height, title.c_str())) {
        std::cerr << "Failed to initialize renderer!" << std::endl;
        return -1;
    }
    std::cout << "✓ Renderer initialized" << std::endl;
    
    // Initialize Time
    Time::init();
    std::cout << "✓ Time system initialized" << std::endl;
    
    // Initialize input
    Input::init(renderer.getWindow());
    std::cout << "✓ Input system initialized" << std::endl;
    
    // Create lit pipeline
    LitPipeline pipeline;
    std::cout << "Loading lit shaders from:" << std::endl;
    std::cout << "  - shaders/vert_lit.spv" << std::endl;
    std::cout << "  - shaders/frag_lit.spv" << std::endl;
    
    if (!pipeline.init(renderer.getDevice(), renderer.getRenderPass(),
                       "shaders/vert_lit.spv", "shaders/frag_lit.spv")) {
        std::cerr << "\n=== SHADER LOADING FAILED ===" << std::endl;
        std::cerr << "The shaders could not be loaded. This usually means:" << std::endl;
        std::cerr << "1. Shader files don't exist" << std::endl;
        std::cerr << "2. You're not running from the project root directory" << std::endl;
        std::cerr << "3. Shaders haven't been compiled" << std::endl;
        std::cerr << "\nTo fix this:" << std::endl;
        std::cerr << "  1. Make sure you're in the project root directory" << std::endl;
        std::cerr << "  2. Run: bash compile_shaders.sh" << std::endl;
        std::cerr << "  3. Run the program again from the project root" << std::endl;
        std::cerr << "\nSee TROUBLESHOOTING.md for more details." << std::endl;
        renderer.cleanup();
        return -1;
    }
    std::cout << "✓ Lit graphics pipeline created" << std::endl;
    
    // Create cube mesh
    LitMesh cubeMesh;
    if (!cubeMesh.createCube(&renderer)) {
        std::cerr << "Failed to create cube mesh!" << std::endl;
        return -1;
    }
    std::cout << "✓ Cube mesh created" << std::endl;
    
    // Create camera
    Camera camera;
    camera.position = config.getVec3("camera_position", glm::vec3(0, 2, 8));
    camera.target = glm::vec3(0);
    camera.aspectRatio = (float)width / (float)height;
    camera.fov = config.get("camera_fov", 45.0f);
    camera.nearPlane = config.get("camera_near", 0.1f);
    camera.farPlane = config.get("camera_far", 100.0f);
    
    CameraController cameraController(&camera);
    cameraController.moveSpeed = config.get("camera_speed", 5.0f);
    cameraController.sprintMultiplier = config.get("camera_sprint_multiplier", 2.0f);
    cameraController.mouseSensitivity = config.get("camera_sensitivity", 0.15f);
    
    std::cout << "✓ Camera system initialized" << std::endl;
    
    // Initialize ECS
    ECS ecs;
    ecs.registerComponent<Transform>();
    ecs.registerComponent<Velocity>();
    ecs.registerComponent<MeshComponent>();
    std::cout << "✓ ECS initialized (3 components registered)" << std::endl;
    
    // Create systems
    auto rotationSystem = ecs.registerSystem<RotationSystem>();
    rotationSystem->ecs = &ecs;
    ComponentMask rotationMask;
    rotationMask.set(0); // Transform
    rotationMask.set(1); // Velocity
    ecs.setSystemSignature<RotationSystem>(rotationMask);
    std::cout << "✓ Rotation system registered" << std::endl;
    
    auto renderSystem = ecs.registerSystem<RenderSystem>();
    renderSystem->ecs = &ecs;
    renderSystem->renderer = &renderer;
    renderSystem->pipeline = &pipeline;
    renderSystem->camera = &camera;
    renderSystem->width = width;
    renderSystem->height = height;
    renderSystem->lightDir = config.getVec3("light_direction", glm::vec3(1, -1, 1));
    renderSystem->lightColor = config.getVec3("light_color", glm::vec3(1));
    renderSystem->ambientStrength = config.get("ambient_strength", 0.3f);
    
    ComponentMask renderMask;
    renderMask.set(0); // Transform
    renderMask.set(2); // MeshComponent
    ecs.setSystemSignature<RenderSystem>(renderMask);
    std::cout << "✓ Render system registered" << std::endl;
    
    // Create a grid of cubes
    std::cout << "Creating entity grid..." << std::endl;
    int gridSize = config.get("grid_size", 5);
    float spacing = config.get("grid_spacing", 2.5f);
    
    for (int x = -gridSize; x <= gridSize; x++) {
        for (int z = -gridSize; z <= gridSize; z++) {
            EntityID cube = ecs.createEntity();
            float height = (x + z) * 0.2f;
            ecs.addComponent(cube, Transform(
                glm::vec3(x * spacing, height, z * spacing), 
                glm::vec3(0.0f), 
                glm::vec3(0.8f)
            ));
            ecs.addComponent(cube, Velocity(
                glm::vec3(0.0f), 
                glm::vec3(0.3f * x * 0.1f, 0.5f + z * 0.1f, 0.2f)
            ));
            MeshComponent mesh; 
            mesh.mesh = &cubeMesh;
            ecs.addComponent(cube, mesh);
        }
    }
    
    std::cout << "✓ Created " << ((gridSize * 2 + 1) * (gridSize * 2 + 1)) << " cubes\n" << std::endl;
    std::cout << "\n=== Controls ===" << std::endl;
    std::cout << "  WASD       - Move camera" << std::endl;
    std::cout << "  Space/Ctrl - Up/Down" << std::endl;
    std::cout << "  Shift      - Sprint" << std::endl;
    std::cout << "  Right Click - Toggle mouse look" << std::endl;
    std::cout << "  Scroll     - Zoom" << std::endl;
    std::cout << "  ESC        - Exit" << std::endl;
    std::cout << "\n=== Starting render loop ===" << std::endl;
    
    // Performance settings
    int maxFPS = config.get("max_fps", 0);
    float targetFrameTime = (maxFPS > 0) ? (1.0f / maxFPS) : 0.0f;
    
    // Main loop
    int frameCount = 0;
    float fpsTimer = 0.0f;
    bool showFPS = config.get("show_fps", 1);
    
    while (!renderer.shouldClose() && !Input::getKey(Key::Escape)) {
        Time::update();
        float dt = Time::getDeltaTime();
        
        // FPS limiting
        if (targetFrameTime > 0.0f) {
            while (Time::getRealDeltaTime() < targetFrameTime) {
                // Busy wait
            }
        }
        
        // Update input
        Input::update();
        renderer.pollEvents();
        
        // Update camera
        cameraController.update(dt, renderer.getWindow());
        
        // Update systems
        ecs.updateSystems(dt);
        
        // FPS counter
        if (showFPS) {
            frameCount++;
            fpsTimer += dt;
            if (fpsTimer >= 1.0f) {
                std::cout << "FPS: " << frameCount
                          << " | Frame time: " << (1000.0f/frameCount) << "ms"
                          << " | Pos: (" << (int)camera.position.x << ", "
                          << (int)camera.position.y << ", " << (int)camera.position.z << ")"
                          << std::endl;
                frameCount = 0;
                fpsTimer = 0.0f;
            }
        }
    }
    
    std::cout << "\n=== Shutting down ===" << std::endl;
    
    // Save config if changed
    config.save("config.ini");
    
    cubeMesh.cleanup();
    pipeline.cleanup();
    renderer.cleanup();
    
    std::cout << "✓ Clean shutdown complete" << std::endl;
    
    return 0;
}
