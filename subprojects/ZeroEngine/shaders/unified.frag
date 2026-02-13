#version 450
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec4 fragLightSpacePos;
layout(location = 4) in vec3 fragWorldPos;
layout(location = 0) out vec4 outColor;
layout(set = 0, binding = 0) uniform sampler2D texSampler;
layout(set = 0, binding = 2) uniform sampler2DShadow shadowMap;
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
    vec3 cameraPos;
    float fogDensity;
    vec3 fogColor;
    float fogStart;
    float fogEnd;
    float emissionStrength;
    float useExponentialFog;
    PointLight pointLights[4];
    int numPointLights;
} pc;

float calcShadow(vec4 lightSpacePos) {
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec3 sampleCoord = vec3(projCoords.xy + vec2(x, y) * texelSize, projCoords.z - pc.shadowBias);
            shadow += texture(shadowMap, sampleCoord);
        }
    }
    
    return shadow / 9.0;
}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 worldPos, vec3 viewDir) {
    vec3 lightDir = light.position - worldPos;
    float distance = length(lightDir);
    
    if (distance > light.radius) return vec3(0.0);
    
    lightDir = normalize(lightDir);
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Specular (Blinn-Phong)
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    
    // Attenuation
    float attenuation = light.intensity / (1.0 + distance * distance);
    attenuation *= smoothstep(light.radius, light.radius * 0.5, distance);
    
    return light.color * (diff + spec * 0.5) * attenuation;
}

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 normal = normalize(fragNormal);
    
    // View direction for specular
    vec3 viewDir = normalize(pc.cameraPos - fragWorldPos);
    
    // Directional lighting
    vec3 lightDirNorm = normalize(-pc.lightDir);
    float diff = max(dot(normal, lightDirNorm), 0.0);
    
    // Specular for directional light
    vec3 halfDir = normalize(lightDirNorm + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    
    float shadow = calcShadow(fragLightSpacePos);
    
    vec3 ambient = pc.ambientStrength * pc.lightColor;
    vec3 diffuse = (diff + spec * 0.5) * pc.lightColor * shadow;
    
    // Point lights
    vec3 pointLighting = vec3(0.0);
    for (int i = 0; i < pc.numPointLights && i < 4; i++) {
        pointLighting += calcPointLight(pc.pointLights[i], normal, fragWorldPos, viewDir);
    }
    
    vec3 finalColor = (ambient + diffuse + pointLighting) * texColor.rgb * fragColor.rgb;
    
    // Emission
    vec3 emission = texColor.rgb * texColor.a * pc.emissionStrength;
    finalColor += emission;
    
    // Fog
    float dist = length(fragWorldPos - pc.cameraPos);
    
    float fogFactor;
    if (pc.useExponentialFog > 0.5) {
        fogFactor = exp(-pc.fogDensity * dist);
    } else {
        fogFactor = clamp((pc.fogEnd - dist) / (pc.fogEnd - pc.fogStart), 0.0, 1.0);
    }
    
    finalColor = mix(pc.fogColor, finalColor, fogFactor);
    
    outColor = vec4(finalColor, 1.0);
}
