/*!****************************************************************************
@File           client.c

@Title          Graphics HAL (example client)

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

@Description    Graphics HAL (example client)

******************************************************************************/
/******************************************************************************
Modifications :-
$Log: client.c $
******************************************************************************/

#include <stdlib.h>
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "fdsocket.h"

#define ALIGN(x,a)	(((x) + (a) - 1L) & ~((a) - 1L))
#define HW_ALIGN	32

#define WIDTH		1019
#define HEIGHT		2048

#define STRIDE		2*ALIGN(WIDTH,HW_ALIGN)

int main(void)
{
	buffer_handle_t buffer_handle;
	gralloc_module_t *module;
	void *vaddr;
	int err;

	err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID,
						(const hw_module_t **)&module);
	if(err)
	{
		printf("hw_get_module failed (err=%d)\n", err);
		return err;
	}

	if(!connectServerRecvHandle((native_handle_t **)&buffer_handle))
	{
		printf("connectServerRecvHandle() failed\n");
		return -EFAULT;
	}

	err = module->registerBuffer(module, buffer_handle);
	if(err)
	{
		printf("module->registerBuffer() failed (err=%d)\n", err);
		return err;
	}

	err = module->lock(module, buffer_handle,
					   GRALLOC_USAGE_SW_READ_OFTEN,
					   0, 0, WIDTH, HEIGHT, &vaddr);
	if(err)
	{
		printf("module->lock() failed (err=%d)\n", err);
		return err;
	}

	printf("assuming stride %ld bytes\n", STRIDE);
	printf("write checking %ld bytes\n", STRIDE * HEIGHT);

	/* Check we can write to the entire MemInfo from this process */
	memset(vaddr, 0, STRIDE * HEIGHT);

	err = module->unlock(module, buffer_handle);
	if(err)
	{
		printf("module->unlock() failed (err=%d)\n", err);
		return err;
	}

	err = module->unregisterBuffer(module, buffer_handle);
	if(err)
	{
		printf("module->unregisterBuffer() failed (err=%d)\n", err);
		return err;
	}

	return err;
}
