// enemy_ai.mg - Simple enemy AI

let f32 state = 0.0;  // 0=idle, 1=chase, 2=attack
let f32 timer = 0.0;
let f32 attack_range = 3.0;
let f32 detect_range = 8.0;
let f32 move_speed = 2.5;

void fn on_start() {
    log("Enemy AI started!");
}

void fn on_update() {
    let f32 dt = delta_time;
    timer = timer + dt;
    
    // Calculate distance to player
    let f32 dist = player_distance;
    
    // Get current position
    let f32 my_x = pos_x;
    let f32 my_y = pos_y;
    let f32 my_z = pos_z;
    
    // State machine
    if (state < 0.5) {
        // Idle - bob up and down
        let f32 new_y = 0.5 + sin(total_time * 2.0) * 0.2;
        set_position(my_x, new_y, my_z);
        
        // Check for player
        if (dist < detect_range) {
            state = 1.0;
            log("Enemy detected player!");
        }
    } elif (state < 1.5) {
        // Chase
        let f32 dx = player_x - my_x;
        let f32 dz = player_z - my_z;
        
        if (dist > 0.1) {
            let f32 nx = dx / dist;
            let f32 nz = dz / dist;
            let f32 new_x = my_x + nx * move_speed * dt;
            let f32 new_z = my_z + nz * move_speed * dt;
            set_position(new_x, my_y, new_z);
        }
        
        // Attack if close
        if (dist < attack_range) {
            state = 2.0;
            timer = 0.0;
        }
        
        // Lose interest if too far
        if (dist > detect_range * 1.5) {
            state = 0.0;
            log("Enemy lost player");
        }
    } else {
        // Attack
        if (timer > 1.0) {
            log("Enemy attacks!");
            timer = 0.0;
        }
        
        // Chase if player moved away
        if (dist > attack_range * 1.2) {
            state = 1.0;
        }
    }
    
    // Visual feedback - change color based on state (via scale hack)
    scale_y = 1.0 + state * 0.2;
}

void fn on_destroy() {
    log("Enemy destroyed");
}
