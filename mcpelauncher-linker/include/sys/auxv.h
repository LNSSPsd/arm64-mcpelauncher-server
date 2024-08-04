#ifdef __BIONIC__
#undef __BIONIC__
#endif
#ifdef __linux__
#include_next <sys/auxv.h>
#endif
#if (defined(__aarch64__) || defined(__arm__)) && (!defined(AT_HWCAP) || !defined(AT_HWCAP2))
#ifndef AT_HWCAP
#define AT_HWCAP 0
#endif
#ifndef AT_HWCAP2
#define AT_HWCAP2 0
#endif
#define getauxval(a) 0
#pragma message "stubbed getauxval"
#endif