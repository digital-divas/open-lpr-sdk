cmake_minimum_required(VERSION 3.16)

project(openlpr_jni)

include_directories(../../../../../include)

add_library(
    openlpr_jni
    SHARED
    jni.cpp
)

add_library(
    lpr_sdk_shared
    SHARED
    IMPORTED
)

set_target_properties(
    lpr_sdk_shared
    PROPERTIES IMPORTED_LOCATION
    ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/liblpr_sdk_shared.so
)

add_library(
    onnxruntime
    SHARED
    IMPORTED
)

set_target_properties(
    onnxruntime
    PROPERTIES IMPORTED_LOCATION
    ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/libonnxruntime.so
)

find_library(log-lib log)

target_link_libraries(
    openlpr_jni
    lpr_sdk_shared
    onnxruntime
    ${log-lib}
)