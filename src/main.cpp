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

#include <iostream>
#include <chrono>

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
        instanceBuffer.create(renderer->getAllocator(), 100000);
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

void spawnDucks(ECS& ecs, int gridSize, float spacing) {
    ecs.clear();
    
    float offset = (gridSize - 1) * spacing * 0.5f;
    
    for (int x = 0; x < gridSize; x++) {
        for (int z = 0; z < gridSize; z++) {
            EntityID entity = ecs.createEntity();
            
            Transform transform;
            transform.position = glm::vec3(
                x * spacing - offset,
                0,
                z * spacing - offset
            );
            transform.scale = glm::vec3(0.5f);
            ecs.addComponent(entity, transform);
            
            Tag tag("Duck");
            ecs.addComponent(entity, tag);
        }
    }
}

int main() {
    ResourcePath::init();
    Config config;
    config.load(ResourcePath::config("config.ini"));
    
    VulkanRenderer renderer;
    if (!renderer.init(1920, 1080, "Zero Engine")) {
        std::cerr << "Failed to init renderer\n";
        return -1;
    }
    
    renderer.initImGui();
    
    float xscale, yscale;
    glfwGetWindowContentScale(renderer.getWindow(), &xscale, &yscale);
    ImGui::GetIO().FontGlobalScale = xscale;
    ImGui::GetStyle().ScaleAllSizes(xscale);
    
    std::cout << "✓ Renderer initialized\n";
    
    Input::init(renderer.getWindow());
    Time::init();
    
    Camera camera;
    camera.position = glm::vec3(0, 5, 15);
    camera.aspectRatio = static_cast<float>(renderer.getWidth()) / static_cast<float>(renderer.getHeight());
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
    
    Model duck = modelLoader.load(ResourcePath::models("Duck.glb"));
    
    ECS ecs;
    g_ecs = &ecs;
    
    ecs.registerComponent<Transform>();
    ecs.registerComponent<Tag>();
    ecs.registerComponent<Layer>();
    
    auto renderSystem = ecs.registerSystem<RenderSystem>();
    ComponentMask renderMask;
    renderMask.set(0);
    ecs.setSystemSignature<RenderSystem>(renderMask);
    
    renderSystem->init(&renderer, &pipeline, &duck, &camera);
    
    // Grid settings
    int gridSize = 4;
    int prevGridSize = gridSize;
    float spacing = 4.0f;
    float prevSpacing = spacing;
    float rotationSpeed = 30.0f;
    bool autoRotate = true;
    
    spawnDucks(ecs, gridSize, spacing);
    
    std::cout << "✓ Spawned " << gridSize * gridSize << " entities\n";
    std::cout << "\nControls:\n";
    std::cout << "  WASD + Mouse2 - Move camera\n";
    std::cout << "  J - Toggle UI\n";
    std::cout << "  ESC - Exit\n\n";
    
    bool showUI = true;
    
    // Accurate FPS timing using chrono
    using Clock = std::chrono::high_resolution_clock;
    auto lastTime = Clock::now();
    auto fpsTimer = Clock::now();
    int frameCount = 0;
    float displayFps = 0.0f;
    float displayFrameTime = 0.0f;
    float minFrameTime = 1000.0f;
    float maxFrameTime = 0.0f;
    
    while (!renderer.shouldClose()) {
        auto now = Clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        
        // FPS calculation every 0.5 seconds
        frameCount++;
        float fpsElapsed = std::chrono::duration<float>(now - fpsTimer).count();
        if (fpsElapsed >= 0.5f) {
            displayFps = frameCount / fpsElapsed;
            displayFrameTime = fpsElapsed / frameCount * 1000.0f;
            frameCount = 0;
            fpsTimer = now;
            minFrameTime = 1000.0f;
            maxFrameTime = 0.0f;
        }
        
        float frameMs = dt * 1000.0f;
        if (frameMs < minFrameTime) minFrameTime = frameMs;
        if (frameMs > maxFrameTime) maxFrameTime = frameMs;
        
        renderer.pollEvents();
        
        if (Input::getKey(Key::Escape)) break;
        if (Input::getKeyDown(Key::J)) showUI = !showUI;
        
        Time::update();
        camController.update(dt, renderer.getWindow());
        
        // Rotate ducks
        if (autoRotate) {
            for (EntityID entity : renderSystem->entities) {
                auto* transform = ecs.getComponent<Transform>(entity);
                if (transform) {
                    transform->rotate(glm::vec3(0, dt * rotationSpeed, 0));
                }
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
        
        if (showUI) {
            renderer.imguiNewFrame();
            
            ImGui::Begin("Debug");
            
            // Performance
            ImGui::Text("FPS: %.1f", displayFps);
            ImGui::Text("Frame: %.2f ms (min: %.2f, max: %.2f)", displayFrameTime, minFrameTime, maxFrameTime);
            ImGui::Separator();
            
            // Grid controls
            ImGui::Text("Grid Settings");
            ImGui::SliderInt("Grid Size", &gridSize, 1, 100);
            ImGui::SliderFloat("Spacing", &spacing, 1.0f, 10.0f, "%.1f");
            ImGui::Text("Total Ducks: %d", gridSize * gridSize);
            ImGui::Separator();
            
            // Rendering stats
            ImGui::Text("Rendering");
            ImGui::Text("Instances: %u", renderSystem->instances_count);
            ImGui::Text("Draw calls: %u", renderSystem->draw_calls);
            ImGui::Text("Triangles: %u", (duck.totalIndices / 3) * renderSystem->instances_count);
            ImGui::Separator();
            
            // Animation
            ImGui::Text("Animation");
            ImGui::Checkbox("Auto Rotate", &autoRotate);
            if (autoRotate) {
                ImGui::SliderFloat("Rotation Speed", &rotationSpeed, 0.0f, 360.0f, "%.0f deg/s");
            }
            ImGui::Separator();
            
            // Camera info
            ImGui::Text("Camera");
            ImGui::Text("Position: %.1f, %.1f, %.1f", camera.position.x, camera.position.y, camera.position.z);
           
            
            ImGui::End();
            
            renderer.imguiRender(cmd);
            
            // Respawn if grid changed
            if (gridSize != prevGridSize || spacing != prevSpacing) {
                vkDeviceWaitIdle(renderer.getDevice());
                spawnDucks(ecs, gridSize, spacing);
                prevGridSize = gridSize;
                prevSpacing = spacing;
            }
        }
        
        renderer.endFrame(cmd);
        Input::update();
    }
    
    std::cout << "\nCleaning up...\n";
    vkDeviceWaitIdle(renderer.getDevice());
    
    renderSystem->instanceBuffer.cleanup();
    modelLoader.cleanup(duck);
    modelLoader.cleanupLoader();
    pipeline.cleanup();
    vkDestroyDescriptorSetLayout(renderer.getDevice(), descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(renderer.getDevice(), descriptorPool, nullptr);
    renderer.cleanup();
    
    return 0;
}
