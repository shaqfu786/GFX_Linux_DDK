/*!****************************************************************************
@File           sgxtransfer_context.c

@Title          Device specific transfer queue routines

@Author         Imagination Technologies

@date           08/02/06

@Copyright      Copyright 2007-2010 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                King's Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       Generic

@Description    Device specific functions

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-
$Log: sgxtransfer_context.c $
******************************************************************************/

#if defined(TRANSFER_QUEUE)
#include <stddef.h>
#include "img_types.h"
#include "services.h"
#include "servicesext.h"
#include "sgxapi.h"
#include "servicesint.h"
#include "sgxpdsdefs.h"
#include "pvr_debug.h"
#include "pixevent.h"
#include "pvr_bridge.h"
#include "sgxinfo.h"
#include "sgxinfo_client.h"
#include "sgxtransfer_client.h"
#include "sgxtransfer_buffer.h"
#include "sgx_bridge_um.h"
#include "pds.h"
#include "sgxutils_client.h"
#include "pdump_um.h"
#include "usecodegen.h"
#include "osfunc_client.h"

#include "pds_tq_0src_primary.h"
#include "pds_tq_1src_primary.h"
#include "pds_tq_2src_primary.h"
#include "pds_tq_3src_primary.h"
#include "pds_iter_primary.h"

#include "pds_secondary.h"
#include "pds_dma_secondary.h"
#include "pds_1attr_secondary.h"
#include "pds_3attr_secondary.h"
#include "pds_4attr_secondary.h"
#include "pds_4attr_dma_secondary.h"
#include "pds_5attr_dma_secondary.h"
#include "pds_6attr_secondary.h"
#include "pds_7attr_secondary.h"

#include "transferqueue_use.h"
#include "transferqueue_use_labels.h"
#include "subtwiddled_eot.h"
#include "subtwiddled_eot_labels.h"

#if defined(PDUMP)
/* PRQA S 0880,0881 20 */ /* ignore 'order of evaluation' warning */
#define SGXTQ_CREATE_ROPBLITCASE(rop, pas, tmps)							\
case SGXTQ_USEBLIT_ROP_##rop:												\
{																			\
	ui32Start = TQUSE_ROP##rop##BLIT_OFFSET;								\
	ui32Size = TQUSE_ROP##rop##BLITEND_OFFSET - ui32Start;					\
	psUSE->ui32NumTempRegs = tmps;											\
	psUSE->ui32NumLayers = (pas);											\
	pszPdumpComment = "TQ Rop " #rop " Blit USE fragment";					\
	break;																	\
}
#else /* PDUMP*/
#define SGXTQ_CREATE_ROPBLITCASE(rop, pas, tmps)							\
case SGXTQ_USEBLIT_ROP_##rop:												\
{																			\
	ui32Start = TQUSE_ROP##rop##BLIT_OFFSET;								\
	ui32Size = TQUSE_ROP##rop##BLITEND_OFFSET - ui32Start;					\
	psUSE->ui32NumTempRegs = tmps;											\
	psUSE->ui32NumLayers = pas;												\
	break;																	\
}
#endif /* PDUMP*/

/*****************************************************************************
 * Function Name		:	SGXTQ_CreateUSEResources
 * Inputs				:	psDevData	-
 * 							hDevMemHeap	- in which heap does the USE code go
 * 							eUSEFragId	- what type of USE code we need
 * Outputs				:	psTQContext	- to store the info about out USE code
 * Returns				:	Error code
 * Description			:	Allocates and writes the static USE shaders.
******************************************************************************/
static PVRSRV_ERROR SGXTQ_CreateUSEResources(PVRSRV_DEV_DATA	*psDevData,
#if defined (SUPPORT_SID_INTERFACE)
					      IMG_SID								hDevMemHeap,
#else
					      IMG_HANDLE							hDevMemHeap,
#endif
						  SGXTQ_CLIENT_TRANSFER_CONTEXT			*psTQContext,
						  SGXTQ_USEFRAGS						eUSEFragId)
{
	IMG_UINT32				ui32Start, ui32Size;
	SGXTQ_RESOURCE			** ppsResource = & psTQContext->apsUSEResources[eUSEFragId];
	SGXTQ_STATIC_RESOURCE	* psStaticRes;
	SGXTQ_USE_RESOURCE		* psUSE;
	PVRSRV_CLIENT_MEM_INFO	** ppsMemInfo;
#if defined(PDUMP)
	IMG_CHAR* pszPdumpComment;
#endif
	PVR_UNREFERENCED_PARAMETER(psDevData);

	if (IMG_NULL == (*ppsResource = PVRSRVAllocUserModeMem(sizeof(SGXTQ_RESOURCE))))
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	(*ppsResource)->eStorage = SGXTQ_STORAGE_STATIC;
	psStaticRes = & (*ppsResource)->uStorage.sStatic;
	(*ppsResource)->eResource = SGXTQ_USE;
	psUSE = &(*ppsResource)->uResource.sUSE;

	ppsMemInfo = & psStaticRes->psMemInfo;

	switch (eUSEFragId)
	{
		case SGXTQ_USEBLIT_NORMAL:
		{
			ui32Start = TQUSE_NORMALCPYBLIT_OFFSET;
			ui32Size = TQUSE_NORMALCPYBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ Normal Blit Copy USE fragment";
#endif
			break;
		}
		case SGXTQ_USEBLIT_SRC_BLEND:
		{
			ui32Start = TQUSE_SRCBLENDBLIT_OFFSET;
			ui32Size = TQUSE_SRCBLENDBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 2;
#if defined(PDUMP)
			pszPdumpComment = "TQ Src Blend Blit USE fragment";
#endif
			break;
		}
		case SGXTQ_USEBLIT_ACCUM_SRC_BLEND:
		{
			ui32Start = TQUSE_ACCUMSRCBLENDBLIT_OFFSET;
			ui32Size = TQUSE_ACCUMSRCBLENDBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ Accumlation object Src Blend Blit USE fragment";
#endif
			break;
		}
		case SGXTQ_USEBLIT_PREMULSRC_BLEND:
		{
			ui32Start = TQUSE_PREMULSRCBLENDBLIT_OFFSET;
			ui32Size = TQUSE_PREMULSRCBLENDBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 2;
#if defined(PDUMP)
			pszPdumpComment = "TQ Premul Src Blend Blit USE fragment";
#endif
			break;
		}
		case SGXTQ_USEBLIT_GLOBAL_BLEND:
		{
			ui32Start = TQUSE_GLOBALBLENDBLIT_OFFSET;
			ui32Size = TQUSE_GLOBALBLENDBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 2;
#if defined(PDUMP)
			pszPdumpComment = "TQ Global Blend Blit USE fragment";
#endif
			break;
		}
		case SGXTQ_USEBLIT_PREMULSRCWITHGLOBAL_BLEND:
		{
			ui32Start = TQUSE_PREMULSRCWITHGLOBALBLENDBLIT_OFFSET;
			ui32Size = TQUSE_PREMULSRCWITHGLOBALBLENDBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 1;
			psUSE->ui32NumLayers = 2;
#if defined(PDUMP)
			pszPdumpComment = "TQ Premul Src with Global Blend Blit USE fragment";
#endif
			break;
		}
		case SGXTQ_USEBLIT_A2R10G10B10:
		{
			ui32Start = TQUSE_A2R10G10B10BLIT_OFFSET;
			ui32Size = TQUSE_A2R10G10B10BLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 1;
			psUSE->ui32NumLayers = 2;
#if defined(PDUMP)
			pszPdumpComment = "TQ A2R10G10B10 USE fragment";
#endif
			break;
		}
		case SGXTQ_USEBLIT_A2B10G10R10:
		{
			ui32Start = TQUSE_A2B10G10R10BLIT_OFFSET;
			ui32Size = TQUSE_A2B10G10R10BLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 1;
			psUSE->ui32NumLayers = 2;
#if defined(PDUMP)
			pszPdumpComment = "TQ A2B10G10R10 Blit USE fragment";
#endif
			break;
		}
#if ! defined(SGX_FEATURE_USE_VEC34)
		case SGXTQ_USEBLIT_SRGB:
		{
			ui32Start = TQUSE_SRGBBLIT_OFFSET;
			ui32Size = TQUSE_SRGBBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 6;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ sRGB Blit USE fragment";
#endif
			break;
		}
#endif
#if defined(EURASIA_PDS_DOUTT0_CHANREPLICATE)
		/* Added special case for A8 Blits when channel replication is present: Fix for BRN 31145 */
		case SGXTQ_USEBLIT_A8:
		{
			ui32Start = TQUSE_A8BLIT_OFFSET;
			ui32Size = TQUSE_A8BLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ A8 Blit USE fragment";
#endif
			break;
		}
#endif
		case SGXTQ_USEBLIT_FILL:
		{
			ui32Start = TQUSE_FILLBLIT_OFFSET;
			ui32Size = TQUSE_FILLBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 0;

			/* Each fill blit needs its own storage in the TQ CB
			 * since fill colour will be patched into the program.
			 */
			(*ppsResource)->eStorage = SGXTQ_STORAGE_CB;
			(*ppsResource)->uStorage.sCB.psCB = psTQContext->psUSECodeCB;
			(*ppsResource)->uStorage.sCB.ui32Size = ui32Size;
			/* PRQA S 0311,3305 1 */ /* stricter alignment */
			(*ppsResource)->uStorage.sCB.pui32SrcAddr = (const IMG_PUINT32)&pbtransferqueue_use[ui32Start];

			return PVRSRV_OK;
		}
		case SGXTQ_USEBLIT_ROPFILL_AND:
		{
			ui32Start = TQUSE_FILLROPANDBLIT_OFFSET;
			ui32Size  = TQUSE_FILLROPANDBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ Rop And Blit USE fragment";
#endif
			break;
		}

		case SGXTQ_USEBLIT_ROPFILL_ANDNOT:
		{
			ui32Start = TQUSE_FILLROPANDNOTBLIT_OFFSET;
			ui32Size  = TQUSE_FILLROPANDNOTBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ Rop AndNot Blit USE fragment";
#endif
			break;
		}

		case SGXTQ_USEBLIT_ROPFILL_NOTAND:
		{
			ui32Start = TQUSE_FILLROPNOTANDBLIT_OFFSET;
			ui32Size  = TQUSE_FILLROPNOTANDBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ Rop NotAnd Blit USE fragment";
#endif
			break;
		}

		case SGXTQ_USEBLIT_ROPFILL_XOR:
		{
			ui32Start = TQUSE_FILLROPXORBLIT_OFFSET;
			ui32Size  = TQUSE_FILLROPXORBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ Rop Xor Blit USE fragment";
#endif
			break;
		}

		case SGXTQ_USEBLIT_ROPFILL_OR:
		{
			ui32Start = TQUSE_FILLROPORBLIT_OFFSET;
			ui32Size  = TQUSE_FILLROPORBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ Rop Or Blit USE fragment";
#endif
			break;
		}

		case SGXTQ_USEBLIT_ROPFILL_NOTANDNOT:
		{
			ui32Start = TQUSE_FILLROPNOTANDNOTBLIT_OFFSET;
			ui32Size  = TQUSE_FILLROPNOTANDNOTBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ Rop NotAndNot Blit USE fragment";
#endif
			break;
		}

		case SGXTQ_USEBLIT_ROPFILL_NOTXOR:
		{
			ui32Start = TQUSE_FILLROPNOTXORBLIT_OFFSET;
			ui32Size  = TQUSE_FILLROPNOTXORBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ Rop NotXor Blit USE fragment";
#endif
			break;
		}

		case SGXTQ_USEBLIT_ROPFILL_ORNOT:
		{
			ui32Start = TQUSE_FILLROPORNOTBLIT_OFFSET;
			ui32Size  = TQUSE_FILLROPORNOTBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ Rop OrNot Blit USE fragment";
#endif
			break;
		}

		case SGXTQ_USEBLIT_ROPFILL_NOT:
		{
			ui32Start = TQUSE_FILLROPNOTBLIT_OFFSET;
			ui32Size  = TQUSE_FILLROPNOTBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ Rop Not Blit USE fragment";
#endif
			break;
		}

		case SGXTQ_USEBLIT_ROPFILL_NOTOR:
		{
			ui32Start = TQUSE_FILLROPNOTORBLIT_OFFSET;
			ui32Size  = TQUSE_FILLROPNOTORBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ Rop NotOr Blit USE fragment";
#endif
			break;
		}

		case SGXTQ_USEBLIT_ROPFILL_NOTORNOT:
		{
			ui32Start = TQUSE_FILLROPNOTORNOTBLIT_OFFSET;
			ui32Size  = TQUSE_FILLROPNOTORNOTBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ Rop NotOrNot Blit USE fragment";
#endif
			break;
		}

		SGXTQ_CREATE_ROPBLITCASE(NOTSANDNOTD,	2, 1);
		SGXTQ_CREATE_ROPBLITCASE(NOTSANDD,		2, 0);
		SGXTQ_CREATE_ROPBLITCASE(NOTS,			1, 0);
		SGXTQ_CREATE_ROPBLITCASE(SANDNOTD,		2, 0);
		SGXTQ_CREATE_ROPBLITCASE(NOTD,			2, 0);
		SGXTQ_CREATE_ROPBLITCASE(SXORD,			2, 0);
		SGXTQ_CREATE_ROPBLITCASE(NOTSORNOTD,	2, 1);
		SGXTQ_CREATE_ROPBLITCASE(SANDD,			2, 0);
		SGXTQ_CREATE_ROPBLITCASE(NOTSXORD,		2, 0);
		SGXTQ_CREATE_ROPBLITCASE(D,				2, 0);
		SGXTQ_CREATE_ROPBLITCASE(NOTSORD,		2, 0);
		SGXTQ_CREATE_ROPBLITCASE(S,				1, 0);
		SGXTQ_CREATE_ROPBLITCASE(SORNOTD,		2, 0);
		SGXTQ_CREATE_ROPBLITCASE(SORD,			2, 0);

		case SGXTQ_USESECONDARY_UPDATE:
		{
			ui32Start = TQUSE_SECONDARYUPDATE_OFFSET;
			ui32Size = TQUSE_SECONDARYUPDATEEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 0;
#if defined(PDUMP)
			pszPdumpComment = "TQ USE pixel secondary program";
#endif
			break;
		}
		case SGXTQ_USESECONDARY_UPDATE_DMA:
		{
			ui32Start = TQUSE_SECONDARYUPDATEDMA_OFFSET;
			ui32Size = TQUSE_SECONDARYUPDATEDMAEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 0;
#if defined(PDUMP)
			pszPdumpComment = "TQ USE pixel secondary program for DMA(doutd) on PDS";
#endif
			break;
		}

#if ! defined(SGX_FEATURE_USE_VEC34)
		case SGXTQ_USEBLIT_LUT256:
		{
			ui32Start = TQUSE_COLOUR256LUTBLIT_OFFSET;
			ui32Size  = TQUSE_COLOUR256LUTBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 2;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ Colour table Lookup (byte addressed) blit";
#endif
			break;
		}

		case SGXTQ_USEBLIT_LUT16:
		{
			ui32Start = TQUSE_COLOUR16LUTBLIT_OFFSET;
			ui32Size  = TQUSE_COLOUR16LUTBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 3;
			psUSE->ui32NumLayers = 2;
#if defined(PDUMP)
			pszPdumpComment = "TQ Colour table Lookup (4 bit addressed) blit";
#endif
			break;
		}

		case SGXTQ_USEBLIT_LUT2:
		{
			ui32Start = TQUSE_COLOUR2LUTBLIT_OFFSET;
			ui32Size  = TQUSE_COLOUR2LUTBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 3;
			psUSE->ui32NumLayers = 2;
#if defined(PDUMP)
			pszPdumpComment = "TQ Colour table Lookup (bit addressed) blit";
#endif
			break;
		}
#endif /* SGX_FEATURE_USE_VEC34*/

		case SGXTQ_USEBLIT_SOURCE_COLOUR_KEY:
		{
			ui32Start = TQUSE_SOURCECOLOURKEYBLIT_OFFSET;
			ui32Size = TQUSE_SOURCECOLOURKEYBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 1;
			psUSE->ui32NumLayers = 2;
#if defined(PDUMP)
			pszPdumpComment = "TQ Source Colour Key Blit USE fragment";
#endif
			break;
		}

		case SGXTQ_USEBLIT_DEST_COLOUR_KEY:
		{
			ui32Start = TQUSE_DESTCOLOURKEYBLIT_OFFSET;
			ui32Size = TQUSE_DESTCOLOURKEYBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 1;
			psUSE->ui32NumLayers = 2;
#if defined(PDUMP)
			pszPdumpComment = "TQ Dest Colour Key Blit USE fragment";
#endif
			break;
		}

#if ! defined(SGX_FEATURE_USE_VEC34)
		case SGXTQ_USEBLIT_STRIDE_HIGHBPP:
			/* imlement me ! */
		case SGXTQ_USEBLIT_STRIDE:
		{
			ui32Start = TQUSE_STRIDEBLIT_OFFSET;
			ui32Size = TQUSE_STRIDEBLITEND_OFFSET - ui32Start;
			(*ppsResource)->eStorage = SGXTQ_STORAGE_CB;
			(*ppsResource)->uStorage.sCB.psCB = psTQContext->psUSECodeCB;
			(*ppsResource)->uStorage.sCB.ui32Size = ui32Size;
			/* PRQA S 0311,3305 1 */ /* stricter alignment */
			(*ppsResource)->uStorage.sCB.pui32SrcAddr = (const IMG_PUINT32)&pbtransferqueue_use[ui32Start];
			psUSE->ui32NumTempRegs = 7;
			psUSE->ui32NumLayers = 1;
			return PVRSRV_OK;
		}
#endif /* SGX_FEATURE_USE_VEC34*/

		case SGXTQ_USEBLIT_CLEARTYPEBLEND_GAMMA:
		{
			ui32Start = TQUSE_CLEARTYPEBLENDGAMMA_OFFSET;
			ui32Size = TQUSE_CLEARTYPEBLENDGAMMAEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 10;
			psUSE->ui32NumLayers = 2;
#if defined(PDUMP)
			pszPdumpComment = "ClearType Blend with Valid Gamma";
#endif	
			break;	
		}
		case SGXTQ_USEBLIT_CLEARTYPEBLEND_INVALIDGAMMA:
		{
			ui32Start = TQUSE_CLEARTYPEBLENDINVALIDGAMMA_OFFSET;
			ui32Size = TQUSE_CLEARTYPEBLENDINVALIDGAMMAEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 10;
			psUSE->ui32NumLayers = 2;	
#if defined(PDUMP)
			pszPdumpComment = "Cleartype Blend with Invalid Gamma USE fragment";
#endif	
			break;	
		}		

		case SGXTQ_USEBLIT_VIDEOPROCESSBLIT_3PLANAR:
		{
			ui32Start = TQUSE_VIDEOPROCESSBLIT3PLANAR_OFFSET;
			ui32Size = TQUSE_VIDEOPROCESSBLIT3PLANAREND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 3;
#if defined(PDUMP)
			pszPdumpComment = "TQ Video Process Blit (3 planar) USE fragment";
#endif
			break;
		}
		case SGXTQ_USEBLIT_VIDEOPROCESSBLIT_2PLANAR:
		{
			ui32Start = TQUSE_VIDEOPROCESSBLIT2PLANAR_OFFSET;
			ui32Size = TQUSE_VIDEOPROCESSBLIT2PLANAREND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 2;
#if defined(PDUMP)
			pszPdumpComment = "TQ Video Process Blit (2 planar) USE fragment";
#endif
			break;
		}
		case SGXTQ_USEBLIT_VIDEOPROCESSBLIT_PACKED:
		{
			ui32Start = TQUSE_VIDEOPROCESSBLITPACKED_OFFSET;
			ui32Size = TQUSE_VIDEOPROCESSBLITPACKEDEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ Video Process Blit (packed) USE fragment";
#endif
			break;
		}
		case SGXTQ_USEBLIT_ARGB2NV12_Y_PLANE:
		{
			ui32Start = TQUSE_ARGB2NV12YPLANE_OFFSET;
			ui32Size = TQUSE_ARGB2NV12YPLANEEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ ARGB2NV12 Y plane USE fragment";
#endif
			break;
		}
		case SGXTQ_USEBLIT_ARGB2NV12_UV_PLANE:
		{
			ui32Start = TQUSE_ARGB2NV12UVPLANE_OFFSET;
			ui32Size = TQUSE_ARGB2NV12UVPLANEEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ ARGB2NV12 UV plane USE fragment";
#endif
			break;
		}
		case SGXTQ_USEBLIT_ROPFILL_NOTD:
		{
			ui32Start = TQUSE_FILLROPNOTDBLIT_OFFSET ;
			ui32Size = TQUSE_FILLROPNOTDBLITEND_OFFSET - ui32Start;
			psUSE->ui32NumTempRegs = 0;
			psUSE->ui32NumLayers = 1;
#if defined(PDUMP)
			pszPdumpComment = "TQ Rop Invert Dst USE fragment";
#endif
			break;
		}


		default:
		{
			PVR_DPF((PVR_DBG_ERROR, "Unknown TQ USE fragment type."));
			return PVRSRV_ERROR_INVALID_PARAMS;
		}
	}

    if (ALLOCDEVICEMEM(psTQContext,
					   hDevMemHeap,
					   PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ,
					   ui32Size,
					   EURASIA_PDS_DOUTU_PHASE_START_ALIGN,
					   ppsMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to allocate device memory"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	(*ppsResource)->sDevVAddr = (*ppsMemInfo)->sDevVAddr;

	/*
	   Copy the USE code to the allocated memory
	*/
	PVRSRVMemCopy((IMG_UINT8 *) (*ppsMemInfo)->pvLinAddr, &pbtransferqueue_use[ui32Start], ui32Size);
#if defined(PDUMP)
	PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "%s", pszPdumpComment);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, *ppsMemInfo, 0, ui32Size, PDUMP_FLAGS_CONTINUOUS);
#endif
	return PVRSRV_OK;
}


/*****************************************************************************
 * Function Name		:	SGXTQ_CreatePDSPrimResources
 * Inputs				:	psDevData	-
 * 							hDevMemHeap	-
 * 							ePDSFragID	-
 * Outputs				:	psTQContext	-
 * Returns				:	Error code
 * Description			:
******************************************************************************/
static PVRSRV_ERROR SGXTQ_CreatePDSPrimResources(PVRSRV_DEV_DATA	*psDevData,
#if defined (SUPPORT_SID_INTERFACE)
					      IMG_SID									hDevMemHeap,
#else
					      IMG_HANDLE								hDevMemHeap,
#endif
						  SGXTQ_CLIENT_TRANSFER_CONTEXT				*psTQContext,
						  SGXTQ_PDSPRIMFRAGS						ePDSFragId)
{
	SGXTQ_RESOURCE		** ppsResource = & psTQContext->apsPDSPrimResources[ePDSFragId];
	SGXTQ_PDS_RESOURCE	*  psPDSResource;

	if (IMG_NULL == (*ppsResource = PVRSRVAllocUserModeMem(sizeof(SGXTQ_RESOURCE))))
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psPDSResource = &(*ppsResource)->uResource.sPDS;

	(*ppsResource)->eStorage = SGXTQ_STORAGE_CB;
	(*ppsResource)->uStorage.sCB.psCB = psTQContext->psPDSCodeCB;
	(*ppsResource)->eResource = SGXTQ_PDS;
	switch (ePDSFragId)
	{
		case SGXTQ_PDSPRIMFRAG_KICKONLY:
		{
			psPDSResource->ui32DataLen = PDS_TQ_KICKONLY_DATA_SEGMENT_SIZE;
			psPDSResource->ui32Attributes = 0;
			(*ppsResource)->uStorage.sCB.pui32SrcAddr = g_pui32PDSTQ_KICKONLY;
			(*ppsResource)->uStorage.sCB.ui32Size = sizeof(g_pui32PDSTQ_KICKONLY);
			break;
		}
		case SGXTQ_PDSPRIMFRAG_ITER:
		{
			psPDSResource->ui32DataLen = PDS_TQ_ITER_DATA_SEGMENT_SIZE;
			psPDSResource->ui32Attributes = 2;

			(*ppsResource)->uStorage.sCB.pui32SrcAddr = g_pui32PDSTQ_ITER;
			(*ppsResource)->uStorage.sCB.ui32Size = sizeof(g_pui32PDSTQ_ITER);
			break;
		}
		case SGXTQ_PDSPRIMFRAG_TWOSOURCE:
		{
			psPDSResource->ui32DataLen = PDS_TQ_2SRC_DATA_SEGMENT_SIZE;
			psPDSResource->ui32Attributes = 2;

			(*ppsResource)->uStorage.sCB.pui32SrcAddr = g_pui32PDSTQ_2SRC;
			(*ppsResource)->uStorage.sCB.ui32Size = sizeof(g_pui32PDSTQ_2SRC);
			break;
		}
		case SGXTQ_PDSPRIMFRAG_THREESOURCE:
		{
			psPDSResource->ui32DataLen = PDS_TQ_3SRC_DATA_SEGMENT_SIZE;
			psPDSResource->ui32Attributes = 3;

			(*ppsResource)->uStorage.sCB.pui32SrcAddr = g_pui32PDSTQ_3SRC;
			(*ppsResource)->uStorage.sCB.ui32Size = sizeof(g_pui32PDSTQ_3SRC);
			break;
		}
		case SGXTQ_PDSPRIMFRAG_SINGLESOURCE:
		{
			IMG_UINT32	ui32InstanceSize;
			IMG_UINT32	i;
			IMG_PBYTE	pbyLinAddr;

			psPDSResource->ui32DataLen = PDS_TQ_1SRC_DATA_SEGMENT_SIZE;
			psPDSResource->ui32Attributes = 1;
			(*ppsResource)->eStorage = SGXTQ_STORAGE_NBUFFER;
			(*ppsResource)->uStorage.sCB.ui32Size = sizeof(g_pui32PDSTQ_1SRC);

			ui32InstanceSize = ((sizeof(g_pui32PDSTQ_1SRC) + SGXTQ_PDS_CODE_GRANULARITY - 1) &
								~(SGXTQ_PDS_CODE_GRANULARITY - 1));


			/* Create the skeleton buffer */
			if (SGXTQ_CreateCB(psDevData,
							   psTQContext,
							   ui32InstanceSize * SGXTQ_PDSPRIMFRAG_SINGLESOURCE_INSTANCENUM,
							   SGXTQ_PDS_CODE_GRANULARITY,
							   IMG_TRUE,
							   IMG_FALSE,
							   hDevMemHeap,
							   "PDS - 1src",
							   &psTQContext->psPDSPrimFragSingleSNB) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "Failed to create TQ CB"));
				return PVRSRV_ERROR_OUT_OF_MEMORY;
			}

			(*ppsResource)->uStorage.sCB.psCB = psTQContext->psPDSPrimFragSingleSNB;
			pbyLinAddr = (IMG_PBYTE)psTQContext->psPDSPrimFragSingleSNB->psBufferMemInfo->pvLinAddr;
			for (i = 0; i < SGXTQ_PDSPRIMFRAG_SINGLESOURCE_INSTANCENUM; i++)
			{
				PVRSRVMemCopy(pbyLinAddr, & g_pui32PDSTQ_1SRC, sizeof(g_pui32PDSTQ_1SRC));
				pbyLinAddr += ui32InstanceSize;
			}
#if defined(PDUMP)
			/*Dump the whole buffer (we could skip the data segments and alignments)*/
			PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "TQ Single source PDS prim skeleton buffer");
			PDUMPMEM(psDevData->psConnection,
					IMG_NULL,
					psTQContext->psPDSPrimFragSingleSNB->psBufferMemInfo,
					0,
					ui32InstanceSize * SGXTQ_PDSPRIMFRAG_SINGLESOURCE_INSTANCENUM,
					PDUMP_FLAGS_CONTINUOUS);
#endif
			break;
		}
		default:
		{
			return PVRSRV_ERROR_INVALID_PARAMS;
		}
	}

	return PVRSRV_OK;
}



/*****************************************************************************
 * Function Name		:	SGXTQ_CreatePDSSecResources
 * Inputs				:	psDevData	-
 * 							hDevMemHeap	-
 * 							eUSEFragId	-
 * Outputs				:	psTQContext	-
 * Returns				:	Error code
 * Description			:
******************************************************************************/
static PVRSRV_ERROR SGXTQ_CreatePDSSecResources(PVRSRV_DEV_DATA	*psDevData,
#if defined (SUPPORT_SID_INTERFACE)
					      IMG_SID								hDevMemHeap,
#else
					      IMG_HANDLE							hDevMemHeap,
#endif
						  SGXTQ_CLIENT_TRANSFER_CONTEXT			*psTQContext,
						  SGXTQ_PDSSECFRAGS						ePDSFragId)
{
	SGXTQ_RESOURCE		** ppsResource = & psTQContext->apsPDSSecResources[ePDSFragId];
	SGXTQ_PDS_RESOURCE	*  psPDSResource;

#if defined(SGX_FEATURE_SECONDARY_REQUIRES_USE_KICK)
	PVR_UNREFERENCED_PARAMETER(psDevData);
	PVR_UNREFERENCED_PARAMETER(hDevMemHeap);
#else
#if !defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(psDevData);
#endif
#endif

	if (IMG_NULL == (*ppsResource = PVRSRVAllocUserModeMem(sizeof(SGXTQ_RESOURCE))))
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psPDSResource = &(*ppsResource)->uResource.sPDS;

	(*ppsResource)->eStorage = SGXTQ_STORAGE_CB;
	(*ppsResource)->uStorage.sCB.psCB = psTQContext->psPDSCodeCB;
	(*ppsResource)->eResource = SGXTQ_PDS;
	switch (ePDSFragId)
	{
		case SGXTQ_PDSSECFRAG_BASIC:
		{
#if ! defined(SGX_FEATURE_SECONDARY_REQUIRES_USE_KICK)
			SGXTQ_STATIC_RESOURCE	*psStorage;
			PVRSRV_CLIENT_MEM_INFO	**ppsMemInfo;
#endif

			psPDSResource->ui32DataLen = PDS_TQ_BASIC_DATA_SEGMENT_SIZE;
			psPDSResource->ui32Attributes = 0;
#if defined(SGX_FEATURE_SECONDARY_REQUIRES_USE_KICK)
			/* probably this needs an NBUFFER */
			(*ppsResource)->uStorage.sCB.pui32SrcAddr = g_pui32PDSTQ_BASIC;
			(*ppsResource)->uStorage.sCB.ui32Size = sizeof(g_pui32PDSTQ_BASIC);
#else
			(*ppsResource)->eStorage = SGXTQ_STORAGE_STATIC;

			psStorage = & (*ppsResource)->uStorage.sStatic;
			ppsMemInfo = & psStorage->psMemInfo;

			if (ALLOCDEVICEMEM(psTQContext,
							   hDevMemHeap,
							   PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ,
							   sizeof(g_pui32PDSTQ_BASIC),
							   (1 << EURASIA_PDS_BASEADD_ALIGNSHIFT),
							   ppsMemInfo) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "Failed to allocate device memory"));
				return PVRSRV_ERROR_OUT_OF_MEMORY;
			}

			PVRSRVMemCopy((*ppsMemInfo)->pvLinAddr,
					(const IMG_VOID *) &g_pui32PDSTQ_BASIC[0],
					sizeof(g_pui32PDSTQ_BASIC));

			(*ppsResource)->sDevVAddr = (*ppsMemInfo)->sDevVAddr;
#if defined(PDUMP)
			PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "TQ Basic PDS secondary program");
			PDUMPMEM(psDevData->psConnection,
					IMG_NULL,
					*ppsMemInfo,
					0,
					sizeof(g_pui32PDSTQ_BASIC),
					PDUMP_FLAGS_CONTINUOUS);
#endif
#endif
			break;
		}
		case SGXTQ_PDSSECFRAG_DMA_ONLY:
		{
			psPDSResource->ui32DataLen = PDS_TQ_DMA_ONLY_DATA_SEGMENT_SIZE;
			psPDSResource->ui32Attributes = 0;
			(*ppsResource)->uStorage.sCB.pui32SrcAddr = g_pui32PDSTQ_DMA_ONLY;
			(*ppsResource)->uStorage.sCB.ui32Size = sizeof(g_pui32PDSTQ_DMA_ONLY);
			break;
		}
		case SGXTQ_PDSSECFRAG_1ATTR:
		{
			psPDSResource->ui32DataLen = PDS_TQ_1ATTR_DATA_SEGMENT_SIZE;
			psPDSResource->ui32Attributes = 1;
			(*ppsResource)->uStorage.sCB.pui32SrcAddr = g_pui32PDSTQ_1ATTR;
			(*ppsResource)->uStorage.sCB.ui32Size = sizeof(g_pui32PDSTQ_1ATTR);
			break;
		}
		case SGXTQ_PDSSECFRAG_3ATTR:
		{
			psPDSResource->ui32DataLen = PDS_TQ_3ATTR_DATA_SEGMENT_SIZE;
			psPDSResource->ui32Attributes = 3;
			(*ppsResource)->uStorage.sCB.pui32SrcAddr = g_pui32PDSTQ_3ATTR;
			(*ppsResource)->uStorage.sCB.ui32Size = sizeof(g_pui32PDSTQ_3ATTR);
			break;
		}
		case SGXTQ_PDSSECFRAG_4ATTR:
		{
			psPDSResource->ui32DataLen = PDS_TQ_4ATTR_DATA_SEGMENT_SIZE;
			psPDSResource->ui32Attributes = 4;
			(*ppsResource)->uStorage.sCB.pui32SrcAddr = g_pui32PDSTQ_4ATTR;
			(*ppsResource)->uStorage.sCB.ui32Size = sizeof(g_pui32PDSTQ_4ATTR);
			break;
		}
		case SGXTQ_PDSSECFRAG_4ATTR_DMA:
		{
			psPDSResource->ui32DataLen = PDS_TQ_4ATTR_DMA_DATA_SEGMENT_SIZE;
			psPDSResource->ui32Attributes = 4;
			(*ppsResource)->uStorage.sCB.pui32SrcAddr = g_pui32PDSTQ_4ATTR_DMA;
			(*ppsResource)->uStorage.sCB.ui32Size = sizeof(g_pui32PDSTQ_4ATTR_DMA);
			break;
		}
		case SGXTQ_PDSSECFRAG_5ATTR_DMA:
		{
			psPDSResource->ui32DataLen = PDS_TQ_5ATTR_DMA_DATA_SEGMENT_SIZE;
			psPDSResource->ui32Attributes = 5;
			(*ppsResource)->uStorage.sCB.pui32SrcAddr = g_pui32PDSTQ_5ATTR_DMA;
			(*ppsResource)->uStorage.sCB.ui32Size = sizeof(g_pui32PDSTQ_5ATTR_DMA);
			break;
		}
		case SGXTQ_PDSSECFRAG_6ATTR:
		{
			psPDSResource->ui32DataLen = PDS_TQ_6ATTR_DATA_SEGMENT_SIZE;
			psPDSResource->ui32Attributes = 6;
			(*ppsResource)->uStorage.sCB.pui32SrcAddr = g_pui32PDSTQ_6ATTR;
			(*ppsResource)->uStorage.sCB.ui32Size = sizeof(g_pui32PDSTQ_6ATTR);
			break;
		}
		case SGXTQ_PDSSECFRAG_7ATTR:
		{
			psPDSResource->ui32DataLen = PDS_TQ_7ATTR_DATA_SEGMENT_SIZE;
			psPDSResource->ui32Attributes = 7;
			(*ppsResource)->uStorage.sCB.pui32SrcAddr = g_pui32PDSTQ_7ATTR;
			(*ppsResource)->uStorage.sCB.ui32Size = sizeof(g_pui32PDSTQ_7ATTR);
			break;
		}
		default:
		{
			PVR_DPF((PVR_DBG_ERROR, "Unknown TQ PDS secondary."));
			return PVRSRV_ERROR_INVALID_PARAMS;
		}
	}
	return PVRSRV_OK;
}

/*****************************************************************************
 * Function Name		:	SGXTQ_CreateISPResources
 * Inputs				:	psDevData		-
 * 							hDevMemHeap		- in which heap does the CtlObj go
 *							ui32NumLayers	- number of input texture layers.
 *											  0->(SGXTQ_NUM_HWBGOBJS-1) supported.
 * Outputs				:	psDevVAddr		- to store the info about the CtlObj
 * 							ppsMemInfo		- to store the info about the CtlObj
 * Returns				:	Error code
 * Description			:	Allocate and set up the Ctl objects for fast scale renders
*****************************************************************************/
static PVRSRV_ERROR SGXTQ_CreateISPResources(PVRSRV_DEV_DATA			* psDevData,
#if defined (SUPPORT_SID_INTERFACE)
										  IMG_SID						hDevMemHeap,
#else
										  IMG_HANDLE					hDevMemHeap,
#endif
										  SGXTQ_CLIENT_TRANSFER_CONTEXT * psTQContext,
										  IMG_UINT32					ui32NumLayers)
{
	SGXTQ_RESOURCE	** ppsResource;

	PVR_UNREFERENCED_PARAMETER(psDevData);
	PVR_UNREFERENCED_PARAMETER(hDevMemHeap);

	PVR_ASSERT(ui32NumLayers < SGXTQ_NUM_HWBGOBJS);
	ppsResource = & psTQContext->apsISPResources[ui32NumLayers];           /* PRQA S 3689 */ /* array not out of bounds */

	if (IMG_NULL == (*ppsResource = PVRSRVAllocUserModeMem(sizeof(SGXTQ_RESOURCE))))
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	(*ppsResource)->eStorage = SGXTQ_STORAGE_CB;
	(*ppsResource)->eResource = SGXTQ_STREAM;

	return PVRSRV_OK;
}

/*****************************************************************************
 * Function Name		:	SGXTQ_CreateFast2DISPControlStream
 * Inputs				:	psDevData	-
 * 							hDevMemHeap	- in which heap does the CtlObj go
 * Outputs				:	psDevVAddr	- to store the info about the CtlObj
 * 							ppsMemInfo	- to store the info about the CtlObj
 * Returns				:	Error code
 * Description			:	Creates the skeleton of an ISP ctrl stream used for
 *							fast 2d renders.
*****************************************************************************/
static PVRSRV_ERROR SGXTQ_CreateFast2DISPControlStream(PVRSRV_DEV_DATA			* psDevData,
#if defined (SUPPORT_SID_INTERFACE)
										  IMG_SID								hDevMemHeap,
#else
										  IMG_HANDLE							hDevMemHeap,
#endif
										  SGXTQ_CLIENT_TRANSFER_CONTEXT			* psTQContext)
{
	SGXTQ_RESOURCE	** ppsResource;

	PVR_UNREFERENCED_PARAMETER(psDevData);
	PVR_UNREFERENCED_PARAMETER(hDevMemHeap);

	ppsResource = & psTQContext->psFast2DISPControlStream;

	if (IMG_NULL == (*ppsResource = PVRSRVAllocUserModeMem(sizeof(SGXTQ_RESOURCE))))
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	(*ppsResource)->eStorage = SGXTQ_STORAGE_CB;
	(*ppsResource)->eResource = SGXTQ_STREAM;

	return PVRSRV_OK;
}

/*****************************************************************************
 * Function Name		:	SGXTQ_CreateEndOfRender
 * Inputs				:	hDevMemHeap	- in which heap does the Pixel Event program go
 * Outputs				:	psDevVAddr	- to store information about our EOR program
 *							ppsMemInfo	- the base of our EOR program
 * Returns				:	Error code
 * Description			:	Creates a basic version of USE EoR handler.
*****************************************************************************/
static PVRSRV_ERROR SGXTQ_CreateUSEEORHandler(PVRSRV_DEV_DATA	*psDevData,
#if defined (SUPPORT_SID_INTERFACE)
						      IMG_SID							hDevMemHeap,
#else
						      IMG_HANDLE						hDevMemHeap,
#endif
							  SGXTQ_CLIENT_TRANSFER_CONTEXT		*psTQContext)
{
	SGXTQ_RESOURCE			** ppsResource = & psTQContext->psUSEEORHandler;

	SGXTQ_STATIC_RESOURCE	* psStorage;
	SGXTQ_USE_RESOURCE		* psUSEResource;
	PVRSRV_CLIENT_MEM_INFO	** ppsMemInfo;
	PVR_UNREFERENCED_PARAMETER(psDevData);


	if (IMG_NULL == (*ppsResource = PVRSRVAllocUserModeMem(sizeof(SGXTQ_RESOURCE))))
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	(*ppsResource)->eStorage = SGXTQ_STORAGE_STATIC;
	(*ppsResource)->eResource = SGXTQ_USE;

	psStorage = & (*ppsResource)->uStorage.sStatic;
	psUSEResource = & (*ppsResource)->uResource.sUSE;
	ppsMemInfo = & psStorage->psMemInfo;

	psUSEResource->ui32NumTempRegs = 1;

	if (ALLOCDEVICEMEM(psTQContext,
					   hDevMemHeap,
					   PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ,
					   USE_PIXELEVENT_EOR_BYTES,
					   EURASIA_PDS_DOUTU_PHASE_START_ALIGN,
					   ppsMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to allocate device memory"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	(*ppsResource)->sDevVAddr = (*ppsMemInfo)->sDevVAddr;

	WriteEndOfRenderUSSECode((IMG_UINT32 *) (*ppsMemInfo)->pvLinAddr);

	PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "TQ EOR program");
	PDUMPMEM(psDevData->psConnection, IMG_NULL, *ppsMemInfo, 0, USE_PIXELEVENT_EOR_BYTES, PDUMP_FLAGS_CONTINUOUS);
	return PVRSRV_OK;
}


static PVRSRV_ERROR SGXTQ_CreateUSEEOTHandlers(PVRSRV_DEV_DATA	*psDevData,
#if defined (SUPPORT_SID_INTERFACE)
					      IMG_SID								hDevMemHeap,
#else
					      IMG_HANDLE							hDevMemHeap,
#endif
						  SGXTQ_CLIENT_TRANSFER_CONTEXT			*psTQContext,
						  SGXTQ_USEEOTHANDLER					eEot)
{
	SGXTQ_RESOURCE		** ppsResource = & psTQContext->apsUSEEOTHandlers[eEot];
	SGXTQ_USE_RESOURCE	* psUSEResource;

#if ! defined(SGXTQ_SUBTILE_TWIDDLING)
	PVR_UNREFERENCED_PARAMETER(psDevData);
	PVR_UNREFERENCED_PARAMETER(hDevMemHeap);
#endif

	if (IMG_NULL == (*ppsResource = PVRSRVAllocUserModeMem(sizeof(SGXTQ_RESOURCE))))
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	(*ppsResource)->eStorage = SGXTQ_STORAGE_CB;
	(*ppsResource)->eResource = SGXTQ_USE;
	psUSEResource = & (*ppsResource)->uResource.sUSE;
	(*ppsResource)->uStorage.sCB.psCB = psTQContext->psUSECodeCB;

	switch (eEot)
	{
		case SGXTQ_USEEOTHANDLER_BASIC:
		{
			psUSEResource->ui32NumTempRegs = USE_PIXELEVENT_NUM_TEMPS;
			/* this one comes from codegen */
			(*ppsResource)->uStorage.sCB.ui32Size = USE_PIXELEVENT_EOT_BYTES;
			break;
		}
#if defined(SGXTQ_SUBTILE_TWIDDLING)
		case SGXTQ_USEEOTHANDLER_SUBTWIDDLED:
		{
			IMG_UINT32	ui32InstanceSize;
			IMG_UINT32	i;
			IMG_PBYTE	pbyLinAddr;
			IMG_UINT32	ui32Start = TQUSE_SUBTWIDDLED_OFFSET;
			IMG_UINT32	ui32Size = TQUSE_SUBTWIDDLEDEND_OFFSET - ui32Start;


#if defined(SGX_FEATURE_UNIFIED_STORE_64BITS)
			psUSEResource->ui32NumTempRegs = 14;
#else
#if defined(SGX_FEATURE_PIXELBE_32K_LINESTRIDE)
			psUSEResource->ui32NumTempRegs = 13;
#else
			psUSEResource->ui32NumTempRegs = 12;
#endif
#endif
			(*ppsResource)->eStorage = SGXTQ_STORAGE_NBUFFER;
			(*ppsResource)->uStorage.sCB.ui32Size = ui32Size;

			ui32InstanceSize = ((ui32Size + SGXTQ_USSE_CODE_GRANULARITY - 1) &
								~(SGXTQ_USSE_CODE_GRANULARITY - 1));

			/* this is roughly a page; we allow it to cross pages*/

			/* Create the skeleton buffer */
			if (SGXTQ_CreateCB(psDevData,
							   psTQContext,
							   ui32InstanceSize * SGXTQ_USEEOTHANDLER_SUBTWIDDLED_INSTANCENUM,
							   SGXTQ_USSE_CODE_GRANULARITY,
							   IMG_TRUE,
							   IMG_FALSE,
							   hDevMemHeap,
							   "USSE - subtwiddled EOT",
							   &psTQContext->psUSEEOTSubTwiddledSNB) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "Failed to create TQ CB"));
				return PVRSRV_ERROR_OUT_OF_MEMORY;
			}

			(*ppsResource)->uStorage.sCB.psCB = psTQContext->psUSEEOTSubTwiddledSNB;
			pbyLinAddr = (IMG_PBYTE)psTQContext->psUSEEOTSubTwiddledSNB->psBufferMemInfo->pvLinAddr;
			for (i = 0; i < SGXTQ_USEEOTHANDLER_SUBTWIDDLED_INSTANCENUM; i++)
			{
				PVRSRVMemCopy(pbyLinAddr, & pbsubtwiddled_eot[ui32Start], ui32Size);
				pbyLinAddr += ui32InstanceSize;
			}
#if defined(PDUMP)
			/*Dump the whole buffer */
			PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "TQ sub tile twiddling skeleton buffer");
			PDUMPMEM(psDevData->psConnection,
					IMG_NULL,
					psTQContext->psUSEEOTSubTwiddledSNB->psBufferMemInfo,
					0,
					ui32InstanceSize * SGXTQ_USEEOTHANDLER_SUBTWIDDLED_INSTANCENUM,
					PDUMP_FLAGS_CONTINUOUS);
#endif
			break;
		}
#endif
		default:
		{
			return PVRSRV_ERROR_INVALID_PARAMS;
		}
	}

	return PVRSRV_OK;
}

/*****************************************************************************
 * Function Name		:	SGXTQ_CreatePixeventHandlers
 * Inputs				:	hDevMemHeap		- in which heap does the Pixel Event program go
 * Outputs				:	psTQContext - to store information about our PE program
 * Returns				:	Error code
 * Description			:
*****************************************************************************/
static PVRSRV_ERROR SGXTQ_CreatePDSPixeventHandlers(PVRSRV_DEV_DATA	*psDevData,
#if defined (SUPPORT_SID_INTERFACE)
						      IMG_SID								hDevMemHeap,
#else
						      IMG_HANDLE							hDevMemHeap,
#endif
							  SGXTQ_PDSPIXEVENTHANDLER				ePixev,
							  SGXTQ_CLIENT_TRANSFER_CONTEXT			*psTQContext)
{
	SGXTQ_RESOURCE	** ppsResource = & psTQContext->apsPDSPixeventHandlers[ePixev];

	PVR_UNREFERENCED_PARAMETER(psDevData);
	PVR_UNREFERENCED_PARAMETER(hDevMemHeap);

	if (IMG_NULL == (*ppsResource = PVRSRVAllocUserModeMem(sizeof(SGXTQ_RESOURCE))))
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	(*ppsResource)->eStorage = SGXTQ_STORAGE_CB;
	(*ppsResource)->uStorage.sCB.psCB = psTQContext->psPDSCodeCB;
	(*ppsResource)->eResource = SGXTQ_PDS;

	switch (ePixev)
	{
		case SGXTQ_PDSPIXEVENTHANDLER_BASIC:
		{
			/* from codegen */
			/* this one definitly needs a skeleton buffer, but codegen doesn't
			 * support the concept.
			 */
			SGXTQ_PDS_RESOURCE* psPDSResource = &(*ppsResource)->uResource.sPDS;

			psPDSResource->ui32Attributes = PDS_PIXEVENT_NUM_PAS;
			(*ppsResource)->uStorage.sCB.ui32Size = PDSGetPixeventProgSize();
			break;
		}
		case SGXTQ_PDSPIXEVENTHANDLER_TILEXY:
		{
			SGXTQ_PDS_RESOURCE* psPDSResource = &(*ppsResource)->uResource.sPDS;

			psPDSResource->ui32Attributes = 1;
			(*ppsResource)->uStorage.sCB.ui32Size = PDSGetPixeventTileXYProgSize();
			break;
		}
		default:
		{
			return PVRSRV_ERROR_INVALID_PARAMS;
		}
	}

	return PVRSRV_OK;
}

/*****************************************************************************
 * Function Name		:	SGXCreateTransferContext
 * Inputs				:	psDevData			-
 *							ui32RequestedSBSize	- the requested staging buffer size ( 0 to disable )
 * 							psCreateTransfer	- the creation hints coming from the client
 * Outputs				: 	phTransferContext	- the returned handle for the created TC
 * Returns				:	Error code
 * Description			:	Context creation
*****************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV SGXCreateTransferContext(PVRSRV_DEV_DATA				* psDevData,
												   IMG_UINT32					ui32RequestedSBSize,
												   SGX_TRANSFERCONTEXTCREATE	* psCreateTransfer,
												   IMG_HANDLE					* phTransferContext)
{
	SGXTQ_CLIENT_TRANSFER_CONTEXT	*psTQContext;
	PSGXMKIF_HWTRANSFERCONTEXT		psHWTransferContext;
#if defined(SGX_FEATURE_2D_HARDWARE)
	PSGXMKIF_HW2DCONTEXT			psHW2DContext;
#endif
	IMG_UINT32						ui32HeapCount;
	IMG_UINT32						i;
	PVRSRV_HEAP_INFO				asHeapInfo[PVRSRV_MAX_CLIENT_HEAPS];

	SGXTQ_HEAP_INFO					sTQHeapInfo = {IMG_NULL};
	PVRSRV_ERROR					eError;

	PVR_DPF ((PVR_DBG_MESSAGE, "Creating transfer context."));

	if (psDevData == IMG_NULL)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* Setup of context shell */
	psTQContext = PVRSRVCallocUserModeMem(sizeof(SGXTQ_CLIENT_TRANSFER_CONTEXT));
	if (psTQContext == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXCreateTransferContext: Couldn't allocate memory for Context!"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	/* memset the context so we can destroy a half created one*/
	PVRSRVMemSet(psTQContext, 0, sizeof(SGXTQ_CLIENT_TRANSFER_CONTEXT));

	/* ALLOCDEVICEMEM macro needs psTQContext->psDevData to be
	 * initialized early, for non-Windows platforms.
	 */
	psTQContext->psDevData = psDevData;

	if(PVRSRVGetDeviceMemHeapInfo(psDevData,
									psCreateTransfer->hDevMemContext,
									&ui32HeapCount,
									asHeapInfo) !=  PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXGetClientDevInfo: Failed to retrieve device "
				 "memory context information\n"));
		return PVRSRV_ERROR_FAILED_TO_RETRIEVE_HEAPINFO;
	}

	psTQContext->hDevMemContext = psCreateTransfer->hDevMemContext;
#if defined(SUPPORT_KERNEL_SGXTQ)
	psTQContext->hCallbackHandle = psCreateTransfer->hCallbackHandle;
#endif

	for(i=0; i<ui32HeapCount; i++)
	{
		if (HEAP_IDX(asHeapInfo[i].ui32HeapID) == SGXTQ_SYNCINFO_HEAP_ID)
		{
			sTQHeapInfo.hSyncMemHeap = asHeapInfo[i].hDevMemHeap;
		}
		if (HEAP_IDX(asHeapInfo[i].ui32HeapID) == SGXTQ_PDS_HEAP_ID)
		{
			sTQHeapInfo.hPDSMemHeap = asHeapInfo[i].hDevMemHeap;
#if !defined(SGX_FEATURE_PIXEL_PDSADDR_FULL_RANGE)
			psTQContext->sPDSExecBase = asHeapInfo[i].sDevVAddrBase;
#endif
		}
		if (HEAP_IDX(asHeapInfo[i].ui32HeapID) == SGXTQ_USSE_HEAP_ID)
		{
			sTQHeapInfo.hUSEMemHeap = asHeapInfo[i].hDevMemHeap;
			psTQContext->sUSEExecBase = asHeapInfo[i].sDevVAddrBase;
		}
		if (HEAP_IDX(asHeapInfo[i].ui32HeapID) == SGXTQ_KERNEL_DATA_ID)
		{
			sTQHeapInfo.hCtrlMemHeap = asHeapInfo[i].hDevMemHeap;
		}
		if (HEAP_IDX(asHeapInfo[i].ui32HeapID) == SGXTQ_BGOBJ_HEAP_ID)
		{
			sTQHeapInfo.hParamMemHeap = asHeapInfo[i].hDevMemHeap;
		}
		if (HEAP_IDX(asHeapInfo[i].ui32HeapID) == SGXTQ_BUFFER_HEAP_ID)
		{
			sTQHeapInfo.hBufferMemHeap = asHeapInfo[i].hDevMemHeap;
		}
	}

#if defined(BLITLIB)
	/* create the blitlib synchronization mod-obj pool (MAX_SURFACES + 1) instance*/
	for (i = 0; i <= SGXTQ_MAX_SURFACES; i++)
	{
		eError = PVRSRVCreateSyncInfoModObj(psDevData->psConnection, & psTQContext->ahSyncModObjPool[i]);
		if (eError != PVRSRV_OK)
		{
			goto ErrorExit;
		}
	}
#endif

	/* Create the staging buffer */
	if (ui32RequestedSBSize != 0)
	{
		IMG_UINT32 ui32StagingBufferSize;

		/* if the requested size if smaller than the min, set to min */
		if (ui32RequestedSBSize < SGXTQ_STAGINGBUFFER_MIN_SIZE)
			ui32StagingBufferSize = SGXTQ_STAGINGBUFFER_MIN_SIZE;
		else
		{
			ui32StagingBufferSize = ui32RequestedSBSize + SGXTQ_STAGINGBUFFER_ALLOC_GRAN;
			ui32StagingBufferSize = ui32StagingBufferSize & ~(SGXTQ_STAGINGBUFFER_ALLOC_GRAN - 1);
		}

		PVR_DPF((PVR_DBG_MESSAGE, "Staging Buffer Size: 0x%x", ui32StagingBufferSize));

		if (SGXTQ_CreateCB(psDevData,
						   psTQContext,
						   ui32StagingBufferSize,
						   SGXTQ_STAGINGBUFFER_ALLOC_GRAN,
						   IMG_TRUE,
						   IMG_TRUE,
						   sTQHeapInfo.hBufferMemHeap,
						   "texture - SB",
						   &psTQContext->psStagingBuffer) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to create TQ CB"));
			goto ErrorExit;
		}
	}

	/* Attempt to create the CCB for TRANSFERCMDs*/
	if (CreateCCB(psDevData,
				  SGX_CCB_SIZE,
				  SGX_CCB_ALLOCGRAN,
				  MAX(1024, sizeof(SGXMKIF_TRANSFERCMD)),
				  sTQHeapInfo.hCtrlMemHeap,
				  &psTQContext->psTransferCCB) != PVRSRV_OK)
	{
		goto ErrorExit;
	}

	PDUMPCOMMENT(psDevData->psConnection,
				 "Initialise TransferContext CCB control structure",
				 IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL,
			 psTQContext->psTransferCCB->psCCBCtlClientMemInfo, 0,
			 sizeof(PVRSRV_SGX_CCB_CTL), PDUMP_FLAGS_CONTINUOUS);

	/* Allocate the TA/TQ dependency sync object */
	if (ALLOCDEVICEMEM(psTQContext,
					   sTQHeapInfo.hSyncMemHeap,
					   PVRSRV_MEM_READ | PVRSRV_MEM_WRITE,
					   sizeof(IMG_UINT32),
					   0x10,
					   &psTQContext->psTASyncObjectMemInfo) != PVRSRV_OK)
    {
        PVR_DPF((PVR_DBG_ERROR,"ERROR - Failed to alloc memory for TASyncObject!"));
        goto ErrorExit;
    }

	psTQContext->psTASyncObject = psTQContext->psTASyncObjectMemInfo->psClientSyncInfo;

	/* Allocate the 3D/TQ dependency sync object */
	if (ALLOCDEVICEMEM(psTQContext,
					   sTQHeapInfo.hSyncMemHeap,
					   PVRSRV_MEM_READ | PVRSRV_MEM_WRITE,
					   sizeof(IMG_UINT32),
					   0x10,
					   &psTQContext->ps3DSyncObjectMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"ERROR - Failed to alloc memory for 3DSyncObject!"));
		goto ErrorExit;
	}

	psTQContext->ps3DSyncObject = psTQContext->ps3DSyncObjectMemInfo->psClientSyncInfo;

	/*
	 * Create the CBs. This must be done before any other Create*() call.
	 */
	if (SGXTQ_CreateCB(psDevData,
					   psTQContext,
					   SGXTQ_USSE_CODE_CBSIZE,
					   SGXTQ_USSE_CODE_GRANULARITY,
					   IMG_FALSE,
					   IMG_FALSE,
					   sTQHeapInfo.hUSEMemHeap,
					   "USSE code",
					   &psTQContext->psUSECodeCB) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to create TQ CB"));
		goto ErrorExit;
	}

	if (SGXTQ_CreateCB(psDevData,
					   psTQContext,
					   SGXTQ_ISP_STREAM_CBSIZE,
					   SGXTQ_ISP_STREAM_GRANULARITY,
					   IMG_FALSE,
					   IMG_FALSE,
					   sTQHeapInfo.hParamMemHeap,
					   "ISP stream",
					   &psTQContext->psStreamCB) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to create TQ CB"));
		goto ErrorExit;
	}

	/* set the 3D req base to the buffer base dropping the align bits*/
	psTQContext->sISPStreamBase.uiAddr = psTQContext->psStreamCB->psBufferMemInfo->sDevVAddr.uiAddr
		& ~ ((1U << EUR_CR_BIF_3D_REQ_BASE_ADDR_ALIGNSHIFT) - 1U);

	if (SGXTQ_CreateCB(psDevData,
					   psTQContext,
					   SGXTQ_PDS_CODE_CBSIZE,
					   SGXTQ_PDS_CODE_GRANULARITY,
					   IMG_FALSE,
					   IMG_FALSE,
					   sTQHeapInfo.hPDSMemHeap,
					   "PDS code",
					   &psTQContext->psPDSCodeCB) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to create TQ CB"));
		goto ErrorExit;
	}

	/*
		Note, TQCreate* functions are responsible for allocating their own
		device memory (as we don't know how much they require
	*/

	/* EOR USE handler */
	if (SGXTQ_CreateUSEEORHandler(psDevData,
								  sTQHeapInfo.hUSEMemHeap,
								  psTQContext) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXCreateTransferContext: Failed to allocate memory for End of Render program!"));
		goto ErrorExit;
	}

	/*
	 * Create EOT USE handlers.
	 */
	{
		SGXTQ_USEEOTHANDLER handler;

		/* PRQA S 1481 1 */ /* ignore enum increment warning */
		for (handler = SGXTQ_USEEOTHANDLER_BASIC; handler < SGXTQ_NUM_USEEOTHANDLERS; handler++)
		{
			if (SGXTQ_CreateUSEEOTHandlers(psDevData,
										  sTQHeapInfo.hUSEMemHeap,
										  psTQContext,
										  handler) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SGXCreateTransferContext: Couldn't create the USE EOT handlers"));
				goto ErrorExit;
			}
		}
	}

	/*
	 * Create ISP resources
	 */
	for (i = 0; i < SGXTQ_NUM_HWBGOBJS; i++)
	{
		if (SGXTQ_CreateISPResources(psDevData,
									  sTQHeapInfo.hParamMemHeap,
									  psTQContext,
									  i) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SGXCreateTransferContext: Couldn't create HW Background object!"));
			goto ErrorExit;
		}
	}

	/*
	 * Fast 2d ISP resource
	 */
	if (SGXTQ_CreateFast2DISPControlStream(psDevData,
								  sTQHeapInfo.hParamMemHeap,
								  psTQContext) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXCreateTransferContext: Failed to allocate memory for fast 2d ISP control stream"));
		goto ErrorExit;
	}

	/*
	 * Setup the pixel event programs
	 */
	for (i = 0; i < SGXTQ_NUM_PDSPIXEVENTHANDLERS; i++)
	{
		/* PRQA S 1482 4 */ /* ignore cast to enum type warning */
		if (SGXTQ_CreatePDSPixeventHandlers(psDevData,
									sTQHeapInfo.hPDSMemHeap,
									(SGXTQ_PDSPIXEVENTHANDLER)i,
									psTQContext) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "SGXCreateTransferContext: Failed to allocate memory for Pixel event program"));
			goto ErrorExit;
		}
	}

	/* Initialize the FenceID*/
	if (ALLOCDEVICEMEM(psTQContext,
					   sTQHeapInfo.hCtrlMemHeap,
					   PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ | PVRSRV_MEM_EDM_PROTECT | PVRSRV_MEM_CACHE_CONSISTENT,
					   sizeof(IMG_UINT32),
					   sizeof(IMG_UINT32),
					   &psTQContext->psFenceIDMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXCreateTransferContext: Couldn't allocate memory for the Fence ID!"));
		goto ErrorExit;
	}
	* (volatile IMG_UINT32 *)psTQContext->psFenceIDMemInfo->pvLinAddr = 0;
	psTQContext->ui32FenceID = 0;
	PDUMPCOMMENT(psDevData->psConnection, "Initialise the TQ fence id", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection,
			IMG_NULL,
			psTQContext->psFenceIDMemInfo,
			0,
			sizeof(IMG_UINT32),
			PDUMP_FLAGS_CONTINUOUS);

	/*
	 * Create USSE Resources
	 */
	for (i = 0; i < SGXTQ_NUM_USEFRAGS; i++)
	{
		/* PRQA S 1482 4 */ /* ignore cast to enum type warning */
		if (SGXTQ_CreateUSEResources(psDevData,
					sTQHeapInfo.hUSEMemHeap,
					psTQContext,
					(SGXTQ_USEFRAGS)i) != PVRSRV_OK)
		{
			goto ErrorExit;
		}
	}



	/*
	 * Create the primary PDS resources
	 */
	for (i = 0; i < SGXTQ_NUM_PDSPRIMFRAGS; i++)
	{
		/* PRQA S 1482 4 */ /* ignore cast to enum type warning */
		if (SGXTQ_CreatePDSPrimResources(psDevData,
					sTQHeapInfo.hPDSMemHeap,
					psTQContext,
					(SGXTQ_PDSPRIMFRAGS)i) != PVRSRV_OK)
		{
			goto ErrorExit;
		}
	}
	/*
	 * Create the secondary PDS resources
	 */
	for (i = 0; i < SGXTQ_NUM_PDSSECFRAGS; i++)
	{
		/* PRQA S 1482 4 */ /* ignore cast to enum type warning */
		if (SGXTQ_CreatePDSSecResources(psDevData,
					sTQHeapInfo.hPDSMemHeap,
					psTQContext,
					(SGXTQ_PDSSECFRAGS)i) != PVRSRV_OK)
		{
			goto ErrorExit;
		}
	}

    psHWTransferContext = PVRSRVAllocUserModeMem(sizeof(SGXMKIF_HWTRANSFERCONTEXT));
    if (psHWTransferContext == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXCreateTransferContext: Couldn't allocate memory for HWTransferContext!"));
		goto ErrorExit;
	}

	/* Initialise HW render context */
	psHWTransferContext->sCommon.ui32Flags = SGXMKIF_HWCFLAGS_NEWCONTEXT;
	psHWTransferContext->sCommon.sPDDevPAddr.uiAddr = IMG_NULL; /* this will be setup in srvkm later */
	psHWTransferContext->sCCBBaseDevAddr = psTQContext->psTransferCCB->psCCBClientMemInfo->sDevVAddr;
	psHWTransferContext->sCCBCtlDevAddr.uiAddr = psTQContext->psTransferCCB->psCCBCtlClientMemInfo->sDevVAddr.uiAddr;
	psHWTransferContext->ui32Count = 0;
	psHWTransferContext->sPrevDevAddr.uiAddr = 0;
	psHWTransferContext->sNextDevAddr.uiAddr = 0;

#if defined(SUPPORT_SGX_PRIORITY_SCHEDULING)
	/* set the context scheduling priority to medium */
	psHWTransferContext->sCommon.ui32Priority = 1;
#else
	/* We only have 1 priority */
	psHWTransferContext->sCommon.ui32Priority = 0;
#endif
	/* Save the PID that created the transfer context */
	psHWTransferContext->ui32PID = PVRSRVGetCurrentProcessID();

#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH) && defined(SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH)
	for(i = 0; i < SGX_FEATURE_MP_CORE_COUNT_3D; i++)
	{
		IMG_UINT32	j;
		for(j = 0; j < SGX_FEATURE_NUM_PDS_PIPES; j++)
		{
			/*
			 * allocate memory for the PDS state
			 */
			if (ALLOCDEVICEMEM(psTQContext,
							   sTQHeapInfo.hCtrlMemHeap,
							   PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ,
							   EURASIA_PDS_CONTEXT_SWITCH_BUFFER_SIZE,
							   (1 << EUR_CR_PDS0_CONTEXT_STORE_ADDRESS_SHIFT),
							   &psTQContext->apsPDSCtxSwitchMemInfo[i][j]) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "SGXCreateTransferContext: Failed to alloc PDS Ctx Switch stream"));	
				goto ErrorExit;
			}
	
			PVRSRVMemSet(psTQContext->apsPDSCtxSwitchMemInfo[i][j]->pvLinAddr,
					0,
					EURASIA_PDS_CONTEXT_SWITCH_BUFFER_SIZE);
	
			/* let the ukernel access it*/
			psHWTransferContext->asPDSStateBaseDevAddr[i][j].uiAddr =
											psTQContext->apsPDSCtxSwitchMemInfo[i][j]->sDevVAddr.uiAddr | \
											EUR_CR_PDS0_CONTEXT_STORE_TILE_ONLY_MASK | \
											EUR_CR_PDS0_CONTEXT_STORE_DISABLE_MASK;
	
			PDUMPCOMMENT(psDevData->psConnection, "TQ PDS ctx Switch memory", IMG_TRUE);
			PDUMPMEM(psDevData->psConnection, IMG_NULL,
					psTQContext->apsPDSCtxSwitchMemInfo[i][j], 0, 
					EURASIA_PDS_CONTEXT_SWITCH_BUFFER_SIZE, PDUMP_FLAGS_CONTINUOUS);
		}
	}
#endif

	/* Register the HW transfer context for cleanup */
	eError = SGXRegisterHWTransferContext(
                psDevData, 
                &psTQContext->hHWTransferContext, 
                (IMG_CPU_VIRTADDR *)psHWTransferContext,
                sizeof(SGXMKIF_HWTRANSFERCONTEXT),
                offsetof(SGXMKIF_HWTRANSFERCONTEXT, sCommon.sPDDevPAddr),
                psCreateTransfer->hDevMemContext,
                &psTQContext->sHWTransferContextDevVAddr);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXCreateTransferContext: failed to register hw transfer context!"));
		goto ErrorExit;
	}

#if defined(SGX_FEATURE_2D_HARDWARE)
	/* Attempt to create the CCB for 2DCMDS*/
	if (CreateCCB(psDevData,
				  SGX_CCB_SIZE,
				  SGX_CCB_ALLOCGRAN,
				  MAX(1024, sizeof(SGXMKIF_2DCMD)),
				  sTQHeapInfo.hCtrlMemHeap,
				  &psTQContext->ps2DCCB) != PVRSRV_OK)
	{
		goto ErrorExit;
	}

    psHW2DContext = PVRSRVAllocUserModeMem(sizeof(SGXMKIF_HW2DCONTEXT));
    if (psHW2DContext == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXCreateTransferContext: Couldn't allocate memory for HW2DContext!"));
		goto ErrorExit;
	}

	/* Initialise HW 2D context */
	psHW2DContext->sCommon.ui32Flags = SGXMKIF_HWCFLAGS_NEWCONTEXT;//0;
	psHW2DContext->sCCBBaseDevAddr = psTQContext->ps2DCCB->psCCBClientMemInfo->sDevVAddr;
	psHW2DContext->sCCBCtlDevAddr.uiAddr = psTQContext->ps2DCCB->psCCBCtlClientMemInfo->sDevVAddr.uiAddr;
	psHW2DContext->ui32Count = 0;
	psHW2DContext->sPrevDevAddr.uiAddr = 0;
	psHW2DContext->sNextDevAddr.uiAddr = 0;
	psHW2DContext->sCommon.sPDDevPAddr.uiAddr = psHWTransferContext->sCommon.sPDDevPAddr.uiAddr;
	psHW2DContext->sCommon.ui32Priority = psHWTransferContext->sCommon.ui32Priority;

	/* Save the PID the created the transfer context */
	psHW2DContext->ui32PID = PVRSRVGetCurrentProcessID();

	/* Register the HW 2D context for cleanup */
	eError = SGXRegisterHW2DContext(
                psDevData, 
                &psTQContext->hHW2DContext, 
                (IMG_CPU_VIRTADDR *)psHW2DContext,
                sizeof(SGXMKIF_HW2DCONTEXT),
                offsetof(SGXMKIF_HW2DCONTEXT, sCommon.sPDDevPAddr),
                psCreateTransfer->hDevMemContext,
                &psTQContext->sHW2DContextDevVAddr);

    PVRSRVFreeUserModeMem(psHW2DContext);
    psHW2DContext = IMG_NULL;

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXCreateTransferContext: failed to register hw 2D context!"));
		goto ErrorExit;
	}

#endif /* defined(SGX_FEATURE_2D_HARDWARE) */

    PVRSRVFreeUserModeMem(psHWTransferContext);
    psHWTransferContext = IMG_NULL;

	/* Create the mutex */
	eError = PVRSRVCreateMutex(&psTQContext->hMutex);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXCreateTransferContext: Failed to create render context mutex (%d)", eError));
		goto ErrorExit;
	}

	/* setup the event object */
	psTQContext->hOSEvent = psCreateTransfer->hOSEvent;

	*phTransferContext = (IMG_HANDLE) psTQContext;

	return PVRSRV_OK;

ErrorExit:
	SGXDestroyTransferContext (psTQContext, CLEANUP_WITH_POLL);

	return PVRSRV_ERROR_OUT_OF_MEMORY;
}

static IMG_VOID SGXTQ_FreeResourceArray(SGXTQ_CLIENT_TRANSFER_CONTEXT	*psTQContext,
										  SGXTQ_RESOURCE				** apsResources,
										  IMG_UINT32					ui32NumResources)
{
	IMG_UINT32 i;

	for (i = 0; i < ui32NumResources; i++)
	{
		if (apsResources[i])
		{
			if (apsResources[i]->eStorage == SGXTQ_STORAGE_STATIC)
			{
				SGXTQ_STATIC_RESOURCE *psResource = & apsResources[i]->uStorage.sStatic;
				if (psResource->psMemInfo)
				{
					FREEDEVICEMEM(psTQContext,
							psResource->psMemInfo);
				}
			}
			else if (apsResources[i]->eStorage == SGXTQ_STORAGE_NBUFFER)
			{
				SGXTQ_DestroyCB(psTQContext, apsResources[i]->uStorage.sCB.psCB);
			}
			PVRSRVFreeUserModeMem(apsResources[i]);
		}
	}
}


/* Destruction stuff */
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV SGXDestroyTransferContext(IMG_HANDLE hTransferContext,
													IMG_BOOL bForceCleanup)
{
	SGXTQ_CLIENT_TRANSFER_CONTEXT *psTQContext;
	PVRSRV_ERROR	eError;
	PVRSRV_DEV_DATA *psDevData;

	PVR_DPF((PVR_DBG_MESSAGE, "Destroying transfer context"));

	if (!hTransferContext)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXDestroyTransferContext: Called with NULL context.  Ignoring"));
		return PVRSRV_OK;
	}

	psTQContext = (SGXTQ_CLIENT_TRANSFER_CONTEXT *) hTransferContext;

	psDevData = psTQContext->psDevData;

    /* Tell the uKernel that we've finished with the transfer context. */
    eError = SGXUnregisterHWTransferContext(psDevData, psTQContext->hHWTransferContext, bForceCleanup);

    if (eError != PVRSRV_OK)
    {
        PVR_DPF((PVR_DBG_ERROR, "Failed to unregister HW transfer context (%d)", eError));
    }

#if defined(BLITLIB)
	{
		IMG_UINT32	i;

		for (i = 0; i <= SGXTQ_MAX_SURFACES; i++)
		{
			if (psTQContext->ahSyncModObjPool[i] != IMG_NULL)
			{
				PVRSRVDestroySyncInfoModObj(psDevData->psConnection, psTQContext->ahSyncModObjPool[i]);
			}
		}
	}
#endif

#if defined(SGX_FEATURE_2D_HARDWARE)
	if (psTQContext->hHW2DContext != IMG_NULL)
	{
		/* Tell the uKernel that we've finished with the 2D context. */
		eError = SGXUnregisterHW2DContext(psDevData, psTQContext->hHW2DContext, bForceCleanup);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to unregister HW 2D context (%d)", eError));
		}
	}
	if (psTQContext->ps2DCCB != IMG_NULL)
	{
		DestroyCCB(psTQContext->psDevData, psTQContext->ps2DCCB);
	}
#endif

	/* freeing the Fence ID*/
	if (psTQContext->psFenceIDMemInfo)
	{
		FREEDEVICEMEM(psTQContext,
							psTQContext->psFenceIDMemInfo);
	}

	/* destroying the staging buffer */
	if (psTQContext->psStagingBuffer != IMG_NULL)
	{
		SGXTQ_DestroyCB(psTQContext, psTQContext->psStagingBuffer);
	}

	/* destroying the PDS Code CB */
	if (psTQContext->psPDSCodeCB != IMG_NULL)
	{
		SGXTQ_DestroyCB(psTQContext, psTQContext->psPDSCodeCB);
	}

	/* destroying the USSE Code CB */
	if (psTQContext->psUSECodeCB != IMG_NULL)
	{
		SGXTQ_DestroyCB(psTQContext, psTQContext->psUSECodeCB);
	}

	/* destroying the ISP stream CB */
	if (psTQContext->psStreamCB != IMG_NULL)
	{
		SGXTQ_DestroyCB(psTQContext, psTQContext->psStreamCB);
	}

	/* deallocating USSE resources */
	SGXTQ_FreeResourceArray(psTQContext, psTQContext->apsUSEResources, SGXTQ_NUM_USEFRAGS);

	/* deallocating PDS resources */
	SGXTQ_FreeResourceArray(psTQContext, psTQContext->apsPDSPrimResources, SGXTQ_NUM_PDSPRIMFRAGS);
	SGXTQ_FreeResourceArray(psTQContext, psTQContext->apsPDSSecResources, SGXTQ_NUM_PDSSECFRAGS);

	/* freeing ISP resources */
	SGXTQ_FreeResourceArray(psTQContext, psTQContext->apsISPResources, SGXTQ_NUM_HWBGOBJS);
	SGXTQ_FreeResourceArray(psTQContext, & psTQContext->psFast2DISPControlStream, 1);

	/* freeing PDS pixevent handlers */
	SGXTQ_FreeResourceArray(psTQContext, psTQContext->apsPDSPixeventHandlers, SGXTQ_NUM_PDSPIXEVENTHANDLERS);

	/* USE pixevent */
	SGXTQ_FreeResourceArray(psTQContext, & psTQContext->psUSEEORHandler, 1);
	SGXTQ_FreeResourceArray(psTQContext, psTQContext->apsUSEEOTHandlers, SGXTQ_NUM_USEEOTHANDLERS);

#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH) && defined(SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH)
	{
		IMG_UINT32		i;
		for(i = 0; i < SGX_FEATURE_MP_CORE_COUNT_3D; i++)
		{
			IMG_UINT32	j;
			for(j = 0; j < SGX_FEATURE_NUM_PDS_PIPES; j++)
			{
				if (psTQContext->apsPDSCtxSwitchMemInfo[i][j])
				{
					FREEDEVICEMEM(psTQContext,
							psTQContext->apsPDSCtxSwitchMemInfo[i][j]);
				}
			}
		}
	}
#endif

	if(psTQContext->psTransferCCB)
	{
		DestroyCCB(psTQContext->psDevData, psTQContext->psTransferCCB);
	}

	if(psTQContext->psTASyncObjectMemInfo)
	{
		FREEDEVICEMEM(psTQContext, psTQContext->psTASyncObjectMemInfo);
	}

	if(psTQContext->ps3DSyncObjectMemInfo)
	{
		FREEDEVICEMEM(psTQContext, psTQContext->ps3DSyncObjectMemInfo);
	}

	if (psTQContext->hMutex != IMG_NULL)
	{
		(IMG_VOID) PVRSRVDestroyMutex(psTQContext->hMutex);
	}

	PVRSRVFreeUserModeMem(psTQContext);

	return PVRSRV_OK;
}

#endif /* TRANSFER_QUEUE */
/******************************************************************************
 End of file (sgxtransfer_context.c)
******************************************************************************/
