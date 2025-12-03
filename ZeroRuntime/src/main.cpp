#include "World.h"
#include <raylib.h>

int main() {
    const int WINDOW_WIDTH = 1280;
    const int WINDOW_HEIGHT = 720;

    World world;
    world.Init(WINDOW_WIDTH, WINDOW_HEIGHT, "ZeroEngine ECS Example");

    // create player cube and camera
    Entity player = world.CreatePlayerCube(1.0f, true);
    world.CreateCameraEntity(player, {0,3,10});

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        BeginDrawing();
        ClearBackground(BLACK);

        // world handles input -> physics -> render -> debug
        world.Update(dt);

        EndDrawing();
    }

    world.Shutdown();
    return 0;
}
