// VMA implementation
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "Engine.h"
#include "Renderer.h"
#include "PipelineLit.h"  // CHANGED: Use LitPipeline instead of GraphicsPipeline
#include "Mesh.h"
#include "Camera.h"
#include "Input.h"
#include "CameraController.h"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

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
class PhysicsSystem : public System {
public:
    ECS* ecs;
    PhysicsSystem() : ecs(nullptr) {}
    void update(float dt) override;
};

class RenderSystem : public System {
public:
    ECS* ecs;
    VulkanRenderer* renderer;
    LitPipeline* pipeline;  // CHANGED: Use LitPipeline
    Camera* camera;
    uint32_t width, height;
    
    RenderSystem() : ecs(nullptr), renderer(nullptr), pipeline(nullptr), camera(nullptr), width(0), height(0) {}
    void update(float dt) override;
};

// System implementations
void PhysicsSystem::update(float dt) {
    for (EntityID entity : entities) {
        auto* transform = ecs->getComponent<Transform>(entity);
        auto* velocity = ecs->getComponent<Velocity>(entity);
        
        if (transform && velocity) {
            transform->position += velocity->linear * dt;
            transform->rotation += velocity->angular * dt;
        }
    }
}

void RenderSystem::update(float dt) {
    VkCommandBuffer cmd;
    renderer->beginFrame(cmd);
    
    // Set dynamic viewport and scissor
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
    
    // Bind pipeline
    pipeline->bind(cmd);
    
    // Render entities
    glm::mat4 viewProj = camera->getViewProjectionMatrix();
    
    for (EntityID entity : entities) {
        auto* transform = ecs->getComponent<Transform>(entity);
        auto* meshComp = ecs->getComponent<MeshComponent>(entity);
        
        if (transform && meshComp && meshComp->mesh) {
            // Calculate matrices
            glm::mat4 model = transform->getMatrix();
            
            // CHANGED: Use PushConstantsLit with lighting info
            PushConstantsLit push{};
            push.mvp = viewProj * model;
            push.model = model;
            push.lightDir = glm::normalize(glm::vec3(1.0f, -1.0f, 1.0f)); // Light from top-right
            push.lightColor = glm::vec3(1.0f, 1.0f, 1.0f); // White light
            push.ambientStrength = 0.3f; // 30% ambient light
            
            // Push constants and draw
            pipeline->pushConstants(cmd, push);
            meshComp->mesh->draw(cmd);
        }
    }
    
    renderer->endFrame(cmd);
}

int main() {
    std::cout << "=== Zero Engine v0.2 - With Lighting! ===" << std::endl;
    
    // Initialize Vulkan Renderer
    VulkanRenderer renderer;
    if (!renderer.init(1280, 720, "Zero Engine")) {
        std::cerr << "Failed to initialize renderer!" << std::endl;
        return -1;
    }
    std::cout << "✓ Renderer initialized\n" << std::endl;
    
    // Initialize input system
    Input::init(renderer.getWindow());
    std::cout << "✓ Input system initialized" << std::endl;
    
    // CHANGED: Create LitPipeline for lighting
    std::cout << "Loading lit graphics pipeline..." << std::endl;
    LitPipeline pipeline;
    if (!pipeline.init(renderer.getDevice(), renderer.getRenderPass(), 
                       "shaders/vert_lit.spv", "shaders/frag_lit.spv")) {
        std::cerr << "Failed to create graphics pipeline!" << std::endl;
        std::cerr << "Make sure to compile lit shaders:" << std::endl;
        std::cerr << "  glslc shader_lit.vert -o shaders/vert_lit.spv" << std::endl;
        std::cerr << "  glslc shader_lit.frag -o shaders/frag_lit.spv" << std::endl;
        return -1;
    }
    std::cout << "✓ Lit graphics pipeline created\n" << std::endl;
    
    // Create cube mesh
    std::cout << "Creating cube mesh..." << std::endl;
    LitMesh cubeMesh;
    if (!cubeMesh.createCube(&renderer)) {
        std::cerr << "Failed to create cube mesh!" << std::endl;
        return -1;
    }
    std::cout << "✓ Cube mesh created\n" << std::endl;
    
    // Create camera with controller
    Camera camera;
    camera.position = glm::vec3(0.0f, 2.0f, 8.0f);
    camera.target = glm::vec3(0.0f, 0.0f, 0.0f);
    camera.aspectRatio = 1280.0f / 720.0f;
    
    CameraController cameraController(&camera);
    cameraController.moveSpeed = 3.0f;
    cameraController.mouseSensitivity = 0.15f;
    
    std::cout << "✓ Camera system initialized\n" << std::endl;
    
    // Initialize ECS
    ECS ecs;
    ecs.registerComponent<Transform>();
    ecs.registerComponent<Velocity>();
    ecs.registerComponent<MeshComponent>();
    std::cout << "✓ ECS initialized\n" << std::endl;
    
    // Create systems
    auto physicsSystem = ecs.registerSystem<PhysicsSystem>();
    physicsSystem->ecs = &ecs;
    ComponentMask physicsMask;
    physicsMask.set(0); // Transform
    physicsMask.set(1); // Velocity
    ecs.setSystemSignature<PhysicsSystem>(physicsMask);
    
    auto renderSystem = ecs.registerSystem<RenderSystem>();
    renderSystem->ecs = &ecs;
    renderSystem->renderer = &renderer;
    renderSystem->pipeline = &pipeline;
    renderSystem->camera = &camera;
    renderSystem->width = 1280;
    renderSystem->height = 720;
    ComponentMask renderMask;
    renderMask.set(0); // Transform
    renderMask.set(2); // MeshComponent
    ecs.setSystemSignature<RenderSystem>(renderMask);
    std::cout << "✓ Systems registered\n" << std::endl;
    
    // Create a grid of cubes
    std::cout << "Creating entity grid..." << std::endl;
    for (int x = -5; x <= 5; x++) {
        for (int z = -5; z <= 5; z++) {
            EntityID cube = ecs.createEntity();
            float height = (x + z) * 0.2f;
            ecs.addComponent(cube, Transform(
                glm::vec3(x * 2.5f, height, z * 2.5f), 
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
    
    std::cout << "✓ Created grid of cubes\n" << std::endl;
    std::cout << "\n=== Controls ===" << std::endl;
    std::cout << "  WASD       - Move camera" << std::endl;
    std::cout << "  Space/Ctrl - Up/Down" << std::endl;
    std::cout << "  Shift      - Sprint" << std::endl;
    std::cout << "  Right Click - Toggle mouse look" << std::endl;
    std::cout << "  Scroll     - Zoom" << std::endl;
    std::cout << "  ESC        - Exit" << std::endl;
    std::cout << "\n=== Starting render loop ===\n" << std::endl;
    
    // Main loop
    auto lastTime = std::chrono::high_resolution_clock::now();
    float frameCount = 0;
    float fpsTimer = 0.0f;
    
    while (!renderer.shouldClose() && !Input::getKey(Key::Escape)) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        // Cap delta time to prevent huge jumps
        if (dt > 0.1f) dt = 0.1f;
        
        // Update input
        Input::update();
        renderer.pollEvents();
        
        // Update camera
        cameraController.update(dt, renderer.getWindow());
        
        // FPS counter
        frameCount++;
        fpsTimer += dt;
        if (fpsTimer >= 1.0f) {
            std::cout << "FPS: " << (int)frameCount 
                      << " | Frame time: " << (1000.0f/frameCount) << "ms"
                      << " | Pos: (" << (int)camera.position.x << ", " 
                      << (int)camera.position.y << ", " << (int)camera.position.z << ")"
                      << std::endl;
            frameCount = 0;
            fpsTimer = 0.0f;
        }
        
        // Update systems
        ecs.updateSystems(dt);
    }
    
    std::cout << "\n=== Shutting down ===" << std::endl;
    cubeMesh.cleanup();
    pipeline.cleanup();
    renderer.cleanup();
    std::cout << "✓ Clean shutdown complete" << std::endl;
    
    return 0;
}
