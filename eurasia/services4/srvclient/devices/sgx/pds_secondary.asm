/*****************************************************************************
 Name		: pds_secondary.asm

 Title		: PDS program 

 Author 	: PowerVR

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

 $Log: pds_secondary.asm $

 *****************************************************************************/
#include "sgxdefs.h"

#if defined(FIX_HW_BRN_25339) || defined(FIX_HW_BRN_27330)
temp dword temp_ds1 = ds1[48];
#endif

/* kicking a program on the USE ensures that the attribute has been written for the shader*/
#if defined(SGX_FEATURE_SECONDARY_REQUIRES_USE_KICK) || (TQ_PDS_SEC_ATTR > 0) || TQ_PDS_SEC_DMA
data dword DOUTU0;
data dword DOUTU1;
data dword DOUTU2;
#endif

#if TQ_PDS_SEC_ATTR > 0
data dword DOUTA0;
#if TQ_PDS_SEC_ATTR > 1
data dword DOUTA1;
#if TQ_PDS_SEC_ATTR > 2
data dword DOUTA2;
#if TQ_PDS_SEC_ATTR > 3
data dword DOUTA3;
#if TQ_PDS_SEC_ATTR > 4
data dword DOUTA4;
#if TQ_PDS_SEC_ATTR > 5
data dword DOUTA5;
#if TQ_PDS_SEC_ATTR > 6
data dword DOUTA6;
#if TQ_PDS_SEC_ATTR > 7
#error TQ PDS secondary error (requested compilation of more direct attributes than supported).
#endif /* 7*/
#endif /* 6*/
#endif /* 5*/
#endif /* 4*/
#endif /* 3*/
#endif /* 2*/
#endif /* 1*/
#endif /* 0*/

#if defined(TQ_PDS_SEC_DMA)
data dword DOUTD0;
data dword DOUTD1;
#endif

#if TQ_PDS_SEC_ATTR > 0
	movs douta, DOUTA0, 0
	#if defined(FIX_HW_BRN_31988)
		movs douta, DOUTA0, 0
		movs douta, DOUTA0, 0
	#endif
	#if TQ_PDS_SEC_ATTR > 1
		movs douta, DOUTA1, 1 << EURASIA_PDS_DOUTA1_AO_SHIFT
		#if defined(FIX_HW_BRN_31988)
			movs douta, DOUTA1, 1 << EURASIA_PDS_DOUTA1_AO_SHIFT
			movs douta, DOUTA1, 1 << EURASIA_PDS_DOUTA1_AO_SHIFT
		#endif
		#if TQ_PDS_SEC_ATTR > 2
			movs douta, DOUTA2, 2 << EURASIA_PDS_DOUTA1_AO_SHIFT
			#if defined(FIX_HW_BRN_31988)
				movs douta, DOUTA2, 2 << EURASIA_PDS_DOUTA1_AO_SHIFT
				movs douta, DOUTA2, 2 << EURASIA_PDS_DOUTA1_AO_SHIFT
			#endif
			#if TQ_PDS_SEC_ATTR > 3
				movs douta, DOUTA3, 3 << EURASIA_PDS_DOUTA1_AO_SHIFT
				#if defined(FIX_HW_BRN_31988)
					movs douta, DOUTA3, 3 << EURASIA_PDS_DOUTA1_AO_SHIFT
					movs douta, DOUTA3, 3 << EURASIA_PDS_DOUTA1_AO_SHIFT
				#endif
				#if TQ_PDS_SEC_ATTR > 4
					movs douta, DOUTA4, 4 << EURASIA_PDS_DOUTA1_AO_SHIFT
					#if defined(FIX_HW_BRN_31988)
						movs douta, DOUTA4, 4 << EURASIA_PDS_DOUTA1_AO_SHIFT
						movs douta, DOUTA4, 4 << EURASIA_PDS_DOUTA1_AO_SHIFT
					#endif
					#if TQ_PDS_SEC_ATTR > 5
						movs douta, DOUTA5, 5 << EURASIA_PDS_DOUTA1_AO_SHIFT
						#if defined(FIX_HW_BRN_31988)
							movs douta, DOUTA5, 5 << EURASIA_PDS_DOUTA1_AO_SHIFT
							movs douta, DOUTA5, 5 << EURASIA_PDS_DOUTA1_AO_SHIFT
						#endif
						#if TQ_PDS_SEC_ATTR > 6
							movs douta, DOUTA6, 6 << EURASIA_PDS_DOUTA1_AO_SHIFT
							#if defined(FIX_HW_BRN_31988)
								movs douta, DOUTA6, 6 << EURASIA_PDS_DOUTA1_AO_SHIFT
								movs douta, DOUTA6, 6 << EURASIA_PDS_DOUTA1_AO_SHIFT
							#endif
						#endif /* 6*/
					#endif /* 5*/
				#endif /* 4*/
			#endif /* 3*/
		#endif /* 2*/
	#endif /* 1*/
#endif /* 0*/

#if defined(TQ_PDS_SEC_DMA)
movs doutd, DOUTD0, DOUTD1 
#endif

#if defined(SGX_FEATURE_SECONDARY_REQUIRES_USE_KICK) || (TQ_PDS_SEC_ATTR > 0) || TQ_PDS_SEC_DMA
#if defined(FIX_HW_BRN_25339) || defined(FIX_HW_BRN_27330)
mov32		temp_ds1, DOUTU2
movs		doutu, DOUTU0, DOUTU1, temp_ds1
#else
movs doutu, DOUTU0, DOUTU1, DOUTU2
#endif
#endif

halt
