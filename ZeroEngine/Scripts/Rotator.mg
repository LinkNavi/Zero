// Rotator.mg - Simple rotation script

class Rotator {
    let f32 rotationSpeed = 90.0; // degrees per second
    let f32 axisX = 0.0;
    let f32 axisY = 1.0;
    let f32 axisZ = 0.0;
    
    void fn OnUpdate() {
        let f32 rotation = rotationSpeed * delta_time;
        Rotate(axisX * rotation, axisY * rotation, axisZ * rotation);
    }
}
