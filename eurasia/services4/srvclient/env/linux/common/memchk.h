/******************************************************************************
* Name         : memchk.h
*
* Copyright    : 2009 by Imagination Technologies Limited.
*                All rights reserved.  No part of this software, either
*                material or conceptual may be copied or distributed,
*                transmitted, transcribed, stored in a retrieval system
*                or translated into any human or computer language in any
*                form by any means, electronic, mechanical, manual or
*                other-wise, or disclosed to third parties without the
*                express written permission of Imagination Technologies
*                Limited, Unit 8, HomePark Industrial Estate,
*                King's Langley, Hertfordshire, WD4 8LZ, U.K.
*
* Platform     : Linux
*
* Modifications:-
* $Log: memchk.h $
******************************************************************************/

#include "img_defs.h"

#ifndef __MEMCHK_H__
#define __MEMCHK_H__

#if defined(DEBUG)
IMG_VOID OutputMemoryStats(IMG_VOID);
#else
static INLINE IMG_VOID OutputMemoryStats(IMG_VOID) {}
#endif

#endif /* __MEMCHK_H__ */
