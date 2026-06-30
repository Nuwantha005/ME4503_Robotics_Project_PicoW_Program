#!/usr/bin/env bash

set -euo pipefail

BUILD_DIR="build"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
    echo "Configuring CMake..."
    cmake -S . -B "$BUILD_DIR"
fi

echo "🔧 Building..."
cmake --build "$BUILD_DIR"

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
# We use || true because if the Pico is ALREADY in BOOTSEL mode, this will safely skip
picotool reboot -f -u || true

# CRITICAL: Give your host OS time to disconnect the serial port 
# and find the Pico as a BOOTSEL mass storage device
sleep 1.5

echo "🚚 Loading UF2..."
picotool load "$UF2_PATH"

echo "🔄 Rebooting into application..."
picotool reboot

echo "✅ Done"