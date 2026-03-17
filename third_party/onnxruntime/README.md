# onnxruntime

using version 1.24.3

<https://github.com/microsoft/onnxruntime/releases/tag/v1.24.3>
<https://repo1.maven.org/maven2/com/microsoft/onnxruntime/onnxruntime-android/1.24.3/>

ios version had to be built by hand:

```sh
git clone https://github.com/microsoft/onnxruntime
cd onnxruntime
git checkout v1.24.3

./build.sh \
  --config Release \
  --ios \
  --apple_sysroot iphoneos \
  --osx_arch arm64 \
  --apple_deploy_target 13.0 \
  --use_xcode

cd build/iOS/Release/Release-iphoneos
libtool -static \
 libonnxruntime_common.a \
 libonnxruntime_framework.a \
 libonnxruntime_graph.a \
 libonnxruntime_mlas.a \
 libonnxruntime_optimizer.a \
 libonnxruntime_providers.a \
 libonnxruntime_session.a \
 libonnxruntime_util.a \
 -o libonnxruntime.a

./build.sh \
  --config Release \
  --ios \
  --apple_sysroot iphonesimulator \
  --osx_arch arm64 \
  --apple_deploy_target 13.0 \
  --use_xcode

cd build/iOS/Release/Release-iphonesimulator
libtool -static \
 libonnxruntime_common.a \
 libonnxruntime_framework.a \
 libonnxruntime_graph.a \
 libonnxruntime_mlas.a \
 libonnxruntime_optimizer.a \
 libonnxruntime_providers.a \
 libonnxruntime_session.a \
 libonnxruntime_util.a \
 -o libonnxruntime.a

cd build/iOS/Release
xcodebuild -create-xcframework \
 -library Release-iphoneos/libonnxruntime.a \
 -library Release-iphonesimulator/libonnxruntime.a \
 -headers ../../../include \
 -output onnxruntime.xcframework
```

```sh
# simulator
./build.sh \
  --config Release \
  --ios \
  --apple_sysroot iphonesimulator \
  --osx_arch arm64 \
  --apple_deploy_target 13.0 \
  --use_xcode \
  --skip_tests \
  --cmake_extra_defines onnxruntime_BUILD_SHARED_LIB=OFF

./build.sh \
  --config Release \
  --ios \
  --apple_sysroot iphoneos \
  --osx_arch arm64 \
  --apple_deploy_target 13.0 \
  --use_xcode \
  --skip_tests \
  --cmake_extra_defines onnxruntime_BUILD_SHARED_LIB=OFF
```
