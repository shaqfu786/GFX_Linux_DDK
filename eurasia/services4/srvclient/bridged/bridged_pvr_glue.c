/*!****************************************************************************
@File           bridged_pvr_glue.c

@Title          shared pvr glue code

@Author         Imagination Technologies

@Copyright      Copyright 2008 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Description    Implements shared PVRSRV API user bridge code

Modifications :-
$Log: bridged_pvr_glue.c $
******************************************************************************/

#include "img_defs.h"
#include "services.h"
#include "pvr_bridge.h"
#include "pvr_bridge_client.h"
#include "pvr_debug.h"
#include "pdump_um.h"
#include "osfunc_client.h"
#include "servicesinit_um.h"

#include "os_srvclient_config.h"

#if defined (SUPPORT_SGX)
#include "sgx/bridged_sgx_glue.h"
#endif

#include <graphics_buffer_object_priv.h>

/*!
 ******************************************************************************

 @Function	FlushClientOps

 @Description
 Flushes all operations on the specified sync object submitted from the
 current client (current process)

 @Input		psConnection : handle for services connection
 @Input		psSyncData : sync data to flush

 @Return	PVRSRV_ERROR

 *****************************************************************************/
#if defined (PVRSRV_DO_NOT_FLUSH_CLIENT_OPS)
static PVRSRV_ERROR FlushClientOps(IMG_CONST PVRSRV_CONNECTION *psConnection,
#if defined (SUPPORT_SID_INTERFACE)
								   IMG_SID hKernelSyncInfo)
#else
								   PVRSRV_CLIENT_SYNC_INFO *psSyncInfo)
#endif
{
    /* No need to flush client ops, for example, at initialisation time */
    PVR_UNREFERENCED_PARAMETER(psConnection);
    #if defined (SUPPORT_SID_INTERFACE)
    PVR_UNREFERENCED_PARAMETER(hKernelSyncInfo);
    #else
    PVR_UNREFERENCED_PARAMETER(psSyncInfo);
    #endif
    return PVRSRV_OK;
}
#else
static PVRSRV_ERROR FlushClientOps(IMG_CONST PVRSRV_CONNECTION *psConnection,
#if defined (SUPPORT_SID_INTERFACE)
								   IMG_SID hKernelSyncInfo)
#else
								   PVRSRV_CLIENT_SYNC_INFO *psSyncInfo)
#endif
{
#if !defined(PVR_KERNEL)
	IMG_UINT32 ui32Start;
#endif
	PVRSRV_ERROR eError;

	PVRSRV_SYNC_TOKEN sSyncToken;

	/*
	   NB: it should be noted that this flush will only work for
	   hardware sync ops, or software ops in a different
	   thread/process.  It will not auto-complete ops started with
	   "PVRSRVModifyPendingSyncOps".  If you see a timeout here, and
	   you have used PVRSRVModifyPendingSyncOps(), please be sure to
	   call PVRSRVSyncOpsFlushToModObj() and
	   PVRSRVModifyCompleteSyncOps() before attempting to free the
	   buffer.
	*/
	/*
	   Also note that we don't expect any new pending ops to come along
	   after this point, e.g. by another process

	   The old code used to take a snapshot of pending value to guard
	   against this.

	   TODO: Consider whether we should use the FlushToModObj() API to
	   emulate such behaviour.
	*/

#if defined (SUPPORT_SID_INTERFACE)
	if(!hKernelSyncInfo)
#else
	if(!psSyncInfo)
#endif
	{
		PVR_DPF((PVR_DBG_ERROR, "FlushClientOps: invalid psSyncInfo"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

#if !defined(PVR_KERNEL)
	ui32Start = PVRSRVClockus();
#endif

	eError = PVRSRVSyncOpsTakeToken(psConnection,
#if defined (SUPPORT_SID_INTERFACE)
									hKernelSyncInfo,
#else
									psSyncInfo,
#endif
									&sSyncToken);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "FlushClientOps: failed to acquire token"));
		return eError;
	}

	for(;;)
	{
		eError = PVRSRVSyncOpsFlushToToken(psConnection,
#if defined (SUPPORT_SID_INTERFACE)
										   hKernelSyncInfo,
#else
										   psSyncInfo,
#endif
										   &sSyncToken,
										   IMG_FALSE);
		if (eError != PVRSRV_ERROR_RETRY)
		{
			break;
		}

#if !defined(PVR_KERNEL)
		/* TODO: ideally should use event object */
		if ((PVRSRVClockus() - ui32Start) > MAX_HW_TIME_US)
		{
			PVR_DPF((PVR_DBG_ERROR, "FlushClientOps: ops pending timeout"));
			return PVRSRV_ERROR_TIMEOUT_POLLING_FOR_VALUE;
		}
		PVRSRVWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
#else
		/* We don't do the retry if we're in the kernel (e.g. srvinit),
		   but there ought not to be any outstanding ops anyway */
		{
			PVR_DPF((PVR_DBG_ERROR, "FlushClientOps: ops pending timeout"));
			return PVRSRV_ERROR_TIMEOUT_POLLING_FOR_VALUE;
		}
#endif
	}

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "FlushClientOps: flush to token failed"));
		return eError;
	}

	return eError;
}
#endif /* PVRSRV_DO_NOT_FLUSH_CLIENT_OPS */


#if !defined(OS_PVRSRV_ENUMERATE_DEVICE_CLASS_UM) && !defined(SRVINIT_MODULE)
/*!
 ******************************************************************************

 @Function	PVRSRVEnumerateDeviceClass

 @Description
 Enumerates devices available in a given class.
 On first call, pass valid ptr for pui32DevCount and NULL for pui32DevID,
 On second call, pass same ptr for pui32DevCount and client allocated ptr
 for pui32DevID device id list

 @Input		psConnection : handle for services connection
 @Input		eDeviceClass : device class identifier

 @Output	pui32DevCount : number of devices available in class
 @Output	pui32DevID : list of device ids in the device class

 @Return	PVRSRV_ERROR

 *****************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVEnumerateDeviceClass(const PVRSRV_CONNECTION *psConnection,
													 PVRSRV_DEVICE_CLASS DeviceClass,
													 IMG_UINT32 *pui32DevCount,
													 IMG_UINT32 *pui32DevID)
{
	PVRSRV_BRIDGE_OUT_ENUMCLASS sOut;
	PVRSRV_BRIDGE_IN_ENUMCLASS sIn;
	IMG_UINT32 i;

	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));
	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	
	if (!psConnection || (!psConnection->hServices))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVEnumerateDeviceClass: Invalid connection"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(pui32DevCount == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVEnumerateDeviceClass: Invalid DevCount"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sIn.sDeviceClass = DeviceClass;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_ENUM_CLASS,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_ENUMCLASS),
						&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_ENUMCLASS)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVEnumerateDeviceClass: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVEnumerateDeviceClass: Error %d returned", sOut.eError));
		return sOut.eError;
	}

	if (!pui32DevID)
	{
		/* First mode, ask for Device Count */
		*pui32DevCount = sOut.ui32NumDevices;
	}
	else
	{
		/* Fill in device id array */
		PVR_ASSERT(*pui32DevCount == sOut.ui32NumDevices);
		for (i=0; i < sOut.ui32NumDevices; i++)
		{
			pui32DevID[i] = sOut.ui32DevID[i];
		}
	}

	return PVRSRV_OK;
}
#endif /* OS_PVRSRV_ENUMERATE_DEVICE_CLASS_UM */


#if !defined(OS_PVRSRV_CONNECT_UM)

#if defined(SUPPORT_PDUMP_MULTI_PROCESS)
static
IMG_BOOL IMG_CALLCONV PVRSRVConnectPDumpActive(IMG_UINT32 *pui32SrvFlags)
{
	IMG_UINT32 ui32PDumpActive = 0; /* from .ini file */
	
	/* app hint state */
	IMG_VOID *pvHintState;

	/* create app hint state */
	PVRSRVCreateAppHintState(IMG_SRVCLIENT, 0, &pvHintState);
	
	/* look up to see if it's in list of active pdumps */	
	if(PVRSRVGetAppHint(pvHintState,
			   			"PDumpActive",
		   				IMG_UINT_TYPE,
		   				&ui32PDumpActive,
		   				&ui32PDumpActive) )
	{
		if (ui32PDumpActive > 0)
		{
			*pui32SrvFlags |= SRV_FLAGS_PDUMP_ACTIVE;
			return IMG_TRUE;			
		}
	}
	return IMG_FALSE;
}
#endif /* SUPPORT_PDUMP_MULTI_PROCESS */

/*!
 ******************************************************************************

 @Function	PVRSRVConnectServices

 @Description	Generic connection to services. Wrapped
			 by PVRSRVConnect() and PVRSRVInitSrvConnect().

 @Input		psConnection
 @Return	PVRSRV_ERROR

 ******************************************************************************
 */
static
PVRSRV_ERROR IMG_CALLCONV PVRSRVConnectServices(PVRSRV_CONNECTION **ppsConnection, IMG_UINT32 ui32SrvFlags)
{
	PVRSRV_ERROR 	eError;

#if defined(SUPPORT_PDUMP_MULTI_PROCESS)
	/* Is this process marked for pdumping? */
	PVRSRVConnectPDumpActive(&ui32SrvFlags);
#endif	
	/* Connect to services.
	 * Allocate a services connection on the heap, and keep it until we
	 * call PVRSRVDisconnect.
	 */
	if (!ppsConnection)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVConnect: Invalid connection."));
		return PVRSRV_ERROR_INIT_FAILURE;
	}
	*ppsConnection = PVRSRVAllocUserModeMem(sizeof(PVRSRV_CONNECTION));
	PVRSRVMemSet(*ppsConnection, 0, sizeof(PVRSRV_CONNECTION));

	if (!*ppsConnection)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVConnectServices: Invalid parameter"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	eError = OpenServices(&(*ppsConnection)->hServices, ui32SrvFlags);
	if(eError != PVRSRV_OK)
	{
		return eError;
	}

	(*ppsConnection)->ui32ProcessID = PVRSRVGetCurrentProcessID();

	MemoryResourceInitService(*ppsConnection);

	return PVRSRV_OK;
}

/*!
 ******************************************************************************

 @Function	PVRSRVConnect

 @Description	Creates a connection from an application to the services module
			 and initialises device-specific call-back functions.

 @Input		psConnection
 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVConnect(PVRSRV_CONNECTION **ppsConnection, IMG_UINT32 ui32SrvFlags)
{
	PVRSRV_ERROR 	eError;
	IMG_UINT		i;

	PVRSRV_CLIENT_DEV_DATA	*psClientDevData;
	PVRSRV_DEV_DATA      	sDevData;

	eError = PVRSRVConnectServices(ppsConnection, ui32SrvFlags);
	if ((eError != PVRSRV_OK) || !*ppsConnection)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVConnect: Unable to open connection."));
		return eError;
	}

	/* Initialise connection flags
	 */
	(*ppsConnection)->ui32SrvFlags = ui32SrvFlags;

	/* Retrieve the client-side device data */
	psClientDevData = &((*ppsConnection)->sClientDevData);

	PVR_DPF((PVR_DBG_MESSAGE, "PVRSRVConnect: Allocating %d bytes on stack for sDevData",
			sizeof(PVRSRV_DEV_DATA) ));

	/* Enumerate devices */
	eError = PVRSRVEnumerateDevices(*ppsConnection,
			&psClientDevData->ui32NumDevices, psClientDevData->asDevID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVConnect: Unable to enumerate devices."));
		return eError;
	}

	/* Initialise loop-backs */
	for (i = 0; i < psClientDevData->ui32NumDevices; i++)
	{
		switch (psClientDevData->asDevID[i].eDeviceType)
		{
#if defined(SUPPORT_SGX)
			/* set up SGX specific call-backs */
		case PVRSRV_DEVICE_TYPE_SGX:
			psClientDevData->apfnDevConnect[i] = &SGXConnectionCheck;
			psClientDevData->apfnDumpTrace[i] = &SGXDumpMKTrace;
			break;
#endif
		default:
			psClientDevData->apfnDevConnect[i] = IMG_NULL;
			psClientDevData->apfnDumpTrace[i] = IMG_NULL;
			break;
		}
	}

	/* Run device-specific loop-backs */
	for (i = 0; i < psClientDevData->ui32NumDevices; i++)
	{
		/* Acquire device data */
		eError = PVRSRVAcquireDeviceData ( 	*ppsConnection,
											psClientDevData->asDevID[i].ui32DeviceIndex,
											&sDevData,
											PVRSRV_DEVICE_TYPE_UNKNOWN);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVConnect: Unable to acquire device data for device index: %d.",
					psClientDevData->asDevID[i].ui32DeviceIndex));
			return eError;
		}

		if (psClientDevData->apfnDevConnect[i] != IMG_NULL)
		{
			eError = psClientDevData->apfnDevConnect[i](&sDevData);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "PVRSRVConnect: Device connect callback failed for device index: %d.",
						psClientDevData->asDevID[i].ui32DeviceIndex));
				return eError;
			}
		}
	}
	/* No errors occurred if we reach this point */
	return PVRSRV_OK;
}
#endif /* OS_PVRSRV_CONNECT_UM */


#if !defined(OS_PVRSRV_DISCONNECT_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVDisconnect

 @Description	Disconnects from the services module

 @input		psConnection

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVDisconnect(IMG_CONST PVRSRV_CONNECTION *psConnection)
{
	PVRSRV_ERROR eError;

	MemoryResourceCloseService(psConnection);

	eError = CloseServices(psConnection->hServices);
	if (psConnection != IMG_NULL)
	{
		/* PRQA S 0311 1 */ /* ignore warning about loss of const qualification */
		PVRSRVFreeUserModeMem((IMG_PVOID)psConnection); /* allocated on the heap in PVRSRVConnect */
	}
	if (eError != PVRSRV_OK)
	{
		return eError;
	}
	return PVRSRV_OK;
}
#endif /* OS_PVRSRV_DISCONNECT_UM */


#if !defined(OS_PVRSRV_ENUMERATE_DEVICES_UM)
/*!
 ******************************************************************************

 @Function PVRSRVEnumerateDevices

 @Description	Enumerate all PowerVR services supported devices in the system

 The function returns a list of the device IDs stored either in the services
 (or constructed in the user mode glue component in certain environments)
 The number of devices in the list is also returned.

 The user is required to provide a buffer large enough to receive an array of
 MAX_NUM_DEVICE_IDS*PVRSRV_DEVICE_IDENTIFIER structures.

 In a binary layered component which does not support dynamic runtime
 selection, the glue code should compile to return the supported devices
 statically, e.g. multiple instances of the same device if multiple devices are
 supported, or the target combination of SGX and display device.

 In the case of an environment (for instance) where one SGX may connect to two
 display devices this code would enumerate all three devices and even
 non-dynamic SGX selection code should retain the facility to parse the list
 to find the index of the SGX device

 @input		psConnection : Connection info

 @output	puiNumDevices : Number of devices present in the system

 @output	puiDevIDs :	Pointer to called supplied array of
 PVRSRV_DEVICE_IDENTIFIER structures. The array is assumed to be at least
 PVRSRV_MAX_DEVICES long.

 @return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVEnumerateDevices(const PVRSRV_CONNECTION *psConnection,
												 IMG_UINT32 *pui32NumDevices,
												 PVRSRV_DEVICE_IDENTIFIER *puiDevIDs)
{
	PVRSRV_BRIDGE_OUT_ENUMDEVICE sOut;
	IMG_UINT32 i=0;

	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));

	if(!pui32NumDevices || !puiDevIDs)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVEnumerateDevices: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_ENUM_DEVICES,
						IMG_NULL,
						0,
						&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_ENUMDEVICE)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVEnumerateDevices: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVEnumerateDevices: Error %d returned", sOut.eError));
		return sOut.eError;
	}

	*pui32NumDevices = sOut.ui32NumDevices;

	for(i=0; i < sOut.ui32NumDevices; i++)
	{
		puiDevIDs[i] = sOut.asDeviceIdentifier[i];
	}

	return sOut.eError;
}
#endif /* OS_PVRSRV_ENUMERATE_DEVICES_UM */


#if !defined(OS_PVRSRV_ACQUIRE_DEVICE_DATA_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVAcquireDeviceData

 @Description	Returns device info structure pointer for the requested device

 This populates a PVRSRV_DEV_DATA structure with appropriate pointers to the
 DevInfo structure for the device requested.

 In a non-plug-and-play the first call to GetDeviceInfo for a device causes
 device initialisation

 Calls to GetDeviceInfo are reference counted

 @Input		psConnection : Handle to services

 @Input		uiDevIndex : Index to the required device obtained from the
 PVRSRVEnumerateDevice function

 @Input		eDeviceType : Required device type. If type is unknown use uiDevIndex
 to locate device data

 @Output	psDevData :

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVAcquireDeviceData(const PVRSRV_CONNECTION *psConnection,
												  IMG_UINT32	uiDevIndex,
												  PVRSRV_DEV_DATA	*psDevData,
												  PVRSRV_DEVICE_TYPE	eDeviceType)
{
	PVRSRV_BRIDGE_OUT_ACQUIRE_DEVICEINFO	sOut;
	PVRSRV_BRIDGE_IN_ACQUIRE_DEVICEINFO		sIn;

	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));
	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	
	sIn.uiDevIndex	= uiDevIndex;
	sIn.eDeviceType	= eDeviceType;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_ACQUIRE_DEVICEINFO,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_ACQUIRE_DEVICEINFO),
						&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_ACQUIRE_DEVICEINFO)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVAcquireDeviceData: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVAcquireDeviceData: Error %d returned", sOut.eError));
		return(sOut.eError);
	}

	psDevData->hDevCookie = sOut.hDevCookie;
	psDevData->psConnection = psConnection;

	return(sOut.eError);
}
#endif /* OS_PVRSRV_ACQUIRE_DEVICE_DATA_UM */


#if !defined(OS_PVRSRV_CREATE_DEVICE_MEM_UM) && !defined(SRVINIT_MODULE)
/*!
 ******************************************************************************

 @Function	PVRSRVCreateDeviceMemContext

 @Description	Creates a Device Addressable Memory Context

 @Input		psDevData : device and  ioctl connection info

 @Output	phDevMemContext : device addressable memory context handle

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVCreateDeviceMemContext(const PVRSRV_DEV_DATA *psDevData,
#if defined (SUPPORT_SID_INTERFACE)
													   IMG_SID *phDevMemContext,
#else
													   IMG_HANDLE *phDevMemContext,
#endif
													   IMG_UINT32 *pui32ClientHeapCount,
													   PVRSRV_HEAP_INFO *psHeapInfo)
{
	PVRSRV_BRIDGE_IN_CREATE_DEVMEMCONTEXT sIn;
	PVRSRV_BRIDGE_OUT_CREATE_DEVMEMCONTEXT sOut;
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_UINT32 i;

	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));
	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));

	if(!psDevData
	   || !phDevMemContext
	   || !pui32ClientHeapCount
	   || !psHeapInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVCreateDeviceMemContext: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sIn.hDevCookie = psDevData->hDevCookie;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_CREATE_DEVMEMCONTEXT,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_CREATE_DEVMEMCONTEXT),
						&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_CREATE_DEVMEMCONTEXT)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVCreateDeviceMemContext: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVCreateDeviceMemContext: Error %d returned", sOut.eError));
		return sOut.eError;
	}

	/* return the context handle */
	*phDevMemContext = sOut.hDevMemContext;

	/* the shared heap information */
	*pui32ClientHeapCount = sOut.ui32ClientHeapCount;
	for(i=0; i<sOut.ui32ClientHeapCount; i++)
	{
		psHeapInfo[i] = sOut.sHeapInfo[i];
	}

	return eError;
}
#endif /* OS_PVRSRV_CREATE_DEVICE_MEM_UM */


#if !defined(OS_PVRSRV_DESTROY_DEVICE_MEM_UM) && !defined(SRVINIT_MODULE)
/*!
 ******************************************************************************

 @Function	PVRSRVDestroyDeviceMemContext

 @Description	Destroys a Device Addressable Memory Context

 @Input		psDevData : device and  ioctl connection info

 @Input		hDevMemContext : device addressable memory context handle

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVDestroyDeviceMemContext(const PVRSRV_DEV_DATA *psDevData,
#if defined (SUPPORT_SID_INTERFACE)
														IMG_SID hDevMemContext)
#else
														IMG_HANDLE hDevMemContext)
#endif
{
	PVRSRV_BRIDGE_IN_DESTROY_DEVMEMCONTEXT sIn;
	PVRSRV_BRIDGE_RETURN sRet;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sRet,0x00,sizeof(sRet));
	
	if(!psDevData || !hDevMemContext)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVDestroyDeviceMemContext: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sIn.hDevMemContext = hDevMemContext;
	sIn.hDevCookie = psDevData->hDevCookie;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_DESTROY_DEVMEMCONTEXT,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_DESTROY_DEVMEMCONTEXT),
						&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVDestroyDeviceMemContext: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sRet.eError != PVRSRV_OK)
	{
		PVR_DPF ((PVR_DBG_ERROR,"PVRSRVDestroyDeviceMemContext: allocations still exist in the memory context to be destroyed"));
		PVR_DPF ((PVR_DBG_ERROR,"Likely Cause: client drivers not freeing alocations before destroying devmemcontext"));
	}

	return sRet.eError;
}
#endif /* OS_PVRSRV_DESTROY_DEVICE_MEM_UM */


#if !defined(OS_PVRSRV_GET_DEVICEMEM_HEAPINFO_UM) && !defined(SRVINIT_MODULE)
/*!
 ******************************************************************************

 @Function	PVRSRVGetDeviceMemHeapInfo

 @Descriptiongets heap info

 @Input		psDevData : device and  ioctl connection info
 @Input		hDevMemContext

 @Output	pui32ClientHeapCount
 @Output	psHeapInfo

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_INTERNAL
PVRSRV_ERROR IMG_CALLCONV PVRSRVGetDeviceMemHeapInfo(const PVRSRV_DEV_DATA *psDevData,
#if defined (SUPPORT_SID_INTERFACE)
													   IMG_SID     hDevMemContext,
#else
													   IMG_HANDLE hDevMemContext,
#endif
													   IMG_UINT32 *pui32ClientHeapCount,
													   PVRSRV_HEAP_INFO *psHeapInfo)
{
	PVRSRV_BRIDGE_IN_GET_DEVMEM_HEAPINFO sIn;
	PVRSRV_BRIDGE_OUT_GET_DEVMEM_HEAPINFO sOut;
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_UINT32 i;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));	

	
	if(!psDevData
	   || !hDevMemContext
	   || !pui32ClientHeapCount
	   || !psHeapInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetDeviceMemHeapInfo: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sIn.hDevCookie = psDevData->hDevCookie;
	sIn.hDevMemContext = hDevMemContext;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_GET_DEVMEM_HEAPINFO,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_GET_DEVMEM_HEAPINFO),
						&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_GET_DEVMEM_HEAPINFO)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetDeviceMemHeapInfo: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetDeviceMemHeapInfo: Error %d returned", sOut.eError));
		return sOut.eError;
	}

	/* the shared heap information */
	*pui32ClientHeapCount = sOut.ui32ClientHeapCount;
	for(i=0; i<sOut.ui32ClientHeapCount; i++)
	{
		psHeapInfo[i] = sOut.sHeapInfo[i];
	}

	return eError;
}
#endif /* OS_PVRSRV_GET_DEVICEMEM_HEAPINFO_UM */


#if !defined(OS_PVRSRV_ALLOC_DEVICE_MEM_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVAllocDeviceMem2

 @Description	Allocates device memory

 @Input		psDevData : DevData for the device

 @Input		ui32Attribs : Some combination of PVRSRV_MEMFLG_ flags

 @Input		uSize : Number of bytes to allocate

 @Input		ui32Alignment : Alignment of allocation

 @Input		pvPrivData : Opaque private data passed through to allocator

 @Input		ui32PrivDataLength : Length of opaque private data

 @Output	**ppsMemInfo : Receives a pointer to the created MEM_INFO structure

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVAllocDeviceMem2(const PVRSRV_DEV_DATA *psDevData,
#if defined (SUPPORT_SID_INTERFACE)
												IMG_SID	 hDevMemHeap,
#else
												IMG_HANDLE hDevMemHeap,
#endif
												IMG_UINT32 ui32Attribs,
												IMG_SIZE_T uSize,
												IMG_SIZE_T uAlignment,
												IMG_PVOID pvPrivData,
												IMG_UINT32 ui32PrivDataLength,
												PVRSRV_CLIENT_MEM_INFO **ppsMemInfo)
{
	PVRSRV_BRIDGE_OUT_ALLOCDEVICEMEM sOut;
	PVRSRV_BRIDGE_IN_ALLOCDEVICEMEM sIn;
	PVRSRV_CLIENT_MEM_INFO	*psMemInfo;
	PVRSRV_CLIENT_SYNC_INFO *psSyncInfo = IMG_NULL;
	PVRSRV_ERROR eError=PVRSRV_OK;
	PVRSRV_BRIDGE_IN_FREEDEVICEMEM sInFailure;
	PVRSRV_BRIDGE_RETURN sRet;
	IMG_BOOL bNoCpuMapping = (ui32Attribs & PVRSRV_MAP_NOUSERVIRTUAL) ? IMG_TRUE : IMG_FALSE;
	IMG_UINT32 ui32OrigSize=0;
	IMG_UINT32 ui32OrigAlignment=0;

	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));	
	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));		
	PVRSRVMemSet(&sInFailure,0x00,sizeof(sInFailure));			
    PVRSRVMemSet(&sRet,0x00,sizeof(sRet));			

	if(!psDevData || !ppsMemInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocDeviceMem: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	ui32Attribs &= ~PVRSRV_MAP_NOUSERVIRTUAL;

	if(ui32Attribs & (PVRSRV_MEM_XPROC | PVRSRV_MEM_ION))
	{
		ui32OrigSize = (IMG_UINT32)uSize;
		ui32OrigAlignment = (IMG_UINT32)uAlignment;

        if (uAlignment < 4096)
        {
            /* TODO: how determine req'd alignment? */
            /* Need to boost alignment to be at least that of the
               heap's data page size, in order to eliminate the risk
               of getting sub-alloc'ed memory.  This would represent a
               security risk, so will be caught kernel-side, but this
               is just an extra check so that we catch the error
               sooner and be more polite about it.  Since we can't
               determine the heap's DP size, we just assume 4kB here.
               It is okay if this is wrong, you just don't get the
               extra checks */
            uAlignment = 4096;
        }
		uSize = 1 + ((uSize-1) | (uAlignment-1));
		if (uAlignment != ui32OrigAlignment)
		{
			PVR_DPF((PVR_DBG_WARNING, "PVRSRVAllocDeviceMem(): insufficient alignment, 0x%x, promoted to 0x%x", ui32OrigAlignment, uAlignment));
		}
		if (uSize != ui32OrigSize)
		{
			PVR_DPF((PVR_DBG_WARNING, "PVRSRVAllocDeviceMem(): unaligned size, 0x%x, promoted to 0x%x", ui32OrigSize, uSize));
		}
	}

	PVRSRVMemSet(&sOut, 0, sizeof(sOut));

	sIn.hDevCookie         = psDevData->hDevCookie;
	sIn.hDevMemHeap        = hDevMemHeap;
	sIn.ui32Attribs        = ui32Attribs;
	sIn.ui32Size           = uSize;
	sIn.ui32Alignment      = uAlignment;
	sIn.pvPrivData         = pvPrivData;
	sIn.ui32PrivDataLength = ui32PrivDataLength;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_ALLOC_DEVICEMEM,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_ALLOCDEVICEMEM),
						(IMG_VOID *)&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_ALLOCDEVICEMEM)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocDeviceMem: BridgeCall failed"));
		PDUMPCOMMENT(psDevData->psConnection, "Device mem allocation failed.", IMG_TRUE);
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocDeviceMem: Error %d returned", sOut.eError));
		PDUMPCOMMENT(psDevData->psConnection, "Device mem allocation failed.", IMG_TRUE);
		return sOut.eError;
	}

	eError = PVRSRV_OK;

	/* If eviction is supported, allocate an Resource Object instead */
	if(sOut.sClientMemInfo.ui32Flags & PVRSRV_HAP_GPU_PAGEABLE)
		psMemInfo = (PVRSRV_CLIENT_MEM_INFO *)PVRSRVCallocUserModeMem(sizeof(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT));
	else
		psMemInfo = (PVRSRV_CLIENT_MEM_INFO *)PVRSRVAllocUserModeMem(sizeof(PVRSRV_CLIENT_MEM_INFO));

	if (!psMemInfo)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		PDUMPCOMMENT(psDevData->psConnection, "Device mem allocation failed: out of memory.", IMG_TRUE);

		goto MEMINFO_CLEANUP_MEM_ALLOC;
	}

	if((ui32Attribs & PVRSRV_MEM_NO_SYNCOBJ) == 0)
	{
		psSyncInfo = (PVRSRV_CLIENT_SYNC_INFO *)PVRSRVAllocUserModeMem(sizeof(PVRSRV_CLIENT_SYNC_INFO));

		if (!psSyncInfo)
		{
			eError = PVRSRV_ERROR_OUT_OF_MEMORY;
			PDUMPCOMMENT(psDevData->psConnection, "Device mem allocation failed: out of memory.", IMG_TRUE);

			goto MEMINFO_CLEANUP_MEMINFO_ALLOC;
		}
	}

	*psMemInfo  = sOut.sClientMemInfo;

	if(bNoCpuMapping)
	{
		psMemInfo->pvLinAddr = IMG_NULL;
	}
	else
	{
		eError = PVRPMapKMem(psDevData->psConnection->hServices,
									 (IMG_VOID**)&psMemInfo->pvLinAddr,
									 psMemInfo->pvLinAddrKM,
									 &psMemInfo->hMappingInfo,
									 psMemInfo->hKernelMemInfo);
		if (!psMemInfo->pvLinAddr || (eError != PVRSRV_OK))
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocDeviceMem : PVRPMapKMem failed for buffer "));
			PDUMPCOMMENT(psDevData->psConnection, "Device mem kernel mapping failed.", IMG_TRUE);

			eError = PVRSRV_ERROR_BAD_MAPPING;

			goto MEMINFO_CLEANUP_SYNCINFO_ALLOC;
		}
	}

	if(ui32Attribs & PVRSRV_MEM_NO_SYNCOBJ)
	{
		psMemInfo->psClientSyncInfo = IMG_NULL;
	}
	else
	{
		*psSyncInfo = sOut.sClientSyncInfo;		/* PRQA S 505 */ /* apparent null ptr */

#if !defined(PVRSRV_DISABLE_UM_SYNCOBJ_MAPPINGS)
		eError = PVRPMapKMem(psDevData->psConnection->hServices,
									 (IMG_VOID**)&psSyncInfo->psSyncData,
									 psSyncInfo->psSyncData,
									 &psSyncInfo->hMappingInfo,
									 psSyncInfo->hKernelSyncInfo);

		if (!psSyncInfo->psSyncData || (eError != PVRSRV_OK))
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocDeviceMem : PVRPMapKMem failed for syncdata"));
			PDUMPCOMMENT(psDevData->psConnection, "Device mem kernel mapping failed.", IMG_TRUE);

			eError = PVRSRV_ERROR_BAD_MAPPING;

			goto MEMINFO_CLEANUP_LINADDR;
		}
#endif
		psMemInfo->psClientSyncInfo = psSyncInfo;
	}


	*ppsMemInfo = psMemInfo;

	goto MEMINFO_OK;

#if !defined(PVRSRV_DISABLE_UM_SYNCOBJ_MAPPINGS)
	/* NB: this call has nothing to do with sync object mappings, but
	   this exit point becomes redundant once sync object mappings
	   are forbidden */
MEMINFO_CLEANUP_LINADDR:
	PVRUnMapKMem(psDevData->psConnection->hServices, psMemInfo->hMappingInfo, psMemInfo->hKernelMemInfo);
#endif

MEMINFO_CLEANUP_SYNCINFO_ALLOC:
	if(psSyncInfo)
	{
		PVRSRVFreeUserModeMem(psSyncInfo);
	}

MEMINFO_CLEANUP_MEMINFO_ALLOC:
	PVRSRVFreeUserModeMem(psMemInfo);

MEMINFO_CLEANUP_MEM_ALLOC:

	sInFailure.hDevCookie = psDevData->hDevCookie;
#if defined (SUPPORT_SID_INTERFACE)
	sInFailure.hKernelMemInfo = sOut.sClientMemInfo.hKernelMemInfo;
#else
	sInFailure.psKernelMemInfo = sOut.sClientMemInfo.hKernelMemInfo;
#endif

	PVRSRVBridgeCall(psDevData->psConnection->hServices,
					 PVRSRV_BRIDGE_FREE_DEVICEMEM,
					 &sInFailure,
					 sizeof(PVRSRV_BRIDGE_IN_FREEDEVICEMEM),
					 (IMG_VOID *)&sRet,
					 sizeof(PVRSRV_BRIDGE_RETURN));

	*ppsMemInfo = IMG_NULL;

MEMINFO_OK:
	return eError;
}


/*!
 ******************************************************************************

 @Function	PVRSRVAllocDeviceMem

 @Description	Allocates device memory

 @Input		psDevData : DevData for the device

 @Input		ui32Attribs : Some combination of PVRSRV_MEMFLG_ flags

 @Input		uSize : Number of bytes to allocate

 @Input		ui32Alignment : Alignment of allocation

 @Output	**ppsMemInfo : Receives a pointer to the created MEM_INFO structure

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVAllocDeviceMem(const PVRSRV_DEV_DATA *psDevData,
#if defined (SUPPORT_SID_INTERFACE)
											   IMG_SID	 hDevMemHeap,
#else
											   IMG_HANDLE hDevMemHeap,
#endif
											   IMG_UINT32 ui32Attribs,
											   IMG_SIZE_T uSize,
											   IMG_SIZE_T uAlignment,
											   PVRSRV_CLIENT_MEM_INFO **ppsMemInfo)
{
	return PVRSRVAllocDeviceMem2(psDevData, hDevMemHeap, ui32Attribs, uSize,
								 uAlignment, IMG_NULL, 0, ppsMemInfo);
}
#endif /* OS_PVRSRV_ALLOC_DEVICE_MEM_UM */


#if !defined(OS_PVRSRV_FREE_DEVICE_MEM_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVFreeDeviceMem

 @Description	Frees PVRAllocDeviceMem allocation (including the MemInfo)

 @Input		psDevData

 @Input		psMemInfo

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVFreeDeviceMem(const PVRSRV_DEV_DATA		*psDevData,
											  PVRSRV_CLIENT_MEM_INFO	*psMemInfo)
{
	PVRSRV_BRIDGE_RETURN sRet;
	PVRSRV_BRIDGE_IN_FREEDEVICEMEM sIn;
	PVRSRV_ERROR eError=PVRSRV_OK;
	PVRSRVMemSet(&sRet,0x00,sizeof(sRet));	
	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));		
	
	if(!psDevData || !psMemInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVFreeDeviceMem: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(psMemInfo->ui32Flags & PVRSRV_HAP_GPU_PAGEABLE)
	{
		MemoryResourceCheckGuardAreaAndInvalidateObject(psMemInfo);
	}

	if (psMemInfo->psClientSyncInfo)
	{
		eError = FlushClientOps(psDevData->psConnection,
#if defined (SUPPORT_SID_INTERFACE)
								psMemInfo->psClientSyncInfo->hKernelSyncInfo);
#else
								psMemInfo->psClientSyncInfo);
#endif
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVFreeDeviceMem: FlushClientOps failed"));
			return eError;
		}

#if !defined(PVRSRV_DISABLE_UM_SYNCOBJ_MAPPINGS)
		PVRUnMapKMem(psDevData->psConnection->hServices, psMemInfo->psClientSyncInfo->hMappingInfo, psMemInfo->psClientSyncInfo->hKernelSyncInfo);
#endif
		PVRSRVFreeUserModeMem(psMemInfo->psClientSyncInfo);
		psMemInfo->psClientSyncInfo = IMG_NULL;
	}

	/* If bNoCpuMapping was true when allocating there, might not be a mapping */
	if(psMemInfo->pvLinAddr)
	{
		PVRUnMapKMem(psDevData->psConnection->hServices, psMemInfo->hMappingInfo, psMemInfo->hKernelMemInfo);
		psMemInfo->pvLinAddr = IMG_NULL;
	}

	sIn.hDevCookie = psDevData->hDevCookie;
#if defined (SUPPORT_SID_INTERFACE)
	sIn.hKernelMemInfo  = psMemInfo->hKernelMemInfo;
#else
	sIn.psKernelMemInfo  = psMemInfo->hKernelMemInfo;
#endif

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_FREE_DEVICEMEM,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_FREEDEVICEMEM),
						(IMG_VOID *)&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVFreeDeviceMem: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if (sRet.eError == PVRSRV_OK)
	{
		PVRSRVFreeUserModeMem(psMemInfo);
	}

	return sRet.eError;
}
#endif /* OS_PVRSRV_FREE_DEVICE_MEM_UM */

/*!
 ******************************************************************************

 @Function	PVRSRVRemapToDev

 @Description	Remaps buffer to GPU virtual address space, and updates vaddr ??? XXX

 @Input		psDevData

 @Input		psMemInfo

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVRemapToDev(const PVRSRV_DEV_DATA		*psDevData,
											  PVRSRV_CLIENT_MEM_INFO	*psMemInfo)
{
	PVRSRV_BRIDGE_OUT_REMAP_TO_DEV sOut;
	PVRSRV_BRIDGE_IN_REMAP_TO_DEV sIn;
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));	
	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));		

	if(!psDevData || !psMemInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVRemapToDev: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sIn.hDevCookie = psDevData->hDevCookie;
#if defined (SUPPORT_SID_INTERFACE)
	sIn.hKernelMemInfo  = psMemInfo->hKernelMemInfo;
#else
	sIn.psKernelMemInfo  = psMemInfo->hKernelMemInfo;
#endif

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_REMAP_TO_DEV,
						&sIn, sizeof(PVRSRV_BRIDGE_IN_REMAP_TO_DEV),
						&sOut, sizeof(PVRSRV_BRIDGE_OUT_REMAP_TO_DEV)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVRemapToDev: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError == PVRSRV_OK)
	{
		psMemInfo->sDevVAddr = sOut.sDevVAddr;
	}

	return sOut.eError;
}

/*!
 ******************************************************************************

 @Function	PVRSRVUnmapFromDev

 @Description	Unmaps buffer from GPU virtual address space

 @Input		psDevData

 @Input		psMemInfo

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVUnmapFromDev(const PVRSRV_DEV_DATA		*psDevData,
											  PVRSRV_CLIENT_MEM_INFO	*psMemInfo)
{
	PVRSRV_BRIDGE_RETURN sRet;
	PVRSRV_BRIDGE_IN_UNMAP_FROM_DEV sIn;

	PVRSRVMemSet(&sRet,0x00,sizeof(sRet));	
	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));		

	if(!psDevData || !psMemInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVUnmapFromDev: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sIn.hDevCookie = psDevData->hDevCookie;
#if defined (SUPPORT_SID_INTERFACE)
	sIn.hKernelMemInfo  = psMemInfo->hKernelMemInfo;
#else
	sIn.psKernelMemInfo  = psMemInfo->hKernelMemInfo;
#endif

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_UNMAP_FROM_DEV,
						&sIn, sizeof(PVRSRV_BRIDGE_IN_UNMAP_FROM_DEV),
						&sRet, sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVUnmapFromDev: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sRet.eError;
}

/*!
 ******************************************************************************

 @Function	PVRSRVMultiManageDevMem

 @Description	Manages the Memory Device mapping of multiple buffers at once

 @Input		psDevData

 @Input		psMultiMemDevRequest

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVMultiManageDevMem(const PVRSRV_DEV_DATA	*psDevData,
		IMG_UINT32 ui32ControlFlags, PVRSRV_BRIDGE_IN_MULTI_MANAGE_DEV_MEM * psMultiMemDevRequest,
		IMG_UINT32 *ui32StatusFlags, IMG_UINT32 *ui32IndexError)
{
	IMG_UINT32 reqNum=0;
	PVRSRV_ERROR err = PVRSRV_OK;
	PVRSRV_BRIDGE_OUT_MULTI_MANAGE_DEV_MEM sOut;
	IMG_UINT32 ui32InBufferSize = 0;
	IMG_UINT32 ui32OutBufferSize = 0;
	IMG_UINT32 ui32VariableBufferSizeIn = 0;
	IMG_UINT32 ui32VariableBufferSizeOut = 0;

	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));	

	if(!psDevData || !psMultiMemDevRequest ||
			(psMultiMemDevRequest->ui32NumberOfValidRequests > psMultiMemDevRequest->ui32MaxNumberOfRequests) )
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid parameter", __FUNCTION__));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(!psMultiMemDevRequest->psSharedMemClientMemInfo &&
			(psMultiMemDevRequest->ui32MaxNumberOfRequests >
			sizeof(psMultiMemDevRequest->sMemRequests)/sizeof(psMultiMemDevRequest->sMemRequests[0])))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Request max size too high %d", __FUNCTION__,
				psMultiMemDevRequest->ui32MaxNumberOfRequests));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	for(reqNum = 0; reqNum < psMultiMemDevRequest->ui32NumberOfValidRequests; reqNum++)
	{
		PVRSRV_MANAGE_DEV_MEM_REQUEST *pRequest = &psMultiMemDevRequest->sMemRequests[reqNum];
		PVRSRV_CLIENT_MEM_INFO	*psMemInfo = pRequest->psClientMemInfo;

		if(psMemInfo)
		{
			pRequest->hKernelMemInfo = psMemInfo->hKernelMemInfo;
			pRequest->uiSubSystem = psMemInfo->uiSubSystem;
			if(psMemInfo->psClientSyncInfo)
			{
				pRequest->hKernelSyncInfo = psMemInfo->psClientSyncInfo->hKernelSyncInfo;
			}

			switch(pRequest->eReqType)
			{
				case PVRSRV_MULTI_MANAGE_DEV_MEM_RQST_MAP:
				case PVRSRV_MULTI_MANAGE_DEV_MEM_RQST_LOCK_MAP:
				case PVRSRV_MULTI_MANAGE_DEV_MEM_RQST_SWAP_MAP_TO_NEXT:
					pRequest->ui32Attribs = psMemInfo->ui32Flags;
					pRequest->uSize = psMemInfo->uAllocSize ;
					pRequest->pvLinAddr = psMemInfo->pvLinAddr; 	/* CPU Virtual Address */
					/* Let the kernel know what do we know about GPU VIRT address
					 * - usually is 0x00 */
					pRequest->sDevVAddr = psMemInfo->sDevVAddr;
					break;
				case PVRSRV_MULTI_MANAGE_DEV_MEM_RQST_UNMAP:
				case PVRSRV_MULTI_MANAGE_DEV_MEM_RQST_UNLOCK_MAP:
				case PVRSRV_MULTI_MANAGE_DEV_MEM_RQST_SWAP_MAP_FROM_PREV:
					pRequest->sDevVAddr = psMemInfo->sDevVAddr;		/* GPU Virtual Address */
					break;
				default:
					err = PVRSRV_ERROR_INVALID_PARAMS;
					break;
			}
		}
		else
		{
			err = PVRSRV_ERROR_INVALID_PARAMS;
		}

		if(err != PVRSRV_OK)
		{
			pRequest->eError = PVRSRV_ERROR_INVALID_PARAMS;
			*ui32IndexError = reqNum;
			return err;
		}
	}

	psMultiMemDevRequest->hDevCookie = psDevData->hDevCookie;
	psMultiMemDevRequest->ui32CtrlFlags = ui32ControlFlags;
	if(psMultiMemDevRequest->psSharedMemClientMemInfo && psMultiMemDevRequest->psSharedMemClientMemInfo->hKernelMemInfo)
	{
		ui32VariableBufferSizeIn = ui32VariableBufferSizeOut = 0; /* Not part of the IN/OUT structures */
		psMultiMemDevRequest->hKernelMemInfo = psMultiMemDevRequest->psSharedMemClientMemInfo->hKernelMemInfo;
	}
	else
	{
		ui32VariableBufferSizeIn = psMultiMemDevRequest->ui32NumberOfValidRequests * sizeof(PVRSRV_MANAGE_DEV_MEM_REQUEST);
		ui32VariableBufferSizeOut = psMultiMemDevRequest->ui32NumberOfValidRequests * sizeof(PVRSRV_MANAGE_DEV_MEM_RESPONSE);
	}

	ui32InBufferSize = (sizeof(*psMultiMemDevRequest) - sizeof(psMultiMemDevRequest->sMemRequests)) + ui32VariableBufferSizeIn;
	ui32OutBufferSize = (sizeof(sOut) - sizeof(sOut.sMemResponse)) + ui32VariableBufferSizeOut;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_MULTI_MANAGE_DEV_MEM,
						psMultiMemDevRequest, ui32InBufferSize,
						&sOut, ui32OutBufferSize))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: BridgeCall failed", __FUNCTION__));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError == PVRSRV_OK)
	{
		/* If we do not use shared memory, we need to populate the data back */
		for(reqNum = 0; reqNum < psMultiMemDevRequest->ui32NumberOfValidRequests; reqNum++)
		{
			PVRSRV_MANAGE_DEV_MEM_REQUEST *pRequest = &psMultiMemDevRequest->sMemRequests[reqNum];
			PVRSRV_MANAGE_DEV_MEM_RESPONSE *pResponse =
					(psMultiMemDevRequest->hKernelMemInfo == 0) ? &sOut.sMemResponse[reqNum] : pRequest;
			PVRSRV_CLIENT_MEM_INFO	*psMemInfo = pRequest->psClientMemInfo;

			/* At the kernel size, psClientMemInfo only works as a verification token */
			PVR_ASSERT(pResponse->psClientMemInfo == pRequest->psClientMemInfo);
			PVR_ASSERT(pResponse->eReqType == pRequest->eReqType);

			if(psMultiMemDevRequest->hKernelMemInfo == 0)
			{
				pRequest->eError = pResponse->eError;
				pRequest->ui32GpuMapRefCount = pResponse->ui32GpuMapRefCount;
			}

			if(pResponse->eError == PVRSRV_OK)
			{
				/* TODO: Remove me - for bridge testing only */
				PVR_DPF((PVR_DBG_VERBOSE, "Device Address Client 0x%08x (%d) -> Kernel 0x%08x (%d)",
							pRequest->psClientMemInfo->sDevVAddr.uiAddr,
							pRequest->ui32GpuMapRefCount,
							pResponse->sDevVAddr.uiAddr,
							pResponse->ui32GpuMapRefCount));

				switch(pResponse->eReqType)
				{
					case PVRSRV_MULTI_MANAGE_DEV_MEM_RQST_MAP:
					case PVRSRV_MULTI_MANAGE_DEV_MEM_RQST_LOCK_MAP:
					case PVRSRV_MULTI_MANAGE_DEV_MEM_RQST_SWAP_MAP_TO_NEXT:
						if((pResponse->ui32GpuMapRefCount == 0) ||
								(pResponse->sDevVAddr.uiAddr == PVRSRV_BAD_DEVICE_ADDRESS))
						{
							PVR_DPF((PVR_DBG_ERROR, "Map Device Address ref count is %d, DevVAddr = 0x%08x",
									pResponse->ui32GpuMapRefCount, pResponse->sDevVAddr.uiAddr));
						}
					case PVRSRV_MULTI_MANAGE_DEV_MEM_RQST_UNMAP:
					case PVRSRV_MULTI_MANAGE_DEV_MEM_RQST_UNLOCK_MAP:
					case PVRSRV_MULTI_MANAGE_DEV_MEM_RQST_SWAP_MAP_FROM_PREV:
						psMemInfo->sDevVAddr = pResponse->sDevVAddr;		/* GPU Virtual Address */
						if((pResponse->ui32GpuMapRefCount == 0) &&
								(pResponse->sDevVAddr.uiAddr != PVRSRV_BAD_DEVICE_ADDRESS))
						{
							PVR_DPF((PVR_DBG_ERROR, "Device Address ref count is %d when DevVAddr = 0x%08x",
									pResponse->ui32GpuMapRefCount, pResponse->sDevVAddr.uiAddr));
						}
						break;
					default:
						err = PVRSRV_ERROR_INVALID_PARAMS;
						break;
				}
			}
		}

		PVR_ASSERT(psMultiMemDevRequest->ui32CtrlFlags == sOut.ui32CtrlFlags);
		psMultiMemDevRequest->ui32StatusFlags = sOut.ui32StatusFlags;
	}

	if(sOut.eError == PVRSRV_OK)
	{
		*ui32StatusFlags = sOut.ui32StatusFlags;
	}
	else
	{
		*ui32IndexError = sOut.ui32IndexError;
	}

	return sOut.eError;
}

/*!
 ******************************************************************************

 @Function	PVRSRVManageDevMem

 @Description	Manages the Memory Device mapping

 @Input		psDevData

 @Input		psMemInfo - client's device memory

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVManageDevMem(const PVRSRV_DEV_DATA	*psDevData,
		PVRSRV_MULTI_MANAGE_DEV_MEM_RQST_TYPE eReq, PVRSRV_CLIENT_MEM_INFO *psMemInfo,
		IMG_UINT32 *pui32StatusFlags)
{
	PVRSRV_BRIDGE_IN_MULTI_MANAGE_DEV_MEM sMultiMemDevRequest;
	IMG_UINT32 ui32IndexError = (IMG_UINT32)-1;

	PVRSRVMemSet(&sMultiMemDevRequest,0x00,sizeof(sMultiMemDevRequest));	
	
	if(!psDevData || !psMemInfo)
		return PVRSRV_ERROR_INVALID_PARAMS;

	PVRSRVMemSet(&sMultiMemDevRequest, 0x00,sizeof(sMultiMemDevRequest));

	sMultiMemDevRequest.ui32MaxNumberOfRequests = sMultiMemDevRequest.ui32NumberOfValidRequests = 1;
	sMultiMemDevRequest.sMemRequests[0].eReqType = eReq;
	sMultiMemDevRequest.sMemRequests[0].psClientMemInfo = psMemInfo;
	sMultiMemDevRequest.sMemRequests[0].ui32FieldSize = sizeof(PVRSRV_MANAGE_DEV_MEM_REQUEST);

	return PVRSRVMultiManageDevMem(psDevData, 0, &sMultiMemDevRequest,
			pui32StatusFlags, &ui32IndexError);

}


/*!
 ******************************************************************************

 @Function	PVRSRVManageDevMemSwapGpuVirtAddr

 @Description	Swaps the GPU virtual addresses from one set of buffers to another

 @Input		psMemInfoSourceArray, psMemInfoTargetArray, ui32NumBuff

 @Input		psMemInfo - client's device memory

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVManageDevMemSwapGpuVirtAddr(const PVRSRV_DEV_DATA	*psDevData,
		PVRSRV_CLIENT_MEM_INFO *psMemInfoSourceArray, PVRSRV_CLIENT_MEM_INFO *psMemInfoTargetArray,
		IMG_UINT32 ui32NumBuff, IMG_UINT32 *ui32StatusFlags)
{
	PVR_UNREFERENCED_PARAMETER(ui32StatusFlags);

	if(!psDevData || !psMemInfoSourceArray || !psMemInfoTargetArray || !ui32NumBuff)
		return PVRSRV_ERROR_INVALID_PARAMS;

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

#if !defined(OS_PVRSRV_EXPORT_DEVICE_MEM_UM) && !defined(SRVINIT_MODULE)
/*!
 ******************************************************************************

 @Function		PVRSRVExportDeviceMem

 @Description	Exports an PVRSRVAllocDeviceMem allocation to other processes

 @Input			psDevData

 @Input			psMemInfo

 @Output		phMeminfo

 @Return		PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVExportDeviceMem(const PVRSRV_DEV_DATA	*psDevData,
												PVRSRV_CLIENT_MEM_INFO	*psMemInfo,
#if defined (SUPPORT_SID_INTERFACE)
												IMG_SID					*phMemInfo)
#else
												IMG_HANDLE				*phMemInfo)
#endif
{
	PVRSRV_BRIDGE_OUT_EXPORTDEVICEMEM sOut;
	PVRSRV_BRIDGE_IN_EXPORTDEVICEMEM sIn;

	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));	
	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));		

	if(!psDevData || !psMemInfo || !phMemInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVExportDeviceMem: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sIn.hDevCookie = psDevData->hDevCookie;
#if defined (SUPPORT_SID_INTERFACE)
	sIn.hKernelMemInfo = psMemInfo->hKernelMemInfo;
#else
	sIn.psKernelMemInfo = psMemInfo->hKernelMemInfo;
#endif

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_EXPORT_DEVICEMEM,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_EXPORTDEVICEMEM),
						(IMG_VOID *)&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_EXPORTDEVICEMEM)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVExportDeviceMem: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError == PVRSRV_OK)
	{
		*phMemInfo = sOut.hMemInfo;
	}

	return sOut.eError;
}
#endif /* OS_PVRSRV_EXPORT_DEVICE_MEM_UM */


#if !defined(OS_PVRSRV_UNREF_DEVICE_MEM_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVUnrefDeviceMem

 @Description

 Unreferences memory allocated with PVRAllocDeviceMem.  Unlike
 PVRSRVFreeDeviceMEM, the device memory and the kernel meminfo are not
 freed.  We are just releasing references to the memory in user space.

 @Input		psDevData

 @Input		psMemInfo

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_INTERNAL
PVRSRV_ERROR IMG_CALLCONV PVRSRVUnrefDeviceMem(const PVRSRV_DEV_DATA *psDevData,
											   PVRSRV_CLIENT_MEM_INFO	*psMemInfo)
{
	if(!psDevData || !psMemInfo )
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVUnrefDeviceMem: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (psMemInfo->psClientSyncInfo)
	{
#if !defined (PVRSRV_DISABLE_UM_SYNCOBJ_MAPPINGS)
		PVRUnMapKMem(psDevData->psConnection->hServices, psMemInfo->psClientSyncInfo->hMappingInfo, psMemInfo->psClientSyncInfo->hKernelSyncInfo);
#endif

		PVRSRVFreeUserModeMem(psMemInfo->psClientSyncInfo);
	}

	if(psMemInfo->pvLinAddr)
	{
		PVRUnMapKMem(psDevData->psConnection->hServices, psMemInfo->hMappingInfo, psMemInfo->hKernelMemInfo);
	}

#ifdef DEBUG
	/*
	 * Typically, the caller needs to save the kernel meminfo handle
	 * in order to pass it to the kernel part of services, so the
	 * handle needs to be saved somewhere before calling this function.
	 * The following tries to catch attempts to use the kernel meminfo
	 * handle after the memory containing it has been freed.
	 */
	psMemInfo->hKernelMemInfo = IMG_NULL;
#endif
	PVRSRVFreeUserModeMem(psMemInfo);

	return PVRSRV_OK;
}
#endif /* OS_PVRSRV_UNREF_DEVICE_MEM_UM */


#if !defined(OS_PVRSRV_WRAP_EXT_MEM_UM) && !defined(SRVINIT_MODULE)
/******************************************************************************
  @Function		PVRSRVWrapExtMemory

  @Description

  Allocates a Device Virtual Address in the shared mapping heap
  and maps physical pages into that allocation. Note, if the pages are
  already mapped into the heap, the existing allocation is returned.

  @Input	psDevData - pointer to device data structure
  @Input	hDevMemContext - device memory context
  @input	ui32ByteSize - Size of allocation
  @input	ui32PageOffset - Offset into the first page of the memory to be wrapped
  @input	bPhysContig - whether the underlying memory is physically contiguous
  @input	psSysPAddr - list of Device Physical page addresses
  @input	pvLinAddr - ptr to buffer to wrap
  @Input	ui32Flags - PVRSRV_MEM_ flags for wrap (can be 0)
  @Input	ppsMemInfo - location for returned client mem info strucure

  @Output	ppsMemInfo - pointer to client mem info structure

  @Return PVRSRV_ERROR:
 *******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR PVRSRVWrapExtMemory(const PVRSRV_DEV_DATA *psDevData,
#if defined (SUPPORT_SID_INTERFACE)
								 IMG_SID hDevMemContext,
#else
								 IMG_HANDLE hDevMemContext,
#endif
								 IMG_SIZE_T ui32ByteSize,
								 IMG_SIZE_T ui32PageOffset,
								 IMG_BOOL bPhysContig,
								 IMG_SYS_PHYADDR *psSysPAddr,
								 IMG_VOID *pvLinAddr,
								 IMG_UINT32 ui32Flags,
								 PVRSRV_CLIENT_MEM_INFO **ppsMemInfo)
{
	PVRSRV_CLIENT_MEM_INFO  *psMemInfo=IMG_NULL;
	PVRSRV_CLIENT_SYNC_INFO *psSyncInfo=IMG_NULL;
	PVRSRV_ERROR eError=PVRSRV_OK;
	IMG_UINT32 i;
	PVRSRV_BRIDGE_IN_WRAP_EXT_MEMORY sIn;
	PVRSRV_BRIDGE_OUT_WRAP_EXT_MEMORY sOut;
#if !defined(PVRSRV_DISABLE_UM_SYNCOBJ_MAPPINGS)
	PVRSRV_BRIDGE_IN_UNWRAP_EXT_MEMORY sInFailure;
	PVRSRV_BRIDGE_RETURN sRet;
#endif

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));	
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));	


	if(!psDevData || (!psSysPAddr && !pvLinAddr) || !ppsMemInfo || !hDevMemContext)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVWrapExtMemory: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psMemInfo = (PVRSRV_CLIENT_MEM_INFO *)PVRSRVAllocUserModeMem(sizeof(PVRSRV_CLIENT_MEM_INFO));
	if(psMemInfo == IMG_NULL)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psSyncInfo = (PVRSRV_CLIENT_SYNC_INFO *)PVRSRVAllocUserModeMem(sizeof(PVRSRV_CLIENT_SYNC_INFO));

	if (!psSyncInfo)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;

		goto MEMINFO_CLEANUP;
	}

	sIn.hDevCookie = psDevData->hDevCookie;
	sIn.hDevMemContext = hDevMemContext;
	sIn.ui32ByteSize =  ui32ByteSize;
	sIn.ui32PageOffset = ui32PageOffset;
	sIn.bPhysContig = bPhysContig;
	sIn.pvLinAddr = pvLinAddr;
	sIn.ui32Flags = ui32Flags;

   	if(psSysPAddr)
	{
		sIn.ui32NumPageTableEntries = (IMG_UINT32)((bPhysContig)
														? 1
														: (ui32ByteSize + ui32PageOffset + 4095) >> 12);

		sIn.psSysPAddr = (IMG_SYS_PHYADDR *)PVRSRVAllocUserModeMem(sizeof(IMG_SYS_PHYADDR) * sIn.ui32NumPageTableEntries);

		if (sIn.psSysPAddr == IMG_NULL)
		{
			eError = PVRSRV_ERROR_OUT_OF_MEMORY;
			goto SYNCINFO_CLEANUP;
		}

		for (i = 0; i < sIn.ui32NumPageTableEntries; i++)
		{
			sIn.psSysPAddr[i] = psSysPAddr[i];
		}
	}
	else
	{
		sIn.ui32NumPageTableEntries = 0;
		sIn.psSysPAddr = IMG_NULL;
	}

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_WRAP_EXT_MEMORY,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_WRAP_EXT_MEMORY),
						&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_WRAP_EXT_MEMORY)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVWrapExtMemory: BridgeCall failed"));
		eError = PVRSRV_ERROR_BRIDGE_CALL_FAILED;
		goto PAGETABLE_CLEANUP;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVWrapExtMemory: Error %d returned", sOut.eError));
		eError = sOut.eError;
		goto PAGETABLE_CLEANUP;
	}

	*psMemInfo  = sOut.sClientMemInfo;
	*psSyncInfo = sOut.sClientSyncInfo;

#if !defined(PVRSRV_DISABLE_UM_SYNCOBJ_MAPPINGS)
	eError = PVRPMapKMem(psDevData->psConnection->hServices,
								 (IMG_VOID**)&psSyncInfo->psSyncData,
								 psSyncInfo->psSyncData,
								 &psSyncInfo->hMappingInfo,
								 psSyncInfo->hKernelSyncInfo);

	if (!psSyncInfo->psSyncData || (eError != PVRSRV_OK))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVWrapExtMemory : PVRPMapKMem failed for syncdata "));

		eError = PVRSRV_ERROR_BAD_MAPPING;

		goto CLEANUP_LINADDR;
	}
#endif
	psMemInfo->psClientSyncInfo = psSyncInfo;

	*ppsMemInfo = psMemInfo;

	PVRSRVFreeUserModeMem(sIn.psSysPAddr);

	return PVRSRV_OK;

#if !defined(PVRSRV_DISABLE_UM_SYNCOBJ_MAPPINGS)
	/* redundant exit path */
CLEANUP_LINADDR:
	sInFailure.hKernelMemInfo = sOut.sClientMemInfo.hKernelMemInfo;

	PVRSRVBridgeCall(psDevData->psConnection->hServices,
					 PVRSRV_BRIDGE_UNWRAP_EXT_MEMORY,
					 &sInFailure,
					 sizeof(PVRSRV_BRIDGE_IN_UNWRAP_EXT_MEMORY),
					 (IMG_VOID *)&sRet,
					 sizeof(PVRSRV_BRIDGE_RETURN));
#endif
PAGETABLE_CLEANUP:
	PVRSRVFreeUserModeMem(sIn.psSysPAddr);
SYNCINFO_CLEANUP:
	PVRSRVFreeUserModeMem(psSyncInfo);
MEMINFO_CLEANUP:
	PVRSRVFreeUserModeMem(psMemInfo);
	*ppsMemInfo = IMG_NULL;
	return eError;
}
#endif /* OS_PVRSRV_WRAP_EXT_MEM_UM */


#if !defined(OS_PVRSRV_UNWRAP_EXT_MEM_UM) && !defined(SRVINIT_MODULE)
/*!
 ******************************************************************************

 @Function	PVRSRVUnwrapExtMemory

 @Description	Reverse of PVRSRVWrapExtMemory

 @Input		psDevData

 @Input		psMemInfo

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR PVRSRVUnwrapExtMemory(const PVRSRV_DEV_DATA *psDevData,
								   PVRSRV_CLIENT_MEM_INFO *psMemInfo)
{
	PVRSRV_BRIDGE_IN_UNWRAP_EXT_MEMORY sIn;
	PVRSRV_BRIDGE_RETURN sOut;
	
	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));	
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));	
	
	if(!psDevData || !psMemInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVUnwrapExtMemory: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sIn.hKernelMemInfo = psMemInfo->hKernelMemInfo;
	sIn.sClientMemInfo = *psMemInfo;

	if(psMemInfo->psClientSyncInfo)
	{
		PVRSRV_ERROR eError;

		eError = FlushClientOps(psDevData->psConnection,
#if defined (SUPPORT_SID_INTERFACE)
								psMemInfo->psClientSyncInfo->hKernelSyncInfo);
#else
								psMemInfo->psClientSyncInfo);
#endif
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVUnwrapExtMemory: FlushClientOps failed"));
			return eError;
		}

		sIn.sClientSyncInfo = *psMemInfo->psClientSyncInfo;
#if !defined(PVRSRV_DISABLE_UM_SYNCOBJ_MAPPINGS)
		PVRUnMapKMem(psDevData->psConnection->hServices, psMemInfo->psClientSyncInfo->hMappingInfo, psMemInfo->psClientSyncInfo->hKernelSyncInfo);
#endif
	}

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_UNWRAP_EXT_MEMORY,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_UNWRAP_EXT_MEMORY),
						&sOut,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVUnwrapExtMemory: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if (sOut.eError == PVRSRV_OK)
	{
		PVRSRVFreeUserModeMem(psMemInfo->psClientSyncInfo);
		PVRSRVFreeUserModeMem(psMemInfo);
	}

	return sOut.eError;
}
#endif /* OS_PVRSRV_UNWRAP_EXT_MEM_UM */


#if !defined(OS_PVRSRV_MAP_DEVCLASS_MEM_UM) && !defined(SRVINIT_MODULE)
/*!
 ******************************************************************************

 @Function	PVRSRVMapDeviceClassMemory

 @Description

 @Input		psDevData
 @Input		hDevMemContext
 @Input		hDeviceClassBuffer
 @Input		psMemInfo

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVMapDeviceClassMemory(const PVRSRV_DEV_DATA *psDevData,
#if defined (SUPPORT_SID_INTERFACE)
													 IMG_SID    hDevMemContext,
													 IMG_SID    hDeviceClassBuffer,
#else
													 IMG_HANDLE hDevMemContext,
													 IMG_HANDLE hDeviceClassBuffer,
#endif
													 PVRSRV_CLIENT_MEM_INFO **ppsMemInfo)
{
	PVRSRV_BRIDGE_IN_MAP_DEVICECLASS_MEMORY sBridgeIn;
	PVRSRV_BRIDGE_OUT_MAP_DEVICECLASS_MEMORY sBridgeOut;
	PVRSRV_CLIENT_MEM_INFO *psMemInfo= IMG_NULL;
	PVRSRV_ERROR eError=PVRSRV_OK;
	PVRSRV_BRIDGE_IN_UNMAP_DEVICECLASS_MEMORY sInFailure;
	PVRSRV_BRIDGE_RETURN sRet;
	PVRSRV_CLIENT_SYNC_INFO *psSyncInfo = IMG_NULL;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));	
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));		
	PVRSRVMemSet(&sRet,0x00,sizeof(sRet));			
	PVRSRVMemSet(&sInFailure,0x00,sizeof(sInFailure));				
	
	if(!psDevData || !ppsMemInfo || !hDeviceClassBuffer)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVMapDeviceClassMemory: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hDevMemContext = hDevMemContext;
	sBridgeIn.hDeviceClassBuffer = hDeviceClassBuffer;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_MAP_DEVICECLASS_MEMORY,
						&sBridgeIn,
						sizeof(PVRSRV_BRIDGE_IN_MAP_DEVICECLASS_MEMORY),
						&sBridgeOut,
						sizeof(PVRSRV_BRIDGE_OUT_MAP_DEVICECLASS_MEMORY)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVMapDeviceClassMemory: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sBridgeOut.eError != PVRSRV_OK)
	{
		return sBridgeOut.eError;
	}

	/* If eviction is supported, allocate an Resource Object instead */
	if(sBridgeOut.sClientMemInfo.ui32Flags & PVRSRV_HAP_GPU_PAGEABLE)
		psMemInfo = (PVRSRV_CLIENT_MEM_INFO *)PVRSRVCallocUserModeMem(sizeof(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT));
	else
		psMemInfo = (PVRSRV_CLIENT_MEM_INFO *)PVRSRVAllocUserModeMem(sizeof(PVRSRV_CLIENT_MEM_INFO));

	if(psMemInfo == IMG_NULL)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;

		goto MAP_DEVICECLASS_CLEANUP_MAP;
	}

	if(sBridgeOut.sClientSyncInfo.hKernelSyncInfo)
	{
		psSyncInfo = (PVRSRV_CLIENT_SYNC_INFO *)PVRSRVAllocUserModeMem(sizeof(PVRSRV_CLIENT_SYNC_INFO));

		if (!psSyncInfo)
		{
			eError = PVRSRV_ERROR_OUT_OF_MEMORY;

			goto MAP_DEVICECLASS_CLEANUP_MEMINFO_ALLOC;
		}
	}

	*psMemInfo  = sBridgeOut.sClientMemInfo;

	eError = PVRPMapKMem(psDevData->psConnection->hServices,
								 (IMG_VOID**)&psMemInfo->pvLinAddr,
								 psMemInfo->pvLinAddrKM,
								 &psMemInfo->hMappingInfo,
								 psMemInfo->hKernelMemInfo);

	if (!psMemInfo->pvLinAddr || (eError != PVRSRV_OK))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVMapDeviceClassMemory : PVRPMapKMem failed for buffer "));

		eError = PVRSRV_ERROR_BAD_MAPPING;

		goto MAP_DEVICECLASS_CLEANUP_SYNCINFO_ALLOC;
	}

	if(psSyncInfo)
	{
		*psSyncInfo = sBridgeOut.sClientSyncInfo;

#if !defined(PVRSRV_DISABLE_UM_SYNCOBJ_MAPPINGS)
		eError = PVRPMapKMem(psDevData->psConnection->hServices,
									(IMG_VOID**)&psSyncInfo->psSyncData,
									psSyncInfo->psSyncData,
									&psSyncInfo->hMappingInfo,
									psSyncInfo->hKernelSyncInfo);

		if (!psSyncInfo->psSyncData || (eError != PVRSRV_OK))
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVMapDeviceClassMemory : PVRPMapKMem failed for syncdata "));

			eError = PVRSRV_ERROR_BAD_MAPPING;

			goto MAP_DEVICECLASS_CLEANUP_LINADDR;
		}
#endif

		psMemInfo->psClientSyncInfo = psSyncInfo;
	}


	*ppsMemInfo = psMemInfo;

	goto MAP_DEVICECLASS_OK;

#if !defined(PVRSRV_DISABLE_UM_SYNCOBJ_MAPPINGS)
	/* NB: this exit path becomes redundant without s.o. mappings */
MAP_DEVICECLASS_CLEANUP_LINADDR:
	PVRUnMapKMem(psDevData->psConnection->hServices, psMemInfo->hMappingInfo, psMemInfo->hKernelMemInfo);
#endif

MAP_DEVICECLASS_CLEANUP_SYNCINFO_ALLOC:
	if(psSyncInfo)
	{
		PVRSRVFreeUserModeMem(psSyncInfo);
	}

MAP_DEVICECLASS_CLEANUP_MEMINFO_ALLOC:
	PVRSRVFreeUserModeMem(psMemInfo);

MAP_DEVICECLASS_CLEANUP_MAP:
#if defined (SUPPORT_SID_INTERFACE)
	sInFailure.hKernelMemInfo = sBridgeOut.sClientMemInfo.hKernelMemInfo;
#else
	sInFailure.psKernelMemInfo = sBridgeOut.sClientMemInfo.hKernelMemInfo;
#endif

	PVRSRVBridgeCall(psDevData->psConnection->hServices,
					 PVRSRV_BRIDGE_UNMAP_DEVICECLASS_MEMORY,
					 &sInFailure,
					 sizeof(PVRSRV_BRIDGE_IN_UNMAP_DEVICECLASS_MEMORY),
					 (IMG_VOID *)&sRet,
					 sizeof(PVRSRV_BRIDGE_RETURN));

	*ppsMemInfo = IMG_NULL;

MAP_DEVICECLASS_OK:
	return eError;
}
#endif /* OS_PVRSRV_MAP_DEVCLASS_MEM_UM */


#if !defined(OS_PVRSRV_UNMAP_DEVCLASS_MEM_UM) && !defined(SRVINIT_MODULE)
/*!
 ******************************************************************************

 @Function	PVRSRVUnmapDeviceClassMemory

 @Description

 @Input		psDevData

 @Input		psMemInfo

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR PVRSRVUnmapDeviceClassMemory(const PVRSRV_DEV_DATA *psDevData,
										  PVRSRV_CLIENT_MEM_INFO *psMemInfo)
{
	PVRSRV_BRIDGE_IN_UNMAP_DEVICECLASS_MEMORY sBridgeIn;
	PVRSRV_BRIDGE_RETURN sBridgeOut;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));				
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));				

	if(!psDevData || !psMemInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVUnmapDeviceClassMemory: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

#if defined (SUPPORT_SID_INTERFACE)
	sBridgeIn.hKernelMemInfo = psMemInfo->hKernelMemInfo;
#else
	sBridgeIn.psKernelMemInfo = psMemInfo->hKernelMemInfo;
#endif
	sBridgeIn.sClientMemInfo = *psMemInfo;

	if(psMemInfo->psClientSyncInfo)
	{
		PVRSRV_ERROR eError;

		sBridgeIn.sClientSyncInfo = *psMemInfo->psClientSyncInfo;

		eError = FlushClientOps(psDevData->psConnection,
#if defined (SUPPORT_SID_INTERFACE)
								psMemInfo->psClientSyncInfo->hKernelSyncInfo);
#else
								psMemInfo->psClientSyncInfo);
#endif
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVUnmapDeviceClassMemory: FlushClientOps failed"));
			return eError;
		}

#if !defined(PVRSRV_DISABLE_UM_SYNCOBJ_MAPPINGS)
		PVRUnMapKMem(psDevData->psConnection->hServices,
						psMemInfo->psClientSyncInfo->hMappingInfo,
						psMemInfo->psClientSyncInfo->hKernelSyncInfo);
#endif

		PVRSRVFreeUserModeMem(psMemInfo->psClientSyncInfo);
	}

	PVRUnMapKMem(psDevData->psConnection->hServices, psMemInfo->hMappingInfo, psMemInfo->hKernelMemInfo);

	PVRSRVFreeUserModeMem(psMemInfo);

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_UNMAP_DEVICECLASS_MEMORY,
						&sBridgeIn,
						sizeof(PVRSRV_BRIDGE_IN_UNMAP_DEVICECLASS_MEMORY),
						&sBridgeOut,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVUnmapDeviceClassMemory: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sBridgeOut.eError != PVRSRV_OK)
	{
		return sBridgeOut.eError;
	}


	return PVRSRV_OK;
}
#endif /* OS_PVRSRV_UNMAP_DEVCLASS_MEM_UM */



#if !defined(SRVINIT_MODULE)

/*!
 ******************************************************************************

 @Function	PVRSRVGetMiscInfo

 @Description	Retrieves miscellaneous information from services

 @Input		psConnection

 @Output	psMiscInfo

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVGetMiscInfo(const PVRSRV_CONNECTION *psConnection,
											PVRSRV_MISC_INFO *psMiscInfo)
{
	PVRSRV_BRIDGE_IN_GET_MISC_INFO	sIn;
	PVRSRV_BRIDGE_OUT_GET_MISC_INFO	sOut;
#if defined (SUPPORT_SID_INTERFACE)
#else
	PVRSRV_CLIENT_MEM_INFO *psClientMemInfo = IMG_NULL;
	PVRSRV_CLIENT_MEM_INFO *psClientMemInfo2 = IMG_NULL;
#endif

	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));
	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));

	if(!psMiscInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetMiscInfo: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sOut.eError = PVRSRV_OK;

	/* some requests can be handled in usermode */

	if(psMiscInfo->ui32StateRequest & PVRSRV_MISC_INFO_CPUCACHEOP_PRESENT)
	{
		/* If we're not deferring the op, and the op is a flush, we may be
		 * able to service it immediately (x86 clflush only, so far).
		 */
		if(!psMiscInfo->sCacheOpCtl.bDeferOp)
		{
			if(OSFlushCPUCacheRange(psMiscInfo->sCacheOpCtl.pvBaseVAddr,
									(IMG_VOID *)((IMG_CHAR *)psMiscInfo->sCacheOpCtl.pvBaseVAddr +
									psMiscInfo->sCacheOpCtl.ui32Length)) == PVRSRV_OK)
			{
				psMiscInfo->ui32StateRequest &= ~PVRSRV_MISC_INFO_CPUCACHEOP_PRESENT;
			}
		}

#if defined (SUPPORT_SID_INTERFACE)
#else
		/* If we've been given a client meminfo, turn it into
		 * a kernel meminfo (handle) for the bridge call.
		 */
		if(psMiscInfo->sCacheOpCtl.u.psClientMemInfo)
		{
			psClientMemInfo = psMiscInfo->sCacheOpCtl.u.psClientMemInfo;
			psMiscInfo->sCacheOpCtl.u.psKernelMemInfo = (PVRSRV_KERNEL_MEM_INFO *)
				psMiscInfo->sCacheOpCtl.u.psClientMemInfo->hKernelMemInfo;
		}
	}

	if(psMiscInfo->ui32StateRequest & PVRSRV_MISC_INFO_GET_REF_COUNT_PRESENT)
	{
		psClientMemInfo2 = psMiscInfo->sGetRefCountCtl.u.psClientMemInfo;
		psMiscInfo->sGetRefCountCtl.u.psKernelMemInfo = (PVRSRV_KERNEL_MEM_INFO *)
			psMiscInfo->sGetRefCountCtl.u.psClientMemInfo->hKernelMemInfo;
#endif
	}

	/* otherwise, we need to make a bridge call */

	if(psMiscInfo->ui32StateRequest != 0UL)
	{
		sIn.sMiscInfo = *psMiscInfo;

		if(PVRSRVBridgeCall(psConnection->hServices,
							PVRSRV_BRIDGE_GET_MISC_INFO,
							&sIn,
							sizeof(PVRSRV_BRIDGE_IN_GET_MISC_INFO),
							&sOut,
							sizeof(PVRSRV_BRIDGE_OUT_GET_MISC_INFO)))
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetMiscInfo: BridgeCall failed"));
			return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
		}

		if(sOut.eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetMiscInfo: Error %d returned", sOut.eError));
			return sOut.eError;
		}

		*psMiscInfo = sOut.sMiscInfo;
	}

#if defined (SUPPORT_SID_INTERFACE)
#else
	/* Caller doesn't expect this field to be corrupted */
	if(psMiscInfo->ui32StateRequest & PVRSRV_MISC_INFO_GET_REF_COUNT_PRESENT)
	{
		psMiscInfo->sGetRefCountCtl.u.psClientMemInfo = psClientMemInfo2;
	}

	/* Caller doesn't expect this field to be corrupted */
	if(psMiscInfo->ui32StatePresent & PVRSRV_MISC_INFO_CPUCACHEOP_PRESENT)
	{
		psMiscInfo->sCacheOpCtl.u.psClientMemInfo = psClientMemInfo;
	}
#endif

	if(psMiscInfo->ui32StatePresent & PVRSRV_MISC_INFO_TIMER_PRESENT)
	{
		sOut.eError = PVRPMapKMem(psConnection->hServices,
										  (IMG_VOID**)&psMiscInfo->pvSOCTimerRegisterUM,
										  psMiscInfo->pvSOCTimerRegisterKM,
										  &psMiscInfo->hSOCTimerRegisterMappingInfo,
										  psMiscInfo->hSOCTimerRegisterOSMemHandle);
	}

	if(psMiscInfo->ui32StatePresent & PVRSRV_MISC_INFO_GLOBALEVENTOBJECT_PRESENT)
	{
		sOut.eError = OSEventObjectOpen(psConnection,
										&psMiscInfo->sGlobalEventObject,
										&psMiscInfo->hOSGlobalEvent);

		if (sOut.eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetMiscInfo: Failed PVRSRVEventObjectOpen (%d)", sOut.eError));
		} 
	}

	return sOut.eError;
}
#endif /* !defined(SRVINIT_MODULE) */


#if !defined(SRVINIT_MODULE)

/*!
 ******************************************************************************

 @Function	PVRSRVReleaseMiscInfo

 @Description	Releases miscellaneous information

 @Input		psMiscInfo

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVReleaseMiscInfo(const PVRSRV_CONNECTION *psConnection,
												PVRSRV_MISC_INFO *psMiscInfo)
{
	if(psMiscInfo->ui32StatePresent & PVRSRV_MISC_INFO_GLOBALEVENTOBJECT_PRESENT)
	{
		OSEventObjectClose(psConnection,
#if defined (SUPPORT_SID_INTERFACE)
							IMG_NULL,
#else
							&psMiscInfo->sGlobalEventObject,
#endif
							psMiscInfo->hOSGlobalEvent);
	}

	if( ((psMiscInfo->ui32StatePresent & PVRSRV_MISC_INFO_TIMER_PRESENT) != 0) &&
		((psMiscInfo->pvSOCTimerRegisterUM) != 0)
	  )
	{
		/* FIXME: Make this an OS function */
/* PRQA S 3332 1 */ /* ignore warning about UNDER_CE */
#if (UNDER_CE >= 600)
		PVRSRV_BRIDGE_IN_RELEASE_MISC_INFO      sIn;
		PVRSRV_BRIDGE_OUT_RELEASE_MISC_INFO     sOut;

		sIn.sMiscInfo = *psMiscInfo;

		if(PVRSRVBridgeCall(psConnection->hServices,
							PVRSRV_BRIDGE_RELEASE_MISC_INFO,
							&sIn,
							sizeof(PVRSRV_BRIDGE_IN_RELEASE_MISC_INFO),
							&sOut,
							sizeof(PVRSRV_BRIDGE_OUT_RELEASE_MISC_INFO)))
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVReleaseMiscInfo: BridgeCall failed"));
			return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
		}

		if(sOut.eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVReleaseMiscInfo: Error %d returned", sOut.eError));
			return sOut.eError;
		}
#else
		PVRUnMapKMem(psConnection->hServices, psMiscInfo->hSOCTimerRegisterMappingInfo, psMiscInfo->hSOCTimerRegisterOSMemHandle);
#endif
	}

	return PVRSRV_OK;
}

#endif


#if !defined(SRVINIT_MODULE)

IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVEventObjectWait(const PVRSRV_CONNECTION *psConnection,
#if defined (SUPPORT_SID_INTERFACE)
												IMG_EVENTSID hOSEvent)
#else
												IMG_HANDLE hOSEvent)
#endif
{
	return OSEventObjectWait(psConnection, hOSEvent);
}

#endif /* !defined(SRVINIT_MODULE) */


#if !defined(OS_PVRSRV_MAP_PHYS_TO_USERSPACE_UM) && !defined(SRVINIT_MODULE)
/*!
 ******************************************************************************

 @Function	PVRSRVMapPhysToUserSpace

 @Description

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVMapPhysToUserSpace(const PVRSRV_DEV_DATA *psDevData,
												   IMG_SYS_PHYADDR sSysPhysAddr,
												   IMG_UINT32 uiSizeInBytes,
												   IMG_PVOID *ppvUserAddr,
												   IMG_UINT32 *puiActualSize,
												   IMG_PVOID *ppvProcess)
{
	PVRSRV_BRIDGE_IN_MAPPHYSTOUSERSPACE		sIn;
	PVRSRV_BRIDGE_OUT_MAPPHYSTOUSERSPACE	sOut;

	sIn.hDevCookie = psDevData->hDevCookie;
	sIn.sSysPhysAddr = sSysPhysAddr;
	sIn.uiSizeInBytes  = uiSizeInBytes;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_MAPPHYSTOUSERSPACE,
						&sIn,
						sizeof(sIn),
						&sOut,
						sizeof(sOut)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVMapPhysToUserSpace: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	*ppvProcess = sOut.pvProcess;
	*puiActualSize = sOut.uiActualSize;
	*ppvUserAddr = sOut.pvUserAddr;

	return PVRSRV_OK;
}
#endif /* OS_PVRSRV_MAP_PHYS_TO_USERSPACE_UM */


#if !defined(OS_PVRSRV_UNMAP_PHYS_TO_USERSPACE_UM) && !defined(SRVINIT_MODULE)
/*!
 ******************************************************************************

 @Function	PVRSRVUnmapPhysToUserSpace

 @Description	Unmaps Sys Physical address from User Space

 @Input		psDevData - DevData ptr
 @Input		pvUserAddr - user space lin address
 @Input		pvProcess - process to which mapped

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVUnmapPhysToUserSpace(const PVRSRV_DEV_DATA *psDevData,
													 IMG_PVOID pvUserAddr,
													 IMG_PVOID pvProcess)
{
	PVRSRV_BRIDGE_IN_UNMAPPHYSTOUSERSPACE	sIn;

	sIn.hDevCookie = psDevData->hDevCookie;
	sIn.pvUserAddr = pvUserAddr;
	sIn.pvProcess  = pvProcess;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_UNMAPPHYSTOUSERSPACE,
						&sIn,
						sizeof(sIn),
						IMG_NULL,
						0))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVUnmapPhysToUserSpace: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return PVRSRV_OK;
}
#endif /* OS_PVRSRV_UNMAP_PHYS_TO_USERSPACE_UM */


#if !defined(OS_PVRSRV_INIT_SRV_CONNECT_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVInitSrvConnect

 @Description	Initialisation server connect

 @Input		psConnection

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVInitSrvConnect(PVRSRV_CONNECTION **ppsConnection)
{
	PVRSRV_BRIDGE_RETURN sRet;
	PVRSRV_ERROR eError=PVRSRV_OK ;

	PVRSRVMemSet(&sRet,0x00,sizeof(sRet));

#if defined(SUPPORT_PDUMP_MULTI_PROCESS)
	/* mark the initialisation process for pdumping */
	eError = PVRSRVConnectServices(ppsConnection, SRV_FLAGS_PDUMP_ACTIVE);
#else
	eError = PVRSRVConnectServices(ppsConnection, 0);
#endif
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVInitSrvConnect: PVRSRVConnect failed"));
		return eError;
	}

	if(PVRSRVBridgeCall((*ppsConnection)->hServices,
				PVRSRV_BRIDGE_INITSRV_CONNECT,
				IMG_NULL,
				0,
				(IMG_VOID *)&sRet,
				sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVInitSrvConnect: BridgeCall failed"));
		(IMG_VOID) PVRSRVDisconnect(*ppsConnection);
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sRet.eError;
}
#endif /* OS_PVRSRV_INIT_SRV_CONNECT_UM */


#if !defined(OS_PVRSRV_INIT_SRV_DISCONNECT_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVInitSrvDisconnect

 @Description	Initialisation server disconnect

 @Input		psConnection

 @Input		bInitSuccesful

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVInitSrvDisconnect(PVRSRV_CONNECTION *psConnection,
												  IMG_BOOL bInitSuccesful)
{
	PVRSRV_BRIDGE_IN_INITSRV_DISCONNECT sIn;
	PVRSRV_BRIDGE_RETURN sRet;
	PVRSRV_ERROR eError=PVRSRV_OK;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sRet,0x00,sizeof(sRet));

	if(!psConnection)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVInitSrvDisconnect: Null connection"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sIn.bInitSuccesful = bInitSuccesful;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_INITSRV_DISCONNECT,
						&sIn,
						sizeof (PVRSRV_BRIDGE_IN_INITSRV_DISCONNECT),
						(IMG_VOID *)&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVInitSrvDisconnect: BridgeCall failed"));
		eError = PVRSRV_ERROR_BRIDGE_CALL_FAILED;
		goto ErrorExit;
	}

	if (sRet.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVInitSrvDisconnect: KM returned %d",sRet.eError));
		eError = sRet.eError;
		goto ErrorExit;
	}

	return PVRSRVDisconnect(psConnection);

ErrorExit:
	PVRSRVDisconnect(psConnection);
	return eError;
}
#endif /* OS_PVRSRV_INIT_SRV_DISCONNECT_UM */


#if !defined(OS_PVRSRV_UNREF_SHARED_SYS_MEM_UM) && !defined(SRVINIT_MODULE)
IMG_INTERNAL
PVRSRV_ERROR PVRSRVUnrefSharedSysMem(const PVRSRV_CONNECTION *psConnection,
									 PVRSRV_CLIENT_MEM_INFO *psClientMemInfo)
{
	if(!PVRUnMapKMem(psConnection->hServices, psClientMemInfo->hMappingInfo, psClientMemInfo->hKernelMemInfo))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVUnrefSharedMem: Failed to unmap allocation from userspace"));
		return PVRSRV_ERROR_BAD_MAPPING;
	}

	PVRSRVFreeUserModeMem(psClientMemInfo);

	return PVRSRV_OK;
}
#endif /* OS_PVRSRV_UNREF_SHARED_SYS_MEM_UM */


#if !defined(OS_PVRSRV_ALLOC_SHARED_SYS_MEM_UM) && !defined(SRVINIT_MODULE)
IMG_INTERNAL
PVRSRV_ERROR IMG_CALLCONV PVRSRVAllocSharedSysMem(const PVRSRV_CONNECTION *psConnection,
												  IMG_UINT32 ui32Flags,
												  IMG_SIZE_T uSize,
												  PVRSRV_CLIENT_MEM_INFO **ppsClientMemInfo)
{
	PVRSRV_BRIDGE_IN_ALLOC_SHARED_SYS_MEM sIn;
	PVRSRV_BRIDGE_OUT_ALLOC_SHARED_SYS_MEM sOut;
	PVRSRV_BRIDGE_IN_FREE_SHARED_SYS_MEM sFreeIn;
	PVRSRV_BRIDGE_OUT_FREE_SHARED_SYS_MEM sFreeOut;
	PVRSRV_CLIENT_MEM_INFO *psClientMemInfo;
	PVRSRV_ERROR eError=PVRSRV_OK;


	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));

	PVRSRVMemSet(&sFreeIn,0x00,sizeof(sFreeIn));
	PVRSRVMemSet(&sFreeOut,0x00,sizeof(sFreeOut));
	

	if((ui32Flags & PVRSRV_HAP_CACHETYPE_MASK) == 0)
	{
		ui32Flags |= PVRSRV_HAP_CACHED;
	}

	sIn.ui32Flags = ui32Flags | PVRSRV_HAP_MULTI_PROCESS | PVRSRV_MEM_NO_SYNCOBJ;
	sIn.ui32Size = uSize;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_ALLOC_SHARED_SYS_MEM,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_ALLOC_SHARED_SYS_MEM),
						&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_ALLOC_SHARED_SYS_MEM)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocSharedSysMem: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocSharedSysMem: Error %d returned", sOut.eError));
		return sOut.eError;
	}

	/* TODO - use a smaller typedef. */
	psClientMemInfo = (PVRSRV_CLIENT_MEM_INFO *)
		PVRSRVAllocUserModeMem(sizeof(PVRSRV_CLIENT_MEM_INFO));
	if(!psClientMemInfo)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto failed_to_alloc_meminfo;
	}
	*psClientMemInfo = sOut.sClientMemInfo;

	eError =
		PVRPMapKMem(psConnection->hServices,
						    (IMG_VOID**)&psClientMemInfo->pvLinAddr,
							psClientMemInfo->pvLinAddrKM,
							&psClientMemInfo->hMappingInfo,
							psClientMemInfo->hKernelMemInfo);
	if(!psClientMemInfo->pvLinAddr || (eError != PVRSRV_OK))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocSharedSysMem: PVRPMapKMem failed for buffer "));
		eError = PVRSRV_ERROR_BAD_MAPPING;
		goto failed_to_map_kernel_ptr;
	}

	*ppsClientMemInfo = psClientMemInfo;

	return eError;

failed_to_map_kernel_ptr:
	PVRSRVFreeUserModeMem(psClientMemInfo);

failed_to_alloc_meminfo:
#if defined (SUPPORT_SID_INTERFACE)
	sFreeIn.hKernelMemInfo = sOut.sClientMemInfo.hKernelMemInfo;
#else
	sFreeIn.psKernelMemInfo =
		(PVRSRV_KERNEL_MEM_INFO *)(sOut.sClientMemInfo.hKernelMemInfo);
#endif

	PVRSRVBridgeCall(psConnection->hServices,
					 PVRSRV_BRIDGE_FREE_SHARED_SYS_MEM,
					 &sFreeIn,
					 sizeof(PVRSRV_BRIDGE_IN_FREE_SHARED_SYS_MEM),
					 &sFreeOut,
					 sizeof(PVRSRV_BRIDGE_OUT_FREE_SHARED_SYS_MEM));

	return eError;
}
#endif /* OS_PVRSRV_ALLOC_SHARED_SYS_MEM_UM */


#if !defined(OS_PVRSRV_FREE_SHARED_SYS_MEM_UM) && !defined(SRVINIT_MODULE)
IMG_INTERNAL
PVRSRV_ERROR IMG_CALLCONV PVRSRVFreeSharedSysMem(const PVRSRV_CONNECTION *psConnection,
					 							 PVRSRV_CLIENT_MEM_INFO *psClientMemInfo)
{
	PVRSRV_BRIDGE_IN_FREE_SHARED_SYS_MEM sIn;
	PVRSRV_BRIDGE_OUT_FREE_SHARED_SYS_MEM sOut;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));

	if(!psConnection || !psClientMemInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVFreeSharedSysMem: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	PVRUnMapKMem(psConnection->hServices, psClientMemInfo->hMappingInfo, psClientMemInfo->hKernelMemInfo);

#if defined (SUPPORT_SID_INTERFACE)
	sIn.hKernelMemInfo = psClientMemInfo->hKernelMemInfo;
	sIn.hMappingInfo   = psClientMemInfo->hMappingInfo;
#else
	sIn.psKernelMemInfo = psClientMemInfo->hKernelMemInfo;
#endif

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_FREE_SHARED_SYS_MEM,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_FREE_SHARED_SYS_MEM),
						&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_FREE_SHARED_SYS_MEM)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVFreeSharedSysMem: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	PVRSRVFreeUserModeMem(psClientMemInfo);

	return sOut.eError;

}
#endif /* OS_PVRSRV_FREE_SHARED_SYS_MEM_UM */


#if !defined(OS_PVRSRV_MAP_MEMINFO_MEM_UM) && !defined(SRVINIT_MODULE)
IMG_INTERNAL
PVRSRV_ERROR IMG_CALLCONV PVRSRVMapMemInfoMem(const PVRSRV_CONNECTION *psConnection,
#if defined (SUPPORT_SID_INTERFACE)
											  IMG_SID    hKernelMemInfo,
#else
											  IMG_HANDLE hKernelMemInfo,
#endif
											  PVRSRV_CLIENT_MEM_INFO **ppsClientMemInfo)
{
	PVRSRV_BRIDGE_IN_MAP_MEMINFO_MEM sIn;
	PVRSRV_BRIDGE_OUT_MAP_MEMINFO_MEM sOut;
	PVRSRV_CLIENT_MEM_INFO *psClientMemInfo;
	PVRSRV_CLIENT_SYNC_INFO *psSyncInfo;
	PVRSRV_ERROR eError=PVRSRV_OK;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));
	
	sIn.hKernelMemInfo = hKernelMemInfo;

	/* FIXME - should this bridge call have a more generic
	 * name like "KERNMEMINFO_TO_CLIENTMEMINFO */
	if (PVRSRVBridgeCall(psConnection->hServices,
							  PVRSRV_BRIDGE_MAP_MEMINFO_MEM,
							  &sIn,
							  sizeof(PVRSRV_BRIDGE_IN_MAP_MEMINFO_MEM),
							  &sOut,
							  sizeof(PVRSRV_BRIDGE_OUT_MAP_MEMINFO_MEM)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVMapMemInfoMem: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVMapMemInfoMem: Error %d returned", sOut.eError));
		return sOut.eError;
	}

	psClientMemInfo = (PVRSRV_CLIENT_MEM_INFO *)
		PVRSRVAllocUserModeMem(sizeof(PVRSRV_CLIENT_MEM_INFO));
	if(!psClientMemInfo)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto failed_to_alloc_meminfo;
	}
	*psClientMemInfo = sOut.sClientMemInfo;

	eError = PVRPMapKMem(psConnection->hServices,
								 (IMG_VOID**)&psClientMemInfo->pvLinAddr,
								 psClientMemInfo->pvLinAddrKM,
								 &psClientMemInfo->hMappingInfo,
								 psClientMemInfo->hKernelMemInfo);
	if(!psClientMemInfo->pvLinAddr || (eError != PVRSRV_OK))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVMapMemInfoMem: "
				 "Failed to map psClientMemInfo->pvLinAddrKM\n"));
		eError = PVRSRV_ERROR_BAD_MAPPING;
		goto failed_to_map_kernel_ptr;
	}

	if(psClientMemInfo->ui32Flags & PVRSRV_MEM_NO_SYNCOBJ)
	{
		psClientMemInfo->psClientSyncInfo = IMG_NULL;
	}
	else
	{
		psSyncInfo = (PVRSRV_CLIENT_SYNC_INFO *)PVRSRVAllocUserModeMem(sizeof(PVRSRV_CLIENT_SYNC_INFO));
		if(!psSyncInfo)
		{
			eError = PVRSRV_ERROR_OUT_OF_MEMORY;
			goto failed_to_alloc_syncinfo;
		}
		*psSyncInfo = sOut.sClientSyncInfo;

#if !defined(PVRSRV_DISABLE_UM_SYNCOBJ_MAPPINGS)
		eError = PVRPMapKMem(psConnection->hServices,
									 (IMG_VOID**)&psSyncInfo->psSyncData,
									 psSyncInfo->psSyncData,
									 &psSyncInfo->hMappingInfo,
									 psSyncInfo->hKernelSyncInfo);
		if(!psSyncInfo->psSyncData || (eError != PVRSRV_OK))
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVMapMemInfoMem : PVRPMapKMem failed for syncdata"));

			eError = PVRSRV_ERROR_BAD_MAPPING;

			goto failed_to_map_syncinfo;
		}
#endif
		psClientMemInfo->psClientSyncInfo = psSyncInfo;
	}

	*ppsClientMemInfo = psClientMemInfo;

	return PVRSRV_OK;

#if !defined(PVRSRV_DISABLE_UM_SYNCOBJ_MAPPINGS)
	/* exit path becomes redundant */
failed_to_map_syncinfo:
	PVRSRVFreeUserModeMem(psClientMemInfo->psClientSyncInfo);
#endif
failed_to_alloc_syncinfo:
	PVRUnMapKMem(psConnection->hServices, psClientMemInfo->hMappingInfo, psClientMemInfo->hKernelMemInfo);
failed_to_map_kernel_ptr:
	PVRSRVFreeUserModeMem(psClientMemInfo);
failed_to_alloc_meminfo:

	return eError;
}
#endif /* OS_PVRSRV_MAP_MEMINFO_MEM_UM */

#if !defined(OS_PVRSRV_MAP_DEV_MEMORY_UM) && !defined(SRVINIT_MODULE)
/*!
 ******************************************************************************

 @Function	PVRSRVMapDeviceMemory

 @Description

 @Input		psDevData
 @Input		hKernelMemInfo
 @Input		hDstDevMemHeap
 @Output	ppsDstMemInfo

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVMapDeviceMemory(IMG_CONST PVRSRV_DEV_DATA *psDevData,
#if defined (SUPPORT_SID_INTERFACE)
												IMG_SID    hKernelMemInfo,
												IMG_SID    hDstDevMemHeap,
#else
												IMG_HANDLE hKernelMemInfo,
												IMG_HANDLE hDstDevMemHeap,
#endif
												PVRSRV_CLIENT_MEM_INFO **ppsDstMemInfo)
{
	PVRSRV_BRIDGE_IN_MAP_DEV_MEMORY sBridgeIn;
	PVRSRV_BRIDGE_OUT_MAP_DEV_MEMORY sBridgeOut;
	PVRSRV_CLIENT_MEM_INFO *psMemInfo;
	PVRSRV_ERROR eError=PVRSRV_OK;
	PVRSRV_BRIDGE_IN_UNMAP_DEV_MEMORY sInFailure;
	PVRSRV_BRIDGE_RETURN sRet;
	PVRSRV_CLIENT_SYNC_INFO *psSyncInfo = IMG_NULL;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));
	PVRSRVMemSet(&sInFailure,0x00,sizeof(sInFailure));
	PVRSRVMemSet(&sRet,0x00,sizeof(sRet));	
	
	if(!psDevData || !hKernelMemInfo || !hDstDevMemHeap || !ppsDstMemInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVMapDeviceMemory: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hKernelMemInfo = hKernelMemInfo;
	sBridgeIn.hDstDevMemHeap = hDstDevMemHeap;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_MAP_DEV_MEMORY,
						&sBridgeIn,
						sizeof(PVRSRV_BRIDGE_IN_MAP_DEV_MEMORY),
						&sBridgeOut,
						sizeof(PVRSRV_BRIDGE_OUT_MAP_DEV_MEMORY)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVMapDeviceMemory: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sBridgeOut.eError != PVRSRV_OK)
	{
		return sBridgeOut.eError;
	}

	/* If eviction is supported, allocate an Resource Object instead */
	if(sBridgeOut.sDstClientMemInfo.ui32Flags & PVRSRV_HAP_GPU_PAGEABLE)
		psMemInfo = (PVRSRV_CLIENT_MEM_INFO *)PVRSRVCallocUserModeMem(sizeof(PVRSRV_GPU_MEMORY_RESOURCE_OBJECT));
	else
		psMemInfo = (PVRSRV_CLIENT_MEM_INFO *)PVRSRVAllocUserModeMem(sizeof(PVRSRV_CLIENT_MEM_INFO));
	if(psMemInfo == IMG_NULL)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;

		goto MAP_DEV_MEMORY_CLEANUP_MAP;
	}

	if(sBridgeOut.sDstClientSyncInfo.hKernelSyncInfo)
	{
		psSyncInfo = (PVRSRV_CLIENT_SYNC_INFO *)PVRSRVAllocUserModeMem(sizeof(PVRSRV_CLIENT_SYNC_INFO));

		if (!psSyncInfo)
		{
			eError = PVRSRV_ERROR_OUT_OF_MEMORY;

			goto MAP_DEV_MEMORY_CLEANUP_MEMINFO_ALLOC;
		}
	}

	*psMemInfo  = sBridgeOut.sDstClientMemInfo;

	eError = PVRPMapKMem(psDevData->psConnection->hServices,
								 (IMG_VOID**)&psMemInfo->pvLinAddr,
								 psMemInfo->pvLinAddrKM,
								 &psMemInfo->hMappingInfo,
								 psMemInfo->hKernelMemInfo);

	if (!psMemInfo->pvLinAddr || (eError != PVRSRV_OK))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVMapDeviceMemory : PVRPMapKMem failed for buffer "));

		eError = PVRSRV_ERROR_BAD_MAPPING;

		goto MAP_DEV_MEMORY_CLEANUP_SYNCINFO_ALLOC;
	}

	if(psSyncInfo)
	{
		*psSyncInfo = sBridgeOut.sDstClientSyncInfo;

#if !defined(PVRSRV_DISABLE_UM_SYNCOBJ_MAPPINGS)
		eError = PVRPMapKMem(psDevData->psConnection->hServices,
									 (IMG_VOID**)&psSyncInfo->psSyncData,
									 psSyncInfo->psSyncData,
									 &psSyncInfo->hMappingInfo,
									 psSyncInfo->hKernelSyncInfo);

		if (!psSyncInfo->psSyncData || (eError != PVRSRV_OK))
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVMapDeviceMemory : PVRPMapKMem failed for syncdata "));

			eError = PVRSRV_ERROR_BAD_MAPPING;

			goto MAP_DEV_MEMORY_CLEANUP_LINADDR;
		}
#endif
		psMemInfo->psClientSyncInfo = psSyncInfo;
	}

	*ppsDstMemInfo = psMemInfo;

	goto MAP_DEV_MEMORY_OK;

#if !defined(PVRSRV_DISABLE_UM_SYNCOBJ_MAPPINGS)
MAP_DEV_MEMORY_CLEANUP_LINADDR:
	PVRUnMapKMem(psDevData->psConnection->hServices, psMemInfo->hMappingInfo, psMemInfo->hKernelMemInfo);
#endif

MAP_DEV_MEMORY_CLEANUP_SYNCINFO_ALLOC:
	if(psSyncInfo)
	{
		PVRSRVFreeUserModeMem(psSyncInfo);
	}

MAP_DEV_MEMORY_CLEANUP_MEMINFO_ALLOC:
	PVRSRVFreeUserModeMem(psMemInfo);

MAP_DEV_MEMORY_CLEANUP_MAP:
#if defined (SUPPORT_SID_INTERFACE)
	sInFailure.hKernelMemInfo = sBridgeOut.sDstClientMemInfo.hKernelMemInfo;
#else
	sInFailure.psKernelMemInfo = sBridgeOut.sDstClientMemInfo.hKernelMemInfo;
#endif
	PVRSRVBridgeCall(psDevData->psConnection->hServices,
					 PVRSRV_BRIDGE_UNMAP_DEV_MEMORY,
					 &sInFailure,
					 sizeof(PVRSRV_BRIDGE_IN_UNMAP_DEV_MEMORY),
					 (IMG_VOID *)&sRet,
					 sizeof(PVRSRV_BRIDGE_RETURN));

	*ppsDstMemInfo = IMG_NULL;

MAP_DEV_MEMORY_OK:
	return eError;
}
#endif /* OS_PVRSRV_MAP_DEV_MEMORY_UM */


#if !defined(OS_PVRSRV_UNMAP_DEV_MEMORY_UM) && !defined(SRVINIT_MODULE)
/*!
 ******************************************************************************

 @Function	PVRSRVUnmapDeviceMemory

 @Description

 @Input		psDevData

 @Input		psMemInfo

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR PVRSRVUnmapDeviceMemory(const PVRSRV_DEV_DATA *psDevData,
										  PVRSRV_CLIENT_MEM_INFO *psMemInfo)
{
	PVRSRV_BRIDGE_IN_UNMAP_DEV_MEMORY sBridgeIn;
	PVRSRV_BRIDGE_RETURN sBridgeOut;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

	if(!psDevData || !psMemInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVUnmapDeviceMemory: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(psMemInfo->psClientSyncInfo)
	{
		PVRSRV_ERROR eError;
		eError = FlushClientOps(psDevData->psConnection,
#if defined (SUPPORT_SID_INTERFACE)
								psMemInfo->psClientSyncInfo->hKernelSyncInfo);
#else
								psMemInfo->psClientSyncInfo);
#endif
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVUnmapDeviceMemory: FlushClientOps failed"));
			return eError;
		}

#if !defined(PVRSRV_DISABLE_UM_SYNCOBJ_MAPPINGS)
		PVRUnMapKMem(psDevData->psConnection->hServices,
						psMemInfo->psClientSyncInfo->hMappingInfo,
						psMemInfo->psClientSyncInfo->hKernelSyncInfo);
#endif
		PVRSRVFreeUserModeMem(psMemInfo->psClientSyncInfo);
	}

	PVRUnMapKMem(psDevData->psConnection->hServices, psMemInfo->hMappingInfo, psMemInfo->hKernelMemInfo);

#if defined (SUPPORT_SID_INTERFACE)
	sBridgeIn.hKernelMemInfo = psMemInfo->hKernelMemInfo;
#else
	sBridgeIn.psKernelMemInfo = psMemInfo->hKernelMemInfo;
#endif
	sBridgeIn.sClientMemInfo = *psMemInfo;

	PVRSRVFreeUserModeMem(psMemInfo);

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_UNMAP_DEV_MEMORY,
						&sBridgeIn,
						sizeof(PVRSRV_BRIDGE_IN_UNMAP_DEV_MEMORY),
						&sBridgeOut,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVUnmapDeviceMemory: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sBridgeOut.eError != PVRSRV_OK)
	{
		return sBridgeOut.eError;
	}

	return PVRSRV_OK;
}
#endif /* OS_PVRSRV_UNMAP_DEV_MEMORY_UM */


#if !defined(OS_PVRSRV_CREATE_SYNC_INFO_MOD_OBJ_UM) && !defined(SRVINIT_MODULE)
/*!
 ******************************************************************************

 @Function	PVRSRVCreateSyncInfoModObj

 @Description Creates an empty SyncInfo Modification Object, kernelside,
              and returns a handle to it.  The Modification Object is to be
			  used in a subsequent call to PVRSRVModifyPendingSyncOps().

 @Input		psConnection

 @Output	phKernelSyncInfoModObj

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVCreateSyncInfoModObj(const PVRSRV_CONNECTION *psConnection,
#if defined (SUPPORT_SID_INTERFACE)
													 IMG_SID *phKernelSyncInfoModObj)
#else
													 IMG_HANDLE *phKernelSyncInfoModObj)
#endif
{
	PVRSRV_BRIDGE_OUT_CREATE_SYNC_INFO_MOD_OBJ	sBridgeOut;

	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));
	
	if (!psConnection || (!psConnection->hServices))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVCreateSyncInfoModObj: Invalid connection"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(!phKernelSyncInfoModObj)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVCreateSyncInfoModObj: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_CREATE_SYNC_INFO_MOD_OBJ,
						IMG_NULL,
						0,
						&sBridgeOut,
						sizeof(PVRSRV_BRIDGE_OUT_CREATE_SYNC_INFO_MOD_OBJ)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVCreateSyncInfoModObj: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sBridgeOut.eError == PVRSRV_ERROR_RETRY)
	{
		return sBridgeOut.eError;
	}

	if(sBridgeOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVCreateSyncInfoModObj: Error %d returned", sBridgeOut.eError));
		return sBridgeOut.eError;
	}

	*phKernelSyncInfoModObj	= sBridgeOut.hKernelSyncInfoModObj;

	return PVRSRV_OK;
}
#endif /* OS_PVRSRV_CREATE_SYNC_INFO_MOD_OBJ_UM */


#if !defined(OS_PVRSRV_DESTROY_SYNC_INFO_MOD_OBJ_UM) && !defined(SRVINIT_MODULE)
/*!
 ******************************************************************************

 @Function	PVRSRVDestroySyncInfoModObj

 @Description Destroy an empty SyncInfo Modification Object.

 @Input		psConnection

 @Input     hKernelSyncInfoModObj

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVDestroySyncInfoModObj(const PVRSRV_CONNECTION *psConnection,
#if defined (SUPPORT_SID_INTERFACE)
													  IMG_SID hKernelSyncInfoModObj)
#else
													  IMG_HANDLE hKernelSyncInfoModObj)
#endif
{
	PVRSRV_BRIDGE_IN_DESTROY_SYNC_INFO_MOD_OBJ	sBridgeIn;
	PVRSRV_BRIDGE_RETURN						        sBridgeOut;


	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

	if (!psConnection || (!psConnection->hServices))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVDestroySyncInfoModObj: Invalid connection"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hKernelSyncInfoModObj = hKernelSyncInfoModObj;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_DESTROY_SYNC_INFO_MOD_OBJ,
						&sBridgeIn,
						sizeof(PVRSRV_BRIDGE_IN_DESTROY_SYNC_INFO_MOD_OBJ),
						&sBridgeOut,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVDestroySyncInfoModObj: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sBridgeOut.eError == PVRSRV_ERROR_RETRY)
	{
		return sBridgeOut.eError;
	}

	if(sBridgeOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVDestroySyncInfoModObj: Error %d returned", sBridgeOut.eError));
		return sBridgeOut.eError;
	}

	return PVRSRV_OK;
}
#endif /* OS_PVRSRV_DESTROY_SYNC_INFO_MOD_OBJ_UM */


#if !defined(OS_PVRSRV_MODIFY_PENDING_SYNC_OPS_UM) && !defined(SRVINIT_MODULE)
/*!
 ******************************************************************************

 @Function	PVRSRVModifyPendingSyncOps

 @Description Returns PRE-INCREMENTED sync op values. Performs thread safe increment
			  of sync ops values as specified by ui32ModifyFlags.

 @Input		psConnection

 @Input		hKernelSyncInfo

 @Input		ui32ModifyFlags

 @Output	pui32ReadOpsPending

 @Output	pui32WriteOpsPending

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVModifyPendingSyncOps(const PVRSRV_CONNECTION *psConnection,
#if defined (SUPPORT_SID_INTERFACE)
													  IMG_SID    hKernelSyncInfoModObj,
#else
													  IMG_HANDLE hKernelSyncInfoModObj,
#endif
													  PVRSRV_CLIENT_SYNC_INFO *psSyncInfo,
													  IMG_UINT32 ui32ModifyFlags,
													  IMG_UINT32 *pui32ReadOpsPending,
													  IMG_UINT32 *pui32WriteOpsPending)
{
	PVRSRV_BRIDGE_OUT_MODIFY_PENDING_SYNC_OPS	sBridgeOut;
	PVRSRV_BRIDGE_IN_MODIFY_PENDING_SYNC_OPS	sBridgeIn;


	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));
	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));


	if (!psConnection || (!psConnection->hServices) || (!psSyncInfo))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVModifyPendingSyncOps: Invalid connection"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hKernelSyncInfoModObj = hKernelSyncInfoModObj;
	sBridgeIn.hKernelSyncInfo = psSyncInfo->hKernelSyncInfo;
	sBridgeIn.ui32ModifyFlags = ui32ModifyFlags;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_MODIFY_PENDING_SYNC_OPS,
						&sBridgeIn,
						sizeof(PVRSRV_BRIDGE_IN_MODIFY_PENDING_SYNC_OPS),
						&sBridgeOut,
						sizeof(PVRSRV_BRIDGE_OUT_MODIFY_PENDING_SYNC_OPS)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVModifyPendingSyncOps: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sBridgeOut.eError == PVRSRV_ERROR_RETRY)
	{
		return sBridgeOut.eError;
	}

	if(sBridgeOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVModifyPendingSyncOps: Error %d returned", sBridgeOut.eError));
		return sBridgeOut.eError;
	}

	if(pui32ReadOpsPending != IMG_NULL)
	{
		*pui32ReadOpsPending	= sBridgeOut.ui32ReadOpsPending;
	}

	if(pui32WriteOpsPending != IMG_NULL)
	{
		*pui32WriteOpsPending	= sBridgeOut.ui32WriteOpsPending;
	}

	return PVRSRV_OK;
}
#endif /* OS_PVRSRV_MODIFY_PENDING_SYNC_OPS_UM */


#if !defined(OS_PVRSRV_MODIFY_COMPLETE_SYNC_OPS_UM) && !defined(SRVINIT_MODULE)
/*!
 ******************************************************************************

 @Function	PVRSRVModifyCompleteSyncOps

 @Description Returns PRE-INCREMENTED sync op values. Performs thread safe increment
			  of sync ops values as specified by ui32ModifyFlags.

 @Input		psConnection

 @Input		hKernelSyncInfo

 @Input		ui32ModifyFlags

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVModifyCompleteSyncOps(const PVRSRV_CONNECTION *psConnection,
#if defined (SUPPORT_SID_INTERFACE)
													  IMG_SID hKernelSyncInfoModObj)
#else
													  IMG_HANDLE hKernelSyncInfoModObj)
#endif
{
	PVRSRV_BRIDGE_IN_MODIFY_COMPLETE_SYNC_OPS	sBridgeIn;
	PVRSRV_BRIDGE_RETURN						sBridgeOut;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

	if (!psConnection || (!psConnection->hServices))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVModifyCompleteSyncOps: Invalid connection"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hKernelSyncInfoModObj = hKernelSyncInfoModObj;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_MODIFY_COMPLETE_SYNC_OPS,
						&sBridgeIn,
						sizeof(PVRSRV_BRIDGE_IN_MODIFY_COMPLETE_SYNC_OPS),
						&sBridgeOut,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVModifyCompleteSyncOps: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if (sBridgeOut.eError == PVRSRV_ERROR_BAD_SYNC_STATE)
	{
		/* This means that the caller has called
		   PVRSRVModifyCompleteSyncOps() without first waiting for the
		   Complete Ops to catch up with the Pending Snapshot.  The
		   caller must call PVRSRVSyncOpsFlushToModObj() before
		   assuming it has exclusive access to the buffer.  Please
		   excuse the use of PVR_ASSERT for catching errors in the
		   caller's code, but we crash and burn here quite
		   deliberately since it is likely that a software bug has
		   caused dangerous concurrent access to buffer data.  The
		   caller ought to assert this, so we're just doing it one
		   step sooner. */
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVModifyCompleteSyncOps: Bad Synchronisation State.  This means a software bug in the caller stack has potentially granted concurrent access to a buffer.  This is dangerous."));
		PVR_ASSERT(sBridgeOut.eError != PVRSRV_ERROR_BAD_SYNC_STATE);
		/* NB: I don't really "assert" this... please refer to comment above */
	}

	if(sBridgeOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVModifyCompleteSyncOps: Error %d returned", sBridgeOut.eError));
		return sBridgeOut.eError;
	}

	return PVRSRV_OK;
}
#endif /* OS_PVRSRV_MODIFY_COMPLETE_SYNC_OPS_UM */


#if !defined(OS_PVRSRV_SYNC_OPS_TAKE_TOKEN_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVSyncOpsFlushToToken

 @Description Captures a snapshot of the current "pending" values of
              the counters in the sync object.

 @Input		psConnection

 @Input		hKernelSyncInfo

 @Output	psSyncToken

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVSyncOpsTakeToken(const PVRSRV_CONNECTION *psConnection,
#if defined (SUPPORT_SID_INTERFACE)
												 const IMG_SID hKernelSyncInfo,
#else
												 const PVRSRV_CLIENT_SYNC_INFO *psSyncInfo,
#endif
												 PVRSRV_SYNC_TOKEN *psSyncToken)
{
	PVRSRV_BRIDGE_IN_SYNC_OPS_TAKE_TOKEN        sBridgeIn;
	PVRSRV_BRIDGE_OUT_SYNC_OPS_TAKE_TOKEN       sBridgeOut;


	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

	if (!psConnection || (!psConnection->hServices))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsTakeToken: Invalid connection"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

#if defined (SUPPORT_SID_INTERFACE)
	if (!psSyncToken || !hKernelSyncInfo)
#else
	if (!psSyncToken || !psSyncInfo)
#endif
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsTakeToken: Invalid arguments"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

#if defined (SUPPORT_SID_INTERFACE)
	sBridgeIn.hKernelSyncInfo = hKernelSyncInfo;
#else
	sBridgeIn.hKernelSyncInfo = psSyncInfo->hKernelSyncInfo;
#endif

	if (PVRSRVBridgeCall(psConnection->hServices,
						 PVRSRV_BRIDGE_SYNC_OPS_TAKE_TOKEN,
						 &sBridgeIn,
						 sizeof(PVRSRV_BRIDGE_IN_SYNC_OPS_TAKE_TOKEN),
						 &sBridgeOut,
						 sizeof(PVRSRV_BRIDGE_OUT_SYNC_OPS_TAKE_TOKEN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsTakeToken: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if (sBridgeOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsTakeToken: Error %d returned",
				 sBridgeOut.eError));
		return sBridgeOut.eError;
	}

	psSyncToken->sPrivate.ui32ReadOpsPendingSnapshot = sBridgeOut.ui32ReadOpsPending;
	psSyncToken->sPrivate.ui32WriteOpsPendingSnapshot = sBridgeOut.ui32WriteOpsPending;
	psSyncToken->sPrivate.ui32ReadOps2PendingSnapshot = sBridgeOut.ui32ReadOps2Pending;
#if defined (SUPPORT_SID_INTERFACE)
	psSyncToken->sPrivate.hKernelSyncInfo = hKernelSyncInfo;
#else
	psSyncToken->sPrivate.hKernelSyncInfo = psSyncInfo->hKernelSyncInfo;
#endif

	return PVRSRV_OK;
}
#endif /* OS_PVRSRV_SYNC_OPS_TAKE_TOKEN_UM */


#if !defined(OS_PVRSRV_SYNC_OPS_FLUSH_TO_TOKEN_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVSyncOpsFlushToToken

 @Description Tests whether the dependencies for a pending sync op modification
              have been satisfied.  If this function returns PVRSRV_OK, then the
              "complete" counts have caught up with the snapshot of the "pending"
              values taken when PVRSRVSyncOpsTakeToken() was called.
              In the event that the dependencies are not (yet) met,
			  this call will auto-retry if bWait is specified, otherwise, it will
			  return PVRSRV_ERROR_RETRY.  (Not really an "error")

			  NB: "bWait" not supported now.

 @Input		psConnection

 @Input		hKernelSyncInfo

 @Input     psSyncToken

 @Input     bWait

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVSyncOpsFlushToToken(const PVRSRV_CONNECTION *psConnection,
#if defined (SUPPORT_SID_INTERFACE)
													const IMG_SID            hKernelSyncInfo,
#else
													const PVRSRV_CLIENT_SYNC_INFO *psSyncInfo,
#endif
													const PVRSRV_SYNC_TOKEN *psSyncToken,
													IMG_BOOL bWait)
{
	PVRSRV_BRIDGE_IN_SYNC_OPS_FLUSH_TO_TOKEN    sBridgeIn;
	PVRSRV_BRIDGE_RETURN						sBridgeOut;

	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

	
	if (!psConnection || (!psConnection->hServices))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsFlushToToken: Invalid connection"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

#if defined (SUPPORT_SID_INTERFACE)
	if (!psSyncToken || !hKernelSyncInfo)
#else
	if (!psSyncToken || !psSyncInfo)
#endif
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsFlushToToken: Invalid argument"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

#if defined (SUPPORT_SID_INTERFACE)
	if (psSyncToken->sPrivate.hKernelSyncInfo != hKernelSyncInfo)
#else
	if (psSyncToken->sPrivate.hKernelSyncInfo != psSyncInfo->hKernelSyncInfo)
#endif
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsFlushToToken: Invalid argument"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (bWait)
	{
		/* Don't use this path yet - it's broken - better to provide
		   bWait = IMG_FALSE and loop in the client driver */
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsFlushToToken, blocking call not supported"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

#if defined (SUPPORT_SID_INTERFACE)
	sBridgeIn.hKernelSyncInfo = hKernelSyncInfo;
#else
	sBridgeIn.hKernelSyncInfo = psSyncInfo->hKernelSyncInfo;
#endif
	sBridgeIn.ui32ReadOpsPendingSnapshot = psSyncToken->sPrivate.ui32ReadOpsPendingSnapshot;
	sBridgeIn.ui32WriteOpsPendingSnapshot = psSyncToken->sPrivate.ui32WriteOpsPendingSnapshot;
	sBridgeIn.ui32ReadOps2PendingSnapshot = psSyncToken->sPrivate.ui32ReadOps2PendingSnapshot;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_SYNC_OPS_FLUSH_TO_TOKEN,
						&sBridgeIn,
						sizeof(PVRSRV_BRIDGE_IN_SYNC_OPS_FLUSH_TO_TOKEN),
						&sBridgeOut,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsFlushToToken: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sBridgeOut.eError != PVRSRV_OK && sBridgeOut.eError != PVRSRV_ERROR_RETRY)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsFlushToToken: Error %d returned", sBridgeOut.eError));
		return sBridgeOut.eError;
	}

	return sBridgeOut.eError;
}
#endif /* OS_PVRSRV_SYNC_OPS_FLUSH_TO_TOKEN_UM */


#if !defined(OS_PVRSRV_SYNC_OPS_FLUSH_TO_MOD_OBJ_UM) && !defined(SRVINIT_MODULE)
/*!
 ******************************************************************************

 @Function	PVRSRVSyncOpsFlushToModObj

 @Description Tests whether the dependencies for a pending sync op modification
              have been satisfied.  If this function returns PVRSRV_OK, then the
              "complete" counts have caught up with the snapshot of the "pending"
              values taken when PVRSRVModifyPendingSyncOps() was called.
              PVRSRVModifyCompleteSyncOps() can then be called without risk of
			  stalling.  In the event that the dependencies are not (yet) met,
			  this call will auto-retry if bWait is specified, otherwise, it will
			  return PVRSRV_ERROR_RETRY.  (Not really an "error")

 @Input		psConnection

 @Input		hKernelSyncInfoModObj

 @Input     bWait

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVSyncOpsFlushToModObj(const PVRSRV_CONNECTION *psConnection,
#if defined (SUPPORT_SID_INTERFACE)
													 IMG_SID hKernelSyncInfoModObj,
#else
													 IMG_HANDLE hKernelSyncInfoModObj,
#endif
													 IMG_BOOL bWait)
{
	PVRSRV_BRIDGE_IN_SYNC_OPS_FLUSH_TO_MOD_OBJ	sBridgeIn;
	PVRSRV_BRIDGE_RETURN						sBridgeOut;


	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));


	if (!psConnection || (!psConnection->hServices))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsFlushToModObj: Invalid connection"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (bWait)
	{
		/* Don't use this path yet - it's broken - better to provide
		   bWait = IMG_FALSE and loop in the client driver */
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsFlushToModObj, blocking call not supported"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hKernelSyncInfoModObj = hKernelSyncInfoModObj;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_SYNC_OPS_FLUSH_TO_MOD_OBJ,
						&sBridgeIn,
						sizeof(PVRSRV_BRIDGE_IN_SYNC_OPS_FLUSH_TO_MOD_OBJ),
						&sBridgeOut,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsFlushToModObj: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sBridgeOut.eError != PVRSRV_OK && sBridgeOut.eError != PVRSRV_ERROR_RETRY)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsFlushToModObj: Error %d returned", sBridgeOut.eError));
		return sBridgeOut.eError;
	}

	return sBridgeOut.eError;
}
#endif /* OS_PVRSRV_SYNC_OPS_FLUSH_TO_MOD_OBJ_UM */


#if !defined(OS_PVRSRV_SYNC_OPS_FLUSH_TO_DELTA_UM) && !defined(SRVINIT_MODULE)
/*!
 ******************************************************************************

 @Function	PVRSRVSyncOpsFlushToDelta

 @Description Compares the number of outstanding operations (pending count minus
              complete count) with the limit specified.  If no more than ui32Delta
              operations are outstanding, this function returns PVRSRV_OK.
              In the event that there are too many outstanding operations,
			  this call will auto-retry if bWait is specified, otherwise, it will
			  return PVRSRV_ERROR_RETRY.  (Not really an "error")

 @Input		psConnection

 @Input		psClientSyncInfo

 @Input     ui32Delta

 @Input     bWait

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVSyncOpsFlushToDelta(const PVRSRV_CONNECTION *psConnection,
													PVRSRV_CLIENT_SYNC_INFO *psClientSyncInfo,
													IMG_UINT32 ui32Delta,
													IMG_BOOL bWait)
{
	PVRSRV_BRIDGE_IN_SYNC_OPS_FLUSH_TO_DELTA	sBridgeIn;
	PVRSRV_BRIDGE_RETURN						sBridgeOut;


	PVRSRVMemSet(&sBridgeIn,0x00,sizeof(sBridgeIn));
	PVRSRVMemSet(&sBridgeOut,0x00,sizeof(sBridgeOut));

	
	if (!psConnection || (!psConnection->hServices))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsFlushToDelta: Invalid connection"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (bWait)
	{
		/* Don't use this path yet - it's broken - better to provide
		   bWait = IMG_FALSE and loop in the client driver */
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsFlushToDelta, blocking call not supported"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sBridgeIn.hKernelSyncInfo = psClientSyncInfo->hKernelSyncInfo;
	sBridgeIn.ui32Delta = ui32Delta;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_SYNC_OPS_FLUSH_TO_DELTA,
						&sBridgeIn,
						sizeof(PVRSRV_BRIDGE_IN_SYNC_OPS_FLUSH_TO_DELTA),
						&sBridgeOut,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsFlushToDelta: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sBridgeOut.eError != PVRSRV_OK && sBridgeOut.eError != PVRSRV_ERROR_RETRY)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSyncOpsFlushToDelta: Error %d returned", sBridgeOut.eError));
		return sBridgeOut.eError;
	}

	return sBridgeOut.eError;
}
#endif /* OS_PVRSRV_SYNC_OPS_FLUSH_TO_DELTA_UM */


#if !defined(OS_PVRSRV_ALLOC_SYNC_INFO_UM) && !defined(SRVINIT_MODULE)
/*!
******************************************************************************

 @Function	PVRSRVAllocSyncInfo

 @Description	Creates a sync object

 @Input		psDevData : DevData for the device

 @Output	**ppsSyncInfo : Receives a pointer to the created PVRSRV_CLIENT_SYNC_INFO structure

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVAllocSyncInfo(const PVRSRV_DEV_DATA *psDevData,
											  PVRSRV_CLIENT_SYNC_INFO **ppsSyncInfo)
{
	PVRSRV_BRIDGE_OUT_ALLOC_SYNC_INFO sOut;
	PVRSRV_BRIDGE_IN_ALLOC_SYNC_INFO sIn;
	PVRSRV_CLIENT_SYNC_INFO *psSyncInfo;
	PVRSRV_ERROR eError;

	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));
	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	
	if(!psDevData || !ppsSyncInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocSyncInfo: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	PVRSRVMemSet(&sOut, 0, sizeof(sOut));

	/* Allocate memory for the syncinfo */
	psSyncInfo = (PVRSRV_CLIENT_SYNC_INFO *)PVRSRVAllocUserModeMem(sizeof(PVRSRV_CLIENT_SYNC_INFO));

	if (!psSyncInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocSyncInfo: Alloc failed"));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto allocsyncinfo_error;
	}

	sIn.hDevCookie   = psDevData->hDevCookie;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_ALLOC_SYNC_INFO,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_ALLOC_SYNC_INFO),
						(IMG_VOID *)&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_ALLOC_SYNC_INFO)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocSyncInfo: BridgeCall failed"));

		eError = PVRSRV_ERROR_BRIDGE_CALL_FAILED;
		goto allocsyncinfo_error_freesyncinfo;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocSyncInfo: Error %d returned", sOut.eError));

		eError = sOut.eError;
		goto allocsyncinfo_error_freesyncinfo;
	}

	psSyncInfo->hKernelSyncInfo = sOut.hKernelSyncInfo;
#if !defined(PVRSRV_DISABLE_UM_SYNCOBJ_MAPPINGS)
	psSyncInfo->psSyncData = IMG_NULL; // mapping of sync data is deprecated
	psSyncInfo->hMappingInfo = IMG_NULL; // mapping of sync data is deprecated
	psSyncInfo->sWriteOpsCompleteDevVAddr.uiAddr = 0; // storing of device virt addrs here is deprecated
	psSyncInfo->sReadOpsCompleteDevVAddr.uiAddr = 0; // storing of device virt addrs here is deprecated
	psSyncInfo->sReadOps2CompleteDevVAddr.uiAddr = 0; // storing of device virt addrs here is deprecated
#endif

	eError = PVRSRV_OK;

	*ppsSyncInfo = psSyncInfo;

	goto allocsyncinfo_return;

 allocsyncinfo_error_freesyncinfo:
	PVRSRVFreeUserModeMem(psSyncInfo);
 allocsyncinfo_error:
	PDUMPCOMMENT(psDevData->psConnection, "PVRSRVAllocSyncInfo failed", IMG_TRUE);

 allocsyncinfo_return:
	return eError;
}
#endif /* OS_PVRSRV_ALLOC_SYNC_INFO_UM */


#if !defined(OS_PVRSRV_FREE_SYNC_INFO_UM) && !defined(SRVINIT_MODULE)
/*!
 ******************************************************************************

 @Function	PVRSRVFreeSyncInfo

 @Description	Destroys a sync object

 @Input		psDevData : DevData for the device

 @Input		psSyncInfo : The PVRSRV_CLIENT_SYNC_INFO structure representing
                           the sync object to be destroyed

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVFreeSyncInfo(const PVRSRV_DEV_DATA *psDevData,
											 PVRSRV_CLIENT_SYNC_INFO *psSyncInfo)
{
	PVRSRV_BRIDGE_RETURN sOut;
	PVRSRV_BRIDGE_IN_FREE_SYNC_INFO sIn;
	PVRSRV_ERROR eError;

	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));
	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	
	if(!psDevData || !psSyncInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVFreeSyncInfo: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	PVRSRVMemSet(&sOut, 0, sizeof(sOut));

	sIn.hKernelSyncInfo = psSyncInfo->hKernelSyncInfo;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_FREE_SYNC_INFO,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_FREE_SYNC_INFO),
						(IMG_VOID *)&sOut,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVFreeSyncInfo: BridgeCall failed"));
		PDUMPCOMMENT(psDevData->psConnection, "PVRSRVFreeSyncInfo failed", IMG_TRUE);
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVFreeSyncInfo: Error %d returned", sOut.eError));
		PDUMPCOMMENT(psDevData->psConnection, "PVRSRVFreeSyncInfo failed", IMG_TRUE);
		return sOut.eError;
	}

	/* Finally, free the memory for the syncinfo */
	PVRSRVFreeUserModeMem(psSyncInfo);

	eError = PVRSRV_OK;

	return eError;
}
#endif /* OS_PVRSRV_FREE_SYNC_INFO_UM */


/******************************************************************************
 End of file (bridged_pvr_glue.c)
******************************************************************************/
