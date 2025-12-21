// enemy_ai.mg - Enemy AI script example

// State constants
let i32 STATE_IDLE = 0;
let i32 STATE_PATROL = 1;
let i32 STATE_CHASE = 2;
let i32 STATE_ATTACK = 3;
let i32 STATE_FLEE = 4;

// Current state
let i32 current_state = 0;
let f32 state_timer = 0.0;

// Stats
let i32 health = 100;
let i32 max_health = 100;
let f32 attack_range = 2.0;
let f32 detect_range = 10.0;
let f32 move_speed = 3.0;

void fn on_start() {
    current_state = STATE_IDLE;
    print_str("Enemy AI started");
}

void fn on_update() {
    let f32 dt = delta_time;
    state_timer = state_timer + dt;
    
    // Get distance to player (would be set by engine)
    let f32 dist = player_distance;
    
    // State machine
    if (current_state == STATE_IDLE) {
        update_idle(dt, dist);
    } elif (current_state == STATE_PATROL) {
        update_patrol(dt, dist);
    } elif (current_state == STATE_CHASE) {
        update_chase(dt, dist);
    } elif (current_state == STATE_ATTACK) {
        update_attack(dt, dist);
    } elif (current_state == STATE_FLEE) {
        update_flee(dt, dist);
    }
}

void fn update_idle(f32 dt, f32 dist) {
    // Check if player is nearby
    if (dist < detect_range) {
        change_state(STATE_CHASE);
        return;
    }
    
    // Start patrolling after idle for 3 seconds
    if (state_timer > 3.0) {
        change_state(STATE_PATROL);
    }
}

void fn update_patrol(f32 dt, f32 dist) {
    // Move in patrol pattern
    let f32 t = total_time;
    pos_x = sin(t * 0.5) * 5.0;
    pos_z = cos(t * 0.5) * 5.0;
    
    // Check for player
    if (dist < detect_range) {
        change_state(STATE_CHASE);
    }
}

void fn update_chase(f32 dt, f32 dist) {
    // Move towards player
    let f32 dir_x = player_x - pos_x;
    let f32 dir_z = player_z - pos_z;
    let f32 len = sqrt(dir_x * dir_x + dir_z * dir_z);
    
    if (len > 0.1) {
        pos_x = pos_x + (dir_x / len) * move_speed * dt;
        pos_z = pos_z + (dir_z / len) * move_speed * dt;
    }
    
    // Check if in attack range
    if (dist < attack_range) {
        change_state(STATE_ATTACK);
    }
    
    // Check if player escaped
    if (dist > detect_range * 1.5) {
        change_state(STATE_PATROL);
    }
    
    // Flee if low health
    if (health < 20) {
        change_state(STATE_FLEE);
    }
}

void fn update_attack(f32 dt, f32 dist) {
    // Attack every 1 second
    if (state_timer > 1.0) {
        do_attack();
        state_timer = 0.0;
    }
    
    // Chase if player moved away
    if (dist > attack_range * 1.5) {
        change_state(STATE_CHASE);
    }
    
    // Flee if low health
    if (health < 20) {
        change_state(STATE_FLEE);
    }
}

void fn update_flee(f32 dt, f32 dist) {
    // Move away from player
    let f32 dir_x = pos_x - player_x;
    let f32 dir_z = pos_z - player_z;
    let f32 len = sqrt(dir_x * dir_x + dir_z * dir_z);
    
    if (len > 0.1) {
        pos_x = pos_x + (dir_x / len) * move_speed * 1.5 * dt;
        pos_z = pos_z + (dir_z / len) * move_speed * 1.5 * dt;
    }
    
    // Stop fleeing if far enough or healed
    if (dist > detect_range * 2.0 || health > 50) {
        change_state(STATE_IDLE);
    }
}

void fn change_state(i32 new_state) {
    current_state = new_state;
    state_timer = 0.0;
}

void fn do_attack() {
    // Would trigger attack animation and damage
    print_str("Enemy attacks!");
}

void fn take_damage(i32 damage) {
    health = health - damage;
    if (health < 0) {
        health = 0;
    }
    
    // React to damage
    if (current_state == STATE_IDLE || current_state == STATE_PATROL) {
        change_state(STATE_CHASE);
    }
}

i32 fn get_health() {
    return health;
}

bool fn is_alive() {
    return health > 0;
}
