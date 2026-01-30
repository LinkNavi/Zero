# Performance Bottleneck Detector
# Save as: profile_bottleneck.gdb
# Usage: gdb -x profile_bottleneck.gdb ./build/Zero

set pagination off
set print pretty on

break main
run

# Profile each major function
define time_function
    set $func_start = clock()
    finish
    set $func_end = clock()
    set $func_time = ($func_end - $func_start) / 1000000.0
    printf "%s took: %.3fms\n", $arg0, $func_time
end

printf "\n=== Profiling Major Functions ===\n\n"

# Profile beginFrame
break VulkanRenderer::beginFrame
commands
    silent
    set $begin_start = clock()
    finish
    set $begin_end = clock()
    set $begin_time = ($begin_end - $begin_start) / 1000000.0
    printf "beginFrame: %.3fms", $begin_time
    if $begin_time > 5.0
        printf " ⚠️ SLOW!"
    end
    printf "\n"
    continue
end

# Profile endFrame  
break VulkanRenderer::endFrame
commands
    silent
    set $end_start = clock()
    finish
    set $end_end = clock()
    set $end_time = ($end_end - $end_start) / 1000000.0
    printf "endFrame: %.3fms", $end_time
    if $end_time > 5.0
        printf " ⚠️ SLOW!"
    end
    printf "\n"
    continue
end

# Profile ECS update
break ECS::updateSystems
commands
    silent
    set $ecs_start = clock()
    finish
    set $ecs_end = clock()
    set $ecs_time = ($ecs_end - $ecs_start) / 1000000.0
    printf "ECS update: %.3fms", $ecs_time
    if $ecs_time > 5.0
        printf " ⚠️ SLOW!"
    end
    printf "\n"
    continue
end

# Profile render system specifically
break GLBRenderSystem::update
commands
    silent
    set $render_start = clock()
    finish
    set $render_end = clock()
    set $render_time = ($render_end - $render_start) / 1000000.0
    printf "Render system: %.3fms", $render_time
    if $render_time > 10.0
        printf " ⚠️ VERY SLOW!"
    end
    printf "\n"
    continue
end

# Track vkWaitForFences - THIS IS LIKELY THE CULPRIT
break vkWaitForFences
commands
    silent
    set $wait_start = clock()
    finish  
    set $wait_end = clock()
    set $wait_time = ($wait_end - $wait_start) / 1000000.0
    printf "  └─ vkWaitForFences: %.3fms", $wait_time
    if $wait_time > 30.0
        printf " 🔴 BLOCKING! (This is your bottleneck!)"
    end
    printf "\n"
    continue
end

# Track vkQueueSubmit
break vkQueueSubmit
commands
    silent
    printf "  └─ vkQueueSubmit called\n"
    continue
end

# Track vkQueuePresentKHR - VSync happens here
break vkQueuePresentKHR
commands
    silent
    set $present_start = clock()
    finish
    set $present_end = clock()
    set $present_time = ($present_end - $present_start) / 1000000.0
    printf "  └─ vkQueuePresentKHR: %.3fms", $present_time
    if $present_time > 30.0
        printf " 🔴 VSync wait! GPU not ready!"
    end
    printf "\n"
    continue
end

# Full frame timing
break Time::update
commands
    silent
    set $dt = Time::deltaTime * 1000.0
    set $frame = Time::frameCount
    
    printf "\n--- Frame %llu: %.2fms total ---\n", $frame, $dt
    
    if $frame > 5
        if $dt > 50.0
            printf "🔴 EXTREMELY SLOW FRAME!\n"
            printf "Expected: ~16.67ms for 60 FPS\n"
            printf "Actual: %.2fms (%.1f FPS)\n", $dt, 1000.0/$dt
        end
    end
    
    continue
end

printf "\n=== Starting profiler ===\n"
printf "Watch for 🔴 markers - those are your bottlenecks!\n\n"

continue
