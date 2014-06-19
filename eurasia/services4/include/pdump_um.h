/******************************************************************************
 * Name         : pdump_um.h
 * Author       : Imagination Technologies
 * Created      : 6/12/2007
 *
 * Copyright    : 2007-2008 by Imagination Technologies Limited.
 *              : All rights reserved. No part of this software, either
 *              : material or conceptual may be copied or distributed,
 *              : transmitted, transcribed, stored in a retrieval system or
 *              : translated into any human or computer language in any form
 *              : by any means, electronic, mechanical, manual or otherwise,
 *              : or disclosed to third parties without the express written
 *              : permission of Imagination Technologies Limited, Home Park
 *              : Estate, Kings Langley, Hertfordshire, WD4 8LZ, U.K.
 *
 * Platform     : ANSI
 *
 * $Log: pdump_um.h $
 *****************************************************************************/
#ifndef _PDUMP_UM_H_
#define _PDUMP_UM_H_

#include "pdump.h"

#if defined(PDUMP)

#ifdef PDUMP_DEBUG
#include "pvr_debug.h"
#define PDUMPDEBUG(x) (((x) != PVRSRV_OK) ? \
			PVR_DPF((PVR_DBG_ERROR, #x " failed")), \
			x : PVRSRV_OK)
#else
#define PDUMPDEBUG(x) (x)
#endif


#define PDUMPINIT(psServices) \
			PDUMPDEBUG(PVRSRVPDumpInit(psServices))
#define PDUMPMEM(psServices, pvAltLinAddr, psMemInfo, ui32Offset, \
				 ui32Bytes, ui32Flags) \
			PDUMPDEBUG(PVRSRVPDumpMem(psServices, pvAltLinAddr, psMemInfo, \
					   ui32Offset, ui32Bytes, ui32Flags))
#define PDUMPMEMPOL(psServices, psMemInfo, ui32Offset, ui32Value, ui32Mask, \
					ui32Flags) \
			PDUMPDEBUG(PVRSRVPDumpMemPol(psServices, psMemInfo, ui32Offset, \
					   ui32Value, ui32Mask, PDUMP_POLL_OPERATOR_EQUAL, ui32Flags))
#define PDUMPMEMPOLWITHOP(psServices, psMemInfo, ui32Offset, ui32Value, ui32Mask, \
					eOperator, ui32Flags) \
			PDUMPDEBUG(PVRSRVPDumpMemPol(psServices, psMemInfo, ui32Offset, \
					   ui32Value, ui32Mask, eOperator, ui32Flags))
#define PDUMPREG(psServices, pszPDumpRegName, ui32RegAddr, ui32RegValue, ui32Flags) \
			PDUMPDEBUG(PVRSRVPDumpReg(psServices, pszPDumpRegName, ui32RegAddr, ui32RegValue, \
					   ui32Flags))
#define PDUMPREGPOLWITHFLAGS(psServices, pszPDumpRegName, ui32RegAddr, ui32RegValue, ui32Mask, \
							 ui32Flags) \
			PDUMPDEBUG(PVRSRVPDumpRegPolWithFlags(psServices, pszPDumpRegName, ui32RegAddr, \
					   ui32RegValue, ui32Mask, ui32Flags))
#define PDUMPREGPOL(psServices, pszPDumpRegName, ui32RegAddr, ui32RegValue, ui32Mask) \
			PDUMPDEBUG(PVRSRVPDumpRegPol(psServices, pszPDumpRegName, ui32RegAddr, \
					   ui32RegValue, ui32Mask))
#define PDUMPPDREG(psServices, ui32RegAddr, ui32RegValue) \
			PDUMPDEBUG(PVRSRVPDumpPDReg(psServices, ui32RegAddr, ui32RegValue))
#define PDUMPPDDEVPADDR(psServices, psMemInfo,ui32Offset,sPDDevPAddr) \
			PDUMPDEBUG(PVRSRVPDumpPDDevPAddr(psServices, psMemInfo, \
					   ui32Offset, sPDDevPAddr))
#define PDUMPSYNC(psServices, pvAltLinAddr, psSyncInfo, ui32Offset, ui32Bytes) \
			PDUMPDEBUG(PVRSRVPDumpSync(psServices, pvAltLinAddr, psSyncInfo, \
					   ui32Offset, ui32Bytes))
#define PDUMPSYNCPOL(psServices, psClientSyncInfo, bIsRead) \
	PDUMPDEBUG(PVRSRVPDumpSyncPol2(psServices, psClientSyncInfo,			\
								   bIsRead))
#define PDUMPSETFRAME(psServices, ui32Frame) \
			PDUMPDEBUG(PVRSRVPDumpSetFrame(psServices, ui32Frame))
#define PDUMPCOMMENT(psServices, pszComment, bContinuous) \
			PDUMPDEBUG(PVRSRVPDumpComment(psServices, pszComment, bContinuous))
/* For the sake of a neat interface that supports varargs with no awkward
 * bracketing of args to trick the preprocessor, we don't use the PDUMPDEBUG
 * macros for these next two. */
#define PDUMPCOMMENTF PVRSRVPDumpCommentf
#define PDUMPCOMMENTWITHFLAGSF PVRSRVPDumpCommentWithFlagsf
#define PDUMPDRIVERINFO(psServices, pszString, bContinuous) \
			PDUMPDEBUG(PVRSRVPDumpDriverInfo(psServices, pszString, bContinuous))
#define PDUMPISCAPTURING(psServices, pbIsCapturing) \
			PDUMPDEBUG(PVRSRVPDumpIsCapturing(psServices, pbIsCapturing))

#define PDUMPISCAPTURINGTEST(psServices) \
			PDUMPDEBUG(PVRSRVPDumpIsCapturingTest(psServices))

#define PDUMPBITMAP(psServices, pszFileName, ui32FileOffset, ui32Width, \
					ui32Height, ui32StrideInBytes, sDevBaseAddr, ui32Size, \
					ePixelFormat, eMemFormat, ui32PDumpFlags) \
			PDUMPDEBUG(PVRSRVPDumpBitmap(psServices, pszFileName, ui32FileOffset, \
					ui32Width, ui32Height, ui32StrideInBytes, sDevBaseAddr, \
					ui32Size, ePixelFormat, eMemFormat, ui32PDumpFlags))
#define PDUMPREGREAD(psServices, pszRegRegion, pszFileName, ui32FileOffset, ui32Address, \
					ui32Size, ui32PDumpFlags) \
			PDUMPDEBUG(PVRSRVPDumpRegRead(psServices, pszRegRegion, pszFileName, \
					   ui32FileOffset, ui32Address, ui32Size, ui32PDumpFlags))

#else /* PDUMP */

#define PDUMPINIT(psServices)
#define PDUMPMEM(psServices, pvAltLinAddr, psMemInfo, ui32Offset, \
				 ui32Bytes, ui32Flags)
#define PDUMPMEMPOL(psServices, psMemInfo, ui32Offset, ui32Value, ui32Mask, \
					ui32Flags)
#define PDUMPMEMPOLWITHOP(psServices, psMemInfo, ui32Offset, ui32Value, ui32Mask, eOperator, ui32Flags)
#define PDUMPREG(psServices, ui32RegAddr, ui32RegValue, ui32Flags)
#define PDUMPREGPOLWITHFLAGS(psServices, pszPDumpRegName, ui32RegAddr, ui32RegValue, ui32Mask, \
							 ui32Flags)
#define PDUMPREGPOL(psServices, pszPDumpRegName, ui32RegAddr, ui32RegValue, ui32Mask)
#define PDUMPPDREG(psServices, ui32RegAddr, ui32RegValue)
#define PDUMPPDDEVPADDR(psServices, psMemInfo,ui32Offset,sPDDevPAddr)
#define PDUMPSYNC(psServices, pvAltLinAddr, psSyncInfo, ui32Offset, ui32Bytes)
#define PDUMPSYNCPOL(psServices, psClientSyncInfo, bIsRead, ui32Value, \
					 ui32Mask)
#define PDUMPSETFRAME(psServices, ui32Frame)
#define PDUMPCOMMENT(psServices, Comment, bContinuous)

#if defined (__linux__)
/* The following while(0) trick is intended as a portable way of turning these
 * vararg macros into NOPS, but will fail for W4 level and treat warnings as errors */
#define PDUMPCOMMENTF	while(0)PVRSRVPDumpCommentf
#define PDUMPCOMMENTWITHFLAGSF	while(0)PVRSRVPDumpCommentWithFlagsf
#else
#define PDUMPCOMMENTF	/##/
#define PDUMPCOMMENTWITHFLAGSF	/##/
#endif

#define PDUMPDRIVERINFO(psServices, pszString, bContinuous)
#define PDUMPISCAPTURING(psServices, pbIsCapturing)
#define PDUMPISCAPTURINGTEST(psServices) 
#define PDUMPBITMAP(psServices, pszFileName, ui32FileOffset, ui32Width, \
					ui32Height, ui32StrideInBytes, sDevBaseAddr, ui32Size, \
					ePixelFormat, eMemFormat, ui32PDumpFlags)
#define PDUMPREGREAD(psServices, pszFileName, ui32FileOffset, ui32Address, \
					ui32Size, ui32PDumpFlags)
#endif /* PDUMP */

#endif /* _PDUMP_UM_H_ */

/******************************************************************************
  End of file (pdump_um.h)
******************************************************************************/
