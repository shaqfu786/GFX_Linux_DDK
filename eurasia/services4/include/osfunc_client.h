/*!****************************************************************************
@File			osfunc_client.h

@Title			environment API Header

@Author			Imagination Technologies

@date   		17 / 9 / 03
 
@Copyright     	Copyright 2003-2004 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either
                material or conceptual may be copied or distributed,
                transmitted, transcribed, stored in a retrieval system
                or translated into any human or computer language in any
                form by any means, electronic, mechanical, manual or
                other-wise, or disclosed to third parties without the
                express written permission of Imagination Technologies
                Limited, Unit 8, HomePark Industrial Estate,
                King's Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform		generic

@Description	Functions which are OS specific

@DoxygenVer		

******************************************************************************/

/******************************************************************************
Modifications :-

$Log: osfunc_client.h $
*****************************************************************************/

#if !defined(__OSFUNC_UM_H__)
#define __OSFUNC_UM_H__

#include "img_defs.h"

#ifdef UITRON
#include "inlines.h"
#endif

#if defined (__cplusplus)
extern "C" {
#endif

#ifndef OSReadHWReg
IMG_UINT32 OSReadHWReg(IMG_PVOID pvLinRegBaseAddr, IMG_UINT32 ui32Offset);
#endif

#ifndef OSWriteHWReg
IMG_VOID OSWriteHWReg(IMG_PVOID pvLinRegBaseAddr, IMG_UINT32 ui32Offset,
						IMG_UINT32 ui32Value);
#endif

#if defined (SUPPORT_SID_INTERFACE)
PVRSRV_ERROR OSEventObjectOpen(IMG_CONST PVRSRV_CONNECTION *psConnection, PVRSRV_EVENTOBJECT *psEventObject, IMG_EVENTSID *phOSEvent);
PVRSRV_ERROR OSEventObjectClose(IMG_CONST PVRSRV_CONNECTION *psConnection, PVRSRV_EVENTOBJECT *psEventObject, IMG_EVENTSID hOSEvent);
PVRSRV_ERROR IMG_CALLCONV OSEventObjectWait(const PVRSRV_CONNECTION *psConnection, IMG_EVENTSID hOSEvent);
#else
PVRSRV_ERROR OSEventObjectOpen(IMG_CONST PVRSRV_CONNECTION *psConnection, PVRSRV_EVENTOBJECT *psEventObject, IMG_HANDLE *phOSEvent);
PVRSRV_ERROR OSEventObjectClose(IMG_CONST PVRSRV_CONNECTION *psConnection, PVRSRV_EVENTOBJECT *psEventObject, IMG_HANDLE hOSEvent);
PVRSRV_ERROR IMG_CALLCONV OSEventObjectWait(const PVRSRV_CONNECTION *psConnection, IMG_HANDLE hOSEvent);
#endif

IMG_VOID OSTermClient(IMG_CHAR* pszMsg, IMG_UINT32 ui32ErrCode);

IMG_UINTPTR_T OSGetCurrentThreadID(IMG_VOID);
IMG_VOID   OSInterlockedExchangeAdd(IMG_UINT32 *pAddend, IMG_UINT32 uiValue);

IMG_BOOL OSIsProcessPrivileged(IMG_VOID);

PVRSRV_ERROR OSFlushCPUCacheRange(IMG_VOID *pvRangeAddrStart,
						 IMG_VOID *pvRangeAddrEnd);


/*--------------------------------- end of file ------------------------------*/

#if defined (__cplusplus)
}
#endif

#endif /* __OSFUNC_UM_H__ */
