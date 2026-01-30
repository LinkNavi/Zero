# stutter_debug.gdb - Comprehensive stutter detection

# Break on main
break main
run

# Track frame times
break Time::update
commands
    silent
    set $dt = Time::deltaTime
    set $dt_ms = $dt * 1000.0
    
    # Detect stutter (>20ms)
    if $dt_ms > 20.0
        printf "\n========== STUTTER DETECTED ==========\n"
        printf "Frame time: %.3fms (should be ~16.67ms)\n", $dt_ms
        printf "FPS: %.1f\n", 1.0 / $dt
        printf "Frame count: %llu\n", Time::frameCount
        
        # Show what's taking time
        backtrace 10
        
        printf "\nChecking subsystems...\n"
        
        # You can add more checks here
        printf "======================================\n\n"
    end
    continue
end

# Track fence waits (potential stall points)
break vkWaitForFences
commands
    silent
    printf "Waiting for fence...\n"
    finish
    printf "Fence wait complete\n"
    continue
end

# Track image acquisition (VSync wait)
break vkAcquireNextImageKHR
commands
    silent
    printf "Acquiring swapchain image...\n"
    finish
    printf "Image acquired\n"
    continue
end

continue
