#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D albedoMap;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProj;
    vec3 lightDir;
    float ambientStrength;
    vec3 lightColor;
    float padding;
} pc;

void main() {
    vec4 albedo = texture(albedoMap, fragTexCoord) * fragColor;
    
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(pc.lightDir);
    
    // Simple diffuse lighting
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = NdotL * pc.lightColor;
    vec3 ambient = pc.ambientStrength * pc.lightColor;
    
    vec3 finalColor = albedo.rgb * (ambient + diffuse);
    outColor = vec4(finalColor, albedo.a);
}
