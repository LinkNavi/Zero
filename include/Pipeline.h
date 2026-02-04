#pragma once
#define GLFW_INCLUDE_VULKAN
#define GLM_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>

#include "AnimationSystem.h"
#include "ModelLoader.h"
#include "Renderer.h"

// Forward declarations
class VulkanRenderer;
class Camera;
class Pipeline;
class ShadowMap;

extern Pipeline* g_pipeline;
extern VkDescriptorPool g_descriptorPool;
extern VulkanRenderer* g_renderer;
extern ModelLoader* g_modelLoader;
extern Camera* g_camera;
extern ShadowMap* g_shadowMap;

// Push constants with shadow support
struct PushConstants {
    glm::mat4 viewProj;
    glm::mat4 model;
    glm::mat4 lightViewProj;
    glm::vec3 lightDir;
    float ambientStrength;
    glm::vec3 lightColor;
    float shadowBias;
};

// Shadow pass push constants
struct ShadowPushConstants {
    glm::mat4 lightViewProj;
    glm::mat4 model;
};

// ============== SHADOW MAP ==============

class ShadowMap {
public:
    static constexpr uint32_t SHADOW_RES = 2048;
    
    VkImage depthImage = VK_NULL_HANDLE;
    VkImageView depthView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descLayout = VK_NULL_HANDLE;
    
    glm::mat4 lightViewProj = glm::mat4(1.0f);
    glm::vec3 lightDir = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
    
    float orthoSize = 15.0f;
    float nearPlane = 0.1f;
    float farPlane = 50.0f;
    float bias = 0.002f;
    
private:
    VkDevice device = VK_NULL_HANDLE;
    VmaAllocator allocator = nullptr;
    
public:
    bool init(VkDevice dev, VmaAllocator alloc) {
        device = dev;
        allocator = alloc;
        
        if (!createDepthImage()) return false;
        if (!createRenderPass()) return false;
        if (!createFramebuffer()) return false;
        if (!createSampler()) return false;
        if (!createDescriptorLayout()) return false;
        
        return true;
    }
    
    bool createPipeline(const std::string& vertPath) {
        auto vertCode = readFile(vertPath);
        if (vertCode.empty()) {
            std::cerr << "Failed to read shadow vertex shader: " << vertPath << std::endl;
            return false;
        }
        
        VkShaderModule vertModule = createShaderModule(vertCode);
        
        VkPipelineShaderStageCreateInfo vertStage{};
        vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = vertModule;
        vertStage.pName = "main";
        
        // Vertex input matching your Vertex struct
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(float) * 20;
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        
        VkVertexInputAttributeDescription attrs[6] = {};
        attrs[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0};
        attrs[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12};
        attrs[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, 24};
        attrs[3] = {3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 32};
        attrs[4] = {4, 0, VK_FORMAT_R32G32B32A32_SINT, 48};
        attrs[5] = {5, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 64};
        
        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &binding;
        vertexInput.vertexAttributeDescriptionCount = 6;
        vertexInput.pVertexAttributeDescriptions = attrs;
        
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;
        
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_TRUE;
        rasterizer.depthBiasConstantFactor = 1.5f;
        rasterizer.depthBiasSlopeFactor = 1.75f;
        
        VkPipelineMultisampleStateCreateInfo multisample{};
        multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        
        VkPipelineColorBlendStateCreateInfo colorBlend{};
        colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlend.attachmentCount = 0;
        
        VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynStates;
        
        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushRange.size = sizeof(ShadowPushConstants);
        
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &descLayout;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushRange;
        
        vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout);
        
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 1;
        pipelineInfo.pStages = &vertStage;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisample;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlend;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        
        VkResult res = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
        vkDestroyShaderModule(device, vertModule, nullptr);
        
        return res == VK_SUCCESS;
    }
    
    void updateLightMatrix(glm::vec3 sceneCenter) {
        glm::vec3 lightPos = sceneCenter - lightDir * 25.0f;
        glm::mat4 view = glm::lookAt(lightPos, sceneCenter, glm::vec3(0, 1, 0));
        glm::mat4 proj = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
        proj[1][1] *= -1;
        lightViewProj = proj * view;
    }
    
    void beginShadowPass(VkCommandBuffer cmd) {
        VkRenderPassBeginInfo rpInfo{};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass = renderPass;
        rpInfo.framebuffer = framebuffer;
        rpInfo.renderArea = {{0, 0}, {SHADOW_RES, SHADOW_RES}};
        
        VkClearValue clearValue{};
        clearValue.depthStencil = {1.0f, 0};
        rpInfo.clearValueCount = 1;
        rpInfo.pClearValues = &clearValue;
        
        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        
        VkViewport viewport{0, 0, float(SHADOW_RES), float(SHADOW_RES), 0, 1};
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        VkRect2D scissor{{0, 0}, {SHADOW_RES, SHADOW_RES}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }
    
    void endShadowPass(VkCommandBuffer cmd) {
        vkCmdEndRenderPass(cmd);
    }
    
    void cleanup() {
        if (pipeline) vkDestroyPipeline(device, pipeline, nullptr);
        if (pipelineLayout) vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        if (descLayout) vkDestroyDescriptorSetLayout(device, descLayout, nullptr);
        if (framebuffer) vkDestroyFramebuffer(device, framebuffer, nullptr);
        if (renderPass) vkDestroyRenderPass(device, renderPass, nullptr);
        if (sampler) vkDestroySampler(device, sampler, nullptr);
        if (depthView) vkDestroyImageView(device, depthView, nullptr);
        if (depthImage) vmaDestroyImage(allocator, depthImage, allocation);
    }
    
private:
    bool createDepthImage() {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {SHADOW_RES, SHADOW_RES, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_D32_SFLOAT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        
        if (vmaCreateImage(allocator, &imageInfo, &allocInfo, &depthImage, &allocation, nullptr) != VK_SUCCESS) {
            return false;
        }
        
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_D32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        
        return vkCreateImageView(device, &viewInfo, nullptr, &depthView) == VK_SUCCESS;
    }
    
    bool createRenderPass() {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = VK_FORMAT_D32_SFLOAT;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        
        VkAttachmentReference depthRef{};
        depthRef.attachment = 0;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.pDepthStencilAttachment = &depthRef;
        
        VkSubpassDependency deps[2] = {};
        deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        deps[0].dstSubpass = 0;
        deps[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        deps[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        deps[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        
        deps[1].srcSubpass = 0;
        deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        deps[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        deps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        deps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        
        VkRenderPassCreateInfo rpInfo{};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpInfo.attachmentCount = 1;
        rpInfo.pAttachments = &depthAttachment;
        rpInfo.subpassCount = 1;
        rpInfo.pSubpasses = &subpass;
        rpInfo.dependencyCount = 2;
        rpInfo.pDependencies = deps;
        
        return vkCreateRenderPass(device, &rpInfo, nullptr, &renderPass) == VK_SUCCESS;
    }
    
    bool createFramebuffer() {
        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &depthView;
        fbInfo.width = SHADOW_RES;
        fbInfo.height = SHADOW_RES;
        fbInfo.layers = 1;
        
        return vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffer) == VK_SUCCESS;
    }
    
    bool createSampler() {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.compareEnable = VK_TRUE;
        samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        samplerInfo.maxLod = 1.0f;
        
        return vkCreateSampler(device, &samplerInfo, nullptr, &sampler) == VK_SUCCESS;
    }
    
    bool createDescriptorLayout() {
        // Shadow pass only needs bone buffer
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 1;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;
        
        return vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descLayout) == VK_SUCCESS;
    }
    
    std::vector<char> readFile(const std::string& path) {
        std::ifstream f(path, std::ios::ate | std::ios::binary);
        if (!f) return {};
        size_t size = f.tellg();
        std::vector<char> buf(size);
        f.seekg(0);
        f.read(buf.data(), size);
        return buf;
    }
    
    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.codeSize = code.size();
        ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule mod;
        vkCreateShaderModule(device, &ci, nullptr, &mod);
        return mod;
    }
};

// ============== MAIN PIPELINE ==============

class Pipeline {
    VkDevice device = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkShaderModule vertShader = VK_NULL_HANDLE;
    VkShaderModule fragShader = VK_NULL_HANDLE;

public:
    bool init(VkDevice dev, VkRenderPass renderPass, const std::string& vertPath, const std::string& fragPath) {
        device = dev;

        auto vertCode = readFile(vertPath);
        auto fragCode = readFile(fragPath);
        if (vertCode.empty() || fragCode.empty()) return false;

        vertShader = createShaderModule(vertCode);
        fragShader = createShaderModule(fragCode);

        // Descriptor layout: texture + bone buffer + shadow map
        VkDescriptorSetLayoutBinding bindings[3] = {};
        bindings[0] = {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
        bindings[1] = {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr};
        bindings[2] = {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 3;
        layoutInfo.pBindings = bindings;
        vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);

        // Vertex input
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(float) * 20;
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attrs[6] = {};
        attrs[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0};
        attrs[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12};
        attrs[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, 24};
        attrs[3] = {3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 32};
        attrs[4] = {4, 0, VK_FORMAT_R32G32B32A32_SINT, 48};
        attrs[5] = {5, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 64};

        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &binding;
        vertexInput.vertexAttributeDescriptionCount = 6;
        vertexInput.pVertexAttributeDescriptions = attrs;

        VkPipelineShaderStageCreateInfo stages[2] = {};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertShader;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragShader;
        stages[1].pName = "main";

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        VkPipelineMultisampleStateCreateInfo multisample{};
        multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.colorWriteMask = 0xF;

        VkPipelineColorBlendStateCreateInfo colorBlend{};
        colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlend.attachmentCount = 1;
        colorBlend.pAttachments = &blendAttachment;

        VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynStates;

        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushRange.size = sizeof(PushConstants);

        VkPipelineLayoutCreateInfo layoutCI{};
        layoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutCI.setLayoutCount = 1;
        layoutCI.pSetLayouts = &descriptorSetLayout;
        layoutCI.pushConstantRangeCount = 1;
        layoutCI.pPushConstantRanges = &pushRange;
        vkCreatePipelineLayout(device, &layoutCI, nullptr, &pipelineLayout);

        VkGraphicsPipelineCreateInfo pipelineCI{};
        pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCI.stageCount = 2;
        pipelineCI.pStages = stages;
        pipelineCI.pVertexInputState = &vertexInput;
        pipelineCI.pInputAssemblyState = &inputAssembly;
        pipelineCI.pViewportState = &viewportState;
        pipelineCI.pRasterizationState = &rasterizer;
        pipelineCI.pMultisampleState = &multisample;
        pipelineCI.pDepthStencilState = &depthStencil;
        pipelineCI.pColorBlendState = &colorBlend;
        pipelineCI.pDynamicState = &dynamicState;
        pipelineCI.layout = pipelineLayout;
        pipelineCI.renderPass = renderPass;

        vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline);
        return true;
    }

    void bind(VkCommandBuffer cmd) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    }

    void pushConstants(VkCommandBuffer cmd, const PushConstants& pc) {
        vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);
    }

    void bindDescriptor(VkCommandBuffer cmd, VkDescriptorSet set) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &set, 0, nullptr);
    }

    VkDescriptorSetLayout getDescriptorLayout() const { return descriptorSetLayout; }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

    void cleanup() {
        if (pipeline) vkDestroyPipeline(device, pipeline, nullptr);
        if (pipelineLayout) vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        if (descriptorSetLayout) vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        if (vertShader) vkDestroyShaderModule(device, vertShader, nullptr);
        if (fragShader) vkDestroyShaderModule(device, fragShader, nullptr);
    }

private:
    std::vector<char> readFile(const std::string& path) {
        std::ifstream f(path, std::ios::ate | std::ios::binary);
        if (!f) return {};
        size_t size = f.tellg();
        std::vector<char> buf(size);
        f.seekg(0);
        f.read(buf.data(), size);
        return buf;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.codeSize = code.size();
        ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule mod;
        vkCreateShaderModule(device, &ci, nullptr, &mod);
        return mod;
    }
};

// ============== BONE BUFFER ==============

class BoneBuffer {
    VmaAllocator allocator = nullptr;
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    void* mapped = nullptr;

public:
    void create(VmaAllocator alloc) {
        allocator = alloc;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(glm::mat4) * 128;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo info;
        vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, &info);
        mapped = info.pMappedData;

        std::vector<glm::mat4> identity(128, glm::mat4(1.0f));
        memcpy(mapped, identity.data(), sizeof(glm::mat4) * 128);
    }

    void update(const std::vector<glm::mat4>& bones) {
        memcpy(mapped, bones.data(), sizeof(glm::mat4) * std::min(bones.size(), size_t(128)));
    }

    VkBuffer getBuffer() const { return buffer; }

    void cleanup() {
        if (buffer) vmaDestroyBuffer(allocator, buffer, allocation);
    }
};

// ============== RENDERABLE ==============

struct Renderable {
    Model* model = nullptr;
    AnimatorComponent animator;
    BoneBuffer boneBuffer;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkDescriptorSet shadowDescriptorSet = VK_NULL_HANDLE;
    glm::mat4 transform = glm::mat4(1.0f);

    void init(Model* mdl, VmaAllocator alloc, VkDevice device, Texture& defaultTex, 
              VkDescriptorSetLayout mainLayout, ShadowMap* shadowMap, VkDescriptorSetLayout shadowLayout) {
        model = mdl;
        boneBuffer.create(alloc);

        if (mdl->hasAnimations()) {
            animator.init(mdl);
            animator.play(0);
        }

        // Main descriptor set
        {
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = g_descriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &mainLayout;
            vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

            Texture* tex = mdl->textures.empty() ? &defaultTex : &mdl->textures[0];

            VkDescriptorImageInfo imgInfo{};
            imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imgInfo.imageView = tex->view;
            imgInfo.sampler = tex->sampler;

            VkDescriptorBufferInfo bufInfo{};
            bufInfo.buffer = boneBuffer.getBuffer();
            bufInfo.offset = 0;
            bufInfo.range = sizeof(glm::mat4) * 128;

            VkDescriptorImageInfo shadowInfo{};
            shadowInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            shadowInfo.imageView = shadowMap->depthView;
            shadowInfo.sampler = shadowMap->sampler;

            VkWriteDescriptorSet writes[3] = {};
            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet = descriptorSet;
            writes[0].dstBinding = 0;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[0].descriptorCount = 1;
            writes[0].pImageInfo = &imgInfo;

            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet = descriptorSet;
            writes[1].dstBinding = 1;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[1].descriptorCount = 1;
            writes[1].pBufferInfo = &bufInfo;

            writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[2].dstSet = descriptorSet;
            writes[2].dstBinding = 2;
            writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[2].descriptorCount = 1;
            writes[2].pImageInfo = &shadowInfo;

            vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);
        }

        // Shadow pass descriptor set
        {
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = g_descriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &shadowLayout;
            vkAllocateDescriptorSets(device, &allocInfo, &shadowDescriptorSet);

            VkDescriptorBufferInfo bufInfo{};
            bufInfo.buffer = boneBuffer.getBuffer();
            bufInfo.offset = 0;
            bufInfo.range = sizeof(glm::mat4) * 128;

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = shadowDescriptorSet;
            write.dstBinding = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.descriptorCount = 1;
            write.pBufferInfo = &bufInfo;

            vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        }
    }

    void update(float dt) {
        if (!model || !model->hasAnimations() || !animator.playing) return;

        const Animation& anim = model->animations[animator.animationIndex];
        float duration = anim.duration / anim.ticksPerSecond;

        animator.currentTime += dt * animator.speed;
        if (animator.currentTime >= duration) {
            animator.currentTime = fmod(animator.currentTime, duration);
        }

        calculateBones();
        boneBuffer.update(animator.finalTransforms);
    }

    void calculateBones() {
        if (!model || model->animations.empty() || animator.animationIndex < 0) return;

        const Animation& anim = model->animations[animator.animationIndex];
        float tick = fmod(animator.currentTime * anim.ticksPerSecond, anim.duration);

        std::unordered_map<std::string, glm::vec3> positions;
        std::unordered_map<std::string, glm::quat> rotations;
        std::unordered_map<std::string, glm::vec3> scales;

        for (const auto& ch : anim.channels) {
            positions[ch.nodeName] = interpVec3(ch.positions, tick, glm::vec3(0));
            rotations[ch.nodeName] = interpQuat(ch.rotations, tick);
            scales[ch.nodeName] = interpVec3(ch.scales, tick, glm::vec3(1));
        }

        for (size_t i = 0; i < model->bones.size(); i++) {
            const BoneInfo& bone = model->bones[i];

            glm::vec3 pos(0.0f);
            glm::quat rot(1, 0, 0, 0);
            glm::vec3 scl(1.0f);

            auto posIt = positions.find(bone.name);
            if (posIt != positions.end()) pos = posIt->second;

            auto rotIt = rotations.find(bone.name);
            if (rotIt != rotations.end()) rot = rotIt->second;

            auto sclIt = scales.find(bone.name);
            if (sclIt != scales.end()) scl = sclIt->second;

            glm::mat4 localTransform = glm::translate(glm::mat4(1), pos) *
                                       glm::toMat4(rot) *
                                       glm::scale(glm::mat4(1), scl);

            glm::mat4 parentGlobal = glm::mat4(1.0f);
            if (bone.parentIndex >= 0) {
                parentGlobal = animator.boneMatrices[bone.parentIndex];
            }

            animator.boneMatrices[i] = parentGlobal * localTransform;
            animator.finalTransforms[i] = model->globalInverseTransform *
                                          animator.boneMatrices[i] * bone.offset;
        }
    }

    glm::vec3 interpVec3(const std::vector<std::pair<float, glm::vec3>>& keys, float t, glm::vec3 def) {
        if (keys.empty()) return def;
        if (keys.size() == 1) return keys[0].second;
        size_t i = 0;
        for (; i < keys.size() - 1 && t >= keys[i + 1].first; i++);
        if (i >= keys.size() - 1) return keys.back().second;
        float f = (t - keys[i].first) / (keys[i + 1].first - keys[i].first);
        return glm::mix(keys[i].second, keys[i + 1].second, f);
    }

    glm::quat interpQuat(const std::vector<std::pair<float, glm::quat>>& keys, float t) {
        if (keys.empty()) return glm::quat(1, 0, 0, 0);
        if (keys.size() == 1) return keys[0].second;
        size_t i = 0;
        for (; i < keys.size() - 1 && t >= keys[i + 1].first; i++);
        if (i >= keys.size() - 1) return keys.back().second;
        float f = (t - keys[i].first) / (keys[i + 1].first - keys[i].first);
        return glm::slerp(keys[i].second, keys[i + 1].second, f);
    }

    void cleanup() { 
        boneBuffer.cleanup(); 
    }
};
