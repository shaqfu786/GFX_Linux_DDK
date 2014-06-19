/*****************************************************************************
 * Name			: umdbgdrvlnx.c
 * 
 * Title			: User mode debug driver interface functions
 * 
 * C Author 		: John Howson / Vlad Stamate
 * 
 * Created  		: 09/05/2002
 *
 * Copyright    : 2001 by Imagination Technologies Ltd. All rights reserved.
 *              : No part of this software, either material or conceptual 
 *              : may be copied or distributed, transmitted, transcribed,
 *              : stored in a retrieval system or translated into any 
 *              : human or computer language in any form by any means,
 *              : electronic, mechanical, manual or other-wise, or 
 *              : disclosed to third parties without the express written
 *              : permission of Imagination Technologies Ltd, Unit 8, HomePark
 *              : Industrial Estate, King's Langley, Hertfordshire,
 *              : WD4 8LZ, U.K.
 *
 *
 * Description 	: 
 *
 * Modifications:
 * $Log: umdbgdrvlnx.c $
 *
 ******************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#if defined(SUPPORT_DRI_DRM)
#if !defined(SUPPORT_DRI_DRM_NO_LIBDRM)
#include <xf86drm.h>
#else
#include "drmlite.h"
#endif
#if defined(SUPPORT_DRI_DRM_EXT)
#include "sys_pvr_drm_import.h"
#endif
#include "pvr_drm_shared.h"
#include "pvr_drm.h"
#endif	/* defined(SUPPORT_DRI_DRM) */

#include "img_types.h"

#include "dbgdrvif.h"
#include "linuxsrv.h"
#include "umdbgdrv.h"

IMG_UINT32 DeviceIoControl(IMG_UINT32 hDevice,		/* handle to device */
						IMG_UINT32 ui32ControlCode, /* operation */
						IMG_VOID *pInBuffer,		/* input data buffer */
						IMG_UINT32 ui32InBufferSize,   /* size of input data buffer */
						IMG_VOID *pOutBuffer,		/* output data buffer */
						IMG_UINT32 ui32OutBufferSize,  /* size of output data buffer */
						IMG_UINT32 *pui32BytesReturned) 
{
    IMG_UINT32 ui32Ret;
    IOCTL_PACKAGE sIoctlPackage;
    
    sIoctlPackage.ui32Cmd = ui32ControlCode;
    sIoctlPackage.ui32Size = sizeof (sIoctlPackage);
    sIoctlPackage.pInBuffer = pInBuffer;
    sIoctlPackage.ui32InBufferSize = ui32InBufferSize;
    sIoctlPackage.pOutBuffer = pOutBuffer;
    sIoctlPackage.ui32OutBufferSize = ui32OutBufferSize;
    
#if defined(SUPPORT_DRI_DRM)
    ui32Ret = drmCommandWrite((int)hDevice, PVR_DRM_DBGDRV_CMD, &sIoctlPackage, sizeof(sIoctlPackage));
#else
    ui32Ret = ioctl((int)hDevice, ui32ControlCode, &sIoctlPackage);
#endif
    
    if (pui32BytesReturned)
    {
		if (!ui32Ret)
			*pui32BytesReturned = ui32OutBufferSize;
		else
			*pui32BytesReturned = 0;
    }
    
    if (ui32Ret)
    {
		printf("DeviceIoControl failed (control code %u)\n", ui32ControlCode);
    }

    return ui32Ret;
}

/*****************************************************************************
 FUNCTION	: DBGDRVConnect
    
 PURPOSE	: Connects to debug driver.

 PARAMETERS	: pui32Handle	- System dependant cookie to get at dbgdriv
			  
 RETURNS	: DBGDRIV_OK or something nasty
*****************************************************************************/
IMG_UINT32 DBGDRVConnect(IMG_UINTPTR_T *puiHandle)
{
	int fd;

#if defined(SUPPORT_DRI_DRM)
    fd = PVRDRMOpen();
    if (fd == -1)
    {
	printf("DBGDRVConnect: drmOpen failed");
	return -1;
    }

    /*
     * We don't want to be the master.  There can be only one, and
     * it should be the X server (or equivalent).
     */
    if (drmCommandNone(fd, PVR_DRM_IS_MASTER_CMD) == 0)
    {
	(IMG_VOID) drmDropMaster(fd);
    }
#else
    fd = open("/dev/dbgdrv",O_RDWR,0);
    if (fd == -1)
    {
	return -1;
    }

#endif
    *puiHandle = (IMG_UINTPTR_T)fd;

    return DBGDRV_OK;

}

/*****************************************************************************
 FUNCTION	: DBGDRVDisconnect
    
 PURPOSE	: Disconnects from debug driver.

 PARAMETERS	: ui32Handle	- System dependant cookie to get at dbgdriv
			  
 RETURNS	: 
*****************************************************************************/
IMG_VOID DBGDRVDisconnect(IMG_UINTPTR_T uiHandle)
{
    int fd = (int)uiHandle;

#if defined(SUPPORT_DRI_DRM)
    (IMG_VOID) drmClose(fd);
#else
    close(fd);
#endif
}

/*****************************************************************************
 FUNCTION	: DBGDRVCreateStream
    
 PURPOSE	: 

 PARAMETERS	: ui32Handle	- System dependant cookie to get at dbgdriv
			  pszName	- Name of buffer to create.
			  ui32Mode	- Hmm...
			  ui32Pages	- Number of memory pages to allocate for buffer
			  
 RETURNS	: A buffer.
*****************************************************************************/
IMG_SID DBGDRVCreateStream(IMG_UINTPTR_T uiHandle, IMG_CHAR *pszName,
						   IMG_UINT32 ui32OutMode, IMG_UINT32 ui32CapMode,
						   IMG_UINT32 ui32Pages)
{
    DBG_IN_CREATESTREAM	sCreateIn;
    IMG_VOID *pvStream;
    IMG_UINT32 ui32BytesReturned;
	
    sCreateIn.ui32Pages = ui32Pages;
    sCreateIn.ui32CapMode = ui32CapMode;
    sCreateIn.ui32OutMode = ui32OutMode;
    sCreateIn.u.pszName = pszName;

    return DeviceIoControl( uiHandle, 
		    DEBUG_SERVICE_CREATESTREAM, 
		    &sCreateIn, 
		    sizeof (DBG_IN_CREATESTREAM), 
		    &pvStream, 
		    sizeof (IMG_UINT32), 
		    &ui32BytesReturned) ? 0 : (IMG_SID)pvStream;
}

/*****************************************************************************
 FUNCTION	: DBGDRVDestroyStream
    
 PURPOSE	: Hmm, err...

 PARAMETERS	: ui32Handle	- System dependant cookie to get at dbgdriv
			  pvStream	- Stream to destroy.
			  
 RETURNS	: Nowt
*****************************************************************************/
IMG_VOID DBGDRVDestroyStream(IMG_UINTPTR_T uiHandle, IMG_SID hStream)
{
	IMG_VOID *pvStream = (IMG_VOID *)hStream;
    IMG_UINT32 ui32BytesReturned;

    DeviceIoControl(uiHandle, 
		    DEBUG_SERVICE_DESTROYSTREAM, 
		    &pvStream,
		    sizeof (IMG_UINT32), 
		    0, 
		    0, 
		    &ui32BytesReturned);
}

/*****************************************************************************
 FUNCTION	: DBGDRVFindStream
    
 PURPOSE	: Finds named debug buffer

 PARAMETERS	: ui32Handle	- System dependant cookie to get at dbgdriv
			  pszName	- Name of buffer to find
			  
 RETURNS	: Pointer to Stream.
*****************************************************************************/
IMG_SID DBGDRVFindStream(IMG_UINTPTR_T uiHandle, IMG_CHAR *pszName, IMG_BOOL bResetStream)
{
    IMG_VOID *pvStream;
    IMG_UINT32 ui32BytesReturned;
    DBG_IN_FINDSTREAM	sFindIn;
	
	sFindIn.u.pszName = pszName;
	sFindIn.bResetStream = bResetStream;

    return DeviceIoControl(uiHandle, 
		    DEBUG_SERVICE_GETSTREAM, 
		    &sFindIn, 
		    sizeof(DBG_IN_FINDSTREAM), 
		    &pvStream, 
		    sizeof (IMG_UINT32), 
		    &ui32BytesReturned) ? 0 : (IMG_SID)pvStream;
}

/*****************************************************************************
 FUNCTION	: DBGDRVSetCaptureMode
    
 PURPOSE	: Set buffer capture mode.

 PARAMETERS	: ui32Handle	- System dependant cookie to get at dbgdriv
			  pvStream	- .
			  ui32CapMode	- Hey look its a mode...
			  ui32Start	- Start frame for frame mode
			  ui32End		- Hmm, given what ui32Start is...
			  
 RETURNS        : None
*****************************************************************************/
IMG_VOID DBGDRVSetCaptureMode(IMG_UINTPTR_T uiHandle, IMG_SID hStream,
							  IMG_UINT32 ui32Mode, IMG_UINT32 ui32Start,
							  IMG_UINT32 ui32End, IMG_UINT32 ui32SampleRate)
{
    DBG_IN_SETDEBUGMODE	sSetModeIn;
    IMG_UINT32 ui32BytesReturned;

    sSetModeIn.hStream = hStream;
    sSetModeIn.ui32Mode = ui32Mode;
    sSetModeIn.ui32Start = ui32Start;
    sSetModeIn.ui32End = ui32End;
	sSetModeIn.ui32SampleRate = ui32SampleRate;

    DeviceIoControl(uiHandle, 
		    DEBUG_SERVICE_SETDEBUGMODE, 
		    &sSetModeIn, 
		    sizeof (DBG_IN_SETDEBUGMODE), 
		    IMG_NULL, 
		    0, 
		    &ui32BytesReturned);
}

/*****************************************************************************
 FUNCTION	: DBGDRVSetOutputMode
    
 PURPOSE	: Sets buffer output mode

 PARAMETERS	: ui32Handle		- System dependant cookie to get at dbgdriv
			  pvStream		- Stream to destroy.
			  ui32OutMode		- err....
			  
 RETURNS        : None
*****************************************************************************/
IMG_VOID DBGDRVSetOutputMode(IMG_UINTPTR_T uiHandle, IMG_SID hStream, IMG_UINT32 ui32OutMode)
{
    DBG_IN_SETDEBUGOUTMODE	sOutModeIn;
    IMG_UINT32 ui32BytesReturned;

    sOutModeIn.hStream = hStream;
    sOutModeIn.ui32Mode = ui32OutMode;

    DeviceIoControl(uiHandle, 
		    DEBUG_SERVICE_SETDEBUGOUTMODE, 
		    &sOutModeIn, 
		    sizeof (DBG_IN_SETDEBUGOUTMODE), 
		    IMG_NULL, 
		    0, 
		    &ui32BytesReturned);
}

/*****************************************************************************
 FUNCTION	: DBGDRVSetDebugLevel
    
 PURPOSE	: Sets buffer debug level

 PARAMETERS	: ui32Handle		- System dependant cookie to get at dbgdriv
			  pvStream		- Stream to destroy.
			  ui32DebugLevel	- err....
			  
 RETURNS        : None
*****************************************************************************/
IMG_VOID DBGDRVSetDebugLevel(IMG_UINTPTR_T uiHandle, IMG_SID hStream, IMG_UINT32 ui32DebugLevel)
{
    DBG_IN_SETDEBUGLEVEL	sSetDebugLevel;
    IMG_UINT32 ui32BytesReturned;

    sSetDebugLevel.hStream = hStream;
    sSetDebugLevel.ui32Level = ui32DebugLevel;

    DeviceIoControl(uiHandle, 
		    DEBUG_SERVICE_SETDEBUGLEVEL, 
		    &sSetDebugLevel, 
		    sizeof (DBG_IN_SETDEBUGLEVEL), 
		    IMG_NULL, 
		    0, 
		    &ui32BytesReturned);
}

/*****************************************************************************
 FUNCTION	: DBGDRVSetStreamFrame
    
 PURPOSE	: Sets current frame number for debug buffer

 PARAMETERS	: ui32Handle	- System dependant cookie to get at dbgdriv
			  pvStream	- Stream !
			  ui32Frame	- Hey look its a frame number...
			  
 RETURNS        : None
*****************************************************************************/
IMG_VOID DBGDRVSetStreamFrame(IMG_UINTPTR_T uiHandle, IMG_SID hStream, IMG_UINT32 ui32Frame)
{
    DBG_IN_SETFRAME	sSetFrameIn;
    IMG_UINT32 ui32BytesReturned;

    sSetFrameIn.hStream = hStream;
    sSetFrameIn.ui32Frame = ui32Frame;

    DeviceIoControl(uiHandle, 
		    DEBUG_SERVICE_SETFRAME, 
		    &sSetFrameIn, 
		    sizeof (DBG_IN_SETFRAME), 
		    IMG_NULL, 
		    0, 
		    &ui32BytesReturned);
}

/*****************************************************************************
 FUNCTION	: DBGDRVGetStreamFrame
    
 PURPOSE	: Sets current frame number for debug buffer

 PARAMETERS	: ui32Handle	- System dependant cookie to get at dbgdriv
			  pvStream	- Stream !
			  
 RETURNS	: Stream frame
*****************************************************************************/
IMG_UINT32 DBGDRVGetStreamFrame(IMG_UINTPTR_T uiHandle, IMG_SID hStream)
{
	IMG_VOID *pvStream = (IMG_VOID *)hStream;
    IMG_UINT32 ui32BytesReturned;
    IMG_UINT32 ui32Frame = 0;

    return DeviceIoControl(uiHandle, 
		    DEBUG_SERVICE_GETFRAME, 
		    &pvStream, 
		    sizeof(IMG_UINT32), 
		    &ui32Frame,
		    sizeof (IMG_UINT32), 
		    &ui32BytesReturned) ? 0 : ui32Frame;
}

/*****************************************************************************
 FUNCTION	: DBGDRVReadBIN
    
 PURPOSE	: Reads binary data from specified buffer

 PARAMETERS	: ui32Handle	- System dependant cookie to get at dbgdriv
			  pvStream	- Stream !
			  pui8Out	- Pointer to output buffer
			  ui32OutSize	- Size of pui8Out
			  
 RETURNS	: Number of bytes read
*****************************************************************************/
IMG_UINT32 DBGDRVReadBIN(IMG_UINTPTR_T uiHandle, IMG_SID hStream,
						 IMG_BOOL bReadInitBuffer, IMG_UINT8 *pui8Out,
						 IMG_UINT32 ui32OutSize)
{
    DBG_IN_READ sReadIn;
    IMG_UINT32 ui32DataRead;
    IMG_UINT32 ui32BytesReturned;

    sReadIn.hStream = hStream;
    sReadIn.u.pui8OutBuffer = pui8Out;
    sReadIn.ui32OutBufferSize = ui32OutSize;
	sReadIn.bReadInitBuffer = bReadInitBuffer;

    return DeviceIoControl(uiHandle, 
		    DEBUG_SERVICE_READ, 
		    &sReadIn, 
		    sizeof(DBG_IN_READ), 
		    &ui32DataRead,
		    sizeof (IMG_UINT32), 
		    &ui32BytesReturned) ? 0 : ui32DataRead;
}

/*****************************************************************************
 FUNCTION	: DBGDRVReadString
    
 PURPOSE	: Reads string from specified buffer

 PARAMETERS	: ui32Handle	- System dependant cookie to get at dbgdriv
			  pvStream	- Stream !
			  pszString	- Pointer to output buffer
			  ui32OutSize	- Max size of pszString
			  
 RETURNS	: String length, if > ui32OutSize, string not read.
*****************************************************************************/
IMG_UINT32 DBGDRVReadString(IMG_UINTPTR_T uiHandle, IMG_SID hStream,
							IMG_CHAR * pszString, IMG_UINT32 ui32OutSize)
{
    DBG_IN_READSTRING	sParams;
    IMG_UINT32 ui32StrLen;
    IMG_UINT32 ui32BytesReturned;

    sParams.hStream = hStream;
    sParams.ui32StringLen = ui32OutSize;
    sParams.u.pszString = pszString;

    return DeviceIoControl(uiHandle, 
		    DEBUG_SERVICE_READSTRING, 
		    &sParams, 
		    sizeof(DBG_IN_READSTRING), 
		    &ui32StrLen,
		    sizeof (IMG_UINT32), 
		    &ui32BytesReturned) ? 0 : ui32StrLen;
}

/*****************************************************************************
 FUNCTION	: DBGDRVWriteBIN
    
 PURPOSE	: Writes binary data to specified buffer

 PARAMETERS	: ui32Handle	- System dependant cookie to get at dbgdriv
			  pvStream	- Stream !
			  pui8In		- Pointer to input buffer
			  ui32Size	- Amount to write
			  ui32Level	- Debug level flags
			  
 RETURNS	: Number of bytes written
*****************************************************************************/
IMG_UINT32 DBGDRVWriteBIN(IMG_UINTPTR_T uiHandle, IMG_SID hStream,
						  IMG_UINT8 *pui8In, IMG_UINT32 ui32Size,
						  IMG_UINT32 ui32Level)
{
    DBG_IN_WRITE	sWriteIn;
    IMG_UINT32 ui32DataWritten;
    IMG_UINT32 ui32BytesReturned;

    sWriteIn.hStream = hStream;
    sWriteIn.ui32Level = ui32Level;
    sWriteIn.u.pui8InBuffer = pui8In;
    sWriteIn.ui32TransferSize = ui32Size;

    return DeviceIoControl(uiHandle, 
		    DEBUG_SERVICE_WRITE, 
		    &sWriteIn, 
		    sizeof(DBG_IN_WRITE), 
		    &ui32DataWritten,
		    sizeof (IMG_UINT32), 
		    &ui32BytesReturned) ? 0 : ui32DataWritten;
}

/*****************************************************************************
 FUNCTION	: DBGDRVWriteString
    
 PURPOSE	: Writes string to specified buffer

 PARAMETERS	: ui32Handle	- System dependant cookie to get at dbgdriv
			  pvStream	- Stream !
			  pszString	- Pointer string
			  ui32Level	- Debug level flags
			  
 RETURNS	: Number of bytes written
*****************************************************************************/
IMG_UINT32 DBGDRVWriteString(IMG_UINTPTR_T uiHandle, IMG_SID hStream,
							 IMG_CHAR *pszString, IMG_UINT32 ui32Level)
{
    DBG_IN_WRITESTRING	sWriteIn;
    IMG_UINT32 ui32DataWritten;
    IMG_UINT32 ui32BytesReturned;

    sWriteIn.hStream = hStream;
    sWriteIn.ui32Level = ui32Level;
    sWriteIn.u.pszString = pszString;

    return DeviceIoControl(uiHandle, 
		    DEBUG_SERVICE_WRITESTRING, 
		    &sWriteIn, 
		    sizeof(DBG_IN_WRITESTRING), 
		    &ui32DataWritten,
		    sizeof (IMG_UINT32), 
		    &ui32BytesReturned) ? 0 : ui32DataWritten;
}

/*****************************************************************************
 FUNCTION	: DBGDRVSetStreamMarker

 PURPOSE	: Sets current marker for debug buffer to split output files

 PARAMETERS	: ui32Handle	- System dependant cookie to get at dbgdriv
			  pvStream	- Stream !
			  ui32Marker	- Hey look its a marker...

 RETURNS    : None
*****************************************************************************/
IMG_VOID DBGDRVSetStreamMarker(IMG_UINTPTR_T uiHandle, IMG_SID hStream, IMG_UINT32 ui32Frame)
{
    DBG_IN_SETMARKER	sSetMarkerIn;
    IMG_UINT32		ui32BytesReturned;

    sSetMarkerIn.hStream = hStream;
    sSetMarkerIn.ui32Marker = ui32Frame;

    DeviceIoControl(uiHandle,
		    DEBUG_SERVICE_SETMARKER,
		    &sSetMarkerIn,
		    sizeof(sSetMarkerIn),
		    IMG_NULL,
		    0,
		    &ui32BytesReturned);
}

/*****************************************************************************
 FUNCTION	: DBGDRVGetStreamMarker

 PURPOSE	: Gets current marker for debug buffer to split output files

 PARAMETERS	: ui32Handle	- System dependant cookie to get at dbgdriv
			  pvStream	- Stream !

 RETURNS	: Stream marker
*****************************************************************************/
IMG_UINT32 DBGDRVGetStreamMarker(IMG_UINTPTR_T uiHandle, IMG_SID hStream)
{
	IMG_VOID	*pvStream = (IMG_VOID *)hStream;
    IMG_UINT32	ui32BytesReturned;
    IMG_UINT32	ui32Marker = 0;

    return DeviceIoControl(uiHandle,
		    DEBUG_SERVICE_GETMARKER,
		    &pvStream,
		    sizeof(pvStream),
		    &ui32Marker,
		    sizeof(ui32Marker),
		    &ui32BytesReturned) ? 0 : ui32Marker;
}

/*****************************************************************************
 FUNCTION	: DBGDRVReadLastFrame
    
 PURPOSE	: Reads "last frame" binary data from specified buffer

 PARAMETERS	: ui32Handle	- System dependant cookie to get at dbgdriv
			  pvStream	- Stream !
			  pbyOut	- Pointer to output buffer
			  dwOutSize	- Size of pbyOut
			  
 RETURNS	: Number of bytes read
*****************************************************************************/
IMG_UINT32 DBGDRVReadLastFrame(IMG_UINTPTR_T uiHandle, IMG_SID hStream,
							   IMG_UINT8 *pui8Out, IMG_UINT32 ui32OutSize)
{
	DBG_IN_READ sReadIn;
	IMG_UINT32	ui32DataRead;
	IMG_UINT32	ui32BytesReturned=0;

	sReadIn.hStream = hStream;
	sReadIn.u.pui8OutBuffer = pui8Out;
	sReadIn.ui32OutBufferSize = ui32OutSize;

	return DeviceIoControl( uiHandle, 
					DEBUG_SERVICE_READLF, 
					&sReadIn, 
					sizeof(DBG_IN_READ), 
					&ui32DataRead,
					sizeof (IMG_UINT32), 
					&ui32BytesReturned) ? 0 : ui32DataRead;
}

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
/*****************************************************************************
 FUNCTION	: DBGDRVWaitForEvent
    
 PURPOSE	: Wait for an event to occur

 PARAMETERS	: ui32Handle    - System dependant cookie to get at dbgdriv
			eEvent	- Event
 RETURNS	: None
*****************************************************************************/
IMG_VOID DBGDRVWaitForEvent(IMG_UINT32 ui32Handle, DBGDRV_EVENT eEvent)
{
	IMG_UINT32 ui32BytesReturned;
	IMG_UINT32 ui32DebugEvent = (IMG_UINT32)eEvent;

	DeviceIoControl( ui32Handle, 
					DEBUG_SERVICE_WAITFOREVENT, 
					&ui32DebugEvent, 
					sizeof(ui32DebugEvent), 
					NULL,
					0, 
					&ui32BytesReturned);
}
#endif	/* defined(SUPPORT_DBGDRV_EVENT_OBJECTS) */
/*****************************************************************************
 End of file (UMDBGDRV.C)
*****************************************************************************/
