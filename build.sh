#!/bin/bash
# set -e

shopt -s nullglob
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info() { echo -e "${GREEN}[INFO]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }

# Detect distro
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        case "$ID" in
            arch|manjaro|endeavouros) echo "arch" ;;
            debian|ubuntu|pop|linuxmint) echo "debian" ;;
            *) echo "unknown" ;;
        esac
    else
        echo "unknown"
    fi
}

# Install dependencies
install_deps() {
    local distro=$(detect_distro)
    info "Detected distro: $distro"
    
    case "$distro" in
        arch)
            local pkgs=""
            command -v glslc &>/dev/null || pkgs+="shaderc "
            command -v pkg-config &>/dev/null || pkgs+="pkgconf "
            ldconfig -p 2>/dev/null | grep -q libglfw || pkgs+="glfw "
            ldconfig -p 2>/dev/null | grep -q libvulkan || pkgs+="vulkan-icd-loader vulkan-headers "
            ldconfig -p 2>/dev/null | grep -q libassimp || pkgs+="assimp "
            
            if [ -n "$pkgs" ]; then
                info "Installing: $pkgs"
                sudo pacman -S --needed --noconfirm $pkgs
            fi
            ;;
        debian)
            local pkgs=""
            command -v glslc &>/dev/null || pkgs+="glslc "
            command -v pkg-config &>/dev/null || pkgs+="pkg-config "
            ldconfig -p 2>/dev/null | grep -q libglfw || pkgs+="libglfw3-dev "
            ldconfig -p 2>/dev/null | grep -q libvulkan || pkgs+="libvulkan-dev vulkan-tools "
            ldconfig -p 2>/dev/null | grep -q libassimp || pkgs+="libassimp-dev "
            
            if [ -n "$pkgs" ]; then
                info "Installing: $pkgs"
                sudo apt-get update
                sudo apt-get install -y $pkgs
            fi
            ;;
        *)
            warn "Unknown distro. Please install manually: glslc, glfw, vulkan, assimp"
            ;;
    esac
}



compile_shaders() {
    local shader_dir="shaders"
    local compiled=0
    local failed=0
    local found=0

    [ -d "$shader_dir" ] || {
        warn "No shaders directory found"
        return 0
    }

    command -v glslc &>/dev/null || error "glslc not found. Run: $0 deps"

    info "Compiling shaders..."

    for ext in vert frag comp geom; do
        for shader in "$shader_dir"/*."$ext"; do
            [ -f "$shader" ] || continue

            found=$((found + 1))

            local name
            name=$(basename "$shader")
            local output="$shader_dir/${name%.*}_${ext}.spv"

            info " → $name"

            if glslc "$shader" -o "$output"; then
                compiled=$((compiled + 1))
            else
                warn "Failed to compile: $name"
                failed=$((failed + 1))
            fi
        done
    done

    if [ "$found" -eq 0 ]; then
        warn "No shaders found"
        return 0
    fi

    info "Shaders: $compiled compiled, $failed failed"
}


# Build project
build_project() {
    local mode="${1:-dev}"
      info "Doing Shaders"
    compile_shaders
    
    info "Building ($mode)..."

    if command -v zora &>/dev/null; then
        zora build
    elif [ -f CMakeLists.txt ]; then
        mkdir -p build && cd build
        cmake .. -DCMAKE_BUILD_TYPE=Release
        make -j$(nproc)
        cd ..
    else
        error "No build system found (zora or CMake)"
    fi
}

# Run project
run_project() {
    local bin=""
    
    for path in "./target/dev/Zero" "./target/debug/Zero" "./build/zero_engine" "./Zero"; do
        if [ -x "$path" ]; then
            bin="$path"
            break
        fi
    done
    
    [ -z "$bin" ] && error "No executable found. Run: $0 build"
    
    info "Running: $bin"
    exec "$bin"
}

# Clean
clean_project() {
    info "Cleaning..."
    rm -rf target/ build/
    rm -f shaders/*.spv
    info "Done"
}

# Help
show_help() {
    cat << EOF
Zero Engine Build Script

Usage: $0 [command]

Commands:
    build       Build the project (default)
    run         Build and run
    shaders     Compile shaders only
    deps        Install dependencies
    clean       Remove build artifacts
    help        Show this help

Examples:
    $0              # Build
    $0 run          # Build and run
    $0 deps         # Install dependencies
EOF
}

# Main
case "${1:-build}" in
    build)      build_project ;;
    run)        build_project && run_project ;;
    shaders)    compile_shaders ;;
    deps)       install_deps ;;
    clean)      clean_project ;;
    help|-h)    show_help ;;
    *)          error "Unknown command: $1" ;;
esac
