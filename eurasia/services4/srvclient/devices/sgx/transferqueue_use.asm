/*****************************************************************************
 Name		: transferqueue_use.asm

 Title		: USE programs for TQ 

 Author 	: PowerVR

 Created  	: 03/19/2008

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

 Description 	: USE programs for different TQ shaders blits etc. 

 Program Type	: USE assembly language

 Version 	: $Revision: 1.59 $

 Modifications	:

 $Log: transferqueue_use.asm $

 *****************************************************************************/

#include "sgxdefs.h"

.export NormalCpyBlit;
.export NormalCpyBlitEnd; 
#if ! defined(SGX_FEATURE_USE_VEC34)
.export StrideBlit;
.export StrideBlitEnd; 
.export StrideHighBlit;
.export StrideHighBlitEnd; 
#endif
.export SrcBlendBlit;
.export SrcBlendBlitEnd;
.export AccumSrcBlendBlit;
.export AccumSrcBlendBlitEnd;
.export PremulSrcBlendBlit;
.export PremulSrcBlendBlitEnd;
.export GlobalBlendBlit;
.export GlobalBlendBlitEnd;
.export PremulSrcWithGlobalBlendBlit;
.export PremulSrcWithGlobalBlendBlitEnd;
.export A2R10G10B10Blit;
.export A2R10G10B10BlitEnd;
.export A2B10G10R10Blit;
.export A2B10G10R10BlitEnd;
#if !defined(SGX_FEATURE_USE_VEC34)
.export sRGBBlit;
.export sRGBBlitEnd;
#endif
#if defined(EURASIA_PDS_DOUTT0_CHANREPLICATE)
.export A8Blit;
.export A8BlitEnd;
#endif
.export FillBlit;
.export FillBlitCommand;
.export FillBlitEnd;
.export SecondaryUpdate;
.export SecondaryUpdateEnd;
.export SecondaryUpdateDMA;
.export SecondaryUpdateDMAEnd;
/* disable colour luts if we have it in hw */
#if ! defined(SGX_FEATURE_USE_VEC34)
.export Colour256LUTBlit;
.export Colour256LUTBlitEnd;
.export Colour16LUTBlit;
.export Colour16LUTBlitEnd;
.export Colour2LUTBlit;
.export Colour2LUTBlitEnd;
#endif

.export SourceColourKeyBlit;
.export SourceColourKeyBlitEnd;
.export DestColourKeyBlit;
.export DestColourKeyBlitEnd;

.export VideoProcessBlit3Planar;
.export VideoProcessBlit3PlanarEnd;
.export VideoProcessBlit2Planar;
.export VideoProcessBlit2PlanarEnd;
.export VideoProcessBlit2Planar;
.export VideoProcessBlit2PlanarEnd;
.export VideoProcessBlitPacked;
.export VideoProcessBlitPackedEnd;
.export	ClearTypeBlendGamma;
.export	ClearTypeBlendGammaEnd;
.export	ClearTypeBlendInvalidGamma;
.export	ClearTypeBlendInvalidGammaEnd;

.export ARGB2NV12YPlane;
.export ARGB2NV12YPlaneEnd;
.export ARGB2NV12UVPlane;
.export ARGB2NV12UVPlaneEnd;
 

#if defined(SGX_FEATURE_USE_UNLIMITED_PHASES)
	/* No following phase. */
#define PHASE	phas	#0, #0, pixel, end, parallel;
#else
#define PHASE
#endif

.skipinvon;

/*
 * primary		: 0
 * secondary	: -
 * temporary	: -
 */
NormalCpyBlit:
{
	PHASE;
	mov.end o0, pa0;
}
NormalCpyBlitEnd:

#if ! defined(SGX_FEATURE_USE_VEC34)

/*
 * primary		: 0..1
 * secondary	: 0..EURASIA_TAG_TEXTURE_STATE_SIZE
 * temporary	: 0..6
 */
StrideBlit:
{
	nop; nop; nop; nop;
	PHASE;

	frcp		r4, r4;
	fmad		r3, r3, r4, c48;
	ffrc		r4, pa1; 
	fmad		r1, r4, r2, c48;
	fsubflr		r5, r1, r1; 
	fmad		r1, r1, c52, r5.neg; 
	ffrc		r4, pa0;
	fmad		r1, r1, c52, r4; 
	fmad		r1, r1, r3, c48; 
	fsubflr		r0, r1, r1; 
	fmad		r1, r1, c52, r0.neg; 
	fmad		r1, r1, c52, r5; 
	frcp		r6, r6; 
	fmad		r1, r1, r6, c48; 

.skipinvoff;
	smp2d		r0, r0, sa0, drc0; 
.skipinvon;
	wdf			drc0; 
	mov.end		o0, r0; 
}
StrideBlitEnd:


/*
 * primary		: 0..1
 * secondary	: 0..EURASIA_TAG_TEXTURE_STATE_SIZE
 * temporary	: 0..9
 */
StrideHighBlit:
{
	nop; nop; nop;
	nop; nop; nop;
	PHASE;

	frcp		r4, r4; 
	fmad		r1, pa1, r2, c48; 
	fsubflr		r5, r1, r1; 
	fmad		r1, r1, c52, r5.neg; 
	fmad		r1, r1, c52, pa0; 
	fmad		r1, r1, r3, c48; 
	fsubflr		r9, r1, r1; 
	fmad		r1, r1, c52, r9.neg; 
	fmad		r1, r1, r7, r8; 
	fmad		r1, r1, r4, c48; 
	fmad		r9, r9, r4, c48; 
	frcp		r4, r6; 
	fmad		r3, r3, r4, c48; 
	fsubflr		r0, r1, r1; 
	fmad		r1, r1, c52, r0.neg; 
	fmad		r0, r0, c52, r9; 
	fmad		r1, r1, c52, r5; 
	/* this instr compiles a bit different - investigate*/
	fmad		r1, r1, r3, c48; 
.skipinvoff;
	smp2d		r0, r0,	sa0, drc0; 
.skipinvon;
	wdf			drc0; 
	mov.end		o0, r0;
}
StrideHighBlitEnd:

#endif
#if defined(SGX_FEATURE_USE_VEC34)

ClearTypeBlendInvalidGamma:
{
	nop.end;
}
ClearTypeBlendInvalidGammaEnd:

ClearTypeBlendGamma:
{
	nop.end;
}
ClearTypeBlendGammaEnd:

#else

ClearTypeBlendInvalidGamma:
{
		PHASE;

		/* pa1.0 - Dest.b, pa1.1 - Dest.g, pa1.2 - Dest.r, pa1.3 - Dest.a */

		/* r0	<-A.r or A.g from pa0 */

		/* (Color.r >= D.r ? A.r : A.g) */
		unpckf32u8			r0, pa1.2;
		unpckf32u8			r1, sa0.2;
		fsub.tp|z			r1, p0, r1, r0;
(p0)	pckf32u8			r2, pa0.2;
(!p0)	pckf32u8			r2, pa0.1;
		fmul				r2, r2, r1;
		mov					r1, #0x3B808081;
		fmad 				r2, r2, r1, r0;
		pcku8f32			o0.0100, r2;		


		
		/* (Color.g >= D.g ? A.r : A.g) */
		unpckf32u8			r0, pa1.1;
		unpckf32u8			r1, sa0.1;
		fsub.tp|z			r1, p0, r1, r0;
(p0)	pckf32u8			r2, pa0.2;
(!p0)	pckf32u8			r2, pa0.1;
		fmul				r2, r2, r1;
		mov					r1, #0x3B808081;
		fmad				r2, r2, r1, r0;
		pcku8f32			o0.0010, r2;		


		
		/* (Color.b >= D.b ? A.r : A.g) */
		unpckf32u8			r0, pa1.0;
		unpckf32u8			r1, sa0.0;
		fsub.tp|z			r1, p0, r1, r0;
(p0)	pckf32u8			r2, pa0.2;
(!p0)	pckf32u8			r2, pa0.1;
		fmul				r2, r2, r1;
		mov					r1, #0x3B808081;
		fmad				r2, r2, r1, r0;
		pcku8f32			o0.0001, r2;
	
		pcku8u8.end			o0.1000, pa1.3;	

	
}
ClearTypeBlendInvalidGammaEnd:


ClearTypeBlendGamma:
{
		PHASE;
		/* pa1.0 - Dest.b, pa1.1 - Dest.g, pa1.2 - Dest.r, pa1.3 - Dest.a */
		/* Unpack Destination */

		mov				r0, #0x00010000;
		pcku8u8			r0.0001, pa1.2;						/* r0.0 = Dest.r */
		ldab			r3, [sa2, r0], drc0; 				/* r3.0 = Tmp.r = GammaTable[Dest.r] */

		mov				r1, #0x00010000;		
		pcku8u8			r1.0001, pa1.1;						/* r1.0 = Dest.g */
		mov				r2, #0x00010000;
		pcku8u8			r2.0001, pa1.0;						/* r2.0 = Dest.b */

		wdf 			drc0;
		

		ldab			r4, [sa2, r1], drc0; 				/* r4 = Tmp.g = GammaTable[Dest.g] */
		pcku8u8			r6.0100, r3.0;						/* r6.2 = Tmp.r */
		sop3			r3, pa0, sa0, r6, s0, 1-s0, add;	/* r3 = round((Tmp.r + (Color.r - Tmp.r) * A.r / 255.0)) */
		+alrp			one, one;							/* r6.2 = Tmp.r, sa0 = Color */
		mov				r0, #0x00010000;
		pcku8u8			r0.0001, r3.2;				
		ldab			r3, [sa3, r0], drc1;				/* r3 = BlendColor.r = InverseGammaTable[round((Tmp.r + (Color.r - Tmp.r) * A.r / 255.0))] */

		wdf 			drc0;


		ldab			r5, [sa2, r2], drc0;				/* r5 = Tmp.b = GammaTable[Dest.b]; */
		pcku8u8			r6.0010, r4.0;						/* r6.1 = Tmp.g */
		sop3			r4, pa0, sa0, r6, s0, 1-s0, add;	/* r4 = round((Tmp.g + (Color.g - Tmp.g) * A.g / 255.0)) */
		+alrp			one, one;							/* r6.1 = Tmp.g, sa0 = Color */
		mov				r1, #0x00010000;
		pcku8u8			r1.0001, r4.1;			
		ldab			r4, [sa3, r1], drc1; 				/* r4 = BlendColor.b = InverseGammaTable[round((Tmp.b + (Color.b - Tmp.b) * A.b / 255.0))] */

		wdf				drc0;

		pcku8u8			r6.0001, r5.0;						/* r6.0 = Tmp.b */
		sop3			r5, pa0, sa0, r6, s0, 1-s0, add;	/* r5 = round((Tmp.b + (Color.b - Tmp.b) * A.b / 255.0)) */
		+alrp			one, one;							/* r6.0 = Tmp.b, sa0 = Color */
		mov				r2, #0x00010000;
		pcku8u8			r2.0001, r5.0;	
		ldab			r5, [sa3, r2], drc1;				/* r5 = BlendColor.b = InverseGammaTable[round((Tmp.b + (Color.b - Tmp.b) * A.b / 255.0))] */


		/* Unpack Alpha (A.r -> r3, A.g -> r4, A.b -> r5 */
		mov				r6, #0xFF;							/* r6 = 0xFF */
		and				r9, pa0, r6;						/* r9 = pa0 & 0xFF */
		shr				r8, pa0, #8;						/* r8 = pa0 >> 8 */
		and				r8, r8, r6;							/* r8 = r8 & 0xFF */
		shr				r7, pa0, #16;						/* r7 = pa0 >> 16 */
		and				r7, r7, r6;							/* r7 = r7 & 0xFF */

		/* Test for Red */
		mov				r6, #0xFF;
		and.tnz			r1, p0, r7, r6;						/* P0 = (A.r & 0xFF != 0) */
		xor.tz			r1, p1, r7, r6;						/* P1 = (A.r ^ 0xFF == 0) */

		wdf 			drc1;
	
		/* Pack the Blend Color (B -> r0) */
		/* r0.0 = r5.0, r0.1 = r4.0, r0.2 = r3.0 */
//		pcku8u8			r0.0101, r5.0, r3.0;	// r0 = (r3 << 16) | (r4 << 8) | r5
//		pcku8u8			r0.0010, r4.0;

(!p1)	pcku8u8			o0.0100, r3.0;						/* o0.r = BlendColor.r */
(p1)	pcku8u8			o0.0100, sa1.2;						/* o0.r = Color.r */
(!p0)	pcku8u8			o0.0100, pa1.2;						/* o0.r = Dest.r */

		/* Test for Green */

		and.tnz			r1, p0, r8, r6;						/* P0 = (A.g & 0xFF != 0) */
		xor.tz			r1, p1, r8, r6;						/* P1 = (A.g ^ 0xFF == 0) */
(!p1)	pcku8u8			o0.0010, r4.0;						/* o0.r = BlendColor.g */
(p1)	pcku8u8			o0.0010, sa1.1;						/* o0.r = Color.g */
(!p0)	pcku8u8			o0.0010, pa1.1;						/* o0.r = Dest.g */

		/* Test for blue */

		and.tnz			r1, p0, r9, r6;						/* P0 = (A.b & 0xFF != 0) */
		xor.tz			r1, p1, r9, r6;						/* P1 = (A.b ^ 0xFF == 0) */
(!p1)	pcku8u8			o0.0001, r5.0;						/* o0.r = BlendColor.b */
(p1)	pcku8u8			o0.0001, sa1.0;						/* o0.r = Color.b */
(!p0)	pcku8u8			o0.0001, pa1.0;						/* o0.r = Dest.b */

		pcku8u8.end		o0.1000, pa1.3;						/* o0.a = Dest.a */
}
ClearTypeBlendGammaEnd:

#endif

SrcBlendBlit:
{
	PHASE;

	/* SGXTQ_ALPHA_SOURCE */
	sop2.end    o0, pa0, pa1, s1a, s1a.comp, add;     /* Cdst = Csrc*Asrc + Cdst*(1-Asrc) */
	+asop2		one, s1a.comp, add;                   /* Adst = ASrc + Adst*(1-Asrc) */
}
SrcBlendBlitEnd:


/* same as SrcBlendBlit, but blend against the accumlation object
 *
 * primaries   : pa0
 * secondaries : -
 * temps       : -
 */
AccumSrcBlendBlit:
{
	PHASE;

	/* SGXTQ_ALPHA_ACCUM_SOURCE */
	sop2.end    o0, pa0, o0, s1a, s1a.comp, add;     /* Cdst = Csrc*Asrc + Cdst*(1-Asrc) */
		+asop2		one, s1a.comp, add;              /* Adst = ASrc + Adst*(1-Asrc) */
}
AccumSrcBlendBlitEnd:


PremulSrcBlendBlit:
{
	PHASE;

	/* SGXTQ_ALPHA_PREMUL_SOURCE : Cdst = Csrc + Cdst*(1-Asrc) */
	sopwm.end o0.bytemask1111, pa0, pa1, zero.comp, s1a.comp, add, add;
}
PremulSrcBlendBlitEnd:


GlobalBlendBlit:
{
	PHASE;

	/* SGXTQ_ALPHA_GLOBAL : Cdst = Csrc*Aglob + Cdst*(1-Aglob) */
	sop3.end o0, pa0, pa1, sa0, s2.comp, s0, add;
		+asop s2a.comp, add;
}
GlobalBlendBlitEnd:

PremulSrcWithGlobalBlendBlit:
{
	PHASE;

	/* SGXTQ_ALPHA_PREMUL_SOURCE_WITH_GLOBAL : Cdest = Aglob*Csrc + (1-(Aglob*Asrc))*Cdest */
	sop3 r0, pa0, pa1, sa0, zero, s0, add;
		+alrp zero, s2a;
	sopwm.end o0.bytemask1111, r0, pa1, one, s1a.comp, add, add;
}
PremulSrcWithGlobalBlendBlitEnd:

A2R10G10B10Blit:
{
	PHASE;
	shr			r0, pa0, #0x00000006;
	and			o0, r0, #0x000003FF;
	shr			r0, pa0, #0x0000000C;
	and			r0, r0, #0x000FFC00;
	or			o0, o0, r0;
	shl			r0, pa1, #0x0000000E;
	and			r0, r0, #0x3FF00000;
	or			o0, o0, r0;
	and			r0, pa1, #0xC0000000;
	or.end		o0, r0, r0;
}
A2R10G10B10BlitEnd:
	
A2B10G10R10Blit:
{
	PHASE;
	shl			r0, pa0, #0x00000010;
	and			o0, r0 , #0xFFFF0000;
	shr			r0, pa0, #0x00000014;
	and			r0, r0 , #0x000000FF;
	or			o0, o0 , r0;
	shr			r0, pa0, #0x00000002;
	and			r0, r0 , #0x0000FF00;
	or.end		o0, o0 , r0;	
}
A2B10G10R10BlitEnd:

#if ! defined(SGX_FEATURE_USE_VEC34)
sRGBBlit:
{
	PHASE;
	/* Our constants */
	mov			r0, #0x3b4d2e1c; /* 0.0031308 */
	mov			r1, #0x414eb852; /* 12.92 */
	mov			r2, #0x3ed55555; /* (1.0) / (2.4) */
	mov			r3, #0x3f870a3d; /* 1.055 */
	mov			r4, #0xbd6147ae; /* -0.055 */

	/* Unpack each channel, convert, and pack each channel in turn */

	/* R channel */
	unpckf32u8.scale r5, pa0.0;
	fsub.tp|z p0, r5, r0; /* Less than 0.0031308 */
(!p0) fmad r5, r5, r1, c48; /* If less than 0.0031308, multiply by 12.92 */
(p0) flog r5, r5; /* Otherwise, the long way - take the log */
(p0) fmad r5, r5, r2, c48; /* divide by 2.4 */
(p0) fexp r5, r5; /* Expontentialise */
(p0) fmad r5, r5, r3, r4; /* Multiply by 1.055 and subtract 0.055 */
	pcku8f32.scale o0.0001, r5, r5;

	/* G channel */
	unpckf32u8.scale r5, pa0.1;
	fsub.tp|z p0, r5, r0; /* Less than 0.0031308 */
(!p0) fmad r5, r5, r1, c48; /* If less than 0.0031308, multiply by 12.92 */
(p0) flog r5, r5; /* Otherwise, the long way - take the log */
(p0) fmad r5, r5, r2, c48; /* divide by 2.4 */
(p0) fexp r5, r5; /* Expontentialise */
(p0) fmad r5, r5, r3, r4; /* Multiply by 1.055 and subtract 0.055 */
	pcku8f32.scale o0.0010, r5, r5;

	/* B channel */
	unpckf32u8.scale r5, pa0.2;
	fsub.tp|z p0, r5, r0; /* Less than 0.0031308 */
(!p0) fmad r5, r5, r1, c48; /* If less than 0.0031308, multiply by 12.92 */
(p0) flog r5, r5; /* Otherwise, the long way - take the log */
(p0) fmad r5, r5, r2, c48; /* divide by 2.4 */
(p0) fexp r5, r5; /* Expontentialise */
(p0) fmad r5, r5, r3, r4; /* Multiply by 1.055 and subtract 0.055 */
	pcku8f32.scale o0.0100, r5, r5;

	/* A channel */
	unpckf32u8.scale r5, pa0.3;
	fsub.tp|z p0, r5, r0; /* Less than 0.0031308 */
(!p0) fmad r5, r5, r1, c48; /* If less than 0.0031308, multiply by 12.92 */
(p0) flog r5, r5; /* Otherwise, the long way - take the log */
(p0) fmad r5, r5, r2, c48; /* divide by 2.4 */
(p0) fexp r5, r5; /* Expontentialise */
(p0) fmad r5, r5, r3, r4; /* Multiply by 1.055 and subtract 0.055 */
	pcku8f32.scale.end o0.1000, r5, r5;
}
sRGBBlitEnd:
#endif

#if defined(EURASIA_PDS_DOUTT0_CHANREPLICATE)
/* primary		: pa0
 * secondary	: -
 * temporary	: -
 */
A8Blit:
{
	shr.end		o0, pa0, #24;
}A8BlitEnd:
#endif

/* primary		: -
 * secondary	: -
 * temporary	: -
 */
FillBlit:
{
	PHASE;
FillBlitCommand:
	/* The correct colour is patched in when the transfer command is queued */
	/* mov.end o0, #colour   --- check for .skipinv! */
	nop.end;
}
FillBlitEnd:


/* Some versions of the windows compiler do not support VARARG macros.
 * So as a nasty hack workaround, define _ and as ,  and use _
 */
#define _ ,

#define MAKEFILLROP(name, commands )											\
.export FillRop##name##Blit;													\
.export FillRop##name##BlitEnd;													\
FillRop##name##Blit:;															\
{;																				\
	PHASE;																		\
	commands;																	\
};																				\
FillRop##name##BlitEnd:;

/* Setup the Fill rops.  These are asm programs to do a solid fill in a certain way.
 * pa0 is the destination and sa0 the color passed in 
 *
 * Note that the first parameter, the name, is anything unique to identify the blit.
 * The label of the asm program becomes, for example, FillRopNotAndBlit
 */
MAKEFILLROP(NotAndNot,   xor			o0 _ sa0 _ #0xFFFFFFFF;  				\
						 and.end		o0 _ o0 _ ~pa0; )             /*  Rop=0x05 :        NOT pat AND NOT dst */
MAKEFILLROP(NotAnd,      and.end        o0 _ pa0 _ ~sa0 ; )           /*  Rop=0x0A :        NOT pat AND dst */
MAKEFILLROP(Not,         xor.end        o0 _ sa0 _ #0xFFFFFFFF; )     /*  Rop=0x0F :        NOT pat */
MAKEFILLROP(AndNot,      and.end        o0 _ sa0 _ ~pa0; )            /*  Rop=0x50 :        pat AND NOT dst */
MAKEFILLROP(Xor,         xor.end        o0 _ sa0 _ pa0; )             /*  Rop=0x5A :        pat XOR dst */
MAKEFILLROP(NotOrNot,    xor            o0 _ sa0 _ #0xFFFFFFFF;  				\
						 or.end         o0 _ o0 _ ~pa0; )             /*  Rop=0x5F :        NOT pat OR NOT dst */
MAKEFILLROP(And,         and.end        o0 _ sa0 _ pa0; )             /*  Rop=0xA0 :        pat AND dst */
MAKEFILLROP(NotXor,      xor.end        o0 _ pa0 _ ~sa0; )            /*  Rop=0xA5 :        NOT pat XOR dst */
MAKEFILLROP(NotOr,       or.end         o0 _ pa0 _ ~sa0; )            /*  Rop=0xAF :        NOT pat OR dst */
MAKEFILLROP(OrNot,       or.end         o0 _ sa0 _ ~pa0; )            /*  Rop=0xF5 :        pat OR NOT dst */
MAKEFILLROP(Or,          or.end         o0 _ pa0 _ sa0; )             /*  Rop=0xFA :        pat OR dst */
MAKEFILLROP(NotD,		 xor.end		o0 _ pa0 _ #0xFFFFFFFF; )	  /*  Rop=0x55 :		Invert dst */


#define MAKEROP(name, commands)													\
.export Rop##name##Blit;														\
.export Rop##name##BlitEnd;														\
Rop##name##Blit:;																\
{;																				\
	PHASE;																		\
	commands;																	\
};																				\
Rop##name##BlitEnd:;

/* Rop=0x11 :		NOT src AND NOT dst */
MAKEROP(NotSAndNotD,	xor						r0 _ pa0 _ #0xFFFFFFFF; \
						and.end					o0 _ r0 _ ~pa1; )

/* Rop=0x22 :		NOT src AND dst */
MAKEROP(NotSAndD,		and.end					o0 _ pa1 _ ~pa0; )

/* Rop=0x33 :		NOT src */
MAKEROP(NotS,			xor.end					o0 _ pa0 _ #0xFFFFFFFF; )

/* Rop=0x44 :		src AND NOT dst */
MAKEROP(SAndNotD,		and.end					o0 _ pa0 _ ~pa1; )

/* Rop=0x55 :		NOT dst */
MAKEROP(NotD,			xor.end					o0 _ pa1 _ #0xFFFFFFFF; )

/* Rop=0x66 :		src XOR dst */
MAKEROP(SXorD,			xor.end					o0 _ pa0 _ pa1; )

/* Rop=0x77 :		NOT src OR NOT dst */
MAKEROP(NotSOrNotD,		xor						r0 _ pa0 _ #0xFFFFFFFF; \
						or.end					o0 _ r0 _ ~pa1; )

/* Rop=0x88 :		src AND dst */
MAKEROP(SAndD,			and.end					o0 _ pa0 _ pa1; )

/* Rop=0x99 :		NOT src XOR dst */
MAKEROP(NotSXorD,		xor.end					o0 _ pa1 _ ~pa0; )

/* Rop=0xAA :		Dst = Dst (not used) */
MAKEROP(D,				mov.end					o0 _ pa1; )

/* Rop=0xBB :		NOT src OR dst */
MAKEROP(NotSOrD,		or.end					o0 _ pa1 _ ~pa0; )

/* Rop=0xCC			Dst = Src */
MAKEROP(S,				mov.end					o0 _ pa0; )

/* Rop=0xDD :		src OR NOT dst */
MAKEROP(SOrNotD,		or.end					o0 _ pa0 _ ~pa1; )

/* Rop=0xEE :		src OR dst */
MAKEROP(SOrD,			or.end					o0 _ pa1 _ pa0; )

#undef _


SecondaryUpdate:
{
	PHASE;

	nop.end;
}
SecondaryUpdateEnd:


SecondaryUpdateDMA:
{
	PHASE;

#if defined(FIX_HW_BRN_31988)
	/* predicated load of size 128-bits */

	and.testz		p0, #1, #1;
	p0	ldad.f4		pa124,	[pa0, #1++], drc0;
	wdf				drc0;
#endif

	nop.end;
}
SecondaryUpdateDMAEnd:


#if ! defined(SGX_FEATURE_USE_VEC34)

Colour256LUTBlit:
{
	PHASE;

	xor		r1, r1, r1; 
	pckf32u8.scale	r0, pa0, pa0;
.skipinvoff;
	smp2d		r0, r0, sa0, drc0; 
.skipinvon;
	wdf		drc0; 
	mov.end		o0, r0; 
}
Colour256LUTBlitEnd:

#define FLOAT32_HALF					0x3f000000
#define FLOAT32_TWO					0x40000000
#define FLOAT32_ONEEIGHTH				0x3e000000
#define FLOAT32_EIGTH					0x41000000
#if (EURASIA_TAG_TEXTURE_STATE_SIZE == 4)
#define SA_WIDTH_SA					sa4
#define SA_LUT_OFFSET					5
#else
#define SA_WIDTH_SA					sa3
#define SA_LUT_OFFSET					4
#endif

Colour16LUTBlit:
{
	PHASE;

	/* obviously this can't wrap and such*/
	fmul			r0, pa0, SA_WIDTH_SA;
	mov			r1, #(FLOAT32_HALF);
	fmul			r0, r0, r1; 
	ffrc			r2, r0;
	fsub			r0, r0, r2; /* go to the left side of the px*/
	fadd			r0, r0, r1; /* position in the middle */
	fmul			r1, r1, SA_WIDTH_SA;
	frcp			r1, r1; 
	fmul			r0, r0, r1;
	mov			r1, pa1;

.skipinvoff;
	smp2d			r0, r0, sa0, drc0; 
.skipinvon;

	/* calculate the bytesel while waiting for the smp2d*/
	mov			r1, #(FLOAT32_TWO);
	fmul			r2, r2, r1; 
	pckf32u8.scale	r2, r2, r2;
	mov			r1, #0x80;
	wdf drc0;
	and.testz		p0, r2, r1; 
	p0 shr			r0, r0, #4;
	and			il, r0, #0xf;
	
	mov.end			o0, sa[SA_LUT_OFFSET + il]; 
}
Colour16LUTBlitEnd:

Colour2LUTBlit:
{
	PHASE;

	/* obviously this can't wrap and such*/
	fmul			r2, pa0, SA_WIDTH_SA;
	mov			r1, #(FLOAT32_ONEEIGHTH);
	fmul			r0, r2, r1; 
	ffrc			r1, r0;
	fsub			r0, r0, r1; /* go to the left side of the px*/
	mov			r1, #(FLOAT32_HALF);
	fadd			r0, r0, r1; /* position in the middle */
	mov			r1, #(FLOAT32_EIGTH);
	fmul			r0, r0, r1;
	mov			r1, SA_WIDTH_SA;
	frcp			r1, r1; 
	fmul			r0, r0, r1;
	mov			r1, pa1;

.skipinvoff;
	smp2d			r0, r0, sa0, drc0; 
.skipinvon;

	/* calculate the bytesel while waiting for the smp2d*/
	ffrc    		r1, r2;
	fsub			r2, r2, r1;
	pcku16f32		r2, r2, #0;
	and				r2, r2, #0x7; /* pcku8f32 replicates the result from bit 16*/
	mov				r1, #8;
	xor				r2, r2, #0xffffffff;
	iadd32			r2, r1.low, r2;
	wdf drc0;
	shr			r0, r0, r2; 
	and			il, r0, #1;
	
	mov.end			o0, sa[SA_LUT_OFFSET + il];
}
Colour2LUTBlitEnd:

#endif /* SGX_FEATURE_USE_VEC34*/


/****************************************************************
 * primary		: 0..1
 * seconady		: 0..1 
 * temporary	: 0
 ****************************************************************/
SourceColourKeyBlit:
{
	PHASE;

	/*	Source colour key
		If the source pixel (pa0) matches key colour (sa0) then output = dest, else output = source */

	xor					r0, pa0, sa0;
	and					r0, r0,  sa1;			/* remove insignificant bits using the colour key mask (sa1) */
#if defined(SGX_FEATURE_USE_VEC34)
	vmovc.i32.testz		o0.xyzw, r0, pa1, pa0, swizzle(xyzw);
#else
	movc.i32.testz		o0, r0, pa1, pa0;
#endif
	nop.end;
}
SourceColourKeyBlitEnd:


/****************************************************************
 * primary		: 0..1
 * seconady		: 0..1 
 * temporary	: 0
 ****************************************************************/
DestColourKeyBlit:
{
	PHASE;

	/*	Dest colour key
		If the destination pixel (pa1) matches the key colour (sa0) then output = source else output = dest */

	xor					r0, pa1, sa0;
	and					r0, r0,  sa1;			/* remove insignificant bits using the colour key mask (sa1) */
#if defined(SGX_FEATURE_USE_VEC34)
	vmovc.i32.testz		o0.xyzw, r0, pa0, pa1, swizzle(xyzw);
#else
	movc.i32.testz		o0, r0, pa0, pa1;
#endif
	nop.end;
}
DestColourKeyBlitEnd:


/****************************************************************
 * primary		: 0..2
 * seconady		: -
 * temporary	: -
 ****************************************************************/
.align 2; /* let's start with a clean pairing */
VideoProcessBlit3Planar:
{
	PHASE;
.align 2; /* PHASE might / might not move us to even instruction alignment*/

	nop.nosched;

#if defined(SGX_FEATURE_USE_NO_INSTRUCTION_PAIRING)
	nop;
#endif

	firh.nosched    o0, pa0, pa1, pa2, u8, msc, fcs0, #5, #2;
	firh        	o0, pa0, pa1, pa2, u8, msc, fcs1, #5, #1;
	firh.end    	o0, pa0, pa1, pa2, u8, msc, fcs2, #5, #0;
}
VideoProcessBlit3PlanarEnd:


/****************************************************************
 * primary		: 0..1
 * seconady		: -
 * temporary	: -
 ****************************************************************/
.align 2; /* let's start with a clean pairing */
VideoProcessBlit2Planar:
{
	PHASE;
.align 2; /* PHASE might / might not move us to even instruction alignment*/

	nop.nosched;

#if defined(SGX_FEATURE_USE_NO_INSTRUCTION_PAIRING)
	nop;
#endif

	firh.nosched	o0, pa0, pa1, pa1, u8, msc, fcs0, #5, #2;
	firh        	o0, pa0, pa1, pa1, u8, msc, fcs1, #5, #1;
	firh.end    	o0, pa0, pa1, pa1, u8, msc, fcs2, #5, #0;
}
VideoProcessBlit2PlanarEnd:



/****************************************************************
 * primary		: 0 
 * seconady		: -
 * temporary	: -
 ****************************************************************/
.align 2; /* let's start with a clean pairing */

VideoProcessBlitPacked:
{
	PHASE;
.align 2; /* PHASE might / might not move us to even instruction alignment*/

	nop.nosched;

#if defined(SGX_FEATURE_USE_NO_INSTRUCTION_PAIRING)
	nop;
#endif

	firh.nosched    o0, pa0, pa0, pa0, u8, msc, fcs0, #5, #2;
	firh        	o0, pa0, pa0, pa0, u8, msc, fcs1, #5, #1;
	firh.end    	o0, pa0, pa0, pa0, u8, msc, fcs2, #5, #0;
}
VideoProcessBlitPackedEnd:


/****************************************************************
 * primary		: 0 
 * seconady		: -
 * temporary	: -
 ****************************************************************/
ARGB2NV12YPlane:
{
	PHASE;

	firh.end    o0, pa0, pa0, pa0, u8, msc, fcs2, #5, #3;
}
ARGB2NV12YPlaneEnd:


/****************************************************************
 * primary		: 0 
 * seconady		: -
 * temporary	: -
 ****************************************************************/
ARGB2NV12UVPlane:
{
    PHASE;

#if defined(SGX_FEATURE_USE_NO_INSTRUCTION_PAIRING)
.align 2;
#else
	nop.nosched;
#endif

	firh        o0, pa0, pa0, pa0, u8, msc, fcs1, #7, #0;
	firh.end    o0, pa0, pa0, pa0, u8, msc, fcs2, #5, #1;
}
ARGB2NV12UVPlaneEnd:
