/*!****************************************************************************
@File           bridged_sgx_glue.c

@Title          Shared SGX glue code

@Author         Imagination Technologies

@Copyright      Copyright 2008-2009 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Description    Implements shared SGX API user bridge code

Modifications :-
$Log: bridged_sgx_glue.c $
******************************************************************************/

#if !defined(PVR_KERNEL)
#include <string.h>
#endif

#include "img_defs.h"
#include "services.h"
#include "sgx_bridge.h"
#include "sgx_bridge_um.h"
#include "pvr_bridge_client.h"
#include "sgxapi.h"
#include "sgxinfo_client.h"
#include "sgxtransfer_client.h"
#include "servicesinit_um.h"
#include "sgxinit_um.h"
#include "pvr_debug.h"
#include "bridged_sgx_glue.h"

#include "pvrversion.h"

#include "sgx_options.h"


#include "os_srvclient_config.h"

#if !defined(OS_SGX_FIND_SHARED_PBDESC_UM)
IMG_INTERNAL
PVRSRV_ERROR SGXFindSharedPBDesc(const PVRSRV_DEV_DATA *psDevData,
								 IMG_BOOL bLockOnFailure,
								 IMG_UINT32 ui32TotalPBSize,
#if defined (SUPPORT_SID_INTERFACE)
								 IMG_SID *phSharedPBDesc,
								 IMG_SID *phSharedPBDescKernelMemInfoHandle,
								 IMG_SID *phHWPBDescKernelMemInfoHandle,
								 IMG_SID *phBlockMemInfoHandle,
								 IMG_SID *phHWBlockMemInfoHandle,
								 IMG_SID **pphSharedPBDescSubKernelMemInfoHandles,
#else
								 IMG_HANDLE *phSharedPBDesc,
								 IMG_HANDLE *phSharedPBDescKernelMemInfoHandle,
								 IMG_HANDLE *phHWPBDescKernelMemInfoHandle,
								 IMG_HANDLE *phBlockMemInfoHandle,
								 IMG_HANDLE *phHWBlockMemInfoHandle,
								 IMG_HANDLE **pphSharedPBDescSubKernelMemInfoHandles,
#endif
								 IMG_UINT32 *pui32SharedPBDescSubKernelMemInfoHandlesCount)
{
	PVRSRV_BRIDGE_IN_SGXFINDSHAREDPBDESC sBridgeIn;
#if defined (SUPPORT_SID_INTERFACE)
	PVRSRV_BRIDGE_OUT_SGXFINDSHAREDPBDESC sBridgeOut = {0};
	IMG_SID    *phSharedPBDescSubKernelMemInfoHandles;
#else
	PVRSRV_BRIDGE_OUT_SGXFINDSHAREDPBDESC sBridgeOut;
	IMG_HANDLE *phSharedPBDescSubKernelMemInfoHandles;
#endif
	IMG_UINT32 i;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	
	PVR_DPF((PVR_DBG_VERBOSE, "SGXFindSharedPBDesc"));

	if(!psDevData || !ui32TotalPBSize)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXFindSharedPBDesc: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hDevCookie = psDevData->hDevCookie;
	sBridgeIn.bLockOnFailure = bLockOnFailure;
	sBridgeIn.ui32TotalPBSize = ui32TotalPBSize;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_SGX_FINDSHAREDPBDESC,
						&sBridgeIn,
						sizeof(PVRSRV_BRIDGE_IN_SGXFINDSHAREDPBDESC),
						&sBridgeOut,
						sizeof(PVRSRV_BRIDGE_OUT_SGXFINDSHAREDPBDESC)))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXFindSharedPBDesc: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sBridgeOut.eError != PVRSRV_OK)
	{
		if (sBridgeOut.eError != PVRSRV_ERROR_PROCESSING_BLOCKED)
		{
			PVR_DPF((PVR_DBG_ERROR, "SGXFindSharedPBDesc: KM failed %d",sBridgeOut.eError));
		}
		return sBridgeOut.eError;
	}

	/* Note: Finding no handle isn't an error, so we don't check */
	*phSharedPBDesc = sBridgeOut.hSharedPBDesc;

	*phSharedPBDescKernelMemInfoHandle=sBridgeOut.hSharedPBDescKernelMemInfoHandle;
	*phHWPBDescKernelMemInfoHandle=sBridgeOut.hHWPBDescKernelMemInfoHandle;

	*phBlockMemInfoHandle=sBridgeOut.hBlockKernelMemInfoHandle;
	*phHWBlockMemInfoHandle=sBridgeOut.hHWBlockKernelMemInfoHandle;

	*pui32SharedPBDescSubKernelMemInfoHandlesCount =
		sBridgeOut.ui32SharedPBDescSubKernelMemInfoHandlesCount;

	if(*pui32SharedPBDescSubKernelMemInfoHandlesCount == 0)
	{
		*pphSharedPBDescSubKernelMemInfoHandles = IMG_NULL;
		return PVRSRV_OK;
	}

	phSharedPBDescSubKernelMemInfoHandles =
		PVRSRVAllocUserModeMem(sizeof(IMG_HANDLE)
							   * sBridgeOut.ui32SharedPBDescSubKernelMemInfoHandlesCount);

	/* FIXME: Check for successful allocation */

	for(i=0; i< *pui32SharedPBDescSubKernelMemInfoHandlesCount; i++)
	{
		phSharedPBDescSubKernelMemInfoHandles[i] =
			sBridgeOut.ahSharedPBDescSubKernelMemInfoHandles[i];
	}

	*pphSharedPBDescSubKernelMemInfoHandles =
		phSharedPBDescSubKernelMemInfoHandles;

	return PVRSRV_OK;
}
#endif /* !defined(OS_SGX_FIND_SHARED_PBDESC_UM) */


#if !defined(OS_SGX_UNREF_SHARED_PBDESC_UM)
IMG_INTERNAL
PVRSRV_ERROR SGXUnrefSharedPBDesc(const PVRSRV_DEV_DATA *psDevData,
#if defined (SUPPORT_SID_INTERFACE)
								  IMG_SID hSharedPBDesc)
#else
								  IMG_HANDLE hSharedPBDesc)
#endif
{
	PVRSRV_BRIDGE_IN_SGXUNREFSHAREDPBDESC sBridgeIn;
	PVRSRV_BRIDGE_OUT_SGXUNREFSHAREDPBDESC sBridgeOut;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

	PVR_DPF((PVR_DBG_VERBOSE, "SGXUnrefSharedPBDesc"));

	if (!psDevData || !hSharedPBDesc)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXUnrefSharedPBDesc: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hSharedPBDesc = hSharedPBDesc;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_SGX_UNREFSHAREDPBDESC,
						&sBridgeIn,
						sizeof(PVRSRV_BRIDGE_IN_SGXUNREFSHAREDPBDESC),
						&sBridgeOut,
						sizeof(PVRSRV_BRIDGE_OUT_SGXUNREFSHAREDPBDESC)))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXUnrefSharedPBDesc: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sBridgeOut.eError;
}
#endif /* !defined(OS_SGX_UNREF_SHARED_PBDESC_UM) */


#if !defined(OS_SGX_ADD_SHARED_PBDESC_UM)
IMG_INTERNAL
PVRSRV_ERROR SGXAddSharedPBDesc(const PVRSRV_DEV_DATA *psDevData,
								IMG_UINT32 ui32TotalPBSize,
#if defined (SUPPORT_SID_INTERFACE)
								IMG_SID    hSharedPBDescKernelMemInfoHandle,
								IMG_SID    hHWPBDescKernelMemInfoHandle,
								IMG_SID    hPBBlockClientMemInfoHandle,
								IMG_SID    hHWPBBlockClientMemInfoHandle,
								IMG_SID    *phSharedPBDesc,
								IMG_SID    *phSubKernelMemInfos,
#else
								IMG_HANDLE hSharedPBDescKernelMemInfoHandle,
								IMG_HANDLE hHWPBDescKernelMemInfoHandle,
								IMG_HANDLE hPBBlockClientMemInfoHandle,
								IMG_HANDLE hHWPBBlockClientMemInfoHandle,
								IMG_HANDLE *phSharedPBDesc,
								IMG_HANDLE *phSubKernelMemInfos,
#endif
								IMG_UINT32 ui32SubKernelMemInfosCount,
								IMG_DEV_VIRTADDR sHWPBDescDevVAddr)
{
	PVRSRV_BRIDGE_IN_SGXADDSHAREDPBDESC sBridgeIn;
	PVRSRV_BRIDGE_OUT_SGXADDSHAREDPBDESC sBridgeOut;

	PVR_DPF((PVR_DBG_VERBOSE, "SGXAddSharedPBDesc"));

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

	if(!psDevData || !hSharedPBDescKernelMemInfoHandle)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXAddSharedPBDesc: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hDevCookie = psDevData->hDevCookie;
	sBridgeIn.hSharedPBDescKernelMemInfo = hSharedPBDescKernelMemInfoHandle;
	sBridgeIn.hHWPBDescKernelMemInfo = hHWPBDescKernelMemInfoHandle;
	sBridgeIn.hBlockKernelMemInfo = hPBBlockClientMemInfoHandle;
	sBridgeIn.hHWBlockKernelMemInfo = hHWPBBlockClientMemInfoHandle;
	sBridgeIn.ui32TotalPBSize = ui32TotalPBSize;

	sBridgeIn.phKernelMemInfoHandles = phSubKernelMemInfos;
	sBridgeIn.ui32KernelMemInfoHandlesCount = ui32SubKernelMemInfosCount;
	sBridgeIn.sHWPBDescDevVAddr = sHWPBDescDevVAddr;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_SGX_ADDSHAREDPBDESC,
						&sBridgeIn,
						sizeof(PVRSRV_BRIDGE_IN_SGXADDSHAREDPBDESC),
						&sBridgeOut,
						sizeof(PVRSRV_BRIDGE_OUT_SGXADDSHAREDPBDESC)))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXAddSharedPBDesc: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sBridgeOut.eError == PVRSRV_OK)
	{
		*phSharedPBDesc = sBridgeOut.hSharedPBDesc;
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXAddSharedPBDesc: KM failed %d",sBridgeOut.eError));
	}

	return sBridgeOut.eError;
}
#endif /* !defined(OS_SGX_ADD_SHARED_PBDESC_UM) */

#if defined(TRANSFER_QUEUE)
#if !defined(OS_SGX_SUBMIT_TRANSFER)

/******************************************************************************
 @Function	SGXSubmitTransferBridge

 @Description	Submits a TQ transfer op to SGX

 @Input		psDevData
		psKick

 @Return	PVRSRV_ERROR
******************************************************************************/
IMG_INTERNAL
PVRSRV_ERROR SGXSubmitTransferBridge(const PVRSRV_DEV_DATA *psDevData, PVRSRV_TRANSFER_SGX_KICK *psKick)
{
	PVRSRV_BRIDGE_IN_SUBMITTRANSFER sBridgeIn;
	PVRSRV_BRIDGE_RETURN sBridgeOut;


	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));
	
	PVR_DPF((PVR_DBG_VERBOSE, "SGXSubmitTransfer"));

	if (!psDevData || !psKick)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXSubmitTransfer: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	do
	{
		sBridgeIn.hDevCookie = psDevData->hDevCookie;
		sBridgeIn.sKick = *psKick;

		if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
							PVRSRV_BRIDGE_SGX_SUBMITTRANSFER,
							&sBridgeIn,
							sizeof(PVRSRV_BRIDGE_IN_SUBMITTRANSFER),
							&sBridgeOut,
							sizeof(PVRSRV_BRIDGE_RETURN)))
		{
			PVR_DPF((PVR_DBG_ERROR, "SGXSubmitTransfer: BridgeCall failed"));
			return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
		}
	} while (sBridgeOut.eError == PVRSRV_ERROR_RETRY);
	
	return sBridgeOut.eError;
}
#endif /* OS_SGX_SUBMIT_TRANSFER */

#if defined(SGX_FEATURE_2D_HARDWARE) && !defined(OS_SGX_SUBMIT_2D)
/******************************************************************************
 @Function	SGXSubmit2D

 @Description	Submits a 2D transfer op to SGX

 @Input		psDevData
		psKick

 @Return	PVRSRV_ERROR
******************************************************************************/
IMG_INTERNAL
PVRSRV_ERROR SGXSubmit2D(const PVRSRV_DEV_DATA *psDevData, PVRSRV_2D_SGX_KICK *psKick)
{
	PVRSRV_BRIDGE_IN_SUBMIT2D sBridgeIn;
	PVRSRV_BRIDGE_RETURN sBridgeOut;


	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

	PVR_DPF((PVR_DBG_VERBOSE, "SGXSubmit2D"));

	if (!psDevData || !psKick)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXSubmit2D: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	do
	{
		sBridgeIn.hDevCookie = psDevData->hDevCookie;
		sBridgeIn.sKick = *psKick;

		if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
					PVRSRV_BRIDGE_SGX_SUBMIT2D,
					&sBridgeIn,
					sizeof(PVRSRV_BRIDGE_IN_SUBMIT2D),
					&sBridgeOut,
					sizeof(PVRSRV_BRIDGE_RETURN)))
		{
			PVR_DPF((PVR_DBG_ERROR, "SGXSubmit2D: BridgeCall failed"));
			return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
		}

	} while (sBridgeOut.eError == PVRSRV_ERROR_RETRY);

	return sBridgeOut.eError;
}
#endif
#endif /* TRANSFER_QUEUE */

IMG_INTERNAL
#if defined (SUPPORT_SID_INTERFACE)
PVRSRV_ERROR SGXRegisterHWRenderContext(const PVRSRV_DEV_DATA *psDevData, IMG_SID *phHWRenderContext, 
                                        IMG_CPU_VIRTADDR *psHWRenderContextCpuVAddr,
                                        IMG_UINT32 ui32HWRenderContextSize,
                                        IMG_UINT32 ui32OffsetToPDDevPAddr,
                                        IMG_HANDLE hDevMemContext,
                                        IMG_DEV_VIRTADDR *psHWRenderContextDevVAddrOut)
#else
PVRSRV_ERROR SGXRegisterHWRenderContext(const PVRSRV_DEV_DATA *psDevData, IMG_HANDLE *phHWRenderContext, 
                                        IMG_CPU_VIRTADDR *psHWRenderContextCpuVAddr,
                                        IMG_UINT32 ui32HWRenderContextSize,
                                        IMG_UINT32 ui32OffsetToPDDevPAddr,
                                        IMG_HANDLE hDevMemContext,
                                        IMG_DEV_VIRTADDR *psHWRenderContextDevVAddrOut)
#endif
{
	PVRSRV_BRIDGE_IN_SGX_REGISTER_HW_RENDER_CONTEXT sBridgeIn;
	PVRSRV_BRIDGE_OUT_SGX_REGISTER_HW_RENDER_CONTEXT sBridgeOut;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

	PVR_DPF((PVR_DBG_VERBOSE, "SGXRegisterHWRenderContext"));

	if (!psDevData || !phHWRenderContext || !psHWRenderContextCpuVAddr)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHWRenderContext: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hDevCookie = psDevData->hDevCookie;
    sBridgeIn.pHWRenderContextCpuVAddr = psHWRenderContextCpuVAddr;
    sBridgeIn.ui32HWRenderContextSize = ui32HWRenderContextSize;
    sBridgeIn.ui32OffsetToPDDevPAddr = ui32OffsetToPDDevPAddr;
    sBridgeIn.hDevMemContext = hDevMemContext;

	if (PVRSRVBridgeCall(psDevData->psConnection->hServices,
						 PVRSRV_BRIDGE_SGX_REGISTER_HW_RENDER_CONTEXT,
						 &sBridgeIn,
						 sizeof(PVRSRV_BRIDGE_IN_SGX_REGISTER_HW_RENDER_CONTEXT),
						 &sBridgeOut,
						 sizeof(PVRSRV_BRIDGE_OUT_SGX_REGISTER_HW_RENDER_CONTEXT)))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHWRenderContext: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if (sBridgeOut.eError == PVRSRV_OK)
	{
		*phHWRenderContext = sBridgeOut.hHWRenderContext;
        psHWRenderContextDevVAddrOut->uiAddr = sBridgeOut.sHWRenderContextDevVAddr.uiAddr;
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHWRenderContext: KM failed %d",sBridgeOut.eError));
	}

	return sBridgeOut.eError;
}
IMG_INTERNAL
#if defined (SUPPORT_SID_INTERFACE)
PVRSRV_ERROR SGXUnregisterHWRenderContext(const PVRSRV_DEV_DATA *psDevData,
										  IMG_SID hHWRenderContext,
										  IMG_BOOL bForceCleanup)
#else
PVRSRV_ERROR SGXUnregisterHWRenderContext(const PVRSRV_DEV_DATA *psDevData,
										  IMG_HANDLE hHWRenderContext,
										  IMG_BOOL bForceCleanup)
#endif
{
	PVRSRV_BRIDGE_IN_SGX_UNREGISTER_HW_RENDER_CONTEXT sBridgeIn;
	PVRSRV_BRIDGE_RETURN sBridgeOut;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

	PVR_DPF((PVR_DBG_VERBOSE, "SGXUnregisterHWRenderContext"));

	if (!psDevData || !hHWRenderContext)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXUnregisterHWRenderContext: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hDevCookie = psDevData->hDevCookie;
	sBridgeIn.hHWRenderContext = hHWRenderContext;
	sBridgeIn.bForceCleanup = bForceCleanup;

	if (PVRSRVBridgeCall(psDevData->psConnection->hServices,
						 PVRSRV_BRIDGE_SGX_UNREGISTER_HW_RENDER_CONTEXT,
						 &sBridgeIn,
						 sizeof(PVRSRV_BRIDGE_IN_SGX_UNREGISTER_HW_RENDER_CONTEXT),
						 &sBridgeOut,
						 sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXUnregisterHWRenderContext: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sBridgeOut.eError;
}

IMG_INTERNAL
#if defined (SUPPORT_SID_INTERFACE)
PVRSRV_ERROR SGXRegisterHWTransferContext(const PVRSRV_DEV_DATA *psDevData, IMG_SID *phHWTransferContext, 
                                          IMG_CPU_VIRTADDR *psHWTransferContextCpuVAddr,
                                          IMG_UINT32 ui32HWTransferContextSize,
                                          IMG_UINT32 ui32OffsetToPDDevPAddr,
                                          IMG_HANDLE hDevMemContext,
                                          IMG_DEV_VIRTADDR *psHWTransferContextDevVAddrOut)
#else
PVRSRV_ERROR SGXRegisterHWTransferContext(const PVRSRV_DEV_DATA *psDevData, IMG_HANDLE *phHWTransferContext, 
                                          IMG_CPU_VIRTADDR *psHWTransferContextCpuVAddr,
                                          IMG_UINT32 ui32HWTransferContextSize,
                                          IMG_UINT32 ui32OffsetToPDDevPAddr,
                                          IMG_HANDLE hDevMemContext,
                                          IMG_DEV_VIRTADDR *psHWTransferContextDevVAddrOut)
#endif
{
	PVRSRV_BRIDGE_IN_SGX_REGISTER_HW_TRANSFER_CONTEXT sBridgeIn;
	PVRSRV_BRIDGE_OUT_SGX_REGISTER_HW_TRANSFER_CONTEXT sBridgeOut;


	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

	
	PVR_DPF((PVR_DBG_VERBOSE, "SGXRegisterHWTransferContext"));

	if (!psDevData || !phHWTransferContext || !psHWTransferContextCpuVAddr)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHWTransferContext: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hDevCookie = psDevData->hDevCookie;
    sBridgeIn.pHWTransferContextCpuVAddr = psHWTransferContextCpuVAddr;
    sBridgeIn.ui32HWTransferContextSize = ui32HWTransferContextSize;
    sBridgeIn.ui32OffsetToPDDevPAddr = ui32OffsetToPDDevPAddr;
    sBridgeIn.hDevMemContext = hDevMemContext;

	if (PVRSRVBridgeCall(psDevData->psConnection->hServices,
						 PVRSRV_BRIDGE_SGX_REGISTER_HW_TRANSFER_CONTEXT,
						 &sBridgeIn,
						 sizeof(PVRSRV_BRIDGE_IN_SGX_REGISTER_HW_TRANSFER_CONTEXT),
						 &sBridgeOut,
						 sizeof(PVRSRV_BRIDGE_OUT_SGX_REGISTER_HW_TRANSFER_CONTEXT)))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHWRenderContext: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if (sBridgeOut.eError == PVRSRV_OK)
	{
		*phHWTransferContext = sBridgeOut.hHWTransferContext;
        psHWTransferContextDevVAddrOut->uiAddr = sBridgeOut.sHWTransferContextDevVAddr.uiAddr;
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHWTransferContext: KM failed %d",sBridgeOut.eError));
	}

	return sBridgeOut.eError;
}
IMG_INTERNAL
#if defined (SUPPORT_SID_INTERFACE)
PVRSRV_ERROR SGXUnregisterHWTransferContext(const PVRSRV_DEV_DATA *psDevData, IMG_SID hHWTransferContext, IMG_BOOL bForceCleanup)
#else
PVRSRV_ERROR SGXUnregisterHWTransferContext(const PVRSRV_DEV_DATA *psDevData, IMG_HANDLE hHWTransferContext, IMG_BOOL bForceCleanup)
#endif
{
	PVRSRV_BRIDGE_IN_SGX_UNREGISTER_HW_TRANSFER_CONTEXT sBridgeIn;
	PVRSRV_BRIDGE_RETURN sBridgeOut;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

	PVR_DPF((PVR_DBG_VERBOSE, "SGXUnregisterHWTransferContext"));

	if (!psDevData || !hHWTransferContext)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXUnregisterHWTransferContext: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hDevCookie = psDevData->hDevCookie;
	sBridgeIn.hHWTransferContext = hHWTransferContext;
	sBridgeIn.bForceCleanup = bForceCleanup;

	if (PVRSRVBridgeCall(psDevData->psConnection->hServices,
						 PVRSRV_BRIDGE_SGX_UNREGISTER_HW_TRANSFER_CONTEXT,
						 &sBridgeIn,
						 sizeof(PVRSRV_BRIDGE_IN_SGX_UNREGISTER_HW_TRANSFER_CONTEXT),
						 &sBridgeOut,
						 sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXUnregisterHWTransferContext: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sBridgeOut.eError;
}

IMG_INTERNAL
#if defined (SUPPORT_SID_INTERFACE)
PVRSRV_ERROR SGXSetTransferContextPriority(const PVRSRV_DEV_DATA *psDevData, 
                                   IMG_SID hHWTransferContext, 
                                   IMG_UINT32 ui32Priority,
                                   IMG_UINT32 ui32OffsetOfPriorityField)
#else
PVRSRV_ERROR SGXSetTransferContextPriority(const PVRSRV_DEV_DATA *psDevData, 
                                   IMG_HANDLE hHWTransferContext, 
                                   IMG_UINT32 ui32Priority,
                                   IMG_UINT32 ui32OffsetOfPriorityField)
#endif
{
    PVRSRV_BRIDGE_IN_SGX_SET_TRANSFER_CONTEXT_PRIORITY sBridgeIn;
	PVRSRV_BRIDGE_RETURN sBridgeOut;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

    PVR_DPF((PVR_DBG_VERBOSE, "SGXSetTransferContextPriority"));

    if ((IMG_NULL == psDevData) 
    ||  (IMG_NULL == hHWTransferContext))
    {
        PVR_DPF((PVR_DBG_ERROR, "SGXSetTransferContextPriority: Invalid params"));
        return PVRSRV_ERROR_INVALID_PARAMS;
    }

    sBridgeIn.hDevCookie = psDevData->hDevCookie;
    sBridgeIn.hHWTransferContext = hHWTransferContext;
    sBridgeIn.ui32Priority = ui32Priority;
    sBridgeIn.ui32OffsetOfPriorityField = ui32OffsetOfPriorityField;

    if (PVRSRVBridgeCall(psDevData->psConnection->hServices,
                PVRSRV_BRIDGE_SGX_SET_TRANSFER_CONTEXT_PRIORITY,
                &sBridgeIn,
                sizeof (PVRSRV_BRIDGE_IN_SGX_SET_TRANSFER_CONTEXT_PRIORITY),
                &sBridgeOut,
                sizeof(PVRSRV_BRIDGE_RETURN)))
    {
        PVR_DPF((PVR_DBG_ERROR, "SGXSetTransferContextPriority: Bridge Call failed"));
        return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
    }

    return sBridgeOut.eError;
}
    
IMG_INTERNAL
#if defined (SUPPORT_SID_INTERFACE)
PVRSRV_ERROR SGXSetRenderContextPriority(const PVRSRV_DEV_DATA *psDevData, 
                                   IMG_SID hHWRenderContext, 
                                   IMG_UINT32 ui32Priority,
                                   IMG_UINT32 ui32OffsetOfPriorityField)
#else
PVRSRV_ERROR SGXSetRenderContextPriority(const PVRSRV_DEV_DATA *psDevData, 
                                   IMG_HANDLE hHWRenderContext, 
                                   IMG_UINT32 ui32Priority,
                                   IMG_UINT32 ui32OffsetOfPriorityField)
#endif
{
    PVRSRV_BRIDGE_IN_SGX_SET_RENDER_CONTEXT_PRIORITY sBridgeIn;
	PVRSRV_BRIDGE_RETURN sBridgeOut;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

    PVR_DPF((PVR_DBG_VERBOSE, "SGXSetRenderContextPriority"));

    if ((IMG_NULL == psDevData) 
    ||  (IMG_NULL == hHWRenderContext))
    {
        PVR_DPF((PVR_DBG_ERROR, "SGXSetRenderContextPriority: Invalid params"));
        return PVRSRV_ERROR_INVALID_PARAMS;
    }

    sBridgeIn.hDevCookie = psDevData->hDevCookie;
    sBridgeIn.hHWRenderContext = hHWRenderContext;
    sBridgeIn.ui32Priority = ui32Priority;
    sBridgeIn.ui32OffsetOfPriorityField = ui32OffsetOfPriorityField;

    if (PVRSRVBridgeCall(psDevData->psConnection->hServices,
                PVRSRV_BRIDGE_SGX_SET_RENDER_CONTEXT_PRIORITY,
                &sBridgeIn,
                sizeof (PVRSRV_BRIDGE_IN_SGX_SET_RENDER_CONTEXT_PRIORITY),
                &sBridgeOut,
                sizeof(PVRSRV_BRIDGE_RETURN)))
    {
        PVR_DPF((PVR_DBG_ERROR, "SGXSetRenderContextPriority: Bridge Call failed"));
        return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
    }

    return sBridgeOut.eError;
}


#if defined(SGX_FEATURE_2D_HARDWARE)
IMG_INTERNAL
#if defined (SUPPORT_SID_INTERFACE)
PVRSRV_ERROR SGXRegisterHW2DContext(const PVRSRV_DEV_DATA *psDevData, IMG_SID *phHW2DContext, 
                                    IMG_CPU_VIRTADDR *psHW2DContextCpuVAddr,
                                    IMG_UINT32 ui32HW2DContextSize,
                                    IMG_UINT32 ui32OffsetToPDDevPAddr,
                                    IMG_HANDLE hDevMemContext,
                                    IMG_DEV_VIRTADDR *psHW2DContextDevVAddrOut)
#else
PVRSRV_ERROR SGXRegisterHW2DContext(const PVRSRV_DEV_DATA *psDevData, IMG_HANDLE *phHW2DContext, 
                                    IMG_CPU_VIRTADDR *psHW2DContextCpuVAddr,
                                    IMG_UINT32 ui32HW2DContextSize,
                                    IMG_UINT32 ui32OffsetToPDDevPAddr,
                                    IMG_HANDLE hDevMemContext,
                                    IMG_DEV_VIRTADDR *psHW2DContextDevVAddrOut)
#endif
{
	PVRSRV_BRIDGE_IN_SGX_REGISTER_HW_2D_CONTEXT sBridgeIn;
	PVRSRV_BRIDGE_OUT_SGX_REGISTER_HW_2D_CONTEXT sBridgeOut;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

	PVR_DPF((PVR_DBG_VERBOSE, "SGXRegisterHW2DContext"));

	if (!psDevData || !phHW2DContext || !psHW2DContextCpuVAddr)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHW2DContext: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hDevCookie = psDevData->hDevCookie;
    sBridgeIn.pHW2DContextCpuVAddr = psHW2DContextCpuVAddr;
    sBridgeIn.ui32HW2DContextSize = ui32HW2DContextSize;
    sBridgeIn.ui32OffsetToPDDevPAddr = ui32OffsetToPDDevPAddr;
    sBridgeIn.hDevMemContext = hDevMemContext;

	if (PVRSRVBridgeCall(psDevData->psConnection->hServices,
						 PVRSRV_BRIDGE_SGX_REGISTER_HW_2D_CONTEXT,
						 &sBridgeIn,
						 sizeof(PVRSRV_BRIDGE_IN_SGX_REGISTER_HW_2D_CONTEXT),
						 &sBridgeOut,
						 sizeof(PVRSRV_BRIDGE_OUT_SGX_REGISTER_HW_2D_CONTEXT)))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHW2DContext: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if (sBridgeOut.eError == PVRSRV_OK)
	{
		*phHW2DContext = sBridgeOut.hHW2DContext;
        psHW2DContextDevVAddrOut->uiAddr = sBridgeOut.sHW2DContextDevVAddr.uiAddr;
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHW2DContext: KM failed %d",sBridgeOut.eError));
	}

	return sBridgeOut.eError;
}

IMG_INTERNAL
#if defined (SUPPORT_SID_INTERFACE)
PVRSRV_ERROR SGXUnregisterHW2DContext(const PVRSRV_DEV_DATA *psDevData,
									  IMG_SID hHW2DContexts,
									  IMG_BOOL bForceCleanup)
#else
PVRSRV_ERROR SGXUnregisterHW2DContext(const PVRSRV_DEV_DATA *psDevData,
									  IMG_HANDLE hHW2DContext,
									  IMG_BOOL bForceCleanup)
#endif
{
	PVRSRV_BRIDGE_IN_SGX_UNREGISTER_HW_2D_CONTEXT sBridgeIn;
	PVRSRV_BRIDGE_RETURN sBridgeOut;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

	PVR_DPF((PVR_DBG_VERBOSE, "SGXUnregisterHW2DContext"));

	if (!psDevData || !hHW2DContext)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXUnregisterHW2DContext: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hDevCookie = psDevData->hDevCookie;
	sBridgeIn.hHW2DContext = hHW2DContext;
	sBridgeIn.bForceCleanup = bForceCleanup;

	if (PVRSRVBridgeCall(psDevData->psConnection->hServices,
						 PVRSRV_BRIDGE_SGX_UNREGISTER_HW_2D_CONTEXT,
						 &sBridgeIn,
						 sizeof(PVRSRV_BRIDGE_IN_SGX_UNREGISTER_HW_2D_CONTEXT),
						 &sBridgeOut,
						 sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXUnregisterHW2DContext: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sBridgeOut.eError;
}
#endif
IMG_INTERNAL
PVRSRV_ERROR SGXFlushHWRenderTarget(const PVRSRV_DEV_DATA	*psDevData,
									PVRSRV_CLIENT_MEM_INFO	*psHWRTDataSetMemInfo)
{
	/* override QAC suggestion and keep initialisation */
	PVRSRV_ERROR eError = PVRSRV_OK;	/* PRQA S 3197 */

	if (!psDevData || !psHWRTDataSetMemInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXFlushHWRenderTarget: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/*
		This could be optimised by checking the status of each psHWRTData, and
		skipping the kernel call to do the uKernel synchronisation in the case
		that no psHWRTData is in use.
		However, this would cause pdump complications.
		In the unlikely event that the overhead of this call is considered too
		large, doing this might be a consideration.
	*/
	{
		PVRSRV_BRIDGE_IN_SGX_FLUSH_HW_RENDER_TARGET sBridgeIn;
		PVRSRV_BRIDGE_RETURN sBridgeOut;

		PVR_DPF((PVR_DBG_VERBOSE, "SGXFlushHWRenderTarget"));

		sBridgeIn.hDevCookie = psDevData->hDevCookie;
		sBridgeIn.sHWRTDataSetDevVAddr = psHWRTDataSetMemInfo->sDevVAddr;

		if (PVRSRVBridgeCall(psDevData->psConnection->hServices,
							 PVRSRV_BRIDGE_SGX_FLUSH_HW_RENDER_TARGET,
							 &sBridgeIn,
							 sizeof(PVRSRV_BRIDGE_IN_SGX_FLUSH_HW_RENDER_TARGET),
							 &sBridgeOut,
							 sizeof(PVRSRV_BRIDGE_RETURN)))
		{
			PVR_DPF((PVR_DBG_ERROR, "SGXFlushHWRenderTarget: BridgeCall failed"));
			return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
		}

		eError = sBridgeOut.eError;
	}

	return eError;
}

/******************************************************************************
 @Function	SGXDoKick

 @Description	Kicks TA

 @Input		psDevData
 @Input		psCCBKick

 @Return	PVRSRV_ERROR
******************************************************************************/
IMG_INTERNAL
PVRSRV_ERROR SGXDoKick(PVRSRV_DEV_DATA *psDevData, SGX_CCB_KICK *psCCBKick)
{
	PVRSRV_BRIDGE_IN_DOKICK sBridgeIn;
	PVRSRV_BRIDGE_RETURN sBridgeOut;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

	PVR_DPF((PVR_DBG_VERBOSE, "SGXDoKick"));

	if (!psDevData || !psCCBKick)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXDoKick: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	do
	{
		sBridgeIn.hDevCookie = psDevData->hDevCookie;
		sBridgeIn.sCCBKick = *psCCBKick;

		if (PVRSRVBridgeCall(psDevData->psConnection->hServices,
							 PVRSRV_BRIDGE_SGX_DOKICK,
							 &sBridgeIn,
							 sizeof(PVRSRV_BRIDGE_IN_DOKICK),
							 &sBridgeOut,
							 sizeof(PVRSRV_BRIDGE_RETURN)))
		{
			PVR_DPF((PVR_DBG_ERROR, "SGXDoKick: BridgeCall failed"));
			return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
		}
	} while (sBridgeOut.eError == PVRSRV_ERROR_RETRY);

	return sBridgeOut.eError;
}



/******************************************************************************
 @Function	SGXScheduleProcessQueues

 @Description	schedules the microkernel

 @Input		psDevData

 @Return	PVRSRV_ERROR
******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR SGXScheduleProcessQueues(PVRSRV_DEV_DATA *psDevData)
{
	PVRSRV_BRIDGE_IN_SGX_SCHEDULE_PROCESS_QUEUES sBridgeIn;
	PVRSRV_BRIDGE_RETURN sBridgeOut;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

	PVR_DPF((PVR_DBG_VERBOSE, "SGXScheduleProcessQueues"));

	if (!psDevData)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXScheduleProcessQueues: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hDevCookie = psDevData->hDevCookie;

	if (PVRSRVBridgeCall(psDevData->psConnection->hServices,
						 PVRSRV_BRIDGE_SGX_SCHEDULE_PROCESS_QUEUES,
						 &sBridgeIn,
						 sizeof(PVRSRV_BRIDGE_IN_SGX_SCHEDULE_PROCESS_QUEUES),
						 &sBridgeOut,
						 sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXScheduleProcessQueues: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sBridgeOut.eError;
}


/*!
*******************************************************************************

 @Function	SGXGetInfoForSrvInit

 @Description	Get SGX related info needed for the initialisation server

 @Input		psDevData

 @Modified	psInitInfo

 @Return	PVRSRV_ERROR :

******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR SGXGetInfoForSrvInit(PVRSRV_DEV_DATA *psDevData,
								  SGX_BRIDGE_INFO_FOR_SRVINIT *psInitInfo)
{
	PVRSRV_BRIDGE_IN_SGXINFO_FOR_SRVINIT sBridgeIn;
	PVRSRV_BRIDGE_OUT_SGXINFO_FOR_SRVINIT sBridgeOut;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

	PVR_DPF((PVR_DBG_VERBOSE, "SGXGetInfoForSrvInit"));

	if (!psInitInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXGetInfoForSrvInit: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hDevCookie = psDevData->hDevCookie;

	if (PVRSRVBridgeCall(psDevData->psConnection->hServices,
						 PVRSRV_BRIDGE_SGXINFO_FOR_SRVINIT,
						 &sBridgeIn,
						 sizeof(PVRSRV_BRIDGE_IN_SGXINFO_FOR_SRVINIT),
						 &sBridgeOut,
						 sizeof(PVRSRV_BRIDGE_OUT_SGXINFO_FOR_SRVINIT)))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXGetInfoForSrvInit: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if (sBridgeOut.eError == PVRSRV_OK)
	{
		*psInitInfo = sBridgeOut.sInitInfo;
	}

	return sBridgeOut.eError;
}


/*!
*******************************************************************************

 @Function	SGXDevInitPart2

 @Description	Triggers second part of SGX initialisation

 @Input		psDevData, psServerInitInfo

 @Return	PVRSRV_ERROR :

******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR SGXDevInitPart2(PVRSRV_DEV_DATA *psDevData, SGX_CLIENT_INIT_INFO *psServerInitInfo)
{
	PVRSRV_BRIDGE_IN_SGXDEVINITPART2 *psBridgeIn;
	PVRSRV_BRIDGE_OUT_SGXDEVINITPART2 sBridgeOut;
	SGX_BRIDGE_INIT_INFO *psInitInfo;

	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));


	PVR_DPF((PVR_DBG_VERBOSE, "SGXDevInitPart2"));

	if (!psDevData || !psServerInitInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXDevInitPart2: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* This is too big to put on the stack on some OSs */
	psBridgeIn = PVRSRVCallocUserModeMem(sizeof(*psBridgeIn));
	if (psBridgeIn == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXDevInitPart2: BridgeIn alloc failed"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psBridgeIn->hDevCookie = psDevData->hDevCookie;
	psInitInfo = &psBridgeIn->sInitInfo;

	psInitInfo->hKernelCCBMemInfo = psServerInitInfo->psKernelCCBMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psKernelCCBMemInfo);

	psInitInfo->hKernelCCBCtlMemInfo = psServerInitInfo->psKernelCCBCtlMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psKernelCCBCtlMemInfo);

	psInitInfo->hKernelCCBEventKickerMemInfo = psServerInitInfo->psKernelCCBEventKickerMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psKernelCCBEventKickerMemInfo);

	psInitInfo->hKernelSGXHostCtlMemInfo = psServerInitInfo->psHostCtlMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psHostCtlMemInfo);

	psInitInfo->hKernelSGXTA3DCtlMemInfo = psServerInitInfo->psTA3DCtlMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psTA3DCtlMemInfo);

#if defined(FIX_HW_BRN_31272) || defined(FIX_HW_BRN_31780) || defined(FIX_HW_BRN_33920)
	psInitInfo->hKernelSGXPTLAWriteBackMemInfo = psServerInitInfo->psPTLAWriteBackMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psPTLAWriteBackMemInfo);
#endif

	/* Unref the user space mapping for the SGXGetMiscInfo buffer */
	psInitInfo->hKernelSGXMiscMemInfo = psServerInitInfo->psKernelSGXMiscMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psKernelSGXMiscMemInfo);

	/* Pass down the client-side build options and structure sizes for check during init */
	psInitInfo->ui32ClientBuildOptions = psServerInitInfo->ui32ClientBuildOptions;
	psInitInfo->sSGXStructSizes = psServerInitInfo->sSGXStructSizes;

#if defined(SGX_SUPPORT_HWPROFILING)
	psInitInfo->hKernelHWProfilingMemInfo = psServerInitInfo->psHWProfilingMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psHWProfilingMemInfo);
#endif

#if defined(SUPPORT_SGX_HWPERF)
	psInitInfo->hKernelHWPerfCBMemInfo = psServerInitInfo->psHWPerfCBMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psHWPerfCBMemInfo);
#endif

	psInitInfo->hKernelTASigBufferMemInfo = psServerInitInfo->psTASigBufferMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psTASigBufferMemInfo);
	psInitInfo->hKernel3DSigBufferMemInfo = psServerInitInfo->ps3DSigBufferMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->ps3DSigBufferMemInfo);

#if defined(FIX_HW_BRN_29702)
	psInitInfo->hKernelCFIMemInfo = psServerInitInfo->psCFIMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psCFIMemInfo);
#endif

#if defined(FIX_HW_BRN_29823)
	psInitInfo->hKernelDummyTermStreamMemInfo = psServerInitInfo->psDummyTermStreamMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psDummyTermStreamMemInfo);
#endif

 
#if defined(FIX_HW_BRN_31542) || defined(FIX_HW_BRN_36513)
	psInitInfo->hKernelClearClipWAVDMStreamMemInfo = psServerInitInfo->psClearClipWAVDMStreamMemInfo->hKernelMemInfo ;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psClearClipWAVDMStreamMemInfo);
	psInitInfo->hKernelClearClipWAIndexStreamMemInfo = psServerInitInfo->psClearClipWAIndexStreamMemInfo->hKernelMemInfo ;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psClearClipWAIndexStreamMemInfo);
	psInitInfo->hKernelClearClipWAPDSMemInfo = psServerInitInfo->psClearClipWAPDSMemInfo->hKernelMemInfo ;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psClearClipWAPDSMemInfo);
	psInitInfo->hKernelClearClipWAUSEMemInfo = psServerInitInfo->psClearClipWAUSEMemInfo->hKernelMemInfo ;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psClearClipWAUSEMemInfo);
	psInitInfo->hKernelClearClipWAParamMemInfo = psServerInitInfo->psClearClipWAParamMemInfo->hKernelMemInfo ;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psClearClipWAParamMemInfo);
	psInitInfo->hKernelClearClipWAPMPTMemInfo = psServerInitInfo->psClearClipWAPMPTMemInfo->hKernelMemInfo ;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psClearClipWAPMPTMemInfo);
	psInitInfo->hKernelClearClipWATPCMemInfo = psServerInitInfo->psClearClipWATPCMemInfo->hKernelMemInfo ;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psClearClipWATPCMemInfo);
	psInitInfo->hKernelClearClipWAPSGRgnHdrMemInfo = psServerInitInfo->psClearClipWAPSGRgnHdrMemInfo->hKernelMemInfo ;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psClearClipWAPSGRgnHdrMemInfo);
#endif

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) && defined(FIX_HW_BRN_31425)
	psInitInfo->hKernelVDMSnapShotBufferMemInfo = psServerInitInfo->psVDMSnapShotBufferMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psVDMSnapShotBufferMemInfo);
	psInitInfo->hKernelVDMCtrlStreamBufferMemInfo = psServerInitInfo->psVDMCtrlStreamBufferMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psVDMCtrlStreamBufferMemInfo);
#endif
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) && \
	defined(FIX_HW_BRN_33657) && defined(SUPPORT_SECURE_33657_FIX)
	psInitInfo->hKernelVDMStateUpdateBufferMemInfo = psServerInitInfo->psVDMStateUpdateBufferMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psVDMStateUpdateBufferMemInfo);
#endif

#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG)
	psInitInfo->hKernelEDMStatusBufferMemInfo = psServerInitInfo->psEDMStatusBufferMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psEDMStatusBufferMemInfo);
#endif


	psInitInfo->asInitMemHandles[SGX_INIT_MEM_USE] = psServerInitInfo->psMicrokernelUSEMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psMicrokernelUSEMemInfo);
#if !defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
	psInitInfo->asInitMemHandles[SGX_INIT_MEM_TA_USE] = psServerInitInfo->psMicrokernelTAUSEMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psMicrokernelTAUSEMemInfo);

	psInitInfo->asInitMemHandles[SGX_INIT_MEM_3D_USE] = psServerInitInfo->psMicrokernel3DUSEMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psMicrokernel3DUSEMemInfo);

	psInitInfo->asInitMemHandles[SGX_INIT_MEM_SPM_USE] = psServerInitInfo->psMicrokernelSPMUSEMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psMicrokernelSPMUSEMemInfo);
#if defined(SGX_FEATURE_2D_HARDWARE)
	psInitInfo->asInitMemHandles[SGX_INIT_MEM_2D_USE] = psServerInitInfo->psMicrokernel2DUSEMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psMicrokernel2DUSEMemInfo);
#endif
#endif /* SGX_FEATURE_MONOLITHIC_UKERNEL */

#if defined(SGX_FEATURE_MP)
	psInitInfo->asInitMemHandles[SGX_INIT_MEM_SLAVE_USE] = psServerInitInfo->psMicrokernelSlaveUSEMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psMicrokernelSlaveUSEMemInfo);

	psInitInfo->asInitMemHandles[SGX_INIT_MEM_SLAVE_PDS] = psServerInitInfo->psMicrokernelPDSEventSlaveMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psMicrokernelPDSEventSlaveMemInfo);
#endif

	psInitInfo->asInitMemHandles[SGX_INIT_MEM_PDS_INIT_PRIM1] = psServerInitInfo->psMicrokernelPDSInitPrimary1MemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psMicrokernelPDSInitPrimary1MemInfo);

	psInitInfo->asInitMemHandles[SGX_INIT_MEM_PDS_INIT_SEC] = psServerInitInfo->psMicrokernelPDSInitSecondaryMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psMicrokernelPDSInitSecondaryMemInfo);

	psInitInfo->asInitMemHandles[SGX_INIT_MEM_PDS_INIT_PRIM2] = psServerInitInfo->psMicrokernelPDSInitPrimary2MemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psMicrokernelPDSInitPrimary2MemInfo);

	psInitInfo->asInitMemHandles[SGX_INIT_MEM_PDS_EVENT] = psServerInitInfo->psMicrokernelPDSEventMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psMicrokernelPDSEventMemInfo);

	psInitInfo->asInitMemHandles[SGX_INIT_MEM_PDS_LOOPBACK] = psServerInitInfo->psMicrokernelPDSLoopbackMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psMicrokernelPDSLoopbackMemInfo);

	PVRSRVMemCopy(psInitInfo->aui32HostKickAddr, psServerInitInfo->aui32HostKickAddr,
				  SGXMKIF_CMD_MAX * sizeof(psInitInfo->aui32HostKickAddr[0]));

	psInitInfo->ui32CacheControl = psServerInitInfo->ui32CacheControl;

	psInitInfo->asInitDevData[SGX_DEV_DATA_EVM_CONFIG] = psServerInitInfo->ui32EVMConfig;

	psInitInfo->asInitDevData[SGX_DEV_DATA_PDS_EXEC_BASE] = psServerInitInfo->sDevVAddrPDSExecBase.uiAddr;
	psInitInfo->asInitDevData[SGX_DEV_DATA_USE_EXEC_BASE] = psServerInitInfo->sDevVAddrUSEExecBase.uiAddr;

	psInitInfo->asInitDevData[SGX_DEV_DATA_NUM_USE_ATTRIBUTE_REGS] = psServerInitInfo->ui32NumUSEAttributeRegisters;
	psInitInfo->asInitDevData[SGX_DEV_DATA_NUM_USE_TEMP_REGS] = psServerInitInfo->ui32NumUSETemporaryRegisters;

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	psInitInfo->asInitMemHandles[SGX_INIT_MEM_USE_CTX_SWITCH_TERM] = psServerInitInfo->psUSECtxSwitchTermMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psUSECtxSwitchTermMemInfo);
	psInitInfo->asInitMemHandles[SGX_INIT_MEM_PDS_CTX_SWITCH_TERM] = psServerInitInfo->psPDSCtxSwitchTermMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psPDSCtxSwitchTermMemInfo);

	psInitInfo->asInitMemHandles[SGX_INIT_MEM_USE_TA_STATE] = psServerInitInfo->psUSETAStateMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psUSETAStateMemInfo);
	psInitInfo->asInitMemHandles[SGX_INIT_MEM_PDS_TA_STATE] = psServerInitInfo->psPDSTAStateMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psPDSTAStateMemInfo);
	#if defined(SGX_FEATURE_MTE_STATE_FLUSH) && defined(SGX_FEATURE_COMPLEX_GEOMETRY_REV_2)
	psInitInfo->asInitMemHandles[SGX_INIT_MEM_USE_CMPLX_TA_STATE] = psServerInitInfo->psUSECmplxTAStateMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psUSECmplxTAStateMemInfo);
	psInitInfo->asInitMemHandles[SGX_INIT_MEM_PDS_CMPLX_TA_STATE] = psServerInitInfo->psPDSCmplxTAStateMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psPDSCmplxTAStateMemInfo);
	#endif
	#if defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
	psInitInfo->asInitMemHandles[SGX_INIT_MEM_USE_SA_RESTORE] = psServerInitInfo->psUSESARestoreMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psUSESARestoreMemInfo);
	psInitInfo->asInitMemHandles[SGX_INIT_MEM_PDS_SA_RESTORE] = psServerInitInfo->psPDSSARestoreMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psPDSSARestoreMemInfo);
	#endif
#endif

#if defined(FIX_HW_BRN_23861)
	psInitInfo->asInitDevData[SGX_DUMMY_STENCIL_LOAD_BASE_ADDR] =
		psServerInitInfo->psDummyStencilLoadMemInfo->sDevVAddr.uiAddr;
	psInitInfo->asInitDevData[SGX_DUMMY_STENCIL_STORE_BASE_ADDR] =
		psServerInitInfo->psDummyStencilStoreMemInfo->sDevVAddr.uiAddr;

	psInitInfo->asInitMemHandles[SGX_INIT_MEM_DUMMY_STENCIL_LOAD] = psServerInitInfo->psDummyStencilLoadMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psDummyStencilLoadMemInfo);
	psInitInfo->asInitMemHandles[SGX_INIT_MEM_DUMMY_STENCIL_STORE] = psServerInitInfo->psDummyStencilStoreMemInfo->hKernelMemInfo;
	PVRSRVUnrefDeviceMem(psDevData, psServerInitInfo->psDummyStencilStoreMemInfo);
#endif

	psInitInfo->ui32EDMTaskReg0 = psServerInitInfo->ui32EDMTaskReg0;
	psInitInfo->ui32EDMTaskReg1 = psServerInitInfo->ui32EDMTaskReg1;

	psInitInfo->ui32ClkGateCtl = psServerInitInfo->ui32ClkGateCtl;
	psInitInfo->ui32ClkGateCtl2 = psServerInitInfo->ui32ClkGateCtl2;
	psInitInfo->ui32ClkGateStatusReg = psServerInitInfo->ui32ClkGateStatusReg;
	psInitInfo->ui32ClkGateStatusMask = psServerInitInfo->ui32ClkGateStatusMask;
#if defined(SGX_FEATURE_MP)
	psInitInfo->ui32MasterClkGateStatusReg = psServerInitInfo->ui32MasterClkGateStatusReg;
	psInitInfo->ui32MasterClkGateStatusMask = psServerInitInfo->ui32MasterClkGateStatusMask;
	psInitInfo->ui32MasterClkGateStatus2Reg = psServerInitInfo->ui32MasterClkGateStatus2Reg;
	psInitInfo->ui32MasterClkGateStatus2Mask = psServerInitInfo->ui32MasterClkGateStatus2Mask;
#endif /* SGX_FEATURE_MP */

	psInitInfo->sScripts = psServerInitInfo->sScripts;

	if (PVRSRVBridgeCall(psDevData->psConnection->hServices,
						 PVRSRV_BRIDGE_SGX_DEVINITPART2,
						 psBridgeIn,
						 sizeof(PVRSRV_BRIDGE_IN_SGXDEVINITPART2),
						 &sBridgeOut,
						 sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXDevInitPart2: BridgeCall failed"));
		sBridgeOut.eError = PVRSRV_ERROR_BRIDGE_CALL_FAILED;

		/* Report the build options */
		PVR_DPF((PVR_DBG_ERROR, "Build options %s: client/UM 0x%x, driver/KM 0x%x",
				 ((SGX_BUILD_OPTIONS) == sBridgeOut.ui32KMBuildOptions) ? "match" : "mismatch",
				 SGX_BUILD_OPTIONS,
				 sBridgeOut.ui32KMBuildOptions));
		if ((SGX_BUILD_OPTIONS) != sBridgeOut.ui32KMBuildOptions)
		{
			PVR_DPF((PVR_DBG_ERROR, "Mismatching options are 0x%x; please see sgx_options.h",
					 (SGX_BUILD_OPTIONS) ^ sBridgeOut.ui32KMBuildOptions));
		}
	}

	PVRSRVFreeUserModeMem(psBridgeIn);

	return sBridgeOut.eError;
}

#if !defined(OS_SGX_GET_CLIENT_INFO_UM)
/*!
*******************************************************************************

 @Function	SGXGetClientInfo

 @Description	Gets SGX Device Information for Client

 @Input		psDevData :

 @Output	psSGXInfo :

 @Return	PVRSRV_ERROR :

******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV SGXGetClientInfo(PVRSRV_DEV_DATA *psDevData,
										   PVRSRV_SGX_CLIENT_INFO *psSGXInfo)
{
	PVRSRV_BRIDGE_OUT_GETCLIENTINFO sBridgeOut;
	PVRSRV_BRIDGE_IN_GETCLIENTINFO sBridgeIn;

	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));
	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	
	PVR_DPF((PVR_DBG_VERBOSE, "SGXGetClientInfo"));

	if(!psDevData || !psSGXInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXGetClientInfo: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	PVRSRVMemSet(&sBridgeOut, 0U, sizeof(sBridgeOut));

	/* use cookie to find sgx device info */
	sBridgeIn.hDevCookie = psDevData->hDevCookie;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_SGX_GETCLIENTINFO,
						&sBridgeIn,
						sizeof(sBridgeIn),
						&sBridgeOut,
						sizeof(sBridgeOut)))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXGetClientInfo: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sBridgeOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXGetClientInfo: KM failed %d",sBridgeOut.eError));
		return sBridgeOut.eError;
	}

	psSGXInfo->ui32ProcessID = sBridgeOut.sClientInfo.ui32ProcessID;
	psSGXInfo->pvProcess = sBridgeOut.sClientInfo.pvProcess;
	psSGXInfo->sMiscInfo = sBridgeOut.sClientInfo.sMiscInfo;

	psSGXInfo->ui32EVMConfig		= sBridgeOut.sClientInfo.asDevData[SGX_DEV_DATA_EVM_CONFIG];
	psSGXInfo->ui32ClientKickFlags	= sBridgeOut.sClientInfo.asDevData[SGX_DEV_DATA_CLIENT_KICK_FLAGS];

	psSGXInfo->ui32NumUSETemporaryRegisters = sBridgeOut.sClientInfo.asDevData[SGX_DEV_DATA_NUM_USE_TEMP_REGS];
	psSGXInfo->ui32NumUSEAttributeRegisters = sBridgeOut.sClientInfo.asDevData[SGX_DEV_DATA_NUM_USE_ATTRIBUTE_REGS];

	psSGXInfo->uPDSExecBase.uiAddr = sBridgeOut.sClientInfo.asDevData[SGX_DEV_DATA_PDS_EXEC_BASE];
	psSGXInfo->uUSEExecBase.uiAddr = sBridgeOut.sClientInfo.asDevData[SGX_DEV_DATA_USE_EXEC_BASE];

	return PVRSRV_OK;
}
#endif /* OS_SGX_GET_CLIENT_INFO_UM */


#if !defined(OS_SGX_RELEASE_CLIENT_INFO_UM)
/*!
*******************************************************************************

 @Function	SGXReleaseClientInfo

 @Description	Release SGX Device Information from Client

 @Input		psDevData :

 @Output	psSGXInfo :

 @Return	PVRSRV_ERROR :

******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV SGXReleaseClientInfo(PVRSRV_DEV_DATA *psDevData,
											   PVRSRV_SGX_CLIENT_INFO *psSGXInfo)
{
	PVRSRV_BRIDGE_IN_RELEASECLIENTINFO sBridgeIn;
	PVRSRV_BRIDGE_RETURN sBridgeOut;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

	PVR_DPF((PVR_DBG_VERBOSE, "SGXReleaseClientInfo"));

	if(!psDevData || !psSGXInfo || !psDevData->psConnection)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXReleaseClientInfo: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* use cookie to find sgx device info */
	sBridgeIn.hDevCookie = psDevData->hDevCookie;

	sBridgeIn.sClientInfo.ui32ProcessID = psSGXInfo->ui32ProcessID;
	sBridgeIn.sClientInfo.pvProcess = psSGXInfo->pvProcess;
	sBridgeIn.sClientInfo.sMiscInfo = psSGXInfo->sMiscInfo;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_SGX_RELEASECLIENTINFO,
						&sBridgeIn,
						sizeof(sBridgeIn),
						&sBridgeOut,
						sizeof(sBridgeOut)))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXReleaseClientInfo: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sBridgeOut.eError;
}
#endif /* OS_SGX_RELEASE_CLIENT_INFO_UM */


#if !defined(OS_SGX_GET_INTERNAL_DEVINFO_UM)
/*!
*******************************************************************************

 @Function	SGXGetInternalDevInfo

 @Description	Gets SGX Device Information for Client

 @Input		psDevData :

 @Output	psSGXInternalDevInfo :

 @Return	PVRSRV_ERROR :

******************************************************************************/
IMG_INTERNAL PVRSRV_ERROR IMG_CALLCONV SGXGetInternalDevInfo(const PVRSRV_DEV_DATA *psDevData,
												PVRSRV_SGX_INTERNALDEV_INFO *psSGXInternalDevInfo)
{
	PVRSRV_BRIDGE_OUT_GETINTERNALDEVINFO sBridgeOut;
	PVRSRV_BRIDGE_IN_GETINTERNALDEVINFO sBridgeIn;

	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));
	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	
	PVR_DPF((PVR_DBG_VERBOSE, "SGXGetInternalDevInfo"));

	if(!psDevData || !psSGXInternalDevInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXGetInternalDevInfo: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	PVRSRVMemSet(&sBridgeOut, 0U, sizeof(sBridgeOut));

	/* use cookie to find sgx device info */
	sBridgeIn.hDevCookie = psDevData->hDevCookie;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_SGX_GETINTERNALDEVINFO,
						&sBridgeIn,
						sizeof(sBridgeIn),
						&sBridgeOut,
						sizeof(sBridgeOut)))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXGetInternalDevInfo: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sBridgeOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXGetInternalDevInfo: KM failed %d",sBridgeOut.eError));
		return sBridgeOut.eError;
	}

	psSGXInternalDevInfo->ui32Flags =
		sBridgeOut.sSGXInternalDevInfo.ui32Flags;
	psSGXInternalDevInfo->hHostCtlKernelMemInfoHandle =
		sBridgeOut.sSGXInternalDevInfo.hHostCtlKernelMemInfoHandle;
	psSGXInternalDevInfo->bForcePTOff =
		sBridgeOut.sSGXInternalDevInfo.bForcePTOff;

	return PVRSRV_OK;
}
#endif /* OS_SGX_GET_INTERNAL_DEVINFO_UM */


IMG_INTERNAL
PVRSRV_ERROR IMG_CALLCONV SGXConnectionCheck(PVRSRV_DEV_DATA	*psDevData)
{
	PVRSRV_ERROR 	eError;
	SGX_MISC_INFO	sMiscInfo;
	IMG_UINT32		ui32BuildOptions;

	PVRSRVMemSet(&sMiscInfo,0x00,sizeof(sMiscInfo));


	/* query the kernel module build config */
	sMiscInfo.eRequest = SGX_MISC_INFO_REQUEST_DRIVER_SGXREV;
	eError = SGXGetMiscInfo(psDevData, &sMiscInfo);
	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXConnectionCheck: Call to SGXGetMiscInfo failed."));
		return eError;
	}

	/* compare to the client build config */
	ui32BuildOptions = sMiscInfo.uData.sSGXFeatures.ui32BuildOptions;
	if (ui32BuildOptions != (SGX_BUILD_OPTIONS))
	{
		IMG_UINT32 ui32BuildOptionsMismatch;
		
		ui32BuildOptionsMismatch = ui32BuildOptions ^ (SGX_BUILD_OPTIONS);
		if ( (ui32BuildOptions & ui32BuildOptionsMismatch) != 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "SGXConnectionCheck: Build option mismatch, "
					"driver contains extra options: %x. Please check sgx_options.h",
					ui32BuildOptions & ui32BuildOptionsMismatch));
		}
		if ( ((SGX_BUILD_OPTIONS) & ui32BuildOptionsMismatch) != 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "SGXConnectionCheck: Build option mismatch, "
					"client contains extra options: %x. Please check sgx_options.h",
					(SGX_BUILD_OPTIONS) & ui32BuildOptionsMismatch));
		}
		return PVRSRV_ERROR_BUILD_MISMATCH;
	}

	if (sMiscInfo.uData.sSGXFeatures.ui32DDKVersion !=
		((PVRVERSION_MAJ << 16) |
		 (PVRVERSION_MIN << 8) |
		  PVRVERSION_BRANCH) ||
		(sMiscInfo.uData.sSGXFeatures.ui32DDKBuild != PVRVERSION_BUILD))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXConnectionCheck: Incompatible client DDK revision "
				"(%d)/device DDK revision (%d).",
				PVRVERSION_BUILD, sMiscInfo.uData.sSGXFeatures.ui32DDKBuild));
		return PVRSRV_ERROR_DDK_VERSION_MISMATCH;
	}

	return PVRSRV_OK;
}


#if !defined(OS_SGX_GET_MISC_INFO_UM)

/*!
******************************************************************************

 @Function	SGXDumpMKTrace
 @Description	Dumps MK trace following a software fault
 @Input		psDevData : device data
 			bPanicKernel : whether to trigger kernel panic
 @Return	none

******************************************************************************/
IMG_INTERNAL
PVRSRV_ERROR IMG_CALLCONV SGXDumpMKTrace(PVRSRV_DEV_DATA *psDevData)
{
	SGX_MISC_INFO sMiscInfo;
	PVRSRVMemSet(&sMiscInfo,0x00,sizeof(sMiscInfo));
	
	PVRSRVMemSet(&sMiscInfo, sizeof(SGX_MISC_INFO), 0);
	sMiscInfo.eRequest = SGX_MISC_INFO_DUMP_DEBUG_INFO;

	return SGXGetMiscInfo(psDevData, &sMiscInfo);
}

/*!
******************************************************************************

 @Function	SGXGetMiscInfo

 @Description	Handles miscellaneous SGX commands

 @Input		psDevData :

 @Modified	psMiscInfo

 @Return	PVRSRV_ERROR :

******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV SGXGetMiscInfo(PVRSRV_DEV_DATA	*psDevData,
										 SGX_MISC_INFO		*psMiscInfo)
{
	PVRSRV_BRIDGE_IN_SGXGETMISCINFO sBridgeIn;
	PVRSRV_BRIDGE_RETURN sBridgeOut;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));
	
	PVR_DPF((PVR_DBG_VERBOSE, "SGXGetMiscInfo"));

	if (!psMiscInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXGetMiscInfo: Invalid psMiscInfo"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (psMiscInfo->eRequest == SGX_MISC_INFO_REQUEST_SPM)
	{
		SGX_RTDATASET *psRTDataSet = (SGX_RTDATASET*)psMiscInfo->uData.sSPM.hRTDataSet;

		if (!psRTDataSet)
		{
			PVR_DPF((PVR_DBG_ERROR, "SGXGetMiscInfo: SGX_MISC_INFO_REQUEST_SPM invalid hRTDataSet"));
			return PVRSRV_ERROR_INVALID_PARAMS;
		}

		psMiscInfo->uData.sSPM.ui32NumOutOfMemSignals = psRTDataSet->psHWRTDataSet->ui32NumOutOfMemSignals;
		psMiscInfo->uData.sSPM.ui32NumSPMRenders = psRTDataSet->psHWRTDataSet->ui32NumSPMRenders;

		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hDevCookie = psDevData->hDevCookie;
	sBridgeIn.psMiscInfo = psMiscInfo;

	if (PVRSRVBridgeCall(psDevData->psConnection->hServices,
						 PVRSRV_BRIDGE_SGX_GETMISCINFO,
						 &sBridgeIn,
						 sizeof(PVRSRV_BRIDGE_IN_SGXGETMISCINFO),
						 &sBridgeOut,
						 sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXGetMiscInfo: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sBridgeOut.eError;
}
#endif /* OS_SGX_GET_MISC_INFO_UM */


/*!
******************************************************************************

 @Function	SGXReadHWPerfCB

 @Description	Read hardware performance circular buffer

 @Input		psDevData :
 			ui32ArraySize :

 @Modified	psHWPerfCBData :
 			pui32DataCount :
			pui32ClockSpeed :

 @Return	PVRSRV_ERROR :

******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV SGXReadHWPerfCB(PVRSRV_DEV_DATA				*psDevData,
										  IMG_UINT32					ui32ArraySize,
										  PVRSRV_SGX_HWPERF_CB_ENTRY	*psHWPerfCBData,
										  IMG_UINT32					*pui32DataCount,
										  IMG_UINT32					*pui32ClockSpeed,
										  IMG_UINT32					*pui32HostTimeStamp)
{
	PVRSRV_BRIDGE_IN_SGX_READ_HWPERF_CB sBridgeIn;
	PVRSRV_BRIDGE_OUT_SGX_READ_HWPERF_CB sBridgeOut;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));
	
	PVR_DPF((PVR_DBG_VERBOSE, "SGXReadHWPerfCB"));

	sBridgeIn.hDevCookie = psDevData->hDevCookie;
	sBridgeIn.ui32ArraySize = ui32ArraySize;
	sBridgeIn.psHWPerfCBData = psHWPerfCBData;

	if (PVRSRVBridgeCall(psDevData->psConnection->hServices,
						 PVRSRV_BRIDGE_SGX_READ_HWPERF_CB,
						 &sBridgeIn,
						 sizeof(sBridgeIn),
						 &sBridgeOut,
						 sizeof(sBridgeOut)))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXReadHWPerfCB: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if (sBridgeOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXReadHWPerfCB: KM failed %d", sBridgeOut.eError));
		return sBridgeOut.eError;
	}

	*pui32DataCount = sBridgeOut.ui32DataCount;
	*pui32ClockSpeed = sBridgeOut.ui32ClockSpeed;
	*pui32HostTimeStamp = sBridgeOut.ui32HostTimeStamp;

	return PVRSRV_OK;
}

/*!
*******************************************************************************
 @Function	SGX2DQueryBlitsComplete

 @Description	Queries if blits have finished or wait for completion

 @Input		psDevData :

 @Input		psSyncInfo :

 @Input		bWaitForComplete : Single query or wait

 @Return	PVRSRV_ERROR :

******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV SGX2DQueryBlitsComplete(PVRSRV_DEV_DATA *psDevData,
												  PVRSRV_CLIENT_SYNC_INFO *psSyncInfo,
												  IMG_BOOL bWaitForComplete)
{
	PVRSRV_BRIDGE_RETURN sBridgeOut;
	PVRSRV_BRIDGE_IN_2DQUERYBLTSCOMPLETE sBridgeIn;

	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));
	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));

	PVR_DPF((PVR_DBG_VERBOSE, "SGX2DQueryBlitsComplete"));

	if(!psDevData || !psSyncInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGX2DQueryBlitsComplete: Invalid psMiscInfo"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hDevCookie = psDevData->hDevCookie;
	sBridgeIn.hKernSyncInfo = psSyncInfo->hKernelSyncInfo;
	sBridgeIn.bWaitForComplete = bWaitForComplete;

	if (PVRSRVBridgeCall(psDevData->psConnection->hServices,
						 PVRSRV_BRIDGE_SGX_2DQUERYBLTSCOMPLETE,
						 &sBridgeIn,
						 sizeof(sBridgeIn),
						 &sBridgeOut,
						 sizeof(sBridgeOut)))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGX2DQueryBlitsComplete: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sBridgeOut.eError;
}


#if defined(PDUMP)
#if !defined(OS_PVRSRV_PDUMP_BUFFER_ARRAY_UM)
/*!
******************************************************************************

 @Function	PVRSRVPDumpBufferArray

 @Description 	PDUMP information in stored buffer array

 @Input		psConnection : connection info
 @Input		psBufferArray
 @Input		ui32BufferArrayLength
 @Input		bDumpPolls

 @Return	PVRSRV_ERROR

******************************************************************************/
IMG_EXPORT
IMG_VOID IMG_CALLCONV PVRSRVPDumpBufferArray(const PVRSRV_CONNECTION *psConnection,
											 SGX_KICKTA_DUMP_BUFFER *psBufferArray,
											 IMG_UINT32 ui32BufferArrayLength,
											 IMG_BOOL bDumpPolls)
{
	PVRSRV_BRIDGE_IN_PDUMP_BUFFER_ARRAY sIn;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));

	if(!psConnection || !psBufferArray)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVPDumpBufferArray: Invalid params"));
		return;
	}

	sIn.psBufferArray = psBufferArray;
	sIn.ui32BufferArrayLength = ui32BufferArrayLength;
	sIn.bDumpPolls = bDumpPolls;

	PVRSRVBridgeCall(psConnection->hServices,
					 PVRSRV_BRIDGE_SGX_PDUMP_BUFFER_ARRAY,
					 &sIn,
					 sizeof(PVRSRV_BRIDGE_IN_PDUMP_BUFFER_ARRAY),
					 IMG_NULL,
					 0);
}
#endif /* OS_PVRSRV_PDUMP_BUFFER_ARRAY_UM */

/*!
 ******************************************************************************

 @Function	PVRSRVPDump3DSignatureRegisters

 @Description	PDumps a formatted comment

 @Input		psConnection : connection info
 @Input		ui32DumpFrameNum
 @Input		bLastFrame
 @Input		pui323DRegisters
 @Input		ui32Num3DRegisters

 @Return	none

 ******************************************************************************/
IMG_INTERNAL IMG_VOID PVRSRVPDump3DSignatureRegisters(const PVRSRV_DEV_DATA *psDevData,
#if defined (SUPPORT_SID_INTERFACE)
														IMG_SID    hDevMemContext,
#else
														IMG_HANDLE hDevMemContext,
#endif
														IMG_UINT32 ui32DumpFrameNum,
														IMG_BOOL bLastFrame,
														IMG_UINT32 *pui32Registers,
														IMG_UINT32 ui32NumRegisters)
{
	PVRSRV_BRIDGE_IN_PDUMP_3D_SIGNATURE_REGISTERS sIn;
	PVRSRV_BRIDGE_RETURN	sOut;


	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));	
	

	if(!psDevData || !pui32Registers)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVPDump3DSignatureRegisters: Invalid params"));
		return;
	}

	sIn.hDevCookie	= psDevData->hDevCookie;
	sIn.hDevMemContext = hDevMemContext;
	sIn.ui32DumpFrameNum = ui32DumpFrameNum;
	sIn.bLastFrame = bLastFrame;
	sIn.pui32Registers = pui32Registers;
	sIn.ui32NumRegisters = ui32NumRegisters;

	if (PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_SGX_PDUMP_3D_SIGNATURE_REGISTERS,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_PDUMP_3D_SIGNATURE_REGISTERS),
						&sOut,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVPDump3DSignatureRegisters: Bridge call failed"));
	}
	if(sOut.eError)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVPDump3DSignatureRegisters: Bridge returned error %u", sOut.eError));
	}
}

/*!
 ******************************************************************************

 @Function	PVRSRVPDumpPerformanceCounterRegisters

 @Description	PDumps a formatted comment

 @Input		psDevData : connection info
 @Input		ui32DumpFrameNum
 @Input		bLastFrame
 @Input		pui323DRegisters
 @Input		ui32Num3DRegisters

 @Return	none

 ******************************************************************************/
IMG_INTERNAL IMG_VOID PVRSRVPDumpPerformanceCounterRegisters(const PVRSRV_DEV_DATA *psDevData,
														IMG_UINT32 ui32DumpFrameNum,
														IMG_BOOL bLastFrame,
														IMG_UINT32 *pui32Registers,
														IMG_UINT32 ui32NumRegisters)
{
	PVRSRV_BRIDGE_IN_PDUMP_COUNTER_REGISTERS sIn;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));


	if(!psDevData || !pui32Registers)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVPDumpPerformanceCounterRegisters: Invalid params"));
		return;
	}

 	sIn.hDevCookie = psDevData->hDevCookie;
	sIn.ui32DumpFrameNum = ui32DumpFrameNum;
	sIn.bLastFrame = bLastFrame;
	sIn.pui32Registers = pui32Registers;
	sIn.ui32NumRegisters = ui32NumRegisters;

	if (PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_SGX_PDUMP_COUNTER_REGISTERS,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_PDUMP_COUNTER_REGISTERS),
						IMG_NULL,
						0))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVPDumpPerformanceCounterRegisters: Bridge call failed"));
	}
}

/*!
 ******************************************************************************

 @Function	PVRSRVPDumpTASignatureRegisters

 @Description	PDumps a formatted comment

 @Input		psConnection : connection info
 @Input		ui32DumpFrameNum
 @Input		ui32TAKickCount
 @Input		bLastFrame
 @Input		pui323DRegisters
 @Input		ui32Num3DRegisters

 @Return	none

 ******************************************************************************/
IMG_INTERNAL IMG_VOID PVRSRVPDumpTASignatureRegisters(const PVRSRV_DEV_DATA	*psDevData,
														IMG_UINT32 ui32DumpFrameNum,
														IMG_UINT32 ui32TAKickCount,
														IMG_BOOL bLastFrame,
														IMG_UINT32 *pui32Registers,
														IMG_UINT32 ui32NumRegisters)
{
	PVRSRV_BRIDGE_IN_PDUMP_TA_SIGNATURE_REGISTERS sIn;
	PVRSRV_BRIDGE_RETURN	sOut;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));

	
	if(!psDevData || !pui32Registers)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVPDumpTASignatureRegisters: Invalid params"));
		return;
	}

	sIn.hDevCookie	= psDevData->hDevCookie;
	sIn.ui32DumpFrameNum = ui32DumpFrameNum;
	sIn.ui32TAKickCount = ui32TAKickCount;
	sIn.bLastFrame = bLastFrame;
	sIn.pui32Registers = pui32Registers;
	sIn.ui32NumRegisters = ui32NumRegisters;

	if (PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_SGX_PDUMP_TA_SIGNATURE_REGISTERS,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_PDUMP_TA_SIGNATURE_REGISTERS),
						&sOut,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVPDumpTASignatureRegisters: Bridge call failed"));
	}
	if(sOut.eError)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVPDumpTASignatureRegisters: Bridge returned error %u", sOut.eError));
	}
}


/*!
 ******************************************************************************

 @Function	PVRSRVPDumpHWPerfCB

 @Description	Dumps the HW Perf circular buffer to a file

 @Input		psDevData :
 @Input		pszFileName
 @Input		ui32FileOffset
 @Input		ui32PDumpFlags

 @Return	PVRSRV_ERROR

 ******************************************************************************/

#if !defined(OS_PVRSRV_PDUMP_HWPERFCB_UM)
IMG_INTERNAL IMG_VOID PVRSRVPDumpHWPerfCB(const PVRSRV_DEV_DATA	*psDevData,
#if defined (SUPPORT_SID_INTERFACE)
										  IMG_SID   		hDevMemContext,
#else
										  IMG_HANDLE		hDevMemContext,
#endif
										  IMG_CHAR			*pszFileName,
										  IMG_UINT32		ui32FileOffset,
										  IMG_UINT32		ui32PDumpFlags)
{
	PVRSRV_BRIDGE_IN_PDUMP_HWPERFCB sIn;
	PVRSRV_BRIDGE_RETURN sRet;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sRet,0x00,sizeof(sRet));

	if(!psDevData || !pszFileName)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVPDumpHWPerfCB: Invalid params"));
		return;
	}

	PVRSRVMemCopy(sIn.szFileName, pszFileName, PVRSRV_PDUMP_MAX_FILENAME_SIZE);

	sIn.hDevCookie		= psDevData->hDevCookie;
	sIn.hDevMemContext	= hDevMemContext;
	sIn.ui32FileOffset	= ui32FileOffset;
	sIn.ui32PDumpFlags	= ui32PDumpFlags;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_SGX_PDUMP_HWPERFCB,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_PDUMP_HWPERFCB),
						&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVPDumpHWPerfCB: Bridge call failed"));
	}

	if(sRet.eError)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVPDumpHWPerfCB: Bridge returned error %u", sRet.eError));
	}
}
#endif  /* OS_PVRSRV_PDUMP_HWPERFCB_UM */

IMG_INTERNAL IMG_VOID PVRSRVPDumpSaveMem(const PVRSRV_DEV_DATA	*psDevData,
										 IMG_CHAR			*pszFileName,
										 IMG_UINT32			ui32FileOffset,
										 IMG_DEV_VIRTADDR 	sDevVAddr,
										 IMG_UINT32			ui32Size,
#if defined (SUPPORT_SID_INTERFACE)
										 IMG_SID			hDevMemContext,
#else
										 IMG_HANDLE			hDevMemContext,
#endif
										 IMG_UINT32			ui32PDumpFlags)
{
	PVRSRV_BRIDGE_IN_PDUMP_SAVEMEM sIn;
	PVRSRV_BRIDGE_RETURN sRet;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sRet,0x00,sizeof(sRet));


	if(!psDevData || !pszFileName)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVPDumpSaveMem: Invalid params"));
		return;
	}

	PVRSRVMemCopy(sIn.szFileName, pszFileName, PVRSRV_PDUMP_MAX_FILENAME_SIZE);

	sIn.hDevCookie		= psDevData->hDevCookie;
	sIn.ui32FileOffset	= ui32FileOffset;
	sIn.sDevVAddr		= sDevVAddr;
	sIn.ui32Size		= ui32Size;
	sIn.hDevMemContext	= hDevMemContext;
	sIn.ui32PDumpFlags	= ui32PDumpFlags;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_SGX_PDUMP_SAVEMEM,
						&sIn, sizeof(sIn),
						&sRet, sizeof(sRet)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVPDumpSaveMem: Bridge call failed"));
	}

	if(sRet.eError)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVPDumpSaveMem: Bridge returned error %u", sRet.eError));
	}
}

#endif /* PDUMP */
/******************************************************************************
 End of file (bridged_sgx_glue.c)
******************************************************************************/
