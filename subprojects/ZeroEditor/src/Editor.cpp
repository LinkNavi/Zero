#include "Editor.h"
#include <iostream>
#include <chrono>
#include <filesystem>

bool Editor::init(uint32_t width, uint32_t height) {
    ResourcePath::init();

    // Init renderer (owns window + swapchain)
    if (!renderer.init(width, height, "Zero Editor")) {
        std::cerr << "Failed to init renderer\n";
        return false;
    }

    renderer.setupResizeCallback();

    Input::init(renderer.getWindow());
    Time::init();

    // Create editor descriptor pool (for ImGui + viewport texture)
    createEditorDescriptorPool();

    // Init ImGui ourselves so we control the descriptor pool
    initImGui();
    setupStyle();
    setupFonts();

    // Init engine in embedded mode - renders to offscreen texture
    viewportWidth = width / 2;
    viewportHeight = height / 2;

    EngineConfig cfg;
    cfg.mode = EngineMode::Embedded;
    cfg.device = renderer.getDevice();
    cfg.physicalDevice = renderer.getPhysicalDevice();
    cfg.allocator = renderer.getAllocator();
    cfg.graphicsQueue = renderer.getGraphicsQueue();
    cfg.graphicsQueueFamily = renderer.getGraphicsQueueFamily();
    cfg.commandPool = renderer.getCommandPool();
    cfg.descriptorPool = editorDescPool;
    cfg.width = viewportWidth;
    cfg.height = viewportHeight;
    cfg.enableShadows = true;
    cfg.enableSkybox = false;
    cfg.enablePostProcess = false;

    if (!engine.init(cfg)) {
        std::cerr << "Failed to init engine\n";
        return false;
    }

    // Create default scene
    newScene();

    log("Zero Editor initialized", LogEntry::Info);
    log("Use File > Open to load a .zscene file", LogEntry::Info);
    log("Right-click viewport to enable camera controls", LogEntry::Info);

    return true;
}

void Editor::run() {
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (!renderer.shouldClose()) {
        renderer.pollEvents();

        // Skip rendering when minimized
        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(renderer.getWindow(), &fbW, &fbH);
        if (fbW == 0 || fbH == 0) {
            glfwWaitEvents();
            continue;
        }

        auto now = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        // FPS counter
        fpsTimer += deltaTime;
        frameCount++;
        if (fpsTimer >= 1.0f) {
            fps = frameCount;
            frameCount = 0;
            fpsTimer = 0.0f;
        }

        // Update engine (renders to offscreen)
        if (playState == EditorPlayState::Play) {
            engine.update(deltaTime);
        } else {
            engine.update(0.0f);
        }

        // Wait for engine GPU work to complete
        vkDeviceWaitIdle(renderer.getDevice());

        // Skip descriptor update for first few frames
        if (engineFrameCount > 5) {
            updateViewportDescriptor();
        }
        engineFrameCount++;

        // Render editor UI
        VkCommandBuffer cmd;
        if (!renderer.beginFrame(cmd)) continue;  // skip frame on resize

        VkRenderPassBeginInfo rpInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rpInfo.renderPass = renderer.getRenderPass();
        rpInfo.framebuffer = renderer.getCurrentFramebuffer();
        rpInfo.renderArea = {{0, 0}, {renderer.getWidth(), renderer.getHeight()}};

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.11f, 0.11f, 0.14f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};
        rpInfo.clearValueCount = 2;
        rpInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport vp{0, 0, float(renderer.getWidth()), float(renderer.getHeight()), 0, 1};
        vkCmdSetViewport(cmd, 0, 1, &vp);
        VkRect2D sc{{0, 0}, {renderer.getWidth(), renderer.getHeight()}};
        vkCmdSetScissor(cmd, 0, 1, &sc);

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        drawUI(cmd);
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

        vkCmdEndRenderPass(cmd);
        renderer.endFrame(cmd);

        Input::update();
    }
}

void Editor::shutdown() {
    vkDeviceWaitIdle(renderer.getDevice());
    engine.shutdown();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (editorDescPool) vkDestroyDescriptorPool(renderer.getDevice(), editorDescPool, nullptr);
    renderer.cleanup();
}

void Editor::createEditorDescriptorPool() {
    VkDescriptorPoolSize sizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},
    };
    VkDescriptorPoolCreateInfo pi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pi.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pi.maxSets = 1000;
    pi.poolSizeCount = 2;
    pi.pPoolSizes = sizes;
    vkCreateDescriptorPool(renderer.getDevice(), &pi, nullptr, &editorDescPool);
}

void Editor::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(renderer.getWindow(), true);

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.ApiVersion = VK_API_VERSION_1_3;
    initInfo.Instance = renderer.getInstance();
    initInfo.PhysicalDevice = renderer.getPhysicalDevice();
    initInfo.Device = renderer.getDevice();
    initInfo.QueueFamily = renderer.getGraphicsQueueFamily();
    initInfo.Queue = renderer.getGraphicsQueue();
    initInfo.DescriptorPool = editorDescPool;
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = 3; // safe default, >= MinImageCount
    initInfo.UseDynamicRendering = false;
    initInfo.PipelineInfoMain.RenderPass = renderer.getRenderPass();
    initInfo.PipelineInfoMain.Subpass = 0;
    initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&initInfo);
}

void Editor::updateViewportDescriptor() {
    EngineFrame frame = engine.getOutputFrame();

    // Validate everything
    if (!frame.outputImageView || !frame.outputImage) return;
    if (!engine.getOutputSampler()) return;
    if (frame.width == 0 || frame.height == 0) return;

    static uint32_t lastW = 0, lastH = 0;
    static VkImageView lastView = VK_NULL_HANDLE;

    // First time: create the descriptor set via ImGui so it's compatible
    // with ImGui's pipeline layout. We only call AddTexture ONCE.
    if (viewportDescSet == VK_NULL_HANDLE) {
        viewportDescSet = ImGui_ImplVulkan_AddTexture(
            engine.getOutputSampler(),
            frame.outputImageView,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        lastW = frame.width;
        lastH = frame.height;
        lastView = frame.outputImageView;
        return;
    }

    // Subsequent frames: if the image changed, update the descriptor in-place
    // without going through ImGui's AddTexture/RemoveTexture (which triggers
    // the lazy-update crash path on Intel)
    bool needsUpdate = (frame.width != lastW) ||
                       (frame.height != lastH) ||
                       (frame.outputImageView != lastView);

    if (!needsUpdate) return;

    vkDeviceWaitIdle(renderer.getDevice());

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = engine.getOutputSampler();
    imageInfo.imageView = frame.outputImageView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = viewportDescSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(renderer.getDevice(), 1, &write, 0, nullptr);

    lastW = frame.width;
    lastH = frame.height;
    lastView = frame.outputImageView;
}

// ============ Actions ============

void Editor::newScene() {
    engine.newScene();

    // Create default camera
    EntityID cam = engine.createEntity("Main Camera");
    engine.setEntityPosition(cam, glm::vec3(0, 2, 8));
    engine.setEntityAsCamera(cam, true);

    selectedEntity = INVALID_ENTITY;
    currentScenePath.clear();
    unsavedChanges = false;
    log("New scene created");
}

void Editor::openScene() {
    showFileDialog = true;
    fileDialogSave = false;
    fileDialogPath = "scenes/";
}

void Editor::saveScene() {
    if (currentScenePath.empty()) {
        saveSceneAs();
        return;
    }
    if (engine.saveScene(currentScenePath)) {
        unsavedChanges = false;
        log("Scene saved: " + currentScenePath);
    } else {
        log("Failed to save scene!", LogEntry::Error);
    }
}

void Editor::saveSceneAs() {
    showFileDialog = true;
    fileDialogSave = true;
    fileDialogPath = "scenes/";
}

void Editor::playScene() {
    if (playState == EditorPlayState::Edit) {
        engine.play();
        playState = EditorPlayState::Play;
        log("Play mode started");
    } else if (playState == EditorPlayState::Pause) {
        engine.play();
        playState = EditorPlayState::Play;
        log("Resumed");
    }
}

void Editor::pauseScene() {
    if (playState == EditorPlayState::Play) {
        engine.pause();
        playState = EditorPlayState::Pause;
        log("Paused");
    }
}

void Editor::stopScene() {
    if (playState != EditorPlayState::Edit) {
        engine.stop();
        playState = EditorPlayState::Edit;
        log("Stopped - back to edit mode");
    }
}

void Editor::duplicateEntity() {
    if (selectedEntity == INVALID_ENTITY) return;
    auto info = engine.getEntityInfo(selectedEntity);
    EntityID e = engine.createEntity(info.name + " (Copy)");
    engine.setEntityPosition(e, info.position + glm::vec3(1, 0, 0));
    engine.setEntityRotation(e, info.rotation);
    engine.setEntityScale(e, info.scale);
    if (info.hasModel) engine.setEntityModel(e, info.modelPath);
    selectedEntity = e;
    unsavedChanges = true;
    log("Duplicated: " + info.name);
}

void Editor::deleteSelectedEntity() {
    if (selectedEntity == INVALID_ENTITY) return;
    auto info = engine.getEntityInfo(selectedEntity);
    engine.destroyEntity(selectedEntity);
    selectedEntity = INVALID_ENTITY;
    unsavedChanges = true;
    log("Deleted: " + info.name);
}

void Editor::log(const std::string& msg, LogEntry::Level level) {
    LogEntry entry;
    entry.level = level;
    entry.message = msg;
    entry.timestamp = Time::getTime();
    consoleLogs.push_back(entry);
    if (consoleLogs.size() > 500) consoleLogs.erase(consoleLogs.begin());
}

ImVec4 Editor::getPlayStateColor() const {
    switch (playState) {
        case EditorPlayState::Play:  return ImVec4(0.2f, 0.8f, 0.3f, 1.0f);
        case EditorPlayState::Pause: return ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
        default: return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
    }
}

const char* Editor::getPlayStateText() const {
    switch (playState) {
        case EditorPlayState::Play:  return "PLAYING";
        case EditorPlayState::Pause: return "PAUSED";
        default: return "EDITING";
    }
}
