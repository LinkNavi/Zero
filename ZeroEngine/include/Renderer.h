#ifndef RENDERER_H
#define RENDERER_H

#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include <vector>

struct Vertex {
    float position[3];
    float color[4];
    float texcoord[2];
};

struct MVP {
    float mvp[16];
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    void Init();
    void Shutdown();
    void BeginFrame();
    void EndFrame();
    void Clear(float r, float g, float b, float a = 1.0f);
    
    sg_buffer CreateVertexBuffer(const Vertex* vertices, size_t count);
    sg_buffer CreateIndexBuffer(const uint16_t* indices, size_t count);
    sg_shader CreateShader(const char* vs_src, const char* fs_src);
    sg_pipeline CreatePipeline(sg_shader shader, sg_vertex_layout_state layout);
    
    void DrawMesh(sg_pipeline pipeline, sg_bindings bindings, int numElements);
    void DrawMeshWithUniforms(sg_pipeline pipeline, sg_bindings bindings, 
                              const void* uniformData, size_t uniformSize, int numElements);
    
    void CreateCubeMesh(sg_buffer* vbuf, sg_buffer* ibuf, int* numIndices);
    void CreateQuadMesh(sg_buffer* vbuf, sg_buffer* ibuf, int* numIndices);
    
    sg_shader GetDefaultColorShader();
    sg_pipeline CreateDefaultColorPipeline();
    
    int GetWidth() const { return sapp_width(); }
    int GetHeight() const { return sapp_height(); }
    float GetAspectRatio() const { return (float)sapp_width() / (float)sapp_height(); }

private:
    sg_pass_action mPassAction = {};
    sg_shader mDefaultShader = {};
    sg_pipeline mDefaultPipeline = {};
    bool mInitialized = false;
};

#endif // RENDERER_H
