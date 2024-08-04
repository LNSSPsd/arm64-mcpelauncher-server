#include <mcpelauncher/minecraft_version.h>
#include <sstream>

int MinecraftVersion::major;
int MinecraftVersion::minor;
int MinecraftVersion::patch;
int MinecraftVersion::revision;

void MinecraftVersion::init() {
    // TODO:
    major = minor = patch = revision = 0;
}

std::string MinecraftVersion::getString() {
    std::stringstream ss;
    ss << major << '.' << minor << '.' << patch << '.' << revision;
    return ss.str();
}