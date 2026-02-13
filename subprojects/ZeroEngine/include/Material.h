#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <fstream>
#include <sstream>
#include <iostream>

// Shader property types
using ShaderProperty = std::variant<float, glm::vec2, glm::vec3, glm::vec4, glm::mat4, int>;

enum class BlendMode {
    Opaque,
    AlphaBlend,
    Additive,
    Multiply
};

enum class CullMode {
    Back,
    Front,
    None
};

struct MaterialProperties {
    // PBR properties
    glm::vec4 baseColor{1.0f};
    glm::vec3 emissive{0.0f};
    float metallic = 0.0f;
    float roughness = 1.0f;
    float ao = 1.0f;
    float normalStrength = 1.0f;
    
    // Rendering settings
    BlendMode blendMode = BlendMode::Opaque;
    CullMode cullMode = CullMode::Back;
    bool depthWrite = true;
    bool depthTest = true;
    bool castShadows = true;
    bool receiveShadows = true;
    
    // Texture indices (-1 = none)
    int albedoMap = -1;
    int normalMap = -1;
    int metallicRoughnessMap = -1;
    int aoMap = -1;
    int emissiveMap = -1;
    
    // Custom properties
    std::unordered_map<std::string, ShaderProperty> customProperties;
};

struct ShaderPass {
    std::string name;
    std::string vertexShader;
    std::string fragmentShader;
    std::vector<std::string> defines;
    
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
};

class Material {
public:
    std::string name;
    std::string shaderName;
    MaterialProperties properties;
    std::vector<ShaderPass> passes;
    
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    
    void setFloat(const std::string& name, float value) {
        properties.customProperties[name] = value;
    }
    
    void setVec2(const std::string& name, glm::vec2 value) {
        properties.customProperties[name] = value;
    }
    
    void setVec3(const std::string& name, glm::vec3 value) {
        properties.customProperties[name] = value;
    }
    
    void setVec4(const std::string& name, glm::vec4 value) {
        properties.customProperties[name] = value;
    }
    
    void setInt(const std::string& name, int value) {
        properties.customProperties[name] = value;
    }
    
    template<typename T>
    T get(const std::string& name, T defaultValue = T{}) const {
        auto it = properties.customProperties.find(name);
        if (it != properties.customProperties.end()) {
            if (auto* val = std::get_if<T>(&it->second)) {
                return *val;
            }
        }
        return defaultValue;
    }
};

// Shader definition file format (simple text-based)
// Example:
// shader "Standard"
// pass "Forward"
//   vertex "shaders/standard.vert"
//   fragment "shaders/standard.frag"
//   define NORMAL_MAP
//   define PBR
// end
// property baseColor vec4 1 1 1 1
// property metallic float 0
// property roughness float 1

class ShaderLibrary {
    struct ShaderDef {
        std::string name;
        std::vector<ShaderPass> passes;
        std::unordered_map<std::string, ShaderProperty> defaultProperties;
    };
    
    std::unordered_map<std::string, ShaderDef> shaders;
    VkDevice device = VK_NULL_HANDLE;
    
public:
    void init(VkDevice dev) {
        device = dev;
        
        // Register built-in shaders
        registerBuiltinShaders();
    }
    
    void cleanup() {
        for (auto& [name, shader] : shaders) {
            for (auto& pass : shader.passes) {
                if (pass.pipeline) vkDestroyPipeline(device, pass.pipeline, nullptr);
                if (pass.layout) vkDestroyPipelineLayout(device, pass.layout, nullptr);
            }
        }
        shaders.clear();
    }
    
    bool loadShader(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open shader file: " << path << std::endl;
            return false;
        }
        
        ShaderDef shader;
        ShaderPass* currentPass = nullptr;
        std::string line;
        
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string token;
            iss >> token;
            
            if (token.empty() || token[0] == '#') continue;
            
            if (token == "shader") {
                std::string name;
                iss >> std::quoted(name);
                shader.name = name;
            }
            else if (token == "pass") {
                shader.passes.emplace_back();
                currentPass = &shader.passes.back();
                iss >> std::quoted(currentPass->name);
            }
            else if (token == "vertex" && currentPass) {
                iss >> std::quoted(currentPass->vertexShader);
            }
            else if (token == "fragment" && currentPass) {
                iss >> std::quoted(currentPass->fragmentShader);
            }
            else if (token == "define" && currentPass) {
                std::string def;
                iss >> def;
                currentPass->defines.push_back(def);
            }
            else if (token == "end") {
                currentPass = nullptr;
            }
            else if (token == "property") {
                std::string propName, propType;
                iss >> propName >> propType;
                
                if (propType == "float") {
                    float v; iss >> v;
                    shader.defaultProperties[propName] = v;
                }
                else if (propType == "vec2") {
                    glm::vec2 v; iss >> v.x >> v.y;
                    shader.defaultProperties[propName] = v;
                }
                else if (propType == "vec3") {
                    glm::vec3 v; iss >> v.x >> v.y >> v.z;
                    shader.defaultProperties[propName] = v;
                }
                else if (propType == "vec4") {
                    glm::vec4 v; iss >> v.x >> v.y >> v.z >> v.w;
                    shader.defaultProperties[propName] = v;
                }
                else if (propType == "int") {
                    int v; iss >> v;
                    shader.defaultProperties[propName] = v;
                }
            }
        }
        
        if (!shader.name.empty()) {
            shaders[shader.name] = std::move(shader);
            std::cout << "Loaded shader: " << shader.name << std::endl;
            return true;
        }
        return false;
    }
    
    Material createMaterial(const std::string& shaderName, const std::string& materialName = "") {
        Material mat;
        mat.name = materialName.empty() ? shaderName + "_material" : materialName;
        mat.shaderName = shaderName;
        
        auto it = shaders.find(shaderName);
        if (it != shaders.end()) {
            mat.passes = it->second.passes;
            for (auto& [k, v] : it->second.defaultProperties) {
                mat.properties.customProperties[k] = v;
            }
        }
        
        return mat;
    }
    
    bool hasShader(const std::string& name) const {
        return shaders.find(name) != shaders.end();
    }
    
    const ShaderDef* getShader(const std::string& name) const {
        auto it = shaders.find(name);
        return it != shaders.end() ? &it->second : nullptr;
    }
    
private:
    void registerBuiltinShaders() {
        // Standard PBR shader
        ShaderDef standard;
        standard.name = "Standard";
        standard.passes.push_back({"Forward", "shaders/standard_vert.spv", "shaders/standard_frag.spv", {}});
        standard.defaultProperties["baseColor"] = glm::vec4(1.0f);
        standard.defaultProperties["metallic"] = 0.0f;
        standard.defaultProperties["roughness"] = 1.0f;
        shaders["Standard"] = standard;
        
        // Unlit shader
        ShaderDef unlit;
        unlit.name = "Unlit";
        unlit.passes.push_back({"Forward", "shaders/unlit_vert.spv", "shaders/unlit_frag.spv", {}});
        unlit.defaultProperties["baseColor"] = glm::vec4(1.0f);
        shaders["Unlit"] = unlit;
        
        // Instanced shader (current optimized one)
        ShaderDef instanced;
        instanced.name = "Instanced";
        instanced.passes.push_back({"Forward", "shaders/instanced_vert.spv", "shaders/instanced_frag.spv", {}});
        instanced.defaultProperties["baseColor"] = glm::vec4(1.0f);
        shaders["Instanced"] = instanced;
        
        // Skinned shader for animated models
        ShaderDef skinned;
        skinned.name = "Skinned";
        skinned.passes.push_back({"Forward", "shaders/skinned_vert.spv", "shaders/skinned_frag.spv", {}});
        skinned.defaultProperties["baseColor"] = glm::vec4(1.0f);
        shaders["Skinned"] = skinned;
    }
};

// GPU material data for shaders (matches push constants or UBO)
struct alignas(16) GPUMaterialData {
    glm::vec4 baseColor;
    glm::vec4 emissiveAndMetallic; // xyz = emissive, w = metallic
    glm::vec4 roughnessAoNormalStrength; // x = roughness, y = ao, z = normalStrength, w = unused
};

inline GPUMaterialData toGPU(const MaterialProperties& props) {
    GPUMaterialData data;
    data.baseColor = props.baseColor;
    data.emissiveAndMetallic = glm::vec4(props.emissive, props.metallic);
    data.roughnessAoNormalStrength = glm::vec4(props.roughness, props.ao, props.normalStrength, 0.0f);
    return data;
}
