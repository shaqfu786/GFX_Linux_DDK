/*****************************************************************************
 Name		: pds_vdmcontextstore.asm

 Title		: PDS program 

 Author 	: PowerVR

 Created  	: Fri Apr 25 11:46:23 BST 2008

 Copyright	: 2008-2009 by Imagination Technologies Limited. All rights reserved.
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

 Version 	: $Revision: 1.6 $

 Modifications	:

 $Log: pds_vdmcontextstore.asm $

 *****************************************************************************/
#include "sgxdefs.h"

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)

data dword INPUT_DOUTU0;
data dword INPUT_DOUTU1;
data dword INPUT_DOUTU2;

#if defined(FIX_HW_BRN_25339) || defined(FIX_HW_BRN_27330)
temp dword temp_ds1 = ds1[48];
mov32	temp_ds1, INPUT_DOUTU2;
movs doutu, INPUT_DOUTU0, INPUT_DOUTU1, temp_ds1;
#else
movs doutu, INPUT_DOUTU0, INPUT_DOUTU1, INPUT_DOUTU2;
#endif

halt;
#endif /* SGX_FEATURE_VDM_CONTEXT_SWITCH_REV_2 */
