// VMA implementation
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "Camera.h"
#include "CameraController.h"
#include "Config.h"
#include "ResourcePath.h"
#include "PipelineGLB.h"
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
    GLBPipeline* pipeline;
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
            glm::mat4 modelMatrix = transform->getMatrix();

            for (size_t i = 0; i < glbComp->model->meshes.size(); i++) {
                const auto& mesh = glbComp->model->meshes[i];
                
                glm::vec4 materialColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
                float metallic = 0.0f;
                float roughness = 0.8f;
                int hasTexture = 0;
                
                if (mesh.materialIndex >= 0 && 
                    mesh.materialIndex < glbComp->model->materials.size()) {
                    const auto& mat = glbComp->model->materials[mesh.materialIndex];
                    materialColor = mat.baseColor;
                    metallic = mat.metallic;
                    roughness = mat.roughness;
                    
                    if (mat.baseColorTexture >= 0 && 
                        mat.baseColorTexture < glbComp->model->textures.size()) {
                        hasTexture = 1;
                    }
                }

                pipeline->bindDescriptorSet(cmd, mesh.descriptorSet);

                PushConstantsGLB push{};
                push.mvp = viewProj * modelMatrix;
                push.model = modelMatrix;
                push.lightDir = glm::normalize(lightDir);
                push.lightColor = lightColor;
                push.ambientStrength = ambientStrength;
                push.materialColor = materialColor;
                push.materialMetallic = metallic;
                push.materialRoughness = roughness;
                push.hasBaseColorTexture = hasTexture;

                pipeline->pushConstants(cmd, push);
                
                VkBuffer vertexBuffers[] = {mesh.vertexBuffer};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(cmd, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(cmd, mesh.indexCount, 1, 0, 0, 0);
            }
        }
    }

    renderer->endFrame(cmd);
}

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

    std::cout << "=== Zero Engine v0.6 - FIXED ===" << std::endl;
    std::cout << "Bug Fix: Removed command buffer reuse that caused smearing" << std::endl;
    std::cout << "Optimizations: Double buffering, batch texture loading" << std::endl;
    std::cout << std::endl;

    Config config;
    if (!config.load("config.ini")) {
        std::cout << "! Using default configuration" << std::endl;
    } else {
        std::cout << "✓ Configuration loaded" << std::endl;
    }

    uint32_t width = config.get("window_width", 960);
    uint32_t height = config.get("window_height", 540);
    std::string title = config.getString("window_title", "Zero Engine - FIXED");

    VulkanRenderer renderer;
    if (!renderer.init(width, height, title.c_str())) {
        std::cerr << "Failed to initialize renderer!" << std::endl;
        return -1;
    }
    std::cout << "✓ Renderer initialized (double buffered)" << std::endl;

    Time::init();
    std::cout << "✓ Time system initialized" << std::endl;

    Input::init(renderer.getWindow());
    std::cout << "✓ Input system initialized" << std::endl;

    GLBPipeline glbPipeline;
    std::cout << "Loading GLB shaders..." << std::endl;
    if (!glbPipeline.init(renderer.getDevice(), renderer.getRenderPass(),
                         "shaders/vert_glb.spv", "shaders/frag_glb.spv")) {
        std::cerr << "Failed to create GLB pipeline!" << std::endl;
        renderer.cleanup();
        return -1;
    }
    std::cout << "✓ GLB graphics pipeline created" << std::endl;

    std::string modelPath = ResourcePath::models(
        config.getString("model_name", "Duck.glb")
    );
    std::cout << "  Model path: " << modelPath << std::endl;
    
    auto loadStart = std::chrono::high_resolution_clock::now();
    
    GLBModel treeModel;
    if (!treeModel.load(modelPath, renderer.getAllocator(),
                        renderer.getDevice(), renderer.getCommandPool(),
                        renderer.getGraphicsQueue(),
                        glbPipeline.getDescriptorSetLayout())) {
        std::cerr << "Failed to load GLB model!" << std::endl;
        glbPipeline.cleanup();
        renderer.cleanup();
        return -1;
    }
    
    auto loadEnd = std::chrono::high_resolution_clock::now();
    auto loadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(loadEnd - loadStart);
    std::cout << "✓ GLB model loaded in " << loadDuration.count() << "ms (optimized batch loading)" << std::endl;

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

    ECS ecs;
    ecs.registerComponent<Transform>();
    ecs.registerComponent<Velocity>();
    ecs.registerComponent<GLBComponent>();
    std::cout << "✓ ECS initialized" << std::endl;

    auto rotationSystem = ecs.registerSystem<RotationSystem>();
    rotationSystem->ecs = &ecs;
    ComponentMask rotationMask;
    rotationMask.set(0);
    rotationMask.set(1);
    ecs.setSystemSignature<RotationSystem>(rotationMask);
    std::cout << "✓ Rotation system registered" << std::endl;

    auto renderSystem = ecs.registerSystem<GLBRenderSystem>();
    renderSystem->ecs = &ecs;
    renderSystem->renderer = &renderer;
    renderSystem->camera = &camera;
    renderSystem->pipeline = &glbPipeline;
    renderSystem->width = width;
    renderSystem->height = height;
    renderSystem->lightDir = config.getVec3("light_direction", glm::vec3(1, -1, 1));
    renderSystem->lightColor = config.getVec3("light_color", glm::vec3(1));
    renderSystem->ambientStrength = config.get("ambient_strength", 0.5f);

    ComponentMask renderMask;
    renderMask.set(0);
    renderMask.set(2);
    ecs.setSystemSignature<GLBRenderSystem>(renderMask);
    std::cout << "✓ Render system registered" << std::endl;

    std::cout << "Creating entity grid..." << std::endl;
    int gridSize = config.get("grid_size", 2);
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

    int entityCount = (gridSize * 2 + 1) * (gridSize * 2 + 1);
    std::cout << "✓ Created " << entityCount << " entities\n" << std::endl;

    std::cout << "\n=== Controls ===" << std::endl;
    std::cout << "  WASD       - Move camera" << std::endl;
    std::cout << "  Space/Ctrl - Up/Down" << std::endl;
    std::cout << "  Shift      - Sprint" << std::endl;
    std::cout << "  Right Click - Toggle mouse look" << std::endl;
    std::cout << "  Scroll     - Zoom" << std::endl;
    std::cout << "  ESC        - Exit" << std::endl;
    std::cout << "\n=== Performance Info ===" << std::endl;
    std::cout << "  Entities: " << entityCount << std::endl;
    std::cout << "  Optimizations: Double buffering, Batch texture loading" << std::endl;
    std::cout << "  Bug Fix: Smearing issue FIXED" << std::endl;
    std::cout << "\n=== Starting render loop ===" << std::endl;

    int maxFPS = config.get("max_fps", 0);
    float targetFrameTime = (maxFPS > 0) ? (1.0f / maxFPS) : 0.0f;

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
                float avgFrameTime = (1000.0f / frameCount);
                std::cout << "FPS: " << frameCount
                          << " | Frame time: " << avgFrameTime << "ms"
                          << " | Pos: (" << (int)camera.position.x << ", "
                          << (int)camera.position.y << ", " << (int)camera.position.z
                          << ")";
                          
                // Performance indicators
                if (frameCount >= 60) {
                    std::cout << " [EXCELLENT]";
                } else if (frameCount >= 30) {
                    std::cout << " [GOOD]";
                } else {
                    std::cout << " [NEEDS OPTIMIZATION]";
                }
                std::cout << std::endl;
                
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
    std::cout << "\nSmearing bug fixed! Models should render cleanly now." << std::endl;

    return 0;
}
