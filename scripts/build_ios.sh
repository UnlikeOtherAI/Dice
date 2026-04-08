#!/bin/bash
set -e

FILAMENT_DIR="${FILAMENT_DIR:-$(pwd)/vendor/filament-ios}"
BUILD_DIR="$(pwd)/build-ios"

# Device (arm64)
cmake -B "$BUILD_DIR/device" \
    -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake \
    -DPLATFORM=OS64 \
    -DDEPLOYMENT_TARGET=14.0 \
    -DFILAMENT_DIR="$FILAMENT_DIR" \
    -DBUILD_TESTING=OFF \
    -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR/device" --config Release

# Simulator (arm64 + x86_64)
cmake -B "$BUILD_DIR/sim" \
    -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake \
    -DPLATFORM=SIMULATORARM64 \
    -DDEPLOYMENT_TARGET=14.0 \
    -DFILAMENT_DIR="$FILAMENT_DIR" \
    -DBUILD_TESTING=OFF \
    -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR/sim" --config Release
