#define SOKOL_IMPL
#define SOKOL_GLCORE
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_glue.h"

#include "Renderer.h"
#include <cmath>


void createMVP(float* mvp, float angle, float aspect) {
    // Simple identity matrix for now
    for (int i = 0; i < 16; i++) mvp[i] = 0.0f;
    mvp[0] = mvp[5] = mvp[10] = mvp[15] = 1.0f;
}

int main() {
    Renderer renderer;
    renderer.Init();
    
    // Create a cube
    sg_buffer vbuf, ibuf;
    int numIndices;
    renderer.CreateCubeMesh(&vbuf, &ibuf, &numIndices);
    
    // Create pipeline
    sg_pipeline pipeline = renderer.CreateDefaultColorPipeline();
    
    // Setup bindings
    sg_bindings bindings = {};
    bindings.vertex_buffers[0] = vbuf;
    bindings.index_buffer = ibuf;
    
    // Render loop
    float angle = 0.0f;
    bool running = true;  // Add this line
    
    while (running) {
        renderer.BeginFrame();
        renderer.Clear(0.1f, 0.1f, 0.1f);
        
        // Update MVP matrix
        MVP mvp;
        createMVP(mvp.mvp, angle, renderer.GetAspectRatio());
        
        // Draw
        renderer.DrawMeshWithUniforms(pipeline, bindings, &mvp, sizeof(MVP), numIndices);
        
        renderer.EndFrame();
        angle += 0.01f;
        
        // You'll need proper window event handling here
        // For now, just run once or add your own loop control
        running = false;
    }
    
    renderer.Shutdown();
    return 0;
}
