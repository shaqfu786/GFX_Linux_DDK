/**************************************************************************
 * Name         : nothreads_tls.c
 *
 * Copyright    : 2005,2007 by Imagination Technologies Limited. All rights reserved.
 *              : No part of this software, either material or conceptual
 *              : may be copied or distributed, transmitted, transcribed,
 *              : stored in a retrieval system or translated into any
 *              : human or computer language in any form by any means,
 *              : electronic, mechanical, manual or other-wise, or
 *              : disclosed to third parties without the express written
 *              : permission of Imagination Technologies Limited, Unit 8, HomePark
 *              : Industrial Estate, King's Langley, Hertfordshire,
 *              : WD4 8LZ, U.K.
 *
 * Platform     : ANSI
 *
 * $Log: nothreads_tls.c $
 **************************************************************************/
#if defined(DISABLE_THREADS)


#include "img_types.h"
#include "common_tls.h"

IMG_VOID *TLS_PREFIX(g_pvTLS);


#endif
