#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <elf.h>
#ifndef __BEGIN_DECLS
#define __BEGIN_DECLS extern "C" {
#endif
#ifndef __END_DECLS
#define __END_DECLS }
#endif
#ifndef __INTRODUCED_IN
#define __INTRODUCED_IN(...)
#endif
#include "../../../mcpelauncher-linker/include/link.h"

class HookManager {

private:
    struct HookedSymbol;
    class LibInfo;
public:
    struct HookInstance {
    private:
        friend class HookManager;
        HookedSymbol* symbol;
        HookInstance* parent = nullptr;
        HookInstance* child = nullptr;
        void* replacement;
        void** orig = nullptr;
    };

private:
    struct LibSymbolPair {
        LibInfo* lib;
        ElfW(Word) symbolIndex;

        bool operator==(LibSymbolPair const& o) const {
            return lib == o.lib && symbolIndex == o.symbolIndex;
        }
    };
    struct LibSymbolPairHash {
        std::size_t operator()(const HookManager::LibSymbolPair &k) const {
            return (((size_t) (void*) k.lib) << 11) | k.symbolIndex;
        }
    };
    struct HookedSymbol {
        LibInfo* lib;
        ElfW(Word) symbolIndex;

        void* original = nullptr;
        HookInstance* firstHook = nullptr;
        HookInstance* lastHook = nullptr;
    };

    class LibInfo {

    private:
        friend class HookManager;
        void* handle;
        void* base;

        const char* strtab = nullptr;
        ElfW(Sym)* symtab = nullptr;
        ElfW(Rel)* rel = nullptr;
        ElfW(Word) relsz = 0;
        ElfW(Rel)* pltrel = nullptr;
        ElfW(Word) pltrelsz = 0;
        void* relro = nullptr;
        ElfW(Word) relrosize = 0;

        std::unordered_map<ElfW(Word), std::shared_ptr<HookedSymbol>> hookedSymbols;

        std::vector<void*> dependencies;

        LibInfo(void* handle);


        void applyHooks(ElfW(Rel)* rel, ElfW(Word) relsz);

    public:
        const char* getSymbolName(ElfW(Word) symbolIndex);

        void setHook(ElfW(Word) symbolIndex, std::shared_ptr<HookedSymbol> hook);

        void setHook(const char* symbolName, std::shared_ptr<HookedSymbol> hook);

        void applyHooks();

    };

    std::unordered_map<void*, std::unique_ptr<LibInfo>> libs;
    std::unordered_map<void*, std::vector<LibInfo*>> dependents;
    std::unordered_map<LibSymbolPair, std::shared_ptr<HookedSymbol>, LibSymbolPairHash> hookedSymbols;

    HookedSymbol* getOrCreateHookSymbol(void* lib, ElfW(Word) symbolIndex);

    static ElfW(Word) getSymbolIndex(void* lib, const char* symbolName);

public:
    static HookManager instance;

    void addLibrary(void* handle);

    void removeLibrary(void* handle);

    HookInstance* createHook(void* lib, ElfW(Word) symbolIndex, void* replacement, void** orig);

    HookInstance* createHook(void* lib, const char* symbolName, void* replacement, void** orig);

    void deleteHook(HookInstance* hook);

    void applyHooks();

    static std::string translateConstructorName(const char *name);

};