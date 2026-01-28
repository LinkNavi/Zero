#!/bin/bash

# Zero Engine Launch Script
# Fixes GTK icon loading issues

# Disable GTK icon cache validation (fixes glycin/bwrap errors)
export GDK_DISABLE_DEPRECATED=1
export GTK_DISABLE_VALIDATION=1

# Alternative: Use X11 backend explicitly (avoiding Wayland issues)
# export GDK_BACKEND=x11

# Disable glycin sandboxing (the actual culprit)
export GLYCIN_USE_SANDBOX=0

# Run the engine from project root
echo "=== Zero Engine Launcher ==="
echo "Working directory: $(pwd)"
echo ""

if [ ! -f ".build/debug/Zero" ]; then
    echo "Error: Executable not found at .build/debug/Zero"
    echo "Make sure you've built the project first:"
    echo "  cd .build/debug && cmake ../.. && make -j\$(nproc)"
    exit 1
fi

if [ ! -d "shaders" ]; then
    echo "Error: shaders/ directory not found"
    echo "Make sure you're running from the project root directory"
    exit 1
fi

if [ ! -f "shaders/vert_lit.spv" ] || [ ! -f "shaders/frag_lit.spv" ]; then
    echo "Warning: Compiled shaders not found"
    echo "Compiling shaders now..."
    bash compile_shaders.sh
    if [ $? -ne 0 ]; then
        echo "Error: Failed to compile shaders"
        exit 1
    fi
fi

# Launch the engine
.build/debug/Zero "$@"
