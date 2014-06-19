/*!****************************************************************************
@File           blitlib_common.c

@Title         	Blit library - common utils 

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
$Log: blitlib_common.c $
******************************************************************************/

#include "img_defs.h"
#include "blitlib.h"
#include "blitlib_common.h"

IMG_INTERNAL IMG_UINT32 BLFindNearestLog2(IMG_UINT32 ui32Num)
{
	IMG_UINT32  ui32Result = 0;

	if (ui32Num)
		ui32Num --;

	if (ui32Num & 0xffff0000)
	{
		ui32Result += 16;
		ui32Num >>= 16;
	}
	if (ui32Num & 0x0000ff00)
	{
		ui32Result += 8;
		ui32Num >>= 8;
	}
	if (ui32Num & 0x000000f0)
	{
		ui32Result += 4;
		ui32Num >>= 4;
	}
	
	while (ui32Num)
	{
		ui32Result ++;
		ui32Num >>= 1;
	}

	return ui32Result;
}

/**
 * Function Name	: BLTwiddleStretch
 * Inputs			: - ui32Num = the number to be stretched.
 *
 * Outputs			: Nothing
 * Returns			: the stretched coordinate.
 * Description		: Stretches the bits of the twiddled coordinates.
 *
 */
IMG_INTERNAL IMG_UINT32 BLTwiddleStretch(IMG_UINT32 ui32Num)
{

	ui32Num = (ui32Num | (ui32Num << 8)) & 0x00ff00ff;
	ui32Num = (ui32Num | (ui32Num << 4)) & 0x0f0f0f0f;
	ui32Num = (ui32Num | (ui32Num << 2)) & 0x33333333;
	ui32Num = (ui32Num | (ui32Num << 1)) & 0x55555555;

	return ui32Num;
}

/**
 * Function Name	: BLInitObject
 * Inputs			: psObject
 * Outputs			: Nothing
 * Returns			: Nothing
 * Description		: Resets the object fields to their default values.
 *
 * Sets the callback functions to IMG_NULL, the blending to IMG_FALSE, the
 * external forma to PVRSRV_PIXEL_FORMAT_UNKNOWN and the upstream objects to
 * IMG_NULL.
 */
IMG_INTERNAL IMG_VOID BLInitObject(BL_OBJECT *psObject)
{
	IMG_UINT i;

	psObject->pfnGetPixel = IMG_NULL;
	psObject->pfnDoCaching = IMG_NULL;
	psObject->bDoesBlending = IMG_FALSE;
	psObject->eExternalPxFmt = PVRSRV_PIXEL_FORMAT_UNKNOWN;
	for (i = 0; i < BL_MAX_SOURCES; i++)
	{
		psObject->apsUpstreamObjects[i] = IMG_NULL;
	}
}

/******************************************************************************
 End of file (blitlib_common.c)
******************************************************************************/
