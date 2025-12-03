#include "Renderer.h"
#include <fstream>
#include <iostream>
#include <set>
#include <algorithm>
#include <cstring>

// Default shaders embedded as SPIR-V
// Vertex shader
static const std::vector<char> vertShaderCode = {
    // This would be compiled SPIR-V. For brevity, showing as comment
    // You need to compile shaders with glslc or include as binary
    // See shader code at the end of this file
};

static const std::vector<char> fragShaderCode = {
    // Compiled fragment shader SPIR-V
};

Renderer::Renderer() {}

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Init(GLFWwindow* win) {
    window = win;
    
    if (!window) {
        std::cerr << "Window is null!" << std::endl;
        return false;
    }
    
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    windowWidth = width;
    windowHeight = height;
    
    // Initialize Vulkan
    if (!createInstance()) return false;
    
    // Create window surface
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        std::cerr << "Failed to create window surface!" << std::endl;
        return false;
    }
    
    if (!pickPhysicalDevice()) return false;
    if (!createLogicalDevice()) return false;
    if (!createSwapChain()) return false;
    if (!createImageViews()) return false;
    if (!createRenderPass()) return false;
    if (!createDescriptorSetLayout()) return false;
    if (!createGraphicsPipeline()) return false;
    if (!createCommandPool()) return false;
    if (!createDepthResources()) return false;
    if (!createFramebuffers()) return false;
    if (!createUniformBuffers()) return false;
    if (!createDescriptorPool()) return false;
    if (!createDescriptorSets()) return false;
    if (!createCommandBuffers()) return false;
    if (!createSyncObjects()) return false;
    
    // Setup default camera
    viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), 
                            glm::vec3(0.0f, 0.0f, 0.0f), 
                            glm::vec3(0.0f, 1.0f, 0.0f));
    projectionMatrix = glm::perspective(glm::radians(45.0f), 
                                       (float)windowWidth / (float)windowHeight, 
                                       0.1f, 100.0f);
    projectionMatrix[1][1] *= -1; // GLM was designed for OpenGL, flip Y for Vulkan
    
    std::cout << "Vulkan Renderer initialized successfully!" << std::endl;
    return true;
}

void Renderer::Shutdown() {
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
    }
    
    cleanupSwapChain();
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (uniformBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }
    }
    
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }
    
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    }
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        }
        if (imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        }
        if (inFlightFences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }
    }
    
    if (commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, commandPool, nullptr);
    }
    
    if (device != VK_NULL_HANDLE) {
        vkDestroyDevice(device, nullptr);
    }
    
    if (surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }
    
    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
    }
}

void Renderer::SetViewport(int width, int height) {
    framebufferResized = true;
    windowWidth = width;
    windowHeight = height;
}

void Renderer::BeginFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                                           imageAvailableSemaphores[currentFrame],
                                           VK_NULL_HANDLE, &imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cerr << "Failed to acquire swap chain image!" << std::endl;
        return;
    }
    
    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    
    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    if (vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
        std::cerr << "Failed to begin recording command buffer!" << std::endl;
        return;
    }
    
    // Begin render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;
    
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    
    vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo, 
                        VK_SUBPASS_CONTENTS_INLINE);
    
    frameInProgress = true;
}

void Renderer::EndFrame() {
    if (!frameInProgress) return;
    
    vkCmdEndRenderPass(commandBuffers[currentFrame]);
    
    if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS) {
        std::cerr << "Failed to record command buffer!" << std::endl;
        frameInProgress = false;
        return;
    }
    
    // Submit command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
    
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        std::cerr << "Failed to submit draw command buffer!" << std::endl;
        frameInProgress = false;
        return;
    }
    
    // Present
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    
    VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        std::cerr << "Failed to present swap chain image!" << std::endl;
    }
    
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    frameInProgress = false;
}

Mesh Renderer::CreateCube() {
    std::vector<Vertex> vertices = {
        // Front face
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}},
        // Back face
        {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}}
    };
    
    std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0,  // Front
        4, 5, 6, 6, 7, 4,  // Back
        5, 0, 3, 3, 6, 5,  // Left
        1, 4, 7, 7, 2, 1,  // Right
        3, 2, 7, 7, 6, 3,  // Top
        5, 4, 1, 1, 0, 5   // Bottom
    };
    
    return CreateMesh(vertices, indices);
}

Mesh Renderer::CreateMesh(const std::vector<Vertex>& vertices, 
                         const std::vector<uint32_t>& indices) {
    Mesh mesh{};
    mesh.indexCount = static_cast<uint32_t>(indices.size());
    mesh.isValid = false;
    
    // Create vertex buffer
    VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferMemory);
    
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, vertexBufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)vertexBufferSize);
    vkUnmapMemory(device, stagingBufferMemory);
    
    createBuffer(vertexBufferSize, 
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                mesh.vertexBuffer, mesh.vertexBufferMemory);
    
    copyBuffer(stagingBuffer, mesh.vertexBuffer, vertexBufferSize);
    
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
    
    // Create index buffer
    VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
    
    createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferMemory);
    
    vkMapMemory(device, stagingBufferMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)indexBufferSize);
    vkUnmapMemory(device, stagingBufferMemory);
    
    createBuffer(indexBufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                mesh.indexBuffer, mesh.indexBufferMemory);
    
    copyBuffer(stagingBuffer, mesh.indexBuffer, indexBufferSize);
    
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
    
    mesh.isValid = true;
    return mesh;
}

void Renderer::DestroyMesh(Mesh& mesh) {
    if (!mesh.isValid) return;
    
    vkDestroyBuffer(device, mesh.indexBuffer, nullptr);
    vkFreeMemory(device, mesh.indexBufferMemory, nullptr);
    vkDestroyBuffer(device, mesh.vertexBuffer, nullptr);
    vkFreeMemory(device, mesh.vertexBufferMemory, nullptr);
    
    mesh.isValid = false;
}

Shader Renderer::CreateDefaultShader() {
    return graphicsPipeline;
}

void Renderer::DestroyShader(Shader shader) {
    // For now, we only have one pipeline
    // In a full implementation, you'd track and destroy custom pipelines
}

void Renderer::DrawMesh(const Mesh& mesh, Shader shader, const glm::mat4& transform) {
    if (!frameInProgress || !mesh.isValid) return;
    
    // Update uniform buffer
    UniformBufferObject ubo{};
    ubo.model = transform;
    ubo.view = viewMatrix;
    ubo.proj = projectionMatrix;
    
    memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
    
    // Bind pipeline
    vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, shader);
    
    // Bind vertex buffer
    VkBuffer vertexBuffers[] = {mesh.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, vertexBuffers, offsets);
    
    // Bind index buffer
    vkCmdBindIndexBuffer(commandBuffers[currentFrame], mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    
    // Bind descriptor sets
    vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
                           pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
    
    // Draw
    vkCmdDrawIndexed(commandBuffers[currentFrame], mesh.indexCount, 1, 0, 0, 0);
}

void Renderer::SetViewMatrix(const glm::mat4& view) {
    viewMatrix = view;
}

void Renderer::SetProjectionMatrix(const glm::mat4& proj) {
    projectionMatrix = proj;
}

// ... (Implementation continues with all the Vulkan setup functions)
// Due to length, I'll include the key setup functions

bool Renderer::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "ZeroEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "ZeroEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }
    
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance!" << std::endl;
        return false;
    }
    
    return true;
}

// ... Additional implementation functions would continue here
// This is a comprehensive starting point!

/*
VERTEX SHADER (shader.vert):
#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}

FRAGMENT SHADER (shader.frag):
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}

Compile with:
glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv
*/
