#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "GLBLoader.h"
#include <iostream>
#include <cstring>

bool GLBModel::load(const std::string& filepath, VmaAllocator alloc, VkDevice dev, VkCommandPool pool, VkQueue queue) {
    allocator = alloc;
    device = dev;
    commandPool = pool;
    graphicsQueue = queue;
    
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    
    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filepath);
    
    if (!warn.empty()) std::cout << "GLTF Warning: " << warn << std::endl;
    if (!err.empty()) std::cerr << "GLTF Error: " << err << std::endl;
    if (!ret) return false;
    
    // Load textures first
    if (!loadTextures(model)) {
        std::cerr << "Failed to load textures" << std::endl;
        return false;
    }
    
    // Load materials
    for (const auto& mat : model.materials) {
        GLBMaterial material;
        
        if (mat.values.find("baseColorFactor") != mat.values.end()) {
            auto factor = mat.values.at("baseColorFactor").ColorFactor();
            material.baseColor = glm::vec4(factor[0], factor[1], factor[2], factor[3]);
        }
        
        if (mat.values.find("metallicFactor") != mat.values.end()) {
            material.metallic = mat.values.at("metallicFactor").Factor();
        }
        
        if (mat.values.find("roughnessFactor") != mat.values.end()) {
            material.roughness = mat.values.at("roughnessFactor").Factor();
        }
        
        if (mat.values.find("baseColorTexture") != mat.values.end()) {
            material.baseColorTexture = mat.values.at("baseColorTexture").TextureIndex();
        }
        
        materials.push_back(material);
    }
    
    // Load meshes from scene
    const tinygltf::Scene& scene = model.scenes[model.defaultScene];
    for (int nodeIdx : scene.nodes) {
        if (!processNode(model, model.nodes[nodeIdx])) {
            return false;
        }
    }
    
    std::cout << "✓ Loaded GLB: " << filepath << " (" << meshes.size() << " meshes)" << std::endl;
    return true;
}

bool GLBModel::processNode(const tinygltf::Model& model, const tinygltf::Node& node) {
    if (node.mesh >= 0) {
        if (!processMesh(model, model.meshes[node.mesh])) {
            return false;
        }
    }
    
    for (int childIdx : node.children) {
        if (!processNode(model, model.nodes[childIdx])) {
            return false;
        }
    }
    
    return true;
}

bool GLBModel::processMesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh) {
    for (const auto& primitive : mesh.primitives) {
        GLBMesh glbMesh;
        std::vector<GLBVertex> vertices;
        std::vector<uint32_t> indices;
        
        // Get positions
        const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
        const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
        const float* positions = reinterpret_cast<const float*>(&model.buffers[posView.buffer].data[posView.byteOffset + posAccessor.byteOffset]);
        
        // Get normals
        const float* normals = nullptr;
        if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
            const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.at("NORMAL")];
            const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
            normals = reinterpret_cast<const float*>(&model.buffers[normView.buffer].data[normView.byteOffset + normAccessor.byteOffset]);
        }
        
        // Get UVs
        const float* texCoords = nullptr;
        if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
            const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
            const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
            texCoords = reinterpret_cast<const float*>(&model.buffers[uvView.buffer].data[uvView.byteOffset + uvAccessor.byteOffset]);
        }
        
        // Build vertices
        for (size_t i = 0; i < posAccessor.count; i++) {
            GLBVertex vertex;
            vertex.position = glm::vec3(positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2]);
            vertex.normal = normals ? glm::vec3(normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2]) : glm::vec3(0, 1, 0);
            vertex.texCoord = texCoords ? glm::vec2(texCoords[i * 2 + 0], texCoords[i * 2 + 1]) : glm::vec2(0);
            vertices.push_back(vertex);
        }
        
        // Get indices
        const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
        const tinygltf::BufferView& indexView = model.bufferViews[indexAccessor.bufferView];
        const uint8_t* indexData = &model.buffers[indexView.buffer].data[indexView.byteOffset + indexAccessor.byteOffset];
        
        if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            const uint16_t* buf = reinterpret_cast<const uint16_t*>(indexData);
            for (size_t i = 0; i < indexAccessor.count; i++) {
                indices.push_back(buf[i]);
            }
        } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            const uint32_t* buf = reinterpret_cast<const uint32_t*>(indexData);
            for (size_t i = 0; i < indexAccessor.count; i++) {
                indices.push_back(buf[i]);
            }
        }
        
        glbMesh.indexCount = static_cast<uint32_t>(indices.size());
        glbMesh.materialIndex = primitive.material;
        
        // Create vertex buffer
        VkDeviceSize vertexSize = sizeof(GLBVertex) * vertices.size();
        VkBufferCreateInfo vertexBufferInfo{};
        vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexBufferInfo.size = vertexSize;
        vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        
        VmaAllocationCreateInfo vertexAllocInfo{};
        vertexAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        
        vmaCreateBuffer(allocator, &vertexBufferInfo, &vertexAllocInfo, &glbMesh.vertexBuffer, &glbMesh.vertexAllocation, nullptr);
        
        void* data;
        vmaMapMemory(allocator, glbMesh.vertexAllocation, &data);
        memcpy(data, vertices.data(), vertexSize);
        vmaUnmapMemory(allocator, glbMesh.vertexAllocation);
        
        // Create index buffer
        VkDeviceSize indexSize = sizeof(uint32_t) * indices.size();
        VkBufferCreateInfo indexBufferInfo{};
        indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        indexBufferInfo.size = indexSize;
        indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        
        VmaAllocationCreateInfo indexAllocInfo{};
        indexAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        
        vmaCreateBuffer(allocator, &indexBufferInfo, &indexAllocInfo, &glbMesh.indexBuffer, &glbMesh.indexAllocation, nullptr);
        
        vmaMapMemory(allocator, glbMesh.indexAllocation, &data);
        memcpy(data, indices.data(), indexSize);
        vmaUnmapMemory(allocator, glbMesh.indexAllocation);
        
        meshes.push_back(glbMesh);
    }
    
    return true;
}

bool GLBModel::loadTextures(const tinygltf::Model& model) {
    for (const auto& gltfTexture : model.textures) {
        GLBTexture texture;
        const tinygltf::Image& image = model.images[gltfTexture.source];
        
        if (!createTextureImage(image, texture)) {
            return false;
        }
        
        textures.push_back(texture);
    }
    return true;
}

bool GLBModel::createTextureImage(const tinygltf::Image& image, GLBTexture& texture) {
    VkDeviceSize imageSize = image.width * image.height * 4;
    
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    
    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingAllocation, nullptr);
    
    void* data;
    vmaMapMemory(allocator, stagingAllocation, &data);
    memcpy(data, image.image.data(), imageSize);
    vmaUnmapMemory(allocator, stagingAllocation);
    
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = image.width;
    imageInfo.extent.height = image.height;
    imageInfo.extent.depth = 1;
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
    transitionImageLayout(cmd, texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {(uint32_t)image.width, (uint32_t)image.height, 1};
    vkCmdCopyBufferToImage(cmd, stagingBuffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    
    transitionImageLayout(cmd, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    endSingleTimeCommands(cmd);
    
    vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
    
    // Create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = texture.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(device, &viewInfo, nullptr, &texture.imageView);
    
    // Create sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    vkCreateSampler(device, &samplerInfo, nullptr, &texture.sampler);
    
    return true;
}

void GLBModel::draw(VkCommandBuffer cmd) {
    for (const auto& mesh : meshes) {
        VkBuffer vertexBuffers[] = {mesh.vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(cmd, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, mesh.indexCount, 1, 0, 0, 0);
    }
}

void GLBModel::cleanup() {
    for (auto& mesh : meshes) {
        vmaDestroyBuffer(allocator, mesh.vertexBuffer, mesh.vertexAllocation);
        vmaDestroyBuffer(allocator, mesh.indexBuffer, mesh.indexAllocation);
    }
    for (auto& texture : textures) {
        vkDestroySampler(device, texture.sampler, nullptr);
        vkDestroyImageView(device, texture.imageView, nullptr);
        vmaDestroyImage(allocator, texture.image, texture.allocation);
    }
}

VkCommandBuffer GLBModel::beginSingleTimeCommands() {
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

void GLBModel::endSingleTimeCommands(VkCommandBuffer cmd) {
    vkEndCommandBuffer(cmd);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &cmd);
}

void GLBModel::transitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    
    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    
    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}
