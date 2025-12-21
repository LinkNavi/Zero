// Collectible.mg - Rotating and bobbing collectible item

class Collectible {
    let f32 rotationSpeed = 180.0;
    let f32 bobSpeed = 2.0;
    let f32 bobAmount = 0.3;
    let f32 startY = 0.0;
    
    void fn OnStart() {
        startY = position_y;
    }
    
    void fn OnUpdate() {
        // Rotate
        Rotate(0.0, rotationSpeed * delta_time, 0.0);
        
        // Bob up and down
        let f32 newY = startY + sin(time * bobSpeed) * bobAmount;
        position_y = newY;
    }
}
