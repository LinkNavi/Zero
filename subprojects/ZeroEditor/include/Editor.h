#pragma once
#include "ZeroEngine.h"
#include "Renderer.h"
#include "Input.h"
#include "Time.h"
#include "Config.h"
#include "ResourcePath.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <string>
#include <vector>
#include <functional>

// Editor play state
enum class EditorPlayState { Edit, Play, Pause };

// Gizmo mode
enum class GizmoMode { Translate, Rotate, Scale };

// Editor panel visibility
struct PanelVisibility {
    bool hierarchy = true;
    bool inspector = true;
    bool viewport = true;
    bool console = true;
    bool assets = true;
    bool toolbar = true;
    bool sceneSettings = false;
};

// Console log entry
struct LogEntry {
    enum Level { Info, Warning, Error };
    Level level;
    std::string message;
    float timestamp;
};

class Editor {
public:
    bool init(uint32_t width, uint32_t height);
    void run();
    void shutdown();

private:
    // Core systems
    VulkanRenderer renderer;
    ZeroEngine engine;
    VkDescriptorPool editorDescPool = VK_NULL_HANDLE;

    // Viewport texture displayed via ImGui
    VkDescriptorSet viewportDescSet = VK_NULL_HANDLE;
    uint32_t viewportWidth = 0, viewportHeight = 0;
    bool viewportFocused = false;
    bool viewportHovered = false;

    // Editor state
    EditorPlayState playState = EditorPlayState::Edit;
    GizmoMode gizmoMode = GizmoMode::Translate;
    PanelVisibility panels;
    EntityID selectedEntity = INVALID_ENTITY;
    std::string currentScenePath;
    bool unsavedChanges = false;
    bool showFileDialog = false;
    bool fileDialogSave = false;
    std::string fileDialogPath;

    // Console
    std::vector<LogEntry> consoleLogs;
    bool consoleAutoScroll = true;
    int consoleFilter = -1; // -1 = all

    // Styling
    ImFont* fontRegular = nullptr;
    ImFont* fontBold = nullptr;
    ImFont* fontSmall = nullptr;
    ImFont* fontIcons = nullptr;

    // Timing
    float deltaTime = 0.0f;
    int frameCount = 0;
    float fpsTimer = 0.0f;
    int fps = 0;
    int engineFrameCount = 0;

    // Methods
    void setupStyle();
    void setupFonts();
    void initImGui();
    void createEditorDescriptorPool();
    void updateViewportDescriptor();

    // UI Drawing
    void drawUI(VkCommandBuffer cmd);
    void drawMenuWindow();
    void drawMenuBar();
    void drawToolbar(float x, float y, float w, float h);
    void drawHierarchyContent();
    void drawInspectorContent();
    void drawViewportContent();
    void drawConsoleContent();
    void drawAssetBrowserContent();
    void drawSceneSettings();
    void drawFileDialog();
    void drawStatusBar(float x, float y, float w, float h);

    // Actions
    void newScene();
    void openScene();
    void saveScene();
    void saveSceneAs();
    void playScene();
    void pauseScene();
    void stopScene();
    void duplicateEntity();
    void deleteSelectedEntity();

    // Helpers
    void log(const std::string& msg, LogEntry::Level level = LogEntry::Info);
    ImVec4 getPlayStateColor() const;
    const char* getPlayStateText() const;
};
