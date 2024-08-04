#pragma once
#ifndef __APPLE__
#include "../../bionic/libc/include/sys/cdefs.h"
#endif
#include_next <sys/cdefs.h>
#ifdef __BIONIC__
#undef __BIONIC__
#endif