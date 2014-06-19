/*****************************************************************************
 Name		: subtwiddled_eot.asm

 Title		: USE programs for TQ 

 Author 	: PowerVR

 Copyright	: 2008-2010 by Imagination Technologies Limited. All rights reserved.
		  No part of this software, either material or conceptual
		  may be copied or distributed, transmitted, transcribed,
		  stored in a retrieval system or translated into any
		  human or computer language in any form by any means,
		  electronic, mechanical, manual or other-wise, or
		  disclosed to third parties without the express written
		  permission of Imagination Technologies Limited, HomePark
		  Industrial Estate, King's Langley, Hertfordshire,
		  WD4 8LZ, U.K.

 Description 	: USE End of tile program for sub tile twiddling

 Program Type	: USE assembly language

 Version 	: $Revision: 1.29 $

 Modifications	:

 $Log: subtwiddled_eot.asm $
 #3
  --- Revision Logs Removed --- 

 *****************************************************************************/

#include "sgxdefs.h"

.export SubTwiddled;
.export SubTwiddledEnd;
.export PixelEventSubTwiddled_PBEState_Byte;
#if defined(FIX_HW_BRN_30842)
.export PixelEventSubTwiddled_Sideband_Byte;
#else
.export PixelEventSubTwiddled_Sideband_SinglePipe_Byte;
.export PixelEventSubTwiddled_Sideband_2Pipes_First_Byte;
.export PixelEventSubTwiddled_Sideband_2Pipes_Second_Byte;
#endif
.export	PixelEventSubTwiddled_SubDim;

#define HSH 	#

#if ! defined(FIX_HW_BRN_30842)
/* cross pipe boundary. ( we never need more than 2 pipes, as touching 4 would mean
 * not sub tile size
 */
#define PEST_2PIPE_MODE_MASK	(1 << 31)
/* if set texture goes through pipes in X otherwise in Y ( if bit 32 is also set )*/
#define PEST_DIRECTION_X_MASK	(1 << 30)
#define PEST_ADDR_SHIFT_MASK	(0x0000ffff)
#endif

#if defined(SUPPORT_SGX_COMPLEX_GEOMETRY)
#define WAITFOROTHERPIPE(label)																		\
	/* flush the g1 by ldr*/																		\
	ldr		 		r7, HSH(EUR_CR_USE_G1 >> 2), drc0;												\
	wdf					drc0;																		\
	label##_:;																						\
	{;																								\
		xor.testnz		 p0, g1, g17;																\
		p0 br label##_;																				\
	};
#else /* SUPPORT_SGX_COMPLEX_GEOMETRY*/
#define WAITFOROTHERPIPE(label)																		\
	label##_:;																						\
	{;																								\
		xor.testnz		 p0, g0, g17;																\
		p0 br label##_;																				\
	};
#endif

#if defined(SUPPORT_SGX_COMPLEX_GEOMETRY)
	#if (4 == EURASIA_TAPDSSTATE_PIPECOUNT)
#define FLIPFLOP																					\
	mov					r7, g1;																		\
	iadd32				r7, r7.low, HSH(1);															\
	and					r7, r7, HSH(3);																\
	str		 		HSH(EUR_CR_USE_G1 >> 2), r7;
	#else
#define FLIPFLOP																					\
	xor					r7, g1, HSH(1);																\
	str		 		HSH(EUR_CR_USE_G1 >> 2), r7; 
	#endif
#else
	#if (4 == EURASIA_TAPDSSTATE_PIPECOUNT)
#define FLIPFLOP																					\
	mov					r7, g0;																		\
	iadd32				r7, r7.low, HSH(1);															\
	and					g0, r7, HSH(3);
	#else /* 2 ==  EURASIA_TAPDSSTATE_PIPECOUNT */
#define FLIPFLOP																					\
	xor					g0, g0, HSH(1);
	#endif
#endif /* SUPPORT_SGX_COMPLEX_GEOMETRY*/

/* kicks off the tile and ends the program*/

#if defined(SGX_FEATURE_UNIFIED_STORE_64BITS)

#define DUMMYTILEEMIT(fbaddr)																		\
	mov					r0, HSH(0) ;																\
	mov					r4, fbaddr ;																\
	emitpix.end.freep	r0, r2, r4, HSH(0) ;

#else

#define DUMMYTILEEMIT(fbaddr)																		\
	mov					r0, HSH(0) ;																\
	emitpix2.end.freep	r0, fbaddr, HSH(0) ;

#endif
	

.align 2;
.skipinvon;
SubTwiddled:
{

#if defined(SGX_FEATURE_USE_UNLIMITED_PHASES)
	/* No following phase. */
	phas				#0, #0, pixel, end, perinstance;
#endif

	/* load program state */ 
	PixelEventSubTwiddled_SubDim:
	nop;				/* mov r7, #UV*/
	nop;				/* mov r6, DstBytesPP*/

	and					r4, r7, #0xffff;
	shr					r5, r7, #16;

	PixelEventSubTwiddled_PBEState_Byte:
	nop;					/* mov r0, PBE 0*/
	nop;					/* mov r1, PBE 1 - noadvance must be on*/
	nop;					/* mov r2, PBE 2*/
	nop;					/* mov r3, PBE 3*/

#if defined(SGX_FEATURE_PIXELBE_32K_LINESTRIDE) || \
	defined(SGX_FEATURE_UNIFIED_STORE_64BITS)
	nop;
#endif
#if defined(SGX_FEATURE_UNIFIED_STORE_64BITS)
	nop;
#endif


	/* get tile number*/
	mov					r10, #0;
	xor.testnz			p0, r5, #EURASIA_ISPREGION_SIZEX; 
	p0 xor.testnz		p0, r5, #(EURASIA_ISPREGION_SIZEX / 2);
	p0 br PEST_WidthLessThanHalfTileFirst;
	{
#if defined(FIX_HW_BRN_23615)
		and						r10, sa0, #~EURASIA_PDS_IR0_PDM_TILEX_CLRMSK;
#else
		and						r10, pa0, #~EURASIA_PDS_IR0_PDM_TILEX_CLRMSK;
#endif
		shr						r10, r10, #(1 + EURASIA_PDS_IR0_PDM_TILEX_SHIFT); 
		br PEST_EndOfHalfTileTest;
	}
	PEST_WidthLessThanHalfTileFirst:
	{

#if defined(FIX_HW_BRN_23615)
		and						r10, sa0, #~EURASIA_PDS_IR0_PDM_TILEY_CLRMSK;
#else
		and						r10, pa0, #~EURASIA_PDS_IR0_PDM_TILEY_CLRMSK;
#endif
		shr						r10, r10, #(1 + EURASIA_PDS_IR0_PDM_TILEY_SHIFT);
	}
	PEST_EndOfHalfTileTest:


	/* jump if the pipe doesn't have pixels */
#if defined(FIX_HW_BRN_30842)
	xor.testnz			p0, r5, #16; 
	p0 and.testnz		p0, #1, g17;
	p0 br PEST_EndOfAspectRatios;
#else
	mov					r7, #PEST_DIRECTION_X_MASK;
#if (EURASIA_TAPDSSTATE_PIPECOUNT == 4)
	/* this assuming the follwing pipe order:
	 * 01
	 * 23
	 */
	and.testnz			p0, r6, r7; 
	p0 and.testnz		p0, #2, g17;
	p0 br PEST_EndOfAspectRatios;
#endif
	and.testz			p0, r6, r7; 
	p0 and.testnz		p0, #1, g17;
	p0 br PEST_EndOfAspectRatios;
#endif


	xor.testnz			p0, r5, #EURASIA_ISPREGION_SIZEX; 
	p0 xor.testnz		p0, r5, #(EURASIA_ISPREGION_SIZEX / 2);
	p0 br PEST_WidthLessThanHalfTile;
	{
		xor.testz				p0, r4, #1;
		p0 br PEST_EndOfAspectRatios;
#if EURASIA_ISPREGION_SIZEX == 32 && EURASIA_ISPREGION_SIZEY == 32
#define HALFTILEWIDE_JUMP(i)																		\
	xor.testz				p0, r4, HSH(i);															\
	p0 br PEST_AspectRatio_16_x_##i;
#else
#define HALFTILEWIDE_JUMP(i)																		\
	xor.testz				p0, r4, HSH(i);															\
	p0 br PEST_AspectRatio_8_x_##i;
#endif
		HALFTILEWIDE_JUMP(2);
		HALFTILEWIDE_JUMP(4);
		HALFTILEWIDE_JUMP(8);
		HALFTILEWIDE_JUMP(16);
#if EURASIA_ISPREGION_SIZEY == 32
		xor.testz			p0, r4, #32; 
		p0 br PEST_AspectRatio_16_x_16;
#endif
	}
	PEST_WidthLessThanHalfTile:

#if EURASIA_ISPREGION_SIZEX == 32
	xor.testz				p0, r5, #8;
	!p0 br PEST_WidthNot8;
	{
		xor.testz				p0, r4, #1;
		p0 br PEST_EndOfAspectRatios;
		xor.testz				p0, r4, #2;
		p0 br PEST_AspectRatio_8_x_2;
		xor.testz				p0, r4, #4;
		p0 br PEST_AspectRatio_8_x_4;
		xor.testz				p0, r4, #16;
		p0 br PEST_AspectRatio_8_x_16;
	}
	PEST_WidthNot8:
#endif

	xor.testz				p0, r5, #4;
	!p0 br PEST_WidthNot4;
	{
		xor.testz				p0, r4, #1;
		p0 br PEST_EndOfAspectRatios;
		xor.testz				p0, r4, #2;
		p0 br PEST_AspectRatio_4_x_2;
		xor.testz				p0, r4, #16;
#if EURASIA_ISPREGION_SIZEY == 32
		(!p0) xor.testz				p0, r4, #32;
#endif
		p0 br PEST_AspectRatio_4_x_16;
	}
	PEST_WidthNot4:

	xor.testz				p0, r5, #2;
	!p0 br PEST_WidthNot2;
	{
		xor.testz				p0, r4, #1;
		p0 br PEST_EndOfAspectRatios;
		xor.testz				p0, r4, #8;
		p0 br PEST_AspectRatio_2_x_8;
		xor.testz				p0, r4, #16;
#if EURASIA_ISPREGION_SIZEY == 32
		(!p0) xor.testz				p0, r4, #32;
#endif
		p0 br PEST_AspectRatio_2_x_16;
	}
	PEST_WidthNot2:

	xor.testz				p0, r5, #1;
	p0 br PEST_EndOfAspectRatios;

	lock; lock; /* dbg break*/

	PEST_AspectRatio_2_x_8:
	{
#if (32 == EURASIA_ISPREGION_SIZEX) && (32 == EURASIA_ISPREGION_SIZEY)
		mov				r7, o1;
		mov				o1, o2;
		mov				o2, r7;

		mov				r7, o17;
		mov				o17, o18;
		mov				o18, r7;

		mov				r7, o65;
		mov				o65, o66;
		mov				o66, r7;

		mov				r7, o81;
		mov				o81, o82;
		mov				o82, r7;

#else
		mov				r7, o1;
		mov				o1, o2;
		mov				o2, r7;

		mov				r7, o17;
		mov				o17, o18;
		mov				o18, r7;

		mov				r7, o33;
		mov				o33, o34;
		mov				o34, r7;

		mov				r7, o49;
		mov				o49, o50;
		mov				o50, r7;

#endif
		br PEST_EndOfAspectRatios;
	}
	PEST_AspectRatio_2_x_16:
	{
#if (32 == EURASIA_ISPREGION_SIZEX) && (32 == EURASIA_ISPREGION_SIZEY)
		mov				r7, o1;
		mov				o1, o2;
		mov				o2, r7;

		mov				r7, o17;
		mov				o17, o18;
		mov				o18, r7;

		mov				r7, o65;
		mov				o65, o66;
		mov				o66, r7;

		mov				r7, o81;
		mov				o81, o82;
		mov				o82, r7;

		smbo			#0, #0, #129, #0;
		mov				r7, o80;
		smbo			#129, #0, #129, #0;

		mov				o80, o0;
		mov				o0, o1;
		mov				o1, o80;

		mov				o80, o16;
		mov				o16, o17;
		mov				o17, o80;

		mov				o80, o64;
		mov				o64, o65;
		mov				o65, o80;

		mov				o80, o81;
		smbo			#129, #0, #0, #0;
		mov				o81, r7;

#else
		mov				r7, o1;
		mov				o1, o2;
		mov				o2, r7;

		mov				r7, o17;
		mov				o17, o18;
		mov				o18, r7;

		mov				r7, o33;
		mov				o33, o34;
		mov				o34, r7;

		mov				r7, o49;
		mov				o49, o50;
		mov				o50, r7;

		mov				r7, o65;
		mov				o65, o66;
		mov				o66, r7;

		mov				r7, o81;
		mov				o81, o82;
		mov				o82, r7;

		mov				r7, o97;
		mov				o97, o98;
		mov				o98, r7;

		mov				r7, o113;
		mov				o113, o114;
		mov				o114, r7;

#endif
		br PEST_EndOfAspectRatios;
	}
	PEST_AspectRatio_4_x_2:
	{
#if (32 == EURASIA_ISPREGION_SIZEX) && (32 == EURASIA_ISPREGION_SIZEY)
		mov				r7, o1;
		mov				o1, o2;
		mov				o2, o4;
		mov				o4, r7;

		mov				r7, o5;
		mov				o5, o3;
		mov				o3, o6;
		mov				o6, r7;

#else
		mov				r7, o1;
		mov				o1, o2;
		mov				o2, o4;
		mov				o4, r7;

		mov				r7, o5;
		mov				o5, o3;
		mov				o3, o6;
		mov				o6, r7;

#endif
		br PEST_EndOfAspectRatios;
	}
	PEST_AspectRatio_4_x_16:
	{
#if (32 == EURASIA_ISPREGION_SIZEX) && (32 == EURASIA_ISPREGION_SIZEY)
		mov				r7, o1;
		mov				o1, o2;
		mov				o2, o16;
		mov				o16, o4;
		mov				o4, r7;

		mov				r7, o5;
		mov				o5, o3;
		mov				o3, o18;
		mov				o18, o20;
		mov				o20, r7;

		mov				r7, o6;
		mov				o6, o17;
		mov				o17, r7;

		mov				r7, o7;
		mov				o7, o19;
		mov				o19, o22;
		mov				o22, o21;
		mov				o21, r7;

		mov				r7, o65;
		mov				o65, o66;
		mov				o66, o80;
		mov				o80, o68;
		mov				o68, r7;

		mov				r7, o69;
		mov				o69, o67;
		mov				o67, o82;
		mov				o82, o84;
		mov				o84, r7;

		mov				r7, o70;
		mov				o70, o81;
		mov				o81, r7;

		mov				r7, o71;
		mov				o71, o83;
		mov				o83, o86;
		mov				o86, o85;
		mov				o85, r7;

		smbo			#0, #0, #129, #0;
		mov				r7, o70;
		smbo			#129, #0, #129, #0;

		mov				o70, o0;
		mov				o0, o1;
		mov				o1, o15;
		mov				o15, o3;
		mov				o3, o70;

		mov				o70, o4;
		mov				o4, o2;
		mov				o2, o17;
		mov				o17, o19;
		mov				o19, o70;

		mov				o70, o5;
		mov				o5, o16;
		mov				o16, o70;

		mov				o70, o6;
		mov				o6, o18;
		mov				o18, o21;
		mov				o21, o20;
		mov				o20, o70;

		mov				o70, o64;
		mov				o64, o65;
		mov				o65, o79;
		mov				o79, o67;
		mov				o67, o70;

		mov				o70, o68;
		mov				o68, o66;
		mov				o66, o81;
		mov				o81, o83;
		mov				o83, o70;

		mov				o70, o69;
		mov				o69, o80;
		mov				o80, o70;

		mov				o70, o82;
		mov				o82, o85;
		mov				o85, o84;
		smbo			#129, #0, #0, #0;
		mov				o84, r7;

#else
		mov				r7, o1;
		mov				o1, o2;
		mov				o2, o16;
		mov				o16, o4;
		mov				o4, r7;

		mov				r7, o5;
		mov				o5, o3;
		mov				o3, o18;
		mov				o18, o20;
		mov				o20, r7;

		mov				r7, o6;
		mov				o6, o17;
		mov				o17, r7;

		mov				r7, o7;
		mov				o7, o19;
		mov				o19, o22;
		mov				o22, o21;
		mov				o21, r7;

		mov				r7, o33;
		mov				o33, o34;
		mov				o34, o48;
		mov				o48, o36;
		mov				o36, r7;

		mov				r7, o37;
		mov				o37, o35;
		mov				o35, o50;
		mov				o50, o52;
		mov				o52, r7;

		mov				r7, o38;
		mov				o38, o49;
		mov				o49, r7;

		mov				r7, o39;
		mov				o39, o51;
		mov				o51, o54;
		mov				o54, o53;
		mov				o53, r7;

		mov				r7, o65;
		mov				o65, o66;
		mov				o66, o80;
		mov				o80, o68;
		mov				o68, r7;

		mov				r7, o69;
		mov				o69, o67;
		mov				o67, o82;
		mov				o82, o84;
		mov				o84, r7;

		mov				r7, o70;
		mov				o70, o81;
		mov				o81, r7;

		mov				r7, o71;
		mov				o71, o83;
		mov				o83, o86;
		mov				o86, o85;
		mov				o85, r7;

		mov				r7, o97;
		mov				o97, o98;
		mov				o98, o112;
		mov				o112, o100;
		mov				o100, r7;

		mov				r7, o101;
		mov				o101, o99;
		mov				o99, o114;
		mov				o114, o116;
		mov				o116, r7;

		mov				r7, o102;
		mov				o102, o113;
		mov				o113, r7;

		mov				r7, o103;
		mov				o103, o115;
		mov				o115, o118;
		mov				o118, o117;
		mov				o117, r7;

#endif
		br PEST_EndOfAspectRatios;
	}
	PEST_AspectRatio_8_x_2:
	{
#if (32 == EURASIA_ISPREGION_SIZEX) && (32 == EURASIA_ISPREGION_SIZEY)
		mov				r7, o1;
		mov				o1, o2;
		mov				o2, o8;
		mov				o8, o4;
		mov				o4, r7;

		mov				r7, o5;
		mov				o5, o3;
		mov				o3, o10;
		mov				o10, o12;
		mov				o12, r7;

		mov				r7, o9;
		mov				o9, o6;
		mov				o6, r7;

		mov				r7, o13;
		mov				o13, o7;
		mov				o7, o11;
		mov				o11, o14;
		mov				o14, r7;

#else
		mov				r7, o1;
		mov				o1, o2;
		mov				o2, o8;
		mov				o8, o4;
		mov				o4, r7;

		mov				r7, o5;
		mov				o5, o3;
		mov				o3, o10;
		mov				o10, o12;
		mov				o12, r7;

		mov				r7, o9;
		mov				o9, o6;
		mov				o6, r7;

		mov				r7, o13;
		mov				o13, o7;
		mov				o7, o11;
		mov				o11, o14;
		mov				o14, r7;

#endif
		br PEST_EndOfAspectRatios;
	}
	PEST_AspectRatio_8_x_4:
	{
#if (32 == EURASIA_ISPREGION_SIZEX) && (32 == EURASIA_ISPREGION_SIZEY)
		mov				r7, o1;
		mov				o1, o2;
		mov				o2, o4;
		mov				o4, r7;

		mov				r7, o5;
		mov				o5, o3;
		mov				o3, o6;
		mov				o6, r7;

		mov				r7, o8;
		mov				o8, o16;
		mov				o16, r7;

		mov				r7, o9;
		mov				o9, o18;
		mov				o18, o12;
		mov				o12, o17;
		mov				o17, o10;
		mov				o10, o20;
		mov				o20, r7;

		mov				r7, o13;
		mov				o13, o19;
		mov				o19, o14;
		mov				o14, o21;
		mov				o21, o11;
		mov				o11, o22;
		mov				o22, r7;

		mov				r7, o15;
		mov				o15, o23;
		mov				o23, r7;

		mov				r7, o25;
		mov				o25, o26;
		mov				o26, o28;
		mov				o28, r7;

		mov				r7, o29;
		mov				o29, o27;
		mov				o27, o30;
		mov				o30, r7;

#else
		mov				r7, o1;
		mov				o1, o2;
		mov				o2, o4;
		mov				o4, r7;

		mov				r7, o5;
		mov				o5, o3;
		mov				o3, o6;
		mov				o6, r7;

		mov				r7, o8;
		mov				o8, o16;
		mov				o16, r7;

		mov				r7, o9;
		mov				o9, o18;
		mov				o18, o12;
		mov				o12, o17;
		mov				o17, o10;
		mov				o10, o20;
		mov				o20, r7;

		mov				r7, o13;
		mov				o13, o19;
		mov				o19, o14;
		mov				o14, o21;
		mov				o21, o11;
		mov				o11, o22;
		mov				o22, r7;

		mov				r7, o15;
		mov				o15, o23;
		mov				o23, r7;

		mov				r7, o25;
		mov				o25, o26;
		mov				o26, o28;
		mov				o28, r7;

		mov				r7, o29;
		mov				o29, o27;
		mov				o27, o30;
		mov				o30, r7;

#endif
		br PEST_EndOfAspectRatios;
	}
	PEST_AspectRatio_8_x_8:
	{
#if (32 == EURASIA_ISPREGION_SIZEX) && (32 == EURASIA_ISPREGION_SIZEY)
		mov				r7, o1;
		mov				o1, o2;
		mov				o2, o4;
		mov				o4, r7;

		mov				r7, o5;
		mov				o5, o3;
		mov				o3, o6;
		mov				o6, r7;

		mov				r7, o8;
		mov				o8, o16;
		mov				o16, o64;
		mov				o64, r7;

		mov				r7, o9;
		mov				o9, o18;
		mov				o18, o68;
		mov				o68, r7;

		mov				r7, o12;
		mov				o12, o17;
		mov				o17, o66;
		mov				o66, r7;

		mov				r7, o13;
		mov				o13, o19;
		mov				o19, o70;
		mov				o70, r7;

		mov				r7, o10;
		mov				o10, o20;
		mov				o20, o65;
		mov				o65, r7;

		mov				r7, o11;
		mov				o11, o22;
		mov				o22, o69;
		mov				o69, r7;

		mov				r7, o14;
		mov				o14, o21;
		mov				o21, o67;
		mov				o67, r7;

		mov				r7, o15;
		mov				o15, o23;
		mov				o23, o71;
		mov				o71, r7;

		mov				r7, o24;
		mov				o24, o80;
		mov				o80, o72;
		mov				o72, r7;

		mov				r7, o25;
		mov				o25, o82;
		mov				o82, o76;
		mov				o76, r7;

		mov				r7, o28;
		mov				o28, o81;
		mov				o81, o74;
		mov				o74, r7;

		mov				r7, o29;
		mov				o29, o83;
		mov				o83, o78;
		mov				o78, r7;

		mov				r7, o26;
		mov				o26, o84;
		mov				o84, o73;
		mov				o73, r7;

		mov				r7, o27;
		mov				o27, o86;
		mov				o86, o77;
		mov				o77, r7;

		mov				r7, o30;
		mov				o30, o85;
		mov				o85, o75;
		mov				o75, r7;

		mov				r7, o31;
		mov				o31, o87;
		mov				o87, o79;
		mov				o79, r7;

		mov				r7, o89;
		mov				o89, o90;
		mov				o90, o92;
		mov				o92, r7;

		mov				r7, o93;
		mov				o93, o91;
		mov				o91, o94;
		mov				o94, r7;

#else
		mov				r7, o1;
		mov				o1, o2;
		mov				o2, o4;
		mov				o4, r7;

		mov				r7, o5;
		mov				o5, o3;
		mov				o3, o6;
		mov				o6, r7;

		mov				r7, o8;
		mov				o8, o16;
		mov				o16, o32;
		mov				o32, r7;

		mov				r7, o9;
		mov				o9, o18;
		mov				o18, o36;
		mov				o36, r7;

		mov				r7, o12;
		mov				o12, o17;
		mov				o17, o34;
		mov				o34, r7;

		mov				r7, o13;
		mov				o13, o19;
		mov				o19, o38;
		mov				o38, r7;

		mov				r7, o10;
		mov				o10, o20;
		mov				o20, o33;
		mov				o33, r7;

		mov				r7, o11;
		mov				o11, o22;
		mov				o22, o37;
		mov				o37, r7;

		mov				r7, o14;
		mov				o14, o21;
		mov				o21, o35;
		mov				o35, r7;

		mov				r7, o15;
		mov				o15, o23;
		mov				o23, o39;
		mov				o39, r7;

		mov				r7, o24;
		mov				o24, o48;
		mov				o48, o40;
		mov				o40, r7;

		mov				r7, o25;
		mov				o25, o50;
		mov				o50, o44;
		mov				o44, r7;

		mov				r7, o28;
		mov				o28, o49;
		mov				o49, o42;
		mov				o42, r7;

		mov				r7, o29;
		mov				o29, o51;
		mov				o51, o46;
		mov				o46, r7;

		mov				r7, o26;
		mov				o26, o52;
		mov				o52, o41;
		mov				o41, r7;

		mov				r7, o27;
		mov				o27, o54;
		mov				o54, o45;
		mov				o45, r7;

		mov				r7, o30;
		mov				o30, o53;
		mov				o53, o43;
		mov				o43, r7;

		mov				r7, o31;
		mov				o31, o55;
		mov				o55, o47;
		mov				o47, r7;

		mov				r7, o57;
		mov				o57, o58;
		mov				o58, o60;
		mov				o60, r7;

		mov				r7, o61;
		mov				o61, o59;
		mov				o59, o62;
		mov				o62, r7;

#endif
		br PEST_EndOfAspectRatios;
	}
	PEST_AspectRatio_8_x_16:
	{
#if (32 == EURASIA_ISPREGION_SIZEX) && (32 == EURASIA_ISPREGION_SIZEY)
		mov				r7, o1;
		mov				o1, o2;
		mov				o2, o4;
		mov				o4, r7;

		mov				r7, o5;
		mov				o5, o3;
		mov				o3, o6;
		mov				o6, r7;

		mov				r7, o8;
		mov				o8, o16;
		mov				o16, o64;
		mov				o64, r7;

		mov				r7, o9;
		mov				o9, o18;
		mov				o18, o68;
		mov				o68, r7;

		mov				r7, o12;
		mov				o12, o17;
		mov				o17, o66;
		mov				o66, r7;

		mov				r7, o13;
		mov				o13, o19;
		mov				o19, o70;
		mov				o70, r7;

		mov				r7, o10;
		mov				o10, o20;
		mov				o20, o65;
		mov				o65, r7;

		mov				r7, o11;
		mov				o11, o22;
		mov				o22, o69;
		mov				o69, r7;

		mov				r7, o14;
		mov				o14, o21;
		mov				o21, o67;
		mov				o67, r7;

		mov				r7, o15;
		mov				o15, o23;
		mov				o23, o71;
		mov				o71, r7;

		mov				r7, o24;
		mov				o24, o80;
		mov				o80, o72;
		mov				o72, r7;

		mov				r7, o25;
		mov				o25, o82;
		mov				o82, o76;
		mov				o76, r7;

		mov				r7, o28;
		mov				o28, o81;
		mov				o81, o74;
		mov				o74, r7;

		mov				r7, o29;
		mov				o29, o83;
		mov				o83, o78;
		mov				o78, r7;

		mov				r7, o26;
		mov				o26, o84;
		mov				o84, o73;
		mov				o73, r7;

		mov				r7, o27;
		mov				o27, o86;
		mov				o86, o77;
		mov				o77, r7;

		mov				r7, o30;
		mov				o30, o85;
		mov				o85, o75;
		mov				o75, r7;

		mov				r7, o31;
		mov				o31, o87;
		mov				o87, o79;
		mov				o79, r7;

		mov				r7, o89;
		mov				o89, o90;
		mov				o90, o92;
		mov				o92, r7;

		mov				r7, o93;
		mov				o93, o91;
		mov				o91, o94;
		mov				o94, r7;

		smbo			#0, #0, #129, #0;
		mov				r7, o92;
		smbo			#129, #0, #129, #0;

		mov				o92, o0;
		mov				o0, o1;
		mov				o1, o3;
		mov				o3, o92;

		mov				o92, o4;
		mov				o4, o2;
		mov				o2, o5;
		mov				o5, o92;

		mov				o92, o7;
		mov				o7, o15;
		mov				o15, o63;
		mov				o63, o92;

		mov				o92, o8;
		mov				o8, o17;
		mov				o17, o67;
		mov				o67, o92;

		mov				o92, o11;
		mov				o11, o16;
		mov				o16, o65;
		mov				o65, o92;

		mov				o92, o12;
		mov				o12, o18;
		mov				o18, o69;
		mov				o69, o92;

		mov				o92, o9;
		mov				o9, o19;
		mov				o19, o64;
		mov				o64, o92;

		mov				o92, o10;
		mov				o10, o21;
		mov				o21, o68;
		mov				o68, o92;

		mov				o92, o13;
		mov				o13, o20;
		mov				o20, o66;
		mov				o66, o92;

		mov				o92, o14;
		mov				o14, o22;
		mov				o22, o70;
		mov				o70, o92;

		mov				o92, o23;
		mov				o23, o79;
		mov				o79, o71;
		mov				o71, o92;

		mov				o92, o24;
		mov				o24, o81;
		mov				o81, o75;
		mov				o75, o92;

		mov				o92, o27;
		mov				o27, o80;
		mov				o80, o73;
		mov				o73, o92;

		mov				o92, o28;
		mov				o28, o82;
		mov				o82, o77;
		mov				o77, o92;

		mov				o92, o25;
		mov				o25, o83;
		mov				o83, o72;
		mov				o72, o92;

		mov				o92, o26;
		mov				o26, o85;
		mov				o85, o76;
		mov				o76, o92;

		mov				o92, o29;
		mov				o29, o84;
		mov				o84, o74;
		mov				o74, o92;

		mov				o92, o30;
		mov				o30, o86;
		mov				o86, o78;
		mov				o78, o92;

		mov				o92, o88;
		mov				o88, o89;
		mov				o89, o91;
		mov				o91, o92;

		mov				o92, o90;
		mov				o90, o93;
		smbo			#129, #0, #0, #0;
		mov				o93, r7;

#else
		mov				r7, o1;
		mov				o1, o2;
		mov				o2, o4;
		mov				o4, r7;

		mov				r7, o5;
		mov				o5, o3;
		mov				o3, o6;
		mov				o6, r7;

		mov				r7, o8;
		mov				o8, o16;
		mov				o16, o32;
		mov				o32, r7;

		mov				r7, o9;
		mov				o9, o18;
		mov				o18, o36;
		mov				o36, r7;

		mov				r7, o12;
		mov				o12, o17;
		mov				o17, o34;
		mov				o34, r7;

		mov				r7, o13;
		mov				o13, o19;
		mov				o19, o38;
		mov				o38, r7;

		mov				r7, o10;
		mov				o10, o20;
		mov				o20, o33;
		mov				o33, r7;

		mov				r7, o11;
		mov				o11, o22;
		mov				o22, o37;
		mov				o37, r7;

		mov				r7, o14;
		mov				o14, o21;
		mov				o21, o35;
		mov				o35, r7;

		mov				r7, o15;
		mov				o15, o23;
		mov				o23, o39;
		mov				o39, r7;

		mov				r7, o24;
		mov				o24, o48;
		mov				o48, o40;
		mov				o40, r7;

		mov				r7, o25;
		mov				o25, o50;
		mov				o50, o44;
		mov				o44, r7;

		mov				r7, o28;
		mov				o28, o49;
		mov				o49, o42;
		mov				o42, r7;

		mov				r7, o29;
		mov				o29, o51;
		mov				o51, o46;
		mov				o46, r7;

		mov				r7, o26;
		mov				o26, o52;
		mov				o52, o41;
		mov				o41, r7;

		mov				r7, o27;
		mov				o27, o54;
		mov				o54, o45;
		mov				o45, r7;

		mov				r7, o30;
		mov				o30, o53;
		mov				o53, o43;
		mov				o43, r7;

		mov				r7, o31;
		mov				o31, o55;
		mov				o55, o47;
		mov				o47, r7;

		mov				r7, o57;
		mov				o57, o58;
		mov				o58, o60;
		mov				o60, r7;

		mov				r7, o61;
		mov				o61, o59;
		mov				o59, o62;
		mov				o62, r7;

		mov				r7, o65;
		mov				o65, o66;
		mov				o66, o68;
		mov				o68, r7;

		mov				r7, o69;
		mov				o69, o67;
		mov				o67, o70;
		mov				o70, r7;

		mov				r7, o72;
		mov				o72, o80;
		mov				o80, o96;
		mov				o96, r7;

		mov				r7, o73;
		mov				o73, o82;
		mov				o82, o100;
		mov				o100, r7;

		mov				r7, o76;
		mov				o76, o81;
		mov				o81, o98;
		mov				o98, r7;

		mov				r7, o77;
		mov				o77, o83;
		mov				o83, o102;
		mov				o102, r7;

		mov				r7, o74;
		mov				o74, o84;
		mov				o84, o97;
		mov				o97, r7;

		mov				r7, o75;
		mov				o75, o86;
		mov				o86, o101;
		mov				o101, r7;

		mov				r7, o78;
		mov				o78, o85;
		mov				o85, o99;
		mov				o99, r7;

		mov				r7, o79;
		mov				o79, o87;
		mov				o87, o103;
		mov				o103, r7;

		mov				r7, o88;
		mov				o88, o112;
		mov				o112, o104;
		mov				o104, r7;

		mov				r7, o89;
		mov				o89, o114;
		mov				o114, o108;
		mov				o108, r7;

		mov				r7, o92;
		mov				o92, o113;
		mov				o113, o106;
		mov				o106, r7;

		mov				r7, o93;
		mov				o93, o115;
		mov				o115, o110;
		mov				o110, r7;

		mov				r7, o90;
		mov				o90, o116;
		mov				o116, o105;
		mov				o105, r7;

		mov				r7, o91;
		mov				o91, o118;
		mov				o118, o109;
		mov				o109, r7;

		mov				r7, o94;
		mov				o94, o117;
		mov				o117, o107;
		mov				o107, r7;

		mov				r7, o95;
		mov				o95, o119;
		mov				o119, o111;
		mov				o111, r7;

		mov				r7, o121;
		mov				o121, o122;
		mov				o122, o124;
		mov				o124, r7;

		mov				r7, o125;
		mov				o125, o123;
		mov				o123, o126;
		mov				o126, r7;

#endif
		br PEST_EndOfAspectRatios;
	}
#if (32 == EURASIA_ISPREGION_SIZEX) && (32 == EURASIA_ISPREGION_SIZEY)
	PEST_AspectRatio_16_x_2:
	{
		mov				r7, o1;
		mov				o1, o2;
		mov				o2, o32;
		mov				o32, o8;
		mov				o8, o4;
		mov				o4, r7;

		mov				r7, o5;
		mov				o5, o3;
		mov				o3, o34;
		mov				o34, o40;
		mov				o40, o12;
		mov				o12, r7;

		mov				r7, o9;
		mov				o9, o6;
		mov				o6, o33;
		mov				o33, o10;
		mov				o10, o36;
		mov				o36, r7;

		mov				r7, o13;
		mov				o13, o7;
		mov				o7, o35;
		mov				o35, o42;
		mov				o42, o44;
		mov				o44, r7;

		mov				r7, o37;
		mov				o37, o11;
		mov				o11, o38;
		mov				o38, o41;
		mov				o41, o14;
		mov				o14, r7;

		mov				r7, o45;
		mov				o45, o15;
		mov				o15, o39;
		mov				o39, o43;
		mov				o43, o46;
		mov				o46, r7;

		br PEST_EndOfAspectRatios;
	}
	PEST_AspectRatio_16_x_4:
	{
		mov				r7, o1;
		mov				o1, o2;
		mov				o2, o8;
		mov				o8, o16;
		mov				o16, o32;
		mov				o32, o4;
		mov				o4, r7;

		mov				r7, o5;
		mov				o5, o3;
		mov				o3, o10;
		mov				o10, o24;
		mov				o24, o48;
		mov				o48, o36;
		mov				o36, r7;

		mov				r7, o9;
		mov				o9, o18;
		mov				o18, o40;
		mov				o40, o20;
		mov				o20, o33;
		mov				o33, o6;
		mov				o6, r7;

		mov				r7, o12;
		mov				o12, o17;
		mov				o17, o34;
		mov				o34, r7;

		mov				r7, o13;
		mov				o13, o19;
		mov				o19, o42;
		mov				o42, o28;
		mov				o28, o49;
		mov				o49, o38;
		mov				o38, r7;

		mov				r7, o37;
		mov				o37, o7;
		mov				o7, o11;
		mov				o11, o26;
		mov				o26, o56;
		mov				o56, o52;
		mov				o52, r7;

		mov				r7, o41;
		mov				o41, o22;
		mov				o22, r7;

		mov				r7, o44;
		mov				o44, o21;
		mov				o21, o35;
		mov				o35, o14;
		mov				o14, o25;
		mov				o25, o50;
		mov				o50, r7;

		mov				r7, o45;
		mov				o45, o23;
		mov				o23, o43;
		mov				o43, o30;
		mov				o30, o57;
		mov				o57, o54;
		mov				o54, r7;

		mov				r7, o15;
		mov				o15, o27;
		mov				o27, o58;
		mov				o58, o60;
		mov				o60, o53;
		mov				o53, o39;
		mov				o39, r7;

		mov				r7, o46;
		mov				o46, o29;
		mov				o29, o51;
		mov				o51, r7;

		mov				r7, o47;
		mov				o47, o31;
		mov				o31, o59;
		mov				o59, o62;
		mov				o62, o61;
		mov				o61, o55;
		mov				o55, r7;

		br PEST_EndOfAspectRatios;
	}
	PEST_AspectRatio_16_x_8:
	{
		mov				r7, o1;
		mov				o1, o2;
		mov				o2, o64;
		mov				o64, o32;
		mov				o32, o4;
		mov				o4, r7;

		mov				r7, o5;
		mov				o5, o3;
		mov				o3, o66;
		mov				o66, o96;
		mov				o96, o36;
		mov				o36, r7;

		mov				r7, o8;
		mov				o8, o16;
		mov				o16, r7;

		mov				r7, o9;
		mov				o9, o18;
		mov				o18, o72;
		mov				o72, o48;
		mov				o48, o12;
		mov				o12, o17;
		mov				o17, o10;
		mov				o10, o80;
		mov				o80, o40;
		mov				o40, o20;
		mov				o20, r7;

		mov				r7, o13;
		mov				o13, o19;
		mov				o19, o74;
		mov				o74, o112;
		mov				o112, o44;
		mov				o44, o21;
		mov				o21, o11;
		mov				o11, o82;
		mov				o82, o104;
		mov				o104, o52;
		mov				o52, r7;

		mov				r7, o33;
		mov				o33, o6;
		mov				o6, o65;
		mov				o65, o34;
		mov				o34, o68;
		mov				o68, r7;

		mov				r7, o37;
		mov				o37, o7;
		mov				o7, o67;
		mov				o67, o98;
		mov				o98, o100;
		mov				o100, r7;

		mov				r7, o41;
		mov				o41, o22;
		mov				o22, o73;
		mov				o73, o50;
		mov				o50, o76;
		mov				o76, o49;
		mov				o49, o14;
		mov				o14, o81;
		mov				o81, o42;
		mov				o42, o84;
		mov				o84, r7;

		mov				r7, o45;
		mov				o45, o23;
		mov				o23, o75;
		mov				o75, o114;
		mov				o114, o108;
		mov				o108, o53;
		mov				o53, o15;
		mov				o15, o83;
		mov				o83, o106;
		mov				o106, o116;
		mov				o116, r7;

		mov				r7, o35;
		mov				o35, o70;
		mov				o70, o97;
		mov				o97, o38;
		mov				o38, o69;
		mov				o69, r7;

		mov				r7, o39;
		mov				o39, o71;
		mov				o71, o99;
		mov				o99, o102;
		mov				o102, o101;
		mov				o101, r7;

		mov				r7, o43;
		mov				o43, o86;
		mov				o86, o105;
		mov				o105, o54;
		mov				o54, o77;
		mov				o77, o51;
		mov				o51, o78;
		mov				o78, o113;
		mov				o113, o46;
		mov				o46, o85;
		mov				o85, r7;

		mov				r7, o47;
		mov				o47, o87;
		mov				o87, o107;
		mov				o107, o118;
		mov				o118, o109;
		mov				o109, o55;
		mov				o55, o79;
		mov				o79, o115;
		mov				o115, o110;
		mov				o110, o117;
		mov				o117, r7;

		mov				r7, o25;
		mov				o25, o26;
		mov				o26, o88;
		mov				o88, o56;
		mov				o56, o28;
		mov				o28, r7;

		mov				r7, o29;
		mov				o29, o27;
		mov				o27, o90;
		mov				o90, o120;
		mov				o120, o60;
		mov				o60, r7;

		mov				r7, o57;
		mov				o57, o30;
		mov				o30, o89;
		mov				o89, o58;
		mov				o58, o92;
		mov				o92, r7;

		mov				r7, o61;
		mov				o61, o31;
		mov				o31, o91;
		mov				o91, o122;
		mov				o122, o124;
		mov				o124, r7;

		mov				r7, o59;
		mov				o59, o94;
		mov				o94, o121;
		mov				o121, o62;
		mov				o62, o93;
		mov				o93, r7;

		mov				r7, o63;
		mov				o63, o95;
		mov				o95, o123;
		mov				o123, o126;
		mov				o126, o125;
		mov				o125, r7;

		mov				r7, o111;
		mov				o111, o119;
		mov				o119, r7;

		br PEST_EndOfAspectRatios;
	}
	PEST_AspectRatio_16_x_16:
	{
		smbo			#1, #0, #1, #0;
		mov				r6, o0;
		mov				o0, o1;
		mov				o1, o63;
		mov				o63, o127;
		mov				o127, o31;
		mov				o31, o3;
		mov				o3, r6;

		mov				r6, o7;
		mov				o7, o15;
		mov				o15, r6;

		smbo			#6, #0, #6, #0;
		mov				r1, o27;
		mov				o27, o0;
		mov				o0, o59;
		mov				o59, o124;
		mov				o124, o90;
		mov				o90, o126;
		mov				o126, r1;

		mov				r1, o28;
		mov				o28, o62;
		mov				o62, o123;
		mov				o123, r1;

		smbo			#0, #0, #10, #0;
		mov				r7, o87;
		smbo			#10, #0, #10, #0;

		mov				o87, o2;
		mov				o2, o7;
		mov				o7, o0;
		mov				o0, o70;
		mov				o70, o126;
		mov				o126, o38;
		mov				o38, o87;

		mov				o87, o124;
		smbo			#10, #0, #0, #0;
		mov				o124, r7;

		smbo			#0, #0, #14, #0;
		mov				r7, o0;
		smbo			#14, #0, #14, #0;

		mov				o0, o67;
		mov				o67, o124;
		mov				o124, o98;
		mov				o98, o126;
		mov				o126, o35;
		smbo			#14, #0, #0, #0;
		mov				o35, r7;

		smbo			#0, #0, #22, #0;
		mov				r7, o91;
		smbo			#22, #0, #22, #0;

		mov				o91, o19;
		mov				o19, o0;
		mov				o0, o51;
		mov				o51, o124;
		mov				o124, o82;
		mov				o82, o126;
		mov				o126, o91;

		mov				o91, o20;
		mov				o20, o62;
		mov				o62, o115;
		mov				o115, o28;
		mov				o28, o54;
		mov				o54, o123;
		mov				o123, o91;

		mov				o91, o120;
		smbo			#22, #0, #0, #0;
		mov				o120, r7;

		smbo			#0, #0, #25, #0;
		mov				r7, o80;
		smbo			#25, #0, #25, #0;

		mov				o80, o0;
		mov				o0, o1;
		mov				o1, o63;
		mov				o63, o127;
		mov				o127, o31;
		mov				o31, o3;
		mov				o3, o80;

		mov				o80, o125;
		smbo			#25, #0, #0, #0;
		mov				o125, r7;

		smbo			#0, #0, #30, #0;
		mov				r7, o28;
		smbo			#30, #0, #30, #0;

		mov				o28, o27;
		mov				o27, o0;
		mov				o0, o59;
		mov				o59, o124;
		mov				o124, o90;
		mov				o90, o126;
		mov				o126, o28;

		mov				o28, o62;
		mov				o62, o123;
		smbo			#30, #0, #0, #0;
		mov				o123, r7;

		smbo			#0, #0, #99, #0;
		mov				r7, o27;
		smbo			#99, #0, #99, #0;

		mov				o27, o2;
		mov				o2, o36;
		mov				o36, o0;
		mov				o0, o99;
		mov				o99, o126;
		mov				o126, o67;
		mov				o67, o27;

		mov				o27, o3;
		mov				o3, o98;
		mov				o98, o64;
		mov				o64, o27;

		mov				o27, o11;
		mov				o11, o114;
		mov				o114, o72;
		mov				o72, o19;
		mov				o19, o106;
		mov				o106, o80;
		mov				o80, o27;

		mov				o27, o22;
		mov				o22, o59;
		mov				o59, o27;

		mov				o27, o122;
		mov				o122, o88;
		smbo			#99, #0, #0, #0;
		mov				o88, r7;

		smbo			#0, #0, #103, #0;
		mov				r7, o0;
		smbo			#103, #0, #103, #0;

		mov				o0, o96;
		mov				o96, o124;
		mov				o124, o127;
		mov				o127, o126;
		mov				o126, o64;
		smbo			#103, #0, #0, #0;
		mov				o64, r7;

		smbo			#0, #0, #107, #0;
		mov				r7, o2;
		smbo			#107, #0, #107, #0;

		mov				o2, o44;
		mov				o44, o0;
		mov				o0, o107;
		mov				o107, o126;
		mov				o126, o75;
		smbo			#107, #0, #0, #0;
		mov				o75, r7;

		smbo			#0, #0, #115, #0;
		mov				r7, o2;
		smbo			#115, #0, #115, #0;

		mov				o2, o28;
		mov				o28, o0;
		mov				o0, o91;
		mov				o91, o126;
		mov				o126, o59;
		smbo			#115, #0, #0, #0;
		mov				o59, r7;

		smbo			#0, #0, #119, #0;
		mov				r7, o0;
		smbo			#119, #0, #119, #0;

		mov				o0, o88;
		mov				o88, o124;
		mov				o124, o119;
		mov				o119, o126;
		mov				o126, o56;
		smbo			#119, #0, #0, #0;
		mov				o56, r7;

		smbo			#0, #0, #123, #0;
		mov				r7, o116;
		smbo			#123, #0, #123, #0;

		mov				o116, o2;
		mov				o2, o36;
		mov				o36, o0;
		mov				o0, o99;
		mov				o99, o126;
		mov				o126, o67;
		mov				o67, o116;

		mov				o116, o124;
		smbo			#123, #0, #0, #0;
		mov				o124, r7;

		smbo			#0, #0, #127, #0;
		mov				r7, o0;
		smbo			#127, #0, #127, #0;

		mov				o0, o96;
		mov				o96, o124;
		mov				o124, o127;
		mov				o127, o126;
		mov				o126, o64;
		smbo			#127, #0, #0, #0;
		mov				o64, r7;

		smbo			#3, #0, #3, #0;
		mov				r4, o33;
		mov				o33, o2;
		mov				o2, o0;
		mov				o0, o63;
		smbo			#66, #0, #160, #0;
		mov				o0, o32;
		mov				o126, o0;
		smbo			#160, #0, #7, #0;
		mov				o0, r0;

		smbo			#7, #0, #9, #0;
		mov				r0, o0;
		mov				o2, o9;
		mov				o11, o63;
		smbo			#20, #0, #20, #0;
		mov				o52, o124;
		mov				o124, o20;
		mov				o20, o0;
		smbo			#13, #0, #7, #0;
		mov				o7, r0;

		smbo			#7, #0, #13, #0;
		mov				r0, o39;
		mov				o45, o0;
		mov				o6, o6;
		mov				o12, o61;
		smbo			#74, #0, #168, #0;
		mov				o0, o40;
		smbo			#168, #0, #168, #0;
		mov				o40, o0;
		smbo			#168, #0, #7, #0;
		mov				o0, r0;

		smbo			#7, #0, #7, #0;
		mov				r0, o30;
		mov				o30, o0;
		mov				o0, o60;
		smbo			#67, #0, #164, #0;
		mov				o0, o30;
		mov				o127, o60;
		smbo			#164, #0, #164, #0;
		mov				o60, o0;
		smbo			#44, #0, #7, #0;
		mov				o120, r0;

		smbo			#7, #0, #11, #0;
		mov				r0, o33;
		mov				o37, o10;
		mov				o14, o0;
		mov				o4, o71;
		smbo			#82, #0, #176, #0;
		mov				o0, o24;
		mov				o118, o0;
		smbo			#176, #0, #7, #0;
		mov				o0, r0;

		smbo			#7, #0, #23, #0;
		mov				r0, o22;
		mov				o38, o0;
		mov				o16, o52;
		smbo			#75, #0, #180, #0;
		mov				o0, o30;
		smbo			#180, #0, #180, #0;
		mov				o30, o52;
		mov				o52, o0;
		smbo			#53, #0, #7, #0;
		mov				o127, r0;

		smbo			#7, #0, #15, #0;
		mov				r0, o38;
		mov				o46, o0;
		mov				o8, o68;
		smbo			#83, #0, #172, #0;
		mov				o0, o30;
		mov				o119, o68;
		smbo			#172, #0, #172, #0;
		mov				o68, o0;
		smbo			#172, #0, #7, #0;
		mov				o0, r0;

		smbo			#7, #0, #35, #0;
		mov				r0, o0;
		mov				o28, o35;
		smbo			#70, #0, #100, #0;
		mov				o0, o93;
		mov				o123, o62;
		mov				o92, o0;
		mov				o30, o33;
		smbo			#38, #0, #7, #0;
		mov				o95, r0;

		smbo			#7, #0, #38, #0;
		mov				r0, o0;
		mov				o31, o31;
		mov				o62, o93;
		mov				o124, o60;
		smbo			#98, #0, #161, #0;
		mov				o0, o35;
		mov				o98, o0;
		smbo			#39, #0, #7, #0;
		mov				o122, r0;

		smbo			#7, #0, #39, #0;
		mov				r0, o0;
		mov				o32, o32;
		smbo			#71, #0, #165, #0;
		mov				o0, o30;
		mov				o124, o61;
		smbo			#165, #0, #165, #0;
		mov				o61, o63;
		mov				o63, o0;
		smbo			#43, #0, #7, #0;
		mov				o122, r0;

		smbo			#7, #0, #43, #0;
		mov				r0, o0;
		mov				o36, o43;
		smbo			#86, #0, #108, #0;
		mov				o0, o93;
		mov				o115, o70;
		mov				o92, o0;
		mov				o22, o41;
		smbo			#46, #0, #7, #0;
		mov				o103, r0;

		smbo			#7, #0, #46, #0;
		mov				r0, o0;
		mov				o39, o39;
		mov				o78, o93;
		smbo			#114, #0, #114, #0;
		mov				o25, o0;
		mov				o0, o90;
		mov				o90, o63;
		smbo			#177, #0, #7, #0;
		mov				o0, r0;

		smbo			#7, #0, #47, #0;
		mov				r0, o0;
		mov				o40, o40;
		smbo			#87, #0, #181, #0;
		mov				o0, o22;
		mov				o116, o61;
		smbo			#181, #0, #181, #0;
		mov				o61, o55;
		mov				o55, o0;
		smbo			#60, #0, #7, #0;
		mov				o121, r0;

		smbo			#7, #0, #27, #0;
		mov				r0, o33;
		mov				o53, o2;
		mov				o22, o0;
		mov				o20, o63;
		smbo			#90, #0, #184, #0;
		mov				o0, o32;
		mov				o126, o0;
		smbo			#61, #0, #7, #0;
		mov				o123, r0;

		smbo			#7, #0, #31, #0;
		mov				r0, o30;
		mov				o54, o0;
		mov				o24, o60;
		smbo			#91, #0, #188, #0;
		mov				o0, o30;
		mov				o127, o60;
		smbo			#188, #0, #188, #0;
		mov				o60, o0;
		smbo			#188, #0, #7, #0;
		mov				o0, r0;

		smbo			#7, #0, #51, #0;
		mov				r0, o0;
		mov				o44, o27;
		smbo			#78, #0, #116, #0;
		mov				o0, o93;
		smbo			#116, #0, #116, #0;
		mov				o93, o54;
		mov				o54, o0;
		mov				o0, o25;
		smbo			#54, #0, #7, #0;
		mov				o87, r0;

		smbo			#7, #0, #54, #0;
		mov				r0, o0;
		mov				o47, o23;
		mov				o70, o93;
		smbo			#106, #0, #106, #0;
		mov				o41, o0;
		mov				o0, o106;
		mov				o106, o63;
		smbo			#55, #0, #7, #0;
		mov				o114, r0;

		smbo			#7, #0, #55, #0;
		mov				r0, o0;
		mov				o48, o24;
		smbo			#79, #0, #173, #0;
		mov				o0, o38;
		smbo			#173, #0, #173, #0;
		mov				o38, o61;
		mov				o61, o71;
		mov				o71, o0;
		smbo			#59, #0, #7, #0;
		mov				o114, r0;

		smbo			#7, #0, #59, #0;
		mov				r0, o0;
		mov				o52, o35;
		smbo			#94, #0, #124, #0;
		mov				o0, o93;
		mov				o123, o62;
		mov				o92, o0;
		mov				o30, o33;
		smbo			#62, #0, #7, #0;
		mov				o95, r0;

		smbo			#7, #0, #62, #0;
		mov				r0, o0;
		mov				o55, o31;
		mov				o86, o93;
		smbo			#122, #0, #122, #0;
		mov				o33, o0;
		mov				o0, o98;
		mov				o98, o63;
		smbo			#63, #0, #7, #0;
		mov				o122, r0;

		smbo			#7, #0, #63, #0;
		mov				r0, o0;
		mov				o56, o32;
		smbo			#95, #0, #189, #0;
		mov				o0, o30;
		mov				o124, o61;
		smbo			#189, #0, #189, #0;
		mov				o61, o63;
		mov				o63, o0;
		smbo			#111, #0, #7, #0;
		mov				o78, r0;

		smbo			#7, #0, #111, #0;
		mov				r0, o0;
		mov				o104, o104;
		smbo			#183, #0, #183, #0;
		mov				o32, o52;
		mov				o52, o63;
		mov				o63, o54;
		mov				o54, o0;
		smbo			#183, #0, #7, #0;
		mov				o0, r0;

		br PEST_EndOfAspectRatios;
	}
#endif
	PEST_EndOfAspectRatios:

#if (32 == EURASIA_ISPREGION_SIZEX) && (32 == EURASIA_ISPREGION_SIZEY)
	/* setting the smbo back after the moves*/
	smbo				#0, #0, #0, #0;
#endif

	/* r4: number of lines,
	 * r5: num of pixels in each line
	 * r6: dst bytes per pixel
	 * r10: number of done tiles 
	 */

#if defined(FIX_HW_BRN_30842)
	/* the number of submited bytes per emit
	 * this assuming 2 pipes per tile in X
	 */
	mov					r7, #~(EURASIA_ISPREGION_SIZEX - 1);
	and.testnz			p0, r5, r7;
	p0 mov				r7, #(EURASIA_ISPREGION_SIZEX / 2);
	(!p0) mov			r7, r5;
	imae				r8, r7.low, r6.low, #0, u32;

	/* set the PBE count*/

#if defined(SGX_FEATURE_UNIFIED_STORE_64BITS)
	shl					r7, r7, #EURASIA_PIXELBES0LO_COUNT_SHIFT;
	or					r0, r0, r7;	
	
	and					r9, r1, #~EURASIA_PIXELBES0HI_FBADDR_CLRMSK;
	shr					r9, r9, #EURASIA_PIXELBES0HI_FBADDR_SHIFT;
	shl					r9, r9, #EURASIA_PIXELBE_FBADDR_ALIGNSHIFT;
#else

#if defined(SGX535)
	/* Round emit count to nearest 4 pixels (HW restriction) */
	add					r7, r7.low, #3;
	shr					r7, r7, #2;
	shl					r7, r7, #(EURASIA_PIXELBE2S0_COUNT_SHIFT + 2);
#else
	shl					r7, r7, #EURASIA_PIXELBE2S0_COUNT_SHIFT;
#endif
	or					r2, r2, r7;
	
	and					r9, r3, #~EURASIA_PIXELBE2S1_FBADDR_CLRMSK;
	shr					r9, r9, #EURASIA_PIXELBE2S1_FBADDR_SHIFT;
	shl					r9, r9, #EURASIA_PIXELBE_FBADDR_ALIGNSHIFT;
#endif

	mov					r7, #((EURASIA_ISPREGION_SIZEX / 2) - 1);
	and.testnz			p0, r5, r7; 
	p0 br PEST_ThinnerThanHalfTile;
	{
		and.testz		p0, r10, r10;
		p0 br PEST_FirstTile;
		{
			/* readjusting tile base this step should have no effect if
			 * inside the first tile or if the strided (PBE) offset matches
			 * the twiddled (SW). otherwise we step over the done tiles
			 * compensating for the PBE doing the same in strided.
			 */
			mov				r11, #~(EURASIA_ISPREGION_SIZEX - 1);
			and.testnz		p0, r5, r11;
			p0 mov			r11, #(EURASIA_ISPREGION_SIZEX / 2);
			(!p0) mov		r11, r5;

			iadd32			r7, r4.low, #(-1);
			shl				r7, r7, #1;
			imae			r7, r11.low, r7.low, #0, u32;
			imae			r7, r6.low, r7.low, #0, u32;
			imae			r9, r10.low, r7.low, r9, u32;
		}
		PEST_FirstTile:
		
		and.testz			p0, #1, g17;
		p0 br PEST_NotRightSide;
		{
			/* on odd pipes we step over the even pipe but compensate
			 * for the PBE doing the same in strided mode hence r4 - 1
			 * on 4 pipes we musn't have pixels on pipe 3 so that shouldn't
			 * matter
			 */ 
			iadd32			r7, r4.low, #(-1);
			imae			r9, r7.low, r8.low, r9, u32;
		}
		PEST_NotRightSide:
	}
	PEST_ThinnerThanHalfTile:

#if EURASIA_ISPREGION_SIZEY == 32 /* the real condition would be 2_USSE_PIPES_PER_Y */ 
	xor.testz			p0, r4, #EURASIA_ISPREGION_SIZEY;
	p0 shr				r4, r4, #1;
#endif

	mov					r6, #0; 
	mov					r10, #0; 
	PEST_EachLine:
	{
#if defined(SGX_FEATURE_UNIFIED_STORE_64BITS)
		and					r0, r0, #EURASIA_PIXELBES0LO_OOFF_CLRMSK;
		shl					r7, r6, #EURASIA_PIXELBES0LO_OOFF_SHIFT;
		or					r0, r0, r7;

		and					r1, r1, #EURASIA_PIXELBES0HI_FBADDR_CLRMSK;
		shr					r7, r9, #EURASIA_PIXELBE_FBADDR_ALIGNSHIFT;
		shl					r7, r7, #EURASIA_PIXELBES0HI_FBADDR_SHIFT;
		or					r1, r1, r7; 
#else
		and					r2, r2, #EURASIA_PIXELBE2S0_OOFF_CLRMSK;
		shl					r7, r6, #EURASIA_PIXELBE2S0_OOFF_SHIFT;
		or					r2, r2, r7;

		and					r3, r3, #EURASIA_PIXELBE2S1_FBADDR_CLRMSK;
		shr					r7, r9, #EURASIA_PIXELBE_FBADDR_ALIGNSHIFT;
		shl					r7, r7, #EURASIA_PIXELBE2S1_FBADDR_SHIFT;
		or					r3, r3, r7; 
#endif
		
		WAITFOROTHERPIPE(PEST_WaitForPipe);

#if ! defined(SGX_FEATURE_UNIFIED_STORE_64BITS)
		/* kick the PBE*/
		emitpix1			r0, r1;
#endif

		PixelEventSubTwiddled_Sideband_Byte:
		nop; /*emitpix2 or emitpix with 64 bit US*/

		FLIPFLOP;

		iaddu32				r9, r8.low, r9;

		/* PBE - unswizzled jump to next line*/
#if (EURASIA_ISPREGION_SIZEX == 32 && EURASIA_ISPREGION_SIZEY == 32)
		and					r7, #3, r10;
		/* ob offsets: 0, 2, 16, 18, !!64 ... (if 16 pix wide)*/
		and.testz			p0, r7, #1;
		p0 iaddu32			r6, r6.low, #2;
		p0 br PEST_Even;
		{
			and.testz			p0, r7, #2;
			p0 iadd32			r6, r6.low, #(14);
			(!p0) iadd32		r6, r6.low, #(46);
		}
		PEST_Even: 
#else
		and.testz			p0, r10, #1;
		/* ob offsets: 0, 2, 16, 18, ... (if 8 pix wide)*/
		p0 iaddu32			r6, r6.low, #(2); 
		(!p0) iaddu32		r6, r6.low, #(14);
#endif

		iadd32				r10, r10.low, #1;
		xor.testnz			p0, r4, r10; 
		p0 br PEST_EachLine;	
	}
	
	DUMMYTILEEMIT(r3);
#else
	/* do we cross pipe boundary ? */
	mov					r7, #PEST_2PIPE_MODE_MASK;
	and.testnz			p0, r6, r7; 
	p0 br PEST_CrossesPipeBoundary;
	{
		/* fast path */
		/* do a single emit that frees up the partitions and kicks off the tile*/
#if ! defined(SGX_FEATURE_UNIFIED_STORE_64BITS)
		/* kick the PBE*/
		emitpix1			r0, r1;
#endif

		PixelEventSubTwiddled_Sideband_SinglePipe_Byte:
		nop; /*emitpix2 or emitpix with 64 bit US*/

		/* should be unreachable if previous had .end*/	
		nop.end;
	}
	PEST_CrossesPipeBoundary:

	/* set NOADVANCE */
#if defined(SGX_FEATURE_UNIFIED_STORE_64BITS)
	or					r13, r13, #EURASIA_PIXELBES2HI_NOADVANCE;
#else
	or					r1, r1, #EURASIA_PIXELBE1S1_NOADVANCE;
#endif

	mov					r7, #PEST_DIRECTION_X_MASK;
	and.testnz			p0, r6, r7; 
	p0 br PEST_DirectionNotYFirst;
	{
		/* now move the clip, we assume 2 pipes in given direction for ISP tile*/
#if defined(SGX_FEATURE_UNIFIED_STORE_64BITS)
		/* YMIN */
		and				r7, r12, #~EURASIA_PIXELBES2LO_YMIN_CLRMSK;
		and 			r12, r12, #EURASIA_PIXELBES2LO_YMIN_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBES2LO_YMIN_SHIFT;
		imae			r7, r10.low, #EURASIA_ISPREGION_SIZEY, r7, u32;
		shl				r7, r7, #EURASIA_PIXELBES2LO_YMIN_SHIFT;
		or				r12, r12, r7;

		/* YMAX */
		and				r7, r13, #~EURASIA_PIXELBES2HI_YMAX_CLRMSK;
		and 			r13, r13, #EURASIA_PIXELBES2HI_YMAX_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBES2HI_YMAX_SHIFT;
		imae			r7, r10.low, #EURASIA_ISPREGION_SIZEY, r7, u32;
		shl				r7, r7, #EURASIA_PIXELBES2HI_YMAX_SHIFT;
		or				r13, r13, r7;
#else
		/* YMIN */
		and				r7, r0, #~EURASIA_PIXELBE1S0_YMIN_CLRMSK;
		and 			r0, r0, #EURASIA_PIXELBE1S0_YMIN_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBE1S0_YMIN_SHIFT;
		imae			r7, r10.low, #EURASIA_ISPREGION_SIZEY, r7, u32;
		shl				r7, r7, #EURASIA_PIXELBE1S0_YMIN_SHIFT;
		or				r0, r0, r7;

		/* YMAX */
		and				r7, r1, #~EURASIA_PIXELBE1S1_YMAX_CLRMSK;
		and 			r1, r1, #EURASIA_PIXELBE1S1_YMAX_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBE1S1_YMAX_SHIFT;
		imae			r7, r10.low, #EURASIA_ISPREGION_SIZEY, r7, u32;
		shl				r7, r7, #EURASIA_PIXELBE1S1_YMAX_SHIFT;
		or				r1, r1, r7;
#endif
		br PEST_EndDirectionsFirst;
	}
	PEST_DirectionNotYFirst:
	{
		/* move the fbaddr*/
#if defined(SGX_FEATURE_UNIFIED_STORE_64BITS)
		and				r7, r1, #~EURASIA_PIXELBES0HI_FBADDR_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBES0HI_FBADDR_SHIFT;
#else
		and				r7, r3, #~EURASIA_PIXELBE2S1_FBADDR_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBE2S1_FBADDR_SHIFT;
#endif
		shl				r7, r7, #EURASIA_PIXELBE_FBADDR_ALIGNSHIFT;
		and				r8, r6, #PEST_ADDR_SHIFT_MASK;
		imae			r8, r8.low, r10.low, #0, u32;
		shl				r8, r8, #1;
		iadd32			r7, r8.low, r7;
		shr				r7, r7, #EURASIA_PIXELBE_FBADDR_ALIGNSHIFT;
#if defined(SGX_FEATURE_UNIFIED_STORE_64BITS)
		shl				r7, r7, #EURASIA_PIXELBES0HI_FBADDR_SHIFT;
		and				r1, r1, #EURASIA_PIXELBES0HI_FBADDR_CLRMSK;
		or				r1, r1, r7; 
#else
		shl				r7, r7, #EURASIA_PIXELBE2S1_FBADDR_SHIFT;
		and				r3, r3, #EURASIA_PIXELBE2S1_FBADDR_CLRMSK;
		or				r3, r3, r7;
#endif

#if defined(SGX_FEATURE_UNIFIED_STORE_64BITS)
		/* XMIN */
		and				r7, r12, #~EURASIA_PIXELBES2LO_XMIN_CLRMSK;
		and 			r12, r12, #EURASIA_PIXELBES2LO_XMIN_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBES2LO_XMIN_SHIFT;
		imae			r7, r10.low, #EURASIA_ISPREGION_SIZEX, r7, u32;
		shl				r7, r7, #EURASIA_PIXELBES2LO_XMIN_SHIFT;
		or				r12, r12, r7;

		/* XMAX */
		and				r7, r13, #~EURASIA_PIXELBES2HI_XMAX_CLRMSK;
		and 			r13, r13, #EURASIA_PIXELBES2HI_XMAX_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBES2HI_XMAX_SHIFT;
		imae			r7, r10.low, #EURASIA_ISPREGION_SIZEX, r7, u32;
		shl				r7, r7, #EURASIA_PIXELBES2HI_XMAX_SHIFT;
		or				r13, r13, r7;
#else
		/* XMIN */
		and				r7, r0, #~EURASIA_PIXELBE1S0_XMIN_CLRMSK;
		and 			r0, r0, #EURASIA_PIXELBE1S0_XMIN_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBE1S0_XMIN_SHIFT;
		imae			r7, r10.low, #EURASIA_ISPREGION_SIZEX, r7, u32;
		shl				r7, r7, #EURASIA_PIXELBE1S0_XMIN_SHIFT;
		or				r0, r0, r7;

		/* XMAX */
		and				r7, r1, #~EURASIA_PIXELBE1S1_XMAX_CLRMSK;
		and 			r1, r1, #EURASIA_PIXELBE1S1_XMAX_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBE1S1_XMAX_SHIFT;
		imae			r7, r10.low, #EURASIA_ISPREGION_SIZEX, r7, u32;
		shl				r7, r7, #EURASIA_PIXELBE1S1_XMAX_SHIFT;
		or				r1, r1, r7;
#endif
	}
	PEST_EndDirectionsFirst:

	WAITFOROTHERPIPE(PEST_2Pipes_First);

#if ! defined(SGX_FEATURE_UNIFIED_STORE_64BITS)
	/* kick the PBE*/
	emitpix1			r0, r1;
#endif

	PixelEventSubTwiddled_Sideband_2Pipes_First_Byte:
	nop; /*emitpix2 or emitpix with 64 bit US*/

	FLIPFLOP;

	mov					r7, #PEST_DIRECTION_X_MASK;
	and.testnz			p0, r6, r7; 
	p0 br PEST_DirectionNotY;
	{
		/* now move the clip, we assume 2 pipes in given direction for ISP tile*/
#if defined(SGX_FEATURE_UNIFIED_STORE_64BITS)
		/* YMIN */
		and				r7, r12, #~EURASIA_PIXELBES2LO_YMIN_CLRMSK;
		and 			r12, r12, #EURASIA_PIXELBES2LO_YMIN_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBES2LO_YMIN_SHIFT;
		iadd32			r7, r7.low, #(EURASIA_ISPREGION_SIZEY / 2);
		shl				r7, r7, #EURASIA_PIXELBES2LO_YMIN_SHIFT;
		or				r12, r12, r7;

		/* YMAX */
		and				r7, r13, #~EURASIA_PIXELBES2HI_YMAX_CLRMSK;
		and 			r13, r13, #EURASIA_PIXELBES2HI_YMAX_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBES2HI_YMAX_SHIFT;
		iadd32			r7, r7.low, #(EURASIA_ISPREGION_SIZEY / 2);
		shl				r7, r7, #EURASIA_PIXELBES2HI_YMAX_SHIFT;
		or				r13, r13, r7;
#else
		/* YMIN */
		and				r7, r0, #~EURASIA_PIXELBE1S0_YMIN_CLRMSK;
		and 			r0, r0, #EURASIA_PIXELBE1S0_YMIN_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBE1S0_YMIN_SHIFT;
		iadd32			r7, r7.low, #(EURASIA_ISPREGION_SIZEY / 2);
		shl				r7, r7, #EURASIA_PIXELBE1S0_YMIN_SHIFT;
		or				r0, r0, r7;

		/* YMAX */
		and				r7, r1, #~EURASIA_PIXELBE1S1_YMAX_CLRMSK;
		and 			r1, r1, #EURASIA_PIXELBE1S1_YMAX_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBE1S1_YMAX_SHIFT;
		iadd32			r7, r7.low, #(EURASIA_ISPREGION_SIZEY / 2);
		shl				r7, r7, #EURASIA_PIXELBE1S1_YMAX_SHIFT;
		or				r1, r1, r7;
#endif
		br PEST_EndDirections;
	}
	PEST_DirectionNotY:
	{
		/* move the fbaddr*/
#if defined(SGX_FEATURE_UNIFIED_STORE_64BITS)
		and				r7, r1, #~EURASIA_PIXELBES0HI_FBADDR_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBES0HI_FBADDR_SHIFT;
#else
		and				r7, r3, #~EURASIA_PIXELBE2S1_FBADDR_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBE2S1_FBADDR_SHIFT;
#endif
		shl				r7, r7, #EURASIA_PIXELBE_FBADDR_ALIGNSHIFT;
		and				r8, r6, #PEST_ADDR_SHIFT_MASK;
		imae			r7, r8.low, #1, r7, u32;
		shr				r7, r7, #EURASIA_PIXELBE_FBADDR_ALIGNSHIFT;
#if defined(SGX_FEATURE_UNIFIED_STORE_64BITS)
		shl				r7, r7, #EURASIA_PIXELBES0HI_FBADDR_SHIFT;
		and				r1, r1, #EURASIA_PIXELBES0HI_FBADDR_CLRMSK;
		or				r1, r1, r7; 
#else
		shl				r7, r7, #EURASIA_PIXELBE2S1_FBADDR_SHIFT;
		and				r3, r3, #EURASIA_PIXELBE2S1_FBADDR_CLRMSK;
		or				r3, r3, r7;
#endif

#if defined(SGX_FEATURE_UNIFIED_STORE_64BITS)
		/* XMIN */
		and				r7, r12, #~EURASIA_PIXELBES2LO_XMIN_CLRMSK;
		and 			r12, r12, #EURASIA_PIXELBES2LO_XMIN_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBES2LO_XMIN_SHIFT;
		iadd32			r7, r7.low, #(EURASIA_ISPREGION_SIZEX / 2);
		shl				r7, r7, #EURASIA_PIXELBES2LO_XMIN_SHIFT;
		or				r12, r12, r7;

		/* XMAX */
		and				r7, r13, #~EURASIA_PIXELBES2HI_XMAX_CLRMSK;
		and 			r13, r13, #EURASIA_PIXELBES2HI_XMAX_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBES2HI_XMAX_SHIFT;
		iadd32			r7, r7.low, #(EURASIA_ISPREGION_SIZEX / 2);
		shl				r7, r7, #EURASIA_PIXELBES2HI_XMAX_SHIFT;
		or				r13, r13, r7;
#else
		/* XMIN */
		and				r7, r0, #~EURASIA_PIXELBE1S0_XMIN_CLRMSK;
		and 			r0, r0, #EURASIA_PIXELBE1S0_XMIN_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBE1S0_XMIN_SHIFT;
		iadd32			r7, r7.low, #(EURASIA_ISPREGION_SIZEX / 2);
		shl				r7, r7, #EURASIA_PIXELBE1S0_XMIN_SHIFT;
		or				r0, r0, r7;

		/* XMAX */
		and				r7, r1, #~EURASIA_PIXELBE1S1_XMAX_CLRMSK;
		and 			r1, r1, #EURASIA_PIXELBE1S1_XMAX_CLRMSK;
		shr				r7, r7, #EURASIA_PIXELBE1S1_XMAX_SHIFT;
		iadd32			r7, r7.low, #(EURASIA_ISPREGION_SIZEX / 2);
		shl				r7, r7, #EURASIA_PIXELBE1S1_XMAX_SHIFT;
		or				r1, r1, r7;
#endif
	}
	PEST_EndDirections:

	/* clear NOADVANCE */
#if defined(SGX_FEATURE_UNIFIED_STORE_64BITS)
	/* despite the name it's a CLRMSK*/
	and					r13, r13, #EURASIA_PIXELBES2HI_NOADVANCE_MASK;
#else
	and					r1, r1, #EURASIA_PIXELBE1S1_NOADVANCE_MASK;
#endif

	/* now kick the PBE with noadvance low, .freep */
	WAITFOROTHERPIPE(PEST_2Pipes_Second);

#if ! defined(SGX_FEATURE_UNIFIED_STORE_64BITS)
	/* kick the PBE*/
	emitpix1.freep			r0, r1;
#endif

	PixelEventSubTwiddled_Sideband_2Pipes_Second_Byte:
	nop; /*emitpix2 or emitpix with 64 bit US*/

	/* flip the pipe mutex */
	FLIPFLOP;

	nop.end;
#endif
}
SubTwiddledEnd:

/*****************************************************************************
 End of file (subtwiddled_eot.asm)
*****************************************************************************/
