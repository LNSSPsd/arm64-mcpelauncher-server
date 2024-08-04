#include "system_properties.h"

#include <unistd.h>
#include <stdexcept>

void shim::add_system_properties_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    list.push_back({"__system_property_find", __system_property_find});
    list.push_back({"__system_property_get", __system_property_get});
}

const prop_info* shim::__system_property_find(const char *name) {
    return 0;
}

int shim::__system_property_get(const char *name, char *value) {
    value[0] = 0;
    return 0;
}
