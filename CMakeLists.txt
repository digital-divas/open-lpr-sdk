cmake_minimum_required(VERSION 3.16)

project(lpr_sdk)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)

enable_language(ASM)

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
# -------- EMBED MODELS (.incbin) ----------
#

if(IOS)
    set(EMBED_PLATFORM ios)
elseif(APPLE)
    set(EMBED_PLATFORM darwin)
elseif(ANDROID)
    set(EMBED_PLATFORM android)
else()
    set(EMBED_PLATFORM linux)
endif()

set(MODELS_DIR ${CMAKE_BINARY_DIR}/models)
file(MAKE_DIRECTORY ${MODELS_DIR})

set(EMBED_SCRIPT ${CMAKE_SOURCE_DIR}/embed_model.py)

set(MODEL_OBJECTS)

foreach(MODEL whoami.onnx cct_xs_v1_global.onnx)

    string(REPLACE "." "_" MODEL_VAR ${MODEL})
    string(REPLACE "-" "_" MODEL_VAR ${MODEL_VAR})

    set(MODEL_SRC ${CMAKE_SOURCE_DIR}/models/${MODEL})
    set(MODEL_SRC_ASM ${MODELS_DIR}/${MODEL_VAR}.S)

    add_custom_command(
        OUTPUT ${MODEL_SRC_ASM}
        COMMAND python3 ${EMBED_SCRIPT}
                ${MODEL_SRC}
                ${MODEL_SRC_ASM}
                ${MODEL_VAR}
                ${EMBED_PLATFORM}
        DEPENDS ${MODEL_SRC}
        COMMENT "Embedding ${MODEL}"
    )

    list(APPEND MODEL_OBJECTS ${MODEL_SRC_ASM})

endforeach()

add_custom_target(models_embedded DEPENDS ${MODEL_OBJECTS})

#
# -------- LPR SDK (SIMPLE + WORKING) ----------
#

set(LPR_SOURCES
    src/lpr.cpp
    src/lpr_c.cpp
    ${MODEL_OBJECTS}
)

# STATIC LIB
add_library(lpr_sdk_static STATIC ${LPR_SOURCES})
add_dependencies(lpr_sdk_static models_embedded)

target_link_libraries(
    lpr_sdk_static
    onnxruntime
)

# SHARED LIB
if(NOT IOS)
    add_library(lpr_sdk_shared SHARED ${LPR_SOURCES})
    add_dependencies(lpr_sdk_shared models_embedded)

    target_link_libraries(
        lpr_sdk_shared
        onnxruntime
    )
endif()

#
# -------- Desktop test ----------
#

if(NOT IOS AND NOT ANDROID)

    include_directories(third_party/stb)

    add_executable(open_lpr
        tests/test.cpp
    )

    add_dependencies(open_lpr models_embedded)

    set_target_properties(open_lpr PROPERTIES
        BUILD_RPATH ${ONNXRUNTIME_DIR}
    )

    set_target_properties(open_lpr PROPERTIES
        INSTALL_RPATH "@loader_path"
    )

    target_link_libraries(
        open_lpr
        lpr_sdk_static
    )

endif()