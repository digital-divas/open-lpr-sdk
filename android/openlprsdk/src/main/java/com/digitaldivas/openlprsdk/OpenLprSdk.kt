package com.digitaldivas.openlprsdk

import android.util.Log

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
    private val TAG = "OpenLprSdk"

    init {
        Log.d(TAG, "Loading native libraries...")
        System.loadLibrary("openlpr_jni")
        System.loadLibrary("lpr_sdk_shared")
        Log.d(TAG, "Native libraries loaded")

        Log.d(TAG, "Initializing LPR engine...")
        enginePtr = nativeCreate()
        Log.d(TAG, "LPR engine initialized")
    }

    fun close() {
        if (enginePtr != 0L) {
            nativeDestroy(enginePtr)
            enginePtr = 0L
            Log.d(TAG, "LPR engine destroyed")
        }
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
