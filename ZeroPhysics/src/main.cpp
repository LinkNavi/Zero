#include "raylib.h"
#include "ZeroPhysicsMesh.h"
#include <iostream>

int main() {
    // --- Window ---
    InitWindow(1024, 768, "ZeroPhysics Mesh Demo");
    SetTargetFPS(60);

    // --- Models ---
    Mesh cubeMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    Model cubeModel = LoadModelFromMesh(cubeMesh);
    PhysicsMesh cubePhysicsMesh = physicsMeshFromModel(cubeModel);

    Mesh floorMesh = GenMeshCube(10.0f, 1.0f, 10.0f);
    Model floorModel = LoadModelFromMesh(floorMesh);
    PhysicsMesh floorPhysicsMesh = physicsMeshFromModel(floorModel);

    // --- Colliders ---
    MeshCollider colliders[3];
    colliders[0] = { &cubePhysicsMesh, Vec3{-1.0f, 5.0f, 0.0f}, Vec3{0,0,0}, 1.0f }; // cube 1
    colliders[1] = { &cubePhysicsMesh, Vec3{ 1.0f, 8.0f, 0.0f}, Vec3{0,0,0}, 1.0f }; // cube 2
    colliders[2] = { &floorPhysicsMesh, Vec3{0.0f, 0.0f, 0.0f}, Vec3{0,0,0}, 0.0f }; // floor, static

    // --- Camera ---
    Camera camera{};
    camera.position = Vector3{6.0f, 6.0f, 6.0f};
    camera.target   = Vector3{0.0f, 2.0f, 0.0f};
    camera.up       = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy     = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    bool showTriangles = false;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        // --- Camera control ---
        if (IsKeyDown(KEY_RIGHT)) camera.position.x += 6.0f * dt;
        if (IsKeyDown(KEY_LEFT))  camera.position.x -= 6.0f * dt;
        if (IsKeyDown(KEY_UP))    camera.position.z -= 6.0f * dt;
        if (IsKeyDown(KEY_DOWN))  camera.position.z += 6.0f * dt;
        if (IsKeyPressed(KEY_SPACE)) showTriangles = !showTriangles;

        // --- Physics step ---
        meshStep(colliders, 3, dt, Vec3{0.0f, -9.81f, 0.0f});

        // --- Drawing ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        DrawGrid(20, 1.0f);

      
for (int i = 0; i < 3; ++i) {
    MeshCollider &c = colliders[i];
    Color col = (i == 2) ? GRAY : (i == 0 ? MAROON : BLUE);

    if (showTriangles) {
        // Draw each triangle
        for (const auto& tri : c.mesh->triangles) {
            Vector3 v0 = toVector3(vecAddPub(tri.v0, c.pos));
            Vector3 v1 = toVector3(vecAddPub(tri.v1, c.pos));
            Vector3 v2 = toVector3(vecAddPub(tri.v2, c.pos));
            DrawTriangle3D(v0, v1, v2, col);
        }
    } else {
        // Draw simple cubes directly
        if (i == 2) {
            DrawCube(toVector3(c.pos), 10.0f, 1.0f, 10.0f, col); // floor
        } else {
            DrawCube(toVector3(c.pos), 1.0f, 1.0f, 1.0f, col);   // cubes
        }
    }
}


        EndMode3D();

        DrawText("Arrow keys: move camera | Space: toggle tri/model view", 10, 10, 14, DARKGRAY);
        EndDrawing();
    }

    UnloadModel(cubeModel);
    UnloadModel(floorModel);
    CloseWindow();
    return 0;
}
