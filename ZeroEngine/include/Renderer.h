#ifndef RENDERER_H
#define RENDERER_H

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <array>
#include <memory>
#include <string>

// Vertex structure
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }
    
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
        
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);
        
        return attributeDescriptions;
    }
};

// Uniform buffer object for MVP matrix
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

// Simple mesh handle
struct Mesh {
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    uint32_t indexCount;
    bool isValid;
};

// Material/Shader handle
using Shader = VkPipeline;

// Renderer class - Unity-like interface with Vulkan backend
class Renderer {
public:
    Renderer();
    ~Renderer();
    
    // Initialize the renderer with a GLFW window
    bool Init(GLFWwindow* window);
    void Shutdown();
    
    // Viewport management
    void SetViewport(int width, int height);
    
    // Frame management
    void BeginFrame();
    void EndFrame();
    bool IsFrameInProgress() const { return frameInProgress; }
    
    // Mesh creation
    Mesh CreateCube();
    Mesh CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    void DestroyMesh(Mesh& mesh);
    
    // Shader/Pipeline management
    Shader CreateDefaultShader();
    void DestroyShader(Shader shader);
    
    // Drawing - Unity-like interface
    void DrawMesh(const Mesh& mesh, Shader shader, const glm::mat4& transform);
    
    // Camera setup
    void SetViewMatrix(const glm::mat4& view);
    void SetProjectionMatrix(const glm::mat4& proj);
    
    // Helper to get current frame dimensions
    VkExtent2D GetSwapChainExtent() const { return swapChainExtent; }

private:
    // Initialization helpers
    bool createInstance();
    bool pickPhysicalDevice();
    bool createLogicalDevice();
    bool createSwapChain();
    bool createImageViews();
    bool createRenderPass();
    bool createDescriptorSetLayout();
    bool createGraphicsPipeline();
    bool createFramebuffers();
    bool createCommandPool();
    bool createDepthResources();
    bool createUniformBuffers();
    bool createDescriptorPool();
    bool createDescriptorSets();
    bool createCommandBuffers();
    bool createSyncObjects();
    
    // Helper functions
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    bool isDeviceSuitable(VkPhysicalDevice device);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
                     VkMemoryPropertyFlags properties, VkBuffer& buffer, 
                     VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    VkFormat findDepthFormat();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, 
                                VkImageTiling tiling, VkFormatFeatureFlags features);
    void createImage(uint32_t width, uint32_t height, VkFormat format, 
                    VkImageTiling tiling, VkImageUsageFlags usage, 
                    VkMemoryPropertyFlags properties, VkImage& image, 
                    VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    
    // Cleanup
    void cleanupSwapChain();
    void recreateSwapChain();
    
    // Window
    GLFWwindow* window = nullptr;
    int windowWidth = 800;
    int windowHeight = 600;
    bool framebufferResized = false;
    
    // Vulkan core
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    
    // Swap chain
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    
    // Surface
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    
    // Pipeline
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    
    // Command buffers
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    
    // Depth buffer
    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;
    
    // Uniform buffers
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    
    // Descriptor pool and sets
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;
    
    // Synchronization
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
    static const int MAX_FRAMES_IN_FLIGHT = 2;
    
    // Frame state
    bool frameInProgress = false;
    uint32_t imageIndex = 0;
    
    // Camera matrices
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    
    // Queue family indices
    struct QueueFamilyIndices {
        int graphicsFamily = -1;
        int presentFamily = -1;
        bool isComplete() const {
            return graphicsFamily >= 0 && presentFamily >= 0;
        }
    };
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    
    // Swap chain support details
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    
    // Validation layers
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif
};

#endif // RENDERER_H
