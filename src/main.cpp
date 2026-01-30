// VMA implementation
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

// Note: TINYGLTF_IMPLEMENTATION should be defined in exactly ONE .cpp file
// If you have GLBLoader.cpp, define it there. Otherwise uncomment below:
// #define TINYGLTF_IMPLEMENTATION
// #define STB_IMAGE_IMPLEMENTATION  
// #define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include "Renderer.h"
#include "PipelineInstanced.h"
#include "ModelLoader.h"
#include "Camera.h"
#include "CameraController.h"
#include "Config.h"
#include "Input.h"
#include "Time.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cstdlib>
#include <cmath>

// Simple frustum for culling
struct Frustum {
    glm::vec4 planes[6];
    
    void update(const glm::mat4& vp) {
        // Extract frustum planes from view-projection matrix
        planes[0] = glm::vec4(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]); // Left
        planes[1] = glm::vec4(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]); // Right
        planes[2] = glm::vec4(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]); // Bottom
        planes[3] = glm::vec4(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]); // Top
        planes[4] = glm::vec4(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2], vp[3][3] + vp[3][2]); // Near
        planes[5] = glm::vec4(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]); // Far
        
        for (int i = 0; i < 6; i++) {
            float len = glm::length(glm::vec3(planes[i]));
            planes[i] /= len;
        }
    }
    
    bool sphereInFrustum(glm::vec3 center, float radius) const {
        for (int i = 0; i < 6; i++) {
            if (glm::dot(glm::vec3(planes[i]), center) + planes[i].w + radius < 0) {
                return false;
            }
        }
        return true;
    }
};

// Entity data
struct Entity {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 angularVel;
    glm::vec4 color;
};

// Scene data
struct Scene {
    std::string name;
    Model model;
    std::vector<Entity> entities;
    glm::vec3 lightDir;
    glm::vec3 lightColor;
    float ambientStrength;
};

int main() {
    // Force X11 on Wayland to avoid Hyprland window sizing issues
    setenv("GDK_BACKEND", "x11", 0);
    setenv("SDL_VIDEODRIVER", "x11", 0);
    if (getenv("WAYLAND_DISPLAY")) {
        std::cout << "Wayland detected, using XWayland for stable window size" << std::endl;
    }
    
    setenv("GLYCIN_USE_SANDBOX", "0", 1);
    
    std::cout << "=== Zero Engine v0.8 - SCENE SYSTEM ===" << std::endl;
    std::cout << "Press J to toggle between scenes" << std::endl << std::endl;

    Config config;
    config.load("config.ini");
    
    uint32_t width = config.get("window_width", 960);
    uint32_t height = config.get("window_height", 540);
    std::string title = config.getString("window_title", "Zero Engine - Scene System");

    VulkanRenderer renderer;
    if (!renderer.init(width, height, title.c_str())) {
        std::cerr << "Renderer init failed!" << std::endl;
        return -1;
    }

    Time::init();
    Input::init(renderer.getWindow());

    // Create descriptor pool
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 100;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 100;

    VkDescriptorPool descriptorPool;
    vkCreateDescriptorPool(renderer.getDevice(), &poolInfo, nullptr, &descriptorPool);

    InstancedPipeline pipeline;
    pipeline.init(renderer.getDevice(), renderer.getRenderPass(),
                  "shaders/instanced_vert.spv", "shaders/instanced_frag.spv");

    ModelLoader modelLoader;
    modelLoader.init(renderer.getDevice(), renderer.getAllocator(), 
                    renderer.getCommandPool(), renderer.getGraphicsQueue(),
                    descriptorPool, pipeline.getDescriptorSetLayout());

    // === CREATE SCENES ===
    std::vector<Scene> scenes(2);
    int currentSceneIndex = 0;
    
    // Scene 0: Duck Grid
    {
        std::cout << "Loading Scene 0: Duck Grid..." << std::endl;
        scenes[0].name = "Duck Grid";
        scenes[0].model = modelLoader.load("models/Duck.glb");
        scenes[0].lightDir = glm::normalize(glm::vec3(1, -1, 1));
        scenes[0].lightColor = glm::vec3(1.0f, 1.0f, 0.9f);
        scenes[0].ambientStrength = 0.5f;
        
        if (scenes[0].model.vertices.empty()) {
            std::cerr << "Failed to load Duck.glb!" << std::endl;
            return -1;
        }
        
        std::cout << "  Vertices: " << scenes[0].model.vertices.size() << std::endl;
        std::cout << "  Indices: " << scenes[0].model.indices.size() << std::endl;
        std::cout << "  Submeshes: " << scenes[0].model.submeshes.size() << std::endl;
        
        // Create duck grid
        int gridSize = config.get("grid_size", 5);
        float spacing = 3.0f;
        for (int x = -gridSize; x <= gridSize; x++) {
            for (int z = -gridSize; z <= gridSize; z++) {
                Entity e;
                e.position = glm::vec3(x * spacing, 0.0f, z * spacing);
                e.rotation = glm::vec3(0);
                e.angularVel = glm::vec3(0.0f, 0.5f + (x + z) * 0.1f, 0.0f);
                e.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                scenes[0].entities.push_back(e);
            }
        }
        std::cout << "  Entities: " << scenes[0].entities.size() << std::endl;
    }
    
    // Scene 1: Tree Grid
    {
        std::cout << "\nLoading Scene 1: Tree Grid..." << std::endl;
        scenes[1].name = "Tree Grid";
        scenes[1].model = modelLoader.load("models/tree.glb");
        scenes[1].lightDir = glm::normalize(glm::vec3(0.5f, -1, 0.8f));
        scenes[1].lightColor = glm::vec3(1.0f, 0.95f, 0.8f);
        scenes[1].ambientStrength = 0.4f;
        
        if (scenes[1].model.vertices.empty()) {
            std::cerr << "Failed to load tree.glb!" << std::endl;
            return -1;
        }
        
        std::cout << "  Vertices: " << scenes[1].model.vertices.size() << std::endl;
        std::cout << "  Indices: " << scenes[1].model.indices.size() << std::endl;
        std::cout << "  Submeshes: " << scenes[1].model.submeshes.size() << std::endl;
        
        // Create tree forest
        int gridSize = 7;
        float spacing = 5.0f;
        for (int x = -gridSize; x <= gridSize; x++) {
            for (int z = -gridSize; z <= gridSize; z++) {
                Entity e;
                e.position = glm::vec3(x * spacing, 0.0f, z * spacing);
                e.rotation = glm::vec3(0, (x * 37 + z * 73) % 360, 0); // Random rotation
                e.angularVel = glm::vec3(0.0f, 0.0f, 0.0f); // Trees don't spin
                float scale = 0.8f + ((x + z) % 5) * 0.1f;
                e.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                scenes[1].entities.push_back(e);
            }
        }
        std::cout << "  Entities: " << scenes[1].entities.size() << std::endl;
    }

    // Camera setup
    Camera camera;
    camera.position = glm::vec3(0, 5, 15);
    camera.target = glm::vec3(0);
    camera.aspectRatio = (float)width / (float)height;
    camera.fov = 45.0f;
    camera.nearPlane = 0.1f;
    camera.farPlane = 200.0f;

    CameraController cameraController(&camera, config);
    cameraController.moveSpeed = 10.0f;
    cameraController.sprintMultiplier = 2.0f;


    // Instance buffer (allocate for max entities)
    size_t maxEntities = std::max(scenes[0].entities.size(), scenes[1].entities.size());
    InstanceBuffer instanceBuffer;
    if (!instanceBuffer.create(renderer.getAllocator(), maxEntities)) {
        std::cerr << "Instance buffer failed!" << std::endl;
        return -1;
    }

    std::vector<InstanceData> instanceData(maxEntities);
    Frustum frustum;

    std::cout << "\n=== Controls ===" << std::endl;
    std::cout << "WASD - Move | Space/Ctrl - Up/Down | Shift - Sprint" << std::endl;
    std::cout << "Right Click - Mouse look | Scroll - Zoom" << std::endl;
    std::cout << "J - Toggle Scene | ESC - Exit" << std::endl;
    std::cout << "\n=== Running ===" << std::endl;
    std::cout << "Current Scene: " << scenes[currentSceneIndex].name << std::endl;

    int frameCount = 0;
    float fpsTimer = 0;
    int visibleCount = 0;
    bool jPressedLastFrame = false;

    while (!renderer.shouldClose() && !Input::getKey(Key::Escape)) {
        Time::update();
        float dt = Time::getDeltaTime();
        
        Input::update();
        renderer.pollEvents();
        cameraController.update(dt, renderer.getWindow());

        // Toggle scene with J key (press once)
        bool jPressed = Input::getKey(Key::J);
        if (jPressed && !jPressedLastFrame) {
            currentSceneIndex = (currentSceneIndex + 1) % scenes.size();
            std::cout << "\n>>> Switched to Scene: " << scenes[currentSceneIndex].name << " <<<\n" << std::endl;
        }
        jPressedLastFrame = jPressed;

        // Get current scene
        Scene& currentScene = scenes[currentSceneIndex];

        // Update entities
        for (auto& e : currentScene.entities) {
            e.rotation += e.angularVel * dt;
        }

        // Update frustum
        glm::mat4 viewProj = camera.getViewProjectionMatrix();
        frustum.update(viewProj);

        // Build visible instance list with frustum culling
        visibleCount = 0;
        for (size_t i = 0; i < currentScene.entities.size(); i++) {
            const auto& e = currentScene.entities[i];
            
            // Frustum cull (assume radius 2)
            if (!frustum.sphereInFrustum(e.position, 2.0f)) continue;
            
            // Build model matrix - optimized rotation
            float cx = cosf(e.rotation.x), sx = sinf(e.rotation.x);
            float cy = cosf(e.rotation.y), sy = sinf(e.rotation.y);
            float cz = cosf(e.rotation.z), sz = sinf(e.rotation.z);
            
            glm::mat4& m = instanceData[visibleCount].model;
            m[0][0] = cy * cz;
            m[0][1] = cy * sz;
            m[0][2] = -sy;
            m[0][3] = 0.0f;
            
            m[1][0] = sx * sy * cz - cx * sz;
            m[1][1] = sx * sy * sz + cx * cz;
            m[1][2] = sx * cy;
            m[1][3] = 0.0f;
            
            m[2][0] = cx * sy * cz + sx * sz;
            m[2][1] = cx * sy * sz - sx * cz;
            m[2][2] = cx * cy;
            m[2][3] = 0.0f;
            
            m[3][0] = e.position.x;
            m[3][1] = e.position.y;
            m[3][2] = e.position.z;
            m[3][3] = 1.0f;
            
            instanceData[visibleCount].color = e.color;
            visibleCount++;
        }

        // Upload instances
        instanceBuffer.update(instanceData);

        // Render
        VkCommandBuffer cmd;
        renderer.beginFrame(cmd);
        
        // Query actual window size
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(renderer.getWindow(), &fbWidth, &fbHeight);
        
        VkViewport viewport{0, 0, (float)fbWidth, (float)fbHeight, 0, 1};
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        
        VkRect2D scissor{{0, 0}, {(uint32_t)fbWidth, (uint32_t)fbHeight}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        
        // Update camera aspect ratio if window resized
        if (fbWidth > 0 && fbHeight > 0) {
            camera.aspectRatio = (float)fbWidth / (float)fbHeight;
        }

        pipeline.bind(cmd);
        pipeline.bindDescriptorSet(cmd, currentScene.model.descriptorSet);

        PushConstantsInstanced pc{};
        pc.viewProj = viewProj;
        pc.lightDir = glm::normalize(currentScene.lightDir);
        pc.lightColor = currentScene.lightColor;
        pc.ambientStrength = currentScene.ambientStrength;
        pipeline.pushConstants(cmd, pc);

        // Bind vertex buffer (binding 0)
        VkBuffer vertexBuffers[] = {currentScene.model.combinedVertexBuffer};
        VkDeviceSize vertexOffsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, vertexOffsets);
        
        // Bind instance buffer (binding 1)
        VkBuffer instanceBuffers[] = {instanceBuffer.getBuffer()};
        VkDeviceSize instanceOffsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 1, 1, instanceBuffers, instanceOffsets);
        
        // Bind index buffer and draw
        vkCmdBindIndexBuffer(cmd, currentScene.model.combinedIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, currentScene.model.totalIndices, visibleCount, 0, 0, 0);

        renderer.endFrame(cmd);

        // FPS counter
        frameCount++;
        fpsTimer += dt;
        if (fpsTimer >= 1.0f) {
            std::cout << "FPS: " << frameCount 
                      << " | Scene: " << currentScene.name
                      << " | Visible: " << visibleCount << "/" << currentScene.entities.size()
                      << " | Culled: " << (currentScene.entities.size() - visibleCount) << std::endl;
            frameCount = 0;
            fpsTimer = 0;
        }
    }

    std::cout << "\nShutting down..." << std::endl;
    
    instanceBuffer.cleanup();
    
    // Cleanup all scenes
    for (auto& scene : scenes) {
        modelLoader.cleanup(scene.model);
    }
    
    modelLoader.cleanupLoader();
    vkDestroyDescriptorPool(renderer.getDevice(), descriptorPool, nullptr);
    pipeline.cleanup();
    renderer.cleanup();

    std::cout << "Done!" << std::endl;
    return 0;
}
