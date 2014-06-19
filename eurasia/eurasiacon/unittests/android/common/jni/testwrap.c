/*!****************************************************************************
@File           testwrap.c

@Title          Test wrapper for Android (JNI component)

@Author         Imagination Technologies

@Date           2010/01/25

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

@Description    Test wrapper for Android (JNI component)

Modifications :-
$Log: testwrap.c $
******************************************************************************/

#include <errno.h>
#include <dlfcn.h>

#define LOG_TAG "IMGTestWrap"
#include <cutils/log.h>
#ifndef ALOGE
#define ALOGE LOGE
#endif

#include <EGL/egl.h>

#include "testwrap.h"

#define __unref __attribute__((unused))

#if !defined(PVR_ANDROID_SURFACE_FIELD_NAME)
#define PVR_ANDROID_SURFACE_FIELD_NAME "mNativeSurface"
#endif

static jfieldID	gSurfaceFieldID;
static void		*gpvWrapLib;

typedef int  (*EglMain_t)(EGLNativeDisplayType display,
						  EGLNativeWindowType window);
typedef void (*ResizeWindow_t)(unsigned int width, unsigned int height);
typedef void (*RequestQuit_t)(void);

static EglMain_t		gpfnEglMain;
static ResizeWindow_t	gpfnResizeWindow;
static RequestQuit_t	gpfnRequestQuit;

JNIEXPORT jint JNICALL Java_com_imgtec_powervr_ddk_TestProgram_init(
	JNIEnv *env, jobject __unref obj, jstring wrapLib)
{
	const char *str;
    jclass c;

	c = (*env)->FindClass(env, "android/view/Surface");
	if(!c)
		return -EFAULT;

	gSurfaceFieldID =
		(*env)->GetFieldID(env, c, PVR_ANDROID_SURFACE_FIELD_NAME, "I");
	if(!gSurfaceFieldID)
		return -EFAULT;

	str = (*env)->GetStringUTFChars(env, wrapLib, NULL);
	if(!str)
		return -EFAULT;

	gpvWrapLib = dlopen(str, RTLD_NOW);
	if(!gpvWrapLib)
	{
		ALOGE("Failed to load library '%s': %s", str, dlerror());
		return -EFAULT;
	}

	gpfnEglMain = (EglMain_t)dlsym(gpvWrapLib, "EglMain");
	if(!gpfnEglMain)
	{
		ALOGE("Failed to load symbol 'EglMain': %s", dlerror());
		return -EFAULT;
	}

	gpfnResizeWindow = (ResizeWindow_t)dlsym(gpvWrapLib, "ResizeWindow");
	if(!gpfnResizeWindow)
	{
		ALOGE("Failed to load symbol 'ResizeWindow': %s", dlerror());
		return -EFAULT;
	}

	gpfnRequestQuit = (RequestQuit_t)dlsym(gpvWrapLib, "RequestQuit");
	if(!gpfnRequestQuit)
	{
		ALOGE("Failed to load symbol 'RequestQuit': %s", dlerror());
		return -EFAULT;
	}

	(*env)->ReleaseStringUTFChars(env, wrapLib, str);
	return 0;
}

JNIEXPORT jint JNICALL Java_com_imgtec_powervr_ddk_TestProgram_eglMain(
	JNIEnv *env, jobject __unref obj, jobject eglNativeWindow)
{
	EGLNativeWindowType window = (EGLNativeWindowType)(
		(*env)->GetIntField(env, eglNativeWindow, gSurfaceFieldID) + 8);

	return gpfnEglMain(EGL_DEFAULT_DISPLAY, window);
}

JNIEXPORT void JNICALL Java_com_imgtec_powervr_ddk_TestProgram_resizeWindow(
	JNIEnv __unref *env, jobject __unref obj, jint width, jint height)
{
	gpfnResizeWindow((unsigned int)width, (unsigned int)height);
}

JNIEXPORT void JNICALL Java_com_imgtec_powervr_ddk_TestProgram_requestQuit(
	JNIEnv __unref *env, jobject __unref obj)
{
	gpfnRequestQuit();
}
