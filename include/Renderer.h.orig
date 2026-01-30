#pragma once
#define GLFW_INCLUDE_VULKAN
#include <array>
#include <vulkan/vulkan.h>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include <cstring>

// FIXED: Triple buffering eliminates stutters with VSync
constexpr int MAX_FRAMES_IN_FLIGHT = 3;

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
    
    // Depth buffer
    AllocatedImage depthImage;
    VkFormat depthFormat;
    
    // TRIPLE buffering for smooth VSync performance
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores;
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores;
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> inFlightFences;
    
    uint32_t width, height;
    uint32_t currentFrame = 0;
    uint32_t imageIndex = 0;

public:
    bool init(uint32_t w, uint32_t h, const char* title) {
        width = w;
        height = h;
        
        std::cout << "  - Initializing GLFW..." << std::endl;
        
        glfwSetErrorCallback([](int error, const char* description) {
            std::cerr << "GLFW Error " << error << ": " << description << std::endl;
        });
        
        if (!glfwInit()) {
            std::cerr << "ERROR: Failed to initialize GLFW!" << std::endl;
            return false;
        }
        
        if (!glfwVulkanSupported()) {
            std::cerr << "ERROR: GLFW reports Vulkan is not supported!" << std::endl;
            glfwTerminate();
            return false;
        }
        std::cout << "  ✓ GLFW Vulkan support confirmed" << std::endl;
        
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        
        if (!window) {
            std::cerr << "ERROR: Failed to create GLFW window!" << std::endl;
            glfwTerminate();
            return false;
        }
        std::cout << "  ✓ GLFW window created" << std::endl;
        
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        
        if (glfwExtensions == nullptr || glfwExtensionCount == 0) {
            std::cerr << "ERROR: Failed to get required Vulkan extensions from GLFW!" << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            return false;
        }
        
        std::cout << "  - Creating Vulkan instance..." << std::endl;
        vkb::InstanceBuilder builder;
        
        for (uint32_t i = 0; i < glfwExtensionCount; i++) {
            builder.enable_extension(glfwExtensions[i]);
        }
        
        auto instRet = builder
            .set_app_name(title)
            .set_engine_name("Zero Engine")
            .request_validation_layers(false)
            .require_api_version(1, 0, 0)
            .build();
        
        if (!instRet) {
            std::cerr << "ERROR: Failed to create Vulkan instance!" << std::endl;
            std::cerr << "Reason: " << instRet.error().message() << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            return false;
        }
        vkbInstance = instRet.value();
        instance = vkbInstance.instance;
        std::cout << "  ✓ Vulkan instance created" << std::endl;
        
        std::cout << "  - Creating window surface..." << std::endl;
        VkResult surfaceResult = glfwCreateWindowSurface(instance, window, nullptr, &surface);
        if (surfaceResult != VK_SUCCESS) {
            std::cerr << "ERROR: Failed to create window surface! Code: " << surfaceResult << std::endl;
            return false;
        }
        std::cout << "  ✓ Window surface created" << std::endl;
        
        std::cout << "  - Selecting physical device..." << std::endl;
        vkb::PhysicalDeviceSelector selector{vkbInstance};
        auto physRet = selector
            .set_surface(surface)
            .set_minimum_version(1, 0)
            .select();
        
        if (!physRet) {
            std::cerr << "ERROR: Failed to select physical device!" << std::endl;
            std::cerr << "Reason: " << physRet.error().message() << std::endl;
            return false;
        }
        vkb::PhysicalDevice vkbPhysicalDevice = physRet.value();
        std::cout << "  ✓ Physical device: " << vkbPhysicalDevice.name << std::endl;
        
        std::cout << "  - Creating logical device..." << std::endl;
        vkb::DeviceBuilder deviceBuilder{vkbPhysicalDevice};
        auto devRet = deviceBuilder.build();
        if (!devRet) {
            std::cerr << "ERROR: Failed to create logical device!" << std::endl;
            std::cerr << "Reason: " << devRet.error().message() << std::endl;
            return false;
        }
        
        vkbDevice = devRet.value();
        device = vkbDevice.device;
        physicalDevice = vkbPhysicalDevice.physical_device;
        std::cout << "  ✓ Logical device created" << std::endl;
        
        auto gqRet = vkbDevice.get_queue(vkb::QueueType::graphics);
        if (!gqRet) return false;
        graphicsQueue = gqRet.value();
        
        auto pqRet = vkbDevice.get_queue(vkb::QueueType::present);
        if (!pqRet) return false;
        presentQueue = pqRet.value();
        std::cout << "  ✓ Queues acquired" << std::endl;
        
        std::cout << "  - Initializing VMA allocator..." << std::endl;
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;
        if (vmaCreateAllocator(&allocatorInfo, &allocator) != VK_SUCCESS) {
            std::cerr << "ERROR: Failed to create VMA allocator!" << std::endl;
            return false;
        }
        std::cout << "  ✓ VMA allocator created" << std::endl;
        
        depthFormat = findDepthFormat();
        std::cout << "  ✓ Depth format selected" << std::endl;
        
        std::cout << "  - Creating swapchain..." << std::endl;
        if (!createSwapchain()) return false;
        std::cout << "  ✓ Swapchain created" << std::endl;
        
        std::cout << "  - Creating depth buffer..." << std::endl;
        if (!createDepthResources()) return false;
        std::cout << "  ✓ Depth buffer created" << std::endl;
        
        std::cout << "  - Creating render pass..." << std::endl;
        if (!createRenderPass()) return false;
        std::cout << "  ✓ Render pass created" << std::endl;
        
        std::cout << "  - Creating framebuffers..." << std::endl;
        if (!createFramebuffers()) return false;
        std::cout << "  ✓ Framebuffers created (" << framebuffers.size() << ")" << std::endl;
        
        std::cout << "  - Creating command pool..." << std::endl;
        if (!createCommandPool()) return false;
        std::cout << "  ✓ Command pool created" << std::endl;
        
        std::cout << "  - Creating command buffers..." << std::endl;
        if (!createCommandBuffers()) return false;
        std::cout << "  ✓ Command buffers created" << std::endl;
        
        std::cout << "  - Creating sync objects..." << std::endl;
        if (!createSyncObjects()) return false;
        std::cout << "  ✓ Sync objects created (TRIPLE buffered - stutter fix)" << std::endl;
        
        return true;
    }

    void beginFrame(VkCommandBuffer& cmd) {
        // With triple buffering, we wait for frame N-3 to complete
        // This gives GPU plenty of time to work without stalling
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        
        // Acquire next image - may return immediately or wait for VSync
        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, 
            imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            std::cerr << "Failed to acquire swapchain image!" << std::endl;
            return;
        }
        
        // Reset fence only after we know which frame we're working on
        vkResetFences(device, 1, &inFlightFences[currentFrame]);
        
        cmd = commandBuffers[imageIndex];
        
        // Reset and re-record command buffer (fixes smearing bug)
        vkResetCommandBuffer(cmd, 0);
        
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        vkBeginCommandBuffer(cmd, &beginInfo);
        
        VkRenderPassBeginInfo rpInfo = {};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass = renderPass;
        rpInfo.framebuffer = framebuffers[imageIndex];
        rpInfo.renderArea.extent = {width, height};
        
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        
        rpInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        rpInfo.pClearValues = clearValues.data();
        
        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void endFrame(VkCommandBuffer cmd) {
        vkCmdEndRenderPass(cmd);
        vkEndCommandBuffer(cmd);
        
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &imageAvailableSemaphores[currentFrame];
        submitInfo.pWaitDstStageMask = &waitStage;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrame];
        
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            std::cerr << "Failed to submit draw command buffer!" << std::endl;
        }
        
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &imageIndex;
        
        vkQueuePresentKHR(presentQueue, &presentInfo);
        
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    bool shouldClose() { return glfwWindowShouldClose(window); }
    void pollEvents() { glfwPollEvents(); }
    
    VkDevice getDevice() { return device; }
    VmaAllocator getAllocator() { return allocator; }
    GLFWwindow* getWindow() { return window; }
    VkRenderPass getRenderPass() { return renderPass; }
    VkCommandPool getCommandPool() { return commandPool; }
    VkQueue getGraphicsQueue() { return graphicsQueue; }
    VkPhysicalDevice getPhysicalDevice() { return physicalDevice; }

    void cleanup() {
        vkDeviceWaitIdle(device);
        
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }
        
        vkDestroyCommandPool(device, commandPool, nullptr);
        
        for (auto fb : framebuffers)
            vkDestroyFramebuffer(device, fb, nullptr);
        
        vkDestroyImageView(device, depthImage.view, nullptr);
        vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);
        
        vkDestroyRenderPass(device, renderPass, nullptr);
        
        for (auto view : swapchainImageViews)
            vkDestroyImageView(device, view, nullptr);
        
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        vmaDestroyAllocator(allocator);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
        
        glfwDestroyWindow(window);
        glfwTerminate();
    }

private:
    VkFormat findDepthFormat() {
        std::vector<VkFormat> candidates = {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };
        
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
            
            if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                return format;
            }
        }
        
        return VK_FORMAT_D32_SFLOAT;
    }
    
    bool createDepthResources() {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        
        if (vmaCreateImage(allocator, &imageInfo, &allocInfo, &depthImage.image, &depthImage.allocation, nullptr) != VK_SUCCESS) {
            std::cerr << "Failed to create depth image!" << std::endl;
            return false;
        }
        
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthImage.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        
        if (vkCreateImageView(device, &viewInfo, nullptr, &depthImage.view) != VK_SUCCESS) {
            std::cerr << "Failed to create depth image view!" << std::endl;
            return false;
        }
        
        return true;
    }

    bool createSwapchain() {
        vkb::SwapchainBuilder swapchainBuilder{vkbDevice};
        auto swapRet = swapchainBuilder
            .set_desired_format({VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(width, height)
            .build();
        
        if (!swapRet) {
            std::cerr << "ERROR: Failed to create swapchain!" << std::endl;
            std::cerr << "Reason: " << swapRet.error().message() << std::endl;
            return false;
        }
        
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
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        
        VkAttachmentReference colorRef = {};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
        VkAttachmentReference depthRef{};
        depthRef.attachment = 1;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pDepthStencilAttachment = &depthRef;
        
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
        std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
        
        VkRenderPassCreateInfo rpInfo = {};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        rpInfo.pAttachments = attachments.data();
        rpInfo.subpassCount = 1;
        rpInfo.pSubpasses = &subpass;
        rpInfo.dependencyCount = 1;
        rpInfo.pDependencies = &dependency;
        
        return vkCreateRenderPass(device, &rpInfo, nullptr, &renderPass) == VK_SUCCESS;
    }

    bool createFramebuffers() {
        framebuffers.resize(swapchainImageViews.size());
        
        for (size_t i = 0; i < swapchainImageViews.size(); i++) {
            std::array<VkImageView, 2> attachments = {
                swapchainImageViews[i],
                depthImage.view
            };
            
            VkFramebufferCreateInfo fbInfo = {};
            fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbInfo.renderPass = renderPass;
            fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            fbInfo.pAttachments = attachments.data();
            fbInfo.width = width;
            fbInfo.height = height;
            fbInfo.layers = 1;
            
            if (vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
                return false;
        }
        return true;
    }

    bool createCommandPool() {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
        
        return vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) == VK_SUCCESS;
    }

    bool createCommandBuffers() {
        commandBuffers.resize(swapchainImages.size());
        
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();
        
        return vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) == VK_SUCCESS;
    }

    bool createSyncObjects() {
        VkSemaphoreCreateInfo semInfo{};
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(device, &semInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                return false;
            }
        }

        return true;
    }
};
