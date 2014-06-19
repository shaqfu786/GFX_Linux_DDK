/******************************************************************************
 * Name         : pvrmmap.h
 * Author       : Vlad Stamate
 * Created      : 19/10/2001
 *
 * Copyright    : 2001-2008 by Imagination Technologies Limited. 
 *                  All rights reserved.
 *                  No part of this software, either material or conceptual 
 *                  may be copied or distributed, transmitted, transcribed,
 *                  stored in a retrieval system or translated into any 
 *                  human or computer language in any form by any means,
 *                  electronic, mechanical, manual or other-wise, or 
 *                  disclosed to third parties without the express written
 *                  permission of:
 *                             Imagination Technologies Limited, 
 *                             Home Park Estate, 
 *                             Kings Langley, 
 *                             Hertfordshire,
 *                             WD4 8LZ, 
 *                             UK
 *
 * Platform     : ANSI
 * Description	: Main include file for PVRMMAP library
 *
 * Modifications:-
 * $Log: pvrmmap.h $
 *****************************************************************************/
#ifndef __PVRMMAP_H__
#define __PVRMMAP_H__

/*!
 **************************************************************************
 @brief         map kernel memory into user memory. 

 @param			hModule - a handle to the device supplying the kernel memory
 @param			ppvLinAddr - pointer to where the user mode address should be placed
 @param			pvLinAddrKM - the base of kernel address range to map
 @param			phMappingInfo - pointer to mapping information handle
 @param			hMHandle - handle associated with memory to be mapped

 @return		PVRSRV_OK, or error code.
 ***************************************************************************/

#if defined (SUPPORT_SID_INTERFACE)
PVRSRV_ERROR PVRPMapKMem(IMG_HANDLE hModule, IMG_VOID **ppvLinAddr, IMG_VOID *pvLinAddrKM, IMG_SID *phMappingInfo, IMG_SID hMHandle);
#else
PVRSRV_ERROR PVRPMapKMem(IMG_HANDLE hModule, IMG_VOID **ppvLinAddr, IMG_VOID *pvLinAddrKM, IMG_HANDLE *phMappingInfo, IMG_HANDLE hMHandle);
#endif


/*!
 **************************************************************************
 @brief	        Removes a kernel to userspace memory mapping.

 @param		hModule - a handle to the device supplying the kernel memory
 @param		hMappingInfo - mapping information handle
 @param		hMHandle - handle associated with memory to be mapped

 @return	IMG_BOOL indicating success or otherwise.
 ***************************************************************************/
#if defined (SUPPORT_SID_INTERFACE)
IMG_BOOL PVRUnMapKMem(IMG_HANDLE hModule, IMG_SID hMappingInfo, IMG_SID hMHandle);
#else
IMG_BOOL PVRUnMapKMem(IMG_HANDLE hModule, IMG_HANDLE hMappingInfo, IMG_HANDLE hMHandle);
#endif

#endif /* _PVRMMAP_H_ */

