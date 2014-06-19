/*!****************************************************************************
@File           blitlib_op.c

@Title         	Blit library - filter objects

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
$Log: blitlib_op.c $
 *******************************************************************************
Renamed from blitlib_fi.c
 --- Revision Logs Removed --- 
 ******************************************************************************/

#include "blitlib_op.h"
#include "pvr_debug.h"

/**
 * Function Name	: _bl_op_scale_nearest_get_pixel
 * Inputs			: psObject, psDownstreamCoords
 * Outputs			: psPixel
 * Returns			: Nothing
 * Description		: Gets the upstream pixel nearest to the downstream pixel,
 *					  once the coordinates are remapped.
 */
static PVRSRV_ERROR _bl_op_scale_nearest_get_pixel(BL_OBJECT	*psObject,
												   BL_COORDS	*psDownstreamCoords,
												   BL_PIXEL		*psPixel)
{
	/* PRQA S 3305 1 */ /* ignore strict aligment warning */
	BL_OP_SCALE_NEAREST *psSelf = (BL_OP_SCALE_NEAREST *) psObject;
	BL_COORDS sUpstreamCoords;

	sUpstreamCoords.i32X = psSelf->sUpstreamClipRect.x0 + (IMG_INT32)
		(psSelf->dXRatio * (IMG_DOUBLE)(psDownstreamCoords->i32X - psSelf->sDownstreamOffset.i32X));

	sUpstreamCoords.i32Y = psSelf->sUpstreamClipRect.y0 + (IMG_INT32)
		(psSelf->dYRatio * (IMG_DOUBLE)(psDownstreamCoords->i32Y - psSelf->sDownstreamOffset.i32Y));

	return BL_OBJECT_PIPECALL(psObject, 0, &sUpstreamCoords, psPixel);
}

/**
 * Function Name	: _bl_op_scale_nearest_caching
 * Inputs			: psSelf, psMappingRect, ePipePxFmt
 * Outputs			: psMappingRect
 * Returns			: PVRSRV_ERROR
 * Description		: Calculates the dimension ratios and updates the mapping rect.
 */
static PVRSRV_ERROR _bl_op_scale_nearest_caching(BL_OBJECT			*psObject,
												 IMG_RECT			*psMappingRect,
												 BL_INTERNAL_PX_FMT	ePipePxFmt)
{
	/* PRQA S 3305 1 */ /* ignore strict aligment warning */
	BL_OP_SCALE_NEAREST *psSelf = (BL_OP_SCALE_NEAREST *) psObject;

	PVR_UNREFERENCED_PARAMETER(ePipePxFmt);

	psSelf->sDownstreamOffset.i32X = psMappingRect->x0;
	psSelf->sDownstreamOffset.i32Y = psMappingRect->y0;

	psSelf->dXRatio = (IMG_DOUBLE)(psSelf->sUpstreamClipRect.x1 - psSelf->sUpstreamClipRect.x0) /
		(IMG_DOUBLE) (psMappingRect->x1 - psMappingRect->x0);
	psSelf->dYRatio = (IMG_DOUBLE)(psSelf->sUpstreamClipRect.y1 - psSelf->sUpstreamClipRect.y0) /
		(IMG_DOUBLE) (psMappingRect->y1 - psMappingRect->y0);

	*psMappingRect = psSelf->sUpstreamClipRect;

	return PVRSRV_OK;
}

/**
 * Function Name	: BLOPScaleBilinearInit
 * Inputs			: psSelf, psUpstreamClipRect, psUpstreamObject
 * Outputs			: psSelf
 * Returns			: Nothing
 * Description		: Initialises the Nearest Scaling operation.
 */
IMG_INTERNAL IMG_VOID BLOPScaleNearestInit(BL_OP_SCALE_NEAREST	*psSelf,
										   IMG_RECT				*psUpstreamClipRect,
										   BL_OBJECT			*psUpstreamObject)
{
	BLInitObject(&psSelf->sObject);

	if (psUpstreamClipRect != IMG_NULL)
	{
		psSelf->sUpstreamClipRect = *psUpstreamClipRect;
	}

	psSelf->sObject.pfnGetPixel = &_bl_op_scale_nearest_get_pixel;
	psSelf->sObject.pfnDoCaching = &_bl_op_scale_nearest_caching;

	psSelf->sObject.apsUpstreamObjects[0] = psUpstreamObject;
}

/*
 * UPSCALE BILINIEAR
 */



/**
 * Function Name	: _bl_op_scale_bilinear_get_pixel
 * Inputs			: psObject, psDownstreamCoords
 * Outputs			: psPixel
 * Returns			: Nothing
 * Description		: Does a bilinear interpolation of the adjacent upstream
 *					  pixels
 */
static PVRSRV_ERROR _bl_op_scale_bilinear_get_pixel(BL_OBJECT	*psObject,
													BL_COORDS	*psDownstreamCoords,
													BL_PIXEL	*psPixel)
{
	PVRSRV_ERROR eError;
	/* PRQA S 3305 1 */ /* ignore strict alignment warning */
	BL_OP_SCALE_BILINEAR *psSelf = (BL_OP_SCALE_BILINEAR *) psObject;
	IMG_INT32 iSrcXa, iSrcXb, iSrcYa, iSrcYb;
	BL_COORDS sUpstreamCoords;

	IMG_DOUBLE dSrcX, dSrcY, dDx, dDy;
	IMG_DOUBLE adPixelWeights[4];

	dSrcX = (IMG_DOUBLE)psSelf->sUpstreamClipRect.x0 + (psSelf->dXRatio * (IMG_DOUBLE)(psDownstreamCoords->i32X - psSelf->sDownstreamOffset.i32X)) - 0.5;
	dSrcY = (IMG_DOUBLE)psSelf->sUpstreamClipRect.y0 + (psSelf->dYRatio * (IMG_DOUBLE)(psDownstreamCoords->i32Y - psSelf->sDownstreamOffset.i32Y)) - 0.5;

	/*get the 4 closest pixels coordinates, checking bounds*/
	iSrcXa = iSrcXb = (IMG_INT32) (dSrcX);
	dDx = dSrcX - (IMG_DOUBLE)iSrcXa;
	if (dDx < 0)
	{
		iSrcXa = iSrcXb = 0;
		dDx = 0;

	}
	else if (iSrcXa != psSelf->sUpstreamClipRect.x1 - 1)
	{
		iSrcXb++;
	}

	iSrcYa = iSrcYb = (IMG_INT32) (dSrcY);
	dDy = dSrcY - (IMG_DOUBLE)iSrcYa;
	if (dDy < 0)
	{
		iSrcYa = iSrcYb = 0;
		dDy = 0;
	}
	else if (iSrcYa != psSelf->sUpstreamClipRect.y1 - 1)
	{
		iSrcYb++;
	}


	/*this is the cache check, if disabled will fetch every pixel always*/
	#if 1
	if (psSelf->sCacheUpperLeftCorner.i32X == iSrcXa - 1 && psSelf->sCacheUpperLeftCorner.i32Y == iSrcYa)
	{
		/*next column: shift the pixels backwards*/
		psSelf->asPixelCache[0] = psSelf->asPixelCache[1];
		psSelf->asPixelCache[2] = psSelf->asPixelCache[3];

		sUpstreamCoords.i32X = iSrcXb;
		sUpstreamCoords.i32Y = iSrcYa;
		eError = BL_OBJECT_PIPECALL(psObject, 0, &sUpstreamCoords, &psSelf->asPixelCache[1]);
		if (eError != PVRSRV_OK)
		{
			return eError;
		}

		sUpstreamCoords.i32X = iSrcXb;
		sUpstreamCoords.i32Y = iSrcYb;
		eError = BL_OBJECT_PIPECALL(psObject, 0, &sUpstreamCoords, &psSelf->asPixelCache[3]);
		if (eError != PVRSRV_OK)
		{
			return eError;
		}
	}
	else if (psSelf->sCacheUpperLeftCorner.i32X == iSrcXa + 1 && psSelf->sCacheUpperLeftCorner.i32Y == iSrcYa)
	{
		/*previous column: shift the pixels forward*/
		psSelf->asPixelCache[1] = psSelf->asPixelCache[0];
		psSelf->asPixelCache[3] = psSelf->asPixelCache[2];

		sUpstreamCoords.i32X = iSrcXa;
		sUpstreamCoords.i32Y = iSrcYa;
		eError = BL_OBJECT_PIPECALL(psObject, 0, &sUpstreamCoords, &psSelf->asPixelCache[0]);
		if (eError != PVRSRV_OK)
		{
			return eError;
		}

		sUpstreamCoords.i32X = iSrcXa;
		sUpstreamCoords.i32Y = iSrcYb;
		eError = BL_OBJECT_PIPECALL(psObject, 0, &sUpstreamCoords, &psSelf->asPixelCache[2]);
		if (eError != PVRSRV_OK)
		{
			return eError;
		}
	}
	else if (psSelf->sCacheUpperLeftCorner.i32X == iSrcXa && psSelf->sCacheUpperLeftCorner.i32Y == iSrcYa - 1)
	{
		/*next row: shift the pixels upwards*/
		psSelf->asPixelCache[0] = psSelf->asPixelCache[2];
		psSelf->asPixelCache[1] = psSelf->asPixelCache[3];

		sUpstreamCoords.i32X = iSrcXa;
		sUpstreamCoords.i32Y = iSrcYb;
		eError = BL_OBJECT_PIPECALL(psObject, 0, &sUpstreamCoords, &psSelf->asPixelCache[2]);
		if (eError != PVRSRV_OK)
		{
			return eError;
		}

		sUpstreamCoords.i32X = iSrcXb;
		sUpstreamCoords.i32Y = iSrcYb;
		eError = BL_OBJECT_PIPECALL(psObject, 0, &sUpstreamCoords, &psSelf->asPixelCache[3]);
		if (eError != PVRSRV_OK)
		{
			return eError;
		}
	}
	else if (psSelf->sCacheUpperLeftCorner.i32X == iSrcXa && psSelf->sCacheUpperLeftCorner.i32Y == iSrcYa + 1)
	{
		/*previous row: shift the pixels downwards*/
		psSelf->asPixelCache[2] = psSelf->asPixelCache[0];
		psSelf->asPixelCache[3] = psSelf->asPixelCache[1];

		sUpstreamCoords.i32X = iSrcXa;
		sUpstreamCoords.i32Y = iSrcYa;
		eError = BL_OBJECT_PIPECALL(psObject, 0, &sUpstreamCoords, &psSelf->asPixelCache[0]);
		if (eError != PVRSRV_OK)
		{
			return eError;
		}

		sUpstreamCoords.i32X = iSrcXb;
		sUpstreamCoords.i32Y = iSrcYa;
		eError = BL_OBJECT_PIPECALL(psObject, 0, &sUpstreamCoords, &psSelf->asPixelCache[1]);
		if (eError != PVRSRV_OK)
		{
			return eError;
		}
	}
	else if (psSelf->sCacheUpperLeftCorner.i32X != iSrcXa || psSelf->sCacheUpperLeftCorner.i32Y != iSrcYa)
	#endif /*caching*/
	{
		/*nothing useful, get everything*/
		sUpstreamCoords.i32X = iSrcXa;
		sUpstreamCoords.i32Y = iSrcYa;
		eError = BL_OBJECT_PIPECALL(psObject, 0, &sUpstreamCoords, &psSelf->asPixelCache[0]);
		if (eError != PVRSRV_OK)
		{
			return eError;
		}

		sUpstreamCoords.i32X = iSrcXb;
		sUpstreamCoords.i32Y = iSrcYa;
		eError = BL_OBJECT_PIPECALL(psObject, 0, &sUpstreamCoords, &psSelf->asPixelCache[1]);
		if (eError != PVRSRV_OK)
		{
			return eError;
		}

		sUpstreamCoords.i32X = iSrcXa;
		sUpstreamCoords.i32Y = iSrcYb;
		eError = BL_OBJECT_PIPECALL(psObject, 0, &sUpstreamCoords, &psSelf->asPixelCache[2]);
		if (eError != PVRSRV_OK)
		{
			return eError;
		}

		sUpstreamCoords.i32X = iSrcXb;
		sUpstreamCoords.i32Y = iSrcYb;
		eError = BL_OBJECT_PIPECALL(psObject, 0, &sUpstreamCoords, &psSelf->asPixelCache[3]);
		if (eError != PVRSRV_OK)
		{
			return eError;
		}
	}
	/*now the cache has all the data needed, update coordinates*/
	psSelf->sCacheUpperLeftCorner.i32X = iSrcXa;
	psSelf->sCacheUpperLeftCorner.i32Y = iSrcYa;

	/*calculate weights*/
	adPixelWeights[0] = (1 - dDx) * (1 - dDy);
	adPixelWeights[1] = dDx * (1 - dDy);
	adPixelWeights[2] = (1 - dDx) * dDy;
	adPixelWeights[3] = dDx * dDy;

	psSelf->pfnBlendFunction(psPixel, psSelf->asPixelCache, adPixelWeights, 4);

	return PVRSRV_OK;
}

/**
 * Function Name	: _bl_op_scale_bilinear_caching
 * Inputs			: psSelf, psMappingRect, ePipePxFmt
 * Outputs			: psMappingRect
 * Returns			: PVRSRV_ERROR
 * Description		: Calculates the dimension ratios, prepares the blending
 *					  function and updates the mapping rect.
 */
static PVRSRV_ERROR _bl_op_scale_bilinear_caching(BL_OBJECT				*psObject,
												  IMG_RECT				*psMappingRect,
												  BL_INTERNAL_PX_FMT	ePipePxFmt)
{
	/* PRQA S 3305 1 */ /* ignore strict aligment warning */
	BL_OP_SCALE_BILINEAR *psSelf = (BL_OP_SCALE_BILINEAR *) psObject;

	PVR_ASSERT(ePipePxFmt < BL_INTERNAL_PX_FMT_UNAVAILABLE);

	/* PRQA S 3689 1 */ /* bounds not exceeded */
	psSelf->pfnBlendFunction = gas_BLInternalPixelTable[ePipePxFmt].pfnBlendFunction;
	if (psSelf->pfnBlendFunction == IMG_NULL)
	{
		return PVRSRV_ERROR_BLIT_SETUP_FAILED;
	}

	psSelf->sDownstreamOffset.i32X = psMappingRect->x0;
	psSelf->sDownstreamOffset.i32Y = psMappingRect->y0;

	/*assign (-2,-2) to ensure that it's not neighbour of (0,0)*/
	psSelf->sCacheUpperLeftCorner.i32X = psSelf->sCacheUpperLeftCorner.i32Y = -2;

	psSelf->dXRatio = (IMG_DOUBLE)(psSelf->sUpstreamClipRect.x1 - psSelf->sUpstreamClipRect.x0) /
		(IMG_DOUBLE) (psMappingRect->x1 - psMappingRect->x0);
	psSelf->dYRatio = (IMG_DOUBLE)(psSelf->sUpstreamClipRect.y1 - psSelf->sUpstreamClipRect.y0) / 
		(IMG_DOUBLE) (psMappingRect->y1 - psMappingRect->y0);

	*psMappingRect = psSelf->sUpstreamClipRect;

	return PVRSRV_OK;
}

/**
 * Function Name	: BLOPScaleBilinearInit
 * Inputs			: psSelf, psUpstreamClipRect, psUpstreamObject
 * Outputs			: psSelf
 * Returns			: Nothing
 * Description		: Initialises the Bilinear Scaling operation.
 */
IMG_INTERNAL IMG_VOID BLOPScaleBilinearInit(BL_OP_SCALE_BILINEAR *psSelf,
											IMG_RECT *psUpstreamClipRect,
											BL_OBJECT *psUpstreamObject)
{
	BLInitObject(&psSelf->sObject);

	if (psUpstreamClipRect != IMG_NULL)
	{
		psSelf->sUpstreamClipRect = *psUpstreamClipRect;
	}

	psSelf->sObject.bDoesBlending = IMG_TRUE;
	psSelf->sObject.pfnGetPixel = &_bl_op_scale_bilinear_get_pixel;
	psSelf->sObject.pfnDoCaching = &_bl_op_scale_bilinear_caching;

	psSelf->sObject.apsUpstreamObjects[0] = psUpstreamObject;
}

/**
 * Function Name	: _bl_op_nop_get_pixel
 * Inputs			: psObject, psDownstreamCoords, psPixel
 * Outputs			: psPixel
 * Returns			: Nothing
 * Description		: Flips the coordinates upside down before getting the
 *					  pixel from the upstream object.
 */
static PVRSRV_ERROR _bl_op_nop_get_pixel(BL_OBJECT	*psObject,
										 BL_COORDS	*psDownstreamCoords,
										 BL_PIXEL	*psPixel)
{
	return BL_OBJECT_PIPECALL(psObject, 0, psDownstreamCoords, psPixel);
}

/*
 * BL_OP_FLIP
 */

/**
 * Function Name	: _bl_op_flip_x_get_pixel
 * Inputs			: psObject, psDownstreamCoords
 * Outputs			: psPixel
 * Returns			: Nothing
 * Description		: Flips the coordinates horizontally before getting the
 *					  pixel from the upstream object.
 */
static PVRSRV_ERROR _bl_op_flip_x_get_pixel(BL_OBJECT	*psObject,
											BL_COORDS	*psDownstreamCoords,
											BL_PIXEL	*psPixel)
{
	BL_OP_FLIP *psSelf = (BL_OP_FLIP*) psObject;
	BL_COORDS sUpstreamCoords;

	sUpstreamCoords.i32Y = psDownstreamCoords->i32Y;
	sUpstreamCoords.i32X = psSelf->i32SumX - psDownstreamCoords->i32X - 1;

	return BL_OBJECT_PIPECALL(psSelf, 0, &sUpstreamCoords, psPixel);
}

/**
 * Function Name	: _bl_op_flip_y_get_pixel
 * Inputs			: psObject, psDownstreamCoords
 * Outputs			: psPixel
 * Returns			: Nothing
 * Description		: Flips the coordinates vertically before getting the
 *					  pixel from the upstream object.
 */
static PVRSRV_ERROR _bl_op_flip_y_get_pixel(BL_OBJECT	*psObject,
											BL_COORDS	*psDownstreamCoords,
											BL_PIXEL	*psPixel)
{
	BL_OP_FLIP *psSelf = (BL_OP_FLIP*) psObject;
	BL_COORDS sUpstreamCoords;

	sUpstreamCoords.i32X = psDownstreamCoords->i32X;
	sUpstreamCoords.i32Y = psSelf->i32SumY - psDownstreamCoords->i32Y - 1;

	return BL_OBJECT_PIPECALL(psSelf, 0, &sUpstreamCoords, psPixel);
}

/**
 * Function Name	: _bl_op_flip_xy_get_pixel
 * Inputs			: psObject, psDownstreamCoords
 * Outputs			: psPixel
 * Returns			: PVRSRV_ERROR
 * Description		: Flips the coordinates vertically and horizontally before
 *					  getting the pixel from the upstream object.
 */
static PVRSRV_ERROR _bl_op_flip_xy_get_pixel(BL_OBJECT * psObject,
											 BL_COORDS * psDownstreamCoords,
											 BL_PIXEL * psPixel)
{
	BL_OP_FLIP *psSelf = (BL_OP_FLIP*) psObject;
	BL_COORDS sUpstreamCoords;

	sUpstreamCoords.i32X = psSelf->i32SumX - psDownstreamCoords->i32X - 1;
	sUpstreamCoords.i32Y = psSelf->i32SumY - psDownstreamCoords->i32Y - 1;

	return BL_OBJECT_PIPECALL(psSelf, 0, &sUpstreamCoords, psPixel);
}

/**
 * Function Name	: _bl_op_flip_caching
 * Inputs			: psObject, psDownstreamMappingRect, ePipePxFmt (ignored)
 * Outputs			: none
 * Returns			: PVRSRV_ERROR
 * Description		: Prepares the flip function.
 */
static PVRSRV_ERROR _bl_op_flip_caching(BL_OBJECT			*psObject,
										IMG_RECT			*psDownstreamMappingRect,
										BL_INTERNAL_PX_FMT	ePipePxFmt)
{
	BL_OP_FLIP *psSelf = (BL_OP_FLIP*) psObject;

	PVR_UNREFERENCED_PARAMETER(ePipePxFmt);

	psSelf->i32SumX = psDownstreamMappingRect->x0 + psDownstreamMappingRect->x1;
	psSelf->i32SumY = psDownstreamMappingRect->y0 + psDownstreamMappingRect->y1;

	/*set the specialised do function*/
	switch (psSelf->ui32Flags & (BL_OP_FLIP_X | BL_OP_FLIP_Y))
	{
		case BL_OP_FLIP_X:
		{
			psObject->pfnGetPixel = &_bl_op_flip_x_get_pixel;
			break;
		}
		case BL_OP_FLIP_Y:
		{
			psObject->pfnGetPixel = &_bl_op_flip_y_get_pixel;
			break;
		}
		case BL_OP_FLIP_X | BL_OP_FLIP_Y:
		{
			psObject->pfnGetPixel = &_bl_op_flip_xy_get_pixel;
			break;
		}
		default:
		{
			psObject->pfnGetPixel = &_bl_op_nop_get_pixel;
			break;
		}
	}

	return PVRSRV_OK;
}

/**
 * Function Name	: BLOPFlipInit
 * Inputs			: psSelf, ui32Flags, psUpstreamObject
 * Outputs			: psSelf
 * Returns			: Nothing
 * Description		: Initialises the Flip operation.
 *
 * The ui32Flags are a bitwise or combination of BL_OP_FLIP_X and BL_OP_FLIP_Y
 */
IMG_INTERNAL IMG_VOID BLOPFlipInit(BL_OP_FLIP	*psSelf,
								   IMG_UINT32	ui32Flags,
								   BL_OBJECT	*psUpstreamObject)
{
	BLInitObject(&psSelf->sObject);

	psSelf->ui32Flags = ui32Flags;

	psSelf->sObject.pfnDoCaching = &_bl_op_flip_caching;

	psSelf->sObject.apsUpstreamObjects[0] = psUpstreamObject;
}

/*
 * BL_OP_ROTATE
 */

/**
 * Function Name	: _bl_op_rotate_90_get_pixel
 * Inputs			: psObject, psCoords, psPixel
 * Outputs			: psPixel
 * Returns			: Nothing
 * Description		: Rotates the coordinates 90 degrees before getting the
 *					  pixel from the upstream object.
 */
static PVRSRV_ERROR _bl_op_rotate_90_get_pixel(BL_OBJECT	*psObject,
											   BL_COORDS	*psDownstreamCoords,
											   BL_PIXEL		*psPixel)
{
	BL_OP_ROTATE *psSelf = (BL_OP_ROTATE*) psObject;
	BL_COORDS sUpstreamCoords;

	sUpstreamCoords.i32X = psSelf->i32Tmp - psDownstreamCoords->i32Y;
	sUpstreamCoords.i32Y = psDownstreamCoords->i32X;

	return BL_OBJECT_PIPECALL(psSelf, 0, &sUpstreamCoords, psPixel);
}

/**
 * Function Name	: _bl_op_rotate_180_get_pixel
 * Inputs			: psObject, psCoords, psPixel
 * Outputs			: psPixel
 * Returns			: Nothing
 * Description		: Rotates the coordinates 180 degrees before getting the
 *					  pixel from the upstream object.
 */
static PVRSRV_ERROR _bl_op_rotate_180_get_pixel(BL_OBJECT	*psObject,
												BL_COORDS	*psDownstreamCoords,
												BL_PIXEL	*psPixel)
{
	BL_COORDS sUpstreamCoords;
	BL_OP_ROTATE *psSelf;

	psSelf = (BL_OP_ROTATE *) psObject;

	sUpstreamCoords.i32X = psSelf->sMappingRect.x0 + psSelf->sMappingRect.x1 - psDownstreamCoords->i32X - 1;
	sUpstreamCoords.i32Y = psSelf->sMappingRect.y0 + psSelf->sMappingRect.y1 - psDownstreamCoords->i32Y - 1;

	return BL_OBJECT_PIPECALL(psObject, 0, &sUpstreamCoords, psPixel);
}

/**
 * Function Name	: _bl_op_rotate_270_get_pixel
 * Inputs			: psObject, psCoords, psPixel
 * Outputs			: psPixel
 * Returns			: Nothing
 * Description		: Rotates the coordinates 270 degrees before getting the
 *					  pixel from the upstream object.
 */
static PVRSRV_ERROR _bl_op_rotate_270_get_pixel(BL_OBJECT	*psObject,
												BL_COORDS	*psCoords,
												BL_PIXEL	*psPixel)
{
	BL_OP_ROTATE * psSelf = (BL_OP_ROTATE*) psObject;
	BL_COORDS sNewCoords;

	sNewCoords.i32X = psCoords->i32Y;
	sNewCoords.i32Y = psSelf->i32Tmp - psCoords->i32X;

	return BL_OBJECT_PIPECALL(psObject, 0, &sNewCoords, psPixel);
}

/**
 * Function Name	: _bl_op_rotate_caching
 * Inputs			: psObject, psMappingRect, ePipePxFmt (ignored)
 * Outputs			: psMappingRect
 * Returns			: PVRSRV_ERROR
 * Description		: Prepares the rotate function and updates the
 *					  psDownstreamMappingRect so the upstream objects can see
 *					  the transformed coordinates.
 */
static PVRSRV_ERROR _bl_op_rotate_caching(BL_OBJECT				*psObject,
										  IMG_RECT				*psMappingRect,
										  BL_INTERNAL_PX_FMT	ePipePxFmt)
{
	BL_OP_ROTATE * psSelf = (BL_OP_ROTATE*) psObject;

	PVR_UNREFERENCED_PARAMETER(ePipePxFmt);

	/* set the specialised do function*/
	switch (psSelf->eRotation)
	{
		case PVRSRV_FLIP_Y:
		case PVRSRV_ROTATE_0:
		{
			psSelf->i32Tmp = 0;

			BL_OBJECT_ASSOC_FUNC(psObject) = &_bl_op_nop_get_pixel;
			break;
		}
		case PVRSRV_ROTATE_90:
		{
			IMG_INT32 i32Tmp;

			psSelf->i32Tmp = psMappingRect->y1 + psMappingRect->y0 - 1;

			/* rotate the mapping quad */
			i32Tmp = psMappingRect->x0;
			psMappingRect->x0 = psMappingRect->y0;
			psMappingRect->y0 = i32Tmp;

			i32Tmp = psMappingRect->x1;
			psMappingRect->x1 = psMappingRect->y1;
			psMappingRect->y1 = i32Tmp;

			BL_OBJECT_ASSOC_FUNC(psObject) = &_bl_op_rotate_90_get_pixel;
			break;
		}
		case PVRSRV_ROTATE_180:
		{
			BL_OBJECT_ASSOC_FUNC(psObject) = &_bl_op_rotate_180_get_pixel;
			break;
		}
		case PVRSRV_ROTATE_270:
		{
			IMG_INT32 i32Tmp;

			psSelf->i32Tmp = psMappingRect->x1 + psMappingRect->x0 - 1;

			/* rotate the mapping quad */
			i32Tmp = psMappingRect->x0;
			psMappingRect->x0 = psMappingRect->y0;
			psMappingRect->y0 = i32Tmp;

			i32Tmp = psMappingRect->x1;
			psMappingRect->x1 = psMappingRect->y1;
			psMappingRect->y1 = i32Tmp;

			BL_OBJECT_ASSOC_FUNC(psObject) = &_bl_op_rotate_270_get_pixel;
			break;
		}
	}
	psSelf->sMappingRect = *psMappingRect;

	return PVRSRV_OK;
}

/**
 * Function Name	: BLOPRotateInit
 * Inputs			: psSelf, eRotation, psUpstreamObject
 * Outputs			: psSelf
 * Returns			: Nothing
 * Description		: Initialises the Rotate operation.
 */
IMG_INTERNAL IMG_VOID BLOPRotateInit(BL_OP_ROTATE		*psSelf,
									 PVRSRV_ROTATION	eRotation,
									 BL_OBJECT			*psUpstreamObject)
{
	BLInitObject(&psSelf->sObject);

	psSelf->eRotation = eRotation;

	psSelf->sObject.pfnDoCaching = &_bl_op_rotate_caching;

	psSelf->sObject.apsUpstreamObjects[0] = psUpstreamObject;
}


/*
 * BL_OP_ALPHA_BLEND
 */

/**
 * Function Name	: _bl_op_alpha_blend_get_pixel
 * Inputs			: psObject, psCoords
 * Outputs			: psPixel
 * Returns			: Nothing
 * Description		: Reads the pixels from the upstream objects and blends the
 *					  colours.
 */
static PVRSRV_ERROR _bl_op_alpha_blend_get_pixel(BL_OBJECT	*psObject,
												 BL_COORDS	*psCoords,
												 BL_PIXEL	*psPixel)
{
	PVRSRV_ERROR eError;
	/* PRQA S 3305 1 */ /* ignore strict aligment warning */
	BL_OP_ALPHA_BLEND *psSelf = (BL_OP_ALPHA_BLEND *) psObject;
	BL_PIXEL asPixelsIn[2];

	eError = BL_OBJECT_PIPECALL(psSelf, 0, psCoords, &asPixelsIn[0]);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	eError = BL_OBJECT_PIPECALL(psSelf, 1, psCoords, &asPixelsIn[1]);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	psSelf->pfnBlend(psPixel, asPixelsIn, psSelf->adWeights, 2);
	return PVRSRV_OK;
}

/**
 * Function Name	: _bl_op_alpha_blend_caching
 * Inputs			: psObject, psRect (ignored), ePipePxFmt
 * Outputs			: Nothing
 * Returns			: Nothing
 * Description		: Prepares the blending function.
 */
static PVRSRV_ERROR _bl_op_alpha_blend_caching(BL_OBJECT			*psObject,
											   IMG_RECT				*psRect,
											   BL_INTERNAL_PX_FMT	ePipePxFmt)
{
	/* PRQA S 3305 1 */ /* ignore strict aligment warning */
	BL_OP_ALPHA_BLEND *psSelf = (BL_OP_ALPHA_BLEND *) psObject;
	PVR_UNREFERENCED_PARAMETER(psRect);

	PVR_ASSERT(ePipePxFmt < BL_INTERNAL_PX_FMT_UNAVAILABLE);

	/* PRQA S 3689 1 */ /* bounds not exceeded */
	psSelf->pfnBlend = gas_BLInternalPixelTable[ePipePxFmt].pfnBlendFunction;
	if (psSelf->pfnBlend == IMG_NULL)
	{
		return PVRSRV_ERROR_BLIT_SETUP_FAILED;
	}
	return PVRSRV_OK;
}

/**
 * Function Name	: BLOPAlphaBlendInit
 * Inputs			: psSelf, dGlobalAlpha, psObjectA, psObjectB
 * Outputs			: psSelf
 * Returns			: Nothing
 * Description		: Initialises the Alpha Blend operation.
 *
 * psObjectA is the object which its pixels will be multiplied by dGlobalAlpha,
 * psObjectB pixels will be multiplied by (1-dGlobalAlpha).
 */
IMG_INTERNAL IMG_VOID BLOPAlphaBlendInit(BL_OP_ALPHA_BLEND	*psSelf,
										 IMG_DOUBLE			dGlobalAlpha,
										 BL_OBJECT			*psObjectA,
										 BL_OBJECT			*psObjectB)
{
	BLInitObject(&psSelf->sObject);

	psSelf->adWeights[0] = dGlobalAlpha;
	psSelf->adWeights[1] = 1 - dGlobalAlpha;

	psSelf->sObject.bDoesBlending = IMG_TRUE;
	psSelf->sObject.pfnDoCaching = &_bl_op_alpha_blend_caching;
	psSelf->sObject.pfnGetPixel = &_bl_op_alpha_blend_get_pixel;

	psSelf->sObject.apsUpstreamObjects[0] = psObjectA;
	psSelf->sObject.apsUpstreamObjects[1] = psObjectB;
}


/*
 * SOURCE_COLOUR_KEY
 */

/**
 * Function Name	: _bl_op_colour_key_get_pixel
 * Inputs			: psObject, psCoords
 * Outputs			: psPixel
 * Returns			: Nothing
 * Description		: Does a colour key operation.
 *
 * The upstream[0] object is always taken as the pixel that
 * is compared against the colour key. Swap the upstream objects to do src/dst
 * colour key operations.
 */
static PVRSRV_ERROR _bl_op_colour_key_get_pixel(BL_OBJECT	*psObject,
												BL_COORDS	*psCoords,
												BL_PIXEL	*psPixel)
{
	PVRSRV_ERROR eError;
	BL_OP_COLOUR_KEY *psSelf = (BL_OP_COLOUR_KEY*) psObject;
	IMG_UINT uiByte;
	IMG_BYTE *pbyPix, *pbyCK, *pbyMask;
	BL_PIXEL sTmpPixel;

	eError = BL_OBJECT_PIPECALL(psSelf, 0, psCoords, &sTmpPixel);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	pbyCK = (IMG_BYTE*) &psSelf->sColourKey;
	pbyPix = (IMG_BYTE*) &sTmpPixel;
	pbyMask = (IMG_BYTE*) &psSelf->sMask;

	/*do a byte per byte comparison*/
	for(uiByte = 0; uiByte < psSelf->uiColourKeySize; uiByte++)
	{
		if ((pbyPix[uiByte] & pbyMask[uiByte]) != (pbyCK[uiByte] & pbyMask[uiByte]))
		{
			*psPixel = sTmpPixel;
			return PVRSRV_OK;
		}
	}
	return BL_OBJECT_PIPECALL(psSelf, 1, psCoords, psPixel);
}

/**
 * Function Name	: _bl_op_colour_key_caching
 * Inputs			: psObject, psRect (ignored), ePipePxFmt
 * Outputs			: Nothing
 * Returns			: Nothing
 * Description		: Prepares the colour key internal pixel format to be the
 *					  same as the pipe's.
 */
static PVRSRV_ERROR _bl_op_colour_key_caching(BL_OBJECT				*psObject,
											  IMG_RECT				*psRect,
											  BL_INTERNAL_PX_FMT	ePipePxFmt)
{
	BL_OP_COLOUR_KEY *psSelf = (BL_OP_COLOUR_KEY*) psObject;

	PVR_UNREFERENCED_PARAMETER(psRect);

	PVR_ASSERT(ePipePxFmt < BL_INTERNAL_PX_FMT_UNAVAILABLE);

	if (psSelf->eColourKeyFormat != ePipePxFmt)
	{
		BL_INTERNAL_FORMAT_CONVERT_FUNC pfnConvert;

		PVR_ASSERT(psSelf->eColourKeyFormat < BL_INTERNAL_PX_FMT_UNAVAILABLE);

		/* PRQA S 3689 1 */ /* bounds not exceeded */
		pfnConvert = gafn_BLInternalPixelFormatConversionTable[psSelf->eColourKeyFormat][ePipePxFmt];
		if (pfnConvert == IMG_NULL)
		{
			return PVRSRV_ERROR_BLIT_SETUP_FAILED;
		}

		pfnConvert((BL_PIXEL*)(&psSelf->sColourKey), &psSelf->sColourKey);
		pfnConvert((BL_PIXEL*)(&psSelf->sMask), &psSelf->sMask);

		psSelf->eColourKeyFormat = ePipePxFmt;
	}

	/* PRQA S 3689 1 */ /* bounds not exceeded */
	psSelf->uiColourKeySize = gas_BLInternalPixelTable[ePipePxFmt].uiBytesPerPixel;
	return PVRSRV_OK;
}

/**
 * Function Name	: BLOPColourKeyInit
 * Inputs			: psSelf, eColourKeyFormat, psColourKey, psMask, psFront, psBack
 * Outputs			: psSelf
 * Returns			: Nothing
 * Description		: Initializes the Colour Key operation.
 *
 * Te psFront is the object which it's pixels will be compared against the key,
 * and if they match the pixel will be taken from the psBack object.
 */
IMG_INTERNAL IMG_VOID BLOPColourKeyInit(BL_OP_COLOUR_KEY	*psSelf,
										BL_PIXEL			*psColourKey,
										BL_PIXEL			*psMask,
										BL_INTERNAL_PX_FMT	eColourKeyFormat,
										BL_OBJECT			*psFront,
										BL_OBJECT			*psBack)
{
	BLInitObject(&psSelf->sObject);

	PVR_ASSERT(eColourKeyFormat < BL_INTERNAL_PX_FMT_UNAVAILABLE);

	psSelf->eColourKeyFormat = eColourKeyFormat;
	if (psColourKey != IMG_NULL)
	{
		psSelf->sColourKey = *psColourKey;
	}
	if (psMask != IMG_NULL)
	{
		psSelf->sMask = *psMask;
	}

	psSelf->sObject.pfnGetPixel = &_bl_op_colour_key_get_pixel;
	psSelf->sObject.pfnDoCaching = &_bl_op_colour_key_caching;

	psSelf->sObject.apsUpstreamObjects[0] = psFront;
	psSelf->sObject.apsUpstreamObjects[1] = psBack;
}

/******************************************************************************
 End of file (blitlib_op.c)
 ******************************************************************************/
