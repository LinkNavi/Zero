// PlayerController.mg - Player controller with dynamic entity access

class PlayerController {
    // Movement settings
    let f32 moveSpeed = 5.0;
    let f32 jumpForce = 8.0;
    let f32 gravity = 20.0;
    
    // State
    let f32 velocityY = 0.0;
    let f32 isGrounded = 1.0;
    let f32 groundY = 0.5;
    
    void fn OnStart() {
        Log(1); // "Player initialized"
    }
    
    void fn OnUpdate() {
        // Get input
        let f32 h = horizontal;
        let f32 v = vertical;
        
        // Move horizontally
        if (abs(h) > 0.01 || abs(v) > 0.01) {
            let f32 moveX = h * moveSpeed * delta_time;
            let f32 moveZ = v * moveSpeed * delta_time;
            position_x = position_x + moveX;
            position_z = position_z + moveZ;
        }
        
        // Jump
        if (GetKeyDown(32) && isGrounded > 0.5) {
            velocityY = jumpForce;
            isGrounded = 0.0;
        }
        
        // Apply gravity
        if (isGrounded < 0.5) {
            velocityY = velocityY - gravity * delta_time;
            position_y = position_y + velocityY * delta_time;
            
            // Ground check
            if (position_y <= groundY) {
                position_y = groundY;
                velocityY = 0.0;
                isGrounded = 1.0;
            }
        }
    }
}
