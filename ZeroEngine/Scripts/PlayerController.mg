// ====================================================================================
// PlayerController.mg - Full physics-based player controller
// ====================================================================================

class PlayerController {
    // Movement
    let f32 moveSpeed = 5.0;
    let f32 jumpForce = 8.0;
    let f32 airControl = 0.3;
    
    // Ground detection
    let f32 groundCheckDistance = 0.1;
    let f32 isGrounded = 0.0;
    
    // State
    let f32 canJump = 1.0;
    let f32 coyoteTime = 0.15;
    let f32 timeSinceGrounded = 0.0;
    
    void fn OnStart() {
        Log("Physics Player Controller initialized!");
    }
    
    void fn OnUpdate() {
        CheckGround();
        HandleMovement();
        HandleJump();
    }
    
    void fn CheckGround() {
        // Raycast down to check if grounded
        // This would be implemented via a native Raycast function
        // For now, simple Y position check
        if (position_y <= 0.55) {
            isGrounded = 1.0;
            timeSinceGrounded = 0.0;
            canJump = 1.0;
        } else {
            isGrounded = 0.0;
            timeSinceGrounded = timeSinceGrounded + delta_time;
        }
    }
    
    void fn HandleMovement() {
        let f32 h = horizontal;
        let f32 v = vertical;
        
        if (abs(h) > 0.01 || abs(v) > 0.01) {
            let f32 speed = moveSpeed;
            
            // Reduce control in air
            if (isGrounded < 0.5) {
                speed = speed * airControl;
            }
            
            // Apply movement force
            let f32 forceX = h * speed;
            let f32 forceZ = v * speed;
            
            AddForce(forceX, 0.0, forceZ);
            
            // Limit velocity
            let f32 velX = velocity_x;
            let f32 velZ = velocity_z;
            let f32 currentSpeed = sqrt(velX * velX + velZ * velZ);
            
            if (currentSpeed > moveSpeed) {
                let f32 scale = moveSpeed / currentSpeed;
                velocity_x = velX * scale;
                velocity_z = velZ * scale;
            }
        }
    }
    
    void fn HandleJump() {
        // Coyote time: can jump shortly after leaving ground
        let f32 canCoyoteJump = 0.0;
        if (timeSinceGrounded < coyoteTime) {
            canCoyoteJump = 1.0;
        }
        
        if (GetKeyDown(32) && (isGrounded > 0.5 || canCoyoteJump > 0.5) && canJump > 0.5) {
            // Apply jump impulse
            velocity_y = jumpForce;
            canJump = 0.0;
            isGrounded = 0.0;
            Log("Jump!");
        }
    }
}


