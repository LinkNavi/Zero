// FollowCamera.mg - Camera that follows a target

class FollowCamera {
    // Target offset
    let f32 offsetX = 0.0;
    let f32 offsetY = 8.0;
    let f32 offsetZ = 12.0;
    
    // Smooth follow speed
    let f32 smoothSpeed = 5.0;
    
    // Target position (set by engine)
    let f32 targetX = 0.0;
    let f32 targetY = 0.0;
    let f32 targetZ = 0.0;
    
    void fn OnUpdate() {
        // Calculate desired position
        let f32 desiredX = targetX + offsetX;
        let f32 desiredY = targetY + offsetY;
        let f32 desiredZ = targetZ + offsetZ;
        
        // Smoothly interpolate
        let f32 t = smoothSpeed * delta_time;
        if (t > 1.0) {
            t = 1.0;
        }
        
        let f32 newX = position_x + (desiredX - position_x) * t;
        let f32 newY = position_y + (desiredY - position_y) * t;
        let f32 newZ = position_z + (desiredZ - position_z) * t;
        
        position_x = newX;
        position_y = newY;
        position_z = newZ;
        
        // Look at target
        LookAt(targetX, targetY, targetZ);
    }
}
