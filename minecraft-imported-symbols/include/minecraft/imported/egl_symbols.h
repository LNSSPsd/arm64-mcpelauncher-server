#pragma once

static const char* egl_symbols[] = {
        "eglGetCurrentDisplay",
        "eglChooseConfig",
        "eglGetError",
        "eglCreateWindowSurface",
        "eglGetConfigAttrib",
        "eglCreateContext",
        "eglDestroySurface",
        "eglSwapBuffers",
        "eglMakeCurrent",
        "eglDestroyContext",
        "eglTerminate",
        "eglGetDisplay",
        "eglInitialize",
        "eglQuerySurface",
        "eglSwapInterval",
        "eglQueryString",
        nullptr
};