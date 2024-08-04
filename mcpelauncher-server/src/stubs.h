#pragma once
#include <string>
#include <unordered_map>

class Stubs {
public:
    static void initHybrisHooks(std::unordered_map<std::string, void *> &syms);
};
