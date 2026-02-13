#include "Renderer.h"
#include "Camera.h"
#include "CameraController.h"
#include "Config.h"
#include "Input.h"
#include "ModelLoader.h"
#include "Pipeline.h"
#include "PostProcessing.h"
#include "ResourcePath.h"
#include "SceneManager.h"
#include "ScenePackager.h"
#include "Skybox.h"
#include "Time.h"
#include "Engine.h"
#include "transform.h"
#include "tags.h"
#include "ModelComponent.h"
#include "CameraComponent.h"
#include <chrono>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

int main(int argc, char** argv) {
    std::string scenePath = argc > 1 ? argv[1] : "scenes/default.zscene";
    
    // Initialize resource paths
    ResourcePath::init();
    
    // Load config
    Config config;
    config.load(ResourcePath::config("config.ini"));
    
    // Initialize renderer
    std::cout << "Initializing renderer...\n";
    VulkanRenderer renderer;
    if (!renderer.init(1920, 1080, "Zero Runtime")) {
        std::cerr << "❌ Failed to initialize renderer\n";
        return -1;
    }
    std::cout << "✓ Renderer initialized\n";
    
    // Initialize input and time
    Input::init(renderer.getWindow());
    Time::init();
    
    // Create descriptor pool
    std::cout << "Creating descriptor pool...\n";
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000}
    };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 1000;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    
    VkDescriptorPool descriptorPool;
    if (vkCreateDescriptorPool(renderer.getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        std::cerr << "❌ Failed to create descriptor pool\n";
        return -1;
    }
    std::cout << "✓ Descriptor pool created\n";
    
    // Initialize post-processing
    std::cout << "Initializing post-processing...\n";
    PostProcessing postProcess;
    if (!postProcess.init(renderer.getDevice(), renderer.getAllocator(), descriptorPool,
                    renderer.getWidth(), renderer.getHeight(), VK_FORMAT_D32_SFLOAT,
                    ResourcePath::shaders("fullscreen_vert.spv"),
                    ResourcePath::shaders("bloom_frag.spv"),
                    ResourcePath::shaders("composite_frag.spv"))) {
        std::cerr << "❌ Failed to initialize post-processing (missing shaders?)\n";
        return -1;
    }
    std::cout << "✓ PostProcessing initialized\n";
    
    // Initialize pipeline
    std::cout << "Initializing pipeline...\n";
    Pipeline pipeline;
    if (!pipeline.init(renderer.getDevice(), postProcess.getSceneRenderPass(),
                 ResourcePath::shaders("unified_vert.spv"),
                 ResourcePath::shaders("unified_frag.spv"))) {
        std::cerr << "❌ Failed to initialize pipeline (missing shaders?)\n";
        return -1;
    }
    
    if (pipeline.getDescriptorLayout() == VK_NULL_HANDLE) {
        std::cerr << "❌ Pipeline descriptor layout is NULL\n";
        return -1;
    }
    std::cout << "✓ Pipeline initialized (layout: " << pipeline.getDescriptorLayout() << ")\n";
    
    // Initialize model loader
    std::cout << "Initializing model loader...\n";
    ModelLoader modelLoader;
    if (!modelLoader.init(renderer.getDevice(), renderer.getAllocator(),
                    renderer.getCommandPool(), renderer.getGraphicsQueue(),
                    descriptorPool, pipeline.getDescriptorLayout())) {
        std::cerr << "❌ Failed to initialize model loader\n";
        return -1;
    }
    std::cout << "✓ ModelLoader initialized\n";
    
    // Initialize skybox (optional - don't fail if it doesn't work)
    std::cout << "Initializing skybox...\n";
    Skybox skybox;
    bool skyboxEnabled = false;
    std::vector<std::string> skyboxFaces = {
        ResourcePath::textures("skybox/right.jpg"),
        ResourcePath::textures("skybox/left.jpg"),
        ResourcePath::textures("skybox/top.jpg"),
        ResourcePath::textures("skybox/bottom.jpg"),
        ResourcePath::textures("skybox/front.jpg"),
        ResourcePath::textures("skybox/back.jpg")
    };
    
    if (skybox.init(renderer.getDevice(), renderer.getAllocator(), descriptorPool,
               postProcess.getSceneRenderPass(), renderer.getCommandPool(),
               renderer.getGraphicsQueue(),
               ResourcePath::shaders("skybox_vert.spv"),
               ResourcePath::shaders("skybox_frag.spv"), skyboxFaces)) {
        std::cout << "✓ Skybox initialized\n";
        skyboxEnabled = true;
    } else {
        std::cout << "⚠ Skybox initialization failed (non-fatal)\n";
    }
    
    // Initialize ECS
    std::cout << "Setting up ECS...\n";
    ECS ecs;
    ecs.registerComponent<Transform>();
    ecs.registerComponent<Tag>();
    ecs.registerComponent<Layer>();
    ecs.registerComponent<ModelComponent>();
    ecs.registerComponent<CameraComponent>();
    std::cout << "✓ ECS components registered\n";
    
    // Load or create scene
    std::cout << "Loading scene: " << scenePath << "\n";
    if (!ScenePackaging::ScenePackager::loadScene(&ecs, scenePath)) {
        std::cout << "⚠ Failed to load scene, creating default scene\n";
        
// In the scene creation section, modify the cube setup:
EntityID cube = ecs.createEntity();
Transform t;
t.position = glm::vec3(0, 0, -5);  // Move it 5 units in front of camera
t.scale = glm::vec3(2);  // Make it bigger
t.rotation = glm::quat(1, 0, 0, 0);
ecs.addComponent(cube, t);
ecs.addComponent(cube, Tag{"TestCube"});
ecs.addComponent(cube, ModelComponent(ResourcePath::models("cube.obj")));

// And adjust camera to look at origin
EntityID camEntity = ecs.createEntity();
Transform camTransform;
camTransform.position = glm::vec3(0, 2, 5);
ecs.addComponent(camEntity, camTransform);
ecs.addComponent(camEntity, Tag{"MainCamera"});

CameraComponent camComp;
camComp.camera.position = glm::vec3(0, 2, 5);
camComp.camera.target = glm::vec3(0, 0, -5);  // Look at the cube
camComp.camera.aspectRatio = float(renderer.getWidth()) / float(renderer.getHeight());
camComp.isActive = true;
ecs.addComponent(camEntity, camComp);
        
        std::cout << "✓ Created default test scene\n";
    }
    
    // Load all models
    std::cout << "Loading models...\n";
    int modelsLoaded = 0;
    for (EntityID e = 0; e < 10000; e++) {
        auto* mc = ecs.getComponent<ModelComponent>(e);
        if (mc && mc->loadedModel == nullptr && !mc->modelPath.empty()) {
            Model m = modelLoader.load(mc->modelPath);
            if (!m.vertices.empty()) {
                mc->loadedModel = new Model(std::move(m));
                std::cout << "  ✓ Loaded: " << mc->modelPath << "\n";
                modelsLoaded++;
            } else {
                std::cerr << "  ✗ Failed to load: " << mc->modelPath << "\n";
                ecs.removeComponent<ModelComponent>(e);
            }
        }
    }
    std::cout << "✓ Loaded " << modelsLoaded << " models\n";
    
    // Find active camera
    EntityID activeCameraEntity = 0;
    for (EntityID e = 0; e < 10000; e++) {
        auto* camComp = ecs.getComponent<CameraComponent>(e);
        if (camComp && camComp->isActive) {
            activeCameraEntity = e;
            auto* tag = ecs.getComponent<Tag>(e);
            std::cout << "✓ Active camera: " << (tag ? tag->name : "Unnamed") << " (Entity " << e << ")\n";
            break;
        }
    }
    
    if (activeCameraEntity == 0) {
        std::cerr << "❌ No active camera found!\n";
        return -1;
    }
    
    auto* camTransform = ecs.getComponent<Transform>(activeCameraEntity);
    auto* camComp = ecs.getComponent<CameraComponent>(activeCameraEntity);
    
    if (!camTransform || !camComp) {
        std::cerr << "❌ Camera entity missing Transform or CameraComponent!\n";
        return -1;
    }
    
    // Initialize camera controller
    CameraController controller(&camComp->camera, config);
    
    std::cout << "\n=== Zero Runtime Ready ===\n";
    std::cout << "Controls:\n";
    std::cout << "  WASD - Move\n";
    std::cout << "  Mouse - Look (hold right-click)\n";
    std::cout << "  Space/Ctrl - Up/Down\n";
    std::cout << "  Shift - Sprint\n";
    std::cout << "  ESC - Quit\n\n";
    
    // Main loop
    using Clock = std::chrono::high_resolution_clock;
    auto lastTime = Clock::now();
    int frameCount = 0;
    
    while (!renderer.shouldClose()) {
        auto now = Clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        
        // Cap delta time to prevent huge jumps
        if (dt > 0.1f) dt = 0.1f;
        
        renderer.pollEvents();
        
        if (Input::getKey(Key::Escape)) {
            std::cout << "Exiting...\n";
            break;
        }
        
        Time::update();
        ecs.updateSystems(dt);
        
        // Update camera
        camComp->camera.position = camTransform->position;
        controller.update(dt, renderer.getWindow());
        camTransform->position = camComp->camera.position;
        
        // Begin frame
    VkCommandBuffer cmd;
    renderer.beginFrame(cmd);
    
    // Scene pass
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.05f, 0.05f, 0.08f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    
    postProcess.beginScenePass(cmd, clearValues);
    
    // Render skybox
    if (skyboxEnabled) {
        skybox.render(cmd, camComp->camera.getViewMatrix(), camComp->camera.getProjectionMatrix());
    }
    
    // Bind main pipeline
    pipeline.bind(cmd);
    
    // Count models to render
    int modelsRendered = 0;
    
    // Render all models
    for (EntityID e = 0; e < 10000; e++) {
        auto* transform = ecs.getComponent<Transform>(e);
        auto* mc = ecs.getComponent<ModelComponent>(e);
        
        if (!transform || !mc || !mc->loadedModel) continue;
        
        Model* model = mc->loadedModel;
        
        // Validate model
        if (model->vertices.empty() || model->indices.empty()) {
            if (frameCount == 0) std::cerr << "⚠ Entity " << e << ": Model has no geometry\n";
            continue;
        }
        
        if (model->vertexBuffer == VK_NULL_HANDLE) {
            if (frameCount == 0) std::cerr << "⚠ Entity " << e << ": Invalid vertex buffer\n";
            continue;
        }
        
        if (model->indexBuffer == VK_NULL_HANDLE) {
            if (frameCount == 0) std::cerr << "⚠ Entity " << e << ": Invalid index buffer\n";
            continue;
        }
        
        if (model->descriptorSet == VK_NULL_HANDLE) {
            if (frameCount == 0) std::cerr << "⚠ Entity " << e << ": Invalid descriptor set\n";
            continue;
        }
        
        if (model->totalIndices == 0) {
            if (frameCount == 0) std::cerr << "⚠ Entity " << e << ": Zero indices\n";
            continue;
        }
        
        // Setup push constants
        glm::mat4 modelMatrix = transform->getLocalMatrix();
        
        PushConstants pc{};
        pc.model = modelMatrix;
        pc.viewProj = camComp->camera.getProjectionMatrix() * camComp->camera.getViewMatrix();
        pc.lightDir = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
        pc.ambientStrength = 0.3f;
        pc.lightColor = glm::vec3(1.0f);
        pc.shadowBias = 0.005f;
        pc.cameraPos = camComp->camera.position;
        pc.fogDensity = 0.0f;
        pc.fogColor = glm::vec3(0.5f);
        pc.fogStart = 10.0f;
        pc.fogEnd = 50.0f;
        pc.emissionStrength = 0.0f;
        pc.useExponentialFog = 0.0f;
        pc.numPointLights = 0;
        
        // Push constants
        vkCmdPushConstants(cmd, pipeline.getPipelineLayout(),
                         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                         0, sizeof(PushConstants), &pc);
        
        // Bind descriptor set
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                               pipeline.getPipelineLayout(), 0, 1,
                               &model->descriptorSet, 0, nullptr);
        
        // Bind buffers
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &model->vertexBuffer, &offset);
        vkCmdBindIndexBuffer(cmd, model->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        
        // Draw
        vkCmdDrawIndexed(cmd, model->totalIndices, 1, 0, 0, 0);
        modelsRendered++;
    }
    
    if (frameCount == 0) {
        std::cout << "First frame: rendered " << modelsRendered << " models\n";
    }
    
    postProcess.endScenePass(cmd);
    
    // Apply post-processing
    postProcess.applyPostProcess(cmd, renderer.getRenderPass(),
                                 renderer.getCurrentFramebuffer());
    
    vkCmdEndRenderPass(cmd);
    
    // End frame
    renderer.endFrame(cmd);
    Input::update();
    
    frameCount++;
}}
