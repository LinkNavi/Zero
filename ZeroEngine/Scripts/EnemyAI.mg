// EnemyAI.mg - Unity-style enemy AI in Magolor

class EnemyAI {
    // Configuration
    let f32 detectRange = 8.0;
    let f32 attackRange = 3.0;
    let f32 moveSpeed = 2.5;
    
    // State (0=Idle, 1=Chase, 2=Attack)
    let f32 state = 0.0;
    let f32 attackTimer = 0.0;
    
    // Target position (would be set by engine)
    let f32 targetX = 0.0;
    let f32 targetY = 0.0;
    let f32 targetZ = 0.0;
    
    void fn OnStart() {
        Log("Enemy AI started!");
    }
    
    void fn OnUpdate() {
        // Calculate distance to target
        let f32 dx = targetX - position_x;
        let f32 dy = targetY - position_y;
        let f32 dz = targetZ - position_z;
        let f32 distance = sqrt(dx * dx + dy * dy + dz * dz);
        
        // State machine
        if (state < 0.5) {
            IdleBehavior();
            if (distance < detectRange) {
                state = 1.0;
                Log("Player detected!");
            }
        } elif (state < 1.5) {
            ChasePlayer(distance, dx, dz);
            if (distance < attackRange) {
                state = 2.0;
                Log("In attack range!");
            } elif (distance > detectRange * 1.5) {
                state = 0.0;
                Log("Lost player");
            }
        } else {
            AttackPlayer();
            if (distance > attackRange * 1.2) {
                state = 1.0;
            }
        }
    }
    
    void fn IdleBehavior() {
        // Bob up and down
        let f32 bobAmount = sin(time * 2.0) * 0.2;
        position_y = 1.0 + bobAmount;
    }
    
    void fn ChasePlayer(f32 dist, f32 dx, f32 dz) {
        if (dist > 0.1) {
            // Normalize direction
            let f32 dirX = dx / dist;
            let f32 dirZ = dz / dist;
            
            // Move toward target
            let f32 moveX = dirX * moveSpeed * delta_time;
            let f32 moveZ = dirZ * moveSpeed * delta_time;
            Translate(moveX, 0.0, moveZ);
        }
    }
    
    void fn AttackPlayer() {
        attackTimer = attackTimer + delta_time;
        if (attackTimer >= 1.0) {
            Log("Enemy attacks!");
            attackTimer = 0.0;
        }
    }
    
    void fn OnDestroy() {
        Log("Enemy destroyed");
    }
}
