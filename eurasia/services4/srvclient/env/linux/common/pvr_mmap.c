/**************************************************************************
 * Name         : pvrmmap.c
 *
 * Copyright    : 2001-2009 by Imagination Technologies Limited.
 *                  All rights reserved.
 *                  No part of this software, either material or conceptual
 *                  may be copied or distributed, transmitted, transcribed,
 *                  stored in a retrieval system or translated into any
 *                  human or computer language in any form by any means,
 *                  electronic, mechanical, manual or other-wise, or
 *                  disclosed to third parties without the express written
 *                  permission of:
 *                             Imagination Technologies Limited,
 *                             HomePark Industrial Estate,
 *                             Kings Langley,
 *                             Hertfordshire,
 *                             WD4 8LZ,
 *                             UK
 *
 * Platform     : Linux
 * Description	: Main file for PVRMMAP library
 *
 * $Log: pvr_mmap.c $
 ***************************************************************************/
#define _GNU_SOURCE

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <errno.h>
#include <pthread.h>

#include "img_defs.h"
#include "services.h"
#include "pvr_debug.h"
#include "pvrmmap.h"
#include "pvr_bridge.h"
#include "pvr_bridge_client.h"

#include <sys/syscall.h>

static pthread_mutex_t sMutex = PTHREAD_MUTEX_INITIALIZER;	/* PRQA S 0684 */ /* not too many initialisers */

static void LockMappings(void)
{
	int error;

	if ((error = pthread_mutex_lock(&sMutex)) != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Couldn't lock mutex (%d)", __FUNCTION__, error));
		abort();
	}
}

static void UnlockMappings(void)
{
	int error;

	if ((error = pthread_mutex_unlock(&sMutex)) != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Couldn't unlock mutex (%d)", __FUNCTION__, error));
		abort();
	}
}

IMG_INTERNAL PVRSRV_ERROR
#if defined (SUPPORT_SID_INTERFACE)
PVRPMapKMem(IMG_HANDLE hModule, IMG_VOID **ppvLinAddr, IMG_VOID *pvLinAddrKM, IMG_SID *phMappingInfo, IMG_SID hMHandle)
#else
PVRPMapKMem(IMG_HANDLE hModule, IMG_VOID **ppvLinAddr, IMG_VOID *pvLinAddrKM, IMG_HANDLE *phMappingInfo, IMG_HANDLE hMHandle)
#endif
{
    PVRSRV_BRIDGE_IN_MHANDLE_TO_MMAP_DATA sIn;
    PVRSRV_BRIDGE_OUT_MHANDLE_TO_MMAP_DATA sOut;
    IMG_INT i32Status=0;
    IMG_VOID *pvUserAddress=IMG_NULL;

    PVR_UNREFERENCED_PARAMETER(pvLinAddrKM);

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));

#if defined(PVR_SECURE_HANDLES) || defined (SUPPORT_SID_INTERFACE)
    sIn.hMHandle = hMHandle;
#else
    /*
     * The mapping info pointed to by phMappingInfo must be initialised to
     * the mapping info passed back by the kernel
     * (e.g. psMemInfo->hMappingInfo).
     */
    sIn.hMHandle = *phMappingInfo;
#endif

    LockMappings();

    i32Status = PVRSRVBridgeCall(hModule,
                                 PVRSRV_BRIDGE_MHANDLE_TO_MMAP_DATA,
                                 &sIn,
                                 sizeof(PVRSRV_BRIDGE_IN_MHANDLE_TO_MMAP_DATA),
                                 &sOut,
                                 sizeof(PVRSRV_BRIDGE_OUT_MHANDLE_TO_MMAP_DATA));
    if(i32Status == -1 || sOut.eError != PVRSRV_OK)
    {
#if defined (SUPPORT_SID_INTERFACE)
        PVR_DPF((PVR_DBG_ERROR,
            "%s: MHANDLE_TO_MMAP_DATA failed: handle=%x, error %d",
            __FUNCTION__, hMHandle, sOut.eError));
#else
        PVR_DPF((PVR_DBG_ERROR,
            "%s: MHANDLE_TO_MMAP_DATA failed: handle=%p, error %d",
            __FUNCTION__, hMHandle, sOut.eError));
#endif
	goto exit_error_unlock;
    }

    if (sOut.ui32UserVAddr != 0)
    {
         pvUserAddress = (IMG_VOID *)sOut.ui32UserVAddr;
    }
    else
    {
        /* On GLIBC and ARM android platforms we'll use syscall()
         * to call mmap2, since neither export the mmap2() entry point.
         * On i686 android we can call it directly.
         */
#if defined(SUPPORT_ANDROID_PLATFORM) && defined(__i386__)
        extern void*  __mmap2(void*, size_t, int, int, int, size_t);

        pvUserAddress = (IMG_VOID *) __mmap2(NULL,
                                  sOut.ui32RealByteSize,
                                  PROT_READ | PROT_WRITE,
                                  MAP_SHARED,
                                  *(IMG_INT*)hModule,
                                  (off_t)sOut.ui32MMapOffset);
#else
        pvUserAddress = (IMG_VOID *) syscall(__NR_mmap2, NULL,
                                  sOut.ui32RealByteSize,
                                  PROT_READ | PROT_WRITE,
                                  MAP_SHARED,
                                  *(IMG_INT*)hModule,	/* PVR Device */
                                  (off_t)sOut.ui32MMapOffset);
#endif
        if(pvUserAddress == MAP_FAILED)
        {
#if defined (SUPPORT_SID_INTERFACE)
	    PVR_DPF((PVR_DBG_ERROR, "%s: mmap(2) failed: Mapping handle=%x: %s",
                __FUNCTION__, hMHandle, strerror(errno)));
#else
	    PVR_DPF((PVR_DBG_ERROR, "%s: mmap(2) failed: Mapping handle=%p: %s",
                __FUNCTION__, hMHandle, strerror(errno)));
#endif
	    goto exit_error_unlock;
	}
    }

    UnlockMappings();

    /*
     * phMappingInfo can be used to store information that needs to be passed
     * to PVRUnMapKMem.  We store the base address of the mapping as a
     * flag to indicate that the mapping exists.
     */
#if defined (SUPPORT_SID_INTERFACE)
    *phMappingInfo = (IMG_SID)pvUserAddress;
#else
    *phMappingInfo = (IMG_HANDLE)pvUserAddress;
#endif

    *ppvLinAddr = (IMG_CHAR *)pvUserAddress + sOut.ui32ByteOffset;

    return PVRSRV_OK;

exit_error_unlock:
    UnlockMappings();
    *ppvLinAddr = IMG_NULL;
    *phMappingInfo = IMG_NULL;

    return PVRSRV_ERROR_BAD_MAPPING;
}


IMG_INTERNAL IMG_BOOL
#if defined (SUPPORT_SID_INTERFACE)
PVRUnMapKMem(IMG_HANDLE hModule, IMG_SID hMappingInfo, IMG_SID hMHandle)
#else
PVRUnMapKMem(IMG_HANDLE hModule, IMG_HANDLE hMappingInfo, IMG_HANDLE hMHandle)
#endif
{
    PVRSRV_BRIDGE_IN_RELEASE_MMAP_DATA sIn;
    PVRSRV_BRIDGE_OUT_RELEASE_MMAP_DATA sOut;
    IMG_INT iRet;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));
	
#if defined (SUPPORT_SID_INTERFACE)
    if (hMappingInfo == 0)
#else
    if (hMappingInfo == IMG_NULL)
#endif
    {
        return IMG_TRUE;
    }

    sIn.hMHandle = hMHandle;

    LockMappings();

    iRet = PVRSRVBridgeCall(hModule,
                                 PVRSRV_BRIDGE_RELEASE_MMAP_DATA,
                                 &sIn,
                                 sizeof(PVRSRV_BRIDGE_IN_RELEASE_MMAP_DATA),
                                 &sOut,
                                 sizeof(PVRSRV_BRIDGE_OUT_RELEASE_MMAP_DATA));
    if(iRet == -1 || sOut.eError != PVRSRV_OK)
    {
#if defined (SUPPORT_SID_INTERFACE)
        PVR_DPF((PVR_DBG_ERROR,
            "%s: RELEASE_MMAP_DATA failed: handle=%x, error %d",
            __FUNCTION__, hMHandle, sOut.eError));
#else
        PVR_DPF((PVR_DBG_ERROR,
            "%s: RELEASE_MMAP_DATA failed: handle=%p, error %d",
            __FUNCTION__, hMHandle, sOut.eError));
#endif
	goto exit_error_unlock;
    }

    if (sOut.bMUnmap)
    {
        iRet = munmap((IMG_VOID *)sOut.ui32UserVAddr, sOut.ui32RealByteSize);
        if(iRet == -1)
        {
            PVR_DPF((PVR_DBG_ERROR,
		"%s: munmap: Failed to unmap memory at address 0x%x, length=%d: %s",
		__FUNCTION__, sOut.ui32UserVAddr,
		sOut.ui32RealByteSize, strerror(errno)));
	    goto exit_error_unlock;
        }
    }

exit_error_unlock:
    UnlockMappings();

    return (IMG_BOOL)((iRet != -1) && (sOut.eError == PVRSRV_OK));
}
