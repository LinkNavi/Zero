// include/BoneBuffer.h
#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>

class BoneBuffer {
    VmaAllocator allocator = nullptr;
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    void* mapped = nullptr;
    
public:
    void create(VmaAllocator alloc) {
        allocator = alloc;
        
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(glm::mat4) * 128;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        
        VmaAllocationInfo allocationInfo;
        vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, &allocationInfo);
        mapped = allocationInfo.pMappedData;
        
        // Initialize to identity
        std::vector<glm::mat4> identity(128, glm::mat4(1.0f));
        memcpy(mapped, identity.data(), sizeof(glm::mat4) * 128);
    }
    
    void update(const std::vector<glm::mat4>& bones) {
        size_t count = std::min(bones.size(), size_t(128));
        memcpy(mapped, bones.data(), sizeof(glm::mat4) * count);
    }
    
    VkBuffer getBuffer() const { return buffer; }
    
    void cleanup() {
        if (buffer) vmaDestroyBuffer(allocator, buffer, allocation);
        buffer = VK_NULL_HANDLE;
    }
};
