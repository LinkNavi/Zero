// Lit vertex shader with normals
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    mat4 model;
} push;

void main() {
    gl_Position = push.mvp * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragNormal = mat3(push.model) * inNormal;
    fragWorldPos = vec3(push.model * vec4(inPosition, 1.0));
}
