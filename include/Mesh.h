#pragma once
#include "Renderer.h"
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>

struct VertexWithNormal {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
    
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(VertexWithNormal);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }
    
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);
        
        // Position
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(VertexWithNormal, position);
        
        // Color
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(VertexWithNormal, color);
        
        // Normal
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(VertexWithNormal, normal);
        
        return attributeDescriptions;
    }
};

class LitMesh {
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation vertexAllocation;
    uint32_t vertexCount = 0;
    VmaAllocator allocator;
    
public:
    bool createCube(VulkanRenderer* renderer) {
        allocator = renderer->getAllocator();
        
        // Cube with proper normals (each face has its own normal)
        std::vector<VertexWithNormal> vertices = {
            // Front face (normal: 0, 0, 1)
            {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.2f, 0.2f}, {0.0f, 0.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.2f, 0.2f}, {0.0f, 0.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.2f, 0.2f}, {0.0f, 0.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.2f, 0.2f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.2f, 0.2f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.2f, 0.2f}, {0.0f, 0.0f, 1.0f}},
            
            // Back face (normal: 0, 0, -1)
            {{ 0.5f, -0.5f, -0.5f}, {0.2f, 1.0f, 0.2f}, {0.0f, 0.0f, -1.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.2f, 1.0f, 0.2f}, {0.0f, 0.0f, -1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.2f, 1.0f, 0.2f}, {0.0f, 0.0f, -1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.2f, 1.0f, 0.2f}, {0.0f, 0.0f, -1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.2f, 1.0f, 0.2f}, {0.0f, 0.0f, -1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {0.2f, 1.0f, 0.2f}, {0.0f, 0.0f, -1.0f}},
            
            // Top face (normal: 0, 1, 0)
            {{-0.5f,  0.5f,  0.5f}, {0.2f, 0.2f, 1.0f}, {0.0f, 1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.2f, 0.2f, 1.0f}, {0.0f, 1.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.2f, 0.2f, 1.0f}, {0.0f, 1.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.2f, 0.2f, 1.0f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.2f, 0.2f, 1.0f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f,  0.5f,  0.5f}, {0.2f, 0.2f, 1.0f}, {0.0f, 1.0f, 0.0f}},
            
            // Bottom face (normal: 0, -1, 0)
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.2f}, {0.0f, -1.0f, 0.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.2f}, {0.0f, -1.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 0.2f}, {0.0f, -1.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 0.2f}, {0.0f, -1.0f, 0.0f}},
            {{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 0.2f}, {0.0f, -1.0f, 0.0f}},
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.2f}, {0.0f, -1.0f, 0.0f}},
            
            // Right face (normal: 1, 0, 0)
            {{ 0.5f, -0.5f,  0.5f}, {0.2f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {0.2f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.2f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.2f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.2f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.2f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
            
            // Left face (normal: -1, 0, 0)
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.2f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
            {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.2f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
            {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.2f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
            {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.2f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
            {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.2f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.2f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
        };
        
        vertexCount = static_cast<uint32_t>(vertices.size());
        VkDeviceSize bufferSize = sizeof(VertexWithNormal) * vertices.size();
        
        // Create vertex buffer
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        
        if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &vertexBuffer, &vertexAllocation, nullptr) != VK_SUCCESS) {
            std::cerr << "Failed to create vertex buffer!" << std::endl;
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
