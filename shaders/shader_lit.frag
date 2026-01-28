// Lit fragment shader with directional light
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    mat4 model;
    vec3 lightDir;
    float padding;
    vec3 lightColor;
    float ambientStrength;
} push;

void main() {
    // Normalize inputs
    vec3 norm = normalize(fragNormal);
    vec3 lightDirection = normalize(-push.lightDir);
    
    // Ambient
    vec3 ambient = push.ambientStrength * push.lightColor;
    
    // Diffuse
    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = diff * push.lightColor;
    
    // Combine
    vec3 result = (ambient + diffuse) * fragColor;
    outColor = vec4(result, 1.0);
}
