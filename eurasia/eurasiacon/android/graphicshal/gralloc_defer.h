/*!****************************************************************************
@File           gralloc_defer.h

@Title          Graphics Allocator (gralloc) HAL component

@Author         Imagination Technologies

@Date           2011/08/01

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

#ifndef GRALLOC_DEFER_H
#define GRALLOC_DEFER_H

#include "hal_private.h"
#include "mapper.h"
#include "hal.h"

typedef int (*IMG_defer_flush_op_pfn)(
	IMG_private_data_t *psPrivateData,
	IMG_mapper_meminfo_t *psMapperMemInfo,
	int aiFd[MAX_SUB_ALLOCS]);

int CheckDeferFlushOp(IMG_private_data_t *psPrivateData,
					  IMG_defer_flush_op_pfn pfnFlushOp,
					  IMG_mapper_meminfo_t *psMapperMemInfo,
					  int aiFd[MAX_SUB_ALLOCS]);

#endif /* GRALLOC_DEFER_H */
