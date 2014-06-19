/*****************************************************************************
 Name		: slave_eventhandler.pds.asm

 Title		: PDS Assembly Code

 Author 	: Imagination Technologies

 Created  	: 08/11/2010

 Copyright	: 2010 by Imagination Technologies Limited. All rights reserved.
		  No part of this software, either material or conceptual
		  may be copied or distributed, transmitted, transcribed,
		  stored in a retrieval system or translated into any
		  human or computer language in any form by any means,
		  electronic, mechanical, manual or other-wise, or
		  disclosed to third parties without the express written
		  permission of Imagination Technologies Limited, HomePark
		  Industrial Estate, King's Langley, Hertfordshire,
		  WD4 8LZ, U.K.

 Description 	: PDS program for handling SGX initialisation

 Program Type	: PDS assembly language

 Version 	: $Revision: 1.3 $

 Modifications	:

 $Log: slave_eventhandler.pds.asm $
 
 ******************************************************************************/

#include "sgxdefs.h"

/*****************************************************************************
* Register declarations
******************************************************************************/

#if defined(SGX_FEATURE_MP)

/* Constants */
data dword INPUT_IR0_PA_DEST;
data dword INPUT_IR1_PA_DEST;
data dword INPUT_TA3D_CTL_ADDR;
data dword INPUT_TA3D_CTL_PA_DEST;
data dword INPUT_DOUTU0;
data dword INPUT_DOUTU1;
data dword INPUT_DOUTU2;

/* Emit input registers to the PAs */
movs	douta,	ir0,	INPUT_IR0_PA_DEST;
movs	douta,	ir1,	INPUT_IR1_PA_DEST;
movs	douta,	INPUT_TA3D_CTL_ADDR, INPUT_TA3D_CTL_PA_DEST;
#if defined(FIX_HW_BRN_31988)
movs	douta,	INPUT_TA3D_CTL_ADDR, INPUT_TA3D_CTL_PA_DEST;
movs	douta,	INPUT_TA3D_CTL_ADDR, INPUT_TA3D_CTL_PA_DEST;
#endif

#if !defined(FIX_HW_BRN_25339) && !defined(FIX_HW_BRN_27330)

#if defined(FIX_HW_BRN_31474)
temp	dword	tmp0;
and		tmp0, ir0, 	EURASIA_PDS_IR0_EDM_EVENT_SW_EVENT2;
tstz	p0, tmp0;
p0	bra	NotCacheClearRequest;
	/* The ukernel has requested the PDS cache be cleared */
	bra	ClearLine2;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine2:
	bra	ClearLine3;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine3:
	bra	ClearLine4;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine4:
	bra	ClearLine5;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine5:
	bra	ClearLine6;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine6:
	bra	ClearLine7;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine7:
	bra	ClearLine8;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine8:
	/* Jump to the doutu */
	bra	NotCacheClearRequest;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
NotCacheClearRequest:
#endif

/* Issue USE task and exit */
movs		doutu,		INPUT_DOUTU0,	INPUT_DOUTU1,	INPUT_DOUTU2;
halt;

#else /* !(FIX_HW_BRN_25339) && !(FIX_HW_BRN_27330) */

/* Temp temps */
temp dword tmp0 = ds1[48];

/* Issue USE task and exit */
mov32		tmp0, INPUT_DOUTU2;
movs		doutu,		INPUT_DOUTU0,	INPUT_DOUTU1,	tmp0;
halt;

#endif /* !(FIX_HW_BRN_25339) && !(FIX_HW_BRN_27330) */
#else
nop;
#endif

/******************************************************************************
 End of file (sgxinit_primary.pds.asm)
******************************************************************************/
