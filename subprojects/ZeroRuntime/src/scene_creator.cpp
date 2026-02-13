// Scene Creator Utility - Create test scenes programmatically
#include "ZeroEngine.h"
#include "ResourcePath.h"
#include <iostream>
#include <string>

void createEmptyScene(ZeroEngine& engine) {
    engine.newScene();
    
    EntityID cam = engine.createEntity("MainCamera");
    engine.setEntityPosition(cam, glm::vec3(0, 2, 5));
    engine.setEntityAsCamera(cam, true);
    
    std::cout << "Created empty scene with camera\n";
}

void createTestScene(ZeroEngine& engine) {
    engine.newScene();
    
    // Ground plane
    EntityID ground = engine.createEntity("Ground");
    engine.setEntityPosition(ground, glm::vec3(0, -1, 0));
    engine.setEntityScale(ground, glm::vec3(10, 0.1f, 10));
    engine.setEntityModel(ground, ResourcePath::models("cube.obj"));
    
    // Test cubes
    for (int i = 0; i < 3; i++) {
        EntityID cube = engine.createEntity("Cube_" + std::to_string(i));
        engine.setEntityPosition(cube, glm::vec3(i * 3 - 3, 1, -5));
        engine.setEntityScale(cube, glm::vec3(1));
        engine.setEntityRotation(cube, glm::vec3(0, i * 45, 0));
        engine.setEntityModel(cube, ResourcePath::models("cube.obj"));
    }
    
    // Camera
    EntityID cam = engine.createEntity("MainCamera");
    engine.setEntityPosition(cam, glm::vec3(0, 3, 10));
    engine.setEntityAsCamera(cam, true);
    
    std::cout << "Created test scene with ground + 3 cubes\n";
}

void createDemoScene(ZeroEngine& engine) {
    engine.newScene();
    
    // Multiple objects at different positions
    struct Object { std::string name; glm::vec3 pos; glm::vec3 rot; glm::vec3 scale; };
    std::vector<Object> objects = {
        {"Center",      {0, 0, -5},    {0, 0, 0},      {2, 2, 2}},
        {"Left",        {-5, 0, -5},   {0, 45, 0},     {1, 1, 1}},
        {"Right",       {5, 0, -5},    {0, -45, 0},    {1, 1, 1}},
        {"Top",         {0, 5, -5},    {45, 0, 0},     {1, 1, 1}},
        {"Floor",       {0, -2, -5},   {0, 0, 0},      {20, 0.1f, 20}},
    };
    
    for (const auto& obj : objects) {
        EntityID e = engine.createEntity(obj.name);
        engine.setEntityPosition(e, obj.pos);
        engine.setEntityRotation(e, obj.rot);
        engine.setEntityScale(e, obj.scale);
        engine.setEntityModel(e, ResourcePath::models("cube.obj"));
    }
    
    // Camera
    EntityID cam = engine.createEntity("MainCamera");
    engine.setEntityPosition(cam, glm::vec3(0, 2, 12));
    engine.setEntityAsCamera(cam, true);
    
    std::cout << "Created demo scene with " << objects.size() << " objects\n";
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <scene_type> <output.zscene>\n";
        std::cout << "Scene types:\n";
        std::cout << "  empty  - Empty scene with just a camera\n";
        std::cout << "  test   - Simple test scene (ground + 3 cubes)\n";
        std::cout << "  demo   - Demo scene with multiple objects\n";
        return 1;
    }
    
    std::string sceneType = argv[1];
    std::string outputPath = argv[2];
    
    if (outputPath.find(".zscene") == std::string::npos) {
        outputPath += ".zscene";
    }
    
    // Initialize engine in headless mode
    ZeroEngine engine;
    
    EngineConfig config;
    config.mode = EngineMode::Standalone;
    config.windowTitle = "Scene Creator";
    config.width = 800;
    config.height = 600;
    config.enablePostProcess = false;
    config.enableShadows = false;
    config.enableSkybox = false;
    
    if (!engine.init(config)) {
        std::cerr << "Failed to initialize engine\n";
        return -1;
    }
    
    // Create the appropriate scene
    if (sceneType == "empty") {
        createEmptyScene(engine);
    } else if (sceneType == "test") {
        createTestScene(engine);
    } else if (sceneType == "demo") {
        createDemoScene(engine);
    } else {
        std::cerr << "Unknown scene type: " << sceneType << "\n";
        engine.shutdown();
        return -1;
    }
    
    // Save the scene
    std::cout << "Saving scene to: " << outputPath << "\n";
    if (engine.saveScene(outputPath)) {
        std::cout << "✓ Scene saved successfully!\n";
        
        // Print entity count
        auto entities = engine.getEntities();
        std::cout << "  Entities: " << entities.size() << "\n";
        for (const auto& e : entities) {
            std::cout << "    - " << e.name << "\n";
        }
    } else {
        std::cerr << "✗ Failed to save scene\n";
        engine.shutdown();
        return -1;
    }
    
    engine.shutdown();
    return 0;
}
