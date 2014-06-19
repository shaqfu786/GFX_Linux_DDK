/*!****************************************************************************
@File           render_targets.c

@Title          Render Target routines

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

@Description    Render Target functions

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-
$Log: sgxrender_targets.c $
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
#include "sgxrender_context.h"

#define FLOAT32_ZERO	0x00000000
#define FLOAT32_ONE		0x3f800000
#define FLOAT32_TWO		0x40000000

/* SGX MT4 and MT16 mode threshold */
#define SGX_MT_DEFAULT_THRESHOLD		(640 * 480)

/*
 * Default number of overlappable renders per frame.
 * A value of 1 results in a double-buffered RTDATA and triple-buffered RENDERDETAILS.
 */
#define DEFAULT_RENDERS_PER_FRAME	1

/*****************************************************************************
  Internal function prototypes
 *****************************************************************************/
static IMG_BOOL CreateRenderDetails(const PVRSRV_DEV_DATA *psDevData,
									SGX_RENDERDETAILS	**ppsRenderDetails,
#if defined (SUPPORT_SID_INTERFACE)
									IMG_SID hDevMemHeap,
									IMG_SID hSyncDataMemHeap,
#else
									IMG_HANDLE hDevMemHeap,
									IMG_HANDLE hSyncDataMemHeap,
#endif
									IMG_UINT16 ui16NumRTsInArray);
static IMG_VOID DestroyRenderDetails(const PVRSRV_DEV_DATA *psDevData,
									 SGX_RTDATASET *psRTDataSet,
									 SGX_RENDERDETAILS *psRenderDetails);
static IMG_BOOL CreateDeviceSyncList(const PVRSRV_DEV_DATA *psDevData,
									SGX_DEVICE_SYNC_LIST	**ppsDeviceSyncList,
#if defined (SUPPORT_SID_INTERFACE)
									IMG_SID hDevMemHeap,
									IMG_SID hSyncDataMemHeap,
#else
									IMG_HANDLE hDevMemHeap,
									IMG_HANDLE hSyncDataMemHeap,
#endif
									IMG_UINT16 ui16NumRTsInArray);
static IMG_VOID DestroyDeviceSyncList(const PVRSRV_DEV_DATA *psDevData,
									 SGX_RTDATASET *psRTDataSet,
									 SGX_DEVICE_SYNC_LIST *psDeviceSyncList);
static PVRSRV_ERROR SetupRTDataSet(const PVRSRV_DEV_DATA *psDevData,
								   SGX_RENDERCONTEXT *psRenderContext,
								   SGX_RTDATASET *psRTDataSet);


/*!
 * *****************************************************************************
 * @brief Takes a number and rounds it up to the nearest power of 2
 *
 * @param ui32Num
 *
 * @return
 ********************************************************************************/
static IMG_UINT32
FindPowerOfTwo(IMG_UINT32 ui32Num)
{
	IMG_UINT32	ui32Test;
	IMG_UINT32	i;

	ui32Test = 0x1000;

	for (i = 12; i > 0; i--)
	{
		if (ui32Num & ui32Test)
		{
			break;
		}

		ui32Test >>= 1;
	}

	if (ui32Num & (ui32Test - 1))
	{
		ui32Test <<= 1;
	}

	return ui32Test;
}

#if defined(FIX_HW_BRN_23862) || defined(FIX_HW_BRN_30089) || defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH) || \
	defined(FIX_HW_BRN_29504) || defined(FIX_HW_BRN_33753) || \
	(!defined(SGX_FEATURE_ISP_BREAKPOINT_RESUME_REV_2) && !defined(FIX_HW_BRN_23761) && !defined(SGX_FEATURE_VISTEST_IN_MEMORY))
/*!
 * *****************************************************************************
 * @brief Calculate the offset of the region header for a tile with given
 *        coordinates.
 *
 * @param ui32TileX
 * @param ui32TileY
 * @param ui32TilesPerMTileX
 * @param ui32TilesPerMTileY
 * @param ui32MacroTilesX
 *
 * @return
 ********************************************************************************/
static IMG_UINT32
TilePositionToOffset(IMG_UINT32	ui32TileX,
					 IMG_UINT32	ui32TileY,
					 IMG_UINT32	ui32TilesPerMTileX,
					 IMG_UINT32	ui32TilesPerMTileY,
					 IMG_UINT32	ui32MacroTilesX)
{
	IMG_UINT32	ui32MTileX;
	IMG_UINT32	ui32MTileY;
	IMG_UINT32	ui32LocalTileX;
	IMG_UINT32	ui32LocalTileY;
	IMG_UINT32	ui32TileWithinMacroBlock;
	IMG_UINT32	ui32TileNumber;

	ui32MTileX = ui32TileX / ui32TilesPerMTileX;
	ui32LocalTileX = ui32TileX % ui32TilesPerMTileX;

	ui32MTileY = ui32TileY / ui32TilesPerMTileY;
	ui32LocalTileY = ui32TileY % ui32TilesPerMTileY;

#if defined(SGX520)
	ui32TileWithinMacroBlock = ui32LocalTileY * ui32TilesPerMTileX + ui32LocalTileX;
#else
	ui32TileWithinMacroBlock = ((ui32LocalTileX & 0x3))+
		((ui32LocalTileY & 0x3UL)<<2)+
		((ui32LocalTileX & (~3UL))<<2)+
		((ui32LocalTileY & (~3UL))*ui32TilesPerMTileX);
#endif

	ui32TileNumber	= ui32MTileY*ui32TilesPerMTileX*ui32TilesPerMTileY+
		ui32MTileX*ui32TilesPerMTileX*ui32TilesPerMTileY*ui32MacroTilesX+
		ui32TileWithinMacroBlock;

	return ui32TileNumber;
}
#endif /* defined(FIX_HW_BRN_23862) */

#if !defined(SGX_FEATURE_ISP_BREAKPOINT_RESUME_REV_2) && !defined(FIX_HW_BRN_23761) && !defined(SGX_FEATURE_VISTEST_IN_MEMORY)
/*!
 * *****************************************************************************
 * @brief Create a look up table of the start of each tile's region headers
 *
 * @param psRTData
 * @param ui32TilesPerMTileX
 * @param ui32TilesPerMTileY
 * @param ui32TilesPerMTileY
 * @param ui32MacroTilesX
 * @param ui32MacroTilesY
 * @param ui32MacroTilesY
 *
 * @return
 ********************************************************************************/
static IMG_VOID
FillTileRgnLUT(PSGX_RTDATA psRTData,
			   IMG_UINT32 ui32TilesPerMTileX,
			   IMG_UINT32 ui32TilesPerMTileY,
			   IMG_UINT32 ui32MacroTilesX,
			   IMG_UINT32 ui32MacroTilesY,
			   IMG_UINT32 ui32MacroTileRgnSize)
{
	IMG_UINT32*	pui32TileRgns;
	IMG_UINT32	ui32RgnBase;
	IMG_UINT32	ui32TileX;
	IMG_UINT32	ui32TileY;

	PVR_UNREFERENCED_PARAMETER(ui32MacroTileRgnSize);

	pui32TileRgns = (IMG_UINT32*)psRTData->psTileRgnLUTClientMemInfo->pvLinAddr;

	for (ui32TileY = 0;
		 ui32TileY < (ui32TilesPerMTileY * ui32MacroTilesY);
		 ui32TileY ++)
	{
		for (ui32TileX = 0;
			 ui32TileX < (ui32TilesPerMTileX * ui32MacroTilesX);
			 ui32TileX ++)
		{
			/* Start with region base */
			//FIXME: MP support will require multiple LUTs
			ui32RgnBase = psRTData->apsRgnHeaderClientMemInfo[0]->sDevVAddr.uiAddr;

			/* Add the offset for the this tile (x, y) */
			ui32RgnBase +=
				EURASIA_REGIONHEADER_SIZE * TilePositionToOffset(ui32TileX,
																 ui32TileY,
																 ui32TilesPerMTileX,
																 ui32TilesPerMTileY,
																 ui32MacroTilesX);
			/* Save region header */
			*pui32TileRgns++ = ui32RgnBase;
		}
	}
}
#endif /* !defined(FIX_HW_BRN_23761) */

#if defined(FIX_HW_BRN_23862) || defined(FIX_HW_BRN_30089) || defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH) || \
	defined(FIX_HW_BRN_29504) || defined(FIX_HW_BRN_33753)
/*!
 * *****************************************************************************
 * @brief Create a look up table of the end of each macro tile's region headers
 *
 * @param psRTDataSet
 * @param psRTData
 * @param psHWRTData
 * @param ui32MacroTilesX
 * @param ui32MacroTilesY
 * @param ui32TilesPerMTileX
 * @param ui32TilesPerMTileY
 *
 * @return
 ********************************************************************************/
static IMG_VOID
FillLastRgnLUT(SGX_RTDATASET *psRTDataSet,
			   SGX_RTDATA *psRTData,
			   SGXMKIF_HWRTDATA *psHWRTData,
			   IMG_UINT32 ui32MacroTilesX,
			   IMG_UINT32 ui32MacroTilesY,
			   IMG_UINT32 ui32TilesPerMTileX,
			   IMG_UINT32 ui32TilesPerMTileY)
{
	IMG_UINT32	*pui32MacroTileLastRgn;
	IMG_UINT32	ui32Core;
	IMG_UINT32	ui32X;
	IMG_UINT32	ui32Y;
	IMG_UINT32	ui32RgnOffs = 0;
	IMG_UINT32	ui32ScreenXMax = (((psRTDataSet->ui32ScreenXMax + 1) * psRTDataSet->ui16MSAASamplesInX) - 1);
	IMG_UINT32	ui32ScreenYMax = (((psRTDataSet->ui32ScreenYMax + 1) * psRTDataSet->ui16MSAASamplesInY) - 1);

	#if ! defined(FIX_HW_BRN_23862)
	PVR_UNREFERENCED_PARAMETER(psHWRTData);
	#endif

	pui32MacroTileLastRgn = (IMG_UINT32*)psRTData->psLastRgnLUTClientMemInfo->pvLinAddr;
 
	for (ui32X = 0; ui32X < ui32MacroTilesX; ui32X++)
	{
		for (ui32Y = 0; ui32Y < ui32MacroTilesY; ui32Y++)
		{
			IMG_UINT32 ui32LastTileX, ui32LastTileY;

			ui32LastTileX = ui32X * ui32TilesPerMTileX + ui32TilesPerMTileX - 1;
			ui32LastTileY = ui32Y * ui32TilesPerMTileY + ui32TilesPerMTileY - 1;

			if (ui32LastTileX > ui32ScreenXMax)
			{
				ui32LastTileX = ui32ScreenXMax;
			}
			if (ui32LastTileY > ui32ScreenYMax)
			{
				ui32LastTileY = ui32ScreenYMax;
			}

			ui32RgnOffs = EURASIA_REGIONHEADER_SIZE * TilePositionToOffset(ui32LastTileX,
					ui32LastTileY,
					ui32TilesPerMTileX,
					ui32TilesPerMTileY,
					ui32MacroTilesX);

#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH)
			if ((ui32ScreenXMax >= (ui32X * ui32TilesPerMTileX))
					&& (ui32ScreenYMax >= (ui32Y * ui32TilesPerMTileY)))
			{
				ui32RgnOffs |= SGXMKIF_HWRTDATA_RGNOFFSET_VALID;
			}
#endif
			/* write the lookup table*/
			* pui32MacroTileLastRgn++ = ui32RgnOffs;
		}
	}

#if defined(FIX_HW_BRN_23862) || defined(FIX_HW_BRN_30089) || defined(FIX_HW_BRN_29504) || defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH) \
	|| defined(FIX_HW_BRN_33753)
	for (ui32Core = 0; ui32Core < SGX_FEATURE_MP_CORE_COUNT_TA; ui32Core++)
	{
		IMG_UINT32	ui32RgnBase;

 		ui32RgnBase = psRTData->apsRgnHeaderClientMemInfo[ui32Core]->sDevVAddr.uiAddr;

		psHWRTData->sRegionArrayDevAddr[ui32Core].uiAddr = ui32RgnBase;

		/* take the last offset from above */
		psHWRTData->sLastRegionDevAddr[ui32Core].uiAddr = ui32RgnBase +
				(ui32RgnOffs & ~SGXMKIF_HWRTDATA_RGNOFFSET_VALID);
 	}
#endif
}
#endif /* defined(FIX_HW_BRN_23862) */

/*!
 * *****************************************************************************
 * @brief Free render target memory
 *
 * @param psDevData
 * @param psRTDataSet
 *
 * @return PVRSRV_ERROR
 ********************************************************************************/
static IMG_VOID FreeRTDataSet(PVRSRV_DEV_DATA *psDevData,
							  SGX_RTDATASET *psRTDataSet)
{
	IMG_UINT32 i;
	IMG_UINT32 j;
	PVRSRV_ERROR eError;

	/* free any non-null meminfos */
	for(i=0; i < psRTDataSet->ui32NumRTData; i++)
	{
		for(j=0; j<SGX_FEATURE_MP_CORE_COUNT_TA; j++)
		{
			if(psRTDataSet->psRTData[i].apsRgnHeaderClientMemInfo[j])
			{
				PVRSRVFreeDeviceMem(psDevData, psRTDataSet->psRTData[i].apsRgnHeaderClientMemInfo[j]);
			}
		}

		if(psRTDataSet->psRTData[i].psContextStateClientMemInfo)
		{
			PVRSRVFreeDeviceMem(psDevData, psRTDataSet->psRTData[i].psContextStateClientMemInfo);
		}

#if !defined(SGX_FEATURE_ISP_BREAKPOINT_RESUME_REV_2) && !defined(FIX_HW_BRN_23761) && !defined(SGX_FEATURE_VISTEST_IN_MEMORY)
		if(psRTDataSet->psRTData[i].psTileRgnLUTClientMemInfo)
		{
			PVRSRVFreeDeviceMem(psDevData, psRTDataSet->psRTData[i].psTileRgnLUTClientMemInfo);
		}
#endif

#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH) || defined(FIX_HW_BRN_23862) || defined(FIX_HW_BRN_30089) || defined(FIX_HW_BRN_29504) \
	|| defined(FIX_HW_BRN_33753)
		if(psRTDataSet->psRTData[i].psLastRgnLUTClientMemInfo)
		{
			PVRSRVFreeDeviceMem(psDevData, psRTDataSet->psRTData[i].psLastRgnLUTClientMemInfo);
		}
#endif
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
		if (psRTDataSet->psRTData[i].psStatusClientMemInfo)
		{
			PVRSRVFreeDeviceMem(psDevData, psRTDataSet->psRTData[i].psStatusClientMemInfo);
		}
#endif
	}

	if(psRTDataSet->psSpecialObjClientMemInfo)
	{
		PVRSRVFreeDeviceMem(psDevData, psRTDataSet->psSpecialObjClientMemInfo);
	}

#if defined(SGX_FEATURE_MP)
#if defined(EUR_CR_MASTER_DPM_MTILE_PARTI_PIM_TABLE_BASE_ADDR)
	if(psRTDataSet->psPIMTableClientMemInfo)
	{
		PVRSRVFreeDeviceMem(psDevData, psRTDataSet->psPIMTableClientMemInfo);
	}
#endif
#endif

	for(j=0; j<SGX_FEATURE_MP_CORE_COUNT_TA; j++)
	{
		if(psRTDataSet->apsTailPtrsClientMemInfo[j])
		{
			PVRSRVFreeDeviceMem(psDevData, psRTDataSet->apsTailPtrsClientMemInfo[j]);
		}

		if(psRTDataSet->psContextControlClientMemInfo[j])
		{
			PVRSRVFreeDeviceMem(psDevData, psRTDataSet->psContextControlClientMemInfo[j]);
		}

		if(psRTDataSet->psContextOTPMClientMemInfo[j])
		{
			PVRSRVFreeDeviceMem(psDevData, psRTDataSet->psContextOTPMClientMemInfo[j]);
		}
	}

	if(psRTDataSet->psHWRTDataSetClientMemInfo)
	{
		PVRSRVFreeDeviceMem(psDevData, psRTDataSet->psHWRTDataSetClientMemInfo);
	}

	if(psRTDataSet->psPendingCountClientMemInfo)
	{
		PVRSRVFreeDeviceMem(psDevData, psRTDataSet->psPendingCountClientMemInfo);
	}

#if defined(FIX_HW_BRN_32044)
	if(psRTDataSet->ps32044CtlStrmClientMemInfo)
	{
		PVRSRVFreeDeviceMem(psDevData, psRTDataSet->ps32044CtlStrmClientMemInfo);
	}
#endif

	/* Free render details objects */
	while (psRTDataSet->psRenderDetailsList!= IMG_NULL)
	{
		DestroyRenderDetails(psDevData, psRTDataSet, psRTDataSet->psRenderDetailsList);
	}

	/* Free device sync list objects */
	while (psRTDataSet->psDeviceSyncList!= IMG_NULL)
	{
		DestroyDeviceSyncList(psDevData, psRTDataSet, psRTDataSet->psDeviceSyncList);
	}

	if(psRTDataSet->hMutex)
	{
		/* Destroy the mutex */
		eError = PVRSRVDestroyMutex(psRTDataSet->hMutex);

		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "FreeRTDataSet: Failed to destroy render context mutex\n"));
		}
	}

	/* Free host memory structures */
	PVRSRVFreeUserModeMem(psRTDataSet->psRTData);
	PVRSRVFreeUserModeMem(psRTDataSet);
}


/*!
 * *****************************************************************************
 * @brief Remove a render target
 *
 * @param psDevData
 * @param hRenderContext
 * @param hRTDataSet
 *
 * @return PVRSRV_ERROR
 ********************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV
SGXRemoveRenderTarget(PVRSRV_DEV_DATA *psDevData,
					  IMG_HANDLE hRenderContext,
					  IMG_HANDLE hRTDataSet)
{
	SGX_RENDERCONTEXT *psRenderContext = hRenderContext;
	SGX_RTDATASET *psRTDataSet=hRTDataSet;
#if defined(DEBUG)
	SGX_CLIENTPBDESC *psClientPBDesc = psRenderContext->psClientPBDesc;
	SGX_PBDESC *psPBDesc = psClientPBDesc->psPBDesc;
#endif

	PVRSRV_ERROR	eError = PVRSRV_OK;
	IMG_BOOL	bFreeRT;

	PVRSRVLockMutex(psRenderContext->hMutex);

#if defined(DEBUG)
	/* As we are removing a RT from the PB let's modify the values */
	PVR_DPF((PVR_DBG_WARNING, "PB Watermark Info - Alloc: 0x%x , Free: 0x%x",
			 ((SGXMKIF_HWPBDESC*)psClientPBDesc->psHWPBDescClientMemInfo->pvLinAddr)->ui32AllocPagesWatermark,
			 (psPBDesc->ui32NumPages) - ((SGXMKIF_HWPBDESC*)psClientPBDesc->psHWPBDescClientMemInfo->pvLinAddr)->ui32AllocPagesWatermark));

	/* Output details */
	if (((PSGXMKIF_HWRTDATASET)psRTDataSet->psHWRTDataSetClientMemInfo->pvLinAddr)->ui32NumOutOfMemSignals ||
		((PSGXMKIF_HWRTDATASET)psRTDataSet->psHWRTDataSetClientMemInfo->pvLinAddr)->ui32NumSPMRenders)
	{
		PVR_DPF((PVR_DBG_WARNING, "RTDataset SPM Details: OUTOFMEM: 0x%x, SPMRenders: 0x%x",
				 ((PSGXMKIF_HWRTDATASET)psRTDataSet->psHWRTDataSetClientMemInfo->pvLinAddr)->ui32NumOutOfMemSignals,
				 ((PSGXMKIF_HWRTDATASET)psRTDataSet->psHWRTDataSetClientMemInfo->pvLinAddr)->ui32NumSPMRenders));
	}
#endif /* DEBUG */

	psRTDataSet->ui32RefCount--;

	/* Testing the ref count won't be safe once the mutex is dropped,
	 * so record a null ref count using a boolean.
	 */
	bFreeRT = (IMG_BOOL)(psRTDataSet->ui32RefCount == 0);

	if (bFreeRT)
	{
		/*
		   The first render target to be destroyed tell the uKernel to
		   cleanup the RenderContext.
		*/
		eError = SGXFlushHWRenderTarget(psDevData, psRTDataSet->psHWRTDataSetClientMemInfo);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to flush HW render context (%d)", eError));
		}

		/* Wait for ukernel tasks to flush out. */
		#if defined(EUR_CR_USE0_DM_SLOT)
		PDUMPREGPOL(psDevData, "SGXREG", SGX_MP_CORE_SELECT(EUR_CR_USE0_DM_SLOT, 0), 0xAAAAAAAA, 0xFFFFFFFF);
		#else
		PDUMPREGPOL(psDevData, "SGXREG", SGX_MP_CORE_SELECT(EUR_CR_USE_DM_SLOT, 0), 0xAAAAAAAA, 0xFFFFFFFF);
		#endif /* EUR_CR_USE0_DM_SLOT */

		/* Remove from render context's linked list */
		if (psRTDataSet == psRenderContext->psRTDataSets)
		{
			psRenderContext->psRTDataSets = psRTDataSet->psNext;
		}
		else
		{
			PSGX_RTDATASET	psCurrentRTDataSet;

			psCurrentRTDataSet = psRenderContext->psRTDataSets;
			while (psCurrentRTDataSet)
			{
				if (psRTDataSet == psCurrentRTDataSet->psNext)
				{
					psCurrentRTDataSet->psNext = psRTDataSet->psNext;
					break;
				}
				psCurrentRTDataSet = psCurrentRTDataSet->psNext;
			}
		}
	}

	PVRSRVUnlockMutex(psRenderContext->hMutex);

	if (bFreeRT)
	{
		/* Free the render target memory */
		FreeRTDataSet(psDevData, psRTDataSet);
	}

	return eError;
}

#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
#define	MATCH_RENDER_TARGET(psRT, psInfo) ( \
	((psRT)->ui32Flags			== (psInfo)->ui32Flags) && \
	((psRT)->ui32NumPixelsX		== (psInfo)->ui32NumPixelsX) && \
	((psRT)->ui32NumPixelsY		== (psInfo)->ui32NumPixelsY) && \
	((psRT)->ui16MSAASamplesInX	== (psInfo)->ui16MSAASamplesInX) && \
	((psRT)->ui16MSAASamplesInY	== (psInfo)->ui16MSAASamplesInY) && \
	((psRT)->ui32BGObjUCoord		== (psInfo)->ui32BGObjUCoord) && \
	((psRT)->ui16NumRTsInArray		== (psInfo)->ui16NumRTsInArray))
#else
#define	MATCH_RENDER_TARGET(psRT, psInfo) ( \
	((psRT)->ui32Flags			== (psInfo)->ui32Flags) && \
	((psRT)->ui32NumPixelsX		== (psInfo)->ui32NumPixelsX) && \
	((psRT)->ui32NumPixelsY		== (psInfo)->ui32NumPixelsY) && \
	((psRT)->ui16MSAASamplesInX	== (psInfo)->ui16MSAASamplesInX) && \
	((psRT)->ui16MSAASamplesInY	== (psInfo)->ui16MSAASamplesInY) && \
	((psRT)->ui32BGObjUCoord		== (psInfo)->ui32BGObjUCoord))
#endif

static IMG_VOID SetMT4Mode(SGX_RTDATASET 	*psRTDataSet,
							IMG_UINT32			ui32NumTilesX,
							IMG_UINT32			ui32NumTilesY)

{
	/*
		Setup 4 macrotiles with a multiple of 4x4 tiles per macrotile
	*/
	psRTDataSet->ui32MTileNumber	= 0;
	if(1 == psRTDataSet->ui16MSAASamplesInX)
	{
		psRTDataSet->ui32MTileX2		= (((ui32NumTilesX + 1) / 2) + 3) & ~3UL;
	}
	else
	{
		PVR_ASSERT(2 == psRTDataSet->ui16MSAASamplesInX);
		psRTDataSet->ui32MTileX2		= (((ui32NumTilesX + 1) / 2) + 1) & ~1UL;
	}

	if(1 == psRTDataSet->ui16MSAASamplesInY)
	{
		psRTDataSet->ui32MTileY2		= (((ui32NumTilesY + 1) / 2) + 3) & ~3UL;
	}
	else
	{
		PVR_ASSERT(2 == psRTDataSet->ui16MSAASamplesInY);
		psRTDataSet->ui32MTileY2		= (((ui32NumTilesY + 1) / 2) + 1) & ~1UL;
	}
	
	/* Do not remove following setup. Not strictly necessary for all cores, but better safe than sorry */
	psRTDataSet->ui32MTileX1		= psRTDataSet->ui32MTileX2;
	psRTDataSet->ui32MTileY1		= psRTDataSet->ui32MTileY2;
	psRTDataSet->ui32MTileX3		= psRTDataSet->ui32MTileX1;
	psRTDataSet->ui32MTileY3		= psRTDataSet->ui32MTileY1;

	psRTDataSet->ui32ScreenXMax		= ui32NumTilesX - 1;
	psRTDataSet->ui32ScreenYMax		= ui32NumTilesY - 1;
	psRTDataSet->ui32MTileStride	= psRTDataSet->ui32MTileX2 * psRTDataSet->ui32MTileY2;
}


static IMG_VOID SetMT16Mode(SGX_RTDATASET	*psRTDataSet,
							IMG_UINT32			ui32NumTilesX,
							IMG_UINT32			ui32NumTilesY)
{
	/*
	   Setup 16 macrotiles with a multiple of 4x4 tiles per macrotile
	*/
	psRTDataSet->ui32MTileNumber	= 1;
	if(1 == psRTDataSet->ui16MSAASamplesInX)
	{
		psRTDataSet->ui32MTileX1		= (((ui32NumTilesX + 3) / 4) + 3) & ~3UL;
	}
	else
	{
		PVR_ASSERT(2 == psRTDataSet->ui16MSAASamplesInX);
		psRTDataSet->ui32MTileX1		= (((ui32NumTilesX + 3) / 4) + 1) & ~1UL;
	}
	psRTDataSet->ui32MTileX2		= 2 * psRTDataSet->ui32MTileX1;
	psRTDataSet->ui32MTileX3		= 3 * psRTDataSet->ui32MTileX1;

	if(1 == psRTDataSet->ui16MSAASamplesInY)
	{
			psRTDataSet->ui32MTileY1	= (((ui32NumTilesY + 3) / 4) + 3) & ~3UL;
	}
	else
	{
		PVR_ASSERT(2 == psRTDataSet->ui16MSAASamplesInY);
		psRTDataSet->ui32MTileY1		= (((ui32NumTilesY + 3) / 4) + 1) & ~1UL;

	}

	psRTDataSet->ui32MTileY2		= 2 * psRTDataSet->ui32MTileY1;
	psRTDataSet->ui32MTileY3		= 3 * psRTDataSet->ui32MTileY1;
	psRTDataSet->ui32ScreenXMax		= ui32NumTilesX - 1;
	psRTDataSet->ui32ScreenYMax		= ui32NumTilesY - 1;
	psRTDataSet->ui32MTileStride	= psRTDataSet->ui32MTileX1 * psRTDataSet->ui32MTileY1;
}


/*****************************************************************************
FUNCTION	: SGXAddRenderTarget

PURPOSE	: Adds render target to specified PB.

PARAMETERS	: psDevData, psAddRTInfo, phRTDataSet

RETURNS	: PVRSRV_ERROR
 *****************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV
SGXAddRenderTarget(PVRSRV_DEV_DATA *psDevData,
				   SGX_ADDRENDTARG *psAddRTInfo,
				   IMG_HANDLE *phRTDataSet)
{
	SGX_RENDERCONTEXT	*psRenderContext = (SGX_RENDERCONTEXT *)psAddRTInfo->hRenderContext;
	SGX_RTDATASET	*psRTDataSet;
	SGX_RTDATASET	*psRTDataSetToFree = IMG_NULL;
	IMG_UINT32		ui32NumPixelsX;
	IMG_UINT32		ui32NumPixelsY;
	IMG_UINT16		ui16MSAASamplesInX;
	IMG_UINT16		ui16MSAASamplesInY;
	IMG_UINT32		ui32MTEMultiSampleCtl = 0;
	IMG_UINT32		ui32ISPMultiSampleCtl = 0;
	IMG_UINT32		ui32BGObjUCoord;
	IMG_UINT32		ui32NumTilesX;
	IMG_UINT32		ui32NumTilesY;
	IMG_UINT32		ui32MTThreshold;
	PVRSRV_ERROR	eError = PVRSRV_OK;
	IMG_UINT32		ui32RendersPerFrame;
	IMG_UINT32		ui32NumRTData;

#if defined(FIX_HW_BRN_22837)
	/* FIX_HW_BRN_22837 - This code ensure the render target is no larger than 2k x 2k */
#endif
	/* Fail if the render target is greater than we support */
	if ((psAddRTInfo->ui32NumPixelsX > EURASIA_RENDERSIZE_MAXX) ||
		(psAddRTInfo->ui32NumPixelsY > EURASIA_RENDERSIZE_MAXY))
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXAddRenderTarget - Max supported RT size is %d x %d pixels!",
									EURASIA_RENDERSIZE_MAXX, EURASIA_RENDERSIZE_MAXY));
		eError = PVRSRV_ERROR_NOT_SUPPORTED;
		goto ExitErrorNoUnlock;
	}
	if (psAddRTInfo->ui16NumRTsInArray == 0)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXAddRenderTarget - Min NumRTsinArray is 1!"));
		eError = PVRSRV_ERROR_NOT_SUPPORTED;
		goto ExitErrorNoUnlock;
	}
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	if (psAddRTInfo->ui16NumRTsInArray >= SGX_FEATURE_MAX_TA_RENDER_TARGETS)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXAddRenderTarget - Max TA Render targets is %d!",
					SGX_FEATURE_MAX_TA_RENDER_TARGETS));
		eError = PVRSRV_ERROR_NOT_SUPPORTED;
		goto ExitErrorNoUnlock;
	}
#else
	if (psAddRTInfo->ui16NumRTsInArray > 1)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXAddRenderTarget - Max TA Render targets is 1"));
		eError = PVRSRV_ERROR_NOT_SUPPORTED;
		goto ExitErrorNoUnlock;
	}
#endif

	/*
		Fail if the multisampling requests can't be honoured
		currently support:
		1 x 1 (x, y)
		2 x 1
		2 x 2
	*/
	if (!(((psAddRTInfo->ui16MSAASamplesInX == 1) && (psAddRTInfo->ui16MSAASamplesInY == 1))
#if defined(SGX_FEATURE_MSAA_2X_IN_X)
	||	((psAddRTInfo->ui16MSAASamplesInX == 2) && (psAddRTInfo->ui16MSAASamplesInY == 1))
#endif
#if defined(SGX_FEATURE_MSAA_2X_IN_Y)
	||	((psAddRTInfo->ui16MSAASamplesInX == 1) && (psAddRTInfo->ui16MSAASamplesInY == 2))
#endif
	||	((psAddRTInfo->ui16MSAASamplesInX == 2) && (psAddRTInfo->ui16MSAASamplesInY == 2))))
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXAddRenderTarget: ui16MSAASamplesInX/Y, pixel samples not supported"));
		eError = PVRSRV_ERROR_NOT_SUPPORTED;
		goto ExitErrorNoUnlock;
	}


	ui32NumPixelsX		= psAddRTInfo->ui32NumPixelsX;
	ui32NumPixelsY		= psAddRTInfo->ui32NumPixelsY;
	ui32BGObjUCoord		= psAddRTInfo->ui32BGObjUCoord;
	ui16MSAASamplesInX	= psAddRTInfo->ui16MSAASamplesInX;
	ui16MSAASamplesInY	= psAddRTInfo->ui16MSAASamplesInY;

	if (psAddRTInfo->ui32RendersPerFrame == 0)
	{
		ui32RendersPerFrame = DEFAULT_RENDERS_PER_FRAME;
	}
	else
	{
		ui32RendersPerFrame = psAddRTInfo->ui32RendersPerFrame;
	}
	/* Assume 1 frame of overlap between TA and 3D, so max RTDATA is 2x the renders per frame at max overlap */
	ui32NumRTData = 2 * ui32RendersPerFrame;

	if (psAddRTInfo->ui32Flags & SGX_ADDRTFLAGS_SHAREDRTDATA)
	{
		PVRSRVLockMutex(psRenderContext->hMutex);

		/*
		 * Check for an existing data set matching the specified
		 * requirements.
		 */
		for (psRTDataSet = psRenderContext->psRTDataSets;
			 psRTDataSet != IMG_NULL;
			 psRTDataSet = psRTDataSet->psNext)
		{
			if (MATCH_RENDER_TARGET(psRTDataSet, psAddRTInfo))
			{
				goto ExitOK;
			}
		}

		/*
		 * Drop the mutex whilst creating the data set.  This means
		 * the check for a suitable already existing data set will
		 * need to be repeated later, and the new data set may have
		 * to be thrown away.
		 */
		PVRSRVUnlockMutex(psRenderContext->hMutex);
	}

	/*
	   OK, alloc mem for Host side structures...
	*/
	psRTDataSet = PVRSRVAllocUserModeMem(sizeof(SGX_RTDATASET));

	if(!psRTDataSet)
	{
		PVR_DPF((PVR_DBG_ERROR,"ERROR - Failed to alloc host mem for TS data set !"));

		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ExitErrorNoUnlock;
	}
	psRTDataSetToFree = psRTDataSet;

	/* clear memory to zero for pointer checking */
	PVRSRVMemSet(psRTDataSet, 0, sizeof(SGX_RTDATASET));

	/*
	 * Allocate RTDATA array.
	 */
	psRTDataSet->psRTData = PVRSRVAllocUserModeMem(ui32NumRTData * sizeof(SGX_RTDATA));
	if(psRTDataSet->psRTData == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,"ERROR - Failed to alloc host mem for RTdata array"));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ExitErrorNoUnlock;
	}

	PVRSRVMemSet(psRTDataSet->psRTData, 0, ui32NumRTData * sizeof(SGX_RTDATA));
	
	psRTDataSet->ui32NumRTData = ui32NumRTData;
	
	/*
	   Setup single instance data for Host
	*/
	psRTDataSet->ui32Flags			= 0;
	psRTDataSet->ui32NumPixelsX		= ui32NumPixelsX;
	psRTDataSet->ui32NumPixelsY		= ui32NumPixelsY;
	psRTDataSet->ui16MSAASamplesInX	= ui16MSAASamplesInX;
	psRTDataSet->ui16MSAASamplesInY	= ui16MSAASamplesInY;
	psRTDataSet->ui32BGObjUCoord	= ui32BGObjUCoord;
	psRTDataSet->eRotation			= psAddRTInfo->eRotation;
	psRTDataSet->ui32RTDataSel		= 0;
	psRTDataSet->ui32RefCount		= 0;
	psRTDataSet->ui16NumRTsInArray	= psAddRTInfo->ui16NumRTsInArray;

	/*
	   Set up the OpenGL flag on the TS dataset.
	   */
	if (psAddRTInfo->ui32Flags & SGX_ADDRTFLAGS_USEOGLMODE)
	{
		psRTDataSet->ui32Flags |= SGX_RTDSFLAGS_OPENGL;
	}
	if (psAddRTInfo->ui32Flags & SGX_ADDRTFLAGS_DPMZLS)
	{
		PVR_DPF((PVR_DBG_WARNING,"SGX_ADDRTFLAGS_DPMZLS ignore in current build config"));
	}
	

	/*
	   Setup the macrotile sizes
	*/
	ui32NumTilesX = (ui32NumPixelsX + (EURASIA_ISPREGION_SIZEX - 1)) / EURASIA_ISPREGION_SIZEX;
	ui32NumTilesY = (ui32NumPixelsY + (EURASIA_ISPREGION_SIZEY - 1)) / EURASIA_ISPREGION_SIZEY;

	/* check apphint for default threshold override */
	ui32MTThreshold = SGX_MT_DEFAULT_THRESHOLD;

	{
		IMG_VOID* pvHintState;
		IMG_UINT32 ui32AppHintDefault;
		PVRSRVCreateAppHintState(IMG_SRVCLIENT, 0, &pvHintState);
		ui32AppHintDefault = ui32MTThreshold;
		PVRSRVGetAppHint(pvHintState, "MTModePixelThreshold", IMG_UINT_TYPE, &ui32AppHintDefault, &ui32MTThreshold);
		PVRSRVFreeAppHintState(IMG_SRVCLIENT, pvHintState);
	}

	if ((ui32NumPixelsX * ui32NumPixelsY >= ui32MTThreshold)
#if defined(FIX_HW_BRN_23055)/* this is also dependent on the implicit presence of SPM Mode 1 */
		&& ((psAddRTInfo->ui32Flags & SGX_ADDRTFLAGS_FORCE_MT_MODE_SELECT) != 0)
#endif
		)
	{
		SetMT16Mode(psRTDataSet, ui32NumTilesX, ui32NumTilesY);
	}
	else
	{
		SetMT4Mode(psRTDataSet, ui32NumTilesX, ui32NumTilesY);
	}


	/*
		setup the MSAA config
	*/
	if (psRTDataSet->ui16MSAASamplesInX == 1)
	{
		psRTDataSet->eScalingInX = SGX_SCALING_NONE;
		if (psRTDataSet->ui16MSAASamplesInY == 1)
		{
			psRTDataSet->eScalingInY = SGX_SCALING_NONE;
			psRTDataSet->ui323DAAMode = 0;

			/* single sample per pixel */
			if (psRTDataSet->ui32Flags & SGX_RTDSFLAGS_OPENGL)
			{
				ui32MTEMultiSampleCtl =	(0x8 << EUR_CR_MTE_MULTISAMPLECTL_MSAA_X0_SHIFT) |
										(0x8 << EUR_CR_MTE_MULTISAMPLECTL_MSAA_Y0_SHIFT);
				ui32ISPMultiSampleCtl =	(0x8 << EUR_CR_ISP_MULTISAMPLECTL_MSAA_X0_SHIFT) |
										(0x8 << EUR_CR_ISP_MULTISAMPLECTL_MSAA_Y0_SHIFT);
			}
			else
			{
				ui32MTEMultiSampleCtl = 0;
				ui32ISPMultiSampleCtl = 0;
			}
		}
#if defined(SGX_FEATURE_MSAA_2X_IN_Y)
		else if (psRTDataSet->ui16MSAASamplesInY == 2)
		{
			psRTDataSet->eScalingInY = SGX_DOWNSCALING;
			psRTDataSet->ui323DAAMode = EUR_CR_3D_AA_MODE_VALUE_2X;
			if ((psAddRTInfo->eForceScalingInX == SGX_UPSCALING)
			||	(psAddRTInfo->eForceScalingInY == SGX_UPSCALING))
			{
				ui32MTEMultiSampleCtl =	(4UL  << EUR_CR_MTE_MULTISAMPLECTL_MSAA_X0_SHIFT) |
										(8UL  << EUR_CR_MTE_MULTISAMPLECTL_MSAA_Y0_SHIFT) |
										(12UL << EUR_CR_MTE_MULTISAMPLECTL_MSAA_X1_SHIFT) |
										(8UL << EUR_CR_MTE_MULTISAMPLECTL_MSAA_Y1_SHIFT);
				ui32ISPMultiSampleCtl =	(4UL  << EUR_CR_ISP_MULTISAMPLECTL_MSAA_X0_SHIFT) |
										(8UL  << EUR_CR_ISP_MULTISAMPLECTL_MSAA_Y0_SHIFT) |
										(12UL << EUR_CR_ISP_MULTISAMPLECTL_MSAA_X1_SHIFT) |
										(8UL << EUR_CR_ISP_MULTISAMPLECTL_MSAA_Y1_SHIFT);
			}
			else
			{
				ui32MTEMultiSampleCtl =	(4UL  << EUR_CR_MTE_MULTISAMPLECTL_MSAA_X0_SHIFT) |
										(5UL  << EUR_CR_MTE_MULTISAMPLECTL_MSAA_Y0_SHIFT) |
										(12UL << EUR_CR_MTE_MULTISAMPLECTL_MSAA_X1_SHIFT) |
										(11UL << EUR_CR_MTE_MULTISAMPLECTL_MSAA_Y1_SHIFT);
				ui32ISPMultiSampleCtl =	(4UL  << EUR_CR_ISP_MULTISAMPLECTL_MSAA_X0_SHIFT) |
										(5UL  << EUR_CR_ISP_MULTISAMPLECTL_MSAA_Y0_SHIFT) |
										(12UL << EUR_CR_ISP_MULTISAMPLECTL_MSAA_X1_SHIFT) |
										(11UL << EUR_CR_ISP_MULTISAMPLECTL_MSAA_Y1_SHIFT);
			}
		}
#endif /* #if defined(SGX_FEATURE_MSAA_2X_IN_Y) */
		else
		{
			PVR_DPF((PVR_DBG_ERROR,"SGXAddRenderTarget: this code path should be unreachable)"));
			PVR_DBG_BREAK;
		}
	}
	else if (psRTDataSet->ui16MSAASamplesInX == 2)
	{
		psRTDataSet->eScalingInX = SGX_DOWNSCALING;
#if defined(SGX_FEATURE_MSAA_2X_IN_X)
		if (psRTDataSet->ui16MSAASamplesInY == 1)
		{
			psRTDataSet->eScalingInY = SGX_SCALING_NONE;
			psRTDataSet->ui323DAAMode = EUR_CR_3D_AA_MODE_VALUE_2X;
			if ((psAddRTInfo->eForceScalingInX == SGX_UPSCALING)
			||	(psAddRTInfo->eForceScalingInY == SGX_UPSCALING))
			{
				ui32MTEMultiSampleCtl =	(4UL  << EUR_CR_MTE_MULTISAMPLECTL_MSAA_X0_SHIFT) |
										(8UL  << EUR_CR_MTE_MULTISAMPLECTL_MSAA_Y0_SHIFT) |
										(12UL << EUR_CR_MTE_MULTISAMPLECTL_MSAA_X1_SHIFT) |
										(8UL << EUR_CR_MTE_MULTISAMPLECTL_MSAA_Y1_SHIFT);
				ui32ISPMultiSampleCtl =	(4UL  << EUR_CR_ISP_MULTISAMPLECTL_MSAA_X0_SHIFT) |
										(8UL  << EUR_CR_ISP_MULTISAMPLECTL_MSAA_Y0_SHIFT) |
										(12UL << EUR_CR_ISP_MULTISAMPLECTL_MSAA_X1_SHIFT) |
										(8UL << EUR_CR_ISP_MULTISAMPLECTL_MSAA_Y1_SHIFT);
			}
			else
			{
				ui32MTEMultiSampleCtl =	(4UL  << EUR_CR_MTE_MULTISAMPLECTL_MSAA_X0_SHIFT) |
										(5UL  << EUR_CR_MTE_MULTISAMPLECTL_MSAA_Y0_SHIFT) |
										(12UL << EUR_CR_MTE_MULTISAMPLECTL_MSAA_X1_SHIFT) |
										(11UL << EUR_CR_MTE_MULTISAMPLECTL_MSAA_Y1_SHIFT);
				ui32ISPMultiSampleCtl =	(4UL  << EUR_CR_ISP_MULTISAMPLECTL_MSAA_X0_SHIFT) |
										(5UL  << EUR_CR_ISP_MULTISAMPLECTL_MSAA_Y0_SHIFT) |
										(12UL << EUR_CR_ISP_MULTISAMPLECTL_MSAA_X1_SHIFT) |
										(11UL << EUR_CR_ISP_MULTISAMPLECTL_MSAA_Y1_SHIFT);
			}
		}
		else
#endif /* #if defined(SGX_FEATURE_MSAA_2X_IN_X) */
		if (psRTDataSet->ui16MSAASamplesInY == 2)
		{
			psRTDataSet->eScalingInY = SGX_DOWNSCALING;
			#if defined(EUR_CR_3D_AA_MODE_VALUE_4X)
			psRTDataSet->ui323DAAMode = EUR_CR_3D_AA_MODE_VALUE_4X;
			#else
			psRTDataSet->ui323DAAMode = EUR_CR_3D_AA_MODE_ENABLE_MASK;
			#endif
			if ((psAddRTInfo->eForceScalingInX == SGX_UPSCALING)
			||	(psAddRTInfo->eForceScalingInY == SGX_UPSCALING))
			{
				ui32MTEMultiSampleCtl =	(4UL  << EUR_CR_MTE_MULTISAMPLECTL_MSAA_X0_SHIFT) |
										(4UL  << EUR_CR_MTE_MULTISAMPLECTL_MSAA_Y0_SHIFT) |
										(12UL << EUR_CR_MTE_MULTISAMPLECTL_MSAA_X1_SHIFT) |
										(4UL  << EUR_CR_MTE_MULTISAMPLECTL_MSAA_Y1_SHIFT) |
										(4UL  << EUR_CR_MTE_MULTISAMPLECTL_MSAA_X2_SHIFT) |
										(12UL << EUR_CR_MTE_MULTISAMPLECTL_MSAA_Y2_SHIFT) |
										(12UL << EUR_CR_MTE_MULTISAMPLECTL_MSAA_X3_SHIFT) |
										(12UL << EUR_CR_MTE_MULTISAMPLECTL_MSAA_Y3_SHIFT);
				ui32ISPMultiSampleCtl =	(4UL  << EUR_CR_ISP_MULTISAMPLECTL_MSAA_X0_SHIFT) |
										(4UL  << EUR_CR_ISP_MULTISAMPLECTL_MSAA_Y0_SHIFT) |
										(12UL << EUR_CR_ISP_MULTISAMPLECTL_MSAA_X1_SHIFT) |
										(4UL  << EUR_CR_ISP_MULTISAMPLECTL_MSAA_Y1_SHIFT) |
										(4UL  << EUR_CR_ISP_MULTISAMPLECTL_MSAA_X2_SHIFT) |
										(12UL << EUR_CR_ISP_MULTISAMPLECTL_MSAA_Y2_SHIFT) |
										(12UL << EUR_CR_ISP_MULTISAMPLECTL_MSAA_X3_SHIFT) |
										(12UL << EUR_CR_ISP_MULTISAMPLECTL_MSAA_Y3_SHIFT);
			}
			else
			{
				ui32MTEMultiSampleCtl =	(6UL  << EUR_CR_MTE_MULTISAMPLECTL_MSAA_X0_SHIFT) |
										(2UL  << EUR_CR_MTE_MULTISAMPLECTL_MSAA_Y0_SHIFT) |
										(14UL << EUR_CR_MTE_MULTISAMPLECTL_MSAA_X1_SHIFT) |
										(6UL  << EUR_CR_MTE_MULTISAMPLECTL_MSAA_Y1_SHIFT) |
										(2UL  << EUR_CR_MTE_MULTISAMPLECTL_MSAA_X2_SHIFT) |
										(10UL << EUR_CR_MTE_MULTISAMPLECTL_MSAA_Y2_SHIFT) |
										(10UL << EUR_CR_MTE_MULTISAMPLECTL_MSAA_X3_SHIFT) |
										(14UL << EUR_CR_MTE_MULTISAMPLECTL_MSAA_Y3_SHIFT);
				ui32ISPMultiSampleCtl =	(6UL  << EUR_CR_ISP_MULTISAMPLECTL_MSAA_X0_SHIFT) |
										(2UL  << EUR_CR_ISP_MULTISAMPLECTL_MSAA_Y0_SHIFT) |
										(14UL << EUR_CR_ISP_MULTISAMPLECTL_MSAA_X1_SHIFT) |
										(6UL  << EUR_CR_ISP_MULTISAMPLECTL_MSAA_Y1_SHIFT) |
										(2UL  << EUR_CR_ISP_MULTISAMPLECTL_MSAA_X2_SHIFT) |
										(10UL << EUR_CR_ISP_MULTISAMPLECTL_MSAA_Y2_SHIFT) |
										(10UL << EUR_CR_ISP_MULTISAMPLECTL_MSAA_X3_SHIFT) |
										(14UL << EUR_CR_ISP_MULTISAMPLECTL_MSAA_Y3_SHIFT);
			}
		}
		else
		{
			PVR_DPF((PVR_DBG_ERROR,"SGXAddRenderTarget: this code path should be unreachable)"));
			PVR_DBG_BREAK;
		}
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXAddRenderTarget: this code path should be unreachable)"));
		PVR_DBG_BREAK;
	}

#if defined(SGX_FEATURE_MSAA_5POSITIONS)
	{
		IMG_UINT32		ui32ISPMultiSampleCtl2 = 0;

		/* TODO: should this only happen when 2x2 multisample is requested from driver? */
		/* Logic here is that 5th position is enabled by default on hardware that
		   supports it, and disabled by a flag communicated from the driver */
		if (!(psAddRTInfo->ui32Flags & SGX_ADDRTFLAGS_MSAA5THPOSNDISABLE))
		{
			ui32ISPMultiSampleCtl2 |= (1 << EUR_CR_ISP_MULTISAMPLECTL2_MSAA_XY4_EN_SHIFT) |
				(8UL << EUR_CR_ISP_MULTISAMPLECTL2_MSAA_X4_SHIFT)                         |
				(8UL << EUR_CR_ISP_MULTISAMPLECTL2_MSAA_Y4_SHIFT);
		}

		psRTDataSet->ui32ISPMultiSampleCtl2 = ui32ISPMultiSampleCtl2;
	}

#endif

	psRTDataSet->ui32MTEMultiSampleCtl = ui32MTEMultiSampleCtl;
	psRTDataSet->ui32ISPMultiSampleCtl = ui32ISPMultiSampleCtl;

	switch(psAddRTInfo->eForceScalingInX)
	{
		case SGX_SCALING_NONE:
			break;
		case SGX_DOWNSCALING:
		case SGX_UPSCALING:
			psRTDataSet->eScalingInX = psAddRTInfo->eForceScalingInX;
			break;
		default:
			PVR_DPF((PVR_DBG_ERROR,"SGXAddRenderTarget: invalid force scaling request (ignored)"));
			break;
	}

	switch(psAddRTInfo->eForceScalingInY)
	{
		case SGX_SCALING_NONE:
			break;
		case SGX_DOWNSCALING:
		case SGX_UPSCALING:
			psRTDataSet->eScalingInY = psAddRTInfo->eForceScalingInY;
			break;
		default:
			PVR_DPF((PVR_DBG_ERROR,"SGXAddRenderTarget: invalid force scaling request (ignored)"));
			break;
	}

	eError = SetupRTDataSet(psDevData,
							psRenderContext,
							psRTDataSet);
	if (eError != PVRSRV_OK)
	{
		goto ExitErrorNoUnlock;
	}

	/* Create the mutex */
	eError = PVRSRVCreateMutex(&psRTDataSet->hMutex);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXAddRenderTarget: Failed to create render target mutex (%d)", eError));
		goto ExitErrorNoUnlock;
	}

	PVRSRVLockMutex(psRenderContext->hMutex);

	if (psAddRTInfo->ui32Flags & SGX_ADDRTFLAGS_SHAREDRTDATA)
	{
		/*
		 * Check whether a data set matching the specified requirements
		 * was allocated whilst the mutex was dropped.
		 */
		for (psRTDataSet = psRenderContext->psRTDataSets;
			 psRTDataSet != IMG_NULL;
			 psRTDataSet = psRTDataSet->psNext)
		{
			if (MATCH_RENDER_TARGET(psRTDataSet, psAddRTInfo))
			{
				/*
				 * Use the existing data set, and free the new
				 * one.
				 */
				PVR_ASSERT(psRTDataSetToFree != IMG_NULL);	/* PRQA S 3355,3358 */ /* QAC says this is always true */
				goto ExitOK;
			}
		}
		/* no matching data set was found so use the new one */ 
		psRTDataSet = psRTDataSetToFree;
	}
	PVR_ASSERT(psRTDataSetToFree == psRTDataSet);

	/* Add the RTDataSet to the context's list of RTDataSets */
	psRTDataSet->psNext = psRenderContext->psRTDataSets;
	psRenderContext->psRTDataSets = psRTDataSet;

	/* Dump data HW data structures */
	PDUMPCOMMENT(psDevData->psConnection, "HWRTDataSet obj (SGXMKIF_HWRTDATASET)", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psRTDataSet->psHWRTDataSetClientMemInfo, 0, (IMG_UINT32)psRTDataSet->psHWRTDataSetClientMemInfo->uAllocSize, PDUMP_FLAGS_CONTINUOUS);

	/* Link back to owning context to save on argument passing */
	psRTDataSet->psRenderContext = psRenderContext;

	/* Don't free the new data set */
	psRTDataSetToFree = IMG_NULL;

ExitOK:
	psRTDataSet->ui32RefCount++;

	*phRTDataSet = psRTDataSet;

	PVRSRVUnlockMutex(psRenderContext->hMutex);

ExitErrorNoUnlock:
	if (psRTDataSetToFree != IMG_NULL)
	{
		FreeRTDataSet(psDevData, psRTDataSetToFree);
	}

	return eError;
}

/*!
 * *****************************************************************************
 * @brief Creates a new render details object
 *
 * @param psDevData
 * @param[out] ppsDeviceSyncList
 * @param hDevMemHeap
 * @param hSyncDevMemHeap
 * @param ui16NumRenderTargets
 *
 * @return
 ********************************************************************************/
static IMG_BOOL
CreateDeviceSyncList(const PVRSRV_DEV_DATA *psDevData,
					SGX_DEVICE_SYNC_LIST	**ppsDeviceSyncList,
#if defined (SUPPORT_SID_INTERFACE)
					IMG_SID    hDevMemHeap,
					IMG_SID    hSyncDevMemHeap,
#else
					IMG_HANDLE hDevMemHeap,
					IMG_HANDLE    hSyncDevMemHeap,
#endif
					IMG_UINT16 ui16NumRenderTargets)
{
	IMG_UINT32		ui32HWDeviceSyncListSize;
	PSGX_DEVICE_SYNC_LIST		psDeviceSyncList;

	/*
	 * Alloc memory for host structures
	 */
	psDeviceSyncList = PVRSRVAllocUserModeMem(sizeof(SGX_DEVICE_SYNC_LIST) +
											  (sizeof(IMG_HANDLE) * ui16NumRenderTargets));
	if(!psDeviceSyncList)
	{
		PVR_DPF((PVR_DBG_ERROR,"ERROR - Failed to alloc host mem for DevSyncList!"));
		goto ErrorExit;
	}

	PVRSRVMemSet(psDeviceSyncList, 0,
				 sizeof(SGX_DEVICE_SYNC_LIST) +
				 (sizeof(IMG_HANDLE) * ui16NumRenderTargets));

	/* Grow the size of the structure to accomdate any extra sync info*/
	ui32HWDeviceSyncListSize = sizeof(SGXMKIF_HWDEVICE_SYNC_LIST) +
								(sizeof(PVRSRV_DEVICE_SYNC_OBJECT) * ui16NumRenderTargets);

	if(PVRSRVAllocDeviceMem(psDevData,
							hDevMemHeap,
							PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | \
							PVRSRV_MEM_NO_SYNCOBJ | PVRSRV_MEM_CACHE_CONSISTENT,
							ui32HWDeviceSyncListSize,
							32,
							&psDeviceSyncList->psHWDeviceSyncListClientMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"ERROR - Failed to alloc mem for HWDevSyncList!"));

		goto ErrorExit;
	}
	psDeviceSyncList->psHWDeviceSyncList =
					psDeviceSyncList->psHWDeviceSyncListClientMemInfo->pvLinAddr;
	psDeviceSyncList->hKernelHWSyncListMemInfo =
					psDeviceSyncList->psHWDeviceSyncListClientMemInfo->hKernelMemInfo;
	PVRSRVMemSet(psDeviceSyncList->psHWDeviceSyncList, 0,
					ui32HWDeviceSyncListSize);

	if(PVRSRVAllocDeviceMem(psDevData,
							hSyncDevMemHeap,
							PVRSRV_MEM_READ | PVRSRV_MEM_WRITE |
								PVRSRV_MEM_NO_SYNCOBJ | PVRSRV_MEM_CACHE_CONSISTENT,
							sizeof(PVRSRV_RESOURCE),
							4,
							&psDeviceSyncList->psAccessResourceClientMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"ERROR - Failed to alloc access resource DevSyncList!"));

		goto ErrorExit;
	}

	psDeviceSyncList->psHWDeviceSyncList->sAccessDevAddr =
		psDeviceSyncList->psAccessResourceClientMemInfo->sDevVAddr;
	psDeviceSyncList->pui32Lock =
		(volatile IMG_UINT32*)psDeviceSyncList->psAccessResourceClientMemInfo->pvLinAddr;

	/* initialise the lock */
	*psDeviceSyncList->pui32Lock = 0;

	PDUMPCOMMENT(psDevData->psConnection,
				 "Initialise DevSyncList access semaphore",
				 IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL,
			 psDeviceSyncList->psAccessResourceClientMemInfo, 0,
			 sizeof(PVRSRV_RESOURCE), PDUMP_FLAGS_CONTINUOUS);

	*ppsDeviceSyncList = psDeviceSyncList;

	/* All done OK... */
	return IMG_TRUE;

ErrorExit:

	if (psDeviceSyncList)
	{
		if(psDeviceSyncList->psHWDeviceSyncList)
		{
			PVRSRVFreeDeviceMem(psDevData, psDeviceSyncList->psHWDeviceSyncListClientMemInfo);
		}

		PVRSRVFreeUserModeMem(psDeviceSyncList);
	}

	return IMG_FALSE;
}


/*!
 * *****************************************************************************
 * @brief Destroy the given DeviceSync object and remove it from the list
 *
 * @param psDevData
 * @param psRTDataSet
 * @param psDeviceSyncList
 *
 * @return
 *******************************************************************************/
static IMG_VOID
DestroyDeviceSyncList(const PVRSRV_DEV_DATA *psDevData,
					 PSGX_RTDATASET psRTDataSet,
					 PSGX_DEVICE_SYNC_LIST psDeviceSyncList)
{
	PSGX_DEVICE_SYNC_LIST		psThis;
	PSGX_DEVICE_SYNC_LIST		psPrev;
	
	PVR_ASSERT(*psDeviceSyncList->pui32Lock == 0);
	
	PVRSRVFreeDeviceMem(psDevData, psDeviceSyncList->psAccessResourceClientMemInfo);
	PVRSRVFreeDeviceMem(psDevData, psDeviceSyncList->psHWDeviceSyncListClientMemInfo);

	psThis = psRTDataSet->psDeviceSyncList;
	psPrev = 0;

	while (psThis != psDeviceSyncList)
	{
		psPrev = psThis;
		psThis = psThis->psNext;
	}

	if (!psPrev)
	{
		/* First item in list */
		psRTDataSet->psDeviceSyncList = psThis->psNext;
	}
	else
	{
		/* Not first item in list */
		psPrev->psNext = psThis->psNext;
	}

	PVRSRVFreeUserModeMem(psDeviceSyncList);
}

/*!
 * *****************************************************************************
 * @brief Creates a new render details object
 *
 * @param psDevData
 * @param[out] ppsRenderDetails
 * @param ui32PFlags
 * @param hDevMemHeap
 * @param hSyncDevMemHeap
 *
 * @return
 ********************************************************************************/
static IMG_BOOL
CreateRenderDetails(const PVRSRV_DEV_DATA *psDevData,
					SGX_RENDERDETAILS	**ppsRenderDetails,
#if defined (SUPPORT_SID_INTERFACE)
					IMG_SID hDevMemHeap,
					IMG_SID hSyncDevMemHeap,
#else
					IMG_HANDLE hDevMemHeap,
					IMG_HANDLE hSyncDevMemHeap,
#endif
					IMG_UINT16 ui16NumRTsInArray)
{
	PSGX_RENDERDETAILS		psRenderDetails;

#if !defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	PVR_UNREFERENCED_PARAMETER(ui16NumRTsInArray);
#endif
	/*
	 * Alloc memory for host structures
	 */
	psRenderDetails = PVRSRVAllocUserModeMem(sizeof(SGX_RENDERDETAILS));
	if(!psRenderDetails)
	{
		PVR_DPF((PVR_DBG_ERROR,"ERROR - Failed to alloc host mem for Render details !"));

		goto ErrorExit;
	}
	
	PVRSRVMemSet(psRenderDetails, 0, sizeof(SGX_RENDERDETAILS));

	psRenderDetails->psNext = IMG_NULL;
	psRenderDetails->psHWRenderDetails = 0;

	PDUMPCOMMENT(psDevData->psConnection, "Allocate HW render details (SGXMKIF_HWRENDERDETAILS)", IMG_TRUE);
	if(PVRSRVAllocDeviceMem(psDevData,
							hDevMemHeap,
							PVRSRV_MEM_READ | PVRSRV_MEM_WRITE |
								PVRSRV_MEM_NO_SYNCOBJ | PVRSRV_MEM_CACHE_CONSISTENT,
							sizeof(SGXMKIF_HWRENDERDETAILS),
							32,
							&psRenderDetails->psHWRenderDetailsClientMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"ERROR - Failed to alloc FB mem for render details structure!"));

		goto ErrorExit;
	}
	psRenderDetails->psHWRenderDetails = psRenderDetails->psHWRenderDetailsClientMemInfo->pvLinAddr;
	PVRSRVMemSet(psRenderDetails->psHWRenderDetails, 0, sizeof(SGXMKIF_HWRENDERDETAILS));

	/* Make sure previous and next pointers are 0 */
	psRenderDetails->psHWRenderDetails->sPrevDevAddr.uiAddr = 0;
	psRenderDetails->psHWRenderDetails->sNextDevAddr.uiAddr = 0;

	PDUMPCOMMENT(psDevData->psConnection, "Allocate render details access resource", IMG_TRUE);
	if(PVRSRVAllocDeviceMem(psDevData,
							hSyncDevMemHeap,
							PVRSRV_MEM_READ | PVRSRV_MEM_WRITE |
								PVRSRV_MEM_NO_SYNCOBJ | PVRSRV_MEM_CACHE_CONSISTENT,
							sizeof(PVRSRV_RESOURCE),
							4,
							&psRenderDetails->psAccessResourceClientMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"ERROR - Failed to alloc FB mem for render details access resource!"));

		goto ErrorExit;
	}

	psRenderDetails->psHWRenderDetails->sAccessDevAddr =
		psRenderDetails->psAccessResourceClientMemInfo->sDevVAddr;
	psRenderDetails->pui32Lock =
		(volatile IMG_UINT32*)psRenderDetails->psAccessResourceClientMemInfo->pvLinAddr;

	/* initialise the lock */
	*psRenderDetails->pui32Lock = 0;

	PDUMPCOMMENT(psDevData->psConnection,
				 "Initialise render details access semaphore",
				 IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL,
			 psRenderDetails->psAccessResourceClientMemInfo, 0,
			 sizeof(PVRSRV_RESOURCE), PDUMP_FLAGS_CONTINUOUS);

#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	if(PVRSRVAllocDeviceMem(psDevData,
							hDevMemHeap,
							PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ | PVRSRV_MEM_CACHE_CONSISTENT,
							EURASIA_USE_INSTRUCTION_SIZE * ui16NumRTsInArray,
							32,
							&psRenderDetails->psHWPixEventUpdateListClientMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"ERROR - Failed to alloc FB mem for render details structure!"));

		goto ErrorExit;
	}
	psRenderDetails->psHWPixEventUpdateList =
									psRenderDetails->psHWPixEventUpdateListClientMemInfo->pvLinAddr;
	psRenderDetails->psHWRenderDetails->sHWPixEventUpdateListDevAddr =
									psRenderDetails->psHWPixEventUpdateListClientMemInfo->sDevVAddr;

	PDUMPCOMMENT(psDevData->psConnection,
				 "Initialise pix event update list",
				 IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL,
			 psRenderDetails->psHWPixEventUpdateListClientMemInfo, 0,
			 EURASIA_USE_INSTRUCTION_SIZE * ui16NumRTsInArray, PDUMP_FLAGS_CONTINUOUS);

	if(PVRSRVAllocDeviceMem(psDevData,
							hDevMemHeap,
							PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ | PVRSRV_MEM_CACHE_CONSISTENT,
							sizeof(IMG_UINT32) * ui16NumRTsInArray,
							32,
							&psRenderDetails->psHWBgObjPrimPDSUpdateListClientMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"ERROR - Failed to alloc FB mem for render details structure!"));

		goto ErrorExit;
	}
	psRenderDetails->psHWBgObjPrimPDSUpdateList =
									psRenderDetails->psHWBgObjPrimPDSUpdateListClientMemInfo->pvLinAddr;
	psRenderDetails->psHWRenderDetails->sHWBgObjPrimPDSUpdateListDevAddr =
									psRenderDetails->psHWBgObjPrimPDSUpdateListClientMemInfo->sDevVAddr;

	PDUMPCOMMENT(psDevData->psConnection,
				 "Initialise HWBgObj Primary PDS state update list",
				 IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL,
			 psRenderDetails->psHWBgObjPrimPDSUpdateListClientMemInfo, 0,
			 sizeof(IMG_UINT32) * ui16NumRTsInArray, PDUMP_FLAGS_CONTINUOUS);
#endif

	*ppsRenderDetails = psRenderDetails;

	/*
	 * All done OK...
	 */
	return IMG_TRUE;

ErrorExit:

	if(psRenderDetails)
	{
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
		if(psRenderDetails->psHWPixEventUpdateList)
		{
			PVRSRVFreeDeviceMem(psDevData, psRenderDetails->psHWPixEventUpdateListClientMemInfo);
		}
#endif
		if(psRenderDetails->psHWRenderDetails)
		{
			PVRSRVFreeDeviceMem(psDevData, psRenderDetails->psHWRenderDetailsClientMemInfo);
		}
		
		if (psRenderDetails->psAccessResourceClientMemInfo)
		{
			PVRSRVFreeDeviceMem(psDevData, psRenderDetails->psAccessResourceClientMemInfo);
		}
		
		/* no need to free psRenderDetails->psHWBgObjPrimPDSUpdateListClientMemInfo
		   as it will not have been allocated*/

		PVRSRVFreeUserModeMem(psRenderDetails);
	}

	return IMG_FALSE;
}


/*!
 * *****************************************************************************
 * @brief Destroy the given details object and remove it from the list
 *
 * @param psDevData
 * @param psRTDataSet
 * @param psRenderDetails
 * @param ui32PFlags
 *
 * @return
 ********************************************************************************/
static IMG_VOID
DestroyRenderDetails(const PVRSRV_DEV_DATA *psDevData,
					 PSGX_RTDATASET psRTDataSet,
					 PSGX_RENDERDETAILS psRenderDetails)
{
	PSGX_RENDERDETAILS		psThis;
	PSGX_RENDERDETAILS		psPrev;
	
	PVR_ASSERT(*psRenderDetails->pui32Lock == 0);
	
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	PVRSRVFreeDeviceMem(psDevData, psRenderDetails->psHWPixEventUpdateListClientMemInfo);
#endif
	PVRSRVFreeDeviceMem(psDevData, psRenderDetails->psAccessResourceClientMemInfo);
	PVRSRVFreeDeviceMem(psDevData, psRenderDetails->psHWRenderDetailsClientMemInfo);

	psThis = psRTDataSet->psRenderDetailsList;
	psPrev = 0;

	while (psThis != psRenderDetails)
	{
		psPrev = psThis;
		psThis = psThis->psNext;
	}

	if (!psPrev)
	{
		/*
		   First item in list
		   */
		psRTDataSet->psRenderDetailsList = psThis->psNext;
	}
	else
	{
		/*
		   Not first item in list
		   */
		psPrev->psNext = psThis->psNext;
	}

	PVRSRVFreeUserModeMem(psRenderDetails);
}

#if defined(FIX_HW_BRN_23862)
/*!
 * *****************************************************************************
 * @brief Creates an object with the same state and dimensions as the
 * 		  background object.
 *
 * @param psRTDataSet
 * @param ui32RTDataSel
 * @param pui32VertexBlock
 * @param ui32ParamBase
 *
 * @return
 ********************************************************************************/
static IMG_VOID
SetupBRN23862FixObject(PSGX_RTDATASET psRTDataSet,
					   IMG_PUINT32 pui32VertexBlock,
					   IMG_UINT32 ui32ParamBase)
{
	IMG_UINT32			*pui32VertexBlockStart = pui32VertexBlock;
	IMG_UINT32			ui32ISPStateOffset;
	IMG_UINT32			ui32Left, ui32Top, ui32Right, ui32Bottom;
	IMG_UINT32			ui32Width, ui32Height;
	IMG_UINTPTR_T		uiTemp;
	IMG_UINT32			*pui32CtrlStream;
	IMG_UINT32			ui32VertexBlockOffset;
	IMG_UINT32			ui32VertexBlockDevAddr;
	IMG_UINT32			ui32VertexPtr;

	/* Calculate the offset after the end of the background object data. */
	ui32VertexBlockOffset =	(IMG_UINT32)((IMG_PCHAR)pui32VertexBlock -
										 (IMG_PCHAR)psRTDataSet->psSpecialObjClientMemInfo->pvLinAddr);

	/* Get the device virtual address of the start of vertex block. */
	ui32VertexBlockDevAddr = psRTDataSet->psSpecialObjClientMemInfo->sDevVAddr.uiAddr;
	ui32VertexBlockDevAddr += ui32VertexBlockOffset;

	/* Calculate the render size */
	ui32Width		= psRTDataSet->ui32NumPixelsX;
	ui32Height		= psRTDataSet->ui32NumPixelsY;

	/* The PDS state is at the start of the vertex block. */
	psRTDataSet->sBRN23862PDSStateDevAddr.uiAddr = ui32VertexBlockDevAddr;

	/*
	 *	The PDS pixel state needs to be setup according to
	 *	the render target format and stride
	*/
	*pui32VertexBlock++	= 0;
	*pui32VertexBlock++	= 0;
	*pui32VertexBlock++	= 0;

	/* TSP vertex data (U, V) */
	*pui32VertexBlock++	= 0;
	*pui32VertexBlock++	= 0;

	*pui32VertexBlock++	= 0;
	*pui32VertexBlock++	= FLOAT32_ONE;

	*pui32VertexBlock++	= psRTDataSet->ui32BGObjUCoord;
	*pui32VertexBlock++	= FLOAT32_ONE;

	*pui32VertexBlock++	= psRTDataSet->ui32BGObjUCoord;
	*pui32VertexBlock++	= 0;

	/*
	 * ISP state word A
	 */
	ui32ISPStateOffset = (IMG_UINT32)((IMG_PCHAR)pui32VertexBlock - (IMG_PCHAR)pui32VertexBlockStart);
	*pui32VertexBlock++ = EURASIA_ISPA_PASSTYPE_OPAQUE |
							EURASIA_ISPA_DCMPMODE_ALWAYS |
							EURASIA_ISPA_LINEFILLLASTPIXEL |
							EURASIA_ISPA_OBJTYPE_TRI |
							EURASIA_ISPA_DWRITEDIS;

	/*
	 * Index data
	 * NOTE: For Eurasia variants with more than 32 primitives in a primitive
	 * block, only one triangle can be packed into a dword.
	 */
	*pui32VertexBlock++ = EURASIA_PARAM_EDGEFLAG1CA |
							3UL << EURASIA_PARAM_INDEX1C_SHIFT |
							EURASIA_PARAM_EDGEFLAG1BC |
							2UL << EURASIA_PARAM_INDEX1B_SHIFT |
							EURASIA_PARAM_EDGEFLAG1AB |
							0UL << EURASIA_PARAM_INDEX1A_SHIFT |
							EURASIA_PARAM_EDGEFLAG0CA |
							2UL << EURASIA_PARAM_INDEX0C_SHIFT |
							EURASIA_PARAM_EDGEFLAG0BC |
							1UL << EURASIA_PARAM_INDEX0B_SHIFT |
							EURASIA_PARAM_EDGEFLAG0AB |
							0UL << EURASIA_PARAM_INDEX0A_SHIFT;

	/* Vertex format word */
	*pui32VertexBlock++	= 2UL << EURASIA_PARAM_VF_TSP_SIZE_SHIFT;

	/* TSP data format word */
	*pui32VertexBlock++	= 0;

	/* TSP vertex data (X, Y, Z) */
	ui32Left = (0 + EURASIA_PARAM_VF_X_OFFSET);
	ui32Left <<= EURASIA_PARAM_VF_X_FRAC;

	ui32Top = (0 + EURASIA_PARAM_VF_Y_OFFSET);
	ui32Top <<= EURASIA_PARAM_VF_Y_FRAC;

	ui32Right = (ui32Width + EURASIA_PARAM_VF_X_OFFSET);
	ui32Right <<= EURASIA_PARAM_VF_X_FRAC;

	ui32Bottom = (ui32Height + EURASIA_PARAM_VF_Y_OFFSET);
	ui32Bottom <<= EURASIA_PARAM_VF_Y_FRAC;

	/* Apply half pixel offset {-0.5, -0.5} if not an OpenGL render target */
	if((psRTDataSet->ui32Flags & SGX_RTDSFLAGS_OPENGL) == 0)
	{
		ui32Left   -= (1UL << (EURASIA_PARAM_VF_X_FRAC - 1));
		ui32Top	   -= (1UL << (EURASIA_PARAM_VF_Y_FRAC - 1));
		ui32Right  -= (1UL << (EURASIA_PARAM_VF_X_FRAC - 1));
		ui32Bottom -= (1UL << (EURASIA_PARAM_VF_Y_FRAC - 1));
	}

	*pui32VertexBlock++ = (ui32Left << EURASIA_PARAM_VF_X_SHIFT) | (ui32Top << EURASIA_PARAM_VF_Y_SHIFT);
	*pui32VertexBlock++ = FLOAT32_ONE;

	*pui32VertexBlock++ = (ui32Left << EURASIA_PARAM_VF_X_SHIFT) | (ui32Bottom << EURASIA_PARAM_VF_Y_SHIFT);
	*pui32VertexBlock++ = FLOAT32_ONE;

	*pui32VertexBlock++ = (ui32Right << EURASIA_PARAM_VF_X_SHIFT) | (ui32Bottom << EURASIA_PARAM_VF_Y_SHIFT);
	*pui32VertexBlock++ = FLOAT32_ONE;

	*pui32VertexBlock++ = (ui32Right << EURASIA_PARAM_VF_X_SHIFT) | (ui32Top << EURASIA_PARAM_VF_Y_SHIFT);
	*pui32VertexBlock++ = FLOAT32_ONE;

	/* Set up the control stream immediately after the vertex block. */
	uiTemp = ALIGNSIZE((IMG_UINTPTR_T)pui32VertexBlock, EURASIA_REGIONHEADER1_CONTROLBASE_ALIGNSHIFT);
	pui32CtrlStream = (IMG_PUINT32)uiTemp;

	/* This looks like some questionable arithmetic */
	psRTDataSet->sBRN23862FixObjectDevAddr.uiAddr = ui32VertexBlockDevAddr -
													ui32ParamBase +
													(IMG_UINT32)((IMG_PCHAR)pui32CtrlStream -
																 (IMG_PCHAR)pui32VertexBlockStart);

	/* Control Stream: Primitive Block Header Type */
	*pui32CtrlStream++ = EURASIA_PARAM_OBJTYPE_PRIMBLOCK |
						(4UL - 1) << EURASIA_PARAM_PB0_VERTEXCOUNT_SHIFT |
						1UL << EURASIA_PARAM_PB0_ISPSTATESIZE_SHIFT |
						EURASIA_PARAM_PB0_READISPSTATE |
						0xfUL << EURASIA_PARAM_PB0_MASKBYTE1_SHIFT |
						0x3UL << EURASIA_PARAM_PB0_MASKBYTE0_SHIFT;

	/*
	 * Primitive Block Pointer
	 * NOTE: PF_VERTEXBLOCK_PTR points to the ISP state associated with the
	 * primitive block.
	 */
	ui32VertexPtr = ui32VertexBlockDevAddr + ui32ISPStateOffset - ui32ParamBase;
	*pui32CtrlStream++ = (((2UL) - 1) << EURASIA_PARAM_PB1_PRIMCOUNT_SHIFT) |
						 ((ui32VertexPtr >> EURASIA_PARAM_PB1_VERTEXPTR_ALIGNSHIFT)
						 	<< EURASIA_PARAM_PB1_VERTEXPTR_SHIFT);

	/* Stream Terminate */
	*pui32CtrlStream++ = EURASIA_PARAM_OBJTYPE_STREAMTERM;	/* PRQA S 3199 */ /* but it is */
}
#endif /* defined(FIX_HW_BRN_23862) */


/*!
 * *****************************************************************************
 * @brief Creates a skeleton background object to be used in multi-render
 * 		  scenes (e.g. SPM mode). Needs stuff we don't know now filled in
 * 		  later, stride, tex format, etc.
 *
 * @param psPBDesc
 * @param psRTDataSet
 *
 * @return
 ********************************************************************************/
static IMG_VOID
SetupBackgroundObj(SGX_PBDESC *psPBDesc,
				   SGX_RTDATASET *psRTDataSet)
{
	IMG_UINT32		ui32ParamBase;
	IMG_UINT32		ui32Width;
	IMG_UINT32		ui32Height;
	IMG_UINT32		ui32Depth;
	IMG_UINT32		ui32RTDataSel;
	IMG_UINT32		ui32VertexBlockAddr;
	IMG_PUINT32 	pui32VertexBlock;
	IMG_UINT32		ui32BgObjTag;
	IMG_UINT32	ui32Left, ui32Right, ui32Top, ui32Bottom;

	/* Get the parameter base address */
	ui32ParamBase	= psPBDesc->sParamHeapBaseDevVAddr.uiAddr;

	/* Calculate the render size */
	ui32Width		= psRTDataSet->ui32NumPixelsX;
	ui32Height		= psRTDataSet->ui32NumPixelsY;

	/* Calculate the depth value */
	ui32Depth		= FLOAT32_ONE;

	ui32VertexBlockAddr	= (psRTDataSet->psSpecialObjClientMemInfo->sDevVAddr.uiAddr - ui32ParamBase)
							>> EUR_CR_ISP_BGOBJTAG_VERTEXPTR_ALIGNSHIFT;
	ui32BgObjTag		= ((ui32VertexBlockAddr << EUR_CR_ISP_BGOBJTAG_VERTEXPTR_SHIFT)
							& EUR_CR_ISP_BGOBJTAG_VERTEXPTR_MASK) |
							((2UL << EUR_CR_ISP_BGOBJTAG_TSPDATASIZE_SHIFT) &
							EUR_CR_ISP_BGOBJTAG_TSPDATASIZE_MASK);

	/* Get a pointer to the vertex block */
	pui32VertexBlock = (IMG_UINT32*)psRTDataSet->psSpecialObjClientMemInfo->pvLinAddr;

	/*
		The PDS pixel state needs to be setup according to
		the render target format and stride
	*/
	*pui32VertexBlock++	= 0;
	*pui32VertexBlock++	= 0;
	*pui32VertexBlock++	= 0;

	/* TSP vertex data (U, V) */
	switch(psRTDataSet->eRotation)
	{
		default:
		case PVRSRV_ROTATE_0:
		case PVRSRV_FLIP_Y:
		{
			*pui32VertexBlock++	= 0;
			*pui32VertexBlock++	= 0;

			*pui32VertexBlock++	= psRTDataSet->ui32BGObjUCoord;
			*pui32VertexBlock++	= 0;

			*pui32VertexBlock++	= 0;
			*pui32VertexBlock++	= FLOAT32_ONE;
			break;
		}
		case PVRSRV_ROTATE_90:
		{
			*pui32VertexBlock++	= FLOAT32_ONE;
			*pui32VertexBlock++	= 0;

			*pui32VertexBlock++	= FLOAT32_ONE;
			*pui32VertexBlock++	= FLOAT32_ONE;

			*pui32VertexBlock++	= 0;
			*pui32VertexBlock++	= 0;
			break;
		}
		case PVRSRV_ROTATE_180:
		{
			*pui32VertexBlock++	= FLOAT32_ONE;
			*pui32VertexBlock++	= FLOAT32_ONE;

			*pui32VertexBlock++	= 0;
			*pui32VertexBlock++	= FLOAT32_ONE;

			*pui32VertexBlock++	= FLOAT32_ONE;
			*pui32VertexBlock++	= 0;
			break;
		}
		case PVRSRV_ROTATE_270:
		{
			*pui32VertexBlock++	= 0;
			*pui32VertexBlock++	= FLOAT32_ONE;

			*pui32VertexBlock++	= 0;
			*pui32VertexBlock++	= 0;

			*pui32VertexBlock++	= FLOAT32_ONE;
			*pui32VertexBlock++	= FLOAT32_ONE;

			break;
		}
	}


#if defined(SGX545)
	/* Vertex format word */
	*pui32VertexBlock++	= 0;

	/* TSP data format word */
	*pui32VertexBlock++	= (2UL << EURASIA_PARAM_VF_TSP_SIZE_SHIFT) |
							(2UL << EURASIA_PARAM_VF_TSP_CLIP_SIZE_SHIFT);

	/* Texcoord16 format word */
	*pui32VertexBlock++	= 0;
	/* Texcoord1D format word */
	*pui32VertexBlock++	= 0;
	/* Texcoord S present word */
	*pui32VertexBlock++	= 0;
	/* Texcoord T present word */
	*pui32VertexBlock++	= 0;
#else /* #if defined(SGX545) */
	/* Vertex format word */
	*pui32VertexBlock++	= 2UL << EURASIA_PARAM_VF_TSP_SIZE_SHIFT;

	/* TSP data format word */
	*pui32VertexBlock++	= 0;
#endif /* #if defined(SGX545) */

	/* TSP vertex data (X, Y, Z) */
	ui32Left = (0 + EURASIA_PARAM_VF_X_OFFSET);
	ui32Left <<= EURASIA_PARAM_VF_X_FRAC;

	ui32Top = (0 + EURASIA_PARAM_VF_Y_OFFSET);
	ui32Top <<= EURASIA_PARAM_VF_Y_FRAC;

	ui32Right = (ui32Width + EURASIA_PARAM_VF_X_OFFSET);
	ui32Right <<= EURASIA_PARAM_VF_X_FRAC;

	ui32Bottom = (ui32Height + EURASIA_PARAM_VF_Y_OFFSET);
	ui32Bottom <<= EURASIA_PARAM_VF_Y_FRAC;

	/* Apply half pixel offset {-0.5, -0.5} if not an OpenGL render target */
	if((psRTDataSet->ui32Flags & SGX_RTDSFLAGS_OPENGL) == 0)
	{
		ui32Left   -= (1UL << (EURASIA_PARAM_VF_X_FRAC - 1));
		ui32Top	   -= (1UL << (EURASIA_PARAM_VF_Y_FRAC - 1));
		ui32Right  -= (1UL << (EURASIA_PARAM_VF_X_FRAC - 1));
		ui32Bottom -= (1UL << (EURASIA_PARAM_VF_Y_FRAC - 1));
	}

#if defined(SGX545)
	// Note: 7.5 dwords are needed for the position data.
	*pui32VertexBlock  = (((ui32Top) << EURASIA_PARAM_VF_Y_EVEN0_SHIFT)
							& ~EURASIA_PARAM_VF_Y_EVEN0_CLRMSK);
	*pui32VertexBlock |= (((ui32Left) << EURASIA_PARAM_VF_X_EVEN0_SHIFT)
							& ~EURASIA_PARAM_VF_X_EVEN0_CLRMSK);
	pui32VertexBlock++;
	*pui32VertexBlock  = (((ui32Left >> EURASIA_PARAM_VF_X_EVEN1_ALIGNSHIFT)
							 << EURASIA_PARAM_VF_X_EVEN1_SHIFT)
							 & ~EURASIA_PARAM_VF_X_EVEN1_CLRMSK);
	*pui32VertexBlock |= (((ui32Depth) << EURASIA_PARAM_VF_Z_EVEN1_SHIFT)
							& ~EURASIA_PARAM_VF_Z_EVEN1_CLRMSK);
	pui32VertexBlock++;
	*pui32VertexBlock  = (((ui32Depth >> EURASIA_PARAM_VF_Z_EVEN2_ALIGNSHIFT)
							 << EURASIA_PARAM_VF_Z_EVEN2_SHIFT)
							 & ~EURASIA_PARAM_VF_Z_EVEN2_CLRMSK);

	*pui32VertexBlock |= (((ui32Top) << EURASIA_PARAM_VF_Y_ODD0_SHIFT)
							& ~EURASIA_PARAM_VF_Y_ODD0_CLRMSK);
	pui32VertexBlock++;
	*pui32VertexBlock  = (((ui32Top >> EURASIA_PARAM_VF_Y_ODD1_ALIGNSHIFT)
							<< EURASIA_PARAM_VF_Y_ODD1_SHIFT)
							& ~EURASIA_PARAM_VF_Y_ODD1_CLRMSK);
	*pui32VertexBlock |= (((ui32Right) << EURASIA_PARAM_VF_X_ODD1_SHIFT)
							& ~EURASIA_PARAM_VF_X_ODD1_CLRMSK);
	pui32VertexBlock++;
	*pui32VertexBlock  = (((ui32Depth) << EURASIA_PARAM_VF_Z_ODD2_SHIFT)
							& ~EURASIA_PARAM_VF_Z_ODD2_CLRMSK);
	pui32VertexBlock++;

	*pui32VertexBlock  = (((ui32Bottom) << EURASIA_PARAM_VF_Y_EVEN0_SHIFT)
							& ~EURASIA_PARAM_VF_Y_EVEN0_CLRMSK);
	*pui32VertexBlock |= (((ui32Left) << EURASIA_PARAM_VF_X_EVEN0_SHIFT)
							& ~EURASIA_PARAM_VF_X_EVEN0_CLRMSK);
	pui32VertexBlock++;
	*pui32VertexBlock  = (((ui32Left >> EURASIA_PARAM_VF_X_EVEN1_ALIGNSHIFT)
							<< EURASIA_PARAM_VF_X_EVEN1_SHIFT)
							& ~EURASIA_PARAM_VF_X_EVEN1_CLRMSK);
	*pui32VertexBlock |= (((ui32Depth) << EURASIA_PARAM_VF_Z_EVEN1_SHIFT)
							& ~EURASIA_PARAM_VF_Z_EVEN1_CLRMSK);
	pui32VertexBlock++;
	*pui32VertexBlock  = (((ui32Depth >> EURASIA_PARAM_VF_Z_EVEN2_ALIGNSHIFT)
							<< EURASIA_PARAM_VF_Z_EVEN2_SHIFT)
							& ~EURASIA_PARAM_VF_Z_EVEN2_CLRMSK);
	pui32VertexBlock++;
#else /* #if defined(SGX545) */
	*pui32VertexBlock++	= (ui32Left << EURASIA_PARAM_VF_X_SHIFT) |
							(ui32Top << EURASIA_PARAM_VF_Y_SHIFT);
	*pui32VertexBlock++	= ui32Depth;
	*pui32VertexBlock++	= (ui32Right << EURASIA_PARAM_VF_X_SHIFT) |
							(ui32Top << EURASIA_PARAM_VF_Y_SHIFT);
	*pui32VertexBlock++	= ui32Depth;
	*pui32VertexBlock++	= (ui32Left << EURASIA_PARAM_VF_X_SHIFT) |
							(ui32Bottom << EURASIA_PARAM_VF_Y_SHIFT);
	*pui32VertexBlock++	= ui32Depth;		/* PRQA S 3199 */ /* It IS used */
#endif /* #if defined(SGX545) */

#if defined(FIX_HW_BRN_23862)
	SetupBRN23862FixObject(psRTDataSet, pui32VertexBlock, ui32ParamBase);
#endif /* defined(FIX_HW_BRN_23862) */

	for (ui32RTDataSel = 0; ui32RTDataSel < psRTDataSet->ui32NumRTData; ui32RTDataSel++)
	{
		/* Setup register value */
		psRTDataSet->psRTData[ui32RTDataSel].ui32BGObjPtr1 = ui32BgObjTag;

#if defined(FIX_HW_BRN_23862)
		/* The PDS state is at the start of the vertex block. */
		psRTDataSet->psHWRTDataSet->asHWRTData[ui32RTDataSel].sBRN23862PDSStateDevAddr.uiAddr =
						psRTDataSet->sBRN23862PDSStateDevAddr.uiAddr;

		/* This looks like some questionable arithmetic */
		psRTDataSet->psHWRTDataSet->asHWRTData[ui32RTDataSel].sBRN23862FixObjectDevAddr.uiAddr =
						psRTDataSet->sBRN23862FixObjectDevAddr.uiAddr;
#endif
	}
}

#if defined(FIX_HW_BRN_32044)

IMG_INTERNAL IMG_VOID
write_ISP_vtx_XY(IMG_UINT32 *buf, IMG_INT32 x, IMG_INT32 y);

IMG_INTERNAL IMG_VOID
write_ISP_vtx_XY(IMG_UINT32 *buf, IMG_INT32 x, IMG_INT32 y)
{
	IMG_UINT32 xconv;
	IMG_UINT32 yconv;

	xconv = (x + EURASIA_PARAM_VF_X_OFFSET) << EURASIA_PARAM_VF_X_FRAC;
	yconv = (y + EURASIA_PARAM_VF_Y_OFFSET) << EURASIA_PARAM_VF_Y_FRAC;

#if defined(SGX545)
	*buf = (xconv << EURASIA_PARAM_VF_X_EVEN0_SHIFT)
	     | ((yconv << EURASIA_PARAM_VF_Y_EVEN0_SHIFT) & 0x00FFFFFF);
#else
	*buf = (xconv << EURASIA_PARAM_VF_X_SHIFT)
	     | (yconv << EURASIA_PARAM_VF_Y_SHIFT);
#endif
}

IMG_INTERNAL IMG_VOID
write_ISP_vtx_Z(IMG_UINT32 *buf, IMG_FLOAT zfz);

IMG_INTERNAL IMG_VOID
write_ISP_vtx_Z(IMG_UINT32 *buf, IMG_FLOAT zf)
{
	*buf = *((IMG_UINT32 *) &zf);
}

#define CUSTOM_PRIM_BLK_LENBYTES(NUMREGIONS) ((16*sizeof(IMG_UINT32))*( (NUMREGIONS) + 1))

IMG_INTERNAL IMG_UINT32
init32044_ExtraCSArray(PVRSRV_CLIENT_MEM_INFO *psCtlStrmClientMemInfo,
                       IMG_UINT32 ui32NumRegions);

/* returns the byte offset of the control stream array relative to the buffer base */
IMG_INTERNAL IMG_UINT32
init32044_ExtraCSArray(PVRSRV_CLIENT_MEM_INFO *psCtlStrmClientMemInfo,
                       IMG_UINT32 ui32NumRegions)
{
	IMG_UINT32 ui32ISPAOffset, ui32VtxDevAddr, ui32CSOffset;
	IMG_UINT32 *pui32cursor;
	IMG_UINT32 ui32Region;

	pui32cursor = psCtlStrmClientMemInfo->pvLinAddr;
	PVRSRVMemSet(pui32cursor, 0, psCtlStrmClientMemInfo->uAllocSize);

/*
Layout in memory:

                  +-- Offset in 32 bit words
                  |
                  V+-----------------+
Primitive Block   0| PDS_SEC         |    Secondary PDS State Pointer
                   | DMS_INFO        |    DMS Pixel Info Pointer
                   | PDS_PRIM        |    Primary PDS State Pointer
                   | (16 byte align) |
                  4| ISPA            |<-+ ISP State Word A (must be 16 byte aligned)
                   | IDXDATA         |  | Index Data
                   | VERTXFMT        |  | Vertex Format Word
                   | TSPDATAFMT      |  | TSP Data Format Wrod
                   | X0 | Y0         |  | Vertex Data
                   | Z0              |  | Vertex Data
                   | X1 | Y1         |  | Vertex Data
                   | Z1              |  | Vertex Data
                   | X2 | Y2         |  | Vertex Data
                   | Z2              |  | Vertex Data
                   | (64 byte align) |  |
                   +-----------------+  |
Control Stream   64| PIM             |  | Pipeline Interleave Marker (filled by MK)
Array              | PRIMBLK_HDR     |  | Primitive Block Header
(repeated          | VRTX_PTR ----------+ Primitive Block Pointer
ui32NumRegions     | LINK_PTR        |    Primitive Block Link Pointer (filled by MK)
times)             | (64 byte align  |
                   +-----------------+
                128|      .          |    NB: each CS element must be 64 byte aligned
                   |      .          |
                   |      .          |
                   |      .          |
*/

	/* PDS_SEC -- Secondary PDS State Pointer */
	*pui32cursor++ = ((0x0 << EURASIA_PDS_DATASIZE_SHIFT) & ~EURASIA_PDS_DATASIZE_CLRMSK)
	               | ((0x0 << EURASIA_PDS_BASEADD_SHIFT) & ~EURASIA_PDS_BASEADD_CLRMSK);

	/* DMS_INFO --  DMS Pixel Info Pointer */
	*pui32cursor++ = ((0x0 << EURASIA_PDS_PIXELSIZE_SHIFT) & ~EURASIA_PDS_PIXELSIZE_CLRMSK)
	               /* PDS_PARTIAL = 0 */
	               | ((0x1 << EURASIA_PDS_PDSTASKSIZE_SHIFT) & ~EURASIA_PDS_PDSTASKSIZE_CLRMSK)
	               | ((0x0 << EURASIA_PDS_SECATTRSIZE_SHIFT) & ~EURASIA_PDS_SECATTRSIZE_CLRMSK)
	               | ((0x0 << EURASIA_PDS_USETASKSIZE_SHIFT) & ~EURASIA_PDS_USETASKSIZE_CLRMSK)
	               /* PDS_BASE_ADD_SEC_MSB = 0 */
	               /* PDS_BASE_ADD_PRI_MSB = 0 */
	               /* PDS_BATCHNUM = 0 */ ;

	/* PDS_PRIM -- Primary PDS State Pointer */
	*pui32cursor++ = ((0x0 << EURASIA_PDS_DATASIZE_SHIFT) & ~EURASIA_PDS_DATASIZE_CLRMSK)
	               | ((0x0 << EURASIA_PDS_BASEADD_SHIFT) & ~EURASIA_PDS_BASEADD_CLRMSK);

	/* pad to 16 byte alignment */
//	*pui32cursor++ = 0;
	pui32cursor = (IMG_PUINT32)(((IMG_INT32) pui32cursor + 15) & ~ 15 );

	ui32ISPAOffset = pui32cursor - (IMG_UINT32 *) psCtlStrmClientMemInfo->pvLinAddr;
	ui32VtxDevAddr = ui32ISPAOffset * 4 + psCtlStrmClientMemInfo->sDevVAddr.uiAddr;

	/* ISPA -- ISP State Word A */
	*pui32cursor++ = EURASIA_ISPA_PASSTYPE_OPAQUE
                   | EURASIA_ISPA_DCMPMODE_NEVER
                   /* ISPA_TAGWRITEDIS = 0 */
                   /* ISPA_DWRITEDIS = 0*/
                   /* ISPA_LINEFILLLASTPIXEL = 0 */
                   | EURASIA_ISPA_OBJTYPE_TRI
                   /* ISPA_2SIDED = 0 */
		   /* ISPA_BFNFF_HWONLY = 0 */
                   /* ISPA_BPRES = 0 */
                   /* ISPA_CPRES = 0 */
                   | ((0x0 << EURASIA_ISPA_SREF_SHIFT) & ~EURASIA_ISPA_SREF_CLRMSK);

	/* IDXDATA -- Index Data */
	*pui32cursor++ = /* Backface Flag1 = 0*/
	               /* PF_EDGE_FLAG1_CA = 0 */
	               ((0x0 << EURASIA_PARAM_INDEX1C_SHIFT) & ~EURASIA_PARAM_INDEX1C_CLRMSK)
	               /* PF_EDGE_FLAG1_BC = 0 */
	               | ((0x0 << EURASIA_PARAM_INDEX1B_SHIFT) & ~EURASIA_PARAM_INDEX1B_CLRMSK)
	               /* PF_EDGE_FLAG1_AB = 0 */
	               | ((0x0 << EURASIA_PARAM_INDEX1A_SHIFT) & ~EURASIA_PARAM_INDEX1A_CLRMSK)
	               /* Backface Flag0 = 0 */
	               /* PF_EDGE_FLAG0_CA = 0 */
	               | ((0x2 << EURASIA_PARAM_INDEX0C_SHIFT) & ~EURASIA_PARAM_INDEX0C_CLRMSK)
	               /* PF_EDGE_FLAG0_BC = 0 */
	               | ((0x1 << EURASIA_PARAM_INDEX0B_SHIFT) & ~EURASIA_PARAM_INDEX0B_CLRMSK)
	               /* PF_EDGE_FLAG0_AB = 0 */
	               | ((0x0 << EURASIA_PARAM_INDEX0A_SHIFT) & ~EURASIA_PARAM_INDEX0A_CLRMSK);

	/* VERTXFMT -- Vertex Format Word 
	*pui32cursor++ = ((0x0 & EURASIA_PARAM_VF_CLIP_MASK_CLRMSK) << EURASIA_PARAM_VF_CLIP_MASK_SHIFT)
	               | ((0x0 & EURASIA_PARAM_VF_PITCHF_CLRMSK) << EURASIA_PARAM_VF_PITCHF_SHIFT)
	               | ((0x0 & EURASIA_PARAM_VF_PITCHB_CLRMSK) << EURASIA_PARAM_VF_PITCHB_SHIFT)
	                VF_FOG_PRESENT = 0 
	                VF_WPRESENT = 0 
	               | ((0x0 & EURASIA_PARAM_VF_TSP_SIZE_CLRMSK) << EURASIA_PARAM_VF_TSP_SIZE_SHIFT); */
	*pui32cursor++ = 0;

	/* TSPDATAFMT -- TSP Data Format Word
	   EURASIA_PARAM_VF_OFFSET_PRESENT = 0
	   EURASIA_PARAM_VF_BASE_PRESENT = 0
	   EURASIA_PARAM_VF_TC9_16B = 0
	   EURASIA_PARAM_VF_TC8_16B = 0
	   EURASIA_PARAM_VF_TC7_16B = 0
	   EURASIA_PARAM_VF_TC6_16B = 0
	   EURASIA_PARAM_VF_TC5_16B = 0
	   EURASIA_PARAM_VF_TC4_16B = 0
	   EURASIA_PARAM_VF_TC3_16B = 0
	   EURASIA_PARAM_VF_TC2_16B = 0
	   EURASIA_PARAM_VF_TC1_16B = 0
	   EURASIA_PARAM_VF_TC0_16B = 0
	   EURASIA_PARAM_VF_TC9_T_PRES = 0
	   EURASIA_PARAM_VF_TC9_S_PRES = 0
	   EURASIA_PARAM_VF_TC8_T_PRES = 0
	   EURASIA_PARAM_VF_TC8_S_PRES = 0
	   EURASIA_PARAM_VF_TC7_T_PRES = 0
	   EURASIA_PARAM_VF_TC7_S_PRES = 0
	   EURASIA_PARAM_VF_TC6_T_PRES = 0
	   EURASIA_PARAM_VF_TC6_S_PRES = 0
	   EURASIA_PARAM_VF_TC5_T_PRES = 0
	   EURASIA_PARAM_VF_TC5_S_PRES = 0
	   EURASIA_PARAM_VF_TC4_T_PRES = 0
	   EURASIA_PARAM_VF_TC4_S_PRES = 0
	   EURASIA_PARAM_VF_TC3_T_PRES = 0
	   EURASIA_PARAM_VF_TC3_S_PRES = 0
	   EURASIA_PARAM_VF_TC2_T_PRES = 0
	   EURASIA_PARAM_VF_TC2_S_PRES = 0
	   EURASIA_PARAM_VF_TC1_T_PRES = 0
	   EURASIA_PARAM_VF_TC1_S_PRES = 0
	   EURASIA_PARAM_VF_TC0_T_PRES = 0
	   EURASIA_PARAM_VF_TC0_S_PRES = 0 */
	*pui32cursor++ = 0x0;

	/* vertices: triangle bounded by (-2, -2) to (-1, -1) */
	write_ISP_vtx_XY(pui32cursor++, -1, -1);
	write_ISP_vtx_Z(pui32cursor++, 0);
	write_ISP_vtx_XY(pui32cursor++, -2, -1);
	write_ISP_vtx_Z(pui32cursor++, 0);
	write_ISP_vtx_XY(pui32cursor++, -1, -2);
	write_ISP_vtx_Z(pui32cursor++, 0);

	/* pad to 64 byte alignment, i.e. 16 word alignment */
	pui32cursor = (IMG_PUINT32)(((IMG_UINT32) pui32cursor + 63) & ~ 63);

	/* The control streams */
	ui32CSOffset = (pui32cursor - (IMG_UINT32 *) psCtlStrmClientMemInfo->pvLinAddr) * sizeof(IMG_UINT32);/* needed later */
	for(ui32Region = 0; ui32Region < ui32NumRegions; ui32Region++)
	{
		/* PIM -- Pipeline Interleave Marker (Filled in by MK) */
		*pui32cursor++ = 0x0; 

		/* PRIMBLK_HDR -- Primitive block header */
		*pui32cursor++ = EURASIA_PARAM_OBJTYPE_PRIMBLOCK
						/* PF_PP_WORDS = 0 */
		               | ((0x2 << EURASIA_PARAM_PB0_VERTEXCOUNT_SHIFT) & ~EURASIA_PARAM_PB0_VERTEXCOUNT_CLRMSK)
		               | ((0x1 << EURASIA_PARAM_PB0_ISPSTATESIZE_SHIFT) & ~EURASIA_PARAM_PB0_ISPSTATESIZE_CLRMSK)
		               /* PF_LINK_VERT_PTR = 0 */
		               /* PF_PRIM_MASK_PRES = 0 */
		               /* PF_FULL_MASK = 0 */
		               | ((0x0 << EURASIA_PARAM_PB0_PRMSTART_SHIFT) & ~EURASIA_PARAM_PB0_PRMSTART_CLRMSK)
		               /* PF_VTM_START = 0 */
		               | ((0x0 <<  EURASIA_PARAM_PB0_MASKBYTE1_SHIFT) & ~EURASIA_PARAM_PB0_MASKBYTE1_CLRMSK)
		               | ((0x1 <<  EURASIA_PARAM_PB0_MASKBYTE0_SHIFT) & ~EURASIA_PARAM_PB0_MASKBYTE0_CLRMSK);

		/* VRTX_PTR -- Primitive Block Pointer */
		*pui32cursor++ = ((0x0 << EURASIA_PARAM_PB1_PRIMCOUNT_SHIFT) & ~EURASIA_PARAM_PB1_PRIMCOUNT_CLRMSK)
		               /* PF_READ_STATE = 0 */
		               | EURASIA_PARAM_PB1_READISPSTATE
		               | (((ui32VtxDevAddr >> EURASIA_PARAM_PB1_VERTEXPTR_ALIGNSHIFT) << EURASIA_PARAM_PB1_VERTEXPTR_SHIFT) & ~EURASIA_PARAM_PB1_VERTEXPTR_CLRMSK);

		/* LINK_PTR */
		*pui32cursor++ = ((0x0 << EURASIA_PARAM_PB3_LINKVERTPTR_SHIFT) & ~EURASIA_PARAM_PB3_LINKVERTPTR_CLRMSK);

		/* pad to 64 byte alignment, i.e. 16 word alignment */
		pui32cursor = (IMG_PUINT32)(((IMG_UINT32) pui32cursor + 63) & ~ 63);
	}

	return ui32CSOffset;
}
#endif // FIX_HW_BRN_32044


/*!
 * *****************************************************************************
 * @brief Adds/Sets up HW/Core1 structures for render target
 *
 * @param psDevData
 * @param psRenderContext
 * @param psRTDataSet
 *
 * @return
 ********************************************************************************/
static PVRSRV_ERROR
SetupRTDataSet(const PVRSRV_DEV_DATA *psDevData,
			   SGX_RENDERCONTEXT *psRenderContext,
			   SGX_RTDATASET *psRTDataSet)
{
	PVRSRV_ERROR				eError;
	PSGX_RENDERDETAILS		psRenderDetails;
	PSGX_DEVICE_SYNC_LIST		psDeviceSyncList;
	IMG_UINT32					n;
	PSGXMKIF_HWRTDATASET		psHWRTDataSet;
	IMG_UINT32					ui32TilesPerMTileX;
	IMG_UINT32					ui32TilesPerMTileY;
	IMG_UINT32					ui32MTilesX;
	IMG_UINT32					ui32MTilesY;
#if defined(FIX_HW_BRN_32044)
	IMG_UINT32					ui32NumTiles;
	IMG_UINT32					ui32ExtraCSOff;
#endif
#if defined(FIX_HW_BRN_32044) || (!defined(SGX_FEATURE_ISP_BREAKPOINT_RESUME_REV_2) && !defined(FIX_HW_BRN_23761) && !defined(SGX_FEATURE_VISTEST_IN_MEMORY))
	IMG_UINT32					ui32TileRgnLUTSize;				/* PRQA S 3203 */ /* but it is used */
#endif
	IMG_UINT32					ui32MacroTileRgnLUTSize;		/* PRQA S 3203 */ /* but it is used */
	IMG_UINT32					ui32RgnSize;
	IMG_UINT32					ui32TailSize;
	IMG_UINT32					i;
	IMG_UINT32					ui32HeapCount;
	IMG_UINT16					ui16NumRTsInArray = 1;
	PVRSRV_HEAP_INFO    		asHeapInfo[PVRSRV_MAX_CLIENT_HEAPS];
	PVRSRV_HEAP_INFO			*ps3DParamsHeapInfo = IMG_NULL;
	PVRSRV_HEAP_INFO			*psKernelVideoDataHeapInfo = IMG_NULL;
	PVRSRV_HEAP_INFO			*psSyncInfoHeapInfo = IMG_NULL;
	PVRSRV_HEAP_INFO			*psTADataHeapInfo = IMG_NULL;
	PVRSRV_HEAP_INFO			*psPBHeapInfo = IMG_NULL;	/* PRQA S 3197 */ /* init anyway */
#if defined(DEBUG)
	PVRSRV_HEAP_INFO			*psTPHeapInfo = IMG_NULL;	/* PRQA S 3197 */ /* init anyway */
#endif
#if defined(SGX_MAX_QUEUED_RENDERS)
	const IMG_UINT32			ui32MaxQueuedRenders = SGX_MAX_QUEUED_RENDERS;
#else
	/* Max Queued Renders is (ui32RendersPerFrame * 3) */
	const IMG_UINT32			ui32MaxQueuedRenders = 3 * (psRTDataSet->ui32NumRTData/2);
#endif
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	IMG_UINT32		ui32OTPMSize;
	IMG_UINT32		ui32DPMControlSize;
	IMG_UINT32		ui32DPMStateSize;

	ui16NumRTsInArray = psRTDataSet->ui16NumRTsInArray;
	ui32OTPMSize = EURASIA_PARAM_MANAGE_OTPM_SIZE *  ui16NumRTsInArray;
	ui32DPMControlSize = EURASIA_PARAM_MANAGE_CONTROL_SIZE * ui16NumRTsInArray;
	ui32DPMStateSize = EURASIA_PARAM_MANAGE_STATE_SIZE * ui16NumRTsInArray;
#endif

	if(PVRSRVGetDeviceMemHeapInfo(psDevData,
									psRenderContext->hDevMemContext,
									&ui32HeapCount,
									asHeapInfo) !=  PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SetupRTDataSet: Failed to retrieve device "
				 "memory context information\n"));
		return PVRSRV_ERROR_FAILED_TO_RETRIEVE_HEAPINFO;
	}

	for(i=0; i<ui32HeapCount; i++)
	{
		switch(HEAP_IDX(asHeapInfo[i].ui32HeapID))
		{
			case SGX_GENERAL_HEAP_ID:
				break;
			case SGX_SHARED_3DPARAMETERS_HEAP_ID:
				if (!psRenderContext->bPerContextPB)
					ps3DParamsHeapInfo = &asHeapInfo[i];
				break;
			case SGX_PERCONTEXT_3DPARAMETERS_HEAP_ID:
				if (psRenderContext->bPerContextPB)
					ps3DParamsHeapInfo = &asHeapInfo[i];
				break;
			case SGX_KERNEL_DATA_HEAP_ID:
				psKernelVideoDataHeapInfo = &asHeapInfo[i];
				break;
			case SGX_SYNCINFO_HEAP_ID:
				psSyncInfoHeapInfo = &asHeapInfo[i];
				break;
			case SGX_TADATA_HEAP_ID:
				psTADataHeapInfo = &asHeapInfo[i];
				break;
			default:
				break;
		}
	}

	if ((psSyncInfoHeapInfo == IMG_NULL) || (psKernelVideoDataHeapInfo == IMG_NULL))
	{
		PVR_DPF((PVR_DBG_ERROR, "SetupRTDataSet: Failed to initialize pointer"));
		PVR_DBG_BREAK;
		return PVRSRV_ERROR_INVALID_HEAPINFO;
	}

	if (psTADataHeapInfo == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "SetupRTDataSet: Failed to initialize psTADataHeapInfo"));
		PVR_DBG_BREAK;
		return PVRSRV_ERROR_INVALID_HEAPINFO;
	}

	psPBHeapInfo = ps3DParamsHeapInfo;
#if defined(DEBUG)
	psTPHeapInfo = psTADataHeapInfo;

	PVR_ASSERT(psPBHeapInfo != IMG_NULL);
	PVR_ASSERT(psTPHeapInfo != IMG_NULL);	/* PRQA S 3355,3358 */ /* QAC says it always true */

#endif /* DEBUG */

	/* Allocate memory for render details structs */
	for (n = 0; n < ui32MaxQueuedRenders; n++)
	{
		if (!CreateRenderDetails(psDevData,
								 &psRenderDetails,
								 psKernelVideoDataHeapInfo->hDevMemHeap,
								 psSyncInfoHeapInfo->hDevMemHeap,
								 ui16NumRTsInArray))
		{
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		}

		/* Add new obj to the list */
		psRenderDetails->psNext = psRTDataSet->psRenderDetailsList;
		psRTDataSet->psRenderDetailsList = psRenderDetails;

		if (!CreateDeviceSyncList(psDevData,
								  &psDeviceSyncList,
								  psKernelVideoDataHeapInfo->hDevMemHeap,
								  psSyncInfoHeapInfo->hDevMemHeap,
								  ui16NumRTsInArray))
		{
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		}

		/* Add new obj to the list */
		psDeviceSyncList->psNext = psRTDataSet->psDeviceSyncList;
		psRTDataSet->psDeviceSyncList = psDeviceSyncList;
	}

	/*
	 * Allocate an HW TS data set.
	 */
	PDUMPCOMMENT(psDevData->psConnection,
				 "Allocate RT dataset pending counter",
				 IMG_TRUE);
	eError = PVRSRVAllocDeviceMem(psDevData,
								  psKernelVideoDataHeapInfo->hDevMemHeap,
								  PVRSRV_MEM_READ | PVRSRV_MEM_WRITE |
								  PVRSRV_MEM_NO_SYNCOBJ | PVRSRV_MEM_EDM_PROTECT |
								  PVRSRV_MEM_CACHE_CONSISTENT,
								  sizeof(*psRTDataSet->pui32PendingCount),
								  32,
								  &psRTDataSet->psPendingCountClientMemInfo);
	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupRTDataSet: Failed to alloc RT dataset pending counter"));
		return eError;
	}

	PDUMPCOMMENT(psDevData->psConnection,
			 "Allocate HW RT dataset (SGXMKIF_HWRTDATASET)",
			 IMG_TRUE);

	/* Overallocate due to variable ui32NumRTData */
	eError = PVRSRVAllocDeviceMem(psDevData,
								  psKernelVideoDataHeapInfo->hDevMemHeap,
								  PVRSRV_MEM_READ | PVRSRV_MEM_WRITE |
								  PVRSRV_MEM_NO_SYNCOBJ | PVRSRV_MEM_EDM_PROTECT |
								  PVRSRV_MEM_CACHE_CONSISTENT,
								  sizeof(SGXMKIF_HWRTDATASET) + (psRTDataSet->ui32NumRTData - 1) * sizeof(SGXMKIF_HWRTDATA),
								  32,
								  &psRTDataSet->psHWRTDataSetClientMemInfo);
	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupRTDataSet: Failed to alloc HW RTDataSet"));
		return eError;
	}

	/* clear memory to zero for pointer checking */
	PVRSRVMemSet(psRTDataSet->psHWRTDataSetClientMemInfo->pvLinAddr,
					0, psRTDataSet->psHWRTDataSetClientMemInfo->uAllocSize);

	psRTDataSet->psHWRTDataSet = psRTDataSet->psHWRTDataSetClientMemInfo->pvLinAddr;
	psRTDataSet->pui32PendingCount = psRTDataSet->psPendingCountClientMemInfo->pvLinAddr;
	psHWRTDataSet = psRTDataSet->psHWRTDataSet;
	psHWRTDataSet->ui32NumRTData = psRTDataSet->ui32NumRTData;

	if (psRTDataSet->ui32MTileNumber == 0)
	{
		ui32MTilesX = 2;
		ui32MTilesY = 2;
		ui32TilesPerMTileX = psRTDataSet->ui32MTileX2;
		ui32TilesPerMTileY = psRTDataSet->ui32MTileY2;
	}
	else
	{
		ui32MTilesX = 4;
		ui32MTilesY = 4;
		ui32TilesPerMTileX = psRTDataSet->ui32MTileX1;
		ui32TilesPerMTileY = psRTDataSet->ui32MTileY1;
	}

	/* Calculate the real number of tiles when multisampling */
	ui32TilesPerMTileX	*= psRTDataSet->ui16MSAASamplesInX;
	ui32TilesPerMTileY	*= psRTDataSet->ui16MSAASamplesInY;

	/* Initialise the pending and complete counts */
	*psRTDataSet->pui32PendingCount = 0;
	psHWRTDataSet->sPendingCountDevAddr = psRTDataSet->psPendingCountClientMemInfo->sDevVAddr;
	psHWRTDataSet->ui32CompleteCount = 0;
#if defined(PDUMP)
	psRTDataSet->ui32PDumpPendingCount = 0;
	PDUMPCOMMENT(psDevData->psConnection,
				 "Initialise RT dataset pending counter",
				 IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psRTDataSet->psPendingCountClientMemInfo,
			 0, (IMG_UINT32)psRTDataSet->psPendingCountClientMemInfo->uAllocSize, PDUMP_FLAGS_CONTINUOUS);
#endif /* PDUMP */

	/* Setup single instance data for HW */
	psHWRTDataSet->ui32MTileWidth =	ui32MTilesX * ui32TilesPerMTileX;
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	#if defined(FIX_HW_BRN_33657) && defined(SUPPORT_SECURE_33657_FIX)
	psHWRTDataSet->ui32MTileHeight = ui32MTilesY * ui32TilesPerMTileY;
	#endif
#endif

	/* Calculate space needed for region headers */
	ui32RgnSize = (ui32MTilesX * ui32TilesPerMTileX) *
					(ui32MTilesY * ui32TilesPerMTileY) *
					EURASIA_REGIONHEADER_SIZE;

#if (defined(FIX_HW_BRN_30089) || defined(FIX_HW_BRN_29504) || defined(FIX_HW_BRN_33753)) && defined(SGX_FEATURE_MP)
	{
		IMG_UINT32 ui32NumExtraRgns = 1;

#if defined(FIX_HW_BRN_30089)
		ui32NumExtraRgns += 3;
#endif

		/* We need to allocate an extra tiles worth of rgn headers */
		ui32RgnSize += ui32NumExtraRgns * EURASIA_REGIONHEADER_SIZE;
	}

	/* Round this size upto the the the nearest 16byte boundary */
	ui32RgnSize = (ui32RgnSize + ((1<<8) - 1)) & ~((1<<8) - 1);
#endif

#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	if (ui16NumRTsInArray > 1)
	{
		/* Round this size upto the nearest page */
		ui32RgnSize = (ui32RgnSize + (EURASIA_PARAM_MANAGE_GRAN - 1))
						& ~(EURASIA_PARAM_MANAGE_GRAN - 1);
		psRTDataSet->ui32RgnPageStride = ui32RgnSize / EURASIA_PARAM_MANAGE_GRAN;
		ui32RgnSize *= ui16NumRTsInArray;
	}
#endif

	/* Calculate space needed for tail pointers - which are twiddled */
	ui32TailSize = MAX(FindPowerOfTwo(ui32MTilesX * ui32TilesPerMTileX),
						FindPowerOfTwo(ui32MTilesY * ui32TilesPerMTileY));
	ui32TailSize *= ui32TailSize;
	ui32TailSize = (ui32TailSize * EURASIA_TAILPOINTER_SIZE);
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	if (ui16NumRTsInArray > 1)
	{
		/* Round this size upto the nearest page */
		ui32TailSize = (ui32TailSize + (EURASIA_PARAM_MANAGE_GRAN - 1))
						& ~(EURASIA_PARAM_MANAGE_GRAN - 1);
		psRTDataSet->ui32TPCStride = ui32TailSize / EURASIA_PARAM_MANAGE_GRAN;
		ui32TailSize *= ui16NumRTsInArray;
	}
#endif

	for(i=0; i<SGX_FEATURE_MP_CORE_COUNT_TA; i++)
	{
		/* Allocate the tails pointers */
		eError = PVRSRVAllocDeviceMem(psDevData,
									  psTADataHeapInfo->hDevMemHeap,
									  PVRSRV_MEM_READ|PVRSRV_MEM_WRITE|PVRSRV_MEM_NO_SYNCOBJ,
									  ui32TailSize,
									  EURASIA_PARAM_MANAGE_TPC_GRAN,
									  &psRTDataSet->apsTailPtrsClientMemInfo[i]);
		if(eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SetupRTDataSet: Failed to alloc Tail Pointers"));
			return eError;
		}
		/* Initialise tail ptr memory to 0 (needed by HW) */
		PVRSRVMemSet(psRTDataSet->apsTailPtrsClientMemInfo[i]->pvLinAddr, 0, ui32TailSize);
		PDUMPCOMMENT(psDevData->psConnection, "Initialise tail pointer memory", IMG_TRUE);
		PDUMPMEM(psDevData->psConnection, IMG_NULL,
					psRTDataSet->apsTailPtrsClientMemInfo[i], 0,
					ui32TailSize, PDUMP_FLAGS_CONTINUOUS);

		/* Allocate 'context control' */
		eError = PVRSRVAllocDeviceMem(psDevData,
									  psTADataHeapInfo->hDevMemHeap,
									  PVRSRV_MEM_READ|PVRSRV_MEM_WRITE|PVRSRV_MEM_NO_SYNCOBJ,
	#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
									  ui32DPMControlSize,
	#else
									  EURASIA_PARAM_MANAGE_CONTROL_SIZE,
	#endif
									  EURASIA_PARAM_MANAGE_CONTROL_GRAN,
									  &psRTDataSet->psContextControlClientMemInfo[i]);
		if(eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SetupRTDataSet: Failed to alloc Context Control"));
			return eError;
		}

		/* Memset to zero to support multi app, per-context PB initialisation on TAKick */
		PDUMPCOMMENT(psDevData->psConnection, "Initialise context control memory", IMG_TRUE);
	#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
		PVRSRVMemSet(psRTDataSet->psContextControlClientMemInfo[i]->pvLinAddr, 0,
					 ui32DPMControlSize);
		PDUMPMEM(psDevData->psConnection, IMG_NULL, psRTDataSet->psContextControlClientMemInfo[i],
					0, ui32DPMControlSize, PDUMP_FLAGS_CONTINUOUS);
	#else
		PVRSRVMemSet(psRTDataSet->psContextControlClientMemInfo[i]->pvLinAddr, 0,
					 EURASIA_PARAM_MANAGE_CONTROL_SIZE);
		PDUMPMEM(psDevData->psConnection, IMG_NULL, psRTDataSet->psContextControlClientMemInfo[i],
					0, EURASIA_PARAM_MANAGE_CONTROL_SIZE, PDUMP_FLAGS_CONTINUOUS);
	#endif

		/* Allocate 'context OTPM' */
		eError = PVRSRVAllocDeviceMem(psDevData,
									  psTADataHeapInfo->hDevMemHeap,
									  PVRSRV_MEM_READ|PVRSRV_MEM_WRITE|PVRSRV_MEM_NO_SYNCOBJ,
	#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
									  ui32OTPMSize,
	#else
									  EURASIA_PARAM_MANAGE_OTPM_SIZE,
	#endif
									  EURASIA_PARAM_MANAGE_OTPM_GRAN,
									  &psRTDataSet->psContextOTPMClientMemInfo[i]);
		if(eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SetupRTDataSet: Failed to alloc Context OTPM"));
			return eError;
		}

		/* memset to zero to support multi app, per-context PB initialisation on TAKick */
		PDUMPCOMMENT(psDevData->psConnection, "Initialise OTPM context memory", IMG_TRUE);
	#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
		PVRSRVMemSet(psRTDataSet->psContextOTPMClientMemInfo[i]->pvLinAddr, 0,
					 ui32OTPMSize);
		PDUMPMEM(psDevData->psConnection, IMG_NULL, psRTDataSet->psContextOTPMClientMemInfo[i],
					0, ui32OTPMSize, PDUMP_FLAGS_CONTINUOUS);
	#else
		PVRSRVMemSet(psRTDataSet->psContextOTPMClientMemInfo[i]->pvLinAddr, 0,
					 EURASIA_PARAM_MANAGE_OTPM_SIZE);
		PDUMPMEM(psDevData->psConnection, IMG_NULL, psRTDataSet->psContextOTPMClientMemInfo[i],
					0, EURASIA_PARAM_MANAGE_OTPM_SIZE, PDUMP_FLAGS_CONTINUOUS);
	#endif
	}

	/* Allocate the memory for Special Objects */
	PDUMPCOMMENT(psDevData->psConnection, "Allocate Special Objects", IMG_TRUE);
	eError = PVRSRVAllocDeviceMem(psDevData,
								  psPBHeapInfo->hDevMemHeap,		/* PRQA S 505 */ /* QAC says this could be NULL */
								  PVRSRV_MEM_READ | PVRSRV_MEM_WRITE| \
								  PVRSRV_MEM_EDM_PROTECT | PVRSRV_MEM_NO_SYNCOBJ,
								  SGX_SPECIALOBJECT_SIZE,
								  EURASIA_PARAM_MANAGE_GRAN,
								  &psRTDataSet->psSpecialObjClientMemInfo);
	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupRTDataSet: Failed to alloc Special Objects"));
		return eError;
	}

#if defined(SGX_FEATURE_MP)
#if defined(EUR_CR_MASTER_DPM_MTILE_PARTI_PIM_TABLE_BASE_ADDR)
	/* Allocate the memory for Special Objects */
	eError = PVRSRVAllocDeviceMem(psDevData,
								  psPBHeapInfo->hDevMemHeap,		/* PRQA S 505 */ /* QAC says this could be NULL */
								  PVRSRV_MEM_READ | PVRSRV_MEM_WRITE| PVRSRV_MEM_NO_SYNCOBJ,
								  EURASIA_PARAM_MANAGE_PIM_SIZE,
								  EURASIA_PARAM_MANAGE_PIM_GRAN,
								  &psRTDataSet->psPIMTableClientMemInfo);
	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupRTDataSet: Failed to alloc DPM PIM Table"));
		return eError;
	}
	PDUMPCOMMENT(psDevData->psConnection, "Initialise PIM Table memory", IMG_TRUE);
	PVRSRVMemSet(psRTDataSet->psPIMTableClientMemInfo->pvLinAddr, 0, EURASIA_PARAM_MANAGE_PIM_SIZE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psRTDataSet->psPIMTableClientMemInfo,
				0, EURASIA_PARAM_MANAGE_PIM_SIZE, PDUMP_FLAGS_CONTINUOUS);
#endif
#endif

#if defined(FIX_HW_BRN_32044) || (!defined(SGX_FEATURE_ISP_BREAKPOINT_RESUME_REV_2) && !defined(FIX_HW_BRN_23761) && !defined(SGX_FEATURE_VISTEST_IN_MEMORY))
	/* Calculate tile LUT sizes */
	ui32TileRgnLUTSize = ((ui32MTilesX * ui32TilesPerMTileX) * 					/* PRQA S 3199 */ /* but it is used */
						  (ui32MTilesY * ui32TilesPerMTileY)) * sizeof(IMG_UINT32);
#endif

	/* Calculate macrotile LUT sizes */
	ui32MacroTileRgnLUTSize = ((ui32MTilesX * ui32MTilesY) * sizeof(IMG_UINT32)) 	/* PRQA S 3199 */ /* but it is used */
								* SGX_FEATURE_MP_CORE_COUNT_TA;
	
#if defined(FIX_HW_BRN_32044)

	/* Calculate to tile and macrotile LUT sizes */
	ui32NumTiles = ui32TileRgnLUTSize/(sizeof(IMG_UINT32));
	

	/* alloc memory in the GENERAL heap to hold the control streams and vertex information */
	eError = PVRSRVAllocDeviceMem(psDevData,
								  ps3DParamsHeapInfo->hDevMemHeap,
								  PVRSRV_MEM_READ|PVRSRV_MEM_WRITE|PVRSRV_MEM_NO_SYNCOBJ,
								  CUSTOM_PRIM_BLK_LENBYTES(ui32NumTiles * SGX_FEATURE_MP_CORE_COUNT_TA),
								  0x1000,
								  &psRTDataSet->ps32044CtlStrmClientMemInfo);
	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupRTDataSet: Failed to alloc control stream patch array"));
		return eError;
	}

	/* Initialize control streams */
	ui32ExtraCSOff =
	  init32044_ExtraCSArray(psRTDataSet->ps32044CtlStrmClientMemInfo,
                             ui32NumTiles * SGX_FEATURE_MP_CORE_COUNT_TA);

#if defined(PDUMP)
	PDUMPCOMMENT(psDevData->psConnection, "32044 BRN WA stream", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL,
			psRTDataSet->ps32044CtlStrmClientMemInfo, 0,
			psRTDataSet->ps32044CtlStrmClientMemInfo->uAllocSize, PDUMP_FLAGS_CONTINUOUS);
#endif

	/* Make it so that the MK can access these fields... */
	psHWRTDataSet->s32044CtlStrmDevAddr.uiAddr =
	  psRTDataSet->ps32044CtlStrmClientMemInfo->sDevVAddr.uiAddr + ui32ExtraCSOff;
	psHWRTDataSet->ui32NumTiles = ui32NumTiles;

#endif /* end FIX_HW_BRN_32044 */


	/*
	 * Setup double buffered data structures
	 */
	for(i=0; i < psRTDataSet->ui32NumRTData; i++)
	{
		IMG_UINT32	j;
#if (defined(FIX_HW_BRN_27311) || defined(FIX_HW_BRN_30089) || defined(FIX_HW_BRN_29504) || defined(FIX_HW_BRN_33753)) && \
	!defined(FIX_HW_BRN_23055)
		psHWRTDataSet->asHWRTData[i].ui16NumMTs = (psRTDataSet->ui32MTileNumber == 0)? 4: 16;
	#if defined(FIX_HW_BRN_27311)
		psHWRTDataSet->asHWRTData[i].ui16BRN27311Mask = ((1 << psHWRTDataSet->asHWRTData[i].ui16NumMTs) - 1);
	#endif
#endif

		/* Duplicate unbuffered structures in RTData for benefit of USE asm */
		psHWRTDataSet->asHWRTData[i].sHWRenderContextDevAddr =
						psRenderContext->sHWRenderContextDevVAddr;

		//FIXME: migrate from hwrtdata to hwrtdataset
		for(j=0; j<SGX_FEATURE_MP_CORE_COUNT_TA; j++)
		{
			psHWRTDataSet->asHWRTData[i].asTailPtrDevAddr[j] =
										psRTDataSet->apsTailPtrsClientMemInfo[j]->sDevVAddr;
			psHWRTDataSet->asHWRTData[i].sContextControlDevAddr[j] =
										psRTDataSet->psContextControlClientMemInfo[j]->sDevVAddr;
			psHWRTDataSet->asHWRTData[i].sContextOTPMDevAddr[j] =
										psRTDataSet->psContextOTPMClientMemInfo[j]->sDevVAddr;
		}
		psHWRTDataSet->asHWRTData[i].ui32TailSize = ui32TailSize;
		psHWRTDataSet->asHWRTData[i].sBGObjBaseDevAddr =
									psRTDataSet->psSpecialObjClientMemInfo->sDevVAddr;
#if defined(SGX_FEATURE_MP)
#if defined(EUR_CR_MASTER_DPM_MTILE_PARTI_PIM_TABLE_BASE_ADDR)
		psHWRTDataSet->asHWRTData[i].sPIMTableDevAddr =
									psRTDataSet->psPIMTableClientMemInfo->sDevVAddr;
#endif
#endif
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
		psHWRTDataSet->asHWRTData[i].ui32RgnStride = psRTDataSet->ui32RgnPageStride *
														 EURASIA_PARAM_MANAGE_GRAN;
#endif
		/* Fill in the info used to workout 1st tile in MT */
		psHWRTDataSet->asHWRTData[i].ui32NumTileBlocksPerMT =
									(ui32TilesPerMTileX * ui32TilesPerMTileY) / 16;

		psHWRTDataSet->asHWRTData[i].ui32MTRegionArraySize =
									(ui32TilesPerMTileX * ui32TilesPerMTileY) * EURASIA_REGIONHEADER_SIZE;

#if (defined(FIX_HW_BRN_30089) || defined(FIX_HW_BRN_29504) || defined(FIX_HW_BRN_33753)) && defined(SGX_FEATURE_MP)
		psHWRTDataSet->asHWRTData[i].ui32MTRegionArraySize += EURASIA_REGIONHEADER_SIZE;
#endif

		/*
		 * Allocate region headers
		 */
		PDUMPCOMMENT(psDevData->psConnection, "Allocate Region headers", IMG_TRUE);
		for(j=0; j<SGX_FEATURE_MP_CORE_COUNT_TA; j++)
		{
			eError = PVRSRVAllocDeviceMem(psDevData,
										  psTADataHeapInfo->hDevMemHeap,
										  PVRSRV_MEM_READ|PVRSRV_MEM_WRITE|PVRSRV_MEM_NO_SYNCOBJ,
										  ui32RgnSize,
										  EURASIA_PARAM_MANAGE_REGION_GRAN,
										  &psRTDataSet->psRTData[i].apsRgnHeaderClientMemInfo[j]);
			if(eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SetupRTDataSet: Failed to alloc Region headers"));
				return eError;
			}

#if defined(FIX_HW_BRN_28475)
			PVRSRVMemSet(psRTDataSet->psRTData[i].apsRgnHeaderClientMemInfo[j]->pvLinAddr, 0, ui32RgnSize);
#endif
		}

		/*
		 * Allocate 'context state'
		 */
		PDUMPCOMMENT(psDevData->psConnection, "Allocate context state memory", IMG_TRUE);
		eError = PVRSRVAllocDeviceMem(psDevData,
									  psTADataHeapInfo->hDevMemHeap,
									  PVRSRV_MEM_READ|PVRSRV_MEM_WRITE|PVRSRV_MEM_NO_SYNCOBJ,
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
									  ui32DPMStateSize,
#else
									  EURASIA_PARAM_MANAGE_STATE_SIZE,
#endif
									  EURASIA_PARAM_MANAGE_STATE_GRAN,
									  &psRTDataSet->psRTData[i].psContextStateClientMemInfo);
		if(eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SetupRTDataSet: Failed to alloc Context State"));
			return eError;
		}

		psHWRTDataSet->asHWRTData[i].sContextStateDevAddr =
			psRTDataSet->psRTData[i].psContextStateClientMemInfo->sDevVAddr;

		/* memset to zero to support multi app, per-context PB initialisation on TAKick */
		PDUMPCOMMENT(psDevData->psConnection, "Initialise context state memory", IMG_TRUE);
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
		PVRSRVMemSet(psRTDataSet->psRTData[i].psContextStateClientMemInfo->pvLinAddr,
					 0,
					 ui32DPMStateSize);
		PDUMPMEM(psDevData->psConnection, IMG_NULL,
					psRTDataSet->psRTData[i].psContextStateClientMemInfo, 0,
					ui32DPMStateSize, PDUMP_FLAGS_CONTINUOUS);
#else
		PVRSRVMemSet(psRTDataSet->psRTData[i].psContextStateClientMemInfo->pvLinAddr,
					 0,
					 EURASIA_PARAM_MANAGE_STATE_SIZE);
		PDUMPMEM(psDevData->psConnection, IMG_NULL,
					psRTDataSet->psRTData[i].psContextStateClientMemInfo, 0,
					EURASIA_PARAM_MANAGE_STATE_SIZE, PDUMP_FLAGS_CONTINUOUS);
#endif

#if !defined(SGX_FEATURE_ISP_BREAKPOINT_RESUME_REV_2) && !defined(FIX_HW_BRN_23761) && !defined(SGX_FEATURE_VISTEST_IN_MEMORY)
		/* Allocate the tile LUT */
		PDUMPCOMMENT(psDevData->psConnection, "Allocate region header tile LUT", IMG_TRUE);
		eError = PVRSRVAllocDeviceMem(psDevData,
									  psKernelVideoDataHeapInfo->hDevMemHeap,
									  PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_EDM_PROTECT \
									  | PVRSRV_MEM_NO_SYNCOBJ | PVRSRV_MEM_CACHE_CONSISTENT,
									  ui32TileRgnLUTSize,
									  32,
									  &psRTDataSet->psRTData[i].psTileRgnLUTClientMemInfo);
		if(eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SetupRTDataSet: Failed to alloc Region header tile LUT"));
			return eError;
		}

		/* Fill in the LUT for region header addresses */
		psHWRTDataSet->asHWRTData[i].sTileRgnLUTDevAddr =
						psRTDataSet->psRTData[i].psTileRgnLUTClientMemInfo->sDevVAddr;
		FillTileRgnLUT(	&psRTDataSet->psRTData[i],
						ui32TilesPerMTileX,
						ui32TilesPerMTileY,
						ui32MTilesX,
						ui32MTilesY,
						ui32TilesPerMTileX * ui32TilesPerMTileY * EURASIA_REGIONHEADER_SIZE);

		PDUMPCOMMENT(psDevData->psConnection, "Initialise Region header tile LUT", IMG_TRUE);
		PDUMPMEM(psDevData->psConnection, IMG_NULL,
					psRTDataSet->psRTData[i].psTileRgnLUTClientMemInfo, 0,
					ui32TileRgnLUTSize, PDUMP_FLAGS_CONTINUOUS);
#endif

#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH) || defined(FIX_HW_BRN_23862) || defined(FIX_HW_BRN_30089) || defined(FIX_HW_BRN_29504) || defined(FIX_HW_BRN_33753)
		/*
		 * allocate the macrotile last region LUT
		 */
		PDUMPCOMMENT(psDevData->psConnection, "Allocate macrotile tile last region LUT", IMG_TRUE);
		eError = PVRSRVAllocDeviceMem(psDevData,
									  psKernelVideoDataHeapInfo->hDevMemHeap,
									  PVRSRV_MEM_READ | PVRSRV_MEM_WRITE |
									  PVRSRV_MEM_EDM_PROTECT | PVRSRV_MEM_NO_SYNCOBJ |
									  PVRSRV_MEM_CACHE_CONSISTENT,
									  ui32MacroTileRgnLUTSize,
									  32,
									  &psRTDataSet->psRTData[i].psLastRgnLUTClientMemInfo);
		if(eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SetupRTDataSet: Failed to alloc Macro Tile last region LUT"));
			return eError;
		}

		/* Fill in the last region LUT */
		psHWRTDataSet->asHWRTData[i].sLastRgnLUTDevAddr =
				psRTDataSet->psRTData[i].psLastRgnLUTClientMemInfo->sDevVAddr;
		FillLastRgnLUT(psRTDataSet,
					   &psRTDataSet->psRTData[i],
					   &psHWRTDataSet->asHWRTData[i],
					   ui32MTilesX,
					   ui32MTilesY,
					   ui32TilesPerMTileX,
					   ui32TilesPerMTileY);

		PDUMPCOMMENT(psDevData->psConnection, "Initialise macrotile tile last region LUT", IMG_TRUE);
		PDUMPMEM(psDevData->psConnection, IMG_NULL,
					psRTDataSet->psRTData[i].psLastRgnLUTClientMemInfo, 0,
					ui32MacroTileRgnLUTSize, PDUMP_FLAGS_CONTINUOUS);
	#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH)
		for (j = 0; j < (ui32MTilesX*ui32MTilesY); j++)
		{
			IMG_PUINT32	pui32LastRgnLUT = psRTDataSet->psRTData[i].psLastRgnLUTClientMemInfo->pvLinAddr;

			if (pui32LastRgnLUT[j] & 0x1)
			{
				psHWRTDataSet->asHWRTData[i].ui32LastMTIdx = j;
			}
		}
	#endif
#endif /* defined(FIX_HW_BRN_23862) */


		/* Mark as rtdata status as Ready and zero stats data */
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
		PDUMPCOMMENT(psDevData->psConnection, "Allocate RTData status memory", IMG_TRUE);
		eError = PVRSRVAllocDeviceMem(psDevData,
									  psKernelVideoDataHeapInfo->hDevMemHeap,
									  PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | \
									  PVRSRV_MEM_NO_SYNCOBJ | PVRSRV_MEM_EDM_PROTECT \
									  | PVRSRV_MEM_CACHE_CONSISTENT,
									  ui16NumRTsInArray * sizeof(IMG_UINT32),
									  32,
									  &psRTDataSet->psRTData[i].psStatusClientMemInfo);
		if(eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SetupRTDataSet: Failed to alloc RTData status memory"));
			return eError;
		}

		psHWRTDataSet->asHWRTData[i].sRTStatusDevAddr =
										psRTDataSet->psRTData[i].psStatusClientMemInfo->sDevVAddr;
		psRTDataSet->psRTData[i].pui32RTStatus =
										psRTDataSet->psRTData[i].psStatusClientMemInfo->pvLinAddr;
		psHWRTDataSet->asHWRTData[i].ui32NumRTsInArray = ui16NumRTsInArray;
		PVRSRVMemSet((IMG_UINT32*)psRTDataSet->psRTData[i].pui32RTStatus, 0,
										ui16NumRTsInArray * sizeof(IMG_UINT32));
		PDUMPCOMMENT(psDevData->psConnection, "Initialise RTData status memory", IMG_TRUE);
		PDUMPMEM(psDevData->psConnection, IMG_NULL,
					psRTDataSet->psRTData[i].psStatusClientMemInfo, 0,
					ui16NumRTsInArray * sizeof(IMG_UINT32), PDUMP_FLAGS_CONTINUOUS);
#else
		/* Point the RTStatusDevAddr at ui32CommonStatus*/
		psHWRTDataSet->asHWRTData[i].sRTStatusDevAddr.uiAddr =
										psRTDataSet->psHWRTDataSetClientMemInfo->sDevVAddr.uiAddr +
												offsetof(SGXMKIF_HWRTDATASET, asHWRTData) +
													offsetof(SGXMKIF_HWRTDATA, ui32CommonStatus) +
													(i * sizeof(SGXMKIF_HWRTDATA));

		psRTDataSet->psRTData[i].pui32RTStatus = &psHWRTDataSet->asHWRTData[i].ui32CommonStatus;
#endif
		psHWRTDataSet->asHWRTData[i].ui32CommonStatus = SGXMKIF_HWRTDATA_CMN_STATUS_READY;
	}

	/* Setup skeleton background objects */
	SetupBackgroundObj(psRenderContext->psClientPBDesc->psPBDesc, psRTDataSet);

	PDUMPCOMMENT(psDevData->psConnection, "Background object", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL,
					psRTDataSet->psSpecialObjClientMemInfo, 0,
					SGX_SPECIALOBJECT_SIZE, PDUMP_FLAGS_CONTINUOUS);
	return PVRSRV_OK;
}

/******************************************************************************
  End of file (sgxrender_targets.c)
 ******************************************************************************/
