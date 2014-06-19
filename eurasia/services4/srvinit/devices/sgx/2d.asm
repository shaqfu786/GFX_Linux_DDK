/*****************************************************************************
 Name		: 2d.asm

 Title		: USE program for handling 2D related events

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

 Description 	: USE program for handling all 2D events

 Program Type	: USE assembly language

 Version 	: $Revision: 1.78 $

 Modifications	:

 $Log: 2d.asm $
 
 
 BRN29022 - Add extra HWPERF data for HW recovery events
  --- Revision Logs Removed --- 
 *****************************************************************************/

#include "usedefs.h"

/***************************
 Sub-routine Imports
***************************/
.import CommonProgramEnd;
.import EmitLoopBack;
.import MKTrace;

.import SetupDirListBase;
#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
.import SetupRequestor;
#else
.import StartTAContextSwitch;
.import	Start3DContextSwitch;
#endif
.import ClearActivePowerFlags;

.import	CheckTAQueue;
.import Check3DQueues;

.import WriteHWPERFEntry;

#if defined(FIX_HW_BRN_25615)
.import IdleCore;
.import Resume;
#endif
#if defined(SGX_FEATURE_MK_SUPERVISOR)
.import JumpToSupervisor;
#endif

#if !defined(SGX_FEATURE_2D_HARDWARE)
	nop;
#else
/***************************
 Sub-routine Exports
***************************/
.export TwoDEventHandler;
.export TwoDHandler;

/*****************************************************************************
 TwoDEventHandler routine - decides which events have occurred and calls the
 							relevent subroutines.

 inputs:
 temps:
*****************************************************************************/
.modulealign 2;
TwoDEventHandler:
{
	UKERNEL_PROGRAM_INIT_COMMON();

	WRITEHWPERFCB(2D, MK_2D_START, MK_EXECUTION, #0, p0, mk_2d_event_start, r2, r3, r4, r5, r6, r7, r8, r9);

	/* Test for any of the TA events occuring */
	/* Test the TA Finished event */
#if defined(SGX_FEATURE_PTLA)
	and		r0, PA(ui32PDSIn0), #EURASIA_PDS_IR0_EDM_EVENT_PTLACOMPLETE;
#else
	and		r0, PA(ui32PDSIn0), #EURASIA_PDS_IR0_EDM_EVENT_TWODCOMPLETE;
#endif	
	and.testnz	p0, r0, r0;
	!p0	br	TDEH_No2DComplete;
	{
#if defined(FIX_HW_BRN_31272) || defined(FIX_HW_BRN_31780) || defined(FIX_HW_BRN_33920)
		/* check whether the request status has changed since it was last sampled */
		MK_LOAD_STATE(r3, ui32I2DFlags, drc0);
		wdf		drc0;
		and.testnz	p0, r3, #TWOD_IFLAGS_DUMMYFILL;
		p0 ba TDEH_AlreadyInDummy;
		{
			ldr		r0, #EUR_CR_MASTER_PTLA_REQUEST >> 2, drc0;
			wdf		drc0;
			and.testz	p0, r0, r0;
			p0		ba TDEH_No2DComplete;
		}
		TDEH_AlreadyInDummy:

		/* wait (at least) 200 clks for the BUSY to go low */
		mov r2, #30;
		TDEH_WaitForIdle:
		{
			ldr		r1, #EUR_CR_MASTER_PTLA_STATUS >> 2, drc0;
			wdf		drc0;
			and.testz	p0, r1, #EUR_CR_MASTER_PTLA_STATUS_BUSY_MASK;
			p0		ba	TDEH_Idle;
			isub16.testz r2, p0, r2, #1;
			p0		ba TDEH_No2DComplete;
			ba TDEH_WaitForIdle;
		}
		TDEH_Idle:

		/* The request status has changed and PTLA is not busy, so it must be a genuine complete */
		MK_MEM_ACCESS_CACHED(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sPTLAWriteBackDevVAddr)], drc1;

		MK_LOAD_STATE(r0, ui32DummyFillCnt, drc0);
		MK_WAIT_FOR_STATE_STORE(drc0);
		wdf	drc0;

		and.testnz	p0, r3, #TWOD_IFLAGS_DUMMYFILL;
		p0 ba TDEH_NoDummyFill;
		{
			or r3, r3, #TWOD_IFLAGS_DUMMYFILL; /* RMW */
			MK_STORE_STATE(ui32I2DFlags, r3);

			iaddu32	r0, r0.low, #1;
			MK_STORE_STATE(ui32DummyFillCnt, r0);

			/* wait for the WBACK addr */
			wdf			drc1;

			mov			r1, #0x50060004;
#if defined(FIX_HW_BRN_29557)
			SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_PTLA_SLAVE_CMD_0 >> 2, r1);
			SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_PTLA_SLAVE_CMD_0 >> 2, r2);
			mov			r1, #0x60000010;
			SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_PTLA_SLAVE_CMD_0 >> 2, r1);
			SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_PTLA_SLAVE_CMD_0 >> 2, r0);
			mov			r1, #0x00000000;
			SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_PTLA_SLAVE_CMD_0 >> 2, r1);
			mov			r1, #0x00002001;
			SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_PTLA_SLAVE_CMD_0 >> 2, r1);
			mov			r1, #0x80000000;     // PTLA_FLUSH
			SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_PTLA_SLAVE_CMD_0 >> 2, r1);
#else
			str			#EUR_CR_MASTER_PTLA_SLAVE_CMD_0 >> 2, r1;
			str			#EUR_CR_MASTER_PTLA_SLAVE_CMD_0 >> 2, r2;
			mov			r1, #0x60000010;
			str			#EUR_CR_MASTER_PTLA_SLAVE_CMD_0 >> 2, r1;
			str			#EUR_CR_MASTER_PTLA_SLAVE_CMD_0 >> 2, r0;
			mov			r1, #0x00000000;
			str			#EUR_CR_MASTER_PTLA_SLAVE_CMD_0 >> 2, r1;
			mov			r1, #0x00002001;
			str			#EUR_CR_MASTER_PTLA_SLAVE_CMD_0 >> 2, r1;
			mov			r1, #0x80000000;     // PTLA_FLUSH
			str			#EUR_CR_MASTER_PTLA_SLAVE_CMD_0 >> 2, r1;
#endif
			ba TDEH_No2DComplete;
		}
		TDEH_NoDummyFill:

		/* wait for the WBACK addr */
		wdf	drc1;
		mov	r3, #30;
		TDEH_WaitForWBack:
		{
			MK_MEM_ACCESS_BPCACHE(ldad)	r1, [r2], drc0;
			wdf		drc0;
			xor.testz	p0, r0, r1;
			p0		ba TDEH_WBack;
			isub16.testz 	r3, p0, r3, #1;
			p0		ba TDEH_No2DComplete;
			ba TDEH_WaitForWBack;
		}
		TDEH_WBack:

		MK_LOAD_STATE(r1, ui32DummyFillCnt, drc0);
		wdf	drc0;
		/* drop the TWOD_IFLAGS_DUMMYFILL - we are all done */
		and r1, r1, #~TWOD_IFLAGS_DUMMYFILL;
		MK_STORE_STATE(ui32I2DFlags, r1);
#endif /* FIX_HW_BRN_31272 || FIX_HW_BRN_31780 || FIX_HW_BRN_33920 */

		MK_TRACE(MKTC_2DEVENT_2DCOMPLETE, MKTF_EV | MKTF_2D);
		WRITEHWPERFCB(2D, 2D_END, GRAPHICS, #0, p0, k2d_event_end_1, r2, r3, r4, r5, r6, r7, r8, r9);

#if defined(SGX_FEATURE_PTLA)
		mov		r0, #EUR_CR_MASTER_EVENT_HOST_CLEAR_TWOD_COMPLETE_MASK;
		str		#EUR_CR_MASTER_EVENT_HOST_CLEAR >> 2, r0;
#else		
		mov		r0, #EUR_CR_EVENT_HOST_CLEAR_TWOD_COMPLETE_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r0;
#endif

		mov		R_EDM_PCLink, #LADDR(TDEH_No2DComplete);
		ba		TwoDComplete;
	}
	TDEH_No2DComplete:

	WRITEHWPERFCB(2D, MK_2D_END, MK_EXECUTION, #0, p0, mk_2d_event_end, r2, r3, r4, r5, r6, r7, r8, r9);

	/* Done */
	UKERNEL_PROGRAM_END(#0, MKTC_2DEVENT_END, MKTF_EV | MKTF_2D);
}

/*****************************************************************************
 TwoDHandler routine	- decides which events have occurred and calls the
 							relevent subroutines.

 inputs:
 temps:
*****************************************************************************/
.align 2;
TwoDHandler:
{
	UKERNEL_PROGRAM_INIT_COMMON();

	CLEARPENDINGLOOPBACKS(r0, r1);

	WRITEHWPERFCB(2D, MK_2D_START, MK_EXECUTION, #0, p0, mk_2d_start, r2, r3, r4, r5, r6, r7, r8, r9);

	/* Test the 2D Complete event */
	and		r0, PA(ui32PDSIn0), #USE_LOOPBACK_2DCOMPLETE;
	and.testnz	p0, r0, r0;
	!p0	ba		TDH_No2DComplete;
	{
		MK_TRACE(MKTC_2DLB_2DCOMPLETE, MKTF_EV | MKTF_2D);

#if defined(SGX_FEATURE_PTLA)
		mov		r0, #EUR_CR_MASTER_EVENT_HOST_CLEAR_TWOD_COMPLETE_MASK;
		str		#EUR_CR_MASTER_EVENT_HOST_CLEAR >> 2, r0;
#else		
		mov		r0, #EUR_CR_EVENT_HOST_CLEAR_TWOD_COMPLETE_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r0;
#endif		

		mov		R_EDM_PCLink, #LADDR(TDH_No2DComplete);
		ba		TwoDComplete;
	}
	TDH_No2DComplete:

	/* Test the Find2D call */
	and		r0, PA(ui32PDSIn0), #USE_LOOPBACK_FIND2D;
	and.testnz	p0, r0, r0;
	!p0	ba		TDH_NoFind2D;
	{
		MK_TRACE(MKTC_2DLB_FIND2D, MKTF_EV | MKTF_2D);

		bal		Find2D;
	}
	TDH_NoFind2D:

	WRITEHWPERFCB(2D, MK_2D_END, MK_EXECUTION, #0, p0, mk_2d_end, r2, r3, r4, r5, r6, r7, r8, r9);

	/* End the program */
	UKERNEL_PROGRAM_END(#0, MKTC_2DLB_END, MKTF_EV | MKTF_2D);
}

/*****************************************************************************
 TwoDComplete routine	- process the 2D complete event

 inputs:
 temps:
*****************************************************************************/
TwoDComplete:
{
	MK_TRACE(MKTC_2DCOMPLETE_START, MKTF_2D);

	/*Work around for the issue BRN32916*/
	/* When 2D and 3D engines run simultaneously 
	      on some cores they contend for the memory/bus and 
	      this may delay 3D events that are suppose to happen.
	      This delay is assumed as a lockup and hardware recovery gets kicked in.
	      To avoid this we invalidate the 3D signature registers */
	/* Load the Render Flags */
	MK_LOAD_STATE(r0, ui32IRenderFlags, drc0);
	mov 	r3, #RENDER_IFLAGS_RENDERINPROGRESS;
	MK_WAIT_FOR_STATE_LOAD(drc0);
	
	/* Clear the render flags */
	and.testz		p0, r0, r3;
	p0	ba NO_3DSignatureInvalidation;
	{
	#if defined(SUPPORT_HW_RECOVERY)
		/* reset the Recovery counter */
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRTimerTicksSince3D)], #0;
	
		/* invalidate the signatures */
		MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], drc0;
		wdf		drc0;
		or		r0, r0, #0x2;
		MK_MEM_ACCESS_BPCACHE(stad) 	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], r0;
		idf		drc0, st;
		wdf		drc0;
	#endif

	}

	NO_3DSignatureInvalidation:

	MK_LOAD_STATE(r0, ui32I2DFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	/* Clear the render flags */
	and		r0, r0, ~#TWOD_IFLAGS_2DBLITINPROGRESS;
	MK_STORE_STATE(ui32I2DFlags, r0)
	MK_WAIT_FOR_STATE_STORE(drc0);

#if defined (FIX_HW_BRN_32303) && ! defined(FIX_HW_BRN_32845)
	/* switch master BIF clock gate to auto */
	ldr		r3, #EUR_CR_MASTER_CLKGATECTL2 >> 2, drc0;
	wdf		drc0;

	and		r3, r3, #(~EUR_CR_MASTER_CLKGATECTL2_BIF_CLKG_MASK);
	or 		r3, r3, #(EUR_CR_CLKGATECTL_AUTO << EUR_CR_MASTER_CLKGATECTL2_BIF_CLKG_SHIFT);
	str		#EUR_CR_MASTER_CLKGATECTL2 >> 2, r3;
#endif

#if defined(FIX_HW_BRN_34811)
	/* switch PTLA clk gate to auto */
	ldr		r3, #EUR_CR_MASTER_CLKGATECTL2 >> 2, drc0;
	wdf		drc0;

	and		r3, r3, #(~EUR_CR_MASTER_CLKGATECTL2_PTLA_CLKG_MASK);
	or 		r3, r3, #(EUR_CR_CLKGATECTL_AUTO << EUR_CR_MASTER_CLKGATECTL2_PTLA_CLKG_SHIFT);
	str		#EUR_CR_MASTER_CLKGATECTL2 >> 2, r3;
#endif

	/* write out status here */
	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s2DCCBCmd)], drc0;
	wdf		drc0;

	/* Update the surface sync ops */
	bal		Update2DSurfaceOps;

	/* Send interrupt to host */
	mov	 			r0, #EUR_CR_EVENT_STATUS_SW_EVENT_MASK;
	str	 			#EUR_CR_EVENT_STATUS >> 2, r0;

	/* Load the HW2DContext */
	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s2DContext)], drc0;
	wdf		drc0;

	/* move on the 2DCCB */
	bal		Advance2DCCBROff;

	/* test the count */
	and.testnz	p1, r2, r2;

	/* Get the priority of the context */
	MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sCommon.ui32Priority)], drc0;
	wdf		drc0;

	/* Remove the current 2D context from the head of the 2D contexts list */
	MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sNextDevAddr)], drc0;
	wdf		drc0;
	and.testz	p0, r4, r4;
	MK_STORE_STATE_INDEXED(s2DContextHead, r1, r4, r2);
	!p0	ba	TDC_ListNotEmpty;
	{
		MK_STORE_STATE_INDEXED(s2DContextTail, r1, #0, r3);
	}
	TDC_ListNotEmpty:
	!p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HW2DCONTEXT.sPrevDevAddr)], #0;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sNextDevAddr)], #0;
	idf		drc0, st;
	wdf		drc0;

	/*
		If there is something queued then add the current 2D context back to the
		tail of the 2D context list
	*/
	!p1	ba		TDC_No2DKicks;
	{
		MK_LOAD_STATE_INDEXED(r4, s2DContextTail, r1, drc0, r3);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		!p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HW2DCONTEXT.sNextDevAddr)], r0;
		!p0	ba	TDC_RLNE_NoHeadUpdate;
		{
			MK_STORE_STATE_INDEXED(s2DContextHead, r1, r0, r2);
		}
		TDC_RLNE_NoHeadUpdate:
		MK_STORE_STATE_INDEXED(s2DContextTail, r1, r0, r3);
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sPrevDevAddr)], r4;
		idf		drc0, st;
		wdf		drc0;
	}
	TDC_No2DKicks:

	FINDTA();

	FIND3D();
	/* check if there are any more 2D operations to be completed */
	bal Find2D;

	MK_TRACE(MKTC_2DCOMPLETE_END, MKTF_2D);

	/* branch back to calling code */
	mov		pclink, R_EDM_PCLink;
	lapc;
}

/*****************************************************************************
 Find2D routine	- check to see if there is a 2D task ready to be
					executed

 inputs:
 temps:		r0
			r1
			r2
			r3
			r4
*****************************************************************************/
Find2D:
{
	/* Make sure it's OK to start a new render now */
	MK_LOAD_STATE(r1, ui32HostRequest, drc0);
	MK_LOAD_STATE(r0, ui32I2DFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testnz	p1, r0, #TWOD_IFLAGS_2DBLITINPROGRESS;

	p1	ba		FTD_End;
	{
		/* If we are trying to turn off the chip then don't try to kick anything */
		and.testz	p2, r1, #SGX_UKERNEL_HOSTREQUEST_POWER;
		p2 ba 		FTD_NoPowerOff;
		{
			MK_TRACE(MKTC_FIND2D_POWERREQUEST, MKTF_2D | MKTF_SCH | MKTF_POW);
			ba			FTD_End;
		}
		FTD_NoPowerOff:
		{
#if defined(FIX_HW_BRN_29900) || defined(USE_SUPPORT_NO_TA2D_OVERLAP)
			/* Don't allow overlapping */
			MK_LOAD_STATE(r1, ui32ITAFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			and.testz		p0, r1, #TA_IFLAGS_TAINPROGRESS;
			p0	ba	FTD_NoTA2DOverlap;
			{
				MK_TRACE(MKTC_FTD_TA2D_OVERLAP_BLOCKED, MKTF_2D | MKTF_SCH);
				ba	FTD_End;
			}
			FTD_NoTA2DOverlap:
#endif
			/* Look for a 2D command */
			/* Start at the highest priority */
			mov		r10, #0;
			MK_LOAD_STATE(r0, s2DContextHead[0], drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			FTD_CheckNextContext:
			and.testnz	p0, r0, r0;
			p0	ba		FTD_NextContext;
			{
				/* Make sure we only check priorities >= blocked priority, if nothing is blocked
					ui32BlockedPriority is equal to the lowest level */
				iaddu32	r10, r10.low, #1;
				MK_LOAD_STATE(r0, ui32BlockedPriority, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);
				isub16.tests	p0, r0, r10;
				p0	ba	FTD_AllPrioritiesChecked;
				{
					MK_LOAD_STATE_INDEXED(r0, s2DContextHead, r10, drc0, r1);
					MK_WAIT_FOR_STATE_LOAD(drc0);
					ba	FTD_CheckNextContext;
				}
				FTD_AllPrioritiesChecked:

				/*
					Indicate to the Active Power Manager that no 2D blits are ready to go.
				*/
				CLEARACTIVEPOWERFLAGS(#SGX_UKERNEL_APFLAGS_2D);
				ba		FTD_End;
			}
			FTD_NextContext:
			{
				/* Calculate base of 2D CCB command */
				MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sCommon.ui32Flags)], drc0;
				MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sCCBCtlDevAddr)], drc0;
				wdf		drc0;
				
				and.testnz p0, r2, #SGXMKIF_HWCFLAGS_SUSPEND;
				!p0 ba FTD_ContextNotSuspended;
				{
					MK_TRACE(MKTC_FTD_CONTEXT_SUSPENDED, MKTF_2D | MKTF_SCH);
					MK_TRACE_REG(r0, MKTF_2D);
					ba		FTD_2DNotReady;
				}
				FTD_ContextNotSuspended:
				
				MK_MEM_ACCESS_BPCACHE(ldad)	r1, [r1, #DOFFSET(PVRSRV_SGX_CCB_CTL.ui32ReadOffset)], drc0;
				MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sCCBBaseDevAddr)], drc1;
				wdf		drc0;
				wdf		drc1;
				iaddu32	r1, r1.low, r2;

				MK_MEM_ACCESS_CACHED(ldad.f4)	r2, [r1, #DOFFSET(SGXMKIF_2DCMD.sShared.sTASyncData.ui32ReadOpsPendingVal)-1], drc0;
				wdf		drc0;
				and.testz	p0, r3, r3;
				p0	ba	FTD_TASyncOpReady;
				{
					/* Check this is ready to go */
					MK_MEM_ACCESS_BPCACHE(ldad)	r6, [r3], drc0; // load ReadOpsComplete
					MK_MEM_ACCESS_BPCACHE(ldad)	r7, [r5], drc0; // load WriteOpsComplete
					wdf		drc0;
					xor.testz	p0, r6, r2;	// check ReadOpsComplete
					p0 xor.testz	p0, r7, r4; // check WriteOpsComplete
					p0	ba	FTD_TASyncOpReady;
					{
						MK_TRACE(MKTC_FTD_TAOPSBLOCKED, MKTF_2D | MKTF_SCH | MKTF_SO);
						ba	FTD_2DNotReady;
					}
				}
				FTD_TASyncOpReady:

				MK_MEM_ACCESS_CACHED(ldad.f4)	r2, [r1, #DOFFSET(SGXMKIF_2DCMD.sShared.s3DSyncData.ui32ReadOpsPendingVal)-1], drc0;
				wdf		drc0;
				and.testz	p0, r3, r3;
				p0	ba	FTD_3DSyncOpReady;
				{
					/* Check this is ready to go */
					MK_MEM_ACCESS_BPCACHE(ldad)	r6, [r3], drc0; // load ReadOpsComplete
					MK_MEM_ACCESS_BPCACHE(ldad)	r7, [r5], drc0; // load WriteOpsComplete
					wdf		drc0;
					xor.testz	p0, r6, r2;	// check ReadOpsComplete
					p0 xor.testz	p0, r7, r4; // check WriteOpsComplete
					p0	ba	FTD_3DSyncOpReady;
					{
						MK_TRACE(MKTC_FTD_3DOPSBLOCKED, MKTF_2D | MKTF_SCH | MKTF_SO);
						ba	FTD_2DNotReady;
					}
				}
				FTD_3DSyncOpReady:

				/* Check syncops values to see if they are at the required values
					SrcReads
					SrcWrites
					DstReads
					DstWrites
				*/
				MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_2DCMD.sShared.ui32NumSrcSync)], drc0;
				wdf		drc0;
				and.testz	p0, r2, r2;
				p0	ba	FTD_AllSrcSyncReady;
				{
					mov		r3, #OFFSET(SGXMKIF_2DCMD.sShared.sSrcSyncData[0]);
					iadd32	r3, r3.low, r1;

					MK_MEM_ACCESS_CACHED(ldad.f4)	r4, [r3, #0++], drc0;
					FTD_CheckNextSource:
					{
						isub16.testz	r2, p0, r2, #1;
						wdf		drc0;
						/* Check this is ready to go */
						MK_MEM_ACCESS_BPCACHE(ldad)	r8, [r5], drc0; // load ReadOpsComplete
						MK_MEM_ACCESS_BPCACHE(ldad)	r9, [r7], drc0; // load WriteOpsComplete
						wdf		drc0;
						xor.testz	p1, r8, r4;	// Test ReadOpsComplete
						p1	ba	FTD_SrcReadReady;
						{
							MK_TRACE(MKTC_FTD_SRCREADOPSBLOCKED, MKTF_2D | MKTF_SO | MKTF_SCH);
							MK_TRACE_REG(r5, MKTF_2D | MKTF_SO);
							MK_TRACE_REG(r8, MKTF_2D | MKTF_SO);
							MK_TRACE_REG(r4, MKTF_2D | MKTF_SO);
							ba	FTD_2DNotReady;
						}
						FTD_SrcReadReady:
						xor.testz	p1, r9, r6; // Test WriteOpsComplete
						p1	ba	FTD_SrcWriteReady;
						{
							MK_TRACE(MKTC_FTD_SRCWRITEOPSBLOCKED, MKTF_2D | MKTF_SO | MKTF_SCH);
							MK_TRACE_REG(r7, MKTF_2D | MKTF_SO);
							MK_TRACE_REG(r9, MKTF_2D | MKTF_SO);
							MK_TRACE_REG(r6, MKTF_2D | MKTF_SO);
							ba	FTD_2DNotReady;
						}
						FTD_SrcWriteReady:

						/* Move past readOps2 */
						mov r5, #8;
						mov r4, r3;
						IADD32(r3, r4, r5, r6);
						!p0	MK_MEM_ACCESS_CACHED(ldad.f4)	r4, [r3, #0++], drc0;
						!p0 ba			FTD_CheckNextSource;
					}
				}
				FTD_AllSrcSyncReady:

				MK_MEM_ACCESS_CACHED(ldad.f4)	r2, [r1, #DOFFSET(SGXMKIF_2DCMD.sShared.sDstSyncData.ui32ReadOpsPendingVal)-1], drc0;
				wdf		drc0;
				and.testz	p0, r3, r3;
				p0	ba	FTD_DestSyncOpReady;
				{
					/* Check this is ready to go */
					MK_MEM_ACCESS_BPCACHE(ldad)	r6, [r3], drc0; // load ReadOpsComplete
					MK_MEM_ACCESS_BPCACHE(ldad)	r7, [r5], drc0; // load WriteOpsComplete
					wdf		drc0;
					xor.testz	p0, r6, r2;	// Test ReadOpsComplete
					p0	ba	FTD_DestReadReady;
					{
						MK_TRACE(MKTC_FTD_DSTREADOPSBLOCKED, MKTF_2D | MKTF_SO | MKTF_SCH);
						MK_TRACE_REG(r3, MKTF_2D | MKTF_SO);
						MK_TRACE_REG(r6, MKTF_2D | MKTF_SO);
						MK_TRACE_REG(r2, MKTF_2D | MKTF_SO);
						ba	FTD_2DNotReady;
					}
					FTD_DestReadReady:
					xor.testz	p0, r7, r4; // Test WriteOpsComplete
					p0	ba	FTD_DestWriteReady;
					{
						MK_TRACE(MKTC_FTD_DSTWRITEOPSBLOCKED, MKTF_2D | MKTF_SO | MKTF_SCH);
						MK_TRACE_REG(r5, MKTF_2D | MKTF_SO);
						MK_TRACE_REG(r7, MKTF_2D | MKTF_SO);
						MK_TRACE_REG(r4, MKTF_2D | MKTF_SO);
						ba	FTD_2DNotReady;
					}
					FTD_DestWriteReady:
				}
				FTD_DestSyncOpReady:

				MK_MEM_ACCESS_CACHED(ldad.f2)	r2, [r1, #DOFFSET(SGXMKIF_2DCMD.sShared.sDstSyncData.ui32ReadOps2PendingVal)-1], drc0;
				wdf		drc0;
				and.testz	p0, r3, r3;
				p0	ba	FTD_DestRead2Ready;
				{
					/* Check this is ready to go */
					MK_MEM_ACCESS_BPCACHE(ldad)	r4, [r3], drc0; // load ReadOps2Complete
					wdf		drc0;
					xor.testz	p0, r4, r2;	// Test ReadOps2Complete
					p0	ba	FTD_DestRead2Ready;
					{
						MK_TRACE(MKTC_FTD_DSTREADOPS2BLOCKED, MKTF_2D | MKTF_SO | MKTF_SCH);
						MK_TRACE_REG(r3, MKTF_2D | MKTF_SO);
						MK_TRACE_REG(r6, MKTF_2D | MKTF_SO);
						MK_TRACE_REG(r2, MKTF_2D | MKTF_SO);
						ba	FTD_2DNotReady;
					}
				}
				FTD_DestRead2Ready:

				ba	FTD_2DReady;
				{
					FTD_2DNotReady:
					MK_MEM_ACCESS(ldad)	r0, [r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sNextDevAddr)], drc0;
					wdf		drc0;
					ba		FTD_CheckNextContext;
				}
				FTD_2DReady:
				
	#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
				/* make sure we only parallelise a single memory context */
				/* check whether there is a render/ta in progress */
				MK_LOAD_STATE(r2, ui32IRenderFlags, drc0);
				MK_LOAD_STATE(r3, ui32ITAFlags, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);
				or	r4, r2, r3;
				and.testz	p0, r4, #RENDER_IFLAGS_RENDERINPROGRESS;
				p0	ba	FTD_NoOperationsInProgress;
				{
					/* Now load up the PDPhys and DirListBase0 */
					ldr		r4, #EUR_CR_BIF_DIR_LIST_BASE0 >> 2, drc0;
					/* note: the wdf follows in SGXMK_HWCONTEXT_GETDEVPADDR */
					SGXMK_HWCONTEXT_GETDEVPADDR(r5, r0, r6, drc0);
	
					/* Are the PDPhysAddrs the same? */
					xor.testnz	p0, r4, r5;
					!p0	ba	FTD_SameAddrSpace;
					{
						MK_TRACE(MKTC_FIND2D_ADDR_SPACE_DIFFERENT, MKTF_2D | MKTF_SCH);
	
						/* Record the priority of the blocked context so nothing of lower priority 
							can be started when the current operations finish */
						MK_STORE_STATE(ui32BlockedPriority, r10);
						MK_WAIT_FOR_STATE_STORE(drc0);

						/* The memory contexts are different so only proceed if the 2D is of a higher priority */
						/* is the 3D active? */
						and.testz	p0, r2, #RENDER_IFLAGS_RENDERINPROGRESS;
						p0	ba	FTD_3DIdle;
						{
							/* is the 2D higher priority? */
							MK_LOAD_STATE(r2, ui323DCurrentPriority, drc0);
							MK_WAIT_FOR_STATE_LOAD(drc0);
							/* if the 3D is higher priority just exit, as the TA will be the same */
							isub16.testns	p0, r10, r2;
							p0 ba FTD_End;
							
							/* The 2D is higher priority so request the 3D to stop */
							START3DCONTEXTSWITCH(FTD);
						}
						FTD_3DIdle:
						
						/* is the TA active? */
						and.testz	p0, r3, #RENDER_IFLAGS_RENDERINPROGRESS;
						p0	ba	FTD_TAIdle;
						{
							/* is the 2D higher priority? */
							MK_LOAD_STATE(r3, ui32TACurrentPriority, drc0);
							MK_WAIT_FOR_STATE_LOAD(drc0);
							/* if the TA is higher priority just exit */
							isub16.testns	p0, r10, r3;
							p0 ba FTD_End;
							
							/* The TA is higher priority so request the TA to stop */
							STARTTACONTEXTSWITCH(FTD);
						}
						FTD_TAIdle:
						
						/* Wait for the operations to finish */
						ba	FTD_End;
					}
					FTD_SameAddrSpace:
					/* The contexts are the same so we can continue */
				}
				FTD_NoOperationsInProgress:
	#endif

				MK_MEM_ACCESS(ldad) r2, [r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sCommon.ui32Flags)], drc0;
				wdf		drc0;
				and.testz	p0, r2, #SGXMKIF_HWCFLAGS_DUMMY2D;
				/*
					If we do kick 2D, No need for a lapc as Kick2D will
					branch straight back to calling code
				*/
				p0 mov	r2, r10;
				p0 ba	Kick2D;
				/* if we get here then we are going to dummy process the 2DCMD */
				/* clear the flag */
				and	r2, r2, ~#SGXMKIF_HWCFLAGS_DUMMY2D;
				MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sCommon.ui32Flags)], r2;
				ba	DummyProcess2D;
			}
		}
	}
	FTD_End:
	/* branch back to calling code */
	lapc;
}

/*****************************************************************************
 Kick2D routine	- performs anything that is required before actually
 			  		starting the 2D blit

 inputs:	r0 - context of 2D about to be started
			r1 - CMD of the 2D about to be started
 temps:		r2, r3, r4, r5, r6, r7, r8, r9
*****************************************************************************/
Kick2D:
{
	MK_TRACE(MKTC_KICK2D_START, MKTF_2D);
	MK_TRACE_REG(r0, MKTF_2D);

	/* Reset the priority scheduling level block, as it is no longer blocked */
	MK_STORE_STATE(ui32BlockedPriority, #(SGX_MAX_CONTEXT_PRIORITY - 1));

	/* Save pointer to CCB command */
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s2DContext)], r0;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s2DCCBCmd)], r1;

	/* Save the PID for easy retrieval later */
	MK_MEM_ACCESS(ldad)	r2, [r0, HSH(DOFFSET(SGXMKIF_HW2DCONTEXT.ui32PID))], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui322DPID)], r2;
	MK_TRACE(MKTC_KICK2D_PID, MKTF_PID);
	MK_TRACE_REG(r2, MKTF_PID);

	/* Ensure the memory context is setup */
	SETUPDIRLISTBASE(K2D, r0, #SETUP_BIF_2D_REQ);


#if defined(FIX_HW_BRN_31272) || defined(FIX_HW_BRN_31780) || defined(FIX_HW_BRN_33920)
	SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_SOFT_RESET >> 2, #EUR_CR_MASTER_SOFT_RESET_PTLA_RESET_MASK);
	SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_SOFT_RESET >> 2, #0);
#endif

	WRITEHWPERFCB(2D, 2D_START, GRAPHICS, #0, p0, k2d_start_1, r2, r3, r4, r5, r6, r7, r8, r9);

	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_2DCMD.ui32CtrlSizeInDwords)], drc1;

#if defined (FIX_HW_BRN_32303) && ! defined(FIX_HW_BRN_32845)
	/* force clocks on to the master BIF */
	ldr		r3, #EUR_CR_MASTER_CLKGATECTL2 >> 2, drc0;
	wdf		drc0;

	and		r3, r3, #(~EUR_CR_MASTER_CLKGATECTL2_BIF_CLKG_MASK);
	or 		r3, r3, #(EUR_CR_CLKGATECTL_ON << EUR_CR_MASTER_CLKGATECTL2_BIF_CLKG_SHIFT);
	str		#EUR_CR_MASTER_CLKGATECTL2 >> 2, r3;
#endif

#if defined(FIX_HW_BRN_34811)
	/* force clocks on to the PTLA */
	ldr		r3, #EUR_CR_MASTER_CLKGATECTL2 >> 2, drc0;
	wdf		drc0;

	and		r3, r3, #(~EUR_CR_MASTER_CLKGATECTL2_PTLA_CLKG_MASK);
	or 		r3, r3, #(EUR_CR_CLKGATECTL_ON << EUR_CR_MASTER_CLKGATECTL2_PTLA_CLKG_SHIFT);
	str		#EUR_CR_MASTER_CLKGATECTL2 >> 2, r3;
#endif

	mov		r3, #OFFSET(SGXMKIF_2DCMD.aui32CtrlStream);
	iaddu32	r3, r3.low, r1;

	MK_MEM_ACCESS_CACHED(ldad)	r4, [r3, #1++], drc0;


#if defined(EUR_CR_2D_ALPHA) 
	/* Update the CR_2D_ALPHA register */
	wdf		drc1;
	MK_MEM_ACCESS_CACHED(ldad)	r5, [r1, #DOFFSET(SGXMKIF_2DCMD.ui32AlphaRegValue)], drc1;
	wdf		drc1;
	str		#EUR_CR_2D_ALPHA >> 2, r5;
#else
	wdf		drc1;
#endif

	/* Write the 2D control stream into the 2D fifo */
	MK_TRACE(MKTC_KICK2D_2DSLAVEPORT, MKTF_2D | MKTF_HW);
	K2D_NextEntry:
	{
		isub16.testz	r2, p0, r2, #1;
		wdf		drc0;
		#if defined(SGX_FEATURE_PTLA)
		#if defined(FIX_HW_BRN_29557)
		/*
			Note: performance impact would be reduced if the entire slave port
			stream was written with a single transition from/to supervisor.
		*/
		SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_PTLA_SLAVE_CMD_0 >> 2, r4);
		#else
		str	#EUR_CR_MASTER_PTLA_SLAVE_CMD_0 >> 2, r4;
		#endif /* FIX_HW_BRN_29557 */
		#else
		str	#0x4000 >> 2, r4;
		#endif
		MK_TRACE_REG(r4, MKTF_2D);
		!p0 MK_MEM_ACCESS_CACHED(ldad)	r4, [r3, #1++], drc0;
		!p0 ba	K2D_NextEntry;
	}

	MK_TRACE(MKTC_KICK2D_2DSLAVEPORT_DONE, MKTF_2D);

#if defined(SUPPORT_HW_RECOVERY)
	/* reset the Recovery counter */
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRTimerTicksSince2D)], #0;

#if defined(SGX_FEATURE_PTLA)
	/* invalidate the signatures */
	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], drc0;
	wdf		drc0;
	or		r1, r1, #0x10;
	MK_MEM_ACCESS_BPCACHE(stad) 	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], r1;
	idf		drc0, st;
	wdf		drc0;
#endif
#endif

	/* Indicate a 2D is in progress */
	MK_LOAD_STATE(r2, ui32I2DFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	or		r2, r2, #TWOD_IFLAGS_2DBLITINPROGRESS;
	MK_STORE_STATE(ui32I2DFlags, r2);
	MK_WAIT_FOR_STATE_STORE(drc0);

	/* Get the context priority */
	MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sCommon.ui32Priority)], drc0;
	wdf		drc0;

	/* Get the runlist offset using the Priority */
	/* Move chosen render context to start of complete list */
	MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sPrevDevAddr)], drc0;
	MK_MEM_ACCESS(ldad)	r5, [r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sNextDevAddr)], drc1;
	wdf		drc0;
	and.testnz	p0, r4, r4;
	wdf		drc1;
	and.testnz	p1, r5, r5;
	p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HW2DCONTEXT.sNextDevAddr)], r5;
	p1	MK_MEM_ACCESS(stad)	[r5, #DOFFSET(SGXMKIF_HW2DCONTEXT.sPrevDevAddr)], r4;
	p0	ba	K2D_NoHeadUpdate;
	{
		MK_STORE_STATE_INDEXED(s2DContextHead, r1, r5, r2);
	}
	K2D_NoHeadUpdate:
	p1	ba	K2D_NoTailUpdate;
	{
		MK_STORE_STATE_INDEXED(s2DContextTail, r1, r4, r3);
	}
	K2D_NoTailUpdate:
	idf		drc0, st;
	wdf		drc0;

	MK_LOAD_STATE_INDEXED(r4, s2DContextHead, r1, drc0, r2);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testnz	p0, r4, r4;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sPrevDevAddr)], #0;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sNextDevAddr)], r4;
	p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HW2DCONTEXT.sPrevDevAddr)], r0;
	MK_STORE_STATE_INDEXED(s2DContextHead, r1, r0, r2);
	p0	ba	K2D_NoTailUpdate2;
	{
		MK_STORE_STATE_INDEXED(s2DContextTail, r1, r0, r3);
	}
	K2D_NoTailUpdate2:
	idf		drc0, st;
	wdf		drc0;

	MK_TRACE(MKTC_KICK2D_END, MKTF_2D);

	/* return */
	lapc;
}



/*****************************************************************************
 Advance2DCCBROff routine - calculates the new ROff for the 2D CCB and
 									decrements the kick count

 inputs:	r0 - 2D context of 2D CCB to be advanced (maintained)
			r1 - CCB command of 2D kick just completed (maintained)
 outputs:	r2 - Count of commands outstanding
 temps:		r3, r4
*****************************************************************************/
Advance2DCCBROff:
{
	/* Calc. new read offset into the CCB */
	MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sCCBCtlDevAddr)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r3, [r2, #DOFFSET(PVRSRV_SGX_CCB_CTL.ui32ReadOffset)], drc0;
	MK_MEM_ACCESS_CACHED(ldad)	r4, [r1, #DOFFSET(SGXMKIF_2DCMD.ui32Size)], drc1;
	wdf		drc0;
	wdf		drc1;
	iaddu32	r3, r3.low, r4;
	and		r3, r3, #(SGX_CCB_SIZE - 1);
	MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_CCB_CTL.ui32ReadOffset)], r3;

	/* Decrement kick count for current context */
	MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.ui32Count)], drc0;
	wdf		drc0;
	iadd32		r2, r2.low, #-1;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.ui32Count)], r2;
	idf		drc0, st;
	wdf		drc0;

	/* return */
	lapc;
}

/*****************************************************************************
 Update2DSurfaceOps routine - updates any surface ops associated with
 									a 2D command.

 inputs:	r1 - CCB command of 2D kick just completed (maintained)
 temps:		r2, r3, r4, r5, r6
*****************************************************************************/
Update2DSurfaceOps:
{
	/* This may have unblocked another operation, so reset APM */
	MK_STORE_STATE(ui32ActivePowerCounter, #0);
	MK_STORE_STATE(ui32ActivePowerFlags, #SGX_UKERNEL_APFLAGS_ALL);
	MK_WAIT_FOR_STATE_STORE(drc0);

	MK_MEM_ACCESS_CACHED(ldad.f2)	r4, [r1, #DOFFSET(SGXMKIF_2DCMD.sShared.sTASyncData.ui32WriteOpsPendingVal)-1], drc1;
	MK_MEM_ACCESS_CACHED(ldad.f2)	r2, [r1, #DOFFSET(SGXMKIF_2DCMD.sShared.s3DSyncData.ui32WriteOpsPendingVal)-1], drc0;

	/* Now Update the TAWrites complete value */
	wdf		drc1;
	and.testz	p0, r5, r5;
	p0	ba	U2DSO_NoTASyncWriteOp;
	{
		mov	r6, #1;
		iaddu32 r4, r6.low, r4;
		MK_MEM_ACCESS_BPCACHE(stad)	[r5], r4;
	}
	U2DSO_NoTASyncWriteOp:

	MK_MEM_ACCESS_CACHED(ldad.f2)	r4, [r1, #DOFFSET(SGXMKIF_2DCMD.sShared.sDstSyncData.ui32WriteOpsPendingVal)-1], drc1;
	/* Now Update the 3DWrites complete value(s) */
	wdf		drc0;
	and.testz	p0, r3, r3;
	p0	ba	U2DSO_No3DSyncWriteOp;
	{
		/* just increment the read ops */
		mov	r6, #1;
		iaddu32 r2, r6.low, r2;
		MK_MEM_ACCESS_BPCACHE(stad) [r3], r2;
	}
	U2DSO_No3DSyncWriteOp:

	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_2DCMD.sShared.ui32NumSrcSync)], drc0;
	/* Now Update the DstWrites complete value(s) */
	wdf		drc1;
	and.testz	p0, r5, r5;
	p0	ba	U2DSO_NoDestOpComplete;
	{
		MK_TRACE(MKTC_U2DSO_UPDATEWRITEOPS, MKTF_2D | MKTF_SO);

		/* just increment the write ops */
		mov	r6, #1;
		iaddu32 r4, r6.low, r4;
		MK_MEM_ACCESS_BPCACHE(stad) [r5], r4;

		MK_TRACE_REG(r5, MKTF_2D | MKTF_SO);
		MK_TRACE_REG(r4, MKTF_2D | MKTF_SO);
	}
	U2DSO_NoDestOpComplete:

	/* Now Update the SrcReads complete value(s) */
	wdf		drc0;
	and.testz	p0, r2, r2;
	p0	ba	U2DSO_NoSrcSyncComplete;
	{
		mov		r3, #OFFSET(SGXMKIF_2DCMD.sShared.sSrcSyncData[0]);
		iadd32	r3, r3.low, r1;

		MK_MEM_ACCESS_CACHED(ldad.f2)	r4, [r3, #0++], drc0;
		U2DSO_UpdateNextSource:
		{
			MK_TRACE(MKTC_U2DSO_UPDATEREADOPS, MKTF_2D | MKTF_SO);

			isub16.testz	r2, p0, r2, #1;
			// load the ReadOpsComplete and ui32ReadOpsPendingVal
			mov		r6, #16;
			iaddu32	r3, r6.low, r3;
			/* Update the value */
			mov	r6, #1;
			wdf		drc0;
			iaddu32 r4, r6.low, r4;
			MK_MEM_ACCESS_BPCACHE(stad)	[r5], r4;

			MK_TRACE_REG(r5, MKTF_2D | MKTF_SO);
			MK_TRACE_REG(r4, MKTF_2D | MKTF_SO);

			!p0	MK_MEM_ACCESS_CACHED(ldad.f2)	r4, [r3, #0++], drc0;
			!p0 ba			U2DSO_UpdateNextSource;
		}
	}
	U2DSO_NoSrcSyncComplete:

	MK_LOAD_STATE(r2, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	shl.testns	p0, r2, #(31 - TA_IFLAGS_OOM_PR_BLOCKED_BIT);
	p0	ba	U2DSO_NoPRBlocked;
	{
		/* clear the flag */
		and	r2, r2, ~#TA_IFLAGS_OOM_PR_BLOCKED;
		MK_STORE_STATE(ui32ITAFlags, r2);
		MK_WAIT_FOR_STATE_STORE(drc0);

		/* restart the TA */
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

		/* wait for the stores */
		wdf		drc0;
	}
	U2DSO_NoPRBlocked:

	/* return */
	lapc;
}

/*****************************************************************************
 DummyProcess2D routine - Dummy process a 2D command as it may have
 								locked up previously

 inputs:	r0 - context of render about to be processed
			r1 - CMD of the 2D about to be processed
 temps:		r2, r3, r4, r5, r7
*****************************************************************************/
DummyProcess2D:
{
	MK_TRACE(MKTC_DUMMYPROC2D, MKTF_2D | MKTF_SCH);

	/* Reset the priority scheduling level block, as it is no longer blocked */
	MK_STORE_STATE(ui32BlockedPriority, #(SGX_MAX_CONTEXT_PRIORITY - 1));

	mov		r7, pclink;
	/* Update the surface sync ops */
	bal		Update2DSurfaceOps;
	/* move on the 2DCCB */
	bal		Advance2DCCBROff;
	mov		pclink, r7;

	/* test the count returned */
	and.testnz	p1, r2, r2;

	/* Send interrupt to host */
	mov	 			r2, #EUR_CR_EVENT_STATUS_SW_EVENT_MASK;
	str	 			#EUR_CR_EVENT_STATUS >> 2, r2;

	/* Get the context priority level */
	MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sCommon.ui32Priority)], drc0;
	wdf		drc0;

	/* Remove the current context from the 2D render contexts list */
	MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sPrevDevAddr)], drc0;
	MK_MEM_ACCESS(ldad)	r5, [r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sNextDevAddr)], drc1;
	wdf		drc0;
	and.testz	p0, r4, r4;	// is prev null?
	wdf		drc1;
	/* (prev == NULL) ? head = current->next : prev->nextptr = current->next */
	!p0	ba	DP2D_NoHeadUpdate;
	{
		MK_STORE_STATE_INDEXED(s2DContextHead, r1, r5, r2);
	}
	DP2D_NoHeadUpdate:
	!p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HW2DCONTEXT.sNextDevAddr)], r5;

	and.testz	p0, r5, r5;	// is next null?
	/* (next == NULL) ? tail == current->prev : next->prevptr = current->prev */
	!p0	ba	DP2D_NoTailUpdate;
	{
		MK_STORE_STATE_INDEXED(s2DContextTail, r1, r4, r3);
	}
	DP2D_NoTailUpdate:
	!p0	MK_MEM_ACCESS(stad)	[r5, #DOFFSET(SGXMKIF_HW2DCONTEXT.sPrevDevAddr)], r4;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sPrevDevAddr)], #0;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sNextDevAddr)], #0;
	idf		drc1, st;

	/* make sure all previous writes have completed */
	wdf		drc1;
	/* If there is something queued then add the current 2D context back to the tail of the 2D context list */
	!p1	ba		DP2D_NoKicksOutstanding;
	{
		MK_LOAD_STATE_INDEXED(r4, s2DContextTail, r1, drc0, r3);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and.testz	p0, r4, r4;
		!p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HW2DCONTEXT.sNextDevAddr)], r0;
		!p0	ba	DP2D_NoHeadUpdate2;
		{
			MK_STORE_STATE_INDEXED(s2DContextHead, r1, r0, r2);
		}
		DP2D_NoHeadUpdate2:
		MK_STORE_STATE_INDEXED(s2DContextTail, r1, r0, r3);
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sPrevDevAddr)], r4;
	}
	DP2D_NoKicksOutstanding:
	idf		drc0, st;
	wdf		drc0;

	/* now check to see if there are any other 2D we can do instead
		not bal so that is returns as if initial call */
	ba	Find2D;
}
#endif /* SGX_FEATURE_2D_HARDWARE */

/*****************************************************************************
 End of file (2d.asm)
*****************************************************************************/

