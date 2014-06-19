/*****************************************************************************
 Name		: tasarestore.pds.asm

 Title		: PDS Assembly Code

 Author 	: Imagination Technologies

 Created  	: 01/01/2008

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

 Description 	: PDS program used to scheduling the USE program respondsible 
 				  for reloading the secondary attributes after a context switch

 Program Type	: PDS assembly language

 Version 	: $Revision: 1.4 $

 Modifications	:

 $Log: tasarestore.pds.asm $
 
 
 ******************************************************************************/
#include "sgxdefs.h"
#include "sgx_mkif.h"

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) && defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
/*!****************************************************************************
* Register declarations
******************************************************************************/

/* Constants */
data dword INPUT_DOUTU0;
data dword INPUT_DOUTU1;
data dword INPUT_DOUTU2;
#if 1 // SGX_SUPPORT_MASTER_ONLY_SWITCHING
data dword INPUT_DOUTD0;
data dword INPUT_DOUTD1;
data dword INPUT_DOUTD1_REMBLOCKS;	/* DOUT1 for last DMA which may be less than full 16x 16 dwords */
data dword INPUT_DMABLOCKS;			/* Number of complete 16x 16 dword SA blocks (>= 0) */
#endif
#if defined(FIX_HW_BRN_25339)
temp dword temp_ds1 = ds1[48];
#endif

#if 1 // SGX_SUPPORT_MASTER_ONLY_SWITCHING
/*
 * Use DMA to load the attributes
 */
	temp dword src_address;
	temp dword loop_count;
	temp dword dest_offset;		/* dword offset into SA buffer */
	temp dword dest_offset_copy;
	temp dword dout1_word;		/* dout1 config word */
	temp dword dma_size;		/* Size of DMA, typically 16 lines of 16 dwords */
	temp dword dma_size_dw;		/* Size of DMA in dwords */


/* Set up DMA source address */
mov32	src_address, INPUT_DOUTD0;
mov32	dout1_word, INPUT_DOUTD1;
mov32	dest_offset, 0;

/* Set up size of DMA (bytes) */
mov32	dma_size, SGX_UKERNEL_SA_BURST_SIZE;
shl		dma_size_dw, dma_size, SGX_31425_DMA_NLINES_SHIFT;
shl		dma_size, dma_size, (SGX_31425_DMA_NLINES_SHIFT+2);

/* Several DMAs may be necessary */
mov32	loop_count, INPUT_DMABLOCKS;
TASARestore_NextDMABlock:
	/* Branch once all the full 16x16 dword blocks are done */
	tstz 	p0, loop_count;
	p0		bra	TASARestore_CheckDMASubBlock;
	/* Start the DMA */
	movs	doutd,		src_address,	dout1_word;

	/* Increment base address */
	add		src_address, src_address, dma_size;
	/* Increment attribute offset */
	add		dest_offset, dest_offset, dma_size_dw;
	shl		dest_offset, dest_offset, EURASIA_PDS_DOUTD1_AO_SHIFT;
	and		dout1_word, dout1_word, EURASIA_PDS_DOUTD1_AO_CLRMSK;
	mov32	dest_offset_copy, dest_offset;
	or		dout1_word, dout1_word, dest_offset_copy;
	
	sub		loop_count, loop_count, 1;
	bra		TASARestore_NextDMABlock;
	/*** End Loop ***/

TASARestore_CheckDMASubBlock:
/* Check if there's any SAs outstanding */
tstz	p0, INPUT_DOUTD1_REMBLOCKS;
p0		bra	TASARestore_NoDMASubBlock;
	/* Use correct attribute buffer offset with new config word */
	mov32	dout1_word, INPUT_DOUTD1_REMBLOCKS;
	and		dout1_word, dout1_word, EURASIA_PDS_DOUTD1_AO_CLRMSK;
	mov32	dest_offset_copy, dest_offset;
	or		dout1_word, dout1_word, dest_offset_copy;

	/* DMA in the remaining attribs */
	movs	doutd,		src_address,	dout1_word;
	/*** End If ***/
	
TASARestore_NoDMASubBlock:

#endif

/*!****************************************************************************
* Issue the task to the USSE
******************************************************************************/
/* Issue USE task and exit */
movs		doutu,		INPUT_DOUTU0,	INPUT_DOUTU1,	INPUT_DOUTU2;

/* End of program */
halt;
#endif
/******************************************************************************
 End of file (tastaterestorepds.asm)
******************************************************************************/

