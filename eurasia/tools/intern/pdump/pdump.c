/******************************************************************************
 Name			: PDUMP.C

 Title			: Parameter dumping tools

 C Author		: John Howson

 Created		: 3/5/98

 Copyright		: Copyright 1998-2009 by Imagination Technologies Limited.
				  All rights reserved. No part of this software, either
				  material or conceptual may be copied or distributed,
				  transmitted, transcribed, stored in a retrieval system or
				  translated into any human or computer language in any form
				  by any means, electronic, mechanical, manual or otherwise,
				  or disclosed to third parties without the express written
				  permission of Imagination Technologies Limited,
				  Home Park Estate, Kings Langley, Hertfordshire,
				  WD4 8LZ, U.K.

 Description	:

 Program Type	: 32-bit DLL

 Version		: $Revision: 1.71 $

 Modifications	:
 $Log: pdump.c $
******************************************************************************/

#include "img_types.h"

#if !defined(LINUX)

#pragma warning(push)
#pragma warning(disable:4201)
#include <windows.h>
#include <winioctl.h>
#include <windowsx.h>
#include <conio.h>

#pragma warning(pop)

/* QAC override as XP link fails without this define */
#define unlink _unlink		/* PRQA S 3436 */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#if defined(LINUX)
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>

typedef struct termios _termios;
_termios gOldTerm;

IMG_INT _kbhit(IMG_VOID);
IMG_INT _getch(IMG_VOID);
IMG_VOID restoreTerminal(IMG_VOID);

#define OutputDebugString(a)	fputs((a), stderr)
/* QAC override as errno is usefull for debugging */
#define OutputErrorString(str)	fprintf(stderr, "%s: Error %d: %s\n", str, errno, strerror(errno))	/* PRQA S 5119 */

#else
#define OutputErrorString(str)
#endif /* #if defined(LINUX) */


#include "dbgdrvif.h"
#include "umdbgdrv.h"
#include "common.h"
#include "event.h"


#define PARAM_PAGES		256
#define SCRIPT_PAGES	64
#define MEM_PAGES		3280

// this should give us enough room for a 1280x1024x32 bpp display.
typedef struct _PARAM_STREAM_
{
	IMG_UINT32 ui32Flags;
	IMG_UINT32 ui32Pages;
	IMG_UINT32 ui32Mode;
	IMG_CHAR * pszFormat;
	IMG_CHAR * pszFormat2;
	IMG_CHAR * pszName;
	IMG_SID    hStream;
	IMG_UINT8 *pui8Buffer;
	FILE     * pFile;
	IMG_UINT32 ui32FileNum;
	IMG_UINT32 ui32FilePos;
} PARAM_STREAM, *PPARAM_STREAM;

#define PSTREAM_FLAGS_DUMPASTEXT	0x0001
#define PSTREAM_FLAGS_KEYSTREAM		0x0002
#define PSTREAM_FLAGS_ONCEONLY		0x0004

/*****************************************************************************
 Parameter streams to be handled...

 Note, script stream MUST be stay as first stream in array.
*****************************************************************************/
#define PDUMP_STREAM_SCRIPT2		0
#define PDUMP_STREAM_PARAM2			1
#define PDUMP_STREAM_DRIVERINFO		2
#define PDUMP_NUM_STREAMS			3

PARAM_STREAM psPS[PDUMP_NUM_STREAMS] =
{
	{
		PSTREAM_FLAGS_DUMPASTEXT | PSTREAM_FLAGS_ONCEONLY,
		SCRIPT_PAGES,
		DEBUG_CAPMODE_FRAMED,
		"%s2.txt",
		"%s2_%u.txt",
		"ScriptStream2",
		0,
		0,
		0,
		0,
		0
	},
	{
		PSTREAM_FLAGS_KEYSTREAM,
		PARAM_PAGES,
		DEBUG_CAPMODE_FRAMED,
		"%s2.prm",
		"%s2_%u.prm",
		"ParamStream2",
		0,
		0,
		0,
		0,
		0
	},
	{
		PSTREAM_FLAGS_DUMPASTEXT,
		SCRIPT_PAGES,
		DEBUG_CAPMODE_FRAMED,
		"%s_drvinfo.txt",
		"%s%u_drvinfo.txt",
		"DriverInfoStream",
		0,
		0,
		0,
		0,
		0
	}
};

#define NUM_PARAM_STREAMS	(sizeof(psPS)/sizeof(PARAM_STREAM))


static IMG_UINT8	g_ui8FinalFrameBuff[1024];
IMG_BOOL		bGoing;

/* in Linux we do killing with the kill program */
#if !defined(LINUX)
IMG_BOOL FAR PASCAL Makeslot();
IMG_BOOL WINAPI Readslot();
HANDLE hSlot1;
#endif

static IMG_VOID ReadFinalFrameDump(IMG_UINTPTR_T uiHandle)
{
	PPARAM_STREAM	psParamStream;
	IMG_UINT32		ui32BytesRead;

	/* Check for last frame data in script stream */
	psParamStream = &psPS[0];

	ui32BytesRead = DBGDRVReadLastFrame(uiHandle, psParamStream->hStream, g_ui8FinalFrameBuff, 1024);

	/*
		Write to the output file
	*/
	fwrite(g_ui8FinalFrameBuff, 1, ui32BytesRead, psParamStream->pFile);
	psParamStream->ui32FilePos += ui32BytesRead;

	/* Check for last frame data in no MMU script stream */
	psParamStream = &psPS[1];

	ui32BytesRead = DBGDRVReadLastFrame(uiHandle, psParamStream->hStream, g_ui8FinalFrameBuff, 1024);


	/*
		Write to the output file
	*/
	fwrite(g_ui8FinalFrameBuff, 1, ui32BytesRead, psParamStream->pFile);
	psParamStream->ui32FilePos += ui32BytesRead;

	/* Check for last frame data in script2 stream */
	psParamStream = &psPS[2];

	/*
		Set input params
	*/
	ui32BytesRead = DBGDRVReadLastFrame(uiHandle, psParamStream->hStream, g_ui8FinalFrameBuff, 1024);

	/*
		Write to the output file
	*/
	fwrite(g_ui8FinalFrameBuff, 1, ui32BytesRead, psParamStream->pFile);
	psParamStream->ui32FilePos += ui32BytesRead;
}

/*****************************************************************************
 FUNCTION	: ReadStream

 PURPOSE	: Reads from the stream into a buffer and writes to a file

 PARAMETERS	: uiHandle, psParamStream

 RETURNS	: ui32BytesRead
*****************************************************************************/
static IMG_UINT32 ReadStream(IMG_UINTPTR_T	uiHandle,
							PPARAM_STREAM	psParamStream,
							IMG_BOOL bReadInitBuffer)
{
	IMG_UINT32	ui32BytesRead;
	IMG_UINT32	ui32Marker;
	IMG_UINT32	ui32BytesReadBeforeMarker;
	IMG_UINT32	ui32BytesReadAfterMarker;
	IMG_CHAR	szFileName	[256];

	/*
		Read from the stream into a buffer
	*/
	ui32BytesRead = DBGDRVReadBIN(uiHandle, psParamStream->hStream, bReadInitBuffer, psParamStream->pui8Buffer, psParamStream->ui32Pages * 4096);

	if (ui32BytesRead != 0)
	{
		/*
			Get the stream marker to decide whether to split output files
		*/
		ui32Marker = DBGDRVGetStreamMarker(uiHandle, psParamStream->hStream);

		if ((ui32Marker != 0) && (psParamStream->ui32FilePos + ui32BytesRead > ui32Marker))
		{
			/*
				Reset the stream marker to zero
			*/
			DBGDRVSetStreamMarker(uiHandle, psParamStream->hStream, 0);

			/*
				Determine the split of bytes before and after the stream marker
			*/
			ui32BytesReadBeforeMarker	= ui32Marker - psParamStream->ui32FilePos;
			ui32BytesReadAfterMarker	= psParamStream->ui32FilePos + ui32BytesRead - ui32Marker;

			/*
				Write the bytes before the stream marker to the old output file
			*/
			fwrite(psParamStream->pui8Buffer, 1, ui32BytesReadBeforeMarker, psParamStream->pFile);

			/*
				Close the old output file
			*/
			fclose(psParamStream->pFile);

			/*
				Open the new output file
			*/
			psParamStream->ui32FileNum++;
			sprintf(szFileName, psParamStream->pszFormat2, szOutFileName, psParamStream->ui32FileNum);
			psParamStream->pFile = fopen(szFileName, "wb");

			/*
				Write the bytes after the stream marker to the new output file
			*/
			fwrite(psParamStream->pui8Buffer + ui32BytesReadBeforeMarker, 1, ui32BytesReadAfterMarker, psParamStream->pFile);
			psParamStream->ui32FilePos = ui32BytesReadAfterMarker;
		}
		else
		{
			/*
				Write the bytes to the output file
			*/
			fwrite(psParamStream->pui8Buffer, 1, ui32BytesRead, psParamStream->pFile);
			fflush(psParamStream->pFile);

			psParamStream->ui32FilePos += ui32BytesRead;
		}
	}

	return ui32BytesRead;
}

#if defined(LINUX)
/*****************************************************************************
  Exit the program nicely on ctrl+c. 
*****************************************************************************/
static IMG_VOID exit_program(IMG_INT sig)
{
	PVR_UNREFERENCED_PARAMETER(sig);

	(void) signal(SIGINT, SIG_DFL);

	printf("Shutting down program nicely...  Press again to force immediate exit\n");

	restoreTerminal();  /* If the user does press ctrl+c a second time, make sure the terminal is at least restored back to normal */

	bGoing = IMG_FALSE;
}
#endif

/*****************************************************************************
 Function			: main

 Description		: 'C' main

 Parameters			:

 Globals effected	:

 Return				:
*****************************************************************************/
#if !defined(LINUX)
IMG_INT __cdecl main (IMG_INT nNumParams, IMG_CHAR **ppszParams)
#else
IMG_INT main (IMG_INT nNumParams, IMG_CHAR **ppszParams)
#endif
{
	IMG_CHAR		szFName[256];
	IMG_UINTPTR_T	uiHandle;
	IMG_UINT32		ui32BytesRead;
	IMG_UINT32		ui32HadData;
	IMG_UINT32		i;
#if !defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
	IMG_BOOL		bUseSleep;
#endif

#if !defined(LINUX)
	HANDLE			hStartupEvent;
	HANDLE			hExitEvent;
	HANDLE			hExitDoneEvent;
#if !defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
	OSVERSIONINFO	VersionInformation;
#endif
#endif

	bGoing = IMG_TRUE;

	if (!ParseInputParams(nNumParams,ppszParams))
	{
		return -1;
	}

	if (DBGDRVConnect(&uiHandle) != DBGDRV_OK)
	{
		printf("Unable to connect to debug driver\nEnsure you are using the latest pdump and debug driver\n");
		return -1;
	}

#if !defined(LINUX)
	/* Create the exit events */
	if ((hStartupEvent=CreateEvent(NULL, IMG_TRUE, IMG_FALSE, PDUMP_STARTUP_EVENT_NAME)) == NULL)
	{
		printf("Failed to create Startup event in PDUMP\n");
		return -1;
	}
	if ((hStartupEvent=OpenEvent(EVENT_MODIFY_STATE, IMG_FALSE, PDUMP_STARTUP_EVENT_NAME)) == NULL)
	{
		printf("Failed Open Startup Event in PDUMP\n");
		CloseHandle(hStartupEvent);
		return -1;
	}
	if ((hExitEvent=CreateEvent(NULL, IMG_FALSE, IMG_FALSE, PDUMP_EXIT_EVENT_NAME)) == NULL)
	{
		printf("Failed to create Exit event in PDUMP\n");
		return -1;
	}
	if ((hExitDoneEvent=CreateEvent(NULL, IMG_FALSE, IMG_FALSE, PDUMP_EXITDONE_EVENT_NAME)) == NULL)
	{
		printf("Failed to create ExitDone event in PDUMP\n");
		return -1;
	}
#else
#endif

#if defined(LINUX)
	(void) signal(SIGINT, exit_program);
#endif

#if !defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
#if !defined(LINUX)
	/* Find out if we are running on Win9x or NT/Win2k */
	VersionInformation.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	if(GetVersionEx(&VersionInformation))
	{
		/* The operation system is Windows NT */
		if(VersionInformation.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
			bUseSleep=IMG_FALSE;
		}
		else
		{
			bUseSleep=IMG_TRUE;
		}
	}
	else
	{
		printf("Could not determine if we are running on Win9x or NT/Win2k\n");
		exit(-1);		/* PRQA S 5126 */ /* exit call is needed */
	}
#else
	bUseSleep = IMG_TRUE;
#endif
#endif

	/*
		Create parameter streams.
	*/
	for (i=0;i<PDUMP_NUM_STREAMS;i++)
	{
		/* Find stream */
		psPS[i].hStream = DBGDRVFindStream(uiHandle, psPS[i].pszName, IMG_TRUE);

		if (!psPS[i].hStream)
		{
			printf("Failed to find param stream %s!\nEnsure you are using the latest pdump and debug driver\n",psPS[i].pszName);

			goto Cleanup;
		}

		/*
			Create local buffer.
		*/

		psPS[i].pui8Buffer = malloc(psPS[i].ui32Pages * 4096);


		if (!psPS[i].pui8Buffer)
		{
			printf("Failed to create local param buffer for stream %s!\n",psPS[i].pszName);

			goto Cleanup;
		}

		/*
			Get the name for the output file for this stream.
		*/
		sprintf(szFName,psPS[i].pszFormat,szOutFileName);

		/*
			Open the files.
		*/
		psPS[i].pFile = fopen(szFName,"wb");

		if (!psPS[i].pFile)
		{
			printf("Failed to create out file for stream %s!\n",psPS[i].pszName);

			goto Cleanup;
		}

		/*
			Set debug output mode, i.e enable buffering only.
			If you want monochrome and/or winice output just OR in the
			additional DEBUG_OUTMODE_XXXX define.
		*/
		DBGDRVSetOutputMode(uiHandle,psPS[i].hStream,DEBUG_OUTMODE_STREAMENABLE);

		/*
			Set buffer debug level.
		*/
		DBGDRVSetDebugLevel(uiHandle,psPS[i].hStream,DEBUG_LEVEL_1);

		/*
			Setup hotkey combination.
		*/
		if (bMultiFrames)
		{
			IMG_UINT32	ui32CapMode;
			/*
				Setup capture mode.
			*/
			ui32CapMode = DEBUG_CAPMODE_HOTKEY;
			if (bSampleRatePresent)
			{
				ui32CapMode |= DEBUG_CAPMODE_FRAMED;
			}
			DBGDRVSetCaptureMode(uiHandle,psPS[i].hStream,ui32CapMode,0xFFFFFFFF,0xFFFFFFFF,ui32SampleRate);
		}
		else
		{
			/*
				Setup capture mode.
			*/
			DBGDRVSetCaptureMode(uiHandle,psPS[i].hStream,psPS[i].ui32Mode,ui32Start,ui32Stop,ui32SampleRate);
		}

		DBGDRVSetStreamFrame(uiHandle,psPS[i].hStream,0);
	}

	if (bMultiFrames)
	{
		printf("Multi-frame/hotkey capture mode. You have to press 'x' to exit.\n");
		if (bSampleRatePresent)
		{
			printf("Sample rate = %u\n",ui32SampleRate);
			printf("Samples will be captured between hotkey presses.\n");
		}
	}
	else
	{
		printf("Start = %u, Stop = %u, Sample rate = %u\n",ui32Start,ui32Stop,ui32SampleRate);
	}

	/*
		Initialisation is complete. Tell everything about it...
	*/
#if !defined(LINUX)
	if (SetEvent(hStartupEvent) == IMG_FALSE)
	{
		printf("Failed to set PDUMP Startup event!\n");
		goto Cleanup;
	}
	Makeslot();
#else
#endif
	/*
		Read and write captured data to file.
	*/
	while (bGoing && (bMultiFrames || (DBGDRVGetStreamFrame(uiHandle,psPS[0].hStream) <= ui32Stop)))
	{
		IMG_UINT32 ui32ReadInLoop = 0;

		for (i = 0; i < NUM_PARAM_STREAMS; i++)
		{
			ui32BytesRead = ReadStream(uiHandle, &psPS[i], IMG_TRUE);

			ui32ReadInLoop += ui32BytesRead;

			ui32BytesRead = ReadStream(uiHandle, &psPS[i], IMG_FALSE);

			ui32ReadInLoop += ui32BytesRead;
		}


		#if !defined(LINUX)
		if (WaitForSingleObject(hExitEvent, 0) != WAIT_TIMEOUT)
		{
			bGoing = IMG_FALSE;
		}
		#endif /* #ifndef LINUX */

		if (_kbhit())
		{
			if (_getch() == 'x')
			{
				bGoing = IMG_FALSE;
			}
		}

		#if !defined(LINUX)
		if(!Readslot())
		{
			bGoing = IMG_FALSE;
		}
		else
		{
			#if !defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
			/*
				If we didn't read any data give up time slot.
			*/
			if (ui32ReadInLoop == 0)
			{
				if (bUseSleep)
				{
					Sleep(0);
				}
				else
				{
					SwitchToThread();
				}
			#else
			if (!ui32ReadInLoop && bGoing)
			{
				DBGDRVWaitForEvent(uiHandle, DBGDRV_EVENT_STREAM_DATA);
			}
			#endif
			}
		}
		#else	/* !defined(LINUX) && !defined(__SYMBIAN32__) */
		#if !defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
		#else	/* !defined(SUPPORT_DBGDRV_EVENT_OBJECTS) */
		if (!ui32ReadInLoop && bGoing)
		{
			DBGDRVWaitForEvent(uiHandle, DBGDRV_EVENT_STREAM_DATA);
		}
		#endif	/* !defined(SUPPORT_DBGDRV_EVENT_OBJECTS) */
		#endif	/* !defined(LINUX) && !defined(__SYMBIAN32__) */
	}

	/*
		Flush buffer(s)..
	*/
	do
	{
		#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
		IMG_UINT32 ui32Pass;
		#endif

		ui32HadData = 0;

		#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
		/*
		 * If no data is read on the first pass, wait for data, and
		 * continue to the second pass.  If there is still no data
		 * on the second pass, assume the streams have been flushed.
		 */
		for (ui32Pass = 0; ui32Pass < 2; ui32Pass++)
		#endif
		{
			for (i = 0; i < NUM_PARAM_STREAMS; i++)
			{
				ui32BytesRead = ReadStream(uiHandle, &psPS[i], IMG_TRUE);
				ui32HadData |= ui32BytesRead;


				ui32BytesRead = ReadStream(uiHandle, &psPS[i], IMG_FALSE);
				ui32HadData |= ui32BytesRead;
			}

			if (_kbhit())
			{
				if (_getch() == 'x')
				{
					bGoing = IMG_FALSE;
				}
			}

			#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
			if (ui32HadData || !bGoing)
			{
				break;
			}
			else
			{
				/* Only wait on the first pass */
				if (!ui32Pass)
				{
					DBGDRVWaitForEvent(uiHandle, DBGDRV_EVENT_STREAM_DATA);
				}
			}
			#endif
		}
	}
	while ((ui32HadData != 0) && bGoing);

	/*
		Get last frame buffer contents
	*/
	ReadFinalFrameDump(uiHandle);

	/*
		Kill everything off...
	*/
	Cleanup:

	for (i=0;i<NUM_PARAM_STREAMS;i++)
	{
		/*
			Reset the sample mode to the default.
		*/
		DBGDRVSetCaptureMode(uiHandle, psPS[i].hStream,DEBUG_CAPMODE_FRAMED,0xFFFFFFFF,0xFFFFFFFF,0);

		/*
			Close file.
		*/
		if (psPS[i].pFile)
		{
			fclose(psPS[i].pFile);
		}

		/*
			Free buffer..
		*/
		if (psPS[i].pui8Buffer)
		{

			free(psPS[i].pui8Buffer);
		}
	}

	/*
		Postprocess the script files to remove all but
		the last of last-only blocks.
	*/
	for (i=0;i<NUM_PARAM_STREAMS; i++)
	{
		if (psPS[i].ui32Flags & PSTREAM_FLAGS_ONCEONLY)
		{
			FILE* pFile;
			IMG_CHAR pszLine[256];
#if !defined(PDUMP_DEBUG_OUTFILES)
			static const IMG_CHAR pszBlockOpen[] = "-- LASTONLY{";
			static const IMG_CHAR pszBlockClose[] = "-- }LASTONLY";
#else
			static const IMG_CHAR pszBlockOpen[] = "LASTONLY{";
			static const IMG_CHAR pszBlockClose[] = "}LASTONLY";
#endif
			IMG_INT32 i32BlockCount;
			IMG_CHAR pszMsg[256];

			sprintf(szFName,psPS[i].pszFormat,szOutFileName);
			pFile = fopen(szFName, "r+b");
			if (pFile == NULL)
			{
				sprintf(pszMsg, "Couldn't reopen file %s\n", szFName);
				OutputDebugString(pszMsg);
				OutputErrorString("fopen");
				break;
			}

			/*
				Count the number of last only blocks in the file.
			*/
			i32BlockCount = 0;
			for(;;)
			{
				if (!fgets(pszLine, sizeof(pszLine), pFile))
				{
					/* QAC override MS macro */
					if (ferror(pFile) != 0)		/* PRQA S  4130 */
					{
						sprintf(pszMsg, "Error reading input file for LASTONLY block counting\n");
						OutputDebugString(pszMsg);
						OutputErrorString("fgets");
					}
					else
					{
						/*
							Must be EOF
						*/
						/* QAC override MS macro */
						if (feof(pFile) == 0)		/* PRQA S 4130 */
						{
							sprintf(pszMsg, "Expected end of file on input file for LASTONLY block counting\n");
							OutputDebugString(pszMsg);
						}
					}
					break;
				}
#if !defined(PDUMP_DEBUG_OUTFILES)
				if (strncmp(pszLine, pszBlockOpen, strlen(pszBlockOpen)) == 0)
#else
				if (strstr(pszLine, pszBlockOpen) != 0)
#endif
				{
					i32BlockCount++;
				}
			}

			/*
				Strip out all but the last of these blocks.
			*/
			if (i32BlockCount > 1)
			{
				IMG_CHAR pszNewFileName[256];
				FILE* pNewFile;
				IMG_BOOL bOutput = IMG_TRUE;

				strcpy(pszNewFileName, szFName);
				strcat(pszNewFileName, "_");

				pNewFile = fopen(pszNewFileName, "wb");
				if (pNewFile == NULL)
				{
					sprintf(pszMsg, "Couldn't open temporary file %s\n", pszNewFileName);
					OutputDebugString(pszMsg);
					OutputErrorString("fopen");
					break;
				}

				rewind(pFile);
				for(;;)
				{
					if (!fgets(pszLine, sizeof(pszLine), pFile))
					{
						/* QAC override MS macro */
						if (ferror(pFile) != 0)		/* PRQA S 4130 */
						{
							sprintf(pszMsg, "Error reading input file for LASTONLY block stripping\n");
							OutputDebugString(pszMsg);
							OutputErrorString("fgets");
						}
						else
						{
							/*
								Must be EOF
							*/
							/* QAC override MS macro */
							if (feof(pFile) == 0)		/* PRQA S 4130 */
							{
								sprintf(pszMsg, "Expected end of file on input file for LASTONLY block stripping\n");
								OutputDebugString(pszMsg);
							}
						}
						break;
					}

					if (strlen(pszLine) == 0)
					{
						sprintf(pszMsg, "Invalid text file: unexpected zero length read\n");
						OutputDebugString(pszMsg);
						break;
					}

#if !defined(PDUMP_DEBUG_OUTFILES)
					if (strncmp(pszLine, pszBlockOpen, strlen(pszBlockOpen)) == 0)
#else
					if (strstr(pszLine, pszBlockOpen) != 0)
#endif
					{
						/* test for second to last Block */
						if (i32BlockCount > 1)
						{
							bOutput = IMG_FALSE;
						}
						i32BlockCount--;

						if (i32BlockCount > 0) continue;
					}
#if !defined(PDUMP_DEBUG_OUTFILES)
					else if (strncmp(pszLine, pszBlockClose, strlen(pszBlockClose)) == 0)
#else
					else if (strstr(pszLine, pszBlockClose) != 0)
#endif
					{
						bOutput = IMG_TRUE;
						if (i32BlockCount > 0) continue;
					}

					if ((bOutput) || (i32BlockCount == 0))
					{
						if (fwrite(pszLine, strlen(pszLine), 1, pNewFile) != 1)
						{
							sprintf(pszMsg, "Couldn't write to temporary file\n");
							OutputDebugString(pszMsg);
							/* QAC override MS macro */
							if (ferror(pNewFile) != 0)		/* PRQA S 4130 */
							{
								OutputErrorString("fwrite");
							}
							else
							{
								/* QAC override MS macro */
								if (feof(pNewFile) != 0)		/* PRQA S 4130 */
								{
									sprintf(pszMsg, "fwrite: Unexpected end of file\n");
									OutputDebugString(pszMsg);
								}
							}
							break;
						}
					}
				}
				fclose(pNewFile);
				fclose(pFile);

				unlink(szFName);
				rename(pszNewFileName, szFName);
			}
		}
	}

#if !defined(LINUX)
	OutputDebugString("PDUMP: Exiting\n");

	if (SetEvent(hExitDoneEvent) == IMG_FALSE)
	{
		printf("Failed to set pdump done exit event!\n");
	}

	CloseHandle(hExitEvent);
	CloseHandle(hExitDoneEvent);
	CloseHandle(hStartupEvent);
#else
	restoreTerminal();
#endif

	DBGDRVDisconnect(uiHandle);

	return 0;
}



#if !defined(LINUX)

IMG_BOOL FAR PASCAL Makeslot() 
{
	LPSTR lpszSlotName = "\\\\.\\mailslot\\killpdump"; 

	// The mailslot handle "hSlot1" is declared globally. 

	hSlot1 = CreateMailslot(lpszSlotName, 
		0,                             // no maximum message size
		MAILSLOT_WAIT_FOREVER,         /* PRQA S 0277 */ // no time-out for operations
		(LPSECURITY_ATTRIBUTES) NULL); // no security attributes

	if (hSlot1 == INVALID_HANDLE_VALUE) 
	{
		printf("Unable to create mailslot for some reason");
		return IMG_FALSE; 
	}
	printf("\nSyncronization Mail Slot opened OK\n");
	return IMG_TRUE;
}



IMG_BOOL WINAPI Readslot() 
{
	IMG_UINT32 cbMessage, cMessage, cbRead;
	IMG_BOOL fResult;
	LPSTR lpszBuffer;

	cbMessage = cMessage = cbRead = 0; 

	// Mailslot handle "hSlot1" is declared globally. 

	fResult = (IMG_BOOL)GetMailslotInfo(hSlot1, // mailslot handle
		(LPDWORD) NULL,               // no maximum message size
		(LPDWORD)&cbMessage,                   // size of next message
		(LPDWORD)&cMessage,                    // number of messages
		(LPDWORD) NULL);              // no read time-out

	if (!fResult)
	{
		printf("Error!");
		return IMG_FALSE;
	}

	/* Override MS #define */
	if (cbMessage == (IMG_UINT32)MAILSLOT_NO_MESSAGE)		/* PRQA S 0277 */
	{
		return IMG_TRUE;
	}

	while (cMessage != 0)  // retrieve all messages
	{
		// Allocate memory for the message.

		lpszBuffer = (LPSTR) GlobalAlloc(GPTR,cbMessage+1); 

		lpszBuffer[0] = '\0'; 

		fResult = (IMG_BOOL)ReadFile(hSlot1,
						   lpszBuffer,
						   cbMessage,
						   (LPDWORD)&cbRead,
						   (LPOVERLAPPED) NULL);

		if (!fResult)
		{
			GlobalFree((HGLOBAL) lpszBuffer);
			return IMG_FALSE;
		}


		// Display the message. 
		printf(lpszBuffer);

		GlobalFree((HGLOBAL) lpszBuffer);

		fResult = (IMG_BOOL)GetMailslotInfo(hSlot1,			// mailslot handle
								 (LPDWORD) NULL,	// no maximum message size 
								 (LPDWORD)&cbMessage,		// size of next message 
								 (LPDWORD)&cMessage,			// number of messages 
								 (LPDWORD) NULL);	// no read time-out 

		if (!fResult)
		{
			return IMG_FALSE; 
		}
	}
	return IMG_FALSE;
}

#else

#if defined(LINUX)
IMG_INT _kbhit()
{
	static IMG_BOOL initialized = IMG_FALSE;
	IMG_INT bytesWaiting;
	_termios term;

	if(!initialized) 
	{
		// Use termios to turn off line buffering and echoing
		tcgetattr(STDIN_FILENO, &gOldTerm);
		term = gOldTerm;

		term.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &term);
#if !defined(ANDROID)
		setbuf(stdin, NULL);
#endif

		initialized = IMG_TRUE;
	}

	ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);

	if(bytesWaiting > 0)
	{
		return IMG_TRUE;
	}

	return IMG_FALSE;
}

IMG_INT _getch()
{
	return (IMG_INT)getchar();
}

void restoreTerminal()
{
	/* Use termios to restore original terminal attributes */
	tcsetattr(STDIN_FILENO, TCSANOW, &gOldTerm);
}
#endif

#endif

/******************************************************************************
 End of file (PDUMP.C)
******************************************************************************/

