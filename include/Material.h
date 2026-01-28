#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <string>

struct MaterialProperties {
    glm::vec3 albedo = glm::vec3(1.0f);
    float metallic = 0.0f;
    float roughness = 0.5f;
    float ao = 1.0f;
    glm::vec3 emissive = glm::vec3(0.0f);
    float emissiveStrength = 0.0f;
    
    bool useAlbedoMap = false;
    bool useNormalMap = false;
    bool useMetallicMap = false;
    bool useRoughnessMap = false;
    bool useAOMap = false;
    bool useEmissiveMap = false;
};

class Material {
public:
    std::string name;
    MaterialProperties properties;
    
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    
    void bind(VkCommandBuffer cmd, VkPipelineLayout layout);
};

class MaterialLibrary {
    std::unordered_map<std::string, Material> materials;
    
public:
    Material* createMaterial(const std::string& name);
    Material* getMaterial(const std::string& name);
    bool loadMaterial(const std::string& filepath);
    bool saveMaterial(const std::string& name, const std::string& filepath);
};
