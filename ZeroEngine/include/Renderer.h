#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <string>
#include <vector>

struct Vertex {
    float position[3];
    float color[4];
    float texcoord[2];
};

struct Mesh {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    uint32_t numIndices;
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    bool Init();
    void Shutdown();
    void BeginFrame();
    void EndFrame();
    void SetClearColor(float r, float g, float b, float a = 1.0f);
    
    // Mesh creation
    Mesh CreateMesh(const Vertex* vertices, uint32_t numVertices, 
                    const uint16_t* indices, uint32_t numIndices);
    Mesh CreateCube();
    Mesh CreateQuad();
    void DestroyMesh(Mesh& mesh);
    
    // Shader management
    GLuint CreateShader(const char* vertexSrc, const char* fragmentSrc);
    GLuint CreateDefaultShader();
    void DestroyShader(GLuint program);
    
    // Drawing
    void DrawMesh(const Mesh& mesh, GLuint shader, const float* mvp);
    
    // Utilities
    void SetViewport(int width, int height);
    
    bool IsInitialized() const { return mInitialized; }

private:
    bool mInitialized = false;
    float mClearColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    
    GLuint mDefaultShader = 0;
    
    GLuint CompileShader(GLenum type, const char* source);
    GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);
};

#endif // RENDERER_H
