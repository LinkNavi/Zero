#version 450

layout(binding = 0) uniform UBO {
    mat4 view;
    mat4 projection;
} ubo;

layout(location = 0) in vec3 inPos;
layout(location = 0) out vec3 fragTexCoord;

void main() {
    fragTexCoord = inPos;
    vec4 pos = ubo.projection * ubo.view * vec4(inPos, 1.0);
    gl_Position = pos.xyww;
}
