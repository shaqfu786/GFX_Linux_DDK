/*!****************************************************************************
@File           eglhelper.h

@Title          SurfaceFlinger EGL helper code

@Author         Imagination Technologies

@Date           2009/08/13

@Copyright      Copyright (C) Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       linux

******************************************************************************/

#ifndef EGLHELPER_H
#define EGLHELPER_H

#include <stdlib.h>
#include <stdio.h>

#if defined(PVR_ANDROID_HAS_ANDROID_NATIVE_BUFFER_H)
#include <ui/android_native_buffer.h>
#else
#include <hardware/gralloc.h>
#endif

#include <EGL/egl.h>

#if defined(PVR_ANDROID_NEEDS_ANATIVEWINDOW_TYPEDEF)
typedef struct android_native_window_t ANativeWindow;
#endif

/* This will need to be removed once HAL_PIXEL_FORMAT_BGRX_8888 is
   officially defined. */
#define HAL_PIXEL_FORMAT_BGRX_8888 0x101

#define ATTRIB_LUT_SIZE	6

/* This is a bit of a hack, but we know this header will only be used in
 * native code that will be wrapped by Java, so prototype the JNI API.
 */
int EglMain(EGLNativeDisplayType eglDisplay, EGLNativeWindowType eglWindow);
void ResizeWindow(unsigned int width, unsigned int height);
void RequestQuit(void);

enum
{
	LUT_EQUALS_PARAM,
	LUT_MASK_PARAM,
};

static
EGLConfig findMatchingWindowConfig(EGLDisplay dpy, EGLint type,
								   ANativeWindow *psNativeWindow)
{
	EGLConfig *pConfigs, config = (EGLConfig)0;
	EGLint i, j, iNumConfigs, iNativeVisualID;
	int format;

	EGLint aAttribLUT[ATTRIB_LUT_SIZE][3] =
	{
		{ EGL_RED_SIZE,			LUT_EQUALS_PARAM,	0 },
		{ EGL_GREEN_SIZE,		LUT_EQUALS_PARAM,	0 },
		{ EGL_BLUE_SIZE,		LUT_EQUALS_PARAM,	0 },
		{ EGL_ALPHA_SIZE,		LUT_EQUALS_PARAM,	0 },
		{ EGL_SURFACE_TYPE,		LUT_MASK_PARAM,		EGL_WINDOW_BIT },
		{ EGL_RENDERABLE_TYPE,	LUT_MASK_PARAM,		type },
	};

	EGLint aiAttribList[] = {
		EGL_RENDERABLE_TYPE, type,
		EGL_CONFIG_CAVEAT, EGL_NONE,
		EGL_NONE
	};

	if(!psNativeWindow)
	{
		printf("%s: Window was unexpectedly NULL\n", __func__);
		goto err_out;
	}

	if(psNativeWindow->query(psNativeWindow, NATIVE_WINDOW_FORMAT, &format))
	{
		printf("%s: Failed to ->query() window format\n", __func__);
		goto err_out;
	}

	switch(format)
	{
		case HAL_PIXEL_FORMAT_RGBA_8888:
		case HAL_PIXEL_FORMAT_BGRA_8888:
			aAttribLUT[0][2] = 8;
			aAttribLUT[1][2] = 8;
			aAttribLUT[2][2] = 8;
			aAttribLUT[3][2] = 8;
			break;

		case HAL_PIXEL_FORMAT_RGBX_8888:
		case HAL_PIXEL_FORMAT_BGRX_8888:
			aAttribLUT[0][2] = 8;
			aAttribLUT[1][2] = 8;
			aAttribLUT[2][2] = 8;
			aAttribLUT[3][2] = 0;
			break;

		case HAL_PIXEL_FORMAT_RGB_565:
			aAttribLUT[0][2] = 5;
			aAttribLUT[1][2] = 6;
			aAttribLUT[2][2] = 5;
			aAttribLUT[3][2] = 0;
			break;

		default:
			printf("%s: Unsupported window pixel format (%d)\n", __func__, format);
			goto err_out;
	}

	if(!eglChooseConfig(dpy, aiAttribList, NULL, 0, &iNumConfigs))
	{
		printf("%s: eglChooseConfig failed (%d)\n", __func__, eglGetError());
		goto err_out;
	}

	pConfigs = calloc(iNumConfigs, sizeof(EGLConfig));

	if(!eglChooseConfig(dpy, aiAttribList, pConfigs, iNumConfigs, &iNumConfigs))
	{
		printf("%s: eglChooseConfig failed (%d)\n", __func__, eglGetError());
		goto err_out;
	}

	for(i = 0; i < iNumConfigs; i++)
	{
		if((eglGetConfigAttrib(dpy, pConfigs[i], EGL_NATIVE_VISUAL_ID,
							   &iNativeVisualID) == EGL_TRUE) &&
		   (iNativeVisualID == format))
		{
			config = pConfigs[i];
			goto found;
		}
	}

	for(i = 0; i < iNumConfigs; i++)
	{
		for(j = 0; j < ATTRIB_LUT_SIZE; j++)
		{
			EGLint value;

			if(!eglGetConfigAttrib(dpy, pConfigs[i], aAttribLUT[j][0], &value))
			{
				printf("%s: eglGetConfigAttrib failed (%d)\n",
					 __func__, eglGetError());
				goto err_out;
			}

			if(aAttribLUT[j][1] == LUT_MASK_PARAM)
			{
				if(!(value & aAttribLUT[j][2]))
					break;
			}
			else
			{
				if(value != aAttribLUT[j][2])
					break;
			}
		}

		if(j != ATTRIB_LUT_SIZE)
			continue;

		/* Found it! */
		config = pConfigs[i];
		goto found;
	}

	printf("%s: Unable to find a config either matching native visual %d, "
		   "or matching EGL config attributes\n",
		   __func__, format);

found:
	free(pConfigs);
err_out:
	return config;
}

#endif /* EGLHELPER_H */
