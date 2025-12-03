class SelfDestruct {
    construct new() {
        _lifetime = 5.0
        _timer = 0
    }

    update(dt) {
        _timer = _timer + dt
        
        if (_timer >= _lifetime) {
            System.print("Destroying entity")
            var entity = Entity.new()
            entity.destroy()
        }
    }
}
