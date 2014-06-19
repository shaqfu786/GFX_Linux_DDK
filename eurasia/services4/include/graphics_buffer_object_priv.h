/*!****************************************************************************
@File          graphics_buffer_object_priv.h

@Title         Common Buffer Interface definitions - internal

@Author        Imagination Technologies

@date          Dec 2011

* Copyright © 2011 Texas Instruments Incorporated
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* Author(s):
*    Atanas (Tony) Zlatinski <x0146664@ti.com> <zlatinski@gmail.com>

@Platform       Generic

@Description    Contains the private structure definition for the
                implementation of the Common Buffer Interface.

@DoxygenVer

******************************************************************************/

#ifndef graphics_buffer_object_priv_h__
#define graphics_buffer_object_priv_h__

/* Support for Graphics Buffer Interface and Eviction */
#include <graphics_buffer_interface_base.h>

typedef enum PVRSRV_GPU_MEMORY_STATE_
{
	PVRSRV_GPU_MEMORY_INVALID_STATE = 0,              /* before init and free (deleted) */
	PVRSRV_GPU_MEMORY_ACTIVE_STATE,                     /* in sBuffActiveList */
	PVRSRV_GPU_MEMORY_PENDING_EVICTION_STATE,   /* in sBuffEvictionPendingList */
	PVRSRV_GPU_MEMORY_EVICTED_STATE,                  /* in sBuffEvictedList */
	PVRSRV_GPU_MEMORY_PENDING_FREE_STATE,		/* in sBuffFreePendingList */
	PVRSRV_GPU_MEMORY_LAST_STATE =(IMG_UINT32)-1,
}PVRSRV_GPU_MEMORY_STATE;

#define ENABLE_PVRSRV_GPU_GUARD_AREA 8

#ifdef ENABLE_PVRSRV_GPU_GUARD_AREA
/* Structure guarding area */
typedef struct PVRSRV_GPU_GUARD_AREA
{
	IMG_UINT32 guard[ENABLE_PVRSRV_GPU_GUARD_AREA];
}PVRSRV_GPU_GUARD_AREA;
#endif

typedef struct PVRSRV_GPU_MEMORY_RESOURCE_OBJECT_
{
	/* This must be the first member */
	PVRSRV_CLIENT_MEM_INFO  sClientMemInfo;

#ifdef ENABLE_PVRSRV_GPU_GUARD_AREA
	PVRSRV_GPU_GUARD_AREA guardBefore;
#endif

	IMG_INT32 validObjectToken;

	/* Mutex protecting the integrity of
	 * the memory structure */
	PVRSRV_MUTEX_HANDLE hMutex;

	/* Unified resource memory interface */
	PVRSRV_RESOURCE_OBJECT_BASE sMemResourceObjectBase;

	/* Resource memory GPU mapping interface */
	PVRSRV_GPU_RESOURCE_OBJECT sGpuMappingResourceObject;

	/* Resource memory CPU mapping interface */
//	PVRSRV_CPU_RESOURCE_OBJECT sCpuMappingResourceObject;

	/* Control flags */
	IMG_UINT32 ui32Hints;

    /* This Buffer Is Exported (if not -1) */
	IMG_INT i32ExportedFd;

	/* This context is needed for memory management */
	PVRSRV_DEV_DATA *psDevData;

	/* Required size of the buffer in pixels */
	IMG_UINT32 ui32Size;

	/* Width of the buffer in pixels */
	IMG_UINT32 ui32Width;

	/* Stride of the buffer in pixels */
	IMG_UINT32 ui32Stride;

	/* Height of the buffer in pixels */
	IMG_UINT32 ui32Height;

	/* Bits per pixel of the buffer */
	IMG_UINT32 ui32Bpp;

	/* Current Object State */
	PVRSRV_GPU_MEMORY_STATE eState;

	/* Double Linked List */
	PVRSRV_LIST sListNode;

#ifdef ENABLE_PVRSRV_GPU_GUARD_AREA
	PVRSRV_GPU_GUARD_AREA guardAfter;
#endif
}PVRSRV_GPU_MEMORY_RESOURCE_OBJECT;

#endif //graphics_buffer_object_priv_h__
