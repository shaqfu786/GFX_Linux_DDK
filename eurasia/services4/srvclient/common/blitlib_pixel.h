/*!****************************************************************************
@File           blitlib_pixel.h

@Title         	Blit library - pixel formats 

@Author         Imagination Technologies

@date           04/09/2009

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
$Log: blitlib_pixel.h $
 ******************************************************************************/

#include "img_defs.h"
#include "servicesext.h"

#if 0 /*redefine IMG_INTERNAL to nothing when building the internal blitlib_test*/
#undef IMG_INTERNAL
#define IMG_INTERNAL
#endif



/*
 * Types of pixels supported internally in the blitlib pipe.
 *
 * When adding or removing internal pixel types, take care to update the
 * following tables which their contents are dependent on the correct values
 * of the indices:
 * 
 * - gafn_BLInternalPixelFormatConversionTable;
 * - gas_BLInternalPixelTable;
 */
typedef enum _BL_INTERNAL_PX_FMT_
{
	/*non RAW types are assumed to come first*/
	BL_INTERNAL_PX_FMT_ARGB8888,
	BL_INTERNAL_PX_FMT_A2RGB10,
	BL_INTERNAL_PX_FMT_ARGB16,
	/*the code assumes that the RAW formats are >= BL_INTERNAL_PX_FMT_RAW8 */
	BL_INTERNAL_PX_FMT_RAW8,
	BL_INTERNAL_PX_FMT_RAW16,
	BL_INTERNAL_PX_FMT_RAW24,
	BL_INTERNAL_PX_FMT_RAW32,
	BL_INTERNAL_PX_FMT_RAW64,
	BL_INTERNAL_PX_FMT_RAW128,
	BL_INTERNAL_PX_FMT_UNAVAILABLE
} BL_INTERNAL_PX_FMT;

/*Pixel data holders*/
typedef struct _BL_PIXEL_ARGB8888_
{
	IMG_UINT32 ui32Value;
} BL_PIXEL_ARGB8888;

typedef struct _BL_PIXEL_A2RGB10_
{
	IMG_UINT32 ui32Value;
} BL_PIXEL_A2RGB10;

typedef struct _BL_PIXEL_ARGB16_
{
	IMG_UINT16 aui16Values[4];/*0:A, 1:R, 2:G, 3:B*/
} BL_PIXEL_ARGB16;

typedef struct _BL_PIXEL_RAW8_
{
	IMG_UINT8 ui8Payload;
} BL_PIXEL_RAW8;

typedef struct _BL_PIXEL_RAW16_
{
	IMG_UINT16 ui16Payload;
} BL_PIXEL_RAW16;

typedef struct _BL_PIXEL_RAW24_
{
	IMG_UINT8 aui8Payload[3];
} BL_PIXEL_RAW24;

typedef struct _BL_PIXEL_RAW32_
{
	IMG_UINT32 ui32Payload;
} BL_PIXEL_RAW32;

typedef struct _BL_PIXEL_RAW64_
{
	IMG_UINT32 aui32Payload[2];
} BL_PIXEL_RAW64;

typedef struct _BL_PIXEL_RAW128_
{
	IMG_UINT32 aui32Payload[4];
} BL_PIXEL_RAW128;

/*
 * generic pixel
 */
typedef union _BL_PIXEL_
{
	BL_PIXEL_ARGB8888	sPixelARGB8888;
	BL_PIXEL_A2RGB10	sPixelA2RGB10;
	BL_PIXEL_ARGB16		sPixelARGB16;
	BL_PIXEL_RAW8		sPixelRAW8;
	BL_PIXEL_RAW16		sPixelRAW16;
	BL_PIXEL_RAW24		sPixelRAW24;
	BL_PIXEL_RAW32		sPixelRAW32;
	BL_PIXEL_RAW64		sPixelRAW64;
	BL_PIXEL_RAW128		sPixelRAW128;
} BL_PIXEL;

#define BL_PIXEL_MAX_SIZE (sizeof(BL_PIXEL))

typedef union _BL_PTR_
{
	IMG_BYTE	*by;
	IMG_UINT16	*w;
	IMG_UINT32	*dw;
} BL_PTR;

typedef struct _BL_PLANAR_SURFACE_INFO_
{
	IMG_BOOL	bIsPlanar;
    IMG_INT32	i32ChunkStride;
    IMG_UINT	uiNChunks;
    IMG_UINT	uiPixelBytesPerPlane;
} BL_PLANAR_SURFACE_INFO;

typedef IMG_VOID (*BL_INTERNAL_FORMAT_RW_FUNC)(BL_PIXEL* psPixel, BL_PTR puFBAddr);
typedef IMG_VOID (*BL_INTERNAL_FORMAT_CONVERT_FUNC)(BL_PIXEL *psPixelIn, BL_PIXEL *psPixelOut);
typedef IMG_VOID (*BL_INTERNAL_FORMAT_BLEND_FUNC) (BL_PIXEL*		psPxOut,
													const BL_PIXEL*		apsPixels,
													const IMG_DOUBLE*	auiWeights,
													const IMG_UINT		uiPixelCount);

typedef struct _BL_EXTERNAL_PIXEL_TYPE_
{
	IMG_UINT32 ui32BytesPerPixel;
	BL_INTERNAL_PX_FMT eInternalFormat;
	BL_INTERNAL_FORMAT_RW_FUNC pfnPxRead;
	BL_INTERNAL_FORMAT_RW_FUNC pfnPxWrite;
} BL_EXTERNAL_PIXEL_TYPE;

typedef struct _BL_INTERNAL_PIXEL_TYPE_
{
	IMG_UINT						uiBitsPerChannel[4];
	IMG_UINT						uiBytesPerPixel;
	BL_INTERNAL_FORMAT_BLEND_FUNC	pfnBlendFunction;
	BL_INTERNAL_FORMAT_RW_FUNC		pfnPxReadRAW;
	BL_INTERNAL_FORMAT_RW_FUNC		pfnPxWriteRAW;
} BL_INTERNAL_PIXEL_TYPE;

extern const BL_EXTERNAL_PIXEL_TYPE gas_BLExternalPixelTable[];
extern const BL_INTERNAL_FORMAT_CONVERT_FUNC gafn_BLInternalPixelFormatConversionTable[BL_INTERNAL_PX_FMT_RAW8][BL_INTERNAL_PX_FMT_RAW8];
extern const BL_INTERNAL_PIXEL_TYPE gas_BLInternalPixelTable[BL_INTERNAL_PX_FMT_RAW128 + 1];

/******************************************************************************
 End of file (blitlib_pixel.h)
 ******************************************************************************/
