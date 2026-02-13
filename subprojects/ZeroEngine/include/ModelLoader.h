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
#include <assimp/config.h>
#include <vector>
#include "iomanip"
#include <unordered_map>
#include <iostream>
#include <filesystem>
#include "Texture.h"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec4 color{1.0f};
    glm::ivec4 boneIds{-1, -1, -1, -1};
    glm::vec4 boneWeights{0.0f};
    
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription desc{};
        desc.binding = 0;
        desc.stride = sizeof(Vertex);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return desc;
    }
    
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attrs(6);
        
        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[0].offset = offsetof(Vertex, position);
        
        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[1].offset = offsetof(Vertex, normal);
        
        attrs[2].binding = 0;
        attrs[2].location = 2;
        attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
        attrs[2].offset = offsetof(Vertex, texCoord);
        
        attrs[3].binding = 0;
        attrs[3].location = 3;
        attrs[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attrs[3].offset = offsetof(Vertex, color);
        
        attrs[4].binding = 0;
        attrs[4].location = 4;
        attrs[4].format = VK_FORMAT_R32G32B32A32_SINT;
        attrs[4].offset = offsetof(Vertex, boneIds);
        
        attrs[5].binding = 0;
        attrs[5].location = 5;
        attrs[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attrs[5].offset = offsetof(Vertex, boneWeights);
        
        return attrs;
    }
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
    
    VkBuffer combinedVertexBuffer = VK_NULL_HANDLE;
    VkBuffer combinedIndexBuffer = VK_NULL_HANDLE;
    VmaAllocation combinedVertexAllocation = nullptr;
    VmaAllocation combinedIndexAllocation = nullptr;
    uint32_t totalIndices = 0;
    
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
    
    // Temporary storage during loading
    std::unordered_map<std::string, int> tempBoneMap;
    std::vector<BoneInfo> tempBones;
    
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
        tempBoneMap.clear();
        tempBones.clear();
        
        Assimp::Importer importer;
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
       unsigned int flags = 
    aiProcess_Triangulate |
    aiProcess_GenNormals |
    aiProcess_CalcTangentSpace |
    aiProcess_JoinIdenticalVertices |
    aiProcess_OptimizeMeshes |
    aiProcess_LimitBoneWeights |
    aiProcess_FlipUVs |
    aiProcess_PopulateArmatureData; 
        
        const aiScene* scene = importer.ReadFile(path, flags);
        
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
            return model;
        }
        
        std::string baseDir = std::filesystem::path(path).parent_path().string();
        if (!baseDir.empty()) baseDir += "/";
        
        model.globalInverseTransform = glm::inverse(aiToGlm(scene->mRootNode->mTransformation));
        
        loadMaterials(scene, baseDir, model);
        
        // First pass: collect all bones
        collectBones(scene);
        
        // Build bone hierarchy
        buildBoneHierarchy(scene->mRootNode, -1);
        
        // Copy to model
        model.bones = tempBones;
        model.boneMap = tempBoneMap;
        
        // Process meshes
        processNode(scene->mRootNode, scene, model, glm::mat4(1.0f));
        
        loadAnimations(scene, model);
        
        createBuffers(model);
        createDescriptorSet(model);
        
        model.combinedVertexBuffer = model.vertexBuffer;
        model.combinedIndexBuffer = model.indexBuffer;
        model.combinedVertexAllocation = model.vertexAllocation;
        model.combinedIndexAllocation = model.indexAllocation;
        model.totalIndices = static_cast<uint32_t>(model.indices.size());
        
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
        model.combinedVertexBuffer = VK_NULL_HANDLE;
        model.combinedIndexBuffer = VK_NULL_HANDLE;
        
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
    
    void collectBones(const aiScene* scene) {
        for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
            aiMesh* mesh = scene->mMeshes[m];
            
            for (unsigned int b = 0; b < mesh->mNumBones; b++) {
                aiBone* bone = mesh->mBones[b];
                std::string boneName = bone->mName.C_Str();
                
                if (tempBoneMap.find(boneName) == tempBoneMap.end()) {
                    int boneIndex = static_cast<int>(tempBones.size());
                    tempBoneMap[boneName] = boneIndex;
                    
                    BoneInfo boneInfo;
                    boneInfo.name = boneName;
                    boneInfo.offset = aiToGlm(bone->mOffsetMatrix);
                    boneInfo.parentIndex = -1;
                    tempBones.push_back(boneInfo);
                }
            }
        }
    }
    
    void buildBoneHierarchy(aiNode* node, int parentBoneIndex) {
        std::string nodeName = node->mName.C_Str();
        int currentBoneIndex = -1;
        
        auto it = tempBoneMap.find(nodeName);
        if (it != tempBoneMap.end()) {
            currentBoneIndex = it->second;
            tempBones[currentBoneIndex].parentIndex = parentBoneIndex;
        }
        
        int nextParent = (currentBoneIndex != -1) ? currentBoneIndex : parentBoneIndex;
        
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            buildBoneHierarchy(node->mChildren[i], nextParent);
        }
    }
    
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
            data = stbi_load_from_memory(
                reinterpret_cast<const unsigned char*>(tex->pcData),
                tex->mWidth, &width, &height, &channels, 4);
        } else {
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
        
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = imageSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        
        vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingAlloc, nullptr);
        
        void* mapped;
        vmaMapMemory(allocator, stagingAlloc, &mapped);
        memcpy(mapped, data, imageSize);
        vmaUnmapMemory(allocator, stagingAlloc);
        
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {(uint32_t)width, (uint32_t)height, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VmaAllocationCreateInfo imgAllocInfo{};
        imgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        
        vmaCreateImage(allocator, &imageInfo, &imgAllocInfo, &texture.image, &texture.allocation, nullptr);
        
        VkCommandBuffer cmd = beginSingleTimeCommands();
        
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = texture.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
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
        
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = texture.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &viewInfo, nullptr, &texture.view);
        
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
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
    
    void processMesh(aiMesh* mesh, const aiScene* /*scene*/, Model& model, const glm::mat4& transform) {
        SubMesh submesh;
        submesh.name = mesh->mName.C_Str();
        submesh.vertexOffset = (uint32_t)model.vertices.size();
        submesh.indexOffset = (uint32_t)model.indices.size();
        submesh.materialIndex = mesh->mMaterialIndex;
        
        // Create vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex{};
            
            glm::vec4 pos = transform * glm::vec4(aiToGlm(mesh->mVertices[i]), 1.0f);
            vertex.position = glm::vec3(pos);
            
            if (mesh->HasNormals()) {
                glm::vec4 norm = transform * glm::vec4(aiToGlm(mesh->mNormals[i]), 0.0f);
                vertex.normal = glm::normalize(glm::vec3(norm));
            } else {
                vertex.normal = glm::vec3(0, 1, 0);
            }
            
            if (mesh->HasTextureCoords(0)) {
                vertex.texCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            } else {
                vertex.texCoord = glm::vec2(0);
            }
            
            if (mesh->HasVertexColors(0)) {
                vertex.color = aiToGlm(mesh->mColors[0][i]);
            } else {
                vertex.color = glm::vec4(1.0f);
            }
            
            // Initialize bone data
            vertex.boneIds = glm::ivec4(-1);
            vertex.boneWeights = glm::vec4(0.0f);
            
            model.vertices.push_back(vertex);
        }
        
        // Load bone weights
        for (unsigned int b = 0; b < mesh->mNumBones; b++) {
            aiBone* bone = mesh->mBones[b];
            std::string boneName = bone->mName.C_Str();
            
            int boneIndex = -1;
            auto it = model.boneMap.find(boneName);
            if (it != model.boneMap.end()) {
                boneIndex = it->second;
            } else {
                continue;
            }
            
            for (unsigned int w = 0; w < bone->mNumWeights; w++) {
                unsigned int vertexId = submesh.vertexOffset + bone->mWeights[w].mVertexId;
                float weight = bone->mWeights[w].mWeight;
                
                if (vertexId >= model.vertices.size()) continue;
                
                Vertex& vertex = model.vertices[vertexId];
                
                // Find empty slot
                for (int i = 0; i < 4; i++) {
                    if (vertex.boneIds[i] < 0) {
                        vertex.boneIds[i] = boneIndex;
                        vertex.boneWeights[i] = weight;
                        break;
                    }
                }
            }
        }
        
        // Normalize bone weights
        for (uint32_t i = submesh.vertexOffset; i < model.vertices.size(); i++) {
            Vertex& v = model.vertices[i];
            float totalWeight = v.boneWeights.x + v.boneWeights.y + v.boneWeights.z + v.boneWeights.w;
            if (totalWeight > 0.0f) {
                v.boneWeights /= totalWeight;
            } else {
                // No bone influence - set to identity (bone 0 with weight 1)
                v.boneIds = glm::ivec4(0, -1, -1, -1);
                v.boneWeights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            }
        }
        
        // Indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                model.indices.push_back(submesh.vertexOffset + face.mIndices[j]);
            }
        }
        
        submesh.indexCount = (uint32_t)model.indices.size() - submesh.indexOffset;
        model.submeshes.push_back(submesh);
    }
    
   void loadAnimations(const aiScene* scene, Model& model) {
    for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
        aiAnimation* anim = scene->mAnimations[i];
        Animation animation;
        animation.name = anim->mName.C_Str();
        animation.duration = (float)anim->mDuration;
        animation.ticksPerSecond = anim->mTicksPerSecond > 0 ? (float)anim->mTicksPerSecond : 25.0f;
        
        // Temporary storage to combine split FBX channels
        std::unordered_map<std::string, Animation::Channel> channelMap;
        
        for (unsigned int j = 0; j < anim->mNumChannels; j++) {
            aiNodeAnim* nodeAnim = anim->mChannels[j];
            std::string nodeName = nodeAnim->mNodeName.C_Str();
            
            // Strip Assimp FBX suffixes
            std::string baseName = nodeName;
            std::string suffix;
            size_t pos = nodeName.find("_$AssimpFbx$_");
            if (pos != std::string::npos) {
                baseName = nodeName.substr(0, pos);
                suffix = nodeName.substr(pos + 13);
            }
            
            // Get or create channel for this bone
            auto& ch = channelMap[baseName];
            ch.nodeName = baseName;
            
            // Only add position keys if this is a Translation channel or regular channel
            if (suffix.empty() || suffix == "Translation") {
                for (unsigned int k = 0; k < nodeAnim->mNumPositionKeys; k++) {
                    ch.positions.emplace_back(
                        (float)nodeAnim->mPositionKeys[k].mTime,
                        aiToGlm(nodeAnim->mPositionKeys[k].mValue)
                    );
                }
            }
            
            // Only add rotation keys if this is a Rotation channel or regular channel
            if (suffix.empty() || suffix == "Rotation") {
                for (unsigned int k = 0; k < nodeAnim->mNumRotationKeys; k++) {
                    ch.rotations.emplace_back(
                        (float)nodeAnim->mRotationKeys[k].mTime,
                        aiToGlm(nodeAnim->mRotationKeys[k].mValue)
                    );
                }
            }
            
            // Only add scale keys if this is a Scaling channel or regular channel
            if (suffix.empty() || suffix == "Scaling") {
                for (unsigned int k = 0; k < nodeAnim->mNumScalingKeys; k++) {
                    ch.scales.emplace_back(
                        (float)nodeAnim->mScalingKeys[k].mTime,
                        aiToGlm(nodeAnim->mScalingKeys[k].mValue)
                    );
                }
            }
        }
        
        // Convert map to vector
        for (auto& [name, ch] : channelMap) {
            animation.channels.push_back(std::move(ch));
        }
        
        model.animations.push_back(animation);
    }
}
    
    void createBuffers(Model& model) {
        if (model.vertices.empty()) return;
        
        VkDeviceSize vbSize = model.vertices.size() * sizeof(Vertex);
        VkBuffer stagingVB;
        VmaAllocation stagingVBAlloc;
        
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = vbSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
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
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    vkAllocateDescriptorSets(device, &allocInfo, &model.descriptorSet);
    
    // Use first texture or default white
    Texture* albedo = &defaultWhiteTexture;
    if (!model.textures.empty() && model.textures[0].view != VK_NULL_HANDLE) {
        albedo = &model.textures[0];
    }
    
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = albedo->view;
    imageInfo.sampler = albedo->sampler;
    
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
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
        
        uint32_t normal = 0xFFFF8080;
        createTextureImage(reinterpret_cast<const unsigned char*>(&normal), 1, 1, defaultNormalTexture);
    }
    
    VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;
        
        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(device, &allocInfo, &cmd);
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);
        
        return cmd;
    }
    
    void endSingleTimeCommands(VkCommandBuffer cmd) {
        vkEndCommandBuffer(cmd);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);
        
        vkFreeCommandBuffers(device, commandPool, 1, &cmd);
    }
};
