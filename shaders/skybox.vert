// shaders/skybox.vert
#version 450

layout(location = 0) in vec3 aPos;
layout(location = 0) out vec3 texCoord;

layout(binding = 0) uniform UBO {
    mat4 view;
    mat4 projection;
} ubo;

void main() {
    texCoord = aPos;
    vec4 pos = ubo.projection * ubo.view * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // Trick: set z=w so depth is always 1.0
}
