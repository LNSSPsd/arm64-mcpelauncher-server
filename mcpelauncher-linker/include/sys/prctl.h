#ifdef __linux__
#include_next <sys/prctl.h>
#endif
#define PR_SET_VMA 0
#define PR_SET_VMA_ANON_NAME 0