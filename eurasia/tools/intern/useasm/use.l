%{
/*************************************************************************
 * Name         : use.l
 * Title        : Pixel Shader Compiler
 * Author       : David Welch
 * Created      : Jan 2002
 *
 * Copyright    : 2002-2006 by Imagination Technologies Limited. All rights reserved.
 *              : No part of this software, either material or conceptual 
 *              : may be copied or distributed, transmitted, transcribed,
 *              : stored in a retrieval system or translated into any 
 *              : human or computer language in any form by any means,
 *              : electronic, mechanical, manual or other-wise, or 
 *              : disclosed to third parties without the express written
 *              : permission of Imagination Technologies Limited, Unit 8, HomePark
 *              : Industrial Estate, King's Langley, Hertfordshire,
 *              : WD4 8LZ, U.K.
 *
 *
 * Modifications:-
 * $Log: use.l $
 **************************************************************************/

#include <string.h>
#if defined(_MSC_VER)
#include <malloc.h>
#endif
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include <sgxdefs.h>

#include "use.h"
#include "useasm.h"

#include "use.tab.h"

#include "ctree.h"
#include "osglue.h"

#if defined(_MSC_VER)
#pragma warning (disable:4131)
#pragma warning (disable:4127)
#pragma warning (disable:4244)
#pragma warning (disable:4505)
#endif /* defined(_MSC_VER) */

int yylex(void);

IMG_UINT32 g_uSourceLine;
static IMG_PCHAR g_pcSavedInvalidInput = NULL;
static IMG_BOOL g_bEof;

static IMG_PCHAR g_pszCCode = NULL;
static IMG_UINT32 g_uCCodeLength = 0;

void yyeatcomment(void);
void yyeatcppcomment(void);
static IMG_UINT32 ParseMask(IMG_PCHAR pszMask);
static IMG_UINT32 ParseTest(IMG_PCHAR pszTest);
IMG_VOID ParseHashDirective(IMG_PCHAR yytext);
IMG_VOID dumpinvalidinput(void);
void addtoinvalidinput(IMG_CHAR c);

int yzparse(void);
int yz_scan_bytes(char* bytes, int len);
extern int yzdebug;
extern IMG_VOID DumpGlobalTypes(IMG_VOID);

void ParseError(IMG_PCHAR pszFmt, ...)
{
	va_list ap;

	va_start(ap, pszFmt);
	fprintf(stderr, "%s(%u): error: ", g_pszInFileName, g_uSourceLine);
	vfprintf(stderr, pszFmt, ap);
	fprintf(stderr, ".\n");
	va_end(ap);
	g_uParserError = 1;
}

IMG_VOID ParseErrorAt(const IMG_CHAR* pszFileName, IMG_UINT32 uSourceLine, 
					  IMG_PCHAR pszFmt, ...)
{
	va_list ap;

	va_start(ap, pszFmt);
	fprintf(stderr, "%s(%u): error: ", pszFileName, uSourceLine);
	vfprintf(stderr, pszFmt, ap);
	fprintf(stderr, ".\n");
	va_end(ap);
	g_uParserError = 1;
}

#define YY_SKIP_YYWRAP

static int yywrap(void)
{
	return 1;
}

static void InitOpLoc(OPCODE_AND_LINE* psOp)
{
	psOp->uSourceLine = g_uSourceLine;
	psOp->pszSourceFile = g_pszInFileName; 
}

static IMG_UINT32 XyzwToN(IMG_CHAR c)
{
	switch (c)
	{
		case 'x': return USEASM_SWIZZLE_SEL_X;
		case 'y': return USEASM_SWIZZLE_SEL_Y;
		case 'z': return USEASM_SWIZZLE_SEL_Z;
		case 'w': return USEASM_SWIZZLE_SEL_W;
		case '0': return USEASM_SWIZZLE_SEL_0;
		case '1': return USEASM_SWIZZLE_SEL_1;
		case '2': return USEASM_SWIZZLE_SEL_2;
		case 'h': return USEASM_SWIZZLE_SEL_HALF;
		default: IMG_ABORT();
	}
}

static IMG_BOOL IsReservedWord(const IMG_PCHAR pszStringIn)
{
	IMG_UINT32 uEndOfMatchedHead = 0;
	IMG_PCHAR pszString = strdup(pszStringIn);	
	IMG_UINT32 uStrLen = strlen(pszString);
	IMG_UINT32 uI = 0;
	for( ; uI < uStrLen && uI < 3; uI++)
	{
		pszString[uI] = tolower(pszString[uI]);
	}
	if(uStrLen > 3)
	{
		if
		(
			(pszString[0] == 'd' && pszString[1] == 'r' && pszString[2] == 'c')
			||
			(pszString[0] == 'f' && pszString[1] == 'c' && pszString[2] == 's')
		)
		{
			uEndOfMatchedHead = 3;
		}
	}
	if(uStrLen > 2 && uEndOfMatchedHead == 0)
	{
		if
		(
			(pszString[0] == 'p' && pszString[1] == 'a')
			||
			(pszString[0] == 's' && pszString[1] == 'a')
			||
			(pszString[0] == 'c' && pszString[1] == 'p')
		)
		{
			uEndOfMatchedHead = 2;
		}
	}
	if(uStrLen > 1  && uEndOfMatchedHead == 0)
	{
		if
		(
			(pszString[0] == 'r')
			||
			(pszString[0] == 'o')
			||
			(pszString[0] == 'g')
			||
			(pszString[0] == 'c')
			||
			(pszString[0] == 'i')
		)
		{
			uEndOfMatchedHead = 1;
		}
	}
	if((uEndOfMatchedHead > 0) && IsAllDigitsString(&pszString[uEndOfMatchedHead]))
	{
		UseAsm_Free(pszString);
		return IMG_TRUE;
	}
	UseAsm_Free(pszString);
	return IMG_FALSE;
}

static void yyaddtoccode(IMG_PCHAR pszString, IMG_UINT32 uStringLength)
{
	g_pszCCode = realloc(g_pszCCode, g_uCCodeLength + uStringLength);
	UseAsm_MemCopy(g_pszCCode + g_uCCodeLength, pszString, uStringLength);
	g_uCCodeLength += uStringLength;
}

/*
	Returns the appropriate token when a newline is encounted. 
		(i) In C preprocessor mode the newline is ignored.
		(ii) In non-C preprocessor mode the newline is treated as delimited the end of an instruction.
*/
#define NEWLINE_TO_WHITESPACE_OR_DELIMITER	\
	if (!g_bCPreprocessor)					\
	{										\
		return INSTRUCTION_DELIMITER;		\
	}

%}
%option never-interactive
%s BRANCH_INSTRUCTION
%s POSSIBLE_FLOAT_NUMBER
%x UNKNOWN_CHARACTER
%x CCODE
%%

<CCODE>^"\n"				{ yyaddtoccode(yytext, yyleng); }
<CCODE>[^#^\n].*"\n"		{ yyaddtoccode(yytext, yyleng);  }

	/* Handle comment blocks and single line comments. */
"/*"				{ yyeatcomment();  }
"//"				{ yyeatcppcomment(); g_uSourceLine++; BEGIN(INITIAL); NEWLINE_TO_WHITESPACE_OR_DELIMITER; }

<CCODE,INITIAL>^[ \f\t]*"#".*"\n"			{  ParseHashDirective(yytext); }

	/* PROC keyword management */	
"PROC"	{	
			if(IsScopeManagementEnabled() == IMG_TRUE)
			{
				return PROC;
			}
			else
			{
				REJECT;
			}
		}

	/* Scope resolution operator management */
[[:alpha:]_][[:alnum:]_]*"::"	{	if(IsScopeManagementEnabled() == IMG_TRUE)
									{
										yylval.sOp.pszCharString = strdup(yytext); return SCOPE_NAME;
									}
									else
									{
										REJECT;
									}
								}

	/* Hex number */
"0x"([0-9]|[A-F]|[a-f])+U?L?L?	{ BEGIN(INITIAL); yylval.n = strtoul(yytext, NULL, 16); return NUMBER; };
	
	/* Floating point opcodes. */
"fmad"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FMAD; return OPCODE; }
"fadm"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FADM; return OPCODE; }
"fmsa"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FMSA; return OPCODE; }
"ffrc"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FFRC; return OPCODE; }
"frcp"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FRCP; return OPCODE; }
"frsq"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FRSQ; return OPCODE; }
"flog"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FLOG; return OPCODE; }
"fexp"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FEXP; return OPCODE; }
"fsqrt"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FSQRT; return OPCODE; }
"fsin"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FSIN; return OPCODE; }
"fcos"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FCOS; return OPCODE; }
"fdp"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FDP; return OPCODE; }
"fminmax"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FMINMAX; return OPCODE; }
"fmaxmin"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FMAXMIN; return OPCODE; }
"fmin"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FMIN; return OPCODE; }
"fmax"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FMAX; return OPCODE; }
"fdsx"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FDSX; return OPCODE; }
"fdsy"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FDSY; return OPCODE; }
"fmad16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FMAD16; return OPCODE; }
"efo"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_EFO; return EFO_OPCODE; }
"fadd"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FADD; return OPCODE; }
"fmul"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FMUL; return OPCODE; }
"fsub"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FSUB; return OPCODE; }
"fssq"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FSSQ; return OPCODE; }
	/* Integer opcodes. */
"pckf16f32"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKF16F32; return OPCODE; }
"pckf16f16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKF16F16; return OPCODE; }
"pckf16u16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKF16U16; return OPCODE; }
"pckf16s16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKF16S16; return OPCODE; }
"pcku16f32"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKU16F32; return OPCODE; }
"pcku16f16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKU16F16; return OPCODE; }
"pcku16u16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKU16U16; return OPCODE; }
"pcku16s16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKU16S16; return OPCODE; }
"pcks16f32"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKS16F32; return OPCODE; }
"pcks16f16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKS16F16; return OPCODE; }
"pcks16u16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKS16U16; return OPCODE; }
"pcks16s16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKS16S16; return OPCODE; }
"pcku8f32"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKU8F32; return OPCODE; }
"pcku8f16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKU8F16; return OPCODE; }
"pcku8u16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKU8U16; return OPCODE; }
"pcku8s16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKU8S16; return OPCODE; }
"pcks8f32"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKS8F32; return OPCODE; }
"pcks8f16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKS8F16; return OPCODE; }
"pcks8u16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKS8U16; return OPCODE; }
"pcks8s16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKS8S16; return OPCODE; }
"pcko8f32"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKO8F32; return OPCODE; }
"pcko8f16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKO8F16; return OPCODE; }
"pcko8u16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKO8U16; return OPCODE; }
"pcko8s16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKO8S16; return OPCODE; }
"pckc10f32"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKC10F32; return OPCODE; }
"pckc10f16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKC10F16; return OPCODE; }
"pckc10u16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKC10U16; return OPCODE; }
"pckc10s16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCKC10S16; return OPCODE; }
"un"?"pckf32f32"	{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKF32F32; return OPCODE; }
"un"?"pckf32f16"	{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKF32F16; return OPCODE; }
"un"?"pckf32u16"	{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKF32U16; return OPCODE; }
"un"?"pckf32s16"	{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKF32S16; return OPCODE; }
"un"?"pckf32u8"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKF32U8; return OPCODE; }
"un"?"pckf32s8"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKF32S8; return OPCODE; }
"un"?"pckf32o8"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKF32O8; return OPCODE; }
"un"?"pckf32c10"	{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKF32C10; return OPCODE; }
"un"?"pckf16f16"	{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKF16F16; return OPCODE; }
"un"?"pckf16u16"	{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKF16U16; return OPCODE; }
"un"?"pckf16s16"	{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKF16S16; return OPCODE; }
"un"?"pckf16u8"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKF16U8; return OPCODE; }
"un"?"pckf16s8"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKF16S8; return OPCODE; }
"un"?"pckf16o8"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKF16O8; return OPCODE; }
"un"?"pckf16c10"	{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKF16C10; return OPCODE; }
"un"?"pcku16f16"	{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKS16F16; return OPCODE; }
"un"?"pcku16u16"	{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKU16U16; return OPCODE; }
"un"?"pcku16s16"	{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKU16S16; return OPCODE; }
"un"?"pcku16u8"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKU16U8; return OPCODE; }
"un"?"pcku16s8"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKU16S8; return OPCODE; }
"un"?"pcku16o8"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKU16O8; return OPCODE; }
"un"?"pcku16c10"	{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKU16C10; return OPCODE; }
"un"?"pcks16f16"	{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKS16F16; return OPCODE; }
"un"?"pcks16u16"	{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKS16U16; return OPCODE; }
"un"?"pcks16s16"	{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKS16S16; return OPCODE; }
"un"?"pcks16u8"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKS16U8; return OPCODE; }
"un"?"pcks16s8"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKS16S8; return OPCODE; }
"un"?"pcks16o8"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKS16O8; return OPCODE; }
"un"?"pcks16c10"	{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKS16C10; return OPCODE; }
"un"?"pcku8u8"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKU8U8; return OPCODE; }
"un"?"pcku8s8"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKU8S8; return OPCODE; }
"un"?"pcks8u8"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKS8U8; return OPCODE; }
"un"?"pckc10c10"	{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_UNPCKC10C10; return OPCODE; }
"and"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_AND; return OPCODE; }
"or"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_OR; return OPCODE; }
"xor"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_XOR; return OPCODE; }
"shl"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SHL; return OPCODE; }
"shr"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SHR; return OPCODE; }
"asr"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_ASR; return OPCODE; }
"rol"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_ROL; return OPCODE; }
"rlp"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_RLP; return OPCODE; }
"sop2wm"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SOP2WM; return OPCODE; }
"sopwm"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SOP2WM; return OPCODE; }
"sop2"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SOP2; return OPCODE; }
"sop3"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SOP3; return OPCODE; }
"u8dot3"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_U8DOT3; return OPCODE; }
"u8dot4"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_U8DOT4; return OPCODE; }
"u16dot3"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_U16DOT3; return OPCODE; }
"u16dot4"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_U16DOT4; return OPCODE; }
"u8dot3off"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_U8DOT3OFF; return OPCODE; }
"u8dot4off"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_U8DOT4OFF; return OPCODE; }
"u16dot3off"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_U16DOT3OFF; return OPCODE; }
"u16dot4off"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_U16DOT4OFF; return OPCODE; }
"ima8"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IMA8; return OPCODE; }
"ima16"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IMA16; return OPCODE; }
"imae"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IMAE; return OPCODE; }
"ima32"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IMA32; return OPCODE; }	
"idiv"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IDIV; return OPCODE; }
"adif"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_ADIF; return OPCODE; }
"adifsum"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_ADIFSUM; return OPCODE; }
"firv"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FIRV; return FIR_OPCODE; }
"firvh"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FIRVH; return FIR_OPCODE; }
"bilin"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_BILIN; return OPCODE; }
"firh"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FIRH; return FIR_OPCODE; }
	/* Special opcodes. */
"smp1d"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SMP1D; return OPCODE; }
"smp2d"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SMP2D; return OPCODE; }
"smp3d"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SMP3D; return OPCODE; }
"smp1dbias"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SMP1DBIAS; return OPCODE; }
"smp2dbias"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SMP2DBIAS; return OPCODE; }
"smp3dbias"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SMP3DBIAS; return OPCODE; }
"smp1dreplace"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SMP1DREPLACE; return OPCODE; }
"smp2dreplace"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SMP2DREPLACE; return OPCODE; }
"smp3dreplace"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SMP3DREPLACE; return OPCODE; }
"smp1dgrad"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SMP1DGRAD; return OPCODE; }
"smp2dgrad"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SMP2DGRAD; return OPCODE; }
"smp3dgrad"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SMP3DGRAD; return OPCODE; }
"ldatomic"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_LDATOMIC; return LD_OPCODE; }	
"ldab"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_LDAB; return LD_OPCODE; }
"ldaw"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_LDAW; return LD_OPCODE; }
"ldad"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_LDAD; return LD_OPCODE; }
"ldaq"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_LDAQ; return LD_OPCODE; }
"stab"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_STAB; return ST_OPCODE; }
"staw"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_STAW; return ST_OPCODE; }
"stad"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_STAD; return ST_OPCODE; }
"staq"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_STAQ; return ST_OPCODE; }
"ldlb"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_LDLB; return LD_OPCODE; }
"ldlw"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_LDLW; return LD_OPCODE; }
"ldld"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_LDLD; return LD_OPCODE; }
"ldlq"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_LDLQ; return LD_OPCODE; }
"stlb"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_STLB; return ST_OPCODE; }
"stlw"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_STLW; return ST_OPCODE; }
"stld"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_STLD; return ST_OPCODE; }
"stlq"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_STLQ; return ST_OPCODE; }
"ldtb"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_LDTB; return LD_OPCODE; }
"ldtw"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_LDTW; return LD_OPCODE; }
"ldtd"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_LDTD; return LD_OPCODE; }
"ldtq"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_LDTQ; return LD_OPCODE; }
"sttb"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_STTB; return ST_OPCODE; }
"sttw"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_STTW; return ST_OPCODE; }
"sttd"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_STTD; return ST_OPCODE; }
"sttq"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_STTQ; return ST_OPCODE; }
"eldd"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_ELDD; return ELD_OPCODE; }
"eldq"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_ELDQ; return ELD_OPCODE; }
"ba"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_BA; BEGIN(BRANCH_INSTRUCTION); return BRANCH_OPCODE; }
"br"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_BR; BEGIN(BRANCH_INSTRUCTION); return BRANCH_OPCODE; }
"lapc"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_LAPC; return LAPC_OPCODE; }
"bexception"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_BEXCEPTION; return PTOFF_OPCODE; }
"sprvv"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SPRVV; return PTOFF_OPCODE; }
"idf"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IDF; return IDF_OPCODE; }
"wdf"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_WDF; return WDF_OPCODE; }
"emitpix1"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_EMITPIXEL1; return OPCODE; }
"emitpix2"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_EMITPIXEL2; return OPCODE; }
"emitpix"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_EMITPIXEL; return OPCODE; }
"emitst"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_EMITSTATE; return OPCODE; }
"emitvtx"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_EMITVERTEX; return EMITVTX_OPCODE; }
"emitprm"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_EMITPRIMITIVE; return OPCODE; }
"emitpds"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_EMITPDS; return OPCODE; }
"emitvcbst"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_EMITVCBSTATE; return EMITVTX_OPCODE; }
"emitvcbvtx"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_EMITVCBVERTEX; return EMITVTX_OPCODE; }
"emitmtest"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_EMITMTESTATE; return EMITVTX_OPCODE; }
"emitmtevtx"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_EMITMTEVERTEX; return EMITVTX_OPCODE; }
"mov"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_MOV; return OPCODE; }
"fmov"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FMOV; return OPCODE; }
"sdm"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SDM; return EXT_OPCODE; }
"sda"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SDA; return EXT_OPCODE; }
"amm"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_AMM; return EXT_OPCODE; }
"smm"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SMM; return EXT_OPCODE; }
"ams"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_AMS; return EXT_OPCODE; }
"sms"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SMS; return EXT_OPCODE; }
"dma"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_DMA; return EXT_OPCODE; }
"setp_gt"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SETPGT; return OPCODE; }
"setp_gte"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SETPGTE; return OPCODE; }
"setp_eq"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SETPEQ; return OPCODE; }
"setp_lt"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SETPLT; return OPCODE; }
"setp_lte"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SETPLTE; return OPCODE; }
"setp_neq"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SETPNEQ; return OPCODE; }
"ftz"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FTZ; return OPCODE; }
"ftnz"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FTNZ; return OPCODE; }
"u16tz"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_U16TZ; return OPCODE; }
"u16tnz"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_U16TNZ; return OPCODE; }
"smoa"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SMOA; return OPCODE; }
"smr"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SMR; return OPCODE; }
"smlsi"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SMLSI; return OPCODE; }
"smbo"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SMBO; return OPCODE; }
"fdp3"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FDP3; return OPCODE; }
"fdp4"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FDP4; return OPCODE; }
"fdpc"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FDPC; return OPCODE; }
"fdpc3"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FDPC3; return OPCODE; }
"fdpc4"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FDPC4; return OPCODE; }
"fddp"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FDDP; return OPCODE; }
"fddp3"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FDDP3; return OPCODE; }
"fddp4"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FDDP4; return OPCODE; }
"fddpc"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FDDPC; return OPCODE; }
"fddpc3"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FDDPC3; return OPCODE; }
"fddpc4"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FDDPC4; return OPCODE; }
"lock"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_LOCK; return LOCKRELEASE_OPCODE; }
"release"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_RELEASE; return LOCKRELEASE_OPCODE; }
"fsubflr"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FSUBFLR; return OPCODE; }
"ldr"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_LDR; return OPCODE; }
"str"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_STR; return OPCODE; }
"pcoeff"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PCOEFF; return OPCODE; }
"atst8"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_ATST8; return OPCODE; }
"depthf"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_DEPTHF; return OPCODE; }
"wop"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_WOP; return ONEARG_OPCODE; }
"setm"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SETM; return ONEARG_OPCODE; }
"phas"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PHAS; return OPCODE; }
"ptoff"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PTOFF; return PTOFF_OPCODE; }
"aintrp1"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_AINTRP1; return COISSUE_OPCODE; }
"aintrp2"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_AINTRP2; return COISSUE_OPCODE; }
"aadd"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_AADD; return COISSUE_OPCODE; }
"asub"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_ASUB; return COISSUE_OPCODE; }
"asop"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_ASOP; return COISSUE_OPCODE; }
"arsop"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_ARSOP; return COISSUE_OPCODE; }
"alrp"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_ALRP; return COISSUE_OPCODE; }
"lrp1"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_LRP1; return OPCODE; }
"lrp2"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_LRP2; return OPCODE; }
"asop2"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_ASOP2; return ASOP2_OPCODE; }
"addw"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IADD16; return OPCODE; }
"iadd16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IADD16; return OPCODE; }
"subw"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_ISUB16; return OPCODE; }
"isub16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_ISUB16; return OPCODE; }
"mulw"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IMUL16; return OPCODE; }
"imul16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IMUL16; return OPCODE; }
"iaddu16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IADDU16; return OPCODE; }
"isubu16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_ISUBU16; return OPCODE; }
"imulu16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IMULU16; return OPCODE; }
"add"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IADD32; return OPCODE; }
"iadd32"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IADD32; return OPCODE; }
"iaddu32"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IADDU32; return OPCODE; }
"iadd8"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IADD8; return OPCODE; }
"isub8"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_ISUB8; return OPCODE; }
"imul8"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IMUL8; return OPCODE; }
"imulu8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IMULU8; return OPCODE; }
"imov16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IMOV16; return OPCODE; }
"imo"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IMO; return OPCODE; }
"setfc"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SETFC; return OPCODE; }
"bal"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_BA; BEGIN(BRANCH_INSTRUCTION); return CALL_OPCODE; }
"brl"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_BR; BEGIN(BRANCH_INSTRUCTION); return CALL_OPCODE; }
"nop"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_PADDING; return NOP_OPCODE; }
"movc"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_MOVC; return OPCODE; }
"movmsk"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_MOVMSK; return OPCODE; }
"fpma"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FPMA; return OPCODE; }
"iaddu8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IADDU8; return OPCODE; }
"isubu8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_ISUBU8; return OPCODE; }
"fpmul8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FPMUL8; return OPCODE; }
"fpadd8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FPADD8; return OPCODE; }
"fpsub8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FPSUB8; return OPCODE; }
"sablnd"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SABLND; return OPCODE; }
"firhh"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FIRHH; return OPCODE; }
"ssum16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_SSUM16; return OPCODE; }
"fnrm32"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FNRM32; return OPCODE; }
"fnrm16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_FNRM16; return OPCODE; }
"moest"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_MOEST; return OPCODE; }
"cf"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_CF; return CFI_OPCODE; }
"ci"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_CI; return CFI_OPCODE; }
"cfi"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_CFI; return CFI_OPCODE; }
"vdp3"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VDP3; return OPCODE; }
"vdp4"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VDP4; return OPCODE; }
"vmad3"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VMAD3; return OPCODE; }
"vmad4"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VMAD4; return OPCODE; }
"vmul3"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VMUL3; return OPCODE; }
"vmul4"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VMUL4; return OPCODE; }
"vadd3"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VADD3; return OPCODE; }
"vadd4"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VADD4; return OPCODE; }
"vsub3"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VSUB3; return OPCODE; }
"vsub4"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VSUB4; return OPCODE; }
"vssq3"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VSSQ3; return OPCODE; }
"vssq4"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VSSQ4; return OPCODE; }
"vmov3"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VMOV3; return OPCODE; }
"vmov4"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VMOV4; return OPCODE; }
"vmov"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VMOV; return OPCODE; }
"vmovc"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VMOVC; return OPCODE; }
"vmovcu8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VMOVCU8; return OPCODE; }
"vmad"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VMAD; return OPCODE; }
"vmul"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VMUL; return OPCODE; }
"vadd"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VADD; return OPCODE; }
"vfrc"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VFRC; return OPCODE; }
"vdsx"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VDSX; return OPCODE; }
"vdsy"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VDSY; return OPCODE; }
"vdp"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VDP; return OPCODE; }
"vmin"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VMIN; return OPCODE; }
"vmax"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VMAX; return OPCODE; }
"vsub"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VSUB; return OPCODE; }
"vf16mad"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VF16MAD; return OPCODE; }
"vf16mul"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VF16MUL; return OPCODE; }
"vf16add"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VF16ADD; return OPCODE; }
"vf16frc"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VF16FRC; return OPCODE; }
"vf16dsx"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VF16DSX; return OPCODE; }
"vf16dsy"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VF16DSY; return OPCODE; }
"vf16min"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VF16MIN; return OPCODE; }
"vf16max"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VF16MAX; return OPCODE; }
"vf16dp"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VF16DP; return OPCODE; }
"vf16sub"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VF16SUB; return OPCODE; }
"vrcp"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VRCP; return OPCODE; }
"vrsq"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VRSQ; return OPCODE; }
"vlog"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VLOG; return OPCODE; }
"vexp"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VEXP; return OPCODE; }
"vpcku8u8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKU8U8; return OPCODE; }
"vpcku8s8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKU8S8; return OPCODE; }
"vpcku8o8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKU8O8; return OPCODE; }
"vpcku8u16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKU8U16; return OPCODE; }
"vpcku8s16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKU8S16; return OPCODE; }
"vpcku8f16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKU8F16; return OPCODE; }
"vpcku8f32"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKU8F32; return OPCODE; }
"vpcks8u8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKS8U8; return OPCODE; }
"vpcks8s8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKS8S8; return OPCODE; }
"vpcks8o8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKS8O8; return OPCODE; }
"vpcks8u16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKS8U16; return OPCODE; }
"vpcks8s16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKS8S16; return OPCODE; }
"vpcks8f16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKS8F16; return OPCODE; }
"vpcks8f32"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKS8F32; return OPCODE; }
"vpcko8u8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKO8U8; return OPCODE; }
"vpcko8s8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKO8S8; return OPCODE; }
"vpcko8o8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKO8O8; return OPCODE; }
"vpcko8u16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKO8U16; return OPCODE; }
"vpcko8s16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKO8S16; return OPCODE; }
"vpcko8f16"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKO8F16; return OPCODE; }
"vpcko8f32"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKO8F32; return OPCODE; }
"vpcku16u8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKU16U8; return OPCODE; }
"vpcku16s8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKU16S8; return OPCODE; }
"vpcku16o8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKU16O8; return OPCODE; }
"vpcku16u16"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKU16U16; return OPCODE; }
"vpcku16s16"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKU16S16; return OPCODE; }
"vpcku16f16"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKU16F16; return OPCODE; }
"vpcku16f32"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKU16F32; return OPCODE; }
"vpcks16u8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKS16U8; return OPCODE; }
"vpcks16s8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKS16S8; return OPCODE; }
"vpcks16o8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKS16O8; return OPCODE; }
"vpcks16u16"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKS16U16; return OPCODE; }
"vpcks16s16"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKS16S16; return OPCODE; }
"vpcks16f16"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKS16F16; return OPCODE; }
"vpcks16f32"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKS16F32; return OPCODE; }
"vpckf16u8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKF16U8; return OPCODE; }
"vpckf16s8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKF16S8; return OPCODE; }
"vpckf16o8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKF16O8; return OPCODE; }
"vpckf16u16"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKF16U16; return OPCODE; }
"vpckf16s16"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKF16S16; return OPCODE; }
"vpckf16f16"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKF16F16; return OPCODE; }
"vpckf16f32"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKF16F32; return OPCODE; }
"vpckf16c10"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKF32C10; return OPCODE; }
"vpckf32u8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKF32U8; return OPCODE; }
"vpckf32s8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKF32S8; return OPCODE; }
"vpckf32o8"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKF32O8; return OPCODE; }
"vpckf32u16"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKF32U16; return OPCODE; }
"vpckf32s16"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKF32S16; return OPCODE; }
"vpckf32f16"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKF32F16; return OPCODE; }
"vpckf32f32"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKF32F32; return OPCODE; }
"vpckf32c10"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKF32C10; return OPCODE; }
"vpckc10f16"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKC10F16; return OPCODE; }
"vpckc10f32"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKC10F32; return OPCODE; }
"vpckc10c10"		{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_VPCKC10C10; return OPCODE; }
"idxscr"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IDXSCR; return OPCODE; }
"idxscw"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_IDXSCW; return OPCODE; }
"isub32"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_ISUB32; return OPCODE; }
"isubu32"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_ISUBU32; return OPCODE; }
"cndst"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_CNDST; return OPCODE; }	
"cndef"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_CNDEF; return OPCODE; }	
"cndsm"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_CNDSM; return OPCODE; }	
"cndlt"				{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_CNDLT; return OPCODE; }	
"cndend"			{ InitOpLoc(&yylval.sOp); yylval.sOp.uOpcode = USEASM_OP_CNDEND; return OPCODE; }	

	/* Pseudo-instructions */
"input"	     		{ return INPUT; }
"output"			{ return OUTPUT; }
".schedoff"			{ return SCHEDOFF; }
".schedon"			{ return SCHEDON; }
".rptend"			{ return REPEATOFF; }
".skipinvon"		{ return SKIPINVON; }
".skipinvoff"		{ return SKIPINVOFF; }
".align"			{ InitOpLoc(&yylval.sOp); return FORCE_ALIGN; }
".misalign"			{ InitOpLoc(&yylval.sOp); return MISALIGN; }
".target"			{ InitOpLoc(&yylval.sOp); return TARGET; }
".export"			{ InitOpLoc(&yylval.sOp); BEGIN(BRANCH_INSTRUCTION); return EXPORT; }
".import"			{ InitOpLoc(&yylval.sOp); BEGIN(BRANCH_INSTRUCTION); return IMPORT; }
".modulealign"		{ InitOpLoc(&yylval.sOp); return MODULEALIGN; }
".named_reg"		{ return TEMP_REG_DEF; }
".named_regs_range"	{ return NAMED_REGS_RANGE; }
".rename_reg"		{ return RENAME_REG; }

	/* Register classes. */
"r"					{ yylval.n = USEASM_REGTYPE_TEMP; return TEMP_REGISTER; }
"o"					{ yylval.n = USEASM_REGTYPE_OUTPUT; return REGISTER; }
"pa"				{ yylval.n = USEASM_REGTYPE_PRIMATTR; return REGISTER; }
"sa"				{ yylval.n = USEASM_REGTYPE_SECATTR; return REGISTER; }
"g"					{ yylval.n = USEASM_REGTYPE_GLOBAL; return REGISTER; }
"c"					{ yylval.n = USEASM_REGTYPE_FPCONSTANT; return REGISTER; }
"pclink"			{ return PCLINK; }
"drc"				{ yylval.n = USEASM_REGTYPE_DRC; return REGISTER; }
"il"				{ return INDEXLOW; }
"ih"				{ return INDEXHIGH; }
"i.l"				{ return INDEXLOW; }
"i.h"				{ return INDEXHIGH; }
"i.lh"				{ return INDEXBOTH; }
"i.hl"				{ return INDEXBOTH; }
[pP]"0"				{ return PRED0; }
[pP]"1"				{ return PRED1; }
[pP]"2"				{ return PRED2; }
[pP]"3"				{ return PRED3; }
[pP][nN]			{ return PREDN; }
"cp"				{ yylval.n = USEASM_REGTYPE_CLIPPLANE; return REGISTER; }
"fcs"				{ yylval.n = USEASM_REGTYPE_FILTERCOEFF; return REGISTER; }
"ref"               { yylval.n = USEASM_REGTYPE_REF; return REGISTER; }


	/* Destination masks. */
"."[xyzw]+			{ yylval.n = ParseMask(yytext); return MASK; }

	/* Instruction flags. */
".skipinv"			{ yylval.n = USEASM_OPFLAGS1_SKIPINVALID; return OPCODE_FLAG1; }
".syncs"			{ yylval.n = USEASM_OPFLAGS1_SYNCSTART; return OPCODE_FLAG1; }
".nosched"			{ yylval.n = USEASM_OPFLAGS1_NOSCHED; return OPCODE_FLAG1; }
".repeat"[0-9]+		{ IMG_UINT32 uRepeatCount  = strtoul(yytext + 7, NULL, 10);  
			          if (uRepeatCount > EURASIA_USE_MAXIMUM_REPEAT) { ParseError("The maximum repeat count is %u", EURASIA_USE_MAXIMUM_REPEAT); uRepeatCount = EURASIA_USE_MAXIMUM_REPEAT; }
					  yylval.n = uRepeatCount << USEASM_OPFLAGS1_REPEAT_SHIFT; 
					  return REPEAT_FLAG; }
".rpt"[0-9]+		{ IMG_UINT32 uRepeatCount  = strtoul(yytext + 4, NULL, 10);  
			          if (uRepeatCount > EURASIA_USE_MAXIMUM_REPEAT) { ParseError("The maximum repeat count is %u", EURASIA_USE_MAXIMUM_REPEAT); uRepeatCount = EURASIA_USE_MAXIMUM_REPEAT; }
					  yylval.n = uRepeatCount << USEASM_OPFLAGS1_REPEAT_SHIFT; 
					  return REPEAT_FLAG; }
".end"				{ yylval.n = USEASM_OPFLAGS1_END; return OPCODE_FLAG1; }
".synce"			{ yylval.n = USEASM_OPFLAGS1_SYNCEND; return OPCODE_FLAG1; }
".savelink"			{ yylval.n = USEASM_OPFLAGS1_SAVELINK; return OPCODE_FLAG1; }
".partial"			{ yylval.n = USEASM_OPFLAGS1_PARTIAL; return OPCODE_FLAG1; }
".p"				{ yylval.n = USEASM_OPFLAGS1_PARTIAL; return OPCODE_FLAG1; }
".abs"				{ return ABS; }
".renable"			{ yylval.n = USEASM_OPFLAGS1_ROUNDENABLE; return OPCODE_FLAG1; }
"abs"				{ return ABS_NODOT; }
".fetch"[0-9]+		{ IMG_UINT32 uRepeatCount  = strtoul(yytext + 6, NULL, 10);  
			          if (uRepeatCount > EURASIA_USE_MAXIMUM_REPEAT) { ParseError("The maximum fetch count is %u", EURASIA_USE_MAXIMUM_REPEAT); uRepeatCount = EURASIA_USE_MAXIMUM_REPEAT; }
					  yylval.n = (uRepeatCount << USEASM_OPFLAGS1_REPEAT_SHIFT) | USEASM_OPFLAGS1_FETCHENABLE; 
					  return REPEAT_FLAG; }
".f"[0-9]+			{ IMG_UINT32 uRepeatCount  = strtoul(yytext + 2, NULL, 10);  
			          if (uRepeatCount > EURASIA_USE_MAXIMUM_REPEAT) { ParseError("The maximum fetch count is %u", EURASIA_USE_MAXIMUM_REPEAT); uRepeatCount = EURASIA_USE_MAXIMUM_REPEAT; }
					  yylval.n = (uRepeatCount << USEASM_OPFLAGS1_REPEAT_SHIFT) | USEASM_OPFLAGS1_FETCHENABLE; 
					  return REPEAT_FLAG; }
".rangeenable"		{ yylval.n = USEASM_OPFLAGS1_RANGEENABLE; return OPCODE_FLAG1; }
".freep"			{ yylval.n = USEASM_OPFLAGS3_FREEP; return OPCODE_FLAG3; }
".onceonly"			{ yylval.n = USEASM_OPFLAGS2_ONCEONLY; return OPCODE_FLAG2; }
".bpcache"			{ yylval.n = USEASM_OPFLAGS2_BYPASSCACHE; return OPCODE_FLAG2; }
".bypassl1"			{ yylval.n = USEASM_OPFLAGS3_BYPASSL1; return OPCODE_FLAG3; }
".noread"			{ yylval.n = USEASM_OPFLAGS3_NOREAD; return OPCODE_FLAG3; }
".gpiext"			{ yylval.n = USEASM_OPFLAGS3_GPIEXT; return OPCODE_FLAG3; }
".bpc"				{ yylval.n = USEASM_OPFLAGS2_BYPASSCACHE; return OPCODE_FLAG2; }
".fcfill"			{ yylval.n = USEASM_OPFLAGS2_FORCELINEFILL; return OPCODE_FLAG2; }
".tasks"			{ yylval.n = USEASM_OPFLAGS2_TASKSTART; return OPCODE_FLAG2; }
".taske"			{ yylval.n = USEASM_OPFLAGS2_TASKEND; return OPCODE_FLAG2; }
".scale"			{ yylval.n = USEASM_OPFLAGS2_SCALE; return OPCODE_FLAG2; }
".sat"				{ yylval.n = USEASM_OPFLAGS2_SAT; return OPCODE_FLAG2; }
".u"				{ yylval.n = USEASM_OPFLAGS2_UNSIGNED; return OPCODE_FLAG2; }
".s"				{ yylval.n = USEASM_OPFLAGS2_SIGNED; return OPCODE_FLAG2; }
".usat"				{ yylval.n = USEASM_OPFLAGS2_UNSIGNED | USEASM_OPFLAGS2_SAT; return OPCODE_FLAG2; }
".ssat"				{ yylval.n = USEASM_OPFLAGS2_SIGNED | USEASM_OPFLAGS2_SAT; return OPCODE_FLAG2; }
".i8"				{ yylval.n = USEASM_OPFLAGS1_MOVC_FMTI8; return OPCODE_FLAG1; }
".i10"				{ yylval.n = USEASM_OPFLAGS2_MOVC_FMTI10; return OPCODE_FLAG2; }
".i16"				{ yylval.n = USEASM_OPFLAGS2_MOVC_FMTI16; return OPCODE_FLAG2; }
".i32"				{ yylval.n = USEASM_OPFLAGS2_MOVC_FMTI32; return OPCODE_FLAG2; }
".mask"				{ yylval.n = USEASM_OPFLAGS1_MOVC_FMTMASK; return OPCODE_FLAG1; }
".flt"				{ yylval.n = USEASM_OPFLAGS2_MOVC_FMTF32; return OPCODE_FLAG2; }
".rs"[0-9]+			{ IMG_UINT32 uShift = strtoul(yytext + 3, NULL, 10); 
					  if (uShift > ((~USEASM_OPFLAGS2_RSHIFT_CLRMSK) >> USEASM_OPFLAGS2_RSHIFT_SHIFT)) { ParseError("The maximum right shift is %u", ((~USEASM_OPFLAGS2_RSHIFT_CLRMSK) >> USEASM_OPFLAGS2_RSHIFT_SHIFT)); uShift = 0; }
					  yylval.n = (uShift << USEASM_OPFLAGS2_RSHIFT_SHIFT); 
					  return OPCODE_FLAG2;
					}
".tpres"			{ yylval.n = USEASM_OPFLAGS2_TYPEPRESERVE; return OPCODE_FLAG2; }
".fsel"				{ yylval.n = USEASM_OPFLAGS2_FORMATSELECT; return OPCODE_FLAG2; }
".tog"				{ yylval.n = USEASM_OPFLAGS2_TOGGLEOUTFILES; return OPCODE_FLAG2; }
".moepoff"			{ yylval.n = USEASM_OPFLAGS2_MOEPOFF; return OPCODE_FLAG2; }
".mon"				{ yylval.n = USEASM_OPFLAGS1_MONITOR; return OPCODE_FLAG1; }
".incs"[01][01][01] { yylval.n = USEASM_OPFLAGS2_PERINSTMOE;
					  if (yytext[5] == '1') yylval.n |= USEASM_OPFLAGS2_PERINSTMOE_INCS0;
					  if (yytext[6] == '1') yylval.n |= USEASM_OPFLAGS2_PERINSTMOE_INCS1;
					  if (yytext[7] == '1') yylval.n |= USEASM_OPFLAGS2_PERINSTMOE_INCS2;
					  return OPCODE_FLAG2; }
"skipinv"			{ return SKIPINV_FLAG; }
"nosched"			{ return NOSCHED_FLAG; }

".pcff16"			{ yylval.n = USEASM_OPFLAGS3_PCFF16; return OPCODE_FLAG3; }
".pcf"				{ yylval.n = USEASM_OPFLAGS3_PCFF32; return OPCODE_FLAG3; }
".rsd"				{ yylval.n = USEASM_OPFLAGS3_SAMPLEDATA; return OPCODE_FLAG3; }
".sinf"				{ yylval.n = USEASM_OPFLAGS3_SAMPLEINFO; return OPCODE_FLAG3; }
".irsd"				{ yylval.n = USEASM_OPFLAGS3_SAMPLEDATAANDINFO; return OPCODE_FLAG3; }
".syncent"			{ yylval.n = USEASM_OPFLAGS3_SYNCENDNOTTAKEN; return OPCODE_FLAG3; }
".trigger"			{ yylval.n = USEASM_OPFLAGS3_TRIGGER; return OPCODE_FLAG3; }
".twopart"			{ yylval.n = USEASM_OPFLAGS3_TWOPART; return OPCODE_FLAG3; }
".threepart"		{ yylval.n = USEASM_OPFLAGS3_THREEPART; return OPCODE_FLAG3; }
".cut"				{ yylval.n = USEASM_OPFLAGS3_CUT; return OPCODE_FLAG3; }
".global"			{ yylval.n = USEASM_OPFLAGS3_GLOBAL; return OPCODE_FLAG3; }
".dm_nomatch"		{ yylval.n = USEASM_OPFLAGS3_DM_NOMATCH; return OPCODE_FLAG3; }
".pwait"			{ yylval.n = USEASM_OPFLAGS3_PWAIT; return OPCODE_FLAG3; }
".minp"				{ yylval.n = USEASM_OPFLAGS3_MINPACK; return OPCODE_FLAG3; }
".s1"				{ yylval.n = USEASM_OPFLAGS3_STEP1; return OPCODE_FLAG3; }
".s2"				{ yylval.n = USEASM_OPFLAGS3_STEP2; return OPCODE_FLAG3; }
".smsk"				{ yylval.n = USEASM_OPFLAGS3_SAMPLEMASK; return OPCODE_FLAG3; }
".allinst"			{ yylval.n = USEASM_OPFLAGS3_ALLINSTANCES; return OPCODE_FLAG3; }
".anyinst"			{ yylval.n = USEASM_OPFLAGS3_ANYINSTANCES; return OPCODE_FLAG3; }

	/* Instruction test types. */
".testz"			{ yylval.n = ParseTest("t&z"); return TEST_TYPE; }
".testnz"			{ yylval.n = ParseTest("t&nz"); return TEST_TYPE; }
".test!z"			{ yylval.n = ParseTest("t&nz"); return TEST_TYPE; }
".tests"			{ yylval.n = ParseTest("n&t"); return TEST_TYPE; }
".testns"			{ yylval.n = ParseTest("p&t"); return TEST_TYPE; }
".testp"			{ yylval.n = ParseTest("p&nz"); return TEST_TYPE; }
".testn"			{ yylval.n = ParseTest("n&nz"); return TEST_TYPE; }
".tz"				{ yylval.n = ParseTest("t&z"); return TEST_TYPE; }
".tnz"				{ yylval.n = ParseTest("t&nz"); return TEST_TYPE; }
".t!z"				{ yylval.n = ParseTest("t&nz"); return TEST_TYPE; }
".ts"				{ yylval.n = ParseTest("n&t"); return TEST_TYPE; }
".tns"				{ yylval.n = ParseTest("p&t"); return TEST_TYPE; }
".tp"				{ yylval.n = ParseTest("p&nz"); return TEST_TYPE; }
".tn"				{ yylval.n = ParseTest("n&nz"); return TEST_TYPE; }
".test"[tpn][&|]("t"|"z"|"nz")	{ yylval.n = ParseTest(yytext + 5); return TEST_TYPE; }
".t"[tpn][&|]("t"|"z"|"nz")	{ yylval.n = ParseTest(yytext + 2); return TEST_TYPE; }

".c0"				{ yylval.n = USEASM_TEST_CHANSEL_C0 << USEASM_TEST_CHANSEL_SHIFT; return TEST_CHANSEL; }
".c1"				{ yylval.n = USEASM_TEST_CHANSEL_C1 << USEASM_TEST_CHANSEL_SHIFT; return TEST_CHANSEL; }
".c2"				{ yylval.n = USEASM_TEST_CHANSEL_C2 << USEASM_TEST_CHANSEL_SHIFT; return TEST_CHANSEL; }
".c3"				{ yylval.n = USEASM_TEST_CHANSEL_C3 << USEASM_TEST_CHANSEL_SHIFT; return TEST_CHANSEL; }
".orall"			{ yylval.n = USEASM_TEST_CHANSEL_ORALL << USEASM_TEST_CHANSEL_SHIFT; return TEST_CHANSEL; }
".andall"			{ yylval.n = USEASM_TEST_CHANSEL_ANDALL << USEASM_TEST_CHANSEL_SHIFT; return TEST_CHANSEL; }
".and02"			{ yylval.n = USEASM_TEST_CHANSEL_AND02 << USEASM_TEST_CHANSEL_SHIFT; return TEST_CHANSEL; }
".or02"				{ yylval.n = USEASM_TEST_CHANSEL_OR02 << USEASM_TEST_CHANSEL_SHIFT; return TEST_CHANSEL; }
".&"				{ yylval.n = USEASM_TEST_CHANSEL_ANDALL << USEASM_TEST_CHANSEL_SHIFT; return TEST_CHANSEL; }
".|"				{ yylval.n = USEASM_TEST_CHANSEL_ORALL << USEASM_TEST_CHANSEL_SHIFT; return TEST_CHANSEL; }
".&02"				{ yylval.n = USEASM_TEST_CHANSEL_AND02 << USEASM_TEST_CHANSEL_SHIFT; return TEST_CHANSEL; }
".|02"				{ yylval.n = USEASM_TEST_CHANSEL_OR02 << USEASM_TEST_CHANSEL_SHIFT; return TEST_CHANSEL; }
".cperchan"			{ yylval.n = USEASM_TEST_CHANSEL_PERCHAN << USEASM_TEST_CHANSEL_SHIFT; return TEST_CHANSEL; } 
".bmsk"				{ yylval.n = USEASM_TEST_MASK_BYTE << USEASM_TEST_MASK_SHIFT; return TEST_MASK; }
".wmsk"				{ yylval.n = USEASM_TEST_MASK_WORD << USEASM_TEST_MASK_SHIFT; return TEST_MASK; }
".dmsk"				{ yylval.n = USEASM_TEST_MASK_DWORD << USEASM_TEST_MASK_SHIFT; return TEST_MASK; }
".msk"				{ yylval.n = USEASM_TEST_MASK_PREC << USEASM_TEST_MASK_SHIFT; return TEST_MASK; }
".num"				{ yylval.n = USEASM_TEST_MASK_NUM << USEASM_TEST_MASK_SHIFT; return TEST_MASK; }

	/* Source operand flags. */
".neg"				{ yylval.n = USEASM_ARGFLAGS_NEGATE; return ARGUMENT_FLAG; }
".bytemask"[01][01][01][01] { yylval.n = (((yytext[9] == '1' ? 0x08 : 0x00) | 
										   (yytext[10] == '1' ? 0x04 : 0x00) | 
										   (yytext[11] == '1' ? 0x02 : 0x00) | 
										   (yytext[12] == '1' ? 0x01 : 0x00)) << USEASM_ARGFLAGS_BYTEMSK_SHIFT) |
										 USEASM_ARGFLAGS_BYTEMSK_PRESENT; 
							  return ARGUMENT_FLAG; }
"."[01][01][01][01]	{ yylval.n = (((yytext[1] == '1' ? 0x08 : 0x00) | 
								   (yytext[2] == '1' ? 0x04 : 0x00) | 
								   (yytext[3] == '1' ? 0x02 : 0x00) | 
								   (yytext[4] == '1' ? 0x01 : 0x00)) << USEASM_ARGFLAGS_BYTEMSK_SHIFT) |
								 USEASM_ARGFLAGS_BYTEMSK_PRESENT; 
					  return ARGUMENT_FLAG; }
".0"				{ yylval.n = 0 << USEASM_ARGFLAGS_COMP_SHIFT; return ARGUMENT_FLAG; }
".1"				{ yylval.n = 1 << USEASM_ARGFLAGS_COMP_SHIFT; return ARGUMENT_FLAG; }
".2"				{ yylval.n = 2 << USEASM_ARGFLAGS_COMP_SHIFT; return ARGUMENT_FLAG; }
".3"				{ yylval.n = 3 << USEASM_ARGFLAGS_COMP_SHIFT; return ARGUMENT_FLAG; }
".comp0"			{ yylval.n = 0 << USEASM_ARGFLAGS_COMP_SHIFT; return ARGUMENT_FLAG; }
".comp1"			{ yylval.n = 1 << USEASM_ARGFLAGS_COMP_SHIFT; return ARGUMENT_FLAG; }
".comp2"			{ yylval.n = 2 << USEASM_ARGFLAGS_COMP_SHIFT; return ARGUMENT_FLAG; }
".comp3"			{ yylval.n = 3 << USEASM_ARGFLAGS_COMP_SHIFT; return ARGUMENT_FLAG; }
".low"				{ yylval.n = USEASM_ARGFLAGS_LOW; return ARGUMENT_FLAG; }
".high"				{ yylval.n = USEASM_ARGFLAGS_HIGH; return ARGUMENT_FLAG; }
".l"				{ yylval.n = USEASM_ARGFLAGS_LOW; return ARGUMENT_FLAG; }
".h"				{ yylval.n = USEASM_ARGFLAGS_HIGH; return ARGUMENT_FLAG; }
".invert"			{ yylval.n = USEASM_ARGFLAGS_INVERT; return ARGUMENT_FLAG; }
".alpha"			{ yylval.n = USEASM_ARGFLAGS_ALPHA; return ARGUMENT_FLAG; }
".moe"				{ yylval.n = USEASM_ARGFLAGS_REQUIREMOE; return ARGUMENT_FLAG; }
".comp"				{ yylval.n = USEASM_ARGFLAGS_COMPLEMENT; return ARGUMENT_FLAG; }
".flt32"			{ yylval.n = USEASM_ARGFLAGS_FMTF32; return ARGUMENT_FLAG; }
".swzlh"			{ yylval.n = (USEASM_MAD16SWZ_LOWHIGH << USEASM_ARGFLAGS_MAD16SWZ_SHIFT); return ARGUMENT_FLAG; }
".swzll"			{ yylval.n = (USEASM_MAD16SWZ_LOWLOW << USEASM_ARGFLAGS_MAD16SWZ_SHIFT); return ARGUMENT_FLAG; }
".swzhh"			{ yylval.n = (USEASM_MAD16SWZ_HIGHHIGH << USEASM_ARGFLAGS_MAD16SWZ_SHIFT); return ARGUMENT_FLAG; }
".swzf32"			{ yylval.n = (USEASM_MAD16SWZ_CVTFROMF32 << USEASM_ARGFLAGS_MAD16SWZ_SHIFT); return ARGUMENT_FLAG; }
".flt16"			{ return F16_FLAG; }
".u8"				{ return U8_FLAG; }
".c10"				{ return C10_FLAG; }

	/* Addressing modes. */
"none"				{ yylval.n = EURASIA_USE1_MOECTRL_ADDRESSMODE_NONE; return ADDRESS_MODE; }
"repeat"			{ yylval.n = EURASIA_USE1_MOECTRL_ADDRESSMODE_REPEAT; return ADDRESS_MODE; }
"clamp"				{ yylval.n = EURASIA_USE1_MOECTRL_ADDRESSMODE_CLAMP; return ADDRESS_MODE; }
"mirror"			{ yylval.n = EURASIA_USE1_MOECTRL_ADDRESSMODE_MIRROR; return ADDRESS_MODE; }

	/* Swizzle mode enable/disable. */
"incrementmode"		{ yylval.n = FALSE; return DIRECT_IMMEDIATE; }
"swizzlemode"		{ yylval.n = TRUE; return DIRECT_IMMEDIATE; }
"rep"				{ yylval.n = EURASIA_USE1_FIRH_EDGEMODE_REPLICATE; return DIRECT_IMMEDIATE; }
"msc"				{ yylval.n = EURASIA_USE1_FIRH_EDGEMODE_MIRRORSINGLE; return DIRECT_IMMEDIATE; }
"mdc"				{ yylval.n = EURASIA_USE1_FIRH_EDGEMODE_MIRRORDOUBLE; return DIRECT_IMMEDIATE; }

	/* Swizzle mode increment format. */
"swizzle("[xyzw012h][xyzw012h][xyzw012h][xyzw012h]")" { 
												yylval.n = (XyzwToN(yytext[8]) << (USEASM_SWIZZLE_FIELD_SIZE * 0)) | 
														   (XyzwToN(yytext[9]) << (USEASM_SWIZZLE_FIELD_SIZE * 1)) |
														   (XyzwToN(yytext[10]) << (USEASM_SWIZZLE_FIELD_SIZE * 2)) | 
														   (XyzwToN(yytext[11]) << (USEASM_SWIZZLE_FIELD_SIZE * 3)); 
												return SWIZZLE; }
"swizzle("[xyzw012h][xyzw012h][xyzw012h]")" { 
												yylval.n = (XyzwToN(yytext[8]) << (USEASM_SWIZZLE_FIELD_SIZE * 0)) | 
														   (XyzwToN(yytext[9]) << (USEASM_SWIZZLE_FIELD_SIZE * 1)) |
														   (XyzwToN(yytext[10]) << (USEASM_SWIZZLE_FIELD_SIZE * 2)) | 
														   (XyzwToN(yytext[10]) << (USEASM_SWIZZLE_FIELD_SIZE * 3)); 
												return SWIZZLE; }

	/* Label */
<BRANCH_INSTRUCTION>[[:alpha:]_][[:alnum:]_]*	{ 	
													BEGIN(INITIAL);
													InitOpLoc(&yylval.sOp);
													yylval.sOp.pszCharString = strdup(yytext);													
													return IDENTIFIER;
												}

	/* Integer source selects. */
"Ax1"				{ yylval.n = USEASM_INTSRCSEL_AX1; return INTSRCSEL; }
"Ax2"				{ yylval.n = USEASM_INTSRCSEL_AX2; return INTSRCSEL; }
"Ax4"				{ yylval.n = USEASM_INTSRCSEL_AX4; return INTSRCSEL; }
"Ax8"				{ yylval.n = USEASM_INTSRCSEL_AX8; return INTSRCSEL; }
"Cx1"				{ yylval.n = USEASM_INTSRCSEL_CX1; return INTSRCSEL; }
"Cx2"				{ yylval.n = USEASM_INTSRCSEL_CX2; return INTSRCSEL; }
"Cx4"				{ yylval.n = USEASM_INTSRCSEL_CX4; return INTSRCSEL; }
"Cx8"				{ yylval.n = USEASM_INTSRCSEL_CX8; return INTSRCSEL; }
"zero"				{ yylval.n = USEASM_INTSRCSEL_ZERO; return INTSRCSEL; }
"one"				{ yylval.n = USEASM_INTSRCSEL_ONE; return INTSRCSEL; }
"s0a"				{ yylval.n = USEASM_INTSRCSEL_SRC0ALPHA; return INTSRCSEL; }
"s1a"				{ yylval.n = USEASM_INTSRCSEL_SRC1ALPHA; return INTSRCSEL; }
"s2a"				{ yylval.n = USEASM_INTSRCSEL_SRC2ALPHA; return INTSRCSEL; }
"sub"				{ yylval.n = USEASM_INTSRCSEL_SUB; return INTSRCSEL; }
"neg"				{ yylval.n = USEASM_INTSRCSEL_NEG; return INTSRCSEL; }
"min"				{ yylval.n = USEASM_INTSRCSEL_MIN; return INTSRCSEL; }
"max"				{ yylval.n = USEASM_INTSRCSEL_MAX; return INTSRCSEL; }
"imin"				{ yylval.n = USEASM_INTSRCSEL_IMIN; return INTSRCSEL; }
"imax"				{ yylval.n = USEASM_INTSRCSEL_IMAX; return INTSRCSEL; }
"umin"				{ yylval.n = USEASM_INTSRCSEL_UMIN; return INTSRCSEL; }
"umax"				{ yylval.n = USEASM_INTSRCSEL_UMAX; return INTSRCSEL; }
"s2scale"			{ yylval.n = USEASM_INTSRCSEL_SRC2SCALE; return INTSRCSEL; }
"zeros"				{ yylval.n = USEASM_INTSRCSEL_ZEROS; return INTSRCSEL; }
"comp"				{ yylval.n = USEASM_INTSRCSEL_COMP; return INTSRCSEL; }
"+src2.2"			{ yylval.n = USEASM_INTSRCSEL_PLUSSRC2DOT2; return INTSRCSEL; }
"iadd"				{ yylval.n = USEASM_INTSRCSEL_IADD; return INTSRCSEL; }
"scale"				{ yylval.n = USEASM_INTSRCSEL_SCALE; return INTSRCSEL; }
"asat"				{ yylval.n = USEASM_INTSRCSEL_SRCALPHASAT; return INTSRCSEL; }
"rasat"				{ yylval.n = USEASM_INTSRCSEL_RSRCALPHASAT; return INTSRCSEL; }
"u8"				{ yylval.n = USEASM_INTSRCSEL_U8; return INTSRCSEL; }
"s8"				{ yylval.n = USEASM_INTSRCSEL_S8; return INTSRCSEL; }
"off8"				{ yylval.n = USEASM_INTSRCSEL_O8; return INTSRCSEL; }
"z16"				{ yylval.n = USEASM_INTSRCSEL_Z16; return INTSRCSEL; }
"s16"				{ yylval.n = USEASM_INTSRCSEL_S16; return INTSRCSEL; }
"u32"				{ yylval.n = USEASM_INTSRCSEL_U32; return INTSRCSEL; }
"cie"				{ yylval.n = USEASM_INTSRCSEL_CINENABLE; return INTSRCSEL; }
"coe"				{ yylval.n = USEASM_INTSRCSEL_COUTENABLE; return INTSRCSEL; }
"u16"				{ yylval.n = USEASM_INTSRCSEL_U16; return INTSRCSEL; }
"interleaved"		{ yylval.n = USEASM_INTSRCSEL_INTERLEAVED; return INTSRCSEL; }
"planar"			{ yylval.n = USEASM_INTSRCSEL_PLANAR; return INTSRCSEL; }
"src01"				{ yylval.n = USEASM_INTSRCSEL_SRC01; return INTSRCSEL; }
"src23"				{ yylval.n = USEASM_INTSRCSEL_SRC23; return INTSRCSEL; }
"dst01"				{ yylval.n = USEASM_INTSRCSEL_DST01; return INTSRCSEL; }
"dst23"				{ yylval.n = USEASM_INTSRCSEL_DST23; return INTSRCSEL; }
"rnd"				{ yylval.n = USEASM_INTSRCSEL_RND; return INTSRCSEL; }
"twosided"			{ yylval.n = USEASM_INTSRCSEL_TWOSIDED; return INTSRCSEL; }
"fb"				{ yylval.n = USEASM_INTSRCSEL_FEEDBACK; return INTSRCSEL; }
"nfb"				{ yylval.n = USEASM_INTSRCSEL_NOFEEDBACK; return INTSRCSEL; }
"optdw"				{ yylval.n = USEASM_INTSRCSEL_OPTDWD; return INTSRCSEL; }
"idst"				{ yylval.n = USEASM_INTSRCSEL_IDST; return INTSRCSEL; }
"down"				{ yylval.n = USEASM_INTSRCSEL_ROUNDDOWN; return INTSRCSEL; }
"nearest"			{ yylval.n = USEASM_INTSRCSEL_ROUNDNEAREST; return INTSRCSEL; }
"up"				{ yylval.n = USEASM_INTSRCSEL_ROUNDUP; return INTSRCSEL; }
"pixel"				{ yylval.n = USEASM_INTSRCSEL_PIXEL; return INTSRCSEL; }
"selective"			{ yylval.n = USEASM_INTSRCSEL_SELECTIVE; return INTSRCSEL; }
"sample"			{ yylval.n = USEASM_INTSRCSEL_SAMPLE; return INTSRCSEL; }
"pt"				{ yylval.n = USEASM_INTSRCSEL_PT; return INTSRCSEL; }
"vcull"				{ yylval.n = USEASM_INTSRCSEL_VCULL; return INTSRCSEL; }
"end"				{ yylval.n = USEASM_INTSRCSEL_END; return INTSRCSEL; }
"parallel"			{ yylval.n = USEASM_INTSRCSEL_PARALLEL; return INTSRCSEL; }
"perinstance"		{ yylval.n = USEASM_INTSRCSEL_PERINSTANCE; return INTSRCSEL; }
"srcneg"			{ yylval.n = USEASM_INTSRCSEL_SRCNEG; return INTSRCSEL; }
"srcabs"			{ yylval.n = USEASM_INTSRCSEL_SRCABS; return INTSRCSEL; }
"incuss"			{ yylval.n = USEASM_INTSRCSEL_INCREMENTUS; return INTSRCSEL; }
"incgpi"			{ yylval.n = USEASM_INTSRCSEL_INCREMENTGPI; return INTSRCSEL; }
"incboth"			{ yylval.n = USEASM_INTSRCSEL_INCREMENTBOTH; return INTSRCSEL; }
"moe"				{ yylval.n = USEASM_INTSRCSEL_INCREMENTMOE; return INTSRCSEL; }
"selnone"			{ yylval.n = USEASM_INTSRCSEL_NONE; return INTSRCSEL; }
"incall"			{ yylval.n = USEASM_INTSRCSEL_INCREMENTALL; return INTSRCSEL; }
"incs1"				{ yylval.n = USEASM_INTSRCSEL_INCREMENTS1; return INTSRCSEL; }
"incs2"				{ yylval.n = USEASM_INTSRCSEL_INCREMENTS2; return INTSRCSEL; }
"incs1s2"			{ yylval.n = USEASM_INTSRCSEL_INCREMENTS1S2; return INTSRCSEL; }
"f16"				{ yylval.n = USEASM_INTSRCSEL_F16; return INTSRCSEL; }
"f32"				{ yylval.n = USEASM_INTSRCSEL_F32; return INTSRCSEL; }
"xchg"				{ yylval.n = USEASM_INTSRCSEL_XCHG; return INTSRCSEL; }
"revsub"			{ yylval.n = USEASM_INTSRCSEL_REVSUB; return INTSRCSEL; }
"true"				{ yylval.n = USEASM_INTSRCSEL_TRUE; return INTSRCSEL; }
"false"				{ yylval.n = USEASM_INTSRCSEL_FALSE; return INTSRCSEL; }

"sgx520"			{ yylval.n = SGX_CORE_ID_520; return TARGET_SPECIFIER; }
"sgx530"			{ yylval.n = SGX_CORE_ID_530; return TARGET_SPECIFIER; }
"sgx531"			{ yylval.n = SGX_CORE_ID_531; return TARGET_SPECIFIER; }
"sgx535"			{ yylval.n = SGX_CORE_ID_535; return TARGET_SPECIFIER; }
"sgx540"			{ yylval.n = SGX_CORE_ID_540; return TARGET_SPECIFIER; }
"sgx541"			{ yylval.n = SGX_CORE_ID_541; return TARGET_SPECIFIER; }
"sgx543"			{ yylval.n = SGX_CORE_ID_543; return TARGET_SPECIFIER; }
"sgx544"			{ yylval.n = SGX_CORE_ID_544; return TARGET_SPECIFIER; }
"sgx545"			{ yylval.n = SGX_CORE_ID_545; return TARGET_SPECIFIER; }
"sgx554"			{ yylval.n = SGX_CORE_ID_554; return TARGET_SPECIFIER; }

"st"				{ return IDF_ST; }
"pixelbe"			{ return IDF_PIXELBE; }

	/* Index/memory addressing. */
"["					{ return OPEN_SQBRACKET; }
"]"					{ return CLOSE_SQBRACKET; }

	/* Assemble-time expressions. */
"&"					{ return AND; }
"|"					{ return OR; }
"^"					{ return XOR; }
"++"				{ return PLUSPLUS; }
"--"				{ return MINUSMINUS; }
"+"					{ return PLUS; }
"-"					{ return MINUS; }
"*"					{ return TIMES; }
"/"					{ return DIVIDE; }
"<<"				{ return LSHIFT; }
">>"				{ return RSHIFT; }
"!"					{ return BANG; }
","					{ return COMMA; }
"="					{ return EQUALS; }
"~"					{ return NOT; }
"%"					{ return MODULUS; }
"("					{ return OPEN_BRACKET; }
")"					{ return CLOSE_BRACKET; }
":"					{ if (!g_bCPreprocessor) { return COLON; } else { return COLON_PLUS_DELIMITER; } }
"\\"[ \f\t\r]*"\r\n"	{ g_uSourceLine++; }
"\\"[ \f\t\r]*"\n"	{ g_uSourceLine++; }
"\n"				{ BEGIN(INITIAL); g_uSourceLine++; NEWLINE_TO_WHITESPACE_OR_DELIMITER; }
"\r\n"				{ BEGIN(INITIAL); g_uSourceLine++; NEWLINE_TO_WHITESPACE_OR_DELIMITER; }
";"					{ BEGIN(INITIAL); return INSTRUCTION_DELIMITER; }
<POSSIBLE_FLOAT_NUMBER>"LADDR"				{ BEGIN(BRANCH_INSTRUCTION); return LABEL_ADDRESS; }
"OFFSET("[^\n)]*")"		{ ResolveNameToType(yytext + 7, yyleng - 8, &yylval.n, NULL, g_pszInFileName, g_uSourceLine); return NUMBER; }
"SIZEOF("[^\n)]*")"		{ ResolveNameToType(yytext + 7, yyleng - 8, NULL, &yylval.n, g_pszInFileName, g_uSourceLine); return NUMBER; }
"BOFFSET("[^\n)]*")"		{ ResolveNameToType(yytext + 8, yyleng - 9, &yylval.n, NULL, g_pszInFileName, g_uSourceLine); return NUMBER; }
"BSIZEOF("[^\n)]*")"		{ ResolveNameToType(yytext + 8, yyleng - 9, NULL, &yylval.n, g_pszInFileName, g_uSourceLine); return NUMBER; }
"WOFFSET("[^\n)]*")"		{ 
					  ResolveNameToType(yytext + 8, yyleng - 9, &yylval.n, NULL, g_pszInFileName, g_uSourceLine); 
					  if ((yylval.n % 2) != 0)
					  {	
						ParseError("Offset of structure member %.*s isn't word aligned", yyleng - 9, yytext + 8);
					  }
					  yylval.n /= 2;
					  return NUMBER; 
					}
"WSIZEOF("[^\n)]*")"		{ 
					  ResolveNameToType(yytext + 8, yyleng - 9, NULL, &yylval.n, g_pszInFileName, g_uSourceLine); 
					  if ((yylval.n % 2) != 0)
					  {	
						ParseError("Size of type %.*s isn't word aligned", yyleng - 9, yytext + 8);
					  }
					  yylval.n /= 2;
					  return NUMBER; 
					}
"DOFFSET("[^\n)]*")"		{ 
					  ResolveNameToType(yytext + 8, yyleng - 9, &yylval.n, NULL, g_pszInFileName, g_uSourceLine); 
					  if ((yylval.n % 4) != 0)
					  {	
						ParseError("Offset of structure member %.*s isn't dword aligned", yyleng - 9, yytext + 8);
					  }
					  yylval.n /= 4;
					  return NUMBER; 
					}
"DSIZEOF("[^\n)]*")"		{ 
					  ResolveNameToType(yytext + 8, yyleng - 9, NULL, &yylval.n, g_pszInFileName, g_uSourceLine); 
					  if ((yylval.n % 4) != 0)
					  {
						ParseError("Size of type %.*s isn't dword aligned", yyleng - 9, yytext + 8);
					  }
					  yylval.n /= 4;
					  return NUMBER; 
					}

	/* Components of an efo expression. */
[iI]"0"				{ return I0; }
[iI]"1"				{ return I1; }
[aA]"0"				{ return A0; }
[aA]"1"				{ return A1; }
[mM]"0"				{ return M0; }
[mM]"1"				{ return M1; }
[sS]"0"				{ return SRC0; }
[sS]"1"				{ return SRC1; }
[sS]"2"				{ return SRC2; }
"i"					{ yylval.n = USEASM_REGTYPE_FPINTERNAL; return REGISTER; }

	/* Numbers */
"#"					{ BEGIN(POSSIBLE_FLOAT_NUMBER); return HASH; }
<POSSIBLE_FLOAT_NUMBER>[0-9]+"f"						{ BEGIN(INITIAL); yylval.f = atof(yytext); return FLOAT_NUMBER; }
[0-9]+U?L?L?											{ BEGIN(INITIAL); yylval.n = strtoul(yytext, NULL, 10); return NUMBER; }
<POSSIBLE_FLOAT_NUMBER>[0-9]+"."[0-9]+"f"*				{ BEGIN(INITIAL); yylval.f = atof(yytext); return FLOAT_NUMBER; }

" "					
\t

"{"					{ 
						InitOpLoc(&yylval.sOp);
						return OPEN_CURLY_BRACKET; 
					}
"}"					{ return CLOSE_CURLY_BRACKET; }

[[:alpha:]_][[:alnum:]_]*	{ 	
								if(IsReservedWord(yytext) == IMG_TRUE)
								{
									REJECT;
								}
								else
								{
									BEGIN(INITIAL);
									InitOpLoc(&yylval.sOp);
									yylval.sOp.pszCharString = strdup(yytext);													
									return IDENTIFIER;									
								}
							}	

.					{ BEGIN(UNKNOWN_CHARACTER); addtoinvalidinput(*yytext); }
<UNKNOWN_CHARACTER>.	{ addtoinvalidinput(*yytext); }
<UNKNOWN_CHARACTER>"\n"	{ BEGIN(INITIAL); dumpinvalidinput(); return INSTRUCTION_DELIMITER; }	
<UNKNOWN_CHARACTER>"\r\n"	{ BEGIN(INITIAL); dumpinvalidinput(); return INSTRUCTION_DELIMITER; }	
<UNKNOWN_CHARACTER><<EOF>>	{ BEGIN(INITIAL); dumpinvalidinput(); return INSTRUCTION_DELIMITER; }	

<<EOF>>				{ if (!g_bEof) { g_bEof = TRUE; NEWLINE_TO_WHITESPACE_OR_DELIMITER; } else { return -1; } }

%%

void addtoinvalidinput(IMG_CHAR c)
{
	if (g_pcSavedInvalidInput != NULL)
	{
		int len = strlen(g_pcSavedInvalidInput);
		g_pcSavedInvalidInput = realloc(g_pcSavedInvalidInput, len + 2);
		g_pcSavedInvalidInput[len] = c;
		g_pcSavedInvalidInput[len + 1] = '\0';
	}
	else
	{
		g_pcSavedInvalidInput = UseAsm_Malloc(2);
		g_pcSavedInvalidInput[0] = c;
		g_pcSavedInvalidInput[1] = '\0';
	}
}

void dumpinvalidinput(void)
{
	ParseError("unexpected input '%s'", g_pcSavedInvalidInput); 
	UseAsm_Free(g_pcSavedInvalidInput);
	g_pcSavedInvalidInput = NULL;
	g_uSourceLine++; 
}

void yyeatcppcomment(void)
{
	char c;
	while ((c = input()) != '\n' && c != EOF); 
}

void yyeatcomment(void)
{
	char c, c1;
	IMG_UINT32	uStartSourceLine = g_uSourceLine;

loop:
	while ((c = input()) != '*' && c != EOF)
	{
		if (c == '\n')
		{
			g_uSourceLine++;
		}
	}

	if (c == EOF)
	{
		fprintf(stderr, "%s(%u): error: End of file found inside comment.\n", g_pszInFileName, uStartSourceLine);
		g_uParserError = 1;
		return;
	}

	if ((c1 = input()) != '/' && c != EOF)
	{
		if (c1 == EOF)
		{
			fprintf(stderr, "%s(%u): error: End of file found inside comment.\n", g_pszInFileName, uStartSourceLine);
			g_uParserError = 1;
			return;
		}
		unput(c1);
		goto loop;
	}
}

static IMG_UINT32 ParseMask(IMG_PCHAR pszMask)
{
	IMG_UINT32 uMask = 0;

	pszMask++;
	for (;;pszMask++)
	{
		switch (*pszMask)
		{
			case 'x': uMask |= USEREG_MASK_X; break;
			case 'y': uMask |= USEREG_MASK_Y; break;
			case 'z': uMask |= USEREG_MASK_Z; break;
			case 'w': uMask |= USEREG_MASK_W; break;
			default: return uMask;
		}
	}
}

static IMG_UINT32 ParseTest(IMG_PCHAR pszTest)
{
	IMG_UINT32 uTest = 0;
	switch (pszTest[0])
	{
		case 't': uTest |= USEASM_TEST_SIGN_TRUE << USEASM_TEST_SIGN_SHIFT; break;
		case 'n': uTest |= USEASM_TEST_SIGN_NEGATIVE << USEASM_TEST_SIGN_SHIFT; break;
		case 'p': uTest |= USEASM_TEST_SIGN_POSITIVE << USEASM_TEST_SIGN_SHIFT; break;
	}
	switch (pszTest[1])
	{
		case '&': uTest |= USEASM_TEST_CRCOMB_AND; break;
		case '|': uTest |= USEASM_TEST_CRCOMB_OR; break;
	}
	switch (pszTest[2])
	{
		case 't': uTest |= USEASM_TEST_ZERO_TRUE << USEASM_TEST_ZERO_SHIFT; break;
		case 'n': uTest |= USEASM_TEST_ZERO_NONZERO << USEASM_TEST_ZERO_SHIFT; break;
		case 'z': uTest |= USEASM_TEST_ZERO_ZERO << USEASM_TEST_ZERO_SHIFT; break;
	}
	return uTest;
}

IMG_VOID ParseHashDirective(IMG_PCHAR yytext)
{
	char* temp = yytext + 1;
	while (isspace(*temp)) { temp++; }
	if (strncmp(temp, "line", 4) == 0 || isdigit(*temp))
    {
		char* pszDot;
		if (strncmp(temp, "line", 4) == 0)
		{
			temp += 4;
			while (isspace(*temp)) { temp++; }
		}
		g_uSourceLine = strtoul(temp, &temp, 0);
		while (isspace(*temp)) { temp++; }
		if (*temp == '"')
		{
			char* temp2;
			temp2 = strchr(temp + 1, '"');
			if (temp2 != NULL)
			{
				g_pszInFileName = UseAsm_Malloc(temp2 - temp);
				UseAsm_MemCopy(g_pszInFileName, temp + 1, temp2 - temp - 1);
				g_pszInFileName[temp2 - temp - 1] = 0;
				CollapseStringCodes(g_pszInFileName);
			}
		}
		if ((pszDot = strrchr(g_pszInFileName, '.')) != NULL &&
			(strncmp(pszDot, ".h", strlen(".h")) == 0 ||
			 strncmp(pszDot, ".hpp", strlen(".hpp")) == 0 ||
			 strncmp(pszDot, ".hxx", strlen(".hxx")) == 0))
		{
			BEGIN(CCODE);

			yyaddtoccode(yytext, yyleng);
		}
		else
		{
			BEGIN(INITIAL);
			if (g_pszCCode != NULL)
			{
				yz_scan_bytes(g_pszCCode, g_uCCodeLength);
				/*yzdebug = 1;*/
				yzparse();
				/*DumpGlobalTypes();*/
				UseAsm_Free(g_pszCCode);
				g_pszCCode = NULL;
				g_uCCodeLength = 0;
			}
		}
	 }
	 else
	 {
		g_uSourceLine++; 
		if (YY_START != CCODE)
		{
			BEGIN(INITIAL);
		}
		else
		{
			yyaddtoccode(yytext, yyleng);
		}
	 }
}

