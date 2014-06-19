/*!****************************************************************************
@File           hal.h

@Title          Graphics HAL core

@Author         Imagination Technologies

@Date           2009/03/19

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

#ifndef HAL_H
#define HAL_H

#ifndef I_KNOW_WHAT_I_AM_DOING
#error Do not include HAL headers unless you know what you are doing
#else
#undef I_KNOW_WHAT_I_AM_DOING
#endif

#include <hardware/gralloc.h>

#include "hal_public.h"

#include "pvr_debug.h"
#include "services.h"

typedef struct _IMG_gralloc_module_
{
	IMG_gralloc_module_public_t base;
	int (*map)(gralloc_module_t const* module, IMG_UINT64 ui64Stamp,
			   int usage, PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS]);
	int (*unmap)(gralloc_module_t const* module, IMG_UINT64 ui64Stamp);
	void (*LogSyncs)(struct _IMG_gralloc_module_ *psGrallocModule);
	const IMG_buffer_format_public_t *(*GetBufferFormats)(void);
	IMG_BOOL (*IsCompositor)(void);
#if defined(PVR_ANDROID_NEEDS_ACCUM_SYNC_WORKAROUND)
	int (*SetAccumBuffer)(gralloc_module_t const* module,
						  IMG_UINT64 ui64Stamp, IMG_UINT64 ui64AccumStamp);
#endif
#if defined(HAL_PRIVATE_H)
	IMG_private_data_t sPrivateData;
#endif
}
IMG_gralloc_module_t;

#endif /* HAL_H */
