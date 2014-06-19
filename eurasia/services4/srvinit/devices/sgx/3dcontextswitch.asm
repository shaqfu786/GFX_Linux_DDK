/*****************************************************************************
 Name		: 3dcontextswitch.asm

 Title		: SGX microkernel 3D context switch utility code

 Author 	: Imagination Technologies

 Created  	: 09/06/2008

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

 $Log: 3dcontextswitch.asm $
 
  --- Revision Logs Removed --- 

*****************************************************************************/

#include "usedefs.h"

/***************************
 Sub-routine Imports 
***************************/
.import MKTrace;
.import SwitchEDMBIFBank;

#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH)
.import	Check3DQueues;
.import CheckTAQueue;
	#if defined(SGX_FEATURE_MK_SUPERVISOR) && defined(FIX_HW_BRN_30701)
.import	JumpToSupervisor;
	#endif

.export RenderHalt;
.export ResumeRender;
#endif

.modulealign 2;

/*****************************************************************************
 Macro definitions
*****************************************************************************/

/*****************************************************************************
 Start of program
*****************************************************************************/

#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH_REV_3)
/*****************************************************************************
 RenderHalt
	This routine handles the storing of HW state during an ISP context switch
	away from the current render.

 inputs:
 temps:		r0, r1, r2, r3, r4, r5, r6, R_RTData
*****************************************************************************/
RenderHalt:
{
	MK_TRACE(MKTC_RENDERHALT, MKTF_3D | MKTF_SCH | MKTF_CSW);

	/* This is a context switch so clear the flags */
	MK_LOAD_STATE(r0, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	#if defined(SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH)
	and.testnz	p0, r0, #RENDER_IFLAGS_TRANSFERRENDER;
	and		r0, r0, ~#RENDER_IFLAGS_TRANSFERRENDER;
	#endif
	and		r0, r0, #~(RENDER_IFLAGS_RENDERINPROGRESS | RENDER_IFLAGS_SPMRENDER | \
						RENDER_IFLAGS_HALTRENDER | RENDER_IFLAGS_HALTRECEIVED | \
						RENDER_IFLAGS_FORCEHALTRENDER);
	MK_STORE_STATE(ui32IRenderFlags, r0);
	
	MK_TRACE(MKTC_RH_CLEARFLAGS, MKTF_3D | MKTF_SCH | MKTF_CSW);
	
	/* Start loading the holding structure base address */
	MK_LOAD_STATE(r0, s3DContext, drc1);
	MK_WAIT_FOR_STATE_LOAD(drc1);
	
	#if defined(SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH)
	p0	ba	RH_PDSStateSaved;
	#endif
	{
		MK_MEM_ACCESS_BPCACHE(ldad)	R_RTData, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRTData)], drc0;
		wdf		drc0;
	
	#if defined(SGX_FEATURE_MP)	
		/* If the render was context switched before check PDS state before saving */
		MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sRTStatusDevAddr)], drc0;
		wdf		drc0;
		MK_MEM_ACCESS(ldad)	r2, [r2], drc0;
		wdf		drc0;
		and.testz	p2, r2, #SGXMKIF_HWRTDATA_RT_STATUS_ISPCTXSWITCH;
	#endif
	
		/* Offset to the save location in the context */
		mov		r1, #OFFSET(SGXMKIF_HWRENDERCONTEXT.asPDSStateBaseDevAddr[0][0]);
		iaddu32	r1, r1.low, r0;
		
		mov		r2, #OFFSET(SGXMKIF_HWRENDERCONTEXT.ui32PDSStatus[0][0]);
		iaddu32	r2, r2.low, r0;
		
	#if defined(SGX_FEATURE_MP)
		/* Need to set the check each cores status to see which streams are valid */
		MK_LOAD_STATE_CORE_COUNT_3D(r6, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		mov		r3, #0;
		RH_CheckNextCoreStatus:
	#endif
		{
			/* If we have not context switched on this render before we must save the state */
	#if defined(SGX_FEATURE_MP)
			p2	ba	RH_SavePDSState;
			{	
				/* This render has been switched atleast once before */
				
				/* Check if this core has already finished */
				MK_MEM_ACCESS(ldad)	r4, [r2], drc0;
				wdf		drc0;
				and.testz	p1, r4, #EUR_CR_PDS_STATUS_RENDER_COMPLETED_MASK;
				p1	ba	RH_SavePDSState;
				{
					/* This core has already completed so keep the current state */
					REG_INCREMENT_BYVALUE(r2, p1, #sizeof(IMG_UINT32));
					REG_INCREMENT_BYVALUE(r1, p1, #sizeof(IMG_UINT32));
					ba	RH_PDSStateUpdated;
				}
			}
			RH_SavePDSState:
	#endif
			{
				/* Read the core specific registers? */
				READCORECONFIG(r4, r3, #EUR_CR_PDS_CONTEXT_STORE >> 2, drc0);
				READCORECONFIG(r5, r3, #EUR_CR_PDS_STATUS >> 2, drc0);
				wdf		drc0;
				
				/* Save the PDS_STATUS value to memory */
				MK_MEM_ACCESS(stad)	[r2, #1++], r5;
				
				/* Clear the tile-mode bit */
				and		r4, r4, ~#EUR_CR_PDS_CONTEXT_STORE_TILE_ONLY_MASK;
				
				/* Did the core store a valid stream */
				and.testz	p1, r4, #EUR_CR_PDS_CONTEXT_STORE_DISABLE_MASK;
				p1 and.testnz	p1, r5, #EUR_CR_PDS_STATUS_CSWITCH_COMPLETED_MASK;
				!p1	and		r4, r4, ~#EUR_CR_PDS_CONTEXT_RESUME_VALID_STREAM_MASK;
				p1	or		r4, r4, #EUR_CR_PDS_CONTEXT_RESUME_VALID_STREAM_MASK;
				
				/* Save the value to memory */
				MK_MEM_ACCESS(stad)	[r1, #1++], r4;
			}
			RH_PDSStateUpdated:
	
	#if defined(SGX_FEATURE_MP)		
			/* Is there another core to check? */
			REG_INCREMENT(r3, p1);
			xor.testnz	p1, r3, r6;
			p1	ba	RH_CheckNextCoreStatus;	
	#endif
		}
	}
	RH_PDSStateSaved:
	
	#if defined(SGX_FEATURE_MP)
		#if defined(SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH)
	p0	ba	RH_DPMStateSaved;
		#endif
	{
		/* Offset to the save location in the context */
		mov		r1, #OFFSET(SGXMKIF_HWRENDERCONTEXT.ui32DPMFreeStatus[0]);
		iaddu32	r1, r1.low, r0;
		
		/* Need to set the check each cores status to see which streams are valid */
		MK_LOAD_STATE_CORE_COUNT_3D(r2, drc0);
		MK_WAIT_FOR_STATE_LOAD(drc0);
		mov		r3, #EUR_CR_MASTER_DPM_TSP0_MTILEFREE >> 2;
		RH_SaveNextDPMState:
		{
			/* Read the registers */
			ldr		r4, r3, drc0;
			wdf		drc0;
			/* Save the value to memory */
			MK_MEM_ACCESS(stad)	[r1, #1++], r4;
					
			/* Increment the register */
			REG_INCREMENT(r3, p1);
			/* Have we saved all the cores yet? */
			isub16.testnz	r2, p1, r2, #1;
			p1	ba	RH_SaveNextDPMState;	
		}
		
		READMASTERCONFIG(r3, #EUR_CR_MASTER_DPM_DEALLOCATE_MASK >> 2, drc0);
		wdf		drc0;
		MK_MEM_ACCESS(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32DPMDeallocMask)], r3;
	}
	RH_DPMStateSaved:
	#endif
			
	/* Save the ISP context switch registers to memory */
	#if defined(SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH)
	p0	mov	r1, #OFFSET(SGXMKIF_HWTRANSFERCONTEXT.ui32ISP2CtxResume);
	p0	mov	r6, #offset(SGXMKIF_HWTRANSFERCONTEXT.ui32ISPRestart);
	p0	ba	RH_ISPStateOffsetReady;
	#endif
	{
		mov	r1, #OFFSET(SGXMKIF_HWRENDERCONTEXT.ui32ISP2CtxResume);
		mov r6, #OFFSET(SGXMKIF_HWRENDERCONTEXT.ui32ISPRestart);
	}
	RH_ISPStateOffsetReady:
	
	/* Add the offset to the base address */
	iaddu32		r6, r6.low, r0;
	iaddu32		r0, r1.low, r0;
	#if defined(SGX_FEATURE_MP)
	mov		r1, #0;
	MK_LOAD_STATE_CORE_COUNT_3D(r2, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	#endif
	RH_SaveNextCore:
	{
	
	#if defined(SGX_FEATURE_MP)
		/* Check if we need to save the ISP status registers */
		READCORECONFIG(r4, r1, #EUR_CR_PDS_CONTEXT_STORE >> 2, drc0);
		READCORECONFIG(r5, r1, #EUR_CR_PDS_STATUS >> 2, drc0);
		wdf		drc0;
		and.testnz	p2, r4, #EUR_CR_PDS_CONTEXT_STORE_DISABLE_MASK; 
		p2	shl.testns	p2, r5, #(31 - EUR_CR_PDS_STATUS_CSWITCH_COMPLETED_SHIFT);
		/* Don't save the register is PDS store disabled and !CSWITCH_COMPLETED */
		p2	ba	RH_NoRegisterSave;
	#endif
		{
			/* Read the next config register */
			READCORECONFIG(r5, r1, #EUR_CR_ISP_STATUS2 >> 2, drc0);
			wdf		drc0;
			shr		r5, r5, #(EUR_CR_ISP_STATUS2_PRIM_NUM_SHIFT - EUR_CR_ISP_START_PRIM_NUM_SHIFT);
			MK_MEM_ACCESS(stad)	[r6, #1++], r5;
				
			mov		r3, #EUR_CR_ISP_CONTEXT_SWITCH2 >> 2;
	#if defined(SGX_FEATURE_MP)
			mov		r4, #EUR_CR_ISP_CONTEXT_SWITCH9 >> 2;
	#else
			mov		r4, #EUR_CR_ISP_CONTEXT_SWITCH3 >> 2;
	#endif
			RH_SaveNextReg:
			{
				/* Is this the last register to save for this core */
				xor.testnz	p2, r3, r4;
				/* Read the next config register */
				READCORECONFIG(r5, r1, r3, drc0);
				wdf		drc0;
				MK_MEM_ACCESS(stad)	[r0, #1++], r5;
				iaddu32	r3, r3.low, #1;
				p2	ba	RH_SaveNextReg;
			}
			
	#if defined(SGX_FEATURE_MP)
			ba	RH_RegisterSaveDone;
	#endif
		}
	#if defined(SGX_FEATURE_MP)
		RH_NoRegisterSave:
		{
			/* Move the addresses on as if we had saved the register state */
			mov		r3, #(((EUR_CR_ISP_CONTEXT_SWITCH9 - EUR_CR_ISP_CONTEXT_SWITCH2) >> 2) + 1);
			mov		r4, #SIZEOF(IMG_UINT32);
			imae	r0, r3.low, #SIZEOF(IMG_UINT32), r0, u32;
			iaddu32	r6, r4.low, r6;
		}
		RH_RegisterSaveDone:
	
		/* Are there any more cores to save */
		REG_INCREMENT(r1, p2);
		xor.testnz	p2, r1, r2;
		p2	ba	RH_SaveNextCore;
	#endif
	}
	
#if defined(SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH)
	p0	ba	RH_SetTransferRenderFlags;
	{
#endif
		/* Load the RT data structure for the render */
		MK_MEM_ACCESS_BPCACHE(ldad) r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRTData)], drc1;
		wdf		drc1;
		
		/* Make sure the RT status flags are loaded */
		MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWRTDATA.sRTStatusDevAddr)], drc1;
		wdf		drc1;
	#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
		MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32ActiveRTIdx)], drc0;
		wdf		drc0;
		imae	r1, r2.low, #SIZEOF(IMG_UINT32), r1, u32;
	#endif
		MK_MEM_ACCESS(ldad)	r2, [r1], drc0;
		wdf		drc0;
		/* Record that we have context switched on this scene for later use */
		or		r2, r2, #SGXMKIF_HWRTDATA_RT_STATUS_ISPCTXSWITCH;
		MK_MEM_ACCESS(stad)	[r1], r2;
#if defined(SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH)
		ba	RH_FlagsSet;
	}
	RH_SetTransferRenderFlags:
	{
		MK_LOAD_STATE(r0, sTransferCCBCmd, drc1);
		MK_WAIT_FOR_STATE_LOAD(drc1);
		MK_MEM_ACCESS_BPCACHE(ldad)	r1, [r0, #DOFFSET(SGXMKIF_TRANSFERCMD.ui32Flags)], drc0;
		wdf		drc0;
		or	r1, r1, #SGXMKIF_TQFLAGS_CTXSWITCH;
		MK_MEM_ACCESS_BPCACHE(stad)	[r0, #DOFFSET(SGXMKIF_TRANSFERCMD.ui32Flags)], r1;
	}
	RH_FlagsSet:
#endif
	idf		drc0, st;
	wdf		drc0;
	
	#if defined(FIX_HW_BRN_30701) || (defined(FIX_HW_BRN_30656) || defined(FIX_HW_BRN_30700))
		#if defined(FIX_HW_BRN_30701)
	SGXMK_SPRV_WRITE_REG(#EUR_CR_SOFT_RESET >> 2, #EUR_CR_SOFT_RESET_ISP_RESET_MASK);
		#endif
		#if (defined(FIX_HW_BRN_30656) || defined(FIX_HW_BRN_30700))
	SGXMK_SPRV_WRITE_REG(#EUR_CR_SOFT_RESET >> 2, #EUR_CR_SOFT_RESET_PBE_RESET_MASK);
		#endif
	SGXMK_SPRV_WRITE_REG(#EUR_CR_SOFT_RESET >> 2, #0);
	#endif

	/* Check the queues */
	FIND3D();
	FINDTA();

	MK_TRACE(MKTC_RENDERHALT_END, MKTF_3D | MKTF_SCH | MKTF_CSW);
	
	/* branch back to calling code */
	mov		pclink, R_EDM_PCLink;

	lapc;
}

/*****************************************************************************
 ResumeRender
	This routine handles the retoring of HW state when resuming a render
	previously context switched.

 inputs:	r0 - SGXMKIF_HWRENDERCONTEXT of the render to be resumed
 temps:		r2, r3, r4, r5, r6, r7
*****************************************************************************/
ResumeRender:
{
	MK_TRACE(MKTC_KICKRENDER_RESUME, MKTF_3D | MKTF_SCH);
#if defined(DEBUG)
	/* Count number of resumes */
	MK_MEM_ACCESS_BPCACHE(ldad)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32Num3DContextResumes)], drc0;
	wdf		drc0;
	mov		r3, #1;
	iaddu32		r2, r3.low, r2;
	MK_MEM_ACCESS_BPCACHE(stad)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32Num3DContextResumes)], r2;
	idf	drc0, st;
	wdf	drc0;
#endif
	/* We know that there is only 1 PDS per core */
	mov		r2, #OFFSET(SGXMKIF_HWRENDERCONTEXT.asPDSStateBaseDevAddr);
	iaddu32	r2, r2.low, r0;
	
	#if defined(SGX_FEATURE_MP)
	/* Need to set the check each cores status to see which streams are valid */
	mov		r3, #0;
	MK_LOAD_STATE_CORE_COUNT_3D(r6, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	RR_PDSBaseNextCore:
	#endif
	{
		MK_MEM_ACCESS(ldad)	r4, [r2], drc0;
		wdf		drc0;
		WRITECORECONFIG(r3, #EUR_CR_PDS_CONTEXT_RESUME >> 2, r4, r5);
		
		/* Clear the VALID/DISABLE bit ready for the next store */
		and		r4, r4, ~#EUR_CR_PDS_CONTEXT_STORE_DISABLE_MASK;
		MK_MEM_ACCESS(stad)	[r2, #1++], r4;

	#if defined(SGX_FEATURE_MP)	
		REG_INCREMENT(r3, p1);
		xor.testnz	p1, r3, r6;
		p1	ba	RR_PDSBaseNextCore;
	#endif
	}
	/* Ensure the right have completed */
	idf		drc0, st;
	wdf		drc0;
	
	#if defined(SGX_FEATURE_MP)
	/* Offset to the save location in the context */
	mov		r2, #OFFSET(SGXMKIF_HWRENDERCONTEXT.ui32DPMFreeStatus[0]);
	iaddu32 r2, r2.low, r0;
	
	/* Need to set the check each cores status to see which streams are valid */
	MK_LOAD_STATE_CORE_COUNT_3D(r3, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	mov		r4, #EUR_CR_MASTER_DPM_TSP0_START_OF_MTILEFREE >> 2;
	RH_RestoreNextDPMState:
	{
		MK_MEM_ACCESS(ldad)	r5, [r2, #1++], drc0;
		wdf		drc0;
		
		str		r4, r5;
				
		/* Increment the register */
		REG_INCREMENT(r4, p1);
		/* Have we saved all the cores yet? */
		isub16.testnz	r3, p1, r3, #1;
		p1	ba	RH_RestoreNextDPMState;	
	}
	
	MK_MEM_ACCESS(ldad)	r3, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32DPMDeallocMask)], drc0;
	wdf		drc0;
	WRITEMASTERCONFIG(#EUR_CR_MASTER_DPM_START_OF_DEALLOCATE_MASK >> 2, r3, r4);
	#endif
	
	/* reload the prim num to each core */
	mov				r2, #OFFSET(SGXMKIF_HWRENDERCONTEXT.ui32ISPRestart);
	iaddu32			r2, r2.low, r0;
	COPYMEMTOCONFIGREGISTER_3D(RR_PrimNum, EUR_CR_ISP_START, r2, r3, r4, r5);
	
	/* Program the buffers to load depth from */
	mov				r2, #OFFSET(SGXMKIF_HWRENDERCONTEXT.sZLSCtxSwitchBaseDevAddr);
	iaddu32			r2, r2.low, r0;
	COPYMEMTOCONFIGREGISTER_3D(RR_ZBase, EUR_CR_ISP_CONTEXT_RESUME, r2, r3, r4, r5);
	
	/* Restore the ISP context switch registers from memory */
	mov		r2, #OFFSET(SGXMKIF_HWRENDERCONTEXT.ui32ISP2CtxResume);
	iaddu32	r2, r2.low, r0;
	
	#if defined(SGX_FEATURE_MP)	
	mov		r3, #OFFSET(SGXMKIF_HWRENDERCONTEXT.ui32PDSStatus[0][0]);
	iaddu32	r3, r3.low, r0;
	
	MK_LOAD_STATE_CORE_COUNT_3D(r8, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
	mov		r7, #0;
	RR_LoadNextCore:
	#endif
	{
		MK_MEM_ACCESS(ldad) r5,	[r2, #1++], drc0;
		wdf		drc0;
		
	#if defined(SGX_FEATURE_MP)
		/* 	
			if PDS_RENDER_COMPLETE == 0 
			{
				set CORE_WILL_END_RENDER
				
				if PDS_EOR_STORED
				{
					// core not active but expect eor 
					set RESUME2_END_OF_RENDER
				}	
				else
				{
					// core active expect eor
					clear  RESUME2_END_OF_RENDER
				}
			}
			else
			{
				// core not active and not expect eor 
				set RESUME2_END_OF_RENDER
				clear CORE_WILL_END_RENDER
			}
		*/
		MK_MEM_ACCESS(ldad)	r6, [r3, #1++], drc0;
		wdf		drc0;
		and.testnz	p2, r6, #EUR_CR_PDS_STATUS_RENDER_COMPLETED_MASK;
		p2	ba	RR_RenderCompleted;
		{
			/* The core has not completed yet and there for will generate an EOR */
			or	r5, r5, #EUR_CR_MASTER_ISP_CORE0_CONTEXT_RESUME2_CORE_ACTIVE_MASK;
			
			/* Has the PDS has store the end of render? */
			and.testnz	p1, r6, #EUR_CR_PDS_STATUS_CSWITCH_STORE_END_RENDER_MASK;		
			p1	or		r5, r5, #EUR_CR_MASTER_ISP_CORE0_CONTEXT_RESUME2_END_OF_RENDER_MASK;
			!p1	and		r5, r5, ~#EUR_CR_MASTER_ISP_CORE0_CONTEXT_RESUME2_END_OF_RENDER_MASK;
			
			WRITECORECONFIG(r7, #EUR_CR_ISP_CONTEXT_RESUME2 >> 2, r5, r6);
			
			ba	RR_WriteMasterResume2;
		}
		RR_RenderCompleted:
		{
			/* 
				The Core is not active anymore from MIPF point of view and will also
				not generate an EOR as it has already finished
			*/
			or	r5, r5, #(EUR_CR_MASTER_ISP_CORE0_CONTEXT_RESUME2_END_OF_RENDER_MASK | \
							EUR_CR_MASTER_ISP_CORE0_CONTEXT_RESUME2_END_OF_TILE_MASK);
			and	r5, r5, ~#EUR_CR_MASTER_ISP_CORE0_CONTEXT_RESUME2_CORE_ACTIVE_MASK;
			
			WRITECORECONFIG(r7, #EUR_CR_ISP_CONTEXT_RESUME2 >> 2, r5, r6);
		}
		RR_WriteMasterResume2:
	
		/* Write CONTEXT_RESUME2 to the master register */
		mov		r4, #EUR_CR_MASTER_ISP_CORE0_CONTEXT_RESUME2 >> 2;
		imae	r4, r7.low, #((EUR_CR_MASTER_ISP_CORE1_CONTEXT_RESUME2 \
								- EUR_CR_MASTER_ISP_CORE0_CONTEXT_RESUME2) >> 2), r4, u32;
		str		r4, r5;
	#endif
		
		// CONTEXT_RESUME3
		MK_MEM_ACCESS(ldad) r5,	[r2, #1++], drc0;
		wdf		drc0;
		WRITECORECONFIG(r7, #EUR_CR_ISP_CONTEXT_RESUME3 >> 2, r5, r6);
		
	#if defined(SGX_FEATURE_MP)
		// CONTEXT_RESUME4-9
		mov		r6, #(((EUR_CR_MASTER_ISP_CORE0_CONTEXT_RESUME9 - EUR_CR_MASTER_ISP_CORE0_CONTEXT_RESUME4) >> 2) + 1);
		RR_LoadNextReg:
		{
			MK_MEM_ACCESS(ldad) r5,	[r2, #1++], drc0;
			/* Point to the next config */
			REG_INCREMENT(r4, p2);
			wdf		drc0;
			str		r4, r5;
			isub16.testnz	r6, p2, r6, #1;
			p2	ba	RR_LoadNextReg;
		}

		/* Are there any more cores to save */
		REG_INCREMENT(r7, p2);
		xor.testnz	p2, r7, r8;
		p2	ba	RR_LoadNextCore;
	#endif
	}
	
	/* Indicate that this is a resumed render */
	str		#EUR_CR_ISP_RENDER >> 2, #EUR_CR_ISP_RENDER_TYPE_NORMAL3D_RESUME;
	
	lapc;
}
#else /* SGX_FEATURE_ISP_CONTEXT_SWITCH_REV_3 */
	#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH_REV_2)
/*****************************************************************************
 RenderHalt
	This routine handles the storing of HW state during an ISP context switch
	away from the current render.

 inputs:	none
 temps:		r0, r1, r2, r3, r4, r5
*****************************************************************************/
RenderHalt:
{
	MK_TRACE(MKTC_RENDERHALT, MKTF_3D | MKTF_SCH | MKTF_CSW);
	
	/* This is a context switch so clear the flags */
	MK_LOAD_STATE(r0, ui32IRenderFlags, drc0);
	MK_WAIT_FOR_STATE_LOAD(drc0);
		#if defined(SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH)
	and.testnz	p2, r0, #RENDER_IFLAGS_TRANSFERRENDER;
	and		r0, r0, ~#RENDER_IFLAGS_TRANSFERRENDER;
		#endif
	and		r0, r0, #~(RENDER_IFLAGS_RENDERINPROGRESS | RENDER_IFLAGS_SPMRENDER | \
						RENDER_IFLAGS_HALTRENDER | RENDER_IFLAGS_HALTRECEIVED | \
						RENDER_IFLAGS_FORCEHALTRENDER);
	MK_STORE_STATE(ui32IRenderFlags, r0);

	MK_TRACE(MKTC_RH_CLEARFLAGS, MKTF_3D | MKTF_SCH | MKTF_CSW);

	/*
		We are interrupting this render so it hasn't actually completed yet - do the
		stuff required so we can resume it later
	*/
	ldr		r0, #EUR_CR_ISP_STATUS2 >> 2, drc0;
	ldr		r2, #EUR_CR_ISP_STATUS3 >> 2, drc1;
	ldr		r1, #EUR_CR_ISP_STATUS1 >> 2, drc1;
	wdf 	drc1;
	/* combine status 1 and 3 */	
	shl		r1, r1, #(EUR_CR_ISP_START_CTRL_STREAM_POS_SHIFT - EUR_CR_ISP_STATUS1_CTRL_STREAM_POS_SHIFT);
	or		r1, r2, r1;
	wdf		drc0;
	
	MK_TRACE(MKTC_RH_RGN_ADDR, MKTF_3D | MKTF_SCH | MKTF_CSW);
	MK_TRACE_REG(r0, MKTF_3D | MKTF_SCH | MKTF_CSW);
	MK_TRACE(MKTC_RH_CTRL_ADDR, MKTF_3D | MKTF_SCH | MKTF_CSW);
	MK_TRACE_REG(r1, MKTF_3D | MKTF_SCH | MKTF_CSW);
	
	/* This is a context switch so save the values for later */
	ldr		r2, #EUR_CR_ISP2_CONTEXT_SWITCH2 >> 2, drc0;
	ldr		r3, #EUR_CR_ISP2_CONTEXT_SWITCH3 >> 2, drc0;
			
	/* load the PDS status register */
	ldr		r5, #EUR_CR_PDS >> 2, drc0;
	wdf		drc0;
	
	/* check if PT was in flight for each pipe */
	mov		r4, #EUR_CR_PDS_1_STATUS_PTOFF_STORED_MASK;
	and.testnz	p0, r5, #EUR_CR_PDS_0_STATUS_PTOFF_STORED_MASK;
	and.testnz	p1, r5, r4;
	/* Start loading the holding structure base address */
	MK_LOAD_STATE(r4, s3DContext, drc1);

	/* Set or clear the bits */
	p0		or	r2, r2, #EUR_CR_ISP2_CONTEXT_RESUME2_PT_IN_FLIGHT0_MASK;
	!p0		and r2, r2, ~#EUR_CR_ISP2_CONTEXT_RESUME2_PT_IN_FLIGHT0_MASK;
	p1		or	r2, r2, #EUR_CR_ISP2_CONTEXT_RESUME2_PT_IN_FLIGHT1_MASK;
	!p1		and r2, r2, ~#EUR_CR_ISP2_CONTEXT_RESUME2_PT_IN_FLIGHT1_MASK;
	/* make sure the holding structure base address has loaded */
	wdf		drc1;
	// ISP_RGN_BASE, ISP_START, ISP2_CONTEXT_SWITCH2, ISP2_CONTEXT_SWITCH3
		#if defined(SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH)
	p2	MK_MEM_ACCESS(stad.f4)	[r4, #DOFFSET(SGXMKIF_HWTRANSFERCONTEXT.ui32NextRgnBase)-1], r0;
	p2	ba	RH_ISPStateSaved;
		#endif
	{
		MK_MEM_ACCESS(stad.f4)	[r4, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32NextRgnBase)-1], r0;
	}
	RH_ISPStateSaved:
	
		#if defined(SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH)
	p2	ba	RH_PDSStateSaved;
		#endif
	{	
		/* Modify the PDS state resume data */
		ldr		r1, #EUR_CR_PDS0_CONTEXT_STORE >> 2, drc0;
		ldr		r2, #EUR_CR_PDS1_CONTEXT_STORE >> 2, drc0;
		mov		r3, #EUR_CR_PDS_1_STATUS_CSWITCH_COMPLETED_MASK;
		and.testnz	p0, r5, #EUR_CR_PDS_0_STATUS_CSWITCH_COMPLETED_MASK;
		and.testnz	p1, r5, r3;
		wdf		drc0;
		p0		or	r1, r1, #EUR_CR_PDS0_CONTEXT_RESUME_VALID_STREAM_MASK;
		!p0		and r1, r1, ~#EUR_CR_PDS0_CONTEXT_RESUME_VALID_STREAM_MASK;
		p1		or	r2, r2, #EUR_CR_PDS1_CONTEXT_RESUME_VALID_STREAM_MASK;
		!p1		and r2, r2, ~#EUR_CR_PDS1_CONTEXT_RESUME_VALID_STREAM_MASK;
		
		MK_MEM_ACCESS(stad.f2)	[r4, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.asPDSStateBaseDevAddr[0][0])-1], r1; 
	}
	RH_PDSStateSaved:

		#if defined(SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH)
	p2	ba	RH_SetTransferRenderFlags;
	{
		#endif
		MK_MEM_ACCESS_BPCACHE(ldad)	r0, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRTData)], drc1;
		wdf		drc1;
		/* Make sure the RT status flags are loaded */
		MK_MEM_ACCESS(ldad)	r1, [r0, #DOFFSET(SGXMKIF_HWRTDATA.sRTStatusDevAddr)], drc1;
		wdf		drc1;
		#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
		MK_MEM_ACCESS_BPCACHE(ldad)	r2, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.ui32ActiveRTIdx)], drc0;
		wdf		drc0;
		imae	r1, r2.low, #SIZEOF(IMG_UINT32), r1, u32;
		#endif
		MK_MEM_ACCESS(ldad)	r2, [r1], drc0;
		wdf		drc0;
		/* Record that we have context switched on this scene for later use */
		or		r2, r2, #SGXMKIF_HWRTDATA_RT_STATUS_ISPCTXSWITCH;
		MK_MEM_ACCESS(stad)	[r1], r2;
		#if defined(SGX_FEATURE_FAST_RENDER_CONTEXT_SWITCH)
		ba	RH_FlagsSet;
	}
	RH_SetTransferRenderFlags:
	{
		MK_LOAD_STATE(r0, sTransferCCBCmd, drc1);
		MK_WAIT_FOR_STATE_LOAD(drc1);
		MK_MEM_ACCESS_BPCACHE(ldad)	r1, [r0, #DOFFSET(SGXMKIF_TRANSFERCMD.ui32Flags)], drc0;
		wdf		drc0;
		or	r1, r1, #SGXMKIF_TQFLAGS_CTXSWITCH;
		MK_MEM_ACCESS_BPCACHE(stad)	[r0, #DOFFSET(SGXMKIF_TRANSFERCMD.ui32Flags)], r1;
	}
	RH_FlagsSet:
		#endif
	idf		drc0, st;
	wdf		drc0;
	
	/* Check the queues */
	FIND3D();
	FINDTA();

	MK_TRACE(MKTC_RENDERHALT_END, MKTF_3D | MKTF_SCH | MKTF_CSW);
	
	/* branch back to calling code */
	mov		pclink, R_EDM_PCLink;

	lapc;
}

/*****************************************************************************
 ResumeRender
	This routine handles the retoring of HW state when resuming a render
	previously context switched.

 inputs:	r0 - SGXMKIF_HWRENDERCONTEXT of the render to be resumed
 temps:		r2, r3, r4, r5, r6
*****************************************************************************/
ResumeRender:
{
	MK_TRACE(MKTC_KICKRENDER_RESUME, MKTF_3D | MKTF_SCH);

	/* Now setup the PDS state resume addresses and zls resume addr  */
	MK_MEM_ACCESS(ldad.f3)	r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.asPDSStateBaseDevAddr[0][0])-1], drc0;
	wdf		drc0;
	str		#EUR_CR_PDS0_CONTEXT_RESUME >> 2, r2;
	str		#EUR_CR_PDS1_CONTEXT_RESUME >> 2, r3;
	
	/* Clear the VALID/DISABLE bit ready for the next store */
	and		r2, r2, ~#EUR_CR_PDS0_CONTEXT_STORE_DISABLE_MASK;
	#if defined(EUR_CR_PDS1_CONTEXT_STORE_DISABLE_MASK)
	and		r3, r3, ~#EUR_CR_PDS1_CONTEXT_STORE_DISABLE_MASK;
	#endif
	MK_MEM_ACCESS(stad.f2)	[r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.asPDSStateBaseDevAddr[0][0])-1], r2;
	
	str		#EUR_CR_ISP2_CONTEXT_RESUME >> 2, r4;
	
	MK_MEM_ACCESS(ldad.f4) r2, [r0, #DOFFSET(SGXMKIF_HWRENDERCONTEXT.ui32NextRgnBase)-1], drc0;
	wdf		drc0;
	str		#EUR_CR_ISP_RGN_BASE >> 2, r2;
	str		#EUR_CR_ISP_START >> 2, r3;
	str		#EUR_CR_ISP2_CONTEXT_RESUME2 >> 2, r4;
	str		#EUR_CR_ISP2_CONTEXT_RESUME3 >> 2, r5;

	/* Indicate that this is a resumed render */
	str		#EUR_CR_ISP_RENDER >> 2, #EUR_CR_ISP_RENDER_TYPE_NORMAL3D_RESUME;
	
	lapc;
}
	#else /* SGX_FEATURE_ISP_CONTEXT_SWITCH_REV_2 */
		#if !defined(FIX_HW_BRN_23761) && !defined(SGX_FEATURE_MP) && \
			!defined(SGX_FEATURE_ISP_BREAKPOINT_RESUME_REV_2) && !defined(SGX_FEATURE_VISTEST_IN_MEMORY)
/*****************************************************************************
 RenderHalt routine	- process the render interrupted event

 inputs:	r7 - Occlusion entry value
 temps: 	r0,r1,r2,r3,r4,r5,r6,r8,r9
*****************************************************************************/
.export RenderHalt;
RenderHalt:
{
	MK_TRACE(MKTC_RENDERHALT, MKTF_3D | MKTF_SCH | MKTF_CSW);
	/* 
		Must have received both the RenderHalt and the PixelBE to be here
	*/	
	
	SWITCHEDMTO3D();

	/*
		We were only interrupting this render so it hasn't actually completed yet - do the
		stuff required so we can resume it later and kick off the previously chosen one
	*/
	MK_MEM_ACCESS_BPCACHE(ldad)	R_RTData, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRTData)], drc0;
	ldr		r0, #EUR_CR_ISP_STATUS1 >> 2, drc0;
	ldr		r1, #EUR_CR_ISP_STATUS2 >> 2, drc1;
	wdf		drc0;
	wdf 		drc1;

	/* Add EUR_CR_BIF_3D_REQ_BASE value to control stream offset in ISP status 1 register */
	ldr		r2, #EUR_CR_BIF_3D_REQ_BASE >> 2, drc0;
	wdf		drc0;

	/* No way of doing a full 32-bit add :( */
	shr		r2, r2, #16;
	mov		r3, #0xFFFF;
	imae		r0, r2.low, r3.low, r0, u32;
	iaddu32	r0, r2.low, r0;
	
	MK_TRACE(MKTC_RH_CTRL_ADDR, MKTF_3D | MKTF_SCH | MKTF_CSW);
	MK_TRACE_REG(r0, MKTF_3D | MKTF_SCH | MKTF_CSW);
	
	/* Convert tile x y into tile number */
	MK_MEM_ACCESS_BPCACHE(ldad)	r8, [R_TA3DCtl, #DOFFSET(SGXMK_TA3D_CTL.s3DRenderDetails)], drc0;
	and		r9, r1, #EUR_CR_ISP_STATUS2_TILE_X_MASK;
	wdf		drc0;
	MK_MEM_ACCESS(ldad)	r3, [r8, #DOFFSET(SGXMKIF_HWRENDERDETAILS.sHWRTDataSetDevAddr)], drc0;
	shr		r9, r9, #EUR_CR_ISP_STATUS2_TILE_X_SHIFT;
	wdf		drc0;
	MK_MEM_ACCESS(ldad)	r8, [r3, #DOFFSET(SGXMKIF_HWRTDATASET.ui32MTileWidth)], drc0;
	and		r5, r1, #EUR_CR_ISP_STATUS2_TILE_Y_MASK;
	shr		r5, r5, #EUR_CR_ISP_STATUS2_TILE_Y_SHIFT;
	wdf		drc0;
	imae		r3, r8.low, r5.low, r9, u32;
	
	/* work out the address of the rgn header */
	MK_MEM_ACCESS(ldad)	r2, [R_RTData, #DOFFSET(SGXMKIF_HWRTDATA.sTileRgnLUTDevAddr)], drc0;

	/* Set data size in top WORD of offset, ld with non-immediate uses src1.high * src1.low and doesn't multiply by data size */
	or		r3, r3, #0x00040000;
	wdf		drc0;
	/* Read new region base */
	MK_MEM_ACCESS_CACHED(ldad)	r8, [r2, r3], drc0;
	wdf		drc0;
	
	MK_TRACE(MKTC_RH_RGN_ADDR, MKTF_3D | MKTF_SCH | MKTF_CSW);
	MK_TRACE_REG(r8, MKTF_3D | MKTF_SCH | MKTF_CSW);

	/* Check region header for empty tile bit */
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r8], drc0;
	mov		r3, #EURASIA_REGIONHEADER0_EMPTY;
	wdf		drc0;
	and.testz	p0, r2, r3;
	p0	ba		RH_NotEmptyTile;
	{
		MK_TRACE(MKTC_RH_EMPTY_TILE, MKTF_3D | MKTF_SCH | MKTF_CSW);
		RH_CheckLastTile:
		/* Check region header for last tile bit */
		MK_MEM_ACCESS_CACHED(ldad)	r2, [r8], drc0;
		mov		r3, #EURASIA_REGIONHEADER0_LASTREGION;
		wdf		drc0;
		and.testz	p0, r2, r3;
		p0	ba		RH_NotLastTile;
		{
			MK_TRACE(MKTC_RH_EMPTY_LAST_TILE, MKTF_3D | MKTF_SCH | MKTF_CSW);

			/* Only Handling breakpoints */
			mov		r5, r0;
			ba		RH_InvalidateObjects;	
		}
		RH_NotLastTile:
	
		mov	r5, r0;
		ba	RH_InvalidateObjects;
	}
	RH_NotEmptyTile:
	
	MK_TRACE(MKTC_RH_NOT_EMPTY, MKTF_3D | MKTF_SCH | MKTF_CSW);

	/* Check if all primitives in the current object have been processed and if so just point to the next one */
	MK_MEM_ACCESS_CACHED(ldad)	r2, [r0], drc0;
	MK_MEM_ACCESS_CACHED(ldad)	r3, [r0, #1], drc0;
	and		r4, r1, #EUR_CR_ISP_STATUS2_PRIM_NUM_MASK;
	shr		r4, r4, #EUR_CR_ISP_STATUS2_PRIM_NUM_SHIFT;
	wdf		drc0;
	and		r5, r3, ~#EURASIA_PARAM_PB1_PRIMCOUNT_CLRMSK;
	shr		r5, r5, #EURASIA_PARAM_PB1_PRIMCOUNT_SHIFT;
	xor.testnz	p0, r4, r5;

	p0	ba		RH_ObjectIncomplete;
	{
		MK_TRACE(MKTC_RH_OBJECT_COMPLETE, MKTF_3D | MKTF_SCH | MKTF_CSW);
		
		/* Calculate the address of the next object */
		mov		r5, #EURASIA_PARAM_PB0_PRIMMASKPRES;
		and.testnz	p0, r2, r5;
		!p0	mov		r5, #8;
		p0	mov		r5, #12;
		iaddu32	r0, r5.low, r0;
	
		/* Check what the next object type in the control stream is */
		MK_MEM_ACCESS_CACHED(ldad)	r2, [r0], drc0;
		wdf		drc0;
		and		r4, r2, ~#EURASIA_PARAM_OBJTYPE_CLRMSK;
		xor.testz	p0, r4, #EURASIA_PARAM_OBJTYPE_STREAMLINK;
		p0	ba		RH_StreamLink;
		{
			mov		r5, #EURASIA_PARAM_OBJTYPE_PRIMBLOCK;
			xor.testz	p0, r4, r5;
			p0	ba		RH_PrimBlock;
			/* Must have been a terminate */
			ba	RH_CheckLastTile;
		}
		RH_StreamLink:
		
		MK_TRACE(MKTC_RH_STREAM_LINK, MKTF_3D | MKTF_SCH | MKTF_CSW);
	
		/* Follow stream link to next block */
		ldr		r3, #EUR_CR_BIF_3D_REQ_BASE >> 2, drc0;
		and		r0, r2, ~#EURASIA_PARAM_STREAMLINK_CLRMSK;
		wdf		drc0;

		/* No way of doing a full 32-bit add :( */
		shr		r3, r3, #16;
		mov		r2, #0xFFFF;
		imae	r0, r3.low, r2.low, r0, u32;
		iaddu32	r0, r3.low, r0;
	
		MK_MEM_ACCESS_CACHED(ldad)	r2, [r0], drc0;
		wdf		drc0;
		/* Drop through so ISP state load bit is set */
	
		RH_PrimBlock:
	
		/* Make sure ISP state is loaded */
		or		r2, r2, #EURASIA_PARAM_PB0_READISPSTATE;
		MK_MEM_ACCESS_BPCACHE(stad)	[r0], r2;
		idf		drc0, st;
		wdf		drc0;
		mov		r5, r0;
	}
	RH_ObjectIncomplete:
	
	RH_InvalidateObjects:

	/* Set preceding DWORDs in the same 64 byte block to dummy value so the HW just skips them */
	and.testz	p0, r0, #0x3F;
	p0	ba		RH_ObjectsInvalidated;
	{
		MK_TRACE(MKTC_RH_INVALIDATE_OBJECTS, MKTF_3D | MKTF_SCH | MKTF_CSW);
		
		and		r2, r0, #~0x3F;
		mov		r3, #4;
		mov		r4, #EURASIA_PARAM_OBJTYPE_DUMMY;

		RH_MoreObjects:
		{
			MK_MEM_ACCESS_BPCACHE(stad)	[r2], r4;
			iaddu32	r2, r3.low, r2;
			xor.testnz	p0, r2, r0;
			p0	ba		RH_MoreObjects;
		}
	}
	RH_ObjectsInvalidated:
	
	MK_TRACE(MKTC_RH_OBJECTS_INVALIDATED, MKTF_3D | MKTF_SCH | MKTF_CSW);

	/* write back the new region base, as it is needed on resume */
	str		#EUR_CR_ISP_RGN_BASE >> 2, r8;

	/* Work out the new control stream offset */
	ldr		r0, #EUR_CR_BIF_3D_REQ_BASE >> 2, drc0;
	wdf		drc0;
	xor		r0, r0, #-1;
	mov		r1, #1;
	iadd32	r0, r1.low, r0;
	
	/* No way of doing a full 32-subtract add :( */
	and		r6, r5, #0xFFFF;
	imae	r0, r6.low, r1.low, r0, u32;
	shr		r5, r5, #16;
	mov		r1, #0xFFFF;
	imae		r0, r5.low, r1.low, r0, u32;
	iaddu32	r0, r5.low, r0;
	
	and		r0, r0, #~EURASIA_REGIONHEADER1_CONTROLBASE_CLRMSK;
	/* Update control stream pointer for this region */
	MK_MEM_ACCESS_BPCACHE(stad)	[r8, #1], r0;
	idf		drc0, st;
	wdf		drc0;

	SWITCHEDMBACK();

	RH_EmptyTileTerminate:
	/* only handling breakpoints */
	lapc;
}
		#else
		/* compiler throws an error if file is empty */
	nop;
		#endif
	#endif /* SGX_FEATURE_ISP_CONTEXT_SWITCH_REV_2 */
#endif /* SGX_FEATURE_ISP_CONTEXT_SWITCH_REV_3 */

/*****************************************************************************
 End of file (3dcontextswitch.asm)
*****************************************************************************/
