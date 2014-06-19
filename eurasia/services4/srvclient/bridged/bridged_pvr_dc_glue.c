/*!****************************************************************************
@File           bridged_pvr_dc_glue.c

@Title          Shared PVR Display Class glue code

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

@Description    Implements shared PVRSRV DC API user bridge code

Modifications :-
$Log: bridged_pvr_dc_glue.c $
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

#if !defined(OS_PVRSRV_OPEN_DC_DEVICE_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVOpenDCDevice

 @Description

 Opens a connection to a display device specified by the device id/index argument.
 Calls into kernel services to retrieve device information structure associated
 with the device id.
 If matching device structure is found it's used to load the appropriate
 external client-side driver library and sets up data structures and a function 
 jump table into the external driver

 @Input		psDevData	- handle for services connection
 @Inpu		ui32DeviceID	- device identifier

 @Return	IMG_HANDLE for matching display class device (IMG_NULL on failure)

 ******************************************************************************/
IMG_EXPORT
IMG_HANDLE IMG_CALLCONV PVRSRVOpenDCDevice(const PVRSRV_DEV_DATA *psDevData,
										   IMG_UINT32 ui32DeviceID)
{
	PVRSRV_BRIDGE_IN_OPEN_DISPCLASS_DEVICE sIn;
	PVRSRV_BRIDGE_OUT_OPEN_DISPCLASS_DEVICE sOut;
	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo=IMG_NULL;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));


	if(!psDevData)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVOpenDCDevice: Invalid params"));
		return IMG_NULL;
	}

	/* Alloc client info structure first, makes clean-up easier if KM fails */
	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO *)PVRSRVAllocUserModeMem(sizeof(PVRSRV_CLIENT_DEVICECLASS_INFO));
	if (psDevClassInfo == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVOpenDCDevice: Alloc failed"));
		return IMG_NULL;
	}

	sIn.ui32DeviceID = ui32DeviceID;
	sIn.hDevCookie = psDevData->hDevCookie;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices, 
						PVRSRV_BRIDGE_OPEN_DISPCLASS_DEVICE, 
						&sIn, 
						sizeof(PVRSRV_BRIDGE_IN_OPEN_DISPCLASS_DEVICE),
						&sOut, 
						sizeof(PVRSRV_BRIDGE_OUT_OPEN_DISPCLASS_DEVICE)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVOpenDCDevice: BridgeCall failed"));
		goto ErrorExit;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVOpenDCDevice: Error - %d returned", sOut.eError));
		goto ErrorExit;
	}

	/* setup private client services structure */
	psDevClassInfo->hServices = psDevData->psConnection->hServices;
	psDevClassInfo->hDeviceKM = sOut.hDeviceKM;

	/* return private client services structure as handle */
	return (IMG_HANDLE)psDevClassInfo;

ErrorExit:
	PVRSRVFreeUserModeMem(psDevClassInfo);
	return IMG_NULL;
}
#endif /* OS_PVRSRV_OPEN_DC_DEVICE_UM */


#if !defined(OS_PVRSRV_CLOSE_DC_DEVICE_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVCloseDCDevice

 @Description	Closes connection to a display device specified by the handle

 @Input hDevice - device handle

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVCloseDCDevice(const PVRSRV_CONNECTION *psConnection,
											  IMG_HANDLE hDevice)
{
	PVRSRV_BRIDGE_RETURN sOut;
	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo=IMG_NULL;
	PVRSRV_BRIDGE_IN_CLOSE_DISPCLASS_DEVICE sIn;

	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));
	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));

	if(!psConnection || !hDevice)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVCloseDCDevice: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO*)hDevice;

	sIn.hDeviceKM = psDevClassInfo->hDeviceKM;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_CLOSE_DISPCLASS_DEVICE,
						&sIn, 
						sizeof(PVRSRV_BRIDGE_IN_CLOSE_DISPCLASS_DEVICE),
						&sOut,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVCloseDCDevice: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVCloseDCDevice: Error - %d returned", sOut.eError));
		return sOut.eError;
	}
	/* free private client structure */
	PVRSRVFreeUserModeMem(psDevClassInfo);

	return sOut.eError;	
}
#endif /* OS_PVRSRV_CLOSE_DC_DEVICE_UM */


#if !defined(OS_PVRSRV_ENUM_DC_FORMATS_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVEnumDCFormats

 @Description	Enumerates displays pixel formats

 @Input hDevice		- device handle
 @Output pui32Count	- number of formats
 @Output psFormat - buffer to return formats in

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVEnumDCFormats(IMG_HANDLE	hDevice,
											  IMG_UINT32	*pui32Count,
											  DISPLAY_FORMAT *psFormat)
{
	PVRSRV_BRIDGE_OUT_ENUM_DISPCLASS_FORMATS sOut;
	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo=IMG_NULL;
	PVRSRV_BRIDGE_IN_ENUM_DISPCLASS_FORMATS sIn;

	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));
	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));

	if (!pui32Count || !hDevice)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVEnumDCFormats: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;	
	}

	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO*)hDevice;

	sIn.hDeviceKM = psDevClassInfo->hDeviceKM;

	if(PVRSRVBridgeCall(psDevClassInfo->hServices, 
						PVRSRV_BRIDGE_ENUM_DISPCLASS_FORMATS, 
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_ENUM_DISPCLASS_FORMATS),
						&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_ENUM_DISPCLASS_FORMATS)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVEnumDCFormats: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVEnumDCFormats: Error - %d returned", sOut.eError));
		return sOut.eError;
	}

	/*
	   call may not preallocate the max psFormats and
	   instead query the count and then enum
	   */
	if(psFormat)
	{
		IMG_UINT32 i;

		//PVR_ASSERT(sOut.ui32Count <= PVRSRV_DISPCLASS_MAX_FORMATS);

		for(i=0; i<sOut.ui32Count; i++)
		{
			psFormat[i] = sOut.asFormat[i];
		}
		*pui32Count = sOut.ui32Count;
	}
	else
	{
		*pui32Count = sOut.ui32Count;
	}

	return PVRSRV_OK;
}
#endif /* OS_PVRSRV_ENUM_DC_FORMATS_UM */


#if !defined(OS_PVRSRV_ENUM_DC_DIMS_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVEnumDCDims

 @Description	Enumerates dimensions for a given format

 @Input hDevice		- device handle
 @Input psFormat - buffer to return formats in
 @Output pui32Count	- number of dimensions
 @Output pDims	- number of dimensions

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVEnumDCDims(IMG_HANDLE hDevice,
										   IMG_UINT32 *pui32Count,
										   DISPLAY_FORMAT *psFormat,
										   DISPLAY_DIMS *psDims)
{
	PVRSRV_BRIDGE_IN_ENUM_DISPCLASS_DIMS sIn;
	PVRSRV_BRIDGE_OUT_ENUM_DISPCLASS_DIMS sOut;
	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo=IMG_NULL;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));

	if (!pui32Count || !hDevice)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVEnumDCDims: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO*)hDevice;

	sIn.hDeviceKM = psDevClassInfo->hDeviceKM;
	sIn.sFormat = *psFormat;

	if(PVRSRVBridgeCall(psDevClassInfo->hServices, 
						PVRSRV_BRIDGE_ENUM_DISPCLASS_DIMS, 
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_ENUM_DISPCLASS_DIMS),
						&sOut, 
						sizeof(PVRSRV_BRIDGE_OUT_ENUM_DISPCLASS_DIMS)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVEnumDCDims: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVEnumDCDims: Error - %d returned", sOut.eError));
		return sOut.eError;
	}

	/*
	   call may not preallocate the max psFormats and
	   instead query the count and then enum 
	   */
	if(psDims)
	{
		IMG_UINT32 i;

		//PVR_ASSERT(sOut.ui32Count <= PVRSRV_DISPCLASS_MAX_DIMS);

		for(i=0; i<sOut.ui32Count; i++)
		{
			psDims[i] = sOut.asDim[i];
		}
		*pui32Count = sOut.ui32Count;
	}
	else
	{
		*pui32Count = sOut.ui32Count;
	}

	return PVRSRV_OK;
}
#endif /* OS_PVRSRV_ENUM_DC_DIMS_UM */


#if !defined(OS_PVRSRV_GET_DC_SYSTEM_BUFFER_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVGetDCSystemBuffer

 @Description	Returns handle to display's 'system buffer' (primary surface)

 @Input hDevice		- device handle
 @Output phBuffer	- handle to buffer

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVGetDCSystemBuffer(IMG_HANDLE hDevice,
#if defined (SUPPORT_SID_INTERFACE)
												  IMG_SID   *phBuffer)
#else
												  IMG_HANDLE *phBuffer)
#endif
{
	PVRSRV_BRIDGE_OUT_GET_DISPCLASS_SYSBUFFER sOut;
	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo=IMG_NULL;
	PVRSRV_BRIDGE_IN_GET_DISPCLASS_SYSBUFFER sIn;


	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));
	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));

	if (!hDevice || !phBuffer)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetDCSystemBuffer: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;	
	}

	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO*)hDevice;

	sIn.hDeviceKM = psDevClassInfo->hDeviceKM;

	if(PVRSRVBridgeCall(psDevClassInfo->hServices,
						PVRSRV_BRIDGE_GET_DISPCLASS_SYSBUFFER,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_GET_DISPCLASS_SYSBUFFER),
						&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_GET_DISPCLASS_SYSBUFFER)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetDCSystemBuffer: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetDCSystemBuffer: Error - %d returned", sOut.eError));
		return sOut.eError;
	}

	*phBuffer = sOut.hBuffer;

	return PVRSRV_OK;
}
#endif /* OS_PVRSRV_GET_DC_SYSTEM_BUFFER_UM */


#if !defined(OS_PVRSRV_GET_DC_INFO_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVGetDCInfo

 @Description

 returns information about the display device

 @Input		hDevice 		- handle for device
 @Input		psDisplayInfo	- display device information

 @Return PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVGetDCInfo(IMG_HANDLE hDevice,
										  DISPLAY_INFO *psDisplayInfo)
{
	PVRSRV_BRIDGE_OUT_GET_DISPCLASS_INFO sOut;
	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo;
	PVRSRV_BRIDGE_IN_GET_DISPCLASS_INFO sIn;
	
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));
	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	
	if (!hDevice || !psDisplayInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetDCInfo: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;	
	}

	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO*)hDevice;

	sIn.hDeviceKM = psDevClassInfo->hDeviceKM;

	if(PVRSRVBridgeCall(psDevClassInfo->hServices,
						PVRSRV_BRIDGE_GET_DISPCLASS_INFO,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_GET_DISPCLASS_INFO),
						&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_GET_DISPCLASS_INFO)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetDCInfo: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetDCInfo: Error - %d returned", sOut.eError));
		return sOut.eError;
	}

	*psDisplayInfo = sOut.sDisplayInfo;

	return PVRSRV_OK;
}
#endif /* OS_PVRSRV_GET_DC_INFO_UM */


#if !defined(OS_PVRSRV_CREATE_DC_SWAP_CHAIN_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVCreateDCSwapChain

 @Description

 Creates a swap chain on the specified display class device

 @Input hDevice				- device handle
 @Input ui32Flags			- flags
 @Input psDstSurfAttrib		- DST surface attribs
 @Input psSrcSurfAttrib		- SRC surface attribs
 @Input ui32BufferCount		- number of buffers in chain
 @Input ui32OEMFlags		- OEM flags for OEM extended functionality
 @Input pui32SwapChainID 	- ID assigned to swapchain (optional feature)
 @Output phSwapChain		- handle to swapchain

 @Return 
 	success: PVRSRV_OK
 	failure: PVRSRV_ERROR_*

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVCreateDCSwapChain(IMG_HANDLE	hDevice,
												  IMG_UINT32	ui32Flags,
												  DISPLAY_SURF_ATTRIBUTES *psDstSurfAttrib,
												  DISPLAY_SURF_ATTRIBUTES *psSrcSurfAttrib,
												  IMG_UINT32	ui32BufferCount,
												  IMG_UINT32	ui32OEMFlags,
												  IMG_UINT32	*pui32SwapChainID,
#if defined (SUPPORT_SID_INTERFACE)
												  IMG_SID		*phSwapChain)
#else
												  IMG_HANDLE	*phSwapChain)
#endif

{
	PVRSRV_BRIDGE_IN_CREATE_DISPCLASS_SWAPCHAIN sIn;
	PVRSRV_BRIDGE_OUT_CREATE_DISPCLASS_SWAPCHAIN sOut;
	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));


	if(!hDevice 
	   || !psDstSurfAttrib 
	   || !psSrcSurfAttrib 
	   || !pui32SwapChainID 
	   || !phSwapChain)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVCreateDCSwapChain: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;	
	}

	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO*)hDevice;

	sIn.hDeviceKM = psDevClassInfo->hDeviceKM;
	sIn.ui32Flags = ui32Flags;
	sIn.sDstSurfAttrib = *psDstSurfAttrib;
	sIn.sSrcSurfAttrib = *psSrcSurfAttrib;
	sIn.ui32BufferCount = ui32BufferCount;
	sIn.ui32OEMFlags = ui32OEMFlags;
	/* optional ID (in/out) */
	sIn.ui32SwapChainID = *pui32SwapChainID;

	if(PVRSRVBridgeCall(psDevClassInfo->hServices, 
						PVRSRV_BRIDGE_CREATE_DISPCLASS_SWAPCHAIN, 
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_CREATE_DISPCLASS_SWAPCHAIN),
						&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_CREATE_DISPCLASS_SWAPCHAIN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVCreateDCSwapChain: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVCreateDCSwapChain: Error - %d returned", sOut.eError));
		return sOut.eError;
	}

	/* Assign output */
	*phSwapChain = sOut.hSwapChain;
	/* optional ID (in/out) */
	*pui32SwapChainID = sOut.ui32SwapChainID;

	return PVRSRV_OK;
}
#endif /* OS_PVRSRV_CREATE_DC_SWAP_CHAIN_UM */


#if !defined(OS_PVRSRV_DESTROY_DC_SWAP_CHAIN_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVDestroyDCSwapChain

 @Description

 Destroy a swap chain on the specified display class device

 @Input hDevice			- device handle
 @Input hSwapChain		- handle to swapchain

 @Return 
	success: PVRSRV_OK
	failure: PVRSRV_ERROR_*

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVDestroyDCSwapChain(IMG_HANDLE hDevice,
#if defined (SUPPORT_SID_INTERFACE)
												   IMG_SID    hSwapChain)
#else
												   IMG_HANDLE hSwapChain)
#endif
{
	PVRSRV_BRIDGE_IN_DESTROY_DISPCLASS_SWAPCHAIN	sIn;
	PVRSRV_BRIDGE_RETURN sOut;	

	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo;


	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));
	
	if (!hDevice || !hSwapChain)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVDestroyDCSwapChain: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO*)hDevice;

	sIn.hDeviceKM = psDevClassInfo->hDeviceKM;
	sIn.hSwapChain = hSwapChain;

	if(PVRSRVBridgeCall(psDevClassInfo->hServices,
						PVRSRV_BRIDGE_DESTROY_DISPCLASS_SWAPCHAIN,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_DESTROY_DISPCLASS_SWAPCHAIN),
						&sOut,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVDestroyDCSwapChain: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sOut.eError;
}
#endif /* OS_PVRSRV_DESTROY_DC_SWAP_CHAIN_UM */


#if !defined(OS_PVRSRV_SET_DC_DST_RECT_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVSetDCDstRect

 @Description

 Set the DC Destination rectangle

 @Input hDevice			- device handle
 @Input hSwapChain		- handle to swapchain
 @Input psDstRect		- DST rectangle

 @Return 
 	success: PVRSRV_OK
 	failure: PVRSRV_ERROR_*

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVSetDCDstRect(IMG_HANDLE hDevice,
#if defined (SUPPORT_SID_INTERFACE)
											 IMG_SID    hSwapChain,
#else
											 IMG_HANDLE hSwapChain,
#endif
											 IMG_RECT *psDstRect)

{
	PVRSRV_BRIDGE_IN_SET_DISPCLASS_RECT	sIn;
	PVRSRV_BRIDGE_RETURN sOut;

	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo;


	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));

	if (!hDevice || !hSwapChain || !psDstRect)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSetDCDstRect: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO*)hDevice;

	sIn.hDeviceKM = psDevClassInfo->hDeviceKM;
	sIn.hSwapChain = hSwapChain;
	sIn.sRect = *psDstRect;

	if(PVRSRVBridgeCall(psDevClassInfo->hServices, 
						PVRSRV_BRIDGE_SET_DISPCLASS_DSTRECT, 
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_SET_DISPCLASS_RECT),
						&sOut,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSetDCDstRect: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sOut.eError;
}
#endif /* OS_PVRSRV_SET_DC_DST_RECT_UM */


#if !defined(OS_PVRSRV_SET_DC_SRC_RECT_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVSetDCSrcRect

 @Description

 Set the DC source rectangle

 @Input hDevice			- device handle
 @Input hSwapChain		- handle to swapchain
 @Input psSrcRect		- SRC rectangle

 @Return 
 	success: PVRSRV_OK
 	failure: PVRSRV_ERROR_*

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVSetDCSrcRect(IMG_HANDLE hDevice,
#if defined (SUPPORT_SID_INTERFACE)
											 IMG_SID    hSwapChain,
#else
											 IMG_HANDLE hSwapChain,
#endif
											 IMG_RECT  *psSrcRect)

{
	PVRSRV_BRIDGE_IN_SET_DISPCLASS_RECT	sIn;
	PVRSRV_BRIDGE_RETURN sOut;

	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));

	if (!hDevice || !hSwapChain || !psSrcRect)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSetDCSrcRect: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;	
	}

	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO*)hDevice;

	sIn.hDeviceKM = psDevClassInfo->hDeviceKM;
	sIn.hSwapChain = hSwapChain;
	sIn.sRect = *psSrcRect;

	if(PVRSRVBridgeCall(psDevClassInfo->hServices, 
						PVRSRV_BRIDGE_SET_DISPCLASS_SRCRECT, 
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_SET_DISPCLASS_RECT),
						&sOut, 
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSetDCSrcRect: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sOut.eError;
}
#endif /* OS_PVRSRV_SET_DC_SRC_RECT_UM */


#if !defined(OS_PVRSRV_SET_DC_DST_COLOUR_KEY_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVSetDCDstColourKey

 @Description

 Set the DC DST colour key colour

 @Input hDevice			- device handle
 @Input hSwapChain		- handle to swapchain
 @Input ui32CKColour	- CK colour

 @Return 
 	success: PVRSRV_OK
 	failure: PVRSRV_ERROR_*

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVSetDCDstColourKey(IMG_HANDLE hDevice,
#if defined (SUPPORT_SID_INTERFACE)
												  IMG_SID    hSwapChain,
#else
												  IMG_HANDLE hSwapChain,
#endif
												  IMG_UINT32 ui32CKColour)

{
	PVRSRV_BRIDGE_IN_SET_DISPCLASS_COLOURKEY sIn;
	PVRSRV_BRIDGE_RETURN sOut;

	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));
	
	if (!hDevice || !hSwapChain)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSetDCDstColourKey: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO*)hDevice;

	sIn.hDeviceKM = psDevClassInfo->hDeviceKM;
	sIn.hSwapChain = hSwapChain;
	sIn.ui32CKColour = ui32CKColour;

	if(PVRSRVBridgeCall(psDevClassInfo->hServices, 
						PVRSRV_BRIDGE_SET_DISPCLASS_DSTCOLOURKEY, 
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_SET_DISPCLASS_COLOURKEY),
						&sOut, 
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSetDCDstColourKey: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sOut.eError;
}
#endif /* OS_PVRSRV_SET_DC_DST_COLOUR_KEY_UM */


#if !defined(OS_PVRSRV_SET_DC_SRC_COLOUR_KEY_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVSetDCSrcColourKey

 @Description

 Set the DC SRC colour key colour

 @Input hDevice			- device handle
 @Input hSwapChain		- handle to swapchain
 @Input ui32CKColour	- CK colour

 @Return 
 	success: PVRSRV_OK
 	failure: PVRSRV_ERROR_*

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVSetDCSrcColourKey(IMG_HANDLE hDevice,
#if defined (SUPPORT_SID_INTERFACE)
												  IMG_SID    hSwapChain,
#else
												  IMG_HANDLE hSwapChain,
#endif
												  IMG_UINT32 ui32CKColour)

{
	PVRSRV_BRIDGE_IN_SET_DISPCLASS_COLOURKEY sIn;
	PVRSRV_BRIDGE_RETURN sOut;

	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo;


	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));

	if (!hDevice || !hSwapChain)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSetDCSrcColourKey: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO*)hDevice;

	sIn.hDeviceKM = psDevClassInfo->hDeviceKM;
	sIn.hSwapChain = hSwapChain;
	sIn.ui32CKColour = ui32CKColour;

	if(PVRSRVBridgeCall(psDevClassInfo->hServices,
						PVRSRV_BRIDGE_SET_DISPCLASS_SRCCOLOURKEY,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_SET_DISPCLASS_COLOURKEY),
						&sOut,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSetDCSrcColourKey: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sOut.eError;
}
#endif /* OS_PVRSRV_SET_DC_SRC_COLOUR_KEY_UM */


#if !defined(OS_PVRSRV_GET_DC_BUFFERS_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVGetDCBuffers2

 @Description

 Gets buffer handles for buffers in swapchain

 @Input hDevice			- device handle
 @Input hSwapChain		- handle to swapchain
 @Input phBuffer		- pointer to array of buffer handles
 							Note: array length defined by ui32BufferCount 
 							passed to PVRSRVCreateDCSwapChain
 @Input psPhyAddr		- pointer to array of SYS physical addresses
 							Note: array length defined by ui32BufferCount 
 							passed to PVRSRVCreateDCSwapChain
 @Return 
 	success: PVRSRV_OK
 	failure: PVRSRV_ERROR_*

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVGetDCBuffers2(IMG_HANDLE hDevice,
#if defined (SUPPORT_SID_INTERFACE)
											  IMG_SID    hSwapChain,
											  IMG_SID   *phBuffer,
#else
											  IMG_HANDLE hSwapChain,
											  IMG_HANDLE *phBuffer,
#endif
											  IMG_SYS_PHYADDR *psPhyAddr)
{
	PVRSRV_BRIDGE_IN_GET_DISPCLASS_BUFFERS sIn;
	PVRSRV_BRIDGE_OUT_GET_DISPCLASS_BUFFERS sOut;
	IMG_UINT32 i;

	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo;


	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));

	if (!hDevice || !hSwapChain || !phBuffer)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetDCBuffers: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;	
	}

	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO*)hDevice;

	sIn.hDeviceKM = psDevClassInfo->hDeviceKM;
	sIn.hSwapChain = hSwapChain;

	if(PVRSRVBridgeCall(psDevClassInfo->hServices, 
						PVRSRV_BRIDGE_GET_DISPCLASS_BUFFERS, 
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_GET_DISPCLASS_BUFFERS),
						&sOut, 
						sizeof(PVRSRV_BRIDGE_OUT_GET_DISPCLASS_BUFFERS)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetDCBuffers: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetDCBuffers: Error - %d returned", sOut.eError));
		return sOut.eError;
	}

	/* Assign output */
	PVR_ASSERT(sOut.ui32BufferCount <= PVRSRV_MAX_DC_SWAPCHAIN_BUFFERS);

	for (i=0; i<sOut.ui32BufferCount; i++)
	{
		phBuffer[i] = sOut.ahBuffer[i];
	}

	if(psPhyAddr)
	{
		for(i = 0; i < sOut.ui32BufferCount; i++)
		{
			psPhyAddr[i] = sOut.asPhyAddr[i];
		}
	}

	return PVRSRV_OK;
}

/*!
 ******************************************************************************

 @Function	PVRSRVGetDCBuffers

 @Description

 Gets buffer handles for buffers in swapchain

 @Input hDevice			- device handle
 @Input hSwapChain		- handle to swapchain
 @Input phBuffer		- pointer to array of buffer handles
 							Note: array length defined by ui32BufferCount 
 							passed to PVRSRVCreateDCSwapChain
 @Return 
 	success: PVRSRV_OK
 	failure: PVRSRV_ERROR_*

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVGetDCBuffers(IMG_HANDLE hDevice,
#if defined (SUPPORT_SID_INTERFACE)
											 IMG_SID    hSwapChain,
											 IMG_SID   *phBuffer)
#else
											 IMG_HANDLE hSwapChain,
											 IMG_HANDLE *phBuffer)
#endif
{
	return PVRSRVGetDCBuffers2(hDevice, hSwapChain, phBuffer, IMG_NULL);
}

#endif /* OS_PVRSRV_GET_DC_BUFFERS_UM */


#if !defined(OS_PVRSRV_SWAP_TO_DC_BUFFER_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVSwapToDCBuffer

 @Description		Swap display to a display class buffer

 @Input hDevice		- device handle
 @Input hBuffer		- buffer handle
 @Input ui32ClipRectCount - clip rectangle count
 @Input psClipRect - array of clip rects
 @Input ui32SwapInterval - number of Vsync intervals between swaps
 @Input hPrivateTag - Private Tag to passed through display 
 handling pipeline (audio sync etc.)

 @Return
 	success: PVRSRV_OK
 	failure: PVRSRV_ERROR_*

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVSwapToDCBuffer(IMG_HANDLE	hDevice,
#if defined (SUPPORT_SID_INTERFACE)
											   IMG_SID		hBuffer,
#else
											   IMG_HANDLE	hBuffer,
#endif
											   IMG_UINT32	ui32ClipRectCount,
											   IMG_RECT		*psClipRect,
											   IMG_UINT32	ui32SwapInterval,
#if defined (SUPPORT_SID_INTERFACE)
											   IMG_SID		hPrivateTag)
#else
											   IMG_HANDLE	hPrivateTag)
#endif
{
	PVRSRV_BRIDGE_IN_SWAP_DISPCLASS_TO_BUFFER sIn;
	PVRSRV_BRIDGE_RETURN sOut;
	IMG_UINT32 i;

	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));
	
	if(!hDevice || !hBuffer)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSwapToDCBuffer: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;	
	}

	if(ui32ClipRectCount)
	{
		if(!psClipRect
		   || (ui32ClipRectCount > PVRSRV_MAX_DC_CLIP_RECTS))
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVSwapToDCBuffer: Invalid rect count (%d)", ui32ClipRectCount));
			return PVRSRV_ERROR_INVALID_PARAMS;
		}
	}

	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO*)hDevice;

	sIn.hDeviceKM = psDevClassInfo->hDeviceKM;
	sIn.hBuffer = hBuffer;
	sIn.ui32SwapInterval = ui32SwapInterval;
	sIn.hPrivateTag = hPrivateTag;
	sIn.ui32ClipRectCount = ui32ClipRectCount;

	for(i=0; i<ui32ClipRectCount; i++)
	{
		sIn.sClipRect[i] = psClipRect[i];
	}

	if(PVRSRVBridgeCall(psDevClassInfo->hServices,
						PVRSRV_BRIDGE_SWAP_DISPCLASS_TO_BUFFER,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_SWAP_DISPCLASS_TO_BUFFER),
						&sOut,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSwapToDCBuffer: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sOut.eError;
}

/*!
 ******************************************************************************

 @Function	PVRSRVSwapToDCBuffer2

 @Description		Swap display to a display class buffer

 @Input hDevice		- device handle
 @Input hBuffer		- buffer handle
 @Input ui32SwapInterval - number of Vsync intervals between swaps
 @Input ppsMemInfos - list of meminfos to synchronize with
 @Input ui32NumMemInfos - number of meminfos
 @Input pvPrivData  - opaque private data passed through to 3PDD
 @Input ui32PrivDataLength - length of opaque private data

 @Return
 	success: PVRSRV_OK
 	failure: PVRSRV_ERROR_*

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVSwapToDCBuffer2(IMG_HANDLE	hDevice,
#if defined (SUPPORT_SID_INTERFACE)
												IMG_SID		hSwapChain,
#else
												IMG_HANDLE	hSwapChain,
#endif
												IMG_UINT32	ui32SwapInterval,
												PVRSRV_CLIENT_MEM_INFO **ppsMemInfos,
												IMG_UINT32	ui32NumMemInfos,
												IMG_PVOID	pvPrivData,
												IMG_UINT32	ui32PrivDataLength)
{
	PVRSRV_BRIDGE_IN_SWAP_DISPCLASS_TO_BUFFER2 sIn;
	PVRSRV_BRIDGE_RETURN sOut;
	IMG_UINT32 i;

	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo;

	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));
	if(!hDevice || !hSwapChain || !ppsMemInfos || ui32NumMemInfos < 1)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSwapToDCBuffer: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO*)hDevice;

	sIn.hDeviceKM = psDevClassInfo->hDeviceKM;
	sIn.hSwapChain = hSwapChain;
	sIn.ui32SwapInterval = ui32SwapInterval;
	sIn.ui32NumMemInfos = ui32NumMemInfos;

	sIn.ppsKernelMemInfos = PVRSRVAllocUserModeMem(
#if defined (SUPPORT_SID_INTERFACE)
		sizeof(IMG_SID)
#else
		sizeof(IMG_HANDLE)
#endif
		* ui32NumMemInfos);
	sIn.ppsKernelSyncInfos = PVRSRVAllocUserModeMem(
#if defined (SUPPORT_SID_INTERFACE)
		sizeof(IMG_SID)
#else
		sizeof(IMG_HANDLE)
#endif
		* ui32NumMemInfos);

	for(i = 0; i < ui32NumMemInfos; i++)
	{
		sIn.ppsKernelMemInfos[i] = ppsMemInfos[i]->hKernelMemInfo;
		sIn.ppsKernelSyncInfos[i] = ppsMemInfos[i]->psClientSyncInfo->hKernelSyncInfo;
	}

	sIn.ui32PrivDataLength = ui32PrivDataLength;
	sIn.pvPrivData = pvPrivData;

	if(PVRSRVBridgeCall(psDevClassInfo->hServices,
						PVRSRV_BRIDGE_SWAP_DISPCLASS_TO_BUFFER2,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_SWAP_DISPCLASS_TO_BUFFER2),
						&sOut,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSwapToDCBuffer2: BridgeCall failed"));
		sOut.eError = PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	PVRSRVFreeUserModeMem(sIn.ppsKernelMemInfos);
	PVRSRVFreeUserModeMem(sIn.ppsKernelSyncInfos);

	return sOut.eError;
}

#endif /* OS_PVRSRV_SWAP_TO_DC_BUFFER_UM */


#if !defined(OS_PVRSRV_SWAP_TO_DC_SYSTEM_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVSwapToDCSystem

 @Description

 Swap display to the display's system buffer

 @Input hDevice		- device handle
 @Input hSwapChain	- swapchain handle

 @Return 
 	success: PVRSRV_OK
 	failure: PVRSRV_ERROR_*

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVSwapToDCSystem(IMG_HANDLE hDevice,
#if defined (SUPPORT_SID_INTERFACE)
											   IMG_SID    hSwapChain)
#else
											   IMG_HANDLE hSwapChain)
#endif
{
	PVRSRV_BRIDGE_IN_SWAP_DISPCLASS_TO_SYSTEM sIn;
	PVRSRV_BRIDGE_RETURN sOut;	

	PVRSRV_CLIENT_DEVICECLASS_INFO *psDevClassInfo;


	PVRSRVMemSet(&sIn,0x00,sizeof(sIn));
	PVRSRVMemSet(&sOut,0x00,sizeof(sOut));
	

	if(!hDevice || !hSwapChain)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSwapToDCSystem: Invalid params"));
		return PVRSRV_ERROR_INVALID_PARAMS;	
	}

	psDevClassInfo = (PVRSRV_CLIENT_DEVICECLASS_INFO*)hDevice;

	sIn.hDeviceKM = psDevClassInfo->hDeviceKM;
	sIn.hSwapChain = hSwapChain;

	if(PVRSRVBridgeCall(psDevClassInfo->hServices,
						PVRSRV_BRIDGE_SWAP_DISPCLASS_TO_SYSTEM,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_SWAP_DISPCLASS_TO_SYSTEM),
						&sOut,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSwapToDCSystem: BridgeCall failed"));
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sOut.eError;
}
#endif /* OS_PVRSRV_SWAP_TO_DC_SYSTEM_UM */

/******************************************************************************
 End of file (bridged_pvr_dc_glue.c)
******************************************************************************/
