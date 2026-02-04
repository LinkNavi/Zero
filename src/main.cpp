#include "Renderer.h"
#include "PipelineInstanced.h"
#include "ModelLoader.h"
#include "Camera.h"
#include "CameraController.h"
#include "Input.h"
#include "Time.h"
#include "Engine.h"
#include "transform.h"
#include "tags.h"
#include "Config.h"
#include "ResourcePath.h"
#include "NuklearGUI.h"

#include <iostream>

ECS* g_ecs = nullptr;

class RenderSystem : public System {
public:
    InstancedPipeline* pipeline = nullptr;
    VulkanRenderer* renderer = nullptr;
    Model* model = nullptr;
    InstanceBuffer instanceBuffer;
    Camera* camera = nullptr;
    uint32_t instances_count = 0;
    uint32_t draw_calls = 0;
    
    void init(VulkanRenderer* rend, InstancedPipeline* pipe, Model* mdl, Camera* cam) {
        renderer = rend;
        pipeline = pipe;
        model = mdl;
        camera = cam;
        instanceBuffer.create(renderer->getAllocator(), 10000);
    }
    
    void update(float dt) override {
        (void)dt;
        std::vector<InstanceData> instances;
        
        for (EntityID entity : entities) {
            auto* transform = g_ecs->getComponent<Transform>(entity);
            if (!transform) continue;
            
            InstanceData instance;
            instance.model = transform->getLocalMatrix();
            instance.color = glm::vec4(1.0f, 0.8f, 0.2f, 1.0f);
            instances.push_back(instance);
        }
        
        instances_count = static_cast<uint32_t>(instances.size());
        if (instances_count > 0) {
            instanceBuffer.update(instances);
        }
    }
    
    void render(VkCommandBuffer cmd) {
        if (instances_count == 0) return;
        
        draw_calls = 1;
        
        pipeline->bind(cmd);
        
        PushConstantsInstanced pc;
        pc.viewProj = camera->getViewProjectionMatrix();
        pc.lightDir = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
        pc.lightColor = glm::vec3(1.0f, 0.95f, 0.9f);
        pc.ambientStrength = 0.3f;
        pipeline->pushConstants(cmd, pc);
        
        pipeline->bindDescriptorSet(cmd, model->descriptorSet);
        
        VkDeviceSize offsets[] = {0};
        VkBuffer instanceBuf = instanceBuffer.getBuffer();
        
        vkCmdBindVertexBuffers(cmd, 0, 1, &model->combinedVertexBuffer, offsets);
        vkCmdBindVertexBuffers(cmd, 1, 1, &instanceBuf, offsets);
        vkCmdBindIndexBuffer(cmd, model->combinedIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        
        vkCmdDrawIndexed(cmd, model->totalIndices, instances_count, 0, 0, 0);
    }
};

int main() {
    ResourcePath::init();
    Config config;
    config.load(ResourcePath::config("config.ini"));
    
    VulkanRenderer renderer;
    if (!renderer.init(1920, 1080, "Zero Engine")) {
        std::cerr << "Failed to init renderer\n";
        return -1;
    }
    
    std::cout << "✓ Renderer initialized\n";
    
    Input::init(renderer.getWindow());
    Time::init();
    
    Camera camera;
    camera.position = glm::vec3(0, 5, 15);
    camera.aspectRatio = 1920.0f / 1080.0f;
    CameraController camController(&camera, config);
    
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 100;
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 100;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    
    VkDescriptorPool descriptorPool;
    vkCreateDescriptorPool(renderer.getDevice(), &poolInfo, nullptr, &descriptorPool);
    
    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0;
    samplerBinding.descriptorCount = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;
    
    VkDescriptorSetLayout descriptorSetLayout;
    vkCreateDescriptorSetLayout(renderer.getDevice(), &layoutInfo, nullptr, &descriptorSetLayout);
    
    InstancedPipeline pipeline;
    pipeline.init(renderer.getDevice(), renderer.getRenderPass(),
                 ResourcePath::shaders("instanced_vert.spv"),
                 ResourcePath::shaders("instanced_frag.spv"));
    
    ModelLoader modelLoader;
    modelLoader.init(renderer.getDevice(), renderer.getAllocator(),
                    renderer.getCommandPool(), renderer.getGraphicsQueue(),
                    descriptorPool, descriptorSetLayout);
    
    Model cube = modelLoader.load(ResourcePath::models("Duck.glb"));
    
    ECS ecs;
    g_ecs = &ecs;
    
    ecs.registerComponent<Transform>();
    ecs.registerComponent<Tag>();
    ecs.registerComponent<Layer>();
    
    auto renderSystem = ecs.registerSystem<RenderSystem>();
    ComponentMask renderMask;
    renderMask.set(0);
    ecs.setSystemSignature<RenderSystem>(renderMask);
    
    renderSystem->init(&renderer, &pipeline, &cube, &camera);
    
    NuklearGUI gui;
    gui.init(renderer.getDevice(), renderer.getAllocator(), renderer.getRenderPass(),
             renderer.getWindow(), renderer.getWidth(), renderer.getHeight(),
             renderer.getCommandPool(), renderer.getGraphicsQueue());
    
    const int gridSize = 5;
    const float spacing = 3.0f;
    uint32_t entityCount = 0;
    
    for (int x = -gridSize/2; x < gridSize/2; x++) {
        for (int z = -gridSize/2; z < gridSize/2; z++) {
            EntityID entity = ecs.createEntity();
            
            Transform transform;
            transform.position = glm::vec3(x * spacing, 0, z * spacing);
            transform.scale = glm::vec3(0.5f);
            ecs.addComponent(entity, transform);
            
            Tag tag("Duck");
            ecs.addComponent(entity, tag);
            
            entityCount++;
        }
    }
    
    std::cout << "✓ Spawned " << entityCount << " entities\n";
    std::cout << "\nControls:\n";
    std::cout << "  WASD + Mouse2 - Move camera\n";
    std::cout << "  J - Toggle UI\n";
    std::cout << "  ESC - Exit\n\n";
    
    bool showUI = true;
    
    while (!renderer.shouldClose()) {
        renderer.pollEvents();
        
        if (Input::getKey(Key::Escape)) break;
        if (Input::getKeyDown(Key::J)) showUI = !showUI;
        
        Time::update();
        float dt = Time::getDeltaTime();
        
        gui.newFrame();
        
        if (showUI && nk_begin(gui.getContext(), "Stats", nk_rect(10, 10, 250, 200),
            NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_TITLE))
        {
            nk_layout_row_dynamic(gui.getContext(), 25, 1);
            nk_labelf(gui.getContext(), NK_TEXT_LEFT, "FPS: %.1f", Time::getFPS());
            nk_labelf(gui.getContext(), NK_TEXT_LEFT, "Frame: %.2fms", Time::getDeltaTime() * 1000);
            nk_labelf(gui.getContext(), NK_TEXT_LEFT, "Entities: %d", entityCount);
            nk_labelf(gui.getContext(), NK_TEXT_LEFT, "Draw Calls: %d", renderSystem->draw_calls);
            
            nk_layout_row_dynamic(gui.getContext(), 25, 2);
            if (nk_button_label(gui.getContext(), "Toggle Vsync")) {
                // TODO
            }
            nk_end(gui.getContext());
        }
        
        camController.update(dt, renderer.getWindow());
        
        for (uint32_t i = 0; i < entityCount; i++) {
            auto* transform = ecs.getComponent<Transform>(i);
            if (transform) {
                transform->rotate(glm::vec3(0, dt * 30.0f, 0));
            }
        }
        
        renderSystem->update(dt);
        
        VkCommandBuffer cmd;
        renderer.beginFrame(cmd);
        
        VkViewport viewport{};
        viewport.width = static_cast<float>(renderer.getWidth());
        viewport.height = static_cast<float>(renderer.getHeight());
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        
        VkRect2D scissor{};
        scissor.extent = {renderer.getWidth(), renderer.getHeight()};
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        
        renderSystem->render(cmd);
        gui.render(cmd);
        
        renderer.endFrame(cmd);
        Input::update();
    }
    
    std::cout << "\nCleaning up...\n";
    vkDeviceWaitIdle(renderer.getDevice());
    
    gui.cleanup();
    renderSystem->instanceBuffer.cleanup();
    modelLoader.cleanup(cube);
    modelLoader.cleanupLoader();
    pipeline.cleanup();
    vkDestroyDescriptorSetLayout(renderer.getDevice(), descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(renderer.getDevice(), descriptorPool, nullptr);
    renderer.cleanup();
    
    return 0;
}
