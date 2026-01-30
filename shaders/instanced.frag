#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    vec3 lightDir;
    float ambientStrength;
    vec3 lightColor;
    float padding;
} pc;

void main() {
    // Sample texture
    vec4 texColor = texture(texSampler, fragTexCoord);
    
    // Simple diffuse lighting
    vec3 normal = normalize(fragNormal);
    vec3 lightDirection = normalize(pc.lightDir);
    
    float diff = max(dot(normal, lightDirection), 0.0);
    vec3 diffuse = diff * pc.lightColor;
    vec3 ambient = pc.ambientStrength * pc.lightColor;
    
    // Combine: texture * instance color * lighting
    vec3 lighting = ambient + diffuse;
    vec3 finalColor = texColor.rgb * fragColor.rgb * lighting;
    
    outColor = vec4(finalColor, texColor.a * fragColor.a);
}
