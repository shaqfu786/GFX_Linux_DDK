/*!****************************************************************************
@File           graphics_buffer_interface_imp.c

@Title          Common Buffer Interface Implementation

@Author         Texas Instruments

@date           01/015/2012

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

@Description    Common graphics buffer interface with support for eviction

@DoxygenVer

******************************************************************************/

#include <stddef.h>
#include <unistd.h>
#include "img_defs.h"
#include "services.h"
#include "servicesint.h"
#include "pvr_debug.h"
#include "osfunc_client.h"

#include <img_list.h>
#include <graphics_buffer_object_priv.h>

#define MAX_NUM_SYSTEMS_MEMORY_DUMP_CLIENT 16

#define ALIGN(x,a)	(((x) + (a) - 1L) & ~((a) - 1L))

static struct
{
	IMG_UINT64 ui32SubsystemsDebugMask;
	pfGpuMemoryOpRequestCbType func;
	IMG_VOID* clientContext;
}gGpuMemoryUserTbl[MAX_NUM_SYSTEMS_MEMORY_DUMP_CLIENT];


/*!
*******************************************************************************

 @Function	Name PVRSRVProcMemorySubSystemToName

 @Description

 @Input        ui64SubSystem :

 @Return       Name of the subsystem

******************************************************************************/
IMG_EXPORT IMG_CONST IMG_CHAR* IMG_CALLCONV PVRSRVProcMemorySubSystemToName(IMG_UINT64 ui64SubSystem)
{
	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_PBUFFER_SURFACE)
		return "PBUFFER_SURFACE";

	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_PIXMAP_SURFACE)
		return "PIXMAP_SURFACE";

	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_WINDOW_SURFACE)
		return "WINDOW_SURFACE";

	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_MM_BUFFER)
		return "MM_BUFFER";

	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_TILER_BUFFER)
		return "TILER_BUFFER";

	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_DEPTH_BUFFER)
	{
		if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_DEPTH_BUFFER)
			return "DEPTH_AND_STENCIL_BUFFER";

		return "DEPTH_BUFFER";
	}

	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_DEPTH_BUFFER)
		return "STENCIL_BUFFER";

	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_IMPORTED_PIXMAP)
		return "IMPORTED_PIXMAP";


	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_EGL_NOK_IMPORT)
		return "EGL_NOK_IMPORT";


	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_OPENGL1_TEXTURE)
		return "OPENGL1_TEXTURE";

	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_OPENGL1_PROGRAM)
		return "OPENGL1_PROGRAM";

	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_OPENGL1_BUFOBJ)
		return "OPENGL1_BUFOBJ";

	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_OPENGL1_RENDERBUFFER)
		return "OPENGL1_RENDERBUFFER";

	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_OPENGL1_FRAMEBUFFER)
		return "OPENGL1_FRAMEBUFFER";

	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_OPENGL1_VAO)
		return "OPENGL1_VAO";


	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_OPENGL2_TEXTURE)
		return "OPENGL2_TEXTURE";

	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_OPENGL2_PROGRAM)
		return "OPENGL2_PROGRAM";

	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_OPENGL2_BUFOBJ)
		return "OPENGL2_BUFOBJ";

	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_OPENGL2_RENDERBUFFER)
		return "OPENGL2_RENDERBUFFER";

	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_OPENGL2_FRAMEBUFFER)
		return "OPENGL2_FRAMEBUFFER";

	if(ui64SubSystem & GPU_MEMORY_SUBSYSTEM_OPENGL2_VAO)
		return "OPENGL2_VAO";


	return "UNKNOWN_BUFFER_TYPE";
}

/*!
*******************************************************************************

 @Function	Name PVRSRVProcMemoryOpRequest

 @Description

 @Input        clientContext :

 @Input        memOpRequest :

 @Return       PVRSRV_ERROR

******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVProcMemoryOpRequest(IMG_VOID* clientContext,
		PVRSRV_CB_MEMORY_OPERAION_REQUEST* memOpRequest)
{
	IMG_UINT32 tblIndex;

	PVR_UNREFERENCED_PARAMETER(clientContext);

	if(!memOpRequest)
		return  PVRSRV_ERROR_INVALID_PARAMS;

	PVRSRVLockProcessGlobalMutex();

	/* Call all the clients with this mask */
	for(tblIndex = 0; tblIndex < sizeof(gGpuMemoryUserTbl)/sizeof(gGpuMemoryUserTbl[0]); tblIndex++)
	{
		if( (gGpuMemoryUserTbl[tblIndex].ui32SubsystemsDebugMask & memOpRequest->ui32SubsystemsMask) &&
				gGpuMemoryUserTbl[tblIndex].func && gGpuMemoryUserTbl[tblIndex].clientContext )
		{
			gGpuMemoryUserTbl[tblIndex].func(gGpuMemoryUserTbl[tblIndex].clientContext,	memOpRequest);
		}
	}

	PVRSRVUnlockProcessGlobalMutex();
	return PVRSRV_OK;
}

/*!
*******************************************************************************

 @Function	Name PVRSRVUnregisterProcMemoryOpRequestUser

 @Description

 @Input        ui64SubsystemClient :

 @Input        clientContext :

 @Return       IMG_TRUE if success

******************************************************************************/
IMG_EXPORT IMG_BOOL IMG_CALLCONV PVRSRVUnregisterProcMemoryOpRequestUser(IMG_UINT64 ui64SubsystemClient,
		IMG_VOID* clientContext )
{
	IMG_UINT32 tblIndex;
	IMG_BOOL bFound = IMG_FALSE;

	PVRSRVLockProcessGlobalMutex();
	for(tblIndex = 0; tblIndex < sizeof(gGpuMemoryUserTbl)/sizeof(gGpuMemoryUserTbl[0]); tblIndex++)
	{
		if( (gGpuMemoryUserTbl[tblIndex].ui32SubsystemsDebugMask & ui64SubsystemClient) )
		{
			if(gGpuMemoryUserTbl[tblIndex].clientContext != clientContext)
			{
				PVR_DPF((PVR_DBG_WARNING," %s: Unregister of %llx with a different client context %p!!!",
						__FUNCTION__,  ui64SubsystemClient, clientContext));
			}
			gGpuMemoryUserTbl[tblIndex].func = IMG_NULL;
			gGpuMemoryUserTbl[tblIndex].clientContext = IMG_NULL;
			gGpuMemoryUserTbl[tblIndex].ui32SubsystemsDebugMask = 0;
			bFound = IMG_TRUE;
			break;
		}
	}
	PVRSRVUnlockProcessGlobalMutex();
	return bFound;
}

/*!
*******************************************************************************

 @Function	Name PVRSRVRegisterProcMemoryOpRequestUser

 @Description

 @Input        ui64SubsystemClient : Subsystem client mask

 @Input        pFunc : CB function

 @Input        clientContext : Client Context

 @Return       IMG_TRUE id success

******************************************************************************/
IMG_EXPORT IMG_BOOL IMG_CALLCONV PVRSRVRegisterProcMemoryOpRequestUser(IMG_UINT64 ui64SubsystemClient,
		pfGpuMemoryOpRequestCbType pFunc, IMG_VOID* clientContext )
{
	IMG_UINT32 tblIndex;
	IMG_BOOL bFound = IMG_FALSE;

	/* Make sure our client mask is unique */
	PVRSRVUnregisterProcMemoryOpRequestUser(ui64SubsystemClient, clientContext);

	PVRSRVLockProcessGlobalMutex();
	/* Find an empty Client mask */
	for(tblIndex = 0; tblIndex < sizeof(gGpuMemoryUserTbl)/sizeof(gGpuMemoryUserTbl[0]); tblIndex++)
	{
		if(gGpuMemoryUserTbl[tblIndex].ui32SubsystemsDebugMask == 0)
		{
			/* Install the callback */
			gGpuMemoryUserTbl[tblIndex].ui32SubsystemsDebugMask = ui64SubsystemClient;
			gGpuMemoryUserTbl[tblIndex].clientContext = clientContext;
			gGpuMemoryUserTbl[tblIndex].func = pFunc;
			bFound = IMG_TRUE;
			break;
		}
	}

	PVRSRVUnlockProcessGlobalMutex();
	/* No Empty slots found */
	return bFound;
}

/*!
*******************************************************************************

 @Function	Name PVRSRVProcBufferReportMemoryUse

 @Description

 @Input        psDevData :

 @Return       PVRSRV_ERROR

******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVProcBufferReportMemoryUse(IMG_CONST PVRSRV_DEV_DATA *psDevData)
{
	PVRSRV_CB_MEMORY_OPERAION_REQUEST memOpRequest;

	PVRSRVMemSet(&memOpRequest, 0x00, sizeof(memOpRequest));
	memOpRequest.ui32SubsystemsMask = GPU_MEMORY_SUBSYSTEM_ALL;
	memOpRequest.ctrlFlags = GPU_MEMORY_CTRL_DUMP
			| GPU_MEMORY_COLLECT_MEMORY_USE_INFO;

	PVRSRVProcMemoryOpRequest((IMG_VOID *)psDevData, &memOpRequest);

	return PVRSRV_OK;
}

/*!
*******************************************************************************

 @Function	Name PVRSRVTriggerProcBufferMemoryReport

 @Description

 @Input        psDevData :

 @Return       PVRSRV_ERROR

******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVTriggerProcBufferMemoryReport(IMG_CONST PVRSRV_DEV_DATA *psDevData)
{
	PVR_UNREFERENCED_PARAMETER(psDevData);
	/* Asynchronous Request - TODO */
	return PVRSRV_OK;
}


inline static
IMG_BOOL IsPendingAheadOfCompleteOps(IMG_UINT32 pending,IMG_UINT32 complete)
{
	return ((IMG_INT32)complete - (IMG_INT32)pending < 0);
}

/*****************************************************************************
 @Function	PVRSRVMemoryBufferInUseByGpu
 @Return    result of query of the the memory:
		-1: Error;
		 0: Not in use
	   > 0: In use
 @Description : Returns information about the GPU use of the buffer represented by psMemInfo
******************************************************************************/
inline static
IMG_INT32 PVRSRVMemoryBufferInUseByGpu(PVRSRV_CLIENT_MEM_INFO *psMemInfo)
{
	IMG_UINT32 ui32OpsPending;

	if(!psMemInfo)
	{
		return 0;
	}

	if(!psMemInfo->psClientSyncInfo)
		return 0;

	// Snapshot the pending counters
	ui32OpsPending =  psMemInfo->psClientSyncInfo->psSyncData->ui32WriteOpsPending;
	if(ui32OpsPending)
	{
		const IMG_UINT32 ui32WriteOpsComplete = psMemInfo->psClientSyncInfo->psSyncData->ui32WriteOpsComplete;
		if(IsPendingAheadOfCompleteOps(ui32OpsPending, ui32WriteOpsComplete))
		{
			return (ui32OpsPending - ui32WriteOpsComplete);
		}
	}

	ui32OpsPending =  psMemInfo->psClientSyncInfo->psSyncData->ui32ReadOpsPending;
	if(ui32OpsPending)
	{
		const IMG_UINT32 ui32ReadOpsComplete = psMemInfo->psClientSyncInfo->psSyncData->ui32ReadOpsComplete;
		if(IsPendingAheadOfCompleteOps(ui32OpsPending, ui32ReadOpsComplete))
		{
			return (ui32OpsPending - ui32ReadOpsComplete);
		}
	}

	ui32OpsPending =  psMemInfo->psClientSyncInfo->psSyncData->ui32ReadOps2Pending;
	if(ui32OpsPending)
	{
		const IMG_UINT32 ui32ReadOps2Complete = psMemInfo->psClientSyncInfo->psSyncData->ui32ReadOps2Complete;
		if(IsPendingAheadOfCompleteOps(ui32OpsPending, ui32ReadOps2Complete))
		{
			return (ui32OpsPending - ui32ReadOps2Complete);
		}
	}

	return 0;
}

/**********************************************************************************************/
/************************ Unified Memory Interface Functions **********************************/
/**********************************************************************************************/
#define PVRSRV_RESOURCE_OBJECT_VALID_TOKEN (IMG_UINT32)(('M' << 24) | ('R' << 16) | ('O' << 8) | ('I' << 0))

#define LOCK_MEM_RES(object) \
	{ \
		PVRSRVLockMutex(psMemResource->hMutex); \
	}

#define UNLOCK_MEM_RES(object) \
	{ \
		PVRSRVUnlockMutex(psMemResource->hMutex); \
	}

#define MEM_RES_FUNC_ENTER(object) \
	{ \
		PVR_DPF((PVR_DBG_CALLTRACE, "Enter %s with object %p", __FUNCTION__, (object))); \
	}


#define MEM_RES_FUNC_EXIT(object, ret_val, ref_count) \
	{ \
		PVR_DPF((PVR_DBG_CALLTRACE, "Exit %s with object %p, ret %d, ref_count %d\n", __FUNCTION__, (object), (ret_val), (ref_count)));\
	}

static IMG_INT32 MemoryResourceDevMemGpuMapBuffer(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource);
static IMG_INT32 MemoryResourceDevMemGpuReleaseBuffer(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource);
static IMG_INT32 MemoryResourceDevMemReleaseBuffer(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource);

static PVRSRV_GPU_MEMORY_RESOURCE_OBJECT* MemoryResourceFromBaseObject(PVRSRV_RESOURCE_OBJECT_BASE *pObject);

static INLINE
IMG_BOOL MemoryResourceEvictionIsEnabled(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource)
{
	PVRSRV_CONNECTION *psConnection = (PVRSRV_CONNECTION *)psMemResource->psDevData->psConnection;

	if(psConnection->ui64EvictionSubSystemEnableMask & psMemResource->sClientMemInfo.uiSubSystem)
		return IMG_TRUE;

	return IMG_FALSE;
}

static INLINE
IMG_BOOL MemoryResourceDebugIsEnabled(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource)
{
	PVRSRV_CONNECTION *psConnection = (PVRSRV_CONNECTION *)psMemResource->psDevData->psConnection;

	if(psConnection->ui64EvictionSubSystemDebugMask & psMemResource->sClientMemInfo.uiSubSystem)
		return IMG_TRUE;

	return IMG_FALSE;
}

static IMG_INT32 MemoryResourceAddRefBaseObject(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource)
{
	IMG_INT32 retVal = PVRSRV_OK;

	MEM_RES_FUNC_ENTER(psMemResource);

	if(retVal == PVRSRV_OK)
	{
		psMemResource->sMemResourceObjectBase.ui32RefCount++;
	}

	MEM_RES_FUNC_EXIT(psMemResource, retVal, psMemResource->sMemResourceObjectBase.ui32RefCount);

	return retVal;
}

static IMG_INT32 MemoryResourceAddRefIf(PVRSRV_RESOURCE_OBJECT_BASE *pObject)
{
	IMG_INT32 retVal;
	PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource = MemoryResourceFromBaseObject(pObject);

	if(psMemResource == IMG_NULL)
		return  -PVRSRV_ERROR_INVALID_PARAMS;

	retVal = MemoryResourceAddRefBaseObject(psMemResource);

	UNLOCK_MEM_RES(psMemResource);

	return retVal;
}

static IMG_INT32 MemoryResourceReleaseIf(PVRSRV_RESOURCE_OBJECT_BASE *pObject, IMG_BOOL bIsShutdown)
{
	IMG_INT32 retVal = PVRSRV_OK;
	IMG_UINT32 refCount = 0;
	PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource = MemoryResourceFromBaseObject(pObject);

	if(psMemResource == IMG_NULL)
		return  -PVRSRV_ERROR_INVALID_PARAMS;

	PVR_UNREFERENCED_PARAMETER(bIsShutdown);

	MEM_RES_FUNC_ENTER(psMemResource);

	if(psMemResource->sMemResourceObjectBase.ui32RefCount == 1)
	{
		/* Unlock Mutex, because we do not know how long is going to take
		 * the PVRSRVManageDevMemFreeBuffer() func to release the memory
		 * */
		psMemResource->validObjectToken = 0x89ABCDEF;

		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Resource %p "
				"RELEASE with size %d and VADDR 0x%08x",
				psMemResource, psMemResource->sClientMemInfo.uAllocSize,
				psMemResource->sClientMemInfo.sDevVAddr.uiAddr));

		/* Must Unlock mutext before calling MemoryResourceDevMemReleaseBuffer() */
		UNLOCK_MEM_RES(psMemResource);

		retVal = MemoryResourceDevMemReleaseBuffer(psMemResource);

		/* Can not touch insight the object anymore - it is gone */
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Resource %p RELEASED",
				psMemResource));

		MEM_RES_FUNC_EXIT(psMemResource, retVal, refCount);

		return refCount;
	}
	else if(psMemResource->sMemResourceObjectBase.ui32RefCount > 0)
	{
		psMemResource->sMemResourceObjectBase.ui32RefCount--;

		refCount = psMemResource->sMemResourceObjectBase.ui32RefCount;
	}

	MEM_RES_FUNC_EXIT(psMemResource, retVal, refCount);

	UNLOCK_MEM_RES(psMemResource);

	return refCount;
}

static PVRSRV_GPU_MEMORY_RESOURCE_OBJECT* MemoryResourceFromGpuResourceObject(PVRSRV_GPU_RESOURCE_OBJECT *pGpuObject);

static IMG_INT32 MemoryResourceClaimGpuResourcesObject(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource)
{
	IMG_INT32 retVal = PVRSRV_OK;

	MEM_RES_FUNC_ENTER(psMemResource);

	if(psMemResource == IMG_NULL)
	{
		retVal =  -PVRSRV_ERROR_INVALID_PARAMS;

		MEM_RES_FUNC_EXIT(psMemResource, retVal, -1);

		return retVal;
	}

	if(psMemResource->sGpuMappingResourceObject.ui32RefCount == 0)
		MemoryResourceAddRefBaseObject(psMemResource);

	psMemResource->sGpuMappingResourceObject.ui32RefCount++;

	if(MemoryResourceDebugIsEnabled(psMemResource))
	{
		PVRSRV_DUMP_MEMINFO(PVR_DBG_WARNING,
			(&psMemResource->sClientMemInfo),
			"CLAIMED");
		PVR_DPF((PVR_DBG_WARNING, "REF COUNT of %p IS %d",
			&psMemResource->sClientMemInfo,
			psMemResource->sGpuMappingResourceObject.ui32RefCount));
	}

	MEM_RES_FUNC_EXIT(psMemResource, retVal, psMemResource->sGpuMappingResourceObject.ui32RefCount);

	return retVal;
}

static IMG_INT32 MemoryResourceClaimGpuResourcesIf(PVRSRV_RESOURCE_OBJECT_BASE *pGpuObject)
{
	IMG_INT32 retVal;
	PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource =
			MemoryResourceFromGpuResourceObject((PVRSRV_GPU_RESOURCE_OBJECT *)pGpuObject);

	if(psMemResource == IMG_NULL)
	{
		retVal =  -PVRSRV_ERROR_INVALID_PARAMS;

		return retVal;
	}

	retVal = MemoryResourceClaimGpuResourcesObject(psMemResource);

	UNLOCK_MEM_RES(psMemResource);

	return retVal;
}

static IMG_INT32 MemoryResourceEnsureGpuMappingIf(PVRSRV_GPU_RESOURCE_OBJECT *pGpuObject,
		IMG_DEV_VIRTADDR* psDevVAddr)
{
	PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource = MemoryResourceFromGpuResourceObject(pGpuObject);
	IMG_INT32 retVal = PVRSRV_OK;

	MEM_RES_FUNC_ENTER(psMemResource);

	if(psMemResource == IMG_NULL)
	{
		retVal =  -PVRSRV_ERROR_INVALID_PARAMS;

		MEM_RES_FUNC_EXIT(psMemResource, retVal, -1);

		return retVal;
	}

	if(pGpuObject->ui32RefCount == 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "Requesting device memory  %p of resource %p with REF COUNT OF 0",
												&psMemResource->sClientMemInfo, psMemResource));
	}

	retVal = MemoryResourceDevMemGpuMapBuffer(psMemResource);
	if(retVal != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "Error ensuring memory %p of resource %p",
								&psMemResource->sClientMemInfo, psMemResource));
	}

	if(psMemResource->sClientMemInfo.sDevVAddr.uiAddr == PVRSRV_BAD_DEVICE_ADDRESS)
	{
		if(psDevVAddr)
			psDevVAddr->uiAddr = PVRSRV_BAD_DEVICE_ADDRESS;

		PVR_DPF((PVR_DBG_ERROR, "Error ensuring memory %p of resource %p, "
			"gpuVAddr 0x%08x",
			&psMemResource->sClientMemInfo, psMemResource, psMemResource->sClientMemInfo.sDevVAddr.uiAddr));
	}
	else
	{
		retVal = psMemResource->sClientMemInfo.uAllocSize;
		if(psDevVAddr)
			*psDevVAddr = psMemResource->sClientMemInfo.sDevVAddr;

		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "ENSURE NEW Resource Address 0x%08x to 0x%08x with size %d for %p",
				psMemResource->sClientMemInfo.sDevVAddr.uiAddr,
				(psMemResource->sClientMemInfo.sDevVAddr.uiAddr + psMemResource->sClientMemInfo.uAllocSize),
				psMemResource->sClientMemInfo.uAllocSize,
				psMemResource));
	}

	if(MemoryResourceDebugIsEnabled(psMemResource))
	{
		PVRSRV_DUMP_MEMINFO(PVR_DBG_WARNING,
			(&psMemResource->sClientMemInfo),
			"ENSURED SINGLE");
		PVR_DPF((PVR_DBG_WARNING, "REF COUNT of %p IS %d",
			&psMemResource->sClientMemInfo,
			psMemResource->sGpuMappingResourceObject.ui32RefCount));

	}

	UNLOCK_MEM_RES(psMemResource);

	MEM_RES_FUNC_EXIT(psMemResource, retVal, pGpuObject->ui32RefCount);

	return retVal;
}

static IMG_INT32 MemoryResourceEnsurePlanesGpuMappingIf(PVRSRV_GPU_RESOURCE_OBJECT *pGpuObject,
		IMG_UINT32 *pasDevPlanesVAddr, IMG_UINT32 *pui32NumPlanes )
{

	PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource = MemoryResourceFromGpuResourceObject(pGpuObject);
	IMG_INT32 retVal = PVRSRV_OK;

	MEM_RES_FUNC_ENTER(psMemResource);

	if(psMemResource == IMG_NULL)
	{
		retVal =  -PVRSRV_ERROR_INVALID_PARAMS;

		MEM_RES_FUNC_EXIT(psMemResource, retVal, -1);

		return retVal;
	}

	if(pGpuObject->ui32RefCount == 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "Requesting device memory  %p of resource %p with REF COUNT OF 0",
												&psMemResource->sClientMemInfo, psMemResource));
	}

	retVal = MemoryResourceDevMemGpuMapBuffer(psMemResource);
	if(retVal != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "Error ensuring memory %p of resource %p",
								&psMemResource->sClientMemInfo, psMemResource));
	}

	if(psMemResource->sClientMemInfo.sDevVAddr.uiAddr == PVRSRV_BAD_DEVICE_ADDRESS)
	{
		if(pui32NumPlanes)
			*pui32NumPlanes = 0;

					// PVRSRV_BAD_DEVICE_ADDRESS;

		PVR_DPF((PVR_DBG_ERROR, "Error ensuring memory %p of resource %p, "
			"gpuVAddr 0x%08x",
			&psMemResource->sClientMemInfo, psMemResource, psMemResource->sClientMemInfo.sDevVAddr.uiAddr));
	}
	else
	{
		IMG_UINT32 ui32Addr = psMemResource->sClientMemInfo.sDevVAddr.uiAddr;
		retVal = psMemResource->sClientMemInfo.uAllocSize;
		pasDevPlanesVAddr[0] = ui32Addr + psMemResource->sClientMemInfo.planeOffsets[0];
		if(pui32NumPlanes && pasDevPlanesVAddr)
		{
			unsigned i;
			for (i = 1;(i < *pui32NumPlanes); i++)
			{
				if((i < PVRSRV_MAX_NUMBER_OF_MM_BUFFER_PLANES) &&
						(psMemResource->sClientMemInfo.planeOffsets[i] != PVRSRV_BAD_DEVICE_ADDRESS))
				{
					pasDevPlanesVAddr[i] = ui32Addr + psMemResource->sClientMemInfo.planeOffsets[i];
				}
				else
				{
					pasDevPlanesVAddr[i] = PVRSRV_BAD_DEVICE_ADDRESS;
				}
			}
		}

		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "ENSURE NEW Resource Address 0 - 0x%08x and 1 - 0x%08x with total size %d for %p",
				psMemResource->sClientMemInfo.sDevVAddr.uiAddr + psMemResource->sClientMemInfo.planeOffsets[0],
				psMemResource->sClientMemInfo.planeOffsets[1] ?
						psMemResource->sClientMemInfo.sDevVAddr.uiAddr + psMemResource->sClientMemInfo.planeOffsets[1]:
						PVRSRV_BAD_DEVICE_ADDRESS,
				psMemResource->sClientMemInfo.uAllocSize,
				psMemResource));
	}

	if(MemoryResourceDebugIsEnabled(psMemResource))
	{
		PVRSRV_DUMP_MEMINFO(PVR_DBG_WARNING,
			(&psMemResource->sClientMemInfo),
			"ENSURED MULTI");
	}

	UNLOCK_MEM_RES(psMemResource);

	MEM_RES_FUNC_EXIT(psMemResource, retVal, pGpuObject->ui32RefCount);

	return retVal;
}

static IMG_INT32 MemoryResourceReleaseGpuResourcesIf(PVRSRV_RESOURCE_OBJECT_BASE *pGpuObject, IMG_BOOL bIsShutdown)
{
	PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource =
			MemoryResourceFromGpuResourceObject((PVRSRV_GPU_RESOURCE_OBJECT *)pGpuObject);
	IMG_INT32 retVal = PVRSRV_OK;
	IMG_INT32 refCount = 0;

	MEM_RES_FUNC_ENTER(psMemResource);

	if(psMemResource == IMG_NULL)
	{
		retVal =  -PVRSRV_ERROR_INVALID_PARAMS;

		MEM_RES_FUNC_EXIT(psMemResource, retVal, -1);

		return retVal;
	}

	if(MemoryResourceDebugIsEnabled(psMemResource))
	{
		PVRSRV_DUMP_MEMINFO(PVR_DBG_WARNING,
			(&psMemResource->sClientMemInfo),
			"RELEASE GPU INTERFACE");
		PVR_DPF((PVR_DBG_WARNING, "REF COUNT of %p BEFORE DEC. IS %d",
			&psMemResource->sClientMemInfo,
			psMemResource->sGpuMappingResourceObject.ui32RefCount));

	}

	if(psMemResource->sGpuMappingResourceObject.ui32RefCount == 1)
	{
		 /* Do not release GPU memory if the reference count of Base object is 1
		This would mean that the entire object would go away anyway */
		if(psMemResource->sMemResourceObjectBase.ui32RefCount > 1)
		{
			/* Do not evict objects that are about to be shutdown,
			 * not having sync object and the eviction is disabled
			 * for the particular type the object belongs to
			 */
			if((psMemResource->sClientMemInfo.psClientSyncInfo) &&
					(bIsShutdown == IMG_FALSE) &&
					MemoryResourceEvictionIsEnabled(psMemResource))
			{
				/* Release GPU mapping */
				if(MemoryResourceDebugIsEnabled(psMemResource))
				{
					PVRSRV_DUMP_MEMINFO(PVR_DBG_WARNING,
							(&psMemResource->sClientMemInfo), "EVICTING");
				}

				retVal = MemoryResourceDevMemGpuReleaseBuffer(psMemResource);

				if(retVal == PVRSRV_OK)
				{
					retVal = psMemResource->sClientMemInfo.uAllocSize;
				}
				else
				{
					PVR_DPF((PVR_DBG_ERROR, "Error evicting memory %p of resource %p - ERROR %d",
							&psMemResource->sClientMemInfo, psMemResource, retVal));
				}
			}
			else
			{
				retVal = PVRSRV_OK;
			}
		}
		else
		{
			if(MemoryResourceDebugIsEnabled(psMemResource))
			{
				PVRSRV_DUMP_MEMINFO(PVR_DBG_WARNING,
					(&psMemResource->sClientMemInfo),
					"WOULD NOT EVICT");
			}
		}

		psMemResource->sGpuMappingResourceObject.ui32RefCount = 0;

	}
	else if(psMemResource->sGpuMappingResourceObject.ui32RefCount > 0)
	{
		psMemResource->sGpuMappingResourceObject.ui32RefCount--;

		if(MemoryResourceDebugIsEnabled(psMemResource))
		{
			PVRSRV_DUMP_MEMINFO(PVR_DBG_WARNING,
				(&psMemResource->sClientMemInfo),
				"RELEASED GPU INTERFACE");
		}

		refCount = psMemResource->sGpuMappingResourceObject.ui32RefCount;
	}

	UNLOCK_MEM_RES(psMemResource);

	if(refCount == 0)
	{
		/* Release the base object from outside the lock */
		MemoryResourceReleaseIf(&psMemResource->sMemResourceObjectBase, IMG_FALSE);
	}

	MEM_RES_FUNC_EXIT(psMemResource, retVal, refCount);

	return retVal;
}

static IMG_BOOL MemoryResourceInUseIf(PVRSRV_GPU_RESOURCE_OBJECT *pGpuObject)
{
	PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource = MemoryResourceFromGpuResourceObject(pGpuObject);
	IMG_BOOL retVal;

	MEM_RES_FUNC_ENTER(psMemResource);

	if(psMemResource == IMG_NULL)
	{
		retVal = IMG_FALSE;

		MEM_RES_FUNC_EXIT(psMemResource, retVal, -1);

		return retVal;
	}

	if( PVRSRVMemoryBufferInUseByGpu(&psMemResource->sClientMemInfo))
		retVal = IMG_TRUE;
	else
		retVal = IMG_FALSE;

	UNLOCK_MEM_RES(psMemResource);

	MEM_RES_FUNC_EXIT(psMemResource, retVal, pGpuObject->ui32RefCount);

	return retVal;
}

static IMG_INT32 MemoryResourceGetInterface(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource,
		PVRSRV_OBJECT_IF_ID interfaceID, IMG_VOID ** ppNewObject);

static IMG_INT32 MemoryResourceGetInterfaceBase(PVRSRV_RESOURCE_OBJECT_BASE *pObject,
		PVRSRV_OBJECT_IF_ID interfaceID, IMG_VOID ** ppNewObject)
{
	PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource = MemoryResourceFromBaseObject(pObject);
	IMG_INT32 retVal;

	if(psMemResource == IMG_NULL)
	{
		retVal =  -PVRSRV_ERROR_INVALID_PARAMS;

		MEM_RES_FUNC_EXIT(psMemResource, retVal, -1);

		return retVal;
	}

	retVal = MemoryResourceGetInterface(psMemResource, interfaceID, ppNewObject);

	UNLOCK_MEM_RES(psMemResource);

	return retVal;
}

static IMG_INT32 MemoryResourceGetInterfaceGpuResources(PVRSRV_RESOURCE_OBJECT_BASE *pGpuObject,
		PVRSRV_OBJECT_IF_ID interfaceID, IMG_VOID ** ppNewObject)
{
	PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource = MemoryResourceFromGpuResourceObject((PVRSRV_GPU_RESOURCE_OBJECT *)pGpuObject);
	IMG_INT32 retVal;

	if(psMemResource == IMG_NULL)
	{
		retVal =  -PVRSRV_ERROR_INVALID_PARAMS;

		MEM_RES_FUNC_EXIT(psMemResource, retVal, -1);

		return retVal;
	}

	retVal = MemoryResourceGetInterface(psMemResource, interfaceID, ppNewObject);

	UNLOCK_MEM_RES(psMemResource);

	return retVal;
}

static const PVRSRV_RESOURCE_OBJECT_BASE_IF sMemoryResourceResourceBaseIf =
{
		MemoryResourceGetInterfaceBase,
		MemoryResourceAddRefIf,
		MemoryResourceReleaseIf,
};

static const PVRSRV_OBJECT_IF_GPU_MAPPING sMemoryResourceGpuMappingIf =
{
		{
				MemoryResourceGetInterfaceGpuResources,
				MemoryResourceClaimGpuResourcesIf,
				MemoryResourceReleaseGpuResourcesIf
		},
		MemoryResourceEnsureGpuMappingIf,
		MemoryResourceEnsurePlanesGpuMappingIf,
		MemoryResourceInUseIf,
};

static IMG_INT32 MemoryResourceGetInterface(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource,
		PVRSRV_OBJECT_IF_ID interfaceID, IMG_VOID ** ppNewObject)
{
	IMG_INT32 retVal = -PVRSRV_ERROR_NOT_SUPPORTED;
	*ppNewObject = IMG_NULL;

	MEM_RES_FUNC_ENTER(psMemResource);

	if(interfaceID == PVRSRV_GPU_RESOURCE_IF_ID)
	{
		if(psMemResource->sGpuMappingResourceObject.pIf == IMG_NULL)
		{
			psMemResource->sGpuMappingResourceObject.pIf = &sMemoryResourceGpuMappingIf;
			psMemResource->sGpuMappingResourceObject.ui32RefCount = 0;
		}

		if(MemoryResourceClaimGpuResourcesObject(psMemResource) >= 0)
		{
			*ppNewObject = &psMemResource->sGpuMappingResourceObject;
			retVal = PVRSRV_OK;
			if(MemoryResourceDebugIsEnabled(psMemResource))
			{
				PVRSRV_DUMP_MEMINFO(PVR_DBG_WARNING,
					(&psMemResource->sClientMemInfo),
					"GOT GPU INTERFACE");
			}
			PVR_DPF((PVR_DBG_EVICT_VERBOSE, "GET INTERFACE Resource %p, base refcount %d SUCCESS",
								psMemResource, psMemResource->sMemResourceObjectBase.ui32RefCount));
		}
	}
	else if(interfaceID == PVRSRV_BASE_IF_ID)
	{
		retVal = MemoryResourceAddRefBaseObject(psMemResource);
		*ppNewObject = &psMemResource->sMemResourceObjectBase;
	}

	MEM_RES_FUNC_EXIT(psMemResource, retVal, psMemResource->sMemResourceObjectBase.ui32RefCount);

	return retVal;
}

static PVRSRV_GPU_MEMORY_RESOURCE_OBJECT* MemoryResourceFromBaseObject(PVRSRV_RESOURCE_OBJECT_BASE *pObject)
{
	PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource;

	if((pObject == IMG_NULL) || (pObject->pIf != &sMemoryResourceResourceBaseIf) )
	{
		PVR_DPF((PVR_DBG_ERROR, "Base Object %p is INVALID", pObject));
		return IMG_NULL;
	}

	psMemResource =
			(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT*)((IMG_UINTPTR_T)pObject -
					offsetof(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT, sMemResourceObjectBase));

	if( (psMemResource->validObjectToken != PVRSRV_RESOURCE_OBJECT_VALID_TOKEN) ||
			(psMemResource->sMemResourceObjectBase.ui32RefCount == 0) ||
			(psMemResource->eState == PVRSRV_GPU_MEMORY_INVALID_STATE))
	{
		PVR_DPF((PVR_DBG_ERROR, "Resource %p "
				"Call With REFCOUNT of %d and meminfo state %d",
				psMemResource,
				psMemResource->sMemResourceObjectBase.ui32RefCount,
				psMemResource->eState));
		return IMG_NULL;
	}

	LOCK_MEM_RES(psMemResource);

	return psMemResource;
}

static PVRSRV_GPU_MEMORY_RESOURCE_OBJECT* MemoryResourceFromGpuResourceObject(PVRSRV_GPU_RESOURCE_OBJECT *pGpuObject)
{
	PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource;

	if((pGpuObject == IMG_NULL) || (pGpuObject->pIf != &sMemoryResourceGpuMappingIf) )
	{
		PVR_DPF((PVR_DBG_ERROR, "GPU Object %p is INVALID", pGpuObject));
		return IMG_NULL;
	}

	psMemResource = (PVRSRV_GPU_MEMORY_RESOURCE_OBJECT*)((IMG_UINTPTR_T)pGpuObject -
			offsetof(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT, sGpuMappingResourceObject));

	if( (psMemResource->validObjectToken != PVRSRV_RESOURCE_OBJECT_VALID_TOKEN) ||
			(psMemResource->sMemResourceObjectBase.ui32RefCount == 0) ||
			(psMemResource->eState == PVRSRV_GPU_MEMORY_INVALID_STATE))
	{
		PVR_DPF((PVR_DBG_ERROR, "Resource %p "
				"Call With REFCOUNT of %d and meminfo state %d",
				psMemResource,
				psMemResource->sMemResourceObjectBase.ui32RefCount,
				psMemResource->eState));
		return IMG_NULL;
	}

	LOCK_MEM_RES(psMemResource);

	return psMemResource;
}


/***********************************************************************************
 Function Name      : PVRSRVGetInterfaceHwAddress
 Inputs             :   psMemResourceObjectBase , ppsGpuMappingResourceObject, psDevVAddr
 Outputs            : new HW address if the base interface can supply one
 Returns            : -
 Description        : Obtain GPU interface and GPU HW address from base interface
************************************************************************************/
IMG_EXPORT IMG_INT32 MemoryResourceGetInterfaceHwAddress(
		PVRSRV_RESOURCE_OBJECT_BASE *psMemResourceObjectBase,
		PVRSRV_GPU_RESOURCE_OBJECT ** ppsGpuMappingResourceObject,
		IMG_DEV_VIRTADDR *psDevVAddr)
{
	PVRSRV_GPU_RESOURCE_OBJECT* psGpuMappingResourceObject = IMG_NULL;
	IMG_INT32 retVal;

	if(psDevVAddr)
		psDevVAddr->uiAddr = PVRSRV_BAD_DEVICE_ADDRESS;

	if(!psMemResourceObjectBase)
		PVRSRV_SwapGpuMemoryResourceInterface(ppsGpuMappingResourceObject, IMG_NULL);

	if(!psMemResourceObjectBase || !ppsGpuMappingResourceObject)
		return PVRSRV_ERROR_INVALID_PARAMS;

	/* Get GPU Mapping interface */
	retVal = PVRSRV_GetInterface(psMemResourceObjectBase,
						PVRSRV_GPU_RESOURCE_IF_ID,
						(IMG_VOID **)&psGpuMappingResourceObject);
	if(retVal != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s Interface %p failed to supply GPU interface - "
				"PVRSRV_GetInterface() error: %d, base ref count %d, if %p",
				__FUNCTION__, psMemResourceObjectBase, retVal,
				psMemResourceObjectBase ?
						psMemResourceObjectBase->ui32RefCount : 0,
				psMemResourceObjectBase ?
						psMemResourceObjectBase->pIf : IMG_NULL));
	}

	if(psGpuMappingResourceObject && psDevVAddr)
	{
		retVal = PVRSRV_EnsureGpuMapping(psGpuMappingResourceObject,
				psDevVAddr);
		if(retVal < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s Interface %p from %p  failed to ensure memory - "
				"PVRSRV_EnsureGpuMapping() error: %d",
				__FUNCTION__, psGpuMappingResourceObject, psMemResourceObjectBase, retVal));
		}
	}

	PVRSRV_SwapGpuMemoryResourceInterface(ppsGpuMappingResourceObject, psGpuMappingResourceObject);

	return PVRSRV_OK;
}

/**********************************************************************************************/
/************************ Resource Memory Management Functions ***************************/
/**********************************************************************************************/

/* Guard Pattern */
#define PVRSRV_GPU_GUARD_PATTERN  0xDEADBEAF

#define LOCK_MEM_MGM_RES(connection) \
	{ \
		PVR_UNREFERENCED_PARAMETER(connection); \
		PVRSRVLockProcessGlobalMutex(); \
	}

#define UNLOCK_MEM_MGM_RES(connection) \
	{ \
		PVR_UNREFERENCED_PARAMETER(connection); \
		PVRSRVUnlockProcessGlobalMutex(); \
	}

#define MEM_MGM_RES_FUNC_ENTER(object) \
	PVR_DPF((PVR_DBG_CALLTRACE, "Enter %s with object %p", __FUNCTION__, (object)))

#define MEM_MGR_RES_FUNC_EXIT(object, ret_val, ref_count) \
	PVR_DPF((PVR_DBG_CALLTRACE, "Exit %s with object %p, ret %d, ref_count %d\n", __FUNCTION__, (object), (ret_val), (ref_count)))

#define MEM_MGM_RES_LIST_FUNC_ENTER(pBuffList) \
	PVR_DPF((PVR_DBG_CALLTRACE, "Enter %s with buffer list %p and memory size %d",\
			__FUNCTION__, (pBuffList),  (pBuffList) ? (pBuffList->ui32BuffersSize) : 0))

#define MEM_MGR_RES_LIST_FUNC_EXIT(pBuffList, numObjects) \
	PVR_DPF((PVR_DBG_CALLTRACE, "Exit %s with buffer list %p and memory size %d - %d processed",\
			__FUNCTION__, (pBuffList),  ((pBuffList) ? (pBuffList->ui32BuffersSize) : 0), numObjects))

#ifdef ENABLE_PVRSRV_GPU_GUARD_AREA

/* This function must be invoked with locked mutex */
static INLINE
PVRSRV_GPU_MEMORY_RESOURCE_OBJECT* MemoryResourceMemoryObjectFromMeminfo(
		PVRSRV_CLIENT_MEM_INFO *psMemInfo)
{
	if(psMemInfo == IMG_NULL)
		return IMG_NULL;

	if(psMemInfo->ui32Flags & PVRSRV_HAP_GPU_PAGEABLE)
	{
		return (PVRSRV_GPU_MEMORY_RESOURCE_OBJECT*)psMemInfo;
	}
	else
	{
		return IMG_NULL;
	}
}

static IMG_UINT32 MemoryResourceSetGuardArea(IMG_UINT32 * ui32Area, IMG_UINT32 ui32SizeWords)
{
	IMG_UINT32 count;

	for(count = 0; 	count < ui32SizeWords; count++)
	{
		ui32Area[count] = PVRSRV_GPU_GUARD_PATTERN;
	}

	return PVRSRV_GPU_GUARD_PATTERN;
}
#endif

static
IMG_INT32 MemoryResourceGetFreeNodesPendingList(PVRSRV_CONNECTION *psConnection, PVRSRV_BUFFER_LIST *psList,
		PVRSRV_BUFFER_LIST *psOutList, PVRSRV_GPU_MEMORY_STATE eNewState,
		IMG_UINT32 maxNodes, IMG_UINT32 maxSize)
{
	PVRSRV_GPU_MEMORY_RESOURCE_OBJECT * psMemResource;
	PVRSRV_GPU_MEMORY_RESOURCE_OBJECT * psNextMemResource;
	IMG_UINT32 numObjectsProcessed = 0;

	MEM_MGM_RES_LIST_FUNC_ENTER(psList);

	LOCK_MEM_MGM_RES(psConnection);

	list_for_each_entry_safe(psMemResource, psNextMemResource, &psList->sList, sListNode)
	{
		if( !PVRSRVMemoryBufferInUseByGpu(&psMemResource->sClientMemInfo))
		{
			IMG_UINT32 nodeSize = psMemResource->sClientMemInfo.uAllocSize;

			/* Move this node to out list */
			psMemResource->eState = eNewState;
			list_move_tail(&psMemResource->sListNode, &psOutList->sList);
			psList->ui32BuffersSize -= nodeSize;
			psOutList->ui32BuffersSize += nodeSize;

			numObjectsProcessed++;

			if(maxNodes)
				maxNodes--;

			if(!maxNodes)
				break; /* Enough nodes already */

			if(maxSize)
			{
				if(maxSize < nodeSize)
					maxSize = 0;
				else
					maxSize -= nodeSize;
			}

			if(!maxSize)
				break; /* Enough nodes  already */
		}
	}

	UNLOCK_MEM_MGM_RES(psConnection);

	MEM_MGR_RES_LIST_FUNC_EXIT(psOutList, numObjectsProcessed);

	return numObjectsProcessed;
}

/* This function is thread save by nature and does not need locking,
 * since the list it uses must be private at the stack
 * Nodes are removed from the list before processing */
static
IMG_INT32 MemoryResourceProcessMemoryNodesList(PVRSRV_BUFFER_LIST *psList,
		IMG_INT32 (*pProcNodeFunc)(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource))
{
	PVRSRV_GPU_MEMORY_RESOURCE_OBJECT * psMemResource;
	PVRSRV_GPU_MEMORY_RESOURCE_OBJECT * psNextMemResource;
	IMG_UINT32 numObjectsProcessed = 0;

	MEM_MGM_RES_LIST_FUNC_ENTER(psList);

	if(!pProcNodeFunc)
		return -PVRSRV_ERROR_INVALID_PARAMS;

	list_for_each_entry_safe(psMemResource, psNextMemResource, &psList->sList, sListNode)
	{
		if( !PVRSRVMemoryBufferInUseByGpu(&psMemResource->sClientMemInfo))
		{
			IMG_UINT32 nodeSize = psMemResource->sClientMemInfo.uAllocSize;
			IMG_UINT32 retVal;

			list_del_init(&psMemResource->sListNode);
			retVal = pProcNodeFunc(psMemResource);

			numObjectsProcessed++;

			/* Reduce the memory size in the list */
			psList->ui32BuffersSize -= nodeSize;
		}
	}

	MEM_MGR_RES_LIST_FUNC_EXIT(psList, numObjectsProcessed);

	return numObjectsProcessed;
}

/*!
 ******************************************************************************

 @Function	MemoryResourceDevMemFreeBuffer

 @Description	Frees PVRAllocDeviceMem allocation (including the MemInfo)

 @Input		psMemResource

 @Return	PVRSRV_ERROR

 ******************************************************************************/
static
IMG_INT32 MemoryResourceDevMemFreeBuffer(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource)
{
	IMG_INT32 retVal = PVRSRV_OK;

	MEM_MGM_RES_FUNC_ENTER(psMemResource);

	PVRSRVDestroyMutex(psMemResource->hMutex);
	psMemResource->hMutex = IMG_NULL;

	if(MemoryResourceDebugIsEnabled(psMemResource))
	{
		PVRSRV_DUMP_MEMINFO(PVR_DBG_WARNING,
			(&psMemResource->sClientMemInfo),
			"FREEING BUFFER");
	}

	/* Always free device memory unlocked */

	if(psMemResource->ui32Hints & PVRSRV_GPU_MEMORY_RESOURCE_OBJECT_ALLOCATED)
	{
		IMG_INT i32ExportedFd = psMemResource->i32ExportedFd;
		retVal = PVRSRVFreeDeviceMem(psMemResource->psDevData, &psMemResource->sClientMemInfo);

		if(i32ExportedFd > 0)
		{
			PVRSRVCloseExportedDeviceMemHanle(psMemResource->psDevData, i32ExportedFd);
		}
	}
	else if(psMemResource->ui32Hints & PVRSRV_GPU_MEMORY_RESOURCE_OBJECT_MAPPED)
	{
		retVal = PVRSRVUnmapDeviceMemory(psMemResource->psDevData, &psMemResource->sClientMemInfo);
	}
	if(retVal != PVRSRV_OK)
		retVal = -retVal;

	MEM_MGR_RES_FUNC_EXIT(psMemResource, retVal, 0);

	return retVal;
}


/*!
 ******************************************************************************

 @Function	MemoryResourceInitializeDevMemResourceObject

 @Description	Initializes PVRSRV_GPU_MEMORY_RESOURCE_OBJECT

 @Input

 @Return	PVRSRV_ERROR

 ******************************************************************************/
static
IMG_INT32 MemoryResourceInitializeDevMemResourceObject(PVRSRV_DEV_DATA *psDevData,
	   PVRSRV_CLIENT_MEM_INFO *psMemInfo, IMG_INT *pi32ExportedFd,
	   IMG_UINT32	ui32Hints,
	   IMG_UINT64 	uiSubSystem,
	   PVRSRV_RESOURCE_OBJECT_BASE **ppsMemResourceObjectBase)
{
	IMG_INT32 retVal = PVRSRV_OK;
	PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource;

	PVR_UNREFERENCED_PARAMETER(uiSubSystem);

	psMemResource = MemoryResourceMemoryObjectFromMeminfo(psMemInfo);
	if(psMemResource == IMG_NULL)
	{
		*ppsMemResourceObjectBase = IMG_NULL;
		return retVal; /* No support for eviction and buffer interface */
	}

	MEM_MGM_RES_FUNC_ENTER(psMemResource);

	PVRSRVCreateMutex( &psMemResource->hMutex );

#ifdef ENABLE_PVRSRV_GPU_GUARD_AREA
	{
		MemoryResourceSetGuardArea(psMemResource->guardBefore.guard,
				(sizeof(psMemResource->guardBefore.guard)/sizeof(psMemResource->guardBefore.guard[0])));

		MemoryResourceSetGuardArea(psMemResource->guardAfter.guard,
				(sizeof(psMemResource->guardAfter.guard)/sizeof(psMemResource->guardAfter.guard[0])));
	}
#endif

	psMemResource->ui32Hints = ui32Hints;
	if(pi32ExportedFd)
		psMemResource->i32ExportedFd = *pi32ExportedFd;
	else
		psMemResource->i32ExportedFd = -1;
	psMemResource->psDevData = psDevData;
	psMemResource->sMemResourceObjectBase.pIf = &sMemoryResourceResourceBaseIf;
	/* The reference of the object should be 1 at this point of time */
	psMemResource->sMemResourceObjectBase.ui32RefCount = 1;
	psMemResource->sGpuMappingResourceObject.pIf = &sMemoryResourceGpuMappingIf;

	/* Required size of the buffer in pixels */
	psMemResource->ui32Size = 0;

	/* Width of the buffer in pixels */
	psMemResource->ui32Width = 0;

	/* Stride of the buffer in pixels */
	psMemResource->ui32Stride = 0;

	/* Height of the buffer in pixels */
	psMemResource->ui32Height = 0;

	/* Bits per pixel of the buffer */
	psMemResource->ui32Bpp = 0;

	/* Make the object valid */
	psMemResource->validObjectToken = PVRSRV_RESOURCE_OBJECT_VALID_TOKEN;

	*ppsMemResourceObjectBase = &psMemResource->sMemResourceObjectBase;

	list_init(&psMemResource->sListNode);
	psMemResource->eState = PVRSRV_GPU_MEMORY_INVALID_STATE;

	/* Put it in the correct list */
	{
		PVRSRV_CONNECTION *psConnection = (PVRSRV_CONNECTION *)psMemResource->psDevData->psConnection;
		/* Lock */
		LOCK_MEM_MGM_RES(psConnection);
		if((psMemResource->sClientMemInfo.ui32Flags & PVRSRV_HAP_NO_GPU_VIRTUAL_ON_ALLOC) &&
				(psMemResource->sClientMemInfo.sDevVAddr.uiAddr == PVRSRV_BAD_DEVICE_ADDRESS))
		{
			psMemResource->eState = PVRSRV_GPU_MEMORY_EVICTED_STATE;
			list_move_tail(&psMemResource->sListNode, &psConnection->sBuffEvictedList.sList);
			psConnection->sBuffEvictedList.ui32BuffersSize += psMemResource->sClientMemInfo.uAllocSize;
		}
		else
		{
			/* If the memory resource is placed in the Eviction Pending list
			 * it would be automatically evicted  */
			if(0)
			{
				psMemResource->eState = PVRSRV_GPU_MEMORY_PENDING_EVICTION_STATE;
				list_move_tail(&psMemResource->sListNode, &psConnection->sBuffEvictionPendingList.sList);
				psConnection->sBuffEvictionPendingList.ui32BuffersSize += psMemResource->sClientMemInfo.uAllocSize;
			}
			else
			{
				/* Lock */
				psMemResource->eState = PVRSRV_GPU_MEMORY_ACTIVE_STATE;
				list_move_tail(&psMemResource->sListNode, &psConnection->sBuffActiveList.sList);
				psConnection->sBuffActiveList.ui32BuffersSize += psMemResource->sClientMemInfo.uAllocSize;


				PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Added  %p  with size %d to the active list with new size of %d",
						psMemResource,  psMemResource->sClientMemInfo.uAllocSize,
						psConnection->sBuffActiveList.ui32BuffersSize));
			}
		}
		/* UnLock */
		UNLOCK_MEM_MGM_RES(psConnection);
	}


	PVR_DPF((PVR_DBG_EVICT_VERBOSE, "INIT memResource %p (%p), "
			"RefCount %d (%p), Mapping RefCount %d (%p), token 0x%08x",
			psMemResource, psMemInfo,
			psMemResource->sMemResourceObjectBase.ui32RefCount,
			psMemResource->sMemResourceObjectBase.pIf,
			psMemResource->sGpuMappingResourceObject.ui32RefCount,
			psMemResource->sGpuMappingResourceObject.pIf,
			psMemResource->validObjectToken));

	if(MemoryResourceDebugIsEnabled(psMemResource))
	{
		if(ui32Hints & PVRSRV_GPU_MEMORY_RESOURCE_OBJECT_ALLOCATED)
		{
			PVRSRV_DUMP_MEMINFO(PVR_DBG_WARNING,
				(&psMemResource->sClientMemInfo),
				"ALLOCATED BUFFER");
		}
		else if(ui32Hints & PVRSRV_GPU_MEMORY_RESOURCE_OBJECT_MAPPED)
		{
			PVRSRV_DUMP_MEMINFO(PVR_DBG_WARNING,
				(&psMemResource->sClientMemInfo),
				"IMPORTED BUFFER");
		}
	}

	MEM_MGR_RES_FUNC_EXIT(psMemResource, retVal, psMemResource->sMemResourceObjectBase.ui32RefCount);

	return PVRSRV_OK;
}

/*!
 ******************************************************************************

 @Function	MemoryResourceDevMemAllocBuffer

 @Description	Allocates Device memory and returns buffer interface.
                The function will also export the memory,
                if iExportedFd is not NULL

 @Input

 @Return	PVRSRV_ERROR, ppsMemInfo, ppsMemResourceObjectBase

 ******************************************************************************/
IMG_EXPORT
IMG_INT32 MemoryResourceDevMemAllocBuffer(PVRSRV_DEV_DATA *psDevData,
#if defined (SUPPORT_SID_INTERFACE)
	   IMG_SID	 hDevMemHeap,
#else
	   IMG_HANDLE hDevMemHeap,
#endif
	   IMG_UINT32 ui32Attribs,
	   IMG_SIZE_T uSize,
	   IMG_SIZE_T uAlignment,
	   IMG_PVOID pvPrivData,
	   IMG_UINT32 ui32PrivDataLength,
	   PVRSRV_CLIENT_MEM_INFO **ppsMemInfo,
	   IMG_INT	*iExportedFd,
	   IMG_UINT32	ui32Hints,
	   IMG_UINT64 	uiSubSystem,
	   PVRSRV_RESOURCE_OBJECT_BASE **ppsMemResourceObjectBase)
{
	IMG_INT32 retVal = PVRSRV_OK;

	if(ui32Attribs & PVRSRV_HAP_GPU_PAGEABLE)
	{
		uAlignment = getpagesize();
		uSize = ALIGN(uSize, uAlignment);
	}

	retVal = PVRSRVAllocDeviceMem2(psDevData, hDevMemHeap,
				ui32Attribs, uSize, uAlignment,
				pvPrivData, ui32PrivDataLength,
				ppsMemInfo);

	if(retVal != PVRSRV_OK)
	{
		*ppsMemInfo  = IMG_NULL;
		*ppsMemResourceObjectBase = IMG_NULL;
		return -retVal;
	}

	(*ppsMemInfo)->uiSubSystem = uiSubSystem;
	(*ppsMemInfo)->ui32ClientFlags = ui32Hints;

	if(iExportedFd)
	{
		retVal = PVRSRVExportDeviceMem2(psDevData, *ppsMemInfo, iExportedFd);
		if(retVal != PVRSRV_OK)
		{
			PVRSRVFreeDeviceMem(psDevData, *ppsMemInfo);
			*ppsMemInfo  = IMG_NULL;
			*ppsMemResourceObjectBase = IMG_NULL;
			return -retVal;
		}
	}

	if(ppsMemResourceObjectBase)
	{
		ui32Hints &= ~PVRSRV_GPU_MEMORY_RESOURCE_OBJECT_TYPE_MASK;
		ui32Hints |= PVRSRV_GPU_MEMORY_RESOURCE_OBJECT_ALLOCATED;

		return MemoryResourceInitializeDevMemResourceObject(
				psDevData, *ppsMemInfo, iExportedFd,
				ui32Hints, uiSubSystem,
				ppsMemResourceObjectBase);
	}
	else
	{
		return retVal;
	}
}

/*!
 ******************************************************************************

 @Function	MemoryResourceDevMemImportBuffer

 @Description	Imports Device memory and returns buffer interface

 @Input

 @Return	PVRSRV_ERROR, ppsMemInfo, ppsMemResourceObjectBase

 ******************************************************************************/
IMG_EXPORT
IMG_INT32 MemoryResourceDevMemImportBuffer(PVRSRV_DEV_DATA *psDevData,
       IMG_INT iFd,
#if defined (SUPPORT_SID_INTERFACE)
	   IMG_SID	 hDevMemHeap,
#else
	   IMG_HANDLE hDevMemHeap,
#endif
	   PVRSRV_CLIENT_MEM_INFO **ppsMemInfo,
	   IMG_UINT32	ui32Hints,
	   IMG_UINT64 	uiSubSystem,
	   PVRSRV_RESOURCE_OBJECT_BASE **ppsMemResourceObjectBase)
{
	IMG_INT32 retVal = PVRSRV_OK;

	retVal = PVRSRVMapDeviceMemory2(psDevData,
			iFd, hDevMemHeap, ppsMemInfo);

	if(retVal != PVRSRV_OK)
		return -retVal;

	(*ppsMemInfo)->uiSubSystem = uiSubSystem;
	(*ppsMemInfo)->ui32ClientFlags = ui32Hints;

	if(ppsMemResourceObjectBase &&
			((*ppsMemInfo)->ui32Flags & PVRSRV_HAP_GPU_PAGEABLE))
	{
		ui32Hints &= ~PVRSRV_GPU_MEMORY_RESOURCE_OBJECT_TYPE_MASK;
		ui32Hints |= PVRSRV_GPU_MEMORY_RESOURCE_OBJECT_MAPPED;

		return MemoryResourceInitializeDevMemResourceObject(
				psDevData, *ppsMemInfo, IMG_NULL,
				ui32Hints, uiSubSystem,
				ppsMemResourceObjectBase);
	}
	else
	{
		if(ppsMemResourceObjectBase)
			*ppsMemResourceObjectBase = IMG_NULL;

		return retVal;
	}
}

#ifdef ENABLE_PVRSRV_GPU_GUARD_AREA
static IMG_BOOL MemoryResourceCheckGuardArea(IMG_UINT32 * ui32Area, IMG_UINT32 ui32SizeWords)
{
	IMG_BOOL valid = IMG_TRUE;
	IMG_UINT32 count;

	for(count = 0; 	count < ui32SizeWords; count++)
	{
		if(ui32Area[count] != PVRSRV_GPU_GUARD_PATTERN)
		{
			PVR_DPF((PVR_DBG_ERROR, "BAD GUARD PATTERN AT %04d -> 0x%08x AND SHOULD BE 0x%08x",
					count, ui32Area[count], PVRSRV_GPU_GUARD_PATTERN));
			valid = IMG_FALSE;
		}
	}

	return valid;
}
#endif

/*!
*******************************************************************************

 @Function	Name MemoryResourceCheckGuardAreaAndInvalidateObject

 @Description

 @Input        psMemInfo :

 @Return       IMG_TRUE if meminfo was successfully verified

******************************************************************************/
IMG_EXPORT
IMG_BOOL MemoryResourceCheckGuardAreaAndInvalidateObject(
		PVRSRV_CLIENT_MEM_INFO *psMemInfo)
{
	IMG_BOOL valid = IMG_TRUE;
	PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource =
			MemoryResourceMemoryObjectFromMeminfo(psMemInfo);

	if(!psMemResource)
		return -1;

	PVR_DPF((PVR_DBG_EVICT_VERBOSE, "FREE memResource %p (%p), "
			"RefCount %d (%p), Mapping RefCount %d (%p), token 0x%08x",
			psMemResource, &psMemResource->sClientMemInfo,
			psMemResource->sMemResourceObjectBase.ui32RefCount,
			psMemResource->sMemResourceObjectBase.pIf,
			psMemResource->sGpuMappingResourceObject.ui32RefCount,
			psMemResource->sGpuMappingResourceObject.pIf,
			psMemResource->validObjectToken));

#ifdef ENABLE_PVRSRV_GPU_GUARD_AREA
	{
		valid = MemoryResourceCheckGuardArea(psMemResource->guardBefore.guard,
				(sizeof(psMemResource->guardBefore.guard)/sizeof(psMemResource->guardBefore.guard[0])));

		if(valid == IMG_FALSE)
		{
			PVR_DPF((PVR_DBG_ERROR, "BAD GUARD AREA BEFORE FOR memResource %p",
					psMemResource));
		}

		valid = MemoryResourceCheckGuardArea(psMemResource->guardAfter.guard,
				(sizeof(psMemResource->guardAfter.guard)/sizeof(psMemResource->guardAfter.guard[0])));

		if(valid == IMG_FALSE)
		{
			PVR_DPF((PVR_DBG_ERROR, "BAD GUARD AREA AFTER FOR memResource %p",
					psMemResource));
		}
	}
#endif

	psMemResource->sMemResourceObjectBase.pIf = IMG_NULL;
	psMemResource->sMemResourceObjectBase.ui32RefCount = 0;

	psMemResource->sGpuMappingResourceObject.pIf = IMG_NULL;
	psMemResource->sGpuMappingResourceObject.ui32RefCount = 0;

	psMemResource->validObjectToken = (IMG_UINT32)-1;
	psMemResource->eState = PVRSRV_GPU_MEMORY_PENDING_FREE_STATE;

	return valid;
}

/*!
 ******************************************************************************

 @Function	MemoryResourceDevMemReleaseBuffer

 @Description	Frees PVRAllocDeviceMem allocation (including the MemInfo)
                        This function must be invoked with unlocked mutex
 @Input		psMemResource

 @Return	PVRSRV_ERROR

 ******************************************************************************/
static
IMG_INT32 MemoryResourceDevMemReleaseBuffer(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource)
{
	IMG_INT32 retVal = PVRSRV_OK;
	PVRSRV_CONNECTION *psConnection =
			(PVRSRV_CONNECTION *)psMemResource->psDevData->psConnection;

	MEM_MGM_RES_FUNC_ENTER(psMemResource);

	/* Put the hot buffers for deletion at the back of the list,
	 * to be able to give the GPU time to process it.
	 */

	/* Lock */
	LOCK_MEM_MGM_RES(psConnection);

	switch(psMemResource->eState)
	{
		case PVRSRV_GPU_MEMORY_ACTIVE_STATE:
			/* It is in the Active list */
			list_del_init(&psMemResource->sListNode);
			psConnection->sBuffActiveList.ui32BuffersSize -= psMemResource->sClientMemInfo.uAllocSize;
			break;
		case PVRSRV_GPU_MEMORY_PENDING_EVICTION_STATE:
			/* It is in the Eviction Pending list */
			list_del_init(&psMemResource->sListNode);
			psConnection->sBuffEvictionPendingList.ui32BuffersSize -= psMemResource->sClientMemInfo.uAllocSize;
			break;
		case PVRSRV_GPU_MEMORY_EVICTED_STATE:
			/* It is in the Evicted list - this seems suspicious */
			list_del_init(&psMemResource->sListNode);
			psConnection->sBuffEvictedList.ui32BuffersSize -= psMemResource->sClientMemInfo.uAllocSize;
			break;
		default:
			PVR_DPF((PVR_DBG_ERROR, "Invalid state %d of resource %p",
					psMemResource->eState, psMemResource));
			/* This is a major error - should never happen */
			retVal = -PVRSRV_ERROR_INVALID_CONTEXT;
			break;
	}

	/* UnLock */
	UNLOCK_MEM_MGM_RES(psConnection);

	if(retVal == PVRSRV_OK)
	{
		/* Buffer is busy - put it on the evition pending list */
		if( PVRSRVMemoryBufferInUseByGpu(&psMemResource->sClientMemInfo))
		{
			/* Put the hot buffers for deletion at the back of the list,
			 * to be able to give the GPU time to process it.
			 */

			PVR_DPF((PVR_DBG_EVICT_VERBOSE, "DEFER FREE BUFFER Resource %p Address 0x%08x to 0x%08x with size %d for %p",
					psMemResource, psMemResource->sClientMemInfo.sDevVAddr.uiAddr,
					(psMemResource->sClientMemInfo.sDevVAddr.uiAddr + psMemResource->sClientMemInfo.uAllocSize),
					psMemResource->sClientMemInfo.uAllocSize,
					psMemResource));

			/* Lock */
			LOCK_MEM_MGM_RES(psConnection);

			psMemResource->eState = PVRSRV_GPU_MEMORY_PENDING_FREE_STATE;
			list_move_tail(&psMemResource->sListNode, &psConnection->sBuffFreePendingList.sList);
			psConnection->sBuffFreePendingList.ui32BuffersSize += psMemResource->sClientMemInfo.uAllocSize;

			/* UnLock */
			UNLOCK_MEM_MGM_RES(psConnection);
		}
		else
		{
			retVal = MemoryResourceDevMemFreeBuffer(psMemResource);
		}
	}

	/* Lets process a few pending nodes */
	MemoryResourceReclaimMemory(psConnection, IMG_FALSE);

	MEM_MGR_RES_FUNC_EXIT(psMemResource, retVal, 0);

	return retVal;
}

/* This function must be invoked with locked memory object mutex */
static
IMG_INT32 MemoryResourceDevMemGpuMapBuffer(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource)
{
	IMG_UINT32 retVal = PVRSRV_OK;
	PVRSRV_CONNECTION *psConnection = (PVRSRV_CONNECTION *)psMemResource->psDevData->psConnection;

	MEM_MGM_RES_FUNC_ENTER(psMemResource);

	/* Lock */
	LOCK_MEM_MGM_RES(psConnection);

	switch(psMemResource->eState)
	{
		case PVRSRV_GPU_MEMORY_ACTIVE_STATE:
			/* It is already in Active state - do nothing here  */
			break;
		case PVRSRV_GPU_MEMORY_PENDING_EVICTION_STATE:
			/* Remove from Eviction Pending list to make sure the list
			 * processing function do not try to evict in the middle */
			list_del_init(&psMemResource->sListNode);
			psConnection->sBuffEvictionPendingList.ui32BuffersSize -= psMemResource->sClientMemInfo.uAllocSize;
			break;
		case PVRSRV_GPU_MEMORY_EVICTED_STATE:
			/* Do not remove resource yet from the Evicted list */
			break;
		default: /* Everything else is invalid for this function*/
			PVR_DPF((PVR_DBG_ERROR, "Invalid state %d of resource %p",
					psMemResource->eState, psMemResource));
			/* This is a major error - should never happen */
			retVal = -PVRSRV_ERROR_INVALID_CONTEXT;
			break;
	}
	/* Unlock */
	UNLOCK_MEM_MGM_RES(psConnection);

	if(psMemResource->eState == PVRSRV_GPU_MEMORY_EVICTED_STATE)
	{
		IMG_UINT32 ui32StatusFlags = 0;

		if(psMemResource->sClientMemInfo.sDevVAddr.uiAddr != PVRSRV_BAD_DEVICE_ADDRESS)
		{
			PVR_DPF((PVR_DBG_WARNING, "Resource %p in EVICTED state when address is valid 0x%08x",
				psMemResource, psMemResource->sClientMemInfo.sDevVAddr.uiAddr));
		}

		retVal = PVRSRVManageDevMem(psMemResource->psDevData,
			PVRSRV_MULTI_MANAGE_DEV_MEM_RQST_MAP,
			&psMemResource->sClientMemInfo, &ui32StatusFlags);

		if(MemoryResourceDebugIsEnabled(psMemResource))
		{
			PVRSRV_DUMP_MEMINFO(PVR_DBG_WARNING,
				(&psMemResource->sClientMemInfo),
				"GPU MAPPED BUFFER");
		}

		if(retVal != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to map  resource %p with state %d, error %d",
				psMemResource, psMemResource->eState, retVal));
			/* Couldn't get GPU mapping - keep the resource in the evicted list */
		}
	}

	if((psMemResource->eState != PVRSRV_GPU_MEMORY_ACTIVE_STATE)
			&& (retVal == PVRSRV_OK))
	{
		/* Lock */
		LOCK_MEM_MGM_RES(psConnection);
		if(psMemResource->eState == PVRSRV_GPU_MEMORY_EVICTED_STATE)
			psConnection->sBuffEvictedList.ui32BuffersSize -= psMemResource->sClientMemInfo.uAllocSize;
		psMemResource->eState = PVRSRV_GPU_MEMORY_ACTIVE_STATE;
		list_move_tail(&psMemResource->sListNode, &psConnection->sBuffActiveList.sList);
		psConnection->sBuffActiveList.ui32BuffersSize += psMemResource->sClientMemInfo.uAllocSize;
		/* Unlock */
		UNLOCK_MEM_MGM_RES(psConnection);
	}

	MEM_MGR_RES_FUNC_EXIT(psMemResource, retVal,  psMemResource->sGpuMappingResourceObject.ui32RefCount);

	return retVal;
}

/* This function must be invoked with locked mutex */
static
IMG_INT32 MemoryResourceDevMemGpuUnmapBuffer(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource)
{
	PVRSRV_CONNECTION *psConnection = (PVRSRV_CONNECTION *)psMemResource->psDevData->psConnection;
	IMG_UINT32 retVal = PVRSRV_OK;
	IMG_UINT32 ui32StatusFlags = 0;

	MEM_MGM_RES_FUNC_ENTER(psMemResource);

	if(psMemResource->sClientMemInfo.sDevVAddr.uiAddr != PVRSRV_BAD_DEVICE_ADDRESS)
	{
		if(MemoryResourceDebugIsEnabled(psMemResource))
		{
			PVRSRV_DUMP_MEMINFO(PVR_DBG_WARNING,
				(&psMemResource->sClientMemInfo),
				"GPU UN-MAPPING BUFFER");
		}

		retVal = PVRSRVManageDevMem(psMemResource->psDevData,
				PVRSRV_MULTI_MANAGE_DEV_MEM_RQST_UNMAP,
				&psMemResource->sClientMemInfo, &ui32StatusFlags);
	}
	else
	{
		PVR_DPF((PVR_DBG_WARNING, "Resource %p in NON-EVICTED state when address is invalid 0x%08x",
						psMemResource, psMemResource->sClientMemInfo.sDevVAddr.uiAddr));
	}

	/* Lock */
	LOCK_MEM_MGM_RES(psConnection);

	if(retVal == PVRSRV_OK)
	{
		psMemResource->eState = PVRSRV_GPU_MEMORY_EVICTED_STATE;
		list_move_tail(&psMemResource->sListNode, &psConnection->sBuffEvictedList.sList);
		psConnection->sBuffEvictedList.ui32BuffersSize += psMemResource->sClientMemInfo.uAllocSize;
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to unmap  resource %p with state %d, error %d",
			psMemResource, psMemResource->eState, retVal));

		psMemResource->eState = PVRSRV_GPU_MEMORY_PENDING_EVICTION_STATE;
		list_move_tail(&psMemResource->sListNode, &psConnection->sBuffEvictionPendingList.sList);
		psConnection->sBuffEvictionPendingList.ui32BuffersSize += psMemResource->sClientMemInfo.uAllocSize;
	}

	UNLOCK_MEM_MGM_RES(psConnection);

	MEM_MGR_RES_FUNC_EXIT(psMemResource, retVal, psMemResource->sGpuMappingResourceObject.ui32RefCount);

	return retVal;
}


/* This function must be invoked with locked mutex */
static
IMG_INT32 MemoryResourceDevMemGpuReleaseBuffer(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT *psMemResource)
{
	PVRSRV_CONNECTION *psConnection = (PVRSRV_CONNECTION *)psMemResource->psDevData->psConnection;
	PVRSRV_GPU_MEMORY_STATE eCurState;
	IMG_UINT32 retVal = PVRSRV_OK;

	MEM_MGM_RES_FUNC_ENTER(psMemResource);

	/* Lock */
	LOCK_MEM_MGM_RES(psConnection);

	switch(psMemResource->eState)
	{
		case PVRSRV_GPU_MEMORY_ACTIVE_STATE:
			/* It is in the Active list -remove it to make sure nobody else gets it */
			list_del_init(&psMemResource->sListNode);
			psConnection->sBuffActiveList.ui32BuffersSize -= psMemResource->sClientMemInfo.uAllocSize;
			break;
		case PVRSRV_GPU_MEMORY_PENDING_EVICTION_STATE:
			/* It is in the Eviction Pending list - remove it to make sure nobody else gets it */
			list_del_init(&psMemResource->sListNode);
			psConnection->sBuffEvictionPendingList.ui32BuffersSize -= psMemResource->sClientMemInfo.uAllocSize;
			break;
		case PVRSRV_GPU_MEMORY_EVICTED_STATE:
			/* It is already in the Evicted list  */
			break;
		default:
			PVR_DPF((PVR_DBG_ERROR, "Invalid state %d of resource %p",
					psMemResource->eState, psMemResource));
			/* This is a major error - should never happen */
			retVal = -PVRSRV_ERROR_INVALID_CONTEXT;
			break;
	}

	eCurState = psMemResource->eState;

	/* UnLock */
	UNLOCK_MEM_MGM_RES(psConnection);

	/* If the memory is already evicted, there is nothing to do here */
	if((eCurState != PVRSRV_GPU_MEMORY_EVICTED_STATE) && (retVal == PVRSRV_OK ))
	{
		/* Buffer is busy - put it on the evition pending list */
		if( PVRSRVMemoryBufferInUseByGpu(&psMemResource->sClientMemInfo))
		{

			PVR_DPF((PVR_DBG_EVICT_VERBOSE, "DEFER UN-MAPPING BUFFER Resource %p Address 0x%08x to 0x%08x with size %d for %p",
					psMemResource, psMemResource->sClientMemInfo.sDevVAddr.uiAddr,
					(psMemResource->sClientMemInfo.sDevVAddr.uiAddr + psMemResource->sClientMemInfo.uAllocSize),
					psMemResource->sClientMemInfo.uAllocSize,
					psMemResource));

			/* Lock */
			LOCK_MEM_MGM_RES(psConnection);

			psMemResource->eState = PVRSRV_GPU_MEMORY_PENDING_EVICTION_STATE;
			list_move_tail(&psMemResource->sListNode, &psConnection->sBuffEvictionPendingList.sList);
			psConnection->sBuffEvictionPendingList.ui32BuffersSize += psMemResource->sClientMemInfo.uAllocSize;

			/* UnLock */
			UNLOCK_MEM_MGM_RES(psConnection);
		}
		else
		{
			retVal = MemoryResourceDevMemGpuUnmapBuffer(psMemResource);
		}
	}

	MEM_MGR_RES_FUNC_EXIT(psMemResource, retVal, psMemResource->sGpuMappingResourceObject.ui32RefCount);

	return retVal;
}

/**********************************************************************************************/
/************************ Resource Services Functions *****************************************/
/**********************************************************************************************/

#define MEM_SERVICE_RES_FUNC_ENTER(psmConnection) \
		PVR_DPF((PVR_DBG_CALLTRACE, "\n\nEnter %s with connection %p, and sizes (%d, %d, %d, %d) = total %d",\
				__FUNCTION__, (psmConnection),\
				psmConnection->sBuffActiveList.ui32BuffersSize, \
				psmConnection->sBuffEvictionPendingList.ui32BuffersSize, \
				psmConnection->sBuffEvictedList.ui32BuffersSize, \
				psmConnection->sBuffFreePendingList.ui32BuffersSize, \
				psmConnection->sBuffActiveList.ui32BuffersSize + \
				psmConnection->sBuffEvictionPendingList.ui32BuffersSize + \
				psmConnection->sBuffEvictedList.ui32BuffersSize + \
				psmConnection->sBuffFreePendingList.ui32BuffersSize))

#define MEM_SERVICE_RES_FUNC_EXIT(psmConnection) \
	PVR_DPF((PVR_DBG_CALLTRACE, "Exit %s with connection %p, and sizes (%d, %d, %d, %d) = total %d\n",\
			__FUNCTION__, (psmConnection),\
			psmConnection->sBuffActiveList.ui32BuffersSize, \
			psmConnection->sBuffEvictionPendingList.ui32BuffersSize, \
			psmConnection->sBuffEvictedList.ui32BuffersSize, \
			psmConnection->sBuffFreePendingList.ui32BuffersSize, \
			psmConnection->sBuffActiveList.ui32BuffersSize + \
			psmConnection->sBuffEvictionPendingList.ui32BuffersSize + \
			psmConnection->sBuffEvictedList.ui32BuffersSize + \
			psmConnection->sBuffFreePendingList.ui32BuffersSize))

IMG_EXPORT
IMG_INT32 MemoryResourceInitService(IMG_CONST PVRSRV_CONNECTION *psConnection)
{
	PVRSRV_CONNECTION *psmConnection = (PVRSRV_CONNECTION *)psConnection;

	MEM_SERVICE_RES_FUNC_ENTER(psConnection);

	PVRSRVCreateMutex( (PVRSRV_MUTEX_HANDLE *) &((psmConnection)->hMutex) );

	list_init(&psmConnection->sBuffActiveList.sList);
	psmConnection->sBuffActiveList.ui32BuffersSize = 0;

	list_init(&psmConnection->sBuffEvictionPendingList.sList);
	psmConnection->sBuffEvictionPendingList.ui32BuffersSize = 0;

	list_init(&psmConnection->sBuffEvictedList.sList);
	psmConnection->sBuffEvictedList.ui32BuffersSize = 0;

	list_init(&psmConnection->sBuffFreePendingList.sList);
	psmConnection->sBuffFreePendingList.ui32BuffersSize = 0;

	psmConnection->ui64EvictionSubSystemEnableMask = GPU_MEMORY_SUBSYSTEM_ALL;

	psmConnection->ui64EvictionSubSystemDebugMask = 0;

	MEM_SERVICE_RES_FUNC_EXIT(psConnection);

	return PVRSRV_OK;
}

/*!
*******************************************************************************

 @Function	Name MemoryResourceReclaimMemory

 @Description

 @Input        psConnection : Description

 @Input        isShutdown : Description

 @Return       -PVRSRV_ERROR if error, >=0 on success

******************************************************************************/
IMG_EXPORT
IMG_INT32 MemoryResourceReclaimMemory(IMG_CONST PVRSRV_CONNECTION *psConnection, IMG_BOOL isShutdown)
{
	PVRSRV_CONNECTION *psmConnection = (PVRSRV_CONNECTION *)psConnection;
	PVRSRV_BUFFER_LIST sOutList;
	IMG_UINT32 numNodes;

	MEM_SERVICE_RES_FUNC_ENTER(psConnection);

	if(isShutdown) /* TODO: Wait forever until all nodes are processed */
	{
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Enter %s with connection %p for SHUTDOWN",
				__FUNCTION__, psConnection));

		/************************ START Free the Free Pending List **********************/
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Start Releasing Free Pending List with size %d",
				psmConnection->sBuffFreePendingList.ui32BuffersSize));

		list_init(&sOutList.sList);
		sOutList.ui32BuffersSize = 0;

		numNodes = MemoryResourceGetFreeNodesPendingList(psmConnection, &psmConnection->sBuffFreePendingList,
				&sOutList, PVRSRV_GPU_MEMORY_PENDING_FREE_STATE,
				(IMG_UINT32)-1 /* Max Nodes */, 	(IMG_UINT32)-1 /* MB Max memory */ );
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Free Pending List %d nodes processed, size left %d, spill list size %d",
				numNodes, psmConnection->sBuffFreePendingList.ui32BuffersSize, sOutList.ui32BuffersSize ));

		numNodes = MemoryResourceProcessMemoryNodesList(&sOutList, MemoryResourceDevMemFreeBuffer);
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Spill List %d nodes processed, size left %d",
				numNodes, sOutList.ui32BuffersSize ));

		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "End Releasing Free Pending List with size  %d\n",
				psmConnection->sBuffFreePendingList.ui32BuffersSize));
		/* ************************ END Releasing the Free Pending List ********************/

		/* ************************ Then free the Evicted List ************************ */
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Start Releasing Evicted List with size  %d",
				psmConnection->sBuffEvictedList.ui32BuffersSize));

		list_init(&sOutList.sList);
		sOutList.ui32BuffersSize = 0;

		numNodes = MemoryResourceGetFreeNodesPendingList(psmConnection, &psmConnection->sBuffEvictedList,
				&sOutList, PVRSRV_GPU_MEMORY_PENDING_FREE_STATE,
				(IMG_UINT32)-1 /* Max Nodes */, 	(IMG_UINT32)-1 /* MB Max memory */ );
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Evicted List %d nodes processed, size left %d, spill list size %d",
				numNodes, psmConnection->sBuffEvictedList.ui32BuffersSize, sOutList.ui32BuffersSize ));

		numNodes = MemoryResourceProcessMemoryNodesList(&sOutList, MemoryResourceDevMemFreeBuffer);
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Spill List %d nodes processed, size left %d",
				numNodes, sOutList.ui32BuffersSize ));

		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "End Releasing Evicted List with size  %d\n",
				psmConnection->sBuffEvictedList.ui32BuffersSize));
		/* ************************ End Evicted List ************************ */

		/* ************************ Then free the Eviction Pending List ************************ */
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Start Releasing Eviction Pending List with size %d",
				psmConnection->sBuffEvictionPendingList.ui32BuffersSize));

		list_init(&sOutList.sList);
		sOutList.ui32BuffersSize = 0;

		numNodes = MemoryResourceGetFreeNodesPendingList(psmConnection, &psmConnection->sBuffEvictionPendingList,
				&sOutList, PVRSRV_GPU_MEMORY_PENDING_FREE_STATE,
				(IMG_UINT32)-1 /* Max Nodes */, 	(IMG_UINT32)-1 /* MB Max memory */ );
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Eviction Pending List %d nodes processed, size left %d, spill list size %d",
				numNodes, psmConnection->sBuffEvictionPendingList.ui32BuffersSize, sOutList.ui32BuffersSize ));

		numNodes = MemoryResourceProcessMemoryNodesList(&sOutList, MemoryResourceDevMemFreeBuffer);
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Spill List %d nodes processed, size left %d",
				numNodes, sOutList.ui32BuffersSize ));

		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "End Eviction Pending List with size  %d\n",
				psmConnection->sBuffEvictionPendingList.ui32BuffersSize));
		/* ************************ End Eviction Pending List ************************ */

		/* ************************ And the active list at the end - must be empty,
		 * if all the clients released their memory correctly *************************************/
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Start Releasing Active List with size  %d",
				psmConnection->sBuffActiveList.ui32BuffersSize));

		list_init(&sOutList.sList);
		sOutList.ui32BuffersSize = 0;

		numNodes = MemoryResourceGetFreeNodesPendingList(psmConnection, &psmConnection->sBuffActiveList,
				&sOutList, PVRSRV_GPU_MEMORY_PENDING_FREE_STATE,
				(IMG_UINT32)-1 /* Max Nodes */, 	(IMG_UINT32)-1 /* MB Max memory */ );
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "ActiveList %d nodes processed, size left %d, spill list size %d",
				numNodes, psmConnection->sBuffActiveList.ui32BuffersSize, sOutList.ui32BuffersSize ));

		numNodes = MemoryResourceProcessMemoryNodesList(&sOutList, MemoryResourceDevMemFreeBuffer);
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Spill List %d nodes processed, size left %d",
				numNodes, sOutList.ui32BuffersSize ));

		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "End Releasing Active List with size  %d\n",
				psmConnection->sBuffActiveList.ui32BuffersSize));
		/* ************************ End active list ************************************************ */

		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Exit %s with connection %p for SHUTDOWN",
				__FUNCTION__, psConnection));

	}
	else /* Not a process shutdown ? Release some memory from the pending lists */
	{
		/* ************************ START Releasing the Free Pending List ********************/
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Start Releasing Free Pending List with size  %d",
				psmConnection->sBuffFreePendingList.ui32BuffersSize));

		list_init(&sOutList.sList);
		sOutList.ui32BuffersSize = 0;

		numNodes = MemoryResourceGetFreeNodesPendingList(psmConnection, &psmConnection->sBuffFreePendingList,
				&sOutList, PVRSRV_GPU_MEMORY_PENDING_FREE_STATE,
				4 /* Max Nodes */, 	(16 * 1024) /* MB Max memory */ );
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Free Pending List %d nodes processed, size left %d, spill list size %d",
				numNodes, psmConnection->sBuffFreePendingList.ui32BuffersSize, sOutList.ui32BuffersSize ));

		numNodes = MemoryResourceProcessMemoryNodesList(&sOutList, MemoryResourceDevMemFreeBuffer);
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Spill List %d nodes processed, size left %d",
				numNodes, sOutList.ui32BuffersSize ));

		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "End Releasing Free Pending List with size  %d\n",
				psmConnection->sBuffFreePendingList.ui32BuffersSize));
		/* ************************ END Releasing the Free Pending List ********************/

		/* ************************ START Releasing the Eviction Pending List ********************/
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Start Releasing Eviction Pending List with size  %d",
				psmConnection->sBuffEvictionPendingList.ui32BuffersSize));

		list_init(&sOutList.sList);
		sOutList.ui32BuffersSize = 0;

		numNodes = MemoryResourceGetFreeNodesPendingList(psmConnection, &psmConnection->sBuffEvictionPendingList,
				&sOutList, PVRSRV_GPU_MEMORY_PENDING_EVICTION_STATE,
				4 /* Max Nodes */, 	(16 * 1024) /* MB Max memory */ );
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Eviction Pending List %d nodes processed, size left %d, spill list size %d",
				numNodes, psmConnection->sBuffEvictionPendingList.ui32BuffersSize, sOutList.ui32BuffersSize ));

		numNodes = MemoryResourceProcessMemoryNodesList(&sOutList, MemoryResourceDevMemGpuUnmapBuffer);
		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Spill List %d nodes processed, size left %d",
						numNodes, sOutList.ui32BuffersSize ));

		PVR_DPF((PVR_DBG_EVICT_VERBOSE, "End Eviction Pending List with size  %d\n",
				psmConnection->sBuffEvictionPendingList.ui32BuffersSize));

		/* ************************ END Releasing the Eviction Pending List ********************/
	}

	MEM_SERVICE_RES_FUNC_EXIT(psConnection);

	return PVRSRV_OK;
}

/*!
*******************************************************************************

 @Function	Name MemoryResourceCloseService

 @Description

 @Input        psConnection :

 @Return       -PVRSRV_ERROR on error

******************************************************************************/
IMG_EXPORT
IMG_INT32 MemoryResourceCloseService(IMG_CONST PVRSRV_CONNECTION *psConnection)
{
	IMG_INT32 retVal;

	MEM_SERVICE_RES_FUNC_ENTER(psConnection);

	retVal = MemoryResourceReclaimMemory(psConnection, IMG_TRUE);

	PVRSRVDestroyMutex((PVRSRV_MUTEX_HANDLE)psConnection->hMutex);
	((PVRSRV_CONNECTION *)psConnection)->hMutex = IMG_NULL;

	MEM_SERVICE_RES_FUNC_EXIT(psConnection);

	return retVal;
}

/*!
*******************************************************************************

 @Function	Name MemoryResourceSetEvictionDebugMask

 @Description

 @Input        psConnection :

 @Input        ui64EvictionSubSystemDebugMask :

 @Return       The Old Debug Enable Mask

******************************************************************************/
IMG_EXPORT
IMG_UINT64 MemoryResourceSetEvictionDebugMask(IMG_CONST PVRSRV_CONNECTION *psConnection,
		IMG_UINT64 ui64EvictionSubSystemDebugMask)
{
	PVRSRV_CONNECTION *psmConnection = (PVRSRV_CONNECTION *)psConnection;
	IMG_UINT64 ui64OldEvictionSubSystemDebugMask = psmConnection->ui64EvictionSubSystemDebugMask;

	psmConnection->ui64EvictionSubSystemDebugMask = ui64EvictionSubSystemDebugMask;

	return ui64OldEvictionSubSystemDebugMask;
}

/*!
*******************************************************************************

 @Function	Name MemoryResourceSetEvictionEnableMask

 @Description

 @Input        psConnection :

 @Input        ui64EvictionSubSystemEnableMask :

 @Return       the Old Eviction Enable Mask

******************************************************************************/
IMG_EXPORT
IMG_UINT64 MemoryResourceSetEvictionEnableMask(IMG_CONST PVRSRV_CONNECTION *psConnection,
		IMG_UINT64 ui64EvictionSubSystemEnableMask)
{
	PVRSRV_CONNECTION *psmConnection = (PVRSRV_CONNECTION *)psConnection;
	IMG_UINT64 ui64OldEvictionSubSystemEnableMask = psmConnection->ui64EvictionSubSystemEnableMask;

	psmConnection->ui64EvictionSubSystemEnableMask = ui64EvictionSubSystemEnableMask;

	return ui64OldEvictionSubSystemEnableMask;
}

/******************************************************************************
 End of file (resources.c)
******************************************************************************/
