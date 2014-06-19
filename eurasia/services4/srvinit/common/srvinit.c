/*!****************************************************************************
@File           srvinit.c

@Title          Services initialisation routines

@Author         Imagination Technologies

@Date           17/10/2007

@Copyright      Copyright 2005-2008 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       Generic

@Description    Device specific functions

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-
$Log: srvinit.c $
******************************************************************************/

#include "img_defs.h"
#include "pvr_debug.h"
#include "srvinit.h"
#include "services.h"
#include "servicesinit_um.h"


PVRSRV_ERROR SrvInit(IMG_VOID)
{
	PVRSRV_CONNECTION *psServices;
	PVRSRV_ERROR eError;
	PVRSRV_ERROR eRet;
	IMG_UINT32 ui32NumDevices;
	PVRSRV_DEVICE_IDENTIFIER asDevID[PVRSRV_MAX_DEVICES];
	IMG_UINT32 i;
	IMG_BOOL bInitSuccesful;

	bInitSuccesful = IMG_FALSE;

	/* Connect to services as the initialisation server */
	eError = PVRSRVInitSrvConnect(&psServices);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SrvInit: PVRSRVInitSrvConnect failed (%d)", eError));
		return eError;
	}

	/* Enumerate devices */
	eError = PVRSRVEnumerateDevices(psServices,
			&ui32NumDevices,
			asDevID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SrvInit: PVRSRVEnumerateDevices failed (%d)", eError));
		goto cleanup;
	}

	/* Initialise devices */
	for (i=0; i < ui32NumDevices; i++)
	{
		eError = PVRSRV_OK;
		switch (asDevID[i].eDeviceClass)
		{
			case PVRSRV_DEVICE_CLASS_3D:
			{
				switch (asDevID[i].eDeviceType)
				{
#if defined(SUPPORT_SGX)
					case PVRSRV_DEVICE_TYPE_SGX:
					{
						eError = SGXInit(psServices, &asDevID[i]);
						break;
					}
#endif
#if defined(SUPPORT_VGX)
					case PVRSRV_DEVICE_TYPE_VGX:
					{
						eError = VGXInit(psServices, &asDevID[i]);
						break;
					}
#endif

					default:
					{
						PVR_DPF((PVR_DBG_ERROR, "SrvInit: Device type %d not supported", asDevID[i].eDeviceType));
						break;
					}
				}
				break;
			}
			case PVRSRV_DEVICE_CLASS_VIDEO:
			{
#if defined(SUPPORT_MSVDX)
				if (asDevID[i].eDeviceType == PVRSRV_DEVICE_TYPE_MSVDX)
				{
					eError = MSVDXInit(psServices, &asDevID[i]);
				}
				else
#endif
				{
					PVR_DPF((PVR_DBG_ERROR, "SrvInit: Device type %d not supported", asDevID[i].eDeviceType));
				}

				break;
			}

			case PVRSRV_DEVICE_CLASS_DISPLAY:
			/* FALLTHROUGH */
			case PVRSRV_DEVICE_CLASS_BUFFER:
			{
				/*
				 * Srvinit doesn't currently do anything with
				 * devices of this class.
				 */
				break;
			}
			default:
			{
				PVR_DPF((PVR_DBG_ERROR, "SrvInit: Device class %d not supported", asDevID[i].eDeviceClass));
				break;
			}
		}
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
					"SrvInit: Initialisation for device of class %d, type %d, index %d, failed (%d)",
					asDevID[i].eDeviceClass, asDevID[i].eDeviceType, asDevID[i].ui32DeviceIndex, eError));
			goto cleanup;
		}
	}

	bInitSuccesful = IMG_TRUE;
	eError = PVRSRV_OK;

cleanup:
	/* PVRSRVInitSrvConnect must have been successful */
	eRet = eError;

	eError = PVRSRVInitSrvDisconnect(psServices, bInitSuccesful);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SrvInit: PVRSRVInitSrvDisconnect failed (%d). See srvkm log for details.", eError));
		eRet = eError;
	}

	return eRet;
}

/******************************************************************************
 End of file (srvinit.c)
*****************************************************************************/
