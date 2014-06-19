/*!****************************************************************************
@File           blitlib.h

@Title         	Blit library 

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
$Log: blitlib.h $
******************************************************************************/

#if ! defined(_BLITLIB_H_)
#define _BLITLIB_H_

#include "img_defs.h"
#include "servicesext.h"
#include "services.h"
#include "blitlib_pixel.h"

/*
 * maximum number of inputs for a BL_OBJECT
 */
#define BL_MAX_SOURCES 2

typedef struct _BL_COORDS_
{
	IMG_INT32			i32X;
	IMG_INT32			i32Y;
} BL_COORDS;

struct _BL_OBJECT_;

typedef PVRSRV_ERROR (*BL_ASSOC_FUNC)(struct _BL_OBJECT_*, BL_COORDS*, BL_PIXEL*);

typedef PVRSRV_ERROR (*BL_CACHE_FUNC)(struct _BL_OBJECT_*, IMG_RECT*, BL_INTERNAL_PX_FMT);

typedef struct _BL_OBJECT_
{
	/* The pipe callback. Receives a pixel position and gives a pixel value.
	 * For the Destination objects, it process the whole surface instead.
	 */
	BL_ASSOC_FUNC		pfnGetPixel;
	/* if the object wants to do caching before the pipe is started
	 * it has to implement this callback. This is always called on every
	 * member of the pipe before the first pixel goes through but after
	 * the full pipe setup.
	 *
	 * Also, this callback can modify the clipping rect to make the subsequent
	 * nodes aware of the geometrical transformations.
	 */
	BL_CACHE_FUNC		pfnDoCaching;
	/*This flag indicates if the object does pixel colour blending.*/
	IMG_BOOL			bDoesBlending;
	/* This is the src/dst external pixel format
	 * - We have it here to avoid overcomplicating the "class" design and avoid
	 * unsafe castings while determining the sources formats.
	 */
	PVRSRV_PIXEL_FORMAT	eExternalPxFmt;

	/* list of upstream objects*/
	struct _BL_OBJECT_ * apsUpstreamObjects[BL_MAX_SOURCES];
} BL_OBJECT;

/*
 * Resets to IMG_NULL the function and source objects pointers.
 */
IMG_INTERNAL IMG_VOID BLInitObject(BL_OBJECT *psObject);

#define BL_OBJECT_ASSOC_FUNC(psObj)		((BL_OBJECT *) (psObj))->pfnGetPixel
#define BL_OBJECT_UPSTREAM(psObj, iUpstreamIndex)	((BL_OBJECT *) (psObj))->apsUpstreamObjects[iUpstreamIndex]

#define BL_OBJECT_PIPECALL(psObj, iUpstreamIndex, psCoords, psPixel)		\
	(*BL_OBJECT_ASSOC_FUNC(BL_OBJECT_UPSTREAM(psObj, iUpstreamIndex)))		\
							((BL_OBJECT_UPSTREAM(psObj, iUpstreamIndex)),	\
							psCoords,										\
							psPixel)

#define BL_OBJECT_START(psObj)					 							\
	(*BL_OBJECT_ASSOC_FUNC(psObj))((BL_OBJECT*)(psObj), IMG_NULL, IMG_NULL)

#define BL_OBJECT_CONNECT(psObjUpstream, iUpstreamIndex, psObjDownstream)	 					\
	((BL_OBJECT_UPSTREAM(psObjDownstream, iUpstreamIndex)) = (BL_OBJECT*)(psObjUpstream))


#endif /* _BLITLIB_H_*/

/******************************************************************************
 End of file (blitlib.h)
******************************************************************************/
