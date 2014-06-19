/*****************************************************************************
 File			pvr_debug.c

 Title			Debug Functionality

 Author			Imagination Technologies

 Copyright     	Copyright 2006 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either
                material or conceptual may be copied or distributed,
                transmitted, transcribed, stored in a retrieval system
                or translated into any human or computer language in any
                form by any means, electronic, mechanical, manual or
                other-wise, or disclosed to third parties without the
                express written permission of Imagination Technologies
                Limited, Unit 8, HomePark Industrial Estate,
                King's Langley, Hertfordshire, WD4 8LZ, U.K.

 Description	Provides client side Debug Functionality

 $Log: pvr_debug.c $


  --- Revision Logs Removed --- 
*****************************************************************************/

/* PRQA S 3332 4 */ /* ignore warning about using undefined macros */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "img_defs.h"
#include "img_types.h"
#include "pvr_debug.h"

#if defined(SUPPORT_ANDROID_PLATFORM) && \
   (defined(PVRSRV_NEED_PVR_DPF) || defined(PVRSRV_NEED_PVR_TRACE))
#define LOG_TAG "IMGSRV"
#include <cutils/log.h>
#ifndef ALOGE
#define ALOGE LOGE
#endif
#ifndef ALOGW
#define ALOGW LOGW
#endif
#ifndef ALOGI
#define ALOGI LOGI
#endif
#ifndef ALOGV
#define ALOGV LOGV
#endif
#ifndef ALOGD
#define ALOGD LOGD
#endif
#endif

#if defined(PVRSRV_NEED_PVR_DPF)

static IMG_UINT32 gPVRDebugLevel = (DBGPRIV_FATAL | DBGPRIV_ERROR | DBGPRIV_WARNING);

static IMG_INT bDefaultRead = 0;

static IMG_VOID PVRSRVInheritEnvironmentDebugLevel(IMG_VOID)
{
	if(!bDefaultRead)
	{
		if (getenv("PVRDebugLevel"))
		{
			gPVRDebugLevel = strtol(getenv("PVRDebugLevel"), NULL,0);

			printf("\nSetting Debug Level to 0x%x\n",(IMG_UINT)gPVRDebugLevel);
		}

		bDefaultRead = 1;
	}
}

#if !defined(SUPPORT_ANDROID_PLATFORM)

/*----------------------------------------------------------------------------
<function>
	FUNCTION   : PVRDebugPrintf
	PURPOSE    : To output a debug message to the user
	PARAMETERS : In : uDebugLevel - The current debug level
	             In : pszFile - The source file generating the message
	             In : uLine - The line of the source file
	             In : pszFormat - The message format string
	             In : ... - Zero or more arguments for use by the format string
	RETURNS    : None
</function>
------------------------------------------------------------------------------*/
IMG_EXPORT IMG_VOID PVRSRVDebugPrintf(IMG_UINT32 ui32DebugLevel,
								   const IMG_CHAR *pszFileName,
								   IMG_UINT32 ui32Line,
								   const IMG_CHAR *pszFormat,
								   ...)
{
	IMG_CHAR szBuffer[PVR_MAX_DEBUG_MESSAGE_LEN];
	IMG_CHAR *szBufferEnd = szBuffer;
	IMG_CHAR *szBufferLimit = szBuffer + sizeof(szBuffer) - 1;
	IMG_CHAR *pszLeafName;
	IMG_BOOL bTrace;
	va_list vaArgs;

	/* The Limit - End pointer arithmetic we're doing in snprintf
	   ensures that our buffer remains null terminated from this */
	*szBufferLimit = '\0';

	PVRSRVInheritEnvironmentDebugLevel();

	pszLeafName = (IMG_CHAR *)strrchr (pszFileName, '/');

	if (pszLeafName)
	{
		pszFileName = pszLeafName;
	}

	bTrace = (IMG_BOOL)(ui32DebugLevel & DBGPRIV_CALLTRACE) ? IMG_TRUE : IMG_FALSE;

	if (!(gPVRDebugLevel & ui32DebugLevel))
	{
		return;
	}

	/* Add in the level of warning */
	snprintf(szBufferEnd, szBufferLimit - szBufferEnd, "PVR:");
	szBufferEnd += strlen(szBufferEnd);
	if (bTrace == IMG_FALSE)
	{
		switch(ui32DebugLevel)
		{
			case DBGPRIV_FATAL:
			{
				snprintf(szBufferEnd, szBufferLimit - szBufferEnd, "(Fatal):");
				break;
			}

			case DBGPRIV_ERROR:
			{
				snprintf(szBufferEnd, szBufferLimit - szBufferEnd, "(Error):");
				break;
			}

			case DBGPRIV_WARNING:
			{
				snprintf(szBufferEnd, szBufferLimit - szBufferEnd, "(Warning):");
				break;
			}

			case DBGPRIV_MESSAGE:
			{
				snprintf(szBufferEnd, szBufferLimit - szBufferEnd, "(Message):");
				break;
			}

			case DBGPRIV_VERBOSE:
			{
				snprintf(szBufferEnd, szBufferLimit - szBufferEnd, "(Verbose):");
				break;
			}

			default:
			{
				snprintf(szBufferEnd, szBufferLimit - szBufferEnd, "(Unknown message level):");
				break;
			}
		}
		szBufferEnd += strlen(szBufferEnd);
	}
	snprintf(szBufferEnd, szBufferLimit - szBufferEnd, " ");
	szBufferEnd += strlen(szBufferEnd);

	va_start (vaArgs, pszFormat);
	vsnprintf(szBufferEnd, szBufferLimit - szBufferEnd, pszFormat, vaArgs);
	va_end (vaArgs);
	szBufferEnd += strlen(szBufferEnd);

	/*
	 * Metrics and Traces don't need a location
	 */
	if (bTrace == IMG_FALSE)
	{
		snprintf(szBufferEnd, szBufferLimit - szBufferEnd,
		         " [%d, %s]", (IMG_INT)ui32Line, pszFileName);
		szBufferEnd += strlen(szBufferEnd);
	}

	printf("%s\n", szBuffer);
}

#else /* !defined(SUPPORT_ANDROID_PLATFORM) */

IMG_EXPORT IMG_VOID PVRSRVDebugPrintf(IMG_UINT32		ui32DebugLevel,
									  const IMG_CHAR*	pszFileName,
									  IMG_UINT32		ui32Line,
									  const IMG_CHAR*	pszFormat,
									  ...)
{
	IMG_CHAR szBuffer[PVR_MAX_DEBUG_MESSAGE_LEN];
	IMG_CHAR *pszLeafName;
	va_list vaArgs;

	PVRSRVInheritEnvironmentDebugLevel();

	pszLeafName = (IMG_CHAR *)strrchr(pszFileName, '/');

	if(pszLeafName)
	{
		pszFileName = pszLeafName + 1;
	}

	va_start(vaArgs, pszFormat);
	vsnprintf(szBuffer, sizeof(szBuffer), pszFormat, vaArgs);
	va_end(vaArgs);

	if (!(gPVRDebugLevel & ui32DebugLevel))
	{
		return;
	}

	switch(ui32DebugLevel)
	{
		case DBGPRIV_FATAL:
		case DBGPRIV_ERROR:
			ALOGE("%s:%lu: %s\n", pszFileName, (unsigned long)ui32Line, szBuffer);
			break;

		case DBGPRIV_WARNING:
			ALOGW("%s:%lu: %s\n", pszFileName, (unsigned long)ui32Line, szBuffer);
			break;

		case DBGPRIV_MESSAGE:
			ALOGI("%s:%lu: %s\n", pszFileName, (unsigned long)ui32Line, szBuffer);
			break;

		case DBGPRIV_VERBOSE:
			ALOGV("%s:%lu: %s\n", pszFileName, (unsigned long)ui32Line, szBuffer);
			break;

		case DBGPRIV_CALLTRACE:
		case DBGPRIV_ALLOC:
		default:
			ALOGD("%s:%lu: %s\n", pszFileName, (unsigned long)ui32Line, szBuffer);
			break;
	}
}

#endif /* !defined(SUPPORT_ANDROID_PLATFORM) */

#endif /* defined(PVRSRV_NEED_PVR_DPF) */

#if defined(PVRSRV_NEED_PVR_ASSERT)

/*----------------------------------------------------------------------------
<function>
	FUNCTION   : PVRDebugAssertFail
	PURPOSE    : To indicate to the user that a debug assertion has failed and
	             to prevent the program from continuing.
	PARAMETERS : In : pszFile - The name of the source file where the assertion failed
	             In : uLine - The line number of the failed assertion
	RETURNS    : NEVER!
</function>
------------------------------------------------------------------------------*/
IMG_EXPORT IMG_VOID PVRSRVDebugAssertFail(const IMG_CHAR *pszFile, IMG_UINT32 uLine)
{
	PVRSRVDebugPrintf(DBGPRIV_FATAL, pszFile, uLine, "Debug assertion failed!");
	abort();
}

#endif /* defined(PVRSRV_NEED_PVR_ASSERT) */

#if defined(PVRSRV_NEED_PVR_TRACE)

/*----------------------------------------------------------------------------
<function>
	FUNCTION   : PVRTrace
	PURPOSE    : To output a debug message to the user
	PARAMETERS : In : pszFormat - The message format string
	             In : ... - Zero or more arguments for use by the format string
	RETURNS    : None
</function>
------------------------------------------------------------------------------*/
IMG_EXPORT IMG_VOID PVRSRVTrace(const IMG_CHAR *pszFormat, ...)
{
	IMG_CHAR szMessage[PVR_MAX_DEBUG_MESSAGE_LEN];
	IMG_CHAR *szMessageEnd = szMessage;
	IMG_CHAR *szMessageLimit = szMessage + sizeof(szMessage) - 1;
	va_list ArgList;

	/* The Limit - End pointer arithmetic we're doing in snprintf
	   ensures that our buffer remains null terminated from this */
	*szMessageLimit = '\0';

	snprintf(szMessageEnd, szMessageLimit - szMessageEnd, "PVR: ");
	szMessageEnd += strlen(szMessageEnd);

	va_start(ArgList, pszFormat);
	vsnprintf(szMessageEnd, szMessageLimit - szMessageEnd, pszFormat, ArgList);
	va_end(ArgList);
	szMessageEnd += strlen(szMessageEnd);

    snprintf(szMessageEnd, szMessageLimit - szMessageEnd, "\n");
	szMessageEnd += strlen(szMessageEnd);

#if defined(SUPPORT_ANDROID_PLATFORM)
	ALOGE("%s", szMessage);
#else /* defined(SUPPORT_ANDROID_PLATFORM) */
	fputs(szMessage, stderr);
	/*
	  This sometimes helps on serial connections,
	  even though stderr should be unbuffered.
	*/
	fflush(stderr);
#endif /* defined(SUPPORT_ANDROID_PLATFORM) */
}

#endif /* defined(PVRSRV_NEED_PVR_TRACE) */
