cmake_minimum_required(VERSION 3.16)
project(AERO7GRAM VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(NOT WIN32)
    message(FATAL_ERROR "AERO7 GRAM is currently a Windows-only project.")
endif()

find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets)

add_executable(AERO7GRAM WIN32
    src/main.cpp
)

target_link_libraries(AERO7GRAM PRIVATE Qt5::Widgets)

if(MSVC)
    target_compile_options(AERO7GRAM PRIVATE /W4 /permissive-)
    target_compile_definitions(AERO7GRAM PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
endif()

target_link_libraries(AERO7GRAM PRIVATE dwmapi)

set_target_properties(AERO7GRAM PROPERTIES
    OUTPUT_NAME "AERO7 GRAM"
)

install(TARGETS AERO7GRAM RUNTIME DESTINATION .)
