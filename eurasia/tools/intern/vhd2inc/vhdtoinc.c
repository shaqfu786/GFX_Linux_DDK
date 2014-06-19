/**************************************************************************
* Name         : vhd2inc.c
* Title        : .VHD to header file translator
* Author       : Marc Stevens
* Created      : 8th March 1997
*
* Copyright    : 1997-2006 by Imagination Technologies Limited. All rights reserved.
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
* Description  : .VHD to header file translator
*
*
*
**************************************************************************/
#include <stdio.h>
#include <string.h>

#ifdef LINUX
#include <errno.h>
/* need errno for debugging */
#define ERRORSTRING strerror(errno)		/* PRQA S 5119 */
#else
#define ERRORSTRING _strerror("")
#endif

/* QAC leave main using basic types */
/* PRQA S 5013 ++ */
#if !defined(LINUX)
int _cdecl main (int argc, char **argv)
#else
int main (int argc, char **argv)
#endif
{
	FILE *pInFile;
	FILE *pOutFile;
	int j;
	int In;
	unsigned char byB;
	unsigned int dwRomData;
	unsigned int dwRomAddr;
	unsigned int dwRom;
	int dwArgStart = 1;
	unsigned int dwBinary = 0;
	char *pszArrayType;

	if (argc >= 2 && strcmp(argv[1], "-b") == 0)
	{
		dwBinary = 1;
		dwArgStart++;
	}

	if (argc < (dwArgStart + 1) || !argv[dwArgStart + 0])
	{
		printf("Need source file !\n");

		return(1);
	}

	if (argc < (dwArgStart + 2) || !argv[dwArgStart + 1])
	{
		printf("Need destination file !\n");

		return(1);
	}

	if (argc < (dwArgStart + 3) || !argv[dwArgStart + 2])
	{
		printf("Need array name !\n");

		return(1);
	}

	if (argc < (dwArgStart + 4) || !argv[dwArgStart + 3])
	{
		pszArrayType = "BYTE";
	}
	else
	{
		pszArrayType = argv[dwArgStart + 3];
	}

	pInFile = fopen(argv[dwArgStart + 0],"rb");
	pOutFile = fopen(argv[dwArgStart + 1],"w");

	if (!pInFile)
	{
		printf("Couldn't create file %s: %s", argv[dwArgStart + 0], ERRORSTRING);

		return(1);
	}

	if (!pOutFile)
	{
		printf("Couldn't create file %s: %s", argv[dwArgStart + 1], ERRORSTRING);

		return(1);
	}

    In = 0;
	dwRomData = 0;
	dwRomAddr = 0;
	j = 0;

	fprintf(pOutFile,"%s pb%s[] =\n{", pszArrayType, argv[dwArgStart + 2]);

	/* Microsoft macro QAC override */						   
	while (feof(pInFile) == 0)	/* PRQA S 4130 */
	{
		if (dwBinary)
		{
			if ((j % 8) == 0)
			{
				fprintf(pOutFile, "\n\t");
			}
			if (fread(&byB,1,1,pInFile) != 1)
			{
				break;
			}
			fprintf(pOutFile, "0x%.2X,", byB);
			j++;
		}
		else
		{
			fread(&In,1,1,pInFile);
			if (In == (int)'"')
			{
				for (j = 25; j >=0; --j)
				{
					fread(&In,1,1,pInFile);
					dwRom = (In == (int)'1') ? 1UL : 0UL;
					dwRom <<= j;
					dwRomData |= dwRom;
				}
				fread(&In,1,1,pInFile); /* read terminating " */
				for (j = 0; j < 4; j++)
				{
					In = (int)(dwRomData & 0xFF);
					dwRomData >>= 8;
					fprintf(pOutFile,"0x%2.2X,",In);
				}
				dwRom = dwRomAddr;
				for (j = 0; j < 4; j++)
				{
					In = (int)(dwRom & 0xFF);
					dwRom >>= 8;
					fprintf(pOutFile,"0x%2.2X,",In);
				}
				dwRomAddr++;
				fprintf(pOutFile,"\n\t");
			}
		}
	}

	if (dwBinary)
	{
		fprintf(pOutFile,"\n};\n\n");
	}
	else
	{
		fprintf(pOutFile,"0x00\n};\n\n");
	}

	fclose(pInFile);
	fclose(pOutFile);

	return(0);
}
