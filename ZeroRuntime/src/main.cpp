#include "World.h"
#include "Logger.h"
#include <raylib.h>
#include <exception>

int main(int argc, char* argv[]) {
    // Initialize logging system
    Logger::Instance().Init("zero_engine.log", LogLevel::DEBUG);
    LOG_INFO("=== ZeroEngine Starting with Wren Scripting ===");

    try {
        const int WINDOW_WIDTH = 1280;
        const int WINDOW_HEIGHT = 720;

        World world;
        world.Init(WINDOW_WIDTH, WINDOW_HEIGHT, "ZeroEngine - Wren Scripting Demo");

        // Create initial scene
        LOG_INFO("Creating initial scene...");
        
        // Create scripted player (uses Player.wren script)
        Entity player = world.CreateScriptedCube(
            {0, 1, 0},
            "scripts/Player.wren",
            "Player",
            BLUE
        );
        
        // Create camera following player
        world.CreateCameraEntity(player, {0, 5, 15});

        // Create some scripted entities
        Entity rotatingCube = world.CreateScriptedCube(
            {5, 1, 0},
            "scripts/RotatingCube.wren",
            "RotatingCube",
            RED
        );

        Entity orbiter = world.CreateScriptedCube(
            {0, 1, 5},
            "scripts/Orbiter.wren",
            "Orbiter",
            GREEN
        );

        Entity follower = world.CreateScriptedCube(
            {-5, 1, 0},
            "scripts/Follower.wren",
            "Follower",
            YELLOW
        );

        // Create a cube that will self-destruct
        Entity selfDestruct = world.CreateScriptedCube(
            {0, 1, -5},
            "scripts/SelfDestruct.wren",
            "SelfDestruct",
            PURPLE
        );

        // Create some static environment cubes (no scripts)
        world.CreateCube({10, 0.5f, 0}, {1, 0.5f, 1}, DARKGRAY);
        world.CreateCube({-10, 0.5f, 0}, {1, 0.5f, 1}, DARKGRAY);
        world.CreateCube({0, 0.5f, 10}, {1, 0.5f, 1}, DARKGRAY);
        world.CreateCube({0, 0.5f, -10}, {1, 0.5f, 1}, DARKGRAY);

        LOG_INFO("Scene creation complete");
        LOG_INFO("=== Controls ===");
        LOG_INFO("WASD - Move Player (controlled by Player.wren script)");
        LOG_INFO("Space - Jump");
        LOG_INFO("F3 - Toggle Debug");
        LOG_INFO("F5 - Save Scene");
        LOG_INFO("F9 - Load Scene");
        LOG_INFO("P - Pause");
        LOG_INFO("ESC - Quit");
        LOG_INFO("");
        LOG_INFO("=== Active Scripts ===");
        LOG_INFO("Player.wren - Blue cube, player movement");
        LOG_INFO("RotatingCube.wren - Red cube, bobbing animation");
        LOG_INFO("Orbiter.wren - Green cube, orbital movement");
        LOG_INFO("Follower.wren - Yellow cube, follows origin");
        LOG_INFO("SelfDestruct.wren - Purple cube, destroys after 5 seconds");

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
            ClearBackground(RAYWHITE);
            
            world.Render();

            // Show pause indicator
            if (world.isPaused) {
                DrawText("PAUSED", WINDOW_WIDTH/2 - 60, 20, 30, YELLOW);
            }

            // Instructions overlay
            DrawText("WASD to move player (via script)", 10, WINDOW_HEIGHT - 60, 20, BLACK);
            DrawText("Watch the scripted cubes interact!", 10, WINDOW_HEIGHT - 35, 20, BLACK);

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
