/*!****************************************************************************
@File           blitlib_src.h

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
$Log: blitlib_src.h $
*******************************************************************************
Renamed from blitlib_tf.h
 --- Revision Logs Removed --- 
******************************************************************************/


#if ! defined(_BLITLIB_SRC_H_)
#define _BLITLIB_SRC_H_

#include "blitlib.h"

/*
 * Strided texture read instance.
 */
typedef struct _BL_SRC_LINEAR_
{
	/* must be the first member*/
	BL_OBJECT				sObject;

	IMG_INT32				i32ByteStride;
	IMG_UINT32				ui32Height;
	IMG_PBYTE				pbyFBAddr;
	BL_PLANAR_SURFACE_INFO	sPlanarSurfaceInfo;

	/* private */
	BL_INTERNAL_FORMAT_RW_FUNC		pfnPxRead;
	BL_INTERNAL_FORMAT_CONVERT_FUNC	pfnConvert;
} BL_SRC_LINEAR;

IMG_INTERNAL IMG_VOID BLSRCLinearInit(BL_SRC_LINEAR				*psSelf,
									  IMG_INT32					i32ByteStride,
									  IMG_UINT32				ui32Height,
									  PVRSRV_PIXEL_FORMAT		ePixelFormat,
									  IMG_PBYTE					pbyFBAddr,
									  BL_PLANAR_SURFACE_INFO	*psPlanarInfo);

/*
 * twiddled texture read
 */
typedef struct _BL_SRC_TWIDDLED_
{
	/* must be the first member*/
	BL_OBJECT				sObject;

	IMG_UINT32				ui32Width;		/* pow of 2 */
	IMG_UINT32				ui32Height;		/* pow of 2 */
	IMG_PBYTE				pbyFBAddr;
	BL_PLANAR_SURFACE_INFO	sPlanarSurfaceInfo;
	IMG_BOOL				bHybridTwiddling;

	/* private */
	IMG_UINT32			ui32TwiddlingMask;
	IMG_INT32			i32Y;
	IMG_UINT32			ui32YStretched;
	IMG_INT32			i32X;
	IMG_UINT32			ui32XStretched;
	IMG_UINT32			ui32LinearPartMultX;
	IMG_UINT32			ui32LinearPartMultY;

	BL_INTERNAL_FORMAT_RW_FUNC		pfnPxRead;
	BL_INTERNAL_FORMAT_CONVERT_FUNC	pfnConvert;
} BL_SRC_TWIDDLED;

IMG_INTERNAL IMG_VOID BLSRCTwiddledInit(BL_SRC_TWIDDLED			*psSelf,
										IMG_UINT32				ui32Width,
										IMG_UINT32				ui32Height,
									    PVRSRV_PIXEL_FORMAT		ePixelFormat,
										IMG_PBYTE				pbyFBAddr,
										BL_PLANAR_SURFACE_INFO	*psPlanarInfo,
										IMG_BOOL				bHybridTwiddling);

/*
 * generate solid colour
 */
typedef struct _BL_SRC_SOLID_
{
	/* must be the first member*/
	BL_OBJECT			sObject;

	/*the solid colour to be copied*/
	BL_PIXEL			sColour;

	/*the pixel format of the sColour attribute*/
	BL_INTERNAL_PX_FMT	eColourFormat;
} BL_SRC_SOLID;

IMG_INTERNAL IMG_VOID BLSRCSolidInit(BL_SRC_SOLID			*psSelf,
									 BL_PIXEL				*psColour,
									 BL_INTERNAL_PX_FMT		eInternalColourFormat,
									 PVRSRV_PIXEL_FORMAT	eExternalColourFormat);

/* tiled and strided share the same structure */
typedef BL_SRC_LINEAR BL_SRC_TILED;


IMG_INTERNAL IMG_VOID BLSRCTiledInit(BL_SRC_TILED			*psSelf,
                                     IMG_INT32				i32ByteStride,
                                     IMG_UINT32				ui32Height,
                                     PVRSRV_PIXEL_FORMAT	ePixelFormat,
                                     IMG_PBYTE				pbyFBAddr,
                                     BL_PLANAR_SURFACE_INFO	*psPlanarInfo);

typedef union
{
	BL_SRC_SOLID solid;
	BL_SRC_LINEAR linear;
	BL_SRC_TWIDDLED twiddled;
	BL_SRC_TILED tiled;
} BL_SRC;



#endif /* _BLITLIB_SRC_H_*/

/******************************************************************************
 End of file (blitlib_src.h)
******************************************************************************/
