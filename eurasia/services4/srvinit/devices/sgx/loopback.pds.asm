/*****************************************************************************
 Name		: loopback.pds.asm

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

 Description 	: USE program for handling all 3D events

 Program Type	: PDS assembly language

 Version 	: $Revision: 1.9 $

 Modifications	:

 $Log: loopback.pds.asm $
 
  --- Revision Logs Removed --- 
  
 ******************************************************************************/

#include <sgxdefs.h>
#include <sgxapi.h>
#include <sgxinfo.h>

#include "usedefs.h"

/*!****************************************************************************
* Register declarations
******************************************************************************/
/* Constants */
data dword INPUT_IR0_PA_DEST;
data dword INPUT_3DEVENTS;
data dword INPUT_TAEVENTS;
#if defined(SGX_FEATURE_2D_HARDWARE)
data dword INPUT_2DEVENTS;
data dword INPUT_2D_DOUTU0;
#endif
data dword INPUT_3D_DOUTU0;
data dword INPUT_TA_DOUTU0;
data dword INPUT_SPM_DOUTU0;
data dword INPUT_DOUTU1;
data dword INPUT_DOUTU2;
data dword INPUT_TIMESTAMP_PA_DEST;

#if !defined(FIX_HW_BRN_25339) && !defined(FIX_HW_BRN_27330)

/* Temp temps */
temp dword tmp0;

/*!****************************************************************************
* Setup the primary attributes
******************************************************************************/

/* Emit ir0 */
movs	douta,	ir0,	INPUT_IR0_PA_DEST;

#if defined(SUPPORT_SGX_HWPERF)
/* Save the time stamp */
movs	douta,	tim, INPUT_TIMESTAMP_PA_DEST;
#endif

/*!****************************************************************************
* Check if it is a loopback for the 3D Module
******************************************************************************/
/* extract the 3D Bits from IR0 */
and		tmp0,	ir0,	INPUT_3DEVENTS;
/* Test whether any of the bits are set */
tstz	p0,	tmp0;
p0	bra	Not3DModule;
	/* if bits are set Issue USE task and exit */
	movs		doutu,		INPUT_3D_DOUTU0,	INPUT_DOUTU1,	INPUT_DOUTU2;
halt;
Not3DModule:
/*!****************************************************************************
* Check if it is a loopback for the TA Module
******************************************************************************/
/* extract the TA Bits from IR0 */
and		tmp0,	ir0,	INPUT_TAEVENTS;
/* Test whether any of the bits are set */
tstz	p0,	tmp0;
p0	bra	NotTAModule;
	/* if bits are set Issue USE task and exit */
	movs		doutu,		INPUT_TA_DOUTU0,	INPUT_DOUTU1,	INPUT_DOUTU2;
halt;
NotTAModule:
#if defined(SGX_FEATURE_2D_HARDWARE)
/*!****************************************************************************
* Check if it is a loopback for the 2D Module
******************************************************************************/
/* extract the 2D Bits from IR0 */
and		tmp0,	ir0,	INPUT_2DEVENTS;
/* Test whether any of the bits are set */
tstz	p0,	tmp0;
p0	bra	Not2DModule;
	/* if bits are set Issue USE task and exit */
	movs		doutu,		INPUT_2D_DOUTU0,	INPUT_DOUTU1,	INPUT_DOUTU2;
halt;
Not2DModule:
#endif
/*!****************************************************************************
* Must be a loopback for the SPM module
******************************************************************************/
/* Issue USE task */
movs		doutu,		INPUT_SPM_DOUTU0,	INPUT_DOUTU1,	INPUT_DOUTU2;
/* End of program */
halt;

#else /* !(FIX_HW_BRN_25339) && !(FIX_HW_BRN_27330) */


/* Temp temps */
temp dword tmp0;
temp dword temp_ds1 = ds1[48];


/*!****************************************************************************
* Setup the primary attributes
******************************************************************************/

#if defined(SUPPORT_SGX_HWPERF)
/* Save the time stamp */
mov32	temp_ds1, INPUT_TIMESTAMP_PA_DEST;
movs	douta,	tim, temp_ds1;
#endif
/* Emit ir0 */
mov32	temp_ds1, INPUT_IR0_PA_DEST;
movs	douta,	ir0,	temp_ds1;
#if defined(FIX_HW_BRN_31988)
movs	douta,	ir0,	temp_ds1;
movs	douta,	ir0,	temp_ds1;
#endif

/*!****************************************************************************
* Check if it is a loopback for the 3D Module
******************************************************************************/
/* extract the 3D Bits from IR0 */
mov32	temp_ds1, INPUT_3DEVENTS;
and		tmp0,	ir0,	temp_ds1;
/* Test whether any of the bits are set */
tstz	p0,	tmp0;
p0	bra	Not3DModule;
	/* if bits are set Issue USE task and exit */
	mov32	temp_ds1, INPUT_DOUTU2;
	movs		doutu,		INPUT_3D_DOUTU0,	INPUT_DOUTU1,	temp_ds1;
halt;
Not3DModule:
/*!****************************************************************************
* Check if it is a loopback for the TA Module
******************************************************************************/
/* extract the TA Bits from IR0 */
mov32	temp_ds1, INPUT_TAEVENTS;
and		tmp0,	ir0,	temp_ds1;
/* Test whether any of the bits are set */
tstz	p0,	tmp0;
p0	bra	NotTAModule;
	/* if bits are set Issue USE task and exit */
	mov32	temp_ds1, INPUT_DOUTU2;
	movs		doutu,		INPUT_TA_DOUTU0,	INPUT_DOUTU1,	temp_ds1;
halt;
NotTAModule:

/*!****************************************************************************
* Must be a loopback for the SPM module
******************************************************************************/
/* Issue USE task */
mov32	temp_ds1, INPUT_DOUTU2;
movs		doutu,		INPUT_SPM_DOUTU0,	INPUT_DOUTU1,	temp_ds1;
/* End of program */
halt;

#endif /* !FIX_HW_BRN_25339 && !(FIX_HW_BRN_27330) */

/******************************************************************************
 End of file (loopback.pds.asm)
******************************************************************************/

