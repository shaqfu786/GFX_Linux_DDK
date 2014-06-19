%{
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
%}

%union
{
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
}

%type <psInst> program
%type <psRegList> opt_register_list
%type <sInst> instruction
%type <n> predicate maybe_mask arg_modifier prearg_mod offset_op idf_path efo_mul_src efo_add_src
%type <sMod> opcode_modifier
%type <sRegister> argument arg_register
%type <sArgs> source_arguments
%type <sLdStArg> ld_argument st_argument ldst_offset_argument eld_argument
%type <n> efo_expression efo_addr_expr efo_mul_expr efo_dest_select maybe_bang maybe_negate maybe_negate_r dest_index_register src_index_register expr
%type <n> repeat register_type
%type <sMod> preopcode_flag
%type <sOp> forcealign
%type <sRegister> label
%type <sRegister> temp_reg_name
%type <sRegister> procedure

%token <n> REGISTER OPCODE_FLAG1 OPCODE_FLAG2 OPCODE_FLAG3 ARGUMENT_FLAG MASK NUMBER TEST_TYPE TEST_CHANSEL TEMP_REGISTER REPEAT_FLAG TEST_MASK C10_FLAG F16_FLAG U8_FLAG
%token <f> FLOAT_NUMBER
%token <sOp> OPCODE COISSUE_OPCODE LD_OPCODE ST_OPCODE ELD_OPCODE BRANCH_OPCODE EFO_OPCODE LAPC_OPCODE WDF_OPCODE EXT_OPCODE 
%token <sOp> LOCKRELEASE_OPCODE IDF_OPCODE FIR_OPCODE ASOP2_OPCODE ONEARG_OPCODE PTOFF_OPCODE CALL_OPCODE NOP_OPCODE
%token <sOp> EMITVTX_OPCODE CFI_OPCODE
%token INPUT OUTPUT
%token OPEN_SQBRACKET CLOSE_SQBRACKET
%token PLUS MINUS TIMES DIVIDE LSHIFT RSHIFT HASH BANG AND OR PLUSPLUS MINUSMINUS EQUALS NOT XOR OPEN_BRACKET CLOSE_BRACKET
%token INSTRUCTION_DELIMITER COMMA
%token PRED0 PRED1 PRED2 PRED3 PREDN
%token SCHEDON SCHEDOFF SKIPINVON SKIPINVOFF REPEATOFF
%token <sOp> FORCE_ALIGN MISALIGN IMPORT EXPORT MODULEALIGN
%token <n> I0 I1 A0 A1 M0 M1 SRC0 SRC1 SRC2 DIRECT_IMMEDIATE ADDRESS_MODE SWIZZLE DATAFORMAT INTSRCSEL
%token ABS ABS_NODOT UNEXPECTED_CHARACTER INDEXLOW INDEXHIGH INDEXBOTH PCLINK LABEL_ADDRESS IDF_ST IDF_PIXELBE NOSCHED_FLAG SKIPINV_FLAG
%token <n> TARGET_SPECIFIER
%token TARGET
%token TEMP_REG_DEF
%token <sOp> OPEN_CURLY_BRACKET
%token CLOSE_CURLY_BRACKET
%token PROC
%token <sOp> SCOPE_NAME
%token <sOp> IDENTIFIER
%token NAMED_REGS_RANGE
%token RENAME_REG
%token COLON COLON_PLUS_DELIMITER

%left OR
%left AND
%left XOR
%left RSHIFT
%left LSHIFT
%left MINUS
%left PLUS
%left DIVIDE
%left TIMES
%left NOT
%left MODULUS

%start program
%%

program:
/* Nothing */
{   $$ = NULL;}
| program instruction INSTRUCTION_DELIMITER
{
	$$ = ProcessInstruction($1, &$2);
}
| program pseudo_instruction INSTRUCTION_DELIMITER
{   $$ = $1;}
| program INPUT opt_register_list
{   OptProcInputRegs($3); $$ = $1;}
| program OUTPUT opt_register_list
{   OptProcOutputRegs($3); $$ = $1;}
| program INSTRUCTION_DELIMITER
{   $$ = $1;}
| program forcealign INSTRUCTION_DELIMITER
{   $$ = ForceAlign($2.uOpcode, $2.uOp2, $2.pszSourceFile, $2.uSourceLine, &$1);}
| program IDENTIFIER COLON_PLUS_DELIMITER
{	USE_INST	sLabelDesc;
	UseAsmInitInst(&sLabelDesc);
	ProcessLabel(&sLabelDesc, &$2);
	$$ = ProcessInstruction($1, &sLabelDesc); }
| program CLOSE_CURLY_BRACKET
{
    if(IsScopeManagementEnabled() == IMG_TRUE)
    {
        CloseScope();
    }
    $$ = $1;
}
| program OPEN_CURLY_BRACKET
{
    const USE_INST* psLastInst = GetLastInstruction();
    USE_INST sNewInst;
    
    UseAsmInitInst(&sNewInst);
    sNewInst.uOpcode = USEASM_OP_LABEL;
    sNewInst.uSourceLine = $2.uSourceLine;
    sNewInst.pszSourceFile = $2.pszSourceFile;
    sNewInst.asArg[0].uType = USEASM_REGTYPE_LABEL;
    sNewInst.asArg[0].uIndex = USEREG_INDEX_NONE;
    sNewInst.asArg[0].uFlags = 0;
    if(psLastInst != NULL && (psLastInst->uOpcode == USEASM_OP_LABEL) &&
	   (IsCurrentScopeLinkedToModifiableLabel(psLastInst->asArg[0].uNumber) == IMG_FALSE))
    {
        sNewInst.asArg[0].uNumber = ReOpenLastScope($2.pszSourceFile, $2.uSourceLine);
    }
    else
    {
        sNewInst.asArg[0].uNumber = OpenScope(NULL, IMG_FALSE, IMG_FALSE,
										$2.pszSourceFile, $2.uSourceLine);
    }
    if(IsScopeManagementEnabled() == IMG_FALSE)
    {
        CloseScope();
    }
    CheckOpcode(&sNewInst, 1, FALSE);
    $$ = ProcessInstruction($1, &sNewInst);
}
;

/*
 opt_register_list: List of input/output registers. Used by the optimiser.

 opt_register_list ::= (<register_type> <register_number>) [, <opt_register_list>]
 */
opt_register_list:
/* empty */
{   $$ = NULL;}
| opt_register_list register_type NUMBER
{
    USEASM_PARSER_REG_LIST *psList = $1;
    USEASM_REGTYPE uType = $2;
    USEASM_REGTYPE uNumber = $3;
    USEASM_PARSER_REG_LIST *psHd = NULL;

    psHd = (USEASM_PARSER_REG_LIST*)UseAsm_Malloc(sizeof(USEASM_PARSER_REG_LIST));
    UseAsm_MemSet(psHd, 0, sizeof(*psHd));

    psHd->psNext = psList;
    psHd->uType = uType;
    psHd->uNumber = uNumber;

    $$ = psHd;
}

pseudo_instruction:
SCHEDOFF
{
    if (g_uSchedOffCount == 0)
    {
        g_uFirstNoSchedInstOffset = g_uInstOffset;
        g_psFirstNoSchedInst = g_psLastNoSchedInst = NULL;
        g_bStartedNoSchedRegion = TRUE;
    }
    g_uSchedOffCount++;
}
| SCHEDON
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
}
| SKIPINVON
{   g_uSkipInvOnCount++;}
| SKIPINVOFF
{   if (g_uSkipInvOnCount == 0)
    {
        yyerror(".skipinvoff without preceding .skipinvon");
    }
    else
    {
        g_uSkipInvOnCount--;
    }
}
| REPEAT_FLAG
{   if (g_uRepeatStackTop >= REPEAT_STACK_LENGTH)
    {
        yyerror("Too many nested .rptN instructions");
    }
    else
    {
        g_puRepeatStack[g_uRepeatStackTop] = $1 >> USEASM_OPFLAGS1_REPEAT_SHIFT;
        g_uRepeatStackTop++;
    }
}
| REPEATOFF
{   if (g_uRepeatStackTop == 0)
    {
        yyerror(".rptend without preceding .rptN instruction");
    }
    else
    {
        g_uRepeatStackTop--;
    }
}
| TARGET TARGET_SPECIFIER
{   SetTarget((SGX_CORE_ID_TYPE)$2, 0);}
| TARGET TARGET_SPECIFIER COMMA NUMBER
{   SetTarget((SGX_CORE_ID_TYPE)$2, $4);}
| EXPORT IDENTIFIER
{
    FindScope($2.pszCharString, EXPORT_SCOPE, $2.pszSourceFile, $2.uSourceLine);
    UseAsm_Free($2.pszCharString);
}
| IMPORT IDENTIFIER
{
    OpenScope($2.pszCharString, IMG_FALSE, IMG_TRUE, $2.pszSourceFile, $2.uSourceLine);
    CloseScope();
    UseAsm_Free($2.pszCharString);
}
| MODULEALIGN NUMBER
{
    g_uModuleAlignment = gcd($2, g_uModuleAlignment);
}
| TEMP_REG_DEF temp_reg_names
{}
| NAMED_REGS_RANGE NUMBER COMMA NUMBER
{
    IMG_UINT32 uRangeStart = $2;
    IMG_UINT32 uRangeEnd = $4;
    if(uRangeStart> uRangeEnd)
    {
        ParseError("Range start '%u' can not be larger than range end '%u'",
                uRangeStart, uRangeEnd);
    }
    else
    {

        SetHwRegsAllocRangeFromFile(uRangeStart, uRangeEnd);
    }
}
| RENAME_REG IDENTIFIER COMMA IDENTIFIER
{
    RenameNamedRegister($2.pszCharString, $2.pszSourceFile,
            $2.uSourceLine, $4.pszCharString,
            $4.pszSourceFile, $4.uSourceLine);
    UseAsm_Free($2.pszCharString);
    UseAsm_Free($4.pszCharString);
}
;

forcealign:
FORCE_ALIGN NUMBER
{
    $$.uSourceLine = $1.uSourceLine;
    $$.pszSourceFile = $1.pszSourceFile;
    $$.uOpcode = $2;
    $$.uOp2 = 0;
}
| FORCE_ALIGN NUMBER COMMA NUMBER
{   $$.uSourceLine = $1.uSourceLine;
    $$.pszSourceFile = $1.pszSourceFile;
    $$.uOpcode = $2;
    $$.uOp2 = $4;}
| MISALIGN
{   $$.uSourceLine = $1.uSourceLine;
    $$.pszSourceFile = $1.pszSourceFile;
    $$.uOpcode = 2;
    $$.uOp2 = 1;}
;

instruction:
preopcode_flag OPCODE opcode_modifier argument maybe_mask COMMA source_arguments
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uFlags2 = $3.uFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.uTest = $3.uTest;
    $$.uFlags1 |= ($5 << USEASM_OPFLAGS1_MASK_SHIFT);
    $$.uFlags1 |= $1.uFlags1;
    $$.asArg[0] = $4;
    UseAsm_MemCopy($$.asArg + 1, $7.asArg, $7.uCount * sizeof($$.asArg[0]));
    CheckOpcode(&$$, $7.uCount + 1, $1.bRepeatPresent || $3.bRepeatPresent);
    if (($$.uOpcode >= USEASM_OP_PCKF16F32 && $$.uOpcode <= USEASM_OP_UNPCKC10C10) ||
            ($$.uOpcode == USEASM_OP_SOP2WM))
    {
        if (!($$.asArg[0].uFlags & USEASM_ARGFLAGS_BYTEMSK_PRESENT))
        {
            $$.asArg[0].uFlags |= (0xF << USEASM_ARGFLAGS_BYTEMSK_SHIFT);
        }
    }}
| PLUS OPCODE opcode_modifier argument maybe_mask COMMA source_arguments
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uFlags2 = $3.uFlags2 | USEASM_OPFLAGS2_COISSUE;
    $$.uFlags3 = $3.uFlags3;
    $$.uTest = $3.uTest;
    $$.uFlags1 |= ($5 << USEASM_OPFLAGS1_MASK_SHIFT);
    $$.asArg[0] = $4;
    UseAsm_MemCopy($$.asArg + 1, $7.asArg, $7.uCount * sizeof($$.asArg[0]));
    CheckOpcode(&$$, $7.uCount + 1, $3.bRepeatPresent);
    if (($$.uOpcode >= USEASM_OP_PCKF16F32 && $$.uOpcode <= USEASM_OP_UNPCKC10C10) ||
            ($$.uOpcode == USEASM_OP_SOP2WM))
    {
        if (!($$.asArg[0].uFlags & USEASM_ARGFLAGS_BYTEMSK_PRESENT))
        {
            $$.asArg[0].uFlags |= (0xF << USEASM_ARGFLAGS_BYTEMSK_SHIFT);
        }
    }}
| preopcode_flag LD_OPCODE opcode_modifier argument COMMA ld_argument COMMA source_arguments
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1 | $6.uOpFlags1;
    $$.uFlags1 |= $1.uFlags1;
    $$.uFlags2 = $3.uFlags2 | $6.uOpFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.uTest = $3.uTest;
    $$.asArg[0] = $4;
    UseAsm_MemCopy($$.asArg + 1, $6.asArg, $6.uCount * sizeof($$.asArg[0]));
    if ($6.uCount == 1)
    {
		InitialiseDefaultRegister(&$$.asArg[2]);
		$$.asArg[2].uType = USEASM_REGTYPE_IMMEDIATE;
		$$.asArg[2].uNumber = 0;
    }
    UseAsm_MemCopy($$.asArg + 3, $8.asArg, $8.uCount * sizeof($$.asArg[0]));
    CheckOpcode(&$$, 3 + $8.uCount, $1.bRepeatPresent || $3.bRepeatPresent);}
| preopcode_flag ST_OPCODE opcode_modifier st_argument COMMA source_arguments
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1 | $4.uOpFlags1;
    $$.uFlags1 |= $1.uFlags1;
    $$.uFlags2 = $3.uFlags2 | $4.uOpFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.uTest = $3.uTest;
    UseAsm_MemCopy($$.asArg, $4.asArg, $4.uCount * sizeof($$.asArg[0]));
    if ($4.uCount == 1)
    {
		InitialiseDefaultRegister(&$$.asArg[1]);
		$$.asArg[1].uType = USEASM_REGTYPE_IMMEDIATE;
		$$.asArg[1].uNumber = 0;
    }
    UseAsm_MemCopy($$.asArg + 2, $6.asArg, $6.uCount * sizeof($$.asArg[0]));
    CheckOpcode(&$$, 2 + $6.uCount, $1.bRepeatPresent || $3.bRepeatPresent);}
| preopcode_flag ELD_OPCODE opcode_modifier argument COMMA eld_argument COMMA argument COMMA argument
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uFlags1 |= $1.uFlags1;
    $$.uFlags2 = $3.uFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.uTest = $3.uTest;
    $$.asArg[0] = $4;
    UseAsm_MemCopy($$.asArg + 1, $6.asArg, $6.uCount * sizeof($$.asArg[0]));
    $$.asArg[$6.uCount + 1] = $8;
    $$.asArg[$6.uCount + 2] = $10;
    CheckOpcode(&$$, $6.uCount + 3, $1.bRepeatPresent || $3.bRepeatPresent);}
| preopcode_flag BRANCH_OPCODE opcode_modifier label
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uFlags1 |= $1.uFlags1;
    $$.uFlags2 = $3.uFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.uTest = $3.uTest;
    $$.asArg[0] = $4;
    CheckOpcode(&$$, 1, $1.bRepeatPresent || $3.bRepeatPresent);}
| preopcode_flag CALL_OPCODE opcode_modifier procedure
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1 | USEASM_OPFLAGS1_SAVELINK;
    $$.uFlags1 |= $1.uFlags1;
    $$.uFlags2 = $3.uFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.uTest = $3.uTest;
    $$.asArg[0] = $4;
    CheckOpcode(&$$, 1, $1.bRepeatPresent || $3.bRepeatPresent);}
| preopcode_flag EFO_OPCODE opcode_modifier argument EQUALS
efo_dest_select COMMA efo_expression COMMA source_arguments
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uFlags1 |= $1.uFlags1;
    $$.uFlags2 = $3.uFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.uTest = $3.uTest;
    $$.asArg[0] = $4;
    UseAsm_MemCopy($$.asArg + 1, $10.asArg, $10.uCount * sizeof($$.asArg[0]));
    $$.asArg[$10.uCount + 1].uNumber = $8 | ($6 << EURASIA_USE1_EFO_DSRC_SHIFT);
    CheckOpcode(&$$, $10.uCount + 2, $1.bRepeatPresent || $3.bRepeatPresent);}
| IDENTIFIER COLON
{ ProcessLabel(&$$, &$1); }
| PROC IDENTIFIER
{
    if(IsCurrentScopeGlobal() == IMG_TRUE)
    {
        $2.uOpcode = OpenScope($2.pszCharString,
							   IMG_TRUE, IMG_FALSE,
							   $2.pszSourceFile, $2.uSourceLine);
    }
    else
    {
        ParseErrorAt($2.pszSourceFile, $2.uSourceLine,
					 "Can not create procedure at nested level");
        $2.uOpcode = OpenScope($2.pszCharString,
							   IMG_FALSE, IMG_FALSE,
							   $2.pszSourceFile, $2.uSourceLine);
    }
    UseAsm_Free($2.pszCharString);
    $2.pszCharString = NULL;
}
OPEN_BRACKET temp_reg_names_optional CLOSE_BRACKET
{
    $$.uOpcode = USEASM_OP_LABEL;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.asArg[0].uType = USEASM_REGTYPE_LABEL;
    $$.asArg[0].uNumber = $2.uOpcode;
    $$.asArg[0].uIndex = USEREG_INDEX_NONE;
    $$.asArg[0].uFlags = 0;
    CheckOpcode(&$$, 1, FALSE);
    CloseScope();
}
| preopcode_flag LAPC_OPCODE opcode_modifier
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uFlags2 = $3.uFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.uTest = $3.uTest;
    $$.uFlags1 |= $1.uFlags1;
    CheckOpcode(&$$, 0, $1.bRepeatPresent);}
| preopcode_flag NOP_OPCODE opcode_modifier
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uFlags2 = $3.uFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.uTest = $3.uTest;
    $$.uFlags1 |= $1.uFlags1;
    CheckOpcode(&$$, 0, TRUE);}
| preopcode_flag LOCKRELEASE_OPCODE opcode_modifier
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uFlags2 = $3.uFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.uTest = 0;
    $$.uFlags1 |= $1.uFlags1;
    CheckOpcode(&$$, 0, $1.bRepeatPresent || $3.bRepeatPresent);}
| preopcode_flag LOCKRELEASE_OPCODE opcode_modifier arg_register
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uFlags2 = $3.uFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.uTest = 0;
    $$.uFlags1 |= $1.uFlags1;
    $$.asArg[0] = $4;
    $$.asArg[0].uFlags = 0;
    CheckOpcode(&$$, 1, $1.bRepeatPresent || $3.bRepeatPresent);}
| PTOFF_OPCODE opcode_modifier
{   $$.uOpcode = $1.uOpcode;
    $$.uSourceLine = $1.uSourceLine;
    $$.pszSourceFile = $1.pszSourceFile;
    $$.uFlags1 = $2.uFlags1;
    $$.uFlags2 = $2.uFlags2;
    $$.uFlags3 = $2.uFlags3;
    $$.uTest = 0;
    CheckOpcode(&$$, 0, FALSE);}
| preopcode_flag WDF_OPCODE opcode_modifier arg_register
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uFlags2 = $3.uFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.uTest = 0;
    $$.uFlags1 |= $1.uFlags1;
    $$.asArg[0] = $4;
    $$.asArg[0].uFlags = 0;
    CheckOpcode(&$$, 1, $1.bRepeatPresent || $3.bRepeatPresent);}
| preopcode_flag CFI_OPCODE opcode_modifier arg_register COMMA arg_register COMMA arg_register
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uFlags2 = $3.uFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.uTest = 0;
    $$.uFlags1 |= $1.uFlags1;
    $$.asArg[0] = $4;
    $$.asArg[0].uFlags = 0;
    $$.asArg[1] = $6;
    $$.asArg[1].uFlags = 0;
    $$.asArg[2] = $8;
    $$.asArg[2].uFlags = 0;
    CheckOpcode(&$$, 3, $1.bRepeatPresent || $3.bRepeatPresent);
}
| preopcode_flag ONEARG_OPCODE opcode_modifier arg_register
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uFlags2 = $3.uFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.uTest = 0;
    $$.uFlags1 |= $1.uFlags1;
    $$.asArg[0] = $4;
    $$.asArg[0].uFlags = 0;
    CheckOpcode(&$$, 1, $1.bRepeatPresent || $3.bRepeatPresent);}
| preopcode_flag IDF_OPCODE opcode_modifier arg_register COMMA idf_path
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uFlags2 = $3.uFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.uTest = 0;
    $$.uFlags1 |= $1.uFlags1;
    $$.asArg[0] = $4;
    $$.asArg[0].uFlags = 0;
    $$.asArg[1].uType = USEASM_REGTYPE_IMMEDIATE;
    $$.asArg[1].uNumber = $6;
    $$.asArg[1].uFlags = 0;
    $$.asArg[1].uIndex = USEREG_INDEX_NONE;
    CheckOpcode(&$$, 2, $1.bRepeatPresent || $3.bRepeatPresent);}
| preopcode_flag EXT_OPCODE opcode_modifier argument EQUALS efo_dest_select
COMMA argument COMMA argument COMMA argument
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uTest = 0;
    $$.uFlags1 |= $1.uFlags1;
    $$.uFlags2 = $3.uFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.asArg[0] = $4;
    $$.asArg[1] = $8;
    $$.asArg[2] = $10;
    $$.asArg[3] = $12;
    $$.asArg[4].uNumber = $6 << EURASIA_USE1_EFO_DSRC_SHIFT;
    CheckOpcode(&$$, 4, $1.bRepeatPresent || $3.bRepeatPresent);}
| preopcode_flag EXT_OPCODE opcode_modifier argument
COMMA argument COMMA argument COMMA argument
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uTest = 0;
    $$.uFlags1 |= $1.uFlags1;
    $$.uFlags2 = $3.uFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.asArg[0] = $4;
    $$.asArg[1] = $6;
    $$.asArg[2] = $8;
    $$.asArg[3] = $10;
    switch ($2.uOpcode)
    {
        case USEASM_OP_AMM:
        case USEASM_OP_SMM:
        case USEASM_OP_AMS:
        case USEASM_OP_SMS:
			$$.asArg[4].uNumber = (EURASIA_USE1_EFO_DSRC_A1 << EURASIA_USE1_EFO_DSRC_SHIFT);
			break;
        case USEASM_OP_DMA:
			$$.asArg[4].uNumber = (EURASIA_USE1_EFO_DSRC_A0 << EURASIA_USE1_EFO_DSRC_SHIFT);
			break;
        default: $$.asArg[4].uNumber = 0;
    }
    CheckOpcode(&$$, 4, $1.bRepeatPresent || $3.bRepeatPresent);}
| PLUS COISSUE_OPCODE opcode_modifier source_arguments
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uTest = 0;
    $$.uFlags2 = $3.uFlags2 | USEASM_OPFLAGS2_COISSUE;
    $$.uFlags3 = $3.uFlags3;
    UseAsm_MemCopy($$.asArg, $4.asArg, $4.uCount * sizeof($$.asArg[0]));
    CheckOpcode(&$$, $4.uCount, $3.bRepeatPresent);
}
| PLUS ASOP2_OPCODE opcode_modifier source_arguments
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uTest = 0;
    $$.uFlags2 = $3.uFlags2 | USEASM_OPFLAGS2_COISSUE;
    $$.uFlags3 = $3.uFlags3;
    UseAsm_MemCopy($$.asArg, $4.asArg, $4.uCount * sizeof($$.asArg[0]));
    CheckOpcode(&$$, $4.uCount, $3.bRepeatPresent);
}
| PLUS ASOP2_OPCODE opcode_modifier COMMA source_arguments
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uTest = 0;
    $$.uFlags2 = $3.uFlags2 | USEASM_OPFLAGS2_COISSUE;
    $$.uFlags3 = $3.uFlags3;
    $$.asArg[0].uType = USEASM_REGTYPE_INTSRCSEL;
    $$.asArg[0].uNumber = USEASM_INTSRCSEL_NONE;
    $$.asArg[0].uFlags = 0;
    $$.asArg[0].uIndex = USEREG_INDEX_NONE;
    UseAsm_MemCopy($$.asArg + 1, $5.asArg, $5.uCount * sizeof($$.asArg[0]));
    CheckOpcode(&$$, $5.uCount + 1, $3.bRepeatPresent);
}
| preopcode_flag FIR_OPCODE opcode_modifier argument maybe_mask COMMA source_arguments
{   $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uFlags2 = $3.uFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.uTest = $3.uTest;
    $$.uFlags1 |= ($5 << USEASM_OPFLAGS1_MASK_SHIFT);
    $$.uFlags1 |= $1.uFlags1;
    $$.asArg[0] = $4;
    if (($$.asArg[0].uFlags & USEASM_ARGFLAGS_NOT) != 0) 
    {
		$$.asArg[0].uFlags &= ~USEASM_ARGFLAGS_NOT;
		$$.asArg[0].uFlags |= USEASM_ARGFLAGS_DISABLEWB;
	}
    UseAsm_MemCopy($$.asArg + 1, $7.asArg, $7.uCount * sizeof($$.asArg[0]));
    CheckOpcode(&$$, $7.uCount + 1, $1.bRepeatPresent || $3.bRepeatPresent);}
| preopcode_flag FIR_OPCODE opcode_modifier COMMA source_arguments
{
    $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uFlags2 = $3.uFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.uTest = $3.uTest;
    $$.uFlags1 |= $1.uFlags1;
    $$.asArg[0].uType = USEASM_REGTYPE_FPINTERNAL;
    $$.asArg[0].uNumber = 0;
    $$.asArg[0].uFlags = USEASM_ARGFLAGS_DISABLEWB;
    $$.asArg[0].uIndex = USEREG_INDEX_NONE;
    UseAsm_MemCopy($$.asArg + 1, $5.asArg, $5.uCount * sizeof($$.asArg[0]));
    CheckOpcode(&$$, $5.uCount + 1, $1.bRepeatPresent || $3.bRepeatPresent);
}
| preopcode_flag EMITVTX_OPCODE opcode_modifier source_arguments
{
    $$.uOpcode = $2.uOpcode;
    $$.uSourceLine = $2.uSourceLine;
    $$.pszSourceFile = $2.pszSourceFile;
    $$.uFlags1 = $3.uFlags1;
    $$.uFlags2 = $3.uFlags2;
    $$.uFlags3 = $3.uFlags3;
    $$.uTest = $3.uTest;
    $$.uFlags1 |= $1.uFlags1;
    UseAsm_MemCopy($$.asArg, $4.asArg, $4.uCount * sizeof($$.asArg[0]));
    CheckOpcode(&$$, $4.uCount, $1.bRepeatPresent || $3.bRepeatPresent);
}
;

label:
IDENTIFIER
{
    IMG_UINT32 uN = FindScope($1.pszCharString,
							  BRANCH_SCOPE,
							  $1.pszSourceFile,
							  $1.uSourceLine);
    UseAsm_Free($1.pszCharString);
    $$.uType = USEASM_REGTYPE_LABEL; $$.uNumber = uN;
}
| IDENTIFIER PLUS expr
{   IMG_UINT32 uN = FindScope($1.pszCharString,
							  BRANCH_SCOPE,
							  $1.pszSourceFile,
							  $1.uSourceLine);
    UseAsm_Free($1.pszCharString);
    $$.uType = USEASM_REGTYPE_LABEL_WITH_OFFSET;
    $$.uNumber = uN;
    $$.uFlags = $3;
}
| IDENTIFIER MINUS expr
{
    IMG_UINT32 uN = FindScope($1.pszCharString,
							  BRANCH_SCOPE, $1.pszSourceFile,
							  $1.uSourceLine);
    UseAsm_Free($1.pszCharString);
    $$.uType = USEASM_REGTYPE_LABEL_WITH_OFFSET;
    $$.uNumber = uN;
    $$.uFlags = -(IMG_INT32)$3;
}
;

procedure:
IDENTIFIER
{
    IMG_UINT32 uN = FindScope($1.pszCharString,
							  PROCEDURE_SCOPE,
							  $1.pszSourceFile,
							  $1.uSourceLine);
    UseAsm_Free($1.pszCharString);
    $$.uType = USEASM_REGTYPE_LABEL; $$.uNumber = uN;
}
;

maybe_mask:
/* Nothing */
{   $$ = USEREG_MASK_X;}
| MASK
{   $$ = $1;}
;

preopcode_flag:
/* Nothing */
{   $$.uFlags1 = 0; $$.bRepeatPresent = FALSE;}
| predicate preopcode_flag
{   $$.uFlags1 = ($1 << USEASM_OPFLAGS1_PRED_SHIFT) | $2.uFlags1;
    $$.bRepeatPresent = $2.bRepeatPresent;
}
| repeat preopcode_flag
{   $$.uFlags1 = $1 | $2.uFlags1; $$.bRepeatPresent = TRUE;}
| SKIPINV_FLAG preopcode_flag
{   $$.uFlags1 = USEASM_OPFLAGS1_SKIPINVALID | $2.uFlags1; $$.bRepeatPresent = TRUE;}
| NOSCHED_FLAG preopcode_flag
{   $$.uFlags1 = USEASM_OPFLAGS1_NOSCHED | $2.uFlags1; $$.bRepeatPresent = TRUE;}
| OPEN_BRACKET preopcode_flag CLOSE_BRACKET
{   $$.uFlags1 = $2.uFlags1;
    $$.bRepeatPresent = $2.bRepeatPresent;}
| OPEN_BRACKET preopcode_flag COMMA preopcode_flag CLOSE_BRACKET
{   $$.uFlags1 = $2.uFlags1 | $4.uFlags1;
    $$.bRepeatPresent = $2.bRepeatPresent || $4.bRepeatPresent;
}
;

predicate:
PRED0
{   $$ = USEASM_PRED_P0;}
| PRED1
{   $$ = USEASM_PRED_P1;}
| BANG PRED0
{   $$ = USEASM_PRED_NEGP0;}
| BANG PRED1
{   $$ = USEASM_PRED_NEGP1;}
| BANG PRED2
{   $$ = USEASM_PRED_NEGP2;}
| PRED2
{   $$ = USEASM_PRED_P2;}
| PRED3
{   $$ = USEASM_PRED_P3;}
| PREDN
{   $$ = USEASM_PRED_PN;}
/* Error cases. */
| BANG PRED3
{   yyerror("Only P0 and P1 can be negated"); $$ = 0;}
| BANG PREDN
{   yyerror("Only P0 and P1 can be negated"); $$ = 0;}
;

repeat: TEMP_REGISTER NUMBER
{   $$ = ($2 << USEASM_OPFLAGS1_REPEAT_SHIFT);}
;

idf_path:
IDF_ST
{   $$ = EURASIA_USE1_IDF_PATH_ST;}
| IDF_PIXELBE
{   $$ = EURASIA_USE1_IDF_PATH_PIXELBE;}

opcode_modifier:
/* Nothing */
{   $$.uFlags1 = 0;
    $$.uFlags2 = 0;
    $$.uFlags3 = 0;
    $$.uTest = 0;
    $$.bRepeatPresent = FALSE;
}
| OPCODE_FLAG1 opcode_modifier
{   $$.uFlags1 = $1 | $2.uFlags1;
    $$.uFlags2 = $2.uFlags2;
    $$.uFlags3 = $2.uFlags3;
    $$.uTest = $2.uTest;
}
| REPEAT_FLAG opcode_modifier
{   $$.uFlags1 = $1 | $2.uFlags1;
    $$.uFlags2 = $2.uFlags2;
    $$.uFlags3 = $2.uFlags3;
    $$.uTest = $2.uTest;
    $$.bRepeatPresent = TRUE;
}
| OPCODE_FLAG2 opcode_modifier
{   $$.uFlags1 = $2.uFlags1;
    $$.uFlags2 = $1 | $2.uFlags2;
    $$.uFlags3 = $2.uFlags3;
    $$.uTest = $2.uTest;
    $$.bRepeatPresent = $2.bRepeatPresent;
}
| OPCODE_FLAG3 opcode_modifier
{   $$.uFlags1 = $2.uFlags1;
    $$.uFlags2 = $2.uFlags2;
    $$.uFlags3 = $1 | $2.uFlags3;
    $$.uTest = $2.uTest;
    $$.bRepeatPresent = $2.bRepeatPresent;
}
| C10_FLAG opcode_modifier
{   $$.uFlags1 = $2.uFlags1;
    $$.uFlags2 = USEASM_OPFLAGS2_C10 | $2.uFlags2;
    $$.uFlags3 = $2.uFlags3;
    $$.uTest = $2.uTest;
    $$.bRepeatPresent = $2.bRepeatPresent;
}
| U8_FLAG opcode_modifier
{   $$.uFlags1 = USEASM_OPFLAGS1_MOVC_FMTI8 | $2.uFlags1;
    $$.uFlags2 = $2.uFlags2;
    $$.uFlags3 = $2.uFlags3;
    $$.uTest = $2.uTest;
    $$.bRepeatPresent = $2.bRepeatPresent;
}
| F16_FLAG opcode_modifier
{   $$.uFlags1 = USEASM_OPFLAGS1_MOVC_FMTF16 | $2.uFlags1;
    $$.uFlags2 = $2.uFlags2;
    $$.uFlags3 = $2.uFlags3;
    $$.uTest = $2.uTest;
    $$.bRepeatPresent = $2.bRepeatPresent;
}
| TEST_TYPE opcode_modifier
{   $$.uFlags1 = $2.uFlags1 | USEASM_OPFLAGS1_TESTENABLE;
    $$.uFlags2 = $2.uFlags2;
    $$.uFlags3 = $2.uFlags3;
    $$.uTest = $1 | ($2.uTest & USEASM_TEST_COND_CLRMSK);
    $$.bRepeatPresent = $2.bRepeatPresent;
}
| TEST_CHANSEL opcode_modifier
{   $$.uFlags1 = $2.uFlags1 | USEASM_OPFLAGS1_TESTENABLE;
    $$.uFlags2 = $2.uFlags2;
    $$.uFlags3 = $2.uFlags3;
    $$.uTest = $1 | ($2.uTest & USEASM_TEST_CHANSEL_CLRMSK);
    $$.bRepeatPresent = $2.bRepeatPresent;
}
| ABS opcode_modifier
{   $$.uFlags1 = $2.uFlags1 | USEASM_OPFLAGS1_ABS;
    $$.uFlags2 = $2.uFlags2;
    $$.uFlags3 = $2.uFlags3;
    $$.uTest = $2.uTest;
    $$.bRepeatPresent = $2.bRepeatPresent;
}
| TEST_MASK opcode_modifier
{   $$.uFlags1 = $2.uFlags1 | USEASM_OPFLAGS1_TESTENABLE;
    $$.uFlags2 = $2.uFlags2;
    $$.uFlags3 = $2.uFlags3;
    $$.uTest = $1 | ($2.uTest & USEASM_TEST_MASK_CLRMSK);
    $$.bRepeatPresent = $2.bRepeatPresent;
}
;

source_arguments:
argument
{   $$.uCount = 1; $$.asArg[0] = $1;}
| argument COMMA source_arguments
{   $$.uCount = $3.uCount + 1;
    UseAsm_MemCopy($$.asArg + 1, $3.asArg, $3.uCount * sizeof($3.asArg[0])),
		$$.asArg[0] = $1;}
;

argument:
prearg_mod arg_register arg_modifier
{   $$ = $2; $$.uFlags = $1 | $3;}
| ABS_NODOT OPEN_BRACKET arg_register CLOSE_BRACKET arg_modifier
{   $$ = $3; $$.uFlags = $5 | USEASM_ARGFLAGS_ABSOLUTE;}
| error
{   InitialiseDefaultRegister(&$$);}
;

prearg_mod:
/* Nothing */
{   $$ = 0;}
| PLUS
{   $$ = 0;}
| MINUS
{   $$ = USEASM_ARGFLAGS_NEGATE;}
| NOT
{   $$ = USEASM_ARGFLAGS_INVERT;}
| BANG
{   $$ = USEASM_ARGFLAGS_NOT;}
| NUMBER MINUS
{   if ($1 != 1)
    {
        ParseError("Found literal number %d but was expecting register.\n", $1);
    }
    $$ = USEASM_ARGFLAGS_COMPLEMENT;
}
;

register_type:
REGISTER
{   $$ = $1;}
| TEMP_REGISTER
{   $$ = $1;}
;

arg_register:
register_type NUMBER
{
    $$.uType = $1;
    $$.uNumber = $2;
    $$.uIndex = USEREG_INDEX_NONE;
}
| dest_index_register
{
    $$.uType = USEASM_REGTYPE_INDEX;
    $$.uNumber = $1;
    $$.uIndex = USEREG_INDEX_NONE;
}
| HASH expr
{
    $$.uType = USEASM_REGTYPE_IMMEDIATE;
    $$.uNumber = $2;
    $$.uIndex = USEREG_INDEX_NONE;
}
| HASH FLOAT_NUMBER
{
	ConvertFloatImmediateToArgument(&$$, $2);
}
| HASH MINUS FLOAT_NUMBER
{
	ConvertFloatImmediateToArgument(&$$, -$3);
}
| HASH LABEL_ADDRESS OPEN_BRACKET IDENTIFIER CLOSE_BRACKET
{
    IMG_UINT32 uN = FindScope($4.pszCharString, BRANCH_SCOPE, $4.pszSourceFile, $4.uSourceLine);
    UseAsm_Free($4.pszCharString);
    $$.uType = USEASM_REGTYPE_LABEL;
    $$.uNumber = uN;
    $$.uIndex = USEREG_INDEX_NONE;
}
| register_type OPEN_SQBRACKET src_index_register CLOSE_SQBRACKET
{   $$.uType = $1;
    $$.uNumber = 0;
    $$.uIndex = USEREG_INDEX_L + $3;}
| register_type OPEN_SQBRACKET src_index_register PLUS HASH NUMBER CLOSE_SQBRACKET
{   $$.uType = $1;
    $$.uNumber = $6;
    $$.uIndex = USEREG_INDEX_L + $3;}
| register_type OPEN_SQBRACKET src_index_register PLUS expr CLOSE_SQBRACKET
{   $$.uType = $1;
    $$.uNumber = $5;
    $$.uIndex = USEREG_INDEX_L + $3;}
| register_type OPEN_SQBRACKET expr PLUS src_index_register CLOSE_SQBRACKET
{   $$.uType = $1;
    $$.uNumber = $3;
    $$.uIndex = USEREG_INDEX_L + $5;}
| register_type NUMBER OPEN_SQBRACKET src_index_register PLUS expr CLOSE_SQBRACKET
{   $$.uType = $1;
    $$.uNumber = $2 + $6;
    $$.uIndex = USEREG_INDEX_L + $4;}
| register_type NUMBER OPEN_SQBRACKET expr PLUS src_index_register CLOSE_SQBRACKET
{   $$.uType = $1;
    $$.uNumber = $2 + $4;
    $$.uIndex = USEREG_INDEX_L + $6;}
| register_type OPEN_SQBRACKET HASH NUMBER CLOSE_SQBRACKET
{   $$.uType = $1; $$.uNumber = $4; $$.uIndex = USEREG_INDEX_NONE;}
| register_type OPEN_SQBRACKET expr CLOSE_SQBRACKET
{   $$.uType = $1; $$.uNumber = $3; $$.uIndex = USEREG_INDEX_NONE;}
| register_type NUMBER OPEN_SQBRACKET src_index_register CLOSE_SQBRACKET
{   $$.uType = $1; $$.uNumber = $2; $$.uIndex = USEREG_INDEX_L + $4;}
| register_type NUMBER OPEN_SQBRACKET src_index_register PLUS HASH NUMBER CLOSE_SQBRACKET
{   $$.uType = $1; $$.uNumber = $2 + $7; $$.uIndex = USEREG_INDEX_L + $4;}
| PRED0
{   $$.uType = USEASM_REGTYPE_PREDICATE; $$.uNumber = 0; $$.uIndex = USEREG_INDEX_NONE;}
| PRED1
{   $$.uType = USEASM_REGTYPE_PREDICATE; $$.uNumber = 1; $$.uIndex = USEREG_INDEX_NONE;}
| PRED2
{   $$.uType = USEASM_REGTYPE_PREDICATE; $$.uNumber = 2; $$.uIndex = USEREG_INDEX_NONE;}
| PRED3
{   $$.uType = USEASM_REGTYPE_PREDICATE; $$.uNumber = 3; $$.uIndex = USEREG_INDEX_NONE;}
| PCLINK
{   $$.uType = USEASM_REGTYPE_LINK; $$.uNumber = 0; $$.uIndex = USEREG_INDEX_NONE;}
| I0
{   $$.uType = USEASM_REGTYPE_FPINTERNAL; $$.uNumber = 0; $$.uIndex = USEREG_INDEX_NONE;}
| I1
{   $$.uType = USEASM_REGTYPE_FPINTERNAL; $$.uNumber = 1; $$.uIndex = USEREG_INDEX_NONE;}
| DIRECT_IMMEDIATE
{   $$.uType = USEASM_REGTYPE_IMMEDIATE; $$.uNumber = $1; $$.uIndex = USEREG_INDEX_NONE;}
| SWIZZLE
{   $$.uType = USEASM_REGTYPE_SWIZZLE; $$.uNumber = $1; $$.uIndex = USEREG_INDEX_NONE;}
| ADDRESS_MODE
{   $$.uType = USEASM_REGTYPE_ADDRESSMODE; $$.uNumber = $1; $$.uIndex = USEREG_INDEX_NONE;}
| INTSRCSEL
{   $$.uType = USEASM_REGTYPE_INTSRCSEL; $$.uNumber = $1; $$.uIndex = USEREG_INDEX_NONE;}
| OPCODE
{   if ($1.uOpcode == USEASM_OP_IADD32)
    {
        $$.uType = USEASM_REGTYPE_INTSRCSEL;
        $$.uNumber = USEASM_INTSRCSEL_ADD;
        $$.uIndex = USEREG_INDEX_NONE;
    }
    else if ($1.uOpcode == USEASM_OP_AND)
    {
        $$.uType = USEASM_REGTYPE_INTSRCSEL;
        $$.uNumber = USEASM_INTSRCSEL_AND;
        $$.uIndex = USEREG_INDEX_NONE;
    }
    else if ($1.uOpcode == USEASM_OP_OR)
    {
        $$.uType = USEASM_REGTYPE_INTSRCSEL;
        $$.uNumber = USEASM_INTSRCSEL_OR;
        $$.uIndex = USEREG_INDEX_NONE;
    }
    else if ($1.uOpcode == USEASM_OP_XOR)
    {
        $$.uType = USEASM_REGTYPE_INTSRCSEL;
        $$.uNumber = USEASM_INTSRCSEL_XOR;
        $$.uIndex = USEREG_INDEX_NONE;
    }
    else
    {
        ParseError("An opcode was unexpected at this point");
    }
}
| SRC0
{
    $$.uType = USEASM_REGTYPE_INTSRCSEL;
    $$.uNumber = USEASM_INTSRCSEL_SRC0;
    $$.uIndex = USEREG_INDEX_NONE;
}
| SRC1
{
    $$.uType = USEASM_REGTYPE_INTSRCSEL;
    $$.uNumber = USEASM_INTSRCSEL_SRC1;
    $$.uIndex = USEREG_INDEX_NONE;
}
| SRC2
{
    $$.uType = USEASM_REGTYPE_INTSRCSEL;
    $$.uNumber = USEASM_INTSRCSEL_SRC2;
    $$.uIndex = USEREG_INDEX_NONE;
}
| IDENTIFIER
{
    IMG_UINT32 uNamedRegLink = 0;
    if(LookupNamedRegister($1.pszCharString, &uNamedRegLink) == IMG_TRUE)
    {
        $$.uType = USEASM_REGTYPE_TEMP_NAMED;
        $$.uNumber = IMG_UINT32_MAX;
        $$.uNamedRegLink = uNamedRegLink;
        $$.uIndex = USEREG_INDEX_NONE;
    }
    else
    {
        ParseError("'%s' is not defined as named temporary register", $1.pszCharString);
        $$.uType = USEASM_REGTYPE_TEMP;
        $$.uNumber = 0;
        $$.uNamedRegLink = 0;
        $$.uIndex = USEREG_INDEX_NONE;
    }
    UseAsm_Free($1.pszCharString);
}
| IDENTIFIER OPEN_SQBRACKET HASH NUMBER CLOSE_SQBRACKET
{
    IMG_UINT32 uNamedRegLink = 0;
    if(LookupNamedRegister($1.pszCharString, &uNamedRegLink) == IMG_TRUE)
    {
        if(IsArray(uNamedRegLink))
        {
            IMG_UINT32 uArrayElementsCount = GetArrayElementsCount(uNamedRegLink);
            if(uArrayElementsCount> $4)
            {
                $$.uType = USEASM_REGTYPE_TEMP_NAMED;
                $$.uNumber = $4;
                $$.uNamedRegLink = uNamedRegLink;
                $$.uIndex = USEREG_INDEX_NONE;
            }
            else
            {
                ParseError("Can not index element '%u' on array '%s', of '%u' elements", $4,
						   GetNamedTempRegName(uNamedRegLink), uArrayElementsCount);
                $$.uType = USEASM_REGTYPE_TEMP_NAMED;
                $$.uNumber = uArrayElementsCount-1;
                $$.uNamedRegLink = uNamedRegLink;
                $$.uIndex = USEREG_INDEX_NONE;
            }
        }
        else
        {
            ParseError("Can not use '%s' as any array at this point",
					   GetNamedTempRegName(uNamedRegLink));
            $$.uType = USEASM_REGTYPE_TEMP_NAMED;
            $$.uNumber = IMG_UINT32_MAX;
            $$.uNamedRegLink = uNamedRegLink;
            $$.uIndex = USEREG_INDEX_NONE;
        }
    }
    else
    {
        ParseError("'%s' is not defined as named temporary register", $1.pszCharString);
        $$.uType = USEASM_REGTYPE_TEMP; $$.uNumber = 0; $$.uNamedRegLink = 0;
        $$.uIndex = USEREG_INDEX_NONE;
    }
    UseAsm_Free($1.pszCharString);
}
| IDENTIFIER OPEN_SQBRACKET expr CLOSE_SQBRACKET
{
    IMG_UINT32 uNamedRegLink = 0;
    if(LookupNamedRegister($1.pszCharString, &uNamedRegLink) == IMG_TRUE)
    {
        if(IsArray(uNamedRegLink))
        {
            IMG_UINT32 uArrayElementsCount = GetArrayElementsCount(uNamedRegLink);
            if(uArrayElementsCount> $3)
            {
                $$.uType = USEASM_REGTYPE_TEMP_NAMED;
                $$.uNumber = $3;
                $$.uNamedRegLink = uNamedRegLink;
                $$.uIndex = USEREG_INDEX_NONE;
            }
            else
            {
                ParseError("Can not index element '%u' on array '%s', of '%u' elements", $3,
						   GetNamedTempRegName(uNamedRegLink), uArrayElementsCount);
                $$.uType = USEASM_REGTYPE_TEMP_NAMED;
                $$.uNumber = uArrayElementsCount-1;
                $$.uNamedRegLink = uNamedRegLink;
                $$.uIndex = USEREG_INDEX_NONE;
            }
        }
        else
        {
            ParseError("Can not use '%s' as any array at this point",
                       GetNamedTempRegName(uNamedRegLink));
            $$.uType = USEASM_REGTYPE_TEMP_NAMED;
            $$.uNumber = IMG_UINT32_MAX;
            $$.uNamedRegLink = uNamedRegLink;
            $$.uIndex = USEREG_INDEX_NONE;
        }
    }
    else
    {
        ParseError("'%s' is not defined as named temporary register", $1.pszCharString);
        $$.uType = USEASM_REGTYPE_TEMP; $$.uNumber = 0; $$.uNamedRegLink = 0;
        $$.uIndex = USEREG_INDEX_NONE;
    }
    UseAsm_Free($1.pszCharString);
}
| SCOPE_NAME IDENTIFIER
{
    IMG_PVOID pvScope;
    IMG_UINT32 uNamedRegLink;
    IMG_PCHAR pszScopeName = GetScopeOrLabelName($1.pszCharString);
    if(LookupParentScopes(pszScopeName, &pvScope))
    {
        const IMG_PCHAR pszTempRegName = $2.pszCharString;
        if(LookupTemporaryRegisterInScope(pvScope, pszTempRegName, &uNamedRegLink))
        {
            $$.uType = USEASM_REGTYPE_TEMP_NAMED;
            $$.uNumber = IMG_UINT32_MAX;
            $$.uNamedRegLink = uNamedRegLink;
            $$.uIndex = USEREG_INDEX_NONE;
        }
        else
        {
            ParseError("No register with name '%s' is defined in scope '%s' at this point",
					   pszTempRegName, pszScopeName);
            $$.uType = USEASM_REGTYPE_TEMP; $$.uNumber = 0; $$.uNamedRegLink = 0;
            $$.uIndex = USEREG_INDEX_NONE;
        }
    }
    else
    {
        if(LookupProcScope(pszScopeName, $1.pszSourceFile, $1.uSourceLine, &pvScope))
        {
            const IMG_PCHAR pszTempRegName = $2.pszCharString;
            if(LookupTemporaryRegisterInScope(pvScope, pszTempRegName, &uNamedRegLink))
            {
                $$.uType = USEASM_REGTYPE_TEMP_NAMED;
                $$.uNumber = IMG_UINT32_MAX;
                $$.uNamedRegLink = uNamedRegLink;
                $$.uIndex = USEREG_INDEX_NONE;
            }
            else
            {
                ParseError("No register with name '%s' is defined in scope '%s' at this point",
						   pszTempRegName, pszScopeName);
                $$.uType = USEASM_REGTYPE_TEMP; $$.uNumber = 0; $$.uNamedRegLink = 0;
                $$.uIndex = USEREG_INDEX_NONE;
            }
        }
        else
        {
            AddUndefinedNamedRegisterInScope(pvScope,
											 $2.pszCharString,
											 $2.pszSourceFile,
											 $2.uSourceLine,
											 &uNamedRegLink);
            $$.uType = USEASM_REGTYPE_TEMP_NAMED;
            $$.uNumber = IMG_UINT32_MAX;
            $$.uNamedRegLink = uNamedRegLink;
            $$.uIndex = USEREG_INDEX_NONE;
        }
    }
    DeleteScopeOrLabelName(&pszScopeName);
    UseAsm_Free($1.pszCharString);
    UseAsm_Free($2.pszCharString);
}
| SCOPE_NAME IDENTIFIER OPEN_SQBRACKET HASH NUMBER CLOSE_SQBRACKET
{
    IMG_PVOID pvScope;
    IMG_UINT32 uNamedRegLink;
    IMG_PCHAR pszScopeName = GetScopeOrLabelName($1.pszCharString);
    if(LookupParentScopes(pszScopeName, &pvScope))
    {
        IMG_UINT32 uNamedRegLink;
        if(LookupNamedRegister($1.pszCharString, &uNamedRegLink) == IMG_TRUE)
        {
            if(IsArray(uNamedRegLink))
            {
                IMG_UINT32 uArrayElementsCount = GetArrayElementsCount(uNamedRegLink);
                if(uArrayElementsCount> $5)
                {
                    $$.uType = USEASM_REGTYPE_TEMP_NAMED;
                    $$.uNumber = $5;
                    $$.uNamedRegLink = uNamedRegLink;
                    $$.uIndex = USEREG_INDEX_NONE;
                }
                else
                {
                    ParseError("Can not index element '%u' on array '%s', of '%u' elements", $5,
							   GetNamedTempRegName(uNamedRegLink), uArrayElementsCount);
                    $$.uType = USEASM_REGTYPE_TEMP_NAMED;
                    $$.uNumber = uArrayElementsCount-1;
                    $$.uNamedRegLink = uNamedRegLink;
                    $$.uIndex = USEREG_INDEX_NONE;
                }
            }
            else
            {
                ParseError("Can not use '%s' as any array at this point",
						   GetNamedTempRegName(uNamedRegLink));
                $$.uType = USEASM_REGTYPE_TEMP_NAMED;
                $$.uNumber = IMG_UINT32_MAX;
                $$.uNamedRegLink = uNamedRegLink;
                $$.uIndex = USEREG_INDEX_NONE;
            }
        }
        else
        {
            ParseError("'%s' is not defined as named temporary registers array in scope '%s'",
					   $2.pszCharString, pszScopeName);
            $$.uType = USEASM_REGTYPE_TEMP; $$.uNumber = 0; $$.uNamedRegLink = 0;
            $$.uIndex = USEREG_INDEX_NONE;
        }
    }
    else
    {
        if(LookupProcScope(pszScopeName, $2.pszSourceFile, $2.uSourceLine, &pvScope))
        {
            IMG_UINT32 uNamedRegLink;
            if(LookupNamedRegister($1.pszCharString, &uNamedRegLink) == IMG_TRUE)
            {
                if(IsArray(uNamedRegLink))
                {
                    IMG_UINT32 uArrayElementsCount = GetArrayElementsCount(uNamedRegLink);
                    if(uArrayElementsCount> $5)
                    {
                        $$.uType = USEASM_REGTYPE_TEMP_NAMED;
                        $$.uNumber = $5;
                        $$.uNamedRegLink = uNamedRegLink;
                        $$.uIndex = USEREG_INDEX_NONE;
                    }
                    else
                    {
                        ParseError("Can not index element '%u' on array '%s', of '%u' elements",
								   $5,
								   GetNamedTempRegName(uNamedRegLink),
								   uArrayElementsCount);
                        $$.uType = USEASM_REGTYPE_TEMP_NAMED;
                        $$.uNumber = uArrayElementsCount-1;
                        $$.uNamedRegLink = uNamedRegLink;
                        $$.uIndex = USEREG_INDEX_NONE;
                    }
                }
                else
                {
                    ParseError("Can not use '%s' as any array at this point",
							   GetNamedTempRegName(uNamedRegLink));
                    $$.uType = USEASM_REGTYPE_TEMP_NAMED;
                    $$.uNumber = IMG_UINT32_MAX;
                    $$.uNamedRegLink = uNamedRegLink;
                    $$.uIndex = USEREG_INDEX_NONE;
                }
            }
            else
            {
                ParseError("'%s' is not defined as named temporary registers array in scope '%s'",
						   $2.pszCharString, pszScopeName);
                $$.uType = USEASM_REGTYPE_TEMP; $$.uNumber = 0; $$.uNamedRegLink = 0;
                $$.uIndex = USEREG_INDEX_NONE;
            }
        }
        else
        {
            AddUndefinedNamedRegsArrayInScope(pvScope, $2.pszCharString, $5,
											  $2.pszSourceFile,
											  $2.uSourceLine,
											  &uNamedRegLink);
            $$.uType = USEASM_REGTYPE_TEMP_NAMED;
            $$.uNumber = $5;
            $$.uNamedRegLink = uNamedRegLink;
            $$.uIndex = USEREG_INDEX_NONE;
        }
    }
    DeleteScopeOrLabelName(&pszScopeName);
    UseAsm_Free($1.pszCharString);
    UseAsm_Free($2.pszCharString);
}
;

expr:
NUMBER
{   $$ = $1;}
| expr PLUS expr
{   $$ = $1 + $3;}
| expr MINUS expr
{   $$ = $1 - $3;}
| expr TIMES expr
{   $$ = $1 * $3;}
| expr DIVIDE expr
{   $$ = $1 / $3;}
| expr LSHIFT expr
{   $$ = $1 << $3;}
| expr RSHIFT expr
{   $$ = $1 >> $3;}
| expr MODULUS expr
{   $$ = $1 % $3;}
| NOT expr
{   $$ = ~$2;}
| expr AND expr
{   $$ = $1 & $3;}
| expr OR expr
{   $$ = $1 | $3;}
| expr XOR expr
{   $$ = $1 ^ $3;}
| OPEN_BRACKET expr CLOSE_BRACKET
{   $$ = $2;}
| MINUS expr
{   $$ = -(IMG_INT32)$2;}
;

src_index_register:
INDEXLOW
{   $$ = 0;}
| INDEXHIGH
{   $$ = 1;}
;

dest_index_register:
INDEXLOW
{   $$ = 1;}
| INDEXHIGH
{   $$ = 2;}
| INDEXBOTH
{   $$ = 3;}
;

arg_modifier:
/* Nothing */
{   $$ = 0;}
| ARGUMENT_FLAG arg_modifier
{   $$ = $1 | $2;}
| C10_FLAG arg_modifier
{   $$ = USEASM_ARGFLAGS_FMTC10 | $2;}
| F16_FLAG arg_modifier
{   $$ = USEASM_ARGFLAGS_FMTF16 | $2;}
| U8_FLAG arg_modifier
{   $$ = USEASM_ARGFLAGS_FMTU8 | $2;}
| ABS arg_modifier
{   $$ = $2 | USEASM_ARGFLAGS_ABSOLUTE;}
;

ld_argument:
/* Base only forms */
OPEN_SQBRACKET arg_register CLOSE_SQBRACKET
{   $$.uOpFlags1 = 0;
    $$.uOpFlags2 = 0;
    $$.uCount = 1;
    $$.asArg[0] = $2;
    $$.asArg[0].uFlags = 0;}
/* Base and offset forms. */
| OPEN_SQBRACKET arg_register COMMA ldst_offset_argument CLOSE_SQBRACKET
{   $$.uOpFlags1 = $4.uOpFlags1;
    $$.uOpFlags2 = $4.uOpFlags2;
    $$.uCount = 2;
    $$.asArg[0] = $2;
    $$.asArg[0].uFlags = 0;
    $$.asArg[1] = $4.asArg[0];
    $$.asArg[1].uFlags = 0;}
/* Base, offset and range forms. */
| OPEN_SQBRACKET arg_register COMMA ldst_offset_argument COMMA arg_register CLOSE_SQBRACKET
{   $$.uOpFlags1 = USEASM_OPFLAGS1_RANGEENABLE | $4.uOpFlags1;
    $$.uOpFlags2 = $4.uOpFlags2;
    $$.uCount = 3;
    $$.asArg[0] = $2;
    $$.asArg[0].uFlags = 0;
    $$.asArg[1] = $4.asArg[0];
    $$.asArg[1].uFlags = 0;
    $$.asArg[2] = $6;
    $$.asArg[2].uFlags = 0;}
;

eld_argument:
/* No offset form. */
OPEN_SQBRACKET arg_register COMMA arg_register CLOSE_SQBRACKET
{   $$.uOpFlags1 = 0;
    $$.uOpFlags2 = 0;
    $$.uCount = 3;
    $$.asArg[0] = $2;
    $$.asArg[0].uFlags = 0;
    $$.asArg[1] = $4;
    $$.asArg[1].uFlags = 0;
    $$.asArg[2].uType = USEASM_REGTYPE_IMMEDIATE;
    $$.asArg[2].uNumber = 0;
    $$.asArg[2].uFlags = 0;
    $$.asArg[2].uIndex = USEREG_INDEX_NONE;}
| OPEN_SQBRACKET arg_register COMMA arg_register COMMA arg_register CLOSE_SQBRACKET
{   $$.uOpFlags1 = 0;
    $$.uOpFlags2 = 0;
    $$.uCount = 3;
    $$.asArg[0] = $2;
    $$.asArg[0].uFlags = 0;
    $$.asArg[1] = $4;
    $$.asArg[1].uFlags = 0;
    $$.asArg[2] = $6;
    $$.asArg[2].uFlags = 0;}

st_argument:
/* Base only forms */
OPEN_SQBRACKET arg_register CLOSE_SQBRACKET
{   $$.uOpFlags1 = 0;
    $$.uOpFlags2 = 0;
    $$.uCount = 1;
    $$.asArg[0] = $2;
    $$.asArg[0].uFlags = 0;}
/* Base and offset forms. */
| OPEN_SQBRACKET arg_register COMMA ldst_offset_argument CLOSE_SQBRACKET
{   $$.uOpFlags1 = $4.uOpFlags1;
    $$.uOpFlags2 = $4.uOpFlags2;
    $$.uCount = 2;
    $$.asArg[0] = $2;
    $$.asArg[0].uFlags = 0;
    $$.asArg[1] = $4.asArg[0];
    $$.asArg[1].uFlags = 0;}
;

ldst_offset_argument:
arg_register
{   $$.asArg[0] = $1;
    $$.uOpFlags1 = 0;
    $$.uOpFlags2 = 0;}
| offset_op arg_register
{   $$.asArg[0] = $2;
    $$.uOpFlags1 = 0;
    $$.uOpFlags2 = 0;
    if ($1 == PLUSPLUS || $1 == MINUSMINUS) $$.uOpFlags1 |= USEASM_OPFLAGS1_PREINCREMENT;
    if ($1 == MINUS || $1 == MINUSMINUS) $$.uOpFlags2 |= USEASM_OPFLAGS2_INCSGN;}
| arg_register offset_op
{   $$.asArg[0] = $1;
    $$.uOpFlags1 = 0;
    $$.uOpFlags2 = 0;
    if ($2 == PLUSPLUS || $2 == MINUSMINUS) $$.uOpFlags1 |= USEASM_OPFLAGS1_POSTINCREMENT;
    if ($2 == MINUS || $2 == MINUSMINUS) $$.uOpFlags2 |= USEASM_OPFLAGS2_INCSGN;}
;

offset_op:
PLUSPLUS
{   $$ = PLUSPLUS;}
| MINUSMINUS
{   $$ = MINUSMINUS;}
| MINUS
{   $$ = MINUS;}
;

efo_expression:
maybe_bang I0 EQUALS A0 COMMA maybe_bang I1 EQUALS A1 COMMA efo_addr_expr COMMA efo_mul_expr
{   $$ = ($1 ? 0 : EURASIA_USE1_EFO_WI0EN) |
		($6 ? 0 : EURASIA_USE1_EFO_WI1EN) |
		$13 |
		$11 |
		(EURASIA_USE1_EFO_ISRC_I0A0_I1A1 << EURASIA_USE1_EFO_ISRC_SHIFT);}
| maybe_bang I0 EQUALS A1 COMMA maybe_bang I1 EQUALS A0 COMMA efo_addr_expr COMMA efo_mul_expr
{   $$ = ($1 ? 0 : EURASIA_USE1_EFO_WI0EN) |
		($6 ? 0 : EURASIA_USE1_EFO_WI1EN) |
		$13 |
		$11 |
		(EURASIA_USE1_EFO_ISRC_I0A1_I1A0 << EURASIA_USE1_EFO_ISRC_SHIFT);}
| maybe_bang I0 EQUALS M0 COMMA maybe_bang I1 EQUALS M1 COMMA efo_addr_expr COMMA efo_mul_expr
{   $$ = ($1 ? 0 : EURASIA_USE1_EFO_WI0EN) |
		($6 ? 0 : EURASIA_USE1_EFO_WI1EN) |
		$13 |
		$11 |
		(EURASIA_USE1_EFO_ISRC_I0M0_I1M1 << EURASIA_USE1_EFO_ISRC_SHIFT);}
| maybe_bang I0 EQUALS A0 COMMA maybe_bang I1 EQUALS M1 COMMA efo_addr_expr COMMA efo_mul_expr
{   $$ = ($1 ? 0 : EURASIA_USE1_EFO_WI0EN) |
		($6 ? 0 : EURASIA_USE1_EFO_WI1EN) |
		$13 |
		$11 |
		(EURASIA_USE1_EFO_ISRC_I0A0_I1M1 << EURASIA_USE1_EFO_ISRC_SHIFT);}
| maybe_bang I1 EQUALS A1 COMMA efo_addr_expr COMMA efo_mul_expr
{   $$ = ($1 ? 0 : EURASIA_USE1_EFO_WI1EN) |
		$6 |
		$8 |
		(EURASIA_USE1_EFO_ISRC_I0A0_I1A1 << EURASIA_USE1_EFO_ISRC_SHIFT);}
| maybe_bang I1 EQUALS A0 COMMA efo_addr_expr COMMA efo_mul_expr
{   $$ = ($1 ? 0 : EURASIA_USE1_EFO_WI1EN) |
		$6 |
		$8 |
		(EURASIA_USE1_EFO_ISRC_I0A1_I1A0 << EURASIA_USE1_EFO_ISRC_SHIFT);}
| maybe_bang I1 EQUALS M1 COMMA efo_addr_expr COMMA efo_mul_expr
{   $$ = ($1 ? 0 : EURASIA_USE1_EFO_WI1EN) |
		$6 |
		$8 |
		(EURASIA_USE1_EFO_ISRC_I0M0_I1M1 << EURASIA_USE1_EFO_ISRC_SHIFT);}
| maybe_bang I0 EQUALS A0 COMMA efo_addr_expr COMMA efo_mul_expr
{   $$ = ($1 ? 0 : EURASIA_USE1_EFO_WI0EN) |
		$6 |
		$8 |
		(EURASIA_USE1_EFO_ISRC_I0A0_I1A1 << EURASIA_USE1_EFO_ISRC_SHIFT);}
| maybe_bang I0 EQUALS A1 COMMA efo_addr_expr COMMA efo_mul_expr
{   $$ = ($1 ? 0 : EURASIA_USE1_EFO_WI0EN) |
		$6 |
		$8 |
		(EURASIA_USE1_EFO_ISRC_I0A1_I1A0 << EURASIA_USE1_EFO_ISRC_SHIFT);}
| maybe_bang I0 EQUALS M0 COMMA efo_addr_expr COMMA efo_mul_expr
{   $$ = ($1 ? 0 : EURASIA_USE1_EFO_WI0EN) |
		$6 |
		$8 |
		(EURASIA_USE1_EFO_ISRC_I0M0_I1M1 << EURASIA_USE1_EFO_ISRC_SHIFT);}
;

efo_addr_expr:
A0 EQUALS efo_add_src PLUS maybe_negate_r efo_add_src COMMA A1
EQUALS maybe_negate efo_add_src PLUS efo_add_src
{
    $$ = ParseEfoAddExpr($3, $5, $6, $10, $11, $13);
}
;

efo_add_src:
SRC0
{   $$ = SRC0;}
| SRC1
{   $$ = SRC1;}
| SRC2
{   $$ = SRC2;}
| I0
{   $$ = I0;}
| I1
{   $$ = I1;}
| M0
{   $$ = M0;}
| M1
{   $$ = M1;}
;

efo_mul_expr:
M0 EQUALS efo_mul_src TIMES efo_mul_src COMMA M1 EQUALS efo_mul_src TIMES efo_mul_src
{
    $$ = ParseEfoMulExpr($3, $5, $9, $11);
}
;

efo_mul_src:
SRC0
{   $$ = SRC0;}
| SRC1
{   $$ = SRC1;}
| SRC2
{   $$ = SRC2;}
| I0
{   $$ = I0;}
| I1
{   $$ = I1;}
;

efo_dest_select:
I0
{   $$ = EURASIA_USE1_EFO_DSRC_I0;}
| I1
{   $$ = EURASIA_USE1_EFO_DSRC_I1;}
| A0
{   $$ = EURASIA_USE1_EFO_DSRC_A0;}
| A1
{   $$ = EURASIA_USE1_EFO_DSRC_A1;}
;

maybe_bang:
/* Nothing */
{   $$ = 0;}
| BANG
{   $$ = 1;}
;

maybe_negate:
/* Nothing */
{   $$ = 0;}
| MINUS
{   $$ = 1;}
;

maybe_negate_r:
/* Nothing */
{   $$ = 0;}
| MINUS
{   $$ = 1;}
;

temp_reg_names_optional:
/* Nothing */
| temp_reg_names
;

temp_reg_names:
temp_reg_name
| temp_reg_names COMMA temp_reg_name
{}

temp_reg_name:
IDENTIFIER
{   AddNamedRegister($1.pszCharString, $1.pszSourceFile, $1.uSourceLine); UseAsm_Free($1.pszCharString);}
| IDENTIFIER OPEN_SQBRACKET HASH NUMBER CLOSE_SQBRACKET
{
    if($4> 0)
    {
        AddNamedRegistersArray($1.pszCharString, $4, $1.pszSourceFile, $1.uSourceLine);
    }
    else
    {
        ParseError("Can not declare any array with %d number of elements", $4);
    }
    UseAsm_Free($1.pszCharString);
}
| IDENTIFIER OPEN_SQBRACKET expr CLOSE_SQBRACKET
{
    if($3> 0)
    {
        AddNamedRegistersArray($1.pszCharString, $3, $1.pszSourceFile, $1.uSourceLine);
    }
    else
    {
        ParseError("Can not declare any array with %d number of elements", $3);
    }
    UseAsm_Free($1.pszCharString);
}

%%
