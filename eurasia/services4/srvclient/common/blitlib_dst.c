/*!****************************************************************************
@File           blitlib_dst.c

@Title         	Blit library - texture write objects

@Author         Imagination Technologies

@date           03/09/2009

@Copyright      Copyright 2009 by Imagination Technologies Limited.
				All rights reserved. No part of this software, either material
				or conceptual may be copied or distributed, transmitted,
				transcribed, stored in a retrieval system or translated into
				any human or computer language in any form by any means,
				electronic, mechanical, manual or otherwise, or disclosed
				to third parties without the express written permission of
				Imagination Technologies Limited, Home Park Estate,
				Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       Generic

 ******************************************************************************/

/******************************************************************************
Modifications :-
$Log: blitlib_dst.c $
 *******************************************************************************
Renamed from blitlib_.c
 --- Revision Logs Removed --- 
 ******************************************************************************/

#include "blitlib_dst.h"
#include "blitlib_src.h"
#include "blitlib_common.h"
#include "services.h"
#include "pvr_debug.h"


#define TWIDDLED_X_MASK 0xAAAAAAAAUL
#define TWIDDLED_Y_MASK 0x55555555UL

#define HYBRID_TWIDDLED_LEN 4 /*log2(HYBRID_TWIDDLED_BOX_SIZE)*/
#define HYBRID_TWIDDLED_BOX_SIZE (1U << HYBRID_TWIDDLED_LEN)
#define HYBRID_TWIDDLED_MASK ((1U << (2*HYBRID_TWIDDLED_LEN)) - 1U)

/* undefine this to iterate the pixels always in row major order */
#define BL_DST_ITERATE_PIXELS_IN_ORDER


/**
 * Function Name	: _bl_reset_caches
 * Inputs			: psRootObject, psDownstreamMappingRect, ePipePxFmt, pfnSrcLinearGetPixel, pfnSrcTwiddledGetPixel
 * Outputs			: puiLinearSrcCount, puiTwiddledSrcCount
 * Returns			: PVRSRV_ERROR
 * Description		: Scans all the objects upstream recursively, calling the
 *					  pfnDoCaching function, if defined, with the updated
 *					  mapping rect and the pipe pixel format.
 *
 *					  It also counts the number of sources that have linear
 *					  and twiddled memory layouts.
 * 					  This is used by the destination surfaces at the start of
 * 					  the pipe, so the source surfaces and intermediate
 * 					  operations can cache the mapping transformations.
 */
static PVRSRV_ERROR _bl_reset_caches_and_count_layouts(BL_OBJECT *psObject,
                                                       IMG_RECT *psDownstreamMappingRect,
                                                       BL_INTERNAL_PX_FMT ePipePxFmt,
                                                       IMG_UINT *puiLinearSrcCount,
                                                       IMG_UINT *puiTwiddledSrcCount,
                                                       BL_ASSOC_FUNC pfnSrcLinearGetPixel,
                                                       BL_ASSOC_FUNC pfnSrcTwiddledGetPixel)
{
	IMG_UINT uiUpstreamIndex;
	IMG_RECT sUpstreamMappingRect;
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (psObject->pfnGetPixel == pfnSrcLinearGetPixel && puiLinearSrcCount)
	{
		(*puiLinearSrcCount)++;
	}
	else if (psObject->pfnGetPixel == pfnSrcTwiddledGetPixel && puiTwiddledSrcCount)
	{
		(*puiTwiddledSrcCount)++;
	}

	/* make a copy so the objects under this object in the pipe see the transformed
	 * mapping rect, but the objects in the same level still see the original.
	 */
	sUpstreamMappingRect = *psDownstreamMappingRect;

	if (psObject->pfnDoCaching != IMG_NULL)
	{
		/* the mapping rect can be modified here */
		eError = psObject->pfnDoCaching(psObject,
										   &sUpstreamMappingRect,
										   ePipePxFmt);
	}

	for (uiUpstreamIndex = 0;
		uiUpstreamIndex < BL_MAX_SOURCES && eError == PVRSRV_OK;
		uiUpstreamIndex++)
	{
		if (psObject->apsUpstreamObjects[uiUpstreamIndex] != IMG_NULL)
		{
			/* PRQA S 3670 3 */ /* ignore recursive warning */
			eError = _bl_reset_caches_and_count_layouts(psObject->apsUpstreamObjects[uiUpstreamIndex],
			                                            &sUpstreamMappingRect,
			                                            ePipePxFmt,
			                                            puiLinearSrcCount,
			                                            puiTwiddledSrcCount,
			                                            pfnSrcLinearGetPixel,
			                                            pfnSrcTwiddledGetPixel);
		}
	}
	return eError;
}

/**
 * Function Name	: _bl_needs_blending
 * Inputs			: psRootObject
 * Outputs			: Nothing
 * Returns			: IMG_BOOL
 * Description		: Scans all the objects upstream recursively to determine
 *					  if some operation requires to do some pixel blending.
 *
 * Returns IMG_TRUE if any object needs blending, IMG_FALSE otherwise.
 */
static IMG_BOOL _bl_needs_blending(BL_OBJECT * psRootObject)
{
	IMG_UINT uiUpstreamIndex;

	if (psRootObject->bDoesBlending)
	{
		return IMG_TRUE;
	}
	for (uiUpstreamIndex = 0; uiUpstreamIndex < BL_MAX_SOURCES; uiUpstreamIndex++)
	{
		/* PRQA S 3415,3670 2 */ /* ignore recursive warning */
		if ((psRootObject->apsUpstreamObjects[uiUpstreamIndex] != IMG_NULL) &&
			(_bl_needs_blending(psRootObject->apsUpstreamObjects[uiUpstreamIndex]) != IMG_FALSE))
		{
				return IMG_TRUE;
		}
	}
	return IMG_FALSE;
}

/**
 * Function Name	: _bl_get_src_external_fmt
 * Inputs			: psRootObject
 * Outputs			: Nothing
 * Returns			: PVRSRV_PIXEL_FORMAT
 * Description		: Scans all the objects upstream recursively to determine
 *					  which is the external format for the source objects.
 *
 * If there are various source objects, and their formats are not equals, then
 * returns PVRSRV_PIXEL_FORMAT_UNKNOWN.
 *
 * An object is considered a source object if it has no upstream objects linked.
 */
static PVRSRV_PIXEL_FORMAT _bl_get_src_external_fmt(BL_OBJECT * psRootObject)
{
	IMG_UINT uiUpstreamIndex;
	IMG_BOOL bSrc = IMG_TRUE;
	PVRSRV_PIXEL_FORMAT ePixelFormat = PVRSRV_PIXEL_FORMAT_UNKNOWN;

	for (uiUpstreamIndex = 0; uiUpstreamIndex < BL_MAX_SOURCES; uiUpstreamIndex++)
	{
		if (psRootObject->apsUpstreamObjects[uiUpstreamIndex] != IMG_NULL)
		{
			/* PRQA S 3670 9 */ /* ignore recursive warning */
			if(bSrc)
			{
				bSrc = IMG_FALSE;
				ePixelFormat = _bl_get_src_external_fmt(psRootObject->apsUpstreamObjects[uiUpstreamIndex]);
			}
			else if (ePixelFormat != _bl_get_src_external_fmt(psRootObject->apsUpstreamObjects[uiUpstreamIndex]))
			{
				ePixelFormat = PVRSRV_PIXEL_FORMAT_UNKNOWN;
			}
		}
	}

	if (bSrc)
	{
		ePixelFormat = psRootObject->eExternalPxFmt;
	}
	return ePixelFormat;
}

/**
 * Function Name	: _bl_get_formats_and_writer
 * Inputs			: psDstObject, uiBytesPP, eOutInternalFmt
 * Outputs			: pePipeFormat, ppfnPxWrite
 * Returns			: Nothing
 * Description		: Scans all the objects upstream to determine wich is the
 *					  most appropriate intermediate format and chooses the right
 *					  writing function accordingly.
 *
 * For now, the pipe format will be RAW if the external formats of both sides of
 * the pipe are equal and no blending operations are required in between, or
 * the preferred internal format for the output function (ARGB16 for all by now)
 * otherwise.
 */
static PVRSRV_ERROR _bl_get_formats_and_writer(BL_OBJECT					*psDstObject,
											   IMG_UINT						uiBytesPP,
											   BL_INTERNAL_PX_FMT			eOutInternalFmt,
											   BL_INTERNAL_PX_FMT			*pePipeFormat,
											   BL_INTERNAL_FORMAT_RW_FUNC	*ppfnPxWrite)
{
	/*determine the pipe format*/
	/* PRQA S 3415 2 */ /* ignore side effects warning */
	if ((_bl_get_src_external_fmt(psDstObject) != psDstObject->eExternalPxFmt) ||
		(_bl_needs_blending(psDstObject) != IMG_FALSE))
	{
		/*an intermediate format is needed, get the preferred for the output
		 (format conversion will be done at src, if needed)*/
		*pePipeFormat = eOutInternalFmt;
		*ppfnPxWrite = gas_BLExternalPixelTable[psDstObject->eExternalPxFmt].pfnPxWrite;
	}
	else
	{
		/*the same external format is used for input and output and no blending
		 is needed, we can use a RAW copy*/
		switch (uiBytesPP)
		{
			case 1:
			{
				*pePipeFormat = BL_INTERNAL_PX_FMT_RAW8;
				break;
			}
			case 2:
			{
				*pePipeFormat = BL_INTERNAL_PX_FMT_RAW16;
				break;
			}
			case 3:
			{
				*pePipeFormat = BL_INTERNAL_PX_FMT_RAW24;
				break;
			}
			case 4:
			{
				*pePipeFormat = BL_INTERNAL_PX_FMT_RAW32;
				break;
			}
			case 8:
			{
				*pePipeFormat = BL_INTERNAL_PX_FMT_RAW64;
				break;
			}
			case 16:
			{
				*pePipeFormat = BL_INTERNAL_PX_FMT_RAW128;
				break;
			}
			default:
			{
				PVR_DBG_BREAK;
				return PVRSRV_ERROR_BLIT_SETUP_FAILED;
			}
		}
		
		/* PRQA S 3689 1 */ /* bounds not exceeded */
		*ppfnPxWrite = gas_BLInternalPixelTable[*pePipeFormat].pfnPxWriteRAW;
	}

	if(*ppfnPxWrite == IMG_NULL)
	{
		PVR_DBG_BREAK;
		return PVRSRV_ERROR_BLIT_SETUP_FAILED;
	}
	return PVRSRV_OK;
}

/**
 * Function Name	: _bl_unpack_planar_data
 * Inputs			: puPackedBuffer, psPlanarSurfaceInfo
 * Outputs			: puPlanarBuffer
 * Returns			: Nothing
 * Description		: Unpacks a packed pixel into a planar buffer.
 */
static IMG_VOID _bl_unpack_planar_data(BL_PTR 					puPlanarBuffer,
                                       BL_PTR 					puPackedBuffer,
                                       BL_PLANAR_SURFACE_INFO	*psPlanarSurfaceInfo)
{
	IMG_UINT	i;
	for (i = 0; i < psPlanarSurfaceInfo->uiNChunks; i++)
	{
#if 1 /*this is the same plane ordering*/
		PVRSRVMemCopy(puPlanarBuffer.by + i*(IMG_UINT32)psPlanarSurfaceInfo->i32ChunkStride,
		              puPackedBuffer.by + i*psPlanarSurfaceInfo->uiPixelBytesPerPlane,
		              psPlanarSurfaceInfo->uiPixelBytesPerPlane);
#else /*reversed plane ordering*/
		PVRSRVMemCopy(puPlanarBuffer.by + i*(IMG_UINT32)psPlanarSurfaceInfo->i32ChunkStride,
		              puPackedBuffer.by + (psPlanarSurfaceInfo->uiNChunks-i-1)*psPlanarSurfaceInfo->uiPixelBytesPerPlane,
		              psPlanarSurfaceInfo->uiPixelBytesPerPlane);
#endif
	}
}


/*
 * BL_DST_LINEAR
 */

/**
 * Function Name	: _bl_dst_linear_do
 * Inputs			: psObject, psCoords (ignored), psPixel (ignored)
 * Outputs			: Nothing
 * Returns			: Nothing
 * Description		: Writes the pixels in the mapping rect of the destination
 *					  surface, after having read them from the upstream object.
 */
static PVRSRV_ERROR _bl_dst_linear_do(BL_OBJECT * psObject,
									  BL_COORDS * psCoords,
									  BL_PIXEL * psPixel)
{
	PVRSRV_ERROR eError;
	BL_DST_LINEAR *psSelf = (BL_DST_LINEAR*) psObject;
	IMG_INT32 i32X, i32Y;
	IMG_UINT uiBytesPP = gas_BLExternalPixelTable[psObject->eExternalPxFmt].ui32BytesPerPixel;
	BL_COORDS sCoords;
	BL_PIXEL sPixel;
	BL_INTERNAL_PX_FMT eInternalFmt = gas_BLExternalPixelTable[psObject->eExternalPxFmt].eInternalFormat;
	BL_INTERNAL_PX_FMT ePipeFormat;
	BL_INTERNAL_FORMAT_RW_FUNC pfnPxWrite;

	PVR_UNREFERENCED_PARAMETER(psCoords);
	PVR_UNREFERENCED_PARAMETER(psPixel);

	eError = _bl_get_formats_and_writer(psObject, uiBytesPP, eInternalFmt, &ePipeFormat, &pfnPxWrite);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	/* call cache functions of all objects going through the pipe*/
	eError = _bl_reset_caches_and_count_layouts(psObject,
	                                            & psSelf->sClipRect,
	                                            ePipeFormat,
	                                            IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}
	
	for (i32Y = psSelf->sClipRect.y0; i32Y < psSelf->sClipRect.y1; i32Y++)
	{
		IMG_INT32 i32LineOff = i32Y * psSelf->i32ByteStride;

		sCoords.i32Y = i32Y;

		for (i32X = psSelf->sClipRect.x0; i32X < psSelf->sClipRect.x1; i32X++)
		{
			BL_PTR puFBAddr;

			sCoords.i32X = i32X;
			BL_OBJECT_PIPECALL(psSelf, 0, &sCoords, &sPixel);

			if (psSelf->sPlanarSurfaceInfo.bIsPlanar)
			{
				BL_PTR puTmp;
				IMG_BYTE abyPackedBuffer[BL_PIXEL_MAX_SIZE];
				puFBAddr.by = psSelf->pbyFBAddr + (IMG_UINT32)i32LineOff + ((IMG_UINT32)i32X * sizeof(IMG_UINT32));
				puTmp.by = abyPackedBuffer;
				pfnPxWrite(&sPixel, puTmp);
				_bl_unpack_planar_data(puFBAddr,
				                       puTmp,
				                       &psSelf->sPlanarSurfaceInfo);
			}
			else
			{
				puFBAddr.by = psSelf->pbyFBAddr + (IMG_UINT32)i32LineOff + ((IMG_UINT32)i32X * uiBytesPP);
				pfnPxWrite(&sPixel, puFBAddr);
			}
		}
	}
	return PVRSRV_OK;
}

/**
 * Function Name	: BLDSTLinearInit
 * Inputs			: psSelf, ui32Height, i32ByteStride, ePixelFormat,
 *					  psClipRect, pbyFBAddr, psUpstreamObject
 * Outputs			: psSelf
 * Returns			: Nothing
 * Description		: Initialises the Linear (Strided) Destination surface.
 */
IMG_INTERNAL IMG_VOID BLDSTLinearInit(BL_DST_LINEAR				*psSelf,
									  IMG_INT32					i32ByteStride,
									  IMG_UINT32				ui32Height,
									  PVRSRV_PIXEL_FORMAT		ePixelFormat,
									  IMG_RECT					*psClipRect,
									  IMG_PBYTE					pbyFBAddr,
									  BL_OBJECT					*psUpstreamObject,
									  BL_PLANAR_SURFACE_INFO	*psPlanarInfo)
{
	BLInitObject(&psSelf->sObject);
	BL_OBJECT_ASSOC_FUNC(psSelf) = &_bl_dst_linear_do;

	psSelf->ui32Height = ui32Height;
	psSelf->i32ByteStride = i32ByteStride;
	psSelf->sClipRect = *psClipRect;
	psSelf->pbyFBAddr = pbyFBAddr;
	psSelf->sObject.apsUpstreamObjects[0] = psUpstreamObject;
	psSelf->sObject.eExternalPxFmt = ePixelFormat;
	if (psPlanarInfo)
		psSelf->sPlanarSurfaceInfo = *psPlanarInfo;
	else
		psSelf->sPlanarSurfaceInfo.bIsPlanar = IMG_FALSE;
}


/*
 * BL_DST_TWIDDLED
 */

static IMG_UINT32 _bl_twiddle(IMG_UINT32 ui32X,
                              IMG_UINT32 ui32Y,
                              BL_DST_TWIDDLED *psSelf)
{
	IMG_UINT32 ui32xTmp, ui32yTmp;

	ui32xTmp = (BLTwiddleStretch(ui32X & psSelf->ui32TwiddlingMask) << 1) |
			((ui32X & ~psSelf->ui32TwiddlingMask) * psSelf->ui32LinearPartMultX);

	ui32yTmp = BLTwiddleStretch(ui32Y & psSelf->ui32TwiddlingMask) |
			((ui32Y & ~psSelf->ui32TwiddlingMask) * psSelf->ui32LinearPartMultY);

	return ui32xTmp + ui32yTmp;
}

/*
 * Iterates in row major order.
 *
 * Useful when the sources are in strided format.
 */
static IMG_VOID _bl_dst_twiddled_row_major_next(BL_COORDS *psCoords,
                                                IMG_UINT32	*pui32twiddled,
                                                BL_DST_TWIDDLED *psDstTwiddled)
{
	psCoords->i32X++;

	if (psCoords->i32X >= psDstTwiddled->sClipRect.x1)
	{
		psCoords->i32Y++;
		psCoords->i32X = psDstTwiddled->sClipRect.x0;
	}

	*pui32twiddled = _bl_twiddle((IMG_UINT32)psCoords->i32X, (IMG_UINT32)psCoords->i32Y, psDstTwiddled);
}

#ifdef BL_DST_ITERATE_PIXELS_IN_ORDER


/**
 * Function Name	: BLTwiddleUnStretch
 * Inputs			: - ui32Twiddled
 * Outputs			: pui32OddBits, pui32EvenBits
 * Description		: restores the twiddled bits of the Z coordinate to its
 * 					  original positions. Extra steps are needed to deal with
 * 					  the rest of the bits in the non twiddled part for non
 * 					  square surfaces.
 *
 */
static IMG_VOID _bl_twiddle_unstretch(IMG_UINT32 ui32Twiddled,
                                      IMG_UINT32 *pui32OddBits,
                                      IMG_UINT32 *pui32EvenBits)
{
	IMG_UINT32 ui32odd, ui32even;

	ui32odd  = (ui32Twiddled >> 1) & TWIDDLED_Y_MASK;
	ui32even = ui32Twiddled & TWIDDLED_Y_MASK;

	ui32even = (ui32even | (ui32even >> 1)) & 0x33333333UL;
	ui32odd  = (ui32odd  | (ui32odd  >> 1)) & 0x33333333UL;
	ui32even = (ui32even | (ui32even >> 2)) & 0x0f0f0f0fUL;
	ui32odd  = (ui32odd  | (ui32odd  >> 2)) & 0x0f0f0f0fUL;
	ui32even = (ui32even | (ui32even >> 4)) & 0x00ff00ffUL;
	ui32odd  = (ui32odd  | (ui32odd  >> 4)) & 0x00ff00ffUL;
	ui32even = (ui32even | (ui32even >> 8)) & 0x0000ffffUL;
	ui32odd  = (ui32odd  | (ui32odd  >> 8)) & 0x0000ffffUL;

	*pui32OddBits = ui32odd;
	*pui32EvenBits = ui32even;
}

/*
 * Function Name	: _bl_untwiddle
 * Inputs			: - ui32Twiddled
 * 					  - psDstTwiddled
 * Outputs			: psCoords
 * Description		: Converts a twiddled coordinate to cartesian coordiantes.
 */
static IMG_VOID _bl_untwiddle(BL_COORDS *psCoords,
                              IMG_UINT32 ui32Twiddled,
                              BL_DST_TWIDDLED *psDstTwiddled)
{
	IMG_UINT32 ui32XTw, ui32Ytw, ui32XLin = 0, ui32YLin = 0;

	/* start doing the expensive divisions first to hide the latency */
	if (psDstTwiddled->bHybridTwiddling)
	{
		ui32XLin = ((ui32Twiddled / psDstTwiddled->ui32LinearPartMultX) %
				psDstTwiddled->ui32LinearPartMultY) & ~psDstTwiddled->ui32TwiddlingMask;
		ui32YLin = (ui32Twiddled / psDstTwiddled->ui32LinearPartMultY) &
				~psDstTwiddled->ui32TwiddlingMask;
	}
	else
	{
		if (psDstTwiddled->ui32LinearPartMultX)
		{
			ui32XLin = (ui32Twiddled / psDstTwiddled->ui32LinearPartMultX) & ~psDstTwiddled->ui32TwiddlingMask;
		}
		if (psDstTwiddled->ui32LinearPartMultY)
		{
			ui32YLin = (ui32Twiddled / psDstTwiddled->ui32LinearPartMultY) & ~psDstTwiddled->ui32TwiddlingMask;
		}
	}

	_bl_twiddle_unstretch(ui32Twiddled,
	                      &ui32XTw,
	                      &ui32Ytw);

	ui32XTw &= psDstTwiddled->ui32TwiddlingMask;
	ui32Ytw &= psDstTwiddled->ui32TwiddlingMask;

	psCoords->i32X = (IMG_INT32)(ui32XTw | ui32XLin);
	psCoords->i32Y = (IMG_INT32)(ui32Ytw | ui32YLin);
}


/*
 * Iterates over a subrectangle that is in the same z-curve region.
 *
 * Useful when the sources are also twiddled and the pixels are contiguous in
 * memory.
 */
static IMG_VOID _bl_dst_twiddled_zorder_contiguous_next(BL_COORDS *psCoords,
                                                        IMG_UINT32 *pui32twiddled,
                                                        BL_DST_TWIDDLED *psDstTwiddled)
{
	(*pui32twiddled)++;
	_bl_untwiddle(psCoords, *pui32twiddled, psDstTwiddled);
	return;
}

/*
 * Returns true if a point is inside the rectangular clip region delimited by
 * minZ and maxZ.
 */
static INLINE IMG_BOOL _bl_packed_twiddled_is_inside_clipbox(IMG_UINT32	ui32CurZ,
                                                             IMG_UINT32	ui32MaskX,
                                                             IMG_UINT32	ui32MaskY,
                                                             IMG_UINT32	ui32zStart,
                                                             IMG_UINT32	ui32zEnd)
{
	return 	((ui32CurZ & ui32MaskX) >= (ui32zStart & ui32MaskX) &&
			 (ui32CurZ & ui32MaskY) >= (ui32zStart & ui32MaskY) &&
			 (ui32CurZ & ui32MaskX) <= (ui32zEnd   & ui32MaskX) &&
			 (ui32CurZ & ui32MaskY) <= (ui32zEnd   & ui32MaskY)) ? IMG_TRUE : IMG_FALSE;
}

/*
 * Returns true if no part of the z region in the box designated by the current
 * z coordinate and a step lies within the rectangular clip region delimited by
 * minZ and maxZ.
 */
static INLINE IMG_BOOL _bl_packed_twiddled_is_zregion_outside_clip(IMG_UINT32	ui32CurZ,
                                                                   IMG_UINT32	ui32StepMask,
                                                                   IMG_UINT32	ui32MaskX,
                                                                   IMG_UINT32	ui32MaskY,
                                                                   IMG_UINT32	ui32zStart,
                                                                   IMG_UINT32	ui32zEnd)
{
	IMG_UINT32 ui32MaxStep;
	IMG_UINT32 ui32MinStep;

	ui32MaxStep = ui32CurZ | ui32StepMask;
	ui32MinStep = ui32CurZ & ~ui32StepMask;

	return	((ui32MaxStep & ui32MaskX) < (ui32zStart & ui32MaskX) ||
			 (ui32MaxStep & ui32MaskY) < (ui32zStart & ui32MaskY) ||
			 (ui32MinStep & ui32MaskX) > (ui32zEnd   & ui32MaskX) ||
			 (ui32MinStep & ui32MaskY) > (ui32zEnd   & ui32MaskY)) ? IMG_TRUE : IMG_FALSE;
}

/*
 * Returns the next z coordinate greater than the current one inside of the
 * rectangular bounding box delimited by ui32MinZ and ui32MaxZ.
 *
 * Does a bisection search by scanning consecutive larger boxes until it hits
 * the clip region, then narrows the box and repeats until a box of size 1x1
 * lies within the clip region.
 *
 * I think the worst case time complexity is O(log(n)) being n size of the gap
 * in the Z curve between two regions.
 *
 * Works for packed twiddling only, NOT hybrid twiddling.
 */
static IMG_UINT32 _bl_packed_twiddled_skip_zregion(IMG_UINT32		ui32CurZ,
                                                   BL_DST_TWIDDLED	*psDstTwiddled)
{
	IMG_INT iStep = 0; /* the log2(size) of the current z box */
	IMG_BOOL bOutside = IMG_TRUE; /* if we are being called, assume we're already out */
	IMG_UINT uiMaskSize = 0;	/* PRQA S 3197 */ /* PVR_ASSERT requires this initialisation */
	IMG_UINT32 ui32Mask = 0;

	PVR_ASSERT(!psDstTwiddled->bHybridTwiddling);

	while (iStep > 0 || bOutside)
	{
		/* if this happens we've been called after the last pixel... */
		PVR_ASSERT(uiMaskSize < 8*sizeof(ui32CurZ));

		if (bOutside)
		{
			ui32CurZ |= ui32Mask;
			ui32CurZ++;
			iStep++;
		}
		else
		{
			iStep--;
		}

		uiMaskSize = (IMG_UINT32)iStep + MIN((IMG_UINT32)iStep, psDstTwiddled->uiTwiddlingLength);
		ui32Mask = (1U << uiMaskSize)-1U;
		bOutside = _bl_packed_twiddled_is_zregion_outside_clip(ui32CurZ,
		                                                       ui32Mask,
		                                                       psDstTwiddled->uTwTypeData.packed.ui32TwiddledMaskX,
		                                                       psDstTwiddled->uTwTypeData.packed.ui32TwiddledMaskY,
		                                                       psDstTwiddled->ui32zStart,
		                                                       psDstTwiddled->ui32zEnd);
	}
	return ui32CurZ;
}

/*
 * Iterates over a subrectangle that is split among several fragments of the z-curve.
 */
static IMG_VOID _bl_packed_twiddled_zorder_disjoint_next(BL_COORDS *psCoords,
                                                         IMG_UINT32 *pui32twiddled,
                                                         BL_DST_TWIDDLED *psDstTwiddled)
{
	PVR_ASSERT(!psDstTwiddled->bHybridTwiddling);

	(*pui32twiddled)++;

	if (!_bl_packed_twiddled_is_inside_clipbox(*pui32twiddled,
                                               psDstTwiddled->uTwTypeData.packed.ui32TwiddledMaskX,
                                               psDstTwiddled->uTwTypeData.packed.ui32TwiddledMaskY,
                                               psDstTwiddled->ui32zStart,
                                               psDstTwiddled->ui32zEnd))
	{
		*pui32twiddled = _bl_packed_twiddled_skip_zregion(*pui32twiddled,
		                                                  psDstTwiddled);
	}
	_bl_untwiddle(psCoords, *pui32twiddled, psDstTwiddled);
}

/*
 * Checks if the clipping area is contained in a contiguous z-region.
 */
static IMG_BOOL _bl_dst_twiddled_is_clip_in_same_zregion(BL_DST_TWIDDLED *psSelf)
{
	IMG_UINT32 ui32width, ui32height;

	ui32width  = (IMG_UINT32)(psSelf->sClipRect.x1 - psSelf->sClipRect.x0);
	ui32height = (IMG_UINT32)(psSelf->sClipRect.y1 - psSelf->sClipRect.y0);

	/* the length of the curve is equal to the number of pixels */
	return (psSelf->ui32zEnd + 1 - psSelf->ui32zStart == ui32width * ui32height) ? IMG_TRUE : IMG_FALSE;
}

/*
 *
 */
static IMG_VOID _bl_hybrid_twiddled_go_to_next_box(BL_COORDS *psCoords,
                                                       IMG_UINT32 *pui32Twiddled,
                                                       BL_DST_TWIDDLED *psDstTwiddled)
{
	if (psCoords->i32X >= psDstTwiddled->sClipRect.x1)
	{
		psCoords->i32Y = (IMG_INT32)((*pui32Twiddled / psDstTwiddled->ui32LinearPartMultY + HYBRID_TWIDDLED_BOX_SIZE) &
				~psDstTwiddled->ui32TwiddlingMask);
	}
	psCoords->i32X = (IMG_INT32)((IMG_UINT32)psDstTwiddled->sClipRect.x0 & ~((1U << HYBRID_TWIDDLED_LEN)-1U));

	*pui32Twiddled = (IMG_UINT32)psCoords->i32Y * psDstTwiddled->ui32LinearPartMultY +
			((IMG_UINT32)psCoords->i32X << HYBRID_TWIDDLED_LEN);
}

/*
 * Iterates over a subrectangle that is split among several fragments of the z-curve.
 *
 * It's mostly an inlined version of the packed disjoint next with some values
 * that are known to be constant replacing variables.
 */
static IMG_VOID _bl_hybrid_twiddled_zorder_next(BL_COORDS *psCoords,
                                                IMG_UINT32 *pui32Twiddled,
                                                BL_DST_TWIDDLED *psDstTwiddled)
{
	IMG_BOOL bOutside = IMG_TRUE;

	PVR_ASSERT(psDstTwiddled->bHybridTwiddling);

	if (psDstTwiddled->uTwTypeData.hybrid.ui32CurrentTileConsecutive > 0)
	{	/*we are in the middle of a consecutive pixels run */
		psDstTwiddled->uTwTypeData.hybrid.ui32CurrentTileConsecutive--;
		bOutside = IMG_FALSE;
	}
	else if ((*pui32Twiddled & HYBRID_TWIDDLED_MASK) == 0)
	{	/* beginning of a twiddled box */
		if (psCoords->i32X >= psDstTwiddled->sClipRect.x0 &&
			psCoords->i32X + (IMG_INT32)HYBRID_TWIDDLED_BOX_SIZE <= psDstTwiddled->sClipRect.x1 &&
			psCoords->i32Y >= psDstTwiddled->sClipRect.y0 &&
			psCoords->i32Y + (IMG_INT32)HYBRID_TWIDDLED_BOX_SIZE <= psDstTwiddled->sClipRect.y1)
		{	/* whole box inside */
			psDstTwiddled->uTwTypeData.hybrid.ui32CurrentTileConsecutive =
					(HYBRID_TWIDDLED_BOX_SIZE * HYBRID_TWIDDLED_BOX_SIZE) - 2;
			bOutside = IMG_FALSE;
		}
	}

	(*pui32Twiddled)++;
	_bl_untwiddle(psCoords, *pui32Twiddled, psDstTwiddled);

	while (bOutside)
	{	/* we are in a box containing some pixels */

		if ((*pui32Twiddled & HYBRID_TWIDDLED_MASK) == 0)
		{	/* beginning of a twiddled box */

			if (psCoords->i32X >= psDstTwiddled->sClipRect.x1 ||
				psCoords->i32X + (IMG_INT32)HYBRID_TWIDDLED_BOX_SIZE <= psDstTwiddled->sClipRect.x0 ||
				psCoords->i32Y >= psDstTwiddled->sClipRect.y1 ||
				psCoords->i32Y + (IMG_INT32)HYBRID_TWIDDLED_BOX_SIZE <= psDstTwiddled->sClipRect.y0)
			{	/* whole box outside */
				_bl_hybrid_twiddled_go_to_next_box(psCoords, pui32Twiddled, psDstTwiddled);
			}

			if (psCoords->i32X >= psDstTwiddled->sClipRect.x0 &&
				psCoords->i32X + (IMG_INT32)HYBRID_TWIDDLED_BOX_SIZE <= psDstTwiddled->sClipRect.x1 &&
				psCoords->i32Y >= psDstTwiddled->sClipRect.y0 &&
				psCoords->i32Y + (IMG_INT32)HYBRID_TWIDDLED_BOX_SIZE <= psDstTwiddled->sClipRect.y1)
			{	/* whole box inside */
				psDstTwiddled->uTwTypeData.hybrid.ui32CurrentTileConsecutive =
						(HYBRID_TWIDDLED_BOX_SIZE * HYBRID_TWIDDLED_BOX_SIZE) - 1;
				bOutside = IMG_FALSE;
			}
		}

		if (bOutside)
		{
			if (psCoords->i32X >= psDstTwiddled->sClipRect.x0 &&
				psCoords->i32X < psDstTwiddled->sClipRect.x1 &&
				psCoords->i32Y >= psDstTwiddled->sClipRect.y0 &&
				psCoords->i32Y < psDstTwiddled->sClipRect.y1)
			{	/* pixel inside */
				bOutside = IMG_FALSE;
			}
			else
			{	/* pixel outside*/
				IMG_BOOL bEndBox = IMG_FALSE;
				IMG_INT iStep = 0;
				IMG_UINT32 ui32Mask = 0;
				IMG_UINT32 ui32CurZ = *pui32Twiddled;
				IMG_UINT32 ui32BoxStart = *pui32Twiddled & ~HYBRID_TWIDDLED_MASK;


				/* if one of the borders is completely inside, fix it to the
				 * max/min value in each axis to solve the wrap problem */
				IMG_UINT32 ui32ClipWrapStart = psDstTwiddled->ui32zStart;
				IMG_UINT32 ui32ClipWrapEnd = psDstTwiddled->ui32zEnd;

				ui32ClipWrapStart &= ~((psCoords->i32X >> HYBRID_TWIDDLED_LEN) >
					(psDstTwiddled->sClipRect.x0 >> HYBRID_TWIDDLED_LEN) ?
					TWIDDLED_X_MASK & HYBRID_TWIDDLED_MASK : 0);
				ui32ClipWrapStart &= ~((psCoords->i32Y >> HYBRID_TWIDDLED_LEN) >
					(psDstTwiddled->sClipRect.y0 >> HYBRID_TWIDDLED_LEN) ?
					TWIDDLED_Y_MASK & HYBRID_TWIDDLED_MASK : 0);
				ui32ClipWrapEnd |= (psCoords->i32X >> HYBRID_TWIDDLED_LEN) <
					(psDstTwiddled->sClipRect.x1 >> HYBRID_TWIDDLED_LEN) ?
					TWIDDLED_X_MASK & HYBRID_TWIDDLED_MASK : 0;
				ui32ClipWrapEnd |= (psCoords->i32Y >> HYBRID_TWIDDLED_LEN) <
					(psDstTwiddled->sClipRect.y1 >> HYBRID_TWIDDLED_LEN) ?
					TWIDDLED_Y_MASK & HYBRID_TWIDDLED_MASK : 0;

				/* skip region */
				while ((iStep > 0 || bOutside) && !bEndBox)
				{
					if (bOutside)
					{
						ui32CurZ |= ui32Mask;
						ui32CurZ++;
						iStep++;
					}
					else
					{
						iStep--;
					}
					ui32Mask = (1U << (iStep*2))-1U;
					/* it's safe to use the packed version, as it will only look inside the twiddled box */
					bOutside = _bl_packed_twiddled_is_zregion_outside_clip(ui32CurZ,
																		   ui32Mask,
																		   TWIDDLED_X_MASK & HYBRID_TWIDDLED_MASK,
																		   TWIDDLED_Y_MASK & HYBRID_TWIDDLED_MASK,
																		   ui32ClipWrapStart,
																		   ui32ClipWrapEnd);

					bEndBox = ((iStep > HYBRID_TWIDDLED_LEN) || (ui32CurZ & ~HYBRID_TWIDDLED_MASK) != ui32BoxStart) ? IMG_TRUE : IMG_FALSE;
				}
				if (bEndBox)
				{
					bOutside = IMG_TRUE;
					*pui32Twiddled = (*pui32Twiddled | HYBRID_TWIDDLED_MASK) + 1;
				}
				else
				{
					*pui32Twiddled = ui32CurZ;
				}
				_bl_untwiddle(psCoords, *pui32Twiddled, psDstTwiddled);
			}
		}
	}

}
#endif

/**
 * Function Name	: _bl_dst_twiddled_do
 * Inputs			: psObject, psCoords (ignored), psPixel (ignored)
 * Outputs			: Nothing
 * Returns			: Nothing
 * Description		: Writes the pixels in the mapping rect of the destination
 *					  surface, after having read them from the upstream object.
 */
/* PRQA S 3195 3 */ /* ignore passed psPixel param */
static PVRSRV_ERROR _bl_dst_twiddled_do(BL_OBJECT	*psObject,
										BL_COORDS	*psCoords,
										BL_PIXEL	*psPixel)
{
	PVRSRV_ERROR eError;
	BL_DST_TWIDDLED *psSelf = (BL_DST_TWIDDLED*) psObject;
	IMG_UINT uiBytesPP = gas_BLExternalPixelTable[psObject->eExternalPxFmt].ui32BytesPerPixel;
	BL_COORDS sCoords;
	BL_PIXEL sPixel;

	BL_INTERNAL_PX_FMT eInternalFmt = gas_BLExternalPixelTable[psObject->eExternalPxFmt].eInternalFormat;
	BL_INTERNAL_PX_FMT ePipeFormat;
	BL_INTERNAL_FORMAT_RW_FUNC pfnPxWrite;

	IMG_CONST IMG_BOOL bIsPlanar = psSelf->sPlanarSurfaceInfo.bIsPlanar;

	IMG_UINT32 ui32Twiddled;
	IMG_VOID (*pfnNextPixelCoords)(BL_COORDS*, IMG_UINT32*, BL_DST_TWIDDLED*);
	IMG_UINT32 ui32nPixels;
	BL_PTR puFBAddr;

	PVR_UNREFERENCED_PARAMETER(psCoords);
	PVR_UNREFERENCED_PARAMETER(psPixel);

	eError = _bl_get_formats_and_writer(psObject,
	                                    uiBytesPP,
	                                    eInternalFmt,
	                                    &ePipeFormat,
	                                    &pfnPxWrite);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	/* pre-calculate twiddling related values */
	if (psSelf->bHybridTwiddling)
	{
		psSelf->uiTwiddlingLength = HYBRID_TWIDDLED_LEN;
		psSelf->ui32TwiddlingMask = (1U << psSelf->uiTwiddlingLength) - 1U;
		psSelf->ui32LinearPartMultX = 1U << psSelf->uiTwiddlingLength;
		psSelf->ui32LinearPartMultY = (psSelf->ui32Width + psSelf->ui32TwiddlingMask)
				& ~psSelf->ui32TwiddlingMask; /* ceiling(width/block_width)*block_width */
	}
	else
	{
		if (psSelf->ui32Width < psSelf->ui32Height)
		{
			psSelf->uiTwiddlingLength = BLFindNearestLog2(psSelf->ui32Width);
			psSelf->ui32TwiddlingMask = (1U << psSelf->uiTwiddlingLength) - 1U;
			psSelf->ui32LinearPartMultX = 0;
			psSelf->ui32LinearPartMultY = 1U << psSelf->uiTwiddlingLength;
			psSelf->uTwTypeData.packed.ui32TwiddledMaskX = TWIDDLED_X_MASK & ((1U << (2*psSelf->uiTwiddlingLength)) - 1);
			psSelf->uTwTypeData.packed.ui32TwiddledMaskY = ~psSelf->uTwTypeData.packed.ui32TwiddledMaskX;
		}
		else
		{
			psSelf->uiTwiddlingLength = BLFindNearestLog2(psSelf->ui32Height);
			psSelf->ui32TwiddlingMask = (1U << psSelf->uiTwiddlingLength) - 1U;
			psSelf->ui32LinearPartMultX = 1U << psSelf->uiTwiddlingLength;
			psSelf->ui32LinearPartMultY = 0;
			psSelf->uTwTypeData.packed.ui32TwiddledMaskY = TWIDDLED_Y_MASK & ((1U << (2*psSelf->uiTwiddlingLength)) - 1U);
			psSelf->uTwTypeData.packed.ui32TwiddledMaskX = ~psSelf->uTwTypeData.packed.ui32TwiddledMaskY;
		}
	}
	psSelf->ui32zStart = _bl_twiddle((IMG_UINT32)psSelf->sClipRect.x0,
									 (IMG_UINT32)psSelf->sClipRect.y0,
									 psSelf);
	psSelf->ui32zEnd = _bl_twiddle((IMG_UINT32)psSelf->sClipRect.x1 - 1,
								   (IMG_UINT32) psSelf->sClipRect.y1 - 1,
								   psSelf);


	/*select the best iterating order*/
	{
		IMG_UINT uiLinearCount = 0;
		IMG_UINT uiTwiddledCount = 0;

		/* temporary objects to get a pointer to the static getPixel
		 * function from the initialised objects */
		BL_SRC_LINEAR sSrcLinearTmp;
		BL_SRC_TWIDDLED sSrcTwiddledTmp;

		BLSRCLinearInit(&sSrcLinearTmp, 0, 0, PVRSRV_PIXEL_FORMAT_UNKNOWN, 0, 0);
		BLSRCTwiddledInit(&sSrcTwiddledTmp, 0, 0, PVRSRV_PIXEL_FORMAT_UNKNOWN, 0, 0, IMG_FALSE);

		/* call cache functions of all objects going through the pipe*/
		eError = _bl_reset_caches_and_count_layouts(psObject,
													& psSelf->sClipRect,
													ePipeFormat,
													& uiLinearCount,
													& uiTwiddledCount,
													sSrcLinearTmp.sObject.pfnGetPixel,
													sSrcTwiddledTmp.sObject.pfnGetPixel);
		if (eError != PVRSRV_OK)
		{
			return eError;
		}

#ifdef BL_DST_ITERATE_PIXELS_IN_ORDER
		/* XXX
		 * those if-else's are to select the best way to iterate over the textures:
		 * we might need to profile if the processor overhead of the calculations
		 * for iterating over the Z curve in-order skipping gaps are worth the
		 * cache misses we can save.
		 */
		if (uiLinearCount > uiTwiddledCount)
		{	/* we won't gain anything iterating in twiddled order if other surfaces are linear */
			pfnNextPixelCoords = _bl_dst_twiddled_row_major_next;
		}
		else if(_bl_dst_twiddled_is_clip_in_same_zregion(psSelf))
		{	/* it's safe to iterate in sequential z-order */
			pfnNextPixelCoords = _bl_dst_twiddled_zorder_contiguous_next;
		}
		else if (!psSelf->bHybridTwiddling)
		{	/* this works for non hybrid twiddled only */
			pfnNextPixelCoords = _bl_packed_twiddled_zorder_disjoint_next;
		}
		else
		{	/* hybrid twiddled texture */
			pfnNextPixelCoords = _bl_hybrid_twiddled_zorder_next;
		}
#else
		pfnNextPixelCoords = _bl_dst_twiddled_row_major_next;
#endif
	}

	/* set the first coordinate */
	sCoords.i32X = psSelf->sClipRect.x0;
	sCoords.i32Y = psSelf->sClipRect.y0;
	ui32Twiddled = _bl_twiddle((IMG_UINT32)sCoords.i32X, (IMG_UINT32)sCoords.i32Y, psSelf);

	ui32nPixels = (IMG_UINT32)((psSelf->sClipRect.x1 - psSelf->sClipRect.x0) * (psSelf->sClipRect.y1 - psSelf->sClipRect.y0));

	/* loop over the pixels */
	if (ui32nPixels > 0)
	{
		for (;;)
		{
			BL_OBJECT_PIPECALL(psSelf, 0, &sCoords, &sPixel);
			if (bIsPlanar)
			{
				BL_PTR puTmp;
				IMG_BYTE abyPackedBuffer[BL_PIXEL_MAX_SIZE];
				puFBAddr.by = psSelf->pbyFBAddr + ui32Twiddled * sizeof(IMG_UINT32);
				puTmp.by = abyPackedBuffer;
				pfnPxWrite(&sPixel, puTmp);
				_bl_unpack_planar_data(puFBAddr,
									   puTmp,
									   &psSelf->sPlanarSurfaceInfo);
			}
			else
			{
				puFBAddr.by = psSelf->pbyFBAddr + ui32Twiddled * uiBytesPP;
				pfnPxWrite(&sPixel, puFBAddr);
			}
			/* don't try to get the next pixel coordinates if we're done */
			if (--ui32nPixels > 0)
			{
				pfnNextPixelCoords(&sCoords, &ui32Twiddled, psSelf);
			}
			else
			{
				break;
			}
		}
	}

	return PVRSRV_OK;
}


/**
 * Function Name	: BLDSTTwiddledInit
 * Inputs			: psSelf, ui32Height, ui32Width, ePixelFormat, psClipRect,
 *					  pbyFBAddr, psUpstreamObject
 * Outputs			: psSelf
 * Returns			: Nothing
 * Description		: Initialises the Twiddled Destination surface.
 */
IMG_INTERNAL IMG_VOID BLDSTTwiddledInit(BL_DST_TWIDDLED			*psSelf,
										IMG_UINT32				ui32Width,
										IMG_UINT32				ui32Height,
										PVRSRV_PIXEL_FORMAT		ePixelFormat,
										IMG_RECT				*psClipRect,
										IMG_PBYTE				pbyFBAddr,
										BL_OBJECT				*psUpstreamObject,
										BL_PLANAR_SURFACE_INFO	*psPlanarInfo,
										IMG_BOOL				bHybridTwiddling)
{
	BLInitObject(&psSelf->sObject);
	BL_OBJECT_ASSOC_FUNC(psSelf) = &_bl_dst_twiddled_do;

	psSelf->ui32Height = ui32Height;
	psSelf->ui32Width = ui32Width;
	psSelf->sClipRect = *psClipRect;
	psSelf->pbyFBAddr = pbyFBAddr;
	psSelf->sObject.apsUpstreamObjects[0] = psUpstreamObject;
	psSelf->sObject.eExternalPxFmt = ePixelFormat;
	if (psPlanarInfo)
		psSelf->sPlanarSurfaceInfo = *psPlanarInfo;
	else
		psSelf->sPlanarSurfaceInfo.bIsPlanar = IMG_FALSE;
	psSelf->bHybridTwiddling = bHybridTwiddling;
}


/**
 * Function Name	: _bl_dst_tiled_do
 * Inputs			: psObject, psCoords (ignored), psPixel (ignored)
 * Outputs			: Nothing
 * Returns			: PVRSRV_ERROR
 * Description		: Writes the pixels in the mapping rect of the destination
 *					  surface, after having read them from the upstream object.
 */
static PVRSRV_ERROR _bl_dst_tiled_do(BL_OBJECT * psObject,
                                     BL_COORDS * psCoords,
                                     BL_PIXEL * psPixel)
{
	PVRSRV_ERROR eError;
	BL_DST_TILED *psSelf = (BL_DST_TILED*) psObject;
	IMG_UINT uiBytesPP = gas_BLExternalPixelTable[psObject->eExternalPxFmt].ui32BytesPerPixel;
	BL_COORDS sCoords;
	BL_PIXEL sPixel;
	BL_INTERNAL_PX_FMT eInternalFmt = gas_BLExternalPixelTable[psObject->eExternalPxFmt].eInternalFormat;
	BL_INTERNAL_PX_FMT ePipeFormat;
	BL_INTERNAL_FORMAT_RW_FUNC pfnPxWrite;

	PVR_UNREFERENCED_PARAMETER(psCoords);
	PVR_UNREFERENCED_PARAMETER(psPixel);

	eError = _bl_get_formats_and_writer(psObject, uiBytesPP, eInternalFmt, &ePipeFormat, &pfnPxWrite);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	/* call cache functions of all objects going through the pipe*/
	eError = _bl_reset_caches_and_count_layouts(psObject,
	                                            & psSelf->sClipRect,
	                                            ePipeFormat,
	                                            IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	for (sCoords.i32Y = psSelf->sClipRect.y0; sCoords.i32Y < psSelf->sClipRect.y1; sCoords.i32Y++)
	{
		for (sCoords.i32X  = psSelf->sClipRect.x0; sCoords.i32X < psSelf->sClipRect.x1; sCoords.i32X++)
		{
			BL_PTR puFBAddr;
			IMG_UINT32 ui32Offset;
			IMG_UINT32 ui32inTile;
			IMG_UINT32 ui32tileY;

			/* this is the vhdl formula, note that "&" is bit concatenation:
			 Address = (u(12:5]+(stride*v(12:5])  & v[4:0] & u[4:0]
			 */
			ui32inTile = ((((IMG_UINT32)sCoords.i32Y & 0x1FU) << 5) | ((IMG_UINT32)sCoords.i32X & 0x1FU));
			ui32tileY = (((IMG_UINT32)sCoords.i32Y & ~0x1FU)) * (IMG_UINT32)psSelf->i32ByteStride;

			BL_OBJECT_PIPECALL(psSelf, 0, &sCoords, &sPixel);

			if (psSelf->sPlanarSurfaceInfo.bIsPlanar)
			{
				BL_PTR puTmp;
				IMG_BYTE abyPackedBuffer[BL_PIXEL_MAX_SIZE];

				ui32Offset = ui32tileY + (((IMG_UINT32)sCoords.i32X & ~0x1FU) << 5) * sizeof(IMG_UINT32);
				ui32Offset |= ui32inTile * sizeof(IMG_UINT32);
				puFBAddr.by = psSelf->pbyFBAddr + ui32Offset;
				puTmp.by = abyPackedBuffer;
				pfnPxWrite(&sPixel, puTmp);
				_bl_unpack_planar_data(puFBAddr,
				                       puTmp,
				                       &psSelf->sPlanarSurfaceInfo);
			}
			else
			{
				ui32Offset = ui32tileY + (((IMG_UINT32)sCoords.i32X & ~0x1FU) << 5) * uiBytesPP;
				ui32Offset |= ui32inTile * uiBytesPP;

				puFBAddr.by = psSelf->pbyFBAddr + ui32Offset;
				pfnPxWrite(&sPixel, puFBAddr);
			}
		}
	}
	return PVRSRV_OK;
}

/**
 * Function Name	: BLDSTTiledInit
 * Inputs			: psSelf, ui32Height, i32ByteStride, ePixelFormat,
 *					  psClipRect, pbyFBAddr, psUpstreamObject
 * Outputs			: psSelf
 * Returns			: Nothing
 * Description		: Initialises the Linear (Strided) Destination surface.
 */
IMG_INTERNAL IMG_VOID BLDSTTiledInit(BL_DST_TILED			*psSelf,
                                     IMG_INT32				i32ByteStride,
                                     IMG_UINT32				ui32Height,
                                     PVRSRV_PIXEL_FORMAT	ePixelFormat,
                                     IMG_RECT				*psClipRect,
                                     IMG_PBYTE				pbyFBAddr,
                                     BL_OBJECT				*psUpstreamObject,
                                     BL_PLANAR_SURFACE_INFO	*psPlanarInfo)
{
	BLInitObject(&psSelf->sObject);
	BL_OBJECT_ASSOC_FUNC(psSelf) = &_bl_dst_tiled_do;

	psSelf->ui32Height = ui32Height;
	psSelf->i32ByteStride = i32ByteStride;
	psSelf->sClipRect = *psClipRect;
	psSelf->pbyFBAddr = pbyFBAddr;
	psSelf->sObject.apsUpstreamObjects[0] = psUpstreamObject;
	psSelf->sObject.eExternalPxFmt = ePixelFormat;
	if (psPlanarInfo)
		psSelf->sPlanarSurfaceInfo = *psPlanarInfo;
	else
		psSelf->sPlanarSurfaceInfo.bIsPlanar = IMG_FALSE;
}
/******************************************************************************
 End of file (blitlib_dst.c)
 ******************************************************************************/
