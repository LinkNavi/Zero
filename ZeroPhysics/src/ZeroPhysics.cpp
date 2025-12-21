#include "ZeroPhysics.h"
#include <cstdlib>
#include <cmath>
World* world_create(size_t capacity, Vec3 gravity) {
    World* w = (World*) std::malloc(sizeof(World));
    if (!w) return nullptr;
    w->bodies = (RigidBody*) std::malloc(sizeof(RigidBody) * capacity);
    if (!w->bodies) { std::free(w); return nullptr; }
    w->capacity = capacity;
    w->count = 0;
    w->gravity = gravity;
    return w;
}
void world_destroy(World* w) {
    if (!w) return;
    std::free(w->bodies);
    std::free(w);
}
size_t world_add_body(World* w, float x, float y, float z,
                      float vx, float vy, float vz,
                      float radius, float inv_mass) {
    if (w->count >= w->capacity) return static_cast<size_t>(-1);
    RigidBody& b = w->bodies[w->count];
    b.pos = {x, y, z};
    b.vel = {vx, vy, vz};
    b.radius = radius;
    b.inv_mass = inv_mass;
    b.rotation = {0,0,0};
    b.angular_velocity = {0,0,0};
    return w->count++;
}
void world_step(World* w, float dt) {
    if (!w || dt <= 0) return;
    const float restitution = 0.5f;
    const float floor_y = 0.0f;
    // integrate velocities & positions
    for (size_t i = 0; i < w->count; ++i) {
        RigidBody& b = w->bodies[i];
        if (b.inv_mass > 0.0f) {
            b.vel.x += w->gravity.x * dt;
            b.vel.y += w->gravity.y * dt;
            b.vel.z += w->gravity.z * dt;
            b.pos.x += b.vel.x * dt;
            b.pos.y += b.vel.y * dt;
            b.pos.z += b.vel.z * dt;
            if (b.pos.y - b.radius < floor_y) {
                b.pos.y = floor_y + b.radius;
                if (b.vel.y < 0.0f) b.vel.y = -b.vel.y * restitution;
            }
        }
    }
    // sphere-sphere collisions
    for (size_t a = 0; a < w->count; ++a) {
        RigidBody& A = w->bodies[a];
        for (size_t b_idx = a + 1; b_idx < w->count; ++b_idx) {
            RigidBody& B = w->bodies[b_idx];
            float dx = B.pos.x - A.pos.x;
            float dy = B.pos.y - A.pos.y;
            float dz = B.pos.z - A.pos.z;
            float dist2 = dx*dx + dy*dy + dz*dz;
            float rsum = A.radius + B.radius;
            if (dist2 <= rsum*rsum && dist2 > 0.0f) {
                float dist = std::sqrt(dist2);
                float penetration = rsum - dist;
                float nx = dx / dist;
                float ny = dy / dist;
                float nz = dz / dist;
                float invMassSum = A.inv_mass + B.inv_mass;
                if (invMassSum > 0.0f) {
                    float px = nx * penetration * 0.5f;
                    float py = ny * penetration * 0.5f;
                    float pz = nz * penetration * 0.5f;
                    if (A.inv_mass > 0.0f) {
                        A.pos.x -= px * (A.inv_mass / invMassSum);
                        A.pos.y -= py * (A.inv_mass / invMassSum);
                        A.pos.z -= pz * (A.inv_mass / invMassSum);
                    }
                    if (B.inv_mass > 0.0f) {
                        B.pos.x += px * (B.inv_mass / invMassSum);
                        B.pos.y += py * (B.inv_mass / invMassSum);
                        B.pos.z += pz * (B.inv_mass / invMassSum);
                    }
                }
                float rvx = B.vel.x - A.vel.x;
                float rvy = B.vel.y - A.vel.y;
                float rvz = B.vel.z - A.vel.z;
                float relVel = rvx*nx + rvy*ny + rvz*nz;
                if (relVel < 0) {
                    float j = -(1.0f + restitution) * relVel / invMassSum;
                    float jx = j * nx;
                    float jy = j * ny;
                    float jz = j * nz;
                    if (A.inv_mass > 0.0f) { A.vel.x -= jx * A.inv_mass; A.vel.y -= jy * A.inv_mass; A.vel.z -= jz * A.inv_mass; }
                    if (B.inv_mass > 0.0f) { B.vel.x += jx * B.inv_mass; B.vel.y += jy * B.inv_mass; B.vel.z += jz * B.inv_mass; }
                }
            }
        }
    }
}
