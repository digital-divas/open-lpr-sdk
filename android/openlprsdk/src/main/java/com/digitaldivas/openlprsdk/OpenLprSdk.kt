package com.digitaldivas.openlprsdk

data class LprDetection(
        val plate: String,
        val confidence: Float,
        val x1: Int,
        val y1: Int,
        val x2: Int,
        val y2: Int
)

class OpenLprSdk {

    private var enginePtr: Long = 0

    init {
        System.loadLibrary("openlpr_jni")
        System.loadLibrary("lpr_sdk_shared")

        enginePtr = nativeCreate()
    }

    fun close() {
        nativeDestroy(enginePtr)
    }

    fun process(frame: ByteArray, width: Int, height: Int): Array<LprDetection> {

        return nativeProcess(enginePtr, frame, width, height)
    }

    private external fun nativeCreate(): Long

    private external fun nativeDestroy(ptr: Long)

    private external fun nativeProcess(
            ptr: Long,
            frame: ByteArray,
            width: Int,
            height: Int
    ): Array<LprDetection>
}
