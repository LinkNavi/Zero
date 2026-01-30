#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragColor;

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
    // Sample texture only once
    vec4 texColor = texture(texSampler, fragTexCoord);
    
    // Fast half-lambert for softer lighting without normalize
    float NdotL = dot(fragNormal, pc.lightDir) * 0.5 + 0.5;
    float light = pc.ambientStrength + NdotL * (1.0 - pc.ambientStrength);
    
    outColor = vec4(texColor.rgb * fragColor.rgb * light * pc.lightColor, 1.0);
}
