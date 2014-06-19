/*!****************************************************************************
@File           pvr_bridge_u.h

@Title          PVR Bridge Functionality

@Author         Imagination Technologies

@Date           10/9/2003

@Copyright      Copyright 2003-2007 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or other-wise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       Generic

@Description    Header for the PVR Bridge code

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-
$Log: pvr_bridge_client.h $
******************************************************************************/

#ifndef __PVR_BRIDGE_U_H__
#define __PVR_BRIDGE_U_H__

#if defined (__cplusplus)
extern "C" {
#endif


/******************************************************************************
 * Function prototypes 
 *****************************************************************************/

PVRSRV_ERROR OpenServices(IMG_HANDLE *phServices, IMG_UINT32 ui32SrvFlags);
PVRSRV_ERROR CloseServices(IMG_HANDLE hServices);
IMG_RESULT PVRSRVBridgeCall(IMG_HANDLE hServices,
							IMG_UINT32 ui32FunctionID,
							IMG_VOID *pvParamIn,
							IMG_UINT32 ui32InBufferSize,
							IMG_VOID *pvParamOut,
							IMG_UINT32	ui32OutBufferSize);
PVRSRV_ERROR GetClassDeviceInfo(IMG_HANDLE			hServices,
									PVRSRV_DEVICE_CLASS	DeviceClass,
									IMG_UINT32			ui32DeviceID, 
									IMG_VOID			**ppvDevInfo);
PVRSRV_ERROR FreeClassDeviceInfo(IMG_HANDLE			hServices,
									PVRSRV_DEVICE_CLASS	DeviceClass,
									IMG_VOID			*pvDevInfo);

#if defined (__cplusplus)
}
#endif
#endif /* __PVR_BRIDGE_U_H__ */

/******************************************************************************
 End of file (pvr_bridge_u.h)
******************************************************************************/

