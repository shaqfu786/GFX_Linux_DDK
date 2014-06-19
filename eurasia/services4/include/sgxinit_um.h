/*!****************************************************************************
@File           sgxapiint_u.h

@Title          SGX internal API Header

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

@Description    Services API used between different parts of services

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-

$Log: sgxinit_um.h $
*****************************************************************************/

#ifndef __SGXINIT_UM_H__
#define __SGXINIT_UM_H__

#if defined (__cplusplus)
extern "C" {
#endif

#include "sgxapi.h"
#include "sgxinfo.h"

typedef struct _SGX_CLIENT_INIT_INFO_ {
	PVRSRV_CLIENT_MEM_INFO *psKernelCCBMemInfo;
	PVRSRV_SGX_KERNEL_CCB 	*psKernelCCB;
	PVRSRV_CLIENT_MEM_INFO	*psKernelCCBEventKickerMemInfo;	/*!< meminfo for kernel CCB event kicker */
	PVRSRV_CLIENT_MEM_INFO *psKernelCCBCtlMemInfo;
	PVRSRV_SGX_CCB_CTL *psKernelCCBCtl;
	PVRSRV_CLIENT_MEM_INFO *psTA3DCtlMemInfo;
	PVRSRV_CLIENT_MEM_INFO	*psHostCtlMemInfo;
 	PVRSRV_CLIENT_MEM_INFO *psKernelSGXMiscMemInfo;
 	IMG_UINT32	ui32ClientBuildOptions;
 	SGX_MISCINFO_STRUCT_SIZES	sSGXStructSizes;
#if defined(FIX_HW_BRN_29702)
	PVRSRV_CLIENT_MEM_INFO *psCFIMemInfo;
#endif
#if defined(FIX_HW_BRN_29823)
	PVRSRV_CLIENT_MEM_INFO *psDummyTermStreamMemInfo;
#endif

#if defined(FIX_HW_BRN_31542) || defined(FIX_HW_BRN_36513)
	PVRSRV_CLIENT_MEM_INFO *psClearClipWAVDMStreamMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psClearClipWAIndexStreamMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psClearClipWAPDSMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psClearClipWAUSEMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psClearClipWAParamMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psClearClipWAPMPTMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psClearClipWATPCMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psClearClipWAPSGRgnHdrMemInfo;
#endif

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) && defined(FIX_HW_BRN_31425)
	PVRSRV_CLIENT_MEM_INFO *psVDMSnapShotBufferMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psVDMCtrlStreamBufferMemInfo;
#endif
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) && \
	defined(FIX_HW_BRN_33657) && defined(SUPPORT_SECURE_33657_FIX)
	PVRSRV_CLIENT_MEM_INFO *psVDMStateUpdateBufferMemInfo;	
#endif
#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG)
	PVRSRV_CLIENT_MEM_INFO *psEDMStatusBufferMemInfo;
#endif
#if defined(SGX_SUPPORT_HWPROFILING)
	PVRSRV_CLIENT_MEM_INFO 	*psHWProfilingMemInfo;
#endif
#if defined(SUPPORT_SGX_HWPERF)
	PVRSRV_CLIENT_MEM_INFO	*psHWPerfCBMemInfo;
#endif
	PVRSRV_CLIENT_MEM_INFO	*psTASigBufferMemInfo;
	PVRSRV_CLIENT_MEM_INFO	*ps3DSigBufferMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psMicrokernelUSEMemInfo;
#if !defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
	PVRSRV_CLIENT_MEM_INFO *psMicrokernelTAUSEMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psMicrokernelSPMUSEMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psMicrokernel3DUSEMemInfo;
#if defined(SGX_FEATURE_2D_HARDWARE)
	PVRSRV_CLIENT_MEM_INFO *psMicrokernel2DUSEMemInfo;
#endif
#endif /* SGX_FEATURE_MONOLITHIC_UKERNEL */
#if defined(SGX_FEATURE_MP)
	PVRSRV_CLIENT_MEM_INFO *psMicrokernelSlaveUSEMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psMicrokernelPDSEventSlaveMemInfo;
#endif
	PVRSRV_CLIENT_MEM_INFO *psMicrokernelPDSInitPrimary1MemInfo;
	PVRSRV_CLIENT_MEM_INFO *psMicrokernelPDSInitSecondaryMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psMicrokernelPDSInitPrimary2MemInfo;
	PVRSRV_CLIENT_MEM_INFO *psMicrokernelPDSEventMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psMicrokernelPDSLoopbackMemInfo;
	IMG_UINT32 aui32HostKickAddr[SGXMKIF_CMD_MAX];
	IMG_DEV_VIRTADDR sDevVAddrPDSExecBase;
	IMG_DEV_VIRTADDR sDevVAddrUSEExecBase;
	IMG_UINT32 ui32NumUSETemporaryRegisters;
	IMG_UINT32 ui32NumUSEAttributeRegisters;
	IMG_UINT32 ui32CacheControl;

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	PVRSRV_CLIENT_MEM_INFO *psPDSCtxSwitchTermMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psUSECtxSwitchTermMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psUSETAStateMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psPDSTAStateMemInfo;
	#if defined(SGX_FEATURE_MTE_STATE_FLUSH) && defined(SGX_FEATURE_COMPLEX_GEOMETRY_REV_2)
	PVRSRV_CLIENT_MEM_INFO *psUSECmplxTAStateMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psPDSCmplxTAStateMemInfo;
	#endif
	#if defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
	PVRSRV_CLIENT_MEM_INFO *psUSESARestoreMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psPDSSARestoreMemInfo;
	#endif
#endif
	IMG_UINT32 ui32EVMConfig;
	IMG_UINT32 ui32EDMTaskReg0;
	IMG_UINT32 ui32EDMTaskReg1;
	IMG_UINT32 ui32ClkGateCtl;
	IMG_UINT32 ui32ClkGateCtl2;
	IMG_UINT32 ui32ClkGateStatusReg;
	IMG_UINT32 ui32ClkGateStatusMask;
#if defined(SGX_FEATURE_MP)
	IMG_UINT32 ui32MasterClkGateStatusReg;
	IMG_UINT32 ui32MasterClkGateStatusMask;
	IMG_UINT32 ui32MasterClkGateStatus2Reg;
	IMG_UINT32 ui32MasterClkGateStatus2Mask;
#endif /* SGX_FEATURE_MP */
	SGX_INIT_SCRIPTS sScripts;
#if defined(FIX_HW_BRN_23861)
	PVRSRV_CLIENT_MEM_INFO *psDummyStencilLoadMemInfo;
	PVRSRV_CLIENT_MEM_INFO *psDummyStencilStoreMemInfo;
#endif

#if defined(FIX_HW_BRN_31272) || defined(FIX_HW_BRN_31780) || defined(FIX_HW_BRN_33920)
	PVRSRV_CLIENT_MEM_INFO	* psPTLAWriteBackMemInfo;
#endif

} SGX_CLIENT_INIT_INFO;


IMG_IMPORT
PVRSRV_ERROR SGXGetInfoForSrvInit(PVRSRV_DEV_DATA *psDevData, SGX_BRIDGE_INFO_FOR_SRVINIT *psInitInfo);

IMG_IMPORT
PVRSRV_ERROR SGXDevInitPart2(PVRSRV_DEV_DATA *psDevData, SGX_CLIENT_INIT_INFO *psInitInfo);

#if defined(SGX543) || defined(SGX544) || defined(SGX554)
	#define SGX_DEFAULT_NUM_EXTRA_OUTPUT_PARTITIONS 1
#else
	#define SGX_DEFAULT_NUM_EXTRA_OUTPUT_PARTITIONS 0
#endif


#if defined(FIX_HW_BRN_31542) || defined(FIX_HW_BRN_36513)

#define SGX_1M_ALIGNED				1<<20
#define SGX_SAFELY_PAGE_ALIGNED			1<<12
#define SGX_128b_ALIGNED			1<<7 /* Most buffers like VDM, Index-bufs, PDS progs, USE progs are happy with this. */
#define SGX_32b_ALIGNED				1<<5

#define SGX_CLIPPERBUG_PARTITION_SIZE		3
#define MAX_CORES				4
#define NUM_TRIANGLES				MAX_CORES * 2
#define INDEX_SIZE				4
#define SGX_CLIPPERBUG_VDM_BUFFER_SIZE		128 /* 2 MTE state update, 5 index list words, 1 mte-terminate dwords + buffer*/
#define SGX_CLIPPERBUG_INDEX_BUFFER_SIZE	3 * NUM_TRIANGLES * INDEX_SIZE
#define SGX_CLIPPERBUG_PDS_BUFFER_SIZE 		128 * 4 /* ~16 dwords(mte-state update program), ~16 dwords(vertex-loading) + buffer */
#define SGX_CLIPPERBUG_USE_BUFFER_SIZE		128 * 4 /* ~30 dwords(mte-state update program), ~30 dwords(vertex-loading) + buffer */
#define SGX_CLIPPERBUG_PARAMMEM_NUM_PAGES	20
#define SGX_CLIPPERBUG_PARAMMEM_BUFFER_SIZE	SGX_CLIPPERBUG_PARAMMEM_NUM_PAGES * 4096 /* More than enough parameter-memory,
									     */
#define SGX_CLIPPERBUG_DPMPT_BUFFER_SIZE	4096 /* A larger than required page table. */
#define SGX_CLIPPERBUG_TPC_BUFFER_SIZE		EURASIA_TAILPOINTER_SIZE * 32 * 32 * 4 /* EURASIA_TAILPOINTER_SIZE * 32 * 32 EURASIA_TAILPOINTER_SIZE is 8(MP-tail-pointer size) */
#define SGX_CLIPPERBUG_PSGRGNHDR_BUFFER_SIZE	EURASIA_REGIONHEADER_SIZE * 32 * 32 * 4 /* Region Header Size using a macro  */

#define SGX_CLIPPERBUG_MTE_UPDATE_PDSPROG_BASE_OFFSET 0
#define SGX_CLIPPERBUG_INDEX_BUFFER_BASE_OFFSET 3
#define SGX_CLIPPERBUG_VTXLOAD_PDSPROG_BASE_OFFSET 5
#define SGX_CLIPPERBUG_MTETERM_PDSPROG_BASE_OFFSET 7

#define SGX_CLIPPERBUG_PDS_PROGRAM_SIZE 80
#define SGX_CLIPPERBUG_HWBRN_CODE_BASE_INDEX 14

#define SGX_CLIPPERBUG_PDSPROG_USE0OFFSET 32
#define SGX_CLIPPERBUG_VTX_SHADER_USE_PROG_OFFSET 16*8 /* 14 instructions */
#define SGX_CLIPPERBUG_TERM_USE_PROG_OFFSET (16+19) * 8 
#define SGX_CLIPPERBUG_NUMTEMPS 1
#endif


#if defined (__cplusplus)
}
#endif
#endif /* __SGXINIT_UM_H__ */

/******************************************************************************
 End of file (sgxapiint_u.h)
******************************************************************************/
