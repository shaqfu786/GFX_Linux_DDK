/*****************************************************************************
 Name		: tasarestore.use.asm

 Title		:  Secondary Attribute Restore Program

 Author 	: PowerVR

 Created  	: 10/05/2009

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

 Description 	: USE program for reloading the secondary attributes saved 
 				  during a context switch.

 Program Type	: USE assembly language

 Version 	: $Revision: 1.13 $

 Modifications	:

 $Log: tasarestore.use.asm $
 .#
  --- Revision Logs Removed --- 
 
 *****************************************************************************/

#include "sgxdefs.h"
#include "usedefs.h"

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) && defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
	.skipinvon;

#if defined(SGX_FEATURE_USE_UNLIMITED_PHASES)
	/* No following phase. */
	phas	#0, #0, pixel, end, parallel;
#endif
#if defined(FIX_HW_BRN_31988)
	/* predicated load of size 128-bits */
	.align 2, 1;
	nop.nosched;
	and.testz.skipinv	p0, #1, #1;
	p0	ldad.skipinv.f4		pa0,	[pa0, #1++], drc0;
	wdf		drc0;
#endif
#if 0 // !SGX_SUPPORT_MASTER_ONLY_SWITCHING
	/* Set macro operations to only increment src2 */
	smlsi	#1,#1,#1,#1,#0,#0,#0,#0,#0,#0,#0;
	
	/* The immediate below will be replaced with the address at runtime */
	SABufferBaseAddr:
	mov		r0, #0xDEADBEEF;
	
	
	#if defined(SGX_FEATURE_MP)
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

	/* The first value is the number of SA's stored */
	MK_MEM_ACCESS_CACHED(ldad)	r1, [r0, #1++], drc0;
	mov		r2, #0;
	mov		i.l, r2;
	wdf		drc0;
	/* There were secondary attributes allocated so reload them */
	LoadNextBatch:
	{
		/* SA and PA are the same on a secondary update task */
		MK_MEM_ACCESS_CACHED(ldad.f16)	r3, [r0, #0++], drc0;
		isub16.testnz	r1, p0, r1, #1;
		wdf		drc0;
		mov.rpt16		pa[i.l], r3;
		p0	iaddu32	r2, r2.low, #16;
		p0	mov		i.l, r2;		
		p0	br	LoadNextBatch;
	}
#endif /* !SGX_SUPPORT_MASTER_ONLY_SWITCHING */	
	nop.end;
#else
	/* File can not be empty due to compiler */
	nop;
#endif
