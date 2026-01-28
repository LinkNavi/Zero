#!/bin/bash

# Compile shaders using glslc (part of vulkan-tools)
echo "Compiling shaders..."

# Create shaders directory if it doesn't exist
mkdir -p shaders

# Compile lit shaders
glslc shaders/shader_lit.vert -o shaders/vert_lit.spv
glslc shaders/shader_lit.frag -o shaders/frag_lit.spv

echo "✓ Shaders compiled successfully!"
echo "  - shaders/vert_lit.spv"
echo "  - shaders/frag_lit.spv"
