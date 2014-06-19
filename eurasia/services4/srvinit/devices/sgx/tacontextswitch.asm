/*****************************************************************************
 Name		: tacontextswitch.asm

 Title		: SGX microkernel TA context switch utility code

 Author 	: Imagination Technologies

 Created  	: 03/08/2008

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

 $Log: tacontextswitch.asm $
*****************************************************************************/

#include "usedefs.h"
#include "sgxdefs.h"

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
.export HaltTA;
.export ResumeTA;
.export RTA_MTEStateRestore0;
.export RTA_MTEStateRestore1;
	#if defined(SGX_FEATURE_MTE_STATE_FLUSH) && defined(SGX_FEATURE_COMPLEX_GEOMETRY_REV_2)
.export RTA_CmplxStateRestore0;
.export RTA_CmplxStateRestore1;
	#endif
	#if defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
.export RTA_SARestore0;
.export RTA_SARestore1;
	#endif

.import MKTrace;
.import SwitchEDMBIFBank;
.import	Check3DQueues;
.import CheckTAQueue;
#endif

.modulealign 2;

/*****************************************************************************
 Macro definitions
*****************************************************************************/

/*****************************************************************************
 Start of program
*****************************************************************************/

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)

	#if defined(EUR_CR_MASTER_VDM_TASK_KICK_STATUS_CLEAR_COMPLETE_MASK)
#define 	SGXMK_VDM_TASK_KICK_CLEAR_MASK	EUR_CR_MASTER_VDM_TASK_KICK_STATUS_CLEAR_COMPLETE_MASK;
	#else
#define 	SGXMK_VDM_TASK_KICK_CLEAR_MASK	EUR_CR_MASTER_VDM_TASK_KICK_STATUS_CLEAR_PULSE_MASK;
	#endif

	#if defined(EUR_CR_MASTER_VDM_CONTEXT_LOAD_STATUS_CLEAR_COMPLETE_MASK)
#define 	SGXMK_VDM_CONTEXT_LOAD_STATUS_CLEAR_MASK	EUR_CR_MASTER_VDM_CONTEXT_LOAD_STATUS_CLEAR_COMPLETE_MASK;
	#else
#define 	SGXMK_VDM_CONTEXT_LOAD_STATUS_CLEAR_MASK	EUR_CR_MASTER_VDM_CONTEXT_LOAD_STATUS_CLEAR_PULSE_MASK;
	#endif

/*****************************************************************************
 HaltTA routine	- routine for handling the completion of a VDM Context
 					switch.

 inputs:
 temps:	r0, r1, r2, r3, r4
*****************************************************************************/
HaltTA:
{
	MK_TRACE(MKTC_HALTTA, MKTF_TA | MKTF_SCH | MKTF_CSW);

	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTACCBCmd)], drc0;
	wdf		drc0;
	/* Set the flag so we know to resume this TA later */
	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [r0, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
	wdf		drc0;
	or	r1, r1, #SGXMKIF_TAFLAGS_VDM_CTX_SWITCH;
	MK_MEM_ACCESS_BPCACHE(stad)	[r0, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], r1;

	MK_TRACE(MKTC_HTA_SET_FLAG, MKTF_TA | MKTF_SCH | MKTF_CSW);
	
	{
	#if defined(SGX_FEATURE_MTE_STATE_FLUSH) || \
		(defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && defined(EUR_CR_MASTER_VDM_FENCE_STATUS))
		MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderContext)], drc0;
		wdf		drc0;
	#endif
	
	#if defined(SGX_FEATURE_MTE_STATE_FLUSH)
		/* Store out the MTE state */
		#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
		/* Save out the state for each enabled core. */
		mov		r2, #OFFSET(SGXMKIF_HWRENDERCONTEXT.sMTEStateFlushDevAddr);
		iaddu32	r2, r2.low, r1;
		COPYMEMTOCONFIGREGISTER_TA(HTA_MTE_STATE, EUR_CR_MTE_STATE_FLUSH_BASE, r2, r3, r4, r5);
		#else
		MK_MEM_ACCESS(ldad) 	r3, [r1, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sMTEStateFlushDevAddr)], drc0;
		wdf		drc0;
			#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && !defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
		/* Only get the first core to output the MTE state */
		mov		r2, #1;
			#endif /* SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH && !SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH */
		WRITECORECONFIG(r2, #EUR_CR_MTE_STATE_FLUSH_BASE >> 2, r3, r4);
		#endif

		#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
		MK_LOAD_STATE_CORE_COUNT_TA(r2, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		isub16	r2, r2, #1;
		#endif
		HTA_WaitForNextCoreState:
		{
			/* State the state store */
			WRITECORECONFIG(r2, #EUR_CR_MTE_STATE >> 2, #EUR_CR_MTE_STATE_FLUSH_MASK, r3);
			
			/* Wait for the MTE state of this core to be saved. */
			ENTER_UNMATCHABLE_BLOCK;
			HTA_WaitForMTEStateFlush:
			{
				READCORECONFIG(r3, r2, #EUR_CR_EVENT_STATUS2 >> 2, drc0);
				wdf		drc0;
				shl.testns	p1, r3, #(31 - EUR_CR_EVENT_STATUS2_MTE_STATE_FLUSHED_SHIFT);
				p1	ba	HTA_WaitForMTEStateFlush;
			}
			LEAVE_UNMATCHABLE_BLOCK;
	
			mov		r3, #EUR_CR_EVENT_HOST_CLEAR2_MTE_STATE_FLUSHED_MASK;
			WRITECORECONFIG(r2, #EUR_CR_EVENT_HOST_CLEAR2 >> 2, r3, r4);
		
		#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
			/* If >= 0 carry on */
			isub16.testns	r2, p1, r2, #1;
			p1	ba	HTA_WaitForNextCoreState;
		#endif
		}
	#endif /* SGX_FEATURE_MTE_STATE_FLUSH */
	
	#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && defined(EUR_CR_MASTER_VDM_FENCE_STATUS)
		SWITCHEDMTOTA();
		MK_MEM_ACCESS(ldad)	r3, [r1, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sMasterVDMSnapShotBufferDevAddr)], drc0;
		READMASTERCONFIG(r4, #EUR_CR_MASTER_VDM_FENCE_STATUS >> 2, drc0);
		wdf		drc0;
		/* Realign to a byte address */
		shl		r3, r3, #EURASIA_VDM_SNAPSHOT_BUFFER_ALIGNSHIFT;
		MK_MEM_ACCESS_BPCACHE(stad)	[r3, #28], r4;
		idf		drc0, st;
		wdf		drc0;
		SWITCHEDMBACK();
	#endif
	
	#if defined(SGX_FEATURE_COMPLEX_GEOMETRY_REV_2)
		MK_MEM_ACCESS_CACHED(ldad)	r1, [r0, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
		wdf		drc0;
		mov		r3, #SGXMKIF_TAFLAGS_CMPLX_GEOMETRY_PRESENT;
		and.testz	p0, r1, r3;
		p0	ba	HTA_NoGSGStore;
		{
			/* Store out the GSG state, the store addr is already setup */
			str		#EUR_CR_GSG_STATE >> 2, #EUR_CR_GSG_STATE_STORE_MASK;
			
			/* Get the TA HWRenderDetails */
			MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderDetails)], drc0;
			/* make sure we capture the complex geometry pointer */
			ldr		r3, #EUR_CR_MTE_1ST_PHASE_COMPLEX_POINTER >> 2, drc0;
			wdf		drc0;
			/* Remember the value for the resume */
			or		r3, r3, #EUR_CR_MTE_1ST_PHASE_COMPLEX_RESTORE_LOAD_MASK;
			MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32MTE1stPhaseComplexPtr)], r3;
		
			MK_TRACE(MKTC_HTA_SAVE_COMPLEX_PTR, MKTF_TA | MKTF_SCH | MKTF_CSW);
	
			ENTER_UNMATCHABLE_BLOCK;
			HTA_WaitForGSGStateFlush:
			{
				ldr		r1, #EUR_CR_EVENT_STATUS2 >> 2, drc0;
				wdf		drc0;
				shl.testns	p1, r1, #(31 - EUR_CR_EVENT_STATUS2_GSG_FLUSHED_SHIFT);
				p1	ba	HTA_WaitForGSGStateFlush;
			}
			LEAVE_UNMATCHABLE_BLOCK;
	
			mov		r1, #EUR_CR_EVENT_HOST_CLEAR2_GSG_FLUSHED_MASK;
			str		#EUR_CR_EVENT_HOST_CLEAR2 >> 2, r1;
		}
		HTA_NoGSGStore:
	#endif
	}
	HTA_EmitLoopBack:

	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], drc0;
	wdf		drc0;

	#if defined(FIX_HW_BRN_33657)
	MK_MEM_ACCESS(ldad)	r1, [r2, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc1;
	wdf		drc1;
	or		r1, r1, #SGXMKIF_HWRTDATA_RT_STATUS_VDMCTXSTORE;
	MK_MEM_ACCESS(stad)	[r2, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], r1;
	#endif
	
	/* Make sure the RT status flags are loaded */
	MK_MEM_ACCESS(ldad)	r1, [r2, #DOFFSET(SGXMKIF_HWRTDATA.sRTStatusDevAddr)], drc1;
	wdf		drc1;
	MK_MEM_ACCESS(ldad)	r2, [r1], drc0;
	wdf		drc0;
	/* Record that we have context switched on this scene for later use */
	or		r2, r2, #SGXMKIF_HWRTDATA_RT_STATUS_VDMCTXSWITCH;
	MK_MEM_ACCESS(stad)	[r1], r2;
	idf		drc0, st;
	wdf		drc0;

	/* Check the queues */
	FIND3D();
	FINDTA();

	MK_TRACE(MKTC_HALTTA_END, MKTF_TA | MKTF_SCH | MKTF_CSW);

	/* return to calling code */
	lapc;
}

/*****************************************************************************
 ResumeTA routine - routine for handling the resuming of a VDM Context
 					switch.

 inputs:	r0 - SGXMKIF_HWRENDERCONTEXT of the TA being resumed
 			r1 - SGXMKIF_CMDTA of the TA being resumed
 temps:		r2, r3, r4, r5, r6, r7, r8
 predicate	p0, p1
*****************************************************************************/
ResumeTA:
{
	MK_TRACE(MKTC_RESUMETA, MKTF_TA | MKTF_SCH | MKTF_CSW);
#if defined(DEBUG)
	/* Count number of resumes */
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32NumTAContextResumes)], drc0;
	mov		r3, #1;
	wdf		drc0;
	iaddu32		r2, r3.low, r2;
	MK_MEM_ACCESS_BPCACHE(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32NumTAContextResumes)], r2;
	idf	drc0, st;
	wdf	drc0;
#endif
	/* Reload the previously stored VDM state */
	#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH)
	MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sMasterVDMSnapShotBufferDevAddr)], drc0;
	wdf		drc0;
	MK_TRACE_REG(r2, MKTF_TA | MKTF_SCH | MKTF_CSW);
	/* Setup the registers to store out the state */
	WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_CONTEXT_STORE_SNAPSHOT >> 2, r2, r3);

	#endif
	#if defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
	mov				r2, #OFFSET(SGXMKIF_HWRENDERCONTEXT.sVDMSnapShotBufferDevAddr[0]);
	iaddu32			r2, r2.low, r0;
	COPYMEMTOCONFIGREGISTER_TA(RTA_VDMSnapShotBuffer, EUR_CR_VDM_CONTEXT_STORE_SNAPSHOT, r2, r3, r4, r5);
	#endif
#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG) && defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	/* Dump out the VDM context store snapshot buffer (if this is resume) */
	mov		r2, #20; /* size in dwords */
	ldr		r3, #EUR_CR_VDM_CONTEXT_STORE_SNAPSHOT >> 2, drc0;
	wdf		drc0;
	/* byte-align the address */
	shl		r3, r3, #4;
	MK_TRACE_REG(r3, MKTF_TA | MKTF_SCH | MKTF_CSW);

	SWITCHEDMTOTA();
	RTA_ContextSnapshotNext:
	{
		MK_MEM_ACCESS_BPCACHE(ldad)	r4, [r3, #1++], drc0;
		wdf		drc0;
		
		MK_TRACE_REG(r4, MKTF_TA | MKTF_SCH | MKTF_CSW);
		isub16.testnz r2, p0, r2, #1;
		p0	br	RTA_ContextSnapshotNext;
	}
	SWITCHEDMBACK();
#endif

	#if defined(SGX_FEATURE_COMPLEX_GEOMETRY_REV_2)
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
	wdf		drc0;
	mov		r3, #SGXMKIF_TAFLAGS_CMPLX_GEOMETRY_PRESENT;
	and.testz	p0, r2, r3;
	p0	ba	RTA_NoCmplxGeomPtrRestore;
	{
		MK_TRACE(MKTC_RTA_CMPLX_GEOM_PRESENT, MKTF_TA | MKTF_SCH | MKTF_CSW);

		/* Load in the GSG state, the load addr is already setup */
		str		#EUR_CR_GSG_STATE >> 2, #EUR_CR_GSG_STATE_LOAD_MASK;

		MK_MEM_ACCESS_CACHED(ldad)	r2, [r1, #DOFFSET(SGXMKIF_CMDTA.sHWRenderDetailsDevAddr)], drc0;
		/* Wait for the HWRender Details addr to load */
		wdf		drc0;
		MK_MEM_ACCESS(ldad)	r2, [r2, #DOFFSET(SGXMKIF_HWRENDERDETAILS.ui32MTE1stPhaseComplexPtr)], drc0;
		/* Wait for the 1st Phase pointer to load before writing it */
		wdf		drc0;
		str		#EUR_CR_MTE_1ST_PHASE_COMPLEX_RESTORE >> 2, r2;

		ENTER_UNMATCHABLE_BLOCK;
		RTA_WaitForGSGStateLoad:
		{
			ldr		r4, #EUR_CR_EVENT_STATUS2 >> 2, drc0;
			wdf		drc0;
			shl.testns	p1, r4, #(31 - EUR_CR_EVENT_STATUS2_GSG_LOADED_SHIFT);
			p1	ba	RTA_WaitForGSGStateLoad;
		}
		LEAVE_UNMATCHABLE_BLOCK;

		mov		r4, #EUR_CR_EVENT_HOST_CLEAR2_GSG_LOADED_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR2 >> 2, r4;
	}
	RTA_NoCmplxGeomPtrRestore:
	#endif

	#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) // SGX_SUPPORT_MASTER_ONLY_SWITCHING
	/*
	 *	Start master VDM context load
	 */
		#if defined(FIX_HW_BRN_30893) || defined(FIX_HW_BRN_30970)
	/* Check that master VDM still has work */
	MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32MasterVDMCtxStoreStatus)], drc0;
	mov		r3, #(SGX_MK_VDM_CTX_STORE_STATUS_NA_MASK	|	\
				  SGX_MK_VDM_CTX_STORE_STATUS_COMPLETE_MASK);
	wdf		drc0;

	xor.testz	p0, r2, r3;
	p0	ba	RTA_AllReady;
		#endif /* FIX_HW_BRN_30893) || FIX_HW_BRN_30970 */

	{
		WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_CONTEXT_LOAD_START >> 2, \
							#EUR_CR_MASTER_VDM_CONTEXT_LOAD_START_PULSE_MASK, r4);
	
		/* Wait for the load to finish */
		ENTER_UNMATCHABLE_BLOCK;
		RTA_WaitForSnapshotLoad:
		{
			READMASTERCONFIG(r2, #EUR_CR_MASTER_VDM_CONTEXT_LOAD_STATUS >> 2, drc0);
			wdf		drc0;
			and.testz	p1, r2, #EUR_CR_MASTER_VDM_CONTEXT_LOAD_STATUS_COMPLETE_MASK;
			p1 ba RTA_WaitForSnapshotLoad;
		}
		LEAVE_UNMATCHABLE_BLOCK;
	
		/* Clear the bit */
		WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_CONTEXT_LOAD_STATUS_CLEAR >> 2, \
							#SGXMK_VDM_CONTEXT_LOAD_STATUS_CLEAR_MASK, r4);
	}
	/*
	 *	Finish master VDM context load
	 */
	#endif /* SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH */
	
	#if !defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
	{
		/*
		 *	Start VDM context load
		 */
		str		#EUR_CR_VDM_CONTEXT_LOAD_START >> 2, #EUR_CR_VDM_CONTEXT_LOAD_START_PULSE_MASK;
	
		/* Wait for the load to finish */
		ENTER_UNMATCHABLE_BLOCK;
		RTA_WaitForSnapshotLoad:
		{
		#if defined(EUR_CR_VDM_CONTEXT_LOAD_STATUS)
			ldr		r2, #EUR_CR_VDM_CONTEXT_LOAD_STATUS >> 2, drc0;
			wdf		drc0;
			shl.testns	p1, r2, #(31 - EUR_CR_VDM_CONTEXT_LOAD_STATUS_COMPLETE_SHIFT);
		#else
			ldr		r2, #EUR_CR_EVENT_STATUS2 >> 2, drc0;
			wdf		drc0;
			shl.testns	p1, r2, #(31 - EUR_CR_EVENT_STATUS2_VDM_CONTEXT_LOAD_SHIFT);
		#endif
			p1 	ba 	RTA_WaitForSnapshotLoad;
		}
		LEAVE_UNMATCHABLE_BLOCK;
	
		/* Clear the bit */
		#if defined(EUR_CR_VDM_CONTEXT_LOAD_STATUS)
		str		#EUR_CR_VDM_CONTEXT_LOAD_STATUS_CLEAR >> 2, #EUR_CR_VDM_CONTEXT_LOAD_STATUS_CLEAR_PULSE_MASK;
		#else
		mov		r3, #EUR_CR_EVENT_HOST_CLEAR2_VDM_CONTEXT_LOAD_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR2 >> 2, r3;
		#endif
		/*
		 *	Finish VDM context load
		 */
	}
	#endif /* !SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH && defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH) */

	MK_TRACE(MKTC_RTA_CONTEXT_LOADED, MKTF_TA | MKTF_SCH | MKTF_CSW);

	/*
	  	Kick the tasks required to reload:
		1) MTE state
		2) secondary attributes
	*/
	.align 2;
	/* The following limms will be replaced at runtime with the ui32PDSMTEStateRestore words */
	RTA_MTEStateRestore0:
	mov		r7, #0xDEADBEEF;
	RTA_MTEStateRestore1:
	mov		r8, #0xDEADBEEF;

	#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && !defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
	/* Reload MTE state on all cores */
	WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_CONTEXT_STORE_STATE0 >> 2, r7, r4);
	WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_CONTEXT_STORE_STATE1 >> 2, r8, r4);

	WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_TASK_KICK >> 2, \
						#EUR_CR_MASTER_VDM_TASK_KICK_PULSE_MASK, r4);

	/* Wait for the load to finish */
	ENTER_UNMATCHABLE_BLOCK;
	RTA_WaitForMasterMTETaskKick:
	{
		READMASTERCONFIG(r2, #EUR_CR_MASTER_VDM_TASK_KICK_STATUS >> 2, drc0);
		wdf		drc0;
		and.testz	p1, r2, #EUR_CR_MASTER_VDM_TASK_KICK_STATUS_COMPLETE_MASK;
		p1 ba RTA_WaitForMasterMTETaskKick;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the bit */
	WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_TASK_KICK_STATUS_CLEAR >> 2, \
						#SGXMK_VDM_TASK_KICK_CLEAR_MASK, r4);
	#else
		#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) // SGX_SUPPORT_MASTER_ONLY_SWITCHING
	/*
	 ** Slave core MTE state load is managed in the master.
	 *	The master will schedule the task on each slave core.
	 */
	str		#EUR_CR_VDM_CONTEXT_STORE_STATE0 >> 2, r7;
	str		#EUR_CR_VDM_CONTEXT_STORE_STATE1 >> 2, r8;

	WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_TASK_KICK >> 2, \
					  #EUR_CR_MASTER_VDM_TASK_KICK_PULSE_MASK, r4);

	/* Wait for the load to finish */
	ENTER_UNMATCHABLE_BLOCK;
	RTA_WaitForMasterMTETaskKick:
	{
		READMASTERCONFIG(r2, #EUR_CR_MASTER_VDM_TASK_KICK_STATUS >> 2, drc0);
		wdf		drc0;
		and.testz	p1, r2, #EUR_CR_MASTER_VDM_TASK_KICK_STATUS_COMPLETE_MASK;
		p1 ba RTA_WaitForMasterMTETaskKick;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the bit */
	WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_TASK_KICK_STATUS_CLEAR >> 2, \
						#SGXMK_VDM_TASK_KICK_CLEAR_MASK, r4);
		#else
	/*
	 *	Restore the MTE state on all cores
	 */
	str		#EUR_CR_VDM_CONTEXT_STORE_STATE0 >> 2, r7;
	str		#EUR_CR_VDM_CONTEXT_STORE_STATE1 >> 2, r8;

	str		#EUR_CR_VDM_TASK_KICK >> 2, #EUR_CR_VDM_TASK_KICK_PULSE_MASK;
	
			#if defined(EUR_CR_VDM_TASK_KICK_STATUS)
	/* Wait for the load to finish */
	ENTER_UNMATCHABLE_BLOCK;
	RTA_WaitForAllMTETaskKick:
	{
		ldr		r2, #EUR_CR_VDM_TASK_KICK_STATUS >> 2, drc0;
		wdf		drc0;
		and.testz	p1, r2, #EUR_CR_VDM_TASK_KICK_STATUS_COMPLETE_MASK;
		p1 ba RTA_WaitForAllMTETaskKick;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the bit */
	str		#EUR_CR_VDM_TASK_KICK_STATUS_CLEAR >> 2, #EUR_CR_VDM_TASK_KICK_STATUS_CLEAR_PULSE_MASK;
			#else
	/* Wait for the task to be kicked */
	ENTER_UNMATCHABLE_BLOCK;
	RTA_WaitForAllMTETaskKick:
	{
		ldr		r2, #EUR_CR_EVENT_STATUS2 >> 2, drc0;
		wdf		drc0;
		shl.testns	p1, r2, #(31 - EUR_CR_EVENT_STATUS2_VDM_TASK_KICKED_SHIFT);
		p1 ba RTA_WaitForAllMTETaskKick;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the bit */
	mov		r2, #EUR_CR_EVENT_HOST_CLEAR2_VDM_TASK_KICKED_MASK;
	str		#EUR_CR_EVENT_HOST_CLEAR2 >> 2, r2;
			#endif /* EUR_CR_VDM_TASK_KICK_STATUS */
		#endif /* SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH */
	#endif /* SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH && !SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH */
	
	/*
	 **	MTE state load complete **
	 */
	MK_TRACE(MKTC_RTA_MTE_STATE_KICKED, MKTF_TA | MKTF_SCH | MKTF_CSW);

	#if defined(SGX_FEATURE_MTE_STATE_FLUSH) && defined(SGX_FEATURE_COMPLEX_GEOMETRY_REV_2)
	p0	ba	RTA_No1stPhaseStateRestore;
	{	
		.align 2;
		/* The following limms will be replaced at runtime with the ui32PDSCmplxStateRestore words */
		RTA_CmplxStateRestore0:
		mov		r2, #0xDEADBEEF;
		RTA_CmplxStateRestore1:
		mov		r3, #0xDEADBEEF;

		str		#EUR_CR_VDM_CONTEXT_STORE_STATE0 >> 2, r2;
		str		#EUR_CR_VDM_CONTEXT_STORE_STATE1 >> 2, r3;
	
		str		#EUR_CR_VDM_TASK_KICK >> 2, #EUR_CR_VDM_TASK_KICK_PULSE_MASK;

		/* Wait for the task to be kicked */
		ENTER_UNMATCHABLE_BLOCK;
		RTA_WaitForCmplxMTETaskKick:
		{
			ldr		r2, #EUR_CR_EVENT_STATUS2 >> 2, drc0;
			wdf		drc0;
			shl.testns	p1, r2, #(31 - EUR_CR_EVENT_STATUS2_VDM_TASK_KICKED_SHIFT);
			p1 ba 	RTA_WaitForCmplxMTETaskKick;
		}
		LEAVE_UNMATCHABLE_BLOCK;
	
		/* Clear the bit */
		mov		r2, #EUR_CR_EVENT_HOST_CLEAR2_VDM_TASK_KICKED_MASK;
		str		#EUR_CR_EVENT_HOST_CLEAR2 >> 2, r2;
		MK_TRACE(MKTC_RTA_MTE_STATE_KICKED, MKTF_TA | MKTF_SCH | MKTF_CSW);
	}
	RTA_No1stPhaseStateRestore:
	#endif
	
	#if defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
	SWITCHEDMTOTA();
	
	/* Copy the SA buffer address to the TA3D ctl so it is ready for the restore program (if required) */
	MK_MEM_ACCESS(ldad) 	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sSABufferDevAddr)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sVDMSABufferDevAddr)], r2;
	
	MK_TRACE(MKTC_RTA_DEBUG_SAS, -1);

#if 1 // SGX_SUPPORT_MASTER_ONLY_SWITCHING
	/*
	 * Master VDM only SA state restore: only one SA buffer
	 */

	/* The first DWORD in the buffer contains the number of 16 DWORD chunks saved */
	MK_MEM_ACCESS_CACHED(ldad)	r4, [r2], drc0;
	wdf		drc0;
	MK_TRACE_REG(r4, MKTF_TA | MKTF_SCH | MKTF_CSW);

	/* Switch to EDM to patch PDS program */
	SWITCHEDMBACK();
		
	and.testz	p1,	r4, r4;
	p1		ba	NoMasterSARestore;
	{
		/* Set up the VDM secondary PDS task.
		 *	r2 = DOUTD0 = SA buffer devVaddr
		 *	r3 = DOUTD1 = DMA block size control
		 */
		
		/* 16 lines (= 256 dwords of SAs) per DMA, plus remainder (see below) */
		mov		r5, #SGX_31425_DMA_NLINES;
		
		/* Write DOUTD1 control into r3 */
		PATCH_PDS_DMA_CONTROL(r3, r5, r6, r7);
		
		/* Increment SA buffer pointer by 1 dword to skip number of stored SAs */
		mov		r5, #SIZEOF(IMG_UINT32);
		iaddu32	r2, r5.low, r2;
		MK_MEM_ACCESS(ldad) 	r5, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sDevAddrSAStateRestoreDMA_InputDOUTD0)], drc0;
		wdf		drc0;
		MK_MEM_ACCESS_BPCACHE(stad) 	[r5], r2;
		
		MK_MEM_ACCESS(ldad) 	r5, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sDevAddrSAStateRestoreDMA_InputDOUTD1)], drc0;
		wdf		drc0;
		MK_MEM_ACCESS_BPCACHE(stad) 	[r5], r3;

		/* Set up the DOUTD for remaining SAs outside 256-dword blocks.
		 */
		
		/* Write DOUTD1 control into r3 */
		and		r5, r4, #SGX_31425_DMA_NLINES_MINUS1;	/* remainder of blocks after dividing into size-16 DMA chunks */
		PATCH_PDS_DMA_CONTROL(r3, r5, r6, r7);
		and.testz	p1, r5, r5;
		p1		mov r3, #0;		/* No sub-16x16 dword DMA chunk remaining */
		
		MK_MEM_ACCESS(ldad) 	r5, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sDevAddrSAStateRestoreDMA_InputDOUTD1_REMBLOCKS)], drc0;
		wdf		drc0;
		MK_MEM_ACCESS_BPCACHE(stad) 	[r5], r3;

		/* Write the number of 256-dword blocks to restore (sets up loop limit in PDS prog) */
		MK_MEM_ACCESS(ldad) 	r5, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sDevAddrSAStateRestoreDMA_InputDMABLOCKS)], drc0;
		wdf		drc0;
		shr		r6, r4, #SGX_31425_DMA_NLINES_SHIFT;
		MK_MEM_ACCESS_BPCACHE(stad) 	[r5], r6;
		
		/* Fence the secondary PDS task patch */
		idf		drc0, st;
		wdf		drc0;
	
		/* Set up the VDM secondary USSE task */
		RTA_SARestore1:
		mov		r5, #0xDEADBEEF;
		and		r5, r5, ~#EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_USEATTRIBUTESIZE_MASK;
		/* Convert from number of 16 register chunks into 4 register chunks */
		shl		r4, r4, #2;
		/* Protect against overflow */
		and		r4, r4, #EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_USEATTRIBUTESIZE_MASK;
		or		r5, r5, r4;
		
		.align 2;
		/* The following limm will be replaced at runtime with the ui32PDSSAStateRestore words */
		RTA_SARestore0:
		mov		r4, #0xDEADBEEF;
		
	#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH)
		/* Set up the SA restore task on master	core.
		 */
		WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_CONTEXT_STORE_STATE0 >> 2, r4, r7);
		WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_CONTEXT_STORE_STATE1 >> 2, r5, r7);
	
		/* All the cores are configured. Kick the task in master core only,
		 * tasks in the slave cores will be scheduled as required by the master.
		 */
		WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_TASK_KICK >> 2, \
						  #EUR_CR_MASTER_VDM_TASK_KICK_PULSE_MASK, r4);
	
		/* Wait for the load to finish */
		ENTER_UNMATCHABLE_BLOCK;
		RTA_WaitForMasterSATaskKick:
		{
			READMASTERCONFIG(r4, #EUR_CR_MASTER_VDM_TASK_KICK_STATUS >> 2, drc0);
			wdf		drc0;
			and.testz	p1, r4, #EUR_CR_MASTER_VDM_TASK_KICK_STATUS_COMPLETE_MASK;
			p1 ba RTA_WaitForMasterSATaskKick;
		}
		LEAVE_UNMATCHABLE_BLOCK;
	
		/* Clear the bit */
		WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_TASK_KICK_STATUS_CLEAR >> 2, \
							#SGXMK_VDM_TASK_KICK_CLEAR_MASK, r4);
	#else
		str		#EUR_CR_VDM_CONTEXT_STORE_STATE0 >> 2, r4;
		str		#EUR_CR_VDM_CONTEXT_STORE_STATE1 >> 2, r5;
		str		#EUR_CR_VDM_TASK_KICK >> 2, #EUR_CR_VDM_TASK_KICK_PULSE_MASK;

		/* Wait for the task to be kicked */
		ENTER_UNMATCHABLE_BLOCK;
		RTA_WaitForSATaskKick:
		{
			ldr		r4, #EUR_CR_VDM_TASK_KICK_STATUS >> 2, drc0;
			wdf		drc0;
			and.testz	p1, r4, #EUR_CR_VDM_TASK_KICK_STATUS_COMPLETE_MASK;
			p1 ba RTA_WaitForSATaskKick;
		}
		LEAVE_UNMATCHABLE_BLOCK;

		/* Clear the bit */
		str		#EUR_CR_VDM_TASK_KICK_STATUS_CLEAR >> 2, #EUR_CR_VDM_TASK_KICK_STATUS_CLEAR_PULSE_MASK;
	#endif
	}
	NoMasterSARestore:
#else /* SGX_SUPPORT_MASTER_ONLY_SWITCHING */
	/*
	 * Slave VDM SA state restore: per-core SA buffers
	 */
		#if	defined(SGX_FEATURE_MP)
	MK_MEM_ACCESS(ldad) 	r3, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32SABufferStride)], drc0;
	wdf		drc0;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32VDMSABufferStride)], r3;
		#endif
	idf		drc0, st;

		#if defined(SGX_FEATURE_MP)
	MK_LOAD_STATE_CORE_COUNT_TA(r6, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	isub16	r6, r6, #1;
		#else
	mov		r6, #0;
		#endif
	
	RTA_RestoreNextCoreSA:
	{
		/* The first DWORD in the buffer contains the number of 16 DWORD chunks saved */
		#if defined(SGX_FEATURE_MP)
		/* Offset the address to the specific cores SAs */
		imae	r4, r6.low, r3.low, r2, u32;
		
		MK_MEM_ACCESS_CACHED(ldad)	r4, [r4], drc0;
		#else
		/* The first DWORD in the buffer contains the number of 16 DWORD chunks saved */
		MK_MEM_ACCESS_CACHED(ldad)	r4, [r2], drc0;
		#endif
		
		wdf		drc0;
		
		MK_TRACE_REG(r4, MKTF_TA | MKTF_SCH | MKTF_CSW);
		
		/* Do any SAs need restoring? */
		and.testz	p1, r4, r4;
		p1	ba	RTA_NoSARestoreRequired;
		/* Only restore SAs in cores with some geometry processing remaining */
		#if defined(FIX_HW_BRN_30893) || defined(FIX_HW_BRN_30970)
		/* temps r5, r7, r8 are available here */
		mov			r5, #OFFSET(SGXMKIF_HWRENDERCONTEXT.ui32VDMCtxStoreStatus[0]);
		iaddu32		r5, r5.low, r0;
		imae		r7, r6.low, #SIZEOF(IMG_UINT32).low, r5, u32; /* r7 = core# * sizeof(ui32) + r5 */
		MK_MEM_ACCESS_BPCACHE(ldad)	r7, [r7], drc0;
		#else
		/* Check core has work remaining */
		READCORECONFIG(r7, r6, #EUR_CR_VDM_CONTEXT_STORE_STATUS >> 2, drc0);
		#endif
		mov			r5, #(EUR_CR_VDM_CONTEXT_STORE_STATUS_NA_MASK	|	\
						  EUR_CR_VDM_CONTEXT_STORE_STATUS_COMPLETE_MASK);
		wdf			drc0;
		xor.testz	p1, r7, r5;
		p1	ba	RTA_NoSARestoreRequired;
		{
			/* The following limm will be replaced at runtime with the ui32PDSSAStateRestore words */
			RTA_SARestore1:
			mov		r5, #0xDEADBEEF;
			
			/* Modify the attribute allocation count to include the SAs required */
			and		r7, r5, #EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_USEATTRIBUTESIZE_MASK;
			and		r5, r5, ~#EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_USEATTRIBUTESIZE_MASK;
			/* Convert from number of 16 register chunks into 4 register chunks */
			shl		r4, r4, #2;
			iaddu32	r7, r4.low, r7;
			/* Protect against overflow */
			and		r7, r7, #EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_USEATTRIBUTESIZE_MASK;
			or		r5, r5, r7;
		
			.align 2;
			/* The following limm will be replaced at runtime with the ui32PDSSAStateRestore words */
			RTA_SARestore0:
			mov		r4, #0xDEADBEEF;
			
			WRITECORECONFIG(r6, #EUR_CR_VDM_CONTEXT_STORE_STATE0 >> 2, r4, r7);
			WRITECORECONFIG(r6, #EUR_CR_VDM_CONTEXT_STORE_STATE1 >> 2, r5, r7);
			WRITECORECONFIG(r6, #EUR_CR_VDM_TASK_KICK >> 2, #EUR_CR_VDM_TASK_KICK_PULSE_MASK, r7);

			/* Wait for the task to be kicked */
			ENTER_UNMATCHABLE_BLOCK;
			RTA_WaitForSATaskKick:
			{
				READCORECONFIG(r4, r6, #EUR_CR_VDM_TASK_KICK_STATUS >> 2, drc0);
				wdf		drc0;
				and.testz	p1, r4, #EUR_CR_VDM_TASK_KICK_STATUS_COMPLETE_MASK;
				p1 ba RTA_WaitForSATaskKick;
			}
			LEAVE_UNMATCHABLE_BLOCK;

			/* Clear the bit */
			WRITECORECONFIG(r6, #EUR_CR_VDM_TASK_KICK_STATUS_CLEAR >> 2, #EUR_CR_VDM_TASK_KICK_STATUS_CLEAR_PULSE_MASK, r7);
		}
		RTA_NoSARestoreRequired:

		/* Check all the slave cores */
		#if defined(SGX_FEATURE_MP)
		isub16.testns	r6, p1, r6, #1;
		p1	ba	RTA_RestoreNextCoreSA;
		#endif
	}

	SWITCHEDMBACK();
#endif /* SGX_SUPPORT_MASTER_ONLY_SWITCHING */
	#else /* SGX_FEATURE_USE_SA_COUNT_REGISTER */
	mov		r2, #OFFSET(SGXMKIF_CMDTA.sVDMSARestoreDataDevAddr[0]);
	iaddu32	r2, r2.low, r1;
	mov		r3, #(SGX_MAX_VDM_STATE_RESTORE_PROGS * SIZEOF(IMG_UINT32));
	iaddu32	r3, r3.low, r2;

	/* Switch to the TA address space so the Microkernel can read the
		client api SA restore buffer */
	SWITCHEDMTOTA();
	RTA_CheckNextStateRestoreProg:
	{
		MK_MEM_ACCESS_CACHED(ldad)		r4, [r2], drc0;
		wdf		drc0;
		and.testz	p1, r4, r4;
		p1	ba	RTA_NoRestoreProgram;
		{
			MK_MEM_ACCESS_BPCACHE(ldad.f2)		r4, [r4, #0++], drc0;
			wdf		drc0;
			or.testz	p1, r4, r5;
			p1	ba	RTA_NoRestoreProgram;
			{
	#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH)
				WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_CONTEXT_STORE_STATE0 >> 2, r4, r6);
				WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_CONTEXT_STORE_STATE1 >> 2, r5, r6);

				WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_TASK_KICK >> 2, \
									#EUR_CR_MASTER_VDM_TASK_KICK_PULSE_MASK, r6);

				/* Wait for the task to be kicked */
				ENTER_UNMATCHABLE_BLOCK;
				RTA_WaitForTaskKick:
				{
					READMASTERCONFIG(r4, #EUR_CR_MASTER_VDM_TASK_KICK_STATUS >> 2, drc0);
					wdf		drc0;
					and.testz	p1, r4, #EUR_CR_MASTER_VDM_TASK_KICK_STATUS_COMPLETE_MASK;
					p1 ba RTA_WaitForTaskKick;
				}
				LEAVE_UNMATCHABLE_BLOCK;

				/* Clear the bit */
				WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_TASK_KICK_STATUS_CLEAR >> 2, \
									#SGXMK_VDM_TASK_KICK_CLEAR_MASK, r4);
	#else
				str		#EUR_CR_VDM_CONTEXT_STORE_STATE0 >> 2, r4;
				str		#EUR_CR_VDM_CONTEXT_STORE_STATE1 >> 2, r5;

				str		#EUR_CR_VDM_TASK_KICK >> 2, #EUR_CR_VDM_TASK_KICK_PULSE_MASK;
				/* Wait for the task to be kicked */
				ENTER_UNMATCHABLE_BLOCK;
				RTA_WaitForTaskKick:
				{
					ldr		r4, #EUR_CR_EVENT_STATUS2 >> 2, drc0;
					wdf		drc0;
					shl.testns	p1, r4, #(31 - EUR_CR_EVENT_STATUS2_VDM_TASK_KICKED_SHIFT);
					p1 ba RTA_WaitForTaskKick;
				}
				LEAVE_UNMATCHABLE_BLOCK;

				/* Clear the bit */
				mov		r4, #EUR_CR_EVENT_HOST_CLEAR2_VDM_TASK_KICKED_MASK;
				str		#EUR_CR_EVENT_HOST_CLEAR2 >> 2, r4;
	#endif
			}
		}
		RTA_NoRestoreProgram:

		MK_TRACE(MKTC_RTA_CHECK_NEXT_SA_PROG, MKTF_TA | MKTF_SCH | MKTF_CSW);

		mov		r4, #SIZEOF(IMG_UINT32);
		iaddu32	r2, r4.low, r2;
		xor.testnz	p1, r2, r3;
		p1	ba	RTA_CheckNextStateRestoreProg;
	}
	SWITCHEDMBACK();
	#endif /* SGX_FEATURE_USE_SA_COUNT_REGISTER */

	#if 0 //defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
	/* Check if any of the cores have nothing to do and therefore will not flag TAFinished Event */
	mov		r2, #OFFSET(SGXMKIF_HWRENDERCONTEXT.ui32VDMCtxStoreStatus[0]);
	iaddu32	r2, r2.low, r0;

	MK_LOAD_STATE_CORE_COUNT_TA(r6, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	mov		r3, #0;
	RTA_CheckNextCore:
	{
		MK_MEM_ACCESS(ldad)	r4, [r2, #1++], drc0;		
		mov		r5, #(EUR_CR_VDM_CONTEXT_STORE_STATUS_COMPLETE_MASK | \
						EUR_CR_VDM_CONTEXT_STORE_STATUS_NA_MASK);
		wdf		drc0;

		and		r4, r4, r5;
		xor.testnz	p1, r4, r5;
		p1	ba	RTA_CoreNotComplete;
		{
			/* This core has completed the TA command, so set its TAFINISHED bit */
			mov		r4, #EUR_CR_EVENT_STATUS_TA_FINISHED_MASK;
			WRITECORECONFIG(r3, #EUR_CR_EVENT_STATUS >> 2, r4, r5);	
			MK_TRACE(MKTC_RTA_CORE_COMPLETED, MKTF_TA | MKTF_SCH | MKTF_CSW);
			MK_TRACE_REG(r3,  MKTF_TA | MKTF_SCH | MKTF_CSW);
		}
		RTA_CoreNotComplete:
		
		REG_INCREMENT(r3, p1);
		xor.testnz	p1, r3, r6;
		p1	ba	RTA_CheckNextCore;
	}
	#endif
	
	RTA_AllReady:
	MK_TRACE(MKTC_RESUMETA_END, MKTF_TA | MKTF_SCH | MKTF_CSW);

	/* return */
	lapc;
}

#else /* SGX_FEATURE_VDM_CONTEXT_SWITCH_REV_2 */
	/* compiler throws an error if file is empty */
	nop;
#endif /* SGX_FEATURE_VDM_CONTEXT_SWITCH_REV_2 */
