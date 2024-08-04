#include <mcpelauncher/hybris_utils.h>
#include <mcpelauncher/path_helper.h>
#include <log.h>
#include <dlfcn.h>
#include <mcpelauncher/linker.h>

const char* HybrisUtils::TAG = "LinkerUtils";


bool HybrisUtils::loadLibrary(std::string path) {
    void* handle = linker::dlopen(PathHelper::findDataFile("libs/hybris/" + path).c_str(), 0);
    if (handle == nullptr) {
        Log::error(TAG, "Failed to load hybris library %s: %s", path.c_str(), linker::dlerror());
        return false;
    }
    return true;
}

void* HybrisUtils::loadLibraryOS(const char *name, std::string const &path, const char** symbols, std::unordered_map<std::string, void*> syms) {
    void* handle = dlopen(path.c_str(), RTLD_LAZY);
    if (handle == nullptr) {
        Log::error(TAG, "Failed to load OS library %s", path.c_str());
        return nullptr;
    }
    Log::trace(TAG, "Loaded OS library %s", path.c_str());
    int i = 0;
    while (true) {
        const char* sym = symbols[i];
        if (sym == nullptr)
            break;
        void* ptr = dlsym(handle, sym);
        if (ptr)
            syms[sym] = ptr;
        i++;
    }
    linker::load_library(name, syms);
    return handle;
}

void HybrisUtils::stubSymbols(const char *name, const char** symbols, void* stubfunc) {
    int i = 0;
    std::unordered_map<std::string, void*> syms;
    while (true) {
        const char* sym = symbols[i];
        if (sym == nullptr)
            break;
        syms[sym] = stubfunc;
        i++;
    }
    linker::load_library(name, syms);
}