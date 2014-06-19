/*****************************************************************************
 Name		: sgx_init.use.asm

 Title		: USSE program for SGX initialisation

 Author 	: Imagination Technologies

 Created  	: 4th Aug 2009

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

 Description 	: USSE program for SGX initialisation

 Program Type	: USSE assembly language

 Version 	: $Revision: 1.58 $

 Modifications	:

 $Log: sgx_init.use.asm $
 
  --- Revision Logs Removed --- 
 *****************************************************************************/

#include "usedefs.h"

/***************************
 Sub-routine Imports
***************************/
/* Utility routines */
.import MKTrace;
#if defined(FIX_HW_BRN_31542) || defined(FIX_HW_BRN_36513)
.import JumpToSupervisor;
#endif
#if defined(SGX_FEATURE_2D_HARDWARE)
.import HostKick2D;
#endif /* SGX_FEATURE_2D_HARDWARE */
.import HostKickTransfer;
.import HostKickTA;

.import WriteHWPERFEntry;

#if defined(FIX_HW_BRN_27738)
#define SGX_MK_SECONDARY_LOOPBACK_DM EURASIA_PDSSB3_USEDATAMASTER_RESERVED
#else
#define SGX_MK_SECONDARY_LOOPBACK_DM EURASIA_PDSSB3_USEDATAMASTER_EVENT
#endif

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) && defined(FIX_HW_BRN_31425)
.import SetupDirListBase;
.import JumpToSupervisor;
#endif

.modulealign 2;
/*****************************************************************************
 Initialise the microkernel (primary 1)
*****************************************************************************/
.export InituKernelPrimary1;
.export IUKP1_PDSConstSizeSec;
.export IUKP1_PDSDevAddrSec;
.export IUKP1_TA3DCtl;
.export IUKP1_PDSConstSizePrim2;
.export IUKP1_PDSDevAddrPrim2;
InituKernelPrimary1:
{

	IUKP1_PDSConstSizeSec:
	mov	r0, #0xDEADBEEF;

	/*
		Ensure that the following is at the start of an instruction pair
		to prevent tampering with the patched executable address.
	*/
	.align 2;
	IUKP1_PDSDevAddrSec:
	mov	r2, #0xDEADBEEF;

	/*
		This will be patched by the host with the address of the TA3D control.
	*/
	IUKP1_TA3DCtl:
	mov	r4, #0xDEADBEEF;

	/*
		A non-zero value in r4 will cause the secondary PDS program to suppress
		DMAing the SAs. This is required when initialising after hardware recovery.
		Soft reset preserves the USSE attribute store, and it is better to rely on
		the state at the time of the lockup than to revert to the last saved state.
	*/
	MK_MEM_ACCESS_BPCACHE(ldad)	r4, [r4, #DOFFSET(SGXMK_TA3D_CTL.sMKSecAttr.sHostCtl)], drc0;
	wdf				drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r4, [r4, #DOFFSET(SGXMKIF_HOST_CTL.ui32InterruptClearFlags)], drc0;
	wdf				drc0;
	and				r4, r4, #PVRSRV_USSE_EDM_INTERRUPT_HWR;

	/*
		Loop back to InituKernelSecondary.
		Note that even numbered registers must be used for the sources in order
		to support 64-bit encoding.
	*/
	emitpds.tasks.taske #0, r0, r2, r4,	#(EURASIA_PDSSB3_USEATTRIBUTEPERSISTENCE	|						\
							(SGX_MK_SECONDARY_LOOPBACK_DM << EURASIA_PDSSB3_USEDATAMASTER_SHIFT) |			\
							EURASIA_PDSSB3_PDSSEQDEPENDENCY | 												\
                        	(1 << EURASIA_PDSSB3_USEINSTANCECOUNT_SHIFT));

	#if defined(SGX_FAST_DPM_INIT) && (SGX_FEATURE_NUM_USE_PIPES > 1)
	/*
		InitPBLoopback and the TA handler run on pipe 1, so
		secondaries need to be allocated for that as well.
	*/
	#if defined(EURASIA_PDSSB3_USEPIPE_PIPE1)
		emitpds.tasks.taske #0, r0, r2, r4, #(EURASIA_PDSSB3_USEATTRIBUTEPERSISTENCE	| \
											 (SGX_MK_SECONDARY_LOOPBACK_DM << EURASIA_PDSSB3_USEDATAMASTER_SHIFT) | \
                                           	 (1 << EURASIA_PDSSB3_USEINSTANCECOUNT_SHIFT) |	\
                                           	 EURASIA_PDSSB3_USEPIPE_PIPE1);
	#else
		#if defined(EURASIA_PDSSB1_USEPIPE_SHIFT)
		and	r2, r2, #EURASIA_PDSSB1_USEPIPE_CLRMSK;
		or	r2, r2, #(EURASIA_PDSSB1_USEPIPE_PIPE1 << EURASIA_PDSSB1_USEPIPE_SHIFT);
		#else
		and	r0, r0, #EURASIA_PDSSB0_USEPIPE_CLRMSK;
		or	r0, r0, #(EURASIA_PDSSB0_USEPIPE_PIPE1 << EURASIA_PDSSB0_USEPIPE_SHIFT);
		#endif
		emitpds.tasks.taske #0, r0, r2, r4, #(EURASIA_PDSSB3_USEATTRIBUTEPERSISTENCE | \
											 (SGX_MK_SECONDARY_LOOPBACK_DM << EURASIA_PDSSB3_USEDATAMASTER_SHIFT) | \
                                           	 (1 << EURASIA_PDSSB3_USEINSTANCECOUNT_SHIFT));
	#endif /* EURASIA_PDSSB3_USEPIPE_PIPE1 */
	#endif /* defined(SGX_FAST_DPM_INIT) && (SGX_FEATURE_NUM_USE_PIPES > 1) */

	IUKP1_PDSConstSizePrim2:
	mov	r0, #0xDEADBEEF;

	/*
		Ensure that the following is at the start of an instruction pair
		to prevent tampering with the patched executable address.
	*/
	.align 2;
	IUKP1_PDSDevAddrPrim2:
	mov	r2, #0xDEADBEEF;

	mov r4, #0;

	/*
		Loop back to InituKernelPrimary2, which is dependent on the
		completion of InituKernelSecondary.
		Note that even numbered registers must be used for the sources in order
		to support 64-bit encoding.
	*/
	emitpds.tasks.taske #0, r0, r2, r4,	#(EURASIA_PDSSB3_PDSSEQDEPENDENCY | 								\
							(EURASIA_PDSSB3_USEDATAMASTER_EVENT << EURASIA_PDSSB3_USEDATAMASTER_SHIFT) |	\
                        	(1 << EURASIA_PDSSB3_USEINSTANCECOUNT_SHIFT));

	UKERNEL_PROGRAM_PHAS();

	nop.end;
}


/*****************************************************************************
 Initialise the microkernel (secondary)
*****************************************************************************/
.export InituKernelSecondary;
.align 2;
InituKernelSecondary:
{
	UKERNEL_PROGRAM_PHAS();

#if defined(FIX_HW_BRN_31988)
	/* predicated load of size 128-bits */
	.align 2, 1;
	nop.nosched;
	and.testz.skipinv	p0, #1, #1;
	p0	ldad.skipinv.f4		pa0,	[pa0, #1++], drc0;
	wdf		drc0;
#endif
	/* Nothing to do because the secondary attributes have been DMAd by the PDS. */

	nop.end;
}


/*****************************************************************************
 Initialise the microkernel (primary 2)
*****************************************************************************/
.export InituKernelPrimary2;
.export IUKP2_PDSExec;
.export IUKP2_PDSData;
#if defined(FIX_HW_BRN_31542) || defined(FIX_HW_BRN_36513)
.export	IKP_LIMMUSECodeBaseAddr;
.export	IKP_LIMMPSGRgnHdrAddr;
.export	IKP_LIMMTPCBaseAddr;
.export	IKP_LIMMParamMemAddr;
.export	IKP_LIMMDPMPTBaseAddr;
.export	IKP_LIMMVDMStreamBaseAddr;
#endif

.align 2;
InituKernelPrimary2:
{
	UKERNEL_PROGRAM_INIT_COMMON();

	/*
		Update the PDS executable address and data associated with a host kick.
	*/
	.align 2;
	IUKP2_PDSExec:
	mov	r0, #0xDEADBEEF;
	str		#(SGX_MP_CORE_SELECT(EUR_CR_EVENT_OTHER_PDS_EXEC, 0)) >> 2, r0;

	IUKP2_PDSData:
	mov	r0, #0xDEADBEEF;
	str		#(SGX_MP_CORE_SELECT(EUR_CR_EVENT_OTHER_PDS_DATA, 0)) >> 2, r0;

#if defined(SGX_FEATURE_MP)
	/* Record how many cores are enabled */
	READMASTERCONFIG(r0, #EUR_CR_MASTER_CORE >> 2, drc0);
	wdf		drc0;
	#if defined(SGX_FEATURE_MP_PLUS)
	/* Extract the TA and 3D core count */
	mov		r1, r0;
	and		r0, r0, #EUR_CR_MASTER_CORE_ENABLE_MASK;
	shr		r0, r0, #EUR_CR_MASTER_CORE_ENABLE_SHIFT;
	and		r1, r1, #EUR_CR_MASTER_CORE_ENABLE_3D_MASK;
	shr		r1, r1, #EUR_CR_MASTER_CORE_ENABLE_3D_SHIFT;
	/* Add 1 to the value, as the register is 0 based */
	REG_INCREMENT(r0, p0);
	REG_INCREMENT(r1, p0);
	/* Shift the fields for ukernel state */
	shl		r0, r0, #USE_CORE_COUNT_TA_SHIFT;
	shl		r1, r1, #USE_CORE_COUNT_3D_SHIFT;
	or		r0, r0, r1;
	#else
	/* Add 1 to the value, as the register is 0 based */
	REG_INCREMENT(r0, p0);
	#endif /* defined(SGX_FEATURE_MP_PLUS) */
	MK_STORE_STATE(ui32CoresEnabled, r0);
	MK_WAIT_FOR_STATE_STORE(drc0);
#endif

	/*
		Initialise configuration registers.
	*/
	bal	InitConfigRegisters;

	/*
		Initialise registers associated with hardware perf counters.
	*/
	xor.testz	p0, r0, r0; /* pass p0 as true to always clear the counters */
	bal UpdateHWPerfRegs;

#if defined(FIX_HW_BRN_31272) || defined(FIX_HW_BRN_31780) || defined(FIX_HW_BRN_33920)
        /*
	        PTLA Req Status should be cleared before the complete HW recovery
	        as corresponding HW registers are auto cleared by HW.
        */
        MK_STORE_STATE(ui32DummyFillCnt, #0x00000000UL);
        MK_WAIT_FOR_STATE_STORE(drc0);
#endif   



	/*
		Check for hardware recovery flag.
	*/
	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32InterruptClearFlags)], drc0;
	wdf				drc0;
	and.testnz		p0, r0, #PVRSRV_USSE_EDM_INTERRUPT_HWR;
	p0				bal	HWRecovery;

#if defined(FIX_HW_BRN_31542) || defined(FIX_HW_BRN_36513)
	/*******************************
	 * r0 - BIF_BANK stored value
	 * r1 - PIM_SPLIT_THRESHOLD stored value
	 * r2 - USE_CODE_BASE(14) stored value
	 * r3 - PDS events
	 * r9 - ui32CoresEnabled
	 * r4 - r10: temps
	 *******************************/
	MK_LOAD_STATE(r9, ui32CoresEnabled, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	ldr r0, #EUR_CR_BIF_BANK0 >> 2, drc0;
	wdf drc0;
	mov r4, r0;
	and r5, r4, #EUR_CR_BIF_BANK0_INDEX_EDM_MASK;
	shl r5, r5, #(EUR_CR_BIF_BANK0_INDEX_TA_SHIFT - EUR_CR_BIF_BANK0_INDEX_EDM_SHIFT);

	and r4, r4, ~#EUR_CR_BIF_BANK0_INDEX_TA_MASK;
	or r4, r4, r5;
	SGXMK_SPRV_WRITE_REG(#EUR_CR_BIF_BANK0 >> 2 , r4);

	ldr r1, #EUR_CR_MASTER_MP_PRIMITIVE >> 2, drc0;
	wdf drc0;
	mov r4, #1; /* Split threshold(One triangle) */
	mov r5, #(1 << EUR_CR_MASTER_MP_PRIMITIVE_MODE_SHIFT); /* RR */
	or r4, r4, r5;

	str  #EUR_CR_MASTER_MP_PRIMITIVE >> 2, r4;

	ldr r2, #(SGX_MP_CORE_SELECT(EUR_CR_USE_CODE_BASE_14 ,0)) >> 2, drc0;
	wdf drc0;
	mov r4, #EURASIA_USECODEBASE_DATAMASTER_VERTEX;
	shl r4, r4, #EUR_CR_USE_CODE_BASE_DM_14_SHIFT;

	IKP_LIMMUSECodeBaseAddr:
	mov r5, #0xDEADBEEF;

	shr r5, r5, #EUR_CR_USE_CODE_BASE_ADDR_ALIGNSHIFT;
	or r4, r4, r5;

	mov r6, #(SGX_MP_CORE_SELECT(EUR_CR_USE_CODE_BASE_14, 0)) >> 2;
	mov r5, r9;
	mov 	r7, 	#SGX_REG_BANK_SIZE >> 2;
	IKP_USECodeBaseContinue:
	{
		SGXMK_SPRV_WRITE_REG(r6, r4);
		iaddu32		r6, r7.low, r6;
		isub16.testnz 	r5, p0, r5, #1;
		p0 	ba 	IKP_USECodeBaseContinue;
	}
	

	mov r4, #0;
	str #EUR_CR_TE_AA >> 2, r4;

	mov r4, #0x10010010; /* MT number=0, X1=X2=X3=0x10 based on 32 tiles MT4 mode */
	str #EUR_CR_TE_MTILE1 >> 2, r4;
	str #EUR_CR_TE_MTILE2 >> 2, r4;

	mov r4, #0x01f01f;
	str #EUR_CR_MTE_SCREEN >> 2, r4;

	mov r4, #0x001001;
	str #EUR_CR_TE_SCREEN >> 2, r4;

	mov r4, #0x100;
	str #EUR_CR_TE_MTILE >> 2, r4;

	mov r4, #EUR_CR_TE_PSG_COMPLETEONTERMINATE_MASK;
	str #EUR_CR_TE_PSG >> 2, r4;

	mov r4, #0x11104100; /* Arbitrary 4 partitions each, 16 tasks each */
	str #EUR_CR_DMS_CTRL >> 2, r4;
	
	ldr r4, #EUR_CR_MTE_CTRL >> 2, drc0;
	wdf	drc0;

	or r4, r4, #2; /* Global Macro-tile threshold */
	str	#EUR_CR_MTE_CTRL >> 2, r4;


	mov r4, #0x10000;
	str	#EUR_CR_MASTER_DPM_PIMSHARE_RESERVE_PAGES >> 2, r4;
	IKP_LIMMPSGRgnHdrAddr:
	mov r4, #0xDEADBEEF;
	mov r5, r9;
	mov r6, #(SGX_MP_CORE_SELECT(EUR_CR_TE_PSGREGION_BASE, 0)) >> 2;
	IKP_PSGRConfigContinue:
	{
		str	r6,	r4;
		mov	r7,	#SGX_REG_BANK_SIZE >> 2;
		iaddu32	r6,	r7.low, r6;
		mov	r7,	#SGX_CLIPPERBUG_PERCORE_PSGRGN_SIZE;	
		iaddu32		r4, r7.low, r4;
		isub16.testnz 	r5, p0, r5, #1;
		p0 	ba 	IKP_PSGRConfigContinue;
	}

	mov r4, #1;
	str	#EUR_CR_TE_RGNHDR_INIT >> 2, r4;
	ENTER_UNMATCHABLE_BLOCK;
	IKP_WaitForRgnInit:
	{
		ldr		r4, #EUR_CR_EVENT_STATUS2 >> 2, drc0;
		wdf		drc0;
		shl.testns	p0, r4, #(31 - EUR_CR_EVENT_STATUS2_TE_RGNHDR_INIT_COMPLETE_SHIFT);
		p0	ba		IKP_WaitForRgnInit;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	mov		r4, #EUR_CR_EVENT_HOST_CLEAR2_TE_RGNHDR_INIT_COMPLETE_MASK;
	str		#EUR_CR_EVENT_HOST_CLEAR2 >> 2, r4;


	IKP_LIMMTPCBaseAddr:
	mov r4, #0xDEADBEEF;
	mov r5, r9;
	mov r6, #(SGX_MP_CORE_SELECT(EUR_CR_TE_TPC_BASE, 0)) >> 2;
	IKP_TPCConfigContinue:
	{
		str 	r6, 	r4;
		mov 	r7, 	#SGX_REG_BANK_SIZE >> 2;
		iaddu32		r6, r7.low, r6;
		mov	r7, 	#SGX_CLIPPERBUG_PERCORE_TPC_SIZE;
		iaddu32		r4, r7.low, r4;
		isub16.testnz 	r5, p0, r5, #1;
		p0 	ba 	IKP_TPCConfigContinue;
	}
	
	IKP_LIMMParamMemAddr:
	mov r4, #0xDEADBEEF;
	shr r4, r4, #EUR_CR_BIF_TA_REQ_BASE_ADDR_ALIGNSHIFT;
	shl r4, r4, #EUR_CR_BIF_TA_REQ_BASE_ADDR_SHIFT;
	str #EUR_CR_BIF_TA_REQ_BASE >> 2, r4;

	/* LoadTAPB's work */
	IKP_LIMMDPMPTBaseAddr:
	mov r4, #0xDEADBEEF;

	mov r5, #EURASIA_PARAM_DPM_PAGE_LIST_PREV_TERM_MSK;
	mov r6, #1;
	mov r7, #0;

	xor.testnz p0, r7, #(SGX_CLIPPERBUG_PARAMMEM_NUM_PAGES);
	IKP_FreeListInitContinue:
	{
		or r8, r5, r6;
		imae r10, r7.low, #4, r4, u32;
		MK_MEM_ACCESS_BPCACHE(stad) [r10], r8;
		idf drc0, st;
		wdf drc0;
		!p0	ba IKP_FreeListInitComplete;
		mov r5, r7;
		shl r5, r5, #16;
		iaddu32 r7, r7.low, #1;
		xor.testnz p0, r7, #(SGX_CLIPPERBUG_PARAMMEM_NUM_PAGES);
		p0 iaddu32 r6, r6.low, #1;
		!p0 mov r6, #EURASIA_PARAM_DPM_PAGE_LIST_NEXT_TERM_MSK;
		ba	IKP_FreeListInitContinue;
	}
	IKP_FreeListInitComplete:

	and r4, r4, #EUR_CR_DPM_TA_ALLOC_PAGE_TABLE_BASE_ADDR_MASK;
	str #EUR_CR_DPM_TA_ALLOC_PAGE_TABLE_BASE >> 2, r4;


	mov r4, #(SGX_CLIPPERBUG_PARAMMEM_NUM_PAGES-1); /* head=0, tail=0x27*/
	shl r4, r4, #16;
	str #EUR_CR_DPM_TA_ALLOC_FREE_LIST>>2, r4;

	mov r4, #(SGX_CLIPPERBUG_PARAMMEM_NUM_PAGES-2); /* head=0, tail=0x26*/
	shl r4, r4, #16;
	str #EUR_CR_DPM_TA_ALLOC >> 2, r4;

	mov r4, #0x0;
	str #EUR_CR_DPM_START_OF_CONTEXT_PAGES_ALLOCATED >> 2, r4;

	mov r4, #(SGX_CLIPPERBUG_PARAMMEM_NUM_PAGES);
	str #EUR_CR_DPM_TA_PAGE_THRESHOLD>>2, r4;

	str #EUR_CR_DPM_ZLS_PAGE_THRESHOLD>>2, r4;

	mov r4, #0;
	str #EUR_CR_DPM_PDS_PAGE_THRESHOLD>>2, r4;

	/* Global list policy enabled, half the PB size */
	mov r4, #(SGX_CLIPPERBUG_PARAMMEM_NUM_PAGES/2) | (1 << EUR_CR_DPM_TA_GLOBAL_LIST_POLICY_SHIFT);
	str #EUR_CR_DPM_TA_GLOBAL_LIST>>2, r4;
	str #EUR_CR_MASTER_DPM_TA_GLOBAL_LIST>>2, r4;

	str #EUR_CR_DPM_TASK_TA_FREE >> 2, #EUR_CR_DPM_TASK_TA_FREE_LOAD_MASK;

	/* Wait for the PB load to finish */
	ENTER_UNMATCHABLE_BLOCK;
	IKP_WaitForPBLoad:
	{
		ldr r4, #EUR_CR_EVENT_STATUS2>> 2, drc0;
		wdf drc0;
		and.testz p2, r4, #EUR_CR_EVENT_STATUS2_DPM_TA_FREE_LOAD_MASK;
		p2 ba IKP_WaitForPBLoad;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	/* Clear the load event */
	str #EUR_CR_EVENT_HOST_CLEAR2 >> 2, #EUR_CR_EVENT_HOST_CLEAR2_DPM_TA_FREE_LOAD_MASK;

	mov	r4, #(EUR_CR_DPM_STATE_CONTEXT_ID_NCOP_MASK);
	str 	#EUR_CR_DPM_STATE_CONTEXT_ID >> 2, r4;

	str		#EUR_CR_DPM_LSS_PARTIAL_CONTEXT >> 2, #0;
	str	#EUR_CR_DPM_TASK_STATE >> 2, #EUR_CR_DPM_TASK_STATE_CLEAR_MASK;
	ENTER_UNMATCHABLE_BLOCK;
	IKP_WaitForLSSClear:
	{
		ldr r4, #EUR_CR_EVENT_STATUS>> 2, drc0;
		wdf drc0;
		and.testz p2, r4, #EUR_CR_EVENT_STATUS_DPM_STATE_CLEAR_MASK;
		p2 ba IKP_WaitForLSSClear;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	str #EUR_CR_EVENT_HOST_CLEAR >> 2, #EUR_CR_EVENT_HOST_CLEAR_DPM_STATE_CLEAR_MASK;

	str	#EUR_CR_DPM_TASK_CONTROL >> 2, #EUR_CR_DPM_TASK_CONTROL_CLEAR_MASK;
	ENTER_UNMATCHABLE_BLOCK;
	IKP_WaitForTAACClear:
	{
		ldr r4, #EUR_CR_EVENT_STATUS>> 2, drc0;
		wdf drc0;
		mov r5, #EUR_CR_EVENT_STATUS_DPM_CONTROL_CLEAR_MASK;
		and.testz p2, r4, r5;
		p2 ba IKP_WaitForTAACClear;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	mov r4, #EUR_CR_EVENT_HOST_CLEAR_DPM_CONTROL_CLEAR_MASK;
	str #EUR_CR_EVENT_HOST_CLEAR >> 2, r4;

	IKP_LIMMVDMStreamBaseAddr:
	mov r4, #0xDEADBEEF;
	str #EUR_CR_VDM_CTRL_STREAM_BASE >> 2, r4;

	READMASTERCONFIG( r3, #EUR_CR_MASTER_EVENT_PDS_ENABLE >> 2, drc0);
	wdf drc0;
	and r4, r3, ~#EUR_CR_MASTER_EVENT_PDS_ENABLE_TA_FINISHED_MASK;
	WRITEMASTERCONFIG(#EUR_CR_MASTER_EVENT_PDS_ENABLE >> 2, r4, r5);

	/* Reset the PIM values in both the VDM and the DPM */
	WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_PIM >> 2, #EUR_CR_MASTER_VDM_PIM_CLEAR_MASK, r8);
	WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_TASK_DPLIST >> 2, #EUR_CR_MASTER_DPM_TASK_DPLIST_CLEAR_MASK, r8);

	str #EUR_CR_VDM_START >> 2, #EUR_CR_VDM_START_PULSE_MASK;

	ENTER_UNMATCHABLE_BLOCK;
	IKP_WaitForTAFinished:
	{
		ldr r4, #EUR_CR_EVENT_STATUS >> 2, drc0;
		wdf drc0;
		shl.testns p0, r4, #(31 - EUR_CR_EVENT_STATUS_TA_FINISHED_SHIFT);
		p0 ba IKP_WaitForTAFinished;
	}
	LEAVE_UNMATCHABLE_BLOCK;

	mov r4, #EUR_CR_EVENT_HOST_CLEAR_TA_FINISHED_MASK;
	str #EUR_CR_EVENT_HOST_CLEAR >> 2, r4;

	/* Restore state back */
	SGXMK_SPRV_WRITE_REG(#EUR_CR_BIF_BANK0 >> 2, r0);
	str #EUR_CR_MASTER_MP_PRIMITIVE >> 2, r1;

	mov r6, #(SGX_MP_CORE_SELECT(EUR_CR_USE_CODE_BASE_14, 0)) >> 2;
	mov r5, r9;
	mov 	r7, 	#SGX_REG_BANK_SIZE >> 2;
	IKP_USECodeBaseRestoreContinue:
	{
		SGXMK_SPRV_WRITE_REG(r6, r2);
		iaddu32		r6, r7.low, r6;
		isub16.testnz 	r5, p0, r5, #1;
		p0 	ba 	IKP_USECodeBaseRestoreContinue;
	}
	
	WRITEMASTERCONFIG(#EUR_CR_MASTER_EVENT_PDS_ENABLE >> 2, r3, r5);
                /* Make sure we clear the TPC */
        mov             r3, #EUR_CR_TE_TPCCONTROL_CLEAR_MASK;
        str             #EUR_CR_TE_TPCCONTROL >> 2, r3;

        /* Wait for the TPC to clear */
        mov             r3, #EUR_CR_EVENT_STATUS_TPC_CLEAR_MASK;
        ENTER_UNMATCHABLE_BLOCK;
        IKP_WaitForTPCClear:
        {
                ldr             r2, #EUR_CR_EVENT_STATUS >> 2, drc0;
                wdf             drc0;
                and             r2, r2, r3;
                xor.testnz      p0, r2, r3;
                p0              ba              IKP_WaitForTPCClear;
        }
	LEAVE_UNMATCHABLE_BLOCK;
	str		#EUR_CR_EVENT_HOST_CLEAR >> 2, r3;
#endif
	/*
		Unblock the host.
	*/
	MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32InitStatus)], #PVRSRV_USSE_EDM_INIT_COMPLETE;
	idf drc0, st;
	wdf drc0;
	

	/*
		Start the microkernel timer
	*/
	bal	StartTimer;

	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32HostClock)], drc0;
	wdf				drc0;
	WRITEHWPERFCB(TA, POWER_START, ANY, r0, p0, IUKP2_power_start, r2, r3, r4, r5, r6, r7, r8, r9);

	MK_TRACE(MKTC_UKERNEL_INIT, MKTF_SCH | MKTF_HW | MKTF_POW);

#if !defined(FIX_HW_BRN_29104)
	UKERNEL_PROGRAM_PHAS();
#endif

	nop.end;
}


InitConfigRegisters:
{
#if defined(FIX_HW_BRN_31109)
	READMASTERCONFIG(r0, #EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, drc0);
	wdf		drc0;
	or	r0, r0, #EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC_EN_N_MASK;
	WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_PROACTIVE_PIM_SPEC >> 2, r0, r1);
#endif

#if defined(FIX_HW_BRN_28033)
	/*
		Store the core number into g1 in each core.
	*/
	mov	r0, #(SGX_MP_CORE_SELECT(EUR_CR_USE_G1, 0)) >> 2;
	mov	r1, #0;
	mov	r2, #(SGX_REG_BANK_SIZE >> 2);
	MK_LOAD_STATE(r3, ui32CoresEnabled, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	ICR_BRN28033Loop:
	{
		str				r0, r1;
		iaddu32			r0, r2.low, r0;
		iaddu32			r1, r1.low, #1;
		/* Have we done all the enabled cores? */
		xor.testz		p0, r1, r3;
		!p0	ba			ICR_BRN28033Loop;
	}
#endif /* FIX_HW_BRN_28033 */

#if defined(FIX_HW_BRN_33309)
	#if defined(SGX_FEATURE_MP)
	/* Force TA clocks from start to finish */
	mov	r0, #(SGX_MP_CORE_SELECT(EUR_CR_TA_CLK_GATE, 0)) >> 2;
	mov	r1, #0;
	mov	r2, #(SGX_REG_BANK_SIZE >> 2);
	MK_LOAD_STATE_CORE_COUNT_TA(r3, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	#else
	mov 	r0, #(EUR_CR_TA_CLK_GATE >> 2);
	#endif
	ICR_TAClocksLoop:
	{
		str				r0, #EUR_CR_TA_CLK_GATE_START_TO_END_MASK;
	#if defined(SGX_FEATURE_MP)
		iaddu32			r0, r2.low, r0;
		iaddu32			r1, r1.low, #1;
		/* Have we done all the enabled cores? */
		xor.testz		p0, r1, r3;
		!p0	ba			ICR_TAClocksLoop;
	#endif
	}
#endif

#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && !defined(FIX_HW_BRN_31559)
	/* Enable master-only context switch */
	WRITEMASTERCONFIG(#EUR_CR_MASTER_VDM_CONTEXT_SWITCH >> 2, \
					  #EUR_CR_MASTER_VDM_CONTEXT_SWITCH_MASTER_ONLY_MASK, r2);
#endif
 
	lapc;
}


/*****************************************************************************
	UpdateHWPerfRegs - Program the hardware based on the current value of the
						ui32HWPerfFlags state word.

	input:	p0 - TRUE to clear the counters
	temps:	r0, r1, r2, r3
*****************************************************************************/
.export UpdateHWPerfRegs;
UpdateHWPerfRegs:
{
	!p0	ba UHWPR_NoClearCounters;
	{
		mov	r1, #(EUR_CR_PERF_COUNTER_0_CLR_MASK | \
				  EUR_CR_PERF_COUNTER_1_CLR_MASK | \
				  EUR_CR_PERF_COUNTER_2_CLR_MASK | \
				  EUR_CR_PERF_COUNTER_3_CLR_MASK | \
				  EUR_CR_PERF_COUNTER_4_CLR_MASK | \
				  EUR_CR_PERF_COUNTER_5_CLR_MASK | \
				  EUR_CR_PERF_COUNTER_6_CLR_MASK | \
				  EUR_CR_PERF_COUNTER_7_CLR_MASK);
		#if !defined(SGX_FEATURE_EXTENDED_PERF_COUNTERS)
		or	r1, r1, #EUR_CR_PERF_COUNTER_8_CLR_MASK;
		#endif /* SGX_FEATURE_EXTENDED_PERF_COUNTERS */
		str		#EUR_CR_PERF >> 2, r1;
		
		str		#EUR_CR_EMU_CYCLE_COUNT >> 2, #EUR_CR_EMU_CYCLE_COUNT_RESET_MASK;
		str		#EUR_CR_EMU_CYCLE_COUNT >> 2, #0;
	}
	UHWPR_NoClearCounters:

	MK_LOAD_STATE(r0, ui32HWPerfFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testz		p0, r0, r0;

#if defined(SGX_FEATURE_EXTENDED_PERF_COUNTERS)
	/* If any HW Perf flags are set, enable the hardware counters. */
	p0	mov			r1, #0;
	!p0 mov			r1, #EUR_CR_PERF_DEBUG_CTRL_ENABLE_MASK;
	str				#(EUR_CR_PERF_DEBUG_CTRL >> 2), r1;
#endif /* SGX_FEATURE_EXTENDED_PERF_COUNTERS */

	p0	ba	UHWPR_End;
	{
		/* Program the counter group(s). */
		#if defined(SGX_FEATURE_EXTENDED_PERF_COUNTERS)
			SET_PERF_GROUP(r0, r1, r2, r3, 0, 0);
			SET_PERF_GROUP(r0, r1, r2, r3, 2, 1);
			SET_PERF_GROUP(r0, r1, r2, r3, 4, 2);
			SET_PERF_GROUP(r0, r1, r2, r3, 6, 3);
			str		#EUR_CR_PERF >> 2, #0;
		#else
			MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32PerfGroup)], drc0;
			wdf	drc0;
			shl	r0, r0, #EUR_CR_PERF_COUNTER_SELECT_SHIFT;
			and	r0, r0, #EUR_CR_PERF_COUNTER_SELECT_MASK;
			str		#EUR_CR_PERF >> 2, r0;
		#endif /* SGX_FEATURE_EXTENDED_PERF_COUNTERS */
	}
	UHWPR_End:
	
	lapc;
}


.export StartTimer;
StartTimer:
{
	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32uKernelTimerClock)], drc0;
	wdf				drc0;
	or				r0, r0, #EUR_CR_EVENT_TIMER_ENABLE_MASK;
	str				#SGX_MP_CORE_SELECT(EUR_CR_EVENT_TIMER, 0) >> 2, r0;
#if defined(SUPPORT_SGX_HWPERF)
	mov				PA(ui32TimeStamp), r0;
#endif
	lapc;
}


/*****************************************************************************
	HWRecovery - All queued tasks will be cleared and the synchronisation
				 objects updated.

	input:
	temps:	r0, r1, r2, r3, r4, r5, r6
*****************************************************************************/
HWRecovery:
{
	MK_TRACE(MKTC_HWR_START, MKTF_SCH | MKTF_HWR);

	/* First, process any host kicks so they are not lost, as we will reset the WOFF and ROFF */
	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_CCBCtl, #DOFFSET(PVRSRV_SGX_CCB_CTL.ui32WriteOffset)], drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_CCBCtl, #DOFFSET(PVRSRV_SGX_CCB_CTL.ui32ReadOffset)], drc1;
	wdf				drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sKernelCCB)], drc0;
	wdf				drc1;
	wdf				drc0;

	HWR_Hostkicks:
	{
		/* check whether ROFF == WOFF */
		xor.testz		p0, r0, r1;
		p0 ba			HWR_KernelCCBEmpty;
		{
			MK_TRACE(MKTC_HWR_HKS, MKTF_SCH | MKTF_HWR);

			imae			r4, r1.low, #SIZEOF(SGXMKIF_COMMAND), r2, u32;

			{
				MK_MEM_ACCESS_CACHED(ldad)	r5, [r4, #DOFFSET(SGXMKIF_COMMAND.ui32ServiceAddress)], drc0;
				MK_MEM_ACCESS_CACHED(ldad)	r4, [r4, #DOFFSET(SGXMKIF_COMMAND.ui32Data[1])], drc0;
				wdf				drc0;
#if defined(SGX_FEATURE_2D_HARDWARE)
				mov				r3, #LADDR(HostKick2D);
				xor.testz		p0, r5, r3;
				!p0 ba			HWR_Not2DCMD;
				{
					/* It is a 2D kick, r4 = HW 2D context */
					MK_MEM_ACCESS(ldad)	r5, [r4, #DOFFSET(SGXMKIF_HW2DCONTEXT.ui32Count)], drc0;
					wdf		drc0;
					and.testnz	p0, r5, r5;
					iaddu32		r5, r5.low, #1;
					MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HW2DCONTEXT.ui32Count)], r5;
					p0	ba	HWR_IDFBeforeNext;
					{
						/* context is not already on the lit therefore we must add it */
						/* Find out the context Priority and add it to the correct list */
						MK_MEM_ACCESS(ldad)	r5, [r4, #DOFFSET(SGXMKIF_HW2DCONTEXT.sCommon.ui32Priority)], drc0;
						wdf		drc0;

						/* Find out the offset of the runlist */
						MK_LOAD_STATE_INDEXED(r8, s2DContextTail, r5, drc0, r7);
						MK_WAIT_FOR_STATE_LOAD(drc0);
						and.testz	p0, r8, r8;
						MK_STORE_STATE_INDEXED(s2DContextTail, r5, r4, r7);
						!p0	ba	HWR_No2DHeadUpdate;
						{
							MK_STORE_STATE_INDEXED(s2DContextHead, r5, r4, r6);
						}
						HWR_No2DHeadUpdate:
						/* if required link to the old tail */
						!p0	MK_MEM_ACCESS(stad)	[r8, #DOFFSET(SGXMKIF_HW2DCONTEXT.sNextDevAddr)], r4;
						!p0 MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HW2DCONTEXT.sPrevDevAddr)], r8;
					}
					ba	HWR_IDFBeforeNext;
				}
				HWR_Not2DCMD:
#endif
				mov				r3, #LADDR(HostKickTransfer);
				xor.testz		p0, r5, r3;
				!p0 ba			HWR_NotTransferCMD;
				{
					/* It is a Transfer kick, r4 = HW Render context */
					MK_MEM_ACCESS(ldad)	r5, [r4, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.ui32Count)], drc0;
					wdf		drc0;
					and.testnz	p0, r5, r5;
					iaddu32		r5, r5.low, #1;
					MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.ui32Count)], r5;
					p0	ba	HWR_IDFBeforeNext;
					{
						/* context is not already on the list therefore we must add it */
						/* Find out the context priority and add it to the correct list */
						MK_MEM_ACCESS(ldad)	r5, [r4, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sCommon.ui32Priority)], drc0;
						wdf		drc0;

						/* Find out the offset of the runlist */
						MK_LOAD_STATE_INDEXED(r8, sTransferContextTail, r5, drc0, r7);
						MK_WAIT_FOR_STATE_LOAD(drc0);
						and.testz	p0, r8, r8;
						MK_STORE_STATE_INDEXED(sTransferContextTail, r5, r4, r7);
						!p0	ba	HWR_NoTCHeadUpdate;
						{
							MK_STORE_STATE_INDEXED(sTransferContextHead, r5, r4, r6);
						}
						HWR_NoTCHeadUpdate:
						/* If required link to the Old tail */
						!p0	MK_MEM_ACCESS(stad)	[r8, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sNextDevAddr)], r4;
						!p0 MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sPrevDevAddr)], r8;
					}
					ba	HWR_IDFBeforeNext;
				}
				HWR_NotTransferCMD:
				mov				r3, #LADDR(HostKickTA);
				xor.testz		p0, r5, r3;
				!p0 ba			HWR_NextHostKick;
				{
					/* It is a TA kick, r4 = HW Render Context. */
					/* Increment the TA count and add to partial renders tail if non-zero. */
					MK_MEM_ACCESS(ldad)	r5, [r4, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32TACount)], drc0;
					wdf				drc0;
					and.testnz		p0, r5, r5;
					iaddu32			r5, r5.low, #1;
					MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32TACount)], r5;
					/* Only add if this is only kick and context has no mid scene renders */
					p0 ba			HWR_IDFBeforeNext;
					{
						/* check whether this context has mid scene renders */
						MK_MEM_ACCESS(ldad)	r5, [r4, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPartialRendersHead)], drc0;
						wdf		drc0;
						and.testnz	p0, r5, r5;
						p0	ba	HWR_IDFBeforeNext;
						{
							/* context is not already on the list therefore we must add it */
							/* Find out the context priority and add it to the correct list */
							MK_MEM_ACCESS(ldad)	r5, [r4, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sCommon.ui32Priority)], drc0;
							wdf		drc0;

							/* Find out the offset of the runlist */
							MK_LOAD_STATE_INDEXED(r8, sPartialRenderContextTail, r5, drc0, r7);
							MK_WAIT_FOR_STATE_LOAD(drc0);
							and.testz		p0, r8, r8;
							MK_STORE_STATE_INDEXED(sPartialRenderContextTail, r5, r4, r7);
							!p0	ba	HWR_IDFBN_NoHeadUpdate;
							{
								MK_STORE_STATE_INDEXED(sPartialRenderContextHead, r5, r4, r6);
							}
							HWR_IDFBN_NoHeadUpdate:
							/* if required link to old Tail */
							!p0 MK_MEM_ACCESS(stad)	[r8, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], r4;
							!p0 MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], r8;
						}
					}
					HWR_IDFBeforeNext:
					idf				drc0, st;
					wdf				drc0;
				}
			}
			HWR_NextHostKick:
			iaddu32			r1, r1.low, #1;
			and				r1, r1, #0xFF;
			ba				HWR_Hostkicks;
		}
	}
	HWR_KernelCCBEmpty:
#if defined(SGX_FEATURE_2D_HARDWARE)
	HWR_2DListStart:
	{
		MK_TRACE(MKTC_HWR_2DL, MKTF_SCH | MKTF_HWR);

		/* check whether a 2D blit was actually in progress */
		MK_LOAD_STATE(r1, ui32I2DFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and.testz	p0, r1, #TWOD_IFLAGS_2DBLITINPROGRESS;
		p0	ba		HWR_PartialListStart;
		{
			MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s2DContext)], drc0;
			wdf				drc0;

			/* There was a 2D blit in progress, so mark for Dummy processing */
			MK_MEM_ACCESS(ldad) r2, [r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sCommon.ui32Flags)], drc0;
			wdf		drc0;
			or		r2, r2, #SGXMKIF_HWCFLAGS_DUMMY2D;
			MK_MEM_ACCESS(stad) [r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.sCommon.ui32Flags)], r2;
		}
	}
#endif
	HWR_PartialListStart:
	{
		MK_TRACE(MKTC_HWR_PRL, MKTF_SCH | MKTF_HWR);

		/* check whether the TA was running at the point of lockup */
		MK_LOAD_STATE(r0, ui32ITAFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and.testz	p0, r0, #TA_IFLAGS_TAINPROGRESS;
		p0	ba	HWR_PL_TANotActive;
		{
			/* TA was active so reset the cmds RTDATA.ui32Status if it was a first kick */
			MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTACCBCmd)], drc0;
			wdf		drc0;
			MK_MEM_ACCESS_CACHED(ldad)	r1, [r0, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
			mov		r2, #(SGXMKIF_TAFLAGS_FIRSTKICK | SGXMKIF_TAFLAGS_RESUMEDAFTERFREE);
			wdf		drc0;
			and.testz	p0, r1, r2;
			p0	ba		HWR_PL_DontResetStatus;
			{
				/* load the rtdata and reset the status */
				MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], drc0;
				wdf		drc0;
				MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], #(SGXMKIF_HWRTDATA_CMN_STATUS_READY | \
																				SGXMKIF_HWRTDATA_CMN_STATUS_DUMMYPROC);
			}
			HWR_PL_DontResetStatus:
		}
		HWR_PL_TANotActive:

		/* r0 = Partial Render Context head. */
		/* Start at priority level 0 */
		mov		r3, #0;
		MK_LOAD_STATE(r0, sPartialRenderContextHead[0], drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		/* check if there are any render contexts on the list */
		and.testz		p0, r0, r0;
		p0	ba			HWR_NextPriorityPartialList;
		{
			HWR_NextPartialRenderContext:
			{
				/* Mark the PB for this render context for re-initialisation. */
				MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPBDescDevVAddr.uiAddr)], drc0;
				wdf				drc0;
				MK_MEM_ACCESS(ldad)	r2, [r1, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], drc0;
				wdf				drc0;
				or				r2, r2, #HWPBDESC_FLAGS_INITPT;
				MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], r2;
				idf				drc0, st;
				wdf				drc0;

				/* mark the partial render details list for dummy processing */
				MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPartialRendersHead)], drc0;
				wdf				drc0;
				and.testz		p0, r1, r1;
				p0	ba			HWR_NoPartialRenders;
				{
					HWR_NextPartialRender:
					{
						MK_TRACE(MKTC_HWR_PRL_DP, MKTF_SCH | MKTF_HWR);

						/* Get the RTData associated with this scene */
						MK_MEM_ACCESS(ldad)	r2, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWRTDataDevAddr)], drc0;
						wdf		drc0;
						MK_MEM_ACCESS(ldad)	r4, [r2, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc0;
						MK_MEM_ACCESS(ldad)	r1, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sNextDevAddr)], drc1;
						wdf		drc0;
						/*
							Set the bit so that all remaining cmds and render is dummy processed,
						   	this is cleared by the next FIRSTKICK
						*/
						or		r4, r4, #SGXMKIF_HWRTDATA_CMN_STATUS_DUMMYPROC;
						MK_MEM_ACCESS(stad)	[r2, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], r4;
						/* wait for sNextDevAddr */
						wdf		drc1;
						/* check if we are finished */
						and.testnz	p0, r1, r1;
						p0	ba	HWR_NextPartialRender;
					}
				}
				HWR_NoPartialRenders:
				/* Move on to the next render context */
				MK_MEM_ACCESS(ldad)	r0, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], drc0;
				wdf				drc0;
				and.testnz		p0, r0, r0;
				p0 ba	HWR_NextPartialRenderContext;
			}
		}
		HWR_NextPriorityPartialList:
		/* Move onto the next priority */
		iaddu32		r3, r3.low, #1;
		xor.testz	p0, r3, #SGX_MAX_CONTEXT_PRIORITY;
		p0	ba	HWR_CompleteListStart;
		{
			/* Get next Head */
			MK_LOAD_STATE_INDEXED(r0, sPartialRenderContextHead, r3, drc0, r1);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			and.testnz		p0, r0, r0;
			p0 ba	HWR_NextPartialRenderContext;
			/* List was empty so try next priority */
			ba	HWR_NextPriorityPartialList;
		}
	}
	HWR_CompleteListStart:
	{
		/* Mark all the complete renders in the system for dummy processing */
		MK_TRACE(MKTC_HWR_CRL, MKTF_SCH | MKTF_HWR);

		/* Start at priority level 0 */
		mov		r3, #0;

		/* r0 = Complete Render Context head. */
		MK_LOAD_STATE(r0, sCompleteRenderContextHead[0], drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		/* check if there are any render contexts on the list */
		and.testz		p0, r0, r0;
		p0	ba			HWR_NextPriorityCompleteList;
		{
			HWR_NextCompleteRenderContext:
			{
				/* Mark the PB for this render context for re-initialisation. */
				MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPBDescDevVAddr.uiAddr)], drc0;
				wdf				drc0;
				MK_MEM_ACCESS(ldad)	r2, [r1, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], drc0;
				wdf				drc0;
				or				r2, r2, #HWPBDESC_FLAGS_INITPT;
				MK_MEM_ACCESS(stad)	[r1, #DOFFSET(SGXMKIF_HWPBDESC.ui32PBFlags)], r2;
				idf				drc0, st;
				wdf				drc0;

				/* mark the complete render details list for dummy processing */
				MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWCompleteRendersHead)], drc0;
				wdf				drc0;
				HWR_NextCompleteRender:
				{
					MK_TRACE(MKTC_HWR_CRL_DP, MKTF_SCH | MKTF_HWR);

					/* Get the RTData associated with this scene */
					MK_MEM_ACCESS(ldad)	r2, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWRTDataDevAddr)], drc0;
					wdf		drc0;
					MK_MEM_ACCESS(ldad)	r4, [r2, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc0;
					MK_MEM_ACCESS(ldad)	r1, [r1, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sNextDevAddr)], drc1;
					wdf		drc0;
					/*
						Set the bit so that all remaining cmds and render is dummy processed,
					   	this is cleared by the next FIRSTKICK
					*/
					or		r4, r4, #SGXMKIF_HWRTDATA_CMN_STATUS_DUMMYPROC;
					MK_MEM_ACCESS(stad)	[r2, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], r4;
					wdf		drc1;
					/* check if we are finished */
					and.testnz	p0, r1, r1;
					p0	ba	HWR_NextCompleteRender;
				}
				/* Move on to the next render context */
				MK_MEM_ACCESS(ldad)	r0, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextCompleteDevAddr)], drc0;
				wdf				drc0;
				and.testnz		p0, r0, r0;
				p0	ba	HWR_NextCompleteRenderContext;
			}
		}
		HWR_NextPriorityCompleteList:
		/* Move onto the next priority */
		iaddu32		r3, r3.low, #1;
		xor.testz	p0, r3, #SGX_MAX_CONTEXT_PRIORITY;
		p0	ba	HWR_TransferListStart;
		{
			/* Get next Head */
			MK_LOAD_STATE_INDEXED(r0, sCompleteRenderContextHead, r3, drc0, r1);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			and.testnz		p0, r0, r0;
			p0 ba	HWR_NextCompleteRenderContext;
			/* List was empty so try next priority */
			ba	HWR_NextPriorityCompleteList;
		}
	}
	HWR_TransferListStart:
	{
		/* Mark all the complete renders in the system for dummy processing */
		MK_TRACE(MKTC_HWR_TRL, MKTF_SCH | MKTF_HWR);

		/* check whether a fast render was actually in progress */
		MK_LOAD_STATE(r1, ui32IRenderFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and.testz	p0, r1, #RENDER_IFLAGS_TRANSFERRENDER;
		p0	ba		HWR_InternalStateCleanup;
		{
			/* There was a Transfer render in progress, so mark for Dummy processing */
			MK_LOAD_STATE(r0, s3DContext, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);

			MK_MEM_ACCESS(ldad) r2, [r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sCommon.ui32Flags)], drc0;
			wdf		drc0;
			or		r2, r2, #SGXMKIF_HWCFLAGS_DUMMYTRANSFER;
			MK_MEM_ACCESS(stad) [r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.sCommon.ui32Flags)], r2;
		}
	}
	HWR_InternalStateCleanup:
	{
		/* Clear the hardware state. */
		MK_TRACE(MKTC_HWR_ISC, MKTF_SCH | MKTF_HWR);

		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc)], #0;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DHWPBDesc)], #0;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext0RTData)], #0;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext1RTData)], #0;
		MK_STORE_STATE(ui32ITAFlags, #0);
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTACCBCmd)], #0;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], #0;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderDetails)], #0;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARenderContext)], #0;
		MK_STORE_STATE(ui32BlockedPriority, #(SGX_MAX_CONTEXT_PRIORITY - 1));
		MK_STORE_STATE(ui32IRenderFlags, #0);
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRTData)], #0;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRenderDetails)], #0;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32PendingLoopbacks)], #0;
		MK_STORE_STATE(sTransferCCBCmd, #0);
#if defined(SGX_FEATURE_2D_HARDWARE)
		MK_STORE_STATE(ui32I2DFlags, #0);
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s2DCCBCmd)], #0;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s2DContext)], #0;
#endif
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32IdleCoreRefCount)], #0;
		MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_CCBCtl, #DOFFSET(PVRSRV_SGX_CCB_CTL.ui32WriteOffset)], drc0;
		wdf drc0;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_CCBCtl, #DOFFSET(PVRSRV_SGX_CCB_CTL.ui32ReadOffset)], r0;

		/* Clear the Hardware Recovery interrupt flags. */
		MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32InterruptFlags)], drc0;
		wdf				drc0;
		and				r0, r0, #~PVRSRV_USSE_EDM_INTERRUPT_HWR;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32InterruptFlags)], r0;

		MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32InterruptClearFlags)], drc0;
		wdf				drc0;
		and				r0, r0, #~PVRSRV_USSE_EDM_INTERRUPT_HWR;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32InterruptClearFlags)], r0;
		/* Ensure all writes are in memory before */
		idf				drc0, st;
		wdf				drc0;
	}
	MK_TRACE(MKTC_HWR_END, MKTF_SCH | MKTF_HWR);
	lapc;
}


/*****************************************************************************
 End of file (sgx_init.use.asm)
*****************************************************************************/
