class Rotator {
    construct new() {
        _speed = 45.0  // Degrees per second
        _angle = 0
    }

    update(dt) {
        var entity = Entity.new()
        var pos = entity.getPosition()
        
        _angle = _angle + (_speed * dt)
        if (_angle > 360) _angle = _angle - 360
        
        // Bob up and down
        var y = 1.0 + ((_angle * 0.01745).sin * 0.5)
        entity.setPosition(pos[0], y, pos[2])
    }
}
