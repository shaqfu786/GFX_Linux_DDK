/*!****************************************************************************
@File           bridged_pvr_bc_glue.c

@Title          Shared PVR Buffer Class glue code

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

@Description    Implements shared PVRSRV BC API user bridge code

Modifications :-
$Log: bridged_pvr_bc_glue.c $
******************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "img_defs.h"
#include "services.h"
#include "pvr_bridge.h"
#include "pvr_bridge_client.h"
#include "pdump.h"
#include "pvr_debug.h"

#include "os_srvclient_config.h"


#if !defined(OS_PVRSRV_OPEN_BC_DEVICE_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVOpenBCDevice

 @Description

 Opens a connection to a buffer device specified by the device id/index argument.
 Calls into kernel services to retrieve device information structure associated
 with the device id.

 @Input psDevData	- device data representing device into which buffers 
 are subsequently mapped
 @Input ui32DeviceID	- device identifier

 @Return
success: handle to matching buffer class device
failure: IMG_NULL

 ******************************************************************************/
IMG_EXPORT
IMG_HANDLE IMG_CALLCONV PVRSRVOpenBCDevice(const PVRSRV_DEV_DATA *psDevData,
										   IMG_UINT32 ui32DeviceID)
{
	PVRSRV_BRIDGE_IN_OPEN_BUFFERCLASS_DEVICE sIn;
	PVRSRV_BRIDGE_OUT_OPEN_BUFFERCLASS_DEVICE sOut;
	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo;

	if(!psDevData)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVOpenBCDevice: Invalid DevData"));
		return IMG_NULL;
	}

	/* Alloc client info structure first, makes clean-up easier if KM fails */
	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO *)PVRSRVAllocUserModeMem(sizeof(PVRSRV_CLIENT_DEVICECLASS_INFO));
	if (psDevClassInfo == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVOpenBCDevice: DevClassInfo alloc failed"));
		return IMG_NULL;
	}

	sIn.ui32DeviceID = ui32DeviceID;
	sIn.hDevCookie = psDevData->hDevCookie;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_OPEN_BUFFERCLASS_DEVICE,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_OPEN_BUFFERCLASS_DEVICE),
						&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_OPEN_BUFFERCLASS_DEVICE)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVOpenBCDevice: BridgeCall failed"));
		goto ErrorExit;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetBCBufferInfo: Error - %d returned", sOut.eError));
		goto ErrorExit;
	}

	/* setup private client services structure */
	psDevClassInfo->hServices = psDevData->psConnection->hServices;
	psDevClassInfo->hDeviceKM = sOut.hDeviceKM;

	/* return private client services structure as handle */
	return (IMG_HANDLE)psDevClassInfo;

ErrorExit:
	/* free private client structure */
	PVRSRVFreeUserModeMem(psDevClassInfo);
	return IMG_NULL;

}
#endif /* OS_PVRSRV_OPEN_BC_DEVICE_UM */



#if !defined(OS_PVRSRV_CLOSE_BC_DEVICE_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVCloseBCDevice

 @Description

 Closes a connection to a buffer device specified by the device handle

 @Input hDevice - device handle

 @Return 
success: OK
failure: ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVCloseBCDevice(const PVRSRV_CONNECTION *psConnection, 
											  IMG_HANDLE hDevice)
{
	PVRSRV_BRIDGE_RETURN sOut;
	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo;
	PVRSRV_BRIDGE_IN_CLOSE_BUFFERCLASS_DEVICE sIn;

	if(!psConnection || !hDevice)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVCloseBCDevice: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO*)hDevice;

	sIn.hDeviceKM = psDevClassInfo->hDeviceKM;

	if(PVRSRVBridgeCall(psConnection->hServices, 
						PVRSRV_BRIDGE_CLOSE_BUFFERCLASS_DEVICE,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_CLOSE_BUFFERCLASS_DEVICE),
						&sOut,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVCloseBCDevice: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	/* free private client structure */
	PVRSRVFreeUserModeMem(psDevClassInfo);

	return sOut.eError;
}
#endif /* OS_PVRSRV_CLOSE_BC_DEVICE_UM */


#if !defined(OS_PVRSRV_GET_BC_BUFFER_INFO_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVGetBCBufferInfo

 @Description

 Gets information about the buffer class buffers

 @Input hDevice - device handle
 @Output psBuffer - pointer to structure containing buffer information

 @Return
success: OK
failure: ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVGetBCBufferInfo(IMG_HANDLE hDevice,
												BUFFER_INFO *psBufferInfo)
{
	PVRSRV_BRIDGE_OUT_GET_BUFFERCLASS_INFO sOut;
	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo;
	PVRSRV_BRIDGE_IN_GET_BUFFERCLASS_INFO sIn;

	if (!hDevice || !psBufferInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetBCBufferInfo: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;	
	}

	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO*)hDevice;

	sIn.hDeviceKM = psDevClassInfo->hDeviceKM;

	if(PVRSRVBridgeCall(psDevClassInfo->hServices,
						PVRSRV_BRIDGE_GET_BUFFERCLASS_INFO,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_GET_BUFFERCLASS_INFO),
						&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_GET_BUFFERCLASS_INFO)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetBCBufferInfo: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError == PVRSRV_OK)
	{
		*psBufferInfo = sOut.sBufferInfo;
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetBCBufferInfo: Error - %d returned", sOut.eError));
	}

	return sOut.eError;

}
#endif /* OS_PVRSRV_GET_BC_BUFFER_INFO_UM */


#if !defined(OS_PVRSRV_GET_BC_BUFFER_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVGetBCBuffer

 @Description

 Retrieves buffer handle given zero based buffer index

 @Input hDevice - device handle
 @Input ui32BufferIndex - buffer index
 @Output phBuffer - pointer to buffer handle

 @Return 
success: OK
failure: ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVGetBCBuffer(IMG_HANDLE hDevice,
											IMG_UINT32 ui32BufferIndex,
#if defined (SUPPORT_SID_INTERFACE)
											IMG_SID   *phBuffer)
#else
											IMG_HANDLE *phBuffer)
#endif
{
	PVRSRV_BRIDGE_IN_GET_BUFFERCLASS_BUFFER sIn;
	PVRSRV_BRIDGE_OUT_GET_BUFFERCLASS_BUFFER sOut;

	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo;

	if (!hDevice || !phBuffer)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetBCBuffer: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;	
	}

	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO*)hDevice;

	sIn.hDeviceKM = psDevClassInfo->hDeviceKM;
	sIn.ui32BufferIndex = ui32BufferIndex;
	*phBuffer = IMG_NULL;

	if(PVRSRVBridgeCall(psDevClassInfo->hServices,
						PVRSRV_BRIDGE_GET_BUFFERCLASS_BUFFER,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_GET_BUFFERCLASS_BUFFER),
						&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_GET_BUFFERCLASS_BUFFER)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetBCBuffer: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError == PVRSRV_OK)
	{
		*phBuffer = sOut.hBuffer;
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetBCBuffer: Error - %d returned", sOut.eError));
	}

	return sOut.eError;
}
#endif /* OS_PVRSRV_GET_BC_BUFFER_UM */

/******************************************************************************
 End of file (bridged_pvr_bc_glue.c)
******************************************************************************/
