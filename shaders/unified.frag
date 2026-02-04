#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec4 fragLightSpacePos;
layout(location = 4) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler;
layout(set = 0, binding = 2) uniform sampler2DShadow shadowMap;

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    mat4 model;
    mat4 lightViewProj;
    vec3 lightDir;
    float ambientStrength;
    vec3 lightColor;
    float shadowBias;
};

float calcShadow(vec4 lightSpacePos) {
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    
    // 3x3 PCF
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec3 sampleCoord = vec3(projCoords.xy + vec2(x, y) * texelSize, projCoords.z - shadowBias);
            shadow += texture(shadowMap, sampleCoord);
        }
    }
    
    return shadow / 9.0;
}

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 normal = normalize(fragNormal);
    
    // Lighting
    vec3 lightDirNorm = normalize(-lightDir);
    float diff = max(dot(normal, lightDirNorm), 0.0);
    
    // Shadow
    float shadow = calcShadow(fragLightSpacePos);
    
    vec3 ambient = ambientStrength * lightColor;
    vec3 diffuse = diff * lightColor * shadow;
    
    vec3 finalColor = (ambient + diffuse) * texColor.rgb * fragColor.rgb;
    outColor = vec4(finalColor, texColor.a);
}
