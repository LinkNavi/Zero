#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D hdrTexture;

layout(push_constant) uniform PushConstants {
    float threshold;
    float intensity;
    int horizontal;
    float padding;
} pc;

void main() {
    vec3 color = texture(hdrTexture, fragTexCoord).rgb;
    
    // Calculate luminance
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    
    // Extract bright pixels above threshold
    float brightness = max(luma - pc.threshold, 0.0);
    vec3 bloom = color * (brightness / max(luma, 0.0001));
    
    outColor = vec4(bloom * pc.intensity, 1.0);
}
