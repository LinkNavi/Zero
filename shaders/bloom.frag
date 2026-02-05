#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D sceneTex;

layout(push_constant) uniform PC {
    float threshold;
    float intensity;
    float texelX;
    float texelY;
};

void main() {
    // 5-tap box blur + threshold in one pass
    vec2 off = vec2(texelX, texelY) * 2.0;
    
    vec3 c = texture(sceneTex, uv).rgb;
    c += texture(sceneTex, uv + vec2(off.x, 0)).rgb;
    c += texture(sceneTex, uv - vec2(off.x, 0)).rgb;
    c += texture(sceneTex, uv + vec2(0, off.y)).rgb;
    c += texture(sceneTex, uv - vec2(0, off.y)).rgb;
    c *= 0.2;
    
    // Threshold
    float luma = dot(c, vec3(0.299, 0.587, 0.114));
    float bloom = max(luma - threshold, 0.0);
    c *= bloom / max(luma, 0.001) * intensity;
    
    outColor = vec4(c, 1.0);
}
