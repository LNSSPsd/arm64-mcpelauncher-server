#include <mcpelauncher/crash_handler.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <csignal>
#include <cxxabi.h>
#include <execinfo.h>
#include <mcpelauncher/linker.h>
#include <thread>


bool CrashHandler::hasCrashed = false;

void CrashHandler::handleSignal(int signal, void *aptr) {
    printf("Signal %i received\n", signal);

    struct sigaction act;
    act.sa_handler = nullptr;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGSEGV, &act, 0);
    sigaction(SIGABRT, &act, 0);
    sigaction(SIGFPE, &act, 0);
    sigaction(SIGBUS, &act, 0);
    sigaction(SIGILL, &act, 0);

    if (hasCrashed)
        return;
    hasCrashed = true;

    // Workaround against application freeze while dumping stacktrace
    // stop app from bouncing more than one sec. on crash macOS x86_64
    std::thread([signal](){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        printf("Backtrace or dumping stack hung up, aborting\n");
        printf("Why does some people think exit code %d is something meanful? It is just a unix signal number.\n", signal);
        fflush(stdout);
        _Exit(signal);
    }).detach();

#if defined(__x86_64__) && defined(__APPLE__)
    // called by more detailed signalhandler with correct stack address
    void** ptr = (void**)aptr;
#else
    void** ptr = &aptr;
#endif
    void *array[25];
    int count = backtrace(array, 25);
    char **symbols = backtrace_symbols(array, count);
    char *nameBuf = (char*) malloc(256);
    size_t nameBufLen = 256;
    printf("Backtrace elements: %i\n", count);
    for (int i = 0; i < count; i++) {
        if (symbols[i] == nullptr) {
            printf("#%i unk [%4p]\n", i, array[i]);
            continue;
        }
        if (symbols[i][0] == '[') { // unknown symbol
            Dl_info symInfo;
            if (linker::dladdr(array[i], &symInfo)) {
                int status = 0;
                nameBuf = abi::__cxa_demangle(symInfo.dli_sname, nameBuf, &nameBufLen, &status);
                printf("#%i LINKER %s+%p in %s+0x%4p [0x%4p]\n", i, nameBuf, (void *) ((size_t) array[i] - (size_t) symInfo.dli_saddr), symInfo.dli_fname, (void *) ((size_t) array[i] - (size_t) symInfo.dli_fbase), array[i]);
                continue;
            }
        }
        printf("#%i %s\n", i, symbols[i]);
    }
    printf("Dumping stack...\n");
    for (int i = 0; i < 1000; i++) {
        void* pptr = *ptr;
        Dl_info symInfo;
        if (pptr && linker::dladdr(pptr, &symInfo)) {
            int status = 0;
            nameBuf = abi::__cxa_demangle(symInfo.dli_sname, nameBuf, &nameBufLen, &status);
            printf("#%i LINKER %s+%p in %s+%4p [%4p]\n", i, nameBuf, (void *) ((size_t) pptr - (size_t) symInfo.dli_saddr), symInfo.dli_fname, (void *) ((size_t) pptr - (size_t) symInfo.dli_fbase), pptr);
        }
        ptr++;
    }
    printf("program failed with unix signal number: %d\n", signal);
    fflush(stdout);
    _Exit(signal);
}

#if defined(__x86_64__) && defined(__APPLE__)
void CrashHandler::handle_fs_fault(int sig, void *si, void *ucp) {
    ucontext_t *uc = (ucontext_t*)ucp;
    unsigned char *p = (unsigned char *)uc->uc_mcontext->__ss.__rip;
    if (p && *p == 0x64) {
        *p = 0x65;
    } else if (p && *p == 0x65) {
    } else {
        handleSignal(sig, (void**)uc->uc_stack.ss_sp);
    }
}
#endif

void CrashHandler::registerCrashHandler() {
    struct sigaction act;
    sigemptyset(&act.sa_mask);
#if defined(__x86_64__) && defined(__APPLE__)
    act.sa_sigaction = (void (*)(int, __siginfo *, void *)) handle_fs_fault;
    act.sa_flags = SA_SIGINFO;
#else
    act.sa_handler = (void (*)(int)) handleSignal;
    act.sa_flags = 0;
#endif
    sigaction(SIGSEGV, &act, 0);
    sigaction(SIGABRT, &act, 0);
    sigaction(SIGFPE, &act, 0);
    sigaction(SIGBUS, &act, 0);
    sigaction(SIGILL, &act, 0);
    
    // Ignore SIGTRAP of 1.16.100.51++
    // ToDo find all debugbreaks
    {
        struct sigaction act;
        sigemptyset(&act.sa_mask);
        act.sa_handler = SIG_IGN;
        act.sa_flags = 0;
        sigaction(SIGTRAP, &act, 0);
#if defined(__APPLE__)
        // Try to ignore this for macOS 10.12 and older and Minecraft 1.16.230+
        // macOS 12.3 bug reports, crash with sigsys
        sigaction(SIGSYS, &act, 0);
#endif
    }
}
