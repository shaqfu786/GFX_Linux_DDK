
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 1 "tools/intern/useasm/use.y"

/*************************************************************************
 * Name         : use.y
 * Title        : Pixel Shader Compiler
 * Author       : John Russell
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
 * $Log: use.y $
 **************************************************************************/

#if defined(_MSC_VER)
#include <malloc.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include <sgxdefs.h>

#include "use.h"
#include "useasm.h"
#include "osglue.h"

#include "use.tab.h"

#if defined(_MSC_VER)
#pragma warning (disable:4131)
#pragma warning (disable:4244)
#pragma warning (disable:4701)
#pragma warning (disable:4127)
#pragma warning (disable:4102)
#pragma warning (disable:4706)
#pragma warning (disable:4702)

/* Specify the malloc and free, to avoid problems with redeclarations on Windows */
#define YYMALLOC malloc
#define YYFREE free

#endif /* defined(_MSC_VER) */

#define YYDEBUG 1
#define YYERROR_VERBOSE

int yylex(void);
static IMG_VOID CheckOpcode(PUSE_INST psInst, IMG_UINT32 uArgCount, IMG_BOOL bRepeatPresent);
static const USE_INST* GetLastInstruction(IMG_VOID);

static IMG_VOID InvalidPair(PUSE_INST psFirst, PUSE_INST psLast, IMG_PCHAR pszError, ...) IMG_FORMAT_PRINTF(3, 4);
static void yyerror(const char *fmt);

/*
 * Functions
 */

static void yyerror(const char *fmt)
{
	fprintf(stderr, "%s(%u): error: ", g_pszInFileName, g_uSourceLine);
	fputs(fmt, stderr);
	fputs(".\n", stderr);

	g_uParserError = 1;
}
	
static IMG_VOID InitialiseDefaultRegister(PUSE_REGISTER psReg)
{
	psReg->uType = USEASM_REGTYPE_MAXIMUM;
	psReg->uNumber = 0;
	psReg->uFlags = 0;
	psReg->uIndex = USEREG_INDEX_NONE;
}

static IMG_VOID ConvertFloatImmediateToArgument(PUSE_REGISTER psReg, IMG_FLOAT fFloatImm)
{
	psReg->uType = USEASM_REGTYPE_FLOATIMMEDIATE;
    psReg->uNumber = *(IMG_UINT32*)&fFloatImm;
    psReg->uIndex = USEREG_INDEX_NONE;
}

static IMG_BOOL InstUsesSpace(PUSE_INST psInst)
{
	switch (psInst->uOpcode)
	{
		case USEASM_OP_AINTRP1:
		case USEASM_OP_AINTRP2:
		case USEASM_OP_AADD:
		case USEASM_OP_ASUB:
		case USEASM_OP_ASOP:
		case USEASM_OP_ARSOP:
		case USEASM_OP_ALRP:
		case USEASM_OP_ASOP2:
		case USEASM_OP_LABEL: return FALSE;
		default:
		{
			if (psInst->uFlags2 & USEASM_OPFLAGS2_COISSUE)
			{
				return FALSE;
			}
			else
			{
				return TRUE;
			}
		}
	}
}

static IMG_BOOL CanSetNoSchedFlag(PUSE_INST psInst)
{
	if (g_sTargetCoreInfo.eID != SGX_CORE_ID_INVALID && IsEnhancedNoSched(g_psTargetCoreDesc))
	{
		if (!OpcodeAcceptsNoSchedEnhanced(psInst->uOpcode))
		{
			return IMG_FALSE;
		}
	}
	else
	{
		if (((psInst->uFlags1 & USEASM_OPFLAGS1_TESTENABLE) &&
			 (psInst->uOpcode != USEASM_OP_MOVC)) ||
			!OpcodeAcceptsNoSched(psInst->uOpcode))
		{
			return FALSE;
		}
		if (psInst->uOpcode == USEASM_OP_MOV && psInst->asArg[0].uType == USEASM_REGTYPE_LINK)
		{
			return FALSE;
		}
		if (psInst->uOpcode == USEASM_OP_MOV && psInst->asArg[1].uType == USEASM_REGTYPE_LINK)
		{
			return FALSE;
		}
	}
	return TRUE;
}

static PUSE_INST StepInst(PUSE_INST psInst)
{
	if (psInst->psNext == NULL)
	{
		return NULL;
	}
	psInst = psInst->psNext;
	while (!InstUsesSpace(psInst))
	{
		psInst = psInst->psNext;
		if (psInst == NULL)
		{
			return NULL;
		}
	}
	return psInst;
}

static IMG_BOOL IsDeactivationPoint(PUSE_INST psInst)
{
	if (psInst->uOpcode == USEASM_OP_BR ||
		psInst->uOpcode == USEASM_OP_BA ||
		psInst->uOpcode == USEASM_OP_WOP ||
		psInst->uOpcode == USEASM_OP_WDF ||
		psInst->uOpcode == USEASM_OP_LOCK ||
		psInst->uOpcode == USEASM_OP_BEXCEPTION ||
		(psInst->uFlags1 & USEASM_OPFLAGS1_SYNCSTART))
	{
		return IMG_TRUE;
	}
	return FALSE;
}

static IMG_VOID SetNoSchedFlagNoPairing(PUSE_INST psFirstInst, PUSE_INST psLastInst)
{
	PUSE_INST psInst;
	IMG_BOOL bFirst;

	/*
	  Nothing to do if there are no instructions in the region.
	*/
	if (psFirstInst == NULL)
	{
		return;
	}

	psInst = psFirstInst->psPrev;
	while (psInst != IMG_NULL && !InstUsesSpace(psInst))
	{
		psInst = psInst->psPrev;
	}

	while (!InstUsesSpace(psLastInst))
	{
		psLastInst = psLastInst->psPrev;
	}

	if (psInst == IMG_NULL)
	{
		AssemblerError(IMG_NULL, psFirstInst, "The .schedoff region starts too early in the program to set the nosched flag - "
					   "try inserting nop instructions or rearranging your code");
		return;
	}

	bFirst = TRUE;
	for (;;)
	{
		PUSE_INST psNextInst;

		psNextInst = StepInst(psInst);

		if (!bFirst && IsDeactivationPoint(psInst))
		{
			AssemblerError(IMG_NULL, psInst, "Instructions which force a deactivation of the slot can't be inside the "
						   ".schedoff region or too close to its start");
		}
		bFirst = FALSE;

		if (psNextInst == psLastInst)
		{
			break;
		}

		if (!CanSetNoSchedFlag(psInst))
		{
			AssemblerError(IMG_NULL, psInst, "This instruction doesn't support "
						   "the nosched flag - try inserting nop instructions or rearranging your code");
		}
		psInst->uFlags1 |= USEASM_OPFLAGS1_NOSCHED;

		psInst = psNextInst;
	}
}

IMG_VOID SetNoSchedFlag(PUSE_INST psFirstInst, IMG_UINT32 uStartOffset, PUSE_INST psLastInst, IMG_UINT32 uLastOffset)
{
	PUSE_INST psFirstInPair;
	PUSE_INST psLastInPair;
	PUSE_INST psFirstPairInRegion;
	PUSE_INST psLastNoSched = NULL;
	IMG_BOOL bSetFlagOnLast;

	if (g_sTargetCoreInfo.eID != SGX_CORE_ID_INVALID && IsEnhancedNoSched(g_psTargetCoreDesc))
	{
		SetNoSchedFlagNoPairing(psFirstInst, psLastInst);
		return;
	}

	/*
	  Nothing to do if there are no instructions in the region.
	*/
	if (psFirstInst == NULL)
	{
		return;
	}
	assert(psLastInst != NULL);
	while (!InstUsesSpace(psLastInst))
	{
		psLastInst = psLastInst->psPrev;
	}

	/*
	  If the nosched region includes only one instruction pair there is no need for the nosched flag.
	*/
	if ((uStartOffset & ~1) == (uLastOffset & ~1))
	{
		return;
	}

	/*
	  Move to the first instruction in the first instruction pair in the nosched region.
	*/
	if ((uStartOffset % 2) == 1)
	{
		psFirstPairInRegion = psFirstInst->psPrev;
		while (psFirstPairInRegion != NULL && !InstUsesSpace(psFirstPairInRegion))
		{
			psFirstPairInRegion = psFirstPairInRegion->psPrev;
		}
	}
	else
	{
		psFirstPairInRegion = psFirstInst;
	}

	/*
	  Get the last instruction of the first instruction pair before the nosched region.
	*/
	if (psFirstPairInRegion == NULL)
	{
		psLastInPair = NULL;
	}
	else
	{
		psLastInPair = psFirstPairInRegion->psPrev;
		if (psLastInPair != NULL)
		{
			while (psLastInPair != NULL && !InstUsesSpace(psLastInPair))
			{
				psLastInPair = psLastInPair->psPrev;
			}
		}
	}

	/*
	  Get the first instruction of the first instruction pair before the nosched region.
	*/
	if (psLastInPair == NULL)
	{
		psFirstInPair = NULL;
	}
	else
	{
		psFirstInPair = psLastInPair->psPrev;
		if (psFirstInPair != NULL)
		{
			while (psFirstInPair != NULL && !InstUsesSpace(psFirstInPair))
			{
				psFirstInPair = psFirstInPair->psPrev;
			}
		}
	}

	/*
	  Check if the nosched region starts too close to the start of the program.
	*/
	if (psLastInPair == NULL)
	{
		AssemblerError(IMG_NULL, psFirstInst, "The .schedoff region starts too early in the program to set the nosched flag - "
					   "try inserting nop instructions or rearranging your code");
		return;
	}

	for(;;)
	{
		/*
		  Check if either of the instructions in the pair are suitable for setting the nosched flag.
		*/
		bSetFlagOnLast = FALSE;
		if (IsDeactivationPoint(psLastInPair))
		{
			AssemblerError(IMG_NULL, psLastInPair, "Instructions which force a deactivation of the slot can't be inside the "
						   ".schedoff region or too close to its start");
		}
		if (CanSetNoSchedFlag(psLastInPair))
		{
			if (!(psLastInPair->uFlags1 & USEASM_OPFLAGS1_NOSCHED))
			{
				bSetFlagOnLast = TRUE;
			}
			psLastInPair->uFlags1 |= USEASM_OPFLAGS1_NOSCHED;
			psLastNoSched = psLastInPair;
		}
		else if (psFirstInPair != NULL && CanSetNoSchedFlag(psFirstInPair))
		{
			if (!(psLastInPair->uFlags1 & USEASM_OPFLAGS1_NOSCHED))
			{
				bSetFlagOnLast = TRUE;
			}
			psFirstInPair->uFlags1 |= USEASM_OPFLAGS1_NOSCHED;
			psLastNoSched = psFirstInPair;
		}
		else
		{
			AssemblerError(IMG_NULL, psFirstInPair, "Neither of the instructions in this instruction pair  "
						   "support the nosched flag - try inserting nop instructions or rearranging your code");
		}

		/*
		  Move to the next instruction pair.
		*/
		psFirstInPair = StepInst(psLastInPair);
		psLastInPair = StepInst(psFirstInPair);

		/*
		  If we have reached the last instruction in the region then exit.
		*/
		if (psFirstInPair == psLastInst || psLastInPair == psLastInst)
		{
			if (bSetFlagOnLast)
			{
				psLastNoSched->uFlags1 &= ~USEASM_OPFLAGS1_NOSCHED;
			}
			break;
		}
	}
}

static IMG_VOID InvalidPair(PUSE_INST psFirst, PUSE_INST psLast, IMG_PCHAR pszError, ...)
{
	if (g_bFixInvalidPairs)
	{
		PUSE_INST psTemp;
		PUSE_INST psInsertPoint;

		/*
		  Insert a NOP to split up the pair.
		*/
		psInsertPoint = psLast;
		while (psInsertPoint->psPrev->uOpcode == USEASM_OP_LABEL)
		{
			psInsertPoint = psInsertPoint->psPrev;
		}

		psTemp = UseAsm_Malloc(sizeof(*psTemp));
		psTemp->uOpcode = USEASM_OP_PADDING;
		psTemp->uSourceLine = psLast->uSourceLine;
		psTemp->pszSourceFile = psLast->pszSourceFile;
		psTemp->uFlags1 = 0;
		psTemp->uFlags2 = 0;
		psTemp->uFlags3 = 0;
		psTemp->uTest = 0;
		CheckOpcode(psTemp, 0, TRUE);
		g_uInstCount++;

		psTemp->psPrev = psInsertPoint->psPrev;
		psTemp->psNext = psInsertPoint;

		psInsertPoint->psPrev->psNext = psTemp;
		psInsertPoint->psPrev = psTemp;
	}
	else
	{
		va_list ap;
		IMG_CHAR szError[256];

		va_start(ap, pszError);
		vsprintf(szError, pszError, ap);
		AssemblerError(IMG_NULL, psFirst, "%s", szError);
		va_end(ap);
	}
}

static IMG_BOOL IsBranchWithLink(PUSE_INST psInst)
{
	if (psInst->uOpcode == USEASM_OP_BEXCEPTION)
	{
		return IMG_TRUE;
	}
	if (psInst->uOpcode == USEASM_OP_BR || psInst->uOpcode == USEASM_OP_BA)
	{
		if (psInst->uFlags1 & USEASM_OPFLAGS1_SAVELINK)
		{
			return IMG_TRUE;
		}
	}
	return IMG_FALSE;
}

static IMG_VOID CheckPairing(PUSE_INST psLast)
{
	PUSE_INST psFirst;

	psFirst = psLast->psPrev;
	while (psFirst != NULL)
	{
		if (InstUsesSpace(psFirst))
		{
			break;
		}
		psFirst = psFirst->psPrev;
	}
	if (psFirst == NULL)
	{
		return;
	}

	/*
	  1)	An LAPC, BA or BR instruction, followed by an LAPC, BA or BR instruction.
	  2)	An SETL instruction followed by an SAVL instruction.
	  3)	An SETL instruction followed by an LAPC instruction
	  4)	An SETL followed by a BA or BR instruction with the link bit set.
	  5)	A BA or BR instruction with the link bit set followed by an SAVL instruction.
	  6)	Two PCOEFF instructions.
	*/

	if ((psFirst->uOpcode == USEASM_OP_LAPC ||
		 psFirst->uOpcode == USEASM_OP_BA ||
		 psFirst->uOpcode == USEASM_OP_BR ||
		 psFirst->uOpcode == USEASM_OP_BEXCEPTION) &&
		(psLast->uOpcode == USEASM_OP_LAPC ||
		 psLast->uOpcode == USEASM_OP_BA ||
		 psLast->uOpcode == USEASM_OP_BR ||
		 psLast->uOpcode == USEASM_OP_BEXCEPTION))
	{
		InvalidPair(psFirst, psLast,
					"A %s instruction followed by a %s instruction is illegal as an instruction pair",
                    OpcodeName(psFirst->uOpcode),
					OpcodeName(psLast->uOpcode));
	}
	if (psFirst->uOpcode == USEASM_OP_MOV && psFirst->asArg[0].uType == USEASM_REGTYPE_LINK)
	{
		if (psLast->uOpcode == USEASM_OP_MOV && psLast->asArg[1].uType == USEASM_REGTYPE_LINK)
		{
			InvalidPair(psFirst, psLast,
						"A move to the link register followed by a move from the link register "
                        "is illegal as an instruction pair");
		}
		if (psLast->uOpcode == USEASM_OP_LAPC)
		{
			InvalidPair(psFirst, psLast,
						"A move to the link register followed by an lapc instruction is illegal "
                        "as an instruction pair");
		}
		if (IsBranchWithLink(psLast))
		{
			InvalidPair(psFirst, psLast,
						"A move to the link register followed by a brl or bal instruction is illegal "
                        "as an instruction pair");
		}
	}
	if (IsBranchWithLink(psFirst) &&
		psLast->uOpcode == USEASM_OP_MOV &&
		psLast->asArg[1].uType == USEASM_REGTYPE_LINK)
	{
		InvalidPair(psFirst, psLast,
					"A brl or bal instruction following by a mov from the link register is illegal "
                    "as an instruction pair");
	}
	if (psFirst->uOpcode == USEASM_OP_PCOEFF && psLast->uOpcode == USEASM_OP_PCOEFF)
	{
		InvalidPair(psFirst, psLast,
					"Two pcoeff instructions in an instruction pair is illegal");
	}
	if (
			(psFirst->uOpcode == USEASM_OP_LOCK || psFirst->uOpcode == USEASM_OP_RELEASE) &&
			(psLast->uOpcode == USEASM_OP_LOCK || psLast->uOpcode == USEASM_OP_RELEASE)
	   )
	{
		InvalidPair(psFirst, psLast,
					"An instruction pair can only contain one lock or release instruction");
	}
}

static IMG_VOID InsertTestDummyDestination(PUSE_INST psInst, IMG_PUINT32 puArgCount)
{
	IMG_UINT32 uMask = (psInst->uTest & ~USEASM_TEST_MASK_CLRMSK) >> USEASM_TEST_MASK_SHIFT;
	if ((psInst->uFlags1 & USEASM_OPFLAGS1_TESTENABLE) && uMask == USEASM_TEST_MASK_NONE)
	{
		if (psInst->asArg[0].uType != USEASM_REGTYPE_PREDICATE)
		{
			(*puArgCount)--;
		}
		else
		{
			memmove(psInst->asArg + 1, psInst->asArg, (*puArgCount) * sizeof(psInst->asArg[0]));
			psInst->asArg[0].uType = USEASM_REGTYPE_TEMP;
			psInst->asArg[0].uNumber = 0;
			psInst->asArg[0].uIndex = USEREG_INDEX_NONE;
			psInst->asArg[0].uFlags = USEASM_ARGFLAGS_DISABLEWB;
		}
	}
}

static IMG_VOID CheckOpcode(PUSE_INST psInst, IMG_UINT32 uArgCount, IMG_BOOL bRepeatPresent)
{
	if (g_uSchedOffCount> 0)
	{
		g_uLastNoSchedInstOffset = g_uInstOffset;
	}
	if (InstUsesSpace(psInst))
	{
		g_uInstOffset++;
	}
	if (g_uSkipInvOnCount> 0)
	{
		psInst->uFlags3 |= USEASM_OPFLAGS3_SKIPINVALIDON;
	}
	if (!bRepeatPresent && g_uRepeatStackTop> 0)
	{
		if (psInst->uOpcode != USEASM_OP_LABEL)
		{
			psInst->uFlags1 |= (g_puRepeatStack[g_uRepeatStackTop - 1] << USEASM_OPFLAGS1_REPEAT_SHIFT);
		}
	}
	switch (psInst->uOpcode)
	{
		case USEASM_OP_LDAB:
		case USEASM_OP_LDAW:
		case USEASM_OP_LDAD:
		case USEASM_OP_LDAQ:
		case USEASM_OP_LDLB:
		case USEASM_OP_LDLW:
		case USEASM_OP_LDLD:
		case USEASM_OP_LDLQ:
		case USEASM_OP_LDTB:
		case USEASM_OP_LDTW:
		case USEASM_OP_LDTD:
		case USEASM_OP_LDTQ:
		{
			if (psInst->uFlags1 & USEASM_OPFLAGS1_TESTENABLE)
			{
				AssemblerError(IMG_NULL, psInst, "Load and stores can't have an attached test");
			}
			/* LD DEST, BASE, OFFSET, DRCN -> LD DEST, BASE, OFFSET, LIMIT=#0, DRCN */
			if (uArgCount == (OpcodeArgumentCount(psInst->uOpcode) -  1))
			{
				memmove(&psInst->asArg[4], &psInst->asArg[3], (uArgCount - 3) * sizeof(psInst->asArg[0]));
				psInst->asArg[3].uType = USEASM_REGTYPE_IMMEDIATE;
				psInst->asArg[3].uNumber = 0;
				psInst->asArg[3].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[3].uFlags = 0;
				uArgCount++;
			}
			else
			{
				psInst->uFlags1 |= USEASM_OPFLAGS1_RANGEENABLE;
			}
			break;
		}
		case USEASM_OP_STAB:
		case USEASM_OP_STAW:
		case USEASM_OP_STAD:
		case USEASM_OP_STAQ:
		case USEASM_OP_STLB:
		case USEASM_OP_STLW:
		case USEASM_OP_STLD:
		case USEASM_OP_STLQ:
		case USEASM_OP_STTB:
		case USEASM_OP_STTW:
		case USEASM_OP_STTD:
		case USEASM_OP_STTQ:
		{
			if (psInst->uFlags1 & USEASM_OPFLAGS1_TESTENABLE)
			{
				AssemblerError(IMG_NULL, psInst, "Load and stores can't have an attached test");
			}
			break;
		}
		case USEASM_OP_ELDD:
		case USEASM_OP_ELDQ:
		{
			if (psInst->uFlags1 & USEASM_OPFLAGS1_TESTENABLE)
			{
				AssemblerError(IMG_NULL, psInst, "Load and stores can't have an attached test");
			}
			break;
		}
		case USEASM_OP_DEPTHF:
		{
			if (uArgCount == 3)
			{
				psInst->asArg[3].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[3].uNumber = USEASM_INTSRCSEL_NONE;
				psInst->asArg[3].uFlags = 0;
				psInst->asArg[3].uIndex = USEREG_INDEX_NONE;

				psInst->asArg[4].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[4].uNumber = USEASM_INTSRCSEL_FEEDBACK;
				psInst->asArg[4].uFlags = 0;
				psInst->asArg[4].uIndex = USEREG_INDEX_NONE;

				uArgCount += 2;
			}
			else if (uArgCount == 4)
			{
				if (psInst->asArg[3].uType == USEASM_REGTYPE_INTSRCSEL &&
					(psInst->asArg[3].uNumber == USEASM_INTSRCSEL_TWOSIDED ||
					 psInst->asArg[3].uNumber == USEASM_INTSRCSEL_NONE))
				{
					psInst->asArg[4].uType = USEASM_REGTYPE_INTSRCSEL;
					psInst->asArg[4].uNumber = USEASM_INTSRCSEL_FEEDBACK;
					psInst->asArg[4].uFlags = 0;
					psInst->asArg[4].uIndex = USEREG_INDEX_NONE;
				}
				else
				{
					memmove(psInst->asArg + 4, psInst->asArg + 3, (uArgCount - 3) * sizeof(psInst->asArg[0]));
					psInst->asArg[3].uType = USEASM_REGTYPE_INTSRCSEL;
					psInst->asArg[3].uNumber = USEASM_INTSRCSEL_NONE;
					psInst->asArg[3].uFlags = 0;
					psInst->asArg[3].uIndex = USEREG_INDEX_NONE;
				}
				uArgCount++;
			}
			break;
		}
		case USEASM_OP_ATST8:
		{
			if (uArgCount < 7 && psInst->asArg[1].uType != USEASM_REGTYPE_PREDICATE)
			{
				memmove(psInst->asArg + 2, psInst->asArg + 1, (uArgCount - 1) * sizeof(psInst->asArg[0]));
				psInst->asArg[1].uType = USEASM_REGTYPE_PREDICATE;
				psInst->asArg[1].uNumber = 0;
				psInst->asArg[1].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[1].uFlags = USEASM_ARGFLAGS_DISABLEWB;
				uArgCount++;
			}
			if (uArgCount == 5)
			{
				psInst->asArg[5].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[5].uNumber = USEASM_INTSRCSEL_NONE;
				psInst->asArg[5].uFlags = 0;
				psInst->asArg[5].uIndex = USEREG_INDEX_NONE;

				psInst->asArg[6].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[6].uNumber = USEASM_INTSRCSEL_FEEDBACK;
				psInst->asArg[6].uFlags = 0;
				psInst->asArg[6].uIndex = USEREG_INDEX_NONE;

				uArgCount += 2;
			}
			else if (uArgCount == 6)
			{
				if (psInst->asArg[5].uType == USEASM_REGTYPE_INTSRCSEL &&
					(psInst->asArg[5].uNumber == USEASM_INTSRCSEL_TWOSIDED ||
					 psInst->asArg[5].uNumber == USEASM_INTSRCSEL_NONE))
				{
					psInst->asArg[6].uType = USEASM_REGTYPE_INTSRCSEL;
					psInst->asArg[6].uNumber = USEASM_INTSRCSEL_FEEDBACK;
					psInst->asArg[6].uFlags = 0;
					psInst->asArg[6].uIndex = USEREG_INDEX_NONE;
				}
				else
				{
					memmove(psInst->asArg + 6, psInst->asArg + 5, (uArgCount - 5) * sizeof(psInst->asArg[0]));
					psInst->asArg[5].uType = USEASM_REGTYPE_INTSRCSEL;
					psInst->asArg[5].uNumber = USEASM_INTSRCSEL_NONE;
					psInst->asArg[5].uFlags = 0;
					psInst->asArg[5].uIndex = USEREG_INDEX_NONE;
				}
				uArgCount++;
			}
			break;
		}
		case USEASM_OP_FIRH:
		{

			break;
		}
		case USEASM_OP_FIRVH:
		{
			if (uArgCount == 3)
			{
				psInst->asArg[3].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[3].uNumber = USEASM_INTSRCSEL_NONE;
				psInst->asArg[3].uFlags = 0;
				psInst->asArg[3].uIndex = USEREG_INDEX_NONE;
				uArgCount++;
			}
			if (uArgCount == 4)
			{
				if (psInst->asArg[3].uType == USEASM_REGTYPE_INTSRCSEL &&
					(psInst->asArg[3].uNumber == USEASM_INTSRCSEL_IADD ||
					 psInst->asArg[3].uNumber == USEASM_INTSRCSEL_SCALE))
				{
					psInst->asArg[4] = psInst->asArg[3];
					psInst->asArg[3].uType = USEASM_REGTYPE_INTSRCSEL;
					psInst->asArg[3].uNumber = USEASM_INTSRCSEL_NONE;
					psInst->asArg[3].uFlags = 0;
					psInst->asArg[3].uIndex = USEREG_INDEX_NONE;
				}
				else
				{
					psInst->asArg[4].uType = USEASM_REGTYPE_INTSRCSEL;
					psInst->asArg[4].uNumber = USEASM_INTSRCSEL_NONE;
					psInst->asArg[4].uFlags = 0;
					psInst->asArg[4].uIndex = USEREG_INDEX_NONE;
				}
				uArgCount++;
			}
			if (uArgCount == 5)
			{
				if (psInst->asArg[4].uType == USEASM_REGTYPE_INTSRCSEL &&
					psInst->asArg[4].uNumber == USEASM_INTSRCSEL_SCALE)
				{
					if (psInst->asArg[3].uType == USEASM_REGTYPE_INTSRCSEL &&
						psInst->asArg[3].uNumber == USEASM_INTSRCSEL_IADD)
					{
						psInst->asArg[5] = psInst->asArg[4];
						psInst->asArg[4] = psInst->asArg[3];
						psInst->asArg[3].uType = USEASM_REGTYPE_INTSRCSEL;
						psInst->asArg[3].uNumber = USEASM_INTSRCSEL_NONE;
						psInst->asArg[3].uFlags = 0;
						psInst->asArg[3].uIndex = USEREG_INDEX_NONE;
					}
					else
					{
						psInst->asArg[5] = psInst->asArg[4];
						psInst->asArg[4].uType = USEASM_REGTYPE_INTSRCSEL;
						psInst->asArg[4].uNumber = USEASM_INTSRCSEL_NONE;
						psInst->asArg[4].uFlags = 0;
						psInst->asArg[4].uIndex = USEREG_INDEX_NONE;
					}
				}
				else
				{
					psInst->asArg[5].uType = USEASM_REGTYPE_INTSRCSEL;
					psInst->asArg[5].uNumber = USEASM_INTSRCSEL_NONE;
					psInst->asArg[5].uFlags = 0;
					psInst->asArg[5].uIndex = USEREG_INDEX_NONE;
				}
				uArgCount++;
			}
			break;
		}
		case USEASM_OP_FIRV:
		{
			if (uArgCount == 5)
			{
				psInst->asArg[5].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[5].uNumber = USEASM_INTSRCSEL_NONE;
				psInst->asArg[5].uFlags = 0;
				psInst->asArg[5].uIndex = USEREG_INDEX_NONE;
				uArgCount++;
			}
			if (uArgCount == 6)
			{
				if (psInst->asArg[5].uType == USEASM_REGTYPE_INTSRCSEL &&
					(psInst->asArg[5].uNumber == USEASM_INTSRCSEL_IADD ||
					 psInst->asArg[5].uNumber == USEASM_INTSRCSEL_SCALE))
				{
					psInst->asArg[6] = psInst->asArg[5];
					psInst->asArg[5].uType = USEASM_REGTYPE_INTSRCSEL;
					psInst->asArg[5].uNumber = USEASM_INTSRCSEL_NONE;
					psInst->asArg[5].uFlags = 0;
					psInst->asArg[5].uIndex = USEREG_INDEX_NONE;
				}
				else
				{
					psInst->asArg[6].uType = USEASM_REGTYPE_INTSRCSEL;
					psInst->asArg[6].uNumber = USEASM_INTSRCSEL_NONE;
					psInst->asArg[6].uFlags = 0;
					psInst->asArg[6].uIndex = USEREG_INDEX_NONE;
				}
				uArgCount++;
			}
			if (uArgCount == 7)
			{
				if (psInst->asArg[6].uType == USEASM_REGTYPE_INTSRCSEL &&
					psInst->asArg[6].uNumber == USEASM_INTSRCSEL_SCALE)
				{
					if (psInst->asArg[5].uType == USEASM_REGTYPE_INTSRCSEL &&
						psInst->asArg[5].uNumber == USEASM_INTSRCSEL_IADD)
					{
						psInst->asArg[7] = psInst->asArg[6];
						psInst->asArg[6] = psInst->asArg[5];
						psInst->asArg[5].uType = USEASM_REGTYPE_INTSRCSEL;
						psInst->asArg[5].uNumber = USEASM_INTSRCSEL_NONE;
						psInst->asArg[5].uFlags = 0;
						psInst->asArg[5].uIndex = USEREG_INDEX_NONE;
					}
					else
					{
						psInst->asArg[7] = psInst->asArg[6];
						psInst->asArg[6].uType = USEASM_REGTYPE_INTSRCSEL;
						psInst->asArg[6].uNumber = USEASM_INTSRCSEL_NONE;
						psInst->asArg[6].uFlags = 0;
						psInst->asArg[6].uIndex = USEREG_INDEX_NONE;
					}
				}
				else
				{
					psInst->asArg[7].uType = USEASM_REGTYPE_INTSRCSEL;
					psInst->asArg[7].uNumber = USEASM_INTSRCSEL_NONE;
					psInst->asArg[7].uFlags = 0;
					psInst->asArg[7].uIndex = USEREG_INDEX_NONE;
				}
				uArgCount++;
			}
			break;
		}
		case USEASM_OP_U8DOT3:
		case USEASM_OP_U8DOT4:
		case USEASM_OP_U8DOT3OFF:
		case USEASM_OP_U8DOT4OFF:
		case USEASM_OP_U16DOT3:
		case USEASM_OP_U16DOT4:
		case USEASM_OP_U16DOT3OFF:
		case USEASM_OP_U16DOT4OFF:
		{
			if (uArgCount == 3)
			{
				memmove(psInst->asArg + 2, psInst->asArg + 1, uArgCount * sizeof(psInst->asArg[0]));
				psInst->asArg[1].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[1].uNumber = USEASM_INTSRCSEL_CX1;
				psInst->asArg[1].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[1].uFlags = 0;
				uArgCount++;
			}
			if (uArgCount == 4)
			{
				memmove(psInst->asArg + 3, psInst->asArg + 2, uArgCount * sizeof(psInst->asArg[0]));
				psInst->asArg[2].uType = USEASM_REGTYPE_INTSRCSEL;
				switch (psInst->asArg[1].uNumber)
				{
					case USEASM_INTSRCSEL_CX1: psInst->asArg[2].uNumber = USEASM_INTSRCSEL_AX1; break;
					case USEASM_INTSRCSEL_CX2: psInst->asArg[2].uNumber = USEASM_INTSRCSEL_AX2; break;
					case USEASM_INTSRCSEL_CX4: psInst->asArg[2].uNumber = USEASM_INTSRCSEL_AX4; break;
					case USEASM_INTSRCSEL_CX8: psInst->asArg[2].uNumber = USEASM_INTSRCSEL_AX8; break;
				}
				psInst->asArg[2].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[2].uFlags = 0;
				uArgCount++;
			}
			break;
		}
		case USEASM_OP_SOP2:
		{
			if (uArgCount> 4 &&
				(psInst->asArg[3].uType != USEASM_REGTYPE_INTSRCSEL ||
				 (psInst->asArg[3].uNumber != USEASM_INTSRCSEL_COMP &&
				  psInst->asArg[3].uNumber != USEASM_INTSRCSEL_NONE)))
			{
				memmove(psInst->asArg + 4, psInst->asArg + 3, (uArgCount - 3) * sizeof(psInst->asArg[0]));
				psInst->asArg[3].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[3].uNumber = USEASM_INTSRCSEL_NONE;
				psInst->asArg[3].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[3].uFlags = 0;
				uArgCount++;
			}
			break;
		}
		case USEASM_OP_ASOP:
		{
			if (uArgCount == 2)
			{
				psInst->asArg[3].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[3].uNumber = USEASM_INTSRCSEL_NONE;
				psInst->asArg[3].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[3].uFlags = 0;
				uArgCount++;
			}
			break;
		}
		case USEASM_OP_ASOP2:
		{
			if (
				(
					uArgCount == 3 ||
					uArgCount == 4
					) &&
				!(
					psInst->asArg[0].uType == USEASM_REGTYPE_INTSRCSEL &&
					psInst->asArg[0].uIndex == USEREG_INDEX_NONE &&
					psInst->asArg[0].uFlags == 0 &&
					(
						psInst->asArg[0].uNumber == USEASM_INTSRCSEL_NONE ||
						psInst->asArg[0].uNumber == USEASM_INTSRCSEL_COMP
						)
					)
                )
			{
				memmove(psInst->asArg + 1, psInst->asArg + 0, uArgCount * sizeof(psInst->asArg[0]));
				psInst->asArg[0].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[0].uNumber = USEASM_INTSRCSEL_NONE;
				psInst->asArg[0].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[0].uFlags = 0;
				uArgCount++;
			}
			if (uArgCount == 4)
			{
				psInst->asArg[4].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[4].uNumber = USEASM_INTSRCSEL_NONE;
				psInst->asArg[4].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[4].uFlags = 0;
				uArgCount++;
			}
			break;
		}
		case USEASM_OP_SOP3:
		{
			if (uArgCount == 7)
			{
				psInst->asArg[7].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[7].uNumber = USEASM_INTSRCSEL_NONE;
				psInst->asArg[7].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[7].uFlags = 0;
				uArgCount++;
			}
			break;
		}
		case USEASM_OP_IMA16:
		{
			if (uArgCount == 4)
			{
				psInst->asArg[4].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[4].uNumber = USEASM_INTSRCSEL_U16;
				psInst->asArg[4].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[4].uFlags = 0;
				uArgCount++;
			}
			if (uArgCount == 5)
			{
				psInst->asArg[5].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[5].uNumber = USEASM_INTSRCSEL_U16;
				psInst->asArg[5].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[5].uFlags = 0;
				uArgCount++;
			}
			break;
		}
		case USEASM_OP_SSUM16:
		{
			IMG_UINT32 uModeCount = 0;
			IMG_BOOL bIDstPresent = IMG_FALSE, bIAddPresent = IMG_FALSE, bRoundModePresent = IMG_FALSE, bGPIPresent = IMG_FALSE;
			IMG_BOOL bSrcFormatPresent = IMG_FALSE;
			IMG_UINT32 uArg;
			for (uArg = 4; uArg < uArgCount; uArg++)
			{
				if (psInst->asArg[uArg].uType == USEASM_REGTYPE_INTSRCSEL)
				{
					if (psInst->asArg[uArg].uNumber == USEASM_INTSRCSEL_IDST)
					{
						if (!bIDstPresent)
						{
							uModeCount++;
						}
						bIDstPresent = IMG_TRUE;
					}
					else if (psInst->asArg[uArg].uNumber == USEASM_INTSRCSEL_IADD)
					{
						if (!bIAddPresent)
						{
							uModeCount++;
						}
						bIAddPresent = IMG_TRUE;
					}
					else if (psInst->asArg[uArg].uNumber == USEASM_INTSRCSEL_ROUNDDOWN ||
							 psInst->asArg[uArg].uNumber == USEASM_INTSRCSEL_ROUNDNEAREST ||
							 psInst->asArg[uArg].uNumber == USEASM_INTSRCSEL_ROUNDUP)
					{
						if (!bRoundModePresent)
						{
							uModeCount++;
						}
						bRoundModePresent = IMG_TRUE;
					}
					else if (psInst->asArg[uArg].uNumber == USEASM_INTSRCSEL_U8 ||
							 psInst->asArg[uArg].uNumber == USEASM_INTSRCSEL_S8)
					{
						if (!bSrcFormatPresent)
						{
							uModeCount++;
						}
						bSrcFormatPresent = IMG_TRUE;
					}
				}
				if (psInst->asArg[uArg].uType == USEASM_REGTYPE_FPINTERNAL)
				{
					if (!bGPIPresent)
					{
						uModeCount++;
					}
					bGPIPresent = IMG_TRUE;
				}
			}
			if ((uModeCount + 4) != uArgCount)
			{
				AssemblerError(IMG_NULL, psInst, "Unknown instruction modes for ssum16");
			}
			if (!bIDstPresent)
			{
				memmove(&psInst->asArg[5], &psInst->asArg[4], sizeof(USE_REGISTER) * 4);
				psInst->asArg[4].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[4].uNumber = USEASM_INTSRCSEL_NONE;
				psInst->asArg[4].uFlags = 0;
				psInst->asArg[4].uIndex = USEREG_INDEX_NONE;
			}
			if (!bIAddPresent)
			{
				memmove(&psInst->asArg[6], &psInst->asArg[5], sizeof(USE_REGISTER) * 3);
				psInst->asArg[5].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[5].uNumber = USEASM_INTSRCSEL_NONE;
				psInst->asArg[5].uFlags = 0;
				psInst->asArg[5].uIndex = USEREG_INDEX_NONE;
			}
			if (!bGPIPresent)
			{
				memmove(&psInst->asArg[7], &psInst->asArg[6], sizeof(USE_REGISTER) * 2);
				psInst->asArg[6].uType = USEASM_REGTYPE_FPINTERNAL;
				psInst->asArg[6].uNumber = 0;
				psInst->asArg[6].uFlags = 0;
				psInst->asArg[6].uIndex = USEREG_INDEX_NONE;
			}
			if (!bRoundModePresent)
			{
				psInst->asArg[8] = psInst->asArg[7];
				psInst->asArg[7].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[7].uNumber = USEASM_INTSRCSEL_ROUNDNEAREST;
				psInst->asArg[7].uFlags = 0;
				psInst->asArg[7].uIndex = USEREG_INDEX_NONE;
			}
			if (!bSrcFormatPresent)
			{
				psInst->asArg[8].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[8].uNumber = USEASM_INTSRCSEL_U8;
				psInst->asArg[8].uFlags = 0;
				psInst->asArg[8].uIndex = USEREG_INDEX_NONE;
			}
			uArgCount = 9;
			break;
		}
		case USEASM_OP_IMAE:
		{
			if (uArgCount == 5)
			{
				psInst->asArg[5].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[5].uNumber = USEASM_INTSRCSEL_NONE;
				psInst->asArg[5].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[5].uFlags = 0;
				uArgCount++;
			}
			if (uArgCount == 6)
			{
				if (psInst->asArg[5].uType == USEASM_REGTYPE_INTSRCSEL &&
					psInst->asArg[5].uNumber == USEASM_INTSRCSEL_CINENABLE)
				{
					psInst->asArg[6] = psInst->asArg[5];
					psInst->asArg[5].uType = USEASM_REGTYPE_INTSRCSEL;
					psInst->asArg[5].uNumber = USEASM_INTSRCSEL_NONE;
					psInst->asArg[5].uIndex = USEREG_INDEX_NONE;
					psInst->asArg[5].uFlags = 0;
					uArgCount++;
				}
				else
				{
					psInst->asArg[6].uType = USEASM_REGTYPE_INTSRCSEL;
					psInst->asArg[6].uNumber = USEASM_INTSRCSEL_NONE;
					psInst->asArg[6].uIndex = USEREG_INDEX_NONE;
					psInst->asArg[6].uFlags = 0;
					uArgCount++;
				}
			}
			if (uArgCount == 7)
			{
				if (psInst->asArg[6].uType == USEASM_REGTYPE_FPINTERNAL)
				{
					if (psInst->asArg[5].uType == USEASM_REGTYPE_INTSRCSEL &&
						psInst->asArg[5].uNumber == USEASM_INTSRCSEL_CINENABLE)
					{
						psInst->asArg[7] = psInst->asArg[6];
						psInst->asArg[6] = psInst->asArg[5];
						psInst->asArg[5].uType = USEASM_REGTYPE_INTSRCSEL;
						psInst->asArg[5].uNumber = USEASM_INTSRCSEL_NONE;
						psInst->asArg[5].uIndex = USEREG_INDEX_NONE;
						psInst->asArg[5].uFlags = 0;
						uArgCount++;
					}
					else
					{
						psInst->asArg[7] = psInst->asArg[6];
						psInst->asArg[6].uType = USEASM_REGTYPE_INTSRCSEL;
						psInst->asArg[6].uNumber = USEASM_INTSRCSEL_NONE;
						psInst->asArg[6].uIndex = USEREG_INDEX_NONE;
						psInst->asArg[6].uFlags = 0;
						uArgCount++;
					}
				}
				else
				{
					if ((psInst->asArg[5].uType == USEASM_REGTYPE_INTSRCSEL &&
						 psInst->asArg[5].uNumber == USEASM_INTSRCSEL_CINENABLE) ||
						(psInst->asArg[6].uType == USEASM_REGTYPE_INTSRCSEL &&
						 psInst->asArg[6].uNumber == USEASM_INTSRCSEL_COUTENABLE))
					{
						AssemblerError(IMG_NULL, psInst, "imae with carry in or carry out enabled should have a carry source/destination as the last argument");
					}
					else
					{
						psInst->asArg[7].uType = USEASM_REGTYPE_IMMEDIATE;
						psInst->asArg[7].uNumber = 0;
						psInst->asArg[7].uIndex = USEREG_INDEX_NONE;
						psInst->asArg[7].uFlags = 0;
						uArgCount++;
					}
				}
			}
			break;
		}
		case USEASM_OP_IMOV16:
		{
			if (uArgCount == 2)
			{
				psInst->asArg[2] = psInst->asArg[1];
				psInst->asArg[2].uFlags |= USEASM_ARGFLAGS_HIGH;
				uArgCount++;
			}
			break;
		}
		case USEASM_OP_BILIN:
		{
			if (uArgCount == 8)
			{
				psInst->asArg[8].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[8].uNumber = USEASM_INTSRCSEL_NONE;
				psInst->asArg[8].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[8].uFlags = 0;
				uArgCount++;
			}
			break;
		}
		case USEASM_OP_IMA32:
		{
			if (!(uArgCount >= 2 &&
				  psInst->asArg[uArgCount - 1].uType == USEASM_REGTYPE_FPINTERNAL &&
				  psInst->asArg[uArgCount - 2].uType == USEASM_REGTYPE_INTSRCSEL &&
				  psInst->asArg[uArgCount - 2].uNumber == USEASM_INTSRCSEL_CINENABLE))
			{
				psInst->asArg[uArgCount].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[uArgCount].uNumber = USEASM_INTSRCSEL_NONE;
				psInst->asArg[uArgCount].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[uArgCount].uFlags = 0;
				uArgCount++;
				psInst->asArg[uArgCount].uType = USEASM_REGTYPE_FPINTERNAL;
				psInst->asArg[uArgCount].uNumber = 0;
				psInst->asArg[uArgCount].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[uArgCount].uFlags = 0;
				uArgCount++;
			}
			if (uArgCount == 6)
			{
				memmove(&psInst->asArg[2], &psInst->asArg[1], sizeof(USE_REGISTER) * (uArgCount - 1));
				psInst->asArg[1].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[1].uNumber = USEASM_INTSRCSEL_NONE;
				psInst->asArg[1].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[1].uFlags = 0;
				uArgCount++;
			}
			break;
		}
		case USEASM_OP_SMLSI:
		case USEASM_OP_SMBO:
		case USEASM_OP_SMOA:
		case USEASM_OP_SMR:
		{
			if (psInst->asArg[0].uType != USEASM_REGTYPE_IMMEDIATE &&
				psInst->asArg[0].uType != USEASM_REGTYPE_SWIZZLE)
			{
				IMG_UINT32 uArg;
				if (uArgCount != 2)
				{
					AssemblerError(IMG_NULL, psInst, "Too %s arguments (was %d; should be %d) for instruction", (uArgCount < 2) ? "few" : "many", uArgCount, 2);
				}
				for (uArg = 2; uArg < OpcodeArgumentCount(psInst->uOpcode); uArg++)
				{
					psInst->asArg[uArg].uType = USEASM_REGTYPE_IMMEDIATE;
					psInst->asArg[uArg].uNumber = 0;
					psInst->asArg[uArg].uIndex = USEREG_INDEX_NONE;
					psInst->asArg[uArg].uFlags = 0;
					uArgCount++;
				}
			}
			break;
		}
		case USEASM_OP_FDSX:
		case USEASM_OP_FDSY:
		{
			if (psInst->uFlags1 & USEASM_OPFLAGS1_TESTENABLE)
			{
				if (psInst->asArg[0].uType != USEASM_REGTYPE_PREDICATE)
				{
					uArgCount--;
				}
				else
				{
					memmove(psInst->asArg + 1, psInst->asArg, uArgCount * sizeof(psInst->asArg[0]));
					psInst->asArg[0].uType = USEASM_REGTYPE_TEMP;
					psInst->asArg[0].uNumber = 0;
					psInst->asArg[0].uIndex = USEREG_INDEX_NONE;
					psInst->asArg[0].uFlags = USEASM_ARGFLAGS_DISABLEWB;
				}
				if (uArgCount == 2)
				{
					psInst->asArg[3] = psInst->asArg[2];
					uArgCount++;
				}
			}
			else
			{
				if (uArgCount == 2)
				{
					psInst->asArg[2] = psInst->asArg[1];
					uArgCount++;
				}
			}
			break;
		}
		case USEASM_OP_MOVC:
		{
			break;
		}
		case USEASM_OP_FNRM16:
		case USEASM_OP_FNRM32:
		{
			if (
				!(
					psInst->asArg[4].uType == USEASM_REGTYPE_INTSRCSEL &&
					psInst->asArg[4].uNumber == USEASM_INTSRCSEL_SRCNEG
					)
                )
			{
				psInst->asArg[6] = psInst->asArg[5];
				psInst->asArg[5] = psInst->asArg[4];
				psInst->asArg[4].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[4].uNumber = USEASM_INTSRCSEL_NONE;
				psInst->asArg[4].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[4].uFlags = 0;
				uArgCount++;
			}
			if (
				!(
					psInst->asArg[5].uType == USEASM_REGTYPE_INTSRCSEL &&
					psInst->asArg[5].uNumber == USEASM_INTSRCSEL_SRCABS
					)
                )
			{
				psInst->asArg[6] = psInst->asArg[5];
				psInst->asArg[5].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[5].uNumber = USEASM_INTSRCSEL_NONE;
				psInst->asArg[5].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[5].uFlags = 0;
				uArgCount++;
			}
			break;
		}
		case USEASM_OP_EMITPIXEL1:
		case USEASM_OP_EMITPIXEL2:
		case USEASM_OP_EMITPIXEL:
		case USEASM_OP_EMITVCBVERTEX:
		case USEASM_OP_EMITVCBSTATE:
		case USEASM_OP_EMITMTEVERTEX:
		case USEASM_OP_EMITMTESTATE:
		{
			/*
			  The INCP field can be optionally supplied as an immediate
			  first argument. If missing, then add a default of 0 here.
			*/
			if (psInst->asArg[0].uType != USEASM_REGTYPE_IMMEDIATE)
			{
				if (
					(
						(psInst->uOpcode == USEASM_OP_EMITPIXEL2 || psInst->uOpcode == USEASM_OP_EMITPIXEL) &&
						uArgCount == (OpcodeArgumentCount(psInst->uOpcode) - 2)
						) ||
					(
						uArgCount == (OpcodeArgumentCount(psInst->uOpcode) - 1)
						)
                    )
				{
					memmove(psInst->asArg + 1, psInst->asArg, uArgCount * sizeof(psInst->asArg[0]));
					psInst->asArg[0].uType = USEASM_REGTYPE_IMMEDIATE;
					psInst->asArg[0].uNumber = 0;
					psInst->asArg[0].uIndex = USEREG_INDEX_NONE;
					psInst->asArg[0].uFlags = 0;
					uArgCount++;
				}
			}
			/*
			  If the last non-sideband source isn't present then default to #0.
			  
					INCP, SRC0, SRC1, SIDEBAND
				->
					INCP, SRC0, SRC1, #0, SIDEBAND
			*/
			if (
				(psInst->uOpcode == USEASM_OP_EMITPIXEL2 || psInst->uOpcode == USEASM_OP_EMITPIXEL) &&
				uArgCount == 4 &&
				psInst->asArg[3].uType == USEASM_REGTYPE_IMMEDIATE
                )
			{
				psInst->asArg[4] = psInst->asArg[3];
				psInst->asArg[3].uType = USEASM_REGTYPE_IMMEDIATE;
				psInst->asArg[3].uNumber = 0;
				psInst->asArg[3].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[3].uFlags = 0;
				uArgCount++;
			}
			break;
		}
		case USEASM_OP_PCKF16F32:
		case USEASM_OP_PCKF16F16:
		case USEASM_OP_PCKF16U16:
		case USEASM_OP_PCKF16S16:
		case USEASM_OP_PCKU16F32:
		case USEASM_OP_PCKU16F16:
		case USEASM_OP_PCKU16U16:
		case USEASM_OP_PCKU16S16:
		case USEASM_OP_PCKS16F32:
		case USEASM_OP_PCKS16F16:
		case USEASM_OP_PCKS16U16:
		case USEASM_OP_PCKS16S16:
		case USEASM_OP_PCKU8F32:
		case USEASM_OP_PCKU8F16:
		case USEASM_OP_PCKU8U16:
		case USEASM_OP_PCKU8S16:
		case USEASM_OP_PCKS8F32:
		case USEASM_OP_PCKS8F16:
		case USEASM_OP_PCKS8U16:
		case USEASM_OP_PCKS8S16:
		case USEASM_OP_PCKO8F32:
		case USEASM_OP_PCKO8F16:
		case USEASM_OP_PCKO8U16:
		case USEASM_OP_PCKO8S16:
		case USEASM_OP_PCKC10F32:
		case USEASM_OP_PCKC10F16:
		case USEASM_OP_PCKC10U16:
		case USEASM_OP_PCKC10S16:
		case USEASM_OP_UNPCKF32F32:
		case USEASM_OP_UNPCKF32F16:
		case USEASM_OP_UNPCKF32U16:
		case USEASM_OP_UNPCKF32S16:
		case USEASM_OP_UNPCKF32U8:
		case USEASM_OP_UNPCKF32S8:
		case USEASM_OP_UNPCKF32O8:
		case USEASM_OP_UNPCKF32C10:
		case USEASM_OP_UNPCKF16F16:
		case USEASM_OP_UNPCKF16U16:
		case USEASM_OP_UNPCKF16S16:
		case USEASM_OP_UNPCKF16U8:
		case USEASM_OP_UNPCKF16S8:
		case USEASM_OP_UNPCKF16O8:
		case USEASM_OP_UNPCKF16C10:
		case USEASM_OP_UNPCKU16F16:
		case USEASM_OP_UNPCKU16U16:
		case USEASM_OP_UNPCKU16S16:
		case USEASM_OP_UNPCKU16U8:
		case USEASM_OP_UNPCKU16S8:
		case USEASM_OP_UNPCKU16O8:
		case USEASM_OP_UNPCKU16C10:
		case USEASM_OP_UNPCKS16F16:
		case USEASM_OP_UNPCKS16U16:
		case USEASM_OP_UNPCKS16S16:
		case USEASM_OP_UNPCKS16U8:
		case USEASM_OP_UNPCKS16S8:
		case USEASM_OP_UNPCKS16O8:
		case USEASM_OP_UNPCKS16C10:
		case USEASM_OP_UNPCKU8U8:
		case USEASM_OP_UNPCKC10C10:
		{
			if (uArgCount == 2)
			{
				psInst->asArg[2] = psInst->asArg[1];
				uArgCount++;
			}
			if (uArgCount == 3)
			{
				psInst->asArg[3].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[3].uNumber = USEASM_INTSRCSEL_ROUNDNEAREST;
				psInst->asArg[3].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[3].uFlags = 0;
				uArgCount++;
			}
			break;
		}
		case USEASM_OP_LOCK:
		case USEASM_OP_RELEASE:
		{
			if (uArgCount == 0)
			{
				psInst->asArg[0].uType = USEASM_REGTYPE_IMMEDIATE;
				psInst->asArg[0].uNumber = 0;
				psInst->asArg[0].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[0].uFlags = 0;
				uArgCount++;
			}
			break;
		}
		case USEASM_OP_PHAS:
		{
			if (uArgCount == 5)
			{
				psInst->uOpcode = USEASM_OP_PHASIMM;
			}
			break;
		}
		case USEASM_OP_VMAD3:
		case USEASM_OP_VMAD4:
		{
			if (uArgCount == 7)
			{
				psInst->asArg[7].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[7].uNumber = USEASM_INTSRCSEL_INCREMENTBOTH;
				psInst->asArg[7].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[7].uFlags = 0;
				uArgCount++;
			}
			break;
		}
		case USEASM_OP_VDP3:
		case USEASM_OP_VDP4:
		{
			InsertTestDummyDestination(psInst, &uArgCount);
		
			if (psInst->uFlags1 & USEASM_OPFLAGS1_TESTENABLE)
			{
				if (uArgCount == 5)
				{
					psInst->asArg[6].uType = USEASM_REGTYPE_INTSRCSEL;
					psInst->asArg[6].uNumber = USEASM_INTSRCSEL_NONE;
					psInst->asArg[6].uIndex = USEREG_INDEX_NONE;
					psInst->asArg[6].uFlags = 0;
					uArgCount++;
			
					psInst->asArg[7].uType = USEASM_REGTYPE_INTSRCSEL;
					psInst->asArg[7].uNumber = USEASM_INTSRCSEL_INCREMENTBOTH;
					psInst->asArg[7].uIndex = USEREG_INDEX_NONE;
					psInst->asArg[7].uFlags = 0;
					uArgCount++;
				}
			}
			else
			{
				if (uArgCount == 5)
				{
					psInst->asArg[5].uType = USEASM_REGTYPE_INTSRCSEL;
					psInst->asArg[5].uNumber = USEASM_INTSRCSEL_NONE;
					psInst->asArg[5].uIndex = USEREG_INDEX_NONE;
					psInst->asArg[5].uFlags = 0;
					uArgCount++;
			
					psInst->asArg[6].uType = USEASM_REGTYPE_INTSRCSEL;
					psInst->asArg[6].uNumber = USEASM_INTSRCSEL_INCREMENTBOTH;
					psInst->asArg[6].uIndex = USEREG_INDEX_NONE;
					psInst->asArg[6].uFlags = 0;
					uArgCount++;
				}
				else if (uArgCount == 6)
				{
					if (
							psInst->asArg[5].uType == USEASM_REGTYPE_INTSRCSEL &&
							(
								psInst->asArg[5].uNumber == USEASM_INTSRCSEL_INCREMENTBOTH ||
								psInst->asArg[5].uNumber == USEASM_INTSRCSEL_INCREMENTGPI ||
								psInst->asArg[5].uNumber == USEASM_INTSRCSEL_INCREMENTUS ||
								psInst->asArg[5].uNumber == USEASM_INTSRCSEL_INCREMENTMOE
							)
						)
					{
						psInst->asArg[6] = psInst->asArg[5];
						
						psInst->asArg[5].uType = USEASM_REGTYPE_INTSRCSEL;
						psInst->asArg[5].uNumber = USEASM_INTSRCSEL_NONE;
						psInst->asArg[5].uIndex = USEREG_INDEX_NONE;
						psInst->asArg[5].uFlags = 0;
						uArgCount++;
					}
					else if (psInst->asArg[5].uType == USEASM_REGTYPE_CLIPPLANE)
					{
						psInst->asArg[6].uType = USEASM_REGTYPE_INTSRCSEL;
						psInst->asArg[6].uNumber = USEASM_INTSRCSEL_INCREMENTBOTH;
						psInst->asArg[6].uIndex = USEREG_INDEX_NONE;
						psInst->asArg[6].uFlags = 0;
						uArgCount++;
					}
				}
			}
			break;
		}
		case USEASM_OP_VMOVC:
		case USEASM_OP_VMOVCU8:
		{
			break;
		}
		case USEASM_OP_SMP1D:
		case USEASM_OP_SMP2D:
		case USEASM_OP_SMP3D:
		case USEASM_OP_SMP1DBIAS:
		case USEASM_OP_SMP2DBIAS:
		case USEASM_OP_SMP3DBIAS:
		case USEASM_OP_SMP1DREPLACE:
		case USEASM_OP_SMP2DREPLACE:
		case USEASM_OP_SMP3DREPLACE:
		case USEASM_OP_SMP1DGRAD:
		case USEASM_OP_SMP2DGRAD:
		case USEASM_OP_SMP3DGRAD:
		{
			IMG_UINT32	uExpectedArgCount;
		
			uExpectedArgCount = OpcodeArgumentCount(psInst->uOpcode);
			if (uArgCount == (uExpectedArgCount - 1))
			{
				psInst->asArg[uArgCount].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[uArgCount].uNumber = USEASM_INTSRCSEL_NONE;
				psInst->asArg[uArgCount].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[uArgCount].uFlags = 0;
				uArgCount++;
			}
			break;
		}
		case USEASM_OP_MOVMSK:
		{
			if ((psInst->uFlags3 & USEASM_OPFLAGS3_SAMPLEMASK) == 0)
			{
				memmove(&psInst->asArg[2], &psInst->asArg[1], (uArgCount - 1) * sizeof(psInst->asArg[0]));
				psInst->asArg[1].uType = USEASM_REGTYPE_INTSRCSEL;
				psInst->asArg[1].uNumber = USEASM_INTSRCSEL_NONE;
				psInst->asArg[1].uIndex = USEREG_INDEX_NONE;
				psInst->asArg[1].uFlags = 0;
				uArgCount++;
			}
			break;
		}
		default:
		{
			InsertTestDummyDestination(psInst, &uArgCount);
			break;
		}
	}
	if (OpcodeArgumentCount(psInst->uOpcode) != uArgCount)
	{
		const IMG_PCHAR pszQuant = ((uArgCount < OpcodeArgumentCount(psInst->uOpcode)) ?
									"few" : "many");
		AssemblerError(IMG_NULL, psInst,
					   "Too %s arguments (was %d; should be %d) for instruction",
					   pszQuant,
					   uArgCount,
					   OpcodeArgumentCount(psInst->uOpcode));
	}
	if (OpcodeArgumentCount(psInst->uOpcode)> uArgCount)
	{
		for (; uArgCount < OpcodeArgumentCount(psInst->uOpcode); uArgCount++)
		{
			InitialiseDefaultRegister(&psInst->asArg[uArgCount]);
		}
	}
}

static IMG_VOID AttachInst(PUSE_INST* psTail, PUSE_INST psInst)
{
	if ((*psTail) == NULL)
	{
		g_psInstListHead = psInst;
		(*psTail) = psInst;
		psInst->psPrev = psInst->psNext = NULL;
	}
	else
	{
		(*psTail)->psNext = psInst;
		psInst->psPrev = *psTail;
	}
}

#if defined(SUPPORT_SGX520) || defined(SUPPORT_SGX530) || defined(SUPPORT_SGX531) || defined(SUPPORT_SGX535) || defined(SUPPORT_SGX540) || defined(SUPPORT_SGX541) || defined(SUPPORT_SGX545)
static IMG_BOOL CheckForNewEfoOptions(IMG_VOID)
{
	if (g_sTargetCoreInfo.eID == SGX_CORE_ID_INVALID)
	{
		g_bUsedOldEfoOptions = IMG_TRUE;
		return IMG_FALSE;
	}
	return HasNewEfoOptions(g_psTargetCoreDesc);
}
#endif /* defined(SUPPORT_SGX520) || defined(SUPPORT_SGX530) || defined(SUPPORT_SGX531) || defined(SUPPORT_SGX535) || defined(SUPPORT_SGX540) || defined(SUPPORT_SGX541) || defined(SUPPORT_SGX545) */

static IMG_UINT32 ParseEfoMulExpr(IMG_UINT32 uM0S0, IMG_UINT32 uM0S1, IMG_UINT32 uM1S0, IMG_UINT32 uM1S1)
{
	PVR_UNREFERENCED_PARAMETER(uM0S0);
	PVR_UNREFERENCED_PARAMETER(uM0S1);
	PVR_UNREFERENCED_PARAMETER(uM1S0);
	PVR_UNREFERENCED_PARAMETER(uM1S1);

#if defined(SUPPORT_SGX520) || defined(SUPPORT_SGX530) || defined(SUPPORT_SGX531) || defined(SUPPORT_SGX535) || defined(SUPPORT_SGX540) || defined(SUPPORT_SGX541)
	if (uM0S0 == SRC0 && uM0S1 == SRC1 && uM1S0 == SRC0 && uM1S1 == SRC2)
	{
		if (CheckForNewEfoOptions())
		{
			ParseError("These efo multiplier options aren't available on this processor");
		}
		return EURASIA_USE1_EFO_MSRC_SRC0SRC1_SRC0SRC2 << EURASIA_USE1_EFO_MSRC_SHIFT;
	}
	if (uM0S0 == SRC0 && uM0S1 == SRC1 && uM1S0 == SRC0 && uM1S1 == SRC0)
	{
		if (CheckForNewEfoOptions())
		{
			ParseError("These efo multiplier options aren't available on this processor");
		}
		return EURASIA_USE1_EFO_MSRC_SRC0SRC1_SRC0SRC0 << EURASIA_USE1_EFO_MSRC_SHIFT;
	}
	if (uM0S0 == SRC1 && uM0S1 == SRC2 && uM1S0 == SRC0 && uM1S1 == SRC0)
	{
		if (CheckForNewEfoOptions())
		{
			ParseError("These efo multiplier options aren't available on this processor");
		}
		return EURASIA_USE1_EFO_MSRC_SRC1SRC2_SRC0SRC0 << EURASIA_USE1_EFO_MSRC_SHIFT;
	}
	if (uM0S0 == SRC1 && uM0S1 == I0 && uM1S0 == SRC0 && uM1S1 == I1)
	{
		if (CheckForNewEfoOptions())
		{
			ParseError("These efo multiplier options aren't available on this processor");
		}
		return EURASIA_USE1_EFO_MSRC_SRC1I0_SRC0I1 << EURASIA_USE1_EFO_MSRC_SHIFT;
	}
#endif /* defined(SUPPORT_SGX520) || defined(SUPPORT_SGX530) || defined(SUPPORT_SGX531) || defined(SUPPORT_SGX535) || defined(SUPPORT_SGX540) || defined(SUPPORT_SGX541) */

#if defined(SUPPORT_SGX545)
	if (uM0S0 == SRC0 && uM0S1 == SRC2 && uM1S0 == SRC1 && uM1S1 == SRC2)
	{
		if (!CheckForNewEfoOptions())
		{
			ParseError("These efo multiplier options aren't available on this processor");
		}
		return SGX545_USE1_EFO_MSRC_SRC0SRC2_SRC1SRC2 << EURASIA_USE1_EFO_MSRC_SHIFT;
	}
	if (uM0S0 == SRC2 && uM0S1 == SRC1 && uM1S0 == SRC2 && uM1S1 == SRC2)
	{
		if (!CheckForNewEfoOptions())
		{
			ParseError("These efo multiplier options aren't available on this processor");
		}
		return SGX545_USE1_EFO_MSRC_SRC2SRC1_SRC2SRC2 << EURASIA_USE1_EFO_MSRC_SHIFT;
	}
	if (uM0S0 == SRC1 && uM0S1 == SRC0 && uM1S0 == SRC2 && uM1S1 == SRC2)
	{
		if (!CheckForNewEfoOptions())
		{
			ParseError("These efo multiplier options aren't available on this processor");
		}
		return SGX545_USE1_EFO_MSRC_SRC1SRC0_SRC2SRC2 << EURASIA_USE1_EFO_MSRC_SHIFT;
	}
	if (uM0S0 == SRC1 && uM0S1 == I0 && uM1S0 == SRC2 && uM1S1 == I1)
	{
		if (!CheckForNewEfoOptions())
		{
			ParseError("These efo multiplier options aren't available on this processor");
		}
		return SGX545_USE1_EFO_MSRC_SRC1I0_SRC2I1 << EURASIA_USE1_EFO_MSRC_SHIFT;
	}
#endif /* defined(SUPPORT_SGX545) */

	ParseError("Unknown efo multiplier options");
	return 0;
}

static IMG_UINT32 ParseEfoAddExpr(IMG_UINT32 uA0S0,
								  IMG_UINT32 uNegateA0R,
								  IMG_UINT32 uA0S1,
								  IMG_UINT32 uNegateA1L,
								  IMG_UINT32 uA1S0,
								  IMG_UINT32 uA1S1)
{
	IMG_UINT32 uLeftNegate = 0, uRightNegate = 0;

	if (uNegateA1L)
	{
		uLeftNegate = EURASIA_USE1_EFO_A1LNEG;
	}
	if (uNegateA0R)
#if defined(SUPPORT_SGX545)
	{
		uRightNegate = SGX545_USE1_EFO_A0RNEG;
	}
#else /* #if defined(SUPPORT_SGX545) */
	{
		ParseError("Negating the right-hand input to the A1 adder isn't supported on this processor");
	}
#endif /* defined(SUPPORT_SGX545) */

	if (uA0S0 == M0 && uA0S1 == M1 && uA1S0 == I1 && uA1S1 == I0)
	{
		return (EURASIA_USE1_EFO_ASRC_M0M1_I1I0 << EURASIA_USE1_EFO_ASRC_SHIFT) | uLeftNegate | uRightNegate;
	}
	if (uA0S0 == M0 && uA0S1 == I0 && uA1S0 == I1 && uA1S1 == M1)
	{
		return (EURASIA_USE1_EFO_ASRC_M0I0_I1M1 << EURASIA_USE1_EFO_ASRC_SHIFT) | uLeftNegate | uRightNegate;
	}

#if defined(SUPPORT_SGX520) || defined(SUPPORT_SGX530) || defined(SUPPORT_SGX531) || defined(SUPPORT_SGX535) || defined(SUPPORT_SGX540) || defined(SUPPORT_SGX541)
	if (uA0S0 == M0 && uA0S1 == SRC2 && uA1S0 == I1 && uA1S1 == I0)
	{
		if (CheckForNewEfoOptions())
		{
			ParseError("These efo adder options aren't available on this processor");
		}
		return (EURASIA_USE1_EFO_ASRC_M0SRC2_I1I0 << EURASIA_USE1_EFO_ASRC_SHIFT) | uLeftNegate | uRightNegate;
	}
	if (uA0S0 == SRC0 && uA0S1 == SRC1 && uA1S0 == SRC2 && uA1S1 == SRC0)
	{
		if (CheckForNewEfoOptions())
		{
			ParseError("These efo adder options aren't available on this processor");
		}
		return (EURASIA_USE1_EFO_ASRC_SRC0SRC1_SRC2SRC0 << EURASIA_USE1_EFO_ASRC_SHIFT) | uLeftNegate | uRightNegate;
	}
#endif /* defined(SUPPORT_SGX520) || defined(SUPPORT_SGX530) || defined(SUPPORT_SGX531) || defined(SUPPORT_SGX535) || defined(SUPPORT_SGX540) || defined(SUPPORT_SGX541) */

#if defined(SUPPORT_SGX545)
	if (uA0S0 == M0 && uA0S1 == SRC0 && uA1S0 == I1 && uA1S1 == I0)
	{
		if (!CheckForNewEfoOptions())
		{
			ParseError("These efo adder options aren't available on this processor");
		}
		return (SGX545_USE1_EFO_ASRC_M0SRC0_I1I0 << EURASIA_USE1_EFO_ASRC_SHIFT) | uLeftNegate | uRightNegate;
	}
	if (uA0S0 == SRC0 && uA0S1 == SRC2 && uA1S0 == SRC1 && uA1S1 == SRC2)
	{
		if (!CheckForNewEfoOptions())
		{
			ParseError("These efo adder options aren't available on this processor");
		}
		return (SGX545_USE1_EFO_ASRC_SRC0SRC2_SRC1SRC2 << EURASIA_USE1_EFO_ASRC_SHIFT) | uLeftNegate | uRightNegate;
	}
#endif /* defined(SUPPORT_SGX545) */

	ParseError("Unknown efo adder options");
	return 0;
}

static const USE_INST* GetLastInstruction(IMG_VOID)
/*****************************************************************************
     FUNCTION	: GetLastInstruction

     PURPOSE	: Gives last instruction in the instructions list

     PARAMETERS	: None.

     RETURNS	: Constatn pointer to last instruction.
*****************************************************************************/
{
	if(g_psInstListHead == NULL)
	{
		return NULL;
	}
	else
	{
		PUSE_INST psInstsItrator = g_psInstListHead;
		for(; psInstsItrator->psNext!=NULL;
			psInstsItrator = psInstsItrator->psNext);
		return psInstsItrator;
	}
	return NULL;
}

static IMG_VOID SetTarget(SGX_CORE_ID_TYPE eNewType,
						  IMG_UINT32 uRev)

{
	SGX_CORE_INFO	sNewTarget;
	PCSGX_CORE_DESC	psNewTargetDesc;

	if (g_sTargetCoreInfo.eID != SGX_CORE_ID_INVALID &&
		g_sTargetCoreInfo.eID != eNewType)
	{
		yyerror("Multiple, different target specifications are invalid");
		return;
	}
	sNewTarget.eID = eNewType;
	if (!CheckCore(sNewTarget.eID))
	{
		yyerror("This target processor isn't supported by this build of useasm");
		return;
	}
	sNewTarget.uiRev = uRev;
	psNewTargetDesc = UseAsmGetCoreDesc(&sNewTarget);
	if (psNewTargetDesc == NULL)
	{
		yyerror("Invalid Core revision specified");
		return;
	}
	if (g_bUsedOldEfoOptions && HasNewEfoOptions(psNewTargetDesc))
	{
		yyerror("This target processor only supports the new EFO options but EFO instructions using the old options have "
				"already been assembled.");
		return;
	}
	g_sTargetCoreInfo = sNewTarget;
	g_psTargetCoreDesc = psNewTargetDesc;
}

static IMG_UINT32 gcd(IMG_UINT32 a, IMG_UINT32 b)
{
	while (b != 0)
	{
		IMG_UINT32 t;

		t = b;
		b = a % b;
		a = t;
	}
	return a;
}

static PUSE_INST ForceAlign(IMG_UINT32 uAlign1,
							IMG_UINT32 uAlign2,
							IMG_PCHAR pszSourceFile,
							IMG_UINT32 uSourceLine,
							PUSE_INST* ppsTail)
{
	PUSE_INST psTemp = NULL;
	PUSE_INST psLast;

	/*
	  Align the start of the module to at least the
	  alignment at this point.
	*/
	g_uModuleAlignment = (uAlign1 * g_uModuleAlignment) / gcd(uAlign1, g_uModuleAlignment);

	/*
	  Insert NOP instructions until we reach the
	  the required alignment.
	*/
	while ((g_uInstOffset % uAlign1) != uAlign2)
	{
		psTemp = UseAsm_Malloc(sizeof(*psTemp));
		psTemp->psNext = NULL;
		psTemp->uOpcode = USEASM_OP_PADDING;
		psTemp->uSourceLine = uSourceLine;
		psTemp->pszSourceFile = pszSourceFile;
		psTemp->uFlags1 = 0;
		psTemp->uFlags2 = 0;
		psTemp->uFlags3 = 0;
		psTemp->uTest = 0;
		CheckOpcode(psTemp, 0, TRUE);
		AttachInst(ppsTail, psTemp);
		psLast = psTemp;
		ppsTail = &psLast;

		if (g_uSchedOffCount> 0)
		{
			if (g_bStartedNoSchedRegion)
			{
				g_psFirstNoSchedInst = psTemp;
				g_bStartedNoSchedRegion = FALSE;
			}
			g_psLastNoSchedInst = psTemp;
		}

		g_uInstCount++;
	}
	if (psTemp != NULL)
	{
		return psTemp;
	}
	else
	{
		return *ppsTail;
	}
}

/*****************************************************************************
     FUNCTION   : OptProcInputRegs

     PURPOSE    : Process an input register list for the optimiser data.

     PARAMETERS : psRegList    - List of registers to process

     RETURNS    : Nothing
*****************************************************************************/
static
IMG_VOID OptProcInputRegs(USEASM_PARSER_REG_LIST *psRegList)
{
	USEASM_PARSER_REG_LIST *psCurr, *psNext;

	psNext = NULL;
	for (psCurr = psRegList; psCurr != NULL; psCurr = psNext)
	{
		IMG_UINT32 uTmp = 0;
		psNext = psCurr->psNext;

		if (psCurr->uType == USEASM_REGTYPE_TEMP)
		{
			uTmp = g_sUseasmParserOptState.uMaxTemp;
			if (psCurr->uNumber> uTmp)
                g_sUseasmParserOptState.uMaxTemp = psCurr->uNumber;
		}
		else if (psCurr->uType == USEASM_REGTYPE_PRIMATTR)
		{
			uTmp = g_sUseasmParserOptState.uMaxPrimAttr;
			if (psCurr->uNumber> uTmp)
                g_sUseasmParserOptState.uMaxPrimAttr = psCurr->uNumber;
		}
		else if (psCurr->uType == USEASM_REGTYPE_OUTPUT)
		{
			uTmp = g_sUseasmParserOptState.uMaxOutput;
			if (psCurr->uNumber> uTmp)
                g_sUseasmParserOptState.uMaxOutput = psCurr->uNumber;
		}

		UseAsm_Free(psCurr);
	}
}

/*****************************************************************************
     FUNCTION   : OptProcOutputRegs

     PURPOSE    : Process an output register list for the optimiser data.

     PARAMETERS : psRegList    - List of registers to process

     RETURNS    : Nothing
*****************************************************************************/
static
IMG_VOID OptProcOutputRegs(USEASM_PARSER_REG_LIST *psRegList)
{
	USEASM_PARSER_REG_LIST *psCurr;

	if (psRegList == NULL)
	{
		return;
	}

	for(psCurr = psRegList; psCurr->psNext != NULL; psCurr = psCurr->psNext)
	{
		/* skip */
	}

	psCurr->psNext = g_sUseasmParserOptState.psOutputRegs;
	g_sUseasmParserOptState.psOutputRegs = psRegList;
}

static
IMG_VOID ProcessLabel(PUSE_INST psInst, OPCODE_AND_LINE* psLabel)
{
	IMG_UINT32 uN = OpenScope(psLabel->pszCharString, IMG_FALSE, IMG_FALSE, psLabel->pszSourceFile, psLabel->uSourceLine);
    UseAsm_Free(psLabel->pszCharString);
    psLabel->pszCharString = NULL;
    psInst->uOpcode = USEASM_OP_LABEL;
    psInst->uSourceLine = psLabel->uSourceLine;
    psInst->pszSourceFile = psLabel->pszSourceFile;
    psInst->asArg[0].uType = USEASM_REGTYPE_LABEL;
    psInst->asArg[0].uNumber = uN;
    psInst->asArg[0].uIndex = USEREG_INDEX_NONE;
    psInst->asArg[0].uFlags = 0;
    CheckOpcode(psInst, 1, FALSE);
    CloseScope();
}

static
PUSE_INST ProcessInstruction(PUSE_INST psOldProgramTail, PUSE_INST psNewInst)
{
	PUSE_INST psNewProgramTail;
	
	psNewProgramTail = UseAsm_Malloc(sizeof(USE_INST));
    *psNewProgramTail = *psNewInst;
    psNewProgramTail->psNext = NULL;
    
    if (psOldProgramTail == NULL)
    {
        g_psInstListHead = psNewProgramTail;
        if (psNewProgramTail->uFlags2 & USEASM_OPFLAGS2_COISSUE)
        {
            AssemblerError(IMG_NULL, psNewProgramTail, "Coissue instruction can't be the first instruction");
        }
        psNewProgramTail->psPrev = psNewProgramTail->psNext = NULL;
    }
    else
    {
        if (psNewProgramTail->uFlags2 & USEASM_OPFLAGS2_COISSUE)
        {
            psOldProgramTail->uFlags1 |= USEASM_OPFLAGS1_MAINISSUE;
        }
        psOldProgramTail->psNext = psNewProgramTail;
        psNewProgramTail->psPrev = psOldProgramTail;
    }
    if (g_uSchedOffCount> 0)
    {
        if (g_bStartedNoSchedRegion)
        {
            g_psFirstNoSchedInst = psNewProgramTail;
            g_bStartedNoSchedRegion = FALSE;
        }
        g_psLastNoSchedInst = psNewProgramTail;
    }
    g_uInstCount++;
    if ((g_uInstOffset % 2) == 0 && psOldProgramTail != NULL)
    {
        CheckPairing(psNewProgramTail);
    }
    
    return psNewProgramTail;
}

USEASM_PARSER_OPT_STATE g_sUseasmParserOptState;
PUSE_INST g_psInstListHead;
IMG_UINT32 g_uInstCount;

IMG_UINT32 g_uParserError;


/* Line 189 of yacc.c  */
#line 2155 "eurasiacon/binary2_540_omap4430_android_release/host/intermediates/useparser/use.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     REGISTER = 258,
     OPCODE_FLAG1 = 259,
     OPCODE_FLAG2 = 260,
     OPCODE_FLAG3 = 261,
     ARGUMENT_FLAG = 262,
     MASK = 263,
     NUMBER = 264,
     TEST_TYPE = 265,
     TEST_CHANSEL = 266,
     TEMP_REGISTER = 267,
     REPEAT_FLAG = 268,
     TEST_MASK = 269,
     C10_FLAG = 270,
     F16_FLAG = 271,
     U8_FLAG = 272,
     FLOAT_NUMBER = 273,
     OPCODE = 274,
     COISSUE_OPCODE = 275,
     LD_OPCODE = 276,
     ST_OPCODE = 277,
     ELD_OPCODE = 278,
     BRANCH_OPCODE = 279,
     EFO_OPCODE = 280,
     LAPC_OPCODE = 281,
     WDF_OPCODE = 282,
     EXT_OPCODE = 283,
     LOCKRELEASE_OPCODE = 284,
     IDF_OPCODE = 285,
     FIR_OPCODE = 286,
     ASOP2_OPCODE = 287,
     ONEARG_OPCODE = 288,
     PTOFF_OPCODE = 289,
     CALL_OPCODE = 290,
     NOP_OPCODE = 291,
     EMITVTX_OPCODE = 292,
     CFI_OPCODE = 293,
     INPUT = 294,
     OUTPUT = 295,
     OPEN_SQBRACKET = 296,
     CLOSE_SQBRACKET = 297,
     PLUS = 298,
     MINUS = 299,
     TIMES = 300,
     DIVIDE = 301,
     LSHIFT = 302,
     RSHIFT = 303,
     HASH = 304,
     BANG = 305,
     AND = 306,
     OR = 307,
     PLUSPLUS = 308,
     MINUSMINUS = 309,
     EQUALS = 310,
     NOT = 311,
     XOR = 312,
     OPEN_BRACKET = 313,
     CLOSE_BRACKET = 314,
     INSTRUCTION_DELIMITER = 315,
     COMMA = 316,
     PRED0 = 317,
     PRED1 = 318,
     PRED2 = 319,
     PRED3 = 320,
     PREDN = 321,
     SCHEDON = 322,
     SCHEDOFF = 323,
     SKIPINVON = 324,
     SKIPINVOFF = 325,
     REPEATOFF = 326,
     FORCE_ALIGN = 327,
     MISALIGN = 328,
     IMPORT = 329,
     EXPORT = 330,
     MODULEALIGN = 331,
     I0 = 332,
     I1 = 333,
     A0 = 334,
     A1 = 335,
     M0 = 336,
     M1 = 337,
     SRC0 = 338,
     SRC1 = 339,
     SRC2 = 340,
     DIRECT_IMMEDIATE = 341,
     ADDRESS_MODE = 342,
     SWIZZLE = 343,
     DATAFORMAT = 344,
     INTSRCSEL = 345,
     ABS = 346,
     ABS_NODOT = 347,
     UNEXPECTED_CHARACTER = 348,
     INDEXLOW = 349,
     INDEXHIGH = 350,
     INDEXBOTH = 351,
     PCLINK = 352,
     LABEL_ADDRESS = 353,
     IDF_ST = 354,
     IDF_PIXELBE = 355,
     NOSCHED_FLAG = 356,
     SKIPINV_FLAG = 357,
     TARGET_SPECIFIER = 358,
     TARGET = 359,
     TEMP_REG_DEF = 360,
     OPEN_CURLY_BRACKET = 361,
     CLOSE_CURLY_BRACKET = 362,
     PROC = 363,
     SCOPE_NAME = 364,
     IDENTIFIER = 365,
     NAMED_REGS_RANGE = 366,
     RENAME_REG = 367,
     COLON = 368,
     COLON_PLUS_DELIMITER = 369,
     MODULUS = 370
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 2083 "tools/intern/useasm/use.y"

    IMG_UINT32 n;
    float f;
    USE_REGISTER sRegister;
    SOURCE_ARGUMENTS sArgs;
    USE_INST sInst;
    PUSE_INST psInst;
    OPCODE_AND_LINE sOp;
    LDST_ARGUMENT sLdStArg;
    INSTRUCTION_MODIFIER sMod;
    USEASM_PARSER_REG_LIST* psRegList;



/* Line 214 of yacc.c  */
#line 2321 "eurasiacon/binary2_540_omap4430_android_release/host/intermediates/useparser/use.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 2333 "eurasiacon/binary2_540_omap4430_android_release/host/intermediates/useparser/use.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   825

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  116
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  41
/* YYNRULES -- Number of rules.  */
#define YYNRULES  224
/* YYNRULES -- Number of states.  */
#define YYNSTATES  510

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   370

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     8,    12,    16,    20,    23,    27,
      31,    34,    37,    38,    42,    44,    46,    48,    50,    52,
      54,    57,    62,    65,    68,    71,    74,    79,    84,    87,
      92,    94,   102,   110,   119,   126,   137,   142,   147,   158,
     161,   162,   169,   173,   177,   181,   186,   189,   194,   203,
     208,   215,   228,   239,   244,   249,   255,   263,   269,   274,
     276,   280,   284,   286,   287,   289,   290,   293,   296,   299,
     302,   306,   312,   314,   316,   319,   322,   325,   327,   329,
     331,   334,   337,   340,   342,   344,   345,   348,   351,   354,
     357,   360,   363,   366,   369,   372,   375,   378,   380,   384,
     388,   394,   396,   397,   399,   401,   403,   405,   408,   410,
     412,   415,   417,   420,   423,   427,   433,   438,   446,   453,
     460,   468,   476,   482,   487,   493,   502,   504,   506,   508,
     510,   512,   514,   516,   518,   520,   522,   524,   526,   528,
     530,   532,   534,   540,   545,   548,   555,   557,   561,   565,
     569,   573,   577,   581,   585,   588,   592,   596,   600,   604,
     607,   609,   611,   613,   615,   617,   618,   621,   624,   627,
     630,   633,   637,   643,   651,   657,   665,   669,   675,   677,
     680,   683,   685,   687,   689,   703,   717,   731,   745,   754,
     763,   772,   781,   790,   799,   813,   815,   817,   819,   821,
     823,   825,   827,   839,   841,   843,   845,   847,   849,   851,
     853,   855,   857,   858,   860,   861,   863,   864,   866,   867,
     869,   871,   875,   877,   883
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     117,     0,    -1,    -1,   117,   121,    60,    -1,   117,   119,
      60,    -1,   117,    39,   118,    -1,   117,    40,   118,    -1,
     117,    60,    -1,   117,   120,    60,    -1,   117,   110,   114,
      -1,   117,   107,    -1,   117,   106,    -1,    -1,   118,   134,
       9,    -1,    68,    -1,    67,    -1,    69,    -1,    70,    -1,
      13,    -1,    71,    -1,   104,   103,    -1,   104,   103,    61,
       9,    -1,    75,   110,    -1,    74,   110,    -1,    76,     9,
      -1,   105,   155,    -1,   111,     9,    61,     9,    -1,   112,
     110,    61,   110,    -1,    72,     9,    -1,    72,     9,    61,
       9,    -1,    73,    -1,   126,    19,   130,   132,   125,    61,
     131,    -1,    43,    19,   130,   132,   125,    61,   131,    -1,
     126,    21,   130,   132,    61,   140,    61,   131,    -1,   126,
      22,   130,   142,    61,   131,    -1,   126,    23,   130,   132,
      61,   141,    61,   132,    61,   132,    -1,   126,    24,   130,
     123,    -1,   126,    35,   130,   124,    -1,   126,    25,   130,
     132,    55,   150,    61,   145,    61,   131,    -1,   110,   113,
      -1,    -1,   108,   110,   122,    58,   154,    59,    -1,   126,
      26,   130,    -1,   126,    36,   130,    -1,   126,    29,   130,
      -1,   126,    29,   130,   135,    -1,    34,   130,    -1,   126,
      27,   130,   135,    -1,   126,    38,   130,   135,    61,   135,
      61,   135,    -1,   126,    33,   130,   135,    -1,   126,    30,
     130,   135,    61,   129,    -1,   126,    28,   130,   132,    55,
     150,    61,   132,    61,   132,    61,   132,    -1,   126,    28,
     130,   132,    61,   132,    61,   132,    61,   132,    -1,    43,
      20,   130,   131,    -1,    43,    32,   130,   131,    -1,    43,
      32,   130,    61,   131,    -1,   126,    31,   130,   132,   125,
      61,   131,    -1,   126,    31,   130,    61,   131,    -1,   126,
      37,   130,   131,    -1,   110,    -1,   110,    43,   136,    -1,
     110,    44,   136,    -1,   110,    -1,    -1,     8,    -1,    -1,
     127,   126,    -1,   128,   126,    -1,   102,   126,    -1,   101,
     126,    -1,    58,   126,    59,    -1,    58,   126,    61,   126,
      59,    -1,    62,    -1,    63,    -1,    50,    62,    -1,    50,
      63,    -1,    50,    64,    -1,    64,    -1,    65,    -1,    66,
      -1,    50,    65,    -1,    50,    66,    -1,    12,     9,    -1,
      99,    -1,   100,    -1,    -1,     4,   130,    -1,    13,   130,
      -1,     5,   130,    -1,     6,   130,    -1,    15,   130,    -1,
      17,   130,    -1,    16,   130,    -1,    10,   130,    -1,    11,
     130,    -1,    91,   130,    -1,    14,   130,    -1,   132,    -1,
     132,    61,   131,    -1,   133,   135,   139,    -1,    92,    58,
     135,    59,   139,    -1,     1,    -1,    -1,    43,    -1,    44,
      -1,    56,    -1,    50,    -1,     9,    44,    -1,     3,    -1,
      12,    -1,   134,     9,    -1,   138,    -1,    49,   136,    -1,
      49,    18,    -1,    49,    44,    18,    -1,    49,    98,    58,
     110,    59,    -1,   134,    41,   137,    42,    -1,   134,    41,
     137,    43,    49,     9,    42,    -1,   134,    41,   137,    43,
     136,    42,    -1,   134,    41,   136,    43,   137,    42,    -1,
     134,     9,    41,   137,    43,   136,    42,    -1,   134,     9,
      41,   136,    43,   137,    42,    -1,   134,    41,    49,     9,
      42,    -1,   134,    41,   136,    42,    -1,   134,     9,    41,
     137,    42,    -1,   134,     9,    41,   137,    43,    49,     9,
      42,    -1,    62,    -1,    63,    -1,    64,    -1,    65,    -1,
      97,    -1,    77,    -1,    78,    -1,    86,    -1,    88,    -1,
      87,    -1,    90,    -1,    19,    -1,    83,    -1,    84,    -1,
      85,    -1,   110,    -1,   110,    41,    49,     9,    42,    -1,
     110,    41,   136,    42,    -1,   109,   110,    -1,   109,   110,
      41,    49,     9,    42,    -1,     9,    -1,   136,    43,   136,
      -1,   136,    44,   136,    -1,   136,    45,   136,    -1,   136,
      46,   136,    -1,   136,    47,   136,    -1,   136,    48,   136,
      -1,   136,   115,   136,    -1,    56,   136,    -1,   136,    51,
     136,    -1,   136,    52,   136,    -1,   136,    57,   136,    -1,
      58,   136,    59,    -1,    44,   136,    -1,    94,    -1,    95,
      -1,    94,    -1,    95,    -1,    96,    -1,    -1,     7,   139,
      -1,    15,   139,    -1,    16,   139,    -1,    17,   139,    -1,
      91,   139,    -1,    41,   135,    42,    -1,    41,   135,    61,
     143,    42,    -1,    41,   135,    61,   143,    61,   135,    42,
      -1,    41,   135,    61,   135,    42,    -1,    41,   135,    61,
     135,    61,   135,    42,    -1,    41,   135,    42,    -1,    41,
     135,    61,   143,    42,    -1,   135,    -1,   144,   135,    -1,
     135,   144,    -1,    53,    -1,    54,    -1,    44,    -1,   151,
      77,    55,    79,    61,   151,    78,    55,    80,    61,   146,
      61,   148,    -1,   151,    77,    55,    80,    61,   151,    78,
      55,    79,    61,   146,    61,   148,    -1,   151,    77,    55,
      81,    61,   151,    78,    55,    82,    61,   146,    61,   148,
      -1,   151,    77,    55,    79,    61,   151,    78,    55,    82,
      61,   146,    61,   148,    -1,   151,    78,    55,    80,    61,
     146,    61,   148,    -1,   151,    78,    55,    79,    61,   146,
      61,   148,    -1,   151,    78,    55,    82,    61,   146,    61,
     148,    -1,   151,    77,    55,    79,    61,   146,    61,   148,
      -1,   151,    77,    55,    80,    61,   146,    61,   148,    -1,
     151,    77,    55,    81,    61,   146,    61,   148,    -1,    79,
      55,   147,    43,   153,   147,    61,    80,    55,   152,   147,
      43,   147,    -1,    83,    -1,    84,    -1,    85,    -1,    77,
      -1,    78,    -1,    81,    -1,    82,    -1,    81,    55,   149,
      45,   149,    61,    82,    55,   149,    45,   149,    -1,    83,
      -1,    84,    -1,    85,    -1,    77,    -1,    78,    -1,    77,
      -1,    78,    -1,    79,    -1,    80,    -1,    -1,    50,    -1,
      -1,    44,    -1,    -1,    44,    -1,    -1,   155,    -1,   156,
      -1,   155,    61,   156,    -1,   110,    -1,   110,    41,    49,
       9,    42,    -1,   110,    41,   136,    42,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  2155,  2155,  2156,  2160,  2162,  2164,  2166,  2168,  2170,
    2175,  2183,  2221,  2222,  2240,  2250,  2268,  2270,  2280,  2291,
    2301,  2303,  2305,  2310,  2316,  2320,  2322,  2337,  2348,  2355,
    2360,  2368,  2389,  2409,  2428,  2446,  2460,  2471,  2482,  2496,
    2499,  2498,  2529,  2539,  2549,  2559,  2571,  2580,  2592,  2609,
    2621,  2637,  2653,  2681,  2692,  2703,  2718,  2736,  2753,  2769,
    2778,  2788,  2801,  2814,  2815,  2821,  2822,  2826,  2828,  2830,
    2832,  2835,  2842,  2844,  2846,  2848,  2850,  2852,  2854,  2856,
    2859,  2861,  2865,  2870,  2872,  2877,  2883,  2889,  2896,  2903,
    2910,  2917,  2924,  2931,  2938,  2945,  2952,  2962,  2964,  2971,
    2973,  2975,  2981,  2982,  2984,  2986,  2988,  2990,  3000,  3002,
    3007,  3013,  3019,  3025,  3029,  3033,  3041,  3045,  3049,  3053,
    3057,  3061,  3065,  3067,  3069,  3071,  3073,  3075,  3077,  3079,
    3081,  3083,  3085,  3087,  3089,  3091,  3093,  3095,  3125,  3131,
    3137,  3143,  3163,  3206,  3249,  3309,  3423,  3425,  3427,  3429,
    3431,  3433,  3435,  3437,  3439,  3441,  3443,  3445,  3447,  3449,
    3454,  3456,  3461,  3463,  3465,  3471,  3472,  3474,  3476,  3478,
    3480,  3486,  3493,  3502,  3516,  3528,  3541,  3548,  3559,  3563,
    3569,  3578,  3580,  3582,  3587,  3593,  3599,  3605,  3611,  3616,
    3621,  3626,  3631,  3636,  3644,  3652,  3654,  3656,  3658,  3660,
    3662,  3664,  3669,  3676,  3678,  3680,  3682,  3684,  3689,  3691,
    3693,  3695,  3701,  3702,  3708,  3709,  3715,  3716,  3720,  3722,
    3726,  3727,  3731,  3733,  3745
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "REGISTER", "OPCODE_FLAG1",
  "OPCODE_FLAG2", "OPCODE_FLAG3", "ARGUMENT_FLAG", "MASK", "NUMBER",
  "TEST_TYPE", "TEST_CHANSEL", "TEMP_REGISTER", "REPEAT_FLAG", "TEST_MASK",
  "C10_FLAG", "F16_FLAG", "U8_FLAG", "FLOAT_NUMBER", "OPCODE",
  "COISSUE_OPCODE", "LD_OPCODE", "ST_OPCODE", "ELD_OPCODE",
  "BRANCH_OPCODE", "EFO_OPCODE", "LAPC_OPCODE", "WDF_OPCODE", "EXT_OPCODE",
  "LOCKRELEASE_OPCODE", "IDF_OPCODE", "FIR_OPCODE", "ASOP2_OPCODE",
  "ONEARG_OPCODE", "PTOFF_OPCODE", "CALL_OPCODE", "NOP_OPCODE",
  "EMITVTX_OPCODE", "CFI_OPCODE", "INPUT", "OUTPUT", "OPEN_SQBRACKET",
  "CLOSE_SQBRACKET", "PLUS", "MINUS", "TIMES", "DIVIDE", "LSHIFT",
  "RSHIFT", "HASH", "BANG", "AND", "OR", "PLUSPLUS", "MINUSMINUS",
  "EQUALS", "NOT", "XOR", "OPEN_BRACKET", "CLOSE_BRACKET",
  "INSTRUCTION_DELIMITER", "COMMA", "PRED0", "PRED1", "PRED2", "PRED3",
  "PREDN", "SCHEDON", "SCHEDOFF", "SKIPINVON", "SKIPINVOFF", "REPEATOFF",
  "FORCE_ALIGN", "MISALIGN", "IMPORT", "EXPORT", "MODULEALIGN", "I0", "I1",
  "A0", "A1", "M0", "M1", "SRC0", "SRC1", "SRC2", "DIRECT_IMMEDIATE",
  "ADDRESS_MODE", "SWIZZLE", "DATAFORMAT", "INTSRCSEL", "ABS", "ABS_NODOT",
  "UNEXPECTED_CHARACTER", "INDEXLOW", "INDEXHIGH", "INDEXBOTH", "PCLINK",
  "LABEL_ADDRESS", "IDF_ST", "IDF_PIXELBE", "NOSCHED_FLAG", "SKIPINV_FLAG",
  "TARGET_SPECIFIER", "TARGET", "TEMP_REG_DEF", "OPEN_CURLY_BRACKET",
  "CLOSE_CURLY_BRACKET", "PROC", "SCOPE_NAME", "IDENTIFIER",
  "NAMED_REGS_RANGE", "RENAME_REG", "COLON", "COLON_PLUS_DELIMITER",
  "MODULUS", "$accept", "program", "opt_register_list",
  "pseudo_instruction", "forcealign", "instruction", "$@1", "label",
  "procedure", "maybe_mask", "preopcode_flag", "predicate", "repeat",
  "idf_path", "opcode_modifier", "source_arguments", "argument",
  "prearg_mod", "register_type", "arg_register", "expr",
  "src_index_register", "dest_index_register", "arg_modifier",
  "ld_argument", "eld_argument", "st_argument", "ldst_offset_argument",
  "offset_op", "efo_expression", "efo_addr_expr", "efo_add_src",
  "efo_mul_expr", "efo_mul_src", "efo_dest_select", "maybe_bang",
  "maybe_negate", "maybe_negate_r", "temp_reg_names_optional",
  "temp_reg_names", "temp_reg_name", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   116,   117,   117,   117,   117,   117,   117,   117,   117,
     117,   117,   118,   118,   119,   119,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   119,   119,   119,   120,   120,
     120,   121,   121,   121,   121,   121,   121,   121,   121,   121,
     122,   121,   121,   121,   121,   121,   121,   121,   121,   121,
     121,   121,   121,   121,   121,   121,   121,   121,   121,   123,
     123,   123,   124,   125,   125,   126,   126,   126,   126,   126,
     126,   126,   127,   127,   127,   127,   127,   127,   127,   127,
     127,   127,   128,   129,   129,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   131,   131,   132,
     132,   132,   133,   133,   133,   133,   133,   133,   134,   134,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   136,   136,   136,   136,
     136,   136,   136,   136,   136,   136,   136,   136,   136,   136,
     137,   137,   138,   138,   138,   139,   139,   139,   139,   139,
     139,   140,   140,   140,   141,   141,   142,   142,   143,   143,
     143,   144,   144,   144,   145,   145,   145,   145,   145,   145,
     145,   145,   145,   145,   146,   147,   147,   147,   147,   147,
     147,   147,   148,   149,   149,   149,   149,   149,   150,   150,
     150,   150,   151,   151,   152,   152,   153,   153,   154,   154,
     155,   155,   156,   156,   156
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     3,     3,     3,     3,     2,     3,     3,
       2,     2,     0,     3,     1,     1,     1,     1,     1,     1,
       2,     4,     2,     2,     2,     2,     4,     4,     2,     4,
       1,     7,     7,     8,     6,    10,     4,     4,    10,     2,
       0,     6,     3,     3,     3,     4,     2,     4,     8,     4,
       6,    12,    10,     4,     4,     5,     7,     5,     4,     1,
       3,     3,     1,     0,     1,     0,     2,     2,     2,     2,
       3,     5,     1,     1,     2,     2,     2,     1,     1,     1,
       2,     2,     2,     1,     1,     0,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     1,     3,     3,
       5,     1,     0,     1,     1,     1,     1,     2,     1,     1,
       2,     1,     2,     2,     3,     5,     4,     7,     6,     6,
       7,     7,     5,     4,     5,     8,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     5,     4,     2,     6,     1,     3,     3,     3,
       3,     3,     3,     3,     2,     3,     3,     3,     3,     2,
       1,     1,     1,     1,     1,     0,     2,     2,     2,     2,
       2,     3,     5,     7,     5,     7,     3,     5,     1,     2,
       2,     1,     1,     1,    13,    13,    13,    13,     8,     8,
       8,     8,     8,     8,    13,     1,     1,     1,     1,     1,
       1,     1,    11,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     0,     1,     0,     1,     0,     1,     0,     1,
       1,     3,     1,     5,     4
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,    65,     1,     0,    18,    85,    12,    12,     0,     0,
      65,     7,    72,    73,    77,    78,    79,    15,    14,    16,
      17,    19,     0,    30,     0,     0,     0,    65,    65,     0,
       0,    11,    10,     0,     0,     0,     0,     0,     0,     0,
       0,    65,    65,    82,    85,    85,    85,    85,    85,    85,
      85,    85,    85,    85,    85,    46,     5,     6,    85,    85,
      85,    74,    75,    76,    80,    81,     0,    28,    23,    22,
      24,    69,    68,    20,   222,    25,   220,    40,    39,     9,
       0,     0,     4,     8,     3,    85,    85,    85,    85,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    85,
      85,    85,    66,    67,    86,    88,    89,    93,    94,    87,
      96,    90,    92,    91,    95,   108,   109,     0,     0,     0,
       0,    70,    65,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    42,     0,     0,    44,
       0,     0,     0,     0,    43,     0,     0,    13,   101,     0,
     103,   104,   106,   105,     0,    63,     0,    53,    97,     0,
      54,     0,    29,    21,   146,     0,     0,     0,     0,     0,
     221,   218,    26,    27,    63,     0,     0,     0,     0,    59,
      36,     0,   137,     0,   126,   127,   128,   129,   131,   132,
     138,   139,   140,   133,   135,   134,   136,   162,   163,   164,
     130,     0,   141,     0,    47,   111,     0,    45,     0,     0,
      63,    49,    62,    37,    58,     0,   107,     0,    64,     0,
     165,     0,    55,    71,   159,     0,   154,     0,   224,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     219,     0,     0,     0,     0,     0,     0,     0,     0,   113,
       0,     0,   112,   144,     0,   110,     0,     0,     0,     0,
      57,     0,     0,     0,     0,   165,   165,   165,   165,   165,
      99,    98,   223,   158,   147,   148,   149,   150,   151,   152,
     155,   156,   157,   153,    41,     0,     0,     0,   176,     0,
      34,     0,     0,    60,    61,   208,   209,   210,   211,     0,
     114,     0,     0,     0,     0,     0,     0,   160,   161,     0,
       0,     0,     0,    83,    84,    50,     0,     0,   165,    32,
     166,   167,   168,   169,   170,    31,     0,     0,   183,   181,
     182,   178,     0,     0,     0,     0,   212,     0,     0,     0,
     143,     0,     0,     0,   123,     0,   116,     0,     0,     0,
      56,     0,   100,   171,     0,    33,   180,   177,   179,     0,
       0,   213,     0,     0,   115,     0,   142,     0,   124,     0,
     122,     0,     0,     0,     0,     0,    48,     0,     0,     0,
       0,     0,     0,   145,     0,     0,     0,   119,     0,   118,
       0,     0,   172,     0,   174,     0,    35,    38,     0,     0,
     121,     0,   120,   117,     0,    52,     0,     0,     0,     0,
       0,     0,     0,     0,   125,     0,   173,   175,   212,   212,
     212,     0,     0,     0,    51,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   198,   199,   200,   201,   195,
     196,   197,     0,     0,   191,     0,   192,     0,   193,     0,
     189,   188,   190,   216,     0,     0,     0,     0,     0,   217,
       0,   206,   207,   203,   204,   205,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   184,   187,   185,   186,   214,
       0,   215,     0,     0,     0,     0,     0,     0,   194,   202
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    56,    37,    38,    39,   127,   180,   213,   219,
      40,    41,    42,   315,    55,   157,   158,   156,   203,   331,
     274,   310,   205,   270,   287,   292,   177,   332,   333,   362,
     426,   452,   454,   476,   299,   363,   502,   470,   239,    75,
      76
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -434
static const yytype_int16 yypact[] =
{
    -434,   252,  -434,     2,  -434,    95,  -434,  -434,   196,   636,
      -7,  -434,  -434,  -434,  -434,  -434,  -434,  -434,  -434,  -434,
    -434,  -434,     5,  -434,   -92,   -84,    30,    -7,    -7,   -54,
     -49,  -434,  -434,   -38,   -17,    74,   -22,    33,    53,    69,
     787,    -7,    -7,  -434,    95,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,  -434,    73,    73,    95,    95,
      95,  -434,  -434,  -434,  -434,  -434,   160,    75,  -434,  -434,
    -434,  -434,  -434,    89,   137,   101,  -434,  -434,  -434,  -434,
     128,   168,  -434,  -434,  -434,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    95,  -434,  -434,  -434,  -434,  -434,  -434,  -434,  -434,
    -434,  -434,  -434,  -434,  -434,  -434,  -434,   189,   442,   442,
     328,  -434,    -7,   213,   231,   121,   -49,   186,   244,   149,
     442,   442,   221,   442,   169,   442,  -434,   545,   442,   545,
     545,   385,   545,   171,  -434,   442,   545,  -434,  -434,   240,
    -434,  -434,  -434,  -434,   238,   292,   545,  -434,   242,   442,
    -434,   239,  -434,  -434,  -434,   139,   295,   139,   139,   112,
    -434,   -49,  -434,  -434,   292,   246,   545,   247,   248,   123,
    -434,   250,  -434,    26,  -434,  -434,  -434,  -434,  -434,  -434,
    -434,  -434,  -434,  -434,  -434,  -434,  -434,  -434,  -434,  -434,
    -434,   201,   260,    36,  -434,  -434,   -30,  -434,   269,   442,
     292,  -434,  -434,  -434,  -434,   271,  -434,   545,  -434,   274,
      47,   442,  -434,  -434,   -24,   294,   198,   223,  -434,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   139,   291,
     101,   288,   310,    37,   442,   311,   139,   139,   210,  -434,
     202,   297,   616,   320,   290,   325,    31,   210,   442,    85,
    -434,   304,   545,   308,   442,    47,    47,    47,    47,    47,
    -434,  -434,  -434,  -434,   -44,   -24,   198,   -41,   -37,    99,
     661,   646,   522,  -434,  -434,   442,   545,   309,  -434,   496,
    -434,   545,   313,   616,   616,  -434,  -434,  -434,  -434,   314,
    -434,   259,   327,   370,   161,    93,   372,  -434,  -434,   191,
     183,   322,   326,  -434,  -434,  -434,   442,   334,    47,  -434,
    -434,  -434,  -434,  -434,  -434,  -434,    72,   442,  -434,  -434,
    -434,   138,   343,   545,   335,   442,   348,   344,   398,   366,
    -434,   631,   208,   367,  -434,    93,  -434,   324,   442,   442,
    -434,   545,  -434,  -434,   496,  -434,  -434,  -434,  -434,   545,
     349,  -434,   358,   200,  -434,   379,  -434,    93,  -434,   408,
    -434,   384,   418,   569,   369,   371,  -434,    92,   119,   442,
     442,   376,   378,  -434,   394,   430,   601,  -434,   400,  -434,
     442,   442,  -434,   545,  -434,   545,  -434,  -434,   176,   114,
    -434,   402,  -434,  -434,   392,  -434,   413,   414,   397,   399,
     404,   406,   415,   417,  -434,   442,  -434,  -434,   -12,   -12,
     -12,   361,   361,   361,  -434,   419,   422,   381,   423,   409,
     427,   411,   429,   432,   435,   518,   416,   445,   416,   446,
     416,   447,   416,   416,   416,  -434,  -434,  -434,  -434,  -434,
    -434,  -434,   460,   454,  -434,   165,  -434,   434,  -434,   439,
    -434,  -434,  -434,   468,   433,   453,   461,   462,   463,  -434,
     518,  -434,  -434,  -434,  -434,  -434,   486,   361,   361,   361,
     361,   472,   433,   474,   480,   481,   482,   464,   485,   416,
     416,   416,   416,   492,   471,  -434,  -434,  -434,  -434,   510,
     500,  -434,   518,   433,   513,   517,   518,   433,  -434,  -434
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -434,  -434,   556,  -434,  -434,  -434,  -434,  -434,  -434,  -150,
       6,  -434,  -434,  -434,   704,  -117,  -118,  -434,   237,  -110,
    -115,  -264,  -434,   -94,  -434,  -434,  -434,   217,   241,  -434,
     -78,  -433,   277,  -417,   318,  -218,  -434,  -434,  -434,   405,
     451
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -103
static const yytype_int16 yytable[] =
{
     155,   231,   232,   160,   231,     3,   229,   230,   231,   232,
     169,    43,   174,   175,    67,   178,    66,   181,    68,   229,
     206,   231,   232,   210,   241,   257,    69,   204,   214,   207,
     208,   258,   211,    71,    72,   164,   215,   481,   361,    70,
     164,   342,   222,     9,   249,   255,   220,   102,   103,    73,
     224,    10,   226,   227,   265,    12,    13,    14,    15,    16,
     261,    74,   266,   267,   268,   488,   243,   425,   252,   504,
     250,   238,    77,   508,   238,   165,   115,   256,   238,   288,
     306,   371,   167,    80,   168,   116,   505,   167,    81,   168,
     509,   238,   260,    82,    27,    28,    78,    79,   289,    44,
      45,    46,   164,   384,   271,    47,    48,   263,    49,    50,
      51,    52,    53,    83,   353,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   251,   307,   308,   290,   161,    84,
     164,   293,   294,   354,   392,   224,   123,   165,   269,   304,
     312,   309,   229,   230,   231,   232,   233,   319,   164,   167,
     124,   168,   317,   393,   228,   229,   230,   231,   232,   233,
     234,   394,   126,   235,   236,   165,   246,   247,   325,   237,
     166,   320,   321,   322,   323,   324,   326,   167,   125,   168,
     395,   334,   328,   165,   313,   314,    54,   307,   308,   128,
     341,   329,   330,   411,   412,   167,   413,   168,   147,   350,
     427,   429,   431,   340,   229,   230,   231,   232,   233,   234,
     355,   164,   235,   236,   238,    58,    59,   360,   237,   121,
     300,   122,   162,   358,   352,   346,   347,   238,    60,   129,
     374,   375,   373,   344,   345,   230,   231,   232,   233,   234,
     163,   376,   235,   236,   171,   465,   165,   466,   237,   378,
     368,   369,     2,   172,   386,   408,   409,   410,   167,   173,
     168,   396,   176,   397,     3,     4,   229,   230,   231,   232,
     233,   234,   404,   405,   235,   236,   238,   381,   382,   179,
     237,   212,   273,   406,   216,   407,     5,   295,   296,   297,
     298,     6,     7,   117,   117,     8,   217,   424,   223,   164,
     218,   254,     9,   221,   225,   248,   238,   242,   244,   245,
      10,   253,    11,   238,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,   148,
     259,  -102,   262,   164,   165,   264,   272,   149,   238,   303,
    -102,   428,   430,   432,   433,   434,   167,  -102,   168,   285,
     284,   286,   291,    27,    28,   301,    29,    30,    31,    32,
      33,   302,    34,    35,    36,   316,   305,   318,   165,   337,
     327,   150,   151,   372,   335,   336,   338,  -102,   152,   339,
     167,   343,   168,   348,   153,   357,   148,   349,  -102,   159,
    -102,  -102,  -102,  -102,   149,   351,   359,  -102,   361,   483,
     484,   485,   486,   364,  -102,  -102,  -102,   365,   366,   370,
     379,  -102,  -102,  -102,  -102,  -102,  -102,   164,  -102,   380,
     154,   383,  -102,  -102,  -102,  -102,   387,   388,   150,   151,
     390,   398,   391,   399,  -102,   152,   400,  -102,  -102,   401,
     425,   153,   403,   148,   414,  -102,   209,  -102,  -102,  -102,
    -102,   149,   165,   415,  -102,   416,   417,   385,   418,   437,
     419,  -102,  -102,  -102,   167,   420,   168,   421,  -102,  -102,
    -102,  -102,  -102,  -102,   435,  -102,   422,   154,   423,  -102,
    -102,  -102,  -102,   436,   438,   150,   151,   439,   440,   441,
     442,  -102,   152,   443,  -102,  -102,   444,   453,   153,   115,
     455,   457,   459,   463,  -102,  -102,  -102,  -102,   116,   464,
     471,   472,   469,   467,   477,   182,   473,   474,   475,  -102,
    -102,   468,   478,   479,   480,  -102,  -102,  -102,  -102,  -102,
    -102,   482,  -102,   487,   154,   489,  -102,  -102,  -102,  -102,
     328,   490,   491,   492,   493,   183,   494,   499,   115,   329,
     330,  -102,  -102,   500,   501,   503,   506,   116,   184,   185,
     186,   187,   507,    57,   182,   229,   230,   231,   232,   233,
     234,   377,   356,   188,   189,   311,   240,   170,     0,   190,
     191,   192,   193,   194,   195,     0,   196,     0,     0,     0,
     197,   198,   199,   200,   183,   445,   446,     0,     0,   447,
     448,   449,   450,   451,     0,   201,   202,   184,   185,   186,
     187,   389,   229,   230,   231,   232,   233,   234,     0,     0,
     235,   236,   188,   189,     0,     0,   237,     0,   190,   191,
     192,   193,   194,   195,     0,   196,     0,   238,     0,   197,
     198,   199,   200,   402,   229,   230,   231,   232,   233,   234,
       0,     0,   235,   236,   201,   202,     0,     0,   237,   229,
     230,   231,   232,   233,   234,     0,     0,   235,   236,     0,
       0,     0,     0,   237,   367,   230,   231,   232,   233,   234,
       0,     0,   235,   236,   238,     0,     0,     0,   237,   229,
     230,   231,   232,   233,   234,     0,     0,   235,    61,    62,
      63,    64,    65,   237,   229,   230,   231,   232,   233,   234,
       0,     0,     0,     0,     0,   456,   238,   458,   237,   460,
     461,   462,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   238,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   238,     0,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,     0,
       0,   238,   118,   119,   120,     0,   495,   496,   497,   498,
       0,     0,     0,     0,     0,     0,   238,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,    85,     0,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,     0,
      97,     0,    98,    99,   100,   101
};

static const yytype_int16 yycheck[] =
{
     118,    45,    46,   120,    45,    12,    43,    44,    45,    46,
     125,     9,   130,   131,     9,   133,    10,   135,   110,    43,
     138,    45,    46,   141,   174,    55,   110,   137,   145,   139,
     140,    61,   142,    27,    28,     9,   146,   470,    50,     9,
       9,   305,   159,    50,    18,     9,   156,    41,    42,   103,
     165,    58,   167,   168,     7,    62,    63,    64,    65,    66,
     210,   110,    15,    16,    17,   482,   176,    79,   183,   502,
      44,   115,   110,   506,   115,    44,     3,    41,   115,    42,
      49,   345,    56,     9,    58,    12,   503,    56,   110,    58,
     507,   115,   209,    60,   101,   102,   113,   114,    61,     4,
       5,     6,     9,   367,   221,    10,    11,   217,    13,    14,
      15,    16,    17,    60,    42,   230,   231,   232,   233,   234,
     235,   236,   237,   238,    98,    94,    95,   244,   122,    60,
       9,   246,   247,    61,    42,   250,    61,    44,    91,   254,
     258,   256,    43,    44,    45,    46,    47,   264,     9,    56,
      61,    58,   262,    61,    42,    43,    44,    45,    46,    47,
      48,    42,    61,    51,    52,    44,    43,    44,   285,    57,
      49,   265,   266,   267,   268,   269,   286,    56,    41,    58,
      61,   291,    44,    44,    99,   100,    91,    94,    95,    61,
     305,    53,    54,    79,    80,    56,    82,    58,     9,   316,
     418,   419,   420,    42,    43,    44,    45,    46,    47,    48,
     327,     9,    51,    52,   115,    19,    20,   335,    57,    59,
      18,    61,     9,   333,   318,    42,    43,   115,    32,    61,
     348,   349,   347,    42,    43,    44,    45,    46,    47,    48,
       9,   351,    51,    52,    58,    80,    44,    82,    57,   359,
      42,    43,     0,     9,   369,    79,    80,    81,    56,   110,
      58,   379,    41,   380,    12,    13,    43,    44,    45,    46,
      47,    48,   390,   391,    51,    52,   115,    77,    78,   110,
      57,   110,    59,   393,    44,   395,    34,    77,    78,    79,
      80,    39,    40,    56,    57,    43,    58,   415,    59,     9,
       8,    41,    50,    61,     9,    55,   115,    61,    61,    61,
      58,   110,    60,   115,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,     1,
      61,     3,    61,     9,    44,    61,    42,     9,   115,    49,
      12,   419,   420,   421,   422,   423,    56,    19,    58,    61,
      59,    41,    41,   101,   102,    58,   104,   105,   106,   107,
     108,    41,   110,   111,   112,    61,    41,    59,    44,   110,
      61,    43,    44,    49,    61,    61,    49,    49,    50,     9,
      56,     9,    58,    61,    56,    42,     1,    61,     3,    61,
      62,    63,    64,    65,     9,    61,    61,    12,    50,   477,
     478,   479,   480,    59,    19,    77,    78,     9,    42,    42,
      61,    83,    84,    85,    86,    87,    88,     9,    90,    61,
      92,    42,    94,    95,    96,    97,    42,     9,    43,    44,
      61,    55,    61,    55,    49,    50,    42,   109,   110,     9,
      79,    56,    42,     1,    42,     3,    61,    62,    63,    64,
      65,     9,    44,    61,    12,    42,    42,    49,    61,    78,
      61,    19,    77,    78,    56,    61,    58,    61,    83,    84,
      85,    86,    87,    88,    55,    90,    61,    92,    61,    94,
      95,    96,    97,    61,    61,    43,    44,    78,    61,    78,
      61,    49,    50,    61,   109,   110,    61,    81,    56,     3,
      55,    55,    55,    43,    62,    63,    64,    65,    12,    55,
      77,    78,    44,    79,    61,    19,    83,    84,    85,    77,
      78,    82,    61,    61,    61,    83,    84,    85,    86,    87,
      88,    45,    90,    61,    92,    61,    94,    95,    96,    97,
      44,    61,    61,    61,    80,    49,    61,    55,     3,    53,
      54,   109,   110,    82,    44,    55,    43,    12,    62,    63,
      64,    65,    45,     7,    19,    43,    44,    45,    46,    47,
      48,   354,   331,    77,    78,   257,   171,   126,    -1,    83,
      84,    85,    86,    87,    88,    -1,    90,    -1,    -1,    -1,
      94,    95,    96,    97,    49,    77,    78,    -1,    -1,    81,
      82,    83,    84,    85,    -1,   109,   110,    62,    63,    64,
      65,    42,    43,    44,    45,    46,    47,    48,    -1,    -1,
      51,    52,    77,    78,    -1,    -1,    57,    -1,    83,    84,
      85,    86,    87,    88,    -1,    90,    -1,   115,    -1,    94,
      95,    96,    97,    42,    43,    44,    45,    46,    47,    48,
      -1,    -1,    51,    52,   109,   110,    -1,    -1,    57,    43,
      44,    45,    46,    47,    48,    -1,    -1,    51,    52,    -1,
      -1,    -1,    -1,    57,    43,    44,    45,    46,    47,    48,
      -1,    -1,    51,    52,   115,    -1,    -1,    -1,    57,    43,
      44,    45,    46,    47,    48,    -1,    -1,    51,    62,    63,
      64,    65,    66,    57,    43,    44,    45,    46,    47,    48,
      -1,    -1,    -1,    -1,    -1,   438,   115,   440,    57,   442,
     443,   444,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   115,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   115,    -1,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    -1,
      -1,   115,    58,    59,    60,    -1,   489,   490,   491,   492,
      -1,    -1,    -1,    -1,    -1,    -1,   115,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,    19,    -1,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    -1,
      33,    -1,    35,    36,    37,    38
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   117,     0,    12,    13,    34,    39,    40,    43,    50,
      58,    60,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,   101,   102,   104,
     105,   106,   107,   108,   110,   111,   112,   119,   120,   121,
     126,   127,   128,     9,     4,     5,     6,    10,    11,    13,
      14,    15,    16,    17,    91,   130,   118,   118,    19,    20,
      32,    62,    63,    64,    65,    66,   126,     9,   110,   110,
       9,   126,   126,   103,   110,   155,   156,   110,   113,   114,
       9,   110,    60,    60,    60,    19,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    33,    35,    36,
      37,    38,   126,   126,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,     3,    12,   134,   130,   130,
     130,    59,    61,    61,    61,    41,    61,   122,    61,    61,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,     9,     1,     9,
      43,    44,    50,    56,    92,   132,   133,   131,   132,    61,
     131,   126,     9,     9,     9,    44,    49,    56,    58,   136,
     156,    58,     9,   110,   132,   132,    41,   142,   132,   110,
     123,   132,    19,    49,    62,    63,    64,    65,    77,    78,
      83,    84,    85,    86,    87,    88,    90,    94,    95,    96,
      97,   109,   110,   134,   135,   138,   132,   135,   135,    61,
     132,   135,   110,   124,   131,   135,    44,    58,     8,   125,
     135,    61,   131,    59,   136,     9,   136,   136,    42,    43,
      44,    45,    46,    47,    48,    51,    52,    57,   115,   154,
     155,   125,    61,   135,    61,    61,    43,    44,    55,    18,
      44,    98,   136,   110,    41,     9,    41,    55,    61,    61,
     131,   125,    61,   135,    61,     7,    15,    16,    17,    91,
     139,   131,    42,    59,   136,   136,   136,   136,   136,   136,
     136,   136,   136,   136,    59,    61,    41,   140,    42,    61,
     131,    41,   141,   136,   136,    77,    78,    79,    80,   150,
      18,    58,    41,    49,   136,    41,    49,    94,    95,   136,
     137,   150,   132,    99,   100,   129,    61,   135,    59,   131,
     139,   139,   139,   139,   139,   131,   135,    61,    44,    53,
      54,   135,   143,   144,   135,    61,    61,   110,    49,     9,
      42,   136,   137,     9,    42,    43,    42,    43,    61,    61,
     131,    61,   139,    42,    61,   131,   144,    42,   135,    61,
     132,    50,   145,   151,    59,     9,    42,    43,    42,    43,
      42,   137,    49,   136,   132,   132,   135,   143,   135,    61,
      61,    77,    78,    42,   137,    49,   136,    42,     9,    42,
      61,    61,    42,    61,    42,    61,   132,   131,    55,    55,
      42,     9,    42,    42,   132,   132,   135,   135,    79,    80,
      81,    79,    80,    82,    42,    61,    42,    42,    61,    61,
      61,    61,    61,    61,   132,    79,   146,   151,   146,   151,
     146,   151,   146,   146,   146,    55,    61,    78,    61,    78,
      61,    78,    61,    61,    61,    77,    78,    81,    82,    83,
      84,    85,   147,    81,   148,    55,   148,    55,   148,    55,
     148,   148,   148,    43,    55,    80,    82,    79,    82,    44,
     153,    77,    78,    83,    84,    85,   149,    61,    61,    61,
      61,   147,    45,   146,   146,   146,   146,    61,   149,    61,
      61,    61,    61,    80,    61,   148,   148,   148,   148,    55,
      82,    44,   152,    55,   147,   149,    43,    45,   147,   149
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1455 of yacc.c  */
#line 2155 "tools/intern/useasm/use.y"
    {   (yyval.psInst) = NULL;;}
    break;

  case 3:

/* Line 1455 of yacc.c  */
#line 2157 "tools/intern/useasm/use.y"
    {
	(yyval.psInst) = ProcessInstruction((yyvsp[(1) - (3)].psInst), &(yyvsp[(2) - (3)].sInst));
;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 2161 "tools/intern/useasm/use.y"
    {   (yyval.psInst) = (yyvsp[(1) - (3)].psInst);;}
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 2163 "tools/intern/useasm/use.y"
    {   OptProcInputRegs((yyvsp[(3) - (3)].psRegList)); (yyval.psInst) = (yyvsp[(1) - (3)].psInst);;}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 2165 "tools/intern/useasm/use.y"
    {   OptProcOutputRegs((yyvsp[(3) - (3)].psRegList)); (yyval.psInst) = (yyvsp[(1) - (3)].psInst);;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 2167 "tools/intern/useasm/use.y"
    {   (yyval.psInst) = (yyvsp[(1) - (2)].psInst);;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 2169 "tools/intern/useasm/use.y"
    {   (yyval.psInst) = ForceAlign((yyvsp[(2) - (3)].sOp).uOpcode, (yyvsp[(2) - (3)].sOp).uOp2, (yyvsp[(2) - (3)].sOp).pszSourceFile, (yyvsp[(2) - (3)].sOp).uSourceLine, &(yyvsp[(1) - (3)].psInst));;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 2171 "tools/intern/useasm/use.y"
    {	USE_INST	sLabelDesc;
	UseAsmInitInst(&sLabelDesc);
	ProcessLabel(&sLabelDesc, &(yyvsp[(2) - (3)].sOp));
	(yyval.psInst) = ProcessInstruction((yyvsp[(1) - (3)].psInst), &sLabelDesc); ;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 2176 "tools/intern/useasm/use.y"
    {
    if(IsScopeManagementEnabled() == IMG_TRUE)
    {
        CloseScope();
    }
    (yyval.psInst) = (yyvsp[(1) - (2)].psInst);
;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 2184 "tools/intern/useasm/use.y"
    {
    const USE_INST* psLastInst = GetLastInstruction();
    USE_INST sNewInst;
    
    UseAsmInitInst(&sNewInst);
    sNewInst.uOpcode = USEASM_OP_LABEL;
    sNewInst.uSourceLine = (yyvsp[(2) - (2)].sOp).uSourceLine;
    sNewInst.pszSourceFile = (yyvsp[(2) - (2)].sOp).pszSourceFile;
    sNewInst.asArg[0].uType = USEASM_REGTYPE_LABEL;
    sNewInst.asArg[0].uIndex = USEREG_INDEX_NONE;
    sNewInst.asArg[0].uFlags = 0;
    if(psLastInst != NULL && (psLastInst->uOpcode == USEASM_OP_LABEL) &&
	   (IsCurrentScopeLinkedToModifiableLabel(psLastInst->asArg[0].uNumber) == IMG_FALSE))
    {
        sNewInst.asArg[0].uNumber = ReOpenLastScope((yyvsp[(2) - (2)].sOp).pszSourceFile, (yyvsp[(2) - (2)].sOp).uSourceLine);
    }
    else
    {
        sNewInst.asArg[0].uNumber = OpenScope(NULL, IMG_FALSE, IMG_FALSE,
										(yyvsp[(2) - (2)].sOp).pszSourceFile, (yyvsp[(2) - (2)].sOp).uSourceLine);
    }
    if(IsScopeManagementEnabled() == IMG_FALSE)
    {
        CloseScope();
    }
    CheckOpcode(&sNewInst, 1, FALSE);
    (yyval.psInst) = ProcessInstruction((yyvsp[(1) - (2)].psInst), &sNewInst);
;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 2221 "tools/intern/useasm/use.y"
    {   (yyval.psRegList) = NULL;;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 2223 "tools/intern/useasm/use.y"
    {
    USEASM_PARSER_REG_LIST *psList = (yyvsp[(1) - (3)].psRegList);
    USEASM_REGTYPE uType = (yyvsp[(2) - (3)].n);
    USEASM_REGTYPE uNumber = (yyvsp[(3) - (3)].n);
    USEASM_PARSER_REG_LIST *psHd = NULL;

    psHd = (USEASM_PARSER_REG_LIST*)UseAsm_Malloc(sizeof(USEASM_PARSER_REG_LIST));
    UseAsm_MemSet(psHd, 0, sizeof(*psHd));

    psHd->psNext = psList;
    psHd->uType = uType;
    psHd->uNumber = uNumber;

    (yyval.psRegList) = psHd;
;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 2241 "tools/intern/useasm/use.y"
    {
    if (g_uSchedOffCount == 0)
    {
        g_uFirstNoSchedInstOffset = g_uInstOffset;
        g_psFirstNoSchedInst = g_psLastNoSchedInst = NULL;
        g_bStartedNoSchedRegion = TRUE;
    }
    g_uSchedOffCount++;
;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 2251 "tools/intern/useasm/use.y"
    {
    if (g_uSchedOffCount == 0)
    {
        yyerror(".schedon without preceding .schedoff");
    }
    else
    {
        g_uSchedOffCount--;
        if (g_uSchedOffCount == 0)
        {
            SetNoSchedFlag(g_psFirstNoSchedInst,
                    g_uFirstNoSchedInstOffset,
                    g_psLastNoSchedInst,
                    g_uLastNoSchedInstOffset);
        }
    }
;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 2269 "tools/intern/useasm/use.y"
    {   g_uSkipInvOnCount++;;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 2271 "tools/intern/useasm/use.y"
    {   if (g_uSkipInvOnCount == 0)
    {
        yyerror(".skipinvoff without preceding .skipinvon");
    }
    else
    {
        g_uSkipInvOnCount--;
    }
;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 2281 "tools/intern/useasm/use.y"
    {   if (g_uRepeatStackTop >= REPEAT_STACK_LENGTH)
    {
        yyerror("Too many nested .rptN instructions");
    }
    else
    {
        g_puRepeatStack[g_uRepeatStackTop] = (yyvsp[(1) - (1)].n) >> USEASM_OPFLAGS1_REPEAT_SHIFT;
        g_uRepeatStackTop++;
    }
;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 2292 "tools/intern/useasm/use.y"
    {   if (g_uRepeatStackTop == 0)
    {
        yyerror(".rptend without preceding .rptN instruction");
    }
    else
    {
        g_uRepeatStackTop--;
    }
;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 2302 "tools/intern/useasm/use.y"
    {   SetTarget((SGX_CORE_ID_TYPE)(yyvsp[(2) - (2)].n), 0);;}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 2304 "tools/intern/useasm/use.y"
    {   SetTarget((SGX_CORE_ID_TYPE)(yyvsp[(2) - (4)].n), (yyvsp[(4) - (4)].n));;}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 2306 "tools/intern/useasm/use.y"
    {
    FindScope((yyvsp[(2) - (2)].sOp).pszCharString, EXPORT_SCOPE, (yyvsp[(2) - (2)].sOp).pszSourceFile, (yyvsp[(2) - (2)].sOp).uSourceLine);
    UseAsm_Free((yyvsp[(2) - (2)].sOp).pszCharString);
;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 2311 "tools/intern/useasm/use.y"
    {
    OpenScope((yyvsp[(2) - (2)].sOp).pszCharString, IMG_FALSE, IMG_TRUE, (yyvsp[(2) - (2)].sOp).pszSourceFile, (yyvsp[(2) - (2)].sOp).uSourceLine);
    CloseScope();
    UseAsm_Free((yyvsp[(2) - (2)].sOp).pszCharString);
;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 2317 "tools/intern/useasm/use.y"
    {
    g_uModuleAlignment = gcd((yyvsp[(2) - (2)].n), g_uModuleAlignment);
;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 2321 "tools/intern/useasm/use.y"
    {;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 2323 "tools/intern/useasm/use.y"
    {
    IMG_UINT32 uRangeStart = (yyvsp[(2) - (4)].n);
    IMG_UINT32 uRangeEnd = (yyvsp[(4) - (4)].n);
    if(uRangeStart> uRangeEnd)
    {
        ParseError("Range start '%u' can not be larger than range end '%u'",
                uRangeStart, uRangeEnd);
    }
    else
    {

        SetHwRegsAllocRangeFromFile(uRangeStart, uRangeEnd);
    }
;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 2338 "tools/intern/useasm/use.y"
    {
    RenameNamedRegister((yyvsp[(2) - (4)].sOp).pszCharString, (yyvsp[(2) - (4)].sOp).pszSourceFile,
            (yyvsp[(2) - (4)].sOp).uSourceLine, (yyvsp[(4) - (4)].sOp).pszCharString,
            (yyvsp[(4) - (4)].sOp).pszSourceFile, (yyvsp[(4) - (4)].sOp).uSourceLine);
    UseAsm_Free((yyvsp[(2) - (4)].sOp).pszCharString);
    UseAsm_Free((yyvsp[(4) - (4)].sOp).pszCharString);
;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 2349 "tools/intern/useasm/use.y"
    {
    (yyval.sOp).uSourceLine = (yyvsp[(1) - (2)].sOp).uSourceLine;
    (yyval.sOp).pszSourceFile = (yyvsp[(1) - (2)].sOp).pszSourceFile;
    (yyval.sOp).uOpcode = (yyvsp[(2) - (2)].n);
    (yyval.sOp).uOp2 = 0;
;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 2356 "tools/intern/useasm/use.y"
    {   (yyval.sOp).uSourceLine = (yyvsp[(1) - (4)].sOp).uSourceLine;
    (yyval.sOp).pszSourceFile = (yyvsp[(1) - (4)].sOp).pszSourceFile;
    (yyval.sOp).uOpcode = (yyvsp[(2) - (4)].n);
    (yyval.sOp).uOp2 = (yyvsp[(4) - (4)].n);;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 2361 "tools/intern/useasm/use.y"
    {   (yyval.sOp).uSourceLine = (yyvsp[(1) - (1)].sOp).uSourceLine;
    (yyval.sOp).pszSourceFile = (yyvsp[(1) - (1)].sOp).pszSourceFile;
    (yyval.sOp).uOpcode = 2;
    (yyval.sOp).uOp2 = 1;;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 2369 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (7)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (7)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (7)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (7)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (7)].sMod).uFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (7)].sMod).uFlags3;
    (yyval.sInst).uTest = (yyvsp[(3) - (7)].sMod).uTest;
    (yyval.sInst).uFlags1 |= ((yyvsp[(5) - (7)].n) << USEASM_OPFLAGS1_MASK_SHIFT);
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (7)].sMod).uFlags1;
    (yyval.sInst).asArg[0] = (yyvsp[(4) - (7)].sRegister);
    UseAsm_MemCopy((yyval.sInst).asArg + 1, (yyvsp[(7) - (7)].sArgs).asArg, (yyvsp[(7) - (7)].sArgs).uCount * sizeof((yyval.sInst).asArg[0]));
    CheckOpcode(&(yyval.sInst), (yyvsp[(7) - (7)].sArgs).uCount + 1, (yyvsp[(1) - (7)].sMod).bRepeatPresent || (yyvsp[(3) - (7)].sMod).bRepeatPresent);
    if (((yyval.sInst).uOpcode >= USEASM_OP_PCKF16F32 && (yyval.sInst).uOpcode <= USEASM_OP_UNPCKC10C10) ||
            ((yyval.sInst).uOpcode == USEASM_OP_SOP2WM))
    {
        if (!((yyval.sInst).asArg[0].uFlags & USEASM_ARGFLAGS_BYTEMSK_PRESENT))
        {
            (yyval.sInst).asArg[0].uFlags |= (0xF << USEASM_ARGFLAGS_BYTEMSK_SHIFT);
        }
    };}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 2390 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (7)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (7)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (7)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (7)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (7)].sMod).uFlags2 | USEASM_OPFLAGS2_COISSUE;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (7)].sMod).uFlags3;
    (yyval.sInst).uTest = (yyvsp[(3) - (7)].sMod).uTest;
    (yyval.sInst).uFlags1 |= ((yyvsp[(5) - (7)].n) << USEASM_OPFLAGS1_MASK_SHIFT);
    (yyval.sInst).asArg[0] = (yyvsp[(4) - (7)].sRegister);
    UseAsm_MemCopy((yyval.sInst).asArg + 1, (yyvsp[(7) - (7)].sArgs).asArg, (yyvsp[(7) - (7)].sArgs).uCount * sizeof((yyval.sInst).asArg[0]));
    CheckOpcode(&(yyval.sInst), (yyvsp[(7) - (7)].sArgs).uCount + 1, (yyvsp[(3) - (7)].sMod).bRepeatPresent);
    if (((yyval.sInst).uOpcode >= USEASM_OP_PCKF16F32 && (yyval.sInst).uOpcode <= USEASM_OP_UNPCKC10C10) ||
            ((yyval.sInst).uOpcode == USEASM_OP_SOP2WM))
    {
        if (!((yyval.sInst).asArg[0].uFlags & USEASM_ARGFLAGS_BYTEMSK_PRESENT))
        {
            (yyval.sInst).asArg[0].uFlags |= (0xF << USEASM_ARGFLAGS_BYTEMSK_SHIFT);
        }
    };}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 2410 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (8)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (8)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (8)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (8)].sMod).uFlags1 | (yyvsp[(6) - (8)].sLdStArg).uOpFlags1;
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (8)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (8)].sMod).uFlags2 | (yyvsp[(6) - (8)].sLdStArg).uOpFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (8)].sMod).uFlags3;
    (yyval.sInst).uTest = (yyvsp[(3) - (8)].sMod).uTest;
    (yyval.sInst).asArg[0] = (yyvsp[(4) - (8)].sRegister);
    UseAsm_MemCopy((yyval.sInst).asArg + 1, (yyvsp[(6) - (8)].sLdStArg).asArg, (yyvsp[(6) - (8)].sLdStArg).uCount * sizeof((yyval.sInst).asArg[0]));
    if ((yyvsp[(6) - (8)].sLdStArg).uCount == 1)
    {
		InitialiseDefaultRegister(&(yyval.sInst).asArg[2]);
		(yyval.sInst).asArg[2].uType = USEASM_REGTYPE_IMMEDIATE;
		(yyval.sInst).asArg[2].uNumber = 0;
    }
    UseAsm_MemCopy((yyval.sInst).asArg + 3, (yyvsp[(8) - (8)].sArgs).asArg, (yyvsp[(8) - (8)].sArgs).uCount * sizeof((yyval.sInst).asArg[0]));
    CheckOpcode(&(yyval.sInst), 3 + (yyvsp[(8) - (8)].sArgs).uCount, (yyvsp[(1) - (8)].sMod).bRepeatPresent || (yyvsp[(3) - (8)].sMod).bRepeatPresent);;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 2429 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (6)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (6)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (6)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (6)].sMod).uFlags1 | (yyvsp[(4) - (6)].sLdStArg).uOpFlags1;
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (6)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (6)].sMod).uFlags2 | (yyvsp[(4) - (6)].sLdStArg).uOpFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (6)].sMod).uFlags3;
    (yyval.sInst).uTest = (yyvsp[(3) - (6)].sMod).uTest;
    UseAsm_MemCopy((yyval.sInst).asArg, (yyvsp[(4) - (6)].sLdStArg).asArg, (yyvsp[(4) - (6)].sLdStArg).uCount * sizeof((yyval.sInst).asArg[0]));
    if ((yyvsp[(4) - (6)].sLdStArg).uCount == 1)
    {
		InitialiseDefaultRegister(&(yyval.sInst).asArg[1]);
		(yyval.sInst).asArg[1].uType = USEASM_REGTYPE_IMMEDIATE;
		(yyval.sInst).asArg[1].uNumber = 0;
    }
    UseAsm_MemCopy((yyval.sInst).asArg + 2, (yyvsp[(6) - (6)].sArgs).asArg, (yyvsp[(6) - (6)].sArgs).uCount * sizeof((yyval.sInst).asArg[0]));
    CheckOpcode(&(yyval.sInst), 2 + (yyvsp[(6) - (6)].sArgs).uCount, (yyvsp[(1) - (6)].sMod).bRepeatPresent || (yyvsp[(3) - (6)].sMod).bRepeatPresent);;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 2447 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (10)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (10)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (10)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (10)].sMod).uFlags1;
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (10)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (10)].sMod).uFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (10)].sMod).uFlags3;
    (yyval.sInst).uTest = (yyvsp[(3) - (10)].sMod).uTest;
    (yyval.sInst).asArg[0] = (yyvsp[(4) - (10)].sRegister);
    UseAsm_MemCopy((yyval.sInst).asArg + 1, (yyvsp[(6) - (10)].sLdStArg).asArg, (yyvsp[(6) - (10)].sLdStArg).uCount * sizeof((yyval.sInst).asArg[0]));
    (yyval.sInst).asArg[(yyvsp[(6) - (10)].sLdStArg).uCount + 1] = (yyvsp[(8) - (10)].sRegister);
    (yyval.sInst).asArg[(yyvsp[(6) - (10)].sLdStArg).uCount + 2] = (yyvsp[(10) - (10)].sRegister);
    CheckOpcode(&(yyval.sInst), (yyvsp[(6) - (10)].sLdStArg).uCount + 3, (yyvsp[(1) - (10)].sMod).bRepeatPresent || (yyvsp[(3) - (10)].sMod).bRepeatPresent);;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 2461 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (4)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (4)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (4)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (4)].sMod).uFlags1;
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (4)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (4)].sMod).uFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (4)].sMod).uFlags3;
    (yyval.sInst).uTest = (yyvsp[(3) - (4)].sMod).uTest;
    (yyval.sInst).asArg[0] = (yyvsp[(4) - (4)].sRegister);
    CheckOpcode(&(yyval.sInst), 1, (yyvsp[(1) - (4)].sMod).bRepeatPresent || (yyvsp[(3) - (4)].sMod).bRepeatPresent);;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 2472 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (4)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (4)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (4)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (4)].sMod).uFlags1 | USEASM_OPFLAGS1_SAVELINK;
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (4)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (4)].sMod).uFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (4)].sMod).uFlags3;
    (yyval.sInst).uTest = (yyvsp[(3) - (4)].sMod).uTest;
    (yyval.sInst).asArg[0] = (yyvsp[(4) - (4)].sRegister);
    CheckOpcode(&(yyval.sInst), 1, (yyvsp[(1) - (4)].sMod).bRepeatPresent || (yyvsp[(3) - (4)].sMod).bRepeatPresent);;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 2484 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (10)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (10)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (10)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (10)].sMod).uFlags1;
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (10)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (10)].sMod).uFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (10)].sMod).uFlags3;
    (yyval.sInst).uTest = (yyvsp[(3) - (10)].sMod).uTest;
    (yyval.sInst).asArg[0] = (yyvsp[(4) - (10)].sRegister);
    UseAsm_MemCopy((yyval.sInst).asArg + 1, (yyvsp[(10) - (10)].sArgs).asArg, (yyvsp[(10) - (10)].sArgs).uCount * sizeof((yyval.sInst).asArg[0]));
    (yyval.sInst).asArg[(yyvsp[(10) - (10)].sArgs).uCount + 1].uNumber = (yyvsp[(8) - (10)].n) | ((yyvsp[(6) - (10)].n) << EURASIA_USE1_EFO_DSRC_SHIFT);
    CheckOpcode(&(yyval.sInst), (yyvsp[(10) - (10)].sArgs).uCount + 2, (yyvsp[(1) - (10)].sMod).bRepeatPresent || (yyvsp[(3) - (10)].sMod).bRepeatPresent);;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 2497 "tools/intern/useasm/use.y"
    { ProcessLabel(&(yyval.sInst), &(yyvsp[(1) - (2)].sOp)); ;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 2499 "tools/intern/useasm/use.y"
    {
    if(IsCurrentScopeGlobal() == IMG_TRUE)
    {
        (yyvsp[(2) - (2)].sOp).uOpcode = OpenScope((yyvsp[(2) - (2)].sOp).pszCharString,
							   IMG_TRUE, IMG_FALSE,
							   (yyvsp[(2) - (2)].sOp).pszSourceFile, (yyvsp[(2) - (2)].sOp).uSourceLine);
    }
    else
    {
        ParseErrorAt((yyvsp[(2) - (2)].sOp).pszSourceFile, (yyvsp[(2) - (2)].sOp).uSourceLine,
					 "Can not create procedure at nested level");
        (yyvsp[(2) - (2)].sOp).uOpcode = OpenScope((yyvsp[(2) - (2)].sOp).pszCharString,
							   IMG_FALSE, IMG_FALSE,
							   (yyvsp[(2) - (2)].sOp).pszSourceFile, (yyvsp[(2) - (2)].sOp).uSourceLine);
    }
    UseAsm_Free((yyvsp[(2) - (2)].sOp).pszCharString);
    (yyvsp[(2) - (2)].sOp).pszCharString = NULL;
;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 2518 "tools/intern/useasm/use.y"
    {
    (yyval.sInst).uOpcode = USEASM_OP_LABEL;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (6)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (6)].sOp).pszSourceFile;
    (yyval.sInst).asArg[0].uType = USEASM_REGTYPE_LABEL;
    (yyval.sInst).asArg[0].uNumber = (yyvsp[(2) - (6)].sOp).uOpcode;
    (yyval.sInst).asArg[0].uIndex = USEREG_INDEX_NONE;
    (yyval.sInst).asArg[0].uFlags = 0;
    CheckOpcode(&(yyval.sInst), 1, FALSE);
    CloseScope();
;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 2530 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (3)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (3)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (3)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (3)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (3)].sMod).uFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (3)].sMod).uFlags3;
    (yyval.sInst).uTest = (yyvsp[(3) - (3)].sMod).uTest;
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (3)].sMod).uFlags1;
    CheckOpcode(&(yyval.sInst), 0, (yyvsp[(1) - (3)].sMod).bRepeatPresent);;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 2540 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (3)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (3)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (3)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (3)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (3)].sMod).uFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (3)].sMod).uFlags3;
    (yyval.sInst).uTest = (yyvsp[(3) - (3)].sMod).uTest;
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (3)].sMod).uFlags1;
    CheckOpcode(&(yyval.sInst), 0, TRUE);;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 2550 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (3)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (3)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (3)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (3)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (3)].sMod).uFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (3)].sMod).uFlags3;
    (yyval.sInst).uTest = 0;
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (3)].sMod).uFlags1;
    CheckOpcode(&(yyval.sInst), 0, (yyvsp[(1) - (3)].sMod).bRepeatPresent || (yyvsp[(3) - (3)].sMod).bRepeatPresent);;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 2560 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (4)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (4)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (4)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (4)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (4)].sMod).uFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (4)].sMod).uFlags3;
    (yyval.sInst).uTest = 0;
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (4)].sMod).uFlags1;
    (yyval.sInst).asArg[0] = (yyvsp[(4) - (4)].sRegister);
    (yyval.sInst).asArg[0].uFlags = 0;
    CheckOpcode(&(yyval.sInst), 1, (yyvsp[(1) - (4)].sMod).bRepeatPresent || (yyvsp[(3) - (4)].sMod).bRepeatPresent);;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 2572 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(1) - (2)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(1) - (2)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(1) - (2)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(2) - (2)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(2) - (2)].sMod).uFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(2) - (2)].sMod).uFlags3;
    (yyval.sInst).uTest = 0;
    CheckOpcode(&(yyval.sInst), 0, FALSE);;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 2581 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (4)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (4)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (4)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (4)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (4)].sMod).uFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (4)].sMod).uFlags3;
    (yyval.sInst).uTest = 0;
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (4)].sMod).uFlags1;
    (yyval.sInst).asArg[0] = (yyvsp[(4) - (4)].sRegister);
    (yyval.sInst).asArg[0].uFlags = 0;
    CheckOpcode(&(yyval.sInst), 1, (yyvsp[(1) - (4)].sMod).bRepeatPresent || (yyvsp[(3) - (4)].sMod).bRepeatPresent);;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 2593 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (8)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (8)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (8)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (8)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (8)].sMod).uFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (8)].sMod).uFlags3;
    (yyval.sInst).uTest = 0;
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (8)].sMod).uFlags1;
    (yyval.sInst).asArg[0] = (yyvsp[(4) - (8)].sRegister);
    (yyval.sInst).asArg[0].uFlags = 0;
    (yyval.sInst).asArg[1] = (yyvsp[(6) - (8)].sRegister);
    (yyval.sInst).asArg[1].uFlags = 0;
    (yyval.sInst).asArg[2] = (yyvsp[(8) - (8)].sRegister);
    (yyval.sInst).asArg[2].uFlags = 0;
    CheckOpcode(&(yyval.sInst), 3, (yyvsp[(1) - (8)].sMod).bRepeatPresent || (yyvsp[(3) - (8)].sMod).bRepeatPresent);
;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 2610 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (4)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (4)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (4)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (4)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (4)].sMod).uFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (4)].sMod).uFlags3;
    (yyval.sInst).uTest = 0;
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (4)].sMod).uFlags1;
    (yyval.sInst).asArg[0] = (yyvsp[(4) - (4)].sRegister);
    (yyval.sInst).asArg[0].uFlags = 0;
    CheckOpcode(&(yyval.sInst), 1, (yyvsp[(1) - (4)].sMod).bRepeatPresent || (yyvsp[(3) - (4)].sMod).bRepeatPresent);;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 2622 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (6)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (6)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (6)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (6)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (6)].sMod).uFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (6)].sMod).uFlags3;
    (yyval.sInst).uTest = 0;
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (6)].sMod).uFlags1;
    (yyval.sInst).asArg[0] = (yyvsp[(4) - (6)].sRegister);
    (yyval.sInst).asArg[0].uFlags = 0;
    (yyval.sInst).asArg[1].uType = USEASM_REGTYPE_IMMEDIATE;
    (yyval.sInst).asArg[1].uNumber = (yyvsp[(6) - (6)].n);
    (yyval.sInst).asArg[1].uFlags = 0;
    (yyval.sInst).asArg[1].uIndex = USEREG_INDEX_NONE;
    CheckOpcode(&(yyval.sInst), 2, (yyvsp[(1) - (6)].sMod).bRepeatPresent || (yyvsp[(3) - (6)].sMod).bRepeatPresent);;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 2639 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (12)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (12)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (12)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (12)].sMod).uFlags1;
    (yyval.sInst).uTest = 0;
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (12)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (12)].sMod).uFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (12)].sMod).uFlags3;
    (yyval.sInst).asArg[0] = (yyvsp[(4) - (12)].sRegister);
    (yyval.sInst).asArg[1] = (yyvsp[(8) - (12)].sRegister);
    (yyval.sInst).asArg[2] = (yyvsp[(10) - (12)].sRegister);
    (yyval.sInst).asArg[3] = (yyvsp[(12) - (12)].sRegister);
    (yyval.sInst).asArg[4].uNumber = (yyvsp[(6) - (12)].n) << EURASIA_USE1_EFO_DSRC_SHIFT;
    CheckOpcode(&(yyval.sInst), 4, (yyvsp[(1) - (12)].sMod).bRepeatPresent || (yyvsp[(3) - (12)].sMod).bRepeatPresent);;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 2655 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (10)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (10)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (10)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (10)].sMod).uFlags1;
    (yyval.sInst).uTest = 0;
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (10)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (10)].sMod).uFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (10)].sMod).uFlags3;
    (yyval.sInst).asArg[0] = (yyvsp[(4) - (10)].sRegister);
    (yyval.sInst).asArg[1] = (yyvsp[(6) - (10)].sRegister);
    (yyval.sInst).asArg[2] = (yyvsp[(8) - (10)].sRegister);
    (yyval.sInst).asArg[3] = (yyvsp[(10) - (10)].sRegister);
    switch ((yyvsp[(2) - (10)].sOp).uOpcode)
    {
        case USEASM_OP_AMM:
        case USEASM_OP_SMM:
        case USEASM_OP_AMS:
        case USEASM_OP_SMS:
			(yyval.sInst).asArg[4].uNumber = (EURASIA_USE1_EFO_DSRC_A1 << EURASIA_USE1_EFO_DSRC_SHIFT);
			break;
        case USEASM_OP_DMA:
			(yyval.sInst).asArg[4].uNumber = (EURASIA_USE1_EFO_DSRC_A0 << EURASIA_USE1_EFO_DSRC_SHIFT);
			break;
        default: (yyval.sInst).asArg[4].uNumber = 0;
    }
    CheckOpcode(&(yyval.sInst), 4, (yyvsp[(1) - (10)].sMod).bRepeatPresent || (yyvsp[(3) - (10)].sMod).bRepeatPresent);;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 2682 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (4)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (4)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (4)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (4)].sMod).uFlags1;
    (yyval.sInst).uTest = 0;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (4)].sMod).uFlags2 | USEASM_OPFLAGS2_COISSUE;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (4)].sMod).uFlags3;
    UseAsm_MemCopy((yyval.sInst).asArg, (yyvsp[(4) - (4)].sArgs).asArg, (yyvsp[(4) - (4)].sArgs).uCount * sizeof((yyval.sInst).asArg[0]));
    CheckOpcode(&(yyval.sInst), (yyvsp[(4) - (4)].sArgs).uCount, (yyvsp[(3) - (4)].sMod).bRepeatPresent);
;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 2693 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (4)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (4)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (4)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (4)].sMod).uFlags1;
    (yyval.sInst).uTest = 0;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (4)].sMod).uFlags2 | USEASM_OPFLAGS2_COISSUE;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (4)].sMod).uFlags3;
    UseAsm_MemCopy((yyval.sInst).asArg, (yyvsp[(4) - (4)].sArgs).asArg, (yyvsp[(4) - (4)].sArgs).uCount * sizeof((yyval.sInst).asArg[0]));
    CheckOpcode(&(yyval.sInst), (yyvsp[(4) - (4)].sArgs).uCount, (yyvsp[(3) - (4)].sMod).bRepeatPresent);
;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 2704 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (5)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (5)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (5)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (5)].sMod).uFlags1;
    (yyval.sInst).uTest = 0;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (5)].sMod).uFlags2 | USEASM_OPFLAGS2_COISSUE;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (5)].sMod).uFlags3;
    (yyval.sInst).asArg[0].uType = USEASM_REGTYPE_INTSRCSEL;
    (yyval.sInst).asArg[0].uNumber = USEASM_INTSRCSEL_NONE;
    (yyval.sInst).asArg[0].uFlags = 0;
    (yyval.sInst).asArg[0].uIndex = USEREG_INDEX_NONE;
    UseAsm_MemCopy((yyval.sInst).asArg + 1, (yyvsp[(5) - (5)].sArgs).asArg, (yyvsp[(5) - (5)].sArgs).uCount * sizeof((yyval.sInst).asArg[0]));
    CheckOpcode(&(yyval.sInst), (yyvsp[(5) - (5)].sArgs).uCount + 1, (yyvsp[(3) - (5)].sMod).bRepeatPresent);
;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 2719 "tools/intern/useasm/use.y"
    {   (yyval.sInst).uOpcode = (yyvsp[(2) - (7)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (7)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (7)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (7)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (7)].sMod).uFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (7)].sMod).uFlags3;
    (yyval.sInst).uTest = (yyvsp[(3) - (7)].sMod).uTest;
    (yyval.sInst).uFlags1 |= ((yyvsp[(5) - (7)].n) << USEASM_OPFLAGS1_MASK_SHIFT);
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (7)].sMod).uFlags1;
    (yyval.sInst).asArg[0] = (yyvsp[(4) - (7)].sRegister);
    if (((yyval.sInst).asArg[0].uFlags & USEASM_ARGFLAGS_NOT) != 0) 
    {
		(yyval.sInst).asArg[0].uFlags &= ~USEASM_ARGFLAGS_NOT;
		(yyval.sInst).asArg[0].uFlags |= USEASM_ARGFLAGS_DISABLEWB;
	}
    UseAsm_MemCopy((yyval.sInst).asArg + 1, (yyvsp[(7) - (7)].sArgs).asArg, (yyvsp[(7) - (7)].sArgs).uCount * sizeof((yyval.sInst).asArg[0]));
    CheckOpcode(&(yyval.sInst), (yyvsp[(7) - (7)].sArgs).uCount + 1, (yyvsp[(1) - (7)].sMod).bRepeatPresent || (yyvsp[(3) - (7)].sMod).bRepeatPresent);;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 2737 "tools/intern/useasm/use.y"
    {
    (yyval.sInst).uOpcode = (yyvsp[(2) - (5)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (5)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (5)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (5)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (5)].sMod).uFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (5)].sMod).uFlags3;
    (yyval.sInst).uTest = (yyvsp[(3) - (5)].sMod).uTest;
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (5)].sMod).uFlags1;
    (yyval.sInst).asArg[0].uType = USEASM_REGTYPE_FPINTERNAL;
    (yyval.sInst).asArg[0].uNumber = 0;
    (yyval.sInst).asArg[0].uFlags = USEASM_ARGFLAGS_DISABLEWB;
    (yyval.sInst).asArg[0].uIndex = USEREG_INDEX_NONE;
    UseAsm_MemCopy((yyval.sInst).asArg + 1, (yyvsp[(5) - (5)].sArgs).asArg, (yyvsp[(5) - (5)].sArgs).uCount * sizeof((yyval.sInst).asArg[0]));
    CheckOpcode(&(yyval.sInst), (yyvsp[(5) - (5)].sArgs).uCount + 1, (yyvsp[(1) - (5)].sMod).bRepeatPresent || (yyvsp[(3) - (5)].sMod).bRepeatPresent);
;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 2754 "tools/intern/useasm/use.y"
    {
    (yyval.sInst).uOpcode = (yyvsp[(2) - (4)].sOp).uOpcode;
    (yyval.sInst).uSourceLine = (yyvsp[(2) - (4)].sOp).uSourceLine;
    (yyval.sInst).pszSourceFile = (yyvsp[(2) - (4)].sOp).pszSourceFile;
    (yyval.sInst).uFlags1 = (yyvsp[(3) - (4)].sMod).uFlags1;
    (yyval.sInst).uFlags2 = (yyvsp[(3) - (4)].sMod).uFlags2;
    (yyval.sInst).uFlags3 = (yyvsp[(3) - (4)].sMod).uFlags3;
    (yyval.sInst).uTest = (yyvsp[(3) - (4)].sMod).uTest;
    (yyval.sInst).uFlags1 |= (yyvsp[(1) - (4)].sMod).uFlags1;
    UseAsm_MemCopy((yyval.sInst).asArg, (yyvsp[(4) - (4)].sArgs).asArg, (yyvsp[(4) - (4)].sArgs).uCount * sizeof((yyval.sInst).asArg[0]));
    CheckOpcode(&(yyval.sInst), (yyvsp[(4) - (4)].sArgs).uCount, (yyvsp[(1) - (4)].sMod).bRepeatPresent || (yyvsp[(3) - (4)].sMod).bRepeatPresent);
;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 2770 "tools/intern/useasm/use.y"
    {
    IMG_UINT32 uN = FindScope((yyvsp[(1) - (1)].sOp).pszCharString,
							  BRANCH_SCOPE,
							  (yyvsp[(1) - (1)].sOp).pszSourceFile,
							  (yyvsp[(1) - (1)].sOp).uSourceLine);
    UseAsm_Free((yyvsp[(1) - (1)].sOp).pszCharString);
    (yyval.sRegister).uType = USEASM_REGTYPE_LABEL; (yyval.sRegister).uNumber = uN;
;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 2779 "tools/intern/useasm/use.y"
    {   IMG_UINT32 uN = FindScope((yyvsp[(1) - (3)].sOp).pszCharString,
							  BRANCH_SCOPE,
							  (yyvsp[(1) - (3)].sOp).pszSourceFile,
							  (yyvsp[(1) - (3)].sOp).uSourceLine);
    UseAsm_Free((yyvsp[(1) - (3)].sOp).pszCharString);
    (yyval.sRegister).uType = USEASM_REGTYPE_LABEL_WITH_OFFSET;
    (yyval.sRegister).uNumber = uN;
    (yyval.sRegister).uFlags = (yyvsp[(3) - (3)].n);
;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 2789 "tools/intern/useasm/use.y"
    {
    IMG_UINT32 uN = FindScope((yyvsp[(1) - (3)].sOp).pszCharString,
							  BRANCH_SCOPE, (yyvsp[(1) - (3)].sOp).pszSourceFile,
							  (yyvsp[(1) - (3)].sOp).uSourceLine);
    UseAsm_Free((yyvsp[(1) - (3)].sOp).pszCharString);
    (yyval.sRegister).uType = USEASM_REGTYPE_LABEL_WITH_OFFSET;
    (yyval.sRegister).uNumber = uN;
    (yyval.sRegister).uFlags = -(IMG_INT32)(yyvsp[(3) - (3)].n);
;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 2802 "tools/intern/useasm/use.y"
    {
    IMG_UINT32 uN = FindScope((yyvsp[(1) - (1)].sOp).pszCharString,
							  PROCEDURE_SCOPE,
							  (yyvsp[(1) - (1)].sOp).pszSourceFile,
							  (yyvsp[(1) - (1)].sOp).uSourceLine);
    UseAsm_Free((yyvsp[(1) - (1)].sOp).pszCharString);
    (yyval.sRegister).uType = USEASM_REGTYPE_LABEL; (yyval.sRegister).uNumber = uN;
;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 2814 "tools/intern/useasm/use.y"
    {   (yyval.n) = USEREG_MASK_X;;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 2816 "tools/intern/useasm/use.y"
    {   (yyval.n) = (yyvsp[(1) - (1)].n);;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 2821 "tools/intern/useasm/use.y"
    {   (yyval.sMod).uFlags1 = 0; (yyval.sMod).bRepeatPresent = FALSE;;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 2823 "tools/intern/useasm/use.y"
    {   (yyval.sMod).uFlags1 = ((yyvsp[(1) - (2)].n) << USEASM_OPFLAGS1_PRED_SHIFT) | (yyvsp[(2) - (2)].sMod).uFlags1;
    (yyval.sMod).bRepeatPresent = (yyvsp[(2) - (2)].sMod).bRepeatPresent;
;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 2827 "tools/intern/useasm/use.y"
    {   (yyval.sMod).uFlags1 = (yyvsp[(1) - (2)].n) | (yyvsp[(2) - (2)].sMod).uFlags1; (yyval.sMod).bRepeatPresent = TRUE;;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 2829 "tools/intern/useasm/use.y"
    {   (yyval.sMod).uFlags1 = USEASM_OPFLAGS1_SKIPINVALID | (yyvsp[(2) - (2)].sMod).uFlags1; (yyval.sMod).bRepeatPresent = TRUE;;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 2831 "tools/intern/useasm/use.y"
    {   (yyval.sMod).uFlags1 = USEASM_OPFLAGS1_NOSCHED | (yyvsp[(2) - (2)].sMod).uFlags1; (yyval.sMod).bRepeatPresent = TRUE;;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 2833 "tools/intern/useasm/use.y"
    {   (yyval.sMod).uFlags1 = (yyvsp[(2) - (3)].sMod).uFlags1;
    (yyval.sMod).bRepeatPresent = (yyvsp[(2) - (3)].sMod).bRepeatPresent;;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 2836 "tools/intern/useasm/use.y"
    {   (yyval.sMod).uFlags1 = (yyvsp[(2) - (5)].sMod).uFlags1 | (yyvsp[(4) - (5)].sMod).uFlags1;
    (yyval.sMod).bRepeatPresent = (yyvsp[(2) - (5)].sMod).bRepeatPresent || (yyvsp[(4) - (5)].sMod).bRepeatPresent;
;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 2843 "tools/intern/useasm/use.y"
    {   (yyval.n) = USEASM_PRED_P0;;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 2845 "tools/intern/useasm/use.y"
    {   (yyval.n) = USEASM_PRED_P1;;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 2847 "tools/intern/useasm/use.y"
    {   (yyval.n) = USEASM_PRED_NEGP0;;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 2849 "tools/intern/useasm/use.y"
    {   (yyval.n) = USEASM_PRED_NEGP1;;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 2851 "tools/intern/useasm/use.y"
    {   (yyval.n) = USEASM_PRED_NEGP2;;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 2853 "tools/intern/useasm/use.y"
    {   (yyval.n) = USEASM_PRED_P2;;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 2855 "tools/intern/useasm/use.y"
    {   (yyval.n) = USEASM_PRED_P3;;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 2857 "tools/intern/useasm/use.y"
    {   (yyval.n) = USEASM_PRED_PN;;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 2860 "tools/intern/useasm/use.y"
    {   yyerror("Only P0 and P1 can be negated"); (yyval.n) = 0;;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 2862 "tools/intern/useasm/use.y"
    {   yyerror("Only P0 and P1 can be negated"); (yyval.n) = 0;;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 2866 "tools/intern/useasm/use.y"
    {   (yyval.n) = ((yyvsp[(2) - (2)].n) << USEASM_OPFLAGS1_REPEAT_SHIFT);;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 2871 "tools/intern/useasm/use.y"
    {   (yyval.n) = EURASIA_USE1_IDF_PATH_ST;;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 2873 "tools/intern/useasm/use.y"
    {   (yyval.n) = EURASIA_USE1_IDF_PATH_PIXELBE;;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 2877 "tools/intern/useasm/use.y"
    {   (yyval.sMod).uFlags1 = 0;
    (yyval.sMod).uFlags2 = 0;
    (yyval.sMod).uFlags3 = 0;
    (yyval.sMod).uTest = 0;
    (yyval.sMod).bRepeatPresent = FALSE;
;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 2884 "tools/intern/useasm/use.y"
    {   (yyval.sMod).uFlags1 = (yyvsp[(1) - (2)].n) | (yyvsp[(2) - (2)].sMod).uFlags1;
    (yyval.sMod).uFlags2 = (yyvsp[(2) - (2)].sMod).uFlags2;
    (yyval.sMod).uFlags3 = (yyvsp[(2) - (2)].sMod).uFlags3;
    (yyval.sMod).uTest = (yyvsp[(2) - (2)].sMod).uTest;
;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 2890 "tools/intern/useasm/use.y"
    {   (yyval.sMod).uFlags1 = (yyvsp[(1) - (2)].n) | (yyvsp[(2) - (2)].sMod).uFlags1;
    (yyval.sMod).uFlags2 = (yyvsp[(2) - (2)].sMod).uFlags2;
    (yyval.sMod).uFlags3 = (yyvsp[(2) - (2)].sMod).uFlags3;
    (yyval.sMod).uTest = (yyvsp[(2) - (2)].sMod).uTest;
    (yyval.sMod).bRepeatPresent = TRUE;
;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 2897 "tools/intern/useasm/use.y"
    {   (yyval.sMod).uFlags1 = (yyvsp[(2) - (2)].sMod).uFlags1;
    (yyval.sMod).uFlags2 = (yyvsp[(1) - (2)].n) | (yyvsp[(2) - (2)].sMod).uFlags2;
    (yyval.sMod).uFlags3 = (yyvsp[(2) - (2)].sMod).uFlags3;
    (yyval.sMod).uTest = (yyvsp[(2) - (2)].sMod).uTest;
    (yyval.sMod).bRepeatPresent = (yyvsp[(2) - (2)].sMod).bRepeatPresent;
;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 2904 "tools/intern/useasm/use.y"
    {   (yyval.sMod).uFlags1 = (yyvsp[(2) - (2)].sMod).uFlags1;
    (yyval.sMod).uFlags2 = (yyvsp[(2) - (2)].sMod).uFlags2;
    (yyval.sMod).uFlags3 = (yyvsp[(1) - (2)].n) | (yyvsp[(2) - (2)].sMod).uFlags3;
    (yyval.sMod).uTest = (yyvsp[(2) - (2)].sMod).uTest;
    (yyval.sMod).bRepeatPresent = (yyvsp[(2) - (2)].sMod).bRepeatPresent;
;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 2911 "tools/intern/useasm/use.y"
    {   (yyval.sMod).uFlags1 = (yyvsp[(2) - (2)].sMod).uFlags1;
    (yyval.sMod).uFlags2 = USEASM_OPFLAGS2_C10 | (yyvsp[(2) - (2)].sMod).uFlags2;
    (yyval.sMod).uFlags3 = (yyvsp[(2) - (2)].sMod).uFlags3;
    (yyval.sMod).uTest = (yyvsp[(2) - (2)].sMod).uTest;
    (yyval.sMod).bRepeatPresent = (yyvsp[(2) - (2)].sMod).bRepeatPresent;
;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 2918 "tools/intern/useasm/use.y"
    {   (yyval.sMod).uFlags1 = USEASM_OPFLAGS1_MOVC_FMTI8 | (yyvsp[(2) - (2)].sMod).uFlags1;
    (yyval.sMod).uFlags2 = (yyvsp[(2) - (2)].sMod).uFlags2;
    (yyval.sMod).uFlags3 = (yyvsp[(2) - (2)].sMod).uFlags3;
    (yyval.sMod).uTest = (yyvsp[(2) - (2)].sMod).uTest;
    (yyval.sMod).bRepeatPresent = (yyvsp[(2) - (2)].sMod).bRepeatPresent;
;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 2925 "tools/intern/useasm/use.y"
    {   (yyval.sMod).uFlags1 = USEASM_OPFLAGS1_MOVC_FMTF16 | (yyvsp[(2) - (2)].sMod).uFlags1;
    (yyval.sMod).uFlags2 = (yyvsp[(2) - (2)].sMod).uFlags2;
    (yyval.sMod).uFlags3 = (yyvsp[(2) - (2)].sMod).uFlags3;
    (yyval.sMod).uTest = (yyvsp[(2) - (2)].sMod).uTest;
    (yyval.sMod).bRepeatPresent = (yyvsp[(2) - (2)].sMod).bRepeatPresent;
;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 2932 "tools/intern/useasm/use.y"
    {   (yyval.sMod).uFlags1 = (yyvsp[(2) - (2)].sMod).uFlags1 | USEASM_OPFLAGS1_TESTENABLE;
    (yyval.sMod).uFlags2 = (yyvsp[(2) - (2)].sMod).uFlags2;
    (yyval.sMod).uFlags3 = (yyvsp[(2) - (2)].sMod).uFlags3;
    (yyval.sMod).uTest = (yyvsp[(1) - (2)].n) | ((yyvsp[(2) - (2)].sMod).uTest & USEASM_TEST_COND_CLRMSK);
    (yyval.sMod).bRepeatPresent = (yyvsp[(2) - (2)].sMod).bRepeatPresent;
;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 2939 "tools/intern/useasm/use.y"
    {   (yyval.sMod).uFlags1 = (yyvsp[(2) - (2)].sMod).uFlags1 | USEASM_OPFLAGS1_TESTENABLE;
    (yyval.sMod).uFlags2 = (yyvsp[(2) - (2)].sMod).uFlags2;
    (yyval.sMod).uFlags3 = (yyvsp[(2) - (2)].sMod).uFlags3;
    (yyval.sMod).uTest = (yyvsp[(1) - (2)].n) | ((yyvsp[(2) - (2)].sMod).uTest & USEASM_TEST_CHANSEL_CLRMSK);
    (yyval.sMod).bRepeatPresent = (yyvsp[(2) - (2)].sMod).bRepeatPresent;
;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 2946 "tools/intern/useasm/use.y"
    {   (yyval.sMod).uFlags1 = (yyvsp[(2) - (2)].sMod).uFlags1 | USEASM_OPFLAGS1_ABS;
    (yyval.sMod).uFlags2 = (yyvsp[(2) - (2)].sMod).uFlags2;
    (yyval.sMod).uFlags3 = (yyvsp[(2) - (2)].sMod).uFlags3;
    (yyval.sMod).uTest = (yyvsp[(2) - (2)].sMod).uTest;
    (yyval.sMod).bRepeatPresent = (yyvsp[(2) - (2)].sMod).bRepeatPresent;
;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 2953 "tools/intern/useasm/use.y"
    {   (yyval.sMod).uFlags1 = (yyvsp[(2) - (2)].sMod).uFlags1 | USEASM_OPFLAGS1_TESTENABLE;
    (yyval.sMod).uFlags2 = (yyvsp[(2) - (2)].sMod).uFlags2;
    (yyval.sMod).uFlags3 = (yyvsp[(2) - (2)].sMod).uFlags3;
    (yyval.sMod).uTest = (yyvsp[(1) - (2)].n) | ((yyvsp[(2) - (2)].sMod).uTest & USEASM_TEST_MASK_CLRMSK);
    (yyval.sMod).bRepeatPresent = (yyvsp[(2) - (2)].sMod).bRepeatPresent;
;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 2963 "tools/intern/useasm/use.y"
    {   (yyval.sArgs).uCount = 1; (yyval.sArgs).asArg[0] = (yyvsp[(1) - (1)].sRegister);;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 2965 "tools/intern/useasm/use.y"
    {   (yyval.sArgs).uCount = (yyvsp[(3) - (3)].sArgs).uCount + 1;
    UseAsm_MemCopy((yyval.sArgs).asArg + 1, (yyvsp[(3) - (3)].sArgs).asArg, (yyvsp[(3) - (3)].sArgs).uCount * sizeof((yyvsp[(3) - (3)].sArgs).asArg[0])),
		(yyval.sArgs).asArg[0] = (yyvsp[(1) - (3)].sRegister);;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 2972 "tools/intern/useasm/use.y"
    {   (yyval.sRegister) = (yyvsp[(2) - (3)].sRegister); (yyval.sRegister).uFlags = (yyvsp[(1) - (3)].n) | (yyvsp[(3) - (3)].n);;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 2974 "tools/intern/useasm/use.y"
    {   (yyval.sRegister) = (yyvsp[(3) - (5)].sRegister); (yyval.sRegister).uFlags = (yyvsp[(5) - (5)].n) | USEASM_ARGFLAGS_ABSOLUTE;;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 2976 "tools/intern/useasm/use.y"
    {   InitialiseDefaultRegister(&(yyval.sRegister));;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 2981 "tools/intern/useasm/use.y"
    {   (yyval.n) = 0;;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 2983 "tools/intern/useasm/use.y"
    {   (yyval.n) = 0;;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 2985 "tools/intern/useasm/use.y"
    {   (yyval.n) = USEASM_ARGFLAGS_NEGATE;;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 2987 "tools/intern/useasm/use.y"
    {   (yyval.n) = USEASM_ARGFLAGS_INVERT;;}
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 2989 "tools/intern/useasm/use.y"
    {   (yyval.n) = USEASM_ARGFLAGS_NOT;;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 2991 "tools/intern/useasm/use.y"
    {   if ((yyvsp[(1) - (2)].n) != 1)
    {
        ParseError("Found literal number %d but was expecting register.\n", (yyvsp[(1) - (2)].n));
    }
    (yyval.n) = USEASM_ARGFLAGS_COMPLEMENT;
;}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 3001 "tools/intern/useasm/use.y"
    {   (yyval.n) = (yyvsp[(1) - (1)].n);;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 3003 "tools/intern/useasm/use.y"
    {   (yyval.n) = (yyvsp[(1) - (1)].n);;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 3008 "tools/intern/useasm/use.y"
    {
    (yyval.sRegister).uType = (yyvsp[(1) - (2)].n);
    (yyval.sRegister).uNumber = (yyvsp[(2) - (2)].n);
    (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 3014 "tools/intern/useasm/use.y"
    {
    (yyval.sRegister).uType = USEASM_REGTYPE_INDEX;
    (yyval.sRegister).uNumber = (yyvsp[(1) - (1)].n);
    (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 3020 "tools/intern/useasm/use.y"
    {
    (yyval.sRegister).uType = USEASM_REGTYPE_IMMEDIATE;
    (yyval.sRegister).uNumber = (yyvsp[(2) - (2)].n);
    (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 3026 "tools/intern/useasm/use.y"
    {
	ConvertFloatImmediateToArgument(&(yyval.sRegister), (yyvsp[(2) - (2)].f));
;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 3030 "tools/intern/useasm/use.y"
    {
	ConvertFloatImmediateToArgument(&(yyval.sRegister), -(yyvsp[(3) - (3)].f));
;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 3034 "tools/intern/useasm/use.y"
    {
    IMG_UINT32 uN = FindScope((yyvsp[(4) - (5)].sOp).pszCharString, BRANCH_SCOPE, (yyvsp[(4) - (5)].sOp).pszSourceFile, (yyvsp[(4) - (5)].sOp).uSourceLine);
    UseAsm_Free((yyvsp[(4) - (5)].sOp).pszCharString);
    (yyval.sRegister).uType = USEASM_REGTYPE_LABEL;
    (yyval.sRegister).uNumber = uN;
    (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 3042 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = (yyvsp[(1) - (4)].n);
    (yyval.sRegister).uNumber = 0;
    (yyval.sRegister).uIndex = USEREG_INDEX_L + (yyvsp[(3) - (4)].n);;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 3046 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = (yyvsp[(1) - (7)].n);
    (yyval.sRegister).uNumber = (yyvsp[(6) - (7)].n);
    (yyval.sRegister).uIndex = USEREG_INDEX_L + (yyvsp[(3) - (7)].n);;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 3050 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = (yyvsp[(1) - (6)].n);
    (yyval.sRegister).uNumber = (yyvsp[(5) - (6)].n);
    (yyval.sRegister).uIndex = USEREG_INDEX_L + (yyvsp[(3) - (6)].n);;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 3054 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = (yyvsp[(1) - (6)].n);
    (yyval.sRegister).uNumber = (yyvsp[(3) - (6)].n);
    (yyval.sRegister).uIndex = USEREG_INDEX_L + (yyvsp[(5) - (6)].n);;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 3058 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = (yyvsp[(1) - (7)].n);
    (yyval.sRegister).uNumber = (yyvsp[(2) - (7)].n) + (yyvsp[(6) - (7)].n);
    (yyval.sRegister).uIndex = USEREG_INDEX_L + (yyvsp[(4) - (7)].n);;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 3062 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = (yyvsp[(1) - (7)].n);
    (yyval.sRegister).uNumber = (yyvsp[(2) - (7)].n) + (yyvsp[(4) - (7)].n);
    (yyval.sRegister).uIndex = USEREG_INDEX_L + (yyvsp[(6) - (7)].n);;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 3066 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = (yyvsp[(1) - (5)].n); (yyval.sRegister).uNumber = (yyvsp[(4) - (5)].n); (yyval.sRegister).uIndex = USEREG_INDEX_NONE;;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 3068 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = (yyvsp[(1) - (4)].n); (yyval.sRegister).uNumber = (yyvsp[(3) - (4)].n); (yyval.sRegister).uIndex = USEREG_INDEX_NONE;;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 3070 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = (yyvsp[(1) - (5)].n); (yyval.sRegister).uNumber = (yyvsp[(2) - (5)].n); (yyval.sRegister).uIndex = USEREG_INDEX_L + (yyvsp[(4) - (5)].n);;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 3072 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = (yyvsp[(1) - (8)].n); (yyval.sRegister).uNumber = (yyvsp[(2) - (8)].n) + (yyvsp[(7) - (8)].n); (yyval.sRegister).uIndex = USEREG_INDEX_L + (yyvsp[(4) - (8)].n);;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 3074 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = USEASM_REGTYPE_PREDICATE; (yyval.sRegister).uNumber = 0; (yyval.sRegister).uIndex = USEREG_INDEX_NONE;;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 3076 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = USEASM_REGTYPE_PREDICATE; (yyval.sRegister).uNumber = 1; (yyval.sRegister).uIndex = USEREG_INDEX_NONE;;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 3078 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = USEASM_REGTYPE_PREDICATE; (yyval.sRegister).uNumber = 2; (yyval.sRegister).uIndex = USEREG_INDEX_NONE;;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 3080 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = USEASM_REGTYPE_PREDICATE; (yyval.sRegister).uNumber = 3; (yyval.sRegister).uIndex = USEREG_INDEX_NONE;;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 3082 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = USEASM_REGTYPE_LINK; (yyval.sRegister).uNumber = 0; (yyval.sRegister).uIndex = USEREG_INDEX_NONE;;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 3084 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = USEASM_REGTYPE_FPINTERNAL; (yyval.sRegister).uNumber = 0; (yyval.sRegister).uIndex = USEREG_INDEX_NONE;;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 3086 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = USEASM_REGTYPE_FPINTERNAL; (yyval.sRegister).uNumber = 1; (yyval.sRegister).uIndex = USEREG_INDEX_NONE;;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 3088 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = USEASM_REGTYPE_IMMEDIATE; (yyval.sRegister).uNumber = (yyvsp[(1) - (1)].n); (yyval.sRegister).uIndex = USEREG_INDEX_NONE;;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 3090 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = USEASM_REGTYPE_SWIZZLE; (yyval.sRegister).uNumber = (yyvsp[(1) - (1)].n); (yyval.sRegister).uIndex = USEREG_INDEX_NONE;;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 3092 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = USEASM_REGTYPE_ADDRESSMODE; (yyval.sRegister).uNumber = (yyvsp[(1) - (1)].n); (yyval.sRegister).uIndex = USEREG_INDEX_NONE;;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 3094 "tools/intern/useasm/use.y"
    {   (yyval.sRegister).uType = USEASM_REGTYPE_INTSRCSEL; (yyval.sRegister).uNumber = (yyvsp[(1) - (1)].n); (yyval.sRegister).uIndex = USEREG_INDEX_NONE;;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 3096 "tools/intern/useasm/use.y"
    {   if ((yyvsp[(1) - (1)].sOp).uOpcode == USEASM_OP_IADD32)
    {
        (yyval.sRegister).uType = USEASM_REGTYPE_INTSRCSEL;
        (yyval.sRegister).uNumber = USEASM_INTSRCSEL_ADD;
        (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
    }
    else if ((yyvsp[(1) - (1)].sOp).uOpcode == USEASM_OP_AND)
    {
        (yyval.sRegister).uType = USEASM_REGTYPE_INTSRCSEL;
        (yyval.sRegister).uNumber = USEASM_INTSRCSEL_AND;
        (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
    }
    else if ((yyvsp[(1) - (1)].sOp).uOpcode == USEASM_OP_OR)
    {
        (yyval.sRegister).uType = USEASM_REGTYPE_INTSRCSEL;
        (yyval.sRegister).uNumber = USEASM_INTSRCSEL_OR;
        (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
    }
    else if ((yyvsp[(1) - (1)].sOp).uOpcode == USEASM_OP_XOR)
    {
        (yyval.sRegister).uType = USEASM_REGTYPE_INTSRCSEL;
        (yyval.sRegister).uNumber = USEASM_INTSRCSEL_XOR;
        (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
    }
    else
    {
        ParseError("An opcode was unexpected at this point");
    }
;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 3126 "tools/intern/useasm/use.y"
    {
    (yyval.sRegister).uType = USEASM_REGTYPE_INTSRCSEL;
    (yyval.sRegister).uNumber = USEASM_INTSRCSEL_SRC0;
    (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 3132 "tools/intern/useasm/use.y"
    {
    (yyval.sRegister).uType = USEASM_REGTYPE_INTSRCSEL;
    (yyval.sRegister).uNumber = USEASM_INTSRCSEL_SRC1;
    (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 3138 "tools/intern/useasm/use.y"
    {
    (yyval.sRegister).uType = USEASM_REGTYPE_INTSRCSEL;
    (yyval.sRegister).uNumber = USEASM_INTSRCSEL_SRC2;
    (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 3144 "tools/intern/useasm/use.y"
    {
    IMG_UINT32 uNamedRegLink = 0;
    if(LookupNamedRegister((yyvsp[(1) - (1)].sOp).pszCharString, &uNamedRegLink) == IMG_TRUE)
    {
        (yyval.sRegister).uType = USEASM_REGTYPE_TEMP_NAMED;
        (yyval.sRegister).uNumber = IMG_UINT32_MAX;
        (yyval.sRegister).uNamedRegLink = uNamedRegLink;
        (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
    }
    else
    {
        ParseError("'%s' is not defined as named temporary register", (yyvsp[(1) - (1)].sOp).pszCharString);
        (yyval.sRegister).uType = USEASM_REGTYPE_TEMP;
        (yyval.sRegister).uNumber = 0;
        (yyval.sRegister).uNamedRegLink = 0;
        (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
    }
    UseAsm_Free((yyvsp[(1) - (1)].sOp).pszCharString);
;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 3164 "tools/intern/useasm/use.y"
    {
    IMG_UINT32 uNamedRegLink = 0;
    if(LookupNamedRegister((yyvsp[(1) - (5)].sOp).pszCharString, &uNamedRegLink) == IMG_TRUE)
    {
        if(IsArray(uNamedRegLink))
        {
            IMG_UINT32 uArrayElementsCount = GetArrayElementsCount(uNamedRegLink);
            if(uArrayElementsCount> (yyvsp[(4) - (5)].n))
            {
                (yyval.sRegister).uType = USEASM_REGTYPE_TEMP_NAMED;
                (yyval.sRegister).uNumber = (yyvsp[(4) - (5)].n);
                (yyval.sRegister).uNamedRegLink = uNamedRegLink;
                (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
            }
            else
            {
                ParseError("Can not index element '%u' on array '%s', of '%u' elements", (yyvsp[(4) - (5)].n),
						   GetNamedTempRegName(uNamedRegLink), uArrayElementsCount);
                (yyval.sRegister).uType = USEASM_REGTYPE_TEMP_NAMED;
                (yyval.sRegister).uNumber = uArrayElementsCount-1;
                (yyval.sRegister).uNamedRegLink = uNamedRegLink;
                (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
            }
        }
        else
        {
            ParseError("Can not use '%s' as any array at this point",
					   GetNamedTempRegName(uNamedRegLink));
            (yyval.sRegister).uType = USEASM_REGTYPE_TEMP_NAMED;
            (yyval.sRegister).uNumber = IMG_UINT32_MAX;
            (yyval.sRegister).uNamedRegLink = uNamedRegLink;
            (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
        }
    }
    else
    {
        ParseError("'%s' is not defined as named temporary register", (yyvsp[(1) - (5)].sOp).pszCharString);
        (yyval.sRegister).uType = USEASM_REGTYPE_TEMP; (yyval.sRegister).uNumber = 0; (yyval.sRegister).uNamedRegLink = 0;
        (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
    }
    UseAsm_Free((yyvsp[(1) - (5)].sOp).pszCharString);
;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 3207 "tools/intern/useasm/use.y"
    {
    IMG_UINT32 uNamedRegLink = 0;
    if(LookupNamedRegister((yyvsp[(1) - (4)].sOp).pszCharString, &uNamedRegLink) == IMG_TRUE)
    {
        if(IsArray(uNamedRegLink))
        {
            IMG_UINT32 uArrayElementsCount = GetArrayElementsCount(uNamedRegLink);
            if(uArrayElementsCount> (yyvsp[(3) - (4)].n))
            {
                (yyval.sRegister).uType = USEASM_REGTYPE_TEMP_NAMED;
                (yyval.sRegister).uNumber = (yyvsp[(3) - (4)].n);
                (yyval.sRegister).uNamedRegLink = uNamedRegLink;
                (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
            }
            else
            {
                ParseError("Can not index element '%u' on array '%s', of '%u' elements", (yyvsp[(3) - (4)].n),
						   GetNamedTempRegName(uNamedRegLink), uArrayElementsCount);
                (yyval.sRegister).uType = USEASM_REGTYPE_TEMP_NAMED;
                (yyval.sRegister).uNumber = uArrayElementsCount-1;
                (yyval.sRegister).uNamedRegLink = uNamedRegLink;
                (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
            }
        }
        else
        {
            ParseError("Can not use '%s' as any array at this point",
                       GetNamedTempRegName(uNamedRegLink));
            (yyval.sRegister).uType = USEASM_REGTYPE_TEMP_NAMED;
            (yyval.sRegister).uNumber = IMG_UINT32_MAX;
            (yyval.sRegister).uNamedRegLink = uNamedRegLink;
            (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
        }
    }
    else
    {
        ParseError("'%s' is not defined as named temporary register", (yyvsp[(1) - (4)].sOp).pszCharString);
        (yyval.sRegister).uType = USEASM_REGTYPE_TEMP; (yyval.sRegister).uNumber = 0; (yyval.sRegister).uNamedRegLink = 0;
        (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
    }
    UseAsm_Free((yyvsp[(1) - (4)].sOp).pszCharString);
;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 3250 "tools/intern/useasm/use.y"
    {
    IMG_PVOID pvScope;
    IMG_UINT32 uNamedRegLink;
    IMG_PCHAR pszScopeName = GetScopeOrLabelName((yyvsp[(1) - (2)].sOp).pszCharString);
    if(LookupParentScopes(pszScopeName, &pvScope))
    {
        const IMG_PCHAR pszTempRegName = (yyvsp[(2) - (2)].sOp).pszCharString;
        if(LookupTemporaryRegisterInScope(pvScope, pszTempRegName, &uNamedRegLink))
        {
            (yyval.sRegister).uType = USEASM_REGTYPE_TEMP_NAMED;
            (yyval.sRegister).uNumber = IMG_UINT32_MAX;
            (yyval.sRegister).uNamedRegLink = uNamedRegLink;
            (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
        }
        else
        {
            ParseError("No register with name '%s' is defined in scope '%s' at this point",
					   pszTempRegName, pszScopeName);
            (yyval.sRegister).uType = USEASM_REGTYPE_TEMP; (yyval.sRegister).uNumber = 0; (yyval.sRegister).uNamedRegLink = 0;
            (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
        }
    }
    else
    {
        if(LookupProcScope(pszScopeName, (yyvsp[(1) - (2)].sOp).pszSourceFile, (yyvsp[(1) - (2)].sOp).uSourceLine, &pvScope))
        {
            const IMG_PCHAR pszTempRegName = (yyvsp[(2) - (2)].sOp).pszCharString;
            if(LookupTemporaryRegisterInScope(pvScope, pszTempRegName, &uNamedRegLink))
            {
                (yyval.sRegister).uType = USEASM_REGTYPE_TEMP_NAMED;
                (yyval.sRegister).uNumber = IMG_UINT32_MAX;
                (yyval.sRegister).uNamedRegLink = uNamedRegLink;
                (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
            }
            else
            {
                ParseError("No register with name '%s' is defined in scope '%s' at this point",
						   pszTempRegName, pszScopeName);
                (yyval.sRegister).uType = USEASM_REGTYPE_TEMP; (yyval.sRegister).uNumber = 0; (yyval.sRegister).uNamedRegLink = 0;
                (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
            }
        }
        else
        {
            AddUndefinedNamedRegisterInScope(pvScope,
											 (yyvsp[(2) - (2)].sOp).pszCharString,
											 (yyvsp[(2) - (2)].sOp).pszSourceFile,
											 (yyvsp[(2) - (2)].sOp).uSourceLine,
											 &uNamedRegLink);
            (yyval.sRegister).uType = USEASM_REGTYPE_TEMP_NAMED;
            (yyval.sRegister).uNumber = IMG_UINT32_MAX;
            (yyval.sRegister).uNamedRegLink = uNamedRegLink;
            (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
        }
    }
    DeleteScopeOrLabelName(&pszScopeName);
    UseAsm_Free((yyvsp[(1) - (2)].sOp).pszCharString);
    UseAsm_Free((yyvsp[(2) - (2)].sOp).pszCharString);
;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 3310 "tools/intern/useasm/use.y"
    {
    IMG_PVOID pvScope;
    IMG_UINT32 uNamedRegLink;
    IMG_PCHAR pszScopeName = GetScopeOrLabelName((yyvsp[(1) - (6)].sOp).pszCharString);
    if(LookupParentScopes(pszScopeName, &pvScope))
    {
        IMG_UINT32 uNamedRegLink;
        if(LookupNamedRegister((yyvsp[(1) - (6)].sOp).pszCharString, &uNamedRegLink) == IMG_TRUE)
        {
            if(IsArray(uNamedRegLink))
            {
                IMG_UINT32 uArrayElementsCount = GetArrayElementsCount(uNamedRegLink);
                if(uArrayElementsCount> (yyvsp[(5) - (6)].n))
                {
                    (yyval.sRegister).uType = USEASM_REGTYPE_TEMP_NAMED;
                    (yyval.sRegister).uNumber = (yyvsp[(5) - (6)].n);
                    (yyval.sRegister).uNamedRegLink = uNamedRegLink;
                    (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
                }
                else
                {
                    ParseError("Can not index element '%u' on array '%s', of '%u' elements", (yyvsp[(5) - (6)].n),
							   GetNamedTempRegName(uNamedRegLink), uArrayElementsCount);
                    (yyval.sRegister).uType = USEASM_REGTYPE_TEMP_NAMED;
                    (yyval.sRegister).uNumber = uArrayElementsCount-1;
                    (yyval.sRegister).uNamedRegLink = uNamedRegLink;
                    (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
                }
            }
            else
            {
                ParseError("Can not use '%s' as any array at this point",
						   GetNamedTempRegName(uNamedRegLink));
                (yyval.sRegister).uType = USEASM_REGTYPE_TEMP_NAMED;
                (yyval.sRegister).uNumber = IMG_UINT32_MAX;
                (yyval.sRegister).uNamedRegLink = uNamedRegLink;
                (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
            }
        }
        else
        {
            ParseError("'%s' is not defined as named temporary registers array in scope '%s'",
					   (yyvsp[(2) - (6)].sOp).pszCharString, pszScopeName);
            (yyval.sRegister).uType = USEASM_REGTYPE_TEMP; (yyval.sRegister).uNumber = 0; (yyval.sRegister).uNamedRegLink = 0;
            (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
        }
    }
    else
    {
        if(LookupProcScope(pszScopeName, (yyvsp[(2) - (6)].sOp).pszSourceFile, (yyvsp[(2) - (6)].sOp).uSourceLine, &pvScope))
        {
            IMG_UINT32 uNamedRegLink;
            if(LookupNamedRegister((yyvsp[(1) - (6)].sOp).pszCharString, &uNamedRegLink) == IMG_TRUE)
            {
                if(IsArray(uNamedRegLink))
                {
                    IMG_UINT32 uArrayElementsCount = GetArrayElementsCount(uNamedRegLink);
                    if(uArrayElementsCount> (yyvsp[(5) - (6)].n))
                    {
                        (yyval.sRegister).uType = USEASM_REGTYPE_TEMP_NAMED;
                        (yyval.sRegister).uNumber = (yyvsp[(5) - (6)].n);
                        (yyval.sRegister).uNamedRegLink = uNamedRegLink;
                        (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
                    }
                    else
                    {
                        ParseError("Can not index element '%u' on array '%s', of '%u' elements",
								   (yyvsp[(5) - (6)].n),
								   GetNamedTempRegName(uNamedRegLink),
								   uArrayElementsCount);
                        (yyval.sRegister).uType = USEASM_REGTYPE_TEMP_NAMED;
                        (yyval.sRegister).uNumber = uArrayElementsCount-1;
                        (yyval.sRegister).uNamedRegLink = uNamedRegLink;
                        (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
                    }
                }
                else
                {
                    ParseError("Can not use '%s' as any array at this point",
							   GetNamedTempRegName(uNamedRegLink));
                    (yyval.sRegister).uType = USEASM_REGTYPE_TEMP_NAMED;
                    (yyval.sRegister).uNumber = IMG_UINT32_MAX;
                    (yyval.sRegister).uNamedRegLink = uNamedRegLink;
                    (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
                }
            }
            else
            {
                ParseError("'%s' is not defined as named temporary registers array in scope '%s'",
						   (yyvsp[(2) - (6)].sOp).pszCharString, pszScopeName);
                (yyval.sRegister).uType = USEASM_REGTYPE_TEMP; (yyval.sRegister).uNumber = 0; (yyval.sRegister).uNamedRegLink = 0;
                (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
            }
        }
        else
        {
            AddUndefinedNamedRegsArrayInScope(pvScope, (yyvsp[(2) - (6)].sOp).pszCharString, (yyvsp[(5) - (6)].n),
											  (yyvsp[(2) - (6)].sOp).pszSourceFile,
											  (yyvsp[(2) - (6)].sOp).uSourceLine,
											  &uNamedRegLink);
            (yyval.sRegister).uType = USEASM_REGTYPE_TEMP_NAMED;
            (yyval.sRegister).uNumber = (yyvsp[(5) - (6)].n);
            (yyval.sRegister).uNamedRegLink = uNamedRegLink;
            (yyval.sRegister).uIndex = USEREG_INDEX_NONE;
        }
    }
    DeleteScopeOrLabelName(&pszScopeName);
    UseAsm_Free((yyvsp[(1) - (6)].sOp).pszCharString);
    UseAsm_Free((yyvsp[(2) - (6)].sOp).pszCharString);
;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 3424 "tools/intern/useasm/use.y"
    {   (yyval.n) = (yyvsp[(1) - (1)].n);;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 3426 "tools/intern/useasm/use.y"
    {   (yyval.n) = (yyvsp[(1) - (3)].n) + (yyvsp[(3) - (3)].n);;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 3428 "tools/intern/useasm/use.y"
    {   (yyval.n) = (yyvsp[(1) - (3)].n) - (yyvsp[(3) - (3)].n);;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 3430 "tools/intern/useasm/use.y"
    {   (yyval.n) = (yyvsp[(1) - (3)].n) * (yyvsp[(3) - (3)].n);;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 3432 "tools/intern/useasm/use.y"
    {   (yyval.n) = (yyvsp[(1) - (3)].n) / (yyvsp[(3) - (3)].n);;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 3434 "tools/intern/useasm/use.y"
    {   (yyval.n) = (yyvsp[(1) - (3)].n) << (yyvsp[(3) - (3)].n);;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 3436 "tools/intern/useasm/use.y"
    {   (yyval.n) = (yyvsp[(1) - (3)].n) >> (yyvsp[(3) - (3)].n);;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 3438 "tools/intern/useasm/use.y"
    {   (yyval.n) = (yyvsp[(1) - (3)].n) % (yyvsp[(3) - (3)].n);;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 3440 "tools/intern/useasm/use.y"
    {   (yyval.n) = ~(yyvsp[(2) - (2)].n);;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 3442 "tools/intern/useasm/use.y"
    {   (yyval.n) = (yyvsp[(1) - (3)].n) & (yyvsp[(3) - (3)].n);;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 3444 "tools/intern/useasm/use.y"
    {   (yyval.n) = (yyvsp[(1) - (3)].n) | (yyvsp[(3) - (3)].n);;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 3446 "tools/intern/useasm/use.y"
    {   (yyval.n) = (yyvsp[(1) - (3)].n) ^ (yyvsp[(3) - (3)].n);;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 3448 "tools/intern/useasm/use.y"
    {   (yyval.n) = (yyvsp[(2) - (3)].n);;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 3450 "tools/intern/useasm/use.y"
    {   (yyval.n) = -(IMG_INT32)(yyvsp[(2) - (2)].n);;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 3455 "tools/intern/useasm/use.y"
    {   (yyval.n) = 0;;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 3457 "tools/intern/useasm/use.y"
    {   (yyval.n) = 1;;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 3462 "tools/intern/useasm/use.y"
    {   (yyval.n) = 1;;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 3464 "tools/intern/useasm/use.y"
    {   (yyval.n) = 2;;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 3466 "tools/intern/useasm/use.y"
    {   (yyval.n) = 3;;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 3471 "tools/intern/useasm/use.y"
    {   (yyval.n) = 0;;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 3473 "tools/intern/useasm/use.y"
    {   (yyval.n) = (yyvsp[(1) - (2)].n) | (yyvsp[(2) - (2)].n);;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 3475 "tools/intern/useasm/use.y"
    {   (yyval.n) = USEASM_ARGFLAGS_FMTC10 | (yyvsp[(2) - (2)].n);;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 3477 "tools/intern/useasm/use.y"
    {   (yyval.n) = USEASM_ARGFLAGS_FMTF16 | (yyvsp[(2) - (2)].n);;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 3479 "tools/intern/useasm/use.y"
    {   (yyval.n) = USEASM_ARGFLAGS_FMTU8 | (yyvsp[(2) - (2)].n);;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 3481 "tools/intern/useasm/use.y"
    {   (yyval.n) = (yyvsp[(2) - (2)].n) | USEASM_ARGFLAGS_ABSOLUTE;;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 3487 "tools/intern/useasm/use.y"
    {   (yyval.sLdStArg).uOpFlags1 = 0;
    (yyval.sLdStArg).uOpFlags2 = 0;
    (yyval.sLdStArg).uCount = 1;
    (yyval.sLdStArg).asArg[0] = (yyvsp[(2) - (3)].sRegister);
    (yyval.sLdStArg).asArg[0].uFlags = 0;;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 3494 "tools/intern/useasm/use.y"
    {   (yyval.sLdStArg).uOpFlags1 = (yyvsp[(4) - (5)].sLdStArg).uOpFlags1;
    (yyval.sLdStArg).uOpFlags2 = (yyvsp[(4) - (5)].sLdStArg).uOpFlags2;
    (yyval.sLdStArg).uCount = 2;
    (yyval.sLdStArg).asArg[0] = (yyvsp[(2) - (5)].sRegister);
    (yyval.sLdStArg).asArg[0].uFlags = 0;
    (yyval.sLdStArg).asArg[1] = (yyvsp[(4) - (5)].sLdStArg).asArg[0];
    (yyval.sLdStArg).asArg[1].uFlags = 0;;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 3503 "tools/intern/useasm/use.y"
    {   (yyval.sLdStArg).uOpFlags1 = USEASM_OPFLAGS1_RANGEENABLE | (yyvsp[(4) - (7)].sLdStArg).uOpFlags1;
    (yyval.sLdStArg).uOpFlags2 = (yyvsp[(4) - (7)].sLdStArg).uOpFlags2;
    (yyval.sLdStArg).uCount = 3;
    (yyval.sLdStArg).asArg[0] = (yyvsp[(2) - (7)].sRegister);
    (yyval.sLdStArg).asArg[0].uFlags = 0;
    (yyval.sLdStArg).asArg[1] = (yyvsp[(4) - (7)].sLdStArg).asArg[0];
    (yyval.sLdStArg).asArg[1].uFlags = 0;
    (yyval.sLdStArg).asArg[2] = (yyvsp[(6) - (7)].sRegister);
    (yyval.sLdStArg).asArg[2].uFlags = 0;;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 3517 "tools/intern/useasm/use.y"
    {   (yyval.sLdStArg).uOpFlags1 = 0;
    (yyval.sLdStArg).uOpFlags2 = 0;
    (yyval.sLdStArg).uCount = 3;
    (yyval.sLdStArg).asArg[0] = (yyvsp[(2) - (5)].sRegister);
    (yyval.sLdStArg).asArg[0].uFlags = 0;
    (yyval.sLdStArg).asArg[1] = (yyvsp[(4) - (5)].sRegister);
    (yyval.sLdStArg).asArg[1].uFlags = 0;
    (yyval.sLdStArg).asArg[2].uType = USEASM_REGTYPE_IMMEDIATE;
    (yyval.sLdStArg).asArg[2].uNumber = 0;
    (yyval.sLdStArg).asArg[2].uFlags = 0;
    (yyval.sLdStArg).asArg[2].uIndex = USEREG_INDEX_NONE;;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 3529 "tools/intern/useasm/use.y"
    {   (yyval.sLdStArg).uOpFlags1 = 0;
    (yyval.sLdStArg).uOpFlags2 = 0;
    (yyval.sLdStArg).uCount = 3;
    (yyval.sLdStArg).asArg[0] = (yyvsp[(2) - (7)].sRegister);
    (yyval.sLdStArg).asArg[0].uFlags = 0;
    (yyval.sLdStArg).asArg[1] = (yyvsp[(4) - (7)].sRegister);
    (yyval.sLdStArg).asArg[1].uFlags = 0;
    (yyval.sLdStArg).asArg[2] = (yyvsp[(6) - (7)].sRegister);
    (yyval.sLdStArg).asArg[2].uFlags = 0;;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 3542 "tools/intern/useasm/use.y"
    {   (yyval.sLdStArg).uOpFlags1 = 0;
    (yyval.sLdStArg).uOpFlags2 = 0;
    (yyval.sLdStArg).uCount = 1;
    (yyval.sLdStArg).asArg[0] = (yyvsp[(2) - (3)].sRegister);
    (yyval.sLdStArg).asArg[0].uFlags = 0;;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 3549 "tools/intern/useasm/use.y"
    {   (yyval.sLdStArg).uOpFlags1 = (yyvsp[(4) - (5)].sLdStArg).uOpFlags1;
    (yyval.sLdStArg).uOpFlags2 = (yyvsp[(4) - (5)].sLdStArg).uOpFlags2;
    (yyval.sLdStArg).uCount = 2;
    (yyval.sLdStArg).asArg[0] = (yyvsp[(2) - (5)].sRegister);
    (yyval.sLdStArg).asArg[0].uFlags = 0;
    (yyval.sLdStArg).asArg[1] = (yyvsp[(4) - (5)].sLdStArg).asArg[0];
    (yyval.sLdStArg).asArg[1].uFlags = 0;;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 3560 "tools/intern/useasm/use.y"
    {   (yyval.sLdStArg).asArg[0] = (yyvsp[(1) - (1)].sRegister);
    (yyval.sLdStArg).uOpFlags1 = 0;
    (yyval.sLdStArg).uOpFlags2 = 0;;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 3564 "tools/intern/useasm/use.y"
    {   (yyval.sLdStArg).asArg[0] = (yyvsp[(2) - (2)].sRegister);
    (yyval.sLdStArg).uOpFlags1 = 0;
    (yyval.sLdStArg).uOpFlags2 = 0;
    if ((yyvsp[(1) - (2)].n) == PLUSPLUS || (yyvsp[(1) - (2)].n) == MINUSMINUS) (yyval.sLdStArg).uOpFlags1 |= USEASM_OPFLAGS1_PREINCREMENT;
    if ((yyvsp[(1) - (2)].n) == MINUS || (yyvsp[(1) - (2)].n) == MINUSMINUS) (yyval.sLdStArg).uOpFlags2 |= USEASM_OPFLAGS2_INCSGN;;}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 3570 "tools/intern/useasm/use.y"
    {   (yyval.sLdStArg).asArg[0] = (yyvsp[(1) - (2)].sRegister);
    (yyval.sLdStArg).uOpFlags1 = 0;
    (yyval.sLdStArg).uOpFlags2 = 0;
    if ((yyvsp[(2) - (2)].n) == PLUSPLUS || (yyvsp[(2) - (2)].n) == MINUSMINUS) (yyval.sLdStArg).uOpFlags1 |= USEASM_OPFLAGS1_POSTINCREMENT;
    if ((yyvsp[(2) - (2)].n) == MINUS || (yyvsp[(2) - (2)].n) == MINUSMINUS) (yyval.sLdStArg).uOpFlags2 |= USEASM_OPFLAGS2_INCSGN;;}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 3579 "tools/intern/useasm/use.y"
    {   (yyval.n) = PLUSPLUS;;}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 3581 "tools/intern/useasm/use.y"
    {   (yyval.n) = MINUSMINUS;;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 3583 "tools/intern/useasm/use.y"
    {   (yyval.n) = MINUS;;}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 3588 "tools/intern/useasm/use.y"
    {   (yyval.n) = ((yyvsp[(1) - (13)].n) ? 0 : EURASIA_USE1_EFO_WI0EN) |
		((yyvsp[(6) - (13)].n) ? 0 : EURASIA_USE1_EFO_WI1EN) |
		(yyvsp[(13) - (13)].n) |
		(yyvsp[(11) - (13)].n) |
		(EURASIA_USE1_EFO_ISRC_I0A0_I1A1 << EURASIA_USE1_EFO_ISRC_SHIFT);;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 3594 "tools/intern/useasm/use.y"
    {   (yyval.n) = ((yyvsp[(1) - (13)].n) ? 0 : EURASIA_USE1_EFO_WI0EN) |
		((yyvsp[(6) - (13)].n) ? 0 : EURASIA_USE1_EFO_WI1EN) |
		(yyvsp[(13) - (13)].n) |
		(yyvsp[(11) - (13)].n) |
		(EURASIA_USE1_EFO_ISRC_I0A1_I1A0 << EURASIA_USE1_EFO_ISRC_SHIFT);;}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 3600 "tools/intern/useasm/use.y"
    {   (yyval.n) = ((yyvsp[(1) - (13)].n) ? 0 : EURASIA_USE1_EFO_WI0EN) |
		((yyvsp[(6) - (13)].n) ? 0 : EURASIA_USE1_EFO_WI1EN) |
		(yyvsp[(13) - (13)].n) |
		(yyvsp[(11) - (13)].n) |
		(EURASIA_USE1_EFO_ISRC_I0M0_I1M1 << EURASIA_USE1_EFO_ISRC_SHIFT);;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 3606 "tools/intern/useasm/use.y"
    {   (yyval.n) = ((yyvsp[(1) - (13)].n) ? 0 : EURASIA_USE1_EFO_WI0EN) |
		((yyvsp[(6) - (13)].n) ? 0 : EURASIA_USE1_EFO_WI1EN) |
		(yyvsp[(13) - (13)].n) |
		(yyvsp[(11) - (13)].n) |
		(EURASIA_USE1_EFO_ISRC_I0A0_I1M1 << EURASIA_USE1_EFO_ISRC_SHIFT);;}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 3612 "tools/intern/useasm/use.y"
    {   (yyval.n) = ((yyvsp[(1) - (8)].n) ? 0 : EURASIA_USE1_EFO_WI1EN) |
		(yyvsp[(6) - (8)].n) |
		(yyvsp[(8) - (8)].n) |
		(EURASIA_USE1_EFO_ISRC_I0A0_I1A1 << EURASIA_USE1_EFO_ISRC_SHIFT);;}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 3617 "tools/intern/useasm/use.y"
    {   (yyval.n) = ((yyvsp[(1) - (8)].n) ? 0 : EURASIA_USE1_EFO_WI1EN) |
		(yyvsp[(6) - (8)].n) |
		(yyvsp[(8) - (8)].n) |
		(EURASIA_USE1_EFO_ISRC_I0A1_I1A0 << EURASIA_USE1_EFO_ISRC_SHIFT);;}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 3622 "tools/intern/useasm/use.y"
    {   (yyval.n) = ((yyvsp[(1) - (8)].n) ? 0 : EURASIA_USE1_EFO_WI1EN) |
		(yyvsp[(6) - (8)].n) |
		(yyvsp[(8) - (8)].n) |
		(EURASIA_USE1_EFO_ISRC_I0M0_I1M1 << EURASIA_USE1_EFO_ISRC_SHIFT);;}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 3627 "tools/intern/useasm/use.y"
    {   (yyval.n) = ((yyvsp[(1) - (8)].n) ? 0 : EURASIA_USE1_EFO_WI0EN) |
		(yyvsp[(6) - (8)].n) |
		(yyvsp[(8) - (8)].n) |
		(EURASIA_USE1_EFO_ISRC_I0A0_I1A1 << EURASIA_USE1_EFO_ISRC_SHIFT);;}
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 3632 "tools/intern/useasm/use.y"
    {   (yyval.n) = ((yyvsp[(1) - (8)].n) ? 0 : EURASIA_USE1_EFO_WI0EN) |
		(yyvsp[(6) - (8)].n) |
		(yyvsp[(8) - (8)].n) |
		(EURASIA_USE1_EFO_ISRC_I0A1_I1A0 << EURASIA_USE1_EFO_ISRC_SHIFT);;}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 3637 "tools/intern/useasm/use.y"
    {   (yyval.n) = ((yyvsp[(1) - (8)].n) ? 0 : EURASIA_USE1_EFO_WI0EN) |
		(yyvsp[(6) - (8)].n) |
		(yyvsp[(8) - (8)].n) |
		(EURASIA_USE1_EFO_ISRC_I0M0_I1M1 << EURASIA_USE1_EFO_ISRC_SHIFT);;}
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 3646 "tools/intern/useasm/use.y"
    {
    (yyval.n) = ParseEfoAddExpr((yyvsp[(3) - (13)].n), (yyvsp[(5) - (13)].n), (yyvsp[(6) - (13)].n), (yyvsp[(10) - (13)].n), (yyvsp[(11) - (13)].n), (yyvsp[(13) - (13)].n));
;}
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 3653 "tools/intern/useasm/use.y"
    {   (yyval.n) = SRC0;;}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 3655 "tools/intern/useasm/use.y"
    {   (yyval.n) = SRC1;;}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 3657 "tools/intern/useasm/use.y"
    {   (yyval.n) = SRC2;;}
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 3659 "tools/intern/useasm/use.y"
    {   (yyval.n) = I0;;}
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 3661 "tools/intern/useasm/use.y"
    {   (yyval.n) = I1;;}
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 3663 "tools/intern/useasm/use.y"
    {   (yyval.n) = M0;;}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 3665 "tools/intern/useasm/use.y"
    {   (yyval.n) = M1;;}
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 3670 "tools/intern/useasm/use.y"
    {
    (yyval.n) = ParseEfoMulExpr((yyvsp[(3) - (11)].n), (yyvsp[(5) - (11)].n), (yyvsp[(9) - (11)].n), (yyvsp[(11) - (11)].n));
;}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 3677 "tools/intern/useasm/use.y"
    {   (yyval.n) = SRC0;;}
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 3679 "tools/intern/useasm/use.y"
    {   (yyval.n) = SRC1;;}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 3681 "tools/intern/useasm/use.y"
    {   (yyval.n) = SRC2;;}
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 3683 "tools/intern/useasm/use.y"
    {   (yyval.n) = I0;;}
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 3685 "tools/intern/useasm/use.y"
    {   (yyval.n) = I1;;}
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 3690 "tools/intern/useasm/use.y"
    {   (yyval.n) = EURASIA_USE1_EFO_DSRC_I0;;}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 3692 "tools/intern/useasm/use.y"
    {   (yyval.n) = EURASIA_USE1_EFO_DSRC_I1;;}
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 3694 "tools/intern/useasm/use.y"
    {   (yyval.n) = EURASIA_USE1_EFO_DSRC_A0;;}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 3696 "tools/intern/useasm/use.y"
    {   (yyval.n) = EURASIA_USE1_EFO_DSRC_A1;;}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 3701 "tools/intern/useasm/use.y"
    {   (yyval.n) = 0;;}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 3703 "tools/intern/useasm/use.y"
    {   (yyval.n) = 1;;}
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 3708 "tools/intern/useasm/use.y"
    {   (yyval.n) = 0;;}
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 3710 "tools/intern/useasm/use.y"
    {   (yyval.n) = 1;;}
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 3715 "tools/intern/useasm/use.y"
    {   (yyval.n) = 0;;}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 3717 "tools/intern/useasm/use.y"
    {   (yyval.n) = 1;;}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 3728 "tools/intern/useasm/use.y"
    {;}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 3732 "tools/intern/useasm/use.y"
    {   AddNamedRegister((yyvsp[(1) - (1)].sOp).pszCharString, (yyvsp[(1) - (1)].sOp).pszSourceFile, (yyvsp[(1) - (1)].sOp).uSourceLine); UseAsm_Free((yyvsp[(1) - (1)].sOp).pszCharString);;}
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 3734 "tools/intern/useasm/use.y"
    {
    if((yyvsp[(4) - (5)].n)> 0)
    {
        AddNamedRegistersArray((yyvsp[(1) - (5)].sOp).pszCharString, (yyvsp[(4) - (5)].n), (yyvsp[(1) - (5)].sOp).pszSourceFile, (yyvsp[(1) - (5)].sOp).uSourceLine);
    }
    else
    {
        ParseError("Can not declare any array with %d number of elements", (yyvsp[(4) - (5)].n));
    }
    UseAsm_Free((yyvsp[(1) - (5)].sOp).pszCharString);
;}
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 3746 "tools/intern/useasm/use.y"
    {
    if((yyvsp[(3) - (4)].n)> 0)
    {
        AddNamedRegistersArray((yyvsp[(1) - (4)].sOp).pszCharString, (yyvsp[(3) - (4)].n), (yyvsp[(1) - (4)].sOp).pszSourceFile, (yyvsp[(1) - (4)].sOp).uSourceLine);
    }
    else
    {
        ParseError("Can not declare any array with %d number of elements", (yyvsp[(3) - (4)].n));
    }
    UseAsm_Free((yyvsp[(1) - (4)].sOp).pszCharString);
;}
    break;



/* Line 1455 of yacc.c  */
#line 6637 "eurasiacon/binary2_540_omap4430_android_release/host/intermediates/useparser/use.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 3758 "tools/intern/useasm/use.y"


