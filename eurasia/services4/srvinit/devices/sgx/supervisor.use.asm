/*****************************************************************************
 Name		: supervisor.use.asm

 Title		: USE program for handling supervisor tasks

 Author 	: Imagination Technologies

 Created  	: 19/11/2009

 Copyright	: 2009 by Imagination Technologies Limited. All rights reserved.
			  No part of this software, either material or conceptual
			  may be copied or distributed, transmitted, transcribed,
			  stored in a retrieval system or translated into any
			  human or computer language in any form by any means,
			  electronic, mechanical, manual or other-wise, or
			  disclosed to third parties without the express written
			  permission of Imagination Technologies Limited, HomePark
			  Industrial Estate, King's Langley, Hertfordshire,
			  WD4 8LZ, U.K.

 Description :USE program for handling supervisor tasks

 Program Type:USE assembly language

 Version 	: $Revision: 1.13 $

 Modifications	:

 $Log: supervisor.use.asm $
 *****************************************************************************/

/*****************************************************************************
 Temporary register usage in supervisor code
 -------------------------------------------

 6 extra temps are required, r24-r29, for the supervisor code:
 	- 4 for temporary usage in functions
	- 1 for pclink copy for calls from utils
	- 1 for pclink + predicate state restore from sprv

*****************************************************************************/

#include "usedefs.h"

#if defined(SGX_FEATURE_MK_SUPERVISOR)
.export Supervisor;
.export SPRV_Phas;
/* Align to the largest cache line size */
.modulealign 32;
#if defined(FIX_HW_BRN_29997)
/*****************************************************************************
 SPRV_BRN29997Inval routine	
 	- This routine invalidates the BIF whilst working around BRN29997

 inputs:
 	SA(ui32SPRVArg0)

 temps:
 	r24 - stores DMS Ctrl state (const), don't use it for anything else
 	r25, r26, r27, p1
*****************************************************************************/
	#if defined(EUR_CR_BIF_CTRL_INVAL)
SPRV_BRN29997Inval:
{
	/* Mark p1 = false: Need to recheck USSE queues/slots once the
	   PDS is idle. p1 goes true after the first pass */
	mov		r25, #1;
	and.testz	p1, r25, r25;
	
	/* Before doing the invalidate drain the core 0 */
	ldr		r24, #(SGX_MP_CORE_SELECT(EUR_CR_DMS_CTRL, 0)) >> 2, drc0;
	
	ENTER_UNMATCHABLE_BLOCK;
	SPRV_Inval_DrainPipe:
	{
		/* Reload the DMS control into variable */
		ldr		r25, #(SGX_MP_CORE_SELECT(EUR_CR_DMS_CTRL, 0)) >> 2, drc0;
		wdf		drc0;

		or		r25, r25, #EUR_CR_DMS_CTRL_DISABLE_DM_MASK;
		str		#(SGX_MP_CORE_SELECT(EUR_CR_DMS_CTRL, 0)) >> 2, r25;
	
		/* Number of times to try before enabling DMS */
		mov	r27, #25;
		
		/* Wait for the queues on core 0 pipe 0 to drain */
		SPRV_Inval_QueueDrain:
		{
			ldr		r25, #(SGX_MP_CORE_SELECT(EUR_CR_USE0_SERV_PIXEL, 0)) >> 2, drc0;
			ldr		r26, #(SGX_MP_CORE_SELECT(EUR_CR_USE0_SERV_VERTEX, 0)) >> 2, drc0;
			wdf		drc0;
			and		r25, r25, r26;		
			shl.tests	p0, r25, #(31 - EUR_CR_USE0_SERV_PIXEL_EMPTY_SHIFT);
			/* The Queues are empty exit */
			p0	ba	SPRV_Inval_QueuesDrained;
			{
				/* If we have check 25 times and it has still not drain, could be a deadlock */
				isub16.testz	r27, p0, r27, #1;
				p0	ba	SPRV_Inval_ResetDMS;
				
				/* Delay before retrying */
				mov	r25, #500;
				SPRV_Inval_QueueRetryDelay:
				{
					isub16.testns	r25, p2, r25, #1;
					p2 ba	SPRV_Inval_QueueRetryDelay;
				}
				
				/* Check queues again */
				ba	SPRV_Inval_QueueDrain;
			}
		}
		SPRV_Inval_QueuesDrained:
	
		/* Reset the retry counter */
		mov		r27, #25;

		/* Wait for all other tasks on pipe 0 to finish */
		/* Load the current tasks's mask into r18. */
		mov			r25, #0xAAAAAAAA;
		shl			r26, G22, #1;
		shl			r26, #0x3, r26;
		or			r25, r25, r26;
		SPRV_Inval_Pipe0IdleWait:
		{
			ldr 	r26, #(SGX_MP_CORE_SELECT(EUR_CR_USE0_DM_SLOT, 0)) >> 2, drc0;
			wdf		drc0;
			xor.testz	p0, r25, r26;
			/* only uKernel left on the pipe so exit */
			p0	ba		SPRV_Inval_DrainComplete;
			{
				isub16.tests r27, p0, r27, #1;
				/* If we have timed out, could be a deadlock */
				p0 	ba	SPRV_Inval_ResetDMS;
				/* wait for a bit before checking again */
				mov			r26, #500;
				SPRV_Inval_Pipe0RetryDelay:
				{
					isub16.testns	r26, p2, r26, #1;
					p2 ba		SPRV_Inval_Pipe0RetryDelay;
				}
				ba		SPRV_Inval_Pipe0IdleWait;
			}
		}
			
		/* Now wait for the slots to empty */
		SPRV_Inval_ResetDMS:
		{
			/* Reset the DMS */
			str		#(SGX_MP_CORE_SELECT(EUR_CR_DMS_CTRL, 0)) >> 2, r24;
			
			/* One of the polls timed out so wait before disabling DMS again */
			mov		r27, #1000;
			SPRV_Inval_AllowDMSToRun:
			{
				isub16.testns	r27, p0, r27, #1;
				p0 ba	SPRV_Inval_AllowDMSToRun;
			}
			ba	SPRV_Inval_DrainPipe;
		}		
	}
	SPRV_Inval_DrainComplete:
	
	/* 
	 *	Wait for the PDS to go idle ... 
	 */
	/* Enable the perf counter block */
	str		#(SGX_MP_CORE_SELECT(EUR_CR_PERF_DEBUG_CTRL, 0)) >> 2, #EUR_CR_PERF_DEBUG_CTRL_ENABLE_MASK;

	SPRV_PDSIdle_DMS:
	{
		/* Program the perf control for DMS idle */
		mov r25, #PERF_PDS_9;
		shl	r25, r25, #EUR_CR_PERF0_GROUP_SELECT_A0_SHIFT;
		shl	r26, #SHIFT_PERF_PDS_9_PERF_PDS_DMS_STATE, #EUR_CR_PERF0_COUNTER_SELECT_A0_SHIFT;
		or	r25, r25, r26;

		str		#(SGX_MP_CORE_SELECT(EUR_CR_PERF0, 0)) >> 2, r25;
		mov		r27, #500;

		SPRV_PDSIdle_DMS_WaitForTasks:
		{
			/* Sample the PDS/DMS idle state.
			 * DMS events are disabled so once 1 idle clock occurs, block stays idle */
			ldr r26, #(SGX_MP_CORE_SELECT(EUR_CR_DEBUG_REG0, 0)) >> 2, drc0;
			wdf		drc0;
			and.testz	p0, r26, #MASK_PERF_PDS_9_PERF_PDS_DMS_STATE;
			p0 ba	SPRV_PDSIdle_PX;
			
			/* Wait for PDS SM to go idle */
			isub16.testns	r27, p0, r27, #1;
			p0 ba	SPRV_PDSIdle_DMS_WaitForTasks;
		}
		/* TODO: timeout behaviour? Retry draining USSE queues/tasks */
		ba SPRV_Inval_DrainPipe;
	}
	
	SPRV_PDSIdle_PX:
	{
		/* Program the perf control for PX idle */
		mov r25, #PERF_PDS_9;
		shl r25, r25, #EUR_CR_PERF0_GROUP_SELECT_A0_SHIFT;
		shl r26, #SHIFT_PERF_PDS_9_PERF_PDS_DOUTIUT_PX_STATE, #EUR_CR_PERF0_COUNTER_SELECT_A0_SHIFT;
		or	r25, r25, r26;
		str		#(SGX_MP_CORE_SELECT(EUR_CR_PERF0, 0)) >> 2, r25;
		mov		r27, #500;

		SPRV_PDSIdle_PX_WaitForTasks:
		{
			/* Sample the PDS/PX idle state.
			 * DMS events are disabled so once 1 idle clock occurs, block stays idle */
			ldr r26, #(SGX_MP_CORE_SELECT(EUR_CR_DEBUG_REG0, 0)) >> 2, drc0;
			wdf		drc0;
			and.testz	p0, r26, #(MASK_PERF_PDS_9_PERF_PDS_DOUTIUT_PX_STATE >> SHIFT_PERF_PDS_9_PERF_PDS_DOUTIUT_PX_STATE);	
			p0 ba	SPRV_PDSIdleComplete;
			
			/* Wait for PDS SM to go idle */
			isub16.testns	r27, p0, r27, #1;
			p0 ba	SPRV_PDSIdle_PX_WaitForTasks;
		}
		/* TODO: timeout behaviour? Retry draining USSE queues/tasks */
		ba SPRV_Inval_DrainPipe;
	}
	
	SPRV_PDSIdleComplete:
	/* Restore the reg EUR_CR_PERF0.
	 * FIXME: unable to use the generic macro here since only have 3 temps available. 
	 */
	{
		/* Set up bits [15:0] of EUR_CR_PERF0 */
		MK_MEM_ACCESS_BPCACHE(ldad)	r25, [R_HostCtl, HSH(DOFFSET(SGXMKIF_HOST_CTL.aui32PerfGroup[0]))], drc0;
		MK_MEM_ACCESS_BPCACHE(ldad)	r26, [R_HostCtl, HSH(DOFFSET(SGXMKIF_HOST_CTL.aui32PerfBit[0]))], drc0;
		wdf drc0;
		shl	r25, r25, HSH(EUR_CR_PERF0_GROUP_SELECT_A0_SHIFT);
		shl	r26, r26, HSH(EUR_CR_PERF0_COUNTER_SELECT_A0_SHIFT);
		or	r25, r25, r26;

		/* Set up bits [31:16] of EUR_CR_PERF0 */
		MK_MEM_ACCESS_BPCACHE(ldad)	r27, [R_HostCtl, HSH(DOFFSET(SGXMKIF_HOST_CTL.aui32PerfGroup[1]))], drc0;
		MK_MEM_ACCESS_BPCACHE(ldad)	r26, [R_HostCtl, HSH(DOFFSET(SGXMKIF_HOST_CTL.aui32PerfBit[1]))], drc0;
		wdf drc0;
		shl	r27, r27, HSH(EUR_CR_PERF0_GROUP_SELECT_A1_SHIFT);
		shl	r26, r26, HSH(EUR_CR_PERF0_COUNTER_SELECT_A1_SHIFT);
		or	r27, r27, r26;
		or	r25, r25, r27;
		str		#(SGX_MP_CORE_SELECT(EUR_CR_PERF0, 0)) >> 2, r25;
	}
	
	/*
	 * 	... and ensure any remaining tasks are flushed from the USSE
	 */
	p1	ba SPRV_IdleComplete;
	{
		/* Once the PDS is idle need to check remaining tasks in the USSE (pass 2) */
		mov		r25, #0;
		and.testz	p1, r25, r25;
		ba 	SPRV_Inval_DrainPipe;
	}

	SPRV_IdleComplete:
	LEAVE_UNMATCHABLE_BLOCK;
	
	/* Prefetch the next cache line */
	ba	SPRV_Inval_FetchPhase2;
	SPRV_Inval_ExecPhase1:
	{
		MK_LOAD_STATE(r25, ui32SPRVArg0, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and.testnz	p0, r25, r25;
		p0	mov		r25, #EUR_CR_BIF_CTRL_INVAL_ALL_MASK;
		!p0	mov		r25, #EUR_CR_BIF_CTRL_INVAL_PTE_MASK;
		mov	r26, #50;
		and.testnz	p2, r26, r26;
	
		.align 8;
		/* Align the following code to start on a cache line boundary */
		str		#EUR_CR_BIF_CTRL >> 2, #EUR_CR_BIF_CTRL_PAUSE_MASK;

		SPRV_Inval_PreWait:
		{
			isub16.testns r26, p1, r26, #1;
			p1	ba	SPRV_Inval_PreWait;
		}
	#if defined(FIX_HW_BRN_30182)
		/* Setup timeout for mem_req_stat loop */
		mov	r26, #125;
	#endif
	
		ba	SPRV_Inval_ExecPhase2;
	}
	/* Align the following code to start on a cache line boundary */
	.align 8;
	SPRV_Inval_FetchPhase2:
	{
		ba	SPRV_Inval_FetchPhase3;
		
		SPRV_Inval_ExecPhase2:
		SPRV_Inval_WaitForMemReqs:
		{
			ldr		r27, #EUR_CR_BIF_MEM_REQ_STAT >> 2, drc0;
			wdf		drc0;
			and.testnz	p0, r27, r27;
	#if defined(FIX_HW_BRN_30182)
			p0	isub16.testns	r26, p0, r26, #1;
	#endif
			p0 ba	SPRV_Inval_WaitForMemReqs;
		}
		
		ba	SPRV_Inval_ExecPhase3;
	}
	/* Align the following code to start on a cache line boundary */
	.align 8;
	SPRV_Inval_FetchPhase3:
	{
		ba SPRV_Inval_ExecPhase1;
		
		SPRV_Inval_ExecPhase3:

		/* Do the invalidate */
	#if defined(FIX_HW_BRN_31620)
		str		#EUR_CR_BIF_CTRL_INVAL >> 2, #EUR_CR_BIF_CTRL_INVAL_PTE_MASK;
		str		#EUR_CR_MASTER_BIF_CTRL_INVAL >> 2, r25;
	#else
		str		#EUR_CR_BIF_CTRL_INVAL >> 2, r25;
	#endif

		mov	r27, #50;
		SPRV_Inval_PostWait:
		{
			isub16.testns r27, p0, r27, #1;
			p0	ba	SPRV_Inval_PostWait;
		}
		
		/* Unpause the BIF */
		str	#EUR_CR_BIF_CTRL >> 2, #0;
	}
	
	/* Reset the DMS */
	str		#(SGX_MP_CORE_SELECT(EUR_CR_DMS_CTRL, 0)) >> 2, r24;

	/* exit out of supervisor mode */
	ba	SPRV_Phas;
}
	#else
#error ERROR: BRN29997 needs to be implemented for non EUR_CR_BIF_CTRL_INVAL case;
	#endif
#endif

#if defined(FIX_HW_BRN_31620)
/*****************************************************************************
 SPRV_BRN31620Inval routine	
 	- This routine invalidates the DC cache by causing controlled misses

 inputs:
 	SA(ui32SPRVArg0) - inval_msk0
 	SA(ui32SPRVArg1) - inval_msk1

 temps:
 	r24, r25, r26, r27, p1
*****************************************************************************/
SPRV_BRN31620Inval:
{
	/* Copy dirlistbase0 to inval_context */
	ldr		r24, #EUR_CR_BIF_DIR_LIST_BASE0 >> 2, drc0;
	wdf		drc0;
	str		#BRN31620_INVAL_DIR_LIST_BASE >> 2, r24;
	
	#if defined(SGX_FEATURE_MP)
	/* Write the cache invalidate mask to the TA3DCtl ready for the slaves */
	MK_MEM_ACCESS_BPCACHE(stad)		[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32PDInvalMask[0])], SA(sMKState.ui32SPRVArg0);
	MK_MEM_ACCESS_BPCACHE(stad)		[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32PDInvalMask[1])], SA(sMKState.ui32SPRVArg1);
	idf		drc0, st;
	wdf		drc0;
		
	/* First thing is to kick the job off on the other enable cores */
	MK_LOAD_STATE(r24, ui32CoresEnabled, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	SPRV_BRN31620_KickNextSlave:
	{
		/* exit once only core0 remains */
		isub16.testz	r24, p0, r24, #1;
		p0	ba	SPRV_BRN31620_SlavesKicked;
		
		/* Write to the slave core specific register bank */
		WRITECORECONFIG(r24, #(EUR_CR_USE_GLOBCOM2 >> 2), #0, r25);
		WRITECORECONFIG(r24, #(EUR_CR_EVENT_KICK2 >> 2), #EUR_CR_EVENT_KICK2_NOW_MASK, r25);
		
		ba	SPRV_BRN31620_KickNextSlave;
	}
	SPRV_BRN31620_SlavesKicked:
	#endif

	/* Refresh the cache lines indicated in the mask */
	REFRESH_DC_CACHE(p0, r24, r25, r26, r27, #BRN31620_START_ADDR_LOW, SA(sMKState.ui32SPRVArg0), SPRV_BRN31620Inval0);
	REFRESH_DC_CACHE(p0, r24, r25, r26, r27, #BRN31620_START_ADDR_HIGH, SA(sMKState.ui32SPRVArg1), SPRV_BRN31620Inval1);
	
	#if defined(SGX_FEATURE_MP)
	/* Now wait for the other enables cores to complete */
	MK_LOAD_STATE(r24, ui32CoresEnabled, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	SPRV_BRN31620_WaitForNextSlave:
	{
		/* exit once only core0 remains */
		isub16.testz	r24, p0, r24, #1;
		p0	ba	SPRV_BRN31620_AllSlavesComplete;
		
		SPRV_BRN31620_WaitForSlave:
		{
			/* wait for the core to indicate it has completed the refresh */
			READCORECONFIG(r25, r24, #(EUR_CR_USE_GLOBCOM2 >> 2), drc0);
			wdf		drc0;
			and.testz	p0, r25, r25;
			p0	ba	SPRV_BRN31620_WaitForSlave;
		}
		
		ba	SPRV_BRN31620_WaitForNextSlave;
	}
	SPRV_BRN31620_AllSlavesComplete:
	#endif
	
	/* exit out of supervisor mode */
	ba	SPRV_Phas;
}
#endif

/*****************************************************************************
 SupervisorMain routine	
 	- Decides which supervisor routine should be run.

 inputs:
 	SA(ui32SPRVTaskType)

 temps:
 	r24
*****************************************************************************/
Supervisor:
{
	/* Which supervisor function do we need to call? */
	MK_LOAD_STATE(r24, ui32SPRVTask, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	
	
	xor.testz	p0, r24, #SGX_UKERNEL_SPRV_WRITE_REG;
	p0	ba	SPRV_WriteReg;
#if defined(FIX_HW_BRN_29997)
	/* Do we need to do the workaround for BRN29997 */
	xor.testz	p0, r24, #SGX_UKERNEL_SPRV_BRN_29997;
	p0	ba	SPRV_BRN29997Inval;
#endif
#if defined(FIX_HW_BRN_31474)
	/* Do we need to do the workaround for BRN31474 */
	xor.testz	p0, r24, #SGX_UKERNEL_SPRV_BRN_31474;
	p0	ba	SPRV_BRN31474Inval;
#endif
#if defined(FIX_HW_BRN_31620)
	/* Do we need to do the workaround for BRN31620 */
	xor.testz	p0, r24, #SGX_UKERNEL_SPRV_BRN_31620;
	p0	ba	SPRV_BRN31620Inval;
#endif
	
	.align 2;
SPRV_Phas: /* to be patched */
	phas.end	#0, #0, pixel, none, perinstance;
}

/*****************************************************************************
 SPRV_WriteReg routine	
 	- writes security critical registers on behalf of the uKernel

 inputs:
 	SA(ui32SPRVArg0) - This is the register offset
 	SA(ui32SPRVArg1) - This is the value to be written

 temps:
 	r24
*****************************************************************************/
SPRV_WriteReg:
{
	// FIXME: add code to validate addresses to write;
	// if PD phys addr is controlled in host kernel driver (not UM);
	// then no validation should be required;
	MK_LOAD_STATE(r24, ui32SPRVArg0, drc0);
	MK_LOAD_STATE(r25, ui32SPRVArg1, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	str		r24, r25;
	
	/* exit out of supervisor mode */
	ba	SPRV_Phas;
}

#if defined(FIX_HW_BRN_31474)
/*****************************************************************************
 SPRV_BRN31474Inval routine	
 	- Invalidates the PDS code cache

 inputs:
 
 temps:
 	r24
*****************************************************************************/
SPRV_BRN31474Inval:
{
	mov		r24, #(SGX_MP_CORE_SELECT(EUR_CR_EVENT_KICK3, 0)) >> 2;
	MK_LOAD_STATE(r25, ui32CoresEnabled, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	SPRV_BRN31474_NextCore:
	{
		str				r24, #EUR_CR_EVENT_KICK3_NOW_MASK;
		isub16.testnz	r25, p0, r25, #1;
		mov				r26, #SGX_REG_BANK_SIZE >> 2;
		iaddu32			r24, r26.low, r24;
		p0 ba			SPRV_BRN31474_NextCore;
	}
	
	/* exit out of supervisor mode */
	ba	SPRV_Phas;
}
#endif
#else
	nop.end;
#endif /* #if defined(SGX_FEATURE_MK_SUPERVISOR) */

