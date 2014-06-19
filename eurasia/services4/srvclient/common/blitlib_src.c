/*!****************************************************************************
@File           blitlib_src.c

@Title         	Blit library - texture fetch objects 

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
$Log: blitlib_src.c $
 *******************************************************************************
Renamed from blitlib_tf.c
 --- Revision Logs Removed --- 
 ******************************************************************************/

#include "blitlib_src.h"
#include "blitlib_common.h"
#include "pvr_debug.h"

/**
 * Function Name	: _bl_src_get_read_and_convert
 * Inputs			: ePipePxFmt, eExternalPxFmt
 * Outputs			: ppfnConvert, ppfnPxRead
 * Returns			: PVRSRV_ERROR
 * Description		: Gets the appropriate pixel read and conversion functions.
 */
static PVRSRV_ERROR _bl_get_read_and_convert_funcs(BL_INTERNAL_PX_FMT				ePipePxFmt,
												   PVRSRV_PIXEL_FORMAT				eExternalPxFmt,
												   BL_INTERNAL_FORMAT_CONVERT_FUNC	*ppfnConvert,
												   BL_INTERNAL_FORMAT_RW_FUNC		*ppfnPxRead)
{
	BL_INTERNAL_PX_FMT ePreferredInternalFormat;

	PVR_ASSERT(ePipePxFmt < BL_INTERNAL_PX_FMT_UNAVAILABLE);

	*ppfnConvert = IMG_NULL;
	if (ePipePxFmt >= BL_INTERNAL_PX_FMT_RAW8)
	{
		/* PRQA S 3689 1 */ /* bounds not exceeded */
		*ppfnPxRead = gas_BLInternalPixelTable[ePipePxFmt].pfnPxReadRAW;
	}
	else
	{
		*ppfnPxRead = gas_BLExternalPixelTable[eExternalPxFmt].pfnPxRead;
		ePreferredInternalFormat = gas_BLExternalPixelTable[eExternalPxFmt].eInternalFormat;
		if (ePreferredInternalFormat != ePipePxFmt)
		{
			PVR_ASSERT(ePreferredInternalFormat < BL_INTERNAL_PX_FMT_UNAVAILABLE);
			
			/* PRQA S 3689 1 */ /* bounds not exceeded */
			*ppfnConvert = gafn_BLInternalPixelFormatConversionTable[ePreferredInternalFormat][ePipePxFmt];
			if (*ppfnConvert == IMG_NULL)
			{
				return PVRSRV_ERROR_BLIT_SETUP_FAILED;
			}
		}
	}

	if (*ppfnPxRead == IMG_NULL)
	{
		return PVRSRV_ERROR_BLIT_SETUP_FAILED;
	}
	return PVRSRV_OK;
}

/**
 * Function Name	: _bl_pack_planar_data
 * Inputs			: puFBBaseAddr, psPlanarSurfaceInfo
 * Outputs			: puPackedBufer
 * Returns			: Nothing
 * Description		: Packs a planar pixel into a packed buffer.
 */
static IMG_VOID _bl_pack_planar_data(BL_PTR					puPackedBuffer,
                                     BL_PTR					puPlanarBuffer,
                                     BL_PLANAR_SURFACE_INFO	*psPlanarSurfaceInfo)
{
	IMG_UINT	i;
	for (i = 0; i < psPlanarSurfaceInfo->uiNChunks; i++)
	{
#if 1 /*this is the same plane ordering*/
		PVRSRVMemCopy(puPackedBuffer.by + i*psPlanarSurfaceInfo->uiPixelBytesPerPlane,
		              puPlanarBuffer.by + i*(IMG_UINT32)psPlanarSurfaceInfo->i32ChunkStride,
		              psPlanarSurfaceInfo->uiPixelBytesPerPlane);
#else
		PVRSRVMemCopy(puPackedBuffer.by + i*psPlanarSurfaceInfo->uiPixelBytesPerPlane,
		              puPlanarBuffer.by + (psPlanarSurfaceInfo->uiNChunks-i-1)*psPlanarSurfaceInfo->i32ChunkStride,
		              psPlanarSurfaceInfo->uiPixelBytesPerPlane);
#endif
	}
}

/*
 * BL_SRC_LINEAR
 */

/**
 * Function Name	: _bl_src_linear_get_pixel
 * Inputs			: psObject, psCoords
 * Outputs			: psPixel
 * Returns			: Nothing
 * Description		: Gets a pixel from a linear source surface and transforms
 *					  it to the pipe internal pixel format.
 */
static PVRSRV_ERROR _bl_src_linear_get_pixel(BL_OBJECT * psObject,
											 BL_COORDS * psCoords,
											 BL_PIXEL * psPixel)
{
	BL_SRC_LINEAR * psSelf = (BL_SRC_LINEAR*) psObject;
	BL_PTR puFBAddr;
	IMG_UINT32 ui32BytesPP = gas_BLExternalPixelTable[psObject->eExternalPxFmt].ui32BytesPerPixel;

	if ((psCoords->i32X < 0) ||
		(psCoords->i32X >= psSelf->i32ByteStride/(IMG_INT32)ui32BytesPP) ||
		(psCoords->i32Y < 0) ||
		(psCoords->i32Y >= (IMG_INT32) psSelf->ui32Height))
	{
		return PVRSRV_ERROR_BAD_REGION_SIZE_MISMATCH;
	}


	if (psSelf->sPlanarSurfaceInfo.bIsPlanar)
	{
		BL_PTR puTmp;
		IMG_BYTE abyPackedBuffer[BL_PIXEL_MAX_SIZE];
		puFBAddr.by = psSelf->pbyFBAddr + (IMG_UINT32)(psCoords->i32Y * psSelf->i32ByteStride) + ((IMG_UINT32)psCoords->i32X * sizeof(IMG_UINT32));
		puTmp.by = abyPackedBuffer;
		_bl_pack_planar_data(puTmp,
		                     puFBAddr,
		                     &psSelf->sPlanarSurfaceInfo);
		psSelf->pfnPxRead(psPixel, puTmp);
	}
	else
	{
		puFBAddr.by = psSelf->pbyFBAddr + psCoords->i32Y * psSelf->i32ByteStride + (psCoords->i32X * (IMG_INT32)ui32BytesPP);
		psSelf->pfnPxRead(psPixel, puFBAddr);
	}

	if (psSelf->pfnConvert != IMG_NULL)
	{
		psSelf->pfnConvert(psPixel, psPixel);
	}
	return PVRSRV_OK;
}

/**
 * Function Name	: _bl_src_linear_caching
 * Inputs			: psObject, psRect, ePipePxFmt
 * Outputs			: Nothing
 * Returns			: Nothing
 * Description		: Precalculates the values needed to do the coordinates
 *					  to memory address mapping and prepares the pixel
 *					  format transform function.
 */
static PVRSRV_ERROR _bl_src_linear_caching(BL_OBJECT			*psObject,
										   IMG_RECT				*psDownstreamRect,
										   BL_INTERNAL_PX_FMT	ePipePxFmt)
{
	BL_SRC_LINEAR *psSelf = (BL_SRC_LINEAR *) psObject;

	PVR_UNREFERENCED_PARAMETER(psDownstreamRect);

	return _bl_get_read_and_convert_funcs(ePipePxFmt,
										  psObject->eExternalPxFmt,
										  &psSelf->pfnConvert,
										  &psSelf->pfnPxRead);
}

/**
 * Function Name	: BLSRCLinearInit
 * Inputs			: psSelf, ui32Width, i32ByteStride, ePixelFormat, pbyFBAddr
 * Outputs			: psSelf
 * Returns			: Nothing
 * Description		: Initialises a Linear (Strided) Source Surface
 */
IMG_INTERNAL IMG_VOID BLSRCLinearInit(BL_SRC_LINEAR				*psSelf,
									  IMG_INT32					i32ByteStride,
									  IMG_UINT32				ui32Height,
									  PVRSRV_PIXEL_FORMAT		ePixelFormat,
									  IMG_PBYTE					pbyFBAddr,
									  BL_PLANAR_SURFACE_INFO	*psPlanarInfo)
{
	BLInitObject(&psSelf->sObject);
	psSelf->sObject.pfnDoCaching = &_bl_src_linear_caching;
	psSelf->sObject.pfnGetPixel = &_bl_src_linear_get_pixel;

	psSelf->ui32Height = ui32Height;
	psSelf->i32ByteStride = i32ByteStride;
	psSelf->pbyFBAddr = pbyFBAddr;
	psSelf->sObject.eExternalPxFmt = ePixelFormat;
	if (psPlanarInfo)
		psSelf->sPlanarSurfaceInfo = *psPlanarInfo;
	else
		psSelf->sPlanarSurfaceInfo.bIsPlanar = IMG_FALSE;
}


/*
 * BL_SRC_TILED
 */

/**
 * Function Name	: _bl_src_tiled_get_pixel
 * Inputs			: psObject, psCoords
 * Outputs			: psPixel
 * Returns			: Nothing
 * Description		: Gets a pixel from a linear source surface and transforms
 *					  it to the pipe internal pixel format.
 */
static PVRSRV_ERROR _bl_src_tiled_get_pixel(BL_OBJECT * psObject,
                                            BL_COORDS * psCoords,
                                            BL_PIXEL * psPixel)
{
	BL_SRC_TILED * psSelf = (BL_SRC_TILED*) psObject;
	BL_PTR puFBAddr;
	IMG_UINT uiBytesPP = gas_BLExternalPixelTable[psObject->eExternalPxFmt].ui32BytesPerPixel;

	IMG_UINT32 ui32Offset;
	IMG_UINT32 ui32inTile;
	IMG_UINT32 ui32tileY;

	if ((psCoords->i32X < 0) ||
		(psCoords->i32X >= psSelf->i32ByteStride/(IMG_INT32)uiBytesPP) ||
		(psCoords->i32Y < 0) ||
		(psCoords->i32Y >= (IMG_INT32) psSelf->ui32Height))
	{
		return PVRSRV_ERROR_BAD_REGION_SIZE_MISMATCH;
	}

	/* this is the vhdl formula, note that "&" is bit concatenation:
	 Address = (u(12:5]+(stride*v(12:5])  & v[4:0] & u[4:0]
	 */
	ui32inTile = ((((IMG_UINT32)psCoords->i32Y & 0x1FU) << 5) | ((IMG_UINT32)psCoords->i32X & 0x1FU));
	ui32tileY = (((IMG_UINT32)psCoords->i32Y & ~0x1FU)) * (IMG_UINT32)psSelf->i32ByteStride;

	if (psSelf->sPlanarSurfaceInfo.bIsPlanar)
	{
		BL_PTR puTmp;
		IMG_BYTE abyPackedBuffer[BL_PIXEL_MAX_SIZE];

		ui32Offset = ui32tileY + (((IMG_UINT32)psCoords->i32X & ~0x1FU) << 5) * sizeof(IMG_UINT32);
		ui32Offset |= ui32inTile * sizeof(IMG_UINT32);

		puFBAddr.by = psSelf->pbyFBAddr + ui32Offset;
		puTmp.by = abyPackedBuffer;
		_bl_pack_planar_data(puTmp,
		                     puFBAddr,
		                     &psSelf->sPlanarSurfaceInfo);
		psSelf->pfnPxRead(psPixel, puTmp);
	}
	else
	{
		ui32Offset = ui32tileY + (((IMG_UINT32)psCoords->i32X & ~0x1FU) << 5) * uiBytesPP;
		ui32Offset |= ui32inTile * uiBytesPP;

		puFBAddr.by = psSelf->pbyFBAddr + ui32Offset;
		psSelf->pfnPxRead(psPixel, puFBAddr);
	}

	if (psSelf->pfnConvert != IMG_NULL)
	{
		psSelf->pfnConvert(psPixel, psPixel);
	}
	return PVRSRV_OK;
}

/**
 * Function Name	: BLSRCLinearInit
 * Inputs			: psSelf, ui32Width, i32ByteStride, ePixelFormat, pbyFBAddr
 * Outputs			: psSelf
 * Returns			: Nothing
 * Description		: Initialises a Tiled Source Surface
 */
IMG_INTERNAL IMG_VOID BLSRCTiledInit(BL_SRC_TILED			*psSelf,
                                     IMG_INT32				i32ByteStride,
                                     IMG_UINT32				ui32Height,
                                     PVRSRV_PIXEL_FORMAT	ePixelFormat,
                                     IMG_PBYTE				pbyFBAddr,
                                     BL_PLANAR_SURFACE_INFO	*psPlanarInfo)
{
	BLInitObject(&psSelf->sObject);
	/*yes, this is right; it shared the same basic caching as linear src*/
	psSelf->sObject.pfnDoCaching = &_bl_src_linear_caching;
	psSelf->sObject.pfnGetPixel = &_bl_src_tiled_get_pixel;

	psSelf->ui32Height = ui32Height;
	psSelf->i32ByteStride = i32ByteStride;
	psSelf->pbyFBAddr = pbyFBAddr;
	psSelf->sObject.eExternalPxFmt = ePixelFormat;
	if (psPlanarInfo)
		psSelf->sPlanarSurfaceInfo = *psPlanarInfo;
	else
		psSelf->sPlanarSurfaceInfo.bIsPlanar = IMG_FALSE;
}


/*
 * BL_SRC_TWIDDLED
 */

/**
 * Function Name	: _bl_src_twiddled_get_pixel
 * Inputs			: psObject, psCoords
 * Outputs			: psPixel
 * Returns			: Nothing
 * Description		: Gets a pixel from a twiddled source surface and transforms
 *					  it to the pipe internal pixel format.
 */
static PVRSRV_ERROR _bl_src_twiddled_get_pixel(BL_OBJECT	*psObject,
											   BL_COORDS	*psCoords,
											   BL_PIXEL		*psPixel)
{
	BL_SRC_TWIDDLED *psSelf = (BL_SRC_TWIDDLED*) psObject;
	IMG_UINT32 ui32PixelIndex;
	BL_PTR puFBAddr;
	IMG_UINT32 ui32BytesPP = gas_BLExternalPixelTable[psObject->eExternalPxFmt].ui32BytesPerPixel;

	if ((psCoords->i32X < 0) ||
		(psCoords->i32X >= (IMG_INT32) psSelf->ui32Width) ||
		(psCoords->i32Y < 0) ||
		(psCoords->i32Y >= (IMG_INT32) psSelf->ui32Height))
	{
		return PVRSRV_ERROR_BAD_REGION_SIZE_MISMATCH;
	}


	if (psSelf->i32Y != psCoords->i32Y)
	{
		psSelf->i32Y = psCoords->i32Y;
		psSelf->ui32YStretched = BLTwiddleStretch((IMG_UINT32)psSelf->i32Y  & psSelf->ui32TwiddlingMask) |
				(((IMG_UINT32)psSelf->i32Y & ~psSelf->ui32TwiddlingMask) * psSelf->ui32LinearPartMultY);
	}
	if (psSelf->i32X != psCoords->i32X)
	{
		psSelf->i32X = psCoords->i32X;
		psSelf->ui32XStretched = (BLTwiddleStretch((IMG_UINT32)psSelf->i32X & psSelf->ui32TwiddlingMask) << 1) |
				(((IMG_UINT32)psSelf->i32X  & ~psSelf->ui32TwiddlingMask) * psSelf->ui32LinearPartMultX);
	}

	ui32PixelIndex = psSelf->ui32XStretched + psSelf->ui32YStretched;
	if (psSelf->sPlanarSurfaceInfo.bIsPlanar)
	{
		BL_PTR puTmp;
		IMG_BYTE abyPackedBuffer[BL_PIXEL_MAX_SIZE];
		puFBAddr.by = psSelf->pbyFBAddr + ui32PixelIndex * sizeof(IMG_UINT32);
		puTmp.by = abyPackedBuffer;
		_bl_pack_planar_data(puTmp,
		                     puFBAddr,
		                     &psSelf->sPlanarSurfaceInfo);
		psSelf->pfnPxRead(psPixel, puTmp);
	}
	else
	{
		puFBAddr.by = psSelf->pbyFBAddr + ui32PixelIndex * ui32BytesPP;
		psSelf->pfnPxRead(psPixel, puFBAddr);
	}

	if (psSelf->pfnConvert != IMG_NULL)
	{
		psSelf->pfnConvert(psPixel, psPixel);
	}
	return PVRSRV_OK;
}

/**
 * Function Name	: _bl_src_twiddled_caching
 * Inputs			: psObject, psRect, ePipePxFmt
 * Outputs			: Nothing
 * Returns			: Nothing
 * Description		: Precalculates the values needed to do the coordinates
 *					  to memory address mapping and prepares the pixel
 *					  format transform function.
 */
static PVRSRV_ERROR _bl_src_twiddled_caching(BL_OBJECT			*psObject,
											 IMG_RECT			*psDownstreamRect,
											 BL_INTERNAL_PX_FMT	ePipePxFmt)
{
	BL_SRC_TWIDDLED *psSelf = (BL_SRC_TWIDDLED *) psObject;
	IMG_UINT32 ui32TwiddlingLength;

	PVR_UNREFERENCED_PARAMETER(psDownstreamRect);

	if (psSelf->bHybridTwiddling)
	{
		/*4 is log2(16), that is the max tile size*/
		ui32TwiddlingLength = MAX(4, BLFindNearestLog2(MIN(psSelf->ui32Width, psSelf->ui32Height)));
		psSelf->ui32TwiddlingMask = (1U << ui32TwiddlingLength) - 1U;
		//XXX this isn't quite tested yet... but seems more sound than the previous version
		psSelf->ui32LinearPartMultY = (psSelf->ui32Width + psSelf->ui32TwiddlingMask) & ~psSelf->ui32TwiddlingMask;
	}
	else
	{
		ui32TwiddlingLength = BLFindNearestLog2(MIN(psSelf->ui32Width, psSelf->ui32Height));
		psSelf->ui32TwiddlingMask = (1U << ui32TwiddlingLength) - 1U;
		psSelf->ui32LinearPartMultY = 1U << ui32TwiddlingLength;
	}

	psSelf->ui32LinearPartMultX = 1U << ui32TwiddlingLength;
	psSelf->i32Y = -1;
	psSelf->i32X = -1;

	return _bl_get_read_and_convert_funcs(ePipePxFmt,
										  psObject->eExternalPxFmt,
										  &psSelf->pfnConvert,
										  &psSelf->pfnPxRead);
}

/**
 * Function Name	: BLSRCTwiddledInit
 * Inputs			: psSelf, ui32Width, ui32Height, ePixelFormat, pbyFBAddr
 * Outputs			: psSelf
 * Returns			: Nothing
 * Description		: Initialises a Twiddled Source Surface
 */
IMG_INTERNAL IMG_VOID BLSRCTwiddledInit(BL_SRC_TWIDDLED			*psSelf,
										IMG_UINT32				ui32Width,
										IMG_UINT32				ui32Height,
										PVRSRV_PIXEL_FORMAT		ePixelFormat,
										IMG_PBYTE				pbyFBAddr,
										BL_PLANAR_SURFACE_INFO	*psPlanarInfo,
										IMG_BOOL				bHybridTwiddling)
{
	BLInitObject(&psSelf->sObject);
	psSelf->sObject.pfnDoCaching = &_bl_src_twiddled_caching;
	psSelf->sObject.pfnGetPixel = &_bl_src_twiddled_get_pixel;

	psSelf->ui32Height = ui32Height;
	psSelf->ui32Width = ui32Width;
	psSelf->pbyFBAddr = pbyFBAddr;
	psSelf->sObject.eExternalPxFmt = ePixelFormat;
	if (psPlanarInfo)
		psSelf->sPlanarSurfaceInfo = *psPlanarInfo;
	else
		psSelf->sPlanarSurfaceInfo.bIsPlanar = IMG_FALSE;
	psSelf->bHybridTwiddling = bHybridTwiddling;
}

/*
 * BL_SRC_SOLID
 */

/**
 * Function Name	: _bl_src_solid_get_pixel
 * Inputs			: psObject, psCoords
 * Outputs			: psPixel
 * Returns			: Nothing
 * Description		: Copies the source colour pixel to the output pixel.
 */
static PVRSRV_ERROR _bl_src_solid_get_pixel(BL_OBJECT	*psObject,
											BL_COORDS	*psCoords,
											BL_PIXEL	*psPixel)
{
	BL_SRC_SOLID *psSelf = (BL_SRC_SOLID*) psObject;
	PVR_UNREFERENCED_PARAMETER(psCoords);
	
	*psPixel = psSelf->sColour;
	return PVRSRV_OK;
}

/**
 * Function Name	: _bl_op_solid_caching
 * Inputs			: psObject, psRect, ePipePxFmt
 * Outputs			: Nothing
 * Returns			: Nothing
 * Description		: Prepares solid colour internal pixel format to be the
 *					  same as the pipe's.
 */
static PVRSRV_ERROR _bl_src_solid_caching(BL_OBJECT			*psObject,
										  IMG_RECT			*psRect,
										  BL_INTERNAL_PX_FMT	ePipePxFmt)
{
	BL_SRC_SOLID *psSelf = (BL_SRC_SOLID*) psObject;

	PVR_UNREFERENCED_PARAMETER(psRect);

	if (ePipePxFmt < BL_INTERNAL_PX_FMT_RAW8 && psSelf->eColourFormat != ePipePxFmt)
	{
		BL_INTERNAL_FORMAT_CONVERT_FUNC pfnConvert;

		PVR_ASSERT(psSelf->eColourFormat < BL_INTERNAL_PX_FMT_UNAVAILABLE);

		/* PRQA S 3689 1 */ /* bounds not exceeded */
		pfnConvert = gafn_BLInternalPixelFormatConversionTable[psSelf->eColourFormat][ePipePxFmt];
		if (pfnConvert == IMG_NULL)
		{
			return PVRSRV_ERROR_BLIT_SETUP_FAILED;
		}

		pfnConvert(&psSelf->sColour, &psSelf->sColour);

		psSelf->eColourFormat = ePipePxFmt;
	}
	return PVRSRV_OK;
}

/**
 * Function Name	: BLSRCSolidInit
 * Inputs			: psSelf, psColour, eInternalColourFormat, eExternalColourFormat
 * Outputs			: Nothing
 * Returns			: Nothing
 * Description		: Initialises a Solid Colour Source Surface.
 *
 * The psColour is in the internal format specifiedby eInternalColourFormat,
 * if it's a RAW format, then the eExternalColourFormat indicates what type
 * of RAW format it is.
 */
IMG_INTERNAL IMG_VOID BLSRCSolidInit(BL_SRC_SOLID			*psSelf,
									 BL_PIXEL				*psColour,
									 BL_INTERNAL_PX_FMT		eInternalColourFormat,
									 PVRSRV_PIXEL_FORMAT	eExternalColourFormat)
{
	BLInitObject(&psSelf->sObject);
	psSelf->sObject.pfnDoCaching = &_bl_src_solid_caching;
	psSelf->sObject.pfnGetPixel = &_bl_src_solid_get_pixel;

	psSelf->sColour = *psColour;
	psSelf->eColourFormat = eInternalColourFormat;

	switch (eInternalColourFormat)
	{
		case BL_INTERNAL_PX_FMT_ARGB8888:
		{
			psSelf->sObject.eExternalPxFmt = PVRSRV_PIXEL_FORMAT_ARGB8888;
			break;
		}
		case BL_INTERNAL_PX_FMT_A2RGB10:
		{
			psSelf->sObject.eExternalPxFmt = PVRSRV_PIXEL_FORMAT_A2RGB10;
			break;
		}
		case BL_INTERNAL_PX_FMT_ARGB16:
		{
			psSelf->sObject.eExternalPxFmt = PVRSRV_PIXEL_FORMAT_A16B16G16R16;
			break;
		}
		case BL_INTERNAL_PX_FMT_RAW8:
		case BL_INTERNAL_PX_FMT_RAW16:
		case BL_INTERNAL_PX_FMT_RAW24:
		case BL_INTERNAL_PX_FMT_RAW32:
		case BL_INTERNAL_PX_FMT_RAW64:
		case BL_INTERNAL_PX_FMT_RAW128:
		{
			psSelf->sObject.eExternalPxFmt = eExternalColourFormat;
			break;
		}
		default:
		{
			PVR_DBG_BREAK;
		}
	}
}
/******************************************************************************
 End of file (blitlib_src.c)
 ******************************************************************************/
