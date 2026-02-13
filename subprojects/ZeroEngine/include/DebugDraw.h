#pragma once
#include "Renderer.h"
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>

class DebugGrid {
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation vertexAllocation;
    uint32_t vertexCount = 0;
    VmaAllocator allocator;
    
public:
    bool create(VulkanRenderer* renderer, int size = 20, float spacing = 1.0f) {
        allocator = renderer->getAllocator();
        
        std::vector<glm::vec3> vertices;
        
        // Grid lines on XZ plane
        float halfSize = size * spacing / 2.0f;
        for (int i = -size/2; i <= size/2; i++) {
            float pos = i * spacing;
            
            // Lines parallel to X axis
            vertices.push_back(glm::vec3(-halfSize, 0.0f, pos));
            vertices.push_back(glm::vec3( halfSize, 0.0f, pos));
            
            // Lines parallel to Z axis
            vertices.push_back(glm::vec3(pos, 0.0f, -halfSize));
            vertices.push_back(glm::vec3(pos, 0.0f,  halfSize));
        }
        
        vertexCount = static_cast<uint32_t>(vertices.size());
        VkDeviceSize bufferSize = sizeof(glm::vec3) * vertices.size();
        
        // Create vertex buffer
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        
        if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &vertexBuffer, &vertexAllocation, nullptr) != VK_SUCCESS) {
            return false;
        }
        
        // Copy vertex data
        void* data;
        vmaMapMemory(allocator, vertexAllocation, &data);
        memcpy(data, vertices.data(), bufferSize);
        vmaUnmapMemory(allocator, vertexAllocation);
        
        return true;
    }
    
    void draw(VkCommandBuffer cmd) {
        VkBuffer buffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
        vkCmdDraw(cmd, vertexCount, 1, 0, 0);
    }
    
    void cleanup() {
        if (vertexBuffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(allocator, vertexBuffer, vertexAllocation);
        }
    }
};

class DebugAxis {
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation vertexAllocation;
    VmaAllocator allocator;
    
public:
    bool create(VulkanRenderer* renderer, float length = 10.0f) {
        allocator = renderer->getAllocator();
        
        // Axis lines with colors
        struct AxisVertex {
            glm::vec3 position;
            glm::vec3 color;
        };
        
        std::vector<AxisVertex> vertices = {
            // X axis (red)
            {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
            {{length, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
            
            // Y axis (green)
            {{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
            {{0.0f, length, 0.0f}, {0.0f, 1.0f, 0.0f}},
            
            // Z axis (blue)
            {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            {{0.0f, 0.0f, length}, {0.0f, 0.0f, 1.0f}},
        };
        
        VkDeviceSize bufferSize = sizeof(AxisVertex) * vertices.size();
        
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        
        if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &vertexBuffer, &vertexAllocation, nullptr) != VK_SUCCESS) {
            return false;
        }
        
        void* data;
        vmaMapMemory(allocator, vertexAllocation, &data);
        memcpy(data, vertices.data(), bufferSize);
        vmaUnmapMemory(allocator, vertexAllocation);
        
        return true;
    }
    
    void draw(VkCommandBuffer cmd) {
        VkBuffer buffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
        vkCmdDraw(cmd, 6, 1, 0, 0);
    }
    
    void cleanup() {
        if (vertexBuffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(allocator, vertexBuffer, vertexAllocation);
        }
    }
};
