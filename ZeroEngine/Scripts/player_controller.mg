// player_controller.mg - Example Magolor script for ZeroEngine

// Called when the script starts
void fn on_start() {
    print_str("Player controller initialized!");
}

// Called every frame
void fn on_update() {
    // Read engine-provided variables
    let f32 dt = delta_time;
    let f32 x = pos_x;
    let f32 y = pos_y;
    let f32 z = pos_z;
    
    // Simple movement logic
    let f32 speed = 5.0;
    
    // Move in a circle
    let f32 time = total_time;
    pos_x = sin(time) * 3.0;
    pos_z = cos(time) * 3.0;
    
    // Bounce up and down
    pos_y = abs(sin(time * 2.0)) * 2.0 + 0.5;
}

// Called when destroyed
void fn on_destroy() {
    print_str("Player controller destroyed!");
}

// Custom function for damage calculation
i32 fn calculate_damage(i32 base_damage, i32 armor) {
    let i32 reduced = base_damage - armor;
    if (reduced < 0) {
        return 0;
    }
    return reduced;
}

// AI decision making
i32 fn ai_decide(i32 health, i32 distance) {
    if (health < 20) {
        return 0; // flee
    } elif (distance < 5) {
        return 1; // attack
    } else {
        return 2; // patrol
    }
}
