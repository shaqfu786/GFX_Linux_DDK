/*****************************************************************************
* File			pvr_bridge_u.c
* 
* Title			Bridging code
* 
* Author		 Imagination Technologies
* 
* date   		16 September 2003
* 
* Copyright      Copyright 2003-2010 by Imagination Technologies Limited.
*                All rights reserved. No part of this software, either
*                material or conceptual may be copied or distributed,
*                transmitted, transcribed, stored in a retrieval system
*                or translated into any human or computer language in any
*                form by any means, electronic, mechanical, manual or
*                other-wise, or disclosed to third parties without the
*                express written permission of Imagination Technologies
*                Limited, Unit 8, HomePark Industrial Estate,
*                King's Langley, Hertfordshire, WD4 8LZ, U.K.
* 
* 
* $Log: pvr_bridge_u.c $
* 
*  --- Revision Logs Removed --- 
* *****************************************************************************/
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#if defined(SUPPORT_DRI_DRM)
#if !defined(SUPPORT_DRI_DRM_NO_LIBDRM)
#include <xf86drm.h>
#else
#include "drmlite.h"
#endif
#if defined(SUPPORT_DRI_DRM_EXT)
#include "sys_pvr_drm_import.h"
#endif
#include "pvr_drm_shared.h"
#include "pvr_drm.h"
#endif
#include "img_defs.h"
#include "services.h"
#include "pvr_bridge.h"
#include "pvr_bridge_client.h"
#include "pvr_debug.h"
#include "memchk.h"

#include <graphics_buffer_object_priv.h>

struct UserServicesHandle {
	/* The following file descriptor field must be first in the
 	 * structure, so that Linux specific code can access it without
 	 * having to know the details of the rest of ths structure.
 	 */
	IMG_INT fd;				/* Must be first field in structure */
#if defined (SUPPORT_SID_INTERFACE)
	IMG_SID hKernelServices;
#else
	IMG_HANDLE hKernelServices;
#endif
};

/*!
*****************************************************************************
 @Function	: OpenServices
    
 @Description	: opens a services connection

 @Output : phServices - handle for connection
		  
 @Return	: PVRSRV_ERROR
*****************************************************************************/
PVRSRV_ERROR IMG_INTERNAL OpenServices(IMG_HANDLE *phServices, IMG_UINT32 ui32SrvFlags)
{
	IMG_INT fd;
	PVRSRV_BRIDGE_IN_CONNECT_SERVICES sIn;
	PVRSRV_BRIDGE_OUT_CONNECT_SERVICES sOut;
	struct UserServicesHandle *psUserHand;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));

#if defined(SUPPORT_DRI_DRM)
	fd = PVRDRMOpen();
	if (fd == -1)
	{
		PVR_DPF((PVR_DBG_ERROR, "OpenServices: drmOpen failed"));
		return PVRSRV_ERROR_INIT_FAILURE;
	}
#if !defined(SUPPORT_DRI_DRM_NO_DROPMASTER)
	/*
	 * We don't want to be the master.  There can be only one, and
	 * it should be the X server (or equivalent).
	 */
	if (drmCommandNone(fd, PVR_DRM_IS_MASTER_CMD) == 0)
	{
		(void) drmDropMaster(fd);
	}
#endif
#else /* defined(SUPPORT_DRI_DRM) */
	fd = open("/dev/" PVRSRV_MODNAME, O_RDWR);
	if (fd == -1)
	{
		PVR_DPF((PVR_DBG_ERROR, "OpenServices: Cannot open device driver /dev/" PVRSRV_MODNAME "."));
		return PVRSRV_ERROR_INIT_FAILURE;
	}
#endif /* defined(SUPPORT_DRI_DRM) */

	/* We don't want our services connection to be inherited by child
	 * processes that fork() then exec(). Otherwise, the services
	 * connection for the parent is not released until the termination
	 * of all child processes.
	 */
	if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0)
	{
		(IMG_VOID)close(fd);

		PVR_DPF((PVR_DBG_ERROR, "OpenServices: Failed to set FD_CLOEXEC on services fd."));
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	psUserHand = (struct UserServicesHandle *)PVRSRVAllocUserModeMem(sizeof (*psUserHand));

	if (psUserHand == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "OpenServices: Cannot allocate user services handle."));
		return PVRSRV_ERROR_INIT_FAILURE;
	}
	psUserHand->fd = fd;
	psUserHand->hKernelServices = IMG_NULL;
	
	PVR_DPF((PVR_DBG_MESSAGE, "OpenServices: PID = %d, flags = %x", getpid(), ui32SrvFlags));
	sIn.ui32Flags = ui32SrvFlags;

	if (PVRSRVBridgeCall((IMG_HANDLE)psUserHand,
			 PVRSRV_BRIDGE_CONNECT_SERVICES,
			 &sIn,
			 sizeof(sIn),
			 &sOut,
			 sizeof(sOut)))
	{
		PVRSRVFreeUserModeMem(psUserHand);
#if defined(SUPPORT_DRI_DRM)
		(IMG_VOID)drmClose(fd);
#else
		(IMG_VOID)close(fd);
#endif
		PVR_DPF((PVR_DBG_ERROR, "OpenServices: PVRSRVBridgeCall failed."));
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	psUserHand->hKernelServices = sOut.hKernelServices;

	*phServices = (IMG_HANDLE)psUserHand;

	return PVRSRV_OK;
}



/*!
*****************************************************************************
 @Function	: CloseServices
    
 @Description	: closes a services connection

 @Input : hServices - handle for connection
		  
 @Return	: PVRSRV_ERROR
*****************************************************************************/
PVRSRV_ERROR IMG_INTERNAL CloseServices(IMG_HANDLE hServices)
{
	struct UserServicesHandle *psUserHand = (struct UserServicesHandle *)hServices;
	PVRSRV_BRIDGE_RETURN sOut;
	IMG_INT iRes;

	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));
 

	if(psUserHand == IMG_NULL)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	(IMG_VOID)PVRSRVBridgeCall(hServices,
			 PVRSRV_BRIDGE_DISCONNECT_SERVICES,
			 IMG_NULL,
			 0,
			 &sOut,
			 sizeof(sOut));

#if defined(SUPPORT_DRI_DRM)
	iRes = drmClose(psUserHand->fd);
#else
	iRes = close(psUserHand->fd);
#endif
	if (iRes == -1)
	{
		return PVRSRV_ERROR_UNABLE_TO_CLOSE_SERVICES;
	}

	PVRSRVFreeUserModeMem(psUserHand);
	
	OutputMemoryStats();

	return PVRSRV_OK;
}



/*!
*****************************************************************************
 @Function	: PVRSRVBridgeCall
    
 @Description	: To call a kernel-resident function from user space

 @Input : hServices - handle for connection

 @Input : ui32FunctionID - function index

 @Input : pvParamIn
 @Input : ui32InBufferSize
 @Input : pvParamOut
 @Input : ui32OutBufferSize
		  
 @Return	: IMG_RESULT
*****************************************************************************/
IMG_RESULT IMG_INTERNAL PVRSRVBridgeCall(IMG_HANDLE hServices,
										 IMG_UINT32 ui32FunctionID,
										 IMG_VOID *pvParamIn,
										 IMG_UINT32 ui32InBufferSize,
										 IMG_VOID *pvParamOut,
										 IMG_UINT32	ui32OutBufferSize)
{
	struct UserServicesHandle *psUserHand = (struct UserServicesHandle *)hServices;
	PVRSRV_BRIDGE_PACKAGE sBridgePackage;
	IMG_INT iResult;

	PVRSRVMemSet(&sBridgePackage,0x00,sizeof(sBridgePackage));

	PVR_DPF((PVR_DBG_VERBOSE, "%s: Calling function %d", __FUNCTION__, ui32FunctionID));

	sBridgePackage.ui32BridgeID = ui32FunctionID;
	sBridgePackage.ui32Size = sizeof (PVRSRV_BRIDGE_PACKAGE);
	sBridgePackage.pvParamIn = pvParamIn;
	sBridgePackage.ui32InBufferSize = ui32InBufferSize;
	sBridgePackage.pvParamOut = pvParamOut;
	sBridgePackage.ui32OutBufferSize = ui32OutBufferSize;
	sBridgePackage.hKernelServices = psUserHand->hKernelServices;

#if defined(SUPPORT_DRI_DRM)
	iResult = drmCommandWrite(psUserHand->fd, PVR_DRM_SRVKM_CMD, &sBridgePackage, sizeof(sBridgePackage));
#else
	iResult = ioctl(psUserHand->fd, ui32FunctionID, &sBridgePackage);
#endif
	if(iResult < 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVBridgeCall: Failed to access device.  Function ID:%u (%s).", ui32FunctionID, strerror(errno)));
	}

	return iResult;
}

IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVCloseExportedDeviceMemHanle(const PVRSRV_DEV_DATA *psDevData,
															IMG_INT i32Fd)
{
	PVR_UNREFERENCED_PARAMETER(psDevData);

	if(i32Fd < 0)
		return PVRSRV_ERROR_INVALID_DEVICE;

	close(i32Fd);

	return PVRSRV_OK;
}

IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVExportDeviceMem2(const PVRSRV_DEV_DATA	*psDevData,
												 PVRSRV_CLIENT_MEM_INFO	*psMemInfo,
												 IMG_INT				*iFd)
{
	PVRSRV_BRIDGE_OUT_EXPORTDEVICEMEM sOut;
	struct UserServicesHandle *psUserHand;
	PVRSRV_BRIDGE_IN_EXPORTDEVICEMEM sIn;
	int fd;

	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));
	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));


	if(!psDevData || !psMemInfo || !iFd)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVExportDeviceMem2: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sIn.hDevCookie = psDevData->hDevCookie;
#if defined (SUPPORT_SID_INTERFACE)
	sIn.hKernelMemInfo = psMemInfo->hKernelMemInfo;
#else
	sIn.psKernelMemInfo = psMemInfo->hKernelMemInfo;
#endif

	/* With secure fd exports we want to create a new services connection for
	 * every exported device MemInfo. All subsequent bridge calls related to
	 * this device memory must use this file descriptor.
	 */
#if defined(SUPPORT_DRI_DRM)
	fd = PVRDRMOpen();
	if (fd == -1)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVExportDeviceMem2: drmOpen failed"));
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	/*
	 * We don't want to be the master.  There can be only one, and
	 * it should be the X server (or equivalent).
	 */
	if (drmCommandNone(fd, PVR_DRM_IS_MASTER_CMD) == 0)
	{
#ifndef SUPPORT_DRM_NO_DROPMASTER
		(void) drmDropMaster(fd);
#endif
	}
#else
	fd = open("/dev/" PVRSRV_MODNAME, O_RDWR);
	if(fd == -1)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVExportDeviceMem2: Cannot open device driver /dev/" PVRSRV_MODNAME "."));
		return PVRSRV_ERROR_INIT_FAILURE;
	}
#endif

	psUserHand = (struct UserServicesHandle *)PVRSRVAllocUserModeMem(sizeof (*psUserHand));
	psUserHand->fd = fd;
	psUserHand->hKernelServices =
		((struct UserServicesHandle *)psDevData->psConnection->hServices)->hKernelServices;

	if(PVRSRVBridgeCall((IMG_HANDLE)psUserHand,
						PVRSRV_BRIDGE_EXPORT_DEVICEMEM_2,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_EXPORTDEVICEMEM),
						(IMG_VOID *)&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_EXPORTDEVICEMEM)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVExportDeviceMem2: BridgeCall failed"));
		sOut.eError = PVRSRV_ERROR_BRIDGE_CALL_FAILED;
		goto free_and_return;
	}

	if(sOut.eError == PVRSRV_OK)
	{
		*iFd = psUserHand->fd;
#if defined(SUPPORT_MEMINFO_IDS)
		psMemInfo->ui64Stamp = sOut.ui64Stamp;
#endif
	}

free_and_return:
	PVRSRVFreeUserModeMem(psUserHand);
	return sOut.eError;
}


IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVMapDeviceMemory2(IMG_CONST PVRSRV_DEV_DATA	*psDevData,
												 IMG_INT					iFd,
#if defined (SUPPORT_SID_INTERFACE)
												 IMG_SID					hDstDevMemHeap,
#else
												 IMG_HANDLE					hDstDevMemHeap,
#endif
												 PVRSRV_CLIENT_MEM_INFO		**ppsDstMemInfo)
{
	PVRSRV_BRIDGE_IN_MAP_DEV_MEMORY sBridgeIn;
	PVRSRV_BRIDGE_OUT_MAP_DEV_MEMORY sBridgeOut;
	PVRSRV_CLIENT_MEM_INFO *psMemInfo=IMG_NULL;
	PVRSRV_ERROR eError=PVRSRV_OK;
	PVRSRV_BRIDGE_IN_UNMAP_DEV_MEMORY sInFailure;
	PVRSRV_BRIDGE_RETURN sRet;
	PVRSRV_CLIENT_SYNC_INFO *psSyncInfo = IMG_NULL;
	struct UserServicesHandle *psUserHand = IMG_NULL;
	IMG_HANDLE hServices;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));
	PVRSRVMemSet(&sInFailure,0x00,sizeof(sInFailure));
	PVRSRVMemSet(&sRet,0x00,sizeof(sRet));	
	PVRSRVMemSet(&hServices,0x00,sizeof(hServices));	
	
	if(!psDevData || iFd < 0 || !hDstDevMemHeap || !ppsDstMemInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVMapDeviceMemory2: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* The iFd file descriptor has stored a handle to the desired MemInfo.
	 * We must make the bridge call on this file descriptor in order to
	 * retrieve the global handle in the kernel.
	 */
	psUserHand = (struct UserServicesHandle *)PVRSRVAllocUserModeMem(sizeof (*psUserHand));
	psUserHand->fd = iFd;
	psUserHand->hKernelServices =
		((struct UserServicesHandle *)psDevData->psConnection->hServices)->hKernelServices;

	hServices = (IMG_HANDLE)psUserHand;

	sBridgeIn.hDstDevMemHeap = hDstDevMemHeap;

	if(PVRSRVBridgeCall(hServices,
						PVRSRV_BRIDGE_MAP_DEV_MEMORY_2,
						&sBridgeIn,
						sizeof(PVRSRV_BRIDGE_IN_MAP_DEV_MEMORY),
						&sBridgeOut,
						sizeof(PVRSRV_BRIDGE_OUT_MAP_DEV_MEMORY)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVMapDeviceMemory2: BridgeCall failed"));
		eError = PVRSRV_ERROR_BRIDGE_CALL_FAILED;

		goto MAP_DEV_MEMORY_OK;
	}

	if(sBridgeOut.eError != PVRSRV_OK)
	{
		eError = sBridgeOut.eError;

		goto MAP_DEV_MEMORY_OK;
	}

	if(sBridgeOut.sDstClientMemInfo.ui32Flags & PVRSRV_HAP_GPU_PAGEABLE) /* If eviction is supported, allocate an Resource Object instead */
		psMemInfo = (PVRSRV_CLIENT_MEM_INFO *)PVRSRVCallocUserModeMem(sizeof(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT));
	else
		psMemInfo = (PVRSRV_CLIENT_MEM_INFO *)PVRSRVAllocUserModeMem(sizeof(PVRSRV_CLIENT_MEM_INFO));

	if(psMemInfo == IMG_NULL)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;

		goto MAP_DEV_MEMORY_CLEANUP_MAP;
	}

	if(sBridgeOut.sDstClientSyncInfo.hKernelSyncInfo)
	{
		psSyncInfo = (PVRSRV_CLIENT_SYNC_INFO *)PVRSRVAllocUserModeMem(sizeof(PVRSRV_CLIENT_SYNC_INFO));

		if (!psSyncInfo)
		{
			eError = PVRSRV_ERROR_OUT_OF_MEMORY;

			goto MAP_DEV_MEMORY_CLEANUP_MEMINFO_ALLOC;
		}
	}

	*psMemInfo  = sBridgeOut.sDstClientMemInfo;

	eError = PVRPMapKMem(psDevData->psConnection->hServices,
								 (IMG_VOID**)&psMemInfo->pvLinAddr,
								 psMemInfo->pvLinAddrKM,
								 &psMemInfo->hMappingInfo,
								 psMemInfo->hKernelMemInfo);

	if (!psMemInfo->pvLinAddr || (eError != PVRSRV_OK))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVMapDeviceMemory2: PVRPMapKMem failed for buffer "));

		eError = PVRSRV_ERROR_BAD_MAPPING;

		goto MAP_DEV_MEMORY_CLEANUP_SYNCINFO_ALLOC;
	}

	if(psSyncInfo)
	{
		*psSyncInfo = sBridgeOut.sDstClientSyncInfo;

#if !defined(PVRSRV_DISABLE_UM_SYNCOBJ_MAPPINGS)
		eError = PVRPMapKMem(psDevData->psConnection->hServices,
									 (IMG_VOID**)&psSyncInfo->psSyncData,
									 psSyncInfo->psSyncData,
									 &psSyncInfo->hMappingInfo,
									 psSyncInfo->hKernelSyncInfo);

		if (!psSyncInfo->psSyncData || (eError != PVRSRV_OK))
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVMapDeviceMemory2: PVRPMapKMem failed for syncdata "));

			eError = PVRSRV_ERROR_BAD_MAPPING;

			goto MAP_DEV_MEMORY_CLEANUP_LINADDR;
		}
#endif
		psMemInfo->psClientSyncInfo = psSyncInfo;
	}

	*ppsDstMemInfo = psMemInfo;
	goto MAP_DEV_MEMORY_OK;

#if !defined(PVRSRV_DISABLE_UM_SYNCOBJ_MAPPINGS)
MAP_DEV_MEMORY_CLEANUP_LINADDR:
	PVRUnMapKMem(psDevData->psConnection->hServices, psMemInfo->hMappingInfo, psMemInfo->hKernelMemInfo);
#endif

MAP_DEV_MEMORY_CLEANUP_SYNCINFO_ALLOC:
	if(psSyncInfo)
	{
		PVRSRVFreeUserModeMem(psSyncInfo);
	}

MAP_DEV_MEMORY_CLEANUP_MEMINFO_ALLOC:
	PVRSRVFreeUserModeMem(psMemInfo);

MAP_DEV_MEMORY_CLEANUP_MAP:
#if defined (SUPPORT_SID_INTERFACE)
	sInFailure.hKernelMemInfo = sBridgeOut.sDstClientMemInfo.hKernelMemInfo;
#else
	sInFailure.psKernelMemInfo = sBridgeOut.sDstClientMemInfo.hKernelMemInfo;
#endif
	PVRSRVBridgeCall(psDevData->psConnection->hServices,
					 PVRSRV_BRIDGE_UNMAP_DEV_MEMORY,
					 &sInFailure,
					 sizeof(PVRSRV_BRIDGE_IN_UNMAP_DEV_MEMORY),
					 (IMG_VOID *)&sRet,
					 sizeof(PVRSRV_BRIDGE_RETURN));

	*ppsDstMemInfo = IMG_NULL;

MAP_DEV_MEMORY_OK:
	PVRSRVFreeUserModeMem(psUserHand);
	return eError;
}
