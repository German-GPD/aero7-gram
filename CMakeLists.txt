cmake_minimum_required(VERSION 3.16)
project(AERO7GRAM VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(NOT WIN32)
    message(FATAL_ERROR "AERO7 GRAM targets Windows 7 SP1 only.")
endif()

if(NOT MSVC)
    message(FATAL_ERROR "Use the supported MSVC toolchain for Windows 7 builds.")
endif()

find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets)

add_executable(AERO7GRAM WIN32
    src/main.cpp
)

target_link_libraries(AERO7GRAM PRIVATE
    Qt5::Widgets
    dwmapi
)

target_compile_definitions(AERO7GRAM PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    WINVER=0x0601
    _WIN32_WINNT=0x0601
    NTDDI_VERSION=0x06010000
)

target_compile_options(AERO7GRAM PRIVATE
    /W4
    /permissive-
)

set_target_properties(AERO7GRAM PROPERTIES
    OUTPUT_NAME "AERO7 GRAM"
)

install(TARGETS AERO7GRAM RUNTIME DESTINATION .)
