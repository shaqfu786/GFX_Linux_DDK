/*!****************************************************************************
@File           sgxrender_context.c

@Title          Render Context routines

@Author         Imagination Technologies

@Date           14 / 10 / 05

@Copyright      Copyright 2005-2008 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       Generic

@Description    Render Context functions

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-
$Log: sgxrender_context.c $
******************************************************************************/
#include <stddef.h>

#include "img_defs.h"
#include "services.h"
#include "sgxapi.h"
#include "sgxinfo.h"
#include "sgxinfo_client.h"
#include "sgxutils_client.h"
#include "sgxmmu.h"
#include "pvr_debug.h"
#include "pdump_um.h"
#include "sysinfo.h"
#include "sgx_bridge.h"
#include "sgx_bridge_um.h"
#include "servicesinit_um.h"
#include "osfunc_client.h"
#include "sgxpb.h"
#include "pds.h"
#include "usecodegen.h"

/*****************************************************************************
 FUNCTION	: SGXDestroyRenderContext

 PURPOSE	: Destroys a rendering context and everything associated with it

 PARAMETERS	:

 RETURNS	: PVRSRV_ERROR
*****************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV
SGXDestroyRenderContext(PVRSRV_DEV_DATA *psDevData,
						IMG_HANDLE hRenderContext,
						PVRSRV_CLIENT_MEM_INFO *psVisTestResultMemInfo,
						IMG_BOOL bForceCleanup)
{
	SGX_RENDERCONTEXT *psRenderContext = (SGX_RENDERCONTEXT *)hRenderContext;
	PVRSRV_ERROR	eError;
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	IMG_UINT32		i;
#endif

	PVR_UNREFERENCED_PARAMETER(psVisTestResultMemInfo);

	if (!psRenderContext)
	{
	    PVR_DPF((PVR_DBG_ERROR, "SGXDestroyRenderContext: NULL handle"));
	    return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* Destroy the render context mutex */
	eError = PVRSRVDestroyMutex(psRenderContext->hMutex);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXDestroyRenderContext: Failed to destroy render context mutex\n"));
	}

	/* Tell the uKernel that we've finished with the render context. */
	eError = SGXUnregisterHWRenderContext(psDevData, psRenderContext->hHWRenderContext, bForceCleanup);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to unregister HW render context (%d)", eError));
		return eError;
	}

	/* Wait for ukernel tasks to flush out. */
	#if defined(EUR_CR_USE0_DM_SLOT)
	PDUMPREGPOL(psDevData, "SGXREG", SGX_MP_CORE_SELECT(EUR_CR_USE0_DM_SLOT, 0), 0xAAAAAAAA, 0xFFFFFFFF);
	#else
	PDUMPREGPOL(psDevData, "SGXREG", SGX_MP_CORE_SELECT(EUR_CR_USE_DM_SLOT, 0), 0xAAAAAAAA, 0xFFFFFFFF);
	#endif /* EUR_CR_USE0_DM_SLOT */

#if defined(DEBUG)
	/* Stats for context switching */
#if 0 /* Please see BRN34305. The HW (MK) Render/Transfer Contexts are no longer accessible to UM. */
	#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	PVR_DPF((PVR_DBG_ERROR, "VDM context switch: %u stores, %u resumes.",
			psRenderContext->psHWRenderContext->ui32NumTAContextStores,
			psRenderContext->psHWRenderContext->ui32NumTAContextResumes));
	#endif
	#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH_REV_3)
	PVR_DPF((PVR_DBG_ERROR, "ISP context switch: %u stores, %u resumes.",
			psRenderContext->psHWRenderContext->ui32Num3DContextStores,
			psRenderContext->psHWRenderContext->ui32Num3DContextResumes));
	#endif
#endif
	/* Miscellaneous stats */
	{
		SGX_MISC_INFO sMiscInfo = {SGX_MISC_INFO_REQUEST_CLOCKSPEED};
		
	#if !defined(SUPPORT_ACTIVE_POWER_MANAGEMENT) && !defined(SUPPORT_HW_RECOVERY)
		PVR_UNREFERENCED_PARAMETER(sMiscInfo);
	#endif
		
	#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
		sMiscInfo.eRequest = SGX_MISC_INFO_REQUEST_ACTIVEPOWER;
		eError = SGXGetMiscInfo(psDevData, &sMiscInfo);
		if(eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "SGXDestroyRenderContext: Call to SGXGetMiscInfo failed."));
			return eError;
		}
	
		PVR_DPF((PVR_DBG_WARNING, "Active Power stats: Total: %u", 
									sMiscInfo.uData.sActivePower.ui32NumActivePowerEvents));
	#endif /* SUPPORT_ACTIVE_POWER_MANAGEMENT */


	#if defined(SUPPORT_HW_RECOVERY)
		sMiscInfo.eRequest = SGX_MISC_INFO_REQUEST_LOCKUPS;
		eError = SGXGetMiscInfo(psDevData, &sMiscInfo);
		if(eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "SGXDestroyRenderContext: Call to SGXGetMiscInfo failed."));
			return eError;
		}
		PVR_DPF((PVR_DBG_WARNING, "HW Recovery stats: Host: %d \t uKernel: %d",
										 sMiscInfo.uData.sLockups.ui32HostDetectedLockups, 
										 sMiscInfo.uData.sLockups.ui32uKernelDetectedLockups));
	#endif /* SUPPORT_HW_RECOVERY */
	}
#endif /* DEBUG */

	/* Destroy CCB if required */
	DestroyCCB(psDevData, psRenderContext->psTACCB);

	/* Deallocate TA/3D dependancy sync object */
	PVRSRVFreeDeviceMem(psDevData, psRenderContext->psTA3DSyncObjectMemInfo);

#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH)
	{
		IMG_UINT32	j;
		for (j=0; j < SGX_FEATURE_MP_CORE_COUNT_3D; j++)
		{
			IMG_UINT32	k;
			for (k=0; k < SGX_FEATURE_NUM_PDS_PIPES; k++)
			{
				if (psRenderContext->apsPDSCtxSwitchMemInfo[j][k])
				{
					PVRSRVFreeDeviceMem(psDevData, psRenderContext->apsPDSCtxSwitchMemInfo[j][k]);
				}
			}
	
			if (psRenderContext->psZLSCtxSwitchMemInfo[j])
			{
				PVRSRVFreeDeviceMem(psDevData, psRenderContext->psZLSCtxSwitchMemInfo[j]);
			}
		}
	}
#else
	#if defined(SGX_SCRATCH_PRIM_BLOCK_ENABLE)
	/* Deallocate scratch primitive block */
	PVRSRVFreeDeviceMem(psDevData, psRenderContext->psScratchPrimBlockClientMemInfo);
	#endif
#endif
#if defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
	for (i=0; i < SGX_FEATURE_MP_CORE_COUNT_TA; i++)
	{
		PVRSRVFreeDeviceMem(psDevData, psRenderContext->psVDMSnapShotBufferMemInfo[i]);	
	}
#endif
#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH)
	PVRSRVFreeDeviceMem(psDevData, psRenderContext->psMasterVDMSnapShotBufferMemInfo);
#endif
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	#if defined(SGX_FEATURE_MTE_STATE_FLUSH)
		#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
	for (i = 0; i < SGX_FEATURE_MP_CORE_COUNT_TA; i++)
		#else
	i=0;
		#endif
	{
		PVRSRVFreeDeviceMem(psDevData, psRenderContext->psMTEStateFlushMemInfo[i]);
	}	
	#endif
	#if defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
	PVRSRVFreeDeviceMem(psDevData, psRenderContext->psSABufferMemInfo);
	#endif
#endif

	/* Deallocate vistest result buffer */
	PVRSRVFreeDeviceMem(psDevData, psRenderContext->psVisTestResultClientMemInfo);

#if defined(SUPPORT_PERCONTEXT_PB)
	if (psRenderContext->bPerContextPB)
		DestroyPerContextPB(psDevData, psRenderContext->psClientPBDesc);
#endif
#if defined(SUPPORT_SHARED_PB)
	/* Destroy parameter buffer if required */
	if (!psRenderContext->bPerContextPB)
		UnrefPB(psDevData, psRenderContext->psClientPBDesc);
#endif

	PVRSRVFreeUserModeMem(psRenderContext->psClientPBDesc);

	/* Free render context */
	PVRSRVFreeUserModeMem(psRenderContext);

	return PVRSRV_OK;
}


/*****************************************************************************
 FUNCTION	: SGXCreateRenderContext

 PURPOSE	: Creates a rendering context and everything associated with it

 PARAMETERS	:

 RETURNS	: PVRSRV_ERROR
*****************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV
SGXCreateRenderContext(PVRSRV_DEV_DATA *psDevData,
						 PSGX_CREATERENDERCONTEXT psCreateRenderContext,
						 IMG_HANDLE *phRenderContext,
						 PVRSRV_CLIENT_MEM_INFO **ppsVisTestResultClientMemInfo)
{
	PSGX_RENDERCONTEXT		psRenderContext;
	PSGXMKIF_HWRENDERCONTEXT	psHWRenderContext;
	PVRSRV_ERROR				eError;
	IMG_UINT32					ui32PBSize;
#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
	IMG_UINT32					ui32PBSizeLimit = 0;
#endif
	IMG_UINT32					ui32HeapCount;
	PVRSRV_HEAP_INFO			asHeapInfo[PVRSRV_MAX_CLIENT_HEAPS];
	IMG_UINT32					i;
	PVRSRV_HEAP_INFO			*ps3DParamsHeapInfo = IMG_NULL;
	PVRSRV_HEAP_INFO			*psKernelVideoDataHeapInfo = IMG_NULL;
	PVRSRV_HEAP_INFO			*psSyncInfoHeapInfo = IMG_NULL;
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) || defined(SGX_FEATURE_ISP_CONTEXT_SWITCH)
	PVRSRV_HEAP_INFO			*psGeneralHeapInfo = IMG_NULL;
#endif
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) && defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
	IMG_UINT32					ui32SABufferSize;
#endif
	SGX_PBDESC				*psPBDesc=IMG_NULL;	/* PRQA S 3197 */ /* keeps compiler happy */
	SGX_CLIENTPBDESC		*psClientPBDesc=IMG_NULL;
#if defined(SUPPORT_SHARED_PB)
#if defined (SUPPORT_SID_INTERFACE)
	IMG_SID					hSharedPBDescKernelMemInfoHandle = 0;
	IMG_SID					hHWPBDescKernelMemInfoHandle = 0;
	IMG_SID					hBlockKernelMemInfoHandle = 0;
	IMG_SID					hHWBlockKernelMemInfoHandle = 0;
#else
	IMG_HANDLE				hSharedPBDescKernelMemInfoHandle = IMG_NULL;
	IMG_HANDLE				hHWPBDescKernelMemInfoHandle = IMG_NULL;
	IMG_HANDLE				hBlockKernelMemInfoHandle = IMG_NULL;
	IMG_HANDLE				hHWBlockKernelMemInfoHandle = IMG_NULL;
#endif
	IMG_UINT32				ui32SubKernelMemInfoHandlesCount = 0;
#if defined (SUPPORT_SID_INTERFACE)
	IMG_SID					*phSharedPBDescSubKernelMemInfoHandles = IMG_NULL;
	IMG_SID					hSharedPBDesc = 0;
#else
	IMG_HANDLE				*phSharedPBDescSubKernelMemInfoHandles = IMG_NULL;
	IMG_HANDLE				hSharedPBDesc = IMG_NULL;
#endif
#endif /* SUPPORT_SHARED_PB */
	PVRSRV_SGX_INTERNALDEV_INFO sSGXInternalDevInfo={0};

	eError = PVRSRVGetDeviceMemHeapInfo(psDevData,
									psCreateRenderContext->hDevMemContext,
									&ui32HeapCount,
									asHeapInfo);
	if (eError !=  PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXCreateRenderContext: Failed to retrieve device "
				 "memory context information\n"));
		return eError;
	}


	for(i=0; i<ui32HeapCount; i++)
	{
		switch(HEAP_IDX(asHeapInfo[i].ui32HeapID))
		{
#if (defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) || defined(SGX_FEATURE_ISP_CONTEXT_SWITCH))
			case SGX_GENERAL_HEAP_ID:
				psGeneralHeapInfo = &asHeapInfo[i];
				break;
#endif

/* Some compilers don't like if (constant expression) */
#if defined(SUPPORT_HYBRID_PB)
			case SGX_SHARED_3DPARAMETERS_HEAP_ID:
				if (!psCreateRenderContext->bPerContextPB)
					ps3DParamsHeapInfo = &asHeapInfo[i];
				break;
			case SGX_PERCONTEXT_3DPARAMETERS_HEAP_ID:
				if (psCreateRenderContext->bPerContextPB)
					ps3DParamsHeapInfo = &asHeapInfo[i];
				break;
#else
#if defined(SUPPORT_SHARED_PB)
			case SGX_SHARED_3DPARAMETERS_HEAP_ID:
#endif
#if defined(SUPPORT_PERCONTEXT_PB)
			case SGX_PERCONTEXT_3DPARAMETERS_HEAP_ID:
#endif
				ps3DParamsHeapInfo = &asHeapInfo[i];
				break;
#endif
			case SGX_KERNEL_DATA_HEAP_ID:
				psKernelVideoDataHeapInfo = &asHeapInfo[i];
				break;
			case SGX_SYNCINFO_HEAP_ID:
				psSyncInfoHeapInfo = &asHeapInfo[i];
				break;
			default:
				break;
		}
	}


	/*
		FIXME:
		do we already have a device memory context in this process?
		1) use per-process global data (symbian issue?)
		2) call into kernel to query this info
	*/

	/* 
     * Alloc render context structure, using User-Mode mem. Server will copy
     * this into device mem and insert the PD PAddr 
     */
	psRenderContext = PVRSRVAllocUserModeMem(sizeof(SGX_RENDERCONTEXT));
	if(psRenderContext == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,"ERROR - Failed to alloc host mem for render context !"));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorExit;
	}
	PVRSRVMemSet(psRenderContext, 0, sizeof(SGX_RENDERCONTEXT));

	/* store the associated Dev Mem Context */
	psRenderContext->hDevMemContext = psCreateRenderContext->hDevMemContext;

	/* 3D Param heap can be shared or per context */
	if(ps3DParamsHeapInfo == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,"ERROR - failed to find the 3d param heap"));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorExit;
	}
#if defined(SUPPORT_HYBRID_PB)
	psRenderContext->bPerContextPB = psCreateRenderContext->bPerContextPB;
#else
#if defined(SUPPORT_SHARED_PB)
	psRenderContext->bPerContextPB = IMG_FALSE;
#endif
#if defined(SUPPORT_PERCONTEXT_PB)
	psRenderContext->bPerContextPB = IMG_TRUE;
#endif
#endif

	/* get incoming PB request size */
	ui32PBSize = psCreateRenderContext->ui32PBSize;
	#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
	if (psRenderContext->bPerContextPB)
	{
		ui32PBSizeLimit = psCreateRenderContext->ui32PBSizeLimit;
		if ((ui32PBSizeLimit != 0) && (ui32PBSizeLimit < SGX_MIN_PBSIZE))
		{
			ui32PBSizeLimit = SGX_MIN_PBSIZE;
		}
		/* Round up to the nearest page */
		ui32PBSizeLimit = GET_ALIGNED_PB_SIZE(ui32PBSizeLimit);
	}
	#endif
	
	/* and decide if given PB size should be ignored */
	{
		IMG_VOID* pvHintState;
		IMG_UINT32 ui32AppHintDefault;
		PVRSRVCreateAppHintState(IMG_SRVCLIENT, 0, &pvHintState);

		ui32AppHintDefault = ui32PBSize;
		if (PVRSRVGetAppHint(pvHintState, "PBSize", IMG_UINT_TYPE, &ui32AppHintDefault, &ui32PBSize))
		{
			ui32PBSize *= EURASIA_PARAM_MANAGE_GRAN;
		}

		PVRSRVFreeAppHintState(IMG_SRVCLIENT, pvHintState);
	}

	if (psKernelVideoDataHeapInfo == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXCreateRenderContext: Failed to initialize psKernelVideoDataHeapInfo"));
		PVR_DBG_BREAK;
		eError = PVRSRV_ERROR_INVALID_HEAPINFO;
		goto ErrorExit;
	}

	/* Attempt to allocate HW render context struct */
	PDUMPCOMMENT(psDevData->psConnection, "Allocate HW render context struct", IMG_TRUE);
    psHWRenderContext = PVRSRVAllocUserModeMem(sizeof(SGXMKIF_HWRENDERCONTEXT));

    if (psHWRenderContext == IMG_NULL)
    {
		PVR_DPF((PVR_DBG_ERROR,"Failed to allocate HW render list struct!"));

		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorExit;
	}

	PVRSRVMemSet(psHWRenderContext, 0, sizeof(SGXMKIF_HWRENDERCONTEXT));

	/* allocate per app structure for the PB*/

	psClientPBDesc = PVRSRVAllocUserModeMem(sizeof(SGX_CLIENTPBDESC));
	if(!psClientPBDesc)
	{
		PVR_DPF((PVR_DBG_ERROR,"ERROR - Failed to alloc host mem for psClientPBDesc!"));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorExit;
	}
	PVRSRVMemSet(psClientPBDesc, 0, sizeof(SGX_CLIENTPBDESC));
	psRenderContext->psClientPBDesc = psClientPBDesc;

	/* ensure PB size requests are not less than SGX_MIN_PBSIZE */
	if(ui32PBSize < SGX_MIN_PBSIZE)
	{
		ui32PBSize = SGX_MIN_PBSIZE;
	}
	ui32PBSize = GET_ALIGNED_PB_SIZE(ui32PBSize);

#if defined(SUPPORT_PERCONTEXT_PB)
	if (psRenderContext->bPerContextPB)
	{
		/* we've got a per context PB */
		eError = CreatePerContextPB(psDevData,
									psRenderContext,
									ui32PBSize,
	#if defined(FORCE_ENABLE_GROW_SHRINK)
									ui32PBSizeLimit,
	#endif
									ps3DParamsHeapInfo,
									psKernelVideoDataHeapInfo);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "CreatePerContextPB: Failed\n"));
			goto ErrorExit;
		}

		/* get PBDesc */
		psPBDesc = psClientPBDesc->psPBDesc;
	}
#endif
#if defined(SUPPORT_SHARED_PB)
	if (!psRenderContext->bPerContextPB)
	{
	#if defined(DEBUG)
		IMG_BOOL bSGXFindSharedPBDescBlocked = IMG_FALSE;
	#endif

		for (;;)
		{
			/*
			 * Look for a suitable shared PB, obtaining the shared
			 * PB creation lock if there isn't one.
			 */
			eError = SGXFindSharedPBDesc(psDevData,
							   IMG_TRUE,
							   ui32PBSize,
							   &hSharedPBDesc,
							   &hSharedPBDescKernelMemInfoHandle,
							   &hHWPBDescKernelMemInfoHandle,
							   &hBlockKernelMemInfoHandle,
							   &hHWBlockKernelMemInfoHandle,
							   &phSharedPBDescSubKernelMemInfoHandles,
							   &ui32SubKernelMemInfoHandlesCount);
			if (eError != PVRSRV_ERROR_PROCESSING_BLOCKED)
			{
				break;
			}
	#if defined(DEBUG)
			if (!bSGXFindSharedPBDescBlocked)
			{
				PVR_TRACE(("SGXCreateRenderContext: SGXFindSharedPBDesc blocked"));
				bSGXFindSharedPBDescBlocked = IMG_TRUE;
			}
	#endif
			PVRSRVReleaseThreadQuanta();
		}
	#if defined(DEBUG)
		if (bSGXFindSharedPBDescBlocked)
		{
				PVR_TRACE(("SGXCreateRenderContext: SGXFindSharedPBDesc unblocked"));
		}
	#endif
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "SGXCreateRenderContext: "
					 "Error when searching for a suitable parameter buffer\n"));
			goto ErrorExit;
		}

		/* If we haven't got one then try and create a new one */
		if(!hSharedPBDesc)
		{
#if defined (SUPPORT_SID_INTERFACE)
			IMG_SID hCreateSharedPBDesc;
#else
			IMG_HANDLE hCreateSharedPBDesc;
#endif

			eError = CreateSharedPB(psDevData,
									&hCreateSharedPBDesc,
									ui32PBSize,
									psCreateRenderContext->hDevMemContext,
									ps3DParamsHeapInfo,
									psKernelVideoDataHeapInfo);
			if (eError != PVRSRV_OK)
			{
				goto ErrorExit;
			}

			eError = SGXFindSharedPBDesc(psDevData,
								   IMG_FALSE,
								   ui32PBSize,
								   &hSharedPBDesc,
								   &hSharedPBDescKernelMemInfoHandle,
								   &hHWPBDescKernelMemInfoHandle,
								   &hBlockKernelMemInfoHandle,
								   &hHWBlockKernelMemInfoHandle,
								   &phSharedPBDescSubKernelMemInfoHandles,
								   &ui32SubKernelMemInfoHandlesCount);
			/*
			 * Once we've done the find, do an unref on the shared PB desc
			 * handle returned by CreateSharedPB, in order to keep the PB desc
			 * reference count sensible.  If this isn't done, the PB desc
			 * may not cleaned up until all processes using it have
			 * terminated, rather than as soon as the PB desc is unused.
			 */
			if (SGXUnrefSharedPBDesc(psDevData, hCreateSharedPBDesc) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "SGXCreateRenderContext: Error unreferencing shared PB description handle returned by CreateSharedPB"));
			}

			if (eError != PVRSRV_OK || !hSharedPBDescKernelMemInfoHandle)
			{
				PVR_DPF((PVR_DBG_ERROR, "SGXCreateRenderContext: "
						 "Error when searching for our newly created parameter buffer\n"));
				eError = PVRSRV_ERROR_CREATE_RENDER_CONTEXT_FAILED;
				goto ErrorExit;
			}
		}

		/* MAP shared PBDesc into a new client PBDesc */
		eError = MapSharedPBDescToClient(psDevData,
										 hSharedPBDesc,
										 hSharedPBDescKernelMemInfoHandle,
										 hHWPBDescKernelMemInfoHandle,
										 hBlockKernelMemInfoHandle,
										 hHWBlockKernelMemInfoHandle,
										 phSharedPBDescSubKernelMemInfoHandles,
										 ui32SubKernelMemInfoHandlesCount,
										 psClientPBDesc);
		PVRSRVFreeUserModeMem(phSharedPBDescSubKernelMemInfoHandles);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "MapSharedPBDescToClient: "
					 "Failed to map shared PBDesc into client PBDesc\n"));
			goto ErrorExit;
		}

		psPBDesc = psClientPBDesc->psPBDesc;

		/* mark PB as shared */
		psPBDesc->ui32Flags |= SGX_PBDESCFLAGS_SHARED;

		if (psPBDesc->ui32TotalPBSize != ui32PBSize)
		{
			/*
				FIXME: The requested PB size check has been temporarily disabled
				because the microkernel cannot support multiple shared PBs.
			*/
			PVR_DPF((PVR_DBG_ERROR,
					"SGXCreateRenderContext: Shared PB requested with different size (0x%x) from existing shared PB (0x%x) - requested size ignored",
					ui32PBSize, psPBDesc->ui32TotalPBSize));
		}
	}
#endif /* SUPPORT_SHARED_PB */

	/* Attempt to create the CCB for CMDTAs */
	PDUMPCOMMENT(psDevData->psConnection, "Create TA per context CCB control structure", IMG_TRUE);
	eError = CreateCCB(psDevData,
						 SGX_CCB_SIZE,
						 SGX_CCB_ALLOCGRAN,
						 MAX(1024, sizeof(SGXMKIF_CMDTA) + SGX_NUM_3D_REGS_UPDATED * sizeof(PVRSRV_HWREG)),
						 psKernelVideoDataHeapInfo->hDevMemHeap,
						 &psRenderContext->psTACCB);

	if (eError != PVRSRV_OK)
	{
		goto ErrorExit;
	}

	PDUMPCOMMENT(psDevData->psConnection,
				 "TA per context CCB control structure",
				 IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL,
			 psRenderContext->psTACCB->psCCBCtlClientMemInfo, 0,
			 sizeof(PVRSRV_SGX_CCB_CTL), PDUMP_FLAGS_CONTINUOUS);


	if (psSyncInfoHeapInfo == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXCreateRenderContext: Failed to initialize psSyncInfoHeapInfo"));
		PVR_DBG_BREAK;
		eError = PVRSRV_ERROR_INVALID_HEAPINFO;
		goto ErrorExit;
	}

	/* Attempt to allocate sync object for TA/3D dependancies */
	PDUMPCOMMENT(psDevData->psConnection, "Allocate TA/3D dependency sync object", IMG_TRUE);
	if(PVRSRVAllocDeviceMem(psDevData,
							psSyncInfoHeapInfo->hDevMemHeap,
							PVRSRV_MEM_READ | PVRSRV_MEM_WRITE,
							sizeof(IMG_UINT32),
							0x10,
							&psRenderContext->psTA3DSyncObjectMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"Failed to allocate TA/3D dependency sync object!"));

		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorExit;
	}

	psRenderContext->psTA3DSyncObject = psRenderContext->psTA3DSyncObjectMemInfo->psClientSyncInfo;

#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH)
	PDUMPCOMMENT(psDevData->psConnection, "Allocate Master VDM SnapShot buffer memory", IMG_TRUE);
	/* Allocate memory for the VDM snap shot buffer */
	eError = PVRSRVAllocDeviceMem(psDevData,
								  psGeneralHeapInfo->hDevMemHeap,
								  PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ,
								  EURASIA_VDM_SNAPSHOT_BUFFER_SIZE,
								  (1 << EURASIA_VDM_SNAPSHOT_BUFFER_ALIGNSHIFT),
								  &psRenderContext->psMasterVDMSnapShotBufferMemInfo);
	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"Failed to alloc VDM CtxSwitch SnapShot memory!"));

		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorExit;
	}

	PVRSRVMemSet(psRenderContext->psMasterVDMSnapShotBufferMemInfo->pvLinAddr,
				 0,
				 EURASIA_VDM_SNAPSHOT_BUFFER_SIZE);

	PDUMPCOMMENT(psDevData->psConnection, "Initialise Master VDM SnapShot buffer memory", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL,
				psRenderContext->psMasterVDMSnapShotBufferMemInfo, 0,
				EURASIA_VDM_SNAPSHOT_BUFFER_SIZE, PDUMP_FLAGS_CONTINUOUS);
#endif
#if defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
	for (i = 0; i < SGX_FEATURE_MP_CORE_COUNT_TA; i++)
	{
		PDUMPCOMMENT(psDevData->psConnection, "Allocate VDM SnapShot buffer memory", IMG_TRUE);
		/* Allocate memory for the VDM snap shot buffer */
		eError = PVRSRVAllocDeviceMem(psDevData,
									  psGeneralHeapInfo->hDevMemHeap,
									  PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ,
									  EURASIA_VDM_SNAPSHOT_BUFFER_SIZE,
									  (1 << EURASIA_VDM_SNAPSHOT_BUFFER_ALIGNSHIFT),
									  &psRenderContext->psVDMSnapShotBufferMemInfo[i]);
		if(eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"Failed to alloc VDM CtxSwitch SnapShot memory!"));
	
			eError = PVRSRV_ERROR_OUT_OF_MEMORY;
			goto ErrorExit;
		}
	
		PVRSRVMemSet(psRenderContext->psVDMSnapShotBufferMemInfo[i]->pvLinAddr,
					 0,
					 EURASIA_VDM_SNAPSHOT_BUFFER_SIZE);
	
		
	#if defined (PDUMP)
		PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE,
				  "Initialise Core %i VDM SnapShot buffer memory", i);
	#endif
		PDUMPMEM(psDevData->psConnection, IMG_NULL,
					psRenderContext->psVDMSnapShotBufferMemInfo[i], 0,
					EURASIA_VDM_SNAPSHOT_BUFFER_SIZE, PDUMP_FLAGS_CONTINUOUS);
	}
	
#endif

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	#if defined(SGX_FEATURE_MTE_STATE_FLUSH)
		#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
	for (i = 0; i < SGX_FEATURE_MP_CORE_COUNT_TA; i++)
		#else
	i=0;
		#endif
	{
		PDUMPCOMMENT(psDevData->psConnection, "Allocate MTE state flush buffer memory", IMG_TRUE);
		/* Allocate memory for the MTE state buffer */
		eError = PVRSRVAllocDeviceMem(psDevData,
									  psGeneralHeapInfo->hDevMemHeap,
									  PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ,
									  EURASIA_MTE_STATE_BUFFER_SIZE,
									  (1 << EURASIA_MTE_STATE_BUFFER_ALIGN_SHIFT),
									  &psRenderContext->psMTEStateFlushMemInfo[i]);
		if(eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"Failed to alloc MTE state flush memory"));
			return eError;
		}
	
		PVRSRVMemSet(psRenderContext->psMTEStateFlushMemInfo[i]->pvLinAddr,
					 0,
					 EURASIA_MTE_STATE_BUFFER_SIZE);
	
		PDUMPCOMMENT(psDevData->psConnection, "Initialise MTE state flush buffer memory", IMG_TRUE);
		PDUMPMEM(psDevData->psConnection, IMG_NULL,
					psRenderContext->psMTEStateFlushMemInfo[i], 0,
					EURASIA_MTE_STATE_BUFFER_SIZE, PDUMP_FLAGS_CONTINUOUS);
	}
	#endif

	#if defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
	/* Round this up to the SA allocation granularity (16 DWORDS).
	 * Add 1 since the ukernel takes the first dword to store number of SA blocks */
	ui32SABufferSize = ALIGNSIZE(psCreateRenderContext->ui32MaxSACount + 1, EURASIA_TAPDSSTATE_USEATTRIBUTESIZE_ALIGNSHIFT);
	
	/* Convert into Bytes */
	ui32SABufferSize *= sizeof(IMG_UINT32);
	#if defined(SGX_FEATURE_MP)
	#if 0 // !SGX_SUPPORT_MASTER_ONLY_SWITCHING
	ui32SABufferSize *= SGX_FEATURE_MP_CORE_COUNT_TA;
	#endif
	#endif
	
		#if defined (PDUMP)
	PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE,
			  "Allocate MTE state flush buffer memory (%08X bytes)", ui32SABufferSize);
		#endif
	/* Allocate memory for the MTE state buffer */
	eError = PVRSRVAllocDeviceMem(psDevData,
								  psGeneralHeapInfo->hDevMemHeap,
								  PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ,
								  ui32SABufferSize,
								  1,
								  &psRenderContext->psSABufferMemInfo);
	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"Failed to alloc SA save buffer memory"));
		return eError;
	}

	PVRSRVMemSet(psRenderContext->psSABufferMemInfo->pvLinAddr,
				 0,
				 ui32SABufferSize);

	PDUMPCOMMENT(psDevData->psConnection, "Initialise Secondary Attribute Save Buffer memory", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL,
				psRenderContext->psSABufferMemInfo, 0,
				ui32SABufferSize, PDUMP_FLAGS_CONTINUOUS);
	#endif
#endif /* SGX_FEATURE_VDM_CONTEXT_SWITCH */
	
#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH)
	{
		IMG_UINT32	j;
		for( j=0; j < SGX_FEATURE_MP_CORE_COUNT_3D; j++)
		{
			IMG_UINT32	k;
			for(k=0; k<SGX_FEATURE_NUM_PDS_PIPES; k++)
			{
				/*
				 * allocate memory for the PDS state
				 */
				PDUMPCOMMENT(psDevData->psConnection, "Allocate memory for PDS context switch", IMG_TRUE);
				eError = PVRSRVAllocDeviceMem(psDevData,
											  psGeneralHeapInfo->hDevMemHeap,
											  PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ,
											  EURASIA_PDS_CONTEXT_SWITCH_BUFFER_SIZE,
									#if defined(EUR_CR_PDS_CONTEXT_STORE)
											  (1 << EUR_CR_PDS_CONTEXT_STORE_ADDRESS_SHIFT),
									#else
											  (1 << EUR_CR_PDS0_CONTEXT_STORE_ADDRESS_SHIFT),
									#endif
											  &psRenderContext->apsPDSCtxSwitchMemInfo[j][k]);
				if(eError != PVRSRV_OK)
				{
					PVR_DPF((PVR_DBG_ERROR,"Failed to alloc PDS Ctx Switch stream"));
					return eError;
				}
	
				PVRSRVMemSet(psRenderContext->apsPDSCtxSwitchMemInfo[j][k]->pvLinAddr,
						 0,
						 EURASIA_PDS_CONTEXT_SWITCH_BUFFER_SIZE);
	
				PDUMPCOMMENT(psDevData->psConnection, "Initialise PDS context switch memory", IMG_TRUE);
				PDUMPMEM(psDevData->psConnection, IMG_NULL,
							psRenderContext->apsPDSCtxSwitchMemInfo[j][k], 0,
							EURASIA_PDS_CONTEXT_SWITCH_BUFFER_SIZE, PDUMP_FLAGS_CONTINUOUS);
				
				psHWRenderContext->asPDSStateBaseDevAddr[j][k] = psRenderContext->apsPDSCtxSwitchMemInfo[j][k]->sDevVAddr;
			}
	
			PDUMPCOMMENT(psDevData->psConnection, "Allocate ZLS context switch memory", IMG_TRUE);
			/* Allocate memory for the ZLS tile on context switch */
			eError = PVRSRVAllocDeviceMem(psDevData,
										  psGeneralHeapInfo->hDevMemHeap,
										  PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ,
										  EURASIA_ISPREGION_SIZEX * EURASIA_ISPREGION_SIZEY * 5,
									#if defined(EUR_CR_ISP_CONTEXT_SWITCH)
										  (1 << EUR_CR_ISP_CONTEXT_SWITCH_ZSTORE_BASE_SHIFT),
									#else
										  (1 << EUR_CR_ISP2_CONTEXT_SWITCH_ZSTORE_BASE_SHIFT),
									#endif
										  &psRenderContext->psZLSCtxSwitchMemInfo[j]);
			if(eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"Failed to alloc ZLS Ctx Switch memory"));
				return eError;
			}
		
			PVRSRVMemSet(psRenderContext->psZLSCtxSwitchMemInfo[j]->pvLinAddr,
						 0,
						 (EURASIA_ISPREGION_SIZEX * EURASIA_ISPREGION_SIZEY * 5));
	
			PDUMPCOMMENT(psDevData->psConnection, "Initialise ZLS context switch memory", IMG_TRUE);
			PDUMPMEM(psDevData->psConnection, IMG_NULL, psRenderContext->psZLSCtxSwitchMemInfo[j], 0,
						(EURASIA_ISPREGION_SIZEX * EURASIA_ISPREGION_SIZEY * 5), PDUMP_FLAGS_CONTINUOUS);

			psHWRenderContext->sZLSCtxSwitchBaseDevAddr[j] = psRenderContext->psZLSCtxSwitchMemInfo[j]->sDevVAddr;
		}
	}
#else
	#if defined(SGX_SCRATCH_PRIM_BLOCK_ENABLE)
	PDUMPCOMMENT(psDevData->psConnection, "Allocate scratch primitive block", IMG_TRUE);
	/* Attempt to allocate scratch primitive block memory */
	if(PVRSRVAllocDeviceMem(psDevData,
							ps3DParamsHeapInfo->hDevMemHeap,
							PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ,
							64,
							64,
							&psRenderContext->psScratchPrimBlockClientMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"Failed to allocate primitive block!"));

		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorExit;
	}
	psHWRenderContext->sScratchPrimBlock = psRenderContext->psScratchPrimBlockClientMemInfo->sDevVAddr;
		#if defined(PDUMP)
	PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "Scratch prim block devVAddr: 0x%08X",
				psRenderContext->psScratchPrimBlockClientMemInfo->sDevVAddr.uiAddr);
		#endif /* PDUMP */
	#endif
#endif /* SGX_FEATURE_ISP_CONTEXT_SWITCH */

	if (psCreateRenderContext->ui32VisTestResultBufferSize > 0)
	{
#if defined(PDUMP)
		PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "Allocate Visibility test buffer (size == 0x%X)",
				psCreateRenderContext->ui32VisTestResultBufferSize * sizeof(IMG_UINT32));
#endif
		/* Attempt to allocate vistest result buffer */
		if(PVRSRVAllocDeviceMem(psDevData,
								psSyncInfoHeapInfo->hDevMemHeap,
								PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_EDM_PROTECT | PVRSRV_MEM_NO_SYNCOBJ,
								psCreateRenderContext->ui32VisTestResultBufferSize * sizeof(IMG_UINT32),
								32,
								&psRenderContext->psVisTestResultClientMemInfo) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"Failed to allocate vistest result buffer!"));
			eError = PVRSRV_ERROR_OUT_OF_MEMORY;
			goto ErrorExit;
		}

		PVRSRVMemSet(psRenderContext->psVisTestResultClientMemInfo->pvLinAddr,
						0,
						psCreateRenderContext->ui32VisTestResultBufferSize * sizeof(IMG_UINT32));
#if defined(PDUMP)
		PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "Initialise Visibility test buffer (size == 0x%X)",
				psCreateRenderContext->ui32VisTestResultBufferSize * sizeof(IMG_UINT32));
		PDUMPMEM(psDevData->psConnection, IMG_NULL,
				 psRenderContext->psVisTestResultClientMemInfo, 0,
				 psCreateRenderContext->ui32VisTestResultBufferSize * sizeof(IMG_UINT32), PDUMP_FLAGS_CONTINUOUS);
#endif

		*ppsVisTestResultClientMemInfo = psRenderContext->psVisTestResultClientMemInfo;
	}
	else
	{
		*ppsVisTestResultClientMemInfo = IMG_NULL;
	}

	/* Fill in the remaining SGXMKIF_HWRENDERCONTEXT members */
	psHWRenderContext->sCommon.ui32Flags = SGXMKIF_HWCFLAGS_NEWCONTEXT;

	psHWRenderContext->sTACCBBaseDevAddr = psRenderContext->psTACCB->psCCBClientMemInfo->sDevVAddr;
	psHWRenderContext->sTACCBCtlDevAddr.uiAddr = psRenderContext->psTACCB->psCCBCtlClientMemInfo->sDevVAddr.uiAddr;

	psHWRenderContext->ui32TACount = 0;

	if (psPBDesc == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "Fatal error PBDesc not valid, possible config error\n"));
		goto ErrorExit;
	}

	psHWRenderContext->sHWPBDescDevVAddr = psPBDesc->sHWPBDescDevVAddr;

#if defined(SUPPORT_SGX_PRIORITY_SCHEDULING)
	/* set the context scheduling priority to medium */
	psHWRenderContext->sCommon.ui32Priority = 1;
#else
	/* We only have 1 priority */
	psHWRenderContext->sCommon.ui32Priority = 0;
#endif

#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH)
	psHWRenderContext->sMasterVDMSnapShotBufferDevAddr.uiAddr =
						(psRenderContext->psMasterVDMSnapShotBufferMemInfo->sDevVAddr.uiAddr >>
							EURASIA_VDM_SNAPSHOT_BUFFER_ALIGNSHIFT);
							
	#if defined(PDUMP)
	PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE,
					"Master VDM snapshot buffer DevVAddr: 0x%08X",
					psHWRenderContext->sMasterVDMSnapShotBufferDevAddr.uiAddr);
	#endif
#endif
#if defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
	for (i=0; i < SGX_FEATURE_MP_CORE_COUNT_TA; i++)
	{
		psHWRenderContext->sVDMSnapShotBufferDevAddr[i].uiAddr =
						(psRenderContext->psVDMSnapShotBufferMemInfo[i]->sDevVAddr.uiAddr >>
							EURASIA_VDM_SNAPSHOT_BUFFER_ALIGNSHIFT);
			
		#if defined(PDUMP)
		PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE,
						"VDM snapshot buffer Core %i DevVAddr: 0x%08X", i,
						psHWRenderContext->sVDMSnapShotBufferDevAddr[i].uiAddr);
		#endif
	}
	
#endif
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	#if defined(SGX_FEATURE_MTE_STATE_FLUSH)
		#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
		for(i=0; i < SGX_FEATURE_MP_CORE_COUNT_TA; i++)
		#else
		i = 0;
		#endif
		{
			psHWRenderContext->sMTEStateFlushDevAddr[i].uiAddr =
						((psRenderContext->psMTEStateFlushMemInfo[i]->sDevVAddr.uiAddr >>
							EURASIA_MTE_STATE_BUFFER_ALIGN_SHIFT) << EUR_CR_MTE_STATE_FLUSH_BASE_ADDR_SHIFT);
		#if defined(PDUMP)
			PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "MTE state flush DevVAddr: 0x%08X", psHWRenderContext->sMTEStateFlushDevAddr[i].uiAddr);
		#endif
		}
	#endif
	#if defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
	psHWRenderContext->sSABufferDevAddr = psRenderContext->psSABufferMemInfo->sDevVAddr;
		#if defined(SGX_FEATURE_MP)
	psHWRenderContext->ui32SABufferStride = ui32SABufferSize / SGX_FEATURE_MP_CORE_COUNT_TA;
		#endif
		#if defined(PDUMP)
	PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "Secondary Attribute buffer DevVAddr: 0x%08X", psHWRenderContext->sSABufferDevAddr.uiAddr);
		#endif
	#endif
#endif
	/* Done, remember to set the pointer to the new context */
	*phRenderContext = psRenderContext;

	if(SGXGetInternalDevInfo(psDevData, &sSGXInternalDevInfo)
		!= PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXCreateRenderContext: Failed SGXGetInternalDevInfo"));
		goto ErrorExit;
	}

	psRenderContext->bForcePTOff = sSGXInternalDevInfo.bForcePTOff;

#if defined(PDUMP)
	psRenderContext->ui32KickTANum = 0;
	psRenderContext->ui32RenderNum = 0;
#endif

	/* setup the event object */
	psRenderContext->hOSEvent = psCreateRenderContext->hOSEvent;

	/* Save the PID that created the render context */
	psHWRenderContext->ui32PID = PVRSRVGetCurrentProcessID();

	/* Check if this is a per-context PB */
	if (psRenderContext->bPerContextPB)
		psHWRenderContext->sCommon.ui32Flags |= SGXMKIF_HWCFLAGS_PERCONTEXTPB;

	/* Register the HW render context for cleanup */
	eError = SGXRegisterHWRenderContext(
                psDevData, 
                &psRenderContext->hHWRenderContext, 
                (IMG_CPU_VIRTADDR *)psHWRenderContext,
                sizeof(SGXMKIF_HWRENDERCONTEXT),
                offsetof(SGXMKIF_HWRENDERCONTEXT, sCommon.sPDDevPAddr),
                psCreateRenderContext->hDevMemContext,
                &psRenderContext->sHWRenderContextDevVAddr);

    /* Free the User-Mem Copy of the HW Render Context now */
    PVRSRVFreeUserModeMem(psHWRenderContext);
    psHWRenderContext = IMG_NULL;

	if (eError != PVRSRV_OK)
	{
		goto ErrorExit;
	}

	/* Create the mutex */
	eError = PVRSRVCreateMutex(&psRenderContext->hMutex);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXCreateRenderContext: Failed to create render context mutex (%d)", eError));
		goto ErrorExit;
	}

	psRenderContext->ui32NumPixelPartitions = 0;
	psRenderContext->ui32NumVertexPartitions = 0;

	{
		IMG_VOID* pvHintState;
		IMG_UINT32 ui32AppHintDefault;
		PVRSRVCreateAppHintState(IMG_SRVCLIENT, 0, &pvHintState);
		ui32AppHintDefault = psRenderContext->ui32NumPixelPartitions;
		PVRSRVGetAppHint(pvHintState, "NumPixelPartitions", IMG_UINT_TYPE, &ui32AppHintDefault, &psRenderContext->ui32NumPixelPartitions);
		ui32AppHintDefault = psRenderContext->ui32NumVertexPartitions;
		PVRSRVGetAppHint(pvHintState, "NumVertexPartitions", IMG_UINT_TYPE, &ui32AppHintDefault, &psRenderContext->ui32NumVertexPartitions);

#if defined(DEBUG) || defined(PDUMP)
		{
			IMG_UINT32	ui32ReturnValue;
			ui32AppHintDefault = 0;
			if (PVRSRVGetAppHint(pvHintState, "TASync", IMG_UINT_TYPE, &ui32AppHintDefault, &ui32ReturnValue))
			{
				if (ui32ReturnValue != 0)
					psRenderContext->ui32TAKickRegFlags |= PVRSRV_SGX_TAREGFLAGS_TACOMPLETESYNC;
			}
			if (PVRSRVGetAppHint(pvHintState, "RenderSync", IMG_UINT_TYPE, &ui32AppHintDefault, &ui32ReturnValue))
			{
				if (ui32ReturnValue != 0)
					psRenderContext->ui32TAKickRegFlags |= PVRSRV_SGX_TAREGFLAGS_RENDERSYNC;
			}
		}
#endif
		PVRSRVFreeAppHintState(IMG_SRVCLIENT, pvHintState);
	}

	return PVRSRV_OK;

ErrorExit:

	/* Free whatever got allocated */
	if(!psRenderContext)
	{
		*phRenderContext = IMG_NULL;
		return eError;
	}

	if(psRenderContext->psVisTestResultClientMemInfo)
	{
		PVRSRVFreeDeviceMem(psDevData,
							psRenderContext->psVisTestResultClientMemInfo);
	}

#if defined(SGX_SCRATCH_PRIM_BLOCK_ENABLE)
	if(psRenderContext->psScratchPrimBlockClientMemInfo)
	{
		PVRSRVFreeDeviceMem(psDevData,
							psRenderContext->psScratchPrimBlockClientMemInfo);
	}
#endif
#if defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
	for (i=0; i < SGX_FEATURE_MP_CORE_COUNT_TA; i++)
	{
		if (psRenderContext->psVDMSnapShotBufferMemInfo[i])
		{
			PVRSRVFreeDeviceMem(psDevData,
								psRenderContext->psVDMSnapShotBufferMemInfo[i]);
		}
	}
#endif
#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH)
	if (psRenderContext->psMasterVDMSnapShotBufferMemInfo)
	{
		PVRSRVFreeDeviceMem(psDevData,
							psRenderContext->psMasterVDMSnapShotBufferMemInfo);
	}
#endif

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	#if defined(SGX_FEATURE_MTE_STATE_FLUSH)
		#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
	for (i=0; i < SGX_FEATURE_MP_CORE_COUNT_TA; i++)
		#else
	i=0;
		#endif
	{
		if (psRenderContext->psMTEStateFlushMemInfo[i])
		{
			PVRSRVFreeDeviceMem(psDevData,
								psRenderContext->psMTEStateFlushMemInfo[i]);
		}
	}
	#endif
	#if defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
	if (psRenderContext->psSABufferMemInfo)
	{
		PVRSRVFreeDeviceMem(psDevData,
							psRenderContext->psSABufferMemInfo);
	}
	#endif
#endif

	if(psRenderContext->psTA3DSyncObjectMemInfo)
	{
		PVRSRVFreeDeviceMem(psDevData,
							psRenderContext->psTA3DSyncObjectMemInfo);
	}

	if(psRenderContext->psTACCB)
	{
		DestroyCCB(psDevData, psRenderContext->psTACCB);
	}

#if defined(SUPPORT_PERCONTEXT_PB)
	if (psRenderContext->bPerContextPB)
		DestroyPerContextPB(psDevData, psClientPBDesc);
#endif
#if defined(SUPPORT_SHARED_PB)
	if (psRenderContext->bPerContextPB)
	{
		if(psPBDesc)
		{
			UnrefPB(psDevData, psClientPBDesc);
		}
		else if(hSharedPBDesc)
		{
			SGXUnrefSharedPBDesc(psDevData, hSharedPBDesc);
		}
	}
#endif

	PVRSRVFreeUserModeMem(psClientPBDesc);

	if (psRenderContext->hMutex != IMG_NULL)
	{
		(IMG_VOID) PVRSRVDestroyMutex(psRenderContext->hMutex);
	}

	PVRSRVFreeUserModeMem(psRenderContext);

	*phRenderContext = IMG_NULL;

	return eError;
}

/******************************************************************************
  End of file (sgxrender_context.c)
******************************************************************************/
