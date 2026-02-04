// shaders/skybox.frag
#version 450

layout(location = 0) in vec3 texCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform samplerCube skybox;

void main() {
    fragColor = texture(skybox, texCoord);
}
