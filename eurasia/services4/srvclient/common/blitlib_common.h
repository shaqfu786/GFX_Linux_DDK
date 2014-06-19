/*!****************************************************************************
@File           blitlib_common.h

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
$Log: blitlib_common.h $
******************************************************************************/

IMG_INTERNAL IMG_UINT32 BLFindNearestLog2(IMG_UINT32 ui32Num);

IMG_INTERNAL IMG_UINT32 BLTwiddleStretch(IMG_UINT32 ui32Num);

#if ! defined(MIN)
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#if ! defined(MAX)
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif


/******************************************************************************
 End of file (blitlib_common.h)
******************************************************************************/
