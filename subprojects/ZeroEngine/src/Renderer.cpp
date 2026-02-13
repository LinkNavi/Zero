#include "Renderer.h"
#include "VkBootstrap.h"
#include <iostream>

bool VulkanRenderer::init(uint32_t w, uint32_t h, const char* title) {
    width = w;
    height = h;
    windowWidth = w;
    windowHeight = h;
    
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    
    vkb::InstanceBuilder builder;
    builder.enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    auto instRet = builder.set_app_name(title)
           .set_engine_name("Zero")
           .request_validation_layers(true)
           .require_api_version(1, 3, 0)
           .build();
    
    if (!instRet) {
        std::cerr << "Failed to create Vulkan instance\n";
        return false;
    }
    vkbInstance = instRet.value();
    instance = vkbInstance.instance;
    
    VkSurfaceKHR surf;
    glfwCreateWindowSurface(instance, window, nullptr, &surf);
    surface = surf;
    
    vkb::PhysicalDeviceSelector selector{vkbInstance};
    auto physRet = selector.set_surface(surface)
                          .set_minimum_version(1, 3)
                          .select();
    if (!physRet) return false;
    
    vkb::PhysicalDevice vkbPhysDev = physRet.value();
    physicalDevice = vkbPhysDev.physical_device;
    
    vkb::DeviceBuilder devBuilder{vkbPhysDev};
    auto devRet = devBuilder.build();
    if (!devRet) return false;
    vkbDevice = devRet.value();
    device = vkbDevice.device;
    
    auto gfxRet = vkbDevice.get_queue(vkb::QueueType::graphics);
    if (!gfxRet) return false;
    graphicsQueue = gfxRet.value();
    
    auto presRet = vkbDevice.get_queue(vkb::QueueType::present);
    if (!presRet) return false;
    presentQueue = presRet.value();
    
    VmaAllocatorCreateInfo allocInfo{};
    allocInfo.instance = instance;
    allocInfo.physicalDevice = physicalDevice;
    allocInfo.device = device;
    allocInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    vmaCreateAllocator(&allocInfo, &allocator);
    
    depthFormat = findDepthFormat();
    
    if (!createSwapchain()) return false;
    if (!createDepthResources()) return false;
    if (!createRenderPass()) return false;
    if (!createFramebuffers()) return false;
    if (!createCommandPool()) return false;
    if (!createCommandBuffers()) return false;
    if (!createSyncObjects()) return false;
    
    return true;
}

bool VulkanRenderer::createSwapchain() {
    vkb::SwapchainBuilder swapchainBuilder{vkbDevice};
    
    VkSurfaceFormatKHR fmt{VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    
    auto swapRet = swapchainBuilder
        .set_desired_format(fmt)
        .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
        .add_fallback_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .add_fallback_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
        .set_desired_extent(width, height)
        .set_desired_min_image_count(3)
        .build();
    
    if (!swapRet) return false;
    vkbSwapchain = swapRet.value();
    swapchain = vkbSwapchain.swapchain;
    
    auto imgRet = vkbSwapchain.get_images();
    auto viewRet = vkbSwapchain.get_image_views();
    if (!imgRet || !viewRet) return false;
    
    swapchainImages = imgRet.value();
    swapchainImageViews = viewRet.value();
    
    return true;
}

bool VulkanRenderer::createCommandPool() {
    auto queueRet = vkbDevice.get_queue_index(vkb::QueueType::graphics);
    if (!queueRet) return false;
    
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueRet.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    return vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) == VK_SUCCESS;
}
