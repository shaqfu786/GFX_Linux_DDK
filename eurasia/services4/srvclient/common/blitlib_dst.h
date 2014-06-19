/*!****************************************************************************
@File           blitlib_dst.h

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
$Log: blitlib_dst.h $
*******************************************************************************
Renamed from blitlib_tw.h
 --- Revision Logs Removed --- 
******************************************************************************/

#if ! defined(_BLITLIB_DST_H_)
#define _BLITLIB_DST_H_

#include "blitlib.h"

/*
 * linear write 
 */
typedef struct _BL_DST_LINEAR_
{
	/* must be the first member*/
	BL_OBJECT				sObject;
	IMG_INT32				i32ByteStride;
	IMG_UINT32				ui32Height;
	IMG_PBYTE				pbyFBAddr;
	IMG_RECT				sClipRect;
	BL_PLANAR_SURFACE_INFO	sPlanarSurfaceInfo;
} BL_DST_LINEAR;

IMG_INTERNAL IMG_VOID BLDSTLinearInit(BL_DST_LINEAR				*psSelf,
									  IMG_INT32					i32ByteStride,
									  IMG_UINT32				ui32Height,
									  PVRSRV_PIXEL_FORMAT		ePixelFormat,
									  IMG_RECT					*psClipRect,
									  IMG_PBYTE					pbyFBAddr,
									  BL_OBJECT					*psUpstreamObject,
									  BL_PLANAR_SURFACE_INFO	*psPlanarInfo);

/*
 * twiddled write 
 */
typedef struct _BL_DST_TWIDDLED_
{
	/* must be the first member*/
	BL_OBJECT			sObject;

	IMG_UINT32				ui32Width;		/* pow of 2 */
	IMG_UINT32				ui32Height;		/* pow of 2 */
	IMG_PBYTE				pbyFBAddr;
	IMG_RECT				sClipRect;
	BL_PLANAR_SURFACE_INFO	sPlanarSurfaceInfo;
	IMG_BOOL				bHybridTwiddling;

	/* -- private -- */

	/* number of bits from a coordinate that will be twiddled */
	IMG_UINT				uiTwiddlingLength;
	/* mask for the part to be twiddled in a cartesian coordinate */
	IMG_UINT32				ui32TwiddlingMask;
	 /* values to multiply the non-twiddled parts to convert them to its Z-order value */
	IMG_UINT32				ui32LinearPartMultX;
	IMG_UINT32				ui32LinearPartMultY;
	/* the twiddled coordinates of the corners of the clip rect (both inclusive) */
	IMG_UINT32		ui32zStart;
	IMG_UINT32		ui32zEnd;

	union
	{
		struct
		{
			/* mask for the bits of the twiddled coordinate used for each cartesian coordinate,
			 * not used for hybrid twiddling (as the number of blocks might not be pow of 2) */
			IMG_UINT32		ui32TwiddledMaskX;
			IMG_UINT32		ui32TwiddledMaskY;
		} packed;

		struct
		{
			/* the number of consecutive pixels we can scan in this tile*/
			IMG_UINT32		ui32CurrentTileConsecutive;
		} hybrid;
	} uTwTypeData;

} BL_DST_TWIDDLED;

IMG_INTERNAL IMG_VOID BLDSTTwiddledInit(BL_DST_TWIDDLED			*psSelf,
										IMG_UINT32				ui32Width,
										IMG_UINT32				ui32Height,
										PVRSRV_PIXEL_FORMAT		ePixelFormat,
										IMG_RECT				*psClipRect,
										IMG_PBYTE				pbyFBAddr,
										BL_OBJECT				*psUpstreamObject,
										BL_PLANAR_SURFACE_INFO	*psPlanarInfo,
										IMG_BOOL				bHybridTwiddling);

/* tiled write */

typedef struct _BL_DST_TILED_
{
	/* must be the first member*/
	BL_OBJECT			sObject;

	IMG_INT32				i32ByteStride;
	IMG_UINT32				ui32Height;
	IMG_PBYTE				pbyFBAddr;
	IMG_RECT				sClipRect;
	BL_PLANAR_SURFACE_INFO	sPlanarSurfaceInfo;
} BL_DST_TILED;

IMG_INTERNAL IMG_VOID BLDSTTiledInit(BL_DST_TILED			*psSelf,
                                     IMG_INT32				i32ByteStride,
                                     IMG_UINT32				ui32Height,
                                     PVRSRV_PIXEL_FORMAT	ePixelFormat,
                                     IMG_RECT				*psClipRect,
                                     IMG_PBYTE				pbyFBAddr,
                                     BL_OBJECT				*psUpstreamObject,
                                     BL_PLANAR_SURFACE_INFO	*psPlanarInfo);


typedef union
{
	BL_DST_LINEAR	linear;
	BL_DST_TWIDDLED	twiddled;
	BL_DST_TILED	tiled;
} BL_DST;

#endif /* _BLITLIB_DST_H_*/

/******************************************************************************
 End of file (blitlib_dst.h)
******************************************************************************/
