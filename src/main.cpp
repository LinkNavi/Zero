// VMA implementation
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "Camera.h"
#include "CameraController.h"
#include "Config.h"
#include "PipelineGLB.h"  // CHANGED: Was PipelineLit.h
#include "Engine.h"
#include "GLBLoader.h"
#include "Input.h"
#include "Renderer.h"
#include "Time.h"
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

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

struct GLBComponent : Component {
    GLBModel* model = nullptr;
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

class GLBRenderSystem : public System {
public:
    ECS* ecs;
    VulkanRenderer* renderer;
    GLBPipeline* pipeline;  // CHANGED: Was LitPipeline*
    Camera* camera;
    uint32_t width, height;
    glm::vec3 lightDir;
    glm::vec3 lightColor;
    float ambientStrength;

    GLBRenderSystem()
        : ecs(nullptr), renderer(nullptr), pipeline(nullptr),
          camera(nullptr), width(0), height(0), lightDir(1, -1, 1),
          lightColor(1, 1, 1), ambientStrength(0.3f) {}

    void update(float /*dt*/) override;
};

void GLBRenderSystem::update(float /*dt*/) {
    if (!renderer || !camera || !ecs || !pipeline) {
        std::cerr << "GLBRenderSystem: null pointer detected!" << std::endl;
        return;
    }

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    renderer->beginFrame(cmd);

    if (cmd == VK_NULL_HANDLE) {
        std::cerr << "GLBRenderSystem: Failed to get command buffer!" << std::endl;
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
        auto* glbComp = ecs->getComponent<GLBComponent>(entity);

        if (transform && glbComp && glbComp->model) {
            glm::mat4 model = transform->getMatrix();

            // Get material properties from the first material in the model
            glm::vec4 materialColor = glm::vec4(0.5f, 0.7f, 0.3f, 1.0f);  // Default tree green
            float metallic = 0.0f;
            float roughness = 0.8f;
            
            if (!glbComp->model->materials.empty()) {
                const auto& mat = glbComp->model->materials[0];
                materialColor = mat.baseColor;
                metallic = mat.metallic;
                roughness = mat.roughness;
            }

            PushConstantsGLB push{};
            push.mvp = viewProj * model;
            push.model = model;
            push.lightDir = glm::normalize(lightDir);
            push.lightColor = lightColor;
            push.ambientStrength = ambientStrength;
            push.materialColor = materialColor;
            push.materialMetallic = metallic;
            push.materialRoughness = roughness;

            pipeline->pushConstants(cmd, push);
            glbComp->model->draw(cmd);
        }
    }

    renderer->endFrame(cmd);
}

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

int main() {
    setenv("GLYCIN_USE_SANDBOX", "0", 1);
    setenv("GTK_DISABLE_VALIDATION", "1", 1);

    std::cout << "=== Zero Engine v0.4 - GLB Model Loading (Fixed) ===" << std::endl;

    // Load configuration
    Config config;
    if (!config.load("config.ini")) {
        std::cout << "! Using default configuration" << std::endl;
    } else {
        std::cout << "✓ Configuration loaded" << std::endl;
    }

    uint32_t width = config.get("window_width", 960);   // CHANGED: Default reduced
    uint32_t height = config.get("window_height", 540);  // CHANGED: Default reduced
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

    // Load GLB model
    GLBModel treeModel;
    if (!treeModel.load("models/tree.glb", renderer.getAllocator(),
                        renderer.getDevice(), renderer.getCommandPool(),
                        renderer.getGraphicsQueue())) {
        std::cerr << "Failed to load GLB model!" << std::endl;
        renderer.cleanup();
        return -1;
    }
    std::cout << "✓ GLB model loaded" << std::endl;

    // CHANGED: Create GLB-specific pipeline
    GLBPipeline glbPipeline;
    std::cout << "Loading GLB shaders..." << std::endl;
    if (!glbPipeline.init(renderer.getDevice(), renderer.getRenderPass(),
                         "shaders/vert_glb.spv", "shaders/frag_glb.spv")) {
        std::cerr << "Failed to create GLB pipeline!" << std::endl;
        std::cerr << "Did you compile the shaders? Run:" << std::endl;
        std::cerr << "  glslc shader_glb.vert -o shaders/vert_glb.spv" << std::endl;
        std::cerr << "  glslc shader_glb.frag -o shaders/frag_glb.spv" << std::endl;
        renderer.cleanup();
        return -1;
    }
    std::cout << "✓ GLB graphics pipeline created" << std::endl;

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
    ecs.registerComponent<GLBComponent>();
    std::cout << "✓ ECS initialized (3 components registered)" << std::endl;

    // Create rotation system
    auto rotationSystem = ecs.registerSystem<RotationSystem>();
    rotationSystem->ecs = &ecs;
    ComponentMask rotationMask;
    rotationMask.set(0); // Transform
    rotationMask.set(1); // Velocity
    ecs.setSystemSignature<RotationSystem>(rotationMask);
    std::cout << "✓ Rotation system registered" << std::endl;

    // Create GLB render system
    auto renderSystem = ecs.registerSystem<GLBRenderSystem>();
    renderSystem->ecs = &ecs;
    renderSystem->renderer = &renderer;
    renderSystem->camera = &camera;
    renderSystem->pipeline = &glbPipeline;
    renderSystem->width = width;
    renderSystem->height = height;
    renderSystem->lightDir = config.getVec3("light_direction", glm::vec3(1, -1, 1));
    renderSystem->lightColor = config.getVec3("light_color", glm::vec3(1));
    renderSystem->ambientStrength = config.get("ambient_strength", 0.5f);  // CHANGED: Higher default

    ComponentMask renderMask;
    renderMask.set(0); // Transform
    renderMask.set(2); // GLBComponent
    ecs.setSystemSignature<GLBRenderSystem>(renderMask);
    std::cout << "✓ GLB Render system registered" << std::endl;

    // Create a grid of trees - OPTIMIZED FOR CHROMEBOOK
    std::cout << "Creating tree grid..." << std::endl;
    int gridSize = config.get("grid_size", 2);  // CHANGED: Default reduced from 5 to 2
    float spacing = config.get("grid_spacing", 2.5f);

    for (int x = -gridSize; x <= gridSize; x++) {
        for (int z = -gridSize; z <= gridSize; z++) {
            EntityID tree = ecs.createEntity();
            float height = (x + z) * 0.2f;
            
            ecs.addComponent(tree,
                Transform(glm::vec3(x * spacing, height, z * spacing),
                         glm::vec3(0.0f), glm::vec3(1.0f)));
            
            ecs.addComponent(tree,
                Velocity(glm::vec3(0.0f),
                        glm::vec3(0.3f * x * 0.1f, 0.5f + z * 0.1f, 0.2f)));
            
            GLBComponent glbComp;
            glbComp.model = &treeModel;
            ecs.addComponent(tree, glbComp);
        }
    }

    std::cout << "✓ Created " << ((gridSize * 2 + 1) * (gridSize * 2 + 1))
              << " trees\n" << std::endl;

    std::cout << "\n=== Controls ===" << std::endl;
    std::cout << "  WASD       - Move camera" << std::endl;
    std::cout << "  Space/Ctrl - Up/Down" << std::endl;
    std::cout << "  Shift      - Sprint" << std::endl;
    std::cout << "  Right Click - Toggle mouse look" << std::endl;
    std::cout << "  Scroll     - Zoom" << std::endl;
    std::cout << "  ESC        - Exit" << std::endl;
    std::cout << "\n=== Starting render loop ===" << std::endl;

    // Performance settings
    int maxFPS = config.get("max_fps", 30);  // CHANGED: Default 30 FPS for Chromebook
    float targetFrameTime = (maxFPS > 0) ? (1.0f / maxFPS) : 0.0f;

    // Main loop
    int frameCount = 0;
    float fpsTimer = 0.0f;
    bool showFPS = config.get("show_fps", 1);

    while (!renderer.shouldClose() && !Input::getKey(Key::Escape)) {
        Time::update();
        float dt = Time::getDeltaTime();

        if (targetFrameTime > 0.0f) {
            while (Time::getRealDeltaTime() < targetFrameTime) {
                // Busy wait
            }
        }

        Input::update();
        renderer.pollEvents();

        cameraController.update(dt, renderer.getWindow());

        ecs.updateSystems(dt);

        if (showFPS) {
            frameCount++;
            fpsTimer += dt;
            if (fpsTimer >= 1.0f) {
                std::cout << "FPS: " << frameCount
                          << " | Frame time: " << (1000.0f / frameCount) << "ms"
                          << " | Pos: (" << (int)camera.position.x << ", "
                          << (int)camera.position.y << ", " << (int)camera.position.z
                          << ")" << std::endl;
                frameCount = 0;
                fpsTimer = 0.0f;
            }
        }
    }

    std::cout << "\n=== Shutting down ===" << std::endl;

    config.save("config.ini");

    treeModel.cleanup();
    glbPipeline.cleanup();
    renderer.cleanup();

    std::cout << "✓ Clean shutdown complete" << std::endl;

    return 0;
}
