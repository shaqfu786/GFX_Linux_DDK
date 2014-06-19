/*****************************************************************************
 Name		: ta.asm

 Title		: USE program for TA kick event processing

 Author		: Imagination Technologies

 Created	: 01/01/2006

 Copyright	: 2006-2008 by Imagination Technologies Limited.
		  All rights reserved. No part of this software, either material or
		  conceptual may be copied or distributed, transmitted, transcribed,
		  stored in a retrieval system or translated into any human or
		  computer language in any form by any means, electronic, mechanical
		  manual or other-wise, or disclosed to third parties without the
		  express written permission of Imagination Technologies Limited,
		  Home Park Estate, Kings Langley, Hertfordshire, WD4 8LZ, U.K.

 Description	: USE program for host event kicks and automatic renders

 Program Type	: USE assembly language

 Version 	: $Revision: 1.319 $

 Modifications

*****************************************************************************/

#include "usedefs.h"

/***************************
 Sub-routine Imports
***************************/
.import MKTrace;
.import SetupDirListBase;
#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
.import SetupRequestor;
#endif
.import SwitchEDMBIFBank;
#if defined(FIX_HW_BRN_28889)
.import InvalidateBIFCache;
#endif

#if defined(SGX_FEATURE_MK_SUPERVISOR)
.import JumpToSupervisor;
#endif

.import IdleCore;
.import Resume;
.import LoadDPMContext;
.import StoreDPMContext;
.import DPMReloadFreeList;
.import DPMThresholdUpdate;

#if defined(FIX_HW_BRN_29798)
.import DPMStateClear;
#endif

.import EmitLoopBack;

.import LoadTAPB;
.import StoreTAPB;
#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
.import Load3DPB;
#endif
.import Store3DPB;
.import SetupDPMPagetable;

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
.import HaltTA;
.import	ResumeTA;
#endif
#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH)
.import Start3DContextSwitch;
#endif

#if (defined(FIX_HW_BRN_24304) || defined(FIX_HW_BRN_27510)) && defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
.import	ConfigureCache;
#endif

#if defined(FIX_HW_BRN_23055)
.import DPMPauseAllocations;
.import DPMResumeAllocations;
#endif

#if defined(FIX_HW_BRN_27311) && !defined(FIX_HW_BRN_23055)
.import	InitBRN27311StateTable;
#endif

.import ClearActivePowerFlags;

#if defined(SGX_FAST_DPM_INIT) && (SGX_FEATURE_NUM_USE_PIPES > 1)
.import InitPBLoopback;
#endif

.import FreeContextMemory;
.import MoveSceneToCompleteList;
.import CommonProgramEnd;

.import WriteHWPERFEntry;

/***************************
 Sub-routine Exports
***************************/
.export TAEventHandler;
.export TAHandler;
.export TAFinished;
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
.export KickTA;
#endif

/*****************************************************************************
 Macro definitions
*****************************************************************************/

/*****************************************************************************
 Start of program
*****************************************************************************/
.modulealign 2;
TAEventHandler:
{
	UKERNEL_PROGRAM_INIT_COMMON();

	WRITEHWPERFCB(TA, MK_TA_START, MK_EXECUTION, #0, p0, ta_event_start, r2, r3, r4, r5, r6, r7, r8, r9);

	/* Test the TA Finished event */
	and		r0, PA(SGX_MK_PDS_TA_PA), #SGX_MK_PDS_TAFINISHED;
	and.testnz	p0, r0, r0;
	!p0	br		TAEH_NoTAFinished;
	{
		MK_TRACE(MKTC_TAEVENT_TAFINISHED, MKTF_EV | MKTF_TA);

		ldr		r10, #EUR_CR_DPM_PAGE_STATUS >> 2, drc0;
		wdf		drc0;
		and		r10, r10, #EUR_CR_DPM_PAGE_STATUS_TA_MASK;
		shr		r10, r10, #EUR_CR_DPM_PAGE_STATUS_TA_SHIFT;

		WRITEHWPERFCB(TA, TA_END, GRAPHICS, r10, p0, kta_end_1, r2, r3, r4, r5, r6, r7, r8, r9);
		WRITE_SIGNATURES(TA, r2, r3, r4, r5);

#if defined(SGX_SUPPORT_HWPROFILING)
		/* Move the frame no. into the structure PVRSRV_SGX_HWPROFILING */
		MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32TAFrameNum)], drc0;
		MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sHWProfilingDevVAddr)], drc0;
		wdf		drc0;
		MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_HWPROFILING.ui32EndTA)], r3;
#endif /* SGX_SUPPORT_HWPROFILING */

		mov		r0, #EUR_CR_EVENT_HOST_CLEAR_TA_FINISHED_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r0;

		mov		R_EDM_PCLink, #LADDR(TAEH_NoTAFinished);
		ba		TAFinished;
	}
	TAEH_NoTAFinished:

	WRITEHWPERFCB(TA, MK_TA_END, MK_EXECUTION, #0, p0, ta_event_end, r2, r3, r4, r5, r6, r7, r8, r9);

	/* Done */
	UKERNEL_PROGRAM_END(#0, MKTC_TAEVENT_END, MKTF_EV | MKTF_TA);
}

.align 2;
TAHandler:
{
	UKERNEL_PROGRAM_INIT_COMMON();

	CLEARPENDINGLOOPBACKS(r0, r1);

	WRITEHWPERFCB(TA, MK_TA_START, MK_EXECUTION, #0, p0, ta_start, r2, r3, r4, r5, r6, r7, r8, r9);

	/* Test the TA finished event */
	and		r0, PA(ui32PDSIn0), #USE_LOOPBACK_TAFINISHED;
	and.testnz	p0, r0, r0;
	!p0	ba		TAH_NoTAFinished;
	{
		MK_TRACE(MKTC_TALB_TAFINISHED, MKTF_EV | MKTF_TA);

		mov		r0, #EUR_CR_EVENT_HOST_CLEAR_TA_FINISHED_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r0;

		mov		R_EDM_PCLink, #LADDR(TAH_NoTAFinished);
		ba		TAFinished;
	}
	TAH_NoTAFinished:

	/* Test the FindTA call */
	and		r0, PA(ui32PDSIn0), #USE_LOOPBACK_FINDTA;
	and.testnz	p0, r0, r0;
	!p0	ba		TAH_NoFindTA;
	{
		MK_TRACE(MKTC_TALB_FINDTA, MKTF_EV | MKTF_TA);

		bal		FindTA;
	}
	TAH_NoFindTA:

#if defined(SGX_FAST_DPM_INIT) && (SGX_FEATURE_NUM_USE_PIPES > 1)
	/* Test the InitPB call */
	and		r0, PA(ui32PDSIn0), #USE_LOOPBACK_INITPB;
	and.testnz	p0, r0, r0;
	!p0	ba		TAH_NoInitPB;
	{
		PVRSRV_SGXUTILS_CALL(InitPBLoopback);
	}
	TAH_NoInitPB:
#endif /* SGX_FAST_DPM_INIT */

	WRITEHWPERFCB(TA, MK_TA_END, MK_EXECUTION, #0, p0, ta_end, r2, r3, r4, r5, r6, r7, r8, r9);

	/* Done */
	UKERNEL_PROGRAM_END(#0, MKTC_TALB_END, MKTF_EV | MKTF_TA);
}


#if defined(FIX_HW_BRN_23055)
/*****************************************************************************
 RestoreZLSThreshold routine - Resets the ZLS threshold back to the value
 								saved off by ZeroZLSThreshold

 inputs:
 temps:	r1
*****************************************************************************/
RestoreZLSThreshold:
{
	MK_TRACE(MKTC_RESTORE_ZLS_THRESHOLD, MKTF_SPM | MKTF_HW);

	/* Make sure the 3D is not deallocating memory */
	mov		r16, #USE_IDLECORE_3D_REQ;
	PVRSRV_SGXUTILS_CALL(IdleCore);

	/* Reduce the threshold by the number of page inserted for the workaround. */
	MK_MEM_ACCESS_BPCACHE(ldaw)	r1, [R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16SavedZLSThreshold)], drc0;
	wdf		drc0;
	str		#EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, r1;
	
	/* Reload the free list to make the hardware use the new threshold */
	PVRSRV_SGXUTILS_CALL(DPMReloadFreeList);

	/* Restart allocations. */
	RESUME(r9);

	/* return */
	lapc;
}

/*****************************************************************************
 RemoveReserveMemory routine	- Reset the TA thresholds back to the saved values.

	FIXME: move this function to sgx_utils.asm some how so it not duplicated.
 inputs:
 temps:
*****************************************************************************/
RemoveReserveMemory:
{
	MK_TRACE(MKTC_REMOVE_RESERVE_MEM, MKTF_SCH | MKTF_SPM);

	/*
		Store the maximum threshold increment
	*/
	mov		r16, #USE_IDLECORE_3D_REQ;
	PVRSRV_SGXUTILS_CALL(IdleCore);

	/* Reduce the threshold by the number of page inserted for the workaround. */
	MK_MEM_ACCESS_BPCACHE(ldaw)	r2, [R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16Num23055PagesUsed)], drc0;
	ldr		r1, #EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, drc0;
	wdf		drc0;
	isub16	r1, r1, r2;
	str		#EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, r1;

	/* Reload the free list to make the hardware use the new threshold */
	PVRSRV_SGXUTILS_CALL(DPMReloadFreeList);

#if defined(DEBUG)
	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32MaximumZLSThresholdIncrement)], drc0;
	wdf		drc0;
	isub16.tests	p2, r1, r2;
	p2	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32MaximumZLSThresholdIncrement)], r0;
#endif /* defined(DEBUG) */
	MK_MEM_ACCESS_BPCACHE(staw)	[R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16Num23055PagesUsed)], #0;
	idf		drc0, st;
	wdf		drc0;

	/* Restart allocations. */
	RESUME(r9);

	/* return */
	lapc;
}
#endif /* defined(FIX_HW_BRN_23055) */

/*****************************************************************************
 TAFinished routine	- process the TA finished event

 inputs:
 temps:
*****************************************************************************/
.import WaitForReset;
TAFinished:
{
#if (defined(FIX_HW_BRN_24304) || defined(FIX_HW_BRN_27510)) && defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	CONFIGURECACHE(#CC_FLAGS_HW_FINISH);
#endif

	/* TA Finished could unblock a render, so reset active power management state. */
	MK_STORE_STATE(ui32ActivePowerCounter, #0);
	MK_STORE_STATE(ui32ActivePowerFlags, #SGX_UKERNEL_APFLAGS_ALL);
	MK_WAIT_FOR_STATE_STORE(drc0);

	MK_LOAD_STATE(r0, ui32ITAFlags, drc1);
	MK_WAIT_FOR_STATE_LOAD(drc1);

#if defined(FIX_HW_BRN_23055)
	/* Check if the TA_IFLAGS_FIND_MTE_PAGE flag is set */
	shl.testns	p0, r0, #(31 - TA_IFLAGS_FIND_MTE_PAGE_BIT);
	p0	ba	TAF_RestoreZLSThreshold;
	{
		/* The no longer need to find the MTE page so clear 
			the flag and restore threshold  */
		and r0, r0, ~#TA_IFLAGS_FIND_MTE_PAGE;
		
		/* Reset the zls threshold */
		bal	RestoreZLSThreshold;
	}
	TAF_RestoreZLSThreshold:
#endif

#if defined(FIX_HW_BRN_23055)
	/* Check if we need to reset the TA thresholds. */
	shl.tests	p1, r0, #(31 - TA_IFLAGS_INCREASEDTHRESHOLDS_BIT);
	p1	bal		RemoveReserveMemory;
#endif /* defined(FIX_HW_BRN_23055) */

#if 1 // SPM_PAGE_INJECTION;
	#if defined(DEBUG)
	/* Clear SPM deadlock counters */
	mov		r2, #0;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32ForceGlobalOOMCount)], r2;
	idf		drc1, st;
	wdf 	drc1;
	#endif

	/* If we injected reserve pages hijack tafinish and issue loopback */
	shl.testns	p1, r0, #(31 - TA_IFLAGS_SPM_DEADLOCK_MEM_BIT);
	#if defined(FIX_HW_BRN_23055)
	p1	shl.testns	p1, r0, #(31 - TA_IFLAGS_INCREASEDTHRESHOLDS_BIT);
	#endif
	/* If we kicked a render to free reserve memory */
	p1	shl.testns	p1, r0, #(31 - TA_IFLAGS_SPM_STATE_FREE_ABORT_BIT);
	p1	ba	TAF_NoReserveFree;
	{
		MK_TRACE(MKTC_TAF_RESERVE_MEM, MKTF_SPM | MKTF_SCH);

	#if defined(FIX_HW_BRN_23055)
		/* if this was a last kick, handle TAF as normal. */
		and	r0, r0, ~#(TA_IFLAGS_BOTH_REQ_DENIED | TA_IFLAGS_INCREASEDTHRESHOLDS | TA_IFLAGS_IDLE_ABORT);
	#endif
	
		/* Have we done the State free render? */
		shl.tests	p1, r0, #(31 - TA_IFLAGS_SPM_STATE_FREE_ABORT_BIT);
		p1	ba	TAF_ReserveFreeRenderCompleted;
		{
	#if defined(DBGBREAK_ON_SPM)
			MK_TRACE_REG(r0, MKTF_SPM);
			MK_ASSERT_FAIL(MKTC_TAF_RESERVE_MEM_REQUEST_RENDER);
	#endif
			MK_TRACE(MKTC_TAF_RESERVE_MEM_REQUEST_RENDER, MKTF_SPM | MKTF_SCH);
			
			/* Set the flags so we know what caused the "partial" render */
			or	r0, r0, #(TA_IFLAGS_ABORTINPROGRESS | TA_IFLAGS_SPM_STATE_FREE_ABORT \
						  | TA_IFLAGS_ABORTALL);
			MK_STORE_STATE(ui32ITAFlags, r0);
			MK_WAIT_FOR_STATE_STORE(drc0);

	#if defined(DEBUG)
			MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32NumDLRenders)], drc0;
			wdf				drc0;
			iaddu32			r1, r1.low, #1;
			MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32NumDLRenders)], r1;
	#endif
	
	#if defined(SUPPORT_HW_RECOVERY)
			/* Reset the recovery counter */
			MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRTimerTicksSinceTA)], #0;
		
			/* invalidate the signatures */
			MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], drc0;
			wdf		drc0;
			or		r1, r1, #0x1;
			MK_MEM_ACCESS_BPCACHE(stad) 	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], r1;
	#endif

			/* Issue a loopback to kick a render, there is no need to check sync objects 
				as we must have done a partial render to get here */
			TATERMINATE();
			ba		TAF_End;
		}
		TAF_ReserveFreeRenderCompleted:
		
		MK_TRACE(MKTC_TAF_RESERVE_FREE_RENDER_FINISHED, MKTF_SPM | MKTF_SCH);
		
		/* If this was a last kick, mark the scene for "dummy" processing as we have already rendered the scene */
		MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTACCBCmd)], drc0;
		wdf		drc0;
		MK_MEM_ACCESS_CACHED(ldad)	r1, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
		MK_MEM_ACCESS_BPCACHE(ldad)	R_RTData, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], drc0;
		wdf		drc0;
		MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc0;
		wdf		drc0;
		and.testz	p1, r1, #SGXMKIF_TAFLAGS_LASTKICK;
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
		/* If context switch is in progress, the scene may have work left to do */
		!p1	and.testnz	p1, r0, #TA_IFLAGS_HALTTA;
#endif
		p1	or		r2, r2, #SGXMKIF_HWRTDATA_CMN_STATUS_ENABLE_ZLOAD_MASK;		
		p1	ba	TAF_UpdateRTDataStatus;
		{
			MK_TRACE(MKTC_TAF_RESERVE_FREE_DUMMY_RENDER, MKTF_SPM | MKTF_SCH);

			or		r2, r2, #SGXMKIF_HWRTDATA_CMN_STATUS_DUMMYPROC;
		}
		TAF_UpdateRTDataStatus:
		/* write the updated status to memory */
		MK_MEM_ACCESS(stad)	[R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], r2;
		idf		drc0, st;
		wdf		drc0;

	#if defined(EUR_CR_TE_RGNHDR_INIT)
		/* We need to reinitialise the RgnHdrs ready for the next kick */
		str		#EUR_CR_TE_RGNHDR_INIT >> 2, #EUR_CR_TE_RGNHDR_INIT_PULSE_MASK;
	#endif

		/* Idle the 3D as it could be deallocating memory */
		IDLE3D();

		str		#EUR_CR_DPM_TASK_STATE >> 2, #EUR_CR_DPM_TASK_STATE_CLEAR_MASK;
		str		#EUR_CR_DPM_TASK_CONTROL >> 2, #EUR_CR_DPM_TASK_CONTROL_CLEAR_MASK;
		str		#EUR_CR_MTE_OTPM_OP >> 2, #EUR_CR_MTE_OTPM_OP_CSM_INV_MASK;
		mov		r2, #EUR_CR_TE_TPCCONTROL_FLUSH_MASK;
		str		#EUR_CR_TE_TPCCONTROL >> 2, r2;

		mov		r3, #(EUR_CR_EVENT_STATUS_DPM_STATE_CLEAR_MASK | \
				      EUR_CR_EVENT_STATUS_DPM_CONTROL_CLEAR_MASK | \
					  EUR_CR_EVENT_STATUS_TPC_FLUSH_MASK | \
				      EUR_CR_EVENT_STATUS_OTPM_INV_MASK);

		ENTER_UNMATCHABLE_BLOCK;
		TAF_WaitForContextReset:
		{
			ldr		r2, #EUR_CR_EVENT_STATUS >> 2, drc0;
			wdf		drc0;
			and		r2, r2, r3;
			xor.testnz	p0, r2, r3;
			p0	ba		TAF_WaitForContextReset;
		}
		LEAVE_UNMATCHABLE_BLOCK;

		RESUME_3D(r2);

		mov		r3, #(EUR_CR_EVENT_HOST_CLEAR_DPM_STATE_CLEAR_MASK | \
			     	  EUR_CR_EVENT_HOST_CLEAR_DPM_CONTROL_CLEAR_MASK | \
					  EUR_CR_EVENT_HOST_CLEAR_TPC_FLUSH_MASK | \
			     	  EUR_CR_EVENT_HOST_CLEAR_OTPM_INV_MASK);
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r3;
	
	#if defined(EUR_CR_TE_RGNHDR_INIT)
		ENTER_UNMATCHABLE_BLOCK;
		TAF_WaitForRgnInit:
		{
			ldr		r2, #EUR_CR_EVENT_STATUS2 >> 2, drc0;
			wdf		drc0;
			shl.testns	p0, r2, #(31 - EUR_CR_EVENT_STATUS2_TE_RGNHDR_INIT_COMPLETE_SHIFT);
			p0	ba		TAF_WaitForRgnInit;
		}
		LEAVE_UNMATCHABLE_BLOCK;

		mov		r3, #EUR_CR_EVENT_HOST_CLEAR2_TE_RGNHDR_INIT_COMPLETE_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR2 >> 2, r3;
	#endif /* EUR_CR_TE_RGNHDR_INIT */

	#if defined(SGX_FEATURE_MP)
		/* Switch to the application address space */
		SWITCHEDMTOTA();

		/* Find out how big the Tail Pointer Memory is for the render target */
		MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32TailSize)], drc1;
		/* Find out how many cores are enabled */
		MK_LOAD_STATE_CORE_COUNT_TA(r3, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		/* Start with core 0 */
		mov		r4, #0;

		TAF_ClearNextCoreTPtrs:
		{		
			/* Find out the based address for the TailPtrs for each core */
			READCORECONFIG(r5, r4, #EUR_CR_TE_TPC_BASE >> 2, drc1);
			wdf		drc1;
			/* Convert the byte size into a number of bursts (16DWORDS) */
			shr		r6, r2, #6;

			TAF_ClearNextTPMem:
			{
				/* Do the burst */
				MK_MEM_ACCESS_BPCACHE(stad.f16)	[r5, #0++], #0;

				/* have we finished yet */
				isub16.testnz	r6, p0, r6, #1;
				p0	ba	TAF_ClearNextTPMem;
			}

			REG_INCREMENT(r4, p0);
			/* Have we cleared all the enabled cores */
			xor.testnz	p0, r4, r3;
			p0	ba	TAF_ClearNextCoreTPtrs;
		}
		/* Wait for all the writes to flush */
		idf 	drc1, st;
		wdf		drc1;

		SWITCHEDMBACK();
	#endif

	#if defined(EUR_CR_MASTER_DPM_PAGE_MANAGEOP_SERIAL_MASK)
		READMASTERCONFIG(r2, #EUR_CR_MASTER_DPM_PAGE_MANAGEOP >> 2, drc0);
		wdf		drc0;
		and		r2, r2, ~#EUR_CR_MASTER_DPM_PAGE_MANAGEOP_SERIAL_MASK;
		/* Deactivate DPM serial mode */
		WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_PAGE_MANAGEOP >> 2, r2, r1);

		/* Reset PIM spec timeout count */
		READMASTERCONFIG(r2, #EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, drc0);
		wdf		drc0;
		and		r2, r2, ~#EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC_TIMOUT_CNT_MASK;
		or		r2, r2, #(0x1FFF << EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC_TIMOUT_CNT_SHIFT);
		WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, r2, r1);
	#endif

		/* If Deadlock memory was injected we need to remove it by dropping the threshold down again */
		shl.testns	p1, r0, #(31 - TA_IFLAGS_SPM_DEADLOCK_MEM_BIT);
		p1	ba	TAF_NoSPMDeadlock;
		{
			/* The render to free the reserve pages has completed. So remove them from the freelist */
			/* Reduce the threshold by the number of page inserted for the workaround. */
			MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32DeadlockPageIncrease)], drc0;
			ldr		r2, #EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, drc0;
			wdf		drc0;
			isub16	r2, r2, r1;
			str		#EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, r2;

	#if !defined(FIX_HW_BRN_23055)
			/* Restore the GLOBAL threshold to the before pages were injected */
			MK_MEM_ACCESS_BPCACHE(ldad) 	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32SavedGlobalList)], drc0;
			wdf		drc0;
			or		r2, r2, #EUR_CR_DPM_TA_GLOBAL_LIST_POLICY_MASK;
			str		#EUR_CR_DPM_TA_GLOBAL_LIST >> 2, r2;
	#endif

			/* Reload the free list to make the hardware use the new threshold */
			PVRSRV_SGXUTILS_CALL(DPMReloadFreeList);
			
			MK_TRACE(MKTC_TAF_SPM_DEADLOCK_MEM_REMOVED, MKTF_SPM | MKTF_SCH);
		}
		TAF_NoSPMDeadlock:
	}
	TAF_NoReserveFree:
#endif

#if defined(FIX_HW_BRN_32302) || defined(SUPPORT_SGX_DOUBLE_PIXEL_PARTITIONS)
	mov r2, #TA_IFLAGS_ZLSTHRESHOLD_RESTORED;
	and.testz	p0, r0, r2;
	/*
	if (p0)  we have not restored, so skip lowering again */
	p0	ba	TF_SkipLowerAgain;
	
	MK_MEM_ACCESS_CACHED(ldaw)		r3, [R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16RealZLSThreshold)], drc0;
	wdf		drc0;
	mov r2, #(SGX_THRESHOLD_REDUCTION_FOR_VPCHANGE);
	isub16 r3, r3, r2;
	MK_TRACE(MKTC_LOWERED_TO_PDS_THRESHOLD, MKTF_TA | MKTF_SPM);
	MK_TRACE_REG(r3, MKTF_TA | MKTF_SPM);
	str		#EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, r3;
	/* Clear flag to indicate we have not restored(i.e staying LOWERED)*/
	mov	r2, ~#TA_IFLAGS_ZLSTHRESHOLD_RESTORED;
	and	r0, r0, r2;
	MK_STORE_STATE(ui32ITAFlags, r0);
	MK_WAIT_FOR_STATE_STORE(drc0);


	/* Get the TA and 3D HWPBDescs */
	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc.uiAddr)], drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DHWPBDesc.uiAddr)], drc0;
	wdf drc0;

	/* Check the TA and 3D PB match? */
	xor.testz	p0, r1, r2;
	!p0	ba 	TAF_ThresholdChangeNo3DIdle;
	{
		/* Because the TA and 3D PB's match, the 3D also needs to be idled! */
		mov	r16, #USE_IDLECORE_3D_REQ;
		PVRSRV_SGXUTILS_CALL(IdleCore);
	}
	TAF_ThresholdChangeNo3DIdle:

	PVRSRV_SGXUTILS_CALL(DPMReloadFreeList);
	xor.testz	p0, r1, r2;
	!p0	ba	TAF_ThresholdChangeNo3DResume;
	{
		RESUME(r1);
	}
	TAF_ThresholdChangeNo3DResume:
	TF_SkipLowerAgain:
#endif

#if defined(FIX_HW_BRN_31109)
	and		r0, r0, ~#TA_IFLAGS_TIMER_DETECTED_SPM;
#endif
#if defined(FIX_HW_BRN_29424) || defined(FIX_HW_BRN_29461) || defined(FIX_HW_BRN_31109)
	and 	r0, r0, ~#TA_IFLAGS_OUTOFMEM_GLOBAL;
#endif
	and		r1, r0, ~#(TA_IFLAGS_TAINPROGRESS | TA_IFLAGS_HALTTA | \
						TA_IFLAGS_SPM_DEADLOCK | TA_IFLAGS_SPM_DEADLOCK_GLOBAL | TA_IFLAGS_SPM_DEADLOCK_MEM | \
						TA_IFLAGS_SPM_STATE_FREE_ABORT);

	and		r1, r1, ~#(TA_IFLAGS_HWLOCKUP_MASK);

	/* Clear the flags in memory */
	MK_STORE_STATE(ui32ITAFlags, r1);
	MK_WAIT_FOR_STATE_STORE(drc0);

#if defined(EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC)
	/* Reset the register to default timeout value for the next TA command */
	READMASTERCONFIG(r2, #EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, drc0);
	wdf		drc0;
	and		r2, r2, ~#EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC_TIMOUT_CNT_MASK;
	or		r2, r2, #0x1FFF;
	WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, r2, r1);
#endif

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	#if defined(FIX_HW_BRN_33657) && defined(SUPPORT_SECURE_33657_FIX)
	/* Is this a completeonterminate? If so, it's real TA finish */
	MK_MEM_ACCESS_CACHED(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32VDMCtxTerminateMode)], drc0;
	wdf		drc0;
	and.testnz	p0, r1, #VDM_TERMINATE_COMPLETE_ONLY;
	p0	ba		TAF_CompleteOnTerminateFinish;
	#endif

	/* Check for TA being interrupted */
	and.testz	p0, r0, #TA_IFLAGS_HALTTA;
	p0	ba		TAF_NoContextSwitch;
	{
		/* Load the TA HWRENDERCONTEXT as it will be needed */
		MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderContext)], drc0;
		wdf		drc0;
		
	#if defined(FIX_HW_BRN_31769) || defined(FIX_HW_BRN_33657)
		/* Ensure we process all incomplete index blocks */
		SWITCHEDMTOTA();
		MK_MEM_ACCESS(ldad)  r2, [r1, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sMasterVDMSnapShotBufferDevAddr)], drc0;
		wdf			drc0;
		/* Realign to a byte address */
		shl			r2, r2, #EURASIA_VDM_SNAPSHOT_BUFFER_ALIGNSHIFT;
		#if defined(FIX_HW_BRN_31769)
		MK_MEM_ACCESS_BPCACHE(ldad)	r3, [r2, #14], drc0;
		MK_MEM_ACCESS_BPCACHE(ldad)	r4, [r2, #27], drc0;
		wdf			drc0;
		and.testz	p0, r3, r3;
		p0	ba		TAF_NoIndexBlockUpdate;
		{
			or		r4, r4, #1;
			MK_MEM_ACCESS_BPCACHE(stad)	[r2, #27], r4;
			idf		drc0, st;
			wdf		drc0;	
		}
		TAF_NoIndexBlockUpdate:
		#endif /* FIX_HW_BRN_31769 */
		
		#if defined(FIX_HW_BRN_33657)
		/* Patch the PIM number from which to resume */
		READMASTERCONFIG(r3, #EUR_CR_MASTER_VDM_PIM_STATUS >> 2, drc0);
		MK_LOAD_STATE_CORE_COUNT_TA(r4, drc0);
		wdf		drc0;
		shr		r3, r3, #EUR_CR_MASTER_VDM_PIM_STATUS_VALUE_SHIFT;
		iaddu32		r4, r4.low, r3;
		MK_MEM_ACCESS_BPCACHE(stad)	[r2, #23], r4;
		idf		drc0, st;
		wdf		drc0;
		#endif
		SWITCHEDMBACK();

		#if defined(FIX_HW_BRN_33657)
		/* Save the shadow VDM PIM to the RTdata */
		MK_TRACE_REG(r3, MKTF_TA | MKTF_CSW);

		MK_MEM_ACCESS_BPCACHE(ldad)	R_RTData, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], drc0;
		wdf		drc0;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32VDMPIM)], r3;
		idf		drc0, st;
		wdf		drc0;
		#endif
	#endif
	
	#if defined(SGX_FEATURE_MP) && defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
		/* Debug the number of SAs which were saved off */
		MK_TRACE(MKTC_TAF_DEBUG_SAS, MKTF_TA | MKTF_CSW);
		MK_MEM_ACCESS(ldad)  r2, [r1, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sSABufferDevAddr)], drc0;
		MK_MEM_ACCESS(ldad)  r3, [r1, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32SABufferStride)], drc0;
		wdf             drc0;
		
		MK_TRACE_REG(r2, MKTF_TA | MKTF_CSW);

		/* Switch to TA requestor context */
		SWITCHEDMTOTA();
		
		#if 0 // !SGX_SUPPORT_MASTER_ONLY_SWITCHING
		MK_LOAD_STATE_CORE_COUNT_TA(r6, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		isub16  r6, r6, #1;     /* start with core N-1 */
		#else
		mov		r6, #0; /* core0 only */
		#endif
		TAF_DebugSAsNextCore:
		{
			/* offset the base address by the core index */
			imae    r4, r3.low, r6.low, r2, u32;
			/* Load number of SAs stored */
			MK_MEM_ACCESS_BPCACHE(ldad)             r5, [r4], drc0;
			wdf             drc0;
			MK_TRACE_REG(r5, MKTF_TA | MKTF_CSW);
			isub16.testns   r6, p0, r6, #1;
			p0      ba      TAF_DebugSAsNextCore;
		}
		
		/* Switch back */
		SWITCHEDMBACK();
	#endif

		/* We were trying to interrupt the TA, check if this happened */
	#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && !defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
		READMASTERCONFIG(r0, #EUR_CR_MASTER_VDM_CONTEXT_STORE_STATUS >> 2, drc0);
		wdf		drc0;
		mov		r2, #(SGX_MK_VDM_CTX_STORE_STATUS_NA_MASK | \
						SGX_MK_VDM_CTX_STORE_STATUS_COMPLETE_MASK);
		and		r0, r0, r2;
		xor.testz		p0, r0, r2;
	#endif
	#if defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH) && !defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH)
		ldr		r0, #EUR_CR_VDM_CONTEXT_STORE_STATUS >> 2, drc0;
		mov		r2, #EUR_CR_VDM_CONTEXT_STORE_STATUS_NA_MASK;
		wdf		drc0;
		MK_TRACE_REG(r0, MKTF_TA | MKTF_SCH | MKTF_CSW);
		and.testnz		p0, r0, r2;
	#endif
	#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
		/* 
			As we have context switch support in the slave and master VDMs we 
			need to check & save which cores have done what.
		*/
		mov		r0, #USE_FALSE;
		READMASTERCONFIG(r2, #EUR_CR_MASTER_VDM_CONTEXT_STORE_STATUS >> 2, drc0);
		mov		r3, #(EUR_CR_MASTER_VDM_CONTEXT_STORE_STATUS_NA_MASK | \
						EUR_CR_MASTER_VDM_CONTEXT_STORE_STATUS_COMPLETE_MASK);
		wdf		drc0;
		#if defined(FIX_HW_BRN_30893) || defined(FIX_HW_BRN_30970)
		/* The register may be incorrect after the first context load. If the
		 * master VDM was already TA&complete, ignore the config register.
		 */
		MK_MEM_ACCESS(ldad)	r7, [r1, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32MasterVDMCtxStoreStatus)], drc0;
		wdf		drc0;
		or		r2, r2, r7;
		MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32MasterVDMCtxStoreStatus)], r2;
		idf		drc0, st;
		wdf		drc0;
		#endif

		and		r2, r2, r3;
		xor.testnz		p0, r2, r3;
		p0	mov		r0, #USE_TRUE;
		
		MK_TRACE_REG(r2, MKTF_TA | MKTF_SCH | MKTF_CSW);

		/* Debug context switch status */
		MK_LOAD_STATE_CORE_COUNT_TA(r4, drc0);
		mov		r3, #0;
		MK_WAIT_FOR_STATE_LOAD(drc0);
		TAF_DebugNextCoreStatus:
		{
			READCORECONFIG(r2, r3, #EUR_CR_VDM_CONTEXT_STORE_STATUS >> 2, drc0);
			wdf		drc0;
			MK_TRACE_REG(r2, MKTF_TA | MKTF_SCH | MKTF_CSW);
			REG_INCREMENT(r3, p2);
			xor.testnz	p0, r3, r4;
			p0	ba	TAF_DebugNextCoreStatus;			
		}
			
		/* If r0 = USE_FALSE, all the VDMs completed and therefore this a is TA finish event */
		xor.testz	p0, r0, #USE_FALSE;
	#endif /* SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH && SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH */
		p0	ba		TAF_NoContextSwitch;
		{
	#if defined(FIX_HW_BRN_33657) && !defined(SUPPORT_SECURE_33657_FIX)
			/* Re-enable security */
			SGXMK_SPRV_UTILS_WRITE_REG(#EUR_CR_USE_SECURITY >> 2, #0);
	#endif
			/* Going to switch context so save off the EUR_CR_TE_STATE register */
			MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderDetails)], drc0;
			mov				r3, #OFFSET(SGXMKIF_HWRENDERDETAILS.ui32TEState[0]);
			wdf				drc0;
			iaddu32			r2, r3.low, r2;
			COPYCONFIGREGISTERTOMEM_TA(TAF_InterruptedStoreTEState, EUR_CR_TE_STATE, r2, r3, r4, r5);
			
			/* Flush TPC to ensure no writes to deallocated memory are possible */
			mov		r2, #EUR_CR_TE_TPCCONTROL_FLUSH_MASK;
			str		#EUR_CR_TE_TPCCONTROL >> 2, r2;
			mov		r2, #EUR_CR_EVENT_STATUS_TPC_FLUSH_MASK;

			ENTER_UNMATCHABLE_BLOCK;
			TAF_InterruptedWaitForTPCFlush:
			{
				ldr		r3, #EUR_CR_EVENT_STATUS >> 2, drc0;
				wdf		drc0;
				and.testz	p0, r3, r2;
				p0	ba		TAF_InterruptedWaitForTPCFlush;
			}
			LEAVE_UNMATCHABLE_BLOCK;

			mov		r2, #EUR_CR_EVENT_HOST_CLEAR_TPC_FLUSH_MASK;
			str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r2;
			
			TAF_ContextSwitch:
			
			/* Move the Context to the tail of the run list */
			MK_MEM_ACCESS(ldad)	r2, [r1, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], drc0;
			wdf		drc0;
			/* If the NextPartialDevAddr is null it is the only context in list */
			and.testz	p0, r2, r2;
			p0	ba	TAF_InterruptedAlreadyTail;
			{
				/* */
				MK_LOAD_STATE(r4, ui32TACurrentPriority, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);

				/* Get the old Tail */
				MK_LOAD_STATE_INDEXED(r6, sPartialRenderContextTail, r4, drc0, r5);
				/* Update the Head */
				MK_STORE_STATE_INDEXED(sPartialRenderContextHead, r4, r2, r3);
				MK_MEM_ACCESS(stad)	[r2, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], #0;
				/* Make context next ptr null */
				MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], #0;
				MK_WAIT_FOR_STATE_LOAD(drc0);
				/* Update the Tail */
				MK_STORE_STATE_INDEXED(sPartialRenderContextTail, r4, r1, r5);
				/* Link context and old tail to each other */
				MK_MEM_ACCESS(stad)	[r6, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], r1;
				MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], r6;
			}
			TAF_InterruptedAlreadyTail:
			
			/* Complete any outstanding stores */
			idf		drc0, st;
			wdf		drc0;

			bal		HaltTA;

			ba		TAF_End;
		}
	}
	TAF_NoContextSwitch:

	MK_TRACE(MKTC_TAFINISHED_NOCONTEXTSWITCH, MKTF_TA | MKTF_CSW);

	#if defined(FIX_HW_BRN_33657) && defined(SUPPORT_SECURE_33657_FIX)
	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTACCBCmd)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_CACHED(ldad)	r0, [r0, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
	wdf		drc0;
	and.testz	p0, r0, #SGXMKIF_TAFLAGS_LASTKICK;
	p0		ba	TAF_CompleteOnTerminateFinish;
	{
		/* If last in scene, complete and terminate the TA */
		MK_TRACE(MKTC_TAFINISHED_TERM_COMPLETE_START, MKTF_TA | MKTF_CSW);
	
		ldr		r2, #EUR_CR_TE_PSG >> 2, drc0;
		wdf		drc0;
		or		r2, r2, #EUR_CR_TE_PSG_COMPLETEONTERMINATE_MASK;
		str		#EUR_CR_TE_PSG >> 2, r2;
	
		/* Copy the MTE bounding box size */
		MK_MEM_ACCESS_BPCACHE(ldad)	r6, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderDetails)], drc0;
		wdf		drc0;
		MK_MEM_ACCESS(ldad)	r4, [r6, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWRTDataSetDevAddr)], drc0;
		wdf		drc0;
		MK_MEM_ACCESS(ldad)	r2, [r4, #DOFFSET(SGXMKIF_HWRTDATASET.ui32MTileWidth)], drc0;
		MK_MEM_ACCESS(ldad)	r3, [r4, #DOFFSET(SGXMKIF_HWRTDATASET.ui32MTileHeight)], drc0;
		wdf		drc0;
		
		shl		r2, r2, #EURASIA_TARNDCLIP_RIGHT_SHIFT;
		and		r2, r2, ~#EURASIA_TARNDCLIP_RIGHT_CLRMSK;
		shl		r3, r3, #EURASIA_TARNDCLIP_BOTTOM_SHIFT;
		and		r3, r3, ~#EURASIA_TARNDCLIP_BOTTOM_CLRMSK;
		or		r2, r2, r3;
	
		/* This task in non-interruptible */
		MK_LOAD_STATE(r0, ui32ITAFlags, drc1);
		MK_WAIT_FOR_STATE_LOAD(drc1);
		or	r0, r0, #(TA_IFLAGS_HALTTA | TA_IFLAGS_TAINPROGRESS);
		MK_STORE_STATE(ui32ITAFlags, r0);
		MK_WAIT_FOR_STATE_STORE(drc1);
	
		/* Set the COMPLETE_ONLY bit annd store the bounding box */
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32VDMCtxTerminateMode)], #VDM_TERMINATE_COMPLETE_ONLY;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32StateTARNDClip)], r2;
		idf	drc0, st;
		wdf	drc0;
	
		/* Issue state terminate task to prepare the scene for rendering */
	
		/* Program the control stream */
		MK_MEM_ACCESS(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sVDMStateUpdateBufferDevAddr)], drc0;
		wdf		drc0;
		str		#EUR_CR_VDM_CTRL_STREAM_BASE >> 2, r2;
	
		/* Start the VDM */
		str		#EUR_CR_VDM_START >> 2, #EUR_CR_VDM_START_PULSE_MASK;
	
		/* Wait for TA finish */
		MK_TRACE(MKTC_TAFINISHED_TERM_COMPLETE_END, MKTF_TA | MKTF_CSW);
		
		ba		TAF_End;
	}
	TAF_CompleteOnTerminateFinish:
	/* Clear the flag */
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32VDMCtxTerminateMode)], #0;
	idf	drc0, st;
	wdf	drc0;
	#endif /* FIX_HW_BRN_33657 */
#endif /* SGX_FEATURE_VDM_CONTEXT_SWITCH */

#if defined(FIX_HW_BRN_31109) && defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH)
	/* Disable proactive PIM spec in case it was enabled during context store */
	READMASTERCONFIG(r0, #EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, drc0);
	wdf		drc0;
	or	r0, r0, #EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC_EN_N_MASK;
	WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, r0, r1);
#endif

	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTACCBCmd)], drc0;
	wdf		drc0;
	
	/* Clear the ready flag, ready for when the CCB wraps */
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r0, #DOFFSET(SGXMKIF_CMDTA.sShared.ui32CtrlFlags)], drc0;
	wdf		drc0;
	and		r2, r2, ~#SGXMKIF_CMDTA_CTRLFLAGS_READY;
	MK_MEM_ACCESS_CACHED(stad)	[r0, #DOFFSET(SGXMKIF_CMDTA.sShared.ui32CtrlFlags)], r2;

	MK_MEM_ACCESS_CACHED(ldad.f2)	r2, [r0, #DOFFSET(SGXMKIF_CMDTA.sShared.ui32TATQSyncReadOpsPendingVal)-1], drc0;
	wdf		drc0;
	and.testz	p0, r3, r3;
	p0	ba	TAF_NoTQSyncReadOp;
	{
		mov	r4, #1;
		iaddu32 r2, r4.low, r2;
		MK_MEM_ACCESS_BPCACHE(stad)	[r3], r2;
		idf		drc0, st;
	}
	TAF_NoTQSyncReadOp:

#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)
	/* update the Src dependencies, first get the count */
	MK_MEM_ACCESS(ldad) r2, [r0, #DOFFSET(SGXMKIF_CMDTA.sShared.ui32NumTASrcSyncs)], drc0;
	wdf		drc0;
	and.testz	p0, r2, r2;
	p0	ba	TAF_NoTASrcReadOpUpdates;
	{
		/* get the base address of the first block + offset to ui32ReadOpsPendingVal */
		mov		r3, #OFFSET(SGXMKIF_CMDTA.sShared.asTASrcSyncs[0].ui32ReadOpsPendingVal);
		iadd32	r3, r3.low, r0;

		/* loop through each dependency, loading a PVRSRV_DEVICE_SYNC_OBJECT structure */
		/* load ui32ReadOpsPendingVal and sReadOpsCompleteDevVAddr */
		MK_MEM_ACCESS(ldad.f2)	r4, [r3, #0++], drc0;
		TAF_UpdateNextTASrcDependency:
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
			!p0	ba	TAF_UpdateNextTASrcDependency;
		}
	}
	TAF_NoTASrcReadOpUpdates:

	/* update the Dst dependencies, first get the count */
	MK_MEM_ACCESS(ldad) r2, [r0, #DOFFSET(SGXMKIF_CMDTA.sShared.ui32NumTADstSyncs)], drc0;
	wdf		drc0;
	and.testz	p0, r2, r2;
	p0	ba	TAF_NoTADstWriteOpUpdates;
	{
		/* get the base address of the first block + offset to ui32WriteOpsPendingVal */
		mov		r3, #OFFSET(SGXMKIF_CMDTA.sShared.asTADstSyncs[0].ui32WriteOpsPendingVal);
		iadd32	r3, r3.low, r0;

		/* loop through each dependency, loading a PVRSRV_DEVICE_SYNC_OBJECT structure */
		/* load ui32WriteOpsPendingVal and sWriteOpsCompleteDevVAddr */
		MK_MEM_ACCESS(ldad.f2)	r4, [r3, #0++], drc0;
		TAF_UpdateNextTADstDependency:
		{
			/* decrement the count and test for 0 */
			isub16.testz r2, p0, r2, #1;
			/* advance to the next block */
			mov	r6, #16;
			iaddu32	r3, r6.low, r3;
			mov	r6, #1;
			wdf	drc0;

			/* Increment ui32SrcWritesOpsPendingVal and copy to sSrcWriteOpsCompleteDevVAddr */
			iaddu32 r4, r6.low, r4;
			MK_MEM_ACCESS_BPCACHE(stad)	[r5], r4;

			!p0	MK_MEM_ACCESS(ldad.f2)	r4, [r3, #0++], drc0;
			!p0	ba	TAF_UpdateNextTADstDependency;
		}
	}
	TAF_NoTADstWriteOpUpdates:
#endif/* #if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS) */

	/* Is a full TA finished so clear the switching variables */
	MK_MEM_ACCESS_BPCACHE(ldad)	r6, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderDetails)], drc0;
	wdf		drc0;

	/* Record the number of pages currently allocated */
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r0, #DOFFSET(SGXMKIF_CMDTA.sHWPBDescDevVAddr)], drc0;
	ldr		r4, #EUR_CR_DPM_PAGE_STATUS >> 2, drc1;
	wdf		drc1;
	and		r4, r4, #EUR_CR_DPM_PAGE_STATUS_TA_MASK;
	shr		r4, r4, #EUR_CR_DPM_PAGE_STATUS_TA_SHIFT;
	wdf		drc0;
	/* write the value out */
	MK_MEM_ACCESS(stad)	[r2, #DOFFSET(SGXMKIF_HWPBDESC.ui32AllocPages)], r4;
	idf		drc0, st;

#if defined(DEBUG)
	MK_MEM_ACCESS(ldad)	r3, [r2, #DOFFSET(SGXMKIF_HWPBDESC.ui32AllocPagesWatermark)], drc0;
	ldr		r4, #EUR_CR_DPM_PAGE_STATUS >> 2, drc1;
	wdf		drc0;
	wdf		drc1;
	and		r4, r4, #EUR_CR_DPM_PAGE_STATUS_TA_MASK;
	shr		r4, r4, #EUR_CR_DPM_PAGE_STATUS_TA_SHIFT;
	/* find out which if this value is higher than the current watermark */
	isub16.tests	p0, r4, r3;
	p0	ba	TAF_NoNewWatermark;
	{
		/* this a new high alloc */
		MK_MEM_ACCESS(stad)	[r2, #DOFFSET(SGXMKIF_HWPBDESC.ui32AllocPagesWatermark)], r4;
		idf 	drc0, st;
	}
	TAF_NoNewWatermark:
#endif

	/*
		FIXME: We need to use the common code in DummyProcessTA rather
		than duplicate it here.
	*/
	/* Write out any status values put into the CCB */
	MK_MEM_ACCESS_CACHED(ldad)	r1, [r0, #DOFFSET(SGXMKIF_CMDTA.sShared.ui32NumTAStatusVals)], drc0;
	wdf		drc0;

	and.testz		p0, r1, r1;
	p0	ba		TAF_NoStatusWrites;
	{
		MK_TRACE(MKTC_TAFINISHED_UPDATESTATUSVALS, MKTF_TA | MKTF_SV);

		mov		r2, #OFFSET(SGXMKIF_CMDTA.sShared.sCtlTAStatusInfo);
		iaddu32		r2, r2.low, r0;
		imae		r3, r1.low, #SIZEOF(CTL_STATUS), r2, u32;

		MK_MEM_ACCESS_CACHED(ldad.f2)	r4, [r2, #0++], drc0;
		TAF_WriteNextStatus:
		{
			wdf		drc0;
			xor.testnz	p0, r2, r3;
			MK_MEM_ACCESS_BPCACHE(stad)	[r4], r5;

			p0	MK_MEM_ACCESS_CACHED(ldad.f2)	r4, [r2, #0++], drc0;
			p0	ba		TAF_WriteNextStatus;
		}
		MK_TRACE(MKTC_TAFINISHED_UPDATESTATUSVALS_DONE, MKTF_TA | MKTF_SV);
	}
	TAF_NoStatusWrites:

	/* Send interrupt to host */
	mov	 	r2, #EUR_CR_EVENT_STATUS_SW_EVENT_MASK;
	str	 	#EUR_CR_EVENT_STATUS >> 2, r2;

	/* Update RTData status if necessary */
	MK_MEM_ACCESS_BPCACHE(ldad)	R_RTData, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], drc0;
	MK_MEM_ACCESS_CACHED(ldad)	r1, [r0, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc1;
	/* Wait for the RTData */
	wdf		drc0;
	MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc0;
	/* Wait for the CMDTA.ui32Flags */
	wdf		drc1;
	MK_MEM_ACCESS_CACHED(ldad)	r7, [r0, #DOFFSET(SGXMKIF_CMDTA.ui32RenderFlags)], drc1;

	and.testz	p0, r1, #SGXMKIF_TAFLAGS_FIRSTKICK | SGXMKIF_TAFLAGS_RESUMEMIDSCENE;
	/* Set p1 to true if LASTKICK is set and NODEALLOC isn't set. */
	/* Wait for the RenderFlags */
	wdf		drc1;
	
	and.testz	p1, r7, #SGXMKIF_RENDERFLAGS_NODEALLOC;
	p1	and.testnz	p1, r1, #SGXMKIF_TAFLAGS_LASTKICK;
	and.testnz	p2, r1, #SGXMKIF_TAFLAGS_LASTKICK;
	/* Wait for the sCommonStatus */
	wdf		drc0;
	p2	or		r2, r2, #SGXMKIF_HWRTDATA_CMN_STATUS_TACOMPLETE;
	p2	MK_MEM_ACCESS(stad)	[R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], r2;

	!p1	ba		TAF_NotLastDealloc;
	{
		/* This is a last kick and the params will be freed */

		/* Clear the RTData the TA is associated with */
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], #0;
		idf		drc0, st;
		
		/* Flush TPC to ensure no writes to deallocated memory are possible */
		mov		r2, #EUR_CR_TE_TPCCONTROL_FLUSH_MASK;
		str		#EUR_CR_TE_TPCCONTROL >> 2, r2;
		
		ENTER_UNMATCHABLE_BLOCK;
		TAF_WaitForTPCFlush:
		{
			ldr		r2, #EUR_CR_EVENT_STATUS >> 2, drc1;
			wdf		drc1;
			shl.testns	p3, r2, #(31 - EUR_CR_EVENT_STATUS_TPC_FLUSH_SHIFT);
			p3	ba		TAF_WaitForTPCFlush;
		}
		LEAVE_UNMATCHABLE_BLOCK;

		mov		r2, #EUR_CR_EVENT_HOST_CLEAR_TPC_FLUSH_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r2;
		
		/* Wait for the write to complete */
		wdf		drc0;
	}
	TAF_NotLastDealloc:

	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderContext)], drc0;
	wdf		drc0;

	p0	ba	TAF_NotFirstKick;
	{
		/* First kick, add render details to list of partial renders for this context */
		MK_MEM_ACCESS(ldad)	r3, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPartialRendersHead)], drc0;
		wdf		drc0;
		MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPartialRendersTail)], drc0;
		wdf		drc0;

		and.testz	p0, r3, r3;
		p0	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPartialRendersHead)], r6;
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPartialRendersTail)], r6;
		!p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sNextDevAddr)], r6;
		!p0	MK_MEM_ACCESS(stad)	[r6, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sPrevDevAddr)], r4;
		idf		drc0, st;
		wdf		drc0;
	}
	TAF_NotFirstKick:

	p1 ba		TAF_LastKick;
	{
		/* Could potentially switch context so save off the EUR_CR_TE_STATE register */
		mov				r3, #OFFSET(SGXMKIF_HWRENDERDETAILS.ui32TEState[0]);
		iaddu32			r2, r3.low, r6;
		COPYCONFIGREGISTERTOMEM_TA(TAF_NotLastKickStoreTEState, EUR_CR_TE_STATE, r2, r3, r4, r5);
		
#if defined(SGX_FEATURE_MP)
	#if defined(FIX_HW_BRN_31079)
		/* If the vdm_pim_wrap is set, save 0 for PIM status */
		shl.tests	p0, r1, #(31 - SGXMKIF_TAFLAGS_VDM_PIM_WRAP_BIT);
		p0	mov	r3, #0;
		p0	ba	TAF_SaveVDMPIMStatus;
	#endif
		{
			READMASTERCONFIG(r3, #EUR_CR_MASTER_VDM_PIM_STATUS >> 2, drc0);
			wdf		drc0; 
		}
		TAF_SaveVDMPIMStatus:
		MK_MEM_ACCESS(stad)	[r6, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32VDMPIMStatus)], r3;			
#endif
		idf		drc0, st;
		wdf		drc0;
		p2	ba		TAF_LastKick;
		{
			/* Increment the complete count for the RTDataSet.
			 * This is necessary to ensure the dummyprocess3D is not called to cleanup RTData
			 * if further TA kicks are queued.
			 */
			MK_MEM_ACCESS(ldad)	r4, [r6, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWRTDataSetDevAddr)], drc0;
			wdf		drc0;
			MK_MEM_ACCESS(ldad)	r3, [r4, #DOFFSET(SGXMKIF_HWRTDATASET.ui32CompleteCount)], drc0;
			wdf		drc0;
			REG_INCREMENT(r3, p0);
			MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWRTDATASET.ui32CompleteCount)], r3;
			idf		drc0, st;
			wdf		drc0;

			/*
				FIXME: Checking for renders and transfer renders after a non-lastkick
				TA finishes may be unnecessary, but appears to be required in the
				presence of BRN_23378.
			*/
			ba		TAF_FindRender;
		}
	}
	TAF_LastKick:

	/* Is norender set? If so free the params */
	and.testz	p0, r7, #SGXMKIF_RENDERFLAGS_NORENDER;
	p0	ba	TAF_NoParamFree;
	{
		MK_TRACE(MKTC_TAFINISHED_NORENDER, MKTF_SCH | MKTF_TA);

		/* save off the registers needed after */
		mov		r7, pclink;
		bal		FreeContextMemory;
		mov		pclink, r7;
	}
	TAF_NoParamFree:
	
#if defined(PDUMP) && defined(EUR_CR_SIM_TA_FRAME_COUNT)
	/* Tell the simulator that this was a last in scene TA terminate */
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32TAFrameNum)], drc0;
	wdf		drc0;
	str		#EUR_CR_SIM_TA_FRAME_COUNT >> 2, r2;
#endif


	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTACCBCmd)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_CACHED(ldad)	r3, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
	mov		r1, #SGXMKIF_TAFLAGS_TA_TASK_OPENCL;
	wdf		drc0;
	and.testz	p0, r3, r1;
	p0		ba KTA_NotOpenCLTaskDelayCountReset;
	{
		/* Load the OpenCL default Delay count */
		MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32OpenCLDelayCount)], #0;
		idf		drc0, st;
		wdf		drc0;
	}
	
	KTA_NotOpenCLTaskDelayCountReset:

	
	MK_TRACE(MKTC_TAFINISHED_LASTKICK, MKTF_TA | MKTF_SCH);

	/* Last kick, remove from partial render list */
	/* Update the lists, move HWRenderContext to r0 */
	MOVESCENETOCOMPLETELIST(r0, r6);

#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH)

#if ! defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH_ALWAYS_SPLIT)
	/* Do not patch the Region Headers if this is the highest priority */
	MK_LOAD_STATE(r3, ui32TACurrentPriority, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testz	p0, r3, r3;
	p0	ba	TAF_HighestPriority;
#endif
	{
		/* Do not patch the region headers if it is a BBox terminate */
		MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTACCBCmd)], drc0;
		wdf		drc0;
		MK_MEM_ACCESS_CACHED(ldad)	r3, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32RenderFlags)], drc0;
#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH_ALWAYS_SPLIT)
		mov		r2, #(SGXMKIF_RENDERFLAGS_BBOX_RENDER | SGXMKIF_RENDERFLAGS_NORENDER);
#else
		/* Check if this render is no MT splitting */
		mov		r2, #(SGXMKIF_RENDERFLAGS_BBOX_RENDER | SGXMKIF_RENDERFLAGS_NORENDER | SGXMKIF_RENDERFLAGS_NO_MT_SPLIT);
#endif
		wdf		drc0;
		and.testnz	p0, r3, r2;

		p0	ba 	TAF_NoRenderSplitting;
		{
			/* Change region header base to point to terminated regions */
			MK_MEM_ACCESS(ldad) 	r1, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32LastMTIdx)], drc0;

			/*
				Addr = (MT * 16 * BlocksPerMT * EURASIA_REGIONHEADER_SIZE ) + base
				       |---loaded from LUT-(per MT)-----------------------|    ` per TA core
			*/
			SWITCHEDMTOTA();
	
			MK_MEM_ACCESS(ldad)	r5, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sLastRgnLUTDevAddr)], drc0;
			mov		r2, #0;
			wdf		drc0;
			TAF_ModifyNextRgnHdr:
			{
				/*
					Load the address of the last region for each macrotile.
				*/
				imae	r4, r2.low, #4, r5, u32;
				MK_MEM_ACCESS_CACHED(ldad)	r4, [r4], drc0;
				wdf		drc0;

				/* check if the valid is set */
				and.testz	p0, r4, #SGXMKIF_HWRTDATA_RGNOFFSET_VALID;
				p0	ba	TAF_MTInvalid;
				{
					/* mask to get the offset into the rgn array*/
					and		r4, r4, #EUR_CR_ISP_RGN_BASE_ADDR_MASK;

#if defined(SGX_FEATURE_MP)

					/*
						Count up to number of TA cores
					 */
					mov		r3, #OFFSET(SGXMKIF_HWRTDATA.sRegionArrayDevAddr);
					iaddu32	r6, r3.low, R_RTData;
					mov 	r3, #0;
					TAF_NextTACore:
					{
						/*
							Get the rgn base
						 */
						MK_MEM_ACCESS_CACHED(ldad)	r7, [r6, #1++], drc0;
						wdf		drc0;

						IADD32(r7, r4, r7, r8);

						MK_MEM_ACCESS_BPCACHE(ldad)	r8, [r7], drc0;
						wdf		drc0;

						or	r8, r8, #EURASIA_REGIONHEADER0_LASTREGION;
						MK_MEM_ACCESS_BPCACHE(stad)	[r7], r8;

						MK_LOAD_STATE_CORE_COUNT_TA(r7, drc0);
						iaddu32		r3, r3.low, #1;
						MK_WAIT_FOR_STATE_LOAD(drc0);
						xor.testnz	p0, r3, r7;
						p0	ba	TAF_NextTACore;
					}
#else
					MK_MEM_ACCESS_CACHED(ldad)	r8, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sRegionArrayDevAddr)], drc0;
					wdf		drc0;
					
					IADD32(r3, r4, r8, r7);

					MK_MEM_ACCESS_BPCACHE(ldad)	r6, [r3], drc0;
					wdf		drc0;

					or	r6, r6, #EURASIA_REGIONHEADER0_LASTREGION;
					MK_MEM_ACCESS_BPCACHE(stad)	[r3], r6;
#endif
				}
				TAF_MTInvalid:

				xor.testnz	p0, r2, r1;
				p0	iaddu32	r2, r2.low, #1;
				p0	ba	TAF_ModifyNextRgnHdr;
			}
			idf		drc0, st;
			wdf		drc0;
			SWITCHEDMBACK();
		}
		TAF_NoRenderSplitting:
	}
	TAF_HighestPriority:
#endif

#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS) && defined(FIX_HW_BRN_23410)
	/* Make sure the TA is always pointing at a valid address space. */
	ldr		r4, #EUR_CR_BIF_BANK0 >> 2, drc1;
	mov		r3, #SGX_BIF_DIR_LIST_INDEX_EDM;
	shl		r3, r3, #EUR_CR_BIF_BANK0_INDEX_TA_SHIFT;
	wdf		drc1;
	and		r4, r4, #~(EUR_CR_BIF_BANK0_INDEX_TA_MASK);
	or		r4, r4, r3;
	SGXMK_SPRV_WRITE_REG(#EUR_CR_BIF_BANK0 >> 2, r4);
#endif /* defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS) */

TAF_FindRender:
#if defined(SGX_FEATURE_2D_HARDWARE)
	FIND2D();
#endif

	FIND3D();

	/* setup r1 for the remaining code */
	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTACCBCmd)], drc0;
	wdf		drc0;

	/* Move on the TACCB */
	bal 	AdvanceTACCBROff;

	/* Remove the current render context from the head of the partial render contexts list */
	MK_LOAD_STATE(r1, ui32TACurrentPriority, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], drc0;
	wdf		drc0;
	and.testz	p0, r4, r4;
	/* Update the Head */
	MK_STORE_STATE_INDEXED(sPartialRenderContextHead, r1, r4, r2);
	/* Update the Tail if required */
	!p0	ba	TAF_NoTailUpdate;
	{
		MK_STORE_STATE_INDEXED(sPartialRenderContextTail, r1, #0, r3);
	}
	TAF_NoTailUpdate:
	!p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], #0;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], #0;
	idf		drc0, st;
	wdf		drc0;

	/* check whether there are any more kicks for current context */
	MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32TACount)], drc0;
	/* also check whether there is any mid scene renders */
	MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPartialRendersHead)], drc0;
	wdf		drc0;
	/* if either is non-zero add it back onto the list */
	or.testz	p1, r2, r4;
	wdf		drc0;

	/*
		If there is something queued or partially completed, then add the current render
		context back to the tail of the partial render context list
	*/
	p1	ba		TAF_NoKicksOutstanding;
	{
		/* Update the Head if required */
		!p0	ba	TAF_KO_NoHeadUpdate;
		{
			MK_STORE_STATE_INDEXED(sPartialRenderContextHead, r1, r0, r2);
		}
		TAF_KO_NoHeadUpdate:
		/* Get the Tail */
		MK_LOAD_STATE_INDEXED(r2, sPartialRenderContextTail, r1, drc0, r3);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		/* Update the Tail */
		MK_STORE_STATE_INDEXED(sPartialRenderContextTail, r1, r0, r3);
		/* Link the context into the list */
		!p0	MK_MEM_ACCESS(stad)	[r2, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], r0;
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], r2;
		idf		drc0, st;
		wdf		drc0;
	}
	TAF_NoKicksOutstanding:

	/* Check for TA commands that can be started */
	FINDTA();

#if defined(SUPPORT_HW_RECOVERY)
	/* check if the 3D had a potential lockup, if so indicate an idle occurred */
	MK_LOAD_STATE(r1, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	mov		r0, #RENDER_IFLAGS_HWLOCKUP_MASK;
	and		r2, r1, r0;
	mov		r0, #RENDER_IFLAGS_HWLOCKUP_DETECTED;
	xor.testnz	p1, r2, r0;
	p1	ba	TAF_No3DLockup;
	{
		mov		r0, #RENDER_IFLAGS_HWLOCKUP_IDLE;
		or		r1, r1, r0;
		MK_STORE_STATE(ui32IRenderFlags, r1);
		MK_WAIT_FOR_STATE_STORE(drc0);
	}
	TAF_No3DLockup:
#endif

TAF_End:
	MK_TRACE(MKTC_TAFINISHED_END, MKTF_TA);

	/* branch back to calling code */
	mov		pclink, R_EDM_PCLink;

	lapc;
}

/*****************************************************************************
 FindTA routine	- check to see if there is a TA command ready to be
			executed

 inputs:
 temps:		r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10
*****************************************************************************/
FindTA:
{
	/* Make sure it's OK to kick the TA now */
	MK_LOAD_STATE(r1, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testnz	p0, r1, #TA_IFLAGS_HALTTA;
	and.testnz	p1, r1, #TA_IFLAGS_TAINPROGRESS;
	shl.tests	p2, r1, #(31 - TA_IFLAGS_DEFERREDCONTEXTFREE_BIT);

	/* If the TA is trying to free a context do not kick anything */
	p2	ba	FTA_End;
	{
		/* If a context switch is in progress wait for it to finish */
		p0	ba		FTA_End;
		{
			/* If we are trying to turn off the chip then don't kick anything */
			MK_LOAD_STATE(r1, ui32HostRequest, drc1);
			MK_WAIT_FOR_STATE_LOAD(drc1);
			and.testz	p2, r1, #SGX_UKERNEL_HOSTREQUEST_POWER;
			p2 ba 		FTA_NoPowerOff;
			{
				MK_TRACE(MKTC_FINDTA_POWERREQUEST, MKTF_TA | MKTF_SCH | MKTF_POW);
				ba			FTA_End;
			}
			FTA_NoPowerOff:

#if defined(FIX_HW_BRN_28889)
			MK_LOAD_STATE(r0, ui32CacheCtrl, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			and.testz	p0, r0, r0;
			p0	ba	FTA_NoCacheRequestPending;
			{
				/* If the TA is active then don't do the Invalidate but we can check the runlists */
				p1	ba 	FTA_NoCacheRequestPending;

				/* There is a request pending so checking if the 3D is running */
				MK_LOAD_STATE(r1, ui32IRenderFlags, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);
				and.testnz		p0, r1, #RENDER_IFLAGS_RENDERINPROGRESS;
				p0	ba	FTA_End;
				/* The 3D must not be running so do the invalidate */
				DOBIFINVAL(FTA_, p0, r0, drc0);
			}
			FTA_NoCacheRequestPending:
#else
	#if defined(USE_SUPPORT_NO_TA3D_OVERLAP)
			/* Don't allow overlapping */
			MK_LOAD_STATE(r1, ui32IRenderFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			and.testz		p0, r1, #RENDER_IFLAGS_RENDERINPROGRESS;
			p0	ba	FTA_NoTA3DOverlap;
			{
				MK_TRACE(MKTC_FINDTA_TA3D_OVERLAP_BLOCKED, MKTF_TA | MKTF_SCH);
				ba	FTA_End;
			}
			FTA_NoTA3DOverlap:
	#endif
#endif
#if defined(SGX_FEATURE_2D_HARDWARE)
	#if defined(FIX_HW_BRN_29900) || defined(USE_SUPPORT_NO_TA2D_OVERLAP)
			/* Don't allow overlapping */
			MK_LOAD_STATE(r1, ui32I2DFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			and.testz		p0, r1, #TWOD_IFLAGS_2DBLITINPROGRESS;
			p0	ba	FTA_NoTA2DOverlap;
			{
				MK_TRACE(MKTC_FINDTA_TA2D_OVERLAP_BLOCKED, MKTF_TA | MKTF_SCH);
				ba	FTA_End;
			}
			FTA_NoTA2DOverlap:
	#endif
#endif

			/* Look for a CCB command */
			/* Start at priority level 0 (highest) */
			mov		r8, #0;
			/* Get the first head */
			MK_LOAD_STATE(r0, sPartialRenderContextHead[0], drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);

			/* If TA is currently active so skip the first context in the list,
				i.e. the one the current TA kick belongs to */
			!p1	ba		FTA_CheckNextContext;
			{
				FTA_CheckActiveContextPriority:
				MK_LOAD_STATE(r1, ui32TACurrentPriority, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);
				/* only proceed if the run-list priority is higher than the current context */
				isub16.testns	p0, r8, r1;
				p0 ba FTA_End;
				/* Run-list is a higher priority, start checking it */
			}
			FTA_CheckNextContext:
			and.testz	p0, r0, r0;
			!p0	ba		FTA_CheckContext;
			{
				/* Make sure we only check priorities >= the highest blocked context, 
					if nothing is blocked ui32BlockedPriority is the lowest level */
				iaddu32	r8, r8.low, #1;
				MK_LOAD_STATE(r0, ui32BlockedPriority, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);
				isub16.tests	p0, r0, r8;
				p0	ba	FTA_AllPrioritiesChecked;
				{
					MK_LOAD_STATE_INDEXED(r0, sPartialRenderContextHead, r8, drc0, r1);
					MK_WAIT_FOR_STATE_LOAD(drc0);
					/* If the TA is active check against the current priority */
					p1	ba	FTA_CheckActiveContextPriority;
					/* If not active start searching for a TA to kick */
					ba	FTA_CheckNextContext;
				}
				FTA_AllPrioritiesChecked:

				/* If the TA is active don't set the active power flag */
				p1	ba	FTA_End;

				/*
					Indicate to the Active Power Manager that no TAs are ready to go.
				*/
				CLEARACTIVEPOWERFLAGS(#SGX_UKERNEL_APFLAGS_TA);
				ba			FTA_End;
			}
			FTA_CheckContext:
			{
				/*
					Check whether this context actually has any kicks waiting in
					the CCB, and that it is not suspended.
				*/
				MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32TACount)], drc0;
				MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sCommon.ui32Flags)], drc0;
				wdf		drc0;
				and.testz	p0, r1, r1;
				/* If this context does NOT have any kicks oustanding, move on to the next one */
				p0	ba		FTA_NextContext;

				and.testnz p0, r2, #SGXMKIF_HWCFLAGS_SUSPEND;
				!p0 ba FTA_ContextNotSuspended;
				{
					MK_TRACE(MKTC_FINDTA_CONTEXT_SUSPENDED, MKTF_TA | MKTF_SCH | MKTF_RC);
					MK_TRACE_REG(r0, MKTF_TA | MKTF_RC);
					ba		FTA_NextContext;
				}
				FTA_ContextNotSuspended:
	
				/* Calculate base of TA CCB command */
				MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sTACCBCtlDevAddr)], drc0;
				wdf		drc0;
				MK_MEM_ACCESS_BPCACHE(ldad)	r1, [r1, #DOFFSET(PVRSRV_SGX_CCB_CTL.ui32ReadOffset)], drc0;
				MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sTACCBBaseDevAddr)], drc1;
				wdf		drc0;
				wdf		drc1;
				iaddu32	r1, r1.low, r2;
				
				/* Load the Ctrl flags, is the command ready to be processed? */
				MK_MEM_ACCESS(ldad)	r9, [r1, #DOFFSET(SGXMKIF_CMDTA.sShared.ui32CtrlFlags)], drc0;
				wdf		drc0;
				and.testz	p0, r9, #SGXMKIF_CMDTA_CTRLFLAGS_READY;
				p0	br	FTA_NextContext;

				MK_MEM_ACCESS_CACHED(ldad)	R_RTData, [r1, #DOFFSET(SGXMKIF_CMDTA.sHWRTDataDevAddr)], drc0;
				wdf		drc0;

				/* Load the flags */
				MK_MEM_ACCESS_CACHED(ldad)	r9, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
				wdf		drc0;

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
				/* Check if kick partially done */
				mov	r2, #SGXMKIF_TAFLAGS_VDM_CTX_SWITCH;
				and.testnz	p0, r9, r2;
				p0	ba		FTA_FoundTA;
#endif
				{
					MK_MEM_ACCESS_CACHED(ldad.f4)	r2, [r1, #DOFFSET(SGXMKIF_CMDTA.sShared.ui32TATQSyncWriteOpsPendingVal)-1], drc0;
					wdf		drc0;
					and.testz	p0, r3, r3;
					p0	ba	FTA_TQSyncOpReady;
					{
						/* Check this is ready to go */
						MK_MEM_ACCESS_BPCACHE(ldad)	r6, [r3], drc0; // load WriteOpsComplete;
						MK_MEM_ACCESS_BPCACHE(ldad)	r7, [r5], drc0; // load ReadOpsComplete;
						wdf		drc0;
						xor.testz	p0, r6, r2;	// Test WriteOpsComplete;
						p0 xor.testz	p0, r7, r4; // Test ReadOpsComplete;
						!p0	ba		FTA_NextContext;
					}
					FTA_TQSyncOpReady:

					/* check if this TA is dependent on a render finishing */
					mov	r2, #SGXMKIF_TAFLAGS_DEPENDENT_TA;
					wdf	drc0;
					and.testz	p0, r9, r2;
					p0	ba	FTA_TADependencyReady;
					{
						/* load the WriteOpsComplete and check against pending */
						MK_MEM_ACCESS_CACHED(ldad.f2)	r4, [r1, #DOFFSET(SGXMKIF_CMDTA.sShared.sTA3DDependency.ui32WriteOpsPendingVal)-1], drc0;
						wdf		drc0;
						MK_MEM_ACCESS_BPCACHE(ldad)	r6, [r5], drc0;
						wdf		drc0;
						xor.testz	p0, r6, r4;
						!p0	ba		FTA_NextContext;
					}
					FTA_TADependencyReady:

#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)
					/* check the TA SRC dependencies, first get the count */
					MK_MEM_ACCESS(ldad) r2, [r4, #DOFFSET(SGXMKIF_CMDTA.sShared.ui32NumTASrcSyncs)], drc0;
					wdf		drc0;
					and.testz	p1, r2, r2;
					p1	ba	FTA_NoTASrcSyncs;
					{			
						/* get the base address of the first block + offset to ui32ReadOpsPendingVal */
						mov		r5, #OFFSET(SGXMKIF_CMDTA.sShared.asTASrcSyncs[0].ui32ReadOpsPendingVal);
						iadd32	r5, r5.low, r4;
			
						/* loop through each dependency, loading a PVRSRV_DEVICE_SYNC_OBJECT structure */
						/* load ui32ReadOpsPendingVal and sReadOpsCompleteDevVAddr */
						MK_MEM_ACCESS(ldad.f2)	r6, [r5, #0++], drc0;
						FTA_CheckNextTASrcDependency:
						{
							wdf	drc0;
			
							/* Check this is ready to go */
							MK_MEM_ACCESS_BPCACHE(ldad)	r10, [r7], drc0; // load ReadOpsComplete
							wdf		drc0;
							xor.testz	p1, r10, r6;	// Test ReadOpsComplete
							!p0	ba		FTA_NextContext;
			
							MK_MEM_ACCESS(ldad.f2)	r6, [r5, #0++], drc0;
							wdf	drc0;

							MK_MEM_ACCESS_BPCACHE(ldad)	r10, [r7], drc0;	// load WriteOpsComplete
							wdf		drc0;
							xor.testz	p1, r10, r6; 	// Test WriteOpsComplete
							!p0	ba		FTA_NextContext;
			
							/* decrement the count and test for 0 */
							isub16.testz r2, p1, r2, #1;
							!p1	MK_MEM_ACCESS(ldad.f2)	r6, [r5, #0++], drc0;
							!p1	ba	FTA_CheckNextTASrcDependency;
						}
					}
					FTA_NoTASrcSyncs:


					/* check the TA DST dependencies, first get the count */
					MK_MEM_ACCESS(ldad) r2, [r4, #DOFFSET(SGXMKIF_CMDTA.sShared.ui32NumTADstSyncs)], drc0;
					wdf		drc0;
					and.testz	p1, r2, r2;
					p1	ba	FTA_NoTADstSyncs;
					{			
						/* get the base address of the first block + offset to ui32ReadOpsPendingVal */
						mov		r5, #OFFSET(SGXMKIF_CMDTA.sShared.asTADstSyncs[0].ui32ReadOpsPendingVal);
						iadd32	r5, r5.low, r4;
			
						/* loop through each dependency, loading a PVRSRV_DEVICE_SYNC_OBJECT structure */
						/* load ui32ReadOpsPendingVal and sReadOpsCompleteDevVAddr */
						MK_MEM_ACCESS(ldad.f2)	r6, [r5, #0++], drc0;
						FTA_CheckNextTADstDependency:
						{
							wdf	drc0;
			
							/* Check this is ready to go */
							MK_MEM_ACCESS_BPCACHE(ldad)	r10, [r7], drc0; // load ReadOpsComplete
							wdf		drc0;
							xor.testz	p1, r10, r6;	// Test ReadOpsComplete
							!p0	ba		FTA_NextContext;
			
							MK_MEM_ACCESS(ldad.f2)	r6, [r5, #0++], drc0;
							wdf	drc0;
	
							MK_MEM_ACCESS_BPCACHE(ldad)	r10, [r7], drc0;	// load WriteOpsComplete
							wdf		drc0;
							xor.testz	p1, r10, r6; 	// Test WriteOpsComplete
							!p0	ba		FTA_NextContext;
			
							/* decrement the count and test for 0 */
							isub16.testz r2, p1, r2, #1;
							!p1	MK_MEM_ACCESS(ldad.f2)	r6, [r5, #0++], drc0;
							!p1	ba	FTA_CheckNextTADstDependency;
						}
					}
					FTA_NoTADstSyncs:
#endif/* #if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS) */

	#if defined(FORCE_PRE_TA_SYNCOP_CHECK)
					/* Force the checking of the sync objects */
					mov		r10, #USE_TRUE;
	#else
					/*
						When we have multiple applications and possible SPM events we have to ensure
						that all sync objects have been flushed before starting a TA kick. This is 
						to ensure that should we hit SPM that the partial render can take place.

						if ( (There exists a context other than this one in a partial list)
						#if (defined(FIX_HW_BRN_25910) && defined(SUPPORT_PERCONTEXT_PB)) || !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
							||  (There exists a context other than this one in a complete list)
						#endif
							)
							{
								if ((DstSync->ui32ReadOpsPendingVal != *(DstSync->sReadOpsCompleteDevVAddr)
									||	(DstSync->ui32WriteOpsPendingVal != *(DstSync->sWriteOpsCompleteDevVAddr))
								{
									 // TA cannot be started (see SW BRN 26300), try another context
								}
							}
					*/
					
					/* r4 = render context priority, loops from 0 to SGX_MAX_CONTEXT_PRIORITY */
					mov		r10, #USE_FALSE;
					mov		r4, #0;
					FTA_SearchPartialList_Start:
					{
						MK_LOAD_STATE_INDEXED(r2, sPartialRenderContextHead, r4, drc0, r3);
						MK_WAIT_FOR_STATE_LOAD(drc0);
						
						FTA_SearchPartialList_NextCtx:
						{
							or.testz p0, r2, r2;
							p0 ba FTA_SearchPartialList_NextPriority;
							
							/* if it does not match the context being kicked, do sync check */
							xor.testnz	p0, r2, r0;
							p0	mov	r10, #USE_TRUE;
							p0 ba FTA_ContextCheckDone;
							
							/* move onto the next context at this priority level */
							MK_MEM_ACCESS(ldad) r2, [r2, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], drc0;
							wdf	 drc0;
							ba FTA_SearchPartialList_NextCtx;
						}
						FTA_SearchPartialList_NextPriority:
						
						/* Move onto next priority */
						iaddu32	r4, r4.low, #1;
						xor.testz	p0, r4, #SGX_MAX_CONTEXT_PRIORITY;
						!p0 ba FTA_SearchPartialList_Start;
					}

		#if (defined(FIX_HW_BRN_25910) && defined(SUPPORT_PERCONTEXT_PB)) || !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
					/*
						If there are no other render contexts in the
						complete lists then it's safe to go ahead with the
						TA without satisfying the sync objects.
					*/
					
					/* r4 = render context priority, loops from 0 to SGX_MAX_CONTEXT_PRIORITY */
					mov		r4, #0;
					FTA_SearchCompleteList_Start:
					{
						MK_LOAD_STATE_INDEXED(r2, sCompleteRenderContextHead, r4, drc0, r3);
						MK_WAIT_FOR_STATE_LOAD(drc0);
						
						FTA_SearchCompleteList_NextCtx:
						{
							or.testz p0, r2, r2;
							p0 ba FTA_SearchCompleteList_NextPriority;
						
							/* if it does not match the context being kicked, do sync check */
							xor.testnz	p0, r2, r0;
							p0 mov	r10, #USE_TRUE;
							p0 ba FTA_ContextCheckDone;
							
							/* move onto the next context */
							MK_MEM_ACCESS(ldad) r2, [r2, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextCompleteDevAddr)], drc0;
							wdf	 drc0;
							ba FTA_SearchCompleteList_NextCtx;
						}
						FTA_SearchCompleteList_NextPriority:
						
						/* Move onto next priority */
						iaddu32	r4, r4.low, #1;
						xor.testz	p0, r4, #SGX_MAX_CONTEXT_PRIORITY;
						!p0 ba FTA_SearchCompleteList_Start;
					}
		#endif
					FTA_ContextCheckDone:
	#endif /* defined(FORCE_PRE_TA_SYNCOP_CHECK) */
										
					/* Check if first kick after a render */
					mov		r2, #(SGXMKIF_TAFLAGS_FIRSTKICK | SGXMKIF_TAFLAGS_RESUMEMIDSCENE);
					and.testz	p0, r9, r2;
					p0	ba		FTA_NotFirstResumeKick;
					{
						/* Check status to ensure the last render has completed */
						MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc0;
						wdf		drc0;
						and.testnz	p0, r2, #SGXMKIF_HWRTDATA_CMN_STATUS_READY;
						p0	ba		FTA_RTDataReady;
						{
							MK_TRACE(MKTC_FINDTA_RTDATA_RENDERING, MKTF_TA | MKTF_SCH | MKTF_RT);
							MK_TRACE_REG(R_RTData, MKTF_TA | MKTF_RT);
							FTA_NextContext:
							MK_MEM_ACCESS(ldad)	r0, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], drc0;
							wdf		drc0;
							ba	FTA_CheckNextContext;
						}
						FTA_RTDataReady:
						
						/* check whether we have found more than one context. */
						xor.testz	p0, r10, #USE_FALSE;
						p0	ba	FTA_FoundTA;
						
						/* This is not the only context so we must check the dst sync before starting the TA */
						MK_MEM_ACCESS_BPCACHE(ldad)	r2, [r1, #DOFFSET(SGXMKIF_CMDTA.sHWDstSyncListDevAddr)], drc0;
						wdf		drc0;
						/* Do we have a dst sync to check? */
						and.testz	p0, r2, r2;
						p0	ba		FTA_NoDstSync;
						{
							/* Load the dst sync values */
							MK_MEM_ACCESS(ldad.f4)	r3, [r2, #DOFFSET(SGXMKIF_HWDEVICE_SYNC_LIST.asSyncData[0].ui32ReadOpsPendingVal)-1], drc0;
							wdf		drc0;

							/* Check read ops */
							MK_MEM_ACCESS_BPCACHE(ldad)	r7, [r4], drc0;
							wdf		drc0;
							xor.testz	p0, r7, r3;
							p0	ba		FTA_ReadOpsNotBlocked;
							{
								MK_TRACE(MKTC_FINDTA_READOPSBLOCKED, MKTF_TA | MKTF_SO | MKTF_SCH);
								MK_TRACE_REG(r4, MKTF_TA | MKTF_SO);
								MK_TRACE_REG(r7, MKTF_TA | MKTF_SO);
								MK_TRACE_REG(r3, MKTF_TA | MKTF_SO);
								
								/* This context has been blocked by oustanding ops, look for another */
								ba		FTA_NextContext;
							}
							FTA_ReadOpsNotBlocked:

							/* Check write ops */
							MK_MEM_ACCESS_BPCACHE(ldad)	r7, [r6], drc0;
							wdf		drc0;
							xor.testz	p0, r7, r5;
							p0	ba		FTA_WriteOpsNotBlocked;
							{
								MK_TRACE(MKTC_FINDTA_WRITEOPSBLOCKED, MKTF_TA | MKTF_SO | MKTF_SCH);
								MK_TRACE_REG(r6, MKTF_TA | MKTF_SO);
								MK_TRACE_REG(r7, MKTF_TA | MKTF_SO);
								MK_TRACE_REG(r5, MKTF_TA | MKTF_SO);
								
								/* This context has been blocked by oustanding ops, look for another */
								ba		FTA_NextContext;
							}
							FTA_WriteOpsNotBlocked:

							/* Load the read2 sync values */
							MK_MEM_ACCESS(ldad.f2)	r3, [r2, #DOFFSET(SGXMKIF_HWDEVICE_SYNC_LIST.asSyncData[0].ui32ReadOps2PendingVal)-1], drc0;
							wdf		drc0;

							/* Check read2 ops */
							MK_MEM_ACCESS_BPCACHE(ldad)	r7, [r4], drc0;
							wdf		drc0;
							xor.testz	p0, r7, r3;
							p0	ba		FTA_ReadOps2NotBlocked;
							{
								MK_TRACE(MKTC_FINDTA_READOPS2BLOCKED, MKTF_TA | MKTF_SO | MKTF_SCH);
								MK_TRACE_REG(r4, MKTF_TA | MKTF_SO);
								MK_TRACE_REG(r7, MKTF_TA | MKTF_SO);
								MK_TRACE_REG(r3, MKTF_TA | MKTF_SO);
								
								/* This context has been blocked by oustanding ops, look for another */
								ba		FTA_NextContext;
							}
							FTA_ReadOps2NotBlocked:
						}
						FTA_NoDstSync:
					}
					FTA_NotFirstResumeKick:
					
					/* check whether we have found more than one context. */
					xor.testz	p0, r10, #USE_FALSE;
					p0	ba	FTA_FoundTA;
					{
						/* find out the number of src syncs*/
						MK_MEM_ACCESS_BPCACHE(ldad)	r2, [r1, #DOFFSET(SGXMKIF_CMDTA.sShared.ui32NumSrcSyncs)], drc0;
						wdf		drc0;
						and.testz	p0, r2, r2;
						p0 br		FTA_FoundTA;
						{
							/* work out the address of where the src syncs start */
							mov		r3, #OFFSET(SGXMKIF_CMDTA.sShared.asSrcSyncs[0]);
							iaddu32	r3, r3.low, r1;

							FTA_ForAllSrcSyncOps:
							{
								/* asSrcSyncs[i].ui32ReadOpPending */
								MK_MEM_ACCESS_BPCACHE(ldad.f2)	r4, [r3, #0++], drc0;
								wdf		drc0;
								and.testz	p0, r5, r5;
								p0 ba		FTA_NoSrcReadOpAddr;
								{
									MK_MEM_ACCESS_BPCACHE(ldad)	r6, [r5], drc0;
									wdf		drc0;
									xor.testz	p0, r6, r4;
									p0 ba		FTA_SrcReadsReady;
									{
										MK_TRACE(MKTC_FINDTA_SRC_READOPSBLOCKED, MKTF_TA | MKTF_SO | MKTF_SCH);
										MK_TRACE_REG(r5, MKTF_TA | MKTF_SO);
										MK_TRACE_REG(r6, MKTF_TA | MKTF_SO);
										MK_TRACE_REG(r4, MKTF_TA | MKTF_SO);
										
										/* This context has been blocked by oustanding ops, look for another */
										ba	FTA_NextContext;
									}
									FTA_SrcReadsReady:
								}
								FTA_NoSrcReadOpAddr:

								/* ui32WriteOpsPending Val and Completion Dev V Addr, previous #0++, would have got
								   r3 incremented already to where we want it to be (pointing to ui32WriteOpsPendingVal) */	
								MK_MEM_ACCESS_BPCACHE(ldad.f2)	r4, [r3, #0++], drc0;
								wdf		drc0;
								and.testz	p0, r5, r5;
								p0 ba		FTA_NoSrcWriteOpAddr;
								{
									MK_MEM_ACCESS_BPCACHE(ldad)	r6, [r5], drc0;
									wdf		drc0;
									xor.testz	p0, r6, r4;
									p0	ba		FTA_SrcWritesReady;
									{
										MK_TRACE(MKTC_FINDTA_SRC_WRITEOPSBLOCKED, MKTF_TA | MKTF_SO | MKTF_SCH);
										MK_TRACE_REG(r5, MKTF_TA | MKTF_SO);
										MK_TRACE_REG(r6, MKTF_TA | MKTF_SO);
										MK_TRACE_REG(r4, MKTF_TA | MKTF_SO);
										
										/* This context has been blocked by oustanding ops, look for another */
										ba	FTA_NextContext;
									}
									FTA_SrcWritesReady:
								}
								FTA_NoSrcWriteOpAddr:

								/* Skip over ui32ReadOps2PendingVal/sReadOps2CompleteDevVAddr for next time round the loop */
								mov				r6, #SIZEOF(PVRSRV_DEVICE_SYNC_OBJECT.ui32ReadOps2PendingVal) + SIZEOF(PVRSRV_DEVICE_SYNC_OBJECT.sReadOps2CompleteDevVAddr);
								iaddu32			r3, r6.low, r3;

								/* decrement SrcSync count in r2 and test for zero */
								isub16.testnz	r2, p0, r2, #1;
								p0 ba			FTA_ForAllSrcSyncOps;
							}
						}
					}
				}
			}
			FTA_FoundTA:

#if (defined(FIX_HW_BRN_25910) && defined(SUPPORT_PERCONTEXT_PB)) || !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
			/* make sure we only parallelise a single context */
			/* check whether there is a render in progress */
			MK_LOAD_STATE(r2, ui32IRenderFlags, drc0);
	#if defined(SGX_FEATURE_2D_HARDWARE)
			MK_LOAD_STATE(r3, ui32I2DFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			/* All modules use the same bit to indicate 'busy' */
			or	r2, r2, r3;
	#else
			MK_WAIT_FOR_STATE_LOAD(drc0);
	#endif
			and.testz	p0, r2, #RENDER_IFLAGS_RENDERINPROGRESS;
			p0	ba	FTA_NoRenderInProgress;
			{
				/* Now load up the PDPhys and DirListBase0 */
#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
				ldr			r3, #EUR_CR_BIF_BANK0 >> 2, drc0;
				wdf			drc0;
				and			r3, r3, #EUR_CR_BIF_BANK0_INDEX_3D_MASK;
				shr			r3, r3, #EUR_CR_BIF_BANK0_INDEX_3D_SHIFT;
				and.testz	p0, r3, r3;
				p0	mov 	r3, #(EUR_CR_BIF_DIR_LIST_BASE0 >> 2);
				!p0	mov		r2, #((EUR_CR_BIF_DIR_LIST_BASE1 >> 2)-1);
				!p0	iaddu32	r3, r3.low, r2;
				ldr			r3, r3, drc0;
#else
				ldr		r3, #EUR_CR_BIF_DIR_LIST_BASE0 >> 2, drc0;
#endif /* SGX_FEATURE_MULTIPLE_MEM_CONTEXTS */
				/* note: the wdf follows in SGXMK_HWCONTEXT_GETDEVPADDR */
				SGXMK_HWCONTEXT_GETDEVPADDR(r2, r0, r4, drc0);

				xor.testnz	p0, r2, r3;
				!p0	ba	FTA_SameContext;
				{
					MK_TRACE(MKTC_FINDTA_3DRC_DIFFERENT, MKTF_TA | MKTF_SCH);

					FTA_CheckPriority:
					/* record the blocked priority, so nothing of lower priority can be started.
						NOTE: The code guarantees the priority is >= to current blocked level. */
					MK_STORE_STATE(ui32BlockedPriority, r8);
					MK_WAIT_FOR_STATE_STORE(drc0);
				
					
	#if defined(SGX_FEATURE_2D_HARDWARE)
					/* if the 3D is not busy, the 2D must be! 
						Given there is nothing we can do to stop it, just exit */
					MK_LOAD_STATE(r4, ui32IRenderFlags, drc0);
					MK_WAIT_FOR_STATE_LOAD(drc0);
					and.testz	p0, r4, #RENDER_IFLAGS_RENDERINPROGRESS;
					p0	ba	FTA_End;
	#endif
					/* The memory contexts are different so only proceed if the TA is of a higher priority */
					MK_LOAD_STATE(r3, ui323DCurrentPriority, drc0);
					MK_WAIT_FOR_STATE_LOAD(drc0);
					isub16.testns	p0, r8, r3;
					p0 ba FTA_End;

					/* The TA is higher priority so request the 3D to stop */
					START3DCONTEXTSWITCH(FTA);

					ba	FTA_End;
				}
				FTA_SameContext:
				/* The contexts are the same so we can continue */

				#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)

				/*
					It is not possible to proceed if the current TA PB is not the same as the new one,
					since it would require changing memory context in order to save the PB state.
				*/
				MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc.uiAddr)], drc0;
				MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPBDescDevVAddr.uiAddr)], drc0;
				wdf	drc0;
				and.testz		p0, r3, r3;
				!p0 xor.testz	p0, r3, r4;
				p0	ba FTA_TAPBOK;
				{
					MK_TRACE(MKTC_FINDTA_TAPB_DIFFERENT, MKTF_TA | MKTF_SCH);
					ba FTA_CheckPriority;
				}
				FTA_TAPBOK:


#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
				/*
					It is not possible to proceed if the current TA context is not the same as the new one,
					since it would require changing memory context in order to save the context.
				*/
				MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], drc0;
				wdf	drc0;
				and.testz		p0, r3, r3;
				!p0 xor.testz	p0, r3, R_RTData;
				p0	ba FTA_TAContextOK;
				{
					/*
					3D does context check only if TA-inprogress. This
					means 3D can start on a diff context, and TA can
					start a diff context for TA as well, since it matches
					3D, but STORETACONTEXT, cant store into correct context

					if (no-render-in-progress) ba FTA_TAContextOK
					if (sTARenderContext == newContext) ba FTA_TAContextOK
					*/
				
					MK_LOAD_STATE(r3, ui32IRenderFlags, drc0);
					MK_WAIT_FOR_STATE_LOAD(drc0);
					and.testnz	p0, r3, #RENDER_IFLAGS_RENDERINPROGRESS;
					!p0	ba	FTA_TAContextOK;

					MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderContext)], drc0;
					wdf		drc0;
					xor.testz p0, r3, r0;
					p0	ba	FTA_TAContextOK;

					MK_TRACE(MKTC_FINDTA_TACONTEXT_DIFFERENT, MKTF_TA | MKTF_SCH);
					ba FTA_CheckPriority;
				}
				FTA_TAContextOK:
#endif

				/* is the context already loaded */
				/* retreive the HWRTData */
				MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext0RTData)], drc0;
				MK_MEM_ACCESS_BPCACHE(ldad)	r4, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext1RTData)], drc1;
				wdf				drc0;
				xor.testz	p0, R_RTData, r3;
				wdf		drc1;
				!p0 xor.testz	p0, R_RTData, r4;
				/* if it is not loaded, is there one free - FIXME using 'and' in this way is not completely safe */
				!p0 and.testz	p0, r3, r4;
				p0	ba FTA_RTDataLoaded;
				{
					/* It is not loaded and there is not a free context,
						so check if the one not in use is on the same address space
						and can be stored safely */
					ldr		r2, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc0;
					wdf		drc0;
					and		r2, r2, #EUR_CR_DPM_STATE_CONTEXT_ID_DALLOC_MASK;
					and.testnz	p0, r2, r2;
					p0	MK_MEM_ACCESS(ldad)	r3, [r3, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderContextDevAddr)], drc0;
					!p0	MK_MEM_ACCESS(ldad)	r3, [r4, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderContextDevAddr)], drc0;
					wdf	drc0;
					ldr		r4, #EUR_CR_BIF_DIR_LIST_BASE0 >> 2, drc0;
					/* note: the wdf follows in SGXMK_HWCONTEXT_GETDEVPADDR */
					SGXMK_HWCONTEXT_GETDEVPADDR(r5, r3, r3, drc0);

					xor.testnz	p0, r5, r4;
					p0 ba	FTA_CheckPriority;
				}
				FTA_RTDataLoaded:
				#endif
			}
			FTA_NoRenderInProgress:
#endif /* (defined(FIX_HW_BRN_25910) || !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)) */
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
			/* if this command has been context switched the update has already been done so skip it */
			mov	 r2, #SGXMKIF_TAFLAGS_VDM_CTX_SWITCH;
			and.testnz	p0, r9, r2;
			p0	ba	FTA_NoPBUpdate;
#endif
			mov		r2, #SGXMKIF_TAFLAGS_PB_UPDATE_MASK;
			and.testz	p0, r9, r2;
			p0	br	 FTA_NoPBUpdate;
			{
				/* check if there are any new DPM threshold values */
				mov		r2, #SGXMKIF_TAFLAGS_PB_THRESHOLD_UPDATE;
				and.testz	p0, r9, r2;
				p0	ba	 FTA_NoPBThresholdUpdate;
				{
					/* We have some new values to update, only continue if:
						1) the 3D is idle
						OR
						2) TODO: 3D is on a different PB
					*/
					MK_LOAD_STATE(r2, ui32IRenderFlags, drc0);
					MK_WAIT_FOR_STATE_LOAD(drc0);
					and.testnz	p0, r2, #RENDER_IFLAGS_RENDERINPROGRESS;
					p0	ba		FTA_End;

					MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPBDescDevVAddr.uiAddr)], drc1;
					MK_MEM_ACCESS_CACHED(ldad.f4)	r3, [r1, #DOFFSET(SGXMKIF_CMDTA.sHWPBDescUpdate.ui32TAThreshold)-1], drc1;
					wdf		drc1;
					MK_MEM_ACCESS(stad.f4)	[r2, #DOFFSET(SGXMKIF_HWPBDESC.ui32TAThreshold)-1], r3;
					
					MK_MEM_ACCESS(ldad)	r3, [r2, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], drc0;
					wdf		drc0;
					or		r3, r3, #HWPBDESC_FLAGS_THRESHOLD_UPDATE;
					MK_MEM_ACCESS(stad)	[r2, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], r3;
					idf		drc0, st;
					wdf		drc0;

					/* We are able to do the update at this point so continue */
					br	FTA_NoPBUpdate;
				}
				FTA_NoPBThresholdUpdate:

#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
				/* Check if there are any new blocks to add */
				mov		r2, #(SGXMKIF_TAFLAGS_PB_GROW | SGXMKIF_TAFLAGS_PB_SHRINK);
				and.testz	p0, r9, r2;
				p0	ba	 FTA_NoPBResize;
				{
					mov		r2, #SGXMKIF_TAFLAGS_PB_SHRINK;
					and.testz	p0, r9, r2;
					p0	ba	FTA_NotPBShrink;
					{
						/* Make sure the PB is not currently in use */
						MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWCompleteRendersHead)], drc0;
						wdf		drc0;
						
						/* If there are still renders to flush just exit */
						and.testz	p2, r2, r2;
						p2	ba	FTA_PBNotInUse;
						{
							MK_TRACE(MKTC_FINDTA_RESIZE_PB_BLOCKED, MKTF_TA | MKTF_SCH | MKTF_PB);
							
							ba	FTA_End;
						}
						FTA_PBNotInUse:
					}
					FTA_NotPBShrink:

					MK_TRACE(MKTC_FINDTA_RESIZE_PB, MKTF_TA | MKTF_SCH | MKTF_PB);

					MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPBDescDevVAddr.uiAddr)], drc1;

					/* Copy the new values over and set the init flag again */
					MK_MEM_ACCESS_CACHED(ldad.f4)	r3, [r1, #DOFFSET(SGXMKIF_CMDTA.sHWPBDescUpdate.ui32TAThreshold)-1], drc1;
					wdf		drc1;
					MK_MEM_ACCESS(stad.f4)	[r2, #DOFFSET(SGXMKIF_HWPBDESC.ui32TAThreshold)-1], r3;

					MK_MEM_ACCESS_CACHED(ldad.f3)	r3, [r1, #DOFFSET(SGXMKIF_CMDTA.sHWPBDescUpdate.ui32NumPages)-1], drc0;
					wdf		drc0;
					MK_MEM_ACCESS(stad.f3)	[r2, #DOFFSET(SGXMKIF_HWPBDESC.ui32NumPages)-1], r3;

					/* Added blocks to the link list */
					MK_MEM_ACCESS_CACHED(ldad.f2)	r3, [r1, #DOFFSET(SGXMKIF_CMDTA.sHWPBDescUpdate.sPBBlockPtrUpdateDevAddr)-1], drc0;
					wdf		drc0;
					
					/* Update the block list */
					MK_MEM_ACCESS(stad)	[r3, #DOFFSET(SGXMKIF_HWPBBLOCK.sNextHWPBBlockDevVAddr)], r4;
					
					/* Are we doing a grow or shrink? */
					p0	ba	 FTA_PBGrow;
					{
						MK_TRACE(MKTC_FINDTA_SHRINK_PB, MKTF_TA | MKTF_SCH | MKTF_PB);
						
						MK_MEM_ACCESS(stad)	[r2, #DOFFSET(SGXMKIF_HWPBDESC.ui32ShrinkComplete)], #USE_TRUE;
						MK_MEM_ACCESS(stad)	[r2, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], #HWPBDESC_FLAGS_INITPT;
						
						/* If the PB is already load clear the values, so it is reloaded with the new values */
						/* get the current TA HWPBDesc */
						MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc.uiAddr)], drc0;
						/* get the current 3D HWPBDesc */
						MK_MEM_ACCESS_BPCACHE(ldad)	r4, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DHWPBDesc.uiAddr)], drc0;
						wdf		drc0;
						
						/* Check if it is the same as the TA PB */
						xor.testz	p0, r2, r3;
						p0	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc.uiAddr)], #0;
						/* Check if it is the same as the 3D PB */
						xor.testz	p0, r2, r4;
						p0	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DHWPBDesc.uiAddr)], #0;
						
						ba	FTA_FlushWrites;
					}
					FTA_PBGrow:
					{
						/* Save pointer, so we know where Grow init needs to start */
						MK_MEM_ACCESS(stad)	[r2, #DOFFSET(SGXMKIF_HWPBDESC.sGrowListPBBlockDevVAddr)], r4;
						MK_MEM_ACCESS(stad)	[r2, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], #HWPBDESC_FLAGS_GROW;	
					}
					FTA_FlushWrites:
					idf		drc0, st;
					wdf		drc0;
				}
				FTA_NoPBResize:
#endif
			}
			FTA_NoPBUpdate:

			/* Quick last check to see whether we can proceed */
			p1	ba		FTA_No_KickTA;
			{
				/* check whether we should dummy process the cmd */
				MK_MEM_ACCESS(ldad)	r3, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc0;
				MK_MEM_ACCESS_CACHED(ldad)	r4, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32RenderFlags)], drc1;
				wdf		drc0;
				and		r3, r3, #SGXMKIF_HWRTDATA_CMN_STATUS_DUMMYPROC;
				wdf		drc1;
				and		r4, r4, #SGXMKIF_RENDERFLAGS_ABORT;
				/* if either of these flags are set branch to DummyProcessTA */
				or.testnz	p0, r3, r4;
				p0	ba	DummyProcessTA;
				/* If we get to here then we are going to Kick the HW for the CMDTA */
				/* KickTA will branch straight back to calling code, if called */
				mov		r2, r8;	/* Move the priority level into r2 */
				ba		KickTA;
			}
			FTA_No_KickTA:
			STARTTACONTEXTSWITCH(FTA);
		}
	}
	FTA_End:
	/* branch back to calling code */
	lapc;
}

/*****************************************************************************
 RenderDetailsUpdate routine - copies all the required data from the CMDTA
 							   to its allocated HWRenderDetails

 inputs:	r0 - CCB command of TA kick about to be started
 temps:		r1, r2, r3, r4, r5, r6, r7, r8
*****************************************************************************/
RenderDetailsUpdate:
{
	MK_MEM_ACCESS_CACHED(ldad)	r1, [r0, #DOFFSET(SGXMKIF_CMDTA.sHWRenderDetailsDevAddr)], drc0;
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r0, #DOFFSET(SGXMKIF_CMDTA.ui32RenderFlags)], drc1;
	wdf		drc0;
	wdf		drc1;
#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH)
	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui3CurrentMTIdx)], #0;
#endif
	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32RenderFlags)], r2;

	/* check if there is sTA3DDependancy to copy */
	mov	r3, #SGXMKIF_RENDERFLAGS_TA_DEPENDENCY;
	and.testz	p0, r2, r3;
	p0	ba	RDU_NotTADependency;
	{
		/* Copy the sTA3DDependency data */
		MK_MEM_ACCESS_CACHED(ldad.f2)	r2, [r0, #DOFFSET(SGXMKIF_CMDTA.sShared.sTA3DDependency.ui32WriteOpsPendingVal)-1], drc0;
		wdf		drc0;
		MK_MEM_ACCESS(stad.f2)	[r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sTA3DDependency.ui32WriteOpsPendingVal)-1], r2;
	}
	RDU_NotTADependency:

#if defined(DEBUG) || defined(PDUMP) || defined(SUPPORT_SGX_HWPERF)
	MK_MEM_ACCESS_CACHED(ldad)	r3, [r0, #DOFFSET(SGXMKIF_CMDTA.ui32FrameNum)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32TAFrameNum)], r3;
	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32FrameNum)], r3;
	MK_TRACE(MKTC_TA_FRAMENUM, MKTF_TA | MKTF_FRMNUM);
	MK_TRACE_REG(r3, MKTF_TA | MKTF_FRMNUM);
#endif

	/* copy the 3D/TQ synchronization info as well */
	MK_MEM_ACCESS_CACHED(ldad.f4)	r2, [r0, #DOFFSET(SGXMKIF_CMDTA.sShared.ui323DTQSyncWriteOpsPendingVal)-1], drc0;
	wdf		drc0;
	MK_MEM_ACCESS(stad.f4)	[r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32TQSyncWriteOpsPendingVal)-1], r2;

	/* link the RenderDetails to DstSyncList */
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r0, #DOFFSET(SGXMKIF_CMDTA.sHWDstSyncListDevAddr)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWDstSyncListDevAddr)], r2;

	/*
		texture dependency details
		r0 - SGXMKIF_CMDTA
		r1 - SGXMKIF_HWRENDERDETAILS
		r2 - copy of SGXMKIF_CMDTA
		r3 - copy of SGXMKIF_HWRENDERDETAILS
		r4 - stores ui32NumSrcSyncs
		r5, r6, r7, r8 - temps
	*/

#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)

	/* Load ui32NumTASrcSyncs */
	MK_MEM_ACCESS_CACHED(ldad)	r4, [r0, #DOFFSET(SGXMKIF_CMDTA.sShared.ui32NumTASrcSyncs)], drc0;
	wdf		drc0;

	/* Copy ui32NumTASrcSyncs to SGXMKIF_HWRENDERDETAILS */
	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32NumTASrcSyncs)], r4;

	/* Skip the following code block if (ui32NumTASrcSyncs == 0) */
	and.testz	p0, r4, r4;
	p0	ba		RDU_NoTASrcSyncs;
	{
		/* set the bases */
		mov 	r5, #OFFSET(SGXMKIF_CMDTA.sShared.asTASrcSyncs);
		iaddu32 r2, r5.low, r0;
		mov 	r5, #OFFSET(SGXMKIF_HWRENDERDETAILS.asTASrcSyncs);
		iaddu32 r3, r5.low, r1;

		MK_MEM_ACCESS_CACHED(ldad.f4)	r5, [r2, #0++], drc0;
		RDU_LoadNextTASrcSync:
		{
			/* Copy the next asTASrcSyncs[] array element to SGXMKIF_HWRENDERDETAILS */

			/* Decrement the ui32NumTASrcSyncs count: exit loop if (ui32NumTASrcSyncs == 0) */
			isub16.testz	r4, p0, r4, #1;

			wdf		drc0;
			MK_MEM_ACCESS(stad.f4)	[r3, #0++], r5;
			!p0	MK_MEM_ACCESS_CACHED(ldad.f4)	r5, [r2, #0++], drc0;
			!p0	ba	RDU_LoadNextTASrcSync;
		}
	}
	RDU_NoTASrcSyncs:

	/* Load ui32NumTASrcSyncs */
	MK_MEM_ACCESS_CACHED(ldad)	r4, [r0, #DOFFSET(SGXMKIF_CMDTA.sShared.ui32NumTADstSyncs)], drc0;
	wdf		drc0;

	/* Copy ui32NumTADstSyncs to SGXMKIF_HWRENDERDETAILS */
	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32NumTADstSyncs)], r4;

	/* Skip the following code block if (ui32NumTADstSyncs == 0) */
	and.testz	p0, r4, r4;
	p0	ba		RDU_NoTADstSyncs;
	{
		/* set the bases */
		mov 	r5, #OFFSET(SGXMKIF_CMDTA.sShared.asTADstSyncs);
		iaddu32 r2, r5.low, r0;
		mov 	r5, #OFFSET(SGXMKIF_HWRENDERDETAILS.asTADstSyncs);
		iaddu32 r3, r5.low, r1;

		MK_MEM_ACCESS_CACHED(ldad.f4)	r5, [r2, #0++], drc0;
		RDU_LoadNextTADstSync:
		{
			/* Copy the next asTADstSyncs[] array element to SGXMKIF_HWRENDERDETAILS */

			/* Decrement the ui32NumTADstSyncs count: exit loop if (ui32NumTADstSyncs == 0) */
			isub16.testz	r4, p0, r4, #1;

			wdf		drc0;
			MK_MEM_ACCESS(stad.f4)	[r3, #0++], r5;
			!p0	MK_MEM_ACCESS_CACHED(ldad.f4)	r5, [r2, #0++], drc0;
			!p0	ba	RDU_LoadNextTADstSync;
		}
	}
	RDU_NoTADstSyncs:

	/* Load ui32Num3DSrcSyncs */
	MK_MEM_ACCESS_CACHED(ldad)	r4, [r0, #DOFFSET(SGXMKIF_CMDTA.sShared.ui32Num3DSrcSyncs)], drc0;
	wdf		drc0;

	/* Copy ui32Num3DSrcSyncs to SGXMKIF_HWRENDERDETAILS */
	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32Num3DSrcSyncs)], r4;

	/* Skip the following code block if (ui32Num3DSrcSyncs == 0) */
	and.testz	p0, r4, r4;
	p0	ba		RDU_No3DSrcSyncs;
	{
		/* set the bases */
		mov 	r5, #OFFSET(SGXMKIF_CMDTA.sShared.as3DSrcSyncs);
		iaddu32 r2, r5.low, r0;
		mov 	r5, #OFFSET(SGXMKIF_HWRENDERDETAILS.as3DSrcSyncs);
		iaddu32 r3, r5.low, r1;

		MK_MEM_ACCESS_CACHED(ldad.f4)	r5, [r2, #0++], drc0;
		RDU_LoadNext3DSrcSync:
		{
			/* Copy the next as3DSrcSyncs[] array element to SGXMKIF_HWRENDERDETAILS */

			/* Decrement the ui32Num3DSrcSyncs count: exit loop if (ui32NumSrcSyncs == 0) */
			isub16.testz	r4, p0, r4, #1;

			wdf		drc0;
			MK_MEM_ACCESS(stad.f4)	[r3, #0++], r5;
			!p0	MK_MEM_ACCESS_CACHED(ldad.f4)	r5, [r2, #0++], drc0;
			!p0	ba	RDU_LoadNext3DSrcSync;
		}
	}
	RDU_No3DSrcSyncs:

#else/* #if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS) */

	/* Load ui32NumSrcSyncs */
	MK_MEM_ACCESS_CACHED(ldad)	r4, [r0, #DOFFSET(SGXMKIF_CMDTA.sShared.ui32NumSrcSyncs)], drc0;
	wdf		drc0;

	/* Copy ui32NumSrcSyncs to SGXMKIF_HWRENDERDETAILS */
	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32NumSrcSyncs)], r4;

	/* Skip the following code block if (ui32NumSrcSyncs == 0) */
	and.testz	p0, r4, r4;
	p0	ba		RDU_NoSrcSyncs;
	{
		/* set the bases */
		mov 	r5, #OFFSET(SGXMKIF_CMDTA.sShared.asSrcSyncs);
		iaddu32 r2, r5.low, r0;
		mov 	r5, #OFFSET(SGXMKIF_HWRENDERDETAILS.asSrcSyncs);
		iaddu32 r3, r5.low, r1;

		/*
			We don't have enough temps to block load all 6 elements
			so load two and then 4
		*/
		MK_MEM_ACCESS_CACHED(ldad.f2)	r5, [r2, #0++], drc0;
		wdf		drc0;
		MK_MEM_ACCESS(stad.f2)	[r3, #0++], r5;
		MK_MEM_ACCESS_CACHED(ldad.f4)	r5, [r2, #0++], drc0;
		RDU_LoadNextSrcSync:
		{
			/* Copy the next asSrcSyncs[] array element to SGXMKIF_HWRENDERDETAILS */

			/* Decrement the ui32NumSrcSyncs count: exit loop if (ui32NumSrcSyncs == 0) */
			isub16.testz	r4, p0, r4, #1;

			wdf		drc0;
			MK_MEM_ACCESS(stad.f4)	[r3, #0++], r5;

			p0	ba		RDU_NoSrcSyncs;
			MK_MEM_ACCESS_CACHED(ldad.f2)	r5, [r2, #0++], drc0;
			wdf		drc0;
			MK_MEM_ACCESS(stad.f2)	[r3, #0++], r5;
			MK_MEM_ACCESS_CACHED(ldad.f4)	r5, [r2, #0++], drc0;
			ba	RDU_LoadNextSrcSync;
		}
	}
	RDU_NoSrcSyncs:
#endif /* #if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS) */

#if defined(SGX_FEATURE_VISTEST_IN_MEMORY)
	MK_MEM_ACCESS_CACHED(ldad)	r3, [r0, #DOFFSET(SGXMKIF_CMDTA.ui32VisTestCount)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32VisTestCount)], r3;
#else
	MK_MEM_ACCESS_CACHED(ldad)	r3, [r0, #DOFFSET(SGXMKIF_CMDTA.sVisTestResultsDevAddr)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sVisTestResultsDevAddr)], r3;
#endif

#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	MK_MEM_ACCESS_CACHED(ldad.f2)	r3, [r0, #DOFFSET(SGXMKIF_CMDTA.ui32ZBufferStride)-1], drc0;
	wdf		drc0;
	MK_MEM_ACCESS(stad.f2)	[r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32ZBufferStride)-1], r3;
#endif

	MK_MEM_ACCESS_CACHED(ldad.f3)	r3, [r0, #DOFFSET(SGXMKIF_CMDTA.aui32SpecObject)-1], drc0;
	wdf		drc0;
	MK_MEM_ACCESS(stad.f3)	[r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.aui32SpecObject)-1], r3;

	MK_MEM_ACCESS_CACHED(ldad)	r2, [r0, #DOFFSET(SGXMKIF_CMDTA.ui32Num3DRegs)], drc0;
	wdf		drc0;

	and.testz	p0, r2, r2;
	p0	ba		RDU_No3DRegUpdates;
	{
		mov		r3, #OFFSET(SGXMKIF_CMDTA.s3DRegUpdates);
		iadd32		r3, r3.low, r0;
		imae		r4, r2.low, #SIZEOF(PVRSRV_HWREG), r3, u32;

		MK_MEM_ACCESS_CACHED(ldad.f2)	r5, [r3, #0++], drc0;
		RDU_UpdateNext3DRegister:
		{
			wdf		drc0;
			xor.testnz	p0, r3, r4;
			MK_MEM_ACCESS(stad)	[r5], r6;
			p0	MK_MEM_ACCESS_CACHED(ldad.f2)	r5, [r3, #0++], drc0;
			p0	ba		RDU_UpdateNext3DRegister;
		}
	}
	RDU_No3DRegUpdates:

	MK_MEM_ACCESS_CACHED(ldad)	r2, [r0, #DOFFSET(SGXMKIF_CMDTA.sShared.ui32Num3DStatusVals)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32Num3DStatusVals)], r2;
	and.testz	p0, r2, r2;
	p0	ba		RDU_No3DStatusVals;
	{
		mov			r3, #OFFSET(SGXMKIF_CMDTA.sShared.sCtl3DStatusInfo);
		iadd32		r3, r3.low, r0;
		imae		r4, r2.low, #SIZEOF(CTL_STATUS), r3, u32;

		MK_MEM_ACCESS_CACHED(ldad.f2)	r5, [r3, #0++], drc0;
		RDU_CopyNext3DStatusVal:
		{
			wdf		drc0;
			xor.testnz	p0, r3, r4;
			MK_MEM_ACCESS(stad.f2)	[r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sCtl3DStatusInfo)++], r5;
			p0	MK_MEM_ACCESS_CACHED(ldad.f2)	r5, [r3, #0++], drc0;
			p0	ba		RDU_CopyNext3DStatusVal;
		}
	}
	RDU_No3DStatusVals:
	idf		drc1, st;
	wdf		drc1;

	/* return */
	lapc;
}


/*****************************************************************************
 AdvanceTACCBROff routine - calculates the new ROff for the TA CCB and
 							decrements the kick count

 inputs:	r0 - render context of TA CCB to be advanced (maintained)
			r1 - CCB command of TA kick just completed (maintained)
 temps:		r2, r3, r4
*****************************************************************************/
AdvanceTACCBROff:
{
	/* Calc. new read offset into the CCB */
	MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sTACCBCtlDevAddr)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r3, [r2, #DOFFSET(PVRSRV_SGX_CCB_CTL.ui32ReadOffset)], drc0;
	MK_MEM_ACCESS_CACHED(ldad)	r4, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32Size)], drc1;
	wdf		drc0;
	wdf		drc1;
	iaddu32	r3, r3.low, r4;
	and		r3, r3, #(SGX_CCB_SIZE - 1);
	MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_CCB_CTL.ui32ReadOffset)], r3;

	/* Decrement kick count for current context */
	MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32TACount)], drc0;
	wdf		drc0;
	iadd32		r2, r2.low, #-1;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32TACount)], r2;
	idf		drc0, st;
	wdf		drc0;

	/* return */
	lapc;
}


/*****************************************************************************
 DummyProcessTA routine	- performs the minimum that is required to make a TA
 						  appear to have been completed

 inputs:	r0 - context of TA kick to be started
			r1 - CCB command of TA kick about to be started
			R_RTData - SGXMKIF_HWRTDATA for TA command
 temps:		r2, r3, r4, r5, r6, r7, r8, r9, r10, r11
*****************************************************************************/
DummyProcessTA:
{
	MK_TRACE(MKTC_DPTA_START, MKTF_SCH | MKTF_TA);

	WRITEHWPERFCB(TA, MK_TA_DUMMY_START, GRAPHICS, #0, p0, kta_dummy_1, r2, r3, r4, r5, r6, r7, r8, r9);

	/* Reset the priority scheduling level block, as it is no longer blocked */
	MK_STORE_STATE(ui32BlockedPriority, #(SGX_MAX_CONTEXT_PRIORITY - 1));

	MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc0;
	wdf		drc0;
	and.testnz		p0, r2, #SGXMKIF_HWRTDATA_CMN_STATUS_DUMMYPROC;
	p0	ba	DPTA_NotSceneAbort;
	{
		MK_TRACE(MKTC_DPTA_NORENDER, MKTF_SCH | MKTF_TA);
		MK_MEM_ACCESS_CACHED(ldad)	r3, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
		mov		r4, #(SGXMKIF_TAFLAGS_FIRSTKICK | SGXMKIF_TAFLAGS_RESUMEDAFTERFREE);
		wdf		drc0;

		/* Check if the context does not need freeing. */
		and.testnz	p0, r3, r4;
		p0	ba	DPTA_NotSceneAbort;
		{
			MK_TRACE(MKTC_DPTA_MEMFREE, MKTF_SCH | MKTF_TA);

			mov		r6, r1;
			mov		r7, pclink;
			bal		FreeContextMemory;
			mov		pclink, r7;
			mov		r1, r6;
		}
	}
	DPTA_NotSceneAbort:

	/* Set up the TA for the memory context bring processed. */
	MK_MEM_ACCESS(ldad)	r4, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderContextDevAddr)], drc0;
	wdf				drc0;
	SETUPDIRLISTBASE(DPTA, r4, #0);

	SWITCHEDMTOTA();

	/* Clear the Tail Pointer Memory */
	MK_MEM_ACCESS(ldad)	r3, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32TailSize)], drc1;
	//FIXME-MP: loop by SGXMK_STATE.ui32CoresEnabled;
	MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.asTailPtrDevAddr[0])], drc0;
	wdf		drc1;
	shr		r3, r3, #6;		// ((TailSizeInBytes / sizeof(IMG_UINT32)) / BurstSizeInDWords)
	wdf		drc0;

	DPTA_ClearNextTPMem:
	{
		/* Do the burst */
		MK_MEM_ACCESS_BPCACHE(stad.f16)	[r2, #0++], #0;

		/* have we finished yet */
		isub16.testnz	r3, p0, r3, #1;
		p0	ba	DPTA_ClearNextTPMem;
	}
	idf 	drc1, st;
	wdf		drc1;

	SWITCHEDMBACK();

	/* this TA was aborted, therefore do the minimum */
	/* Do the jobs associated with KickTA */
	/* if the DPM threshold need to be changed do it */
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
	mov		r3, #SGXMKIF_TAFLAGS_PB_THRESHOLD_UPDATE;
	wdf		drc0;
	and.testz	p0, r2, r3;
	p0	ba	 DPTA_NoThresholdUpdate;
	{
#if defined(FIX_HW_BRN_32302) || defined(SUPPORT_SGX_DOUBLE_PIXEL_PARTITIONS)
		mov	r17, #USE_FALSE;
#endif
		PVRSRV_SGXUTILS_CALL(DPMThresholdUpdate);
	}
	DPTA_NoThresholdUpdate:

	/* Save the pclink */
	mov		r9, pclink;
	mov		r10, r0;	// save the rendercontext
	mov		r0, r1;	// move CMD to r0
	/* Copy to render details any updates sent in the CCB */
	bal		RenderDetailsUpdate;
	mov		r1, r0;	// restore the CMD
	mov		r0, r10;	// restore the rendercontext
	/* Restore the pclink */
	mov		pclink, r9;

	/* end of Jobs associcated with KickTA, onto TAFinished Jobs */

	MK_MEM_ACCESS_CACHED(ldad.f2)	r2, [r1, #DOFFSET(SGXMKIF_CMDTA.sShared.ui32TATQSyncReadOpsPendingVal)-1], drc0;
	wdf		drc0;
	and.testz	p0, r3, r3;
	p0	ba	DPTA_NoTQSyncReadOp;
	{
		mov	r4, #1;
		iaddu32 r2, r4.low, r2;
		MK_MEM_ACCESS_BPCACHE(stad)	[r3], r2;
		idf		drc0, st;
	}
	DPTA_NoTQSyncReadOp:

	/* Clear the switching variables */
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_CMDTA.sHWRenderDetailsDevAddr)], drc0;
	wdf		drc0;

	/* Write out any status values put into the CCB */
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_CMDTA.sShared.ui32NumTAStatusVals)], drc0;
	wdf		drc0;
	and.testz		p0, r2, r2;
	p0	ba		DPTA_NoStatusWrites;
	{
		MK_TRACE(MKTC_DPTA_UPDATESTATUSVALS, MKTF_TA | MKTF_SV);


		mov		r3, #OFFSET(SGXMKIF_CMDTA.sShared.sCtlTAStatusInfo);
		iaddu32		r3, r3.low, r1;
		imae		r2, r2.low, #SIZEOF(CTL_STATUS), r3, u32;

		MK_MEM_ACCESS_CACHED(ldad.f2)	r4, [r3, #0++], drc0;
		DPTA_WriteNextStatus:
		{
			wdf		drc0;
			xor.testnz	p0, r2, r3;
			MK_MEM_ACCESS_BPCACHE(stad)	[r4], r5;
			MK_TRACE_REG(r5, MKTF_TA | MKTF_SV);
			p0	MK_MEM_ACCESS_CACHED(ldad.f2)	r4, [r3, #0++], drc0;
			p0	ba		DPTA_WriteNextStatus;
		}


		MK_TRACE(MKTC_DPTA_UPDATESTATUSVALS_DONE, MKTF_TA | MKTF_SV);
	}
	DPTA_NoStatusWrites:

	/* Send interrupt to host */
	mov	 	r2, #EUR_CR_EVENT_STATUS_SW_EVENT_MASK;
	str	 	#EUR_CR_EVENT_STATUS >> 2, r2;

	/* Update RTData status if necessary */
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc1;
	MK_MEM_ACCESS_CACHED(ldad)	r3, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32RenderFlags)], drc1;
	MK_MEM_ACCESS(ldad)	r4, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc0;
	wdf		drc1;

	and.testnz		p0, r2, #SGXMKIF_TAFLAGS_FIRSTKICK | SGXMKIF_TAFLAGS_RESUMEMIDSCENE;
	/* Set p1 to true if LASTKICK is set and NODEALLOC isn't set. */
	and.testz		p1, r3, #SGXMKIF_RENDERFLAGS_NODEALLOC;
	p1	and.testnz	p1, r2, #SGXMKIF_TAFLAGS_LASTKICK;
	and.testnz		p2, r2, #SGXMKIF_TAFLAGS_LASTKICK;
	wdf		drc0;
	/* If first kick, only leave the dummy proc untouched in common status */
	p0	and		r4, r4, #SGXMKIF_HWRTDATA_CMN_STATUS_DUMMYPROC;
	p1	or		r4, r4, #SGXMKIF_HWRTDATA_CMN_STATUS_TACOMPLETE;

#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	/* Store out the modified common status */
	p1	MK_MEM_ACCESS(stad)	[R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], r4;
	/* Clear all the RTs status bits */
	MK_MEM_ACCESS(ldad)	r4, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sRTStatusDevAddr)], drc0;
	MK_MEM_ACCESS(ldad)	r5, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32NumRTsInArray)], drc0;
	mov		r6, #SIZEOF(IMG_UINT32);
	wdf		drc0;
	DPTA_ClearNextRTStatus:
	{
		MK_MEM_ACCESS(stad)	[r4], #0;
		iaddu32		r4, r6.low, r4;
		isub16.testnz	r5, p3, r5, #1;
		p3	ba	DPTA_ClearNextRTStatus;
	}
#else
	/* No need to load RT status separately, as RT -> ui32CommonStatus */
	/* Clear the RT status bits */
	and		r4, r4, ~#SGXMKIF_HWRTDATA_RT_STATUS_ALL_MASK;
	MK_MEM_ACCESS(stad)	[R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], r4;
#endif
	idf		drc0, st;
	wdf		drc0;

	/* get the HWRenderDetails */
	MK_MEM_ACCESS_CACHED(ldad)	r6, [r1, #DOFFSET(SGXMKIF_CMDTA.sHWRenderDetailsDevAddr)], drc0;
	wdf		drc0;

	/* only FirstKick possible is the one in pwhen dummy processing */
	!p0	ba	DPTA_NotFirstKick;
	{
		/* First kick, add render details to list of partial renders for this context */
		MK_MEM_ACCESS(ldad)	r3, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPartialRendersHead)], drc0;
		wdf		drc0;
		MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPartialRendersTail)], drc0;
		wdf		drc0;
		and.testz	p0, r3, r3;
		p0	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPartialRendersHead)], r6;
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPartialRendersTail)], r6;
		!p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sNextDevAddr)], r6;
		!p0	MK_MEM_ACCESS(stad)	[r6, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sPrevDevAddr)], r4;
		idf		drc0, st;
		wdf		drc0;
	}
	DPTA_NotFirstKick:

	p1 ba	DPTA_LastKick;
	{
		p2 ba	DPTA_LastKick;
		{
			/* Increment the complete count for the RTDataSet */
			/* This is necessary to ensure the dummyprocess3D is not called to clean up RTData
			 * if further TA kicks are queued.
			 */
			MK_TRACE(MKTC_DPTA_INC_COMPLETECOUNT, MKTF_TA | MKTF_SCH);

			MK_MEM_ACCESS(ldad)	r4, [r6, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWRTDataSetDevAddr)], drc0;
			wdf		drc0;
			MK_MEM_ACCESS(ldad)	r3, [r4, #DOFFSET(SGXMKIF_HWRTDATASET.ui32CompleteCount)], drc0;
			wdf		drc0;
			REG_INCREMENT(r3, p0);
			MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWRTDATASET.ui32CompleteCount)], r3;
			idf		drc0, st;
			wdf		drc0;
			ba		DPTA_NoTaskReady;
		}
	}
	DPTA_LastKick:
	/* Last Kick of the scene */
	/* move the scene to the complete list */
	MOVESCENETOCOMPLETELIST(r0, r6);

#if defined(SGX_FEATURE_2D_HARDWARE)
	FIND2D();
#endif

	/* Before searching for another TA make sure we check for unblocked renders */
	FIND3D();

	DPTA_NoTaskReady:
	/* move on the TACCB */
	mov		r5, pclink;	// save of the pclink
	bal		AdvanceTACCBROff;
	mov		pclink, r5;	// restore the pclink

	/* Remove the current render context from the list of the partial render contexts list */
	MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sCommon.ui32Priority)], drc0;
	wdf		drc0;

	MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], drc0;
	MK_MEM_ACCESS(ldad)	r5, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], drc1;
	wdf		drc0;
	and.testz	p0, r4, r4;	// is prev null?
	wdf		drc1;
	/* (prev == NULL) ? head = current->next : prev->nextptr = current->next */
	!p0	ba	DPTA_NoHeadUpdate;
	{
		MK_STORE_STATE_INDEXED(sPartialRenderContextHead, r1, r5, r2);
	}
	DPTA_NoHeadUpdate:
	!p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], r5;

	and.testz	p1, r5, r5;	// is next null?
	/* (next == NULL) ? tail == current->prev : next->prevptr = current->prev */
	!p1	ba	DPTA_NoTailUpdate;
	{
		MK_STORE_STATE_INDEXED(sPartialRenderContextTail, r1, r4, r3);
	}
	DPTA_NoTailUpdate:
	!p1	MK_MEM_ACCESS(stad)	[r5, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], r4;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], #0;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], #0;
	idf		drc1, st;

	/* make sure all previous writes have completed */
	wdf		drc1;
	/* check whether there are any more kicks */
	MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32TACount)], drc0;
	/* also check whether there is any mid scene renders */
	MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPartialRendersHead)], drc1;
	wdf		drc0;

	wdf		drc1;
	/* if either is non-zero add it back onto the list */
	or.testz	p1, r2, r4;
	wdf		drc0;

	/* If there is something queued then add the current render context back to the tail of the partial render context list */
	p1	ba		DPTA_NoTAKicksOutstanding;
	{
		/* Get the Tail */
		MK_LOAD_STATE_INDEXED(r4, sPartialRenderContextTail, r1, drc0, r3);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and.testz	p0, r4, r4;
		!p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], r0;
		/* Update the Head if required */
		!p0	ba	DPTA_NoHeadUpdate2;
		{
			MK_STORE_STATE_INDEXED(sPartialRenderContextHead, r1, r0, r2);
		}
		DPTA_NoHeadUpdate2:
		/* Update the Tail */
		MK_STORE_STATE_INDEXED(sPartialRenderContextTail, r1, r0, r3);
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], r4;
	}
	DPTA_NoTAKicksOutstanding:
	idf		drc0, st;
	wdf		drc0;

	/* 	Now check to see if there are any other TAs we can do instead
		not bal so that it returns as if this occurrence of the call */
	FINDTA();

	WRITEHWPERFCB(TA, MK_TA_DUMMY_END, GRAPHICS, #0, p0, kta_dummy_2, r2, r3, r4, r5, r6, r7, r8, r9);
	/* Return */
	lapc;
}

#if defined(FIX_HW_BRN_33657) && defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
.import StoreDPMContextPIMUpdate;
#endif

/*****************************************************************************
 KickTA routine	- performs anything that is required before actually
 			  starting the TA

 inputs:	r0 - context of TA kick to be started
			r1 - CCB command of TA kick about to be started
			r2 - priority level of the context to be started
			R_RTData - SGXMKIF_HWRTDATA for TA command
 temps:		r2, r3, r4, r5, r6, r7, r8, r9, r10, r11
*****************************************************************************/
KickTA:
{
	MK_TRACE(MKTC_KICKTA_START, MKTF_TA);
	
	/* Trace the render context/target pointers. */
	MK_TRACE(MKTC_KICKTA_RENDERCONTEXT, MKTF_TA | MKTF_RC);
	MK_TRACE_REG(r0, MKTF_TA | MKTF_RC);
	MK_TRACE(MKTC_KICKTA_RTDATA, MKTF_TA | MKTF_RT);
	MK_TRACE_REG(R_RTData, MKTF_TA | MKTF_RT);

	/* Check if context has been switched */
	MK_MEM_ACCESS_CACHED(ldad)	r3, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
	wdf		drc0;

	MK_TRACE(MKTC_KICKTA_TACMD_DEBUG, MKTF_TA | MKTF_CSW);
	MK_TRACE_REG(r1, MKTF_TA | MKTF_CSW); /* SGXMKIF_CMDTA */
	MK_TRACE_REG(r3, MKTF_TA | MKTF_CSW); /* SGXMKIF_CMDTA.ui32Flags */

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) && defined(FIX_HW_BRN_31425)
	{
		#if defined(SGX_FEATURE_MP)

		/* Reset the master VDM to clear previous context store in multi-process environment */
		SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_SOFT_RESET >> 2, #EUR_CR_MASTER_SOFT_RESET_VDM_RESET_MASK);
		SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_SOFT_RESET >> 2, #0);
		#endif

		#if defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
		/* Reset the slave VDMs */
		SGXMK_SPRV_WRITE_REG(#EUR_CR_SOFT_RESET >> 2, #EUR_CR_SOFT_RESET_VDM_RESET_MASK);
		SGXMK_SPRV_WRITE_REG(#EUR_CR_SOFT_RESET >> 2, #0);
		#endif
		
		/* Set up the VDM dummy snapshot buffer */
		/*	r8: temp
		 *	r7: temp
		 *	r6: EUR_CR_BIF_BANK0 state
		 *	r5: temp
		 *	r4: temp
		 *	r0-r3: reserved
		 */
	#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && !defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
		MK_MEM_ACCESS(ldad)	r4, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sVDMSnapShotBufferDevAddr)], drc0;
		wdf		drc0;
		WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_CONTEXT_STORE_SNAPSHOT >> 2, r4, r5);
	#else
		MK_MEM_ACCESS(ldad)	r4, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sVDMSnapShotBufferDevAddr)], drc0;
		wdf		drc0;
	
		MK_TRACE(MKTC_KICKTA_RESET_BUFFERS,  MKTF_TA | MKTF_CSW);
		MK_TRACE_REG(r4, MKTF_TA | MKTF_CSW);
	
		/* Write all cores */
		WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_CONTEXT_STORE_SNAPSHOT >> 2, r4, r5);
		MK_LOAD_STATE_CORE_COUNT_TA(r6, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
	
		/* Start checking core0 */
		mov		r5, #0;
		KTA_NextCore:
		{
			WRITECORECONFIG(r5, #EUR_CR_VDM_CONTEXT_STORE_SNAPSHOT >> 2, r4, r7);
			REG_INCREMENT(r5, p0);
			xor.testnz	p0, r5, r6;
			p0	ba	KTA_NextCore;
		}
	#endif

		/* Switch VDM MMU context to match ukernel/EDM */
		ldr		r4, #EUR_CR_BIF_BANK0 >> 2, drc0;
		wdf		drc0;
		mov		r6, r4;
		and		r5, r4, #EUR_CR_BIF_BANK0_INDEX_EDM_MASK;
		shl		r5, r5, #(EUR_CR_BIF_BANK0_INDEX_TA_SHIFT - EUR_CR_BIF_BANK0_INDEX_EDM_SHIFT);
		
		and		r4, r4, ~#EUR_CR_BIF_BANK0_INDEX_TA_MASK;
		or		r4, r4, r5;
		SGXMK_SPRV_WRITE_REG(#EUR_CR_BIF_BANK0 >> 2, r4);
		
		MK_TRACE_REG(r4, MKTF_TA | MKTF_CSW);
		MK_TRACE(MKTC_KICKTA_CHKPT_START_DUMMY_CS,  MKTF_TA | MKTF_CSW);
		
		/* Start dummy context switch */
	#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && !defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
		WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_CONTEXT_STORE_START >> 2, \
							#EUR_CR_MASTER_VDM_CONTEXT_STORE_START_PULSE_MASK, r4);
	#else
		/* Perform a VDM context store and wait for the store to occur */
		str		#EUR_CR_VDM_CONTEXT_STORE_START >> 2, #EUR_CR_VDM_CONTEXT_STORE_START_PULSE_MASK;
	#endif
	
	 	/* Wait for dummy store. */
		KTA_WaitForStore:
		{
			READMASTERCONFIG(r5, #EUR_CR_MASTER_VDM_CONTEXT_STORE_STATUS >> 2, drc0);
			wdf				drc0;
			shl.tests		p0, r5, #(31 - EUR_CR_MASTER_VDM_CONTEXT_STORE_STATUS_NA_SHIFT);
			!p0				ba	KTA_WaitForStore;
		}
		MK_TRACE(MKTC_KICKTA_CHKPT_START_DUMMY_TAK, MKTF_TA | MKTF_CSW);

		/* Set up VDM control stream in master and slaves */
		MK_MEM_ACCESS(ldad)	r4, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sVDMCtrlStreamBufferDevAddr)], drc0;
		ldr		r8, #EUR_CR_VDM_CTRL_STREAM_BASE >> 2, drc0; /* save off current CS base */
		wdf		drc0;
	
		str		#EUR_CR_VDM_CTRL_STREAM_BASE >> 2, r4;
	
		/* Start the VDM */
		str		#EUR_CR_VDM_START >> 2, #EUR_CR_VDM_START_PULSE_MASK;
	
		/* Wait for VDM to go idle */
		KTA_VDMWaitForMasterKick:
		{
			READMASTERCONFIG(r4, #EUR_CR_MASTER_VDM_WAIT_FOR_KICK >> 2, drc0);
			wdf				drc0;
			and.testz		p0, r4, #EUR_CR_MASTER_VDM_WAIT_FOR_KICK_STATUS_MASK;
			p0 				ba	KTA_VDMWaitForMasterKick;
		}
		MK_TRACE(MKTC_KICKTA_CHKPT_WAIT_FOR_DUMMY_KICK, MKTF_TA | MKTF_CSW);

		/* Check the slaves */
		MK_LOAD_STATE_CORE_COUNT_TA(r5, drc0);
		mov             r7, #0;
		MK_WAIT_FOR_STATE_LOAD(drc0);
		KTA_VDMWaitForSlavesKick:
		{
			{
				READCORECONFIG(r4, r7, #EUR_CR_VDM_WAIT_FOR_KICK >> 2, drc0);
				wdf				drc0;
				and.testz		p0, r4, #EUR_CR_VDM_WAIT_FOR_KICK_STATUS_MASK;
				p0 				ba	KTA_VDMWaitForSlavesKick;
			}
			/* Move on to the next core */
			MK_TRACE(MKTC_KICKTA_CHKPT_WAIT_NEXT_CORE, MKTF_TA | MKTF_CSW);
			REG_INCREMENT(r7, p0);
			xor.testnz      p0, r7, r5;
			p0				ba	KTA_VDMWaitForSlavesKick;
		}

		/* Reset the master VDM */
		#if defined(SGX_FEATURE_MP)
		SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_SOFT_RESET >> 2, #EUR_CR_MASTER_SOFT_RESET_VDM_RESET_MASK);
		SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_SOFT_RESET >> 2, #0);
		#endif

		/* Reset VDM MMU context and CS base address */
		str		#EUR_CR_VDM_CTRL_STREAM_BASE >> 2, r8;
		SGXMK_SPRV_WRITE_REG(#EUR_CR_BIF_BANK0 >> 2, r6);
	}
	KTA_DontResetContextStoreBuffer:
	MK_TRACE(MKTC_KICKTA_CHKPT_RESET_COMPLETE, MKTF_TA | MKTF_CSW);

#endif /* SGX_FEATURE_VDM_CONTEXT_SWITCH && FIX_HW_BRN_31425 */

	/* Reset the priority scheduling level block */
	MK_STORE_STATE(ui32BlockedPriority, #(SGX_MAX_CONTEXT_PRIORITY - 1));

	/* Save all the main data */
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderContext)], r0;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTACCBCmd)], r1;
	MK_STORE_STATE(ui32TACurrentPriority, r2);
	idf		drc0, st;
	wdf		drc0;

	/* Save the PID for easy retrieval later */
	MK_MEM_ACCESS(ldad)	r4, [r0, HSH(DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32PID))], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32TAPID)], r4;
	MK_TRACE(MKTC_KICKTA_PID, MKTF_PID);
	MK_TRACE_REG(r4, MKTF_PID);
	
	/* Save the current TA render details */
	MK_MEM_ACCESS_CACHED(ldad)	r4, [r1, #DOFFSET(SGXMKIF_CMDTA.sHWRenderDetailsDevAddr)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderDetails)], r4;
	MK_MEM_ACCESS(stad)	[R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderDetailsDevAddr)], r4;
	idf		drc0, st;

	/* Wait till the sTARenderDetails write has completed */
	wdf		drc0;

	/* If RTData has changed then store context to ensure tail pointers are kept */
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], drc0;
	wdf		drc0;
	and.testz	p0, r2, r2;
	p0	ba		KTA_NoContextStore;
	{
		xor.testz	p0, r2, R_RTData;
		/* if they are the same check whether this scene has been interrupted and needs storing */
		/* r1=scene about to be kicked */
		!p0	ba	KTA_ContextStore;
		{
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
			MK_MEM_ACCESS_CACHED(ldad)	r3, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
			mov		r4, #SGXMKIF_TAFLAGS_VDM_CTX_SWITCH;
			wdf		drc0;
			and.testz	p0, r3, r4;
			p0	ba		KTA_NoContextStore;
#else
			ba KTA_NoContextStore;
#endif
		}
		KTA_ContextStore:
		{
			/* Mark this context as 'free' */
			MK_TRACE(MKTC_KICKTA_FREECONTEXT, MKTF_TA | MKTF_SCH | MKTF_RC);

			ldr		r4, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc0;
			wdf		drc0;
			and		r4, r4, #EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_MASK;
			shr		r11, r4, #EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_SHIFT;
			and.testz	p0, r11, #1;
			p0	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext0RTData)], #0;
			!p0	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext1RTData)], #0;
			idf		drc0, st;
			wdf		drc0;

			/* Find out the RenderContext of the RTDATA */
			MK_MEM_ACCESS(ldad)	r3, [r2, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderContextDevAddr)], drc0;
			wdf		drc0;
			/* Before we store the context we need to make sure we have
			   the correct memory context */
			SETUPDIRLISTBASE(KTA_STAC, r3, #SETUP_BIF_TA_REQ);

			/* Store the DPM state via the TA Index */
			STORETACONTEXT(r2, r11);

#if defined(FIX_HW_BRN_33657) && defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
			/* Load the RT status flags for the context which was stored */
			MK_MEM_ACCESS(ldad)	r3, [r2, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc1;
			wdf		drc1;
			and.testz	p0, r3, #SGXMKIF_HWRTDATA_RT_STATUS_VDMCTXSTORE;
			p0	ba		KTA_NoPIMPatch;
			{
				/* Patch the DPM PIM list */
				MK_TRACE(MKTC_KICKTA_PIM_PATCHING, MKTF_TA | MKTF_CSW);
				SWITCHEDMTOTA();
				mov	r16, r2;	/* RTdata to patch */
				PVRSRV_SGXUTILS_CALL(StoreDPMContextPIMUpdate);
				SWITCHEDMBACK();
			}
			KTA_NoPIMPatch:
#endif
		}
	}
	KTA_NoContextStore:

#if defined(FIX_HW_BRN_35239)
	MK_MEM_ACCESS_CACHED(ldad)	r3, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
	wdf		drc0;

	/* Only on first in scene kick*/
	and.testz	p0, r3, #SGXMKIF_TAFLAGS_FIRSTKICK;
	p0	br	KTA_NoResetTE;
	{
		str		#EUR_CR_SOFT_RESET >> 2, #EUR_CR_SOFT_RESET_TA_RESET_MASK;
		str		#EUR_CR_SOFT_RESET >> 2, #0;
	}
	KTA_NoResetTE:
#endif

	/* get the current TA HWPBDesc */
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc.uiAddr)], drc0;
	/* get the new TA HWPBDesc */
	MK_MEM_ACCESS(ldad)	r3, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPBDescDevVAddr.uiAddr)], drc0;
	/* get the current 3D HWPBDesc */
	MK_MEM_ACCESS_BPCACHE(ldad)	r4, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DHWPBDesc.uiAddr)], drc0;
	wdf		drc0;

	/* Check if there are PB Blocks to initialise */
	MK_MEM_ACCESS(ldad)	r5, [r3, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], drc0;
	/* if Current PB == New PB then no store or load required */
	xor.testz	p0, r2, r3;
	wdf		drc0;

#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
	/* If PB is already loaded, a load or store is still required when a grow or shrink is pending */
	/* 
	 If the PB is the same and the context switch flag is set, there is no need to reload PB.
	 As any PB update would have already been done when the command first ran
	*/
	p0	mov	r6, #SGXMKIF_TAFLAGS_VDM_CTX_SWITCH;
	p0	and.testnz	p0, r5, r6;
	p0	ba	KTA_NoPBLoad;

	/* The PB is different or we have not context switched before on this command */
	/* reset the original predicate */
	xor.testz	p0, r2, r3;

	p0	and.testz	p0, r5, #HWPBDESC_FLAGS_GROW;
#endif
	p0	ba	KTA_NoPBLoad;
	{
		/* if Current PB == NULL then no store required */
		and.testz	p0, r2, r2;
		p0	ba	KTA_NoPBStore;
		{
#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS) && !defined(FIX_HW_BRN_25910)
			/* if Current PB == 3D PB then no store required */
			xor.testz	p0, r2, r4;
			p0	ba	KTA_NoPBStore;
#endif
			{
				/* PB Store: No idle required as we know the PB is not in use */
				STORETAPB();
			}
		}
		KTA_NoPBStore:

		/*
			If the new PB == 3D PB, we have to:
				1) Idle the 3D, so no deallocations can take place
				2) Store the 3D PB
				3) Then load the up-to-date values to TA PB
				4) Resume the 3D
		*/
		xor.testnz	p0, r3, r4;
		p0	ba	KTA_NoPBCopy;
		{
			/* Setup registers */
			mov		r16, #USE_IDLECORE_3D_REQ;
			PVRSRV_SGXUTILS_CALL(IdleCore);

			/* Store the current details of the 3D PB */
			STORE3DPB();

#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
			ba	KTA_PBDescReady;
#endif
		}
		KTA_NoPBCopy:
#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
		{
			/* If the PB is already loaded, it is a grow so store the PB first */
			xor.testnz	p0, r3, r2;
			p0	ba KTA_PBDescReady;
			{
				/* Ensure the correct memory context is setup */
				SETUPDIRLISTBASE(KTA_PBGROW, r0, #SETUP_BIF_TA_REQ);

				/* PB Load: */
				STORETAPB();

				/* Skip the dirlist setup as it has been done */
				ba	KTA_LoadTAPB;
			}
		}
		KTA_PBDescReady:
#endif

		/* Ensure the correct memory context is setup */
		SETUPDIRLISTBASE(KTA_LTAPB, r0, #SETUP_BIF_TA_REQ);

#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
		KTA_LoadTAPB:
#endif
		/*
			FIXME: On cores where EUR_CR_DPM_CONTEXT_PB_BASE is correctly implemented,
			we need to ensure that the 3D is idle when updating that register.
		*/

		/* PB Load: */
#if defined(FIX_HW_BRN_32302) || defined(SUPPORT_SGX_DOUBLE_PIXEL_PARTITIONS)
		MK_MEM_ACCESS_CACHED(ldad)	r6, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32NumVertexPartitions)], drc1;
		MK_MEM_ACCESS_CACHED(ldad)	r7, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32SPMNumVertexPartitions)], drc1;
		wdf		drc1;
		xor.testnz	p0, r6, r7;
		/* We lower only if SPM value different to default */
		p0	mov	r6, #USE_TRUE;
		!p0	mov	r6, #USE_FALSE;
		LOADTAPB(r3, r6);
#else
		LOADTAPB(r3);
#endif


#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
		/* 
			If the PB is the same as the 3D PB and it is a grow 
			we need to reload the 3D PB, as it would not have been done
			inside LOADTAPB
		*/
		xor.testz	p0, r3, r4;
		p0	and.testnz	p0, r5, #HWPBDESC_FLAGS_GROW;
		!p0	br	KTA_No3DPBReload;
		{
			/*
				The 3D PB would have been stored earlier, now reload it
				as there has been a grow.
			*/
			LOAD3DPB(r3);
		}
		KTA_No3DPBReload:
#endif
		/* Resume Allocations if required */
		RESUME(r4);

		ba	KTA_NoPBThresholdUpdate;
	}
	KTA_NoPBLoad:

	/* Check if there are any new DPM threshold values */
	and.testz	p0, r5, #HWPBDESC_FLAGS_THRESHOLD_UPDATE;
	p0	ba	 KTA_NoPBThresholdUpdate;
	{
#if defined(FIX_HW_BRN_32302) || defined(SUPPORT_SGX_DOUBLE_PIXEL_PARTITIONS)
		MK_MEM_ACCESS_CACHED(ldad)	r6, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32NumVertexPartitions)], drc1;
		MK_MEM_ACCESS_CACHED(ldad)	r7, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32SPMNumVertexPartitions)], drc1;
		wdf		drc1;
		xor.testnz	p0, r6, r7;
		p0	mov	r17, #USE_TRUE;
		!p0	mov	r17, #USE_FALSE;
#endif
		PVRSRV_SGXUTILS_CALL(DPMThresholdUpdate);
	}
	KTA_NoPBThresholdUpdate:

	/* Copy the TA registers from the command buffer to the hardware */
	mov		r2, #OFFSET(SGXMKIF_CMDTA.sTARegs);
	iaddu32	r2, r2.low, r1;

	MK_MEM_ACCESS_CACHED(ldad.f8)	r3, [r2, #0++], drc0;
	wdf		drc0;
	str		#EUR_CR_TE_AA >> 2, r3;
	str		#EUR_CR_TE_MTILE1 >> 2, r4;
	str		#EUR_CR_TE_MTILE2 >> 2, r5;
	str		#EUR_CR_TE_SCREEN >> 2, r6;
	str		#EUR_CR_TE_MTILE >> 2, r7;
	str		#EUR_CR_TE_PSG >> 2, r8;
	str		#EUR_CR_VDM_CTRL_STREAM_BASE >> 2, r9;
	str		#EUR_CR_MTE_CTRL >> 2, r10;
	COPYMEMTOCONFIGREGISTER_TA(KTA_TE_PSG, EUR_CR_TE_PSGREGION_BASE, r2, r3, r4, r5);
	COPYMEMTOCONFIGREGISTER_TA(KTA_TE_TPC, EUR_CR_TE_TPC_BASE, r2, r3, r4, r5);
#if defined(EUR_CR_MTE_FIXED_POINT)
	MK_MEM_ACCESS_CACHED(ldad.f7)	r3, [r2, #0++], drc0;
#else
	MK_MEM_ACCESS_CACHED(ldad.f6)	r3, [r2, #0++], drc0;
#endif
	wdf		drc0;
	str		#EUR_CR_MTE_WCOMPARE >> 2, r3;
	str		#EUR_CR_MTE_WCLAMP >> 2, r4;
	str		#EUR_CR_MTE_SCREEN >> 2, r5;
	str		#EUR_CR_USE_LD_REDIRECT >> 2, r6;
	str		#EUR_CR_USE_ST_RANGE >> 2, r7;
	str		#EUR_CR_BIF_TA_REQ_BASE >> 2, r8;
#if defined(EUR_CR_MTE_FIXED_POINT)
	str		#EUR_CR_MTE_FIXED_POINT >> 2, r9;
#endif

#if defined(SGX545)
	#if defined(FIX_HW_BRN_26915)
	MK_MEM_ACCESS_CACHED(ldad.f8)	r3, [r2, #0++], drc0;
	wdf		drc0;
	str		#EUR_CR_GSG >> 2, r3;
	str		#EUR_CR_GSG_WRAP >> 2, r4;
	str		#EUR_CR_GSG_STORE >> 2, r5;
	str		#EUR_CR_MTE_1ST_PHASE_COMPLEX_BASE >> 2, r6;
	str		#EUR_CR_MTE_1ST_PHASE_COMPLEX_BUFFER >> 2, r7;
	str		#EUR_CR_VDM_VTXBUF_WRPTR_PDSPROG >> 2, r8;
	str		#EUR_CR_VDM_SCWORDCP_PDSPROG >> 2, r9;
	str		#EUR_CR_VDM_ITP_PDSPROG >> 2, r10;
	#else /* #if defined(FIX_HW_BRN_26915) */
	MK_MEM_ACCESS_CACHED(ldad.f9)	r3, [r2, #0++], drc0;
	wdf		drc0;
	str		#EUR_CR_GSG >> 2, r3;
	str		#EUR_CR_GSG_BASE >> 2, r4;
	str		#EUR_CR_GSG_WRAP >> 2, r5;
	str		#EUR_CR_GSG_STORE >> 2, r6;
	str		#EUR_CR_MTE_1ST_PHASE_COMPLEX_BASE >> 2, r7;
	str		#EUR_CR_MTE_1ST_PHASE_COMPLEX_BUFFER >> 2, r8;
	str		#EUR_CR_VDM_VTXBUF_WRPTR_PDSPROG >> 2, r9;
	str		#EUR_CR_VDM_SCWORDCP_PDSPROG >> 2, r10;
	str		#EUR_CR_VDM_ITP_PDSPROG >> 2, r11;
	#endif /* #if !defined(FIX_HW_BRN_26915) */
	#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	MK_MEM_ACCESS_CACHED(ldad.f2)	r3, [r2, #0++], drc0;
	wdf		drc0;
	str		#EUR_CR_DPM_TA_RENDER_TARGET_MAX >> 2, r3;
	str		#EUR_CR_TE_TPC >> 2, r4;
	#endif
#endif
	MK_MEM_ACCESS_CACHED(ldad)	r3, [r2, #1++], drc0;
	wdf		drc0;
	str		#EUR_CR_MTE_MULTISAMPLECTL >> 2, r3;

	/* Set the vertex related part of the DMS control register. */
	ldr		r4, #EUR_CR_DMS_CTRL >> 2, drc0;

	MK_MEM_ACCESS_CACHED(ldad)	r5, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32NumVertexPartitions)], drc1;
	wdf	drc1;
#if defined(FIX_HW_BRN_32302) || defined(SUPPORT_SGX_DOUBLE_PIXEL_PARTITIONS)
	MK_MEM_ACCESS_CACHED(ldad)	r6, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32SPMNumVertexPartitions)], drc1;
	wdf		drc1;
	xor.testz	p3, r5, r6;
	/* Do nothing if SPM value and default value of
	MAX VP are the same
	*/
	p3	ba	KTA_SkipVPOverride;

	MK_LOAD_STATE(r2, ui32ITAFlags, drc0);
	/* this waits the ldr r4 as well*/
	wdf		drc0;
	mov		r7, #TA_IFLAGS_ZLSTHRESHOLD_LOWERED;
	and.testz       p3, r2, r7;
	p3      ba      KTA_Override;
	{
		ldr             r3, #EUR_CR_DPM_PAGE_STATUS >> 2, drc0;
		wdf             drc0;
		shr             r3, r3, #EUR_CR_DPM_PAGE_STATUS_TA_SHIFT;

		ldr             r2, #EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, drc0;
		wdf     drc0;

		isub16.tests p3, r3, r2;
		p3      ba      KTA_SkipVPOverride;
	}
	KTA_Override:

	mov	r5, r6;
	MK_TRACE(MKTC_KTAOVERRIDE_MAX_VTX_PARTITIONS, MKTF_TA | MKTF_SPM);
	KTA_SkipVPOverride:
	
#endif
	wdf		drc0;

	and		r4, r4, ~#EUR_CR_DMS_CTRL_MAX_NUM_VERTEX_PARTITIONS_MASK;
	or		r4, r4, r5;
	str		#EUR_CR_DMS_CTRL >> 2, r4;

	/* Ensure the correct memory context is setup */
	SETUPDIRLISTBASE(KTA, r0, #SETUP_BIF_TA_REQ);

	/* 	FIXME: optimize this section so the invalidates are only done when NOT
		done in SetupDirListBase */
	/* Check whether the driver has requested that we invalidate any caches */
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
	mov		r4, #SGXMKIF_TAFLAGS_INVALIDATEDATACACHE;
#if defined(SGX_FEATURE_SYSTEM_CACHE) && !defined(SGX_FEATURE_MP)
	/* Init R6 to USE_FALSE */
	mov		r6, #USE_FALSE;
#endif
	wdf		drc0;
	and.testz	p0, r2, r4;
	p0	ba	KTA_NoDrvDataCacheInvalidate;
	{
#if defined(FIX_HW_BRN_24549)
		/* Pause the USSE */
		mov 	r16, #(USE_IDLECORE_3D_REQ | USE_IDLECORE_USSEONLY_REQ);
		PVRSRV_SGXUTILS_CALL(IdleCore);
#endif

#if defined(EUR_CR_CACHE_CTRL)
		/* Invalidate the MADD cache */
		ldr		r4, #EUR_CR_CACHE_CTRL >> 2, drc0;
		wdf		drc0;
		or		r4, r4, #EUR_CR_CACHE_CTRL_INVALIDATE_MASK;
		str		#EUR_CR_CACHE_CTRL >> 2, r4;

		mov 	r5, #EUR_CR_EVENT_STATUS_MADD_CACHE_INVALCOMPLETE_MASK;
		ENTER_UNMATCHABLE_BLOCK;
		KTA_WaitForDrvMADDInvalidate:
		{
			ldr		r4, #EUR_CR_EVENT_STATUS >> 2, drc0;
			wdf		drc0;
			and.testz	p0, r4, r5;
			p0	ba		KTA_WaitForDrvMADDInvalidate;
		}
		LEAVE_UNMATCHABLE_BLOCK;

		mov		r5, #EUR_CR_EVENT_HOST_CLEAR_MADD_CACHE_INVALCOMPLETE_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r5;
#else
		/* Invalidate the data cache */
		str		#EUR_CR_DCU_ICTRL >> 2, #EUR_CR_DCU_ICTRL_INVALIDATE_MASK;

		mov 	r5, #EUR_CR_EVENT_STATUS2_DCU_INVALCOMPLETE_MASK;
		ENTER_UNMATCHABLE_BLOCK;
		KTA_WaitForDrvDCUInvalidate:
		{
			ldr		r4, #EUR_CR_EVENT_STATUS2 >> 2, drc0;
			wdf		drc0;
			and.testz	p0, r4, r5;
			p0	ba		KTA_WaitForDrvDCUInvalidate;
		}
		LEAVE_UNMATCHABLE_BLOCK;

		mov		r5, #EUR_CR_EVENT_HOST_CLEAR2_DCU_INVALCOMPLETE_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR2 >> 2, r5;

		/* Invalidate the texture cache */
		str		#EUR_CR_TCU_ICTRL >> 2, #EUR_CR_TCU_ICTRL_INVALTCU_MASK;

		mov 	r5, #EUR_CR_EVENT_STATUS_TCU_INVALCOMPLETE_MASK;
		ENTER_UNMATCHABLE_BLOCK;
		KTA_WaitForDrvTCUInvalidate:
		{
			ldr		r4, #EUR_CR_EVENT_STATUS >> 2, drc0;
			wdf		drc0;
			and.testz	p0, r4, r5;
			p0	ba		KTA_WaitForDrvTCUInvalidate;
		}
		LEAVE_UNMATCHABLE_BLOCK;

		mov		r5, #EUR_CR_EVENT_HOST_CLEAR_TCU_INVALCOMPLETE_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r5;
#endif

#if defined(FIX_HW_BRN_24549)
		RESUME(r4);
#endif
#if defined(SGX_FEATURE_WRITEBACK_DCU)
	/* Invalidate the DCU cache. */
	ci.global	#SGX545_USE_OTHER2_CFI_DATAMASTER_VERTEX, #0, #SGX545_USE1_OTHER2_CFI_LEVEL_L0L1L2;
#endif
#if defined(SGX_FEATURE_SYSTEM_CACHE) && !defined(SGX_FEATURE_MP)
		/* indicate that the SLC needs invalidating */
		mov		r6, #USE_TRUE;
#endif
	}
	KTA_NoDrvDataCacheInvalidate:

	mov		r4, #SGXMKIF_TAFLAGS_INVALIDATEPDSCACHE;
	and.testz	p0, r2, r4;
	p0	ba	KTA_NoDrvPDSDataFlush;
	{
		/* Invalidate the PDS cache */
		str		#EUR_CR_PDS_INV0 >> 2, #EUR_CR_PDS_INV0_DSC_MASK;
		INVALIDATE_PDS_CODE_CACHE();

		ENTER_UNMATCHABLE_BLOCK;
		KTA_WaitForDrvPDSDataFlush:
		{
			ldr		r4, #EUR_CR_PDS_CACHE_STATUS >> 2, drc0;
			wdf		drc0;

			and		r4, r4, #EURASIA_PDS_CACHE_VERTEX_STATUS_MASK;
			xor.testnz	p0, r4, #EURASIA_PDS_CACHE_VERTEX_STATUS_MASK;
			p0	ba		KTA_WaitForDrvPDSDataFlush;
		}
		LEAVE_UNMATCHABLE_BLOCK;

		str		#EUR_CR_PDS_CACHE_HOST_CLEAR >> 2, #EURASIA_PDS_CACHE_VERTEX_CLEAR_MASK;

#if defined(SGX_FEATURE_SYSTEM_CACHE) && !defined(SGX_FEATURE_MP)
		/* indicate that the SLC needs invalidating */
		mov		r6, #USE_TRUE;
#endif
	}
	KTA_NoDrvPDSDataFlush:

	and.testz	p0, r2, #SGXMKIF_TAFLAGS_INVALIDATEUSECACHE;
	p0	ba	KTA_NoDrvUSECacheInvalidate;
	{
		INVALIDATE_USE_CACHE(KTA_, p0, r3, #SETUP_BIF_TA_REQ);
#if defined(SGX_FEATURE_SYSTEM_CACHE) && !defined(SGX_FEATURE_MP)
		/* indicate that the SLC needs invalidating */
		mov		r6, #USE_TRUE;
#endif
	}
	KTA_NoDrvUSECacheInvalidate:

#if defined(SGX_FEATURE_SYSTEM_CACHE)
	#if defined(SGX_FEATURE_MP)
	INVALIDATE_SYSTEM_CACHE(#EUR_CR_MASTER_SLC_CTRL_INVAL_DM_VERTEX_MASK, #EUR_CR_MASTER_SLC_CTRL_FLUSH_DM_VERTEX_MASK);
	#else
	/* check whether the SLC needs invalidating */
	xor.testnz	p0, r6, #USE_TRUE;
	p0	ba	KTA_NoSLCInval;
	{
		INVALIDATE_SYSTEM_CACHE();
	}
	KTA_NoSLCInval:
	#endif /* SGX_FEATURE_MP */
#endif

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
	INVALIDATE_EXT_SYSTEM_CACHE();
#endif /* SUPPORT_EXTERNAL_SYSTEM_CACHE */

#if defined(UNDER_VISTA_BASIC)
	/* Check if we need to do any splitting */
	ba	SplitControlStream;
	SplitControlStreamReturn:
#endif

	/* Decide which context should be used by the TA */
	MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext0RTData)], drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r4, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext1RTData)], drc1;
	mov		r5, #0 << EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_SHIFT;
	wdf		drc0;
	wdf		drc1;

	xor.testz	p1, R_RTData, r3;
	p1	ba		KTA_ContextChosen;
	{
		xor.testz	p1, R_RTData, r4;
		p1	mov		r5, #1 << EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_SHIFT;
		p1	mov		r3, r4;
		p1	ba		KTA_ContextChosen;
		{
			and.testz	p1, r3, r3;
			p1	ba		KTA_ContextChosen;
			{
				and.testz	p1, r4, r4;
				p1	mov		r5, #1 << EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_SHIFT;
				p1	mov		r3, r4;
				p1	ba		KTA_ContextChosen;
				{
					/* Choose context not being used by 3D */
					ldr		r5, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc0;
					wdf		drc0;
					and		r5, r5, #EUR_CR_DPM_STATE_CONTEXT_ID_DALLOC_MASK;
					shr		r5, r5, #EUR_CR_DPM_STATE_CONTEXT_ID_DALLOC_SHIFT;
					xor		r5, r5, #1;
					and.testnz	p1, r5, #1;
					shl		r5, r5, #EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_SHIFT;
					p1	mov		r3, r4;
				}
			}
		}
	}
	KTA_ContextChosen:

	/* Save which context is being used */
	and.testz	p1, r5, #1 << EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_SHIFT;
	p1	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext0RTData)], R_RTData;
	!p1	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext1RTData)], R_RTData;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], R_RTData;
	idf		drc0, st;
	wdf		drc0;

	/* Set context alloc ID */
	ldr		r2, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc0;
	wdf		drc0;
	and		r2, r2, #~EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_MASK;
	or		r2, r2, r5;
	str		#EUR_CR_DPM_STATE_CONTEXT_ID >> 2, r2;

	/* 	Convert context alloc ID to load/store ID as this is all that is
		required from now on */
	shr		r5, r5, #EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_SHIFT;

	/* If there's something using the chosen context we need to store it first */
	and.testz	p1, r3, r3;
	!p1	xor.testz	p1, r3, R_RTData;
	p1	ba		KTA_NoContextStore2;
	{
		/* find out the RenderContext of the RTDATA */
		MK_MEM_ACCESS(ldad)	r4, [r3, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderContextDevAddr)], drc0;
		wdf		drc0;

		/* Set up the TA for the memory context to be stored. */
		SETUPDIRLISTBASE(KTA_STAC2, r4, #SETUP_BIF_TA_REQ);

		/* Store the DPM state via the TA Index */
		STORETACONTEXT(r3, r5);

		/* Set up the TA for the memory context of this kick. */
		SETUPDIRLISTBASE(KTA_STAC2_RESTORE, r0, #SETUP_BIF_TA_REQ);
	}
	KTA_NoContextStore2:

#if defined(FIX_HW_BRN_33657) && defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc0;
	wdf		drc0;
	/* Clear VDM context store bit on this kick */
	and		r2, r2, ~#SGXMKIF_HWRTDATA_RT_STATUS_VDMCTXSTORE;
	MK_MEM_ACCESS(stad)	[R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], r2;
#endif

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	MK_TRACE(MKTC_KICKTA_CHKPT_CHECK_SWITCH,  MKTF_TA | MKTF_CSW);
	/* Check to see if this TA has been context switched */
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
	/* Ensure the MTE 1st phase complex ptr is 0, incase it is not a resume */
	mov		r4, #SGXMKIF_TAFLAGS_VDM_CTX_SWITCH;
	wdf		drc0;
	and.testz	p0, r2, r4;
	p0	ba	KTA_NoVDMResume;
	{
		mov		r9, r5;
		mov		r10, pclink;
		
		/* Reload the previously stored VDM state */
		bal		ResumeTA;
		
		mov		pclink, r10;
		mov		r5, r9;
		ba	KTA_ContextLoad;
	}
	KTA_NoVDMResume:
	#if defined(SGX_FEATURE_COMPLEX_GEOMETRY_REV_2)
	str		#EUR_CR_MTE_1ST_PHASE_COMPLEX_RESTORE >> 2, #0;
	#endif
#else
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
	wdf		drc0;
#endif
	{
		mov	r4, #(SGXMKIF_TAFLAGS_FIRSTKICK | SGXMKIF_TAFLAGS_RESUMEDAFTERFREE);
		/* If this is the first kick then reset the context. */
		and.testnz	p0, r2, r4;
		p0	ba		KTA_NoContextLoad;

		/* If this context is loaded then skip loading it. */
		xor.testz	p0, R_RTData, r3;
		p0	ba		KTA_NoContextReset;
	}
	KTA_ContextLoad:
	{
		/* Make sure the correct memory context is setup */
		SETUPDIRLISTBASE(KTA_LTAC, r0, #SETUP_BIF_TA_REQ);

		/* Load the DPM state via the TA Index */
		LOADTACONTEXT(R_RTData, r5);

		ba		KTA_NoContextReset;
	}
	KTA_NoContextLoad:
	{
		/*
			This is the first kick since 3D parameters were deallocated -
			reset OTPM state plus state and control tables
		*/
		MK_TRACE(MKTC_KICKTA_RESETCONTEXT, MKTF_TA | MKTF_SCH);

#if defined(SGX_FEATURE_MP)
		/* Reset the PIM values in both the VDM and the DPM */
		WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_PIM >> 2, #EUR_CR_MASTER_VDM_PIM_CLEAR_MASK, r2);
		WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_TASK_DPLIST >> 2, \
							#EUR_CR_MASTER_DPM_TASK_DPLIST_CLEAR_MASK, r2);
#endif /* SGX_FEATURE_MP */

#if defined(EUR_CR_TE_RGNHDR_INIT)
		str		#EUR_CR_TE_RGNHDR_INIT >> 2, #EUR_CR_TE_RGNHDR_INIT_PULSE_MASK;
#endif /* EUR_CR_TE_RGNHDR_INIT */

		ldr		r2, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc0;
		wdf		drc0;
		and		r2, r2, #~EUR_CR_DPM_STATE_CONTEXT_ID_LS_MASK;
#if defined(EUR_CR_DPM_STATE_CONTEXT_ID_NCOP_MASK)
		/* Set the NCOP bit as we also want to clear the NC list */
		or		r2, r2, #EUR_CR_DPM_STATE_CONTEXT_ID_NCOP_MASK;
#endif
		/* EUR_CR_DPM_STATE_CONTEXT_ID_LS field is bit 0 so no shift required */
		or		r2, r2, r5;
		str		#EUR_CR_DPM_STATE_CONTEXT_ID >> 2, r2;

		/* Idle the 3D as it could be deallocating memory */
		IDLE3D();

#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
		/* Setup base and clear the DPM state table */
		MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sContextStateDevAddr)], drc0;
		//FIXME-MP loop by SGXMK_STATE.ui32CoresEnabled;
		MK_MEM_ACCESS(ldad)	r3, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sContextControlDevAddr[0])], drc1;
		wdf		drc0;
		SETUPSTATETABLEBASE(KTA, p0, r2, r5);
		wdf		drc1;
		str		#EUR_CR_DPM_CONTROL_TABLE_BASE >> 2, r3;
		MK_MEM_ACCESS(ldad)	r4, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32NumRTsInArray)], drc0;
		wdf		drc0;

	#if defined(FIX_HW_BRN_29798)
		/* Find out which render target is currently on-chip */
		ldr		r6, #EUR_CR_DPM_STATE_RENTARGET >> 2, drc0;
		and.testz	p0, r5, r5;
		wdf		drc0;
		p0	and	r6, r6, #EUR_CR_DPM_STATE_RENTARGET_CONTEXT0_STATUS_MASK;
		!p0	and	r6, r6, #EUR_CR_DPM_STATE_RENTARGET_CONTEXT1_STATUS_MASK;
		
		/* If the on-chip RT is not 0, CLEAR operation will clear the memory */
		and.testnz	p0, r6, r6;
		p0	ba	KTA_NotRT0OnChip;
		{
			/* RT 0 is on-chip therefore do the following:
				1) clear on-chip using CLEAR operation
				2) 	for each RT
					{
						use STORE operation to clear memory location
						advance address by state table size
					}				
			*/
			/* Clear the on-chip memory */
			PVRSRV_SGXUTILS_CALL(DPMStateClear);
			
			str		#EUR_CR_DPM_TASK_CONTROL >> 2, #EUR_CR_DPM_TASK_CONTROL_CLEAR_MASK;
			
			/* If there is only 1 RT then exit only clear on-chip */
			xor.testz	p0, r4, #1;
			p0	mov	r3, #EUR_CR_EVENT_STATUS_DPM_CONTROL_CLEAR_MASK;
			p0	ba	KTA_ContextCleared;
			{
				ENTER_UNMATCHABLE_BLOCK;
				KTA_WaitForCtrlClear:
				{
					ldr		r6, #EUR_CR_EVENT_STATUS >> 2, drc0;
					wdf		drc0;
					shl.testns	p0, r6, #(31 - EUR_CR_EVENT_STATUS_DPM_CONTROL_CLEAR_SHIFT);
					p0	ba		KTA_WaitForCtrlClear;
				}
				LEAVE_UNMATCHABLE_BLOCK;
	
				/* Bits are in the same position for host_clear and event_status */
				mov		r6, #EUR_CR_EVENT_HOST_CLEAR_DPM_CONTROL_CLEAR_MASK;
				str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r6;
				
				/* This is a RTA */
				KTA_StoreClearToNextRT:
				{
					/* Increment the addresses */
					REG_INCREMENT_BYVALUE(r2, p0, #EURASIA_PARAM_MANAGE_STATE_SIZE / 2);
					REG_INCREMENT_BYVALUE(r2, p0, #EURASIA_PARAM_MANAGE_STATE_SIZE / 2);
					REG_INCREMENT_BYVALUE(r3, p0, #EURASIA_PARAM_MANAGE_CONTROL_SIZE);
					
					SETUPSTATETABLEBASE(KTA_StoreClear, p0, r2, r5);
					str		#EUR_CR_DPM_CONTROL_TABLE_BASE >> 2, r3;
		
					/* Store the clear state and control tables to memory */
					str		#EUR_CR_DPM_TASK_STATE >> 2, #EUR_CR_DPM_TASK_STATE_STORE_MASK;
					str		#EUR_CR_DPM_TASK_CONTROL >> 2, #EUR_CR_DPM_TASK_CONTROL_STORE_MASK;
					
					mov	r7, #(EUR_CR_EVENT_STATUS_DPM_STATE_STORE_MASK | \
				      			EUR_CR_EVENT_STATUS_DPM_CONTROL_STORE_MASK);

					ENTER_UNMATCHABLE_BLOCK;
					KTA_WaitForDPMStore:
					{
						ldr		r6, #EUR_CR_EVENT_STATUS >> 2, drc0;
						wdf		drc0;
						and		r6, r6, r7;
						xor.testnz	p0, r6, r7;
						p0	ba		KTA_WaitForDPMStore;
					}
					LEAVE_UNMATCHABLE_BLOCK;
		
					/* Bits are in the same position for host_clear and event_status */
					str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r7;
				
#if defined(FIX_HW_BRN_31543)
					str		#EUR_CR_DPM_TASK_CONTROL >> 2, #EUR_CR_DPM_TASK_CONTROL_LOAD_MASK;

					mov		r7, #(EUR_CR_EVENT_STATUS_DPM_CONTROL_LOAD_MASK);

					ENTER_UNMATCHABLE_BLOCK;
					KTA_WaitForDPMLoad:
					{
						ldr		r6, #EUR_CR_EVENT_STATUS >> 2, drc0;
						wdf		drc0;
						and		r6, r6, r7;
						xor.testnz	p0, r6, r7;
						p0	ba		KTA_WaitForDPMLoad;
					}
					LEAVE_UNMATCHABLE_BLOCK;

					/* Bits are in the same position for host_clear and event_status */
					str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r7;
#endif	

					/* 
						If this is an odd render target we need to clear the global 
						entries manually
					*/
					and.testz	p0, r2, #((1 << EUR_CR_DPM_STATE_TABLE_CONTEXT0_BASE_ADDR_SHIFT) - 1);
					p0	ba	KTA_StateAddrAligned;
					{
						/* Do a store to clear the global list */
						stad.f2.bpcache	[r2, #(32)-1], #0;
						idf	drc1, st;
					}
					KTA_StateAddrAligned:
									
					isub16.testnz	r4, p0, r4, #1;
					p0	ba	KTA_StoreClearToNextRT;
				}
				
				ba KTA_ResetDPMBaseAddr;
			}
		}
		KTA_NotRT0OnChip:
		{
			/* RT 0 is not on-chip therefore do the following:
				1) 	for each RT
					{
						use CLEAR operation to clear memory location
						advance address by state table size
					}
				2) use LOAD operation to clear on chip		
			*/
			KTA_ClearNextRTMem:
			{
				/* Clear the state and control tables in memory */
				str		#EUR_CR_DPM_TASK_STATE >> 2, #EUR_CR_DPM_TASK_STATE_CLEAR_MASK;
				str		#EUR_CR_DPM_TASK_CONTROL >> 2, #EUR_CR_DPM_TASK_CONTROL_CLEAR_MASK;
	
				mov	r7, #(EUR_CR_EVENT_STATUS_DPM_STATE_CLEAR_MASK | \
			      			EUR_CR_EVENT_STATUS_DPM_CONTROL_CLEAR_MASK);
				ENTER_UNMATCHABLE_BLOCK;
				KTA_WaitForDPMClear:
				{
					ldr		r6, #EUR_CR_EVENT_STATUS >> 2, drc0;
					wdf		drc0;
					and		r6, r6, r7;
					xor.testnz	p0, r6, r7;
					p0	ba		KTA_WaitForDPMClear;
				}
				LEAVE_UNMATCHABLE_BLOCK;
	
				/* Bits are in the same position for host_clear and event_status */
				str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r7;
				
				/* 
					If this is an odd render target we need to clear the global 
					entries manually
				*/
				and.testz	p0, r2, #((1 << EUR_CR_DPM_STATE_TABLE_CONTEXT0_BASE_ADDR_SHIFT) - 1);
				p0	ba	KTA_StateAddrAligned2;
				{
					/* Do a store to clear the global list */
					stad.f2.bpcache	[r2, #(32)-1], #0;
					idf	drc1, st;
				}
				KTA_StateAddrAligned2:
									
				isub16.testz	r4, p0, r4, #1;
				p0	ba	KTA_LoadRT0DPMState;
				{
					/* Increment the addresses */
					REG_INCREMENT_BYVALUE(r2, p0, #EURASIA_PARAM_MANAGE_STATE_SIZE / 2);
					REG_INCREMENT_BYVALUE(r2, p0, #EURASIA_PARAM_MANAGE_STATE_SIZE / 2);
					REG_INCREMENT_BYVALUE(r3, p0, #EURASIA_PARAM_MANAGE_CONTROL_SIZE);	

					SETUPSTATETABLEBASE(KTA_ClearRT, p0, r2, r5);
					str		#EUR_CR_DPM_CONTROL_TABLE_BASE >> 2, r3;
					
					ba	KTA_ClearNextRTMem;
				}
			}
			KTA_LoadRT0DPMState:
			/* Wait for store to finish */
			wdf		drc1;
			
			/* Load RT0 */
			/* Return the base address to the correct values */
			MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sContextStateDevAddr)], drc0;
			//FIXME-MP loop by SGXMK_STATE.ui32CoresEnabled;
			MK_MEM_ACCESS(ldad)	r3, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sContextControlDevAddr[0])], drc1;
			wdf		drc0;
			SETUPSTATETABLEBASE(KTA_LoadRT0, p0, r2, r5);
			wdf		drc1;
			str		#EUR_CR_DPM_CONTROL_TABLE_BASE >> 2, r3;			
		
			/* Clear the state and control tables in memory */
			str		#EUR_CR_DPM_TASK_STATE >> 2, #EUR_CR_DPM_TASK_STATE_LOAD_MASK;
			str		#EUR_CR_DPM_TASK_CONTROL >> 2, #EUR_CR_DPM_TASK_CONTROL_LOAD_MASK;

			/* We still have to wait for the loads to complete */
			mov	r3, #(EUR_CR_EVENT_STATUS_DPM_STATE_LOAD_MASK | \
		      			EUR_CR_EVENT_STATUS_DPM_CONTROL_LOAD_MASK);
			ba	KTA_ContextCleared;
		}
		KTA_ResetDPMBaseAddr:
		
		/* Return the base address to the correct values */
		MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sContextStateDevAddr)], drc0;
		//FIXME-MP loop by SGXMK_STATE.ui32CoresEnabled;
		MK_MEM_ACCESS(ldad)	r3, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sContextControlDevAddr[0])], drc1;
		wdf		drc0;
		SETUPSTATETABLEBASE(KTA_ResetBase, p0, r2, r5);
		wdf		drc1;
		str		#EUR_CR_DPM_CONTROL_TABLE_BASE >> 2, r3;
		/* Nothing left to wait for, so initialise r3 to 0 */
		mov		r3, #0;
		
		KTA_ContextCleared:
	#else
		/* same bit used for clear in STATE and CONTROL registers */
		mov	r3, #EUR_CR_DPM_TASK_STATE_CLEAR_MASK;
		/* setup the mask, so we know when to finish */
		shl	r4, r4, #EUR_CR_DPM_TASK_CONTROL_RENTARGETID_SHIFT;
		or	r4, r4, #EUR_CR_DPM_TASK_STATE_CLEAR_MASK;

		/* Setup the mask */
		mov	r6, #(EUR_CR_EVENT_STATUS_DPM_STATE_CLEAR_MASK | \
				      EUR_CR_EVENT_STATUS_DPM_CONTROL_CLEAR_MASK);
		KTA_ClearNextRenderTarget:
		{
			str		#EUR_CR_DPM_TASK_STATE >> 2, r3;
			str		#EUR_CR_DPM_TASK_CONTROL >> 2, r3;

			ENTER_UNMATCHABLE_BLOCK;
			KTA_WaitForDPMClear:
			{
				ldr		r2, #EUR_CR_EVENT_STATUS >> 2, drc0;
				wdf		drc0;
				and		r2, r2, r6;
				xor.testnz	p0, r2, r6;
				p0	ba		KTA_WaitForDPMClear;
			}
			LEAVE_UNMATCHABLE_BLOCK;

			/* Bits are in the same position for host_clear and event_status */
			str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r6;

			/* add 1 to RT ID */
			mov	r2, #(1 << EUR_CR_DPM_TASK_CONTROL_RENTARGETID_SHIFT);
			imae	r3, r2.low, #1, r3, u32;
			/* Have we finished clearing everything we need to? */
			xor.testnz	p0, r3, r4;
			p0	ba	KTA_ClearNextRenderTarget;
		}
		
		/* if RT0 is not on chip, do a store to flush the RT on chip to memory */
		ldr r2, #EUR_CR_DPM_STATE_RENTARGET >> 2, drc0;
		wdf drc0;
		and.testnz	p0, r5, r5;
		p0	shr	r2, r2, #EUR_CR_DPM_STATE_RENTARGET_CONTEXT1_STATUS_SHIFT;
		and	r2, r2, #EUR_CR_DPM_STATE_RENTARGET_CONTEXT0_STATUS_MASK;
		and.testz	p0, r2, r2;
		p0	ba KTA_NoOnChipRTFlush;
		{
			/* same bit used for store in STATE and CONTROL registers */
			shl	r2, r2, #EUR_CR_DPM_TASK_CONTROL_RENTARGETID_SHIFT;
			or	r3, r2, #EUR_CR_DPM_TASK_STATE_STORE_MASK;
			
			str		#EUR_CR_DPM_TASK_STATE >> 2, r3;
			str		#EUR_CR_DPM_TASK_CONTROL >> 2, r3;

			mov	r6, #(EUR_CR_EVENT_STATUS_DPM_STATE_STORE_MASK | \
				      EUR_CR_EVENT_STATUS_DPM_CONTROL_STORE_MASK);
		
			ENTER_UNMATCHABLE_BLOCK;
			KTA_WaitForOnChipStateStore:
			{
				ldr		r2, #EUR_CR_EVENT_STATUS >> 2, drc0;
				wdf		drc0;
				and		r2, r2, r6;
				xor.testnz	p0, r2, r6;
				p0	ba		KTA_WaitForOnChipStateStore;
			}
			LEAVE_UNMATCHABLE_BLOCK;

			/* Bits are in the same position for host_clear and event_status */
			str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r6;
		}
		KTA_NoOnChipRTFlush:
		
		/* There is nothing oustanding to wait for */
		mov		r3, #0;		
	#endif
	
	ldr r2, #EUR_CR_DPM_STATE_RENTARGET >> 2, drc0;
	wdf drc0;
	and.testnz	p0, r5, r5;
	p0	shr	r2, r2, #EUR_CR_DPM_STATE_RENTARGET_CONTEXT1_STATUS_SHIFT;
	and	r2, r2, #EUR_CR_DPM_STATE_RENTARGET_CONTEXT0_STATUS_MASK;
	and.testz	p0, r2, r2;
	p0 ba KTA_NoRT0Load;
	{
		/* Make sure that RT0 is on-chip so that data is not flush incorrectly later */
		str		#EUR_CR_DPM_TASK_STATE >> 2, #EUR_CR_DPM_TASK_STATE_LOAD_MASK;
		str		#EUR_CR_DPM_TASK_CONTROL >> 2, #EUR_CR_DPM_TASK_CONTROL_LOAD_MASK;

		/* Setup the poll mask */
		mov		r3, #(EUR_CR_EVENT_STATUS_DPM_STATE_LOAD_MASK | \
					  EUR_CR_EVENT_STATUS_DPM_CONTROL_LOAD_MASK);
	}
	KTA_NoRT0Load:
	
#else /* SGX_FEATURE_RENDER_TARGET_ARRAYS */
	#if defined(FIX_HW_BRN_27311) && !defined(FIX_HW_BRN_23055)
		MK_MEM_ACCESS(ldad)	r17, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPBDescDevVAddr.uiAddr)], drc0;
		mov		r16, R_RTData;
		wdf		drc0;
		PVRSRV_SGXUTILS_CALL(InitBRN27311StateTable);
		/* Setup base and clear the DPM state table */
		MK_MEM_ACCESS(ldad)	r3, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sContextStateDevAddr)], drc0;
		wdf		drc0;
		SETUPSTATETABLEBASE(KTA_BRN27311, p0, r3, r5);
		str		#EUR_CR_DPM_TASK_STATE >> 2, #EUR_CR_DPM_TASK_STATE_LOAD_MASK;
		mov		r3, #(EUR_CR_EVENT_STATUS_DPM_STATE_LOAD_MASK | \
				      EUR_CR_EVENT_STATUS_DPM_CONTROL_CLEAR_MASK);
	#else
		str		#EUR_CR_DPM_TASK_STATE >> 2, #EUR_CR_DPM_TASK_STATE_CLEAR_MASK;
		mov		r3, #(EUR_CR_EVENT_STATUS_DPM_STATE_CLEAR_MASK | \
				      EUR_CR_EVENT_STATUS_DPM_CONTROL_CLEAR_MASK);
	#endif
		str		#EUR_CR_DPM_TASK_CONTROL >> 2, #EUR_CR_DPM_TASK_CONTROL_CLEAR_MASK;
#endif /* SGX_FEATURE_RENDER_TARGET_ARRAYS */

#if defined(EUR_CR_MTE_OTPM_CSM_BASE)
		/*
			for SGX545 the CSM is true cache and will access data from memory
			as required.  As a consequence the load base address must be valid
			at all times, not just at context load time, i.e. includes first
			kick of a new scene
		*/
		MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sContextOTPMDevAddr[0])], drc0;
		wdf		drc0;
		str		#EUR_CR_MTE_OTPM_CSM_BASE >> 2, r2;
		
	#if defined(FIX_HW_BRN_29823)
		/* Before we can clear the memory we must get HW to pick up the MAX RT ID */
		MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sDummyTermDevAddr)], drc0;
		ldr		r4, #EUR_CR_VDM_CTRL_STREAM_BASE >> 2, drc0;
		wdf		drc0;
		str		#EUR_CR_VDM_CTRL_STREAM_BASE >> 2, r2;
		
		/* Start the VDM */
		str		#EUR_CR_VDM_START >> 2, #EUR_CR_VDM_START_PULSE_MASK;
		
		/* Wait for the VDM to IDLE */
		mov		r2, #EUR_CR_VDM_IDLE_STATUS_MASK;
		ENTER_UNMATCHABLE_BLOCK;
		KTA_WaitForVDMIdle:
		{
			ldr 	r5, #EUR_CR_VDM_IDLE >> 2, drc0;
			wdf		drc0;
			xor.testnz	p0, r5, r2;
			p0	ba	KTA_WaitForVDMIdle;
		}
		LEAVE_UNMATCHABLE_BLOCK;
		
		/* Now we can change the address back and do clear CSM memory */
		str		#EUR_CR_VDM_CTRL_STREAM_BASE >> 2, r4;
	#endif

		/* Clear the context of the CSM memory and invalidate internal cache */
		str		#EUR_CR_MTE_OTPM_OP >> 2, #(EUR_CR_MTE_OTPM_OP_CSM_OTPM_MEM_CLEAR_MASK | \
											EUR_CR_MTE_OTPM_OP_CSM_INV_MASK);
#else
		str		#EUR_CR_MTE_OTPM_OP >> 2, #EUR_CR_MTE_OTPM_OP_CSM_INV_MASK;
#endif /* EUR_CR_MTE_OTPM_CSM_BASE */
		or		r3, r3, #EUR_CR_EVENT_STATUS_OTPM_INV_MASK;

		ENTER_UNMATCHABLE_BLOCK;
		KTA_WaitForContextReset:
		{
			ldr		r2, #EUR_CR_EVENT_STATUS >> 2, drc0;
			wdf		drc0;
			and		r2, r2, r3;
			xor.testnz	p0, r2, r3;
			p0	ba		KTA_WaitForContextReset;
		}
		LEAVE_UNMATCHABLE_BLOCK;

		RESUME_3D(r2);
		/* event_status bits match event_host_clear bits */
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r3;

#if defined(EUR_CR_TE_RGNHDR_INIT) || defined(EUR_CR_EVENT_STATUS2_OTPM_MEM_CLEARED_MASK)
		mov		r3, #0;
		mov		r4, #0;
	#if defined(EUR_CR_TE_RGNHDR_INIT)
		or	r3, r3, #EUR_CR_EVENT_STATUS2_TE_RGNHDR_INIT_COMPLETE_MASK;
		or	r4, r4, #EUR_CR_EVENT_HOST_CLEAR2_TE_RGNHDR_INIT_COMPLETE_MASK;
	#endif /* EUR_CR_TE_RGNHDR_INIT */

	#if defined(EUR_CR_EVENT_STATUS2_OTPM_MEM_CLEARED_MASK)
		or	r3, r3, #EUR_CR_EVENT_STATUS2_OTPM_MEM_CLEARED_MASK;
		or	r4, r4, #EUR_CR_EVENT_HOST_CLEAR2_OTPM_MEM_CLEARED_MASK;
	#endif /* EUR_CR_EVENT_STATUS2_OTPM_MEM_CLEARED_MASK */

		ENTER_UNMATCHABLE_BLOCK;
		KTA_WaitForEventStatus2:
		{
			ldr		r2, #EUR_CR_EVENT_STATUS2 >> 2, drc0;
			wdf		drc0;
			and			r2, r2, r3;
			xor.testnz	p0, r2, r3;
			p0	ba		KTA_WaitForEventStatus2;
		}
		LEAVE_UNMATCHABLE_BLOCK;

		str		#EUR_CR_EVENT_HOST_CLEAR2 >> 2, r4;
#endif

#if defined(SGX_FEATURE_MP)
		/*
			Poll for the VDM PIM cleared event.
			The cleared bit will go low again when we start the VDM below.
		*/
		mov	r4, #EUR_CR_MASTER_VDM_PIM_STATUS >> 2;
		ENTER_UNMATCHABLE_BLOCK;
		KTA_WaitForVDMPIMClear:
		{
			ldr			r2, r4, drc0;
			wdf			drc0;
			and.testz	p0, r2, #EUR_CR_MASTER_VDM_PIM_STATUS_CLEARED_MASK;
			p0	ba		KTA_WaitForVDMPIMClear;
		}
		LEAVE_UNMATCHABLE_BLOCK;
		
		/* Poll for the DPM PIM cleared event. */
		mov	r4, #EUR_CR_MASTER_DPM_DPLIST_EVENT_STATUS >> 2;
		ENTER_UNMATCHABLE_BLOCK;
		KTA_WaitForDPMPIMClear:
		{
			ldr			r2, r4, drc0;
			wdf			drc0;
			and.testz	p0, r2, #EUR_CR_MASTER_DPM_DPLIST_EVENT_STATUS_CLEARED_MASK;
			p0	ba		KTA_WaitForDPMPIMClear;
		}
		LEAVE_UNMATCHABLE_BLOCK;		
		
		mov	r2, #EUR_CR_MASTER_DPM_DPLIST_CLEAR_EVENT >> 2;
		str	r2, #EUR_CR_MASTER_DPM_DPLIST_CLEAR_EVENT_CLEAR_MASK;
		
	#if defined(EUR_CR_MASTER_DPM_MTILE_ABORTED) && defined(EUR_CR_DPM_MTILE_ABORTED)
		READMASTERCONFIG(r3, #EUR_CR_MASTER_DPM_MTILE_ABORTED >> 2, drc0);
		wdf		drc0;
		str		#EUR_CR_DPM_MTILE_ABORTED >> 2, r3;
	#endif
#endif /* SGX_FEATURE_MP */
	}
	KTA_NoContextReset:


	/* Load the TE state. */
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderDetails)], drc0;
	mov				r4, #OFFSET(SGXMKIF_HWRENDERDETAILS.ui32TEState[0]);
	wdf				drc0;
	iaddu32			r3, r4.low, r2;
	COPYMEMTOCONFIGREGISTER_TA(KTA_TEState, EUR_CR_TE_STATE, r3, r4, r5, r6);

	MK_MEM_ACCESS(ldad)	r3, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc0;
	ldr		r4, #EUR_CR_TE_PSG >> 2, drc0;
	wdf		drc0;
	or		r4, r4, #EUR_CR_TE_PSG_ENABLE_CONTEXT_STATE_RESTORE_MASK;
	shl.tests	p0, r3, #(31 - SGXMKIF_HWRTDATA_CMN_STATUS_ENABLE_ZLOAD_SHIFT);
	p0	or	r4, r4, #EUR_CR_TE_PSG_ZLOADENABLE_MASK;
	str		#EUR_CR_TE_PSG >> 2, r4;

#if defined(SUPPORT_HW_RECOVERY)
	/* Reset the recovery counter */
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRTimerTicksSinceTA)], #0;

	/* invalidate the signatures */
	MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], drc0;
	wdf		drc0;
	or		r3, r3, #0x1;
	MK_MEM_ACCESS_BPCACHE(stad) 	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], r3;
#endif

	/* Check if status needs updating */
	MK_MEM_ACCESS_CACHED(ldad)	r3, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
	wdf		drc0;
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	/* If this TA command has been context switched the state should not be reset */
	mov	r4, #SGXMKIF_TAFLAGS_VDM_CTX_SWITCH;
	and.testnz	p0, r3, r4;
	p0	ba	KTA_DontResetStatus;
#endif
	{	
	#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && (defined(FIX_HW_BRN_30893) || defined(FIX_HW_BRN_30970))
		/* Clear the VDM Context Store status, as we haven't stored yet on this command */
		MK_TRACE(MKTC_KICKTA_RESET_VDMCSSTATUS, MKTF_TA | MKTF_SCH | MKTF_CSW);
		
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32MasterVDMCtxStoreStatus)], #0;

		MK_LOAD_STATE_CORE_COUNT_TA(r5, drc0);
		mov		r7, #OFFSET(SGXMKIF_HWRENDERCONTEXT.ui32VDMCtxStoreStatus[0]);
		iaddu32	r6, r7.low, r0;
		MK_WAIT_FOR_STATE_LOAD(drc0);

		mov		r4, #0;	/* core counter */
			
		KTA_ClearNextCoreVDMCtxStore:
		{
			/* reset the slave core state */
			MK_MEM_ACCESS(stad)	[r6, #1++], #0;
			REG_INCREMENT(r4, p0);
			xor.testnz	p0, r4, r5;
			p0	ba	KTA_ClearNextCoreVDMCtxStore;
		}
		/* Fence the stores here in case we jump to KTA_DontResetStatus */
		idf		drc0, st;
		wdf		drc0;
	#endif /* SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH && (FIX_HW_BRN_30893 || FIX_HW_BRN_30970)*/

		mov		r4, #(SGXMKIF_TAFLAGS_FIRSTKICK | SGXMKIF_TAFLAGS_RESUMEMIDSCENE);
		and.testz	p0, r3, r4;
		p0	ba		KTA_DontResetStatus;
		{
			/* Clear the Status Bits */
			/* Clear all the RTs status bits */
		#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
			/* Clear the Common Status Bits */
			MK_MEM_ACCESS(ldad)	r5, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc0;
			MK_MEM_ACCESS(ldad)	r4, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sRTStatusDevAddr)], drc1;
			wdf		drc0;
			and		r5, r5, ~#SGXMKIF_HWRTDATA_CMN_STATUS_ALL_MASK;
			MK_MEM_ACCESS(stad)	[R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], r5;
			/* Clear all the RTs status bits */
			MK_MEM_ACCESS(ldad)	r5, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32NumRTsInArray)], drc1;
			wdf		drc1;
			KTA_ClearNextRTStatus:
			{
				MK_MEM_ACCESS(stad)	[r4, #0++], #0;
				isub16.testnz	r5, p3, r5, #1;
				p3	ba	KTA_ClearNextRTStatus;
			}
		#else
			/* No need to clear status seperately as RT -> Common */
			MK_MEM_ACCESS(stad)	[R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], #0;
		#endif
			idf		drc0, st;
			wdf		drc0;
		}
	}
	KTA_DontResetStatus:
	
#if defined(SGX_FEATURE_MP)
	#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	and		r3, r3, #(SGXMKIF_TAFLAGS_FIRSTKICK | SGXMKIF_TAFLAGS_VDM_CTX_SWITCH);
	and.testnz	p0, r3, r3;
	#else
	and.testnz	p0, r3, #SGXMKIF_TAFLAGS_FIRSTKICK;
	#endif
	p0	ba		KTA_DontLoadVDMPIM;
	{
		MK_MEM_ACCESS(ldad)	r3, [r2, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32VDMPIMStatus)], drc0;			
		wdf		drc0; 
		or		r3, r3, #EUR_CR_MASTER_VDM_PIM_LOAD_MASK;
		WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_PIM >> 2, r3, r2);
		
		/*
			Poll for the VDM PIM Load event.
			The cleared bit will go low again when we start the VDM below.
		*/
		ENTER_UNMATCHABLE_BLOCK;
		KTA_WaitForVDMPIMLoad:
		{
			READMASTERCONFIG(r2, #EUR_CR_MASTER_VDM_PIM_STATUS >> 2, drc0);
			wdf			drc0;
			and.testz	p0, r2, #EUR_CR_MASTER_VDM_PIM_STATUS_LOADED_MASK;
			p0	ba		KTA_WaitForVDMPIMLoad;
		}
		LEAVE_UNMATCHABLE_BLOCK;
	}
	KTA_DontLoadVDMPIM:
#endif

	/* Indicate a TA kick is in progress */
	MK_LOAD_STATE(r1, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	or		r1, r1, #TA_IFLAGS_TAINPROGRESS;
	MK_STORE_STATE(ui32ITAFlags, r1);
	MK_WAIT_FOR_STATE_STORE(drc0);

	MK_LOAD_STATE(r5, ui32TACurrentPriority, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	/* Move chosen render context to start of partial list */
	MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], drc0;
	and.testnz	p0, r1, r1;
	wdf		drc0;
	and.testnz	p1, r4, r4;
	p0	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], r4;
	p1	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], r1;
	p0	ba	KTA_NoHeadUpdate;
	{
		MK_STORE_STATE_INDEXED(sPartialRenderContextHead, r5, r4, r2);
	}
	KTA_NoHeadUpdate:
	p1	ba	KTA_NoTailUpdate;
	{
		MK_STORE_STATE_INDEXED(sPartialRenderContextTail, r5, r1, r3);
	}
	KTA_NoTailUpdate:
	idf 	drc0, st;
	wdf		drc0;

	MK_LOAD_STATE_INDEXED(r1, sPartialRenderContextHead, r5, drc0, r2);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testnz	p0, r1, r1;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], #0;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], r1;
	p0	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], r0;
	MK_STORE_STATE_INDEXED(sPartialRenderContextHead, r5, r0, r2);
	p0	ba	KTA_NoTailUpdate2;
	{
		MK_STORE_STATE_INDEXED(sPartialRenderContextTail, r5, r0, r3);
	}
	KTA_NoTailUpdate2:
	idf		drc0, st;
	wdf		drc0;

	/* Copy to render details any updates sent in the CCB */
	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTACCBCmd)], drc0;
	wdf		drc0;
	/* Save the pclink */
	mov		r9, pclink;
	/* copy the data across */
	bal		RenderDetailsUpdate;
	/* Restore the pclink */
	mov		pclink, r9;

#if !defined(USE_SUPPORT_NO_TA3D_OVERLAP) && defined(DEBUG)
	MK_LOAD_STATE(r2, ui32IRenderFlags, drc0);
	MK_MEM_ACCESS_BPCACHE(ldad) r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32OverlapCount)], drc1;
	MK_WAIT_FOR_STATE_LOAD(drc0);
	wdf		drc1;
	and.testz	p0, r2, #RENDER_IFLAGS_RENDERINPROGRESS;
	p0	ba KTA_NoOverlap;
	{
		MK_TRACE(MKTC_KICKTA_OVERLAP, MKTF_TA);
		mov		r4, #1;
		iaddu32		r3, r4.low, r3;
		MK_MEM_ACCESS_BPCACHE(stad) [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32OverlapCount)], r3;
	}
	KTA_NoOverlap:
#endif

#if (defined(FIX_HW_BRN_24304) || defined(FIX_HW_BRN_27510)) && defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	CONFIGURECACHE(#CC_FLAGS_TA_KICK);
#endif

	ldr		r10, #EUR_CR_DPM_PAGE_STATUS >> 2, drc0;
	wdf		drc0;
	and		r10, r10, #EUR_CR_DPM_PAGE_STATUS_TA_MASK;
	shr		r10, r10, #EUR_CR_DPM_PAGE_STATUS_TA_SHIFT;
	WRITEHWPERFCB(TA, TA_START, GRAPHICS, r10, p0, kta_start_1, r2, r3, r4, r5, r6, r7, r8, r9);

	MK_TRACE(MKTC_KICKTA_VDM_START, MKTF_TA | MKTF_HW);

	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTACCBCmd)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
	mov		r4, #SGXMKIF_TAFLAGS_TA_TASK_OPENCL;
	wdf		drc0;
	and.testz	p0, r2, r4;
	p0		ba KTA_NotOpenCLTask;
	{
		/* Load the OpenCL default Delay count */
		mov r4, #TA_IFLAGS_OPENCL_TASK_DELAY_COUNT;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32OpenCLDelayCount)], r4;
		idf		drc0, st;
		wdf		drc0;
	}
	KTA_NotOpenCLTask:

#if (defined(FIX_HW_BRN_30893) || defined(FIX_HW_BRN_30970)) && defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH)
	/* Check to see if this TA has been context switched */
	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTACCBCmd)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
	mov		r4, #SGXMKIF_TAFLAGS_VDM_CTX_SWITCH;
	wdf		drc0;
	and.testz	p0, r2, r4;
	p0	ba	KTA_VDMKickAllCores;
	{
		/* temps:
		 *	r5 - cores enabled
		 *	r4 - core counter (counts down from CoresEnabled to 0)
		 *	r3 - NA+complete status
		 *	r2 - core status from ukernel state (HW state unreliable)
		 *	r1 - Address of VDMCtxStoreStatus dword in main memory
		 *	r0 - HW render context
		 */
		MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderContext)], drc0;
		mov		r3, #(SGX_MK_VDM_CTX_STORE_STATUS_NA_MASK	|	\
					  SGX_MK_VDM_CTX_STORE_STATUS_COMPLETE_MASK);
		wdf		drc0;

		/* load the state for master core */
		MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32MasterVDMCtxStoreStatus)], drc0;
		wdf		drc0;
		xor.testz	p0, r2, r3;
		p0	ba	KTA_NoMasterVDMResume;
		{
			/* Restart the master VDM */
			WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_START >> 2, #EUR_CR_VDM_START_PULSE_MASK, r2);
		}
		KTA_NoMasterVDMResume:

	#if defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
		/* Resume the slaves which still have work */
		MK_LOAD_STATE(r5, ui32CoresEnabled, drc0);
		mov		r2, #OFFSET(SGXMKIF_HWRENDERCONTEXT.ui32VDMCtxStoreStatus[0]);
		iaddu32	r1, r2.low, r0;
		MK_WAIT_FOR_STATE_LOAD(drc0);

		mov		r4, #0;	/* core counter */
		KTA_VDMCheckNextCore:
		{
			/* load the slave core state */
			MK_MEM_ACCESS(ldad)	r2, [r1, #1++], drc0;
			wdf		drc0;
			xor.testz	p0, r2, r3;
			p0	ba	KTA_VDMIncCoreCounter;
			{
				/* Restart the VDM */
				WRITECORECONFIG(r4, #EUR_CR_VDM_START >> 2, #EUR_CR_VDM_START_PULSE_MASK, r2);
			}
			KTA_VDMIncCoreCounter:
			REG_INCREMENT(r4, p0);
			xor.testnz	p0, r4, r5;
			p0	ba	KTA_VDMCheckNextCore;
		}
	#endif /* SGX_SUPPORT_SLAVE_VDM_CONTEXT_SWITCHING */
		ba	KTA_VDMKickComplete;
	}
	KTA_VDMKickAllCores:
#endif /* (FIX_HW_BRN_30970 || FIX_HW_BRN_30893) && SGX_SUPPORT_MASTER_VDM_CONTEXT_SWITCHING */
	{
		/* Start the VDM on all cores */
		str		#EUR_CR_VDM_START >> 2, #EUR_CR_VDM_START_PULSE_MASK;
	}
	KTA_VDMKickComplete:
#if defined(SGX_SUPPORT_HWPROFILING)
	/* Move the frame no. into the structure SGXMKIF_HWPROFILING */
	MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32TAFrameNum)], drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sHWProfilingDevVAddr)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(SGXMKIF_HWPROFILING.ui32StartTA)], r3;
#endif /* SGX_SUPPORT_HWPROFILING */

	MK_TRACE(MKTC_KICKTA_END, MKTF_TA);
	/* branch back to calling code */
	lapc;
}


/*****************************************************************************
 Split command buffer routine

 inputs:	r0 - context of TA kick to be started
			r1 - CCB command of TA kick about to be started
			R_RTData - SGXMKIF_HWRTDATA for TA command
 temps:		r2
			r3
			r4
			r5
			r6
*****************************************************************************/
#if defined(UNDER_VISTA_BASIC)
SplitControlStream:
{
	/*
		Check if we need to patch VDM Base register
		If SGXMKIF_TAFLAGS_SPLITRESUME flag is set, then we need to replace
		the VDMCtrlStreamBase register
	*/
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
	mov		r3, #SGXMKIF_TAFLAGS_SPLITRESUME;
	wdf		drc0;
	and.testz	p0, r2, r3;
	p0	ba		SCS_DontAdjustBase;
	{
		/* Replace psTACmd->sTARegs.sVDMCtrlStreamBase.ui32RegVal with our saved value,
		   and update the register (as it's already been written) */
		MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sSplitNextKickAddress)], drc0;
		wdf		drc0;
		mov		r3, #OFFSET(SGXMKIF_CMDTA.sTARegs.ui32VDMCtrlStreamBase);
		iaddu32	r3, r3.low, r1;
		MK_MEM_ACCESS(stad)	[r3], r4;
		idf		drc0, st;
		wdf		drc0;
		str		#EUR_CR_VDM_CTRL_STREAM_BASE >> 2, r4;
	}
	SCS_DontAdjustBase:

	/* Check if we need to split the VDM control stream */
	mov		r3, #SGXMKIF_TAFLAGS_SPLIT;
	and.testz	p0, r2, r3;
	p0	ba		SCS_DontSplit;
	{
		/*
			Need to set as TA requestor so we can access VDM streams
		*/
		SWITCHEDMTOTA();
		/*
			 Need to split VDM control stream.
			 This means we are supplied the address of an index list command in the VDM control stream.
			 This is the last index list command that must be executed, so we copy this to a temporary
			 buffer (also supplied), appending a link to the control stream terminate (also supplied).
			 The original index list command is changed to a link to the temporary copy we have made.
			 This means the VDM will run up to, and including, this index list command and then terminate.
			 We also need to store the address of the command immediately following the index list command
			 in the original control stream, as that is where the next Kick will start from.


			For example:

			Say our VDM control stream looks like this

			         +-------++-------++-------++-------++-------++------+
			 KICK -> | Idx 0 || State || Idx 1 || Idx 2 || Idx 3 || Term |
			         +-------++-------++-------++-------++-------++------+

			 If we split at Idx 2, we supply the address of Idx2 as the split point, and we want to get


			         +-------++-------++-------++------+                                +------+
			 KICK -> | Idx 0 || State || Idx 1 || Link | --+                        +-->| Term |
			         +-------++-------++-------++------+   |                        |   +------+
	                                                       |    +-------++------+   |
														   +--->| Idx 2 || Link | --+
			                                                    +-------++------+
	                                                                              \
																				   \ This is the temporary buffer supplied by driver

			and the next kick will be

					        +-------++------+
					KICK -> | Idx 3 || Term |
					        +-------++------+

			NOTE: The next kick will use the same setup as the previous - except a flag will be set to update the VDM
			base address register. This is because the VDM base register in the TACmd points to the start of the stream,
			but we now want to run it from somewhere in the middle. The new start address will have been calculated by
			the EDM code.
		*/

		/*
			Step 1: Obtain the device address of the index list command we are splitting at,
					read the index list header and copy all the words marked as present into temp.

					NOTE: Source addresses must be one dword before required address due to pre-increment in USSE
		*/
		MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_CMDTA.sSplitIndexAddress)], drc0; /* Find out the source address */
		MK_MEM_ACCESS_CACHED(ldad)	r4, [r1, #DOFFSET(SGXMKIF_CMDTA.sSplitTempAddress)], drc1; /* Find out the destination address */
		wdf		drc0;
		wdf		drc1;

		MK_MEM_ACCESS_BPCACHE(ldad.f2)	r5, [r2, #0], drc0; /* Read index list header and first word */
		wdf		drc0;
		MK_MEM_ACCESS_BPCACHE(stad.f2)	[r4, #0], r5; /* Copy the header and first word into the destination */
		idf		drc0, st;
		wdf		drc0;

		mov		r6, #0x10008; /* Initial size of index list is 2 dwords, also need 1 in top 16 bits for multiply */

		/* Note, there may be a clever way of doing this with muls or something */
		mov		r3, #EURASIA_VDM_IDXPRES2;
		and.testz	p0, r5, r3;
		p0	ba		SCS_SkipPRES2;
		{
			MK_MEM_ACCESS_BPCACHE(ldad.f1)	r7, [r2, r6], drc0; /* Read word 2 */
			wdf		drc0;
			MK_MEM_ACCESS_BPCACHE(stad.f1)	[r4, r6], r7; /* Copy word 2 */
			idf		drc0, st;
			wdf		drc0;
			mov		r7, #0x10004;
			iaddu32		r6, r7.low, r6;
		}
		SCS_SkipPRES2:
		mov		r3, #EURASIA_VDM_IDXPRES3;
		and.testz	p0, r5, r3;
		p0	ba		SCS_SkipPRES3;
		{
			MK_MEM_ACCESS_BPCACHE(ldad.f1)	r7, [r2, r6], drc0; /* Read word 3 */
			wdf		drc0;
			MK_MEM_ACCESS_BPCACHE(stad.f1)	[r4, r6], r7; /* Copy word 3 */
			idf		drc0, st;
			wdf		drc0;
			mov		r7, #0x10004;
			iaddu32		r6, r7.low, r6;
		}
		SCS_SkipPRES3:
#if defined(SGX545)
		/** TODO: FIXME! I'M PROBABLY BROKEN! **/
		// 4&5 always present on SGX545;...
		MK_MEM_ACCESS_BPCACHE(ldad.f2)	r7, [r2, r6], drc0; /* Read words 4&5 */
		wdf		drc0;
		MK_MEM_ACCESS_BPCACHE(stad.f2)	[r4, r6], r7; /* Copy words 4&5 */
		idf		drc0, st;
		wdf		drc0;
		mov		r7, #0x10008;
		iaddu32		r6, r7.low, r6;
#else
		mov		r3, #EURASIA_VDM_IDXPRES45;
		and.testz	p0, r5, r3;
		p0	ba		SCS_SkipPRES45;
		{
			MK_MEM_ACCESS_BPCACHE(ldad.f2)	r7, [r2, r6], drc0; /* Read words 4&5 */
			wdf		drc0;
			MK_MEM_ACCESS_BPCACHE(stad.f2)	[r4, r6], r7; /* Copy words 4&5 */
			idf		drc0, st;
			wdf		drc0;
			mov		r7, #0x10008;
			iaddu32		r6, r7.low, r6;
		}
		SCS_SkipPRES45:
#endif
#if defined(EURASIA_VDM_IDXPRESCOMPLEXSTAT)
		mov		r3, #EURASIA_VDM_IDXPRESCOMPLEXSTAT;
		and.testz	p0, r5, r3;
		p0	ba		SCS_SkipPRESCOMPLEXSTAT;
		{
			MK_MEM_ACCESS_BPCACHE(ldad.f5)	r7, [r2, r6], drc0; /* Read words 6,7,8,9&10 */
			wdf		drc0;
			MK_MEM_ACCESS_BPCACHE(stad.f5)	[r4, r6], r7; /* Copy words 6,7,8,9&10 */
			idf		drc0, st;
			wdf		drc0;
			mov		r7, #0x10014;
			iaddu32		r6, r7.low, r6;
		}
		SCS_SkipPRESCOMPLEXSTAT:
#else
		mov		r3, #EURASIA_VDM_IDXPRES67;
		and.testz	p0, r5, r3;
		p0	ba		SCS_SkipPRES67;
		{
			MK_MEM_ACCESS_BPCACHE(ldad.f2)	r7, [r2, r6], drc0; /* Read words 6&7 */
			wdf		drc0;
			MK_MEM_ACCESS_BPCACHE(stad.f2)	[r4, r6], r7; /* Copy words 6&7 */
			idf		drc0, st;
			wdf		drc0;
			mov		r7, #0x10008;
			iaddu32		r6, r7.low, r6;
		}
		SCS_SkipPRES67:
#endif
		/*
			Step 2: Save the start of the next kick.
			This is the address of the index list command supplied by the driver
			PLUS the size of the index list command.
		*/
		iaddu32		r3, r6.low, r2;
		mov			r7, #4;										/* r3 is one dword behind, so add 4 */
		iaddu32		r3, r7.low, r3;
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sSplitNextKickAddress)], r3;

		/*
			Step 3: Overwrite index list command with link to temp
		*/
		mov r3, #4;												/* r4 is one dword behind, so add 4 */
		iaddu32	r4, r3.low, r4;
		iaddu32 r2, r3.low, r2;
		shr		r3, r4, #EURASIA_TASTRMLINK_ADDR_ALIGNSHIFT;		/* Convert ui32SplitTempAddress address into link */
		MK_MEM_ACCESS_BPCACHE(stad)	[r2, #0], r3;							/* Write this to ui32SplitIndexAddress */

		/*
			Step 4: Append 'link to terminate' to temp copy
		*/
		MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32SplitTermLink)], drc0;		/* Get link word for terminate */
		iaddu32	r4, r6.low, r4;		/* Get to just after new index list command - i.e. ui32SplitTempAddress + sizeof index list command */
		wdf		drc0;
		MK_MEM_ACCESS_BPCACHE(stad)	[r4, #0], r2;	/* Write the link to the terminate */
		idf		drc0, st;
		wdf		drc0;
		SWITCHEDMBACK();
	}
	SCS_DontSplit:
	/* Done */
	ba	SplitControlStreamReturn;
}
#endif /* UNDER_VISTA_BASIC */

/*****************************************************************************
 End of file (ta.asm)
*****************************************************************************/
