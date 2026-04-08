#!/bin/bash
set -e
NDK="${ANDROID_NDK_ROOT:-$HOME/Library/Android/sdk/ndk/25.2.9519653}"
for ABI in arm64-v8a armeabi-v7a; do
    cmake -B "build-android/$ABI" \
        -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="$ABI" \
        -DANDROID_PLATFORM=android-28 \
        -DFILAMENT_DIR="${FILAMENT_DIR}" \
        -DBUILD_TESTING=OFF \
        -DCMAKE_BUILD_TYPE=Release
    cmake --build "build-android/$ABI" --config Release
done
mkdir -p platform/android/src/main/jniLibs/{arm64-v8a,armeabi-v7a}
cp build-android/arm64-v8a/core/libdice3d.so platform/android/src/main/jniLibs/arm64-v8a/
cp build-android/armeabi-v7a/core/libdice3d.so platform/android/src/main/jniLibs/armeabi-v7a/
echo "Android .so files placed. Run gradle assembleRelease for AAR."
