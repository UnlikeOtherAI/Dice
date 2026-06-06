#!/bin/bash
# Build libdice3d.so for Android from the core C++ + JNI, linked against
# Filament's Android native libs. Outputs into platform/android/src/main/jniLibs/<abi>/.
set -e
cd "$(dirname "$0")/.."   # repo root

# Pick an installed NDK (override with ANDROID_NDK_ROOT). Filament 1.71.5's
# static libs use the newer libc++ ABI (abi:ne210000) → needs NDK r27 to match;
# r25 produced __throw_bad_function_call link errors.
NDK="${ANDROID_NDK_ROOT:-$HOME/Library/Android/sdk/ndk/27.1.12297006}"
FILAMENT_DIR="${FILAMENT_DIR:-$(pwd)/vendor/filament-android/filament}"
ABIS="${ABIS:-arm64-v8a}"   # space-separated; emulators on Apple Silicon + modern devices are arm64-v8a

[ -d "$NDK" ] || { echo "NDK not found at $NDK — set ANDROID_NDK_ROOT"; exit 1; }
[ -f "$FILAMENT_DIR/include/filament/Engine.h" ] || { echo "Filament not found at $FILAMENT_DIR"; exit 1; }

for ABI in $ABIS; do
    echo "=== building $ABI ==="
    cmake -B "build-android/$ABI" -S platform/android \
        -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="$ABI" \
        -DANDROID_PLATFORM=android-28 \
        -DFILAMENT_DIR="$FILAMENT_DIR" \
        -DCMAKE_BUILD_TYPE=Release
    cmake --build "build-android/$ABI" --config Release -j
    mkdir -p "platform/android/src/main/jniLibs/$ABI"
    cp "build-android/$ABI/libdice3d.so" "platform/android/src/main/jniLibs/$ABI/"
    echo "  -> platform/android/src/main/jniLibs/$ABI/libdice3d.so"
done
echo "Done. .so placed in jniLibs. Build the AAR with: gradle :dice3d:assembleRelease"
