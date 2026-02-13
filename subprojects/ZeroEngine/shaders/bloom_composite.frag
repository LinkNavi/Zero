#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D hdrScene;
layout(binding = 1) uniform sampler2D bloomTexture;

layout(push_constant) uniform PushConstants {
    float bloomStrength;
    float exposure;
    float gamma;
    float padding;
} pc;

void main() {
    vec3 hdrColor = texture(hdrScene, fragTexCoord).rgb;
    vec3 bloom = texture(bloomTexture, fragTexCoord).rgb;
    
    // Add bloom
    vec3 result = hdrColor + bloom * pc.bloomStrength;
    
    // Tone mapping (Reinhard)
    result = vec3(1.0) - exp(-result * pc.exposure);
    
    // Gamma correction
    result = pow(result, vec3(1.0 / pc.gamma));
    
    outColor = vec4(result, 1.0);
}
