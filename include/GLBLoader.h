// include/GLBLoader.h - Optimized version
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <tiny_gltf.h>
#include <unordered_map>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

struct GLBVertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoord;

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(GLBVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
  }

  static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(GLBVertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(GLBVertex, normal);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(GLBVertex, texCoord);

    return attributeDescriptions;
  }
};

struct GLBMesh {
  VkBuffer vertexBuffer = VK_NULL_HANDLE;
  VkBuffer indexBuffer = VK_NULL_HANDLE;
  VmaAllocation vertexAllocation;
  VmaAllocation indexAllocation;
  uint32_t indexCount = 0;
  int materialIndex = -1;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

struct GLBMaterial {
  glm::vec4 baseColor = glm::vec4(1.0f);
  float metallic = 0.0f;
  float roughness = 1.0f;
  int baseColorTexture = -1;
  int normalTexture = -1;
  int metallicRoughnessTexture = -1;
};

struct GLBTexture {
  VkImage image = VK_NULL_HANDLE;
  VkImageView imageView = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;
  VmaAllocation allocation;
};

class GLBModel {
  VmaAllocator allocator = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkCommandPool commandPool = VK_NULL_HANDLE;
  VkQueue graphicsQueue = VK_NULL_HANDLE;

  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  VkDescriptorSet defaultDescriptorSet = VK_NULL_HANDLE;

  GLBTexture defaultTexture;

public:
  std::vector<GLBMesh> meshes;
  std::vector<GLBMaterial> materials;
  std::vector<GLBTexture> textures;

  bool load(const std::string &filepath, VmaAllocator alloc, VkDevice dev,
            VkCommandPool pool, VkQueue queue,
            VkDescriptorSetLayout descriptorSetLayout);
  void draw(VkCommandBuffer cmd);
  void cleanup();

private:
  // OPTIMIZED: Pass command buffer to avoid creating new ones
  void generateMipmaps(VkCommandBuffer cmd, VkImage image, VkFormat format, 
                       int32_t width, int32_t height, uint32_t mipLevels);
  bool createDefaultTexture();
  bool createDescriptorPool(VkDescriptorSetLayout descriptorSetLayout);
  bool createDescriptorSets(VkDescriptorSetLayout descriptorSetLayout);
  bool processNode(const tinygltf::Model &model, const tinygltf::Node &node,
                   glm::mat4 parentTransform);
  bool processMesh(const tinygltf::Model &model, const tinygltf::Mesh &mesh,
                   glm::mat4 transform);
  bool loadTextures(const tinygltf::Model &model);
  // OPTIMIZED: Pass command buffer for batched texture upload
  bool createTextureImage(const tinygltf::Image &image, GLBTexture &texture, 
                          VkCommandBuffer cmd);
  void transitionImageLayout(VkCommandBuffer cmd, VkImage image,
                             VkImageLayout oldLayout, VkImageLayout newLayout);
  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer cmd);
};
