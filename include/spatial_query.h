#pragma once
#include "Engine.h"
#include "transform.h"
#include "PhysicsSystem.h"
#include <glm/glm.hpp>
#include <vector>
#include <limits>
#include <algorithm>

// Raycast hit result
struct RaycastHit {
    bool hit = false;
    EntityID entity = 0;
    glm::vec3 point{0};
    glm::vec3 normal{0};
    float distance = 0.0f;
    
    RaycastHit() = default;
    
    bool operator<(const RaycastHit& other) const {
        return distance < other.distance;
    }
};

// Spatial query system
class SpatialQuery {
public:
    // Raycast against all colliders
    static RaycastHit raycast(ECS* ecs, glm::vec3 origin, glm::vec3 direction, 
                             float maxDistance = 1000.0f, uint32_t layerMask = 0xFFFFFFFF) {
        direction = glm::normalize(direction);
        RaycastHit closestHit;
        closestHit.distance = maxDistance;
        
        // Check all entities with colliders
        for (size_t i = 0; i < 10000; ++i) {
            auto* transform = ecs->getComponent<Transform>(i);
            auto* collider = ecs->getComponent<Collider>(i);
            auto* layer = ecs->getComponent<Layer>(i);
            
            if (!transform || !collider) continue;
            
            // Layer mask filter
            if (layer && !(layer->mask & layerMask)) continue;
            
            RaycastHit hit;
            hit.entity = i;
            
            // Ray-collider intersection
            bool intersects = false;
            
            switch (collider->type) {
                case ColliderType::Sphere:
                    intersects = raySphere(origin, direction, transform->position, 
                                          collider->radius, hit);
                    break;
                    
                case ColliderType::Box:
                    intersects = rayBox(origin, direction, transform->position,
                                       collider->size * transform->scale, hit);
                    break;
                    
                case ColliderType::Capsule:
                    // Simplified capsule as sphere for now
                    intersects = raySphere(origin, direction, transform->position,
                                          collider->radius, hit);
                    break;
            }
            
            if (intersects && hit.distance < closestHit.distance && hit.distance <= maxDistance) {
                closestHit = hit;
            }
        }
        
        return closestHit;
    }
    
    // Raycast returning all hits
    static std::vector<RaycastHit> raycastAll(ECS* ecs, glm::vec3 origin, glm::vec3 direction,
                                               float maxDistance = 1000.0f, uint32_t layerMask = 0xFFFFFFFF) {
        direction = glm::normalize(direction);
        std::vector<RaycastHit> hits;
        
        for (size_t i = 0; i < 10000; ++i) {
            auto* transform = ecs->getComponent<Transform>(i);
            auto* collider = ecs->getComponent<Collider>(i);
            auto* layer = ecs->getComponent<Layer>(i);
            
            if (!transform || !collider) continue;
            if (layer && !(layer->mask & layerMask)) continue;
            
            RaycastHit hit;
            hit.entity = i;
            
            bool intersects = false;
            
            switch (collider->type) {
                case ColliderType::Sphere:
                    intersects = raySphere(origin, direction, transform->position,
                                          collider->radius, hit);
                    break;
                case ColliderType::Box:
                    intersects = rayBox(origin, direction, transform->position,
                                       collider->size * transform->scale, hit);
                    break;
                case ColliderType::Capsule:
                    intersects = raySphere(origin, direction, transform->position,
                                          collider->radius, hit);
                    break;
            }
            
            if (intersects && hit.distance <= maxDistance) {
                hits.push_back(hit);
            }
        }
        
        // Sort by distance
        std::sort(hits.begin(), hits.end());
        
        return hits;
    }
    
    // Overlap sphere - find all colliders in radius
    static std::vector<EntityID> overlapSphere(ECS* ecs, glm::vec3 center, float radius,
                                                uint32_t layerMask = 0xFFFFFFFF) {
        std::vector<EntityID> results;
        float radiusSq = radius * radius;
        
        for (size_t i = 0; i < 10000; ++i) {
            auto* transform = ecs->getComponent<Transform>(i);
            auto* collider = ecs->getComponent<Collider>(i);
            auto* layer = ecs->getComponent<Layer>(i);
            
            if (!transform || !collider) continue;
            if (layer && !(layer->mask & layerMask)) continue;
            
            bool overlaps = false;
            
            switch (collider->type) {
                case ColliderType::Sphere: {
                    float dist = glm::distance(center, transform->position);
                    overlaps = dist < (radius + collider->radius);
                    break;
                }
                case ColliderType::Box: {
                    // Sphere-box overlap (simplified)
                    glm::vec3 halfExtents = collider->size * transform->scale * 0.5f;
                    glm::vec3 closestPoint = glm::clamp(center, 
                                                        transform->position - halfExtents,
                                                        transform->position + halfExtents);
                    float distSq = glm::distance2(center, closestPoint);
                    overlaps = distSq < radiusSq;
                    break;
                }
                case ColliderType::Capsule: {
                    float dist = glm::distance(center, transform->position);
                    overlaps = dist < (radius + collider->radius);
                    break;
                }
            }
            
            if (overlaps) {
                results.push_back(i);
            }
        }
        
        return results;
    }
    
    // Overlap box - find all colliders in box
    static std::vector<EntityID> overlapBox(ECS* ecs, glm::vec3 center, glm::vec3 halfExtents,
                                            uint32_t layerMask = 0xFFFFFFFF) {
        std::vector<EntityID> results;
        
        for (size_t i = 0; i < 10000; ++i) {
            auto* transform = ecs->getComponent<Transform>(i);
            auto* collider = ecs->getComponent<Collider>(i);
            auto* layer = ecs->getComponent<Layer>(i);
            
            if (!transform || !collider) continue;
            if (layer && !(layer->mask & layerMask)) continue;
            
            bool overlaps = false;
            
            switch (collider->type) {
                case ColliderType::Sphere: {
                    // Box-sphere overlap
                    glm::vec3 closestPoint = glm::clamp(transform->position,
                                                        center - halfExtents,
                                                        center + halfExtents);
                    float distSq = glm::distance2(transform->position, closestPoint);
                    overlaps = distSq < (collider->radius * collider->radius);
                    break;
                }
                case ColliderType::Box: {
                    // Box-box overlap (AABB test)
                    glm::vec3 otherHalf = collider->size * transform->scale * 0.5f;
                    glm::vec3 min1 = center - halfExtents;
                    glm::vec3 max1 = center + halfExtents;
                    glm::vec3 min2 = transform->position - otherHalf;
                    glm::vec3 max2 = transform->position + otherHalf;
                    
                    overlaps = (min1.x <= max2.x && max1.x >= min2.x) &&
                              (min1.y <= max2.y && max1.y >= min2.y) &&
                              (min1.z <= max2.z && max1.z >= min2.z);
                    break;
                }
                case ColliderType::Capsule: {
                    glm::vec3 closestPoint = glm::clamp(transform->position,
                                                        center - halfExtents,
                                                        center + halfExtents);
                    float distSq = glm::distance2(transform->position, closestPoint);
                    overlaps = distSq < (collider->radius * collider->radius);
                    break;
                }
            }
            
            if (overlaps) {
                results.push_back(i);
            }
        }
        
        return results;
    }
    
    // Find closest entity to point
    static EntityID findClosest(ECS* ecs, glm::vec3 point, float maxDistance = 1000.0f,
                                uint32_t layerMask = 0xFFFFFFFF) {
        EntityID closest = 0;
        float closestDistSq = maxDistance * maxDistance;
        
        for (size_t i = 0; i < 10000; ++i) {
            auto* transform = ecs->getComponent<Transform>(i);
            auto* layer = ecs->getComponent<Layer>(i);
            
            if (!transform) continue;
            if (layer && !(layer->mask & layerMask)) continue;
            
            float distSq = glm::distance2(point, transform->position);
            if (distSq < closestDistSq) {
                closestDistSq = distSq;
                closest = i;
            }
        }
        
        return closest;
    }
    
    // Find all entities within distance
    static std::vector<EntityID> findInRadius(ECS* ecs, glm::vec3 center, float radius,
                                              uint32_t layerMask = 0xFFFFFFFF) {
        std::vector<EntityID> results;
        float radiusSq = radius * radius;
        
        for (size_t i = 0; i < 10000; ++i) {
            auto* transform = ecs->getComponent<Transform>(i);
            auto* layer = ecs->getComponent<Layer>(i);
            
            if (!transform) continue;
            if (layer && !(layer->mask & layerMask)) continue;
            
            float distSq = glm::distance2(center, transform->position);
            if (distSq <= radiusSq) {
                results.push_back(i);
            }
        }
        
        return results;
    }

private:
    // Ray-sphere intersection
    static bool raySphere(glm::vec3 origin, glm::vec3 direction, glm::vec3 center, 
                         float radius, RaycastHit& hit) {
        glm::vec3 oc = origin - center;
        float a = glm::dot(direction, direction);
        float b = 2.0f * glm::dot(oc, direction);
        float c = glm::dot(oc, oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;
        
        if (discriminant < 0) return false;
        
        float t = (-b - std::sqrt(discriminant)) / (2.0f * a);
        if (t < 0) {
            t = (-b + std::sqrt(discriminant)) / (2.0f * a);
            if (t < 0) return false;
        }
        
        hit.hit = true;
        hit.distance = t;
        hit.point = origin + direction * t;
        hit.normal = glm::normalize(hit.point - center);
        
        return true;
    }
    
    // Ray-box intersection (AABB)
    static bool rayBox(glm::vec3 origin, glm::vec3 direction, glm::vec3 center,
                      glm::vec3 size, RaycastHit& hit) {
        glm::vec3 halfSize = size * 0.5f;
        glm::vec3 boxMin = center - halfSize;
        glm::vec3 boxMax = center + halfSize;
        
        glm::vec3 invDir = 1.0f / direction;
        glm::vec3 t1 = (boxMin - origin) * invDir;
        glm::vec3 t2 = (boxMax - origin) * invDir;
        
        glm::vec3 tmin = glm::min(t1, t2);
        glm::vec3 tmax = glm::max(t1, t2);
        
        float tNear = glm::max(glm::max(tmin.x, tmin.y), tmin.z);
        float tFar = glm::min(glm::min(tmax.x, tmax.y), tmax.z);
        
        if (tNear > tFar || tFar < 0) return false;
        
        float t = tNear > 0 ? tNear : tFar;
        if (t < 0) return false;
        
        hit.hit = true;
        hit.distance = t;
        hit.point = origin + direction * t;
        
        // Calculate normal
        glm::vec3 localPoint = hit.point - center;
        glm::vec3 absLocal = glm::abs(localPoint);
        
        if (absLocal.x > absLocal.y && absLocal.x > absLocal.z) {
            hit.normal = glm::vec3(localPoint.x > 0 ? 1 : -1, 0, 0);
        } else if (absLocal.y > absLocal.z) {
            hit.normal = glm::vec3(0, localPoint.y > 0 ? 1 : -1, 0);
        } else {
            hit.normal = glm::vec3(0, 0, localPoint.z > 0 ? 1 : -1);
        }
        
        return true;
    }
};
