/*****************************************************************************
 Name		: spm.asm

 Title		: USE program for handling SPM related events

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

 Description 	: USE program for handling all SPM events

 Program Type	: USE assembly language

 Version 	: $Revision: 1.228 $

 Modifications	:

 $Log: spm.asm $

 *****************************************************************************/

#include "usedefs.h"

/***************************
 Sub-routine Imports
***************************/
.import MKTrace;

.import	IdleCore;
.import Resume;
.import DPMStateLoad;
.import DPMStateClear;
.import DPMStateStore;
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
.import EmitLoopBack;

.import	CheckTAQueue;
.import Check3DQueues;
#if defined(SGX_FEATURE_2D_HARDWARE)
.import Check2DQueue;
#endif

#if defined(SGX_FEATURE_MK_SUPERVISOR)
.import JumpToSupervisor;
#endif

#if defined(SGX_FEATURE_SYSTEM_CACHE)
.import InvalidateSLC;
#endif

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
.import InvalidateExtSysCache;
#endif

#if defined(FIX_HW_BRN_23862)
.import FixBRN23862;
#endif
#if defined(FIX_HW_BRN_23055)
.import LoadTAPB;
.import DPMReloadFreeList;
.import DPMPauseAllocations;
.import DPMResumeAllocations;
#else
#if 1 // SPM_PAGE_INJECTION
.import DPMReloadFreeList;
.import DPMPauseAllocations;
.import DPMResumeAllocations;
#endif
#endif /* defined(FIX_HW_BRN_23055)*/
#if (defined(FIX_HW_BRN_24304) || defined(FIX_HW_BRN_27510)) && defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
.import	ConfigureCache;
#endif
#if defined(FIX_HW_BRN_27311) && !defined(FIX_HW_BRN_23055)
.import ResetBRN27311StateTable;
#endif
.import CommonProgramEnd;
.import WaitForReset;
.import WriteHWPERFEntry;
/***************************
 Sub-routine Exports
***************************/
.export SPMEventHandler;
.export	SPMHandler;

.modulealign 2;
/*****************************************************************************
 SPMEventHandler routine - deals with the TA being unable to allocate pages
			due to reaching the software specified threshold

 inputs:
 temps:
*****************************************************************************/
SPMEventHandler:
{
	UKERNEL_PROGRAM_INIT_COMMON();

	/* Test the mem threshold reached event */
	and		r0, PA(SGX_MK_PDS_SPM_PA), #(SGX_MK_PDS_GBLOUTOFMEM | SGX_MK_PDS_MTOUTOFMEM);
	and.testnz	p0, r0, r0;
	!p0	ba		SPMEH_NoOutOfMem;
	{
		MK_TRACE(MKTC_SPMEVENT_OUTOFMEM, MKTF_EV | MKTF_SPM);

		#if defined(DBGBREAK_ON_SPM) || defined(FIX_HW_BRN_27511)
		MK_ASSERT_FAIL(MKTC_SPMEVENT_OUTOFMEM);
		#endif /* DBGBREAK_ON_SPM */

		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, #(EUR_CR_EVENT_HOST_CLEAR_DPM_OUT_OF_MEMORY_GBL_MASK | \
								     			EUR_CR_EVENT_HOST_CLEAR_DPM_OUT_OF_MEMORY_MT_MASK);

	#if defined(FIX_HW_BRN_29424) || defined(FIX_HW_BRN_29461) || defined(FIX_HW_BRN_31109) || \
		(!defined(FIX_HW_BRN_23055) && !defined(SGX_FEATURE_MP))
		/* Record the event type so we can decide the abort type later */
		MK_LOAD_STATE(r1, ui32ITAFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		
		mov		r2, #SGX_MK_PDS_GBLOUTOFMEM;
		and.testnz	p1, r0, r2;
		p1	or	r1, r1, #TA_IFLAGS_OUTOFMEM_GLOBAL;
		!p1	and r1, r1, ~#TA_IFLAGS_OUTOFMEM_GLOBAL;
		
		/* Update the flags */
		MK_STORE_STATE(ui32ITAFlags, r1);
		MK_WAIT_FOR_STATE_STORE(drc1);
	#endif
	
#if defined(FIX_HW_BRN_23055)
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32MTETEDeadlockTicks)], #0;
		idf				drc0, st;
		wdf				drc0;
#endif

		mov		R_EDM_PCLink, #LADDR(SPMEH_NoOutOfMem);
		ba		TAOutOfMem;
	}
	SPMEH_NoOutOfMem:

	/* Test the TA terminate complete event */
	and		r0, PA(SGX_MK_PDS_SPM_PA), #SGX_MK_PDS_TATERMINATE;
	and.testnz	p0, r0, r0;
	!p0 ba	SPMEH_NoTATerminate;
	{
		MK_TRACE(MKTC_SPMEVENT_TATERMINATE, MKTF_EV | MKTF_SPM);

		mov		r0, #EUR_CR_EVENT_HOST_CLEAR_TA_TERMINATE_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r0;

		mov		R_EDM_PCLink, #LADDR(SPMEH_NoTATerminate);
		ba		SPMAbortComplete;
	}
	SPMEH_NoTATerminate:
	
	/* End the program */
	UKERNEL_PROGRAM_END(#0, MKTC_SPMEVENT_END, MKTF_EV | MKTF_SPM);
}
/*****************************************************************************
 SPMHandler routine	- deals with the TA being unable to allocate pages
			due to reaching the software specified threshold

 inputs:
 temps:
*****************************************************************************/
.align 2;
SPMHandler:
{
	UKERNEL_PROGRAM_INIT_COMMON();

	CLEARPENDINGLOOPBACKS(r0, r1);

	/* Test the mem threshold reached event */
	and		r0, PA(ui32PDSIn0), #(USE_LOOPBACK_GBLOUTOFMEM | USE_LOOPBACK_MTOUTOFMEM);
	and.testnz	p0, r0, r0;
	!p0	ba		NoOutOfMem;
	{
		MK_TRACE(MKTC_SPMLB_OUTOFMEM, MKTF_EV | MKTF_SPM);
	
	#if defined(FIX_HW_BRN_31109)
		/* check the event_status, if either bit is set honour it */
		ldr		r2, #EUR_CR_EVENT_STATUS >> 2, drc0;
		wdf		drc0;
		and.testz	p0, r2, #(EUR_CR_EVENT_STATUS_DPM_OUT_OF_MEMORY_GBL_MASK | \
								EUR_CR_EVENT_STATUS_DPM_OUT_OF_MEMORY_MT_MASK);
								
		p0	ba	SPMLB_NoEventsSet;
		{
			/* There are bits set so honor them */
			and.testnz	p1, r2, #EUR_CR_EVENT_STATUS_DPM_OUT_OF_MEMORY_GBL_MASK;
			/* clear the current bit and set the correct one */
			and	r0, r0, ~#(USE_LOOPBACK_GBLOUTOFMEM | USE_LOOPBACK_MTOUTOFMEM);
			p1	or	r0, r0, #USE_LOOPBACK_GBLOUTOFMEM;
			!p1	or 	r0, r0, #USE_LOOPBACK_MTOUTOFMEM;
			
			/* Clear the flag as the HW generated the event */
			MK_LOAD_STATE(r1, ui32ITAFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			and		r1, r1, ~#TA_IFLAGS_TIMER_DETECTED_SPM;
			/* Update the flags */
			MK_STORE_STATE(ui32ITAFlags, r1);
			MK_WAIT_FOR_STATE_STORE(drc1);
		}
		SPMLB_NoEventsSet:
	#endif
	#if defined(FIX_HW_BRN_29424) || defined(FIX_HW_BRN_29461) || defined(FIX_HW_BRN_31109) || \
		(!defined(FIX_HW_BRN_23055) && !defined(SGX_FEATURE_MP))
		/* Record the event type so we can decide the abort type later */
		MK_LOAD_STATE(r1, ui32ITAFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		
		and.testnz	p1, r0, #USE_LOOPBACK_GBLOUTOFMEM;
		p1	or	r1, r1, #TA_IFLAGS_OUTOFMEM_GLOBAL;
		!p1	and r1, r1, ~#TA_IFLAGS_OUTOFMEM_GLOBAL;
		
		/* Update the flags */
		MK_STORE_STATE(ui32ITAFlags, r1);
		MK_WAIT_FOR_STATE_STORE(drc1);
	#endif

		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, #(EUR_CR_EVENT_HOST_CLEAR_DPM_OUT_OF_MEMORY_GBL_MASK | \
								     EUR_CR_EVENT_HOST_CLEAR_DPM_OUT_OF_MEMORY_MT_MASK);

		mov		R_EDM_PCLink, #LADDR(NoOutOfMem);
		ba		TAOutOfMem;
	}
	NoOutOfMem:

	/* Test the TA terminate complete event */
	and		r0, PA(ui32PDSIn0), #USE_LOOPBACK_TATERMINATE;
	and.testnz	p0, r0, r0;
	!p0	ba		NoTATerminate;
	{
		MK_TRACE(MKTC_SPMLB_TATERMINATE, MKTF_EV | MKTF_SPM);

		mov		r0, #EUR_CR_EVENT_HOST_CLEAR_TA_TERMINATE_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r0;

		mov		R_EDM_PCLink, #LADDR(NoTATerminate);
		ba		SPMAbortComplete;
	}
	NoTATerminate:

	/* Test the SPM render finished call */
	and		r0, PA(ui32PDSIn0), #USE_LOOPBACK_SPMRENDERFINISHED;
	and.testnz	p0, r0, r0;
	!p0	ba		NoSPMRenderFinished;
	{
		MK_TRACE(MKTC_SPMLB_SPMRENDERFINSHED, MKTF_EV | MKTF_SPM);

		mov		R_EDM_PCLink, #LADDR(NoSPMRenderFinished);
		ba		SPMRenderFinished;
	}
	NoSPMRenderFinished:
	
	/* End the program */
	UKERNEL_PROGRAM_END(#0, MKTC_SPMLB_END, MKTF_EV | MKTF_SPM);
}

/*****************************************************************************
 IsSceneBlocked routine	-

 inputs:	r1 - SGXMKIF_HWRENDERDETAILS of the scene
 returns: 	r2 - TRUE/FALSE
 temps:	r2, r3, r4, r5, r6, r7, r8
*****************************************************************************/
IsSceneBlocked:
{
	/* Before doing anything, check the sync objects */
	MK_MEM_ACCESS(ldad)	r2, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWDstSyncListDevAddr)], drc1;
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	ldr		r3, #EUR_CR_DPM_ABORT_STATUS_MTILE >> 2, drc0;
	wdf		drc0;
	and		r3, r3, #EUR_CR_DPM_ABORT_STATUS_MTILE_RENDERTARGETID_MASK;
	shr		r3, r3, #EUR_CR_DPM_ABORT_STATUS_MTILE_RENDERTARGETID_SHIFT;
#endif
	mov		r4, #OFFSET(SGXMKIF_HWDEVICE_SYNC_LIST.asSyncData[0]);
	/* Calculate the Address of the first PVRSRV_DEVICE_SYNC_OBJECT */
	wdf		drc1;
	and.testz	p0, r2, r2;
	p0	ba	ISB_NoDstSyncObject;
	{
		iaddu32	r2, r4.low, r2;
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
		mov	r4, #SIZEOF(PVRSRV_DEVICE_SYNC_OBJECT);
		imae	r2, r3.low, r4.low, r2, u32;
#endif
		MK_MEM_ACCESS(ldad.f2)	r3, [r2, #DOFFSET(PVRSRV_DEVICE_SYNC_OBJECT.ui32WriteOpsPendingVal)-1], drc0;
		wdf		drc0;
		and.testz	p0, r4, r4;
		p0	ba		ISB_WriteOpsNotBlocked;
		{
			MK_MEM_ACCESS_BPCACHE(ldad)	r5, [r4], drc0;
			wdf		drc0;
			xor.testz	p0, r5, r3;
			p0	ba		ISB_WriteOpsNotBlocked;
			{
				MK_TRACE(MKTC_OOM_WRITEOPSBLOCKED, MKTF_SPM | MKTF_SCH | MKTF_SO);
				ba		ISB_BlockedExit;
			}
		}
		ISB_WriteOpsNotBlocked:
		MK_MEM_ACCESS(ldad.f2)	r3, [r2, #DOFFSET(PVRSRV_DEVICE_SYNC_OBJECT.ui32ReadOps2PendingVal)-1], drc0;
		wdf		drc0;
		and.testz	p0, r4, r4;
		p0	ba		ISB_ReadOps2NotBlocked;
		{
			MK_MEM_ACCESS_BPCACHE(ldad)	r5, [r4], drc0;
			wdf		drc0;
			xor.testz	p0, r5, r3;
			p0	ba		ISB_ReadOps2NotBlocked;
			{
				MK_TRACE(MKTC_OOM_READOPS2BLOCKED, MKTF_SPM | MKTF_SCH | MKTF_SO);
				MK_TRACE_REG(r4, MKTF_SPM | MKTF_SO);
				MK_TRACE_REG(r5, MKTF_SPM | MKTF_SO);
				MK_TRACE_REG(r3, MKTF_SPM | MKTF_SO);
				ba		ISB_BlockedExit;
			}
		}
		ISB_ReadOps2NotBlocked:
		/*
			We check the first element in the structure last as we can't do -1
			to workaround the pre-increment we do ++ instead which will
			post increment r2
		*/
		MK_MEM_ACCESS(ldad.f2)	r3, [r2, #DOFFSET(PVRSRV_DEVICE_SYNC_OBJECT.ui32ReadOpsPendingVal)++], drc0;
		wdf		drc0;
		and.testz	p0, r4, r4;
		p0	ba		ISB_ReadOpsNotBlocked;
		{
			MK_MEM_ACCESS_BPCACHE(ldad)	r5, [r4], drc0;
			wdf		drc0;
			xor.testz	p0, r5, r3;
			p0	ba		ISB_ReadOpsNotBlocked;
			{
				MK_TRACE(MKTC_OOM_READOPSBLOCKED, MKTF_SPM | MKTF_SCH | MKTF_SO);
				MK_TRACE_REG(r4, MKTF_SPM | MKTF_SO);
				MK_TRACE_REG(r5, MKTF_SPM | MKTF_SO);
				MK_TRACE_REG(r3, MKTF_SPM | MKTF_SO);
				ba		ISB_BlockedExit;
			}
		}
		ISB_ReadOpsNotBlocked:
	}
	ISB_NoDstSyncObject:

	/* check the source dependencies, first get the count */
#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)
	MK_MEM_ACCESS(ldad) r2, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32Num3DSrcSyncs)], drc0;
#else
	MK_MEM_ACCESS(ldad) r2, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32NumSrcSyncs)], drc0;
#endif
	wdf		drc0;
	and.testz	p0, r2, r2;
	p0	ba	ISB_NoSrcSyncs;
	{
		/* get the base address of the first block + offset to ui32ReadOpsPendingVal */
#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)
		mov		r3, #OFFSET(SGXMKIF_HWRENDERDETAILS.as3DSrcSyncs[0].ui32ReadOpsPendingVal);
#else
		mov		r3, #OFFSET(SGXMKIF_HWRENDERDETAILS.asSrcSyncs[0].ui32ReadOpsPendingVal);
#endif
		iadd32	r3, r3.low, r1;

		/* loop through each dependency, loading a PVRSRV_DEVICE_SYNC_OBJECT structure */
		/* load ui32ReadOpsPendingVal and sReadOpsCompleteDevVAddr */
		MK_MEM_ACCESS(ldad.f4)	r4, [r3, #0++], drc0;
		ISB_CheckNextSrcDependency:
		{
			wdf	drc0;

			/* Check this is ready to go */
			MK_MEM_ACCESS_BPCACHE(ldad)	r8, [r5], drc0; // load ReadOpsComplete
			wdf		drc0;
			xor.testz	p0, r8, r4;	// Test ReadOpsComplete
			p0	ba	ISB_SrcReadOpsNotBlocked;
			{
				MK_TRACE(MKTC_OOM_SRC_READOPSBLOCKED, MKTF_SPM | MKTF_SCH | MKTF_SO);
				MK_TRACE_REG(r5, MKTF_SPM | MKTF_SO);
				MK_TRACE_REG(r8, MKTF_SPM | MKTF_SO);
				MK_TRACE_REG(r4, MKTF_SPM | MKTF_SO);
				ba		ISB_BlockedExit;
			}
			ISB_SrcReadOpsNotBlocked:

			MK_MEM_ACCESS_BPCACHE(ldad)	r8, [r7], drc0;	// load WriteOpsComplete
			wdf		drc0;
			xor.testz	p0, r8, r6;	// Test WriteOpsComplete
			p0	ba	ISB_SrcWriteOpsNotBlocked;
			{
				MK_TRACE(MKTC_OOM_SRC_WRITEOPSBLOCKED, MKTF_SPM | MKTF_SCH | MKTF_SO);
				ba		ISB_BlockedExit;
			}
			ISB_SrcWriteOpsNotBlocked:
			/* We don't need to check readops2 but we need to move the pointer passed them */
			mov	r4, #8;
			mov	r5, r3;
			IADD32(r3, r4, r5, r6);

			/* decrement the count and test for 0 */
			isub16.testz r2, p0, r2, #1;
			!p0	MK_MEM_ACCESS(ldad.f4)	r4, [r3, #0++], drc0;
			!p0	ba	ISB_CheckNextSrcDependency;
		}
	}
	ISB_NoSrcSyncs:

	/* indicate the scene is not blocked */
	mov		r2, #USE_FALSE;
	/* Return */
	lapc;

	ISB_BlockedExit:
	/* indicate the scene is blocked */
	mov		r2, #USE_TRUE;
	/* Return */
	lapc;
}


/*****************************************************************************
  IPRB
  IsPartialRenderBlocked routine -
    Check to see if there is a render task ready to be executed in the
    RENDERDETAILS list for the current RENDERCONTEXT

 inputs:	R_TA3DCtl - register holding pointer to the SGXMK_TA3D_CTL struct
 outputs:   r0 - boolean indicating that there are RENDERDETAILS items in the
                 3D queue. This indicates that partial rendering cannot be
                 done on the current scene and those renderdetails must be
                 processed first.
 temps: r8
        p1
*****************************************************************************/

IsPartialRenderBlocked:
{
	MK_MEM_ACCESS_BPCACHE(ldad)	r8, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderContext)], drc0;
	wdf drc0;
	MK_TRACE_REG(r8, MKTF_SPM | MKTF_SCH);

	MK_MEM_ACCESS(ldad)	r8, [r8, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWCompleteRendersHead)], drc0;
	wdf drc0;

	and.testz	p1, r8, r8;
	!p1	ba		IPRB_HaveDetails;
	{
		/* no RENDERDETAILS nodes for current -- active scene is not blocked for partial render */
		MK_TRACE(MKTC_IPRB_NORENDERDETAILS, MKTF_SPM | MKTF_SCH);
		mov r0, #0;
		lapc;
	}
	IPRB_HaveDetails:
	MK_TRACE(MKTC_IPRB_HAVERENDERDETAILS, MKTF_SPM | MKTF_SCH);

	/* set a TRUE flag here (partial render is blocked because there is a render on the queue) */
	mov r0, #1;
	lapc;
}

/*****************************************************************************
 TAOutOfMem routine	- deals with the TA being unable to allocate pages
			due to reaching the software specified threshold

 inputs:
 temps:
*****************************************************************************/
TAOutOfMem:
{
	/* Indicate that we have received an out of memory event */
	MK_LOAD_STATE(r0, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	or	r0, r0, #TA_IFLAGS_OUTOFMEM;	
	MK_STORE_STATE(ui32ITAFlags, r0);
	MK_WAIT_FOR_STATE_STORE(drc1);

	MK_LOAD_STATE(r5, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testnz p0, r5, #RENDER_IFLAGS_RENDERINPROGRESS;
	p0 ba TAOOM_PartialRenderIsNotBlocked;

#if defined(FIX_HW_BRN_31109) || defined(FIX_HW_BRN29461)
	ba TAOOM_PartialRenderIsNotBlocked;
#endif

#if defined(FIX_HW_BRN23055)
	and.testnz p0, r0, #TA_IFLAGS_FIND_MTE_PAGE;
	p0 ba TAOOM_PartialRenderIsNotBlocked;
#endif
	
	/* check to see whether there are any unblocked renders already in the queue */
	mov r5, pclink;
	bal IsPartialRenderBlocked;
	mov pclink, r5;
	and.testz	p0, r0, r0;

	p0 ba TAOOM_PartialRenderIsNotBlocked;
	{ /* IN: R_TA3DCtl
    	 OUT: noreturn
    	 TEMPS: r5 */
		/* IsPartialRenderBlocked returned nonzero.
           Doing a partial render would be out of order */
		MK_TRACE(MKTC_RENDER_OUT_OF_ORDER, MKTF_SPM | MKTF_SCH);

		MK_LOAD_STATE(r5, ui32IRenderFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		or		r5, r5, #RENDER_IFLAGS_TAWAITINGFORMEM;
		or		r5, r5, #RENDER_IFLAGS_PARTIAL_RENDER_OUTOFORDER;
		MK_STORE_STATE(ui32IRenderFlags, r5);
		MK_WAIT_FOR_STATE_STORE(drc0);

		MK_LOAD_STATE(r5, ui32ITAFlags, drc1);
		MK_WAIT_FOR_STATE_LOAD(drc1);
		or		r5, r5, #TA_IFLAGS_OOM_PR_BLOCKED;
		MK_STORE_STATE(ui32ITAFlags, r5);
		MK_WAIT_FOR_STATE_STORE(drc1);

		FIND3D();

		ba	TAOOM_Exit;
	}
	TAOOM_PartialRenderIsNotBlocked:
	MK_TRACE(MKTC_RENDER_NOT_OUT_OF_ORDER, MKTF_SPM | MKTF_SCH);
	TAOOM_RenderBlockedCheckDone:
	/* Reload the ITA Flags as they have changed */
	MK_LOAD_STATE(r0, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	
#if defined(FIX_HW_BRN_32302) || defined(SUPPORT_SGX_DOUBLE_PIXEL_PARTITIONS)
	mov r2, #TA_IFLAGS_ZLSTHRESHOLD_LOWERED;
	and.testz	p0, r0, r2;

	/* if (p0) ZLSThreshold was too low to lower further
	So, we did not lower and just set numVertexPartisions lower.
	*/
	p0	ba	TAOOM_RealOOM;

	mov r2, #TA_IFLAGS_ZLSTHRESHOLD_RESTORED;
	and.testnz	p0, r0, r2;

	/* if(p0) We have already restored back to
	Real Threshold, so this is a real-OOM
	*/
	p0	ba	TAOOM_RealOOM;

	or	r0, r0, #TA_IFLAGS_ZLSTHRESHOLD_RESTORED;
	MK_TRACE(MKTC_REDUCE_MAX_VTX_PARTITIONS, -1);

	MK_STORE_STATE(ui32ITAFlags, r0);
	MK_WAIT_FOR_STATE_STORE(drc0);

	MK_MEM_ACCESS_BPCACHE(ldaw)	r2, [R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16RealZLSThreshold)], drc0;
	wdf	drc0;

	str	#EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, r2;


	/* Get the TA and 3D HWPBDescs */
	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc.uiAddr)], drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DHWPBDesc.uiAddr)], drc0;
	wdf drc0;

	/* Check the TA and 3D PB match? */
	xor.testz	p0, r1, r2;
	!p0	ba 	TAOOM_ThresholdChangeNo3DIdle;
	{
		/* Because the TA and 3D PB's match, the 3D also needs to be idled! */
		mov	r16, #USE_IDLECORE_3D_REQ;
		PVRSRV_SGXUTILS_CALL(IdleCore);
	}
	TAOOM_ThresholdChangeNo3DIdle:

	PVRSRV_SGXUTILS_CALL(DPMPauseAllocations);
	PVRSRV_SGXUTILS_CALL(DPMReloadFreeList);

	ldr	r3, #EUR_CR_DMS_CTRL >> 2, drc0;	
	wdf	drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r5, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTACCBCmd)], drc0;
	wdf	drc0;
	MK_MEM_ACCESS_CACHED(ldad)	r4, [r5, #DOFFSET(SGXMKIF_CMDTA.ui32SPMNumVertexPartitions)], drc1;
	wdf	drc1;

	and	r3, r3, ~#EUR_CR_DMS_CTRL_MAX_NUM_VERTEX_PARTITIONS_MASK;
	or	r3, r3, r4;
	str	#EUR_CR_DMS_CTRL >> 2, r3;

	PVRSRV_SGXUTILS_CALL(DPMResumeAllocations);
	xor.testz	p0, r1, r2;
	!p0	ba	TAOOM_ThresholdChangeNo3DResume;
	{
		RESUME(r1);
	}
	TAOOM_ThresholdChangeNo3DResume:
	/* Restart the TA */
	str	#EUR_CR_DPM_OUTOFMEM >> 2, #EUR_CR_DPM_OUTOFMEM_RESTART_MASK;
	
	/* Exit with out and abort */
	ba	TAOOM_Exit;

TAOOM_RealOOM:
#endif
	or	r0, r0, #TA_IFLAGS_OUTOFMEM;	
	MK_STORE_STATE(ui32ITAFlags, r0);
	MK_WAIT_FOR_STATE_STORE(drc1);
	
	/* Check for an abort already in progress. */
	and.testnz		p0, r0, #TA_IFLAGS_ABORTINPROGRESS;
	p0	ba			TAOOM_Exit;

#if defined(SUPPORT_HW_RECOVERY)
	/* Reset the recovery counter, as we have received activity and will deal with it */
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRTimerTicksSinceTA)], #0;
#endif

	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderDetails)], drc0;

	/* wait for the render details to load */
	wdf		drc0;
	MK_MEM_ACCESS(ldad)	r2, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWRTDataSetDevAddr)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS(ldad)	r3, [r2, #DOFFSET(SGXMKIF_HWRTDATASET.ui32NumOutOfMemSignals)], drc0;
	wdf		drc0;
	mov		r4, #1;
	iaddu32	r3, r4.low, r3;
	MK_MEM_ACCESS(stad)	[r2, #DOFFSET(SGXMKIF_HWRTDATASET.ui32NumOutOfMemSignals)], r3;
	idf		drc0, st;
	wdf		drc0;

	MK_MEM_ACCESS_BPCACHE(ldad)	R_RTData, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc0;
	wdf		drc0;
	or		r2, r2, #SGXMKIF_HWRTDATA_CMN_STATUS_OUTOFMEM;
	MK_MEM_ACCESS(stad)	[R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], r2;
	idf		drc0, st;
	wdf		drc0;

	/* Check for renders in progress or queued */
	MK_LOAD_STATE(r2, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testz		p0, r2, #RENDER_IFLAGS_RENDERINPROGRESS;
	p0	ba		TAOOM_NoRenderInProgress;
	{
		/* Set the flag to restart the TA after the render finishes. */
		or		r2, r2, #RENDER_IFLAGS_TAWAITINGFORMEM;
		MK_STORE_STATE(ui32IRenderFlags, r2);
		MK_WAIT_FOR_STATE_STORE(drc0);

		MK_TRACE(MKTC_CSRENDERINPROGRESS, MKTF_SPM | MKTF_SCH);

		ba		TAOOM_Exit;
	}
	TAOOM_NoRenderInProgress:
	
#if defined(FIX_HW_BRN_23055)
	/* Check if the TA_IFLAGS_FIND_MTE_PAGE flag is set */
	shl.testns	p0, r0, #(31 - TA_IFLAGS_FIND_MTE_PAGE_BIT);
	p0	ba	TAOOM_NoMTEPageInternal;
	{
		/*
			This OOM has been generated to cause the CSM to be updated by the MTE.
			Now that it has been updated we need to flush it to memory and find the page
		*/
		and	r0, r0, ~#TA_IFLAGS_FIND_MTE_PAGE;
		MK_STORE_STATE(ui32ITAFlags, r0);
		MK_WAIT_FOR_STATE_STORE(drc0);
		
		/* Find the valid entry in the CSM */
		bal	FindMTEPageInCSM;


		/* Pause allocations before we start modifying the free list */
		PVRSRV_SGXUTILS_CALL(DPMPauseAllocations);

		/* Get the TA and 3D HWPBDescs */
		MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc.uiAddr)], drc0;
		MK_MEM_ACCESS_BPCACHE(ldad)	r4, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DHWPBDesc.uiAddr)], drc0;
		wdf drc0;

		/* Check the TA and 3D PB match? */
		xor.testz	p0, r1, r4;
		!p0	ba 	TAOOM_RMTEPNo3DIdle;
		{
			/* Because the TA and 3D PB's match, the 3D also needs to be idled! */
			mov	r16, #USE_IDLECORE_3D_REQ;
			PVRSRV_SGXUTILS_CALL(IdleCore);
		}
		TAOOM_RMTEPNo3DIdle:

		MK_MEM_ACCESS_BPCACHE(ldaw)	r1, [R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16LastMTEPageRgn)], drc0;
		wdf	drc0;
		mov	r2, #0xFFFF;
		xor.testnz	p0, r1, r2;
		/* If we found a page re-issue it to the context */
		p0	bal	ReIssueMTEPage;

		/*
			Now that the page has been found and removed from the free list
			we can restore the ZLS threshold and restart the TA.
		*/
		bal	RestoreZLSThreshold;

		/* Resume allocations after messing with the DPM State table and free list. */
		PVRSRV_SGXUTILS_CALL(DPMResumeAllocations);
		RESUME(r1);

		/* Restart the TA */
		str	#EUR_CR_DPM_OUTOFMEM >> 2, #EUR_CR_DPM_OUTOFMEM_RESTART_MASK;
		
		/* Exit with out and abort */
		ba	TAOOM_Exit;
	}
	TAOOM_NoMTEPageInternal:
#endif

	/* check if the scene can be rendered at this point */
	/* r1 - SGXMKIF_HWRENDERDETAILS */
	bal	IsSceneBlocked;
	/* if r2 = USE_TRUE, exit */
	and.testz	p0, r2, r2;

	/* If we've reached here this scene can be partially rendered */
	p0	ba	TAOOM_SyncObjectReady;
	{
		/* The scene can not be partially render yet as the sync objects are blocked,
			therefore, set flag to indicate this and then wait for a syncops update kick */
		or	r0, r0, #TA_IFLAGS_OOM_PR_BLOCKED;
		MK_STORE_STATE(ui32ITAFlags, r0);
		MK_WAIT_FOR_STATE_STORE(drc1);

		/* Try to unblock the render */
#if defined(SGX_FEATURE_2D_HARDWARE)
		FIND2D();
#endif
		FIND3D();

		/* Exit */
		ba		TAOOM_Exit;
	}
	TAOOM_SyncObjectReady:

	/* Clear the flag as the PR is not blocked, if it ever was */
	and	r0, r0, ~#TA_IFLAGS_OOM_PR_BLOCKED;

#if defined(FIX_HW_BRN_29461) || \
	(!defined(FIX_HW_BRN_23055) && !defined(SGX_FEATURE_MP))
	
	#if !defined(SGX_FEATURE_MP)
	/* Normal flow if TE is requesting */
	ldr			r1, #EUR_CR_DPM_REQUESTING >> 2, drc0;
	wdf			drc0;
	
	and			r1, r1, #EUR_CR_DPM_REQUESTING_SOURCE_MASK;
	and.testnz	p0,	r1, #1; /* bit0: TE requestor */
	p0			br	TAOOM_DeadlockHandler;
	#endif
	
	/* On non-MP cores without BRN 23055, we force a global OOM from MT OOM */
	shl.tests	p0, r0, #(31 - TA_IFLAGS_OUTOFMEM_GLOBAL_BIT);
	p0	ba	TAOOM_OutOfMemGbl;
	{
		MK_TRACE(MKTC_OOM_TYPE_MT, MKTF_SPM | MKTF_SCH);
	
	#if !defined(SGX_FEATURE_MP) // SPM_PAGE_INJECTION
		/* check if we have hit DPM deadlock and reserve mem not yet added */
		shl.tests			p0, r0, #(31 - TA_IFLAGS_SPM_DEADLOCK_BIT);
		p0	shl.testns		p0, r0, #(31 - TA_IFLAGS_SPM_DEADLOCK_MEM_BIT);
		p0	ba	TAOOM_DeadlockHandler;
		
		#if defined(DEBUG)
		/* Trigger after 6 OOM deadlock events which didn't free the TA (TA finish) */
		MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32ForceGlobalOOMCount)], drc0;
		mov		r3, #1;
		wdf		drc0;
		iaddu32		r2, r3.low, r2;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32ForceGlobalOOMCount)], r2;
		idf		drc1, st;
		wdf 	drc1;
		xor.testnz	p0, r2, #SGX_DEADLOCK_MEM_MAX_COUNT;
		p0		ba	TAOOM_NotStuck2;
		{
			MK_ASSERT_FAIL(MKTC_SPM_FORCE_GLOBAL_OOM_FAILED);
		}
		TAOOM_NotStuck2:
		#endif

	#endif

	#if defined(SGX_FEATURE_MP)	
		TAOOM_WaitForRequestingSource:
		{
			/* Find out what the requesting source is */
			READMASTERCONFIG(r3, #EUR_CR_MASTER_DPM_REQUESTING >> 2, drc0);
			wdf		drc0;
			and.testz	p0, r3, r3;
			p0	ba	TAOOM_WaitForRequestingSource;
		}
		
		/* if the MTE bit is set there is no need to delay  */
		and.testnz	p0, r3, #EUR_CR_DPM_REQUESTING_SOURCE_MTE;
		p0	ba	TAOOM_NoDelay;
		{
			/* Stall for a bit to make sure we detect a TE abort */
			mov		r2, #2000;
			TAOOM_BRN29461Delay:
			{
				isub16.testnz	r2, p0, r2, #1;
				p0	ba		TAOOM_BRN29461Delay;
			}
		}
		TAOOM_NoDelay:
	
		/* Find out what the requesting source is */
		READMASTERCONFIG(r3, #EUR_CR_MASTER_DPM_REQUESTING >> 2, drc0);
		wdf		drc0;
		and.testz	p0, r3, #EUR_CR_DPM_REQUESTING_SOURCE_MTE;
		!p0 shl.tests	p0, r0, #(31 - TA_IFLAGS_STORE_DPLIST_BIT);
		p0	ba	TAOOM_MTENotPresent;
	#endif /* SGX_FEATURE_MP */
		{
			MK_TRACE(MKTC_OOM_CAUSE_GBL_OOM, MKTF_SPM | MKTF_SCH);
			
			/* 
				The MTE is a requesting source and the dplist has not been stored, so:
					1) save global_threshold
					2) set global_threshold to current global_pages
					3) issue Restart
			*/
			ldr		r1, #EUR_CR_DPM_GLOBAL_PAGE_STATUS >> 2, drc0;
			ldr		r2, #EUR_CR_DPM_TA_GLOBAL_LIST >> 2, drc0;
			wdf		drc0;
			/* Saved the current global page threshold */
			MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32SavedGblPolicy)], r2;
			
			/* Set the global_threshold to the current global_pages */
			shr		r1, r1, #EUR_CR_DPM_GLOBAL_PAGE_STATUS_TA_SHIFT;
			or		r1, r1, #EUR_CR_DPM_TA_GLOBAL_LIST_POLICY_MASK;
			
			/* Pause allocations before we start messing with the free list. */
			PVRSRV_SGXUTILS_CALL(DPMPauseAllocations);
			
			str		#EUR_CR_DPM_TA_GLOBAL_LIST >> 2, r1;			
			
			/* Reload the free list so the hardware uses the new threshold. */
			PVRSRV_SGXUTILS_CALL(DPMReloadFreeList);
				
			/* Resume allocations. */
			PVRSRV_SGXUTILS_CALL(DPMResumeAllocations);
			
	#if defined(SGX_FEATURE_MP)	
			/* 
				Set the flag to indicate that the global outofmem has been generated 
				to do the dplist_store.
			*/
			or		r0, r0, #TA_IFLAGS_STORE_DPLIST;
			MK_STORE_STATE(ui32ITAFlags, r0);
			/* Make sure the stores have flushed */
			idf		drc0, st;
			wdf		drc0;
	#else
			/* 
				Set the flag. (non-MP only)
			*/
			or		r0, r0, #TA_IFLAGS_FORCING_GLOBAL_OOM;
			MK_STORE_STATE(ui32ITAFlags, r0);
			/* Make sure the stores have flushed */
			idf		drc0, st;
			wdf		drc0;
	#endif

			/* Restart the TA. */
			str		#EUR_CR_DPM_OUTOFMEM >> 2, #EUR_CR_DPM_OUTOFMEM_RESTART_MASK;

			/* Exit and wait for Gbl OUTOFMEM */
			ba	TAOOM_Exit;
		}
	#if defined(SGX_FEATURE_MP)	
		TAOOM_MTENotPresent:
		
		ba	TAOOM_NoDPListStore;
	#endif
	}
	TAOOM_OutOfMemGbl:
	/* Handle the global OOM in the normal manner on non-MP cores */
	{
		MK_TRACE(MKTC_OOM_TYPE_GLOBAL, MKTF_SPM | MKTF_SCH);
		
	#if defined(SGX_FEATURE_MP)
		/* If the STORE_DPLIST bit is set and the STORED_DPLIST is not,
			enter Phase 2 and store the DPLIST */
		shl.testns	p0, r0, #(31 - TA_IFLAGS_STORE_DPLIST_BIT);
		!p0 shl.tests	p0, r0, #(31 - TA_IFLAGS_DPLIST_STORED_BIT);
		p0	ba	TAOOM_NoDPListStore;
		{
			MK_TRACE(MKTC_CPRI_STORE_DPLIST, MKTF_SPM | MKTF_SCH);
			
			/* 
				This global abort was generated so we can store the DPLIST, so
					1) do dplist_store
					2) reset global_threshold
					3) issue Restart
			*/
			/* dump the dplist (parti pim table) */
			MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sPIMTableDevAddr)], drc0; 
			wdf		drc0;
			WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_MTILE_PARTI_PIM_TABLE_BASE_ADDR >> 2, r2, r3);
			WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_TASK_DPLIST >> 2, #EUR_CR_MASTER_DPM_TASK_DPLIST_STORE_MASK, r3);
			
			/* Poll for the DPM PIM stored event. */
			ENTER_UNMATCHABLE_BLOCK;
			TAOOM_WaitForDPListStore:
			{
				READMASTERCONFIG(r2, #EUR_CR_MASTER_DPM_DPLIST_EVENT_STATUS >> 2, drc0);
				wdf			drc0;
				and.testz	p0, r2, #EUR_CR_MASTER_DPM_DPLIST_EVENT_STATUS_STORED_MASK;
				p0	ba		TAOOM_WaitForDPListStore;
			}
			LEAVE_UNMATCHABLE_BLOCK;		
			
			/* Clear the event bit */
			WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_DPLIST_CLEAR_EVENT >> 2, #EUR_CR_MASTER_DPM_DPLIST_CLEAR_EVENT_STORE_MASK, r3);
			
			/* Pause allocations before we start messing with the free list. */
			PVRSRV_SGXUTILS_CALL(DPMPauseAllocations);
				
			/* restore the global_threshold */
			MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32SavedGblPolicy)], drc0;
			wdf		drc0;
			str		#EUR_CR_DPM_TA_GLOBAL_LIST >> 2, r2;
			
			/* Reload the free list so the hardware uses the new threshold. */
			PVRSRV_SGXUTILS_CALL(DPMReloadFreeList);
				
			/* Resume allocations. */
			PVRSRV_SGXUTILS_CALL(DPMResumeAllocations);
			
			/* 
				Set the flag to indicate we have stored the dplist and any further 
				OOM events should be handle as such.
			*/
			or		r0, r0, #TA_IFLAGS_DPLIST_STORED;
			MK_STORE_STATE(ui32ITAFlags, r0);
			MK_WAIT_FOR_STATE_STORE(drc0);
			
			/* Restart the TA. */
			str		#EUR_CR_DPM_OUTOFMEM >> 2, #EUR_CR_DPM_OUTOFMEM_RESTART_MASK;

			/* Exit and wait for original out_of_mt event to be regenerated */
			ba	TAOOM_Exit;
		}
		TAOOM_NoDPListStore:
	#else
		/* If the FORCING_GLOBAL_OOM bit is set we need to restore the global threshold */
		shl.tests	p0, r0, #(31 - TA_IFLAGS_FORCING_GLOBAL_OOM_BIT);
		!p0	ba	TAOOM_NoRestoreThreshold;
		{
			/* Pause allocations before we start messing with the free list. */
			PVRSRV_SGXUTILS_CALL(DPMPauseAllocations);
				
			/* restore the global_threshold */
			MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32SavedGblPolicy)], drc0;
			wdf		drc0;
			or		r2, r2, #EUR_CR_DPM_TA_GLOBAL_LIST_POLICY_MASK;
			
			MK_TRACE(MKTC_OOM_RESTORE_LIST_SIZE, MKTF_SPM | MKTF_SCH);
			MK_TRACE_REG(r2, MKTF_SPM | MKTF_SCH);
			
			str		#EUR_CR_DPM_TA_GLOBAL_LIST >> 2, r2;
			
			/* Reload the free list so the hardware uses the new threshold. */
			PVRSRV_SGXUTILS_CALL(DPMReloadFreeList);
				
			/* Resume allocations. */
			PVRSRV_SGXUTILS_CALL(DPMResumeAllocations);
				
			/* Clear the flag */
			and		r0, r0, ~#TA_IFLAGS_FORCING_GLOBAL_OOM;
			MK_STORE_STATE(ui32ITAFlags, r0);
			MK_WAIT_FOR_STATE_STORE(drc0);		
		}
		TAOOM_NoRestoreThreshold:
	#endif /* SGX_FEATURE_MP */
	}
#endif

#if 1 // SPM_PAGE_INJECTION
	TAOOM_DeadlockHandler:
	
	/* check if we have hit DPM deadlock */
	shl.testns		p0, r0, #(31 - TA_IFLAGS_SPM_DEADLOCK_BIT);
	p0	ba		TAOOM_NotDPMDeadlock;
	{
		MK_TRACE(MKTC_OOM_SPM_DEADLOCK, MKTF_SPM | MKTF_SCH);
		
		shl.tests	p0, r0, #(31 - TA_IFLAGS_SPM_DEADLOCK_MEM_BIT);
		p0	ba	TAOOM_ReserveAdded;
		{
			/* Add the number of un-committed pages to the global page count as the HW does this */
		#if defined(FIX_HW_BRN_23055)
			ldr		r1, #EUR_CR_DPM_PAGE_STATUS >> 2, drc0;
			ldr		r3, #EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, drc0;
			wdf		drc0;
			and		r1, r1, #EUR_CR_DPM_PAGE_STATUS_TA_MASK;
			shr		r1, r1, #EUR_CR_DPM_PAGE_STATUS_TA_SHIFT;
			isub16.tests	p0, r1, r3;
		#else
			shl.tests	p0, r0, #(31 - TA_IFLAGS_SPM_DEADLOCK_GLOBAL_BIT);
			p0	ba	TAOOM_CheckGblDeadlock;
			{			
				MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32AbortPages)], drc0;
				
				ldr		r3, #EUR_CR_DPM_PAGE_STATUS >> 2, drc0;
				wdf		drc0;
				and		r3, r3, #EUR_CR_DPM_PAGE_STATUS_TA_MASK;
				shr		r3, r3, #EUR_CR_DPM_PAGE_STATUS_TA_SHIFT;
				MK_TRACE(MKTC_SPM_CHECK_MT_DEADLOCK, MKTF_SPM | MKTF_SCH);
				isub16.testns	p0, r2, r3; // (OLD-2) - (NEW) < 0 = deadlock
				
				ba	TAOOM_CheckComplete;
			}
			TAOOM_CheckGblDeadlock:
			{
				MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32GblAbortPages)], drc0;
				
				ldr		r3, #EUR_CR_DPM_GLOBAL_PAGE_STATUS >> 2, drc0;
				wdf		drc0;
				and		r3, r3, #EUR_CR_DPM_GLOBAL_PAGE_STATUS_TA_MASK;
				shr		r3, r3, #EUR_CR_DPM_GLOBAL_PAGE_STATUS_TA_SHIFT;
				MK_TRACE(MKTC_SPM_CHECK_GLOBAL_DEADLOCK, MKTF_SPM | MKTF_SCH);
				isub16.testns	p0, r2, r3; // (OLD-2) - (NEW) < 0 = deadlock
			}
			TAOOM_CheckComplete:
		#endif
			p0	ba	TAOOM_NotDeadlock;
			{
				MK_TRACE(MKTC_OOM_SPM_DEADLOCK_MEM_ADDED, MKTF_SPM | MKTF_SCH);

				/* 
					Set the flag to reset the ZLS threshold back to normal after the render.
				*/
				or				r0, r0, #TA_IFLAGS_SPM_DEADLOCK_MEM;

				/* Are we context switching? */
				and.testnz		p0, r0, #TA_IFLAGS_HALTTA;
				p0	ba		TAOOM_ContextStoreInProgress;
				{
					/* Ensure the complete on terminate bit is set */
					ldr		r1, #EUR_CR_TE_PSG >> 2, drc0;
					wdf		drc0;
					or		r1, r1, #(EUR_CR_TE_PSG_COMPLETEONTERMINATE_MASK | \
								  EUR_CR_TE_PSG_ZSTOREENABLE_MASK);
					str		#EUR_CR_TE_PSG >> 2, r1;
				}
				TAOOM_ContextStoreInProgress:
				/* This OOM will not free any pages, therefore make use of the reserve pages and issue restart */
				/* Pause allocations before we start messing with the free list. */
				PVRSRV_SGXUTILS_CALL(DPMPauseAllocations);

				/* Increment ZLS threshold. */
	#if defined(FIX_HW_BRN_23055)
				ldr		r1, #EUR_CR_DPM_PAGE_STATUS >> 2, drc0;
				ldr		r3, #EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, drc1;
				mov		r2, #SPM_DL_RESERVE_PAGES;
				wdf		drc0;
				and		r1, r1, #EUR_CR_DPM_PAGE_STATUS_TA_MASK;
				shr		r1, r1, #EUR_CR_DPM_PAGE_STATUS_TA_SHIFT;
				iaddu32	r1, r2.low, r1;
				wdf		drc1;
				/* what is the increase going to be? */
				isub16	r2, r1, r3;
	#else
				ldr		r1, #EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, drc1;
				mov		r2, #SPM_DL_RESERVE_PAGES;
				wdf		drc1;
				iaddu32	r1, r2.low, r1;
	#endif	
				str		#EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, r1;

				/* remember the increase for later! */
				MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32DeadlockPageIncrease)], r2;

	#if !defined(FIX_HW_BRN_23055)
				/* Drop the GLOBAL threshold done to the current allocation cnt, 
					This will stop any off the pages being allocated to the global list */
				ldr		r2, #EUR_CR_DPM_TA_GLOBAL_LIST >> 2, drc0;
				wdf		drc0;
				MK_MEM_ACCESS_BPCACHE(stad) 	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32SavedGlobalList)], r2;
				and		r2, r2, #EUR_CR_DPM_TA_GLOBAL_LIST_SIZE_MASK;
				/* Allow half the pages to go to the global list */
				iaddu32	r2, r2.low, #(SPM_DL_RESERVE_PAGES / 2);
				or		r2, r2, #EUR_CR_DPM_TA_GLOBAL_LIST_POLICY_MASK;
				str		#EUR_CR_DPM_TA_GLOBAL_LIST >> 2, r2;
				
	#endif
				/* Flush the writes to memory */
				idf		drc0, st;
				wdf		drc0;

				/* Reload the free list so the hardware uses the new threshold. */
				PVRSRV_SGXUTILS_CALL(DPMReloadFreeList);
	#if defined(FIX_HW_BRN_23055)
				/* Check if the TE has been previously denied */
				shl.tests	p1, r0, #(31 - TA_IFLAGS_INCREASEDTHRESHOLDS_BIT);
				/* Don't restart VDM if both have been denied */
				p1	ba	TAOOM_DLRestartNoVDM;
	#endif
				{
					MK_TRACE(MKTC_RESUMEVDM, MKTF_SPM | MKTF_SCH);
					/* Resume allocations. */
					PVRSRV_SGXUTILS_CALL(DPMResumeAllocations);
				}
				TAOOM_DLRestartNoVDM:

	#if defined(EUR_CR_MASTER_DPM_PAGE_MANAGEOP_SERIAL_MASK)
				READMASTERCONFIG(r2, #EUR_CR_MASTER_DPM_PAGE_MANAGEOP >> 2, drc0);
				wdf		drc0;
				or		r2, r2, #EUR_CR_MASTER_DPM_PAGE_MANAGEOP_SERIAL_MASK;
				/* Activate DPM serial mode so all injected pages go to the active core */
				WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_PAGE_MANAGEOP >> 2, r2, r5);
	#endif
	#if defined(EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC)
				/* Reduce PIM spec timeout count so we don't commit too many pages */
				READMASTERCONFIG(r2, #EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, drc0);
				wdf		drc0;
				and		r2, r2, ~#EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC_TIMOUT_CNT_MASK;
				or		r2, r2, #(0xF << EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC_TIMOUT_CNT_SHIFT);
				WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, r2, r5);
	#endif	
			
		#if defined(FIX_HW_BRN_31109)
				/* Clear OOM now that we have added reserve pages */
				and		r0, r0, ~#TA_IFLAGS_OUTOFMEM;
		#endif
				/* Store out the updated flags */
				MK_STORE_STATE(ui32ITAFlags, r0);
				MK_WAIT_FOR_STATE_STORE(drc0);
	
				/* Restart the TA. */
		#if defined(SGX_FEATURE_MP)
				ldr		r5, #EUR_CR_DPM_ABORT_STATUS_MTILE >> 2, drc0;
				wdf		drc0;
				/* and with its mask */
	   			and		r5, r5, #EUR_CR_MASTER_DPM_ABORT_STATUS_MTILE_CORE_MASK;
	    			/* shift to base address position */
			        shr		r5, r5, #EUR_CR_MASTER_DPM_ABORT_STATUS_MTILE_CORE_SHIFT;
				/* issue abort to correct core */
				WRITECORECONFIG(r5, #EUR_CR_DPM_OUTOFMEM >> 2, #EUR_CR_DPM_OUTOFMEM_RESTART_MASK, r1);
				/* Also issue abort to master */
		#endif /* if defined (SGX_FEATURE_MP) */
				str		#EUR_CR_DPM_OUTOFMEM >> 2, #EUR_CR_DPM_OUTOFMEM_RESTART_MASK;
	
				/* Exit without an abort. */
				ba		TAOOM_Exit;
			}
			TAOOM_NotDeadlock:
			/* We have freed some memory so no need to add memory */
			and		r0, r0, ~#(TA_IFLAGS_SPM_DEADLOCK | TA_IFLAGS_SPM_DEADLOCK_GLOBAL);
			/* Hopefully, the scene should render now */
		}
		TAOOM_ReserveAdded:
		/* Hoepfully, the scene should render now */
		MK_TRACE(MKTC_SPM_RESERVE_ADDED, MKTF_SPM | MKTF_SCH);
	}
	TAOOM_NotDPMDeadlock:
#endif

#if defined(FIX_HW_BRN_23055)
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32MTETEDeadlockTicks)], #0;

	#if defined(FIX_HW_BRN_28026)
	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTACCBCmd)], drc0;
	
	/* Stall for a bit to make sure we detect a TE abort */
	mov		r2, #10000;
	TAOOM_BRN23055Pause:
	{
		isub16.testnz	r2, p0, r2, #1;
		p0	ba		TAOOM_BRN23055Pause;
	}
	
	wdf		drc0;
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
	mov		r3, #SGXMKIF_TAFLAGS_CMPLX_GEOMETRY_PRESENT;
	#else
	/* Stall for a bit to make sure we detect a TE abort */
	mov		r1, #10000;
	TAOOM_BRN23055Pause:
	{
		isub16.testnz	r1, p0, r1, #1;
		p0	ba		TAOOM_BRN23055Pause;
	}
	#endif
	/* Check which requestors have been denied */
	ldr		r1, #EUR_CR_DPM_REQUESTING >> 2, drc0;
	wdf		drc0;
	#if defined(FIX_HW_BRN_28026)
	/* If cmplx geom and MTE only treat as MTETE */
	and.testnz	p1, r2, r3;
	p1	and.testz	p1, r1, #EUR_CR_DPM_REQUESTING_SOURCE_TE;
	p1	mov		r1, #(EUR_CR_DPM_REQUESTING_SOURCE_MTE | EUR_CR_DPM_REQUESTING_SOURCE_TE);
	#endif
	and.testz	p0, r1, #EUR_CR_DPM_REQUESTING_SOURCE_TE;
	p0	ba		TAOOM_MTEDenied;
	{
		/* check if both the MTE and TE have been denied */
		and		r1, r1, #(EUR_CR_DPM_REQUESTING_SOURCE_MTE | EUR_CR_DPM_REQUESTING_SOURCE_TE);
		xor.testz	p1, r1, #(EUR_CR_DPM_REQUESTING_SOURCE_MTE | EUR_CR_DPM_REQUESTING_SOURCE_TE);

		p1	ba	TAOOM_MTEAndTEDenied;
		{
	#if defined(DEBUG)
			MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32TEOutOfMem)], drc0;
			wdf				drc0;
			mov				r2, #1;
			iaddu32			r1, r2.low, r1;
			MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32TEOutOfMem)], r1;
	#endif /* defined(DEBUG) */

			/* This is a TE OOM therfore check if we need to find the MTE page */
			shl.testns	p0, r0, #(31 - TA_IFLAGS_BOTH_REQ_DENIED_BIT);
			p0	ba	TAOOM_NoMTEPageCheck;
			{
				/* We may need to find out what rgn the last MTE page was allocated */
				MK_MEM_ACCESS_BPCACHE(ldaw)	r1, [R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16LastMTEPageRgn)], drc0;
				wdf		drc0;
				mov		r2, #0xFFFF;
				xor.testnz	p0, r1, r2;
				p0	ba	TAOOM_MTEPageAlreadylocated;
				{
	#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
					/* Set DPM 3D/TA indicator bit appropriately */
					ldr 	r1, #EUR_CR_BIF_BANK_SET >> 2, drc0;
					wdf		drc0;
					and 	r1, r1, ~#USE_DPM_3D_TA_INDICATOR_MASK;	// clear bit 9 of the register
					str		#EUR_CR_BIF_BANK_SET >> 2, r1;
	#endif
					/* Store out the current state and check the tails to find the page */
					bal		FindMTEPageInStateTable;
				}
				TAOOM_MTEPageAlreadylocated:
			}
			TAOOM_NoMTEPageCheck:
		}
		TAOOM_MTEAndTEDenied:

		!p1	ba	TAOOM_NotMTEAndTE;
		{
			MK_TRACE(MKTC_MTETE_OOM, MKTF_SPM | MKTF_SCH);
	#if defined(DEBUG)
			MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32MTETEOutOfMem)], drc0;
			wdf				drc0;
			mov				r1, #1;
			iaddu32			r2, r1.low, r2;
			MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32MTETEOutOfMem)], r2;
	#endif
			ldr		r2, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_STATUS1 >> 2, drc0;
			wdf		drc0;
			and		r2, r2, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_STATUS1_HEAD_MASK;
			MK_MEM_ACCESS_BPCACHE(staw)	[R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16LastMTEPageAllocation)], r2;
			MK_TRACE_REG(r2, MKTF_SPM | MKTF_SCH);
			mov		r1, #0xFFFF;
			MK_MEM_ACCESS_BPCACHE(staw)	[R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16LastMTEPageRgn)], r1;
			/*
				If this is the first time we have seen a MTE and TE outofmem do the following:
					1) store out the state table (reference)
			*/
			/* extract the bit from the ITAFlags */
			shl.testns	p0, r0, #(31 - TA_IFLAGS_BOTH_REQ_DENIED_BIT);
			/* Set flag to indicate this has happened if it is not already set */
			p0	or		r0, r0, #TA_IFLAGS_BOTH_REQ_DENIED;
			p0	MK_STORE_STATE(ui32ITAFlags, r0);
			idf				drc0, st;
		}
		TAOOM_NotMTEAndTE:

		MK_TRACE(MKTC_INCREASEZLSTHRESHOLD, MKTF_SPM | MKTF_SCH);

		/*
			Check if this is the first time we'd have to increase the ZLS threshold.
		*/
		MK_LOAD_STATE(r5, ui32ITAFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		shl.tests	p0, r5, #(31 - TA_IFLAGS_INCREASEDTHRESHOLDS_BIT);
		p0	ba				TAOOM_NotFirstIncrease;
		{
			/*
				Set the flag to reset the ZLS threshold back to normal on TA finish.
			*/
			or				r5, r5, #TA_IFLAGS_INCREASEDTHRESHOLDS;
			MK_STORE_STATE(ui32ITAFlags, r5);
		}
		TAOOM_NotFirstIncrease:
		/*
			Save the preadjusted value.
		*/
		MK_MEM_ACCESS_BPCACHE(ldaw)	r0, [R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16Num23055PagesUsed)], drc0;
		wdf				drc0;
		iaddu32			r0, r0.low, #1;
		MK_MEM_ACCESS_BPCACHE(staw)	[R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16Num23055PagesUsed)], r0;

		/* Pause allocations before we start messing with the free list. */
		PVRSRV_SGXUTILS_CALL(DPMPauseAllocations);

		/* Invalidate the OTPM to force an MTE out of memory as soon as possible. */
		str		#EUR_CR_MTE_OTPM_OP >> 2, #EUR_CR_MTE_OTPM_OP_CSM_INV_MASK;
		mov		r4, #EUR_CR_EVENT_STATUS_OTPM_INV_MASK;
		ENTER_UNMATCHABLE_BLOCK;
		TAOOM_WaitForCSMInvalidate:
		{
			ldr		r2, #EUR_CR_EVENT_STATUS >> 2, drc0;
			wdf		drc0;
			and.testz	p0, r2, r4;
			p0	ba		TAOOM_WaitForCSMInvalidate;
		}
		LEAVE_UNMATCHABLE_BLOCK;

		/* Clear the invalidate event. */
		mov		r4, #EUR_CR_EVENT_HOST_CLEAR_OTPM_INV_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r4;

		/* Increment ZLS threshold. */
		ldr		r0, #EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, drc0;
		wdf		drc0;
		iaddu32	r0, r0.low, #1;
		str		#EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, r0;

		/* Set the complete on terminate bit so we can render if we get a TA finished */
		ldr		r1, #EUR_CR_TE_PSG >> 2, drc0;
		wdf		drc0;
		or		r1, r1, #(EUR_CR_TE_PSG_COMPLETEONTERMINATE_MASK | \
						 EUR_CR_TE_PSG_ZSTOREENABLE_MASK);
		str		#EUR_CR_TE_PSG >> 2, r1;

	#if defined(DEBUG)
		/* keep a track of the highest number of pages allocated */
		MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTACCBCmd)], drc0;
		wdf		drc0;
		MK_MEM_ACCESS_CACHED(ldad)	r1, [r1, #DOFFSET(SGXMKIF_CMDTA.sHWPBDescDevVAddr)], drc0;
		wdf		drc0;
		MK_MEM_ACCESS(ldad)	r1, [r1, #DOFFSET(SGXMKIF_HWPBDESC.ui32NumPages)], drc0;
		wdf		drc0;
		#if 1 // SPM_PAGE_INJECTION
		/* if we have not used the deadlock reserve subtract it from the check */
		shl.testns	p0, r5, #(31 - TA_IFLAGS_SPM_DEADLOCK_MEM_BIT);
		p0	isub16		r1, r1, #SPM_DL_RESERVE_PAGES;
		#endif
		/* if (ZLSThreshold > r1) pagefault */
		isub16.tests	p0, r1, r0;
		!p0	ba	NoPageFault;
		{
			MK_ASSERT_FAIL(MKTC_NO_PAGES_LEFT_FOR_23055);
		}
		NoPageFault:
	#endif
		/* Reload the free list so the hardware uses the new threshold. */
		PVRSRV_SGXUTILS_CALL(DPMReloadFreeList);

		/* Don't restart VDM as we have increased the threshold */

		/* Restart the TA. */
		str		#EUR_CR_DPM_OUTOFMEM >> 2, #EUR_CR_DPM_OUTOFMEM_RESTART_MASK;

		/* Exit without an abort. */
		ba		TAOOM_Exit;
	}
	TAOOM_MTEDenied:
	#if defined(DEBUG)
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32MTEOutOfMem)], drc0;
	wdf				drc0;
	mov				r1, #1;
	iaddu32			r2, r1.low, r2;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32MTEOutOfMem)], r2;
	#endif
#endif /* defined(FIX_HW_BRN_23055) */

	/* Invalidate the OTPM, so MTE does not try and use a page that will be freed. */
#if defined(SGX_FEATURE_MP)
	ldr		r1, #EUR_CR_DPM_ABORT_STATUS_MTILE >> 2, drc0;
	wdf		drc0;
	/* and with its mask */
	and		r1, r1, #EUR_CR_MASTER_DPM_ABORT_STATUS_MTILE_CORE_MASK;
	/* shift to base address position */
	shr		r1, r1, #EUR_CR_MASTER_DPM_ABORT_STATUS_MTILE_CORE_SHIFT;
	WRITECORECONFIG(r1, #EUR_CR_MTE_OTPM_OP >> 2, #EUR_CR_MTE_OTPM_OP_CSM_INV_MASK, r4);
#else
	str		#EUR_CR_MTE_OTPM_OP >> 2, #EUR_CR_MTE_OTPM_OP_CSM_INV_MASK;
#endif /* if defined (SGX_FEATURE_MP) */

	mov		r4, #EUR_CR_EVENT_STATUS_OTPM_INV_MASK;
	ENTER_UNMATCHABLE_BLOCK;
#if defined(SGX_FEATURE_MP)
	ldr		r3, #EUR_CR_DPM_ABORT_STATUS_MTILE >> 2, drc0;
	wdf		drc0;
	/* and with its mask */
   	and		r3, r3, #EUR_CR_MASTER_DPM_ABORT_STATUS_MTILE_CORE_MASK;
	/* shift to base address position */
	shr		r3, r3, #EUR_CR_MASTER_DPM_ABORT_STATUS_MTILE_CORE_SHIFT;
#endif
	TAOOM_WaitForAbortCSMInvalidate:
	{
#if defined(SGX_FEATURE_MP)
		READCORECONFIG(r2, r3, #EUR_CR_EVENT_STATUS >> 2, drc0);
#else
		ldr		r2, #EUR_CR_EVENT_STATUS >> 2, drc0;
#endif /* if defined (SGX_FEATURE_MP) */
		wdf		drc0;
		and.testz	p0, r2, r4;
		p0	ba		TAOOM_WaitForAbortCSMInvalidate;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the invalidate event. */
	mov		r4, #EUR_CR_EVENT_HOST_CLEAR_OTPM_INV_MASK;
	str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r4;

	/* Set the abort in progress flag */
	or				r0, r0, #TA_IFLAGS_ABORTINPROGRESS;

#if !defined(FIX_HW_BRN_23055)
	/* Check if this was an allocation to the global list and issue the correct type of abort */
	ldr		r1, #EUR_CR_DPM_ABORT_STATUS_MTILE >> 2, drc0;
	wdf		drc0;

	#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	and		r3, r1, #EUR_CR_DPM_ABORT_STATUS_MTILE_RENDERTARGETID_MASK;
	shr		r3, r3, #EUR_CR_DPM_ABORT_STATUS_MTILE_RENDERTARGETID_SHIFT;
	MK_MEM_ACCESS_BPCACHE(stad) [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32AbortedRT)], r3;
	#endif
	
	#if defined(FIX_HW_BRN_29424) || defined(FIX_HW_BRN_31109)
	/* If this is a GBL_OUTOFMEM override the abort mask, and clear the flag */
	shl.tests	p0, r0, #(31 - TA_IFLAGS_OUTOFMEM_GLOBAL_BIT);
	p0	and	r1, r1, ~#EUR_CR_DPM_ABORT_STATUS_MTILE_INDEX_MASK;
	p0	or	r1, r1, #EUR_CR_DPM_ABORT_STATUS_MTILE_GLOBAL_MASK;
	p0	and	r0, r0, ~#TA_IFLAGS_OUTOFMEM_GLOBAL;
	#endif
	
	MK_MEM_ACCESS_BPCACHE(stad) [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32AbortedMacroTile)], r1;
	idf		drc0, st;
	wdf		drc0;
#endif /* !defined(FIX_HW_BRN_23055) */
	{
		/* Flag this was an abort all so we know what to do when rendering */
		or		r0, r0, #TA_IFLAGS_ABORTALL;

		mov		r2, #EUR_CR_DPM_OUTOFMEM_ABORTALL_MASK;
#if defined(SGX_FEATURE_MP)
		mov		r3, #EUR_CR_MASTER_DPM_OUTOFMEM_ABORTALL_MASK;
		mov		r4, #EUR_CR_DPM_ABORT_STATUS_MTILE_GLOBAL_MASK;
#endif /* if defined (SGX_FEATURE_MP) */

#if !defined(FIX_HW_BRN_23055)
		/* Save off the number of global pages allocated, for deadlock checking later */
		ldr		r5, #EUR_CR_DPM_GLOBAL_PAGE_STATUS >> 2, drc0;
		wdf		drc0;
		and		r5, r5, #EUR_CR_DPM_GLOBAL_PAGE_STATUS_TA_MASK;
		shr		r5, r5, #EUR_CR_DPM_GLOBAL_PAGE_STATUS_TA_SHIFT;
		isub16.tests	r5, p0, r5, #2;
		!p0	and		r5, r5, #0xFFFF;
		p0	mov		r5, #0;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32GblAbortPages)], r5;	
#endif
		MK_TRACE(MKTC_CSABORTALL, MKTF_SPM | MKTF_SCH);
	}
	TAOOM_NoAbortAll:
	
#if defined(SGX_FEATURE_MP)
	/* and with its mask */
	and		r1, r1, #EUR_CR_MASTER_DPM_ABORT_STATUS_MTILE_CORE_MASK;
	/* shift to base address position */
	shr		r1, r1, #EUR_CR_MASTER_DPM_ABORT_STATUS_MTILE_CORE_SHIFT;

	/* copy the aborted macrotile from hydra to core */
	WRITECORECONFIG(r1, #EUR_CR_DPM_ABORT_STATUS_MTILE >> 2, r4, r5);

	/* now abort the core */
	WRITECORECONFIG(r1, #EUR_CR_DPM_OUTOFMEM >> 2, r2, r5);
	
	#if defined(EUR_CR_MTE_ABORT)
	ENTER_UNMATCHABLE_BLOCK;
	TAOOM_WaitForMTEToSettle:
	{
		READCORECONFIG(r2, r1, #EUR_CR_MTE_ABORT >> 2, drc0);
		wdf		drc0;
		and.testz	p0, r2, #EUR_CR_MTE_ABORT_SETTLED_MASK;
		p0	ba	TAOOM_WaitForMTEToSettle;
	}
	LEAVE_UNMATCHABLE_BLOCK;
	
		#if defined(FIX_HW_BRN_29461)
	/* check if there are pages to be reissued */	
	bal	CheckMTEPageReIssue;
		#else /* FIX_HW_BRN_29461 */
	/* Copy the cores MTE pages to the master */
	READCORECONFIG(r2, r1, #EUR_CR_MTE_FIRST_PAGE >> 2, drc0);
	READCORECONFIG(r4, r1, #EUR_CR_MTE_SECOND_PAGE >> 2, drc0);
	wdf		drc0;
	WRITEMASTERCONFIG(#EUR_CR_MASTER_MTE_FIRST_PAGE >> 2, r2, r5);
	WRITEMASTERCONFIG(#EUR_CR_MASTER_MTE_SECOND_PAGE >> 2, r4, r5);
		#endif /* FIX_HW_BRN_29461 */
	#endif /* EUR_CR_MTE_ABORT */
		
	/* write into hydra bank as well */
	WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_OUTOFMEM >> 2, r3, r5);
#else
	str		#EUR_CR_DPM_OUTOFMEM >> 2, r2;
#endif /* if defined (SGX_FEATURE_MP) */

#if !defined(FIX_HW_BRN_23055)
	/* Save off the number of pages allocated, for deadlock checking later */
	ldr		r3, #EUR_CR_DPM_PAGE_STATUS >> 2, drc0;
	wdf		drc0;
	and		r3, r3, #EUR_CR_DPM_PAGE_STATUS_TA_MASK;
	shr		r3, r3, #EUR_CR_DPM_PAGE_STATUS_TA_SHIFT;
	isub16	r3, r3, #2;
	and		r3, r3, #0xFFFF;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32AbortPages)], r3;
#endif

	/*
		We have done an abort of some kind therefore,
		Disable the vertex data master to stop a USSE deadlock.
	*/
	ldr				r1, #EUR_CR_DMS_CTRL >> 2, drc0;
	wdf				drc0;
	or				r1, r1, #EUR_CR_DMS_CTRL_DISABLE_DM_VERTEX_MASK;
	str				#EUR_CR_DMS_CTRL >> 2, r1;

#if defined(FIX_HW_BRN_31109)
	/* we are about to handle the event so can clear the flag */
	and	r0, r0, ~#TA_IFLAGS_TIMER_DETECTED_SPM;
#endif

	MK_STORE_STATE(ui32ITAFlags, r0);
	/* wait for stores to complete */
	idf		drc1, st;
	wdf		drc1;
TAOOM_Exit:
	/* branch back to calling code */
	mov		pclink, R_EDM_PCLink;

	lapc;
}

#if defined(FIX_HW_BRN_29461) && defined(SGX_FEATURE_MP)
/*****************************************************************************
 CheckMTEPageReIssue routine	- starts a SPM render now the TA
										has been aborted

 inputs:	r0 - ui32ITAFlags
 			r1 - Core to be aborted.
 			R_RTData - SGXMKIF_HWRTDATA of the render target being aborted
 temps:	r2, r4, r5, r6, r7, r8, r9
*****************************************************************************/
CheckMTEPageReIssue:
{
	MK_TRACE(MKTC_CHECK_MTE_PAGE_REISSUE, MKTF_SCH | MKTF_SPM);
	
	/* read first and second page */
	READCORECONFIG(r2, r1, #EUR_CR_MTE_FIRST_PAGE >> 2, drc0);
	READCORECONFIG(r4, r1, #EUR_CR_MTE_SECOND_PAGE >> 2, drc0);
	wdf		drc0;
	
	#if defined(FIX_HW_BRN_31109)
	/* If the flag is set, just copy the registers as the core is idle. */
	shl.tests	p0, r0, #(31 - TA_IFLAGS_TIMER_DETECTED_SPM_BIT);
	p0	ba	CPRI_CopyRegisters;
	#endif
	{
		/* Find out what the requesting source is */
		READMASTERCONFIG(r5, #EUR_CR_MASTER_DPM_REQUESTING >> 2, drc0);
		wdf		drc0;
	
		MK_TRACE_REG(r5, MKTF_SPM);
		
		/* if it is an out_of_mt and requesting_source == TE, just copy the values */
		shl.testns	p0, r0, #(31 - TA_IFLAGS_OUTOFMEM_GLOBAL_BIT);
		p0	xor.testz	p0, r5, #EUR_CR_DPM_REQUESTING_SOURCE_TE;
		p0	ba	CPRI_CopyRegisters;
		{
			/* 
				If we have get to here it is either:
					1) out_of_mt && requesting_source_mte present
					2) out_of_gbl
			*/
			MK_TRACE_REG(r2, MKTF_SPM);
			MK_TRACE_REG(r4, MKTF_SPM);
		
			/* check for valid bits in either register */
			or		r5, r2, r4;
			shl.testns	p0, r5, #(31 - EUR_CR_MTE_FIRST_PAGE_VALID_SHIFT);
			/* if neither entry is valid we can just write the values */
			p0	ba	CPRI_CopyRegisters;
			{
				MK_TRACE(MKTC_CPRI_VALID_ENTRIES, MKTF_SPM);
				
				/* 
					r1 - aborted_core_idx
					r2 - first_page
					r4 - second_page
				*/
				/* extract the MT from the first_page register */
				and		r5, r2, #EUR_CR_MTE_FIRST_PAGE_MACROTILE_MASK;
				shr		r5, r5, #EUR_CR_MTE_FIRST_PAGE_MACROTILE_SHIFT;
					
				/* if we are doing an abortall, still check PIM values for the MT in register  */
				and.testnz	p0, r0, #TA_IFLAGS_ABORTALL;
				p0	ba	CPRI_AbortAll;
				{
					/* extract MT idx from dpm_abort_status_mtile */
					ldr		r6, #EUR_CR_DPM_ABORT_STATUS_MTILE >> 2, drc0;
					wdf		drc0;
					and		r6, r6, #(EUR_CR_DPM_ABORT_STATUS_MTILE_GLOBAL_MASK | \
										EUR_CR_DPM_ABORT_STATUS_MTILE_INDEX_MASK);
					
					MK_TRACE(MKTC_CPRI_ABORT_MT_IDX, MKTF_SCH);
					MK_TRACE_REG(r6, MKTF_SPM);
					
					/* if (first_page_mt != abort_mt_idx) {copy values} */
					xor.testnz	p0, r5, r6;
					p0	ba	CPRI_CopyRegisters;
				}
				CPRI_AbortAll:
				{
					MK_TRACE(MKTC_CPRI_STORE_OTPM_CSM, MKTF_SPM);
					
					/* Flush the CSM to memory
						NOTE: It is OK to flush after invalidate as the PIM remains intact */
					mov		r6, #OFFSET(SGXMKIF_HWRTDATA.sContextOTPMDevAddr[0]);
					imae	r6, r1.low, #4, r6, u32;
					iaddu32	r6, r6.low, R_RTData;
					MK_MEM_ACCESS(ldad)	r6, [r6], drc1;
					wdf		drc1;
					WRITECORECONFIG(r1, #EUR_CR_MTE_OTPM_CSM_FLUSH_BASE >> 2, r6, r7);
					WRITECORECONFIG(r1, #EUR_CR_MTE_OTPM_OP >> 2, #EUR_CR_MTE_OTPM_OP_CSM_FLUSH_MASK, r7);
					
					ENTER_UNMATCHABLE_BLOCK;
					CPRI_WaitForCSMFlush:
					{
						READCORECONFIG(r7, r1, #EUR_CR_EVENT_STATUS >> 2, drc1);
						wdf		drc1;
						shl.testns	p0, r7, #(31 - EUR_CR_EVENT_STATUS_OTPM_FLUSHED_SHIFT);
						p0	ba		CPRI_WaitForCSMFlush;
					}
					LEAVE_UNMATCHABLE_BLOCK;
					
					mov		r7, #EUR_CR_EVENT_HOST_CLEAR_OTPM_FLUSHED_MASK;
					WRITECORECONFIG(r1, #EUR_CR_EVENT_HOST_CLEAR >> 2, r7, r8);
							
					/* dump the dplist (parti pim table) */
					MK_MEM_ACCESS(ldad)	r7, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sPIMTableDevAddr)], drc0; 
					wdf		drc0;
					/* Skip the dplist_store if it has already been done */
					shl.tests	p0, r0, #(31 - TA_IFLAGS_DPLIST_STORED_BIT);
					p0	ba	CPRI_NoDPListStore;
					{
						MK_TRACE(MKTC_CPRI_STORE_DPLIST, MKTF_SPM);
						
						WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_MTILE_PARTI_PIM_TABLE_BASE_ADDR >> 2, r7, r8);
						WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_TASK_DPLIST >> 2, #EUR_CR_MASTER_DPM_TASK_DPLIST_STORE_MASK, r8);
						
						/* Poll for the DPM PIM stored event. */
						ENTER_UNMATCHABLE_BLOCK;
						CPRI_WaitForDPMPIMStore:
						{
							READMASTERCONFIG(r8, #EUR_CR_MASTER_DPM_DPLIST_EVENT_STATUS >> 2, drc0);
							wdf			drc0;
							and.testz	p0, r8, #EUR_CR_MASTER_DPM_DPLIST_EVENT_STATUS_STORED_MASK;
							p0	ba		CPRI_WaitForDPMPIMStore;
						}
						LEAVE_UNMATCHABLE_BLOCK;		
						
						/* Clear the event bit */
						WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_DPLIST_CLEAR_EVENT >> 2, #EUR_CR_MASTER_DPM_DPLIST_CLEAR_EVENT_STORE_MASK, r8);
					}
					CPRI_NoDPListStore:
					
					MK_TRACE(MKTC_CPRI_ABORT_CORE_IDX, MKTF_SPM);
					MK_TRACE_REG(r1, MKTF_SPM);
					
					/* 
						r1 - aborted_core_idx
						r2 - first_page
						r4 - second_page
						r5 - aborted_mt_idx
						r6 - optm_csm_table_dev_addr
						r7 - dpm_dplist_table_dev_addr
					*/
					
					/* use aborted_mt_idx to offset into the dpm_dplist_table to get dpm_partial_pim */					
					imae	r9, r5.low, #4, r7, u32;
										
					/* use aborted_core_idx to offset into the dpm_dplist_table to get dpm_working_pim */
					iaddu32	r8, r1.low, #32;
					imae	r8, r8.low, #4, r7, u32;
					
					SWITCHEDMTOTA();
					
					/* Read the dpm_working_pim from the dpm_dplist_table */
					MK_MEM_ACCESS_BPCACHE(ldad)	r8, [r8], drc0;
					/* Read the dpm_partial_pim from the dpm_dplist_table */
					MK_MEM_ACCESS_BPCACHE(ldad)	r9, [r9], drc0;
							
					/* 
						r8 - dpm_working_pim
						r9 - dpm_partial_pim
					*/
					
					/* use the aborted_mt_idx to offset into the otpm_csm_table to get the mte_working_pim */
					imae	r6, r5.low, #8, r6, u32;
					/* Read the working pim value for the core from the CSM table */
					MK_MEM_ACCESS_BPCACHE(ldad.f2)	r6, [r6, #0++], drc0;
					/* wait for all data to return from memory */
					wdf		drc0;
					
					MK_TRACE(MKTC_CPRI_CSM_TABLE_DATA, MKTF_SPM);
					MK_TRACE_REG(r6, MKTF_SPM);
					MK_TRACE_REG(r7, MKTF_SPM);
					
					/* extract the working PIM from the OPTM_CSM entry */
					and		r6, r6, #0xFC000000;
					shr		r6, r6, #26;
					and		r7, r7, #0x3FFFFF;
					shl		r7, r7, #6;
					or		r7, r6, r7;
					
					/* 
						r1 - aborted_core_idx
						r2 - first_page
						r4 - second_page
						r5 - aborted_mt_idx
						r7 - mte_working_pim
						r8 - dpm_working_pim
						r9 - dpm_partial_pim
					*/
					
					MK_TRACE(MKTC_CPRI_PIM_DATA, MKTF_SPM);
					MK_TRACE_REG(r7, MKTF_SPM);
					MK_TRACE_REG(r8, MKTF_SPM);
					MK_TRACE_REG(r9, MKTF_SPM);
			
					SWITCHEDMBACK();
					
					/* extract the PIM values */
					and	r8, r8, #(0x0FFFFFFF);
					
					/* if (mte_working_pim == dpm_working_pim) {copy values} */
					xor.testz	p0, r7, r8;
					p0	ba	CPRI_CopyRegisters;
					{
						/* if (mte_working_pim == dpm_partial_pim) {copy values}  */
						and		r10, r9, #0x0FFFFFFF;
						xor.testz	p0, r7, r9;
						p0	ba	CPRI_WriteZero;
						{
							/* if (dpm_partial_pim_valid == 0) {copy values} */
							and.testns	p0, r9, r9;
							p0	ba	CPRI_CopyRegisters;
							{
								/* do the circular test */
								MK_TRACE(MKTC_CPRI_DO_CIRCULAR_TEST, MKTF_SPM);
								/* 	
									a = dpm_partial_pim
									b = mte_working_pim
								*/
								isub32	r10, r10, r7;
								
								READMASTERCONFIG(r5, #EUR_CR_MASTER_VDM_PIM_MAX >> 2, drc0);
								wdf		drc0;
								REG_INCREMENT(r5, p0);
								shr		r5, r5, #1;
								/* if (((VDM_MAX_PIM)/2) - (a-b)) < 0 */
								isubu32.tests	r10, p0, r5, r10;
								MK_TRACE_REG(r10, MKTF_SPM);
								p0	ba	CPRI_CopyRegisters;
							}
						}
					}
				}
			}
		}
		CPRI_WriteZero:
		mov		r2, #0;
		mov		r4, #0;
	}
	CPRI_CopyRegisters:
	
	MK_TRACE(MKTC_CPRI_WRITE_ENTRIES, MKTF_SPM);
	MK_TRACE_REG(r2, MKTF_SPM);
	MK_TRACE_REG(r4, MKTF_SPM);
	
	/* Ensure the flag is cleared */
	and		r0, r0, ~#(TA_IFLAGS_STORE_DPLIST | TA_IFLAGS_DPLIST_STORED);
	
	/* Write the values in to the master bank */
	WRITEMASTERCONFIG(#EUR_CR_MASTER_MTE_FIRST_PAGE >> 2, r2, r5);
	WRITEMASTERCONFIG(#EUR_CR_MASTER_MTE_SECOND_PAGE >> 2, r4, r5);
	
	lapc;
}
#endif

/*****************************************************************************
 SPMAbortComplete routine	- starts a SPM render now the TA
			has been aborted

 inputs:
 temps:
*****************************************************************************/
SPMAbortComplete:
{
	/* Indicate that we have received an Abort complete event */
	MK_LOAD_STATE(r0, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

#if defined(FIX_HW_BRN_31562) && defined(SGX_FEATURE_MP)
	shl.testns	p0, r0, #(31 - TA_IFLAGS_ABORT_COMPLETE_BIT);
	!p0	shl.tests	p0, r0, #(31 - TA_IFLAGS_WAITINGTOSTARTCSRENDER_BIT);
	p0	ba	SPMAC_HandleTerminate;
	{
		MK_TRACE(MKTC_SPMAC_IGNORE_TERMINATE, MKTF_SPM | MKTF_SCH);

		/* This has been generated by the workaround so ignore it */
		ba	SPMAC_Exit;
	}
	SPMAC_HandleTerminate:
#endif

	or	r0, r0, #TA_IFLAGS_ABORT_COMPLETE;	
	MK_STORE_STATE(ui32ITAFlags, r0);
	MK_WAIT_FOR_STATE_STORE(drc1);
	
	/* Check for renders in progress or queued */
	MK_LOAD_STATE(r0, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testz		p0, r0, #RENDER_IFLAGS_RENDERINPROGRESS;
	p0	ba		SPMAC_NoRenderInProgress;
	{
		/*
			Flag that we are waiting to start a partial render.
		*/
		MK_LOAD_STATE(r0, ui32ITAFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		or				r0, r0, #TA_IFLAGS_WAITINGTOSTARTCSRENDER;
		MK_STORE_STATE(ui32ITAFlags, r0);
		MK_WAIT_FOR_STATE_STORE(drc0);

		/*
			Clear the restart on render finish flag.
		*/
		MK_LOAD_STATE(r0, ui32IRenderFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and				r0, r0, ~#RENDER_IFLAGS_TAWAITINGFORMEM;
		MK_STORE_STATE(ui32IRenderFlags, r0);
		MK_WAIT_FOR_STATE_STORE(drc0);

		MK_TRACE(MKTC_TATERMRENDERINPROGRESS, MKTF_SPM | MKTF_SCH);

		ba		SPMAC_Exit;
	}
	SPMAC_NoRenderInProgress:

	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderDetails)], drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	R_RTData, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], drc1;
	wdf		drc0;
	wdf		drc1;

	MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWRTDataSetDevAddr)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS(ldad)	r2, [r1, #DOFFSET(SGXMKIF_HWRTDATASET.ui32NumSPMRenders)], drc0;
	wdf		drc0;
	mov			r3, #1;
	iaddu32		r2, r3.low, r2;
	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRTDATASET.ui32NumSPMRenders)], r2;
	idf		drc0, st;
	wdf		drc0;

#if defined(DEBUG) || defined(PDUMP) || defined(SUPPORT_SGX_HWPERF)
	MK_MEM_ACCESS(ldad)	r3, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32FrameNum)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui323DFrameNum)], r3;
	MK_TRACE(MKTC_SPM3D_FRAMENUM, MKTF_3D | MKTF_FRMNUM);
	MK_TRACE_REG(r3, MKTF_3D | MKTF_FRMNUM);
#endif

#if defined(FIX_HW_BRN_31562) && defined(SGX_FEATURE_MP)
	/* If this is a state-free render the TA has already finished on all cores. */
	MK_LOAD_STATE(r1, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	shl.tests	p0, r1, #(31 - TA_IFLAGS_SPM_STATE_FREE_ABORT_BIT);
	p0	ba	SPMAC_FlushComplete;
	{
		SPMAC_WaitForTAWrites:
		{
			/* Wait for the TE_CHECKSUM, MTE_TE_CHECKSUM and MTE_MEM_CHECKSUM */
			ldr			r1, #EUR_CR_MTE_MEM_CHECKSUM >> 2, drc0;
			ldr			r2, #EUR_CR_MTE_TE_CHECKSUM >> 2, drc0;
			ldr			r3, #EUR_CR_TE_CHECKSUM >> 2, drc0;
			wdf			drc0;
			xor			r4, r1, r2;
			xor			r4, r4, r3;
			mov			r2, #2500;
			SPMAC_DelayBeforeSample:
			{
				isub16.testns	r2, p0, r2, #1;
				p0 ba		SPMAC_DelayBeforeSample;
			}
			/* capture the registers again */
			ldr			r1, #EUR_CR_MTE_MEM_CHECKSUM >> 2, drc0;
			ldr			r2, #EUR_CR_MTE_TE_CHECKSUM >> 2, drc0;
			ldr			r3, #EUR_CR_TE_CHECKSUM >> 2, drc0;
			wdf			drc0;
			xor			r1, r1, r2;
			xor			r1, r1, r3;
		
			/* Are the checksums stable? */
			xor.testnz	p0, r1, r4;
			p0	ba		SPMAC_WaitForTAWrites;
		}
		/* Now that the TAs have finished writing, cause the data to be flushed by the burster */
		MK_MEM_ACCESS_BPCACHE(ldad) r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32AbortedMacroTile)], drc0;
		wdf		drc0;
		shr		r1, r1, #EUR_CR_MASTER_DPM_ABORT_STATUS_MTILE_CORE_SHIFT;
		mov		r2, #EUR_CR_EVENT_STATUS_TA_TERMINATE_MASK;
		WRITECORECONFIG(r1, #EUR_CR_EVENT_STATUS >> 2, r2, r3);
		/* Clear the event back down again */
		mov		r1, #EUR_CR_EVENT_HOST_CLEAR_TA_TERMINATE_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r1;
	
		/* Delay for 1000 clocks to allow the writes to complete */
		mov		r1, #250;
		SPMAC_BursterWriteDelay:
		{
			isub16.testns	r1, p0, r1, #1;
			p0 ba		SPMAC_BursterWriteDelay;
		}
	}
	SPMAC_FlushComplete:
#endif
#if defined(SGX_FEATURE_MP)
#if defined(FIX_HW_BRN_30710) || defined(FIX_HW_BRN_30764)
	SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_SOFT_RESET >> 2, #EUR_CR_MASTER_SOFT_RESET_IPF_RESET_MASK);
	SGXMK_SPRV_WRITE_REG(#EUR_CR_MASTER_SOFT_RESET >> 2, #0);
#endif /* SGX_FEATURE_MP */
#endif
#if defined(FIX_HW_BRN_31964)
	SGXMK_SPRV_WRITE_REG(#EUR_CR_SOFT_RESET >> 2, #EUR_CR_SOFT_RESET_ISP2_RESET_MASK);
	SGXMK_SPRV_WRITE_REG(#EUR_CR_SOFT_RESET >> 2, #0);
#endif

	/*
		Set the 3D context ID to the opposite of the TA context ID.
		Set the LS context ID to the same as the TA context ID.
	*/
	ldr		r1, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc0;
	wdf		drc0;
	and		r1, r1, #~(EUR_CR_DPM_STATE_CONTEXT_ID_DALLOC_MASK | EUR_CR_DPM_STATE_CONTEXT_ID_LS_MASK);
	and		r2, r1, #EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_MASK;
	shr		r2, r2, #EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_SHIFT;
	xor		r3, r2, #1;

	/* before we start messing with the other context make sure the state is maintained */
	and.testz	p0, r3, r3;
	// RTData to be stored
	p0	MK_MEM_ACCESS_BPCACHE(ldad)	r10, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext0RTData)], drc0;
	!p0	MK_MEM_ACCESS_BPCACHE(ldad)	r10, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext1RTData)], drc0;

	/* Call StoreContext subroutine */
	/* Wait for the RTdata to Load */
	wdf		drc0;
	/* if the context is not being used, dont store it out to memory */
	and.testz	p1, r10, r10;
	p1	ba	SPMAC_NoContextStore;
	{
		/* find out the RenderContext of the RTDATA */
		MK_MEM_ACCESS(ldad)	r4, [r10, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderContextDevAddr)], drc0;
		wdf		drc0;

		/* Set up the 3D for the memory context to be stored. */
		PVRSRV_SGXUTILS_SETUPDIRLISTBASE(r4, #SETUP_BIF_3D_REQ);

		/* Store the DPM state via the 3D Index */
		STORE3DCONTEXT_SPM(r10, r3);

#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
		MK_MEM_ACCESS_BPCACHE(ldad)	r4, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderContextDevAddr)], drc0;
		wdf		drc0;
		/* Restore the memory context we were using. */
		PVRSRV_SGXUTILS_SETUPDIRLISTBASE(r4, #SETUP_BIF_3D_REQ);
#endif
		and.testz	p0, r3, r3;
		p0	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext0RTData)], #0;
		!p0	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext1RTData)], #0;
		idf		drc1, st;
		/* Make sure the store has completed */
		wdf		drc1;
	}
	SPMAC_NoContextStore:

	shl		r3, r3, #EUR_CR_DPM_STATE_CONTEXT_ID_DALLOC_SHIFT;
	shl		r2, r2, #EUR_CR_DPM_STATE_CONTEXT_ID_LS_SHIFT;
	or		r1, r1, r3;
	or		r1, r1, r2;
#if defined(EUR_CR_DPM_STATE_CONTEXT_ID_NCOP_MASK)
	and		r1, r1, ~#EUR_CR_DPM_STATE_CONTEXT_ID_NCOP_MASK;
#endif
	str		#EUR_CR_DPM_STATE_CONTEXT_ID >> 2, r1;

	/* Setup the state table addr */
	MK_MEM_ACCESS(ldad)	r1, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sContextStateDevAddr)], drc0;
	wdf		drc0;
	SETUPSTATETABLEBASE(SPMAC_ST, p0, r1, r2);

#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	/* Set DPM 3D/TA indicator bit appropriately */
	ldr 	r11, #EUR_CR_BIF_BANK_SET >> 2, drc0;
	wdf		drc0;
	and 	r11, r11, ~#USE_DPM_3D_TA_INDICATOR_MASK;	// clear bit 9 of the register
	str		#EUR_CR_BIF_BANK_SET >> 2, r11;
#endif

	/* Check if we're aborting a different MT to the one the TA selected. */
	MK_LOAD_STATE(r11, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
#if defined(FIX_HW_BRN_23055)
	/* extract the bit from the ITAFlags */
	shl.tests	p0, r11, #(31 - TA_IFLAGS_BOTH_REQ_DENIED_BIT);
	!p0	ba	SPMAC_NoModsRequired;
	{
		/* We may need to find out what rgn the last MTE page was allocated */
		MK_MEM_ACCESS_BPCACHE(ldaw)	r1, [R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16LastMTEPageRgn)], drc0;
		wdf		drc0;
		mov		r2, #0xFFFF;
		xor.testz	p0, r1, r2;
		/* Store out the current state and check the tails to find the page */
		p0	bal		FindMTEPageInStateTable;
	
		/* The MTE and TE have been denied at some point so do the state merging */
		/* See if we need to remove the MTE page from the context given to the 3D */
		bal		MoveMTEPageToTAState;
		and.testnz	p1, r11, #TA_IFLAGS_ABORTALL;
		ba		SPMAC_ContextsModified;
	}
	SPMAC_NoModsRequired:
	MK_TRACE(MKTC_NO_STATE_MODS, MKTF_SPM | MKTF_SCH);
#endif /* defined(FIX_HW_BRN_23055) */

#if defined(FIX_HW_BRN_31076)
		/* Ensure the Context which will be used by the 3D has been cleared first */
		/* Flip the LS context ID to the same as the 3D context ID. */
		ldr		r2, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc0;
		wdf		drc0;
		xor		r1, r2, #EUR_CR_DPM_STATE_CONTEXT_ID_LS_MASK;
	#if defined(EUR_CR_DPM_STATE_CONTEXT_ID_NCOP_MASK)
		and		r1, r1, ~#EUR_CR_DPM_STATE_CONTEXT_ID_NCOP_MASK;
	#endif
		str		#EUR_CR_DPM_STATE_CONTEXT_ID >> 2, r1;

		/* Reset the DPM state for the 3D. */
		PVRSRV_SGXUTILS_CALL(DPMStateClear);

		/* Reset the LS context ID to the same as the TA context ID. */
		str		#EUR_CR_DPM_STATE_CONTEXT_ID >> 2, r2;
#endif

	/* check if abort all */
	and.testnz	p1, r11, #TA_IFLAGS_ABORTALL;
	p1	ba		SPMAC_AbortAll;
	{
		/* Do this only for the macrotile we're aborting. */
		str		#EUR_CR_DPM_LSS_PARTIAL_CONTEXT >> 2, #EUR_CR_DPM_LSS_PARTIAL_CONTEXT_OPERATION_MASK;
	}
	SPMAC_AbortAll:
	/*
		Store the DPM state for the TA.
	*/
	PVRSRV_SGXUTILS_CALL(DPMStateStore);

	/*
		Reset the DPM state for the TA.
	*/
	PVRSRV_SGXUTILS_CALL(DPMStateClear);

	/*
		Flip the LS context ID to the same as the 3D context ID.
	*/
	ldr		r1, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc0;
	wdf		drc0;
	xor		r1, r1, #EUR_CR_DPM_STATE_CONTEXT_ID_LS_MASK;
	str		#EUR_CR_DPM_STATE_CONTEXT_ID >> 2, r1;

#if defined(EUR_CR_DPM_STATE_TABLE_CONTEXT0_BASE)
	/* Copy the State table address to the other register */
	and.testz	r1, p0, r1, #EUR_CR_DPM_STATE_CONTEXT_ID_LS_MASK;
	p0	ba	SPMAC_CopyBaseFromContext1;
	{
		/* Load address from register and setup r1 to opposite ID */
		ldr		r2, #EUR_CR_DPM_STATE_TABLE_CONTEXT0_BASE >> 2, drc0;
		ba		SPMAC_SetupLoadBase;
	}
	SPMAC_CopyBaseFromContext1:
	{
		/* Load address from register and setup r1 to opposite ID */
		ldr		r2, #EUR_CR_DPM_STATE_TABLE_CONTEXT1_BASE >> 2, drc0;
	}
	SPMAC_SetupLoadBase:
	/* Wait for the register to load */
	wdf		drc0;
	SETUPSTATETABLEBASE(SPMAC_LD, p0, r2, r1);
#endif
	/*
		Load the DPM state for the 3D.
	*/
	PVRSRV_SGXUTILS_CALL(DPMStateLoad);

	/*
		Flip the LS context ID back to the TA context ID.
	*/
	ldr		r1, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc0;
	wdf		drc0;
	xor		r1, r1, #EUR_CR_DPM_STATE_CONTEXT_ID_LS_MASK;
	str		#EUR_CR_DPM_STATE_CONTEXT_ID >> 2, r1;

	str		#EUR_CR_DPM_LSS_PARTIAL_CONTEXT >> 2, #0;

#if defined(FIX_HW_BRN_23055)
SPMAC_ContextsModified:
#endif

	/* get the new render context */
	MK_MEM_ACCESS(ldad)	r3, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderContextDevAddr.uiAddr)], drc0;
	wdf		drc0;

#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	MK_MEM_ACCESS(ldad)	r3, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderContextDevAddr.uiAddr)], drc0;
	wdf		drc0;
	/* Ensure the memory context is setup */
	PVRSRV_SGXUTILS_SETUPDIRLISTBASE(r3, #SETUP_BIF_3D_REQ);
#endif

	/* get the current 3D HWPBDesc */
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DHWPBDesc.uiAddr)], drc0;
	/* get the new 3D HWPBDesc */
	MK_MEM_ACCESS(ldad)	r3, [r3, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPBDescDevVAddr.uiAddr)], drc0;
	/* get the current TA HWPBDesc */
	MK_MEM_ACCESS_BPCACHE(ldad)	r4, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc.uiAddr)], drc0;
	wdf		drc0;

#if defined(EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC)
	/* Disable proactive PIM spec */
	READMASTERCONFIG(r16, #EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, drc0);
	wdf		drc0;
	or		r16, r16, #EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC_EN_N_MASK;
	WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, r16, r17);
#endif

	/* if Current PB == New PB then no store or load required */
	xor.testz	p0, r2, r3;
	p0	ba	SPMAC_NoPBLoad;
	{
		/* if Current PB == NULL then no store required */
		and.testz	p0, r2, r2;
		p0	ba	SPMAC_NoPBStore;
		{
#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS) && !defined(FIX_HW_BRN_25910)
			/* if current PB == TA PB then no store required */
			xor.testz	p0, r2, r4;
			p0	ba	SPMAC_NoPBStore;
#endif /* SGX_FEATURE_MULTIPLE_MEM_CONTEXTS */
			{
				/* PB Store: No idle required as we know the PB is not in use */
				STORE3DPB();
			}
		}
		SPMAC_NoPBStore:

		/*
			If the new PB == TA PB, we have to:
				1) Idle the TA, so no allocations can take place
				2) Store the TA PB
				3) Then load the up-to-date values to 3D
				4) Resume the TA
		*/
		xor.testnz	p0, r3, r4;
		p0	ba	SPMAC_NoPBCopy;
		{
			/* Store the current details of the TA PB */
			STORETAPB();
		}
		SPMAC_NoPBCopy:

		/* PB Load: */
		LOAD3DPB(r3);
	}
	SPMAC_NoPBLoad:

	/* Check the TA flags */
	MK_LOAD_STATE(r1, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	/* Ensure DPM dealloc bits are set correctly */
	and.testnz	p0, r1, #TA_IFLAGS_ABORTALL;
	mov		r2, #EUR_CR_DPM_3D_DEALLOCATE_ENABLE_MASK;
	p0		or	r2, r2, #EUR_CR_DPM_3D_DEALLOCATE_GLOBAL_MASK;
	str		#EUR_CR_DPM_3D_DEALLOCATE >> 2, r2;

#if defined(FIX_HW_BRN_31076)
 	/* Deallocate the memory before the render that way
		any pages which are reissued are not freed inadvertently! */
	/* Temporarily disable 3D_MEM_FREE event */
	READMASTERCONFIG(r2, #EUR_CR_MASTER_EVENT_PDS_ENABLE >> 2, drc0);
	wdf		drc0;
	and		r3, r2, ~#EUR_CR_MASTER_EVENT_PDS_ENABLE_DPM_3D_MEM_FREE_MASK;
	WRITEMASTERCONFIG(#EUR_CR_MASTER_EVENT_PDS_ENABLE >> 2, r3, r4);

	MK_TRACE(MKTC_SPMAC_REQUEST_3D_TIMEOUT, MKTF_SPM);

	/* Free the memory */
	str		#EUR_CR_DPM_3D_TIMEOUT >> 2, #EUR_CR_DPM_3D_TIMEOUT_NOW_MASK;
 		
	ENTER_UNMATCHABLE_BLOCK;
	SPMAC_WaitForMemFree:
	{
		ldr		r4, #EUR_CR_EVENT_STATUS >> 2, drc0;
		wdf		drc0;
		shl.testns	p0, r4, #(31 - EUR_CR_EVENT_STATUS_DPM_3D_MEM_FREE_SHIFT);
		p0 ba	SPMAC_WaitForMemFree;
	}
	LEAVE_UNMATCHABLE_BLOCK;
 		
	/* Clear the event */
 	mov		r4, #EUR_CR_EVENT_HOST_CLEAR_DPM_3D_MEM_FREE_MASK;
 	str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r4;

	MK_TRACE(MKTC_SPMAC_3D_TIMEOUT_COMPLETE, MKTF_SPM);

	/* Re-enable 3D_MEM_FREE event */
	WRITEMASTERCONFIG(#EUR_CR_MASTER_EVENT_PDS_ENABLE >> 2, r2, r4);
#endif

#if 1 // SPM_PAGE_INJECTION
	shl.tests	p0, r1, #(31 - TA_IFLAGS_SPM_STATE_FREE_ABORT_BIT);
	p0	ba	SPMAC_NotPartialRender;
#endif
	{
		/* Indicate this is going to be a partial render */
		str		#EUR_CR_DPM_PARTIAL_RENDER >> 2, #EUR_CR_DPM_PARTIAL_RENDER_ENABLE_MASK;
	}
	SPMAC_NotPartialRender:

	/* Load 3D registers */
	mov		r1, #OFFSET(SGXMKIF_HWRENDERDETAILS.s3DRegs);
	iadd32	r2, r1.low, r0;

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

	WRITEISPRGNBASE(SPMAC_ISPRgnBase, r2, r3, r4, r5);

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
	COPYMEMTOCONFIGREGISTER_3D(SPMAC_ZLS_EXTZ, EUR_CR_ISP2_ZLS_EXTZ_RGN_BASE, r2, r3, r4, r5);
#else
	COPYMEMTOCONFIGREGISTER_3D(SPMAC_ZLS_EXTZ, EUR_CR_ISP_ZLS_EXTZ_RGN_BASE, r2, r3, r4, r5);
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
	MK_MEM_ACCESS_BPCACHE(ldad) 	R_RTAIdx, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32AbortedRT)], drc0;
	wdf		drc0;
	str		#EUR_CR_DPM_3D_RENDER_TARGET_INDEX >> 2, R_RTAIdx;
	and.testz	p0, R_RTAIdx, R_RTAIdx;
	p0	ba	SPMAC_FirstRTInArray;
	{
		/* Move the RgnBase to point to the correct RT */
		ldr		r2, #EUR_CR_ISP_RGN_BASE >> 2, drc0;
		wdf		drc0;
		MK_MEM_ACCESS(ldad)	r4, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32RgnStride)], drc0;
		wdf		drc0;
		ima32	r2, R_RTAIdx, r4, r2;
		str		#EUR_CR_ISP_RGN_BASE >> 2, r2;
		
		/* Move on the Z/S Buffer addresses */
		MK_MEM_ACCESS(ldad)	r3, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32ZBufferStride)], drc0;
		ldr		r4, #EUR_CR_ISP_ZLOAD_BASE >> 2, drc0;
		ldr		r5, #EUR_CR_ISP_ZSTORE_BASE >> 2, drc0;
		wdf		drc0;
		ima32	r4, R_RTAIdx, r3, r4;
		ima32	r5, R_RTAIdx, r3, r5;
		str	#EUR_CR_ISP_ZLOAD_BASE >> 2, r4;
		str	#EUR_CR_ISP_ZSTORE_BASE >> 2, r5;
		
		MK_MEM_ACCESS(ldad)	r3, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32SBufferStride)], drc0;
		ldr		r4, #EUR_CR_ISP_STENCIL_LOAD_BASE >> 2, drc0;
		ldr		r5, #EUR_CR_ISP_STENCIL_STORE_BASE >> 2, drc0;
		wdf		drc0;
		ima32	r4, R_RTAIdx, r3, r4;
		ima32	r5, R_RTAIdx, r3, r5;
		str	#EUR_CR_ISP_STENCIL_LOAD_BASE >> 2, r4;
		str	#EUR_CR_ISP_STENCIL_STORE_BASE >> 2, r5;
	}
	SPMAC_FirstRTInArray:
#endif

	/*
		Pseudo code:
		SPM kick partial render:
		if (ext Z buffer)
		{
			- application already has set ZLS controls
			- must merge SPM ZLS controls into the register. Here's the logic:
				- first partial render to a MT, OR Z/Stencil Store bits
				- subsequent partial render to a MT, OR Z/Stencil Load and Store bits
				- final main scene render, CLEAR Z/Stencil Store bits BUT only if SPM code set them
				- Z buffer base is 16byte aligned and setup by client
			- need the following state per render target
				- application Stencil and Z load/store bits
					- this is in the HWrenderdetails register value
				- SPM Stencil and Z load/store bits per Macrotile
					- 1bit per-MT, 0 - no previous partial renders to MT, 1 - had partial renders
					- 4 � 16(?) MTs per RT so require 4 or 16bits
						- 4 - 16 bits would fit in an extra word in the HWRenderdetails
							- clear the word to zero on TA firstprod in sgxkick.c
		}
		else
		{
			- HW determines the ZLS controls for each partial render
			- implicitly no application ZLS controls
			- just kick partial render
			- Z buffer base is PB and setup by kernel services
		}
	*/

	/* load the client/application specified ZLSCTL value */
	ldr r1, #EUR_CR_ISP_ZLSCTL>>2, drc0;
	wdf		drc0;
	/* determine whether DPM or ext Z Buffer by testing for the compressed Zformat */
	/* if a compressed format is selected */
	and		r3, r1, #EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_MASK;
#if defined(SGX545)
	mov 	r2, #EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_CMPR_F32Z1V;
	xor.testz	p0, r3, r2;
	!p0	mov 	r2, #EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_CMPR_I8S1V;
	!p0 xor.testz	p0, r3, r2;
#else
	mov r2, #EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_F32ZI8S1V;
	xor.testz	p0, r3, r2;
#endif
#if defined(EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_CMPR_F32ZI8S1V)
	!p0 mov	r2, #EUR_CR_ISP_ZLSCTL_ZLOADFORMAT_CMPR_F32ZI8S1V;
	!p0 xor.testz	p0, r3, r2;
#endif
#if defined(SGX_FEATURE_ZLS_EXTERNALZ)
	/* if a compressed mode is selected check if it will be DPM allocated */
	p0	mov	r2, #EUR_CR_ISP_ZLSCTL_EXTERNALZBUFFER_MASK;
	p0	and.testz	p0, r1, r2;
#endif
	p0	ba		SPMAC_DPMDepthBuffer;
	{
		/* external Depth Buffer: */

		/* Check if there is a Z-buffer present */
		MK_MEM_ACCESS(ldad)	r3, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32RenderFlags)], drc0;
		mov				r4, #(SGXMKIF_RENDERFLAGS_DEPTHBUFFER | SGXMKIF_RENDERFLAGS_STENCILBUFFER);
		wdf		drc0;
		and.testnz		p0, r3, r4;
		p0 		ba	SPMAC_ZSBufferPresent;
		{
#if defined(FIX_HW_BRN_23861)
			MK_TRACE(MKTC_DUMMY_DEPTH_CS, MKTF_SPM | MKTF_SCH);

			/* setup the Dummy Depth, Stencil, load, store and ZLSCtrl registers */
			MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sDummyZLSReqBaseDevVAddr.uiAddr)], drc0;
			wdf		drc0;
			/* setup the ZLS req. base */
			str		#EUR_CR_BIF_ZLS_REQ_BASE >> 2, r3;

			MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sDummyStencilLoadDevVAddr.uiAddr)], drc0;
			wdf		drc0;
			str		#EUR_CR_ISP_STENCIL_LOAD_BASE >> 2, r3;

			MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sDummyStencilStoreDevVAddr.uiAddr)], drc0;
			wdf		drc0;
			str		#EUR_CR_ISP_STENCIL_STORE_BASE >> 2, r3;

			/*
				- Enable S load/store bits
				- set ZLS Ext to 1 tile (0)
				- set format to uncompressed ieee float
			*/
			mov		r3, #(EUR_CR_ISP_ZLSCTL_SLOADEN_MASK | \
							EUR_CR_ISP_ZLSCTL_SSTOREEN_MASK);
#else
			/* disable all ZLS if no ZBuffer present */
			/* ZLS module will load HW BGOBJ depth value only */
			mov		r4, ~#(EUR_CR_ISP_ZLSCTL_ZLOADEN_MASK |		\
							EUR_CR_ISP_ZLSCTL_SLOADEN_MASK |	\
							EUR_CR_ISP_ZLSCTL_ZSTOREEN_MASK |	\
							EUR_CR_ISP_ZLSCTL_SSTOREEN_MASK |	\
							EUR_CR_ISP_ZLSCTL_FORCEZLOAD_MASK |	\
							EUR_CR_ISP_ZLSCTL_FORCEZSTORE_MASK);
			and		r3, r1, r4;
#endif
			str		#EUR_CR_ISP_ZLSCTL >> 2, r3;
			/* don't bother with external Zbuffer checks if one doesn't exist */
			ba SPMAC_ZLSConfigured;
		}
		SPMAC_ZSBufferPresent:

		/*
			combine ZLSCTL with SPM specific controls, ensuring the
			mask plane is accumulated across partial renders
		*/
		/* Enable Stencil operations if stencil buffer is present */
		mov		r4, #SGXMKIF_RENDERFLAGS_STENCILBUFFER;
		and.testnz	p0, r3, r4;
		!p0	mov	r2, #0;
		p0	mov r2, #(EUR_CR_ISP_ZLSCTL_SLOADEN_MASK | EUR_CR_ISP_ZLSCTL_SSTOREEN_MASK);

		/* Enable Z operartions if a depth buffer is present */
		mov		r4, #SGXMKIF_RENDERFLAGS_DEPTHBUFFER;
		and.testnz	p0, r3, r4;
		!p0		ba	SPMAC_NoDepthBuffer;
		{
			MK_MEM_ACCESS(ldad)	r5, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sRTStatusDevAddr)], drc0;
			or	r2, r2, #(EUR_CR_ISP_ZLSCTL_ZLOADEN_MASK | EUR_CR_ISP_ZLSCTL_ZSTOREEN_MASK);
#if defined(EUR_CR_ISP_ZLSCTL_MSTOREEN_MASK)
			or	r2, r2, #EUR_CR_ISP_ZLSCTL_MSTOREEN_MASK;
#else
			or	r2, r2, #EUR_CR_ISP_ZLSCTL_STOREMASK_MASK;
#endif
			/* wait for status to load */
			wdf		drc0;
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
			imae	r5, R_RTAIdx.low, #SIZEOF(IMG_UINT32), r5, u32;
#endif
			MK_MEM_ACCESS(ldad)	r5, [r5], drc0;
			wdf		drc0;
			/* Don't load the mask on the first partial render */
			and.testnz		p0, r5, #SGXMKIF_HWRTDATA_RT_STATUS_SPMRENDER;
#if defined(EUR_CR_ISP_ZLSCTL_MLOADEN_MASK)
			p0	or	r2, r2, #EUR_CR_ISP_ZLSCTL_MLOADEN_MASK;
#else
			p0	or	r2, r2, #EUR_CR_ISP_ZLSCTL_LOADMASK_MASK;
#endif
		}
		SPMAC_NoDepthBuffer:
		or 		r2, r1, r2;
		/* and write register */
		str		#EUR_CR_ISP_ZLSCTL >> 2, r2;
		
		/* If client is requesting ZLS, set processempty bit */
		and.testz	p0, r2, #EUR_CR_ISP_ZLSCTL_FORCEZLOAD_MASK;
		p0	ba		SPMAC_NoProcessEmpty;
		{
			/* One of the force bits is set therefore we must set processempty bit */
			ldr		r3, #EUR_CR_ISP_IPFMISC >> 2, drc0;
			wdf		drc0;
			or		r3, r3, #EUR_CR_ISP_IPFMISC_PROCESSEMPTY_MASK;
			str		#EUR_CR_ISP_IPFMISC >> 2, r3;
		}
		SPMAC_NoProcessEmpty:

		ba		SPMAC_ZLSConfigured;
	}
	SPMAC_DPMDepthBuffer:
	{
	}
	SPMAC_ZLSConfigured:

	#if defined(FIX_HW_BRN_23870)
	/* for SPM we're always doing ZLS so we need to process empty regions */
	ldr		r3, #EUR_CR_ISP_IPFMISC >> 2, drc0;
	wdf		drc0;
	or		r3, r3, #EUR_CR_ISP_IPFMISC_PROCESSEMPTY_MASK;
	str		#EUR_CR_ISP_IPFMISC >> 2, r3;
	#endif

	/* check if this is a z-only render */
	MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32RenderFlags)], drc0;
	wdf		drc0;
	and.testz		p0, r4, #SGXMKIF_RENDERFLAGS_ZONLYRENDER;
	p0 		ba	SPMAC_NotZOnlyRender;
	{
		ldr		r3, #EUR_CR_ISP_ZLSCTL >> 2, drc0;
		wdf		drc0;
		or		r3, r3, #EUR_CR_ISP_ZLSCTL_ZONLYRENDER_MASK;
		str		#EUR_CR_ISP_ZLSCTL >> 2, r3;
	}
	SPMAC_NotZOnlyRender:
	
	/* Set up the pixel task part of the DMS control. */
	ldr		r3, #EUR_CR_DMS_CTRL >> 2, drc0;
	MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32NumPixelPartitions)], drc1;
	wdf		drc0;
	and		r3, r3, ~#EUR_CR_DMS_CTRL_MAX_NUM_PIXEL_PARTITIONS_MASK;
	wdf		drc1;
	or		r3, r3, r4;
	str		#EUR_CR_DMS_CTRL >> 2, r3;

#if defined(SGX_FEATURE_SYSTEM_CACHE)
	/* We need to invalidate at the whole cache */
	#if defined(SGX_FEATURE_MP)
		INVALIDATE_SYSTEM_CACHE(#EUR_CR_MASTER_SLC_CTRL_INVAL_ALL_MASK, #EUR_CR_MASTER_SLC_CTRL_FLUSH_ALL_MASK);
	#else
		INVALIDATE_SYSTEM_CACHE();
	#endif /* SGX_FEATURE_MP */
#endif

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
	INVALIDATE_EXT_SYSTEM_CACHE();
#endif /* SUPPORT_EXTERNAL_SYSTEM_CACHE */

	/* Invalidate the USE cache. */
#if defined(FIX_HW_BRN_25615)
	and.testnz	p0, R_CacheStatus, #CACHE_INVAL_USE;
	p0	ba SPMAC_NoUseInval;
	{
		/* The next three instructions must be done in a cache line */
		.align 16;
		str		#EUR_CR_USE_CACHE >> 2, #EUR_CR_USE_CACHE_INVALIDATE_MASK;
		ldr		r1, #EUR_CR_USE_CACHE >> 2, drc0;
		wdf		drc0;
		or		R_CacheStatus, R_CacheStatus, #CACHE_INVAL_USE;
	}
	SPMAC_NoUseInval:
#else
	INVALIDATE_USE_CACHE(SPMAC_, p0, r1, #SETUP_BIF_3D_REQ);
#endif

	/* Invalidate PDS cache */
	str		#EUR_CR_PDS_INV1 >> 2, #EUR_CR_PDS_INV1_DSC_MASK;
	INVALIDATE_PDS_CODE_CACHE();

	/* Invalidate the TSP cache	*/
	str	#EUR_CR_TSP_PARAMETER_CACHE >> 2, #EUR_CR_TSP_PARAMETER_CACHE_INVALIDATE_MASK;

	/* Wait for PDS cache to be invalidated */
	ENTER_UNMATCHABLE_BLOCK;
	SPMAC_WaitForPDSCacheFlush:
	{
		ldr		r1, #EUR_CR_PDS_CACHE_STATUS >> 2, drc0;
		wdf		drc0;
		and		r1, r1, #EURASIA_PDS_CACHE_PIXEL_STATUS_MASK;
		xor.testnz	p0, r1, #EURASIA_PDS_CACHE_PIXEL_STATUS_MASK;
		p0	ba		SPMAC_WaitForPDSCacheFlush;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the PDS cache flush event. */
	str		#EUR_CR_PDS_CACHE_HOST_CLEAR >> 2, #EURASIA_PDS_CACHE_PIXEL_CLEAR_MASK;

	/* If we aborted all MTs, Don't change RgnBase */
	and.testnz	p1, r11, #TA_IFLAGS_ABORTALL;
	p1	ba	SPMAC_NoRgnBaseChange;
	{
		/* Change region header base to point to terminated regions */
		MK_MEM_ACCESS_BPCACHE(ldad) 	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32AbortedMacroTile)], drc0;
		wdf		drc0;

#if defined(FIX_HW_BRN_27311) && !defined(FIX_HW_BRN_23055)
		MK_MEM_ACCESS(ldaw) 	r2, [R_RTData, #WOFFSET(SGXMKIF_HWRTDATA.ui16SPMRenderMask)], drc1;
		mov		r3, #1;
		and		r4, r1, #EUR_CR_DPM_ABORT_STATUS_MTILE_INDEX_MASK;
		shl		r3, r3, r4;
		wdf		drc1;
		or		r2, r2, r3;
		MK_MEM_ACCESS(staw) [R_RTData, #WOFFSET(SGXMKIF_HWRTDATA.ui16SPMRenderMask)], r2;
#endif
		and.testz		r1, p0, r1, #EUR_CR_DPM_ABORT_STATUS_MTILE_INDEX_MASK;
		/* If MT0 was aborted no change is required */
		p0	ba	SPMAC_RgnHeadersReady;
		{
			/*
				Addr = (MT * 16 * BlocksPerMT * EURASIA_REGIONHEADER_SIZE ) + base
			*/
			MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32NumTileBlocksPerMT)], drc1;
			mov		r3, #EURASIA_REGIONHEADER_SIZE;
			imae	r1, r1.low, r3.low, #0, u32;
			shl		r1, r1, #4;

			/* Move on core 0 first */
			ldr		r3, #EUR_CR_ISP_RGN_BASE >> 2, drc1;
			wdf		drc1;
#if !defined(SGX_FEATURE_MP)
			imae	r3, r1.low, r2.low, r3, u32;
			str		#EUR_CR_ISP_RGN_BASE >> 2, r3;
	
	#if defined(EUR_CR_ISP_MTILE)
			/* FIXME: this is a temporary solution for single core 
				variants of Ares2, Klio and Enyo. */
			str	#EUR_CR_ISP_MTILE >> 2, #0;
	#endif
#endif
#if defined(SGX_FEATURE_ZLS_EXTERNALZ)
			/* Make sure the ZLSExtRgnBase points to the same RgnHdrs as the ISP,
				incase a compressed format is being used */
	#if defined(EUR_CR_ISP2_ZLS_EXTZ_RGN_BASE)
			str		#EUR_CR_ISP2_ZLS_EXTZ_RGN_BASE >> 2, r3;
	#else
			str		#EUR_CR_ISP_ZLS_EXTZ_RGN_BASE >> 2, r3;
	#endif
#endif
#if defined(SGX_FEATURE_MP)
			/* Move on core 0 */
			mov		r5, #EUR_CR_MASTER_ISP_RGN_BASE >> 2;
			ldr		r3, r5, drc1;
			wdf		drc1;
			imae	r3, r1.low, r2.low, r3, u32;
			str		r5, r3;

			mov		r5, #EUR_CR_MASTER_ISP_RGN_BASE1 >> 2;
			
			/* Now move on the other cores */
			MK_LOAD_STATE_CORE_COUNT_3D(r4, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			SPMAC_SetupNextCoreRgnBase:
			{
				/* Check if there are anymore cores to setup */
				isub16.testz	r4, p0, r4, #1;
				p0	ba	SPMAC_RgnHeadersReady;
				{	
					/* Read the core RGN_BASE from master bank */
					ldr		r3, r5, drc1;
					wdf		drc1;
					imae	r3, r1.low, r2.low, r3, u32;
					/* Write to the core RGN_BASE in master bank */
					str		r5, r3;
					
					/* read the core specific RGN_BASE */
					REG_INCREMENT(r5, p0);
					
					ba	SPMAC_SetupNextCoreRgnBase;
				}
			}
#else
			ba	SPMAC_RgnHeadersReady;
#endif
		}
	}
	SPMAC_NoRgnBaseChange:
	{
#if defined(FIX_HW_BRN_27311) && !defined(FIX_HW_BRN_23055)
		MK_MEM_ACCESS(ldaw)	r3, [R_RTData, #WOFFSET(SGXMKIF_HWRTDATA.ui16BRN27311Mask)], drc0;
		wdf		drc0;
		MK_MEM_ACCESS(staw) [R_RTData, #WOFFSET(SGXMKIF_HWRTDATA.ui16SPMRenderMask)], r3;
#endif
#if defined(SGX_FEATURE_ZLS_EXTERNALZ)
		/* 	Make sure the ZLSExtRgnBase points to the same RgnHdrs as the ISP,
			incase a compressed format is being used */
		ldr		r3, #EUR_CR_ISP_RGN_BASE >> 2, drc0;
		wdf		drc0;
		
#if defined(EUR_CR_ISP2_ZLS_EXTZ_RGN_BASE)
		str		#EUR_CR_ISP2_ZLS_EXTZ_RGN_BASE >> 2, r3;
#else
		str		#EUR_CR_ISP_ZLS_EXTZ_RGN_BASE >> 2, r3;
#endif
#endif
	}
	SPMAC_RgnHeadersReady:

	/*
		uKernel needs to access a per-context heaps
	*/
	SWITCHEDMTO3D();

#if defined(FIX_HW_BRN_23862)
	/*
		p1 = TRUE if we are rendering all macrotiles.
	*/
	MK_LOAD_STATE(r1, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testnz		p1, r1, #TA_IFLAGS_ABORTALL;

	/*
		Apply the BRN23862 fix.
			r9 - Render details structure
			r10 - Macrotile we are rendering.
	*/
	mov				r9, r0;
	MK_MEM_ACCESS_BPCACHE(ldad) 	r10, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32AbortedMacroTile)], drc0;
	wdf				drc0;
	p1	mov			r10, #EUR_CR_DPM_ABORT_STATUS_MTILE_GLOBAL_MASK;
	bal				FixBRN23862;
#endif /* defined(FIX_HW_BRN_23862) */

#if defined(FIX_HW_BRN_30089) && defined(SGX_FEATURE_MP)
	/* Are we rendering all macrotiles. */
	MK_LOAD_STATE(r1, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testnz		p1, r1, #TA_IFLAGS_ABORTALL;
	
	/* if we are not rendering all MTs load the idx for the 1 we are rendering */
	!p1	MK_MEM_ACCESS_BPCACHE(ldad) 	r10, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32AbortedMacroTile)], drc0;
	p1	mov	r10, #EUR_CR_DPM_ABORT_STATUS_MTILE_GLOBAL_MASK;
	/* Will only wait if there is a load pending */
	wdf		drc0;
	/* Modify the region headers as required */
 	and		r10, r10, #EUR_CR_DPM_ABORT_STATUS_MTILE_GLOBAL_MASK | EUR_CR_DPM_ABORT_STATUS_MTILE_INDEX_MASK;
	ADDLASTRGN(R_RTData, r10);
#endif

	/* Copy background object state words */
	MK_MEM_ACCESS(ldad)		r4, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sBGObjBaseDevAddr)], drc0;
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	MK_MEM_ACCESS(ldad.f2)	r1, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.aui32SpecObject)-1], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(stad.f2)	[r4, #0++], r1;

	/* Update the HWBGObj so it references the correct surface */
	MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWBgObjPrimPDSUpdateListDevAddr)], drc1;
	wdf		drc1;
	imae	r1, R_RTAIdx.low, #SIZEOF(IMG_UINT32), r1, u32;
	MK_MEM_ACCESS(ldad)	r2, [r1], drc1;
	wdf		drc1;
	MK_MEM_ACCESS_BPCACHE(stad)	[r4, #0++], r2;
#else
	MK_MEM_ACCESS(ldad.f3)	r1, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.aui32SpecObject)-1], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(stad.f3)	[r4, #0++], r1;
#endif
	idf		drc0, st;
	wdf		drc0;

	/* Set the page directory base register select back to the original value. */
	SWITCHEDMBACK();

	MK_TRACE(MKTC_SPM_KICKRENDER, MKTF_SPM | MKTF_SCH | MKTF_HW);

#if (defined(FIX_HW_BRN_24304) || defined(FIX_HW_BRN_27510)) && defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	CONFIGURECACHE(#CC_FLAGS_3D_KICK);
#endif

	ldr		r10, #EUR_CR_DPM_PAGE >> 2, drc0;
	wdf		drc0;
	and		r10, r10, #EUR_CR_DPM_PAGE_STATUS_3D_MASK;
	shr		r10, r10, #EUR_CR_DPM_PAGE_STATUS_3D_SHIFT;

	WRITEHWPERFCB(3D, 3DSPM_START, GRAPHICS, r10, p0, spmac_start_3d, r2, r3, r4, r5, r6, r7, r8, r9);

#if  defined(FIX_HW_BRN_32044)
	/*
	Register Usages
	r0  ----->  Points to the base address of the fake control streams.
	r1 ------> Temporary Register.
	r2 ------> Base address of the original control streams derived from Region headers
	r3 ------> Base address of the region header
	r4 ------> Loop count for the cores.
	r5 ------> Temp register
	r6 ------> Loop count that keeps track of number of tiles
	r7 ------> Total Number of Tiles
	r8 ------> Control stream base address offset from the parameter buffer base
	r9 ------> Temp
	r10 -----> Holds the MASTER_BIF_3D_REQ_BASE address
	*/

	/* Now move on the other cores */
	MK_LOAD_STATE_CORE_COUNT_TA(r4, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

        MK_MEM_ACCESS_BPCACHE(ldad)    r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderDetails)], drc0;
        wdf                   drc0;
        MK_MEM_ACCESS_BPCACHE(ldad)    r0, [r0, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWRTDataSetDevAddr)], drc1;
        wdf                   drc1;
	MK_MEM_ACCESS_BPCACHE(ldad)	r6, [r0, #DOFFSET(SGXMKIF_HWRTDATASET.ui32NumTiles)], drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [r0, #DOFFSET(SGXMKIF_HWRTDATASET.s32044CtlStrmDevAddr)], drc1;
	wdf		drc0;
	wdf		drc1;
	
	mov		r2, #EUR_CR_MASTER_BIF_3D_REQ_BASE >> 2;
	ldr		r10, r2, drc0;
	mov		r7, r6;
	xor.testz	p2, r7, r7;
	/* Move on core 0 & load the ISP region base */
	mov		r5, #EUR_CR_MASTER_ISP_RGN_BASE >> 2;
	wdf		drc0;

	SPMAC_PatchUpNextCoreRgnBase:
	{
		ldr		r3, r5, drc1;
		wdf		drc1;

		SPMAC_LoopForAllTiles:
		{
			MK_MEM_ACCESS(ldad)	r2, [r3, #0], drc1;

			/* Do nothing if the Region header is empty or invalid */
			mov		r1, #(EURASIA_REGIONHEADER0_EMPTY | EURASIA_REGIONHEADER0_INVALID);
			wdf		drc1;
			and.testnz	p1, r2, r1;
			p1		ba SPMAC_Moveto_NextRegionHeader;
			{
				/* Load the Control stream base address from the region header */
				MK_MEM_ACCESS(ldad)	r8, [r3, #1], drc1;
				wdf		drc1;
				and		r8, r8, #~EURASIA_REGIONHEADER1_CONTROLBASE_CLRMSK;
				shr		r8, r8, #EURASIA_REGIONHEADER1_CONTROLBASE_SHIFT;
				shl		r8, r8, #EURASIA_REGIONHEADER1_CONTROLBASE_ALIGNSHIFT;

				/* r2 = r8 (Control stream base address offset )+ r10 (BIF_3D_REQ_BASE) */
				IADD32(r2,r8, r10, r9);

				/* Check if the fake control stream is already patched before if yes skip patching */				
				xor.testz	p1, r2, r0;
				p1		ba SPMAC_Moveto_NextRegionHeader;
				{

					/* Read the Control Stream block header */
					/* The first header is always going to be PIM header */
					MK_MEM_ACCESS(ldad)	r1, [r2, #0], drc1;

					/* Copy the base address of the original control stream from Region header to the fake control stream link ptr*/
					MK_MEM_ACCESS(stad)		[r0, #3] , r8;
										
					/*r8 = r0(Fake Control stream Address) - r10 (BIF_3D_REQ_BASE)*/
					ISUB32(r8, r0, r10, r2);
					/* Copy the base address of the fake control stream to the region header control stream ptr*/
					MK_MEM_ACCESS(stad)		[r3, #1], r8;

					wdf		drc1;
					/* Copy the original PIM header from Region header to the fake control stream */
					MK_MEM_ACCESS(stad)		[r0], r1;
					
				}
			}

			SPMAC_Moveto_NextRegionHeader:

			/* Increment the region header to next cell */
			mov		r1, #EURASIA_REGIONHEADER_SIZE;
			iaddu32		r3, r1.low, r3;

			/* Increment the fake control stream ptr to the next control stream */
			mov		r1, #64;
			iaddu32		r0, r1.low, r0;

			/* Check if there are anymore tiles to be processed */
			mov		r1, #0xFFFFFFFF;
			iadd32.testz	r6, p1, r1, r6;
			!p1		ba SPMAC_LoopForAllTiles;
			
		}
		
		mov		r6, r7;
		p2 mov		r5, #EUR_CR_MASTER_ISP_RGN_BASE1 >> 2;
		p2 ba 		SPMAC_Checkfor_MoreCores;
		
		/* read the core specific RGN_BASE */
		REG_INCREMENT(r5, p0);
		
		SPMAC_Checkfor_MoreCores:
		p2 xor.testnz	p2, r5, r5;

		/* Check if there are anymore cores to setup */
		isub16.testz	r4, p0, r4, #1;
		!p0	ba	SPMAC_PatchUpNextCoreRgnBase;
	}

	idf		drc0, st;
	wdf		drc0;
#endif

	/* Start the render */
	str		#EUR_CR_ISP_START_RENDER >> 2, #EUR_CR_ISP_START_RENDER_PULSE_MASK;

	/* Indicate a SPM render is in progress */
	MK_LOAD_STATE(r0, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
#if 1 // SPM_PAGE_INJECTION
	MK_LOAD_STATE(r1, ui32ITAFlags, drc1);
	MK_WAIT_FOR_STATE_LOAD(drc1);
	shl.tests	p0, r1, #(31 - TA_IFLAGS_SPM_STATE_FREE_ABORT_BIT);
	p0	or		r0, r0, #RENDER_IFLAGS_SPM_PB_FREE_RENDER;
#endif
	or		r0, r0, #RENDER_IFLAGS_RENDERINPROGRESS | \
					 RENDER_IFLAGS_SPMRENDER | \
					 RENDER_IFLAGS_WAITINGFOREOR | \
					 RENDER_IFLAGS_WAITINGFOR3DFREE;
	MK_STORE_STATE(ui32IRenderFlags, r0);
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRTData)], R_RTData;

	idf		drc0, st;
	wdf		drc0;
	
SPMAC_Exit:
	/* branch back to calling code */
	mov		pclink, R_EDM_PCLink;

	/* Return */
	lapc;
}

#if defined(FIX_HW_BRN_23055)
/*****************************************************************************
 FindMTEPageInCSM routine - Stores out the CSM and checks entries for each MT
		to find out where the "hidden" page is allocated.
 inputs:   none
 temps:		r1, r2, r3, r4, r5, r6
 Preds:		p0
******************************************************************************/
FindMTEPageInCSM:
{
	MK_TRACE(MKTC_FIND_MTE_PAGE_IN_CSM, MKTF_SPM | MKTF_HW);

	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], drc0;
	wdf	drc0;

	MK_MEM_ACCESS(ldad)	r2, [r1, #DOFFSET(SGXMKIF_HWRTDATA.sContextOTPMDevAddr[0])], drc0;
	wdf	drc0;
	/* Flush OTPM */
	#if defined(EUR_CR_MTE_OTPM_CSM_BASE)
	str	#EUR_CR_MTE_OTPM_CSM_BASE >> 2, r2;
	#else
	str	#EUR_CR_MTE_OTPM_CSM_FLUSH_BASE >> 2, r2;
	#endif
	str	#EUR_CR_MTE_OTPM_OP >> 2, #EUR_CR_MTE_OTPM_OP_CSM_FLUSH_MASK;

	/* Wait for all the events to be flagged complete */
	ENTER_UNMATCHABLE_BLOCK
	FMTEPCSM_WaitForCSMStore:
	{
		ldr	r3, #EUR_CR_EVENT_STATUS >> 2, drc0;
		wdf	drc0;
		shl.testns	p0, r3, #(31 - EUR_CR_EVENT_STATUS_OTPM_FLUSHED_SHIFT);
		p0	ba	FMTEPCSM_WaitForCSMStore;
	}
	LEAVE_UNMATCHABLE_BLOCK

	mov	r17, #EUR_CR_EVENT_HOST_CLEAR_OTPM_FLUSHED_MASK;
	str	#EUR_CR_EVENT_HOST_CLEAR >> 2, r17;

	SWITCHEDMTOTA();

	mov		r1, #0xFFFF;
	MK_MEM_ACCESS_BPCACHE(staw)	[R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16LastMTEPageRgn)], r1;
	idf		drc1, st;
	wdf		drc1;

	/* look through the CSM table, to find the valid entry */
	mov	r1, #0;
	mov	r4, #USE_FALSE;

	MK_MEM_ACCESS_BPCACHE(ldad)	r3, [r2, #1++], drc0;
	FMTEPCSM_CheckNextRgn:
	{
		/* Wait for the entry to load */
		wdf	drc0;

		/* check the values */
		shl.testns	p0, r3, #(31 - EURASIA_PARAM_OTPM_CSM_VALID_SHIFT);
		p0	ba	FMTEPCSM_InvalidEntry;
		{
			/* We have found the rgn and page so record it */
			and	r3, r3, #EURASIA_PARAM_OTPM_CSM_PAGE_MASK;
			shr	r3, r3, #EURASIA_PARAM_OTPM_CSM_PAGE_SHIFT;

			MK_TRACE(MKTC_MTE_PAGE_FOUND, MKTF_SPM | MKTF_HW);
			MK_TRACE_REG(r1, MKTF_SPM | MKTF_HW);
			MK_TRACE_REG(r3, MKTF_SPM | MKTF_HW);

			xor.testz	p0, r4, #USE_TRUE;
			!p0	ba	FMTEPCSM_FirstFind;
			{
				lock; lock;
			}
			FMTEPCSM_FirstFind:
			mov	r4, #USE_TRUE;

			MK_MEM_ACCESS_BPCACHE(staw)	[R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16LastMTEPageRgn)], r1;
			MK_MEM_ACCESS_BPCACHE(staw)	[R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16LastMTEPageAllocation)], r3;
			idf	drc0, st;
			ba	FMTEPCSM_PageFound;
		}
		FMTEPCSM_InvalidEntry:

		iaddu32		r1, r1.low, #1;
		xor.testnz	p0, r1, #EURASIA_NUM_MACROTILES;
		p0	MK_MEM_ACCESS_BPCACHE(ldad)	r3, [r2, #1++], drc0;
		p0	ba	FMTEPCSM_CheckNextRgn;
	}
	FMTEPCSM_PageFound:

	/* wait for the store to finish */
	wdf	drc0;

	SWITCHEDMBACK();

	/* return */
	lapc;
}

/*****************************************************************************
 ReIssueMTEPage routine - Remove the MTE page from the free list and
			  adds it to the state table on the TA for in the correct
			  region.
 inputs:   none
 temps:		r1, r2, r3, r4, r5, r6, r7, r8
 Preds:		p0
******************************************************************************/
ReIssueMTEPage:
{
	MK_TRACE(MKTC_REISSUE_MTE_PAGE, MKTF_SPM | MKTF_HW);

#if defined(DEBUG)
	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32FLPatchOutOfMem)], drc0;
	wdf	drc0;
	mov	r2, #1;
	iaddu32	r1, r2.low, r1;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32FLPatchOutOfMem)], r1;
	idf	drc1, st;
	wdf	drc1;

#endif

	/* Store the TA context as we need to modify it */
	MK_MEM_ACCESS_BPCACHE(ldad)	r1,[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], drc0;

	/* Set the LS bit to the same as the TA context */
	ldr	r2, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc1;

	/* We need the whole state table */
	str	#EUR_CR_DPM_LSS_PARTIAL_CONTEXT >> 2, #0;

	/* Wait for CONTEXT_ID register to load */
	wdf	drc1;
	and	r3, r2, #EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_MASK;
	shr	r3, r3, #EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_SHIFT;
	and	r2, r2, ~#EUR_CR_DPM_STATE_CONTEXT_ID_LS_MASK;
	or	r2, r2, r3;
	str	#EUR_CR_DPM_STATE_CONTEXT_ID >> 2, r2;

	/* Wait for the sTARTData */
	wdf	drc0;
	MK_MEM_ACCESS(ldad)	r4, [r1, #DOFFSET(SGXMKIF_HWRTDATA.sContextStateDevAddr)], drc0;
	/* Wait for the state table address to load */
	wdf	drc0;
	SETUPSTATETABLEBASE(MPFFL, p1, r4, r3);

	/* Do the state table store*/
	PVRSRV_SGXUTILS_CALL(DPMStateStore);

	/* Load the MTE page details */
	MK_MEM_ACCESS_BPCACHE(ldaw)	r2, [R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16LastMTEPageAllocation)], drc1;
	MK_MEM_ACCESS_BPCACHE(ldaw)	r3, [R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16LastMTEPageRgn)], drc1;
	wdf	drc1;

	/* Switch the EDM to the TA address space so we can modify the Free list and state table */
	SWITCHEDMTOTA();

	/*
		Look at the state table entry for the MTEPageRgn,
		if there are pages allocated, traverse the list to see if the MTE page is one of them
		if it is there is nothing to do, exit!!
	*/
	READSTATETABLEBASE(RMTEP_Check, p0, r1, r1, drc0);
	/* Read the entry for the region affected */
	imae	r1, r3.low, #8, r1, u32;
	MK_MEM_ACCESS_BPCACHE(ldad)	r5, [r1, #0], drc0;
	wdf	drc0;

	/* If the page count is non-zero, continue checking */
	mov	r6, ~#EURASIA_PARAM_DPM_STATE_MT_COUNT_CLRMSK;
	and.testz	p0, r5, r6;
	p0	ba	RMTEP_ReIssuePage;
	{
		/* The page count is non-zero therefore, we must traverse the page list */
		ldr	r1, #EUR_CR_DPM_TA_ALLOC_PAGE_TABLE_BASE >> 2, drc0;
		wdf	drc0;

		/* extract the head from the state table entry */
		and	r5, r5, ~#EURASIA_PARAM_DPM_STATE_MT_HEAD_CLRMSK;

		RMTEP_CheckNextPage:
		{
			/* Is this the MTE page, no re-issue is required */
			xor.testz	p0, r5, r2;
			p0	ba	RMTEP_NoReIssueRequired;
			
			/* It was not the MTE page so lets move on */
			imae r6, r5.low, #4, r1, u32;
			/* Read the data */
			MK_MEM_ACCESS_BPCACHE(ldad)	r5, [r6], drc0;
			wdf	drc0;
			/* Extract the next page entry */
			and 	r5, r5, ~#EURASIA_PARAM_DPM_PAGE_LIST_NEXT_CLRMSK;

			/* Have we reached the end of the list */
			mov 	r7, #EURASIA_PARAM_DPM_PAGE_LIST_NEXT_TERM_MSK;
			xor.testz	p0, r5, r7;
			!p0	ba	RMTEP_CheckNextPage;
		}		
		/* We have reached the end. The page must not have been reissued */
	}
	RMTEP_ReIssuePage:
	{
		MK_TRACE(MKTC_REISSUE_MTE_PAGE_REQUIRED, MKTF_SPM | MKTF_HW);

		/* Get the TA and 3D HWPBDescs */
		MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc.uiAddr)], drc0;

		/* Store TA PB */
		STORETAPB();
		
		/* PB and state table have been stored. We need to
			1. Check if page is in free list
			2. Remove the MTE page from the free list
			3. Add the MTE page to the state table and increment the count
		*/

		/* 1. Check if the page is in the free list */
		
		/* TA PB */
		ldr r4, #EUR_CR_DPM_TA_ALLOC_PAGE_TABLE_BASE >> 2, drc1;
		ldr r6, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_STATUS1 >> 2, drc1;
		wdf	drc1;
	
		/* 2. Remove the MTE page from the free list start */
		/*	
			r1 - sTAHWPBDesc (at this point has)
			r2 - MTE page number (at this point has)
			r3 - MTE page region (at this point has)
			r4 - Address of the TA PB page table 
			r5 - MTE address (To be loaded)
		*/
		/* calculate the adress of the page in the table */


		/* find the MTE page address */
		imae	r5, r2.low, #4, r4, u32;
		/* Read the data */
		MK_MEM_ACCESS_BPCACHE(ldad)	r6, [r5], drc0;
		wdf drc0;

		/*
			r6 - MTE page entry
		*/
		/* Start re-linking the free list */
		/* Extract the Prev entry value */
		shr		r5, r6, #EURASIA_PARAM_DPM_PAGE_LIST_PREV_SHIFT;
		/* Extract the Next entry value */
		and		r6, r6, ~#EURASIA_PARAM_DPM_PAGE_LIST_NEXT_CLRMSK;
		/* If the Prev entry is terminate, MTE page is head */
		mov		r7, #EURASIA_PARAM_DPM_PAGE_LIST_NEXT_TERM_MSK;
		xor.testz	p1, r5, r7;
		p1	ba	RMTEP_MTEPageHead;
		{
			/* Calculate MTE page's prev entry page address */
			imae	r8, r5.low, #4, r4, u32;
			/* Write "next" value to the lower 16 bits using masked write. */
			MK_MEM_ACCESS_BPCACHE(staw)	[r8], r6;
	
			ba	RMTEP_PrevEntryUpdated;
		}
		RMTEP_MTEPageHead:
		{
			/* The MTE page is the head so we need to modify the HWPBDesc to reflect this */
			/* We can do a masked 16 bit write to update the value */
			MK_MEM_ACCESS(staw)	[r1, #WOFFSET(SGXMKIF_HWPBDESC.ui32FreeListHT)], r6;
		}
		RMTEP_PrevEntryUpdated:
		                          
		/*
			r1 - sTAHWPBDesc
			r2 - MTE page number
			r3 - MTE page region
			r4 - Address of the TA PB page table
			r5 - MTE Prev Page entry
			r6 - MTE Next Page entry
		*/
		mov		r7, #EURASIA_PARAM_DPM_PAGE_LIST_NEXT_TERM_MSK;
		xor.testz	p1, r6, r7;
		p1	ba	RMTEP_MTEPageTail;
		{
			/* find out the MTE page's next page address */
			imae	r8, r6.low, #4, r4, u32;
			/* We need to modify the top 16 bits so add another 2 bytes */
			mov		r7, #2;
			iaddu32 r8, r7.low, r8;
			/* Write the "prev" value to the upper 16 bits using masked write. */
			MK_MEM_ACCESS_BPCACHE(staw)	[r8], r5;
			
			/* The MTE page is not the tail but we need to check whether it is the FreeListPrev */
			MK_MEM_ACCESS(ldad)	r7, [r1, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListPrev)], drc0;
			wdf		drc0;
			shr		r7, r7, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_PREVIOUS_SHIFT;
			xor.testnz	p1, r2, r7;
			p1 ba	RMTEP_NextEntryUpdated;
			
			/* If we get here the MTE page is the FreeListPrev therefore it needs to be modified */
			/* NOTE: If the MTE page is the FreeListPrev is will not be the Head, as we have just 
				freed atleast 1 page and there are always 2 left in the PB */
			/* Move the MTE "prev" into r7 */
			shl		r7, r5, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_PREVIOUS_SHIFT;
					
			ba	RMTEP_UpdateFreeListPrev;
		}
		RMTEP_MTEPageTail:
		{
			/* Wait for all the previous stores to complete */
			idf 	drc1, st;
			wdf		drc1;
			/* The MTE page is the Tail so we need to modify the HWPBDesc to reflect this */
			/* We can do a masked 16 bit write to update the value */
			MK_MEM_ACCESS(staw)	[r1, #WOFFSET(SGXMKIF_HWPBDESC.ui32FreeListHT)+1], r5;
	
			/* 
				We also need to modify the FreeListPrev, so we need to get the 
				prev value of the MTE's Prev page
			*/
			/* find out the "prev" page address */
			imae	r8, r5.low, #4, r4, u32;
			/* Now get the entry */
			MK_MEM_ACCESS_BPCACHE(ldad)	r7, [r8], drc0;
			wdf		drc0;
			/* Extract the Prev value */
			and		r7, r7, ~#EURASIA_PARAM_DPM_PAGE_LIST_PREV_CLRMSK;
			
			RMTEP_UpdateFreeListPrev:
			
			MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListPrev)], r7;	
		}
		RMTEP_NextEntryUpdated:
		
		/*
			r1 - sTAHWPBDesc
			r2 - MTE page number
			r3 - MTE page region
			r4 - Address of the TA PB page table
		*/
		/* Increment the "pages allocated" count in the HWPBDesc */
		/* Is the page allocated to the Global List? */
		xor.testz	p1, r3, #(EURASIA_NUM_MACROTILES - 1);
		MK_MEM_ACCESS(ldad)	r5, [r1, #DOFFSET(SGXMKIF_HWPBDESC.uiLocalPages)], drc0;
		p1	MK_MEM_ACCESS(ldad)	r6, [r1, #DOFFSET(SGXMKIF_HWPBDESC.uiGlobalPages)], drc0;
		wdf		drc0;
		iaddu32	r5, r5.low, #1;
		p1	iaddu32	r6, r6.low, #1;
		MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWPBDESC.uiLocalPages)], r5;
		p1	MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWPBDESC.uiGlobalPages)], r6;
	
		/* Get the real ZLS threshold */
		MK_MEM_ACCESS(ldad)	r5, [r1, #DOFFSET(SGXMKIF_HWPBDESC.ui32ZLSThreshold)], drc0;
		wdf		drc0;
		MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWPBDESC.ui32ZLSThreshold)], #0;
		idf		drc1, st;
		wdf		drc1;
		
		/* Reload the PB now that the page has been removed */
#if defined(FIX_HW_BRN_32302) || defined(SUPPORT_SGX_DOUBLE_PIXEL_PARTITIONS)
		mov	r6, #USE_FALSE;
		LOADTAPB(r1, r6);
#else
		LOADTAPB(r1);
#endif
		!p0	ba	RMTEP_No3DPBLoad;
		{
			LOAD3DPB(r1);
			
			/* Re-enable the pixel data master. */
			ldr		r6, #EUR_CR_DMS_CTRL >> 2, drc0;
			wdf		drc0;
			and		r6, r6, ~#EUR_CR_DMS_CTRL_DISABLE_DM_PIXEL_MASK;
			str		#EUR_CR_DMS_CTRL >> 2, r6;
		}
		RMTEP_No3DPBLoad:
		
		/* Restore the original ZLS threshold value */
		MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWPBDESC.ui32ZLSThreshold)], r5;
		idf		drc1, st;
		wdf		drc1;
		
		/* Switch EDM back to TA as LOADTAPB reverts back to EDM address space */
		SWITCHEDMTOTA();
		
		RMTEP_FLEntryNotFound:

		/* find the MTE page address */
		imae	r5, r2.low, #4, r4, u32;
		
		/* Initialise prev and next to the term masks, so we can later, update the prev/next based on
		   how we add the page back to the alloc-list */

		mov		r7, #(EURASIA_PARAM_DPM_PAGE_LIST_PREV_TERM_MSK | EURASIA_PARAM_DPM_PAGE_LIST_NEXT_TERM_MSK);
		MK_MEM_ACCESS_BPCACHE(stad)	[r5], r7;
		idf drc0, st;
		wdf drc0;
		/* Now we have to add the page to the state table */
		
		/* 2. Add the MTE page to the state table and update count start */


		/* Put the MTE page back on the correct Rgn state */
		/*
			r1 - DPM State table base
			r2 - MTE page number
			r3 - MTE page region
			r4 - Address of the TA PB page table
		*/
		READSTATETABLEBASE(RMTEP_, p0, r1, r1, drc0);
		
		/* Read the Entries for the region affected */
		imae	r1, r3.low, #8, r1, u32;
		MK_MEM_ACCESS_BPCACHE(ldad)	r5, [r1, #0], drc0;
		MK_MEM_ACCESS_BPCACHE(ldad)	r6, [r1, #1], drc0;
		wdf		drc0;
		
		/* 
			r1 - DPM State table addr
			r2 - MTE page number
			r4 - Address of the TA PB page table
			r5 - [Count | Head]
			r6 - [ZLS Count | Tail]
		*/
		mov		r7, ~#EURASIA_PARAM_DPM_STATE_MT_COUNT_CLRMSK;
		and.testnz	p0, r5, r7;
		p0	ba	RMTEP_RgnCountNonZero;
		{
			/* There are no pages allocated, so update the head */
			mov		r5, #(1 << EURASIA_PARAM_DPM_STATE_MT_COUNT_SHIFT);
			or		r5, r5, r2;
	
			/* We have updates the count and Head so now lets update the Tail */
			ba	RMTEP_UpdateTail;
		}
		RMTEP_RgnCountNonZero:
		{
			/* Increment the count */
			and		r3, r5, ~#EURASIA_PARAM_DPM_STATE_MT_COUNT_CLRMSK;
			and		r5, r5, #EURASIA_PARAM_DPM_STATE_MT_COUNT_CLRMSK;
			shr		r3, r3, #EURASIA_PARAM_DPM_STATE_MT_COUNT_SHIFT;
			iaddu32	r3, r3.low, #1;
			shl		r3, r3, #EURASIA_PARAM_DPM_STATE_MT_COUNT_SHIFT;
			or		r5, r5, r3;
			
			/* Link the MTE page with the current tail */
			/* Extract the old tail value */
	#if ((EURASIA_PARAM_PAGENUMBER_BITS == 16) || \
		((EURASIA_PARAM_PAGENUMBER_BITS == 14) && defined(SGX_FEATURE_UNPACKED_DPM_STATE)))
			and		r3, r6,  ~#EURASIA_PARAM_DPM_STATE_MT_TAIL_CLRMSK;
	#else
			/* Extract the bottom nibble from DWORD 0 */
			and		r3, r5, #EURASIA_PARAM_DPM_STATE_MT_TAIL_0_MASK;
			/* Get the bottom 4-bits in position */
			shr		r3, r3, #EURASIA_PARAM_DPM_STATE_MT_TAIL_0_SHIFT;
			
			/* Remove the old tail fom the bottom 10 bits of DWORD 1 */
			and		r7, r6, #EURASIA_PARAM_DPM_STATE_MT_TAIL_1_MASK;
			/* Set the top 10 bits in the lower 10 bits of DWORD 1*/
			shl		r7, r7, #EURASIA_PARAM_DPM_STATE_MT_TAIL_1_ALIGNSHIFT;
			or		r3, r3, r7;
	#endif
			/* Now we have the Tail page number, we need to patch the "next" value */
			imae	r7, r3.low, #4, r4, u32;
			/* Write the MTE page number to the lower WORD */
			MK_MEM_ACCESS_BPCACHE(staw)	[r7], r2;
			
			/* Now link the MTE page back to the Tail */
			imae	r7, r2.low, #4, r4, u32;
			shl		r3, r3, #EURASIA_PARAM_DPM_PAGE_LIST_PREV_SHIFT;
			or		r8, r3, #EURASIA_PARAM_DPM_PAGE_LIST_NEXT_TERM_MSK;
			/* Write the MTE page number to the higher WORD */
			MK_MEM_ACCESS_BPCACHE(stad)	[r7], r8;
		}
		RMTEP_UpdateTail:
		/* If required the page has already been linked into the list,
			so now we just need to update the tail entry */	
		
		/* remove the old tail value and insert the new value */
	#if ((EURASIA_PARAM_PAGENUMBER_BITS == 16) || \
		((EURASIA_PARAM_PAGENUMBER_BITS == 14) && defined(SGX_FEATURE_UNPACKED_DPM_STATE)))
		and		r6, r6,  #EURASIA_PARAM_DPM_STATE_MT_TAIL_CLRMSK;
		/* Set the tail to the MTE page */
		or		r6, r6, r2;
	#else
		/* Remove the old tail nibble from DWORD 0 */
		and		r5, r5, ~#EURASIA_PARAM_DPM_STATE_MT_TAIL_0_MASK;
		/* Get the bottom 4-bits in position */
		shl		r3, r2, #EURASIA_PARAM_DPM_STATE_MT_TAIL_0_SHIFT;
		/* Set the bottom nibble in the top nibble of DWORD 0 */
		or		r5, r5, r3;
		
		/* Remove the old tail fom the bottom 10 bits of DWORD 1 */
		and		r6, r6, ~#EURASIA_PARAM_DPM_STATE_MT_TAIL_1_MASK;
		/* Set the top 10 bits in the lower 10 bits of DWORD 1*/
		shr		r3, r2, #EURASIA_PARAM_DPM_STATE_MT_TAIL_1_ALIGNSHIFT;
		or		r6, r6, r3;
	#endif
	
		/* write the values into memory */
		MK_MEM_ACCESS_BPCACHE(stad.f2)	[r1, #0++], r5;


		/* 2. Add the MTE page to the state table and update count end */

		mov		r1, #0xFFFF;
		MK_MEM_ACCESS_BPCACHE(staw)	[R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16LastMTEPageRgn)], r1;
		idf		drc1, st;
		wdf		drc1;
		
		/* Load the context */
		PVRSRV_SGXUTILS_CALL(DPMStateLoad);
		
	}
	RMTEP_NoReIssueRequired:

	SWITCHEDMBACK();
	
	/* we have corrected the state for the TA and Freelist so return TRUE and exit */
	mov	r1, #USE_TRUE;
	MK_TRACE(MKTC_REISSUE_MTE_PAGE_END, MKTF_SPM | MKTF_HW);

	/* Return */
	lapc;
}

/*****************************************************************************
 FindMTEPageInStateTable routine - Stores out the current TA state and checks the
 			tail value of each rgn to find out where the page was allocated.

 inputs: none
 temps:		r1, r2, r3, r4, r5, r6
 Preds:		p2
*****************************************************************************/
FindMTEPageInStateTable:
{
	MK_TRACE(MKTC_FIND_MTE_PAGE_IN_STATE, MKTF_SPM | MKTF_HW);

	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS(ldad)	r5, [r1, #DOFFSET(SGXMKIF_HWRTDATA.sContextStateDevAddr)], drc0;
	/* make sure the LS bit is the same as the TA */
	ldr		r3, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc1;
	wdf		drc1;
	and		r3, r3, ~#EUR_CR_DPM_STATE_CONTEXT_ID_LS_MASK;
	and		r4, r3, #EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_MASK;
	shr		r4, r4, #(EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_SHIFT - EUR_CR_DPM_STATE_CONTEXT_ID_LS_SHIFT);
	or		r3, r3, r4;
	str		#EUR_CR_DPM_STATE_CONTEXT_ID >> 2, r3;
	MK_MEM_ACCESS_BPCACHE(ldaw)	r2, [R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16LastMTEPageAllocation)], drc1;
	/* wait for r1 */
	wdf		drc0;
	SETUPSTATETABLEBASE(FMTEP, p2, r5, r4);
	PVRSRV_SGXUTILS_CALL(DPMStateStore);

	SWITCHEDMTOTA();

	/* look through the state table, to find the MTE page (r2) */
	/* offset */
	mov		r1, #0;
	wdf		drc1;
	MK_MEM_ACCESS_BPCACHE(ldad.f2)		r3, [r5, #0++], drc0;
	FMTEP_CheckNextRgn:
	{
		wdf		drc0;

		/* check the values */
		and		r6, r3, ~#EURASIA_PARAM_DPM_STATE_MT_COUNT_CLRMSK;
		and.testz	p2, r6, r6;
		p2	ba	FMTEP_ZeroCount;
		{
			/* check the tail for the correct page */
#if ((EURASIA_PARAM_PAGENUMBER_BITS == 16) || \
	((EURASIA_PARAM_PAGENUMBER_BITS == 14) && defined(SGX_FEATURE_UNPACKED_DPM_STATE)))
			and		r4, r4, ~#EURASIA_PARAM_DPM_STATE_MT_TAIL_CLRMSK;
#else
			/* extract bottom 4-bits */
			and		r3, r3, #EURASIA_PARAM_DPM_STATE_MT_TAIL_0_MASK;
			shr		r3, r3, #EURASIA_PARAM_DPM_STATE_MT_TAIL_0_SHIFT;
			/* extract the top 10-bits of the reference tail */
			and		r4, r4, #EURASIA_PARAM_DPM_STATE_MT_TAIL_1_MASK;
			shl		r4, r4, #EURASIA_PARAM_DPM_STATE_MT_TAIL_1_ALIGNSHIFT;
			or		r4, r3, r4;	// combine the values to make tail
#endif
			xor.testnz	p2, r4, r2;
			p2	ba	FMTEP_IncorrectTail;
			{
				MK_TRACE(MKTC_MTE_PAGE_FOUND, MKTF_SPM | MKTF_HW);
				MK_TRACE_REG(r1, MKTF_SPM | MKTF_HW);
				/* we have found the rgn so record it */
				MK_MEM_ACCESS_BPCACHE(staw)	[R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16LastMTEPageRgn)], r1;
				idf		drc0, st;
				ba		FMTEP_PageFound;
			}
			FMTEP_IncorrectTail:
		}
		FMTEP_ZeroCount:
		iaddu32		r1, r1.low, #1;
		xor.testnz	p2, r1, #17;
		p2	MK_MEM_ACCESS_BPCACHE(ldad.f2)		r3, [r5, #0++], drc0;
		p2	ba	FMTEP_CheckNextRgn;
	}
	FMTEP_PageFound:

	/* wait for store to finish */
	wdf		drc0;

	SWITCHEDMBACK();

	/* return */
	lapc;
}
/*****************************************************************************
 MoveMTEPageToTAState routine - deals with the removing the last page
 			allocated to the MTE from the stored state and placing it back onto
 			the cleared state for the TA.

 inputs: none
 returns:	r1 - TRUE = State has already been loaded,
 				 FALSE = Store/Clear TA and Load 3D in AbortComplete
 temps:		r1, r2, r3, r4, r5, r6, r7, r8, r16
 Preds:		p1, p2
*****************************************************************************/
MoveMTEPageToTAState:
{
	MK_TRACE(MKTC_MOVE_MTE_PAGE_TO_TA_STATE, MKTF_SPM | MKTF_SCH);

	/* if we are going to do a global partial render then there is no need to copy state table */
	MK_LOAD_STATE(r1, ui32ITAFlags, drc0);
	MK_MEM_ACCESS_BPCACHE(ldaw)	r2, [R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16LastMTEPageRgn)], drc1;
	/* are we doing abortall */
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testnz	p1, r1, #TA_IFLAGS_ABORTALL;
	p1	ba	MPTTA_ModsRequired;
	{
		/* check if the MTE page is in the Rgn that was aborted */
		MK_MEM_ACCESS_BPCACHE(ldad) r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32AbortedMacroTile)], drc0;
		wdf		drc1;
		wdf		drc0;
		/* if r1 == r2 the MTE page is in RGN aborted */
		xor.testnz	p2, r1, r2;
		p2 mov		r1, #USE_FALSE;
		p2	ba	MPTTA_ExitNoMods;
	}
	MPTTA_ModsRequired:

	/* Store the TA context as we need to modify it */
	MK_MEM_ACCESS_BPCACHE(ldad)	r4, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], drc0;

	/* LS bit is already the same as the TA context */
	/* Find out if we can do just a partial store */
	p1	mov		r3, #0;
	!p1	mov		r3, #EUR_CR_DPM_LSS_PARTIAL_CONTEXT_OPERATION_MASK;
	str		#EUR_CR_DPM_LSS_PARTIAL_CONTEXT >> 2, r3;

	wdf		drc0;
	MK_MEM_ACCESS(ldad)	r8, [r4, #DOFFSET(SGXMKIF_HWRTDATA.sContextStateDevAddr)], drc0;
	ldr		r4, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc1;
	wdf		drc1;
	and		r4, r4, #EUR_CR_DPM_STATE_CONTEXT_ID_LS_MASK;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(ldaw)	r3, [R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16LastMTEPageAllocation)], drc1;

	SETUPSTATETABLEBASE(MPTTA, p1, r8, r4);
	PVRSRV_SGXUTILS_CALL(DPMStateStore);

	/* Make sure the MTEPage number load has completed */
	wdf		drc1;
	/* Let's get the state table ready for the 3D first */
	/* We need to:
		1) remove the MTE pages from the page list
		2) decrement the count for the rgn
	*/
	SWITCHEDMTOTA();

	/*
		r1 - EUR_CR_DPM_TA_ALLOC_PAGE_TABLE_BASE
		r2 - MTE rgn
		r3 - MTE page number
		r4 - Addr of MTE Page in EVM table
		r5 - MTE last page entry
	*/
	/* first lets remve the MTE page from the list */
	/* calculate the address of the page in the table */
	ldr		r1, #EUR_CR_DPM_TA_ALLOC_PAGE_TABLE_BASE >> 2, drc1;
	wdf		drc1;
	/* find out the MTE page's address */
	imae	r4, r3.low, #4, r1, u32;
	/* read the data */
	MK_MEM_ACCESS_BPCACHE(ldad)	r5, [r4], drc0;
	wdf		drc0;
	/* Make sure the page is in a list on its own,
		do this now to rather than later */
	mov		r6, #(EURASIA_PARAM_DPM_PAGE_LIST_PREV_TERM_MSK | EURASIA_PARAM_DPM_PAGE_LIST_NEXT_TERM_MSK);
	MK_MEM_ACCESS_BPCACHE(stad)	[r4], r6;

	/*
		r1 - EUR_CR_DPM_TA_ALLOC_PAGE_TABLE_BASE
		r2 - MTE rgn
		r3 - MTE page number
		r4 - MTE page PREV
		r5 - MTE page NEXT
		r6 - MTE page Prev Addr
		r7 - tmp
	*/
	/* Start re-linking the list the MTE page was in */
	and		r4, r5, ~#EURASIA_PARAM_DPM_PAGE_LIST_PREV_CLRMSK;
	shr		r4, r4, #EURASIA_PARAM_DPM_PAGE_LIST_PREV_SHIFT;
	and		r5, r5, ~#EURASIA_PARAM_DPM_PAGE_LIST_NEXT_CLRMSK;
	MK_TRACE_REG(r4, MKTF_SPM);
	MK_TRACE_REG(r5, MKTF_SPM);
	mov		r6, #EURASIA_PARAM_DPM_PAGE_LIST_NEXT_TERM_MSK;
	xor.testz	p1, r4, r6;
	p1	ba	MPTTA_MTEPageHead;
	{
		/* find out the MTE page's prev page address */
		imae	r6, r4.low, #4, r1, u32;
		/* read the prev page entry */
		MK_MEM_ACCESS_BPCACHE(ldad)	r7, [r6], drc0;
		/* wait for the prev page entry load */
		wdf		drc0;
		/* clear the next */
		and		r7, r7, ~#EURASIA_PARAM_DPM_PAGE_LIST_PREV_CLRMSK;
		/* set the next value */
		or		r7, r7, r5;
		/* write the modified entry back out */
		MK_MEM_ACCESS_BPCACHE(stad)	[r6], r7;
		idf	drc1, st;
	}
	MPTTA_MTEPageHead:

	/* Decrement the count, and change head if required */
	/* Find out the address of the Rgn */
	imae	r8, r2.low, #8, r8, u32;
	MK_MEM_ACCESS_BPCACHE(ldad)	r6, [r8], drc0;
	wdf		drc0;

	/* if p1 == TRUE
		don't forget to change the head!!!! */
	/* clear old head and set to MTE page next */
	p1	and		r6, r6, #EURASIA_PARAM_DPM_STATE_MT_HEAD_CLRMSK;
	p1	or		r6, r6, r5;

	/* now decrement the count, if count == 0, DPM will ignore head and tail values */
	and		r2, r6, ~#EURASIA_PARAM_DPM_STATE_MT_COUNT_CLRMSK;
	and		r6, r6, #EURASIA_PARAM_DPM_STATE_MT_COUNT_CLRMSK;
	shr		r2, r2, #EURASIA_PARAM_DPM_STATE_MT_COUNT_SHIFT;
	isub16	r2, r2, #1;
	shl		r2, r2, #EURASIA_PARAM_DPM_STATE_MT_COUNT_SHIFT;
	and		r2, r2, ~#EURASIA_PARAM_DPM_STATE_MT_COUNT_CLRMSK;	// mask the unwanted bits
	or		r6, r6, r2;
	MK_MEM_ACCESS_BPCACHE(stad)	[r8], r6;
	idf		drc0, st;
	wdf		drc0;

	/*
		r1 - EUR_CR_DPM_TA_ALLOC_PAGE_TABLE_BASE
		r3 - MTE page number
		r4 - MTE page PREV
		r5 - MTE page NEXT
		r6 - MTE page next Addr
		r7 - tmp
	*/
	mov		r6, #EURASIA_PARAM_DPM_PAGE_LIST_NEXT_TERM_MSK;
	xor.testz	p1, r5, r6;
	p1	ba	MPTTA_MTEPageTail;
	{
		/* find out the MTE page's next page address */
		imae	r6, r5.low, #4, r1, u32;
		/* read the next page entry */
		MK_MEM_ACCESS_BPCACHE(ldad)	r5, [r6], drc0;
		/* wait for the next page entry load */
		wdf		drc0;
		/* clear the previous */
		and		r5, r5, ~#EURASIA_PARAM_DPM_PAGE_LIST_NEXT_CLRMSK;
		/* set the prev value */
		shl		r7, r4, #EURASIA_PARAM_DPM_PAGE_LIST_PREV_SHIFT;
		or		r5, r5, r7;
		/* write the modified entry back out */
		MK_MEM_ACCESS_BPCACHE(stad)	[r6], r5;
		idf	drc1, st;
	}
	MPTTA_MTEPageTail:

	/* if p1 == TRUE
		don't forget to change the tail!!!! */
	!p1	ba	MPTTA_TailEntryOK;
	{
		/* clear old tail and set to MTE previous */
#if ((EURASIA_PARAM_PAGENUMBER_BITS == 16) || \
	((EURASIA_PARAM_PAGENUMBER_BITS == 14) && defined(SGX_FEATURE_UNPACKED_DPM_STATE)))
		MK_MEM_ACCESS_BPCACHE(ldad)	r2, [r8, #1], drc0;
		wdf		drc0;
		/* clear old tail and set to MTE previous */
		and		r2, r2, #EURASIA_PARAM_DPM_STATE_MT_TAIL_CLRMSK;
		or		r2, r2, r4;
		MK_MEM_ACCESS_BPCACHE(stad)	[r8, #1], r2;
#else
		MK_MEM_ACCESS_BPCACHE(ldad)	r1, [r8, #0], drc0;
		MK_MEM_ACCESS_BPCACHE(ldad)	r2, [r8, #1], drc0;
		wdf		drc0;
		/* clear and set bottom 4-bits of tail */
		and		r1, r1, ~#EURASIA_PARAM_DPM_STATE_MT_TAIL_0_MASK;
		shl		r5, r4, #EURASIA_PARAM_DPM_STATE_MT_TAIL_0_SHIFT;
		or		r1, r1, r5;
		MK_MEM_ACCESS_BPCACHE(stad)	[r8, #0], r1;
		/* clear and set top 10 bits tail */
		and		r2, r2, ~#EURASIA_PARAM_DPM_STATE_MT_TAIL_1_MASK;
		shr		r5, r4, #EURASIA_PARAM_DPM_STATE_MT_TAIL_1_ALIGNSHIFT;
		or		r2, r2, r5;
		MK_MEM_ACCESS_BPCACHE(stad)	[r8, #1], r2;
#endif
		idf		drc0, st;
		wdf		drc0;
	}
	MPTTA_TailEntryOK:

	/* All mods made for 3D context so load it */
	/*
		Set the LS context ID to the same as the 3D context ID.
	*/
	ldr		r1, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc0;
	wdf		drc0;
	mov		r5, r1;
	xor		r1, r1, #EUR_CR_DPM_STATE_CONTEXT_ID_LS_MASK;
	str		#EUR_CR_DPM_STATE_CONTEXT_ID >> 2, r1;

	/* Load the context */
	PVRSRV_SGXUTILS_CALL(DPMStateLoad);

	/*
		Before starting the clear and load flip the LS bit
	*/
	str		#EUR_CR_DPM_STATE_CONTEXT_ID >> 2, r5;

	/* check if we have done a global abort */
	ldr		r1, #EUR_CR_DPM_LSS_PARTIAL_CONTEXT >> 2, drc0;
	wdf		drc0;
	and.testnz	p1, r1, r1;
	p1	ba	MPTTA_NoTAStateClear;
	{
		/* Do the state store */
		PVRSRV_SGXUTILS_CALL(DPMStateClear);
		/* Do the state store */
		PVRSRV_SGXUTILS_CALL(DPMStateStore);
	}
	MPTTA_NoTAStateClear:

	/* Now sort out the state table for the TA */
	/* Put the MTE page back on the correct Rgn state */
	/* Make the changes to the state table */
	mov		r4, r3;	// set the head to the MTE page
	or		r4, r4, #(1 << EURASIA_PARAM_DPM_STATE_MT_COUNT_SHIFT);	// set page count to 1
	/* now sort out the tail */
#if ((EURASIA_PARAM_PAGENUMBER_BITS == 16) || \
	((EURASIA_PARAM_PAGENUMBER_BITS == 14) && defined(SGX_FEATURE_UNPACKED_DPM_STATE)))
	mov		r5, r3;		// set the tail to the MTE page;
#else
	shl		r1, r3, #EURASIA_PARAM_DPM_STATE_MT_TAIL_0_SHIFT;	// get the bottom 4-bits in position;
	or		r4, r4, r1;	// set the tail;
	shr		r5, r3, #EURASIA_PARAM_DPM_STATE_MT_TAIL_1_ALIGNSHIFT;	// set the top 10 bits;
#endif

	/* write the values into memory */
	MK_MEM_ACCESS_BPCACHE(stad.f2)	[r8, #0++], r4;
	idf		drc1, st;

	/* Start the load of the state onto the TA context */
	wdf		drc1;

	SWITCHEDMBACK();

	/* Load the context */
	PVRSRV_SGXUTILS_CALL(DPMStateLoad);

	/* reset this */
	str		#EUR_CR_DPM_LSS_PARTIAL_CONTEXT >> 2, #0;

	/* we have corrected the state for the TA and 3D so return TRUE and exit */
	mov	r1, #USE_TRUE;
MPTTA_ExitNoMods:
	MK_TRACE(MKTC_MOVE_MTE_PAGE_TO_TA_STATE_END, MKTF_SPM | MKTF_SCH);

	/* Return */
	lapc;
}

/*****************************************************************************
 RemoveReserveMemory routine	- Reset the ZLS threshold back to the saved value.

	FIXME: move this function to sgx_utils.asm some how so it not duplicated.
 inputs:
 temps:
*****************************************************************************/
RemoveReserveMemory:
{
	MK_TRACE(MKTC_REMOVE_RESERVE_MEM, MKTF_SPM | MKTF_SCH);

	/*
		Store the maximum threshold increment
	*/
	/* Reduce the threshold by the number of page inserted for the workaround. */
	MK_MEM_ACCESS_BPCACHE(ldaw)	r0, [R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16Num23055PagesUsed)], drc0;
	ldr		r1, #EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, drc0;
	wdf		drc0;
	isub16	r1, r1, r0;
	str		#EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, r1;

	/* Reload the free list to make the hardware use the new threshold */
	PVRSRV_SGXUTILS_CALL(DPMReloadFreeList);

#if defined(DEBUG)
	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32MaximumZLSThresholdIncrement)], drc0;
	wdf		drc0;
	isub16.tests	p1, r1, r0;
	p1	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32MaximumZLSThresholdIncrement)], r0;
#endif /* defined(DEBUG) */

	MK_MEM_ACCESS_BPCACHE(staw)	[R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16Num23055PagesUsed)], #0;

	/* return */
	lapc;
}

/*****************************************************************************
 ZeroZLSThreshold routine - Sets the ZLS threshold to 0.

 inputs:
 temps: r1
*****************************************************************************/
ZeroZLSThreshold:
{
	MK_TRACE(MKTC_ZERO_ZLS_THRESHOLD, MKTF_SPM | MKTF_HW);

	/*
	 Store the maximum threshold increment
	*/
	 /* Reduce the threshold by the number of page inserted for the workaround. */
	ldr	r1, #EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, drc0;
	wdf	drc0;
	str	#EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, #0;

	MK_MEM_ACCESS_BPCACHE(staw)	[R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16SavedZLSThreshold)], r1;

	/* Reload the free list to make the hardware use the new threshold */
	PVRSRV_SGXUTILS_CALL(DPMReloadFreeList);

	/* return */
	lapc;
}

/*****************************************************************************
 RestoreZLSThreshold routine - Resets the ZLS threshold back to the value
                                                                saved off by ZeroZLSThreshold

 inputs:
 temps: r1
*****************************************************************************/
RestoreZLSThreshold:
{
	MK_TRACE(MKTC_RESTORE_ZLS_THRESHOLD, MKTF_SPM | MKTF_HW);

	/* Reduce the threshold by the number of page inserted for the workaround. */
	MK_MEM_ACCESS_BPCACHE(ldaw)	r1, [R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16SavedZLSThreshold)], drc0;
	wdf	drc0;
	str	#EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, r1;

	/* Reload the free list to make the hardware use the new threshold */
	PVRSRV_SGXUTILS_CALL(DPMReloadFreeList);

	/* return */
	lapc;
}
#endif /* defined(FIX_HW_BRN_23055) */


/*****************************************************************************
 SPMRenderFinished routine	- process the render finished event

 inputs:
 temps:
*****************************************************************************/
SPMRenderFinished:
{
	MK_LOAD_STATE(r10, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	mov		r11,#~(RENDER_IFLAGS_SPMRENDER);
	and		r10, r10, r11;

	MK_STORE_STATE(ui32IRenderFlags, r10);
	MK_WAIT_FOR_STATE_STORE(drc0);

#if defined(EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC)
#if !defined(FIX_HW_BRN_31109)
	/* Re-enable proactive PIM spec */
	READMASTERCONFIG(r16, #EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, drc0);
	wdf		drc0;
	and		r16, r16, ~#EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC_EN_N_MASK;
	WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, r16, r17);
#endif
#endif
	/* Load the TA flag for use in all of the following code. */
	MK_LOAD_STATE(r10, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

#if defined(FIX_HW_BRN_31109)
	/* clear the flag, so we don't get a false positive in the future */
	and	r10, r10, ~#TA_IFLAGS_TIMER_DETECTED_SPM;
#endif
		
	/* Clear the abort complete flag as the render has finished */
	and		r10, r10, ~#(TA_IFLAGS_OUTOFMEM | TA_IFLAGS_ABORT_COMPLETE);
	MK_STORE_STATE(ui32ITAFlags, r10);
	MK_WAIT_FOR_STATE_STORE(drc1);
	
#if defined(FIX_HW_BRN_23055)
	/* Check if we need to reset the TA thresholds. */
	shl.testns	p0, r10, #(31 - TA_IFLAGS_INCREASEDTHRESHOLDS_BIT);
	p0	ba	SPMRF_NoThresholdIncrease;
	{
		/* Bring the Threshold back down */
		bal		RemoveReserveMemory;
		and		r10, r10, ~#TA_IFLAGS_INCREASEDTHRESHOLDS;

		/* If deadlock_mem is set, don't reset complete on terminate */
		shl.tests	p0, r10, #(31 - TA_IFLAGS_SPM_DEADLOCK_MEM_BIT);
		p0	ba	SPMRF_NoTE_PSGReset;
		{
			MK_TRACE(MKTC_RESET_TE_PSG, MKTF_SPM);
			/* We have got the reserve memory back so no longer need to override completeonterminate */
			MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTACCBCmd)], drc0;
			wdf		drc0;
			mov		r2, #OFFSET(SGXMKIF_CMDTA.sTARegs.ui32TEPSG);
			iaddu32	r1, r2.low, r1;
			MK_MEM_ACCESS_CACHED(ldad)	r2, [r1], drc0;
			wdf		drc0;
			str		#EUR_CR_TE_PSG >> 2, r2;
		}
		SPMRF_NoTE_PSGReset:

		/* Check if the timer did the abort due to an IDLE? */	
		shl.testns	p1, r10, #(31 - TA_IFLAGS_IDLE_ABORT_BIT);
		p1	ba	SPMRF_NotIdleAbort;
		{
			and 	r10, r10, ~#TA_IFLAGS_IDLE_ABORT;
			
			/* We both denied at some point? */
			shl.tests	p0, r10, #(31 - TA_IFLAGS_BOTH_REQ_DENIED_BIT);
			p0	ba	SPMRF_BothReqDenied;
			{
				/*
				 * The MTE was NOT denied between TA Start/Restart and abort.
				 * The timer issued an abort due to the TA becoming IDLE,
				 * The MTE might have a page, which does not
				 * get reissued and is in the free list after 3D. Marking flag for software re-issue.
				 */
				or 	r10, r10, #TA_IFLAGS_FIND_MTE_PAGE;
				/* Set the threshold to 0, so OOM generated straight away. The CSM
				   Is not gaurenteed to be up-to-date, so we have to generate a OOM, to get the CSM
				   updated, before doing findMTEPageInCSM and ReissueMTEPage */
				bal 	ZeroZLSThreshold;
	
				ba	SPMRF_NoDeadlockCheck;
			}
			SPMRF_BothReqDenied:
		}
		SPMRF_NotIdleAbort:
		/* If both were denied at some point, clear the flag */
		shl.tests	p0, r10, #(31 - TA_IFLAGS_BOTH_REQ_DENIED_BIT);
		p0	and		r10, r10, ~#TA_IFLAGS_BOTH_REQ_DENIED;
	
	}
	SPMRF_NoThresholdIncrease:
#endif /* defined(FIX_HW_BRN_23055) */

#if 1 // SPM_PAGE_INJECTION;
	/* Check if we need to reset the threshold */
	MK_LOAD_STATE(r1, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	shl.tests		p1, r1, #31 - RENDER_IFLAGS_SPM_PB_FREE_RENDER_BIT;
	!p1	ba	SPMRF_NotDeadlockRender;
	{
		/* Clear the flag from the IRenderFlags */
		and		r1, r1, ~#RENDER_IFLAGS_SPM_PB_FREE_RENDER;
		MK_STORE_STATE(ui32IRenderFlags, r1);

		ba	SPMRF_DeadlockCheckComplete;
	}
	SPMRF_NotDeadlockRender:
	{
		/* Before we re-enable the VDM check if we have freed any pages */
	#if defined(FIX_HW_BRN_23055)
		/* Before we re-enable the VDM check if we have freed enough pages to go below the threshold */
		ldr		r2, #EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, drc0;
		ldr		r3, #EUR_CR_DPM_PAGE_STATUS >> 2, drc0;
		wdf		drc0;
		and		r3, r3, #EUR_CR_DPM_PAGE_STATUS_TA_MASK;
		shr		r3, r3, #EUR_CR_DPM_PAGE_STATUS_TA_SHIFT;
		isub16.testns	p0, r3, r2; // EUR_CR_DPM_PAGE_STATUS - EUR_CR_DPM_ZLS_PAGE_THRESHOLD >= 0
		/* if !p0 (pages allocated >= threshold) set deadlock bit */
	#else	
		/* Make sure the TA page count has dropped by more than 2 pages */
		MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32AbortPages)], drc0;
		
		ldr		r3, #EUR_CR_DPM_PAGE_STATUS >> 2, drc0;
		wdf		drc0;
		and		r3, r3, #EUR_CR_DPM_PAGE_STATUS_TA_MASK;
		shr		r3, r3, #EUR_CR_DPM_PAGE_STATUS_TA_SHIFT;
		isub16.tests	p0, r2, r3; // (OLD-2) - (NEW) < 0 = deadlock
		p0	ba	SPMRF_TAPageDeadlock;
		{
			/* Was it a global abort? */
			and.testz	p0, r10, #TA_IFLAGS_ABORTALL;
			p0	ba	SPMRF_DeadlockCheckComplete;
			{
				/* This was a global abort, therefore check for a global list deadlock */
				MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32GblAbortPages)], drc0;
			
				ldr		r3, #EUR_CR_DPM_GLOBAL_PAGE_STATUS >> 2, drc0;
				wdf		drc0;
				and		r3, r3, #EUR_CR_DPM_GLOBAL_PAGE_STATUS_TA_MASK;
				shr		r3, r3, #EUR_CR_DPM_GLOBAL_PAGE_STATUS_TA_SHIFT;
				isub16.tests	p0, r2, r3; // (OLD-2) - (NEW) < 0 = deadlock
				p0	or	r10, r10, #TA_IFLAGS_SPM_DEADLOCK_GLOBAL;
			}
		}
		SPMRF_TAPageDeadlock:
	#endif
		p0	or	r10, r10, #TA_IFLAGS_SPM_DEADLOCK;
	}
	SPMRF_DeadlockCheckComplete:
	SPMRF_NoDeadlockCheck:
#endif

#if defined(FIX_HW_BRN_27311) && !defined(FIX_HW_BRN_23055)
	MK_MEM_ACCESS_BPCACHE(ldad)	R_RTData, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS(ldaw) r1, [R_RTData, #WOFFSET(SGXMKIF_HWRTDATA.ui16SPMRenderMask)], drc1;
	MK_MEM_ACCESS(ldaw)	r2, [R_RTData, #WOFFSET(SGXMKIF_HWRTDATA.ui16BRN27311Mask)], drc1;
	wdf		drc1;
	/* Check to see if all the MTs have been rendered and the bias needs to be reset */
	xor.testnz	p0, r1, r2;
	p0	ba	SPMRF_NotAllMTsRendered;
	{
		MK_TRACE(MKTC_BRN27311_RESET, MKTF_SPM | MKTF_SCH);

		/* We have rendered all the MTs so reset and start again */
		ldr		r1, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc1;
		wdf		drc0;
		MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sContextStateDevAddr)], drc0;
		wdf		drc1;
		and		r3, r1, #EUR_CR_DPM_STATE_CONTEXT_ID_LS_MASK;

		/* wait for the addr to load */
		wdf				drc0;

		/* Store the TA context. */
		SETUPSTATETABLEBASE(SRF_BRN27311_STA, p0, r2, r3);
		/* Do the state store */
		PVRSRV_SGXUTILS_CALL(DPMStateStore);

		/* Add the ZLSCount */
		mov		r16, R_RTData;
		MK_MEM_ACCESS_BPCACHE(ldad)	r17, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderContext)], drc0;
		wdf		drc0;
		MK_MEM_ACCESS(ldad)	r17, [r17, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPBDescDevVAddr.uiAddr)], drc0;
		wdf		drc0;
		PVRSRV_SGXUTILS_CALL(ResetBRN27311StateTable);

		/* Reload the modified table */
		PVRSRV_SGXUTILS_CALL(DPMStateLoad);
	}
	SPMRF_NotAllMTsRendered:
#endif

	/*
		We have finished the render now so, re-enable the
		vertex data master so the TA can continue when we restart.
	*/
	MK_TRACE(MKTC_RESUMEVDM, MKTF_SPM | MKTF_SCH);
	ldr				r2, #EUR_CR_DMS_CTRL >> 2, drc1;
	wdf				drc1;
	and				r2, r2, ~#EUR_CR_DMS_CTRL_DISABLE_DM_VERTEX_MASK;
	str				#EUR_CR_DMS_CTRL >> 2, r2;

	/* Check if we rendered all MTs. */
	and.testnz		p0, r10, #TA_IFLAGS_ABORTALL;
	p0	ba				SRF_AbortAll;
	{
		/* Do DPM context operations only for the aborted macrotile */
		str		#EUR_CR_DPM_LSS_PARTIAL_CONTEXT >> 2, #EUR_CR_DPM_LSS_PARTIAL_CONTEXT_OPERATION_MASK;
	}
	SRF_AbortAll:


	/* Reset the task control for the aborted macrotile. */
#if defined(SGX_FEATURE_MP)
	#if defined(EUR_CR_MASTER_DPM_MTILE_ABORTED) && defined(EUR_CR_DPM_MTILE_ABORTED)
	READMASTERCONFIG(r3, #EUR_CR_MASTER_DPM_MTILE_ABORTED >> 2, drc0);
	wdf		drc0;
	str		#EUR_CR_DPM_MTILE_ABORTED >> 2, r3;
	#endif

	MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32AbortedMacroTile)], drc0;
	wdf	drc0;
	/* and with its mask */
	and	r3, r3, #EUR_CR_MASTER_DPM_ABORT_STATUS_MTILE_CORE_MASK;
	/* shift to base address position */
	shr	r3, r3, #EUR_CR_MASTER_DPM_ABORT_STATUS_MTILE_CORE_SHIFT;
	WRITECORECONFIG(r3, #EUR_CR_DPM_TASK_CONTROL >> 2, #EUR_CR_DPM_TASK_CONTROL_CLEAR_MASK, r1);
#else
	str		#EUR_CR_DPM_TASK_CONTROL >> 2, #EUR_CR_DPM_TASK_CONTROL_CLEAR_MASK;
#endif /* if defined (SGX_FEATURE_MP) */

	/* Clear partial render bit */
	str		#EUR_CR_DPM_PARTIAL_RENDER >> 2, #0;

	mov		r0, #EUR_CR_EVENT_STATUS_DPM_CONTROL_CLEAR_MASK;

	/* Wait for the reset to finish. */
	ENTER_UNMATCHABLE_BLOCK;
	SRF_WaitForContextReset:
	{

#if defined(SGX_FEATURE_MP)
		READCORECONFIG(r1, r3, #EUR_CR_EVENT_STATUS >> 2, drc0);
#else
		ldr		r1, #EUR_CR_EVENT_STATUS >> 2, drc0;
#endif /* if defined (SGX_FEATURE_MP) */
		wdf		drc0;
		and		r1, r1, r0;
		xor.testnz	p0, r1, r0;
		p0	ba		SRF_WaitForContextReset;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the reset event. */
	mov		r0, #EUR_CR_EVENT_HOST_CLEAR_DPM_CONTROL_CLEAR_MASK;
#if defined(SGX_FEATURE_MP)
	WRITECORECONFIG(r3, #EUR_CR_EVENT_HOST_CLEAR >> 2, r0, r2);
#else
	str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r0;
#endif /*if defined(SGX_FEATURE_MP)*/

	str		#EUR_CR_DPM_LSS_PARTIAL_CONTEXT >> 2, #0;

	MK_MEM_ACCESS_BPCACHE(ldad)	R_RTData, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRTData)], drc0;
	wdf		drc0;

#if (defined(FIX_HW_BRN_30089) || defined(FIX_HW_BRN_29490)) && defined(SGX_FEATURE_MP)
	/* Switch to the application address space */
	SWITCHEDMTO3D();
#endif

#if defined(FIX_HW_BRN_30089) && defined(SGX_FEATURE_MP)
	/* if we are not rendering all MTs load the idx for the 1 we are rendering */
	and.testnz		p0, r10, #TA_IFLAGS_ABORTALL;
	!p0	MK_MEM_ACCESS_BPCACHE(ldad) 	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32AbortedMacroTile)], drc0;
	p0	mov	r1, #EUR_CR_DPM_ABORT_STATUS_MTILE_GLOBAL_MASK;
	/* only waits if there is a load pending */
	wdf		drc0;
	/* Modify the region headers as required */
 	and		r1, r1, #EUR_CR_DPM_ABORT_STATUS_MTILE_GLOBAL_MASK | EUR_CR_DPM_ABORT_STATUS_MTILE_INDEX_MASK;
	RESTORERGNHDR(R_RTData, r1);
#endif

#if defined(FIX_HW_BRN_29490) && defined(SGX_FEATURE_MP)
	/* Before issuing a restart we may need to modify the DPList */
	and.testz		p0, r10, #TA_IFLAGS_ABORTALL;
	p0	ba	SPMRF_NoDPListFixRequired;
	{
		/* we did an abort, was that what the HW requested */
		MK_MEM_ACCESS_BPCACHE(ldad) 	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32AbortedMacroTile)], drc0;
		wdf		drc0;
		and.testnz	p0, r1, #EUR_CR_DPM_ABORT_STATUS_MTILE_GLOBAL_MASK;
		p0	ba	SPMRF_NoDPListFixRequired;
		{
			/* We must have issued an abortall when the HW indicated out_of_mt */
			/* dump the dplist (parti pim table) */
			MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sPIMTableDevAddr)], drc0; 
			wdf		drc0;
			WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_MTILE_PARTI_PIM_TABLE_BASE_ADDR >> 2, r2, r4);
			WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_TASK_DPLIST >> 2, #EUR_CR_MASTER_DPM_TASK_DPLIST_STORE_MASK, r4);
			
			/* Poll for the DPM PIM stored event. */
			ENTER_UNMATCHABLE_BLOCK;
			SPMRF_WaitForDPListStore:
			{
				READMASTERCONFIG(r4, #EUR_CR_MASTER_DPM_DPLIST_EVENT_STATUS >> 2, drc0);
				wdf			drc0;
				and.testz	p0, r4, #EUR_CR_MASTER_DPM_DPLIST_EVENT_STATUS_STORED_MASK;
				p0	ba		SPMRF_WaitForDPListStore;
			}
			LEAVE_UNMATCHABLE_BLOCK;		
			
			/* Clear the event bit */
			WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_DPLIST_CLEAR_EVENT >> 2, #EUR_CR_MASTER_DPM_DPLIST_CLEAR_EVENT_STORE_MASK, r4);
			
			/* Find out the values which should be written */
			READMASTERCONFIG(r4, #EUR_CR_MASTER_DPM_DPLIST_STATUS >> 2, drc0);
			wdf		drc0;
			and		r4, r4, #EUR_CR_MASTER_DPM_DPLIST_STATUS_PIM_MASK;
			or		r4, r4, #(1 << 31);
			
			/* change the smlsi so r4 is always referenced */
			smlsi	#0, #0, #0, #0, #0, #0, #0, #0, #0, #0, #0;
			
			/* Modify the values in memory */
			MK_MEM_ACCESS_BPCACHE(stad.f16)	[r2, #0++], r4;
			MK_MEM_ACCESS_BPCACHE(stad)	[r2, #1++], r4;
			idf		drc0, st;
			wdf		drc0;
			
			/* restore the smlsi */
			smlsi	#0, #0, #0, #1, #0, #0, #0, #0, #0, #0, #0;
			
			/* Reload the modified DPList Table */ 
			WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_TASK_DPLIST >> 2, #EUR_CR_MASTER_DPM_TASK_DPLIST_LOAD_MASK, r4);
			
			/* Poll for the DPM PIM loaded event. */
			ENTER_UNMATCHABLE_BLOCK;
			SPMRF_WaitForDPListLoad:
			{
				READMASTERCONFIG(r4, #EUR_CR_MASTER_DPM_DPLIST_EVENT_STATUS >> 2, drc0);
				wdf			drc0;
				and.testz	p0, r4, #EUR_CR_MASTER_DPM_DPLIST_EVENT_STATUS_LOADED_MASK;
				p0	ba		SPMRF_WaitForDPListLoad;
			}
			LEAVE_UNMATCHABLE_BLOCK;		
			
			/* Clear the event bit */
			WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_DPLIST_CLEAR_EVENT >> 2, #EUR_CR_MASTER_DPM_DPLIST_CLEAR_EVENT_LOAD_MASK, r4);
		}		
	}
	SPMRF_NoDPListFixRequired:
#endif

#if (defined(FIX_HW_BRN_30089) || defined(FIX_HW_BRN_29490)) && defined(SGX_FEATURE_MP)
	/* Switch back to the ukernel address space */
	SWITCHEDMBACK();
#endif


#if 1 // SPM_PAGE_INJECTION;
	!p1	ba	SPMRF_CheckRestart;
	{
		//FIXME MK_TRACE(0xDEAD2, MKTF_SPM | MKTF_SCH);
		/* This was a deadlock render, so issue TAFinished loopback */
		mov		r16, #USE_LOOPBACK_TAFINISHED;
		PVRSRV_SGXUTILS_CALL(EmitLoopBack);
		ba	SPMRF_NoRestartRequired;
	}
	SPMRF_CheckRestart:
#endif
	{
		{
			MK_TRACE(MKTC_RESTARTTA, MKTF_SPM | MKTF_SCH | MKTF_HW);
#if defined(SGX_FEATURE_MP)
			WRITECORECONFIG(r3, #EUR_CR_DPM_OUTOFMEM >> 2, #EUR_CR_DPM_OUTOFMEM_RESTART_MASK, r2);
			/* issue abort to master */
			str		#EUR_CR_DPM_OUTOFMEM >> 2, #EUR_CR_DPM_OUTOFMEM_RESTART_MASK;
#else
			str		#EUR_CR_DPM_OUTOFMEM >> 2, #EUR_CR_DPM_OUTOFMEM_RESTART_MASK;
#endif /* if defined (SGX_FEATURE_MP) */
		}
	}
	SPMRF_NoRestartRequired:

	/* Indicate there has been a SPM render, this is used later */
	MK_MEM_ACCESS(ldad)	r0, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sRTStatusDevAddr)], drc0;
	wdf		drc0;
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	MK_MEM_ACCESS_BPCACHE(ldad) 	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32AbortedRT)], drc0;
	wdf		drc0;
	imae	r0, r1.low, #SIZEOF(IMG_UINT32), r0, u32;
#endif
	MK_MEM_ACCESS(ldad)	r2, [r0], drc0;
	wdf		drc0;
	or		r2, r2, #SGXMKIF_HWRTDATA_RT_STATUS_SPMRENDER;
	MK_MEM_ACCESS(stad)	[r0], r2;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRTData)], #0;
	idf		drc0, st;
	wdf		drc0;

	/* Clear the abort in progress flag. */
	mov		r9, ~#(TA_IFLAGS_ABORTINPROGRESS | TA_IFLAGS_ABORTALL | TA_IFLAGS_WAITINGTOSTARTCSRENDER);
	and		r10, r10, r9;

	/* Update the TA flags after changes in this routine. */
	MK_STORE_STATE(ui32ITAFlags, r10);
	MK_WAIT_FOR_STATE_STORE(drc0);
	
	/* branch back to calling code */
	mov		pclink, R_EDM_PCLink;

	lapc;
}

/*****************************************************************************
 End of file (spm.asm)
*****************************************************************************/
