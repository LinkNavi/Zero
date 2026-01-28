#pragma once
#include "Pipeline.h"
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>
#include <vk_mem_alloc.h>
struct VertexTextured {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 texCoord;
    
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(VertexTextured);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }
    
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);
        
        // Position
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(VertexTextured, position);
        
        // Color
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(VertexTextured, color);
        
        // TexCoord
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(VertexTextured, texCoord);
        
        return attributeDescriptions;
    }
};

class OBJLoader {
public:
    static bool loadOBJ(const std::string& filepath, std::vector<VertexTextured>& vertices) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open OBJ file: " << filepath << std::endl;
            return false;
        }
        
        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> texCoords;
        std::vector<glm::vec3> normals;
        
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;
            
            if (prefix == "v") {
                // Vertex position
                glm::vec3 pos;
                iss >> pos.x >> pos.y >> pos.z;
                positions.push_back(pos);
            }
            else if (prefix == "vt") {
                // Texture coordinate
                glm::vec2 tex;
                iss >> tex.x >> tex.y;
                texCoords.push_back(tex);
            }
            else if (prefix == "vn") {
                // Normal (we'll skip for now)
                glm::vec3 normal;
                iss >> normal.x >> normal.y >> normal.z;
                normals.push_back(normal);
            }
            else if (prefix == "f") {
                // Face - assumes triangulated faces
                std::string vertex1, vertex2, vertex3;
                iss >> vertex1 >> vertex2 >> vertex3;
                
                // Parse face format: v/vt/vn or v/vt or v//vn or v
                auto parseVertex = [&](const std::string& vertexStr) -> VertexTextured {
                    VertexTextured vertex;
                    vertex.color = glm::vec3(1.0f); // Default white
                    
                    size_t pos1 = vertexStr.find('/');
                    int posIndex = std::stoi(vertexStr.substr(0, pos1)) - 1;
                    vertex.position = positions[posIndex];
                    
                    if (pos1 != std::string::npos) {
                        size_t pos2 = vertexStr.find('/', pos1 + 1);
                        if (pos2 > pos1 + 1) {
                            int texIndex = std::stoi(vertexStr.substr(pos1 + 1, pos2 - pos1 - 1)) - 1;
                            if (texIndex >= 0 && texIndex < texCoords.size()) {
                                vertex.texCoord = texCoords[texIndex];
                            }
                        }
                    } else {
                        vertex.texCoord = glm::vec2(0.0f);
                    }
                    
                    return vertex;
                };
                
                vertices.push_back(parseVertex(vertex1));
                vertices.push_back(parseVertex(vertex2));
                vertices.push_back(parseVertex(vertex3));
            }
        }
        
        file.close();
        
        if (vertices.empty()) {
            std::cerr << "No vertices loaded from OBJ file!" << std::endl;
            return false;
        }
        
        std::cout << "✓ Loaded OBJ: " << filepath << " (" << vertices.size() << " vertices)" << std::endl;
        return true;
    }
};

class TexturedMesh {
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation vertexAllocation;
    uint32_t vertexCount = 0;
    VmaAllocator allocator;
    
public:
    bool createFromOBJ(const std::string& filepath, VmaAllocator alloc) {
        allocator = alloc;
        
        std::vector<VertexTextured> vertices;
        if (!OBJLoader::loadOBJ(filepath, vertices)) {
            return false;
        }
        
        vertexCount = static_cast<uint32_t>(vertices.size());
        VkDeviceSize bufferSize = sizeof(VertexTextured) * vertices.size();
        
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
