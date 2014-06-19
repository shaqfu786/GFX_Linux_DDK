/*!****************************************************************************
@File           servicesint_u.h

@Title          Services internal API Header

@Author         Imagination Technologies

@Date           01/08/2003

@Copyright      Copyright 2007 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       Cross platform / environment

@Description    Services API used between different parts of services

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-

$Log: servicesinit_um.h $
*****************************************************************************/

#ifndef __SERVICESINT_U_H__
#define __SERVICESINT_U_H__

#if defined (__cplusplus)
extern "C" {
#endif

#include "img_defs.h"

IMG_IMPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVUnrefDeviceMem(
	const PVRSRV_DEV_DATA *psDevData, PVRSRV_CLIENT_MEM_INFO *psMemInfo);

IMG_IMPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVInitSrvConnect(PVRSRV_CONNECTION **ppsConnection);

IMG_IMPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVInitSrvDisconnect(PVRSRV_CONNECTION *psConnection, IMG_BOOL bInitSuccesful);

#if defined (__cplusplus)
}
#endif
#endif /* __SERVICESINT_U_H__ */

/******************************************************************************
 End of file (servicesint_u.h)
******************************************************************************/
