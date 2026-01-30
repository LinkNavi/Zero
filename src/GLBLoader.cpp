#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "GLBLoader.h"
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

bool GLBModel::load(const std::string &filepath, VmaAllocator alloc,
                    VkDevice dev, VkCommandPool pool, VkQueue queue,
                    VkDescriptorSetLayout descriptorSetLayout) {
  allocator = alloc;
  device = dev;
  commandPool = pool;
  graphicsQueue = queue;

  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err, warn;

  bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filepath);

  if (!warn.empty())
    std::cout << "GLTF Warning: " << warn << std::endl;
  if (!err.empty())
    std::cerr << "GLTF Error: " << err << std::endl;
  if (!ret)
    return false;

  // Create default white texture first
  if (!createDefaultTexture()) {
    std::cerr << "Failed to create default texture!" << std::endl;
    return false;
  }

  // Load textures - OPTIMIZED: all in one command buffer submission
  if (!loadTextures(model)) {
    std::cerr << "Failed to load textures" << std::endl;
    return false;
  }

  // Load materials
  materials.reserve(model.materials.size());
  for (const auto &mat : model.materials) {
    GLBMaterial material;

    if (mat.values.find("baseColorFactor") != mat.values.end()) {
      auto factor = mat.values.at("baseColorFactor").ColorFactor();
      material.baseColor =
          glm::vec4(factor[0], factor[1], factor[2], factor[3]);
    }

    if (mat.values.find("metallicFactor") != mat.values.end()) {
      material.metallic =
          static_cast<float>(mat.values.at("metallicFactor").Factor());
    }

    if (mat.values.find("roughnessFactor") != mat.values.end()) {
      material.roughness =
          static_cast<float>(mat.values.at("roughnessFactor").Factor());
    }

    if (mat.values.find("baseColorTexture") != mat.values.end()) {
      material.baseColorTexture =
          mat.values.at("baseColorTexture").TextureIndex();
    }

    if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
      material.normalTexture =
          mat.additionalValues.at("normalTexture").TextureIndex();
    }

    if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
      material.metallicRoughnessTexture =
          mat.values.at("metallicRoughnessTexture").TextureIndex();
    }

    materials.push_back(material);
  }

  // Load meshes from scene with proper transform hierarchy
  const tinygltf::Scene &scene =
      model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
  glm::mat4 identity = glm::mat4(1.0f);

  for (int nodeIdx : scene.nodes) {
    if (!processNode(model, model.nodes[nodeIdx], identity)) {
      return false;
    }
  }

  // Create descriptor pool and sets
  if (!createDescriptorPool(descriptorSetLayout)) {
    std::cerr << "Failed to create descriptor pool!" << std::endl;
    return false;
  }

  if (!createDescriptorSets(descriptorSetLayout)) {
    std::cerr << "Failed to create descriptor sets!" << std::endl;
    return false;
  }

  std::cout << "✓ Loaded GLB: " << filepath << " (" << meshes.size()
            << " meshes, " << materials.size() << " materials, " 
            << textures.size() << " textures)" << std::endl;
  return true;
}

void GLBModel::generateMipmaps(VkCommandBuffer cmd, VkImage image, VkFormat format,
                               int32_t width, int32_t height,
                               uint32_t mipLevels) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = width;
    int32_t mipHeight = height;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit{};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};

        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {
            mipWidth > 1 ? mipWidth / 2 : 1,
            mipHeight > 1 ? mipHeight / 2 : 1,
            1
        };

        vkCmdBlitImage(cmd,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
}

bool GLBModel::createDefaultTexture() {
  unsigned char whitePixel[4] = {255, 255, 255, 255};
  
  VkDeviceSize imageSize = 4;
  
  VkBuffer stagingBuffer;
  VmaAllocation stagingAllocation;
  
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = imageSize;
  bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  
  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
  
  if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &stagingBuffer,
                      &stagingAllocation, nullptr) != VK_SUCCESS) {
    return false;
  }
  
  void* data;
  vmaMapMemory(allocator, stagingAllocation, &data);
  memcpy(data, whitePixel, imageSize);
  vmaUnmapMemory(allocator, stagingAllocation);
  
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = 1;
  imageInfo.extent.height = 1;
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
  
  if (vmaCreateImage(allocator, &imageInfo, &imgAllocInfo, &defaultTexture.image,
                     &defaultTexture.allocation, nullptr) != VK_SUCCESS) {
    vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
    return false;
  }
  
  VkCommandBuffer cmd = beginSingleTimeCommands();
  transitionImageLayout(cmd, defaultTexture.image, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  
  VkBufferImageCopy region{};
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.layerCount = 1;
  region.imageExtent = {1, 1, 1};
  vkCmdCopyBufferToImage(cmd, stagingBuffer, defaultTexture.image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  
  transitionImageLayout(cmd, defaultTexture.image,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  endSingleTimeCommands(cmd);
  
  vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
  
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = defaultTexture.image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.layerCount = 1;
  
  if (vkCreateImageView(device, &viewInfo, nullptr, &defaultTexture.imageView) != VK_SUCCESS) {
    return false;
  }
  
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 1.0f;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  
  if (vkCreateSampler(device, &samplerInfo, nullptr, &defaultTexture.sampler) != VK_SUCCESS) {
    return false;
  }
  
  return true;
}

bool GLBModel::createDescriptorPool(VkDescriptorSetLayout) {
  uint32_t maxSets = meshes.size() + 1;
  
  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSize.descriptorCount = maxSets;
  
  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = maxSets;
  
  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
    return false;
  }
  
  return true;
}

bool GLBModel::createDescriptorSets(VkDescriptorSetLayout descriptorSetLayout) {
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &descriptorSetLayout;
  
  if (vkAllocateDescriptorSets(device, &allocInfo, &defaultDescriptorSet) != VK_SUCCESS) {
    std::cerr << "Failed to allocate default descriptor set!" << std::endl;
    return false;
  }
  
  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = defaultTexture.imageView;
  imageInfo.sampler = defaultTexture.sampler;
  
  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = defaultDescriptorSet;
  descriptorWrite.dstBinding = 0;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pImageInfo = &imageInfo;
  
  vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
  
  // Batch allocate all descriptor sets at once
  std::vector<VkDescriptorSetLayout> layouts(meshes.size(), descriptorSetLayout);
  std::vector<VkDescriptorSet> sets(meshes.size());
  
  allocInfo.descriptorSetCount = static_cast<uint32_t>(meshes.size());
  allocInfo.pSetLayouts = layouts.data();
  
  if (vkAllocateDescriptorSets(device, &allocInfo, sets.data()) != VK_SUCCESS) {
    std::cerr << "Failed to batch allocate descriptor sets!" << std::endl;
    return false;
  }
  
  // Batch update descriptor sets
  std::vector<VkWriteDescriptorSet> writes;
  std::vector<VkDescriptorImageInfo> imageInfos;
  writes.reserve(meshes.size());
  imageInfos.reserve(meshes.size());
  
  for (size_t i = 0; i < meshes.size(); i++) {
    auto& mesh = meshes[i];
    mesh.descriptorSet = sets[i];
    
    VkImageView texView = defaultTexture.imageView;
    VkSampler texSampler = defaultTexture.sampler;
    
    if (mesh.materialIndex >= 0 && mesh.materialIndex < materials.size()) {
      const auto& material = materials[mesh.materialIndex];
      if (material.baseColorTexture >= 0 && material.baseColorTexture < textures.size()) {
        texView = textures[material.baseColorTexture].imageView;
        texSampler = textures[material.baseColorTexture].sampler;
      }
    }
    
    VkDescriptorImageInfo imgInfo{};
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imgInfo.imageView = texView;
    imgInfo.sampler = texSampler;
    imageInfos.push_back(imgInfo);
    
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = mesh.descriptorSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfos[i];
    writes.push_back(write);
  }
  
  vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
  
  return true;
}

bool GLBModel::processNode(const tinygltf::Model &model,
                           const tinygltf::Node &node,
                           glm::mat4 parentTransform) {
  glm::mat4 nodeTransform = parentTransform;

  if (node.matrix.size() == 16) {
    nodeTransform =
        parentTransform * glm::mat4(glm::make_mat4(node.matrix.data()));
  } else {
    glm::mat4 translation = glm::mat4(1.0f);
    glm::mat4 rotation = glm::mat4(1.0f);
    glm::mat4 scale = glm::mat4(1.0f);

    if (node.translation.size() == 3) {
      translation = glm::translate(
          glm::mat4(1.0f), glm::vec3(node.translation[0], node.translation[1],
                                     node.translation[2]));
    }

    if (node.rotation.size() == 4) {
      glm::quat q(static_cast<float>(node.rotation[3]),
                  static_cast<float>(node.rotation[0]),
                  static_cast<float>(node.rotation[1]),
                  static_cast<float>(node.rotation[2]));
      rotation = glm::mat4_cast(q);
    }

    if (node.scale.size() == 3) {
      scale =
          glm::scale(glm::mat4(1.0f),
                     glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
    }

    nodeTransform = parentTransform * translation * rotation * scale;
  }

  if (node.mesh >= 0) {
    if (!processMesh(model, model.meshes[node.mesh], nodeTransform)) {
      return false;
    }
  }

  for (int childIdx : node.children) {
    if (!processNode(model, model.nodes[childIdx], nodeTransform)) {
      return false;
    }
  }

  return true;
}

bool GLBModel::processMesh(const tinygltf::Model &model,
                           const tinygltf::Mesh &mesh, glm::mat4 transform) {
  for (const auto &primitive : mesh.primitives) {
    GLBMesh glbMesh;
    std::vector<GLBVertex> vertices;
    std::vector<uint32_t> indices;

    if (primitive.attributes.find("POSITION") == primitive.attributes.end()) {
      std::cerr << "Mesh primitive missing POSITION attribute!" << std::endl;
      continue;
    }

    const tinygltf::Accessor &posAccessor =
        model.accessors[primitive.attributes.at("POSITION")];
    const tinygltf::BufferView &posView =
        model.bufferViews[posAccessor.bufferView];
    const float *positions = reinterpret_cast<const float *>(
        &model.buffers[posView.buffer]
             .data[posView.byteOffset + posAccessor.byteOffset]);

    const float *normals = nullptr;
    if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
      const tinygltf::Accessor &normAccessor =
          model.accessors[primitive.attributes.at("NORMAL")];
      const tinygltf::BufferView &normView =
          model.bufferViews[normAccessor.bufferView];
      normals = reinterpret_cast<const float *>(
          &model.buffers[normView.buffer]
               .data[normView.byteOffset + normAccessor.byteOffset]);
    }

    const float *texCoords = nullptr;
    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
      const tinygltf::Accessor &uvAccessor =
          model.accessors[primitive.attributes.at("TEXCOORD_0")];
      const tinygltf::BufferView &uvView =
          model.bufferViews[uvAccessor.bufferView];
      texCoords = reinterpret_cast<const float *>(
          &model.buffers[uvView.buffer]
               .data[uvView.byteOffset + uvAccessor.byteOffset]);
    }

    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

    for (size_t i = 0; i < posAccessor.count; i++) {
      GLBVertex vertex;

      glm::vec4 pos =
          transform * glm::vec4(positions[i * 3 + 0], positions[i * 3 + 1],
                                positions[i * 3 + 2], 1.0f);
      vertex.position = glm::vec3(pos);

      if (normals) {
        glm::vec3 normal = glm::normalize(
            normalMatrix * glm::vec3(normals[i * 3 + 0], normals[i * 3 + 1],
                                     normals[i * 3 + 2]));
        vertex.normal = normal;
      } else {
        vertex.normal = glm::vec3(0, 1, 0);
      }

      vertex.texCoord =
          texCoords ? glm::vec2(texCoords[i * 2 + 0], texCoords[i * 2 + 1])
                    : glm::vec2(0);

      vertices.push_back(vertex);
    }

    if (primitive.indices >= 0) {
      const tinygltf::Accessor &indexAccessor =
          model.accessors[primitive.indices];
      const tinygltf::BufferView &indexView =
          model.bufferViews[indexAccessor.bufferView];
      const uint8_t *indexData =
          &model.buffers[indexView.buffer]
               .data[indexView.byteOffset + indexAccessor.byteOffset];

      indices.reserve(indexAccessor.count);

      if (indexAccessor.componentType ==
          TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        const uint16_t *buf = reinterpret_cast<const uint16_t *>(indexData);
        for (size_t i = 0; i < indexAccessor.count; i++) {
          indices.push_back(buf[i]);
        }
      } else if (indexAccessor.componentType ==
                 TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
        const uint32_t *buf = reinterpret_cast<const uint32_t *>(indexData);
        for (size_t i = 0; i < indexAccessor.count; i++) {
          indices.push_back(buf[i]);
        }
      } else if (indexAccessor.componentType ==
                 TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        const uint8_t *buf = indexData;
        for (size_t i = 0; i < indexAccessor.count; i++) {
          indices.push_back(buf[i]);
        }
      }
    } else {
      for (size_t i = 0; i < vertices.size(); i++) {
        indices.push_back(static_cast<uint32_t>(i));
      }
    }

    glbMesh.indexCount = static_cast<uint32_t>(indices.size());
    glbMesh.materialIndex = primitive.material;

    VkDeviceSize vertexSize = sizeof(GLBVertex) * vertices.size();
    VkBufferCreateInfo vertexBufferInfo{};
    vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferInfo.size = vertexSize;
    vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    VmaAllocationCreateInfo vertexAllocInfo{};
    vertexAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    if (vmaCreateBuffer(allocator, &vertexBufferInfo, &vertexAllocInfo,
                        &glbMesh.vertexBuffer, &glbMesh.vertexAllocation,
                        nullptr) != VK_SUCCESS) {
      std::cerr << "Failed to create vertex buffer!" << std::endl;
      return false;
    }

    void *data;
    vmaMapMemory(allocator, glbMesh.vertexAllocation, &data);
    memcpy(data, vertices.data(), vertexSize);
    vmaUnmapMemory(allocator, glbMesh.vertexAllocation);

    VkDeviceSize indexSize = sizeof(uint32_t) * indices.size();
    VkBufferCreateInfo indexBufferInfo{};
    indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indexBufferInfo.size = indexSize;
    indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    VmaAllocationCreateInfo indexAllocInfo{};
    indexAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    if (vmaCreateBuffer(allocator, &indexBufferInfo, &indexAllocInfo,
                        &glbMesh.indexBuffer, &glbMesh.indexAllocation,
                        nullptr) != VK_SUCCESS) {
      std::cerr << "Failed to create index buffer!" << std::endl;
      vmaDestroyBuffer(allocator, glbMesh.vertexBuffer,
                       glbMesh.vertexAllocation);
      return false;
    }

    vmaMapMemory(allocator, glbMesh.indexAllocation, &data);
    memcpy(data, indices.data(), indexSize);
    vmaUnmapMemory(allocator, glbMesh.indexAllocation);

    meshes.push_back(glbMesh);
  }

  return true;
}

bool GLBModel::loadTextures(const tinygltf::Model &model) {
  if (model.textures.empty()) {
    return true;
  }
  
  std::cout << "Loading " << model.textures.size() << " textures (optimized)..." << std::endl;
  
  // OPTIMIZATION: Create ONE command buffer for ALL texture operations
  VkCommandBuffer cmd = beginSingleTimeCommands();
  
  // Track ALL staging buffers - clean up AFTER fence wait
  std::vector<VkBuffer> stagingBuffers;
  std::vector<VmaAllocation> stagingAllocations;
  stagingBuffers.reserve(model.textures.size());
  stagingAllocations.reserve(model.textures.size());
  
  // Process all textures in one command buffer
  for (size_t i = 0; i < model.textures.size(); i++) {
    const auto &gltfTexture = model.textures[i];
    
    if (gltfTexture.source < 0 || gltfTexture.source >= model.images.size()) {
      std::cerr << "Invalid texture source index: " << gltfTexture.source << std::endl;
      continue;
    }

    GLBTexture texture;
    const tinygltf::Image &image = model.images[gltfTexture.source];

    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    
    // Create texture, but don't wait for GPU yet
    if (!createTextureImage(image, texture, cmd, stagingBuffer, stagingAllocation)) {
      std::cerr << "Failed to create texture image " << i << std::endl;
      // Clean up staging buffers created so far
      for (size_t j = 0; j < stagingBuffers.size(); j++) {
        vmaDestroyBuffer(allocator, stagingBuffers[j], stagingAllocations[j]);
      }
      endSingleTimeCommands(cmd);
      return false;
    }

    stagingBuffers.push_back(stagingBuffer);
    stagingAllocations.push_back(stagingAllocation);
    textures.push_back(texture);
  }
  
  // Submit ALL texture uploads in ONE batch with ONE fence wait
  endSingleTimeCommands(cmd);
  
  // NOW it's safe to clean up ALL staging buffers at once
  for (size_t i = 0; i < stagingBuffers.size(); i++) {
    vmaDestroyBuffer(allocator, stagingBuffers[i], stagingAllocations[i]);
  }
  
  std::cout << "✓ Loaded " << textures.size() << " textures" << std::endl;
  return true;
}

bool GLBModel::createTextureImage(const tinygltf::Image &image, GLBTexture &texture, 
                                   VkCommandBuffer cmd, VkBuffer& outStagingBuffer, 
                                   VmaAllocation& outStagingAllocation) {
    if (image.image.empty()) {
        std::cerr << "Empty image data!" << std::endl;
        return false;
    }

    const uint32_t texWidth = image.width;
    const uint32_t texHeight = image.height;
    const VkDeviceSize imageSize = texWidth * texHeight * 4;

    std::vector<uint8_t> rgba(imageSize);
    if (image.component == 3) {
        for (int i = 0; i < texWidth * texHeight; i++) {
            rgba[i * 4 + 0] = image.image[i * 3 + 0];
            rgba[i * 4 + 1] = image.image[i * 3 + 1];
            rgba[i * 4 + 2] = image.image[i * 3 + 2];
            rgba[i * 4 + 3] = 255;
        }
    } else if (image.component == 4) {
        memcpy(rgba.data(), image.image.data(), imageSize);
    } else if (image.component == 1) {
        for (int i = 0; i < texWidth * texHeight; i++) {
            uint8_t g = image.image[i];
            rgba[i * 4 + 0] = g;
            rgba[i * 4 + 1] = g;
            rgba[i * 4 + 2] = g;
            rgba[i * 4 + 3] = 255;
        }
    } else {
        std::cerr << "Unsupported image component count: " << image.component << std::endl;
        return false;
    }

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &outStagingBuffer, 
                        &outStagingAllocation, nullptr) != VK_SUCCESS) {
        return false;
    }

    void *data;
    vmaMapMemory(allocator, outStagingAllocation, &data);
    memcpy(data, rgba.data(), imageSize);
    vmaUnmapMemory(allocator, outStagingAllocation);

    // Reduce to 2 mip levels for best performance on low-end hardware
    uint32_t mipLevels = std::min(2u, static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = texWidth;
    imageInfo.extent.height = texHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | 
                      VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo imgAllocInfo{};
    imgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(allocator, &imageInfo, &imgAllocInfo, 
                       &texture.image, &texture.allocation, nullptr) != VK_SUCCESS) {
        vmaDestroyBuffer(allocator, outStagingBuffer, outStagingAllocation);
        return false;
    }

    // Transition and copy using existing command buffer (no stall!)
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = texture.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {texWidth, texHeight, 1};

    vkCmdCopyBufferToImage(cmd, outStagingBuffer, texture.image, 
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Generate mipmaps in same command buffer
    generateMipmaps(cmd, texture.image, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);

    // Create view and sampler (CPU operations, not GPU)
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = texture.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &texture.imageView) != VK_SUCCESS) {
        return false;
    }

    // Reduced to 2x anisotropy for better performance
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 2.0f;  // Reduced from 4.0f
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipLevels);
    samplerInfo.mipLodBias = 0.0f;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &texture.sampler) != VK_SUCCESS) {
        return false;
    }

    return true;
}

void GLBModel::draw(VkCommandBuffer cmd) {
  for (const auto &mesh : meshes) {
    VkBuffer vertexBuffers[] = {mesh.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmd, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, mesh.indexCount, 1, 0, 0, 0);
  }
}

void GLBModel::cleanup() {
  for (auto &mesh : meshes) {
    if (mesh.vertexBuffer != VK_NULL_HANDLE) {
      vmaDestroyBuffer(allocator, mesh.vertexBuffer, mesh.vertexAllocation);
    }
    if (mesh.indexBuffer != VK_NULL_HANDLE) {
      vmaDestroyBuffer(allocator, mesh.indexBuffer, mesh.indexAllocation);
    }
  }
  
  for (auto &texture : textures) {
    if (texture.sampler != VK_NULL_HANDLE) {
      vkDestroySampler(device, texture.sampler, nullptr);
    }
    if (texture.imageView != VK_NULL_HANDLE) {
      vkDestroyImageView(device, texture.imageView, nullptr);
    }
    if (texture.image != VK_NULL_HANDLE) {
      vmaDestroyImage(allocator, texture.image, texture.allocation);
    }
  }
  
  if (defaultTexture.sampler != VK_NULL_HANDLE) {
    vkDestroySampler(device, defaultTexture.sampler, nullptr);
  }
  if (defaultTexture.imageView != VK_NULL_HANDLE) {
    vkDestroyImageView(device, defaultTexture.imageView, nullptr);
  }
  if (defaultTexture.image != VK_NULL_HANDLE) {
    vmaDestroyImage(allocator, defaultTexture.image, defaultTexture.allocation);
  }
  
  if (descriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  }
  
  meshes.clear();
  textures.clear();
  materials.clear();
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
  
  // Use fence for synchronization
  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  VkFence fence;
  vkCreateFence(device, &fenceInfo, nullptr, &fence);
  
  vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence);
  
  // CRITICAL: Wait for GPU to finish BEFORE returning
  vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
  
  vkDestroyFence(device, fence, nullptr);
  vkFreeCommandBuffers(device, commandPool, 1, &cmd);
}

void GLBModel::transitionImageLayout(VkCommandBuffer cmd, VkImage image,
                                     VkImageLayout oldLayout,
                                     VkImageLayout newLayout) {
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }

  vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1,
                       &barrier);
}
