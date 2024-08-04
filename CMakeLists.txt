cmake_minimum_required(VERSION 3.0)

enable_language(C CXX ASM)

set(CMAKE_CXX_STANDARD 17)

set(IS_64BIT FALSE)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(IS_64BIT TRUE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-delete-null-pointer-checks -stdlib=libc++")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

string(TIMESTAMP BUILD_TIMESTAMP "%Y%m%d-%H%M%S" UTC)

set(IS_ARM_BUILD FALSE)
set(IS_ARMHF_BUILD FALSE)
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
    set(IS_ARM_BUILD TRUE)
    set(IS_ARMHF_BUILD TRUE)
elseif(BUILD_X86)
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -m32")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
    if(APPLE)
        set(CMAKE_LIBRARY_ARCHITECTURE "i386-apple-darwin")
        set(CMAKE_CXX_COMPILER_TARGET "i686-apple-darwin")
    else()
        set(CMAKE_LIBRARY_ARCHITECTURE "i386-linux-gnu")
        set(CMAKE_CXX_COMPILER_TARGET "i686-linux-gnu")
    endif()
endif()

set(DEB_OS_NAME "none" CACHE STRING "Specifies the target OS for building the debs (ubuntu-xenial, ubuntu-bionic, ubuntu-disco)")
mark_as_advanced(DEB_OS_NAME)

add_subdirectory(logger)
add_subdirectory(file-util)
add_subdirectory(properties-parser)
add_subdirectory(arg-parser)

add_subdirectory(mcpelauncher-linker)
add_subdirectory(libc-shim)

add_subdirectory(minecraft-imported-symbols)
add_subdirectory(mcpelauncher-common)
add_subdirectory(mcpelauncher-core)

add_subdirectory(android-support-headers)
add_subdirectory(mcpelauncher-server)

