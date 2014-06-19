/*****************************************************************************
 Name		: pds_iter_primary.asm

 Title		: PDS program 

 Author 	: PowerVR

 Created  	: Fri Apr 25 11:46:23 BST 2008

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

 Program Type	: PDS assembly language

 Version 	: $Revision: 1.9 $

 Modifications	:

 $Log: pds_iter_primary.asm $

 *****************************************************************************/
#include "sgxdefs.h"

#if !defined(FIX_HW_BRN_25339) && !defined(FIX_HW_BRN_27330)

data dword DOUTU0;
data dword DOUTU1;
data dword DOUTU2;

data dword DOUTI0_SRC0;
#if (EURASIA_PDS_DOUTI_STATE_SIZE == 2) 
data dword DOUTI1_SRC0;
#endif

movs doutu, DOUTU0, DOUTU1, DOUTU2
#if (EURASIA_PDS_DOUTI_STATE_SIZE == 2) 
movs douti, DOUTI0_SRC0, DOUTI1_SRC0
#else
movs douti, DOUTI0_SRC0
#endif
halt

#else /* !FIX_HW_BRN_25339 && !FIX_HW_BRN_27330 */

temp dword temp_ds1=ds1[48]
data dword DOUTU0;
data dword DOUTU1;
data dword DOUTU2;

data dword DOUTI0_SRC0;
#if (EURASIA_PDS_DOUTI_STATE_SIZE == 2)
data dword DOUTI1_SRC0;
#endif


mov32		temp_ds1, DOUTU2
movs doutu, DOUTU0, DOUTU1, temp_ds1


#if (EURASIA_PDS_DOUTI_STATE_SIZE == 1)
movs douti, DOUTI0_SRC0
#else
movs douti, DOUTI0_SRC0, DOUTI1_SRC0
#endif

halt


#endif /* !FIX_HW_BRN_25339 && !FIX_HW_BRN_27330 */


