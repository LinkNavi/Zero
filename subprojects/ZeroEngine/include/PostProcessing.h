#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>
#include <iostream>

struct BloomSettings {
    float threshold = 1.0f;
    float intensity = 1.0f;
    float strength = 0.5f;
    bool enabled = false;
};

struct PostProcessSettings {
    BloomSettings bloom;
    float exposure = 1.0f;
    float gamma = 2.2f;
};

class PostProcessing {
    VkDevice device = VK_NULL_HANDLE;
    VmaAllocator allocator = nullptr;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    
    uint32_t width = 0, height = 0;
    uint32_t bloomWidth = 0, bloomHeight = 0;
    
    // Offscreen scene render target
    VkImage sceneImage = VK_NULL_HANDLE;
    VkImageView sceneView = VK_NULL_HANDLE;
    VmaAllocation sceneAlloc = nullptr;
    
    VkImage sceneDepthImage = VK_NULL_HANDLE;
    VkImageView sceneDepthView = VK_NULL_HANDLE;
    VmaAllocation sceneDepthAlloc = nullptr;
    
    VkRenderPass sceneRenderPass = VK_NULL_HANDLE;
    VkFramebuffer sceneFramebuffer = VK_NULL_HANDLE;
    
    // Bloom target at 1/4 res
    VkImage bloomImage = VK_NULL_HANDLE;
    VkImageView bloomView = VK_NULL_HANDLE;
    VmaAllocation bloomAlloc = nullptr;
    
    VkRenderPass bloomRenderPass = VK_NULL_HANDLE;
    VkFramebuffer bloomFramebuffer = VK_NULL_HANDLE;
    
    VkSampler linearSampler = VK_NULL_HANDLE;
    
    // Bloom extract pipeline
    VkPipelineLayout bloomLayout = VK_NULL_HANDLE;
    VkPipeline bloomPipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout bloomDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet bloomDescSet = VK_NULL_HANDLE;
    
    // Final composite pipeline (scene + bloom to swapchain)
    VkPipelineLayout compositeLayout = VK_NULL_HANDLE;
    VkPipeline compositePipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout compositeDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet compositeDescSet = VK_NULL_HANDLE;
    
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    
    struct BloomPC { float threshold, intensity, texelX, texelY; };
    struct CompositePC { float strength, exposure, gamma, bloomEnabled; };

public:
    PostProcessSettings settings;
    
    bool init(VkDevice dev, VmaAllocator alloc, VkDescriptorPool pool,
              uint32_t w, uint32_t h, VkFormat depthFmt,
              const std::string& fullscreenVertPath,
              const std::string& bloomFragPath,
              const std::string& compositeFragPath) {
        device = dev;
        allocator = alloc;
        descriptorPool = pool;
        width = w;
        height = h;
        bloomWidth = w / 4;
        bloomHeight = h / 4;
        depthFormat = depthFmt;
        
        if (!createSampler()) return false;
        if (!createSceneResources()) return false;
        if (!createBloomResources()) return false;
        if (!createDescriptors()) return false;
        if (!createPipelines(fullscreenVertPath, bloomFragPath, compositeFragPath)) return false;
        
        std::cout << "✓ PostProcessing initialized (" << width << "x" << height << ", bloom " << bloomWidth << "x" << bloomHeight << ")\n";
        return true;
    }
    
    VkRenderPass getSceneRenderPass() const { return sceneRenderPass; }
    VkFramebuffer getSceneFramebuffer() const { return sceneFramebuffer; }
    
    void beginScenePass(VkCommandBuffer cmd, const std::array<VkClearValue, 2>& clearValues) {
        VkRenderPassBeginInfo rpInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rpInfo.renderPass = sceneRenderPass;
        rpInfo.framebuffer = sceneFramebuffer;
        rpInfo.renderArea = {{0, 0}, {width, height}};
        rpInfo.clearValueCount = 2;
        rpInfo.pClearValues = clearValues.data();
        
        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        VkViewport viewport{0, 0, float(width), float(height), 0, 1};
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        VkRect2D scissor{{0, 0}, {width, height}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }
    
    void endScenePass(VkCommandBuffer cmd) {
        vkCmdEndRenderPass(cmd);
    }
    
    void applyPostProcess(VkCommandBuffer cmd, VkRenderPass swapchainPass, VkFramebuffer swapchainFB) {
        // Transition scene image for sampling
        transitionImage(cmd, sceneImage, 
                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                       VK_ACCESS_SHADER_READ_BIT);
        
        if (settings.bloom.enabled) {
            renderBloom(cmd);
        }
        
        // Composite to swapchain
        composite(cmd, swapchainPass, swapchainFB);
        
        // Transition scene image back for next frame
        transitionImage(cmd, sceneImage,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_ACCESS_SHADER_READ_BIT,
                       VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
    }
    
    void cleanup() {
        auto destroyImg = [&](VkImage& i, VkImageView& v, VmaAllocation& a) {
            if (v) vkDestroyImageView(device, v, nullptr);
            if (i) vmaDestroyImage(allocator, i, a);
            v = VK_NULL_HANDLE; i = VK_NULL_HANDLE; a = nullptr;
        };
        
        destroyImg(sceneImage, sceneView, sceneAlloc);
        destroyImg(sceneDepthImage, sceneDepthView, sceneDepthAlloc);
        destroyImg(bloomImage, bloomView, bloomAlloc);
        
        if (linearSampler) vkDestroySampler(device, linearSampler, nullptr);
        if (sceneFramebuffer) vkDestroyFramebuffer(device, sceneFramebuffer, nullptr);
        if (sceneRenderPass) vkDestroyRenderPass(device, sceneRenderPass, nullptr);
        if (bloomFramebuffer) vkDestroyFramebuffer(device, bloomFramebuffer, nullptr);
        if (bloomRenderPass) vkDestroyRenderPass(device, bloomRenderPass, nullptr);
        if (bloomPipeline) vkDestroyPipeline(device, bloomPipeline, nullptr);
        if (bloomLayout) vkDestroyPipelineLayout(device, bloomLayout, nullptr);
        if (bloomDescLayout) vkDestroyDescriptorSetLayout(device, bloomDescLayout, nullptr);
        if (compositePipeline) vkDestroyPipeline(device, compositePipeline, nullptr);
        if (compositeLayout) vkDestroyPipelineLayout(device, compositeLayout, nullptr);
        if (compositeDescLayout) vkDestroyDescriptorSetLayout(device, compositeDescLayout, nullptr);
    }

private:
    void transitionImage(VkCommandBuffer cmd, VkImage image,
                        VkImageLayout oldLayout, VkImageLayout newLayout,
                        VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                        VkAccessFlags srcAccess, VkAccessFlags dstAccess) {
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        barrier.srcAccessMask = srcAccess;
        barrier.dstAccessMask = dstAccess;
        
        vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }
    
    bool createSampler() {
        VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        si.magFilter = si.minFilter = VK_FILTER_LINEAR;
        si.addressModeU = si.addressModeV = si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        return vkCreateSampler(device, &si, nullptr, &linearSampler) == VK_SUCCESS;
    }
    
    bool createSceneResources() {
        // Scene color image
        VkImageCreateInfo imgInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imgInfo.imageType = VK_IMAGE_TYPE_2D;
        imgInfo.extent = {width, height, 1};
        imgInfo.mipLevels = imgInfo.arrayLayers = 1;
        imgInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT; // HDR format
        imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        
        if (vmaCreateImage(allocator, &imgInfo, &allocInfo, &sceneImage, &sceneAlloc, nullptr) != VK_SUCCESS)
            return false;
        
        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = sceneImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        
        if (vkCreateImageView(device, &viewInfo, nullptr, &sceneView) != VK_SUCCESS)
            return false;
        
        // Scene depth image
        imgInfo.format = depthFormat;
        imgInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        
        if (vmaCreateImage(allocator, &imgInfo, &allocInfo, &sceneDepthImage, &sceneDepthAlloc, nullptr) != VK_SUCCESS)
            return false;
        
        viewInfo.image = sceneDepthImage;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        
        if (vkCreateImageView(device, &viewInfo, nullptr, &sceneDepthView) != VK_SUCCESS)
            return false;
        
        // Scene render pass
        VkAttachmentDescription attachments[2] = {};
        attachments[0].format = VK_FORMAT_R16G16B16A16_SFLOAT;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        attachments[1].format = depthFormat;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
        VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pDepthStencilAttachment = &depthRef;
        
        VkSubpassDependency dep{};
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass = 0;
        dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
        VkRenderPassCreateInfo rpInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rpInfo.attachmentCount = 2;
        rpInfo.pAttachments = attachments;
        rpInfo.subpassCount = 1;
        rpInfo.pSubpasses = &subpass;
        rpInfo.dependencyCount = 1;
        rpInfo.pDependencies = &dep;
        
        if (vkCreateRenderPass(device, &rpInfo, nullptr, &sceneRenderPass) != VK_SUCCESS)
            return false;
        
        // Scene framebuffer
        VkImageView fbViews[] = {sceneView, sceneDepthView};
        VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fbInfo.renderPass = sceneRenderPass;
        fbInfo.attachmentCount = 2;
        fbInfo.pAttachments = fbViews;
        fbInfo.width = width;
        fbInfo.height = height;
        fbInfo.layers = 1;
        
        return vkCreateFramebuffer(device, &fbInfo, nullptr, &sceneFramebuffer) == VK_SUCCESS;
    }
    
    bool createBloomResources() {
        VkImageCreateInfo imgInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imgInfo.imageType = VK_IMAGE_TYPE_2D;
        imgInfo.extent = {bloomWidth, bloomHeight, 1};
        imgInfo.mipLevels = imgInfo.arrayLayers = 1;
        imgInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        
        if (vmaCreateImage(allocator, &imgInfo, &allocInfo, &bloomImage, &bloomAlloc, nullptr) != VK_SUCCESS)
            return false;
        
        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = bloomImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        
        if (vkCreateImageView(device, &viewInfo, nullptr, &bloomView) != VK_SUCCESS)
            return false;
        
        // Bloom render pass
        VkAttachmentDescription att{};
        att.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        att.samples = VK_SAMPLE_COUNT_1_BIT;
        att.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        att.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        
        VkAttachmentReference ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription sub{};
        sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sub.colorAttachmentCount = 1;
        sub.pColorAttachments = &ref;
        
        VkRenderPassCreateInfo rpInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rpInfo.attachmentCount = 1;
        rpInfo.pAttachments = &att;
        rpInfo.subpassCount = 1;
        rpInfo.pSubpasses = &sub;
        
        if (vkCreateRenderPass(device, &rpInfo, nullptr, &bloomRenderPass) != VK_SUCCESS)
            return false;
        
        VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fbInfo.renderPass = bloomRenderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &bloomView;
        fbInfo.width = bloomWidth;
        fbInfo.height = bloomHeight;
        fbInfo.layers = 1;
        
        return vkCreateFramebuffer(device, &fbInfo, nullptr, &bloomFramebuffer) == VK_SUCCESS;
    }
    
    bool createDescriptors() {
        // Bloom descriptor - samples scene
        VkDescriptorSetLayoutBinding binding{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT};
        VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;
        
        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &bloomDescLayout) != VK_SUCCESS)
            return false;
        
        // Composite descriptor - samples scene + bloom
        VkDescriptorSetLayoutBinding bindings[2] = {
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT}
        };
        layoutInfo.bindingCount = 2;
        layoutInfo.pBindings = bindings;
        
        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &compositeDescLayout) != VK_SUCCESS)
            return false;
        
        // Allocate sets
        VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &bloomDescLayout;
        if (vkAllocateDescriptorSets(device, &allocInfo, &bloomDescSet) != VK_SUCCESS)
            return false;
        
        allocInfo.pSetLayouts = &compositeDescLayout;
        if (vkAllocateDescriptorSets(device, &allocInfo, &compositeDescSet) != VK_SUCCESS)
            return false;
        
        // Update bloom descriptor (scene image)
        VkDescriptorImageInfo sceneInfo{linearSampler, sceneView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = bloomDescSet;
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &sceneInfo;
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        
        // Update composite descriptor (scene + bloom)
        VkDescriptorImageInfo bloomInfo{linearSampler, bloomView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet writes[2] = {};
        writes[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, compositeDescSet, 0, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER};
        writes[0].pImageInfo = &sceneInfo;
        writes[1] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, compositeDescSet, 1, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER};
        writes[1].pImageInfo = &bloomInfo;
        vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
        
        return true;
    }
    
    bool createPipelines(const std::string& vertPath, const std::string& bloomPath, const std::string& compositePath) {
        auto vert = readFile(vertPath);
        if (vert.empty()) { std::cerr << "PostProcess: no vert shader\n"; return false; }
        
        VkShaderModule vertMod = createShader(vert);
        
        // Bloom pipeline
        auto bloom = readFile(bloomPath);
        if (!bloom.empty()) {
            VkPushConstantRange pc{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BloomPC)};
            VkPipelineLayoutCreateInfo li{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
            li.setLayoutCount = 1;
            li.pSetLayouts = &bloomDescLayout;
            li.pushConstantRangeCount = 1;
            li.pPushConstantRanges = &pc;
            vkCreatePipelineLayout(device, &li, nullptr, &bloomLayout);
            bloomPipeline = makePipeline(vertMod, createShader(bloom), bloomLayout, bloomRenderPass, false);
        }
        
        // Composite pipeline
        auto comp = readFile(compositePath);
        if (!comp.empty()) {
            VkPushConstantRange pc{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CompositePC)};
            VkPipelineLayoutCreateInfo li{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
            li.setLayoutCount = 1;
            li.pSetLayouts = &compositeDescLayout;
            li.pushConstantRangeCount = 1;
            li.pPushConstantRanges = &pc;
            vkCreatePipelineLayout(device, &li, nullptr, &compositeLayout);
        }
        
        vkDestroyShaderModule(device, vertMod, nullptr);
        return bloomPipeline != VK_NULL_HANDLE;
    }
    
    // Create composite pipeline for a specific swapchain render pass
    void ensureCompositePipeline(VkRenderPass swapchainPass) {
        if (compositePipeline) return;
        
        auto vert = readFile("shaders/fullscreen_vert.spv");
        auto comp = readFile("shaders/composite_frag.spv");
        if (vert.empty() || comp.empty()) return;
        
        VkShaderModule vertMod = createShader(vert);
        compositePipeline = makePipeline(vertMod, createShader(comp), compositeLayout, swapchainPass, false);
        vkDestroyShaderModule(device, vertMod, nullptr);
    }
    
    VkPipeline makePipeline(VkShaderModule vert, VkShaderModule frag, VkPipelineLayout layout,
                            VkRenderPass rp, bool additive) {
        VkPipelineShaderStageCreateInfo stages[2] = {
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vert, "main"},
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, frag, "main"}
        };
        
        VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        
        VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        vp.viewportCount = vp.scissorCount = 1;
        
        VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode = VK_CULL_MODE_NONE;
        rs.lineWidth = 1.0f;
        
        VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        
        VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        
        VkPipelineColorBlendAttachmentState cba{};
        cba.colorWriteMask = 0xF;
        if (additive) {
            cba.blendEnable = VK_TRUE;
            cba.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            cba.colorBlendOp = VK_BLEND_OP_ADD;
            cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            cba.alphaBlendOp = VK_BLEND_OP_ADD;
        }
        
        VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        cb.attachmentCount = 1;
        cb.pAttachments = &cba;
        
        VkDynamicState dyn[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynSt{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynSt.dynamicStateCount = 2;
        dynSt.pDynamicStates = dyn;
        
        VkGraphicsPipelineCreateInfo ci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        ci.stageCount = 2;
        ci.pStages = stages;
        ci.pVertexInputState = &vi;
        ci.pInputAssemblyState = &ia;
        ci.pViewportState = &vp;
        ci.pRasterizationState = &rs;
        ci.pMultisampleState = &ms;
        ci.pDepthStencilState = &ds;
        ci.pColorBlendState = &cb;
        ci.pDynamicState = &dynSt;
        ci.layout = layout;
        ci.renderPass = rp;
        
        VkPipeline p;
        vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &ci, nullptr, &p);
        vkDestroyShaderModule(device, frag, nullptr);
        return p;
    }
    
    void renderBloom(VkCommandBuffer cmd) {
        VkRenderPassBeginInfo rpbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rpbi.renderPass = bloomRenderPass;
        rpbi.framebuffer = bloomFramebuffer;
        rpbi.renderArea = {{0, 0}, {bloomWidth, bloomHeight}};
        
        vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
        
        VkViewport vp{0, 0, float(bloomWidth), float(bloomHeight), 0, 1};
        vkCmdSetViewport(cmd, 0, 1, &vp);
        VkRect2D sc{{0, 0}, {bloomWidth, bloomHeight}};
        vkCmdSetScissor(cmd, 0, 1, &sc);
        
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomPipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomLayout, 0, 1, &bloomDescSet, 0, nullptr);
        
        BloomPC pc{settings.bloom.threshold, settings.bloom.intensity, 1.0f / width, 1.0f / height};
        vkCmdPushConstants(cmd, bloomLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);
        vkCmdDraw(cmd, 3, 1, 0, 0);
        
        vkCmdEndRenderPass(cmd);
    }
    
    void composite(VkCommandBuffer cmd, VkRenderPass swapchainPass, VkFramebuffer swapchainFB) {
        // Lazily create composite pipeline for swapchain render pass
        if (!compositePipeline && compositeLayout) {
            auto vert = readFile("shaders/fullscreen_vert.spv");
            auto comp = readFile("shaders/composite_frag.spv");
            if (!vert.empty() && !comp.empty()) {
                VkShaderModule vertMod = createShader(vert);
                compositePipeline = makePipeline(vertMod, createShader(comp), compositeLayout, swapchainPass, false);
                vkDestroyShaderModule(device, vertMod, nullptr);
            }
        }
        
        if (!compositePipeline) return;
        
        VkRenderPassBeginInfo rpbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rpbi.renderPass = swapchainPass;
        rpbi.framebuffer = swapchainFB;
        rpbi.renderArea = {{0, 0}, {width, height}};
        
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        rpbi.clearValueCount = 2;
        rpbi.pClearValues = clearValues.data();
        
        vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
        
        VkViewport vp{0, 0, float(width), float(height), 0, 1};
        vkCmdSetViewport(cmd, 0, 1, &vp);
        VkRect2D sc{{0, 0}, {width, height}};
        vkCmdSetScissor(cmd, 0, 1, &sc);
        
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, compositePipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, compositeLayout, 0, 1, &compositeDescSet, 0, nullptr);
        
        CompositePC pc{settings.bloom.strength, settings.exposure, settings.gamma, settings.bloom.enabled ? 1.0f : 0.0f};
        vkCmdPushConstants(cmd, compositeLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);
        vkCmdDraw(cmd, 3, 1, 0, 0);
        
        // Don't end render pass - let UI render in same pass
    }
    
    std::vector<char> readFile(const std::string& p) {
        std::ifstream f(p, std::ios::ate | std::ios::binary);
        if (!f) return {};
        std::vector<char> b(f.tellg());
        f.seekg(0);
        f.read(b.data(), b.size());
        return b;
    }
    
    VkShaderModule createShader(const std::vector<char>& c) {
        VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        ci.codeSize = c.size();
        ci.pCode = (const uint32_t*)c.data();
        VkShaderModule m;
        vkCreateShaderModule(device, &ci, nullptr, &m);
        return m;
    }
};
