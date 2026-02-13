#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D inputTexture;

layout(push_constant) uniform PushConstants {
    float threshold;
    float intensity;
    int horizontal;
    float padding;
} pc;

const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 texelSize = 1.0 / textureSize(inputTexture, 0);
    vec3 result = texture(inputTexture, fragTexCoord).rgb * weights[0];
    
    if (pc.horizontal == 1) {
        for (int i = 1; i < 5; i++) {
            result += texture(inputTexture, fragTexCoord + vec2(texelSize.x * i, 0.0)).rgb * weights[i];
            result += texture(inputTexture, fragTexCoord - vec2(texelSize.x * i, 0.0)).rgb * weights[i];
        }
    } else {
        for (int i = 1; i < 5; i++) {
            result += texture(inputTexture, fragTexCoord + vec2(0.0, texelSize.y * i)).rgb * weights[i];
            result += texture(inputTexture, fragTexCoord - vec2(0.0, texelSize.y * i)).rgb * weights[i];
        }
    }
    
    outColor = vec4(result, 1.0);
}
