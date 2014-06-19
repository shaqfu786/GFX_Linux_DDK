/*****************************************************************************
 Name		: tastaterestorepds.asm

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

 Description 	: USE program for handling the saving of TA state

 Program Type	: PDS assembly language

 Version 	: $Revision: 1.10 $

 Modifications	:

 $Log: tastaterestorepds.asm $
 
 ******************************************************************************/
#include "sgxdefs.h"

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
/*!****************************************************************************
* Register declarations
******************************************************************************/

/* Constants */
data dword INPUT_TA3DCTRL_ADDR;
data dword INPUT_TA3DCTRL_PA_DEST;
data dword INPUT_DOUTU0;
data dword INPUT_DOUTU1;
data dword INPUT_DOUTU2;
#if defined(FIX_HW_BRN_25339) || defined(FIX_HW_BRN_27330)
temp dword temp_ds1 = ds1[48];
#endif

/*!****************************************************************************
* Setup the primary attributes
******************************************************************************/

/* Emit the TA/3D Ctrl Address */
movs	douta,	INPUT_TA3DCTRL_ADDR,		INPUT_TA3DCTRL_PA_DEST;
#if defined(FIX_HW_BRN_31988)
movs	douta,	INPUT_TA3DCTRL_ADDR,		INPUT_TA3DCTRL_PA_DEST;
movs	douta,	INPUT_TA3DCTRL_ADDR,		INPUT_TA3DCTRL_PA_DEST;
#endif

/*!****************************************************************************
* Issue the task to the USSE
******************************************************************************/
/* Issue USE task and exit */
#if defined(FIX_HW_BRN_25339) || defined(FIX_HW_BRN_27330)
mov32		temp_ds1,	INPUT_DOUTU2;
movs		doutu,		INPUT_DOUTU0,	INPUT_DOUTU1,	temp_ds1;
#else
movs		doutu,		INPUT_DOUTU0,	INPUT_DOUTU1,	INPUT_DOUTU2;
#endif

/* End of program */
halt;
#endif
/******************************************************************************
 End of file (tastaterestorepds.asm)
******************************************************************************/

