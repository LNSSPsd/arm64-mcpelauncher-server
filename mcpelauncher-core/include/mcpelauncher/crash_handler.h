#pragma once

class CrashHandler {

private:
    static bool hasCrashed;

    static void handleSignal(int signal, void* aptr);
#if defined(__x86_64__) && defined(__APPLE__)
    static void handle_fs_fault(int sig, void *si, void *ucp);
#endif
public:
    static void registerCrashHandler();

};