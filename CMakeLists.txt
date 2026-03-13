cmake_minimum_required(VERSION 3.16)

project(lpr_sdk)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_BUILD_TYPE Release)

include_directories(include)

add_library(lpr_objects OBJECT
    src/lpr.cpp
    src/lpr_c.cpp
)

# static library (.a)
add_library(lpr_sdk_static STATIC
    $<TARGET_OBJECTS:lpr_objects>
)

if(NOT IOS)
    # dynamic library (.dylib / .so)
    add_library(lpr_sdk_shared SHARED
        $<TARGET_OBJECTS:lpr_objects>
    )
endif()

if(NOT IOS AND NOT ANDROID)
    add_executable(lpr_test
        tests/test.cpp
    )

    target_link_libraries(lpr_test
        lpr_sdk_static
    )
endif()