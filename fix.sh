#!/bin/bash

# Crash Analysis Script using coredumpctl
# For when ptrace is disabled

echo "=== Zero Engine Crash Analysis ==="
echo ""

# First, try to run and capture crash
export GLYCIN_USE_SANDBOX=0
export GTK_DISABLE_VALIDATION=1

echo "Running Zero Engine (will crash)..."
.build/debug/Zero 2>&1 | tee crash_log.txt

if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo ""
    echo "=== Crash detected! ==="
    echo ""
    
    # Wait a moment for coredump to be saved
    sleep 1
    
    # Try to get crash info from coredumpctl
    echo "Checking coredumpctl..."
    if command -v coredumpctl &> /dev/null; then
        echo ""
        echo "=== Recent crash info ==="
        coredumpctl list Zero | tail -5
        
        echo ""
        echo "=== Crash details ==="
        coredumpctl info Zero 2>/dev/null | grep -A 30 "PID:"
        
        echo ""
        echo "=== Stack trace ==="
        echo "Getting backtrace (may require password)..."
        coredumpctl gdb Zero --batch -ex "thread apply all bt" -ex "quit" 2>/dev/null
        
        if [ $? -ne 0 ]; then
            echo ""
            echo "Couldn't get automatic backtrace."
            echo "Run manually with:"
            echo "  coredumpctl gdb Zero"
            echo "  (gdb) bt"
            echo "  (gdb) thread apply all bt"
        fi
    else
        echo "coredumpctl not available"
    fi
    
    # Save crash log
    echo ""
    echo "Crash log saved to: crash_log.txt"
    
    # Provide manual debugging instructions
    echo ""
    echo "=== Manual Debugging ==="
    echo "To debug manually, you have two options:"
    echo ""
    echo "Option 1: Fix ptrace restrictions (temporary):"
    echo "  echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope"
    echo "  gdb .build/debug/Zero"
    echo "  (gdb) run"
    echo "  (gdb) bt"
    echo ""
    echo "Option 2: Use coredumpctl (requires systemd):"
    echo "  coredumpctl gdb Zero"
    echo "  (gdb) bt"
    echo "  (gdb) thread apply all bt"
    echo ""
    echo "Option 3: Run as root (not recommended):"
    echo "  sudo -E gdb .build/debug/Zero"
    echo "  (gdb) run"
    echo "  (gdb) bt"
fi
