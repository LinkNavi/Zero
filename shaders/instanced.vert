#version 450

// Vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

// Instance attributes (model matrix as 4 vec4s + color)
layout(location = 3) in vec4 inModelCol0;
layout(location = 4) in vec4 inModelCol1;
layout(location = 5) in vec4 inModelCol2;
layout(location = 6) in vec4 inModelCol3;
layout(location = 7) in vec4 inColor;

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    vec3 lightDir;
    float ambientStrength;
    vec3 lightColor;
    float padding;
} pc;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out vec3 fragWorldPos;

void main() {
    // Reconstruct model matrix from columns
    mat4 model = mat4(inModelCol0, inModelCol1, inModelCol2, inModelCol3);
    
    vec4 worldPos = model * vec4(inPosition, 1.0);
    gl_Position = pc.viewProj * worldPos;
    
    // Transform normal (simplified - assumes uniform scale)
    mat3 normalMatrix = mat3(model);
    fragNormal = normalize(normalMatrix * inNormal);
    
    fragTexCoord = inTexCoord;
    fragColor = inColor;
    fragWorldPos = worldPos.xyz;
}
