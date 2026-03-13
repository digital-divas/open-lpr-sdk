# open-lpr-sdk

High performance **on-device license plate recognition (LPR)** focused on **Brazilian / Mercosur plates**.

This project provides a lightweight SDK capable of detecting and recognizing Brazilian license plates directly on-device using modern deep learning models.

The SDK is designed to run on:

- iOS
- Android
- Linux
- macOS

without requiring cloud APIs.

## Features

- 🇧🇷 Optimized for **Brazilian / Mercosur license plates**
- 🚗 Supports **car and motorcycle plates**
- ⚡ Real-time performance on mobile devices
- 📦 Fully **on-device inference**
- 🧠 Uses modern deep learning models
- 🧩 C++ core with cross-platform support
- 🔌 Easy integration into iOS / Android apps

## How it works

_The models used by the sdk were generated using the script `export_models.py`._

The pipeline consists of two neural networks:

- Plate Detector - [YOLOv11x: Detecção de Placas Brasileiras](https://huggingface.co/wh0am-i/yolov11x-BrPlate) by `wh0am-i`.
- Plate OCR - [fast-plate-ocr](https://github.com/ankandrew/fast-plate-ocr) by `ankandrew`

## build android

```sh
mkdir build-android
cd build-android

cmake .. \
 -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
 -DANDROID_ABI=arm64-v8a \
 -DANDROID_PLATFORM=android-24 \
 -DOpenCV_DIR=/path/OpenCV-android-sdk/sdk/native/jni

cmake --build .
```
