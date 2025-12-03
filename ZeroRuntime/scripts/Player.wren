class Player {
    construct new() {
        _speed = 5.0
        _jumpForce = 10.0
    }

    start() {
        System.print("Player ready!")
    }

    update(dt) {
        var entity = Entity.new()
        var vel = entity.getVelocity()
        
        var vx = 0
        var vy = vel[1]  // Preserve Y velocity
        var vz = 0

        // WASD movement
        if (Input.isKeyDown(87)) vz = vz - _speed  // W
        if (Input.isKeyDown(83)) vz = vz + _speed  // S
        if (Input.isKeyDown(65)) vx = vx - _speed  // A
        if (Input.isKeyDown(68)) vx = vx + _speed  // D

        // Jump
        if (Input.isKeyPressed(32)) vy = _jumpForce

        entity.setVelocity(vx, vy, vz)
    }
}

