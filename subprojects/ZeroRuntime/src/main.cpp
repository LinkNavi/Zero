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
    
    ResourcePath::init();
    Config config;
    config.load(ResourcePath::config("config.ini"));
    
    VulkanRenderer renderer;
    if (!renderer.init(1920, 1080, "Zero Runtime")) {
        std::cerr << "Failed to init renderer\n";
        return -1;
    }
    
    Input::init(renderer.getWindow());
    Time::init();
    
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
    vkCreateDescriptorPool(renderer.getDevice(), &poolInfo, nullptr, &descriptorPool);
    
    ShadowMap shadowMap;
    shadowMap.init(renderer.getDevice(), renderer.getAllocator());
    shadowMap.createPipeline(ResourcePath::shaders("shadow_vert.spv"));
    
    PostProcessing postProcess;
    postProcess.init(renderer.getDevice(), renderer.getAllocator(), descriptorPool,
                    renderer.getWidth(), renderer.getHeight(), VK_FORMAT_D32_SFLOAT,
                    ResourcePath::shaders("fullscreen_vert.spv"),
                    ResourcePath::shaders("bloom_frag.spv"),
                    ResourcePath::shaders("composite_frag.spv"));
    
    Pipeline pipeline;
    pipeline.init(renderer.getDevice(), postProcess.getSceneRenderPass(),
                 ResourcePath::shaders("unified_vert.spv"),
                 ResourcePath::shaders("unified_frag.spv"));
    
    ModelLoader modelLoader;
    modelLoader.init(renderer.getDevice(), renderer.getAllocator(),
                    renderer.getCommandPool(), renderer.getGraphicsQueue(),
                    descriptorPool, pipeline.getDescriptorLayout());
    
    Skybox skybox;
    std::vector<std::string> skyboxFaces = {
        ResourcePath::textures("skybox/right.jpg"),
        ResourcePath::textures("skybox/left.jpg"),
        ResourcePath::textures("skybox/top.jpg"),
        ResourcePath::textures("skybox/bottom.jpg"),
        ResourcePath::textures("skybox/front.jpg"),
        ResourcePath::textures("skybox/back.jpg")
    };
    skybox.init(renderer.getDevice(), renderer.getAllocator(), descriptorPool,
               postProcess.getSceneRenderPass(), renderer.getCommandPool(),
               renderer.getGraphicsQueue(),
               ResourcePath::shaders("skybox_vert.spv"),
               ResourcePath::shaders("skybox_frag.spv"), skyboxFaces);
    
    ECS ecs;
    ecs.registerComponent<Transform>();
    ecs.registerComponent<Tag>();
    ecs.registerComponent<Layer>();
    ecs.registerComponent<ModelComponent>();
    ecs.registerComponent<CameraComponent>();
    
    std::cout << "Loading: " << scenePath << "\n";
    if (!ScenePackaging::ScenePackager::loadScene(&ecs, scenePath)) {
        std::cerr << "⚠ Failed to load scene\n";
        
        EntityID cube = ecs.createEntity();
        Transform t;
        t.position = glm::vec3(0, 0, 0);
        t.scale = glm::vec3(1);
        t.rotation = glm::quat(1, 0, 0, 0);
        ecs.addComponent(cube, t);
        ecs.addComponent(cube, ModelComponent(ResourcePath::models("cube.obj")));
        
        EntityID camEntity = ecs.createEntity();
        Transform camTransform;
        camTransform.position = glm::vec3(0, 2, 5);
        ecs.addComponent(camEntity, camTransform);
        
        CameraComponent camComp;
        camComp.camera.position = glm::vec3(0, 2, 5);
        camComp.camera.aspectRatio = float(renderer.getWidth()) / float(renderer.getHeight());
        camComp.isActive = true;
        ecs.addComponent(camEntity, camComp);
        
        std::cout << "Created test scene\n";
    }
    
    std::cout << "Loading models...\n";
    for (EntityID e = 0; e < 10000; e++) {
        auto* mc = ecs.getComponent<ModelComponent>(e);
        if (mc && mc->loadedModel == nullptr && !mc->modelPath.empty()) {
            Model m = modelLoader.load(mc->modelPath);
            mc->loadedModel = new Model(std::move(m));
            std::cout << "  ✓ " << mc->modelPath << "\n";
        }
    }
    
    EntityID activeCameraEntity = 0;
    for (EntityID e = 0; e < 10000; e++) {
        auto* camComp = ecs.getComponent<CameraComponent>(e);
        if (camComp && camComp->isActive) {
            activeCameraEntity = e;
            break;
        }
    }
    
    if (activeCameraEntity == 0) {
        std::cerr << "No active camera!\n";
        return -1;
    }
    
    std::cout << "Controls: WASD+Mouse, ESC=Quit\n";
    
    using Clock = std::chrono::high_resolution_clock;
    auto lastTime = Clock::now();
    
    glm::vec3 lightPos(5, 10, 5);
    
    while (!renderer.shouldClose()) {
        auto now = Clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        
        renderer.pollEvents();
        if (Input::getKey(Key::Escape)) break;
        
        Time::update();
        ecs.updateSystems(dt);
        
        auto* camTransform = ecs.getComponent<Transform>(activeCameraEntity);
        auto* camComp = ecs.getComponent<CameraComponent>(activeCameraEntity);
        if (camTransform && camComp) {
            camComp->camera.position = camTransform->position;
            CameraController controller(&camComp->camera, config);
            controller.update(dt, renderer.getWindow());
            camTransform->position = camComp->camera.position;
        }
        
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0), glm::vec3(0, 1, 0));
        glm::mat4 lightProj = glm::ortho(-10.f, 10.f, -10.f, 10.f, 0.1f, 50.f);
        glm::mat4 lightMatrix = lightProj * lightView;
        
        VkCommandBuffer cmd;
        renderer.beginFrame(cmd);
        
        // Shadow pass
        shadowMap.beginShadowPass(cmd);
        for (EntityID e = 0; e < 10000; e++) {
            auto* transform = ecs.getComponent<Transform>(e);
            auto* mc = ecs.getComponent<ModelComponent>(e);
            if (!transform || !mc || !mc->loadedModel) continue;
            
            glm::mat4 model = transform->getLocalMatrix();
            
            ShadowPushConstants shadowPc;
            shadowPc.lightViewProj = lightMatrix;
            shadowPc.model = model;
            
            vkCmdPushConstants(cmd, shadowMap.pipelineLayout,
                             VK_SHADER_STAGE_VERTEX_BIT,
                             0, sizeof(ShadowPushConstants), &shadowPc);
            
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &mc->loadedModel->vertexBuffer, &offset);
            vkCmdBindIndexBuffer(cmd, mc->loadedModel->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, mc->loadedModel->totalIndices, 1, 0, 0, 0);
        }
        shadowMap.endShadowPass(cmd);
        
        // Scene pass
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.05f, 0.05f, 0.08f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        
        postProcess.beginScenePass(cmd, clearValues);
        
        skybox.render(cmd, camComp->camera.getViewMatrix(), camComp->camera.getProjectionMatrix());
        
        pipeline.bind(cmd);
        
        for (EntityID e = 0; e < 10000; e++) {
            auto* transform = ecs.getComponent<Transform>(e);
            auto* mc = ecs.getComponent<ModelComponent>(e);
            if (!transform || !mc || !mc->loadedModel) continue;
            
            glm::mat4 model = transform->getLocalMatrix();
            
            PushConstants pc;
            pc.model = model;
            pc.viewProj = camComp->camera.getProjectionMatrix() * camComp->camera.getViewMatrix();
            
            vkCmdPushConstants(cmd, pipeline.getPipelineLayout(),
                             VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                             0, sizeof(PushConstants), &pc);
            
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                   pipeline.getPipelineLayout(), 0, 1,
                                   &mc->loadedModel->descriptorSet, 0, nullptr);
            
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &mc->loadedModel->vertexBuffer, &offset);
            vkCmdBindIndexBuffer(cmd, mc->loadedModel->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, mc->loadedModel->totalIndices, 1, 0, 0, 0);
        }
        
        postProcess.endScenePass(cmd);
        
        postProcess.applyPostProcess(cmd, renderer.getRenderPass(),
                                     renderer.getCurrentFramebuffer());
        
        vkCmdEndRenderPass(cmd);
        
        renderer.endFrame(cmd);
        Input::update();
    }
    
    vkDeviceWaitIdle(renderer.getDevice());
    
    for (EntityID e = 0; e < 10000; e++) {
        auto* mc = ecs.getComponent<ModelComponent>(e);
        if (mc && mc->loadedModel) {
            modelLoader.cleanup(*mc->loadedModel);
            delete mc->loadedModel;
        }
    }
    
    modelLoader.cleanupLoader();
    pipeline.cleanup();
    shadowMap.cleanup();
    postProcess.cleanup();
    skybox.cleanup();
    vkDestroyDescriptorPool(renderer.getDevice(), descriptorPool, nullptr);
    renderer.cleanup();
    
    return 0;
}
