cmake_minimum_required(VERSION 3.16)

project(lpr_sdk)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_library(lpr_objects OBJECT
    src/lpr.cpp
)

# static library (.a)
add_library(lpr_sdk_static STATIC
    src/lpr.cpp
    src/lpr_c.cpp
)

# dynamic library (.dylib / .so)
add_library(lpr_sdk_shared SHARED
    src/lpr.cpp
    src/lpr_c.cpp
)

add_executable(lpr_test
    tests/test.cpp
)

target_link_libraries(lpr_test
    lpr_sdk_static
)