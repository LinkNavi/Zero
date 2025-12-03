class Orbiter {
    construct new() {
        _radius = 5.0
        _speed = 1.0
        _angle = 0
        _centerX = 0
        _centerZ = 0
    }

    start() {
        var entity = Entity.new()
        var pos = entity.getPosition()
        _centerX = pos[0]
        _centerZ = pos[2]
    }

    update(dt) {
        var entity = Entity.new()
        
        _angle = _angle + (_speed * dt)
        
        var x = _centerX + (_radius * _angle.cos)
        var z = _centerZ + (_radius * _angle.sin)
        
        entity.setPosition(x, 1.0, z)
    }
}
