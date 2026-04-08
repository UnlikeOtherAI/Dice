#!/bin/bash
set -e
mkdir -p dist
xcodebuild -create-xcframework \
    -library build-ios/device/core/libdice3d.a \
    -headers core/include \
    -library build-ios/sim/core/libdice3d.a \
    -headers core/include \
    -output dist/Dice3D.xcframework
echo "XCFramework built at dist/Dice3D.xcframework"
