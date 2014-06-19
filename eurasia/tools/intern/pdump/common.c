/******************************************************************************
 Name          : COMMON.C

 Title         : PDump common stuff...

 C Author      : John Howson

 Created       : 4/11/97

 Copyright     : 1997-2007 by Imagination Technologies Limited.
                 All rights reserved. No part of this software, either material
                 or conceptual may be copied or distributed, transmitted,
                 transcribed, stored in a retrieval system or translated into
                 any human or computer language in any form by any means,
                 electronic, mechanical, manual or otherwise, or disclosed
                 to third parties without the express written permission of
                 Imagination Technologies Limited, Home Park Estate,
                 Kings Langley, Hertfordshire, WD4 8LZ, U.K.

 Description   :

 Program Type  : 32-bit DLL

 Version       : $Revision: 1.22 $

 Modifications :
 $Log: common.c $
******************************************************************************/

#if !defined(LINUX)
#pragma  warning(disable:4201)
#pragma  warning(disable:4214)
#pragma  warning(disable:4115)
#pragma  warning(disable:4514)


#include <windows.h>
#include <windowsx.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "img_types.h"

#include "common.h"
#include "event.h"

IMG_CHAR	szOutFileName[256];
IMG_UINT32	ui32Start = 10;
IMG_UINT32	ui32Stop = 10;
IMG_UINT32	ui32SampleRate = 1;
IMG_BOOL	bMultiFrames = IMG_FALSE;
IMG_BOOL	bSampleRatePresent = IMG_FALSE;
IMG_BOOL	bNoDAC = IMG_FALSE;
IMG_BOOL	bNoREF = IMG_FALSE;

PFN_WRITENPIXPAIRS	pfnWriteNPixelPairs = 0;
PFN_REGWRITE		pfnRegWrite = 0;
PFN_WRITEBMP		pfnWriteBMP = 0;


#if !defined(LINUX)

/*****************************************************************************
 FUNCTION	: KillPdump

 PURPOSE	:

 PARAMETERS	:

 RETURNS	:
*****************************************************************************/
static void KillPdump(void)
{
	HANDLE	hEvent;
	HANDLE	hExitDoneEvent;

	/*
		Get a handle on the pdump finished event
	*/
	if ((hExitDoneEvent=OpenEvent(EVENT_MODIFY_STATE, IMG_FALSE, PDUMP_EXITDONE_EVENT_NAME)) == NULL)
	{
		printf("Failed to open pdump exit done event, are you sure pdump is running?\n");
		return;
	}
	/*
		Gen a kill pdump event
	*/
	if ((hEvent=OpenEvent(EVENT_MODIFY_STATE, IMG_FALSE, PDUMP_EXIT_EVENT_NAME)) == NULL)
	{
		printf("Failed to open pdump exit event, are you sure pdump is running?\n");
		return;
	}
	
	if (SetEvent(hEvent) == IMG_FALSE)
	{
		printf("Failed to set pdump exit event!\n");
		return;
	}
	
	printf("Waiting for pdump to exit.....\n");
	WaitForSingleObject(hExitDoneEvent, INFINITE);

	CloseHandle(hEvent);
	CloseHandle(hExitDoneEvent);
	return;
}

/*****************************************************************************
 FUNCTION	: ConnectToSim

 PURPOSE	:

 PARAMETERS	:

 RETURNS	:
*****************************************************************************/
IMG_BOOL ConnectToSim(void)
{
	HANDLE hSimLib = LoadLibrary(TEXT("SGX3DSIM.DLL"));

	if (!hSimLib)
	{
		printf("Failed to load sim library\n");

		return(IMG_FALSE);
	}

	pfnWriteNPixelPairs = (PFN_WRITENPIXPAIRS) GetProcAddress(hSimLib,TEXT("WriteNPixelPairs"));
	pfnRegWrite = (PFN_REGWRITE) GetProcAddress(hSimLib,TEXT("RegWrite"));
	pfnWriteBMP = (PFN_WRITEBMP) GetProcAddress(hSimLib,TEXT("WriteBMP"));

	if 	(
			!(
				pfnWriteNPixelPairs &&
				pfnRegWrite &&
				pfnWriteBMP
			)
		)
	{
		printf("Couldn't get FN address(s)...\n");

		return(IMG_FALSE);
	}

	return(IMG_TRUE);
}
#endif

/*****************************************************************************
 FUNCTION	: ParseInputParams

 PURPOSE	:

 PARAMETERS	:

 RETURNS	:
*****************************************************************************/
IMG_BOOL ParseInputParams(IMG_INT nNumParams,IMG_CHAR * *ppszParams)
{
	IMG_UINT32 	i;

	strcpy(szOutFileName,"out");

	for(i=1;i<(IMG_UINT32) nNumParams;i++)
	{
		IMG_CHAR c1;
		IMG_CHAR c2;

		c1 = ppszParams[i][0];
		c2 = ppszParams[i][1];

		if ((c1 != '-') && (c1 != '/'))
		{
			printf("Invalid params...\n");
			return(IMG_FALSE);
		}

		#if !defined(LINUX)
		if (strcmp("kill", &ppszParams[i][1]) == 0)
		{
			KillPdump();
			return IMG_FALSE;
		}
		#endif

		/*
			Stream disable options
		*/
		if (strcmp("nodac", &ppszParams[i][1]) == 0)
		{
			bNoDAC = IMG_TRUE;
			continue;
		}
		if (strcmp("noref", &ppszParams[i][1]) == 0)
		{
			bNoREF = IMG_TRUE;
			continue;
		}

		switch (c2)
		{
			case 'f':
			{
				switch (ppszParams[i][2])
				{
					case 'o':
					{
						strcpy(szOutFileName,&ppszParams[i][3]);
						break;
					}
				}

				break;
			}
			case 'c':
			{
				switch (ppszParams[i][2])
				{
					case 'm':
					{
						bMultiFrames = IMG_TRUE;
						break;
					}
					default:
					{
						IMG_CHAR szTemp[32];
						IMG_UINT32 	j,p2;
						IMG_CHAR *	pszParam;

						pszParam = &ppszParams[i][2];

						/*
							Read start and stop frames.
						*/
						j = 0;
						while ((pszParam[j] != '-') && (pszParam[j] != '\0'))
						{
							szTemp[j] = pszParam[j];
							j++;
						}

						szTemp[j] = 0;
						ui32Start = (IMG_UINT32)atoi(szTemp);

						p2 = j;
						j = 0;
						if (pszParam[p2+j] == '-')
						{
							/*
								Skip - character.
							*/
							p2 ++;
						}

						while ((pszParam[p2+j] != ' ') && (pszParam[p2+j] != '\0'))
						{
							szTemp[j] = pszParam[p2+j];
							j++;
						}

						szTemp[j] = 0;

						if (szTemp[0] == 0)
						{
							ui32Stop = ui32Start;
						}
						else
						{
							ui32Stop = (IMG_UINT32)atoi(szTemp);
						}

						break;
					}
				}

				break;
			}
			case 's':
			{
				IMG_CHAR szTemp[32];
				IMG_UINT32 j;
				IMG_CHAR *	pszParam;

				switch (ppszParams[i][2])
				{
					case 'r':
					{
						pszParam = &ppszParams[i][3];

						/*
							Read frame sample skip...
						*/
						j = 0;

						while ((pszParam[j] != ' ') && (pszParam[j] != '\0'))
						{
							szTemp[j] = pszParam[j];
							j++;
						}

						szTemp[j] = 0;

						if (szTemp[0] != 0)
						{
							ui32SampleRate = (IMG_UINT)atoi(szTemp);
							bSampleRatePresent = IMG_TRUE;
						}

						break;
					}
				}

				break;
			}
			case 'h':
			case '?':	/* Frequently used DOS query parameter */
			{
				printf("Command line options...\n\n");
				printf("  -h Displays this help.\n");
				printf("  -c<start frame[-stop frame]>, start and stop frames for param capture.\n");
				printf("    m - Hotkey parameter capture.\n");
				printf("  Note: in order for pdump to shut-down automatically, some platforms require the\n");
				printf("  target application to generate at least one more frame than is being captured.\n");
				printf("  E.G. for 'pdump -c0-3', the application would need to generate at least 5 frames.\n");
				printf("  -fo<file name> Output file name excluding extension.\n");
				printf("  -sr<Sample rate> Specifies rate at which to grab parameters:\n");
				printf("  -kill to close pdump down\n");
				printf("  -nodac Disables DAC stream.\n");
				printf("  -noref Disables REF stream.\n");
				printf("\n");

				return(IMG_FALSE);
			}
		}
	}

	return(IMG_TRUE);
}

/*****************************************************************************
 End of file (COMMON.C)
*****************************************************************************/

