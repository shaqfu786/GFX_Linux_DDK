/*!****************************************************************************
@File           bridged_pvr_pdump_glue.c

@Title          Shared pvr PDUMP glue code

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

@Description    Implements shared PVRSRV PDUMP API user bridge code

Modifications :-
$Log: bridged_pvr_pdump_glue.c $
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

#define PDUMP_TRANSLATE_FLAGS(ui32In, ui32Out)	\
	ui32Out |= ( (ui32In & PVRSRV_PDUMP_FLAGS_CONTINUOUS) != 0) ? PDUMP_FLAGS_CONTINUOUS : 0;

#if defined (PDUMP)

#if !defined(OS_PVRSRV_PDUMP_INIT_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpInit

 @Description	Pdump initialisation

 @Input		psConnection : connection info

 @Return	PVRSRV_ERROR

 *****************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpInit(const PVRSRV_CONNECTION *psConnection)
{
	PVRSRV_BRIDGE_RETURN sRet;

	if(!psConnection)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_PDUMP_INIT,
						IMG_NULL,
						0,
						&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sRet.eError;
}
#endif /* !defined(OS_PVRSRV_PDUMP_INIT_UM) */


#if !defined(OS_PVRSRV_PDUMP_MEM_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpMem

 @Description	PDumps memory to file via kernel services

 @Input		psConnection : connection info
 @Input		pvAltLinAddr
 @Input		psMemInfo
 @Input		ui32Offset
 @Input		ui32Bytes
 @Input		ui32Flags

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpMem(const PVRSRV_CONNECTION *psConnection,
													IMG_PVOID pvAltLinAddr,
													PVRSRV_CLIENT_MEM_INFO *psMemInfo,
													IMG_UINT32 ui32Offset,
													IMG_UINT32 ui32Bytes,
													IMG_UINT32 ui32Flags)
{
	PVRSRV_BRIDGE_IN_PDUMP_DUMPMEM sIn;
	PVRSRV_BRIDGE_RETURN sRet;

	if(!psConnection || !psMemInfo)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sIn.pvLinAddr = psMemInfo->pvLinAddr;
	sIn.pvAltLinAddr = pvAltLinAddr;
#if defined (SUPPORT_SID_INTERFACE)
	sIn.hKernelMemInfo = psMemInfo->hKernelMemInfo;
#else
	sIn.psKernelMemInfo = psMemInfo->hKernelMemInfo;
#endif
	sIn.ui32Offset = ui32Offset;
	sIn.ui32Bytes = ui32Bytes;

	/* Translate the pdump flags
	 * Note: initialising the internal flag to zero will break
	 * clients which have not been updated.
	 */
	sIn.ui32Flags = ui32Flags;
	PDUMP_TRANSLATE_FLAGS(ui32Flags, sIn.ui32Flags);

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_PDUMP_DUMPMEM,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_PDUMP_DUMPMEM),
						&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sRet.eError != PVRSRV_OK)
	{
		return sRet.eError;
	}

	return sRet.eError;
}
#endif /* !defined(OS_PVRSRV_PDUMP_MEM_UM) */


#if !defined(OS_PVRSRV_PDUMP_MEM_POL_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpMemPol

 @Description	PDumps memory to file via kernel services

 @Input		psConnection : connection info
 @Input		psMemInfo
 @Input		ui32Offset
 @Input		ui32Bytes
 @Input		ui32Flags

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpMemPol(const PVRSRV_CONNECTION *psConnection,
#if defined (SUPPORT_SID_INTERFACE)
												       IMG_SID    hKernelMemInfo,
#else
													   PVRSRV_CLIENT_MEM_INFO *psMemInfo,
#endif
													   IMG_UINT32 ui32Offset,
													   IMG_UINT32 ui32Value,
													   IMG_UINT32 ui32Mask,
													   PDUMP_POLL_OPERATOR eOperator,
													   IMG_UINT32 ui32Flags)
{
	PVRSRV_BRIDGE_IN_PDUMP_MEMPOL	sIn;
	PVRSRV_BRIDGE_RETURN			sRet;

#if defined (SUPPORT_SID_INTERFACE)
	if(!psConnection || !hKernelMemInfo)
#else
	if(!psConnection || !psMemInfo)
#endif
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

#if defined (SUPPORT_SID_INTERFACE)
	sIn.hKernelMemInfo = hKernelMemInfo;
#else
	sIn.psKernelMemInfo = psMemInfo->hKernelMemInfo;
#endif
	sIn.ui32Offset = ui32Offset;
	sIn.ui32Value = ui32Value;
	sIn.ui32Mask = ui32Mask;
	sIn.eOperator = eOperator;
	sIn.ui32Flags = ui32Flags;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_PDUMP_MEMPOL,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_PDUMP_MEMPOL),
						&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sRet.eError != PVRSRV_OK)
	{
		return sRet.eError;
	}

	return sRet.eError;
}
#endif /* !defined(OS_PVRSRV_PDUMP_MEM_POL_UM) */

#if !defined(OS_PVRSRV_PDUMP_MEM_PAGES_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpMemPages

 @Description	PDumps memory to file via kernel services

 @Input		psConnection : connection info
 @Input		psMemInfo
 @Input		ui32Offset
 @Input		ui32Bytes
 @Input		ui32Flags

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpMemPages(const PVRSRV_DEV_DATA *psDevData,
#if defined (SUPPORT_SID_INTERFACE)
														   IMG_SID				hKernelMemInfo,
#else
														   IMG_HANDLE			hKernelMemInfo,
#endif
														   IMG_DEV_PHYADDR		*pPages,
														   IMG_UINT32			ui32NumPages,
												   		   IMG_DEV_VIRTADDR		sDevVAddr,
														   IMG_UINT32			ui32Start,
														   IMG_UINT32			ui32Length,
														   IMG_UINT32			ui32Flags)
{
	PVRSRV_BRIDGE_IN_PDUMP_MEMPAGES	sIn;
	PVRSRV_BRIDGE_RETURN			sRet;

	if( !psDevData || !psDevData->psConnection )
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sIn.pPages = pPages;
	sIn.ui32NumPages = ui32NumPages;
	sIn.sDevVAddr = sDevVAddr;
	sIn.ui32Start = ui32Start;
	sIn.ui32Length = ui32Length;
	sIn.ui32Flags = ui32Flags;
	sIn.hKernelMemInfo = hKernelMemInfo;
	sIn.hDevCookie = psDevData->hDevCookie;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_PDUMP_MEMPAGES,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_PDUMP_MEMPAGES),
						&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sRet.eError != PVRSRV_OK)
	{
		return sRet.eError;
	}

	return sRet.eError;
}
#endif /* !defined(OS_PVRSRV_PDUMP_MEM_PAGES_UM) */


#if !defined(OS_PVRSRV_PDUMP_SYNC_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpSync

 @Description	PDumps sync info to file via kernel services

 @Input		psConnection : connection info
 @Input		pvAltLinAddr
 @Input		psSyncInfo
 @Input		ui32Offset
 @Input		ui32Bytes

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpSync(const PVRSRV_CONNECTION *psConnection,
													 IMG_PVOID pvAltLinAddr,
													 PVRSRV_CLIENT_SYNC_INFO *psSyncInfo,
													 IMG_UINT32 ui32Offset,
													 IMG_UINT32 ui32Bytes)
{
	PVRSRV_BRIDGE_IN_PDUMP_DUMPSYNC sIn;
	PVRSRV_BRIDGE_RETURN sRet;

	if(!psConnection || !psSyncInfo)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sIn.pvAltLinAddr = pvAltLinAddr;
#if defined (SUPPORT_SID_INTERFACE)
	sIn.hKernelSyncInfo = psSyncInfo->hKernelSyncInfo;
#else
	sIn.psKernelSyncInfo = psSyncInfo->hKernelSyncInfo;
#endif
	sIn.ui32Offset = ui32Offset;
	sIn.ui32Bytes = ui32Bytes;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_PDUMP_DUMPSYNC,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_PDUMP_DUMPSYNC),
						&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sRet.eError;
}
#endif /* !defined(OS_PVRSRV_PDUMP_SYNC_UM) */


#if !defined(OS_PVRSRV_PDUMP_SYNC_POL_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpSyncPolBridge

 @Description	PDumps memory to file via kernel services

 @Input		psConnection : connection info
 @Input		psClientSyncInfo
 @Input		bIsRead
 @Input		bUseLastOpDumpVal
 @Input		ui32Value  (relevant iff !bUseLastOpDumpVal)
 @Input		ui32Mask   (relevant iff !bUseLastOpDumpVal)

 @Return	PVRSRV_ERROR

 ******************************************************************************/
static PVRSRV_ERROR PVRSRVPDumpSyncPolBridge(const PVRSRV_CONNECTION *psConnection,
#if defined (SUPPORT_SID_INTERFACE)
											 IMG_SID  hKernelSyncInfo,
#else
											 PVRSRV_CLIENT_SYNC_INFO *psClientSyncInfo,
#endif
											 IMG_BOOL bIsRead,
											 IMG_BOOL bUseLastOpDumpVal,
											 IMG_UINT32 ui32Value,
											 IMG_UINT32 ui32Mask)
{
	PVRSRV_BRIDGE_IN_PDUMP_SYNCPOL	sIn;
	PVRSRV_BRIDGE_RETURN			sRet;

#if defined (SUPPORT_SID_INTERFACE)
	if(!psConnection || !hKernelSyncInfo)
#else
	if(!psConnection || !psClientSyncInfo)
#endif
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

#if defined (SUPPORT_SID_INTERFACE)
	sIn.hKernelSyncInfo = hKernelSyncInfo;
#else
	sIn.psKernelSyncInfo = psClientSyncInfo->hKernelSyncInfo;
#endif
	sIn.bIsRead = bIsRead;
	sIn.bUseLastOpDumpVal = bUseLastOpDumpVal;
	sIn.ui32Value = ui32Value;
	sIn.ui32Mask = ui32Mask;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_PDUMP_SYNCPOL,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_PDUMP_SYNCPOL),
						&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sRet.eError;
}


/*!
 ******************************************************************************

 @Function	PVRSRVPDumpSyncPol

 @Description	PDumps memory to file via kernel services

 @Input		psConnection : connection info
 @Input		psClientSyncInfo
 @Input		bIsRead
 @Input		ui32Value
 @Input		ui32Mask

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpSyncPol(const PVRSRV_CONNECTION *psConnection,
#if defined (SUPPORT_SID_INTERFACE)
														IMG_SID    hKernelSyncInfo,
#else
														PVRSRV_CLIENT_SYNC_INFO *psClientSyncInfo,
#endif
														IMG_BOOL   bIsRead,
														IMG_UINT32 ui32Value,
														IMG_UINT32 ui32Mask)
{
	/* TODO: move this function outside of bridge code (srvclient/common, perhaps?) */
#if defined (SUPPORT_SID_INTERFACE)
    return PVRSRVPDumpSyncPolBridge(psConnection, hKernelSyncInfo, bIsRead, IMG_FALSE, ui32Value, ui32Mask);
#else
    return PVRSRVPDumpSyncPolBridge(psConnection, psClientSyncInfo, bIsRead, IMG_FALSE, ui32Value, ui32Mask);
#endif
}


/*!
 ******************************************************************************

 @Function	PVRSRVPDumpSyncPol2

 @Description	PDumps memory to file via kernel services

 @Input		psConnection : connection info
 @Input		psClientSyncInfo
 @Input		bIsRead

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpSyncPol2(const PVRSRV_CONNECTION *psConnection,
#if defined (SUPPORT_SID_INTERFACE)
														 IMG_SID    hKernelSyncInfo,
#else
														 PVRSRV_CLIENT_SYNC_INFO *psClientSyncInfo,
#endif
														 IMG_BOOL bIsRead)
{
	/* TODO: move this function outside of bridge code (srvclient/common, perhaps?) */
#if defined (SUPPORT_SID_INTERFACE)
    return PVRSRVPDumpSyncPolBridge(psConnection, hKernelSyncInfo, bIsRead, IMG_TRUE, 0, 0);
#else
    return PVRSRVPDumpSyncPolBridge(psConnection, psClientSyncInfo, bIsRead, IMG_TRUE, 0, 0);
#endif
}
#endif /* !defined(OS_PVRSRV_PDUMP_SYNC_POL_UM) */


#if !defined(OS_PVRSRV_PDUMP_REG_READ_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpRegRead

 @Description	Dumps a read from a device register to a file

 @Input		psConnection : connection info
 @Input		pszFileName
 @Input		ui32FileOffset
 @Input		ui32Address
 @Input		ui32Size
 @Input		ui32PDumpFlags

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpRegRead(const PVRSRV_DEV_DATA *psDevData,
											 const IMG_CHAR		   *pszRegRegion,
											 const IMG_CHAR 	   *pszFileName,
											 IMG_UINT32 		   ui32FileOffset,
											 IMG_UINT32 		   ui32Address,
											 IMG_UINT32 		   ui32Size,
											 IMG_UINT32 		   ui32PDumpFlags)
{
	PVRSRV_BRIDGE_IN_PDUMP_READREG sIn;
	PVRSRV_BRIDGE_RETURN sRet;

	if(!psDevData)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	strncpy(sIn.szFileName, pszFileName, PVRSRV_PDUMP_MAX_FILENAME_SIZE);

	sIn.hDevCookie = psDevData->hDevCookie;
	sIn.ui32FileOffset = ui32FileOffset;
	sIn.ui32Address = ui32Address;
	sIn.ui32Size = ui32Size;
	sIn.ui32Flags = ui32PDumpFlags;

	PVRSRVMemCopy(sIn.szRegRegion, pszRegRegion, (IMG_UINT32)strlen(pszRegRegion) + 1);

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_PDUMP_DUMPREADREG,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_PDUMP_READREG),
						&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sRet.eError;
}
#endif /* !defined(OS_PVRSRV_PDUMP_REG_READ_UM) */


#if !defined(OS_PVRSRV_PDUMP_REG_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpReg

 @Description	PDumps a register write to file via kernel services

 @Input		psDevData : device data info
 @Input		ui32RegAddr
 @Input		ui32RegValue
 @Input		ui32Flags

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpReg(const PVRSRV_DEV_DATA	*psDevData,
													IMG_CHAR				*pszRegRegion,
													IMG_UINT32				ui32RegAddr,
													IMG_UINT32				ui32RegValue,
													IMG_UINT32				ui32Flags)
{
	PVRSRV_BRIDGE_IN_PDUMP_DUMPREG	sIn;
	PVRSRV_BRIDGE_RETURN sRet;

	if(!psDevData)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sIn.hDevCookie			= psDevData->hDevCookie;
	sIn.sHWReg.ui32RegAddr	= ui32RegAddr;
	sIn.sHWReg.ui32RegVal	= ui32RegValue;
	sIn.ui32Flags			= ui32Flags;

	PVRSRVMemCopy(sIn.szRegRegion, pszRegRegion, (IMG_UINT32)strlen(pszRegRegion) + 1);

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_PDUMP_REG,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_PDUMP_DUMPREG),
						&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sRet.eError != PVRSRV_OK)
	{
		return sRet.eError;
	}

	return sRet.eError;
}
#endif /* !defined(OS_PVRSRV_PDUMP_REG_UM) */


#if !defined(OS_PVRSRV_PDUMP_REG_POL_WITH_FLAGS_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpRegPolWithFlags

 @Description	PDumps a register pol to file via kernel services

 @Input		psConnection : connection info
 @Input		ui32RegAddr
 @Input		ui32RegValue
 @Input		ui32Mask
 @Input		ui32Flags

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpRegPolWithFlags(const PVRSRV_DEV_DATA *psDevData,
																IMG_CHAR	*pszRegRegion,
																IMG_UINT32 ui32RegAddr,
																IMG_UINT32 ui32RegValue,
																IMG_UINT32 ui32Mask,
																IMG_UINT32 ui32Flags)
{
	PVRSRV_BRIDGE_IN_PDUMP_REGPOL	sIn;
	PVRSRV_BRIDGE_RETURN sRet;

	if(!psDevData)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sIn.sHWReg.ui32RegAddr = ui32RegAddr;
	sIn.sHWReg.ui32RegVal = ui32RegValue;
	sIn.ui32Mask = ui32Mask;
	sIn.ui32Flags = ui32Flags;
	sIn.hDevCookie = psDevData->hDevCookie;

	PVRSRVMemCopy(sIn.szRegRegion, pszRegRegion, (IMG_UINT32)strlen(pszRegRegion) + 1);

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_PDUMP_REGPOL,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_PDUMP_REGPOL),
						&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sRet.eError != PVRSRV_OK)
	{
		return sRet.eError;
	}

	return sRet.eError;
}
#endif /* !defined(OS_PVRSRV_PDUMP_REG_POL_WITH_FLAGS_UM) */


#if !defined(OS_PVRSRV_PDUMP_REG_POL_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpRegPol

 @Description	PDumps a register pol to file via kernel services

 @Input		psConnection : connection info
 @Input		ui32RegAddr
 @Input		ui32RegValue
 @Input		ui32Mask

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpRegPol(const PVRSRV_DEV_DATA *psDevData,
													   IMG_CHAR	*pszRegRegion,
													   IMG_UINT32 ui32RegAddr,
													   IMG_UINT32 ui32RegValue,
													   IMG_UINT32 ui32Mask)
{
	/* FIXME: should we really be assuming the CONTINUOUS flag here? */

	return PVRSRVPDumpRegPolWithFlags(psDevData,
									  pszRegRegion,
									  ui32RegAddr,
									  ui32RegValue,
									  ui32Mask,
									  PDUMP_FLAGS_CONTINUOUS);
}
#endif /* !defined(OS_PVRSRV_PDUMP_REG_POL_UM) */


#if !defined(OS_PVRSRV_PDUMP_PDDEVPADDR_UM)
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpPDDevPAddr(const PVRSRV_CONNECTION *psConnection,
														   PVRSRV_CLIENT_MEM_INFO *psMemInfo,
														   IMG_UINT32 ui32Offset,
														   IMG_DEV_PHYADDR sPDDevPAddr)
{
	PVRSRV_BRIDGE_IN_PDUMP_DUMPPDDEVPADDR sIn;
	PVRSRV_BRIDGE_RETURN sRet;

	if(!psConnection)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sIn.hKernelMemInfo = psMemInfo->hKernelMemInfo;
	sIn.ui32Offset = ui32Offset;
	sIn.sPDDevPAddr = sPDDevPAddr;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_PDUMP_DUMPPDDEVPADDR,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_PDUMP_DUMPPDDEVPADDR),
						&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sRet.eError;
}
#endif /* !defined(OS_PVRSRV_PDUMP_PDDEVPADDR_UM) */


#if !defined(OS_PVRSRV_PDUMP_CYCLE_COUNT_REG_READ_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpCycleCountRegRead

 @Description	Dumps the specified cycle counter

 @Input		psDevData
 @Input		ui32RegOffset
 @Input		bLastFrame

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpCycleCountRegRead(const PVRSRV_DEV_DATA *psDevData,
													   IMG_UINT32 ui32RegOffset,
													   IMG_BOOL bLastFrame)
{
	PVRSRV_BRIDGE_IN_PDUMP_CYCLE_COUNT_REG_READ	sIn;
	PVRSRV_BRIDGE_RETURN sRet;

	if(!psDevData)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sIn.hDevCookie = psDevData->hDevCookie;
	sIn.ui32RegOffset = ui32RegOffset;
	sIn.bLastFrame = bLastFrame;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_PDUMP_CYCLE_COUNT_REG_READ,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_PDUMP_CYCLE_COUNT_REG_READ),
						&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sRet.eError;
}
#endif /* !defined(OS_PVRSRV_PDUMP_CYCLE_COUNT_REG_READ_UM) */


#if !defined(OS_PVRSRV_PDUMP_SET_FRAME_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpSetFrame

 @Description	Sets the pdump frame

 @Input		psConnection : connection info
 @Input		ui32Frame

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpSetFrame(const PVRSRV_CONNECTION *psConnection,
														 IMG_UINT32 ui32Frame)
{
	PVRSRV_BRIDGE_IN_PDUMP_SETFRAME	sIn;
	PVRSRV_BRIDGE_RETURN sRet;

	if(!psConnection)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sIn.ui32Frame = ui32Frame;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_PDUMP_SETFRAME,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_PDUMP_SETFRAME),
						&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sRet.eError;
}
#endif /* !defined(OS_PVRSRV_PDUMP_SET_FRAME_UM) */


#if !defined(OS_PVRSRV_PDUMP_DRIVER_INFO_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpDriverInfo

 @Description	PDumps driver-internal information

 @Input		psConnection : connection info
 @Input		pszString
 @Input		bContinuous

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpDriverInfo(const PVRSRV_CONNECTION *psConnection,
														   IMG_CHAR *pszString,
														   IMG_BOOL bContinuous)
{
	PVRSRV_BRIDGE_IN_PDUMP_DRIVERINFO	sIn;
	PVRSRV_BRIDGE_RETURN				sRet;

	if(!psConnection)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	strncpy(sIn.szString, pszString, PVRSRV_PDUMP_MAX_COMMENT_SIZE);
	sIn.bContinuous = bContinuous;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_PDUMP_DRIVERINFO,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_PDUMP_DRIVERINFO),
						&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sRet.eError;
}
#endif /* !defined(OS_PVRSRV_PDUMP_DRIVER_INFO_UM) */


#if !defined(OS_PVRSRV_PDUMP_IS_CAPTURING_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpIsCapturing

 @Description	Boolean query

 @Input		psConnection : connection info
 @Output	pbIsCapturing

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpIsCapturing(const PVRSRV_CONNECTION *psConnection,
															IMG_BOOL *pbIsCapturing)
{
	PVRSRV_BRIDGE_OUT_PDUMP_ISCAPTURING sOut;

	if(!psConnection)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_PDUMP_ISCAPTURING,
						IMG_NULL,
						0,
						&sOut,
						sizeof(PVRSRV_BRIDGE_OUT_PDUMP_ISCAPTURING)))
	{
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sOut.eError != PVRSRV_OK)
	{
		return sOut.eError;
	}

	*pbIsCapturing = sOut.bIsCapturing;

	return sOut.eError;
}
#endif /* !defined(OS_PVRSRV_PDUMP_IS_CAPTURING_UM) */


#if !defined(OS_PVRSRV_PDUMP_IS_CAPTURING_TEST_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpIsCapturingTest

 @Description	Boolean query

 @Input		psConnection : connection info

 @Return	IMG_BOOL

 ******************************************************************************/
IMG_EXPORT
IMG_BOOL IMG_CALLCONV PVRSRVPDumpIsCapturingTest(const PVRSRV_CONNECTION *psConnection)
{
	IMG_BOOL bIsCapturing;
	bIsCapturing = IMG_FALSE;

	PVRSRVPDumpIsCapturing(psConnection, &bIsCapturing);

	return (bIsCapturing);
}
#endif /* !defined(OS_PVRSRV_PDUMP_IS_CAPTURING_TEST_UM) */


#if !defined(OS_PVRSRV_PDUMP_BITMAP_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpBitmap

 @Description	Dumps a bitmap from device memory to a file

 @Input		psDevData : connection info
 @Input		pszFileName
 @Input		ui32FileOffset
 @Input		ui32Width
 @Input		ui32Height
 @Input		ui32StrideInBytes
 @Input		sDevBaseAddr
 @Input		ui32Size
 @Input		ePixelFormat
 @Input		eMemFormat
 @Input		ui32PDumpFlags

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpBitmap(const PVRSRV_DEV_DATA *psDevData,
											IMG_CHAR *pszFileName,
											IMG_UINT32 ui32FileOffset,
											IMG_UINT32 ui32Width,
											IMG_UINT32 ui32Height,
											IMG_UINT32 ui32StrideInBytes,
											IMG_DEV_VIRTADDR sDevBaseAddr,
#if defined (SUPPORT_SID_INTERFACE)
											IMG_SID    hDevMemContext,
#else
											IMG_HANDLE hDevMemContext,
#endif
											IMG_UINT32 ui32Size,
											PDUMP_PIXEL_FORMAT ePixelFormat,
											PDUMP_MEM_FORMAT eMemFormat,
											IMG_UINT32 ui32PDumpFlags)
{
	PVRSRV_BRIDGE_IN_PDUMP_BITMAP sIn;
	PVRSRV_BRIDGE_RETURN sRet;

	if(!psDevData)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	strncpy(sIn.szFileName, pszFileName, PVRSRV_PDUMP_MAX_FILENAME_SIZE);

	sIn.hDevCookie = psDevData->hDevCookie;
	sIn.ui32FileOffset = ui32FileOffset;
	sIn.ui32Width = ui32Width;
	sIn.ui32Height = ui32Height;
	sIn.ui32StrideInBytes = ui32StrideInBytes;
	sIn.sDevBaseAddr = sDevBaseAddr;
	sIn.hDevMemContext = hDevMemContext;
	sIn.ui32Size = ui32Size;
	sIn.ePixelFormat = ePixelFormat;
	sIn.eMemFormat = eMemFormat;
	sIn.ui32Flags = ui32PDumpFlags;

	if(PVRSRVBridgeCall(psDevData->psConnection->hServices,
						PVRSRV_BRIDGE_PDUMP_DUMPBITMAP,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_PDUMP_BITMAP),
						&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sRet.eError;
}
#endif /* !defined(OS_PVRSRV_PDUMP_BITMAP_UM) */


#if !defined(OS_PVRSRV_PDUMP_COMMENT_WITH_FLAGSF_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpCommentWithFlagsf

 @Description	PDumps a comment, passing in flags

 @Input		psConnection : connection info
 @Input		ui32Flags
 @Input		pszFormat
 @Input		...

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV
PVRSRVPDumpCommentWithFlagsf(const PVRSRV_CONNECTION *psConnection,
							 IMG_UINT32 ui32Flags,
							 const IMG_CHAR *pszFormat, ...)
{
	PVRSRV_BRIDGE_IN_PDUMP_COMMENT	sIn;
	PVRSRV_BRIDGE_RETURN sRet;
	IMG_CHAR szScript[PVRSRV_PDUMP_MAX_COMMENT_SIZE];
	va_list argList;

	va_start(argList, pszFormat);
	vsnprintf(szScript, sizeof(szScript), pszFormat, argList);
	va_end(argList);

	if(!psConnection)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	strncpy(sIn.szComment, szScript, PVRSRV_PDUMP_MAX_COMMENT_SIZE);
	sIn.ui32Flags = ui32Flags;

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_PDUMP_COMMENT,
						&sIn,
						sizeof(PVRSRV_BRIDGE_IN_PDUMP_COMMENT),
						&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	if(sRet.eError != PVRSRV_OK)
	{
		return sRet.eError;
	}

	return sRet.eError;
}
#endif /* !defined(OS_PVRSRV_PDUMP_COMMENT_WITH_FLAGSF_UM) */


#if !defined(OS_PVRSRV_PDUMP_COMMENT_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpComment

 @Description	PDumps a comment

 @Input		psConnection : connection info
 @Input		pszComment
 @Input		bContinuous

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpComment(const PVRSRV_CONNECTION *psConnection,
														const IMG_CHAR *pszComment,
														IMG_BOOL bContinuous)
{
	return PVRSRVPDumpCommentWithFlagsf(psConnection,
										bContinuous?PDUMP_FLAGS_CONTINUOUS:0,
										"%s",
										pszComment);
}
#endif /* !defined(OS_PVRSRV_PDUMP_COMMENT_UM) */


#if !defined(OS_PVRSRV_PDUMP_COMMENTF_UM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpCommentf

 @Description	PDumps a formatted comment

 @Input		psConnection : connection info
 @Input		bContinuous
 @Input		pszFormat
 @Input		...

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpCommentf(const PVRSRV_CONNECTION *psConnection,
														 IMG_BOOL bContinuous,
														 const IMG_CHAR *pszFormat, ...)
{
	IMG_CHAR szScript[PVRSRV_PDUMP_MAX_COMMENT_SIZE];
	va_list argList;

	va_start(argList, pszFormat);
	vsnprintf(szScript, sizeof(szScript), pszFormat, argList);
	va_end(argList);

	/* On some platforms snprintf() et. al. are not guaranteed to null terminate the result */
	szScript[PVRSRV_PDUMP_MAX_COMMENT_SIZE - 1] = '\0';

	return PVRSRVPDumpComment(psConnection, szScript, bContinuous);
}
#endif /* !defined(OS_PVRSRV_PDUMP_COMMENTF_UM) */

#if !defined(OS_PVRSRV_PDUMP_RESUME_INIT_PHASE_KM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpStartInitPhase

 @Description	Resume the pdump init phase state

 @Input		psConnection : connection info

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpStartInitPhase(IMG_CONST PVRSRV_CONNECTION *psConnection)
{
	PVRSRV_BRIDGE_RETURN sRet;

	if(!psConnection)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_PDUMP_STARTINITPHASE,
						IMG_NULL,
						0,
						&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sRet.eError;
}
#endif /* !defined(OS_PVRSRV_PDUMP_RESUME_INIT_PHASE_KM) */

#if !defined(OS_PVRSRV_PDUMP_STOP_INIT_PHASE_KM)
/*!
 ******************************************************************************

 @Function	PVRSRVPDumpStopInitPhase

 @Description	Resume the pdump init phase state

 @Input		psConnection : connection info

 @Return	PVRSRV_ERROR

 ******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVPDumpStopInitPhase(IMG_CONST PVRSRV_CONNECTION *psConnection)
{
	PVRSRV_BRIDGE_RETURN sRet;

	if(!psConnection)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(PVRSRVBridgeCall(psConnection->hServices,
						PVRSRV_BRIDGE_PDUMP_STOPINITPHASE,
						IMG_NULL,
						0,
						&sRet,
						sizeof(PVRSRV_BRIDGE_RETURN)))
	{
		return PVRSRV_ERROR_BRIDGE_CALL_FAILED;
	}

	return sRet.eError;
}
#endif /* !defined(OS_PVRSRV_PDUMP_STOP_INIT_PHASE_KM) */

#endif /* PDUMP */

/******************************************************************************
 End of file (bridged_pvr_pdump_glue.c)
******************************************************************************/
