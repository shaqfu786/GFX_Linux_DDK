/*****************************************************************************
 Name		: tastaterestore.asm

 Title		: State restore USE program

 Author 	: PowerVR

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

 Description 	: USE program for restoring the state information after a context switch

 Program Type	: USE assembly language

 Version 	: $Revision: 1.25 $

 Modifications	:

 $Log: tastaterestore.asm $
*****************************************************************************/

#include "usedefs.h"

	.skipinvon;

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	/* Set macro operations to only increment src2 */
	smlsi	#1,#1,#1,#1,#0,#0,#0,#0,#0,#0,#0;

	#if defined(SGX_FEATURE_USE_UNLIMITED_PHASES)
	/* No following phase. */
	phas	#0, #0, pixel, end, parallel;
	#endif

	/* Get the location of the memory structures as passed via PAs... */
	#if defined(SGX_FEATURE_MTE_STATE_FLUSH)
	MK_MEM_ACCESS_BPCACHE(ldad)	pa1, [pa0, #DOFFSET(SGXMK_TA3D_CTL.sTARenderContext)], drc0;
	wdf		drc0;
	mov			pa2, #OFFSET(SGXMKIF_HWRENDERCONTEXT.sMTEStateFlushDevAddr);
	iaddu32		pa1, pa2.low, pa1;
		#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
	/* offset to the state for this core */	
	mov		pa2, g45;
	imae	pa1, pa2.low, #SIZEOF(IMG_UINT32), pa1, u32;
		#endif
	MK_MEM_ACCESS_BPCACHE(ldad) 	pa1, [pa1], drc0;
	wdf		drc0;
	#else
	MK_MEM_ACCESS_BPCACHE(ldad)	pa1, [pa0, #DOFFSET(SGXMK_TA3D_CTL.sTACCBCmd)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	pa1, [pa1, #DOFFSET(SGXMKIF_CMDTA.sTAStateShadowDevAddr)], drc0;
	wdf		drc0;
	#endif
	/***************************************
		Send the MTE state information...
	****************************************/

	/* Copy the data from memory into PA regs... */
	MK_MEM_ACCESS_BPCACHE(ldad.f16) pa2, [pa1, #0++], drc1;
#if defined(SGX545)
	MK_MEM_ACCESS_BPCACHE(ldad.f16) pa18, [pa1, #0++], drc1;
	MK_MEM_ACCESS_BPCACHE(ldad.f16) pa34, [pa1, #0++], drc1;
	MK_MEM_ACCESS_BPCACHE(ldad.f16) pa50, [pa1, #0++], drc1;
	MK_MEM_ACCESS_BPCACHE(ldad.f16) pa66, [pa1, #0++], drc1;
	MK_MEM_ACCESS_BPCACHE(ldad.f16) pa82, [pa1, #0++], drc1;
	MK_MEM_ACCESS_BPCACHE(ldad.f16) pa98, [pa1, #0++], drc1;
	MK_MEM_ACCESS_BPCACHE(ldad.f12) pa114, [pa1, #0++], drc1;
#else
	#if defined(SGX543) || defined(SGX544)
	MK_MEM_ACCESS_BPCACHE(ldad.f9) pa18, [pa1, #0++], drc1;
	#else
	MK_MEM_ACCESS_BPCACHE(ldad.f8) pa18, [pa1, #0++], drc1;
	#endif
#endif
	wdf		drc1;

	/* Copy the data to the output buffer and then emit it... */
	mov.rpt16 o0, pa2;
#if defined(SGX545)
	mov.rpt16 o16, pa18;
	mov.rpt16 o32, pa34;
	mov.rpt16 o48, pa50;
	mov.rpt16 o64, pa66;
	mov.rpt16 o80, pa82;
	mov.rpt16 o96, pa98;
	mov.rpt12 o112, pa114;
	emitvcbst.threepart	#0, #0;
	wop	#0;
	emitmtest.threepart.freep.end #0, #0, #0;
#else
	#if defined(SGX543) || defined(SGX544)
	mov.rpt9 o16, pa18;
	mov pa2, #(EURASIA_TACTL_ALL_SIZEINDWORDS+2); // +2 (header dword and pds_batch_num).
	#else
	mov.rpt8 o16, pa18;
	mov pa2, #(EURASIA_TACTL_ALL_SIZEINDWORDS+1); // +1 for the header dword.
	#endif
	emitst.freep.end #0, pa2;
#endif
#else
	nop.end;
#endif

/*****************************************************************************
 End of file (tastaterestore.asm)
*****************************************************************************/
