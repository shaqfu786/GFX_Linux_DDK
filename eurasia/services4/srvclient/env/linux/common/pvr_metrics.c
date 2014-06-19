/*****************************************************************************
 * File				pvr_metrics.c
 * 
 * Title			Metrics related functions
 * 
 * Author			Imagination Technologies
 * 
 * date   			30th April 2007
 *  
 * Copyright       	Copyright 2007-2010 by Imagination Technologies Limited.
 *                 	All rights reserved. No part of this software, either
 *                 	material or conceptual may be copied or distributed,
 *                 	transmitted, transcribed, stored in a retrieval system
 *                 	or translated into any human or computer language in any
 *                 	form by any means, electronic, mechanical, manual or
 *                 	other-wise, or disclosed to third parties without the
 *                 	express written permission of Imagination Technologies
 *                 	Limited, Unit 8, HomePark Industrial Estate,
 *                 	King's Langley, Hertfordshire, WD4 8LZ, U.K.
 * 
 * $Log: pvr_metrics.c $
 */

#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "services.h"
#include "pvr_debug.h"
#include "pvr_metrics.h"


#if defined(DEBUG) || defined(TIMING)

static volatile IMG_UINT32 *pui32TimerRegister;

typedef struct tagLARGE_INTEGER
{
	IMG_UINT32 	LowPart;
	IMG_UINT32	HighPart;

} LARGE_INTEGER;

typedef union tagLI
{
	LARGE_INTEGER LargeInt;
	IMG_INT64 LongLong;

} LI;



/***********************************************************************************
 Function Name      : PVRSRVGetTimerRegister
 Inputs             : psConnection (connection info), psMiscInfo (misc. info)
 Outputs            : -	
 Returns            : PVRSRV_ERROR
 Description        : get timer register from misc info 
************************************************************************************/
IMG_EXPORT IMG_VOID PVRSRVGetTimerRegister(IMG_VOID	*pvSOCTimerRegisterUM)
{
	pui32TimerRegister = pvSOCTimerRegisterUM;
}

/***********************************************************************************
 Function Name      : PVRSRVRelaseTimerRegister
 Inputs             : psConnection (connection info), psMiscInfo (misc. info)
 Outputs            : -	
 Returns            : PVRSRV_ERROR
 Description        : get timer register from misc info 
************************************************************************************/
IMG_EXPORT IMG_VOID PVRSRVReleaseTimerRegister(IMG_VOID)
{
	pui32TimerRegister = IMG_NULL;
}

/***********************************************************************************
 Function Name      : PVRSRVMetricsSleep
 Inputs             : ui32MilliSeconds
 Outputs            : -
 Returns            : -
 Description        : Relinquish the time slice for a specified amount of time
************************************************************************************/
static IMG_VOID PVRSRVMetricsSleep(IMG_UINT32 ui32MilliSeconds)
{
	usleep(ui32MilliSeconds * 1000);
}


/***********************************************************************************
 Function Name      : PVRSRVMetricsTimeNow
 Inputs             : -
 Outputs            : -
 Returns            : Time
 Description        : Unsigned int value being the current 32bit clock tick with ref
					  to the CPU Speed.
************************************************************************************/
IMG_EXPORT IMG_UINT32 PVRSRVMetricsTimeNow(IMG_VOID)
{
#if defined (__i386__) || defined(__mips__)

	return PVRSRVClockus();

#else

	static IMG_BOOL bFirstTime = IMG_TRUE;

#if defined(__arm__)

	if (pui32TimerRegister)
	{
		return *pui32TimerRegister;
	}

#endif /* defined(__arm__) */

	/* Metrics not supported */
	if (bFirstTime)
	{
		PVR_DPF ((PVR_DBG_WARNING, "PVRSRVMetricsTimeNow: not implemented"));

		bFirstTime = IMG_FALSE;
	}

	return 0;

#endif /* defined (__i386__) || defined(__mips__) */
}


/***********************************************************************************
 Function Name      : PVRSRVMetricsGetCPUFreq
 Inputs             : -
 Outputs            : -
 Returns            : Float value being the CPU frequency.
 Description        : Gets the CPU frequency
************************************************************************************/
IMG_EXPORT IMG_FLOAT PVRSRVMetricsGetCPUFreq(IMG_VOID)
{
	IMG_UINT32 ui32Time1;
	IMG_FLOAT fTPS;

#if defined(__i386__) || defined(__arm__) || defined(__sh__) || defined(__mips__)

	IMG_UINT32 ui32Time2;

	ui32Time1 = PVRSRVMetricsTimeNow();

	PVRSRVMetricsSleep(1000);

	ui32Time2 = PVRSRVMetricsTimeNow();

	ui32Time1 = (ui32Time2 - ui32Time1);

	fTPS = (IMG_FLOAT) ui32Time1;

#else

	ui32Time1 = 0;

	fTPS = 1.0f;

#endif

	if (ui32Time1 != 0)
	{
		return fTPS;
	}
	else
	{
		return 1.0f;
	}
}

#define BUFLEN 128

/***********************************************************************************
 Function Name      : PVRSRVInitProfileOutput
 Inputs             : ppvFileInfo
 Outputs            : -
 Returns            : -
 Description        : Inits the profile output file
************************************************************************************/
IMG_EXPORT IMG_VOID PVRSRVInitProfileOutput(IMG_VOID **ppvFileInfo)
{
	IMG_CHAR appname[BUFLEN];
	IMG_CHAR *slash;
	FILE *f;

	f = fopen("/proc/self/cmdline", "r");

	if (!f)
	{
		perror ("/proc/self/cmdline");

		return;
	}

	if (fgets(appname, 128, f) == NULL)
	{
		perror("/proc/self/cmdline");
		fclose(f);
		return;
	}

	fclose(f);

	if ((slash=strrchr(appname, '/')) != NULL)
	{
		memmove(appname, slash+1, strlen(slash));
	}

	strncat(appname, "_profile.txt", BUFLEN - strlen(appname));
	appname[BUFLEN - 1] = 0;

	*ppvFileInfo = fopen(appname, "w");
	if(!*ppvFileInfo)
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: Failed to open %s for writing",
				 __func__, appname));
	}
}


/***********************************************************************************
 Function Name      : PVRSRVDeInitProfileOutput
 Inputs             : ppvFileInfo
 Outputs            : -
 Returns            : -
 Description        : Ends the profile output file by closing it
************************************************************************************/
IMG_EXPORT IMG_VOID PVRSRVDeInitProfileOutput(IMG_VOID **ppvFileInfo)
{
	if(!*ppvFileInfo)
		return;

	fclose(*ppvFileInfo);
	*ppvFileInfo = NULL;
}


/***********************************************************************************
 Function Name      : PVRSRVProfileOutput
 Inputs             : pvFileInfo, psString
 Outputs            : -
 Returns            : -
 Description        : Prints the string to the profile file and to the console
************************************************************************************/
IMG_EXPORT IMG_VOID PVRSRVProfileOutput(IMG_VOID *pvFileInfo, const IMG_CHAR *psString)
{
	if(pvFileInfo)
	{
		fprintf(pvFileInfo, "%s\n", psString);
	}

	PVR_TRACE(("%s", psString));
}


#endif  /* defined (TIMING) || defined (DEBUG) */







