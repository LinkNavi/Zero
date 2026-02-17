// Complete ZeroEngine.cpp implementation with scene save/load
#include "ZeroEngine.h"
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
#include <glm/gtx/euler_angles.hpp>

// Globals required by existing code
Pipeline* g_pipeline = nullptr;
VkDescriptorPool g_descriptorPool = VK_NULL_HANDLE;
VulkanRenderer* g_renderer = nullptr;
ModelLoader* g_modelLoader = nullptr;
Camera* g_camera = nullptr;
ShadowMap* g_shadowMap = nullptr;

// ============================================================
// Offscreen render target for embedded mode
// ============================================================
struct OffscreenTarget {
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    
    VkImage depthImage = VK_NULL_HANDLE;
    VkImageView depthView = VK_NULL_HANDLE;
    VmaAllocation depthAllocation = nullptr;
    
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    
    uint32_t width = 0, height = 0;
    bool valid = false;
    
    bool create(VkDevice device, VmaAllocator allocator, uint32_t w, uint32_t h) {
        width = w;
        height = h;
        
        // Color image (RGBA8 for editor display)
        VkImageCreateInfo imgInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imgInfo.imageType = VK_IMAGE_TYPE_2D;
        imgInfo.extent = {w, h, 1};
        imgInfo.mipLevels = imgInfo.arrayLayers = 1;
        imgInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        
        if (vmaCreateImage(allocator, &imgInfo, &allocInfo, &image, &allocation, nullptr) != VK_SUCCESS)
            return false;
        
        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
            return false;
        
        // Depth image
        imgInfo.format = VK_FORMAT_D32_SFLOAT;
        imgInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        
        if (vmaCreateImage(allocator, &imgInfo, &allocInfo, &depthImage, &depthAllocation, nullptr) != VK_SUCCESS)
            return false;
        
        viewInfo.image = depthImage;
        viewInfo.format = VK_FORMAT_D32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        
        if (vkCreateImageView(device, &viewInfo, nullptr, &depthView) != VK_SUCCESS)
            return false;
        
        // Render pass
        VkAttachmentDescription attachments[2] = {};
        attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        
        attachments[1].format = VK_FORMAT_D32_SFLOAT;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
        VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pDepthStencilAttachment = &depthRef;
        
        VkSubpassDependency deps[2] = {};
        deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        deps[0].dstSubpass = 0;
        deps[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        deps[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
        deps[1].srcSubpass = 0;
        deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        deps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        deps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        VkRenderPassCreateInfo rpInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rpInfo.attachmentCount = 2;
        rpInfo.pAttachments = attachments;
        rpInfo.subpassCount = 1;
        rpInfo.pSubpasses = &subpass;
        rpInfo.dependencyCount = 2;
        rpInfo.pDependencies = deps;
        
        if (vkCreateRenderPass(device, &rpInfo, nullptr, &renderPass) != VK_SUCCESS)
            return false;
        
        // Framebuffer
        VkImageView fbViews[] = {imageView, depthView};
        VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = 2;
        fbInfo.pAttachments = fbViews;
        fbInfo.width = w;
        fbInfo.height = h;
        fbInfo.layers = 1;
        
        if (vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffer) != VK_SUCCESS)
            return false;
        
        // Sampler for editor to sample this image
        VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        samplerInfo.magFilter = samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        
        if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
            return false;
        
        valid = true;
        return true;
    }
    
    void destroy(VkDevice device, VmaAllocator allocator) {
        if (sampler) vkDestroySampler(device, sampler, nullptr);
        if (framebuffer) vkDestroyFramebuffer(device, framebuffer, nullptr);
        if (renderPass) vkDestroyRenderPass(device, renderPass, nullptr);
        if (depthView) vkDestroyImageView(device, depthView, nullptr);
        if (depthImage) vmaDestroyImage(allocator, depthImage, depthAllocation);
        if (imageView) vkDestroyImageView(device, imageView, nullptr);
        if (image) vmaDestroyImage(allocator, image, allocation);
        *this = {};
    }
};

// ============================================================
// Internal implementation
// ============================================================
struct ZeroEngine::Impl {
    EngineConfig config;
    EngineMode mode = EngineMode::Standalone;
    PlayState playState = PlayState::Editing;
    bool running = false;
  
    // Vulkan context
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VmaAllocator allocator = nullptr;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily = 0;
    
    // Subsystems
    VulkanRenderer* renderer = nullptr;
    Pipeline pipeline;
    ModelLoader modelLoader;
    ShadowMap shadowMap;
    Skybox skybox;
    BoneBuffer defaultBoneBuffer;
    PostProcessing postProcess;
    
    // ECS
    ECS* ecs = nullptr;
    
    // Cameras
    Camera editorCamera;
    CameraController* cameraController = nullptr;  // For editor camera controls
    EntityID activeCameraEntity = INVALID_ENTITY;
    
    // Offscreen target (embedded mode)
    OffscreenTarget offscreen;
    
    // Embedded mode command buffer
    VkCommandBuffer frameCmd = VK_NULL_HANDLE;
    VkFence frameFence = VK_NULL_HANDLE;
    
    // Settings
    bool postProcessEnabled = false;
    bool shadowsEnabled = true;
    bool skyboxEnabled = false;
    
    glm::vec3 lightDir = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
    glm::vec3 lightColor = glm::vec3(1.0f);
    float ambientStrength = 0.3f;
    
    // Timing
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point lastTime;
    int frameCount = 0;
    
    // Track loaded models for cleanup
    std::vector<EntityID> modelEntities;
    
    // Snapshot for play mode
   struct SceneSnapshot {
    std::vector<EntityInfo> entities;
    std::unordered_map<EntityID, EntityID> parentMap;
};

SceneSnapshot sceneSnapshot;
    // ==================== Init ====================
    
    bool init(const EngineConfig& cfg) {
        config = cfg;
        mode = cfg.mode;
        
        ResourcePath::init();
        
        if (mode == EngineMode::Standalone) {
            return initStandalone();
        } else {
            return initEmbedded();
        }
    }
    
    bool initStandalone() {
        renderer = new VulkanRenderer();
        if (!renderer->init(config.width, config.height, config.windowTitle.c_str())) {
            std::cerr << "Failed to initialize renderer\n";
            return false;
        }
        
        device = renderer->getDevice();
        physicalDevice = renderer->getPhysicalDevice();
        allocator = renderer->getAllocator();
        commandPool = renderer->getCommandPool();
        graphicsQueue = renderer->getGraphicsQueue();
        graphicsQueueFamily = renderer->getGraphicsQueueFamily();
        
        g_renderer = renderer;
        
        Input::init(renderer->getWindow());
        Time::init();
        
        // Initialize editor camera
        editorCamera.position = glm::vec3(0, 2, 5);
        editorCamera.target = glm::vec3(0, 0, 0);
        editorCamera.aspectRatio = float(config.width) / float(config.height);
        
        // Create camera controller for editor camera
        // CameraController needs a Config object
        Config cfg;
        cfg.load("config.ini");  // Load config if it exists
        cameraController = new CameraController(&editorCamera, cfg);
        
        if (!createDescriptorPool()) return false;
        if (!initSubsystems(renderer->getRenderPass())) return false;
        
        running = true;
        lastTime = Clock::now();
        
        std::cout << "\n=== Zero Engine Initialized (Standalone) ===\n";
        return true;
    }
    
    bool initEmbedded() {
        device = config.device;
        physicalDevice = config.physicalDevice;
        allocator = config.allocator;
        commandPool = config.commandPool;
        graphicsQueue = config.graphicsQueue;
        graphicsQueueFamily = config.graphicsQueueFamily;
        descriptorPool = config.descriptorPool;
        g_descriptorPool = descriptorPool;
        
        if (!device || !allocator) {
            std::cerr << "Embedded mode requires device and allocator\n";
            return false;
        }
        
        uint32_t w = config.width > 0 ? config.width : 1280;
        uint32_t h = config.height > 0 ? config.height : 720;
        
        if (!offscreen.create(device, allocator, w, h)) {
            std::cerr << "Failed to create offscreen target\n";
            return false;
        }
        
        if (!descriptorPool) {
            if (!createDescriptorPool()) return false;
        }
        
        VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(device, &fenceInfo, nullptr, &frameFence);
        
        if (!initSubsystems(offscreen.renderPass)) return false;
        
        running = true;
        lastTime = Clock::now();
        
        editorCamera.position = glm::vec3(0, 2, 5);
        editorCamera.target = glm::vec3(0, 0, 0);
        editorCamera.aspectRatio = float(w) / float(h);
        
        std::cout << "\n=== Zero Engine Initialized (Embedded " << w << "x" << h << ") ===\n";
        return true;
    }
    
    bool createDescriptorPool() {
        VkDescriptorPoolSize poolSizes[] = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000}
        };
        VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        poolInfo.poolSizeCount = 2;
        poolInfo.pPoolSizes = poolSizes;
        poolInfo.maxSets = 1000;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        
        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
            return false;
        
        g_descriptorPool = descriptorPool;
        return true;
    }
    
    bool initSubsystems(VkRenderPass renderPass) {
        if (config.enableShadows) {
            if (!shadowMap.init(device, allocator)) {
                std::cerr << "Failed to init shadow map\n";
                return false;
            }
            if (!shadowMap.createPipeline(ResourcePath::shaders("shadow_vert.spv"))) {
                std::cerr << "Failed to create shadow pipeline\n";
                return false;
            }
            g_shadowMap = &shadowMap;
            shadowsEnabled = true;
        }
        
        if (!pipeline.init(device, renderPass,
                     ResourcePath::shaders("unified_vert.spv"),
                     ResourcePath::shaders("unified_frag.spv"))) {
            std::cerr << "Failed to init pipeline\n";
            return false;
        }
        g_pipeline = &pipeline;
        
        if (!modelLoader.init(device, allocator, commandPool, graphicsQueue,
                        descriptorPool, pipeline.getDescriptorLayout())) {
            std::cerr << "Failed to init model loader\n";
            return false;
        }
        g_modelLoader = &modelLoader;
        
        defaultBoneBuffer.create(allocator);
        
        if (config.enableSkybox) {
            std::vector<std::string> skyboxFaces = {
                ResourcePath::textures("skybox/right.jpg"),
                ResourcePath::textures("skybox/left.jpg"),
                ResourcePath::textures("skybox/top.jpg"),
                ResourcePath::textures("skybox/bottom.jpg"),
                ResourcePath::textures("skybox/front.jpg"),
                ResourcePath::textures("skybox/back.jpg")
            };
            
            skyboxEnabled = skybox.init(device, allocator, descriptorPool,
                   renderPass, commandPool, graphicsQueue,
                   ResourcePath::shaders("skybox_vert.spv"),
                   ResourcePath::shaders("skybox_frag.spv"), skyboxFaces);
        }
        
        ecs = new ECS();
        ecs->registerComponent<Transform>();
        ecs->registerComponent<Tag>();
        ecs->registerComponent<Layer>();
        ecs->registerComponent<ModelComponent>();
        ecs->registerComponent<CameraComponent>();
        
        return true;
    }
    
    // ==================== Update ====================
    
    void update(float dt) {
        if (!running) return;
        
        if (dt < 0.0f) {
            auto now = Clock::now();
            dt = std::chrono::duration<float>(now - lastTime).count();
            lastTime = now;
            if (dt > 0.1f) dt = 0.1f;
        }
        
        if (mode == EngineMode::Standalone) {
            updateStandalone(dt);
        } else {
            updateEmbedded(dt);
        }
        
        frameCount++;
    }
    
    void updateStandalone(float dt) {
        renderer->pollEvents();
        if (renderer->shouldClose()) { running = false; return; }
        
        Time::update();
        
        // Update camera controller in edit mode
        if (playState == PlayState::Editing && cameraController) {
            cameraController->update(dt, renderer->getWindow());
        }
        
        if (playState == PlayState::Playing) {
            ecs->updateSystems(dt);
        }
        
        Camera* cam = getActiveCamera();
        if (!cam) return;
        
        VkCommandBuffer cmd;
        renderer->beginFrame(cmd);
        
        if (shadowsEnabled) {
            renderShadowPass(cmd);
        }
        
        VkRenderPassBeginInfo rpInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rpInfo.renderPass = renderer->getRenderPass();
        rpInfo.framebuffer = renderer->getCurrentFramebuffer();
        rpInfo.renderArea = {{0, 0}, {renderer->getWidth(), renderer->getHeight()}};
        
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.05f, 0.05f, 0.08f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        rpInfo.clearValueCount = 2;
        rpInfo.pClearValues = clearValues.data();
        
        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        VkViewport viewport{0, 0, float(renderer->getWidth()), float(renderer->getHeight()), 0, 1};
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        VkRect2D scissor{{0, 0}, {renderer->getWidth(), renderer->getHeight()}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        
        renderScene(cmd, cam);
        
        vkCmdEndRenderPass(cmd);
        renderer->endFrame(cmd);
        
        Input::update();
    }
    
    void updateEmbedded(float dt) {
        if (!offscreen.valid) return;
        
        if (playState == PlayState::Playing) {
            ecs->updateSystems(dt);
        }
        
        Camera* cam = &editorCamera;
        if (playState == PlayState::Playing) {
            Camera* gameCam = getActiveGameCamera();
            if (gameCam) cam = gameCam;
        }
        
        vkWaitForFences(device, 1, &frameFence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &frameFence);
        
        VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        
        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(device, &allocInfo, &cmd);
        
        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);
        
        if (shadowsEnabled) {
            renderShadowPass(cmd);
        }
        
        VkRenderPassBeginInfo rpInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rpInfo.renderPass = offscreen.renderPass;
        rpInfo.framebuffer = offscreen.framebuffer;
        rpInfo.renderArea = {{0, 0}, {offscreen.width, offscreen.height}};
        
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.05f, 0.05f, 0.08f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        rpInfo.clearValueCount = 2;
        rpInfo.pClearValues = clearValues.data();
        
        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        VkViewport viewport{0, 0, float(offscreen.width), float(offscreen.height), 0, 1};
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        VkRect2D scissor{{0, 0}, {offscreen.width, offscreen.height}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        
        renderScene(cmd, cam);
        
        vkCmdEndRenderPass(cmd);
        
        vkEndCommandBuffer(cmd);
        
        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, frameFence);
        
        frameCmd = cmd;
    }
    
    // ==================== Rendering ====================
    
    void renderShadowPass(VkCommandBuffer cmd) {
        shadowMap.updateLightMatrix(glm::vec3(0, 0, 0));
        shadowMap.beginShadowPass(cmd);
        
        for (EntityID e = 0; e < 10000; e++) {
            auto* transform = ecs->getComponent<Transform>(e);
            auto* mc = ecs->getComponent<ModelComponent>(e);
            if (!transform || !mc || !mc->loadedModel) continue;
            
            Model* model = mc->loadedModel;
            if (!model->vertexBuffer || !model->indexBuffer || !model->totalIndices) continue;
            
            ShadowPushConstants spc{};
            spc.lightViewProj = shadowMap.lightViewProj;
            spc.model = transform->getLocalMatrix();
            vkCmdPushConstants(cmd, shadowMap.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(spc), &spc);
            
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                   shadowMap.pipelineLayout, 0, 1,
                                   &model->descriptorSet, 0, nullptr);
            
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &model->vertexBuffer, &offset);
            vkCmdBindIndexBuffer(cmd, model->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, model->totalIndices, 1, 0, 0, 0);
        }
        shadowMap.endShadowPass(cmd);
    }
    
    void renderScene(VkCommandBuffer cmd, Camera* cam) {
    if (skyboxEnabled) {
        skybox.render(cmd, cam->getViewMatrix(), cam->getProjectionMatrix());
    }
    
    pipeline.bind(cmd);
    
    int rendered = 0;
    for (EntityID e = 0; e < 10000; e++) {
        auto* transform = ecs->getComponent<Transform>(e);
        auto* mc = ecs->getComponent<ModelComponent>(e);
        if (!transform || !mc || !mc->loadedModel) continue;
        
        Model* model = mc->loadedModel;
        if (!model->vertexBuffer || !model->indexBuffer) continue;
        if (!model->descriptorSet || !model->totalIndices) continue;
        
        PushConstants pc{};
        pc.viewProj = cam->getProjectionMatrix() * cam->getViewMatrix();
        pc.model = transform->getWorldMatrix(ecs);  // Changed from getLocalMatrix()
        pc.lightViewProj = shadowsEnabled ? shadowMap.lightViewProj : glm::mat4(1.0f);
        pc.lightDir = lightDir;
        pc.ambientStrength = ambientStrength;
        pc.lightColor = lightColor;
        pc.shadowBias = shadowsEnabled ? shadowMap.bias : 0.0f;
        pc.cameraPos = cam->position;
        pc.fogDensity = 0.0f;
        pc.fogColor = glm::vec3(0.5f);
        pc.fogStart = 10.0f;
        pc.fogEnd = 50.0f;
        pc.emissionStrength = 0.0f;
        pc.useExponentialFog = 0.0f;
        pc.numPointLights = 0;
        
        vkCmdPushConstants(cmd, pipeline.getPipelineLayout(),
                         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                         0, sizeof(PushConstants), &pc);
        
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                               pipeline.getPipelineLayout(), 0, 1,
                               &model->descriptorSet, 0, nullptr);
        
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &model->vertexBuffer, &offset);
        vkCmdBindIndexBuffer(cmd, model->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, model->totalIndices, 1, 0, 0, 0);
        rendered++;
    }
    
    if (frameCount == 0) {
        std::cout << "First frame: rendered " << rendered << " models\n";
    }
}    
    // ==================== Camera helpers ====================
    
    Camera* getActiveCamera() {
        if (playState == PlayState::Editing) {
            return &editorCamera;
        }
        
        Camera* gameCam = getActiveGameCamera();
        return gameCam ? gameCam : &editorCamera;
    }
    
    Camera* getActiveGameCamera() {
        for (EntityID e = 0; e < 10000; e++) {
            auto* cc = ecs->getComponent<CameraComponent>(e);
            if (cc && cc->isActive) {
                auto* t = ecs->getComponent<Transform>(e);
                if (t) cc->camera.position = t->position;
                g_camera = &cc->camera;
                return &cc->camera;
            }
        }
        return nullptr;
    }
    
    // ==================== Entity Management ====================
    
    EntityID createEntity(const std::string& name) {
        EntityID e = ecs->createEntity();
        
        Transform t;
        t.position = glm::vec3(0);
        t.scale = glm::vec3(1);
        t.rotation = glm::quat(1, 0, 0, 0);
        ecs->addComponent(e, t);
        ecs->addComponent(e, Tag{name});
        
        return e;
    }
    
    void destroyEntity(EntityID id) {
        auto* mc = ecs->getComponent<ModelComponent>(id);
        if (mc && mc->loadedModel) {
            modelLoader.cleanup(*mc->loadedModel);
            delete mc->loadedModel;
            mc->loadedModel = nullptr;
        }
        
        auto it = std::find(modelEntities.begin(), modelEntities.end(), id);
        if (it != modelEntities.end()) {
            modelEntities.erase(it);
        }
        
        ecs->destroyEntity(id);
    }
    
    bool setEntityModel(EntityID id, const std::string& path) {
        auto* mc = ecs->getComponent<ModelComponent>(id);
        if (!mc) {
            ecs->addComponent(id, ModelComponent(path));
            mc = ecs->getComponent<ModelComponent>(id);
        } else {
            if (mc->loadedModel) {
                modelLoader.cleanup(*mc->loadedModel);
                delete mc->loadedModel;
                mc->loadedModel = nullptr;
            }
            mc->modelPath = path;
        }
        
        Model m = modelLoader.load(path);
        if (m.vertices.empty()) return false;
        
        mc->loadedModel = new Model(std::move(m));
        
        fixDescriptorSet(mc->loadedModel);
        
        modelEntities.push_back(id);
        return true;
    }
    
    void fixDescriptorSet(Model* model) {
        if (!model || !model->descriptorSet) return;
        
        VkDescriptorBufferInfo bufInfo{};
        bufInfo.buffer = defaultBoneBuffer.getBuffer();
        bufInfo.offset = 0;
        bufInfo.range = sizeof(glm::mat4) * 128;
        
        VkWriteDescriptorSet writes[2] = {};
        
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = model->descriptorSet;
        writes[0].dstBinding = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo = &bufInfo;
        
        uint32_t writeCount = 1;
        
        VkDescriptorImageInfo shadowInfo{};
        if (shadowsEnabled && shadowMap.depthView && shadowMap.sampler) {
            shadowInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            shadowInfo.imageView = shadowMap.depthView;
            shadowInfo.sampler = shadowMap.sampler;
            
            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet = model->descriptorSet;
            writes[1].dstBinding = 2;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[1].descriptorCount = 1;
            writes[1].pImageInfo = &shadowInfo;
            writeCount = 2;
        }
        
        vkUpdateDescriptorSets(device, writeCount, writes, 0, nullptr);
    }
    
    // ==================== Resize ====================
    
    void resize(uint32_t w, uint32_t h) {
        if (w == 0 || h == 0) return;
        
        vkDeviceWaitIdle(device);
        
        if (mode == EngineMode::Embedded) {
            offscreen.destroy(device, allocator);
            offscreen.create(device, allocator, w, h);
            editorCamera.aspectRatio = float(w) / float(h);
        }
    }
    
    // ==================== Scene ====================
    
    bool loadScene(const std::string& path) {
        clearScene();
        
        if (!ScenePackaging::ScenePackager::loadScene(ecs, path)) {
            std::cerr << "Failed to load scene: " << path << "\n";
            return false;
        }
        
        // Load model components
        std::cout << "Loading models from scene...\n";
        int modelsLoaded = 0;
        for (EntityID e = 0; e < 10000; e++) {
            auto* mc = ecs->getComponent<ModelComponent>(e);
            if (mc) {
                std::cout << "  Entity " << e << " has ModelComponent, path: '" << mc->modelPath << "'\n";
                if (!mc->loadedModel && !mc->modelPath.empty()) {
                    std::cout << "    Loading model: " << mc->modelPath << "\n";
                    Model m = modelLoader.load(mc->modelPath);
                    if (!m.vertices.empty()) {
                        mc->loadedModel = new Model(std::move(m));
                        fixDescriptorSet(mc->loadedModel);
                        modelEntities.push_back(e);
                        modelsLoaded++;
                        std::cout << "    ✓ Model loaded successfully\n";
                    } else {
                        std::cout << "    ✗ Model load failed (empty vertices)\n";
                    }
                } else if (mc->loadedModel) {
                    std::cout << "    Model already loaded\n";
                } else {
                    std::cout << "    ModelPath is empty!\n";
                }
            }
        }
        std::cout << "Models loaded: " << modelsLoaded << "/" << modelEntities.size() << "\n";
        
        std::cout << "✓ Scene loaded: " << path << "\n";
        return true;
    }
    
    bool saveScene(const std::string& path) {
        return ScenePackaging::ScenePackager::saveScene(ecs, path, "GameScene");
    }
    
    void clearScene() {
        vkDeviceWaitIdle(device);
        
        for (EntityID e : modelEntities) {
            auto* mc = ecs->getComponent<ModelComponent>(e);
            if (mc && mc->loadedModel) {
                modelLoader.cleanup(*mc->loadedModel);
                delete mc->loadedModel;
                mc->loadedModel = nullptr;
            }
        }
        modelEntities.clear();
        
        delete ecs;
        ecs = new ECS();
        ecs->registerComponent<Transform>();
        ecs->registerComponent<Tag>();
        ecs->registerComponent<Layer>();
        ecs->registerComponent<ModelComponent>();
        ecs->registerComponent<CameraComponent>();
    }
    
    // ==================== Play Mode ====================
    
void snapshotScene() {
    sceneSnapshot.entities.clear();
    sceneSnapshot.parentMap.clear();
    
    for (size_t i = 0; i < 10000; i++) {
        auto* t = ecs->getComponent<Transform>(i);
        if (!t) continue;
        
        EntityInfo info;
        info.id = i;
        
        auto* tag = ecs->getComponent<Tag>(i);
        if (tag) info.name = tag->name;
        
        info.position = t->position;
        info.rotation = t->getEulerAngles();
        info.scale = t->scale;
        
        auto* model = ecs->getComponent<ModelComponent>(i);
        if (model) {
            info.hasModel = true;
            info.modelPath = model->modelPath;
        }
        
        auto* cam = ecs->getComponent<CameraComponent>(i);
        if (cam) {
            info.isCamera = true;
            info.isActiveCamera = cam->isActive;
        }
        
        sceneSnapshot.entities.push_back(info);
        if (t->parent != 0) {
            sceneSnapshot.parentMap[i] = t->parent;
        }
    }
}

void restoreSnapshot() {
    if (sceneSnapshot.entities.empty()) return;
    
    ecs->clear();
    
    std::unordered_map<EntityID, EntityID> oldToNew;
    
    for (const auto& info : sceneSnapshot.entities) {
        EntityID newId = ecs->createEntity();
        oldToNew[info.id] = newId;
        
        Transform t;
        t.position = info.position;
        t.setEulerAngles(info.rotation);
        t.scale = info.scale;
        ecs->addComponent(newId, t);
        
        if (!info.name.empty()) {
            ecs->addComponent(newId, Tag(info.name));
        }
        
        ecs->addComponent(newId, Layer());
        
        if (info.hasModel) {
            ModelComponent mc(info.modelPath);
            Model m = modelLoader.load(info.modelPath);
            if (!m.vertices.empty()) {
                mc.loadedModel = new Model(std::move(m));
                fixDescriptorSet(mc.loadedModel);
            }
            ecs->addComponent(newId, mc);
        }
        
        if (info.isCamera) {
            CameraComponent cc;
            cc.isActive = info.isActiveCamera;
            ecs->addComponent(newId, cc);
        }
    }
    
    for (const auto& [oldChild, oldParent] : sceneSnapshot.parentMap) {
        auto childIt = oldToNew.find(oldChild);
        auto parentIt = oldToNew.find(oldParent);
        
        if (childIt != oldToNew.end() && parentIt != oldToNew.end()) {
            auto* childTransform = ecs->getComponent<Transform>(childIt->second);
            if (childTransform) {
                childTransform->parent = parentIt->second;
            }
        }
    }
}
    
    // ==================== Shutdown ====================
    
    void shutdown() {
        if (!running) return;
        running = false;
        
        vkDeviceWaitIdle(device);
        
        for (EntityID e : modelEntities) {
            auto* mc = ecs->getComponent<ModelComponent>(e);
            if (mc && mc->loadedModel) {
                modelLoader.cleanup(*mc->loadedModel);
                delete mc->loadedModel;
            }
        }
        
        delete ecs;
        ecs = nullptr;
        
        if (cameraController) {
            delete cameraController;
            cameraController = nullptr;
        }
        
        defaultBoneBuffer.cleanup();
        skybox.cleanup();
        shadowMap.cleanup();
        postProcess.cleanup();
        pipeline.cleanup();
        modelLoader.cleanupLoader();
        
        if (mode == EngineMode::Embedded) {
            offscreen.destroy(device, allocator);
            if (frameFence) vkDestroyFence(device, frameFence, nullptr);
            if (frameCmd) vkFreeCommandBuffers(device, commandPool, 1, &frameCmd);
        }
        
        if (mode == EngineMode::Standalone || !config.descriptorPool) {
            if (descriptorPool) vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        }
        
        if (renderer) {
            renderer->cleanup();
            delete renderer;
            renderer = nullptr;
        }
    }
};

// ============================================================
// Public API implementation
// ============================================================

ZeroEngine::ZeroEngine() : impl(new Impl()) {}
ZeroEngine::~ZeroEngine() { if (impl) { impl->shutdown(); delete impl; } }

bool ZeroEngine::init(const EngineConfig& config) { return impl->init(config); }
void ZeroEngine::shutdown() { impl->shutdown(); }
bool ZeroEngine::isRunning() const { return impl->running; }
void ZeroEngine::update(float dt) { impl->update(dt); }
void ZeroEngine::resize(uint32_t w, uint32_t h) { impl->resize(w, h); }

EngineFrame ZeroEngine::getOutputFrame() const {
    EngineFrame f;
    if (impl->mode == EngineMode::Embedded && impl->offscreen.valid) {
        f.outputImage = impl->offscreen.image;
        f.outputImageView = impl->offscreen.imageView;
        f.outputLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        f.width = impl->offscreen.width;
        f.height = impl->offscreen.height;
    }
    return f;
}

VkSampler ZeroEngine::getOutputSampler() const { return impl->offscreen.sampler; }

bool ZeroEngine::loadScene(const std::string& path) { return impl->loadScene(path); }
bool ZeroEngine::saveScene(const std::string& path) { return impl->saveScene(path); }
void ZeroEngine::newScene() { impl->clearScene(); }

EntityID ZeroEngine::createEntity(const std::string& name) { return impl->createEntity(name); }
void ZeroEngine::destroyEntity(EntityID id) { impl->destroyEntity(id); }

std::vector<EntityInfo> ZeroEngine::getEntities() const {
    std::vector<EntityInfo> result;
    for (EntityID e = 0; e < 10000; e++) {
        auto* t = impl->ecs->getComponent<Transform>(e);
        if (!t) continue;
        
        EntityInfo info;
        info.id = e;
        
        auto* tag = impl->ecs->getComponent<Tag>(e);
        info.name = tag ? tag->name : ("Entity_" + std::to_string(e));
        
        info.position = t->position;
        info.scale = t->scale;
        info.rotation = glm::degrees(glm::eulerAngles(t->rotation));
        
        auto* mc = impl->ecs->getComponent<ModelComponent>(e);
        if (mc) {
            info.hasModel = true;
            info.modelPath = mc->modelPath;
        }
        
        auto* cc = impl->ecs->getComponent<CameraComponent>(e);
        if (cc) {
            info.isCamera = true;
            info.isActiveCamera = cc->isActive;
        }
        
        result.push_back(info);
    }
    return result;
}

EntityInfo ZeroEngine::getEntityInfo(EntityID id) const {
    EntityInfo info;
    info.id = id;
    
    auto* t = impl->ecs->getComponent<Transform>(id);
    if (!t) return info;
    
    auto* tag = impl->ecs->getComponent<Tag>(id);
    info.name = tag ? tag->name : "";
    info.position = t->position;
    info.scale = t->scale;
    info.rotation = glm::degrees(glm::eulerAngles(t->rotation));
    info.parent = t->parent;  // Add this
    
    auto* mc = impl->ecs->getComponent<ModelComponent>(id);
    if (mc) { info.hasModel = true; info.modelPath = mc->modelPath; }
    
    auto* cc = impl->ecs->getComponent<CameraComponent>(id);
    if (cc) { info.isCamera = true; info.isActiveCamera = cc->isActive; }
    
    return info;
}

void ZeroEngine::setEntityPosition(EntityID id, glm::vec3 pos) {
    auto* t = impl->ecs->getComponent<Transform>(id);
    if (t) t->position = pos;
}

void ZeroEngine::setEntityRotation(EntityID id, glm::vec3 eulerDeg) {
    auto* t = impl->ecs->getComponent<Transform>(id);
    if (t) t->rotation = glm::quat(glm::radians(eulerDeg));
}

void ZeroEngine::setEntityScale(EntityID id, glm::vec3 scale) {
    auto* t = impl->ecs->getComponent<Transform>(id);
    if (t) t->scale = scale;
}

bool ZeroEngine::setEntityModel(EntityID id, const std::string& path) {
    return impl->setEntityModel(id, path);
}

void ZeroEngine::removeEntityModel(EntityID id) {
    auto* mc = impl->ecs->getComponent<ModelComponent>(id);
    if (mc) {
        if (mc->loadedModel) {
            impl->modelLoader.cleanup(*mc->loadedModel);
            delete mc->loadedModel;
        }
        impl->ecs->removeComponent<ModelComponent>(id);
    }
}

void ZeroEngine::setEntityAsCamera(EntityID id, bool active) {
    auto* cc = impl->ecs->getComponent<CameraComponent>(id);
    if (!cc) {
        CameraComponent comp;
        comp.isActive = active;
        auto* t = impl->ecs->getComponent<Transform>(id);
        if (t) {
            comp.camera.position = t->position;
            comp.camera.aspectRatio = impl->mode == EngineMode::Embedded ?
                float(impl->offscreen.width) / float(impl->offscreen.height) :
                float(impl->renderer->getWidth()) / float(impl->renderer->getHeight());
        }
        impl->ecs->addComponent(id, comp);
    } else {
        cc->isActive = active;
    }
}

void ZeroEngine::setActiveCamera(EntityID id) {
    for (EntityID e = 0; e < 10000; e++) {
        auto* cc = impl->ecs->getComponent<CameraComponent>(e);
        if (cc) cc->isActive = (e == id);
    }
}

EntityID ZeroEngine::getActiveCamera() const {
    for (EntityID e = 0; e < 10000; e++) {
        auto* cc = impl->ecs->getComponent<CameraComponent>(e);
        if (cc && cc->isActive) return e;
    }
    return INVALID_ENTITY;
}

PlayState ZeroEngine::getPlayState() const { return impl->playState; }

void ZeroEngine::play() {
    if (impl->playState == PlayState::Playing) return;
    impl->snapshotScene();
    impl->playState = PlayState::Playing;
}

void ZeroEngine::stop() {
    if (impl->playState == PlayState::Editing) return;
    impl->restoreSnapshot();
    impl->playState = PlayState::Editing;
}



void ZeroEngine::pause() { impl->playState = PlayState::Paused; }

void ZeroEngine::setEntityName(EntityID id, const std::string& name) {
    auto* tag = impl->ecs->getComponent<Tag>(id);
    if (tag) {
        tag->name = name;
    } else {
        impl->ecs->addComponent(id, Tag{name});
    }
}

void ZeroEngine::setEntityParent(EntityID id, EntityID parentId) {
    auto* t = impl->ecs->getComponent<Transform>(id);
    if (t) t->parent = parentId;
}

void ZeroEngine::removeEntityParent(EntityID id) {
    auto* t = impl->ecs->getComponent<Transform>(id);
    if (t) t->parent = 0;
}

EntityID ZeroEngine::getEntityParent(EntityID id) const {
    auto* t = impl->ecs->getComponent<Transform>(id);
    return t ? t->parent : 0;
}

std::vector<EntityID> ZeroEngine::getEntityChildren(EntityID id) const {
    std::vector<EntityID> children;
    for (EntityID e = 0; e < 10000; e++) {
        auto* t = impl->ecs->getComponent<Transform>(e);
        if (t && t->parent == id) {
            children.push_back(e);
        }
    }
    return children;
}

void ZeroEngine::setEditorCameraPosition(glm::vec3 pos) { impl->editorCamera.position = pos; }
void ZeroEngine::setEditorCameraTarget(glm::vec3 target) { impl->editorCamera.target = target; }
glm::vec3 ZeroEngine::getEditorCameraPosition() const { return impl->editorCamera.position; }

void ZeroEngine::setPostProcessEnabled(bool enabled) { impl->postProcessEnabled = enabled; }
void ZeroEngine::setShadowsEnabled(bool enabled) { impl->shadowsEnabled = enabled; }
void ZeroEngine::setSkyboxEnabled(bool enabled) { impl->skyboxEnabled = enabled; }
void ZeroEngine::setExposure(float exposure) { impl->postProcess.settings.exposure = exposure; }
void ZeroEngine::setGamma(float gamma) { impl->postProcess.settings.gamma = gamma; }

void ZeroEngine::setDirectionalLight(glm::vec3 dir, glm::vec3 color, float ambient) {
    impl->lightDir = glm::normalize(dir);
    impl->lightColor = color;
    impl->ambientStrength = ambient;
    impl->shadowMap.lightDir = impl->lightDir;
}

VkDevice ZeroEngine::getDevice() const { return impl->device; }
VmaAllocator ZeroEngine::getAllocator() const { return impl->allocator; }
VkDescriptorPool ZeroEngine::getDescriptorPool() const { return impl->descriptorPool; }
