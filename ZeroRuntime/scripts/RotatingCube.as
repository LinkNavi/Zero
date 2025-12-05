class RotatingCube {
    private float speed;
    private float angle;
    
    RotatingCube() {
        speed = 45.0f; // Degrees per second
        angle = 0.0f;
    }
    
    void Start() {
        Print("Rotating cube initialized");
    }
    
    void Update(float dt) {
        Entity@ entity = GetEntity();
        array<float>@ pos = entity.GetPosition();
        
        angle += speed * dt;
        if (angle > 360.0f) {
            angle -= 360.0f;
        }
        
        // Bob up and down
        float angleRad = angle * 0.01745f; // degrees to radians
        float y = 1.0f + (sin(angleRad) * 0.5f);
        
        entity.SetPosition(pos[0], y, pos[2]);
        entity.SetRotation(angle, angle * 0.5f, 0.0f);
    }
}
