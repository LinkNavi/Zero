#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;
layout(location = 4) in ivec4 inBoneIds;
layout(location = 5) in vec4 inBoneWeights;

layout(set = 0, binding = 1) uniform BoneBuffer {
    mat4 bones[128];
};

layout(push_constant) uniform PushConstants {
    mat4 lightViewProj;
    mat4 model;
};

void main() {
    vec4 pos = vec4(inPosition, 1.0);
    
    float totalWeight = inBoneWeights.x + inBoneWeights.y + inBoneWeights.z + inBoneWeights.w;
    if (totalWeight > 0.01) {
        mat4 skinMatrix = 
            bones[inBoneIds.x] * inBoneWeights.x +
            bones[inBoneIds.y] * inBoneWeights.y +
            bones[inBoneIds.z] * inBoneWeights.z +
            bones[inBoneIds.w] * inBoneWeights.w;
        pos = skinMatrix * pos;
    }
    
    gl_Position = lightViewProj * model * pos;
}
