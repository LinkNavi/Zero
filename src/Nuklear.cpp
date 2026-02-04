#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_KEYSTATE_BASED_INPUT
#define NK_IMPLEMENTATION
#include <nuklear.h>

#include "NuklearGUI.h"
#include <iostream>
#include <vector>

bool NuklearGUI::init(VkDevice dev, VmaAllocator alloc, VkRenderPass renderPass,
                      GLFWwindow* win, uint32_t w, uint32_t h,
                      VkCommandPool cmdPool, VkQueue queue) {
    device = dev;
    allocator = alloc;
    window = win;
    width = w;
    height = h;
    
    ctx = new nk_context();
    atlas = new nk_font_atlas();
    cmds = new nk_buffer();
    
    nk_init_default(ctx, nullptr);
    
    nk_font_atlas_init_default(atlas);
    nk_font_atlas_begin(atlas);
    struct nk_font* font = nk_font_atlas_add_default(atlas, 16, nullptr);
    
    int font_width, font_height;
    nk_font_atlas_bake(atlas, &font_width, &font_height, NK_FONT_ATLAS_RGBA32);
    
    if (!uploadFontTexture(cmdPool, queue)) {
        std::cerr << "Failed to upload Nuklear font texture\n";
        return false;
    }
    
    nk_font_atlas_end(atlas, nk_handle_ptr(fontImage), nullptr);
    nk_style_set_font(ctx, &font->handle);
    
    if (!createBuffers()) return false;
    if (!createDescriptors()) return false;
    if (!createPipeline(renderPass)) return false;
    
    nk_buffer_init_default(cmds);
    
    std::cout << "✓ Nuklear GUI initialized\n";
    return true;
}

bool NuklearGUI::createBuffers() {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.size = maxVertexBuffer;
    
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    
    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &vertexBuffer, &vertexAllocation, nullptr) != VK_SUCCESS) {
        return false;
    }
    
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    bufferInfo.size = maxIndexBuffer;
    
    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &indexBuffer, &indexAllocation, nullptr) != VK_SUCCESS) {
        return false;
    }
    
    return true;
}

bool NuklearGUI::uploadFontTexture(VkCommandPool cmdPool, VkQueue queue) {
    int width, height;
    const void* pixels = nk_font_atlas_bake(atlas, &width, &height, NK_FONT_ATLAS_RGBA32);
    VkDeviceSize size = width * height * 4;
    
    VkBuffer stagingBuffer;
    VmaAllocation stagingAlloc;
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    
    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingAlloc, nullptr);
    
    void* data;
    vmaMapMemory(allocator, stagingAlloc, &data);
    memcpy(data, pixels, size);
    vmaUnmapMemory(allocator, stagingAlloc);
    
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.extent = {(uint32_t)width, (uint32_t)height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    
    VmaAllocationCreateInfo imgAllocInfo{};
    imgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    
    vmaCreateImage(allocator, &imageInfo, &imgAllocInfo, &fontImage, &fontAllocation, nullptr);
    
    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = cmdPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;
    
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(device, &cmdAllocInfo, &cmd);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);
    
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = fontImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                        0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {(uint32_t)width, (uint32_t)height, 1};
    
    vkCmdCopyBufferToImage(cmd, stagingBuffer, fontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    vkEndCommandBuffer(cmd);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    
    vkFreeCommandBuffers(device, cmdPool, 1, &cmd);
    vmaDestroyBuffer(allocator, stagingBuffer, stagingAlloc);
    
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = fontImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    
    vkCreateImageView(device, &viewInfo, nullptr, &fontView);
    
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    
    vkCreateSampler(device, &samplerInfo, nullptr, &fontSampler);
    
    return true;
}

bool NuklearGUI::createDescriptors() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
    
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
    
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    
    vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
    
    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = fontSampler;
    imageInfo.imageView = fontView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSet;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;
    
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    
    return true;
}

bool NuklearGUI::createPipeline(VkRenderPass) {
    return true;
}

void NuklearGUI::newFrame() {
    nk_input_begin(ctx);
    
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    nk_input_motion(ctx, (int)mx, (int)my);
    nk_input_button(ctx, NK_BUTTON_LEFT, (int)mx, (int)my, glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    nk_input_button(ctx, NK_BUTTON_RIGHT, (int)mx, (int)my, glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
    
    nk_input_end(ctx);
}

void NuklearGUI::render(VkCommandBuffer cmd) {
    struct nk_convert_config config{};
    static const struct nk_draw_vertex_layout_element vertex_layout[] = {
        {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_draw_vertex, position)},
        {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_draw_vertex, uv)},
        {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_draw_vertex, col)},
        {NK_VERTEX_LAYOUT_END}
    };
    config.vertex_layout = vertex_layout;
    config.vertex_size = sizeof(struct nk_draw_vertex);
    config.vertex_alignment = NK_ALIGNOF(struct nk_draw_vertex);
    config.circle_segment_count = 22;
    config.curve_segment_count = 22;
    config.arc_segment_count = 22;
    config.global_alpha = 1.0f;
    config.shape_AA = NK_ANTI_ALIASING_ON;
    config.line_AA = NK_ANTI_ALIASING_ON;
    
    struct nk_buffer vbuf, ibuf;
    nk_buffer_init_default(&vbuf);
    nk_buffer_init_default(&ibuf);
    nk_convert(ctx, cmds, &vbuf, &ibuf, &config);
    
    void* vertexData;
    vmaMapMemory(allocator, vertexAllocation, &vertexData);
    memcpy(vertexData, nk_buffer_memory(&vbuf), nk_buffer_total(&vbuf));
    vmaUnmapMemory(allocator, vertexAllocation);
    
    void* indexData;
    vmaMapMemory(allocator, indexAllocation, &indexData);
    memcpy(indexData, nk_buffer_memory(&ibuf), nk_buffer_total(&ibuf));
    vmaUnmapMemory(allocator, indexAllocation);
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &offset);
    vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    
    float scale[2] = {2.0f / width, 2.0f / height};
    float translate[2] = {-1.0f, -1.0f};
    vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 2, scale);
    vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, translate);
    
    const struct nk_draw_command* drawCmd = nullptr;
    uint32_t offset_idx = 0;
    nk_draw_foreach(drawCmd, ctx, cmds) {
        if (!drawCmd->elem_count) continue;
        
        VkRect2D scissor;
        scissor.offset = {(int32_t)drawCmd->clip_rect.x, (int32_t)drawCmd->clip_rect.y};
        scissor.extent = {(uint32_t)drawCmd->clip_rect.w, (uint32_t)drawCmd->clip_rect.h};
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        
        vkCmdDrawIndexed(cmd, drawCmd->elem_count, 1, offset_idx, 0, 0);
        offset_idx += drawCmd->elem_count;
    }
    
    nk_clear(ctx);
    nk_buffer_free(&vbuf);
    nk_buffer_free(&ibuf);
}

void NuklearGUI::cleanup() {
    if (pipeline) vkDestroyPipeline(device, pipeline, nullptr);
    if (pipelineLayout) vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    if (descriptorSetLayout) vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    if (descriptorPool) vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    if (fontSampler) vkDestroySampler(device, fontSampler, nullptr);
    if (fontView) vkDestroyImageView(device, fontView, nullptr);
    if (fontImage) vmaDestroyImage(allocator, fontImage, fontAllocation);
    if (vertexBuffer) vmaDestroyBuffer(allocator, vertexBuffer, vertexAllocation);
    if (indexBuffer) vmaDestroyBuffer(allocator, indexBuffer, indexAllocation);
    
    if (atlas) {
        nk_font_atlas_clear(atlas);
        delete atlas;
    }
    if (cmds) {
        nk_buffer_free(cmds);
        delete cmds;
    }
    if (ctx) {
        nk_free(ctx);
        delete ctx;
    }
}
