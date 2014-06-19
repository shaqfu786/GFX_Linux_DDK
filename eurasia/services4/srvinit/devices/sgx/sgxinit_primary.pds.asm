/*****************************************************************************
 Name		: sgxinit_primary.pds.asm

 Title		: PDS Assembly Code

 Author 	: Imagination Technologies

 Created  	: 23/05/2009

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

 Description 	: PDS program for handling SGX initialisation

 Program Type	: PDS assembly language

 Version 	: $Revision: 1.4 $

 Modifications	:

 $Log: sgxinit_primary.pds.asm $
 
 ******************************************************************************/

#include "sgxdefs.h"

/*****************************************************************************
* Register declarations
******************************************************************************/

/* Constants */
data dword INPUT_DOUTU0;
data dword INPUT_DOUTU1;
data dword INPUT_DOUTU2;


#if !defined(FIX_HW_BRN_25339) && !defined(FIX_HW_BRN_27330)

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


/******************************************************************************
 End of file (sgxinit_primary.pds.asm)
******************************************************************************/
