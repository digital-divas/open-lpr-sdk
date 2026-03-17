#!/usr/bin/env bash

set -e

NDK=${ANDROID_NDK}
API=24
ABIS=(
  arm64-v8a
  armeabi-v7a
  x86_64
)

OUT=android/openlprsdk/src/main

for ABI in "${ABIS[@]}"; do

  BUILD_DIR=build-android-$ABI

  echo "Building for $ABI"

  rm -rf $BUILD_DIR
  mkdir $BUILD_DIR
  cd $BUILD_DIR

  cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=$ABI \
    -DANDROID_PLATFORM=android-$API

  cmake --build . --config Release

  cd ..

  mkdir -p $OUT/jniLibs/$ABI

  cp $BUILD_DIR/liblpr_sdk_shared.so $OUT/jniLibs/$ABI/
  cp third_party/onnxruntime/android/lib/$ABI/libonnxruntime.so $OUT/jniLibs/$ABI/

done
