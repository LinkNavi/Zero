#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragWorldPos;
layout(location = 4) in vec4 fragLightSpacePos;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D shadowMap;

struct PointLight {
    vec3 position;
    float radius;
    vec3 color;
    float intensity;
};

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    mat4 model;
    mat4 lightViewProj;
    vec3 lightDir;
    float ambientStrength;
    vec3 lightColor;
    float shadowBias;
    vec3 fogColor;
    float fogDensity;
    float fogStart;
    float fogEnd;
    float emissionStrength;
    float useExponentialFog;
    PointLight pointLights[4];
    int numPointLights;
} pc;

float calculateShadow(vec4 lightSpacePos, vec3 normal, vec3 lightDir) {
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    
    if(projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
       projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }
    
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, -lightDir)), pc.shadowBias);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 0.0 : 1.0;
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

vec3 calculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 baseColor) {
    vec3 lightDir = light.position - fragPos;
    float distance = length(lightDir);
    
    if(distance > light.radius) return vec3(0.0);
    
    lightDir = normalize(lightDir);
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Attenuation
    float attenuation = 1.0 - (distance / light.radius);
    attenuation = attenuation * attenuation;
    
    vec3 diffuse = diff * light.color * light.intensity * attenuation;
    
    return diffuse * baseColor;
}

float calculateFog(float distance) {
    if(pc.useExponentialFog > 0.5) {
        // Exponential fog
        return 1.0 - exp(-pc.fogDensity * distance);
    } else {
        // Linear fog
        return clamp((distance - pc.fogStart) / (pc.fogEnd - pc.fogStart), 0.0, 1.0);
    }
}

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 baseColor = texColor.rgb * fragColor;
    
    vec3 normal = normalize(fragNormal);
    
    // Directional light
    float diff = max(dot(normal, -pc.lightDir), 0.0);
    float shadow = calculateShadow(fragLightSpacePos, normal, pc.lightDir);
    vec3 directionalLight = (pc.ambientStrength + diff * shadow) * pc.lightColor;
    
    // Point lights
    vec3 pointLightContribution = vec3(0.0);
    for(int i = 0; i < pc.numPointLights && i < 4; ++i) {
        pointLightContribution += calculatePointLight(pc.pointLights[i], normal, fragWorldPos, baseColor);
    }
    
    // Combine lighting
    vec3 litColor = baseColor * directionalLight + pointLightContribution;
    
    // Add emission
    litColor += baseColor * pc.emissionStrength;
    
    // Apply fog
    float distance = length(fragWorldPos);
    float fogFactor = calculateFog(distance);
    vec3 finalColor = mix(litColor, pc.fogColor, fogFactor);
    
    outColor = vec4(finalColor, texColor.a);
}
