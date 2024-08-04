#pragma once

#include <unordered_map>
#include <string>
#include <dlfcn.h>
#ifndef __BEGIN_DECLS
#define __BEGIN_DECLS extern "C" {
#endif
#ifndef __END_DECLS
#define __END_DECLS }
#endif
#ifndef __INTRODUCED_IN
#define __INTRODUCED_IN(...)
#endif

#ifdef __APPLE__
typedef off_t off64_t;
#endif
#include "../bionic/libc/include/android/dlext.h"

extern "C" {
    void* __loader_dlopen(const char* filename, int flags, const void* caller_addr);
    void* __loader_dlsym(void* handle, const char* symbol, const void* caller_addr);
    int __loader_dladdr(const void* addr, Dl_info* info);
    int __loader_dlclose(void* handle);
    char* __loader_dlerror();
    int __loader_dl_iterate_phdr(int (*cb)(struct dl_phdr_info* info, size_t size, void* data), void* data);
    void __loader_android_update_LD_LIBRARY_PATH(const char* ld_library_path);
    void* __loader_android_dlopen_ext(const char* filename,
                           int flags,
                           const android_dlextinfo* extinfo,
                           const void* caller_addr);
}

namespace linker {

    inline void *dlopen(const char* filename, int flags) {
        return __loader_dlopen(filename, flags, nullptr);
    }

    inline void *dlopen_ext(const char* filename, int flags, const android_dlextinfo* extinfo) {
        return __loader_android_dlopen_ext(filename, flags, extinfo, nullptr);
    }

    inline void *dlsym(void *handle, const char *symbol) {
        return __loader_dlsym(handle, symbol, nullptr);
    }

    inline int dladdr(const void* addr, Dl_info* info) {
        return __loader_dladdr(addr, info);
    }

    inline int dlclose(void* handle) {
        return __loader_dlclose(handle);
    }

    inline char *dlerror() {
        return __loader_dlerror();
    }

    inline int dl_iterate_phdr(int (*cb)(struct dl_phdr_info* info, size_t size, void *data), void *data) {
        return __loader_dl_iterate_phdr(cb, data);
    }

    inline void update_LD_LIBRARY_PATH(const char *ld_library_path) {
        __loader_android_update_LD_LIBRARY_PATH(ld_library_path);
    }

    void init();

    void *load_library(const char *name, const std::unordered_map<std::string, void*> &symbols);

    size_t get_library_base(void *handle);

    void get_library_code_region(void *handle, size_t &base, size_t &size);

    int dlclose_unlocked(void* handle);

}
