/******************************************************************************
 Name			: COMMON.H

 Title			: 3D Tools common stuff...

 C Author		: John Howson

 Created		: 4/11/97

 Copyright		: Copyright 1997-2007 by Imagination Technologies Limited.
				  All rights reserved. No part of this software, either
				  material or conceptual may be copied or distributed,
				  transmitted, transcribed, stored in a retrieval system or
				  translated into any human or computer language in any form
				  by any means, electronic, mechanical, manual or otherwise,
				  or disclosed to third parties without the express written
				  permission of Imagination Technologies Limited,
				  Home Park Estate, Kings Langley, Hertfordshire,
				  WD4 8LZ, U.K.

 Description	:

 Program Type	: 32-bit DLL

 Version		: $Revision: 1.9 $

 Modifications	:
 $Log: common.h $
******************************************************************************/

#ifndef _PDCOMMON_
#define _PDCOMMON_

typedef IMG_VOID (*PFN_WRITENPIXPAIRS) (IMG_UINT32 ui32Addr,IMG_UINT16 *pui16Pix,IMG_UINT32 ui32Count);
typedef IMG_VOID (*PFN_REGWRITE) (IMG_UINT32 ui32Reg,IMG_UINT32 ui32Value);
typedef IMG_VOID (*PFN_WRITEBMP) (IMG_CHAR *);

extern PFN_WRITENPIXPAIRS	pfnWriteNPixelPairs;
extern PFN_REGWRITE			pfnRegWrite;
extern PFN_WRITEBMP			pfnWriteBMP;

extern IMG_CHAR		szOutFileName[256];
extern IMG_UINT32	ui32Start;
extern IMG_UINT32	ui32Stop;
extern IMG_UINT32	ui32SampleRate;
extern IMG_BOOL		bMultiFrames;
extern IMG_BOOL		bSampleRatePresent;
extern IMG_BOOL		bNoDAC;
extern IMG_BOOL		bNoREF;

void FreeStuff(void);

IMG_BOOL ParseInputParams(IMG_INT nNumParams,IMG_CHAR ** ppszParams);
IMG_BOOL ConnectToSim(IMG_VOID);
IMG_BOOL ReadHWRegisters(IMG_UINT32 *);

#endif /* _PDCOMMON_ */

/******************************************************************************
 End of file (COMMON.H)
******************************************************************************/

