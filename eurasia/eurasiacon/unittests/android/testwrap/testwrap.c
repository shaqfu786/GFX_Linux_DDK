/*!****************************************************************************
@File           testwrap.c

@Title          Test wrapper for Android (standalone binary component)

@Author         Imagination Technologies

@Date           2010/02/10

@Copyright      Copyright 2010 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       android

@Description    Test wrapper for Android (standalone binary component)

Modifications :-
$Log: testwrap.c $
******************************************************************************/

#include <limits.h>
#include <stdio.h>
#include <dlfcn.h>

#if defined(PVR_ANDROID_HAS_ANDROID_NATIVE_BUFFER_H)
#include <ui/android_native_buffer.h>
#else
#include <hardware/gralloc.h>
#endif

#include <EGL/egl.h>

/* FIXME: Android's ui/FrameBufferNativeWindow.h doesn't compile in C mode */
extern EGLNativeWindowType android_createDisplaySurface(void);

#define TESTBASE "/data/data/com.imgtec.powervr.ddk."

typedef int (*EglMain_t)(EGLNativeDisplayType display,
                         EGLNativeWindowType window);

int main(int argc, char *argv[])
{
	EGLNativeWindowType eglWindow;
	char acPath[PATH_MAX];
	EglMain_t pfnEglMain;
	void *hTestLib;
	int err = -1;

	if(argc <= 1)
	{
		printf("usage: %s [test name]\n", argv[0]);
		goto err_out;
	}

	snprintf(acPath, PATH_MAX, TESTBASE "%s/lib/lib%s.so", argv[1], argv[1]);
	acPath[PATH_MAX - 1] = 0;

	hTestLib = dlopen(acPath, RTLD_NOW);
	if(!hTestLib)
	{
		printf("Failed to load '%s': %s\n", acPath, dlerror());
		goto err_out;
	}

	pfnEglMain = (EglMain_t)dlsym(hTestLib, "EglMain");
	if(!pfnEglMain)
	{
		printf("No eglMain symbol in '%s': %s\n", acPath, dlerror());
		goto err_close;
	}

	eglWindow = android_createDisplaySurface();
	eglWindow->common.incRef(&eglWindow->common);
	err = pfnEglMain(EGL_DEFAULT_DISPLAY, eglWindow);
	eglWindow->common.decRef(&eglWindow->common);

err_close:
	dlclose(hTestLib);
err_out:
	return err;
}
