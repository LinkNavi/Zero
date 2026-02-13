#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D sceneTex;
layout(binding = 1) uniform sampler2D bloomTex;

layout(push_constant) uniform PC {
    float strength;
    float exposure;
    float gamma;
    float bloomEnabled;
};

void main() {
    vec3 scene = texture(sceneTex, uv).rgb;
    vec3 bloom = texture(bloomTex, uv).rgb;
    
    // Add bloom if enabled
    vec3 result = scene;
    if (bloomEnabled > 0.5) {
        result += bloom * strength;
    }
    
    // Tone mapping (ACES approximation)
    result *= exposure;
    result = (result * (2.51 * result + 0.03)) / (result * (2.43 * result + 0.59) + 0.14);
    
    // Gamma correction
    result = pow(clamp(result, 0.0, 1.0), vec3(1.0 / gamma));
    
    outColor = vec4(result, 1.0);
}
