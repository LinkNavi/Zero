class SelfDestruct {
    private float lifetime;
    private float timer;
    
    SelfDestruct() {
        lifetime = 5.0f;
        timer = 0.0f;
    }
    
    void Start() {
        Print("SelfDestruct initialized - will destroy in " + lifetime + " seconds");
    }
    
    void Update(float dt) {
        timer += dt;
        
        if (timer >= lifetime) {
            Print("Time's up! Destroying entity...");
            Entity@ entity = GetEntity();
            entity.Destroy();
        }
    }
    
    void OnDestroy() {
        Print("SelfDestruct entity destroyed");
    }
}
