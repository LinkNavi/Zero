#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;
    float lifetime;
    float size;
};

struct ParticleEmitterConfig {
    glm::vec3 position = glm::vec3(0);
    glm::vec3 direction = glm::vec3(0, 1, 0);
    float spread = 0.3f;
    float spawnRate = 10.0f;
    float particleLifetime = 2.0f;
    float particleSpeed = 5.0f;
    float particleSize = 0.1f;
    glm::vec4 startColor = glm::vec4(1, 1, 1, 1);
    glm::vec4 endColor = glm::vec4(1, 1, 1, 0);
    glm::vec3 gravity = glm::vec3(0, -9.8f, 0);
    int maxParticles = 1000;
};

class ParticleEmitter {
    std::vector<Particle> particles;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation vertexAllocation;
    VmaAllocator allocator;
    
    float spawnTimer = 0.0f;
    
public:
    ParticleEmitterConfig config;
    
    bool init(VmaAllocator alloc);
    void update(float dt);
    void draw(VkCommandBuffer cmd);
    void cleanup();
    
private:
    void spawnParticle();
    void updateBuffer();
};
