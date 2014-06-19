/*!****************************************************************************
@File           bridged_sgx_glue.h

@Title          Shared SGX glue code

@Author         Imagination Technologies

@Copyright      Copyright 2009 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Description    Implements shared SGX API user bridge code

Modifications :-
$Log: bridged_sgx_glue.h $

******************************************************************************/

#include "img_defs.h"
#include "services.h"

#ifndef __BRIDGED_SGX_GLUE_H__
#define __BRIDGED_SGX_GLUE_H__

PVRSRV_ERROR IMG_CALLCONV SGXConnectionCheck(PVRSRV_DEV_DATA *psDevData);
PVRSRV_ERROR IMG_CALLCONV SGXDumpMKTrace(PVRSRV_DEV_DATA *psDevData);

#endif /* __BRIDGED_SGX_GLUE_H__ */
