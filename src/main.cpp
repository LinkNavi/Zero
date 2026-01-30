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
#include "GLBLoaderOpt.h"
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

int main() {
    // Force X11 on Wayland to avoid Hyprland window sizing issues
    setenv("GDK_BACKEND", "x11", 0);
    setenv("SDL_VIDEODRIVER", "x11", 0);
    // Try XWayland first - more stable window sizing
    if (getenv("WAYLAND_DISPLAY")) {
        std::cout << "Wayland detected, using XWayland for stable window size" << std::endl;
    }
    
    setenv("GLYCIN_USE_SANDBOX", "0", 1);
    
    std::cout << "=== Zero Engine v0.7 - OPTIMIZED ===" << std::endl;
    std::cout << "Instanced rendering | Frustum culling | D16 depth" << std::endl << std::endl;

    Config config;
    config.load("config.ini");
    
    uint32_t width = config.get("window_width", 960);
    uint32_t height = config.get("window_height", 540);
    std::string title = config.getString("window_title", "Zero Engine - OPTIMIZED");

    VulkanRenderer renderer;
    if (!renderer.init(width, height, title.c_str())) {
        std::cerr << "Renderer init failed!" << std::endl;
        return -1;
    }

    Time::init();
    Input::init(renderer.getWindow());

    InstancedPipeline pipeline;
    if (!pipeline.init(renderer.getDevice(), renderer.getRenderPass(),
                      "shaders/instanced_vert.spv", "shaders/instanced_frag.spv")) {
        std::cerr << "Pipeline init failed!" << std::endl;
        renderer.cleanup();
        return -1;
    }

    std::string modelPath = config.getString("model_path", "models/tree.glb");
    // Remove quotes if present
    if (modelPath.front() == '"') modelPath = modelPath.substr(1, modelPath.size() - 2);
    
    GLBModelOpt model;
    if (!model.load(modelPath, renderer.getAllocator(), renderer.getDevice(),
                    renderer.getCommandPool(), renderer.getGraphicsQueue(),
                    pipeline.getDescriptorSetLayout())) {
        std::cerr << "Model load failed!" << std::endl;
        pipeline.cleanup();
        renderer.cleanup();
        return -1;
    }

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

    // Create entities
    int gridSize = config.get("grid_size", 10);
    float spacing = config.get("grid_spacing", 5.0f);
    
    std::vector<Entity> entities;
    for (int x = -gridSize; x <= gridSize; x++) {
        for (int z = -gridSize; z <= gridSize; z++) {
            Entity e;
            e.position = glm::vec3(x * spacing, (x + z) * 0.2f, z * spacing);
            e.rotation = glm::vec3(0);
            e.angularVel = glm::vec3(0.3f * x * 0.1f, 0.5f + z * 0.1f, 0.2f);
            // Use white color to show actual texture colors
            e.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            entities.push_back(e);
        }
    }
    
    std::cout << "Created " << entities.size() << " entities" << std::endl;

    // Instance buffer
    InstanceBuffer instanceBuffer;
    if (!instanceBuffer.create(renderer.getAllocator(), entities.size())) {
        std::cerr << "Instance buffer failed!" << std::endl;
        return -1;
    }

    std::vector<InstanceData> instanceData(entities.size());
    Frustum frustum;
    
    glm::vec3 lightDir = config.getVec3("light_direction", glm::vec3(1, -1, 1));
    glm::vec3 lightColor = config.getVec3("light_color", glm::vec3(1));
    float ambientStrength = config.get("ambient_strength", 0.5f);

    std::cout << "\n=== Controls ===" << std::endl;
    std::cout << "WASD - Move | Space/Ctrl - Up/Down | Shift - Sprint" << std::endl;
    std::cout << "Right Click - Mouse look | Scroll - Zoom | ESC - Exit" << std::endl;
    std::cout << "\n=== Running ===" << std::endl;

    int frameCount = 0;
    float fpsTimer = 0;
    int visibleCount = 0;

    while (!renderer.shouldClose() && !Input::getKey(Key::Escape)) {
        Time::update();
        float dt = Time::getDeltaTime();
        
        Input::update();
        renderer.pollEvents();
        cameraController.update(dt, renderer.getWindow());

        // Update entities
        for (auto& e : entities) {
            e.rotation += e.angularVel * dt;
        }

        // Update frustum
        glm::mat4 viewProj = camera.getViewProjectionMatrix();
        frustum.update(viewProj);

        // Build visible instance list with frustum culling
        visibleCount = 0;
        for (size_t i = 0; i < entities.size(); i++) {
            const auto& e = entities[i];
            
            // Frustum cull (assume radius 2 for ducks)
            if (!frustum.sphereInFrustum(e.position, 2.0f)) continue;
            
            // Build model matrix - optimized: combine rotation into single matrix
            float cx = cosf(e.rotation.x), sx = sinf(e.rotation.x);
            float cy = cosf(e.rotation.y), sy = sinf(e.rotation.y);
            float cz = cosf(e.rotation.z), sz = sinf(e.rotation.z);
            
            // Combined rotation matrix (ZYX order) with translation
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
        
        // Query actual window size (Hyprland may have resized it)
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
        pipeline.bindDescriptorSet(cmd, model.descriptorSet);

        PushConstantsInstanced pc{};
        pc.viewProj = viewProj;
        pc.lightDir = glm::normalize(lightDir);
        pc.lightColor = lightColor;
        pc.ambientStrength = ambientStrength;
        pipeline.pushConstants(cmd, pc);

        // Bind vertex buffer (binding 0)
        VkBuffer vertexBuffers[] = {model.combinedVertexBuffer};
        VkDeviceSize vertexOffsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, vertexOffsets);
        
        // Bind instance buffer (binding 1)
        VkBuffer instanceBuffers[] = {instanceBuffer.getBuffer()};
        VkDeviceSize instanceOffsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 1, 1, instanceBuffers, instanceOffsets);
        
        // Bind index buffer and draw ALL visible instances in ONE call
        vkCmdBindIndexBuffer(cmd, model.combinedIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, model.totalIndices, visibleCount, 0, 0, 0);

        renderer.endFrame(cmd);

        // FPS counter
        frameCount++;
        fpsTimer += dt;
        if (fpsTimer >= 1.0f) {
            std::cout << "FPS: " << frameCount 
                      << " | Visible: " << visibleCount << "/" << entities.size()
                      << " | Culled: " << (entities.size() - visibleCount) << std::endl;
            frameCount = 0;
            fpsTimer = 0;
        }
    }

    std::cout << "\nShutting down..." << std::endl;
    
    instanceBuffer.cleanup();
    model.cleanup();
    pipeline.cleanup();
    renderer.cleanup();

    std::cout << "Done!" << std::endl;
    return 0;
}
