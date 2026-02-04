// include/Skybox.h
#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <vector>
#include <iostream>
#include <string>
#include <stb_image.h>

class Skybox {
    VkDevice device = VK_NULL_HANDLE;
    VmaAllocator allocator = nullptr;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation vertexAlloc = nullptr;
    
    VkImage cubemapImage = VK_NULL_HANDLE;
    VmaAllocation cubemapAlloc = nullptr;
    VkImageView cubemapView = VK_NULL_HANDLE;
    VkSampler cubemapSampler = VK_NULL_HANDLE;
    
    VkBuffer uniformBuffer = VK_NULL_HANDLE;
    VmaAllocation uniformAlloc = nullptr;
    void* uniformMapped = nullptr;
    
    struct UBO {
        glm::mat4 view;
        glm::mat4 projection;
    };
    
public:
    bool init(VkDevice dev, VmaAllocator alloc, VkDescriptorPool pool, VkRenderPass renderPass,
              VkCommandPool cmdPool, VkQueue q, const std::string& vertPath, const std::string& fragPath,
              const std::vector<std::string>& facesPaths) {
        device = dev;
        allocator = alloc;
        commandPool = cmdPool;
        queue = q;
        
        if (!createCubemap(facesPaths)) {
            std::cerr << "Failed to create cubemap\n";
            return false;
        }
        if (!createVertexBuffer()) {
            std::cerr << "Failed to create vertex buffer\n";
            return false;
        }
        if (!createUniformBuffer()) {
            std::cerr << "Failed to create uniform buffer\n";
            return false;
        }
        if (!createDescriptors(pool)) {
            std::cerr << "Failed to create descriptors\n";
            return false;
        }
        if (!createPipeline(renderPass, vertPath, fragPath)) {
            std::cerr << "Failed to create pipeline\n";
            return false;
        }
        
        std::cout << "✓ Skybox initialized\n";
        return true;
    }
    
    void render(VkCommandBuffer cmd, const glm::mat4& view, const glm::mat4& proj) {
        if (pipeline == VK_NULL_HANDLE || vertexBuffer == VK_NULL_HANDLE) return;
        
        // Remove translation from view matrix
        glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));
        
        UBO ubo{viewNoTranslation, proj};
        memcpy(uniformMapped, &ubo, sizeof(UBO));
        
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);
        
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &offset);
        vkCmdDraw(cmd, 36, 1, 0, 0);
    }
    
    void cleanup() {
        if (pipeline) vkDestroyPipeline(device, pipeline, nullptr);
        if (layout) vkDestroyPipelineLayout(device, layout, nullptr);
        if (descLayout) vkDestroyDescriptorSetLayout(device, descLayout, nullptr);
        if (cubemapSampler) vkDestroySampler(device, cubemapSampler, nullptr);
        if (cubemapView) vkDestroyImageView(device, cubemapView, nullptr);
        if (cubemapImage) vmaDestroyImage(allocator, cubemapImage, cubemapAlloc);
        if (vertexBuffer) vmaDestroyBuffer(allocator, vertexBuffer, vertexAlloc);
        if (uniformBuffer) vmaDestroyBuffer(allocator, uniformBuffer, uniformAlloc);
    }
    
private:
    bool createCubemap(const std::vector<std::string>& faces) {
        if (faces.size() != 6) {
            std::cerr << "Skybox needs 6 faces\n";
            return false;
        }
        stbi_set_flip_vertically_on_load(false);
        int width, height, channels;
        stbi_uc* pixels = stbi_load(faces[0].c_str(), &width, &height, &channels, 4);
        if (!pixels) {
            std::cerr << "Failed to load skybox face: " << faces[0] << "\n";
            return false;
        }
        
        std::cout << "Loaded skybox face: " << faces[0] << " (" << width << "x" << height << ")\n";
        
        VkDeviceSize layerSize = width * height * 4;
        VkDeviceSize imageSize = layerSize * 6;
        
        // Staging buffer
        VkBuffer stagingBuffer;
        VmaAllocation stagingAlloc;
        VkBufferCreateInfo bufInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufInfo.size = imageSize;
        bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        if (vmaCreateBuffer(allocator, &bufInfo, &allocInfo, &stagingBuffer, &stagingAlloc, nullptr) != VK_SUCCESS) {
            stbi_image_free(pixels);
            return false;
        }
        
        void* data;
        vmaMapMemory(allocator, stagingAlloc, &data);
        
        // Load all 6 faces
        memcpy(data, pixels, layerSize);
        stbi_image_free(pixels);
        
        for (int i = 1; i < 6; ++i) {
            pixels = stbi_load(faces[i].c_str(), &width, &height, &channels, 4);
            if (!pixels) {
                std::cerr << "Failed to load skybox face: " << faces[i] << "\n";
                vmaUnmapMemory(allocator, stagingAlloc);
                vmaDestroyBuffer(allocator, stagingBuffer, stagingAlloc);
                return false;
            }
            memcpy((char*)data + layerSize * i, pixels, layerSize);
            stbi_image_free(pixels);
        }
        
        vmaUnmapMemory(allocator, stagingAlloc);
        
        // Create cubemap image
        VkImageCreateInfo imgInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imgInfo.imageType = VK_IMAGE_TYPE_2D;
       imgInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imgInfo.extent = {(uint32_t)width, (uint32_t)height, 1};
        imgInfo.mipLevels = 1;
        imgInfo.arrayLayers = 6;
        imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imgInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        
        VmaAllocationCreateInfo imgAllocInfo{};
        imgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        if (vmaCreateImage(allocator, &imgInfo, &imgAllocInfo, &cubemapImage, &cubemapAlloc, nullptr) != VK_SUCCESS) {
            vmaDestroyBuffer(allocator, stagingBuffer, stagingAlloc);
            return false;
        }
        
        // Copy buffer to image
        VkCommandBuffer cmd = beginSingleTimeCommands();
        
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.image = cubemapImage;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6};
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        VkBufferImageCopy region{};
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 6};
        region.imageExtent = {(uint32_t)width, (uint32_t)height, 1};
        vkCmdCopyBufferToImage(cmd, stagingBuffer, cubemapImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        endSingleTimeCommands(cmd);
        vmaDestroyBuffer(allocator, stagingBuffer, stagingAlloc);
        
        // Image view
        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = cubemapImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
       viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6};
        if (vkCreateImageView(device, &viewInfo, nullptr, &cubemapView) != VK_SUCCESS) {
            return false;
        }
        
        // Sampler
        VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.maxLod = 1.0f;
        if (vkCreateSampler(device, &samplerInfo, nullptr, &cubemapSampler) != VK_SUCCESS) {
            return false;
        }
        
        return true;
    }
    
   bool createVertexBuffer() {
    // Scale up the cube - was -1 to 1, now -100 to 100
    float vertices[] = {
        -100,-100,-100,  100,-100,-100,  100,100,-100,  100,100,-100,  -100,100,-100,  -100,-100,-100,
        -100,-100,100,  -100,100,100,  100,100,100,  100,100,100,  100,-100,100,  -100,-100,100,
        -100,100,-100,  -100,100,100,  100,100,100,  100,100,100,  100,100,-100,  -100,100,-100,
        -100,-100,-100,  100,-100,-100,  100,-100,100,  100,-100,100,  -100,-100,100,  -100,-100,-100,
        -100,-100,-100,  -100,-100,100,  -100,100,100,  -100,100,100,  -100,100,-100,  -100,-100,-100,
        100,-100,-100,  100,100,-100,  100,100,100,  100,100,100,  100,-100,100,  100,-100,-100
    };
        
        VkDeviceSize bufSize = sizeof(vertices);
        
        // Create staging buffer
        VkBuffer stagingBuffer;
        VmaAllocation stagingAlloc;
        
        VkBufferCreateInfo bufInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufInfo.size = bufSize;
        bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        if (vmaCreateBuffer(allocator, &bufInfo, &allocInfo, &stagingBuffer, &stagingAlloc, nullptr) != VK_SUCCESS) {
            return false;
        }
        
        // Copy data to staging
        void* data;
        vmaMapMemory(allocator, stagingAlloc, &data);
        memcpy(data, vertices, bufSize);
        vmaUnmapMemory(allocator, stagingAlloc);
        
        // Create vertex buffer
        bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        VmaAllocationCreateInfo vbAllocInfo{};
        vbAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        if (vmaCreateBuffer(allocator, &bufInfo, &vbAllocInfo, &vertexBuffer, &vertexAlloc, nullptr) != VK_SUCCESS) {
            vmaDestroyBuffer(allocator, stagingBuffer, stagingAlloc);
            return false;
        }
        
        // Copy staging to vertex buffer
        VkCommandBuffer cmd = beginSingleTimeCommands();
        VkBufferCopy copyRegion{};
        copyRegion.size = bufSize;
        vkCmdCopyBuffer(cmd, stagingBuffer, vertexBuffer, 1, &copyRegion);
        endSingleTimeCommands(cmd);
        
        vmaDestroyBuffer(allocator, stagingBuffer, stagingAlloc);
        return true;
    }
    
    bool createUniformBuffer() {
        VkBufferCreateInfo bufInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufInfo.size = sizeof(UBO);
        bufInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        
        VmaAllocationInfo allocationInfo;
        if (vmaCreateBuffer(allocator, &bufInfo, &allocInfo, &uniformBuffer, &uniformAlloc, &allocationInfo) != VK_SUCCESS) {
            return false;
        }
        uniformMapped = allocationInfo.pMappedData;
        return true;
    }
    
    bool createDescriptors(VkDescriptorPool pool) {
        VkDescriptorSetLayoutBinding bindings[2] = {};
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        
        VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        layoutInfo.bindingCount = 2;
        layoutInfo.pBindings = bindings;
        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descLayout) != VK_SUCCESS) {
            return false;
        }
        
        VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descLayout;
        if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
            return false;
        }
        
        VkDescriptorBufferInfo bufInfo{uniformBuffer, 0, sizeof(UBO)};
        VkDescriptorImageInfo imgInfo{cubemapSampler, cubemapView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        
        VkWriteDescriptorSet writes[2] = {};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = descriptorSet;
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo = &bufInfo;
        
        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = descriptorSet;
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &imgInfo;
        
        vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
        return true;
    }
    
    bool createPipeline(VkRenderPass renderPass, const std::string& vertPath, const std::string& fragPath) {
         auto vertCode = readFile(vertPath);
    auto fragCode = readFile(fragPath);
    if (vertCode.empty()) {
        std::cerr << "Failed to read vertex shader: " << vertPath << "\n";
        return false;
    }
    if (fragCode.empty()) {
        std::cerr << "Failed to read fragment shader: " << fragPath << "\n";
        return false;
    }
        if (vertCode.empty() || fragCode.empty()) return false;
        
        VkShaderModule vertModule = createShaderModule(vertCode);
        VkShaderModule fragModule = createShaderModule(fragCode);
        
        VkPipelineShaderStageCreateInfo stages[2] = {};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertModule;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragModule;
        stages[1].pName = "main";
        
        VkVertexInputBindingDescription binding{0, sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX};
        VkVertexInputAttributeDescription attr{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0};
        
        VkPipelineVertexInputStateCreateInfo vertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &binding;
        vertexInput.vertexAttributeDescriptionCount = 1;
        vertexInput.pVertexAttributeDescriptions = &attr;
        
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        
        VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;
        
        VkPipelineRasterizationStateCreateInfo rasterizer{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.lineWidth = 1.0f;
        
        VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        
        VkPipelineDepthStencilStateCreateInfo depthStencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_FALSE;
       depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
        
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = 0xF;
        
        VkPipelineColorBlendStateCreateInfo colorBlend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        colorBlend.attachmentCount = 1;
        colorBlend.pAttachments = &colorBlendAttachment;
        
        VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynStates;
        
        VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &descLayout;
        if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
            vkDestroyShaderModule(device, vertModule, nullptr);
            vkDestroyShaderModule(device, fragModule, nullptr);
            return false;
        }
        
        VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = stages;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisample;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlend;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = layout;
        pipelineInfo.renderPass = renderPass;
        
        VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
        
        vkDestroyShaderModule(device, vertModule, nullptr);
        vkDestroyShaderModule(device, fragModule, nullptr);
        
        return result == VK_SUCCESS;
    }
    
    VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(device, &allocInfo, &cmd);
        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);
        return cmd;
    }
    
    void endSingleTimeCommands(VkCommandBuffer cmd) {
        vkEndCommandBuffer(cmd);
        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);
        vkFreeCommandBuffers(device, commandPool, 1, &cmd);
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
        VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        ci.codeSize = code.size();
        ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule mod;
        vkCreateShaderModule(device, &ci, nullptr, &mod);
        return mod;
    }
};
