/*****************************************************************************
 Name		: sgxinit_secondary.pds.asm

 Title		: PDS Assembly Code

 Author 	: Imagination Technologies

 Created  	: 23/05/2008

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

 Description 	: PDS program for SGX microkernel secondary initialisation

 Program Type	: PDS assembly language

 Version 	: $Revision: 1.8 $

 Modifications	:

 $Log: sgxinit_secondary.pds.asm $
 ******************************************************************************/

#include "usedefs.h"

/*!****************************************************************************
* Register declarations
******************************************************************************/
/* Constants */
data dword INPUT_DOUTD0;
data dword INPUT_DOUTD1;
data dword INPUT_DOUTU0;
data dword INPUT_DOUTU1;
data dword INPUT_DOUTU2;

#if defined(FIX_HW_BRN_25339) || defined(FIX_HW_BRN_27330)
temp dword temp_ds1 = ds1[48];
#endif

/*!****************************************************************************
* Set up the secondary attributes
******************************************************************************/

tstz	p0,	ir0;
p0		bra	sgxinit_secondary_DMA;

/* ir0 is non-zero - skip the DMA */
bra		sgxinit_secondary_noDMA;

sgxinit_secondary_DMA:
/* DMA the secondary attributes. */
movs		doutd,	INPUT_DOUTD0, INPUT_DOUTD1;

sgxinit_secondary_noDMA:

/* Emit the secondary USSE program */
#if defined(FIX_HW_BRN_25339) || defined(FIX_HW_BRN_27330)
mov32	temp_ds1, INPUT_DOUTU2;
movs	doutu,		INPUT_DOUTU0,	INPUT_DOUTU1,	temp_ds1;
#else
movs	doutu,		INPUT_DOUTU0,	INPUT_DOUTU1,	INPUT_DOUTU2;
#endif

halt;

/******************************************************************************
 End of file (sgxinit_secondary.pds.asm)
******************************************************************************/

