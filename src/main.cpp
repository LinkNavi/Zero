#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Renderer.h"
#include "ModelLoader.h"
#include "Camera.h"
#include "CameraController.h"
#include "Input.h"
#include "Time.h"
#include "Engine.h"
#include "transform.h"
#include "tags.h"
#include "Config.h"
#include "ResourcePath.h"
#include "SceneManager.h"
#include "Pipeline.h"

#include <chrono>
#include <iostream>

// Globals
VulkanRenderer *g_renderer = nullptr;
ModelLoader *g_modelLoader = nullptr;
Pipeline *g_pipeline = nullptr;
Camera *g_camera = nullptr;
VkDescriptorPool g_descriptorPool = VK_NULL_HANDLE;

// ============== SCENES ==============

class DuckScene : public Scene {
public:
  Model duckModel;
  std::vector<Renderable> ducks;
  int gridSize = 4;
  float spacing = 4.0f;
  float rotationSpeed = 30.0f;
  bool autoRotate = true;

  DuckScene() { name = "DuckScene"; }

  void onLoad() override {
    duckModel = g_modelLoader->load(ResourcePath::models("Duck.glb"));

    ducks.clear();
    float offset = (gridSize - 1) * spacing * 0.5f;

    for (int x = 0; x < gridSize; x++) {
      for (int z = 0; z < gridSize; z++) {
        Renderable r;
        r.init(&duckModel, g_renderer->getAllocator(), g_renderer->getDevice(),
               g_modelLoader->getDefaultWhite(),
               g_pipeline->getDescriptorLayout());
        r.transform =
            glm::translate(glm::mat4(1), glm::vec3(x * spacing - offset, 0,
                                                   z * spacing - offset));
        r.transform = glm::scale(r.transform, glm::vec3(0.5f));
        ducks.push_back(std::move(r));
      }
    }

    std::cout << "✓ DuckScene: " << ducks.size() << " ducks\n";
  }

  void update(float dt) override {
    if (autoRotate) {
      for (auto &duck : ducks) {
        duck.transform =
            glm::rotate(duck.transform, glm::radians(rotationSpeed * dt),
                        glm::vec3(0, 1, 0));
      }
    }
  }

  void render(VkCommandBuffer cmd) {
    g_pipeline->bind(cmd);
    for (auto &duck : ducks) {
      g_pipeline->bindDescriptor(cmd, duck.descriptorSet);

      PushConstants pc;
      pc.viewProj = g_camera->getViewProjectionMatrix();
      pc.model = duck.transform;
      pc.lightDir = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
      pc.lightColor = glm::vec3(1.0f, 0.95f, 0.9f);
      pc.ambientStrength = 0.3f;
      g_pipeline->pushConstants(cmd, pc);

      VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(cmd, 0, 1, &duck.model->vertexBuffer, &offset);
      vkCmdBindIndexBuffer(cmd, duck.model->indexBuffer, 0,
                           VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexed(cmd, duck.model->totalIndices, 1, 0, 0, 0);
    }
  }

  void onUnload() override {
    for (auto &d : ducks)
      d.cleanup();
    ducks.clear();
    g_modelLoader->cleanup(duckModel);
  }
};

class WalkingScene : public Scene {
public:
  Model humanModel;
  Renderable human;

  WalkingScene() { name = "WalkingScene"; }

  void onLoad() override {
    humanModel = g_modelLoader->load(ResourcePath::models("Walking.fbx"));

    human.init(&humanModel, g_renderer->getAllocator(), g_renderer->getDevice(),
               g_modelLoader->getDefaultWhite(),
               g_pipeline->getDescriptorLayout());
    human.transform = glm::scale(glm::mat4(1), glm::vec3(0.01f));

    std::cout << "✓ WalkingScene: " << humanModel.bones.size() << " bones, "
              << humanModel.animations.size() << " anims\n";

    // In WalkingScene::onLoad() after loading
    std::cout << "Bone hierarchy:\n";
    for (size_t i = 0; i < humanModel.bones.size() && i < 10; i++) {
      std::cout << "  [" << i << "] " << humanModel.bones[i].name
                << " parent=" << humanModel.bones[i].parentIndex << "\n";
    }

    // In WalkingScene::onLoad(), after loading:
    std::cout << "Animation channels:\n";
    for (size_t i = 0; i < humanModel.animations[0].channels.size() && i < 10;
         i++) {
      std::cout << "  " << humanModel.animations[0].channels[i].nodeName
                << "\n";
    }
std::cout << "Channel to bone matching (after fix):\n";
int matched = 0;
for (const auto& ch : humanModel.animations[0].channels) {
    bool found = humanModel.boneMap.find(ch.nodeName) != humanModel.boneMap.end();
    if (found) matched++;
    else std::cout << "  NO MATCH: " << ch.nodeName << "\n";
}
std::cout << "Matched: " << matched << "/" << humanModel.animations[0].channels.size() << "\n";

  }

  void update(float dt) override { human.update(dt); }

  void render(VkCommandBuffer cmd) {
    g_pipeline->bind(cmd);
    g_pipeline->bindDescriptor(cmd, human.descriptorSet);

    PushConstants pc;
    pc.viewProj = g_camera->getViewProjectionMatrix();
    pc.model = human.transform;
    pc.lightDir = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
    pc.lightColor = glm::vec3(1.0f, 0.95f, 0.9f);
    pc.ambientStrength = 0.3f;
    g_pipeline->pushConstants(cmd, pc);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &humanModel.vertexBuffer, &offset);
    vkCmdBindIndexBuffer(cmd, humanModel.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, humanModel.totalIndices, 1, 0, 0, 0);
  }

  void onUnload() override {
    human.cleanup();
    g_modelLoader->cleanup(humanModel);
  }
};

// ============== MAIN ==============

int main() {
  ResourcePath::init();
  Config config;
  config.load(ResourcePath::config("config.ini"));

  VulkanRenderer renderer;
  if (!renderer.init(1920, 1080, "Zero Engine")) {
    std::cerr << "Failed to init renderer\n";
    return -1;
  }
  renderer.initImGui();

  float xscale, yscale;
  glfwGetWindowContentScale(renderer.getWindow(), &xscale, &yscale);
  ImGui::GetIO().FontGlobalScale = xscale;
  ImGui::GetStyle().ScaleAllSizes(xscale);

  g_renderer = &renderer;

  Input::init(renderer.getWindow());
  Time::init();

  Camera camera;
  camera.position = glm::vec3(0, 2, 5);
  camera.aspectRatio = float(renderer.getWidth()) / float(renderer.getHeight());
  CameraController camController(&camera, config);
  g_camera = &camera;

  // Descriptor pool
  VkDescriptorPoolSize poolSizes[] = {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 200},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 200}};
  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 2;
  poolInfo.pPoolSizes = poolSizes;
  poolInfo.maxSets = 200;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  vkCreateDescriptorPool(renderer.getDevice(), &poolInfo, nullptr,
                         &g_descriptorPool);

  // Pipeline
  Pipeline pipeline;
  pipeline.init(renderer.getDevice(), renderer.getRenderPass(),
                ResourcePath::shaders("unified_vert.spv"),
                ResourcePath::shaders("unified_frag.spv"));
  g_pipeline = &pipeline;

  // Model loader
  ModelLoader modelLoader;
  modelLoader.init(renderer.getDevice(), renderer.getAllocator(),
                   renderer.getCommandPool(), renderer.getGraphicsQueue(),
                   g_descriptorPool, pipeline.getDescriptorLayout());
  g_modelLoader = &modelLoader;

  // Scenes
  SceneManager sceneManager;
  DuckScene duckScene;
  WalkingScene walkingScene;
  sceneManager.registerScene("DuckScene", &duckScene);
  sceneManager.registerScene("WalkingScene", &walkingScene);
  sceneManager.loadScene("WalkingScene");

  std::cout
      << "\nControls: WASD+Mouse2=Move, 1=Ducks, 2=Walking, J=UI, ESC=Quit\n\n";

  bool showUI = true;

  using Clock = std::chrono::high_resolution_clock;
  auto lastTime = Clock::now();
  auto fpsTimer = Clock::now();
  int frameCount = 0;
  float displayFps = 0, displayFrameTime = 0;

  while (!renderer.shouldClose()) {
    auto now = Clock::now();
    float dt = std::chrono::duration<float>(now - lastTime).count();
    lastTime = now;

    frameCount++;
    float elapsed = std::chrono::duration<float>(now - fpsTimer).count();
    if (elapsed >= 0.5f) {
      displayFps = frameCount / elapsed;
      displayFrameTime = elapsed / frameCount * 1000.0f;
      frameCount = 0;
      fpsTimer = now;
    }

    renderer.pollEvents();

    if (Input::getKey(Key::Escape))
      break;
    if (Input::getKeyDown(Key::J))
      showUI = !showUI;
    if (Input::getKeyDown(Key::Num1)) {
      vkDeviceWaitIdle(renderer.getDevice());
      sceneManager.loadScene("DuckScene");
    }
    if (Input::getKeyDown(Key::Num2)) {
      vkDeviceWaitIdle(renderer.getDevice());
      sceneManager.loadScene("WalkingScene");
    }

    Time::update();
    camController.update(dt, renderer.getWindow());
    sceneManager.update(dt);

    VkCommandBuffer cmd;
    renderer.beginFrame(cmd);

    VkViewport viewport{
        0, 0, float(renderer.getWidth()), float(renderer.getHeight()), 0, 1};
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    VkRect2D scissor{{0, 0}, {renderer.getWidth(), renderer.getHeight()}};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Render scene
    if (auto *duck =
            dynamic_cast<DuckScene *>(sceneManager.getCurrentScene())) {
      duck->render(cmd);
    } else if (auto *walk = dynamic_cast<WalkingScene *>(
                   sceneManager.getCurrentScene())) {
      walk->render(cmd);
    }

    if (showUI) {
      renderer.imguiNewFrame();
      ImGui::Begin("Debug");
      ImGui::Text("FPS: %.1f (%.2f ms)", displayFps, displayFrameTime);
      ImGui::Separator();

      auto *scene = sceneManager.getCurrentScene();
      ImGui::Text("Scene: %s", scene ? scene->name.c_str() : "None");

      if (ImGui::Button("Ducks [1]")) {
        vkDeviceWaitIdle(renderer.getDevice());
        sceneManager.loadScene("DuckScene");
      }
      ImGui::SameLine();
      if (ImGui::Button("Walking [2]")) {
        vkDeviceWaitIdle(renderer.getDevice());
        sceneManager.loadScene("WalkingScene");
      }

      ImGui::Separator();
      if (auto *duck = dynamic_cast<DuckScene *>(scene)) {
        ImGui::Text("Ducks: %zu", duck->ducks.size());
        ImGui::Checkbox("Auto Rotate", &duck->autoRotate);
        if (duck->autoRotate)
          ImGui::SliderFloat("Speed", &duck->rotationSpeed, 0, 360);
      } else if (auto *walk = dynamic_cast<WalkingScene *>(scene)) {
        ImGui::Text("Bones: %zu", walk->humanModel.bones.size());
        ImGui::Text("Anims: %zu", walk->humanModel.animations.size());
        if (walk->human.animator.playing) {
          ImGui::Text("Time: %.2f / %.2f", walk->human.animator.currentTime,
                      walk->human.animator.getDuration());
          ImGui::SliderFloat("Speed", &walk->human.animator.speed, 0.1f, 3.0f);
        }
      }

      ImGui::Separator();
      ImGui::Text("Camera: %.1f, %.1f, %.1f", camera.position.x,
                  camera.position.y, camera.position.z);

      ImGui::End();
      renderer.imguiRender(cmd);
    }

    renderer.endFrame(cmd);
    Input::update();
  }

  vkDeviceWaitIdle(renderer.getDevice());

  sceneManager.cleanup();
  modelLoader.cleanupLoader();
  pipeline.cleanup();
  vkDestroyDescriptorPool(renderer.getDevice(), g_descriptorPool, nullptr);
  renderer.cleanup();

  return 0;
}
