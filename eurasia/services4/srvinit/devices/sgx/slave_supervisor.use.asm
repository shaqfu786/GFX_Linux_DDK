/*****************************************************************************
 Name		: slave_supervisor.use.asm

 Title		: USE program for handling supervisor tasks

 Author 	: Imagination Technologies

 Created  	: 08/11/2010

 Copyright	: 2010 by Imagination Technologies Limited. All rights reserved.
			  No part of this software, either material or conceptual
			  may be copied or distributed, transmitted, transcribed,
			  stored in a retrieval system or translated into any
			  human or computer language in any form by any means,
			  electronic, mechanical, manual or other-wise, or
			  disclosed to third parties without the express written
			  permission of Imagination Technologies Limited, HomePark
			  Industrial Estate, King's Langley, Hertfordshire,
			  WD4 8LZ, U.K.

 Description : USE program for handling supervisor tasks

 Program Type: USE assembly language

 Version 	: $Revision: 1.1 $

 Modifications	:

 $Log: slave_supervisor.use.asm $
 
 *****************************************************************************/

/*****************************************************************************
 Temporary register usage in supervisor code
 -------------------------------------------

 6 extra temps are required, r24-r29, for the supervisor code:
 	- 4 for temporary usage in functions
	- 1 for pclink copy for calls from utils
	- 1 for pclink + predicate state restore from sprv

*****************************************************************************/

#include "usedefs.h"

#if defined(SGX_FEATURE_MK_SUPERVISOR) && defined(SGX_FEATURE_MP)
.export SlaveSPRVMain;
.export SSPRV_Exit;
/* Align to the largest cache line size */
.modulealign 32;

/*****************************************************************************
 SlaveSPRVMain routine	
 	- Decides which supervisor routine should be run.

 inputs:
 	r0 - Supervisor task type
	r1 - Supervisor arg0 
	r2 - Supervisor arg1
 temps:
 	
*****************************************************************************/
SlaveSPRVMain:
{
	/* Which supervisor function do we need to call? */
#if defined(FIX_HW_BRN_31620)
	/* Do we need to do the workaround for BRN31620 */
	xor.testz	p0, r0, #SGX_UKERNEL_SPRV_BRN_31620;
	p0	brl	SSPRV_BRN31620Inval;
#endif
	
	.align 2;
SSPRV_Exit: /* to be patched */
	phas.end	#0, #0, pixel, none, perinstance;
}

#if defined(FIX_HW_BRN_31620)
/*****************************************************************************
 SPRV_BRN31620Inval routine	
 	- This routine invalidates the DC cache by causing controlled misses

 inputs:
 	
 temps:
 	r25, r26, r27, p1
*****************************************************************************/
SSPRV_BRN31620Inval:
{
	/* Fetch the invalidate mask from memory) */
	MK_MEM_ACCESS_BPCACHE(ldad)	r5, [SLAVE_PA(sTA3DCtl), #DOFFSET(SGXMK_TA3D_CTL.ui32PDInvalMask[0])], drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r6, [SLAVE_PA(sTA3DCtl), #DOFFSET(SGXMK_TA3D_CTL.ui32PDInvalMask[1])], drc0;
	wdf		drc0;
	
	/* Refresh the cache lines indicated in the mask */
	REFRESH_DC_CACHE(p0, r1, r2, r3, r4, #BRN31620_START_ADDR_LOW, r5, SSPRV_BRN31620Inval0);
	REFRESH_DC_CACHE(p0, r1, r2, r3, r4, #BRN31620_START_ADDR_HIGH, r6, SSPRV_BRN31620Inval1);
	/* signal that we have completed the PD cache refresh */
	mov		r1, g45;
	WRITECORECONFIG(r1, #(EUR_CR_USE_GLOBCOM2 >> 2), #1, r2);
		
	/* return to SlaveSPRVMain */
	lapc;
}
#endif
#else
	nop.end;
#endif /* #if defined(SGX_FEATURE_MK_SUPERVISOR) */

