#!/bin/bash
# Complete build script for Debian

echo "=== Zero Engine Build Script for Debian ==="
echo ""

# Step 1: Install shader compiler if needed
if ! command -v glslc &> /dev/null && ! command -v glslangValidator &> /dev/null; then
    echo "Shader compiler not found. Installing..."
    sudo apt update
    sudo apt install -y shaderc
fi

# Step 2: Create shaders directory if it doesn't exist
mkdir -p shaders

# Step 3: Compile shaders
echo "Compiling shaders..."
if command -v glslc &> /dev/null; then
    glslc shaders/shader_glb.vert -o shaders/vert_glb.spv
    glslc shaders/shader_glb.frag -o shaders/frag_glb.spv
elif command -v glslangValidator &> /dev/null; then
    glslangValidator -V shaders/shader_glb.vert -o shaders/vert_glb.spv
    glslangValidator -V shaders/shader_glb.frag -o shaders/frag_glb.spv
else
    echo "ERROR: No shader compiler found!"
    exit 1
fi

if [ -f "shaders/vert_glb.spv" ] && [ -f "shaders/frag_glb.spv" ]; then
    echo "✓ Shaders compiled successfully"
else
    echo "ERROR: Shader compilation failed!"
    exit 1
fi

# Step 4: Build the project
echo ""
echo "Building project..."
Zora build
if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Build successful!"
    echo ""
    echo "To run:"
    echo "  cd ./target/dev/"
    echo "  ./Zero"
else
    echo "ERROR: Build failed!"
    exit 1
fi
