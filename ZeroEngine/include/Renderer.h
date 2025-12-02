

#ifndef RENDERER_H
#define RENDERER_H

#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_glue.h"
#include <vector>

// Vertex structure
struct Vertex {
    float position[3];
    float color[4];
    float texcoord[2];
};

// Transform/MVP matrix
struct MVP {
    float mvp[16];
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    // Initialize the renderer
    void Init();
    
    // Shutdown and cleanup
    void Shutdown();
    
    // Begin a new frame
    void BeginFrame();
    
    // End frame and present
    void EndFrame();
    
    // Clear screen with color
    void Clear(float r, float g, float b, float a = 1.0f);
    
    // Create a mesh from vertices and indices
    sg_buffer CreateVertexBuffer(const Vertex* vertices, size_t count);
    sg_buffer CreateIndexBuffer(const uint16_t* indices, size_t count);
    
    // Create a shader from source
    sg_shader CreateShader(const char* vs_src, const char* fs_src);
    
    // Create a pipeline
    sg_pipeline CreatePipeline(sg_shader shader, sg_vertex_layout_state layout);
    
    // Draw a mesh
    void DrawMesh(sg_pipeline pipeline, sg_bindings bindings, int numElements);
    
    // Draw with custom uniform data
    void DrawMeshWithUniforms(sg_pipeline pipeline, sg_bindings bindings, 
                              const void* uniformData, size_t uniformSize, int numElements);
    
    // Helper: Create a basic colored cube
    void CreateCubeMesh(sg_buffer* vbuf, sg_buffer* ibuf, int* numIndices);
    
    // Helper: Create a basic quad
    void CreateQuadMesh(sg_buffer* vbuf, sg_buffer* ibuf, int* numIndices);
    
    // Helper: Get default shader for colored meshes
    sg_shader GetDefaultColorShader();
    
    // Helper: Create default pipeline for colored meshes
    sg_pipeline CreateDefaultColorPipeline();
    
    // Get current framebuffer dimensions
    int GetWidth() const { return sapp_width(); }
    int GetHeight() const { return sapp_height(); }
    float GetAspectRatio() const { return (float)sapp_width() / (float)sapp_height(); }

private:
    sg_pass_action mPassAction = {};
    sg_shader mDefaultShader = {};
    sg_pipeline mDefaultPipeline = {};
    bool mInitialized = false;
};

// Implementation
inline Renderer::~Renderer() {
    if (mInitialized) {
        Shutdown();
    }
}

inline void Renderer::Init() {
    sg_desc desc = {};
    desc.environment = sglue_environment();
    sg_setup(&desc);
    
    mPassAction.colors[0].load_action = SG_LOADACTION_CLEAR;
    mPassAction.colors[0].clear_value = { 0.1f, 0.1f, 0.1f, 1.0f };
    
    mInitialized = true;
}

inline void Renderer::Shutdown() {
    if (mDefaultShader.id != 0) {
        sg_destroy_shader(mDefaultShader);
    }
    if (mDefaultPipeline.id != 0) {
        sg_destroy_pipeline(mDefaultPipeline);
    }
    sg_shutdown();
    mInitialized = false;
}

inline void Renderer::BeginFrame() {
    sg_pass pass = {};
    pass.action = mPassAction;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);
}

inline void Renderer::EndFrame() {
    sg_end_pass();
    sg_commit();
}

inline void Renderer::Clear(float r, float g, float b, float a) {
    mPassAction.colors[0].clear_value = { r, g, b, a };
}

inline sg_buffer Renderer::CreateVertexBuffer(const Vertex* vertices, size_t count) {
    sg_buffer_desc desc = {};
    desc.data.ptr = vertices;
    desc.data.size = count * sizeof(Vertex);
    return sg_make_buffer(&desc);
}

inline sg_buffer Renderer::CreateIndexBuffer(const uint16_t* indices, size_t count) {
    sg_buffer_desc desc = {};
    desc.data.ptr = indices;
    desc.data.size = count * sizeof(uint16_t);
    return sg_make_buffer(&desc);
}

inline sg_shader Renderer::CreateShader(const char* vs_src, const char* fs_src) {
    sg_shader_desc desc = {};
    desc.vertex_func.source = vs_src;
    desc.fragment_func.source = fs_src;
    return sg_make_shader(&desc);
}

inline sg_pipeline Renderer::CreatePipeline(sg_shader shader, sg_vertex_layout_state layout) {
    sg_pipeline_desc desc = {};
    desc.shader = shader;
    desc.layout = layout;
    desc.index_type = SG_INDEXTYPE_UINT16;
    desc.cull_mode = SG_CULLMODE_BACK;
    desc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.write_enabled = true;
    return sg_make_pipeline(&desc);
}

inline void Renderer::DrawMesh(sg_pipeline pipeline, sg_bindings bindings, int numElements) {
    sg_apply_pipeline(pipeline);
    sg_apply_bindings(&bindings);
    sg_draw(0, numElements, 1);
}

inline void Renderer::DrawMeshWithUniforms(sg_pipeline pipeline, sg_bindings bindings,
                                           const void* uniformData, size_t uniformSize, int numElements) {
    sg_apply_pipeline(pipeline);
    sg_apply_bindings(&bindings);
    sg_range uniform_data = { uniformData, uniformSize };
    sg_apply_uniforms(0, &uniform_data);
    sg_draw(0, numElements, 1);
}

inline void Renderer::CreateCubeMesh(sg_buffer* vbuf, sg_buffer* ibuf, int* numIndices) {
    // Cube vertices (position + color)
    Vertex vertices[] = {
        // Front face (red)
        {{-1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        
        // Back face (green)
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        
        // Top face (blue)
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        
        // Bottom face (yellow)
        {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-1.0f, -1.0f,  1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        
        // Right face (magenta)
        {{ 1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        
        // Left face (cyan)
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f,  1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };
    
    uint16_t indices[] = {
        0, 1, 2,  0, 2, 3,      // Front
        4, 5, 6,  4, 6, 7,      // Back
        8, 9, 10, 8, 10, 11,    // Top
        12, 13, 14, 12, 14, 15, // Bottom
        16, 17, 18, 16, 18, 19, // Right
        20, 21, 22, 20, 22, 23  // Left
    };
    
    *vbuf = CreateVertexBuffer(vertices, 24);
    *ibuf = CreateIndexBuffer(indices, 36);
    *numIndices = 36;
}

inline void Renderer::CreateQuadMesh(sg_buffer* vbuf, sg_buffer* ibuf, int* numIndices) {
    Vertex vertices[] = {
        {{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };
    
    uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };
    
    *vbuf = CreateVertexBuffer(vertices, 4);
    *ibuf = CreateIndexBuffer(indices, 6);
    *numIndices = 6;
}

inline sg_shader Renderer::GetDefaultColorShader() {
    if (mDefaultShader.id != 0) {
        return mDefaultShader;
    }
    
    const char* vs_src = R"(
        #version 330
        uniform mat4 mvp;
        layout(location=0) in vec3 position;
        layout(location=1) in vec4 color0;
        out vec4 color;
        void main() {
            gl_Position = mvp * vec4(position, 1.0);
            color = color0;
        }
    )";
    
    const char* fs_src = R"(
        #version 330
        in vec4 color;
        out vec4 frag_color;
        void main() {
            frag_color = color;
        }
    )";
    
    sg_shader_desc desc = {};
    desc.vertex_func.source = vs_src;
    desc.uniform_blocks[0].stage = SG_SHADERSTAGE_VERTEX;
    desc.uniform_blocks[0].size = sizeof(MVP);
    desc.uniform_blocks[0].glsl_uniforms[0].glsl_name = "mvp";
    desc.uniform_blocks[0].glsl_uniforms[0].type = SG_UNIFORMTYPE_MAT4;
    desc.fragment_func.source = fs_src;
    
    mDefaultShader = sg_make_shader(&desc);
    return mDefaultShader;
}

inline sg_pipeline Renderer::CreateDefaultColorPipeline() {
    if (mDefaultPipeline.id != 0) {
        return mDefaultPipeline;
    }
    
    sg_pipeline_desc desc = {};
    desc.shader = GetDefaultColorShader();
    desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;  // position
    desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;  // color
    desc.index_type = SG_INDEXTYPE_UINT16;
    desc.cull_mode = SG_CULLMODE_BACK;
    desc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.write_enabled = true;
    
    mDefaultPipeline = sg_make_pipeline(&desc);
    return mDefaultPipeline;
}

#endif // RENDERER_H
