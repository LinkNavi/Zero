
#!/bin/bash
# Build and run ZeroPhysics

# Make sure the script exits on error
set -e

# Compile
bear -- g++ -Iinclude src/ZeroPhysics.cpp src/ZeroPhysicsMesh.cpp src/main.cpp -o ZeroPhysics \
    -lraylib -lm -lpthread -ldl -lGL -Wall -Wextra -Wpedantic -O2

# Run
./ZeroPhysics
