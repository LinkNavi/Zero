// Collectible.mg - Collectible item that detects nearby player

class Collectible {
    let f32 rotationSpeed = 180.0;
    let f32 bobSpeed = 2.0;
    let f32 bobAmount = 0.3;
    let f32 collectRange = 1.5;
    
    let f32 startY = 0.0;
    let f32 bobTimer = 0.0;
    let f32 playerHandle = 0.0;
    
    void fn OnStart() {
        startY = position_y;
        playerHandle = FindByName(0); // "Player"
	Log(playerHandle);
    }
    
    void fn OnUpdate() {
        // Rotate
        Rotate(0.0, rotationSpeed * delta_time, 0.0);
        
        // Bob up and down
        bobTimer = bobTimer + delta_time * bobSpeed;
        position_y = startY + sin(bobTimer) * bobAmount;
        
        // Check if player is nearby
        if (IsValidEntity(playerHandle) > 0.5) {
            let f32 dist = DistanceTo(playerHandle);
            if (dist < collectRange) {
                Log(200); // "Collected!"
                DestroySelf();
            }
        } else {
            // Try to find player
            playerHandle = FindByName(0);
        }
    }
}
