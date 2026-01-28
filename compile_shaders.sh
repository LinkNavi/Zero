#!/bin/bash

# Compile shaders using glslc (part of vulkan-tools)
echo "Compiling shaders..."

glslc shaders/shader.vert -o shaders/vert.spv
glslc shaders/shader.frag -o shaders/frag.spv

echo "✓ Shaders compiled successfully!"
echo "  - shaders/vert.spv"
echo "  - shaders/frag.spv"
