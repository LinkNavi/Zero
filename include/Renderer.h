#pragma once
#include <vulkan/vulkan.h>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
};

struct AllocatedImage {
    VkImage image;
    VkImageView view;
    VmaAllocation allocation;
};

class VulkanRenderer {
    GLFWwindow* window;
    vkb::Instance vkbInstance;
    vkb::Device vkbDevice;
    vkb::Swapchain vkbSwapchain;
    
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    
    VmaAllocator allocator;
    
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> framebuffers;
    VkRenderPass renderPass;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    
    VkSemaphore imageAvailable, renderFinished;
    VkFence inFlightFence;
    
    uint32_t width, height;
    uint32_t currentFrame = 0;

public:
    bool init(uint32_t w, uint32_t h, const char* title) {
        width = w;
        height = h;
        
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        
        // Instance
        vkb::InstanceBuilder builder;
        auto instRet = builder.set_app_name(title)
            .request_validation_layers(true)
            .use_default_debug_messenger()
            .require_api_version(1, 2, 0)
            .build();
        
        if (!instRet) return false;
        vkbInstance = instRet.value();
        instance = vkbInstance.instance;
        
        // Surface
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
            return false;
        
        // Device
        vkb::PhysicalDeviceSelector selector{vkbInstance};
        auto physRet = selector.set_surface(surface)
            .set_minimum_version(1, 2)
            .require_dedicated_transfer_queue()
            .select();
        
        if (!physRet) return false;
        vkb::PhysicalDevice vkbPhysicalDevice = physRet.value();
        
        vkb::DeviceBuilder deviceBuilder{vkbPhysicalDevice};
        auto devRet = deviceBuilder.build();
        if (!devRet) return false;
        
        vkbDevice = devRet.value();
        device = vkbDevice.device;
        physicalDevice = vkbPhysicalDevice.physical_device;
        
        auto gqRet = vkbDevice.get_queue(vkb::QueueType::graphics);
        if (!gqRet) return false;
        graphicsQueue = gqRet.value();
        
        auto pqRet = vkbDevice.get_queue(vkb::QueueType::present);
        if (!pqRet) return false;
        presentQueue = pqRet.value();
        
        // VMA
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;
        vmaCreateAllocator(&allocatorInfo, &allocator);
        
        // Swapchain
        if (!createSwapchain()) return false;
        if (!createRenderPass()) return false;
        if (!createFramebuffers()) return false;
        if (!createCommandPool()) return false;
        if (!createCommandBuffers()) return false;
        if (!createSyncObjects()) return false;
        
        return true;
    }

    void beginFrame(VkCommandBuffer& cmd) {
        vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlightFence);
        
        vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE, &currentFrame);
        
        cmd = commandBuffers[currentFrame];
        vkResetCommandBuffer(cmd, 0);
        
        VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);
        
        VkRenderPassBeginInfo rpInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rpInfo.renderPass = renderPass;
        rpInfo.framebuffer = framebuffers[currentFrame];
        rpInfo.renderArea.extent = {width, height};
        
        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        rpInfo.clearValueCount = 1;
        rpInfo.pClearValues = &clearColor;
        
        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void endFrame(VkCommandBuffer cmd) {
        vkCmdEndRenderPass(cmd);
        vkEndCommandBuffer(cmd);
        
        VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &imageAvailable;
        submitInfo.pWaitDstStageMask = &waitStage;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinished;
        
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);
        
        VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinished;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &currentFrame;
        
        vkQueuePresentKHR(presentQueue, &presentInfo);
    }

    bool shouldClose() { return glfwWindowShouldClose(window); }
    void pollEvents() { glfwPollEvents(); }
    
    VkDevice getDevice() { return device; }
    VmaAllocator getAllocator() { return allocator; }
    GLFWwindow* getWindow() { return window; }

    void cleanup() {
        vkDeviceWaitIdle(device);
        
        vkDestroySemaphore(device, imageAvailable, nullptr);
        vkDestroySemaphore(device, renderFinished, nullptr);
        vkDestroyFence(device, inFlightFence, nullptr);
        
        vkDestroyCommandPool(device, commandPool, nullptr);
        
        for (auto fb : framebuffers)
            vkDestroyFramebuffer(device, fb, nullptr);
        
        vkDestroyRenderPass(device, renderPass, nullptr);
        
        for (auto view : swapchainImageViews)
            vkDestroyImageView(device, view, nullptr);
        
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        vmaDestroyAllocator(allocator);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkbDevice.destroy();
        vkbInstance.destroy();
        
        glfwDestroyWindow(window);
        glfwTerminate();
    }

private:
    bool createSwapchain() {
        vkb::SwapchainBuilder swapchainBuilder{vkbDevice};
        auto swapRet = swapchainBuilder
            .set_desired_format({VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(width, height)
            .build();
        
        if (!swapRet) return false;
        
        vkbSwapchain = swapRet.value();
        swapchain = vkbSwapchain.swapchain;
        swapchainImages = vkbSwapchain.get_images().value();
        swapchainImageViews = vkbSwapchain.get_image_views().value();
        
        return true;
    }

    bool createRenderPass() {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        
        VkAttachmentReference colorRef = {};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        
        VkRenderPassCreateInfo rpInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rpInfo.attachmentCount = 1;
        rpInfo.pAttachments = &colorAttachment;
        rpInfo.subpassCount = 1;
        rpInfo.pSubpasses = &subpass;
        
        return vkCreateRenderPass(device, &rpInfo, nullptr, &renderPass) == VK_SUCCESS;
    }

    bool createFramebuffers() {
        framebuffers.resize(swapchainImageViews.size());
        
        for (size_t i = 0; i < swapchainImageViews.size(); i++) {
            VkFramebufferCreateInfo fbInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            fbInfo.renderPass = renderPass;
            fbInfo.attachmentCount = 1;
            fbInfo.pAttachments = &swapchainImageViews[i];
            fbInfo.width = width;
            fbInfo.height = height;
            fbInfo.layers = 1;
            
            if (vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
                return false;
        }
        return true;
    }

    bool createCommandPool() {
        VkCommandPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
        
        return vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) == VK_SUCCESS;
    }

    bool createCommandBuffers() {
        commandBuffers.resize(swapchainImages.size());
        
        VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();
        
        return vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) == VK_SUCCESS;
    }

    bool createSyncObjects() {
        VkSemaphoreCreateInfo semInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        
        return vkCreateSemaphore(device, &semInfo, nullptr, &imageAvailable) == VK_SUCCESS &&
               vkCreateSemaphore(device, &semInfo, nullptr, &renderFinished) == VK_SUCCESS &&
               vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) == VK_SUCCESS;
    }
};
