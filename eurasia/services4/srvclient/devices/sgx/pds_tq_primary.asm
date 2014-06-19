/*****************************************************************************
 Name		: pds_tq_primary.asm 

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

 Version 	: $Revision: 1.6 $

 Modifications	:

 $Log: pds_tq_primary.asm $
 *****************************************************************************/

#include "sgxdefs.h"

#if !defined(FIX_HW_BRN_25339) && !defined(FIX_HW_BRN_27330)

	data dword DOUTU0;
	data dword DOUTU1;
	data dword DOUTU2;

	#if TQ_PDS_PRIM_SRCS > 0

		/* Texture 0 */
		data dword DOUTI0_SRC0;
		#if (EURASIA_PDS_DOUTI_STATE_SIZE == 2)
			data dword DOUTI1_SRC0;
		#endif
		data dword DOUTT0_SRC0;
		data dword DOUTT1_SRC0;
		data dword DOUTT2_SRC0;
		#if (EURASIA_TAG_TEXTURE_STATE_SIZE == 4)
			data dword DOUTT3_SRC0;
		#endif

		#if TQ_PDS_PRIM_SRCS > 1
			/* Texure 1 */
			data dword DOUTI0_SRC1;
			#if (EURASIA_PDS_DOUTI_STATE_SIZE == 2)
				data dword DOUTI1_SRC1;
			#endif
			data dword DOUTT0_SRC1;
			data dword DOUTT1_SRC1;
			data dword DOUTT2_SRC1;
			#if (EURASIA_TAG_TEXTURE_STATE_SIZE == 4)
				data dword DOUTT3_SRC1;
			#endif

			#if TQ_PDS_PRIM_SRCS > 2
				/* Texure 2 */
				data dword DOUTI0_SRC2;
				#if (EURASIA_PDS_DOUTI_STATE_SIZE == 2)
					data dword DOUTI1_SRC2;
				#endif
				data dword DOUTT0_SRC2;
				data dword DOUTT1_SRC2;
				data dword DOUTT2_SRC2;
				#if (EURASIA_TAG_TEXTURE_STATE_SIZE == 4)
					data dword DOUTT3_SRC2;
				#endif
			#endif
		#endif
	#endif

	movs doutu, DOUTU0, DOUTU1, DOUTU2
	
	#if (EURASIA_TAG_TEXTURE_STATE_SIZE == 3) && (EURASIA_PDS_DOUTI_STATE_SIZE == 1) 

		#if TQ_PDS_PRIM_SRCS > 0
			movs douti, DOUTI0_SRC0
			movs doutt, DOUTT0_SRC0, DOUTT1_SRC0, DOUTT2_SRC0

			#if TQ_PDS_PRIM_SRCS > 1
				movs douti, DOUTI0_SRC1
				movs doutt, DOUTT0_SRC1, DOUTT1_SRC1, DOUTT2_SRC1

				#if TQ_PDS_PRIM_SRCS > 2
					movs douti, DOUTI0_SRC2
					movs doutt, DOUTT0_SRC2, DOUTT1_SRC2, DOUTT2_SRC2
				#endif
			#endif
		#endif

	#else
		#if (EURASIA_TAG_TEXTURE_STATE_SIZE == 4) && (EURASIA_PDS_DOUTI_STATE_SIZE == 1) 
			#if TQ_PDS_PRIM_SRCS > 0
				movs douti, DOUTI0_SRC0
				movs doutt, DOUTT0_SRC0, DOUTT1_SRC0, DOUTT2_SRC0, DOUTT3_SRC0

				#if TQ_PDS_PRIM_SRCS > 1
					movs douti, DOUTI0_SRC1
					movs doutt, DOUTT0_SRC1, DOUTT1_SRC1, DOUTT2_SRC1, DOUTT3_SRC1

					#if TQ_PDS_PRIM_SRCS > 2
						movs douti, DOUTI0_SRC2
						movs doutt, DOUTT0_SRC2, DOUTT1_SRC2, DOUTT2_SRC2, DOUTT3_SRC2
					#endif
				#endif
			#endif
	
		#else

			#if TQ_PDS_PRIM_SRCS > 0
				movs douti, DOUTI0_SRC0, DOUTI1_SRC0
				movs doutt, DOUTT0_SRC0, DOUTT1_SRC0, DOUTT2_SRC0, DOUTT3_SRC0

				#if TQ_PDS_PRIM_SRCS > 1
					movs douti, DOUTI0_SRC1, DOUTI1_SRC1
					movs doutt, DOUTT0_SRC1, DOUTT1_SRC1, DOUTT2_SRC1, DOUTT3_SRC1

					#if TQ_PDS_PRIM_SRCS > 2
						movs douti, DOUTI0_SRC2, DOUTI1_SRC2
						movs doutt, DOUTT0_SRC2, DOUTT1_SRC2, DOUTT2_SRC2, DOUTT3_SRC2
					#endif
				#endif
			#endif
		#endif
	#endif

	halt

#else /* !FIX_HW_BRN_25339 && !FIX_HW_BRN_27330 */

	temp dword temp_ds1=ds1[48]
	temp dword temp2_ds1=ds1[49]
	temp dword temp_ds0=ds0[48]
	temp dword temp2_ds0=ds0[49]
	data dword DOUTU0;
	data dword DOUTU1;
	data dword DOUTU2;

	#if TQ_PDS_PRIM_SRCS > 0

		/* Texture 0 */
		data dword DOUTI0_SRC0;
		#if (EURASIA_PDS_DOUTI_STATE_SIZE == 2)
			data dword DOUTI1_SRC0;
		#endif
		data dword DOUTT0_SRC0;
		data dword DOUTT1_SRC0;
		data dword DOUTT2_SRC0;
		#if (EURASIA_TAG_TEXTURE_STATE_SIZE == 4)
			data dword DOUTT3_SRC0;
		#endif

		#if TQ_PDS_PRIM_SRCS > 1
			/* Texure 1 */
			data dword DOUTI0_SRC1;
			#if (EURASIA_PDS_DOUTI_STATE_SIZE == 2)
				data dword DOUTI1_SRC1;
			#endif
			data dword DOUTT0_SRC1;
			data dword DOUTT1_SRC1;
			data dword DOUTT2_SRC1;
			#if (EURASIA_TAG_TEXTURE_STATE_SIZE == 4)
				data dword DOUTT3_SRC1;
			#endif

			#if TQ_PDS_PRIM_SRCS > 2
				/* Texure 2 */
				data dword DOUTI0_SRC2;
				#if (EURASIA_PDS_DOUTI_STATE_SIZE == 2)
					data dword DOUTI1_SRC2;
				#endif
				data dword DOUTT0_SRC2;
				data dword DOUTT1_SRC2;
				data dword DOUTT2_SRC2;
				#if (EURASIA_TAG_TEXTURE_STATE_SIZE == 4)
					data dword DOUTT3_SRC2;
				#endif
			#endif
		#endif
	#endif

	mov32 temp_ds1, DOUTU2
	movs doutu, DOUTU0, DOUTU1, temp_ds1
	
	#if (EURASIA_TAG_TEXTURE_STATE_SIZE == 3) && (EURASIA_PDS_DOUTI_STATE_SIZE == 1) 

		#if TQ_PDS_PRIM_SRCS > 0
			movs douti, DOUTI0_SRC0
			
			mov32 temp_ds1, DOUTT2_SRC0
			movs doutt, DOUTT0_SRC0, DOUTT1_SRC0, temp_ds1

			#if TQ_PDS_PRIM_SRCS > 1
				movs douti, DOUTI0_SRC1
				
				mov32 temp_ds1, DOUTT2_SRC1
				movs doutt, DOUTT0_SRC1, DOUTT1_SRC1, temp_ds1

				#if TQ_PDS_PRIM_SRCS > 2
					movs douti, DOUTI0_SRC2
					
					mov32 temp_ds1, DOUTT2_SRC2
					movs doutt, DOUTT0_SRC2, DOUTT1_SRC2, temp_ds1
				#endif
			#endif
		#endif

	#else
		#if (EURASIA_TAG_TEXTURE_STATE_SIZE == 4) && (EURASIA_PDS_DOUTI_STATE_SIZE == 1) 
			#if TQ_PDS_PRIM_SRCS > 0
				movs douti, DOUTI0_SRC0
				
				mov32 temp_ds0, DOUTT0_SRC0
				mov32 temp2_ds0, DOUTT1_SRC0
				mov32 temp_ds1, DOUTT2_SRC0
				mov32 temp2_ds1, DOUTT3_SRC0
				movs doutt, temp_ds0, temp2_ds0, temp_ds1, temp2_ds1

				#if TQ_PDS_PRIM_SRCS > 1
					movs douti, DOUTI0_SRC1
					
					mov32 temp_ds0, DOUTT0_SRC1
					mov32 temp2_ds0, DOUTT1_SRC1
					mov32 temp_ds1, DOUTT2_SRC1
					mov32 temp2_ds1, DOUTT3_SRC1
					movs doutt, temp_ds0, temp2_ds0, temp_ds1, temp2_ds1

					#if TQ_PDS_PRIM_SRCS > 2
						movs douti, DOUTI0_SRC2
						
						mov32 temp_ds0, DOUTT0_SRC2
						mov32 temp2_ds0, DOUTT1_SRC2
						mov32 temp_ds1, DOUTT2_SRC2
						mov32 temp2_ds1, DOUTT3_SRC2
						movs doutt, temp_ds0, temp2_ds0, temp_ds1, temp2_ds1
					#endif
				#endif
			#endif
	
		#else

			#if TQ_PDS_PRIM_SRCS > 0
				movs douti, DOUTI0_SRC0, DOUTI1_SRC0
				
				mov32 temp_ds0, DOUTT0_SRC0
				mov32 temp2_ds0, DOUTT1_SRC0
				mov32 temp_ds1, DOUTT2_SRC0
				mov32 temp2_ds1, DOUTT3_SRC0
				movs doutt, temp_ds0, temp2_ds0, temp_ds1, temp2_ds1

				#if TQ_PDS_PRIM_SRCS > 1
					movs douti, DOUTI0_SRC1, DOUTI1_SRC1
					
					mov32 temp_ds0, DOUTT0_SRC1
					mov32 temp2_ds0, DOUTT1_SRC1
					mov32 temp_ds1, DOUTT2_SRC1
					mov32 temp2_ds1, DOUTT3_SRC1
					movs doutt, temp_ds0, temp2_ds0, temp_ds1, temp2_ds1

					#if TQ_PDS_PRIM_SRCS > 2
						movs douti, DOUTI0_SRC2, DOUTI1_SRC2
						
						mov32 temp_ds0, DOUTT0_SRC2
						mov32 temp2_ds0, DOUTT1_SRC2
						mov32 temp_ds1, DOUTT2_SRC2
						mov32 temp2_ds1, DOUTT3_SRC2
						movs doutt, temp_ds0, temp2_ds0, temp_ds1, temp2_ds1
					#endif
				#endif
			#endif
		#endif
	#endif

	halt

#endif /* !FIX_HW_BRN_25339 && !FIX_HW_BRN_27330*/
/******************************************************************************
 End of file (pds_tq_primary.asm)
******************************************************************************/
