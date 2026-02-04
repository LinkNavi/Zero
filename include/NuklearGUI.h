#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

// Forward declare nuklear types
struct nk_context;
struct nk_font_atlas;
struct nk_buffer;

class NuklearGUI {
    nk_context* ctx = nullptr;
    nk_font_atlas* atlas = nullptr;
    nk_buffer* cmds = nullptr;
    
    VkDevice device;
    VmaAllocator allocator;
    
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VmaAllocation vertexAllocation;
    VmaAllocation indexAllocation;
    
    VkImage fontImage = VK_NULL_HANDLE;
    VkImageView fontView = VK_NULL_HANDLE;
    VkSampler fontSampler = VK_NULL_HANDLE;
    VmaAllocation fontAllocation;
    
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
    
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    
    GLFWwindow* window;
    uint32_t width, height;
    
    size_t maxVertexBuffer = 512 * 1024;
    size_t maxIndexBuffer = 128 * 1024;
    
public:
    bool init(VkDevice dev, VmaAllocator alloc, VkRenderPass renderPass, 
              GLFWwindow* win, uint32_t w, uint32_t h,
              VkCommandPool cmdPool, VkQueue queue);
    
    void newFrame();
    void render(VkCommandBuffer cmd);
    void cleanup();
    
    nk_context* getContext() { return ctx; }
    
private:
    bool createBuffers();
    bool uploadFontTexture(VkCommandPool cmdPool, VkQueue queue);
    bool createDescriptors();
    bool createPipeline(VkRenderPass renderPass);
};
