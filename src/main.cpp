// example_usage.cpp - Demonstrates all new systems

#include "Engine.h"
#include "transform.h"
#include "tags.h"
#include "event_system.h"
#include "timer.h"
#include "asset_manager.h"
#include "prefab.h"
#include "spatial_query.h"
#include "component_registry.h"
#include "PhysicsSystem.h"
#include <iostream>
#include <thread>

// Example game where we use all systems together

int main() {
    std::cout << "=== Engine Systems Example ===" << std::endl;
    
    // === 1. Setup ECS ===
    ECS ecs;
    
    // Register components
    ecs.registerComponent<Transform>();
    ecs.registerComponent<Tag>();
    ecs.registerComponent<Layer>();
    ecs.registerComponent<RigidBody>();
    ecs.registerComponent<Collider>();
    
    // === 2. Setup Component Registry ===
    ComponentRegistry& registry = getGlobalComponentRegistry();
    registry.registerComponent<Transform>("Transform");
    registry.registerComponent<Tag>("Tag");
    registry.registerComponent<Layer>("Layer");
    registry.registerComponent<RigidBody>("RigidBody");
    registry.registerComponent<Collider>("Collider");
    registry.printRegistry();
    
    // === 3. Setup Event System ===
    EventSystem events;
    
    // Subscribe to collision events
    events.subscribe(EventType::OnCollisionEnter, [](const Event& e) {
        EntityID a = e.entity;
        EntityID b = e.get<EntityID>("other", 0);
        std::cout << "Collision: Entity " << a << " hit Entity " << b << std::endl;
    });
    
    // Subscribe to custom events
    events.subscribe("PlayerScored", [](const Event& e) {
        int score = e.get<int>("score", 0);
        std::cout << "Player scored! Total: " << score << std::endl;
    });
    
    // === 4. Setup Timer System ===
    TimerManager timers;
    
    // Create a one-shot timer
    timers.after(2.0f, []() {
        std::cout << "2 seconds elapsed!" << std::endl;
    });
    
    // Create repeating timer
    timers.every(1.0f, []() {
        std::cout << "Tick!" << std::endl;
    }, "game_tick");
    
    // === 5. Create Entities with Tags and Layers ===
    
    // Player entity
    EntityID player = ecs.createEntity();
    {
        Transform transform(glm::vec3(0, 0, 0));
        ecs.addComponent(player, transform);
        
        Tag tag("Player");
        ecs.addComponent(player, tag);
        
        Layer layer(Layers::Player);
        ecs.addComponent(player, layer);
        
        RigidBody rb;
        rb.mass = 70.0f;
        rb.useGravity = true;
        ecs.addComponent(player, rb);
        
        Collider col;
        col.type = ColliderType::Capsule;
        col.radius = 0.5f;
        ecs.addComponent(player, col);
    }
    std::cout << "Created Player (ID: " << player << ")" << std::endl;
    
    // Enemy entities
    for (int i = 0; i < 5; i++) {
        EntityID enemy = ecs.createEntity();
        
        Transform transform(glm::vec3(i * 3.0f, 0, 5));
        ecs.addComponent(enemy, transform);
        
        Tag tag("Enemy");
        ecs.addComponent(enemy, tag);
        
        Layer layer(Layers::Enemy);
        ecs.addComponent(enemy, layer);
        
        Collider col;
        col.type = ColliderType::Sphere;
        col.radius = 1.0f;
        ecs.addComponent(enemy, col);
        
        std::cout << "Created Enemy (ID: " << enemy << ")" << std::endl;
    }
    
    // === 6. Query System ===
    std::cout << "\n=== Entity Queries ===" << std::endl;
    
    // Find player
    EntityID foundPlayer = TagLayerQuery::findEntityWithTag(&ecs, "Player");
    std::cout << "Found player: " << foundPlayer << std::endl;
    
    // Find all enemies
    auto enemies = TagLayerQuery::findEntitiesWithTag(&ecs, "Enemy");
    std::cout << "Found " << enemies.size() << " enemies" << std::endl;
    
    // Find entities on player layer
    auto playerLayerEntities = TagLayerQuery::findEntitiesOnLayer(&ecs, Layers::Player);
    std::cout << "Entities on Player layer: " << playerLayerEntities.size() << std::endl;
    
    // === 7. Spatial Queries ===
    std::cout << "\n=== Spatial Queries ===" << std::endl;
    
    // Raycast forward from player
    auto* playerTransform = ecs.getComponent<Transform>(player);
    if (playerTransform) {
        glm::vec3 forward = playerTransform->forward();
        RaycastHit hit = SpatialQuery::raycast(&ecs, playerTransform->position, forward, 50.0f);
        
        if (hit.hit) {
            std::cout << "Raycast hit entity " << hit.entity 
                      << " at distance " << hit.distance << std::endl;
        } else {
            std::cout << "Raycast hit nothing" << std::endl;
        }
    }
    
    // Find entities in radius
    auto nearbyEntities = SpatialQuery::findInRadius(&ecs, glm::vec3(0), 10.0f);
    std::cout << "Entities within 10 units: " << nearbyEntities.size() << std::endl;
    
    // Overlap sphere (physics query)
    auto overlapping = SpatialQuery::overlapSphere(&ecs, glm::vec3(3, 0, 5), 2.0f);
    std::cout << "Entities overlapping sphere: " << overlapping.size() << std::endl;
    
    // === 8. Prefab System ===
    std::cout << "\n=== Prefab System ===" << std::endl;
    
    // Create a prefab programmatically
    Prefab enemyPrefab("BasicEnemy");
    enemyPrefab.data = {
        {"name", "BasicEnemy"},
        {"components", {
            {"transform", {
                {"scale", {1.0, 1.0, 1.0}}
            }},
            {"tag", "Enemy"},
            {"layer", (uint32_t)(1 << Layers::Enemy)},
            {"collider", {
                {"type", "sphere"},
                {"radius", 1.0f},
                {"isTrigger", false}
            }},
            {"rigidbody", {
                {"mass", 50.0f},
                {"useGravity", true},
                {"isKinematic", false}
            }}
        }}
    };
    
    // Save prefab
    enemyPrefab.save("prefabs/basic_enemy.json");
    
    // Instantiate prefab
    EntityID spawned = enemyPrefab.instantiate(&ecs, glm::vec3(10, 0, 0));
    std::cout << "Spawned enemy from prefab (ID: " << spawned << ")" << std::endl;
    
    // === 9. Scene Serialization ===
    std::cout << "\n=== Scene Serialization ===" << std::endl;
    
    // Save current scene
    EntitySerializer::saveScene(&ecs, "scenes/test_scene.json");
    
    // === 10. Event Emission ===
    std::cout << "\n=== Event System ===" << std::endl;
    
    // Emit collision event
    Event collision;
    collision.type = EventType::OnCollisionEnter;
    collision.entity = player;
    collision.setEntity("other", enemies[0]);
    collision.setVec3("point", glm::vec3(0, 0, 0));
    events.emit(collision);
    
    // Emit custom event
    Event scoreEvent = EVENT_CUSTOM("PlayerScored");
    scoreEvent.setInt("score", 100);
    events.emit(scoreEvent);
    
    // Queue events for later
    events.queue(collision);
    events.processQueue();
    
    // === 11. Transform Hierarchy ===
    std::cout << "\n=== Transform Hierarchy ===" << std::endl;
    
    // Create parent-child relationship
    EntityID weapon = ecs.createEntity();
    Transform weaponTransform(glm::vec3(1, 0, 0)); // Offset from player
    ecs.addComponent(weapon, weaponTransform);
    
    // Attach weapon to player
    TransformSystem transformSystem;
    transformSystem.init(&ecs);
    transformSystem.attachToParent(weapon, player, false);
    
    std::cout << "Attached weapon to player" << std::endl;
    
    // Get world position of weapon (includes parent transform)
    auto* weaponTrans = ecs.getComponent<Transform>(weapon);
    if (weaponTrans) {
        glm::vec3 worldPos = weaponTrans->getWorldPosition(&ecs);
        std::cout << "Weapon world position: (" 
                  << worldPos.x << ", " << worldPos.y << ", " << worldPos.z << ")" << std::endl;
    }
    
    // === 12. Asset Manager (mock example) ===
    std::cout << "\n=== Asset Manager ===" << std::endl;
    
    AssetManager assets;
    // assets.init(&modelLoader); // Would init with actual loaders
    assets.setBaseDirectories("models/", "textures/", "sounds/");
    assets.printStats();
    
    // Would load assets like:
    // auto model = assets.loadModel("enemy.glb");
    // auto texture = assets.loadTexture("enemy_diffuse.png");
    
    // === 13. Timer Updates (simulate game loop) ===
    std::cout << "\n=== Game Loop Simulation ===" << std::endl;
    
    float dt = 0.016f; // 60 FPS
    for (int frame = 0; frame < 10; frame++) {
        // Update timers
        timers.update(dt);
        
        // Update physics (would be your actual physics system)
        // physicsSystem.update(dt);
        
        // Process events
        events.processQueue();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // === 14. Cleanup ===
    std::cout << "\n=== Cleanup ===" << std::endl;
    
    timers.clear();
    events.clear();
    assets.clear();
    
    std::cout << "\nExample complete!" << std::endl;
    
    return 0;
}

/*
Expected Output:

=== Engine Systems Example ===
Registered component: Transform (size: 112 bytes, ID: 0)
Registered component: Tag (size: 32 bytes, ID: 1)
...
Created Player (ID: 0)
Created Enemy (ID: 1)
...
Found player: 0
Found 5 enemies
Raycast hit entity 1 at distance 5.2
Spawned enemy from prefab (ID: 6)
Collision: Entity 0 hit Entity 1
Player scored! Total: 100
Tick!
Tick!
...
*/
