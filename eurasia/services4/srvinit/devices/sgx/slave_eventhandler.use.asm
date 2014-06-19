/*****************************************************************************
 Name		: slave_eventhandler.use.asm

 Title		: USE program for EDM event processing

 Author 	: PowerVR

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

 Description 	: USE program to handle ukernel work outside of core 0 on 
 					SGX-MP cores.

 Program Type	: USE assembly language

 Version 	: $Revision: 1.4 $

 Modifications	:

 $Log: slave_eventhandler.use.asm $

 *****************************************************************************/

#include "usedefs.h"
#include "pvrversion.h"
#include "sgx_options.h"

#if defined(SGX_FEATURE_MP)
/***************************
 Sub-routine Imports
***************************/
/* Utility routines */
.import InvalidateBIFCache;
#if defined(FIX_HW_BRN_32052) && defined(SGX_FEATURE_ISP_CONTEXT_SWITCH_REV_3)
.import BRN32052ClearMCIState;
.import MKTrace;
#endif

/***************************
 Sub-routine Exports
***************************/
.export SlaveEventHandler;
.export SSPRV_Return;

.modulealign 2;
SlaveEventHandler:
{
	UKERNEL_PROGRAM_INIT_COMMON();
	
#if defined(FIX_HW_BRN_31474)
	/* This program will run for this workaround. Nothing is required in 
		the USSE thou, so the program will just exit */	
#endif

#if defined(FIX_HW_BRN_31620)
	/* Does the ukernel need to re-fill the PD cache */
	and		r0, SLAVE_PA(ui32PDSIn0), #EURASIA_PDS_IR0_EDM_EVENT_SW_EVENT1;
	and.testnz	p0, r0, r0;
	!p0	br		NoPDECacheFill;
	{
		/* force a refresh of the updated entries for the PD cache */
		mov		r0, #SGX_UKERNEL_SPRV_BRN_31620;
		mov		r1, #0;
		mov		r2, #0;
		brl		JumpToSlaveSupervisor;
	}
	NoPDECacheFill:
#endif

#if defined(FIX_HW_BRN_32052) && defined(SGX_FEATURE_ISP_CONTEXT_SWITCH_REV_3)
	/* Check for slave core EOR occurring */
	and		r0, SLAVE_PA(ui32PDSIn0), #EURASIA_PDS_IR0_EDM_EVENT_MPPIXENDRENDER;
	and.testnz	p0, r0, r0;
	!p0	br		NoEOREvent;
	{
		/* Check each core's event_status */
		/* FIXME: should not use microkernel SA state in slave program. */ 
		MK_LOAD_STATE_CORE_COUNT_3D(r2, drc0);
		mov				r0, ~#0;
		MK_WAIT_FOR_STATE_LOAD(drc0);
		NextCore_EORStatus:
		{
			isub32		r2, r2, #1;
			READCORECONFIG(r1, r2, #EUR_CR_EVENT_STATUS >> 2, drc0);
			mov			r3, g45;
			wdf			drc0;
			MK_TRACE_REG(r3, MKTF_EHEV | MKTF_3D | MKTF_SCH | MKTF_CSW);
			MK_TRACE_REG(r1, MKTF_EHEV | MKTF_3D | MKTF_SCH | MKTF_CSW);
			and			r1, r1, #EUR_CR_EVENT_STATUS_PIXELBE_END_RENDER_MASK;

			/* Additional check for context store finishing */
			READCORECONFIG(r3, r2, #EUR_CR_ISP_CONTEXT_SWITCH2 >> 2, drc0);
			wdf			drc0;
			MK_TRACE_REG(r3, MKTF_EHEV | MKTF_3D | MKTF_SCH | MKTF_CSW);
			and			r3, r3, #EUR_CR_ISP_CONTEXT_SWITCH2_END_OF_RENDER_MASK;
			
			/* Check event_status_eor OR context_switch2_eor is set */
			shl			r3, r3, #(EUR_CR_EVENT_STATUS_PIXELBE_END_RENDER_SHIFT - EUR_CR_ISP_CONTEXT_SWITCH2_END_OF_RENDER_SHIFT);
			or			r1, r1, r3;
			and			r0, r0, r1;
	
			/* Check next core */
			and.testnz	p0, r2, r2;
			p0			br	NextCore_EORStatus;
		}
		
		shl.tests		p0, r0, #(31 - EUR_CR_EVENT_STATUS_PIXELBE_END_RENDER_SHIFT);
		/* Is this the last core to complete EOR? */
		p0				brl	BRN32052ClearMCIState;	/* raises MCI EOR event */
	}
	NoEOREvent:
#endif

#if !defined(FIX_HW_BRN_29104)
	UKERNEL_PROGRAM_PHAS();
#endif

	/* End the program */
	mov.end		r0, r0;
}

JumpToSlaveSupervisor:
{
	/* 
		Note:
		 - we can use PAs, SAs or Temps for argument passing from
		   micorkernel to SPRV
		 - using Temps for this implementation
	*/
	
	/* Args are passed in temps r0-2 */

	/*
		Save state:
	*/
	/* pclink */
	mov r31, pclink;
	shl r31, r31, #4;
	
	/* predicates */

	/* transition to supervisor */
	sprvv.end;
	
SSPRV_Return: /* return address to patch sprv phas with */

	/* re-specify the smlsi on returning from supervisor */
	smlsi	#0,#0,#0,#1,#0,#0,#0,#0,#0,#0,#0;

	/*
		Restore state
	*/
	/* predicates */
	shr		r31, r31, #4;
	mov		pclink, r31;
	
	/* pclink */
	
	/* return */
	lapc;
}

#else
nop;
#endif

/******************************************************************************
 End of file (slave_eventhandler.use.asm)
******************************************************************************/
