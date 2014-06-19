/*!****************************************************************************
@File           framebuffer.h

@Title          Graphics HAL framebuffer component

@Author         Imagination Technologies

@Date           2009/03/26

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

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "hal.h"

#if !defined(PVR_ANDROID_HAS_SW_INCOMPATIBLE_FRAMEBUFFER) && \
    (defined(NO_HARDWARE) && defined(PDUMP))
#define PVR_ANDROID_HAS_SW_INCOMPATIBLE_FRAMEBUFFER
#endif

/* Use -2, because -1 is reserved for open(2)'s error code */
#define IMG_FRAMEBUFFER_FD -2

typedef struct
{
	IMG_framebuffer_device_public_t base;
	IMG_UINT32 ui32NumBuffers;
}
IMG_framebuffer_device_t;

extern IMG_BOOL gbForeignGLES;

int framebuffer_device_alloc(framebuffer_device_t *fb, int w, int h,
							 int format, int usage,
							 IMG_native_handle_t **ppsNativeHandle,
							 int *stride);
int framebuffer_device_free(framebuffer_device_t *fb,
                            IMG_native_handle_t *psNativeHandle);

int framebuffer_setup(const hw_module_t *module, hw_device_t **device);

#endif /* FRAMEBUFFER_H */
