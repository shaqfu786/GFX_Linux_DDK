/*!****************************************************************************
@File           sgxpb.h

@Title          Device specific utility routines declarations

@Author         Imagination Technologies

@Date           15 / 5 / 06

@Copyright      Copyright 2006-2008 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       Generic

@Description    Inline functions/structures specific to SGX PB

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-
$Log: sgxpb.h $
******************************************************************************/

#ifndef _SGXPB_H_
#define _SGXPB_H_

#define GET_ALIGNED_PB_SIZE(Size) ((Size) + (EURASIA_PARAM_MANAGE_GRAN - 1)) \
									& ~(EURASIA_PARAM_MANAGE_GRAN - 1);

#if defined(SUPPORT_PERCONTEXT_PB)
PVRSRV_ERROR CreatePerContextPB(const PVRSRV_DEV_DATA *psDevData,
									SGX_RENDERCONTEXT *psRenderContext,
									IMG_UINT32 ui32ParamSize,
							#if defined(FORCE_ENABLE_GROW_SHRINK)
									IMG_UINT32	ui23ParamSizeLimit,
							#endif
									const PVRSRV_HEAP_INFO *psPBHeapInfo,
									const PVRSRV_HEAP_INFO *psKernelVideoDataHeapInfo);

IMG_VOID DestroyPerContextPB(const PVRSRV_DEV_DATA *psDevData,
									SGX_CLIENTPBDESC *psClientPBDesc);

	#if defined(FORCE_ENABLE_GROW_SHRINK) && defined(SUPPORT_PERCONTEXT_PB)
		#if defined(PDUMP)
		PVRSRV_ERROR	PdumpPBGrowShrinkUpdates(const PVRSRV_DEV_DATA *psDevData,
											SGX_RENDERCONTEXT	*psRenderContext);
		#endif
PVRSRV_ERROR	HandlePBGrowShrink(const PVRSRV_DEV_DATA *psDevData,
									SGX_RENDERCONTEXT	*psRenderContext,
									IMG_BOOL			bOutOfMemory);
PVRSRV_ERROR	PBGrowShrinkComplete(const PVRSRV_DEV_DATA *psDevData,
									SGX_RENDERCONTEXT	*psRenderContext);
	#endif
#endif
#if defined(SUPPORT_SHARED_PB)

PVRSRV_ERROR CreateSharedPB(const PVRSRV_DEV_DATA *psDevData,
#if defined (SUPPORT_SID_INTERFACE)
									IMG_SID *phSharedPBDesc,
#else
									IMG_HANDLE *phSharedPBDesc,
#endif
									IMG_UINT32 ui32ParamSize,
#if defined (SUPPORT_SID_INTERFACE)
									IMG_SID		hDevMemContext,
#else
									IMG_HANDLE	hDevMemContext,
#endif
									const PVRSRV_HEAP_INFO *psPBHeapInfo,
									const PVRSRV_HEAP_INFO *psKernelVideoDataHeapInfo);

IMG_VOID UnrefPB(const PVRSRV_DEV_DATA *psDevData,
						SGX_CLIENTPBDESC *psClientPBDesc);

PVRSRV_ERROR MapSharedPBDescToClient(const PVRSRV_DEV_DATA *psDevData,
#if defined (SUPPORT_SID_INTERFACE)
										IMG_SID hSharedPBDesc,
										IMG_SID hSharedPBDescKernelMemInfoHandle,
										IMG_SID hHWPBDescKernelMemInfoHandle,
										IMG_SID hBlockKernelMemInfoHandle,
										IMG_SID hHWBlockKernelMemInfoHandle,
										const IMG_SID *phSubKernelMemInfoHandles,
#else
										IMG_HANDLE hSharedPBDesc,
										IMG_HANDLE hSharedPBDescKernelMemInfoHandle,
										IMG_HANDLE hHWPBDescKernelMemInfoHandle,
										IMG_HANDLE hBlockKernelMemInfoHandle,
										IMG_HANDLE hHWBlockKernelMemInfoHandle,
										const IMG_HANDLE *phSubKernelMemInfoHandles,
#endif
										IMG_UINT32 ui32SubKernelMemInfoHandlesCount,
										SGX_CLIENTPBDESC *psClientPBDesc);
#endif /* SUPPORT_SHARED_PB */

#endif /*_SGXPB_H_*/

/******************************************************************************
 End of file (sgxpb.h)
******************************************************************************/
