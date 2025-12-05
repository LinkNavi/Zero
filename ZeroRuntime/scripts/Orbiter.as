class Orbiter {
    private float radius;
    private float speed;
    private float angle;
    private float centerX;
    private float centerZ;
    
    Orbiter() {
        radius = 5.0f;
        speed = 1.0f;
        angle = 0.0f;
        centerX = 0.0f;
        centerZ = 0.0f;
    }
    
    void Start() {
        Entity@ entity = GetEntity();
        array<float>@ pos = entity.GetPosition();
        centerX = pos[0];
        centerZ = pos[2];
        Print("Orbiter initialized at center: " + centerX + ", " + centerZ);
    }
    
    void Update(float dt) {
        Entity@ entity = GetEntity();
        
        angle += speed * dt;
        
        float x = centerX + (radius * cos(angle));
        float z = centerZ + (radius * sin(angle));
        
        entity.SetPosition(x, 1.0f, z);
    }
}
