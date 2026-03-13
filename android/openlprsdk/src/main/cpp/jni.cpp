#include <jni.h>
#include "lpr_c.h"

extern "C"
JNIEXPORT jlong JNICALL
Java_com_digitaldivas_openlprsdk_OpenLprSdk_nativeCreate(
        JNIEnv* env,
        jobject thiz) {

    LprEngineHandle* engine = lpr_create();

    return reinterpret_cast<jlong>(engine);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_digitaldivas_openlprsdk_OpenLprSdk_nativeDestroy(
        JNIEnv* env,
        jobject thiz,
        jlong ptr) {

    LprEngineHandle* engine =
        reinterpret_cast<LprEngineHandle*>(ptr);

    lpr_destroy(engine);
}

extern "C"
JNIEXPORT jobjectArray JNICALL
Java_com_digitaldivas_openlprsdk_OpenLprSdk_nativeProcess(
        JNIEnv* env,
        jobject thiz,
        jlong ptr,
        jbyteArray frame,
        jint width,
        jint height) {

    LprEngineHandle* engine =
        reinterpret_cast<LprEngineHandle*>(ptr);

    jbyte* frameData = env->GetByteArrayElements(frame, NULL);

    const int MAX_RESULTS = 16;
    LprDetection results[MAX_RESULTS];

    int count = lpr_process(
        engine,
        reinterpret_cast<uint8_t*>(frameData),
        width,
        height,
        results,
        MAX_RESULTS
    );

    env->ReleaseByteArrayElements(frame, frameData, 0);

    jclass detectionClass =
        env->FindClass("com/digitaldivas/openlprsdk/LprDetection");

    jmethodID ctor = env->GetMethodID(
        detectionClass,
        "<init>",
        "(Ljava/lang/String;FIIII)V"
    );

    jobjectArray arr =
        env->NewObjectArray(count, detectionClass, NULL);

    for (int i = 0; i < count; i++) {

        jstring plate = env->NewStringUTF(results[i].plate);

        jobject obj = env->NewObject(
            detectionClass,
            ctor,
            plate,
            results[i].confidence,
            results[i].x1,
            results[i].y1,
            results[i].x2,
            results[i].y2
        );

        env->SetObjectArrayElement(arr, i, obj);
    }

    return arr;
}