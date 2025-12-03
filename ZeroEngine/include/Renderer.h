// Renderer.h
#pragma once
#include <raylib.h>
#include "Objects.h"
#include <glm/glm.hpp>

class Renderer {
public:
    Camera camera;

    bool Init(int width, int height, const char* title);
    void Shutdown();

    void Begin3D();
    void End3D();

    void DrawMesh(const ZeroMesh& myMesh,
                  const Vector3& position,
                  const Vector3& rotation,
                  const Vector3& scale,
                  Color color);

    ZeroMesh CreateCube(float size);

    // New helper methods
    Shader CreateDefaultShader();
    void UnloadMesh(ZeroMesh& mesh);
    void UnloadShader(Shader& shader);

    void SetCameraPosition(const glm::vec3& pos);
    void SetCameraTarget(const glm::vec3& target);
};
