#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

// Texture sampler
layout(binding = 0) uniform sampler2D texSampler;

// Push constants
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
    int hasBaseColorTexture;  // 0 = use material color, 1 = use texture
    int padding2;
} push;

void main() {
    // Sample texture or use material color
    vec4 baseColor;
    if (push.hasBaseColorTexture > 0) {
        baseColor = texture(texSampler, fragTexCoord) * push.materialColor;
    } else {
        baseColor = push.materialColor;
    }
    
    // Normalize the normal
    vec3 norm = normalize(fragNormal);
    vec3 lightDirection = normalize(push.lightDir);
    
    // Ambient
    vec3 ambient = push.ambientStrength * push.lightColor;
    
    // Diffuse
    float diff = max(dot(norm, -lightDirection), 0.0);
    vec3 diffuse = diff * push.lightColor;
    
    // Simple approximation of metallic/roughness
    float specularFactor = (1.0 - push.materialRoughness) * push.materialMetallic;
    vec3 specular = vec3(specularFactor * diff);
    
    // Combine
    vec3 result = (ambient + diffuse + specular) * baseColor.rgb;
    
    outColor = vec4(result, baseColor.a);
}
