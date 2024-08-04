#include "stubs.h"
#include <semaphore.h>
#include <vector>
#include <stdint.h>

void Stubs::initHybrisHooks(std::unordered_map<std::string, void*>& syms) {
    syms["__libc_current_sigrtmin"]=(void*)+[](void)->int {
	return 34;
    };
    syms["__libc_current_sigrtmax"]=(void*)+[](void)->int {
	return 64;
    };
    syms["sem_trywait"]=(void*)+[](sem_t* a)->int {
	    return sem_trywait(a);
    };
    std::vector<std::string> otherStubs={
	    "AAssetManager_open","AAssetManager_openDir","AAsset_close","AAsset_isAllocated",
	    "AAsset_read","AAsset_seek64","AAsset_seek","AAsset_getLength64",
	    "AAsset_getLength","AAsset_getRemainingLength64","AAsset_getRemainingLength",
	    "AAsset_getBuffer","AAssetDir_close","AAssetDir_rewind",
	    "AAssetDir_getNextFileName",
	    "ALooper_prepare", "ALooper_addFd", "ALooper_pollAll", "AInputQueue_attachLooper",
	    "ANativeActivity_finish",
	    "getservbyname", "ANativeWindow_fromSurface", "environ",
	    "ANativeWindow_getWidth", "ANativeWindow_getHeight",
	    "getservbyport","getprotobyname", "getprotobyport",
    "__get_h_errno","ttyname","__umask_chk","eglQueryContext","eglReleaseThread", "eglCreatePbufferSurface","eglGetCurrentSurface","socketpair","memrchr","waitpid", "sem_wait",
    "ALooper_prepare", "ALooper_addFd", "ALooper_pollAll", "AInputQueue_attachLooper", "ANativeActivity_finish", "AKeyEvent_getRepeatCount", "AKeyEvent_getMetaState",
    "AMotionEvent_getAction","AMotionEvent_getPointerCount","AMotionEvent_getButtonState","AMotionEvent_getPointerId","AMotionEvent_getX",
"AMotionEvent_getY","AMotionEvent_getRawX","AMotionEvent_getRawY","AMotionEvent_getAxisValue","AInputQueue_getEvent",
"AInputQueue_finishEvent","AInputQueue_preDispatchEvent","AInputEvent_getSource","AInputEvent_getType",
"AInputEvent_getDeviceId","AKeyEvent_getAction","AKeyEvent_getKeyCode", "glActiveTexture","glAttachShader","glBindBuffer","glBindFramebuffer","glBindRenderbuffer","glBindTexture","glBlendColor","glBlendEquationSeparate","glBlendFuncSeparate","glBufferData","glBufferSubData","glCheckFramebufferStatus","glClear","glClearColor","glClearDepthf","glClearStencil","glColorMask","glCompileShader","glCompressedTexImage2D","glCompressedTexSubImage2D","glCreateProgram","glCreateShader","glCullFace","glDeleteBuffers","glDeleteFramebuffers","glDeleteProgram","glDeleteRenderbuffers","glDeleteShader","glDeleteTextures","glDepthFunc","glDepthMask","glDetachShader","glDisable","glDisableVertexAttribArray","glDrawArrays","glDrawElements","glEnable","glEnableVertexAttribArray","glFlush","glFramebufferRenderbuffer","glFramebufferTexture2D","glFrontFace","glGenBuffers","glGenFramebuffers","glGenRenderbuffers","glGenTextures","glGenerateMipmap","glGetActiveAttrib","glGetActiveUniform","glGetAttribLocation","glGetError","glGetFloatv","glGetIntegerv","glGetProgramInfoLog","glGetProgramiv","glGetShaderInfoLog","glGetShaderPrecisionFormat","glGetShaderiv","glGetString","glGetUniformLocation","glLinkProgram","glPixelStorei","glPolygonOffset","glReadPixels","glRenderbufferStorage","glScissor","glShaderSource","glStencilFuncSeparate","glStencilOpSeparate","glTexImage2D","glTexParameterf","glTexParameterfv","glTexParameteri","glTexSubImage2D","glUniform1i","glUniform1iv","glUniform4fv","glUniformMatrix3fv","glUniformMatrix4fv","glUseProgram","glVertexAttribPointer","glViewport",
    "eglChooseConfig", "eglGetConfigAttrib", "eglWaitClient", "eglCreateWindowSurface", "eglGetError", "eglMakeCurrent", "eglDestroySurface", "eglDestroyContext",
    "eglQueryString", "eglCreateContext", "eglTerminate", "eglGetDisplay", "eglInitialize", "eglSwapInterval", "eglGetProcAddress", "eglSwapBuffers"};
    for(auto &i:otherStubs) {
	    syms[i]=(void*)+[]()->void *{return nullptr;};
    }
}
