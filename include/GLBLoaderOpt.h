#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <tiny_gltf.h>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <cstring>

struct GLBVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct GLBMeshOpt {
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VmaAllocation vertexAllocation;
    VmaAllocation indexAllocation;
    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;
    glm::vec4 baseColor = glm::vec4(1.0f);
};

struct GLBTextureOpt {
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VmaAllocation allocation;
};

class GLBModelOpt {
    VmaAllocator allocator = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    GLBTextureOpt defaultTexture;

public:
    std::vector<GLBMeshOpt> meshes;
    std::vector<GLBTextureOpt> textures;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    
    // Combined vertex/index buffers for all meshes
    VkBuffer combinedVertexBuffer = VK_NULL_HANDLE;
    VkBuffer combinedIndexBuffer = VK_NULL_HANDLE;
    VmaAllocation combinedVertexAllocation;
    VmaAllocation combinedIndexAllocation;
    uint32_t totalVertices = 0;
    uint32_t totalIndices = 0;

    bool load(const std::string& filepath, VmaAllocator alloc, VkDevice dev,
              VkCommandPool pool, VkQueue queue, VkDescriptorSetLayout descLayout) {
        allocator = alloc;
        device = dev;
        commandPool = pool;
        graphicsQueue = queue;

        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err, warn;

        if (!loader.LoadBinaryFromFile(&model, &err, &warn, filepath)) {
            std::cerr << "GLB load failed: " << err << std::endl;
            return false;
        }

        if (!createDefaultTexture()) return false;
        
        // Load first texture only (optimization for iGPU)
        loadFirstTexture(model);
        
        // Process all meshes into combined buffers
        if (!processMeshes(model)) return false;
        
        if (!createDescriptorPool(descLayout)) return false;
        if (!createDescriptorSet(descLayout)) return false;

        std::cout << "✓ GLB loaded: " << totalVertices << " verts, " 
                  << totalIndices << " indices" << std::endl;
        return true;
    }

    void draw(VkCommandBuffer cmd, uint32_t instanceCount) {
        VkBuffer buffers[] = {combinedVertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
        vkCmdBindIndexBuffer(cmd, combinedIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, totalIndices, instanceCount, 0, 0, 0);
    }

    void cleanup() {
        if (combinedVertexBuffer) vmaDestroyBuffer(allocator, combinedVertexBuffer, combinedVertexAllocation);
        if (combinedIndexBuffer) vmaDestroyBuffer(allocator, combinedIndexBuffer, combinedIndexAllocation);
        
        for (auto& tex : textures) {
            if (tex.sampler) vkDestroySampler(device, tex.sampler, nullptr);
            if (tex.imageView) vkDestroyImageView(device, tex.imageView, nullptr);
            if (tex.image) vmaDestroyImage(allocator, tex.image, tex.allocation);
        }
        
        if (defaultTexture.sampler) vkDestroySampler(device, defaultTexture.sampler, nullptr);
        if (defaultTexture.imageView) vkDestroyImageView(device, defaultTexture.imageView, nullptr);
        if (defaultTexture.image) vmaDestroyImage(allocator, defaultTexture.image, defaultTexture.allocation);
        
        if (descriptorPool) vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }

private:
    bool processMeshes(const tinygltf::Model& model) {
        std::vector<GLBVertex> allVertices;
        std::vector<uint32_t> allIndices;
        
        const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
        
        for (int nodeIdx : scene.nodes) {
            processNode(model, model.nodes[nodeIdx], glm::mat4(1.0f), allVertices, allIndices);
        }
        
        if (allVertices.empty()) return false;
        
        totalVertices = static_cast<uint32_t>(allVertices.size());
        totalIndices = static_cast<uint32_t>(allIndices.size());
        
        // Create combined vertex buffer
        VkDeviceSize vertexSize = sizeof(GLBVertex) * allVertices.size();
        VkBufferCreateInfo vbInfo{};
        vbInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vbInfo.size = vertexSize;
        vbInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        
        if (vmaCreateBuffer(allocator, &vbInfo, &allocInfo, &combinedVertexBuffer, 
                           &combinedVertexAllocation, nullptr) != VK_SUCCESS) {
            return false;
        }
        
        void* data;
        vmaMapMemory(allocator, combinedVertexAllocation, &data);
        memcpy(data, allVertices.data(), vertexSize);
        vmaUnmapMemory(allocator, combinedVertexAllocation);
        
        // Create combined index buffer
        VkDeviceSize indexSize = sizeof(uint32_t) * allIndices.size();
        VkBufferCreateInfo ibInfo{};
        ibInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        ibInfo.size = indexSize;
        ibInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        
        if (vmaCreateBuffer(allocator, &ibInfo, &allocInfo, &combinedIndexBuffer,
                           &combinedIndexAllocation, nullptr) != VK_SUCCESS) {
            return false;
        }
        
        vmaMapMemory(allocator, combinedIndexAllocation, &data);
        memcpy(data, allIndices.data(), indexSize);
        vmaUnmapMemory(allocator, combinedIndexAllocation);
        
        return true;
    }
    
    void processNode(const tinygltf::Model& model, const tinygltf::Node& node,
                     glm::mat4 parentTransform, std::vector<GLBVertex>& vertices,
                     std::vector<uint32_t>& indices) {
        glm::mat4 nodeTransform = parentTransform;
        
        if (node.matrix.size() == 16) {
            // Convert double array to float mat4
            glm::mat4 nodeMat;
            for (int i = 0; i < 16; i++) {
                nodeMat[i / 4][i % 4] = static_cast<float>(node.matrix[i]);
            }
            nodeTransform = parentTransform * nodeMat;
        } else {
            glm::mat4 T(1.0f), R(1.0f), S(1.0f);
            if (node.translation.size() == 3) {
                T = glm::translate(glm::mat4(1.0f), glm::vec3(node.translation[0], 
                    node.translation[1], node.translation[2]));
            }
            if (node.rotation.size() == 4) {
                glm::quat q(float(node.rotation[3]), float(node.rotation[0]),
                           float(node.rotation[1]), float(node.rotation[2]));
                R = glm::mat4_cast(q);
            }
            if (node.scale.size() == 3) {
                S = glm::scale(glm::mat4(1.0f), glm::vec3(node.scale[0],
                    node.scale[1], node.scale[2]));
            }
            nodeTransform = parentTransform * T * R * S;
        }
        
        if (node.mesh >= 0) {
            processMesh(model, model.meshes[node.mesh], nodeTransform, vertices, indices);
        }
        
        for (int childIdx : node.children) {
            processNode(model, model.nodes[childIdx], nodeTransform, vertices, indices);
        }
    }
    
    void processMesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh,
                     glm::mat4 transform, std::vector<GLBVertex>& vertices,
                     std::vector<uint32_t>& indices) {
        for (const auto& primitive : mesh.primitives) {
            if (primitive.attributes.find("POSITION") == primitive.attributes.end()) continue;
            
            uint32_t baseVertex = static_cast<uint32_t>(vertices.size());
            
            const auto& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
            const auto& posView = model.bufferViews[posAccessor.bufferView];
            const float* positions = reinterpret_cast<const float*>(
                &model.buffers[posView.buffer].data[posView.byteOffset + posAccessor.byteOffset]);
            
            const float* normals = nullptr;
            if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                const auto& normAccessor = model.accessors[primitive.attributes.at("NORMAL")];
                const auto& normView = model.bufferViews[normAccessor.bufferView];
                normals = reinterpret_cast<const float*>(
                    &model.buffers[normView.buffer].data[normView.byteOffset + normAccessor.byteOffset]);
            }
            
            const float* texCoords = nullptr;
            if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                const auto& uvAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                const auto& uvView = model.bufferViews[uvAccessor.bufferView];
                texCoords = reinterpret_cast<const float*>(
                    &model.buffers[uvView.buffer].data[uvView.byteOffset + uvAccessor.byteOffset]);
            }
            
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
            
            for (size_t i = 0; i < posAccessor.count; i++) {
                GLBVertex vertex;
                glm::vec4 pos = transform * glm::vec4(positions[i*3], positions[i*3+1], positions[i*3+2], 1.0f);
                vertex.position = glm::vec3(pos);
                
                if (normals) {
                    vertex.normal = glm::normalize(normalMatrix * 
                        glm::vec3(normals[i*3], normals[i*3+1], normals[i*3+2]));
                } else {
                    vertex.normal = glm::vec3(0, 1, 0);
                }
                
                vertex.texCoord = texCoords ? glm::vec2(texCoords[i*2], texCoords[i*2+1]) : glm::vec2(0);
                vertices.push_back(vertex);
            }
            
            if (primitive.indices >= 0) {
                const auto& idxAccessor = model.accessors[primitive.indices];
                const auto& idxView = model.bufferViews[idxAccessor.bufferView];
                const uint8_t* idxData = &model.buffers[idxView.buffer].data[
                    idxView.byteOffset + idxAccessor.byteOffset];
                
                for (size_t i = 0; i < idxAccessor.count; i++) {
                    uint32_t idx;
                    if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                        idx = reinterpret_cast<const uint16_t*>(idxData)[i];
                    } else if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                        idx = reinterpret_cast<const uint32_t*>(idxData)[i];
                    } else {
                        idx = idxData[i];
                    }
                    indices.push_back(baseVertex + idx);
                }
            }
        }
    }
    
    void loadFirstTexture(const tinygltf::Model& model) {
        if (model.textures.empty() || model.images.empty()) {
            std::cout << "  No textures in model, using default white" << std::endl;
            return;
        }
        
        const auto& gltfTex = model.textures[0];
        if (gltfTex.source < 0 || gltfTex.source >= (int)model.images.size()) {
            std::cout << "  Invalid texture source index" << std::endl;
            return;
        }
        
        const auto& image = model.images[gltfTex.source];
        if (image.image.empty()) {
            std::cout << "  Texture image data is empty" << std::endl;
            return;
        }
        
        std::cout << "  Loading texture: " << image.width << "x" << image.height 
                  << " (" << image.component << " components)" << std::endl;
        
        GLBTextureOpt tex;
        if (createTexture(image, tex)) {
            textures.push_back(tex);
            std::cout << "  ✓ Texture loaded successfully" << std::endl;
        } else {
            std::cout << "  ✗ Failed to create texture" << std::endl;
        }
    }
    
    bool createTexture(const tinygltf::Image& image, GLBTextureOpt& tex) {
        uint32_t w = image.width, h = image.height;
        VkDeviceSize imageSize = w * h * 4;
        
        std::vector<uint8_t> rgba(imageSize);
        if (image.component == 4) {
            memcpy(rgba.data(), image.image.data(), imageSize);
        } else if (image.component == 3) {
            for (int i = 0; i < w * h; i++) {
                rgba[i*4+0] = image.image[i*3+0];
                rgba[i*4+1] = image.image[i*3+1];
                rgba[i*4+2] = image.image[i*3+2];
                rgba[i*4+3] = 255;
            }
        } else return false;
        
        // Staging buffer
        VkBuffer staging;
        VmaAllocation stagingAlloc;
        
        VkBufferCreateInfo bufInfo{};
        bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size = imageSize;
        bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        
        vmaCreateBuffer(allocator, &bufInfo, &allocInfo, &staging, &stagingAlloc, nullptr);
        
        void* data;
        vmaMapMemory(allocator, stagingAlloc, &data);
        memcpy(data, rgba.data(), imageSize);
        vmaUnmapMemory(allocator, stagingAlloc);
        
        // NO MIPMAPS - single level for speed
        VkImageCreateInfo imgInfo{};
        imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imgInfo.imageType = VK_IMAGE_TYPE_2D;
        imgInfo.extent = {w, h, 1};
        imgInfo.mipLevels = 1;
        imgInfo.arrayLayers = 1;
        imgInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        
        VmaAllocationCreateInfo imgAllocInfo{};
        imgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        
        vmaCreateImage(allocator, &imgInfo, &imgAllocInfo, &tex.image, &tex.allocation, nullptr);
        
        // Transfer
        VkCommandBuffer cmd = beginCmd();
        
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.image = tex.image;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        VkBufferImageCopy region{};
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageExtent = {w, h, 1};
        
        vkCmdCopyBufferToImage(cmd, staging, tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                            0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        endCmd(cmd);
        vmaDestroyBuffer(allocator, staging, stagingAlloc);
        
        // View
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = tex.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        
        vkCreateImageView(device, &viewInfo, nullptr, &tex.imageView);
        
        // Sampler - NO ANISOTROPY for iGPU
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE;  // DISABLED for speed
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        
        vkCreateSampler(device, &samplerInfo, nullptr, &tex.sampler);
        
        return true;
    }
    
    bool createDefaultTexture() {
        uint8_t white[4] = {255, 255, 255, 255};
        
        VkBuffer staging;
        VmaAllocation stagingAlloc;
        
        VkBufferCreateInfo bufInfo{};
        bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size = 4;
        bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        
        vmaCreateBuffer(allocator, &bufInfo, &allocInfo, &staging, &stagingAlloc, nullptr);
        
        void* data;
        vmaMapMemory(allocator, stagingAlloc, &data);
        memcpy(data, white, 4);
        vmaUnmapMemory(allocator, stagingAlloc);
        
        VkImageCreateInfo imgInfo{};
        imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imgInfo.imageType = VK_IMAGE_TYPE_2D;
        imgInfo.extent = {1, 1, 1};
        imgInfo.mipLevels = 1;
        imgInfo.arrayLayers = 1;
        imgInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        
        VmaAllocationCreateInfo imgAllocInfo{};
        imgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        
        vmaCreateImage(allocator, &imgInfo, &imgAllocInfo, &defaultTexture.image, 
                      &defaultTexture.allocation, nullptr);
        
        VkCommandBuffer cmd = beginCmd();
        
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.image = defaultTexture.image;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        VkBufferImageCopy region{};
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageExtent = {1, 1, 1};
        
        vkCmdCopyBufferToImage(cmd, staging, defaultTexture.image, 
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                            0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        endCmd(cmd);
        vmaDestroyBuffer(allocator, staging, stagingAlloc);
        
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = defaultTexture.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        
        vkCreateImageView(device, &viewInfo, nullptr, &defaultTexture.imageView);
        
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        
        vkCreateSampler(device, &samplerInfo, nullptr, &defaultTexture.sampler);
        
        return true;
    }
    
    bool createDescriptorPool(VkDescriptorSetLayout) {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 1;
        
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 1;
        
        return vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) == VK_SUCCESS;
    }
    
    bool createDescriptorSet(VkDescriptorSetLayout layout) {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;
        
        if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
            return false;
        }
        
        // Use first texture if available, else default
        VkImageView view = textures.empty() ? defaultTexture.imageView : textures[0].imageView;
        VkSampler sampler = textures.empty() ? defaultTexture.sampler : textures[0].sampler;
        
        VkDescriptorImageInfo imgInfo{};
        imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imgInfo.imageView = view;
        imgInfo.sampler = sampler;
        
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptorSet;
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imgInfo;
        
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        return true;
    }
    
    VkCommandBuffer beginCmd() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        
        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(device, &allocInfo, &cmd);
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);
        
        return cmd;
    }
    
    void endCmd(VkCommandBuffer cmd) {
        vkEndCommandBuffer(cmd);
        
        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;
        
        vkQueueSubmit(graphicsQueue, 1, &submit, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);
        
        vkFreeCommandBuffers(device, commandPool, 1, &cmd);
    }
};
