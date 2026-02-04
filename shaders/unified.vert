#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;
layout(location = 4) in ivec4 inBoneIds;
layout(location = 5) in vec4 inBoneWeights;

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    mat4 model;
    vec3 lightDir;
    float ambientStrength;
    vec3 lightColor;
    float padding;
} pc;

layout(set = 0, binding = 1) uniform BoneBuffer {
    mat4 bones[128];
};

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragColor;

void main() {
    mat4 skinMatrix = mat4(0.0);
    float totalWeight = 0.0;
    
    for (int i = 0; i < 4; i++) {
        if (inBoneIds[i] >= 0 && inBoneWeights[i] > 0.0) {
            skinMatrix += bones[inBoneIds[i]] * inBoneWeights[i];
            totalWeight += inBoneWeights[i];
        }
    }
    
    // If no bones, use identity
    if (totalWeight < 0.001) {
        skinMatrix = mat4(1.0);
    }
    
    vec4 skinnedPos = skinMatrix * vec4(inPosition, 1.0);
    vec3 skinnedNormal = mat3(skinMatrix) * inNormal;
    
    gl_Position = pc.viewProj * pc.model * skinnedPos;
    fragNormal = mat3(pc.model) * skinnedNormal;
    fragTexCoord = inTexCoord;
    fragColor = inColor;
}
