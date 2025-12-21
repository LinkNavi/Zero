// EnemyAI.mg - Enemy AI that dynamically finds and chases the player

class EnemyAI {
    // Settings
    let f32 detectRange = 10.0;
    let f32 attackRange = 2.0;
    let f32 moveSpeed = 3.0;
    
    // State: 0=idle, 1=chase, 2=attack
    let f32 state = 0.0;
    let f32 attackTimer = 0.0;
    
    // Player handle (found dynamically)
    let f32 playerHandle = 0.0;
    
    // Bob animation
    let f32 startY = 0.0;
    let f32 bobTimer = 0.0;
    
    void fn OnStart() {
        startY = position_y;
        
        // Find player dynamically by name
        playerHandle = FindByName(0); // 0 = "Player" in string table
        
        if (IsValidEntity(playerHandle) > 0.5) {
            Log(100); // "Found player!"
        } else {
            Log(101); // "Player not found"
        }
    }
    
    void fn OnUpdate() {
        // Check if player still exists
        if (IsValidEntity(playerHandle) < 0.5) {
            // Try to find player again
            playerHandle = FindByName(0);
            if (IsValidEntity(playerHandle) < 0.5) {
                DoIdle();
                return;
            }
        }
        
        // Get distance to player
        let f32 dist = DistanceTo(playerHandle);
        
        // State machine
        if (state < 0.5) {
            // Idle state
            DoIdle();
            if (dist < detectRange) {
                state = 1.0;
                Log(102); // "Player detected!"
            }
        } else if (state < 1.5) {
            // Chase state
            DoChase(dist);
            if (dist < attackRange) {
                state = 2.0;
                Log(103); // "Attacking!"
            } else if (dist > detectRange * 1.5) {
                state = 0.0;
                Log(104); // "Lost player"
            }
        } else {
            // Attack state
            DoAttack();
            if (dist > attackRange * 1.2) {
                state = 1.0;
            }
        }
    }
    
    void fn DoIdle() {
        // Bob up and down
        bobTimer = bobTimer + delta_time * 2.0;
        let f32 bobAmount = sin(bobTimer) * 0.2;
        position_y = startY + bobAmount;
    }
    
    void fn DoChase(f32 dist) {
        if (dist > 0.1) {
            // Move towards player
            MoveTowardsEntity(playerHandle, moveSpeed * delta_time);
            
            // Look at player
            LookAtEntity(playerHandle);
        }
    }
    
    void fn DoAttack() {
        attackTimer = attackTimer + delta_time;
        if (attackTimer >= 1.0) {
            Log(105); // "Enemy attacks!"
            attackTimer = 0.0;
        }
    }
}
