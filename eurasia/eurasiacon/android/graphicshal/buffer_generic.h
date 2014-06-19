/*!****************************************************************************
@File           buffer_generic.h

@Title          Graphics Allocator (gralloc) HAL component

@Author         Imagination Technologies

@Date           2011/07/05

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

#ifndef BUFFER_GENERIC_H
#define BUFFER_GENERIC_H

#include "hal_private.h"
#include "hal.h"

/* Other allocators may want to use or wrap these implementations */

int GenericAlloc(PVRSRV_DEV_DATA *psDevData, IMG_HANDLE hGeneralHeap,
				 int iAllocBytes, IMG_UINT32 ui32Flags,
				 IMG_VOID *pvPrivData, IMG_UINT32 ui32PrivDataLength,
				 PVRSRV_CLIENT_MEM_INFO **ppsMemInfo, int *piFd);

int GenericFree(PVRSRV_DEV_DATA *psDevData,
				PVRSRV_CLIENT_MEM_INFO *apsMemInfo[MAX_SUB_ALLOCS],
				int aiFd[MAX_SUB_ALLOCS]);

#endif /* BUFFER_GENERIC_H */
