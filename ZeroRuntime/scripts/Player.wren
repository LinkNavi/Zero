class Player {
    private float speed;
    private float jumpForce;
    private bool isGrounded;
    private float yVelocity;
    private float gravity;
    
    Player() {
        speed = 10.0f;
        jumpForce = 15.0f;
        isGrounded = true;
        yVelocity = 0.0f;
        gravity = -30.0f;
    }
    
    void Start() {
        Print("Player initialized!");
        Entity@ entity = GetEntity();
        Print("Player entity ID: " + entity.GetId());
    }
    
    void Update(float dt) {
        Entity@ entity = GetEntity();
        array<float>@ pos = entity.GetPosition();
        array<float>@ vel = entity.GetVelocity();
        
        float vx = 0.0f;
        float vy = vel[1];
        float vz = 0.0f;
        
        // WASD movement
        if (IsKeyDown(Key::W)) vz -= speed;
        if (IsKeyDown(Key::S)) vz += speed;
        if (IsKeyDown(Key::A)) vx -= speed;
        if (IsKeyDown(Key::D)) vx += speed;
        
        // Sprint with Shift
        if (IsKeyDown(Key::LEFT_SHIFT)) {
            vx *= 2.0f;
            vz *= 2.0f;
        }
        
        // Simple ground check (y <= 1)
        isGrounded = (pos[1] <= 1.0f);
        
        // Apply gravity
        if (!isGrounded) {
            yVelocity += gravity * dt;
        } else {
            yVelocity = 0.0f;
            if (pos[1] < 1.0f) {
                entity.SetPosition(pos[0], 1.0f, pos[2]);
            }
        }
        
        // Jump
        if (IsKeyPressed(Key::SPACE) && isGrounded) {
            yVelocity = jumpForce;
            Print("Player jumped!");
        }
        
        vy = yVelocity;
        
        entity.SetVelocity(vx, vy, vz);
        
        // Debug visualization
        if (IsKeyDown(Key::F)) {
            DrawSphere(pos[0], pos[1], pos[2], 2.0f);
        }
    }
    
    void OnDestroy() {
        Print("Player destroyed");
    }
}
