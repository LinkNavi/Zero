// ZeroRuntime - Standalone executable using ZeroEngine library
#include "ZeroEngine.h"
#include "Input.h"
#include "CameraController.h"
#include "Config.h"
#include "ResourcePath.h"
#include <iostream>
#include <filesystem>

int main(int argc, char** argv) {
    std::cout << "=== Zero Runtime ===\n";
    
    // Parse command line
    std::string scenePath = argc > 1 ? argv[1] : "";
    bool createDefaultScene = scenePath.empty();
    
    // Initialize engine
    ZeroEngine engine;
    
    EngineConfig config;
    config.mode = EngineMode::Standalone;
    config.windowTitle = "Zero Runtime";
    config.width = 1920;
    config.height = 1080;
    config.enablePostProcess = false;
    config.enableShadows = true;
    config.enableSkybox = false;  // Disable skybox by default (requires textures)
    
    if (!engine.init(config)) {
        std::cerr << "Failed to initialize engine\n";
        return -1;
    }
    
    // Load or create scene
    if (!createDefaultScene && std::filesystem::exists(scenePath)) {
        std::cout << "Loading scene: " << scenePath << "\n";
        if (!engine.loadScene(scenePath)) {
            std::cerr << "Failed to load scene, creating default instead\n";
            createDefaultScene = true;
        }
    } else {
        createDefaultScene = true;
    }
    
    if (createDefaultScene) {
        std::cout << "Creating default scene...\n";
        engine.newScene();
        
        // Create a test cube
        EntityID cube = engine.createEntity("TestCube");
        engine.setEntityPosition(cube, glm::vec3(0, 0, -5));
        engine.setEntityScale(cube, glm::vec3(2));
        
        // Try to load a model if available
        std::string cubePath = ResourcePath::models("cube.obj");
        if (std::filesystem::exists(cubePath)) {
            engine.setEntityModel(cube, cubePath);
            std::cout << "  ✓ Loaded cube model\n";
        } else {
            std::cout << "  ! No cube.obj found at: " << cubePath << "\n";
        }
        
        // Create camera
        EntityID cam = engine.createEntity("MainCamera");
        engine.setEntityPosition(cam, glm::vec3(0, 2, 8));
        engine.setEntityAsCamera(cam, true);
        std::cout << "  ✓ Created camera\n";
        
        // Optionally save the default scene
        if (argc > 1) {
            std::string savePath = argv[1];
            if (savePath.find(".zscene") == std::string::npos) {
                savePath += ".zscene";
            }
            
            std::cout << "Saving default scene to: " << savePath << "\n";
            if (engine.saveScene(savePath)) {
                std::cout << "  ✓ Scene saved\n";
            } else {
                std::cerr << "  ✗ Failed to save scene\n";
            }
        }
    }
    
    // Print scene info
    auto entities = engine.getEntities();
    std::cout << "\n=== Scene Loaded ===\n";
    std::cout << "Entities: " << entities.size() << "\n";
    int modelCount = 0;
    for (const auto& e : entities) {
        std::cout << "  - " << e.name << " (ID: " << e.id << ")\n";
        std::cout << "    Position: (" << e.position.x << ", " << e.position.y << ", " << e.position.z << ")\n";
        if (e.hasModel) {
            std::cout << "    Model: " << e.modelPath << "\n";
            modelCount++;
        }
        if (e.isCamera) {
            std::cout << "    Camera" << (e.isActiveCamera ? " [ACTIVE]" : "") << "\n";
        }
    }
    std::cout << "Total models to render: " << modelCount << "\n";
    
    // Start in edit mode (editor camera has controls)
    // engine.play();  // Don't start playing - stay in edit mode for camera controls
    
    std::cout << "\n=== Controls ===\n";
    std::cout << "  WASD - Move\n";
    std::cout << "  Mouse - Look (hold right-click)\n";
    std::cout << "  Space/Ctrl - Up/Down\n";
    std::cout << "  Shift - Sprint\n";
    std::cout << "  ESC - Quit\n";
    std::cout << "\nRunning in EDIT mode (camera controls active)...\n\n";
    
    // Main loop
    int frameCount = 0;
    while (engine.isRunning()) {
        if (Input::getKey(Key::Escape)) {
            std::cout << "Quitting...\n";
            break;
        }
        
        engine.update();
        
        // Print FPS every 60 frames
        if (++frameCount % 60 == 0) {
            // std::cout << "Frame " << frameCount << "\n";
        }
    }
    
    std::cout << "Shutting down...\n";
    engine.shutdown();
    
    std::cout << "Goodbye!\n";
    return 0;
}
