#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"

#include "Renderer.h"
#include <cmath>

// Global state
static struct {
    Renderer renderer;
    sg_buffer vbuf;
    sg_buffer ibuf;
    sg_pipeline pipeline;
    sg_bindings bindings;
    int numIndices;
    float angle;
} state;

// Helper to multiply 4x4 matrices
void mat4_multiply(const float* a, const float* b, float* out) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            out[i * 4 + j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                out[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
            }
        }
    }
}

// Create perspective projection matrix
void mat4_perspective(float* m, float fov, float aspect, float near_plane, float far_plane) {
    float f = 1.0f / tanf(fov / 2.0f);
    for (int i = 0; i < 16; i++) m[i] = 0.0f;
    
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (far_plane + near_plane) / (near_plane - far_plane);
    m[11] = -1.0f;
    m[14] = (2.0f * far_plane * near_plane) / (near_plane - far_plane);
}

// Create view matrix (lookAt)
void mat4_lookat(float* m, float eye_x, float eye_y, float eye_z) {
    // Simple view matrix looking at origin from eye position
    for (int i = 0; i < 16; i++) m[i] = 0.0f;
    m[0] = m[5] = m[10] = m[15] = 1.0f;
    m[12] = -eye_x;
    m[13] = -eye_y;
    m[14] = -eye_z;
}

// Create rotation matrix around Y axis
void mat4_rotate_y(float* m, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    
    for (int i = 0; i < 16; i++) m[i] = 0.0f;
    m[0] = c;
    m[2] = s;
    m[5] = 1.0f;
    m[8] = -s;
    m[10] = c;
    m[15] = 1.0f;
}

void createMVP(float* mvp, float angle, float aspect) {
    float proj[16];
    float view[16];
    float model[16];
    float temp[16];
    
    // Create projection matrix (FOV 60 degrees)
    mat4_perspective(proj, 3.14159f / 3.0f, aspect, 0.1f, 100.0f);
    
    // Create view matrix (camera at 0, 0, 5 looking at origin)
    mat4_lookat(view, 0.0f, 0.0f, 5.0f);
    
    // Create model matrix (rotation around Y axis)
    mat4_rotate_y(model, angle);
    
    // Combine: MVP = projection * view * model
    mat4_multiply(view, model, temp);
    mat4_multiply(proj, temp, mvp);
}

static void init(void) {
    // Initialize renderer
    state.renderer.Init();
    
    // Create a cube
    state.renderer.CreateCubeMesh(&state.vbuf, &state.ibuf, &state.numIndices);
    
    // Create pipeline
    state.pipeline = state.renderer.CreateDefaultColorPipeline();
    
    // Setup bindings
    state.bindings.vertex_buffers[0] = state.vbuf;
    state.bindings.index_buffer = state.ibuf;
    
    state.angle = 0.0f;
}

static void frame(void) {
    state.renderer.BeginFrame();
    state.renderer.Clear(0.1f, 0.1f, 0.1f);
    
    // Update MVP matrix
    MVP mvp;
    createMVP(mvp.mvp, state.angle, state.renderer.GetAspectRatio());
    
    // Draw
    state.renderer.DrawMeshWithUniforms(state.pipeline, state.bindings, &mvp, sizeof(MVP), state.numIndices);
    
    state.renderer.EndFrame();
    state.angle += 0.01f;
}

static void cleanup(void) {
    state.renderer.Shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    sapp_desc desc = {};
    desc.init_cb = init;
    desc.frame_cb = frame;
    desc.cleanup_cb = cleanup;
    desc.width = 800;
    desc.height = 600;
    desc.window_title = "ZeroEngine";
    desc.logger.func = slog_func;
    
    return desc;
}
