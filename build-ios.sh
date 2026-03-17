#!/usr/bin/env bash

set -e

BUILD_DIR=build-ios
IOS_PLATFORM=OS
SIM_PLATFORM=SIMULATOR

rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR

echo "Building iOS device (arm64)..."

cmake -B $BUILD_DIR/device \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0

cmake --build $BUILD_DIR/device --config Release

# echo "Building iOS simulator..."

# cmake -B $BUILD_DIR/simulator \
#     -DCMAKE_SYSTEM_NAME=iOS \
#     -DCMAKE_OSX_ARCHITECTURES=arm64 \
#     -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 \
#     -DCMAKE_OSX_SYSROOT=iphonesimulator

# cmake --build $BUILD_DIR/simulator --config Release

libtool -static \
  $BUILD_DIR/device/liblpr_sdk_static.a \
  third_party/onnxruntime/ios/onnxruntime.xcframework/ios-arm64/libonnxruntime.a \
  -o $BUILD_DIR/device/liblpr_combined.a

xcodebuild -create-xcframework \
 -library $BUILD_DIR/device/liblpr_combined.a \
 -headers include \
 -output $BUILD_DIR/LprSdk.xcframework
