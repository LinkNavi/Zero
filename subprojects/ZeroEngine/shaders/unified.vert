#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;
layout(location = 4) in ivec4 inBoneIds;
layout(location = 5) in vec4 inBoneWeights;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out vec4 fragLightSpacePos;
layout(location = 4) out vec3 fragWorldPos;

layout(set = 0, binding = 1) uniform BoneBuffer {
    mat4 bones[128];
};

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    mat4 model;
    mat4 lightViewProj;
    vec3 lightDir;
    float ambientStrength;
    vec3 lightColor;
    float shadowBias;
};

void main() {
    vec4 pos = vec4(inPosition, 1.0);
    vec4 norm = vec4(inNormal, 0.0);
    
    float totalWeight = inBoneWeights.x + inBoneWeights.y + inBoneWeights.z + inBoneWeights.w;
    if (totalWeight > 0.01) {
        mat4 skinMatrix = 
            bones[inBoneIds.x] * inBoneWeights.x +
            bones[inBoneIds.y] * inBoneWeights.y +
            bones[inBoneIds.z] * inBoneWeights.z +
            bones[inBoneIds.w] * inBoneWeights.w;
        pos = skinMatrix * pos;
        norm = skinMatrix * norm;
    }
    
    vec4 worldPos = model * pos;
    fragWorldPos = worldPos.xyz;
    fragTexCoord = inTexCoord;
    fragNormal = normalize(mat3(model) * norm.xyz);
    fragColor = inColor;
    fragLightSpacePos = lightViewProj * worldPos;
    
    gl_Position = viewProj * worldPos;
}
