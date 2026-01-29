// GLB vertex shader
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragWorldPos;
layout(location = 2) out vec2 fragTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    mat4 model;
    vec3 lightDir;
    float padding;
    vec3 lightColor;
    float ambientStrength;
} push;

void main() {
    gl_Position = push.mvp * vec4(inPosition, 1.0);
    fragNormal = mat3(push.model) * inNormal;
    fragWorldPos = vec3(push.model * vec4(inPosition, 1.0));
    fragTexCoord = inTexCoord;
}
