/*!****************************************************************************
@File           srvinit.h

@Title          Initialisation server internal header

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

@Description    Defines the connections between the various parts of the
		initialisation server.

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-

$Log: srvinit.h $
*****************************************************************************/

#ifndef __SRVINIT_H__
#define __SRVINIT_H__

#if defined (__cplusplus)
extern "C" {
#endif

#include "img_defs.h"
#include "services.h"

PVRSRV_ERROR SrvInit(IMG_VOID);

struct board_properties {
	int chip_family;
	int chip_rev_maj;
	int chip_rev_min;
	int sgx_core;
	int sgx_rev;
	int num_bufs;
};

IMG_BOOL LoadModules(int sgx_core, int sgx_rev);
IMG_BOOL UnloadModules(int sgx_core, int sgx_rev);
IMG_BOOL DetectPlatform(struct board_properties *props);

#if defined(SUPPORT_SGX)
PVRSRV_ERROR SGXInit(PVRSRV_CONNECTION *psServices, PVRSRV_DEVICE_IDENTIFIER *psDevID);
#endif
#if defined(SUPPORT_VGX)
PVRSRV_ERROR VGXInit(PVRSRV_CONNECTION *psServices, PVRSRV_DEVICE_IDENTIFIER *psDevID);
#endif
#if defined(SUPPORT_MSVDX)
PVRSRV_ERROR MSVDXInit(PVRSRV_CONNECTION *psServices, PVRSRV_DEVICE_IDENTIFIER *psDevID);
#endif
#if defined (__cplusplus)
}
#endif
#endif /* __SRVINIT_H__ */

/******************************************************************************
 End of file (srvinit.h)
******************************************************************************/
