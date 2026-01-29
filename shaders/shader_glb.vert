#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragWorldPos;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    mat4 model;
    vec3 lightDir;
    float padding1;
    vec3 lightColor;
    float ambientStrength;
    vec4 materialColor;
    float materialMetallic;
    float materialRoughness;
    int hasBaseColorTexture;
    int padding2;
} push;

void main() {
    gl_Position = push.mvp * vec4(inPosition, 1.0);
    
    // Transform normal to world space
    fragNormal = mat3(push.model) * inNormal;
    
    // Pass through texture coordinates
    fragTexCoord = inTexCoord;
    
    // World position
    fragWorldPos = vec3(push.model * vec4(inPosition, 1.0));
}
