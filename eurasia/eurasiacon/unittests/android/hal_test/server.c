/*!****************************************************************************
@File           server.c

@Title          Graphics HAL (example server)

@Author         Imagination Technologies

@Date           2009/03/19

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

@Description    Graphics HAL (example server)

******************************************************************************/
/******************************************************************************
Modifications :-
$Log: server.c $
******************************************************************************/

#include <stdlib.h>
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "fdsocket.h"

#include "services.h"

#define WIDTH	1019
#define HEIGHT	2048

int main(void)
{
	buffer_handle_t buffer_handle;
	gralloc_module_t *module;
	alloc_device_t *device;
	int err, stride;
	void *vaddr;

	err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID,
						(const hw_module_t **)&module);
	if(err)
	{
		printf("hw_get_module failed (err=%d)\n", err);
		return err;
	}

	err = module->common.methods->open((const hw_module_t *)module,
									   GRALLOC_HARDWARE_GPU0,
									   (hw_device_t **)&device);
	if(err)
	{
		printf("module->common.methods->open() failed (err=%d)\n", err);
		return err;
	}

	err = device->alloc(device, WIDTH, HEIGHT, HAL_PIXEL_FORMAT_RGB_565,
						GRALLOC_USAGE_SW_READ_OFTEN |
						GRALLOC_USAGE_SW_WRITE_OFTEN,
						&buffer_handle, &stride);
	if(err)
	{
		printf("device->alloc() failed (err=%d)\n", err);
		return err;
	}

	/* Check we can write to the entire MemInfo from this process */

	stride *= 2; /* 565 = 2 bytes per pixel */
	printf("computed stride %d bytes\n", stride);
	printf("write checking %d bytes\n", stride * HEIGHT);

	err = module->lock(module, buffer_handle,
					   GRALLOC_USAGE_SW_WRITE_OFTEN,
					   0, 0, WIDTH, HEIGHT, &vaddr);
	if(err)
	{
		printf("module->lock() failed (err=%d)\n", err);
		return err;
	}

	memset(vaddr, 0, stride * HEIGHT);

	err = module->unlock(module, buffer_handle);
	if(err)
	{
		printf("module->unlock() failed (err=%d)\n", err);
		return err;
	}

	if(!waitForClientSendHandle((native_handle_t *)buffer_handle))
	{
		printf("waitForClientSendHandle() failed\n");
		return -EFAULT;
	}

	sleep(2);

	err = device->free(device, buffer_handle);
	if(err)
	{
		printf("device->free() failed (err=%d)\n", err);
		return err;
	}

	err = device->common.close((hw_device_t *)device);
	if(err)
	{
		printf("hal->close() failed (err=%d)\n", err);
		return err;
	}

	return err;
}
