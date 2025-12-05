class Follower {
    private float speed;
    private Vector3 targetPos;
    
    Follower() {
        speed = 5.0f;
        targetPos = Vector3(0.0f, 1.0f, 0.0f);
    }
    
    void Start() {
        Print("Follower initialized - following origin");
    }
    
    void Update(float dt) {
        Entity@ entity = GetEntity();
        array<float>@ pos = entity.GetPosition();
        
        Vector3 currentPos(pos[0], pos[1], pos[2]);
        
        // Calculate direction to target
        Vector3 direction = (targetPos - currentPos).Normalized();
        
        // Move towards target
        float distance = Distance(currentPos, targetPos);
        
        if (distance > 0.5f) {
            Vector3 velocity = direction * speed;
            entity.SetVelocity(velocity.x, 0.0f, velocity.z);
        } else {
            entity.SetVelocity(0.0f, 0.0f, 0.0f);
        }
    }
    
    float Distance(const Vector3 &in a, const Vector3 &in b) {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        float dz = a.z - b.z;
        return sqrt(dx*dx + dy*dy + dz*dz);
    }
}
