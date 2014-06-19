/*!****************************************************************************
@File           sgxkick_client.c

@Title          Device specific kickTA routines

@Author         Imagination Technologies

@Date           03/02/06

@Copyright      Copyright 2006-2009 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       Generic

@Description    User mode part of SGXKickTA

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-
$Log: sgxkick_client.c $

 --- Revision Logs Removed --- 

 --- Revision Logs Removed --- 
******************************************************************************/

#include <stddef.h>

#include "img_defs.h"
#include "services.h"
#include "sgxapi.h"
#include "sgxinfo.h"
#include "sgxinfo_client.h"
#include "pvr_debug.h"
#include "pdump.h"
#include "sgxutils_client.h"
#include "sgxkick_client.h"
#include "sgxtransfer_client.h"
#include "sgx_bridge_um.h"
#include "pdump_um.h"
#include "osfunc_client.h"
#include "sgxpb.h"
#if defined (PDUMP)
#include <stdio.h>
#endif


#if !defined(SGXTQ_PREP_SUBMIT_SEPERATE)
static PVRSRV_ERROR SGXKickSubmit(PVRSRV_DEV_DATA *psDevData,
						  			IMG_VOID *pvKickSubmit);
#endif


#if defined (PDUMP)
/*****************************************************************************
 FUNCTION	: PDumpBitmap

 PURPOSE	: Dumps the bitmap

 PARAMETERS	:

 RETURNS	:
*****************************************************************************/
static IMG_VOID PDumpBitmap (PSGX_RENDERCONTEXT		psRenderContext,
							 IMG_CONST PVRSRV_DEV_DATA		*psDevData,
							 IMG_UINT32				ui32PDumpBitmapSize,
							 PSGX_KICKTA_DUMPBITMAP	psPDumpBitmap,
							 IMG_UINT32				ui32DumpFrameNum,
							 IMG_BOOL				bLastFrame)
{
	IMG_CHAR pszFileName[64];
	IMG_UINT32			i;
	PDUMP_MEM_FORMAT	eAddrMode;
	IMG_UINT32			ui32Size;
	IMG_UINT32			ui32Height;
	IMG_UINT32			ui32Stride;

	for (i=0; i<ui32PDumpBitmapSize; i++, psPDumpBitmap++)
	{
		sprintf(pszFileName, psPDumpBitmap->pszName, ui32DumpFrameNum);

		ui32Height = psPDumpBitmap->ui32Height;
		ui32Stride = psPDumpBitmap->ui32Stride;

		if (psPDumpBitmap->ui32Flags & SGX_KICKTA_DUMPBITMAP_FLAGS_TWIDDLED)
		{
			#if defined(SGX545)
			eAddrMode = PVRSRV_PDUMP_MEM_FORMAT_HYBRID;
			#else /* defined(SGX545) */
			eAddrMode = PVRSRV_PDUMP_MEM_FORMAT_TWIDDLED;
			#endif /* defined(SGX545) */
		}
		else if (psPDumpBitmap->ui32Flags & SGX_KICKTA_DUMPBITMAP_FLAGS_TILED)
		{
			eAddrMode = PVRSRV_PDUMP_MEM_FORMAT_TILED;
		}
		else
		{
			eAddrMode = PVRSRV_PDUMP_MEM_FORMAT_STRIDE;
		}
		if (psPDumpBitmap->ui32Flags & SGX_KICKTA_DUMPBITMAP_FLAGS_TILED)
		{
			ui32Size = ((ui32Height + (EURASIA_TAG_TILE_SIZEY - 1)) & ~(EURASIA_TAG_TILE_SIZEY - 1)) * ui32Stride;
		}
		else
		{
			ui32Size = ui32Height * ui32Stride;
		}

		
		/* PRQA S 1482 11 */ /* ignore cast to enum type warning*/
		PVRSRVPDumpBitmap( psDevData,
					pszFileName,
					0,
					psPDumpBitmap->ui32Width,
					ui32Height,
					ui32Stride,
					psPDumpBitmap->sDevBaseAddr,
					psRenderContext->hDevMemContext,
					ui32Size,
					(PDUMP_PIXEL_FORMAT)psPDumpBitmap->ui32PDUMPFormat,
					eAddrMode,
					bLastFrame ? PDUMP_FLAGS_LASTFRAME : 0);
	}
}


/*****************************************************************************
 FUNCTION	: PDumpPerformanceCounterRegisters

 PURPOSE	: Dumps the performance counters.

 PARAMETERS	:

 RETURNS	:
*****************************************************************************/
static IMG_VOID PDumpPerformanceCounterRegisters (IMG_CONST PVRSRV_DEV_DATA	*psDevData,
												  IMG_UINT32		ui32DumpFrameNum,
												  IMG_BOOL			bLastFrame)
{
	IMG_UINT32 aui32PerfCountRegs[] =
	{
		EUR_CR_PERF,
		EUR_CR_PERF_COUNTER0,
		EUR_CR_PERF_COUNTER1,
		EUR_CR_PERF_COUNTER2,
		EUR_CR_PERF_COUNTER3,
		EUR_CR_PERF_COUNTER4,
		EUR_CR_PERF_COUNTER5,
		EUR_CR_PERF_COUNTER6,
		EUR_CR_PERF_COUNTER7,
#if defined(EUR_CR_PERF_COUNTER8)
		EUR_CR_PERF_COUNTER8,
#endif
	};

	const IMG_UINT32 ui32NumPerfCountRegs = sizeof(aui32PerfCountRegs)/sizeof(aui32PerfCountRegs[0]);
	IMG_UINT32 aui32PerfCountRegsArray[sizeof(aui32PerfCountRegs)/sizeof(aui32PerfCountRegs[0]) * SGX_FEATURE_MP_CORE_COUNT_3D];
	const IMG_UINT32 ui32PerfCountRegsArraySize = sizeof(aui32PerfCountRegsArray)/sizeof(aui32PerfCountRegsArray[0]);
	IMG_UINT32 ui32Core, ui32Counter;

	for(ui32Core = 0; ui32Core < SGX_FEATURE_MP_CORE_COUNT_3D; ui32Core++)
	{
		for(ui32Counter = 0; ui32Counter < ui32NumPerfCountRegs; ui32Counter++)
		{
			aui32PerfCountRegsArray[ui32Core * ui32NumPerfCountRegs + ui32Counter] =
				SGX_MP_CORE_SELECT(aui32PerfCountRegs[ui32Counter], ui32Core);
		}
	}

	PVRSRVPDumpPerformanceCounterRegisters(psDevData, ui32DumpFrameNum, bLastFrame,
										   aui32PerfCountRegsArray, ui32PerfCountRegsArraySize);
}


/*****************************************************************************
 FUNCTION	: PDumpCycleCounters

 PURPOSE	: Dumps the cycle counter registers

 PARAMETERS	:

 RETURNS	:
*****************************************************************************/
static IMG_VOID PDumpCycleCounters(IMG_CONST PVRSRV_DEV_DATA 		*psDevData,
								   IMG_BOOL				bLastFrame,
								   IMG_UINT32			ui32Offset)
{
	#if defined(NO_HARDWARE) || defined(EMULATOR)
	PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "Cycle counters\r\n");
	PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_EMU_TA_PHASE + ui32Offset, bLastFrame);
	PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_EMU_TA_CYCLE + ui32Offset, bLastFrame);
	PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_EMU_3D_PHASE + ui32Offset, bLastFrame);
	PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_EMU_3D_CYCLE + ui32Offset, bLastFrame);
	#if defined(EUR_CR_EMU_INITIAL_TA_CYCLE)
	PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_EMU_INITIAL_TA_CYCLE + ui32Offset, bLastFrame);
	PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_EMU_FINAL_3D_CYCLE + ui32Offset, bLastFrame);
	#endif /* EUR_CR_EMU_INITIAL_TA_CYCLE */
	PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_EMU_TA_OR_3D_CYCLE + ui32Offset, bLastFrame);
	#else
	PVR_UNREFERENCED_PARAMETER(psDevData);
	PVR_UNREFERENCED_PARAMETER(bLastFrame);
	PVR_UNREFERENCED_PARAMETER(ui32Offset);
	#endif /* defined(NO_HARDWARE) || defined(EMULATOR) */
}


/*****************************************************************************
 FUNCTION	: PDumpMemCounters

 PURPOSE	: Dumps the memory bandwidth counter registers

 PARAMETERS	:

 RETURNS	:
*****************************************************************************/
static IMG_VOID PDumpMemCounters(IMG_CONST PVRSRV_DEV_DATA 	*psDevData,
								 IMG_BOOL			bLastFrame)
{
	#if defined(NO_HARDWARE) || defined(EMULATOR)

	PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "Memory Bandwidth Counters\r\n");

	#if defined(SGX_FEATURE_MP)
		PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_MASTER_EMU_MEM_READ, bLastFrame);
		PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_MASTER_EMU_MEM_WRITE, bLastFrame);
		PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_MASTER_EMU_MEM_BYTE_WRITE, bLastFrame);
		PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_MASTER_EMU_MEM1_READ, bLastFrame);
		PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_MASTER_EMU_MEM1_WRITE, bLastFrame);
		PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_MASTER_EMU_MEM1_BYTE_WRITE, bLastFrame);
		PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_MASTER_EMU_MEM2_READ, bLastFrame);
		PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_MASTER_EMU_MEM2_WRITE, bLastFrame);
		PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_MASTER_EMU_MEM2_BYTE_WRITE, bLastFrame);
		PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_MASTER_EMU_MEM3_READ, bLastFrame);
		PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_MASTER_EMU_MEM3_WRITE, bLastFrame);
		PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_MASTER_EMU_MEM3_BYTE_WRITE, bLastFrame);
	#else
		PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_EMU_MEM_READ, bLastFrame);
		PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_EMU_MEM_WRITE, bLastFrame);
		PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_EMU_MEM_BYTE_WRITE, bLastFrame);
		#if defined(EUR_CR_EMU_MEM_STALL)
		PVRSRVPDumpCycleCountRegRead(psDevData, EUR_CR_EMU_MEM_STALL, bLastFrame);
		#endif
	#endif /* SGX_FEATURE_MP */

	#else
	PVR_UNREFERENCED_PARAMETER(psDevData);
	PVR_UNREFERENCED_PARAMETER(bLastFrame);
	#endif /* defined(NO_HARDWARE) || defined(EMULATOR) */
}


/*****************************************************************************
 FUNCTION	: PDump3DSignatureRegisters

 PURPOSE	: Dumps the signature registers for 3D modules.

 PARAMETERS	:

 RETURNS	:
*****************************************************************************/
static IMG_VOID PDump3DSignatureRegisters(PSGX_RENDERCONTEXT		psRenderContext,
										  IMG_CONST PVRSRV_DEV_DATA	*psDevData,
										  IMG_UINT32		ui32DumpFrameNum,
										  IMG_BOOL			bLastFrame)
{

	IMG_UINT32 aui323DRegs[] =
	{
		/* ISP signature registers */
		EUR_CR_ISP_FPU,
#if defined(EUR_CR_ISP_PIPE0_SIG1)
		EUR_CR_ISP_PIPE0_SIG1,
		EUR_CR_ISP_PIPE0_SIG2,
		EUR_CR_ISP_PIPE0_SIG3,
		EUR_CR_ISP_PIPE0_SIG4,
		EUR_CR_ISP_PIPE1_SIG1,
		EUR_CR_ISP_PIPE1_SIG2,
		EUR_CR_ISP_PIPE1_SIG3,
		EUR_CR_ISP_PIPE1_SIG4,
#endif
#if defined(EUR_CR_ISP_SIG1)
		EUR_CR_ISP_SIG1,
		EUR_CR_ISP_SIG2,
		EUR_CR_ISP_SIG3,
		EUR_CR_ISP_SIG4,
#endif
#if defined(EUR_CR_ISP2_ZLS_LOAD_DATA0)
		/* ZLS signature registers */
		EUR_CR_ISP2_ZLS_LOAD_DATA0,
		EUR_CR_ISP2_ZLS_LOAD_DATA1,
		EUR_CR_ISP2_ZLS_LOAD_STENCIL0,
		EUR_CR_ISP2_ZLS_LOAD_STENCIL1,
		EUR_CR_ISP2_ZLS_STORE_DATA0,
		EUR_CR_ISP2_ZLS_STORE_DATA1,
		EUR_CR_ISP2_ZLS_STORE_STENCIL0,
		EUR_CR_ISP2_ZLS_STORE_STENCIL1,
		EUR_CR_ISP2_ZLS_STORE_BIF,
#endif

#if !defined(SGX_FEATURE_MP) && defined(EUR_CR_ISP_FPU_CHECKSUM)
		EUR_CR_ISP_FPU_CHECKSUM,
		EUR_CR_ISP_PRECALC_CHECKSUM,
		EUR_CR_ISP_EDGE_CHECKSUM,
		EUR_CR_ISP_TAGWRITE_CHECKSUM,
		EUR_CR_ISP_SPAN_CHECKSUM,
#endif

#if defined(EUR_CR_ITR_TAG0)
		/* Iterator TAG signature registers */
		EUR_CR_ITR_TAG0,
		EUR_CR_ITR_TAG1,
#endif

#if defined(EUR_CR_ITR_TAG00)
		/* Iterator TAG signature registers */
		EUR_CR_ITR_TAG00,
		EUR_CR_ITR_TAG01,
		EUR_CR_ITR_TAG10,
		EUR_CR_ITR_TAG11,
#endif

#if defined(EUR_CR_TF_SIG00)
		/* TF signature registers */
		EUR_CR_TF_SIG00,
		EUR_CR_TF_SIG01,
		EUR_CR_TF_SIG02,
		EUR_CR_TF_SIG03,
#endif

#if defined(EUR_CR_ITR_USE0)
		/* Iterator USE signature registers */
		EUR_CR_ITR_USE0,
		EUR_CR_ITR_USE1,
		EUR_CR_ITR_USE2,
		EUR_CR_ITR_USE3,
#endif

		/* PIXELBE signature registers */
#if defined(EUR_CR_PIXELBE_SIG01)
		EUR_CR_PIXELBE_SIG01,
		EUR_CR_PIXELBE_SIG02,
#endif
#if defined(EUR_CR_PIXELBE_SIG11)
		EUR_CR_PIXELBE_SIG11,
		EUR_CR_PIXELBE_SIG12,
#endif /* #if defined(EUR_CR_PIXELBE_SIG11) */

#if defined(EUR_CR_PBE_PIXEL_CHECKSUM)
#if !defined(SGX_FEATURE_MP)
		EUR_CR_PBE_PIXEL_CHECKSUM,
#endif /* SGX_FEATURE_MP */
		EUR_CR_PBE_NONPIXEL_CHECKSUM,
#endif
	};
	const IMG_UINT32 ui32Num3DRegs = sizeof(aui323DRegs) / sizeof(aui323DRegs[0]);

	#if defined(SGX_FEATURE_MP)
	IMG_UINT32 aui323DRegsGlobal[] =
	{
		EUR_CR_ISP_FPU_CHECKSUM,
#if defined(EUR_CR_ISP2_PRECALC_CHECKSUM)
		EUR_CR_ISP2_PRECALC_CHECKSUM,
		EUR_CR_ISP2_EDGE_CHECKSUM,
		EUR_CR_ISP2_TAGWRITE_CHECKSUM,
		EUR_CR_ISP2_SPAN_CHECKSUM,
		EUR_CR_PBE_CHECKSUM,
#else
		EUR_CR_ISP_PRECALC_CHECKSUM,
		EUR_CR_ISP_EDGE_CHECKSUM,
		EUR_CR_ISP_TAGWRITE_CHECKSUM,
		EUR_CR_ISP_SPAN_CHECKSUM,
		EUR_CR_PBE_PIXEL_CHECKSUM,
#endif
	};
	const IMG_UINT32 ui32Num3DRegsGlobal = sizeof(aui323DRegsGlobal) / sizeof(aui323DRegsGlobal[0]);
	#endif /* SGX_FEATURE_MP */

	IMG_UINT32 *pui32RegsArray;
	IMG_UINT32 ui32RegsArraySize;
	IMG_UINT32 ui32Core, ui32Counter;

	ui32RegsArraySize = ui32Num3DRegs * SGX_FEATURE_MP_CORE_COUNT_3D;
	#if defined(SGX_FEATURE_MP)
	ui32RegsArraySize += ui32Num3DRegsGlobal;
	#endif /* SGX_FEATURE_MP */

	pui32RegsArray = PVRSRVAllocUserModeMem(ui32RegsArraySize * sizeof(pui32RegsArray[0]));
	if(pui32RegsArray == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "PDump3DSignatureRegisters Out of memory"));
		return;
	}

	for(ui32Core = 0; ui32Core < SGX_FEATURE_MP_CORE_COUNT_3D; ui32Core++)
	{
		for(ui32Counter = 0; ui32Counter < ui32Num3DRegs; ui32Counter++)
		{
			pui32RegsArray[ui32Core * ui32Num3DRegs + ui32Counter] =
				SGX_MP_CORE_SELECT(aui323DRegs[ui32Counter], ui32Core);
		}
	}

	#if defined(SGX_FEATURE_MP)
	for(ui32Counter = 0; ui32Counter < ui32Num3DRegsGlobal; ui32Counter++)
	{
		pui32RegsArray[ui32Core * ui32Num3DRegs + ui32Counter] =
			aui323DRegsGlobal[ui32Counter];
	}
	#endif /* SGX_FEATURE_MP */

	PVRSRVPDump3DSignatureRegisters(psDevData, psRenderContext->hDevMemContext,
									ui32DumpFrameNum, bLastFrame,
									pui32RegsArray, ui32RegsArraySize);

	PVRSRVFreeUserModeMem(pui32RegsArray);
}


/*****************************************************************************
 FUNCTION	: PDumpTASignatureRegisters

 PURPOSE	: Dumps the signature registers for TA modules.

 PARAMETERS	:

 RETURNS	:
*****************************************************************************/
static IMG_VOID PDumpTASignatureRegisters(IMG_CONST PVRSRV_DEV_DATA	*psDevData,
										  IMG_UINT32		ui32DumpFrameNum,
										  IMG_UINT32		ui32TAKickCount,
										  IMG_BOOL			bLastFrame)
{
	IMG_UINT32 aui32TARegs[] =
	{
		EUR_CR_CLIP_SIG1,
#if defined(EUR_CR_MTE_MEM_CHECKSUM)
		EUR_CR_MTE_MEM_CHECKSUM,
		EUR_CR_MTE_TE_CHECKSUM,
		EUR_CR_TE_CHECKSUM,
#else
		EUR_CR_MTE_SIG1,
		EUR_CR_MTE_SIG2,
#endif
#if defined(EUR_CR_TE1)
		EUR_CR_TE1,
		EUR_CR_TE2
#endif
#if defined(VDM_MTE)
		EUR_CR_VDM_MTE,
#endif
	};
	const IMG_UINT32 ui32NumTARegs = sizeof(aui32TARegs) / sizeof(aui32TARegs[0]);

	#if defined(SGX_FEATURE_MP)
	IMG_UINT32 aui32TARegsGlobal[] = {
		EUR_CR_CLIP_CHECKSUM,
		EUR_CR_MTE_MEM_CHECKSUM,
		EUR_CR_MTE_TE_CHECKSUM,
		EUR_CR_TE_CHECKSUM,
	};
	const IMG_UINT32 ui32NumTARegsGlobal = sizeof(aui32TARegsGlobal) / sizeof(aui32TARegsGlobal[0]);
	#endif /* SGX_FEATURE_MP */

	IMG_UINT32 *pui32RegsArray;
	IMG_UINT32 ui32RegsArraySize;
	IMG_UINT32 ui32Core, ui32Counter;

	ui32RegsArraySize = ui32NumTARegs * SGX_FEATURE_MP_CORE_COUNT_TA;
	#if defined(SGX_FEATURE_MP)
	ui32RegsArraySize += ui32NumTARegsGlobal;
	#endif /* SGX_FEATURE_MP */

	pui32RegsArray = PVRSRVAllocUserModeMem(ui32RegsArraySize * sizeof(pui32RegsArray[0]));
	if(pui32RegsArray == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "PDumpTASignatureRegisters Out of memory"));
		return;
	}

	for(ui32Core = 0; ui32Core < SGX_FEATURE_MP_CORE_COUNT_TA; ui32Core++)
	{
		for(ui32Counter = 0; ui32Counter < ui32NumTARegs; ui32Counter++)
		{
			pui32RegsArray[ui32Core * ui32NumTARegs + ui32Counter] =
				SGX_MP_CORE_SELECT(aui32TARegs[ui32Counter], ui32Core);
		}
	}

	#if defined(SGX_FEATURE_MP)
	for(ui32Counter = 0; ui32Counter < ui32NumTARegsGlobal; ui32Counter++)
	{
		pui32RegsArray[ui32Core * ui32NumTARegs + ui32Counter] =
			aui32TARegsGlobal[ui32Counter];
	}
	#endif /* SGX_FEATURE_MP */

	PVRSRVPDumpTASignatureRegisters(psDevData, ui32DumpFrameNum, ui32TAKickCount,
									bLastFrame, pui32RegsArray, ui32RegsArraySize);

	PVRSRVFreeUserModeMem(pui32RegsArray);
}


/*****************************************************************************
 FUNCTION	: PDumpDPMInfo

 PURPOSE	: Dumps data for debugging the DPM.

 PARAMETERS	:

 RETURNS	:
*****************************************************************************/
static IMG_VOID PDumpDPMInfo(PVRSRV_DEV_DATA	*psDevData,
							 SGX_RENDERCONTEXT	*psRenderContext)
{
	PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "\r\n-- Dump DPM page table\r\n");
	PVRSRVPDumpSaveMem(psDevData, "dpmpagetable", 0,
					   psRenderContext->psClientPBDesc->psEVMPageTableClientMemInfo->sDevVAddr,
					   (IMG_UINT32)psRenderContext->psClientPBDesc->psEVMPageTableClientMemInfo->uAllocSize,
					   psRenderContext->hDevMemContext, 0);

	PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "Dump DPM counters\r\n");
	#if defined(SGX_FEATURE_MP)
	PVRSRVPDumpRegRead(psDevData, "SGXREG", "dpm_flist.sig", 0x0, EUR_CR_MASTER_DPM_TA_ALLOC_FREE_LIST, 0, 0);
	PVRSRVPDumpRegRead(psDevData, "SGXREG","dpm_flist.sig", 0x4, EUR_CR_MASTER_DPM_TA_ALLOC_FREE_LIST_STATUS1, 0, 0);
	PVRSRVPDumpRegRead(psDevData, "SGXREG","dpm_flist.sig", 0x8, EUR_CR_MASTER_DPM_TA_ALLOC_FREE_LIST_STATUS2, 0, 0);
	#else
	PVRSRVPDumpRegRead(psDevData, "SGXREG","dpm_flist.sig", 0x0, EUR_CR_DPM_TA_ALLOC_FREE_LIST, 0, 0);
	PVRSRVPDumpRegRead(psDevData, "SGXREG","dpm_flist.sig", 0x4, EUR_CR_DPM_TA_ALLOC_FREE_LIST_STATUS1, 0, 0);
	PVRSRVPDumpRegRead(psDevData, "SGXREG","dpm_flist.sig", 0x8, EUR_CR_DPM_TA_ALLOC_FREE_LIST_STATUS2, 0, 0);
	#endif /* SGX_FEATURE_MP */
}


/*!
 * *****************************************************************************
 * @brief Do pdumping actions after a render.
 *
 * @param psDevData
 * @param pui32RenderNum
 * @param bLastFrameDumped
 * @param ui32PDumpBitmapSize
 * @param psPDumpBitmap
 * @param bLastFrame
 *
 * @return
 ********************************************************************************/
static IMG_VOID PDumpAfterRender(PVRSRV_DEV_DATA				*psDevData,
								 SGX_RENDERCONTEXT				*psRenderContext,
								 IMG_PUINT32					pui32RenderNum,
								 IMG_BOOL						bLastFrameDumped,
								 IMG_UINT32						ui32PDumpBitmapSize,
								 PSGX_KICKTA_DUMPBITMAP			psPDumpBitmap,
								 IMG_BOOL						bLastFrame)
{
	if (ui32PDumpBitmapSize > 0)
	{
		PDumpBitmap(psRenderContext, psDevData, ui32PDumpBitmapSize, psPDumpBitmap,
					*pui32RenderNum, bLastFrame);
	}

	{
		IMG_UINT32 ui32CycleCountersOffset;

		#if defined(SGX_FEATURE_MP)
		ui32CycleCountersOffset = SGX_MP_MASTER_SELECT(0);
		#else
		ui32CycleCountersOffset = 0;
		#endif /* SGX_FEATURE_MP */

		PDumpCycleCounters(psDevData, bLastFrame, ui32CycleCountersOffset);

		PDumpMemCounters(psDevData, bLastFrame);

		PDump3DSignatureRegisters(psRenderContext, psDevData, *pui32RenderNum, bLastFrame);

		PDumpPerformanceCounterRegisters(psDevData, *pui32RenderNum, bLastFrame);

		PDumpDPMInfo(psDevData, psRenderContext);

		#if defined(SUPPORT_SGX_HWPERF)
		PVRSRVPDumpHWPerfCB(psDevData, psRenderContext->hDevMemContext,
							"out.perfcb", 0, bLastFrame ? PDUMP_FLAGS_LASTFRAME : 0);
		#endif /* SUPPORT_SGX_HWPERF */

		PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0,
							   "\r\n-- Disable PDS timer to fake an SGX active power event\r\n");
		PVRSRVPDumpReg(psDevData, "SGXREG", SGX_MP_CORE_SELECT(EUR_CR_EVENT_TIMER, 0), 0, 0);
	}

	if (bLastFrameDumped)
	{
		(*pui32RenderNum)++;
	}
}
#endif /* PDUMP */


/*!
 * *****************************************************************************
 * @brief Sets up the 3D registers needed at the start of a scene
 *
 * @param psRenderDetails
 * @param psKick  Kick data
 * @param psRTDataSet
 * @param bForcePTOff
 *
 * @return
 ********************************************************************************/
static IMG_VOID Setup3DRegs	(PSGX_RENDERDETAILS	psRenderDetails,
							 SGX_KICKTA_COMMON	*psKick,
							 PSGX_RTDATASET		psRTDataSet,
							 PSGX_RENDERCONTEXT	psRenderContext)
{
	SGX_PBDESC 			*psPBDesc;
	PSGX_RTDATA			psRTData;
	PSGXMKIF_3DREGISTERS	ps3DRegs;
	IMG_UINT32				ui32ISPMultiSampleCtl;
	IMG_UINT32				ui32ZLSBase;
	IMG_UINT32				ui32StencilBase;
	IMG_UINT32				i;
#if defined(EUR_CR_ISP_MULTISAMPLECTL2)
	IMG_UINT32				ui32ISPMultiSampleCtl2 = 0;
#endif
#if defined(SGX_FEATURE_ZLS_EXTERNALZ)
	PSGX_RTDATA		psZLSRTData;

	/* Point ZLS at the opposite set of region headers... */
	psZLSRTData = &psRTDataSet->psRTData[(psRTDataSet->ui32RTDataSel + 1) % psRTDataSet->ui32NumRTData];
#endif

	psPBDesc = psRenderContext->psClientPBDesc->psPBDesc;

	/* Decide which buffer we're going to be using */
	psRTData = &psRTDataSet->psRTData[psRTDataSet->ui32RTDataSel];

	/* set the precalculated MSAA config */
	ui32ISPMultiSampleCtl = psRTDataSet->ui32ISPMultiSampleCtl;

#if defined(EUR_CR_ISP_MULTISAMPLECTL2)
	ui32ISPMultiSampleCtl2 = psRTDataSet->ui32ISPMultiSampleCtl2;

	/* TODO: get rid of the SGX545 conditional below... this is how it is in the .h
	   for now though -- ** NEEDS A FEATUREDEF! ** */
#if defined(SGX545)
	/* Set multisample sub-pixel offsets */
	if ((psRTDataSet->ui16MSAASamplesInX == 1)
		&& (psRTDataSet->ui16MSAASamplesInY == 1)
		&& ((psRTDataSet->ui32Flags & SGX_RTDSFLAGS_OPENGL) == 0)
		&& (psKick->ui32OrderDepAASampleGrid != 0 || psKick->ui32OrderDepAASampleGrid2 != 0))
	{
#if defined(SGX_FEATURE_MSAA_5POSITIONS)
		PVR_DPF((PVR_DBG_ERROR, "Setup3dRegs: order dependent anti-aliasing not compatible with 5position option");
#endif
		/* Use the order dependant AA sample positions if present... */
		ui32ISPMultiSampleCtl = psKick->ui32OrderDepAASampleGrid;
		ui32ISPMultiSampleCtl2 = psKick->ui32OrderDepAASampleGrid2;
	}
#endif /* defined(SGX545) */
#endif /* #if defined(EUR_CR_ISP_MULTISAMPLECTL2) */

	/* Set 3D register values */
	ps3DRegs = &psRenderDetails->s3DRegs;

#if defined(EUR_CR_PIXELBE_TTE0_SHIFT)
	ps3DRegs->ui32PixelBE	=	psKick->ui32PixelBE |
								(0x80UL << EUR_CR_PIXELBE_ALPHATHRESHOLD_SHIFT) |
								(0xFFUL << EUR_CR_PIXELBE_TTE0_SHIFT) |
								(0xFFUL << EUR_CR_PIXELBE_TTE1_SHIFT);
#else
	ps3DRegs->ui32PixelBE	=	psKick->ui32PixelBE |
								(0x80UL << EUR_CR_PIXELBE_ALPHATHRESHOLD_SHIFT);
#endif /* EUR_CR_PIXELBE_TTE0_SHIFT */

#if defined(EUR_CR_4Kx4K_RENDER)
	/* If the render target is greater than 2K in either direction switch mode */
	if (psRTDataSet->ui32NumPixelsX > 2048 || psRTDataSet->ui32NumPixelsY > 2048)
	{
		ps3DRegs->ui324KRender	=	EUR_CR_4KX4K_RENDER_MODE_MASK;
	}
	else
	{
		ps3DRegs->ui324KRender	=	0;
	}
#endif

#if defined(SGX_FEATURE_VISTEST_IN_MEMORY)
	for (i = 0; i < SGX_FEATURE_MP_CORE_COUNT_3D; i++)
	{
		if (psKick->ui32KickFlags & SGX_KICKTA_FLAGS_GETVISRESULTS)
		{
			ps3DRegs->aui32VisRegBase[i] = psKick->sVisTestResultMem.uiAddr +
											((i * psKick->ui32VisTestCount) * sizeof(IMG_UINT32));
		}
		else
		{
			ps3DRegs->aui32VisRegBase[i] = 0;
		}
	}
#endif

	for(i=0; i<SGX_FEATURE_MP_CORE_COUNT_TA; i++)
	{
		ps3DRegs->aui32ISPRgnBase[i]	=	psRTData->apsRgnHeaderClientMemInfo[i]->sDevVAddr.uiAddr;
	}
#if defined(SGX_FEATURE_MP)
	ps3DRegs->ui32ISPRgn		=	psRTData->apsRgnHeaderClientMemInfo[0]->uAllocSize;
#endif
#if defined(EUR_CR_ISP_MTILE)
	#if defined(FIX_HW_BRN_29960)
	ps3DRegs->ui32ISPMTile1	= (psRTDataSet->ui32MTileNumber << EUR_CR_ISP_MTILE1_NUMBER_SHIFT) |
								((psRTDataSet->ui32MTileX1*psRTDataSet->ui16MSAASamplesInX) << EUR_CR_ISP_MTILE1_X1_SHIFT) |
								((psRTDataSet->ui32MTileX2*psRTDataSet->ui16MSAASamplesInX) << EUR_CR_ISP_MTILE1_X2_SHIFT) |
								((psRTDataSet->ui32MTileX3*psRTDataSet->ui16MSAASamplesInX) << EUR_CR_ISP_MTILE1_X3_SHIFT);
	ps3DRegs->ui32ISPMTile2	= 	((psRTDataSet->ui32MTileY1*psRTDataSet->ui16MSAASamplesInY) << EUR_CR_ISP_MTILE2_Y1_SHIFT) |
								((psRTDataSet->ui32MTileY2*psRTDataSet->ui16MSAASamplesInY) << EUR_CR_ISP_MTILE2_Y2_SHIFT) |
								((psRTDataSet->ui32MTileY3*psRTDataSet->ui16MSAASamplesInY) << EUR_CR_ISP_MTILE2_Y3_SHIFT);
	ps3DRegs->ui32ISPMTile 	= ((psRTDataSet->ui32MTileStride * (psRTDataSet->ui16MSAASamplesInX*psRTDataSet->ui16MSAASamplesInY)) 
									<< EUR_CR_ISP_MTILE_STRIDE_SHIFT);
	#else
	ps3DRegs->ui32ISPMTile		=	(psRTDataSet->ui32MTileStride << EUR_CR_ISP_MTILE_STRIDE_SHIFT);
	ps3DRegs->ui32ISPMTile1		=	(psRTDataSet->ui32MTileNumber << EUR_CR_ISP_MTILE1_NUMBER_SHIFT) |
									(psRTDataSet->ui32MTileX1 << EUR_CR_ISP_MTILE1_X1_SHIFT) |
									(psRTDataSet->ui32MTileX2 << EUR_CR_ISP_MTILE1_X2_SHIFT) |
									(psRTDataSet->ui32MTileX3 << EUR_CR_ISP_MTILE1_X3_SHIFT);
	ps3DRegs->ui32ISPMTile2		=	(psRTDataSet->ui32MTileY1 << EUR_CR_ISP_MTILE2_Y1_SHIFT) |
									(psRTDataSet->ui32MTileY2 << EUR_CR_ISP_MTILE2_Y2_SHIFT) |
									(psRTDataSet->ui32MTileY3 << EUR_CR_ISP_MTILE2_Y3_SHIFT);
	#endif
#endif /* SGX_FEATURE_MP */

	ps3DRegs->ui32ISPIPFMisc	=	psKick->ui32ISPIPFMisc |
									(psKick->ui32ISPValidId << EUR_CR_ISP_IPFMISC_VALIDID_SHIFT);
#if defined(EUR_CR_DOUBLE_PIXEL_PARTITIONS)
	ps3DRegs->ui32DoublePixelPartitions	=	psKick->ui32DoublePixelPartitions;


#endif

#if defined(EUR_CR_ISP_OGL_MODE)
	ps3DRegs->ui32ISPOGLMode	=	(psRTDataSet->ui32Flags & SGX_RTDSFLAGS_OPENGL) ? EUR_CR_ISP_OGL_MODE_ENABLE_MASK : 0;
	ps3DRegs->ui32ISPPerpendicular	=	psKick->ui32ISPPerpendicular;
	ps3DRegs->ui32ISPCullValue		=	psKick->ui32ISPCullValue;
#endif
#if defined(SGX_FEATURE_ZLS_EXTERNALZ)
	for(i=0; i<SGX_FEATURE_MP_CORE_COUNT_3D; i++)
	{
		/* All cores point to core 0s region headers */
		ps3DRegs->aui32ZLSExtZRgnBase[i]	=	psZLSRTData->apsRgnHeaderClientMemInfo[0]->sDevVAddr.uiAddr;
	}
#endif
#if defined(SGX_FEATURE_DEPTH_BIAS_OBJECTS)
	ps3DRegs->ui32ISPDBias[0]	=	psKick->ui32ISPDBias;
	ps3DRegs->ui32ISPDBias[1]	=	psKick->ui32ISPDBias1;
	ps3DRegs->ui32ISPDBias[2]	=	psKick->ui32ISPDBias2;
#else
	ps3DRegs->ui32ISPDBias[0]	=	psKick->ui32ISPDBias;
#endif
	ps3DRegs->ui323DAAMode			=	psRTDataSet->ui323DAAMode;
	ps3DRegs->ui32ISPZLSCtl			=	psKick->ui32ZLSCtl;
	ps3DRegs->ui32ISPBgObjDepth		=	psKick->ui32ISPBGObjDepth;
	ps3DRegs->ui32ISPBgObj			=	psKick->ui32ISPBGObj | psKick->ui32ISPSceneBGObj;
	ps3DRegs->ui32ISPBgObjTag		=	psRTData->ui32BGObjPtr1;
	ps3DRegs->ui32ISPMultisampleCtl		=	ui32ISPMultiSampleCtl;
#if defined(EUR_CR_ISP_MULTISAMPLECTL2)
	ps3DRegs->ui32ISPMultisampleCtl2	=	ui32ISPMultiSampleCtl2;
#endif
	ps3DRegs->ui32ISPTAGCtrl	=	(IMG_UINT32)((psRenderContext->bForcePTOff) ? EUR_CR_ISP_TAGCTRL_FORCE_PT_OFF_MASK : 0UL);
#if defined(FIX_HW_BRN_23141) || defined(FIX_HW_BRN_22656) || defined(FIX_HW_BRN_22849)
	ps3DRegs->ui32ISPTAGCtrl	|=	EUR_CR_ISP_TAGCTRL_SLOWFPT_MASK;
#endif

#if defined(EUR_CR_PDS_PP_INDEPENDANT_STATE)
	if(psKick->ui32TriangleSplitPixelThreshold)
	{
		ps3DRegs->ui32ISPTAGCtrl |= (psKick->ui32TriangleSplitPixelThreshold << EUR_CR_ISP_TAGCTRL_TRIANGLE_FRAGMENT_PIXELS_SHIFT);
		ps3DRegs->ui32PDSPPState = 0;
	}
	else
	{
		ps3DRegs->ui32ISPTAGCtrl |= EUR_CR_ISP_TAGCTRL_TRIANGLE_FRAGMENT_DISABLE_MASK;
		ps3DRegs->ui32PDSPPState = EUR_CR_PDS_PP_INDEPENDANT_STATE_DISABLE_MASK;
	}
#endif

#if defined(EUR_CR_ISP_FPUCTRL)
	ps3DRegs->ui32ISPFPUCtrl	=	(psRTDataSet->ui32Flags & SGX_RTDSFLAGS_OPENGL) ? EUR_CR_ISP_FPUCTRL_SAMPLE_POS_MASK : 0;
#endif

#if defined(EUR_CR_3D_AA_MODE_GLOBALREGISTER_MASK)
	/* Enable global sample positions */
	ps3DRegs->ui323DAAMode		|= EUR_CR_3D_AA_MODE_GLOBALREGISTER_MASK;
#endif

	ps3DRegs->ui32TSPConfig		=	0;

#if defined(EUR_CR_TSP_CONFIG_DADJUST_RANGE_MASK)
	ps3DRegs->ui32TSPConfig |= EUR_CR_TSP_CONFIG_DADJUST_RANGE_MASK;
#endif
#if defined(SGX_FEATURE_CEM_FACE_PACKING)
	ps3DRegs->ui32TSPConfig |= EUR_CR_TSP_CONFIG_CEM_FACE_PACKING_MASK;
#endif

	ps3DRegs->ui32BIF3DReqBase	=	(psPBDesc->sParamHeapBaseDevVAddr.uiAddr >> EUR_CR_BIF_3D_REQ_BASE_ADDR_ALIGNSHIFT)
									<< EUR_CR_BIF_3D_REQ_BASE_ADDR_SHIFT;

	/*
		If automatic allocation of ZLS pages from the parameter buffer is on then point the ZLS requestor base to the
		same point as the TA requestor base - otherwise use the Z base address passed in by the client.
	*/
#if defined(EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_CMPR_F32ZI8S1V)
	if( ((psKick->ui32ZLSCtl & EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_MASK) >= EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_CMPR_F32ZI8S1V) &&
		((psKick->ui32ZLSCtl & EUR_CR_ISP_ZLSCTL_EXTERNALZBUFFER_MASK) == 0))
#else
	if ( ((psKick->ui32ZLSCtl & EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_MASK) == EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_F32ZI8S1V)
		#if defined(SGX_FEATURE_ZLS_EXTERNALZ)
			&& ((psKick->ui32ZLSCtl & EUR_CR_ISP_ZLSCTL_EXTERNALZBUFFER_MASK) == 0)
		#endif
		)
#endif
	{
		ui32ZLSBase	=	(psPBDesc->sParamHeapBaseDevVAddr.uiAddr >> EUR_CR_BIF_ZLS_REQ_BASE_ADDR_ALIGNSHIFT) << EUR_CR_BIF_ZLS_REQ_BASE_ADDR_SHIFT;
	}
	else
	{
		if((psKick->sISPZLoadBase.uiAddr != 0) && (psKick->sISPZLoadBase.uiAddr < psKick->sISPZStoreBase.uiAddr))
		{
			ui32ZLSBase	= psKick->sISPZLoadBase.uiAddr;
		}
		else
		{
			ui32ZLSBase	= psKick->sISPZStoreBase.uiAddr;
		}

		ui32StencilBase	= psKick->sISPStencilLoadBase.uiAddr;

		if((ui32StencilBase != 0) && ((ui32ZLSBase == 0) || (ui32StencilBase < ui32ZLSBase)))
		{
			ui32ZLSBase	= ui32StencilBase;
		}

		ui32StencilBase = psKick->sISPStencilStoreBase.uiAddr;

		if((ui32StencilBase != 0) && ((ui32ZLSBase == 0) || (ui32StencilBase < ui32ZLSBase)))
		{
			ui32ZLSBase = ui32StencilBase;
		}

		ui32ZLSBase	= (ui32ZLSBase >> EUR_CR_BIF_ZLS_REQ_BASE_ADDR_ALIGNSHIFT) << EUR_CR_BIF_ZLS_REQ_BASE_ADDR_SHIFT;
	}

	ps3DRegs->ui32BIFZLSReqBase			=	ui32ZLSBase;
	ps3DRegs->ui32ISPZLoadBase			=	(psKick->sISPZLoadBase.uiAddr - ui32ZLSBase) & EUR_CR_ISP_ZLOAD_BASE_ADDR_MASK;
	ps3DRegs->ui32ISPZStoreBase			=	(psKick->sISPZStoreBase.uiAddr - ui32ZLSBase) & EUR_CR_ISP_ZSTORE_BASE_ADDR_MASK;
	ps3DRegs->ui32ISPStencilLoadBase	=	((psKick->sISPStencilLoadBase.uiAddr - ui32ZLSBase) & EUR_CR_ISP_STENCIL_LOAD_BASE_ADDR_MASK) |
											(psKick->bSeperateStencilLoadStore ? EUR_CR_ISP_STENCIL_LOAD_BASE_ENABLE_MASK : 0);
	ps3DRegs->ui32ISPStencilStoreBase	=	((psKick->sISPStencilStoreBase.uiAddr - ui32ZLSBase) & EUR_CR_ISP_STENCIL_STORE_BASE_ADDR_MASK) |
											(psKick->bSeperateStencilLoadStore ? EUR_CR_ISP_STENCIL_STORE_BASE_ENABLE_MASK : 0);

	/* Setup the Pixel EDM registers */
	ps3DRegs->ui32EDMPixelPDSExec		= psKick->ui32EDMPixelPDSAddr;
	ps3DRegs->ui32EDMPixelPDSData		= psKick->ui32EDMPixelPDSDataSize;
	ps3DRegs->ui32EDMPixelPDSInfo		= psKick->ui32EDMPixelPDSInfo;

#if defined(SUPPORT_DISPLAYCONTROLLER_TILING)
	ps3DRegs->ui32BIFTile0Config		= psKick->ui32BIFTile0Config;
#endif

#if defined(SGX_FEATURE_ALPHATEST_COEFREORDER) && defined(EUR_CR_ISP_DEPTHSORT_FB_ABC_ORDER_MASK)

	if (psKick->ui32KickFlags & SGX_KICKTA_FLAGS_ABC_REORDER)
	{
		ps3DRegs->ui32ISPDepthsort |= EUR_CR_ISP_DEPTHSORT_FB_ABC_ORDER_MASK;
	}
	else
	{
		ps3DRegs->ui32ISPDepthsort &= ~EUR_CR_ISP_DEPTHSORT_FB_ABC_ORDER_MASK;
	}

#endif

	/*
		Setup the DMS ctrl.
	*/
	if (psRenderContext->ui32NumPixelPartitions != 0)
	{
		psRenderDetails->psHWRenderDetails->ui32NumPixelPartitions =
				psRenderContext->ui32NumPixelPartitions << EUR_CR_DMS_CTRL_MAX_NUM_PIXEL_PARTITIONS_SHIFT;
	}
	else
	{
#if defined(SGX_MAX_PIXEL_PARTITIONS_CHECK_MSAA) 
		if ((psRTDataSet->ui16MSAASamplesInX * psRTDataSet->ui16MSAASamplesInY) > 1)
		{
			psRenderDetails->psHWRenderDetails->ui32NumPixelPartitions = (SGX_DEFAULT_MAX_NUM_PIXEL_PARTITIONS - SGX_MAX_PIXEL_PARTITIONS_MSAA_REDUCTION) << EUR_CR_DMS_CTRL_MAX_NUM_PIXEL_PARTITIONS_SHIFT;
		}
		else
		{
			psRenderDetails->psHWRenderDetails->ui32NumPixelPartitions = SGX_DEFAULT_MAX_NUM_PIXEL_PARTITIONS << EUR_CR_DMS_CTRL_MAX_NUM_PIXEL_PARTITIONS_SHIFT;
		}
#else
		psRenderDetails->psHWRenderDetails->ui32NumPixelPartitions = SGX_DEFAULT_MAX_NUM_PIXEL_PARTITIONS << EUR_CR_DMS_CTRL_MAX_NUM_PIXEL_PARTITIONS_SHIFT;
#endif
	}

	/* Setup FIRH coeffs */
	ps3DRegs->ui32USEFilter0Left	= psKick->ui32Filter0Left;
#if defined(SGX_FEATURE_USE_HIGH_PRECISION_FIR_COEFF)
	ps3DRegs->ui32USEFilter0Centre 	= psKick->ui32Filter0Centre;
#endif
	ps3DRegs->ui32USEFilter0Right 	= psKick->ui32Filter0Right;
	ps3DRegs->ui32USEFilter0Extra 	= psKick->ui32Filter0Extra;
	ps3DRegs->ui32USEFilter1Left 	= psKick->ui32Filter1Left;
#if defined(SGX_FEATURE_USE_HIGH_PRECISION_FIR_COEFF)
	ps3DRegs->ui32USEFilter1Centre 	= psKick->ui32Filter1Centre;
#endif
	ps3DRegs->ui32USEFilter1Right 	= psKick->ui32Filter1Right;
	ps3DRegs->ui32USEFilter1Extra 	= psKick->ui32Filter1Extra;
	ps3DRegs->ui32USEFilter2Left 	= psKick->ui32Filter2Left;
	ps3DRegs->ui32USEFilter2Right 	= psKick->ui32Filter2Right;
	ps3DRegs->ui32USEFilter2Extra 	= psKick->ui32Filter2Extra;
	ps3DRegs->ui32USEFilterTable 	= psKick->ui32FilterTable;

#if defined(SGX_FEATURE_TAG_YUV_TO_RGB)
	/* Setup CSC coeffs */
	ps3DRegs->ui32CSC0Coeff01	= psKick->ui32CSC0Coeff01;
	ps3DRegs->ui32CSC0Coeff23 	= psKick->ui32CSC0Coeff23;
	ps3DRegs->ui32CSC0Coeff45 	= psKick->ui32CSC0Coeff45;
	ps3DRegs->ui32CSC0Coeff67 	= psKick->ui32CSC0Coeff67;

	ps3DRegs->ui32CSC1Coeff01 	= psKick->ui32CSC1Coeff01;
	ps3DRegs->ui32CSC1Coeff23 	= psKick->ui32CSC1Coeff23;
	ps3DRegs->ui32CSC1Coeff45 	= psKick->ui32CSC1Coeff45;
	ps3DRegs->ui32CSC1Coeff67 	= psKick->ui32CSC1Coeff67;

#if !defined(SGX_FEATURE_TAG_YUV_TO_RGB_HIPREC)
	ps3DRegs->ui32CSC0Coeff8 	= psKick->ui32CSC0Coeff8;
	ps3DRegs->ui32CSC1Coeff8 	= psKick->ui32CSC1Coeff8;
	ps3DRegs->ui32CSCScale 		= psKick->ui32CSCScale;
#else
	ps3DRegs->ui32CSC0Coeff89 	= psKick->ui32CSC0Coeff89;
	ps3DRegs->ui32CSC0Coeff1011 = psKick->ui32CSC0Coeff1011;
	ps3DRegs->ui32CSC1Coeff89 	= psKick->ui32CSC1Coeff89;
	ps3DRegs->ui32CSC1Coeff1011 = psKick->ui32CSC1Coeff1011;
#endif

#endif
#if defined(EUR_CR_TPU_LUMA0)
	ps3DRegs->ui32TPULuma0			= psKick->ui32TPULuma0;
	ps3DRegs->ui32TPULuma1			= psKick->ui32TPULuma1;
	ps3DRegs->ui32TPULuma2			= psKick->ui32TPULuma2;
#endif

	/*
		Copy the host structure to the hardware structure, fine to copy at this
		point as the ukernel must have released the lock
	*/
	PVRSRVMemCopy(&psRenderDetails->psHWRenderDetails->s3DRegs, ps3DRegs,
					sizeof(SGXMKIF_3DREGISTERS));
}


/*!
 * *****************************************************************************
 * @brief Sets up the TA registers
 *
 * @param psKick
 * @param ui32TARegFlags
 * @param psTARegs
 * @param pui32NumVertexPartitions
 * @param psRTDataSet
 *
 * @return
 ********************************************************************************/
static IMG_VOID SetupTARegs(SGX_KICKTA_COMMON *psKick,
						SGXMKIF_CMDTA  *psTACmd,
						PSGX_RTDATASET         psRTDataSet,
						PSGX_RENDERCONTEXT     psRenderContext)
{
	SGX_PBDESC 				*psPBDesc;
	IMG_UINT32				ui32TEPSG;
	IMG_UINT32				ui32TEAA = 0;
	IMG_UINT32				i;
	IMG_UINT32				ui32NumVertexPartitions;
	IMG_FLOAT				fWClamp;
	SGXMKIF_TAREGISTERS   			*psTARegs = &(psTACmd->sTARegs);
	IMG_PUINT32            			pui32NumVertexPartitions = &(psTACmd->ui32NumVertexPartitions);
	IMG_PUINT32            			pui32SPMNumVertexPartitions = &(psTACmd->ui32SPMNumVertexPartitions);

	psPBDesc = psRenderContext->psClientPBDesc->psPBDesc;

	/* How many partitions are vertex tasks allowed to use? */
	if (psRenderContext->ui32NumVertexPartitions != 0)
	{
		ui32NumVertexPartitions = psRenderContext->ui32NumVertexPartitions;
	}
	else
	{
#if defined(SGX_MAX_VERTEX_PARTITIONS_CHECK_COMPLEX_GEOMETRY)
		if(psKick->ui32KickFlags & SGX_KICKTA_FLAGS_COMPLEX_GEOMETRY_PRESENT)
		{
			ui32NumVertexPartitions = SGX_DEFAULT_MAX_NUM_VERTEX_PARTITIONS - SGX_MAX_VERTEX_PARTITIONS_COMPLEX_GEOMETRY_REDUCTION;
		}
		else
		{
			ui32NumVertexPartitions = SGX_DEFAULT_MAX_NUM_VERTEX_PARTITIONS;
		}
#else
		ui32NumVertexPartitions = SGX_DEFAULT_MAX_NUM_VERTEX_PARTITIONS;
#endif
	}
	*pui32SPMNumVertexPartitions = (SGX_DEFAULT_SPM_MAX_NUM_VERTEX_PARTITIONS << EUR_CR_DMS_CTRL_MAX_NUM_VERTEX_PARTITIONS_SHIFT);
	*pui32NumVertexPartitions = ui32NumVertexPartitions << EUR_CR_DMS_CTRL_MAX_NUM_VERTEX_PARTITIONS_SHIFT;

	/* Determine which bits need to be set in the TE PSG register */
	ui32TEPSG = 0;

	if (psKick->ui32KickFlags & (SGX_KICKTA_FLAGS_TERMINATE | SGX_KICKTA_FLAGS_ABORT))
	{
		/*
			Select terminate of the tile lists with or without a reset
			of the tail pointer cache.
		*/
		if (psKick->ui32KickFlags & SGX_KICKTA_FLAGS_RESETTPC)
		{
			ui32TEPSG |= EUR_CR_TE_PSG_COMPLETEONTERMINATE_MASK;
		}
		else
		{
			ui32TEPSG |= EUR_CR_TE_PSG_ZONLYRENDER_MASK;
		}
	}

	if (psRenderContext->ui32TAKickRegFlags & PVRSRV_SGX_TAREGFLAGS_FORCEPSGPADZER0)
	{
		ui32TEPSG |= EUR_CR_TE_PSG_PADZEROS_MASK;
	}

	#if defined(SGX_FEATURE_ZLS_EXTERNALZ)
	if (psKick->ui32ZLSCtl & EUR_CR_ISP_ZLSCTL_EXTERNALZBUFFER_MASK)
	{
		ui32TEPSG |= EUR_CR_TE_PSG_EXTERNALZBUFFER_MASK;
	}
	#endif
	#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	ui32TEPSG |= (psRTDataSet->ui32RgnPageStride << EUR_CR_TE_PSG_REGION_STRIDE_SHIFT);
	#endif

	fWClamp = 1.0e-6f;

#if defined(EUR_CR_TE_AA_GLOBALREGISTER_MASK)
	/* setup TE config for global sample positions */
	ui32TEAA |= EUR_CR_TE_AA_GLOBALREGISTER_MASK;
#endif

	if (psRTDataSet->ui16MSAASamplesInX == 2)
	{
		/* setup TE config for 2 samples per pixel in x */
		ui32TEAA |= EUR_CR_TE_AA_X_MASK;
	}

	if (psRTDataSet->ui16MSAASamplesInY == 2)
	{
		/* setup TE config for 2 samples per pixel in y */
		ui32TEAA |= EUR_CR_TE_AA_Y_MASK;
	}

	/*
		Setup the TA registers
	*/
	psTARegs->ui32TEAA		=	ui32TEAA;
	psTARegs->ui32TEMTile1	=	(psRTDataSet->ui32MTileNumber << EUR_CR_TE_MTILE1_NUMBER_SHIFT) |
								(psRTDataSet->ui32MTileX1 << EUR_CR_TE_MTILE1_X1_SHIFT) |
								(psRTDataSet->ui32MTileX2 << EUR_CR_TE_MTILE1_X2_SHIFT) |
								(psRTDataSet->ui32MTileX3 << EUR_CR_TE_MTILE1_X3_SHIFT);
	psTARegs->ui32TEMTile2	=	(psRTDataSet->ui32MTileY1 << EUR_CR_TE_MTILE2_Y1_SHIFT) |
								(psRTDataSet->ui32MTileY2 << EUR_CR_TE_MTILE2_Y2_SHIFT) |
								(psRTDataSet->ui32MTileY3 << EUR_CR_TE_MTILE2_Y3_SHIFT);
	psTARegs->ui32TEScreen	=	(psRTDataSet->ui32ScreenXMax << EUR_CR_TE_SCREEN_XMAX_SHIFT) |
								(psRTDataSet->ui32ScreenYMax << EUR_CR_TE_SCREEN_YMAX_SHIFT);

	psTARegs->ui32TEMTileStride		=	(psRTDataSet->ui32MTileStride << EUR_CR_TE_MTILE_STRIDE_SHIFT);
	psTARegs->ui32TEPSG				=	ui32TEPSG;
	for(i=0; i<SGX_FEATURE_MP_CORE_COUNT_TA; i++)
	{
		psTARegs->aui32TEPSGRgnBase[i]	=	psRTDataSet->psRTData[psRTDataSet->ui32RTDataSel].apsRgnHeaderClientMemInfo[i]->sDevVAddr.uiAddr;
		psTARegs->aui32TETPCBase[i]		=	psRTDataSet->apsTailPtrsClientMemInfo[i]->sDevVAddr.uiAddr;
	}
	psTARegs->ui32VDMCtrlStreamBase	=	psKick->sTABufferCtlStreamBase.uiAddr;
	psTARegs->ui32MTECtrl			=	psKick->ui32MTECtrl;
	psTARegs->ui32MTEWCompare		=	*((IMG_PUINT32)&fWClamp);
	psTARegs->ui32MTEWClamp			=	*((IMG_PUINT32)&fWClamp);
	psTARegs->ui32MTEScreen			=	((psRTDataSet->ui32NumPixelsX - 1) << EUR_CR_MTE_SCREEN_PIXXMAX_SHIFT) |
										((psRTDataSet->ui32NumPixelsY - 1) << EUR_CR_MTE_SCREEN_PIXYMAX_SHIFT);
#if defined(SGX545)
	#if defined(FIX_HW_BRN_26915)
	psTARegs->ui32GSGBaseAndStride	=	((psKick->ui32GSGBase << EUR_CR_GSG_BASE_ADDR_SHIFT) & EUR_CR_GSG_BASE_ADDR_MASK) |
										((psKick->ui32GSGStride << EUR_CR_GSG_STRIDE_SHIFT) & EUR_CR_GSG_STRIDE_MASK);
	#else
	psTARegs->ui32GSGBase			=	(psKick->ui32GSGBase << EUR_CR_GSG_BASE_ADDR_SHIFT) & EUR_CR_GSG_BASE_ADDR_MASK;
	psTARegs->ui32GSGStride			=	(psKick->ui32GSGStride << EUR_CR_GSG_STRIDE_SHIFT) & EUR_CR_GSG_STRIDE_MASK;
	#endif
	psTARegs->ui32GSGWrap			=	psKick->ui32GSGWrapAddr;
	psTARegs->ui32GSGStateStore		=	psKick->ui32GSGStateStoreBase;
	psTARegs->ui32MTE1stPhaseCompGeomBase	=	psKick->ui321stPhaseComplexGeomBufferBase;
	psTARegs->ui32MTE1stPhaseCompGeomSize	=	psKick->ui321stPhaseComplexGeomBufferSize;
	psTARegs->ui32VtxBufWritePointerPDSProg	=	(psKick->ui32VtxBufWritePointerPDSProgBaseAddr
												<< EUR_CR_VDM_VTXBUF_WRPTR_PDSPROG_BASE_ADDR_SHIFT) |
												(psKick->ui32VtxBufWritePointerPDSProgDataSize
												<< EUR_CR_VDM_VTXBUF_WRPTR_PDSPROG_DATASIZE_SHIFT);
	psTARegs->ui32SCWordCopyProgram		=	psKick->ui32SCWordCopyProgram;
	psTARegs->ui32ITPProgram			= 	psKick->ui32ITPProgram;
	#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	psTARegs->ui32TARTMax		=	((psRTDataSet->ui16NumRTsInArray-1) <<
									EUR_CR_DPM_TA_RENDER_TARGET_MAX_ID_SHIFT);
	psTARegs->ui32TETPC 		=	psRTDataSet->ui32TPCStride << EUR_CR_TE_TPC_STRIDE_SHIFT;
	#endif
#endif
	psTARegs->ui32MTEMSCtrl		=	psRTDataSet->ui32MTEMultiSampleCtl;
	psTARegs->ui32USELDRedirect	=	psKick->ui32USELoadRedirect;
	psTARegs->ui32USESTRange	=	psKick->ui32USEStoreRange;
	psTARegs->ui32BIFTAReqBase	=	(psPBDesc->sParamHeapBaseDevVAddr.uiAddr >> EUR_CR_BIF_TA_REQ_BASE_ADDR_ALIGNSHIFT) << EUR_CR_BIF_TA_REQ_BASE_ADDR_SHIFT;
#if defined(EUR_CR_MTE_FIXED_POINT)
	/* If the render target is greater than 2K in either direction switch mode */
	if (psRTDataSet->ui32NumPixelsX > 2048 || psRTDataSet->ui32NumPixelsY > 2048)
	{
		psTARegs->ui32MTEFixedPoint	=	EUR_CR_MTE_FIXED_POINT_FORMAT_MASK;
	}
	else
	{
		psTARegs->ui32MTEFixedPoint	=	0;
	}
#endif
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	#if defined(FIX_HW_BRN_33657) && defined(SUPPORT_SECURE_33657_FIX)
	if (psKick->ui32KickFlags & SGXMKIF_TAFLAGS_LASTKICK)
	{
		psTARegs->ui32TEPSG &= ~EUR_CR_TE_PSG_COMPLETEONTERMINATE_MASK;
	}
	#endif
#endif
}


/*!
 * *****************************************************************************
 * @brief Writes updated 3D register changes to the given array
 *
 * Checks if any of the 3D registers have changed and writes those
 * that have to the given array. The reg address is set to the offset
 * into the SGXMKIF_3DREGISTERS struct so the EDM code knows where
 * to copy the values
 *
 * @param psRenderDetails
 * @param psRegUpdates
 * @param psKick
 *
 * @return Number of regs
 ********************************************************************************/
static IMG_UINT32 Update3DRegs	(PSGX_RENDERDETAILS		psRenderDetails,
								 PVRSRV_HWREG			*psRegUpdates,
								 SGX_KICKTA_COMMON		*psKick,
								 SGX_RENDERCONTEXT		*psRenderContext)
{
	SGX_PBDESC			*psPBDesc;
	PSGXMKIF_3DREGISTERS	psHost3DRegs;
	IMG_UINT32				ui32ISPIFPMisc;
	IMG_UINT32				ui32ISPBgObj;
	PVRSRV_HWREG			*psRegsStart;
	IMG_UINT32 				ui32ZLSBase;
	IMG_UINT32 				ui32ZBase;
	IMG_UINT32 				ui32ISPStencilBase;
	IMG_UINT32				ui32StencilBase;
	IMG_DEV_VIRTADDR		s3DRegsDevAddr;
#if defined(EUR_CR_PDS_PP_INDEPENDANT_STATE)
	IMG_UINT32				ui32ISPTAGCtrl, ui32PDSPPState;
#endif
	psPBDesc = psRenderContext->psClientPBDesc->psPBDesc;

	psRegsStart = psRegUpdates;

	/* Get pointers to registers */
	psHost3DRegs = &psRenderDetails->s3DRegs;
	s3DRegsDevAddr.uiAddr = psRenderDetails->psHWRenderDetailsClientMemInfo->sDevVAddr.uiAddr + offsetof(SGXMKIF_HWRENDERDETAILS, s3DRegs);

#if defined(SGX_FEATURE_VISTEST_IN_MEMORY)
	if ((psKick->sVisTestResultMem.uiAddr != psHost3DRegs->aui32VisRegBase[0]) &&
		(psKick->ui32KickFlags & SGX_KICKTA_FLAGS_GETVISRESULTS))
	{
		IMG_UINT32	i;
		for (i = 0; i < SGX_FEATURE_MP_CORE_COUNT_3D; i++)
		{
			psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, aui32VisRegBase[i]);
			psRegUpdates->ui32RegVal = psKick->sVisTestResultMem.uiAddr + ((i * psKick->ui32VisTestCount) * sizeof(IMG_UINT32));
			psHost3DRegs->aui32VisRegBase[i] = psRegUpdates->ui32RegVal;
			psRegUpdates++;
		}
	}
#endif

	/* Check if the pixel back end state has changed */
	if (psKick->ui32EDMPixelPDSAddr != psHost3DRegs->ui32EDMPixelPDSExec)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32EDMPixelPDSExec);
		psRegUpdates->ui32RegVal = psKick->ui32EDMPixelPDSAddr;
		psRegUpdates++;
		psHost3DRegs->ui32EDMPixelPDSExec = psKick->ui32EDMPixelPDSAddr;
	}
	if (psKick->ui32EDMPixelPDSDataSize != psHost3DRegs->ui32EDMPixelPDSData)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32EDMPixelPDSData);
		psRegUpdates->ui32RegVal = psKick->ui32EDMPixelPDSDataSize;
		psRegUpdates++;
		psHost3DRegs->ui32EDMPixelPDSData = psKick->ui32EDMPixelPDSDataSize;
	}
	if (psKick->ui32EDMPixelPDSInfo != psHost3DRegs->ui32EDMPixelPDSInfo)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32EDMPixelPDSInfo);
		psRegUpdates->ui32RegVal = psKick->ui32EDMPixelPDSInfo;
		psRegUpdates++;
		psHost3DRegs->ui32EDMPixelPDSInfo = psKick->ui32EDMPixelPDSInfo;
	}

#if defined(SUPPORT_DISPLAYCONTROLLER_TILING)
	if (psKick->ui32BIFTile0Config != psHost3DRegs->ui32BIFTile0Config)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32BIFTile0Config);
		psRegUpdates->ui32RegVal = psKick->ui32BIFTile0Config;
		psRegUpdates++;
		psHost3DRegs->ui32BIFTile0Config = psKick->ui32BIFTile0Config;
	}
#endif

	/* Check if Process Empty Regions setting or ISP valid ID has changed */
	ui32ISPIFPMisc = psKick->ui32ISPIPFMisc | (psKick->ui32ISPValidId << EUR_CR_ISP_IPFMISC_VALIDID_SHIFT);
	if (ui32ISPIFPMisc != psHost3DRegs->ui32ISPIPFMisc)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32ISPIPFMisc);
		psRegUpdates->ui32RegVal = ui32ISPIFPMisc;
		psRegUpdates++;
		psHost3DRegs->ui32ISPIPFMisc = ui32ISPIFPMisc;
	}

	/* Check if Background Object Control setting has changed. */
	ui32ISPBgObj = psKick->ui32ISPBGObj | psKick->ui32ISPSceneBGObj;
	if (ui32ISPBgObj != psHost3DRegs->ui32ISPBgObj)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32ISPBgObj);
		psRegUpdates->ui32RegVal = ui32ISPBgObj;
		psRegUpdates++;
		psHost3DRegs->ui32ISPBgObj = ui32ISPBgObj;
	}

	/* Check if depth bias adjusts have changed. */
#if defined(SGX_FEATURE_DEPTH_BIAS_OBJECTS)
	if (psKick->ui32ISPDBias != psHost3DRegs->ui32ISPDBias[0])
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + (IMG_UINT32)offsetof(SGXMKIF_3DREGISTERS, ui32ISPDBias[0]);
		psRegUpdates->ui32RegVal = psKick->ui32ISPDBias;
		psRegUpdates++;
		psHost3DRegs->ui32ISPDBias[0] = psKick->ui32ISPDBias;
	}
	if (psKick->ui32ISPDBias1 != psHost3DRegs->ui32ISPDBias[1])
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + (IMG_UINT32)offsetof(SGXMKIF_3DREGISTERS, ui32ISPDBias[1]);
		psRegUpdates->ui32RegVal = psKick->ui32ISPDBias1;
		psRegUpdates++;
		psHost3DRegs->ui32ISPDBias[1] = psKick->ui32ISPDBias1;
	}
	if (psKick->ui32ISPDBias2 != psHost3DRegs->ui32ISPDBias[2])
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + (IMG_UINT32)offsetof(SGXMKIF_3DREGISTERS, ui32ISPDBias[2]);
		psRegUpdates->ui32RegVal = psKick->ui32ISPDBias2;
		psRegUpdates++;
		psHost3DRegs->ui32ISPDBias[2] = psKick->ui32ISPDBias2;
	}
#else
	if (psKick->ui32ISPDBias != psHost3DRegs->ui32ISPDBias[0])
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32ISPDBias);
		psRegUpdates->ui32RegVal = psKick->ui32ISPDBias;
		psRegUpdates++;
		psHost3DRegs->ui32ISPDBias[0] = psKick->ui32ISPDBias;
	}
#endif

	/* Check if the zls control register has changed. */
	if (psKick->ui32ZLSCtl != psHost3DRegs->ui32ISPZLSCtl)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32ISPZLSCtl);
		psRegUpdates->ui32RegVal = psKick->ui32ZLSCtl;
		psRegUpdates++;
		psHost3DRegs->ui32ISPZLSCtl = psKick->ui32ZLSCtl;
	}

	/* check for Z buffer changes */
#if defined(EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_CMPR_F32ZI8S1V)
	if( ((psKick->ui32ZLSCtl & EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_MASK) >= EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_CMPR_F32ZI8S1V) &&
		((psKick->ui32ZLSCtl & EUR_CR_ISP_ZLSCTL_EXTERNALZBUFFER_MASK) == 0))
#else
	if ( ((psKick->ui32ZLSCtl & EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_MASK) == EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_F32ZI8S1V)
	#if defined(SGX_FEATURE_ZLS_EXTERNALZ)
		&& ((psKick->ui32ZLSCtl & EUR_CR_ISP_ZLSCTL_EXTERNALZBUFFER_MASK) == 0)
	#endif
		)
#endif
	{
		ui32ZLSBase		= (psPBDesc->sParamHeapBaseDevVAddr.uiAddr >> EUR_CR_BIF_ZLS_REQ_BASE_ADDR_ALIGNSHIFT) << EUR_CR_BIF_ZLS_REQ_BASE_ADDR_SHIFT;
	}
	else
	{
		if((psKick->sISPZLoadBase.uiAddr != 0) && (psKick->sISPZLoadBase.uiAddr < psKick->sISPZStoreBase.uiAddr))
		{
			ui32ZLSBase	= psKick->sISPZLoadBase.uiAddr;
		}
		else
		{
			ui32ZLSBase	= psKick->sISPZStoreBase.uiAddr;
		}

		ui32StencilBase = psKick->sISPStencilLoadBase.uiAddr;

		if((ui32StencilBase != 0) && ((ui32ZLSBase == 0) || (ui32StencilBase < ui32ZLSBase)))
		{
			ui32ZLSBase	= ui32StencilBase;
		}

		ui32StencilBase = psKick->sISPStencilStoreBase.uiAddr;

		if((ui32StencilBase != 0) && ((ui32ZLSBase == 0) || (ui32StencilBase < ui32ZLSBase)))
		{
			ui32ZLSBase = ui32StencilBase;
		}

		ui32ZLSBase	= (ui32ZLSBase >> EUR_CR_BIF_ZLS_REQ_BASE_ADDR_ALIGNSHIFT) << EUR_CR_BIF_ZLS_REQ_BASE_ADDR_SHIFT;
	}

	if(ui32ZLSBase != psHost3DRegs->ui32BIFZLSReqBase)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32BIFZLSReqBase);
		psRegUpdates->ui32RegVal = ui32ZLSBase;
		psRegUpdates++;
		psHost3DRegs->ui32BIFZLSReqBase = ui32ZLSBase;
	}

	ui32ZBase = (psKick->sISPZLoadBase.uiAddr - ui32ZLSBase) & EUR_CR_ISP_ZLOAD_BASE_ADDR_MASK;

	if(ui32ZBase != psHost3DRegs->ui32ISPZLoadBase)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32ISPZLoadBase);
		psRegUpdates->ui32RegVal = ui32ZBase;
		psRegUpdates++;
		psHost3DRegs->ui32ISPZLoadBase = ui32ZBase;
	}

	ui32ZBase = (psKick->sISPZStoreBase.uiAddr - ui32ZLSBase) & EUR_CR_ISP_ZSTORE_BASE_ADDR_MASK;

	if(ui32ZBase != psHost3DRegs->ui32ISPZStoreBase)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32ISPZStoreBase);
		psRegUpdates->ui32RegVal = ui32ZBase;
		psRegUpdates++;
		psHost3DRegs->ui32ISPZStoreBase = ui32ZBase;
	}

	/* check for stencil changes */
	ui32ISPStencilBase = ((psKick->sISPStencilLoadBase.uiAddr - ui32ZLSBase) & EUR_CR_ISP_STENCIL_LOAD_BASE_ADDR_MASK)
							| (psKick->bSeperateStencilLoadStore ? EUR_CR_ISP_STENCIL_LOAD_BASE_ENABLE_MASK : 0);
	if(ui32ISPStencilBase != psHost3DRegs->ui32ISPStencilLoadBase)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32ISPStencilLoadBase);
		psRegUpdates->ui32RegVal = ui32ISPStencilBase;
		psRegUpdates++;
		psHost3DRegs->ui32ISPStencilLoadBase = ui32ISPStencilBase;
	}

	ui32ISPStencilBase = ((psKick->sISPStencilStoreBase.uiAddr - ui32ZLSBase) & EUR_CR_ISP_STENCIL_STORE_BASE_ADDR_MASK)
							| (psKick->bSeperateStencilLoadStore ? EUR_CR_ISP_STENCIL_STORE_BASE_ENABLE_MASK : 0);
	if(ui32ISPStencilBase != psHost3DRegs->ui32ISPStencilStoreBase)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32ISPStencilStoreBase);
		psRegUpdates->ui32RegVal = ui32ISPStencilBase;
		psRegUpdates++;
		psHost3DRegs->ui32ISPStencilStoreBase = ui32ISPStencilBase;
	}

#if defined(EUR_CR_DOUBLE_PIXEL_PARTITIONS)
	if (psKick->ui32DoublePixelPartitions != psHost3DRegs->ui32DoublePixelPartitions)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32DoublePixelPartitions);
		psRegUpdates->ui32RegVal = psKick->ui32DoublePixelPartitions;
		psRegUpdates++;
		psHost3DRegs->ui32DoublePixelPartitions = psKick->ui32DoublePixelPartitions;
	}
#endif

#if defined(EUR_CR_PDS_PP_INDEPENDANT_STATE)
	ui32ISPTAGCtrl	=	(IMG_UINT32)((psRenderContext->bForcePTOff) ? EUR_CR_ISP_TAGCTRL_FORCE_PT_OFF_MASK : 0UL);

#if defined(FIX_HW_BRN_23141) || defined(FIX_HW_BRN_22656) || defined(FIX_HW_BRN_22849)
	ui32ISPTAGCtrl	|=	EUR_CR_ISP_TAGCTRL_SLOWFPT_MASK;
#endif

	if(psKick->ui32TriangleSplitPixelThreshold)
	{
		ui32ISPTAGCtrl |= (psKick->ui32TriangleSplitPixelThreshold << EUR_CR_ISP_TAGCTRL_TRIANGLE_FRAGMENT_PIXELS_SHIFT);
		ui32PDSPPState = 0;
	}
	else
	{
		ui32ISPTAGCtrl |= EUR_CR_ISP_TAGCTRL_TRIANGLE_FRAGMENT_DISABLE_MASK;
		ui32PDSPPState = EUR_CR_PDS_PP_INDEPENDANT_STATE_DISABLE_MASK;
	}

	if (ui32ISPTAGCtrl != psHost3DRegs->ui32ISPTAGCtrl)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32ISPTAGCtrl);
		psRegUpdates->ui32RegVal = ui32ISPTAGCtrl;
		psRegUpdates++;
		psHost3DRegs->ui32ISPTAGCtrl = ui32ISPTAGCtrl;
	}

	if (ui32PDSPPState != psHost3DRegs->ui32PDSPPState)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32PDSPPState);
		psRegUpdates->ui32RegVal = ui32PDSPPState;
		psRegUpdates++;
		psHost3DRegs->ui32PDSPPState = ui32PDSPPState;
	}

#endif

	/* Update FIRH coeffs */
	if (psKick->ui32Filter0Left != psHost3DRegs->ui32USEFilter0Left)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32USEFilter0Left);
		psRegUpdates->ui32RegVal = psKick->ui32Filter0Left;
		psRegUpdates++;
		psHost3DRegs->ui32USEFilter0Left = psKick->ui32Filter0Left;
	}
#if defined(SGX_FEATURE_USE_HIGH_PRECISION_FIR_COEFF)
	if (psKick->ui32Filter0Centre != psHost3DRegs->ui32USEFilter0Centre)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32USEFilter0Centre);
		psRegUpdates->ui32RegVal = psKick->ui32Filter0Centre;
		psRegUpdates++;
		psHost3DRegs->ui32USEFilter0Centre = psKick->ui32Filter0Centre;
	}
#endif
	if (psKick->ui32Filter0Right != psHost3DRegs->ui32USEFilter0Right)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32USEFilter0Right);
		psRegUpdates->ui32RegVal = psKick->ui32Filter0Right;
		psRegUpdates++;
		psHost3DRegs->ui32USEFilter0Right = psKick->ui32Filter0Right;
	}
	if (psKick->ui32Filter0Extra != psHost3DRegs->ui32USEFilter0Extra)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32USEFilter0Extra);
		psRegUpdates->ui32RegVal = psKick->ui32Filter0Extra;
		psRegUpdates++;
		psHost3DRegs->ui32USEFilter0Extra = psKick->ui32Filter0Extra;
	}
	if (psKick->ui32Filter1Left != psHost3DRegs->ui32USEFilter1Left)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32USEFilter1Left);
		psRegUpdates->ui32RegVal = psKick->ui32Filter1Left;
		psRegUpdates++;
		psHost3DRegs->ui32USEFilter1Left = psKick->ui32Filter1Left;
	}
#if defined(SGX_FEATURE_USE_HIGH_PRECISION_FIR_COEFF)
	if (psKick->ui32Filter1Centre != psHost3DRegs->ui32USEFilter1Centre)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32USEFilter1Centre);
		psRegUpdates->ui32RegVal = psKick->ui32Filter1Centre;
		psRegUpdates++;
		psHost3DRegs->ui32USEFilter1Centre = psKick->ui32Filter1Centre;
	}
#endif
	if (psKick->ui32Filter1Right != psHost3DRegs->ui32USEFilter1Right)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32USEFilter1Right);
		psRegUpdates->ui32RegVal = psKick->ui32Filter1Right;
		psRegUpdates++;
		psHost3DRegs->ui32USEFilter1Right = psKick->ui32Filter1Right;
	}
	if (psKick->ui32Filter1Extra != psHost3DRegs->ui32USEFilter1Extra)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32USEFilter1Extra);
		psRegUpdates->ui32RegVal = psKick->ui32Filter1Extra;
		psRegUpdates++;
		psHost3DRegs->ui32USEFilter1Extra = psKick->ui32Filter1Extra;
	}
	if (psKick->ui32Filter2Left != psHost3DRegs->ui32USEFilter2Left)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32USEFilter2Left);
		psRegUpdates->ui32RegVal = psKick->ui32Filter2Left;
		psRegUpdates++;
		psHost3DRegs->ui32USEFilter2Left = psKick->ui32Filter2Left;
	}
	if (psKick->ui32Filter2Right != psHost3DRegs->ui32USEFilter2Right)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32USEFilter2Right);
		psRegUpdates->ui32RegVal = psKick->ui32Filter2Right;
		psRegUpdates++;
		psHost3DRegs->ui32USEFilter2Right = psKick->ui32Filter2Right;
	}
	if (psKick->ui32Filter2Extra != psHost3DRegs->ui32USEFilter2Extra)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32USEFilter2Extra);
		psRegUpdates->ui32RegVal = psKick->ui32Filter2Extra;
		psRegUpdates++;
		psHost3DRegs->ui32USEFilter2Extra = psKick->ui32Filter2Extra;
	}
	if (psKick->ui32FilterTable != psHost3DRegs->ui32USEFilterTable)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32USEFilterTable);
		psRegUpdates->ui32RegVal = psKick->ui32FilterTable;
		psRegUpdates++;
		psHost3DRegs->ui32USEFilterTable = psKick->ui32FilterTable;
	}

#if defined(SGX_FEATURE_TAG_YUV_TO_RGB)
	if (psKick->ui32CSC0Coeff01 != psHost3DRegs->ui32CSC0Coeff01)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32CSC0Coeff01);
		psRegUpdates->ui32RegVal = psKick->ui32CSC0Coeff01;
		psRegUpdates++;
		psHost3DRegs->ui32CSC0Coeff01 = psKick->ui32CSC0Coeff01;
	}
	if (psKick->ui32CSC0Coeff23 != psHost3DRegs->ui32CSC0Coeff23)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32CSC0Coeff23);
		psRegUpdates->ui32RegVal = psKick->ui32CSC0Coeff23;
		psRegUpdates++;
		psHost3DRegs->ui32CSC0Coeff23 = psKick->ui32CSC0Coeff23;
	}
	if (psKick->ui32CSC0Coeff45 != psHost3DRegs->ui32CSC0Coeff45)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32CSC0Coeff45);
		psRegUpdates->ui32RegVal = psKick->ui32CSC0Coeff45;
		psRegUpdates++;
		psHost3DRegs->ui32CSC0Coeff45 = psKick->ui32CSC0Coeff45;
	}
	if (psKick->ui32CSC0Coeff67 != psHost3DRegs->ui32CSC0Coeff67)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32CSC0Coeff67);
		psRegUpdates->ui32RegVal = psKick->ui32CSC0Coeff67;
		psRegUpdates++;
		psHost3DRegs->ui32CSC0Coeff67 = psKick->ui32CSC0Coeff67;
	}
	if (psKick->ui32CSC1Coeff01 != psHost3DRegs->ui32CSC1Coeff01)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32CSC1Coeff01);
		psRegUpdates->ui32RegVal = psKick->ui32CSC1Coeff01;
		psRegUpdates++;
		psHost3DRegs->ui32CSC1Coeff01 = psKick->ui32CSC1Coeff01;
	}
	if (psKick->ui32CSC1Coeff23 != psHost3DRegs->ui32CSC1Coeff23)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32CSC1Coeff23);
		psRegUpdates->ui32RegVal = psKick->ui32CSC1Coeff23;
		psRegUpdates++;
		psHost3DRegs->ui32CSC1Coeff23 = psKick->ui32CSC1Coeff23;
	}
	if (psKick->ui32CSC1Coeff45 != psHost3DRegs->ui32CSC1Coeff45)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32CSC1Coeff45);
		psRegUpdates->ui32RegVal = psKick->ui32CSC1Coeff45;
		psRegUpdates++;
		psHost3DRegs->ui32CSC1Coeff45 = psKick->ui32CSC1Coeff45;
	}
	if (psKick->ui32CSC1Coeff67 != psHost3DRegs->ui32CSC1Coeff67)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32CSC1Coeff67);
		psRegUpdates->ui32RegVal = psKick->ui32CSC1Coeff67;
		psRegUpdates++;
		psHost3DRegs->ui32CSC1Coeff67 = psKick->ui32CSC1Coeff67;
	}

#if defined(SGX_FEATURE_TAG_YUV_TO_RGB_HIPREC)
	if (psKick->ui32CSC0Coeff89 != psHost3DRegs->ui32CSC0Coeff89)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32CSC0Coeff89);
		psRegUpdates->ui32RegVal = psKick->ui32CSC0Coeff89;
		psRegUpdates++;
		psHost3DRegs->ui32CSC0Coeff89 = psKick->ui32CSC0Coeff89;
	}
	if (psKick->ui32CSC0Coeff1011 != psHost3DRegs->ui32CSC0Coeff1011)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32CSC0Coeff1011);
		psRegUpdates->ui32RegVal = psKick->ui32CSC0Coeff1011;
		psRegUpdates++;
		psHost3DRegs->ui32CSC0Coeff1011 = psKick->ui32CSC0Coeff1011;
	}
	if (psKick->ui32CSC1Coeff89 != psHost3DRegs->ui32CSC1Coeff89)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32CSC1Coeff89);
		psRegUpdates->ui32RegVal = psKick->ui32CSC1Coeff89;
		psRegUpdates++;
		psHost3DRegs->ui32CSC1Coeff89 = psKick->ui32CSC1Coeff89;
	}
	if (psKick->ui32CSC1Coeff1011 != psHost3DRegs->ui32CSC1Coeff1011)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32CSC1Coeff1011);
		psRegUpdates->ui32RegVal = psKick->ui32CSC1Coeff1011;
		psRegUpdates++;
		psHost3DRegs->ui32CSC1Coeff1011 = psKick->ui32CSC1Coeff1011;
	}

#else
	if (psKick->ui32CSC0Coeff8 != psHost3DRegs->ui32CSC0Coeff8)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32CSC0Coeff8);
		psRegUpdates->ui32RegVal = psKick->ui32CSC0Coeff8;
		psRegUpdates++;
		psHost3DRegs->ui32CSC0Coeff8 = psKick->ui32CSC0Coeff8;
	}
	if (psKick->ui32CSC1Coeff8 != psHost3DRegs->ui32CSC1Coeff8)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32CSC1Coeff8);
		psRegUpdates->ui32RegVal = psKick->ui32CSC1Coeff8;
		psRegUpdates++;
		psHost3DRegs->ui32CSC1Coeff8 = psKick->ui32CSC1Coeff8;
	}
	if (psKick->ui32CSCScale != psHost3DRegs->ui32CSCScale)
	{
		psRegUpdates->ui32RegAddr = s3DRegsDevAddr.uiAddr + offsetof(SGXMKIF_3DREGISTERS, ui32CSCScale);
		psRegUpdates->ui32RegVal = psKick->ui32CSCScale;
		psRegUpdates++;
		psHost3DRegs->ui32CSCScale = psKick->ui32CSCScale;
	}

#endif /* SGX_FEATURE_TAG_YUV_TO_RGB_HIPREC */

#endif /* SGX_FEATURE_TAG_YUV_TO_RGB */


	/*
		Check the number of registers written hasn't grown since the function
		was orginally written.
	*/
	PVR_ASSERT((psRegUpdates - psRegsStart) <= SGX_NUM_3D_REGS_UPDATED);

	return (IMG_UINT32)(psRegUpdates - psRegsStart);
}


/*!
 * *****************************************************************************
 * @brief Get next RTData for TA.
 *
 * @param psDevData
 * @param psRTDataSet
 *
 * @return
 ********************************************************************************/
static IMG_VOID GetNextRTData(PVRSRV_DEV_DATA	*psDevData,
							  PSGX_RTDATASET	psRTDataSet)
{
#if defined(DEBUG)
	PSGXMKIF_HWRTDATA		psHWRTData;
#endif

	psRTDataSet->ui32RTDataSel = (psRTDataSet->ui32RTDataSel + 1) % psRTDataSet->ui32NumRTData;

	PVR_UNREFERENCED_PARAMETER(psDevData);

#if defined (DEBUG)
	psHWRTData = &psRTDataSet->psHWRTDataSet->asHWRTData[psRTDataSet->ui32RTDataSel];
	if (psHWRTData->ui32CommonStatus & SGXMKIF_HWRTDATA_CMN_STATUS_ERROR_MASK)
	{
		IMG_UINT32 ui32FrameNum;
		ui32FrameNum = (psHWRTData->ui32CommonStatus >> SGXMKIF_HWRTDATA_CMN_STATUS_FRAMENUM_SHIFT);

		/* Check to see why we timed out */
		if (psHWRTData->ui32CommonStatus & SGXMKIF_HWRTDATA_CMN_STATUS_RENDERTIMEOUT)
		{
			PVR_DPF((PVR_DBG_ERROR, "Render Timeout! LastFrame: %d", ui32FrameNum));
		}
		else if (psHWRTData->ui32CommonStatus & SGXMKIF_HWRTDATA_CMN_STATUS_TATIMEOUT)
		{
			PVR_DPF((PVR_DBG_ERROR, "TA Timeout! LastFrame: %d", ui32FrameNum));
		}
	}
	else if (psHWRTData->ui32CommonStatus & SGXMKIF_HWRTDATA_RT_STATUS_ISPCTXSWITCH)
	{
		PVR_DPF((PVR_DBG_MESSAGE, "RTDataSet: 0x%x was ISP context switched", (IMG_UINTPTR_T)psRTDataSet));
	}
	else if (psHWRTData->ui32CommonStatus & SGXMKIF_HWRTDATA_RT_STATUS_VDMCTXSWITCH)
	{
		PVR_DPF((PVR_DBG_MESSAGE, "RTDataSet: 0x%x was VDM context switched", (IMG_UINTPTR_T)psRTDataSet));
	}
#endif /* DEBUG */
}


/*!
 * *****************************************************************************
 * @brief Find a free render details structure in the list.
 *
 * @param psDevData
 * @param psRTDataSet
 * @param ppsRenderDetails
 *
 * @return ui32Error - success or failure
 ********************************************************************************/
static IMG_VOID SGXGetFreeRenderDetails(PVRSRV_DEV_DATA				*psDevData,
											PSGX_RENDERCONTEXT		psRenderContext,
											PSGX_RTDATASET			psRTDataSet,
											PSGX_RENDERDETAILS		*ppsRenderDetails)
{
	PSGX_RENDERDETAILS		psRenderDetails;

#if defined(PDUMP) && defined(NO_HARDWARE)
	if (psRTDataSet->psLastFreedRenderDetails && psRTDataSet->psLastFreedRenderDetails->psNext)
	{
		/*
			The purpose of this if statement is to try and make
			use of different render details in parameter dumps in
			order to increase the chances that we'll be able to get
			the TA and render running in parallel
		*/
		psRenderDetails = psRTDataSet->psLastFreedRenderDetails->psNext;
	}
	else
#endif
	{
		psRenderDetails = psRTDataSet->psRenderDetailsList;
	}

	for (;;)
	{
		while (psRenderDetails != IMG_NULL)
		{
			if(*psRenderDetails->pui32Lock == 0)
			{
				*psRenderDetails->pui32Lock = 1;

				goto FoundSpace;
			}
			psRenderDetails = psRenderDetails->psNext;
		}

		if(psRenderContext->hOSEvent)
		{
			if(PVRSRVEventObjectWait(psDevData->psConnection,
									 psRenderContext->hOSEvent) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_MESSAGE, "SGXGetFreeRenderDetails: PVRSRVEventObjectWait failed"));
			}
		}

		/* Start the loop again at the head of the list */
		psRenderDetails = psRTDataSet->psRenderDetailsList;
	}

FoundSpace:
	*ppsRenderDetails = psRenderDetails;
}

/*!
 * *****************************************************************************
 * @brief Find a free Device sync list structure in the list.
 *
 * @param psDevData
 * @param psRenderContext
 * @param psRTDataSet
 * @param ppsDeviceSyncList
 *
 * @return ui32Error - success or failure
 ********************************************************************************/
static IMG_VOID SGXGetFreeDeviceSyncList(PVRSRV_DEV_DATA				*psDevData,
											PSGX_RENDERCONTEXT		psRenderContext,
											PSGX_RTDATASET			psRTDataSet,
											PSGX_DEVICE_SYNC_LIST		*ppsDeviceSyncList)
{
	PSGX_DEVICE_SYNC_LIST		psDeviceSyncList;

#if defined(PDUMP) && defined(NO_HARDWARE)
	if (psRTDataSet->psLastFreedDeviceSyncList && psRTDataSet->psLastFreedDeviceSyncList->psNext)
	{
		/*
			The purpose of this if statement is to try and make
			use of different render details in parameter dumps in
			order to increase the chances that we'll be able to get
			the TA and render running in parallel
		*/
		psDeviceSyncList = psRTDataSet->psLastFreedDeviceSyncList->psNext;
	}
	else
#endif
	{
		psDeviceSyncList = psRTDataSet->psDeviceSyncList;
	}

	for (;;)
	{
		while (psDeviceSyncList != IMG_NULL)
		{
			if(*psDeviceSyncList->pui32Lock == 0)
			{
				*psDeviceSyncList->pui32Lock = 1;

				goto SyncListFound;
			}
			psDeviceSyncList = psDeviceSyncList->psNext;
		}

		if(psRenderContext->hOSEvent)
		{
			if(PVRSRVEventObjectWait(psDevData->psConnection,
									 psRenderContext->hOSEvent) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_MESSAGE, "SGXGetFreeDeviceSyncList: PVRSRVEventObjectWait failed"));
			}
		}

		/* Start the loop again at the head of the list */
		psDeviceSyncList = psRTDataSet->psDeviceSyncList;
	}

SyncListFound:
	*ppsDeviceSyncList = psDeviceSyncList;
}

#if !defined(PDUMP) && defined(DEBUG)
/*!
 * *****************************************************************************
 * @brief Repeatedly poll sync op completion status until all sync ops complete
 *        NB:  we expect this to be called in single threaded context.  Especially
 *             we don't expect the pending count to be advanced while this function
 *             is being executed.  TODO: can we assert that, somehow?
 *
 * @param psDevData
 * @param psRenderContext
 * @param ui32Waitus
 * @param ui32Tries
 *
 * @return PVRSRV_ERROR - success or failure
 ********************************************************************************/
static PVRSRV_ERROR WaitForRender(const PVRSRV_DEV_DATA *psDevData,
								  SGX_RENDERCONTEXT *psRenderContext,
#if defined (SUPPORT_SID_INTERFACE)
								  IMG_SID    hSyncInfo,
#else
								  PVRSRV_CLIENT_SYNC_INFO *psSyncInfo,
#endif
								  IMG_UINT32 ui32Waitus,
								  IMG_UINT32 ui32Tries)
{
#if defined (SUPPORT_SID_INTERFACE)
	IMG_EVENTSID   hEventObject;
#else
	IMG_HANDLE hEventObject;
#endif
	IMG_BOOL          bUseEventObject;
	IMG_UINT32        uiStart = 0;
	PVRSRV_ERROR      eError;
	PVRSRV_SYNC_TOKEN _sSyncToken, *psSyncToken = &_sSyncToken;

	/* Use event object if available */
	hEventObject = psRenderContext->hOSEvent;
	bUseEventObject = (IMG_BOOL)(hEventObject != IMG_NULL);

	if (!bUseEventObject)
	{
		uiStart = PVRSRVClockus();
	}

	eError = PVRSRVSyncOpsTakeToken(psDevData->psConnection,
#if defined (SUPPORT_SID_INTERFACE)
									hSyncInfo,
#else
									psSyncInfo,
#endif
									psSyncToken);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "FlushClientOps: failed to acquire token"));
		return eError;
	}


	for(;;)
	{
#if defined (SUPPORT_SID_INTERFACE)
		eError = PVRSRVSyncOpsFlushToToken(psDevData->psConnection, hSyncInfo, psSyncToken, IMG_FALSE);
#else
		eError = PVRSRVSyncOpsFlushToToken(psDevData->psConnection, psSyncInfo, psSyncToken, IMG_FALSE);
#endif

		if(eError != PVRSRV_ERROR_RETRY)
		{
			break;
		}

		if(bUseEventObject)
		{
			if(!ui32Tries)
			{
				return PVRSRV_ERROR_TIMEOUT_POLLING_FOR_VALUE;
			}

			ui32Tries--;

			if(PVRSRVEventObjectWait(psDevData->psConnection,
									 hEventObject) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "WaitForRender: PVRSRVEventObjectWait failed"));
			}
		}
		else
		{
			if ((PVRSRVClockus() - uiStart) > (ui32Tries * ui32Waitus))
			{
				eError = PVRSRV_ERROR_TIMEOUT_POLLING_FOR_VALUE;
				break;
			}
			PVRSRVWaitus(ui32Waitus);
		}
	}

	return eError;
}
#endif

/*!
 * *****************************************************************************
 * @brief Prepares to kick the TA and kicks the TA if pvKickSubmit is IMG_NULL
 *
 * @param psDevData  pointer to device info
 * @param psKickUM  pointer to the kick info
 * @param psKickOutput
 * @param pvKickPDUMP  PDUMP only data
 * @param pvKickSubmit  Vista only, pass IMG_NULL for everything else. Used to split
 * 						construction and submission of CCB.
 *
 * @return ui32Error - success or failure
 ********************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV SGXKickTA(PVRSRV_DEV_DATA			*psDevData,
									SGX_KICKTA				*psKickTA,
									SGX_KICKTA_OUTPUT		*psKickOutput,
									IMG_PVOID				pvKickPDUMP,
									IMG_PVOID				pvKickSubmit)
{
	PVRSRV_ERROR				eError = PVRSRV_OK;	/* PRQA S 3197 */ /* init is required for some builds */
	SGX_KICKTA_COMMON			*psKick = &psKickTA->sKickTACommon;
	PSGX_RENDERCONTEXT			psRenderContext;
	SGX_PBDESC					*psPBDesc;
	SGX_CLIENTPBDESC			*psClientPBDesc;
	PSGX_KICKTA_SUBMIT			psSubmit;
	PSGX_RTDATASET				psRTDataSet;
	PSGXMKIF_CMDTA				psTACmd;
	PSGX_RENDERDETAILS			psRenderDetails;
	PSGXMKIF_HWRENDERDETAILS	psHWRenderDetails;
	PSGX_DEVICE_SYNC_LIST		psDstDeviceSyncList;
#if !defined(SGX_PREP_SUBMIT_SEPERATE)
	SGX_KICKTA_SUBMIT			sKickSubmit;
	/* See above for description of this array size */
	IMG_UINT32					aui32Temp[SGX_TEMP_CMD_BUF_SIZE];
#endif
	IMG_UINT32					ui32CmdFlags;
	IMG_UINT32					ui32RenderFlags;
	IMG_UINT32					ui32Num3DRegs;
	IMG_UINT32					i;
	PVRSRV_CLIENT_MEM_INFO		*psHWPBDescClientMemInfo;
	SGXTQ_CLIENT_TRANSFER_CONTEXT	*psTransferContext;

#if defined(PDUMP)
	PSGX_KICKTA_PDUMP psKickPDUMP = (PSGX_KICKTA_PDUMP) pvKickPDUMP;
#else
	PVR_UNREFERENCED_PARAMETER(pvKickPDUMP);
#endif

	psRTDataSet = (SGX_RTDATASET *)psKickTA->hRTDataSet;
	if (psRTDataSet == IMG_NULL)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	PVRSRVLockMutex(psRTDataSet->hMutex);

	psRenderContext = (SGX_RENDERCONTEXT *)psKickTA->hRenderContext;
	psClientPBDesc = psRenderContext->psClientPBDesc;
	psPBDesc = psClientPBDesc->psPBDesc;

#if defined(SGX_PREP_SUBMIT_SEPERATE)
	/*
		When prepare and submit are seperate, the caller must supply
		the memory for the submit structure and the TA command
	*/
	psSubmit = (SGX_KICKTA_SUBMIT *) pvKickSubmit;
	psTACmd = (SGXMKIF_CMDTA *) psSubmit->pvTACmd;
#else /* #if defined(SGX_PREP_SUBMIT_SEPERATE) */
	PVR_UNREFERENCED_PARAMETER(pvKickSubmit);

	/* Use the local variables from the stack */
	psSubmit = &sKickSubmit;
	PVRSRVMemSet(psSubmit, 0, sizeof(SGX_KICKTA_SUBMIT));
	
	/* Pickup temp command buffer */
	psTACmd = (PSGXMKIF_CMDTA) aui32Temp;
#endif /* #if defined(SGX_PREP_SUBMIT_SEPERATE) */

	/* Clear the temp command memory */
	PVRSRVMemSet (psTACmd, 0, sizeof(*psTACmd));

	/* Initialise psKickOutput structure */
	PVRSRVMemSet (psKickOutput, 0, sizeof(SGX_KICKTA_OUTPUT));

	/* Setup command flags.. */
	ui32CmdFlags = 0;

	/* Fill in the KickOutput structure */
	psKickOutput->bSPMOutOfMemEvent = IMG_FALSE;
	for ( i = 0; i < psRTDataSet->ui32NumRTData; i++)
	{
		IMG_UINT32	ui32Status = psRTDataSet->psHWRTDataSet->asHWRTData[i].ui32CommonStatus;
		if ((ui32Status & SGXMKIF_HWRTDATA_CMN_STATUS_OUTOFMEM) != 0)
		{
			psKickOutput->bSPMOutOfMemEvent = IMG_TRUE;
			psKickOutput->ui32MaxZTileX = 2 * psRTDataSet->ui32MTileX2;
			psKickOutput->ui32MaxZTileY = 2 * psRTDataSet->ui32MTileY2;
			break;
		}
	}

#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
	/* Check whether the PB needs Growing or shrinking */
	HandlePBGrowShrink(psDevData, psRenderContext, psKickOutput->bSPMOutOfMemEvent);
#endif

	if (psKick->ui32KickFlags & (SGX_KICKTA_FLAGS_FIRSTTAPROD |
								SGX_KICKTA_FLAGS_RENDEREDMIDSCENE))
	{
		psDstDeviceSyncList = IMG_NULL;

		if (psKickTA->ppsRenderSurfSyncInfo != IMG_NULL)
		{
			/* Get a free device sync list. */
			SGXGetFreeDeviceSyncList(psDevData, psRenderContext,
											  psRTDataSet, &psDstDeviceSyncList);
		}
		/* Save DeviceSyncList into RT data set. */
		psRTDataSet->psCurrentDeviceSyncList = psDstDeviceSyncList;

#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
		if ( ((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_FIRSTTAPROD) != 0) ||
			(((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_RENDEREDMIDSCENE) != 0) &&
			((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_RENDEREDMIDSCENENOFREE) == 0)))
		{
			psPBDesc->ui32BusySceneCount++;
		}
#endif
	}
	else
	{
		/* Pull saved DeviceSyncList */
		psDstDeviceSyncList = psRTDataSet->psCurrentDeviceSyncList;
	}

	if (psKick->ui32KickFlags & SGX_KICKTA_FLAGS_FIRSTTAPROD)
	{
		ui32CmdFlags |= SGXMKIF_TAFLAGS_FIRSTKICK;

		/* Get a free render details. */
		psRenderDetails = IMG_NULL;		/* fix for compiler warning */
		SGXGetFreeRenderDetails(psDevData, psRenderContext,
											psRTDataSet, &psRenderDetails);

		/* Save RenderDetails into RT data set. */
		psRTDataSet->psCurrentRenderDetails = psRenderDetails;
		psHWRenderDetails = psRenderDetails->psHWRenderDetails;

		/* On first prod, toggle to the next RTData */
		GetNextRTData(psDevData, psRTDataSet);

		/* Fill in the rest of the render details */
		psHWRenderDetails->sHWRTDataSetDevAddr =
								psRTDataSet->psHWRTDataSetClientMemInfo->sDevVAddr;
		psHWRenderDetails->sHWRTDataDevAddr.uiAddr =
								psHWRenderDetails->sHWRTDataSetDevAddr.uiAddr +
								offsetof(SGXMKIF_HWRTDATASET, asHWRTData) +
								(psRTDataSet->ui32RTDataSel * sizeof(SGXMKIF_HWRTDATA));

		for(i=0; i<SGX_FEATURE_MP_CORE_COUNT_TA; i++)
		{
			psHWRenderDetails->ui32TEState[i] = 0;
		}

	#if defined(SGX_FEATURE_MP)
		psHWRenderDetails->ui32VDMPIMStatus = 0;
	#endif

	#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
		if (psKickTA->ui32NumRTs == 0)
		{
			psHWRenderDetails->ui32NumRTs = psRTDataSet->ui16NumRTsInArray;
		}
		else
		{
			psHWRenderDetails->ui32NumRTs = psKickTA->ui32NumRTs;
		}
		psHWRenderDetails->ui32NextRTIdx = psKickTA->ui32IdxRT;


		/* if this a RTArray with updates then copy the PixEvent updates */
		if ((psHWRenderDetails->ui32NumRTs > 1) &&
			(psKickTA->psPixEventUpdateList != IMG_NULL &&
			 psKickTA->sPixEventUpdateDevAddr.uiAddr != 0))
		{
			PVR_ASSERT(psKickTA->psPixEventUpdateList != IMG_NULL);
			PVR_ASSERT(psKickTA->sPixEventUpdateDevAddr.uiAddr != 0);

			psHWRenderDetails->sPixEventUpdateDevAddr = psKickTA->sPixEventUpdateDevAddr;
			PVRSRVMemCopy(psRenderDetails->psHWPixEventUpdateList,
							psKickTA->psPixEventUpdateList,
							EURASIA_USE_INSTRUCTION_SIZE * psHWRenderDetails->ui32NumRTs);
		}
		else
		{
			psHWRenderDetails->sPixEventUpdateDevAddr.uiAddr = 0;
		}

		PVR_ASSERT(psKickTA->psHWBgObjPrimPDSUpdateList != IMG_NULL);

		PVRSRVMemCopy(psRenderDetails->psHWBgObjPrimPDSUpdateList,
						psKickTA->psHWBgObjPrimPDSUpdateList,
						sizeof(IMG_UINT32) * psHWRenderDetails->ui32NumRTs);
	#endif

		/*
			Get the registers we'll have to load when we want to kick off this render
		*/
		ui32Num3DRegs = 0;
		Setup3DRegs(psRenderDetails, psKick, psRTDataSet, psRenderContext);
	}
	else
	{
		/* Pull saved RenderDetails and DeviceSyncList */
		psRenderDetails = psRTDataSet->psCurrentRenderDetails;
		psHWRenderDetails = psRenderDetails->psHWRenderDetails;	/* PRQA S 3199 */ /* assignment is needed */

		/* Check if we need to update any 3D registers via the TA cmd */
		ui32Num3DRegs = Update3DRegs(psRenderDetails, &psTACmd->s3DRegUpdates[0], psKick, psRenderContext);
	}

	if	(((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_RENDEREDMIDSCENE) != 0) &&
		 ((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_TAUSEDAFTERRENDER) == 0))
	{
		/*
			This is the first TA kick after a midscene render - set a flag to
			make sure the EDM code waits for the midscene render to complete
			before kicking the TA again
		*/
		ui32CmdFlags |= SGXMKIF_TAFLAGS_RESUMEMIDSCENE;

		if ((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_RENDEREDMIDSCENENOFREE) == 0)
		{
			ui32CmdFlags |= SGXMKIF_TAFLAGS_RESUMEDAFTERFREE;
		}
	}

	/* Decide what render flags we need to send */
	ui32RenderFlags = 0;

	/* microkernel needs to know if there is a depthbuffer present */
	if ((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_DEPTHBUFFER) != 0)
	{
		ui32RenderFlags |= SGXMKIF_RENDERFLAGS_DEPTHBUFFER;
	}
	if ((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_STENCILBUFFER) != 0)
	{
		ui32RenderFlags |= SGXMKIF_RENDERFLAGS_STENCILBUFFER;
	}

	if ((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_KICKRENDER) == 0)
	{
		ui32RenderFlags |= SGXMKIF_RENDERFLAGS_NORENDER;
	}
	if ((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_ABORT) != 0)
	{
		ui32RenderFlags |= SGXMKIF_RENDERFLAGS_ABORT;
	}

	if (((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_MIDSCENENOFREE) != 0)
#if !defined(FIX_HW_BRN_23761)
		 || ((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_VISTESTTERM) != 0)
#endif
		)
	{
		/*
			Don't deallocate the 3D parameters as this is a z-only render or a mid-scene render
		*/
		ui32RenderFlags |= SGXMKIF_RENDERFLAGS_NODEALLOC;
	}

	if	(((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_ZONLYRENDER) != 0) ||
#if !defined(FIX_HW_BRN_23761)
		 ((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_VISTESTTERM) != 0) ||
#endif
		 ((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_COLOURALPHAVALID) == 0)
		)
	{
		/*
			Only rendering to get z details - get EDM code to hack some registers
		*/
		ui32RenderFlags |= SGXMKIF_RENDERFLAGS_ZONLYRENDER;
	}

	if ((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_BBOX_TERMINATE) != 0)
	{
		ui32RenderFlags |= SGXMKIF_RENDERFLAGS_BBOX_RENDER;
	}

	#if defined(SGX_FEATURE_SYSTEM_CACHE) && defined(SGX_FEATURE_MP)
	if ((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_POST_RENDER_SLC_FLUSH) != 0)
	{
		ui32RenderFlags |= SGXMKIF_RENDERFLAGS_FLUSH_SLC;
	}
	#endif

	/*
		Flag any caches that need to be flushed by the EDM
	*/
	if (psKick->ui32KickFlags & SGX_KICKTA_FLAGS_FLUSHDATACACHE)
	{
		ui32CmdFlags |= SGXMKIF_TAFLAGS_INVALIDATEDATACACHE;
	}
	if (psKick->ui32KickFlags & SGX_KICKTA_FLAGS_FLUSHPDSCACHE)
	{
		ui32CmdFlags |= SGXMKIF_TAFLAGS_INVALIDATEPDSCACHE;
	}
	if (psKick->ui32KickFlags & SGX_KICKTA_FLAGS_FLUSHUSECACHE)
	{
		ui32CmdFlags |= SGXMKIF_TAFLAGS_INVALIDATEUSECACHE;
	}

	if (psKick->ui32KickFlags & SGX_KICKTA_FLAGS_COMPLEX_GEOMETRY_PRESENT)
	{
		ui32CmdFlags |= SGXMKIF_TAFLAGS_CMPLX_GEOMETRY_PRESENT;
	}

	/* Set the appropriate flag to let the MICRO kernel know that this is an OpenCL task */
	if (psKick->ui32KickFlags & SGX_KICKTA_FLAGS_COMPUTE_TASK_OPENCL)
	{
		ui32CmdFlags |= SGXMKIF_TAFLAGS_TA_TASK_OPENCL;
	}

	/*
		Setup the TA registers
	*/
	SetupTARegs(psKick, psTACmd,
				psRTDataSet, psRenderContext);

	/* Initialise the number of src syncs */
#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)
	psSubmit->ui32NumTASrcSyncs = 0;
	psSubmit->ui32NumTADstSyncs = 0;
	psSubmit->ui32Num3DSrcSyncs = 0;
#else/* SUPPORT_SGX_GENERALISED_SYNCOBJECTS */
	psSubmit->ui32NumSrcSyncs = 0;
#endif/* SUPPORT_SGX_GENERALISED_SYNCOBJECTS */

	if (psKick->ui32KickFlags & (SGX_KICKTA_FLAGS_TERMINATE | SGX_KICKTA_FLAGS_ABORT))
	{
		/* Set LastKick bit if we are being asked to terminate */
		ui32CmdFlags |= SGXMKIF_TAFLAGS_LASTKICK;

		/* If it's a last kick the HWRTDataset completed ops count is updated
		 * following the render.
		 */
		ui32RenderFlags |= SGXMKIF_RENDERFLAGS_RENDERSCENE;

		/*
			Make sure we don't discard parameters on a midscene render
			so another render can occure using the same data
		*/
		if (psKick->ui32KickFlags & SGX_KICKTA_FLAGS_MIDSCENETERM)
		{
			/* We are rendering midscene so don't reset pointers */
			ui32RenderFlags |= SGXMKIF_RENDERFLAGS_RENDERMIDSCENE;
		}

#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
		if (((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_MIDSCENENOFREE) == 0)
#if !defined(FIX_HW_BRN_23761)
			 && ((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_VISTESTTERM) == 0)
#endif
		)
		{
			psPBDesc->ui32BusySceneCount--;
		}
		if ((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_ABORT) == 0)
		{
			psPBDesc->ui32RenderCount++;
		}
#endif

		ui32RenderFlags |= (psKick->ui32Frame << SGXMKIF_RENDERFLAGS_FRAMENUM_SHIFT);

#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)
		PVR_ASSERT(psKickTA->ui32NumTASrcSyncs <= SGX_MAX_TA_SRC_SYNCS);
		PVR_ASSERT(psKickTA->ui32NumTADstSyncs <= SGX_MAX_TA_DST_SYNCS);
		PVR_ASSERT(psKickTA->ui32Num3DSrcSyncs <= SGX_MAX_3D_SRC_SYNCS);
		psSubmit->ui32NumTASrcSyncs = psKickTA->ui32NumTASrcSyncs;
		psSubmit->ui32NumTADstSyncs = psKickTA->ui32NumTADstSyncs;
		psSubmit->ui32Num3DSrcSyncs = psKickTA->ui32Num3DSrcSyncs;
#else/* SUPPORT_SGX_GENERALISED_SYNCOBJECTS */
		PVR_ASSERT(psKick->ui32NumSrcSyncs <= SGX_MAX_SRC_SYNCS);
		psSubmit->ui32NumSrcSyncs = psKick->ui32NumSrcSyncs;
#endif/* SUPPORT_SGX_GENERALISED_SYNCOBJECTS */
	}

#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	if (psRTDataSet->ui16NumRTsInArray > 1)
	{
		/* This is a render target array so capture the Z/S strides */
		psTACmd->ui32ZBufferStride = psKick->ui32RTAStideZ;
		psTACmd->ui32SBufferStride = psKick->ui32RTAStideS;
	}
#endif

	/* Set up to get the visibility test results back. (services) */
	if (psKick->ui32KickFlags & SGX_KICKTA_FLAGS_GETVISRESULTS)
	{
#if defined(SGX_FEATURE_VISTEST_IN_MEMORY)
		psTACmd->ui32VisTestCount = psKick->ui32VisTestCount;

		/* Default case: use ukernel accumulation */
		if (! (psKick->ui32KickFlags & SGX_KICKTA_FLAGS_DISABLE_ACCUMVISRESULTS))
		{
			ui32RenderFlags |= SGXMKIF_RENDERFLAGS_ACCUMVISRESULTS;
		}
#else
		/* Always use ukernel accumulation */
		ui32RenderFlags |= SGXMKIF_RENDERFLAGS_ACCUMVISRESULTS;
		psTACmd->sVisTestResultsDevAddr = psKick->sVisTestResultMem;
#endif
		ui32RenderFlags |= SGXMKIF_RENDERFLAGS_GETVISRESULTS;
	}

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	#if !defined(SGX_FEATURE_MTE_STATE_FLUSH)
	/* Setup the MTE state shadow address and size... */
	psTACmd->sTAStateShadowDevAddr.uiAddr = psKick->sTAStateShadowDevAddr.uiAddr;
	#endif

	#if !defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
	for (i = 0; i < SGX_MAX_VDM_STATE_RESTORE_PROGS; i++)
	{
		psTACmd->sVDMSARestoreDataDevAddr[i] = psKick->sVDMSARestoreDataDevAddr[i];
	}
	#endif
#endif /* defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) */

	if (psKick->ui32KickFlags & (SGX_KICKTA_FLAGS_DEPENDENT_TA |
									SGX_KICKTA_FLAGS_TA_DEPENDENCY))
	{
		psSubmit->psTA3DSyncInfo = psRenderContext->psTA3DSyncObject;
		psSubmit->bTADependency = IMG_FALSE;

		if (psKick->ui32KickFlags & SGX_KICKTA_FLAGS_DEPENDENT_TA)
		{
	#if defined(DEBUG)
			if (((psRenderContext->ui32LastKickFlags & SGX_KICKTA_FLAGS_TA_DEPENDENCY) != 0) &&
				((psRenderContext->ui32LastKickFlags & (SGX_KICKTA_FLAGS_TERMINATE |
														SGX_KICKTA_FLAGS_ABORT)) == 0))
			{
				/* The last kick for this context set the TA dependency bit but was not a lastkic,
					this will cause a deadlock in the microkernel, so assert */
				PVR_DPF((PVR_DBG_ERROR, "SGXKickTA: TA/3D dependency deadlock, check kickflags!"));
				PVR_DBG_BREAK;
			}
	#endif
			/* Set the flag so the microkernel knows to do the sync check */
			ui32CmdFlags |= SGXMKIF_TAFLAGS_DEPENDENT_TA;
		}
		if (psKick->ui32KickFlags & SGX_KICKTA_FLAGS_TA_DEPENDENCY)
		{
			/* Set the flag so the microkernel knows to do the sync update */
			ui32RenderFlags |= SGXMKIF_RENDERFLAGS_TA_DEPENDENCY;

			psSubmit->bTADependency = IMG_TRUE;
		}
	}
	else
	{
		psSubmit->psTA3DSyncInfo = IMG_NULL;
		psSubmit->bTADependency = IMG_FALSE;
	}
	
#if defined(SGX_FEATURE_MP) && defined(FIX_HW_BRN_31079)
	if (psKick->ui32KickFlags & SGX_KICKTA_FLAGS_VDM_PIM_SPLIT)
	{
		/* This command has been kicked to ensure the PIM does not wrap, 
			set the flag so the ukernel will load 0 for the next TA */
		ui32CmdFlags |= SGXMKIF_TAFLAGS_VDM_PIM_WRAP;
	}
#endif

	/*
		Setup rest of Command.
	*/
	/* Calculate space required for the TA command */
	psTACmd->ui32Size	= sizeof(SGXMKIF_CMDTA) + ui32Num3DRegs * sizeof(PVRSRV_HWREG);;
	psTACmd->ui32Flags	= ui32CmdFlags;
	psTACmd->ui32RenderFlags = ui32RenderFlags;
	psTACmd->ui32FrameNum 	 = psKick->ui32Frame;

	psHWPBDescClientMemInfo = psClientPBDesc->psHWPBDescClientMemInfo;
	psTACmd->sHWPBDescDevVAddr = psHWPBDescClientMemInfo->sDevVAddr;
	psTACmd->sHWRenderDetailsDevAddr = psRenderDetails->psHWRenderDetailsClientMemInfo->sDevVAddr;

	if (psDstDeviceSyncList != IMG_NULL)
	{
		psTACmd->sHWDstSyncListDevAddr =
								psDstDeviceSyncList->psHWDeviceSyncListClientMemInfo->sDevVAddr;
	}
	else
	{
		psTACmd->sHWDstSyncListDevAddr.uiAddr = IMG_NULL;
	}
	psTACmd->sHWRTDataSetDevAddr = psRTDataSet->psHWRTDataSetClientMemInfo->sDevVAddr;
	psTACmd->sHWRTDataDevAddr.uiAddr = psTACmd->sHWRTDataSetDevAddr.uiAddr +
										offsetof(SGXMKIF_HWRTDATASET, asHWRTData) +
										(psRTDataSet->ui32RTDataSel * sizeof(SGXMKIF_HWRTDATA));

	psTransferContext = (SGXTQ_CLIENT_TRANSFER_CONTEXT *)psKickTA->hTQContext;
	if (((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_TQTA_SYNC) != 0) && (psTransferContext != IMG_NULL))
	{
		PVRSRV_CLIENT_SYNC_INFO *psTASyncInfo = psTransferContext->psTASyncObject;

		psSubmit->psTASyncInfo = psTASyncInfo;
	}
	else
	{
		psSubmit->psTASyncInfo = IMG_NULL;
		if (psKick->ui32KickFlags & SGX_KICKTA_FLAGS_TQTA_SYNC)
		{
			PVR_DPF((PVR_DBG_ERROR, "SGXKickTA: hTQContext was missing from the kick."));
		}
	}
	if (((psKick->ui32KickFlags & SGX_KICKTA_FLAGS_TQ3D_SYNC) != 0) && (psTransferContext != IMG_NULL))
	{
		PVRSRV_CLIENT_SYNC_INFO *ps3DSyncInfo = psTransferContext->ps3DSyncObject;

	#if defined(DEBUG)
		if ((psKick->ui32KickFlags & (SGX_KICKTA_FLAGS_TERMINATE | SGX_KICKTA_FLAGS_ABORT)) == 0)
		{
			/* This is not a last kick but has TQ/3D sync set, could cause deadlock so give a warning */
			PVR_DPF((PVR_DBG_WARNING, "SGXKickTA: potential TQ/3D sync deadlock (LASTKICK flag not set)!"));
		}
	#endif

		psSubmit->ps3DSyncInfo = ps3DSyncInfo;
	}
	else
	{
		psSubmit->ps3DSyncInfo = IMG_NULL;
		if (psKick->ui32KickFlags & SGX_KICKTA_FLAGS_TQ3D_SYNC)
		{
			PVR_DPF((PVR_DBG_ERROR, "SGXKickTA: hTQContext was missing from the kick."));
		}
	}

	psTACmd->ui32Num3DRegs = ui32Num3DRegs;
	psTACmd->aui32SpecObject[0] = psKick->aui32SpecObject[0];
	psTACmd->aui32SpecObject[1] = psKick->aui32SpecObject[1];
	psTACmd->aui32SpecObject[2] = psKick->aui32SpecObject[2];


	/*
		Save submit details
	*/
	/* The command will actually be inserted in the CCB in submit */
	psSubmit->pvTACmd = (IMG_VOID *)psTACmd;
	PVR_ASSERT(psKick->ui32NumTAStatusVals <= SGX_MAX_TA_STATUS_VALS);
	PVR_ASSERT(psKick->ui32Num3DStatusVals <= SGX_MAX_3D_STATUS_VALS);
	psSubmit->ui32NumTAStatusVals = psKick->ui32NumTAStatusVals;
	psSubmit->ui32Num3DStatusVals = psKick->ui32Num3DStatusVals;

	psSubmit->bFirstKickOrResume = (ui32CmdFlags & (SGXMKIF_TAFLAGS_FIRSTKICK | SGXMKIF_TAFLAGS_RESUMEMIDSCENE))
									 ? IMG_TRUE : IMG_FALSE;

#if (defined(NO_HARDWARE) || defined(PDUMP))
	psSubmit->bTerminateOrAbort = (psKick->ui32KickFlags & (SGX_KICKTA_FLAGS_TERMINATE | SGX_KICKTA_FLAGS_ABORT))
									? IMG_TRUE : IMG_FALSE;
#endif

	/* Prepare for TA Status value copying */
	for (i = 0; i < psKick->ui32NumTAStatusVals; i++)
	{
#if defined(SUPPORT_SGX_NEW_STATUS_VALS)
		psSubmit->asTAStatusUpdate[i].sCtlStatus = psKickTA->asTAStatusUpdate[i].sCtlStatus;
		psSubmit->asTAStatusUpdate[i].hKernelMemInfo = psKickTA->asTAStatusUpdate[i].hKernelMemInfo;
#else
		psSubmit->ahTAStatusInfo[i] = (IMG_HANDLE)psKickTA->apsTAStatusInfo[i];
#endif
	}
	/* Prepare for 3D Status value copying */
	for (i = 0; i < psKick->ui32Num3DStatusVals; i++)
	{
#if defined(SUPPORT_SGX_NEW_STATUS_VALS)
		psSubmit->as3DStatusUpdate[i].sCtlStatus = psKickTA->as3DStatusUpdate[i].sCtlStatus;
		psSubmit->as3DStatusUpdate[i].hKernelMemInfo = psKickTA->as3DStatusUpdate[i].hKernelMemInfo;
#else
		psSubmit->ah3DStatusInfo[i] = (IMG_HANDLE)psKickTA->aps3DStatusInfo[i];
#endif
	}

	psSubmit->psRTData 				= &psRTDataSet->psRTData[psRTDataSet->ui32RTDataSel];
	psSubmit->psRTDataSet 			= psRTDataSet;
	psSubmit->psRenderDetails 		= psRenderDetails;
	psSubmit->psDstDeviceSyncList 	= psDstDeviceSyncList;
	psSubmit->psRenderContext 		= psKickTA->hRenderContext;
	psSubmit->ui32KickFlags 		= psKick->ui32KickFlags;
	psSubmit->ui32Frame 			= psKick->ui32Frame;
#if defined(PDUMP)
	psSubmit->psKickPDUMP 			= psKickPDUMP;
#endif

	if (psDstDeviceSyncList)
	{
		PPVRSRV_CLIENT_SYNC_INFO psRenderSurfSyncInfo = IMG_NULL;

		psDstDeviceSyncList->ui32NumSyncObjects = psRTDataSet->ui16NumRTsInArray;
		for (i=0; i < psDstDeviceSyncList->ui32NumSyncObjects; i++)
		{
			/* Get the next SyncInfo ptr */
			psRenderSurfSyncInfo = psKickTA->ppsRenderSurfSyncInfo[i];
			if (psRenderSurfSyncInfo != IMG_NULL)
			{
				psDstDeviceSyncList->ahSyncHandles[i] = psRenderSurfSyncInfo->hKernelSyncInfo;
			}
			else
			{
				psDstDeviceSyncList->ahSyncHandles[i] = 0;
			}
		}

#if defined(PDUMP) || defined(DEBUG)
#if defined (SUPPORT_SID_INTERFACE)
		psSubmit->hLastSyncInfo = psDstDeviceSyncList->ahSyncHandles[psDstDeviceSyncList->ui32NumSyncObjects - 1];
#else
		psSubmit->psLastSyncInfo = psRenderSurfSyncInfo;
#endif
#endif
	}

#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)
	/* TA SRC read dependencies */
	for (i=0; i<psSubmit->ui32NumTASrcSyncs; i++)
	{
		psSubmit->apsTASrcSurfSyncInfo[i] = psKickTA->apsTASrcSurfSyncInfo[i];
	}
	/* TA DST read dependencies */
	for (i=0; i<psSubmit->ui32NumTADstSyncs; i++)
	{
		psSubmit->apsTADstSurfSyncInfo[i] = psKickTA->apsTADstSurfSyncInfo[i];
	}
	/* 3D SRC read dependencies */
	for (i=0; i<psSubmit->ui32Num3DSrcSyncs; i++)
	{
		psSubmit->aps3DSrcSurfSyncInfo[i] = psKickTA->aps3DSrcSurfSyncInfo[i];
	}
#else/* SUPPORT_SGX_GENERALISED_SYNCOBJECTS */
	/* SRC read dependencies */
	for (i=0; i<psSubmit->ui32NumSrcSyncs; i++)
	{
		psSubmit->apsSrcSurfSyncInfo[i] = psKickTA->apsSrcSurfSyncInfo[i];
	}
#endif/* SUPPORT_SGX_GENERALISED_SYNCOBJECTS */

	/* Pass back the number of pages left available in the PB */
	psKickOutput->ui32NumPBPagesFree = psPBDesc->ui32NumPages;
#if defined(FIX_HW_BRN_23055)
	psKickOutput->ui32NumPBPagesFree -= BRN23055_BUFFER_PAGE_SIZE;
#endif
#if 1 // SPM_PAGE_INJECTION
	psKickOutput->ui32NumPBPagesFree -= SPM_DL_RESERVE_PAGES;
#endif
	psKickOutput->ui32NumPBPagesFree -=
        ((PSGXMKIF_HWPBDESC)psHWPBDescClientMemInfo->pvLinAddr)->ui32AllocPages;

	/*
 	 * Since the TA command structure has already been put in the
 	 * circular buffer, but the command hasn't been submitted yet,
 	 * the render context mutex must be held to ensure the information
 	 * in the kick structure is associated with the correct command
 	 * in the circular buffer, when running in a multithreaded
 	 * environment.
 	 */
#if defined(SGX_PREP_SUBMIT_SEPERATE)
	PVR_ASSERT(psSubmit->ui32TACmdBytes >= psTACmd->ui32Size);
#else
	eError = SGXKickSubmit(psDevData, psSubmit);
#endif

	PVRSRVUnlockMutex(psRTDataSet->hMutex);

	return eError;
}

#if defined(NO_HARDWARE)
/* PRQA S 3410 3 */ /* macros require the absence of some brackets */
#define	CCB_DATA_FROM_OFFSET(type, psSubmit, offset) \
	((type *)(((IMG_CHAR *)(psSubmit)->psTACCBClientMemInfo->pvLinAddr) + \
		(psSubmit)->offset))
#endif

/*!
 * *****************************************************************************
 * @brief Really kicks the TA
 *
 * @param psDevData
 * @param pvKickSubmit
 *
 * @return success or failure
 ********************************************************************************/
#if defined(SGXTQ_PREP_SUBMIT_SEPERATE)
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV SGXKickSubmit(PVRSRV_DEV_DATA *psDevData, IMG_PVOID pvKickSubmit)
#else
static PVRSRV_ERROR SGXKickSubmit(PVRSRV_DEV_DATA *psDevData, IMG_PVOID pvKickSubmit)
#endif
{
	PVRSRV_ERROR			eError;
	PSGX_KICKTA_SUBMIT		psKickSubmit 			= (PSGX_KICKTA_SUBMIT)pvKickSubmit;
	/* PRQA S 703 1 */ /* too few initialisers*/
	SGX_CCB_KICK 			sCCBKick 				= {{0}}; // FIXME: remove clear, as all(?check) fields are initialised
	SGX_RENDERCONTEXT 		*psRenderContext 		= psKickSubmit->psRenderContext;
	SGX_DEVICE_SYNC_LIST 	*psDstDeviceSyncList 	= psKickSubmit->psDstDeviceSyncList;
	PSGXMKIF_CMDTA 			psTACmd 				= (PSGXMKIF_CMDTA) psKickSubmit->pvTACmd;
#if defined(PDUMP) || defined(DEBUG)
#if defined (SUPPORT_SID_INTERFACE)
	IMG_SID					hSyncInfo 			= psKickSubmit->hLastSyncInfo;
#else
	PVRSRV_CLIENT_SYNC_INFO *psSyncInfo 			= psKickSubmit->psLastSyncInfo;
#endif
#endif
#if defined(PDUMP) || defined(NO_HARDWARE) || defined(DEBUG)
	IMG_UINT32 ui32KickFlags 	= psKickSubmit->ui32KickFlags;
#endif
#if defined(PDUMP)
	SGX_KICKTA_PDUMP	*psKickPDUMP = psKickSubmit->psKickPDUMP;
#endif
#if (defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)) 
	SGX_PBDESC			*psPBDesc = psRenderContext->psClientPBDesc->psPBDesc;
#endif
	IMG_UINT32		ui32CmdSizeRounded;
	IMG_UINT32		ui32CmdSize;
	IMG_PUINT32 	pui32Dest;
	IMG_UINT32 		i;


	/* Lock the rendercontxt mutex so only 1 thread can insert a command */
	PVRSRVLockMutex(psRenderContext->hMutex);

	/* Get space in the CCB... */
	ui32CmdSize	= psTACmd->ui32Size;
	ui32CmdSizeRounded = SGXCalcContextCCBParamSize(ui32CmdSize, SGX_CCB_ALLOCGRAN);
	pui32Dest = SGXAcquireCCB(psDevData, psRenderContext->psTACCB, ui32CmdSizeRounded, psRenderContext->hOSEvent);

	/* Waited for space and there was none */
	if(pui32Dest == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXKickSubmit: failed acquire CCB space!"));
		eError = PVRSRV_ERROR_TIMEOUT;
		goto SubmitExit;
	}

#if defined(PDUMP)
	if(PDUMPISCAPTURINGTEST(psDevData->psConnection))
	{
		PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0,
							   "\r\n-- Increment the pending operation count for the RTDataSet\r\n");
		psKickSubmit->psRTDataSet->ui32PDumpPendingCount++;
		PDUMPMEM(psDevData->psConnection,
				 &psKickSubmit->psRTDataSet->ui32PDumpPendingCount,
				 psKickSubmit->psRTDataSet->psPendingCountClientMemInfo,
				 0,
				(IMG_UINT32)psKickSubmit->psRTDataSet->psPendingCountClientMemInfo->uAllocSize,
				 0);
	}
#endif

	/* Update the HWRTDataset pending count */
	(*psKickSubmit->psRTDataSet->pui32PendingCount)++;

	/* Replace the actual size of the aligned size */
	psTACmd->ui32Size = ui32CmdSizeRounded;

        PVR_ASSERT((IMG_UINTPTR_T)pui32Dest - (IMG_UINTPTR_T)
                        psRenderContext->psTACCB->psCCBClientMemInfo->pvLinAddr + ui32CmdSize < psRenderContext->psTACCB->psCCBClientMemInfo->uAllocSize);

	PVRSRVMemCopy(pui32Dest, (IMG_PUINT32)psTACmd, ui32CmdSize); /* Copy actual, not rounded size */

	psTACmd = (PSGXMKIF_CMDTA)pui32Dest;

	sCCBKick.ui32CCBOffset 	= CURRENT_CCB_OFFSET(psRenderContext->psTACCB) +
											offsetof(SGXMKIF_CMDTA, sShared);

	/* Update CCB */
	UPDATE_CCB_OFFSET(*psRenderContext->psTACCB->pui32WriteOffset,
						ui32CmdSizeRounded, psRenderContext->psTACCB->ui32Size);

#if defined(DEBUG)
	psRenderContext->ui32LastKickFlags = ui32KickFlags;
#endif
#if defined(SUPPORT_SGX_PRIORITY_SCHEDULING)
	psRenderContext->bKickSubmitted = IMG_TRUE;
#endif

	/* PDUMP and supplied ROff values */
#if defined(PDUMP)
	if(psKickPDUMP)
	{
		PVRSRV_CLIENT_MEM_INFO	sClientMemInfo;

		if (ui32KickFlags & (SGX_KICKTA_FLAGS_FIRSTTAPROD |
							 SGX_KICKTA_FLAGS_RENDEREDMIDSCENE))
		{
			if (psDstDeviceSyncList != IMG_NULL)
			{
				PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "\r\n-- Wait for DeviceSyncList to become available");
#if defined (SUPPORT_SID_INTERFACE)
				PDUMPMEMPOL(psDevData->psConnection,
							psDstDeviceSyncList->psAccessResourceClientMemInfo->hKernelMemInfo,
							0,
							0,
							0xFFFFFFFF,
							0);
#else
				PDUMPMEMPOL(psDevData->psConnection,
							psDstDeviceSyncList->psAccessResourceClientMemInfo,
							0,
							0,
							0xFFFFFFFF,
							0);
#endif

				PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "Set DeviceSyncList access bit\r\n");
				PDUMPMEM(psDevData->psConnection,
						 IMG_NULL,
						 psDstDeviceSyncList->psAccessResourceClientMemInfo,
						 0,
						 sizeof(PVRSRV_RESOURCE),
						 0);
			}

			if (ui32KickFlags & SGX_KICKTA_FLAGS_FIRSTTAPROD)
			{
				PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "\r\n-- Wait for render details block to become available");
#if defined (SUPPORT_SID_INTERFACE)
				PDUMPMEMPOL(psDevData->psConnection,
							psKickSubmit->psRenderDetails->psAccessResourceClientMemInfo->hKernelMemInfo,
							0,
							0,
							0xFFFFFFFF,
							0);
#else
				PDUMPMEMPOL(psDevData->psConnection,
							psKickSubmit->psRenderDetails->psAccessResourceClientMemInfo,
							0,
							0,
							0xFFFFFFFF,
							0);
#endif

				PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "Set render details access bit\r\n");
				PDUMPMEM(psDevData->psConnection,
						 IMG_NULL,
						 psKickSubmit->psRenderDetails->psAccessResourceClientMemInfo,
						 0,
						 sizeof(PVRSRV_RESOURCE),
						 0);
			}
		}

		if (psKickPDUMP->ui32BufferArraySize > 0)
		{
			PVRSRVPDumpBufferArray(psDevData->psConnection, psKickPDUMP->psBufferArray, psKickPDUMP->ui32BufferArraySize, IMG_TRUE);
		}

		/*
 		 * Pdump only requries the kernel handle, so we create a
 		 * temp ClientMemInfo struct and add the handle.
		 */
		PVRSRVMemSet(&sClientMemInfo, 0, sizeof(sClientMemInfo));

		for (i=0; i<psKickPDUMP->ui32ROffArraySize; i++)
		{
			PSGX_KICKTA_DUMP_ROFF	psROff;

			psROff = &psKickPDUMP->psROffArray[i];
			sClientMemInfo.hKernelMemInfo = psROff->hKernelMemInfo;

			if (psROff->pszName)
			{
				PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "%s\r\n", psROff->pszName);
			}
			else
			{
				PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "ROff Array[%d]\r\n", i);
			}

			PDUMPMEM(psDevData->psConnection,
					 &psROff->ui32Value,
					 &sClientMemInfo,
					 psROff->ui32Offset,
					 sizeof(IMG_UINT32),
					 0);
		}
	}
#endif

#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
	#if defined(PDUMP)
	if ( psPBDesc->bPdumpRequired == IMG_TRUE &&
		(PDUMPISCAPTURINGTEST(psDevData->psConnection) != IMG_FALSE))
	{
		if ((psPBDesc->psListPBBlockHead != psPBDesc->psListPBBlockTail)
			|| psPBDesc->psShrinkListPBBlock != IMG_NULL)
		{
			PdumpPBGrowShrinkUpdates(psDevData, psRenderContext);
		}
	}
	#endif

	if ((psPBDesc->ui32Flags & (SGX_PBDESCFLAGS_GROW_PENDING | SGX_PBDESCFLAGS_SHRINK_PENDING)) != 0)
	{
		/* Fill in the updates which are common between grow and shrink */
		psTACmd->sHWPBDescUpdate.ui32TAThreshold		= psPBDesc->ui32TAThreshold;
		psTACmd->sHWPBDescUpdate.ui32ZLSThreshold		= psPBDesc->ui32ZLSThreshold;
		psTACmd->sHWPBDescUpdate.ui32GlobalThreshold	= psPBDesc->ui32GlobalThreshold;
		psTACmd->sHWPBDescUpdate.ui32PDSThreshold		= psPBDesc->ui32PDSThreshold;

		psTACmd->sHWPBDescUpdate.ui32NumPages 			= psPBDesc->ui32NumPages;
		psTACmd->sHWPBDescUpdate.ui32FreeListInitialHT	= psPBDesc->ui32FreeListInitialHT;
		psTACmd->sHWPBDescUpdate.ui32FreeListInitialPrev = psPBDesc->ui32FreeListInitialPrev;

		/* Link the grow block list into the HWPBBlock list */
		psTACmd->sHWPBDescUpdate.sPBBlockPtrUpdateDevAddr = psPBDesc->psListPBBlockTail->sHWPBBlockDevVAddr;

		if ((psPBDesc->ui32Flags & SGX_PBDESCFLAGS_GROW_PENDING) != 0)
		{
			psTACmd->ui32Flags |= SGXMKIF_TAFLAGS_PB_GROW;
			psTACmd->sHWPBDescUpdate.ui32PBBlockPtrUpdate = psPBDesc->psGrowListPBBlockHead->sHWPBBlockDevVAddr.uiAddr;
		}
		else
		{
			psTACmd->ui32Flags |= SGXMKIF_TAFLAGS_PB_SHRINK;
			psTACmd->sHWPBDescUpdate.ui32PBBlockPtrUpdate = 0;
		}
	}
#endif
	{
#if defined(PDUMP)
		if(PDUMPISCAPTURINGTEST(psDevData->psConnection))
		{
			IMG_UINT32		ui32Flag;
			static const struct
			{
				IMG_UINT32	ui32Flag;
				IMG_PCHAR	pszName;
			} psTAFlagNames[] = {
				{SGXMKIF_TAFLAGS_FIRSTKICK, "SGXMKIF_TAFLAGS_FIRSTKICK"},
				{SGXMKIF_TAFLAGS_LASTKICK, "SGXMKIF_TAFLAGS_LASTKICK"},
				{SGXMKIF_TAFLAGS_RESUMEMIDSCENE, "SGXMKIF_TAFLAGS_RESUMEMIDSCENE"},
				{SGXMKIF_TAFLAGS_RESUMEDAFTERFREE, "SGXMKIF_TAFLAGS_RESUMEDAFTERFREE"},
				{SGXMKIF_TAFLAGS_INVALIDATEUSECACHE, "SGXMKIF_TAFLAGS_INVALIDATEUSECACHE"},
				{SGXMKIF_TAFLAGS_INVALIDATEPDSCACHE, "SGXMKIF_TAFLAGS_INVALIDATEPDSCACHE"},
				{SGXMKIF_TAFLAGS_INVALIDATEDATACACHE, "SGXMKIF_TAFLAGS_INVALIDATEDATACACHE"},
#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
				{SGXMKIF_TAFLAGS_PB_GROW, "SGXMKIF_TAFLAGS_PB_GROW"},
				{SGXMKIF_TAFLAGS_PB_SHRINK, "SGXMKIF_TAFLAGS_PB_SHRINK"},
#endif
				{SGXMKIF_TAFLAGS_DEPENDENT_TA, "SGXMKIF_TAFLAGS_DEPENDENT_TA"}};
			static const struct
			{
				IMG_UINT32	ui32Flag;
				IMG_PCHAR	pszName;
			} psRenderFlagNames[] = {
				{SGXMKIF_RENDERFLAGS_NORENDER, "SGXMKIF_RENDERFLAGS_NORENDER"},
				{SGXMKIF_RENDERFLAGS_ABORT, "SGXMKIF_RENDERFLAGS_ABORT"},
				{SGXMKIF_RENDERFLAGS_RENDERMIDSCENE, "SGXMKIF_RENDERFLAGS_RENDERMIDSCENE"},
				{SGXMKIF_RENDERFLAGS_NODEALLOC, "SGXMKIF_RENDERFLAGS_NODEALLOC"},
				{SGXMKIF_RENDERFLAGS_ZONLYRENDER, "SGXMKIF_RENDERFLAGS_ZONLYRENDER"},
				{SGXMKIF_RENDERFLAGS_TA_DEPENDENCY, "SGXMKIF_RENDERFLAGS_TA_DEPENDENCY"}};


			/* Pdump the command from the per context CCB */
			PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "TA command\r\n");
			PDUMPMEM(psDevData->psConnection, psTACmd, psRenderContext->psTACCB->psCCBClientMemInfo, psRenderContext->psTACCB->ui32CCBDumpWOff, psTACmd->ui32Size, 0);

			/* Print the flags from the command for ease of debugging. */
			for (ui32Flag = 0; ui32Flag < (sizeof(psTAFlagNames) / sizeof(psTAFlagNames[0])); ui32Flag++)
			{
				if (psTACmd->ui32Flags & psTAFlagNames[ui32Flag].ui32Flag)
				{
					PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "   %s\r\n", psTAFlagNames[ui32Flag].pszName);
				}
			}
			for (ui32Flag = 0; ui32Flag < (sizeof(psRenderFlagNames) / sizeof(psRenderFlagNames[0])); ui32Flag++)
			{
				if (psTACmd->ui32RenderFlags & psRenderFlagNames[ui32Flag].ui32Flag)
				{
					PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "   %s\r\n", psRenderFlagNames[ui32Flag].pszName);
				}
			}
			PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "   RenderDetails: 0x%.8X RTData: 0x%.8X\r\n", psTACmd->sHWRenderDetailsDevAddr.uiAddr, psTACmd->sHWRTDataDevAddr.uiAddr);

			if (psTACmd->ui32Flags & SGXMKIF_TAFLAGS_FIRSTKICK)
			{
	#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
				IMG_UINT32	ui32NumRTs = psKickSubmit->psRenderDetails->psHWRenderDetails->ui32NumRTs;
	#endif
				/* Pdump the render details */
				PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "Render details for the TA kick\r\n");
				PDUMPMEM(psDevData->psConnection,
						 IMG_NULL,
						 psKickSubmit->psRenderDetails->psHWRenderDetailsClientMemInfo,
						 0,
						 sizeof(SGXMKIF_HWRENDERDETAILS),
						 0);

	#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
				PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "Num RTs in Array: %d\r\n", ui32NumRTs);
				if (ui32NumRTs > 1)
				{
					PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0,
											"Pixel Event Program update list for TA kick\r\n");
					PDUMPMEM(psDevData->psConnection,
							 IMG_NULL,
							 psKickSubmit->psRenderDetails->psHWPixEventUpdateListClientMemInfo,
							 0, EURASIA_USE_INSTRUCTION_SIZE * ui32NumRTs, 0);
				}

				PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0,
										"HW BgObj Primary PDS state update list for TA kick\r\n");
				PDUMPMEM(psDevData->psConnection,
				 		 IMG_NULL,
						 psKickSubmit->psRenderDetails->psHWBgObjPrimPDSUpdateListClientMemInfo,
						 0, sizeof(IMG_UINT32) * ui32NumRTs, 0);
	#endif

			}

			sCCBKick.ui32CCBDumpWOff = psRenderContext->psTACCB->ui32CCBDumpWOff +
										offsetof(SGXMKIF_CMDTA, sShared);
			UPDATE_CCB_OFFSET(psRenderContext->psTACCB->ui32CCBDumpWOff,
							  psTACmd->ui32Size,
							  psRenderContext->psTACCB->ui32Size);
			/* Make sure we pdumped the update of the WOFF */
			PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "Update the TA CCB WOFF\r\n");
			PDUMPMEM(psDevData->psConnection,
						 &psRenderContext->psTACCB->ui32CCBDumpWOff,
						 psRenderContext->psTACCB->psCCBCtlClientMemInfo,
						 offsetof(PVRSRV_SGX_CCB_CTL, ui32WriteOffset),
						 sizeof(IMG_UINT32),
						 0);
		}
#endif /* PDUMP */

		/* prepare the kernel CCB kick */
		sCCBKick.hCCBKernelMemInfo 		= psRenderContext->psTACCB->psCCBClientMemInfo->hKernelMemInfo;
		sCCBKick.sCommand.ui32Data[1] 	= psRenderContext->sHWRenderContextDevVAddr.uiAddr;

		/* TA/3D dependency details */
		if (psKickSubmit->psTA3DSyncInfo != IMG_NULL)
		{
			sCCBKick.hTA3DSyncInfo = psKickSubmit->psTA3DSyncInfo->hKernelSyncInfo;
			sCCBKick.bTADependency = psKickSubmit->bTADependency;
		}

		if (psKickSubmit->psTASyncInfo != IMG_NULL)
		{
			sCCBKick.hTASyncInfo = psKickSubmit->psTASyncInfo->hKernelSyncInfo;
		}

		if (psKickSubmit->ps3DSyncInfo != IMG_NULL)
		{
			sCCBKick.h3DSyncInfo = psKickSubmit->ps3DSyncInfo->hKernelSyncInfo;
		}

		/* Prepare for TA Status value copying */
		for (i = 0; i < psKickSubmit->ui32NumTAStatusVals; i++)
		{
#if defined(SUPPORT_SGX_NEW_STATUS_VALS)
			sCCBKick.asTAStatusUpdate[i].sCtlStatus = psKickSubmit->asTAStatusUpdate[i].sCtlStatus;
			sCCBKick.asTAStatusUpdate[i].hKernelMemInfo = psKickSubmit->asTAStatusUpdate[i].hKernelMemInfo;
#else
			sCCBKick.ahTAStatusSyncInfo[i] = ((PVRSRV_CLIENT_SYNC_INFO *)(psKickSubmit->ahTAStatusInfo[i]))->hKernelSyncInfo;
#endif
		}
		sCCBKick.ui32NumTAStatusVals = psKickSubmit->ui32NumTAStatusVals;

		/* Prepare for 3D Status value copying */
		for (i = 0; i < psKickSubmit->ui32Num3DStatusVals; i++)
		{
#if defined(SUPPORT_SGX_NEW_STATUS_VALS)
			sCCBKick.as3DStatusUpdate[i].sCtlStatus = psKickSubmit->as3DStatusUpdate[i].sCtlStatus;
			sCCBKick.as3DStatusUpdate[i].hKernelMemInfo = psKickSubmit->as3DStatusUpdate[i].hKernelMemInfo;
#else
			sCCBKick.ah3DStatusSyncInfo[i] = ((PVRSRV_CLIENT_SYNC_INFO *)(psKickSubmit->ah3DStatusInfo[i]))->hKernelSyncInfo;
#endif
		}
		sCCBKick.ui32Num3DStatusVals = psKickSubmit->ui32Num3DStatusVals;

#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)
		/* TA phase SRC syncs */
		for (i=0; i<psKickSubmit->ui32NumTASrcSyncs; i++)
		{
			sCCBKick.ahTASrcKernelSyncInfo[i] = psKickSubmit->apsTASrcSurfSyncInfo[i]->hKernelSyncInfo;
		}
		sCCBKick.ui32Num3DSrcSyncs = psKickSubmit->ui32Num3DSrcSyncs;
		/* TA phase DST syncs */
		for (i=0; i<psKickSubmit->ui32NumTADstSyncs; i++)
		{
			sCCBKick.ahTADstKernelSyncInfo[i] = psKickSubmit->apsTADstSurfSyncInfo[i]->hKernelSyncInfo;
		}
		sCCBKick.ui32NumTADstSyncs = psKickSubmit->ui32NumTADstSyncs;
		/* 3D phase SRC syncs */
		for (i=0; i<psKickSubmit->ui32Num3DSrcSyncs; i++)
		{
			sCCBKick.ah3DSrcKernelSyncInfo[i] = psKickSubmit->aps3DSrcSurfSyncInfo[i]->hKernelSyncInfo;
		}
		sCCBKick.ui32Num3DSrcSyncs = psKickSubmit->ui32Num3DSrcSyncs;
#else/* SUPPORT_SGX_GENERALISED_SYNCOBJECTS */
		/* texture read dependencies */
		for (i=0; i<psKickSubmit->ui32NumSrcSyncs; i++)
		{
			sCCBKick.ahSrcKernelSyncInfo[i] = psKickSubmit->apsSrcSurfSyncInfo[i]->hKernelSyncInfo;
		}
		sCCBKick.ui32NumSrcSyncs = psKickSubmit->ui32NumSrcSyncs;
#endif/* SUPPORT_SGX_GENERALISED_SYNCOBJECTS */

		if (psDstDeviceSyncList)
		{
			sCCBKick.ui32NumDstSyncObjects = psDstDeviceSyncList->ui32NumSyncObjects;
			sCCBKick.pahDstSyncHandles = psDstDeviceSyncList->ahSyncHandles;
			sCCBKick.hKernelHWSyncListMemInfo = psDstDeviceSyncList->hKernelHWSyncListMemInfo;
		}

		sCCBKick.bFirstKickOrResume = psKickSubmit->bFirstKickOrResume;

#if (defined(NO_HARDWARE) || defined(PDUMP))
		sCCBKick.bTerminateOrAbort = psKickSubmit->bTerminateOrAbort;
#endif

#if defined(NO_HARDWARE)
		if (psKickSubmit->bFirstKickOrResume)
		{
			psKickSubmit->psRenderDetails->ui32WriteOpsPendingVal = 0;
		}

		/*
		 * If the write ops pending value is generated on this
		 * kick, the write ops pending value will be obtained
		 * directly from the sync object in srvkm, and the value
		 * passed in here ignored.
		 */
		sCCBKick.ui32WriteOpsPendingVal = psKickSubmit->psRenderDetails->ui32WriteOpsPendingVal;
#endif
		sCCBKick.hDevMemContext = psRenderContext->hDevMemContext;

		{
			sCCBKick.bLastInScene =
				(psTACmd->ui32Flags & SGXMKIF_TAFLAGS_LASTKICK) ? IMG_TRUE : IMG_FALSE;

			/* Update sync objects and do Kick */
			eError = SGXDoKick(psDevData, &sCCBKick);
			if(eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "SGXKickSubmit: failed call to SGXDoKick (%d)", eError));
				goto SubmitExit;
			}
		}

#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
		PBGrowShrinkComplete(psDevData, psRenderContext);
#endif

#if defined(NO_HARDWARE)
		/* Update read offset */
		UPDATE_CCB_OFFSET(*psRenderContext->psTACCB->pui32ReadOffset, psTACmd->ui32Size, psRenderContext->psTACCB->ui32Size);
#endif

#if defined(PDUMP)
		if(PDUMPISCAPTURINGTEST(psDevData->psConnection))
		{
			if (psRenderContext->ui32TAKickRegFlags & PVRSRV_SGX_TAREGFLAGS_TACOMPLETESYNC)
			{
				/* Wait for TA kick to complete */
				PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "Poll for TA Kick to complete");
#if defined (SUPPORT_SID_INTERFACE)
				PDUMPMEMPOL(psDevData->psConnection,
							psRenderContext->psTACCB->psCCBCtlClientMemInfo->hKernelMemInfo,
							offsetof(PVRSRV_SGX_CCB_CTL, ui32ReadOffset),
							psRenderContext->psTACCB->ui32CCBDumpWOff,
							0xFFFFFFFF,
							0);
#else
				PDUMPMEMPOL(psDevData->psConnection,
							psRenderContext->psTACCB->psCCBCtlClientMemInfo,
							offsetof(PVRSRV_SGX_CCB_CTL, ui32ReadOffset),
							psRenderContext->psTACCB->ui32CCBDumpWOff,
							0xFFFFFFFF,
							0);
#endif

				/* Now the kick has complete capture the signatures */
				PDumpTASignatureRegisters(psDevData,
										psRenderContext->ui32RenderNum,
										psRenderContext->ui32KickTANum, IMG_FALSE);
				psRenderContext->ui32KickTANum++;
			}

			if (psKickSubmit->bTerminateOrAbort)
			{
				/* Unless we are forcing synchronization do this only on the last render. */
				if (
					((ui32KickFlags & SGX_KICKTA_FLAGS_ABORT) == 0) &&
					((psRenderContext->ui32TAKickRegFlags & (PVRSRV_SGX_TAREGFLAGS_TACOMPLETESYNC | PVRSRV_SGX_TAREGFLAGS_RENDERSYNC)) == 0)
					)
				{
					PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "LASTONLY{\r\n");
				}
				/* Wait for the render to complete */
#if defined (SUPPORT_SID_INTERFACE)
				if(hSyncInfo)
#else
				if(psSyncInfo)
#endif
				{
					PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "\r\n-- Last frame - poll for render to complete\r\n");
#if defined (SUPPORT_SID_INTERFACE)
					PDUMPSYNCPOL(psDevData->psConnection,
					             hSyncInfo,
					             IMG_FALSE);
#else
					PDUMPSYNCPOL(psDevData->psConnection,
					             psSyncInfo,
					             IMG_FALSE);
#endif
					PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "-- IDLE to make sure last event USE program has finished\r\nIDL 5000");
				}

				/* Dump a bitmap of the render. */
				if ((ui32KickFlags & (SGX_KICKTA_FLAGS_TERMINATE | SGX_KICKTA_FLAGS_ABORT)) != 0)
				{
					IMG_UINT32					ui32PDumpBitmapSize;
					PSGX_KICKTA_DUMPBITMAP	psPDumpBitmapArray;

					if (!psKickPDUMP)
					{
						ui32PDumpBitmapSize = 0;
						psPDumpBitmapArray = IMG_NULL;
					}
					else
					{
						ui32PDumpBitmapSize = psKickPDUMP->ui32PDumpBitmapSize;
						psPDumpBitmapArray = psKickPDUMP->psPDumpBitmapArray;
					}

					/*
						Dump a bitmap of the render
					*/
					PDumpAfterRender(psDevData,
									 psRenderContext,
									 &psRenderContext->ui32RenderNum,
									 IMG_TRUE,
									 ui32PDumpBitmapSize,
									 psPDumpBitmapArray,
									 IMG_FALSE);
				}

				/* Unless we are forcing synchronization do this only on the last render. */
				if (
					((ui32KickFlags & SGX_KICKTA_FLAGS_ABORT) == 0) &&
					((psRenderContext->ui32TAKickRegFlags & (PVRSRV_SGX_TAREGFLAGS_TACOMPLETESYNC | PVRSRV_SGX_TAREGFLAGS_RENDERSYNC)) == 0)
					)
				{
					PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection, 0, "}LASTONLY\r\n");
				}

				/* Reset the TA count ready for next render */
				psRenderContext->ui32KickTANum = 0;
			}
		}
#else
#if defined(DEBUG)
		if ((psRenderContext->ui32TAKickRegFlags & PVRSRV_SGX_TAREGFLAGS_TACOMPLETESYNC) != 0)
		{
			/* Wait for TA kick to complete */
			PVRSRVPollForValue(psDevData->psConnection,
							  psRenderContext->hOSEvent,
							  psRenderContext->psTACCB->pui32ReadOffset,
							  *psRenderContext->psTACCB->pui32WriteOffset,
							  0xFFFFFFFF,
							  MAX_HW_TIME_US/WAIT_TRY_COUNT,
							  WAIT_TRY_COUNT);
		}

		if (psRenderContext->ui32TAKickRegFlags & PVRSRV_SGX_TAREGFLAGS_RENDERSYNC)
		{
			if ((psKickSubmit->ui32KickFlags & (SGX_KICKTA_FLAGS_TERMINATE | SGX_KICKTA_FLAGS_ABORT)) != 0)
			{
				/* Wait for the render to complete */
#if defined (SUPPORT_SID_INTERFACE)
				if(hSyncInfo)
#else
				if(psSyncInfo)
#endif
				{
					WaitForRender(psDevData,
								  psRenderContext,
#if defined (SUPPORT_SID_INTERFACE)
								  hSyncInfo,
#else
								  psSyncInfo,
#endif
								  MAX_HW_TIME_US/WAIT_TRY_COUNT,
								  WAIT_TRY_COUNT);
				}
			}
		}
#endif
#endif /* PDUMP */

	}

#if defined(NO_HARDWARE)
	/* Need to do CPU writeback of op completes if NO_HARDWARE */
	if ((psKickSubmit->bTerminateOrAbort) != 0)
	{
		if ((ui32KickFlags & SGX_KICKTA_FLAGS_MIDSCENETERM) == 0)
		{
			/* Free render details */
			*psKickSubmit->psRenderDetails->pui32Lock = 0;
#if defined(PDUMP)
			psKickSubmit->psRTDataSet->psLastFreedRenderDetails = psKickSubmit->psRenderDetails;
#endif
		}
		if ((psKickSubmit->psDstDeviceSyncList) != 0)
		{
#if defined(PDUMP)
			psKickSubmit->psRTDataSet->psLastFreedDeviceSyncList = psKickSubmit->psDstDeviceSyncList;
#endif
			*psKickSubmit->psDstDeviceSyncList->pui32Lock = 0;
		}
	}
#endif	/* #if defined(NO_HARDWARE) */

SubmitExit:
	/* Release Lock before exiting */
	PVRSRVUnlockMutex(psRenderContext->hMutex);

	return eError;
}

/******************************************************************************
 End of file (sgxkick_client.c)
******************************************************************************/

