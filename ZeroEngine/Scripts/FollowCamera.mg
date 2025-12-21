// FollowCamera.mg - Camera that dynamically finds and follows a target

class FollowCamera {
    // Offset from target
    let f32 offsetX = 0.0;
    let f32 offsetY = 8.0;
    let f32 offsetZ = 12.0;
    
    // Smooth follow
    let f32 smoothSpeed = 5.0;
    
    // Target handle (found dynamically)
    let f32 targetHandle = 0.0;
    
    void fn OnStart() {
        // Find player to follow
        targetHandle = FindByName(0); // "Player"
        
        if (IsValidEntity(targetHandle) < 0.5) {
            // Try finding by tag
            targetHandle = FindByTag(1); // "MainCamera" target or similar
        }
    }
    
    void fn OnUpdate() {
        // Validate target exists
        if (IsValidEntity(targetHandle) < 0.5) {
            targetHandle = FindByName(0); // Try to find player again
            if (IsValidEntity(targetHandle) < 0.5) {
                return;
            }
        }
        
        // Get target position
        let f32 targetX = GetEntityX(targetHandle);
        let f32 targetY = GetEntityY(targetHandle);
        let f32 targetZ = GetEntityZ(targetHandle);
        
        // Calculate desired camera position
        let f32 desiredX = targetX + offsetX;
        let f32 desiredY = targetY + offsetY;
        let f32 desiredZ = targetZ + offsetZ;
        
        // Smooth interpolation
        let f32 t = smoothSpeed * delta_time;
        if (t > 1.0) {
            t = 1.0;
        }
        
        position_x = lerp(position_x, desiredX, t);
        position_y = lerp(position_y, desiredY, t);
        position_z = lerp(position_z, desiredZ, t);
        
        // Look at target
        LookAt(targetX, targetY, targetZ);
    }
}
