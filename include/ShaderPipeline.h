#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include "Material.h"

struct VertexInputDesc {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
};

class ShaderPipeline {
    VkDevice device = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    
    std::string vertPath, fragPath;
    
public:
    struct Config {
        // Shaders
        std::string vertexShader;
        std::string fragmentShader;
        
        // Vertex input
        bool useStandardVertex = true;  // position, normal, uv, color, bones
        bool useInstancing = false;
        
        // Rasterization
        VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
        VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
        VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        float lineWidth = 1.0f;
        
        // Depth
        bool depthTest = true;
        bool depthWrite = true;
        VkCompareOp depthCompare = VK_COMPARE_OP_LESS;
        
        // Blending
        BlendMode blendMode = BlendMode::Opaque;
        
        // Push constants size
        uint32_t pushConstantSize = 128;
        
        // Descriptor bindings (textures, UBOs)
        std::vector<VkDescriptorSetLayoutBinding> descriptorBindings;
    };
    
    bool create(VkDevice dev, VkRenderPass renderPass, const Config& config) {
        device = dev;
        vertPath = config.vertexShader;
        fragPath = config.fragmentShader;
        
        // Load shaders
        auto vertCode = readFile(config.vertexShader);
        auto fragCode = readFile(config.fragmentShader);
        
        if (vertCode.empty() || fragCode.empty()) {
            std::cerr << "Failed to load shaders: " << config.vertexShader << ", " << config.fragmentShader << std::endl;
            return false;
        }
        
        VkShaderModule vertModule = createShaderModule(vertCode);
        VkShaderModule fragModule = createShaderModule(fragCode);
        
        VkPipelineShaderStageCreateInfo shaderStages[2] = {};
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = vertModule;
        shaderStages[0].pName = "main";
        
        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = fragModule;
        shaderStages[1].pName = "main";
        
        // Vertex input
        VertexInputDesc vertexInput = config.useInstancing ? 
            getInstancedVertexInput() : getStandardVertexInput();
        
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)vertexInput.bindings.size();
        vertexInputInfo.pVertexBindingDescriptions = vertexInput.bindings.data();
        vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)vertexInput.attributes.size();
        vertexInputInfo.pVertexAttributeDescriptions = vertexInput.attributes.data();
        
        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        
        // Viewport (dynamic)
        VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;
        
        // Rasterization
        VkPipelineRasterizationStateCreateInfo rasterizer{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        rasterizer.polygonMode = config.polygonMode;
        rasterizer.lineWidth = config.lineWidth;
        rasterizer.cullMode = config.cullMode;
        rasterizer.frontFace = config.frontFace;
        
        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        
        // Depth stencil
        VkPipelineDepthStencilStateCreateInfo depthStencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depthStencil.depthTestEnable = config.depthTest ? VK_TRUE : VK_FALSE;
        depthStencil.depthWriteEnable = config.depthWrite ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp = config.depthCompare;
        
        // Color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        
        switch (config.blendMode) {
            case BlendMode::Opaque:
                colorBlendAttachment.blendEnable = VK_FALSE;
                break;
            case BlendMode::AlphaBlend:
                colorBlendAttachment.blendEnable = VK_TRUE;
                colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
                colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
                break;
            case BlendMode::Additive:
                colorBlendAttachment.blendEnable = VK_TRUE;
                colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
                colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
                break;
            case BlendMode::Multiply:
                colorBlendAttachment.blendEnable = VK_TRUE;
                colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
                colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
                break;
        }
        
        VkPipelineColorBlendStateCreateInfo colorBlending{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        
        // Dynamic state
        VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;
        
        // Descriptor set layout
        std::vector<VkDescriptorSetLayoutBinding> bindings = config.descriptorBindings;
        if (bindings.empty()) {
            // Default: one combined image sampler
            VkDescriptorSetLayoutBinding samplerBinding{};
            samplerBinding.binding = 0;
            samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            samplerBinding.descriptorCount = 1;
            samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings.push_back(samplerBinding);
        }
        
        VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        layoutInfo.bindingCount = (uint32_t)bindings.size();
        layoutInfo.pBindings = bindings.data();
        vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
        
        // Pipeline layout
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = config.pushConstantSize;
        
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        
        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            std::cerr << "Failed to create pipeline layout" << std::endl;
            return false;
        }
        
        // Create pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        
        VkResult result = vkCreateGraphicsPipeline(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
        
        vkDestroyShaderModule(device, vertModule, nullptr);
        vkDestroyShaderModule(device, fragModule, nullptr);
        
        if (result != VK_SUCCESS) {
            std::cerr << "Failed to create graphics pipeline" << std::endl;
            return false;
        }
        
        std::cout << "Created pipeline: " << config.vertexShader << " + " << config.fragmentShader << std::endl;
        return true;
    }
    
    void cleanup() {
        if (pipeline) vkDestroyPipeline(device, pipeline, nullptr);
        if (pipelineLayout) vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        if (descriptorSetLayout) vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        pipeline = VK_NULL_HANDLE;
        pipelineLayout = VK_NULL_HANDLE;
        descriptorSetLayout = VK_NULL_HANDLE;
    }
    
    void bind(VkCommandBuffer cmd) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    }
    
    void bindDescriptorSet(VkCommandBuffer cmd, VkDescriptorSet set) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &set, 0, nullptr);
    }
    
    template<typename T>
    void pushConstants(VkCommandBuffer cmd, const T& data, VkShaderStageFlags stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) {
        vkCmdPushConstants(cmd, pipelineLayout, stages, 0, sizeof(T), &data);
    }
    
    VkPipeline getPipeline() const { return pipeline; }
    VkPipelineLayout getLayout() const { return pipelineLayout; }
    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

private:
    std::vector<char> readFile(const std::string& path) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open()) return {};
        
        size_t size = (size_t)file.tellg();
        std::vector<char> buffer(size);
        file.seekg(0);
        file.read(buffer.data(), size);
        return buffer;
    }
    
    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        
        VkShaderModule module;
        vkCreateShaderModule(device, &createInfo, nullptr, &module);
        return module;
    }
    
    VertexInputDesc getStandardVertexInput() {
        VertexInputDesc desc;
        
        // Binding 0: vertex data
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(float) * (3 + 3 + 2 + 4 + 4 + 4); // pos, norm, uv, color, boneIds, boneWeights
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        desc.bindings.push_back(binding);
        
        uint32_t loc = 0;
        uint32_t offset = 0;
        
        // Position
        desc.attributes.push_back({loc++, 0, VK_FORMAT_R32G32B32_SFLOAT, offset});
        offset += sizeof(float) * 3;
        
        // Normal
        desc.attributes.push_back({loc++, 0, VK_FORMAT_R32G32B32_SFLOAT, offset});
        offset += sizeof(float) * 3;
        
        // TexCoord
        desc.attributes.push_back({loc++, 0, VK_FORMAT_R32G32_SFLOAT, offset});
        offset += sizeof(float) * 2;
        
        // Color
        desc.attributes.push_back({loc++, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offset});
        offset += sizeof(float) * 4;
        
        // BoneIds
        desc.attributes.push_back({loc++, 0, VK_FORMAT_R32G32B32A32_SINT, offset});
        offset += sizeof(int) * 4;
        
        // BoneWeights
        desc.attributes.push_back({loc++, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offset});
        
        return desc;
    }
    
    VertexInputDesc getInstancedVertexInput() {
        VertexInputDesc desc = getStandardVertexInput();
        
        // Binding 1: instance data (model matrix + color)
        VkVertexInputBindingDescription instanceBinding{};
        instanceBinding.binding = 1;
        instanceBinding.stride = sizeof(float) * (16 + 4); // mat4 + vec4
        instanceBinding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
        desc.bindings.push_back(instanceBinding);
        
        uint32_t loc = 6; // After vertex attributes
        
        // Model matrix (4 vec4s)
        for (int i = 0; i < 4; i++) {
            desc.attributes.push_back({loc++, 1, VK_FORMAT_R32G32B32A32_SFLOAT, (uint32_t)(sizeof(float) * 4 * i)});
        }
        
        // Instance color
        desc.attributes.push_back({loc++, 1, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 16});
        
        return desc;
    }
};

// Helper to create common pipeline configurations
namespace PipelinePresets {
    inline ShaderPipeline::Config standard(const std::string& vert = "shaders/standard_vert.spv",
                                           const std::string& frag = "shaders/standard_frag.spv") {
        ShaderPipeline::Config cfg;
        cfg.vertexShader = vert;
        cfg.fragmentShader = frag;
        cfg.useStandardVertex = true;
        cfg.useInstancing = false;
        return cfg;
    }
    
    inline ShaderPipeline::Config instanced(const std::string& vert = "shaders/instanced_vert.spv",
                                            const std::string& frag = "shaders/instanced_frag.spv") {
        ShaderPipeline::Config cfg;
        cfg.vertexShader = vert;
        cfg.fragmentShader = frag;
        cfg.useStandardVertex = true;
        cfg.useInstancing = true;
        return cfg;
    }
    
    inline ShaderPipeline::Config transparent(const std::string& vert, const std::string& frag) {
        ShaderPipeline::Config cfg;
        cfg.vertexShader = vert;
        cfg.fragmentShader = frag;
        cfg.blendMode = BlendMode::AlphaBlend;
        cfg.depthWrite = false;
        return cfg;
    }
    
    inline ShaderPipeline::Config unlit(const std::string& vert = "shaders/unlit_vert.spv",
                                        const std::string& frag = "shaders/unlit_frag.spv") {
        ShaderPipeline::Config cfg;
        cfg.vertexShader = vert;
        cfg.fragmentShader = frag;
        return cfg;
    }
    
    inline ShaderPipeline::Config wireframe(const std::string& vert, const std::string& frag) {
        ShaderPipeline::Config cfg;
        cfg.vertexShader = vert;
        cfg.fragmentShader = frag;
        cfg.polygonMode = VK_POLYGON_MODE_LINE;
        cfg.cullMode = VK_CULL_MODE_NONE;
        return cfg;
    }
}
