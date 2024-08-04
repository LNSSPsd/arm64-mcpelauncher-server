#pragma once

#include <libc_shim.h>
#include <sched.h>

namespace shim {
    int sched_setaffinity(pid_t pid, size_t set_size, const void* mask);

}
