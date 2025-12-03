#include "World.h"
#include "Logger.h"
#include <raylib.h>
#include <exception>

int main(int argc, char* argv[]) {
    // Initialize logging system
    Logger::Instance().Init("zero_engine.log", LogLevel::DEBUG);
    LOG_INFO("=== ZeroEngine Starting ===");

    try {
        const int WINDOW_WIDTH = 1280;
        const int WINDOW_HEIGHT = 720;

        World world;
        world.Init(WINDOW_WIDTH, WINDOW_HEIGHT, "ZeroEngine - ECS Runtime");

        // Create initial scene
        LOG_INFO("Creating initial scene...");
        
        // Create player
        Entity player = world.CreatePlayerCube({0, 0, 0}, 1.0f, true);
        
        // Create camera following player
        world.CreateCameraEntity(player, {0, 5, 15});

        // Create some environment cubes
        world.CreateCube({5, 0, 0}, {1, 1, 1}, RED);
        world.CreateCube({-5, 0, 0}, {1, 1, 1}, GREEN);
        world.CreateCube({0, 0, 5}, {1, 2, 1}, YELLOW);
        world.CreateCube({0, 0, -5}, {2, 1, 1}, PURPLE);

        LOG_INFO("Scene creation complete");
        LOG_INFO("=== Controls ===");
        LOG_INFO("WASD/Arrows - Move");
        LOG_INFO("Space/Shift - Up/Down");
        LOG_INFO("F3 - Toggle Debug");
        LOG_INFO("F5 - Save Scene");
        LOG_INFO("F9 - Load Scene");
        LOG_INFO("P - Pause");
        LOG_INFO("ESC - Quit");

        SetTargetFPS(60);

        // Main game loop
        while (!WindowShouldClose()) {
            float dt = GetFrameTime();

            // Handle input
            world.HandleInput();

            // Update world
            world.Update(dt);

            // Render
            BeginDrawing();
            ClearBackground(DARKGRAY);
            
            world.Render();

            // Show pause indicator
            if (world.isPaused) {
                DrawText("PAUSED", WINDOW_WIDTH/2 - 60, 20, 30, YELLOW);
            }

            EndDrawing();
        }

        // Cleanup
        world.Shutdown();
        Logger::Instance().Shutdown();

        LOG_INFO("=== ZeroEngine Shutdown Complete ===");
        return 0;

    } catch (const std::exception& e) {
        LOG_FATAL("Fatal exception: " + std::string(e.what()));
        Logger::Instance().Shutdown();
        return 1;
    }
}
