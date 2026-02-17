#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <functional>

// Forward declarations
class VulkanRenderer;
class Pipeline;
class ModelLoader;
class ShadowMap;
class Skybox;
class Camera;
class PostProcessing;
class ECS;
struct BoneBuffer;
struct Model;
struct Transform;

// ============================================================
// ZeroEngine — embeddable engine interface
//
// Usage (standalone runtime):
//   ZeroEngine engine;
//   engine.init({.windowTitle = "My Game", .width = 1920, .height = 1080});
//   while (engine.isRunning()) { engine.update(); }
//   engine.shutdown();
//
// Usage (embedded in editor):
//   ZeroEngine engine;
//   engine.init({.mode = EngineMode::Embedded, .device = editorDevice, ...});
//   engine.resize(viewportW, viewportH);
//   engine.update();
//   VkImageView view = engine.getOutputImageView(); // display in ImGui
//   engine.shutdown();
// ============================================================

enum class EngineMode {
    Standalone,  // Engine owns window, swapchain, presents to screen
    Embedded     // Engine renders to offscreen texture, caller owns window
};

enum class PlayState {
    Editing,     // Scene is editable, no game logic runs
    Playing,     // Game logic runs, scene is live
    Paused       // Game logic paused, scene frozen
};

struct EngineConfig {
    EngineMode mode = EngineMode::Standalone;
    
    // Standalone mode
    std::string windowTitle = "Zero Runtime";
    uint32_t width = 1920;
    uint32_t height = 1080;
    
    // Embedded mode — caller provides Vulkan context
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VmaAllocator allocator = nullptr;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily = 0;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    
    // Shared settings
    std::string resourceRoot = "";  // empty = auto-detect
    bool enablePostProcess = true;
    bool enableShadows = true;
    bool enableSkybox = true;
    bool enableValidation = true;
};

// Per-frame output from the engine
struct EngineFrame {
    VkImage outputImage = VK_NULL_HANDLE;
    VkImageView outputImageView = VK_NULL_HANDLE;
    VkImageLayout outputLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    uint32_t width = 0;
    uint32_t height = 0;
};

// Entity handle (opaque to editor, maps to ECS entity)
using EntityID = uint32_t;
static constexpr EntityID INVALID_ENTITY = UINT32_MAX;

// Lightweight scene entity info for editor
struct EntityInfo {
    EntityID id = INVALID_ENTITY;
    std::string name;
    glm::vec3 position{0};
    glm::vec3 rotation{0};
    glm::vec3 scale{1};
    bool hasModel = false;
    std::string modelPath;
    bool isCamera = false;
    bool isActiveCamera = false;
    EntityID parent = 0;  // Add this
};

class ZeroEngine {
public:
    ZeroEngine();
    ~ZeroEngine();
    
    // No copy
    ZeroEngine(const ZeroEngine&) = delete;
    ZeroEngine& operator=(const ZeroEngine&) = delete;
    
    // ==================== Lifecycle ====================
    bool init(const EngineConfig& config);
    void shutdown();
    
    // Returns false when window closed (standalone) or after shutdown
    bool isRunning() const;
    
    // Main update: polls input, updates systems, renders frame
    // In embedded mode, renders to offscreen target
    void update(float dt = -1.0f);  // dt < 0 = auto-calculate
    
    // ==================== Rendering ====================
    
    // Resize the render target (embedded mode) or window (standalone)
    void resize(uint32_t width, uint32_t height);
    
    // Get the rendered frame (embedded mode)
    // Returns the offscreen image that the editor can sample in ImGui
    EngineFrame getOutputFrame() const;
    
    // Get a sampler suitable for displaying the output in ImGui
    VkSampler getOutputSampler() const;
    
    // ==================== Scene ====================
    
    bool loadScene(const std::string& path);
    bool saveScene(const std::string& path);
    void newScene();
    
    // ==================== Entity Management ====================
    
    EntityID createEntity(const std::string& name = "Entity");
    void destroyEntity(EntityID id);
    // Entity hierarchy operations
void setEntityName(EntityID id, const std::string& name);
void setEntityParent(EntityID id, EntityID parentId);
void removeEntityParent(EntityID id);
EntityID getEntityParent(EntityID id) const;
std::vector<EntityID> getEntityChildren(EntityID id) const;
    // Get all entities in scene
    std::vector<EntityInfo> getEntities() const;
    
    // Get/set entity transform
    EntityInfo getEntityInfo(EntityID id) const;
    void setEntityPosition(EntityID id, glm::vec3 pos);
    void setEntityRotation(EntityID id, glm::vec3 eulerDeg);
    void setEntityScale(EntityID id, glm::vec3 scale);
    
    // Model
    bool setEntityModel(EntityID id, const std::string& modelPath);
    void removeEntityModel(EntityID id);
    
    // Camera
    void setEntityAsCamera(EntityID id, bool active = true);
    void setActiveCamera(EntityID id);
    EntityID getActiveCamera() const;
    
    // ==================== Play Mode ====================
    
    PlayState getPlayState() const;
    void play();   // Snapshot scene, start game logic
    void pause();
    void stop();   // Restore scene snapshot
    
    // ==================== Camera Control (Editor) ====================
    
    // Editor camera (separate from game cameras)
    void setEditorCameraPosition(glm::vec3 pos);
    void setEditorCameraTarget(glm::vec3 target);
    glm::vec3 getEditorCameraPosition() const;
    
    // ==================== Settings ====================
    
    void setPostProcessEnabled(bool enabled);
    void setShadowsEnabled(bool enabled);
    void setSkyboxEnabled(bool enabled);
    void setExposure(float exposure);
    void setGamma(float gamma);
    
    // Light settings
    void setDirectionalLight(glm::vec3 direction, glm::vec3 color, float ambient);
    
    // ==================== Vulkan Access (for editor integration) ====================
    
    VkDevice getDevice() const;
    VmaAllocator getAllocator() const;
    VkDescriptorPool getDescriptorPool() const;
    
private:
    struct Impl;
    Impl* impl = nullptr;
};
