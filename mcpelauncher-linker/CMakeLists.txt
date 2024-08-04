cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 17)
enable_language(CXX)

project(mcpelauncher-linker)

# Not working hiding it to impl
# add_library(linker_private INTERFACE)
# target_include_directories(linker_private INTERFACE . include core/base/include core/liblog/include core/libcutils/include)
# target_compile_definitions(linker_private INTERFACE PATH_MAX=256 _GNU_SOURCE)
# target_compile_options(linker_private INTERFACE -include compat.h)

add_library(linker STATIC bionic/linker/rt.cpp bionic/linker/linker_gdb_support.cpp bionic/libc/bionic/bionic_call_ifunc_resolver.cpp bionic/linker/linker_dlwarning.cpp bionic/linker/dlfcn.cpp bionic/linker/linker_phdr.cpp bionic/linker/linker_soinfo.cpp bionic/linker/linker.cpp bionic/linker/linker_config.cpp bionic/linker/linker_utils.cpp bionic/linker/linker_debug.cpp bionic/linker/linker_block_allocator.cpp bionic/linker/linker_mapped_file_fragment.cpp bionic/linker/linker_relocate.cpp bionic/linker/linker_namespaces.cpp core/base/mapped_file.cpp bionic/linker/linker_globals.cpp bionic/linker/linker_main.cpp bionic/linker/linker_cfi.cpp bionic/linker/linker_sdk_versions.cpp bionic/linker/linker_logger.cpp core/base/file.cpp core/base/logging.cpp core/base/liblog_symbols.cpp bionic/libc/async_safe/async_safe_log.cpp core/base/stringprintf.cpp core/base/strings.cpp core/liblog/logger_write.cpp core/liblog/properties.cpp core/base/threads.cpp core/base/properties.cpp core/base/parsebool.cpp src/zip_archive_stream_entry.cc core/libziparchive/zip_archive.cc public_include/mcpelauncher/linker.h src/linker.cpp bionic/libdl/libdl.cpp)
target_link_libraries(linker PUBLIC z pthread ${LINKER_LIBS})
target_include_directories(linker PRIVATE include core/base/include core/liblog/include core/libcutils/include)
target_compile_definitions(linker PRIVATE PATH_MAX=256 _GNU_SOURCE)
target_compile_options(linker PRIVATE -include compat.h)
target_include_directories(linker PUBLIC public_include)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm.*|ARM.*)" OR CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64.*|AARCH64.*|arm64.*|ARM64.*)")
    target_sources(linker PRIVATE bionic/linker/arch/arm_neon/linker_gnu_hash_neon.cpp)
endif()

if(NOT APPLE)
    target_sources(linker PRIVATE bionic/libc/upstream-openbsd/lib/libc/string/strlcpy.c bionic/libc/upstream-openbsd/lib/libc/string/strlcat.c)
endif()