#include <mcpelauncher/minecraft_utils.h>
#include <mcpelauncher/patch_utils.h>
#include <mcpelauncher/hybris_utils.h>
#include <mcpelauncher/hook.h>
#include <mcpelauncher/path_helper.h>
#include <mcpelauncher/minecraft_version.h>
#include <minecraft/imported/android_symbols.h>
#include <minecraft/imported/egl_symbols.h>
#include <minecraft/imported/libm_symbols.h>
#include <minecraft/imported/fmod_symbols.h>
#include <minecraft/imported/glesv2_symbols.h>
#include <minecraft/imported/libz_symbols.h>
#include <log.h>
#include <FileUtil.h>
#include <mcpelauncher/linker.h>
#include <libc_shim.h>
#include <stdexcept>
#include <cstring>
#if defined(__APPLE__) && defined(__aarch64__)
#include <libkern/OSCacheControl.h>
#include <pthread.h>
#endif

void MinecraftUtils::workaroundLocaleBug() {
    setenv("LC_ALL", "C", 1); // HACK: Force set locale to one recognized by MCPE so that the outdated C++ standard library MCPE uses doesn't fail to find one
}

std::unordered_map<std::string, void*> MinecraftUtils::getLibCSymbols() {
    std::unordered_map<std::string, void*> syms;
    for (auto const &s : shim::get_shimmed_symbols())
        syms[s.name] = s.value;
    return syms;
}

void* MinecraftUtils::loadLibM() {
#ifdef __APPLE__
    void* libmLib = HybrisUtils::loadLibraryOS("libm.so", "libm.dylib", libm_symbols, std::unordered_map<std::string, void*>{ {std::string("sincos"), (void*)__sincos }, {std::string("sincosf"), (void*)__sincosf } });
#elif defined(__FreeBSD__)
    void* libmLib = HybrisUtils::loadLibraryOS("libm.so", "libm.so", libm_symbols);
#else
    void* libmLib = HybrisUtils::loadLibraryOS("libm.so", "libm.so.6", libm_symbols);
#endif
    if (libmLib == nullptr)
        throw std::runtime_error("Failed to load libm");
    return libmLib;
}

void* MinecraftUtils::loadFMod() {
void* fmodLib = HybrisUtils::loadLibraryOS("libfmod.so", PathHelper::findDataFile(std::string("lib/native/") + getLibraryAbi() +
#ifdef __APPLE__
#if defined(__i386__)
    // Minecraft releases linked against libc++-shared have to use a newer version of libfmod
    // Throwing here allows using pulseaudio if available / starting the game without sound
    (linker::dlopen("libc++_shared.so", 0) ? throw std::runtime_error("Fmod removed i386 support, after deprecation by Apple") : "/libfmod.dylib")
#else
    "/libfmod.dylib"
#endif
#else
#ifdef __LP64__
    "/libfmod.so.12.0"
#else
    // Minecraft releases linked against libc++-shared have to use a newer version of libfmod
    (linker::dlopen("libc++_shared.so", 0) ? "/libfmod.so.12.0" : "/libfmod.so.10.20")
#endif
#endif
), fmod_symbols);
    if (fmodLib == nullptr)
        throw std::runtime_error("Failed to load fmod");
    return fmodLib;
}

void MinecraftUtils::stubFMod() {
    HybrisUtils::stubSymbols("libfmod.so", fmod_symbols, (void*) (void* (*)()) []() {
        Log::warn("Launcher", "FMod stub called");
        return (void*) nullptr;
    });
}

void MinecraftUtils::setupHybris() {
    HybrisUtils::loadLibraryOS("libz.so", 
#ifdef __APPLE__
    "libz.dylib"
#elif defined(__FreeBSD__)
    "libz.so"
#else
    "libz.so.1"
#endif
    , libz_symbols);
    HybrisUtils::hookAndroidLog();
    setupApi();
    linker::load_library("libOpenSLES.so", {});
    linker::load_library("libGLESv1_CM.so", {});

    linker::load_library("libstdc++.so", {});
    linker::load_library("libz.so", {}); // needed for <0.17
}

std::unordered_map<std::string, void*> MinecraftUtils::getApi() {
    std::unordered_map<std::string, void*> syms;
    // Deprecated use android liblog
#if !(defined(__APPLE__) && defined(__aarch64__))
    syms["mcpelauncher_log"] = (void*) Log::log;
    syms["mcpelauncher_vlog"] = (void*) Log::vlog;
#endif

    syms["mcpelauncher_preinithook2"] = (void*) (void (*)(const char*, void*, void*, void (*)(void*, void*))) [](const char*name, void*sym, void*user, void (*callback)(void*, void*)) {
        preinitHooks[name] = { sym, user, callback };
    };
    syms["mcpelauncher_preinithook"] = (void*) (void (*)(const char*, void*, void **)) [](const char*name, void*sym, void** orig) {
        auto&& def = [](void* user, void* orig) {
            *(void**)user = orig;
        };
        preinitHooks[name] = { sym, orig, orig ? def : nullptr };
    };

    syms["mcpelauncher_hook"] = (void*) (void* (*)(void*, void*, void**)) [](void* sym, void* hook, void** orig) {
        Dl_info i;
        if (!linker::dladdr(sym, &i)) {
            Log::error("Hook", "Failed to resolve hook for symbol %lx", (long unsigned) sym);
            return (void*) nullptr;
        }
        void* handle = linker::dlopen(i.dli_fname, 0);
        std::string tName = HookManager::translateConstructorName(i.dli_sname);
        auto ret = HookManager::instance.createHook(handle, tName.empty() ? i.dli_sname : tName.c_str(), hook, orig);
        linker::dlclose(handle);
        HookManager::instance.applyHooks();
        return (void*) ret;
    };

    syms["mcpelauncher_hook2"] = (void *) (void *(*)(void *, const char *, void *, void **))
            [](void *lib, const char *sym, void *hook, void **orig) {
                return (void *) HookManager::instance.createHook(lib, sym, hook, orig);
            };
    syms["mcpelauncher_hook2_add_library"] = (void *) (void (*)(void*)) [](void* lib) {
        HookManager::instance.addLibrary(lib);
    };
    syms["mcpelauncher_hook2_remove_library"] = (void *) (void (*)(void*)) [](void* lib) {
        HookManager::instance.removeLibrary(lib);
    };
    syms["mcpelauncher_hook2_delete"] = (void *) (void (*)(void*)) [](void* hook) {
        HookManager::instance.deleteHook((HookManager::HookInstance*) hook);
    };
    syms["mcpelauncher_hook2_apply"] = (void *) (void (*)()) []() {
        HookManager::instance.applyHooks();
    };
#if defined(__APPLE__) && defined(__aarch64__)
    syms["mcpelauncher_patch"] = (void *)+ [](void* address, void* data, size_t size) -> void* {
        pthread_jit_write_protect_np(0);
        auto ret = memcpy(address, data, size);
        sys_icache_invalidate(address, size);
        pthread_jit_write_protect_np(1);
        return ret;
    };
#else
    syms["mcpelauncher_patch"] = (void *)+ [](void* address, void* data, size_t size) -> void* {
        return memcpy(address, data, size);
    };
#endif
    syms["mcpelauncher_host_dlopen"] = (void *) dlopen;
    syms["mcpelauncher_host_dlsym"] = (void *) dlsym;
    syms["mcpelauncher_host_dlclose"] = (void *) dlclose;
    return syms;
}

void MinecraftUtils::setupApi() {
    linker::load_library("libmcpelauncher_mod.so", getApi());
}

std::unordered_map<std::string, MinecraftUtils::HookEntry> MinecraftUtils::preinitHooks;

void* MinecraftUtils::loadMinecraftLib(void *showMousePointerCallback, void *hideMousePointerCallback, void *fullscreenCallback) {
    linker::dlopen("libc++_shared.so", 0);

    android_dlextinfo extinfo;
    std::vector<mcpelauncher_hook_t> hooks;
#ifdef __arm__
// Workaround for v8 allocator crash Minecraft 1.16.100+ on a RaspberryPi2 running raspbian
// Shadow some new overrides with host allocator fixes the crash
// Seems to be unnecessary on a RaspberryPi4 running ubuntu arm64
    hooks.emplace_back(mcpelauncher_hook_t{ "_Znaj", (void*)((void*(*)(std::size_t))&::operator new[]) });
    hooks.emplace_back(mcpelauncher_hook_t{ "_Znwj", (void*)((void*(*)(std::size_t))&::operator new) });
    hooks.emplace_back(mcpelauncher_hook_t{ "_ZnwjSt11align_val_t", (void*)((void*(*)(std::size_t, std::align_val_t))&::operator new[]) });
// The Openssl cpuid setup seems to not work correctly and allways crashs with "invalid instruction" Minecraft 1.16.10 (beta 1.16.0.66) or lower
// Shadowing it, avoids allways defining OPENSSL_armcap=0
    hooks.emplace_back(mcpelauncher_hook_t{ "OPENSSL_cpuid_setup", (void*) + []() -> void {} });
#endif

    for (auto&& e : preinitHooks) {
        hooks.emplace_back(mcpelauncher_hook_t{ e.first.data(), e.second.value});
    }

// Minecraft 1.16.210+ removes the symbols previously used to patch it via vtables, so use hooks instead if supplied
    if (showMousePointerCallback && hideMousePointerCallback) {
        hooks.emplace_back(mcpelauncher_hook_t{ "_ZN11AppPlatform16showMousePointerEv", showMousePointerCallback });
        hooks.emplace_back(mcpelauncher_hook_t{ "_ZN11AppPlatform16hideMousePointerEv", hideMousePointerCallback });
    }
    if (fullscreenCallback) {
        hooks.emplace_back(mcpelauncher_hook_t{ "_ZN11AppPlatform17setFullscreenModeE14FullscreenMode", fullscreenCallback });
    }
    auto libc = linker::dlopen("libc.so", 0);
    // webrtc shortcut
    auto bgetifaddrs = linker::dlsym(libc, "getifaddrs");
    if(bgetifaddrs) {
        hooks.emplace_back(mcpelauncher_hook_t{ "_ZN3rtc10getifaddrsEPP7ifaddrs", bgetifaddrs });
    }
    auto bfreeifaddrs = linker::dlsym(libc, "freeifaddrs");
    if(bfreeifaddrs) {
        hooks.emplace_back(mcpelauncher_hook_t{ "_ZN3rtc11freeifaddrsEP7ifaddrs", bfreeifaddrs });
    }
    hooks.emplace_back(mcpelauncher_hook_t{ nullptr, nullptr });
    extinfo.flags = ANDROID_DLEXT_MCPELAUNCHER_HOOKS;
    extinfo.mcpelauncher_hooks = hooks.data();
    void* handle = linker::dlopen_ext("libminecraftpe.so", 0, &extinfo);
    linker::dlclose(libc);
    if (handle == nullptr) {
        Log::error("MinecraftUtils", "Failed to load Minecraft: %s", linker::dlerror());
    } else {
        for (auto&& h : hooks) {
            if(h.name) {
                printf("Found hook: %s @ %p\n", h.name, linker::dlsym(handle, h.name));
                if(auto&& res = preinitHooks.find(h.name); res != preinitHooks.end() && res->second.callback != nullptr) {
                    printf("with value: %p\n", h.value);
                    res->second.callback(res->second.user, h.value);
                }
            }
        }
        HookManager::instance.addLibrary(handle);
    }
    return handle;
}
const char *MinecraftUtils::getLibraryAbi() {
    return PathHelper::getAbiDir();
}

size_t MinecraftUtils::getLibraryBase(void *handle) {
    return linker::get_library_base(handle);
}

void MinecraftUtils::setupGLES2Symbols(void* (*resolver)(const char *)) {
    int i = 0;
    std::unordered_map<std::string, void*> syms;
    while (true) {
        const char* sym = glesv2_symbols[i];
        if (sym == nullptr)
            break;
        syms[sym] = resolver(sym);
        i++;
    }
    linker::load_library("libGLESv2.so", syms);
}
