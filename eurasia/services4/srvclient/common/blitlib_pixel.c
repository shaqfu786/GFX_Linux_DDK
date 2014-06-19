/*!****************************************************************************
@File           blitlib_pixel.c

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
$Log: blitlib_pixel.c $
 ******************************************************************************/

#include "blitlib_pixel.h"
#include "pvr_debug.h"

/*TODO re-check the endianness of 16 bit pixels...*/

/*=================
 * COLOUR BLENDING
 *=================*/

/**
 * The weights must be normalized to sum up to 1.
 */
#ifdef INLINE_IS_PRAGMA
#pragma inline(_bl_blend_32bit_pixels_float_weights)
#endif
static INLINE IMG_VOID _bl_blend_32bit_ARGB_pixels(BL_PIXEL			*psPixelOut,
												   const IMG_UINT	auiComponentsWidth[4],
												   const BL_PIXEL	psPixelsIn[],
												   const IMG_DOUBLE	auiWeights[],
												   const IMG_UINT	uiNumberPixels)
{
	IMG_UINT uiPixelIndex;
	IMG_UINT uiComponent;
	IMG_UINT uiCurrentOffset;

	psPixelOut->sPixelRAW32.ui32Payload = 0;

	uiCurrentOffset = 0;
	for (uiComponent = 0; uiComponent < 4; uiComponent++)
	{
		IMG_UINT32 uiMask = (1UL << auiComponentsWidth[uiComponent]) - 1;
		IMG_DOUBLE uiComponentSum = 0;
		for (uiPixelIndex = 0; uiPixelIndex < uiNumberPixels; uiPixelIndex++)
		{
			uiComponentSum += (IMG_DOUBLE)((psPixelsIn[uiPixelIndex].sPixelRAW32.ui32Payload >> uiCurrentOffset) & uiMask) * auiWeights[uiPixelIndex];
		}
		psPixelOut->sPixelRAW32.ui32Payload |= (IMG_UINT32)(uiComponentSum) << uiCurrentOffset;
		uiCurrentOffset += auiComponentsWidth[uiComponent];
	}
}

static IMG_VOID _bl_blend_argb8888_pixels(BL_PIXEL			*psPixelOut,
										  const BL_PIXEL	*psPixelsIn,
										  const IMG_DOUBLE	adWeights[],
										  const IMG_UINT	uiPixelCount)
{
	_bl_blend_32bit_ARGB_pixels(psPixelOut,
						 gas_BLInternalPixelTable[BL_INTERNAL_PX_FMT_A2RGB10].uiBitsPerChannel,
						 psPixelsIn,
						 adWeights,
						 uiPixelCount);
}

static IMG_VOID _bl_blend_a2rgb10_pixels(BL_PIXEL			*psPixelOut,
										 const BL_PIXEL		*psPixelsIn,
										 const IMG_DOUBLE	adWeights[],
										 const IMG_UINT	uiPixelCount)
{
	_bl_blend_32bit_ARGB_pixels(psPixelOut,
						 gas_BLInternalPixelTable[BL_INTERNAL_PX_FMT_A2RGB10].uiBitsPerChannel,
						 psPixelsIn,
						 adWeights,
						 uiPixelCount);
}

static IMG_VOID _bl_blend_argb16_pixels(BL_PIXEL			*psPixelOut,
										const BL_PIXEL		*psPixelsIn,
										const IMG_DOUBLE	adWeights[],
										const IMG_UINT		uiPixelCount)
{
	IMG_UINT uiComponent;
	for (uiComponent = 0; uiComponent < 4; uiComponent++)
	{
		IMG_UINT uiPixelIndex;
		IMG_DOUBLE dTmp = 0;
		for (uiPixelIndex = 0; uiPixelIndex < uiPixelCount; uiPixelIndex++)
		{
			 dTmp += (adWeights[uiPixelIndex]*(IMG_DOUBLE)psPixelsIn[uiPixelIndex].sPixelARGB16.aui16Values[uiComponent]);
		}
		psPixelOut->sPixelARGB16.aui16Values[uiComponent] = (IMG_UINT16) dTmp;
	}
}

/*========================
 * RAW READERS AND WRITERS
 *========================*/
/* PRQA S 3203++ */ /* QAC does not understand writing to a union pointer */
static IMG_VOID _bl_write_raw8(BL_PIXEL *psPixel,
								BL_PTR puBFAddr)
{
	*puBFAddr.by = psPixel->sPixelRAW8.ui8Payload;
}
static IMG_VOID _bl_read_raw8(BL_PIXEL *psPixel,
							   BL_PTR puBFAddr)
{
	psPixel->sPixelRAW8.ui8Payload = *puBFAddr.by;
}

static IMG_VOID _bl_write_raw16(BL_PIXEL *psPixel,
								BL_PTR puBFAddr)
{
	*puBFAddr.w = psPixel->sPixelRAW16.ui16Payload;
}
static IMG_VOID _bl_read_raw16(BL_PIXEL *psPixel,
							   BL_PTR puBFAddr)
{
	psPixel->sPixelRAW16.ui16Payload = *puBFAddr.w;
}

static IMG_VOID _bl_write_raw24(BL_PIXEL *psPixel,
								BL_PTR puBFAddr)
{
	puBFAddr.by[0] = psPixel->sPixelRAW24.aui8Payload[0];
	puBFAddr.by[1] = psPixel->sPixelRAW24.aui8Payload[1];
	puBFAddr.by[2] = psPixel->sPixelRAW24.aui8Payload[2];
}
static IMG_VOID _bl_read_raw24(BL_PIXEL *psPixel,
							   BL_PTR puBFAddr)
{
	psPixel->sPixelRAW24.aui8Payload[0] = puBFAddr.by[0];
	psPixel->sPixelRAW24.aui8Payload[1] = puBFAddr.by[1];
	psPixel->sPixelRAW24.aui8Payload[2] = puBFAddr.by[2];
}

static IMG_VOID _bl_write_raw32(BL_PIXEL *psPixel,
								BL_PTR puBFAddr)
{
	*puBFAddr.dw = psPixel->sPixelRAW32.ui32Payload;
}
static IMG_VOID _bl_read_raw32(BL_PIXEL *psPixel,
							   BL_PTR puBFAddr)
{
	psPixel->sPixelRAW32.ui32Payload = *puBFAddr.dw;
}

static IMG_VOID _bl_write_raw64(BL_PIXEL *psPixel,
								BL_PTR puBFAddr)
{
	puBFAddr.dw[0] = psPixel->sPixelRAW64.aui32Payload[0];
	puBFAddr.dw[1] = psPixel->sPixelRAW64.aui32Payload[1];
}
static IMG_VOID _bl_read_raw64(BL_PIXEL *psPixel,
							   BL_PTR puBFAddr)
{
	psPixel->sPixelRAW64.aui32Payload[0] = puBFAddr.dw[0];
	psPixel->sPixelRAW64.aui32Payload[1] = puBFAddr.dw[1];
}

static IMG_VOID _bl_write_raw128(BL_PIXEL *psPixel,
								 BL_PTR puBFAddr)
{
	puBFAddr.dw[0] = psPixel->sPixelRAW128.aui32Payload[0];
	puBFAddr.dw[1] = psPixel->sPixelRAW128.aui32Payload[1];
	puBFAddr.dw[2] = psPixel->sPixelRAW128.aui32Payload[2];
	puBFAddr.dw[3] = psPixel->sPixelRAW128.aui32Payload[3];
}
static IMG_VOID _bl_read_raw128(BL_PIXEL *psPixel,
								BL_PTR puBFAddr)
{
	psPixel->sPixelRAW128.aui32Payload[0] = puBFAddr.dw[0];
	psPixel->sPixelRAW128.aui32Payload[1] = puBFAddr.dw[1];
	psPixel->sPixelRAW128.aui32Payload[2] = puBFAddr.dw[2];
	psPixel->sPixelRAW128.aui32Payload[3] = puBFAddr.dw[3];
}

/*
 * Table with the sizes and operations for the internal types of the pipe.
 * - ARGB bits
 * - Byte size
 * - blend function
 * - raw read function
 * - raw write function
 */
const IMG_INTERNAL BL_INTERNAL_PIXEL_TYPE gas_BLInternalPixelTable[BL_INTERNAL_PX_FMT_RAW128 + 1] =
{
	/* BL_INTERN_PX_FMT_ARGB8888 */
	{{8,8,8,8}, 4, &_bl_blend_argb8888_pixels, IMG_NULL, IMG_NULL},

	/* BL_INTERN_PX_FMT_A2RGB10 */
	{{2,10,10,10}, 4, &_bl_blend_a2rgb10_pixels, IMG_NULL, IMG_NULL},

	/* BL_INTERN_PX_FMT_ARGB16 */
	{{16,16,16,16}, 8, &_bl_blend_argb16_pixels, IMG_NULL, IMG_NULL},

	/* BL_INTERN_PX_FMT_RAW8 */
	{{0,0,0,0}, 1, IMG_NULL, &_bl_read_raw8, &_bl_write_raw8},

	/* BL_INTERN_PX_FMT_RAW16 */
	{{0,0,0,0}, 2, IMG_NULL, &_bl_read_raw16, &_bl_write_raw16},

	/* BL_INTERN_PX_FMT_RAW24 */
	{{0,0,0,0}, 3, IMG_NULL, &_bl_read_raw24, &_bl_write_raw24},

	/* BL_INTERN_PX_FMT_RAW32 */
	{{0,0,0,0}, 4, IMG_NULL, &_bl_read_raw32, &_bl_write_raw32},

	/* BL_INTERN_PX_FMT_RAW64 */
	{{0,0,0,0}, 8, IMG_NULL, &_bl_read_raw64, &_bl_write_raw64},

	/* BL_INTERN_PX_FMT_RAW128 */
	{{0,0,0,0}, 16, IMG_NULL, &_bl_read_raw128, &_bl_write_raw128}
};

/*=========================================
 * FORMAT CONVERSION BETWEEN INTERNAL TYPES
 *=========================================*/

static IMG_VOID _bl_ARGB8_A2RGB10(BL_PIXEL *psPxIn, BL_PIXEL *psPxOut)
{
	IMG_UINT32 ui32Tmp = psPxIn->sPixelARGB8888.ui32Value;
	psPxOut->sPixelA2RGB10.ui32Value = (ui32Tmp & 0xc0000000)		 |/*alpha*/
		(((ui32Tmp & 0x00ff0000) | ((ui32Tmp & 0x00c00000)>>4)) << 6)|/*red*/
		(((ui32Tmp & 0x0000ff00) | ((ui32Tmp & 0x0000c000)>>4)) << 4)|/*green*/
		(((ui32Tmp & 0x000000ff) | ((ui32Tmp & 0x000000c0)>>4)) << 2);/*blue*/
}

static IMG_VOID _bl_A2RGB10_ARGB8(BL_PIXEL *psPxIn, BL_PIXEL *psPxOut)
{
	IMG_UINT32 ui32Tmp = psPxIn->sPixelA2RGB10.ui32Value;

	psPxOut->sPixelARGB8888.ui32Value = (((ui32Tmp >> 6) & 0xff000000) * 0x55)|/*alpha*/
												 ((ui32Tmp & 0x3fc00000) >> 6)|/*red*/
												 ((ui32Tmp & 0x000ff000) >> 4)|/*green*/
												 ((ui32Tmp & 0x000003fc) >> 2);/*blue*/
}

static IMG_VOID _bl_ARGB8_ARGB16(BL_PIXEL *psPxIn, BL_PIXEL *psPxOut)
{
	IMG_UINT32 ui32Tmp = psPxIn->sPixelARGB8888.ui32Value;
	IMG_UINT uiComponent;
	for (uiComponent = 0; uiComponent < 4; uiComponent++)
	{
		IMG_UINT uiShift = 24 - uiComponent*8;
		IMG_UINT16 ui16v = (IMG_UINT16) ((ui32Tmp >> uiShift) & 0xff);
		psPxOut->sPixelARGB16.aui16Values[uiComponent] = (IMG_UINT16) (ui16v * 0x101u);
	}
}

static IMG_VOID _bl_ARGB16_ARGB8(BL_PIXEL *psPxIn, BL_PIXEL *psPxOut)
{
	IMG_UINT32 ui32Tmp = 0;
	IMG_UINT uiComponent;
	for (uiComponent = 0; uiComponent < 4; uiComponent++)
	{
		IMG_UINT uiShift = 24 - uiComponent*8;
		ui32Tmp |= ((IMG_UINT32)psPxIn->sPixelARGB16.aui16Values[uiComponent] >> 8) << uiShift;
	}
	psPxOut->sPixelARGB8888.ui32Value = ui32Tmp;
}

static IMG_VOID _bl_A2RGB10_ARGB16(BL_PIXEL *psPxIn, BL_PIXEL *psPxOut)
{
	IMG_UINT32 ui32Tmp = psPxIn->sPixelA2RGB10.ui32Value;
	IMG_UINT uiComponent;

	psPxOut->sPixelARGB16.aui16Values[0] = (IMG_UINT16) (((ui32Tmp >> 30) & 0x3) * 0x5555); /*Alpha*/

	for (uiComponent = 1; uiComponent < 4; uiComponent++)
	{
		IMG_UINT uiShift = 10*(3-uiComponent);
		IMG_UINT16 ui16v = (IMG_UINT16) ((ui32Tmp >> uiShift) & 0x3ff);
		psPxOut->sPixelARGB16.aui16Values[uiComponent] = (IMG_UINT16) ((ui16v << 6) | (ui16v >> 4));
	}
}

static IMG_VOID _bl_ARGB16_A2RGB10(BL_PIXEL *psPxIn, BL_PIXEL *psPxOut)
{
	IMG_UINT32 ui32Tmp = 0;
	IMG_UINT uiComponent;

	ui32Tmp |= (psPxIn->sPixelARGB16.aui16Values[0] & 0xc0000000) << 8;

	for (uiComponent = 1; uiComponent < 4; uiComponent++)
	{
		IMG_UINT uiShift = 10*(3-uiComponent);
		ui32Tmp |= ((IMG_UINT32)psPxIn->sPixelARGB16.aui16Values[uiComponent] >> 6) << uiShift;
	}
	psPxOut->sPixelA2RGB10.ui32Value = ui32Tmp;
}

/*
 * Table to convert from one internal blitlib pixel format to another.
 *
 * Only for _non_ RAW pixel formats (obviously).
 *
 * Indexed by [SrcFmt:BL_INTERNAL_PX_FMT][DstFmt:BL_INTERNAL_PX_FMT]
 */
const IMG_INTERNAL BL_INTERNAL_FORMAT_CONVERT_FUNC
	gafn_BLInternalPixelFormatConversionTable[BL_INTERNAL_PX_FMT_RAW8][BL_INTERNAL_PX_FMT_RAW8] = {
/*TO\\FROM*//*ARGB8888*/			/*A2RGB10*/				/*ARGB16*/
/*ARGB8888*/{IMG_NULL,				&_bl_ARGB8_A2RGB10,		&_bl_ARGB8_ARGB16	},
/*A2RGB10*/	{&_bl_A2RGB10_ARGB8,	IMG_NULL,				&_bl_A2RGB10_ARGB16	},
/*ARGB16*/	{&_bl_ARGB16_ARGB8,		&_bl_ARGB16_A2RGB10,	IMG_NULL,			}
};


/* ===================================
 * EXTERNAL FORMAT READERS AND WRITERS
 * =================================== */

static IMG_VOID _bl_pixel_rgb565_read(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	IMG_UINT16 ui16Tmp;
	IMG_UINT16 ui16Component;
	BL_PIXEL_ARGB16* psSelf = &psPixel->sPixelARGB16;

	ui16Tmp = *puFBAddr.w;

	psSelf->aui16Values[0] = 0xffff; /*alpha*/

	ui16Component = (IMG_UINT16) ((ui16Tmp >> 11) & 0x1f);
	/*roughly equivalent to multiply by 2114.0625 >~ (2^16-1)/(2^5-1) = 2114.0322580645161*/
	psSelf->aui16Values[1] = (IMG_UINT16) ((ui16Component * 2114) | (ui16Component >> 4)); /*red*/

	ui16Component = (IMG_UINT16) ((ui16Tmp >> 5) & 0x3f);
	/*roughly equivalent to multiply by 1040.25 >~ (2^16-1)/(2^6-1) = 1040.2380952380952*/
	psSelf->aui16Values[2] = (IMG_UINT16) ((ui16Component * 1040) | (ui16Component >> 2));  /*green*/
	
	ui16Component = (IMG_UINT16)(ui16Tmp & 0x1f);
	/*roughly equivalent to multiply by 2114.0625 >~ (2^16-1)/(2^5-1) = 2114.0322580645161*/
	psSelf->aui16Values[3] = (IMG_UINT16)((ui16Component * 2114) | (ui16Component >> 4)); /*blue*/
}


static IMG_VOID _bl_pixel_rgb565_write(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	IMG_UINT16 ui16Tmp;
	BL_PIXEL_ARGB16* psSelf = &psPixel->sPixelARGB16;

	ui16Tmp = (IMG_UINT16)(
					( psSelf->aui16Values[1] 		& 0xf800)	| /*red*/
					((psSelf->aui16Values[2] >>  5) & 0x07e0)	| /*green*/
					((psSelf->aui16Values[3] >> 11) & 0x001f) 	  /*blue*/
				);

	puFBAddr.by[0] = (IMG_BYTE) (ui16Tmp>>0);
	puFBAddr.by[1] = (IMG_BYTE) (ui16Tmp>>8);
}

static IMG_VOID _bl_pixel_rgb888_read(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16* psSelf = &psPixel->sPixelARGB16;
	IMG_UINT uiComponentIndex;

	psSelf->aui16Values[0] = 0xffff; /*full alpha*/
	for (uiComponentIndex = 1; uiComponentIndex < 4; uiComponentIndex++)
	{
		psSelf->aui16Values[uiComponentIndex] = (IMG_UINT16) (0x101u * puFBAddr.by[3-uiComponentIndex]);
	}
}

static IMG_VOID _bl_pixel_rgb888_write(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16* psSelf = &psPixel->sPixelARGB16;
	IMG_UINT uiComponentIndex;

	for (uiComponentIndex = 1; uiComponentIndex < 4; uiComponentIndex++)
	{
		puFBAddr.by[3-uiComponentIndex] = (IMG_BYTE)(psSelf->aui16Values[uiComponentIndex] >> 8);
	}
}

static IMG_VOID _bl_pixel_argb8888_read(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16* psSelf = &psPixel->sPixelARGB16;
	IMG_UINT uiComponentIndex;

	for (uiComponentIndex = 0; uiComponentIndex < 4; uiComponentIndex++)
	{
		psSelf->aui16Values[uiComponentIndex] = (IMG_UINT16) (0x101u * puFBAddr.by[3-uiComponentIndex]);
	}
}

static IMG_VOID _bl_pixel_argb8888_write(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16* psSelf = &psPixel->sPixelARGB16;
	IMG_UINT uiComponentIndex;

	for (uiComponentIndex = 0; uiComponentIndex < 4; uiComponentIndex++)
	{
		puFBAddr.by[3-uiComponentIndex] = (IMG_BYTE)(psSelf->aui16Values[uiComponentIndex] >> 8);
	}
}

static IMG_VOID _bl_pixel_abgr8888_read(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16* psSelf = &psPixel->sPixelARGB16;
	IMG_UINT uiComponentIndex;

	for (uiComponentIndex = 0; uiComponentIndex < 4; uiComponentIndex++)
	{
		/* PRQA S 3372 1 */ /* relies on wraparound past zero */
		psSelf->aui16Values[uiComponentIndex] = (IMG_UINT16) (0x101u * puFBAddr.by[(uiComponentIndex - 1) & 3]);
	}
}

static IMG_VOID _bl_pixel_abgr8888_write(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16* psSelf = &psPixel->sPixelARGB16;
	IMG_UINT uiComponentIndex;

	for (uiComponentIndex = 0; uiComponentIndex < 4; uiComponentIndex++)
	{
		/* PRQA S 3372 1 */ /* relies on wraparound past zero */
		puFBAddr.by[(uiComponentIndex - 1) & 3] = (IMG_BYTE)(psSelf->aui16Values[uiComponentIndex] >> 8);
	}
}

static IMG_VOID _bl_pixel_bgra8888_read(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16* psSelf = &psPixel->sPixelARGB16;
	IMG_UINT uiComponentIndex;

	for (uiComponentIndex = 0; uiComponentIndex < 4; uiComponentIndex++)
	{
		psSelf->aui16Values[uiComponentIndex] = (IMG_UINT16) (0x101u * puFBAddr.by[uiComponentIndex]);
	}
}

static IMG_VOID _bl_pixel_bgra8888_write(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16* psSelf = &psPixel->sPixelARGB16;
	IMG_UINT uiComponentIndex;

	for (uiComponentIndex = 0; uiComponentIndex < 4; uiComponentIndex++)
	{
		puFBAddr.by[uiComponentIndex] = (IMG_BYTE)(psSelf->aui16Values[uiComponentIndex] >> 8);
	}
}

static IMG_VOID _bl_pixel_a2rgb10_read(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	IMG_UINT uiComponentIndex;

	for (uiComponentIndex = 0; uiComponentIndex < 4; uiComponentIndex++)
	{
		psPixel->sPixelA2RGB10.ui32Value |= (IMG_UINT32)puFBAddr.by[uiComponentIndex] << (8*(3-uiComponentIndex));
	}

	_bl_A2RGB10_ARGB16(psPixel, psPixel);
}
static IMG_VOID _bl_pixel_a2rgb10_write(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_A2RGB10 sTmp;
	IMG_UINT uiComponentIndex;

	_bl_ARGB16_A2RGB10(psPixel, (BL_PIXEL*) &sTmp);
	for (uiComponentIndex = 0; uiComponentIndex < 4; uiComponentIndex++)
	{
		puFBAddr.by[uiComponentIndex] = (IMG_BYTE)(sTmp.ui32Value >> (8*(3-uiComponentIndex)));
	}
}

static IMG_VOID _bl_pixel_bgr888_read(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16* psSelf = &psPixel->sPixelARGB16;
	IMG_UINT uiComponentIndex;

	psSelf->aui16Values[0] = 0xffff; /*full alpha*/
	for (uiComponentIndex = 1; uiComponentIndex < 4; uiComponentIndex++)
	{
		psSelf->aui16Values[uiComponentIndex] = (IMG_UINT16)(0x101u * puFBAddr.by[uiComponentIndex-1]);
	}
}
static IMG_VOID _bl_pixel_bgr888_write(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16* psSelf = &psPixel->sPixelARGB16;
	IMG_UINT uiComponentIndex;

	for (uiComponentIndex = 1; uiComponentIndex < 4; uiComponentIndex++)
	{
		puFBAddr.by[uiComponentIndex-1] = (IMG_BYTE)((psSelf->aui16Values[uiComponentIndex])>>8u);
	}
}

static IMG_VOID _bl_pixel_argb1555_read(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	IMG_UINT uiComponentIndex;

	BL_PIXEL_ARGB16 *psSelf = &psPixel->sPixelARGB16;
	IMG_UINT16 ui16tmp = (IMG_UINT16) ((puFBAddr.by[1]<<8) | puFBAddr.by[0]);

	/* same as (value & 0x8000 ? 0xffff : 0x0000) but without any conditional instruction */
	psSelf->aui16Values[0] = (IMG_UINT16) (((ui16tmp >> 15) & 1) * 0xffff); /*alpha*/
	for (uiComponentIndex = 1; uiComponentIndex < 4; uiComponentIndex++) 	/*rgb*/
	{
		IMG_UINT uiShift = 5*(3-uiComponentIndex);
		IMG_UINT16 ui16v = (IMG_UINT16) ((ui16tmp >> uiShift) & 0x1f);
		/*roughly equivalent to multiply by 2114.0625 >~ (2^16-1)/(2^5-1) = 2114.0322580645161*/
		psSelf->aui16Values[uiComponentIndex] = (IMG_UINT16)((2114 * ui16v) | (ui16v >> 4));
	}
}

static IMG_VOID _bl_pixel_argb1555_write(BL_PIXEL* psSelf, BL_PTR puFBAddr)
{
	IMG_UINT16 ui16tmp;

	ui16tmp = (IMG_UINT16)((psSelf->sPixelARGB16.aui16Values[0]  & 0x8000)	|/*alpha*/
					((psSelf->sPixelARGB16.aui16Values[1] >>  1) & 0x7c00)	|/*r*/
					((psSelf->sPixelARGB16.aui16Values[2] >>  6) & 0x03e0)	|/*g*/
					((psSelf->sPixelARGB16.aui16Values[3] >> 11) & 0x001f));  /*b*/

	puFBAddr.by[0] = (IMG_BYTE) (ui16tmp);
	puFBAddr.by[1] = (IMG_BYTE) (ui16tmp >> 8);
}

static IMG_VOID _bl_pixel_argb4444_read(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	IMG_UINT uiComponentIndex;

	BL_PIXEL_ARGB16 *psSelf = &psPixel->sPixelARGB16;
	IMG_UINT16 ui16tmp = (IMG_UINT16) (puFBAddr.by[0] | (puFBAddr.by[1] << 8));

	for (uiComponentIndex = 0; uiComponentIndex < 4; uiComponentIndex++) /*rgb*/
	{
		IMG_UINT uiShift = 4*(3-uiComponentIndex);
		IMG_UINT16 ui16v = (IMG_UINT16) ((ui16tmp >> uiShift) & 0xf);
		psSelf->aui16Values[uiComponentIndex] = (IMG_UINT16)(0x1111 * ui16v);
	}
}

static IMG_VOID _bl_pixel_argb4444_write(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	IMG_UINT16 ui16tmp = 0;
	BL_PIXEL_ARGB16 *psSelf = &psPixel->sPixelARGB16;
	IMG_UINT uiComponentIndex;

	for (uiComponentIndex = 0; uiComponentIndex < 4; uiComponentIndex++)
	{
		IMG_UINT uiShift = 4*(3-uiComponentIndex);
		ui16tmp |= (IMG_UINT16) (((psSelf->aui16Values[uiComponentIndex] >> 12) & 0xf) << uiShift);
	}

	puFBAddr.by[0] = (IMG_BYTE) ui16tmp;
	puFBAddr.by[1] = (IMG_BYTE) (ui16tmp >> 8);
}

static IMG_VOID _bl_pixel_abgr16_read(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16 *psSelf = &psPixel->sPixelARGB16;

	psSelf->aui16Values[0] = puFBAddr.w[3]; /*Alpha*/
	psSelf->aui16Values[1] = puFBAddr.w[0]; /*Red*/
	psSelf->aui16Values[2] = puFBAddr.w[1]; /*Green*/
	psSelf->aui16Values[3] = puFBAddr.w[2]; /*Blue*/
}

static IMG_VOID _bl_pixel_abgr16_write(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16 *psSelf = &psPixel->sPixelARGB16;

	puFBAddr.w[3] = psSelf->aui16Values[0]; /*Alpha*/
	puFBAddr.w[0] = psSelf->aui16Values[1]; /*Red*/
	puFBAddr.w[1] = psSelf->aui16Values[2]; /*Green*/
	puFBAddr.w[2] = psSelf->aui16Values[3]; /*Blue*/
}

static IMG_VOID _bl_pixel_abgr32_read(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16 *psSelf = &psPixel->sPixelARGB16;

	psSelf->aui16Values[0] = (IMG_UINT16)(puFBAddr.dw[3] >> 16); /*Alpha*/
	psSelf->aui16Values[1] = (IMG_UINT16)(puFBAddr.dw[0] >> 16); /*Red*/
	psSelf->aui16Values[2] = (IMG_UINT16)(puFBAddr.dw[1] >> 16); /*Green*/
	psSelf->aui16Values[3] = (IMG_UINT16)(puFBAddr.dw[2] >> 16); /*Blue*/
}

static IMG_VOID _bl_pixel_abgr32_write(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16 *psSelf = &psPixel->sPixelARGB16;

	puFBAddr.dw[3] = psSelf->aui16Values[0] * 0x10001u; /*Alpha*/
	puFBAddr.dw[0] = psSelf->aui16Values[1] * 0x10001u; /*Red*/
	puFBAddr.dw[1] = psSelf->aui16Values[2] * 0x10001u; /*Green*/
	puFBAddr.dw[2] = psSelf->aui16Values[3] * 0x10001u; /*Blue*/
}

static IMG_VOID _bl_pixel_bgr32_read(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16 *psSelf = &psPixel->sPixelARGB16;

	psSelf->aui16Values[0] = 0; 
	psSelf->aui16Values[1] = (IMG_UINT16)(puFBAddr.dw[0] >> 16); /*Red*/
	psSelf->aui16Values[2] = (IMG_UINT16)(puFBAddr.dw[1] >> 16); /*Green*/
	psSelf->aui16Values[3] = (IMG_UINT16)(puFBAddr.dw[2] >> 16); /*Blue*/
}

static IMG_VOID _bl_pixel_bgr32_write(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16 *psSelf = &psPixel->sPixelARGB16;

	puFBAddr.dw[0] = psSelf->aui16Values[1] * 0x10001u; /*Red*/
	puFBAddr.dw[1] = psSelf->aui16Values[2] * 0x10001u; /*Green*/
	puFBAddr.dw[2] = psSelf->aui16Values[3] * 0x10001u; /*Blue*/
}

static IMG_VOID _bl_pixel_gr32_read(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16 *psSelf = &psPixel->sPixelARGB16;

	psSelf->aui16Values[0] = 0; 
	psSelf->aui16Values[1] = (IMG_UINT16)(puFBAddr.dw[0] >> 16); /*Red*/
	psSelf->aui16Values[2] = (IMG_UINT16)(puFBAddr.dw[1] >> 16); /*Green*/
	psSelf->aui16Values[3] = 0; 
}

static IMG_VOID _bl_pixel_gr32_write(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16 *psSelf = &psPixel->sPixelARGB16;

	puFBAddr.dw[0] = psSelf->aui16Values[1] * 0x10001u; /*Red*/
	puFBAddr.dw[1] = psSelf->aui16Values[2] * 0x10001u; /*Green*/
}

static IMG_VOID _bl_pixel_r32_read(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16 *psSelf = &psPixel->sPixelARGB16;

	psSelf->aui16Values[0] = 0; 
	psSelf->aui16Values[1] = (IMG_UINT16)(puFBAddr.dw[0] >> 16); /*Red*/
	psSelf->aui16Values[2] = 0; 
	psSelf->aui16Values[3] = 0; 
}

static IMG_VOID _bl_pixel_r32_write(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16 *psSelf = &psPixel->sPixelARGB16;

	puFBAddr.dw[0] = psSelf->aui16Values[1] * 0x10001u; /*Red*/
}

static IMG_VOID _bl_pixel_gr88_read(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16 *psSelf = &psPixel->sPixelARGB16;

	psSelf->aui16Values[0] = 0; 
	psSelf->aui16Values[1] = puFBAddr.by[0] * 0x101u; /*Red*/
	psSelf->aui16Values[2] = puFBAddr.by[1] * 0x101u; /*Green*/
	psSelf->aui16Values[3] = 0; 
}

static IMG_VOID _bl_pixel_gr88_write(BL_PIXEL* psPixel, BL_PTR puFBAddr)
{
	BL_PIXEL_ARGB16 *psSelf = &psPixel->sPixelARGB16;

	puFBAddr.by[0] = (IMG_BYTE)(psSelf->aui16Values[1] >> 8); /*Red*/
	puFBAddr.by[1] = (IMG_BYTE)(psSelf->aui16Values[2] >> 8); /*Green*/
}

/* PRQA S 3203-- */ /* QAC does not understand writing to a union pointer */

/*
 * This table contains the external format types:
 * - Number of bits per pixel,
 * - Internal used by reader and writer (that could be split and expanded in the future)
 * - Reader function
 * - Writer function
 */
const IMG_INTERNAL BL_EXTERNAL_PIXEL_TYPE gas_BLExternalPixelTable[] = {
							/* PVRSRV_PIXEL_FORMAT_UNKNOWN*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_RGB565*/
	{2, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_rgb565_read, _bl_pixel_rgb565_write},
							/* PVRSRV_PIXEL_FORMAT_RGB555*/
	{2, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_argb1555_read, _bl_pixel_argb1555_write},
							/* PVRSRV_PIXEL_FORMAT_RGB888*/
	{3, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_rgb888_read, _bl_pixel_rgb888_write},
							/* PVRSRV_PIXEL_FORMAT_BGR888*/
	{3, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_bgr888_read, _bl_pixel_bgr888_write},
							/* 5*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* 6*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* 7*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_GREY_SCALE*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* 9*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* 10*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* 11*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* 12*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_PAL12*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_PAL8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_PAL4*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_PAL2*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_PAL1*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_ARGB1555*/
	{2, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_argb1555_read, _bl_pixel_argb1555_write},
							/* PVRSRV_PIXEL_FORMAT_ARGB4444*/
	{2, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_argb4444_read, _bl_pixel_argb4444_write},
							/* PVRSRV_PIXEL_FORMAT_ARGB8888*/
	{4, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_argb8888_read, _bl_pixel_argb8888_write},
							/* PVRSRV_PIXEL_FORMAT_ABGR8888*/
	{4, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_abgr8888_read, _bl_pixel_abgr8888_write},
							/* PVRSRV_PIXEL_FORMAT_YV12*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_I420*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* 24*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_IMC2*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_XRGB8888*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_XBGR8888*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_BGRA8888*/
	{4, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_bgra8888_read, _bl_pixel_bgra8888_write},
							/* PVRSRV_PIXEL_FORMAT_XRGB4444*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_ARGB8332*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A2RGB10*/
	{4, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_a2rgb10_read, _bl_pixel_a2rgb10_write},
							/* PVRSRV_PIXEL_FORMAT_A2BGR10*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_P8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_L8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A8L8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A4L4*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_L16*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_L6V5U5*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_V8U8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_V16U16*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_QWVU8888*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_XLVU8888*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_QWVU16*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_D16*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_D24S8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_D24X8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_ABGR16*/
	{8, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_abgr16_read, _bl_pixel_abgr16_write},
							/* PVRSRV_PIXEL_FORMAT_ABGR16F*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_ABGR32*/
	{16, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_abgr32_read, _bl_pixel_abgr32_write},
							/* PVRSRV_PIXEL_FORMAT_ABGR32F*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_B10GR11*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_GR88*/
	{2, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_gr88_read, _bl_pixel_gr88_write},
							/* PVRSRV_PIXEL_FORMAT_BGR32*/
	{12, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_bgr32_read, _bl_pixel_bgr32_write},
							/* PVRSRV_PIXEL_FORMAT_GR32*/
	{8, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_gr32_read, _bl_pixel_gr32_write},
							/* PVRSRV_PIXEL_FORMAT_E5BGR9*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_RESERVED1*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_RESERVED2*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_RESERVED3*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_RESERVED4*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_RESERVED5*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_R8G8_B8G8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_G8R8_G8B8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_NV11*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_NV12*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_YUY2*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_YUV420*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_YUV444*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_VUY444*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_YUYV*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_YVYU*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_UYVY*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_VYUY*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_FOURCC_ORG_UYVY*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_FOURCC_ORG_YUYV*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_FOURCC_ORG_YVYU*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_FOURCC_ORG_VYUY*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_FOURCC_ORG_AYUV*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A32B32G32R32*/
	{16, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_abgr32_read, _bl_pixel_abgr32_write},
							/* PVRSRV_PIXEL_FORMAT_A32B32G32R32F*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A32B32G32R32_UINT*/
	{16, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_abgr32_read, _bl_pixel_abgr32_write},
							/* PVRSRV_PIXEL_FORMAT_A32B32G32R32_SINT*/
	{16, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_abgr32_read, _bl_pixel_abgr32_write},
							/* PVRSRV_PIXEL_FORMAT_B32G32R32*/
	{12, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_bgr32_read, _bl_pixel_bgr32_write},
							/* PVRSRV_PIXEL_FORMAT_B32G32R32F*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_B32G32R32_UINT*/
	{12, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_bgr32_read, _bl_pixel_bgr32_write},
							/* PVRSRV_PIXEL_FORMAT_B32G32R32_SINT*/
	{12, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_bgr32_read, _bl_pixel_bgr32_write},
							/* PVRSRV_PIXEL_FORMAT_G32R32*/
	{8, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_gr32_read, _bl_pixel_gr32_write},
							/* PVRSRV_PIXEL_FORMAT_G32R32F*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_G32R32_UINT*/
	{8, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_gr32_read, _bl_pixel_gr32_write},
							/* PVRSRV_PIXEL_FORMAT_G32R32_SINT*/
	{8, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_gr32_read, _bl_pixel_gr32_write},
							/* PVRSRV_PIXEL_FORMAT_D32F*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_R32*/
	{4, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_r32_read, _bl_pixel_r32_write},
							/* PVRSRV_PIXEL_FORMAT_R32F*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_R32_UINT*/
	{4, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_r32_read, _bl_pixel_r32_write},
							/* PVRSRV_PIXEL_FORMAT_R32_SINT*/
	{4, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_r32_read, _bl_pixel_r32_write},
							/* PVRSRV_PIXEL_FORMAT_A16B16G16R16*/
	{8, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_abgr16_read, _bl_pixel_abgr16_write},
							/* PVRSRV_PIXEL_FORMAT_A16B16G16R16F*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A16B16G16R16_SINT*/
	{8, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_abgr16_read, _bl_pixel_abgr16_write},
							/* PVRSRV_PIXEL_FORMAT_A16B16G16R16_SNORM*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A16B16G16R16_UINT*/
	{8, BL_INTERNAL_PX_FMT_ARGB16, _bl_pixel_abgr16_read, _bl_pixel_abgr16_write},
							/* PVRSRV_PIXEL_FORMAT_A16B16G16R16_UNORM*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_G16R16*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_G16R16F*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_G16R16_UINT*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_G16R16_UNORM*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_G16R16_SINT*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_G16R16_SNORM*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_R16*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_R16F*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_R16_UINT*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_R16_UNORM*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_R16_SINT*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_R16_SNORM*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_X8R8G8B8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_X8R8G8B8_UNORM*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_X8R8G8B8_UNORM_SRGB*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A8R8G8B8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A8R8G8B8_UNORM*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A8R8G8B8_UNORM_SRGB*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A8B8G8R8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A8B8G8R8_UINT*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A8B8G8R8_UNORM*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A8B8G8R8_UNORM_SRGB*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A8B8G8R8_SINT*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A8B8G8R8_SNORM*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_G8R8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_G8R8_UINT*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_G8R8_UNORM*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_G8R8_SINT*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_G8R8_SNORM*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_R8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_R8_UINT*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_R8_UNORM*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_R8_SINT*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_R8_SNORM*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A2B10G10R10*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A2B10G10R10_UNORM*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A2B10G10R10_UINT*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_B10G11R11*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_B10G11R11F*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_X24G8R32*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_G8R24*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_X8R24*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_E5B9G9R9*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_R1*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							   /* PVRSRV_PIXEL_FORMAT_RESERVED*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							   /* PVRSRV_PIXEL_FORMAT_RESERVED7*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							   /* PVRSRV_PIXEL_FORMAT_RESERVED8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							   /* PVRSRV_PIXEL_FORMAT_RESERVED9*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							   /* PVRSRV_PIXEL_FORMAT_RESERVED10*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							   /* PVRSRV_PIXEL_FORMAT_RESERVED11*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							   /* PVRSRV_PIXEL_FORMAT_RESERVED12*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							   /* PVRSRV_PIXEL_FORMAT_RESERVED13*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							   /* PVRSRV_PIXEL_FORMAT_RESERVED14*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							   /* PVRSRV_PIXEL_FORMAT_RESERVED15*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							   /* PVRSRV_PIXEL_FORMAT_RESERVED16*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							   /* PVRSRV_PIXEL_FORMAT_RESERVED17*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							   /* PVRSRV_PIXEL_FORMAT_RESERVED18*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							   /* PVRSRV_PIXEL_FORMAT_RESERVED19*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							   /* PVRSRV_PIXEL_FORMAT_RESERVED20*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_UBYTE4*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_SHORT4*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_SHORT4N*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_USHORT4N*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_SHORT2N*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_SHORT2*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_USHORT2N*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_UDEC3*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_DEC3N*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_F16_2*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_F16_4*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_L_F16*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_L_F16_REP*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_L_F16_A_F16*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A_F16*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_B16G16R16F*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_L_F32*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A_F32*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_L_F32_A_F32*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_PVRTC2*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_PVRTC4*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_PVRTCII2*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_PVRTCII4*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_PVRTCIII*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_PVRO8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_PVRO88*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_PT1*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_PT2*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_PT4*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_PT8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_PTW*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_PTB*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_MONO8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_MONO16*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_C0_YUYV*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_C0_UYVY*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_C0_YVYU*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_C0_VYUY*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_C1_YUYV*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_C1_UYVY*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_C1_YVYU*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_C1_VYUY*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_C0_YUV420_2P_UV*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_C0_YUV420_2P_VU*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_C0_YUV420_3P*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_C1_YUV420_2P_UV*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_C1_YUV420_2P_VU*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_C1_YUV420_3P*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A2B10G10R10F*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_B8G8R8_SINT*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_PVRF32SIGNMASK*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_ABGR4444*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_ABGR1555*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_BGR565*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_C0_4KYUV420_2P_UV*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_C0_4KYUV420_2P_VU*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_C1_4KYUV420_2P_UV*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_C1_4KYUV420_2P_VU*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_P208*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_A8P8*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
							/* PVRSRV_PIXEL_FORMAT_FORCE_I32*/
	{0, BL_INTERNAL_PX_FMT_UNAVAILABLE, IMG_NULL, IMG_NULL},
};

/******************************************************************************
 End of file (blitlib_pixel.c)
 ******************************************************************************/
