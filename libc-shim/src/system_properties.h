#pragma once

#include <libc_shim.h>

typedef struct prop_info prop_info;

namespace shim {
    const prop_info *__system_property_find(const char *name);
    int __system_property_get(const char *name, char *value);

    void add_system_properties_shimmed_symbols(std::vector<shimmed_symbol> &list);

}
