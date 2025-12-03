// Renderer.cpp
#include "Renderer.h"

bool Renderer::Init(int width, int height, const char* title) {
    InitWindow(width, height, title);
    SetTargetFPS(60);

    camera.position = {0.0f, 3.0f, 10.0f};
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    return true;
}

void Renderer::Shutdown() {
    CloseWindow();
}

void Renderer::Begin3D() { BeginMode3D(camera); }
void Renderer::End3D() { EndMode3D(); }

void Renderer::DrawMesh(const ZeroMesh& myMesh,
                        const Vector3& position,
                        const Vector3& rotation,
                        const Vector3& scale,
                        Color color)
{
    if (!myMesh.isValid) return;

    DrawModelEx(myMesh.model,
                position,
                rotation, // Raylib rotates around axis by degrees
                0.0f,     // angle (if needed)
                scale,
                color);
}


ZeroMesh Renderer::CreateCube(float size) {
    Mesh mesh = GenMeshCube(size, size, size);
    return ZeroMesh::FromMesh(mesh); // moved out
}

Shader Renderer::CreateDefaultShader() {
    // Many raylib versions don't have LoadShaderDefault().
    // Return an "empty" shader (id==0). If you want an actual shader,
    // you can LoadShaderFromMemory(vertexSrc, fragSrc) here.
    Shader s{};
    s.id = 0;

    return s;
}

void Renderer::UnloadShader(Shader& shader) {
    // Only unload if shader appears valid (id != 0)
    if (shader.id != 0) {
        ::UnloadShader(shader);
    }
    shader = Shader{}; // reset
}


void Renderer::UnloadMesh(ZeroMesh& mesh) {
    if (mesh.isValid) {
        UnloadModel(mesh.model);
        mesh.isValid = false;
    }
}

