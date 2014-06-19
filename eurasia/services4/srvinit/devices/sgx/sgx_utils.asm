/*****************************************************************************
 Name		: sgx_utils.asm

 Title		: SGX microkernel utility code

 Author 	: Imagination Technologies

 Created  	: 29/08/2007

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

 Description 	: SGX Microkernel utility code

 Program Type	: USSE assembly language

 Modifications	:

 File: sgx_utils.asm 
*****************************************************************************/

/*****************************************************************************
 Temporary register usage in sgx_utils code
 -------------------------------------------

 10 temps are required, r16-r23 & r30-r31, for the utils code:
 	- 8 for temporary usage in functions
	- 1 for pclink copy for calls from utils
	- 1 for cache status

*****************************************************************************/

#include "usedefs.h"
#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
/* Pull in system specific code */
#include "extsyscache.h"
#endif

.export InvalidateBIFCache;
.export SetupDirListBase;
#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
.export SetupRequestor;
#if defined(SUPPORT_SGX_EDM_MEMORY_DEBUG)
.export SetupRequestorEDM;
#endif /* SUPPORT_SGX_EDM_MEMORY_DEBUG */
#endif /* SGX_FEATURE_MULTIPLE_MEM_CONTEXTS */
.export DPMThresholdUpdate;
.export DPMReloadFreeList;
.export IdleCore;
.export Resume;
.export LoadTAPB;
.export StoreTAPB;
.export Load3DPB;
.export Store3DPB;
.export SetupDPMPagetable;
#if defined(SGX_FAST_DPM_INIT) && (SGX_FEATURE_NUM_USE_PIPES > 1)
.export SDPT_PDSConstSize;
.export SDPT_PDSDevAddr;
#endif
.export StoreDPMContext;
.export LoadDPMContext;
.export DPMStateClear;
.export DPMStateStore;
.export DPMStateLoad;
.export EmitLoopBack;
.export ELB_PDSConstSize;
.export ELB_PDSDevAddr;

.modulealign 16;

#if defined(FIX_HW_BRN_21590)
/* FIX_HW_BRN_21590 - Code is written so Load/Store operations of the TAAC (TA Control Table)
					  and LSS (TA State Table) are NOT done simultaneously. */
#endif
#if defined(FIX_HW_BRN_23533)
/* FIX_HW_BRN_23533 - Code is written to only support SGX_SUPPORT_SPM_MODE_1 and this is
					  SPM_MODE_0 specific. */
#endif
#if defined(FIX_HW_BRN_27006)
/* FIX_HW_BRN_27006 - Code is written to only use IEEE32 format */
#endif

/*****************************************************************************
 Macro definitions
*****************************************************************************/

/*****************************************************************************
 Start of program
*****************************************************************************/

/*****************************************************************************
 InvalidateBIFCache
	Invalidate the BIF directory cache.

 inputs:	r16 - zero for PTE, non-zero for PD
 temps:		r17, r18
*****************************************************************************/
InvalidateBIFCache:
{
#if defined(DEBUG) && defined(FIX_HW_BRN_28889)
	MK_LOAD_STATE(r17, ui32ITAFlags, drc0);
	MK_LOAD_STATE(r18, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testz	p0, r17, #TA_IFLAGS_TAINPROGRESS;
	/* if the pr_block bit is set the TA is idle, so continue with 3D check */
	!p0	shl.tests	p0, r17, #(31 - TA_IFLAGS_OOM_PR_BLOCKED_BIT);
	p0	and.testz	p0, r18, #RENDER_IFLAGS_RENDERINPROGRESS;
	p0	ba	IBC_CoreIdle;
	{
		MK_ASSERT_FAIL(MKTC_IBC_ILLEGAL);
	}
	IBC_CoreIdle:
#endif
	INVALIDATE_BIF_CACHE(r16, r17, r18, SGX_Utils_IDC);
	lapc;
}

/*****************************************************************************
 CheckTAQueue
	Checks the TA queue and either emits a loopback or sets ACTIVE_POWER
	flag
 inputs:
 temps:		r16, r17, r18
*****************************************************************************/
.export CheckTAQueue;
CheckTAQueue:
{
	/* Check all the priority levels */
	mov		r16, #0;

	CTAQ_NextPriority:
	{
		MK_LOAD_STATE_INDEXED(r18, sPartialRenderContextHead, r16, drc0, r17);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and.testnz	p2, r18, r18;
		p2  mov	r16, #USE_LOOPBACK_FINDTA;
		p2	ba	EmitLoopBack;

		/* Move on to the next priority */
		iaddu32		r16, r16.low, #1;
		xor.testnz	p2, r16, #SGX_MAX_CONTEXT_PRIORITY;
		p2	ba	CTAQ_NextPriority;
	}

	/* All priorities are empty */
	mov	r16, #SGX_UKERNEL_APFLAGS_TA;
	ba	ClearActivePowerFlags;

	/* We will never get here as one of the subroutine will be called */
}

/*****************************************************************************
 Check3DQueues
	Checks the Transfer queue and either emits a loopback or sets ACTIVE_POWER
	flag
 inputs:
 temps:		r16, r17, r18, r19, r20
*****************************************************************************/
.export Check3DQueues;
Check3DQueues:
{
	/* Check all the priority levels */
	mov		r16, #0;

	C3DQ_NextPriority:
	{
		MK_LOAD_STATE_INDEXED(r19, sTransferContextHead, r16, drc0, r17);
		MK_LOAD_STATE_INDEXED(r20, sCompleteRenderContextHead, r16, drc0, r18);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		or.testnz	p2, r19, r20;
		p2  mov	r16, #USE_LOOPBACK_FIND3D;
		p2	ba	EmitLoopBack;

		/* Move on to the next priority */
		iaddu32		r16, r16.low, #1;
		xor.testnz	p2, r16, #SGX_MAX_CONTEXT_PRIORITY;
		p2	ba	C3DQ_NextPriority;
	}

	/* All priorities are empty */
	mov	r16, #SGX_UKERNEL_APFLAGS_3D;
	ba	ClearActivePowerFlags;

	/* We will never get here as one of the subroutine will be called */
}

#if defined(SGX_FEATURE_2D_HARDWARE)
/*****************************************************************************
 Check2DQueue
	Checks the 2D queue and either emits a loopback or sets ACTIVE_POWER
	flag
 inputs:
 temps:		r16, r17, r18
*****************************************************************************/
.export Check2DQueue;
Check2DQueue:
{
	/* Check all the priority levels */
	mov		r16, #0;

	C2DQ_NextPriority:
	{
		MK_LOAD_STATE_INDEXED(r18, s2DContextHead, r16, drc0, r17);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and.testnz	p2, r18, r18;
		p2  mov	r16, #USE_LOOPBACK_FIND2D;
		p2	ba	EmitLoopBack;

		/* Move on to the next priority */
		iaddu32		r16, r16.low, #1;
		xor.testnz	p2, r16, #SGX_MAX_CONTEXT_PRIORITY;
		p2	ba	C2DQ_NextPriority;
	}

	/* All priorities are empty */
	mov	r16, #SGX_UKERNEL_APFLAGS_2D;
	ba	ClearActivePowerFlags;

	/* We will never get here as one of the subroutine will be called */
}
#endif

/*****************************************************************************
 Resume
      Resume DMS
 inputs: None
 temps : r16
 predicates: p3
******************************************************************************/
Resume:
{
        /* enable all the DMs */
	MK_MEM_ACCESS_BPCACHE(ldad)	r16, [R_TA3DCtl, HSH(DOFFSET(SGXMK_TA3D_CTL.ui32IdleCoreRefCount))], drc0  ;
	wdf		drc0	;
	xor.testz	p3, r16, #0;
	p3		ba	RSM_JustResume;
	iadd32		r16, r16.low, #-1 ;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, HSH(DOFFSET(SGXMK_TA3D_CTL.ui32IdleCoreRefCount))],  r16;
	idf		drc0, st;
	wdf		drc0;
	xor.testnz	p3, r16, #0;
	p3		ba	RSM_NoResume; 
	RSM_JustResume:
#if defined(EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC)
#if !defined(FIX_HW_BRN_31109) 
	/* Re-enable proactive PIM spec */
	READMASTERCONFIG(r16, #EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, drc0);
	wdf		drc0;
	and		r16, r16, ~#EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC_EN_N_MASK;
	WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, r16, r17);
#endif
#endif
	ldr		r16, HSH(EUR_CR_DMS_CTRL) >> 2, drc0;					
	wdf		drc0;													
	and		r16, r16, HSH(~EUR_CR_DMS_CTRL_DISABLE_DM_MASK);		
	str		HSH(EUR_CR_DMS_CTRL) >> 2, r16; 
	RSM_NoResume:
	lapc;
}

/*****************************************************************************
 IdleCore
	Ensure the DPM/USSE is idle

 inputs:	r16 - 0x1 = Idle3D, 0x2= IdleTA, 0x4 = IdleUSSEOnly flag
 temps:		r17, r18, r19, r20, r21
*****************************************************************************/
IdleCore:
{
	/* Before we try to idle anything, check if it is actually busy! */
	MK_MEM_ACCESS_BPCACHE(ldad)	r17,	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32IdleCoreRefCount)], drc0;
	wdf	drc0;
	iaddu32		r17,	r17.low,	#1;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32IdleCoreRefCount)], r17;
	idf		drc0, st;
	wdf		drc0;
	IC_FromTheStart:
	/* Before we try to idle anything, check if it is actually busy! */
	and		r17, r16, #USE_IDLECORE_TA3D_REQ_MASK;
	xor.testz	p2, r17, #USE_IDLECORE_3D_REQ;
	p2 MK_LOAD_STATE(r17, ui32IRenderFlags, drc0);
	p2	ba	IC_FlagsLoaded;
	{
		MK_LOAD_STATE(r17, ui32ITAFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		/* if we have issued an abort the TA will not be allocating pages, so exit */
		and.testnz		p2, r17, #TA_IFLAGS_ABORTINPROGRESS;
		p2	ba		IC_CoreIdle;
	}
	IC_FlagsLoaded:
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testz	p2, r17, #TA_IFLAGS_TAINPROGRESS; // note TA and 3D use same bit for "busy"
	p2	ba IC_CoreIdle;
	{
		/* disable DM events on PDS */
		ldr		r17, #EUR_CR_DMS_CTRL >> 2, drc0;
		wdf		drc0;
		/* all but loopbacks */
		or		r17, r17, #(EUR_CR_DMS_CTRL_DISABLE_DM_VERTEX_MASK | \
							EUR_CR_DMS_CTRL_DISABLE_DM_PIXEL_MASK | \
							EUR_CR_DMS_CTRL_DISABLE_DM_EVENT_MASK);
		str		#EUR_CR_DMS_CTRL >> 2, r17;


#if defined(EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC)
		/* Disable proactive PIM spec */
		READMASTERCONFIG(r17, #EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, drc0);
		wdf		drc0;
		or		r17, r17, #EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC_EN_N_MASK;
		WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, r17, r18);
#endif

		IC_RecheckAll:
		{
	#if defined(FIX_HW_BRN_24549) || defined(FIX_HW_BRN_25615)
			and.testnz	p2, r16, #USE_IDLECORE_USSEONLY_REQ;
			p2	ba	IC_PollForUSSEIdle;
	#endif
			{
				/* TA or 3D */
				and		r17, r16, #USE_IDLECORE_TA3D_REQ_MASK;
				xor.testz	p2, r17, #USE_IDLECORE_3D_REQ;
				p2	ba IC_PollForISPIdle;
				{
					/* Wait for the MTE sig 1 to go idle */
					IC_PollForMTEIdle:
					{
		#if defined(EUR_CR_MTE_MEM_CHECKSUM)
						ldr			r17, #EUR_CR_MTE_MEM_CHECKSUM >> 2, drc0;
		#else
						ldr			r17, #EUR_CR_MTE_SIG1 >> 2, drc0;
		#endif
						mov			r18, #1000;
						wdf			drc0;
						IC_PauseBeforeMTERecheck:
						{
							isub16.testns	r18, p2, r18, #1;
							p2 ba		IC_PauseBeforeMTERecheck;
						}
						/* capture the registers again */
		#if defined(EUR_CR_MTE_MEM_CHECKSUM)
						ldr			r18, #EUR_CR_MTE_MEM_CHECKSUM >> 2, drc0;
		#else
						ldr			r18, #EUR_CR_MTE_SIG1 >> 2, drc0;
		#endif
						wdf			drc0;
						xor.testnz	p2, r17, r18;
						p2	ba		IC_PollForMTEIdle;
					}

		#if (defined(EUR_CR_MTE_TE_CHECKSUM) && defined(EUR_CR_TE_CHECKSUM)) \
				|| defined(EUR_CR_TE_DIAG5) || defined(EUR_CR_TE1)
					/* Wait for the TE to go idle */
					IC_PollForTEIdle:
					{
			#if defined(EUR_CR_MTE_TE_CHECKSUM) && defined(EUR_CR_TE_CHECKSUM)
						ldr			r17, #EUR_CR_MTE_TE_CHECKSUM >> 2, drc0;
						ldr			r18, #EUR_CR_TE_CHECKSUM >> 2, drc0;
			#else
			#if defined(EUR_CR_TE_DIAG5)
						ldr			r17, #EUR_CR_TE_DIAG2 >> 2, drc0;
						ldr			r18, #EUR_CR_TE_DIAG5 >> 2, drc0;
			#else
						ldr			r17, #EUR_CR_TE1 >> 2, drc0;
						ldr			r18, #EUR_CR_TE2 >> 2, drc0;
			#endif
			#endif
						wdf			drc0;
						xor			r19, r17, r18;
						mov			r18, #1000;
						IC_PauseBeforeTERecheck:
						{
							isub16.testns	r18, p2, r18, #1;
							p2 ba		IC_PauseBeforeTERecheck;
						}
						/* capture the registers again */
			#if defined(EUR_CR_MTE_TE_CHECKSUM) && defined(EUR_CR_TE_CHECKSUM)
						ldr			r17, #EUR_CR_MTE_TE_CHECKSUM >> 2, drc0;
						ldr			r18, #EUR_CR_TE_CHECKSUM >> 2, drc0;
			#else
			#if defined(EUR_CR_TE_DIAG5)
						ldr			r17, #EUR_CR_TE_DIAG2 >> 2, drc0;
						ldr			r18, #EUR_CR_TE_DIAG5 >> 2, drc0;
			#else
						ldr			r17, #EUR_CR_TE1 >> 2, drc0;
						ldr			r18, #EUR_CR_TE2 >> 2, drc0;
			#endif
			#endif
						wdf			drc0;
						xor			r17, r17, r18;
						xor.testnz	p2, r19, r17;
						p2	ba		IC_PollForTEIdle;
					}
		#endif
					ba	IC_PollForUSSEIdle;
				}
				{
					IC_PollForISPIdle:
					{
						/* Wait for the ISP sigs to go idle */
			#if defined(EUR_CR_ISP_PIPE0_SIG3) && defined(EUR_CR_ISP_PIPE0_SIG4)
						ldr			r17, #EUR_CR_ISP_PIPE0_SIG3 >> 2, drc0;
						ldr			r18, #EUR_CR_ISP_PIPE0_SIG4 >> 2, drc0;
						wdf			drc0;
						xor			r19, r17, r18;
						ldr			r17, #EUR_CR_ISP_PIPE1_SIG3 >> 2, drc0;
						ldr			r18, #EUR_CR_ISP_PIPE1_SIG4 >> 2, drc1;
						wdf			drc0;
						xor			r19, r17, r19;
						wdf			drc1;
						xor			r19, r18, r19;
			#else
				#if defined(EUR_CR_ISP_TAGWRITE_CHECKSUM) && defined(EUR_CR_ISP_SPAN_CHECKSUM)
						ldr			r17, #EUR_CR_ISP_TAGWRITE_CHECKSUM >> 2, drc0;
						ldr			r18, #EUR_CR_ISP_SPAN_CHECKSUM >> 2, drc0;
				#else
						ldr			r17, #EUR_CR_ISP_SIG3 >> 2, drc0;
						ldr			r18, #EUR_CR_ISP_SIG4 >> 2, drc0;
				#endif
						wdf			drc0;
						xor			r19, r17, r18;
			#endif
						mov			r18, #1000;
						IC_PauseBeforeISPRecheck:
						{
							isub16.testns	r18, p2, r18, #1;
							p2 ba		IC_PauseBeforeISPRecheck;
						}
						/* capture the registers again */
			#if defined(EUR_CR_ISP_PIPE0_SIG3) && defined(EUR_CR_ISP_PIPE0_SIG4)
						ldr			r17, #EUR_CR_ISP_PIPE0_SIG3 >> 2, drc0;
						ldr			r18, #EUR_CR_ISP_PIPE0_SIG4 >> 2, drc0;
						wdf			drc0;
						xor			r17, r17, r18;
						ldr			r18, #EUR_CR_ISP_PIPE1_SIG3 >> 2, drc0;
						wdf			drc0;
						xor			r17, r17, r18;
						ldr			r18, #EUR_CR_ISP_PIPE1_SIG4 >> 2, drc0;
						wdf			drc0;
						xor			r17, r17, r18;					
			#else
				#if defined(EUR_CR_ISP_TAGWRITE_CHECKSUM) && defined(EUR_CR_ISP_SPAN_CHECKSUM)
						ldr			r17, #EUR_CR_ISP_TAGWRITE_CHECKSUM >> 2, drc0;
						ldr			r18, #EUR_CR_ISP_SPAN_CHECKSUM >> 2, drc0;
				#else
						ldr			r17, #EUR_CR_ISP_SIG3 >> 2, drc0;
						ldr			r18, #EUR_CR_ISP_SIG4 >> 2, drc0;
				#endif
						wdf			drc0;
						xor			r17, r17, r18;
			#endif
						xor.testnz	p2, r19, r17;
						p2	ba		IC_PollForISPIdle;
					}
					IC_PollForPBEIdle:
					{
						/* Wait for the PBE sigs to go idle */
			#if defined(EUR_CR_PBE_PIXEL_CHECKSUM)
						ldr			r19, #EUR_CR_PBE_PIXEL_CHECKSUM >> 2, drc0;
			#else
						ldr			r17, #EUR_CR_PIXELBE_SIG01 >> 2, drc0;
						ldr			r18, #EUR_CR_PIXELBE_SIG02 >> 2, drc0;
						wdf			drc0;
						xor			r19, r17, r18;
				#if defined(EUR_CR_PIXELBE_SIG11) && defined(EUR_CR_PIXELBE_SIG12)
						ldr			r17, #EUR_CR_PIXELBE_SIG11 >> 2, drc0;
						ldr			r18, #EUR_CR_PIXELBE_SIG12 >> 2, drc0;
						wdf			drc0;
						xor			r19, r19, r17;
						xor			r19, r19, r18;
				#endif
			#endif
						mov			r18, #1000;
						IC_PauseBeforePBERecheck:
						{
							isub16.testns	r18, p2, r18, #1;
							p2 ba		IC_PauseBeforePBERecheck;
						}
						/* capture the registers again */
			#if defined(EUR_CR_PBE_PIXEL_CHECKSUM)
						ldr			r17, #EUR_CR_PBE_PIXEL_CHECKSUM >> 2, drc0;				
			#else
						ldr			r17, #EUR_CR_PIXELBE_SIG01 >> 2, drc0;
						ldr			r18, #EUR_CR_PIXELBE_SIG02 >> 2, drc0;
						wdf			drc0;
						xor			r17, r17, r18;
				#if defined(EUR_CR_PIXELBE_SIG11) && defined(EUR_CR_PIXELBE_SIG12)
						ldr			r18, #EUR_CR_PIXELBE_SIG11 >> 2, drc0;
						wdf			drc0;
						xor			r17, r17, r18;
						ldr			r18, #EUR_CR_PIXELBE_SIG12 >> 2, drc0;
						wdf			drc0;
						xor			r17, r17, r18;
				#endif
			#endif
						xor.testnz	p2, r19, r17;
						p2	ba		IC_PollForPBEIdle;				
					}
				}
			}
			IC_PollForUSSEIdle:
			{
				/* Reset the retry counter */
		#if defined(SGX_FEATURE_MP)
				MK_LOAD_STATE_CORE_COUNT_3D(r19, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);
				isub16	r19, r19, #1;
				IC_CheckNextCoreQueue:
		#endif
				{
					mov		r20, #USE_FALSE;
					mov		r21, #25;
	
					/* wait for queued USSE task queues to empty */
					IC_PollForUSSEQueueEmpty:
					{
						/* TA or 3D */
						and		r17, r16, #USE_IDLECORE_TA3D_REQ_MASK;
						xor.testz	p2, r17, #USE_IDLECORE_3D_REQ;
						p2	ba	IC_CheckPixTasks;
						{
		#if (SGX_FEATURE_NUM_USE_PIPES == 1)
							READCORECONFIG(r17, r19, #EUR_CR_USE_SERV_VERTEX >> 2, drc0);
							wdf		drc0;
		#else
			#if (SGX_FEATURE_NUM_USE_PIPES >= 2)
							READCORECONFIG(r17, r19, #EUR_CR_USE0_SERV_VERTEX >> 2, drc0);
							READCORECONFIG(r18, r19, #EUR_CR_USE1_SERV_VERTEX >> 2, drc0);
							/* Wait for pipes 0 and 1 to load */
							wdf		drc0;
							/* combine pipes 0 and 1 */
							and		r17, r17, r18;
			#endif
			#if (SGX_FEATURE_NUM_USE_PIPES >= 4)
							READCORECONFIG(r18, r19, #EUR_CR_USE2_SERV_VERTEX >> 2, drc0);
							wdf		drc0;
							and		r17, r17, r18;
							READCORECONFIG(r18, r19, #EUR_CR_USE3_SERV_VERTEX >> 2, drc0);
							wdf		drc0;
							and		r17, r17, r18;
			#endif /* SGX_FEATURE_NUM_USE_PIPES >= 4 */
			#if (SGX_FEATURE_NUM_USE_PIPES >= 8)
							READCORECONFIG(r18, r19, #EUR_CR_USE4_SERV_VERTEX >> 2, drc0);
							wdf		drc0;
							and		r17, r17, r18;
							READCORECONFIG(r18, r19, #EUR_CR_USE5_SERV_VERTEX >> 2, drc0);
							wdf		drc0;
							and		r17, r17, r18;
							READCORECONFIG(r18, r19, #EUR_CR_USE6_SERV_VERTEX >> 2, drc0);
							wdf		drc0;
							and		r17, r17, r18;
							READCORECONFIG(r18, r19, #EUR_CR_USE7_SERV_VERTEX >> 2, drc0);
							wdf		drc0;
							and		r17, r17, r18;
			#endif /* SGX_FEATURE_NUM_USE_PIPES >= 8 */
		#endif /* SGX_FEATURE_NUM_USE_PIPES == 1 */
							ba	IC_CheckEmpty;
						}
						IC_CheckPixTasks:
						{
		#if (SGX_FEATURE_NUM_USE_PIPES == 1)
							READCORECONFIG(r17, r19, #EUR_CR_USE_SERV_PIXEL >> 2, drc0);
							wdf		drc0;
		#else
			#if (SGX_FEATURE_NUM_USE_PIPES >= 2)
							READCORECONFIG(r17, r19, #EUR_CR_USE0_SERV_PIXEL >> 2, drc0);
							READCORECONFIG(r18, r19, #EUR_CR_USE1_SERV_PIXEL >> 2, drc0);
							/* Wait for pipes 0 and 1 to load */
							wdf		drc0;
							/* combine pipes 0 and 1 */
							and		r17, r17, r18;
			#endif
			#if (SGX_FEATURE_NUM_USE_PIPES >= 4)
							READCORECONFIG(r18, r19, #EUR_CR_USE2_SERV_PIXEL >> 2, drc0);
							wdf		drc0;
							and		r17, r17, r18;
							READCORECONFIG(r18, r19, #EUR_CR_USE3_SERV_PIXEL >> 2, drc0);
							wdf		drc0;
							and		r17, r17, r18;
			#endif /* SGX_FEATURE_NUM_USE_PIPES >= 4 */
			#if (SGX_FEATURE_NUM_USE_PIPES >= 8)
							READCORECONFIG(r18, r19, #EUR_CR_USE4_SERV_PIXEL >> 2, drc0);
							wdf		drc0;
							and		r17, r17, r18;
							READCORECONFIG(r18, r19, #EUR_CR_USE5_SERV_PIXEL >> 2, drc0);
							wdf		drc0;
							and		r17, r17, r18;
							READCORECONFIG(r18, r19, #EUR_CR_USE6_SERV_PIXEL >> 2, drc0);
							wdf		drc0;
							and		r17, r17, r18;
							READCORECONFIG(r18, r19, #EUR_CR_USE7_SERV_PIXEL >> 2, drc0);
							wdf		drc0;
							and		r17, r17, r18;
			#endif /* SGX_FEATURE_NUM_USE_PIPES >= 8 */
		#endif /* SGX_FEATURE_NUM_USE_PIPES == 1 */
						}
						IC_CheckEmpty:
	
		#if defined(EUR_CR_USE_SERV_VERTEX)
						mov			r18, #EUR_CR_USE_SERV_VERTEX_EMPTY_MASK;
		#else
						mov			r18, #EUR_CR_USE0_SERV_VERTEX_EMPTY_MASK;
		#endif
						and.testnz	p2, r17, r18;
						p2	ba	IC_USSEQueuesEmpty;
						{
							/* indicate we have looped atleast once */
							mov		r20, #USE_TRUE;
	
							/* how many times have we failed */
							isub16.tests r21, p2, r21, #1;
							/* if loops == 25, then re-enable DMS as we may have hit deadlock */
							p2 	ba	IC_EnableDMS;
							/* wait for a bit before checking again */
							mov			r18, #500;
							IC_PauseBeforeQueueRecheck:
							{
								isub16.testns	r18, p2, r18, #1;
								p2 ba		IC_PauseBeforeQueueRecheck;
							}
							ba		IC_PollForUSSEQueueEmpty;
						}
					}
					IC_USSEQueuesEmpty:
					
	#if defined(SGX_FEATURE_MP)
					isub16.testns	r19, p2, r19, #1;
					p2	ba	IC_CheckNextCoreQueue;
	#endif
				}

	#if defined(SGX_FEATURE_MP)
				MK_LOAD_STATE_CORE_COUNT_3D(r19, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);
				isub16	r19, r19, #1;
				IC_CheckNextCoreSlots:
	#endif
				{
				
	#if (SGX_FEATURE_NUM_USE_PIPES > 1)
					/* Reset the retry counter */
					mov		r21, #25;
	
					/* wait for all pipes other than 0 to finish their tasks */
					IC_PollForUSSEPipesIdle:
					{
						mov		r18, #0xAAAAAAAA;

						READCORECONFIG(r17, r19, #EUR_CR_USE1_DM_SLOT >> 2, drc0);
						wdf		drc0;

		#if (SGX_FEATURE_NUM_USE_PIPES >= 4)
						xor.testnz	p2, r17, r18;
						p2 ba IC_PipeNotIdle;

						READCORECONFIG(r17, r19, #EUR_CR_USE2_DM_SLOT >> 2, drc0);
						wdf		drc0;

						xor.testnz	p2, r17, r18;
						p2 ba IC_PipeNotIdle;

						READCORECONFIG(r17, r19, #EUR_CR_USE3_DM_SLOT >> 2, drc0);
						wdf		drc0;
		#endif
		#if (SGX_FEATURE_NUM_USE_PIPES >= 8)
						xor.testnz	p2, r17, r18;
						p2 ba IC_PipeNotIdle;

						READCORECONFIG(r17, r19, #EUR_CR_USE4_DM_SLOT >> 2, drc0);
						wdf		drc0;

						xor.testnz	p2, r17, r18;
						p2 ba IC_PipeNotIdle;

						READCORECONFIG(r17, r19, #EUR_CR_USE5_DM_SLOT >> 2, drc0);
						wdf		drc0;

						xor.testnz	p2, r17, r18;
						p2 ba IC_PipeNotIdle;

						READCORECONFIG(r17, r19, #EUR_CR_USE6_DM_SLOT >> 2, drc0);
						wdf		drc0;

						xor.testnz	p2, r17, r18;
						p2 ba IC_PipeNotIdle;

						READCORECONFIG(r17, r19, #EUR_CR_USE7_DM_SLOT >> 2, drc0);
						wdf		drc0;
		#endif /* SGX_FEATURE_NUM_USE_PIPES >= 8 */

						xor.testz	p2, r17, r18;
						p2 ba		IC_PipesIdle;
						{
							IC_PipeNotIdle:
							{
								/* indicate we have looped atleast once */
								mov		r20, #USE_TRUE;
		
								/* how many times have we failed */
								isub16.tests r21, p2, r21, #1;
								/* if loops == 50, then re-enable DMS as we may have hit deadlock */
								p2 	ba	IC_EnableDMS;
								/* wait for a bit before checking again */
								mov			r18, #500;
								IC_PauseBeforePipe1Recheck:
								{
									isub16.testns	r18, p2, r18, #1;
									p2 ba		IC_PauseBeforePipe1Recheck;
								}
								ba		IC_PollForUSSEPipesIdle;
							}
						}
					}
					IC_PipesIdle:
	#endif /* SGX_FEATURE_NUM_USE_PIPES > 1 */

					/* Reset the retry counter */
					mov		r21, #25;
	
					/* wait for active pipe 0 tasks to finish */
					/* Load the current tasks's mask into r18. */
					mov			r18, #0xAAAAAAAA;
	#if defined(SGX_FEATURE_MP)
					/* Are we checking core 0? */
					isub16.testns	p2, r19, #1;
					p2	ba	IC_PollForUSSEPipe0Idle;
	#endif
					{
						/* We are checking core 0 pipe 0, make sure we take the ukernel into account */
						shl			r17, G22, #1;
						shl			r17, #0x3, r17;
						or			r17, r17, r18;
					}
					IC_PollForUSSEPipe0Idle:
					{
	#if defined(EUR_CR_USE_DM_SLOT)
						READCORECONFIG(r18, r19, #EUR_CR_USE_DM_SLOT >> 2, drc0);
	#else
						READCORECONFIG(r18, r19, #EUR_CR_USE0_DM_SLOT >> 2, drc0);
	#endif
						wdf			drc0;
						xor.testz	p2, r17, r18;
						p2	ba		IC_Pipe0Idle;
						{
							/* indicate we have looped atleast once */
							mov		r20, #USE_TRUE;
	
							/* how many times have we failed */
							isub16.tests r21, p2, r21, #1;
							/* if loops == 50, then re-enable DMS as we may have hit deadlock */
							p2 	ba	IC_EnableDMS;
							/* wait for a bit before checking again */
							mov			r18, #500;
							IC_PauseBeforePipe0Recheck:
							{
								isub16.testns	r18, p2, r18, #1;
								p2 ba		IC_PauseBeforePipe0Recheck;
							}
							ba		IC_PollForUSSEPipe0Idle;
						}
					}
					IC_Pipe0Idle:

	#if defined(SGX_FEATURE_MP)
					/* Are there anymore cores to check? */
					isub16.testns	r19, p2, r19, #1;
					p2	ba	IC_CheckNextCoreSlots;
	#endif
				}
			}
			
			/* if we had to wait for a task to flush on the USSE then check all sigs again */
			xor.testz	p2, r20, #USE_TRUE;
			p2	ba	IC_RecheckAll;
		}
	}
	IC_CoreIdle:

	/* Return */
	lapc;

	IC_EnableDMS:
	{

		/* Ok we have re-checked enough, enable the DMS for a bit */
		/* enable DM events on PDS */
		ldr		r17, #EUR_CR_DMS_CTRL >> 2, drc0;
		wdf		drc0;
		/* re-enable all, leaving loopbacks as is */
		and		r17, r17, ~#(EUR_CR_DMS_CTRL_DISABLE_DM_VERTEX_MASK | \
							EUR_CR_DMS_CTRL_DISABLE_DM_PIXEL_MASK | \
							EUR_CR_DMS_CTRL_DISABLE_DM_EVENT_MASK);
		str		#EUR_CR_DMS_CTRL >> 2, r17;

		/* now wait for a bit */
		mov			r18, #1000;
		IC_AllowDMSToRun:
		{
			isub16.testns	r18, p2, r18, #1;
			p2 ba		IC_AllowDMSToRun;
		}
		/* Start all over again */
		ba		IC_FromTheStart;
	}
}

/*****************************************************************************
 DPMThresholdUpdate - routine to update the DPM threshold values
 					- This should only be called when the TA and 3D are idle

 inputs:	
 temps:		r16, r17, r18, r19, r20
*****************************************************************************/
DPMThresholdUpdate:
{
#if defined(FIX_HW_BRN_32302) || defined(SUPPORT_SGX_DOUBLE_PIXEL_PARTITIONS)
	xor.testnz	p3, r17, #USE_TRUE;
#endif
	MK_MEM_ACCESS_BPCACHE(ldad)	r16, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc.uiAddr)], drc0;
	wdf		drc0;
	
	MK_MEM_ACCESS_CACHED(ldad.f4)		r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32TAThreshold)-1], drc0;
	wdf		drc0;
	str		#EUR_CR_DPM_TA_PAGE_THRESHOLD >> 2, r17;

	MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], drc0;

	and		r19, r19, #EUR_CR_DPM_TA_GLOBAL_LIST_SIZE_MASK;
	or		r19, r19, #EUR_CR_DPM_TA_GLOBAL_LIST_POLICY_MASK;
	str		#EUR_CR_DPM_TA_GLOBAL_LIST >> 2, r19;
	str		#EUR_CR_DPM_PDS_PAGE_THRESHOLD >> 2, r20;

#if defined(FIX_HW_BRN_32302) || defined(SUPPORT_SGX_DOUBLE_PIXEL_PARTITIONS)
	p3		ba	DPTU_SkipZLSThresholdOverride;
	and.testz	p3,	r20, r20;

	MK_LOAD_STATE(r20, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	mov	r19, #~(TA_IFLAGS_ZLSTHRESHOLD_LOWERED);
	and	r20, r20, r19;
	MK_STORE_STATE(ui32ITAFlags, r20);
	MK_WAIT_FOR_STATE_STORE(drc0);

	p3		ba	DPTU_SkipZLSThresholdOverride;

	MK_MEM_ACCESS_BPCACHE(staw)	[R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16RealZLSThreshold)], r18;
	idf	drc0, st;
	wdf	drc0;
	/* The default is a lowered threshold */
	mov	r19, #TA_IFLAGS_ZLSTHRESHOLD_LOWERED;
	or	r20, r20, r19;
	MK_STORE_STATE(ui32ITAFlags, r20);
	MK_WAIT_FOR_STATE_STORE(drc0);

	mov r19, #(SGX_THRESHOLD_REDUCTION_FOR_VPCHANGE);
	isub16 r18, r18, r19;
	DPTU_SkipZLSThresholdOverride:
#endif

	str		#EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, r18;
	wdf				drc0;
	and				r17, r17, ~#HWPBDESC_FLAGS_UPDATE_MASK;
	MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], r17;
	idf		drc0, st;
	wdf		drc0;
	
	/* Do a free list load to get the hardware to load the thresholds. */
	/* Returns directly to our caller. */
	ba		DPMReloadFreeList;
}

/*****************************************************************************
 DPMReloadFreeList  - routine to do a TA free list load using the current values
 					- This should only be called when the TA and 3D are idle

 inputs:	none
 temps:		r16, r17, r18, r19
*****************************************************************************/
DPMReloadFreeList:
{
	/* DPM thresholds need modifying */
	/* First do the dummy store */
	/* PB Store: */
	ldr		r17, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_STATUS1 >> 2, drc0;
	ldr		r18, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_STATUS2 >> 2, drc0;
	ldr		r19, #EUR_CR_DPM_PAGE_STATUS >> 2, drc0;
	wdf		drc0;
	and		r19, r19, #EUR_CR_DPM_PAGE_STATUS_TA_MASK;
	shr		r19, r19, #EUR_CR_DPM_PAGE_STATUS_TA_SHIFT;

	ldr		r16, #EUR_CR_DPM_GLOBAL_PAGE_STATUS >> 2, drc0;
	wdf		drc0;
	and		r16, r16, #EUR_CR_DPM_GLOBAL_PAGE_STATUS_TA_MASK;
	shr		r16, r16, #EUR_CR_DPM_GLOBAL_PAGE_STATUS_TA_SHIFT;

	/* PB Load: */
	str		#EUR_CR_DPM_TA_ALLOC_FREE_LIST >> 2, r17;
	shl		r18, r18, #16;
	str		#EUR_CR_DPM_TA_ALLOC >> 2, r18;
	shl		r16, r16, #EUR_CR_DPM_START_OF_CONTEXT_PAGES_ALLOCATED_GLOBAL_SHIFT;
	or		r19, r19, r16;
	str		#EUR_CR_DPM_START_OF_CONTEXT_PAGES_ALLOCATED >> 2, r19;

	/* load up the PB on the TA */
	str		#EUR_CR_DPM_TASK_TA_FREE >> 2, #EUR_CR_DPM_TASK_TA_FREE_LOAD_MASK;

	/* Wait for the PB load to finish */
	ENTER_UNMATCHABLE_BLOCK;
	DPMTU_WaitForPBLoad:
	{
		ldr		r18, #EUR_CR_EVENT_STATUS2 >> 2, drc0;
		wdf		drc0;
		and.testz	p2,	r18, #EUR_CR_EVENT_STATUS2_DPM_TA_FREE_LOAD_MASK;
		p2	ba		DPMTU_WaitForPBLoad;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the load event */
	str		#EUR_CR_EVENT_HOST_CLEAR2 >> 2, #EUR_CR_EVENT_HOST_CLEAR2_DPM_TA_FREE_LOAD_MASK;
	lapc;
}

/*****************************************************************************
 StoreTAPB  - routine to do a TA free list store using the current values
 					- This should only be called when the TA is idle

 inputs:	none
 temps:		r16, r17, r18
*****************************************************************************/
StoreTAPB:
{
	/* Load the address of the TA's SGXMKIF_HWPBDESC */
	MK_MEM_ACCESS_BPCACHE(ldad)	r16, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc.uiAddr)], drc0;

	ldr		r17, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_STATUS1 >> 2, drc0;
	ldr		r18, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_STATUS2 >> 2, drc1;

	wdf		drc0;
	MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListHT)], r17;

	wdf		drc1;
	shl		r18, r18, #16;
	MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListPrev)], r18;

	ldr		r17, #EUR_CR_DPM_PAGE_STATUS >> 2, drc0;
	ldr		r18, #EUR_CR_DPM_GLOBAL_PAGE_STATUS >> 2, drc1;

	wdf		drc0;
	and		r17, r17, #EUR_CR_DPM_PAGE_STATUS_TA_MASK;
	shr		r17, r17, #EUR_CR_DPM_PAGE_STATUS_TA_SHIFT;
	MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWPBDESC.uiLocalPages)], r17;

	wdf		drc1;
	and		r18, r18, #EUR_CR_DPM_GLOBAL_PAGE_STATUS_TA_MASK;
	shr		r18, r18, #EUR_CR_DPM_GLOBAL_PAGE_STATUS_TA_SHIFT;
	MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWPBDESC.uiGlobalPages)], r18;
	idf		drc0, st;
	wdf		drc0;

	lapc;
}

/*****************************************************************************
 LoadTAPB  - routine to do a TA free list load using the values in memory
 					- This should only be called when the TA is idle

 inputs:	r16 - SGXMKIF_HWPBDESC
 temps:		r16, r17, r18
*****************************************************************************/
LoadTAPB:
{
	/* save the new HW PB Desc as the current HW PB Desc */
#if defined(FIX_HW_BRN_32302) || defined(SUPPORT_SGX_DOUBLE_PIXEL_PARTITIONS)
	xor.testnz	p3,	r17,	#USE_TRUE;
#endif
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc.uiAddr)], r16;
	idf		drc1, st;

	MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.sEVMPageTableDevVAddr.uiAddr)], drc0;
	wdf		drc0;
	and r17, r17, #EUR_CR_DPM_TA_ALLOC_PAGE_TABLE_BASE_ADDR_MASK;
	str		#EUR_CR_DPM_TA_ALLOC_PAGE_TABLE_BASE >> 2, r17;

	MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListHT)], drc0;
	wdf		drc0;
	str		#EUR_CR_DPM_TA_ALLOC_FREE_LIST >> 2, r17;

	MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListPrev)], drc0;
	wdf		drc0;
	str		#EUR_CR_DPM_TA_ALLOC >> 2, r17;

	MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.uiLocalPages)], drc0;
	MK_MEM_ACCESS(ldad)	r18, [r16, #DOFFSET(SGXMKIF_HWPBDESC.uiGlobalPages)], drc0;
	wdf		drc0;
	shl		r18, r18, #EUR_CR_DPM_START_OF_CONTEXT_PAGES_ALLOCATED_GLOBAL_SHIFT;
	or		r17, r17, r18;
	str		#EUR_CR_DPM_START_OF_CONTEXT_PAGES_ALLOCATED >> 2, r17;

	MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32TAThreshold)], drc0;
	wdf		drc0;
	str		#EUR_CR_DPM_TA_PAGE_THRESHOLD>>2, r17;


	MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32PDSThreshold)], drc0;
	wdf		drc0;
	str		#EUR_CR_DPM_PDS_PAGE_THRESHOLD >> 2, r17;

#if defined(FIX_HW_BRN_32302) || defined(SUPPORT_SGX_DOUBLE_PIXEL_PARTITIONS)
	p3	ba	LTAPB_NoOverrideZLSThreshold;
	and.testz	p3, r17, r17;

	/* Clear the flag, we do not want it set if 
 	   we are going to jump off for PDS threshold
	   being zero.
	*/
	MK_LOAD_STATE(r18, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	mov	r19, #~(TA_IFLAGS_ZLSTHRESHOLD_LOWERED);
	and	r18, r18, r19;
	MK_STORE_STATE(ui32ITAFlags, r18);
	MK_WAIT_FOR_STATE_STORE(drc0);

	p3	ba	LTAPB_NoOverrideZLSThreshold;

	mov	r19, #TA_IFLAGS_ZLSTHRESHOLD_LOWERED;
	or	r18, r18, r19;
	MK_STORE_STATE(ui32ITAFlags, r18);
	MK_WAIT_FOR_STATE_STORE(drc0);
	

	MK_MEM_ACCESS(ldad)	r18, [r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32ZLSThreshold)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(staw)	[R_TA3DCtl, #WOFFSET(SGXMK_TA3D_CTL.ui16RealZLSThreshold)], r18;
	idf	drc0, st;
	wdf	drc0;
	mov	r19, #(SGX_THRESHOLD_REDUCTION_FOR_VPCHANGE);
	isub16 r17, r18, r19;

	ba	LTAPB_OverrideZLSThreshold;
	LTAPB_NoOverrideZLSThreshold:
#endif
	MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32ZLSThreshold)], drc0;
	wdf		drc0;
#if defined(FIX_HW_BRN_32302) || defined(SUPPORT_SGX_DOUBLE_PIXEL_PARTITIONS)
	LTAPB_OverrideZLSThreshold:
#endif
	str		#EUR_CR_DPM_ZLS_PAGE_THRESHOLD>>2, r17;

	MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32GlobalThreshold)], drc0;
	wdf		drc0;
	and		r17, r17, #EUR_CR_DPM_TA_GLOBAL_LIST_SIZE_MASK;
	or		r17, r17, #EUR_CR_DPM_TA_GLOBAL_LIST_POLICY_MASK;
	str		#EUR_CR_DPM_TA_GLOBAL_LIST >> 2, r17;
	wdf		drc1;
	
	#if defined(EUR_CR_MASTER_DPM_DRAIN_HEAP)
	/* Invalidate the Drain Heap head and tail values */
	mov		r18, #0xFFFFFFFF;
	WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_DRAIN_HEAP >> 2, r18, r17);
	#endif
	/* call macro to check PB sharing, must start from r17 */
	CHECKPBSHARING();

	/* load up the PB on the TA */
	str		#EUR_CR_DPM_TASK_TA_FREE >> 2, #EUR_CR_DPM_TASK_TA_FREE_LOAD_MASK;

	/* Wait for the PB load to finish */
	ENTER_UNMATCHABLE_BLOCK;
	LTAPB_WaitForPBLoad:
	{
		ldr		r17, #EUR_CR_EVENT_STATUS2 >> 2, drc0;
		wdf		drc0;
		and.testz	p2,	r17, #EUR_CR_EVENT_STATUS2_DPM_TA_FREE_LOAD_MASK;
		p2	ba		LTAPB_WaitForPBLoad;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the load event */
	str		#EUR_CR_EVENT_HOST_CLEAR2 >> 2, #EUR_CR_EVENT_HOST_CLEAR2_DPM_TA_FREE_LOAD_MASK;

#if !defined(EUR_CR_DPM_CONTEXT_PB_BASE) || defined(FIX_HW_BRN_25910)
	MK_MEM_ACCESS_BPCACHE(ldad)	r17, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DHWPBDesc.uiAddr)], drc0;
	wdf		drc0;
	/* if the TA and 3D are different, then load onto 3D also (which has already been stored) */
	xor.testnz	p2, r16, r17;
	p2	ba	Load3DPB;
#endif

	lapc;
}

/*****************************************************************************
 Store3DPB  - routine to do a 3D free list store using the current values
 					- This should only be called when the 3D is idle

 inputs:	none
 temps:		r16, r17, r18
*****************************************************************************/
Store3DPB:
{
	/* Load the address of the 3D's SGXMKIF_HWPBDESC */
	MK_MEM_ACCESS_BPCACHE(ldad)	r16, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DHWPBDesc.uiAddr)], drc0;

	ldr		r17, #EUR_CR_DPM_3D_FREE_LIST_STATUS1 >> 2, drc0;
	ldr		r18, #EUR_CR_DPM_3D_FREE_LIST_STATUS2 >> 2, drc1;
	wdf		drc0;
	MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListHT)], r17;

	wdf		drc1;
	shl		r18, r18, #16;
	MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListPrev)], r18;

	ldr		r17, #EUR_CR_DPM_PAGE >> 2, drc0;
	ldr		r18, #EUR_CR_DPM_GLOBAL_PAGE >> 2, drc1;
	wdf		drc0;
	and		r17, r17, #EUR_CR_DPM_PAGE_STATUS_3D_MASK;
	shr		r17, r17, #EUR_CR_DPM_PAGE_STATUS_3D_SHIFT;
	MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWPBDESC.uiLocalPages)], r17;

	wdf		drc1;
	and		r18, r18, #EUR_CR_DPM_GLOBAL_PAGE_STATUS_3D_MASK;
	shr		r18, r18, #EUR_CR_DPM_GLOBAL_PAGE_STATUS_3D_SHIFT;
	MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWPBDESC.uiGlobalPages)], r18;
	idf		drc0, st;
	wdf		drc0;

	lapc;
}


/*****************************************************************************
 Load3DPB  - routine to do a 3D free list load using the values in memory
 					- This should only be called when the 3D is idle

 inputs:	r16 - SGXMKIF_HWPBDESC
 temps:		r16, r17, r18
*****************************************************************************/
Load3DPB:
{
	/* save the new PB as the current PB */
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DHWPBDesc.uiAddr)], r16;
	idf		drc1, st;

	MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.sEVMPageTableDevVAddr.uiAddr)], drc0;
	wdf		drc0;
	and r17, r17, #EUR_CR_DPM_3D_PAGE_TABLE_BASE_ADDR_MASK;
	str		#EUR_CR_DPM_3D_PAGE_TABLE_BASE >> 2, r17;

	MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListHT)], drc0;
	wdf		drc0;
	str		#EUR_CR_DPM_3D_FREE_LIST >> 2, r17;

	MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListPrev)], drc0;
	wdf		drc0;
	str		#EUR_CR_DPM_3D >> 2, r17;

	MK_MEM_ACCESS(ldad)	r18, [r16, #DOFFSET(SGXMKIF_HWPBDESC.uiGlobalPages)], drc0;
	MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.uiLocalPages)], drc1;
	wdf		drc0;
	shl		r18, r18, #EUR_CR_DPM_START_OF_3D_CONTEXT_PAGES_ALLOCATED_GLOBAL_SHIFT;
	wdf		drc1;
	or		r17, r17, r18;
	str		#EUR_CR_DPM_START_OF_3D_CONTEXT_PAGES_ALLOCATED >> 2, r17;

	/* call macro to check PB sharing, must start from r17 */
	CHECKPBSHARING();

	/* load up the PB on the 3D */
	str		#EUR_CR_DPM_TASK_3D_FREE >> 2, #EUR_CR_DPM_TASK_3D_FREE_LOAD_MASK;

	/* Wait for the PB load to finish */
	ENTER_UNMATCHABLE_BLOCK;
	L3DPB_WaitForPBLoad:
	{
		ldr		r17, #EUR_CR_EVENT_STATUS2 >> 2, drc0;
		wdf		drc0;
		and.testz	p2,	r17, #EUR_CR_EVENT_STATUS2_DPM_3D_FREE_LOAD_MASK;
		p2	ba		L3DPB_WaitForPBLoad;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the load event */
	str		#EUR_CR_EVENT_HOST_CLEAR2 >> 2, #EUR_CR_EVENT_HOST_CLEAR2_DPM_3D_FREE_LOAD_MASK;

#if !defined(EUR_CR_DPM_CONTEXT_PB_BASE) || defined(FIX_HW_BRN_25910)
	MK_MEM_ACCESS_BPCACHE(ldad)	r17, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc.uiAddr)], drc0;
	wdf		drc0;
	/* if the TA and 3D are different, then load onto TA also (which has already been stored) */
	xor.testnz	p2, r16, r17;
#if defined(FIX_HW_BRN_32302) || defined(SUPPORT_SGX_DOUBLE_PIXEL_PARTITIONS)
	mov	r17, #USE_FALSE;
#endif
	p2	ba	LoadTAPB;
#endif

	lapc;
}


#if !defined(SGX_FAST_DPM_INIT) || (SGX_FEATURE_NUM_USE_PIPES < 2)

/*****************************************************************************
 FUNCTION	: SetupDPMPagetable

 inputs:	r16 - SGXMKIF_HWPBDESC
 temps:		r16, r17, r18, r19, r20, r21
*****************************************************************************/
SetupDPMPagetable:
{
	MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], drc0;
	wdf				drc0;
	and.testz		p2, r17, #HWPBDESC_FLAGS_INITPT;
	p2				ba SDPT_NoInitPending;
	{
		and				r17, r17, ~#HWPBDESC_FLAGS_UPDATE_MASK;
		MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], r17;
#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
		MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWPBDESC.sGrowListPBBlockDevVAddr)], #0;
#endif

		MK_MEM_ACCESS(ldad.f2)	r18, [r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListInitialHT)-1], drc0;
		wdf				drc0;
		MK_MEM_ACCESS(stad.f2)	[r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListHT)-1], r18;
		MK_MEM_ACCESS(stad.f2)	[r16, #DOFFSET(SGXMKIF_HWPBDESC.uiLocalPages)-1], #0;

		MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.sEVMPageTableDevVAddr.uiAddr)], drc1;
		MK_MEM_ACCESS(ldad)	r16, [r16, #DOFFSET(SGXMKIF_HWPBDESC.sListPBBlockDevVAddr)], drc0;
		wdf		drc0;

		MK_MEM_ACCESS(ldad)	r18, [r16, #DOFFSET(SGXMKIF_HWPBBLOCK.ui32PageCount)], drc1;
		MK_MEM_ACCESS(ldaw)	r19, [r16, #WOFFSET(SGXMKIF_HWPBBLOCK.ui16Head)], drc1;
		wdf		drc1;
		/* Head prev = 0xFFFF */
		mov				r20, #(0xFFFF << 16);

		ba	SDPT_SetupPBNextPage;
	}
	SDPT_NoInitPending:
#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
	and.testz		p2, r17, #HWPBDESC_FLAGS_GROW;
	p2				ba SDPT_NoGrowPending;
	{
		and				r17, r17, ~#HWPBDESC_FLAGS_GROW;
		MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], r17;

		MK_MEM_ACCESS(ldad.f2)	r18, [r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListInitialHT)-1], drc0;
		MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListHT)], drc0;
		wdf				drc0;
		/* Change the tail */
		and		r18, r18, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_TAIL_MASK;
		and		r20, r17, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_HEAD_MASK;
		or		r18, r20, r18;
		MK_MEM_ACCESS(stad.f2)	[r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListHT)-1], r18;

		mov		r18, r16;

		/* r16 = hwpbblock */
		MK_MEM_ACCESS(ldad)	r16, [r18, #DOFFSET(SGXMKIF_HWPBDESC.sGrowListPBBlockDevVAddr)], drc0;

		/* r19 = current (Calculate the offset of the current tail) */
		and	r19, r17, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_TAIL_MASK;

		/* r17 = pagetablebase */
		MK_MEM_ACCESS(ldad)	r17, [r18, #DOFFSET(SGXMKIF_HWPBDESC.sEVMPageTableDevVAddr.uiAddr)], drc1;

		shr	r19, r19, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_TAIL_SHIFT;
		or	r19, r19, #0x40000;

		/* Wait for the PT base address to load from memory */
		wdf		drc1;
		/* r20 = prev (Load the PT entry for the current tail) */
		MK_MEM_ACCESS_BPCACHE(ldad)	r20, [r17, r19], drc0;
		wdf		drc0;
		and		r20, r20, #EURASIA_PARAM_MANAGE_TABLE_PREV_MASK;

		/* There is another block to initialise, get the head of the new block */
		MK_MEM_ACCESS(ldaw)	r21, [r16, #WOFFSET(SGXMKIF_HWPBBLOCK.ui16Head)], drc0;

		/* Combine the prev with the new next value */
		wdf		drc0;
		or	r20, r20, r21;
		/* re-write the entry */
		MK_MEM_ACCESS_BPCACHE(stad)	[r17, r19], r20;

		/* Prev = Current */
		shl		r20, r19, #EURASIA_PARAM_MANAGE_TABLE_PREV_SHIFT;
		/* Current = Head */
		mov		r19, r21;
		or	r19, r19, #0x40000;

		/* load the new blocks head entry */
		MK_MEM_ACCESS_BPCACHE(ldad)	r21, [r17, r19], drc0;
		wdf		drc0;

		/* Change the next with the new prev value */
		and		r21, r21, #EURASIA_PARAM_MANAGE_TABLE_NEXT_MASK;
		or		r21, r21, r20;
		
		/* re-write the entry */
		MK_MEM_ACCESS_BPCACHE(stad)	[r17, r19], r21;

		/* The Grow list is about to be linked so clear the pointer */
		MK_MEM_ACCESS(stad)	[r18, #DOFFSET(SGXMKIF_HWPBDESC.sGrowListPBBlockDevVAddr)], #0;
		idf		drc0, st;
		wdf		drc0;
		
		/* Exit as the new block has been linked in */
		ba	SDPT_EndOfPT;
	}
	SDPT_NoGrowPending:
#endif
	/* If we get here no initialisation is required */
	ba SDPT_EndOfPT;
	{
		/*
			r16 = hwpbblock
			r17 = pagetablebase
			r18 = count
			r19 = current
			r20 = prev
			r21 = next (calculated)
		*/
		SDPT_SetupPBNextPage:
		{
			/* or in DWORD size */
			or	r19, r19, #0x40000;

			isub16.testnz	r18, p2, r18, #1;
			p2 ba SDPT_NotLastEntry;
			{
				/* We have finished the current block, check if there is another */
				MK_MEM_ACCESS(ldad)	r16, [r16, #DOFFSET(SGXMKIF_HWPBBLOCK.sNextHWPBBlockDevVAddr)], drc0;
				wdf		drc0;
				and.testz	p2, r16, r16;
				p2	br	SDPT_FinishedAllBlocks;
				{
					SDPT_LinkPBBlock:

					/* There is another block to initialise, get the head of the new block */
					MK_MEM_ACCESS(ldaw)	r21, [r16, #WOFFSET(SGXMKIF_HWPBBLOCK.ui16Head)], drc0;
					MK_MEM_ACCESS(ldad)	r18, [r16, #DOFFSET(SGXMKIF_HWPBBLOCK.ui32PageCount)], drc1;

					/* Combine the prev with the new next value */
					wdf		drc0;
					or	r20, r20, r21;
					/* re-write the entry */
					MK_MEM_ACCESS_BPCACHE(stad)	[r17, r19], r20;

					/* Prev = Current */
					shl		r20, r19, #EURASIA_PARAM_MANAGE_TABLE_PREV_SHIFT;
					/* Current = Head */
					mov		r19, r21;

					wdf		drc1;

					/* setup the new block */
					ba	SDPT_SetupPBNextPage;
				}
				SDPT_FinishedAllBlocks:

				/* patch the last one */
				or				r21, r20, #0xFFFF;
				MK_MEM_ACCESS_BPCACHE(stad)	[r17, r19], r21;
				idf				drc0, st;
				wdf				drc0;
				ba 				SDPT_EndOfPT;
			}
			SDPT_NotLastEntry:

	#if defined(FIX_HW_BRN_23281)
			iaddu32		r21, r19.low, #2;
	#else
			iaddu32		r21, r19.low, #1;
	#endif

			/* Combine the prev and next values */
			or		r21, r20, r21;
			MK_MEM_ACCESS_BPCACHE(stad)	[r17, r19], r21;
			idf	drc0, st;
			wdf	drc0;

			/* Prev = Current */
			shl				r20, r19, #EURASIA_PARAM_MANAGE_TABLE_PREV_SHIFT;
	#if defined(FIX_HW_BRN_23281)
			/* Current = Current + 2 */
			iaddu32			r19, r19.low, #2;
	#else
			/* Current = Current + 1 */
			iaddu32			r19, r19.low, #1;
	#endif
			ba				SDPT_SetupPBNextPage;
		}
	}
	SDPT_EndOfPT:

	lapc;
}

#else /* SGX_FAST_DPM_INIT */

/*****************************************************************************
 FUNCTION	: SetupDPMPagetable

 inputs:	r16 - SGXMKIF_HWPBDESC
 temps:		r16, r17, r18, r19, r20, r21
*****************************************************************************/
SetupDPMPagetable:
{
	MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], drc0;
	wdf				drc0;
	and.testz		p2, r17, #HWPBDESC_FLAGS_INITPT;
	p2				ba SDPT_NoInitPending;
	{
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sInitHWPBDesc.uiAddr)], r16;
		idf		drc0, st;

#if !defined(SGX545)
		ENTER_UNMATCHABLE_BLOCK;
		SDPT_FifoFull:
		{
			ldr		r19, #EUR_CR_LOOPBACK >> 2, drc1;
			wdf		drc1;
			and.testz	p2, r19, #EUR_CR_LOOPBACK_STATUS_MASK;
			p2	ba		SDPT_FifoFull;
		}
		LEAVE_UNMATCHABLE_BLOCK;
#endif

		/* Clear bit 0 of GLOBCOM1, before emitting loopback */
		str		#SGX_MP_CORE_SELECT(EUR_CR_USE_GLOBCOM1, 0) >> 2, #0;

		/* Save r16 because even-numbered sources are required for emitpds with 64-bit encoding */
		mov	r17, r16;

		/* Setup the Loopback type data */
		mov	r20, #USE_LOOPBACK_INITPB;

		SDPT_PDSConstSize:
		mov	r16, #0xDEADBEEF;
		wdf		drc0;

		.align 2;
		SDPT_PDSDevAddr:
		mov	r18, #0xDEADBEEF;
		/* emit the loop back */
#if defined(EURASIA_PDSSB3_USEPIPE_PIPE1)
		emitpds.tasks.taske #0, r16, r18, r20, #((EURASIA_PDSSB3_USEDATAMASTER_EVENT << EURASIA_PDSSB3_USEDATAMASTER_SHIFT) | \
                                           		(1 << EURASIA_PDSSB3_USEINSTANCECOUNT_SHIFT) | EURASIA_PDSSB3_USEPIPE_PIPE1);
#else
		/* make it emit to pipe 1 */
	#if defined(EURASIA_PDSSB1_USEPIPE_SHIFT)
		and	r18, r18, #EURASIA_PDSSB1_USEPIPE_CLRMSK;
		or	r18, r18, #(EURASIA_PDSSB1_USEPIPE_PIPE1 << EURASIA_PDSSB1_USEPIPE_SHIFT);
	#else
		and	r16, r16, #EURASIA_PDSSB0_USEPIPE_CLRMSK;
		or	r16, r16, #(EURASIA_PDSSB0_USEPIPE_PIPE1 << EURASIA_PDSSB0_USEPIPE_SHIFT);
	#endif
		emitpds.tasks.taske #0, r16, r18, r20, #((EURASIA_PDSSB3_USEDATAMASTER_EVENT << EURASIA_PDSSB3_USEDATAMASTER_SHIFT) | \
                                           		(1 << EURASIA_PDSSB3_USEINSTANCECOUNT_SHIFT));
#endif

		/* Restore saved value of r16 */
		mov r16, r17;

		/* wait for signal back */
#if defined(SGX_FEATURE_GLOBAL_REGISTER_MONITORING)
		setm #1;
#endif /* SGX_FEATURE_GLOBAL_REGISTER_MONITORING */

		ENTER_UNMATCHABLE_BLOCK;
		SDPT_WaitForComplete:
		{
			ldr			r17, #SGX_MP_CORE_SELECT(EUR_CR_USE_GLOBCOM1, 0) >> 2, drc0;
			wdf			drc0;
			and.testz	p2, r17, #1;

#if defined(SGX_FEATURE_GLOBAL_REGISTER_MONITORING)
			p2	ba.mon	SDPT_WaitForComplete;
#else
			p2	ba		SDPT_WaitForComplete;
#endif /* SGX_FEATURE_GLOBAL_REGISTER_MONITORING */
		}
		LEAVE_UNMATCHABLE_BLOCK;

		str		#SGX_MP_CORE_SELECT(EUR_CR_USE_GLOBCOM1, 0) >> 2, #0;

		MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], drc0;
		wdf				drc0;
		and				r17, r17, ~#HWPBDESC_FLAGS_INITPT;
		MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], r17;
		idf		drc0, st;
		wdf		drc0;
		
		ba	SDPT_EndOfPT;
	}
	SDPT_NoInitPending:
#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
	and.testz		p2, r17, #HWPBDESC_FLAGS_GROW;
	p2				ba SDPT_NoGrowPending;
	{
		and				r17, r17, ~#HWPBDESC_FLAGS_GROW;
		MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], r17;

		MK_MEM_ACCESS(ldad.f2)	r18, [r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListInitialHT)-1], drc0;
		MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListHT)], drc0;
		wdf				drc0;
		/* Change the tail */
		and		r18, r18, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_TAIL_MASK;
		and		r20, r17, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_HEAD_MASK;
		or		r18, r20, r18;
		MK_MEM_ACCESS(stad.f2)	[r16, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListHT)-1], r18;

		mov		r18, r16;

		/* r16 = hwpbblock */
		MK_MEM_ACCESS(ldad)	r16, [r18, #DOFFSET(SGXMKIF_HWPBDESC.sGrowListPBBlockDevVAddr)], drc0;

		/* r19 = current (Calculate the offset of the current tail) */
		and	r19, r17, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_TAIL_MASK;

		/* r17 = pagetablebase */
		MK_MEM_ACCESS(ldad)	r17, [r18, #DOFFSET(SGXMKIF_HWPBDESC.sEVMPageTableDevVAddr.uiAddr)], drc1;

		shr	r19, r19, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_TAIL_SHIFT;
		or	r19, r19, #0x40000;

		/* Wait for the PT base address to load from memory */
		wdf		drc1;
		/* r20 = prev (Load the PT entry for the current tail) */
		MK_MEM_ACCESS_CACHED(ldad)	r20, [r17, r19], drc0;
		wdf		drc0;
		and		r20, r20, #EURASIA_PARAM_MANAGE_TABLE_PREV_MASK;

		/* There is another block to initialise, get the head of the new block */
		MK_MEM_ACCESS(ldaw)	r21, [r16, #WOFFSET(SGXMKIF_HWPBBLOCK.ui16Head)], drc0;

		/* Combine the prev with the new next value */
		wdf		drc0;
		or	r20, r20, r21;
		/* re-write the entry */
		MK_MEM_ACCESS_BPCACHE(stad)	[r17, r19], r20;

		/* Prev = Current */
		shl		r20, r19, #EURASIA_PARAM_MANAGE_TABLE_PREV_SHIFT;
		/* Current = Head */
		mov		r19, r21;
		or	r19, r19, #0x40000;

		/* load the new blocks head entry */
		MK_MEM_ACCESS_BPCACHE(ldad)	r21, [r17, r19], drc0;
		wdf		drc0;

		/* Change the next with the new prev value */
		and		r21, r21, #EURASIA_PARAM_MANAGE_TABLE_NEXT_MASK;
		or		r21, r21, r20;
		
		/* re-write the entry */
		MK_MEM_ACCESS_BPCACHE(stad)	[r17, r19], r21;

		/* The Grow list is about to be linked so clear the pointer */
		MK_MEM_ACCESS(stad)	[r18, #DOFFSET(SGXMKIF_HWPBDESC.sGrowListPBBlockDevVAddr)], #0;
		idf		drc0, st;
		wdf		drc0;
	}
	SDPT_NoGrowPending:
#endif
	SDPT_EndOfPT:
	
	lapc;
}

/*****************************************************************************
 FUNCTION	: InitPBLoopback

 inputs:
 temps:		r0 -
*****************************************************************************/
.export InitPBLoopback;
InitPBLoopback:
{
	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sInitHWPBDesc)], drc0;
	wdf		drc0;

	/* used for imae later */
	mov		r19, #0x00010001;

	/* What operation is required */
	MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], drc0;
	wdf				drc0;
	and.testz		p0, r1, #HWPBDESC_FLAGS_INITPT;
	p0	ba IPBL_NoInitPending;
	{
		and				r1, r1, ~#HWPBDESC_FLAGS_UPDATE_MASK;
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], r1;
#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWPBDESC.sGrowListPBBlockDevVAddr)], #0;
#endif

		MK_MEM_ACCESS(ldad.f2)	r1, [r0, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListInitialHT)-1], drc0;
		wdf				drc0;
		MK_MEM_ACCESS(stad.f2)	[r0, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListHT)-1], r1;
		MK_MEM_ACCESS(stad.f2)	[r0, #DOFFSET(SGXMKIF_HWPBDESC.uiLocalPages)-1], #0;

		/* Get the first block within the PB */
		MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWPBDESC.sEVMPageTableDevVAddr)], drc1;
		MK_MEM_ACCESS(ldad)	r21, [r0, #DOFFSET(SGXMKIF_HWPBDESC.sListPBBlockDevVAddr)], drc0;
		wdf		drc0;

		MK_MEM_ACCESS(ldaw)	r4, [r21, #WOFFSET(SGXMKIF_HWPBBLOCK.ui16Head)], drc1;
		MK_MEM_ACCESS(ldad)	r0, [r21, #DOFFSET(SGXMKIF_HWPBBLOCK.ui32PageCount)], drc1;

		wdf		drc1;

		ba	IPBL_WritePBHeadEntry;
	}
	IPBL_NoInitPending:
#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
	and.testz		p0, r1, #HWPBDESC_FLAGS_GROW;
	p0	ba IPBL_NoGrowPending;
	{
		and				r1, r1, ~#HWPBDESC_FLAGS_GROW;
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], r1;

		MK_MEM_ACCESS(ldad.f2)	r2, [r0, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListInitialHT)-1], drc0;
		MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListHT)], drc0;
		wdf				drc0;
		/* Change the tail */
		and		r2, r2, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_TAIL_MASK;
		and		r5, r4, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_HEAD_MASK;
		or		r2, r5, r2;
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListHT)], r2;
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWPBDESC.ui32FreeListPrev)], r3;

		/* Get the first block within the Grow list */
		MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWPBDESC.sEVMPageTableDevVAddr.uiAddr)], drc1;
		MK_MEM_ACCESS(ldad)	r21, [r0, #DOFFSET(SGXMKIF_HWPBDESC.sGrowListPBBlockDevVAddr)], drc0;

		/* r1 = current (Calculate the offset of the current tail) */
		and	r4, r4, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_TAIL_MASK;
		shr	r4, r4, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_TAIL_SHIFT;

		/* Wait for the PT base address to load from memory */
		wdf		drc1;
		/* Offset into the pagetable by the head */
		imae	r1, r4.low, #SIZEOF(IMG_UINT32), r1, u32;

		/* r3 = prev (Load the PT entry for the current tail) */
		MK_MEM_ACCESS_CACHED(ldad)	r3, [r1], drc0;
		/* The Grow list is about to be linked so clear the pointer */
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWPBDESC.sGrowListPBBlockDevVAddr)], #0;

 		/* r5 = old tail */
 		mov		r5,	r4;
 		
 		/* Initialise the new block, get the head of the new block */
 		MK_MEM_ACCESS(ldaw)	r4, [r21, #WOFFSET(SGXMKIF_HWPBBLOCK.ui16Head)], drc0;
 		wdf		drc0;
 		/* Make the last entry point to the head of the new block */
 		and		r3, r3, ~#0xFFFF;
 		or		r3, r3, r4;
 
 		MK_MEM_ACCESS_BPCACHE(stad)	[r1], r3;
 		
 		/* calculate the address of the first entry in the pagetable for the new block */
 		MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWPBDESC.sEVMPageTableDevVAddr)], drc0;
 		wdf		drc0;
 		/* Offset into the pagetable by the head */
 		imae	r1, r4.low, #SIZEOF(IMG_UINT32), r1, u32;
 
 		MK_MEM_ACCESS(ldad)	r0, [r21, #DOFFSET(SGXMKIF_HWPBBLOCK.ui32PageCount)], drc0;
 		wdf		drc0;
 
 		/* write the first entry of the new block */
 		shl		r5, r5, #16;
 		#if defined(FIX_HW_BRN_23281)
 		iaddu32		r3, r4.low, #2;
 		or		r3, r5, r3;
 		MK_MEM_ACCESS_BPCACHE(stad)	[r1, #2++], r3;
 		#else
 		iaddu32		r3, r4.low, #1;
 		or		r3, r5, r3;
 		MK_MEM_ACCESS_BPCACHE(stad)	[r1, #1++], r3;
 		#endif
 		
 		/* remove 1 from the page count */
 		isub16		r0, r0, #1;
 		mov	r5, #0xFFFF;
 		and.testz	r0, p0, r0, r5;
 		/* if (!p0) There are still pages left to initialise */
 		!p0	mov		r2, #0xFFFFFFF0;
 		!p0	br	IPBL_SetupNextBlock;	
 		
 		/* If there are no more pages  */
 		ba	IPBL_FinishedAllBlocks;
	}
	IPBL_NoGrowPending:
#endif

	/* We should not get here! */
	#if defined(DEBUG)
	lock;
	lock;
	#endif

	IPBL_WritePBHeadEntry:

	mov		r2, #0xFFFFFFF0;

	/* Offset into the pagetable by the head */
	imae	r1, r4.low, #SIZEOF(IMG_UINT32), r1, u32;

	/* write the first single entry */
	mov	r3, #0xFFFF0000;
#if defined(FIX_HW_BRN_23281)
	iaddu32	r5, r4.low, #2;
	or	r3, r3, r5;
	MK_MEM_ACCESS_BPCACHE(stad)	[r1, #2++], r3;
#else
	iaddu32	r5, r4.low, #1;
	or	r3, r3, r5;
	MK_MEM_ACCESS_BPCACHE(stad)	[r1, #1++], r3;
#endif
	isub16.testz	r0, p0, r0, #1;
	p0	br IPBL_FinishedAllBlocks;
	{
		IPBL_SetupNextBlock:
		{
		#if defined(FIX_HW_BRN_23281)
			/* are there multiples left? */
			/* ((Head-2)<<16) | (head+2) */
			and.testz	p0, r0, r2;
			p0	isub16	r5, r4, #2;
			p0	shl	r5, r5, #16;
			p0	iaddu32	r4, r4.low, #2;
			p0	or	r3, r5, r4;
			p0	br IPBL_SingularPages;
			/* ((Head-16)<<16) | (head-12) */
			isub16	r5, r4, #16;
			shl		r5, r5, #16;
			isub16	r4, r4, #12;
			and		r4, r4, #0xFFFF;
			or		r3, r4, r5;
		#else
			/* are there >= 16 pages, if so do it in blocks */
			and.testz	p0, r0, r2;
			/* ((Head-1)<<16) | (head+1) */
			p0	isub16	r5, r4, #1;
			p0	shl	r5, r5, #16;
			p0	iaddu32	r4, r4.low, #1;
			p0	or	r3, r5, r4;
			p0	br IPBL_SingularPages;
			/* ((Head-16)<<16) | (head-14) */
			isub16	r5, r4, #16;
			shl		r5, r5, #16;
			isub16	r4, r4, #14;
			and		r4, r4, #0xFFFF;
			or		r3, r4, r5;
		#endif
			{
				and		r0, r0, #0xFFFF;
				shr		r2, r0, #4;
				/* Set macro operations to increment dst and src1 */
				smlsi	#1,#0,#1,#0,#0,#0,#0,#0,#0,#0,#0;

			#if defined(FIX_HW_BRN_23281)
				mov		r20, #0x00020002;
				ima16.repeat8		r4, r19, r3, r20;
				ima16.repeat7		r12, r19, r11, r20;
				mov		r20, #0x00200020;
			#else
				mov		r20, #0x00010001;
				ima16.repeat8		r4, r19, r3, r20;
				ima16.repeat7		r12, r19, r11, r20;
				mov		r20, #0x00100010;
			#endif

				IPBL_SetupNextBatch:
				{
					isub16.testz	r2, p0, r2, #1;
					smlsi	#1,#0,#1,#0,#0,#0,#0,#0,#0,#0,#0;
					ima16.repeat8		r3, r19, r3, r20;
					ima16.repeat8		r11, r19, r11, r20;
					smlsi	#1,#0,#0,#1,#0,#0,#0,#0,#0,#0,#0;
			#if defined(FIX_HW_BRN_23281)
					MK_MEM_ACCESS_BPCACHE(stad.repeat16)	[r1, #2++], r3;
			#else
					MK_MEM_ACCESS_BPCACHE(stad.repeat16)	[r1, #1++], r3;
			#endif

					!p0 br			IPBL_SetupNextBatch;
				}
				and.testz	p0, r0, #0xF;
				mov	r3, r18;
				p0 	br	IPBL_LastEntry;
			}
			IPBL_SingularPages:
			{
				and		r0, r0, #0xF;
			#if defined(FIX_HW_BRN_23281)
				mov		r20, #0x00020002;
			#else
				mov		r20, #0x00010001;
			#endif
				IPBL_SetupNextPage:
				{
					isub16.testz	r0, p0, r0, #1;
					ima16			r3, r19, r3, r20;
			#if defined(FIX_HW_BRN_23281)
					MK_MEM_ACCESS_BPCACHE(stad)	[r1, #2++], r3;
			#else
					MK_MEM_ACCESS_BPCACHE(stad)	[r1, #1++], r3;
			#endif
					!p0 br	IPBL_SetupNextPage;
				}
			}
			IPBL_LastEntry:

			/* We have finished this block check if there is another and join them */
			MK_MEM_ACCESS(ldad)	r21, [r21, #DOFFSET(SGXMKIF_HWPBBLOCK.sNextHWPBBlockDevVAddr)], drc0;
			wdf		drc0;
			and.testz	p0, r21, r21;
			p0	br	IPBL_FinishedAllBlocks;
			{
				IPBL_LinkPBBlock:

				/* There is another block to initialise, get the head of the new block */
				MK_MEM_ACCESS(ldaw)	r4, [r21, #WOFFSET(SGXMKIF_HWPBBLOCK.ui16Head)], drc0;
				wdf		drc0;
				/* Make the last entry point to the head of the new block */
				and		r3, r3, ~#0xFFFF;
				or		r3, r3, r4;
		#if defined(FIX_HW_BRN_23281)
				MK_MEM_ACCESS_BPCACHE(stad)	[r1, -#2], r3;
		#else
				MK_MEM_ACCESS_BPCACHE(stad)	[r1, -#1], r3;
		#endif
				/* calculate the address of the first entry in the pagetable for the new block */
				MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sInitHWPBDesc)], drc0;
				wdf		drc0;
				MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWPBDESC.sEVMPageTableDevVAddr)], drc0;
				wdf		drc0;
				/* Offset into the pagetable by the head */
				imae	r1, r4.low, #SIZEOF(IMG_UINT32), r1, u32;

				MK_MEM_ACCESS(ldad)	r0, [r21, #DOFFSET(SGXMKIF_HWPBBLOCK.ui32PageCount)], drc0;
				wdf		drc0;

				/* write the first entry of the new block */
				shr		r3, r3, #16;
		#if defined(FIX_HW_BRN_23281)
				iaddu32		r3, r3.low, #2;
				iaddu32		r5, r4.low, #2;
				shl		r3, r3, #16;
				or		r3, r3, r5;
				MK_MEM_ACCESS_BPCACHE(stad)	[r1, #2++], r3;
		#else
				iaddu32		r3, r3.low, #1;
				iaddu32		r5, r4.low, #1;
				shl		r3, r3, #16;
				or		r3, r3, r5;
				MK_MEM_ACCESS_BPCACHE(stad)	[r1, #1++], r3;
		#endif
				/* remove 1 from the page count */
				isub16		r0, r0, #1;
				mov	r5, #0xFFFF;
				and.testz	r0, p0, r0, r5;
				/* if (!p0) There are still pages left to initialise */
				!p0	mov		r2, #0xFFFFFFF0;
				!p0	br	IPBL_SetupNextBlock;
			}
		}
	}
	IPBL_FinishedAllBlocks:

	/* patch the last one */
	or				r3, r3, #0xffff;
#if defined(FIX_HW_BRN_23281)
	MK_MEM_ACCESS_BPCACHE(stad)	[r1, -#2], r3;
#else
	MK_MEM_ACCESS_BPCACHE(stad)	[r1, -#1], r3;
#endif
	idf		drc0, st;
	wdf		drc0;

	str		#SGX_MP_CORE_SELECT(EUR_CR_USE_GLOBCOM1, 0) >> 2, #1;

	lapc;
}
#endif /* SGX_FAST_DPM_INIT */


#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG)

#if !defined(MKTRACE_CUSTOM_REGISTER)
	#if defined(EUR_CR_TIMER)
		#define MKTRACE_CUSTOM_REGISTER SGX_MP_CORE_SELECT(EUR_CR_CLKGATESTATUS, 0)
	#else
		#define MKTRACE_CUSTOM_REGISTER SGX_MP_CORE_SELECT(EUR_CR_EVENT_TIMER, 0)
	#endif /* EUR_CR_TIMER */
#endif /* MKTRACE_CUSTOM_REGISTER */

.export MKTrace;
/*****************************************************************************
 MKTrace - Write an entry out to the microkernel trace circular buffer.

 inputs:	r16 - status code
			r17 - family mask, matched against ui32MKTraceProfile
 temps:		r18, r19, r20, r21, p2
*****************************************************************************/
MKTrace:
{
	/* Check that one of the families in the mask is enabled in the current profile. */
	MK_LOAD_STATE(r18, ui32MKTraceProfile, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testz	p3, r17, r18;
	p3	ba	MKTrace_End;

	/* Load status buffer base address and offset. */
	MK_LOAD_STATE(r18, sMKTraceBuffer, drc0);
	MK_LOAD_STATE(r19, ui32MKTBOffset, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	MK_MEM_ACCESS_BPCACHE(stad)	[r18, #DOFFSET(SGXMK_TRACE_BUFFER.ui32LatestStatusValue)], r16;

	/* Calculate address of buffer entry to be written. */
	imae			r21, r19.low, #SIZEOF(SGXMK_TRACE_BUFFER.asBuffer[0]), #OFFSET(SGXMK_TRACE_BUFFER.asBuffer), u32;
	iaddu32			r21, r21.low, r18;

	/* Increment CCB write offset. */
	iaddu32			r19, r19.low, #1;
	and				r19, r19, #(SGXMK_TRACE_BUFFER_SIZE - 1);

	/* Save new CCB write offset. */
	MK_MEM_ACCESS_BPCACHE(stad)	[r18, #DOFFSET(SGXMK_TRACE_BUFFER.ui32WriteOffset)], r19;
	MK_STORE_STATE(ui32MKTBOffset, r19);
	MK_WAIT_FOR_STATE_STORE(drc0);

	/* Store status value. */
	MK_MEM_ACCESS_BPCACHE(stad)	[r21, #DOFFSET(SGXMK_TRACE_BUFFER_ENTRY.ui32StatusValue)], r16;

	ENTER_UNMATCHABLE_BLOCK;
	/* Load misc info into r16 and store. */
	ldr				r19, #EUR_CR_BIF_INT_STAT >> 2, drc0;

#if defined(EUR_CR_USE_SERV_EVENT)
	ldr				r20, #EUR_CR_USE_SERV_EVENT >> 2, drc1;
#else
	ldr				r20, #EUR_CR_USE0_SERV_EVENT >> 2, drc1;
#endif

#if defined(EUR_CR_BIF_INT_STAT_FAULT_MASK)
	mov				r16, #EUR_CR_BIF_INT_STAT_FAULT_MASK;
#else
	mov				r16, #EUR_CR_BIF_INT_STAT_FAULT_REQ_MASK;
#endif /* defined(EUR_CR_BIF_INT_STAT_FAULT_MASK) */
	wdf				drc0;
	and.testnz		p3, r19, r16;
	mov				r16, #0;
	p3	or			r16, r16, #SGXMK_TRACE_BUFFER_MISC_FAULT_MASK;
	wdf				drc1;
	shl				r20, r20, #SGXMK_TRACE_BUFFER_MISC_EVENTS_SHIFT;
	or				r16, r16, r20;
	or				r16, r16, G21;
	MK_MEM_ACCESS_BPCACHE(stad)	[r21, #DOFFSET(SGXMK_TRACE_BUFFER_ENTRY.ui32MiscInfo)], r16;

	/* Store timestamp. */
#if defined(EUR_CR_TIMER)
	ldr				r19, #SGX_MP_CORE_SELECT(EUR_CR_TIMER, 0) >> 2, drc1;
#else
	MK_MEM_ACCESS_BPCACHE(ldad)	r19, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32TimeWraps)], drc1;
#endif /* EUR_CR_TIMER */
	ldr				r16, #MKTRACE_CUSTOM_REGISTER >> 2, drc0;
	wdf				drc0;
	MK_MEM_ACCESS_BPCACHE(stad)	[r21, #DOFFSET(SGXMK_TRACE_BUFFER_ENTRY.ui32Custom)], r16;
	wdf				drc1;
	MK_MEM_ACCESS_BPCACHE(stad)	[r21, #DOFFSET(SGXMK_TRACE_BUFFER_ENTRY.ui32TimeStamp)], r19;
	LEAVE_UNMATCHABLE_BLOCK;

#if !defined(PVRSRV_USSE_EDM_STATUS_DEBUG_NOSYNC)
	/*
		Flush the writes to the buffer.
	*/
	idf				drc1, st;
	wdf				drc1;
#endif /* PVRSRV_USSE_EDM_STATUS_DEBUG_NOSYNC */

MKTrace_End:
	lapc;
}
#endif /* PVRSRV_USSE_EDM_STATUS_DEBUG */


/*****************************************************************************
 WaitForReset - Wait for the host to reset SGX

 inputs: none
 temps: N/A
*****************************************************************************/
.export WaitForReset;
WaitForReset:
{
	/*
		Lock the microkernel, in order to protect against further events being
		processed which could be disrupted by the host resetting the chip.
		FIXME: Is there a cleaner way to do this?
	*/
	lock; lock;
}


/*****************************************************************************
 DPMStateClear - Clears the state table only at the address already setup.
		LS bit must be set in EUR_CR_DPM_STATE_CONTEXT_ID before calling.

 temps:		r16
 preds:		p2
*****************************************************************************/
DPMStateClear:
{
	/*
		Clear the DPM state.
	*/
	str		#EUR_CR_DPM_TASK_STATE >> 2, #EUR_CR_DPM_TASK_STATE_CLEAR_MASK;

	/* Wait for the Clear to finish. */
	ENTER_UNMATCHABLE_BLOCK;
	DPMSL_WaitForStateClear:
	{
		ldr		r16, #EUR_CR_EVENT_STATUS >> 2, drc0;
		wdf		drc0;
		and.testz	p2, r16, #EUR_CR_EVENT_STATUS_DPM_STATE_CLEAR_MASK;
		p2	ba		DPMSL_WaitForStateClear;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the store event. */
	str		#EUR_CR_EVENT_HOST_CLEAR >> 2, #EUR_CR_EVENT_HOST_CLEAR_DPM_STATE_CLEAR_MASK;

	/* return */
	lapc;
}

/*****************************************************************************
 DPMStateStore - Stores the state table only to the address already setup.
		LS bit must be set in EUR_CR_DPM_STATE_CONTEXT_ID before calling.

 temps:		r16, r17
 preds:		p2
*****************************************************************************/
DPMStateStore:
{

#if defined(FIX_HW_BRN_28705)
	ldr		r17, #EUR_CR_DPM_LSS_PARTIAL_CONTEXT >> 2, drc0;
	wdf		drc0;
	/* If the partial bit is already set no need to do store twice */
	and.testz	p2, r17, #EUR_CR_DPM_LSS_PARTIAL_CONTEXT_OPERATION_MASK;
	p2	ba	DPMSS_DoPartialStore;
	{
		/* This is a partial store so only do store once */
		mov	r17, #1;
		ba	DPMSS_DoStateStore;
	}
	DPMSS_DoPartialStore:
	{
		/* First do a partial store then do the full state store */
		str	#EUR_CR_DPM_LSS_PARTIAL_CONTEXT >> 2, #EUR_CR_DPM_LSS_PARTIAL_CONTEXT_OPERATION_MASK;
		mov	r17, #2;
	}
	DPMSS_DoStateStore:
#endif
	{
		/* Store the DPM state. */
		str		#EUR_CR_DPM_TASK_STATE >> 2, #EUR_CR_DPM_TASK_STATE_STORE_MASK;
	
		/* Wait for the store to finish. */
		ENTER_UNMATCHABLE_BLOCK;
		DPMSS_WaitForStateStore:
		{
			ldr		r16, #EUR_CR_EVENT_STATUS >> 2, drc0;
			wdf		drc0;
			and.testz	p2, r16, #EUR_CR_EVENT_STATUS_DPM_STATE_STORE_MASK;
			p2	ba		DPMSS_WaitForStateStore;
		}
		LEAVE_UNMATCHABLE_BLOCK;
	
		/* Clear the store event. */
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, #EUR_CR_EVENT_HOST_CLEAR_DPM_STATE_STORE_MASK;
	
#if defined(FIX_HW_BRN_28705)
		/* Decrement the loop count */
		isub16.testz	r17, p2, r17, #1;
		p2	ba	DPMSS_Done;
		{
			/* Now for the partial load */
			str		#EUR_CR_DPM_TASK_STATE >> 2, #EUR_CR_DPM_TASK_STATE_LOAD_MASK;
		
			/* Wait for the store to finish. */
			ENTER_UNMATCHABLE_BLOCK;
			DPMSS_WaitForStateLoad:
			{
				ldr		r16, #EUR_CR_EVENT_STATUS >> 2, drc0;
				wdf		drc0;
				and.testz	p2, r16, #EUR_CR_EVENT_STATUS_DPM_STATE_LOAD_MASK;
				p2	ba		DPMSS_WaitForStateLoad;
			}
			LEAVE_UNMATCHABLE_BLOCK;
		
			/* Clear the store event. */
			str		#EUR_CR_EVENT_HOST_CLEAR >> 2, #EUR_CR_EVENT_HOST_CLEAR_DPM_STATE_LOAD_MASK;
			
			/* Now do the full state store */
			str	#EUR_CR_DPM_LSS_PARTIAL_CONTEXT >> 2, #0;
			ba	DPMSS_DoStateStore;
		}
#endif
	}
	DPMSS_Done:
	
	/* return */
	lapc;
}
/*****************************************************************************
 DPMStateLoad - Loads the state table only from the address already setup.
		LS bit must be set in EUR_CR_DPM_STATE_CONTEXT_ID before calling.

 temps:		r16
 preds:		p2
*****************************************************************************/
DPMStateLoad:
{
	/*
		Store the DPM state.
	*/
	str		#EUR_CR_DPM_TASK_STATE >> 2, #EUR_CR_DPM_TASK_STATE_LOAD_MASK;

	/* Wait for the load to finish. */
	ENTER_UNMATCHABLE_BLOCK;
	DPMSL_WaitForStateLoad:
	{
		ldr		r16, #EUR_CR_EVENT_STATUS >> 2, drc0;
		wdf		drc0;
		and.testz	p2, r16, #EUR_CR_EVENT_STATUS_DPM_STATE_LOAD_MASK;
		p2	ba		DPMSL_WaitForStateLoad;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the store event. */
	str		#EUR_CR_EVENT_HOST_CLEAR >> 2, #EUR_CR_EVENT_HOST_CLEAR_DPM_STATE_LOAD_MASK;

	/* return */
	lapc;
}

/*****************************************************************************
 EmitLoopBack - Emits a loopback to handle an event.

 inputs:	r16	- Data to be emitted back as IR0
 temps:		r16, r17, r18, r19, r20
*****************************************************************************/
EmitLoopBack:
{
	and.testz	p2, r16, r16;
	p2	ba	EmitLoopBack_Return;
	{
		/* If loopback(s) already pending for these events, don't emit another one */
		MK_MEM_ACCESS_BPCACHE(ldad)	r17, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32PendingLoopbacks)], drc0;
		wdf				drc0;
		or				r19, r17, r16;
		xor.testz		p2, r19, r17;
		p2	ba			EmitLoopBack_Return;
		{
			MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32PendingLoopbacks)], r19;
			idf		drc0, st;

#if !defined(SGX545)
			/* FIXME: How does this loop help matters? */
			ENTER_UNMATCHABLE_BLOCK;
			ELB_FifoFull:
			{
				ldr		r19, #EUR_CR_LOOPBACK >> 2, drc1;
				wdf		drc1;
				and.testz	p2, r19, #EUR_CR_LOOPBACK_STATUS_MASK;
				p2	ba		ELB_FifoFull;
			}
			LEAVE_UNMATCHABLE_BLOCK;
#endif
			ELB_PDSConstSize:
			mov	r20, #0xDEADBEEF;
			/* Wait for store to complete */
			wdf		drc0;

			.align 2;
			ELB_PDSDevAddr:
			mov	r18, #0xDEADBEEF;
			/* emit the loop back */
#if !defined(SGX_FEATURE_MULTITHREADED_UKERNEL)
			emitpds.tasks.taske #0, r20, r18, r16, #(EURASIA_PDSSB3_USEDATAMASTER_EVENT |	\
													 EURASIA_PDSSB3_PDSSEQDEPENDENCY | \
	                                                 (1 << EURASIA_PDSSB3_USEINSTANCECOUNT_SHIFT));
#else
			emitpds.tasks.taske #0, r20, r18, r16, #((EURASIA_PDSSB3_USEDATAMASTER_EVENT << EURASIA_PDSSB3_USEDATAMASTER_SHIFT) | \
	                                                 (1 << EURASIA_PDSSB3_USEINSTANCECOUNT_SHIFT));
#endif
		}
	}
	EmitLoopBack_Return:
	/* return */
	lapc;
}

#if 1 // SPM_PAGE_INJECTION
/*****************************************************************************
 DPMPauseAllocations - Pause DPM allocations.

 inputs:	none
 temps:		r16, r17
*****************************************************************************/
.export DPMPauseAllocations;
DPMPauseAllocations:
{
	/*
		Disable the vertex data master to pause TA allocations.
	*/
	ldr				r16, #EUR_CR_DMS_CTRL >> 2, drc0;
	wdf				drc0;
	or				r16, r16, #EUR_CR_DMS_CTRL_DISABLE_DM_VERTEX_MASK;
	str				#EUR_CR_DMS_CTRL >> 2, r16;

	/*
		Wait for the head/tail of the free list to stop changing.
	*/
	DPMPA_FlushAllocs:
	{
		ldr				r16, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_STATUS1 >> 2, drc0;
		wdf				drc0;
		mov				r17, #400;
		DPMPA_Delay:
		{
			isubu16.testnz	r17, p2, r17, #1;
			p2	ba	DPMPA_Delay;
		}
		ldr				r17, #EUR_CR_DPM_TA_ALLOC_FREE_LIST_STATUS1 >> 2, drc0;
		wdf				drc0;
		xor.testnz		p2, r17, r16;
		p2	ba		DPMPA_FlushAllocs;
	}
	/* return */
	lapc;
}

/*****************************************************************************
 DPMResumeAllocations - Resume DPM allocations.

 inputs:	none
 temps:		r16
*****************************************************************************/
.export DPMResumeAllocations;
DPMResumeAllocations:
{
	/*
		Reenable the vertex data master.
	*/
	ldr		r16, #EUR_CR_DMS_CTRL >> 2, drc0;
	wdf		drc0;
	and		r16, r16, ~#EUR_CR_DMS_CTRL_DISABLE_DM_VERTEX_MASK;
	str		#EUR_CR_DMS_CTRL >> 2, r16;

	lapc;
}
#endif

#if defined(FIX_HW_BRN_23862)
.export FixBRN23862;
/*****************************************************************************
 FixBRN23862 routine	- Apply the BRN23862 to the region headers for a
						render about to be kicked.

 inputs:	r9 - Render details structure.
			r10	- Macrotile to be rendered - 0x10 if all are being rendered.
			R_RTData - RTData structure for the render
 temps:		p2, r16, r17, r18
*****************************************************************************/
FixBRN23862:
{
	/*
		Check if process empty region is set - if so the bug doesn't apply.
	*/
	ldr		r16, #EUR_CR_ISP_IPFMISC >> 2, drc0;
	wdf		drc0;
	and		r16, r16, #EUR_CR_ISP_IPFMISC_PROCESSEMPTY_MASK;
	or.testnz	p2, r16, #0;
	p2	ba		FixBRN23862_ProcessEmptyRegionsSet;
	{
		/*
			Check if we are rendering all macrotiles.
		*/
		and.testnz	p2, r10, #EUR_CR_DPM_ABORT_STATUS_MTILE_GLOBAL_MASK;
		p2	ba		FixBRN23862_AllMTiles;
		{
			/*
				Load the address of the lookup table for last regions.
			*/
			MK_MEM_ACCESS_BPCACHE(ldad)	r16, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sLastRgnLUTDevAddr)], drc0;
			MK_MEM_ACCESS_BPCACHE(ldad)	r17, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sRegionArrayDevAddr)], drc1;
			wdf		drc0;

			/*
				Load the address of the last region for the aborted macrotile.
			*/
			and		r10, r10, #EUR_CR_DPM_ABORT_STATUS_MTILE_INDEX_MASK;
			imae	r10, r10.low, #4, r16, u32;
			MK_MEM_ACCESS_CACHED(ldad)	r18, [r10], drc1;
			wdf		drc1;

			IADD32(r10, r18, r17, r16);
			and		r10, r10, #EUR_CR_ISP_RGN_BASE_ADDR_MASK;

			ba		FixBRN23862_CheckRegion;
		}
		FixBRN23862_AllMTiles:
		{
			/*
			   Load the address of the last region for the whole render target.
			 */
			MK_MEM_ACCESS(ldad)	r10, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sLastRegionDevAddr)], drc0;
			wdf		drc0;
		}
		FixBRN23862_CheckRegion:
		/*
			Load the region header for the last region.
		*/
		MK_MEM_ACCESS_CACHED(ldad)	r16, [r10, #0], drc0;
		wdf		drc0;

		/*
			Check if it's empty.
		*/
		and		r17, r16, #EURASIA_REGIONHEADER0_EMPTY;
		or.testz	p0, r17, #0;
		p0	ba		FixBRN23862_LastRegionNotEmpty;
		{
			/*
				Clear the empty flag and store back into the region array.
			*/
			and		r16, r16, #~EURASIA_REGIONHEADER0_EMPTY;
			MK_MEM_ACCESS_BPCACHE(stad)	[r10, #0], r16;
			idf		drc0, st;
			wdf		drc0;

			/*
				Load the address of the control stream for the BRN23862 object.
			*/
			MK_MEM_ACCESS(ldad)	r16, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sBRN23862FixObjectDevAddr)], drc0;
			wdf		drc0;
			/*
				Store the control stream into the region array.
			*/
			MK_MEM_ACCESS_BPCACHE(stad)	[r10, #1], r16;

			/*
				Load the location to update with the PDS state.
			*/
			MK_MEM_ACCESS(ldad)	r10, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sBRN23862PDSStateDevAddr)], drc0;
			/*
				Copy the PDS state from the render details into the parameter buffer.
			*/
			MK_MEM_ACCESS(ldad.f3)	r16, [r9, #DOFFSET(SGXMKIF_HWRENDERDETAILS.aui32SpecObject)-1], drc0;
			wdf		drc0;
			MK_MEM_ACCESS_BPCACHE(stad.f3)	[r10, #0++], r16;
			idf		drc0, st;
			wdf		drc0;
		}
		FixBRN23862_LastRegionNotEmpty:
	}
	FixBRN23862_ProcessEmptyRegionsSet:
	lapc;
}
#endif /* defined(FIX_HW_BRN_23862) */

/*****************************************************************************
 StoreDPMContext routine - store the specified context data

 inputs:	r16 - RTData address
			r17 - context ID
			r18 - bif requestor (0 - TA / 1 - 3D)
 temps:		r19, r20
			p2
*****************************************************************************/
StoreDPMContext:
{
#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	/* Set DPM 3D/TA indicator bit appropriately */
	ldr 	r19, #EUR_CR_BIF_BANK_SET >> 2, drc0;
	wdf		drc0;
	and 	r19, r19, ~#USE_DPM_3D_TA_INDICATOR_MASK;	// clear bit 9 of the register
	or		r19, r18, r19;
	str		#EUR_CR_BIF_BANK_SET >> 2, r19;
#endif

	/* Load the ui32CommonStatus, to see what we need to store */
	MK_MEM_ACCESS(ldad)	r18, [r16, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc1;
	/* Wait for the ui32CommonStatus to load */
	wdf		drc1;

	/* Set load/store context ID */
	ldr		r19, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc0;
	wdf		drc0;
#if defined(EUR_CR_DPM_STATE_CONTEXT_ID_NCOP_MASK)
	/* Clear the LS and NCOP bits */
	and		r19, r19, ~#(EUR_CR_DPM_STATE_CONTEXT_ID_LS_MASK | \
							EUR_CR_DPM_STATE_CONTEXT_ID_NCOP_MASK);
	/* Set the NCOP bit if we are loading an incomplete TA */
	and.testz p2, r18, #SGXMKIF_HWRTDATA_CMN_STATUS_TACOMPLETE;
	p2	or		r19, r19, #EUR_CR_DPM_STATE_CONTEXT_ID_NCOP_MASK;
#else
	/* Clear the LS bit */
	and		r19, r19, ~#EUR_CR_DPM_STATE_CONTEXT_ID_LS_MASK;
#endif
	/* set the LS bit according to the selected ID */
	or		r19, r17, r19;
	str		#EUR_CR_DPM_STATE_CONTEXT_ID >> 2, r19;

	/* Save context state  */
	MK_MEM_ACCESS(ldad)	r19, [r16, #DOFFSET(SGXMKIF_HWRTDATA.sContextStateDevAddr)], drc0;
	wdf		drc0;
	SETUPSTATETABLEBASE(SC, p2, r19, r17);

#if defined(FIX_HW_BRN_28705)
	/* First do a partial store then do the full state store */
	str		#EUR_CR_DPM_LSS_PARTIAL_CONTEXT >> 2, #EUR_CR_DPM_LSS_PARTIAL_CONTEXT_OPERATION_MASK;
	mov		r19, #2;
#endif
	SC_DoStateStore:
	{
		/* Do the state store */
		str		#EUR_CR_DPM_TASK_STATE >> 2, #EUR_CR_DPM_TASK_STATE_STORE_MASK;
	
		/* Wait for the store to finish. */
		ENTER_UNMATCHABLE_BLOCK;
		SC_WaitForStateStore:
		{
			ldr		r17, #EUR_CR_EVENT_STATUS >> 2, drc0;
			wdf		drc0;
			and.testz	p2, r17, #EUR_CR_EVENT_STATUS_DPM_STATE_STORE_MASK;
			p2	ba		SC_WaitForStateStore;
		}
		LEAVE_UNMATCHABLE_BLOCK;
	
		/* Clear the store event. */
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, #EUR_CR_EVENT_HOST_CLEAR_DPM_STATE_STORE_MASK;

#if defined(FIX_HW_BRN_28705)
		/* Decrement the loop count */
		isub16.testz	r19, p2, r19, #1;
		p2	ba	SC_StateStoreDone;
		{
			/* Now do a partial state load */
			str		#EUR_CR_DPM_TASK_STATE >> 2, #EUR_CR_DPM_TASK_STATE_LOAD_MASK;
		
			/* Wait for the store to finish. */
			ENTER_UNMATCHABLE_BLOCK;
			SC_WaitForStateLoad:
			{
				ldr		r16, #EUR_CR_EVENT_STATUS >> 2, drc0;
				wdf		drc0;
				and.testz	p2, r16, #EUR_CR_EVENT_STATUS_DPM_STATE_LOAD_MASK;
				p2	ba		SC_WaitForStateLoad;
			}
			LEAVE_UNMATCHABLE_BLOCK;
		
			/* Clear the store event. */
			str		#EUR_CR_EVENT_HOST_CLEAR >> 2, #EUR_CR_EVENT_HOST_CLEAR_DPM_STATE_LOAD_MASK;
			
			/* Now do the full state store */
			str		#EUR_CR_DPM_LSS_PARTIAL_CONTEXT >> 2, #0;
			ba		SC_DoStateStore;
		}
#endif
	}
	SC_StateStoreDone:

	/* if the scene has been completed by the TA then we only need the state table */
	and.testnz p2, r18, #SGXMKIF_HWRTDATA_CMN_STATUS_TACOMPLETE;
	/* Only flush control, OTPM and tail pointers if doing full store */
	p2	ba		SC_PartialStore;
	{
		/* Save the context control */
		mov				r19, #OFFSET(SGXMKIF_HWRTDATA.sContextControlDevAddr[0]);
		iaddu32			r17, r19.low, r16;
		COPYMEMTOCONFIGREGISTER_TA(SC_PS_ControlTableBase, EUR_CR_DPM_CONTROL_TABLE_BASE, r17, r18, r19, r20);

		str		#EUR_CR_DPM_TASK_CONTROL >> 2, #EUR_CR_DPM_TASK_CONTROL_STORE_MASK;

		mov				r19, #OFFSET(SGXMKIF_HWRTDATA.sContextOTPMDevAddr[0]);
		iaddu32			r17, r19.low, r16;
	#if defined(EUR_CR_MTE_OTPM_CSM_BASE)
		/* Flush and Invalidate OTPM */
		COPYMEMTOCONFIGREGISTER_TA(SC_PS_OTPMCSBase, EUR_CR_MTE_OTPM_CSM_BASE, r17, r18, r19, r20);
		str		#EUR_CR_MTE_OTPM_OP >> 2, #EUR_CR_MTE_OTPM_OP_CSM_FLUSH_INV_MASK;
	#else /* defined(EUR_CR_MTE_OTPM_CSM_BASE) */
		/* Flush OTPM */
		COPYMEMTOCONFIGREGISTER_TA(SC_PS_OTPMCSBase, EUR_CR_MTE_OTPM_CSM_FLUSH_BASE, r17, r18, r19, r20);
		str		#EUR_CR_MTE_OTPM_OP >> 2, #EUR_CR_MTE_OTPM_OP_CSM_FLUSH_MASK;
	#endif /* defined(EUR_CR_MTE_OTPM_CSM_BASE) */

		/* Flush tail pointer cache */
		mov				r19, #OFFSET(SGXMKIF_HWRTDATA.asTailPtrDevAddr[0]);
		iaddu32			r17, r19.low, r16;
		COPYMEMTOCONFIGREGISTER_TA(SC_PS_TETPCBase, EUR_CR_TE_TPC_BASE, r17, r18, r19, r20);
		mov				r17, #EUR_CR_TE_TPCCONTROL_FLUSH_MASK;
		str				#EUR_CR_TE_TPCCONTROL >> 2, r17;

		/* Wait for all the events to be flagged complete */
#if defined(EUR_CR_EVENT_STATUS2_OTPM_FLUSHED_INV_MASK)
		mov		r17, #(EUR_CR_EVENT_STATUS_TPC_FLUSH_MASK | \
				       EUR_CR_EVENT_STATUS_DPM_CONTROL_STORE_MASK);
#else
		mov		r17, #(EUR_CR_EVENT_STATUS_TPC_FLUSH_MASK | \
				       EUR_CR_EVENT_STATUS_OTPM_FLUSHED_MASK | \
				       EUR_CR_EVENT_STATUS_DPM_CONTROL_STORE_MASK);
#endif /* EUR_CR_EVENT_STATUS2_OTPM_FLUSHED_INV_MASK */

		ENTER_UNMATCHABLE_BLOCK;
		SC_WaitForContextStore:
		{
			ldr		r18, #EUR_CR_EVENT_STATUS >> 2, drc0;
			wdf		drc0;
			and		r18, r18, r17;
			xor.testnz	p2, r18, r17;
			p2	ba		SC_WaitForContextStore;
		}
		LEAVE_UNMATCHABLE_BLOCK;

#if defined(EUR_CR_EVENT_STATUS2_OTPM_FLUSHED_INV_MASK)
		mov		r17, #(EUR_CR_EVENT_HOST_CLEAR_TPC_FLUSH_MASK | \
				       	EUR_CR_EVENT_HOST_CLEAR_DPM_CONTROL_STORE_MASK);
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r17;

		/* Wait for the event to be flagged complete */
		mov		r17, #EUR_CR_EVENT_STATUS2_OTPM_FLUSHED_INV_MASK;
		ENTER_UNMATCHABLE_BLOCK;
		SC_WaitForOPTM:
		{
			ldr		r18, #EUR_CR_EVENT_STATUS2 >> 2, drc0;
			wdf		drc0;
			and.testz	p2, r18, r17;
			p2	ba		SC_WaitForOPTM;
		}
		LEAVE_UNMATCHABLE_BLOCK;
		mov		r17, #EUR_CR_EVENT_HOST_CLEAR2_OTPM_FLUSHED_INV_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r17;
#else
		mov		r17, #(EUR_CR_EVENT_HOST_CLEAR_OTPM_FLUSHED_MASK | \
				       EUR_CR_EVENT_HOST_CLEAR_TPC_FLUSH_MASK | \
				       EUR_CR_EVENT_HOST_CLEAR_DPM_CONTROL_STORE_MASK);
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r17;

#endif /* EUR_CR_EVENT_STATUS2_OTPM_FLUSHED_INV_MASK */

#if defined(FIX_HW_BRN_31543)
					str		#EUR_CR_DPM_TASK_CONTROL >> 2, #EUR_CR_DPM_TASK_CONTROL_LOAD_MASK;

					mov		r17, #(EUR_CR_EVENT_STATUS_DPM_CONTROL_LOAD_MASK);

					ENTER_UNMATCHABLE_BLOCK;
					SC_WaitForDPMLoad:
					{
						ldr		r18, #EUR_CR_EVENT_STATUS >> 2, drc0;
						wdf		drc0;
						and		r18, r18, r17;
						xor.testnz	p0, r18, r17;
						p0	ba		SC_WaitForDPMLoad;
					}
					LEAVE_UNMATCHABLE_BLOCK;

					/* Bits are in the same position for host_clear and event_status */
					str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r17;
#endif	

#if defined(SGX_FEATURE_MP)
	#if defined(EUR_CR_MASTER_DPM_MTILE_PARTI_PIM_TABLE_BASE_ADDR)
		MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWRTDATA.sPIMTableDevAddr)], drc0; 
		wdf		drc0;
		WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_MTILE_PARTI_PIM_TABLE_BASE_ADDR >> 2, r17, r18);
		WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_TASK_DPLIST >> 2, #EUR_CR_MASTER_DPM_TASK_DPLIST_STORE_MASK, r17);
		
		/* Poll for the DPM PIM cleared event. */
		ENTER_UNMATCHABLE_BLOCK;
		SC_WaitForDPMPIMStore:
		{
			READMASTERCONFIG(r17, #EUR_CR_MASTER_DPM_DPLIST_EVENT_STATUS >> 2, drc0);
			wdf			drc0;
			and.testz	p2, r17, #EUR_CR_MASTER_DPM_DPLIST_EVENT_STATUS_STORED_MASK;
			p2	ba		SC_WaitForDPMPIMStore;
		}
		LEAVE_UNMATCHABLE_BLOCK;		
		
		WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_DPLIST_CLEAR_EVENT >> 2, #EUR_CR_MASTER_DPM_DPLIST_CLEAR_EVENT_STORE_MASK, r17);
	#endif
#endif
	}
	SC_PartialStore:

	/* branch back to calling code */
	lapc;
}

#if defined(FIX_HW_BRN_33657) && defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)

.export StoreDPMContextPIMUpdate;

/*****************************************************************************
 StoreDPMContextPIMUpdate - patch the partial PIMs in memory after
 VDM context store

 inputs:	r16 - RT data to patch
 temps:		r17, r18, r19, r20, r21
 			p2
*****************************************************************************/
StoreDPMContextPIMUpdate:
{
	MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWRTDATA.sPIMTableDevAddr)], drc0; 
	MK_MEM_ACCESS(ldad)	r18, [r16, #DOFFSET(SGXMKIF_HWRTDATA.ui32VDMPIM)], drc0; 
	mov			r20, #1;
	wdf			drc0;
	
	/* Patch dplist starting value */
	or			r19, r18, #(4 << EUR_CR_MASTER_DPM_START_OF_NCPIM0_ACTIVE_SHIFT);
	MK_MEM_ACCESS(stad)	[r17, #40], r19;

    MK_LOAD_STATE_CORE_COUNT_TA(r21, drc0);
    MK_WAIT_FOR_STATE_LOAD(drc0);
    isub16   r21, r21, #1;

	/* Always at least 1 core */
    {
        /* core0 = N */
        or                      r19, r18, #(5 << EUR_CR_MASTER_DPM_REQUESTOR0_PIM_STATUS_ACTIVE_SHIFT);
        MK_MEM_ACCESS(stad)     [r17, #36], r19;        /* TE */
        or                      r19, r18, #(7 << EUR_CR_MASTER_DPM_REQUESTOR0_PIM_STATUS_ACTIVE_SHIFT);
        MK_MEM_ACCESS(stad)     [r17, #32], r19;        /* MTE */

        isub16.tests   r21, p2, r21, #1;
        p2      ba      PIMU_AllCoresDone;
        {
            /* core1 = N+1 */
            iaddu32         r18, r20.low, r18;
            or                      r19, r18, #(5 << EUR_CR_MASTER_DPM_REQUESTOR1_PIM_STATUS_ACTIVE_SHIFT);
            MK_MEM_ACCESS(stad)     [r17, #37], r19;
            or                      r19, r18, #(7 << EUR_CR_MASTER_DPM_REQUESTOR1_PIM_STATUS_ACTIVE_SHIFT);
            MK_MEM_ACCESS(stad)     [r17, #33], r19;
            MK_MEM_ACCESS(stad)     [r17, #23], r19;        /* non-committed PIM */

            isub16.tests   r21, p2, r21, #1;
            p2      ba      PIMU_AllCoresDone;
            {
                /* core2 = N+2 */
                iaddu32         r18, r20.low, r18;
                or                      r19, r18, #(5 << EUR_CR_MASTER_DPM_REQUESTOR2_PIM_STATUS_ACTIVE_SHIFT);
                MK_MEM_ACCESS(stad)     [r17, #38], r19;
                or                      r19, r18, #(7 << EUR_CR_MASTER_DPM_REQUESTOR2_PIM_STATUS_ACTIVE_SHIFT);
                MK_MEM_ACCESS(stad)     [r17, #34], r19;
                MK_MEM_ACCESS(stad)     [r17, #26], r19;        /* non-committed PIM */

                isub16.tests   r21, p2, r21, #1;
                p2      ba      PIMU_AllCoresDone;
                {
                    /* core3 = N+3 */
                    iaddu32         r18, r20.low, r18;
                    or                      r19, r18, #(5 << EUR_CR_MASTER_DPM_REQUESTOR3_PIM_STATUS_ACTIVE_SHIFT);
                    MK_MEM_ACCESS(stad)     [r17, #39], r19;
                    or                      r19, r18, #(7 << EUR_CR_MASTER_DPM_REQUESTOR3_PIM_STATUS_ACTIVE_SHIFT);
                    MK_MEM_ACCESS(stad)     [r17, #35], r19;
                    MK_MEM_ACCESS(stad)     [r17, #29], r19;        /* non-committed PIM */
                }
        	}
        }
    }
	PIMU_AllCoresDone:

	/* Fence the stores (12 dwords) */			
	idf		drc0, st;
	wdf		drc0;

	lapc;
}
#endif

/*****************************************************************************
 LoadDPMContext routine - load the specified context data

 inputs:	r16 - RTData address
			r17 - context ID
			r18 - bif requestor (0 - TA / 1 - 3D)
 temps:		r19, r20
			p2
*****************************************************************************/
LoadDPMContext:
{
#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	/* Set DPM 3D/TA indicator bit to TA */
	ldr 	r19, #EUR_CR_BIF_BANK_SET >> 2, drc0;
	wdf		drc0;
	and 	r19, r19, ~#USE_DPM_3D_TA_INDICATOR_MASK;
	or		r19, r18, r19;
	str		#EUR_CR_BIF_BANK_SET >> 2, r19;
#endif

	/* Load the ui32CommonStatus, to see what needs to be loaded */
	MK_MEM_ACCESS(ldad)	r18, [r16, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc1;
	/* Wait for the ui32CommonStatus to load */
	wdf		drc1;

	/* Set load/store context ID */
	ldr		r19, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc0;
	wdf		drc0;
#if defined(EUR_CR_DPM_STATE_CONTEXT_ID_NCOP_MASK)
	/* Clear the LS and NCOP bits */
	and		r19, r19, ~#(EUR_CR_DPM_STATE_CONTEXT_ID_LS_MASK | \
							EUR_CR_DPM_STATE_CONTEXT_ID_NCOP_MASK);
	/* Set the NCOP bit if we are loading an incomplete TA */
	and.testz p2, r18, #SGXMKIF_HWRTDATA_CMN_STATUS_TACOMPLETE;
	p2	or		r19, r19, #EUR_CR_DPM_STATE_CONTEXT_ID_NCOP_MASK;
#else
	/* Clear the LS bit */
	and		r19, r19, ~#EUR_CR_DPM_STATE_CONTEXT_ID_LS_MASK;
#endif
	or		r19, r17, r19;
	str		#EUR_CR_DPM_STATE_CONTEXT_ID >> 2, r19;

	/* Load context state */
	MK_MEM_ACCESS(ldad)	r19, [r16, #DOFFSET(SGXMKIF_HWRTDATA.sContextStateDevAddr)], drc0;
	wdf		drc0;
	SETUPSTATETABLEBASE(LC, p2, r19, r17);

	/* Load the context */
	str		#EUR_CR_DPM_TASK_STATE >> 2, #EUR_CR_DPM_TASK_STATE_LOAD_MASK;

	/* Wait for the load to finish. */
	ENTER_UNMATCHABLE_BLOCK;
	LC_WaitForStateLoad:
	{
		ldr		r17, #EUR_CR_EVENT_STATUS >> 2, drc0;
		wdf		drc0;
		and.testz	p2, r17, #EUR_CR_EVENT_STATUS_DPM_STATE_LOAD_MASK;
		p2	ba		LC_WaitForStateLoad;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the store event. */
	str		#EUR_CR_EVENT_HOST_CLEAR >> 2, #EUR_CR_EVENT_HOST_CLEAR_DPM_STATE_LOAD_MASK;

	/* if the scene has been completed by the TA then we only need the state table */
	and.testnz p2, r18, #SGXMKIF_HWRTDATA_CMN_STATUS_TACOMPLETE;
	/* Only flush control, OTPM and tail pointers if doing full store */
	p2	ba		LC_PartialLoad;
	{
		#if defined(SGX_FEATURE_MP)
		mov				r19, #OFFSET(SGXMKIF_HWRTDATA.sContextControlDevAddr[0]);
		iaddu32			r17, r19.low, r16;
		COPYMEMTOCONFIGREGISTER_TA(LC_PL_ControlTableBase, EUR_CR_DPM_CONTROL_TABLE_BASE, r17, r18, r19, r20);
		#else
		/* FIXME - use common path above when ready */
		MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWRTDATA.sContextControlDevAddr)], drc0;
		wdf		drc0;
		str		#EUR_CR_DPM_CONTROL_TABLE_BASE >> 2, r17;
		#endif /* SGX_FEATURE_MP */

		str		#EUR_CR_DPM_TASK_CONTROL >> 2, #EUR_CR_DPM_TASK_CONTROL_LOAD_MASK;

		mov				r19, #OFFSET(SGXMKIF_HWRTDATA.sContextOTPMDevAddr[0]);
		iaddu32			r17, r19.low, r16;
	#if defined(EUR_CR_MTE_OTPM_CSM_BASE)
		/* Setup the OTPM CSM Base */
		COPYMEMTOCONFIGREGISTER_TA(LC_PL_OTPMCSBase, EUR_CR_MTE_OTPM_CSM_BASE, r17, r18, r19, r20);
		str		#EUR_CR_MTE_OTPM_OP >> 2, #EUR_CR_MTE_OTPM_OP_CSM_INV_MASK;
		/* Wait for all the loads to be flagged complete */
		mov		r17, #(EUR_CR_EVENT_STATUS_OTPM_INV_MASK | \
				       EUR_CR_EVENT_STATUS_DPM_CONTROL_LOAD_MASK);
	#else /* #if defined(EUR_CR_MTE_OTPM_CSM_BASE) */
		/* Load OTPM */
		COPYMEMTOCONFIGREGISTER_TA(LC_PL_OTPMCSBase, EUR_CR_MTE_OTPM_CSM_LOAD_BASE, r17, r18, r19, r20);
		str		#EUR_CR_MTE_OTPM_OP >> 2, #EUR_CR_MTE_OTPM_OP_CSM_LOAD_MASK;

		/* Wait for all the events to be flagged complete */
		mov		r17, #(EUR_CR_EVENT_STATUS_OTPM_LOADED_MASK | \
				       EUR_CR_EVENT_STATUS_DPM_CONTROL_LOAD_MASK);
	#endif /* #if defined(EUR_CR_MTE_OTPM_CSM_BASE) */

		ENTER_UNMATCHABLE_BLOCK;
		LC_WaitForContextLoad:
		{
			ldr		r18, #EUR_CR_EVENT_STATUS >> 2, drc0;
			wdf		drc0;
			and		r18, r18, r17;
			xor.testnz	p2, r18, r17;
			p2	ba		LC_WaitForContextLoad;
		}
		LEAVE_UNMATCHABLE_BLOCK;

		#if defined(EUR_CR_EVENT_HOST_CLEAR_OTPM_LOADED_MASK)
		mov		r17, #(EUR_CR_EVENT_HOST_CLEAR_OTPM_LOADED_MASK | \
				       EUR_CR_EVENT_HOST_CLEAR_DPM_CONTROL_LOAD_MASK);
		#else /* #if defined(EUR_CR_EVENT_HOST_CLEAR_OTPM_LOADED_MASK) */
		mov		r17, #(EUR_CR_EVENT_HOST_CLEAR_OTPM_INV_MASK | \
					EUR_CR_EVENT_HOST_CLEAR_DPM_CONTROL_LOAD_MASK);
		#endif /* #if defined(EUR_CR_EVENT_HOST_CLEAR_OTPM_LOADED_MASK) */
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r17;

#if defined(SGX_FEATURE_MP)
	#if defined(EUR_CR_MASTER_DPM_MTILE_PARTI_PIM_TABLE_BASE_ADDR)
		MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWRTDATA.sPIMTableDevAddr)], drc0; 
		wdf		drc0;
		WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_MTILE_PARTI_PIM_TABLE_BASE_ADDR >> 2, r17, r18);
	#endif
		
		WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_TASK_DPLIST >> 2, #EUR_CR_MASTER_DPM_TASK_DPLIST_LOAD_MASK, r17);
		
		/* Poll for the DPM PIM cleared event. */
		ENTER_UNMATCHABLE_BLOCK;
		LC_WaitForDPMPIMLoad:
		{
			READMASTERCONFIG(r17, #EUR_CR_MASTER_DPM_DPLIST_EVENT_STATUS >> 2, drc0);
			wdf			drc0;
			and.testz	p2, r17, #EUR_CR_MASTER_DPM_DPLIST_EVENT_STATUS_LOADED_MASK;
			p2	ba		LC_WaitForDPMPIMLoad;
		}
		LEAVE_UNMATCHABLE_BLOCK;		
		
		WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_DPLIST_CLEAR_EVENT >> 2, #EUR_CR_MASTER_DPM_DPLIST_CLEAR_EVENT_LOAD_MASK, r17);
	#if defined(EUR_CR_MASTER_DPM_MTILE_ABORTED) && defined(EUR_CR_DPM_MTILE_ABORTED)
		READMASTERCONFIG(r3, #EUR_CR_MASTER_DPM_MTILE_ABORTED >> 2, drc0);
		wdf		drc0;
		str		#EUR_CR_DPM_MTILE_ABORTED >> 2, r3;
	#endif
#endif
	}
	LC_PartialLoad:

	/* branch back to calling code */
	lapc;
}

#if (defined(FIX_HW_BRN_24304) || defined(FIX_HW_BRN_27510)) && defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
/*****************************************************************************
 ConfigureCache - 	This routine checks which contexts are running and configs
 					the cache partitions for the best balance between performance
 					and reliabilty

 inputs:	r16 - 0 = TA Kick, 1= 3D Kick, 2 = HW Finish (reset partitions)
 temps:		r17, r18, r19
*****************************************************************************/
.export ConfigureCache;
ConfigureCache:
{
	mov	r17, #(EUR_CR_CACHE_CTRL_PARTDM0_MASK | EUR_CR_CACHE_CTRL_PARTDM1_MASK);
	/* If it is a finished event, reset partitions */
	xor.testz	p2, r16, #2;
	p2	ba	CC_CheckValue;
	{
		ldr		r18, #EUR_CR_BIF_BANK0 >> 2, drc0;
		wdf		drc0;
		and		r19, r18, #EUR_CR_BIF_BANK0_INDEX_3D_MASK;
		shr		r19, r19, #(EUR_CR_BIF_BANK0_INDEX_3D_SHIFT - EUR_CR_BIF_BANK0_INDEX_TA_SHIFT);
		and		r18, r18, #EUR_CR_BIF_BANK0_INDEX_TA_MASK;
		xor.testz	p2, r18, r19;
		/* if the 3D is on the same memory context just exit */
		p2	ba	CC_Exit;

		/* We are about to kick job on the HW work out if the other half is busy */
		xor.testz	p2, r16, #1;
		p2	ba	CC_Kick3D;
		{
			MK_LOAD_STATE(r18, ui32IRenderFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			and.testnz	p2, r18, #RENDER_IFLAGS_RENDERINPROGRESS;
			p2	mov	r17, #((0x1 << EUR_CR_CACHE_CTRL_PARTDM0_SHIFT) | \
							(0xE << EUR_CR_CACHE_CTRL_PARTDM1_SHIFT));
			p2	ba	CC_CheckValue;
			/* if we get to here, the 3D is inactive and TA will already have all partitions */
			ba	CC_Exit;
		}
		CC_Kick3D:
		{
			MK_LOAD_STATE(r18, ui32ITAFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			and.testnz	p2, r18, #TA_IFLAGS_TAINPROGRESS;
			p2	mov	r17, #((0x1 << EUR_CR_CACHE_CTRL_PARTDM0_SHIFT) | \
							(0xE << EUR_CR_CACHE_CTRL_PARTDM1_SHIFT));
			p2	ba	CC_CheckValue;
			/* if we get to here, the TA is inactive and 3D will already have all partitions */
			ba	CC_Exit;
		}
	}
	CC_CheckValue:

	ldr		r18, #EUR_CR_CACHE_CTRL >> 2, drc0;
	wdf		drc0;
	and		r19, r18, #(EUR_CR_CACHE_CTRL_PARTDM0_MASK | EUR_CR_CACHE_CTRL_PARTDM1_MASK);
	and		r18, r18, ~#(EUR_CR_CACHE_CTRL_PARTDM0_MASK | EUR_CR_CACHE_CTRL_PARTDM1_MASK);
	/* if it the same as we have just exit */
	xor.testz	p2, r17, r19;
	p2	ba	CC_Exit;
	{
		or		r18, r18, #EUR_CR_CACHE_CTRL_INVALIDATE_MASK;
		or		r18, r18, r17;
		str		#EUR_CR_CACHE_CTRL >> 2, r18;

		mov 	r17, #EUR_CR_EVENT_STATUS_MADD_CACHE_INVALCOMPLETE_MASK;
		ENTER_UNMATCHABLE_BLOCK;
		CC_WaitForMADDInvalidate:
		{
			ldr		r18, #EUR_CR_EVENT_STATUS >> 2, drc0;
			wdf		drc0;
			and.testz	p2, r18, r17;
			p2	ba		CC_WaitForMADDInvalidate;
		}
		LEAVE_UNMATCHABLE_BLOCK;

		mov		r17, #EUR_CR_EVENT_HOST_CLEAR_MADD_CACHE_INVALCOMPLETE_MASK;
		str		#(EUR_CR_EVENT_HOST_CLEAR >> 2), r17;
	}
	CC_Exit:
	/* branch back to calling code */
	lapc;
}
#endif

/*****************************************************************************
 SetupDirListBase - Set up a directory list base for either TA or 3D requestor.

 inputs:	r16 = HWRenderContext structure for the application
			r17 = Setup TA/3D/2D/EDM requestor, allowed values are:
				SETUP_BIF_TA_REQ = TA
				SETUP_BIF_3D_REQ = 3D
				SETUP_BIF_TQ_REQ = TQ
				SETUP_BIF_2D_REQ = 2D only if 2D hardware present
				SETUP_BIF_EDM_REQ = EDM only if SUPPORT_SGX_EDM_MEMORY_DEBUG enabled.
 temps:		r18, r19, r20, r21, p0, p1, p3
			p1, p3 are permanent state values (assigned once)

if(SUPPORT_SGX_EDM_MEMORY_DEBUG)
Additional to those above.
 inputs:	r5  = PDDevPhysAddr if switching EDM to new memory context
 			r16 = 0 in this case
 temps:		r22
*****************************************************************************/
#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
SetupDirListBase:
{
	#if defined(SUPPORT_SGX_EDM_MEMORY_DEBUG)
	mov		r18, r5;
	and.testz	p0, r16, r16;
	p0	ba	SD_HavePDDevPhysAddr;
	{
		/* Find out the context's Page Directory Physical Address */
		SGXMK_HWCONTEXT_GETDEVPADDR(r18, r16, r19, drc0);
	}
	SD_HavePDDevPhysAddr:
	mov		r20, #(EUR_CR_BIF_DIR_LIST_BASE0 >> 2);
	ldr		r21, r20, drc1;
	wdf		drc1;
	#else
	/* Find out the context's Page Directory Physical Address */
	mov		r20, #(EUR_CR_BIF_DIR_LIST_BASE0 >> 2);
	ldr		r21, r20, drc0;
	/* note: the wdf follows in SGXMK_HWCONTEXT_GETDEVPADDR */
	SGXMK_HWCONTEXT_GETDEVPADDR(r18, r16, r19, drc0);
	#endif

	/* Find out the requestor */
	#if defined(SGX_FEATURE_2D_HARDWARE)
	xor.testz	p3, r17, #SETUP_BIF_2D_REQ;
	#endif
	and.testnz	p1, r17, #(SETUP_BIF_3D_REQ | SETUP_BIF_TQ_REQ);

	/* start with dirlist 0 */
	mov	r19, #0;

	/* Check against DirListBase0 */
	and.testz		p0, r21, r21;
	p0	ba	SD_SetupDirectory;
	{
		/* If the value of the DirList is the same */
		xor.testz		p0, r18, r21;
		!p0	ba	SD_NotDirList0;
		{
			/* simply exit */
			ba SD_SetupDirectory;
		}
		SD_NotDirList0:

		/* Loop through the DirListBases */
		mov	r19, #1;
		SD_CheckNextDirBase:
		{
			xor.testz	p0, r19, #(SGX_FEATURE_BIF_NUM_DIRLISTS - 1);
			p0	ba	SD_DirListNotFound;
			{
				mov		r20, #((EUR_CR_BIF_DIR_LIST_BASE1 >> 2)-1);
				iaddu32	r20, r19.low, r20;
				/* Load the value from current DirListBase */
				ldr		r21, r20, drc0;
				wdf		drc0;

				/* check if the DirListBase is non-zero */
				and.testz		p0, r21, r21;
				p0	ba	SD_SetupDirectory;

				/* If the value of the DirList is the same, simply invalidate internal caches and setup requestor */
				xor.testz		p0, r18, r21;
				/* if p0, r18 == r21. */
				p0 ba	SD_SetupDirectory;

				/* Not this one, move onto next DirList */
				iaddu32	r19, r19.low, #1;
				/* Check whether next one */
				ba	SD_CheckNextDirBase;
			}
		}
		SD_DirListNotFound:

		/* Could not find a free DirListBase or the PhysicalAddr in a DirList, so allocate the next DirListBase */
		/* only thing we must check is that the other requestor(s) (TA/3D) is not using it */
		/* load the last DirList to be evicted */
		MK_MEM_ACCESS_BPCACHE(ldad)		r19, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32LastDirListBaseAlloc)], drc0;
		/* find out if this base is currently being used by the other requestors TA/3D/2D */
		ldr		r20, #EUR_CR_BIF_BANK0 >> 2, drc1;
		wdf		drc1;
	
		p1 	and	r21, r20, #EUR_CR_BIF_BANK0_INDEX_TA_MASK;
		!p1 and	r21, r20, #EUR_CR_BIF_BANK0_INDEX_3D_MASK;
		p1	shr	r21, r21, #EUR_CR_BIF_BANK0_INDEX_TA_SHIFT;
		!p1	shr	r21, r21, #EUR_CR_BIF_BANK0_INDEX_3D_SHIFT;
	#if defined(SGX_FEATURE_2D_HARDWARE)
		p3	and	r20, r20, #EUR_CR_BIF_BANK0_INDEX_TA_MASK;
		p3	shr	r20, r20, #EUR_CR_BIF_BANK0_INDEX_TA_SHIFT;
		p3	ba	SD_ReqValuesReady;
		{
			/* Must be the TA we are setting up */
		#if defined(SGX_FEATURE_PTLA)
			and		r20, r20, #EUR_CR_BIF_BANK0_INDEX_PTLA_MASK;
			shr		r20, r20, #EUR_CR_BIF_BANK0_INDEX_PTLA_SHIFT;	
		#else
			and		r20, r20, #EUR_CR_BIF_BANK0_INDEX_2D_MASK;
			shr		r20, r20, #EUR_CR_BIF_BANK0_INDEX_2D_SHIFT;		
		#endif			
		}
		SD_ReqValuesReady:
	#endif
		wdf		drc0;

		SD_CheckNextDirAgainstReq:
		{
			iaddu32		r19, r19.low, #1;
			xor.testz	p0, r19, #(SGX_FEATURE_BIF_NUM_DIRLISTS - 1);
			/* if we have exceeded the max, loop back to zero */
			p0	mov	r19, #0;
			/* compare the requestor(s) */
			xor.testnz		p0, r21, r19;
	#if defined(SGX_FEATURE_2D_HARDWARE)
			p0	xor.testnz	p0, r20, r19;
	#endif
			!p0	ba	SD_CheckNextDirAgainstReq;

	#if defined(SUPPORT_SGX_EDM_MEMORY_DEBUG)
			/* if using EDM requestor, we have checked that this dirlistbase
			 * isn't used by 2D or 3D tasks. Still need to check it's not used
			 * by the TA
			 */
			xor.testnz	p1, r17, #SETUP_BIF_EDM_REQ;
			p1	ba	SD_DirListBaseReady;
			{
				/* Check requestor not used by TA */
				ldr		r20, #EUR_CR_BIF_BANK0 >> 2, drc1;
				wdf		drc1;
				and		r20, r20, #EUR_CR_BIF_BANK0_INDEX_TA_MASK;
				shr		r20, r20, #EUR_CR_BIF_BANK0_INDEX_TA_SHIFT;
				xor.testnz		p0, r20, r19;
				!p0	ba	SD_CheckNextDirAgainstReq;
			}
	#endif
			/* If we have got to here must be ok to use the DirListBase */
		}
	#if defined(SUPPORT_SGX_EDM_MEMORY_DEBUG)
		SD_DirListBaseReady:
		/* Reset p1 */
		and.testnz	p1, r17, #(SETUP_BIF_3D_REQ | SETUP_BIF_TQ_REQ);
	#endif
		/* Setup registers ready for Writing to DirListBase Register */
		and.testz	p0, r19, r19;
		p0	mov		r20, #(EUR_CR_BIF_DIR_LIST_BASE0 >> 2);
		p0	ba		SD_InvalidateBif;
		{
			mov	r20, #((EUR_CR_BIF_DIR_LIST_BASE1 >> 2)-1);
			iaddu32		r20, r19.low, r20;
		}
		SD_InvalidateBif:
		MK_MEM_ACCESS_BPCACHE(stad)		[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32LastDirListBaseAlloc)], r19;
		idf		drc1, st;

		/* invalidate the BIF caches */
		mov		r17, #1;
	#if defined(SUPPORT_SGX_EDM_MEMORY_DEBUG)
		INVALIDATE_BIF_CACHE(r17, r22, r21, SetupDirlistBase);
	#else
		INVALIDATE_BIF_CACHE(r17, r18, r21, SetupDirlistBase);

		/* Restore the PD physical address to r18. */
		SGXMK_HWCONTEXT_GETDEVPADDR(r18, r16, r17, drc0);
	#endif
	}
	SD_SetupDirectory:
	#if defined (SUPPORT_SGX_EDM_MEMORY_DEBUG)
	and.testz	p0, r16, r16;
	p0	ba	SD_NotNewContext;
	#endif
	{
		MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWCONTEXT.ui32Flags)], drc0;
		wdf		drc0;
		and.testnz		p0, r17, #SGXMKIF_HWCFLAGS_NEWCONTEXT;
		/* If the flag is set clear it and invaldc */
		!p0	ba	SD_NotNewContext;
		{
			and		r17, r17, #~(SGXMKIF_HWCFLAGS_NEWCONTEXT);
			MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWCONTEXT.ui32Flags)], r17;
			idf		drc1, st;
			wdf		drc1;
			ba	SD_InvalidateBif;
		}
	}
	SD_NotNewContext:
	#if defined(SGX_FEATURE_2D_HARDWARE)
	p3	ba	SD_CachesInvalidated;
	#endif
	{
		p1	ba	SD_NoPDSCacheInvalidate;
		{
			/* Invalidate PDS cache */
			str		#EUR_CR_PDS_INV0 >> 2, #EUR_CR_PDS_INV0_DSC_MASK;
			INVALIDATE_PDS_CODE_CACHE_UTILS();
		}
		SD_NoPDSCacheInvalidate:

	#if defined(EUR_CR_CACHE_CTRL)
		/* Invalidate the MADD cache */
		ldr		r21, #EUR_CR_CACHE_CTRL >> 2, drc0;
		wdf		drc0;
		or		r21, r21, #EUR_CR_CACHE_CTRL_INVALIDATE_MASK;
		str		#EUR_CR_CACHE_CTRL >> 2, r21;
	#else
		/* Invalidate the data cache */
		str		#EUR_CR_DCU_ICTRL >> 2, #EUR_CR_DCU_ICTRL_INVALIDATE_MASK;

		/* Invalidate the texture cache */
		str		#EUR_CR_TCU_ICTRL >> 2, #EUR_CR_TCU_ICTRL_INVALTCU_MASK;
	#endif

		/* Invalidate the USE cache */
	#if defined(FIX_HW_BRN_25615)
		/* The next three instruction must be done in a cache line */
		.align 16;
		str		#EUR_CR_USE_CACHE >> 2, #EUR_CR_USE_CACHE_INVALIDATE_MASK;
		ldr		r21, #EUR_CR_USE_CACHE >> 2, drc0;
		wdf		drc0;
		or		R_CacheStatus, R_CacheStatus, #CACHE_INVAL_USE;
	#else
		/* Invalidate the USE cache */
		str	#EUR_CR_USE_CACHE >> 2, #EUR_CR_USE_CACHE_INVALIDATE_MASK;
		or		R_CacheStatus, R_CacheStatus, #CACHE_INVAL_USE;
	#endif

		p1	ba	SD_NoWaitForPDSCacheInvalidate;
		{
			/* Wait for PDS cache to be invalidated */
			ENTER_UNMATCHABLE_BLOCK;
			SD_PDSCacheNotInvalidated:
			{
				ldr		r21, #EUR_CR_PDS_CACHE_STATUS >> 2, drc0;
				wdf		drc0;
				and		r21, r21, #EURASIA_PDS_CACHE_VERTEX_STATUS_MASK;
				xor.testnz	p0, r21, #EURASIA_PDS_CACHE_VERTEX_STATUS_MASK;
				p0	ba	SD_PDSCacheNotInvalidated;
			}
			LEAVE_UNMATCHABLE_BLOCK;

			/* Clear the PDS cache flush event */
			str 	#EUR_CR_PDS_CACHE_HOST_CLEAR >> 2, #EURASIA_PDS_CACHE_VERTEX_CLEAR_MASK;
		}
		SD_NoWaitForPDSCacheInvalidate:

	#if defined(EUR_CR_CACHE_CTRL)
		mov		r17, #EUR_CR_EVENT_STATUS_MADD_CACHE_INVALCOMPLETE_MASK;

		ENTER_UNMATCHABLE_BLOCK;
		SD_WaitForMaddCacheInvalidate:
		{
			ldr		r21, #EUR_CR_EVENT_STATUS >> 2, drc0;
			wdf		drc0;
			and.testz	p0, r21, r17;
			p0	ba	SD_WaitForMaddCacheInvalidate;
		}
		LEAVE_UNMATCHABLE_BLOCK;

		/* clear the bits */
		mov		r21, #EUR_CR_EVENT_HOST_CLEAR_MADD_CACHE_INVALCOMPLETE_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r21;
	#else
		/* data cache */
		ENTER_UNMATCHABLE_BLOCK;
		SD_WaitForDataCacheInvalidate:
		{
			ldr		r21, #EUR_CR_EVENT_STATUS2 >> 2, drc0;
			wdf		drc0;
			shl.testns	p0, r21, #(31 - EUR_CR_EVENT_STATUS2_DCU_INVALCOMPLETE_SHIFT);
			p0	ba	SD_WaitForDataCacheInvalidate;
		}
		LEAVE_UNMATCHABLE_BLOCK;

		/* clear the bits */
		mov		r21, #EUR_CR_EVENT_HOST_CLEAR2_DCU_INVALCOMPLETE_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR2 >> 2, r21;

		/* texture cache */
		ENTER_UNMATCHABLE_BLOCK;
		SD_WaitForTextureCacheInvalidate:
		{
			ldr		r21, #EUR_CR_EVENT_STATUS >> 2, drc0;
			wdf		drc0;
			shl.testns	p0, r21, #(31 - EUR_CR_EVENT_STATUS_TCU_INVALCOMPLETE_SHIFT);
			p0	ba	SD_WaitForTextureCacheInvalidate;
		}
		LEAVE_UNMATCHABLE_BLOCK;

		/* clear the bits */
		mov		r21, #EUR_CR_EVENT_HOST_CLEAR_TCU_INVALCOMPLETE_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r21;
	#endif

	#if defined(SGX_FEATURE_WRITEBACK_DCU)
		p1 	mov		r21, #SGX545_USE_OTHER2_CFI_DATAMASTER_PIXEL;
		!p1 mov		r21, #SGX545_USE_OTHER2_CFI_DATAMASTER_VERTEX;
		/* Invalidate the DCU cache. */
		ci.global	r21, #0, #SGX545_USE1_OTHER2_CFI_LEVEL_L0L1L2;
	#endif

	#if defined(SGX_FEATURE_SYSTEM_CACHE)
		#if defined(SGX_FEATURE_MP)
			!p1	mov		r16, #(EUR_CR_MASTER_SLC_CTRL_INVAL_DM_VERTEX_MASK | \
								EUR_CR_MASTER_SLC_CTRL_FLUSH_DM_VERTEX_MASK);
			p1	mov		r16, #(EUR_CR_MASTER_SLC_CTRL_INVAL_DM_PIXEL_MASK | \
								EUR_CR_MASTER_SLC_CTRL_FLUSH_DM_PIXEL_MASK);
			INVALIDATE_SYSTEM_CACHE_INLINE(r17, r16, r21, SD);
		#else
			INVALIDATE_SYSTEM_CACHE_INLINE(r17, r21, SD);
		#endif /* SGX_FEATURE_MP */
	#endif
	#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
		INVALIDATE_EXT_SYSTEM_CACHE_INLINE(r17);
	#endif
	}
	SD_CachesInvalidated:

	/* check if the value is already setup, if so don't re-write it */
	ldr		r21, r20, drc0;
	wdf		drc0;
	xor.testz		p0, r21, r18;
	p0		ba	SD_DirListBaseStored;
	{
		/* As we have found a DirListBase we must set it up */
		SGXMK_SPRV_UTILS_WRITE_REG(r20, r18);
	}
	SD_DirListBaseStored:
	lapc;
}

/*****************************************************************************
 SetupRequestor - Set up a TA, 3D or 2D requestor for new directory list base.

 inputs:	r19 = dir list base used for current requestor
			p1 = true if using 3D/TQ requestor, false otherwise
 temps:		r19, r20, r21
if(SGX_FEATURE_2D_HARDWARE)
In addition to above
 inputs:	p3 = true if using 2D requestor, false otherwise

*****************************************************************************/
SetupRequestor:
{
	/* Setup the correct requestor */
	ldr		r20, #EUR_CR_BIF_BANK0 >> 2, drc1;
	wdf		drc0;
#if defined(SGX_FEATURE_2D_HARDWARE)
	#if defined(SGX_FEATURE_PTLA)
	p3 		shl		r21, r19, #EUR_CR_BIF_BANK0_INDEX_PTLA_SHIFT;
	wdf		drc1;
	p3		and		r20, r20, #~(EUR_CR_BIF_BANK0_INDEX_PTLA_MASK);
	#else
	p3 		shl		r21, r19, #EUR_CR_BIF_BANK0_INDEX_2D_SHIFT;
	wdf		drc1;
	p3		and		r20, r20, #~(EUR_CR_BIF_BANK0_INDEX_2D_MASK);	
	#endif
	p3	ba	SD_SetBifBank0;
#endif
	{
		!p1		shl		r21, r19, #EUR_CR_BIF_BANK0_INDEX_TA_SHIFT;
		p1		shl		r21, r19, #EUR_CR_BIF_BANK0_INDEX_3D_SHIFT;
#if !defined(SGX_FEATURE_2D_HARDWARE)
		wdf		drc1;
#endif
		!p1 	and		r20, r20, #~(EUR_CR_BIF_BANK0_INDEX_TA_MASK);
		p1 		and		r20, r20, #~(EUR_CR_BIF_BANK0_INDEX_3D_MASK);
	}
	SD_SetBifBank0:
	or		r20, r20, r21;
	SGXMK_SPRV_UTILS_WRITE_REG(#EUR_CR_BIF_BANK0 >> 2, r20);
	/* Read back the register to ensure write has completed */
	ldr		r20, #EUR_CR_BIF_BANK0 >> 2, drc0;
	wdf		drc0;
	lapc;
}

#if defined(SUPPORT_SGX_EDM_MEMORY_DEBUG)
/*****************************************************************************
 SetupRequestorEDM - Set up an EDM requestor for new directory list base.

 inputs:	r19 = dir list base used for EDM requestor
 temps:		r19, r20, r21

*****************************************************************************/
SetupRequestorEDM:
{
	/* Setup the correct requestor */
	ldr		r20, #EUR_CR_BIF_BANK0 >> 2, drc1;
	wdf		drc0;

	/* set up EDM requestor in BIF_BANK0 */
	ldr		r20, #EUR_CR_BIF_BANK0 >> 2, drc0;
	wdf		drc0;
	shl		r21, r19, #EUR_CR_BIF_BANK0_INDEX_EDM_SHIFT;
	and		r20, r20, #~(EUR_CR_BIF_BANK0_INDEX_EDM_MASK);

	or		r20, r20, r21;
	SGXMK_SPRV_UTILS_WRITE_REG(#EUR_CR_BIF_BANK0 >> 2, r20);
	/* Read back the register to ensure write has completed */
	ldr		r20, #EUR_CR_BIF_BANK0 >> 2, drc0;
	wdf		drc0;
	lapc;
}
#endif /* SUPPORT_SGX_EDM_MEMORY_DEBUG */

#else /* SGX_FEATURE_MULTIPLE_MEM_CONTEXTS */

/*****************************************************************************
 SetupDirListBase - Set up a directory list base for either TA or 3D requestor.

 inputs:	r16 = HWRenderContext structure for the application
			r17 = Setup TA/3D/EDM requestor
			0 = TA, 1 = 3D, 2 = TQ, 4 = 2D if 2D hardware, 8 = EDM
 temps:		r18, r19, r20, r21
*****************************************************************************/
SetupDirListBase:
{
	/* At this point we need to flush the caches and update directory base 0 */
	ldr		r19, #EUR_CR_BIF_DIR_LIST_BASE0 >> 2, drc1;
	/* note: the wdf follows in SGXMK_HWCONTEXT_GETDEVPADDR */
	SGXMK_HWCONTEXT_GETDEVPADDR(r18, r16, r20, drc1);

	xor.testz	p1, r18, r19;
	p1	ba		SD_NoNewDirBase;
	{
		#if defined(DEBUG)
			/*
				Directory base is changing, so assert that everything is idle.
			*/
			MK_LOAD_STATE(r19, ui32ITAFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			mov	r21, #TA_IFLAGS_ABORTINPROGRESS;
			and.testnz	p1, r19, #TA_IFLAGS_TAINPROGRESS;
			p1	and.testz	p1, r19, r21;
			p1	ba	SD_Busy;
			{
				MK_LOAD_STATE(r19, ui32IRenderFlags, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);
				and.testnz	p1, r19, #RENDER_IFLAGS_RENDERINPROGRESS;
				p1	ba	SD_Busy;
				{
					#if defined(SGX_FEATURE_2D_HARDWARE)
					MK_LOAD_STATE(r19, ui32I2DFlags, drc0);
					MK_WAIT_FOR_STATE_LOAD(drc0);
					and.testnz	p1, r19, #TWOD_IFLAGS_2DBLITINPROGRESS;
					p1	ba	SD_Busy;
					#endif
					/* If we get here the hardware is idle and it's ok to continue */
					ba	SD_NotBusy;
				}
			}
			SD_Busy:
			{
				MK_ASSERT_FAIL(MKTC_SDLB_ILLEGAL);
			}
			SD_NotBusy:
		#endif /* DEBUG */

		/* check which caches we need to flush */
		and.testnz	p1, r17, r17;

		/* Clear for later use */
		mov		r19, #0;
		p1	ba	SD_NoPDSOrMADDFlush;
		{
			/* Invalidate the PDS cache */
			str		#EUR_CR_PDS_INV0 >> 2, #EUR_CR_PDS_INV0_DSC_MASK;
			INVALIDATE_PDS_CODE_CACHE_UTILS();

		#if defined(EUR_CR_CACHE_CTRL)
			/* Invalidate the MADD cache */
			ldr		r17, #EUR_CR_CACHE_CTRL >> 2, drc0;
			wdf		drc0;
			or		r17, r17, #EUR_CR_CACHE_CTRL_INVALIDATE_MASK;
			str		#EUR_CR_CACHE_CTRL >> 2, r17;
			
			/* Indicate that we need to wait for the MADD to Invalidate */
			or		r19, r19, #EUR_CR_EVENT_STATUS_MADD_CACHE_INVALCOMPLETE_MASK;
		#else
			/* Invalidate the data cache */
			str		#EUR_CR_DCU_ICTRL >> 2, #EUR_CR_DCU_ICTRL_INVALIDATE_MASK;
			
			/* Wait for the DCU to invalidate */
			ENTER_UNMATCHABLE_BLOCK;
			SD_WaitForDataCacheInvalidate:
			{
				ldr		r17, #EUR_CR_EVENT_STATUS2 >> 2, drc0;
				wdf		drc0;
				shl.testns	p0, r17, #(31 - EUR_CR_EVENT_STATUS2_DCU_INVALCOMPLETE_SHIFT);
				p0	ba	SD_WaitForDataCacheInvalidate;
			}
			LEAVE_UNMATCHABLE_BLOCK;
		
			/* clear the bits */
			mov		r17, #EUR_CR_EVENT_HOST_CLEAR2_DCU_INVALCOMPLETE_MASK;
			str		#EUR_CR_EVENT_HOST_CLEAR2 >> 2, r17;

			/* Invalidate the texture cache */
			str		#EUR_CR_TCU_ICTRL >> 2, #EUR_CR_TCU_ICTRL_INVALTCU_MASK;
			
			/* Indicate that we need to wait for the texture cache to Invalidate */
			or		r19, r19, #EUR_CR_EVENT_STATUS_TCU_INVALCOMPLETE_MASK;
		#endif
		
			/* Wait for the PDS cache invalidate to complete */
			ENTER_UNMATCHABLE_BLOCK;
			SD_WaitForPDSCacheInvalidate:
			{
				ldr		r17, #EUR_CR_PDS_CACHE_STATUS >> 2, drc0;
				wdf		drc0;
				and		r17, r17, #EURASIA_PDS_CACHE_VERTEX_STATUS_MASK;
				xor.testnz	p0, r17, #EURASIA_PDS_CACHE_VERTEX_STATUS_MASK;

				p0	ba	SD_WaitForPDSCacheInvalidate;
			}
			LEAVE_UNMATCHABLE_BLOCK;

			/* Clear the PDS cache flush event */
			str 	#EUR_CR_PDS_CACHE_HOST_CLEAR >> 2, #EURASIA_PDS_CACHE_VERTEX_CLEAR_MASK;
		}
		SD_NoPDSOrMADDFlush:

		/* Flush TPC to ensure writes to memory are complete */
		mov		r17, #EUR_CR_TE_TPCCONTROL_FLUSH_MASK;
		str		#EUR_CR_TE_TPCCONTROL >> 2, r17;

		/* Invalidate the USE cache */
	#if defined(FIX_HW_BRN_25615)
		/* The next three instruction must be done in a cache line */
		.align 16;
		str		#EUR_CR_USE_CACHE >> 2, #EUR_CR_USE_CACHE_INVALIDATE_MASK;
		ldr		r17, #EUR_CR_USE_CACHE >> 2, drc0;
		wdf		drc0;
		or		R_CacheStatus, R_CacheStatus, #CACHE_INVAL_USE;
	#else
		/* Invalidate the USE cache */
		str	#EUR_CR_USE_CACHE >> 2, #EUR_CR_USE_CACHE_INVALIDATE_MASK;
		or		R_CacheStatus, R_CacheStatus, #CACHE_INVAL_USE;
	#endif

		or		r19, r19, #EUR_CR_EVENT_STATUS_TPC_FLUSH_MASK;
		ENTER_UNMATCHABLE_BLOCK;
		SD_WaitForOperations:
		{
			ldr		r17, #EUR_CR_EVENT_STATUS >> 2, drc0;
			wdf		drc0;
			and		r17, r17, r19;
			xor.testnz	p0, r17, r19;
			p0	ba		SD_WaitForOperations;
		}
		LEAVE_UNMATCHABLE_BLOCK;

		/* The clear bits match the status bits */
		str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r19;

	#if defined(SGX_FEATURE_SYSTEM_CACHE)
		#if defined(SGX_FEATURE_MP)
			!p1	mov		r16, #(EUR_CR_MASTER_SLC_CTRL_INVAL_DM_VERTEX_MASK | \
								EUR_CR_MASTER_SLC_CTRL_FLUSH_DM_VERTEX_MASK);
			p1	mov		r16, #(EUR_CR_MASTER_SLC_CTRL_INVAL_DM_PIXEL_MASK | \
								EUR_CR_MASTER_SLC_CTRL_FLUSH_DM_PIXEL_MASK);
			INVALIDATE_SYSTEM_CACHE_INLINE(r20, r16, r19, SD);
		#else
			INVALIDATE_SYSTEM_CACHE_INLINE(r17, r19, SD);
		#endif /* SGX_FEATURE_MP */
	#endif
#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
		INVALIDATE_EXT_SYSTEM_CACHE_INLINE(r17);
#endif

		/* write new dir base */
		SGXMK_SPRV_UTILS_WRITE_REG(#EUR_CR_BIF_DIR_LIST_BASE0 >> 2, r18);
		ldr		r18, #EUR_CR_BIF_DIR_LIST_BASE0 >> 2, drc0;
		wdf 	drc0;

	#if defined(FIX_HW_BRN_31620)
		/* refill all the DC cache with the new PD */
		REQUEST_DC_LOAD();

		/* The call to InvalidateBIFCache will do a PT invalidate */
	#endif
		
		/* Invalidate the BIF caches */
		mov		r17, #1;
		INVALIDATE_BIF_CACHE(r17, r18, r21, SetupDirlistBase);
	}
	SD_NoNewDirBase:
	lapc;
}
#endif /* defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS) */


.export ClearActivePowerFlags;
/*****************************************************************************
 ClearActivePowerFlags - Clear an active power flag and check whether to start the
 							active power countdown.

 inputs:	r16 - flag to clear
 temps:		r17
*****************************************************************************/
ClearActivePowerFlags:
{
	MK_LOAD_STATE(r17, ui32ActivePowerFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testz		p0, r17, r16;
	p0 ba			ClearActivePowerFlags_End;
	xor				r16, r16, #~0;
	and				r17, r17, r16;
	MK_STORE_STATE(ui32ActivePowerFlags, r17);
	and.testz		p0,	r17, r17;
	MK_WAIT_FOR_STATE_STORE(drc0);
	!p0 ba			ClearActivePowerFlags_End;
	/* All the flags are cleared, so start the active power counter. */
	MK_MEM_ACCESS_BPCACHE(ldad)	r17, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32ActivePowManSampleRate)], drc0;
	wdf				drc0;
	MK_STORE_STATE(ui32ActivePowerCounter, r17);
	MK_WAIT_FOR_STATE_STORE(drc0);
	
	/* Notify the host that the activity state has changed. */
	MK_MEM_ACCESS_BPCACHE(ldad)	r17, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32InterruptFlags)], drc0;
	wdf				drc0;
	or				r17, r17, #PVRSRV_USSE_EDM_INTERRUPT_IDLE;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32InterruptFlags)], r17;
	idf				drc0, st;
	wdf				drc0;

	/* Send iterrupt to host. */
	mov	 			r17, #EUR_CR_EVENT_STATUS_SW_EVENT_MASK;
	str	 			#EUR_CR_EVENT_STATUS >> 2, r17;
	
ClearActivePowerFlags_End:
	lapc;
}

#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
.export SwitchEDMBIFBank;
/*****************************************************************************
 SwitchEDMBIFBank - Switches the EDM BIF BANK mapping to the TA, 3D
 					or original EDM view

 inputs:	r16 - 0=TA, 1=3D, 2=EDM
 temps:		r17, r18
*****************************************************************************/
SwitchEDMBIFBank:
{
	/* load the BIFBANK reg */
	ldr			r17, #EUR_CR_BIF_BANK0 >> 2, drc0;

	/* move to EDM? */
	xor.testz	p2, r16, #2;
	p2 mov		r18,  #(SGX_BIF_DIR_LIST_INDEX_EDM << EUR_CR_BIF_BANK0_INDEX_EDM_SHIFT);
	wdf			drc0;

#if defined(SGX_FEATURE_WRITEBACK_DCU)
	/* Flush the DCU cache before switching to a new data master */
	cf.global	#SGX545_USE_OTHER2_CFI_DATAMASTER_EVENT, #0, #SGX545_USE1_OTHER2_CFI_LEVEL_L0L1L2;
#endif

	p2 ba		SE_DMSelected;
	{
		/* move to TA? */
		xor.testz	p2, r16, #0;
		p2 and		r18, r17, #EUR_CR_BIF_BANK0_INDEX_TA_MASK;
		p2 shr		r18, r18, #(EUR_CR_BIF_BANK0_INDEX_TA_SHIFT - EUR_CR_BIF_BANK0_INDEX_EDM_SHIFT);
		p2 ba		SE_DMSelected;
		{
			/* move to 3D */
			and			r18, r17, #EUR_CR_BIF_BANK0_INDEX_3D_MASK;
			shr			r18, r18, #(EUR_CR_BIF_BANK0_INDEX_3D_SHIFT - EUR_CR_BIF_BANK0_INDEX_EDM_SHIFT);
		}
	}
	SE_DMSelected:

	/* clear the current EDM mapping */
	and			r17, r17, ~#EUR_CR_BIF_BANK0_INDEX_EDM_MASK;

	/* set the new mapping, store and flush */
	or			r17, r17, r18;
	SGXMK_SPRV_UTILS_WRITE_REG(#EUR_CR_BIF_BANK0 >> 2, r17);
	ldr			r17, #EUR_CR_BIF_BANK0 >> 2, drc0;
	wdf			drc0;

	xor.testnz 		p2, r16, #2;
	p2	ba		SE_NoDummyStoreRequired;
	{
		MK_MEM_ACCESS_BPCACHE(ldad)	r17, [R_TA3DCtl, #0], drc0;
		wdf				drc0;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #0], r17;
		idf				drc0, st;
		wdf				drc0;
	}
	SE_NoDummyStoreRequired:	

	lapc;
}
#endif /* defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS) */


#if defined(SGX_FEATURE_SYSTEM_CACHE)
.export InvalidateSLC;
/*****************************************************************************
 InvalidateSLC - Invalidate the SLC

 inputs:	r16 - invalidate mask bits
 			r17 - flush mask bits
 temps:		r18, r19, r20
*****************************************************************************/
InvalidateSLC:
{
	#if defined(SGX_FEATURE_MP)
	/* Combine the Flush and Invalidate masks */
	or		r16, r16, r17;
	INVALIDATE_SYSTEM_CACHE_INLINE(r20, r16, r19, ISLC);
	#else
	INVALIDATE_SYSTEM_CACHE_INLINE(r17, r19, ISLC);
	#endif
	lapc;
}
#endif


#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
.export InvalidateExtSysCache;
/*****************************************************************************
 InvalidateExtSysCache - Invalidate the external system cache

 inputs:	none
 temps:		r17
*****************************************************************************/
InvalidateExtSysCache:
{
	INVALIDATE_EXT_SYSTEM_CACHE_INLINE(r17);
	lapc;
}
#endif

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
.export StartTAContextSwitch;
.export STACS_TermState0;
.export STACS_TermState1;
/*****************************************************************************
 StartTAContextSwitch
	This routines cause the TA to start context switching away from the
	current operation.

 inputs:
 temps:		r16: ui32ITAFlags
 			r17, r18, r19, r20
*****************************************************************************/
StartTAContextSwitch:
{
	/*
		Check if a context store is already in progress before requesting one
	*/
	MK_LOAD_STATE(r16, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testnz	p2, r16, #TA_IFLAGS_HALTTA;
	p2	mov	r17, #SGX_UKERNEL_TA_CTXSWITCH_INPROGRESS;
	p2	ba	STACS_InProgress;

	/* Check if reserve pages were added for SPM deadlock condition, if so don't context switch */
	shl.tests	p2, r16, #(31 - TA_IFLAGS_SPM_DEADLOCK_MEM_BIT);
	p2	ba	STACS_InProgress;
	{
#if defined(FIX_HW_BRN_33657)
	#if !defined(SUPPORT_SECURE_33657_FIX)
		/* Disable security to allow VDM terminate task to modify the TE_PSG control */
		SGXMK_SPRV_UTILS_WRITE_REG(#EUR_CR_USE_SECURITY >> 2, #EUR_CR_USE_SECURITY_DISABLE_MASK );
	#endif
#endif

		MK_MEM_ACCESS_BPCACHE(ldad)	r17, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderContext)], drc0;
		wdf		drc0;
#if defined(DEBUG)	
		/* Count number of stores */
		MK_MEM_ACCESS_BPCACHE(ldad)	r18, [r17, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32NumTAContextStores)], drc0;
		wdf		drc0;
		mov		r19, #1;
		iaddu32		r18, r19.low, r18;
		MK_MEM_ACCESS_BPCACHE(stad)	[r17, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32NumTAContextStores)], r18;
		idf	drc0, st;
		wdf	drc0;
#endif
	#if defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
		/* Copy the SA buffer address to the TA3D ctl so it is ready for the terminate program */
		MK_MEM_ACCESS(ldad) 	r18, [r17, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sSABufferDevAddr)], drc0;
		#if defined(SGX_FEATURE_MP)
		MK_MEM_ACCESS(ldad) 	r19, [r17, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32SABufferStride)], drc0;
		#endif
		wdf		drc0;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sVDMSABufferDevAddr)], r18;
		#if defined(SGX_FEATURE_MP)
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32VDMSABufferStride)], r19;
		#endif
		idf		drc0, st;
	#endif
	
	#if defined(EUR_CR_MASTER_DPM_PAGE_MANAGEOP_SERIAL_MASK)
		/* Disable DPM serial mode during context switch */
		READMASTERCONFIG(r18, #EUR_CR_MASTER_DPM_PAGE_MANAGEOP >> 2, drc0);
		wdf		drc0;
		and		r18, r18, ~#EUR_CR_MASTER_DPM_PAGE_MANAGEOP_SERIAL_MASK;
		WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_PAGE_MANAGEOP >> 2, r18, r19);
	#endif

	#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH)
		/* Enable proactive PIM speculation */
		READMASTERCONFIG(r2, #EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, drc0);
		wdf		drc0;
		and	r2, r2, ~#EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC_EN_N_MASK;
		WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, r2, r3);

		MK_MEM_ACCESS(ldad)	r18, [r17, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sMasterVDMSnapShotBufferDevAddr)], drc0;
		wdf		drc0;
		/* Setup the registers to store out the state */
		WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_CONTEXT_STORE_SNAPSHOT >> 2, r18, r19);
	#endif
	#if defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
		mov				r18, #OFFSET(SGXMKIF_HWRENDERCONTEXT.sVDMSnapShotBufferDevAddr[0]);
		iaddu32			r18, r18.low, r17;
		COPYMEMTOCONFIGREGISTER_TA(STACS_VDMSnapShotBuffer, EUR_CR_VDM_CONTEXT_STORE_SNAPSHOT, r18, r19, r20, r21);
	#endif
	
		STACS_TermState0:
		/* The following limms will be replaced at runtime with the ui32TATermState words */
		mov		r17, #0xDEADBEEF;
		STACS_TermState1:
		mov		r18, #0xDEADBEEF;

		/* Setup registers that point to TA terminate state update code */
	#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && !defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
		WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_CONTEXT_STORE_STATE0 >> 2, r17, r19);
		WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_CONTEXT_STORE_STATE1 >> 2, r18, r19);

		WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_CONTEXT_STORE_START >> 2, \
							#EUR_CR_MASTER_VDM_CONTEXT_STORE_START_PULSE_MASK, r19);
	#else
		str		#EUR_CR_VDM_CONTEXT_STORE_STATE0 >> 2, r17;
		str		#EUR_CR_VDM_CONTEXT_STORE_STATE1 >> 2, r18;

		#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) // SGX_SUPPORT_MASTER_ONLY_SWITCHING
		/* Only start the master state store */
		WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_CONTEXT_STORE_START >> 2, \
						  #EUR_CR_VDM_CONTEXT_STORE_START_PULSE_MASK, r18);
		#else
		/* Perform a VDM context store and wait for the store to occur */
		str		#EUR_CR_VDM_CONTEXT_STORE_START >> 2, #EUR_CR_VDM_CONTEXT_STORE_START_PULSE_MASK;
		#endif /* FIX_HW_BRN_31425 */

	#endif
	
		{
			mov		r18, #(SGX_MK_VDM_CTX_STORE_STATUS_COMPLETE_MASK | \
							SGX_MK_VDM_CTX_STORE_STATUS_PROCESS_MASK | \
							SGX_MK_VDM_CTX_STORE_STATUS_NA_MASK);
			mov		r19, #3; /* Try 3 times */
			STACS_WaitForStoreToStart:
			{
		#if (defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && !defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)) \
			|| defined(FIX_HW_BRN_31425)
				READMASTERCONFIG(r17, #EUR_CR_MASTER_VDM_CONTEXT_STORE_STATUS >> 2, drc0);
		#else
				ldr		r17, #EUR_CR_VDM_CONTEXT_STORE_STATUS >> 2, drc0;
		#endif
				wdf		drc0;
				and.testz		p2, r17, r18;
				p2		isub16.testnz	r19, p2, r19, #1;
				p2		ba	STACS_WaitForStoreToStart;
			}
		
			/* Check if store has started: if not, it's a normal switch */
			and.testz	p2, r17, r18;
			p2			ba	STACS_NotFastSwitch;
			
		#if defined(EUR_CR_TIMER)
			/* Only poll for 2400 clks (timer increments in every 16 clks) */
			ldr		r18, #SGX_MP_CORE_SELECT(EUR_CR_TIMER, 0) >> 2, drc1;
			wdf		drc1;
			mov		r19, #(2400 / 16);
			iaddu32	r18, r19.low, r18;
		#endif
			STACS_WaitForProcessToDrop:
			{
		#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) // SGX_SUPPORT_MASTER_ONLY_SWITCHING
				READMASTERCONFIG(r17, #EUR_CR_MASTER_VDM_CONTEXT_STORE_STATUS >> 2, drc0);
		#else
				ldr		r17, #EUR_CR_VDM_CONTEXT_STORE_STATUS >> 2, drc0;
		#endif
		
		#if defined(EUR_CR_TIMER)
				/* Check if we have time out. */
				ldr		r19, #SGX_MP_CORE_SELECT(EUR_CR_TIMER, 0) >> 2, drc0;
				wdf		drc0;
				
				and.testz	p2, r17, #SGX_MK_VDM_CTX_STORE_STATUS_PROCESS_MASK;
				p2 ba	STACS_NotProcessing;
				{
					/* We still need to poll, so check if it has timed out */	
					/* If the new value is > timeout, then exit loop */
					isub16.tests	p2, r18, r19;
					p2	ba	STACS_NotFastSwitch;
					/* We have not timed out yet */
					ba	STACS_WaitForProcessToDrop;
				}
				STACS_NotProcessing:
		#else
				wdf		drc0;
				and.testnz	p2, r17, #SGX_MK_VDM_CTX_STORE_STATUS_PROCESS_MASK;
				p2	ba	STACS_WaitForProcessToDrop;
		#endif
			}
		}
		STACS_NotFastSwitch:
		{
			/* 	Flag we were trying to context switch the TA so we know what caused the
				TA finished event */
			mov		r17, #SGX_UKERNEL_TA_CTXSWITCH_NORMAL;
			or		r16, r16, #TA_IFLAGS_HALTTA;
		}
		STACS_Exit:
		MK_STORE_STATE(ui32ITAFlags, r16);
		MK_WAIT_FOR_STATE_STORE(drc0);
	}
	STACS_InProgress:
	
	/* return */
	lapc;
}
#endif

#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH)
.export Start3DContextSwitch;
/*****************************************************************************
 Start3DContextSwitch
	This routines cause the ISP to start context switching away from the
	current render.

 inputs:
 temps:		r16, r17, r18, r19
*****************************************************************************/
Start3DContextSwitch:
{
	/*
		The context store will cause a render finished event so just save whatever is
		needed so we can get to the KickRender routine ASAP after this happens
	*/
	MK_LOAD_STATE(r17, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	/* indicate that a context store has been requested */
	or		r17, r17, #RENDER_IFLAGS_HALTRENDER;
	MK_STORE_STATE(ui32IRenderFlags, r17);
	MK_WAIT_FOR_STATE_STORE(drc0);

#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH_REV_3)
#if defined(FIX_HW_BRN_31930) || defined(FIX_HW_BRN_32052)
	#if defined(SGX_FEATURE_MP)
	/* Clear down the slave core EOR event status */
	MK_LOAD_STATE_CORE_COUNT_3D(r18, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	#else
	mov	r18, #1;
	#endif
	S3DCS_NextCore:
	{
		isub32		r18, r18, #1;
		mov				r16, #EUR_CR_EVENT_HOST_CLEAR_PIXELBE_END_RENDER_MASK;
		WRITECORECONFIG(r18, #EUR_CR_EVENT_HOST_CLEAR >> 2, \
						r16, r19);

		/* Check next core */
		and.testnz	p0, r18, r18;
		p0			ba	S3DCS_NextCore;
	}
	idf		drc0, st;
	wdf		drc0;
#endif
#endif
	/* Load up the context */
	MK_LOAD_STATE(r16, s3DContext, drc0);
	
#if defined(SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH)
	and.testnz	p3, r17, #RENDER_IFLAGS_TRANSFERRENDER;
	p3	ba	S3DCS_FastRender;
	{
#endif
		/* Wait for the render context to load */
		MK_WAIT_FOR_STATE_LOAD(drc0);
		
#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH_REV_3)
#if defined(DEBUG)
		/* Count number of stores */
		MK_MEM_ACCESS_BPCACHE(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32Num3DContextStores)], drc0;
		wdf		drc0;
		mov		r18, #1;
		iaddu32		r17, r18.low, r17;
		MK_MEM_ACCESS_BPCACHE(stad)	[r16, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32Num3DContextStores)], r17;
		idf	drc0, st;
		wdf	drc0;
#endif
		/* Setup the buffers to be used during the context store */
		mov				r17, #OFFSET(SGXMKIF_HWRENDERCONTEXT.sZLSCtxSwitchBaseDevAddr);
		iaddu32			r17, r17.low, r16;
						
		COPYMEMTOCONFIGREGISTER_3D(S3DCS_ZBase, EUR_CR_ISP_CONTEXT_SWITCH, r17, r18, r19, r20);
		
		/* We know that there is only 1 PDS */
		mov		r17, #OFFSET(SGXMKIF_HWRENDERCONTEXT.asPDSStateBaseDevAddr);
		iaddu32	r16, r17.low, r16;
		COPYMEMTOCONFIGREGISTER_3D(S3DCS_PDSBase, EUR_CR_PDS_CONTEXT_STORE, r16, r17, r18, r19);
	#if defined(SGX_FEATURE_MP)
		/* Enabled Tile mode context switching */
		mov		r16, #(SGX_MP_CORE_SELECT(EUR_CR_PDS_CONTEXT_STORE, 0) >> 2);
		MK_LOAD_STATE_CORE_COUNT_3D(r17, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		S3DCS_SetTileMode:
		{
			/* Modify the PDS setup so we context switch on a tile granularity */
			ldr		r18, r16, drc0;
			wdf		drc0;
			or		r18, r18, #(EUR_CR_PDS_CONTEXT_STORE_TILE_ONLY_MASK | \
								EUR_CR_PDS_CONTEXT_STORE_DISABLE_MASK);
			str		r16, r18;
			isub16.testz	r17, p0, r17, #1;
			p0	ba			S3DCS_SetTileModeDone;
			{
				mov				r18, #(SGX_REG_BANK_SIZE >> 2);
				iaddu32			r16, r18.low, r16;
				ba				S3DCS_SetTileMode;
			}
		}
		S3DCS_SetTileModeDone:
		
		/* configure the 3D to context switch on a tile granularity */
		ldr	r16, #EUR_CR_ISP_IPFMISC >> 2, drc0;
		wdf	drc0;
		or	r16, r16, #EUR_CR_ISP_IPFMISC_CONTEXT_STORE_TILE_ONLY_MASK;
		str	#EUR_CR_ISP_IPFMISC >> 2, r16;
	#else
	/* TODO: Fix me! Need to do somthing here for non-mp cores! */
	#endif
	
	#if defined(FIX_HW_BRN_31930)
		/* Clear potential 3D lockup bit */
		ldr		r16, #EUR_CR_EVENT_HOST_CLEAR2 >> 2, drc0;
		wdf		drc0;
		and		r16, r16, ~#EUR_CR_EVENT_HOST_CLEAR2_TRIG_3D_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR2 >> 2, r16;
	#endif
#else
		MK_MEM_ACCESS(ldad)	r18, [r16, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sZLSCtxSwitchBaseDevAddr)], drc0;
		wdf		drc0;
		/* Setup the registers to store out the state */
		str		#EUR_CR_ISP2_CONTEXT_SWITCH >> 2, r18;

		/* Now setup the PDS state store address */
		MK_MEM_ACCESS(ldad)	r18, [r16, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.asPDSStateBaseDevAddr[0][0])], drc0;
		wdf		drc0;
		str		#EUR_CR_PDS0_CONTEXT_STORE >> 2, r18;
	#if defined(EUR_CR_PDS1_CONTEXT_STORE)
		MK_MEM_ACCESS(ldad)	r18, [r16, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.asPDSStateBaseDevAddr[0][1])], drc0;
		wdf		drc0;
		str		#EUR_CR_PDS1_CONTEXT_STORE >> 2, r18;
	#endif
#endif

#if defined(SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH)
		ba	S3DCS_SetupComplete;
	}
	S3DCS_FastRender:
	{
		/* Setup the registers to store out the state */
		str		#EUR_CR_ISP2_CONTEXT_SWITCH >> 2, #0;

		/* Wait for the transfer context to load */
		MK_WAIT_FOR_STATE_LOAD(drc0);

	#if defined(SGX_FEATURE_MP)
		/* Setup the buffers to be used during the context store */
		/* We know that there is only 1 PDS */
		mov		r17, #OFFSET(SGXMKIF_HWTRANSFERCONTEXT.asPDSStateBaseDevAddr);
		iaddu32	r16, r17.low, r16;
		COPYMEMTOCONFIGREGISTER_3D(S3DCS_PDSBase, EUR_CR_PDS_CONTEXT_STORE, r16, r17, r18, r19);
	#else
		MK_MEM_ACCESS(ldad)	r18, [r16, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.asPDSStateBaseDevAddr[0][0])], drc0;
		wdf		drc0;
		str		#EUR_CR_PDS0_CONTEXT_STORE >> 2, r18;
		#if defined(EUR_CR_PDS1_CONTEXT_STORE)
		MK_MEM_ACCESS(ldad)	r18, [r16, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.asPDSStateBaseDevAddr[0][1])], drc0;
		wdf		drc0;
		str		#EUR_CR_PDS1_CONTEXT_STORE >> 2, r18;
		#endif
	#endif
	}
	S3DCS_SetupComplete:
#endif	/* SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH */

	str		#EUR_CR_ISP_3DCONTEXT >> 2, #1;

	/* return */
	lapc;
}
#endif


/*
	FIXME: the following routine does not follow the guidelines for sgx_utils.asm
	with respect to temporary register usage, etc.
*/

/*****************************************************************************
 FreeContextMemory routine	- performs the minimum that is required to free
 								parameter memory associated with a scene

 inputs:	r0 - SGXMKIF_HWRENDERCONTEXT of scene to free
			R_RTData - SGXMKIF_HWRTDATA for TA command
 temps:		r2, r3, r4, r5
*****************************************************************************/
.export FreeContextMemory;
FreeContextMemory:
{
	MK_TRACE(MKTC_FCM_START, MKTF_SCH);
	/*
		We need to make sure we maintain the state of any context currently
		loaded on the TA if it is different
	*/
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], drc0;
	wdf		drc0;
	and.testz	p0, r2, r2;
	!p0	xor.testz	p0, r2, R_RTData;
	p0	ba	FCM_NoTAContextStore;
	{
		/* There is a different context loaded in progress, therefore store it out */
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], #0;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderContext)], #0;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderDetails)], #0;

		/* Mark this context as 'free' */
		ldr		r3, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc0;
		wdf		drc0;
		and		r3, r3, #EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_MASK;
		shr		r4, r3, #EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_SHIFT;
		and.testz	p0, r4, r4;
		p0	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext0RTData)], #0;
		!p0	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext1RTData)], #0;
		idf		drc0, st;

		/* find out the RenderContext of the RTDATA */
		MK_MEM_ACCESS(ldad)	r3, [r2, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderContextDevAddr)], drc0;
		wdf		drc0;

		/* Set up the TA for the memory context to be stored. */
		SETUPDIRLISTBASE(FCM_STAC1, r3, #SETUP_BIF_TA_REQ);

		/* Store the DPM state via the TA Index */
		STORETACONTEXT(r2, r4);
	}
	FCM_NoTAContextStore:

	/* Decide which context should be used by freed */
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext0RTData)], drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext1RTData)], drc1;
	mov		r4, #0 << EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_SHIFT;
	wdf		drc0;
	wdf		drc1;

	xor.testz	p0, R_RTData, r2;
	p0	ba	FCM_ContextChosen;
	{
		xor.testz	p0, R_RTData, r3;
		p0	mov	r4, #1 << EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_SHIFT;
		p0	ba	FCM_ContextChosen;
		{
			and.testz	p0, r2, r2;
			p0	ba	FCM_ContextChosen;
			{
				and.testz	p0, r3, r3;
				p0	mov	r4, #1 << EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_SHIFT;
				p0	ba	FCM_ContextChosen;
				{
					/* Choose the context not being used by 3D */
					ldr		r4, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc0;
					wdf		drc0;
					and		r4, r4, #EUR_CR_DPM_STATE_CONTEXT_ID_DALLOC_MASK;
					shr		r4, r4, #EUR_CR_DPM_STATE_CONTEXT_ID_DALLOC_SHIFT;
					xor		r4, r4, #1;
					shl		r4, r4, #EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_SHIFT;
				}
			}
		}
	}
	FCM_ContextChosen:

	MK_MEM_ACCESS_BPCACHE(ldad)	r5, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], drc0;
	wdf		drc0;
	xor.testz	p0, r5, R_RTData;
	p0	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], #0;

	/* Mark the context as freed */
	and.testz	p0, r4, r4;
	p0	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext0RTData)], #0;
	!p0	mov 	r2, r3;
	!p0	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext1RTData)], #0;

	/* Set the context alloc ID */
	ldr		r5, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc0;
	wdf		drc0;
#if defined(EUR_CR_DPM_STATE_CONTEXT_ID_NCOP_MASK)
	and		r5, r5, ~#EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_MASK;
	or		r5, r5, #EUR_CR_DPM_STATE_CONTEXT_ID_NCOP_MASK;
#else
	and		r5, r5, ~#EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_MASK;
#endif
	or		r5, r5, r4;
	str		#EUR_CR_DPM_STATE_CONTEXT_ID >> 2, r5;

	/* Convert context alloc ID to load/store ID as this is all that is required from now on */
	shr		r4, r4, #EUR_CR_DPM_STATE_CONTEXT_ID_ALLOC_SHIFT;

	/* If there's something using the chosen context we need to store it first */
	and.testz	p0, r2, r2;
	!p0	xor.testz	p0, r2, R_RTData;
	p0	ba		FCM_NoContextStore;
	{
		/* find out the RenderContext of the RTDATA */
		MK_MEM_ACCESS(ldad)	r3, [r2, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderContextDevAddr)], drc0;
		wdf		drc0;

		/* Set up the TA for the memory context to be stored. */
		SETUPDIRLISTBASE(FCM_STAC2, r3, #SETUP_BIF_TA_REQ);

		/* Store the DPM state via the TA Index */
		STORETACONTEXT(r2, r4);
	}
	FCM_NoContextStore:

	/* Set up the TA for the memory context of this kick. */
	SETUPDIRLISTBASE(FCM, r0, #SETUP_BIF_TA_REQ);

	xor.testz	p0, r2, R_RTData;
	p0	ba	FCM_NoStateLoad;
	{
		/* The State needs to be loaded */
		/* Set the context LS ID */
		ldr		r5, #EUR_CR_DPM_STATE_CONTEXT_ID >> 2, drc0;
		wdf		drc0;
		and		r5, r5, ~#EUR_CR_DPM_STATE_CONTEXT_ID_LS_MASK;
		or		r5, r5, r4;
		str		#EUR_CR_DPM_STATE_CONTEXT_ID >> 2, r5;

	#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
		/* Set DPM BIF indicator bit for TA */
		ldr 	r5, #EUR_CR_BIF_BANK_SET >> 2, drc0;
		wdf		drc0;
		and 	r5, r5, ~#USE_DPM_3D_TA_INDICATOR_MASK;	// clear bit 9 of the register
		str		#EUR_CR_BIF_BANK_SET >> 2, r5;
	#endif
		/* Setup the DPM state table base */
		MK_MEM_ACCESS(ldad)	r3, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sContextStateDevAddr)], drc0;
		wdf		drc0;
		SETUPSTATETABLEBASE(FCM, p0, r3, r4);
		PVRSRV_SGXUTILS_CALL(DPMStateLoad);
	}
	FCM_NoStateLoad:

	/* get the current TA HWPBDesc */
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc.uiAddr)], drc0;
	/* get the new TA HWPBDesc */
	MK_MEM_ACCESS(ldad)	r3, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPBDescDevVAddr.uiAddr)], drc0;
	/* get the current 3D HWPBDesc */
	MK_MEM_ACCESS_BPCACHE(ldad)	r4, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DHWPBDesc.uiAddr)], drc0;
	wdf		drc0;

	/* if the RC PB matches both current TA and 3D. 
		Just do the free as everything is setup ready to use */
	xor.testz	p0, r2, r4;
	p0	xor.testz	p0, r2, r3;
	p0	ba	FCM_StartFreeContextNow;

	xor.testz	p0, r4, #0;
	/* NOTE: Only actually need to store/IDLE 3D if ( (3D != NewPB) || ((3D == New) && (TA != New)) ) */
	p0	ba	FCM_3DIdleNotRequired;
	{
		mov		r16, #USE_IDLECORE_3D_REQ;
		PVRSRV_SGXUTILS_CALL(IdleCore);
		/* If different store is required to load-back, if same
		store is required to loadtapb. */
		STORE3DPB();
	}
	FCM_3DIdleNotRequired:

	/* if Current PB == New PB then no store or load required */
	xor.testz	p0, r2, r3;
	p0	ba	FCM_NoPBLoad;
	{
		/* if Current PB == NULL then no store required */
		and.testz	p0, r2, r2;
		p0	ba	FCM_NoPBStore;
		{
#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS) && !defined(FIX_HW_BRN_25910)
			/* if Current PB == 3D PB then no store required */
			xor.testz	p0, r2, r4;
			p0	ba	FCM_NoPBStore;
#endif
			{
				/* PB Store: No idle required as we know the PB is not in use */
				STORETAPB();
			}
		}
		FCM_NoPBStore:

		/* PB Load: */
#if defined(FIX_HW_BRN_32302) || defined(SUPPORT_SGX_DOUBLE_PIXEL_PARTITIONS)
		mov	r5, #USE_FALSE;
		LOADTAPB(r3, r5);
#else
		LOADTAPB(r3);
#endif

		/* As we have just loaded the TAPB there is no need to store it before 3D load */
		ba	FCM_NoTAStoreRequired;

	}
	FCM_NoPBLoad:
	STORETAPB();

	FCM_NoTAStoreRequired:
	/* if the RC PB matches the current 3DPB no load is required, as TA and 3D are already sync'd */
	xor.testz	p0, r3, r4;
	p0	ba	FCM_StartFreeContextNow;
	/* Load3D PB required to set context-pb base correctly */
	LOAD3DPB(r3);

	FCM_StartFreeContextNow:

#if defined(FIX_HW_BRN_34264)
	ldr	r5, #EUR_CR_BIF_BANK0 >> 2, drc0;
	wdf	drc0;
	/* Store BIF_BANK0 in r16 to restore it later */
	mov	r16, r5;
	/* Get the TA Dirlist-Index */
	and	r17, r5, #EUR_CR_BIF_BANK0_INDEX_TA_MASK;
	/* Clear the 3D Dirlist-Index */
	and	r5, r5, #(~EUR_CR_BIF_BANK0_INDEX_3D_MASK);		
	/* Shift the TA index into the 3D index position */
	shl	r17, r17, #(EUR_CR_BIF_BANK0_INDEX_3D_SHIFT -  EUR_CR_BIF_BANK0_INDEX_TA_SHIFT);
	or	r5, r5, r17;
	SGXMK_SPRV_WRITE_REG(#EUR_CR_BIF_BANK0 >> 2, r5);
	/* r16 is lost if any utils calls is made
	Allthough there is none, using r5 for safety of compatibility
	with future code changes.
	, so store in r5 */
	mov	r5, r16;
#endif	

	/* Scene has been marked for freeing, so free the pages allocated */
	str		#EUR_CR_DPM_FREE_CONTEXT >> 2, #EUR_CR_DPM_FREE_CONTEXT_NOW_MASK;

		/* Make sure we clear the TPC */
	mov		r3, #EUR_CR_TE_TPCCONTROL_CLEAR_MASK;
	str		#EUR_CR_TE_TPCCONTROL >> 2, r3;

	/* Wait for the memory to be freed */
	mov		r3, #EUR_CR_EVENT_STATUS_DPM_TA_MEM_FREE_MASK | EUR_CR_EVENT_STATUS_TPC_CLEAR_MASK;
	ENTER_UNMATCHABLE_BLOCK;
	FCM_WaitForTAMemFree:
	{
		ldr		r2, #EUR_CR_EVENT_STATUS >> 2, drc0;
		wdf		drc0;
		and		r2, r2, r3;
		xor.testnz	p0, r2, r3;
		p0		ba		FCM_WaitForTAMemFree;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the event status. */
	mov		r3, #EUR_CR_EVENT_HOST_CLEAR_DPM_TA_MEM_FREE_MASK | EUR_CR_EVENT_HOST_CLEAR_TPC_CLEAR_MASK;
	str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r3;

#if defined(FIX_HW_BRN_34264)
	SGXMK_SPRV_WRITE_REG(#EUR_CR_BIF_BANK0 >> 2, r5);
#endif
	xor.testz	p0, r4, #0;
	p0	ba	FCM_3DResumeNotRequired;	
	{
		/* Dont reload the 3DPB if the original matches the one used for free, just do the resume */
		MK_MEM_ACCESS(ldad)	r3, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPBDescDevVAddr.uiAddr)], drc0;
		wdf		drc0;
		xor.testz	p0, r3, r4;
		p0	ba	FCM_No3DLoadRequired;

		STORE3DPB();
		LOAD3DPB(r4);
		
		FCM_No3DLoadRequired:

		RESUME(r4);
	}
	FCM_3DResumeNotRequired:

#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS) && defined(FIX_HW_BRN_23410)
	/* Make sure the TA is always pointing at a valid address space. */
	ldr		r4, #EUR_CR_BIF_BANK0 >> 2, drc1;
	mov		r3, #SGX_BIF_DIR_LIST_INDEX_EDM;
	shl		r3, r3, #EUR_CR_BIF_BANK0_INDEX_TA_SHIFT;
	wdf		drc1;
	and		r4, r4, #~(EUR_CR_BIF_BANK0_INDEX_TA_MASK);
	or		r4, r4, r3;
	SGXMK_SPRV_UTILS_WRITE_REG(#EUR_CR_BIF_BANK0 >> 2, r4);
#endif /* defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS) */

	MK_TRACE(MKTC_FCM_END, MKTF_SCH);

	/* return */
	lapc;
}


/*****************************************************************************
 MoveSceneToCompleteList routine - moves a completed partial scene to the
 								   render context complete list and adds the
 								   rendercontext to the CompleteRenderContext list
 								   if required.

 inputs:	r16 - SGXMKIF_HWRENDERCONTEXT
 			r17 - SGXMKIF_HWRENDERDETAILS
 temps:		r18, r19, r20
*****************************************************************************/
.export MoveSceneToCompleteList;
MoveSceneToCompleteList:
{
	/* remove it from the partial render list */
	MK_MEM_ACCESS(ldad)	r18, [r17, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sPrevDevAddr)], drc0;
	MK_MEM_ACCESS(ldad)	r19, [r17, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sNextDevAddr)], drc1;
	wdf		drc0;
	and.testnz	p2, r18, r18;
	wdf		drc1;
	p2	ba	MSCL_NotPartialHead;
	{
		MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPartialRendersHead)], r19;
		ba	MSCL_PartialHeadModified;
	}
	MSCL_NotPartialHead:
	{
		MK_MEM_ACCESS(stad)	[r18, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sNextDevAddr)], r19;
	}
	MSCL_PartialHeadModified:
	and.testnz	p2, r19, r19;
	p2	ba	MSCL_NotPartialTail;
	{
		MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPartialRendersTail)], r18;
		ba	MSCL_PartialTailModified;
	}
	MSCL_NotPartialTail:
	{
		MK_MEM_ACCESS(stad)	[r19, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sPrevDevAddr)], r18;
	}
	MSCL_PartialTailModified:

	/*
		No need to wait for stores to complete as these write do not affect insertion
		about to insert into complete list so no need to set Next and Prev to 0
	*/

	/* Add the render to the tail of the context's complete render list */
	MK_MEM_ACCESS(ldad)	r18, [r16, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWCompleteRendersHead)], drc0;
	MK_MEM_ACCESS(ldad)	r19, [r16, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWCompleteRendersTail)], drc0;
	wdf		drc0;
	and.testnz	p2, r18, r18;
	p2	ba	MSCL_CompleteListNotEmpty;
	{
		/* Add to the Head */
		MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWCompleteRendersHead)], r17;
		ba	MSCL_CompleteHeadModified;
	}
	MSCL_CompleteListNotEmpty:
	{
		/* Link to current Tail */
		MK_MEM_ACCESS(stad)	[r19, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sNextDevAddr)], r17;
	}
	MSCL_CompleteHeadModified:
	MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWCompleteRendersTail)], r17;
	MK_MEM_ACCESS(stad)	[r17, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sPrevDevAddr)], r19;
	MK_MEM_ACCESS(stad)	[r17, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sNextDevAddr)], #0;
	idf		drc0, st;
	wdf		drc0;

	/* If it already has render on the complete list, then don't add the current context to the tail of the complete render contexts list */
	p2	ba		MSCL_CompleteListUpdated;
	{
		/* The sHWCompleteRendersHead must have been empty so add the context to the complete list */
		MK_MEM_ACCESS(ldad)	r17, [r16, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sCommon.ui32Priority)], drc0;
		wdf		drc0;

		/* Get the Head and Tails */
		MK_LOAD_STATE_INDEXED(r20, sCompleteRenderContextHead, r17, drc0, r18);
		MK_LOAD_STATE_INDEXED(r21, sCompleteRenderContextTail, r17, drc0, r19);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and.testnz	p2, r20, r20;
		p2	ba	MSCL_ContextListNotEmpty;
		{
			/* Update the Head */
			MK_STORE_STATE_INDEXED(sCompleteRenderContextHead, r17, r16, r18);
			ba	MSCL_ContextListHeadModified;
		}
		MSCL_ContextListNotEmpty:
		{
			/* Link the old tail to the context */
			MK_MEM_ACCESS(stad)	[r21, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextCompleteDevAddr)], r16;
		}
		MSCL_ContextListHeadModified:
		/* Update the Tail */
		MK_STORE_STATE_INDEXED(sCompleteRenderContextTail, r17, r16, r19);
		MK_MEM_ACCESS(stad)	[r16, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevCompleteDevAddr)], r21;
		idf		drc0, st;
		wdf		drc0;
	}
	MSCL_CompleteListUpdated:

	/* return */
	lapc;
}

/*****************************************************************************
 CommonProgramEnd routine - This does all the task required before ending a 
 								task

 inputs:	r16 - Non-zero to flush SLC (where applicable)
 temps:		r18, r19
*****************************************************************************/
.export CommonProgramEnd;
CommonProgramEnd:
{
	#if defined(SGX_FEATURE_WRITEBACK_DCU)

#if defined(FIX_HW_BRN_31939)
	/*
	 * Write a unique value through the DCU before executing the cfi, then read
	 * it back from memory after the cfi. If the value read is incorrect, the
	 * cfi was lost due to BRN31939, so retry.
	 */
	CPE_CFI_retry:
	MK_LOAD_STATE(r18, ui32BRN31939SA, drc0);
	REG_INCREMENT(r18, p0);
	MK_STORE_STATE(ui32BRN31939SA, r18);
	stad	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32BRN31939Mem)], r18;
	
	idf drc0, st;
	wdf drc0;
#endif /* FIX_HW_BRN_31939 */

	/* Flush all write to memory before exiting */
	cfi.global	#SGX545_USE_OTHER2_CFI_DATAMASTER_EVENT, #0, #SGX545_USE1_OTHER2_CFI_LEVEL_L0L1L2;

#if defined(FIX_HW_BRN_31939)
	ldad.bpcache	r19, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32BRN31939Mem)], drc0;
	wdf drc0;
	xor.testz	p0, r18, r19;
	!p0 ba CPE_CFI_retry;
#endif /* FIX_HW_BRN_31939 */

	#endif
	
	
#if defined(SGX_FEATURE_MP)
	/*
		Write-back SLC - flush if requested by caller.
	*/
	and.testz	p0, r16, r16;
	p0	ba	CPE_NoSLCFlush;
	{
		FLUSH_SYSTEM_CACHE_INLINE(p0, r16, #EUR_CR_MASTER_SLC_CTRL_FLUSH_ALL_MASK, r17, CPE);
	}
	CPE_NoSLCFlush:
#endif /* SGX_FEATURE_MP */
	
	
	/* Ensure all writes to memory complete */
#if defined(DEBUG)
	MK_MEM_ACCESS_BPCACHE(ldad)	r18, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32IdleCoreRefCount)], drc0;
	wdf		drc0;
	xor.testz	p0, r18, #0;
	p0	ba	CPE_RefCountOK;
	{
		MK_ASSERT_FAIL(MKTC_IDLECORE_REFCOUNT_FAIL);
	}
	CPE_RefCountOK:
#endif
	idf		drc0, st;
	wdf		drc0;
#if !defined(FIX_HW_BRN_29104)
	UKERNEL_PROGRAM_PHAS();
#endif
	
	/* return */
	nop.end;
}


#if defined(SGX_FEATURE_MK_SUPERVISOR)
/*****************************************************************************
 JumpToSupervisor routine - writes.

 inputs:	r24 - task identifier
			r25 - argument 0 for task
 			r26 - argument 1 for task
 temps:		r29
			- supervisor thread will use r24-27
*****************************************************************************/
.export JumpToSupervisor;
.export SPRVReturn;
JumpToSupervisor:
{
	/* 
		Note:
		 - we can use PAs, SAs or Temps in 543 for argument passing from
		   micorkernel to SPRV
		 - future cores can only use PAs and SAs
		 - using SAs for this implementation
	*/
	
	/* Save args into SAs */
	MK_STORE_STATE(ui32SPRVTask, r24);
	MK_STORE_STATE(ui32SPRVArg0, r25);
	MK_STORE_STATE(ui32SPRVArg1, r26);
	MK_WAIT_FOR_STATE_STORE(drc0);

	/*
		Save state:
	*/
	/* pclink */
	mov r29, pclink;
	shl r29, r29, #4;
	/* predicates */
p0	or	r29, r29, #0x1;
p1	or	r29, r29, #0x2;
p2	or	r29, r29, #0x4;
p3	or	r29, r29, #0x8;

	/* transition to supervisor */
	sprvv.end;
	
SPRVReturn: /* return address to patch sprv phas with */

	/* re-specify the smlsi on returning from supervisor */
	smlsi	#0,#0,#0,#1,#0,#0,#0,#0,#0,#0,#0;

	/*
		Restore state
	*/
	/* predicates */
	and.testnz	p0, r29, #0x1;
	and.testnz	p1, r29, #0x2;
	and.testnz	p2, r29, #0x4;
	and.testnz	p3, r29, #0x8;

	/* pclink */
	shr		r29, r29, #4;
	mov		pclink, r29;

	/* return */
	lapc;
}
#endif


/*****************************************************************************
 STORE_MISC_COUNTER - helper macro for WriteHWPERFCounters.
 name - name of counter register to store
*****************************************************************************/
#define STORE_MISC_COUNTER(name)							\
		mov r20, HSH((name >> 2));							\
		iaddu32 r20, r20.low, r19;							\
		ldr				r20, r20, drc0;						\
		wdf				drc0;								\
		MK_MEM_ACCESS_BPCACHE(stad)	[r21, HSH(1)++], r20;

#if defined(SUPPORT_SGX_HWPERF)
/*****************************************************************************
 WriteHWPERFEntry - writes a HWPERF CB entry.

 inputs:	r16.low		- Frame number offset in TA3D_CTL
 		r16.high	- PID offset in TA3D_CTL
		r17 		- HWPERF_TYPE
		r18 		- RegInfo
		r19.low 	- RTData offset in TA3D_CTL
 temps:		r20, r21, r30, r31, p2

*****************************************************************************/
.export WriteHWPERFEntry;
WriteHWPERFEntry:
{
	/*
	 * Perf entry address		- r21
	 * writeoffset			- r30
	 * sHWPerfCBDevVAddr		- r31
	 */

	/* Store SGXMKIF_HWPERF_CB in r31. */
	MK_MEM_ACCESS_BPCACHE(ldad)	r31, [R_TA3DCtl, HSH(DOFFSET(SGXMK_TA3D_CTL.sHWPerfCBDevVAddr))], drc0;
	wdf				drc0;

	/* Read the CB Woff and store in r30. */
	MK_MEM_ACCESS_BPCACHE(ldad)	r30, [r31, HSH(DOFFSET(SGXMKIF_HWPERF_CB.ui32Woff))], drc0;
	wdf				drc0;

	/* Calculate the address to write the CCB entry and store in r21.*/
	mov				r20, HSH(SIZEOF(SGXMKIF_HWPERF_CB_ENTRY));
	imae			r20, r30.low, r20.low, HSH(OFFSET(SGXMKIF_HWPERF_CB.psHWPerfCBData)), u32;
	IADD32(r21, r31, r20, r30);

	/* load r30 back*/
	MK_MEM_ACCESS_BPCACHE(ldad)	r30, [r31, HSH(DOFFSET(SGXMKIF_HWPERF_CB.ui32Woff))], drc0;

	/* Read the PID, frame number and RTData from TA3D_CTL */
	iaddu32				r20, r16.high, R_TA3DCtl;
	MK_MEM_ACCESS_BPCACHE(ldad)	r20, [r20], drc0;
	iaddu32				r16, r16.low, R_TA3DCtl;
	MK_MEM_ACCESS_BPCACHE(ldad)	r16, [r16], drc0;
	iaddu32				r19, r19.low, R_TA3DCtl;
	MK_MEM_ACCESS_BPCACHE(ldad)	r19, [r19], drc0;
	wdf				drc0;

	/* Store the data we have at the moment, this frees up registers for the next stage. */
	MK_MEM_ACCESS_BPCACHE(stad)	[r21, HSH(DOFFSET(SGXMKIF_HWPERF_CB_ENTRY.ui32FrameNo))], r16;
	MK_MEM_ACCESS_BPCACHE(stad)	[r21, HSH(DOFFSET(SGXMKIF_HWPERF_CB_ENTRY.ui32PID))], r20;
	MK_MEM_ACCESS_BPCACHE(stad)	[r21, HSH(DOFFSET(SGXMKIF_HWPERF_CB_ENTRY.ui32Type))], r17;
	MK_MEM_ACCESS_BPCACHE(stad)	[r21, HSH(DOFFSET(SGXMKIF_HWPERF_CB_ENTRY.ui32Info))], r18;
	MK_MEM_ACCESS_BPCACHE(stad)	[r21, HSH(DOFFSET(SGXMKIF_HWPERF_CB_ENTRY.ui32RTData))], r19;

	/*
	 * Load and store the timestamp. We have to use p0 as the macro checks !regP.
	 * We also don't have any spare registers so we save p0 in the LSB of pointer to the
	 * HWPERF_CB who's address is 32-bit aligned so we can safely use the LSB
	 */
	p0				or	r31, r31, HSH(0x1);
	WRITEHWPERFCB_TIMESTAMP(p0, r21, r16, r17, r18, r19, r20);
	and.testnz			p0, r31, HSH(0x1);
	and				r31, r31, HSH(0xfffffffe);

	/* Wait for writes to complete */
	idf				drc0, st;
	wdf				drc0;

	/* Write the current ordinal value to the CCB entry and then increament, mask and store the value for next time */
	MK_MEM_ACCESS_BPCACHE(ldad)	r16, [r31, HSH(DOFFSET(SGXMKIF_HWPERF_CB.ui32Ordinal))], drc0;
	wdf				drc0;
	mov				r17, r16;
	REG_INCREMENT(r16, p2);
	/* cut down to a 31-bit value, as bit 32 indicates a SW reset */
	and				r16, r16, HSH(0x7fffffff);
	MK_MEM_ACCESS_BPCACHE(stad)	[r31, HSH(DOFFSET(SGXMKIF_HWPERF_CB.ui32Ordinal))], r16;
	MK_MEM_ACCESS_BPCACHE(stad)	[r21, HSH(DOFFSET(SGXMKIF_HWPERF_CB_ENTRY.ui32Ordinal))], r17;
	idf				drc0, st;
	wdf				drc0;

	/* Move the pointer to the CCB entry to r16 for the next section */
	mov	r16, r21;

	/*
		Sample the performance counters:
		r17 - Core counter
		r18 - Counter offset
		r19 - Config register offset
	*/
	mov		r17, #0;
	mov		r18, #OFFSET(SGXMKIF_HWPERF_CB_ENTRY.ui32Counters[0][0]);
	iaddu32	r21, r18.low, r16;

	WHWPC_NextCore_Perf:
	{
		mov		r19, #(SGX_MP_CORE_SELECT(EUR_CR_PERF_COUNTER0, 0)) >> 2;
		#if defined(SGX_FEATURE_MP)
		mov		r18, #(SGX_REG_BANK_SIZE >> 2);
		imae	r19, r17.low, r18.low, r19, u32;
		#endif /* SGX_FEATURE_MP */
		mov		r18, #0;

		WHWPC_NextCounter:
		{
			/* Store the next performance counter. */
			ldr				r20, r19, drc0;
			wdf				drc0;
			MK_MEM_ACCESS_BPCACHE(stad)	[r21, #1++], r20;

			/* Move on to next counter. */
			REG_INCREMENT(r18, p2);
			REG_INCREMENT(r19, p2);
			xor.testnz		p2, r18, #PVRSRV_SGX_HWPERF_NUM_COUNTERS;
			p2				ba WHWPC_NextCounter;
		}

		/* Move on to next core. */
		REG_INCREMENT(r17, p2);
		#if defined(SGX_FEATURE_MP)
		MK_LOAD_STATE_CORE_COUNT_3D(r19, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		#else
		mov		r19, #1;
		#endif
		xor.testnz	p2, r17, r19;
		p2			ba WHWPC_NextCore_Perf;
	}


	/*
		Sample the misc counters:
		r17 - Core counter
	*/
	mov		r17, #0;
	mov		r18, #OFFSET(SGXMKIF_HWPERF_CB_ENTRY.ui32MiscCounters[0][0]);
	iaddu32	r21, r18.low, r16;

	WHWPC_NextCore_Misc:
	{
		mov		r19, #(SGX_MP_CORE_SELECT(0, 0)) >> 2;
		#if defined(SGX_FEATURE_MP)
		mov		r18, #(SGX_REG_BANK_SIZE >> 2);
		imae	r19, r17.low, r18.low, r19, u32;
		#endif /* SGX_FEATURE_MP */

		/*
			Note: the value of PVRSRV_SGX_HWPERF_NUM_MISC_COUNTERS must be
			synchronised with the following list.
		*/
		STORE_MISC_COUNTER(EUR_CR_EMU_TA_PHASE);
		STORE_MISC_COUNTER(EUR_CR_EMU_3D_PHASE);
		STORE_MISC_COUNTER(EUR_CR_EMU_TA_CYCLE);
		STORE_MISC_COUNTER(EUR_CR_EMU_3D_CYCLE);
		STORE_MISC_COUNTER(EUR_CR_EMU_TA_OR_3D_CYCLE);
		STORE_MISC_COUNTER(EUR_CR_EMU_MEM_READ);
		STORE_MISC_COUNTER(EUR_CR_EMU_MEM_WRITE);
		STORE_MISC_COUNTER(EUR_CR_EMU_MEM_BYTE_WRITE);
		#if defined(SGX_FEATURE_EXTENDED_PERF_COUNTERS)
		STORE_MISC_COUNTER(EUR_CR_EMU_INITIAL_TA_CYCLE);
		STORE_MISC_COUNTER(EUR_CR_EMU_FINAL_3D_CYCLE);
		STORE_MISC_COUNTER(EUR_CR_EMU_MEM_STALL);
		#endif /* SGX_FEATURE_EXTENDED_PERF_COUNTERS */

		/* Move on to next core. */
		REG_INCREMENT(r17, p2);
		#if defined(SGX_FEATURE_MP)
		MK_LOAD_STATE_CORE_COUNT_3D(r19, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		#else
		mov		r19, #1;
		#endif
		xor.testnz	p2, r17, r19;
		p2			ba WHWPC_NextCore_Misc;
	}

	/* Decide whether the HW Perf CB is full or not. */
	MK_MEM_ACCESS_BPCACHE(ldad)	r16, [r31, HSH(DOFFSET(SGXMKIF_HWPERF_CB.ui32Roff))], drc0;
	iaddu32				r30, r30.low, HSH(1);
	and				r30, r30, HSH(SGXMKIF_HWPERF_CB_SIZE-1);
	wdf				drc0;
	xor.testnz			p2, r16, r30; /* p2 != CB is full (1 space left for invalid) */

	/* Increment the Woff - AFTER the packet data is written. */
	p2 MK_MEM_ACCESS_BPCACHE(stad)	[r31, HSH(DOFFSET(SGXMKIF_HWPERF_CB.ui32Woff))], r30;

	/* issue & wait for data fence */
	idf				drc0, st;
	wdf				drc0;

	lapc;
}
#endif

#if defined(FIX_HW_BRN_27311) && !defined(FIX_HW_BRN_23055)
/*****************************************************************************
 InitBRN27311StateTable routine - Clear the DPM state table and Set the
 									ZLS count fields to (MaxPBSize - PBSize).
 									To guarantee different MTs are partially
 									rendered if we hit SPM.

 inputs:	r16 - SGXMKIF_HWRTDATA
 			r17 - SGXMKIF_HWPBDESC
 temps:		r18, r19
*****************************************************************************/
.export InitBRN27311StateTable;
InitBRN27311StateTable:
{
	/* Calculate the value to be used for the ZLSCount */
	MK_MEM_ACCESS(ldad)	r17, [r17, #DOFFSET(SGXMKIF_HWPBDESC.ui32NumPages)], drc0;
	wdf		drc0;

	mov		r18, #(EURASIA_PARAMETER_BUFFER_SIZE_IN_PAGES - 1);
	isub16	r17, r18, r17;
	shl		r17, r17, #EURASIA_PARAM_DPM_STATE_MT_ZLSCOUNT_SHIFT;
	and		r17, r17, ~#EURASIA_PARAM_DPM_STATE_MT_ZLSCOUNT_CLRMSK;

	MK_MEM_ACCESS(ldaw)	r18, [r16, #WOFFSET(SGXMKIF_HWRTDATA.ui16NumMTs)], drc0;
	MK_MEM_ACCESS(ldad)	r19, [r16, #DOFFSET(SGXMKIF_HWRTDATA.sContextStateDevAddr)], drc0;
	wdf		drc0;

	/* Clear the Global list entry */
	MK_MEM_ACCESS_BPCACHE(stad)	[r19, #32], #0;
	MK_MEM_ACCESS_BPCACHE(stad)	[r19, #33], #0;

	IST_NextMTEntry:
	{
		/* Clear the first word and set the ZLSCnt in the second */
		MK_MEM_ACCESS_BPCACHE(stad)	[r19, #1++], #0;
		MK_MEM_ACCESS_BPCACHE(stad)	[r19, #1++], r17;

		isub16.testnz	r18, p2, r18, #1;
		p2	ba	IST_NextMTEntry;
	}

	MK_MEM_ACCESS(staw)	[r16, #WOFFSET(SGXMKIF_HWRTDATA.ui16SPMRenderMask)], #0;
	idf		drc0, st;
	wdf		drc0;

	/* return */
	lapc;
}

/*****************************************************************************
 ResetBRN27311StateTable routine - Set the ZLS count fields in the DPM state
 									table to (MaxPBSize - PBSize). To guarantee
 									different MTs are partially rendered if we
 									hit SPM.

 inputs:	r16 - SGXMKIF_HWRTDATA
 			r17 - SGXMKIF_HWPBDESC
 temps:		r18, r19
*****************************************************************************/
.export ResetBRN27311StateTable;
ResetBRN27311StateTable:
{
	/* Calculate the value to be used for the ZLSCount */
	MK_MEM_ACCESS(ldad)	r17, [r17, #DOFFSET(SGXMKIF_HWPBDESC.ui32NumPages)], drc0;
	wdf		drc0;

	mov		r18, #(EURASIA_PARAMETER_BUFFER_SIZE_IN_PAGES - 1);
	isub16	r17, r18, r17;
	shl		r17, r17, #EURASIA_PARAM_DPM_STATE_MT_ZLSCOUNT_SHIFT;
	and		r17, r17, ~#EURASIA_PARAM_DPM_STATE_MT_ZLSCOUNT_CLRMSK;

	MK_MEM_ACCESS(ldaw)	r18, [r16, #WOFFSET(SGXMKIF_HWRTDATA.ui16NumMTs)], drc0;
	MK_MEM_ACCESS(ldad)	r19, [r16, #DOFFSET(SGXMKIF_HWRTDATA.sContextStateDevAddr)], drc0;
	wdf		drc0;

	RST_NextMTEntry:
	{
		/* Clear the first word and set the ZLSCnt in the second */
		MK_MEM_ACCESS_BPCACHE(ldad)	r20, [r19, #1], drc0;
		wdf		drc0;
		and		r20, r20, #EURASIA_PARAM_DPM_STATE_MT_ZLSCOUNT_CLRMSK;
		or		r20, r20, r17;
		MK_MEM_ACCESS_BPCACHE(stad)	[r19, #1], r20;
		mov		r20, #8;
		iaddu32		r19, r20.low, r19;

		isub16.testnz	r18, p2, r18, #1;
		p2	ba	RST_NextMTEntry;
	}

	MK_MEM_ACCESS(staw)	[r16, #WOFFSET(SGXMKIF_HWRTDATA.ui16SPMRenderMask)], #0;
	idf		drc0, st;
	wdf		drc0;

	/* return */
	lapc;
}
#endif

#if (defined(FIX_HW_BRN_30089) || defined(FIX_HW_BRN_29504) || defined(FIX_HW_BRN_33753)) && defined(SGX_FEATURE_MP)
/*****************************************************************************
 BRN30089AddLastRgn routine - Modify the region headers so there is an 
 								additional "last region" for the render about
 								to be started.

 inputs:	r16 - SGXMKIF_HWRTDATA
 			r17 - Index of macrotile to be rendered
 temps:		r18, r19, r20, r21, r22, r30
*****************************************************************************/
.export BRN30089AddLastRgn;
BRN30089AddLastRgn:
{
	/* Single MT ? */
	and.testnz	p2, r17, #EUR_CR_DPM_ABORT_STATUS_MTILE_GLOBAL_MASK;
	p2	ba	ALR_AllMTs;
	{

		/* load the address of the last rgn LUT*/
		MK_MEM_ACCESS(ldad)	r18, [r16, #DOFFSET(SGXMKIF_HWRTDATA.sLastRgnLUTDevAddr)], drc0;
		wdf	drc0;

		/* offset the last rgn LUT with the MT idx*/
		imae	r18, r17.low, #4, r18, u32;

		/* load the rgn offset*/
		MK_MEM_ACCESS(ldad)	r18, [r18], drc1;
		
	
		/* the base of the rgn arrays */
		mov		r17, #OFFSET(SGXMKIF_HWRTDATA.sRegionArrayDevAddr);

		wdf		drc1;
		and		r18, r18, #~SGXMKIF_HWRTDATA_RGNOFFSET_VALID;

		ba	ALR_LUTReady;
	}
	ALR_AllMTs:
	{
		/* We are rendering all the MTs so use the sLastRegion */
		mov		r17, #OFFSET(SGXMKIF_HWRTDATA.sLastRegionDevAddr);
		mov		r18, #0;
	}
	ALR_LUTReady:

	/* get the address to be loaded in the loop*/
	iaddu32	r16, r17.low, r16;
	
	/* Setup the address to save the current values */
	mov		r17, #OFFSET(SGXMK_TA3D_CTL.ui32SavedRgnHeader);
	iaddu32	r17, r17.low, R_TA3DCtl;
	
	/* Now move on the other cores */
	MK_LOAD_STATE_CORE_COUNT_TA(r19, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	ALR_SetNextLastRgn:
	{
		/* Look up the Last Region DevAddr */
		MK_MEM_ACCESS_CACHED(ldad)	r20, [r16, #1++], drc0;
		wdf		drc0;

		/* if it's coming from the LUT it's just an offset rather than address*/
		IADD32(r20, r20, r18, r21);
	
#if defined(FIX_HW_BRN_29504) || defined(FIX_HW_BRN_33753)
		/* Unset the last region bit, to avoid PT in last region */
		MK_MEM_ACCESS_BPCACHE(ldad)	r21, [r20, #0], drc0;
		wdf		drc0;
		and		r21, r21, ~#((EURASIA_REGIONHEADER0_LASTREGION) | (EURASIA_REGIONHEADER0_LASTINMACROTILE));
		MK_MEM_ACCESS_BPCACHE(stad)	[r20, #0], r21;
		and		r22, r21, #~EURASIA_REGIONHEADER0_MACROTILE_CLRMSK; /* save the macrotile number */
#endif
	
		/* Save the next rgn header 0 */
		MK_MEM_ACCESS_BPCACHE(ldad)	r30, [r20, #3], drc0;
		wdf		drc0;
		MK_MEM_ACCESS_BPCACHE(stad)	[r17, #1++], r30;

		mov 		r30, #~(EURASIA_REGIONHEADER0_XPOS_CLRMSK & EURASIA_REGIONHEADER0_YPOS_CLRMSK);
		and 		r21, r21, r30;
		xor.testz 	p0, r21, r30; /* p0 if last tile of maximum render size => tile (0,0) otherwise (1ff,1ff)*/

		p0			mov r21, #0;	
		(!p0)		mov	r21, r30;

		/* Modify the region header */
		or		r21, r21, #((EURASIA_REGIONHEADER0_LASTREGION) | (EURASIA_REGIONHEADER0_EMPTY));
#if defined(FIX_HW_BRN_29504) || defined(FIX_HW_BRN_33753)
		/* Set up the tileX,Y to 0,0; last macrotile bit and macrotile number correctly */
		or		r21, r21, #((EURASIA_REGIONHEADER0_LASTREGION) | (EURASIA_REGIONHEADER0_EMPTY) | (EURASIA_REGIONHEADER0_LASTINMACROTILE));
		or		r21, r21, r22; /* set MT number from previous region hdr */
#endif
		MK_MEM_ACCESS_BPCACHE(stad)	[r20, #3], r21;
	
#if defined(FIX_HW_BRN_30089)
		/* Save the current rgn header 1 */
		MK_MEM_ACCESS_BPCACHE(ldad)	r21, [r20, #6], drc0;
		wdf		drc0;
		MK_MEM_ACCESS_BPCACHE(stad)	[r17, #1++], r21;
		/* Modify the region header */
		mov		r21, #((EURASIA_REGIONHEADER0_LASTREGION) | (EURASIA_REGIONHEADER0_EMPTY));
		MK_MEM_ACCESS_BPCACHE(stad)	[r20, #6], r21;
		
		/* Save the current rgn header 2 */
		MK_MEM_ACCESS_BPCACHE(ldad)	r21, [r20, #9], drc0;
		wdf		drc0;
		MK_MEM_ACCESS_BPCACHE(stad)	[r17, #1++], r21;
		/* Modify the region header */
		mov		r21, #((EURASIA_REGIONHEADER0_LASTREGION) | (EURASIA_REGIONHEADER0_EMPTY));
		MK_MEM_ACCESS_BPCACHE(stad)	[r20, #9], r21;
		
		/* Save the current rgn header 3 */
		MK_MEM_ACCESS_BPCACHE(ldad)	r21, [r20, #12], drc0;
		wdf		drc0;
		MK_MEM_ACCESS_BPCACHE(stad)	[r17, #1++], r21;
		/* Modify the region header */
		mov		r21, #((EURASIA_REGIONHEADER0_LASTREGION) | (EURASIA_REGIONHEADER0_EMPTY));
		MK_MEM_ACCESS_BPCACHE(stad)	[r20, #12], r21;
#endif
	
		/* Check if there are anymore cores to setup */
		isub16.testnz	r19, p0, r19, #1;
		p0	ba	ALR_SetNextLastRgn;
	}
	/* Flush the writes */
	idf		drc0, st;
	wdf		drc0;
	
	/* Return */
	lapc;
}

/*****************************************************************************
 BRN30089RestoreRgnHdr routine - Modify the region headers back to their original
 								state now that the render has finished.

 inputs:	r16 - SGXMKIF_HWRTDATA
 			r17 - Index of macrotile to be rendered
 temps:		r18, r19, r20, r21
*****************************************************************************/
.export BRN30089RestoreRgnHdr;
BRN30089RestoreRgnHdr:
{
	/* Single MT ? */
	and.testnz	p2, r17, #EUR_CR_DPM_ABORT_STATUS_MTILE_GLOBAL_MASK;
	p2	ba	RRH_AllMTs;
	{

		/* load the address of the last rgn LUT*/
		MK_MEM_ACCESS(ldad)	r18, [r16, #DOFFSET(SGXMKIF_HWRTDATA.sLastRgnLUTDevAddr)], drc0;
		wdf	drc0;

		/* offset the last rgn LUT with the MT idx*/
		imae	r18, r17.low, #4, r18, u32;

		/* load the rgn offset*/
		MK_MEM_ACCESS(ldad)	r18, [r18], drc1;
		
		/* the base of the rgn arrays */
		mov		r17, #OFFSET(SGXMKIF_HWRTDATA.sRegionArrayDevAddr);

		wdf		drc1;
		and		r18, r18, #~SGXMKIF_HWRTDATA_RGNOFFSET_VALID;

		ba	RRH_LUTReady;
	}
	RRH_AllMTs:
	{
		/* We are rendering all the MTs so use the sLastRegion */
		mov		r17, #OFFSET(SGXMKIF_HWRTDATA.sLastRegionDevAddr);
		mov     r18, #0;
	}
	RRH_LUTReady:

	/* get the address to be loaded in the loop*/
	iaddu32	r16, r17.low, r16;
	
	/* Setup the address to read back the saved values */
	mov		r17, #OFFSET(SGXMK_TA3D_CTL.ui32SavedRgnHeader);
	iaddu32	r17, r17.low, R_TA3DCtl;
	
	/* Now move on the other cores */
	MK_LOAD_STATE_CORE_COUNT_TA(r19, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	RRH_SetNextLastRgn:
	{
		MK_MEM_ACCESS_CACHED(ldad)	r20, [r16, #1++], drc0;
		wdf		drc0;

		/* if it's coming from the LUT it's just an offset rather than address*/
		IADD32(r20, r20, r18, r21);

		/* Retrieve the saved region header value */
		MK_MEM_ACCESS_BPCACHE(ldad)	r21, [r17, #1++], drc0;
		wdf		drc0;
	
		/* Restore the region header */
		MK_MEM_ACCESS_BPCACHE(stad)	[r20, #3], r21;
	
#if defined(FIX_HW_BRN_30089)
		/* Restrieve the saved region header value 1 */
		MK_MEM_ACCESS_BPCACHE(ldad)	r21, [r17, #1++], drc0;
		wdf		drc0;
		/* Restore the region header */
		MK_MEM_ACCESS_BPCACHE(stad)	[r20, #6], r21;

		/* Restrieve the saved region header value 2 */
		MK_MEM_ACCESS_BPCACHE(ldad)	r21, [r17, #1++], drc0;
		wdf		drc0;
		/* Restore the region header */
		MK_MEM_ACCESS_BPCACHE(stad)	[r20, #9], r21;

		/* Restrieve the saved region header value 3 */
		MK_MEM_ACCESS_BPCACHE(ldad)	r21, [r17, #1++], drc0;
		wdf		drc0;
		/* Restore the region header */
		MK_MEM_ACCESS_BPCACHE(stad)	[r20, #12], r21;
#endif

	
		/* Check if there are anymore cores to setup */
		isub16.testnz	r19, p2, r19, #1;
		p2	ba	RRH_SetNextLastRgn;
	}
	/* Flush the writes */
	idf		drc0, st;
	wdf		drc0;

	
	/* Return */
	lapc;
}
#endif /* FIX_HW_BRN_30089 || FIX_HW_BRN_29504 || FIX_HW_BRN_33753*/

#if defined(FIX_HW_BRN_32052) && defined(SGX_FEATURE_ISP_CONTEXT_SWITCH_REV_3)
/*****************************************************************************
 BRN32052ClearMCIState routine - Clear down outstanding held state in MCI.

 temps:		r16, r17, r18
*****************************************************************************/
.export BRN32052ClearMCIState;
BRN32052ClearMCIState:
{
	/* Mask out interrupt. TODO: may want the interrupt */
	
	/* Set EOR in each core until we receive the MCI event */
	MK_LOAD_STATE_CORE_COUNT_3D(r18, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	CMS_NextCore:
	{
		isub32		r18, r18, #1;
		mov			r16, #EUR_CR_EVENT_STATUS_PIXELBE_END_RENDER_MASK;
		WRITECORECONFIG(r18, #EUR_CR_EVENT_STATUS >> 2, r16, r17);
		idf			drc0, st;
		wdf			drc0;
		
		READMASTERCONFIG(r16, #EUR_CR_MASTER_EVENT_STATUS >> 2, drc0);
		wdf			drc0;
		shl.tests	p0, r16, #(31 - EUR_CR_MASTER_EVENT_STATUS_PIXELBE_END_RENDER_SHIFT);
		p0			ba	CMS_MCIStateCleared;

		/* Check next core */
		and.testnz	p0, r18, r18;
		p0			ba	CMS_NextCore;
	}
	/* If we arrive here sthg is wrong... */
	MK_ASSERT_FAIL(MKTC_MCISTATE_NOT_CLEARED);
	
	CMS_MCIStateCleared:
	/* Clear master event */
	mov			r16, #EUR_CR_MASTER_EVENT_HOST_CLEAR_PIXELBE_END_RENDER_MASK;
	WRITEMASTERCONFIG(#EUR_CR_MASTER_EVENT_HOST_CLEAR >> 2, r16, r17);
	idf			drc0, st;
	wdf			drc0;

	/* Unmask interrupt. TODO: may want the interrupt */
	
	/* Return */
	lapc;
}
#endif
/*****************************************************************************
 End of file (sgx_utils.asm)
*****************************************************************************/
