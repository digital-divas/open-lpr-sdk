cmake_minimum_required(VERSION 3.18)
project(lpr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── OpenCV ────────────────────────────────────────────────────────────────────
find_package(OpenCV QUIET)
if(NOT OpenCV_FOUND)
    execute_process(
        COMMAND brew --prefix opencv
        OUTPUT_VARIABLE BREW_OCV_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    find_package(OpenCV REQUIRED HINTS "${BREW_OCV_PREFIX}/lib/cmake/opencv4")
endif()

# ── ONNX Runtime ──────────────────────────────────────────────────────────────
if(DEFINED ONNXRUNTIME_ROOT)
    set(ORT_INCLUDE "${ONNXRUNTIME_ROOT}/include")
    set(ORT_LIB    "${ONNXRUNTIME_ROOT}/lib")
else()
    execute_process(
        COMMAND brew --prefix onnxruntime
        OUTPUT_VARIABLE BREW_ORT_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(ORT_INCLUDE "${BREW_ORT_PREFIX}/include/onnxruntime")
    set(ORT_LIB    "${BREW_ORT_PREFIX}/lib")
    message(STATUS "ONNX Runtime (Homebrew): ${BREW_ORT_PREFIX}")
endif()

if(APPLE)
    set(ORT_SHARED_LIB "${ORT_LIB}/libonnxruntime.dylib")
else()
    set(ORT_SHARED_LIB "${ORT_LIB}/libonnxruntime.so")
endif()

# ── Embute modelos ONNX via .incbin ───────────────────────────────────────────
set(MODELS_DIR "${CMAKE_BINARY_DIR}/models")
file(MAKE_DIRECTORY ${MODELS_DIR})

set(EMBED_SCRIPT "${CMAKE_SOURCE_DIR}/embed_model.py")

foreach(MODEL whoami.onnx cct_xs_v1_global.onnx)
    string(REPLACE "." "_" MODEL_VAR ${MODEL})
    string(REPLACE "-" "_" MODEL_VAR ${MODEL_VAR})
    set(MODEL_SRC "${CMAKE_SOURCE_DIR}/${MODEL}")
    set(MODEL_OBJ "${MODELS_DIR}/${MODEL_VAR}.o")
    add_custom_command(
        OUTPUT  ${MODEL_OBJ}
        COMMAND python3 ${EMBED_SCRIPT} ${MODEL_SRC} ${MODEL_OBJ} ${MODEL_VAR}
        DEPENDS ${MODEL_SRC}
        COMMENT "Embutindo ${MODEL} via .incbin"
    )
    list(APPEND MODEL_OBJECTS ${MODEL_OBJ})
endforeach()

add_custom_target(embedded_models DEPENDS ${MODEL_OBJECTS})

# ── Sources compartilhados ────────────────────────────────────────────────────
set(LPR_SOURCES
    src/lpr.cpp
    src/lpr_c.cpp
    ${MODEL_OBJECTS}
)

set(LPR_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
    ${ORT_INCLUDE}
)

set(LPR_LIBS
    ${OpenCV_LIBS}
    ${ORT_SHARED_LIB}
)

# ── Shared library: liblpr.dylib ─────────────────────────────────────────────
add_library(lpr_shared SHARED ${LPR_SOURCES})
add_dependencies(lpr_shared embedded_models)
set_target_properties(lpr_shared PROPERTIES OUTPUT_NAME "lpr")
target_include_directories(lpr_shared PUBLIC ${LPR_INCLUDES})
target_link_libraries(lpr_shared PRIVATE ${LPR_LIBS})
set_target_properties(lpr_shared PROPERTIES
    BUILD_RPATH   "${ORT_LIB}"
    INSTALL_RPATH "${ORT_LIB}"
)

# ── Static library: liblpr.a ─────────────────────────────────────────────────
add_library(lpr_static STATIC ${LPR_SOURCES})
add_dependencies(lpr_static embedded_models)
set_target_properties(lpr_static PROPERTIES OUTPUT_NAME "lpr")
target_include_directories(lpr_static PUBLIC ${LPR_INCLUDES})
target_link_libraries(lpr_static PRIVATE ${LPR_LIBS})

# ── Test binary ───────────────────────────────────────────────────────────────
add_executable(lpr_run tests/test.cpp)
add_dependencies(lpr_run embedded_models)
target_include_directories(lpr_run PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(lpr_run PRIVATE lpr_shared ${OpenCV_LIBS})
