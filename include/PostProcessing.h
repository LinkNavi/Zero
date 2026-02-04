#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <string>

struct BloomSettings {
    float threshold = 1.0f;
    float intensity = 1.0f;
    float strength = 0.5f;
    bool enabled = false;
};

struct PostProcessSettings {
    BloomSettings bloom;
};

class PostProcessing {
    VkDevice device = VK_NULL_HANDLE;
    VmaAllocator allocator = nullptr;
    
public:
    PostProcessSettings settings;
    
    bool init(VkDevice dev, VmaAllocator alloc, VkDescriptorPool pool,
              uint32_t w, uint32_t h,
              const std::string& vertPath,
              const std::string& extractPath,
              const std::string& blurPath,
              const std::string& compositePath) {
        device = dev;
        allocator = alloc;
        // Bloom not fully implemented - requires offscreen rendering
        return true;
    }
    
    void applyBloom(VkCommandBuffer cmd, VkImageView sceneView) {
        // TODO: Implement when offscreen rendering is set up
    }
    
    void cleanup() {
        // Nothing to clean up yet
    }
};
