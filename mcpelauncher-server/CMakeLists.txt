cmake_minimum_required (VERSION 3.6)

project(mcpelauncher-server LANGUAGES CXX ASM)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

add_executable(mcpelauncher-server src/main.cpp src/main.h src/console_reader.cpp src/stubs.cpp)
target_link_libraries(mcpelauncher-server logger properties-parser mcpelauncher-core argparser android-support-headers libc-shim)

