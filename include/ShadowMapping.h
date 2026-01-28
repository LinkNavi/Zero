#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

struct ShadowConfig {
    bool enabled = true;
    int resolution = 2048;
    float bias = 0.005f;
    float normalBias = 0.01f;
    int pcfSamples = 4;
    float shadowDistance = 50.0f;
    int cascadeCount = 3;
};

class ShadowMapper {
    VkDevice device = VK_NULL_HANDLE;
    VmaAllocator allocator = VK_NULL_HANDLE;
    
    VkImage shadowMap = VK_NULL_HANDLE;
    VkImageView shadowView = VK_NULL_HANDLE;
    VmaAllocation shadowAllocation;
    
    VkRenderPass shadowPass = VK_NULL_HANDLE;
    VkFramebuffer shadowFramebuffer = VK_NULL_HANDLE;
    
    VkPipeline shadowPipeline = VK_NULL_HANDLE;
    VkPipelineLayout shadowLayout = VK_NULL_HANDLE;
    
    VkSampler shadowSampler = VK_NULL_HANDLE;
    
public:
    ShadowConfig config;
    
    bool init(VkDevice dev, VmaAllocator alloc);
    void beginShadowPass(VkCommandBuffer cmd);
    void endShadowPass(VkCommandBuffer cmd);
    glm::mat4 getLightViewProjection(glm::vec3 lightDir, glm::vec3 sceneCenter, float sceneRadius);
    VkImageView getShadowMap() { return shadowView; }
    void cleanup();
};
