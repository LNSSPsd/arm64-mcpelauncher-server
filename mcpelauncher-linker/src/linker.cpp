#include <mcpelauncher/linker.h>

#include "../bionic/linker/linker_soinfo.h"
#include "../bionic/linker/linker_debug.h"
#include <cstdlib>

void solist_init();
soinfo* soinfo_from_handle(void* handle);

namespace linker::libdl {
    std::unordered_map<std::string, void *> get_dl_symbols();
}

void linker::init() {
    const char * verbosity = getenv("MCPELAUNCHER_LINKER_VERBOSITY");
    if(verbosity) {
        g_ld_debug_verbosity = std::stoi(std::string(verbosity));
    }
    solist_init();
    linker::load_library("libdl.so", linker::libdl::get_dl_symbols());
}

void *linker::load_library(const char *name, const std::unordered_map<std::string, void *> &symbols) {
    return soinfo::load_library(name, symbols)->to_handle();
}

size_t linker::get_library_base(void *handle) {
    return soinfo_from_handle(handle)->base;
}

void linker::get_library_code_region(void *handle, size_t &base, size_t &size) {
    auto s = soinfo_from_handle(handle);
    for (auto i = 0; i < s->phnum; i++) {
        if (s->phdr[i].p_type == PT_LOAD && s->phdr[i].p_flags & PF_X) {
            base = s->base + s->phdr[i].p_vaddr;
            size = s->phdr[i].p_memsz;
        }
    }
}

extern "C" void __loader_assert(const char* file, int line, const char* msg) {
    fprintf(stderr, "linker assert failed at %s:%i: %s\n", file, line, msg);
    abort();
}

extern int do_dlclose(void* handle);

int linker::dlclose_unlocked(void* handle) {
    return do_dlclose(handle);
}