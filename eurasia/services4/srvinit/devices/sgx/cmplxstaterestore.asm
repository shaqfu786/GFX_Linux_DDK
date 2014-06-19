/*****************************************************************************
 Name		: cmplxstaterestore.asm

 Title		: Complex Geometry State restore USE program

 Author 	: Christopher Plant

 Created  	: 08/08/2006

 Copyright	: 2006 by Imagination Technologies Limited. All rights reserved.
		  No part of this software, either material or conceptual
		  may be copied or distributed, transmitted, transcribed,
		  stored in a retrieval system or translated into any
		  human or computer language in any form by any means,
		  electronic, mechanical, manual or other-wise, or
		  disclosed to third parties without the express written
		  permission of Imagination Technologies Limited, HomePark
		  Industrial Estate, King's Langley, Hertfordshire,
		  WD4 8LZ, U.K.

 Description 	: USE program for restoring the complex geometry state 
 					information after a context switch

 Program Type	: USE assembly language

 Version 	: $Revision: 1.10 $

 Modifications	:

 $Log: cmplxstaterestore.asm $
*****************************************************************************/

#include "usedefs.h"

	.skipinvon;

#if defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH) && defined(SGX_FEATURE_MTE_STATE_FLUSH) && defined(SGX_FEATURE_COMPLEX_GEOMETRY_REV_2)
	/* Set macro operations to only increment src2 */
	smlsi	#1,#1,#1,#1,#0,#0,#0,#0,#0,#0,#0;

	#if defined(SGX_FEATURE_USE_UNLIMITED_PHASES)
	/* No following phase. */
	phas	#0, #0, pixel, end, parallel;
	#endif

	/* Get the location of the memory structures as passed via PAs... */
	MK_MEM_ACCESS_BPCACHE(ldad)	pa0, [pa0, #DOFFSET(SGXMK_TA3D_CTL.sTARenderContext)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS(ldad) 	pa0, [pa0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sMTEStateFlushDevAddr)], drc0;
	wdf		drc0;

	/***************************************
		Send the Complex Geometry MTE state information...
	****************************************/
	mov		r0, #EURASIA_MTE_STATE_BUFFER_CMPLX_OFFSET;
	iaddu32		r0, r0.low, pa0;

	/* Copy the data from memory into PA regs... */
	MK_MEM_ACCESS_BPCACHE(ldad.f4) pa0, [r0, #0++], drc1;
	wdf		drc1;

	or	pa3, pa3, #EURASIA_TAPDSSTATE_COMPLEXGEOM4_CTX_RESTORE;
	/* Copy the data to the output buffer and then emit it... */
	mov.rpt4 o0, pa0;
	emitmtest.freep.end #0, #(EURASIA_MTEEMIT1_COMPLEX | EURASIA_MTEEMIT1_COMPLEX_PHASE1), #0;
#else
	nop.end;
#endif

/*****************************************************************************
 End of file (tastaterestore.asm)
*****************************************************************************/
