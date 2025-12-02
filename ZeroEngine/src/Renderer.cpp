#include "Renderer.h"
#include <cstring>

Renderer::~Renderer() {
    if (mInitialized) {
        Shutdown();
    }
}

void Renderer::Init() {
    sg_desc desc = {};
    desc.environment = sglue_environment();
    sg_setup(&desc);
    
    mPassAction.colors[0].load_action = SG_LOADACTION_CLEAR;
    mPassAction.colors[0].clear_value = { 0.1f, 0.1f, 0.1f, 1.0f };
    
    mInitialized = true;
}

void Renderer::Shutdown() {
    if (mDefaultShader.id != 0) {
        sg_destroy_shader(mDefaultShader);
    }
    if (mDefaultPipeline.id != 0) {
        sg_destroy_pipeline(mDefaultPipeline);
    }
    sg_shutdown();
    mInitialized = false;
}

void Renderer::BeginFrame() {
    sg_pass pass = {};
    pass.action = mPassAction;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);
}

void Renderer::EndFrame() {
    sg_end_pass();
    sg_commit();
}

void Renderer::Clear(float r, float g, float b, float a) {
    mPassAction.colors[0].clear_value = { r, g, b, a };
}

sg_buffer Renderer::CreateVertexBuffer(const Vertex* vertices, size_t count) {
    sg_buffer_desc desc = {};
    desc.data.ptr = vertices;
    desc.data.size = count * sizeof(Vertex);
    return sg_make_buffer(&desc);
}

sg_buffer Renderer::CreateIndexBuffer(const uint16_t* indices, size_t count) {
    sg_buffer_desc desc = {};
    desc.data.ptr = indices;
    desc.data.size = count * sizeof(uint16_t);
    return sg_make_buffer(&desc);
}

sg_shader Renderer::CreateShader(const char* vs_src, const char* fs_src) {
    sg_shader_desc desc = {};
    desc.vertex_func.source = vs_src;
    desc.fragment_func.source = fs_src;
    return sg_make_shader(&desc);
}

sg_pipeline Renderer::CreatePipeline(sg_shader shader, sg_vertex_layout_state layout) {
    sg_pipeline_desc desc = {};
    desc.shader = shader;
    desc.layout = layout;
    desc.index_type = SG_INDEXTYPE_UINT16;
    desc.cull_mode = SG_CULLMODE_BACK;
    desc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.write_enabled = true;
    return sg_make_pipeline(&desc);
}

void Renderer::DrawMesh(sg_pipeline pipeline, sg_bindings bindings, int numElements) {
    sg_apply_pipeline(pipeline);
    sg_apply_bindings(&bindings);
    sg_draw(0, numElements, 1);
}

void Renderer::DrawMeshWithUniforms(sg_pipeline pipeline, sg_bindings bindings,
                                    const void* uniformData, size_t uniformSize, int numElements) {
    sg_apply_pipeline(pipeline);
    sg_apply_bindings(&bindings);
    sg_range uniform_data = { uniformData, uniformSize };
    sg_apply_uniforms(0, &uniform_data);
    sg_draw(0, numElements, 1);
}

void Renderer::CreateCubeMesh(sg_buffer* vbuf, sg_buffer* ibuf, int* numIndices) {
    Vertex vertices[] = {
        {{-1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        
        {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-1.0f, -1.0f,  1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        
        {{ 1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f,  1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };
    
    uint16_t indices[] = {
        0, 1, 2,  0, 2, 3,
        4, 5, 6,  4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23
    };
    
    *vbuf = CreateVertexBuffer(vertices, 24);
    *ibuf = CreateIndexBuffer(indices, 36);
    *numIndices = 36;
}

void Renderer::CreateQuadMesh(sg_buffer* vbuf, sg_buffer* ibuf, int* numIndices) {
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

sg_shader Renderer::GetDefaultColorShader() {
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

sg_pipeline Renderer::CreateDefaultColorPipeline() {
    if (mDefaultPipeline.id != 0) {
        return mDefaultPipeline;
    }
    
    sg_pipeline_desc desc = {};
    desc.shader = GetDefaultColorShader();
    desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
    desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;
    desc.index_type = SG_INDEXTYPE_UINT16;
    desc.cull_mode = SG_CULLMODE_BACK;
    desc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth.write_enabled = true;
    
    mDefaultPipeline = sg_make_pipeline(&desc);
    return mDefaultPipeline;
}
