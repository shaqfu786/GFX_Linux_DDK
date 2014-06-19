#ifndef __ex_global_h_
#define __ex_global_h_

/*******************************************************
 * Name:   ex_global.h
 *
 *******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__linux__)
	#include <dlfcn.h>
#else 
		#error "Platform headers required"
#endif

#ifdef __GNUC__
#define __internal __attribute__((visibility("hidden")))
#else
#define __internal
#endif

/* Setup of the EnumCheck array used by the following functions */
#define ATTRIB(X) \
    { \
        #X, X  \
    }


/* Setup of the EnumCheck array used by the following functions */
#define ATTRIBMORE(A, X)	\
    { \
        A, #X, X     \
    }


#ifdef __cplusplus
extern "C" {
#endif

void *exLoadLibrary(char *exLibraryName);

int exUnloadLibrary(void *exhLib);

typedef void (*GENERICPF)(void);

int exGetLibFuncAddr(void *exhLib, char *exFuncName, GENERICPF *ppfFuncAddr);


#ifdef __cplusplus
}
#endif

#if defined(SUPPORT_ANDROID_PLATFORM)
#define LOG_TAG "EGLInfo"
#include <cutils/log.h>
#ifndef ALOGI
#define ALOGI LOGI
#endif
#ifndef ALOGE
#define ALOGE LOGE
#endif
#define INFO  ALOGI
#define ERROR ALOGE
#else /* defined(SUPPORT_ANDROID_PLATFORM) */
#define INFO  printf
#define ERROR printf
#endif /* defined(SUPPORT_ANDROID_PLATFORM) */

enum
{
	XINFO_ES1	= (1 << 0), /* EGL_OPENGL_ES_BIT */
	XINFO_VG	= (1 << 1), /* EGL_OPENVG_BIT */
	XINFO_ES2	= (1 << 2), /* EGL_OPENVG_ES2_BIT */
	XINFO_GL	= (1 << 3), /* EGL_OPENGL_BIT */
	XINFO_EGL	= (1 << 4),
	XINFO_GETCONFIGS 	= (1 << 5), /* Use eglChooseconfig */
	XINFO_CHOOSECONFIG 	= (1 << 6), /* Use eglGetConfigs */
	XINFO_ALL			= XINFO_ES1 | XINFO_VG | XINFO_ES2 | XINFO_GL | XINFO_EGL | XINFO_GETCONFIGS | XINFO_CHOOSECONFIG
};

#endif /* __ex_global_h_ */
