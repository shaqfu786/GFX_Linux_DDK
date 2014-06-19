/*****************************************************************************
 Name		: vdmcontextstore_use.asm

 Title		: USE programs for TQ 

 Author 	: PowerVR

 Created  	: 03/19/2008

 Copyright	: 2008 by Imagination Technologies Limited. All rights reserved.
		  No part of this software, either material or conceptual
		  may be copied or distributed, transmitted, transcribed,
		  stored in a retrieval system or translated into any
		  human or computer language in any form by any means,
		  electronic, mechanical, manual or other-wise, or
		  disclosed to third parties without the express written
		  permission of Imagination Technologies Limited, HomePark
		  Industrial Estate, King's Langley, Hertfordshire,
		  WD4 8LZ, U.K.

 Description 	: USE programs for different TQ shaders blits etc. 

 Program Type	: USE assembly language

 Version 	: $Revision: 1.14 $

 Modifications	:

 $Log: vdmcontextstore_use.asm $
 
 *****************************************************************************/

#include "usedefs.h"
	.skipinvon;
	
#if defined(SGX_FEATURE_USE_UNLIMITED_PHASES)
	/* No following phase. */
	phas	#0, #0, pixel, end, parallel;
#endif

#if defined(FIX_HW_BRN_33657) && defined(SUPPORT_SECURE_33657_FIX)
	/* Is this a VDM state complete on terminate pass? */
	TerminateFlagsBaseAddr:
	mov		r0, #0xDEADBEEF;
	MK_MEM_ACCESS_BPCACHE(ldad.f2)	r1, [r0, #0++], drc0;
	wdf		drc0;
	and.testz	p0, r1, #VDM_TERMINATE_COMPLETE_ONLY;
	p0		br	NoTerminate;
	{	
		/* Set the MTE state update control (normal TA terminate, set bbox) */
		mov	o0, #EURASIA_TACTLPRES_TERMINATE;
		mov o1, r2;	/* ui32StateTARNDClip */
		br StateEmit;
	}
	NoTerminate:
#endif/* FIX_HW_BRN_33657 && SUPPORT_SECURE_33657_FIX */

#if defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
	/* TODO: Make use of the PBE to write the SAs to memory */
	smlsi	#0, #0, #0,#1,#0,#0,#0,#0,#0,#0,#0;
	{
		/* The immediate below will be replaced with the address at runtime */
		SABufferBaseAddr:
		mov		r0, #0xDEADBEEF;
	
	#if 0 // defined(SGX_FEATURE_MP) && !SGX_SUPPORT_MASTER_ONLY_SWITCHING
		/* Load the DevVAddr and Stride directly from the TA3D in memory */
		MK_MEM_ACCESS_BPCACHE(ldad.f2)	r0, [r0, #0++], drc0;
		mov		r2, g45;
		wdf		drc0;
		/* offset the base address by the core index */
		imae	r0, r1.low, r2.low, r0, u32;
	#else
		/* Load the DevVAddr directly from the TA3D in memory */
		MK_MEM_ACCESS_BPCACHE(ldad)	r0, [r0], drc0;
		wdf		drc0;
	#endif
		
		mov		r1, g41;
		mov		r2, #0;
		mov		i.l, r2;
		wdf		drc0;
		/* double the value as it reports number of 64bit register and we need 32bits */
		shl		r1, r1, #1;
		/* The first value is the number of SA's stored */
		MK_MEM_ACCESS_CACHED(stad)	[r0, #1++], r1;
		and.testz	p0, g41, g41;
		p0	br	NoSASave;
		{
			/* There are secondary attributes allocated so save them off */
			SaveNextBatch:
			{
				MK_MEM_ACCESS_CACHED(stad.f16)	[r0, #0++], sa[i.l];
				isub16.testnz	r1, p0, r1, #1;
				p0	iaddu32		r2, r2.low, #16;
				p0	mov			i.l, r2;
				p0	br	SaveNextBatch;
			}
		}
		NoSASave:
		/* Wait for all the memory accesses to flush */
		idf		drc0, st;
		wdf		drc0;
	}
#endif /* SGX_FEATURE_USE_SA_COUNT_REGISTER */

#if defined(FIX_HW_BRN_33657)
	#if !defined(SUPPORT_SECURE_33657_FIX)
	/* Clear complete on terminate on this core */
	mov	r2, g45;
	READCORECONFIG(r1, r2, #EUR_CR_TE_PSG >> 2, drc0);
	wdf	drc0;
	and	r1, r1, ~#(EUR_CR_TE_PSG_COMPLETEONTERMINATE_MASK | EUR_CR_TE_PSG_ZONLYRENDER_MASK);
	WRITECORECONFIG(r2, #EUR_CR_TE_PSG >> 2, r1, r3);
	idf	drc0, st;
	wdf	drc0;
	#endif /* SUPPORT_SECURE_33657_FIX */

	/* Set the MTE state update control (ctx store terminate, bbox not applicable) */
	mov	o0, #EURASIA_TACTLPRES_TERMINATE;
#else
	/* Set the MTE state update control */
	mov	o0, #(EURASIA_TACTLPRES_TERMINATE|EURASIA_TACTLPRES_CONTEXTSWITCH);
#endif
	mov o1, #0;
	
	StateEmit:
#if !defined(SGX545)
	emitst.freep.end #0, #(EURASIA_TACTL_ALL_SIZEINDWORDS+1);
#else /* !defined(SGX545) */
	emitvcbst	#0, #0;
	wop		#0;
	emitmtest.freep.end	#0, #0, #0;
#endif /* !defined(SGX545) */

