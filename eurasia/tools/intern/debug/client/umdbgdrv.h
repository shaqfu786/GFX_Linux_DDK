/*****************************************************************************
 Name     		: UMDBGDRV.H

 Title    		: User mode debug driver interface stuff..

 Author 		: John Howson

 Created  		: 
		  		 
 Copyright		: 1998-2006 by Imagination Technologies Limited. All rights reserved.
                  No part of this software, either material or conceptual 
                  may be copied or distributed, transmitted, transcribed,
                  stored in a retrieval system or translated into any 
                  human or computer language in any form by any means,
                  electronic, mechanical, manual or other-wise, or 
                  disclosed to third parties without the express written
                  permission of Imagination Technologies Limited, Unit 8, HomePark
                  Industrial Estate, King's Langley, Hertfordshire,
                  WD4 8LZ, U.K.

 Description	: Header fileis used with both user and kernel mode .c's

 Program Type	: 
	    
 Version	 	: $Revision: 1.14 $

 Modifications	: 

 $Log: umdbgdrv.h $

*****************************************************************************/
#ifndef _UMDBGDRV_
#define _UMDBGDRV_

#if defined(__cplusplus)
extern "C" {
#endif

/*****************************************************************************
 Error values
*****************************************************************************/
#define DBGDRV_OK						0
#define DBGDRVERR_CANTCONNECT			1

/*****************************************************************************
 FN Prototypes
*****************************************************************************/
IMG_UINT32 DBGDRVConnect(IMG_UINTPTR_T *puiHandle);
IMG_VOID DBGDRVDisconnect(IMG_UINTPTR_T uiHandle);
IMG_SID DBGDRVCreateStream(IMG_UINTPTR_T uiHandle,IMG_CHAR * pszName,IMG_UINT32 ui32OutMode,IMG_UINT32 ui32CapMode,IMG_UINT32 ui32Pages);
IMG_VOID DBGDRVDestroyStream(IMG_UINTPTR_T uiHandle,IMG_SID hStream);
IMG_SID DBGDRVFindStream(IMG_UINTPTR_T uiHandle,IMG_CHAR * pszName, IMG_BOOL bResetStream);
IMG_VOID DBGDRVSetCaptureMode(IMG_UINTPTR_T uiHandle, IMG_SID hStream,IMG_UINT32 ui32Mode,IMG_UINT32 ui32Start,IMG_UINT32 ui32End, IMG_UINT32 ui32SampleRate);
IMG_VOID DBGDRVSetOutputMode(IMG_UINTPTR_T uiHandle,IMG_SID hStream,IMG_UINT32 ui32OutMode);
IMG_VOID DBGDRVSetDebugLevel(IMG_UINTPTR_T uiHandle,IMG_SID hStream,IMG_UINT32 ui32DebugLevel);
IMG_VOID DBGDRVSetStreamFrame(IMG_UINTPTR_T uiHandle,IMG_SID hStream,IMG_UINT32 ui32Frame);
IMG_UINT32 DBGDRVGetStreamFrame(IMG_UINTPTR_T uiHandle,IMG_SID hStream);
IMG_UINT32 DBGDRVReadBIN(IMG_UINTPTR_T uiHandle,IMG_SID hStream,IMG_BOOL bReadInitBuffer, IMG_UINT8 *pui8Out,IMG_UINT32 ui32OutSize);
IMG_UINT32 DBGDRVReadString(IMG_UINTPTR_T uiHandle,IMG_SID hStream,IMG_CHAR * pszString,IMG_UINT32 ui32OutSize);
IMG_UINT32 DBGDRVWriteBIN(IMG_UINTPTR_T uiHandle,IMG_SID hStream,IMG_UINT8 *pui8In,IMG_UINT32 ui32Size,IMG_UINT32 ui32Level);
IMG_UINT32 DBGDRVWriteString(IMG_UINTPTR_T uiHandle,IMG_SID hStream,IMG_CHAR * pszString,IMG_UINT32 ui32Level);
IMG_VOID DBGDRVOverrideMode(IMG_UINTPTR_T uiHandle,IMG_SID hStream,IMG_UINT32 ui32Mode);
IMG_VOID DBGDRVDefaultMode(IMG_UINTPTR_T uiHandle,IMG_SID hStream);
IMG_VOID DBGDRVSetStreamMarker(IMG_UINTPTR_T uiHandle,IMG_SID hStream,IMG_UINT32 ui32Frame);
IMG_UINT32 DBGDRVGetStreamMarker(IMG_UINTPTR_T uiHandle,IMG_SID hStream);
IMG_UINT32 DBGDRVReadLastFrame(IMG_UINTPTR_T uiHandle,IMG_SID hStream,IMG_UINT8 *pui8Out,IMG_UINT32 ui32OutSize);

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
typedef enum _DBGDRV_EVENT_ {
	DBGDRV_EVENT_STREAM_DATA = DBG_EVENT_STREAM_DATA
} DBGDRV_EVENT;

IMG_VOID DBGDRVWaitForEvent(IMG_UINTPTR_T uiHandle, DBGDRV_EVENT eEvent);
#endif	/* defined(SUPPORT_DBGDRV_EVENT_OBJECTS) */

#if defined(__cplusplus)
}
#endif

#endif
/*****************************************************************************
 End of file (UMDBGDRV.H)
*****************************************************************************/
