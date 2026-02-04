#pragma once
#define GLFW_INCLUDE_VULKAN 
#define GLM_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "AnimationSystem.h"
#include "ModelLoader.h"
#include "Renderer.h"
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
class VulkanRenderer;
class Camera;
struct PushConstants {
  glm::mat4 viewProj;
  glm::mat4 model;
  glm::vec3 lightDir;
  float ambientStrength;
  glm::vec3 lightColor;
  float padding;
};

class Pipeline {
  VkDevice device = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  VkShaderModule vertShader = VK_NULL_HANDLE;
  VkShaderModule fragShader = VK_NULL_HANDLE;

public:
  bool init(VkDevice dev, VkRenderPass renderPass, const std::string &vertPath,
            const std::string &fragPath) {
    device = dev;

    auto vertCode = readFile(vertPath);
    auto fragCode = readFile(fragPath);
    if (vertCode.empty() || fragCode.empty())
      return false;

    vertShader = createShaderModule(vertCode);
    fragShader = createShaderModule(fragCode);

    // Descriptor layout: texture + bone buffer
    VkDescriptorSetLayoutBinding bindings[2] = {};
    bindings[0] = {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                   VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    bindings[1] = {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                   VK_SHADER_STAGE_VERTEX_BIT, nullptr};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                &descriptorSetLayout);

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
    vertexInput.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
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
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = 0xF;

    VkPipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &blendAttachment;

    VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                  VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynStates;

    VkPushConstantRange pushRange{};
    pushRange.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
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

    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr,
                              &pipeline);
    return true;
  }

  void bind(VkCommandBuffer cmd) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  }

  void pushConstants(VkCommandBuffer cmd, const PushConstants &pc) {
    vkCmdPushConstants(cmd, pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT |
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(pc), &pc);
  }

  void bindDescriptor(VkCommandBuffer cmd, VkDescriptorSet set) {
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &set, 0, nullptr);
  }

  VkDescriptorSetLayout getDescriptorLayout() const {
    return descriptorSetLayout;
  }

  void cleanup() {
    if (pipeline)
      vkDestroyPipeline(device, pipeline, nullptr);
    if (pipelineLayout)
      vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    if (descriptorSetLayout)
      vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    if (vertShader)
      vkDestroyShaderModule(device, vertShader, nullptr);
    if (fragShader)
      vkDestroyShaderModule(device, fragShader, nullptr);
  }

private:
  std::vector<char> readFile(const std::string &path) {
    std::ifstream f(path, std::ios::ate | std::ios::binary);
    if (!f)
      return {};
    size_t size = f.tellg();
    std::vector<char> buf(size);
    f.seekg(0);
    f.read(buf.data(), size);
    return buf;
  }

  VkShaderModule createShaderModule(const std::vector<char> &code) {
    VkShaderModuleCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = code.size();
    ci.pCode = reinterpret_cast<const uint32_t *>(code.data());
    VkShaderModule mod;
    vkCreateShaderModule(device, &ci, nullptr, &mod);
    return mod;
  }
};

class BoneBuffer {
  VmaAllocator allocator = nullptr;
  VkBuffer buffer = VK_NULL_HANDLE;
  VmaAllocation allocation = nullptr;
  void *mapped = nullptr;

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
    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation,
                    &info);
    mapped = info.pMappedData;

    std::vector<glm::mat4> identity(128, glm::mat4(1.0f));
    memcpy(mapped, identity.data(), sizeof(glm::mat4) * 128);
  }

  void update(const std::vector<glm::mat4> &bones) {
    memcpy(mapped, bones.data(),
           sizeof(glm::mat4) * std::min(bones.size(), size_t(128)));
  }

  VkBuffer getBuffer() const { return buffer; }

  void cleanup() {
    if (buffer)
      vmaDestroyBuffer(allocator, buffer, allocation);
  }
};

// Forward declarations for Renderable
class Pipeline;
extern Pipeline *g_pipeline;
extern VkDescriptorPool g_descriptorPool;
extern VulkanRenderer *g_renderer;
extern ModelLoader *g_modelLoader;
extern Camera *g_camera;

struct Renderable {
  Model *model = nullptr;
  AnimatorComponent animator;
  BoneBuffer boneBuffer;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  glm::mat4 transform = glm::mat4(1.0f);

  void init(Model *mdl, VmaAllocator alloc, VkDevice device,
            Texture &defaultTex, VkDescriptorSetLayout layout) {
    model = mdl;
    boneBuffer.create(alloc);

    if (mdl->hasAnimations()) {
      animator.init(mdl);
      animator.play(0);
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = g_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;
    vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

    Texture *tex = mdl->textures.empty() ? &defaultTex : &mdl->textures[0];

    VkDescriptorImageInfo imgInfo{};
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imgInfo.imageView = tex->view;
    imgInfo.sampler = tex->sampler;

    VkDescriptorBufferInfo bufInfo{};
    bufInfo.buffer = boneBuffer.getBuffer();
    bufInfo.offset = 0;
    bufInfo.range = sizeof(glm::mat4) * 128;

    VkWriteDescriptorSet writes[2] = {};
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

    vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
  }

  void update(float dt) {
    if (!model || !model->hasAnimations() || !animator.playing)
      return;

    const Animation &anim = model->animations[animator.animationIndex];
    float duration = anim.duration / anim.ticksPerSecond;

    animator.currentTime += dt * animator.speed;
    if (animator.currentTime >= duration) {
      animator.currentTime = fmod(animator.currentTime, duration);
    }

    calculateBones();
    boneBuffer.update(animator.finalTransforms);
  }

void calculateBones() {
    if (!model || model->animations.empty() || animator.animationIndex < 0)
        return;

    const Animation &anim = model->animations[animator.animationIndex];
    float tick = fmod(animator.currentTime * anim.ticksPerSecond, anim.duration);

    // Get current animation pose
    std::unordered_map<std::string, glm::vec3> positions;
    std::unordered_map<std::string, glm::quat> rotations;
    std::unordered_map<std::string, glm::vec3> scales;
    
    sampleAnimation(anim, tick, positions, rotations, scales);
    
    // Blend with previous animation if blending
    if (animator.blending && animator.blendFromIndex >= 0) {
        const Animation &fromAnim = model->animations[animator.blendFromIndex];
        float fromTick = fmod(animator.blendFromTime * fromAnim.ticksPerSecond, fromAnim.duration);
        
        std::unordered_map<std::string, glm::vec3> fromPositions;
        std::unordered_map<std::string, glm::quat> fromRotations;
        std::unordered_map<std::string, glm::vec3> fromScales;
        
        sampleAnimation(fromAnim, fromTick, fromPositions, fromRotations, fromScales);
        
        // Blend transforms
        float t = animator.blendFactor;
        for (auto& [name, pos] : positions) {
            auto it = fromPositions.find(name);
            if (it != fromPositions.end()) {
                pos = glm::mix(it->second, pos, t);
            }
        }
        for (auto& [name, rot] : rotations) {
            auto it = fromRotations.find(name);
            if (it != fromRotations.end()) {
                rot = glm::slerp(it->second, rot, t);
            }
        }
        for (auto& [name, scl] : scales) {
            auto it = fromScales.find(name);
            if (it != fromScales.end()) {
                scl = glm::mix(it->second, scl, t);
            }
        }
    }
    
    // Root motion extraction
    glm::vec3 rootPosition(0);
    if (animator.useRootMotion && !model->bones.empty()) {
        auto it = positions.find(model->bones[0].name);
        if (it != positions.end()) {
            rootPosition = it->second;
            animator.rootMotionDelta = rootPosition - animator.lastRootPosition;
            animator.lastRootPosition = rootPosition;
            
            // Zero out root translation for in-place animation
            it->second = glm::vec3(0, rootPosition.y, 0); // Keep Y for jumping
        }
    }

    // Build bone transforms
    for (size_t i = 0; i < model->bones.size(); i++) {
        const BoneInfo &bone = model->bones[i];

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

void sampleAnimation(const Animation& anim, float tick,
                     std::unordered_map<std::string, glm::vec3>& positions,
                     std::unordered_map<std::string, glm::quat>& rotations,
                     std::unordered_map<std::string, glm::vec3>& scales) {
    for (const auto &ch : anim.channels) {
        positions[ch.nodeName] = interpVec3(ch.positions, tick, glm::vec3(0));
        rotations[ch.nodeName] = interpQuat(ch.rotations, tick);
        scales[ch.nodeName] = interpVec3(ch.scales, tick, glm::vec3(1));
    }
}

  glm::vec3 interpVec3(const std::vector<std::pair<float, glm::vec3>> &keys,
                       float t, glm::vec3 def) {
    if (keys.empty())
      return def;
    if (keys.size() == 1)
      return keys[0].second;
    size_t i = 0;
    for (; i < keys.size() - 1 && t >= keys[i + 1].first; i++)
      ;
    if (i >= keys.size() - 1)
      return keys.back().second;
    float f = (t - keys[i].first) / (keys[i + 1].first - keys[i].first);
    return glm::mix(keys[i].second, keys[i + 1].second, f);
  }

  glm::quat interpQuat(const std::vector<std::pair<float, glm::quat>> &keys,
                       float t) {
    if (keys.empty())
      return glm::quat(1, 0, 0, 0);
    if (keys.size() == 1)
      return keys[0].second;
    size_t i = 0;
    for (; i < keys.size() - 1 && t >= keys[i + 1].first; i++)
      ;
    if (i >= keys.size() - 1)
      return keys.back().second;
    float f = (t - keys[i].first) / (keys[i + 1].first - keys[i].first);
    return glm::slerp(keys[i].second, keys[i + 1].second, f);
  }

  void cleanup() { boneBuffer.cleanup(); }
};
