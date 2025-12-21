// player_controller.mg - Player movement script

void fn on_start() {
    log("Player script started!");
}

void fn on_update() {
    let f32 dt = delta_time;
    let f32 spd = speed;
    
    // Get input (set by engine)
    let f32 h = input_h;
    let f32 v = input_v;
    
    // Get current position
    let f32 cur_x = pos_x;
    let f32 cur_y = pos_y;
    let f32 cur_z = pos_z;
    
    // Calculate new position based on input
    let f32 new_x = cur_x + h * spd * dt;
    let f32 new_z = cur_z + v * spd * dt;
    let f32 new_y = cur_y;
    
    // Jump
    if (input_jump > 0.5 && is_grounded > 0.5) {
        velocity_y = jump_force;
        is_grounded = 0.0;
    }
    
    // Apply gravity
    if (is_grounded < 0.5) {
        velocity_y = velocity_y - 20.0 * dt;
        new_y = cur_y + velocity_y * dt;
    }
    
    // Ground check
    if (new_y <= 0.5) {
        new_y = 0.5;
        velocity_y = 0.0;
        is_grounded = 1.0;
    }
    
    // Update position using set_position function
    set_position(new_x, new_y, new_z);
}

void fn on_destroy() {
    log("Player script destroyed");
}
