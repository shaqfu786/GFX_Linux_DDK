/*****************************************************************************
 Name		: 3d.asm

 Title		: USE program for handling 3D related events

 Author 	: Imagination Technologies

 Created  	: 01/01/2006

 Copyright	: 2007 by Imagination Technologies Limited. All rights reserved.
		  No part of this software, either material or conceptual
		  may be copied or distributed, transmitted, transcribed,
		  stored in a retrieval system or translated into any
		  human or computer language in any form by any means,
		  electronic, mechanical, manual or other-wise, or
		  disclosed to third parties without the express written
		  permission of Imagination Technologies Limited, HomePark
		  Industrial Estate, King's Langley, Hertfordshire,
		  WD4 8LZ, U.K.

 Description 	: USE program for handling all 3D events

 Program Type	: USE assembly language

 Version 	: $Revision: 1.283 $

 Modifications	:

 $Log: 3d.asm $
 *****************************************************************************/
#include "usedefs.h"

/***************************
 Sub-routine Imports
***************************/
.import EmitLoopBack;
.import MKTrace;
#if defined(FIX_HW_BRN_28889)
.import InvalidateBIFCache;
#endif

#if defined(SGX_FEATURE_MK_SUPERVISOR)
.import JumpToSupervisor;
#endif

.import	IdleCore;
.import Resume;
.import LoadDPMContext;
.import StoreDPMContext;
.import SetupDirListBase;
#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
.import SetupRequestor;
#endif
.import SwitchEDMBIFBank;

.import StoreTAPB;
.import Store3DPB;
.import Load3DPB;
.import SetupDPMPagetable;

.import RenderHalt;

.import	CheckTAQueue;
.import Check3DQueues;
#if defined(SGX_FEATURE_2D_HARDWARE)
.import Check2DQueue;
#endif

#if defined(SGX_FEATURE_SYSTEM_CACHE)
.import InvalidateSLC;
#endif

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
.import InvalidateExtSysCache;
#endif

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
.import StartTAContextSwitch;
#endif
#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH)
.import Start3DContextSwitch;
.import ResumeRender;
#endif

#if defined(FIX_HW_BRN_23862)
.import FixBRN23862;
#endif
#if defined(FIX_HW_BRN_23055)
.import DPMReloadFreeList;
.import DPMPauseAllocations;
.import DPMResumeAllocations;
#endif /* defined(FIX_HW_BRN_23055) */
#if (defined(FIX_HW_BRN_24304) || defined(FIX_HW_BRN_27510)) && defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
.import	ConfigureCache;
#endif
#if defined(FIX_HW_BRN_32052) && defined(SGX_FEATURE_ISP_CONTEXT_SWITCH_REV_3)
.import	BRN32052ClearMCIState;
#endif

.import ClearActivePowerFlags;
.import CommonProgramEnd;

.import WriteHWPERFEntry;

/***************************
 Sub-routine Exports
***************************/
.export ThreeDEventHandler;
.export ThreeDHandler;
.export Find3D;

.modulealign 2;
/*****************************************************************************
 ThreeDEventHandler routine	- decides which events have occurred and calls the
 						relevent subroutines.

 inputs:
 temps:
*****************************************************************************/
ThreeDEventHandler:
{
	UKERNEL_PROGRAM_INIT_COMMON();

	WRITEHWPERFCB(3D, MK_3D_START, MK_EXECUTION, #0, p0, mk_3d_event_start, r2, r3, r4, r5, r6, r7, r8, r9);

	/* We have received a 3D event, so lets find out what and set
		the bits and emit loopback */
	/* Test the 3D mem free event */
	and		r0, PA(SGX_MK_PDS_3DMEMFREE_PA), #SGX_MK_PDS_3DMEMFREE;
	and.testnz	p0, r0, r0;
	!p0 	br	TDEH_No3DMemFree;
	{
		MK_TRACE(MKTC_3DEVENT_3DMEMFREE, MKTF_EV | MKTF_3D);
		mov		r0, #EUR_CR_EVENT_HOST_CLEAR_DPM_3D_MEM_FREE_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r0;
		mov		R_EDM_PCLink, #LADDR(TDEH_No3DMemFree);
		ba		ThreeDMemFree;
	}
	TDEH_No3DMemFree:

	/* Test the pixelbe end of render (non 3D mem free renders) */
	and		r0, PA(SGX_MK_PDS_EOR_PA), #SGX_MK_PDS_EOR;
	and.testnz	p0, r0, r0;
	!p0 	br	TDEH_NoPixelEndRender;
	{
		MK_TRACE(MKTC_3DEVENT_PIXELENDRENDER, MKTF_EV | MKTF_3D);
#if defined(SGX_SUPPORT_HWPROFILING)
		/* Move the frame no. into the structure PVRSRV_SGX_HWPROFILING*/
		MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui323DFrameNum)], drc0;
		MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sHWProfilingDevVAddr)], drc0;
		wdf	drc0;
		MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_HWPROFILING.ui32End3D)], r3;
#endif /* SGX_SUPPORT_HWPROFILING*/

		/* Check whether this event is for a Transfer Queue render. */
		MK_LOAD_STATE(r3, ui32IRenderFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and.testnz		p0, r3, #RENDER_IFLAGS_TRANSFERRENDER;
		p0 ba			PixelEndRender_Transfer;
		{
			ldr		r10, #EUR_CR_DPM_PAGE >> 2, drc0;
			wdf		drc0;
			and		r10, r10, #EUR_CR_DPM_PAGE_STATUS_3D_MASK;
			shr		r10, r10, #EUR_CR_DPM_PAGE_STATUS_3D_SHIFT;

			and.testnz		p0, r3, #RENDER_IFLAGS_SPMRENDER;
			p0 ba			PixelEndRender_SPMRender;
			{
				WRITEHWPERFCB(3D, 3D_END, GRAPHICS, r10, p0, k3d_end_1, r2, r3, r4, r5, r6, r7, r8, r9);
				ba	PixelEndRender_EndHWPerf;
			}
			PixelEndRender_SPMRender:
			WRITEHWPERFCB(3D, 3DSPM_END, GRAPHICS, r10, p0, k3d_end_2, r2, r3, r4, r5, r6, r7, r8, r9);
			ba	PixelEndRender_EndHWPerf;
		}
		PixelEndRender_Transfer:
		{
			WRITEHWPERFCB(3D, TRANSFER_END, GRAPHICS, #0, p0, kt_event_end_1, r2, r3, r4, r5, r6, r7, r8, r9);
		}
		PixelEndRender_EndHWPerf:

		mov		r0, #EUR_CR_EVENT_HOST_CLEAR_PIXELBE_END_RENDER_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r0;

		mov		R_EDM_PCLink, #LADDR(TDEH_NoPixelEndRender);
		ba		PixelEndRender;
	}
	TDEH_NoPixelEndRender:

	/* Test for ISP breakpoint events */
	and		r0, PA(ui32PDSIn1), #EURASIA_PDS_IR1_EDM_EVENT_ISPBREAKPOINT;
	and.testnz	p0, r0, r0;
	!p0	br		TDEH_NoISPBreakpoint;
	{
		MK_TRACE(MKTC_3DEVENT_ISPBREAKPOINT, MKTF_EV | MKTF_3D);
#if !defined(FIX_HW_BRN_23761) && !defined(SGX_FEATURE_VISTEST_IN_MEMORY)
		mov		r0, #EUR_CR_EVENT_HOST_CLEAR_BREAKPOINT_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r0;

		/* Jump to occlusion query handler */
		mov		R_EDM_PCLink, #LADDR(TDEH_NoISPBreakpoint);
		/* Set VQA mode to breakpoints */
		mov		r1, #USE_FALSE;
		ba		VisibilityQueryAccumulator;
#endif
	}
	TDEH_NoISPBreakpoint:

	WRITEHWPERFCB(3D, MK_3D_END, MK_EXECUTION, #0, p0, mk_3d_event_end, r2, r3, r4, r5, r6, r7, r8, r9);

	/* End the program */
	UKERNEL_PROGRAM_END(#0, MKTC_3DEVENT_END, MKTF_EV | MKTF_3D);
}

/*****************************************************************************
 3DHandler routine	- decides which events have occurred and calls the
 						relevent subroutines.

 inputs:
 temps:
*****************************************************************************/
.align 2;
ThreeDHandler:
{
	UKERNEL_PROGRAM_INIT_COMMON();

	CLEARPENDINGLOOPBACKS(r0, r1);

	WRITEHWPERFCB(3D, MK_3D_START, MK_EXECUTION, #0, p0, mk_3d_start, r2, r3, r4, r5, r6, r7, r8, r9);

	/* Test the 3D mem free event */
	and		r0, PA(ui32PDSIn0), #USE_LOOPBACK_3DMEMFREE;
	and.testnz	p0, r0, r0;
	!p0	ba		NoRenderFinished;
	{
		MK_TRACE(MKTC_3DLB_3DMEMFREE, MKTF_EV | MKTF_3D);

		mov		r0, #EUR_CR_EVENT_HOST_CLEAR_DPM_3D_MEM_FREE_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r0;

		mov		R_EDM_PCLink, #LADDR(NoRenderFinished);
		ba		ThreeDMemFree;
	}
	NoRenderFinished:

	/* Test the pixelbe end of render (non 3D mem free renders) */
	and		r0, PA(ui32PDSIn0), #USE_LOOPBACK_PIXENDRENDER;
	and.testnz	p0, r0, r0;
	!p0	ba		NoPixelEndRender;
	{
		MK_TRACE(MKTC_3DLB_PIXELENDRENDER, MKTF_EV | MKTF_3D);

		mov		r0, #EUR_CR_EVENT_HOST_CLEAR_PIXELBE_END_RENDER_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r0;

		mov		R_EDM_PCLink, #LADDR(NoPixelEndRender);
		ba		PixelEndRender;
	}
	NoPixelEndRender:

	/* Test for ISP breakpoint events */
	and		r0, PA(ui32PDSIn0), #USE_LOOPBACK_ISPBREAKPOINT;
	and.testnz	p0, r0, r0;
	!p0	ba		NoISPBreakpoint;
	{
		MK_TRACE(MKTC_3DLB_ISPBREAKPOINT, MKTF_EV | MKTF_3D);
#if !defined(FIX_HW_BRN_23761) && !defined(SGX_FEATURE_VISTEST_IN_MEMORY)
		mov		r0, #EUR_CR_EVENT_HOST_CLEAR_BREAKPOINT_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r0;

		/* Jump to occlusion query handler */
		mov		R_EDM_PCLink, #LADDR(NoISPBreakpoint);
		/* Set VQA mode to breakpoints */
		mov		r1, #USE_FALSE;
		ba		VisibilityQueryAccumulator;
#endif
	}
	NoISPBreakpoint:

	/* Test the Find3D call */
	and		r0, PA(ui32PDSIn0), #USE_LOOPBACK_FIND3D;
	and.testnz	p0, r0, r0;
	!p0	ba		NoFind3D;
	{
		MK_TRACE(MKTC_3DLB_FIND3D, MKTF_EV | MKTF_3D);

		bal		Find3D;
	}
	NoFind3D:

	WRITEHWPERFCB(3D, MK_3D_END, MK_EXECUTION, #0, p0, mk_3d_end, r2, r3, r4, r5, r6, r7, r8, r9);

	/* End the program */
	UKERNEL_PROGRAM_END(#0, MKTC_3DLB_END, MKTF_EV | MKTF_3D);
}

/*****************************************************************************
 3DMemFree routine	- process the 3D memory free event

 inputs:
 temps:
*****************************************************************************/
ThreeDMemFree:
{
	/* Get the internal render flags */
	MK_LOAD_STATE(r0, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	/* Flag that the 3D mem free event has happened and check if both events have happened. */
	and			r0, r0, ~#RENDER_IFLAGS_WAITINGFOR3DFREE;
	and.testz		p1, r0, #RENDER_IFLAGS_WAITINGFOREOR;

	/* If both events have happened then do the post-render cleanup. */
	p1	ba		RenderFinished;

	/* Otherwise update the internal render flags. */
	MK_STORE_STATE(ui32IRenderFlags, r0);
	MK_WAIT_FOR_STATE_STORE(drc0);

	/* branch back to calling code */
	mov		pclink, R_EDM_PCLink;

	lapc;
}

/*****************************************************************************
 PixelEndRender routine	- process the PBE render finished event

 inputs:
 temps:
*****************************************************************************/
PixelEndRender:
{
	/* Get the internal render flags */
	MK_LOAD_STATE(r0, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	/* Find out if we were trying to context switch */
#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH)
	and.testz	p0, r0, #RENDER_IFLAGS_HALTRENDER;
	p0	ba	PBE_NotRenderHalt;
	{
	#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH_REV_3)
		#if defined(FIX_HW_BRN_32052)
		shl.tests	p0, r0, #(31 - RENDER_IFLAGS_WAITINGFORCSEOR_BIT);
		p0			ba	RH_AllReady;
		{
			/* Check whether context store finished */
			MK_LOAD_STATE_CORE_COUNT_3D(r2, drc0);
			mov				r0, ~#0;
			MK_WAIT_FOR_STATE_LOAD(drc0);
			RH_NextCoreStatus:
			{
				isub32		r2, r2, #1;
				READCORECONFIG(r1, r2, #EUR_CR_EVENT_STATUS >> 2, drc0);
				wdf			drc0;
				MK_TRACE_REG(r1, MKTF_3D | MKTF_SCH | MKTF_CSW);
				and			r1, r1, #EUR_CR_EVENT_STATUS_PIXELBE_END_RENDER_MASK;
		
				/* Additional check for context store finishing */
				READCORECONFIG(r3, r2, #EUR_CR_ISP_CONTEXT_SWITCH2 >> 2, drc0);
				wdf			drc0;
				MK_TRACE_REG(r3, MKTF_3D | MKTF_SCH | MKTF_CSW);
				and			r3, r3, #EUR_CR_ISP_CONTEXT_SWITCH2_END_OF_RENDER_MASK;
				
				/* Check event_status_eor OR context_switch2_eor is set */
				shl			r3, r3, #(EUR_CR_EVENT_STATUS_PIXELBE_END_RENDER_SHIFT - EUR_CR_ISP_CONTEXT_SWITCH2_END_OF_RENDER_SHIFT);
				or			r1, r1, r3;
				and			r0, r0, r1;
				
				/* Check next core */
				and.testnz	p0, r2, r2;
				p0			ba	RH_NextCoreStatus;
			}
			
			mov				r1, #EUR_CR_EVENT_STATUS_PIXELBE_END_RENDER_MASK;
			and.testnz		p0, r0, r1;
			p0	ba	RH_AllComplete;
			
			{
				MK_TRACE(MKTC_RH_STILL_RUNNING, MKTF_3D | MKTF_SCH | MKTF_CSW);
	
				/* Enable slave core EOR events */
				MK_LOAD_STATE_CORE_COUNT_3D(r2, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);
				RH_EOREnableNextCore:
				{
					isub32		r2, r2, #1;
					READCORECONFIG(r1, r2, #EUR_CR_EVENT_PDS_ENABLE >> 2, drc0);
					wdf			drc0;
					or			r1, r1, #EUR_CR_EVENT_PDS_ENABLE_PIXELBE_END_RENDER_MASK;
					WRITECORECONFIG(r2, #EUR_CR_EVENT_PDS_ENABLE >> 2, r1, r0);
			
					/* Check next core */
					and.testnz	p0, r2, r2;
					p0			ba	RH_EOREnableNextCore;
				}
				idf			drc0, st;
				wdf			drc0;
				
				/* At least one core didn't complete the store correctly */
				MK_LOAD_STATE(r0, ui32IRenderFlags, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);
				or			r0, r0, #RENDER_IFLAGS_WAITINGFORCSEOR;
				MK_STORE_STATE(ui32IRenderFlags, r0);
				MK_WAIT_FOR_STATE_STORE(drc0);
				
				/* Cannot end render until slave core EOR event(s) */
			#if !defined(FIX_HW_BRN_29104)
				UKERNEL_PROGRAM_PHAS();
			#endif
				nop.end;
			}
			RH_AllComplete:
			
			MK_TRACE(MKTC_RH_CLEARMCI, MKTF_3D | MKTF_SCH | MKTF_CSW);
	
			/* Un-stick the MCI (disable MCI EOR interrupt) */
			READMASTERCONFIG(r1, #EUR_CR_MASTER_EVENT_PDS_ENABLE >> 2, drc0);
			wdf			drc0;
			and			r1, r1, ~#EUR_CR_MASTER_EVENT_PDS_ENABLE_PIXELBE_END_RENDER_MASK;
			WRITEMASTERCONFIG(#EUR_CR_MASTER_EVENT_PDS_ENABLE >> 2, r1, r2);
			idf			drc0, st;
			wdf			drc0;
			PVRSRV_SGXUTILS_CALL(BRN32052ClearMCIState);
			/* Enable the MCI EOR interrupt */
			or			r1, r1, #EUR_CR_MASTER_EVENT_PDS_ENABLE_PIXELBE_END_RENDER_MASK;
			WRITEMASTERCONFIG(#EUR_CR_MASTER_EVENT_PDS_ENABLE >> 2, r1, r2);
			idf			drc0, st;
			wdf			drc0;
		}
		RH_AllReady:
	
		MK_TRACE(MKTC_RH_EOR, MKTF_3D | MKTF_SCH | MKTF_CSW);
	
		/* Disable slave core EOR events */
		MK_LOAD_STATE_CORE_COUNT_3D(r2, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		RH_EORDisableNextCore:
		{
			isub32		r2, r2, #1;
			READCORECONFIG(r1, r2, #EUR_CR_EVENT_PDS_ENABLE >> 2, drc0);
			wdf			drc0;
			and			r1, r1, ~#EUR_CR_EVENT_PDS_ENABLE_PIXELBE_END_RENDER_MASK;
			WRITECORECONFIG(r2, #EUR_CR_EVENT_PDS_ENABLE >> 2, r1, r0);
	
			/* Check next core */
			and.testnz	p0, r2, r2;
			p0			ba	RH_EORDisableNextCore;
		}
		idf			drc0, st;
		wdf			drc0;
	
		/* Clear EOR event in MCI */
		mov			r1, #EUR_CR_MASTER_EVENT_HOST_CLEAR_PIXELBE_END_RENDER_MASK;
		WRITEMASTERCONFIG(#EUR_CR_MASTER_EVENT_HOST_CLEAR >> 2, r1, r2);
		idf			drc0, st;
		wdf			drc0;
		
		/* Clear the waiting for context store EOR flag in case it was set */
		MK_LOAD_STATE(r0, ui32IRenderFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and		r0, r0, ~#RENDER_IFLAGS_WAITINGFORCSEOR;
		MK_STORE_STATE(ui32IRenderFlags, r0);
		MK_WAIT_FOR_STATE_STORE(drc0);
		#endif /* FIX_HW_BRN_32052 */

		{
			mov		r2, #(EUR_CR_PDS_STATUS_RENDER_COMPLETED_MASK | EUR_CR_PDS_STATUS_CSWITCH_COMPLETED_MASK);
			ENTER_UNMATCHABLE_BLOCK;
			PBE_WaitForIPFComplete:
			{
		#if defined(SGX_FEATURE_MP) && defined(FIX_HW_BRN_31989)
				READ_COLLATE_TA_REG_OR(PBE_PDSStatus, r1, #EUR_CR_PDS_STATUS >> 2, r4, r3);
		#else
				ldr		r1, #EUR_CR_PDS_STATUS >> 2, drc0;
				wdf		drc0;
		#endif
				//shl.testns	p0, r1, #(31 - EUR_CR_PDS_STATUS_IPF_COMPLETED_SHIFT);
				and.testz	p0, r1, r2;
				p0	ba	PBE_WaitForIPFComplete;
			}
			LEAVE_UNMATCHABLE_BLOCK;
			/* check if this was a true EOR or a CtxSwitch generated EOR */
			mov		r2, #EUR_CR_PDS_STATUS_CSWITCH_COMPLETED_MASK;
		}
	#else
		/* check if this was a true EOR or a CtxSwitch generated EOR */
		ldr		r1, #EUR_CR_PDS >> 2, drc0;
		wdf		drc0;
		mov		r2, #(EUR_CR_PDS_0_STATUS_CSWITCH_COMPLETED_MASK | \
						EUR_CR_PDS_1_STATUS_CSWITCH_COMPLETED_MASK);
	#endif /* SGX_FEATURE_ISP_CONTEXT_SWITCH_REV_3 */
		and.testz p0, r1, r2;
		p0	ba	PBE_NotRenderHalt;
		{
			/* This EOR was generated by a successful CtxSwitch */
			mov		r7, #0;
			/* Branch to the RenderHalt code section */
			ba		RenderHalt;
		}
	}
	PBE_NotRenderHalt:
#endif /* SGX_FEATURE_ISP_CONTEXT_SWITCH */

	/* Check if this event is for a Transfer Queue render. */
	and.testnz		p0, r0, #RENDER_IFLAGS_TRANSFERRENDER;
	p0	ba		TransferRenderFinished;
	{
		/* Poll for ZLS to finish before clearing the RENDER_IFLAGS_WAITINGFOREOR flag (see SW BRN34236) */
		MK_TRACE(MKTC_ZLS_IDLE_BEGIN, MKTF_3D);

		/* check for auto-clock gating enabled */
		ldr		r1, #EUR_CR_CLKGATECTL >> 2, drc0;
		wdf		drc0;
		and 	r1, r1, #EUR_CR_CLKGATECTL_ISP_CLKG_MASK;
		xor.testnz p0, r1, #(EUR_CR_CLKGATECTL_AUTO << EUR_CR_CLKGATECTL_ISP_CLKG_SHIFT);
		p0 ba PBE_NoZlsPollPossible;
		{
			MK_TRACE(MKTC_ZLS_ISP_CLK_GATING_EN, MKTF_3D);

			/* Poll */
			PBE_ZlsPoll:
			{
				#if defined(SGX_FEATURE_MP)
					READ_COLLATE_3D_REG_OR(PBE_ZlsPoll, r1, #EUR_CR_CLKGATESTATUS >> 2, r2, r3);
				#else
					ldr		r1, #EUR_CR_CLKGATESTATUS >> 2, drc0;
					wdf		drc0;
				#endif
				and.testnz p0, r1, #EUR_CR_CLKGATESTATUS_ISP_CLKS_MASK;
				p0 ba PBE_ZlsPoll;
			}
		}

		PBE_NoZlsPollPossible:
		MK_TRACE(MKTC_ZLS_IDLE_END, MKTF_3D);
	}

	/* Flag that the PBE EOR event has happened and check if both events have happened. */
	and.testz		p1, r0, #RENDER_IFLAGS_WAITINGFOR3DFREE;
	and				r0, r0, ~#RENDER_IFLAGS_WAITINGFOREOR;

	/* If both events have happened then do the post-render cleanup. */
	p1	ba		RenderFinished;

	/* Otherwise update the internal render flags. */
	MK_STORE_STATE(ui32IRenderFlags, r0);
	MK_WAIT_FOR_STATE_STORE(drc0);

	/* branch back to calling code */
	mov		pclink, R_EDM_PCLink;

	lapc;
}

#if defined(SGX_FEATURE_VISTEST_IN_MEMORY) 
	#if defined(SGX_FEATURE_MP) && (SGX_FEATURE_MP_CORE_COUNT_3D > 1)
/*****************************************************************************
 VisibilityQueryAccumulator - Accumulates results from occlusion queries into memory.

 input: 	r0 - SGXMKIF_HWRENDERDETAILS of scene which has just been rendered
 temps:		r0 - r11
*****************************************************************************/
VisibilityQueryAccumulator:
{
	MK_TRACE(MKTC_VISQUERY_START, MKTF_SCH);

	MK_MEM_ACCESS(ldad)	r0, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32VisTestCount)], drc0;
	wdf		drc0;
	
	/* Double check that there are results to combine */
	and.testz	p1, r0, r0;
	p1	ba	VQA_ZeroCount;
	{
		/* Find out the address of where to store the results  */	
		mov		r2, #0;
		READCORECONFIG(r3, r2, #EUR_CR_ISP_VISREGBASE >> 2, drc0);
		wdf		drc0;
		
		/* Check if there is more than 1 core enabled */
		MK_LOAD_STATE_CORE_COUNT_3D(r2, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		isub16.testz	r2, p1, r2, #1;
		/* If only 1 core is enabled no accumulating is required so exit */
		p1	ba	VQA_ZeroCount;
		{
			/* Calculate where the memory for core 1 starts */
			mov		r4, r3;
			imae	r5, r0.low, #SIZEOF(IMG_UINT32), r4, u32;
	
			SWITCHEDMTO3D();
	
			VQA_NextCore:
			{
				/* Setup the count and accumulate address */
				mov		r1, r0;
				mov		r4, r3;
					
				VQA_NextQuery:
				{
					MK_MEM_ACCESS(ldad)	r6, [r4], drc0;
					MK_MEM_ACCESS_CACHED(ldad)	r7, [r5, #1++], drc0;
					wdf		drc0;
					IADD32(r6, r6, r7, r8);
				
					MK_MEM_ACCESS(stad)	[r4, #1++], r6;
					isub16.testnz	r1, p1, r1, #1;
					p1	ba	VQA_NextQuery;
				}
				
				/* Decrement the core count and see if anymore are enabled */
				isub16.testnz	r2, p1, r2, #1;
				p1	ba	VQA_NextCore;
			}		
	
			idf	drc0, st;
			wdf	drc0;
	
			str	#EUR_CR_MASTER_SLC_CTRL_FLUSH >> 2, #EUR_CR_MASTER_SLC_CTRL_FLUSH_ALL_MASK;
			ENTER_UNMATCHABLE_BLOCK;
			VQA_WaitForFlush:
			{
				ldr		r1, #EUR_CR_MASTER_SLC_EVENT_STATUS >> 2, drc0;
				wdf		drc0;
				and.testz	p1, r1, #EUR_CR_MASTER_SLC_EVENT_STATUS_FLUSH_MASK;
				p1	ba		VQA_WaitForFlush;
			}
			LEAVE_UNMATCHABLE_BLOCK;
	
			/* Clear the status bits */
			str		#EUR_CR_MASTER_SLC_EVENT_CLEAR >> 2, #EUR_CR_MASTER_SLC_EVENT_CLEAR_FLUSH_MASK;
			
			SWITCHEDMBACK();
		}
	}
	VQA_ZeroCount:		

	MK_TRACE(MKTC_VISQUERY_END, MKTF_SCH);
	/* Branch back to calling code */
	lapc;	
}
	#endif /* SGX_FEATURE_MP */
#else
/*****************************************************************************
 VisibilityQueryAccumulator - Accumulates results from occlusion queries into memory.

#if defined(FIX_HW_BRN_23761)
 input: 	r0 - SGXMKIF_HWRENDERDETAILS of scene being rendered
#else
 input:		r1 - USE_TRUE: use memory accumulation, USE_FALSE: use breakpoints
 			r0 - SGXMKIF_HWRENDERDETAILS of scene being rendered (mem accumulation only)
#endif
 temps:		r0 - r11
*****************************************************************************/
VisibilityQueryAccumulator:
{
	MK_TRACE(MKTC_VISQUERY_START, MKTF_SCH);

#if !defined(FIX_HW_BRN_23761)
	/* p1 is true if r1 == USE_TRUE */
	xor.testz		p1, r1, #USE_TRUE;
	p1	ba	VQA_HWRDLoaded;
	{
		/* Get the location of the result mem (breakpoints case) */
		MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRenderDetails)], drc0;
		wdf		drc0;
	}
	VQA_HWRDLoaded:
#endif
	/* Get the location of the result mem */
	MK_MEM_ACCESS(ldad)	r11, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sVisTestResultsDevAddr)], drc0;
	wdf		drc0;

	/* Switch to 3D requestor. */
	SWITCHEDMTO3D();

#if !defined(FIX_HW_BRN_23761)
	p1	ba	VQA_SkipBreakpoints;
	{
		/* Find out the id of this breakpoint object... */
		ldr		r0, #EUR_CR_ISP_STATUS1 >> 2, drc0;
		/* Add EUR_CR_BIF_3D_REQ_BASE value to control stream offset */
		ldr		r2, #EUR_CR_BIF_3D_REQ_BASE >> 2, drc0;
		wdf		drc0;
	
		/* No way of doing a full 32-bit add */
		shr		r2, r2, #16;
		mov		r3, #0xFFFF;
		imae	r0, r2.low, r3.low, r0, u32;
		iaddu32	r0, r2.low, r0;
	
		/* Get the vertex block pointer (to ISP state) from the object... */
		MK_MEM_ACCESS_CACHED(ldad)	r3, [r0, #1], drc0;
		wdf		drc0;
		and		r3, r3, #~EURASIA_PARAM_PB1_VERTEXPTR_CLRMSK;
		shr		r0, r3, #EURASIA_PARAM_PB1_VERTEXPTR_SHIFT;
		shl		r0, r0, #EURASIA_PARAM_PB1_VERTEXPTR_ALIGNSHIFT;
	
		mov		r3, #0xFFFF;
		imae	r0, r2.low, r3.low, r0, u32;
		iaddu32	r0, r2.low, r0;
	
		/* Extract the id from the colour information in the TSP vertex data... */
		MK_MEM_ACCESS_CACHED(ldad)	r0, [r0, -#1], drc0; // (-1 offset to go backwards into the last dword of TSP data)
		wdf		drc0;
		and		r0, r0, #0xFFFF;
	
		/* Find the correct bit of memory for this breakpoint... */
		imae.u	r11, r0.l, #(8 * SIZEOF(IMG_UINT32)), r11, U32;
	}
	VQA_SkipBreakpoints:
#endif /* !defined(FIX_HW_BRN_23761) */

	/* Copy the address so we have a write ptr */
	mov		r12, r11;
	
	/* load batch 0 */
	MK_MEM_ACCESS(ldad.f2)	r0, [r11, #0++], drc0;
	ldr		r2, #EUR_CR_ISP_VISTEST_VISIBLE0 >> 2, drc0;
	ldr		r3, #EUR_CR_ISP_VISTEST_VISIBLE1 >> 2, drc0;
	/* load batch 1 */
	MK_MEM_ACCESS(ldad.f2)	r4, [r11, #0++], drc1;
	ldr		r6, #EUR_CR_ISP_VISTEST_VISIBLE2 >> 2, drc1;
	ldr		r7, #EUR_CR_ISP_VISTEST_VISIBLE3 >> 2, drc1;
	
	/* accumulate batch 0 */
	wdf 		drc0;
	IADD32(r9, r0, r2, r8);
	IADD32(r10, r1, r3, r8);
	MK_MEM_ACCESS(stad.f2)	[r12, #0++], r9;	
	
	/* load batch 2 */
	MK_MEM_ACCESS(ldad.f2)	r0, [r11, #0++], drc0;
	ldr		r2, #EUR_CR_ISP_VISTEST_VISIBLE4 >> 2, drc0;
	ldr		r3, #EUR_CR_ISP_VISTEST_VISIBLE5 >> 2, drc0;
	
	/* accumulate batch 1 */
	wdf 		drc1;
	IADD32(r9, r4, r6, r8);
	IADD32(r10, r5, r7, r8);
	MK_MEM_ACCESS(stad.f2)	[r12, #0++], r9;	
	
	/* load batch 3 */
	MK_MEM_ACCESS(ldad.f2)	r4, [r11, #0++], drc0;
	ldr		r6, #EUR_CR_ISP_VISTEST_VISIBLE6 >> 2, drc1;
	ldr		r7, #EUR_CR_ISP_VISTEST_VISIBLE7 >> 2, drc1;

	/* accumulate batch 2 */
	wdf 		drc0;
	IADD32(r9, r0, r2, r8);
	IADD32(r10, r1, r3, r8);
	MK_MEM_ACCESS(stad.f2)	[r12, #0++], r9;
	
	/* accumulate batch 1 */
	wdf 		drc1;
	IADD32(r9, r4, r6, r8);
	IADD32(r10, r5, r7, r8);
	MK_MEM_ACCESS(stad.f2)	[r12, #0++], r9;	
	
	/* Ensure all writes have completed */
	idf		drc0, st;
	wdf		drc0;

	/* Switch back to EDM requestor... */
	SWITCHEDMBACK();

	/* Clear the all vistest result registers */
	mov		r0, #(EUR_CR_ISP_VISTEST_CTRL_CLEAR0_MASK | \
				  EUR_CR_ISP_VISTEST_CTRL_CLEAR1_MASK | \
				  EUR_CR_ISP_VISTEST_CTRL_CLEAR2_MASK | \
				  EUR_CR_ISP_VISTEST_CTRL_CLEAR3_MASK | \
				  EUR_CR_ISP_VISTEST_CTRL_CLEAR4_MASK | \
				  EUR_CR_ISP_VISTEST_CTRL_CLEAR5_MASK | \
				  EUR_CR_ISP_VISTEST_CTRL_CLEAR6_MASK | \
				  EUR_CR_ISP_VISTEST_CTRL_CLEAR7_MASK);
	str		#EUR_CR_ISP_VISTEST_CTRL >> 2, r0;

#if !defined(FIX_HW_BRN_23761)
	#if !defined(SGX_FEATURE_MP)
		#if defined(SGX_FEATURE_ISP_BREAKPOINT_RESUME_REV_2)
	p1	ba	VQA_SkipBreakpointResume;
	{
		ldr		r0, #EUR_CR_ISP_STATUS2 >> 2, drc0;
		ldr		r2, #EUR_CR_ISP_STATUS3 >> 2, drc1;
		ldr		r1, #EUR_CR_ISP_STATUS1 >> 2, drc1;
		wdf 	drc1;
		/* combine status 1 and 3 */	
		shl		r1, r1, #(EUR_CR_ISP_START_CTRL_STREAM_POS_SHIFT - EUR_CR_ISP_STATUS1_CTRL_STREAM_POS_SHIFT);
		or		r1, r2, r1;
		wdf		drc0;
		
		/* write back the new region base, as it is needed on resume */
		str		#EUR_CR_ISP_RGN_BASE >> 2, r0;
		str		#EUR_CR_ISP_START >> 2, r1;
			#else
		/* Indicate that branch is from VisibilityQueryAccumulator */
		mov		r7, #1;
		/* Adjust the region header base to resume at the correct point */
		bal		RenderHalt;
			#endif
		#endif
	
		/* Setup the return link address */
		mov		pclink, R_EDM_PCLink;
		
		/* Resume the ISP... */
		str		#EUR_CR_ISP_BREAK >> 2, #EUR_CR_ISP_BREAK_RESUME_MASK;
	}
	VQA_SkipBreakpointResume:
#endif /* !defined(FIX_HW_BRN_23761) */

	MK_TRACE(MKTC_VISQUERY_END, MKTF_SCH);
	/* Branch back to calling code */
	lapc;
}
#endif

/*****************************************************************************
 UpdateRenderStatusValues routine	- writes out all status values required to
 			mark a scene as completed
 inputs:	r0 - HWRenderDetails
 			r1 - RTData
 temps:		r2, r3, r4, r5, r6, R_RTAIdx
*****************************************************************************/
UpdateRenderStatusValues:
{
	/* Update the WOpsComplete for this RT */
	MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWDstSyncListDevAddr)], drc1;
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	MK_MEM_ACCESS_BPCACHE(ldad)	R_RTAIdx, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32ActiveRTIdx)], drc0;
#endif
	wdf		drc1;
	and.testz	p0, r2, r2;
	p0	ba	URSV_NoWriteOpUpdate;
	{
	#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
		/* Calculate the Address of the first PVRSRV_DEVICE_SYNC_OBJECT */
		mov	r3, #OFFSET(SGXMKIF_HWDEVICE_SYNC_LIST.asSyncData[0]);
		iaddu32	r2, r3.low, r2;
		/* Apply offset to get this RTs SyncInfo */
		wdf		drc0;
		imae	r2, R_RTAIdx.low, #SIZEOF(PVRSRV_DEVICE_SYNC_OBJECT), r2, u32;
		MK_MEM_ACCESS(ldad.f2)	r2, [r2, #DOFFSET(PVRSRV_DEVICE_SYNC_OBJECT.ui32WriteOpsPendingVal)-1], drc0;
	#else
		MK_MEM_ACCESS(ldad.f2)	r2, [r2, #DOFFSET(SGXMKIF_HWDEVICE_SYNC_LIST.asSyncData[0].ui32WriteOpsPendingVal)-1], drc0;
	#endif
		wdf		drc0;
		and.testz	p0, r3, r3;
		p0	ba	URSV_NoWriteOpUpdate;
		{
			MK_TRACE(MKTC_URSV_UPDATEWRITEOPS, MKTF_3D | MKTF_SO);
			mov	r4, #1;
			iaddu32 r2, r4.low, r2;
			MK_MEM_ACCESS_BPCACHE(stad)	[r3], r2;
			MK_TRACE_REG(r3, MKTF_3D | MKTF_SO);
			MK_TRACE_REG(r2, MKTF_3D | MKTF_SO);
		}
	}
	URSV_NoWriteOpUpdate:

#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	/* Update the ui32NextRTIdx value and check for completion of array */
	/* Check if this is the last RT in the array */
	MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32NumRTs)], drc0;
	wdf		drc0;
	iaddu32		r3, R_RTAIdx.low, #1;
	xor.testnz	p1, r2, r3;
	!p1	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32NextRTIdx)], #0;
	p1	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32NextRTIdx)], r3;
	/* Only update on the last Render in array */
	p1	ba	URSV_NotLastRender;
#endif
	{
		/* check if there is a sTA3DDependancy to update */
		MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32RenderFlags)], drc0;
		mov	r3, #SGXMKIF_RENDERFLAGS_TA_DEPENDENCY;
		wdf		drc0;
		and.testz	p0, r2, r3;
		p0	ba	URSV_NoTADependencyUpdate;
		{
			/* Copy the sTA3DDependancy data */
			MK_MEM_ACCESS(ldad.f2)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sTA3DDependency.ui32WriteOpsPendingVal)-1], drc0;
			wdf		drc0;

			mov	r4, #1;
			iaddu32 r2, r4.low, r2;
			MK_MEM_ACCESS_BPCACHE(stad)	[r3], r2;
		}
		URSV_NoTADependencyUpdate:
		
		MK_MEM_ACCESS(ldad.f2)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32TQSyncReadOpsPendingVal)-1], drc0;
		wdf		drc0;
		and.testz	p0, r3, r3;
		p0	ba	URSV_NoTQSyncUpdate;
		{
			mov	r4, #1;
			iaddu32 r2, r4.low, r2;
			MK_MEM_ACCESS_BPCACHE(stad)	[r3], r2;
		}
		URSV_NoTQSyncUpdate:

		/* update the source dependencies, first get the count */
#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)
		MK_MEM_ACCESS(ldad) r2, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32Num3DSrcSyncs)], drc0;
#else
		MK_MEM_ACCESS(ldad) r2, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32NumSrcSyncs)], drc0;
#endif		
		wdf		drc0;
		and.testz	p0, r2, r2;
		p0	ba	URSV_NoReadOpUpdates;
		{
			/* get the base address of the first block + offset to ui32ReadOpsPendingVal */
#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)
			mov		r3, #OFFSET(SGXMKIF_HWRENDERDETAILS.as3DSrcSyncs[0].ui32ReadOpsPendingVal);
#else
			mov		r3, #OFFSET(SGXMKIF_HWRENDERDETAILS.asSrcSyncs[0].ui32ReadOpsPendingVal);
#endif			
			iadd32	r3, r3.low, r0;

			/* loop through each dependency, loading a PVRSRV_DEVICE_SYNC_OBJECT structure*/
			/* load ui32ReadOpsPendingVal and sReadOpsCompleteDevVAddr */
			MK_MEM_ACCESS(ldad.f2)	r4, [r3, #0++], drc0;
			URSV_UpdateNextDependency:
			{
				/* decrement the count and test for 0 */
				isub16.testz r2, p0, r2, #1;
				/* advance to the next block */
				mov	r6, #16;
				iaddu32	r3, r6.low, r3;
				mov	r6, #1;
				wdf	drc0;

				/* Increment ui32SrcReadOpsPendingVal and copy to sSrcReadOpsCompleteDevVAddr */
				iaddu32 r4, r6.low, r4;
				MK_MEM_ACCESS_BPCACHE(stad)	[r5], r4;

				!p0	MK_MEM_ACCESS(ldad.f2)	r4, [r3, #0++], drc0;
				!p0	ba	URSV_UpdateNextDependency;
			}
		}
		URSV_NoReadOpUpdates:

		MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32Num3DStatusVals)], drc0;
		wdf		drc0;
		and.testz	p0, r2, r2;
		p0	ba		URSV_No3DStatusVals;
		{
			MK_TRACE(MKTC_URSV_UPDATESTATUSVALS, MKTF_3D | MKTF_SV);

			mov		r3, #OFFSET(SGXMKIF_HWRENDERDETAILS.sCtl3DStatusInfo);
			iadd32		r3, r3.low, r0;	// start
			imae		r2, r2.low, #SIZEOF(CTL_STATUS), r3, u32; // finish

			MK_MEM_ACCESS(ldad.f2)	r4, [r3, #0++], drc0;
			URSV_Next3DStatusVal:
			{
				xor.testnz	p0, r2, r3;
				wdf		drc0;
				MK_MEM_ACCESS_BPCACHE(stad)	[r4], r5;

				MK_TRACE_REG(r4, MKTF_3D | MKTF_SV);
				MK_TRACE_REG(r5, MKTF_3D | MKTF_SV);

				p0	MK_MEM_ACCESS(ldad.f2)	r4, [r3, #0++], drc0;
				p0	ba	URSV_Next3DStatusVal;
			}
			MK_TRACE(MKTC_URSV_UPDATESTATUSVALS_DONE, MKTF_3D | MKTF_SV);
		}
		URSV_No3DStatusVals:
	}
	URSV_NotLastRender:
	
	/* Update the RTStatus Flags */
	MK_MEM_ACCESS(ldad)	r4, [r1, #DOFFSET(SGXMKIF_HWRTDATA.sRTStatusDevAddr)], drc0;
	wdf		drc0;
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	imae	r4, R_RTAIdx.low, #SIZEOF(IMG_UINT32), r4, u32;
#endif
	MK_MEM_ACCESS(ldad)	r3, [r4], drc0;
	wdf		drc0;
	or		r3, r3, #SGXMKIF_HWRTDATA_RT_STATUS_RENDERCOMPLETE;
	MK_MEM_ACCESS(stad)	[r4], r3;

	/* make sure all the writes have completed */
	idf		drc0, st;
	wdf		drc0;

	MK_LOAD_STATE(r2, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	shl.testns	p0, r2, #(31 - TA_IFLAGS_OOM_PR_BLOCKED_BIT);
	p0	ba	URSV_NoPRBlocked;
	{
		/* clear the flag */
		and	r2, r2, ~#TA_IFLAGS_OOM_PR_BLOCKED;
		MK_STORE_STATE(ui32ITAFlags, r2);

		/* restart the TA */
		mov		r4, pclink;
		bal 	TARestartWithoutRender;
		mov		pclink, r4;
		
		MK_WAIT_FOR_STATE_STORE(drc0);
	}
	URSV_NoPRBlocked:
	
	/* branch back to calling code */
	lapc;
}

/*****************************************************************************
 ReleaseRenderResources routine	- Releases any locks held on data structures
 inputs:	r0 - HWRenderDetails
 			r1 - RTData
 temps:		r2, r3, r4, r5, r6, R_RTAIdx
*****************************************************************************/
ReleaseRenderResources:
{

#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	/* Update the ui32NextRTIdx value and check for completion of array */
	MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32NextRTIdx)], drc1;
	wdf		drc1;
	and.testnz	p1, r2, r2;
	/* Only update on the last Render in array - ui32NextRTIdx will be 0 */
	p1	ba	RRR_NoRelease;
#endif
	{
		/* Flag render complete status (combined with frame number to aid debugging) */
		MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32RenderFlags)], drc0;
		wdf		drc0;
		
		/* Update the CommonStatus Flags */
		shr		r3, r2, #SGXMKIF_RENDERFLAGS_FRAMENUM_SHIFT;
		wdf		drc1;
		MK_MEM_ACCESS(ldad)	r4, [r1, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc0;
		shl		r3, r3, #SGXMKIF_HWRTDATA_CMN_STATUS_FRAMENUM_SHIFT;
		or		r3, r3, #SGXMKIF_HWRTDATA_CMN_STATUS_READY;
		wdf		drc0;
		and		r4, r4, ~#(SGXMKIF_HWRTDATA_CMN_STATUS_TACOMPLETE | SGXMKIF_HWRTDATA_CMN_STATUS_DUMMYPROC);
		or		r3, r3, r4;
		MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], r3;
	
		/* Freeing the RTData could unblock a TA command, so reset active power management state. */
		MK_STORE_STATE(ui32ActivePowerCounter, #0);
		MK_STORE_STATE(ui32ActivePowerFlags, #SGX_UKERNEL_APFLAGS_ALL);

		/* Increment the complete count for the RTDataSet, only if it's following the last TA kick */
		mov	r3, #SGXMKIF_RENDERFLAGS_RENDERSCENE;
		and.testnz	p0, r2, r3;
		!p0 br RRR_NotRenderScene;
		{
			MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWRTDataSetDevAddr)], drc0;
			wdf		drc0;
			MK_MEM_ACCESS(ldad)	r3, [r4, #DOFFSET(SGXMKIF_HWRTDATASET.ui32CompleteCount)], drc0;
			wdf		drc0;
			REG_INCREMENT(r3, p0);
			MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWRTDATASET.ui32CompleteCount)], r3;
		}
		RRR_NotRenderScene:
		/* Clear the HWRenderDetails pointer */
		MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderDetailsDevAddr)], #0;
	
		/* Check if the HWRenderDetails should be freed */
		and.testnz p0, r2, #SGXMKIF_RENDERFLAGS_RENDERMIDSCENE | SGXMKIF_RENDERFLAGS_NODEALLOC;
		/* We always release the SyncList as we will get another on next kick */
		MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWDstSyncListDevAddr)], drc0;
		p0	ba		RRR_NoRenderDetailsRelease;
		{
			/* Release lock on render details structure */
			MK_MEM_ACCESS(ldad)	r3, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sAccessDevAddr)], drc1;
			wdf		drc1;
			MK_MEM_ACCESS_BPCACHE(stad)	[r3], #0;

			/*
				Check if any of the pointers to RTData structures in the context are
				equal to one we just finished with and if so then clear them.
			*/
			MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext0RTData)], drc1;
			MK_MEM_ACCESS_BPCACHE(ldad)	r4, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext1RTData)], drc0;
			wdf				drc1;
			xor.testz		p0, r1, r3;
			p0	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext0RTData)], #0;
			wdf				drc0;
			xor.testz		p0, r1, r4;
			p0	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext1RTData)], #0;
		}
		RRR_NoRenderDetailsRelease:
		/* wait for the HWDstSyncListDevAddr to load */
		wdf		drc0;
		and.testnz	p0, r2, r2;
		!p0	ba	RRR_NoSyncListRelease;
		{
			MK_MEM_ACCESS(ldad)	r2, [r2, #DOFFSET(SGXMKIF_HWDEVICE_SYNC_LIST.sAccessDevAddr)], drc0;
			wdf		drc0;
			/* Release lock on SyncList structure, if we had one */
			MK_MEM_ACCESS_BPCACHE(stad)	[r2], #0;
		}
		RRR_NoSyncListRelease:
	}
	RRR_NoRelease:
	/* make sure all the writes have completed */
	idf		drc0, st;
	wdf		drc0;

	/* Send interrupt to host */
	mov	 	r3, #EUR_CR_EVENT_STATUS_SW_EVENT_MASK;
	str		#EUR_CR_EVENT_STATUS >> 2, r3;

	/* branch back to calling code */
	lapc;
}

/*****************************************************************************
 RenderFinished routine	- process the render finished event

 inputs:
 temps:
*****************************************************************************/
RenderFinished:
{
	MK_TRACE(MKTC_RENDERFINISHED_START, MKTF_3D);

#if (defined(FIX_HW_BRN_24304) || defined(FIX_HW_BRN_27510)) && defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	CONFIGURECACHE(#CC_FLAGS_HW_FINISH);
#endif

#if defined(FIX_HW_BRN_31938)
	/* Reset the ISP in case this was a subtile render */
	str		#EUR_CR_SOFT_RESET >> 2, #EUR_CR_SOFT_RESET_ISP_RESET_MASK;
	str		#EUR_CR_SOFT_RESET >> 2, #0;
#endif

	/* Clear the render flags and check for partial renders completing */
	and.testnz	p0, r0, #RENDER_IFLAGS_SPMRENDER;
	shl.tests	p2, r0, #31 - RENDER_IFLAGS_TAWAITINGFORMEM_BIT;
#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH)
	and		r1, r0, #~(RENDER_IFLAGS_RENDERINPROGRESS | \
					   RENDER_IFLAGS_TAWAITINGFORMEM | \
					   RENDER_IFLAGS_HWLOCKUP_MASK);
#else
	/* Clear the context switch EOR in case the render completed whilst context storing. */
	and		r1, r0, #~(RENDER_IFLAGS_RENDERINPROGRESS | \
					   RENDER_IFLAGS_HALTRENDER | \
					   RENDER_IFLAGS_HALTRECEIVED | \
					   RENDER_IFLAGS_TAWAITINGFORMEM | \
					   RENDER_IFLAGS_HWLOCKUP_MASK | \
					   RENDER_IFLAGS_FORCEHALTRENDER | \
					   RENDER_IFLAGS_WAITINGFORCSEOR);
#endif
	MK_STORE_STATE(ui32IRenderFlags, r1);
	MK_WAIT_FOR_STATE_STORE(drc0);

	/*
		If this wasn't a SPM partial render but the TA has run out of memory then
		restart it since the render will have freed some memory.
	*/
	p2	bal		TARestartWithoutRender;

#if (SGX_FEATURE_MP_CORE_COUNT_3D > 1) || !defined(SGX_FEATURE_VISTEST_IN_MEMORY)
	#if defined(SGX_FEATURE_VISTEST_IN_MEMORY)
	/* Accumulation and SLC flush only occurs after the Final render with SGX_FEATURE_VISTEST_IN_MEMORY */
	p0	ba	RF_SPMRenderFinished;
	#endif
	{
	#if defined(SGX_FEATURE_VISTEST_IN_MEMORY)
		/* Must be a final render so always use s3DRenderDetails */
		MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRenderDetails)], drc0;
	#else
		/* Accumulation is required after every render including SPM partial renders */
		/* Therefore we need to load the correct HWRenderDetails */
		!p0	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRenderDetails)], drc0;
		p0	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderDetails)], drc0;
	#endif
		/* Wait for the HWRenderDetails base address to load */
		wdf		drc0;
		/* Load the ui32RenderFlags to see if the visibility results have been requested */
		MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32RenderFlags)], drc0;
		wdf		drc0;
	
	#if defined(SGX_FEATURE_VISTEST_IN_MEMORY)
		/* IMG_INTERNAL_COMMENT
			IPG8127: Microkernel accumulation is optional */
		mov		r2, #SGXMKIF_RENDERFLAGS_ACCUMVISRESULTS;
	#else
		mov		r2, #SGXMKIF_RENDERFLAGS_GETVISRESULTS;
	#endif
		and.testnz	p1, r1, r2;
		/* Always use memory accumulation (BP objects handled via 3D event handler) */
		mov		r1, #USE_TRUE;
		/* r0 contains the HWRenderDetails of the scene which has just finished rendering */
		p1 bal		VisibilityQueryAccumulator;
	}
	RF_SPMRenderFinished:
#endif

#if defined(SGX_FEATURE_MP)
	/* If the ZLS format was DPM compressed mode we must change the tile
		allocation mode */
	ldr		r2, #EUR_CR_ISP_ZLSCTL >> 2, drc0;
	wdf		drc0;
	and		r3, r2, #EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_MASK;
	mov 	r4, #EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_F32ZI8S1V;
	xor.testz	p1, r3, r4;
	#if defined(EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_CMPR_F32ZI8S1V)
	!p1 mov	r4, #EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_CMPR_F32ZI8S1V;
	!p1 xor.testz	p1, r3, r4;
	#endif
	#if defined(SGX_FEATURE_ZLS_EXTERNALZ)
	/* if a compressed mode is selected check if it will be DPM allocated */
	p1	mov	r4, #EUR_CR_ISP_ZLSCTL_EXTERNALZBUFFER_MASK;
	p1	and.testz	p1, r2, r4;
	#endif
	!p1	ba	RF_NotDPMZLS;
	{
		MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32MPTileMode)], drc0;
		wdf		drc0;
		/* Change the tile allocation mode back to the default */
		WRITEMASTERCONFIG(#EUR_CR_MASTER_MP_TILE >> 2, r2, r3);
	}
	RF_NotDPMZLS:
#endif
	
	/*
		If this was a SPM partial render then restart the TA and don't do
		any of the normal post-render cleanup (since we haven't actually finished the scene).
	*/
	!p0	ba	RF_NotSPMRender;
	{
		SPMRENDERFINISHED();
		/* Exit the routine */
		ba	RF_End;
	}
	RF_NotSPMRender:

#if (defined(FIX_HW_BRN_30749) && defined(SGX_FEATURE_ISP_CONTEXT_SWITCH_REV_3)) || defined(FIX_HW_BRN_31076)
 	/* Temporary workaround for memory leak */
 	ldr		r2, #EUR_CR_DPM_3D_DEALLOCATE >> 2, drc0;
 	wdf		drc0;
 	and.testz	p0, r2, r2;
 	p0	ba	RF_NoForcedDealloc;
 	{
 	#if defined(SGX_FEATURE_MP)
		/* Temporarily disable 3D_MEM_FREE event */
		READMASTERCONFIG(r2, #EUR_CR_MASTER_EVENT_PDS_ENABLE >> 2, drc0);
		wdf		drc0;
		and		r3, r2, ~#EUR_CR_MASTER_EVENT_PDS_ENABLE_DPM_3D_MEM_FREE_MASK;
		WRITEMASTERCONFIG(#EUR_CR_MASTER_EVENT_PDS_ENABLE >> 2, r3, r4);
	#else
		ldr		r2, #EUR_CR_EVENT_PDS_ENABLE >> 2, drc0;
		wdf		drc0;
		and		r3, r2, ~#EUR_CR_EVENT_PDS_ENABLE_DPM_3D_MEM_FREE_MASK;
		str		#EUR_CR_EVENT_PDS_ENABLE >> 2, r3;
	#endif

 		/* Free the memory */
 		str		#EUR_CR_DPM_3D_TIMEOUT >> 2, #EUR_CR_DPM_3D_TIMEOUT_NOW_MASK;
 		
 		ENTER_UNMATCHABLE_BLOCK;
 		RF_WaitForMemFree:
 		{
 			ldr		r4, #EUR_CR_EVENT_STATUS >> 2, drc0;
 			wdf		drc0;
 			shl.testns	p0, r4, #(31 - EUR_CR_EVENT_STATUS_DPM_3D_MEM_FREE_SHIFT);
 			p0 ba	RF_WaitForMemFree;
 		}
 		LEAVE_UNMATCHABLE_BLOCK;
 		
		/* Clear the event */
 		mov		r4, #EUR_CR_EVENT_HOST_CLEAR_DPM_3D_MEM_FREE_MASK;
 		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r4;

	#if defined(SGX_FEATURE_MP)
		/* Re-enable 3D_MEM_FREE event */
		WRITEMASTERCONFIG(#EUR_CR_MASTER_EVENT_PDS_ENABLE >> 2, r2, r4);
	#else
		str		#EUR_CR_EVENT_PDS_ENABLE >> 2, r2;
	#endif
 	}
 	RF_NoForcedDealloc:
#endif

#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH)
#if ! defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH_ALWAYS_SPLIT)
	/* Do not patch the Region Headers if this is the highest priority */
	MK_LOAD_STATE(r3, ui323DCurrentPriority, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testz	p0, r3, r3;
	p0	br	RF_AllMTsRendered;
#endif
	{
		/* If this is a bounding box render then we have not split the render up */
		MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRenderDetails)], drc1;
		wdf		drc1;
		MK_MEM_ACCESS(ldad)	r3, [r2, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32RenderFlags)], drc0;
#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH_ALWAYS_SPLIT)
		mov		r4, #SGXMKIF_RENDERFLAGS_BBOX_RENDER;
#else
		mov		r4, #(SGXMKIF_RENDERFLAGS_BBOX_RENDER | SGXMKIF_RENDERFLAGS_NO_MT_SPLIT);
#endif
		wdf		drc0;
		and.testnz	p0, r3, r4;
		p0	ba 	RF_AllMTsRendered;
		{
			MK_MEM_ACCESS_BPCACHE(ldad)	R_RTData, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRTData)], drc0;
			wdf		drc0;
			MK_MEM_ACCESS(ldad) 	r1, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32LastMTIdx)], drc0;
			wdf		drc1;
			MK_MEM_ACCESS(ldad)		r3, [r2, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui3CurrentMTIdx)], drc0;
			wdf		drc0;
			xor.testz	p1, r1, r3;
			p1	ba	RF_AllMTsRendered;
			{

#if (defined(FIX_HW_BRN_30089) || defined(FIX_HW_BRN_29504) || defined(FIX_HW_BRN_33753)) && defined(SGX_FEATURE_MP)
				/* if this isn't the first (neither the last) MT */
				SWITCHEDMTO3D();

				/* Restore the regions to the state they were prior to the workaround for BRN30089 */
				RESTORERGNHDR(R_RTData, r3);

				SWITCHEDMBACK();
#endif

				MK_MEM_ACCESS(ldad)	r0, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sLastRgnLUTDevAddr)], drc0;
				wdf		drc0;

				/* Remember where we got to */
				iaddu32		r3, r3.low, #1;
				RF_CheckNextMTValid:
				/* Check if the MT is valid */
				imae	r4, r3.low, #4, r0, u32;
				MK_MEM_ACCESS_CACHED(ldad)	r4, [r4], drc0;
				wdf		drc0;
				/* check if the valid is set */
				and.testnz	p0, r4, #SGXMKIF_HWRTDATA_RGNOFFSET_VALID;
				p0 ba	RF_NextMTValid;
				{
					/* This MT is invalid, if last treat as RenderFinished */
					xor.testz	p0, r1, r3;
					p0 ba	RF_AllMTsRendered;
	
					/* It was not the last MT so move on to next one */
					iaddu32		r3, r3.low, #1;
					ba	RF_CheckNextMTValid;
				}
				RF_NextMTValid:
				MK_MEM_ACCESS(stad)	[r2, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui3CurrentMTIdx)], r3;

				/* We have still got MTs to render, check if something wants us to stop */
				MK_LOAD_STATE(r0, ui32IRenderFlags, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);
	
				/* Do we need to switch */
				wdf		drc0;
				and.testz	p0, r0, #RENDER_IFLAGS_HALTRENDER;
				p0	ba	RF_NoContextSwitch;
				{
					/* We have been asked to context switch */
					and		r0, r0, ~#(RENDER_IFLAGS_HALTRENDER | RENDER_IFLAGS_FORCEHALTRENDER);
					MK_STORE_STATE(ui32IRenderFlags, r0);
	
					MK_MEM_ACCESS(ldad)	r1, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sRTStatusDevAddr)], drc0;
					wdf		drc0;
			#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
					MK_MEM_ACCESS(ldad)	R_RTAIdx, [r2, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32NextRTIdx)], drc0;
					wdf		drc0;
					imae	r1, R_RTAIdx.low, #SIZEOF(IMG_UINT32), r1, u32;
			#endif
					MK_MEM_ACCESS(ldad)	r2, [r1], drc0;
					wdf		drc0;
					or	r2, r2, #SGXMKIF_HWRTDATA_RT_STATUS_ISPCTXSWITCH;
					MK_MEM_ACCESS(stad)	[r1], r2;
	
					/* Check the queues */
					FINDTA();
					bal	Find3D;
	
					/* Exit the routine */
					ba	RF_End;
				}
				RF_NoContextSwitch:
	
#if (defined(FIX_HW_BRN_30089) || defined(FIX_HW_BRN_29504) || defined(FIX_HW_BRN_33753)) && defined(SGX_FEATURE_MP)
				SWITCHEDMTO3D();

				/* Apply the workaround */
				ADDLASTRGN(R_RTData, r3);

				SWITCHEDMBACK();
#endif

				/* Change region header base to point to next MT */
				MK_MEM_ACCESS(ldad)	r6, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32NumTileBlocksPerMT)], drc1;

				mov		r0, #EURASIA_REGIONHEADER_SIZE;
				imae	r4, r0.low, r3.low, #0, u32;
				/* as ui32NumTileBlocksPerMT is in 4x4 tiles */
				shl		r4, r4, #4;

#if defined(SGX_FEATURE_MP)
				/* count up to number of TAs*/
				mov		r5, #0;
				RF_NextTACore:
				{
					imae	r7, r5.low, #4, #OFFSET(SGXMKIF_HWRENDERDETAILS.s3DRegs.aui32ISPRgnBase[0]), u32; 
					iaddu32	r7, r7.low, r2;
					MK_MEM_ACCESS(ldad)	r7, [r7], drc1;	
					wdf		drc1;

					imae	r0, r4.low, r6.low, r7, u32;

					xor.testz	p0, r5, #0;
					p0 str		#EUR_CR_ISP_RGN_BASE >> 2, r0;
					p0 ba RF_Not0TA;
					{
						mov		r7, #((EUR_CR_MASTER_ISP_RGN_BASE1 >> 2) - 1);
						imae	r7, r5.low, #1, r7, u32;
						str		r7, r0;
					}
					RF_Not0TA:
	
					MK_LOAD_STATE_CORE_COUNT_TA(r7, drc0);
					iaddu32		r5, r5.low, #1;
					MK_WAIT_FOR_STATE_LOAD(drc0);

					xor.testnz	p0, r5, r7;
					p0	ba	RF_NextTACore;
				}
#else
				MK_MEM_ACCESS(ldad)	r5, [r2, #DOFFSET(SGXMKIF_HWRENDERDETAILS.s3DRegs.aui32ISPRgnBase[0])], drc1;
				wdf		drc1;
				imae	r4, r4.low, r6.low, r5, u32;
	
				str		#EUR_CR_ISP_RGN_BASE >> 2, r4;
#endif /* SGX_FEATURE_MP */
	
				MK_MEM_ACCESS(ldad)	r5, [r2, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32RenderFlags)], drc0;
				MK_LOAD_STATE(r4, ui32IRenderFlags, drc1);
				wdf		drc0;
	
				/* Set DPM dealloc bits */
				and.testnz p0, r5, #SGXMKIF_RENDERFLAGS_NODEALLOC;
				MK_WAIT_FOR_STATE_LOAD(drc1);
				p0	ba		RF_NoDealloc;
				{
					/* Only dealloc global if last MT */
					xor.testz	p1, r1, r3;
					/* Flag the events that need to happen for the render to be finished. */
					or		r4, r4, #RENDER_IFLAGS_WAITINGFOREOR | RENDER_IFLAGS_WAITINGFOR3DFREE | \
									RENDER_IFLAGS_RENDERINPROGRESS;
					!p1	mov		r3, #EUR_CR_DPM_3D_DEALLOCATE_ENABLE_MASK;
					p1	mov		r3, #EUR_CR_DPM_3D_DEALLOCATE_GLOBAL_MASK | EUR_CR_DPM_3D_DEALLOCATE_ENABLE_MASK;
	
					ba		RF_DeallocSet;
				}
				RF_NoDealloc:
				{
					/* Disable DPM deallocation. */
					mov	r3, #0;
					/* Flag the events that need to happen for the render to be finished. */
					or		r4, r4, #RENDER_IFLAGS_WAITINGFOREOR | RENDER_IFLAGS_RENDERINPROGRESS;
				}
				RF_DeallocSet:
				str		#EUR_CR_DPM_3D_DEALLOCATE >> 2, r3;
				MK_STORE_STATE(ui32IRenderFlags, r4);
	
		#if defined(SUPPORT_HW_RECOVERY)
				/* reset the Recovery counter */
				MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRTimerTicksSince3D)], #0;
	
				/* invalidate the signatures */
				MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], drc0;
				wdf		drc0;
				or		r2, r2, #0x2;
				MK_MEM_ACCESS_BPCACHE(stad) 	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], r2;
		#endif
				idf		drc0, st;
				wdf		drc0;
	
#if defined(SGX_FEATURE_MP)
#if defined(FIX_HW_BRN_30710) || defined(FIX_HW_BRN_30764)
				SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_SOFT_RESET >> 2, #EUR_CR_MASTER_SOFT_RESET_IPF_RESET_MASK);
				SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_SOFT_RESET >> 2, #0);
#endif
#endif /* SGX_FEATURE_MP */
#if defined(FIX_HW_BRN_31964)
				/* Reset the ISP2 in case some cores have no work */
				SGXMK_SPRV_WRITE_REG(#EUR_CR_SOFT_RESET >> 2, #EUR_CR_SOFT_RESET_ISP2_RESET_MASK);
				SGXMK_SPRV_WRITE_REG(#EUR_CR_SOFT_RESET >> 2, #0);
#endif

#if defined(SGX_FEATURE_MP)
				MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32MTRegionArraySize)], drc0;
				wdf drc0;
				WRITEMASTERCONFIG(#EUR_CR_MASTER_ISP_RGN >> 2, r2, r10);
#endif

				MK_TRACE(MKTC_RF_START_NEXT_MT, MKTF_3D | MKTF_HW);

				ldr		r10, #EUR_CR_DPM_PAGE >> 2, drc0;
				wdf		drc0;
				and		r10, r10, #EUR_CR_DPM_PAGE_STATUS_3D_MASK;
				shr		r10, r10, #EUR_CR_DPM_PAGE_STATUS_3D_SHIFT;
	
				WRITEHWPERFCB(3D, 3D_START, GRAPHICS, r10, p0, k3d_start_mt, r2, r3, r4, r5, r6, r7, r8, r9);
	
				str		#EUR_CR_ISP_START_RENDER >> 2, #EUR_CR_ISP_START_RENDER_PULSE_MASK;
	
				ba	RF_End;
			}
		}
	}
	RF_AllMTsRendered:

	MK_TRACE(MKTC_RF_ALL_MTS_DONE, MKTF_3D | MKTF_SCH);

	/* Make sure we clear the HALTRENDER flag as it may have been set */
	MK_LOAD_STATE(r0, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and		r0, r0, ~#RENDER_IFLAGS_HALTRENDER;
	MK_STORE_STATE(ui32IRenderFlags, r0);
#endif

#if (defined(FIX_HW_BRN_30089) || defined(FIX_HW_BRN_29504) || defined(FIX_HW_BRN_33753)) && defined(SGX_FEATURE_MP)
	/* Restore the regions to the state they were prior to the workaround for BRN30089 */
	MK_MEM_ACCESS_BPCACHE(ldad)	R_RTData, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRTData)], drc0;
	wdf		drc0;
	SWITCHEDMTO3D();
	RESTORERGNHDR(R_RTData, #EUR_CR_DPM_ABORT_STATUS_MTILE_GLOBAL_MASK);
	SWITCHEDMBACK();
#endif

#if defined(PDUMP) && defined(EUR_CR_SIM_3D_FRAME_COUNT)
	/* Tell the simulator that this was a main render complete and not a partial render */
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui323DFrameNum)], drc0;
	wdf		drc0;
	str		#EUR_CR_SIM_3D_FRAME_COUNT >> 2, r2;
#endif

	/* load the s3DRenderDetails and s3DRTData */
	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRenderDetails)], drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRTData)], drc1;
	wdf		drc0;
	wdf		drc1;
	
#if defined(SGX_FEATURE_MP)
	/* Have we been requested to flush the SLC after the render */
	MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32RenderFlags)], drc0;
	wdf		drc0;
	mov		r3, #SGXMKIF_RENDERFLAGS_FLUSH_SLC;
	and.testz	p1, r2, r3;
	p1	ba	RF_NoSLCFlush;
	{
		/* We have been requested to flush the SLC */	
		FLUSH_SYSTEM_CACHE_INLINE(p1, r2, #EUR_CR_MASTER_SLC_CTRL_FLUSH_DM_PIXEL_MASK, r3, RF);
	}
	RF_NoSLCFlush:
#endif
	
	/* Update last op complete values */
	bal		UpdateRenderStatusValues;

	/* Remove the current render context from the head of the complete render contexts list */
	MK_LOAD_STATE(r1, ui323DCurrentPriority, drc0);
	MK_LOAD_STATE(r0, s3DContext, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	
	MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextCompleteDevAddr)], drc0;
	wdf		drc0;
	and.testz	p0, r4, r4;
	/* Update the Head */
	MK_STORE_STATE_INDEXED(sCompleteRenderContextHead, r1, r4, r2);

	!p0	ba	RF_ListNotEmpty;
	{
		MK_STORE_STATE_INDEXED(sCompleteRenderContextTail, r1, #0, r3);
	}
	RF_ListNotEmpty:
	!p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevCompleteDevAddr)], #0;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextCompleteDevAddr)], #0;
	idf		drc0, st;
	wdf		drc0;

	/* Remove the just completed render from the head of the complete renders list */
	MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWCompleteRendersHead)], drc0;
	wdf		drc0;
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	/* Ony remove the HWRenderDetails if there are no more renders in the array,
		if value is 0 all renders have been completed and ui32NextRTIdx reset */
	MK_MEM_ACCESS(ldad)	r5, [r4, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32NextRTIdx)], drc0;
	wdf		drc0;
	and.testz	p1, r5, r5;
	!p1	ba	RF_RendersLeftInArray;
#endif
	{
		/* Remove the render details from the sHWCompleteRender list */
		MK_MEM_ACCESS(ldad)	r5, [r4, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sNextDevAddr)], drc0;
		wdf		drc0;
		and.testz	p1, r5, r5;
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWCompleteRendersHead)], r5;
		p1	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWCompleteRendersTail)], #0;
		!p1	MK_MEM_ACCESS(stad)	[r5, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sPrevDevAddr)], #0;
		MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sNextDevAddr)], #0;
		idf		drc0, st;
		wdf		drc0;
	}
	RF_RendersLeftInArray:

	/* If there are more renders waiting for this context, then add the context back to the tail of the complete render context list */
	p1	ba	RF_RenderListEmpty;
	{
		/* Get the Tail */
		MK_LOAD_STATE_INDEXED(r4, sCompleteRenderContextTail, r1, drc0, r3);
		MK_WAIT_FOR_STATE_LOAD(drc0);

		!p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextCompleteDevAddr)], r0;
		/* If list is empty update the Head */
		!p0	ba	RF_RLNE_NoHeadUpdate;
		{
			MK_STORE_STATE_INDEXED(sCompleteRenderContextHead, r1, r0, r2);
		}
		RF_RLNE_NoHeadUpdate:
		/* Update the Tail */
		MK_STORE_STATE_INDEXED(sCompleteRenderContextTail, r1, r0, r3);
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevCompleteDevAddr)], r4;
		idf		drc0, st;
		wdf		drc0;
	}
	RF_RenderListEmpty:
	
	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRenderDetails)], drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRTData)], drc0;
	wdf		drc0;
	bal 	ReleaseRenderResources;

#if defined(SUPPORT_HW_RECOVERY)
	/* check if the TA had a potential lockup, if so indicate an idle occurred */
	MK_LOAD_STATE(r1, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	mov		r0, #TA_IFLAGS_HWLOCKUP_MASK;
	and		r2, r1, r0;
	mov		r0, #TA_IFLAGS_HWLOCKUP_DETECTED;
	xor.testnz	p1, r2, r0;
	p1	ba	RF_NoTALockup;
	{
		mov		r0, #TA_IFLAGS_HWLOCKUP_IDLE;
		or		r1, r1, r0;
		MK_STORE_STATE(ui32ITAFlags, r1);
		MK_WAIT_FOR_STATE_STORE(drc0);
	}
	RF_NoTALockup:
#endif

	/* Check if the TA is waiting to start a partial render. */
	MK_LOAD_STATE(r0, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	shl.testns		p0, r0, #(31 - TA_IFLAGS_WAITINGTOSTARTCSRENDER_BIT);
	p0	ba				RF_NoSPMRenderWaiting;
	{
		MK_TRACE(MKTC_SPM_RESUME_ABORTCOMPLETE, MKTF_3D | MKTF_SCH);
		/*
			Start the partial render now.
			#####################################################
			NOTE: Returns directly to our caller.
			#####################################################
		*/
		/* Emit the loopback */
		TATERMINATE();
		ba	RF_End;
	}
	RF_NoSPMRenderWaiting:

	/* Check for TA commands that can be started */
	FINDTA();
#if defined(SGX_FEATURE_2D_HARDWARE)
	FIND2D();
#endif

	bal	Find3D;

	RF_End:
	MK_TRACE(MKTC_RENDERFINISHED_END, MKTF_3D);

	/* branch back to calling code */
	mov		pclink, R_EDM_PCLink;
	lapc;
}

/*****************************************************************************
 Find3D routine	- check to see if there is a render or transfer task ready
 					to be executed

 inputs:
 temps:		r0
			r1
			r2
			r3
			r4
*****************************************************************************/
Find3D:
{
	/* Make sure it's OK to start a new operation now */
	MK_LOAD_STATE(r0, ui32HostRequest, drc0);
	MK_LOAD_STATE(r1, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	/*
		If any of the following are true don't try to start anything
		1) the chip has been requested to power off
		2) there is a ISP context switching pending
		3) there is a SPM render in progress
		4) the TA is trying to do context free but 3D is blocking the memory context switch
	*/
	and.testz	p0, r0, #SGX_UKERNEL_HOSTREQUEST_POWER;
	p0 ba 		F3D_NoPowerOff;
	{
		MK_TRACE(MKTC_FIND3D_POWERREQUEST, MKTF_3D | MKTF_SCH | MKTF_POW);
		ba			F3D_End;
	}
	F3D_NoPowerOff:
	and.testnz	p0, r1, #(RENDER_IFLAGS_SPMRENDER | RENDER_IFLAGS_HALTRENDER);
#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	/* TA is trying to context free, only 1 memory context so must block 3D */
	!p0	shl.tests	p0, r1, #(31 - RENDER_IFLAGS_DEFERREDCONTEXTFREE_BIT);
#endif

	p0	ba	F3D_End;
	{
		and.testnz	p0, r1, #RENDER_IFLAGS_RENDERINPROGRESS;
#if !defined(SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH)
		/*
			we can only interrupt normal renders, so if there is a fast render
			in progress we must wait for it to finish.
		*/
		!p0	ba	F3D_NoOperationInProgress;
		{
			and.testnz	p1, r1, #RENDER_IFLAGS_TRANSFERRENDER;
			p1	ba	F3D_End;
		}
		F3D_NoOperationInProgress:
#endif
 
#if defined(FIX_HW_BRN_28889)
		MK_LOAD_STATE(r0, ui32CacheCtrl, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and.testz	p1, r0, r0;
		p1	ba	F3D_NoCacheRequestPending;
		{
			/* If the 3D is active then don't do the Invalidate but we can check the runlists */
			p0	br	F3D_NoCacheRequestPending;
			
			/* There is a request pending so checking if the TA is running */
			MK_LOAD_STATE(r2, ui32ITAFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			and.testnz		p1, r2, #TA_IFLAGS_TAINPROGRESS;
			/* If the TA has gone out of memoy and the partial render is 
				blocked, the TA is idle and	we can therefore do the BIF invalidate */
			p1	shl.testns	p1, r2, #(31 - TA_IFLAGS_OOM_PR_BLOCKED_BIT);
			p1	br	F3D_End;
			/* The TA must not be running so do the invalidate */
			DOBIFINVAL(F3D_, p1, r0, drc0);
			/* Re-set the predicate as the invalidate could have changed it */
			and.testnz	p0, r1, #RENDER_IFLAGS_RENDERINPROGRESS;
		}
		F3D_NoCacheRequestPending:
#else
	#if defined(USE_SUPPORT_NO_TA3D_OVERLAP)
		/* Don't allow overlapping */
		MK_LOAD_STATE(r0, ui32IRenderFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and.testz	p1, r0, #RENDER_IFLAGS_TAWAITINGFORMEM;
		p1 ba F3D_OverlapNoOperationFound;
		mov r4, #RENDER_IFLAGS_PARTIAL_RENDER_OUTOFORDER;
		and.testz	p1, r0, r4;
		p1 ba F3D_OverlapNoOperationFound;
		/* Proceed with FIND3D, this is because, we can
		 *not partially render due to ordering issues.
		 */
		ba	F3D_Proceed;
		{
			F3D_OverlapNoOperationFound:
			MK_LOAD_STATE(r0, ui32ITAFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			and.testnz		p1, r0, #TA_IFLAGS_TAINPROGRESS;
			p1	ba	F3D_End;
		}
		F3D_Proceed:
	#endif
#endif

		/* Start at priority level 0 (highest) */
		mov		r1, #0;
		F3D_CheckNextPriorityLevel:
		{
			/* If a 3D operations is in progress, check if its priority is higher */
			!p0	ba F3D_NoActiveContextCheck;
			{
				MK_LOAD_STATE(r2, ui323DCurrentPriority, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);
				/* only proceed if the run-list priority is higher than the current context */
				isub16.testns	p1, r1, r2;
				p1 ba F3D_End;
				/* Run-list is a higher priority, start checking it */
			}
			F3D_NoActiveContextCheck:

			/* Look for a transfer first */
			mov		r0, pclink;
			bal 	CheckTransferRunList;

			/* Look for a render second */
			bal		CheckRenderRunlist;
			mov		pclink, r0;

			/* If we get here no 3D operations are ready at the current priority level */
			/* Check the next priority level */
			MK_LOAD_STATE(r2, ui32BlockedPriority, drc0);
			iaddu32	r1, r1.low, #1;
			MK_WAIT_FOR_STATE_LOAD(drc0);
			/* Make sure we only check priorities >= the blocked context, when nothing is blocked
				ui32BlockedPriority contains the lowest level */
			isub16.tests	p1, r2, r1;
			!p1	ba	F3D_CheckNextPriorityLevel;

			/* if both TAWAITINGFORMEM and PARTIAL_RENDER_OUTOFORDER flags are set, check all
               remaining priorities. */
			MK_LOAD_STATE(r3, ui32IRenderFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			and.testz	p1, r3, #RENDER_IFLAGS_TAWAITINGFORMEM;
			p1 ba F3D_NoOperationFound;
			mov r4, #RENDER_IFLAGS_PARTIAL_RENDER_OUTOFORDER;
			and.testz	p1, r3, r4;
			p1 ba F3D_NoOperationFound;
			mov r2, #(SGX_MAX_CONTEXT_PRIORITY - 1);
			isub16.tests	p1, r2, r1;
			!p1	ba	F3D_CheckNextPriorityLevel;
		}
		F3D_NoOperationFound:

		/*
			Indicate to the Active Power Manager that no 3D operations are ready to go.
		*/
		CLEARACTIVEPOWERFLAGS(#SGX_UKERNEL_APFLAGS_3D);
	}
	F3D_End:
	/* branch back to calling code */
	lapc;
}

/*****************************************************************************
 CheckRenderRunlist routine	- check to see if there is a render task ready to be
								executed at the given priority level

 inputs:	p0 - true if there is an operation in progress
 			r0 - return address if a render is found
			r1 - the priority level to check
 temps:		p2, r2, r3, r4, r5, r6
*****************************************************************************/
CheckRenderRunlist:
{
	/* Get the head of the run list for this priority */
	MK_LOAD_STATE_INDEXED(r3, sCompleteRenderContextHead, r1, drc0, r2);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	CRRL_CheckNextContext:
	{
		and.testz	p1, r3, r3;
		!p1	ba		CRRL_NextContext;
		{
			/* We have check all the run-list and not found anything ready */
			/* Return to Find3D routine */
			lapc;
		}
		CRRL_NextContext:
		/*
			Only check if we can execute the first render in the list as we need to do
			the renders in the order they were submitted
		*/
		MK_MEM_ACCESS(ldad)	r2, [r3, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sCommon.ui32Flags)], drc0;
		MK_MEM_ACCESS(ldad)	r4, [r3, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWCompleteRendersHead)], drc0;
		wdf		drc0;
		
		and.testz p2, r2, #SGXMKIF_HWCFLAGS_SUSPEND;
		p2 ba CRRL_ContextNotSuspended;
		{
			MK_MEM_ACCESS(ldad)	r5, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderContext)], drc0;
			wdf drc0;
			xor.testnz p2, r5, r3; /* p2 = (r3 != r5) */
			p2 ba CRRL_DoCantKickRender;

			MK_LOAD_STATE(r5, ui32ITAFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			mov r6, #TA_IFLAGS_OUTOFMEM;
			and.testz p2, r5, r6; /* p2 = !(ui32ITAFlags & TA_IFLAGS_OUTOFMEM) */
			p2 ba CRRL_DoCantKickRender;

			ba CRRL_ContextNotSuspended;

			CRRL_DoCantKickRender:
			MK_TRACE(MKTC_CRRL_CONTEXT_SUSPENDED, MKTF_3D | MKTF_SCH | MKTF_RC);
			MK_TRACE_REG(r3, MKTF_3D | MKTF_RC);
			ba		CRRL_CantKickRender;
		}
		CRRL_ContextNotSuspended:

		/* retreive the HWRTData */
		MK_MEM_ACCESS(ldad)	R_RTData, [r4, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWRTDataDevAddr)], drc0;
		wdf		drc0;

#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
		/* Now offset to the next RT to be rendered */
		MK_MEM_ACCESS(ldad)	R_RTAIdx, [r4, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32NextRTIdx)], drc1;
		wdf		drc1;
#endif
#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH) || defined(SGX_FEATURE_ISP_CONTEXT_SWITCH)
		/* If the render was context switched there is no need to check the dependencies */
		MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sRTStatusDevAddr)], drc0;
		wdf		drc0;
	#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
		imae	r2, R_RTAIdx.low, #SIZEOF(IMG_UINT32), r2, u32;
	#endif
		MK_MEM_ACCESS(ldad)	r2, [r2], drc0;
		wdf		drc0;
		and.testnz	p1, r2, #SGXMKIF_HWRTDATA_RT_STATUS_ISPCTXSWITCH;
		p1	ba	CRRL_FoundRender;
#endif
		{
			MK_MEM_ACCESS(ldad)	r5, [r4, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWDstSyncListDevAddr)], drc0;
			/* Calculate the Address of the first PVRSRV_DEVICE_SYNC_OBJECT */
			mov		r6, #OFFSET(SGXMKIF_HWDEVICE_SYNC_LIST.asSyncData[0]);
			wdf		drc0;
			and.testz	p1, r5, r5;
			p1	ba		CRRL_NoDstSyncObject;
			{
				iaddu32	r5, r6.low, r5;
			#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
				imae	r5, R_RTAIdx.low, #SIZEOF(PVRSRV_DEVICE_SYNC_OBJECT), r5, u32;
			#endif
				MK_MEM_ACCESS(ldad.f2)	r6, [r5, #DOFFSET(PVRSRV_DEVICE_SYNC_OBJECT.ui32WriteOpsPendingVal)-1], drc0;
				MK_MEM_ACCESS(ldad.f2)	r8, [r5, #DOFFSET(PVRSRV_DEVICE_SYNC_OBJECT.ui32ReadOps2PendingVal)-1], drc1;
				wdf		drc0;
				and.testz	p1, r7, r7;
				p1	ba		CRRL_WriteOpsNotBlocked;
				{
					MK_MEM_ACCESS_BPCACHE(ldad)	r10, [r7], drc0;
					wdf		drc0;
					xor.testz	p1, r10, r6;
					p1	ba		CRRL_WriteOpsNotBlocked;
					{
						MK_TRACE(MKTC_CRRL_WRITEOPSBLOCKED, MKTF_3D | MKTF_SO | MKTF_SCH);
						MK_TRACE_REG(r7, MKTF_3D | MKTF_SO);
						MK_TRACE_REG(r10, MKTF_3D | MKTF_SO);
						MK_TRACE_REG(r6, MKTF_3D | MKTF_SO);
						/* make sure we wait for the ui32ReadOpsPendingVal pair */
						wdf		drc1;
						ba		CRRL_CantKickRender;
					}
				}
				CRRL_WriteOpsNotBlocked:
				/*
					We check the first element in the structure last as we can't do -1
					to workaround the pre-increment we do ++ instead which will
					post increment r5
				*/
				MK_MEM_ACCESS(ldad.f2)	r6, [r5, #DOFFSET(PVRSRV_DEVICE_SYNC_OBJECT.ui32ReadOpsPendingVal)++], drc0;
				/* wait for the ReadOps2 data pair */
				wdf		drc1;
				and.testz	p1, r9, r9;
				p1	ba		CRRL_ReadOps2NotBlocked;
				{
					MK_MEM_ACCESS_BPCACHE(ldad)	r10, [r9], drc0;
					wdf		drc0;
					xor.testz	p1, r10, r8;					
					p1	ba		CRRL_ReadOps2NotBlocked;
					{
						MK_TRACE(MKTC_CRRL_READOPS2BLOCKED, MKTF_3D | MKTF_SO | MKTF_SCH);
						MK_TRACE_REG(r9, MKTF_3D | MKTF_SO);
						MK_TRACE_REG(r10, MKTF_3D | MKTF_SO);
						MK_TRACE_REG(r8, MKTF_3D | MKTF_SO);
						/* make sure we wait for the ui32ReadOpsPendingVal pair */
						wdf		drc0;
						ba		CRRL_CantKickRender;
					}
				}
				CRRL_ReadOps2NotBlocked:
				/* wait for the ReadOps data pair */
				wdf		drc0;
				and.testz	p1, r7, r7;
				p1	ba		CRRL_ReadOpsNotBlocked;
				{
					MK_MEM_ACCESS_BPCACHE(ldad)	r10, [r7], drc0;
					wdf		drc0;
					xor.testz	p1, r10, r6;					
					p1	ba		CRRL_ReadOpsNotBlocked;
					{
						MK_TRACE(MKTC_CRRL_READOPSBLOCKED, MKTF_3D | MKTF_SO | MKTF_SCH);
						MK_TRACE_REG(r7, MKTF_3D | MKTF_SO);
						MK_TRACE_REG(r10, MKTF_3D | MKTF_SO);
						MK_TRACE_REG(r6, MKTF_3D | MKTF_SO);
						ba		CRRL_CantKickRender;
					}
				}
				CRRL_ReadOpsNotBlocked:
			}
			CRRL_NoDstSyncObject:

			/* check the TQ/3D synchronization */
			MK_MEM_ACCESS(ldad.f4)	r5, [r4, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32TQSyncWriteOpsPendingVal)-1], drc0;
			wdf		drc0;
			and.testz	p1, r6, r6;
			p1	ba	CRRL_TQSyncOpReady;
			{
				/* Check this is ready to go */
				MK_MEM_ACCESS_BPCACHE(ldad)	r9, [r6], drc0; 	// load WriteOpsComplete
				MK_MEM_ACCESS_BPCACHE(ldad)	r10, [r8], drc0;	// load ReadOpsComplete
				wdf		drc0;
				xor.testz	p1, r9, r5;	// Test WriteOpsComplete
				p1 xor.testz	p1, r10, r7; 	// Test ReadOpsComplete
				!p1	ba	CRRL_CantKickRender;
			}
			CRRL_TQSyncOpReady:

			/* check the source dependencies, first get the count */
#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)
			MK_MEM_ACCESS(ldad) r2, [r4, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32Num3DSrcSyncs)], drc0;
#else
			MK_MEM_ACCESS(ldad) r2, [r4, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32NumSrcSyncs)], drc0;
#endif			
			wdf		drc0;
			and.testz	p1, r2, r2;
			p1	ba	CRRL_NoSrcSyncs;
			{
				/* get the base address of the first block + offset to ui32ReadOpsPendingVal */
#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)
				mov		r5, #OFFSET(SGXMKIF_HWRENDERDETAILS.as3DSrcSyncs[0].ui32ReadOpsPendingVal);
#else
				mov		r5, #OFFSET(SGXMKIF_HWRENDERDETAILS.asSrcSyncs[0].ui32ReadOpsPendingVal);
#endif
				iadd32	r5, r5.low, r4;

				/* loop through each dependency, loading a PVRSRV_DEVICE_SYNC_OBJECT structure */
				/* load ui32ReadOpsPendingVal and sReadOpsCompleteDevVAddr */
				MK_MEM_ACCESS(ldad.f4)	r6, [r5, #0++], drc0;
				CRRL_CheckNextSrcDependency:
				{
					wdf	drc0;

					/* Check this is ready to go */
					MK_MEM_ACCESS_BPCACHE(ldad)	r10, [r7], drc0; // load ReadOpsComplete
					wdf		drc0;
					xor.testz	p1, r10, r6;	// Test ReadOpsComplete
					!p1	ba	CRRL_CantKickRender;

					MK_MEM_ACCESS_BPCACHE(ldad)	r10, [r9], drc0;	// load WriteOpsComplete
					wdf		drc0;
					xor.testz	p1, r10, r8; 	// Test WriteOpsComplete
					!p1	ba	CRRL_CantKickRender;

					/* Move past readops2 */
					mov	r6, #8;
					mov	r7, r5;
					IADD32(r5, r7, r6, r8);
					/* decrement the count and test for 0 */
					isub16.testz r2, p1, r2, #1;
					!p1	MK_MEM_ACCESS(ldad.f4)	r6, [r5, #0++], drc0;
					!p1	ba	CRRL_CheckNextSrcDependency;
				}
			}
			CRRL_NoSrcSyncs:

			/* If we've reached here this render must be ready to go */
			ba	CRRL_FoundRender;
			{
				CRRL_CantKickRender:
				MK_TRACE(MKTC_CRRL_BLOCKEDRC, MKTF_3D | MKTF_RC);
				MK_TRACE_REG(r3, MKTF_3D | MKTF_RC);
				MK_TRACE(MKTC_CRRL_BLOCKEDRTDATA, MKTF_3D | MKTF_RT);
				MK_TRACE_REG(R_RTData, MKTF_3D | MKTF_RT);
				
				/* Load next render context address and loop round again */
				MK_MEM_ACCESS(ldad)	r3, [r3, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextCompleteDevAddr)], drc0;
				wdf		drc0;
				ba		CRRL_CheckNextContext;
			}
		}
	}
	CRRL_FoundRender:
	MK_TRACE(MKTC_CRRL_FOUNDRENDER, MKTF_3D | MKTF_SCH);
	/* This routine will kick a render or context switch so don't return to Find3D */
	mov	pclink, r0;
	mov	r0, r3;	/* Move Context to r0 */
	mov	r2, r1;	/* Move priority to r2 */
	mov	r1, r4;	/* Move renderdetails to r1 */

#if (defined(FIX_HW_BRN_25910) && defined(SUPPORT_PERCONTEXT_PB)) || !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	/* Check if the TA is active if so make sure we only parallelise a single context */
	MK_LOAD_STATE(r3, ui32ITAFlags, drc0);
#if defined(SGX_FEATURE_2D_HARDWARE)
	MK_LOAD_STATE(r4, ui32I2DFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	/* All modules use the same bit to indicate 'busy' */
	or		r3, r3, r4;
#else
	MK_WAIT_FOR_STATE_LOAD(drc0);
#endif
	and.testz	p1, r3, #TA_IFLAGS_TAINPROGRESS;
	p1	ba	CRRL_NoOperationInProgress;
	{
		/* Now load up the PDPhys and DirListBase0 */
	#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
		ldr			r3, #EUR_CR_BIF_BANK0 >> 2, drc0;
		wdf			drc0;
		and			r3, r3, #EUR_CR_BIF_BANK0_INDEX_TA_MASK;
		shr			r3, r3, #EUR_CR_BIF_BANK0_INDEX_TA_SHIFT;
		and.testz	p1, r3, r3;
		p1	mov 	r3, #(EUR_CR_BIF_DIR_LIST_BASE0 >> 2);
		p1	ba	CRRL_TAUsingDirListBaseZero;
		{
			mov	r4, #((EUR_CR_BIF_DIR_LIST_BASE1 >> 2)-1);
			iaddu32	r3, r3.low, r4;
		}
		CRRL_TAUsingDirListBaseZero:
		ldr	r3, r3, drc0;
	#else
		ldr		r3, #EUR_CR_BIF_DIR_LIST_BASE0 >> 2, drc0;
	#endif /* SGX_FEATURE_MULTIPLE_MEM_CONTEXTS */
		SGXMK_HWCONTEXT_GETDEVPADDR(r4, r0, r5, drc0);

		xor.testnz	p1, r4, r3;
		!p1	ba	CRRL_SameContext;
		{
			MK_TRACE(MKTC_CRRL_TARC_DIFFERENT, MKTF_3D | MKTF_SCH);

			MK_LOAD_STATE(r4, ui32IRenderFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			and.testz	p2, r4, #RENDER_IFLAGS_TAWAITINGFORMEM;
			p2 ba CRRL_TARCDiffNoOOMWaiting;
			mov r6, #RENDER_IFLAGS_PARTIAL_RENDER_OUTOFORDER;
			and.testz	p2, r4, r6;
			p2 ba CRRL_TARCDiffNoOOMWaiting;
			{	
				/* Load next render context address and loop round again */
				MK_MEM_ACCESS(ldad)	r3, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextCompleteDevAddr)], drc0;
				mov 	r0, pclink;
				mov		r1, r2;
				wdf		drc0;
				ba		CRRL_CheckNextContext;
			}
			CRRL_TARCDiffNoOOMWaiting:

	#if defined(SGX_FEATURE_2D_HARDWARE)
			/* Record the blocked priority, so nothing of lower priority can be started.
				NOTE: Calling code guarantees this context is >= to current blocked level. */
			MK_STORE_STATE(ui32BlockedPriority, r2);
			MK_WAIT_FOR_STATE_STORE(drc0);
				
			/* If the TA is not busy, the 2D must be! 
				Given there is nothing we can do to stop it, just exit */
			MK_LOAD_STATE(r4, ui32ITAFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			and.testz	p1, r4, #TA_IFLAGS_TAINPROGRESS;
			p1	ba	CTRL_End;
	#endif
			/* The memory contexts are different so only proceed if the render is of a higher priority */
			MK_LOAD_STATE(r3, ui32TACurrentPriority, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);

			isub16.testns	p1, r2, r3;
			p1 ba CRRL_End;
			/* The render is higher priority so request the TA to stop */
			/* Setting the nextContext to 0, indicates the 3D is waiting */
			STARTTACONTEXTSWITCH(FR);
			ba	CRRL_End;
		}
		CRRL_SameContext:

	#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
		/* is the context already loaded */
		/* retreive the HWRTData */
		MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext0RTData)], drc0;
		MK_MEM_ACCESS_BPCACHE(ldad)	r4, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext1RTData)], drc1;
		wdf				drc0;
		xor.testz	p1, R_RTData, r3;
		wdf		drc1;
		!p1 xor.testz	p1, R_RTData, r4;
		#if defined(FIX_HW_BRN_23378) && !defined(EUR_CR_TE_DIAG5)
		/* if it is not loaded, exit */
		!p1 ba CRRL_End;
		#else
		/* if it is not loaded, is there one free */
		!p1 and.testz	p1, r3, r4;
		p1	ba CRRL_RTDataLoaded;
		{
			/* It is not loaded and there is not a free context,
				so check if the one not in use is on the same address space
				and can be stored safely */
			ldr		r5, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc0;
			wdf		drc0;
			and		r5, r5, #EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_MASK;
			and.testnz	p1, r5, r5;
			p1	MK_MEM_ACCESS(ldad)	r3, [r3, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderContextDevAddr)], drc0;
			!p1	MK_MEM_ACCESS(ldad)	r3, [r4, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderContextDevAddr)], drc0;
			wdf		drc0;
			ldr		r4, #EUR_CR_BIF_DIR_LIST_BASE0 >> 2, drc0;
			/* note: the wdf follows in SGXMK_HWCONTEXT_GETDEVPADDR */
			SGXMK_HWCONTEXT_GETDEVPADDR(r5, r3, r3, drc0);

			xor.testnz	p1, r5, r4;
			p1 ba	CRRL_End;
		}
		CRRL_RTDataLoaded:
		#endif
	#endif
	}
	CRRL_NoOperationInProgress:
#endif /* ((defined(FIX_HW_BRN_25910) && defined(SUPPORT_PERCONTEXT_PB) || !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)) */

	/* If we are currently rendering, perform a context switch */
	p0	ba 	CRRL_No_KickRender;
	{
		/* check if this is actually meant to be rendered or just dummy processed */
		MK_MEM_ACCESS(ldad)	r3, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc0;
		MK_MEM_ACCESS(ldad)	r4, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32RenderFlags)], drc1;
		wdf		drc0;
		and		r3, r3, #SGXMKIF_HWRTDATA_CMN_STATUS_DUMMYPROC;
		wdf		drc1;
		and		r4, r4, #(SGXMKIF_RENDERFLAGS_NORENDER | SGXMKIF_RENDERFLAGS_ABORT);
		/* if either of the flags are set then dummy process */
		or.testnz	p1, r4, r3;

		{/* temps: p2 r3 r4 r5 */
			MK_LOAD_STATE(r4, ui32IRenderFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			and.testz	p2, r4, #RENDER_IFLAGS_TAWAITINGFORMEM;
			p2 ba CRRL_NoOOMWaiting;
			mov r5, #RENDER_IFLAGS_PARTIAL_RENDER_OUTOFORDER;
			and.testz	p2, r4, r5;
			p2 ba CRRL_NoOOMWaiting;
   			/* priority of the context that went out of mem is >= the priority contained in r2 */
			MK_LOAD_STATE(r3, ui32TACurrentPriority, drc0);
			wdf drc0;
			isub16.testn	p2, r2, r3;
			p2 ba CRRL_PrioInversion;
		}
		CRRL_OOMWaiting:
		{
			MK_TRACE(MKTC_CRRL_TAWAITINGFORMEM , MKTF_SPM | MKTF_SCH);

			MK_MEM_ACCESS(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderContext)], drc0;
			wdf drc0;	

			MK_MEM_ACCESS(ldad) r1, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWCompleteRendersHead)], drc0;
			wdf drc0;

			MK_MEM_ACCESS(ldad) r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sCommon.ui32Priority)], drc0;
			wdf drc0;

			MK_MEM_ACCESS(ldad)	R_RTData, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWRTDataDevAddr)], drc0;
			wdf drc0;

			#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
			/* Now offset to the next RT to be rendered */
			MK_MEM_ACCESS(ldad)	R_RTAIdx, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32NextRTIdx)], drc0;
			wdf drc0;
			#endif

			!p1 ba KickRender;	
			ba CRRL_DummyProcess;
		}
		CRRL_PrioInversion:
		MK_TRACE(MKTC_CRRL_TAOOMBUTPRIOINV, MKTF_SPM | MKTF_SCH);
		MK_TRACE_REG(r2, MKTF_SPM | MKTF_SCH); /* prio of ctxt found by CRRL */
		MK_TRACE_REG(r3, MKTF_SPM | MKTF_SCH); /* prio of out of mem ctxt */

		CRRL_NoOOMWaiting:

		/* KickRender will branch straight back to calling code, if called */
		!p1	ba	KickRender;
		CRRL_DummyProcess:
		{
			/* if we get to here then we are going to dummy process the render */
			MK_TRACE(MKTC_CRRL_NORENDER, MKTF_3D | MKTF_SCH);
			
			/* Reset the priority scheduling level block, as it is no longer blocked */
			MK_STORE_STATE(ui32BlockedPriority, #(SGX_MAX_CONTEXT_PRIORITY - 1));

			WRITEHWPERFCB(3D, MK_3D_DUMMY_START, GRAPHICS, #0, p0, k3d_dummy_1, r10, r3, r4, r5, r6, r7, r8, r9);
			/* this render was aborted, therefore do the minimum */
			/* Remove the current render context from the list of the complete render contexts list */
			MK_MEM_ACCESS(ldad)	r5, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevCompleteDevAddr)], drc0;
			MK_MEM_ACCESS(ldad)	r6, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextCompleteDevAddr)], drc1;
			wdf		drc0;
			and.testz	p0, r5, r5;	// is prev null?
			wdf		drc1;
			/* (prev == NULL) ? head = current->next : prev->nextptr = current->next */
			!p0	ba	CRRL_NoHeadUpdate;
			{
				MK_STORE_STATE_INDEXED(sCompleteRenderContextHead, r2, r6, r3);
			}
			CRRL_NoHeadUpdate:
			!p0	MK_MEM_ACCESS(stad)	[r5, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextCompleteDevAddr)], r6;

			and.testz	p1, r6, r6;	// is next null?
			/* (next == NULL) ? tail == current->prev : next->prevptr = current->prev */
			!p1	ba	CRRL_NoTailUpdate;
			{
				MK_STORE_STATE_INDEXED(sCompleteRenderContextTail, r2, r5, r4);
			}
			CRRL_NoTailUpdate:
			!p1	MK_MEM_ACCESS(stad)	[r6, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevCompleteDevAddr)], r5;
			MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevCompleteDevAddr)], #0;
			MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextCompleteDevAddr)], #0;
			idf		drc1, st;

			/* Remove the render from the head of the complete renders list */
			MK_MEM_ACCESS(ldad)	r5, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sNextDevAddr)], drc0;
			wdf		drc0;
			and.testz	p0, r5, r5;
			MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWCompleteRendersHead)], r5;
			p0	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWCompleteRendersTail)], #0;
			!p0	MK_MEM_ACCESS(stad)	[r5, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sPrevDevAddr)], #0;
			MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sNextDevAddr)], #0;

			/* make sure all previous writes have completed */
			wdf		drc1;
			/* If it is not empty, then add the current render context back to the tail of the complete render context list */
			p0	ba		CRRL_RenderListEmpty;
			{
				MK_LOAD_STATE_INDEXED(r5, sCompleteRenderContextTail, r2, drc0, r4);
				MK_WAIT_FOR_STATE_LOAD(drc0);

				and.testz		p1, r5, r5;	// is tail null?
				!p1	MK_MEM_ACCESS(stad)	[r5, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextCompleteDevAddr)], r0;
				!p1	ba	CRRL_RLNE_NoHeadUpdate;
				{
					MK_STORE_STATE_INDEXED(sCompleteRenderContextHead, r2, r0, r3);
				}
				CRRL_RLNE_NoHeadUpdate:
				MK_STORE_STATE_INDEXED(sCompleteRenderContextTail, r2, r0, r3);
				MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevCompleteDevAddr)], r5;
				idf		drc0, st;
				wdf		drc0;
			}
			CRRL_RenderListEmpty:

			/* Update last op complete values */
			mov		r0, r1; // HWRenderDetails
			MK_MEM_ACCESS(ldad)	r1, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWRTDataDevAddr)], drc0;
			mov		r7, pclink;	// save off the pclink
			wdf		drc0;
			bal		UpdateRenderStatusValues;
			bal 	ReleaseRenderResources;
			mov		pclink, r7; 	// restore the pclink

			/* Check if this has unblocked any TA commands */
			FINDTA();
#if defined(SGX_FEATURE_2D_HARDWARE)
			FIND2D();
#endif

			WRITEHWPERFCB(3D, MK_3D_DUMMY_END, GRAPHICS, #0, p0, k3d_dummy_2, r2, r3, r4, r5, r6, r7, r8, r9);
			/* 	Now check to see if there are any other 3D operations we can do instead
				not bal so that it returns as if this occurrence of the call */
			ba	Find3D;
		}
	}
	CRRL_No_KickRender:
#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH)
	/* Check whether a PixelBE_EOR is set, if so don't switch wait for current render to finish */
	ldr		r0, #EUR_CR_EVENT_STATUS >> 2, drc0;
	mov		r1, #EUR_CR_EVENT_STATUS_PIXELBE_END_RENDER_MASK;
	wdf		drc0;
	and.testnz	p1, r0, r1;
	p1	ba		CRRL_End;
#endif
	{
		/* Start the ISP context store
			r0 - next rendercontext */
		START3DCONTEXTSWITCH(CRRL);
	}
	CRRL_End:

	/* branch back to calling code */
	lapc;
}

/*****************************************************************************
 KickRender routine	- performs anything that is required before actually
 			  starting the render

 inputs:	r0 - context of render about to be started
		r1 - render details of render about to be started
		r2 - priority of the render context about to be started
	#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
		R_RTAIdx - RT index of render about to be started
	#endif
		R_RTData - SGXMKIF_HWRTDATA for render
 temps:		r2, r3, r4, r5, r6, r7, r8, r9, r10, r11
*****************************************************************************/
KickRender:
{
	MK_TRACE(MKTC_KICKRENDER_START, MKTF_3D);

	/* Reset the priority scheduling level block, as it is no longer blocked */
	MK_STORE_STATE(ui32BlockedPriority, #(SGX_MAX_CONTEXT_PRIORITY - 1));

	/* Trace the render context/target pointers. */
	MK_TRACE(MKTC_KICKRENDER_RENDERCONTEXT, MKTF_3D | MKTF_RC);
	MK_TRACE_REG(r0, MKTF_3D | MKTF_RC);
	MK_TRACE(MKTC_KICKRENDER_RTDATA, MKTF_3D | MKTF_RT);
	MK_TRACE_REG(R_RTData, MKTF_3D | MKTF_RT);

	/* Save the PID for easy retrieval later */
	MK_MEM_ACCESS(ldad)	r4, [r0, HSH(DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32PID))], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui323DPID)], r4;
	MK_TRACE(MKTC_KICKRENDER_PID, MKTF_PID);
	MK_TRACE_REG(r4, MKTF_PID);

	/* This render is ready to go! */
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32ActiveRTIdx)], R_RTAIdx;
#endif
	MK_STORE_STATE(s3DContext, r0);
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRenderDetails)], r1;
	MK_STORE_STATE(ui323DCurrentPriority, r2);
	idf		drc0, st;
	wdf		drc0;

#if defined(DEBUG) || defined(PDUMP) || defined(SUPPORT_SGX_HWPERF)
	MK_MEM_ACCESS(ldad)	r3, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32FrameNum)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui323DFrameNum)], r3;
	MK_TRACE(MKTC_3D_FRAMENUM, MKTF_3D | MKTF_FRMNUM);
	MK_TRACE_REG(r3, MKTF_3D | MKTF_FRMNUM);
#endif

#if defined(SGX_FEATURE_MP)
#if defined(FIX_HW_BRN_30710) || defined(FIX_HW_BRN_30764)
	SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_SOFT_RESET >> 2, #EUR_CR_MASTER_SOFT_RESET_IPF_RESET_MASK);
	SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_SOFT_RESET >> 2, #0);
#endif
#endif /* SGX_FEATURE_MP */
#if defined(FIX_HW_BRN_31964)
	/* Reset the ISP2 in case some cores have no work */
	SGXMK_SPRV_WRITE_REG(#EUR_CR_SOFT_RESET >> 2, #EUR_CR_SOFT_RESET_ISP2_RESET_MASK);
	SGXMK_SPRV_WRITE_REG(#EUR_CR_SOFT_RESET >> 2, #0);
#endif

	/* Decide which context should be used by the 3D */
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext0RTData)], drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext1RTData)], drc1;
	mov		r4, #0 << EUR_CR_DPM_STATE_CONTEXT_ID_DALLOC_SHIFT;
	wdf		drc0;
	wdf		drc1;

	xor.testz	p0, R_RTData, r2;
	p0	ba		KR_ContextChosen;
	{
		xor.testz	p0, R_RTData, r3;
		p0	mov		r4, #1 << EUR_CR_DPM_STATE_CONTEXT_ID_DALLOC_SHIFT;
		p0	mov		r2, r3;
		p0	ba		KR_ContextChosen;
		{
			and.testz	p0, r2, r2;
			p0	ba		KR_ContextChosen;
			{
				and.testz	p0, r3, r3;
				p0	mov		r4, #1 << EUR_CR_DPM_STATE_CONTEXT_ID_DALLOC_SHIFT;
				p0	mov		r2, r3;
				p0	ba		KR_ContextChosen;
				{
					/* Choose context not being used by TA */
					ldr		r4, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc0;
					wdf		drc0;
					and		r4, r4, #EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_MASK;
					shr		r4, r4, #EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_SHIFT;
					xor		r4, r4, #1;
					and.testnz	p0, r4, #1;
					shl		r4, r4, #EUR_CR_DPM_STATE_CONTEXT_ID_DALLOC_SHIFT;
					p0	mov		r2, r3;
				}
			}
		}
	}
	KR_ContextChosen:

	/* Save which context is being used */
	and.testz	p0, r4, #1 << EUR_CR_DPM_STATE_CONTEXT_ID_DALLOC_SHIFT;
	p0	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext0RTData)], R_RTData;
	!p0	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext1RTData)], R_RTData;
	idf 		drc0, st;
	wdf		drc0;

	/* Set context dalloc ID */
	ldr		r3, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc0;
	wdf		drc0;
	and		r3, r3, #~EUR_CR_DPM_STATE_CONTEXT_ID_DALLOC_MASK;
	or		r3, r3, r4;
	str		#EUR_CR_DPM_STATE_CONTEXT_ID >> 2, r3;

	/* Convert context dalloc ID to load/store ID as this is all that is required from now on */
	shr		r4, r4, #EUR_CR_DPM_STATE_CONTEXT_ID_DALLOC_SHIFT;

	/* Check if any context loading or storing is required */
	xor.testz	p0, R_RTData, r2;
	p0	ba		KR_NoContextLoad;
	{
		and.testz	p0, r2, r2;
		p0	ba		KR_NoContextStore;
		{
			/* check whether the store is actually required */
			MK_MEM_ACCESS(ldad)	r3, [r2, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc0;
			wdf		drc0;
			/* if the scene has been completed by the TA then we need to store the state table */
			and.testz p0, r3, #SGXMKIF_HWRTDATA_CMN_STATUS_TACOMPLETE;
			/*
				Store not required if partially TA'd as it would have been stored when the TA changed context
				NOTE: this guarantees that the TPC & CSM will not be flushed when the TA is active.
			*/
			p0	ba	KR_NoContextStore;
			{
				/* find out the RenderContext of the RTDATA */
				MK_MEM_ACCESS(ldad)	r3, [r2, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderContextDevAddr)], drc0;
				wdf		drc0;

				/* Make sure the correct memory context is setup */
				SETUPDIRLISTBASE(KR_CS, r3, #SETUP_BIF_3D_REQ);

				/* Store the DPM state via the 3D Index */
				STORE3DCONTEXT(r2, r4);
			}
		}
		KR_NoContextStore:

		/* Make sure the correct memory context is setup */
		SETUPDIRLISTBASE(KR_CL, r0, #SETUP_BIF_3D_REQ);

		/* Load the DPM state via the 3D Index */
		LOAD3DCONTEXT(R_RTData, r4);

		ba		KR_MemoryContextAlreadySetup;
	}
	KR_NoContextLoad:
	{
		/* Ensure the memory context is setup */
		SETUPDIRLISTBASE(KR_NCL, r0, #SETUP_BIF_3D_REQ);
	}
	KR_MemoryContextAlreadySetup:

	/* get the new render context */
	MK_MEM_ACCESS(ldad)	r3, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderContextDevAddr.uiAddr)], drc0;
	wdf		drc0;

	/* get the current 3D HWPBDesc */
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DHWPBDesc.uiAddr)], drc0;
	/* get the new 3D HWPBDesc */
	MK_MEM_ACCESS(ldad)	r3, [r3, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPBDescDevVAddr.uiAddr)], drc0;
	/* get the current TA HWPBDesc */
	MK_MEM_ACCESS_BPCACHE(ldad)	r4, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc.uiAddr)], drc0;
	wdf		drc0;

	/* if Current PB == New PB then no store or load required */
	xor.testz	p0, r2, r3;
	p0	ba	KR_NoPBLoad;
	{
		/* if Current PB == NULL then no store required */
		and.testz	p0, r2, r2;
		p0	ba	KR_NoPBStore;
		{
#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS) && !defined(FIX_HW_BRN_25910)
			/* if current PB == TA PB then no store required */
			xor.testz	p0, r2, r4;
			p0	ba	KR_NoPBStore;
#endif /* defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS) && !defined(FIX_HW_BRN_25910) */
			{
				/* PB Store: No idle required as we know the PB is not in use */
				STORE3DPB();
			}
		}
		KR_NoPBStore:

		/*
			If the new PB == TA PB, we have to:
				1) Idle the TA, so no allocations can take place
				2) Store the TA PB
				3) Then load the up-to-date values to 3D
				4) Resume the TA
		*/
		xor.testnz	p0, r3, r4;
		p0	ba	KR_NoPBCopy;
		{
			/* Setup registers */
			mov		r16, #USE_IDLECORE_TA_REQ;
			PVRSRV_SGXUTILS_CALL(IdleCore);

			/* Store the current details of the TA PB */
			STORETAPB();
		}
		KR_NoPBCopy:

		/*
			FIXME: On cores where EUR_CR_DPM_CONTEXT_PB_BASE is correctly implemented,
			we need to ensure that the TA is idle when updating that register.
		*/

		/* PB Load: */
		LOAD3DPB(r3);

		RESUME(r4);
	}
	KR_NoPBLoad:

	MK_MEM_ACCESS(ldad)	r5, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32RenderFlags)], drc0;
	wdf		drc0;

	/* Set DPM dealloc bits */
	and.testnz p0, r5, #SGXMKIF_RENDERFLAGS_NODEALLOC;
	p0	ba		KR_NoDealloc;
	{
		/*
			Flag the events that need to happen for the render to be finished.
		*/
		MK_LOAD_STATE(r5, ui32IRenderFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		or		r5, r5, #RENDER_IFLAGS_WAITINGFOREOR | RENDER_IFLAGS_WAITINGFOR3DFREE;
		MK_STORE_STATE(ui32IRenderFlags, r5);
		MK_WAIT_FOR_STATE_STORE(drc0);

		str		#EUR_CR_DPM_3D_DEALLOCATE >> 2, #EUR_CR_DPM_3D_DEALLOCATE_GLOBAL_MASK | EUR_CR_DPM_3D_DEALLOCATE_ENABLE_MASK;
		ba		KR_DeallocSet;
	}
	KR_NoDealloc:
	{
		/* Disable DPM deallocation. */
		str		#EUR_CR_DPM_3D_DEALLOCATE >> 2, #0;

		/* Flag the events that need to happen for the render to be finished. */
		MK_LOAD_STATE(r5, ui32IRenderFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		or		r5, r5, #RENDER_IFLAGS_WAITINGFOREOR;
		MK_STORE_STATE(ui32IRenderFlags, r5);
		MK_WAIT_FOR_STATE_STORE(drc0);
	}
	KR_DeallocSet:

	/* Load 3D registers */
	mov		r2, #OFFSET(SGXMKIF_HWRENDERDETAILS.s3DRegs);
	iadd32	r2, r2.low, r1;

#if defined(SGX_FEATURE_VISTEST_IN_MEMORY)
	COPYMEMTOCONFIGREGISTER_3D(SPMAC_VISREGBASE, EUR_CR_ISP_VISREGBASE, r2, r3, r4, r5);
#else
	mov	r10, #(EUR_CR_ISP_VISTEST_CTRL_CLEAR0_MASK | \
				  EUR_CR_ISP_VISTEST_CTRL_CLEAR1_MASK | \
				  EUR_CR_ISP_VISTEST_CTRL_CLEAR2_MASK | \
				  EUR_CR_ISP_VISTEST_CTRL_CLEAR3_MASK | \
				  EUR_CR_ISP_VISTEST_CTRL_CLEAR4_MASK | \
				  EUR_CR_ISP_VISTEST_CTRL_CLEAR5_MASK | \
				  EUR_CR_ISP_VISTEST_CTRL_CLEAR6_MASK | \
				  EUR_CR_ISP_VISTEST_CTRL_CLEAR7_MASK);
	str	#EUR_CR_ISP_VISTEST_CTRL >> 2, r10;
#endif
#if defined(SGX_FEATURE_DEPTH_BIAS_OBJECTS)
	MK_MEM_ACCESS_CACHED(ldad.f8)	r3, [r2, #0++], drc0;
#else
	MK_MEM_ACCESS_CACHED(ldad.f6)	r3, [r2, #0++], drc0;
#endif
	str	#EUR_CR_ISP_RENDBOX1 >> 2, #0;
	str	#EUR_CR_ISP_RENDBOX2 >> 2, #0;
	str	#EUR_CR_ISP_RENDER >> 2, #EUR_CR_ISP_RENDER_TYPE_NORMAL3D;
	wdf		drc0;
	str	#EUR_CR_BIF_3D_REQ_BASE >> 2, r3;
	str	#EUR_CR_BIF_ZLS_REQ_BASE >> 2, r4;
	str	#EUR_CR_PIXELBE >> 2, r5;
	str	#EUR_CR_ISP_IPFMISC >> 2, r6;
	str	#EUR_CR_3D_AA_MODE >> 2, r7;
#if defined(SGX_FEATURE_DEPTH_BIAS_OBJECTS)
	str	#EUR_CR_ISP_DBIAS0 >> 2, r8;
	str	#EUR_CR_ISP_DBIAS1 >> 2, r9;
	str	#EUR_CR_ISP_DBIAS2 >> 2, r10;
#else
	str	#EUR_CR_ISP_DBIAS >> 2, r8;
#endif

#if defined(EUR_CR_4Kx4K_RENDER)
	MK_MEM_ACCESS_CACHED(ldad)	r3, [r2, #1++], drc0;
	wdf		drc0;
	str	#EUR_CR_4Kx4K_RENDER >> 2, r3;
#endif

	WRITEISPRGNBASE(KR_ISPRgnBase, r2, r3, r4, r5);

#if defined(SGX_FEATURE_MP)
	MK_MEM_ACCESS_CACHED(ldad)	r3, [r2, #1++], drc0;
	wdf		drc0;
	WRITEMASTERCONFIG(#EUR_CR_MASTER_ISP_RGN >> 2, r3, r7);
#endif

#if defined(EUR_CR_ISP_MTILE)
	MK_MEM_ACCESS_CACHED(ldad.f3)	r3, [r2, #0++], drc0;
	wdf		drc0;
	str	#EUR_CR_ISP_MTILE >> 2, r3;
	str	#EUR_CR_ISP_MTILE1 >> 2, r4;
	str	#EUR_CR_ISP_MTILE2 >> 2, r5;
#endif

#if defined(EUR_CR_DOUBLE_PIXEL_PARTITIONS)
	MK_MEM_ACCESS_CACHED(ldad)	r3, [r2, #1++], drc0;
	wdf		drc0;
	/* 
	The number of output partitions is chosen, to minimize
	fragmentation based on possibility of double and
	DMS_CTRL2 is already programmed at initialisation time
	 */
	str	#EUR_CR_DOUBLE_PIXEL_PARTITIONS >> 2, r3;
#endif
#if defined(EUR_CR_ISP_OGL_MODE)
	MK_MEM_ACCESS_CACHED(ldad.f3)	r3, [r2, #0++], drc0;
	wdf		drc0;
	str	#EUR_CR_ISP_OGL_MODE >> 2, r3;
	str	#EUR_CR_ISP_PERPENDICULAR >> 2, r4;
	str	#EUR_CR_ISP_CULLVALUE >> 2, r5;
#endif

#if defined(SGX_FEATURE_ZLS_EXTERNALZ)
	#if defined(EUR_CR_ISP2_ZLS_EXTZ_RGN_BASE)
	COPYMEMTOCONFIGREGISTER_3D(KR_ZLS_EXTZ, EUR_CR_ISP2_ZLS_EXTZ_RGN_BASE, r2, r3, r4, r5);
	#else
	COPYMEMTOCONFIGREGISTER_3D(KR_ZLS_EXTZ, EUR_CR_ISP_ZLS_EXTZ_RGN_BASE, r2, r3, r4, r5);
	#endif
#endif
	MK_MEM_ACCESS_CACHED(ldad.f8)	r3, [r2, #0++], drc0;
	wdf		drc0;
	str #EUR_CR_ISP_ZLSCTL >> 2, r3;
	str	#EUR_CR_ISP_ZLOAD_BASE >> 2, r4;
	str	#EUR_CR_ISP_ZSTORE_BASE >> 2, r5;
	str	#EUR_CR_ISP_STENCIL_LOAD_BASE >> 2, r6;
	str	#EUR_CR_ISP_STENCIL_STORE_BASE >> 2, r7;
	str	#EUR_CR_ISP_BGOBJDEPTH >> 2, r8;
	str	#EUR_CR_ISP_BGOBJ >> 2, r9;
	str	#EUR_CR_ISP_BGOBJTAG >> 2, r10;

#if defined(EUR_CR_ISP_MULTISAMPLECTL2)
	MK_MEM_ACCESS_CACHED(ldad)	r3, [r2, #1++], drc0;
	wdf		drc0;
	str	#EUR_CR_ISP_MULTISAMPLECTL2 >> 2, r3;
#endif

	MK_MEM_ACCESS_CACHED(ldad.f8)	r3, [r2, #0++], drc0;
	wdf		drc0;
	str	#EUR_CR_ISP_MULTISAMPLECTL >> 2, r3;
	str	#EUR_CR_ISP_TAGCTRL >> 2, r4;
	str	#EUR_CR_TSP_CONFIG >> 2, r5;
	str	#EUR_CR_EVENT_PIXEL_PDS_EXEC >> 2, r6;
	str	#EUR_CR_EVENT_PIXEL_PDS_DATA >> 2, r7;
	str	#EUR_CR_EVENT_PIXEL_PDS_INFO >> 2, r8;
	
#if defined(EUR_CR_ISP_FPUCTRL)
	str	#EUR_CR_ISP_FPUCTRL >> 2, r9;
	str	#EUR_CR_USE_FILTER0_LEFT >> 2, r10;
	
	MK_MEM_ACCESS_CACHED(ldad)	r3, [r2, #1++], drc0;
	wdf		drc0;
	str	#EUR_CR_USE_FILTER0_RIGHT >> 2, r3;
#else
	str	#EUR_CR_USE_FILTER0_LEFT >> 2, r9;
	str	#EUR_CR_USE_FILTER0_RIGHT >> 2, r10;
#endif

	MK_MEM_ACCESS_CACHED(ldad.f8)	r3, [r2, #0++], drc0;
	wdf		drc0;
	str	#EUR_CR_USE_FILTER0_EXTRA >> 2, r3;
	str	#EUR_CR_USE_FILTER1_LEFT >> 2, r4;
	str	#EUR_CR_USE_FILTER1_RIGHT >> 2, r5;
	str	#EUR_CR_USE_FILTER1_EXTRA >> 2, r6;
	str	#EUR_CR_USE_FILTER2_LEFT >> 2, r7;
	str	#EUR_CR_USE_FILTER2_RIGHT >> 2, r8;
	str	#EUR_CR_USE_FILTER2_EXTRA >> 2, r9;
	str	#EUR_CR_USE_FILTER_TABLE >> 2, r10;

	#if defined(SGX_FEATURE_USE_HIGH_PRECISION_FIR_COEFF)
	MK_MEM_ACCESS_CACHED(ldad.f2)	r3, [r2, #0++], drc0;
	wdf		drc0;
	str	#EUR_CR_USE_FILTER0_CENTRE >> 2, r3;
	str	#EUR_CR_USE_FILTER1_CENTRE >> 2, r4;
	#endif

#if defined(SGX_FEATURE_TAG_YUV_TO_RGB)
	MK_MEM_ACCESS_CACHED(ldad.f8)	r3, [r2, #0++], drc0;
	wdf		drc0;
	str	#EUR_CR_TPU_YUV00 >> 2, r3;
	str	#EUR_CR_TPU_YUV01 >> 2, r4;
	str	#EUR_CR_TPU_YUV02 >> 2, r5;
	str	#EUR_CR_TPU_YUV03 >> 2, r6;
	str	#EUR_CR_TPU_YUV10 >> 2, r7;
	str	#EUR_CR_TPU_YUV11 >> 2, r8;
	str	#EUR_CR_TPU_YUV12 >> 2, r9;
	str	#EUR_CR_TPU_YUV13 >> 2, r10;

	MK_MEM_ACCESS_CACHED(ldad.f2)	r3, [r2, #0++], drc0;
	wdf		drc0;
	str	#EUR_CR_TPU_YUV04 >> 2, r3;
	str	#EUR_CR_TPU_YUV14 >> 2, r4;

	#if !defined(SGX_FEATURE_TAG_YUV_TO_RGB_HIPREC)
	MK_MEM_ACCESS_CACHED(ldad)	r3, [r2, #1++], drc0;
	wdf		drc0;
	str	#EUR_CR_TPU_YUVSCALE >> 2, r3;
	#else
	MK_MEM_ACCESS_CACHED(ldad.f2)	r3, [r2, #0++], drc0; 
	wdf		drc0;
	str	#EUR_CR_TPU_YUV05 >> 2, r3;
	str	#EUR_CR_TPU_YUV15 >> 2, r4;
	#endif
#endif

#if defined(EUR_CR_TPU_LUMA0)
	MK_MEM_ACCESS_CACHED(ldad.f3)	r3, [r2, #0++], drc0;
	wdf		drc0;
	str	#EUR_CR_TPU_LUMA0 >> 2, r3;
	str	#EUR_CR_TPU_LUMA1 >> 2, r4;
	str	#EUR_CR_TPU_LUMA2 >> 2, r5;
#endif

#if defined(EUR_CR_PDS_PP_INDEPENDANT_STATE)
	MK_MEM_ACCESS_CACHED(ldad)	r3, [r2, #1++], drc0;
	wdf		drc0;
	str	#EUR_CR_PDS_PP_INDEPENDANT_STATE >> 2, r3;
#endif

#if defined(SUPPORT_DISPLAYCONTROLLER_TILING)
	MK_MEM_ACCESS_CACHED(ldad)	r3, [r2, #1++], drc0;
	wdf		drc0;
	SGXMK_SPRV_WRITE_REG(#EUR_CR_BIF_TILE0 >> 2, r3);
#endif

#if defined(SGX_FEATURE_ALPHATEST_COEFREORDER) && defined(EUR_CR_ISP_DEPTHSORT_FB_ABC_ORDER_MASK)
	MK_MEM_ACCESS_CACHED(ldad) r3, [r2, #1++], drc0;
	wdf		drc0;
	SGXMK_SPRV_WRITE_REG(#EUR_CR_ISP_DEPTHSORT >> 2, r3);
#endif

#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	/* Tell the DPM which RT we are rendering */
	str		#EUR_CR_DPM_3D_RENDER_TARGET_INDEX >> 2, R_RTAIdx;
	and.testz	p0, R_RTAIdx, R_RTAIdx;
	p0	ba	KR_FirstRTInArray;
	{
		/* Move the RgnBase to point to the correct RT */
		MK_MEM_ACCESS(ldad)	r4, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32RgnStride)], drc0;
		ldr		r2, #EUR_CR_ISP_RGN_BASE >> 2, drc0;
		wdf		drc0;
		ima32	r2, R_RTAIdx, r4, r2;
		str	#EUR_CR_ISP_RGN_BASE >> 2, r2;
		
		/* Move on the Z/S Buffer addresses */
		MK_MEM_ACCESS(ldad)	r3, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32ZBufferStride)], drc0;
		ldr		r4, #EUR_CR_ISP_ZLOAD_BASE >> 2, drc0;
		ldr		r5, #EUR_CR_ISP_ZSTORE_BASE >> 2, drc0;
		wdf		drc0;
		ima32	r4, R_RTAIdx, r3, r4;
		ima32	r5, R_RTAIdx, r3, r5;
		str	#EUR_CR_ISP_ZLOAD_BASE >> 2, r4;
		str	#EUR_CR_ISP_ZSTORE_BASE >> 2, r5;
		
		MK_MEM_ACCESS(ldad)	r3, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32SBufferStride)], drc0;
		ldr		r4, #EUR_CR_ISP_STENCIL_LOAD_BASE >> 2, drc0;
		ldr		r5, #EUR_CR_ISP_STENCIL_STORE_BASE >> 2, drc0;
		wdf		drc0;
		ima32	r4, R_RTAIdx, r3, r4;
		ima32	r5, R_RTAIdx, r3, r5;
		str	#EUR_CR_ISP_STENCIL_LOAD_BASE >> 2, r4;
		str	#EUR_CR_ISP_STENCIL_STORE_BASE >> 2, r5;
	}
	KR_FirstRTInArray:

	/* Check if there is a Address to update in the pixel event program */
	MK_MEM_ACCESS(ldad)	r3, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sPixEventUpdateDevAddr)], drc1;
	wdf		drc1;
	and.testz	p0, r3, r3;
	p0	ba	KR_NoPixEventUpdate;
	{
		/* Update the program so it writes to the correct surface */
		MK_MEM_ACCESS(ldad)	r2, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWPixEventUpdateListDevAddr)], drc1;
		wdf		drc1;
		imae	r2, R_RTAIdx.low, #(EURASIA_USE_INSTRUCTION_SIZE), r2, u32;
		MK_MEM_ACCESS(ldad.f2)	r4, [r2, #0++], drc1;

		SWITCHEDMTO3D();
		wdf	drc1;
		MK_MEM_ACCESS_BPCACHE(stad.f2)	[r3, #0++], r4;

		SWITCHEDMBACK();
	}
	KR_NoPixEventUpdate:
	/* No need to issue idf, as this happens later */
#endif

	/* check if this is a z-only render */
	MK_MEM_ACCESS(ldad)	r5, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32RenderFlags)], drc0;
	wdf		drc0;
	and.testz		p0, r5, #SGXMKIF_RENDERFLAGS_ZONLYRENDER;
	p0 		ba	KR_NotZOnlyRender;
	{
		ldr		r4, #EUR_CR_ISP_ZLSCTL >> 2, drc0;
		wdf		drc0;
		or		r4, r4, #EUR_CR_ISP_ZLSCTL_ZONLYRENDER_MASK;
		str		#EUR_CR_ISP_ZLSCTL >> 2, r4;
	}
	KR_NotZOnlyRender:

	/* Set up the pixel task part of the DMS control. */
	ldr		r4, #EUR_CR_DMS_CTRL >> 2, drc0;
	MK_MEM_ACCESS(ldad)	r5, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32NumPixelPartitions)], drc1;
	wdf		drc0;
	and		r4, r4, ~#EUR_CR_DMS_CTRL_MAX_NUM_PIXEL_PARTITIONS_MASK;
	wdf		drc1;
	or		r4, r4, r5;
	str		#EUR_CR_DMS_CTRL >> 2, r4;

	/* Make sure the partial render bit is cleared */
	str		#EUR_CR_DPM_PARTIAL_RENDER >> 2, #0;

#if defined(FIX_HW_BRN_24549)
	/* Pause the USSE */
	mov 	r16, #(USE_IDLECORE_TA_REQ | USE_IDLECORE_USSEONLY_REQ);
	PVRSRV_SGXUTILS_CALL(IdleCore);
#endif

#if defined(SGX_FEATURE_SYSTEM_CACHE)
	#if defined(SGX_FEATURE_MP)
		INVALIDATE_SYSTEM_CACHE(#EUR_CR_MASTER_SLC_CTRL_INVAL_DM_PIXEL_MASK, #EUR_CR_MASTER_SLC_CTRL_FLUSH_DM_PIXEL_MASK);
	#else
		INVALIDATE_SYSTEM_CACHE();
	#endif /* SGX_FEATURE_MP */
#endif

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
	INVALIDATE_EXT_SYSTEM_CACHE();
#endif /* SUPPORT_EXTERNAL_SYSTEM_CACHE */
	
#if defined(EUR_CR_CACHE_CTRL)
	/* Invalidate the MADD cache */
	ldr		r3, #EUR_CR_CACHE_CTRL >> 2, drc0;
	wdf		drc0;
	or		r3, r3, #EUR_CR_CACHE_CTRL_INVALIDATE_MASK;
	str		#EUR_CR_CACHE_CTRL >> 2, r3;

	mov		r4, #EUR_CR_EVENT_STATUS_MADD_CACHE_INVALCOMPLETE_MASK;
	/* Wait for the MADD cache to be invalidated */
	ENTER_UNMATCHABLE_BLOCK;
	KR_MADDCacheNotInvalidated:
	{
		ldr		r3, #EUR_CR_EVENT_STATUS >> 2, drc0;
		wdf		drc0;
		and.testz	 p0, r3, r4;
		p0	ba		KR_MADDCacheNotInvalidated;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the bit */
	mov		r3, #EUR_CR_EVENT_HOST_CLEAR_MADD_CACHE_INVALCOMPLETE_MASK;
	str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r3;
#else
	/* Invalidate the data cache */
	str		#EUR_CR_DCU_ICTRL >> 2, #EUR_CR_DCU_ICTRL_INVALIDATE_MASK;

	/* Wait for the data cache to be invalidated */
	ENTER_UNMATCHABLE_BLOCK;
	KR_DataCacheNotInvalidated:
	{
		ldr		r3, #EUR_CR_EVENT_STATUS2 >> 2, drc0;
		wdf		drc0;
		shl.testns	 p0, r3, #(31 - EUR_CR_EVENT_STATUS2_DCU_INVALCOMPLETE_SHIFT);
		p0	ba		KR_DataCacheNotInvalidated;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the bit */
	mov		r3, #EUR_CR_EVENT_HOST_CLEAR2_DCU_INVALCOMPLETE_MASK;
	str		#EUR_CR_EVENT_HOST_CLEAR2 >> 2, r3;

	/* Invalidate the texture cache */
	str		#EUR_CR_TCU_ICTRL >> 2, #EUR_CR_TCU_ICTRL_INVALTCU_MASK;

	mov		r4, #EUR_CR_EVENT_STATUS_TCU_INVALCOMPLETE_MASK;
	/* Wait for the texture cache to be invalidated */
	ENTER_UNMATCHABLE_BLOCK;
	KR_TextureCacheNotInvalidated:
	{
		ldr		r3, #EUR_CR_EVENT_STATUS >> 2, drc0;
		wdf		drc0;
		and.testz	 p0, r3, r4;
		p0	ba		KR_TextureCacheNotInvalidated;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the bit */
	mov		r3, #EUR_CR_EVENT_HOST_CLEAR_TCU_INVALCOMPLETE_MASK;
	str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r3;
#endif


#if defined(FIX_HW_BRN_24549)
	RESUME(r4);
#endif

#if defined(SGX_FEATURE_WRITEBACK_DCU)
	/* Invalidate the DCU cache. */
	ci.global	#SGX545_USE_OTHER2_CFI_DATAMASTER_PIXEL, #0, #SGX545_USE1_OTHER2_CFI_LEVEL_L0L1L2;
#endif

	/* Invalidate the TSP parameter cache. */
	str		#EUR_CR_TSP_PARAMETER_CACHE >> 2, #EUR_CR_TSP_PARAMETER_CACHE_INVALIDATE_MASK;

	/* Invalidate the USE cache. */
	INVALIDATE_USE_CACHE(KR_, p0, r2, #SETUP_BIF_3D_REQ);

	/* Invalidate PDS cache */
	str		#EUR_CR_PDS_INV1 >> 2, #EUR_CR_PDS_INV1_DSC_MASK;
	INVALIDATE_PDS_CODE_CACHE();

	/* Wait for PDS cache to be invalidated */
	ENTER_UNMATCHABLE_BLOCK;
	KR_PDSCacheNotFlushed:
	{
		ldr		r2, #EUR_CR_PDS_CACHE_STATUS >> 2, drc0;
		wdf		drc0;

		and		r2, r2, #EURASIA_PDS_CACHE_PIXEL_STATUS_MASK;
		xor.testnz	p0, r2, #EURASIA_PDS_CACHE_PIXEL_STATUS_MASK;

		p0	ba		KR_PDSCacheNotFlushed;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the PDS cache flush event. */
	str		#EUR_CR_PDS_CACHE_HOST_CLEAR >> 2, #EURASIA_PDS_CACHE_PIXEL_CLEAR_MASK;

#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH)
	MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sRTStatusDevAddr)], drc0;
	wdf		drc0;
	#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	imae	r2, R_RTAIdx.low, #SIZEOF(IMG_UINT32), r2, u32;
	#endif
	MK_MEM_ACCESS(ldad)	r2, [r2], drc0;
	wdf		drc0;
	and.testz	p0, r2, #SGXMKIF_HWRTDATA_RT_STATUS_ISPCTXSWITCH;
	p0	ba	KR_NoRenderResume;
	{
		mov		r9, pclink;
		bal		ResumeRender;
		mov		pclink, r9;
	}
	KR_NoRenderResume:
#else
	#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH)
	/* If this is render is at the highest priority level, there is nothing todo */
	/* Get the context priority */
	MK_LOAD_STATE(r3, ui323DCurrentPriority, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testz	p0, r3, r3;
	p0	br	KR_HighestPriority;
	{
		MK_MEM_ACCESS(ldad)	r3, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32RenderFlags)], drc0;
		#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH_ALWAYS_SPLIT)
		mov		r6, #SGXMKIF_RENDERFLAGS_BBOX_RENDER;
		#else
		mov		r6, #(SGXMKIF_RENDERFLAGS_BBOX_RENDER | SGXMKIF_RENDERFLAGS_NO_MT_SPLIT);
		#endif
		wdf		drc0;
		and.testnz	p0, r3, r6;
		p0	ba 	KR_BBoxRender;
		{

#if defined(SGX_FEATURE_MP)
			MK_MEM_ACCESS(ldad)	r3, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32MTRegionArraySize)], drc0;
			wdf drc0;
			WRITEMASTERCONFIG(#EUR_CR_MASTER_ISP_RGN >> 2, r3, r6);
#endif

			MK_MEM_ACCESS(ldad)	r3, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui3CurrentMTIdx)], drc1;
			MK_MEM_ACCESS(ldad)	r6, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32LastMTIdx)], drc1;
			wdf		drc1;
			/* if it is the first and last MT do nothing */
			and.testz	p0, r6, r6;
			p0	ba	KR_NoRenderResume;
			{
				MK_TRACE(MKTC_KICKRENDER_CONFIG_REGION_HDRS, MKTF_3D | MKTF_SCH);
				/* Change region header base to point to next MT */
				MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32NumTileBlocksPerMT)], drc0;
				mov		r4, #EURASIA_REGIONHEADER_SIZE;
				imae	r4, r4.low, r3.low, #0, u32;
				shl		r4, r4, #4;

#if defined(SGX_FEATURE_MP)
				/* count up to number of TA cores*/
				mov		r5, #0;
				KR_NextTACore:
				{
					imae	r7, r5.low, #4, #OFFSET(SGXMKIF_HWRENDERDETAILS.s3DRegs.aui32ISPRgnBase[0]), u32; 
					iaddu32	r7, r7.low, r1;
					MK_MEM_ACCESS(ldad)	r7, [r7], drc0;
					wdf		drc0;

					imae	r8, r4.low, r2.low, r7, u32;

					xor.testz	p0, r5, #0;
					p0 str		#EUR_CR_ISP_RGN_BASE >> 2, r8;
					p0 ba KR_Not0TA;
					{
						mov		r7, #((EUR_CR_MASTER_ISP_RGN_BASE1 >> 2) - 1);
						imae	r7, r5.low, #1, r7, u32;
						str		r7, r8;
					}
					KR_Not0TA:
	
					MK_LOAD_STATE_CORE_COUNT_TA(r7, drc0);
					iaddu32		r5, r5.low, #1;
					MK_WAIT_FOR_STATE_LOAD(drc0);

					xor.testnz	p0, r5, r7;
					p0	ba	KR_NextTACore;
				}
#else
				MK_MEM_ACCESS(ldad)	r5, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.s3DRegs.aui32ISPRgnBase[0])], drc0;
				wdf		drc0;
				imae	r4, r4.low, r2.low, r5, u32;

				str		#EUR_CR_ISP_RGN_BASE >> 2, r4;
#endif
				/* Modify the DPM dealloc bits, if not last MT */
				xor.testz p0, r3, r6;
				p0	ba		KR_LastMT;
				{
					/* Only dealloc global if last MT */
					ldr		r3, #EUR_CR_DPM_3D_DEALLOCATE >> 2, drc0;
					wdf		drc0;
					and		r3, r3, ~#EUR_CR_DPM_3D_DEALLOCATE_GLOBAL_MASK;
					str		#EUR_CR_DPM_3D_DEALLOCATE >> 2, r3;
				}
				KR_LastMT:
			}
			KR_NoRenderResume:
		}
		KR_BBoxRender:
	}
	KR_HighestPriority:
	#endif
#endif

	/* check if there is a Z-buffer present */
	MK_MEM_ACCESS(ldad)	r5, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32RenderFlags)], drc0;
	mov				r4, #(SGXMKIF_RENDERFLAGS_DEPTHBUFFER | SGXMKIF_RENDERFLAGS_STENCILBUFFER);
	wdf		drc0;
	/* Has the client requested and Depth or Stencil operations */
	and.testnz		p1, r5, r4;

	/* If we've had a partial render then turn on the load bits */
	MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sRTStatusDevAddr)], drc0;
	wdf		drc0;
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	imae	r2, R_RTAIdx.low, #SIZEOF(IMG_UINT32), r2, u32;
#endif
	MK_MEM_ACCESS(ldad)	r2, [r2], drc0;
	wdf		drc0;
	and.testz	p0, r2, #SGXMKIF_HWRTDATA_RT_STATUS_SPMRENDER;
	!p0	ba		KR_SPMRender;
	{
		/* We have not hit SPM in this scene */
		ldr		r3, #EUR_CR_ISP_ZLSCTL >> 2, drc0;
		wdf		drc0;
		
		/* If both Depth or Stencil operations are present no safe guarding is required */
		mov		r2, #(SGXMKIF_RENDERFLAGS_DEPTHBUFFER | SGXMKIF_RENDERFLAGS_STENCILBUFFER);
		and		r4, r5, r2;
		xor.testz	p2, r5, r4;
		p2	ba	KR_NoZLSChecksRequired;
		{
			/* Ensure depth operations are disabled if buffer not present */
			mov		r4, #SGXMKIF_RENDERFLAGS_DEPTHBUFFER;
			and.testz	p2, r5, r4;
			p2	and	r3, r3, ~#(EUR_CR_ISP_ZLSCTL_ZLOADEN_MASK | EUR_CR_ISP_ZLSCTL_ZSTOREEN_MASK);
			
			/* Ensure Stencil operations are disabled if buffer not present */
			mov		r4, #SGXMKIF_RENDERFLAGS_STENCILBUFFER;
			and.testz	p2, r5, r4;
			p2	and	r3, r3, ~#(EUR_CR_ISP_ZLSCTL_SLOADEN_MASK | EUR_CR_ISP_ZLSCTL_SSTOREEN_MASK);
			
			/* If neither depth nor stencil are present disable Force Load/Store */
			!p1	and	r3, r3, ~#(EUR_CR_ISP_ZLSCTL_FORCEZLOAD_MASK | EUR_CR_ISP_ZLSCTL_FORCEZSTORE_MASK);
			
			str		#EUR_CR_ISP_ZLSCTL >> 2, r3;
		}
		KR_NoZLSChecksRequired:

		#if defined(FIX_HW_BRN_23870)
		/* If client is requesting ZLS, set processempty bit */
		and.testz	p1, r3, #(EUR_CR_ISP_ZLSCTL_FORCEZLOAD_MASK | EUR_CR_ISP_ZLSCTL_FORCEZSTORE_MASK);
		p1	ba		KR_NoAppForcedProcessEmpty;
		{
			/* One of the force bits is set therefore we must set processempty bit */
			ldr		r2, #EUR_CR_ISP_IPFMISC >> 2, drc0;
			wdf		drc0;
			or		r2, r2, #EUR_CR_ISP_IPFMISC_PROCESSEMPTY_MASK;
			str		#EUR_CR_ISP_IPFMISC >> 2, r2;
		}
		KR_NoAppForcedProcessEmpty:
		#endif

		/* ZLSCTL setup done */
		ba		KR_NoSPMRender;
	}
	KR_SPMRender:
	{
		ldr		r2, #EUR_CR_ISP_ZLSCTL >> 2, drc0;
		wdf		drc0;

	#if defined(FIX_HW_BRN_23861)
		/* Is there Depth or Stencil buffer present */
		p1	ba 	KR_ZSBufferPresent;
		{
			/* There is no depth or stencil buffer present therefore use dummy depth buffer */
			/* setup the Dummy Stencil load, store and ZLSCtrl registers */
			MK_TRACE(MKTC_DUMMY_DEPTH, MKTF_3D | MKTF_SCH);

			/* setup the ZLS req. base */
			MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sDummyZLSReqBaseDevVAddr.uiAddr)], drc0;
			MK_MEM_ACCESS_BPCACHE(ldad)	r6, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sDummyStencilLoadDevVAddr.uiAddr)], drc1;
			wdf		drc0;
			str		#EUR_CR_BIF_ZLS_REQ_BASE >> 2, r3;
			MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sDummyStencilStoreDevVAddr.uiAddr)], drc0;

			/* setup the ZLS base reqisters */
			/* Wait for loadAddr */
			wdf		drc1;
			str		#EUR_CR_ISP_STENCIL_LOAD_BASE >> 2, r6;
			/* Wait for StoreAddr */
			wdf		drc0;
			str		#EUR_CR_ISP_STENCIL_STORE_BASE >> 2, r3;

			/*
				- Enable Stencil load/store bits
				- set ZLS Ext to 1 tile (0)
				- set format to uncompressed ieee float
			*/
			mov		r2, #(EUR_CR_ISP_ZLSCTL_SLOADEN_MASK | \
						EUR_CR_ISP_ZLSCTL_SSTOREEN_MASK);
			ba KR_SPM_ZLSConfigured;
		}
		KR_ZSBufferPresent:
	#endif
		{
			/* If depth buffer is present set the load bits */
			mov		r4, #SGXMKIF_RENDERFLAGS_DEPTHBUFFER;
			and.testnz	p2, r5, r4;
		#if defined(EUR_CR_ISP_ZLSCTL_MLOADEN_MASK)
			p2	mov	r4, #(EUR_CR_ISP_ZLSCTL_ZLOADEN_MASK | EUR_CR_ISP_ZLSCTL_MLOADEN_MASK);
		#else
			p2	mov r4, #(EUR_CR_ISP_ZLSCTL_ZLOADEN_MASK | EUR_CR_ISP_ZLSCTL_LOADMASK_MASK);
		#endif
			p2	or	r2, r2, r4;
			
			/* If Stencil buffer is present set the load bits */
			mov		r4, #SGXMKIF_RENDERFLAGS_STENCILBUFFER;
			and.testnz	p2, r5, r4;
			p2	or	r2, r2, #EUR_CR_ISP_ZLSCTL_SLOADEN_MASK;
		}
		KR_SPM_ZLSConfigured:
		
		str		#EUR_CR_ISP_ZLSCTL >> 2, r2;
		
#if defined(FIX_HW_BRN_23870)
		/*
			turn on processing of empty regions
		*/
		ldr		r2, #EUR_CR_ISP_IPFMISC >> 2, drc0;
		wdf		drc0;
		or		r2, r2, #EUR_CR_ISP_IPFMISC_PROCESSEMPTY_MASK;
		str		#EUR_CR_ISP_IPFMISC >> 2, r2;
#endif
	}
	KR_NoSPMRender:

	SWITCHEDMTO3D();

#if defined(FIX_HW_BRN_23862)
	/*
		Apply the BRN23862 fix.
		r9 - Render details structure
		r10 - Macrotile we are rendering.
	*/
	mov		r8, pclink;
	mov		r9, r1;
	mov		r10, #EUR_CR_DPM_ABORT_STATUS_MTILE_GLOBAL_MASK;
	PVRSRV_SGXUTILS_CALL(FixBRN23862);
	mov		pclink, r8;
#endif /* defined(FIX_HW_BRN_23862) */

#if (defined(FIX_HW_BRN_30089) || defined(FIX_HW_BRN_29504) || defined(FIX_HW_BRN_33753)) && defined(SGX_FEATURE_MP)
#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH)

#if ! defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH_ALWAYS_SPLIT)
	MK_LOAD_STATE(r2, ui323DCurrentPriority, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	xor.testz	p0, r2, #0;
	p0 br KR_NoMTSplitting;
#endif
	{
		MK_MEM_ACCESS(ldad)	r3, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui3CurrentMTIdx)], drc0;
		wdf drc0;

		ADDLASTRGN(R_RTData, r3);
		br KR_29504Done;
	}
	KR_NoMTSplitting:
#endif
	{
		/* Apply the workaround for BRN30089 */
		ADDLASTRGN(R_RTData, #EUR_CR_DPM_ABORT_STATUS_MTILE_GLOBAL_MASK);
	}
	KR_29504Done:
#endif

	/* Copy background object state words */
	MK_MEM_ACCESS_BPCACHE(ldad)		r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sBGObjBaseDevAddr)], drc0;
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	MK_MEM_ACCESS(ldad.f2)	r3, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.aui32SpecObject)-1], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(stad.f2)	[r2, #0++], r3;

	/* Update the HWBGObj so it references the correct surface */
	MK_MEM_ACCESS(ldad)	r3, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWBgObjPrimPDSUpdateListDevAddr)], drc1;
	wdf		drc1;
	imae	r3, R_RTAIdx.low, #SIZEOF(IMG_UINT32), r3, u32;
	MK_MEM_ACCESS(ldad)	r4, [r3], drc1;
	wdf		drc1;
	MK_MEM_ACCESS_BPCACHE(stad)	[r2, #0++], r4;
#else
	MK_MEM_ACCESS(ldad.f3)	r3, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.aui32SpecObject)-1], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(stad.f3)	[r2, #0++], r3;
#endif
	idf		drc0, st;
	wdf		drc0;

	SWITCHEDMBACK();

#if !defined(USE_SUPPORT_NO_TA3D_OVERLAP) && defined(DEBUG)
	MK_LOAD_STATE(r2, ui32ITAFlags, drc0);
	MK_MEM_ACCESS_BPCACHE(ldad) r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32OverlapCount)], drc1;
	MK_WAIT_FOR_STATE_LOAD(drc0);
	wdf	drc1;
	and.testz	p0, r2, #TA_IFLAGS_TAINPROGRESS;
	p0	ba KR_NoOverlap;
	{
		MK_TRACE(MKTC_KICKRENDER_OVERLAP, MKTF_3D);
		mov		r4, #1;
		iaddu32		r3, r4.low, r3;
		MK_MEM_ACCESS_BPCACHE(stad) [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32OverlapCount)], r3;
	}
	KR_NoOverlap:
#endif

#if defined(SUPPORT_HW_RECOVERY)
	/* reset the Recovery counter */
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRTimerTicksSince3D)], #0;

	/* invalidate the signatures */
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], drc0;
	wdf		drc0;
	or		r2, r2, #0x2;
	MK_MEM_ACCESS_BPCACHE(stad) 	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], r2;
#endif

	MK_MEM_ACCESS_BPCACHE(stad)	 [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRTData)], R_RTData;

	/* Indicate a render is in progress */
	MK_LOAD_STATE(r2, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	or		r2, r2, #RENDER_IFLAGS_RENDERINPROGRESS;
	MK_STORE_STATE(ui32IRenderFlags, r2);
	MK_WAIT_FOR_STATE_STORE(drc0);

	/* Get the context priority */
	MK_LOAD_STATE(r1, ui323DCurrentPriority, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	/* Move chosen render context to start of complete list */
	MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevCompleteDevAddr)], drc0;
	MK_MEM_ACCESS(ldad)	r5, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextCompleteDevAddr)], drc1;
	wdf		drc0;
	and.testnz	p0, r4, r4;
	wdf		drc1;
	and.testnz	p1, r5, r5;
	p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextCompleteDevAddr)], r5;
	p1	MK_MEM_ACCESS(stad)	[r5, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevCompleteDevAddr)], r4;
	/* Update the Head and Tail if required */
	p0 ba	KR_NoHeadUpdate;
	{
		MK_STORE_STATE_INDEXED(sCompleteRenderContextHead, r1, r5, r2);
	}
	KR_NoHeadUpdate:
	p1 ba	KR_NoTailUpdate;
	{
		MK_STORE_STATE_INDEXED(sCompleteRenderContextTail, r1, r4, r3);
	}
	KR_NoTailUpdate:
	idf		drc0, st;
	wdf		drc0;

	/* Get the Head */
	MK_LOAD_STATE_INDEXED(r4, sCompleteRenderContextHead, r1, drc0, r2);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testnz	p0, r4, r4;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevCompleteDevAddr)], #0;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextCompleteDevAddr)], r4;
	p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevCompleteDevAddr)], r0;
	/* Update the Head */
	MK_STORE_STATE_INDEXED(sCompleteRenderContextHead, r1, r0, r2);
	/* Update the Tail if required */
	p0 ba	KR_NoTailUpdate2;
	{
		MK_STORE_STATE_INDEXED(sCompleteRenderContextTail, r1, r0, r3);
	}
	KR_NoTailUpdate2:
	idf		drc0, st;
	wdf		drc0;

#if (defined(FIX_HW_BRN_24304) || defined(FIX_HW_BRN_27510)) && defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	CONFIGURECACHE(#CC_FLAGS_3D_KICK);
#endif

	MK_TRACE(MKTC_KICKRENDER_ISP_START, MKTF_3D | MKTF_HW);

	ldr		r10, #EUR_CR_DPM_PAGE >> 2, drc0;
	wdf		drc0;
	and		r10, r10, #EUR_CR_DPM_PAGE_STATUS_3D_MASK;
	shr		r10, r10, #EUR_CR_DPM_PAGE_STATUS_3D_SHIFT;

	WRITEHWPERFCB(3D, 3D_START, GRAPHICS, r10, p0, k3d_start_1, r2, r3, r4, r5, r6, r7, r8, r9);

	/* Start the render */
	str		#(EUR_CR_ISP_START_RENDER >> 2), #EUR_CR_ISP_START_RENDER_PULSE_MASK;

#if defined(SGX_SUPPORT_HWPROFILING)
	/* Move the frame no. into the structure SGXMKIF_HWPROFILING */
	MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui323DFrameNum)], drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sHWProfilingDevVAddr)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(SGXMKIF_HWPROFILING.ui32Start3D)], r3;
#endif /* SGX_SUPPORT_HWPROFILING */

	MK_TRACE(MKTC_KICKRENDER_END, MKTF_3D);

	/* branch back to calling code */
	lapc;
}

/*****************************************************************************
 CheckTransferRunList routine	- check to see if there is a transfer task ready to be
			executed

 inputs:	p0 - TRUE = 3d operations in progress
 		r0 - return address
 		r1 - priority level to check
 temps:	p2, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11
*****************************************************************************/
CheckTransferRunList:
{
	MK_LOAD_STATE_INDEXED(r3, sTransferContextHead, r1, drc0, r2);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	/* Make sure it's OK to start a new render now */
	CTRL_CheckNextContext:
	and.testnz	p1, r3, r3;
	p1	ba		CTRL_NotEndOfList;
	{
		/* No transfer were ready at this priority */
		/* Return to Find3D routine */
		lapc;
	}
	CTRL_NotEndOfList:
	{
		/* Calculate base of Transfer CCB command */
		MK_MEM_ACCESS(ldad)	r2, [r3, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sCommon.ui32Flags)], drc0;
		MK_MEM_ACCESS(ldad)	r4, [r3, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sCCBCtlDevAddr)], drc0;
		MK_MEM_ACCESS(ldad)	r5, [r3, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sCCBBaseDevAddr)], drc1;
		wdf		drc0;
		
		and.testz p2, r2, #SGXMKIF_HWCFLAGS_SUSPEND;
		p2 ba CTRL_ContextNotSuspended;
		{
			MK_TRACE(MKTC_CTRL_CONTEXT_SUSPENDED, MKTF_TQ | MKTF_SCH);
			MK_TRACE_REG(r3, MKTF_TQ);
			ba		CTRL_TransferNotReady;
		}
		CTRL_ContextNotSuspended:		
		
		MK_MEM_ACCESS_BPCACHE(ldad)	r4, [r4, #DOFFSET(PVRSRV_SGX_CCB_CTL.ui32ReadOffset)], drc1;
		wdf		drc1;
		iaddu32	r4, r4.low, r5;

		/* get the cmd flags */
		MK_MEM_ACCESS_CACHED(ldad)	r2, [r4, #DOFFSET(SGXMKIF_TRANSFERCMD.ui32Flags)], drc0;
		wdf				drc0;

#if defined(SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH)
		/* If the blit was context switched, no need to check the dependencies */
		and.testnz	p1, r2, #SGXMKIF_TQFLAGS_CTXSWITCH;
		p1	ba	CTRL_TransferReady;
#endif
		{
			/* Check syncops values to see if they are at the required values
				SrcReads
				SrcWrites
				DstReads
				DstWrites
			*/
			MK_MEM_ACCESS(ldad) r8, [r4, #DOFFSET(SGXMKIF_TRANSFERCMD.sShared.ui32NumSrcSyncs)], drc0;
			wdf drc0;
			and.testz 	p1, r8, r8;
			p1	ba	CTRL_NoMoreSrcSync;
			mov	r9, #OFFSET(SGXMKIF_TRANSFERCMD.sShared.asSrcSyncs[0]);
			iaddu32	r9, r9.low, r4;

			mov	r10, #OFFSET(SGXMKIF_HWTRANSFERCONTEXT.asSrcSyncObjectSnapshot[0]);
			iaddu32	r10, r10.low, r3;
			CTRL_NextSrcSync:
			{
				/* asSrcSyncs[i].ui32ReadOpPending */
				
				MK_MEM_ACCESS_BPCACHE(ldad.f2)	r5, [r9, #0++], drc0;
				wdf		drc0;
				and.testz	p1, r6, r6;
				p1	ba		CTRL_NoSrcReadOpAddr;
				{
					MK_MEM_ACCESS_BPCACHE(ldad)	r7, [r6], drc0;
					wdf		drc0;
					xor.testz	p1, r7, r5;
					p1	ba		CTRL_SrcReadsReady;
					{
						MK_TRACE(MKTC_CTRL_SRCREADOPSBLOCKED, MKTF_TQ | MKTF_SO | MKTF_SCH);
						MK_TRACE_REG(r6, MKTF_TQ | MKTF_SO);
						MK_TRACE_REG(r7, MKTF_TQ | MKTF_SO);
						MK_TRACE_REG(r5, MKTF_TQ | MKTF_SO);
						ba	CTRL_TransferNotReady;
					}
					CTRL_SrcReadsReady:
				}
				CTRL_NoSrcReadOpAddr:

				/* if it's the firts chunk of a multi-blit sequence */
				and.testnz		p1, r2, #SGXMKIF_TQFLAGS_NOSYNCUPDATE;
				p1 and.testz	p1, r2, #SGXMKIF_TQFLAGS_KEEPPENDING;
				!p1 ba CTRL_SrcDontSnaphsot;
				{
					MK_MEM_ACCESS_BPCACHE(stad.f2)	[r10, #0++], r5;
					idf				drc0, st;
					wdf				drc0;
					/* Move to next sync-object -#8 to revert the
					   increment due to #0++ above
					 */
					mov	r5, #SIZEOF(PVRSRV_DEVICE_SYNC_OBJECT)-8;	
					iaddu32	r10, r5.low, r10;
				}
				CTRL_SrcDontSnaphsot:

				/* ui32WriteOpsPending Val and Completion Dev V Addr, previous #0++, would have got r9 incremented already to where we want it to be(pointing to ui32WriteOpsPendingVal */	
				MK_MEM_ACCESS_BPCACHE(ldad.f2)	r5, [r9, #0++], drc0;
				wdf		drc0;
				and.testz	p1, r6, r6;
				p1	ba		CTRL_NoSrcWriteOpAddr;
				{
					MK_MEM_ACCESS_BPCACHE(ldad)	r7, [r6], drc0;
					wdf		drc0;
					xor.testz	p1, r7, r5;
					p1	ba		CTRL_SrcWritesReady;
					{
						MK_TRACE(MKTC_CTRL_SRCWRITEOPSBLOCKED, MKTF_TQ | MKTF_SO | MKTF_SCH);
						MK_TRACE_REG(r6, MKTF_TQ | MKTF_SO);
						MK_TRACE_REG(r7, MKTF_TQ | MKTF_SO);
						MK_TRACE_REG(r5, MKTF_TQ | MKTF_SO);
						ba	CTRL_TransferNotReady;
					}
					CTRL_SrcWritesReady:
				}
				CTRL_NoSrcWriteOpAddr:

				/* Move past readOps2 */
				mov r5, #8;
				mov r7, r9;
				IADD32(r9, r7, r5, r6);

				/* decrement SrcSync count in r8 
				   and test for zero
				 */
				isub16.testnz	r8, p1, r8, #1;
				p1	ba	CTRL_NextSrcSync;
			}
			CTRL_NoMoreSrcSync:

	
			MK_MEM_ACCESS(ldad)	r8, [r4, #DOFFSET(SGXMKIF_TRANSFERCMD.sShared.ui32NumDstSyncs)], drc0;
			wdf	drc0;
			and.testz		p1, r8, r8;
			p1	ba	 CTRL_NoMoreDstSync;
			mov	r9, #OFFSET(SGXMKIF_TRANSFERCMD.sShared.asDstSyncs[0]);
			iaddu32	r9, r9.low, r4;
			mov	r10, #OFFSET(SGXMKIF_HWTRANSFERCONTEXT.asDstSyncObjectSnapshot[0]);
			iaddu32	r10, r10.low, r3;
			CTRL_NextDstSync:
			{
				/* asDstSyncs[i].ui32ReadOpPending */
	
				/* check the dest read ops */
				MK_MEM_ACCESS_BPCACHE(ldad.f2)	r5, [r9, #0++], drc0;
				wdf		drc0;
				and.testz	p1, r6, r6;
				p1	ba		CTRL_NoDestReadOpAddr;
				{
					MK_MEM_ACCESS_BPCACHE(ldad)	r7, [r6], drc0;
					wdf		drc0;
					xor.testz	p1, r7, r5;
					p1	ba		CTRL_DestReadsReady;
					{
						MK_TRACE(MKTC_CTRL_DSTREADOPSBLOCKED, MKTF_TQ | MKTF_SO | MKTF_SCH);
						MK_TRACE_REG(r6, MKTF_TQ | MKTF_SO);
						MK_TRACE_REG(r7, MKTF_TQ | MKTF_SO);
						MK_TRACE_REG(r5, MKTF_TQ | MKTF_SO);
						ba	CTRL_TransferNotReady;
					}
					CTRL_DestReadsReady:
				}
				CTRL_NoDestReadOpAddr:
	
				/* asSrcSyncs[i].ui32ReadOp2Pending */
				/* r5 was pointing at ui32WriteOpsPendingVal, advance to ui32ReadOps2PendingVal */
				mov	r6, #8;
				mov	r7, r9;
				IADD32(r9, r7, r6, r5);
				MK_MEM_ACCESS_BPCACHE(ldad.f2)	r5, [r9, #0++], drc0;
				wdf		drc0;
				and.testz	p1, r6, r6;
				p1	ba		CTRL_NoSrcReadOp2Addr;
				{
					MK_MEM_ACCESS_BPCACHE(ldad)	r7, [r6], drc0;
					wdf		drc0;
					xor.testz	p1, r7, r5;
					p1	ba		CTRL_SrcReads2Ready;
					{
						MK_TRACE(MKTC_CTRL_SRCREADOPS2BLOCKED, MKTF_TQ | MKTF_SO | MKTF_SCH);
						MK_TRACE_REG(r6, MKTF_TQ | MKTF_SO);
						MK_TRACE_REG(r7, MKTF_TQ | MKTF_SO);
						MK_TRACE_REG(r5, MKTF_TQ | MKTF_SO);
						ba	CTRL_TransferNotReady;
					}
					CTRL_SrcReads2Ready:
				}
				CTRL_NoSrcReadOp2Addr:

				/* Rewind back to ui32ReadOpsPendingVal */
				mov	r7, #16;
				mov r5, r9;

				ISUB32(r9, r5, r7, r6);

				/* check the dest write ops */
				MK_MEM_ACCESS_BPCACHE(ldad.f2)	r5, [r9, #0++], drc0;
				wdf		drc0;
				and.testz	p1, r6, r6;
				p1	ba		CTRL_NoDestWriteOpAddr;
				{
					MK_MEM_ACCESS_BPCACHE(ldad)	r7, [r6], drc0;
					wdf		drc0;
					xor.testz	p1, r7, r5;
					p1	ba		CTRL_DestReady;
					{
						MK_TRACE(MKTC_CTRL_DSTWRITEOPSBLOCKED, MKTF_TQ | MKTF_SO | MKTF_SCH);
						MK_TRACE_REG(r6, MKTF_TQ | MKTF_SO);
						MK_TRACE_REG(r7, MKTF_TQ | MKTF_SO);
						MK_TRACE_REG(r5, MKTF_TQ | MKTF_SO);
						ba	CTRL_TransferNotReady;
					}
					CTRL_DestReady:
				}
				CTRL_NoDestWriteOpAddr:
				/* if it's the firts chunk of a multi-blit sequence */
				and.testnz		p1, r2, #SGXMKIF_TQFLAGS_NOSYNCUPDATE;
				p1 and.testz	p1, r2, #SGXMKIF_TQFLAGS_KEEPPENDING;
				!p1 ba CTRL_DstDontSnapshot;
				{
					/* WriteOp */
					/* skip to write op */
					mov	r11, #OFFSET(PVRSRV_DEVICE_SYNC_OBJECT.ui32WriteOpsPendingVal);
					iaddu32	r10, r11.low, r10;
					MK_MEM_ACCESS_BPCACHE(stad.f2)	[r10, #0++], r5;
					idf				drc0, st;
					wdf				drc0;
				}
				CTRL_DstDontSnapshot:

				/* decrement DstSync count in r8 
				   and test for zero
				 */
				isub16.testnz	r8, p1, r8, #1;
				p1	ba	CTRL_NextDstSync;
			}
			CTRL_NoMoreDstSync:



			/* check the TA/TQ synchronization */
			MK_MEM_ACCESS_CACHED(ldad.f4)	r5, [r4, #DOFFSET(SGXMKIF_TRANSFERCMD.sShared.ui32TASyncWriteOpsPendingVal)-1], drc0;
			wdf		drc0;
			and.testz	p1, r6, r6;
			p1	ba	CTRL_TASyncOpReady;
			{
				/* Check this is ready to go */
				MK_MEM_ACCESS_BPCACHE(ldad)	r9, [r6], drc0; // load WriteOpsComplete
				MK_MEM_ACCESS_BPCACHE(ldad)	r10, [r8], drc0; // load ReadOpsComplete
				wdf		drc0;
				xor.testz	p1, r9, r5;	// Test WriteOpsComplete
				p1 xor.testz	p1, r10, r7; // Test ReadOpsComplete
				!p1	ba	CTRL_TransferNotReady;
			}
			CTRL_TASyncOpReady:

			/* check the 3D/TQ synchronization */
			MK_MEM_ACCESS_CACHED(ldad.f4)	r5, [r4, #DOFFSET(SGXMKIF_TRANSFERCMD.sShared.ui323DSyncWriteOpsPendingVal)-1], drc0;
			wdf		drc0;
			and.testz	p1, r6, r6;
			p1	ba	CTRL_3DSyncOpReady;
			{
				/* Check this is ready to go */
				MK_MEM_ACCESS_BPCACHE(ldad)	r9, [r6], drc0; 	// load WriteOpsComplete
				MK_MEM_ACCESS_BPCACHE(ldad)	r10, [r8], drc0; // load ReadOpsComplete
				wdf		drc0;
				xor.testz	p1, r9, r5;	// Test WriteOpsComplete
				p1 xor.testz	p1, r10, r7; 	// Test ReadOpsComplete
				!p1	ba	CTRL_TransferNotReady;
			}
			CTRL_3DSyncOpReady:

			ba	CTRL_TransferReady;
			{
				CTRL_TransferNotReady:
				MK_MEM_ACCESS(ldad)	r3, [r3, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sNextDevAddr)], drc0;
				wdf		drc0;
				ba		CTRL_CheckNextContext;
			}
		}
		CTRL_TransferReady:
		/* A transfer will be started, so setup pclink accordingly */
		mov	r6, pclink;
		mov	pclink, r0;
		mov	r0, r3;	/* Move context to r0 */
		mov	r2, r1;	/* Move priority to r2 */
		mov	r1, r4;	/* Move transfercmd to r1 */

#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
		/* Check if the TA is active if so make sure we only parallelise a single context */
		MK_LOAD_STATE(r3, ui32ITAFlags, drc0);
	#if defined(SGX_FEATURE_2D_HARDWARE)
		MK_LOAD_STATE(r4, ui32I2DFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		/* All modules use the same bit to indicate 'busy' */
		or		r3, r3, r4;
	#else
		MK_WAIT_FOR_STATE_LOAD(drc0);
	#endif
		and.testz	p1, r3, #TA_IFLAGS_TAINPROGRESS;
		p1	ba	CTRL_NoOperationInProgress;
		{
			/* As a TA is in progress the Head is the current active scene */
			/* Now load up the PDPhys and DirListBase0 */
			ldr		r4, #EUR_CR_BIF_DIR_LIST_BASE0 >> 2, drc0;
			/* note: the wdf follows in SGXMK_HWCONTEXT_GETDEVPADDR */
			SGXMK_HWCONTEXT_GETDEVPADDR(r5, r0, r3, drc0);

			xor.testz	p1, r5, r4;
			p1 ba CTRL_NoOperationInProgress;
			{
				MK_TRACE(MKTC_CTRL_TARC_DIFFERENT, MKTF_TQ | MKTF_SCH);
				mov	r1, r2; /* Restore r1 back, we are not going to kick the transfer, */
				mov	r0, pclink;
				mov	pclink, r6;
	#if defined(SGX_FEATURE_2D_HARDWARE)
				/* record the blocked priority, so nothing of lower priority can be started.
					NOTE: The calling code guarantees this context is >= to current blocked level. */
				MK_STORE_STATE(ui32BlockedPriority, r2);
				MK_WAIT_FOR_STATE_STORE(drc0);
				
				/* if the TA is not busy, the 2D must be! 
					Given there is nothing we can do to stop it, just exit */
				MK_LOAD_STATE(r4, ui32ITAFlags, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);
				and.testz	p1, r4, #TA_IFLAGS_TAINPROGRESS;
				p1	ba	CTRL_End;
	#endif
				/* The memory contexts are different so only proceed if the transfer is of a higher priority */
				MK_LOAD_STATE(r3, ui32TACurrentPriority, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);
				isub16.testns	p1, r2, r3;
				p1 ba CTRL_End;
				/* The operation is higher priority so request the TA to stop */
				STARTTACONTEXTSWITCH(FTR);
				ba	CTRL_End;
			}
		}
		CTRL_NoOperationInProgress:
#endif
		/* If currently rendering, perform a ISP context switch */
		p0	ba		CTRL_RenderInProgress;
		{
			MK_MEM_ACCESS(ldad) r3, [r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sCommon.ui32Flags)], drc0;
			wdf		drc0;
			and.testz	p0, r3, #SGXMKIF_HWCFLAGS_DUMMYTRANSFER;
			/*
				If we do kick render, No need for a lapc as KickTransferRender will
				branch straight back to calling code
			*/
			p0 	ba	KickTransferRender;
			/* if we get here then we are going to dummy process the TRANSFERCMD */
			/* clear the flag */
			and	r3, r3, ~#SGXMKIF_HWCFLAGS_DUMMYTRANSFER;
			MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sCommon.ui32Flags)], r3;
			ba	DummyProcessTransfer;
		}
		CTRL_RenderInProgress:
#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH)
		/* Check whether a PixelBE_EOR is set, if so don't switch wait for current render to finish */
		ldr		r0, #EUR_CR_EVENT_STATUS >> 2, drc0;
		mov		r1, #EUR_CR_EVENT_STATUS_PIXELBE_END_RENDER_MASK;
		wdf		drc0;
		and.testnz	p1, r0, r1;
		p1	ba		CTRL_End;
#endif
		{
			/* Start the ISP context store,
				r0 - next rendercontext */
			START3DCONTEXTSWITCH(CTRL);
		}
	}
	CTRL_End:
	/* branch back to calling code */
	lapc;
}

/*****************************************************************************
 KickTransferRender routine	- performs anything that is required before actually
 			  starting the render

 inputs:	r0 - context of transfer about to be started
			r1 - CMD of the transfer about to be started
			r2 - priority of the transfer about to be started
 temps:		r2, r3, r4, r5, r6, r7, r8, r9
*****************************************************************************/
KickTransferRender:
{
	MK_TRACE(MKTC_KICKTRANSFERRENDER_START, MKTF_TQ);

	/* Reset the priority scheduling level block, as it is no longer blocked */
	MK_STORE_STATE(ui32BlockedPriority, #(SGX_MAX_CONTEXT_PRIORITY - 1));

	/* Save pointer to context and CCB command */
	MK_STORE_STATE(s3DContext, r0);
	MK_STORE_STATE(sTransferCCBCmd, r1);
	MK_STORE_STATE(ui323DCurrentPriority, r2);
	MK_WAIT_FOR_STATE_STORE(drc0);

	/* No need to do any dpm context stores/loads, as DPM is not used */
	/* Ensure the memory context is setup */
	SETUPDIRLISTBASE(KTR, r0, #SETUP_BIF_TQ_REQ);

#if defined(SGX_FEATURE_MP)
#if defined(FIX_HW_BRN_30710) || defined(FIX_HW_BRN_30764)
	SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_SOFT_RESET >> 2, #EUR_CR_MASTER_SOFT_RESET_IPF_RESET_MASK);
	SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_SOFT_RESET >> 2, #0);
#endif
#endif /* SGX_FEATURE_MP */
#if defined(FIX_HW_BRN_31964)
	/* Reset the ISP2 in case some cores have no work */
	SGXMK_SPRV_WRITE_REG(#EUR_CR_SOFT_RESET >> 2, #EUR_CR_SOFT_RESET_ISP2_RESET_MASK);
	SGXMK_SPRV_WRITE_REG(#EUR_CR_SOFT_RESET >> 2, #0);
#endif

	/* Save the PID for easy retrieval later */
	MK_MEM_ACCESS(ldad)	r4, [r0, HSH(DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.ui32PID))], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui323DPID)], r4;
	MK_TRACE(MKTC_KICKTRANSFERRENDER_PID, MKTF_PID);
	MK_TRACE_REG(r4, MKTF_PID);

	/* get the cmd flags */
	MK_MEM_ACCESS_CACHED(ldad)	r6, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.ui32Flags)], drc0;
	wdf		drc0;

	/* if it's the last chunk of a multi-blit sequence */
	and.testnz		p0, r6, #SGXMKIF_TQFLAGS_KEEPPENDING;
	p0 and.testz	p0, r6, #SGXMKIF_TQFLAGS_NOSYNCUPDATE;
	!p0 ba KTR_DontWriteBackSnapshot;
	{
		
		mov r9,		#OFFSET(SGXMKIF_TRANSFERCMD.sShared.asSrcSyncs[0]);
		iaddu32		r9, r9.low, r1;
		mov		r10, #OFFSET(SGXMKIF_HWTRANSFERCONTEXT.asSrcSyncObjectSnapshot[0]);
		iaddu32		r10, r10.low, r0;

		MK_MEM_ACCESS_CACHED(ldad)	r8, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.sShared.ui32NumSrcSyncs)], drc0;
		wdf		drc0;
		and.testz 	p0, r8, r8;
		p0	ba	KTR_NoMoreSrcSnapshots;	
		KTR_NextSrcSnapshot:
		{
		
		
			MK_MEM_ACCESS_BPCACHE(ldad.f2)	r2, [r10, #0++], drc0;
			wdf				drc0;

			/* Increments away to next Sync Object in Array */
			mov				r11, #SIZEOF(PVRSRV_DEVICE_SYNC_OBJECT)-8;  /* Take count of the ++ in the ldad */
			iaddu32				r10, r11.low, r10; 

			MK_MEM_ACCESS_BPCACHE(stad.f2)	[r9, #0++], r2;
			idf				drc0, st;
			wdf				drc0;
			/* Increments away to next Sync Object in Array */
			mov				r11, #SIZEOF(PVRSRV_DEVICE_SYNC_OBJECT)-8;  /* Take count of the ++ in the ldad */
			iaddu32				r9, r11.low, r9;
			isub16.testnz			r8, p0, r8, #1;
			p0 ba KTR_NextSrcSnapshot;
		}
		KTR_NoMoreSrcSnapshots:

		mov	r9,	#OFFSET(SGXMKIF_TRANSFERCMD.sShared.asDstSyncs[0]);
		iaddu32	r9, r9.low, r1;
		mov	r10, #OFFSET(SGXMKIF_HWTRANSFERCONTEXT.asDstSyncObjectSnapshot[0]);
		iaddu32	r10, r10.low, r0;
		
		MK_MEM_ACCESS_CACHED(ldad)	r8, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.sShared.ui32NumDstSyncs)], drc0;
		wdf		drc0;
		and.testz 	p0, r8, r8;
		p0	ba	KTR_NoMoreDstSnapshots;	
		KTR_NextDstSnapshot:
		{
			mov					r11, #OFFSET(PVRSRV_DEVICE_SYNC_OBJECT.ui32WriteOpsPendingVal);
			iaddu32					r10, r11.low, r10;
			MK_MEM_ACCESS_BPCACHE(ldad.f2)		r4, [r10, #0++], drc1;
			wdf					drc1;

			iaddu32					r9, r11.low, r9; 
			MK_MEM_ACCESS_BPCACHE(stad.f2)		[r9, #0++], r4;
			idf					drc1, st;
			wdf					drc1;

			/* Move both pointers on by 2 words to get passed readops2 */
			mov	r4, #8;
			mov	r5, r10;
			IADD32(r10, r4, r5, r3);
			mov	r5, r9;
			IADD32(r9, r4, r5, r3);

			isub16.testnz r8, p0, r8, #1;
			p0 ba KTR_NextSrcSnapshot;
		}
		KTR_NoMoreDstSnapshots:
	}
	KTR_DontWriteBackSnapshot:

	/* if we are requested to do a dummy transfer */
	and.testz		p0, r6, #SGXMKIF_TQFLAGS_DUMMYTRANSFER;
	p0 ba KTR_DummyNotRequested;
	{
		ba	DummyProcessTransfer; 
		lapc;
	}
	KTR_DummyNotRequested:
	

	/* Set up the pixel task part of the DMS control. */
	MK_MEM_ACCESS_CACHED(ldad)	r5, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.ui32NumPixelPartitions)], drc1;
	ldr		r4, #EUR_CR_DMS_CTRL >> 2, drc0;
	wdf		drc0;
	
	/* Start loading the numupdates */
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.ui32NumUpdates)], drc0;
	
	and		r4, r4, ~#EUR_CR_DMS_CTRL_MAX_NUM_PIXEL_PARTITIONS_MASK;
	wdf		drc1;
	or		r4, r4, r5;
	str		#EUR_CR_DMS_CTRL >> 2, r4;

	/* Carry out any updates required */
	wdf		drc0;
	and.testz	p1, r2, r2;
	p1	ba		KTR_NoUpdates;
	{
		SWITCHEDMTO3D();
		mov			r3, #OFFSET(SGXMKIF_TRANSFERCMD.sUpdates);
		iadd32		r3, r3.low, r1;	// start
		mov			r4, #(2 * SIZEOF(IMG_UINT32));
		imae		r2, r2.low, r4.low, r3, u32; // finish

		MK_MEM_ACCESS_CACHED(ldad.f2)	r4, [r3, #0++], drc0;
		KTR_NextUpdate:
		{
			xor.testnz	p0, r2, r3;
			wdf		drc0;
			MK_MEM_ACCESS_BPCACHE(stad)	[r4], r5;
			p0	MK_MEM_ACCESS_CACHED(ldad.f2)	r4, [r3, #0++], drc0;
			p0	ba		KTR_NextUpdate;
		}
		/* make sure all the updates are in memory before starting the transfer */
		idf		drc1, st;
		wdf		drc1;
		SWITCHEDMBACK();
	}
	KTR_NoUpdates:

	/* Load the HW registers */
	mov		r2, #OFFSET(SGXMKIF_TRANSFERCMD.sHWRegs);
	iaddu32	r2, r2.low, r1;
	
	/* Start loading 1st batch of registers */
	MK_MEM_ACCESS_CACHED(ldad.f6)		r3, [r2, #0++], drc0;
	/* Start loading 2nd batch of registers */
	MK_MEM_ACCESS_CACHED(ldad.f6)		r9, [r2, #0++], drc1;
	
	/* Don't deallocate parameter memory */
	str	#EUR_CR_DPM_3D_DEALLOCATE >> 2, #0;
	str	#EUR_CR_ISP_ZLSCTL >> 2, #0;
	
	/* Wait for 1st batch to load */
	wdf	drc0;
	str	#EUR_CR_ISP_BGOBJTAG >> 2, r3;
	str	#EUR_CR_ISP_BGOBJ >> 2,	r4;
	str	#EUR_CR_ISP_BGOBJDEPTH >> 2, r5;
	str	#EUR_CR_ISP_RENDER >> 2, r6;
	str	#EUR_CR_ISP_RGN_BASE >> 2, r7;
	str	#EUR_CR_ISP_IPFMISC >> 2, r8;
	
	/* Start loading 3rd batch */
#if !defined(EUR_CR_4Kx4K_RENDER)
	#if !defined(SGX_FEATURE_PIXEL_PDSADDR_FULL_RANGE)
		MK_MEM_ACCESS_CACHED(ldad.f4)		r3, [r2, #0++], drc0;
	#else
		MK_MEM_ACCESS_CACHED(ldad.f3)		r3, [r2, #0++], drc0;
	#endif
#else
	#if !defined(SGX_FEATURE_PIXEL_PDSADDR_FULL_RANGE)
		MK_MEM_ACCESS_CACHED(ldad.f5)		r3, [r2, #0++], drc0;
	#else
		MK_MEM_ACCESS_CACHED(ldad.f4)		r3, [r2, #0++], drc0;
	#endif
#endif
	/* Wait for 2nd batch to load */
	wdf	drc1;
	str	#EUR_CR_BIF_3D_REQ_BASE >> 2, r9;
	str	#EUR_CR_3D_AA_MODE >> 2, r10;
	str	#EUR_CR_ISP_MULTISAMPLECTL >> 2, r11;
	str	#EUR_CR_EVENT_PIXEL_PDS_EXEC >> 2, r12;
	str	#EUR_CR_EVENT_PIXEL_PDS_DATA >> 2, r13;
	str	#EUR_CR_EVENT_PIXEL_PDS_INFO >> 2, r14;

#if defined(SUPPORT_DISPLAYCONTROLLER_TILING)
	MK_MEM_ACCESS_CACHED(ldad)	r9, [r2, #1++], drc1;
#endif

#if defined(EUR_CR_DOUBLE_PIXEL_PARTITIONS)
	MK_MEM_ACCESS_CACHED(ldad)	r10, [r2, #1++], drc1;
#endif

#if defined(SGX_FEATURE_TAG_YUV_TO_RGB)
	/* Start loading bLoadYUVCoefficients */
	MK_MEM_ACCESS_CACHED(ldad)	r13, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.bLoadYUVCoefficients)], drc1;
#endif

	/* Wait for 3rd batch to load */
	wdf	drc0;
	str	#EUR_CR_ISP_RENDBOX1 >> 2, r3;
	str	#EUR_CR_ISP_RENDBOX2 >> 2, r4;
	str	#EUR_CR_PIXELBE >> 2,	r5;
#if !defined(EUR_CR_4Kx4K_RENDER)
	#if !defined(SGX_FEATURE_PIXEL_PDSADDR_FULL_RANGE)
		str	#EUR_CR_PDS_EXEC_BASE >> 2, r6;
	#endif
#else
	str	#EUR_CR_4Kx4K_RENDER >> 2,	r6;
	#if !defined(SGX_FEATURE_PIXEL_PDSADDR_FULL_RANGE)
		str	#EUR_CR_PDS_EXEC_BASE >> 2, r7;
	#endif
#endif

	/* Wait for bLoadYUVCoefficients */
	wdf	drc1;

#if defined(SUPPORT_DISPLAYCONTROLLER_TILING)
	SGXMK_SPRV_WRITE_REG(#EUR_CR_BIF_TILE0 >> 2, r9);
#endif

#if defined(EUR_CR_DOUBLE_PIXEL_PARTITIONS)
	str	#EUR_CR_DOUBLE_PIXEL_PARTITIONS >> 2, r10;
	/* 
	The number of output partitions is chosen, to minimize
	fragmentation based on possibility of double and
	DMS_CTRL2 is already programmed at initialisation time
	*/
#endif

#if defined(SGX_FEATURE_TAG_YUV_TO_RGB)
	and.testz	p0, r13, r13;
	p0 ba		KTR_AfterYUV;
	{
#if !defined(SGX_FEATURE_TAG_YUV_TO_RGB_HIPREC)
		MK_MEM_ACCESS_CACHED(ldad.f5)	r3, [r2, #0++], drc0;
		MK_MEM_ACCESS_CACHED(ldad.f6)	r9, [r2, #0++], drc1;
#else
		MK_MEM_ACCESS_CACHED(ldad.f6)	r3, [r2, #0++], drc0;
		MK_MEM_ACCESS_CACHED(ldad.f6)	r9, [r2, #0++], drc1;
#endif

		wdf	drc0;
	
		str	#EUR_CR_TPU_YUV00 >> 2, r3;
		str	#EUR_CR_TPU_YUV01 >> 2, r4;
		str	#EUR_CR_TPU_YUV02 >> 2, r5;
		str	#EUR_CR_TPU_YUV03 >> 2, r6;
		str	#EUR_CR_TPU_YUV04 >> 2, r7;

#if defined(SGX_FEATURE_TAG_YUV_TO_RGB_HIPREC)
		str	#EUR_CR_TPU_YUV05 >> 2, r8;
#endif
		wdf drc1;

		str	#EUR_CR_TPU_YUV10 >> 2, r9;
		str	#EUR_CR_TPU_YUV11 >> 2, r10;
		str	#EUR_CR_TPU_YUV12 >> 2, r11;
		str	#EUR_CR_TPU_YUV13 >> 2, r12;
		str	#EUR_CR_TPU_YUV14 >> 2, r13;

#if !defined(SGX_FEATURE_TAG_YUV_TO_RGB_HIPREC)
		str	#EUR_CR_TPU_YUVSCALE >> 2, r14;
#else
		str	#EUR_CR_TPU_YUV15 >> 2, r14;
#endif

	}

	KTR_AfterYUV:
#endif
#if defined(SGX_FEATURE_TAG_LUMAKEY)
	/* Re-load the HW registers because r2 will vary depending upon whether the branch was taken or not */
	mov		r2, #OFFSET(SGXMKIF_TRANSFERCMD.sHWRegs.ui32LumaKeyCoeff0);
	iaddu32	r2, r2.low, r1;

	/* Start loading bLoadLumaKeyCoefficients */
	MK_MEM_ACCESS_CACHED(ldad)	r13, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.bLoadLumaKeyCoefficients)], drc1;
#endif

	/* Start loading bLoadFIRCoefficients */
	MK_MEM_ACCESS_CACHED(ldad)	r14, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.bLoadFIRCoefficients)], drc1;

	/* Wait for bLoadLumaKeyCoefficients & bLoadFIRCoefficients */
	wdf	drc1;

#if defined(SGX_FEATURE_TAG_LUMAKEY)
	and.testz	p0, r13, r13;
	p0 ba		KTR_AfterLumaKey;
	{
		MK_MEM_ACCESS_CACHED(ldad.f3)	r3, [r2, #0++], drc0;

		wdf	drc0;

		str	#EUR_CR_TPU_LUMA0 >> 2, r3;
		str	#EUR_CR_TPU_LUMA1 >> 2, r4;
		str	#EUR_CR_TPU_LUMA2 >> 2, r5;
	}

	KTR_AfterLumaKey:
	/* Re-load the HW registers because r2 will vary depending upon whether the branch was taken or not */
	mov		r2, #OFFSET(SGXMKIF_TRANSFERCMD.sHWRegs.ui32FIRHFilterTable);
	iaddu32	r2, r2.low, r1;
#endif

	/* Update the FIR co-efficients? */
	and.testz	p0, r14, r14;
	p0	ba		KTR_AfterFIR;
	{
		MK_MEM_ACCESS_CACHED(ldad.f8)	r3, [r2, #0++], drc0;
#if defined(SGX_FEATURE_USE_HIGH_PRECISION_FIR_COEFF)
		MK_MEM_ACCESS_CACHED(ldad.f4)	r11, [r2, #0++], drc1;
#else
		MK_MEM_ACCESS_CACHED(ldad.f2)	r11, [r2, #0++], drc1;
#endif
		wdf	drc0;

		str	#EUR_CR_USE_FILTER_TABLE >> 2, r3;
		str	#EUR_CR_USE_FILTER0_LEFT >> 2, r4;
		str	#EUR_CR_USE_FILTER0_RIGHT >> 2, r5;
		str	#EUR_CR_USE_FILTER0_EXTRA >> 2, r6;
		str	#EUR_CR_USE_FILTER1_LEFT >> 2, r7;
		str	#EUR_CR_USE_FILTER1_RIGHT >> 2, r8;
		str	#EUR_CR_USE_FILTER1_EXTRA >> 2, r9;
		str	#EUR_CR_USE_FILTER2_LEFT >> 2, r10;

		wdf	drc1;
		str	#EUR_CR_USE_FILTER2_RIGHT >> 2, r11;
		str	#EUR_CR_USE_FILTER2_EXTRA >> 2, r12;
#if defined(SGX_FEATURE_USE_HIGH_PRECISION_FIR_COEFF)
		str	#EUR_CR_USE_FILTER0_CENTRE >> 2, r13;
		str	#EUR_CR_USE_FILTER1_CENTRE >> 2, r14;
#endif
	}
	
	KTR_AfterFIR:

#if defined(SGX_FEATURE_SYSTEM_CACHE)
	#if defined(SGX_FEATURE_MP)
		INVALIDATE_SYSTEM_CACHE(#EUR_CR_MASTER_SLC_CTRL_INVAL_DM_PIXEL_MASK, #EUR_CR_MASTER_SLC_CTRL_FLUSH_DM_PIXEL_MASK);
	#else
		INVALIDATE_SYSTEM_CACHE();
	#endif /* SGX_FEATURE_MP */
#endif

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
	INVALIDATE_EXT_SYSTEM_CACHE();
#endif /* SUPPORT_EXTERNAL_SYSTEM_CACHE */

	/* Invalidate PDS cache */
	str		#EUR_CR_PDS_INV1 >> 2, #EUR_CR_PDS_INV1_DSC_MASK;
	INVALIDATE_PDS_CODE_CACHE();

	/* Invalidate the TSP cache	*/
	str	#EUR_CR_TSP_PARAMETER_CACHE >> 2, #EUR_CR_TSP_PARAMETER_CACHE_INVALIDATE_MASK;

#if defined(FIX_HW_BRN_24549)
	/* Pause the USSE */
	mov 	r16, #(USE_IDLECORE_TA_REQ | USE_IDLECORE_USSEONLY_REQ);
	PVRSRV_SGXUTILS_CALL(IdleCore);
#endif

#if defined(EUR_CR_CACHE_CTRL)
	/* Invalidate the MADD cache */
	ldr		r2, #EUR_CR_CACHE_CTRL >> 2, drc0;
	wdf		drc0;
	or		r2, r2, #EUR_CR_CACHE_CTRL_INVALIDATE_MASK;
	str		#EUR_CR_CACHE_CTRL >> 2, r2;
	
	ENTER_UNMATCHABLE_BLOCK;
	/* Wait for the MADD cache to be invalidated */
	KTR_MADDCacheNotInvalidated:
	{
		ldr		r2, #EUR_CR_EVENT_STATUS >> 2, drc0;
		wdf		drc0;
		shl.testns	 p0, r2, #(31 - EUR_CR_EVENT_STATUS_MADD_CACHE_INVALCOMPLETE_SHIFT);
		p0	ba		KTR_MADDCacheNotInvalidated;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the bit */
	mov		r2, #EUR_CR_EVENT_HOST_CLEAR_MADD_CACHE_INVALCOMPLETE_MASK;
	str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r2;
#else
	/* Invalidate the data cache */
	str		#EUR_CR_DCU_ICTRL >> 2, #EUR_CR_DCU_ICTRL_INVALIDATE_MASK;

	mov		r3, #EUR_CR_EVENT_STATUS2_DCU_INVALCOMPLETE_MASK;

	ENTER_UNMATCHABLE_BLOCK;
	/* Wait for the data cache to be invalidated */
	KTR_DataCacheNotInvalidated:
	{
		ldr		r2, #EUR_CR_EVENT_STATUS2 >> 2, drc0;
		wdf		drc0;
		and.testz	 p0, r2, r3;
		p0	ba		KTR_DataCacheNotInvalidated;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the bit */
	mov		r2, #EUR_CR_EVENT_HOST_CLEAR2_DCU_INVALCOMPLETE_MASK;
	str		#EUR_CR_EVENT_HOST_CLEAR2 >> 2, r2;

	/* Invalidate the texture cache */
	str		#EUR_CR_TCU_ICTRL >> 2, #EUR_CR_TCU_ICTRL_INVALTCU_MASK;

	ENTER_UNMATCHABLE_BLOCK;
	/* Wait for the texture cache to be invalidated */
	KTR_TextureCacheNotInvalidated:
	{
		ldr		r2, #EUR_CR_EVENT_STATUS >> 2, drc0;
		wdf		drc0;
		shl.testns	 p0, r2, #(31 - EUR_CR_EVENT_STATUS_TCU_INVALCOMPLETE_SHIFT);
		p0	ba		KTR_TextureCacheNotInvalidated;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the bit */
	mov		r2, #EUR_CR_EVENT_HOST_CLEAR_TCU_INVALCOMPLETE_MASK;
	str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r2;
#endif

#if defined(FIX_HW_BRN_24549)
	RESUME(r4);
#endif

#if defined(SGX_FEATURE_WRITEBACK_DCU)
	/* Invalidate the DCU cache. */
	ci.global	#SGX545_USE_OTHER2_CFI_DATAMASTER_PIXEL, #0, #SGX545_USE1_OTHER2_CFI_LEVEL_L0L1L2;
#endif

	/* Invalidate the USE cache. */
	INVALIDATE_USE_CACHE(KTR_, p0, r2, #SETUP_BIF_3D_REQ);

	/* Wait for PDS cache to be invalidated */
	ENTER_UNMATCHABLE_BLOCK;
	KTR_PDSCacheNotFlushed:
	{
		ldr		r2, #EUR_CR_PDS_CACHE_STATUS >> 2, drc0;
		wdf		drc0;
		and		r2, r2, #EURASIA_PDS_CACHE_PIXEL_STATUS_MASK;
		xor.testnz	p0, r2, #EURASIA_PDS_CACHE_PIXEL_STATUS_MASK;
		p0	ba		KTR_PDSCacheNotFlushed;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the PDS cache flush event. */
	str		#EUR_CR_PDS_CACHE_HOST_CLEAR >> 2, #EURASIA_PDS_CACHE_PIXEL_CLEAR_MASK;

#if (defined(FIX_HW_BRN_24304) || defined(FIX_HW_BRN_27510)) && defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	CONFIGURECACHE(#CC_FLAGS_3D_KICK);
#endif

#if defined(SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH)
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.ui32Flags)], drc0;
	wdf		drc0;
	and.testz	p0, r2, #SGXMKIF_TQFLAGS_CTXSWITCH;
	p0	ba	KTR_NoRenderResume;
	{
		/* Now setup the PDS state resume addresses and zls resume addr  */
		MK_MEM_ACCESS(ldad.f2)	r2, [r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.asPDSStateBaseDevAddr[0][0])-1], drc0;
		wdf		drc0;
		str		#EUR_CR_PDS0_CONTEXT_RESUME >> 2, r2;
		str		#EUR_CR_PDS1_CONTEXT_RESUME >> 2, r3;
		/* Setup the registers to reload the state */
		str		#EUR_CR_ISP2_CONTEXT_RESUME >> 2, #0;

		MK_MEM_ACCESS(ldad.f4) r2, [r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.ui32NextRgnBase)-1], drc0;
		wdf		drc0;
		str		#EUR_CR_ISP_RGN_BASE >> 2, r2;
		str		#EUR_CR_ISP_START >> 2, r3;
		str		#EUR_CR_ISP2_CONTEXT_RESUME2 >> 2, r4;
		str		#EUR_CR_ISP2_CONTEXT_RESUME3 >> 2, r5;
		/* Indicate that this is a resumed render */
		ldr		r2, #EUR_CR_ISP_RENDER >> 2, drc0;
		wdf		drc0;
		or		r2, r2, #EUR_CR_ISP_RENDER_CONTEXT_RESUMED_MASK;
		str		#EUR_CR_ISP_RENDER >> 2, r2;
	}
	KTR_NoRenderResume:
#endif

	/* write the TQ fence that identifies the kick in the out2.txt comments*/
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.ui32FenceID)], drc0;
	wdf		drc0;
	MK_TRACE(MKTC_KTR_TQFENCE, MKTF_TQ);
	MK_TRACE_REG(r2, MKTF_TQ); 


	WRITEHWPERFCB(3D, TRANSFER_START, GRAPHICS, #0, p0, k3d_start_2, r2, r3, r4, r5, r6, r7, r8, r9);

#if defined(SUPPORT_HW_RECOVERY)
	/* reset the Recovery counter */
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRTimerTicksSince3D)], #0;

	/* invalidate the signatures */
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], drc0;
	wdf		drc0;
	or		r2, r2, #0x2;
	MK_MEM_ACCESS_BPCACHE(stad) 	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], r2;
#endif

	/* Indicate a render is in progress */
	MK_LOAD_STATE(r2, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	or		r2, r2, #(RENDER_IFLAGS_RENDERINPROGRESS | RENDER_IFLAGS_TRANSFERRENDER);
	MK_STORE_STATE(ui32IRenderFlags, r2);
	MK_WAIT_FOR_STATE_STORE(drc0);

	MK_LOAD_STATE(r1, ui323DCurrentPriority, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	/* Move chosen render context to start of complete list */
	MK_MEM_ACCESS(ldad)	r3, [r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sPrevDevAddr)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sNextDevAddr)], drc0;
	and.testnz	p0, r3, r3;
	wdf		drc0;
	and.testnz	p1, r4, r4;
	p0	MK_MEM_ACCESS(stad)	[r3, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sNextDevAddr)], r4;
	p1	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sPrevDevAddr)], r3;
	p0 ba	KTR_NoTCHeadUpdate;
	{
		MK_STORE_STATE_INDEXED(sTransferContextHead, r1, r4, r2);
	}
	KTR_NoTCHeadUpdate:
	p1 ba	KTR_NoTCTailUpdate;
	{
		MK_STORE_STATE_INDEXED(sTransferContextTail, r1, r3, r2);
	}
	KTR_NoTCTailUpdate:
	idf		drc0, st;
	wdf		drc0;

	MK_LOAD_STATE_INDEXED(r3, sTransferContextHead, r1, drc0, r2);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testnz	p0, r3, r3;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sPrevDevAddr)], #0;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sNextDevAddr)], r3;
	p0	MK_MEM_ACCESS(stad)	[r3, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sPrevDevAddr)], r0;
	MK_STORE_STATE_INDEXED(sTransferContextHead, r1, r0, r2);
	p0	ba	KTR_NoTCTailUpdate2;
	{
		MK_STORE_STATE_INDEXED(sTransferContextTail, r1, r0, r2);
	}
	KTR_NoTCTailUpdate2:
	idf		drc0, st;
	wdf		drc0;

	/* Start the render */
	str		#EUR_CR_ISP_START_RENDER >> 2, #EUR_CR_ISP_START_RENDER_PULSE_MASK;

	MK_TRACE(MKTC_KICKTRANSFERRENDER_ISP_START, MKTF_TQ | MKTF_HW);

#if defined(SGX_SUPPORT_HWPROFILING)
	/* Move the frame no. into the structure SGXMKIF_HWPROFILING */
	MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui323DFrameNum)], drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r4, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sHWProfilingDevVAddr)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(stad)	[r4, #DOFFSET(SGXMKIF_HWPROFILING.ui32Start3D)], r3;
#endif /* SGX_SUPPORT_HWPROFILING */

	MK_TRACE(MKTC_KICKTRANSFERRENDER_END, MKTF_TQ);
	lapc;
}

/*****************************************************************************
 TransferRenderFinished routine	- process the transfer finished event

 inputs:
 temps:
*****************************************************************************/
TransferRenderFinished:
{
	MK_TRACE(MKTC_TRANSFERRENDERFINISHED_START, MKTF_TQ);

#if (defined(FIX_HW_BRN_24304) || defined(FIX_HW_BRN_27510)) && defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	CONFIGURECACHE(#CC_FLAGS_HW_FINISH);
#endif

	shl.tests	p2, r0, #31 - RENDER_IFLAGS_TAWAITINGFORMEM_BIT;
	/* Clear the render flags and check for partial renders completing */
	and		r1, r0, #~(RENDER_IFLAGS_RENDERINPROGRESS | \
					   RENDER_IFLAGS_SPMRENDER | \
					   RENDER_IFLAGS_HALTRENDER | \
					   RENDER_IFLAGS_HALTRECEIVED | \
					   RENDER_IFLAGS_TAWAITINGFORMEM | \
					   RENDER_IFLAGS_TRANSFERRENDER | \
					   RENDER_IFLAGS_HWLOCKUP_MASK);
	MK_STORE_STATE(ui32IRenderFlags, r1);
	MK_WAIT_FOR_STATE_STORE(drc0);

	/*
		The TA has run out of memory, so we need to restart it.
	*/
	p2	bal		TARestartWithoutRender;

	/* write out status here */
	MK_LOAD_STATE(r1, sTransferCCBCmd, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	MK_MEM_ACCESS_CACHED(ldad.f2)	r2, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.sShared.ui32TASyncWriteOpsPendingVal)-1], drc0;
	wdf		drc0;
	and.testz	p0, r3, r3;
	p0	ba	TRF_NoTASyncWriteOp;
	{
		mov	r4, #1;
		iaddu32 r2, r4.low, r2;
		MK_MEM_ACCESS_BPCACHE(stad)	[r3], r2;
		idf	drc0, st;
	}
	TRF_NoTASyncWriteOp:

	MK_MEM_ACCESS_CACHED(ldad.f2)	r2, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.sShared.ui323DSyncWriteOpsPendingVal)-1], drc0;
	wdf		drc0;
	and.testz	p0, r3, r3;
	p0	ba	TRF_No3DSyncWriteOp;
	{
		mov	r4, #1;
		iaddu32 r2, r4.low, r2;
		MK_MEM_ACCESS_BPCACHE(stad)	[r3], r2;
		idf	drc0, st;
	}
	TRF_No3DSyncWriteOp:

	MK_MEM_ACCESS_CACHED(ldad)	r0, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.sShared.ui32NumStatusVals)], drc0;
	wdf		drc0;
	and.testz	p0, r0, r0;
	p0	ba		TRF_NoStatusVals;
	{
		MK_TRACE(MKTC_TRF_UPDATESTATUSVALS, MKTF_TQ | MKTF_SV);

		mov			r2, #OFFSET(SGXMKIF_TRANSFERCMD.sShared.sCtlStatusInfo);
		iadd32		r3, r2.low, r1;	// start
		imae		r2, r0.low, #SIZEOF(CTL_STATUS), r3, u32; // finish

		MK_MEM_ACCESS_CACHED(ldad.f2)	r4, [r3, #0++], drc0;
		TRF_NextStatusVal:
		{
			xor.testnz	p0, r2, r3;
			wdf		drc0;
			MK_MEM_ACCESS_BPCACHE(stad)	[r4], r5;

			MK_TRACE_REG(r4, MKTF_TQ | MKTF_SV);
			MK_TRACE_REG(r5, MKTF_TQ | MKTF_SV);

			p0	MK_MEM_ACCESS_CACHED(ldad.f2)	r4, [r3, #0++], drc0;
			p0	ba		TRF_NextStatusVal;
		}
		MK_TRACE(MKTC_TRF_UPDATESTATUSVALS_DONE, MKTF_TQ | MKTF_SV);
	}
	TRF_NoStatusVals:

	/* Update the surface sync ops */
	bal		UpdateTransferSurfaceOps;

	/* Load the HWTransferContext */
	MK_LOAD_STATE(r0, s3DContext, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	/* move on the TransferCCB */
	bal		AdvanceTransferCCBROff;

	/* test the count */
	and.testnz	p1, r2, r2;

	MK_LOAD_STATE(r1, ui323DCurrentPriority, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	/* Remove the current render context from the head of the transfer render contexts list */
	MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sNextDevAddr)], drc0;
	wdf		drc0;
	MK_STORE_STATE_INDEXED(sTransferContextHead, r1, r4, r2);

	and.testz	p0, r4, r4;
	!p0	ba	TRF_ListNotEmpty;
	{
		MK_STORE_STATE_INDEXED(sTransferContextTail, r1, #0, r3);
	}
	TRF_ListNotEmpty:
	!p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sPrevDevAddr)], #0;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sNextDevAddr)], #0;
	idf		drc0, st;
	wdf		drc0;

	/*
		If there is something queued then add the current render context back to the
		tail of the transfer render context list
	*/
	!p1	ba		TRF_NoTransferKicks;
	{
		MK_LOAD_STATE_INDEXED(r4, sTransferContextTail, r1, drc0, r3);
		MK_WAIT_FOR_STATE_LOAD(drc0);

		!p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sNextDevAddr)], r0;
		!p0	ba	TRF_TK_NoTCHeadUpdate;
		{
			MK_STORE_STATE_INDEXED(sTransferContextHead, r1, r0, r2);
		}
		TRF_TK_NoTCHeadUpdate:
		MK_STORE_STATE_INDEXED(sTransferContextTail, r1, r0, r3);
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sPrevDevAddr)], r4;
		idf		drc0, st;
		wdf		drc0;
	}
	TRF_NoTransferKicks:

	/* Check if the TA is waiting to start a partial render. */
	MK_LOAD_STATE(r0, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	shl.testns		p0, r0, #(31 - TA_IFLAGS_WAITINGTOSTARTCSRENDER_BIT);
	p0	ba				TRF_NoSPMRenderWaiting;
	{
		MK_TRACE(MKTC_SPM_RESUME_ABORTCOMPLETE, MKTF_TQ | MKTF_SCH);
		/*
			Start the partial render now.
			#####################################################
			NOTE: Returns directly to our caller.
			#####################################################
		*/
		/* Emit the loopback */
		TATERMINATE();
	}
	TRF_NoSPMRenderWaiting:

	FINDTA();
#if defined(SGX_FEATURE_2D_HARDWARE)
	FIND2D();
#endif

	bal Find3D;

	MK_TRACE(MKTC_TRANSFERRENDERFINISHED_END, MKTF_TQ);

	/* branch back to calling code */
	mov		pclink, R_EDM_PCLink;
	lapc;
}

/*****************************************************************************
 AdvanceTransferCCBROff routine - calculates the new ROff for the Transfer CCB and
 									decrements the kick count

 inputs:	r0 - render context of Transfer CCB to be advanced (maintained)
			r1 - CCB command of Transfer kick just completed (maintained)
 outputs:	r2 - Count of commands outstanding
 temps:		r3, r4
*****************************************************************************/
AdvanceTransferCCBROff:
{
	/* Calc. new read offset into the CCB */
	MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sCCBCtlDevAddr)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r3, [r2, #DOFFSET(PVRSRV_SGX_CCB_CTL.ui32ReadOffset)], drc0;
	MK_MEM_ACCESS_CACHED(ldad)	r4, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.ui32Size)], drc1;
	wdf		drc0;
	wdf		drc1;
	iaddu32	r3, r3.low, r4;
	and		r3, r3, #(SGX_CCB_SIZE - 1);
	MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_CCB_CTL.ui32ReadOffset)], r3;

	/* Decrement kick count for current context */
	MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.ui32Count)], drc0;
	wdf		drc0;
	iadd32		r2, r2.low, #-1;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.ui32Count)], r2;
	idf		drc0, st;
	wdf		drc0;

	/* return */
	lapc;
}

/*****************************************************************************
 UpdateTransferSurfaceOps routine - updates any surface ops associated with
 									a transfer command.

 inputs:	r1 - CCB command of Transfer kick just completed (maintained)
 temps:		r2, r3, r4, r5, r6, r7, r8, r9
*****************************************************************************/
UpdateTransferSurfaceOps:
{
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.ui32Flags)], drc1;
	/* This may have unblocked another operation, so reset APM */
	MK_STORE_STATE(ui32ActivePowerCounter, #0);
	MK_STORE_STATE(ui32ActivePowerFlags, #SGX_UKERNEL_APFLAGS_ALL);
	MK_WAIT_FOR_STATE_STORE(drc0);
	wdf		drc1;

	/* Don't update if we were asked not to */
	and.testnz	p0, r2, #SGXMKIF_TQFLAGS_NOSYNCUPDATE;
	p0	ba UTSO_SkipOpUpdates;
	{
		/* Update the syncops if there are any */
		MK_MEM_ACCESS(ldad)	r7, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.sShared.ui32NumSrcSyncs)], drc0;
		wdf	drc0;
		and.testz	p1, r7, r7;
		p1	ba	UTSO_NoMoreSrcSyncs;
		mov	r8, #OFFSET(SGXMKIF_TRANSFERCMD.sShared.asSrcSyncs[0]);
		iaddu32	r8, r8.low, r1;
		UTSO_NextSrcSync:
		{
	
			MK_MEM_ACCESS_BPCACHE(ldad.f2)	r2, [r8, #0++], drc0;
			wdf		drc0;

			and.testz	p0, r3, r3;
			p0	ba UTSO_NoSrcOpComplete;
			{
				MK_TRACE(MKTC_UTSO_UPDATEREADOPS, MKTF_TQ | MKTF_SO);
	
				/* just increment the read ops */
				mov	r6, #1;
				iaddu32 r2, r6.low, r2;
				MK_MEM_ACCESS_BPCACHE(stad) [r3], r2;
	
				MK_TRACE_REG(r3, MKTF_TQ | MKTF_SO);
				MK_TRACE_REG(r2, MKTF_TQ | MKTF_SO);
			}
			UTSO_NoSrcOpComplete:
			isub16.testnz	r7, p0, r7, #1;
			p0	mov	r9, #SIZEOF(PVRSRV_DEVICE_SYNC_OBJECT) - 8; /* Take count of the ++ in the ldad */
			p0	iaddu32	r8, r9.low, r8;
			p0	ba	UTSO_NextSrcSync;
		}
		UTSO_NoMoreSrcSyncs:

		MK_MEM_ACCESS(ldad)	r7, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.sShared.ui32NumDstSyncs)], drc0;
		wdf	drc0;
		and.testz	p1, r7, r7;
		p1	ba	UTSO_NoMoreDstSyncs;
		mov	r8,	#OFFSET(SGXMKIF_TRANSFERCMD.sShared.asDstSyncs[0]);
		iaddu32	r8, r8.low, r1;
		UTSO_NextDstSync:
		{
			MK_MEM_ACCESS_BPCACHE(ldad.f2)	r4, [r8, #DOFFSET(PVRSRV_DEVICE_SYNC_OBJECT.ui32WriteOpsPendingVal)-1], drc1;
			wdf		drc1;
			and.testz		p0, r5, r5;
			p0	ba UTSO_NoDstOpComplete;
			{
				MK_TRACE(MKTC_UTSO_UPDATEWRITEOPS, MKTF_TQ | MKTF_SO);
	
				/* write back new writeopcomplete value for dst */
				mov	r6, #1;
				iaddu32 r4, r6.low, r4;
				MK_MEM_ACCESS_BPCACHE(stad)	[r5], r4;
	
				MK_TRACE_REG(r5, MKTF_TQ | MKTF_SO);
				MK_TRACE_REG(r4, MKTF_TQ | MKTF_SO);
			}
			UTSO_NoDstOpComplete:
			isub16.testnz	r7, p0, r7, #1;
			p0	mov r9,	#SIZEOF(PVRSRV_DEVICE_SYNC_OBJECT);
			p0	iaddu32	r8, r9.low, r8;
			p0	ba	UTSO_NextDstSync;
		}
		UTSO_NoMoreDstSyncs:
	}
	UTSO_SkipOpUpdates:

	MK_LOAD_STATE(r2, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	shl.testns	p0, r2, #(31 - TA_IFLAGS_OOM_PR_BLOCKED_BIT);
	p0	ba	UTSO_NoPRNotBlocked;
	{
		/* clear the flag */
		and	r2, r2, ~#TA_IFLAGS_OOM_PR_BLOCKED;
		MK_STORE_STATE(ui32ITAFlags, r2);

		/* restart the TA */
		mov		r4, pclink;
		bal 	TARestartWithoutRender;
		mov		pclink, r4;
		/* wait for the stores */
		idf		drc0, st;
		wdf		drc0;
	}
	UTSO_NoPRNotBlocked:

	/* Send interrupt to host */
	mov	 	r3, #EUR_CR_EVENT_STATUS_SW_EVENT_MASK;
	str		#EUR_CR_EVENT_STATUS >> 2, r3;

	/* return */
	lapc;
}

/*****************************************************************************
 DummyProcessTransfer routine - Dummy process a transfer command as it may have
 								locked up previously

 inputs:	r0 - context of render about to be processed
			r1 - CMD of the transfer about to be processed
 temps:		r2, r3, r4
*****************************************************************************/
DummyProcessTransfer:
{
	MK_TRACE(MKTC_DUMMYPROCTRANSFER, MKTF_TQ | MKTF_SCH);

	/* Reset the priority scheduling level block, as it is no longer blocked */
	MK_STORE_STATE(ui32BlockedPriority, #(SGX_MAX_CONTEXT_PRIORITY - 1));

	WRITEHWPERFCB(3D, MK_TRANSFER_DUMMY_START, GRAPHICS, #0, p0, ktranfser_dummy_1, r2, r3, r4, r5, r6, r7, r8, r9);
	/* Do the minimum required, first do the jobs associated with KickTransferRender */
	/* Ensure the memory context is setup */
	SETUPDIRLISTBASE(DPTR, r0, #SETUP_BIF_TQ_REQ);

	/* Carry out any updates required */
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.ui32NumUpdates)], drc0;
	wdf		drc0;
	and.testz	p0, r2, r2;
	p0	ba		DPTR_NoUpdates;
	{
		SWITCHEDMTO3D();
		mov			r3, #OFFSET(SGXMKIF_TRANSFERCMD.sUpdates);
		iadd32		r3, r3.low, r1;	// start
		mov			r4, #(2 * SIZEOF(IMG_UINT32));
		imae		r2, r2.low, r4.low, r3, u32; // finish

		MK_MEM_ACCESS_CACHED(ldad.f2)	r4, [r3, #0++], drc0;
		DPTR_NextUpdate:
		{
			xor.testnz	p0, r2, r3;
			wdf		drc0;
			MK_MEM_ACCESS_BPCACHE(stad)	[r4], r5;
			p0	MK_MEM_ACCESS_CACHED(ldad.f2)	r4, [r3, #0++], drc0;
			p0	ba		DPTR_NextUpdate;
		}
		/* make sure all the updates are in memory before starting the transfer */
		idf		drc0, st;
		wdf		drc0;
		SWITCHEDMBACK();
	}
	DPTR_NoUpdates:

	/* Now let's do the jobs associated with TransferRenderFinished */

	MK_MEM_ACCESS_CACHED(ldad.f2)	r2, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.sShared.ui32TASyncWriteOpsPendingVal)-1], drc0;
	wdf		drc0;
	and.testz	p0, r3, r3;
	p0	ba	DPTR_NoTASyncWriteOp;
	{
		mov	r4, #1;
		iaddu32 r2, r4.low, r2;
		MK_MEM_ACCESS_BPCACHE(stad)	[r3], r2;
		idf	drc0, st;
	}
	DPTR_NoTASyncWriteOp:

	MK_MEM_ACCESS_CACHED(ldad.f2)	r2, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.sShared.ui323DSyncWriteOpsPendingVal)-1], drc0;
	wdf		drc0;
	and.testz	p0, r3, r3;
	p0	ba	DPTR_No3DSyncWriteOp;
	{
		mov	r4, #1;
		iaddu32 r2, r4.low, r2;
		MK_MEM_ACCESS_BPCACHE(stad)	[r3], r2;
		idf	drc0, st;
	}
	DPTR_No3DSyncWriteOp:

	/* Write out any StatusValues required */
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_TRANSFERCMD.sShared.ui32NumStatusVals)], drc0;
	wdf		drc0;
	and.testz	p0, r2, r2;
	p0	ba		DPTR_NoStatusVals;
	{
		mov		r3, #OFFSET(SGXMKIF_TRANSFERCMD.sShared.sCtlStatusInfo);
		iadd32		r3, r3.low, r1;	// start
		imae		r4, r2.low, #SIZEOF(CTL_STATUS), r3, u32; // finish

		MK_MEM_ACCESS_CACHED(ldad.f2)	r5, [r3, #0++], drc0;
		DPTR_NextStatusVal:
		{
			xor.testnz	p0, r3, r4;
			wdf		drc0;
			MK_MEM_ACCESS_BPCACHE(stad)	[r5], r6;
			p0	MK_MEM_ACCESS_CACHED(ldad.f2)	r5, [r3, #0++], drc0;
			p0	ba		DPTR_NextStatusVal;
		}
	}
	DPTR_NoStatusVals:

	/* Update the surface sync ops */
	mov		r12, pclink;
	bal		UpdateTransferSurfaceOps;
	/* move on the Transfer CCB */
	bal		AdvanceTransferCCBROff;
	mov		pclink, r12;
	/* test the count returned */
	and.testnz	p1, r2, r2;

	/* Get the context priority */
	MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sCommon.ui32Priority)], drc0;
	wdf		drc0;

	/* Remove the current context from the transfer render contexts list */
	MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sPrevDevAddr)], drc0;
	MK_MEM_ACCESS(ldad)	r5, [r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sNextDevAddr)], drc1;
	wdf		drc0;
	and.testz	p0, r4, r4;	// is prev null?
	wdf		drc1;
	/* (prev == NULL) ? head = current->next : prev->nextptr = current->next */
	!p0	ba	DPTR_NoTCHeadUpdate;
	{
		MK_STORE_STATE_INDEXED(sTransferContextHead, r1, r5, r2);
	}
	DPTR_NoTCHeadUpdate:
	!p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sNextDevAddr)], r5;

	and.testz	p0, r5, r5;	// is next null?
	/* (next == NULL) ? tail == current->prev : next->prevptr = current->prev */
	!p0 ba	DPTR_NoTCTailUpdate;
	{
		MK_STORE_STATE_INDEXED(sTransferContextTail, r1, r4, r3);
	}
	DPTR_NoTCTailUpdate:
	!p0	MK_MEM_ACCESS(stad)	[r5, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sPrevDevAddr)], r4;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sPrevDevAddr)], #0;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sNextDevAddr)], #0;
	idf		drc1, st;

	/* make sure all previous writes have completed */
	wdf		drc1;
	/* If there is something queued then add the current render context back to the tail of the transfer render context list */
	!p1	ba		DPTR_NoKicksOutstanding;
	{
		MK_LOAD_STATE_INDEXED(r4, sTransferContextTail, r1, drc0, r3);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and.testz	p0, r4, r4;
		!p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sNextDevAddr)], r0;
		!p0	ba	DPTR_KO_NoHeadUpdate;
		{
			MK_STORE_STATE_INDEXED(sTransferContextHead, r1, r0, r2);
		}
		DPTR_KO_NoHeadUpdate:
		MK_STORE_STATE_INDEXED(sTransferContextTail, r1, r0, r3);
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sPrevDevAddr)], r4;
	}
	DPTR_NoKicksOutstanding:
	idf		drc0, st;
	wdf		drc0;

	WRITEHWPERFCB(3D, MK_TRANSFER_DUMMY_END, GRAPHICS, #0, p0, ktranfser_dummy_2, r2, r3, r4, r5, r6, r7, r8, r9);
	/* now check to see if there are any other Transfers we can do instead
		not bal so that is returns as if initial call */
	ba	Find3D;
}

TARestartWithoutRender:
{
	/* Restart the TA. */
#if defined(SGX_FEATURE_MP)
	ldr		r2, #EUR_CR_DPM_ABORT_STATUS_MTILE >> 2, drc0;
	wdf		drc0;
	/* and with its mask */
	and		r2, r2, #EUR_CR_MASTER_DPM_ABORT_STATUS_MTILE_CORE_MASK;
	/* shift to base address position */
	shr		r2, r2, #EUR_CR_MASTER_DPM_ABORT_STATUS_MTILE_CORE_SHIFT;
	/* issue abort to correct core */
	WRITECORECONFIG(r2, #EUR_CR_DPM_OUTOFMEM >> 2, #EUR_CR_DPM_OUTOFMEM_RESTART_MASK, r3);
	/* Also issue abort to master */
#endif /* if defined (SGX_FEATURE_MP) */
	str		#EUR_CR_DPM_OUTOFMEM >> 2, #EUR_CR_DPM_OUTOFMEM_RESTART_MASK;
	MK_TRACE(MKTC_RESTARTTANORENDER, MKTF_SCH | MKTF_HW);

	MK_LOAD_STATE(r5, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and		r5, r5, ~#RENDER_IFLAGS_PARTIAL_RENDER_OUTOFORDER;
	MK_STORE_STATE(ui32IRenderFlags, r5);
	MK_WAIT_FOR_STATE_STORE(drc0);

	/* Return the render finished code. */
	lapc;
}
