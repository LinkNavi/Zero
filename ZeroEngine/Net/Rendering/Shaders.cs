using OpenTK.Mathematics;

namespace EngineCore.Rendering
{
    /// <summary>
    /// Improved shaders with better lighting
    /// </summary>
    public static class ImprovedShaders
    {
        public static string VertexShader = @"
#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aColor;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 normalMatrix;

void main()
{
    vec4 worldPos = model * vec4(aPosition, 1.0);
    FragPos = worldPos.xyz;
    
    // Transform normal to world space properly
    Normal = normalize(mat3(normalMatrix) * aNormal);
    
    TexCoord = aTexCoord;
    Color = aColor;
    
    gl_Position = projection * view * worldPos;
}
";

        public static string FragmentShader = @"
#version 330 core

struct Light {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    int type; // 0=Directional, 1=Point, 2=Spot
};

struct Material {
    vec4 albedo;
    float metallic;
    float roughness;
    float shininess;
    vec3 emission;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 Color;

out vec4 FragColor;

uniform Material material;
uniform vec3 viewPos;

#define MAX_LIGHTS 8
uniform Light lights[MAX_LIGHTS];
uniform int numLights;

vec3 calculateDirectionalLight(Light light, vec3 normal, vec3 viewDir, vec3 albedo)
{
    vec3 lightDir = normalize(-light.direction);
    
    // Ambient
    vec3 ambient = 0.2 * albedo * light.color;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * albedo * light.color;
    
    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    vec3 specular = vec3(spec) * (1.0 - material.roughness) * light.color;
    
    return (ambient + diffuse + specular) * light.intensity;
}

vec3 calculatePointLight(Light light, vec3 normal, vec3 viewDir, vec3 albedo)
{
    vec3 lightDir = normalize(light.position - FragPos);
    float distance = length(light.position - FragPos);
    
    // Attenuation
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    
    // Ambient
    vec3 ambient = 0.1 * albedo * light.color;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * albedo * light.color;
    
    // Specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    vec3 specular = vec3(spec) * (1.0 - material.roughness) * light.color;
    
    return (ambient + diffuse + specular) * attenuation * light.intensity;
}

void main()
{
    // Get albedo
    vec3 albedo = material.albedo.rgb * Color.rgb;
    
    // Normalize normal (interpolation can denormalize it)
    vec3 normal = normalize(Normal);
    
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Calculate lighting
    vec3 result = material.emission;
    
    // Add contribution from all lights
    for (int i = 0; i < numLights && i < MAX_LIGHTS; i++)
    {
        if (lights[i].type == 0) // Directional
            result += calculateDirectionalLight(lights[i], normal, viewDir, albedo);
        else if (lights[i].type == 1) // Point
            result += calculatePointLight(lights[i], normal, viewDir, albedo);
    }
    
    // Add small ambient if no lights
    if (numLights == 0)
        result += albedo * 0.3;
    
    FragColor = vec4(result, material.albedo.a * Color.a);
}
";
    }
}
