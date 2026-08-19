cmake_minimum_required(VERSION 3.22)

project(PixelForge16 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(PixelForge16 WIN32
    src/main.cpp
)

target_compile_definitions(PixelForge16 PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if(MSVC)
    target_compile_options(PixelForge16 PRIVATE /W4 /permissive-)
endif()

target_link_libraries(PixelForge16 PRIVATE
    user32
    gdi32
    comdlg32
    shell32
)

