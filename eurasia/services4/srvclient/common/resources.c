/*!****************************************************************************
@File           resources.c

@Title          Hardware interface resources/utilities

@Author         Imagination Technologies

@date           05/09/2003

@Copyright      Copyright 2003-2008 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       Generic

@Description    Hardware interface resources/utilities

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-
$Log: resources.c $
******************************************************************************/

#include "img_defs.h"
#include "services.h"
#include "servicesint.h"
#include "pvr_debug.h"
#include "osfunc_client.h"

#if defined(SUPPORT_ANDROID_PLATFORM)
#define I_KNOW_WHAT_I_AM_DOING
#include "hal.h"
#endif

/*!
*******************************************************************************

 @Function	PollForValue

 @Description	Polls for a value to match a masked read of sysmem

 @Input		psConnection : Services connection

 @Input		hOSEvent : Handle to OS event object to wait for

 @Input		ui32Value : required value

 @Input		ui32Mask : Mask

 @Input		ui32Waitus : wait between tries (us)

 @Input		ui32Tries : number of tries

 @Return	PVRSRV_ERROR :

******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR PVRSRVPollForValue(const PVRSRV_CONNECTION *psConnection,
#if defined (SUPPORT_SID_INTERFACE)
						  IMG_SID    hOSEvent,
#else
						  IMG_HANDLE hOSEvent,
#endif
						  volatile IMG_UINT32* pui32LinMemAddr,
						  IMG_UINT32 ui32Value,
						  IMG_UINT32 ui32Mask,
						  IMG_UINT32 ui32Waitus,
						  IMG_UINT32 ui32Tries)
{
	IMG_UINT32 uiStart;

	/* Use event object if available */
	if(hOSEvent)
	{
		while((*pui32LinMemAddr & ui32Mask) != ui32Value)
		{
			if(!ui32Tries)
			{
				return PVRSRV_ERROR_TIMEOUT_POLLING_FOR_VALUE;
			}

			if(PVRSRVEventObjectWait(psConnection,
									 hOSEvent) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_MESSAGE, "PVRSRVPollForValue: PVRSRVEventObjectWait failed"));
				ui32Tries--;
			}
		}

		return PVRSRV_OK;
	}
	else
	{
		/* Quick initial check */
		if ((*pui32LinMemAddr & ui32Mask) == ui32Value)
		{
			return PVRSRV_OK;
		}

		uiStart = PVRSRVClockus();

		do
		{
			PVRSRVWaitus(ui32Waitus);

			if ((*pui32LinMemAddr & ui32Mask) == ui32Value)
			{
				return PVRSRV_OK;
			}

		} while ((PVRSRVClockus() - uiStart) < (ui32Tries * ui32Waitus));
	}

	return PVRSRV_ERROR_TIMEOUT_POLLING_FOR_VALUE;
}

/*!
 ******************************************************************************

 @Function		PVRSRVGetErrorString

 @Description	Returns a text string relating to the PVRSRV_ERROR enum.

 @Note		case statement used rather than an indexed arrary to ensure text is
 			synchronised with the correct enum

 @Input		eError : PVRSRV_ERROR enum

 @Return	const IMG_CHAR * : Text string

 @Note		Must be kept in sync with servicesext.h

******************************************************************************/

IMG_EXPORT
const IMG_CHAR *PVRSRVGetErrorString(PVRSRV_ERROR eError)
{
/* PRQA S 5087 1 */ /* include file needed here */	
#include "pvrsrv_errors.h"
}

/*!
 ******************************************************************************
 
 @Function	HWOpTimeout
 @Description	Handles timeouts occurring following HW task submission
 @Return	PVRSRV_ERROR

 ******************************************************************************/
static
PVRSRV_ERROR IMG_CALLCONV HWOpTimeout(	PVRSRV_DEV_DATA *psDevData,
										IMG_PVOID pvData)
{
	PVRSRV_ERROR eErr;
	IMG_UINT32 i;

	IMG_CONST PVRSRV_CLIENT_DEV_DATA *psClientDevData = &psDevData->psConnection->sClientDevData;
	
	PVR_DPF((PVR_DBG_ERROR, "HW operation timeout occurred."));

#if defined(SUPPORT_ANDROID_PLATFORM)
	{
		IMG_gralloc_module_t *psGrallocModule = pvData;

		/* If the user passed private data, we can assume it was the graphics
		 * HAL and avoid doing a dlopen() of the gralloc module. If another
		 * client API calls PVRSRVClientEvent(), it will pass NULL, and we
		 * must re-load the module.
		 */
		if(!psGrallocModule)
		{
			/* If module loading fails, it's non-fatal -- we just won't dump
			 * the extra sync object debugging info.
			 */
			if(!hw_get_module(GRALLOC_HARDWARE_MODULE_ID,
							  (const hw_module_t**)&psGrallocModule))
			{
				psGrallocModule->LogSyncs(psGrallocModule);
			}
		}
		else
		{
			/* We can trust that this module instance is valid */
			psGrallocModule->LogSyncs(psGrallocModule);
		}
	}
#else /* defined(SUPPORT_ANDROID_PLATFORM) */
	PVR_UNREFERENCED_PARAMETER(pvData);
#endif /* defined(SUPPORT_ANDROID_PLATFORM) */

	for (i = 0; i < psClientDevData->ui32NumDevices; i++)
	{
		/* Dump debug trace for each services-managed device */
		if (psClientDevData->apfnDumpTrace[i])
		{
			eErr = psClientDevData->apfnDumpTrace[i](psDevData);
			if (eErr != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "HWOpTimeout: Failure to write debug trace info (%d).",
					eErr));
			}
			else
			{
				PVR_DPF((PVR_DBG_WARNING, "HWOpTimeout: Debug trace info written to system log ... OK"));
			}
		}
	}
	
#if defined(PVRSRV_RESET_ON_HWTIMEOUT)
	{
		IMG_CONST PVRSRV_CONNECTION *psConnection = psDevData->psConnection;
		PVRSRV_MISC_INFO sMiscInfo;
		PVRSRVMemSet(&sMiscInfo, sizeof(PVRSRV_MISC_INFO), 0);
		sMiscInfo.ui32StateRequest = PVRSRV_MISC_INFO_RESET_PRESENT;
		
		/* Reboot device, device specific details handled in KM */
		PVRSRVGetMiscInfo(psConnection, &sMiscInfo);
	}
#endif


#if defined(PVRSRV_CLIENT_RESET_ON_HWTIMEOUT)
	{
		/* Terminate the client */
		OSTermClient("HW operation timeout", 0);
	}
#endif

	return PVRSRV_OK;
}

/*!
 ******************************************************************************

 @Function	PVRSRVClientEvent
 @Description	Handles timeouts occurring in client drivers
 @Input		psConnection
 			eEvent - event type
 			pvData - client-specific data
 @Output	psMiscInfo
 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVClientEvent(IMG_CONST PVRSRV_CLIENT_EVENT eEvent,
											PVRSRV_DEV_DATA *psDevData,
											IMG_PVOID pvData)
{
	switch (eEvent)
	{
		case PVRSRV_CLIENT_EVENT_HWTIMEOUT:
			return HWOpTimeout(psDevData, pvData);
		default:
			break;
	}
	
	/* unknown event type */
	return PVRSRV_ERROR_INVALID_PARAMS;													
}	

/******************************************************************************
 End of file (resources.c)
******************************************************************************/
