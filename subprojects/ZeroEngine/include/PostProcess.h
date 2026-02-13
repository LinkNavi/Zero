#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>

struct PostProcessConfig {
    bool enableBloom = false;
    float bloomThreshold = 1.0f;
    float bloomIntensity = 0.5f;
    
    bool enableVignette = false;
    float vignetteStrength = 0.3f;
    
    bool enableChromaticAberration = false;
    float chromaticAberrationStrength = 0.002f;
    
    bool enableGammaCorrection = true;
    float gamma = 2.2f;
    
    float exposure = 1.0f;
    float contrast = 1.0f;
    float saturation = 1.0f;
};

class PostProcessor {
    VkDevice device = VK_NULL_HANDLE;
    VmaAllocator allocator = VK_NULL_HANDLE;
    
    VkImage offscreenImage = VK_NULL_HANDLE;
    VkImageView offscreenView = VK_NULL_HANDLE;
    VmaAllocation offscreenAllocation;
    
    VkRenderPass offscreenPass = VK_NULL_HANDLE;
    VkFramebuffer offscreenFramebuffer = VK_NULL_HANDLE;
    
    VkPipeline postPipeline = VK_NULL_HANDLE;
    VkPipelineLayout postLayout = VK_NULL_HANDLE;
    
    VkDescriptorSetLayout descLayout = VK_NULL_HANDLE;
    VkDescriptorPool descPool = VK_NULL_HANDLE;
    VkDescriptorSet descSet = VK_NULL_HANDLE;
    
    VkSampler sampler = VK_NULL_HANDLE;
    
    uint32_t width, height;
    
public:
    PostProcessConfig config;
    
    bool init(VkDevice dev, VmaAllocator alloc, uint32_t w, uint32_t h, VkFormat format);
    void beginOffscreenPass(VkCommandBuffer cmd);
    void endOffscreenPass(VkCommandBuffer cmd);
    void applyEffects(VkCommandBuffer cmd, VkFramebuffer targetFB, VkRenderPass targetPass);
    void cleanup();
    
private:
    bool createOffscreenResources(VkFormat format);
    bool createDescriptors();
    bool createPipeline(VkRenderPass targetPass);
};
