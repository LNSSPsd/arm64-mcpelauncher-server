#include <mcpelauncher/patch_utils.h>

#include <log.h>
#include <cstring>
#include <climits>
#include <stdexcept>
#include <mcpelauncher/linker.h>

const char* PatchUtils::TAG = "Patch";

void *PatchUtils::patternSearch(void *handle, const char *pattern) {
    std::vector<unsigned char> patternRaw;
    std::vector<unsigned char> patternMask;
    while (pattern[0] && pattern[1]) {
        if (*pattern == ' ') {
            ++pattern;
            continue;
        }
        if (pattern[0] == '?' && pattern[1] == '?') {
            patternRaw.push_back(0);
            patternMask.push_back(0);
        } else {
            patternRaw.push_back((char) std::strtoul(pattern, nullptr, 16));
            patternMask.push_back(0xFF);
        }
        pattern += 2;
    }

    size_t base, size;
    linker::get_library_code_region(handle, base, size);
    for (size_t i = size - patternRaw.size(); i > 0; --i) {
        for (size_t j = 0; j < patternRaw.size(); j++) {
            if (patternRaw[j] != (((unsigned char *) base)[i + j] & patternMask[j]))
                goto skip;
        }
        return &((char*) base)[i];
        skip: ;
    }
    return nullptr;
}

void PatchUtils::patchCallInstruction(void* patchOff, void* func, bool jump) {
    unsigned char* data = (unsigned char*) patchOff;
#ifdef __arm__
    if (!jump)
      throw std::runtime_error("Non-jump patches not supported in ARM mode");
    bool thumb = ((size_t) patchOff) & 1;
    if (thumb)
      data--;
    Log::trace(TAG, "Patching - original: %i %i %i %i %i", data[0], data[1], data[2], data[3], data[4]);
    if (thumb) {
        if (((size_t) data) % 4 != 0) {
            unsigned char patch[4] = {0xdf, 0xf8, 0x02, 0xf0};
            memcpy(data, patch, 4);
        } else {
            unsigned char patch[4] = {0xdf, 0xf8, 0x00, 0xf0};
            memcpy(data, patch, 4);
        }
    } else {
        unsigned char patch[4] = {0x04, 0xf0, 0x1f, 0xe5};
        memcpy(data, patch, 4);
    }
    memcpy(&data[4], &func, sizeof(int));
    Log::trace(TAG, "Patching - result: %i %i %i %i %i", data[0], data[1], data[2], data[3], data[4]);
#else
    Log::trace(TAG, "Patching - original: %i %i %i %i %i", data[0], data[1], data[2], data[3], data[4]);
    data[0] = (unsigned char) (jump ? 0xe9 : 0xe8);
    intptr_t ptr = (((intptr_t) func) - (intptr_t) patchOff - 5);
    if (ptr > INT_MAX || ptr < INT_MIN)
        throw std::runtime_error("patchCallInstruction: out of range");
    int iptr = ptr;
    memcpy(&data[1], &iptr, sizeof(int));
    Log::trace(TAG, "Patching - result: %i %i %i %i %i", data[0], data[1], data[2], data[3], data[4]);
#endif
}

void PatchUtils::VtableReplaceHelper::replace(const char* name, void* replacement) {
    replace(linker::dlsym(lib, name), replacement);
}

void PatchUtils::VtableReplaceHelper::replace(void* sym, void* replacement) {
    for (int i = 0; ; i++) {
        if (referenceVtable[i] == nullptr)
            break;
        if (referenceVtable[i] == sym) {
            vtable[i] = replacement;
            return;
        }
    }
}

size_t PatchUtils::getVtableSize(void** vtable) {
    for (size_t size = 2; ; size++) {
        if (vtable[size] == nullptr)
            return size;
    }
}
