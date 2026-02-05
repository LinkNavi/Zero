#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D bloomTex;

layout(push_constant) uniform PC {
    float strength;
    float exposure;
    float gamma;
    float pad;
};

void main() {
    vec3 bloom = texture(bloomTex, uv).rgb * strength;
    outColor = vec4(bloom, 1.0);
}
