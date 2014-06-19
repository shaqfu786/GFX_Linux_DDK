/*!****************************************************************************
@File           sgx_mkif.h

@Title          SGX microkernel interface structures

@Author         Imagination Technologies

@Date           29th Jul 2009

@Copyright      Copyright 2009 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed to
                third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       SGX

@Description    SGX microkernel interface structures used by sgxinit

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-
$Log: sgx_mkif.h $
*****************************************************************************/
#if !defined (__SGX_MKIF_H__)
#define __SGX_MKIF_H__

#include "sgxapi.h"
#include "sgx_mkif_km.h"


/*!
 *****************************************************************************
 * SGX microkernel state. This state is stored in USSE secondary attributes
 * unless SGX_DISABLE_UKERNEL_SECONDARY_STATE is defined.
 *****************************************************************************/
typedef struct _SGXMK_STATE_
{
#if defined(FIX_HW_BRN_28889)
	IMG_UINT32				ui32CacheCtrl;			/*!< holds the cache ctrl word form the command so it can be done later */
#endif

	IMG_DEV_VIRTADDR		sTransferCCBCmd;		/*!< address of SGXMKIF_TRANSFERCMD for current transfer kick */

	IMG_UINT32				ui32TACurrentPriority;	/*!< The priority level of the active TA context */
	IMG_UINT32				ui32BlockedPriority;	/*!< Priority of the waiting context, reset value is SGX_MAX_CONTEXT_PRIORITY-1 */
	IMG_UINT32				ui323DCurrentPriority;	/*!< The priority level of the active 3D context */

	IMG_DEV_VIRTADDR		s3DContext;				/*!< the context the current render belongs to */

	#if defined(SGX_FEATURE_2D_HARDWARE)
	IMG_UINT32				ui32I2DFlags;			/*!< internal flags relating to current 2D kick */
	#endif
	IMG_UINT32				ui32ITAFlags;			/*!< internal flags relating to current TA kick */
	IMG_UINT32				ui32IRenderFlags;		/*!< internal flags relating to current render */

	IMG_UINT32				ui32HostRequest;		/*!< Flags for host requests */

	IMG_UINT32				ui32ActivePowerCounter;	/*!< Idle counter for Active Power Management */
	IMG_UINT32				ui32ActivePowerFlags;	/*!< Private ukernel flags for Active Power Management */

	IMG_DEV_VIRTADDR		sTransferContextHead[SGX_MAX_CONTEXT_PRIORITY];	/*!< first item in list of transfer contexts with outstanding commands */
	IMG_DEV_VIRTADDR		sTransferContextTail[SGX_MAX_CONTEXT_PRIORITY];	/*!< last item in list of transfer contexts with outstanding commands */

	IMG_DEV_VIRTADDR		sPartialRenderContextHead[SGX_MAX_CONTEXT_PRIORITY];	/*!< first item in list of render contexts with outstanding TA commands */
	IMG_DEV_VIRTADDR		sPartialRenderContextTail[SGX_MAX_CONTEXT_PRIORITY];	/*!< last item in list of render contexts with outstanding TA commands */

	IMG_DEV_VIRTADDR		sCompleteRenderContextHead[SGX_MAX_CONTEXT_PRIORITY];	/*!< first item in list of render contexts with outstanding 3D commands */
	IMG_DEV_VIRTADDR		sCompleteRenderContextTail[SGX_MAX_CONTEXT_PRIORITY];	/*!< last item in list of render contexts with outstanding 3D commands */

#if defined(SGX_FEATURE_2D_HARDWARE)
	IMG_DEV_VIRTADDR		s2DContextHead[SGX_MAX_CONTEXT_PRIORITY];	/*!< first item in list of 2D contexts with outstanding commands */
	IMG_DEV_VIRTADDR		s2DContextTail[SGX_MAX_CONTEXT_PRIORITY];	/*!< last item in list of 2D contexts with outstanding commands */
#endif

#if defined(SGX_FEATURE_MP)
	IMG_UINT32				ui32CoresEnabled;		/*!< Number of cores which are enabled */
#endif

#if defined(FIX_HW_BRN_31272) || defined(FIX_HW_BRN_31780) || defined(FIX_HW_BRN_33920)
	IMG_UINT32				ui32DummyFillCnt;		/*!< record the rolling blit counter */
#endif

#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG)
	IMG_DEV_VIRTADDR		sMKTraceBuffer;			/*!< See SGXMK_TRACE_BUFFER */
	IMG_UINT32				ui32MKTBOffset;			/*!< Write offset into the MK Trace Buffer */
	IMG_UINT32				ui32MKTraceProfile;		/*!< Mask of enabled MK Trace Families (MKTF_*) */
#endif

	IMG_UINT32				ui32HWPerfFlags;		/*!< See PVRSRV_SGX_HWPERF_STATUS_* */

#if defined(FIX_HW_BRN_31939)
	IMG_UINT32				ui32BRN31939SA;
#endif

#if defined(SGX_FEATURE_MK_SUPERVISOR)
	#if defined(SGX_DISABLE_UKERNEL_SECONDARY_STATE)
		#error ERROR: SGX_FEATURE_MK_SUPERVISOR only supported if SGX_DISABLE_UKERNEL_SECONDARY_STATE not set
	#else
	IMG_UINT32				ui32SPRVTask;			/*!< Supervisor Task Type */
	IMG_UINT32				ui32SPRVArg0;			/*!< Supervisor Register Argument 0 */
	IMG_UINT32				ui32SPRVArg1;			/*!< Supervisor Register Argument 1 */
	#endif
#endif	
} SGXMK_STATE;


/*!
 *****************************************************************************
 * USE PDS other events primary attribute layout
 *****************************************************************************/
typedef struct _PVRSRV_SGX_EDMPROG_PRIMATTR_
{
	IMG_UINT32				ui32PDSIn0;				/*!< PDS input register 0 */
	IMG_UINT32				ui32PDSIn1;				/*!< PDS input register 1 */
#if defined(SUPPORT_SGX_HWPERF)
	IMG_UINT32				ui32TimeStamp;			/*!< Current timestamp passed down to the USSE */
#endif
} PVRSRV_SGX_EDMPROG_PRIMATTR;

#if defined(SGX_FEATURE_MP)
typedef struct _PVRSRV_SGX_SLAVE_EDMPROG_PRIMATTR_
{
	IMG_UINT32				ui32PDSIn0;				/*!< PDS input register 0 */
	IMG_UINT32				ui32PDSIn1;				/*!< PDS input register 1 */
	IMG_DEV_VIRTADDR		sTA3DCtl;				/*!< device virtual address of the TA/3D control structure */
} PVRSRV_SGX_SLAVE_EDMPROG_PRIMATTR;

/*!< Number of PAs required by the slave ukernel */
#define 	SGX_UKERNEL_SLAVE_NUM_PRIM_ATTRIB		(sizeof(PVRSRV_SGX_SLAVE_EDMPROG_PRIMATTR) / sizeof(IMG_UINT32))
#endif

/*!
 *****************************************************************************
 * USE PDS other events secondary attribute layout
 *****************************************************************************/
typedef struct _PVRSRV_SGX_EDMPROG_SECATTR_
{
	/* Read-only members */
	IMG_DEV_VIRTADDR		sTA3DCtl;			/*!< device virtual address of the TA/3D control structure */
	IMG_DEV_VIRTADDR		sHostCtl;			/*!< device virtual address of the Host control structure */
	IMG_DEV_VIRTADDR		sCCBCtl;			/*!< device virtual address of the CCB control structure */

#if !defined(SGX_DISABLE_UKERNEL_SECONDARY_STATE)
	/* Writable (by the ukernel) members */
	SGXMK_STATE				sMKState;
#endif /* SGX_DISABLE_UKERNEL_SECONDARY_STATE */
	
} PVRSRV_SGX_EDMPROG_SECATTR;


/*!< Number of temps required by the ukernel
	Although we don't need 32 temps in all configurations
	(i.e. without supervisor) as the HW will allocate that
	many anyway then it keeps things simpler and cleaner
	to always request 32 temps
 */
#define 	SGX_UKERNEL_NUM_TEMPS			32UL

/*!< Number of PAs required by the ukernel */
#define 	SGX_UKERNEL_NUM_PRIM_ATTRIB		(sizeof(PVRSRV_SGX_EDMPROG_PRIMATTR) / sizeof(IMG_UINT32))

/*!< DMA burst size for SAs */
#define		SGX_UKERNEL_SA_BURST_SIZE		(1UL << 4)

/*!< Number of SAs required by the ukernel */
#define 	SGX_UKERNEL_NUM_SEC_ATTRIB		(((sizeof(PVRSRV_SGX_EDMPROG_SECATTR) / sizeof(IMG_UINT32)) + EURASIA_PDS_CHUNK_SIZE - 1) & ~(EURASIA_PDS_CHUNK_SIZE - 1))

#define 	SGX_LOOPBACK_ATTRIB_SPACE(NumAttribs)	((((NumAttribs) * sizeof(IMG_UINT32) + ((1UL << EURASIA_PDSSB0_USEATTRSIZE_ALIGNSHIFT) - 1)) \
													 >> EURASIA_PDSSB0_USEATTRSIZE_ALIGNSHIFT) << EURASIA_PDSSB0_USEATTRSIZE_SHIFT)

/*!< Maximum number of lines per DMA (each line is 1 block of SAs = 16 dwords) */
#define		SGX_31425_DMA_NLINES			16U
#define		SGX_31425_DMA_NLINES_MINUS1		15U
#define		SGX_31425_DMA_NLINES_SHIFT		4U

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) && \
	defined(FIX_HW_BRN_33657) && defined(SUPPORT_SECURE_33657_FIX)
	/*!< Flag for the VDM context store state update task */
#define		VDM_TERMINATE_COMPLETE_ONLY		0x1U
#endif

#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG)
/*!
 *****************************************************************************
 * Circular Buffer of EDM status values
 *****************************************************************************/

/*
	Defines for the misc info dword:
	TASKID - USSE taskid (from G21 register)
	EVENTS - accumulated numer of EDM tasks - from EUR_CR_USE0_SERV_EVENT
	FAULT - set if the FAULT bits of EUR_CR_BIF_INT_STAT are non-zero.
*/
#define SGXMK_TRACE_BUFFER_MISC_TASKID_SHIFT	0
#define SGXMK_TRACE_BUFFER_MISC_TASKID_MASK		0x000000FF
#define SGXMK_TRACE_BUFFER_MISC_EVENTS_SHIFT	8
#define SGXMK_TRACE_BUFFER_MISC_EVENTS_MASK		0x01FFFF00
#define SGXMK_TRACE_BUFFER_MISC_FAULT_SHIFT		25
#define SGXMK_TRACE_BUFFER_MISC_FAULT_MASK		0x02000000

typedef struct _SGXMK_TRACE_BUFFER_ENTRY_
{
	/*!< See MKTC_xxx. */
	IMG_UINT32								ui32StatusValue;

	/*!< See SGXMK_TRACE_BUFFER_MISC_* for details. */
	IMG_UINT32								ui32MiscInfo;

	/*!< Timestamp from SGX clock */
	IMG_UINT32								ui32TimeStamp;
	
	/*!< Value of custom hardware register */
	IMG_UINT32								ui32Custom;

} SGXMK_TRACE_BUFFER_ENTRY;

typedef struct _SGXMK_TRACE_BUFFER_
{
	/*!< Latest status value. Same format as SGXMK_TRACE_BUFFER_ENTRY.ui32StatusValue */
	/* This field is needed for debugging using a logic analyser, which requires */
	/* the most recent status value stored at a fixed address. It must be first in the structure, */
	/* to ensure it is 128-bit aligned. */
	IMG_UINT32						ui32LatestStatusValue;

	/*!< Write offset into circular buffer */
	IMG_UINT32						ui32WriteOffset;

	/*!< Circular buffer of status value entries. */
	SGXMK_TRACE_BUFFER_ENTRY		asBuffer[SGXMK_TRACE_BUFFER_SIZE];

} SGXMK_TRACE_BUFFER;
#endif /* PVRSRV_USSE_EDM_STATUS_DEBUG */


/*!
 *****************************************************************************
 * Microkernel trace family defines
 *****************************************************************************/
#define MKTF_EV		(1 << 0)			/* Events handled in-module */
#define MKTF_EHEV	(1 << 1)			/* Events handled in event handler */
#define MKTF_HW		(1 << 2)			/* SGX hardware kicks */
#define MKTF_SCH	(1 << 3)			/* General scheduling information */
#define MKTF_TA		(1 << 4)			/* TA details */
#define MKTF_3D		(1 << 5)			/* 3D details */
#define MKTF_TQ		(1 << 6)			/* Transfer Queue details */
#define MKTF_2D		(1 << 7)			/* 2D details */
#define MKTF_SPM	(1 << 8)			/* SPM details */
#define MKTF_SO		(1 << 9)			/* Synchronisation objects data */
#define MKTF_SV		(1 << 10)			/* Status values data */
#define MKTF_HK		(1 << 11)			/* Host kicks */
#define MKTF_POW	(1 << 12)			/* Power */
#define MKTF_TIM	(1 << 13)			/* Timer */
#define MKTF_HWR	(1 << 14)			/* Hardware Recovery information */
#define MKTF_PB		(1 << 15)			/* Parameter Buffer */
#define MKTF_CSW	(1 << 16)			/* Context switch */
#define MKTF_CU		(1 << 17)			/* Clean-up */
#define MKTF_FRMNUM	(1 << 18)			/* Frame numbers */
#define MKTF_RC		(1 << 19)			/* Render contexts */
#define MKTF_RT		(1 << 20)			/* Render targets */
#define MKTF_PID	(1 << 21)			/* Process ID info */


#if defined(SGX_SUPPORT_HWPROFILING)
/*!
 *****************************************************************************
 * Data for HW Profiling
 *****************************************************************************/
typedef struct _SGXMKIF_HWPROFILING
{
	IMG_UINT32 ui32StartTA;
	IMG_UINT32 ui32EndTA;
	IMG_UINT32 ui32Start3D;
	IMG_UINT32 ui32End3D;
} SGXMKIF_HWPROFILING;
#endif /* SGX_SUPPORT_HWPROFILING*/

/*!
 *****************************************************************************
 * Control data for TA/3D
 *****************************************************************************/
typedef struct _SGXMK_TA3D_CTL_
{
	PVRSRV_SGX_EDMPROG_SECATTR	sMKSecAttr;

	/*****************************************************************************************************
	The fields above are held in EDM secondary attributes.
	The microkernel always accesses the fields below in memory.
	*****************************************************************************************************/

#if defined(SGX_DISABLE_UKERNEL_SECONDARY_STATE)
	SGXMK_STATE				sMKState;
#endif /* SGX_DISABLE_UKERNEL_SECONDARY_STATE */

	/* Common state */
	IMG_DEV_VIRTADDR		sContext0RTData;		/*!< Device virtual address of RTData struct 
															associated with context 0 */
	IMG_DEV_VIRTADDR		sContext1RTData;		/*!< Device virtual address of RTData struct 
															associated with context 1 */
	IMG_DEV_VIRTADDR		sKernelCCB;				/*!< address of the kernel CCB */
	IMG_DEV_PHYADDR			sKernelPDDevPAddr;		/*!< kernel memory context page directory physical address */

#if defined(SGX_FEATURE_2D_HARDWARE)
	/*!< 2D state */
	IMG_DEV_VIRTADDR		s2DCCBCmd;				/*!< address of SGXMKIF_2DCMD for current 2D kick */
	IMG_DEV_VIRTADDR		s2DContext;				/*!< The context the current 2D command belongs to */
#endif

#if defined(FIX_HW_BRN_31272) || defined(FIX_HW_BRN_31780) || defined(FIX_HW_BRN_33920)
	IMG_DEV_VIRTADDR		sPTLAWriteBackDevVAddr;	/*!< writeback address */
#endif

	/*!< TA state */
	IMG_DEV_VIRTADDR		sTACCBCmd;				/*!< address of SGXMKIF_CMDTA for current TA kick */
	IMG_DEV_VIRTADDR		sTARTData;				/*!< address of the RTData the current TA command is using */
	IMG_DEV_VIRTADDR		sTARenderDetails;		/*!< render details associated with current TA kick */
	IMG_DEV_VIRTADDR		sTARenderContext;		/*!< the render context the current TA command belongs to */
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) && defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
	IMG_DEV_VIRTADDR		sVDMSABufferDevAddr;	/*!< Device virtual address of secondary attribute buffer \n
														to be used during context switching */
	#if defined(SGX_FEATURE_MP)
	/*** THIS MEMBER MUST BE IMMEDIATELY AFTER sVDMSABufferDevAddr DUE TO BLOCK LOAD ***/
	IMG_UINT32				ui32VDMSABufferStride;	/*!< size of the SA buffer per core in bytes */
	#endif
#endif

#if defined(FIX_HW_BRN_32302) || defined(SUPPORT_SGX_DOUBLE_PIXEL_PARTITIONS)
	IMG_UINT16				ui16RealZLSThreshold;				/*!< Saved ZLS threshold before setting to to zero to be able to restore it */
#endif
	/*!< 3D state */
	IMG_DEV_VIRTADDR		s3DRTData;				/*!< address of the RTData the current render is using */
	IMG_DEV_VIRTADDR		s3DRenderDetails;		/*!< render details associated with current 3D render */
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	IMG_UINT32				ui32ActiveRTIdx;		/*!< Idx of RT currently being rendered */
#endif
#if defined(FIX_HW_BRN_30089) || defined(FIX_HW_BRN_29504) || defined(FIX_HW_BRN_33753)
	IMG_UINT32				ui32SavedRgnHeader[SGX_FEATURE_MP_CORE_COUNT_3D][4];		/*!< The saved value of the region header modified */
#endif

	IMG_UINT32				ui32PendingLoopbacks;	/*!< loopback events currently queued */

	IMG_DEV_VIRTADDR		sResManCleanupData;		/*!< Resource Management data to clean-up */
#if defined(SGX_FAST_DPM_INIT) && (SGX_FEATURE_NUM_USE_PIPES > 1)
	IMG_DEV_VIRTADDR		sInitHWPBDesc;				/*!< Device virtual address of the SGXMKIF_HWPBDesc for the 
																Parameter Buffer to be initialised */
#endif

#if defined(FIX_HW_BRN_31109)
	IMG_UINT32				ui32TACheckSum;						/*!< Record of checksum value to decide, if its truly hit 31109 */
#endif
#if defined(FIX_HW_BRN_29461) || \
	(!defined(FIX_HW_BRN_23055) && !defined(SGX_FEATURE_MP))
	IMG_UINT32				ui32SavedGblPolicy;					/*!< Record of the global_list policy to be restored after the dplist
																	 has been stored to memory */
#endif
#if defined(FIX_HW_BRN_23055)
	IMG_UINT16				ui16SavedZLSThreshold;				/*!< Saved ZLS threshold before setting to to zero to be able to restore it */
	IMG_UINT16				ui16Num23055PagesUsed;				/*!< Record of the number of pages from the Parameter Buffer
																	 currently being used for the BRN23055 workaround */
	IMG_UINT16				ui16LastMTEPageAllocation;			/*!< Record of the last page allocated to the MTE, this is used
																	 to ensure the page remains allocated during the next partial render */
	IMG_UINT16				ui16LastMTEPageRgn;					/*!< Record of which region (Macrotile or Global list) the last MTE 
																	 page was allocated to. Required for patching the DPM state table. */
	IMG_UINT32				ui32MTETEDeadlockTicks;				/*!< Number of PDS timer events since the last OUTOFMEM event was received */
	#if defined(DEBUG)
	IMG_UINT32				ui32MaximumZLSThresholdIncrement;	/*!< Record of the highest number of pages used by the BRN23055 workaround */
	IMG_UINT32				ui32TEOutOfMem;						/*!< Record of number of OUTOFMEM events received where only the TE is requesting */
	IMG_UINT32				ui32MTEOutOfMem;					/*!< Record of number of OUTOFMEM events received where only the MTE is requesting */
	IMG_UINT32				ui32MTETEOutOfMem;					/*!< Record of number of OUTOFMEM events received where both the MTE and TE are requesting */
	IMG_UINT32				ui32FLPatchOutOfMem;					/*!< Record of number time we re-issued MTE page as part of 23055 workaround */
	#endif /* defined(DEBUG) */
#endif /* defined(FIX_HW_BRN_23055) */
#if 1 // SPM_PAGE_INJECTION
	IMG_UINT32				ui32NumDLRenders;					/*!< Record of the number of renders done as a result of a SPM deadlock condition */ 
	#if !defined(FIX_HW_BRN_23055)
	IMG_UINT32				ui32AbortPages;						/*!< Record of the number of MT and Gbl pages allocated before partial render */
	IMG_UINT32				ui32GblAbortPages;					/*!< Record of the number of Gbl pages allocated before partial render */
	IMG_UINT32				ui32SavedGlobalList;				/*!< Record of the global threshold before pages were injected */
	#endif
	IMG_UINT32				ui32DeadlockPageIncrease;			/*!< Number of pages from the Parameter Buffer currently by used to avoid
																a deadlock condition */
	#if defined(DEBUG)
	IMG_UINT32				ui32ForceGlobalOOMCount;			/*! deadlock counter to detect infinite loop */
	#endif
#endif
#if defined(SGX_FEATURE_MP)
	IMG_UINT32				ui32MPTileMode;						/*!< Record of the default tile mode, saved before render which use a DPM 
																	 compressed ZS buffer mode and restored after. */
#endif
	IMG_UINT32				ui32AbortedMacroTile; 				/*!< Record of which MT has been aborted */

#if defined(FIX_HW_BRN_29702)
	IMG_DEV_VIRTADDR		sCFIMemDevAddr;						/*!< Device virtual address of memory required for BRN29702 */
#endif
#if defined(SGX_FEATURE_RENDER_TARGET_ARRAYS)
	#if defined(FIX_HW_BRN_29823)
	IMG_DEV_VIRTADDR		sDummyTermDevAddr;					/*!< Device virtual address of ctrl stream with only terminate word */
	#endif
	IMG_UINT32				ui32AbortedRT;						/*!< Record of which Render target in the array has been aborted */
#endif

#if !defined(USE_SUPPORT_NO_TA3D_OVERLAP) && defined(DEBUG)
	IMG_UINT32				ui32OverlapCount;					/*!< Records the number of TA/3D operations which have been overlapped */
#endif

#if defined(DEBUG) || defined(PDUMP) || defined(SGX_SUPPORT_HWPROFILING) || defined(SUPPORT_SGX_HWPERF)
	IMG_UINT32				ui32TAFrameNum;						/*!< Frame number of the current or last run TA operation */
	IMG_UINT32				ui323DFrameNum;						/*!< Frame number of the current or last run 3D operation */
	#if defined(SGX_FEATURE_2D_HARDWARE)
	IMG_UINT32				ui322DFrameNum;						/*!< Frame number of the current or last run 2D operation */
	#endif
#endif

#if defined(FIX_HW_BRN_23861)
	IMG_DEV_VIRTADDR	sDummyZLSReqBaseDevVAddr;				/*!< Device virtual address to be used for the ZLS requestor base
																	 when using the dummy stencil buffer */
	IMG_DEV_VIRTADDR	sDummyStencilLoadDevVAddr;				/*!< Device virtual address to be used for loading the dummy stencil buffer */
	IMG_DEV_VIRTADDR	sDummyStencilStoreDevVAddr;				/*!< Device virtual address to be used for storing the dummy stencil buffer */
#endif
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
#if defined(FIX_HW_BRN_31425)
	IMG_DEV_VIRTADDR	sVDMSnapShotBufferDevAddr;				/*!< VDM Snapshot Buffer device virtual address */
	IMG_DEV_VIRTADDR	sVDMCtrlStreamBufferDevAddr;			/*!< VDM control stream Buffer device virtual address */
#endif
#if 1 // SGX_SUPPORT_MASTER_ONLY_SWITCHING
	IMG_DEV_VIRTADDR	sDevAddrSAStateRestoreDMA_InputDOUTD0;	/*!< PDS SA restore DMA control word0 */
	IMG_DEV_VIRTADDR	sDevAddrSAStateRestoreDMA_InputDOUTD1;	/*!< PDS SA restore DMA control word1 */
	IMG_DEV_VIRTADDR	sDevAddrSAStateRestoreDMA_InputDOUTD1_REMBLOCKS;	/*!< PDS SA restore DMA control word1 */
	IMG_DEV_VIRTADDR	sDevAddrSAStateRestoreDMA_InputDMABLOCKS;		/*!< PDS SA restore DMA control word1 */
#endif
#if defined(FIX_HW_BRN_33657) && defined(SUPPORT_SECURE_33657_FIX)
	IMG_DEV_VIRTADDR	sVDMStateUpdateBufferDevAddr;			/*!< VDM state update control stream Buffer device virtual address */
	/*** Keep these two together for ukernel block load ***/
	IMG_UINT32			ui32VDMCtxTerminateMode;				/*!< VDM context terminate mode for workaround */
	IMG_UINT32			ui32StateTARNDClip;						/*!< MTE bounding box state for the context terminate */
#endif
#endif /* SGX_FEATURE_VDM_CONTEXT_SWITCH */
#if defined(SGX_SUPPORT_HWPROFILING)
	IMG_DEV_VIRTADDR	sHWProfilingDevVAddr;
#endif /* SGX_SUPPORT_HWPROFILING*/

	IMG_DEV_VIRTADDR	sHWPerfCBDevVAddr;						/*!< Device virtual address of the HW performance circular buffer */
	IMG_DEV_VIRTADDR	sTASigBufferDevVAddr;					/*!< Device virtual address of the TA signatures buffer */
	IMG_DEV_VIRTADDR	s3DSigBufferDevVAddr;					/*!< Device virtual address of the 3D signatures buffer */

	/* KEEP THESE 3 VARIABLES TOGETHER FOR UKERNEL BLOCK LOAD */
	IMG_DEV_VIRTADDR		sTAHWPBDesc;		/*!< Device virtual address of HWPBDesc currently used by the TA, NULL == none */
	IMG_DEV_VIRTADDR		s3DHWPBDesc;		/*!< Device virtual address of HWPBDesc currently used by the 3D, NULL == none */
	IMG_DEV_VIRTADDR		sHostHWPBDesc;		/*!< Device virtual address of HWPBDesc currently used by the Host, NULL == none.\n
													 NOTE: This will always be NULL while the HOST requestor is not used */

#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	IMG_UINT32				ui32LastDirListBaseAlloc;	/*!< Index of the last EUR_CR_BIF_DIR_LIST_BASE register to be allocate */
#endif
	IMG_UINT32				ui32TAPID;						/*!< Process ID of the current or last TA operation */
	IMG_UINT32				ui323DPID;						/*!< Process ID of the current or last 3D operation */
#if defined(SGX_FEATURE_2D_HARDWARE)
	IMG_UINT32				ui322DPID;						/*!< Process ID of the current or last 2D operation */
	IMG_DEV_VIRTADDR		s2DRTData;						/*!< Dummy, required for the macro magic to work */
#endif

#if defined(FIX_HW_BRN_31620) && defined(SGX_FEATURE_MP)
	IMG_UINT32				ui32PDInvalMask[2];				/*!< 64-bit mask of the PD cache lines to be invalidated */
#endif

	IMG_UINT32				ui32IdleCoreRefCount;			/*!< Refcounting to ensure there are as-many resumes as idles and vice-versa */

#if defined(SUPPORT_HW_RECOVERY)
	/* Hardware Recovery state */
	IMG_UINT32				ui32HWRSignaturesInvalid;		/*!< Mask used to indicate which HW recovery signatures are invalid 
																and should not be used for lockup detection on the first sampling */ 
	IMG_UINT32				ui32HWRTimerTicksSinceTA;		/*!< See USE_TIMER_TA_TIMEOUT */
	IMG_UINT32				ui32HWRTASignature;				/*!< value of TA signature checksum last time TA lockup was checked */
	IMG_UINT32				ui32HWRTimerTicksSince3D;		/*!< See USE_TIMER_3D_TIMEOUT */
	IMG_UINT32				ui32HWR3DSignature;				/*!< value of 3D signature checksum last time 3D lockup was checked */
#if defined(SGX_FEATURE_2D_HARDWARE)
	IMG_UINT32				ui32HWRTimerTicksSince2D;		/*!< See USE_TIMER_2D_TIMEOUT */
#if defined(SGX_FEATURE_PTLA)
	IMG_UINT32				ui32HWR2DSignature;				/*!< value of 2D signature checksum last time 2D lockup was checked */
#endif
#endif
#endif

} SGXMK_TA3D_CTL;


#define USE_LOOPBACK_3DMEMFREE				(0x1 << 0)
#define	USE_LOOPBACK_PIXENDRENDER 			(0x1 << 1)
#define USE_LOOPBACK_ISPHALT				(0x1 << 2)
#define USE_LOOPBACK_ISPBREAKPOINT			(0x1 << 3)
#define USE_LOOPBACK_GBLOUTOFMEM			(0x1 << 4)
#define USE_LOOPBACK_MTOUTOFMEM				(0x1 << 5)
#define USE_LOOPBACK_TATERMINATE			(0x1 << 6)
#define USE_LOOPBACK_TAFINISHED				(0x1 << 7)
#define USE_LOOPBACK_KICKPTR_SHIFT			(8)
#define USE_LOOPBACK_KICKPTR				(0xFF << USE_LOOPBACK_KICKPTR_SHIFT) // 0xFF00
#if defined(SGX_FEATURE_2D_HARDWARE)
#define USE_LOOPBACK_2DCOMPLETE				(0x1 << 16)
#endif

#define	USE_LOOPBACK_FINDTA					(0x1 << 17)
#define	USE_LOOPBACK_FIND3D					(0x1 << 18)
#define	USE_LOOPBACK_SPMRENDERFINISHED		(0x1 << 19)
#define USE_LOOPBACK_INITPB					(0x1 << 20)
#if defined(SGX_FEATURE_2D_HARDWARE)
#define USE_LOOPBACK_FIND2D					(0x1 << 21)
#endif

#if defined(SGX_FEATURE_MK_SUPERVISOR)
/*****************************************************************************
 Supervisor Task types
*****************************************************************************/
#define	SGX_UKERNEL_SPRV_WRITE_REG			0x00000001
#if defined(FIX_HW_BRN_29997)
#define	SGX_UKERNEL_SPRV_BRN_29997			0x00000002
#endif
#if defined(FIX_HW_BRN_31474)
#define SGX_UKERNEL_SPRV_BRN_31474			0x00000004
#endif
#if defined(FIX_HW_BRN_31620)
#define SGX_UKERNEL_SPRV_BRN_31620			0x00000008
#endif
#endif

/*!
 *****************************************************************************
 * TA signature registers - index for SGXMKIF_TASIG_BUFFER.ui32Signature
 *****************************************************************************/
enum
{
	#if defined(EUR_CR_TE_CHECKSUM)
		PVRSRV_SGX_TASIG_CLIP_CHECKSUM = 0,
		PVRSRV_SGX_TASIG_MTE_MEM_CHECKSUM,
		PVRSRV_SGX_TASIG_MTE_TE_CHECKSUM,
		PVRSRV_SGX_TASIG_TE_CHECKSUM,
	#else
		PVRSRV_SGX_TASIG_CLIP_SIG1 = 0,
		PVRSRV_SGX_TASIG_MTE_SIG1,
		PVRSRV_SGX_TASIG_MTE_SIG2,
		#if defined(EUR_CR_TE1)
		PVRSRV_SGX_TASIG_TE1,
		PVRSRV_SGX_TASIG_TE2,
		#endif
		#if defined(EUR_CR_VDM_MTE)
		PVRSRV_SGX_TASIG_VDM_MTE,
		#endif
	#endif

	PVRSRV_SGX_TASIG_NUM
};


/*!
 *****************************************************************************
 * 3D signature registers - index for SGXMKIF_3DSIG_BUFFER.ui32Signature
 *****************************************************************************/
enum
{
	#if defined(EUR_CR_ISP_FPU_CHECKSUM)
		PVRSRV_SGX_3DSIG_ISP_FPU_CHECKSUM	= 0,
		
		#if defined(EUR_CR_ISP2_PRECALC_CHECKSUM)
		PVRSRV_SGX_3DSIG_ISP2_PRECALC_CHECKSUM,
		PVRSRV_SGX_3DSIG_ISP2_EDGE_CHECKSUM,
		PVRSRV_SGX_3DSIG_ISP2_TAGWRITE_CHECKSUM,
		PVRSRV_SGX_3DSIG_ISP2_SPAN_CHECKSUM,
		PVRSRV_SGX_3DSIG_PBE_CHECKSUM,
		#else
		PVRSRV_SGX_3DSIG_ISP_PRECALC_CHECKSUM,
		PVRSRV_SGX_3DSIG_ISP_EDGE_CHECKSUM,
		PVRSRV_SGX_3DSIG_ISP_TAGWRITE_CHECKSUM,
		PVRSRV_SGX_3DSIG_ISP_SPAN_CHECKSUM,
		PVRSRV_SGX_3DSIG_PBE_PIXEL_CHECKSUM,
		PVRSRV_SGX_3DSIG_PBE_NONPIXEL_CHECKSUM,
		#endif
	#else
		/* ISP signature registers */
		PVRSRV_SGX_3DSIG_ISP_FPU	= 0,
		
		#if defined(EUR_CR_ISP_PIPE0_SIG1)
		PVRSRV_SGX_3DSIG_ISP_PIPE0_SIG1,
		PVRSRV_SGX_3DSIG_ISP_PIPE0_SIG2,
		PVRSRV_SGX_3DSIG_ISP_PIPE0_SIG3,
		PVRSRV_SGX_3DSIG_ISP_PIPE0_SIG4,
		PVRSRV_SGX_3DSIG_ISP_PIPE1_SIG1,
		PVRSRV_SGX_3DSIG_ISP_PIPE1_SIG2,
		PVRSRV_SGX_3DSIG_ISP_PIPE1_SIG3,
		PVRSRV_SGX_3DSIG_ISP_PIPE1_SIG4,
		#endif
		#if defined(EUR_CR_ISP_SIG1)
		PVRSRV_SGX_3DSIG_ISP_SIG1,
		PVRSRV_SGX_3DSIG_ISP_SIG2,
		PVRSRV_SGX_3DSIG_ISP_SIG3,
		PVRSRV_SGX_3DSIG_ISP_SIG4,
		#endif
		#if defined(EUR_CR_ISP2_ZLS_LOAD_DATA0)
		/* ZLS signature registers */
		PVRSRV_SGX_3DSIG_ISP2_ZLS_LOAD_DATA0,
		PVRSRV_SGX_3DSIG_ISP2_ZLS_LOAD_DATA1,
		PVRSRV_SGX_3DSIG_ISP2_ZLS_LOAD_STENCIL0,
		PVRSRV_SGX_3DSIG_ISP2_ZLS_LOAD_STENCIL1,
		PVRSRV_SGX_3DSIG_ISP2_ZLS_STORE_DATA0,
		PVRSRV_SGX_3DSIG_ISP2_ZLS_STORE_DATA1,
		PVRSRV_SGX_3DSIG_ISP2_ZLS_STORE_STENCIL0,
		PVRSRV_SGX_3DSIG_ISP2_ZLS_STORE_STENCIL1,
		PVRSRV_SGX_3DSIG_ISP2_ZLS_STORE_BIF,
		#endif

		#if defined(EUR_CR_ITR_TAG0)
		/* Iterator TAG signature registers */
		PVRSRV_SGX_3DSIG_ITR_TAG0,
		PVRSRV_SGX_3DSIG_ITR_TAG1,
		#endif

		#if defined(EUR_CR_ITR_TAG00)
		/* Iterator TAG signature registers */
		PVRSRV_SGX_3DSIG_ITR_TAG00,
		PVRSRV_SGX_3DSIG_ITR_TAG01,
		PVRSRV_SGX_3DSIG_ITR_TAG10,
		PVRSRV_SGX_3DSIG_ITR_TAG11,
		#endif

		#if defined(EUR_CR_TF_SIG00)
		/* TF signature registers */
		PVRSRV_SGX_3DSIG_TF_SIG00,
		PVRSRV_SGX_3DSIG_TF_SIG01,
		PVRSRV_SGX_3DSIG_TF_SIG02,
		PVRSRV_SGX_3DSIG_TF_SIG03,
		#endif

		#if defined(EUR_CR_ITR_USE0)
		/* Iterator USE signature registers */
		PVRSRV_SGX_3DSIG_ITR_USE0,
		PVRSRV_SGX_3DSIG_ITR_USE1,
		PVRSRV_SGX_3DSIG_ITR_USE2,
		PVRSRV_SGX_3DSIG_ITR_USE3,
		#endif

		/* PIXELBE signature registers */
		#if defined(EUR_CR_PIXELBE_SIG01)
		PVRSRV_SGX_3DSIG_PIXELBE_SIG01,
		PVRSRV_SGX_3DSIG_PIXELBE_SIG02,
		#endif
		#if defined(EUR_CR_PIXELBE_SIG11)
		PVRSRV_SGX_3DSIG_PIXELBE_SIG11,
		PVRSRV_SGX_3DSIG_PIXELBE_SIG12,
		#endif /* #if defined(EUR_CR_PIXELBE_SIG11) */
	#endif
	
	PVRSRV_SGX_3DSIG_NUM
};

/* Number of TA signature register samples in the buffer */
#define SGXMKIF_TASIG_BUFFER_SIZE 0x40
/* Number of 3D signature register samples in the buffer */
#define SGXMKIF_3DSIG_BUFFER_SIZE 0x40

/*!
 *****************************************************************************
 * TA and 3D signature register buffers
 *****************************************************************************/
typedef struct
{
	IMG_UINT32	ui32NumSignatures;												/*!< Set to PVRSRV_SGX_TASIG_NUM by the host */
	IMG_UINT32	ui32NumSamples;													/*!< Number of samples the ukernel has written */
	IMG_UINT32	ui32RegisterOffset[PVRSRV_SGX_TASIG_NUM];						/*!< List of register offsets */
	IMG_UINT32	ui32Signature[SGXMKIF_TASIG_BUFFER_SIZE][PVRSRV_SGX_TASIG_NUM];	/*!< TA Signature register sample values, written by the ukernel */
} SGXMKIF_TASIG_BUFFER;

typedef struct
{
	IMG_UINT32	ui32NumSignatures;												/*!< Set to PVRSRV_SGX_3DSIG_NUM by the host */
	IMG_UINT32	ui32NumSamples;													/*!< Number of samples the ukernel has written */
	IMG_UINT32	ui32RegisterOffset[PVRSRV_SGX_3DSIG_NUM];						/*!< List of register offsets */
	IMG_UINT32	ui32Signature[SGXMKIF_3DSIG_BUFFER_SIZE][PVRSRV_SGX_3DSIG_NUM];	/*!< 3D Signature register sample values, written by the ukernel */
} SGXMKIF_3DSIG_BUFFER;

#endif /*  __SGX_MKIF_H__ */

/******************************************************************************
 End of file (sgx_mkif.h)
******************************************************************************/


