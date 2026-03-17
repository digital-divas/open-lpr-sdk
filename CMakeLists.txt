cmake_minimum_required(VERSION 3.16)

project(lpr_sdk)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)

include_directories(include)

#
# -------- ONNXRUNTIME CONFIG ----------
#

set(ONNXRUNTIME_ROOT ${CMAKE_SOURCE_DIR}/third_party/onnxruntime)

if(ANDROID)
    set(ONNXRUNTIME_DIR ${ONNXRUNTIME_ROOT}/android)
elseif(IOS)
    set(ONNXRUNTIME_DIR ${ONNXRUNTIME_ROOT}/ios)
elseif(APPLE)
    set(ONNXRUNTIME_DIR ${ONNXRUNTIME_ROOT}/macos)
else()
    set(ONNXRUNTIME_DIR ${ONNXRUNTIME_ROOT}/linux)
endif()

include_directories(${ONNXRUNTIME_DIR}/include)

#
# Import ONNX Runtime library
#

if(ANDROID)

    add_library(
        onnxruntime
        SHARED
        IMPORTED
    )

    set_target_properties(
        onnxruntime
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/third_party/onnxruntime/android/lib/${ANDROID_ABI}/libonnxruntime.so
    )
elseif(IOS)

    if(CMAKE_OSX_SYSROOT MATCHES "iphonesimulator")
        message(FATAL_ERROR "ONNX Runtime not available for simulator")
    endif()


    add_library(onnxruntime STATIC IMPORTED)

    set_target_properties(
        onnxruntime
        PROPERTIES IMPORTED_LOCATION
        ${ONNXRUNTIME_DIR}/onnxruntime.xcframework/ios-arm64/libonnxruntime.a
    )

elseif(APPLE)

    add_library(onnxruntime SHARED IMPORTED)

    set_target_properties(
        onnxruntime
        PROPERTIES IMPORTED_LOCATION
        ${ONNXRUNTIME_DIR}/libonnxruntime.1.23.2.dylib
    )

else()

    add_library(onnxruntime SHARED IMPORTED)

    set_target_properties(
        onnxruntime
        PROPERTIES IMPORTED_LOCATION
        ${ONNXRUNTIME_DIR}/libonnxruntime.so.1.23.2
    )

endif()

#
# -------- LPR SDK ----------
#

add_library(lpr_objects OBJECT
    src/lpr.cpp
    src/lpr_c.cpp
)

# static library (.a)
add_library(lpr_sdk_static STATIC
    $<TARGET_OBJECTS:lpr_objects>
)

target_link_libraries(
    lpr_sdk_static
    onnxruntime
)

if(NOT IOS)
    # dynamic library (.dylib / .so)
    add_library(lpr_sdk_shared SHARED
        $<TARGET_OBJECTS:lpr_objects>
    )

    target_link_libraries(
        lpr_sdk_shared
        onnxruntime
    )

endif()

#
# Desktop test binary
#

if(NOT IOS AND NOT ANDROID)

    add_executable(lpr_test
        tests/test.cpp
    )

    set_target_properties(lpr_test PROPERTIES
        BUILD_RPATH ${ONNXRUNTIME_DIR}
    )

    target_link_libraries(
        lpr_test
        lpr_sdk_static
        onnxruntime
    )

endif()