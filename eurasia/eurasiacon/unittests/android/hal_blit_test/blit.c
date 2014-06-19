/*!****************************************************************************
@File           blit.c

@Title          Graphics HAL (blit test)

@Author         Imagination Technologies

@Date           2011/08/08

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

#include <stdlib.h>
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define EGL_EGLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>

#define GL_GLEXT_PROTOTYPES

#include <GLES/gl.h>
#include <GLES/glext.h>

#include "hal_public.h"
#include "eglhelper.h"

#if defined(PVR_ANDROID_NEEDS_ANATIVEWINDOWBUFFER_TYPEDEF)
typedef struct android_native_buffer_t ANativeWindowBuffer;
#else
typedef struct ANativeWindowBuffer ANativeWindowBuffer;
#endif

/* FIXME: Android's ui/FrameBufferNativeWindow.h doesn't compile in C mode */
extern EGLNativeWindowType android_createDisplaySurface(void);

/* At the moment this code doesn't check that src and dest
 * parameters are the same / compatible in the same way Blit2
 * does. This is so if (in the future) we are asked to relax
 * Blit2's requirements, this code will continue to work.
 */

#define SRC_WIDTH	320
#define SRC_HEIGHT	240
#define SRC_STRIDE	ALIGN(SRC_WIDTH,HW_ALIGN)

#if defined(SUPPORT_ANDROID_C110_NV12)
# define HAL_PIXEL_FORMAT_C110_NV12			0x100
/* From buffer_c110.c */
# define DEST_STRIDE	ALIGN(SRC_WIDTH,16)
# define DEST_FORMAT	HAL_PIXEL_FORMAT_C110_NV12
# define DEST_WIDTH		ALIGN(SRC_WIDTH,16)
# define DEST_HEIGHT	ALIGN(SRC_HEIGHT,16)
#else /* defined(SUPPORT_ANDROID_C110_NV12) */
#if defined(SUPPORT_ANDROID_OMAP_NV12)
# define HAL_PIXEL_FORMAT_TI_NV12			0x100
/* From buffer_omap.c */
# define DEST_STRIDE	4096
# define DEST_FORMAT	HAL_PIXEL_FORMAT_TI_NV12
# define DEST_WIDTH	SRC_WIDTH
# define DEST_HEIGHT	SRC_HEIGHT
#else /* defined(SUPPORT_ANDROID_OMAP_NV12) */
#error Dont know how to test blits on this platform
#endif /* defined(SUPPORT_ANDROID_OMAP_NV12) */
#endif /* defined(SUPPORT_ANDROID_C110_NV12) */

#if SRC_WIDTH != DEST_WIDTH
#warning Blit2() probably wont like this height
#endif

#if SRC_HEIGHT != DEST_HEIGHT
#warning Blit2() probably wont like this height
#endif

static void incRefNop(struct android_native_base_t __unused *base) {}
static void decRefNop(struct android_native_base_t __unused *base) {}

int main(void)
{
	EGLint major, minor, eglCfgCount, eglCfgVisualId, width, height;
	const IMG_gralloc_module_public_t *module;
	buffer_handle_t srcBuffer, destBuffer;
	EGLImageKHR eglSrcImage, eglDestImage;
	EGLConfig eglConfig, eglFBConfig;
	GLuint fboName, textureNames[2];
	EGLNativeWindowType eglWindow;
	EGLSurface eglWindowSurface;
	alloc_device_t *device;
	EGLContext eglContext;
	EGLDisplay eglDisplay;
	int err = 1, stride;
	GLenum glError;

	ANativeWindowBuffer sSrcBuffer =
	{
		.common.magic	= ANDROID_NATIVE_BUFFER_MAGIC,
		.common.version	= sizeof(ANativeWindowBuffer),
		.common.incRef	= incRefNop,
		.common.decRef	= decRefNop,
		.width			= SRC_WIDTH,
		.height			= SRC_HEIGHT,
		.stride			= SRC_STRIDE,
		.usage			= GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE,
	};

	ANativeWindowBuffer sDestBuffer =
	{
		.common.magic	= ANDROID_NATIVE_BUFFER_MAGIC,
		.common.version	= sizeof(ANativeWindowBuffer),
		.common.incRef	= incRefNop,
		.common.decRef	= decRefNop,
		.width			= DEST_WIDTH,
		.height			= DEST_HEIGHT,
		.stride			= DEST_STRIDE,
		.format			= DEST_FORMAT,
		.usage			= GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE,
	};

	EGLint eglCfgAttribs[] =
	{
		EGL_RED_SIZE,			5,
		EGL_GREEN_SIZE,			6,
		EGL_BLUE_SIZE,			5,
		EGL_ALPHA_SIZE,			0,
		EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES_BIT,
#ifdef EGL_ANDROID_recordable
		EGL_RECORDABLE_ANDROID,	EGL_TRUE,
#endif
		EGL_NONE,
	};

	const float srcVertexArray[2 * 4] = {
		 0.0f,	 1.0f,
		 0.0f,	 0.0f,
		 1.0f, 	 1.0f,
		 1.0f,	 0.0f,
	};

	const float texCoordArray[2 * 4] = {
		 0.0f,	 0.0f,
		 0.0f,	 1.0f,
		 1.0f,	 0.0f,
		 1.0f,	 1.0f,
	};

	const float destVertexArray[2 * 4] = {
		-1.0f,	 0.0f,
		-1.0f,	-1.0f,
		 0.0f,	 0.0f,
		 0.0f,	-1.0f,
	};

	eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if(eglDisplay == EGL_NO_DISPLAY)
	{
		printf("eglGetDisplay failed\n");
		goto err_out;
	}

	if(!eglInitialize(eglDisplay, &major, &minor))
	{
		printf("eglInitialize failed (err=0x%x)\n", eglGetError());
		goto err_out;
	}

	if(!eglChooseConfig(eglDisplay, eglCfgAttribs,
						&eglConfig, 1, &eglCfgCount))
	{
		printf("eglChooseConfig failed (err=0x%x)\n", eglGetError());
		goto err_terminate;
	}

	if(!eglCfgCount)
	{
		printf("eglChooseConfig found no suitable configs\n");
		goto err_terminate;
	}

	if(!eglGetConfigAttrib(eglDisplay, eglConfig,
						   EGL_NATIVE_VISUAL_ID, &eglCfgVisualId))
	{
		printf("eglGetConfigAttrib failed (err=0x%x)\n", eglGetError());
		goto err_terminate;
	}

	sSrcBuffer.format = eglCfgVisualId;

	/* Handle FB rendering ***************************************************/

	eglWindow = android_createDisplaySurface();
	if(!eglWindow)
	{
		printf("android_createDisplaySurface returned NULL\n");
		goto err_terminate;
	}

	eglWindow->common.incRef(&eglWindow->common);

	eglFBConfig = findMatchingWindowConfig(eglDisplay, EGL_OPENGL_ES_BIT, eglWindow);
	/* FIXME: findMatchingWindowConfig returns no error code */

	eglContext = eglCreateContext(eglDisplay, eglFBConfig, EGL_NO_CONTEXT, NULL);
	if(eglContext == EGL_NO_CONTEXT)
	{
		printf("eglCreateContext failed (err=0x%x)\n", eglGetError());
		goto err_window_decref;
	}

	eglWindowSurface = eglCreateWindowSurface(eglDisplay, eglFBConfig, eglWindow, NULL);
	if(eglWindowSurface == EGL_NO_SURFACE)
	{
		printf("eglCreateWindowSurface failed (err=0x%x)\n", eglGetError());
		goto err_destroy_context;
	}

	if(!eglQuerySurface(eglDisplay, eglWindowSurface, EGL_WIDTH, &width))
	{
		printf("eglQuerySurface #1 failed (err=0x%x)\n", eglGetError());
		goto err_destroy_context;
	}

	if(!eglQuerySurface(eglDisplay, eglWindowSurface, EGL_HEIGHT, &height))
	{
		printf("eglQuerySurface #2 failed (err=0x%x)\n", eglGetError());
		goto err_destroy_context;
	}

	if(!eglMakeCurrent(eglDisplay, eglWindowSurface, eglWindowSurface, eglContext))
	{
		printf("eglMakeCurrent failed (err=0x%x)\n", eglGetError());
		goto err_destroy_surface;
	}

	/* Allocate some compatible buffers with gralloc *************************/

	err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID,
						(const hw_module_t **)&module);
	if(err)
	{
		printf("hw_get_module failed (err=%d)\n", err);
		goto err_make_non_current;
	}

	err = module->base.common.methods->open((const hw_module_t *)module,
											GRALLOC_HARDWARE_GPU0,
											(hw_device_t **)&device);
	if(err)
	{
		printf("module->common.methods->open() failed (err=%d)\n", err);
		goto err_make_non_current;
	}

	err = device->alloc(device, SRC_WIDTH, SRC_HEIGHT, eglCfgVisualId,
						GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE,
						&srcBuffer, &stride);
	if(err)
	{
		printf("device->alloc() failed (err=%d)\n", err);
		goto err_close;
	}

	err = device->alloc(device, DEST_WIDTH, DEST_HEIGHT, DEST_FORMAT,
						GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE,
						&destBuffer, &stride);
	if(err)
	{
		printf("device->alloc() failed (err=%d)\n", err);
		goto err_free_src;
	}

	err = module->base.registerBuffer(&module->base, srcBuffer);
	if(err)
	{
		printf("module->registerBuffer() failed (err=%d)\n", err);
		goto err_free_dest;
	}

	err = module->base.registerBuffer(&module->base, destBuffer);
	if(err)
	{
		printf("module->registerBuffer() failed (err=%d)\n", err);
		goto err_unregister_src;
	}

	sSrcBuffer.handle = srcBuffer;
	sDestBuffer.handle = destBuffer;

	/* Make some EGLImageKHRs out of them ************************************/

	eglSrcImage = eglCreateImageKHR(eglDisplay, EGL_NO_CONTEXT,
									EGL_NATIVE_BUFFER_ANDROID,
									(EGLClientBuffer)&sSrcBuffer, 0);
	if(eglSrcImage == EGL_NO_IMAGE_KHR)
	{
		printf("eglCreateImageKHR #1 failed (err=0x%x)\n", eglGetError());
		goto err_unregister_dest;
	}

	eglDestImage = eglCreateImageKHR(eglDisplay, EGL_NO_CONTEXT,
									 EGL_NATIVE_BUFFER_ANDROID,
									 (EGLClientBuffer)&sDestBuffer, 0);
	if(eglDestImage == EGL_NO_IMAGE_KHR)
	{
		printf("eglCreateImageKHR #2 failed (err=0x%x)\n", eglGetError());
		goto err_destroy_src_image;
	}

	/* Create funny textures *************************************************/

	glGenTextures(2, textureNames);
	glError = glGetError();
	if(glError != GL_NO_ERROR)
	{
		printf("glGenTextures generated error 0x%x\n", glError);
		goto err_destroy_dest_image;
	}

	glBindTexture(GL_TEXTURE_2D, textureNames[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, eglSrcImage);
	glError = glGetError();
	if(glError != GL_NO_ERROR)
	{
		printf("glEGLImageTargetTexture2DOES generated error 0x%x\n", glError);
		goto err_delete_textures;
	}

	glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureNames[1]);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, eglDestImage);
	glError = glGetError();
	if(glError != GL_NO_ERROR)
	{
		printf("glEGLImageTargetTexture2DOES generated error 0x%x\n", glError);
		goto err_delete_textures;
	}

	/* Create FBO ************************************************************/

	glGenFramebuffersOES(1, &fboName);
	glError = glGetError();
	if(glError != GL_NO_ERROR)
	{
		printf("glGenFrameBuffersOES generated error 0x%x\n", glError);
		goto err_delete_textures;
	}

	glBindFramebufferOES(GL_FRAMEBUFFER_OES, fboName);
	glError = glGetError();
	if(glError != GL_NO_ERROR)
	{
		printf("glBindFramebufferOES generated error 0x%x\n", glError);
		goto err_delete_framebuffer;
	}

	glBindTexture(GL_TEXTURE_2D, textureNames[0]);

	glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES,
							  GL_TEXTURE_2D, textureNames[0], 0);
	glError = glGetError();
	if(glError != GL_NO_ERROR)
	{
		printf("glFramebufferTexture2DOES generated error 0x%x\n", glError);
		goto err_delete_framebuffer;
	}

	/*************************************************************************/

	glError = glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES);
	if(glError != GL_FRAMEBUFFER_COMPLETE_OES)
	{
		printf("glCheckFramebufferStatus generated error 0x%x\n", glError);
		goto err_delete_framebuffer;
	}

	/* Draw some stuff */

	{
		const float vertexArray[2 * 4] = {
			-1.0f,	 1.0f,
			-1.0f,	-1.0f,
			 1.0f,	 1.0f,
			 1.0f,	-1.0f,
		};

		const float colorArray[4 * 4] = {
			 1.0f, 0.0f, 0.0f, 1.0f,
			 0.0f, 1.0f, 0.0f, 1.0f,
			 0.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, 0.0f, 1.0f, 1.0f,
		};

		char dummy[4];

		glViewport(0, 0, SRC_WIDTH, SRC_HEIGHT);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);

		glVertexPointer(2, GL_FLOAT, 0, vertexArray);
		glColorPointer(4, GL_FLOAT, 0, colorArray);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);

		glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, dummy);
	}

	/* RGB -> YUV blit */

	err = module->Blit2(module, srcBuffer, destBuffer,
						SRC_WIDTH, SRC_HEIGHT, 0, 0);
	if(err)
	{
		printf("module->Blit2() failed (err=%d)\n", err);
		goto err_delete_framebuffer;
	}

	/* Present both to screen (should appear identical) */

	glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);
	glViewport(0, 0, width, height);

	glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, 0, texCoordArray);

	glEnable(GL_TEXTURE_EXTERNAL_OES);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureNames[1]);
	glVertexPointer(2, GL_FLOAT, 0, destVertexArray);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisable(GL_TEXTURE_EXTERNAL_OES);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureNames[0]);
	glVertexPointer(2, GL_FLOAT, 0, srcVertexArray);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisable(GL_TEXTURE_2D);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	eglSwapBuffers(eglDisplay, eglWindowSurface);
	sleep(1);

err_delete_framebuffer:
	glDeleteFramebuffersOES(1, &fboName);
err_delete_textures:
	glDeleteTextures(2, textureNames);
err_destroy_dest_image:
	eglDestroyImageKHR(eglDisplay, eglDestImage);
err_destroy_src_image:
	eglDestroyImageKHR(eglDisplay, eglSrcImage);
err_unregister_dest:
	err = module->base.unregisterBuffer(&module->base, destBuffer);
	if(err)
		printf("module->unregisterBuffer() failed (err=%d)\n", err);
err_unregister_src:
	err = module->base.unregisterBuffer(&module->base, srcBuffer);
	if(err)
		printf("module->unregisterBuffer() failed (err=%d)\n", err);
err_free_dest:
	err = device->free(device, destBuffer);
	if(err)
		printf("device->free() failed (err=%d)\n", err);
err_free_src:
	err = device->free(device, srcBuffer);
	if(err)
		printf("device->free() failed (err=%d)\n", err);
err_close:
	err = device->common.close((hw_device_t *)device);
	if(err)
		printf("hal->close() failed (err=%d)\n", err);
err_make_non_current:
	eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
err_destroy_surface:
	eglDestroySurface(eglDisplay, eglWindowSurface);
err_destroy_context:
	eglDestroyContext(eglDisplay, eglContext);
err_window_decref:
	eglWindow->common.decRef(&eglWindow->common);
err_terminate:
	eglTerminate(eglDisplay);
err_out:
	return err;
}
