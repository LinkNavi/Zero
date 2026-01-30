set pagination off
set logging file stutter_log.txt
set logging on

break main
run

break Time::update
commands
    silent
    set $dt_ms = Time::deltaTime * 1000.0
    if $dt_ms > 20.0
        printf "[STUTTER] Frame: %llu, Time: %.3fms\n", Time::frameCount, $dt_ms
    else
        printf "[OK] Frame: %llu, Time: %.3fms\n", Time::frameCount, $dt_ms
    end
    continue
end

continue
