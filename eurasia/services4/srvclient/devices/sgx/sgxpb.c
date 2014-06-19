/*!****************************************************************************
@File           sgxpb.c

@Title          Paramter Buffer routines

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
$Log: sgxpb.c $
/
 --- Revision Logs Removed --- 
******************************************************************************/
#include <stddef.h>

#include "img_defs.h"
#include "services.h"
#include "sgxapi.h"
#include "sgxinfo.h"
#include "sgxinfo_client.h"
#include "sgxutils_client.h"
#include "sgxmmu.h"
#include "sgxpb.h"
#include "pvr_debug.h"
#include "pdump_um.h"
#include "sysinfo.h"
#include "sgx_bridge.h"
#include "sgx_bridge_um.h"
#include "servicesinit_um.h"
#include "osfunc_client.h"

typedef enum _PDBDESC_SUB_MEMINFO_KEY
{
	PBDESC_SUB_MEMINFO_PB,
	PBDESC_SUB_MEMINFO_EVM_PAGE_TABLE,
	PBDESC_SUB_MEMINFO_LAST_KEY
}PDBDESC_SUB_MEMINFO_KEY;		/* PRQA S 3205 */ /* used as array indexing */

/*****************************************************************************
 FUNCTION	: SetupPBDesc

 PURPOSE	: Calculates all the values for the PB and stores
 				them in the host version of the PB descriptor

 PARAMETERS	:

 RETURNS	: None
*****************************************************************************/
static IMG_VOID SetupPBDesc(const PVRSRV_DEV_DATA	*psDevData,
									SGX_PBDESC			*psPBDesc)
{
	IMG_UINT16			ui16PBHead;
	IMG_UINT16			ui16PBTail;
	IMG_UINT32			ui32NumPBPages = 0;
	IMG_UINT32			ui32TotalPBSize;
	SGX_PBBLOCK			*psPBBlock;
	IMG_VOID			*pvHintState;

	PVR_UNREFERENCED_PARAMETER(psDevData);

	psPBBlock = psPBDesc->psListPBBlockHead;
	ui16PBHead = psPBBlock->ui16Head;

	while (psPBBlock)
	{
		/* Add up the pages and record the start and end pages.	*/
		ui32NumPBPages += psPBBlock->ui32PageCount;

		psPBBlock = psPBBlock->psNext;
	}

#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
	if (psPBDesc->bGrowShrink)
	{
		psPBBlock = psPBDesc->psGrowListPBBlockHead;
		while (psPBBlock)
		{
			/* Add up the pages and record the start and end pages.	*/
			ui32NumPBPages += psPBBlock->ui32PageCount;

			psPBBlock = psPBBlock->psNext;
		}

		if (psPBDesc->psGrowListPBBlockTail != IMG_NULL)
		{
			ui16PBTail = psPBDesc->psGrowListPBBlockTail->ui16Tail;
		}
		else
		{
			ui16PBTail = psPBDesc->psListPBBlockTail->ui16Tail;
		}
	}
	else
#endif
	{
		ui16PBTail = psPBDesc->psListPBBlockTail->ui16Tail;
	}

	/* PRQA S 3382 12 */ /* warning about wraparound past zero */
#if defined(FIX_HW_BRN_23055)
	ui32TotalPBSize = ui32NumPBPages - BRN23055_BUFFER_PAGE_SIZE;
#else
	ui32TotalPBSize = ui32NumPBPages;
#endif /* defined(FIX_HW_BRN_23055) */

	/* SPM_PAGE_INJECTION */
	ui32TotalPBSize -= (SPM_DL_RESERVE_PAGES + EURASIA_PARAM_FREE_LIST_RESERVE_PAGES);

	/* Save the info in PBDESC */
	psPBDesc->ui32TotalPBSize = ui32TotalPBSize * EURASIA_PARAM_MANAGE_GRAN;
	psPBDesc->ui32NumPages = ui32NumPBPages;

	psPBDesc->ui32FreeListInitialHT = (IMG_UINT32)(ui16PBHead << EUR_CR_DPM_TA_ALLOC_FREE_LIST_HEAD_SHIFT)
										| (IMG_UINT32)(ui16PBTail << EUR_CR_DPM_TA_ALLOC_FREE_LIST_TAIL_SHIFT);
#if defined(FIX_HW_BRN_23281)
	psPBDesc->ui32FreeListInitialPrev = (IMG_UINT32)(ui16PBTail-2) << EUR_CR_DPM_TA_ALLOC_FREE_LIST_PREVIOUS_SHIFT;
#else
	psPBDesc->ui32FreeListInitialPrev = (IMG_UINT32)(ui16PBTail-1) << EUR_CR_DPM_TA_ALLOC_FREE_LIST_PREVIOUS_SHIFT;
#endif

	/* ZLSThreshold = AllocatedPBSize - 23055 workaround - deadlock reserve */
	psPBDesc->ui32ZLSThreshold = psPBDesc->ui32TotalPBSize / EURASIA_PARAM_MANAGE_GRAN;

	/* Set the TA and Global thresholds */
	psPBDesc->ui32TAThreshold = ui32NumPBPages + 0x10;

#if defined(FIX_HW_BRN_23055)
	psPBDesc->ui32GlobalThreshold = ui32NumPBPages + 0x10;
	/* The threshold required is based on:
	   ZLSThreshold - (MaxRTSize / 8960) + 2(DPM) + 2(MTE) */
	if( psPBDesc->ui32ZLSThreshold > BRN23055_BUFFER_PAGE_SIZE)
	{
		psPBDesc->ui32PDSThreshold =
						psPBDesc->ui32ZLSThreshold - BRN23055_BUFFER_PAGE_SIZE;
	}
	else
	{
		psPBDesc->ui32PDSThreshold = 0;
	}
#else
	/* Until a RT using DPMZLS is allocated set the global threshold to the size of the PB */
	psPBDesc->ui32GlobalThreshold = psPBDesc->ui32TotalPBSize / EURASIA_PARAM_MANAGE_GRAN;
	
	psPBDesc->ui32PDSThreshold = MIN(psPBDesc->ui32TAThreshold, psPBDesc->ui32ZLSThreshold);
	psPBDesc->ui32PDSThreshold = MIN(psPBDesc->ui32PDSThreshold, psPBDesc->ui32GlobalThreshold);
	if(psPBDesc->ui32PDSThreshold > (IMG_UINT32)EURASIA_PARAM_PDS_SAFETY_MARGIN)
	{
		psPBDesc->ui32PDSThreshold -= (IMG_UINT32)(EURASIA_PARAM_PDS_SAFETY_MARGIN);
	}
	else
	{
		psPBDesc->ui32PDSThreshold = 0;
	}
#endif /* defined(FIX_HW_BRN_23055) */

	/* Check if registry settings to override stuff are present */
	PVRSRVCreateAppHintState(IMG_SRVCLIENT, 0, &pvHintState);

	if (PVRSRVGetAppHint(pvHintState, "TAThreshold", IMG_UINT_TYPE, &psPBDesc->ui32TAThreshold, &psPBDesc->ui32TAThreshold))
	{
		psPBDesc->ui32Flags |= SGX_PBDESCFLAGS_TA_OVERRIDE;
	}
	if (PVRSRVGetAppHint(pvHintState, "ZLSThreshold", IMG_UINT_TYPE, &psPBDesc->ui32ZLSThreshold, &psPBDesc->ui32ZLSThreshold))
	{
		psPBDesc->ui32Flags |= SGX_PBDESCFLAGS_ZLS_OVERRIDE;
	}
	if (PVRSRVGetAppHint(pvHintState, "GlobalThreshold", IMG_UINT_TYPE, &psPBDesc->ui32GlobalThreshold, &psPBDesc->ui32GlobalThreshold))
	{
		psPBDesc->ui32Flags |= SGX_PBDESCFLAGS_GLOBAL_OVERRIDE;
	}
	if (PVRSRVGetAppHint(pvHintState, "PDSThreshold", IMG_UINT_TYPE, &psPBDesc->ui32PDSThreshold, &psPBDesc->ui32PDSThreshold))
	{
		psPBDesc->ui32Flags |= SGX_PBDESCFLAGS_PDS_OVERRIDE;
	}

	PVRSRVFreeAppHintState(IMG_SRVCLIENT, pvHintState);
}

/*****************************************************************************
 FUNCTION	: SetupHWPBDesc

 PURPOSE	: Copies the latest version of the value for the PB
 				from the host version of the PB descriptor to the hardware
 				version

 PARAMETERS	:

 RETURNS	: None
*****************************************************************************/
static IMG_VOID SetupHWPBDesc(const PVRSRV_DEV_DATA	*psDevData,
										SGX_PBDESC			*psPBDesc,
										PVRSRV_CLIENT_MEM_INFO	*psEVMPageTableClientMemInfo,
										PVRSRV_CLIENT_MEM_INFO	*psHWPBDescClientMemInfo)
{
	SGXMKIF_HWPBDESC	*psHWPBDesc;

	PVR_UNREFERENCED_PARAMETER(psDevData);
	PVR_UNREFERENCED_PARAMETER(psHWPBDescClientMemInfo);

	/* Setup state that will be used to 'load' the PB */
	psHWPBDesc = psPBDesc->psHWPBDesc;

	/* THIS HAS TO BE THE FIRST MEMBER*/
	/* FIXME pass down the offset to the kernel*/
	psHWPBDesc->ui32PBFlags = 0;

	/* KEEP THESE 4 VARIABLES TOGETHER FOR UKERNEL BLOCK LOAD */
	psHWPBDesc->ui32NumPages = psPBDesc->ui32NumPages;
	psHWPBDesc->ui32FreeListInitialHT =	psPBDesc->ui32FreeListInitialHT;
	psHWPBDesc->ui32FreeListInitialPrev = psPBDesc->ui32FreeListInitialPrev;
	psHWPBDesc->sEVMPageTableDevVAddr = psEVMPageTableClientMemInfo->sDevVAddr;

	psHWPBDesc->ui32FreeListHT = psHWPBDesc->ui32FreeListInitialHT;
	psHWPBDesc->ui32FreeListPrev = psHWPBDesc->ui32FreeListInitialPrev;

	/* Initialise the thresholds */
	psHWPBDesc->ui32TAThreshold 	= psPBDesc->ui32TAThreshold;
	psHWPBDesc->ui32ZLSThreshold 	= psPBDesc->ui32ZLSThreshold;
	psHWPBDesc->ui32GlobalThreshold	= psPBDesc->ui32GlobalThreshold;
	psHWPBDesc->ui32PDSThreshold 	= psPBDesc->ui32PDSThreshold;

	PDUMPCOMMENT(psDevData->psConnection, "Initialise HWPB Descriptor", IMG_TRUE);	/* PRQA S 0505 */ /* ignore warning about pointer */
	PDUMPMEM(psDevData->psConnection,
			 IMG_NULL,
			 psHWPBDescClientMemInfo,
			 0,
			 sizeof(SGXMKIF_HWPBDESC),
			 PDUMP_FLAGS_CONTINUOUS);
}

/*****************************************************************************
 FUNCTION	: InitialisePBBlockPageTable

 PURPOSE	: Initialises the DPM page table entries for the PB block supplied

 PARAMETERS	:

 RETURNS	: None
*****************************************************************************/
static IMG_VOID InitialisePBBlockPageTable(const PVRSRV_DEV_DATA	*psDevData,
											PVRSRV_CLIENT_MEM_INFO *psEVMPageTableClientMemInfo,
											SGX_PBBLOCK	*psBlock)
{
	IMG_UINT32	*psDPMPageTable;
	IMG_UINT16	ui16Prev = 0xFFFF;
	IMG_UINT16	ui16Page = psBlock->ui16Head;
	IMG_UINT16	ui16Next;
	IMG_UINT32	i;
	
	PVR_UNREFERENCED_PARAMETER(psDevData);
	
	/* Initialise the DPM page table entries for the PB Block */
	psDPMPageTable = psEVMPageTableClientMemInfo->pvLinAddr;
	
	for (i = 0; i < psBlock->ui32PageCount; i++)
	{
	#if defined(FIX_HW_BRN_23281)
		ui16Next = ui16Page + 2;
	#else
		ui16Next = ui16Page + 1;
	#endif
		
		psDPMPageTable[ui16Page] = (((IMG_UINT32)ui16Prev << EURASIA_PARAM_MANAGE_TABLE_PREV_SHIFT) |
									((IMG_UINT32)ui16Next << EURASIA_PARAM_MANAGE_TABLE_NEXT_SHIFT));

	#if defined(FIX_HW_BRN_23281)
		/* Show that this page table entry is invalid. */
		psDPMPageTable[ui16Page+1] = 0xFFFFFFFF;
	#endif

		ui16Prev = ui16Page;
		
	#if defined(FIX_HW_BRN_23281)
		ui16Page += 2;
	#else
		ui16Page += 1;
	#endif
	}
	
	/* Patch the last entry written with the terminate */
	psDPMPageTable[ui16Prev] = ((psDPMPageTable[ui16Prev] & EURASIA_PARAM_MANAGE_TABLE_PREV_MASK) |
								(EURASIA_PARAM_MANAGE_TABLE_NEXT_MASK));

	PDUMPCOMMENT(psDevData->psConnection, "Initialise DPM Page table for PB Block\r\n", IMG_TRUE);

	PDUMPMEM(psDevData->psConnection, IMG_NULL, 
			psEVMPageTableClientMemInfo,
			psBlock->ui16Head * sizeof(IMG_UINT32), 
			(IMG_UINT32) (ui16Page - psBlock->ui16Head) * sizeof(IMG_UINT32), PDUMP_FLAGS_CONTINUOUS);

}

/*****************************************************************************
 FUNCTION	: DestroyPBBlock

 PURPOSE	: Destroys all the memory associated with a parameter buffer block

 PARAMETERS	:

 RETURNS	: PVRSRV_ERROR
*****************************************************************************/
static PVRSRV_ERROR DestroyPBBlock(const PVRSRV_DEV_DATA *psDevData,
									SGX_PBDESC	*psPBDesc,
									PVRSRV_CLIENT_MEM_INFO **ppsTmpPBBlockClientMemInfo,
									SGX_PBBLOCK	*psPBBlock)
{
#if !defined(SUPPORT_SHARED_PB)
	PVR_UNREFERENCED_PARAMETER(ppsTmpPBBlockClientMemInfo);
#endif

	if (psPBBlock != IMG_NULL)
	{
		if(psPBBlock->psPBClientMemInfo)
		{
			PVRSRVFreeDeviceMem(psDevData, psPBBlock->psPBClientMemInfo);
		}

		if(psPBBlock->psHWPBBlockClientMemInfo)
		{
			PVRSRVFreeDeviceMem(psDevData, psPBBlock->psHWPBBlockClientMemInfo);
		}

		if (psPBBlock->psPrev != IMG_NULL)
		{
			psPBBlock->psPrev->psNext = psPBBlock->psNext;
		}
		else
		{
			/* It is a head */
			if (psPBDesc->psListPBBlockHead == psPBBlock)
			{
				psPBDesc->psListPBBlockHead = psPBBlock->psNext;
			}
#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
			else if (psPBDesc->psGrowListPBBlockHead == psPBBlock)
			{
				psPBDesc->psGrowListPBBlockHead = psPBBlock->psNext;
			}
			else if (psPBDesc->psShrinkListPBBlock == psPBBlock)
			{
				psPBDesc->psShrinkListPBBlock = psPBBlock->psNext;
			}
#endif
		}

		if (psPBBlock->psNext != IMG_NULL)
		{
			psPBBlock->psNext->psPrev = psPBBlock->psPrev;
		}
		else
		{
			/* It is a tail */
			if (psPBDesc->psListPBBlockTail == psPBBlock)
			{
				psPBDesc->psListPBBlockTail = psPBBlock->psPrev;
			}
#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
			else if (psPBDesc->psGrowListPBBlockTail == psPBBlock)
			{
				psPBDesc->psGrowListPBBlockTail = psPBBlock->psPrev;
			}
#endif
		}

#if defined(SUPPORT_PERCONTEXT_PB)
		if (psPBDesc->bPerContextPB)
		{
			PVRSRVFreeUserModeMem(psPBBlock);
		}
#endif
#if defined(SUPPORT_SHARED_PB)
		if (!psPBDesc->bPerContextPB)
		{
			if (*ppsTmpPBBlockClientMemInfo != IMG_NULL)
			{
				PVRSRVFreeSharedSysMem(psDevData->psConnection,
								   		*ppsTmpPBBlockClientMemInfo);
			}
		}
#endif
	}

	return PVRSRV_OK;
}

/*****************************************************************************
 FUNCTION	: CreatePBBlock

 PURPOSE	: Allocate all the memory associated with a parameter buffer
 				block and adds the block onto the tail of the grow block list

 PARAMETERS	:

 RETURNS	: PVRSRV_ERROR
*****************************************************************************/
static PVRSRV_ERROR CreatePBBlock(const PVRSRV_DEV_DATA *psDevData,
									SGX_PBDESC	*psPBDesc,
									IMG_UINT32 ui32ParamSize,
									PVRSRV_CLIENT_MEM_INFO **ppsTmpPBBlockClientMemInfo,
									const PVRSRV_HEAP_INFO *psPBHeapInfo,
									const PVRSRV_HEAP_INFO *psKernelVideoDataHeapInfo)
{
	IMG_UINT32	ui32MemFlags = 0;
	IMG_VOID	* pvHintState;
	IMG_BOOL 	bEDMProtectTATest;
	IMG_BOOL 	bAppHintDefault = IMG_FALSE;
	SGX_PBBLOCK * psPBBlock = IMG_NULL;

	/* Alloc the initial PB Block structure */
#if defined(SUPPORT_PERCONTEXT_PB)
	if (psPBDesc->bPerContextPB)
	{
		/* The structure needs to be zeroed as it can jump (on error) to 
		 * DestroyPBBlock at any time during initialization.
		 */
		psPBBlock = PVRSRVCallocUserModeMem(sizeof(SGX_PBBLOCK));
		if(psPBBlock == IMG_NULL)
		{
			PVR_DPF((PVR_DBG_ERROR, "CreatePBBlock: Failed to alloc host mem for PB block\n"));
			goto ErrorExit;
		}
	}
#endif
#if defined(SUPPORT_SHARED_PB)
	if (!psPBDesc->bPerContextPB)
	{
		psPBBlock = IMG_NULL;
		if(PVRSRVAllocSharedSysMem(psDevData->psConnection,
								   0,
								   sizeof(SGX_PBDESC),
								   ppsTmpPBBlockClientMemInfo) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "CreatePBBlock: Failed to alloc shared sys mem for PB block\n"));
			goto ErrorExit;
		}

		psPBBlock = (SGX_PBBLOCK *)(*ppsTmpPBBlockClientMemInfo)->pvLinAddr;

		/* The structure needs to be zeroed as it can jump (on error) to 
		 * DestroyPBBlock at any time during initialization.
		 */	
		PVRSRVMemSet(psPBBlock, 0, sizeof(SGX_PBBLOCK));
	}
#endif

	if (psPBBlock == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "CreatePBBlock: Fatal error PBBlock not valid, possible config error\n"));
		goto ErrorExit;
	}

	psPBBlock->psParentPBDesc = psPBDesc;
	psPBBlock->psNext = IMG_NULL;

	#if defined(SUPPORT_PERCONTEXT_PB)
	if (psPBDesc->bPerContextPB)
	{
		ui32MemFlags = PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ;
	}
	#endif
	#if defined(SUPPORT_SHARED_PB)
	if (!psPBDesc->bPerContextPB)
	{
		ui32MemFlags = PVRSRV_MEM_READ | PVRSRV_MEM_WRITE |
						PVRSRV_MEM_NO_SYNCOBJ | PVRSRV_MEM_NO_RESMAN;
	}
	#endif
	/*  Allocate the HW PB Block */
	if(PVRSRVAllocDeviceMem_log(psDevData,
							psKernelVideoDataHeapInfo->hDevMemHeap,
							ui32MemFlags | PVRSRV_MEM_CACHE_CONSISTENT,
							sizeof(SGXMKIF_HWPBBLOCK),
							4,
							&psPBBlock->psHWPBBlockClientMemInfo,
							"Hardware Parameter Buffer Block Client Memory Info" ) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"CreatePBBlock: Failed to alloc mem for HWPBBlock"));
		goto ErrorExit;
	}

	psPBBlock->psHWPBBlock = psPBBlock->psHWPBBlockClientMemInfo->pvLinAddr;
	psPBBlock->sHWPBBlockDevVAddr = psPBBlock->psHWPBBlockClientMemInfo->sDevVAddr;

#if defined(FIX_HW_BRN_23281)
	ui32MemFlags |= PVRSRV_MEM_INTERLEAVED;
#endif

	PVRSRVCreateAppHintState(IMG_SRVCLIENT, 0, &pvHintState);
	PVRSRVGetAppHint(pvHintState, "EDMProtectTATest", IMG_UINT_TYPE, &bAppHintDefault, &bEDMProtectTATest);
	PVRSRVFreeAppHintState(IMG_SRVCLIENT, pvHintState);

	if (bEDMProtectTATest)
	{
		ui32MemFlags |= PVRSRV_MEM_EDM_PROTECT;
	}

	/* Allocate the PB Block Pages */
	if(PVRSRVAllocDeviceMem_log(psDevData,
							psPBHeapInfo->hDevMemHeap,
							ui32MemFlags,
							ui32ParamSize,
							EURASIA_PARAM_MANAGE_GRAN,
							&psPBBlock->psPBClientMemInfo,
							"Parameter Buffer Block Pages Client Memory Info") != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "CreatePBBlock: Failed to allocate PB block of size: %08X",
				 ui32ParamSize));
		goto ErrorExit;
	}

	psPBBlock->ui32PageCount = (IMG_UINT32)(psPBBlock->psPBClientMemInfo->uAllocSize / EURASIA_PARAM_MANAGE_GRAN);

	/* find the first PT entry corresponding to the first page of the PB block */
	psPBBlock->ui16Head = (IMG_UINT16)((psPBBlock->psPBClientMemInfo->sDevVAddr.uiAddr -
							psPBDesc->sParamHeapBaseDevVAddr.uiAddr) / EURASIA_PARAM_MANAGE_GRAN);

	/* find the last PT entry corresponding to the last page of the PB block */
#if defined(FIX_HW_BRN_23281)
	/* the last page is twice as far in interleaved mode*/
	psPBBlock->ui16Tail = (IMG_UINT16)(psPBBlock->ui16Head + (psPBBlock->ui32PageCount << 1) - 2);
#else
	psPBBlock->ui16Tail = (IMG_UINT16)(psPBBlock->ui16Head + psPBBlock->ui32PageCount - 1);
#endif /* FIX_HW_BRN_23281*/

	/* make sure that the PT fits into the alloated place*/
	/* FIXME: this comparison is always true */
	/* PVR_ASSERT(psPBBlock->ui16Tail < EURASIA_PARAMETER_BUFFER_SIZE_IN_PAGES); */ /* PRQA S 3355,3358 */ /* thinks it's always true */

	/* Fill in the HWPBBlock */
	psPBBlock->psHWPBBlock->ui32PageCount = psPBBlock->ui32PageCount;
	psPBBlock->psHWPBBlock->ui16Head = psPBBlock->ui16Head;
	psPBBlock->psHWPBBlock->ui16Tail = psPBBlock->ui16Tail;
	psPBBlock->psHWPBBlock->sParentHWPBDescDevVAddr = psPBDesc->sHWPBDescDevVAddr;
	psPBBlock->psHWPBBlock->sNextHWPBBlockDevVAddr.uiAddr = 0;

	PDUMPCOMMENT(psDevData->psConnection, "Initialise HW PB Block", IMG_TRUE);	/* PRQA S 505 */
	PDUMPMEM(psDevData->psConnection,
			 IMG_NULL, psPBBlock->psHWPBBlockClientMemInfo,
			 0, sizeof(SGXMKIF_HWPBBLOCK),
			 PDUMP_FLAGS_CONTINUOUS);

#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
	if (psPBDesc->bGrowShrink)
	{
		if (psPBDesc->psGrowListPBBlockTail == IMG_NULL)
		{
			psPBBlock->psPrev = IMG_NULL;
			psPBDesc->psGrowListPBBlockHead = psPBBlock;
			psPBDesc->psGrowListPBBlockTail = psPBBlock;
		}
		else
		{
			psPBBlock->psPrev = psPBDesc->psGrowListPBBlockTail;
			psPBDesc->psGrowListPBBlockTail->psNext = psPBBlock;
			psPBDesc->psGrowListPBBlockTail = psPBBlock;
		}
	}
#endif
#if !defined(FORCE_ENABLE_GROW_SHRINK) || defined(SUPPORT_SHARED_PB)
	if (!psPBDesc->bGrowShrink)
	{
		if (psPBDesc->psListPBBlockTail == IMG_NULL)
		{
			psPBBlock->psPrev = IMG_NULL;
			psPBDesc->psListPBBlockHead = psPBBlock;
			psPBDesc->psListPBBlockTail = psPBBlock;
		}
		else
		{
			psPBBlock->psPrev = psPBDesc->psListPBBlockTail;
			psPBDesc->psListPBBlockTail->psNext = psPBBlock;
			psPBDesc->psListPBBlockTail = psPBBlock;
		}
	}
#endif

	return PVRSRV_OK;
ErrorExit:
	DestroyPBBlock(psDevData, psPBDesc,
					ppsTmpPBBlockClientMemInfo,
					psPBBlock);

	
	if(IMG_NULL != ppsTmpPBBlockClientMemInfo){
        	*ppsTmpPBBlockClientMemInfo = IMG_NULL;
        }

	return PVRSRV_ERROR_OUT_OF_MEMORY;
}

#if defined(SUPPORT_PERCONTEXT_PB)
/*****************************************************************************
 FUNCTION	: DestroyPerContextPB

 PURPOSE	: Destroys a per render context parameter buffer and all the memory
 				associated with it

 PARAMETERS	:

 RETURNS	: None
*****************************************************************************/
IMG_INTERNAL IMG_VOID DestroyPerContextPB(const PVRSRV_DEV_DATA *psDevData,
									SGX_CLIENTPBDESC *psClientPBDesc)
{
	SGX_PBDESC *psPBDesc = psClientPBDesc->psPBDesc;

	if(psClientPBDesc->psEVMPageTableClientMemInfo)
	{
		PVRSRVFreeDeviceMem(psDevData, psClientPBDesc->psEVMPageTableClientMemInfo);
		psClientPBDesc->psEVMPageTableClientMemInfo = IMG_NULL;
	}

	if(psClientPBDesc->psHWPBDescClientMemInfo)
	{
		PVRSRVFreeDeviceMem(psDevData, psClientPBDesc->psHWPBDescClientMemInfo);
		psClientPBDesc->psHWPBDescClientMemInfo = IMG_NULL;
	}

	if (psPBDesc)
	{
		SGX_PBBLOCK *psPBBlock = psClientPBDesc->psPBDesc->psListPBBlockHead;
		SGX_PBBLOCK *psNextPBBlock;

		while (psPBBlock != IMG_NULL)
		{
			psNextPBBlock = psPBBlock->psNext;
			DestroyPBBlock(psDevData, psPBDesc, IMG_NULL, psPBBlock);
			psPBBlock = psNextPBBlock;
		}

#if defined(FORCE_ENABLE_GROW_SHRINK)
		psPBBlock = psClientPBDesc->psPBDesc->psGrowListPBBlockHead;

		/* Free all the block in the grow list */
		while (psPBBlock != IMG_NULL)
		{
			psNextPBBlock = psPBBlock->psNext;
			DestroyPBBlock(psDevData, psPBDesc, IMG_NULL, psPBBlock);
			psPBBlock = psNextPBBlock;
		}
		
		/* Free the shrink block if there is one */
		if (psPBDesc->psShrinkListPBBlock != IMG_NULL)
		{
			DestroyPBBlock(psDevData, psPBDesc, IMG_NULL, psPBDesc->psShrinkListPBBlock);
		}
#endif

		PVRSRVFreeUserModeMem(psPBDesc);
		psClientPBDesc->psPBDesc = IMG_NULL;
	}
}

/*****************************************************************************
 FUNCTION	: CreatePerContextPB

 PURPOSE	: Creates a per render context parameter buffer and all the memory
 				associated with it

 PARAMETERS	:

 RETURNS	: PVRSRV_ERROR
*****************************************************************************/
IMG_INTERNAL PVRSRV_ERROR CreatePerContextPB(const PVRSRV_DEV_DATA *psDevData,
										SGX_RENDERCONTEXT *psRenderContext,
										IMG_UINT32 ui32ParamSize,
								#if defined(FORCE_ENABLE_GROW_SHRINK)
										IMG_UINT32	ui23ParamSizeLimit,
								#endif
										const PVRSRV_HEAP_INFO *psPBHeapInfo,
										const PVRSRV_HEAP_INFO *psKernelVideoDataHeapInfo)
{
	PVRSRV_ERROR			eError;
	SGX_PBDESC				*psPBDesc;
	SGXMKIF_HWPBDESC		*psHWPBDesc;
	SGX_CLIENTPBDESC		*psClientPBDesc;

	/* get the client PB desc */
	psClientPBDesc = psRenderContext->psClientPBDesc;

#if defined(FORCE_ENABLE_GROW_SHRINK)
	if ((ui23ParamSizeLimit != 0) && (ui23ParamSizeLimit < ui32ParamSize))
	{
		PVR_DPF((PVR_DBG_ERROR,"CreatePerContextPB: if PBSizeLimit is non-zero it must be >= ui32PBSize"));
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto ErrorExit;
	}		
#endif

	/* Allocate the PB Desc */
	psPBDesc = PVRSRVAllocUserModeMem(sizeof(SGX_PBDESC));
	if(!psPBDesc)
	{
		PVR_DPF((PVR_DBG_ERROR, "CreatePerContextPB: Failed to allocate PBDesc\n"));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorExit;
	}

	psClientPBDesc->psPBDesc 		 = psPBDesc;
	psPBDesc->bPerContextPB			 = IMG_TRUE;
#if defined(FORCE_ENABLE_GROW_SHRINK)
	psPBDesc->bGrowShrink			 = IMG_TRUE;
#else
	psPBDesc->bGrowShrink			 = IMG_FALSE;
#endif
	psPBDesc->ui32Flags 			 = 0;
	psPBDesc->sParamHeapBaseDevVAddr = psPBHeapInfo->sDevVAddrBase;
	psPBDesc->psListPBBlockHead 	 = IMG_NULL;
	psPBDesc->psListPBBlockTail 	 = IMG_NULL;

#if defined(FORCE_ENABLE_GROW_SHRINK)
	psPBDesc->psGrowListPBBlockHead  = IMG_NULL;
	psPBDesc->psGrowListPBBlockTail  = IMG_NULL;
	psPBDesc->psShrinkListPBBlock	 = IMG_NULL;
	psPBDesc->ui32BusySceneCount 	 = 0;
	psPBDesc->ui32RenderCount		 = 0;
	psPBDesc->ui32ShrinkRender		 = 0;
	psPBDesc->ui32PBSizeLimit		 = ui23ParamSizeLimit;
	/* Calculate how big each block should be */
	if (ui23ParamSizeLimit != 0)
	{
		psPBDesc->ui32PBGrowBlockSize	 = (ui23ParamSizeLimit - ui32ParamSize) / SGXPB_GROW_SHRINK_NUM_BLOCKS;
		if (psPBDesc->ui32PBGrowBlockSize < SGXPB_GROW_MIN_BLOCK_SIZE)
		{
			psPBDesc->ui32PBGrowBlockSize	 = SGXPB_GROW_MIN_BLOCK_SIZE;
		}
	}
	else
	{
		psPBDesc->ui32PBGrowBlockSize	 = SGXPB_GROW_MIN_BLOCK_SIZE;
	}
	psPBDesc->ui32PBGrowBlockSize 	= GET_ALIGNED_PB_SIZE(psPBDesc->ui32PBGrowBlockSize)
	#if defined(PDUMP)
	psPBDesc->bPdumpRequired		= IMG_TRUE;
	#endif
#endif

	/* Allocate memory for HW PB Descriptor */
	if(PVRSRVAllocDeviceMem_log(psDevData,
							psKernelVideoDataHeapInfo->hDevMemHeap,
							PVRSRV_MEM_READ|PVRSRV_MEM_WRITE|PVRSRV_MEM_NO_SYNCOBJ|PVRSRV_MEM_CACHE_CONSISTENT,
							sizeof(SGXMKIF_HWPBDESC),
							32,
							&psClientPBDesc->psHWPBDescClientMemInfo,
							"Hardware Parameter Buffer Descriptor Client Memory Info") != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"CreatePerContextPB: Failed to alloc mem for HW PB Descriptor!"));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorExit;
	}

	psHWPBDesc =  psClientPBDesc->psHWPBDescClientMemInfo->pvLinAddr;
	psPBDesc->psHWPBDesc = psHWPBDesc;
	psPBDesc->sHWPBDescDevVAddr = psClientPBDesc->psHWPBDescClientMemInfo->sDevVAddr;

	PVRSRVMemSet(psHWPBDesc, 0, sizeof(SGXMKIF_HWPBDESC));

#if defined (PDUMP)
	PDUMPCOMMENTF(psDevData->psConnection,
				  IMG_TRUE,
				  "Requested Parameter Buffer Size: %08X", ui32ParamSize);
#endif
	PVR_DPF((PVR_DBG_MESSAGE,
			 "CreatePerContextPB: Requested Parameter Buffer Size: %08X",
			 ui32ParamSize));

#if defined(FIX_HW_BRN_23055)
	/* Add on the pages held in reserve for the 23055 workaround */
	ui32ParamSize += (BRN23055_BUFFER_PAGE_SIZE * EURASIA_PARAM_MANAGE_GRAN);
#endif /* defined(FIX_HW_BRN_23055) */

	/* SPM_PAGE_INJECTION */
	/* Add on the pages held in reserve for the SPM deadlock condition */
	ui32ParamSize += (SPM_DL_RESERVE_SIZE + EURASIA_PARAM_FREE_LIST_RESERVE_SIZE);

	if (CreatePBBlock(psDevData, psPBDesc, ui32ParamSize, IMG_NULL,
						psPBHeapInfo, psKernelVideoDataHeapInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "CreatePerContextPB: Failed to allocate PB Block"));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorExit;
	}

#if defined(FORCE_ENABLE_GROW_SHRINK)
	/* psListPBBlock = psGrowListPBBlock as the start */
	psPBDesc->psListPBBlockHead = psPBDesc->psGrowListPBBlockHead;
	psPBDesc->psListPBBlockTail = psPBDesc->psGrowListPBBlockTail;
	psPBDesc->psGrowListPBBlockTail = IMG_NULL;
	psPBDesc->psGrowListPBBlockHead = IMG_NULL;
#endif
	psHWPBDesc->sListPBBlockDevVAddr = psPBDesc->psListPBBlockHead->sHWPBBlockDevVAddr;

	/*
	 * Allocate memory for EVM page table.
	 * If the PB is shared the EVM PT must exist in the PBHeap in order to be
	 * shared across memory contexts but if the PB is per-context the EVM PT
	 * can move to the General Heap given the DPM has fully address range
	 * (hDevMemHeap will be null in this function if the PB is shared)
	 */
	if(PVRSRVAllocDeviceMem_log(psDevData,
							psPBHeapInfo->hDevMemHeap,
							PVRSRV_MEM_READ|PVRSRV_MEM_WRITE|PVRSRV_MEM_NO_SYNCOBJ,
							EURASIA_PARAM_MANAGE_TABLE_SIZE,
							EURASIA_PARAM_MANAGE_TABLE_GRAN,
							&psClientPBDesc->psEVMPageTableClientMemInfo,
							"EVM Page Table Client Memory Info") != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"CreatePerContextPB: Failed to alloc FB mem for TA/Render HW control structure!"));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorExit;
	}

	PDUMPCOMMENT(psDevData->psConnection, "Initialise PB pagetable", IMG_TRUE);	/* PRQA S 505 */

	/* Initialise the DPM page table for the first PB Block */
	InitialisePBBlockPageTable(psDevData, 
								psClientPBDesc->psEVMPageTableClientMemInfo, 
								psPBDesc->psListPBBlockHead);
	
	/* Success */
#if defined (PDUMP)
	PDUMPCOMMENTF(psDevData->psConnection,
				  IMG_TRUE,
				  "Actual Parameter Buffer Size: %08X", ui32ParamSize);
#endif
	PVR_DPF((PVR_DBG_MESSAGE, "CreatePerContextPB: Actual Parameter Buffer Size: %08X",
			 ui32ParamSize));

	/*
	 * Setup the DPM registers
	 */
	SetupPBDesc(psDevData, psPBDesc);

	SetupHWPBDesc(psDevData, psPBDesc, psClientPBDesc->psEVMPageTableClientMemInfo,
						psClientPBDesc->psHWPBDescClientMemInfo);

	return PVRSRV_OK;

ErrorExit:
	/*
	 * Free up anything that managed to get allocated..
	 */
	DestroyPerContextPB(psDevData, psRenderContext->psClientPBDesc);

	return eError;
}

#if defined(FORCE_ENABLE_GROW_SHRINK)
#if defined(PDUMP)
/*****************************************************************************
 FUNCTION	: PdumpPBGrowShrinkUpdates

 PURPOSE	: Contains the logic required to pdump the updates required to
 				the SGXMKIF_HWPBDESC and SGXMKIF_HWPBBLOCKs on the first
 				command that gets captured.

 PARAMETERS	:

 RETURNS	: PVRSRV_ERROR
*****************************************************************************/
IMG_INTERNAL PVRSRV_ERROR	PdumpPBGrowShrinkUpdates(const PVRSRV_DEV_DATA *psDevData,
											SGX_RENDERCONTEXT	*psRenderContext)
{
	SGX_PBDESC		*psPBDesc;
	SGX_PBBLOCK		*psPBBlock;
	SGX_PBBLOCK		*psNextPBBlock;

	if (psRenderContext == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "PdumpPBGrowShrinkUpdates: Invalid params!"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psPBDesc = psRenderContext->psClientPBDesc->psPBDesc;

	PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "Update HW PB Descriptor\r\n");
	PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "	NumPages: %x\r\n", psPBDesc->ui32PdumpNumPages);
	PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "	Head/Tail: %x\r\n", psPBDesc->ui32PdumpFreeListInitialHT);
	PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "    Prev: %x\r\n", psPBDesc->ui32PdumpFreeListInitialPrev);
	PDUMPMEM(psDevData->psConnection, &psPBDesc->ui32PdumpNumPages,
				psRenderContext->psClientPBDesc->psHWPBDescClientMemInfo,
				offsetof(SGXMKIF_HWPBDESC, ui32NumPages), 
				3*sizeof(IMG_UINT32), 0);
	PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "	TA Threshold: %x\r\n", psPBDesc->ui32PdumpTAThreshold);
	PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "	ZLS Threshold: %x\r\n", psPBDesc->ui32PdumpZLSThreshold);
	PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "	Gbl Threshold: %x\r\n", psPBDesc->ui32PdumpGlobalThreshold);
	PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "	PDS Threshold: %x\r\n", psPBDesc->ui32PdumpPDSThreshold);
	PDUMPMEM(psDevData->psConnection, &psPBDesc->ui32PdumpTAThreshold, 
				psRenderContext->psClientPBDesc->psHWPBDescClientMemInfo,
				offsetof(SGXMKIF_HWPBDESC, ui32TAThreshold), 
				4*sizeof(IMG_UINT32), 0);

	/* Patch the SGXMKIF_HWPBBLOCK next ptrs, so the list is up-to-date */
	PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "Patch HWPBBLOCK next ptrs\r\n");
	psPBBlock = psPBDesc->psListPBBlockHead;
	psNextPBBlock = psPBBlock->psNext;
	while (psNextPBBlock != IMG_NULL)
	{
		PDUMPMEM(psDevData->psConnection, &psNextPBBlock->sHWPBBlockDevVAddr, 
				psPBBlock->psHWPBBlockClientMemInfo,
				offsetof(SGXMKIF_HWPBBLOCK, sNextHWPBBlockDevVAddr), 
				sizeof(IMG_UINT32), 0);
				
		psPBBlock = psNextPBBlock;
		psNextPBBlock = psPBBlock->psNext;
	}
	
	/* If there is a block on the shrink list, point the last block to it,
		as "is" on the list until the first command is processed by the uKernel */
	if (psPBDesc->psShrinkListPBBlock != IMG_NULL)
	{
		psNextPBBlock = psPBDesc->psShrinkListPBBlock;
		
		/* Patch the last block with a NULL ptr */
		PDUMPMEM(psDevData->psConnection, &psNextPBBlock->sHWPBBlockDevVAddr, 
			psPBBlock->psHWPBBlockClientMemInfo,
			offsetof(SGXMKIF_HWPBBLOCK, sNextHWPBBlockDevVAddr), 
			sizeof(IMG_UINT32), 0);
			
		psPBBlock = psNextPBBlock;
		psNextPBBlock = psPBBlock->psNext;
	}
	
	/* Patch the last block with a NULL ptr to terminate the list */
	PDUMPMEM(psDevData->psConnection, &psNextPBBlock, 
			psPBBlock->psHWPBBlockClientMemInfo,
			offsetof(SGXMKIF_HWPBBLOCK, sNextHWPBBlockDevVAddr), 
			sizeof(IMG_UINT32), 0);

	/* Indicate that we no longer need to pdump the updates */
	psPBDesc->bPdumpRequired = IMG_FALSE;
	
	return PVRSRV_OK;
}
	#endif

/*****************************************************************************
 FUNCTION	: GrowPerContextPB

 PURPOSE	: Allocates a new parameter buffer block and adds it to the
 				tail of the PB block grow list

 PARAMETERS	:

 RETURNS	: PVRSRV_ERROR
*****************************************************************************/
static PVRSRV_ERROR GrowPerContextPB(const PVRSRV_DEV_DATA *psDevData,
									SGX_RENDERCONTEXT *psRenderContext,
									SGX_PBDESC		  *psPBDesc)
{
	IMG_UINT32			ui32HeapCount;
	PVRSRV_HEAP_INFO	asHeapInfo[PVRSRV_MAX_CLIENT_HEAPS];
	PVRSRV_HEAP_INFO 	*psPBHeapInfo=0;
	PVRSRV_HEAP_INFO 	*psKernelDataHeapInfo=0;
	IMG_UINT32			i;
	PVRSRV_ERROR		eError;

	if (psRenderContext == IMG_NULL || psPBDesc == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "GrowPerContextPB: Invalid params!"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psPBDesc = psRenderContext->psClientPBDesc->psPBDesc;

	/* A grow was requested therefore reset shrink test */
	psPBDesc->ui32ShrinkRender = 0;
		
	/* Check if we are allowed to grow the PB further */
	if ((psPBDesc->ui32PBSizeLimit != 0) &&
		((psPBDesc->ui32TotalPBSize + psPBDesc->ui32PBGrowBlockSize) > psPBDesc->ui32PBSizeLimit)
		)
	{
		/* We are not allowed to grow any more so just exit */
		PVR_DPF((PVR_DBG_WARNING, "GrowPerContextPB: PB Limit reached"));
		return PVRSRV_OK;
	}

	eError = PVRSRVGetDeviceMemHeapInfo(psDevData,
									psRenderContext->hDevMemContext,
									&ui32HeapCount,
									asHeapInfo);
	if (eError !=  PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "GrowPerContextPB: Failed to retrieve device "
				 					"memory context information\n"));
		return eError;
	}

	for(i=0; i<ui32HeapCount; i++)
	{
		switch(HEAP_IDX(asHeapInfo[i].ui32HeapID))
		{
			case SGX_SHARED_3DPARAMETERS_HEAP_ID:
				if (!psRenderContext->bPerContextPB)
					psPBHeapInfo = &asHeapInfo[i];
				break;
			case SGX_PERCONTEXT_3DPARAMETERS_HEAP_ID:
				if (psRenderContext->bPerContextPB)
					psPBHeapInfo = &asHeapInfo[i];
				break;
			case SGX_KERNEL_DATA_HEAP_ID:
				psKernelDataHeapInfo = &asHeapInfo[i];
				break;
			default:
				break;
		}
	}

	if (CreatePBBlock(psDevData, psPBDesc, psPBDesc->ui32PBGrowBlockSize, IMG_NULL,
						psPBHeapInfo,	psKernelDataHeapInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "GrowPerContextPB: Failed to allocate new PB block"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	InitialisePBBlockPageTable(psDevData, 
								psRenderContext->psClientPBDesc->psEVMPageTableClientMemInfo,
								psPBDesc->psGrowListPBBlockHead);

	/* Calculate all the new values */
	SetupPBDesc(psDevData, psPBDesc);

	psPBDesc->ui32Flags |= SGX_PBDESCFLAGS_GROW_PENDING;

	PVR_DPF((PVR_DBG_MESSAGE, "GrowPerContextPB: PB Grow Pending"));

	return PVRSRV_OK;
}

/*****************************************************************************
 FUNCTION	: ShrinkPerContextPB

 PURPOSE	: Removes parameter buffer blocks

 PARAMETERS	:

 RETURNS	: PVRSRV_ERROR
*****************************************************************************/
static PVRSRV_ERROR ShrinkPerContextPB(const PVRSRV_DEV_DATA *psDevData,
									SGX_PBDESC			*psPBDesc)
{
	if (psPBDesc == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "ShrinkPerContextPB: Invalid params!"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if ((psPBDesc->ui32Flags & SGX_PBDESCFLAGS_SHRINK_QUEUED) != 0)
	{
		/* Can we free the blocks yet? */
		if (psPBDesc->psHWPBDesc->ui32ShrinkComplete != 0)
		{
			DestroyPBBlock(psDevData, psPBDesc, IMG_NULL, psPBDesc->psShrinkListPBBlock);
			psPBDesc->psShrinkListPBBlock = IMG_NULL;
			psPBDesc->psHWPBDesc->ui32ShrinkComplete = 0;
			psPBDesc->ui32ShrinkRender = 0;
			psPBDesc->ui32Flags &= ~SGX_PBDESCFLAGS_SHRINK_QUEUED;
			PVR_DPF((PVR_DBG_MESSAGE, "ShrinkPerContextPB: PB Shrink Complete"));
		}
	}
	else
	{
		IMG_INT32 	i32AllocSize = (IMG_INT32)(psPBDesc->psHWPBDesc->ui32AllocPages * EURASIA_PARAM_MANAGE_GRAN);
		IMG_INT32	i32TestSize = ( ((IMG_INT32)psPBDesc->ui32TotalPBSize) - 
									((IMG_INT32)(SGXPB_SHRINK_TEST_MULTIPLIER*psPBDesc->ui32PBGrowBlockSize)));

		if ((psPBDesc->psListPBBlockTail->psPrev != IMG_NULL) &&
			(i32TestSize > 0) &&
			(i32AllocSize < i32TestSize))
		{
			if (psPBDesc->ui32ShrinkRender == 0)
			{
				psPBDesc->ui32ShrinkRender = psPBDesc->ui32RenderCount + SGXPB_SHRINK_MIN_FRAMES;
				PVR_DPF((PVR_DBG_MESSAGE, "ShrinkPerContextPB: Cnt: %d ASize: %d TestSize: %d",
									psPBDesc->ui32ShrinkRender, i32AllocSize, i32TestSize));
			}
			else
			{
				/* Only Shrink if we have not used the memory for a while */
				if ( (psPBDesc->ui32ShrinkRender <= psPBDesc->ui32RenderCount)
						&& (psPBDesc->ui32BusySceneCount == 0))
				{
					/* Move the block to the shrink list */
					psPBDesc->psShrinkListPBBlock = psPBDesc->psListPBBlockTail;

					psPBDesc->psListPBBlockTail = psPBDesc->psShrinkListPBBlock->psPrev;
					psPBDesc->psListPBBlockTail->psNext = IMG_NULL;

					/* Calculate all the new values */
					SetupPBDesc(psDevData, psPBDesc);

					psPBDesc->ui32Flags |= SGX_PBDESCFLAGS_SHRINK_PENDING;
					PVR_DPF((PVR_DBG_MESSAGE, "ShrinkPerContextPB: PB Shrink Pending"));
				}
			}
		}
		else
		{
			if (psPBDesc->ui32ShrinkRender != 0)
			{
				psPBDesc->ui32ShrinkRender = 0;
			}
		}
	}

	return PVRSRV_OK;
}

/*****************************************************************************
 FUNCTION	: HandlePBGrowShrink

 PURPOSE	: Handles the logic related to whether a PB should grow or shrink

 PARAMETERS	:

 RETURNS	: PVRSRV_ERROR
*****************************************************************************/
IMG_INTERNAL PVRSRV_ERROR HandlePBGrowShrink(const PVRSRV_DEV_DATA *psDevData,
									SGX_RENDERCONTEXT	*psRenderContext,
									IMG_BOOL			bOutOfMemory)
{
	SGX_PBDESC		*psPBDesc;
	PVRSRV_ERROR	eError = PVRSRV_OK;

	if (psRenderContext == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "HandlePBGrowShrink: Invalid params!"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}	

	psPBDesc = psRenderContext->psClientPBDesc->psPBDesc;

	if (!psPBDesc->bGrowShrink)
		return PVRSRV_OK;

	if ( ((psPBDesc->ui32Flags & SGX_PBDESCFLAGS_SHRINK_QUEUED) != 0)
		|| (((psPBDesc->ui32Flags & SGX_PBDESCFLAGS_GROW_PENDING) == 0) && (bOutOfMemory == IMG_FALSE))
	   )
	{
		eError = ShrinkPerContextPB(psDevData, psPBDesc);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "HandlePBGrowShrink: PB Shrink call failed"));
		}
	}
	else if (bOutOfMemory && ((psPBDesc->ui32Flags & SGX_PBDESCFLAGS_GROW_PENDING) == 0))
	{
		eError = GrowPerContextPB(psDevData, psRenderContext, psPBDesc);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "HandlePBGrowShrink: PB Grow call failed"));
		}
	}

	return eError;
}

/*****************************************************************************
 FUNCTION	: PBGrowShrinkComplete

 PURPOSE	: Handles the logic related to whether a PB should grow or shrink

 PARAMETERS	:

 RETURNS	: PVRSRV_ERROR
*****************************************************************************/
IMG_INTERNAL PVRSRV_ERROR PBGrowShrinkComplete(const PVRSRV_DEV_DATA *psDevData,
											SGX_RENDERCONTEXT	*psRenderContext)
{
	SGX_PBDESC		*psPBDesc;

	PVR_UNREFERENCED_PARAMETER(psDevData);

	if (psRenderContext == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "PBGrowShrinkComplete: Invalid params!"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psPBDesc = psRenderContext->psClientPBDesc->psPBDesc;

	if (!psPBDesc->bGrowShrink)
		return PVRSRV_OK;

	#if defined(PDUMP)
	if (psPBDesc->bPdumpRequired == IMG_TRUE)
	{
		/* Copy across the new values ready for when we pdump the first command */
		psPBDesc->ui32PdumpNumPages 			= psPBDesc->ui32NumPages;
		psPBDesc->ui32PdumpFreeListInitialHT 	= psPBDesc->ui32FreeListInitialHT;
		psPBDesc->ui32PdumpFreeListInitialPrev 	= psPBDesc->ui32FreeListInitialPrev;
		psPBDesc->ui32PdumpTAThreshold 			= psPBDesc->ui32TAThreshold;
		psPBDesc->ui32PdumpZLSThreshold			= psPBDesc->ui32ZLSThreshold;
		psPBDesc->ui32PdumpGlobalThreshold		= psPBDesc->ui32GlobalThreshold;
		psPBDesc->ui32PdumpPDSThreshold			= psPBDesc->ui32PDSThreshold;
	}
	#endif

	if ((psPBDesc->ui32Flags & SGX_PBDESCFLAGS_GROW_PENDING) != 0)
	{
		PVR_DPF((PVR_DBG_MESSAGE, "PBGrowShrinkComplete: Grow PB command submitted"));
		PVR_DPF((PVR_DBG_WARNING, "PBGrowShrinkComplete: New PB size is: %d bytes", psPBDesc->ui32TotalPBSize));

		/* Move the grow list on the host */
		psPBDesc->psListPBBlockTail->psNext = psPBDesc->psGrowListPBBlockHead;
		psPBDesc->psGrowListPBBlockHead->psPrev = psPBDesc->psListPBBlockTail;
		psPBDesc->psListPBBlockTail = psPBDesc->psGrowListPBBlockTail;

		/* The pages will be added to the PB, so clear the grow ptrs (ready for next grow) */
		psPBDesc->psGrowListPBBlockHead = IMG_NULL;
		psPBDesc->psGrowListPBBlockTail = IMG_NULL;

		/* Clear the Flag as the grow has been queued */
		psPBDesc->ui32Flags	&= ~(SGX_PBDESCFLAGS_GROW_PENDING |
									SGX_PBDESCFLAGS_THRESHOLD_UPDATE_PENDING);
	}
	else if ((psPBDesc->ui32Flags & SGX_PBDESCFLAGS_SHRINK_PENDING) != 0)
	{
		PVR_DPF((PVR_DBG_MESSAGE, "PBGrowShrinkComplete: Shrink PB command submitted"));
		PVR_DPF((PVR_DBG_WARNING, "PBGrowShrinkComplete: New PB size is: %d bytes", psPBDesc->ui32TotalPBSize));

		/* Clear the Flag as the shrink has been queued */
		psPBDesc->ui32Flags	&= ~(SGX_PBDESCFLAGS_SHRINK_PENDING |
									SGX_PBDESCFLAGS_THRESHOLD_UPDATE_PENDING);
		/* Set the queued flag so we know to wait for the complete */
		psPBDesc->ui32Flags	|= SGX_PBDESCFLAGS_SHRINK_QUEUED;
	}

	return PVRSRV_OK;
}
#endif /* defined(FORCE_ENABLE_GROW_SHRINK) */
#endif /* defined(SUPPORT_PERCONTEXT_PB) */
#if defined(SUPPORT_SHARED_PB)

/*****************************************************************************
 FUNCTION	: CreateSharedPB

 PURPOSE	: Creates a shared parameter buffer to be used by all rendering
 				contexts and all the memory	associated with it

 PARAMETERS	:

 RETURNS	: PVRSRV_ERROR
*****************************************************************************/
IMG_INTERNAL
PVRSRV_ERROR CreateSharedPB(const PVRSRV_DEV_DATA *psDevData,
#if defined (SUPPORT_SID_INTERFACE)
									IMG_SID   *phSharedPBDesc,
#else
									IMG_HANDLE *phSharedPBDesc,
#endif
									IMG_UINT32 ui32RequestedParamSize,
#if defined (SUPPORT_SID_INTERFACE)
									IMG_SID		hDevMemContext,
#else
									IMG_HANDLE	hDevMemContext,
#endif
									const PVRSRV_HEAP_INFO *psPBHeapInfo,
									const PVRSRV_HEAP_INFO *psKernelVideoDataHeapInfo)
{
	PVRSRV_CLIENT_MEM_INFO *psTmpSharedPBDescClientMemInfo = IMG_NULL;
	SGX_PBDESC		   *psPBDesc;
	PVRSRV_CLIENT_MEM_INFO *psTmpPBClientMemInfo = IMG_NULL;
	PVRSRV_CLIENT_MEM_INFO *psTmpHWPBDescClientMemInfo = IMG_NULL;
	PVRSRV_CLIENT_MEM_INFO *psTmpEVMPageTableClientMemInfo = IMG_NULL;
	SGXMKIF_HWPBDESC		*psHWPBDesc;
	PVRSRV_ERROR			eError = PVRSRV_OK;
#if defined (SUPPORT_SID_INTERFACE)
	IMG_SID ahSharedPBDescSubKernelMemInfo[PBDESC_SUB_MEMINFO_LAST_KEY] = {0};

	IMG_SID hSharedPBDescKernelMemInfoHandle;
	IMG_SID hHWPBDescKernelMemInfoHandle;
	
	IMG_SID hPBBlockClientMemInfoHandle = 0;
	IMG_SID hHWPBBlockClientMemInfoHandle = 0;
#else
	IMG_HANDLE ahSharedPBDescSubKernelMemInfo[PBDESC_SUB_MEMINFO_LAST_KEY] = {0};

	IMG_HANDLE hSharedPBDescKernelMemInfoHandle;
	IMG_HANDLE hHWPBDescKernelMemInfoHandle;

	IMG_HANDLE hPBBlockClientMemInfoHandle = 0;
	IMG_HANDLE hHWPBBlockClientMemInfoHandle = 0;
#endif

	PVRSRV_CLIENT_MEM_INFO *psTmpPBBlockClientMemInfo = IMG_NULL;
	IMG_UINT32 	ui32ParamSize;
	IMG_DEV_VIRTADDR sHWPBDescDevVAddr;

	PVR_UNREFERENCED_PARAMETER(hDevMemContext);

	/*
	 * Alloc memory for host structures if necessary
	 */
	if(PVRSRVAllocSharedSysMem(psDevData->psConnection,
							   0,
							   sizeof(SGX_PBDESC),
							   &psTmpSharedPBDescClientMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "CreateSharedPB: Failed to allocate shared PBDesc\n"));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorExit;
	}

	psPBDesc = psTmpSharedPBDescClientMemInfo->pvLinAddr;
	psPBDesc->bPerContextPB = IMG_FALSE;
	psPBDesc->bGrowShrink = IMG_FALSE;

	psPBDesc->ui32Flags = 0;
	psPBDesc->sParamHeapBaseDevVAddr = psPBHeapInfo->sDevVAddrBase;

	psPBDesc->psListPBBlockHead = IMG_NULL;
	psPBDesc->psListPBBlockTail = IMG_NULL;
#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
	psPBDesc->psGrowListPBBlockHead = IMG_NULL;
	psPBDesc->psGrowListPBBlockTail = IMG_NULL;
	psPBDesc->psShrinkListPBBlock = IMG_NULL;
#endif

	/*
	 * Allocate memory for global TA data
	 * NB: This is alligned to the TA control stream granularity - the first
	 * IMG_UINT32 in the struct is used when the primary core code decides it
	 * needs to terminate the TA.
	 */
	if(PVRSRVAllocDeviceMem(psDevData,
							psKernelVideoDataHeapInfo->hDevMemHeap,
							PVRSRV_MEM_READ|PVRSRV_MEM_WRITE|PVRSRV_MEM_NO_SYNCOBJ | PVRSRV_MEM_NO_RESMAN | PVRSRV_MEM_CACHE_CONSISTENT,
							sizeof(SGXMKIF_HWPBDESC),
							32,
							&psTmpHWPBDescClientMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"ERROR - Failed to alloc FB mem for global TA struct!"));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorExit;
	}

	psHWPBDesc =  psTmpHWPBDescClientMemInfo->pvLinAddr;
	psPBDesc->psHWPBDesc = psHWPBDesc;
	psPBDesc->sHWPBDescDevVAddr = psTmpHWPBDescClientMemInfo->sDevVAddr;
	sHWPBDescDevVAddr = psTmpHWPBDescClientMemInfo->sDevVAddr;
	
	PVRSRVMemSet(psHWPBDesc, 0, sizeof(SGXMKIF_HWPBDESC));

#if defined (PDUMP)
	PDUMPCOMMENTF(psDevData->psConnection,
				  IMG_TRUE,
				  "Requested Parameter Buffer Size: %08X", ui32RequestedParamSize);
#endif
	PVR_DPF((PVR_DBG_MESSAGE,
			 "CreateSharedPB: Requested Parameter Buffer Size: %08X",
			 ui32RequestedParamSize));

	ui32ParamSize = ui32RequestedParamSize;
#if defined(FIX_HW_BRN_23055)
	/* Add on the pages held in reserve for the 23055 workaround */
	ui32ParamSize += (BRN23055_BUFFER_PAGE_SIZE * EURASIA_PARAM_MANAGE_GRAN);
#endif /* defined(FIX_HW_BRN_23055) */

	/* SPM_PAGE_INJECTION: add on the pages held in reserve for the SPM deadlock condition */
	ui32ParamSize += (SPM_DL_RESERVE_SIZE + EURASIA_PARAM_FREE_LIST_RESERVE_SIZE);

	if (CreatePBBlock(psDevData, psPBDesc,
						ui32ParamSize, &psTmpPBBlockClientMemInfo,
						psPBHeapInfo, psKernelVideoDataHeapInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "CreatePerContextPB: Failed to allocate PB Block"));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorExit;
	}
	psHWPBDesc->sListPBBlockDevVAddr = psPBDesc->psListPBBlockHead->sHWPBBlockDevVAddr;
	psTmpPBClientMemInfo = ((SGX_PBBLOCK *)(psTmpPBBlockClientMemInfo->pvLinAddr))->psPBClientMemInfo;

	/*
	 * Allocate memory for EVM page table.
	 * If the PB is shared the EVM PT must exist in the PBHeap in order to be
	 * shared across memory contexts but if the PB is per-context the EVM PT
	 * can move to the General Heap given the DPM has fully address range
	 * (hDevMemHeap will be null in this function if the PB is shared)
	 */
	if(PVRSRVAllocDeviceMem(psDevData,
							psPBHeapInfo->hDevMemHeap,
							PVRSRV_MEM_READ|PVRSRV_MEM_WRITE|PVRSRV_MEM_NO_SYNCOBJ | PVRSRV_MEM_NO_RESMAN,
							EURASIA_PARAM_MANAGE_TABLE_SIZE,
							EURASIA_PARAM_MANAGE_TABLE_GRAN,
							&psTmpEVMPageTableClientMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"ERROR - Failed to alloc FB mem for TA/Render HW control structure!"));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorExit;
	}

	PDUMPCOMMENT(psDevData->psConnection, "Initialise PB pagetable", IMG_TRUE);	/* PRQA S 505 */

	InitialisePBBlockPageTable(psDevData, 
								psTmpEVMPageTableClientMemInfo,
								psPBDesc->psListPBBlockHead);
	
	/* Success */
#if defined (PDUMP)
	PDUMPCOMMENTF(psDevData->psConnection,
				  IMG_TRUE,
				  "Actual Parameter Buffer Size: %08X", ui32ParamSize);
#endif
	PVR_DPF((PVR_DBG_MESSAGE, "CreateSharedPB: Actual Parameter Buffer Size: %08X",
			 ui32ParamSize));

	/*
	 * Setup the DPM registers
	 */
	SetupPBDesc(psDevData, psPBDesc);

	SetupHWPBDesc(psDevData, psPBDesc,
					psTmpEVMPageTableClientMemInfo,	psTmpHWPBDescClientMemInfo);


	ahSharedPBDescSubKernelMemInfo[PBDESC_SUB_MEMINFO_PB] =
		psTmpPBClientMemInfo->hKernelMemInfo;
	ahSharedPBDescSubKernelMemInfo[PBDESC_SUB_MEMINFO_EVM_PAGE_TABLE] =
		psTmpEVMPageTableClientMemInfo->hKernelMemInfo;

	/* TODO: explain why we are un-reffing this memory now! */
	hSharedPBDescKernelMemInfoHandle =
			psTmpSharedPBDescClientMemInfo->hKernelMemInfo;
	hHWPBDescKernelMemInfoHandle =
			psTmpHWPBDescClientMemInfo->hKernelMemInfo;
	hHWPBBlockClientMemInfoHandle =
			psPBDesc->psListPBBlockHead->psHWPBBlockClientMemInfo->hKernelMemInfo;
	hPBBlockClientMemInfoHandle  =
			psTmpPBBlockClientMemInfo->hKernelMemInfo;

	if(PVRSRVUnrefDeviceMem(psDevData, psTmpPBClientMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "CreatePB: PVRSRVUnrefDeviceMem Failed\n"));
		goto ErrorExit;
	}

	psTmpPBClientMemInfo=IMG_NULL;

	if(PVRSRVUnrefDeviceMem(psDevData, psPBDesc->psListPBBlockHead->psHWPBBlockClientMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "CreatePB: PVRSRVUnrefDeviceMem Failed\n"));
		goto ErrorExit;
	}

	if(PVRSRVUnrefSharedSysMem(psDevData->psConnection, psTmpPBBlockClientMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "CreatePB: PVRSRVUnrefSharedSysMem Failed\n"));
		goto ErrorExit;
	}
	psTmpPBBlockClientMemInfo = IMG_NULL;

	if(PVRSRVUnrefDeviceMem(psDevData, psTmpEVMPageTableClientMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "CreatePB: PVRSRVUnrefDeviceMem Failed\n"));
		goto ErrorExit;
	}
	psTmpEVMPageTableClientMemInfo=IMG_NULL;

	if(PVRSRVUnrefDeviceMem(psDevData, psTmpHWPBDescClientMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "CreatePB: PVRSRVUnrefDeviceMem Failed\n"));
		goto ErrorExit;
	}
	psTmpHWPBDescClientMemInfo=IMG_NULL;
	if(PVRSRVUnrefSharedSysMem(psDevData->psConnection, psTmpSharedPBDescClientMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "CreatePB: PVRSRVUnrefSharedSysMem Failed\n"));
		goto ErrorExit;
	}
	psTmpSharedPBDescClientMemInfo=IMG_NULL;

	/* After SGXAddSharedPBDesc returns, kernel services owns all the
	 * associated memory. */
	if(SGXAddSharedPBDesc(psDevData,
						  ui32RequestedParamSize,
						  hSharedPBDescKernelMemInfoHandle,
						  hHWPBDescKernelMemInfoHandle,
						  hPBBlockClientMemInfoHandle,
						  hHWPBBlockClientMemInfoHandle,
						  phSharedPBDesc,
						  ahSharedPBDescSubKernelMemInfo,
						  PBDESC_SUB_MEMINFO_LAST_KEY,
						  sHWPBDescDevVAddr) != PVRSRV_OK)
	{
		/* FIXME - Note: Currently the memory associated with all the meminfos
		 * that we Unref'd may be in limbo, with only the resource manager having
		 * a handle. We need a way to actively ensure the memory is freed. */
#if 0
		/* E.g. we need a light version of PVRSRVFreeDeviceMem that simply
		 * does a PVRSRV_BRIDGE_FREE_DEVICEMEM bridge call but doesn't
		 * need to call PVRMMAPRemoveMapping. Vaguely like... */
		<PVRSRVFreeUnrefedDeviceMem>
			sIn.hDevCookie = psDevData->hDevCookie;
		sIn.psKernelMemInfo  = hKernelMemInfo;
		if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
							PVRSRV_BRIDGE_FREE_DEVICEMEM,
							&sIn,
							sizeof(PVRSRV_BRIDGE_IN_FREEDEVICEMEM),
							(IMG_VOID *)&sRet,
							sizeof(PVRSRV_BRIDGE_RETURN)))
		{
			return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
		}
		</PVRSRVFreeUnrefedDeviceMem>

			for(i=0; i<PBDESC_SUB_MEMINFO_LAST_KEY; i++)
				PVRSRVLightFreeDeviceMem(psDevData, ahSharedPBDescSubKernelMemInfo[i]);

		/* Same idea, but for shared memory... */
		PVRSRVFreeUnrefedSharedMem(psDevData, hSharedPBDescKernelMemInfo);

		/* Note: These functions wouldn't return an "error" if they reliably
		 * determine that the memory has _already_ been freed */

		/* Is PVRSRVFreeUnrefedDeviceMem() a better name? */
#endif

		/* Alternatively SGXAddSharedPBDesc could return a list of valid hKernMemInfos
		 * and _after_ the bridge call we would call PVRSRVUnrefDeviceMem() for each
		 * meminfo that is no longer valid, and also PVRSRVUnrefDeviceMem for the
		 * meminfos we no longer require.
		 * TODO: Not sure if we can safely (accross all OSs) implement
		 * PVRSRVUnrefDeviceMem so it handles the possability that the backing memory
		 * has been removed. E.g. on Linux can we reliably do our unmap even though
		 * the backing memory is no longer owned by the process.
		 *  - Note also unless we were carefull to specifically destroy a mapping
		 *    from kernelspace you end up for a period with mappings that won't
		 *    page-fault, but it will point to memory that could be re-used for
		 *    somthing new before the process gets a chance to call unmap(). (Very
		 *    dodgy!)
		 */
		PVR_DPF((PVR_DBG_ERROR, "CreateSharedPB: Failed to register psPBDesc\n"));
		goto ErrorExit;
	}

	return PVRSRV_OK;

ErrorExit:
	/*
	 * All gone to pot, free up anything that managed to get allocated..
	 */

	if(psTmpHWPBDescClientMemInfo)
		PVRSRVFreeDeviceMem(psDevData, psTmpHWPBDescClientMemInfo);

	if(psTmpEVMPageTableClientMemInfo)
		PVRSRVFreeDeviceMem(psDevData, psTmpEVMPageTableClientMemInfo);

	if(psTmpPBClientMemInfo)
		PVRSRVFreeDeviceMem(psDevData, psTmpPBClientMemInfo);

	if(psTmpSharedPBDescClientMemInfo)
		PVRSRVFreeSharedSysMem(psDevData->psConnection, psTmpSharedPBDescClientMemInfo);

	if(psTmpPBBlockClientMemInfo)
	{
			/* FIXME: for now we'll just use the first PBBlock always (until grow/shrink) */
			PVRSRVFreeSharedSysMem(psDevData->psConnection, psTmpPBBlockClientMemInfo);
	}


	return eError;
}

/*****************************************************************************
 FUNCTION	: UnrefPB

 PURPOSE	: Dereferences a shared parameter buffer, once all references
 				have been removed the PB will be freed

 PARAMETERS	:

 RETURNS	: None
*****************************************************************************/
IMG_INTERNAL
IMG_VOID UnrefPB(const PVRSRV_DEV_DATA *psDevData, SGX_CLIENTPBDESC *psClientPBDesc)
{
	PVRSRV_CLIENT_MEM_INFO *psBlockClientMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psHWBlockClientMemInfo;
#if defined (SUPPORT_SID_INTERFACE)
	IMG_SID hSharedPBDesc;
#else
	IMG_HANDLE hSharedPBDesc;
#endif

	PVR_ASSERT(psClientPBDesc);

	PVRSRVUnrefDeviceMem(psDevData, psClientPBDesc->psHWPBDescClientMemInfo);
	PVRSRVUnrefDeviceMem(psDevData, psClientPBDesc->psEVMPageTableClientMemInfo);

	/* keep submembers before unref'ing PBDesc */
	psBlockClientMemInfo = psClientPBDesc->psBlockClientMemInfo;
	psHWBlockClientMemInfo = psClientPBDesc->psHWBlockClientMemInfo;

	hSharedPBDesc = psClientPBDesc->hSharedPBDesc;

	PVRSRVUnrefSharedSysMem(psDevData->psConnection,
							psClientPBDesc->psPBDescClientMemInfo);

	if(psHWBlockClientMemInfo)
	{
		PVRSRVUnrefDeviceMem(psDevData, psHWBlockClientMemInfo);
	}

	if(psBlockClientMemInfo)
	{
		PVRSRVUnrefSharedSysMem(psDevData->psConnection,
							psBlockClientMemInfo);
	}

	/* Finally we can do the kernel side unref */
	SGXUnrefSharedPBDesc(psDevData, hSharedPBDesc);
}

/*****************************************************************************
 FUNCTION	: MapSharedPBDescToClient

 PURPOSE	: Maps the structures created by a shared parameter buffer back
 				to a rendering context

 PARAMETERS	:

 RETURNS	: PVRSRV_ERROR
*****************************************************************************/
IMG_INTERNAL PVRSRV_ERROR
MapSharedPBDescToClient(const PVRSRV_DEV_DATA *psDevData,
#if defined (SUPPORT_SID_INTERFACE)
						IMG_SID    hSharedPBDesc,
						IMG_SID    hSharedPBDescKernelMemInfoHandle,
						IMG_SID    hHWPBDescKernelMemInfoHandle,
						IMG_SID    hBlockKernelMemInfoHandle,
						IMG_SID    hHWBlockKernelMemInfoHandle,
						const IMG_SID *phSubKernelMemInfoHandles,
#else
						IMG_HANDLE hSharedPBDesc,
						IMG_HANDLE hSharedPBDescKernelMemInfoHandle,
						IMG_HANDLE hHWPBDescKernelMemInfoHandle,
						IMG_HANDLE hBlockKernelMemInfoHandle,
						IMG_HANDLE hHWBlockKernelMemInfoHandle,
						const IMG_HANDLE *phSubKernelMemInfoHandles,
#endif
						IMG_UINT32 ui32SubKernelMemInfoHandlesCount,
						SGX_CLIENTPBDESC *psClientPBDesc)
{
	PVRSRV_CLIENT_MEM_INFO *psPBDescClientMemInfo;
	PVRSRV_ERROR eError;

#if defined (DEBUG)
	PVR_ASSERT(ui32SubKernelMemInfoHandlesCount == PBDESC_SUB_MEMINFO_LAST_KEY);
#else
	PVR_UNREFERENCED_PARAMETER(ui32SubKernelMemInfoHandlesCount);
#endif

	eError = PVRSRVMapMemInfoMem(psDevData->psConnection,
						  hSharedPBDescKernelMemInfoHandle,
						  &psPBDescClientMemInfo);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "MapSharedPBDescToClient:"
				" Failed to map psPBKernelMemInfo\n"));
		return eError;
	}
	psClientPBDesc->psPBDesc = (SGX_PBDESC*)psPBDescClientMemInfo->pvLinAddr;
	psClientPBDesc->psPBDescClientMemInfo = psPBDescClientMemInfo;

	eError = PVRSRVMapMemInfoMem(psDevData->psConnection,
						  hHWPBDescKernelMemInfoHandle,
						  &psClientPBDesc->psHWPBDescClientMemInfo);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "MapSharedPBDescToClient:"
				" Failed to map psHWPBDescClientMemInfo\n"));
		return eError;
	}

	eError = PVRSRVMapMemInfoMem(psDevData->psConnection,
						  hBlockKernelMemInfoHandle,
						  &psClientPBDesc->psBlockClientMemInfo);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "MapSharedPBDescToClient:"
				" Failed to map psPBKernelMemInfo\n"));
		return eError;
	}

	eError = PVRSRVMapMemInfoMem(psDevData->psConnection,
						  hHWBlockKernelMemInfoHandle,
						  &psClientPBDesc->psHWBlockClientMemInfo);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "MapSharedPBDescToClient:"
				" Failed to map psPBKernelMemInfo\n"));
		return eError;
	}

	eError = PVRSRVMapMemInfoMem(psDevData->psConnection,
						  phSubKernelMemInfoHandles[PBDESC_SUB_MEMINFO_EVM_PAGE_TABLE],
						  &psClientPBDesc->psEVMPageTableClientMemInfo);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "MapSharedPBDescToClient: Failed to map psEVMPageTableClientMemInfo\n"));
		goto ErrorExit;
	}

#if defined(SUPPORT_PERCONTEXT_PB)
	PVR_UNREFERENCED_PARAMETER(hSharedPBDesc);
#endif
#if defined(SUPPORT_SHARED_PB)
	psClientPBDesc->hSharedPBDesc = hSharedPBDesc;
#endif

	return PVRSRV_OK;

ErrorExit:
	return eError;
}
#endif /* defined(SUPPORT_SHARED_PB) */

/******************************************************************************
  End of file (sgxpb.c)
******************************************************************************/
