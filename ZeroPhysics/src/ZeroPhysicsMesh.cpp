#include "ZeroPhysicsMesh.h"
#include <cmath>
#include <algorithm>
#include <limits>

// --- helpers ------------------------------------------------

static inline Vec3 vecAdd(const Vec3& a, const Vec3& b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
static inline Vec3 vecSub(const Vec3& a, const Vec3& b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
static inline Vec3 vecMul(const Vec3& a, float s) { return {a.x*s, a.y*s, a.z*s}; }
static inline float vecDot(const Vec3& a, const Vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline Vec3 vecCross(const Vec3& a, const Vec3& b) {
    return { a.y*b.z - a.z*b.y,
             a.z*b.x - a.x*b.z,
             a.x*b.y - a.y*b.x };
}
static inline float vecLen(const Vec3& a) { return std::sqrt(vecDot(a,a)); }
static inline Vec3 vecNormalize(const Vec3& a) {
    float l = vecLen(a);
    if (l <= 1e-9f) return {0,0,0};
    return vecMul(a, 1.0f / l);
}

// expand an AABB with a point
static void aabbExpand(Vec3& mn, Vec3& mx, const Vec3& p) {
    mn.x = std::min(mn.x, p.x); mn.y = std::min(mn.y, p.y); mn.z = std::min(mn.z, p.z);
    mx.x = std::max(mx.x, p.x); mx.y = std::max(mx.y, p.y); mx.z = std::max(mx.z, p.z);
}

// check AABB overlap with translation offsets
static bool aabbOverlapTrans(const Vec3& a_min, const Vec3& a_max, const Vec3& a_pos,
                             const Vec3& b_min, const Vec3& b_max, const Vec3& b_pos) {
    if (a_max.x + a_pos.x < b_min.x + b_pos.x) return false;
    if (a_min.x + a_pos.x > b_max.x + b_pos.x) return false;
    if (a_max.y + a_pos.y < b_min.y + b_pos.y) return false;
    if (a_min.y + a_pos.y > b_max.y + b_pos.y) return false;
    if (a_max.z + a_pos.z < b_min.z + b_pos.z) return false;
    if (a_min.z + a_pos.z > b_max.z + b_pos.z) return false;
    return true;
}

// compute triangle AABB
static void triAABB(const Triangle& t, Vec3& mn, Vec3& mx) {
    const float inf = std::numeric_limits<float>::infinity();
    mn = {inf, inf, inf};
    mx = {-inf, -inf, -inf};
    aabbExpand(mn, mx, t.v0);
    aabbExpand(mn, mx, t.v1);
    aabbExpand(mn, mx, t.v2);
}

// segment-triangle intersection (Möller–Trumbore) for segment P->Q
// returns true and sets out_t (0..1) and barycentric u,v if intersects
static bool segmentTriangleIntersect(const Vec3& P, const Vec3& Q, const Triangle& tri, float& out_t, float& out_u, float& out_v) {
    const float EPSILON = 1e-6f;
    Vec3 dir = vecSub(Q, P);
    Vec3 edge1 = vecSub(tri.v1, tri.v0);
    Vec3 edge2 = vecSub(tri.v2, tri.v0);
    Vec3 h = vecCross(dir, edge2);
    float a = vecDot(edge1, h);
    if (a > -EPSILON && a < EPSILON) return false; // parallel
    float f = 1.0f / a;
    Vec3 s = vecSub(P, tri.v0);
    float u = f * vecDot(s, h);
    if (u < -EPSILON || u > 1.0f + EPSILON) return false;
    Vec3 q = vecCross(s, edge1);
    float v = f * vecDot(dir, q);
    if (v < -EPSILON || u + v > 1.0f + EPSILON) return false;
    float t = f * vecDot(edge2, q);
    // for segment, t must be between 0 and 1 in parametric dir length where dir = Q-P
    // param t' = t / |dir_component_along_ray| but since we use dir directly, t is in ray units where ray = dir.
    // We require t in [0,1]
    if (t < -EPSILON || t > 1.0f + EPSILON) return false;
    out_t = t;
    out_u = u;
    out_v = v;
    return true;
}

// point-in-triangle (works if point is coplanar or near)
static bool pointInTriangle3D(const Vec3& p, const Triangle& tri, float plane_eps = 1e-3f) {
    // compute plane normal
    Vec3 n = vecCross(vecSub(tri.v1, tri.v0), vecSub(tri.v2, tri.v0));
    float nlen = vecLen(n);
    if (nlen < 1e-9f) return false;
    Vec3 nn = vecMul(n, 1.0f / nlen);
    // distance of p to plane
    float dist = vecDot(vecSub(p, tri.v0), nn);
    if (std::fabs(dist) > plane_eps) return false; // not coplanar enough
    // project to barycentric using vectors on plane
    Vec3 v0 = vecSub(tri.v2, tri.v0);
    Vec3 v1 = vecSub(tri.v1, tri.v0);
    Vec3 v2 = vecSub(p, tri.v0);
    float dot00 = vecDot(v0,v0);
    float dot01 = vecDot(v0,v1);
    float dot02 = vecDot(v0,v2);
    float dot11 = vecDot(v1,v1);
    float dot12 = vecDot(v1,v2);
    float denom = dot00 * dot11 - dot01 * dot01;
    if (std::fabs(denom) < 1e-9f) return false;
    float invDenom = 1.0f / denom;
    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
    return (u >= -1e-3f) && (v >= -1e-3f) && (u + v <= 1.0f + 1e-3f);
}

// triangle-triangle intersect: check edge vs triangle for both directions and vertex-in-triangle
bool triangleTriangleIntersect(const Triangle& A, const Triangle& B, Vec3& contact_normal, float& penetration) {
    // edges of A
    Vec3 a0 = A.v0, a1 = A.v1, a2 = A.v2;
    Vec3 b0 = B.v0, b1 = B.v1, b2 = B.v2;

    // 1) check edges of A intersect triangle B
    float t,u,v;
    if (segmentTriangleIntersect(a0, a1, B, t, u, v)) {
        // contact normal approx is triangle B normal
        contact_normal = vecNormalize(vecCross(vecSub(B.v1,B.v0), vecSub(B.v2,B.v0)));
        penetration = 0.01f; // small guess
        return true;
    }
    if (segmentTriangleIntersect(a1, a2, B, t, u, v)) {
        contact_normal = vecNormalize(vecCross(vecSub(B.v1,B.v0), vecSub(B.v2,B.v0)));
        penetration = 0.01f;
        return true;
    }
    if (segmentTriangleIntersect(a2, a0, B, t, u, v)) {
        contact_normal = vecNormalize(vecCross(vecSub(B.v1,B.v0), vecSub(B.v2,B.v0)));
        penetration = 0.01f;
        return true;
    }

    // 2) edges of B intersect triangle A
    if (segmentTriangleIntersect(b0, b1, A, t, u, v)) {
        contact_normal = vecNormalize(vecCross(vecSub(A.v1,A.v0), vecSub(A.v2,A.v0)));
        penetration = 0.01f;
        return true;
    }
    if (segmentTriangleIntersect(b1, b2, A, t, u, v)) {
        contact_normal = vecNormalize(vecCross(vecSub(A.v1,A.v0), vecSub(A.v2,A.v0)));
        penetration = 0.01f;
        return true;
    }
    if (segmentTriangleIntersect(b2, b0, A, t, u, v)) {
        contact_normal = vecNormalize(vecCross(vecSub(A.v1,A.v0), vecSub(A.v2,A.v0)));
        penetration = 0.01f;
        return true;
    }

    // 3) vertex of A inside B?
    if (pointInTriangle3D(a0, B) || pointInTriangle3D(a1, B) || pointInTriangle3D(a2, B)) {
        contact_normal = vecNormalize(vecCross(vecSub(B.v1,B.v0), vecSub(B.v2,B.v0)));
        penetration = 0.01f;
        return true;
    }
    // 4) vertex of B inside A?
    if (pointInTriangle3D(b0, A) || pointInTriangle3D(b1, A) || pointInTriangle3D(b2, A)) {
        contact_normal = vecNormalize(vecCross(vecSub(A.v1,A.v0), vecSub(A.v2,A.v0)));
        penetration = 0.01f;
        return true;
    }

    return false;
}

// --- BVH build -------------------------------------------------

static int buildBVHRecursive(PhysicsMesh& mesh, std::vector<int>& tri_indices, int start, int end, int leaf_tri_count) {
    // create node index
    PhysicsMesh::BVHNode node;
    const float inf = std::numeric_limits<float>::infinity();
    Vec3 mn = {inf, inf, inf};
    Vec3 mx = {-inf, -inf, -inf};
    // compute bounds for range
    for (int i = start; i < end; ++i) {
        const Triangle& t = mesh.triangles[tri_indices[i]];
        aabbExpand(mn, mx, t.v0);
        aabbExpand(mn, mx, t.v1);
        aabbExpand(mn, mx, t.v2);
    }
    node.aabb_min = mn;
    node.aabb_max = mx;
    node.left = node.right = -1;
    node.start = -1;
    node.count = 0;

    int my_index = (int)mesh.nodes.size();
    mesh.nodes.push_back(node); // placeholder, will update

    int tri_count = end - start;
    if (tri_count <= leaf_tri_count) {
        // make leaf: store start/count
        mesh.nodes[my_index].start = start;
        mesh.nodes[my_index].count = tri_count;
        return my_index;
    }

    // choose split axis by extent
    Vec3 ext = {mx.x - mn.x, mx.y - mn.y, mx.z - mn.z};
    int axis = 0;
    if (ext.y > ext.x && ext.y >= ext.z) axis = 1;
    else if (ext.z > ext.x && ext.z > ext.y) axis = 2;

    // compute centroids and sort range
    std::sort(tri_indices.begin() + start, tri_indices.begin() + end, [&](int a, int b){
        const Triangle& ta = mesh.triangles[a];
        const Triangle& tb = mesh.triangles[b];
        float ca = (ta.v0.x + ta.v1.x + ta.v2.x) / 3.0f;
        float cb = (tb.v0.x + tb.v1.x + tb.v2.x) / 3.0f;
        if (axis == 1) { ca = (ta.v0.y + ta.v1.y + ta.v2.y) / 3.0f; cb = (tb.v0.y + tb.v1.y + tb.v2.y) / 3.0f; }
        if (axis == 2) { ca = (ta.v0.z + ta.v1.z + ta.v2.z) / 3.0f; cb = (tb.v0.z + tb.v1.z + tb.v2.z) / 3.0f; }
        return ca < cb;
    });

    int mid = start + tri_count / 2;
    int leftIdx = buildBVHRecursive(mesh, tri_indices, start, mid, leaf_tri_count);
    int rightIdx = buildBVHRecursive(mesh, tri_indices, mid, end, leaf_tri_count);
    mesh.nodes[my_index].left = leftIdx;
    mesh.nodes[my_index].right = rightIdx;
    mesh.nodes[my_index].start = -1;
    mesh.nodes[my_index].count = 0;
    // recompute bounds as union of children
    Vec3 child_min = mesh.nodes[leftIdx].aabb_min;
    Vec3 child_max = mesh.nodes[leftIdx].aabb_max;
    Vec3 cmin2 = mesh.nodes[rightIdx].aabb_min;
    Vec3 cmax2 = mesh.nodes[rightIdx].aabb_max;
    mesh.nodes[my_index].aabb_min = { std::min(child_min.x,cmin2.x), std::min(child_min.y,cmin2.y), std::min(child_min.z,cmin2.z) };
    mesh.nodes[my_index].aabb_max = { std::max(child_max.x,cmax2.x), std::max(child_max.y,cmax2.y), std::max(child_max.z,cmax2.z) };
    return my_index;
}

void buildBVH(PhysicsMesh& mesh, int leaf_tri_count) {
    mesh.nodes.clear();
    mesh.tri_indices.resize(mesh.triangles.size());
    for (size_t i = 0; i < mesh.triangles.size(); ++i) mesh.tri_indices[i] = (int)i;
    if (mesh.triangles.empty()) return;
    buildBVHRecursive(mesh, mesh.tri_indices, 0, (int)mesh.tri_indices.size(), leaf_tri_count);
}

// BVH traversal: collect triangle pairs that might overlap (with transforms)
static void bvhTraversePairs(const PhysicsMesh& A, const Vec3& apos, int aNodeIdx,
                             const PhysicsMesh& B, const Vec3& bpos, int bNodeIdx,
                             std::vector<std::pair<int,int>>& outPairs) {
    const PhysicsMesh::BVHNode& na = A.nodes[aNodeIdx];
    const PhysicsMesh::BVHNode& nb = B.nodes[bNodeIdx];
    if (!aabbOverlapTrans(na.aabb_min, na.aabb_max, apos, nb.aabb_min, nb.aabb_max, bpos)) return;
    bool aLeaf = na.count > 0;
    bool bLeaf = nb.count > 0;
    if (aLeaf && bLeaf) {
        // iterate triangles in leaves
        for (int i = 0; i < na.count; ++i) {
            int ti = A.tri_indices[na.start + i];
            for (int j = 0; j < nb.count; ++j) {
                int tj = B.tri_indices[nb.start + j];
                outPairs.emplace_back(ti, tj);
            }
        }
        return;
    }
    // recurse into children
    if (!aLeaf && !bLeaf) {
        bvhTraversePairs(A, apos, na.left, B, bpos, nb.left, outPairs);
        bvhTraversePairs(A, apos, na.left, B, bpos, nb.right, outPairs);
        bvhTraversePairs(A, apos, na.right, B, bpos, nb.left, outPairs);
        bvhTraversePairs(A, apos, na.right, B, bpos, nb.right, outPairs);
    } else if (!aLeaf) {
        bvhTraversePairs(A, apos, na.left, B, bpos, bNodeIdx, outPairs);
        bvhTraversePairs(A, apos, na.right, B, bpos, bNodeIdx, outPairs);
    } else { // !bLeaf
        bvhTraversePairs(A, apos, aNodeIdx, B, bpos, nb.left, outPairs);
        bvhTraversePairs(A, apos, aNodeIdx, B, bpos, nb.right, outPairs);
    }
}


// API: meshMeshCollision (uses BVH)
// Ensures contact_normal points from a -> b (i.e. direction to push A away from B)
bool meshMeshCollision(const MeshCollider& a, const MeshCollider& b, Vec3& contact_normal, float& penetration) {
    if (a.mesh->nodes.empty() || b.mesh->nodes.empty()) return false;
    std::vector<std::pair<int,int>> candidates;
    bvhTraversePairs(*a.mesh, a.pos, 0, *b.mesh, b.pos, 0, candidates);
    // narrowphase: test triangle pairs
    for (auto& p : candidates) {
        const Triangle& A = a.mesh->triangles[p.first];
        const Triangle& B = b.mesh->triangles[p.second];
        // move triangles into world by adding collider position
        Triangle Aw = { vecAdd(A.v0, a.pos), vecAdd(A.v1, a.pos), vecAdd(A.v2, a.pos) };
        Triangle Bw = { vecAdd(B.v0, b.pos), vecAdd(B.v1, b.pos), vecAdd(B.v2, b.pos) };
        Vec3 n; float pen;
        if (triangleTriangleIntersect(Aw, Bw, n, pen)) {
            // orient normal so it points from A -> B
            Vec3 centerA = { (Aw.v0.x + Aw.v1.x + Aw.v2.x) / 3.0f,
                             (Aw.v0.y + Aw.v1.y + Aw.v2.y) / 3.0f,
                             (Aw.v0.z + Aw.v1.z + Aw.v2.z) / 3.0f };
            Vec3 centerB = { (Bw.v0.x + Bw.v1.x + Bw.v2.x) / 3.0f,
                             (Bw.v0.y + Bw.v1.y + Bw.v2.y) / 3.0f,
                             (Bw.v0.z + Bw.v1.z + Bw.v2.z) / 3.0f };
            Vec3 centerAB = vecSub(centerB, centerA);
            if (vecDot(centerAB, n) < 0.0f) {
                // flip so it points from A to B
                n = vecMul(n, -1.0f);
            }
            contact_normal = vecNormalize(n);
            penetration = pen;
            return true;
        }
    }
    return false;
}


// --- build PhysicsMesh from Raylib Model --------------------

PhysicsMesh physicsMeshFromModel(const Model& m) {
    PhysicsMesh mesh;
    for (int mi = 0; mi < m.meshCount; ++mi) {
        const ::Mesh& rm = m.meshes[mi];
        if (rm.triangleCount == 0) continue;
        // Raylib stores vertices as float array [x0,y0,z0, x1,y1,z1, ...]
        for (unsigned int tri = 0; tri < rm.triangleCount; ++tri) {
            unsigned int i0 = rm.indices[tri*3 + 0];
            unsigned int i1 = rm.indices[tri*3 + 1];
            unsigned int i2 = rm.indices[tri*3 + 2];
            Triangle t;
            t.v0 = { rm.vertices[i0*3 + 0], rm.vertices[i0*3 + 1], rm.vertices[i0*3 + 2] };
            t.v1 = { rm.vertices[i1*3 + 0], rm.vertices[i1*3 + 1], rm.vertices[i1*3 + 2] };
            t.v2 = { rm.vertices[i2*3 + 0], rm.vertices[i2*3 + 1], rm.vertices[i2*3 + 2] };
            mesh.triangles.push_back(t);
        }
    }
    // build BVH
    buildBVH(mesh, 8);
    return mesh;
}

// simple step / integration that uses meshMeshCollision for response
void meshStep(MeshCollider* colliders, size_t count, float dt, Vec3 gravity) {
    if (dt <= 0) return;
    // integrate
    for (size_t i = 0; i < count; ++i) {
        MeshCollider& c = colliders[i];
        if (c.inv_mass > 0) {
            c.vel.x += gravity.x * dt;
            c.vel.y += gravity.y * dt;
            c.vel.z += gravity.z * dt;
            c.pos.x += c.vel.x * dt;
            c.pos.y += c.vel.y * dt;
            c.pos.z += c.vel.z * dt;
        }
    }
    // collision pairs and simple response
    for (size_t i = 0; i < count; ++i) {
        for (size_t j = i+1; j < count; ++j) {
            Vec3 normal; float pen;
            if (meshMeshCollision(colliders[i], colliders[j], normal, pen)) {
                // push them apart along normal proportional to inverse mass
                float invSum = colliders[i].inv_mass + colliders[j].inv_mass;
                if (invSum <= 0.0f) continue;
                Vec3 push = vecMul(normal, pen + 1e-3f); // slight bias
                if (colliders[i].inv_mass > 0) {
                    float k = colliders[i].inv_mass / invSum;
                    colliders[i].pos = vecSub(colliders[i].pos, vecMul(push, k));
                    colliders[i].vel = vecSub(colliders[i].vel, vecMul(colliders[i].vel, 0.2f)); // simple damping
                }
                if (colliders[j].inv_mass > 0) {
                    float k = colliders[j].inv_mass / invSum;
                    colliders[j].pos = vecAdd(colliders[j].pos, vecMul(push, k));
                    colliders[j].vel = vecSub(colliders[j].vel, vecMul(colliders[j].vel, 0.2f));
                }
            }
        }
    }
}
