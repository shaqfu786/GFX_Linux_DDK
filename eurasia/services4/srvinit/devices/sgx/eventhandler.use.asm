/*****************************************************************************
 Name		: eventhandler.use.asm

 Title		: USE program for EDM event processing

 Author 	: PowerVR

 Created  	: 02/12/2005

 Copyright	: 2005 by Imagination Technologies Limited. All rights reserved.
		  No part of this software, either material or conceptual
		  may be copied or distributed, transmitted, transcribed,
		  stored in a retrieval system or translated into any
		  human or computer language in any form by any means,
		  electronic, mechanical, manual or other-wise, or
		  disclosed to third parties without the express written
		  permission of Imagination Technologies Limited, HomePark
		  Industrial Estate, King's Langley, Hertfordshire,
		  WD4 8LZ, U.K.

 Description 	: USE program for host event kicks and automatic renders

 Program Type	: USE assembly language

 Version 	: $Revision: 1.108 $

 Modifications	:

 $Log: eventhandler.use.asm $
 *****************************************************************************/

#include "usedefs.h"
#include "pvrversion.h"
#include "sgx_options.h"

/***************************
 Sub-routine Imports
***************************/
/* Utility routines */
.import MKTrace;
.import InvalidateBIFCache;
.import EmitLoopBack;
#if defined(SGX_FEATURE_SYSTEM_CACHE)
.import InvalidateSLC;
#endif

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
.import InvalidateExtSysCache;
#endif

#if defined(SGX_FEATURE_MK_SUPERVISOR)
.import JumpToSupervisor;
#endif

.import StartTimer;

.import	CheckTAQueue;
.import Check3DQueues;
.import Check2DQueue;

.import Timer;
.import CheckHostCleanupRequest;
.import CheckHostPowerRequest;
.import WaitForReset;

.import WriteHWPERFEntry;
.import UpdateHWPerfRegs;

#if (defined(SUPPORT_SGX_EDM_MEMORY_DEBUG) && defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)) || defined(FIX_HW_BRN_24549)
.import IdleCore;
.import Resume;
#endif
#if defined(SUPPORT_SGX_EDM_MEMORY_DEBUG) && defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
.import SwitchEDMBIFBank;
.import SetupDirListBase;
.import SetupRequestorEDM;
#endif /* SUPPORT_SGX_EDM_MEMORY_DEBUG && SGX_FEATURE_MULTIPLE_MEM_CONTEXTS */

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
.import StartTAContextSwitch;
#endif
#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH)
.import Start3DContextSwitch;
#endif

/***************************
 Sub-routine Exports
***************************/
.export EventHandler;


.modulealign 2;

ZeroPC:
{
	/*
		Guard routine to catch erroneous jumps to the instruction at Program Counter 0.
	*/
	MK_ASSERT_FAIL(MKTC_ZEROPC)
}

/*****************************************************************************
 Check for HW events
*****************************************************************************/
.align 2;
EventHandler:
{
	UKERNEL_PROGRAM_INIT_COMMON();

#if defined(FIX_HW_BRN_31474)
	/* is the sw_event2 bit set? */
	and			r0, PA(ui32PDSIn0), #EURASIA_PDS_IR0_EDM_EVENT_SW_EVENT2;
	and.testnz	p0, r0, r0;
	!p0	ba		NoPDSCacheClear;
	{
		/* if this is the only event set, exit */
		mov		r0, #EURASIA_PDS_IR0_EDM_EVENT_SW_EVENT2;
		xor.testz	p0, PA(ui32PDSIn0), r0;
		p0	and		r0, PA(ui32PDSIn1), #EURASIA_PDS_IR1_EDM_EVENT_KICKPTR_CLRMSK;
		p0	and.testz	p0, r0, r0;
		!p0	ba	HandleOtherEvents;
		{
			/* There are no other events to handle so we can just exit */ 
	#if !defined(FIX_HW_BRN_29104)
			UKERNEL_PROGRAM_PHAS();
	#endif
			/* End the program */
			mov.end		r0, r0;
		}
		HandleOtherEvents:
	}
	NoPDSCacheClear:
	
#endif

	WRITEHWPERFCB(TA, MK_EVENT_START, MK_EXECUTION, #0, p0, event_start, r2, r3, r4, r5, r6, r7, r8, r9);

	/* Test for any of the 3D events occuring */
	mov			r0, #SGX_MK_PDS_IR1_3DEVENTS;
	and			r0, PA(ui32PDSIn1), r0;
	and			r1, PA(ui32PDSIn0), #SGX_MK_PDS_IR0_3DEVENTS;
	or			r0, r0, r1;
	and.testnz	p0, r0, r0;
	!p0	ba	No3DModuleEvents;
	{
		mov		r1, #0;
		/* We have received a 3D event, so lets find out what and set
			the bits and emit loopback */
		/* Test the 3D mem free event */
		and		r0, PA(SGX_MK_PDS_3DMEMFREE_PA), #SGX_MK_PDS_3DMEMFREE;
		and.testnz	p0, r0, r0;
		!p0 	ba	No3DMemFree;
		{
			MK_TRACE(MKTC_EHEVENT_3DMEMFREE, MKTF_EHEV);
			or	r1, r1, #USE_LOOPBACK_3DMEMFREE;
		}
		No3DMemFree:

		/* Test the pixelbe end of render (non 3D mem free renders) */
		and		r0, PA(SGX_MK_PDS_EOR_PA), #SGX_MK_PDS_EOR;
		and.testnz	p0, r0, r0;
		!p0 	ba	No3DEnd;
		{
			MK_TRACE(MKTC_EHEVENT_PIXELENDRENDER, MKTF_EHEV);
			or	r1, r1, #USE_LOOPBACK_PIXENDRENDER;
#if defined(SGX_SUPPORT_HWPROFILING)
			/* Move the frame no. into the structure SGXMKIF_HWPROFILING */
			MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui323DFrameNum)], drc0;
			MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sHWProfilingDevVAddr)], drc0;
			wdf	drc0;
			MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(SGXMKIF_HWPROFILING.ui32End3D)], r3;
#endif /* SGX_SUPPORT_HWPROFILING */

#if defined(SUPPORT_SGX_HWPERF)
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
				WRITEHWPERFCB(3D, TRANSFER_END, GRAPHICS, #0, p0, kt_end_1, r2, r3, r4, r5, r6, r7, r8, r9);
			}
			PixelEndRender_EndHWPerf:
#endif /* SUPPORT_SGX_HWPERF */
		}
		No3DEnd:

		/* Test for isp breakpoint events */
		and		r0, PA(ui32PDSIn1), #EURASIA_PDS_IR1_EDM_EVENT_ISPBREAKPOINT;
		and.testnz	p0, r0, r0;
		//MK_TRACE(MKTC_EHEVENT_ISPBREAKPOINT, MKTF_EHEV);
		p0	or	r1, r1, #USE_LOOPBACK_ISPBREAKPOINT;

		/* check if we need to emit a loopback this time */
		and.testz	p0, r1, r1;
		p0	ba	No3DModuleEvents;
		{
			/* setup the registers todo the loopback */
			mov		r16, r1;
			PVRSRV_SGXUTILS_CALL(EmitLoopBack);
		}
	}
	No3DModuleEvents:

	/* Test for any of the SPM Module events occuring */
	and		r0, PA(SGX_MK_PDS_SPM_PA), #SGX_MK_PDS_SPMEVENTS;
	and.testnz	p0, r0, r0;
	!p0	ba	NoSPMModuleEvents;
	{
		mov		r1, #0;

		/* Test the mem threshold reached event */
		and		r0, PA(SGX_MK_PDS_SPM_PA), #(SGX_MK_PDS_GBLOUTOFMEM | SGX_MK_PDS_MTOUTOFMEM);
		and.testnz	p0, r0, r0;
		!p0	ba		NoOutOfMem;
		{
			MK_TRACE(MKTC_EHEVENT_OUTOFMEM, MKTF_EHEV);
			#if defined(DBGBREAK_ON_SPM)
			MK_ASSERT_FAIL(MKTC_EHEVENT_OUTOFMEM);
			#endif /* DBGBREAK_ON_SPM */
#if defined(FIX_HW_BRN_23055)
			MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32MTETEDeadlockTicks)], #0;
			idf				drc0, st;
			wdf				drc0;
#endif
			/* Set the bit to indicate the event has occurred */
			or	r1, r1, #(USE_LOOPBACK_GBLOUTOFMEM | USE_LOOPBACK_MTOUTOFMEM);
		}
		NoOutOfMem:

		/* Test the TA terminate complete event */
		and		r0, PA(SGX_MK_PDS_SPM_PA), #SGX_MK_PDS_TATERMINATE;
		and.testnz	p0, r0, r0;
		//MK_TRACE(MKTC_EHEVENT_TATERMINATE, MKTF_EHEV);
		/* Set the bit if the event has occurred */
		p0 or	r1, r1, #USE_LOOPBACK_TATERMINATE;
	
		/* Send the loopback which will handle the events */
		EMITLOOPBACK(r1);
	}
	NoSPMModuleEvents:

	/* Test for any of the TA events occuring */
	and		r0, PA(SGX_MK_PDS_TA_PA), #SGX_MK_PDS_TAEVENTS;
	and.testnz	p0, r0, r0;
	!p0	ba	NoTAModuleEvents;
	{
		mov		r1, #0;

		/* We have received a TA event, so let's find out what and set
			the bits and emit loopback */
		/* Test the TA Finished event */
		and		r0, PA(SGX_MK_PDS_TA_PA), #SGX_MK_PDS_TAFINISHED;
		and.testnz	p0, r0, r0;
		!p0	ba		NoTAFinished;
		{
			MK_TRACE(MKTC_EHEVENT_TAFINISHED, MKTF_EHEV);

			ldr		r10, #EUR_CR_DPM_PAGE_STATUS >> 2, drc0;
			wdf		drc0;
			and		r10, r10, #EUR_CR_DPM_PAGE_STATUS_TA_MASK;
			shr		r10, r10, #EUR_CR_DPM_PAGE_STATUS_TA_SHIFT;

			WRITEHWPERFCB(TA, TA_END, GRAPHICS, r10, p0, kta_end_1, r2, r3, r4, r5, r6, r7, r8, r9);

#if defined(SGX_SUPPORT_HWPROFILING)
			/* Move the frame no. into the structure SGXMKIF_HWPROFILING */
			MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32TAFrameNum)], drc0;
			MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sHWProfilingDevVAddr)], drc0;
			wdf		drc0;
			MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(SGXMKIF_HWPROFILING.ui32EndTA)], r3;
#endif /* SGX_SUPPORT_HWPROFILING */

			or	r1, r1, #USE_LOOPBACK_TAFINISHED;
		}
		NoTAFinished:

		/* Send the loopback which will handle the events */
		EMITLOOPBACK(r1);
	}
	NoTAModuleEvents:

#if defined(SGX_FEATURE_2D_HARDWARE)

	/* Test for any of the 2D events occuring */
	/* Test the 2D Finished event */
	#if defined(SGX_FEATURE_PTLA)
	and		r0, PA(ui32PDSIn0), #EURASIA_PDS_IR0_EDM_EVENT_PTLACOMPLETE;
	#else
	and		r0, PA(ui32PDSIn0), #EURASIA_PDS_IR0_EDM_EVENT_TWODCOMPLETE;
	#endif
	and.testnz	p0, r0, r0;
	!p0	ba	No2DCompleteEvent;
	{
		MK_TRACE(MKTC_EHEVENT_2DCOMPLETE, MKTF_EHEV);

		WRITEHWPERFCB(2D, 2D_END, GRAPHICS, #0, p0, k2d_end_1, r2, r3, r4, r5, r6, r7, r8, r9);

		/* Send the loopback which will handle the events */
		EMITLOOPBACK(#USE_LOOPBACK_2DCOMPLETE);
	}
	No2DCompleteEvent:
#endif
	/* Test the timer event */
	and		r0, PA(ui32PDSIn0), #EURASIA_PDS_IR0_EDM_EVENT_TIMER;
	and.testnz	p0, r0, r0;
	!p0 ba		NoTimer;
	{
#if defined(FIX_HW_BRN_26620) && defined(SGX_FEATURE_SYSTEM_CACHE) && !defined(SGX_BYPASS_SYSTEM_CACHE)
		/* Before attempting to read anything, the SLC must be invalidated */
		INVALIDATE_SYSTEM_CACHE();
#endif
#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
		INVALIDATE_EXT_SYSTEM_CACHE();
#endif /* SUPPORT_EXTERNAL_SYSTEM_CACHE */

		/* Increment the time wrap counter. */
		MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32TimeWraps)], drc0;
		mov				r3, #1;
		wdf				drc0;
		iaddu32			r2, r3.low, r2;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32TimeWraps)], r2;
		idf				drc0, st;
		wdf				drc0;

		MK_TRACE(MKTC_EHEVENT_TIMER, MKTF_TIM);

		mov				R_EDM_PCLink, #LADDR(NoTimer);
		ba				Timer;
	}
	NoTimer:

#if defined(FIX_HW_BRN_26620) && defined(SGX_FEATURE_SYSTEM_CACHE) && !defined(SGX_BYPASS_SYSTEM_CACHE)
	and			r0, PA(ui32PDSIn0), #EURASIA_PDS_IR0_EDM_EVENT_SW_EVENT1;
	and.testnz	p0, r0, r0;
	!p0	ba		NoSLCInval;
	{
		/* Before attempting to read anything, the SLC must be invalidated */
		INVALIDATE_SYSTEM_CACHE();
#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
		INVALIDATE_EXT_SYSTEM_CACHE();
#endif
		/* Now that the SLC has been invalidated, cause a SWEVENT */
		str		#EUR_CR_EVENT_KICK >> 2, #EUR_CR_EVENT_KICK_NOW_MASK;
	}
	NoSLCInval:
#endif

	/* Test the host kicker event */
	and			r0, PA(ui32PDSIn1), #EURASIA_PDS_IR1_EDM_EVENT_SWEVENT;
	and.testnz	p0, r0, r0;
	!p0	ba		NoHostKick;
	{
		MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_CCBCtl, #DOFFSET(PVRSRV_SGX_CCB_CTL.ui32ReadOffset)], drc0;
		wdf				drc0;
		/*****************************************************************************
		 Process the host kicker event
		*****************************************************************************/
		MK_TRACE(MKTC_EHEVENT_SWEVENT, MKTF_EV | MKTF_EHEV | MKTF_HK);
		MK_TRACE_REG(r1, MKTF_HK);

		/* fetch the command */
		MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sKernelCCB)], drc0;
		wdf		drc0;
		imae	r0, r1.low, #SIZEOF(SGXMKIF_COMMAND), r0, u32;
		/*
			Note: CCB(0...5) maps to r4...r11 so make sure these temps are not used
			for other purposes before branching to the CCB command handler below.
		*/
		#if defined(SGX_FEATURE_WRITEBACK_DCU) && !defined(SGX_BYPASS_DCU) && !defined(SUPPORT_SGX_UKERNEL_DCU_BYPASS)
		MK_MEM_ACCESS_CACHED(ldad.fcfill)	CCB(ui32ServiceAddress), [r0, #1++], drc0;
		#else 
		ldad.fcfill				CCB(ui32ServiceAddress), [r0, #1++], drc0;
		#endif
		MK_MEM_ACCESS_CACHED(ldad.f7)		CCB(ui32CacheControl), [r0, #0++], drc0;
		
		wdf drc0;

		/* test the driver cache control requests */
	#if defined(SGX_FEATURE_SYSTEM_CACHE)
		/* Test for SGXMKIF_CC_INVAL_BIF_SL */
		and.testnz	p0, CCB(ui32CacheControl), #SGXMKIF_CC_INVAL_BIF_SL;
		!p0	ba		NoInvalSLC;
		{
			/* Invalidate the SLC */
			MK_TRACE(MKTC_INVALSLC, MKTF_SCH | MKTF_HW);

			#if defined(SGX_FEATURE_MP)
				INVALIDATE_SYSTEM_CACHE(#EUR_CR_MASTER_SLC_CTRL_INVAL_ALL_MASK, #EUR_CR_MASTER_SLC_CTRL_FLUSH_ALL_MASK);
			#else
				INVALIDATE_SYSTEM_CACHE();
			#endif /* SGX_FEATURE_MP */

			#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
				INVALIDATE_EXT_SYSTEM_CACHE();
			#endif /* SUPPORT_EXTERNAL_SYSTEM_CACHE */
		}
		NoInvalSLC:
	#endif /* SGX_FEATURE_SYSTEM_CACHE */

		/* test for SGXMKIF_CC_INVAL_BIF_PD */
		and.testnz	p0,  CCB(ui32CacheControl), #SGXMKIF_CC_INVAL_BIF_PD;
		!p0	ba		NoBIFInvalPDCache;
		{
			/* invalidate the PD, PT caches and TLBs */
			MK_TRACE(MKTC_INVALDC, MKTF_SCH | MKTF_HW);
	
	#if defined(FIX_HW_BRN_28889)
			MK_LOAD_STATE(r2, ui32IRenderFlags, drc0);
			MK_LOAD_STATE(r3, ui32ITAFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			or		r2, r2, r3;
			and.testz	p0, r2, #RENDER_IFLAGS_RENDERINPROGRESS;
			p0	ba	DoBIFInvalPD;
			{
				/* The HW is busy so we have to defer the invalidate */
				MK_LOAD_STATE(r2, ui32CacheCtrl, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);
				or		r2, r2, #SGXMKIF_CC_INVAL_BIF_PD;
				MK_STORE_STATE(ui32CacheCtrl, r2);
				MK_WAIT_FOR_STATE_STORE(drc0);
				
				ba		NoBIFInvalPTCache;
			}
			DoBIFInvalPD:
			/* Indicate to the host that we will have done the invalidate by the time the next kick arrives */
			MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32InvalStatus)], #PVRSRV_USSE_EDM_BIF_INVAL_COMPLETE;
			idf			drc0, st;
			wdf			drc0;
	#endif
	#if defined(FIX_HW_BRN_31620)
			/* The LSB indicates whether the application PD is valid, if the bit is not set
				just worry about the kernel 'shared' heaps */
			and.testz	p0, CCB(ui32Data[3]), #1;
			p0	ba	RefreshSharedHeaps;
			{
				/* extract the PDPhysAddr from the command */
				and		CCB(ui32Data[3]), CCB(ui32Data[3]), ~#1;
				
				/* Does the application PD supplied match the current PD */
				ldr		r2, #EUR_CR_BIF_DIR_LIST_BASE0 >> 2, drc0;
				wdf		drc0;
				
				/* If the PDs match use the applcication refresh mask.
					The application PD mask also includes the shared heap bits */
				xor.testz	p0, r2, CCB(ui32Data[3]);
				p0	mov		r12, CCB(ui32Data[4]);
				p0	mov		r13, CCB(ui32Data[5]);
				p0	ba	RefreshDCCache;
			}
			RefreshSharedHeaps:
			{
				/* Either the PDPhys does not match or it is a kernel command,
					so use the kernel PD refresh mask */
				
				/* Check whether any of the shared heaps are actually affected? 
					If not, no refresh is required but PTEs should still be invalidated */
				or.testz	p0, CCB(ui32Data[1]), CCB(ui32Data[2]);
				p0	ba	NoDCRefreshRequired;
				
				/* Something has changed in the shared heaps so setup the mask 
					ready for the refresh call */
				mov		r12, CCB(ui32Data[1]);
				mov		r13, CCB(ui32Data[2]);
			}
			RefreshDCCache:
			{
				/* Refresh the DC cache on enabled cores, using the mask supplied */
				REQUEST_DC_REFRESH(r12, r13);
			}
			NoDCRefreshRequired:
			/* The call to InvalidateBIFCache below will do a PT invalidate */
	#endif
			mov		r16, #1;
			PVRSRV_SGXUTILS_CALL(InvalidateBIFCache);
			/*
				SGXMKIF_CC_INVAL_BIF_PD covers SGXMKIF_CC_INVAL_BIF_PT
			*/
			ba		NoBIFInvalPTCache;
		}
		NoBIFInvalPDCache:

		/* Test for SGXMKIF_CC_INVAL_BIF_PT */
		and.testnz	p0, CCB(ui32CacheControl), #SGXMKIF_CC_INVAL_BIF_PT;
		!p0	ba		NoBIFInvalPTCache;
		{
			/* invalidate the PT cache and TLBs */
			MK_TRACE(MKTC_INVALPT, MKTF_SCH | MKTF_HW);
	
	#if defined(FIX_HW_BRN_28889)
			MK_LOAD_STATE(r2, ui32IRenderFlags, drc0);
			MK_LOAD_STATE(r3, ui32ITAFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			or		r2, r2, r3;
			and.testz	p0, r2, #RENDER_IFLAGS_RENDERINPROGRESS;
			p0	ba	DoBIFInvalPT;
			{
				MK_LOAD_STATE(r2, ui32CacheCtrl, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);
				or		r2, r2, #SGXMKIF_CC_INVAL_BIF_PT;
				MK_STORE_STATE(ui32CacheCtrl, r2);
				MK_WAIT_FOR_STATE_STORE(drc0);
				
				ba	NoBIFInvalPTCache;
			}
			DoBIFInvalPT:
			/* Indicate to the host that we will have done the invalidate by the time the next kick arrives */
			MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32InvalStatus)], #PVRSRV_USSE_EDM_BIF_INVAL_COMPLETE;
			idf			drc0, st;
			wdf			drc0;
	#endif
			mov		r16, #0;
			PVRSRV_SGXUTILS_CALL(InvalidateBIFCache);
	
		}
		NoBIFInvalPTCache:
		
		/* Test for SGXMKIF_CC_INVAL_DATA */
		and.testnz	p0, CCB(ui32CacheControl), #SGXMKIF_CC_INVAL_DATA;
		!p0	ba		NoInvalDataCache;
		{
			/* invalidate the data cache */
			MK_TRACE(MKTC_INVALDATA, MKTF_SCH | MKTF_HW);

	#if defined(EUR_CR_CACHE_CTRL)
		#if defined(FIX_HW_BRN_24549)
			/* Pause the USSE */
			mov 	r16, #(USE_IDLECORE_3D_REQ | USE_IDLECORE_USSEONLY_REQ);
			PVRSRV_SGXUTILS_CALL(IdleCore);
		#endif

			/* Invalidate the MADD cache */
			ldr		r2, #EUR_CR_CACHE_CTRL >> 2, drc0;
			wdf		drc0;
			or		r2, r2, #EUR_CR_CACHE_CTRL_INVALIDATE_MASK;
			str		#EUR_CR_CACHE_CTRL >> 2, r2;
	
			ENTER_UNMATCHABLE_BLOCK;
			WaitForMADDInvalidate:
			{
				ldr		r2, #EUR_CR_EVENT_STATUS >> 2, drc0;
				wdf		drc0;
				shl.testns	p0, r2, #(31 - EUR_CR_EVENT_STATUS_MADD_CACHE_INVALCOMPLETE_SHIFT);
				p0	ba		WaitForMADDInvalidate;
			}
			LEAVE_UNMATCHABLE_BLOCK;
	
			mov		r2, #EUR_CR_EVENT_HOST_CLEAR_MADD_CACHE_INVALCOMPLETE_MASK;
			str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r2;

		#if defined(FIX_HW_BRN_24549)
			RESUME(r2);
		#endif
	#else
			/* Invalidate the data cache */
			str		#EUR_CR_DCU_ICTRL >> 2, #EUR_CR_DCU_ICTRL_INVALIDATE_MASK;
	
			ENTER_UNMATCHABLE_BLOCK;
			WaitForDCUInvalidate:
			{
				ldr		r2, #EUR_CR_EVENT_STATUS2 >> 2, drc0;
				wdf		drc0;
				shl.testns	p0, r2, #(31 - EUR_CR_EVENT_STATUS2_DCU_INVALCOMPLETE_SHIFT);
				p0	ba		WaitForDCUInvalidate;
			}
			LEAVE_UNMATCHABLE_BLOCK;
	
			mov		r2, #EUR_CR_EVENT_HOST_CLEAR2_DCU_INVALCOMPLETE_MASK;
			str		#EUR_CR_EVENT_HOST_CLEAR2 >> 2, r2;
	#endif
		}
		NoInvalDataCache:

		/* Setup the branch to the CCB command handler */
		mov		pclink, CCB(ui32ServiceAddress);

		/* Update the CCB read offset to free the command */
		iaddu32  r0, r1.low, #1;
		and		r0, r0, #0xFF;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_CCBCtl, #DOFFSET(PVRSRV_SGX_CCB_CTL.ui32ReadOffset)], r0;
		idf		drc0, st;

		/* Wait for the CCB read offset to be written to memory */
		wdf		drc0;

		/* Branch to the CCB command handler */
		lapc;
	}
	NoHostKick:

	WRITEHWPERFCB(TA, MK_EVENT_END, MK_EXECUTION, #0, p0, event_end, r2, r3, r4, r5, r6, r7, r8, r9);

#if !defined(FIX_HW_BRN_29104)
	UKERNEL_PROGRAM_PHAS();
#endif

	/* End the program */
	mov.end		r0, r0;
}


/*****************************************************************************
 HostKickXXX - CCB command handler generic prototype

	Handle a CCB command in response to a host kick.

 inputs:	CCB(0...3)

 NOTE:		Each command handler must branch to label HostKickProcessed
 				on completion.

 NOTE:		CCB(0...3) maps to r4...r7 - each command handler must ensure that
 				it does not re-use these temps before reading the command
 				data.
*****************************************************************************/

.export HostKickFlushPDCache;
HostKickFlushPDCache:
{
	MK_TRACE(MKTC_HOSTKICK_END, MKTF_SCH | MKTF_HK);

#if !defined(FIX_HW_BRN_29104)
	UKERNEL_PROGRAM_PHAS();
#endif

	/* Done */
	mov.end	r0, r0;

}

.export HostKickPower;
HostKickPower:
{
	xor.testz		p0, CCB(ui32Data[1]), #PVRSRV_POWERCMD_RESUME;
	!p0	ba			HK_NotResume;
	{
		MK_TRACE(MKTC_HOSTKICK_RESUME, MKTF_SCH | MKTF_POW | MKTF_HK);
		bal			StartTimer;
		ba			HostKickProcessed;
	}
	HK_NotResume:

	/* Process the request and set the internal flag */
	MK_LOAD_STATE(r0, ui32HostRequest, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	xor.testz		p0, CCB(ui32Data[1]), #PVRSRV_POWERCMD_POWEROFF;
	!p0	ba			HK_NotPowerOff;
	{
		MK_TRACE(MKTC_HOSTKICK_POWEROFF, MKTF_SCH | MKTF_POW | MKTF_HK);
		or			r0, r0, #SGX_UKERNEL_HOSTREQUEST_POWEROFF;
	}
	HK_NotPowerOff:
	xor.testz		p0, CCB(ui32Data[1]), #PVRSRV_POWERCMD_IDLE;
	!p0	ba			HK_NotIdle;
	{
		MK_TRACE(MKTC_HOSTKICK_IDLE, MKTF_SCH | MKTF_POW | MKTF_HK);
		p0	or			r0, r0, #SGX_UKERNEL_HOSTREQUEST_IDLE;
	}
	HK_NotIdle:
	MK_STORE_STATE(ui32HostRequest, r0);
	MK_WAIT_FOR_STATE_STORE(drc0);
	bal				CheckHostPowerRequest;

	/* Power request could not be dealt with immediately, so will be handled from the timer. */
	ba HostKickProcessed;
}


.export HostKickContextSuspend;
HostKickContextSuspend:
{
	/* r0 is the context being suspended. May be render, TQ, 2D etc. */
	mov		r0, CCB(ui32Data[0]);
	MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWCONTEXT.ui32Flags)], drc0;
	wdf drc0;

	xor.testz		p0, CCB(ui32Data[1]), #PVRSRV_CTXSUSPCMD_RESUME;
	!p0	ba			HK_NotCtxResume;
	{
		MK_TRACE(MKTC_HOSTKICK_CTXRESUME, MKTF_SCH | MKTF_HK);
		
		/* Clear the suspend flag from the context. */
		and	r1, r1, #~SGXMKIF_HWCFLAGS_SUSPEND;
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWCONTEXT.ui32Flags)], r1;
		ba			HostKickProcessed;
	}
	HK_NotCtxResume:
	
	MK_TRACE(MKTC_HOSTKICK_CTXSUSPEND, MKTF_SCH | MKTF_HK);

	/* Set the suspend flag in the context. */
	or	r1, r1, #SGXMKIF_HWCFLAGS_SUSPEND;
	MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWCONTEXT.ui32Flags)], r1;

	/*
		Now switch out any task currenly running on the newly suspended context.
	*/
	
	/* Check running render (if any). */
	MK_LOAD_STATE(r2, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testz	p0, r2, #RENDER_IFLAGS_RENDERINPROGRESS;
	p0	ba	HK_CtxSuspend_3DIdle;
	
	MK_LOAD_STATE(r2, s3DContext, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	xor.testz	p0, r0, r2;
	!p0 ba	HK_CtxSuspend_3DIdle;
	{
		START3DCONTEXTSWITCH(HK_CtxSuspend);
	}
	HK_CtxSuspend_3DIdle:

	/* Check running TA (if any). */
	MK_LOAD_STATE(r2, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testz	p0, r2, #TA_IFLAGS_TAINPROGRESS;
	p0	ba	HK_CtxSuspend_TAIdle;

	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderContext)], drc0;
	wdf drc0;
	xor.testz	p0, r0, r2;
	!p0 ba	HK_CtxSuspend_TAIdle;
	{
		STARTTACONTEXTSWITCH(HK_CtxSuspend);
	}
	HK_CtxSuspend_TAIdle:	

	ba HostKickProcessed;
}


#if defined(SGX_FEATURE_2D_HARDWARE)
.export HostKick2D;
HostKick2D:
{
	MK_TRACE(MKTC_HOSTKICK_2D, MKTF_SCH | MKTF_2D | MKTF_HK);

	/* Get 2D context address out of CCB command */
	mov		r1, CCB(ui32Data[1]);

	MK_TRACE_REG(r1, MKTF_2D);

	MK_MEM_ACCESS(ldad)	r2, [r1, #DOFFSET(SGXMKIF_HW2DCONTEXT.ui32Count)], drc0;
	wdf		drc0;
	and.testnz	p0, r2, r2;
	iaddu32		r2, r2.low, #1;
	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HW2DCONTEXT.ui32Count)], r2;
	idf		drc0, st;
	wdf		drc0;

	p0	ba		HK_2DAlreadyAdded;
	{
		MK_MEM_ACCESS(ldad)	r2, [r1, #DOFFSET(SGXMKIF_HW2DCONTEXT.sCommon.ui32Priority)], drc0;
		wdf		drc0;

		MK_LOAD_STATE_INDEXED(r5, s2DContextTail, r2, drc0, r4);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and.testz	p0, r5, r5;
		!p0 ba	HK2D_NoHeadUpdate;
		{
			MK_STORE_STATE_INDEXED(s2DContextHead, r2, r1, r3);
		}
		HK2D_NoHeadUpdate:
		MK_STORE_STATE_INDEXED(s2DContextTail, r2, r1, r4);
		!p0	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HW2DCONTEXT.sPrevDevAddr)], r5;
		!p0	MK_MEM_ACCESS(stad)	[r5, #DOFFSET(SGXMKIF_HW2DCONTEXT.sNextDevAddr)], r1;
		idf		drc0, st;
		wdf		drc0;
	}
	HK_2DAlreadyAdded:

	ba HostKickProcessed;
}
#endif


.export HostKickTransfer;
HostKickTransfer:
{
	MK_TRACE(MKTC_HOSTKICK_TRANSFER, MKTF_SCH | MKTF_TQ | MKTF_HK);

	/* Get transfer context address out of CCB command */
	mov		r1, CCB(ui32Data[1]);

	MK_MEM_ACCESS(ldad)	r2, [r1, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.ui32Count)], drc0;
	wdf		drc0;
	and.testnz	p0, r2, r2;
	iaddu32		r2, r2.low, #1;
	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.ui32Count)], r2;
	idf		drc0, st;

	p0	ba		HK_TransferAlreadyAdded;
	{
		MK_MEM_ACCESS(ldad)	r2, [r1, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sCommon.ui32Priority)], drc0;
		wdf		drc0;

		MK_LOAD_STATE_INDEXED(r5, sTransferContextTail, r2, drc0, r4);
		MK_WAIT_FOR_STATE_LOAD(drc0);

		and.testz	p0, r5, r5;
		!p0 ba	HKT_NoTCHeadUpdate;
		{
			MK_STORE_STATE_INDEXED(sTransferContextHead, r2, r1, r3);
		}
		HKT_NoTCHeadUpdate:
		MK_STORE_STATE_INDEXED(sTransferContextTail, r2, r1, r4);
		!p0	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sPrevDevAddr)], r5;
		!p0	MK_MEM_ACCESS(stad)	[r5, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sNextDevAddr)], r1;
		idf		drc0, st;
	}
	HK_TransferAlreadyAdded:
	/* Wait for writes to complete */
	wdf		drc0;

	ba HostKickProcessed;
}


.export HostKickTA;
HostKickTA:
{
	/*
		Increment TA kick count for this render context and add current
		render context to partial context list if not already in there
	*/
	MK_TRACE(MKTC_HOSTKICK_TA, MKTF_SCH | MKTF_TA | MKTF_HK);

	/* Get render context address out of CCB command */
	mov		r1, CCB(ui32Data[1]);

	MK_MEM_ACCESS(ldad)	r2, [r1, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32TACount)], drc0;
	wdf		drc0;
	and.testnz	p0, r2, r2;
	iaddu32	r2, r2.low, #1;
	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32TACount)], r2;

	/* Trace the render context pointer and (new) number of outstanding TAs. */
	MK_TRACE(MKTC_HKTA_RENDERCONTEXT, MKTF_TA | MKTF_HK | MKTF_RC);
	MK_TRACE_REG(r1, MKTF_TA | MKTF_HK | MKTF_RC);
	MK_TRACE_REG(r2, MKTF_TA | MKTF_HK | MKTF_RC);

	/* only add to list, if this is only kick and context has no partially TA'd scene */
	p0	ba		HK_TAAlreadyAdded;
	{
		/* check whether the context has any mid TA renders in the list */
		MK_MEM_ACCESS(ldad)	r2, [r1, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPartialRendersHead)], drc0;
		wdf		drc0;
		and.testnz	p0, r2, r2;
		p0	ba		HK_TAAlreadyAdded;
		{
			MK_MEM_ACCESS(ldad)	r2, [r1, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sCommon.ui32Priority)], drc0;
			wdf		drc0;

			/* Get the Head and Tail */
			MK_LOAD_STATE_INDEXED(r5, sPartialRenderContextTail, r2, drc0, r4);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			and.testz	p0, r5, r5;
			!p0 ba	HKTA_NoHeadUpdate;
			{
				MK_STORE_STATE_INDEXED(sPartialRenderContextHead, r2, r1, r3);
			}
			HKTA_NoHeadUpdate:
			MK_STORE_STATE_INDEXED(sPartialRenderContextTail, r2, r1, r4);
			!p0	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], r5;
			!p0	MK_MEM_ACCESS(stad)	[r5, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], r1;
		}
	}
	HK_TAAlreadyAdded:
	idf		drc0, st;
	wdf		drc0;
	ba HostKickProcessed;
}


.export HostKickCleanup;
HostKickCleanup:
{
	/* Load the outstanding host requests value */
	MK_LOAD_STATE(r1, ui32HostRequest, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	xor.testz		p0, CCB(ui32Data[0]), #PVRSRV_CLEANUPCMD_RT;
	!p0	ba			HK_NotCleanupRT;
	{
		MK_TRACE(MKTC_HOSTKICK_CLEANUP_RT, MKTF_SCH | MKTF_HK | MKTF_CU);
		or			r1, r1, #SGX_UKERNEL_HOSTREQUEST_CLEANUP_RT;
		ba			HK_CleanupDone;
	}
	HK_NotCleanupRT:
	xor.testz		p0, CCB(ui32Data[0]), #PVRSRV_CLEANUPCMD_RC;
	!p0	ba			HK_NotCleanupRC;
	{
		MK_TRACE(MKTC_HOSTKICK_CLEANUP_RC, MKTF_SCH | MKTF_HK | MKTF_CU);
		MK_TRACE_REG(CCB(ui32Data[1]), MKTF_RC | MKTF_CU);
		or			r1, r1, #SGX_UKERNEL_HOSTREQUEST_CLEANUP_RC;
		ba			HK_CleanupDone;
	}
	HK_NotCleanupRC:
	xor.testz		p0, CCB(ui32Data[0]), #PVRSRV_CLEANUPCMD_TC;
	!p0	ba			HK_NotCleanupTC;
	{
		MK_TRACE(MKTC_HOSTKICK_CLEANUP_TC, MKTF_SCH | MKTF_HK | MKTF_CU);
		or			r1, r1, #SGX_UKERNEL_HOSTREQUEST_CLEANUP_TC;
		ba			HK_CleanupDone;
	}
	HK_NotCleanupTC:
	xor.testz		p0, CCB(ui32Data[0]), #PVRSRV_CLEANUPCMD_2DC;
	!p0	ba			HK_NotCleanup2DC;
	{
		MK_TRACE(MKTC_HOSTKICK_CLEANUP_2DC, MKTF_SCH | MKTF_HK | MKTF_CU);
		MK_TRACE_REG(CCB(ui32Data[1]), MKTF_2D | MKTF_CU);
		or			r1, r1, #SGX_UKERNEL_HOSTREQUEST_CLEANUP_2DC;
		ba			HK_CleanupDone;
	}
	HK_NotCleanup2DC:
	xor.testz		p0, CCB(ui32Data[0]), #PVRSRV_CLEANUPCMD_PB;
	!p0	ba			HK_NotCleanupPB;
	{
		MK_TRACE(MKTC_HOSTKICK_CLEANUP_PB, MKTF_SCH | MKTF_HK | MKTF_CU);
		or			r1, r1, #SGX_UKERNEL_HOSTREQUEST_CLEANUP_PB;
		ba			HK_CleanupDone;
	}
	HK_NotCleanupPB:

	HK_CleanupDone:
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sResManCleanupData)], CCB(ui32Data[1]);
	MK_STORE_STATE(ui32HostRequest, r1);
	idf		drc0, st;
	wdf		drc0;

	bal				CheckHostCleanupRequest;

	ba HostKickProcessed;
}


.export HostKickProcess_Queues;
HostKickProcess_Queues:
{
	MK_TRACE(MKTC_HOSTKICK_PROCESS_QUEUES, MKTF_SCH | MKTF_HK);
	ba	HostKickProcessed;
}


.export HostKickSetHWPerfStatus;
HostKickSetHWPerfStatus:
{
	MK_TRACE(MKTC_HOSTKICK_SETHWPERFSTATUS, MKTF_SCH | MKTF_HK);

	/* Reset the ordinal. */
	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, HSH(DOFFSET(SGXMK_TA3D_CTL.sHWPerfCBDevVAddr))], drc0;
	mov				r1, #0xFFFFFFFF;
	wdf				drc0;
	MK_MEM_ACCESS_BPCACHE(stad)	[r0, #DOFFSET(SGXMKIF_HWPERF_CB.ui32Ordinal)], r1;
	
	/* p0 indicates to UpdateHWPerfRegs whether to clear the hw counters. */
	and.testnz	p0,	CCB(ui32Data[0]), #PVRSRV_SGX_HWPERF_STATUS_RESET_COUNTERS;
	!p0	ba	HK_SHWPS_NoReset;
	{
		MK_TRACE(MKTC_HWP_CLEARCOUNTERS, MKTF_SCH | MKTF_HK);
	}
	HK_SHWPS_NoReset:

	/* Save the new state. */
	and	r0,	CCB(ui32Data[0]), #~PVRSRV_SGX_HWPERF_STATUS_RESET_COUNTERS;
	MK_STORE_STATE(ui32HWPerfFlags, r0);
	MK_WAIT_FOR_STATE_STORE(drc0);

	/* Reprogram the hardware. */
	bal UpdateHWPerfRegs;
	
	ba	HostKickProcessed;
}


/*****************************************************************************
 HostKickProcessed - common processing required after a new host kick has been
						parsed and handled.

 temps:	r0

*****************************************************************************/
HostKickProcessed:
{
	/* Reset active power management state. */
	MK_LOAD_STATE(r0, ui32ActivePowerFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testnz p0, r0, r0;
	p0 ba HK_NoActivityStateChange;
	{
		/*
			Active power countdown was in progress but will no longer be,
			so notify the host that the activity state has changed.
		*/
		MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32InterruptFlags)], drc0;
		wdf				drc0;
		and				r0, r0, #~(PVRSRV_USSE_EDM_INTERRUPT_IDLE | PVRSRV_USSE_EDM_INTERRUPT_ACTIVE_POWER);
		MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32InterruptFlags)], r0;
		idf				drc0, st;
		wdf				drc0;

		/* Send iterrupt to host. */
		mov	 			r0, #EUR_CR_EVENT_STATUS_SW_EVENT_MASK;
		str	 			#EUR_CR_EVENT_STATUS >> 2, r0;
	}
	HK_NoActivityStateChange:
	
	/*
		We expect the kicking host thread to always set the APM bit on
		ui32InteruptClearFlags as a means to prevent other host threads
		from taking subsequent APM action. Now that APM bit is cleared
		from ui32InterruptFlags (if needed), we proceed to clear from
		ui32InterruptClearFlags.
	*/
	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32InterruptClearFlags)], drc0;
	wdf				drc0;
	and				r0, r0, #~PVRSRV_USSE_EDM_INTERRUPT_ACTIVE_POWER;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32InterruptClearFlags)], r0;
	wdf				drc0;

	/*
		Only reset APM for non-power-off kicks. Otherwise CheckHostPowerRequest
		called on next timer event will think that a new render command slipped
		in and will request immediate restart.
	*/
	MK_LOAD_STATE(r0, ui32HostRequest, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testnz	p0, r0, #SGX_UKERNEL_HOSTREQUEST_POWER;
	p0 ba		HK_PowerKick;
	{
		MK_STORE_STATE(ui32ActivePowerCounter, #0);
		MK_STORE_STATE(ui32ActivePowerFlags, #SGX_UKERNEL_APFLAGS_ALL);
		MK_WAIT_FOR_STATE_STORE(drc0);
	}
	HK_PowerKick:

	MK_LOAD_STATE(r0, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	shl.testns	p0, r0, #(31 - TA_IFLAGS_OOM_PR_BLOCKED_BIT);
	p0	ba	HK_NoPRBlocked;
	{
		/* clear the flag */
		and	r0, r0, ~#TA_IFLAGS_OOM_PR_BLOCKED;
		MK_STORE_STATE(ui32ITAFlags, r0);

		/* restart the TA */
		str		#EUR_CR_DPM_OUTOFMEM >> 2, #EUR_CR_DPM_OUTOFMEM_RESTART_MASK;
		MK_TRACE(MKTC_RESTARTTANORENDER, MKTF_SCH | MKTF_HK | MKTF_HW);

		/* wait for the stores */
		idf		drc0, st;
		wdf		drc0;
	}
	HK_NoPRBlocked:

#if defined(SGX_FEATURE_2D_HARDWARE)
	FIND2D();
#endif

	/* Check for renders & Transfer commands that can be started */
	FIND3D();

	/* Check for TA commands that can be started */
	FINDTA();

	MK_TRACE(MKTC_HOSTKICK_END, MKTF_SCH | MKTF_HK);

#if !defined(FIX_HW_BRN_29104)
	UKERNEL_PROGRAM_PHAS();
#endif

	/* Done */
	mov.end	r0, r0;
}


/*****************************************************************************
 HostKickGetMiscInfo routine	- write SGX core information to memory buffer

 inputs:	CCB(ui32Data[1]) - device V addr of memory buffer
 			(it is necessary to allocate five DWORDS of device addressable memory)

 temps:	r2		- base device virtual address of PVRSRV_SGX_MISCINFO_INFO
 		r1,r3	- temps for core ID/features/DDK version etc

*****************************************************************************/
.export HostKickGetMiscInfo;
HostKickGetMiscInfo:
{
	MK_TRACE(MKTC_HOSTKICK_GETMISCINFO, MKTF_SCH | MKTF_HK);

	/* r2 is base address of PVRSRV_SGX_MISCINFO_INFO */
	mov		r2, CCB(ui32Data[1]);

	/* DWORD 1 and 2. Copy the core revision/ID into the output buffer */
	ldr		r1, #EUR_CR_CORE_REVISION >> 2, drc0;
	ldr		r3, #EUR_CR_CORE_ID >> 2, drc0;
	wdf		drc0;

	MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXFeatures.ui32CoreRev)], r1;
	MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXFeatures.ui32CoreID)], r3;

	/* DWORD 3. Copy the DDK version (major.minor.branch) into the output buffer */
	mov		r1, 	#PVRVERSION_MAJ << 16;
	or  	r1, r1, #PVRVERSION_MIN << 8;
	or		r1, r1, #PVRVERSION_BRANCH;

	MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXFeatures.ui32DDKVersion)], r1;

	/* DWORD 4. Copy the DDK build version into the output buffer */
	mov		r1, #PVRVERSION_BUILD;
	MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXFeatures.ui32DDKBuild)], r1;

	/* DWORD 5. Copy the software Core Id into the output buffer */
	mov		r1, #SGX_CORE_NUM_ID;
	MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXFeatures.ui32CoreIdSW)], r1;

	/* DWORD 6. Copy the software Core Revision into the output buffer */
	mov	r1,	#SGX_SW_CORE_REV_MAJ << 16;
	or  	r1, r1, #SGX_SW_CORE_REV_MIN << 8;
	or	r1, r1, #SGX_SW_CORE_REV_MAINT;
	MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXFeatures.ui32CoreRevSW)], r1;

	/* DWORD 7. Bitfield to store compiler options. See documentation for details */
	mov		r1, #SGX_BUILD_OPTIONS;
	MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXFeatures.ui32BuildOptions)], r1;

	/* check the 'get structure sizes' bit -- only used during driver initialisation */
	MK_MEM_ACCESS_BPCACHE(ldad)	r3, [r2], drc0;
	wdf		drc0;
	and.testnz		p0, r3, #PVRSRV_USSE_MISCINFO_GET_STRUCT_SIZES;
	!p0	ba		GMI_NoStructureSizes;
	{
		/* Structure sizes for SGX device initialisation compatibility check */
		mov		r1, #SIZEOF(SGXMKIF_HOST_CTL);
		MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXStructSizes.ui32Sizeof_HOST_CTL)], r1;
		mov		r1, #SIZEOF(SGXMKIF_COMMAND);
		MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXStructSizes.ui32Sizeof_COMMAND)], r1;

#if defined (SGX_FEATURE_2D_HARDWARE)
		mov		r1, #SIZEOF(SGXMKIF_2DCMD);
		MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXStructSizes.ui32Sizeof_2DCMD)], r1;
		mov		r1, #SIZEOF(SGXMKIF_2DCMD_SHARED);
		MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXStructSizes.ui32Sizeof_2DCMD_SHARED)], r1;
#endif
		mov		r1, #SIZEOF(SGXMKIF_CMDTA);
		MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXStructSizes.ui32Sizeof_CMDTA)], r1;
		mov		r1, #SIZEOF(SGXMKIF_CMDTA_SHARED);
		MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXStructSizes.ui32Sizeof_CMDTA_SHARED)], r1;
		mov		r1, #SIZEOF(SGXMKIF_TRANSFERCMD);
		MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXStructSizes.ui32Sizeof_TRANSFERCMD)], r1;
		mov		r1, #SIZEOF(SGXMKIF_TRANSFERCMD_SHARED);
		MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXStructSizes.ui32Sizeof_TRANSFERCMD_SHARED)], r1;

		mov		r1, #SIZEOF(SGXMKIF_3DREGISTERS);
		MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXStructSizes.ui32Sizeof_3DREGISTERS)], r1;
		mov		r1, #SIZEOF(SGXMKIF_HWPBDESC);
		MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXStructSizes.ui32Sizeof_HWPBDESC)], r1;
		mov		r1, #SIZEOF(SGXMKIF_HWRENDERCONTEXT);
		MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXStructSizes.ui32Sizeof_HWRENDERCONTEXT)], r1;
		mov		r1, #SIZEOF(SGXMKIF_HWRENDERDETAILS);
		MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXStructSizes.ui32Sizeof_HWRENDERDETAILS)], r1;
		mov		r1, #SIZEOF(SGXMKIF_HWRTDATA);
		MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXStructSizes.ui32Sizeof_HWRTDATA)], r1;
		mov		r1, #SIZEOF(SGXMKIF_HWRTDATASET);
		MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXStructSizes.ui32Sizeof_HWRTDATASET)], r1;
		mov		r1, #SIZEOF(SGXMKIF_HWTRANSFERCONTEXT);
		MK_MEM_ACCESS_BPCACHE(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXStructSizes.ui32Sizeof_HWTRANSFERCONTEXT)], r1;
	}
	GMI_NoStructureSizes:

	/* Wait for stores to complete */
	idf		drc0, st;
	wdf		drc0;

#if defined(SUPPORT_SGX_EDM_MEMORY_DEBUG)
	/* check request to read device memory */
	MK_MEM_ACCESS_BPCACHE(ldad)	r3, [r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.ui32MiscInfoFlags)], drc1;
	wdf		drc1;
	and.testz	p1, r3, #PVRSRV_USSE_MISCINFO_MEMREAD;
	p1	ba		GMI_NoMemRead;
	{
		MK_TRACE(MKTC_GETMISCINFO_MEMREAD_START, MKTF_SCH);
		MK_MEM_ACCESS_BPCACHE(ldad)	r4, [r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXMemAccessSrc.sDevVAddr.uiAddr)], drc1;
		MK_MEM_ACCESS_BPCACHE(ldad)	r5, [r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXMemAccessSrc.sPDDevPAddr.uiAddr)], drc1;
		wdf		drc1;

#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
		/* set up dir list PD phys addr for EDM */
		SETUPDIRLISTBASEEDM();
#else
		ldr		r6, #EUR_CR_BIF_DIR_LIST_BASE0 >> 2, drc0;
		wdf		drc0;
		xor.testz p0, r5, r6;
		p0	ba	GMI_NoSwitch;
		{
			MK_LOAD_STATE(r0, ui32ITAFlags, drc1);
			MK_LOAD_STATE(r1, ui32IRenderFlags, drc1);
			MK_WAIT_FOR_STATE_LOAD(drc1);
			
			/* If the core is idle it is safe to change address space */
			or	r0, r0, r1;
			and.testz	p0, r0, #RENDER_IFLAGS_RENDERINPROGRESS;
			p0	ba	GMI_SwitchAddrSpace;
			{
				/*
				 * unable to read from another context as the core is active, set error status
				 */
				or	r3, r3, #PVRSRV_USSE_MISCINFO_MEMREAD_FAIL;
				ba	GMI_NoMemRead;
			}
			GMI_SwitchAddrSpace:
			
			/* Switch to the required memory address space */
			str		#EUR_CR_BIF_DIR_LIST_BASE0 >> 2, r5;
			
			/* Invalidate the BIF caches */
			mov		r16, #1;
			PVRSRV_SGXUTILS_CALL(InvalidateBIFCache);
		}
		GMI_NoSwitch:
#endif
		/* r4 is the address to read in external memory */
		MK_MEM_ACCESS_CACHED(ldad) 	r1, [r4], drc0;
		wdf		drc0;

		/* read completed -- are we writing back to kernel data heap or
		 * a user-defined heap?
		 */
		and.testz	p1, r3, #PVRSRV_USSE_MISCINFO_MEMWRITE;
		p1	ba		GMI_NoUserMemWrite;
		{
			/* user-defined heap -- stay on current context */
			MK_TRACE(MKTC_GETMISCINFO_MEMWRITE_START, MKTF_SCH);
			MK_MEM_ACCESS_BPCACHE(ldad)	r4, [r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXMemAccessDest.sDevVAddr.uiAddr)], drc1;
			wdf		drc1;
			MK_MEM_ACCESS_CACHED(stad) 	[r4], r1;
			MK_TRACE(MKTC_GETMISCINFO_MEMWRITE_END, MKTF_SCH);
		}
		GMI_NoUserMemWrite:
		/* using kernel data heap for write */
		
#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
		/* switch back to EDM context and store dword for client */
		SWITCHEDMBACK();
#else
		/* We have done the read, so switch back to the kernel address space */
		SGXMK_TA3DCTRL_GETDEVPADDR(r4, r5, drc0);
		str             #EUR_CR_BIF_DIR_LIST_BASE0 >> 2, r4;	
#endif
		MK_MEM_ACCESS_CACHED(stad)	[r2, #DOFFSET(PVRSRV_SGX_MISCINFO_INFO.sSGXFeatures.ui32DeviceMemValue)], r1;
		MK_TRACE(MKTC_GETMISCINFO_MEMREAD_END, MKTF_SCH);
	}
	GMI_NoMemRead:

	/* Wait for stores to complete */
	idf		drc0, st;
	wdf		drc0;
#endif /* SUPPORT_SGX_EDM_MEMORY_DEBUG */
	/* Let the driver know the feature data is available */
	or	r3, r3, #PVRSRV_USSE_MISCINFO_READY;
	MK_MEM_ACCESS_BPCACHE(stad)	[r2], r3;
	idf		drc0, st;
	wdf		drc0;


	ba	HostKickProcessed;
}


#if defined(SGX_FEATURE_DATA_BREAKPOINTS)
/*****************************************************************************
 HostKickDataBreakpoint routine	- sets/clears a data breakpoint

 inputs:
 	CCB(ui32Data[0]) - BP index
 	CCB(ui32Data[1]) - BP start reg
 	CCB(ui32Data[2]) - BP end reg
 	CCB(ui32Data[3]) - BP ctrl reg
	Note: maps to r6, r7, r8, r9

 temps:
 	r1, r2

*****************************************************************************/
.export HostKickDataBreakpoint;
HostKickDataBreakpoint:
{
	MK_TRACE(MKTC_HOSTKICK_DATABREAKPOINT, MKTF_SCH | MKTF_HK);

	/* calc offset to first register */	
	mov		r1, #EUR_CR_BREAKPOINT0_START >> 2;
	imae	r1, CCB(ui32Data[0]).low, #3, r1, u32;
	/* write BP start reg */
	str		r1, CCB(ui32Data[1]);
	REG_INCREMENT(r1, p0);
	/* write BP end reg */
	str		r1, CCB(ui32Data[2]);
	REG_INCREMENT(r1, p0);
	/* write BP control reg */
	str		r1, CCB(ui32Data[3]);

#if defined(SGX_FEATURE_MP)
/* BEGIN workaround for lack of wiring from bank0 to hydra on SGX543 */	
/* TODO - this version of code is required for current head of trunk RTL -- We'll add the BRN(s) to the errata or delete this workaround as required. */
/* See BRN29244 */
	/* calc offset to first register */	
	mov		r1, #EUR_CR_MASTER_BREAKPOINT0_START >> 2;
	imae	r1, CCB(ui32Data[0]).low, #3, r1, u32;
	/* write BP start reg (Hydra version) */
	str		r1, CCB(ui32Data[1]);
	REG_INCREMENT(r1, p0);
	/* write BP end reg (Hydra version) */
	str		r1, CCB(ui32Data[2]);
	REG_INCREMENT(r1, p0);
	/* write BP control reg (Hydra version) */
#if EUR_CR_MASTER_BREAKPOINT0_MASK_DM_SHIFT == 4 && EUR_CR_BREAKPOINT0_MASK_DM_SHIFT == 3 && EUR_CR_BREAKPOINT0_CTRL_RENABLE_SHIFT == 0 && EUR_CR_MASTER_BREAKPOINT0_CTRL_RENABLE_SHIFT == 0
	/* We need to move bits [5:3] to bits [6:4] leaving bits [2:0] where they are. */	
	/* We do this (perhaps slightly obscurely) by adding the upper field
	   to itself (doubling it == shift left 1) and adding 1 copy of
	   the lower field (by masking it out from one of the operands of
	   the add) */
/* See BRN29245 */
	and             r2, CCB(ui32Data[3]), ~#7;
	imae            r2, r2.low, #1, CCB(ui32Data[3]), u32;
	str		r1, r2;
#else
#if EUR_CR_MASTER_BREAKPOINT0_MASK_DM_SHIFT != EUR_CR_BREAKPOINT0_MASK_DM_SHIFT;
#error Workaround for BRN29245 is no longer adequate;
#else
	/* Workaround for BRN29245 is not required */
	str		r1, CCB(ui32Data[3]);
#endif
#endif
/* END workaround for lack of wiring from bank0 to hydra on SGX543 */
#endif /* SGX_FEATURE_MP */

	/* signal completion to driver */
	MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32BPSetClearSignal)], #1;
	idf		drc0, st;
	wdf		drc0;

	ba	HostKickProcessed;
}
#endif

/*****************************************************************************
 End of file (eventhandler.use.asm)
*****************************************************************************/
