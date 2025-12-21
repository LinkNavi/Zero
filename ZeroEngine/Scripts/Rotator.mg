// Rotator.mg - Simple rotating object

class Rotator {
    let f32 speedX = 0.0;
    let f32 speedY = 90.0;
    let f32 speedZ = 0.0;
    
    void fn OnUpdate() {
        Rotate(speedX * delta_time, speedY * delta_time, speedZ * delta_time);
    }
}
