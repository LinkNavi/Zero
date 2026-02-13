// ZeroRuntime - standalone executable using ZeroEngine library
#include "ZeroEngine.h"
#include "Input.h"
#include "CameraController.h"
#include "Config.h"
#include "ResourcePath.h"
#include <iostream>

int main(int argc, char** argv) {
    std::string scenePath = argc > 1 ? argv[1] : "scenes/default.zscene";
    
    ZeroEngine engine;
    
    EngineConfig config;
    config.mode = EngineMode::Standalone;
    config.windowTitle = "Zero Runtime";
    config.width = 1920;
    config.height = 1080;
    config.enablePostProcess = false;  // set true once post-process is fixed
    config.enableShadows = true;
    config.enableSkybox = true;
    
    if (!engine.init(config)) {
        std::cerr << "Failed to initialize engine\n";
        return -1;
    }
    
    // Try loading scene file, fall back to default
    if (!engine.loadScene(scenePath)) {
        std::cout << "Creating default scene...\n";
        engine.newScene();
        
        // Create cube
        EntityID cube = engine.createEntity("TestCube");
        engine.setEntityPosition(cube, glm::vec3(0, 0, -5));
        engine.setEntityScale(cube, glm::vec3(2));
        engine.setEntityModel(cube, ResourcePath::models("cube.obj"));
        
        // Create camera
        EntityID cam = engine.createEntity("MainCamera");
        engine.setEntityPosition(cam, glm::vec3(0, 2, 5));
        engine.setEntityAsCamera(cam, true);
    }
    
    // For standalone, go straight to playing
    engine.play();
    
    std::cout << "\nControls:\n"
              << "  WASD - Move\n"
              << "  Mouse - Look (hold right-click)\n"
              << "  Space/Ctrl - Up/Down\n"
              << "  Shift - Sprint\n"
              << "  ESC - Quit\n";
    
    while (engine.isRunning()) {
        if (Input::getKey(Key::Escape)) break;
        engine.update();
    }
    
    engine.shutdown();
    return 0;
}
