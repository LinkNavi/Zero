#pragma once
#include <cstddef>
struct Vec3 {
    float x, y, z;
};
struct RigidBody {
    Vec3 pos;
    Vec3 vel;
    float radius; // Sphere radius
    float inv_mass; // 0 = static body
    Vec3 angular_velocity; // optional for rotation
    Vec3 rotation; // euler angles in radians
};
struct Box {
    Vec3 pos;
    Vec3 half_extents; // half width, height, depth
    float mass;
    float inv_mass;
    Vec3 vel;
    Vec3 angular_velocity; // rotation rate
    // Orientation matrix or quaternion
    float rotation[3][3];
};
struct World {
    RigidBody* bodies;
    size_t capacity;
    size_t count;
    Vec3 gravity;
};
inline Vec3 vecAddPub(const Vec3& a, const Vec3& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
// Create/destroy world
World* world_create(size_t capacity, Vec3 gravity);
void world_destroy(World* w);
// Add a body
size_t world_add_body(World* w, float x, float y, float z,
                      float vx, float vy, float vz,
                      float radius, float inv_mass);
// Step simulation
void world_step(World* w, float dt);
