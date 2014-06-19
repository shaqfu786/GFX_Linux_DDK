/*!****************************************************************************
@File           blitlib_op.h

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
$Log: blitlib_op.h $
******************************************************************************/

#if ! defined(_BLITLIB_OP_H_)
#define _BLITLIB_OP_H_

#include "blitlib.h"

/* ========================
 * = REMAPPING OPERATIONS =
 * ======================== */


typedef struct _BL_OP_SCALE_NEAREST_
{
	/* must be the first member*/
	BL_OBJECT			sObject;

	/* The rect that will be clipped upstream (source-bound).
	 * It must be smaller than the source area, or out of bounds error will happen.
	 */
	IMG_RECT			sUpstreamClipRect;

	/*** private ***/

	/* The top left corner of the downstream clipping rect (towards dst), to translate the coords to the affine space.*/
	BL_COORDS			sDownstreamOffset;

	/*Ratio between the downstream (dst) clipping rect width and the upstream (towards src) clipping rect width*/
	IMG_DOUBLE			dXRatio;
	/*Ratio between the downstream (dst) clipping rect height and the upstream (towards src) clipping rect height*/
	IMG_DOUBLE			dYRatio;
} BL_OP_SCALE_NEAREST;

typedef struct _BL_OP_SCALE_BILINEAR_
{
	/* must be the first member*/
	BL_OBJECT			sObject;

	/* The rect that will be clipped upstream (source-bound).
	 * It must be smaller than the source area, or out of bounds error will happen.
	 */
	IMG_RECT			sUpstreamClipRect;

	/*** private ***/

	/* The top left corner of the downstream clipping rect (towards dst), to translate the coords to the affine space.*/
	BL_COORDS			sDownstreamOffset;

	/*Ratio between the downstream (dst) clipping rect width and the upstream (towards src) clipping rect width*/
	IMG_DOUBLE			dXRatio;
	/*Ratio between the downstream (dst) clipping rect height and the upstream (towards src) clipping rect height*/
	IMG_DOUBLE			dYRatio;

	/* pixel cache of the last 4 accessed pixels for the bilinear interpolation,
	 * stored in row major order, as
	 * [val(x,y),val(x+1,y),val(x,y+1),val(x+1,y+1)]
	 * being (x,y) the coordinates of the upper left corner of the square.
	 */
	BL_PIXEL			asPixelCache[4];
	BL_COORDS			sCacheUpperLeftCorner;
	/*Function used to blend the pixels*/
	BL_INTERNAL_FORMAT_BLEND_FUNC	pfnBlendFunction;
} BL_OP_SCALE_BILINEAR;

IMG_INTERNAL IMG_VOID BLOPScaleNearestInit(BL_OP_SCALE_NEAREST *psSelf,
										   IMG_RECT *psUpstreamClipRect,
										   BL_OBJECT *psUpstreamObject);

IMG_INTERNAL IMG_VOID BLOPScaleBilinearInit(BL_OP_SCALE_BILINEAR *psSelf,
											IMG_RECT *psUpstreamClipRect,
											BL_OBJECT *psUpstreamObject);
/*
 * X, Y flip filter 
 */
typedef struct _BL_OP_FLIP_
{
	/* must be the first member*/
	BL_OBJECT			sObject;

#define	BL_OP_FLIP_X	1
#define	BL_OP_FLIP_Y	2
	IMG_UINT32			ui32Flags;

	/* private */
	IMG_INT32			i32SumX;
	IMG_INT32			i32SumY;
} BL_OP_FLIP;

IMG_INTERNAL IMG_VOID BLOPFlipInit(BL_OP_FLIP *psSelf,
								   IMG_UINT32 ui32Flags,
								   BL_OBJECT *psUpstreamObject);

typedef struct _BL_OP_ROTATE_
{
	/* must be the first member*/
	BL_OBJECT			sObject;

	/* FLIP_Y is not supported, why is it in this anyway?*/
	PVRSRV_ROTATION		eRotation;

	/* private */
	IMG_RECT			sMappingRect;
	IMG_INT32			i32Tmp;
} BL_OP_ROTATE;

IMG_INTERNAL IMG_VOID BLOPRotateInit(BL_OP_ROTATE *psSelf,
									 PVRSRV_ROTATION eRotation,
									 BL_OBJECT *psUpstreamObject);


/* =====================
 * = COLOUR OPERATIONS =
 * ===================== */

typedef struct _BL_OP_ALPHA_BLEND_
{
    /* must be the first member*/
	BL_OBJECT			sObject;	
	IMG_DOUBLE			adWeights[2]; /*[0..1]*/

	BL_INTERNAL_FORMAT_BLEND_FUNC pfnBlend;
} BL_OP_ALPHA_BLEND;

IMG_INTERNAL IMG_VOID BLOPAlphaBlendInit(BL_OP_ALPHA_BLEND *psSelf,
										 IMG_DOUBLE dGlobalAlpha,
										 BL_OBJECT *psObjectA,
										 BL_OBJECT *psObjectB);

/*
 * It filters by the colours of the upstream[0], if it matches the key, then take the
 * colour from upstream[1].
 *
 * With upstream[0] = source and upstream[1] = dest => Source Colour Key
 * With upstream[0] = dest and upstream[1] = source => Destination Colour Key
 */
typedef struct _BL_OP_COLOUR_KEY_
{
	/* must be the first member*/
	BL_OBJECT			sObject;

	BL_PIXEL			sColourKey;
	BL_PIXEL			sMask;

	/*specifies the pixel format of sColourKey and sMask*/
	BL_INTERNAL_PX_FMT	eColourKeyFormat;

	/*private*/
	IMG_SIZE_T			uiColourKeySize;
} BL_OP_COLOUR_KEY;

IMG_INTERNAL IMG_VOID BLOPColourKeyInit(BL_OP_COLOUR_KEY *psSelf,
										BL_PIXEL *psColourKey,
										BL_PIXEL *psMask,
										BL_INTERNAL_PX_FMT eColourKeyFormat,
										BL_OBJECT *psFront,
										BL_OBJECT *psBack);

#endif /* _BLITLIB_OP_H_*/

/******************************************************************************
 End of file (blitlib_op.h)
******************************************************************************/
