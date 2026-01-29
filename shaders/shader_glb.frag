// GLB fragment shader with material support
#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    mat4 model;
    vec3 lightDir;
    float padding;
    vec3 lightColor;
    float ambientStrength;
    vec4 materialColor;  // Material base color
    float materialMetallic;
    float materialRoughness;
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
    
    // Use material color if provided, otherwise use texture coordinates as color
    vec3 baseColor = push.materialColor.rgb;
    if (push.materialColor.a < 0.01) {
        // No material color set, use procedural coloring based on normals and position
        baseColor = abs(norm) * 0.5 + 0.5;
    }
    
    // Simple metallic/roughness visualization
    float specular = pow(max(dot(reflect(-lightDirection, norm), normalize(-fragWorldPos)), 0.0), 
                        32.0 * (1.0 - push.materialRoughness));
    vec3 specularColor = push.lightColor * specular * push.materialMetallic;
    
    // Combine
    vec3 result = (ambient + diffuse) * baseColor + specularColor;
    outColor = vec4(result, 1.0);
}
