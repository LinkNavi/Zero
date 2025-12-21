#pragma once
#include <vector>
#include <cstddef>
#include "ZeroPhysics.h" // Vec3 etc.
#include "raylib.h"
// --- Basic triangle + physics mesh ---
struct Triangle {
    Vec3 v0, v1, v2;
};
struct PhysicsMesh {
    std::vector<Triangle> triangles; // triangles in local/model space
    // BVH data filled after buildBVH()
    struct BVHNode {
        Vec3 aabb_min;
        Vec3 aabb_max;
        int left; // child index or -1
        int right; // child index or -1
        int start; // start triangle index in 'tri_indices' (leaf)
        int count; // number of triangles in leaf (leaf if count > 0)
    };
    std::vector<BVHNode> nodes;
    std::vector<int> tri_indices; // indices into triangles vector laid out for leaves
};
// Collider instance (per-object)
struct MeshCollider {
    PhysicsMesh* mesh;
    Vec3 pos; // world-space translation (no rotation for now)
    Vec3 vel;
    float inv_mass;
};
// API
PhysicsMesh physicsMeshFromModel(const Model& m);
void buildBVH(PhysicsMesh& mesh, int leaf_tri_count = 8);
// narrowphase collision (returns true if collision found; contact_normal is approx, penetration > 0)
bool triangleTriangleIntersect(const Triangle& A, const Triangle& B, Vec3& contact_normal, float& penetration);
bool meshMeshCollision(const MeshCollider& a, const MeshCollider& b, Vec3& contact_normal, float& penetration);
// Integrate & solve collisions for a small set of colliders
void meshStep(MeshCollider* colliders, size_t count, float dt, Vec3 gravity);
// helper to convert Vec3 -> Raylib Vector3
inline Vector3 toVector3(const Vec3& v) { Vector3 r; r.x = v.x; r.y = v.y; r.z = v.z; return r; }
