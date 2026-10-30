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

option(AERO7_ENABLE_TDLIB "Enable real Telegram authorization through TDLib" OFF)
set(TDJSON_ROOT "" CACHE PATH "TDLib JSON installation prefix")

if(AERO7_ENABLE_TDLIB)
    find_path(TDJSON_INCLUDE_DIR
        NAMES td/telegram/td_json_client.h
        HINTS "${TDJSON_ROOT}/include"
    )
    find_library(TDJSON_LIBRARY
        NAMES tdjson
        HINTS "${TDJSON_ROOT}/lib" "${TDJSON_ROOT}/lib/x64" "${TDJSON_ROOT}/lib/x86"
    )
    if(NOT TDJSON_INCLUDE_DIR OR NOT TDJSON_LIBRARY)
        message(FATAL_ERROR "TDLib JSON headers/library not found. Set TDJSON_ROOT to a compatible TDLib installation.")
    endif()
endif()

add_executable(AERO7GRAM WIN32
    src/main.cpp
    src/telegram_auth.cpp
    src/telegram_auth.h
)

target_link_libraries(AERO7GRAM PRIVATE
    Qt5::Widgets
    dwmapi
    crypt32
    bcrypt
)

if(AERO7_ENABLE_TDLIB)
    target_compile_definitions(AERO7GRAM PRIVATE AERO7_WITH_TDLIB)
    target_include_directories(AERO7GRAM PRIVATE "${TDJSON_INCLUDE_DIR}")
    target_link_libraries(AERO7GRAM PRIVATE "${TDJSON_LIBRARY}")
endif()

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
