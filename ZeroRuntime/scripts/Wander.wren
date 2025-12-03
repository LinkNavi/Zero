class Wander {
    construct new() {
        _speed = 3.0
        _wanderRadius = 10.0
        _changeDirectionTime = 2.0
        _timer = 0
        _targetPos = Vector3.new(0, 1, 0)
        _arrivalThreshold = 0.5
    }

    start() {
        Debug.log("Wander AI initialized")
        pickNewTarget()
    }

    update(dt) {
        var entity = Entity.new()
        var pos = entity.getPosition()
        var currentPos = Vector3.new(pos[0], pos[1], pos[2])
        
        _timer = _timer + dt
        
        // Check if we've reached target or timer expired
        var distance = Vector3.distance(currentPos, _targetPos)
        if (distance < _arrivalThreshold || _timer >= _changeDirectionTime) {
            pickNewTarget()
            _timer = 0
        }
        
        // Move towards target
        var direction = (_targetPos - currentPos).normalized
        var velocity = direction * _speed
        entity.setVelocity(velocity.x, 0, velocity.z)
        
        // Rotate to face movement direction
        if (velocity.length > 0.1) {
            var angle = Math.degrees(Math.atan(direction.z / direction.x))
            // Adjust for quadrant
            if (direction.x < 0) angle = angle + 180
            var rot = entity.getRotation()
            
            // Smooth rotation using lerp
            var targetRot = Math.lerp(rot[1], angle, 5.0 * dt)
            entity.setRotation(rot[0], targetRot, rot[2])
        }
        
        // Debug visualization
        if (Input.isKeyDown(Key.J)) {
            Debug.drawLine(pos[0], pos[1], pos[2], _targetPos.x, _targetPos.y, _targetPos.z)
            Debug.drawSphere(_targetPos.x, _targetPos.y, _targetPos.z, _arrivalThreshold)
        }
        
        // React to player proximity (if they press K to ping)
        if (Input.isKeyPressed(Key.K)) {
            flee(currentPos)
        }
    }
    
    pickNewTarget() {
        var entity = Entity.new()
        var pos = entity.getPosition()
        
        // Random point within wander radius
        var randomAngle = (Time.getTime() * 137.5).sin * Math.pi * 2
        var randomDist = ((Time.getTime() * 234.7).cos + 1) * _wanderRadius * 0.5
        
        var x = pos[0] + (Math.cos(randomAngle) * randomDist)
        var z = pos[2] + (Math.sin(randomAngle) * randomDist)
        
        // Clamp to reasonable bounds
        x = Math.clamp(x, -15, 15)
        z = Math.clamp(z, -15, 15)
        
        _targetPos = Vector3.new(x, 1, z)
        Debug.log("New wander target: %(x), %(z)")
    }
    
    flee(fromPos) {
        // Move away from current position
        var awayDir = (fromPos - _targetPos).normalized
        _targetPos = fromPos + (awayDir * _wanderRadius)
        _targetPos = Vector3.new(
            Math.clamp(_targetPos.x, -15, 15),
            1,
            Math.clamp(_targetPos.z, -15, 15)
        )
        Debug.log("Fleeing! New target: %(_targetPos.x), %(_targetPos.z)")
    }
}
