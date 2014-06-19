/*****************************************************************************
 Name		: sgx_timer.use.asm

 Title		: USE program for timer event processing

 Author 	: PowerVR

 Created  	: 10/12/2008

 Copyright	: 2008 by Imagination Technologies Limited. All rights reserved.
		  No part of this software, either material or conceptual
		  may be copied or distributed, transmitted, transcribed,
		  stored in a retrieval system or translated into any
		  human or computer language in any form by any means,
		  electronic, mechanical, manual or other-wise, or
		  disclosed to third parties without the express written
		  permission of Imagination Technologies Limited, HomePark
		  Industrial Estate, King's Langley, Hertfordshire,
		  WD4 8LZ, U.K.

 Description 	: USE program for timer event processing

 Program Type	: USE assembly language

 Modifications	:

 *****************************************************************************/

#include "usedefs.h"

/***************************
 Sub-routine Imports
***************************/
/* Utility routines */
.import MKTrace;
.import InvalidateBIFCache;
.import EmitLoopBack;
.import SetupDirListBase;
#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG) && defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
/* For debugging the VDM snapshot buffer */
.import SwitchEDMBIFBank;
#endif
#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
.import SetupRequestor;
#endif
.import	IdleCore;
.import Resume;

#if defined(SGX_FEATURE_MK_SUPERVISOR)
.import JumpToSupervisor;
#endif

.import	CheckTAQueue;
.import Check3DQueues;
.import Check2DQueue;

.import StoreTAPB;
.import Store3DPB;
.import StoreDPMContext;

.import WriteHWPERFEntry;

#if defined(SGX_FEATURE_SYSTEM_CACHE)
.import InvalidateSLC;
#endif

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
.import StartTAContextSwitch;
.import HaltTA;
#endif
#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH)
.import Start3DContextSwitch;
#endif

.import FreeContextMemory;
.import MoveSceneToCompleteList;
.import CommonProgramEnd;
.import WaitForReset;
#if defined(FIX_HW_BRN_32052) && defined(SGX_FEATURE_ISP_CONTEXT_SWITCH_REV_3)
.import BRN32052ClearMCIState;
#endif
/***************************
 Sub-routine Exports
***************************/
.export Timer;
.export CheckHostCleanupRequest;
.export CheckHostPowerRequest;

.modulealign 2;


#if defined(SUPPORT_HW_RECOVERY)
/*****************************************************************************
	ReadSignatureRegisters - Read a set of configuration registers and
			accumulate their values into a checksum.

	input/output:	r0 - overall signature checksum
	input:	r2 - Repeat count
			r3 - Base of configuration registers to read
			r4 - Mask applied to register value
			r5 - Increment of configuration register per repeat.
			r6 - Left shift applied to alternate configuration registers.
	temps:	p0, r7
*****************************************************************************/
ReadSignatureRegisters:
{
		ldr				r7, r3, drc0;
		wdf				drc0;

		/* Apply the mask. */
		and				r7, r7, r4;

		/* Apply the shift if necessary. */
		and.testz		p0, r2, #1;
		p0 shl			r7, r7, r6;

		/* Xor the new signature into the checksum, then rotate the checksum. */
		xor				r0, r0, r7;
		rol				r0, r0, #5;

		/* Repeat if necessary. */
		isub16.testz	r2, p0, r2, #1;
		!p0	iaddu32		r3, r5.low, r3;
		!p0 ba			ReadSignatureRegisters;

		lapc;
}


/*****************************************************************************
	CheckTALockup - Check whether any TA hardware has locked up.

	input:
	temps:	p0, r0, r1, r2, r3, r4, r5, r6, r7, r8
*****************************************************************************/
CheckTALockup:
{
	MK_TRACE(MKTC_TIMER_CTAL_START, MKTF_TIM | MKTF_HWR);

	/* Save return address. */
	mov				r8, pclink;

	/* r0 will contain all the TA signature registers checksummed together. */
	mov				r0, #0;

#if defined(SGX_FEATURE_MP)
	MK_LOAD_STATE_CORE_COUNT_TA(r1, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	iaddu32	r1, r1.low, #(SGX_REG_BANK_BASE_INDEX - 1);
	mov				r4, #EUR_CR_USE0_SERV_VERTEX_COUNT_MASK;
	mov				r5, #3;
	
	CTAL_ReadNextUseCore:
	{
		/* Move reg offset onto the next core */
	#if (SGX_FEATURE_NUM_USE_PIPES <= 2)
		mov		r2, #SGX_FEATURE_NUM_USE_PIPES;
	#else
		mov		r2, #SGX_FEATURE_NUM_USE_PIPES / 2;
	#endif
		mov		r3, #EUR_CR_USE0_SERV_VERTEX >> 2;
		mov		r6, #SGX_REG_BANK_SIZE >> 2;
		imae	r3, r1.low, r6.low, r3, u32;
		mov		r6, #16; // To avoid cancellations generating false positives
		/* Read from the core */
		bal				ReadSignatureRegisters;
		
	#if (SGX_FEATURE_NUM_USE_PIPES > 2)
		/* Read the next 2 pipes */
		mov		r2, #SGX_FEATURE_NUM_USE_PIPES / 2;
		mov		r3, #EUR_CR_USE2_SERV_VERTEX >> 2;
		mov		r6, #SGX_REG_BANK_SIZE >> 2;
		imae	r3, r1.low, r6.low, r3, u32;
		mov		r6, #16; // To avoid cancellations generating false positives
		/* Read from the core */
		bal				ReadSignatureRegisters;
	#endif
	
		/* Are there any more cores left to read? */
		isub16	r1, r1, #1;
		isub16.testnz	p0, r1, #(SGX_REG_BANK_BASE_INDEX - 1);
		p0	ba	CTAL_ReadNextUseCore;
	}
#else
	/* Check USSE vertex task counters. */
	#if (SGX_FEATURE_NUM_USE_PIPES <= 2)
	mov				r2, #SGX_FEATURE_NUM_USE_PIPES;
	#else
	mov				r2, #SGX_FEATURE_NUM_USE_PIPES / 2;
	#endif
	#if defined(EUR_CR_USE0_SERV_VERTEX)
	mov				r3, #EUR_CR_USE0_SERV_VERTEX >> 2;
	mov				r4, #EUR_CR_USE0_SERV_VERTEX_COUNT_MASK;
	#else
	mov				r3, #EUR_CR_USE_SERV_VERTEX >> 2;
	mov				r4, #EUR_CR_USE_SERV_VERTEX_COUNT_MASK;
	#endif /* EUR_CR_USE0_SERV_VERTEX */
	mov				r5, #3;
	mov				r6, #16; // To avoid cancellations generating false positives
	bal				ReadSignatureRegisters;

	#if (SGX_FEATURE_NUM_USE_PIPES > 2)
	/* Read the next 2 pipes */
	mov				r2, #SGX_FEATURE_NUM_USE_PIPES / 2;
	mov				r3, #EUR_CR_USE2_SERV_VERTEX >> 2;
	bal				ReadSignatureRegisters;
	#endif
#endif
	
#if defined(EUR_CR_TE1) || defined(EUR_CR_TE_DIAG1) || \
	(defined(EUR_CR_MTE_TE_CHECKSUM) && defined(EUR_CR_TE_CHECKSUM))
		/* Check TE Diagnostics registers. */
	#if defined(EUR_CR_MTE_MEM_CHECKSUM) && defined(EUR_CR_MTE_TE_CHECKSUM) && defined(EUR_CR_TE_CHECKSUM)
	mov				r2, #3;
	mov				r3, #EUR_CR_MTE_MEM_CHECKSUM >> 2;
	mov				r4, #~0;
	mov				r5, #1;
	mov				r6, #0;
	#else
		#if defined(EUR_CR_TE1)
	mov				r2, #2;
	mov				r3, #EUR_CR_TE1 >> 2;
	mov				r4, #~0;
	mov				r5, #1;
	mov				r6, #0;
		#else
			#if defined(EUR_CR_TE_DIAG1)
	mov				r2, #8;
	mov				r3, #EUR_CR_TE_DIAG1 >> 2;
	mov				r4, #~0;
	mov				r5, #1;
	mov				r6, #0;
			#endif /* EUR_CR_TE_DIAG1 */
		#endif /* EUR_CR_TE1 */
	#endif /* EUR_CR_*_CHECKSUM */
	bal				ReadSignatureRegisters;
#endif

	/* Check Clip Signature registers. */
	mov				r2, #1;
#if defined(EUR_CR_CLIP_CHECKSUM)
	mov				r3, #EUR_CR_CLIP_CHECKSUM >> 2;
#else
	mov				r3, #EUR_CR_CLIP_SIG1 >> 2;
#endif
	mov				r4, #~0;
	mov				r5, #1;
	mov				r6, #0;
	bal				ReadSignatureRegisters;

	/* Check whether pages are being allocated/deallocated. */
	mov				r2, #1;
	mov				r3, #EUR_CR_DPM_PAGE_STATUS >> 2;
	mov				r4, #EUR_CR_DPM_PAGE_STATUS_TA_MASK;
	mov				r5, #1;
	mov				r6, #0;
	bal				ReadSignatureRegisters;
		
	/* Check the VDM has processed any more control stream/indices. */
#if defined(SGX_FEATURE_MP)
	#if defined(EUR_CR_MASTER_VDM_CONTEXT_STORE_INDEX_ADDR)
	mov				r2, #2;
	#else
	mov				r2, #1;
	#endif
	mov				r3, #EUR_CR_MASTER_VDM_CONTEXT_STORE_STREAM >> 2;
#else
	mov				r2, #2;
	mov				r3, #EUR_CR_VDM_CONTEXT_STORE_STREAM >> 2;
#endif
	mov				r4, #~0;
	mov				r5, #1;
	mov				r6, #0;
	bal				ReadSignatureRegisters;

	/* 
		Opencl global register g1 gets updated as long as there is no lockup
	        Reading the register g1 and computing checksum to determine the lockup
        */

#if defined(SGX_FEATURE_MP)        

	MK_LOAD_STATE_CORE_COUNT_TA(r1, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	
	ReadOpenClSignatureRegisters:
	{
		isub16.testnz	r1, p0, r1, #1;
		/* 
			EUR_CR_USE_G1 is the register that gets updated as long there is no lockup 
			The code fragment below used this as a signature register to detect lockups.

		*/
		
		READCORECONFIG(r2, r1, #EUR_CR_USE_G1, drc0);
		wdf	drc0;
		xor				r0, r0, r2;
		p0	ba	ReadOpenClSignatureRegisters; 
	}
#else
	xor				r0, r0, g1;
#endif
	

	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRTASignature)], drc0;
	wdf				drc0;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRTASignature)], r0;

	/* Check if we are just invalidating the values */
	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], drc0;
	wdf		drc0;
	and.testz		p0, r1, #0x1;
	p0	ba	CTAL_NotInvalidate;
	{
		/* clear the invalidate bit */
		xor		r1, r1, #0x1;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], r1;
		ba		CTAL_Restore;
	}
	CTAL_NotInvalidate:

	/* Compare old and new values of the ui32HWRTASignature. */
	xor.testnz		p0, r0, r2;
	p0 ba			CTAL_Restore;
	{
		MK_TRACE(MKTC_TIMER_POTENTIAL_TA_LOCKUP, MKTF_TIM | MKTF_HWR | MKTF_SCH);
		
#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG) && defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
		/* Dump out the VDM context store snapshot buffer (if this is resume) */
		mov		r2, #(EURASIA_VDM_SNAPSHOT_BUFFER_SIZE / 4); /* size in dwords */
		ldr		r3, #EUR_CR_VDM_CONTEXT_STORE_SNAPSHOT >> 2, drc0;
		wdf		drc0;
		/* byte-align the address */
		shl		r3, r3, #4;

		SWITCHEDMTOTA();
		CTAL_ContextSnapshotNext:
		{
			MK_MEM_ACCESS_BPCACHE(ldad)	r4, [r3, #1++], drc0;
			wdf		drc0;
			
			MK_TRACE_REG(r4, MKTF_TA | MKTF_TIM | MKTF_HWR | MKTF_CSW);
			isub16.testnz r2, p0, r2, #1;
			p0	br	CTAL_ContextSnapshotNext;
		}
		SWITCHEDMBACK();
#endif
		mov		r0, #0;
		/* r1 is SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid */
		bal				Lockup;
		MK_TRACE(MKTC_TIMER_NOT_TA_LOCKUP, MKTF_TIM | MKTF_HWR | MKTF_SCH);
#if defined(FIX_HW_BRN_31093)
		ba CTAL_Restore_BRN31093;
#endif
	}
	CTAL_Restore:
#if defined(FIX_HW_BRN_31093)
	/* TA made forward progress so clear the attempted unstick bit */
	MK_MEM_ACCESS_BPCACHE(ldad)     r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], drc0;
	wdf		drc0;
	and		r1,	r1,	~#0x4;
	MK_MEM_ACCESS_BPCACHE(stad)     [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], r1;
	CTAL_Restore_BRN31093:
#endif
	/* Restore the return address. */
	mov				pclink, r8;
	idf 			drc1, st;
	wdf 			drc1;

	MK_TRACE(MKTC_TIMER_CTAL_END, MKTF_TIM | MKTF_HWR);
	lapc;
}


/*****************************************************************************
	Check3DLockup - Check whether any 3D hardware has locked up.

	input:
	temps:	p0, r0, r1, r2, r3, r4, r5, r6, r7, r8
*****************************************************************************/
Check3DLockup:
{
	MK_TRACE(MKTC_TIMER_C3DL_START, MKTF_TIM | MKTF_HWR);

	/* Save return address. */
	mov				r8, pclink;

	/* r0 will contain all the 3D signature registers checksummed together. */
	mov				r0, #0;

#if defined(SGX_FEATURE_MP)
	MK_LOAD_STATE_CORE_COUNT_3D(r1, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	iaddu32	r1, r1.low, #(SGX_REG_BANK_BASE_INDEX - 1);
	mov				r4, #EUR_CR_USE0_SERV_PIXEL_COUNT_MASK;
	mov				r5, #3;
	
	C3DL_ReadNextUseCore:
	{
		/* Move reg offset onto the next core */
	#if (SGX_FEATURE_NUM_USE_PIPES <= 2)
		mov		r2, #SGX_FEATURE_NUM_USE_PIPES;
	#else
		mov		r2, #SGX_FEATURE_NUM_USE_PIPES / 2;
	#endif
		mov		r3, #EUR_CR_USE0_SERV_PIXEL >> 2;
		mov		r6, #SGX_REG_BANK_SIZE >> 2;
		imae	r3, r1.low, r6.low, r3, u32;
		mov		r6, #16; // To avoid cancellations generating false positives
		/* Read from the core */
		bal				ReadSignatureRegisters;
		
	#if (SGX_FEATURE_NUM_USE_PIPES > 2)
		mov		r2, #SGX_FEATURE_NUM_USE_PIPES / 2;
		mov		r3, #EUR_CR_USE2_SERV_PIXEL >> 2;
		mov		r6, #SGX_REG_BANK_SIZE >> 2;
		imae	r3, r1.low, r6.low, r3, u32;
		mov		r6, #16; // To avoid cancellations generating false positives
		/* Read from the core */
		bal				ReadSignatureRegisters;
	#endif	
		/* Are there any more cores left to read? */
		isub16	r1, r1, #1;
		isub16.testnz	p0, r1, #(SGX_REG_BANK_BASE_INDEX - 1);
		p0	ba	C3DL_ReadNextUseCore;
	}
#else
	/* Check USSE pixel task counters. */
	#if (SGX_FEATURE_NUM_USE_PIPES <= 2)
	mov				r2, #SGX_FEATURE_NUM_USE_PIPES;
	#else
	mov				r2, #SGX_FEATURE_NUM_USE_PIPES / 2;
	#endif
	#if defined(EUR_CR_USE_SERV_PIXEL)
	mov				r3, #EUR_CR_USE_SERV_PIXEL >> 2;
	mov				r4, #EUR_CR_USE_SERV_PIXEL_COUNT_MASK;
	#else
	mov				r3, #EUR_CR_USE0_SERV_PIXEL >> 2;
	mov				r4, #EUR_CR_USE0_SERV_PIXEL_COUNT_MASK;
	#endif
	mov				r5, #3;
	mov				r6, #16; // To avoid cancellations generating false positives
	bal				ReadSignatureRegisters;
	
	#if (SGX_FEATURE_NUM_USE_PIPES > 2)
	mov				r2, #SGX_FEATURE_NUM_USE_PIPES / 2;
	mov				r3, #EUR_CR_USE2_SERV_PIXEL >> 2;
	bal				ReadSignatureRegisters;
	#endif
#endif

	/* Check ISP Status registers. */
#if defined(EUR_CR_ISP_STATUS1)
	#if defined(SGX_FEATURE_MP)
	MK_LOAD_STATE_CORE_COUNT_3D(r1, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	iaddu32			r1, r1.low, #(SGX_REG_BANK_BASE_INDEX - 1);
	mov				r4, ~#0;
	mov				r5, #1;
	C3DL_ReadNextISPStatus:
	{
		/* Move reg offset onto the next core */
		mov		r2, #2;
		mov		r3, #EUR_CR_ISP_STATUS1 >> 2;
		mov		r6, #SGX_REG_BANK_SIZE >> 2;
		imae	r3, r1.low, r6.low, r3, u32;
		mov		r6, #0;
		/* Read from the core */
		bal				ReadSignatureRegisters;
		
		/* Are there any more cores left to read? */
		isub16	r1, r1, #1;
		isub16.testnz	p0, r1, #(SGX_REG_BANK_BASE_INDEX - 1);
		p0	ba	C3DL_ReadNextISPStatus;
	}
	#else
	mov				r2, #2;
	mov				r3, #EUR_CR_ISP_STATUS1 >> 2;
	mov				r4, ~#0;
	mov				r5, #1;
	mov				r6, #0;
	bal				ReadSignatureRegisters;
	#endif
#else
	#if defined(EUR_CR_ISP_STATUS2)
		#if defined(SGX_FEATURE_MP)
	MK_LOAD_STATE_CORE_COUNT_3D(r2, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	mov				r5, #SGX_REG_BANK_SIZE >> 2;
	mov				r3, #(SGX_MP_CORE_SELECT(EUR_CR_ISP_STATUS2, 0) >> 2);
		#else
	mov				r2, #1;		
	mov				r5, #0;
	mov				r3, #EUR_CR_ISP_STATUS2 >> 2;
		#endif
	mov				r4, ~#0;	
	mov				r6, #0;
	bal				ReadSignatureRegisters;
	#endif
#endif

	/* Check ISP Signature registers. */
#if defined(EUR_CR_ISP_FPU_CHECKSUM)
	mov				r2, #4;
	mov				r3, #EUR_CR_ISP_FPU_CHECKSUM >> 2;
	mov				r4, ~#0;
#else
	mov				r2, #1;
	mov				r3, #EUR_CR_ISP_FPU >> 2;
	mov				r4, #EUR_CR_ISP_FPU_SIGNATURE_MASK;
#endif
	mov				r5, #1;
	mov				r6, #0;
	bal				ReadSignatureRegisters;

	/* Check Pixel Backend Signature registers. */
#if defined(EUR_CR_PBE_PIXEL_CHECKSUM)
	mov				r2, #1;
	mov				r3, #EUR_CR_PBE_PIXEL_CHECKSUM >> 2;
#else
	#if defined(EUR_CR_PIXELBE_SIG12)
	mov				r2, #4;
	#else
	mov				r2, #2;
	#endif
	mov				r3, #EUR_CR_PIXELBE_SIG01 >> 2;
#endif
	mov				r4, ~#0;
	mov				r5, #1;
	mov				r6, #0;
	bal				ReadSignatureRegisters;

	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWR3DSignature)], drc0;
	wdf				drc0;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWR3DSignature)], r0;

	/* Check if we are just invalidating the values */
	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], drc0;
	wdf		drc0;
	and.testz		p0, r1, #0x2;
	p0	ba	C3DL_NotInvalidate;
	{
		/* clear the invalidate bit */
		xor		r1, r1, #0x2;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], r1;
		ba		C3DL_Restore;
	}
	C3DL_NotInvalidate:

	/* Compare old and new values of the ui32HWR3DSignature. */
	xor.testnz		p0, r0, r2;
	p0 ba			C3DL_Restore;
	{
		MK_TRACE(MKTC_TIMER_POTENTIAL_3D_LOCKUP, MKTF_TIM | MKTF_HWR | MKTF_SCH);
#if defined(FIX_HW_BRN_31930) && defined(SGX_FEATURE_ISP_CONTEXT_SWITCH_REV_3)
		/* Check if we are waiting for the EOR loopback.
			If so, keep waiting, don't reset the chip */
		MK_LOAD_STATE(r1, ui32IRenderFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
	
		shl.tests	p0, r1, #(31 - RENDER_IFLAGS_FORCEHALTRENDER_BIT);
		p0		ba	C3DL_End;
	
		MK_TRACE(MKTC_TIMER_ISP_SWITCH_POTENTIAL_LOCKUP, MKTF_TIM | MKTF_CSW | MKTF_SCH | MKTF_3D);
		{
			/* Are we waiting for context-switch EOR? */
			and.testz	p0, r1, #RENDER_IFLAGS_HALTRENDER;
			p0		ba	C3DL_Not31930;
			{
				MK_TRACE(MKTC_TIMER_ISP_SWITCH_FORCE_SWITCH, MKTF_TIM | MKTF_CSW | MKTF_SCH | MKTF_3D);
		
				or		r1, r1, #RENDER_IFLAGS_FORCEHALTRENDER;
				MK_STORE_STATE(ui32IRenderFlags, r1);
				MK_WAIT_FOR_STATE_STORE(drc0);

	#if defined(FIX_HW_BRN_32052)
				/* Find out which cores completed */
				MK_LOAD_STATE_CORE_COUNT_3D(r2, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);
				C3DL_CheckEOR_NextCore:
				{
					isub32		r2, r2, #1;
					READCORECONFIG(r3, r2, #EUR_CR_EVENT_STATUS >> 2, drc0);
					wdf			drc0;
					MK_TRACE_REG(r3, MKTF_TIM | MKTF_CSW | MKTF_SCH | MKTF_3D);
					shl.tests	p0, r3, #(31 - EUR_CR_EVENT_STATUS_PIXELBE_END_RENDER_SHIFT);
					p0			ba	C3DL_EOROccurred;
					{
						READCORECONFIG(r3, r2, #EUR_CR_ISP_CONTEXT_SWITCH2 >> 2, drc0);
						wdf			drc0;
						MK_TRACE_REG(r3, MKTF_TIM | MKTF_CSW | MKTF_SCH | MKTF_3D);
						shl.tests	p0, r3, #(31 - EUR_CR_ISP_CONTEXT_SWITCH2_END_OF_RENDER_SHIFT);
						p0			ba	C3DL_EOROccurred;
						{
							mov			r3, #EUR_CR_EVENT_STATUS_PIXELBE_END_RENDER_MASK;
							WRITECORECONFIG(r2, #EUR_CR_EVENT_STATUS >> 2, r3, r4);
						}
					}
					C3DL_EOROccurred:
					/* Check next core */
					and.testnz	p0, r2, r2;
					p0			ba	C3DL_CheckEOR_NextCore;
				}
				idf		drc0, st;
				wdf		drc0;

				/* Force MCI state clear --> EOR event which will unblock the MK */
				PVRSRV_SGXUTILS_CALL(BRN32052ClearMCIState);
				//MK_ASSERT_FAIL(MKTC_TIMER_ISP_SWITCH_FORCE_SWITCH);
	#else
				/* Emit EOR loopback */
				mov		r16, #USE_LOOPBACK_PIXENDRENDER;
				PVRSRV_SGXUTILS_CALL(EmitLoopBack);
	#endif
				ba C3DL_End;
			}
		}
		C3DL_Not31930:
		MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], drc0;
		wdf		drc0;
#endif
		mov		r0, #1;
		// r1 is SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid
		bal				Lockup;
		MK_TRACE(MKTC_TIMER_NOT_3D_LOCKUP, MKTF_TIM | MKTF_HWR | MKTF_SCH);
#if defined(FIX_HW_BRN_31093)
		ba C3DL_Restore_BRN31093;
#endif
	}
	C3DL_Restore:
#if defined(FIX_HW_BRN_31093)
	/* 3D made forward progress so clear the attempted unstick bit */
	MK_MEM_ACCESS_BPCACHE(ldad)     r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], drc0;
	wdf		drc0;
	and		r1,	r1,	~#0x8;
	MK_MEM_ACCESS_BPCACHE(stad)     [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], r1;
	C3DL_Restore_BRN31093:
#endif
	C3DL_End:

	/* Restore the return address. */
	mov				pclink, r8;
	idf 			drc1, st;
	wdf 			drc1;
	
	MK_TRACE(MKTC_TIMER_C3DL_END, MKTF_TIM | MKTF_HWR);
	lapc;
}


#if defined(SGX_FEATURE_2D_HARDWARE)
/*****************************************************************************
	Check2DLockup - Check whether any 2D hardware has locked up.
*****************************************************************************/
Check2DLockup:
{
	MK_TRACE(MKTC_TIMER_C2DL_START, MKTF_TIM | MKTF_HWR);

	/* Save return address. */
	mov				r8, pclink;


#if defined(SGX_FEATURE_PTLA)
	/* r0 will contain all the 2D "signature" registers checksummed together. */
	mov				r0, #0;

	/* check EUR_CR_MASTER_PTLA_REQUEST */ 
	mov				r2, #1;
	mov				r3, #EUR_CR_MASTER_PTLA_REQUEST >> 2;
	mov				r4, #EUR_CR_MASTER_PTLA_REQUEST_STATUS_MASK;
	mov				r4, #EUR_CR_2D_SIG_RESULT_MASK;

	bal				ReadSignatureRegisters;

	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWR2DSignature)], drc0;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWR2DSignature)], r0;
	wdf				drc0;


	/* Check if we are just invalidating the values */
	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], drc0;
	wdf		drc0;
	and.testz		p0, r1, #0x10;
	p0	ba	C2DL_NotInvalidate;
	{
		/* clear the invalidate bit */
		xor		r1, r1, #0x10;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], r1;
		ba		C2DL_Restore;
	}
	C2DL_NotInvalidate:

	/* Compare old and new values of the ui32HWR2DSignature. */
	xor.testnz		p0, r0, r2;
	p0 ba			C2DL_Restore;
	{
		MK_TRACE(MKTC_TIMER_POTENTIAL_2D_LOCKUP, MKTF_TIM | MKTF_HWR | MKTF_SCH);

		mov		r0, #2;
		bal		Lockup;

		MK_TRACE(MKTC_TIMER_NOT_2D_LOCKUP, MKTF_TIM | MKTF_HWR | MKTF_SCH);
	}
	C2DL_Restore:
#endif

	/* check whether the 2D has page faulted */
	ldr		r3, #EUR_CR_BIF_INT_STAT >> 2, drc0;
	wdf		drc0;
	/* FIXME: add defines to sgxdefs.h, and consider 543/PTLA */
	and.testz	p0, r3, #(1 << 4); //USE_BIF_FAULT_STAT_FAULT_2D_MASK
	p0	ba	C2DL_NoBIFFault;
	{
		MK_TRACE(MKTC_TIMER_2D_LOCKUP, MKTF_TIM | MKTF_HWR | MKTF_SCH | MKTF_HW);
		mov	r0, #2;
		bal	Lockup;
	}
	C2DL_NoBIFFault:

	/* Restore the return address. */
	mov				pclink, r8;
	idf 			drc1, st;
	wdf 			drc1;
	
	MK_TRACE(MKTC_TIMER_C2DL_END, MKTF_TIM | MKTF_HWR);
	lapc;
}
#endif /* SGX_FEATURE_2D_HARDWARE */


/*****************************************************************************
	Lockup - Send an interrupt to the host to reset the chip.

	input:	r0, 0 = TA , 1 = 3D, 2 = 2D Core
	temps:	N/A - this routine will never return
*****************************************************************************/
Lockup:
{
#if defined(FIX_HW_BRN_31093)
	mov	r5,	r1;
#endif

#if defined(SGX_FEATURE_2D_HARDWARE)
	and.testnz	p0, r0, #2;
	p0	ba	L_2DLockup;
#endif
	{
		/* Don't signal that we have detected unless the core is completely idle */
		/* check which core may have locked up */
		and.testnz	p0, r0, #1;
		/* check whether the other half of the core is active */
		!p0 MK_LOAD_STATE(r1, ui32IRenderFlags, drc0);
		p0	MK_LOAD_STATE(r1, ui32ITAFlags, drc0);
		!p0 mov		r4, #RENDER_IFLAGS_RENDERINPROGRESS;
		p0 mov		r4, #TA_IFLAGS_TAINPROGRESS;
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and.testz		p1, r1, r4;
		
#if defined(SGX_FEATURE_2D_HARDWARE)
		/*Check if the 2D core is active and wait for it to complete before issuing hw reset */
		/* Load the 2D flags */
		MK_LOAD_STATE(r1, ui32I2DFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		/* Check if blit in progress, if yes bailout! */
		mov	r4, #TWOD_IFLAGS_2DBLITINPROGRESS;
		and.testnz	p2, r1, r4;
		p2	ba L_Exit;
#endif

		p1	ba	L_LockupDetected;
		{
			/*
				The other half of the core is still active so the event may just be
				stuck in the queue, check whether the core has gone idle or locked up
			*/
			MK_LOAD_STATE(r1, ui32ITAFlags, drc0);
			MK_LOAD_STATE(r2, ui32IRenderFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);

			p0	ba 	L_3DLockup;
			{
				/* If both the LOCKUP and IDLE flags are set we have truely locked up */
				mov		r4, #TA_IFLAGS_HWLOCKUP_MASK;
				and		r3, r1, r4;
				xor.testz	p1, r3, r4;
				p1	ba		L_LockupDetected;
				{
					/* We are still waiting for the other half to go idle */
					/* check whether the 3D has signalled a potential lockup */
					mov		r4, #RENDER_IFLAGS_HWLOCKUP_MASK;
					and.testnz	p1, r2, r4;
					p1	ba	L_LockupDetected;
					{
						/* The 3D has not indicated a potential lockup */
						and.testnz	p0, r3, r3;
						p0	ba	L_Exit;

						/* Set the lockup flag, so we know that we think the TA has locked up */
						mov		r4, #TA_IFLAGS_HWLOCKUP_DETECTED;
						or	r1, r1, r4;
						MK_STORE_STATE(ui32ITAFlags, r1);
						ba	L_IDFExit;
					}
				}
			}
			L_3DLockup:
			{
				/* If both the LOCKUP and IDLE flags are set we have truely locked up */
				mov		r4, #RENDER_IFLAGS_HWLOCKUP_MASK;
				and		r3, r2, #RENDER_IFLAGS_HWLOCKUP_MASK;
				xor.testz	p1, r3, r4;
				p1	ba		L_LockupDetected;
				{
					/* We are still waiting for the other half to go idle */
					/* check whether the TA has signalled a potential lockup */
					mov		r4, #TA_IFLAGS_HWLOCKUP_MASK;
					and.testnz		p1, r1, r4;
					p1	ba		L_LockupDetected;
					{
						/* TA does not think it has locked up, is it in SPM thou? */
						shl.tests	p1, r2, #(31 - RENDER_IFLAGS_TAWAITINGFORMEM_BIT);
						!p1 shl.tests p1, r1, #(31 - TA_IFLAGS_OOM_PR_BLOCKED_BIT);
						!p1 and.testnz p1, r1, #TA_IFLAGS_ABORTINPROGRESS;
						p1 ba L_LockupDetected;
						{
							/* Only write the flag once */
							and.testnz	p1, r3, r3;
							p1	ba	L_Exit;
							{
								mov		r4, #RENDER_IFLAGS_HWLOCKUP_DETECTED;
								or	r2, r2, r4;
								MK_STORE_STATE(ui32IRenderFlags, r2);
								ba	L_IDFExit;
							}
						}
					}
				}
			}
		}
		L_LockupDetected:
		
		#if defined(FIX_HW_BRN_31093)
		xor.testnz	p0,	r0,	#0x0;
		p0	ba L_BRN31093_NotTA;
		{
			and.testz	p0,	r5,	#0x4;
			p0	ba	L_BRN31093_UnstickTA;
			{
				MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], #0x0;
				/* Skip to lockup code */
				ba L_BRN31093_Lockup;
			}
			L_BRN31093_UnstickTA:
			or	r5,	r5,	#0x4;
			MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], r5;
			/* attempt to un-block the BIF */
			mov		r16, #0;
			PVRSRV_SGXUTILS_CALL(InvalidateBIFCache);
			/* return */
			ba	L_IDFExit;
		}
		L_BRN31093_NotTA:
		xor.testnz	p0,	r0,	#0x1;
		p0	ba L_BRN31093_Not3D;
		{
			and.testz	p0,	r5,	#0x8;
			p0	ba	L_BRN31093_Unstick3D;
			{
				MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], #0x0;
				/* Skip to lockup code */
				ba L_BRN31093_Lockup;
			}
			L_BRN31093_Unstick3D:
			or	r5,	r5,	#0x8;
			MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRSignaturesInvalid)], r5;
			/* attempt to un-block the BIF */
			mov		r16, #0;
			PVRSRV_SGXUTILS_CALL(InvalidateBIFCache);
			/* return */
			ba	L_IDFExit;
		}
		L_BRN31093_Not3D:
		L_BRN31093_Lockup:
		#endif
		
		/* indicate that the scene caused a lockup in the HWRTData */
		and.testnz	p0, r0, #1;
		/* if it was a fast render do not set any flags */
		p0 MK_LOAD_STATE(r1, ui32IRenderFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and.testnz	p1, #0, #0;
		p0 and.testnz	p1, r1, #RENDER_IFLAGS_TRANSFERRENDER;
		p1	ba	L_FastRenderLockup;
		{
			!p0	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], drc0;
			p0	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRTData)], drc0;
			wdf	drc0;
			MK_MEM_ACCESS_BPCACHE(ldad)	r2, [r1, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc0;
			!p0	mov	r3, #(SGXMKIF_HWRTDATA_CMN_STATUS_TATIMEOUT | SGXMKIF_HWRTDATA_CMN_STATUS_ABORT);
			p0	mov	r3, #SGXMKIF_HWRTDATA_CMN_STATUS_RENDERTIMEOUT;
			wdf	drc0;
			or	r2, r2, r3;
			MK_MEM_ACCESS_BPCACHE(stad)	[r1, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], r2;
		}
		L_FastRenderLockup:
	}
#if defined(SGX_FEATURE_2D_HARDWARE)
	L_2DLockup:
	xor.testnz			p0, r0, #2;
	p0 ba				L_Not2DLockup;
	{
		WRITEHWPERFCB(2D, MK_2D_LOCKUP, GRAPHICS, #0, p0, ktimer_lockup_1, r10, r3, r4, r5, r6, r7, r8, r9);
	}
	L_Not2DLockup:
#endif

	MK_TRACE(MKTC_TIMER_LOCKUP, MKTF_TIM | MKTF_HWR | MKTF_SCH | MKTF_HW);

	xor.testnz			p0, r0, #0;
	p0 ba 				L_NotTALockup;
	{
		WRITEHWPERFCB(TA, MK_TA_LOCKUP, GRAPHICS, #0, p0, ktimer_lockup_2, r10, r3, r4, r5, r6, r7, r8, r9);
	}
	L_NotTALockup:
	xor.testnz			p0, r0, #1;
	p0 ba 				L_Not3DLockup;
	{
		WRITEHWPERFCB(3D, MK_3D_LOCKUP, GRAPHICS, #0, p0, ktimer_lockupt_3, r10, r3, r4, r5, r6, r7, r8, r9);
	}
	L_Not3DLockup:

	/* before setting the flag and increment the counter */
	MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32uKernelDetectedLockups)], drc0;
	/* request the interruptFlags now */
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32InterruptFlags)], drc1;
	wdf				drc0;
	mov				r1, #1;
	iaddu32			r0, r1.low, r0;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32uKernelDetectedLockups)], r0;

	/* Set the Hardware Recovery interrupt flag. */
	wdf				drc1;
	or				r2, r2, #PVRSRV_USSE_EDM_INTERRUPT_HWR;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32InterruptFlags)], r2;
	/* ensure the flags are in memory before generating the interrupt */
	idf				drc0, st;
	wdf				drc0;

	/* Send iterrupt to host. */
	mov	 			r0, #EUR_CR_EVENT_STATUS_SW_EVENT_MASK;
	str	 			#EUR_CR_EVENT_STATUS >> 2, r0;

	ba				WaitForReset;

L_IDFExit:
	/* This is stage 1, at this point register the potential lockup and exit */
	idf		drc0, st;
	wdf		drc0;
L_Exit:
	/* Exit so we can continue */
	lapc;
}
#endif /* defined(SUPPORT_HW_RECOVERY) */


/*****************************************************************************
 RenderContextCleanup routine - check for timeouts and perform context switching
#if defined(SUPPORT_SHARED_PB)
								In the case of a shared PB the host will send 
								PB cleanup request when the last reference is removed.
#endif

 inputs:	r0 - sResManCleanupData
 temps:		r0,	r1, r2, r3, r4, r5
*****************************************************************************/
RenderContextCleanup:
{
	MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPartialRendersHead)], drc0;
	MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWCompleteRendersHead)], drc0;
	MK_MEM_ACCESS(ldad)	r3, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32TACount)], drc0;
	wdf		drc0;
	or			r2, r1, r2;
	or.testz		p0, r2, r3;
	!p0 ba			RCC_WorkPending;
	{
		/* The render context is finished with. */
		MK_TRACE(MKTC_TIMER_RC_CLEANUP_COMPLETE, MKTF_SCH | MKTF_CU);
		/*
			If the EDM memory context is still set to the old render context,
			reset it back to the kernel memory context.
		*/
		/* FIXME - We need a utility 'function' for management of the EDM memory context. */
		/* FIXME - Do any other caches need to be cleared? */
	#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
		ldr				r1, #EUR_CR_BIF_DIR_LIST_BASE0 >> 2, drc0;
		/* note: the wdf follows in SGXMK_HWCONTEXT_GETDEVPADDR */
		SGXMK_HWCONTEXT_GETDEVPADDR(r2, r0, r5, drc0);

		xor.testz		p0, r1, r2;
		!p0 ba			T_RCCleanup_FinishedContext;
		{
			/* check that the core is not actively using the memory context */
			MK_LOAD_STATE(r1, ui32IRenderFlags, drc0);
			MK_LOAD_STATE(r2, ui32ITAFlags, drc0);
		#if defined(SGX_FEATURE_2D_HARDWARE)
			MK_LOAD_STATE(r3, ui32I2DFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			or		r2, r2, r3;
		#else
			MK_WAIT_FOR_STATE_LOAD(drc0);
		#endif
			or		r1, r1, r2;
			/* All the modules use the same bit to indicate 'busy' */
			and.testnz	p0, r1, #RENDER_IFLAGS_RENDERINPROGRESS;
			p0	ba T_RCCleanup_FinishedContext;
			{
				SGXMK_TA3DCTRL_GETDEVPADDR(r2, r3, drc0);
	
				SGXMK_SPRV_WRITE_REG(#EUR_CR_BIF_DIR_LIST_BASE0 >> 2, r2);
	
		#if defined(FIX_HW_BRN_31620)
				/* refill all the DC cache with the new PD */
				REQUEST_DC_REFRESH(#0xFFFFFFFF, #0xFFFFFFFF);
			
				/* The call to InvalidateBIFCache will do a PT invalidate */
		#endif
				/* Invalidate the BIF caches */
				mov		r16, #1;
				PVRSRV_SGXUTILS_CALL(InvalidateBIFCache);
			}
		}
	#endif /* defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS) */

		T_RCCleanup_FinishedContext:

		/* check TA3DCtrl->TA/3D HWPBDesc against HWRC->HWPBDesc and zero TA/3D ctrl if matching */
		MK_MEM_ACCESS(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPBDescDevVAddr.uiAddr)], drc0;
		MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc.uiAddr)], drc0;
		MK_MEM_ACCESS_BPCACHE(ldad)	r4, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DHWPBDesc.uiAddr)], drc1;

		/* check TA */
		wdf				drc0;
#if defined(SUPPORT_PERCONTEXT_PB)
		MK_MEM_ACCESS(ldad)	r5, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sCommon.ui32Flags)], drc0;
		wdf				drc0;
		and.testz		p0, r5, #SGXMKIF_HWCFLAGS_PERCONTEXTPB;
		p0 ba SharedContextTA;
		{
			/* 
				if (RC HWPBDesc == sTAHWPBDesc)
					clear sTAHWPBDesc
			*/
			xor.testz		p0, r2, r3;
			/* clear the ta/3d ctrl (no need for idf,wdf - done later) */
			p0 MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc.uiAddr)], #0;
		}
		SharedContextTA:
#endif /* SUPPORT_PERCONTEXT_PB */

		/* check 3D */
		wdf				drc1;
#if defined(SUPPORT_PERCONTEXT_PB)
		and.testz		p0, r5, #SGXMKIF_HWCFLAGS_PERCONTEXTPB;
		p0 ba SharedContext3D;
		{
			/* 
				if (RC HWPBDesc == s3DHWPBDesc)
					clear s3DHWPBDesc
			*/
			xor.testz		p0, r2, r4;
			/* clear the ta/3d ctrl (no need for idf,wdf - done later) */
			p0 MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DHWPBDesc.uiAddr)], #0;
		}
		SharedContext3D:
#endif /* SUPPORT_PERCONTEXT_PB */

		MK_LOAD_STATE(r3, ui32ITAFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and		r3, r3, ~#TA_IFLAGS_DEFERREDCONTEXTFREE;
		MK_STORE_STATE(ui32ITAFlags, r3);

		#if defined(SGX_FEATURE_SYSTEM_CACHE)
			#if defined(SGX_FEATURE_MP)
				INVALIDATE_SYSTEM_CACHE(#EUR_CR_MASTER_SLC_CTRL_INVAL_ALL_MASK, #EUR_CR_MASTER_SLC_CTRL_FLUSH_ALL_MASK);
			#else
				INVALIDATE_SYSTEM_CACHE();
			#endif /* SGX_FEATURE_MP */
		#endif /* SGX_FEATURE_SYSTEM_CACHE */

		MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32CleanupStatus)], drc0;
		wdf				drc0;
		or				r3, r3, #PVRSRV_USSE_EDM_CLEANUPCMD_COMPLETE;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32CleanupStatus)], r3;

		MK_LOAD_STATE(r3, ui32HostRequest, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and	r3, r3, #~(SGX_UKERNEL_HOSTREQUEST_CLEANUP_RC);
		MK_STORE_STATE(ui32HostRequest, r3);

		idf				drc0, st;
		wdf				drc0;

		/* return */
		lapc;
	}
	RCC_WorkPending:

	/*
		There is still some work to do on this context, check whether all the
		work can be completed without intervention
	*/
	and.testnz	p0, r1, r1;
	p0	and.testz	p0, r3, r3;
	!p0	ba	RCC_Exit;
	{
		/* There are partially completed scenes which will not complete */
		MK_LOAD_STATE(r2, ui32ITAFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and.testnz		p0, r2, #TA_IFLAGS_TAINPROGRESS;
		/* modify the bits accordingly */
		p0 or	r2, r2, #TA_IFLAGS_DEFERREDCONTEXTFREE;
		!p0	and		r2, r2, ~#TA_IFLAGS_DEFERREDCONTEXTFREE;
		MK_STORE_STATE(ui32ITAFlags, r2);
		MK_WAIT_FOR_STATE_STORE(drc0);

		/* If the TA is blocking free, exit and wait for it to finish */
		p0 ba	RCC_Exit;

#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
		{
			/* 
				On single memory context HW, ensure 3D is idle before freeing context memory
			*/
			MK_LOAD_STATE(r2, ui32IRenderFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);		
			and.testnz	p0, r2, #RENDER_IFLAGS_RENDERINPROGRESS;
			/* Assumption: no 2D HW with single mem context core */
			p0 or	r2, r2, #RENDER_IFLAGS_DEFERREDCONTEXTFREE;
			!p0	and		r2, r2, ~#RENDER_IFLAGS_DEFERREDCONTEXTFREE;
			MK_STORE_STATE(ui32IRenderFlags, r2);
			MK_WAIT_FOR_STATE_STORE(drc0);
		}
		/* Wait for 3D */
		p0 ba	RCC_Exit;
		
#endif /* SGX_FEATURE_MULTIPLE_MEM_CONTEXTS */
		{
			MK_TRACE(MKTC_TIMER_RC_CLEANUP, MKTF_SCH | MKTF_CU);

			/* Move them onto the complete list and mark them for dummy processing */
			/* Move sHWPartialRendersHead into r6 */
			mov		r6, r1;

			T_RCCleanup_NextScene:
			{
				MK_MEM_ACCESS(ldad)	R_RTData, [r6, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWRTDataDevAddr)], drc0;
				wdf		drc0;
				MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc0;
				wdf		drc0;
				or		r2, r2, #(SGXMKIF_HWRTDATA_CMN_STATUS_TACOMPLETE | SGXMKIF_HWRTDATA_CMN_STATUS_DUMMYPROC);
				MK_MEM_ACCESS(stad)	[R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], r2;
				idf		drc0, st;

				/* Free the memory associated with the scene */
				/* r0 = HWRenderContext */
				mov		r1, pclink;
				bal		FreeContextMemory;
				mov		pclink, r1;

				/* r0 = HWRenderContext, r6 = HWRenderDetails */
				MOVESCENETOCOMPLETELIST(r0, r6);

				MK_MEM_ACCESS(ldad)	r6, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPartialRendersHead)], drc0;
				wdf		drc0;
				and.testnz	p0, r6, r6;
				p0	ba	T_RCCleanup_NextScene;
			}

			/* Now we have moved all the scenes onto the complete list, remove it from PartialRenderContext list */
			MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sCommon.ui32Priority)], drc0;
			wdf		drc0;

			/* Remove the current render context from the list of the partial render contexts list */
			MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], drc0;
			MK_MEM_ACCESS(ldad)	r5, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], drc1;
			wdf		drc0;
			and.testz	p0, r4, r4;	// is prev null?
			wdf		drc1;
			/* (prev == NULL) ? head = current->next : prev->nextptr = current->next */
			!p0	ba	T_RCC_NoHeadUpdate;
			{
				MK_STORE_STATE_INDEXED(sPartialRenderContextHead, r1, r5, r2);
			}
			T_RCC_NoHeadUpdate:
			!p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], r5;

			and.testz	p1, r5, r5;	// is next null?
			/* (next == NULL) ? tail == current->prev : next->prevptr = current->prev */
			!p1	ba	T_RCC_NoTailUpdate;
			{
				MK_STORE_STATE_INDEXED(sPartialRenderContextTail, r1, r4, r3);
			}
			T_RCC_NoTailUpdate:
			!p1	MK_MEM_ACCESS(stad)	[r5, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], r4;
			MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], #0;
			MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], #0;
			idf		drc1, st;
			wdf		drc1;

			/* Emit loopback to kick 3Ds */
			FIND3D();
		}
	}
	RCC_Exit:

	/* return */
	lapc;
}


/*****************************************************************************
 TransferContextCleanup routine -

 inputs:	r0 - sResManCleanupData
 temps:		r0,	r1, r2, r3, r4
*****************************************************************************/
TransferContextCleanup:
{
	MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.ui32Count)], drc0;
	wdf		drc0;
	and.testz		p0, r1, r1;
	!p0 ba			TCC_WorkPending;
	{
		/* The transfer context is finished with. */
		MK_TRACE(MKTC_TIMER_TC_CLEANUP_COMPLETE, MKTF_SCH | MKTF_CU);
		/*
			If the EDM memory context is still set to the old transfer context,
			reset it back to the kernel memory context.
		*/
		/* FIXME - We need a utility 'function' for management of the EDM memory context. */
		/* FIXME - Do any other caches need to be cleared? */
	#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
		ldr				r1, #EUR_CR_BIF_DIR_LIST_BASE0 >> 2, drc0;
		/* note: the wdf follows in SGXMK_HWCONTEXT_GETDEVPADDR */
		SGXMK_HWCONTEXT_GETDEVPADDR(r2, r0, r3, drc0);

		xor.testz		p0, r1, r2;
		!p0 ba			TCC_FinishedContext;
		{
			/* check that the core is not actively using the memory context */
			MK_LOAD_STATE(r1, ui32IRenderFlags, drc0);
			MK_LOAD_STATE(r2, ui32ITAFlags, drc0);
		#if defined(SGX_FEATURE_2D_HARDWARE)
			MK_LOAD_STATE(r3, ui32I2DFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			or		r2, r2, r3;
		#else
			MK_WAIT_FOR_STATE_LOAD(drc0);
		#endif
			or		r1, r1, r2;
			/* All modules use the same bit to indicate 'busy' */
			and.testnz	p0, r1, #RENDER_IFLAGS_RENDERINPROGRESS;
			p0	ba TCC_FinishedContext;
			{
				/*
					Flush the TPC before switching to the kernel context to avoid
					SetupDirListBase flushing any TPC data when switching to the 
					next application context.
				*/
				mov		r2, #EUR_CR_TE_TPCCONTROL_FLUSH_MASK;
				str		#EUR_CR_TE_TPCCONTROL >> 2, r2;

				ENTER_UNMATCHABLE_BLOCK;
				mov		r1, #EUR_CR_EVENT_STATUS_TPC_FLUSH_MASK;
				TCC_WaitForTPCFlush:
				{
					ldr		r2, #EUR_CR_EVENT_STATUS >> 2, drc0;
					wdf		drc0;
					and		r2, r2, r1;
					xor.testnz	p0, r2, r1;
					p0	ba		TCC_WaitForTPCFlush;
				}
				LEAVE_UNMATCHABLE_BLOCK;
				
				SGXMK_TA3DCTRL_GETDEVPADDR(r2, r3, drc0);
		
				SGXMK_SPRV_WRITE_REG(#EUR_CR_BIF_DIR_LIST_BASE0 >> 2, r2);
		
		#if defined(FIX_HW_BRN_31620)
				/* refill all the DC cache with the new PD */
				REQUEST_DC_REFRESH(#0xFFFFFFFF, #0xFFFFFFFF);
			
				/* The call to InvalidateBIFCache will do a PT invalidate */
		#endif
				/* Invalidate the BIF caches */
				mov		r16, #1;
				PVRSRV_SGXUTILS_CALL(InvalidateBIFCache);
			}
		}
		TCC_FinishedContext:
	#endif /* defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS) */

		#if defined(SGX_FEATURE_SYSTEM_CACHE)
			#if defined(SGX_FEATURE_MP)
				INVALIDATE_SYSTEM_CACHE(#EUR_CR_MASTER_SLC_CTRL_INVAL_ALL_MASK, #EUR_CR_MASTER_SLC_CTRL_FLUSH_ALL_MASK);
			#else
				INVALIDATE_SYSTEM_CACHE();
			#endif /* SGX_FEATURE_MP */
		#endif /* SGX_FEATURE_SYSTEM_CACHE */
		
		MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32CleanupStatus)], drc0;
		wdf				drc0;
		or				r3, r3, #PVRSRV_USSE_EDM_CLEANUPCMD_COMPLETE;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32CleanupStatus)], r3;

		MK_LOAD_STATE(r3, ui32HostRequest, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and	r3, r3, #~(SGX_UKERNEL_HOSTREQUEST_CLEANUP_TC);
		MK_STORE_STATE(ui32HostRequest, r3);

		idf				drc0, st;
		wdf				drc0;

		/* return */
		lapc;
	}
	TCC_WorkPending:

	/* return */
	lapc;
}


#if defined(SGX_FEATURE_2D_HARDWARE)
/*****************************************************************************
 TwoDContextCleanup routine -

 inputs:	r0 - sResManCleanupData
 temps:		r0,	r1, r2, r3, r4
*****************************************************************************/
TwoDContextCleanup:
{
	MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HW2DCONTEXT.ui32Count)], drc0;
	wdf		drc0;
	and.testz		p0, r1, r1;
	!p0 ba			TwoDCC_WorkPending;
	{
		/* The 2D context is finished with. */
		MK_TRACE(MKTC_TIMER_2DC_CLEANUP_COMPLETE, MKTF_2D | MKTF_SCH | MKTF_CU);
		MK_TRACE_REG(r0, MKTF_2D | MKTF_CU);

	#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
		ldr				r1, #EUR_CR_BIF_DIR_LIST_BASE0 >> 2, drc0;
		/* note: the wdf follows in SGXMK_HWCONTEXT_GETDEVPADDR */
		SGXMK_HWCONTEXT_GETDEVPADDR(r2, r0, r3, drc0);

		xor.testz		p0, r1, r2;
		!p0 ba			TwoDCC_FinishedContext;
		{
			/* check that the core is not actively using the memory context */
			MK_LOAD_STATE(r1, ui32IRenderFlags, drc0);
			MK_LOAD_STATE(r2, ui32ITAFlags, drc0);
			MK_LOAD_STATE(r3, ui32I2DFlags, drc0);
			MK_WAIT_FOR_STATE_LOAD(drc0);
			or		r2, r2, r3;
			or		r1, r1, r2;
			/* All modules use the same bit to indicate 'busy' */
			and.testnz	p0, r1, #RENDER_IFLAGS_RENDERINPROGRESS;
			p0	ba TwoDCC_FinishedContext;
			{
				SGXMK_TA3DCTRL_GETDEVPADDR(r2, r3, drc0);
		
				SGXMK_SPRV_WRITE_REG(#EUR_CR_BIF_DIR_LIST_BASE0 >> 2, r2);
		
		#if defined(FIX_HW_BRN_31620)
				/* refill all the DC cache with the new PD */
				REQUEST_DC_REFRESH(#0xFFFFFFFF, #0xFFFFFFFF);
			
				/* The call to InvalidateBIFCache will do a PT invalidate */
		#endif
				/* Invalidate the BIF caches */
				mov		r16, #1;
				PVRSRV_SGXUTILS_CALL(InvalidateBIFCache);
			}
		}
		TwoDCC_FinishedContext:
	#endif /* defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS) */

		#if defined(SGX_FEATURE_SYSTEM_CACHE)
			#if defined(SGX_FEATURE_MP)
				INVALIDATE_SYSTEM_CACHE(#EUR_CR_MASTER_SLC_CTRL_INVAL_ALL_MASK, #EUR_CR_MASTER_SLC_CTRL_FLUSH_ALL_MASK);
			#else
				INVALIDATE_SYSTEM_CACHE();
			#endif /* SGX_FEATURE_MP */
		#endif /* SGX_FEATURE_SYSTEM_CACHE */
	
		MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32CleanupStatus)], drc0;
		wdf				drc0;
		or				r3, r3, #PVRSRV_USSE_EDM_CLEANUPCMD_COMPLETE;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32CleanupStatus)], r3;

		MK_LOAD_STATE(r3, ui32HostRequest, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and	r3, r3, #~(SGX_UKERNEL_HOSTREQUEST_CLEANUP_2DC);
		MK_STORE_STATE(ui32HostRequest, r3);

		idf				drc0, st;
		wdf				drc0;

		/* return */
		lapc;
	}
	TwoDCC_WorkPending:

	/* return */
	lapc;
}
#endif


/*****************************************************************************
 RenderTargetCleanup routine - check for timeouts and perform context switching

 inputs:	r0 - sResManCleanupData (SGXMKIF_HWRTDATASET)
 temps:		r0,	r1, r2, r3, r4, r5, r6, r7, r8, r9

 Loop over RTDatas (ui32NumRTData, usually 2)
 	If SGXMKIF_HWRTDATA_CMN_STATUS_READY is set then check next.
 	Else
 		Unmark RTDatas for cleanup
 		If SGXMKIF_HWRTDATA_CMN_STATUS_TACOMPLETE is set then check next.
 		Else
 			If TA_IFLAGS_TAINPROGRESS is set on current render context
 				If this RTData is in use on the TA
 					Clear TA_IFLAGS_DEFERREDCONTEXTFREE if set
	 			Else
	 				Set TA_IFLAGS_DEFERREDCONTEXTFREE
	 			End
 				Exit
 			Else
 				Clear TA_IFLAGS_DEFERREDCONTEXTFREE if set
  				Set bits for Dummy3D processing
  				Free context memory for associated HW Render Context (see FCM routine)
  				Move scene to complete list
  			End
 		End
 	End
 Loop

 If RTDatas ready
 	If queued ops on RTDataset have all completed
 		Clear TA_IFLAGS_DEFERREDCONTEXTFREE if set
 		Mark RTDatas cleaned so host can free any associated resource (see SGXCleanupRequest)
  	Endif
	Exit
 Else
 	Tidy partial renders list
 Endif

*****************************************************************************/
RenderTargetCleanup:
{
	/* Start Addr */
	mov				r1, #OFFSET(SGXMKIF_HWRTDATASET.asHWRTData[0]);
	iaddu32			R_RTData, r1.low, r0;
	MK_MEM_ACCESS(ldad)	r8, [r0, #DOFFSET(SGXMKIF_HWRTDATASET.ui32NumRTData)], drc0;
	mov				r7, #USE_TRUE;

	/* Load the HWRenderContext just the once */
	MK_MEM_ACCESS(ldad)	r9, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderContextDevAddr)], drc0;

	/* Wait for outstanding TAs to complete */
	MK_MEM_ACCESS(ldad)	r3, [r0, #DOFFSET(SGXMKIF_HWRTDATASET.sPendingCountDevAddr)], drc1;
	MK_MEM_ACCESS(ldad)	r4, [r0, #DOFFSET(SGXMKIF_HWRTDATASET.ui32CompleteCount)], drc0;
	wdf	drc1;
	MK_MEM_ACCESS_BPCACHE(ldad)	r3, [r3], drc0;
	wdf	drc0;
	xor.testz	p0, r3, r4;
	p0 ba	RTC_NextRTData;
	{
		MK_TRACE(MKTC_TIMER_RT_CLEANUP_PENDING, MKTF_CU | MKTF_RT);
		MK_TRACE_REG(r0, MKTF_CU | MKTF_RT);
		MK_TRACE_REG(r3, MKTF_CU);
		MK_TRACE_REG(r4, MKTF_CU);
		ba	RTC_Exit;
	}

	RTC_NextRTData:
	{
		MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc0;
		wdf		drc0;
		and.testnz	p0, r2, #SGXMKIF_HWRTDATA_CMN_STATUS_READY;
		p0	ba		RTC_CheckNextRTData;
		{
			/* The RT is in use, therefore mark r7 as FALSE */
			mov		r7, #USE_FALSE;

			/* If the TA is complete, wait for render to finish normally */
			and.testnz	p0, r2, #SGXMKIF_HWRTDATA_CMN_STATUS_TACOMPLETE;
			p0	ba		RTC_CheckNextRTData;
			{
				MK_TRACE(MKTC_TIMER_RT_CLEANUP, MKTF_SCH | MKTF_CU);

				MK_LOAD_STATE(r3, ui32ITAFlags, drc0);
				MK_MEM_ACCESS_BPCACHE(ldad)	r6, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], drc0;
				wdf		drc0;

				/* TODO: Enable once sTARTData is stored in a SA
				MK_LOAD_STATE(r6, sTARTData, drc0)
				MK_WAIT_FOR_STATE_LOAD(drc0) */
				and.testnz		p0, r3, #TA_IFLAGS_TAINPROGRESS;

				/* modify the bits accordingly */
				!p0	and		r3, r3, ~#TA_IFLAGS_DEFERREDCONTEXTFREE;
				!p0 ba	RTC_TANotInProgress;
				{
					/* if this RTData is currently in use by the TA don't set the deferred context free */
					xor.testnz	p1, R_RTData, r6;
					p1 or	r3, r3, #TA_IFLAGS_DEFERREDCONTEXTFREE;
					!p1 and		r3, r3, ~#TA_IFLAGS_DEFERREDCONTEXTFREE;
				}
				RTC_TANotInProgress:
				MK_STORE_STATE(ui32ITAFlags, r3);
				MK_WAIT_FOR_STATE_STORE(drc0);

				/* The TA is in use so we can't free the scene so exit */
				p0 ba	RTC_Exit;
				
#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
				{
					/* 
						On single memory context HW, ensure 3D is idle before calling FCM
					*/
					MK_LOAD_STATE(r3, ui32IRenderFlags, drc0);
					MK_WAIT_FOR_STATE_LOAD(drc0);
					
					and.testnz	p0, r3, #RENDER_IFLAGS_RENDERINPROGRESS;
					/* Assumption: no 2D HW with single mem context core */
					p0 or	r3, r3, #RENDER_IFLAGS_DEFERREDCONTEXTFREE;
					!p0	and		r3, r3, ~#RENDER_IFLAGS_DEFERREDCONTEXTFREE;
					MK_STORE_STATE(ui32IRenderFlags, r3);
					MK_WAIT_FOR_STATE_STORE(drc0);
					/* 3D is busy so exit */
					p0 ba	RTC_Exit;
				}
#endif

				/* Set the bits so it will be dummy processed by 3D */
				or		r2, r2, #(SGXMKIF_HWRTDATA_CMN_STATUS_TACOMPLETE | \
									SGXMKIF_HWRTDATA_CMN_STATUS_DUMMYPROC);
				MK_MEM_ACCESS(stad)	[R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], r2;
				idf		drc0, st;

				/* Get the HWRenderContext associated with the RTDATA.
				 * NOTE: This overwrites the value of r0 which previously stored
				 * ResManCleanupData. We have set r7 = #USE_FALSE so it's safe
				 * for the Pending/CompleteCount below which uses the old value of r0.
				 */
				MK_MEM_ACCESS(ldad)	r0, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderContextDevAddr)], drc0;
				wdf		drc0;

				/* Free the memory associated with the scene */
				mov		r1, pclink;
				bal		FreeContextMemory;
				mov		pclink, r1;

				/* Get the HWRenderDetails associated with the RTDATA */
				MK_MEM_ACCESS(ldad)	r6, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderDetailsDevAddr)], drc0;
				wdf		drc0;

				/* r9 = HWRenderContext, r6 = HWRenderDetails */
				MOVESCENETOCOMPLETELIST(r9, r6);
			}
		}
		RTC_CheckNextRTData:

		mov			r1, #SIZEOF(SGXMKIF_HWRTDATA);
		iaddu32 	R_RTData, r1.low, R_RTData;

		/* Are there any more to check */
		isub16.testnz	r8, p0, r8, #1;
		p0	ba	RTC_NextRTData;
	}

	/* Are they all ready */
	xor.testz	p0, r7, #USE_TRUE;
	!p0		ba	RTC_RTInUse;
	{
		/* The RT is finished with. */
		MK_TRACE(MKTC_TIMER_RT_CLEANUP_COMPLETE, MKTF_SCH | MKTF_CU);

		#if defined(SGX_FEATURE_SYSTEM_CACHE)
			#if defined(SGX_FEATURE_MP)
				INVALIDATE_SYSTEM_CACHE(#EUR_CR_MASTER_SLC_CTRL_INVAL_ALL_MASK, #EUR_CR_MASTER_SLC_CTRL_FLUSH_ALL_MASK);
			#else
				INVALIDATE_SYSTEM_CACHE();
			#endif /* SGX_FEATURE_MP */
		#endif /* SGX_FEATURE_SYSTEM_CACHE */

		/* Ensure the flag is cleared */
		MK_LOAD_STATE(r3, ui32ITAFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and		r3, r3, ~#TA_IFLAGS_DEFERREDCONTEXTFREE;
		MK_STORE_STATE(ui32ITAFlags, r3);

		MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32CleanupStatus)], drc0;
		wdf				drc0;
		or				r3, r3, #PVRSRV_USSE_EDM_CLEANUPCMD_COMPLETE;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32CleanupStatus)], r3;

		MK_LOAD_STATE(r3, ui32HostRequest, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and	r3, r3, #~(SGX_UKERNEL_HOSTREQUEST_CLEANUP_RT);
		MK_STORE_STATE(ui32HostRequest, r3);

		idf				drc0, st;
		wdf				drc0;
		
		ba	RTC_Exit;
	}
	RTC_RTInUse:
	
	/* Now we have moved all the scenes for the RT onto the complete list, remove it from PartialRenderContext list.
		if needed */
	MK_TRACE(MKTC_TIMER_RT_CLEANUP_TIDYPARTIALLIST, MKTF_SCH | MKTF_CU);

	/* Is the context on the sPartialRenderContext list  */
	MK_MEM_ACCESS(ldad)	r1, [r9, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], drc0;
	MK_MEM_ACCESS(ldad)	r2, [r9, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], drc0;
	wdf		drc0;
	or.testnz	p0, r1, r2;
	p0	ba	RTC_ContextInList;
	{
		/* Check if the context is the head of it priority runlist */
		MK_MEM_ACCESS(ldad)	r1, [r9, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sCommon.ui32Priority)], drc0;
		wdf		drc0;
		MK_LOAD_STATE_INDEXED(r3, sPartialRenderContextHead, r1, drc0, r2);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		xor.testnz	p0, r9, r3;
		p0	ba	RTC_PartialContextListTidyComplete;
	}
	RTC_ContextInList:
	{
		/* it is in the list somewhere, should it be? */
		MK_MEM_ACCESS(ldad)	r1, [r9, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sHWPartialRendersHead)], drc0;
		MK_MEM_ACCESS(ldad)	r2, [r9, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32TACount)], drc0;
		wdf		drc0;
		or.testnz	p0, r1, r2;
		p0	ba	RTC_PartialContextListTidyComplete;
		{
			/* The PartialRenders list and Count are 0 so therefore remove it from the list */
			MK_MEM_ACCESS(ldad)	r1, [r9, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sCommon.ui32Priority)], drc0;
			wdf		drc0;
			/* Remove the current render context from the list of the partial render contexts list */
			MK_MEM_ACCESS(ldad)	r4, [r9, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], drc0;
			MK_MEM_ACCESS(ldad)	r5, [r9, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], drc1;
			wdf		drc0;
			and.testz	p0, r4, r4;	// is prev null?
			wdf		drc1;
			/* (prev == NULL) ? head = current->next : prev->nextptr = current->next */
			!p0	ba	RTC_NoHeadUpdate;
			{
				MK_STORE_STATE_INDEXED(sPartialRenderContextHead, r1, r5, r2);
			}
			RTC_NoHeadUpdate:
			!p0	MK_MEM_ACCESS(stad)	[r4, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], r5;

			and.testz	p1, r5, r5;	// is next null?
			/* (next == NULL) ? tail == current->prev : next->prevptr = current->prev */
			!p1	ba	RTC_NoTailUpdate;
			{
				MK_STORE_STATE_INDEXED(sPartialRenderContextTail, r1, r4, r2);
			}
			RTC_NoTailUpdate:
			!p1	MK_MEM_ACCESS(stad)	[r5, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], r4;
			MK_MEM_ACCESS(stad)	[r9, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sPrevPartialDevAddr)], #0;
			MK_MEM_ACCESS(stad)	[r9, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.sNextPartialDevAddr)], #0;
			idf		drc1, st;
			wdf		drc1;
		}
	}
	RTC_PartialContextListTidyComplete:

	/* Emit loopback to kick 3Ds */
	FIND3D();
	
	RTC_Exit:

	/* return */
	lapc;
}


/*****************************************************************************
 SharedPBDescCleanup routine - clean-up HW PB Desc

 inputs:	none
 temps:		r0,	r1, r2, r3, r4
*****************************************************************************/
SharedPBDescCleanup:
{
	/* clear s3DHWPBDesc and sTAHWPBDesc to 0 */
	MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc)], drc0;
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DHWPBDesc)], drc0;
	wdf				drc0;

	/* Only clear if current PB = requested PB */
	xor.testz		p0, r0, r1;
	p0 MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc)], #0;
	xor.testz		p0, r0, r2;
	p0 MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DHWPBDesc)], #0;

	/* clear the request flag and signal complete */
	MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32CleanupStatus)], drc0;
	wdf				drc0;
	or				r3, r3, #PVRSRV_USSE_EDM_CLEANUPCMD_COMPLETE;
	MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32CleanupStatus)], r3;

	MK_LOAD_STATE(r3, ui32HostRequest, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and	r3, r3, #~(SGX_UKERNEL_HOSTREQUEST_CLEANUP_PB);
	MK_STORE_STATE(ui32HostRequest, r3);

	idf				drc0, st;
	wdf				drc0;

	MK_TRACE(MKTC_TIMER_SHAREDPBDESC_CLEANUP, MKTF_SCH | MKTF_CU);

	/* return */
	lapc;
}


/*****************************************************************************
 Timer routine	- check for timeouts and perform context switching

 inputs:
 temps:		r0,	r1, r2, r3, r4, r5, r6, r7, r8
*****************************************************************************/
Timer:
{
	WRITEHWPERFCB(TA, PERIODIC, PERIODIC, #0, p0, T, r1, r2, r3, r4, r5, r6, r7, r8);
	
#if defined(FIX_HW_BRN_23055)
	/* detect deadlock situation and do abortall */
	MK_LOAD_STATE(r0, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	shl.testns		p0, r0, #(31 - TA_IFLAGS_INCREASEDTHRESHOLDS_BIT);
	p0		ba	T_NoBothDenied;
	{
		and.testnz	p0, r0, #TA_IFLAGS_ABORTINPROGRESS;
		p0		ba	T_NoBothDenied;
		{
			/* Make sure the OUTOFMEM flag is not set */
			ldr		r1, #EUR_CR_EVENT_STATUS >> 2, drc0;
			wdf		drc0;
			mov		r2, #(EUR_CR_EVENT_STATUS_DPM_OUT_OF_MEMORY_GBL_MASK | \
							EUR_CR_EVENT_STATUS_DPM_OUT_OF_MEMORY_MT_MASK | \
							EUR_CR_EVENT_STATUS_TA_FINISHED_MASK);
			and.testnz	p0, r1, r2;
			/* if set don't abort, wait for event */
			p0 ba	T_NoAbortAll;
			{
				/* We could hit a potential deadlock, if we timeout send abortall */
				MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32MTETEDeadlockTicks)], drc0;
				wdf				drc0;
				iaddu32			r1, r1.low, #1;
				xor.testz		p0, r1, #SGX_LOCKUP_DETECT_SAMPLE_RATE;
				p0	mov		r1, #0;
				MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32MTETEDeadlockTicks)], r1;
				!p0 ba			T_NoAbortAll;
				{
					/* we have hit deadlock so send abort */
					str	#EUR_CR_DPM_OUTOFMEM >> 2, #EUR_CR_DPM_OUTOFMEM_ABORTALL_MASK;

					MK_TRACE(MKTC_TIMER_ABORTALL, MKTF_SCH | MKTF_HW | MKTF_TIM);

					/* Flag this was an abort all so we know what to do when rendering */
					mov		r1, #(TA_IFLAGS_ABORTALL | TA_IFLAGS_ABORTINPROGRESS | TA_IFLAGS_IDLE_ABORT);
					or		r0, r0, r1;
					MK_STORE_STATE(ui32ITAFlags, r0);
				}
			}
			T_NoAbortAll:
			idf		drc0, st;
			wdf		drc0;
		}
	}
	T_NoBothDenied:
#endif /* defined(FIX_HW_BRN_23055) */

#if defined(FIX_HW_BRN_31109)
	MK_LOAD_STATE(r0, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testz		p0, r0, #TA_IFLAGS_TAINPROGRESS;
	p0		ba	T_NoSPMCheck;
	{
		ldr		r1, #EUR_CR_EVENT_STATUS >> 2, drc0;
		wdf		drc0;
		/* If we have an outofmem or tafinished event pending there is nothing to do */
		mov		r2, #(EUR_CR_EVENT_STATUS_TA_FINISHED_MASK | \
					  EUR_CR_EVENT_STATUS_DPM_OUT_OF_MEMORY_GBL_MASK | \
					  EUR_CR_EVENT_STATUS_DPM_OUT_OF_MEMORY_MT_MASK);
		and.testnz	p0, r1, r2;
		
		p0	ba	T_WaitForEvent;
		{
			shl.tests	p0, r0, #(31 - TA_IFLAGS_OUTOFMEM_BIT);
			p0	ba	T_SPMInProgress;
			{
				/* We need to check the page status against the threshold */
				ldr		r1, #EUR_CR_DPM_PAGE_STATUS >> 2, drc0;
				ldr		r2, #EUR_CR_DPM_ZLS_PAGE_THRESHOLD >> 2, drc0;
				ldr		r3, #EUR_CR_DPM_GLOBAL_PAGE_STATUS >> 2, drc0;
				ldr		r4, #EUR_CR_DPM_TA_GLOBAL_LIST >> 2, drc0;
				mov		r5, #USE_FALSE;
				wdf		drc0;
				shr		r1, r1, #EUR_CR_DPM_PAGE_STATUS_TA_SHIFT;
				shr		r3, r3, #EUR_CR_DPM_GLOBAL_PAGE_STATUS_TA_SHIFT;
				and		r4, r4, #EUR_CR_DPM_TA_GLOBAL_LIST_SIZE_MASK;
				
				/* if (page_status_ta - zls_threshold) >= 0 */
				isubu16.testns	p0, r1, r2;
				!p0	ba	T_NotMTOOM;
				{
					/* Check for TA finished event on slave core */
					ldr		r1, #EUR_CR_DPM_ABORT_STATUS_MTILE >> 2, drc0;
					wdf		drc0;
					/* and with its mask */
	   				and		r1, r1, #EUR_CR_MASTER_DPM_ABORT_STATUS_MTILE_CORE_MASK;
	    				/* shift to base address position */
			        	shr		r1, r1, #EUR_CR_MASTER_DPM_ABORT_STATUS_MTILE_CORE_SHIFT;

					/* Check TA finish bit */
					READCORECONFIG(r2, r1, #EUR_CR_EVENT_STATUS >> 2, drc0);
					mov		r5, #EUR_CR_EVENT_STATUS_TA_FINISHED_MASK;
					wdf		drc0;

					and.testnz	p1, r2, r5;
					p1		ba	T_SPMNotDetected;	/* core has finished, don't emit abort */
				}
				T_NotMTOOM:

				/* if (page_status_global - global_threshold) >= 0 */
				!p0	isubu16.testns	p0, r3, r4;
				p0	mov	r5, #USE_TRUE;
					
				/* check if we have reached either threshold */
				xor.testnz	p0, r5, #USE_TRUE;
				p0	ba	T_SPMNotDetected;
				{
				
					/* sample the check sums */
					ldr				r1, #EUR_CR_CLIP_CHECKSUM >> 2, drc0;
					ldr				r2, #EUR_CR_MASTER_VDM_CONTEXT_STORE_STREAM >> 2, drc0;
					wdf				drc0;
					xor				r1, r1, r2 ;

					ldr				r2, #EUR_CR_MTE_MEM_CHECKSUM >> 2, drc0;
					ldr				r3, #EUR_CR_MTE_TE_CHECKSUM >> 2, drc0;
					ldr				r4, #EUR_CR_TE_CHECKSUM >> 2, drc0;
					wdf				drc0;
					xor				r1, r1, r2;
					xor				r1, r1, r3;
					xor				r1, r1, r4;
										
					/* None of the events are pending, therefore we may need to issue the abort now, 
						check whether this is the first time we have detected the condition. */
					shl.testns	p0, r0, #(31 - TA_IFLAGS_TIMER_SPM_CHECKSUM_SAVED_BIT);
					p0	ba	T_SaveCheckSum;
					{	
						/* Have the checksums changed */
						MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32TACheckSum)], drc0;
						wdf		drc0;
						xor.testnz	p0, r1, r2;
						p0	ba	T_SaveCheckSum;
						{
							or	r0, r0, #TA_IFLAGS_TIMER_DETECTED_SPM;
							and	r0, r0, ~#TA_IFLAGS_TIMER_SPM_CHECKSUM_SAVED;
						
							/* save the state as the flag was set, TAOutOfMem will clear it */
							MK_STORE_STATE(ui32ITAFlags, r0);
							idf		drc0, st;
							wdf		drc0;
							/* The checksum has not changed since the last sample, nor an event been generated */
							/* Issue a outofmem_gbl loopback, if the loopback is already in flight we will not issue another one */
							EMITLOOPBACK(#USE_LOOPBACK_GBLOUTOFMEM);
												
							/* now wait for the loopback */
							ba T_WaitForEvent;
						}
					}
					T_SaveCheckSum:
					{
						
						/* This is either the first detection or the checksum has changed */
						/* Save the checksum to memory */
						MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32TACheckSum)], r1;
						
						/* Set the flag to indicate we think we have hit SPM */
						or	r0, r0, #TA_IFLAGS_TIMER_SPM_CHECKSUM_SAVED;
						
						/* save the state as the flag was set, TAOutOfMem will clear it */
						MK_STORE_STATE(ui32ITAFlags, r0);
						idf		drc0, st;
						wdf		drc0;
						
						/* Wait for the next timer event */
						ba	T_WaitForEvent;
					}
				}
				T_SPMNotDetected:
				/* If we had indicated a potential SPM requirement before */
				shl.testns	p0, r0, #(31 - TA_IFLAGS_TIMER_DETECTED_SPM_BIT);
				p0	ba	T_NoFlagClear;
				{
					
					/* clear the flag as the HW does not need intervention for now */
					and	r0, r0, ~#TA_IFLAGS_TIMER_DETECTED_SPM;
					MK_STORE_STATE(ui32ITAFlags, r0);
					idf		drc0, st;
					wdf		drc0;
				}
				T_NoFlagClear:
			}
			T_SPMInProgress:
		}
		T_WaitForEvent:
	}
	T_NoSPMCheck:

#endif

#if defined(SUPPORT_HW_RECOVERY)
	/*
		Increment the timer ticks counters, and check hardware lockups when
		a counter matches the corresponding USE_TIMER_xxx_TIMEOUT value.
	*/
	/*
		Without any TE signature register to check, false lockup detection would
		be generated. Therefore TA lockup detection must be disabled.
	*/
	MK_LOAD_STATE(r0, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testnz		p0, r0, #TA_IFLAGS_TAINPROGRESS;
	!p0		ba	T_NoTATimeout;
	{
		/*
			If there is an Abort in progress only check for lockup if there is
			not a render in progress. As the TA is stopped during an abort waiting
			on the render to finish.
		*/
		and.testnz	p0, r0, #TA_IFLAGS_ABORTINPROGRESS;
		MK_LOAD_STATE(r1, ui32IRenderFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		p0	and.testnz		p0, r1, #RENDER_IFLAGS_RENDERINPROGRESS;
		p0	ba 	T_NoTATimeout;
		{
			shl.tests	p0, r0, #(31 - TA_IFLAGS_OOM_PR_BLOCKED_BIT);
			p0	ba	T_NoTATimeout;
			{
				MK_LOAD_STATE(r0, ui32IRenderFlags, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);
				shl.tests p0, r0, #(31 - RENDER_IFLAGS_TAWAITINGFORMEM_BIT);
				p0	ba	T_NoTATimeout;
				{
					MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRTimerTicksSinceTA)], drc0;
					MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32HWRecoverySampleRate)], drc0;
					wdf				drc0;
#if !defined(EUR_CR_TE1) && !defined(EUR_CR_TE_DIAG1) && \
	!defined(EUR_CR_TE_CHECKSUM) && !defined(EUR_CR_MTE_TE_CHECKSUM)
					shl				r1, r1, #3;
#endif
					iaddu32			r0, r0.low, #1;
					xor.testz		p0, r0, r1;
					p0	mov		r0, #0;
					MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRTimerTicksSinceTA)], r0;
					idf				drc0, st;
					wdf				drc0;
					!p0 ba T_NoTATimeout;
					{
						/* if true, 1. Load the KickTA command flags to determine OpenCL task.
						     if Open CL task, load the SGXMKIF_HOST_CTL.ui32OpenCLDelayCount to determine the count left 
						     if zero load the counter again with the reference value and call CheckTALockup */
						MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTACCBCmd)], drc0;
						wdf		drc0;
						MK_MEM_ACCESS_CACHED(ldad)	r0, [r0, #DOFFSET(SGXMKIF_CMDTA.ui32Flags)], drc0;
						wdf		drc0;
						mov		r1, #SGXMKIF_TAFLAGS_TA_TASK_OPENCL;
						and.testz	p0, r0, r1;
						p0	ba 	CheckLockUp;
						{
							MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32OpenCLDelayCount)], drc0;
							wdf		drc0;
							and.testnz	p0, r0, r0; 
							p0 		iaddu32	r0, r0.low, #0xFFFFFFFF;
							MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32OpenCLDelayCount)], r0;
							idf		drc0, st;
							wdf		drc0;
						}
						p0		ba	T_NoTATimeout;
						CheckLockUp:
							bal			CheckTALockup;	
					}
					     
				}
			}
		}
	}
	T_NoTATimeout:

	MK_LOAD_STATE(r0, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testnz		p0, r0, #RENDER_IFLAGS_RENDERINPROGRESS;
	!p0		ba	T_No3DTimeout;
	{
		MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRTimerTicksSince3D)], drc0;
		MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32HWRecoverySampleRate)], drc0;
		wdf				drc0;
		iaddu32			r0, r0.low, #1;
		xor.testz		p0, r0, r1;
		p0 mov			r0, #0;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRTimerTicksSince3D)], r0;
		idf				drc0, st;
		wdf				drc0;
		p0 bal			Check3DLockup;
	}
	T_No3DTimeout:

#if defined(SGX_FEATURE_2D_HARDWARE)
	MK_LOAD_STATE(r0, ui32I2DFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testnz		p0, r0, #TWOD_IFLAGS_2DBLITINPROGRESS;
	!p0		ba	T_No2DLockup;
	{
		MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRTimerTicksSince2D)], drc0;
		MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32HWRecoverySampleRate)], drc0;
		wdf				drc0;
		iaddu32			r0, r0.low, #1;
		xor.testz		p0, r0, r1;
		p0 mov			r0, #0;
		MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32HWRTimerTicksSince2D)], r0;
		idf				drc0, st;
		wdf				drc0;
		p0 bal			Check2DLockup;

	}
	T_No2DLockup:
#endif
#endif /* defined(SUPPORT_HW_RECOVERY) */

	/* Check for host resource cleanup request. */
	bal	CheckHostCleanupRequest;

	/* Handle active power management. */
	MK_LOAD_STATE(r0, ui32ActivePowerCounter, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testz		p0, r0, r0;
	p0 ba			T_ActivePower_Finished;
	{
		isub16.testz	r0, p0, r0, #1;
		and				r0, r0, #0xFFFF;
		MK_STORE_STATE(ui32ActivePowerCounter, r0);
		MK_WAIT_FOR_STATE_STORE(drc0);

		!p0 ba			T_ActivePower_Finished;
		{
			MK_TRACE(MKTC_TIMER_ACTIVE_POWER, MKTF_TIM | MKTF_SCH | MKTF_POW);

			/* Set Active Power interrupt flag. */
			MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32InterruptFlags)], drc0;
			wdf				drc0;
			or				r1, r1, #PVRSRV_USSE_EDM_INTERRUPT_ACTIVE_POWER;
			MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32InterruptFlags)], r1;
			idf				drc0, st;
			wdf				drc0;

			/* Send iterrupt to host. */
			mov	 			r0, #EUR_CR_EVENT_STATUS_SW_EVENT_MASK;
			str	 			#EUR_CR_EVENT_STATUS >> 2, r0;
		}
	}
	T_ActivePower_Finished:

	/* Check for host power change request. */
	bal CheckHostPowerRequest;

#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH) && defined(SGX_SUPPORT_ISP_TIMER_BASED_SWITCHING)
	MK_LOAD_STATE(r0, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testz	p0, r0, #RENDER_IFLAGS_RENDERINPROGRESS;
	p0	ba	T_3DNotActive;
	{
		/* Cause a context switch, and resume then resume the scene */
		START3DCONTEXTSWITCH(T);
	}
	T_3DNotActive:
#endif
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) && defined(SGX_SUPPORT_VDM_TIMER_BASED_SWITCHING)
	MK_LOAD_STATE(r0, ui32ITAFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testz	p0, r0, #TA_IFLAGS_TAINPROGRESS;
	p0	ba	T_TANotActive;
	{
		/* Check for TA context store timeout */
		and.testnz	p0, r0, #TA_IFLAGS_HALTTA;
		p0	ba	T_TACSInProgress;
		{
			/* Cause a context switch, and resume then resume the scene */
			STARTTACONTEXTSWITCH(T);
		}
		T_TACSInProgress:
	}
	T_TANotActive:
#endif

	MK_TRACE(MKTC_TIMER_END, MKTF_TIM);

	/* branch back to calling code */
	mov		pclink, R_EDM_PCLink;
	lapc;
}


/*****************************************************************************
 CheckHostCleanupRequest - check host request flags for cleanup events

 inputs:
 temps:		r0, r3
*****************************************************************************/
CheckHostCleanupRequest:
{
	/*
		Check whether the driver is waiting to clean up anything.
	*/
	MK_LOAD_STATE(r3, ui32HostRequest, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);

	and.testz		p0, r3, #(SGX_UKERNEL_HOSTREQUEST_CLEANUP_RC | \
							  SGX_UKERNEL_HOSTREQUEST_CLEANUP_TC | \
							  SGX_UKERNEL_HOSTREQUEST_CLEANUP_RT | \
							  SGX_UKERNEL_HOSTREQUEST_CLEANUP_PB);
#if defined(SGX_FEATURE_2D_HARDWARE)
	p0 and.testz	p0, r3, #SGX_UKERNEL_HOSTREQUEST_CLEANUP_2DC;
#endif /* SGX_FEATURE_2D_HARDWARE */
	p0 ba			T_NoResmanRequests;
	{
		MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sResManCleanupData)], drc0;
		wdf				drc0;
		and.testnz		p0, r3, #SGX_UKERNEL_HOSTREQUEST_CLEANUP_RC;
		p0	ba			RenderContextCleanup;
		and.testnz		p0, r3, #SGX_UKERNEL_HOSTREQUEST_CLEANUP_RT;
		p0	ba			RenderTargetCleanup;
		and.testnz		p0, r3, #SGX_UKERNEL_HOSTREQUEST_CLEANUP_TC;
		p0	ba			TransferContextCleanup;
	#if defined(SGX_FEATURE_2D_HARDWARE)
		and.testnz		p0, r3, #SGX_UKERNEL_HOSTREQUEST_CLEANUP_2DC;
		p0	ba				TwoDContextCleanup;
	#endif /* SGX_FEATURE_2D_HARDWARE */
		and.testnz		p0, r3, #SGX_UKERNEL_HOSTREQUEST_CLEANUP_PB;
		p0	ba			SharedPBDescCleanup;
	}
	T_NoResmanRequests:

	lapc;
}


/*****************************************************************************
 CheckHostPowerRequest - check host request flags for power events

 inputs:
 temps:		r0, r1, r2, r3, r4, r5, r6, r7, r8, r9
*****************************************************************************/
CheckHostPowerRequest:
{
	/*
		Check whether the host is requesting to idle the core.
	*/
	MK_LOAD_STATE(r6, ui32HostRequest, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	and.testnz		p0, r6, #SGX_UKERNEL_HOSTREQUEST_IDLE | SGX_UKERNEL_HOSTREQUEST_POWEROFF;
	!p0		ba	T_NoPowerOffEvent;
	{
		mov	r1, #USE_FALSE;

		MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32PowerStatus)], drc0;
		wdf		drc0;

		/* check the Render and TA Flags */
		MK_LOAD_STATE(r2, ui32IRenderFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		/* Check if there is a render active */
		and.testz	p0, r2, #RENDER_IFLAGS_RENDERINPROGRESS;
		p0	ba	T_Power3DIdle;
		{
			/* Start the ISP context store */
			/* We are turning off, so dont start anything new. */
			START3DCONTEXTSWITCH(T_Power);

			MK_TRACE(MKTC_TIMER_POWER_3D_ACTIVE, MKTF_TIM | MKTF_SCH | MKTF_POW);

			or	r1, r1, #USE_TRUE;
		}
		T_Power3DIdle:

		/* Check if there is a TA active */
		MK_LOAD_STATE(r2, ui32ITAFlags, drc1);
		MK_WAIT_FOR_STATE_LOAD(drc1);
		and.testz	p0, r2, #TA_IFLAGS_TAINPROGRESS;
		p0	ba	T_PowerTAIdle;
		{
			STARTTACONTEXTSWITCH(T_Power);
			MK_TRACE(MKTC_TIMER_POWER_TA_ACTIVE, MKTF_TIM | MKTF_SCH | MKTF_POW);
			or	r1, r1, #USE_TRUE;
		}
		T_PowerTAIdle:

#if defined(SGX_FEATURE_2D_HARDWARE)
		MK_LOAD_STATE(r2, ui32I2DFlags, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		and.testz		p0, r2, #TWOD_IFLAGS_2DBLITINPROGRESS;
		p0	ba T_Power2DIdle;
		{
			MK_TRACE(MKTC_TIMER_POWER_2D_ACTIVE, MKTF_TIM | MKTF_SCH | MKTF_POW);
			or	r1, r1, #USE_TRUE;
		}
		T_Power2DIdle:
#endif

		/* If any of the core were active wait for them to finish */
		xor.testz	p0, r1, #USE_TRUE;
		p0	ba	T_NoPowerOffEvent;
		{
			/* Ensure that there are no pending uKernel events. */
	#if defined(EUR_CR_USE0_SERV_EVENT)
			ldr		r1, #EUR_CR_USE0_SERV_EVENT >> 2, drc0;
			mov		r2, #EUR_CR_USE0_SERV_EVENT_EMPTY_MASK;
	#else
			ldr		r1, #EUR_CR_USE_SERV_EVENT >> 2, drc0;
			mov		r2, #EUR_CR_USE_SERV_EVENT_EMPTY_MASK;
	#endif /* EUR_CR_USE0_SERV_EVENT */
			wdf	drc0;
			and.testnz	p0, r1, r2;
			p0	ba T_PowerNoPendingEvents;
			{
				MK_TRACE(MKTC_TIMER_POWER_PENDING_EVENTS, MKTF_TIM | MKTF_SCH | MKTF_POW);
				ba	T_NoPowerOffEvent;
			}
			T_PowerNoPendingEvents:

			WRITEHWPERFCB(TA, POWER_END, ANY, #0, p0, T_power_end, r1, r2, r3, r4, r5, r7, r8, r9);

			/* Stop ukernel timer. */
			str	#SGX_MP_CORE_SELECT(EUR_CR_EVENT_TIMER, 0) >> 2, #0;

			/* SGX is now idle. */
			or r0, r0, #PVRSRV_USSE_EDM_POWMAN_IDLE_COMPLETE;
			MK_TRACE(MKTC_TIMER_POWER_IDLE, MKTF_TIM | MKTF_SCH | MKTF_POW);

			and.testnz	p0, r6, #SGX_UKERNEL_HOSTREQUEST_POWEROFF;
			!p0			ba	T_PowerOffComplete;
			{
				/* Store PB context of the PB(s) currenly in use. */
				MK_MEM_ACCESS_BPCACHE(ldad)	r1, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc)], drc0;
				wdf 			drc0;
				and.testz		p0, r1, r1;
				p0 ba			T_PowerTAPBContextStoreDone;
				{
					STORETAPB();
				}
				T_PowerTAPBContextStoreDone:

				MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DHWPBDesc)], drc0;
				wdf 			drc0;
				/* if TAPB == 3DPB then it has already been stored, or does not need storing */
				xor.testz		p0, r1, r2;
				p0 ba			T_Power3DPBContextStoreDone;
				{
					/* TAPB != 3DPB, check if PB needs storing */
					and.testz		p0, r2, r2;
					p0 ba			T_Power3DPBContextStoreDone;
					{
						STORE3DPB();
					}
				}
				T_Power3DPBContextStoreDone:
				MK_MEM_ACCESS_BPCACHE(stad) [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc)], #0;
				MK_MEM_ACCESS_BPCACHE(stad) [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DHWPBDesc)], #0;

				/* If either RTData is still in use, store it for use on power-up. */
				mov				r3, #0; // Hardware context loop counter

				T_PowerContextStore:
				/* r5 = RTData for this context */
				iaddu32			r4, r3.low, #DOFFSET(SGXMK_TA3D_CTL.sContext0RTData);
				imae			r4, r4.low, #SIZEOF(SGXMK_TA3D_CTL.sContext0RTData), R_TA3DCtl, u32;
				MK_MEM_ACCESS_BPCACHE(ldad)	r5, [r4], drc0;
				wdf 			drc0;
				and.testz		p0, r5, r5;
				p0 ba			T_PowerContextStoreDone;
				{
					/* Mark this context as free. */
					MK_MEM_ACCESS_BPCACHE(stad)	[r4], #0;
					idf				drc0, st;
					wdf				drc0;

					/* r1 = HW Render Context */
					MK_MEM_ACCESS(ldad)	r1, [r5, #DOFFSET(SGXMKIF_HWRTDATA.sHWRenderContextDevAddr)], drc0;
					wdf				drc0;

					/* Check whether the TA is finished */
					MK_MEM_ACCESS(ldad)	r2, [r5, #DOFFSET(SGXMKIF_HWRTDATA.ui32CommonStatus)], drc0;
					wdf				drc0;
					and.testnz 		p0, r2, #SGXMKIF_HWRTDATA_CMN_STATUS_TACOMPLETE;

					p0 ba			T_PowerContextStore3D;
					{
						SETUPDIRLISTBASE(T_STAC, r1, #SETUP_BIF_TA_REQ);
						STORETACONTEXT(r5, r3);
						ba T_PowerContextStoreDone;
					}
					T_PowerContextStore3D:
					{
						SETUPDIRLISTBASE(T_S3DC, r1, #SETUP_BIF_3D_REQ);
						STORE3DCONTEXT(r5, r3);
					}
				}
				T_PowerContextStoreDone:
				iaddu32			r3, r3.low, #1;
				xor.testnz		p0, r3, #2;
				p0 ba			T_PowerContextStore;

				MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext0RTData)], #0;
				MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sContext1RTData)], #0;
				MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.sTARTData)], #0;
				MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRTData)], #0;

				/* Check for any outstanding kernel CCB commands. */
				MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_CCBCtl, #DOFFSET(PVRSRV_SGX_CCB_CTL.ui32WriteOffset)], drc0;
				MK_MEM_ACCESS_BPCACHE(ldad)	r3, [R_CCBCtl, #DOFFSET(PVRSRV_SGX_CCB_CTL.ui32ReadOffset)], drc0;
				wdf				drc0;
				xor.testnz		p0, r2, r3;
				!p0 ba			T_PowerCheckRestartImmediate;
				/* There is a bug in the host driver. */
				MK_ASSERT_FAIL(MKTC_TIMER_POWER_CCB_ERROR);

				T_PowerCheckRestartImmediate:
				/*
					CCB commands submitted since the active power interrupt was sent may
					require the ukernel to be restarted immediately.
				*/
				MK_LOAD_STATE(r2, ui32ActivePowerFlags, drc0);
				MK_WAIT_FOR_STATE_LOAD(drc0);
				and.testz		p0,	r2, r2;
				p0	ba			T_PowerCheckWorkQueues;
				MK_TRACE(MKTC_TIMER_POWER_RESTART_IMMEDIATE, MKTF_TIM | MKTF_SCH | MKTF_POW);
				or				r0, r0, #PVRSRV_USSE_EDM_POWMAN_POWEROFF_RESTART_IMMEDIATE;
				ba				T_PowerNoRestartDone;

				T_PowerCheckWorkQueues:
				/* Signal whether we need to be restarted after the power event. */
				mov		r2, #0;
				/* Start at priority level 0 */
				mov		r3, #0;
				T_PowerCheckNextPartialPriority:
				{
					MK_LOAD_STATE_INDEXED(r2, sPartialRenderContextHead, r3, drc0, r4);
					MK_WAIT_FOR_STATE_LOAD(drc0);
					and.testz	p0, r2, r2;
					/* exit when we find work in a list */
					p0	iaddu32	r3, r3.low, #1;
					p0	xor.testnz	p0, r3, #SGX_MAX_CONTEXT_PRIORITY;
					p0	ba	T_PowerCheckNextPartialPriority;
				}

				/* Start at priority level 0 */
				mov		r3, #0;
				T_PowerCheckNextCompletePriority:
				{
					MK_LOAD_STATE_INDEXED(r4, sCompleteRenderContextHead, r3, drc0, r5);
					MK_WAIT_FOR_STATE_LOAD(drc0);

					and.testz	p0, r4, r4;
					/* exit when we find work in a list */
					p0	iaddu32	r3, r3.low, #1;
					p0	xor.testnz	p0, r3, #SGX_MAX_CONTEXT_PRIORITY;
					p0	ba	T_PowerCheckNextCompletePriority;
				}

				/* Combine the Partial and Complete result */
				or	r2, r2, r4;

				/* Start at priority level 0 */
				mov		r3, #0;
				T_PowerCheckNextTransferPriority:
				{
					MK_LOAD_STATE_INDEXED(r4, sTransferContextHead, r3, drc0, r5);
					MK_WAIT_FOR_STATE_LOAD(drc0);

					and.testz	p0, r4, r4;
					/* exit when we find work in a list */
					p0	iaddu32	r3, r3.low, #1;
					p0	xor.testnz	p0, r3, #SGX_MAX_CONTEXT_PRIORITY;
					p0	ba	T_PowerCheckNextTransferPriority;
				}
				/* Combine the Transfer result  */
				or	r2, r2, r4;
		#if defined(SGX_FEATURE_2D_HARDWARE)
				/* Start at priority level 0 */
				mov		r3, #0;
				T_PowerCheckNext2DPriority:
				{
					MK_LOAD_STATE_INDEXED(r4, s2DContextHead, r3, drc0, r5);
					MK_WAIT_FOR_STATE_LOAD(drc0);
					and.testz	p0, r4, r4;
					/* exit when we find work in a list */
					p0	iaddu32	r3, r3.low, #1;
					p0	xor.testnz	p0, r3, #SGX_MAX_CONTEXT_PRIORITY;
					p0	ba	T_PowerCheckNext2DPriority;
				}
				/* Combine the 2D result */
				or	r2, r2, r4;
		#endif
				/* Check whether all the queues were empty */
				and.testz	p0, r2, r2;
				p0 or			r0, r0, #PVRSRV_USSE_EDM_POWMAN_NO_WORK;

				T_PowerNoRestartDone:
				or r0, r0, #PVRSRV_USSE_EDM_POWMAN_POWEROFF_COMPLETE;
			}
			T_PowerOffComplete:

			/* Clear the host power request. */
			and r1, r6, #~(SGX_UKERNEL_HOSTREQUEST_IDLE | SGX_UKERNEL_HOSTREQUEST_POWEROFF);
			MK_STORE_STATE(ui32HostRequest, r1);

			/* Clear the pending loopbacks value */
			MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32PendingLoopbacks)], #0;

			and.testnz	p0, r6, #SGX_UKERNEL_HOSTREQUEST_POWEROFF;
			!p0			ba	T_NotPowerOff;
			{
				MK_TRACE(MKTC_TIMER_POWER_OFF, MKTF_TIM | MKTF_SCH | MKTF_POW);

				/* Save the microkernel state to memory. */
				bal StoreSecondaries;
			}
			T_NotPowerOff:

			/* Update the host flags. */
			MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, #DOFFSET(SGXMKIF_HOST_CTL.ui32PowerStatus)], r0;
			
			UKERNEL_PROGRAM_END(#1, 0, 0);
		}
	}
	T_NoPowerOffEvent:
	lapc;
}


/*****************************************************************************
 StoreSecondaries - Write the secondary attributes to memory for use on restart

 inputs:
 temps: r1, r2, r3
*****************************************************************************/
StoreSecondaries:
{
	#if !defined(SGX_DISABLE_UKERNEL_SECONDARY_STATE)
	mov	r3, #SIZEOF(PVRSRV_SGX_EDMPROG_SECATTR) / SIZEOF(IMG_UINT32);
	mov	r1, #0;

	SS_Loop:
	or	r2, r1, #0x40000;
	mov	i.l, r1;
	MK_MEM_ACCESS_BPCACHE(stad) [R_TA3DCtl, r2], sa[i.l];
	iaddu32	r1, r1.low, #1;
	xor.testz	p0, r1, r3;
	!p0	ba	SS_Loop;

	idf 			drc0, st;
	wdf 			drc0;
	#endif /* SGX_DISABLE_UKERNEL_SECONDARY_STATE */

	lapc;
}


/*****************************************************************************
 End of file (sgx_timer.use.asm)
*****************************************************************************/
