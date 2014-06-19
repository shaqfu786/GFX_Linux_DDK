/*************************************************************************
 * Name         : uselink.c
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
 * $Log: uselink.c $
 **************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#if !defined(LINUX)
#include <io.h>
#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(X) X;
#endif /* UNREFERENCED_PARAMETER */
#else
#include <errno.h>
#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(X)
#endif /* UNREFERENCED_PARAMETER */
#define max(a,b) a>b?a:b
#define stricmp strcasecmp
#endif
#include <assert.h>

#include "sgxdefs.h"
#include "use.h"
#include "useasm.h"
#include "objectfile.h"
#include "osglue.h"

#include <stdio.h>

static struct
{
	IMG_UINT32	uOutputOffset;

	IMG_UINT32 uExportCount;
	struct
	{
		IMG_PCHAR	pszName;
		IMG_UINT32	uAddress;
	} *psExport;

	IMG_UINT32 uImportCount;
	struct
	{
		IMG_PCHAR			pszName;
		IMG_PCHAR			pszSourceFileName;
		USEASM_IMPORTDATA	sData;
	} *psImport;

	IMG_UINT32	uInstCount;
	IMG_PUINT32	puInsts;

	IMG_UINT32	uLADDRRelocationCount;
	PUSEASM_LADDRRELOCATION psLADDRRelocations;

	IMG_PCHAR pszModuleName;

} *g_psModules;
static IMG_UINT32	g_uModuleCount;

/*****************************************************************************
 FUNCTION	: FindExport
    
 PURPOSE	: Look for an exported label.

 PARAMETERS	: pszName			- Name of the label to find.
			  puExportModule	- On success initialized with the index of
								the module that contains the label.
			  puAddress			- On success initialized with the address of
								the label in the module.
			  
 RETURNS	: TRUE if the label was found.
*****************************************************************************/
static IMG_BOOL FindExport(IMG_PCHAR	pszName,
						   IMG_PUINT32	puExportModule,
						   IMG_PUINT32	puAddress)
{
	IMG_UINT32	uModule;
	IMG_UINT32	uExport;

	for (uModule = 0; uModule < g_uModuleCount; uModule++)
	{
		for (uExport = 0; uExport < g_psModules[uModule].uExportCount; uExport++)
		{
			if (strcmp(g_psModules[uModule].psExport[uExport].pszName, pszName) == 0)
			{
				*puExportModule = uModule;
				*puAddress = g_psModules[uModule].psExport[uExport].uAddress;
				return IMG_TRUE;
			}
		}
	}
	return IMG_FALSE;
}

static void usage(void)
{
	printf("Usage: uselink [-startoffset=<number>] [-label_header=<filename> -label_prefix=<text>] -out=<filename> <module name> <module name> ...\n");
}

/*****************************************************************************
 FUNCTION	: GetLimmData
    
 PURPOSE	: Get the data loaded by a LIMM instruction

 PARAMETERS	: puCode		- The instruction.
			  
 RETURNS	: The loaded data.
*****************************************************************************/
static IMG_UINT32 GetLimmData(IMG_PUINT32	puCode)
{
	IMG_UINT32	uData;

	uData = (puCode[0] & ~EURASIA_USE0_LIMM_IMML21_CLRMSK) >> EURASIA_USE0_LIMM_IMML21_SHIFT;
	uData |= ((puCode[1] & ~EURASIA_USE1_LIMM_IMM2521_CLRMSK) >> EURASIA_USE1_LIMM_IMM2521_SHIFT) << 21;
	uData |= ((puCode[1] & ~EURASIA_USE1_LIMM_IMM3126_CLRMSK) >> EURASIA_USE1_LIMM_IMM3126_SHIFT) << 26;

	return uData;
}

/*****************************************************************************
 FUNCTION	: SetLimmData
    
 PURPOSE	: Sets the data loaded by a LIMM instruction

 PARAMETERS	: puCode		- The instruction.
			  uData			- The data to load.
			  
 RETURNS	: Nothing.
*****************************************************************************/
static IMG_VOID SetLimmData(IMG_PUINT32	puCode,
							IMG_UINT32	uValue)
{
	puCode[1] &= (EURASIA_USE1_LIMM_IMM3126_CLRMSK & EURASIA_USE1_LIMM_IMM2521_CLRMSK);
	puCode[0] &= EURASIA_USE0_LIMM_IMML21_CLRMSK;

	puCode[0] |= ((uValue >> 0) << EURASIA_USE0_LIMM_IMML21_SHIFT) & ~EURASIA_USE0_LIMM_IMML21_CLRMSK;
	puCode[1] |= ((uValue >> 21) << EURASIA_USE1_LIMM_IMM2521_SHIFT) & ~EURASIA_USE1_LIMM_IMM2521_CLRMSK;
	puCode[1] |= ((uValue >> 26) << EURASIA_USE1_LIMM_IMM3126_SHIFT) & ~EURASIA_USE1_LIMM_IMM3126_CLRMSK;
}

#if !defined(LINUX)
int __cdecl main (int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
	IMG_UINT32		uModule;
	IMG_UINT32		uOutputSize;
	IMG_PCHAR		pszOutFileName;
	FILE*			fOut;
	SGX_CORE_INFO	sTarget = {0, };
	IMG_UINT32		uPCMax;
	IMG_UINT32		uPCMask;
	IMG_PCHAR		pszLabelHeaderFileName;
	IMG_PCHAR		pszLabelPrefix;
	IMG_UINT32		uCodeOffset;
	IMG_UINT32		uMaxNamedTempRegIdx;
	
	/*
		Parse command line arguments.
	*/
	pszOutFileName = NULL;
	pszLabelHeaderFileName = NULL;
	pszLabelPrefix = "";
	uCodeOffset = 0;
	while (argc > 1 && argv[1][0] == '-')
	{
		if (strncmp(argv[1], "-out=", strlen("-out=")) == 0)
		{
			pszOutFileName = strdup(argv[1] + strlen("-out="));

			if (pszOutFileName == '\0')
			{
				usage();
				return 1;
			}

			memmove(&argv[1], &argv[2], (argc - 2) * sizeof(argv[1]));
			argc--;
		}
		else if (strncmp(argv[1], "-label_prefix=", strlen("-label_prefix=")) == 0)
		{
			pszLabelPrefix = strdup(argv[1] + strlen("-label_prefix="));
			memmove(&argv[1], &argv[2], (argc - 2) * sizeof(argv[1]));
			argc--;
		}
		else if (strncmp(argv[1], "-label_header=", strlen("-label_header=")) == 0)
		{
			pszLabelHeaderFileName = strdup(argv[1] + strlen("-label_header="));
			memmove(&argv[1], &argv[2], (argc - 2) * sizeof(argv[1]));
			argc--;
		}
		else if (strncmp(argv[1], "-startoffset=", strlen("-startoffset=")) == 0)
		{
			uCodeOffset = strtoul(argv[1] + strlen("-startoffset="), NULL, 0);
			if ((uCodeOffset % EURASIA_USE_INSTRUCTION_SIZE) != 0)
			{
				fprintf(stderr, "The code offset should be aligned to the length of an instruction (8 bytes)");
				return 1;
			}
			uCodeOffset /= EURASIA_USE_INSTRUCTION_SIZE;
			memmove(&argv[1], &argv[2], (argc - 2) * sizeof(argv[1]));
			argc--;
		}
		else
		{
			usage();
			return 1;
		}
	}
	if (pszOutFileName == NULL)
	{
		usage();
		return 1;
	}

	uOutputSize = 0;
	uMaxNamedTempRegIdx = USE_UNDEF;
	g_uModuleCount = argc - 1;
	g_psModules = UseAsm_Malloc(sizeof(g_psModules[0]) * g_uModuleCount);
	for (uModule = 0; uModule < g_uModuleCount; uModule++)
	{
		FILE*					fIn;
		USEASM_OBJFILE_HEADER	sHeader;
		IMG_PCHAR				pszFileName = argv[uModule + 1];
		IMG_PVOID				pvExportDataSegment;
		IMG_UINT32				uRound;
		IMG_PVOID				pvStringSegment;
		IMG_UINT32				uExport;
		IMG_PVOID				pvImportDataSegment;
		IMG_UINT32				uImport;
		IMG_UINT32				uReloc;
		IMG_UINT32				uNamedTempStart;
		IMG_UINT32				uNamedTempEnd;

		fIn = fopen(pszFileName, "rb");
		if (fIn == NULL)
		{
			fprintf(stderr, "%s: Couldn't open input file %s: %s.\n", argv[0], pszFileName, strerror(errno));
			return 1;
		}

		g_psModules[uModule].pszModuleName = strdup(pszFileName);

		/*
			Read the file header.
		*/
		if (fread(&sHeader, sizeof(sHeader), 1, fIn) != 1)
		{
			fprintf(stderr, "%s: Couldn't read from input file %s: %s.\n", argv[0], pszFileName, strerror(errno));
			return 1;
		}

		/*
			Check for consistency.
		*/
		if (strncmp(sHeader.pszFileId, USEASM_OBJFILE_ID, sizeof(sHeader.pszFileId)) != 0)
		{
			fprintf(stderr, "%s: Input file %s isn't a valid useasm object file.\n", argv[0], pszFileName);
			return 1;
		}
		if (sHeader.uVersion != 0)
		{
			fprintf(stderr, "%s: Input file %s isn't a valid useasm object file.\n", argv[0], pszFileName);
			return 1;
		}
		if (uModule > 0)
		{
			if (sHeader.sTarget.eID != sTarget.eID || sHeader.sTarget.uiRev != sTarget.uiRev)
			{
				fprintf(stderr, "%s: Input file %s wasn't compiled for the same target as "
					"previous modules.\n", argv[0], pszFileName);
				return 1;
			}
		}
		else
		{
			sTarget = sHeader.sTarget;
		}

		/*
			Align the start of the module.
		*/
		uRound = EURASIA_USE_INSTRUCTION_SIZE * sHeader.uModuleAlignment;
		if ((uOutputSize % uRound) != 0)
		{
			uOutputSize += (uRound - (uOutputSize % uRound));
		}

		/*
			Add space for the module to the output file.
		*/
		g_psModules[uModule].uOutputOffset = uOutputSize;
		uOutputSize += sHeader.uInstructionCount * EURASIA_USE_INSTRUCTION_SIZE;

		/*
			Read in the instructions.
		*/
		g_psModules[uModule].uInstCount = sHeader.uInstructionCount;
		g_psModules[uModule].puInsts = UseAsm_Malloc(sHeader.uInstructionCount * EURASIA_USE_INSTRUCTION_SIZE);
		if (fseek(fIn, sHeader.uInstructionOffset, SEEK_SET) != 0)
		{
			fprintf(stderr, "%s: Couldn't read code segment from input file %s: %s.\n", argv[0], pszFileName, strerror(errno));
			return 1;
		}
		if (fread(g_psModules[uModule].puInsts, EURASIA_USE_INSTRUCTION_SIZE, sHeader.uInstructionCount, fIn) 
			!= sHeader.uInstructionCount)
		{
			fprintf(stderr, "%s: Couldn't read code segment from input file %s: %s.\n", argv[0], pszFileName, strerror(errno));
			return 1;
		}

		/*
			Read the string segment.
		*/
		pvStringSegment = UseAsm_Malloc(sHeader.uStringDataSize);
		if (fseek(fIn, sHeader.uStringDataOffset, SEEK_SET) != 0)
		{
			fprintf(stderr, "%s: Couldn't read string segment from input file %s: %s.\n", argv[0], pszFileName, strerror(errno));
			return 1;
		}
		if (fread(pvStringSegment, 1, sHeader.uStringDataSize, fIn) != sHeader.uStringDataSize)
		{
			fprintf(stderr, "%s: Couldn't read string segment from input file %s: %s.\n", argv[0], pszFileName, strerror(errno));
			return 1;
		}

		/*
			Read the export segment.
		*/
		g_psModules[uModule].uExportCount = sHeader.uExportDataCount;
		g_psModules[uModule].psExport = UseAsm_Malloc(sHeader.uExportDataCount * sizeof(g_psModules[uModule].psExport[0]));
		pvExportDataSegment = UseAsm_Malloc(sHeader.uExportDataCount * sizeof(USEASM_EXPORTDATA));
		if (fseek(fIn, sHeader.uExportDataOffset, SEEK_SET) != 0)
		{
			fprintf(stderr, "%s: Couldn't read export segment from input file %s: %s.\n", argv[0], pszFileName, strerror(errno));
			return 1;
		}
		if (fread(pvExportDataSegment, sizeof(USEASM_EXPORTDATA), sHeader.uExportDataCount, fIn) != sHeader.uExportDataCount)
		{
			fprintf(stderr, "%s: Couldn't read export segment from input file %s: %s.\n", argv[0], pszFileName, strerror(errno));
			return 1;
		}
		for (uExport = 0; uExport < sHeader.uExportDataCount; uExport++)
		{
			PUSEASM_EXPORTDATA	psExport;
			IMG_UINT32			uPrevModule;
			IMG_UINT32			uPrevExport;

			psExport = (PUSEASM_EXPORTDATA)pvExportDataSegment + uExport;

			if (psExport->uNameOffset >= sHeader.uStringDataSize)
			{
				fprintf(stderr, "%s: Invalid export segment in input file %s.\n", argv[0], pszFileName);
				return 1;
			}
			if (psExport->uLabelAddress > sHeader.uInstructionCount)
			{
				fprintf(stderr, "%s: Invalid export segment in input file %s.\n", argv[0], pszFileName);
				return 1;
			}

			g_psModules[uModule].psExport[uExport].pszName = (IMG_PCHAR)pvStringSegment + psExport->uNameOffset;
			g_psModules[uModule].psExport[uExport].uAddress = psExport->uLabelAddress;

			/*
				Check for duplicated export definitions.
			*/
			for (uPrevModule = 0; uPrevModule < uModule; uPrevModule++)
			{
				for (uPrevExport = 0; uPrevExport < g_psModules[uPrevModule].uExportCount; uPrevExport++)
				{
					if (strcmp(g_psModules[uModule].psExport[uExport].pszName,
							   g_psModules[uPrevModule].psExport[uPrevExport].pszName) == 0)
					{
						fprintf(stderr, "%s: error: Label '%s' is defined in both %s and %s.\n",
							argv[0],
							g_psModules[uModule].psExport[uExport].pszName,
							g_psModules[uPrevModule].pszModuleName,
							g_psModules[uModule].pszModuleName);
						return 1;
					}
				}
			}
		}

		/*
			Read the import segment.
		*/
		g_psModules[uModule].uImportCount = sHeader.uImportDataCount;
		g_psModules[uModule].psImport = UseAsm_Malloc(sHeader.uImportDataCount * sizeof(g_psModules[uModule].psImport[0]));
		pvImportDataSegment = UseAsm_Malloc(sHeader.uImportDataCount * sizeof(USEASM_IMPORTDATA));
		if (fseek(fIn, sHeader.uImportDataOffset, SEEK_SET) != 0)
		{
			fprintf(stderr, "%s: Couldn't read import segment from input file %s: %s.\n", argv[0], pszFileName, strerror(errno));
			return 1;
		}
		if (fread(pvImportDataSegment, sizeof(USEASM_IMPORTDATA), sHeader.uImportDataCount, fIn) != sHeader.uImportDataCount)
		{
			fprintf(stderr, "%s: Couldn't read import segment from input file %s: %s.\n", argv[0], pszFileName, strerror(errno));
			return 1;
		}
		for (uImport = 0; uImport < sHeader.uImportDataCount; uImport++)
		{
			PUSEASM_IMPORTDATA	psImport;

			psImport = (PUSEASM_IMPORTDATA)pvImportDataSegment + uImport;

			if (psImport->uNameOffset >= sHeader.uStringDataSize)
			{
				fprintf(stderr, "%s: Invalid import segment in input file %s.\n", argv[0], pszFileName);
				return 1;
			}
			if (psImport->uFixupSourceFileNameOffset >= sHeader.uStringDataSize)
			{
				fprintf(stderr, "%s: Invalid import segment in input file %s.\n", argv[0], pszFileName);
				return 1;
			}
			if (psImport->uFixupAddress >= sHeader.uInstructionCount)
			{
				fprintf(stderr, "%s: Invalid import segment in input file %s.\n", argv[0], pszFileName);
				return 1;
			}

			g_psModules[uModule].psImport[uImport].pszName = (IMG_PCHAR)pvStringSegment + psImport->uNameOffset;
			g_psModules[uModule].psImport[uImport].pszSourceFileName = 
				(IMG_PCHAR)pvStringSegment + psImport->uFixupSourceFileNameOffset;
			g_psModules[uModule].psImport[uImport].sData = *psImport;
		}

		/*
			Read the LADDR relocation segment.
		*/
		g_psModules[uModule].uLADDRRelocationCount = sHeader.uLADDRRelocationCount;
		g_psModules[uModule].psLADDRRelocations = UseAsm_Malloc(sHeader.uLADDRRelocationCount * sizeof(USEASM_LADDRRELOCATION));
		if (fseek(fIn, sHeader.uLADDRRelocationOffset, SEEK_SET) != 0)
		{
			fprintf(stderr, "%s: Couldn't read LADDR relocation segment from input file %s: %s.\n", argv[0], pszFileName, strerror(errno));
			return 1;
		}
		if (fread(g_psModules[uModule].psLADDRRelocations, 
				  sizeof(USEASM_LADDRRELOCATION), 
				  sHeader.uLADDRRelocationCount, 
				  fIn) 
			!= sHeader.uLADDRRelocationCount)
		{
			fprintf(stderr, "%s: Couldn't read LADDR relocation segment from input file %s: %s.\n", argv[0], pszFileName, strerror(errno));
			return 1;
		}
		for (uReloc = 0; uReloc < g_psModules[uModule].uLADDRRelocationCount; uReloc++)
		{
			PUSEASM_LADDRRELOCATION		psReloc = g_psModules[uModule].psLADDRRelocations + uReloc;
			if (psReloc->uAddress >= g_psModules[uModule].uInstCount)
			{
				fprintf(stderr, "%s: Invalid LADDR Relocation segment in input file %s.\n", argv[0], pszFileName);
				return 1;
			}
		}

		/*
			Record the maximum temp-register allocated by the assembler for named
			registers across all modules

			NB: A special case of start=1, end=0 will be set by the assembler for 
				files that don't use named registers at all. For other case, start
				must be <= end and start-end represents the (inclusive) range of
				HW temp registers used by the module (i.e. if start==end then 1
				named register was used). Modules that do not call each other 
				can use overlapping register ranges.
		*/
		uNamedTempStart = sHeader.uHwRegsAllocRangeStart;
		uNamedTempEnd	= sHeader.uHwRegsAllocRangeEnd;

		if	((uNamedTempStart != 1) && (uNamedTempEnd != 0))
		{
			IMG_UINT32	uMaxNumHWTemps = EURASIA_USE_REGISTER_NUMBER_MAX;

			if	(
					(uNamedTempStart > uNamedTempEnd) ||
					(uNamedTempStart >= uMaxNumHWTemps) ||
					(uNamedTempEnd >= uMaxNumHWTemps)
				)
			{
				fprintf(stderr, 
						"%s: Invalid named register-range (%u-%u) in input file %s.\n", 
						argv[0], 
						uNamedTempStart,
						uNamedTempEnd,
						pszFileName);

				return 1;
			}

			/*
				Record the maximum HW Temp register used for named regs
			*/
			if	(uMaxNamedTempRegIdx == USE_UNDEF)
			{
				uMaxNamedTempRegIdx = uNamedTempEnd;
			}
			else
			{
				uMaxNamedTempRegIdx = max(uMaxNamedTempRegIdx, uNamedTempEnd); 
			}
		}
	}

	uPCMax = (1 << NumberOfProgramCounterBits(UseAsmGetCoreDesc(&sTarget))) - 1;
	uPCMask = ~(uPCMax << EURASIA_USE0_BRANCH_OFFSET_SHIFT);

	/*
		Check for an invalid program.
	*/
	if (uOutputSize > ((uPCMax + 1) * EURASIA_USE_INSTRUCTION_SIZE))
	{
		fprintf(stderr, "%s: error: The USE program is too large. The maximum size for this processor is %u bytes but the "
			"actual size is %u bytes.\n", argv[0], (uPCMax + 1) * EURASIA_USE_INSTRUCTION_SIZE, uOutputSize);
		return 1;
	}
	
	/*
		Print the final program size for information.
	*/
	fprintf(stderr, "%s: output program size is %u bytes (%u instructions).\n",
		pszOutFileName, uOutputSize, uOutputSize / EURASIA_USE_INSTRUCTION_SIZE); 

	/*
		Print the maximum HW temp register used for named registers
	*/
	if	(uMaxNamedTempRegIdx != USE_UNDEF)
	{
		fprintf(stderr, "%s: output program maximum HW temp-reg used for named registers is R%u.\n",
			pszOutFileName, uMaxNamedTempRegIdx); 
	}

	/*
		Relocate references to labels in LIMM instructions
	*/
	for (uModule = 0; uModule < g_uModuleCount; uModule++)
	{
		IMG_UINT32	uModuleStart = g_psModules[uModule].uOutputOffset / EURASIA_USE_INSTRUCTION_SIZE;
		IMG_UINT32	uReloc;

		for (uReloc = 0; uReloc < g_psModules[uModule].uLADDRRelocationCount; uReloc++)
		{
			PUSEASM_LADDRRELOCATION		psReloc = g_psModules[uModule].psLADDRRelocations + uReloc;
			IMG_PUINT32					puCode = &g_psModules[uModule].puInsts[psReloc->uAddress * 2 + 0];
			IMG_UINT32					uOldValue;

			uOldValue = GetLimmData(puCode);

			uOldValue += uCodeOffset;
			uOldValue += uModuleStart;

			SetLimmData(puCode, uOldValue);
		}
	}

	/*	
		Relocate the address of absolute branches in each module.
	*/
	for (uModule = 0; uModule < g_uModuleCount; uModule++)
	{
		IMG_UINT32	uModuleStart = g_psModules[uModule].uOutputOffset / EURASIA_USE_INSTRUCTION_SIZE;
		IMG_UINT32	uInst;

		for (uInst = 0; uInst < g_psModules[uModule].uInstCount; uInst++)
		{
			IMG_PUINT32 puInstWord0 = &g_psModules[uModule].puInsts[uInst * 2 + 0];
			IMG_UINT32 uInstWord1 = g_psModules[uModule].puInsts[uInst * 2 + 1];
			IMG_UINT32 uOp = (uInstWord1 & ~EURASIA_USE1_OP_CLRMSK) >> EURASIA_USE1_OP_SHIFT;
			IMG_UINT32 uOpCat = (uInstWord1 & ~EURASIA_USE1_SPECIAL_OPCAT_CLRMSK) >> EURASIA_USE1_SPECIAL_OPCAT_SHIFT;
			IMG_UINT32 uOp2 = (uInstWord1 & ~EURASIA_USE1_FLOWCTRL_OP2_CLRMSK) >> EURASIA_USE1_FLOWCTRL_OP2_SHIFT;

			if (uOp == EURASIA_USE1_OP_SPECIAL && uOpCat == EURASIA_USE1_SPECIAL_OPCAT_FLOWCTRL)
			{
#if defined(SGX_FEATURE_USE_UNLIMITED_PHASES)
				if (!(uInstWord1 & EURASIA_USE1_SPECIAL_OPCAT_EXTRA))
#endif /* defined(SGX_FEATURE_USE_UNLIMITED_PHASES) */
				{
					if (uOp2 == EURASIA_USE1_FLOWCTRL_OP2_BA)
					{
						IMG_UINT32	uAddress;

						uAddress = ((*puInstWord0) & ~uPCMask) >> EURASIA_USE0_BRANCH_OFFSET_SHIFT;
						(*puInstWord0) &= uPCMask;
						uAddress += uCodeOffset;
						uAddress += uModuleStart;
						if (uAddress > uPCMax)
						{
							fprintf(stderr, "%s: error: Couldn't relocate absolute branch at %u in module %s - the "
								"address is out of range.", argv[0], uInst, g_psModules[uModule].pszModuleName);
							return 1;
						}
						(*puInstWord0) |= (uAddress & uPCMax) << EURASIA_USE0_BRANCH_OFFSET_SHIFT;
					}
				}
			}
		}
	}

	/*
		Fix up intermodule references.
	*/
	for (uModule = 0; uModule < g_uModuleCount; uModule++)
	{
		IMG_UINT32	uImport;

		for (uImport = 0; uImport < g_psModules[uModule].uImportCount; uImport++)
		{
			IMG_PCHAR			pszImportName = g_psModules[uModule].psImport[uImport].pszName;
			PUSEASM_IMPORTDATA	psImport = &g_psModules[uModule].psImport[uImport].sData;
			IMG_UINT32			uExportModule, uExportAddress;
			IMG_PUINT32			puCode;
			IMG_UINT32			uFinalAddress;

			/*
				Look for a label matching this import in another other module.
			*/
			if (!FindExport(pszImportName, &uExportModule, &uExportAddress))
			{
				fprintf(stderr, "%s(%u): error: Undefined label '%s' referenced in module %s\n", 
						g_psModules[uModule].psImport[uImport].pszSourceFileName,
						psImport->uFixupSourceLineNumber,
						pszImportName, 
					g_psModules[uModule].pszModuleName);
				return 1;
			}

			/*
				Make the export address an offset in the final binary.
			*/
			uExportAddress += (g_psModules[uExportModule].uOutputOffset / EURASIA_USE_INSTRUCTION_SIZE);

			/*
				Adjust the address for the branch type.
			*/
			if (psImport->uFixupOp == LABEL_REF_BR)
			{
				uFinalAddress = uExportAddress - psImport->uFixupAddress;
				uFinalAddress -= (g_psModules[uModule].uOutputOffset / EURASIA_USE_INSTRUCTION_SIZE);

				if (((IMG_INT32)uFinalAddress < -(IMG_INT32)((uPCMax + 1) / 2)) ||
					((IMG_INT32)uFinalAddress >= (IMG_INT32)((uPCMax + 1) / 2)))
				{
					fprintf(stderr, "%s(%u): error: Branch to label %s is out of range (the maximum range is [%d..%d])\n", 
						g_psModules[uModule].psImport[uImport].pszSourceFileName,
						psImport->uFixupSourceLineNumber,
						pszImportName, 
						-(IMG_INT32)((uPCMax + 1) / 2), 
						(IMG_INT32)((uPCMax + 1) / 2));
					return 1;
				}
			}
			else if (psImport->uFixupOp == LABEL_REF_BA)
			{
				uFinalAddress = uExportAddress + uCodeOffset;

				if (uFinalAddress > uPCMax)
				{
					fprintf(stderr, "%s(%u): error: Branch to label %s is out of range (the maximum range is [0..%u])\n", 
						g_psModules[uModule].psImport[uImport].pszSourceFileName,
						psImport->uFixupSourceLineNumber,
						pszImportName, 
						uPCMax);
					return 1;
				}
			}
			else if (psImport->uFixupOp == LABEL_REF_LIMM)
			{
				uFinalAddress = uExportAddress + uCodeOffset; 
			}
			else
			{
				fprintf(stderr, "%s: error: Unknown fixup type (%u)\n", argv[0], psImport->uFixupOp);
				return 1;
			}

			/*
				Update the branch destination.
			*/
			puCode = &g_psModules[uModule].puInsts[psImport->uFixupAddress * 2];
			if (psImport->uFixupOp == LABEL_REF_BR || psImport->uFixupOp == LABEL_REF_BA)
			{
				puCode[0] &= uPCMask;
				puCode[0] |= (uFinalAddress << EURASIA_USE0_BRANCH_OFFSET_SHIFT) & ~uPCMask;
			}
			else
			{
				SetLimmData(puCode, uFinalAddress);
			}
		}
	}

	/*
		Write the output binary.
	*/
	fOut = fopen(pszOutFileName, "wb");
	if (fOut == NULL)
	{
		fprintf(stderr, "%s: Couldn't open output file %s: %s.\n", argv[0], pszOutFileName, strerror(errno));
		return 1;
	}
	for (uModule = 0; uModule < g_uModuleCount; uModule++)
	{
		if (fseek(fOut, g_psModules[uModule].uOutputOffset, SEEK_SET) != 0)
		{
			fprintf(stderr, "%s: Couldn't write to output file %s: %s.\n", argv[0], pszOutFileName, strerror(errno));
			return 1;
		}
		if (fwrite(g_psModules[uModule].puInsts, EURASIA_USE_INSTRUCTION_SIZE, g_psModules[uModule].uInstCount, fOut) 
			!= g_psModules[uModule].uInstCount)
		{
			fprintf(stderr, "%s: Couldn't write to output file %s: %s.\n", argv[0], pszOutFileName, strerror(errno));
			return 1;
		}
	}
	fclose(fOut);

	/*
		Write the label header.
	*/
	if (pszLabelHeaderFileName != NULL)
	{
		FILE*		fLabelHeaderFile;
		IMG_UINT32	uMaxLabelNameLength;
		static IMG_CHAR pszLabelPostfix[] = "_OFFSET";
		IMG_UINT32	uExport;

		fLabelHeaderFile = fopen(pszLabelHeaderFileName, "wb");
		if (fLabelHeaderFile == NULL)
		{
			fprintf(stderr, "%s: Couldn't open label header file %s: %s.\n", argv[0], pszLabelHeaderFileName, strerror(errno)); 
			return 1;
		}

		uMaxLabelNameLength = 0;
		for (uModule = 0; uModule < g_uModuleCount; uModule++)
		{
			for (uExport = 0; uExport < g_psModules[uModule].uExportCount; uExport++)
			{
				uMaxLabelNameLength = max(uMaxLabelNameLength, strlen(g_psModules[uModule].psExport[uExport].pszName));
			}
		}
		uMaxLabelNameLength += strlen(pszLabelPrefix);
		uMaxLabelNameLength += strlen(pszLabelPostfix);

		if (fprintf(fLabelHeaderFile, "/* Auto-generated file - don't edit */\n\n") < 0)
		{
			fprintf(stderr, "Couldn't write label header file: %s.\n", strerror(errno));
			fclose(fLabelHeaderFile);
			return 1;
		}

		for (uModule = 0; uModule < g_uModuleCount; uModule++)
		{
			for (uExport = 0; uExport < g_psModules[uModule].uExportCount; uExport++)
			{
				IMG_PCHAR	pszDefineName;
				IMG_UINT32	j;
				IMG_PCHAR	pszLabelName;
				IMG_UINT32	uLabelAddress;

				pszLabelName = g_psModules[uModule].psExport[uExport].pszName;
				uLabelAddress = g_psModules[uModule].psExport[uExport].uAddress;
				uLabelAddress += (g_psModules[uModule].uOutputOffset / EURASIA_USE_INSTRUCTION_SIZE);

				pszDefineName = UseAsm_Malloc(strlen(pszLabelPrefix) + strlen(pszLabelName) + strlen(pszLabelPostfix) + 1);
	
				strcpy(pszDefineName, pszLabelPrefix);
				strcat(pszDefineName, pszLabelName);
				strcat(pszDefineName, pszLabelPostfix);

				for (j = 0; j < strlen(pszDefineName); j++)
				{
					pszDefineName[j] = (IMG_CHAR)toupper(pszDefineName[j]);
				}

				if (fprintf(fLabelHeaderFile, 
							"#define %-*s\t0x%.8X\n\n", 
							(int)uMaxLabelNameLength, 
							pszDefineName, 
							uLabelAddress << 3) < 0)
				{
					fprintf(stderr, "Couldn't write label header file: %s.\n", strerror(errno));
					fclose(fLabelHeaderFile);
					return 1;
				}

				UseAsm_Free(pszDefineName);
			}
		}

		/*
			Write out the maximum HW temp register used for named registers allocated by the
			assembler.

			NB: If no named registers are used by any modules, then this definition will not be
				output.
		*/
		if	(uMaxNamedTempRegIdx != USE_UNDEF)
		{
			static IMG_CHAR pszDefinePostfix[] = "NAMED_TEMP_MAX";
			IMG_PCHAR		pszDefineName;
			IMG_UINT32		uDefineStrLen;
			IMG_UINT32		i;
			IMG_UINT32		uNameIndent;

			uDefineStrLen = strlen(pszLabelPrefix) + strlen(pszDefinePostfix) + 1;
			pszDefineName = UseAsm_Malloc(uDefineStrLen);
			if	(!pszDefineName)
			{
				fprintf(stderr, "%s: Out of memory.\n", argv[0]);
				return 1;
			}

			strcpy(pszDefineName, pszLabelPrefix);
			strcat(pszDefineName, pszDefinePostfix);

			for (i = 0; i < uDefineStrLen; i++)
			{
				pszDefineName[i] = (IMG_CHAR)toupper(pszDefineName[i]);
			}

			uNameIndent = max(uDefineStrLen, uMaxLabelNameLength);

			if	(fprintf(fLabelHeaderFile, 
						 "#define %-*s\t0x%.8X\n\n", 
						 (int)uNameIndent, 
						 pszDefineName, 
						 uMaxNamedTempRegIdx) < 0)
			{
				fprintf(stderr, "Couldn't write label header file: %s.\n", strerror(errno));
				fclose(fLabelHeaderFile);
				return 1;
			}

			free(pszDefineName);
		}

		fclose(fLabelHeaderFile);
	}

	return 0;
}
/* EOF */
