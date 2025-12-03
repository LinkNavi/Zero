kclass Player {
    construct new() {
        _speed = 10.0
        _jumpForce = 15.0
        _isGrounded = true
        _yVelocity = 0
        _gravity = -30.0
    }

    start() {
        Debug.log("Player initialized!")
        var entity = Entity.new()
        Debug.log("Player entity ID: %(entity.getId())")
    }

    update(dt) {
        var entity = Entity.new()
        var pos = entity.getPosition()
        var vel = entity.getVelocity()
        
        var vx = 0
        var vy = vel[1]
        var vz = 0

        // WASD movement using Key constants
        if (Input.isKeyDown(Key.W)) vz = vz - _speed
        if (Input.isKeyDown(Key.S)) vz = vz + _speed
        if (Input.isKeyDown(Key.A)) vx = vx - _speed
        if (Input.isKeyDown(Key.D)) vx = vx + _speed

        // Sprint with Shift
        if (Input.isKeyDown(Key.LEFT_SHIFT)) {
            vx = vx * 2
            vz = vz * 2
        }

        // Simple ground check (y <= 1)
        _isGrounded = pos[1] <= 1.0

        // Apply gravity
        if (!_isGrounded) {
            _yVelocity = _yVelocity + (_gravity * dt)
        } else {
            _yVelocity = 0
            if (pos[1] < 1.0) {
                entity.setPosition(pos[0], 1.0, pos[2])
            }
        }

        // Jump
        if (Input.isKeyPressed(Key.SPACE) && _isGrounded) {
            _yVelocity = _jumpForce
            Debug.log("Player jumped!")
        }

        vy = _yVelocity

        entity.setVelocity(vx, vy, vz)

        // Debug visualization
        if (Input.isKeyDown(Key.F)) {
            Debug.drawSphere(pos[0], pos[1], pos[2], 2.0)
        }
    }
}
