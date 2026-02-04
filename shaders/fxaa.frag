// shaders/fxaa.frag
#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D screenTexture;

const float FXAA_SPAN_MAX = 8.0;
const float FXAA_REDUCE_MUL = 1.0/8.0;
const float FXAA_REDUCE_MIN = 1.0/128.0;

void main() {
    vec2 texelSize = 1.0 / textureSize(screenTexture, 0);
    
    vec3 rgbNW = texture(screenTexture, fragTexCoord + vec2(-1,-1) * texelSize).rgb;
    vec3 rgbNE = texture(screenTexture, fragTexCoord + vec2(1,-1) * texelSize).rgb;
    vec3 rgbSW = texture(screenTexture, fragTexCoord + vec2(-1,1) * texelSize).rgb;
    vec3 rgbSE = texture(screenTexture, fragTexCoord + vec2(1,1) * texelSize).rgb;
    vec3 rgbM  = texture(screenTexture, fragTexCoord).rgb;
    
    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM, luma);
    
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    
    vec2 dir = vec2(
        -((lumaNW + lumaNE) - (lumaSW + lumaSE)),
        ((lumaNW + lumaSW) - (lumaNE + lumaSE))
    );
    
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    
    dir = clamp(dir * rcpDirMin, vec2(-FXAA_SPAN_MAX), vec2(FXAA_SPAN_MAX)) * texelSize;
    
    vec3 rgbA = 0.5 * (
        texture(screenTexture, fragTexCoord + dir * (1.0/3.0 - 0.5)).rgb +
        texture(screenTexture, fragTexCoord + dir * (2.0/3.0 - 0.5)).rgb
    );
    
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(screenTexture, fragTexCoord + dir * -0.5).rgb +
        texture(screenTexture, fragTexCoord + dir * 0.5).rgb
    );
    
    float lumaB = dot(rgbB, luma);
    outColor = ((lumaB < lumaMin) || (lumaB > lumaMax)) ? vec4(rgbA, 1.0) : vec4(rgbB, 1.0);
}
