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

## Installation

### Android (Gradle)

The SDK is automatically downloaded during build time.

1. Add to your build.gradle (app)

    ```gradle
    def lprVersion = "1.0.0"
    def lprSdkFile = file("$buildDir/openlprsdk-$lprVersion.aar")

    tasks.register("downloadLprSdk") {
        doLast {
            if (lprSdkFile.exists()) {
                println("OpenLPR SDK already exists")
                return
            }

            def url = "https://github.com/digital-divas/open-lpr-sdk/releases/download/v${lprSdkVersion}/open-lpr-sdk-android-${lprSdkVersion}.aar"

            println("Downloading OpenLPR SDK ${lprSdkVersion}...")

            lprSdkFile.parentFile.mkdirs()

            new URL(url).withInputStream { input ->
                lprSdkFile.withOutputStream { output ->
                    output << input
                }
            }

            println("Downloaded OpenLPR SDK successfully.")

        }
    }

    preBuild.dependsOn(downloadLprSdk)

    dependencies {
        implementation files(lprSdkFile)
    }
    ```

2. Usage in Kotlin

    ```kotlin
    import com.digitaldivas.openlprsdk.OpenLprSdk

    class MyDetector {

        private val sdk = OpenLprSdk()

        fun process() {
            val detections = sdk.process(frame, width, height)
        }

    }
    ```

### iOS (Swift Package Manager)

1. In Xcode

    ```code
    File → Add Package Dependencies...
    ```

    Paste the repository URL:

    ```code
    https://github.com/digital-divas/open-lpr-sdk-ios
    ```

2. Choose version

    Recommended:

    ```code
    Up to Next Major Version
    ```

3. Usage in Swift

    ```swift
    let sdk = lpr_create()

    let maxResults: Int32 = 16
    var detections = [LprDetection](repeating: LprDetection(), count: Int(maxResults))

    let count = frame.withUnsafeBufferPointer { framePtr in
      detections.withUnsafeMutableBufferPointer { resultsPtr in
        lpr_process(sdk, framePtr.baseAddress, Int32(width), Int32(height), resultsPtr.baseAddress, maxResults)
      }
    }

    if (count == 0) {
      logger.debug("None plate detected by OpenLpr")
      return nil
    }
    
    logger.debug("detections found: \(count)")
    
    let d = detections[0]

    let plate = withUnsafePointer(to: &detections[0].plate) {
        $0.withMemoryRebound(to: CChar.self, capacity: 16) {
            String(cString: $0)
        }
    }
    ```

## Distribution

| Platform | Method |
|---|---|
| Android | AAR via Gradle download |
| iOS | Swift Package Manager |
| MacOS | Github Releases |
| Linux | Github Releases |

### Notes

- ONNX models are embedded into the binary
- No external model download is required
- CPU-only (no GPU dependency)

## How it works

_The models used by the sdk were generated using the script `export_models.py`._

The pipeline consists of two neural networks:

- Plate Detector - [YOLOv11x: Detecção de Placas Brasileiras](https://huggingface.co/wh0am-i/yolov11x-BrPlate) by `wh0am-i`.
- Plate OCR - [fast-plate-ocr](https://github.com/ankandrew/fast-plate-ocr) by `ankandrew`
