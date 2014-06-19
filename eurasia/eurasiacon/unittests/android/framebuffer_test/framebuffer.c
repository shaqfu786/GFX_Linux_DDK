/*!****************************************************************************
@File           framebuffer.c

@Title          Graphics HAL (framebuffer test)

@Author         Imagination Technologies

@Date           2009/04/23

@Copyright      Copyright 2009 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       linux

@Description    Graphics HAL (framebuffer test)

******************************************************************************/
/******************************************************************************
Modifications :-
$Log: framebuffer.c $
******************************************************************************/

#include <stdlib.h>
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "img_defs.h"

#if defined(PVR_ANDROID_HAS_ANDROID_NATIVE_BUFFER_H)
#include <ui/android_native_buffer.h>
#else
#include <hardware/gralloc.h>
#endif

#include <EGL/egl.h>

#if defined(PVR_ANDROID_NEEDS_ANATIVEWINDOWBUFFER_TYPEDEF)
typedef struct android_native_buffer_t ANativeWindowBuffer;
#else
typedef struct ANativeWindowBuffer ANativeWindowBuffer;
#endif

/* FIXME: Android's ui/FrameBufferNativeWindow.h doesn't compile in C mode */
extern EGLNativeWindowType android_createDisplaySurface(void);

static void FillGrey8888(ANativeWindowBuffer *psNativeBuffer,
						 IMG_UINT32 *pui32Bits, char g)
{
	IMG_UINT32 p = (0xFF << 24) | (g << 16) | (g << 8) | (g << 0);
	int i, j;

	for (j = 0; j < psNativeBuffer->height; j++)
	{
		for(i = 0; i < psNativeBuffer->width; i++)
			pui32Bits[j * psNativeBuffer->stride + i] = p;
		usleep(2);
	}
}

static void FillGrey565(ANativeWindowBuffer *psNativeBuffer,
						IMG_UINT16 *pui16Bits, char g)
{
	IMG_UINT16 p = ((g & 0xF8) << 8) | ((g & 0xFC) << 3) | ((g & 0xF8) >> 3);
	int i, j;

	for(j = 0; j < psNativeBuffer->height; j++)
	{
		for(i = 0; i < psNativeBuffer->width; i++)
			pui16Bits[j * psNativeBuffer->stride + i] = p;
		usleep(2);
	}
}

static void FillGrey(ANativeWindowBuffer *psNativeBuffer,
					 void *pvBits, char g)
{
	switch(psNativeBuffer->format)
	{
		case HAL_PIXEL_FORMAT_RGBA_8888:
		case HAL_PIXEL_FORMAT_BGRA_8888:
			FillGrey8888(psNativeBuffer, pvBits, g);
			break;
		case HAL_PIXEL_FORMAT_RGB_565:
			FillGrey565(psNativeBuffer, pvBits, g);
			break;
		default:
			printf("Unimplemented pixel format!\n");
			break;
	}
}

int main(void)
{
	EGLNativeWindowType psNativeWindow;
	gralloc_module_t *module;
	IMG_UINT32 i;

	if(hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&module))
	{
		perror("hw_get_module");
		return 0;
	}

	psNativeWindow = android_createDisplaySurface();
	psNativeWindow->common.incRef(&psNativeWindow->common);

	for(i = 0; i < 12; i++)
	{
		ANativeWindowBuffer *psNativeBuffer;
		void *pvBits;

		/* Dequeue a buffer */
		if(psNativeWindow->dequeueBuffer(psNativeWindow, &psNativeBuffer))
		{
			printf("window->dequeueBuffer() failed\n");
			return 0;
		}

		/* Lock it */
		if(psNativeWindow->lockBuffer(psNativeWindow, psNativeBuffer))
		{
			printf("window->lockBuffer() failed\n");
			return 0;
		}

		/* Lock it (again?) */
		if(module->lock((gralloc_module_t const*)module,
						psNativeBuffer->handle,
						GRALLOC_USAGE_SW_WRITE_OFTEN, 0, 0,
						psNativeBuffer->width,
						psNativeBuffer->height, &pvBits))
		{
			printf("gralloc->lock() failed\n");
			return 0;
		}

		/* Draw the buffer slowly */
		FillGrey(psNativeBuffer, pvBits, 255 * ((float)rand() / (float)RAND_MAX));
		printf("Wrote to pvLinAddr %p\n", pvBits);

		/* Unlock it */
		if(module->unlock((gralloc_module_t const*)module,
						  psNativeBuffer->handle))
		{
			printf("gralloc->unlock() failed\n");
			return 0;
		}

		/* Queue the buffer (flips FB) */
		if(psNativeWindow->queueBuffer(psNativeWindow, psNativeBuffer))
		{
			printf("window->queueBuffer() failed\n");
			return 0;
		}

		/* Wait a second */
		sleep(1);
	}

	psNativeWindow->common.decRef(&psNativeWindow->common);

	return 0;
}
