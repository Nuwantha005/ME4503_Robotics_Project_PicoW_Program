#!/usr/bin/env bash

set -euo pipefail

# Decoupled build path sitting safely on your native Linux filesystem
BUILD_DIR="~/.cache/pico_builds/pico_code"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Ensure the native build directory exists before running tools against it
mkdir -p "$BUILD_DIR"

if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
    echo "Configuring CMake..."
    # -S . tracks your source code on NTFS 
    # -B tracks all volatile metadata generation directly in /tmp
    cmake -S . -B "$BUILD_DIR"
fi

# Link compilation commands back to the source directory for LSPs/Agentic code tools
if [[ -f "$BUILD_DIR/compile_commands.json" ]]; then
    ln -sf "$BUILD_DIR/compile_commands.json" ./compile_commands.json
fi

echo "🔧 Building..."
cmake --build "$BUILD_DIR"

# Ensure the symlink is refreshed after the build generates files completely
if [[ -f "$BUILD_DIR/compile_commands.json" ]]; then
    ln -sf "$BUILD_DIR/compile_commands.json" ./compile_commands.json
fi

shopt -s nullglob
uf2_files=("$BUILD_DIR"/*.uf2)
shopt -u nullglob

if (( ${#uf2_files[@]} == 0 )); then
    echo "No UF2 file found in $BUILD_DIR"
    exit 1
fi

UF2_PATH="${uf2_files[0]}"

echo "⚡ Preparing device..."
# Force running Pico (-f) into USB BOOTSEL mode (-u)
picotool reboot -f -u || true

# Give your host OS time to disconnect the serial port 
# and find the Pico as a BOOTSEL mass storage device
sleep 1.5

echo "🚚 Loading UF2..."
picotool load "$UF2_PATH"

echo "🔄 Rebooting into application..."
picotool reboot

echo "✅ Done"
