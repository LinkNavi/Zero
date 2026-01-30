#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include "stb_image.h"
#include "stb_image_write.h"

#include <vector>
#include "iomanip"
#include <unordered_map>
#include <iostream>
#include <filesystem>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec4 color{1.0f};
    glm::ivec4 boneIds{-1};
    glm::vec4 boneWeights{0.0f};
};

struct Texture {
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    uint32_t width = 0, height = 0;
    std::string path;
};

struct SubMesh {
    uint32_t indexOffset = 0;
    uint32_t indexCount = 0;
    uint32_t vertexOffset = 0;
    uint32_t materialIndex = 0;
    std::string name;
};

struct MaterialData {
    glm::vec4 baseColor{1.0f};
    glm::vec3 emissive{0.0f};
    float metallic = 0.0f;
    float roughness = 1.0f;
    float ao = 1.0f;
    int albedoTexture = -1;
    int normalTexture = -1;
    int metallicRoughnessTexture = -1;
    int emissiveTexture = -1;
    std::string name;
};

struct BoneInfo {
    glm::mat4 offset{1.0f};
    glm::mat4 finalTransform{1.0f};
    std::string name;
    int parentIndex = -1;
};

struct Animation {
    std::string name;
    float duration = 0.0f;
    float ticksPerSecond = 25.0f;
    
    struct Channel {
        std::string nodeName;
        std::vector<std::pair<float, glm::vec3>> positions;
        std::vector<std::pair<float, glm::quat>> rotations;
        std::vector<std::pair<float, glm::vec3>> scales;
    };
    std::vector<Channel> channels;
};

struct Model {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<SubMesh> submeshes;
    std::vector<MaterialData> materials;
    std::vector<Texture> textures;
    std::vector<BoneInfo> bones;
    std::vector<Animation> animations;
    std::unordered_map<std::string, int> boneMap;
    
    glm::mat4 globalInverseTransform{1.0f};
    
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VmaAllocation vertexAllocation = nullptr;
    VmaAllocation indexAllocation = nullptr;
    
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    
    bool hasAnimations() const { return !animations.empty(); }
    bool hasBones() const { return !bones.empty(); }
};

class ModelLoader {
    VkDevice device;
    VmaAllocator allocator;
    VkCommandPool commandPool;
    VkQueue queue;
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    
    Texture defaultWhiteTexture;
    Texture defaultNormalTexture;
    
public:
    bool init(VkDevice dev, VmaAllocator alloc, VkCommandPool cmdPool, VkQueue q,
              VkDescriptorPool descPool, VkDescriptorSetLayout descLayout) {
        device = dev;
        allocator = alloc;
        commandPool = cmdPool;
        queue = q;
        descriptorPool = descPool;
        descriptorSetLayout = descLayout;
        
        createDefaultTextures();
        return true;
    }
    
    Model load(const std::string& path) {
        Model model;
        
        std::string ext = std::filesystem::path(path).extension().string();
        for (auto& c : ext) c = tolower(c);
        
        Assimp::Importer importer;
        
        unsigned int flags = 
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_JoinIdenticalVertices |
            aiProcess_OptimizeMeshes |
            aiProcess_OptimizeGraph |
            aiProcess_LimitBoneWeights |
            aiProcess_FlipUVs;
        
        const aiScene* scene = importer.ReadFile(path, flags);
        
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
            return model;
        }
        
        std::string baseDir = std::filesystem::path(path).parent_path().string();
        if (!baseDir.empty()) baseDir += "/";
        
        model.globalInverseTransform = glm::inverse(aiToGlm(scene->mRootNode->mTransformation));
        
        // Load materials
        loadMaterials(scene, baseDir, model);
        
        // Load meshes recursively
        processNode(scene->mRootNode, scene, model, glm::mat4(1.0f));
        
        // Load animations
        loadAnimations(scene, model);
        
        // Upload to GPU
        createBuffers(model);
        createDescriptorSet(model);
        
        std::cout << "Loaded: " << path << std::endl;
        std::cout << "  Vertices: " << model.vertices.size() << std::endl;
        std::cout << "  Indices: " << model.indices.size() << std::endl;
        std::cout << "  Submeshes: " << model.submeshes.size() << std::endl;
        std::cout << "  Materials: " << model.materials.size() << std::endl;
        std::cout << "  Textures: " << model.textures.size() << std::endl;
        std::cout << "  Bones: " << model.bones.size() << std::endl;
        std::cout << "  Animations: " << model.animations.size() << std::endl;
        
        return model;
    }
    
    void cleanup(Model& model) {
        if (model.vertexBuffer) {
            vmaDestroyBuffer(allocator, model.vertexBuffer, model.vertexAllocation);
        }
        if (model.indexBuffer) {
            vmaDestroyBuffer(allocator, model.indexBuffer, model.indexAllocation);
        }
        for (auto& tex : model.textures) {
            if (tex.sampler) vkDestroySampler(device, tex.sampler, nullptr);
            if (tex.view) vkDestroyImageView(device, tex.view, nullptr);
            if (tex.image) vmaDestroyImage(allocator, tex.image, tex.allocation);
        }
    }
    
    void cleanupLoader() {
        if (defaultWhiteTexture.sampler) vkDestroySampler(device, defaultWhiteTexture.sampler, nullptr);
        if (defaultWhiteTexture.view) vkDestroyImageView(device, defaultWhiteTexture.view, nullptr);
        if (defaultWhiteTexture.image) vmaDestroyImage(allocator, defaultWhiteTexture.image, defaultWhiteTexture.allocation);
        
        if (defaultNormalTexture.sampler) vkDestroySampler(device, defaultNormalTexture.sampler, nullptr);
        if (defaultNormalTexture.view) vkDestroyImageView(device, defaultNormalTexture.view, nullptr);
        if (defaultNormalTexture.image) vmaDestroyImage(allocator, defaultNormalTexture.image, defaultNormalTexture.allocation);
    }
    
    Texture& getDefaultWhite() { return defaultWhiteTexture; }
    Texture& getDefaultNormal() { return defaultNormalTexture; }

private:
    glm::mat4 aiToGlm(const aiMatrix4x4& m) {
        return glm::mat4(
            m.a1, m.b1, m.c1, m.d1,
            m.a2, m.b2, m.c2, m.d2,
            m.a3, m.b3, m.c3, m.d3,
            m.a4, m.b4, m.c4, m.d4
        );
    }
    
    glm::vec3 aiToGlm(const aiVector3D& v) { return glm::vec3(v.x, v.y, v.z); }
    glm::quat aiToGlm(const aiQuaternion& q) { return glm::quat(q.w, q.x, q.y, q.z); }
    glm::vec4 aiToGlm(const aiColor4D& c) { return glm::vec4(c.r, c.g, c.b, c.a); }
    
    void loadMaterials(const aiScene* scene, const std::string& baseDir, Model& model) {
        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
            aiMaterial* mat = scene->mMaterials[i];
            MaterialData material;
            
            aiString name;
            if (mat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
                material.name = name.C_Str();
            }
            
            aiColor4D color;
            if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
                material.baseColor = aiToGlm(color);
            }
            if (mat->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS) {
                material.emissive = glm::vec3(color.r, color.g, color.b);
            }
            
            float value;
            if (mat->Get(AI_MATKEY_METALLIC_FACTOR, value) == AI_SUCCESS) {
                material.metallic = value;
            }
            if (mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, value) == AI_SUCCESS) {
                material.roughness = value;
            }
            
            // Load textures
            aiString texPath;
            if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                material.albedoTexture = loadTexture(scene, baseDir, texPath.C_Str(), model);
            }
            if (mat->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS) {
                material.normalTexture = loadTexture(scene, baseDir, texPath.C_Str(), model);
            }
            if (mat->GetTexture(aiTextureType_METALNESS, 0, &texPath) == AI_SUCCESS) {
                material.metallicRoughnessTexture = loadTexture(scene, baseDir, texPath.C_Str(), model);
            }
            if (mat->GetTexture(aiTextureType_EMISSIVE, 0, &texPath) == AI_SUCCESS) {
                material.emissiveTexture = loadTexture(scene, baseDir, texPath.C_Str(), model);
            }
            
            model.materials.push_back(material);
        }
        
        if (model.materials.empty()) {
            model.materials.push_back(MaterialData{});
        }
    }
    
    int loadTexture(const aiScene* scene, const std::string& baseDir, const char* path, Model& model) {
        std::string texPath = path;
        
        // Check if embedded texture
        if (texPath[0] == '*') {
            int texIndex = std::stoi(texPath.substr(1));
            if (texIndex < (int)scene->mNumTextures) {
                aiTexture* tex = scene->mTextures[texIndex];
                Texture texture = loadEmbeddedTexture(tex);
                if (texture.image) {
                    model.textures.push_back(texture);
                    return (int)model.textures.size() - 1;
                }
            }
            return -1;
        }
        
        // External file
        std::string fullPath = baseDir + texPath;
        for (size_t i = 0; i < model.textures.size(); i++) {
            if (model.textures[i].path == fullPath) return (int)i;
        }
        
        Texture texture = loadTextureFile(fullPath);
        if (texture.image) {
            texture.path = fullPath;
            model.textures.push_back(texture);
            return (int)model.textures.size() - 1;
        }
        return -1;
    }
    
    Texture loadEmbeddedTexture(aiTexture* tex) {
        Texture texture;
        
        const unsigned char* data;
        int width, height, channels;
        
        if (tex->mHeight == 0) {
            // Compressed
            data = stbi_load_from_memory(
                reinterpret_cast<const unsigned char*>(tex->pcData),
                tex->mWidth, &width, &height, &channels, 4);
        } else {
            // Raw ARGB
            width = tex->mWidth;
            height = tex->mHeight;
            channels = 4;
            data = reinterpret_cast<const unsigned char*>(tex->pcData);
        }
        
        if (data) {
            createTextureImage(data, width, height, texture);
            if (tex->mHeight == 0) stbi_image_free((void*)data);
        }
        
        return texture;
    }
    
    Texture loadTextureFile(const std::string& path) {
        Texture texture;
        int width, height, channels;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
        if (data) {
            createTextureImage(data, width, height, texture);
            stbi_image_free(data);
        }
        return texture;
    }
    
    void createTextureImage(const unsigned char* data, int width, int height, Texture& texture) {
        VkDeviceSize imageSize = width * height * 4;
        
        VkBuffer stagingBuffer;
        VmaAllocation stagingAlloc;
        
        VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.size = imageSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        
        vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingAlloc, nullptr);
        
        void* mapped;
        vmaMapMemory(allocator, stagingAlloc, &mapped);
        memcpy(mapped, data, imageSize);
        vmaUnmapMemory(allocator, stagingAlloc);
        
        VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {(uint32_t)width, (uint32_t)height, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        
        VmaAllocationCreateInfo imgAllocInfo{};
        imgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        
        vmaCreateImage(allocator, &imageInfo, &imgAllocInfo, &texture.image, &texture.allocation, nullptr);
        
        // Transition and copy
        VkCommandBuffer cmd = beginSingleTimeCommands();
        
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = texture.image;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        VkBufferImageCopy region{};
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageExtent = {(uint32_t)width, (uint32_t)height, 1};
        
        vkCmdCopyBufferToImage(cmd, stagingBuffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        endSingleTimeCommands(cmd);
        
        vmaDestroyBuffer(allocator, stagingBuffer, stagingAlloc);
        
        // Create view and sampler
        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = texture.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCreateImageView(device, &viewInfo, nullptr, &texture.view);
        
        VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.maxLod = 1.0f;
        vkCreateSampler(device, &samplerInfo, nullptr, &texture.sampler);
        
        texture.width = width;
        texture.height = height;
    }
    
    void processNode(aiNode* node, const aiScene* scene, Model& model, glm::mat4 parentTransform) {
        glm::mat4 nodeTransform = parentTransform * aiToGlm(node->mTransformation);
        
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            processMesh(mesh, scene, model, nodeTransform);
        }
        
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene, model, nodeTransform);
        }
    }
    
    void processMesh(aiMesh* mesh, const aiScene* scene, Model& model, const glm::mat4& transform) {
        SubMesh submesh;
        submesh.name = mesh->mName.C_Str();
        submesh.vertexOffset = (uint32_t)model.vertices.size();
        submesh.indexOffset = (uint32_t)model.indices.size();
        submesh.materialIndex = mesh->mMaterialIndex;
        
        // Vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            
            glm::vec4 pos = transform * glm::vec4(aiToGlm(mesh->mVertices[i]), 1.0f);
            vertex.position = glm::vec3(pos);
            
            if (mesh->HasNormals()) {
                glm::vec4 norm = transform * glm::vec4(aiToGlm(mesh->mNormals[i]), 0.0f);
                vertex.normal = glm::normalize(glm::vec3(norm));
            }
            
            if (mesh->HasTextureCoords(0)) {
                vertex.texCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            }
            
            if (mesh->HasVertexColors(0)) {
                vertex.color = aiToGlm(mesh->mColors[0][i]);
            }
            
            model.vertices.push_back(vertex);
        }
        
        // Indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                model.indices.push_back(submesh.vertexOffset + face.mIndices[j]);
            }
        }
        
        submesh.indexCount = (uint32_t)model.indices.size() - submesh.indexOffset;
        
        // Bones
        if (mesh->HasBones()) {
            loadBones(mesh, model, submesh.vertexOffset);
        }
        
        model.submeshes.push_back(submesh);
    }
    
    void loadBones(aiMesh* mesh, Model& model, uint32_t vertexOffset) {
        for (unsigned int i = 0; i < mesh->mNumBones; i++) {
            aiBone* bone = mesh->mBones[i];
            std::string boneName = bone->mName.C_Str();
            
            int boneIndex;
            if (model.boneMap.find(boneName) == model.boneMap.end()) {
                boneIndex = (int)model.bones.size();
                BoneInfo boneInfo;
                boneInfo.name = boneName;
                boneInfo.offset = aiToGlm(bone->mOffsetMatrix);
                model.bones.push_back(boneInfo);
                model.boneMap[boneName] = boneIndex;
            } else {
                boneIndex = model.boneMap[boneName];
            }
            
            for (unsigned int j = 0; j < bone->mNumWeights; j++) {
                uint32_t vertexId = vertexOffset + bone->mWeights[j].mVertexId;
                float weight = bone->mWeights[j].mWeight;
                
                Vertex& v = model.vertices[vertexId];
                for (int k = 0; k < 4; k++) {
                    if (v.boneWeights[k] == 0.0f) {
                        v.boneIds[k] = boneIndex;
                        v.boneWeights[k] = weight;
                        break;
                    }
                }
            }
        }
    }
    
    void loadAnimations(const aiScene* scene, Model& model) {
        for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
            aiAnimation* anim = scene->mAnimations[i];
            Animation animation;
            animation.name = anim->mName.C_Str();
            animation.duration = (float)anim->mDuration;
            animation.ticksPerSecond = anim->mTicksPerSecond > 0 ? (float)anim->mTicksPerSecond : 25.0f;
            
            for (unsigned int j = 0; j < anim->mNumChannels; j++) {
                aiNodeAnim* channel = anim->mChannels[j];
                Animation::Channel ch;
                ch.nodeName = channel->mNodeName.C_Str();
                
                for (unsigned int k = 0; k < channel->mNumPositionKeys; k++) {
                    ch.positions.emplace_back((float)channel->mPositionKeys[k].mTime,
                        aiToGlm(channel->mPositionKeys[k].mValue));
                }
                for (unsigned int k = 0; k < channel->mNumRotationKeys; k++) {
                    ch.rotations.emplace_back((float)channel->mRotationKeys[k].mTime,
                        aiToGlm(channel->mRotationKeys[k].mValue));
                }
                for (unsigned int k = 0; k < channel->mNumScalingKeys; k++) {
                    ch.scales.emplace_back((float)channel->mScalingKeys[k].mTime,
                        aiToGlm(channel->mScalingKeys[k].mValue));
                }
                
                animation.channels.push_back(ch);
            }
            
            model.animations.push_back(animation);
        }
    }
    
    void createBuffers(Model& model) {
        if (model.vertices.empty()) return;
        
        // Vertex buffer
        VkDeviceSize vbSize = model.vertices.size() * sizeof(Vertex);
        VkBuffer stagingVB;
        VmaAllocation stagingVBAlloc;
        
        VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.size = vbSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &stagingVB, &stagingVBAlloc, nullptr);
        
        void* data;
        vmaMapMemory(allocator, stagingVBAlloc, &data);
        memcpy(data, model.vertices.data(), vbSize);
        vmaUnmapMemory(allocator, stagingVBAlloc);
        
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &model.vertexBuffer, &model.vertexAllocation, nullptr);
        
        // Index buffer
        VkDeviceSize ibSize = model.indices.size() * sizeof(uint32_t);
        VkBuffer stagingIB;
        VmaAllocation stagingIBAlloc;
        
        bufferInfo.size = ibSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &stagingIB, &stagingIBAlloc, nullptr);
        
        vmaMapMemory(allocator, stagingIBAlloc, &data);
        memcpy(data, model.indices.data(), ibSize);
        vmaUnmapMemory(allocator, stagingIBAlloc);
        
        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &model.indexBuffer, &model.indexAllocation, nullptr);
        
        // Copy
        VkCommandBuffer cmd = beginSingleTimeCommands();
        
        VkBufferCopy copyRegion{};
        copyRegion.size = vbSize;
        vkCmdCopyBuffer(cmd, stagingVB, model.vertexBuffer, 1, &copyRegion);
        
        copyRegion.size = ibSize;
        vkCmdCopyBuffer(cmd, stagingIB, model.indexBuffer, 1, &copyRegion);
        
        endSingleTimeCommands(cmd);
        
        vmaDestroyBuffer(allocator, stagingVB, stagingVBAlloc);
        vmaDestroyBuffer(allocator, stagingIB, stagingIBAlloc);
    }
    
    void createDescriptorSet(Model& model) {
        VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        vkAllocateDescriptorSets(device, &allocInfo, &model.descriptorSet);
        
        Texture* albedo = model.textures.empty() ? &defaultWhiteTexture : &model.textures[0];
        
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = albedo->view;
        imageInfo.sampler = albedo->sampler;
        
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = model.descriptorSet;
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;
        
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }
    
    void createDefaultTextures() {
        uint32_t white = 0xFFFFFFFF;
        createTextureImage(reinterpret_cast<const unsigned char*>(&white), 1, 1, defaultWhiteTexture);
        
        uint32_t normal = 0xFFFF8080; // (128, 128, 255, 255) = flat normal
        createTextureImage(reinterpret_cast<const unsigned char*>(&normal), 1, 1, defaultNormalTexture);
    }
    
    VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;
        
        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(device, &allocInfo, &cmd);
        
        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);
        
        return cmd;
    }
    
    void endSingleTimeCommands(VkCommandBuffer cmd) {
        vkEndCommandBuffer(cmd);
        
        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);
        
        vkFreeCommandBuffers(device, commandPool, 1, &cmd);
    }
};
