/******************************************************************************
* Name         : osfunc_um.c
* Title        : platform related function header
*
* Copyright    : 2003-2006 by Imagination Technologies Limited.
*                All rights reserved.  No part of this software, either
*                material or conceptual may be copied or distributed,
*                transmitted, transcribed, stored in a retrieval system
*                or translated into any human or computer language in any
*                form by any means, electronic, mechanical, manual or
*                other-wise, or disclosed to third parties without the
*                express written permission of Imagination Technologies
*                Limited, Unit 8, HomePark Industrial Estate,
*                King's Langley, Hertfordshire, WD4 8LZ, U.K.
*
* Description  : Platform related function templated and definitions
*
* Platform     :
*
* Modifications:-
* $Log: osfunc_um.c $
* 
*  --- Revision Logs Removed --- 
*
*  --- Revision Logs Removed --- 
******************************************************************************/
#include <sys/time.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <locale.h>
#include <errno.h>

#include "sys/types.h"
#include "signal.h"

#include "services.h"
#include "pvr_bridge.h"
#include "osfunc_client.h"
#include "pvr_bridge_client.h"
#include "servicesext.h"
#include "pvr_debug.h"

/*
 * FIXME:
 * Android's a bit broken in this area.
 * These hacks work around it; bugs have been filed.
 */
#if defined(SUPPORT_ANDROID_PLATFORM)
#if defined(__arm__)
#include <sys/syscall.h>
static int clock_nanosleep(clockid_t clock_id, int flags,
						   const struct timespec *request,
						   struct timespec *remain)
{
	return syscall(__NR_clock_nanosleep, clock_id, flags, request, remain);
}
#else /* defined(__arm__) */
static int clock_nanosleep(clockid_t clock_id, int flags,
						   const struct timespec *request,
						   struct timespec *remain)
{
	PVR_UNREFERENCED_PARAMETER(clock_id);
	PVR_UNREFERENCED_PARAMETER(flags);
	return nanosleep(request, remain);
}
#endif /* defined(__arm__) */
#endif /* defined(SUPPORT_ANDROID_PLATFORM) */

/******************************************************************************
 Function Name      : PVRSRVAllocUserModeMem
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT IMG_PVOID IMG_CALLCONV PVRSRVAllocUserModeMem(IMG_UINT32 ui32Size)
{
	return (IMG_PVOID)malloc(ui32Size);
}


/******************************************************************************
 Function Name      : PVRSRVCallocUserModeMem
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT IMG_PVOID IMG_CALLCONV PVRSRVCallocUserModeMem(IMG_UINT32 ui32Size)
{
	return (IMG_PVOID)calloc(1,ui32Size);
}


/******************************************************************************
 Function Name      : PVRSRVReallocUserModeMem
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT IMG_PVOID IMG_CALLCONV PVRSRVReallocUserModeMem(IMG_PVOID pvBase, IMG_SIZE_T uNewSize)
{
 	return realloc (pvBase, uNewSize);
}


/******************************************************************************
 Function Name      : PVRSRVFreeUserModeMem
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT IMG_VOID IMG_CALLCONV PVRSRVFreeUserModeMem(IMG_PVOID pvMem)
{
	free(pvMem);
}


/******************************************************************************
 Function Name      : PVRSRVMemCopy
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT IMG_VOID PVRSRVMemCopy(IMG_VOID 	*pvDst,
								const IMG_VOID 	*pvSrc,
								IMG_UINT32 	ui32Size)
{
	memcpy(pvDst, pvSrc, ui32Size);
}


/******************************************************************************
 Function Name      : PVRSRVMemSet
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT IMG_VOID PVRSRVMemSet(IMG_VOID *pvDest, IMG_UINT8 ui8Value, IMG_UINT32 ui32Size)
{
	memset(pvDest, (IMG_INT) ui8Value, (size_t) ui32Size);
}


/******************************************************************************
 Function Name      : PVRSRVLoadLibrary
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT IMG_HANDLE PVRSRVLoadLibrary(const IMG_CHAR *szLibraryName)
{
	IMG_HANDLE	hLib;
	
	hLib = (IMG_HANDLE)dlopen(szLibraryName, RTLD_LAZY);

	return hLib;
}


/******************************************************************************
 Function Name      : PVRSRVUnloadLibrary
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT PVRSRV_ERROR PVRSRVUnloadLibrary(IMG_HANDLE hExtDrv)
{
	if (hExtDrv != IMG_NULL)
	{
		if (dlclose((IMG_VOID*)hExtDrv) == 0)
		{
			return PVRSRV_OK;	
		}
		else
		{
			PVR_DPF((PVR_DBG_ERROR,"PVRSRVUnloadLibrary, dlclose failed to close library"));
		}
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR,"PVRSRVUnloadLibrary, invalid hExtDrv"));
	}

	return PVRSRV_ERROR_UNLOAD_LIBRARY_FAILED;
}


/******************************************************************************
 Function Name      : PVRSRVGetLibFuncAddr
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT PVRSRV_ERROR PVRSRVGetLibFuncAddr(IMG_HANDLE hExtDrv, const IMG_CHAR *szFunctionName, IMG_VOID **ppvFuncAddr)
{
    *ppvFuncAddr = dlsym((IMG_VOID*)hExtDrv, szFunctionName);
    
	if (*ppvFuncAddr == NULL)
	{
		return PVRSRV_ERROR_UNABLE_GET_FUNC_ADDR;
	}
	
	return PVRSRV_OK;
}


#if defined(CLOCK_MONOTONIC) && defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 199309) && defined(_XOPEN_SOURCE) && (_XOPEN_SOURCE >= 600)
/******************************************************************************
 Function Name      : PVRSRVClockus
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT IMG_UINT32 PVRSRVClockus(IMG_VOID)
{
	struct timespec stv;
	IMG_INT iRes;

	iRes = clock_gettime(CLOCK_MONOTONIC, &stv);

	if (iRes != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: clock_gettime failed (%d)", __FUNCTION__, (IMG_INT)errno));
		abort();
	}

	return (IMG_UINT32)(((unsigned long)stv.tv_nsec / 1000ul) + ((unsigned long)stv.tv_sec * 1000000ul));
}


/******************************************************************************
 Function Name      : PVRSRVWaitus
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT IMG_VOID PVRSRVWaitus(IMG_UINT32 ui32Timeus)
{
	struct timespec stReq, stRem;
	IMG_INT iRes;

	stReq.tv_sec = (__time_t)(ui32Timeus / 1000000);
	stReq.tv_nsec = (IMG_INT32)((ui32Timeus % 1000000) * 1000);

	do
	{
		iRes = clock_nanosleep(CLOCK_MONOTONIC, 0, &stReq, &stRem);
		stReq = stRem;
	} while ((iRes != 0 && errno == EINTR) || (iRes == EINTR));

	if ((iRes != 0 && errno != EINTR) && (iRes != EINTR))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: clock_nanosleep failed (%d)", __FUNCTION__, (IMG_INT)errno));
		abort();
	}
}


#else	/* defined(CLOCK_MONOTONIC) && ... */
#if defined(SUPPORT_PVRSRVCLOCKUS_USING_GETTIMEOFDAY)
/******************************************************************************
 Function Name      : PVRSRVClockus
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : PVRSRVClockus using gettimeofday.  If this function
		    : is used for timing loops, and the clock is changed
		    : (e.g. by calling settimeofday) the results may be
		    : unexpected.

******************************************************************************/
IMG_EXPORT IMG_UINT32 PVRSRVClockus(IMG_VOID)
{
	struct timeval stv;
	IMG_INT iRes;

	iRes = gettimeofday(&stv, NULL);

	if (iRes != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: gettimeofday failed (%d)", __FUNCTION__, (IMG_INT)errno));
		abort();
	}

	return (IMG_UINT32)((unsigned long)stv.tv_usec + ((unsigned long)stv.tv_sec * 1000000ul));
}
#else	/* defined(SUPPORT_PVRSRVCLOCKUS_USING_GETTIMEOFDAY) */
/******************************************************************************
 Function Name      : PVRSRVClockus
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : PVRSRVClockus using "clock".  This returns the amount
		    : of CPU time the process has used, which is monotonic,
		    : but if used in a timing loop, may result in the loop
		    : executing for longer than expected.

******************************************************************************/
IMG_EXPORT IMG_UINT32 PVRSRVClockus(IMG_VOID)
{
	return (IMG_UINT32)clock();
}
#endif	/* defined(SUPPORT_PVRSRVCLOCKUS_USING_GETTIMEOFDAY) */

/******************************************************************************
 Function Name      : PVRSRVWaitus
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT IMG_VOID PVRSRVWaitus(IMG_UINT32 ui32Timeus)
{
	struct timespec stReq, stRem;
	IMG_INT iRes;

	stReq.tv_sec = (__time_t)(ui32Timeus / 1000000);
	stReq.tv_nsec = (IMG_INT32)((ui32Timeus % 1000000) * 1000);

	do {
		iRes = nanosleep(&stReq, &stRem);
		stReq = stRem;
	} while (iRes != 0 && errno == EINTR);

	if (iRes != 0 && errno != EINTR)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: nanosleep failed (%d)", __FUNCTION__, (IMG_INT)errno));
		abort();
	}
}
#endif	/* defined(CLOCK_MONOTONIC) && ... */

/******************************************************************************
 Function Name      : PVRSRVReleaseThreadQuanta
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT IMG_VOID PVRSRVReleaseThreadQuanta(IMG_VOID)
{
	sleep(0);
}


/******************************************************************************
 Function Name      : OSGetCurrentProcessID
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT IMG_UINT32 PVRSRVGetCurrentProcessID(IMG_VOID)
{
	static IMG_UINT32 ui32ProcessId = 0;

	return ui32ProcessId ? ui32ProcessId : (ui32ProcessId = (IMG_UINT32)getpid());
}


/******************************************************************************
 Function Name      : PVRSRVSetLocale
 Inputs             : Category, Locale
 Outputs            : 
 Returns            : Current Locale if Locale is NULL
 Description        : Thin wrapper on posix setlocale
                      
******************************************************************************/
IMG_EXPORT IMG_CHAR * PVRSRVSetLocale(const IMG_CHAR *pszLocale)
{
	return setlocale(LC_ALL, pszLocale);
}

/*****************************************************************************
 Function Name      : OSTermClient
 Inputs             : pszMsg - message string to display
					  ui32ErrCode - error identifier
 Outputs            : none
 Returns            : void
 Description        : Terminates a client app which has become unusable.
******************************************************************************/
IMG_INTERNAL
IMG_VOID IMG_CALLCONV OSTermClient(IMG_CHAR* pszMsg, IMG_UINT32 ui32ErrCode)
{
#if defined(PVRSRV_CLIENT_RESET_ON_HWTIMEOUT)
	pid_t pid = getpid();
#endif
#if defined(PVRSRV_NEED_PVR_DPF)
	PVR_DPF((PVR_DBG_ERROR, "%s: %s (error %d).", __func__, pszMsg, ui32ErrCode));
#else
	PVR_UNREFERENCED_PARAMETER(pszMsg);
	PVR_UNREFERENCED_PARAMETER(ui32ErrCode);
#endif

#if defined(PVRSRV_CLIENT_RESET_ON_HWTIMEOUT)
	kill(pid, SIGTERM); /* send terminate signal */
#endif
}

/*****************************************************************************
 Function Name      : OSEventObjectWait
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_INTERNAL
PVRSRV_ERROR IMG_CALLCONV OSEventObjectWait(const PVRSRV_CONNECTION *psConnection,
#if defined (SUPPORT_SID_INTERFACE)
											IMG_EVENTSID hOSEvent)
#else
											IMG_HANDLE hOSEvent)
#endif
{
	PVRSRV_BRIDGE_RETURN sBridgeOut;
	PVRSRV_BRIDGE_IN_EVENT_OBJECT_WAIT sBridgeIn;


	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));
	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));	
	
	if (!psConnection || (!psConnection->hServices))
	{
		PVR_DPF((PVR_DBG_ERROR, "OSEventObjectWait: Invalid connection"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hOSEventKM = hOSEvent;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_EVENT_OBJECT_WAIT,
						&sBridgeIn,
						sizeof(PVRSRV_BRIDGE_IN_EVENT_OBJECT_WAIT),
						&sBridgeOut, 
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "OSEventObjectWait: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sBridgeOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_MESSAGE, "OSEventObjectWait: Error %d returned", sBridgeOut.eError));
		return sBridgeOut.eError;
	}

	return PVRSRV_OK;
}

/*****************************************************************************
 Function Name      : OSEventObjectOpen
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_INTERNAL
PVRSRV_ERROR OSEventObjectOpen(IMG_CONST PVRSRV_CONNECTION *psConnection,
								   PVRSRV_EVENTOBJECT *psEventObject,
#if defined (SUPPORT_SID_INTERFACE)
								   IMG_EVENTSID *phOSEvent)
#else
								   IMG_HANDLE *phOSEvent)
#endif
{
	PVRSRV_BRIDGE_OUT_EVENT_OBJECT_OPEN sBridgeOut;
	PVRSRV_BRIDGE_IN_EVENT_OBJECT_OPEN sBridgeIn;
	
	PVR_UNREFERENCED_PARAMETER(psConnection);

	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));
	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));

	if (!psConnection || (!psConnection->hServices))
	{
		PVR_DPF((PVR_DBG_ERROR, "OSEventObjectOpen: Invalid connection"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.sEventObject = *psEventObject;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_EVENT_OBJECT_OPEN,
						&sBridgeIn,
						sizeof(PVRSRV_BRIDGE_IN_EVENT_OBJECT_OPEN),
						&sBridgeOut, 
						sizeof(PVRSRV_BRIDGE_OUT_EVENT_OBJECT_OPEN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "OSEventObjectOpen: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sBridgeOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "OSEventObjectOpen: Error %d returned", sBridgeOut.eError));
		return sBridgeOut.eError;
	}
	
	*phOSEvent = sBridgeOut.hOSEvent;

	return PVRSRV_OK;
}

/*****************************************************************************
 Function Name      : OSEventObjectClose
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_INTERNAL
PVRSRV_ERROR OSEventObjectClose(IMG_CONST PVRSRV_CONNECTION *psConnection,
								PVRSRV_EVENTOBJECT *psEventObject,
#if defined (SUPPORT_SID_INTERFACE)
								IMG_EVENTSID hOSEvent)
#else
								IMG_HANDLE hOSEvent)
#endif
{
	PVRSRV_BRIDGE_RETURN sBridgeOut;
	PVRSRV_BRIDGE_IN_EVENT_OBJECT_CLOSE sBridgeIn;
	
	if (!psConnection || (!psConnection->hServices))
	{
		PVR_DPF((PVR_DBG_ERROR, "OSEventObjectClose: Invalid connection"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hOSEventKM = hOSEvent;
	sBridgeIn.sEventObject = *psEventObject;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_EVENT_OBJECT_CLOSE,
						&sBridgeIn,
						sizeof(PVRSRV_BRIDGE_IN_EVENT_OBJECT_CLOSE),
						&sBridgeOut, 
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "OSEventObjectClose: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sBridgeOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_WARNING, "OSEventObjectClose: Error %d returned", sBridgeOut.eError));
		return sBridgeOut.eError;
	}

	return PVRSRV_OK;
}


/*****************************************************************************
 Function Name      : OSIsProcessPrivileged
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_INTERNAL
IMG_BOOL IMG_CALLCONV OSIsProcessPrivileged(IMG_VOID)
{
#if defined(ANDROID)
	return IMG_TRUE;
#else /* defined(ANDROID) */
	/* 
		This API must be implemented when porting to a new platform
		This API is used for priority based hardware scheduling
	*/
	PVR_DPF((PVR_DBG_ERROR, "OSIsProcessPrivileged: not implemented, default returns false"));
	
	return IMG_FALSE;
#endif /* defined(ANDROID) */
}

#if defined(__i386__)

static IMG_UINT32 gui32CacheLineSize;

/* QAC does not recognise assembler so warnings generated are overriden */
/* PRQA S 3196,3199,3203,3205,3321 END_DETERCACHE */
static IMG_VOID DetermineCacheLineSize(IMG_VOID)
{
	IMG_UINT32 eax, ebx, edx;

	/* Func 0x1 for clflush */
	eax = 0x1;

	/* Need to save/restore ebx from the stack
	 * and source cpuid ebx from register esi
	 */
	asm volatile(
		"pushl %%ebx\n"
		"cpuid\n"
		"movl %%ebx, %%esi\n"
		"popl %%ebx"
			: "=S" (ebx), "=d" (edx)	/* Outputs */
			: "a"  (eax)			/* Inputs */
			: "ecx"				/* Clobbers */
	);

	/* FIXME: Don't fail hard if the CPU doesn't support clflush.. */

	/* Check Extended Model Number for clflush bit */
	PVR_ASSERT((edx & (1 << 19)) != 0);

	/* Second byte for cacheline size */
	gui32CacheLineSize = ((ebx >> 8) & 0xff) * 8;
/* end of assembler warnings override */
/* PRQA L:END_DETERCACHE */
}

#define ROUND_UP(x,a) (((x) + (a) - 1) & ~((a) - 1))

IMG_INTERNAL PVRSRV_ERROR OSFlushCPUCacheRange(IMG_VOID *pvRangeAddrStart,
											   IMG_VOID *pvRangeAddrEnd)
{
	IMG_BYTE *pbStart = pvRangeAddrStart;
	IMG_BYTE *pbEnd = pvRangeAddrEnd;
	IMG_BYTE *pbBase;

	if(!gui32CacheLineSize)
		DetermineCacheLineSize();

	/* If the end address isn't aligned to cache line size, round it
	 * up to the nearest multiple. This ensures that we flush all
	 * the cache lines affected by unaligned start/end addresses.
	 */
	pbEnd = (IMG_BYTE *)ROUND_UP((IMG_UINTPTR_T)pbEnd, gui32CacheLineSize);

	asm volatile("mfence");

	for(pbBase = pbStart; pbBase < pbEnd; pbBase += gui32CacheLineSize)
		asm volatile("clflush %0" : "+m" (*pbBase));

	asm volatile("mfence");

	return PVRSRV_OK;
}

#else /* defined(__i386__) */

IMG_INTERNAL PVRSRV_ERROR OSFlushCPUCacheRange(IMG_VOID *pvRangeAddrStart,
											   IMG_VOID *pvRangeAddrEnd)
{
	PVR_UNREFERENCED_PARAMETER(pvRangeAddrStart);
	PVR_UNREFERENCED_PARAMETER(pvRangeAddrEnd);
	
	return PVRSRV_ERROR_NOT_SUPPORTED;
}

#endif /* defined(__i386__) */
