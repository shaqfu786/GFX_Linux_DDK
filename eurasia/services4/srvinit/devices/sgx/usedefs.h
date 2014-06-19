/******************************************************************************
 Name         : usedefs.h

 Title        : USE code defines

 Author       : Imagination Technologies

 Created      : 12/01/2006

 Copyright    : 2006-2010 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                King's Langley, Hertfordshire, WD4 8LZ, U.K.

 Description  : Defines used by TA3D use code

 Program Type : USE assembly language

 Version      : $Revision: 1.209 $

 Modifications	:

*****************************************************************************/

#ifndef __USEDEFS_H__
#define __USEDEFS_H__

#include "sgx_mkif.h"
#include "sgx_mkif_client.h"
#include "sgx_mkif_km.h"
#include "sgx_ukernel_status_codes.h"
#include "sgxdefs.h"


/*
	These values are used to determine how long it takes before we decide a context store
	is taking too long and we're just going to interrupt the USE instead
*/
#define USE_TIMER_TA_CONTEXT_TIMEOUT	5
#define USE_TIMER_3D_CONTEXT_TIMEOUT	5


#define SGX_LOCKUP_DETECT_SAMPLE_RATE	10

#if defined(DEBUG)
#define SGX_DEADLOCK_MEM_MAX_COUNT	6
#endif

/*****************************************************************************
 Register defines to make code easier to read
*****************************************************************************/
#define PA(x)			pa[DOFFSET(PVRSRV_SGX_EDMPROG_PRIMATTR.x)]
#define SA(x)			sa[DOFFSET(PVRSRV_SGX_EDMPROG_SECATTR.x)]
#define CCB(x)			r[4 + DOFFSET(SGXMKIF_COMMAND.x)]
#if defined(SGX_FEATURE_MP)
#define SLAVE_PA(x)		pa[DOFFSET(PVRSRV_SGX_SLAVE_EDMPROG_PRIMATTR.x)]
#endif

#define R_RTAIdx		r13
#define R_RTData		r14
#define R_EDM_PCLink	r15
#define R_CacheStatus	r22
#define R_UTILS_PCLINK	r23

#define R_HostCtl		SA(sHostCtl)
#define R_TA3DCtl		SA(sTA3DCtl)
#define R_CCBCtl		SA(sCCBCtl)

#define	USE_FALSE		0x0
#define	USE_TRUE		0x1

#define USE_IDLECORE_3D_REQ			0x1
#define USE_IDLECORE_TA_REQ			0x2
#define USE_IDLECORE_TA3D_REQ_MASK	(USE_IDLECORE_3D_REQ | USE_IDLECORE_TA_REQ)
#if defined(FIX_HW_BRN_24549) || defined(FIX_HW_BRN_25615)
#define USE_IDLECORE_USSEONLY_REQ	0x4
#endif

#define USE_DPM_STORE_LOAD_REQ_SHIFT		0
#define USE_DPM_STORE_LOAD_REQ_MASK			(1 << USE_DPM_STORE_LOAD_REQ_SHIFT)
#define USE_DPM_STORE_LOAD_STORE_TYPE_SHIFT	1
#define USE_DPM_STORE_LOAD_STORE_TYPE_MASK	(1 << USE_DPM_STORE_LOAD_STORE_TYPE_SHIFT)

#define	USE_DPM_3D_TA_INDICATOR_BIT_SHIFT	9
#define USE_DPM_3D_TA_INDICATOR_MASK		(1 << USE_DPM_3D_TA_INDICATOR_BIT_SHIFT)
#define USE_3D_BIF_DPM_LSS					(1 << USE_DPM_3D_TA_INDICATOR_BIT_SHIFT)
#define USE_TA_BIF_DPM_LSS					(0 << USE_DPM_3D_TA_INDICATOR_BIT_SHIFT)

#define	USE_CTRL_KILL_MODE_KILL				1
#define	USE_CTRL_KILL_MODE_FLUSH			2

/*****************************************************************************
 Event Handling Defines
*****************************************************************************/
/* 3D events */
#if defined(SGX_FEATURE_MP)
#define SGX_MK_PDS_3DMEMFREE_PA		ui32PDSIn0
#define SGX_MK_PDS_3DMEMFREE		EURASIA_PDS_IR0_EDM_EVENT_MP3DMEMFREE
#define SGX_MK_PDS_EOR_PA			ui32PDSIn0
#define SGX_MK_PDS_EOR				EURASIA_PDS_IR0_EDM_EVENT_MPPIXENDRENDER

#define SGX_MK_PDS_IR0_3DEVENTS		(EURASIA_PDS_IR0_EDM_EVENT_MPPIXENDRENDER | \
									 EURASIA_PDS_IR0_EDM_EVENT_MP3DMEMFREE)

#define SGX_MK_PDS_IR1_3DEVENTS		EURASIA_PDS_IR1_EDM_EVENT_ISPBREAKPOINT
#else
#define SGX_MK_PDS_3DMEMFREE_PA		ui32PDSIn1
#define SGX_MK_PDS_3DMEMFREE		EURASIA_PDS_IR1_EDM_EVENT_3DMEMFREE
#define SGX_MK_PDS_EOR_PA			ui32PDSIn1
#define SGX_MK_PDS_EOR				EURASIA_PDS_IR1_EDM_EVENT_PIXENDRENDER

#define SGX_MK_PDS_IR0_3DEVENTS		0
#define SGX_MK_PDS_IR1_3DEVENTS		(EURASIA_PDS_IR1_EDM_EVENT_PIXENDRENDER | \
									 EURASIA_PDS_IR1_EDM_EVENT_ISPBREAKPOINT | \
									 EURASIA_PDS_IR1_EDM_EVENT_3DMEMFREE)
#endif /* SGX_FEATURE_MP */

/* SPM events */
#if defined(SGX_FEATURE_MP)
#define SGX_MK_PDS_SPM_PA			ui32PDSIn0
#define SGX_MK_PDS_GBLOUTOFMEM		EURASIA_PDS_IR0_EDM_EVENT_MPGBLOUTOFMEM
#define SGX_MK_PDS_MTOUTOFMEM		EURASIA_PDS_IR0_EDM_EVENT_MPMTOUTOFMEM
#define SGX_MK_PDS_TATERMINATE		EURASIA_PDS_IR0_EDM_EVENT_MPTATERMINATE
#define SGX_MK_PDS_MEMTHRESH		EURASIA_PDS_IR0_EDM_EVENT_MPMEMTHRESH
#else
#define SGX_MK_PDS_SPM_PA			ui32PDSIn1
#define SGX_MK_PDS_GBLOUTOFMEM		EURASIA_PDS_IR1_EDM_EVENT_GBLOUTOFMEM
#define SGX_MK_PDS_MTOUTOFMEM		EURASIA_PDS_IR1_EDM_EVENT_MTOUTOFMEM
#define SGX_MK_PDS_TATERMINATE		EURASIA_PDS_IR1_EDM_EVENT_TATERMINATE
#define SGX_MK_PDS_MEMTHRESH		EURASIA_PDS_IR1_EDM_EVENT_MEMTHRESH
#endif /* SGX_FEATURE_MP */
#define SGX_MK_PDS_SPMEVENTS		(SGX_MK_PDS_GBLOUTOFMEM	| \
									 SGX_MK_PDS_MTOUTOFMEM	| \
									 SGX_MK_PDS_TATERMINATE	| \
									 SGX_MK_PDS_MEMTHRESH)

/* TA events */
#if defined(SGX_FEATURE_MP)
#define SGX_MK_PDS_TA_PA			ui32PDSIn0
#define SGX_MK_PDS_TAFINISHED		EURASIA_PDS_IR0_EDM_EVENT_MPTAFINISHED
#else
#define SGX_MK_PDS_TA_PA			ui32PDSIn1
#define SGX_MK_PDS_TAFINISHED		EURASIA_PDS_IR1_EDM_EVENT_TAFINISHED
#endif /* SGX_FEATURE_MP */
#define SGX_MK_PDS_TAEVENTS			SGX_MK_PDS_TAFINISHED

/*****************************************************************************
 Internal TA flags
*****************************************************************************/
#define TA_IFLAGS_TAINPROGRESS					0x00000001
#define TA_IFLAGS_ABORTALL						0x00000002
#define TA_IFLAGS_HALTTA						0x00000004
#define TA_IFLAGS_HWLOCKUP_BIT					3
#define TA_IFLAGS_HWLOCKUP_DETECTED				(0x1 << TA_IFLAGS_HWLOCKUP_BIT)
#define TA_IFLAGS_HWLOCKUP_IDLE					(0x2 << TA_IFLAGS_HWLOCKUP_BIT)
#define TA_IFLAGS_HWLOCKUP_MASK					(0x3 << TA_IFLAGS_HWLOCKUP_BIT)
#define TA_IFLAGS_ABORTINPROGRESS				0x00000020
#define TA_IFLAGS_OOM_PR_BLOCKED_BIT			6
#define TA_IFLAGS_OOM_PR_BLOCKED				(1 << TA_IFLAGS_OOM_PR_BLOCKED_BIT)
#if 1 // SPM_PAGE_INJECTION
#define TA_IFLAGS_SPM_DEADLOCK_BIT				7
#define TA_IFLAGS_SPM_DEADLOCK					(1 << TA_IFLAGS_SPM_DEADLOCK_BIT)
#define TA_IFLAGS_SPM_DEADLOCK_GLOBAL_BIT		8
#define TA_IFLAGS_SPM_DEADLOCK_GLOBAL			(1 << TA_IFLAGS_SPM_DEADLOCK_GLOBAL_BIT)
#define TA_IFLAGS_SPM_DEADLOCK_MEM_BIT			9
#define TA_IFLAGS_SPM_DEADLOCK_MEM				(1 << TA_IFLAGS_SPM_DEADLOCK_MEM_BIT)
#define TA_IFLAGS_SPM_STATE_FREE_ABORT_BIT		10
#define TA_IFLAGS_SPM_STATE_FREE_ABORT			(1 << TA_IFLAGS_SPM_STATE_FREE_ABORT_BIT)
#endif

#define TA_IFLAGS_OUTOFMEM_BIT					11
#define TA_IFLAGS_OUTOFMEM						(1 << TA_IFLAGS_OUTOFMEM_BIT)
#define TA_IFLAGS_ABORT_COMPLETE_BIT			12
#define TA_IFLAGS_ABORT_COMPLETE				(1 << TA_IFLAGS_ABORT_COMPLETE_BIT)

#if defined(FIX_HW_BRN_23055)
#define TA_IFLAGS_INCREASEDTHRESHOLDS_BIT		13
#define TA_IFLAGS_INCREASEDTHRESHOLDS			(1 << TA_IFLAGS_INCREASEDTHRESHOLDS_BIT)
#define TA_IFLAGS_BOTH_REQ_DENIED_BIT			14
#define TA_IFLAGS_BOTH_REQ_DENIED				(1 << TA_IFLAGS_BOTH_REQ_DENIED_BIT)
#define TA_IFLAGS_IDLE_ABORT_BIT				15
#define TA_IFLAGS_IDLE_ABORT					(1 << TA_IFLAGS_IDLE_ABORT_BIT)
#define TA_IFLAGS_FIND_MTE_PAGE_BIT				16
#define TA_IFLAGS_FIND_MTE_PAGE					(1 << TA_IFLAGS_FIND_MTE_PAGE_BIT)
#endif /* defined(FIX_HW_BRN_23055) */
#define TA_IFLAGS_WAITINGTOSTARTCSRENDER_BIT	17
#define TA_IFLAGS_WAITINGTOSTARTCSRENDER		(1 << TA_IFLAGS_WAITINGTOSTARTCSRENDER_BIT)
#define TA_IFLAGS_DEFERREDCONTEXTFREE_BIT		18
#define TA_IFLAGS_DEFERREDCONTEXTFREE			(1 << TA_IFLAGS_DEFERREDCONTEXTFREE_BIT)
#if defined(FIX_HW_BRN_29424) || defined(FIX_HW_BRN_29461) || defined(FIX_HW_BRN_31109) || \
	(!defined(FIX_HW_BRN_23055) && !defined(SGX_FEATURE_MP))
#define TA_IFLAGS_OUTOFMEM_GLOBAL_BIT			19
#define TA_IFLAGS_OUTOFMEM_GLOBAL				(1 << TA_IFLAGS_OUTOFMEM_GLOBAL_BIT)
#endif

#if defined(FIX_HW_BRN_29461)
#define TA_IFLAGS_STORE_DPLIST_BIT				20
#define TA_IFLAGS_STORE_DPLIST					(1 << TA_IFLAGS_STORE_DPLIST_BIT)
#define TA_IFLAGS_DPLIST_STORED_BIT				21
#define TA_IFLAGS_DPLIST_STORED					(1 << TA_IFLAGS_DPLIST_STORED_BIT)
#endif /* FIX_HW_BRN_29461 */

#if !defined(FIX_HW_BRN_23055) && !defined(SGX_FEATURE_MP)
#define TA_IFLAGS_FORCING_GLOBAL_OOM_BIT		22
#define TA_IFLAGS_FORCING_GLOBAL_OOM			(1 << TA_IFLAGS_FORCING_GLOBAL_OOM_BIT)
#endif

#if defined(FIX_HW_BRN_31109)
#define TA_IFLAGS_TIMER_DETECTED_SPM_BIT		23
#define TA_IFLAGS_TIMER_DETECTED_SPM			(1 << TA_IFLAGS_TIMER_DETECTED_SPM_BIT)
#define TA_IFLAGS_TIMER_SPM_CHECKSUM_SAVED_BIT		24
#define TA_IFLAGS_TIMER_SPM_CHECKSUM_SAVED		(1 << TA_IFLAGS_TIMER_SPM_CHECKSUM_SAVED_BIT)
#endif /* FIX_HW_BRN_31109 */

#define	TA_IFLAGS_OPENCL_TASK_DELAY_COUNT		500

#if defined(FIX_HW_BRN_32302) || defined(SUPPORT_SGX_DOUBLE_PIXEL_PARTITIONS)
#define TA_IFLAGS_ZLSTHRESHOLD_LOWERED_BIT		25
#define TA_IFLAGS_ZLSTHRESHOLD_LOWERED			(1 << TA_IFLAGS_ZLSTHRESHOLD_LOWERED_BIT)
#define TA_IFLAGS_ZLSTHRESHOLD_RESTORED_BIT		26	
#define TA_IFLAGS_ZLSTHRESHOLD_RESTORED			(1 << TA_IFLAGS_ZLSTHRESHOLD_RESTORED_BIT)
#endif

/*****************************************************************************
 Internal render flags
*****************************************************************************/
#define RENDER_IFLAGS_RENDERINPROGRESS		0x00000001
#define RENDER_IFLAGS_SPMRENDER				0x00000002
#define RENDER_IFLAGS_TRANSFERRENDER		0x00000004
#define RENDER_IFLAGS_HALTRENDER			0x00000008
#define RENDER_IFLAGS_WAITINGFOREOR			0x00000010
#define RENDER_IFLAGS_WAITINGFOR3DFREE		0x00000020
#define RENDER_IFLAGS_TAWAITINGFORMEM_BIT		6
#define RENDER_IFLAGS_TAWAITINGFORMEM			(1 << RENDER_IFLAGS_TAWAITINGFORMEM_BIT)
#define RENDER_IFLAGS_HALTRECEIVED_BIT			7
#define RENDER_IFLAGS_HALTRECEIVED				(1 << RENDER_IFLAGS_HALTRECEIVED_BIT)
#if 1 // SPM_PAGE_INJECTION
#define RENDER_IFLAGS_SPM_PB_FREE_RENDER_BIT	8
#define RENDER_IFLAGS_SPM_PB_FREE_RENDER		(1 << RENDER_IFLAGS_SPM_PB_FREE_RENDER_BIT)
#endif

#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
/* Render is deferred so context can be freed */
#define RENDER_IFLAGS_DEFERREDCONTEXTFREE_BIT	9
#define RENDER_IFLAGS_DEFERREDCONTEXTFREE		(1 << RENDER_IFLAGS_DEFERREDCONTEXTFREE_BIT)
#endif /* SGX_FEATURE_MULTIPLE_MEM_CONTEXTS */

#define	RENDER_IFLAGS_FORCEHALTRENDER_BIT		10
#define RENDER_IFLAGS_FORCEHALTRENDER			(1 << RENDER_IFLAGS_FORCEHALTRENDER_BIT)

#define RENDER_IFLAGS_WAITINGFORCSEOR_BIT		11
#define RENDER_IFLAGS_WAITINGFORCSEOR			(1 << RENDER_IFLAGS_WAITINGFORCSEOR_BIT)

#define RENDER_IFLAGS_HWLOCKUP_BIT				12
#define RENDER_IFLAGS_HWLOCKUP_DETECTED			(0x1 << RENDER_IFLAGS_HWLOCKUP_BIT)
#define RENDER_IFLAGS_HWLOCKUP_IDLE				(0x2 << RENDER_IFLAGS_HWLOCKUP_BIT)
#define RENDER_IFLAGS_HWLOCKUP_MASK				(0x3 << RENDER_IFLAGS_HWLOCKUP_BIT)

#define RENDER_IFLAGS_PARTIAL_RENDER_OUTOFORDER_BIT			14
#define RENDER_IFLAGS_PARTIAL_RENDER_OUTOFORDER				(1 << RENDER_IFLAGS_PARTIAL_RENDER_OUTOFORDER_BIT)

#if defined(SGX_FEATURE_2D_HARDWARE)
/*****************************************************************************
 Internal 2D flags
*****************************************************************************/
#define TWOD_IFLAGS_2DBLITINPROGRESS		0x00000001
#if defined(FIX_HW_BRN_31272) || defined(FIX_HW_BRN_31780) || defined(FIX_HW_BRN_33920)
#define TWOD_IFLAGS_DUMMYFILL				0x00000002
#endif
#endif

/*****************************************************************************
 Internal active power flags
*****************************************************************************/
/*
	If one of the following flags is set, the corresponding hardware is
	idle waiting for an external event to happen.
*/
#define SGX_UKERNEL_APFLAGS_TA			(1 << 0)
#define SGX_UKERNEL_APFLAGS_3D			(1 << 1)
#if !defined(SGX_FEATURE_2D_HARDWARE)
#define SGX_UKERNEL_APFLAGS_ALL			(SGX_UKERNEL_APFLAGS_TA | \
										 SGX_UKERNEL_APFLAGS_3D)
#else
#define SGX_UKERNEL_APFLAGS_2D			(1 << 2)
#define SGX_UKERNEL_APFLAGS_ALL			(SGX_UKERNEL_APFLAGS_TA | \
										 SGX_UKERNEL_APFLAGS_3D | \
										 SGX_UKERNEL_APFLAGS_2D)
#endif

/*****************************************************************************
 Internal host request flags
*****************************************************************************/
#define SGX_UKERNEL_HOSTREQUEST_POWEROFF			(1UL << 0)
#define SGX_UKERNEL_HOSTREQUEST_IDLE				(1UL << 1)
#define SGX_UKERNEL_HOSTREQUEST_POWER				(SGX_UKERNEL_HOSTREQUEST_IDLE | SGX_UKERNEL_HOSTREQUEST_POWEROFF)

#define SGX_UKERNEL_HOSTREQUEST_CLEANUP_RT			(1UL << 2)
#define SGX_UKERNEL_HOSTREQUEST_CLEANUP_RC			(1UL << 3)
#define SGX_UKERNEL_HOSTREQUEST_CLEANUP_TC			(1UL << 4)
#define SGX_UKERNEL_HOSTREQUEST_CLEANUP_2DC			(1UL << 5)
#define SGX_UKERNEL_HOSTREQUEST_CLEANUP_PB			(1UL << 6)

/*****************************************************************************
 Internal TA Context switch flags
*****************************************************************************/
#define SGX_UKERNEL_TA_CTXSWITCH_INPROGRESS			(1UL << 0)
#define SGX_UKERNEL_TA_CTXSWITCH_NORMAL				(1UL << 1)

/*****************************************************************************
 Internal core count definition
*****************************************************************************/
#define USE_CORE_COUNT_TA_SHIFT		0
#define USE_CORE_COUNT_TA_MASK		0xFFFFU
#define USE_CORE_COUNT_3D_SHIFT		16
#define USE_CORE_COUNT_3D_MASK		0xFFFF0000U

/*****************************************************************************
	Instruction macros to make code easier to read
*****************************************************************************/

#if defined(DEBUG) || defined(EDM_USSE_HWDEBUG)
#define ENTER_UNMATCHABLE_BLOCK nop.tog;
#define LEAVE_UNMATCHABLE_BLOCK nop.tog;
#else
#define ENTER_UNMATCHABLE_BLOCK
#define LEAVE_UNMATCHABLE_BLOCK
#endif /* defined(DEBUG) || defined(EDM_USSE_HWDEBUG) */

#define HSH			#

/*****************************************************************************
 MK_LOAD_STATE
 	Load a state field into a register.

 arguments:	Dest - Destination register for the state
			field - name of the state field
			DataFence - fence for the data load
*****************************************************************************/
#if !defined(SGX_DISABLE_UKERNEL_SECONDARY_STATE)
#define MK_LOAD_STATE(Dest, field, DataFence)										\
	mov	Dest, SA(sMKState.field);
	#if defined(SGX_FEATURE_MP_PLUS)
	/*****************************************************************************
	 MK_LOAD_STATE_CORE_COUNT_[TA,3D]
	 	Load the TA/3D cores enabled state into a register.
	
	 arguments:	Dest - Destination register for the state
				DataFence - fence for the data load
	*****************************************************************************/
#define MK_LOAD_STATE_CORE_COUNT_TA(Dest, DataFence)								\
	mov 	Dest, HSH(USE_CORE_COUNT_TA_MASK);											\
	and 	Dest, Dest, SA(sMKState.ui32CoresEnabled);								\
	shr		Dest, Dest, HSH(USE_CORE_COUNT_TA_SHIFT);
#define MK_LOAD_STATE_CORE_COUNT_3D(Dest, DataFence)								\
	mov 	Dest, HSH(USE_CORE_COUNT_3D_MASK);											\
	and 	Dest, Dest, SA(sMKState.ui32CoresEnabled);								\
	shr		Dest, Dest, HSH(USE_CORE_COUNT_3D_SHIFT);
	#else /* defined(SGX_FEATURE_MP_PLUS) */
#define MK_LOAD_STATE_CORE_COUNT_TA(Dest, DataFence)								\
	MK_LOAD_STATE(Dest, ui32CoresEnabled, DataFence);
#define MK_LOAD_STATE_CORE_COUNT_3D(Dest, DataFence)								\
	MK_LOAD_STATE(Dest, ui32CoresEnabled, DataFence);
	#endif /* defined(SGX_FEATURE_MP_PLUS) */
#else
#define MK_LOAD_STATE(Dest, field, DataFence)										\
	ldad.bpcache	Dest, [R_TA3DCtl, HSH(DOFFSET(SGXMK_TA3D_CTL.sMKState.field))], DataFence;
	#if defined(SGX_FEATURE_MP_PLUS)
		/* FIXME: use extra dword in MK state? */
		#error "Number of TA/3D cores must be equal without ukernel SA state."
	#endif
#endif /* SGX_DISABLE_UKERNEL_SECONDARY_STATE */


/*****************************************************************************
 MK_LOAD_STATE_INDEXED
 	Load a state array element into a register.

 arguments:	Dest - Destination register for the state
			field - name of the state array
			index - array index
			DataFence - fence for the data load
			Temp - a temporary register
*****************************************************************************/
#if !defined(SGX_DISABLE_UKERNEL_SECONDARY_STATE)
#define MK_LOAD_STATE_INDEXED(Dest, field, index, DataFence, Temp)						\
	iaddu32	Temp, index.low, HSH(DOFFSET(PVRSRV_SGX_EDMPROG_SECATTR.sMKState.field[0]));\
	mov	i.l, Temp;																		\
	mov	Dest, sa[i.l];
#else
#define MK_LOAD_STATE_INDEXED(Dest, field, index, DataFence, Temp)						\
	mov		Temp, HSH(OFFSET(SGXMK_TA3D_CTL.sMKState.field[0]));						\
	imae	Temp, index.low, HSH(SIZEOF(SGXMK_TA3D_CTL.sMKState.field[0])), Temp, u32;	\
	or		Temp, Temp, HSH(0x10000);													\
	ldad.bpcache	Dest, [R_TA3DCtl, Temp], DataFence;
#endif /* SGX_DISABLE_UKERNEL_SECONDARY_STATE */


/*****************************************************************************
 MK_WAIT_FOR_STATE_LOAD
 	Wait for data fence associated with a state load.

 arguments:	DataFence - fence for the data load
*****************************************************************************/
#if !defined(SGX_DISABLE_UKERNEL_SECONDARY_STATE)
#define MK_WAIT_FOR_STATE_LOAD(DataFence)
#else
#define MK_WAIT_FOR_STATE_LOAD(DataFence)												\
	wdf		DataFence;
#endif /* SGX_DISABLE_UKERNEL_SECONDARY_STATE */


/*****************************************************************************
 MK_STORE_STATE
 	Store a value into a state field.

 arguments:	field - name of the state field
 			Src - source for the state store (register or immediate)
*****************************************************************************/
#if !defined(SGX_DISABLE_UKERNEL_SECONDARY_STATE)
#define MK_STORE_STATE(field, Src)											\
	mov	SA(sMKState.field), Src;
#else
#define MK_STORE_STATE(field, Src)											\
	stad.bpcache	[R_TA3DCtl, HSH(DOFFSET(SGXMK_TA3D_CTL.sMKState.field))], Src;
#endif /* SGX_DISABLE_UKERNEL_SECONDARY_STATE */

/*****************************************************************************
 MK_STORE_STATE_INDEXED
 	Store a value into a state array element.

 arguments:	field - name of the state field
			index - array index
 			Src - source for the state store (register or immediate)
			Temp - a temporary register
*****************************************************************************/
#if !defined(SGX_DISABLE_UKERNEL_SECONDARY_STATE)
#define MK_STORE_STATE_INDEXED(field, index, Src, Temp)									\
	iaddu32	Temp, index.low, HSH(DOFFSET(PVRSRV_SGX_EDMPROG_SECATTR.sMKState.field[0]));\
	mov	i.l, Temp;																		\
	mov	sa[i.l], Src;
#else
#define MK_STORE_STATE_INDEXED(field, index, Src, Temp)									\
	mov		Temp, HSH(OFFSET(SGXMK_TA3D_CTL.sMKState.field[0]));						\
	imae	Temp, index.low, HSH(SIZEOF(SGXMK_TA3D_CTL.sMKState.field[0])), Temp, u32;	\
	or		Temp, Temp, HSH(0x10000);													\
	stad.bpcache	[R_TA3DCtl, Temp], Src;
#endif /* SGX_DISABLE_UKERNEL_SECONDARY_STATE */


/*****************************************************************************
 MK_WAIT_FOR_STATE_STORE
 	Wait for data fence associated with a state store.

 arguments:	DataFence - fence for the data store
*****************************************************************************/
#if !defined(SGX_DISABLE_UKERNEL_SECONDARY_STATE)
#define MK_WAIT_FOR_STATE_STORE(DataFence)
#else
#define MK_WAIT_FOR_STATE_STORE(DataFence)												\
	idf	DataFence, st;																	\
	wdf	DataFence;
#endif /* SGX_DISABLE_UKERNEL_SECONDARY_STATE */

/*****************************************************************************
 MK_STORE_MEM
 	Store a value to memory

 arguments:	DataFence - fence for the data store
*****************************************************************************/
#if defined(SGX_FEATURE_WRITEBACK_DCU) && !defined(SGX_BYPASS_DCU) && !defined(SUPPORT_SGX_UKERNEL_DCU_BYPASS)
	#define MK_MEM_ACCESS(Inst)	Inst
#else
	#define MK_MEM_ACCESS(Inst)	Inst.bpcache
#endif

#if defined(SGX_FEATURE_WRITEBACK_DCU)
	#if	 !defined(SGX_BYPASS_DCU) && !defined(SUPPORT_SGX_UKERNEL_DCU_BYPASS)
		#define MK_MEM_ACCESS_CACHED(Inst)	Inst
	#else
		#define MK_MEM_ACCESS_CACHED(Inst)	Inst.bpcache
	#endif
#else
	#define MK_MEM_ACCESS_CACHED(Inst)	Inst
#endif

#define MK_MEM_ACCESS_BPCACHE(Inst)	Inst.bpcache


/*****************************************************************************
 PVRSRV_SGXUTILS_CALL
 	Call a utility function.

 arguments:	utils_fn - name of a label within sgx_utils.asm
 temps:		R_UTILS_PCLINK
*****************************************************************************/
#define PVRSRV_SGXUTILS_CALL(utils_fn)										\
		mov		R_UTILS_PCLINK, pclink;										\
		bal		utils_fn;													\
		mov		pclink, R_UTILS_PCLINK;

/*****************************************************************************
 MK_TRACE[_REG]
 	Write out an entry in the microkernel trace circular buffer.

 variants: _REG - Write a register value instead of a contant status code
 arguments:	code - MKTC_*
			family_mask - matched against ui32MKTraceProfile - see MKTF_*
			reg - register whose value is to be written
 temps:		r16, r17, r18, r19, r20, r21
*****************************************************************************/
#define MK_TRACE(code, family_mask)								\
			MK_TRACE_COMMON(HSH(code), HSH(family_mask))
#define MK_TRACE_REG(reg, family_mask)							\
			MK_TRACE_COMMON(reg, HSH(family_mask))

#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG)
#define MK_TRACE_COMMON(code, family_mask)						\
		mov		r16, code;										\
		mov		r17, family_mask;								\
		PVRSRV_SGXUTILS_CALL(MKTrace);
#else
#if defined(DEBUG) || defined(EDM_USSE_HWDEBUG)
#define MK_TRACE_COMMON(code, family_mask)						\
		mov		r16, code;										\
		mov		r17, family_mask;
#else
#define MK_TRACE_COMMON(code, family_mask)
#endif /* defined(DEBUG) || defined(EDM_USSE_HWDEBUG) */
#endif /* PVRSRV_USSE_EDM_STATUS_DEBUG */


#if defined(SGX_FEATURE_USE_UNLIMITED_PHASES)
/* No following phase. */
#define	UKERNEL_PROGRAM_PHAS()			\
	phas	HSH(0), HSH(0), pixel, end, parallel;
#else
#define	UKERNEL_PROGRAM_PHAS()
#endif /* SGX_FEATURE_USE_UNLIMITED_PHASES */

#if defined(PDUMP)
/*
	Not necessary, but prevents an uninitialised register error from the
	simulator when PVRSRV_SGXUTILS_CALL is called.
*/
#define	UKERNEL_PROGRAM_INIT_PDUMP()			\
	mov			pclink, HSH(0);					\
	mov			r0, HSH(0);						\
	and.testnz	p0, r0, HSH(0);					\
	and.testnz	p1, r0, HSH(0);					\
	and.testnz	p2, r0, HSH(0);					\
	and.testnz	p3, r0, HSH(0);
#else
#define	UKERNEL_PROGRAM_INIT_PDUMP()
#endif /* PDUMP */

#if defined(FIX_HW_BRN_29104)
#define	UKERNEL_PROGRAM_INIT_PHAS()	UKERNEL_PROGRAM_PHAS ()
#else
#define	UKERNEL_PROGRAM_INIT_PHAS()
#endif

/*
	UKERNEL_PROGRAM_INIT_COMMON
		Common initialiser code for all ukernel primary programs.
*/
#define UKERNEL_PROGRAM_INIT_COMMON()													\
	/*																					\
		Set macro operations to only increment src2										\
	*/																					\
	smlsi	HSH(0),HSH(0),HSH(0),HSH(1),HSH(0),HSH(0),HSH(0),HSH(0),HSH(0),HSH(0),HSH(0);\
																						\
	UKERNEL_PROGRAM_INIT_PHAS();														\
																						\
	UKERNEL_PROGRAM_INIT_PDUMP();														\
																						\
	/*																					\
		Clear the EDM PC link address so we can use zero/non-zero tests to decide		\
		whether a branch or terminate should be performed at the end of subroutines		\
	*/																					\
	mov		R_EDM_PCLink, HSH(0);														\
																						\
	/*																					\
		Clear the Cache Status word, so we can keep track of the caches					\
		which have been invalidated														\
	*/																					\
	mov		R_CacheStatus, HSH(0);

/*
	UKERNEL_PROGRAM_END
		Common code for ending all ukernel primary programs.
		slc_flush - non-zero to flush write-back SLC (if present)
		mktc - trace code, MKTC_*
		mktf_mask - trace family mask, MKTF_*
*/
#define UKERNEL_PROGRAM_END(slc_flush, mktc, mktf_mask)	\
	MK_TRACE(mktc, mktf_mask);							\
	mov	r16, slc_flush;									\
	PVRSRV_SGXUTILS_CALL(CommonProgramEnd);
	

/*****************************************************************************
 MK_ASSERT_FAIL
 	Raise an assertion failure error when an invalid state is detected.
	Causes the host to reset the microkernel.

 arguments:	code - unique code to define the failure condition
 temps:		N/A
*****************************************************************************/
#define	MK_ASSERT_FAIL(code)		\
	MK_TRACE(MKTC_ASSERT_FAIL, ~0);	\
	MK_TRACE(code, ~0);				\
	mov	r16, HSH(code);				\
	MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, HSH(DOFFSET(SGXMKIF_HOST_CTL.ui32AssertFail))], r16;	\
	idf		drc0, st;				\
	wdf		drc0;					\
	ba	WaitForReset;


#define CACHE_INVAL_PDS_INV0	(1 << 0)
#define CACHE_INVAL_PDS_INV1	(1 << 1)
#define CACHE_INVAL_PDS_INV_CSC	(1 << 2)
#define	CACHE_INVAL_MADD		(1 << 3)
#define CACHE_INVAL_DCU			(1 << 4)
#define CACHE_INVAL_TCU			(1 << 5)
#define CACHE_INVAL_USE			(1 << 6)

#if defined(FIX_HW_BRN_31474)
#if defined(SGX_FEATURE_MP)
#define INVALIDATE_PDS_CODE_CACHE()						\
	mov		r24, HSH(SGX_UKERNEL_SPRV_BRN_31474);		\
	PVRSRV_SGXUTILS_CALL(JumpToSupervisor);
		
#define INVALIDATE_PDS_CODE_CACHE_UTILS()				\
	mov		r24, HSH(SGX_UKERNEL_SPRV_BRN_31474);		\
	mov		r28, pclink;								\
	bal		JumpToSupervisor;							\
	mov		pclink, r28;
#else /* SGX_FEATURE_MP */
#define INVALIDATE_PDS_CODE_CACHE()									\
	mov		r24, HSH(EUR_CR_EVENT_KICK3 >> 2);\
	str		r24, HSH(EUR_CR_EVENT_KICK3_NOW_MASK);

#define INVALIDATE_PDS_CODE_CACHE_UTILS()		INVALIDATE_PDS_CODE_CACHE()
#endif
#else
#define INVALIDATE_PDS_CODE_CACHE()									\
	str		HSH(EUR_CR_PDS_INV_CSC >> 2), HSH(EUR_CR_PDS_INV_CSC_KICK_MASK);

#define INVALIDATE_PDS_CODE_CACHE_UTILS()		INVALIDATE_PDS_CODE_CACHE()
#endif

#if defined(FIX_HW_BRN_25615)
#define INVALIDATE_USE_CACHE(label, pred, TmpReg, requestor)						\
	and.testnz	pred, R_CacheStatus, HSH(CACHE_INVAL_USE);							\
	pred	ba label##NoUseInval;													\
	{;																				\
		and.testnz	pred, requestor, requestor;										\
		pred	mov	r16, HSH(USE_IDLECORE_TA_REQ | USE_IDLECORE_USSEONLY_REQ);		\
		!pred	mov	r16, HSH(USE_IDLECORE_3D_REQ | USE_IDLECORE_USSEONLY_REQ);		\
		PVRSRV_SGXUTILS_CALL(IdleCore);												\
		/* The next three instructions must be done in a cache line */				\
		.align 16;																	\
		str		HSH(EUR_CR_USE_CACHE >> 2), HSH(EUR_CR_USE_CACHE_INVALIDATE_MASK);	\
		ldr		TmpReg, HSH(EUR_CR_USE_CACHE >> 2), drc0;							\
		wdf		drc0;																\
		RESUME(r16);																	\
		or		R_CacheStatus, R_CacheStatus, HSH(CACHE_INVAL_USE);					\
	};																				\
	label##NoUseInval:;
#else
#define INVALIDATE_USE_CACHE(label, pred, TmpReg, requestor)						\
	and.testnz	pred, R_CacheStatus, HSH(CACHE_INVAL_USE);							\
	pred	ba label##NoUseInval;													\
	{;																				\
		str		HSH(EUR_CR_USE_CACHE >> 2), HSH(EUR_CR_USE_CACHE_INVALIDATE_MASK);	\
		or		R_CacheStatus, R_CacheStatus, HSH(CACHE_INVAL_USE);					\
	};																				\
	label##NoUseInval:
#endif

#if defined(FIX_HW_BRN_33181) || defined(FIX_HW_BRN_23378)
#define IDLETA()				\
		mov	r16, HSH(USE_IDLECORE_TA_REQ);		\
		PVRSRV_SGXUTILS_CALL(IdleCore);
#define RESUME_TA(reg1)		RESUME(reg1);
#else
#define IDLETA() 
#define RESUME_TA(reg1)
#endif

#if defined(FIX_HW_BRN_23378)
#define IDLE3D()				\
		mov	r16, HSH(USE_IDLECORE_3D_REQ);		\
		PVRSRV_SGXUTILS_CALL(IdleCore);

#define RESUME_3D(reg1)		RESUME(reg1);
#else
#define IDLE3D()
#define RESUME_3D(reg1)
#endif


#define RESUME(reg1)		\
	mov reg1, r16;\
	PVRSRV_SGXUTILS_CALL(Resume);\
	mov r16, reg1;\



#if defined(EUR_CR_DPM_STATE_TABLE_CONTEXT0_BASE)
#define SETUPSTATETABLEBASE(Label, Pred, TableBase, ContextID)				\
		and.testnz	Pred, ContextID, ContextID;                         	\
		Pred ba	Label##_SetupContext1StateBase;                         	\
		{;                                                              	\
			str HSH(EUR_CR_DPM_STATE_TABLE_CONTEXT0_BASE >> 2), TableBase;	\
			ba	Label##_StateTableBaseReady;                            	\
		};                                                              	\
		Label##_SetupContext1StateBase:;                                	\
		{;                                                              	\
			str HSH(EUR_CR_DPM_STATE_TABLE_CONTEXT1_BASE >> 2), TableBase;	\
		};                                                              	\
		Label##_StateTableBaseReady:;

#define READSTATETABLEBASE(Label, Pred, TableBase, ContextID, DataFence)				\
		and.testnz	Pred, ContextID, ContextID;                         				\
		Pred ba	Label##_ReadContext1StateBase;                         					\
		{;                                                              				\
			ldr TableBase, HSH(EUR_CR_DPM_STATE_TABLE_CONTEXT0_BASE >> 2), DataFence;	\
			ba	Label##_WaitForTableBase;                            					\
		};                                                              				\
		Label##_ReadContext1StateBase:;                                					\
		{;                                                              				\
			ldr TableBase, HSH(EUR_CR_DPM_STATE_TABLE_CONTEXT1_BASE >> 2), DataFence;	\
		};                                                              				\
		Label##_WaitForTableBase:;														\
		wdf		DataFence;
		
#else
#define READSTATETABLEBASE(Label, Pred, TableBase, ContextID, DataFence)		\
		ldr 	TableBase, HSH(EUR_CR_DPM_STATE_TABLE_BASE >> 2), DataFence;	\
		wdf		DataFence;

#define SETUPSTATETABLEBASE(Label, Pred, TableBase, ContextID)				\
		str HSH(EUR_CR_DPM_STATE_TABLE_BASE >> 2), TableBase;
#endif
/*****************************************************************************
 LOADTACONTEXT_SPM
 	Load DPM context for the TA.

 arguments:	context - PVR3DIF4_RTDATA to set
			context_id -
 temps:		r16, r17, r18, r19
*****************************************************************************/
#define LOADTACONTEXT_SPM(rtdata, context_id)												\
		MK_TRACE(MKTC_LOADTACONTEXT_START, MKTF_SCH | MKTF_TA | MKTF_CSW);				\
		mov		r16, rtdata;															\
		mov		r17, context_id;														\
		mov		r18, HSH(USE_TA_BIF_DPM_LSS);											\
		PVRSRV_SGXUTILS_CALL(LoadDPMContext);											\
		MK_TRACE(MKTC_LOADTACONTEXT_END, MKTF_SCH | MKTF_TA | MKTF_CSW);


/*****************************************************************************
 STORETACONTEXT_SPM
 	Store DPM context for the TA.

 arguments:	context - PVR3DIF4_RTDATA to set
			context_id -
 temps:		r16, r17, r18, r19
*****************************************************************************/
#define STORETACONTEXT_SPM(rtdata, context_id)												\
		MK_TRACE(MKTC_STORETACONTEXT_START, MKTF_SCH | MKTF_TA | MKTF_CSW);				\
		mov		r16, rtdata;															\
		mov		r17, context_id;														\
		mov		r18, HSH(USE_TA_BIF_DPM_LSS);											\
		PVRSRV_SGXUTILS_CALL(StoreDPMContext);											\
		MK_TRACE(MKTC_STORETACONTEXT_END, MKTF_SCH | MKTF_TA | MKTF_CSW);

/*****************************************************************************
 LOAD3DCONTEXT_SPM
 	Load DPM context for the 3D.

 arguments:	context - PVR3DIF4_RTDATA to set
			context_id -
 temps:		r16, r17, r18, r19
*****************************************************************************/
#define LOAD3DCONTEXT_SPM(rtdata, context_id)												\
		MK_TRACE(MKTC_LOAD3DCONTEXT_START, MKTF_SCH | MKTF_3D | MKTF_CSW);				\
		mov		r16, rtdata;															\
		mov		r17, context_id;														\
		mov		r18, HSH(USE_3D_BIF_DPM_LSS);											\
		PVRSRV_SGXUTILS_CALL(LoadDPMContext);											\
		MK_TRACE(MKTC_LOAD3DCONTEXT_END, MKTF_SCH | MKTF_3D | MKTF_CSW);
/*****************************************************************************
 STORE3DCONTEXT_SPM
 	Store DPM context for the 3D.

 arguments:	context - PVR3DIF4_RTDATA to set
			context_id -
 temps:		r16, r17, r18, r19
*****************************************************************************/
#define STORE3DCONTEXT_SPM(rtdata, context_id)												\
		MK_TRACE(MKTC_STORE3DCONTEXT_START, MKTF_SCH | MKTF_3D | MKTF_CSW);				\
		mov		r16, rtdata;															\
		mov		r17, context_id;														\
		mov		r18, HSH(USE_3D_BIF_DPM_LSS);											\
		PVRSRV_SGXUTILS_CALL(StoreDPMContext);											\
		MK_TRACE(MKTC_STORE3DCONTEXT_END, MKTF_SCH | MKTF_3D | MKTF_CSW);

/*****************************************************************************
 LOADTACONTEXT
 	Load DPM context for the TA.

 arguments:	context - PVR3DIF4_RTDATA to set
			context_id -
 temps:		r16, r17, r18, r19
*****************************************************************************/
#define LOADTACONTEXT(rtdata, context_id)												\
		MK_TRACE(MKTC_LOADTACONTEXT_START, MKTF_SCH | MKTF_TA | MKTF_CSW);				\
		IDLE3D();																		\
		mov		r16, rtdata;															\
		mov		r17, context_id;														\
		mov		r18, HSH(USE_TA_BIF_DPM_LSS);											\
		PVRSRV_SGXUTILS_CALL(LoadDPMContext);											\
		RESUME_3D(r16);																	\
		MK_TRACE(MKTC_LOADTACONTEXT_END, MKTF_SCH | MKTF_TA | MKTF_CSW);


/*****************************************************************************
 STORETACONTEXT
 	Store DPM context for the TA.

 arguments:	context - PVR3DIF4_RTDATA to set
			context_id -
 temps:		r16, r17, r18, r19
*****************************************************************************/
#define STORETACONTEXT(rtdata, context_id)												\
		MK_TRACE(MKTC_STORETACONTEXT_START, MKTF_SCH | MKTF_TA | MKTF_CSW);				\
		IDLE3D();																		\
		mov		r16, rtdata;															\
		mov		r17, context_id;														\
		mov		r18, HSH(USE_TA_BIF_DPM_LSS);											\
		PVRSRV_SGXUTILS_CALL(StoreDPMContext);											\
		RESUME_3D(r16);																	\
		MK_TRACE(MKTC_STORETACONTEXT_END, MKTF_SCH | MKTF_TA | MKTF_CSW);

/*****************************************************************************
 LOAD3DCONTEXT
 	Load DPM context for the 3D.

 arguments:	context - PVR3DIF4_RTDATA to set
			context_id -
 temps:		r16, r17, r18, r19
*****************************************************************************/
#define LOAD3DCONTEXT(rtdata, context_id)												\
		MK_TRACE(MKTC_LOAD3DCONTEXT_START, MKTF_SCH | MKTF_3D | MKTF_CSW);				\
		IDLETA();																		\
		mov		r16, rtdata;															\
		mov		r17, context_id;														\
		mov		r18, HSH(USE_3D_BIF_DPM_LSS);											\
		PVRSRV_SGXUTILS_CALL(LoadDPMContext);											\
		RESUME_TA(r16);																	\
		MK_TRACE(MKTC_LOAD3DCONTEXT_END, MKTF_SCH | MKTF_3D | MKTF_CSW);
/*****************************************************************************
 STORE3DCONTEXT
 	Store DPM context for the 3D.

 arguments:	context - PVR3DIF4_RTDATA to set
			context_id -
 temps:		r16, r17, r18, r19
*****************************************************************************/
#define STORE3DCONTEXT(rtdata, context_id)												\
		MK_TRACE(MKTC_STORE3DCONTEXT_START, MKTF_SCH | MKTF_3D | MKTF_CSW);				\
		IDLETA();																		\
		mov		r16, rtdata;															\
		mov		r17, context_id;														\
		mov		r18, HSH(USE_3D_BIF_DPM_LSS);											\
		PVRSRV_SGXUTILS_CALL(StoreDPMContext);											\
		RESUME_TA(r16);																	\
		MK_TRACE(MKTC_STORE3DCONTEXT_END, MKTF_SCH | MKTF_3D | MKTF_CSW);

#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
/*****************************************************************************
 SWITCHEDMTOTA, SWITCHEDMTO3D, SWITCHEDMBACK
 	private helper macros to conditionally compile calls to SwitchEDMBIFBank

 arguments:	hwpbdesc - PVR3DIF4_HWPBDESC to load
 temps:		r16
*****************************************************************************/
#define SWITCHEDMTOTA()																\
		mov		r16, HSH(0);														\
		PVRSRV_SGXUTILS_CALL(SwitchEDMBIFBank);
#define SWITCHEDMTO3D()																\
		mov		r16, HSH(1);														\
		PVRSRV_SGXUTILS_CALL(SwitchEDMBIFBank);
#define SWITCHEDMBACK()																\
		mov		r16, HSH(2);														\
		PVRSRV_SGXUTILS_CALL(SwitchEDMBIFBank);
#else
#define SWITCHEDMTOTA()
#define SWITCHEDMTO3D()
#define SWITCHEDMBACK()
#endif

#if defined(FIX_HW_BRN_32302) || defined(SUPPORT_SGX_DOUBLE_PIXEL_PARTITIONS)
/*****************************************************************************
 LOADTAPB
 	Load PB for the TA.

 arguments:	hwpbdesc - PVR3DIF4_HWPBDESC to load
 temps:		r16, r17, r18, r19, r20, r21
*****************************************************************************/
#define LOADTAPB(hwpbdesc, reg)															\
		MK_TRACE(MKTC_LOADTAPB_START, MKTF_SCH | MKTF_TA | MKTF_PB);					\
		SWITCHEDMTOTA();																\
		mov		r16, hwpbdesc;														\
		PVRSRV_SGXUTILS_CALL(SetupDPMPagetable);										\
		MK_TRACE(MKTC_LOADTAPB_PAGETABLE_DONE, MKTF_TA | MKTF_PB);					\
		mov		r16, hwpbdesc;														\
		mov		r17, reg; \
		PVRSRV_SGXUTILS_CALL(LoadTAPB);												\
		MK_TRACE(MKTC_LOADTAPB_END, MKTF_SCH | MKTF_TA | MKTF_PB);					\
		SWITCHEDMBACK();
#else
/*****************************************************************************
 LOADTAPB
 	Load PB for the TA.

 arguments:	hwpbdesc - PVR3DIF4_HWPBDESC to load
 temps:		r16, r17, r18, r19, r20, r21
*****************************************************************************/
#define LOADTAPB(hwpbdesc)															\
		MK_TRACE(MKTC_LOADTAPB_START, MKTF_SCH | MKTF_TA | MKTF_PB);					\
		SWITCHEDMTOTA();																\
		mov		r16, hwpbdesc;														\
		PVRSRV_SGXUTILS_CALL(SetupDPMPagetable);										\
		MK_TRACE(MKTC_LOADTAPB_PAGETABLE_DONE, MKTF_TA | MKTF_PB);					\
		mov		r16, hwpbdesc;														\
		PVRSRV_SGXUTILS_CALL(LoadTAPB);												\
		MK_TRACE(MKTC_LOADTAPB_END, MKTF_SCH | MKTF_TA | MKTF_PB);					\
		SWITCHEDMBACK();
#endif
/*****************************************************************************
 STORETAPB
 	Store PB for the TA.

 temps:		r16, r17, r18
*****************************************************************************/
#define STORETAPB()																\
		MK_TRACE(MKTC_STORETAPB_START, MKTF_SCH | MKTF_TA | MKTF_PB);			\
		PVRSRV_SGXUTILS_CALL(StoreTAPB);											\
		MK_TRACE(MKTC_STORETAPB_END, MKTF_SCH | MKTF_TA | MKTF_PB);

/*****************************************************************************
 LOAD3DPB
 	Load PB for the 3D.

 arguments:	hwpbdesc - PVR3DIF4_HWPBDESC to load
 temps:		r16, r17, r18, r19, r20, r21
*****************************************************************************/
#define LOAD3DPB(hwpbdesc)															\
		MK_TRACE(MKTC_LOAD3DPB_START, MKTF_SCH | MKTF_3D | MKTF_PB);					\
		mov		r16, hwpbdesc;														\
		PVRSRV_SGXUTILS_CALL(Load3DPB);												\
		MK_TRACE(MKTC_LOAD3DPB_END, MKTF_SCH | MKTF_3D | MKTF_PB);

/*****************************************************************************
 STORE3DPB
 	Store PB for the 3D.

 temps:		r16, r17, r18
*****************************************************************************/
#define STORE3DPB()																\
		MK_TRACE(MKTC_STORE3DPB_START, MKTF_SCH | MKTF_3D | MKTF_PB);			\
		PVRSRV_SGXUTILS_CALL(Store3DPB);											\
		MK_TRACE(MKTC_STORE3DPB_END, MKTF_SCH | MKTF_3D | MKTF_PB);


/*****************************************************************************
 CHECKPBSHARING
 	Checks to see if there is PB sharing between TA/3D/Host

 inputs:
 temps:		r17, r18, r19, r20
*****************************************************************************/
#if defined(EUR_CR_DPM_CONTEXT_PB_BASE)
#define CHECKPBSHARING()																					\
	MK_MEM_ACCESS_BPCACHE(ldad.f3)	r17, [R_TA3DCtl, HSH(DOFFSET(SGXMK_TA3D_CTL.sTAHWPBDesc))-1], drc0;				\
	mov 			r20, HSH(0);																			\
	wdf 			drc0;																					\
	xor.testnz 		p0, r17, r19;																			\
	p0 or 			r20, r20, HSH(0x1);																		\
	xor.testnz 		p0, r17, r18;																			\
	p0 or 			r20, r20, HSH(0x2);																		\
	xor.testnz 		p0, r18, r19;																			\
	p0 or 			r20, r20, HSH(0x4);																		\
	str 	HSH(EUR_CR_DPM_CONTEXT_PB_BASE >> 2), r20;
#else
#define CHECKPBSHARING()
#endif

#if defined(SGX_FEATURE_ISP_CONTEXT_SWITCH)
#define START3DCONTEXTSWITCH(label)		\
	MK_LOAD_STATE(r17, ui32IRenderFlags, drc0);													\
	MK_WAIT_FOR_STATE_LOAD(drc0);																\
	and.testnz	p3, r17, HSH(RENDER_IFLAGS_TRANSFERRENDER | RENDER_IFLAGS_HALTRENDER | RENDER_IFLAGS_SPMRENDER);	\
	p3	ba	label##_NoContextSwitch;															\
	{;																							\
		and.testz	p3, r17, HSH(RENDER_IFLAGS_WAITINGFOREOR);									\
		p3	ba	label##_NoContextSwitch;														\
		{;																						\
			MK_TRACE(MKTC_3DCONTEXT_SWITCH, MKTF_SCH | MKTF_3D | MKTF_CSW);						\
			PVRSRV_SGXUTILS_CALL(Start3DContextSwitch);											\
			MK_TRACE(MKTC_3DCONTEXT_SWITCH_END, MKTF_SCH | MKTF_3D | MKTF_CSW);					\
		};																						\
	};																							\
	label##_NoContextSwitch:;
#else
	#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH)
#define START3DCONTEXTSWITCH(label)		\
	MK_LOAD_STATE(r16, ui32IRenderFlags, drc0);																		\
	MK_WAIT_FOR_STATE_LOAD(drc0);																					\
	and.testnz	p2, r16, HSH(RENDER_IFLAGS_TRANSFERRENDER | RENDER_IFLAGS_HALTRENDER | RENDER_IFLAGS_SPMRENDER);	\
	p2	ba	label##_NoContextSwitch;																				\
	{;																												\
		or		r16, r16, HSH(RENDER_IFLAGS_HALTRENDER);															\
		MK_STORE_STATE(ui32IRenderFlags, r16);																		\
		MK_TRACE(MKTC_3DCONTEXT_SWITCH, MKTF_SCH | MKTF_3D | MKTF_CSW);												\
		idf		drc0, st;																							\
		wdf		drc0;																								\
		MK_TRACE(MKTC_3DCONTEXT_SWITCH_END, MKTF_SCH | MKTF_3D | MKTF_CSW);											\
	};																												\
	label##_NoContextSwitch:;
	#else
	#define START3DCONTEXTSWITCH(label)
	#endif
#endif

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	#if defined(SGX_FEATURE_MASTER_VDM_CONTEXT_SWITCH) && !defined(SGX_FEATURE_SLAVE_VDM_CONTEXT_SWITCH)
		#define SGX_MK_VDM_CTX_STORE_STATUS_PROCESS_MASK	EUR_CR_MASTER_VDM_CONTEXT_STORE_STATUS_PROCESS_MASK
		#define SGX_MK_VDM_CTX_STORE_STATUS_NA_MASK			EUR_CR_MASTER_VDM_CONTEXT_STORE_STATUS_NA_MASK
		#define SGX_MK_VDM_CTX_STORE_STATUS_COMPLETE_MASK	EUR_CR_MASTER_VDM_CONTEXT_STORE_STATUS_COMPLETE_MASK
	#else
		#define SGX_MK_VDM_CTX_STORE_STATUS_PROCESS_MASK	EUR_CR_VDM_CONTEXT_STORE_STATUS_PROCESS_MASK
		#define SGX_MK_VDM_CTX_STORE_STATUS_NA_MASK			EUR_CR_VDM_CONTEXT_STORE_STATUS_NA_MASK
		#define SGX_MK_VDM_CTX_STORE_STATUS_COMPLETE_MASK	EUR_CR_VDM_CONTEXT_STORE_STATUS_COMPLETE_MASK
	#endif

#define STARTTACONTEXTSWITCH(label)													\
	MK_TRACE(MKTC_TACONTEXT_SWITCH, MKTF_SCH | MKTF_TA | MKTF_CSW);					\
	PVRSRV_SGXUTILS_CALL(StartTAContextSwitch);
#else
	#define STARTTACONTEXTSWITCH(label)
#endif

/*****************************************************************************
 EMITLOOPBACK
 	Emits a loopback task.

 arguments:
 temps:		r16
*****************************************************************************/
#define EMITLOOPBACK(reg)							\
		mov		r16, reg;							\
		PVRSRV_SGXUTILS_CALL(EmitLoopBack);

/*****************************************************************************
 OUTOFMEM
 	Emits the loopback task to handle the OUTOFMEM GBL and MT Events.

 arguments:
 temps:		r16
*****************************************************************************/
#define OUTOFMEM()																\
		mov		r16, HSH(USE_LOOPBACK_GBLOUTOFMEM | USE_LOOPBACK_MTOUTOFMEM);	\
		PVRSRV_SGXUTILS_CALL(EmitLoopBack);

/*****************************************************************************
 TATERMINATE
 	Emits the loopback task to handle the TATERMINATE Event.

 arguments:
 temps:		r16
*****************************************************************************/
#define TATERMINATE()											\
		mov		r16, HSH(USE_LOOPBACK_TATERMINATE);				\
		PVRSRV_SGXUTILS_CALL(EmitLoopBack);

/*****************************************************************************
 TAFINISHED
 	Emits the loopback task to handle the TAFINISHED Event.

 arguments:
 temps:		r16
*****************************************************************************/
#define TAFINISHED()											\
		mov		r16, HSH(USE_LOOPBACK_TAFINISHED);				\
		PVRSRV_SGXUTILS_CALL(EmitLoopBack);

#if defined(SGX_FEATURE_2D_HARDWARE)
/*****************************************************************************
 FIND2D
 	Emits the loopback task to call the Find2D routine.

 arguments:
 temps:		r16
*****************************************************************************/
#define FIND2D()							\
		PVRSRV_SGXUTILS_CALL(Check2DQueue);
#endif

/*****************************************************************************
 FINDTA
 	Emits the loopback task to call the FindTA routine.

 arguments:
 temps:		r16
*****************************************************************************/
#define FINDTA()							\
		PVRSRV_SGXUTILS_CALL(CheckTAQueue);

/*****************************************************************************
 FIND3D
 	Emits the loopback task to call the FindRender & FindTransferRender
 	routines.

 arguments:
 temps:		r16
*****************************************************************************/
#define FIND3D()									\
		PVRSRV_SGXUTILS_CALL(Check3DQueues);

/*****************************************************************************
 SPMRENDERFINISHED
 	Emits the loopback task to call the SPM render finished routine.

 arguments:
 temps:		r16
*****************************************************************************/
#define SPMRENDERFINISHED()									\
		mov		r16, HSH(USE_LOOPBACK_SPMRENDERFINISHED);	\
		PVRSRV_SGXUTILS_CALL(EmitLoopBack);

/*****************************************************************************
 SETUPDIRLISTBASE
 	Set up a directory list base for either TA or 3D requestor.

 arguments:	context - PVR3DIF4_HWRENDERCONTEXT to set
			requestor - 0 = TA / 1 = 3D
 temps:		r16, r17, r18, r19, r20, r21, r22
*****************************************************************************/
#define SETUP_BIF_TA_REQ	0x0
#define SETUP_BIF_3D_REQ	0x1
#define SETUP_BIF_TQ_REQ	0x2
#if defined(SGX_FEATURE_2D_HARDWARE)
#define	SETUP_BIF_2D_REQ	0x4
#endif
#if defined(SUPPORT_SGX_EDM_MEMORY_DEBUG)
#define	SETUP_BIF_EDM_REQ	0x8
#endif

/* SetupRequestor is a separate module to save temps/predicates,
 * not used on single memory context devices
 */
#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
#define PVRSRV_SGXUTILS_SETUPDIRLISTBASE(context,req)	\
		mov		r16, context;							\
		mov		r17, req;								\
		PVRSRV_SGXUTILS_CALL(SetupDirListBase);			\
		PVRSRV_SGXUTILS_CALL(SetupRequestor);
#else
#define PVRSRV_SGXUTILS_SETUPDIRLISTBASE(context,req)	\
		mov		r16, context;							\
		mov		r17, req;								\
		PVRSRV_SGXUTILS_CALL(SetupDirListBase);
#endif /* SGX_FEATURE_MULTIPLE_MEM_CONTEXTS */

/* It may be necessary to idle some cores */
#if defined(FIX_HW_BRN_24549) || defined(FIX_HW_BRN_25615)
#if defined(SGX_FEATURE_2D_HARDWARE)
#define SETUPDIRLISTBASE(label, context, requestor)									\
		xor.testz	p1, requestor, HSH(SETUP_BIF_2D_REQ);							\
		p1	ba	label##_NoIdleCore;													\
		{;																			\
			and.testnz	p1, requestor, HSH(SETUP_BIF_3D_REQ | SETUP_BIF_TQ_REQ);	\
			p1	mov	r16, HSH(USE_IDLECORE_TA_REQ | USE_IDLECORE_USSEONLY_REQ);		\
			!p1	mov	r16, HSH(USE_IDLECORE_3D_REQ | USE_IDLECORE_USSEONLY_REQ);		\
			PVRSRV_SGXUTILS_CALL(IdleCore);											\
		};																			\
		label##_NoIdleCore:;														\
		PVRSRV_SGXUTILS_SETUPDIRLISTBASE(context,requestor);							\
		RESUME(r16);
#else /* SGX_FEATURE_2D_HARDWARE */
#define SETUPDIRLISTBASE(label, context, requestor)							\
		and.testnz	p1, requestor, HSH(SETUP_BIF_3D_REQ | SETUP_BIF_TQ_REQ);\
		p1	mov	r16, HSH(USE_IDLECORE_TA_REQ | USE_IDLECORE_USSEONLY_REQ);	\
		!p1	mov	r16, HSH(USE_IDLECORE_3D_REQ | USE_IDLECORE_USSEONLY_REQ);	\
		PVRSRV_SGXUTILS_CALL(IdleCore);										\
		PVRSRV_SGXUTILS_SETUPDIRLISTBASE(context,requestor);					\
		RESUME(r16);
#endif /* SGX_FEATURE_2D_HARDWARE */
#else /* defined(FIX_HW_BRN_24549) || defined(FIX_HW_BRN_25615) */
#define SETUPDIRLISTBASE(label, context, requestor)	\
		PVRSRV_SGXUTILS_SETUPDIRLISTBASE(context,requestor);
#endif

#if defined(SUPPORT_SGX_EDM_MEMORY_DEBUG) && defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)

/* Sets up requestor to allow EDM to read arbitrary memory
 * context (e.g. pixel or vertex heaps)
 */
#define SETUPDIRLISTBASEEDM()							\
		mov		r16, HSH(0);							\
		mov		r17, HSH(SETUP_BIF_EDM_REQ);			\
		PVRSRV_SGXUTILS_CALL(SetupDirListBase);			\
		PVRSRV_SGXUTILS_CALL(SetupRequestorEDM);
#endif /* SUPPORT_SGX_EDM_MEMORY_DEBUG && SGX_FEATURE_MULTIPLE_MEM_CONTEXTS */


#if defined(SGX_FEATURE_SYSTEM_CACHE)

	#if defined(SGX_FEATURE_MP)
		#if defined(SGX_BYPASS_SYSTEM_CACHE)
	#define INVALIDATE_SYSTEM_CACHE(req_mask, flush_mask)
	#define INVALIDATE_SYSTEM_CACHE_INLINE(RegA, inval_mask, RegTmp, label_name)
	#define	FLUSH_SYSTEM_CACHE_INLINE(Pred, RegA, flush_mask, RegTmp, label_name)
		#else

	#define INVALIDATE_SYSTEM_CACHE(req_mask, flush_mask)						\
		mov		r16, req_mask;													\
		mov		r17, flush_mask;												\
		PVRSRV_SGXUTILS_CALL(InvalidateSLC);
		
			#if defined(EUR_CR_MASTER_SLC_CTRL_INVAL)
				#if defined(FIX_HW_BRN_29574) && defined(FIX_HW_BRN_30505)
	/* Note: All HW which has BRN29574 also had BRN30505 when this was implemented */
#define INVALIDATE_SYSTEM_CACHE_INLINE(RegA, inval_mask, RegTmp, label_name)												\
	.align 	8;																												\
	str.nosched		HSH(EUR_CR_MASTER_SLC_CTRL_FLUSH) >> 2, HSH(EUR_CR_MASTER_SLC_CTRL_FLUSH_ALL_MASK);						\
	str.nosched		HSH(EUR_CR_MASTER_SLC_CTRL_INVAL) >> 2, inval_mask;														\
	ENTER_UNMATCHABLE_BLOCK;                                         														\
	label_name##_WaitForInvalidate:;                                   														\
	{;                                                               														\
		ldr		RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS >> 2), drc0;      													\
		wdf		drc0;                                                														\
		and		RegA, RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS_INVAL_MASK | EUR_CR_MASTER_SLC_EVENT_STATUS_FLUSH_MASK);		\
		xor.testz	p0, RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS_INVAL_MASK | EUR_CR_MASTER_SLC_EVENT_STATUS_FLUSH_MASK);	\
		!p0	ba		label_name##_WaitForInvalidate;																			\
	};                                                              														\
	LEAVE_UNMATCHABLE_BLOCK;                                         														\
	                                                                														\
	/* Clear the status bits */                                      														\
	str		HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR >> 2), HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR_INVAL_MASK | EUR_CR_MASTER_SLC_EVENT_CLEAR_FLUSH_MASK);

#define	FLUSH_SYSTEM_CACHE_INLINE(Pred, RegA, flush_mask, RegTmp, label_name)	\
	str		HSH(EUR_CR_MASTER_SLC_CTRL_FLUSH) >> 2, HSH(EUR_CR_MASTER_SLC_CTRL_FLUSH_ALL_MASK);		\
	ENTER_UNMATCHABLE_BLOCK;                                         			\
	label_name##_WaitForSLCFlush:;                                   			\
	{;                                                               			\
		ldr		RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS >> 2), drc0;      		\
		wdf		drc0;                                                			\
		and.testz	Pred, RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS_FLUSH_MASK);	\
		Pred	ba		label_name##_WaitForSLCFlush;							\
	};                                                              			\
	LEAVE_UNMATCHABLE_BLOCK;                                         			\
	                                                                			\
	/* Clear the status bits */                                      			\
	str		HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR >> 2), HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR_FLUSH_MASK);
				#else	/* FIX_HW_BRN_29574 */
					#if defined(FIX_HW_BRN_30505)
#define INVALIDATE_SYSTEM_CACHE_INLINE(RegA, mask, RegTmp, label_name)														\
	/* Align the flush and invalidate writes to a cache line */																\
	.align 	8;																												\
	str.nosched		HSH(EUR_CR_MASTER_SLC_CTRL_FLUSH) >> 2, mask;															\
	str.nosched		HSH(EUR_CR_MASTER_SLC_CTRL_INVAL) >> 2, mask;															\
	ENTER_UNMATCHABLE_BLOCK;                                         														\
	label_name##_WaitForInvalidate:;                                   														\
	{;                                                               														\
		ldr		RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS >> 2), drc0;      													\
		wdf		drc0;                                                														\
		and		RegA, RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS_INVAL_MASK | EUR_CR_MASTER_SLC_EVENT_STATUS_FLUSH_MASK);		\
		xor.testz	p0, RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS_INVAL_MASK | EUR_CR_MASTER_SLC_EVENT_STATUS_FLUSH_MASK);	\
		!p0	ba		label_name##_WaitForInvalidate;																			\
	};                                                              														\
	LEAVE_UNMATCHABLE_BLOCK;                                         														\
	                                                                														\
	/* Clear the status bits */                                      														\
	str		HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR >> 2), HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR_INVAL_MASK | EUR_CR_MASTER_SLC_EVENT_CLEAR_FLUSH_MASK);

#define	FLUSH_SYSTEM_CACHE_INLINE(Pred, RegA, flush_mask, RegTmp, label_name)						\
	str		HSH(EUR_CR_MASTER_SLC_CTRL_FLUSH) >> 2,  flush_mask;									\
	ENTER_UNMATCHABLE_BLOCK;                                         								\
	label_name##_WaitForSLCFlush:;                                   								\
	{;                                                               								\
		ldr		RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS >> 2), drc0;      							\
		wdf		drc0;                                                								\
		and.testz	Pred, RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS_FLUSH_MASK);						\
		Pred	ba		label_name##_WaitForSLCFlush;												\
	};                                                              								\
	LEAVE_UNMATCHABLE_BLOCK;                                         								\
	                                                                								\
	/* Clear the status bits */                                      								\
	str		HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR >> 2), HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR_FLUSH_MASK);							
					#else /* FIX_HW_BRN_30505 */
#define INVALIDATE_SYSTEM_CACHE_INLINE(RegA, mask, RegTmp, label_name)														\
	str		HSH(EUR_CR_MASTER_SLC_CTRL_FLUSH) >> 2, mask;																	\
	str		HSH(EUR_CR_MASTER_SLC_CTRL_INVAL) >> 2, mask;																	\
	ENTER_UNMATCHABLE_BLOCK;                                         														\
	label_name##_WaitForInvalidate:;                                   														\
	{;                                                               														\
		ldr		RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS >> 2), drc0;      													\
		wdf		drc0;                                                														\
		and		RegA, RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS_INVAL_MASK | EUR_CR_MASTER_SLC_EVENT_STATUS_FLUSH_MASK);		\
		xor.testz	p0, RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS_INVAL_MASK | EUR_CR_MASTER_SLC_EVENT_STATUS_FLUSH_MASK);	\
		!p0	ba		label_name##_WaitForInvalidate;																			\
	};                                                              														\
	LEAVE_UNMATCHABLE_BLOCK;                                         														\
	                                                                														\
	/* Clear the status bits */                                      														\
	str		HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR >> 2), HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR_INVAL_MASK | EUR_CR_MASTER_SLC_EVENT_CLEAR_FLUSH_MASK);

#define	FLUSH_SYSTEM_CACHE_INLINE(Pred, RegA, flush_mask, RegTmp, label_name)						\
	str		HSH(EUR_CR_MASTER_SLC_CTRL_FLUSH) >> 2,  flush_mask;									\
	ENTER_UNMATCHABLE_BLOCK;                                         								\
	label_name##_WaitForSLCFlush:;                                   								\
	{;                                                               								\
		ldr		RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS >> 2), drc0;      							\
		wdf		drc0;                                                								\
		and.testz	Pred, RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS_FLUSH_MASK);						\
		Pred	ba		label_name##_WaitForSLCFlush;												\
	};                                                              								\
	LEAVE_UNMATCHABLE_BLOCK;                                         								\
	                                                                								\
	/* Clear the status bits */                                      								\
	str		HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR >> 2), HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR_FLUSH_MASK);				
					#endif /* FIX_HW_BRN_30505 */
				#endif /* FIX_HW_BRN_29574 && FIX_HW_BRN_30505 */
			#else /* EUR_CR_MASTER_SLC_CTRL_INVAL */
				#if defined(FIX_HW_BRN_29574)
#define INVALIDATE_SYSTEM_CACHE_INLINE(RegA, inval_mask, RegTmp, label_name)	\
	mov		RegTmp, HSH(EUR_CR_MASTER_SLC_CTRL >> 2);							\
	ldr		RegA, RegTmp, drc0;													\
	wdf		drc0;																\
	or		RegA, RegA, inval_mask;												\
	str		RegTmp, RegA;                        								\
	mov		RegTmp, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS >> 2);					\
	ENTER_UNMATCHABLE_BLOCK;                                         			\
	label_name##_WaitForInvalidate:;                                   			\
	{;                                                               			\
		ldr		RegA, RegTmp, drc0;      										\
		wdf		drc0;                                                			\
		and		RegA, RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS_INVAL_MASK);		\
		xor.testz	p0, RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS_INVAL_MASK);	\
		!p0	ba		label_name##_WaitForInvalidate;								\
	};                                                              			\
	LEAVE_UNMATCHABLE_BLOCK;                                         			\
	                                                                			\
	/* Clear the status bits */                                      			\
	mov		RegTmp, HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR >> 2);					\
	str		RegTmp, HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR_INVAL_MASK);				\
	mov		RegTmp, HSH(EUR_CR_MASTER_SLC_CTRL >> 2);							\
	ldr		RegA, RegTmp, drc0;													\
	wdf		drc0;																\
	and		RegA, RegA, ~inval_mask;											\
	str		RegTmp, RegA;

#define	FLUSH_SYSTEM_CACHE_INLINE(Pred, RegA, flush_mask, RegTmp, label_name)	\
	mov		RegTmp, HSH(EUR_CR_MASTER_SLC_CTRL >> 2);							\
	ldr		RegA, RegTmp, drc0;													\
	wdf		drc0;																\
	or		RegA, RegA, HSH(EUR_CR_MASTER_SLC_CTRL_FLUSH_ALL_MASK);				\
	str		RegTmp, RegA;                        								\
	mov		RegTmp, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS >> 2);					\
	ENTER_UNMATCHABLE_BLOCK;                                         			\
	label_name##_WaitForSLCFlush:;                                   			\
	{;                                                               			\
		ldr		RegA, RegTmp, drc0;      										\
		wdf		drc0;                                                			\
		and.testz	Pred, RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS_FLUSH_MASK);	\
		Pred	ba		label_name##_WaitForSLCFlush;							\
	};                                                              			\
	LEAVE_UNMATCHABLE_BLOCK;                                         			\
	                                                                			\
	/* Clear the status bits */                                      			\
	mov		RegTmp, HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR >> 2);					\
	str		RegTmp, HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR_FLUSH_MASK);				\
	mov		RegTmp, HSH(EUR_CR_MASTER_SLC_CTRL >> 2);							\
	ldr		RegA, RegTmp, drc0;													\
	wdf		drc0;																\
	and		RegA, RegA, ~HSH(EUR_CR_MASTER_SLC_CTRL_FLUSH_ALL_MASK);			\
	str		RegTmp, RegA;
				#else /* FIX_HW_BRN_29574 */
#define INVALIDATE_SYSTEM_CACHE_INLINE(RegA, inval_mask, RegTmp, label_name)	\
	mov		RegTmp, HSH(EUR_CR_MASTER_SLC_CTRL >> 2);							\
	ldr		RegA, RegTmp, drc0;													\
	wdf		drc0;																\
	or		RegA, RegA, inval_mask;												\
	str		RegTmp, RegA;                        								\
	mov		RegTmp, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS >> 2);					\
	ENTER_UNMATCHABLE_BLOCK;                                         			\
	label_name##_WaitForInvalidate:;                                   			\
	{;                                                               			\
		ldr		RegA, RegTmp, drc0;      										\
		wdf		drc0;                                                			\
		and		RegA, RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS_INVAL_MASK);		\
		xor.testz	p0, RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS_INVAL_MASK);	\
		!p0	ba		label_name##_WaitForInvalidate;								\
	};                                                              			\
	LEAVE_UNMATCHABLE_BLOCK;                                         			\
	                                                                			\
	/* Clear the status bits */                                      			\
	mov		RegTmp, HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR >> 2);					\
	str		RegTmp, HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR_INVAL_MASK);				\
	mov		RegTmp, HSH(EUR_CR_MASTER_SLC_CTRL >> 2);							\
	ldr		RegA, RegTmp, drc0;													\
	wdf		drc0;																\
	and		RegA, RegA, ~inval_mask;											\
	str		RegTmp, RegA;

#define	FLUSH_SYSTEM_CACHE_INLINE(Pred, RegA, flush_mask, RegTmp, label_name)	\
	mov		RegTmp, HSH(EUR_CR_MASTER_SLC_CTRL >> 2);							\
	ldr		RegA, RegTmp, drc0;													\
	wdf		drc0;																\
	or		RegA, RegA, flush_mask;												\
	str		RegTmp, RegA;                        								\
	mov		RegTmp, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS >> 2);					\
	ENTER_UNMATCHABLE_BLOCK;                                         			\
	label_name##_WaitForSLCFlush:;                                   			\
	{;                                                               			\
		ldr		RegA, RegTmp, drc0;      										\
		wdf		drc0;                                                			\
		and.testz	Pred, RegA, HSH(EUR_CR_MASTER_SLC_EVENT_STATUS_FLUSH_MASK);	\
		Pred	ba		label_name##_WaitForSLCFlush;							\
	};                                                              			\
	LEAVE_UNMATCHABLE_BLOCK;                                         			\
	                                                                			\
	/* Clear the status bits */                                      			\
	mov		RegTmp, HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR >> 2);					\
	str		RegTmp, HSH(EUR_CR_MASTER_SLC_EVENT_CLEAR_FLUSH_MASK);				\
	mov		RegTmp, HSH(EUR_CR_MASTER_SLC_CTRL >> 2);							\
	ldr		RegA, RegTmp, drc0;													\
	wdf		drc0;																\
	and		RegA, RegA, ~flush_mask;											\
	str		RegTmp, RegA;				
				#endif /* FIX_HW_BRN_29574 */
			#endif /* EUR_CR_MASTER_SLC_CTRL_INVAL */
		#endif /* SGX_BYPASS_SYSTEM_CACHE */
	#else /* SGX_FEATURE_MP */
	
		#if defined(SGX_BYPASS_SYSTEM_CACHE)
	#define INVALIDATE_SYSTEM_CACHE()
	#define INVALIDATE_SYSTEM_CACHE_INLINE(RegA, RegTmp, label_name)
		#else

	#define INVALIDATE_SYSTEM_CACHE()											\
		PVRSRV_SGXUTILS_CALL(InvalidateSLC);

			#if defined(FIX_HW_BRN_31077)
   			/* note: MNE_CR_CTRL - cannot use per req invals - do not change code in absence of  FIX_HW_BRN_27534  */
			#endif
			#if defined(FIX_HW_BRN_27534)
#define INVALIDATE_SYSTEM_CACHE_INLINE(RegA, RegTmp, label_name)				\
	ldr		RegA, HSH(MNE_CR_CTRL >> 2), drc0;									\
	wdf		drc0;																\
	or		RegA, RegA, HSH(MNE_CR_CTRL_INVAL_ALL_MASK);						\
	str		HSH(MNE_CR_CTRL >> 2), RegA;                        				\
	ENTER_UNMATCHABLE_BLOCK;                                         			\
	label_name##_WaitForInvalidate:;                                   			\
	{;                                                               			\
		ldr		RegA, HSH(MNE_CR_EVENT_STATUS >> 2), drc0;      				\
		wdf		drc0;                                                			\
		and.testz	p0, RegA, HSH(MNE_CR_EVENT_STATUS_INVAL_MASK);				\
		p0	ba		label_name##_WaitForInvalidate;								\
	};                                                              			\
	LEAVE_UNMATCHABLE_BLOCK;                                         			\
	                                                                			\
	/* Clear the status bits */                                      			\
	str		HSH(MNE_CR_EVENT_CLEAR >> 2), HSH(MNE_CR_EVENT_CLEAR_INVAL_MASK);	\
	ldr		RegA, HSH(MNE_CR_CTRL >> 2), drc0;									\
	wdf		drc0;																\
	and		RegA, RegA, HSH(~MNE_CR_CTRL_INVAL_ALL_MASK);						\
	str		HSH(MNE_CR_CTRL >> 2), RegA;
			#else
#define INVALIDATE_SYSTEM_CACHE_INLINE(RegA, RegTmp, label_name)				\
	str		HSH(MNE_CR_CTRL_INVAL >> 2), HSH(MNE_CR_CTRL_INVAL_ALL_MASK);       \
	ENTER_UNMATCHABLE_BLOCK;                                         			\
	label_name##_WaitForInvalidate:;                                   			\
	{;                                                               			\
		ldr		RegA, HSH(MNE_CR_EVENT_STATUS >> 2), drc0;      				\
		wdf		drc0;                                                			\
		and.testz	p0, RegA, HSH(MNE_CR_EVENT_STATUS_INVAL_MASK);				\
		p0	ba		label_name##_WaitForInvalidate;								\
	};                                                              			\
	LEAVE_UNMATCHABLE_BLOCK;                                         			\
	                                                                			\
	/* Clear the status bits */                                      			\
	str		HSH(MNE_CR_EVENT_CLEAR >> 2), HSH(MNE_CR_EVENT_CLEAR_INVAL_MASK);
			#endif /* MNE_CR_CTRL_INVAL (FIX_HW_BRN_27534) */
		#endif /* SGX_BYPASS_SYSTEM_CACHE */
	#endif /* SGX_FEATURE_MP */
#endif /* SGX_FEATURE_SYSTEM_CACHE */

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
#define INVALIDATE_EXT_SYSTEM_CACHE()											\
		PVRSRV_SGXUTILS_CALL(InvalidateExtSysCache);
#endif/* SUPPORT_EXTERNAL_SYSTEM_CACHE */

/*****************************************************************************
 ISUB32
 	32 bit integer sub (signed)
 	!!! IMPORTANT regA and regB can not be the same !!!

 arguments:	regA - result
			regB - number to subtract from
			regC - number to subtract (alters value)
 temps:		regD
*****************************************************************************/
#if defined(SGX543) || defined(SGX544) || defined(SGX554)
#define ISUB32(regA, regB, regC, regD)									\
		ima32.s.s1	regD, regB, HSH(1), -regC;							\
		ima32.s.s2	regA, regB, HSH(1), regD;
#else
#define ISUB32(regA, regB, regC, regD)									\
		xor		regC, regC, HSH(-1);									\
		mov		regD, HSH(1);											\
		iadd32	regC, regD.low, regC;									\
		IADD32(regA, regB, regC, regD);
#endif

/*****************************************************************************
 IADD32
 	32 bit integer add (signed)
 	!!! IMPORTANT regA and regB can not be the same !!!

 arguments:	regA - result
			regB - number to add to
			regC - number to add
 temps:		regD
*****************************************************************************/
#if defined(SGX543) || defined(SGX544) || defined(SGX554)
#define IADD32(regA, regB, regC, regD)									\
		ima32.s.s1	regD, regB, HSH(1), regC;							\
		ima32.s.s2	regA, regB, HSH(1), regD;
#else
#define IADD32(regA, regB, regC, regD)									\
		shr		regD, regB, HSH(16);									\
		shr		regA, regC, HSH(16);									\
		iadd32	regA, regA.low, regD;									\
		shl		regA, regA, HSH(16);									\
		imae	regA, regB.low, HSH(1), regA, u32;						\
		imae	regA, regC.low, HSH(1), regA, u32;
#endif

/*****************************************************************************
  COMPARE
 	32 bit comparison, gives true if and only if regA < regB

 arguments:	regP - (predicate reg) result
			regA - first number to compare
			regB - second number to compare (alters value)
 temps: regC, regD
*****************************************************************************/
#define COMPARE(regP, regA, regB, regC, regD)							\
		ISUB32(regC, regA, regB, regD);									\
		or.tests	regP, regC, regC;


/*****************************************************************************
  REG_INCREMENT
 	32 bit register increment

 arguments:	regA - register to increment
 temps: regP
*****************************************************************************/
#if defined(SGX543) || defined(SGX544) || defined(SGX554)

#define REG_INCREMENT(regA, regP)										\
	REG_INCREMENT_BYVALUE(regA, regP, HSH(1));
#define REG_INCREMENT_BYVALUE(regA, regP, value)						\
	IMA32.s1		regA, regA, HSH(1), value;
	
#else
#define REG_INCREMENT(regA, regP)										\
	REG_INCREMENT_BYVALUE(regA, regP, HSH(1));
#define REG_INCREMENT_BYVALUE(regA, regP, value)						\
	iaddu32.testz	regA, regP, value, regA;
#endif /* SGX543 */


#if defined(SUPPORT_SGX_HWPERF)
#define PVRSRV_SGX_HWPERF_STATUS_ANY_ON		(PVRSRV_SGX_HWPERF_STATUS_GRAPHICS_ON |\
											 PVRSRV_SGX_HWPERF_STATUS_PERIODIC_ON |\
											 PVRSRV_SGX_HWPERF_STATUS_MK_EXECUTION_ON)
#if defined(EUR_CR_TIMER)
#define WRITEHWPERFCB_TIMESTAMP(regP, regBase, regA, regB, regC, regD, regE)							\
	ldr				regA, HSH(SGX_MP_CORE_SELECT(EUR_CR_TIMER, 0)) >> 2, drc0;													\
	wdf				drc0;																				\
	MK_MEM_ACCESS_BPCACHE(stad)	[regBase, HSH(DOFFSET(SGXMKIF_HWPERF_CB_ENTRY.ui32Time))], regA;
#else
#define WRITEHWPERFCB_TIMESTAMP(regP, regBase, regA, regB, regC, regD, regE)									\
	ldr				regA, HSH(EUR_CR_EVENT_TIMER) >> 2, drc0;													\
	MK_MEM_ACCESS_BPCACHE(ldad)	regE, [R_HostCtl, HSH(DOFFSET(SGXMKIF_HOST_CTL.ui32TimeWraps))], drc0;				\
	mov				regB, PA(ui32TimeStamp); 																	\
	wdf				drc0;																						\
	/* if the timer wrapped since we were in the PDS. */														\
	COMPARE(regP, regA, regB, regC, regD);																		\
	!regP mov		regD, HSH(1);																				\
	!regP iaddu32	regE, regD.low, regE;																		\
	MK_MEM_ACCESS_BPCACHE(stad)	[regBase, HSH(DOFFSET(SGXMKIF_HWPERF_CB_ENTRY.ui32TimeWraps))], regE;						\
	MK_MEM_ACCESS_BPCACHE(stad)	[regBase, HSH(DOFFSET(SGXMKIF_HWPERF_CB_ENTRY.ui32Time))], regA;
#endif /* EUR_CR_TIMER */

/*****************************************************************************
 WRITEHWPERFCB
 Writes a HWPerf CB packet. If there is only one space left, writes invalid packet.
 arguments:	type - TA, 3D, 2D
 			swtype - [TRANSFER|TA|3D|2D]_[START|END]
			hwperfstatus - GRAPHICS or MK_EXECUTION
			regInfo - context-specific data for the 'info' field
			regP - predicate
			label_name - unique label id
			regA, regB, regC, regD, regE, regF, regG, regH - temps
 temps: drc0
*****************************************************************************/
#define WRITEHWPERFCB(type, swtype, hwperfstatus, regInfo, regP, label_name, 											\
						regA, regB, regC, regD, regE, regF, regG, regH)													\
		/* Check whether this event type is enabled. */																	\
		MK_LOAD_STATE(regB, ui32HWPerfFlags, drc0);																		\
		MK_WAIT_FOR_STATE_LOAD(drc0);																					\
		and.testz		regP, regB, HSH(PVRSRV_SGX_HWPERF_STATUS_##hwperfstatus##_ON);									\
		regP ba label_name##HWPerfIsOff;																				\
		{;																												\
			mov				r16, HSH(OFFSET(SGXMK_TA3D_CTL.ui32##type##FrameNum));						\
			mov				r17, HSH(OFFSET(SGXMK_TA3D_CTL.ui32##type##PID));						\
			shl				r17, r17, HSH(16);											\
			or				r16, r16, r17;											\
			mov				r17, HSH(PVRSRV_SGX_HWPERF_TYPE_##swtype);							\
			mov				r18, regInfo;											\
			mov				r19, HSH(OFFSET(SGXMK_TA3D_CTL.s##type##RTData));						\
			PVRSRV_SGXUTILS_CALL(WriteHWPERFEntry);												\
		};																												\
		label_name##HWPerfIsOff:

#else
#define WRITEHWPERFCB(type, swtype, hwperfstatus, regInfo, regP, label_name, 											\
						regA, regB, regC, regD, regE, regF, regG, regH)
#endif /* SUPPORT_SGX_HWPERF*/

#if defined(FIX_HW_BRN_28889)
#define	DOBIFINVAL(label, pred, RequestReg, Channel)												\
	/* Indicate to the host the invalidate has been done */											\
	MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, HSH(DOFFSET(SGXMKIF_HOST_CTL.ui32InvalStatus))], HSH(PVRSRV_USSE_EDM_BIF_INVAL_COMPLETE);	\
	idf			drc0, st;																			\
	wdf			drc0;																				\
																									\
	and.testnz	pred, RequestReg,	HSH(SGXMKIF_CC_INVAL_BIF_PD);									\
	!pred	br		label##NoBIFInvalPDCache;														\
	{;																								\
		/* invalidate the PD, PT caches	and	TLBs */													\
		MK_TRACE(MKTC_INVALDC, MKTF_SCH | MKTF_HW);													\
		mov		r16, HSH(1);																		\
		PVRSRV_SGXUTILS_CALL(InvalidateBIFCache);													\
		br		label##NoBIFInvalPTCache;															\
	};																								\
	label##NoBIFInvalPDCache:;																		\
																									\
	and.testnz	pred, RequestReg,	HSH(SGXMKIF_CC_INVAL_BIF_PT);									\
	!pred	br		label##NoBIFInvalPTCache;														\
	{;																								\
		/* invalidate the PT cache and TLBs	*/														\
		MK_TRACE(MKTC_INVALPT, MKTF_SCH | MKTF_HW);													\
		mov		r16, HSH(0);																		\
		PVRSRV_SGXUTILS_CALL(InvalidateBIFCache);													\
	};																								\
	label##NoBIFInvalPTCache:;																		\
																									\
	/* We have done	the	invalidate so clear	the	field */											\
	MK_STORE_STATE(ui32CacheCtrl, HSH(0));															\
	MK_WAIT_FOR_STATE_STORE(drc0)
#endif

#if (defined(FIX_HW_BRN_24304) || defined(FIX_HW_BRN_27510)) && defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
/*****************************************************************************
 CONFIGURECACHE - 	This routine checks which contexts are running and configs
 					the cache partitions for the best balance between performance
 					and reliabilty

 arguments:	0 = TA Kick, 1= 3D Kick, 2 = HW Finish (reset partitions)
 temps:		r17, r18, r19
*****************************************************************************/
#define	CC_FLAGS_TA_KICK	0x0
#define	CC_FLAGS_3D_KICK	0x1
#define	CC_FLAGS_HW_FINISH	0x2

#define CONFIGURECACHE(action)											\
		mov		r16, action;											\
		PVRSRV_SGXUTILS_CALL(ConfigureCache);

#endif
/*****************************************************************************
 INVALIDATE_BIF_CACHE
 	Invalidate the BIF directory cache.

 arguments:	regA - zero for PTE, non-zero for PD
            regB, regC - temporary registers
            name - unique name for generating internal label
 registers: p0, p2
*****************************************************************************/
#if defined(FIX_HW_BRN_22997) && defined(FIX_HW_BRN_23030)
#define INVALIDATE_BIF_CACHE(regA, regB, regC, label_name)					\
	/* check whether this is a PD or PTE flush */							\
	and.testnz	p0, regA, regA;												\
																			\
	/* disable DM events on PDS */											\
	ldr		regA, HSH(EUR_CR_DMS_CTRL) >> 2, drc0;							\
	wdf		drc0;															\
	/* all but loopbacks */													\
	or		regA, regA, HSH(7);												\
	str		HSH(EUR_CR_DMS_CTRL) >> 2, regA;								\
																			\
	/* wait for queued USSE tasks empty */									\
	label_name##PollForUSSEQueueEmpty:;										\
	{;																		\
		ldr			regB, HSH(EUR_CR_USE0_SERV_PIXEL) >> 2, drc0;			\
		ldr			regC, HSH(EUR_CR_USE1_SERV_PIXEL) >> 2, drc0;			\
		wdf			drc0;													\
		and			regA, regB, regC;										\
		ldr			regB, HSH(EUR_CR_USE0_SERV_VERTEX) >> 2, drc0;			\
		ldr			regC, HSH(EUR_CR_USE1_SERV_VERTEX) >> 2, drc0;			\
		wdf			drc0;													\
		and			regA, regA, regB;										\
		and			regA, regA, regC;										\
		mov			regB, HSH(EUR_CR_USE0_SERV_PIXEL_EMPTY_MASK);			\
		and.testz	p2, regA, regB;											\
		p2 ba		label_name##PollForUSSEQueueEmpty;						\
	};																		\
																			\
	/* wait for active pipe 1 tasks to finish */							\
	mov			regB, HSH(0xAAAAAAAA);										\
	label_name##PollForUSSEPipe1Idle:;										\
	{;																		\
		ldr			regA, HSH(EUR_CR_USE1_DM_SLOT) >> 2, drc0;				\
		wdf			drc0;													\
		xor.testnz	p2, regA, regB;											\
		p2 ba		label_name##PollForUSSEPipe1Idle;						\
	};																		\
																			\
	/* wait for active pipe 0 tasks to finish */							\
	/* Load the current tasks's mask into regC. */							\
	shl			regA, G22, HSH(1); 											\
	shl			regC, HSH(0x3), regA;										\
	mov			regB, HSH(0xAAAAAAAA);										\
	or			regB, regB, regC;											\
	label_name##PollForUSSEPipe0Idle:;										\
	{;																		\
		ldr			regA, HSH(EUR_CR_USE0_DM_SLOT) >> 2, drc0;				\
		wdf			drc0;													\
		xor.testnz	p2, regA, regB;											\
		p2	ba		label_name##PollForUSSEPipe0Idle;						\
	};																		\
																			\
	/* prepare the 'idle 10 cycle' counter */								\
	mov		regA, HSH(10);													\
																			\
	/* snapshot the BIF_CTRL register */									\
	ldr		regB, HSH(EUR_CR_BIF_CTRL) >> 2, drc0;							\
	wdf		drc0;															\
																			\
	/* setup the BIF_CTRL 'pause' register value */							\
	or		regC, regB, HSH(EUR_CR_BIF_CTRL_PAUSE_MASK);					\
																			\
	/* pause the BIF */														\
	/* Align this code to an instruction cache line (16 instructions) */	\
	.align 16;																\
	SGXMK_SPRV_UTILS_WRITE_REG(HSH(EUR_CR_BIF_CTRL) >> 2, regC);			\
																			\
	/* idle 10 cycles */													\
	label_name##IdleForTen:;												\
	{;																		\
		iaddu32		regA, regA.low, HSH(-1);								\
		and.testnz	p2, regA, regA;											\
		p2 ba		label_name##IdleForTen;									\
	};																		\
																			\
	/* Poll for outstanding reads = 0 */									\
	label_name##PollForBIFReads:;											\
	{;																		\
		ldr		regA, HSH(EUR_CR_BIF_MEM_REQ_STAT) >> 2, drc0;				\
		wdf		drc0;														\
		and		regA, regA, HSH(EUR_CR_BIF_MEM_REQ_STAT_READS_MASK);		\
		and.testnz p2, regA, regA;											\
		p2 ba		label_name##PollForBIFReads;							\
	};																		\
																			\
	/* clear fault (if we got one) */										\
	or		regA, regC, HSH(EUR_CR_BIF_CTRL_CLEAR_FAULT_MASK);				\
	SGXMK_SPRV_UTILS_WRITE_REG(HSH(EUR_CR_BIF_CTRL) >> 2, regA);			\
																			\
	/* invalidate the cache */												\
	p0 	or	regA, regC, HSH(EUR_CR_BIF_CTRL_INVALDC_MASK);					\
	!p0 or	regA, regC, HSH(EUR_CR_BIF_CTRL_FLUSH_MASK);					\
	SGXMK_SPRV_UTILS_WRITE_REG(HSH(EUR_CR_BIF_CTRL) >> 2, regA);			\
																			\
	/* un-pause the BIF and clear the bit */								\
	SGXMK_SPRV_UTILS_WRITE_REG(HSH(EUR_CR_BIF_CTRL) >> 2, regB);			\
																			\
	/* enable all the DMs */												\
	ldr		regA, HSH(EUR_CR_DMS_CTRL) >> 2, drc0;							\
	wdf		drc0;															\
	and		regB, regA, HSH(~EUR_CR_DMS_CTRL_DISABLE_DM_MASK);				\
	str		HSH(EUR_CR_DMS_CTRL) >> 2, regB;
#else

	#if defined(FIX_HW_BRN_27251)

#define INVALIDATE_BIF_CACHE(reg1, reg2, reg3, label_name)					\
	/* always perform a PD flush */											\
	ldr		reg1, HSH(EUR_CR_BIF_CTRL) >> 2, drc0;							\
	wdf		drc0;															\
	or	reg2, reg1, HSH(EUR_CR_BIF_CTRL_INVALDC_MASK);						\
	SGXMK_SPRV_UTILS_WRITE_REG(HSH(EUR_CR_BIF_CTRL) >> 2, reg2);			\
	SGXMK_SPRV_UTILS_WRITE_REG(HSH(EUR_CR_BIF_CTRL) >> 2, reg1);


	#else

#if defined(FIX_HW_BRN_31620)
				
	#define 	BRN31620_LOAD_OFFSET		(4096)
	#define 	BRN31620_START_ADDR_LOW		(BRN31620_LOAD_OFFSET)
	#define 	BRN31620_START_ADDR_HIGH	((1 << 31) + BRN31620_LOAD_OFFSET)
	#define 	BRN31620_LOAD_ADVANCE		(64*1024*1024)
	#define		BRN31620_INVAL_CONTEXT_ID	(7)
	#define 	BRN31620_INVAL_DIR_LIST_BASE	(EUR_CR_BIF_DIR_LIST_BASE1 + ((BRN31620_INVAL_CONTEXT_ID-1) * 4))

#define REQUEST_DC_REFRESH(inval_msk0, inval_msk1)		\
	mov		r24, HSH(SGX_UKERNEL_SPRV_BRN_31620);		\
	mov		r25, inval_msk0;							\
	mov		r26, inval_msk1;							\
	PVRSRV_SGXUTILS_CALL(JumpToSupervisor);

#define REQUEST_DC_LOAD()								\
	mov		r24, HSH(SGX_UKERNEL_SPRV_BRN_31620);		\
	mov		r25, HSH(0xFFFFFFFF);						\
	mov		r26, HSH(0xFFFFFFFF);						\
	mov		r28, pclink;								\
	bal		JumpToSupervisor;							\
	mov		pclink, r28;
	
#define REFRESH_DC_CACHE(pred, reg1, reg2, reg3, reg4, start_addr, inval_msk, label)	\
	/* switch edm to update context */										\
	mov		reg1, g45;														\
	WRITECORECONFIG(reg1, HSH(EUR_CR_BIF_BANK0 >> 2), HSH(BRN31620_INVAL_CONTEXT_ID), reg2);	\
	/* Initialise the shift count */										\
	mov		reg1, inval_msk;												\
	mov		reg2, start_addr;												\
	mov		reg3, HSH(BRN31620_LOAD_ADVANCE);								\
	label##CheckNextCL:;													\
	{;																		\
		/* Is this cache line affected? */									\
		and.testz	pred, reg1, HSH(1);										\
		pred	br	label##NoCLRefresh;										\
		{;																	\
			/* fetch the updated data from memory */						\
			MK_MEM_ACCESS_BPCACHE(ldad)	reg4, [reg2], drc0;					\
		};																	\
		label##NoCLRefresh:;												\
																			\
		/* shift the mask */												\
		shr		reg1, reg1, HSH(1);											\
		/* move on the address by advance */								\
		ima32.u.s1	reg2, reg3, HSH(1), reg2;								\
																			\
		/* have we checked all the mask bits, if not check next bit */		\
		and.testnz	pred, reg1, reg1;										\
		pred	br	label##CheckNextCL;										\
	};																		\
	{;																		\
		/* wait for the memory reads to come back */						\
		wdf		drc0;														\
																			\
		/* if bif_bank0 == 0, exit */										\
		mov		reg1, g45;													\
		READCORECONFIG(reg2, reg1, HSH(EUR_CR_BIF_BANK0 >> 2), drc0);		\
		wdf		drc0;														\
		and.testz	pred, reg2, reg2;										\
		pred	br	label##PDRefreshComplete;								\
		{;																	\
			/* reset the bif_bank0 register */								\
			WRITECORECONFIG(reg1, HSH(EUR_CR_BIF_BANK0 >> 2), HSH(0), reg2);	\
																			\
			/* reset the inval_msk */										\
			mov		reg1, inval_msk;										\
			/* reset the start addr */										\
			mov		reg2, start_addr;										\
																			\
			br	label##CheckNextCL;											\
		};																	\
	};																		\
	label##PDRefreshComplete:;
#endif


		#if defined(FIX_HW_BRN_29997)
			#if defined(SGX_FEATURE_MK_SUPERVISOR)
#define INVALIDATE_BIF_CACHE(reg1, reg2, reg3, label_name)					\
	mov		r24, HSH(SGX_UKERNEL_SPRV_BRN_29997);							\
	mov		r25, reg1;														\
	mov		r28, pclink;													\
	bal		JumpToSupervisor;												\
	mov		pclink, r28;
			#else
	#error ERROR: FIX_HW_BRN_29997 only supported with SGX_FEATURE_MK_SUPERVISOR
			#endif
			#if defined(SUPPORT_SGX_BYPASS_MMU)
	#error ERROR: FIX_HW_BRN_29997 assumes MMU is not bypassed
			#endif
		#else
			#if defined(EUR_CR_BIF_CTRL_INVAL)

#define INVALIDATE_BIF_CACHE(reg1, reg2, reg3, label_name)					\
	/* check whether this is a PD or PTE flush */							\
	and.testnz	p0, reg1, reg1;												\
	p0  str		HSH(EUR_CR_BIF_CTRL_INVAL) >> 2, HSH(EUR_CR_BIF_CTRL_INVAL_ALL_MASK);	\
	!p0 str		HSH(EUR_CR_BIF_CTRL_INVAL) >> 2, HSH(EUR_CR_BIF_CTRL_INVAL_PTE_MASK);

			#else

#define INVALIDATE_BIF_CACHE(reg1, reg2, reg3, label_name)					\
	/* check whether this is a PD or PTE flush */							\
	and.testnz	p0, reg1, reg1;												\
	ldr		reg1, HSH(EUR_CR_BIF_CTRL) >> 2, drc0;							\
	wdf		drc0;															\
	p0	or	reg2, reg1, HSH(EUR_CR_BIF_CTRL_INVALDC_MASK);					\
	!p0	or	reg2, reg1, HSH(EUR_CR_BIF_CTRL_FLUSH_MASK);					\
	SGXMK_SPRV_UTILS_WRITE_REG(HSH(EUR_CR_BIF_CTRL) >> 2, reg2);			\
	SGXMK_SPRV_UTILS_WRITE_REG(HSH(EUR_CR_BIF_CTRL) >> 2, reg1);

			#endif /* defined(EUR_CR_BIF_CTRL_INVAL) */
		#endif /* defined(FIX_HW_BRN_29997) */
	#endif /* defined(FIX_HW_BRN_27251) */

#endif /* defined(FIX_HW_BRN_22997) && defined(FIX_HW_BRN_23030) */

/*****************************************************************************
 CLEARACTIVEPOWERFLAGS
 	Clear an active power flag and check whether to start the active power
 	countdown.

 arguments:	flag - the flag to set
 temps:		r16, r17, r18
*****************************************************************************/
#define CLEARACTIVEPOWERFLAGS(flag)											\
	mov		r16, flag;														\
	PVRSRV_SGXUTILS_CALL(ClearActivePowerFlags);


/*****************************************************************************
 CLEARPENDINGLOOPBACKS
 	Clear events from the pending loopbacks state tracker.

 arguments:	reg1, reg2
 temps:		PA(ui32PDSIn0)
*****************************************************************************/
#define CLEARPENDINGLOOPBACKS(reg1, reg2)									\
	MK_MEM_ACCESS_BPCACHE(ldad)	reg1, [R_TA3DCtl, HSH(DOFFSET(SGXMK_TA3D_CTL.ui32PendingLoopbacks))], drc0;	\
	xor				reg2, PA(ui32PDSIn0), HSH(0xFFFFFFFF);					\
	wdf				drc0;													\
	and				reg1, reg1, reg2;										\
	MK_MEM_ACCESS_BPCACHE(stad)	[R_TA3DCtl, HSH(DOFFSET(SGXMK_TA3D_CTL.ui32PendingLoopbacks))], reg1;


#define MOVESCENETOCOMPLETELIST(context, renderdetails)		\
	mov		r16, context;									\
	mov		r17, renderdetails;								\
	PVRSRV_SGXUTILS_CALL(MoveSceneToCompleteList);

#if defined(SGX_FEATURE_MP)
	#if defined(SGX_FEATURE_MP_PLUS)

#define ADDR_INCREMENT_BYCORES_LEFT_TA(label_name, reg1, reg2, reg3)			\
	MK_LOAD_STATE_CORE_COUNT_TA(reg3, drc0);									\
	mov		reg2, HSH(SGX_FEATURE_MP_CORE_COUNT_TA);							\
	MK_WAIT_FOR_STATE_LOAD(drc0);												\
	isub16	reg2, reg2, reg3;													\
	imae	reg1, reg2.low, HSH(SIZEOF(IMG_UINT32)), reg1, u32;
#define ADDR_INCREMENT_BYCORES_LEFT_3D(label_name, reg1, reg2, reg3)			\
	MK_LOAD_STATE_CORE_COUNT_3D(reg3, drc0);									\
	mov		reg2, HSH(SGX_FEATURE_MP_CORE_COUNT_3D);							\
	MK_WAIT_FOR_STATE_LOAD(drc0);												\
	isub16	reg2, reg2, reg3;													\
	imae	reg1, reg2.low, HSH(SIZEOF(IMG_UINT32)), reg1, u32;
	#else /* defined(SGX_FEATURE_MP_PLUS) */

#define ADDR_INCREMENT_BYCORES_LEFT(label_name, reg1, reg2, reg3)				\
	MK_LOAD_STATE(reg3, ui32CoresEnabled, drc0);								\
	mov		reg2, HSH(SGX_FEATURE_MP_CORE_COUNT);								\
	MK_WAIT_FOR_STATE_LOAD(drc0);												\
	isub16	reg2, reg2, reg3;													\
	imae	reg1, reg2.low, HSH(SIZEOF(IMG_UINT32)), reg1, u32;
#define ADDR_INCREMENT_BYCORES_LEFT_TA(label_name, reg1, reg2, reg3)			\
	ADDR_INCREMENT_BYCORES_LEFT(label_name, reg1, reg2, reg3);
#define ADDR_INCREMENT_BYCORES_LEFT_3D(label_name, reg1, reg2, reg3)			\
	ADDR_INCREMENT_BYCORES_LEFT(label_name, reg1, reg2, reg3);
	#endif /* defined(SGX_FEATURE_MP_PLUS) */
#endif
/*****************************************************************************
 COPYMEMTOCONFIGREGISTER
 	Copy data from memory to per-core config register.

 arguments:	label_name - unique name
			ConfigReg - EUR_CR_xxx
			reg1 - Base address of data in memory
 temps:		reg2, reg3, reg4
*****************************************************************************/
#if defined(SGX_FEATURE_MP)
#define COPYMEMTOCONFIGREGISTER_PERCORE(label_name, reg1, reg2, reg3, reg4)		\
	label_name##Loop:;															\
	{;																			\
		MK_MEM_ACCESS(ldad)	reg4, [reg1, HSH(1)++], drc0;						\
		wdf				drc0;													\
		str				reg2, reg4;												\
		isub16.testz	reg3, p0, reg3, HSH(1);									\
		p0	ba			label_name##Loaded;										\
		mov				reg4, HSH(SGX_REG_BANK_SIZE) >> 2;						\
		iaddu32			reg2, reg4.low, reg2;									\
		ba				label_name##Loop;										\
	};																			\
	label_name##Loaded:;

	#if defined(SGX_FEATURE_MP_PLUS)
	/* macros for non-equal TA/3D core count */
	#define COPYMEMTOCONFIGREGISTER_TA(label_name, ConfigReg, reg1, reg2, reg3, reg4)	\
		mov				reg2, HSH(SGX_MP_CORE_SELECT(ConfigReg, 0)) >> 2;				\
		MK_LOAD_STATE_CORE_COUNT_TA(reg3, drc0);										\
		MK_WAIT_FOR_STATE_LOAD(drc0);													\
		COPYMEMTOCONFIGREGISTER_PERCORE(label_name, reg1, reg2, reg3, reg4);			\
		ADDR_INCREMENT_BYCORES_LEFT_TA(label_name, reg1, reg2, reg3);
	
	#define COPYMEMTOCONFIGREGISTER_3D(label_name, ConfigReg, reg1, reg2, reg3, reg4)	\
		mov				reg2, HSH(SGX_MP_CORE_SELECT(ConfigReg, 0)) >> 2;				\
		MK_LOAD_STATE_CORE_COUNT_3D(reg3, drc0);										\
		MK_WAIT_FOR_STATE_LOAD(drc0);													\
		COPYMEMTOCONFIGREGISTER_PERCORE(label_name, reg1, reg2, reg3, reg4);			\
		ADDR_INCREMENT_BYCORES_LEFT_3D(label_name, reg1, reg2, reg3);
	#else /* defined(SGX_FEATURE_MP_PLUS) */
	
	/* macros for equal TA/3D core count */
	#define COPYMEMTOCONFIGREGISTER2(label_name, ConfigReg, reg1, reg2, reg3, reg4)		\
		mov				reg2, HSH(SGX_MP_CORE_SELECT(ConfigReg, 0)) >> 2;				\
		MK_LOAD_STATE(reg3, ui32CoresEnabled, drc0);										\
		MK_WAIT_FOR_STATE_LOAD(drc0);													\
		COPYMEMTOCONFIGREGISTER_PERCORE(label_name, reg1, reg2, reg3, reg4);			\
	ADDR_INCREMENT_BYCORES_LEFT(label_name, reg1, reg2, reg3);
	#define COPYMEMTOCONFIGREGISTER_TA(label_name, ConfigReg, reg1, reg2, reg3, reg4)	\
		COPYMEMTOCONFIGREGISTER2(label_name, ConfigReg, reg1, reg2, reg3, reg4);
	#define COPYMEMTOCONFIGREGISTER_3D(label_name, ConfigReg, reg1, reg2, reg3, reg4)	\
		COPYMEMTOCONFIGREGISTER2(label_name, ConfigReg, reg1, reg2, reg3, reg4);	
	#endif /* defined(SGX_FEATURE_MP_PLUS) */
#else
#define COPYMEMTOCONFIGREGISTER(label_name, ConfigReg, reg1, reg2, reg3, reg4)		\
	MK_MEM_ACCESS(ldad)	reg4, [reg1, HSH(1)++], drc0;								\
	wdf				drc0;															\
	str				HSH(ConfigReg) >> 2, reg4;
#define COPYMEMTOCONFIGREGISTER_TA(label_name, ConfigReg, reg1, reg2, reg3, reg4)	\
	COPYMEMTOCONFIGREGISTER(label_name, ConfigReg, reg1, reg2, reg3, reg4);
#define COPYMEMTOCONFIGREGISTER_3D(label_name, ConfigReg, reg1, reg2, reg3, reg4)	\
	COPYMEMTOCONFIGREGISTER(label_name, ConfigReg, reg1, reg2, reg3, reg4);	
#endif /* SGX_FEATURE_MP */

/*****************************************************************************
 COPYCONFIGREGISTERTOMEM
 	Copy data from per-core config register to memory.

 arguments:	label_name - unique name
			ConfigReg - EUR_CR_xxx
			reg1 - Base address of data in memory
 temps:		reg2, reg3, reg4
*****************************************************************************/
#if defined(SGX_FEATURE_MP)
#define COPYCONFIGREGISTERTOMEM_PERCORE(label_name, reg1, reg2, reg3, reg4)		\
	label_name##Loop:;															\
	{;																			\
		ldr				reg4, reg2, drc0;										\
		wdf				drc0;													\
		MK_MEM_ACCESS(stad)	[reg1, HSH(1)++], reg4;								\
		isub16.testz	reg3, p0, reg3, HSH(1);									\
		p0	ba			label_name##Loaded;										\
		mov				reg4, HSH(SGX_REG_BANK_SIZE) >> 2;						\
		iaddu32			reg2, reg4.low, reg2;									\
		ba				label_name##Loop;										\
	};																			\
	label_name##Loaded:;

	#if defined(SGX_FEATURE_MP_PLUS)
	/* macros for non-equal TA/3D core count */
	#define COPYCONFIGREGISTERTOMEM_TA(label_name, ConfigReg, reg1, reg2, reg3, reg4)	\
		mov				reg2, HSH(SGX_MP_CORE_SELECT(ConfigReg, 0)) >> 2;				\
		MK_LOAD_STATE_CORE_COUNT_TA(reg3, drc0);										\
		MK_WAIT_FOR_STATE_LOAD(drc0);													\
		COPYCONFIGREGISTERTOMEM_PERCORE(label_name, reg1, reg2, reg3, reg4);			\
		ADDR_INCREMENT_BYCORES_LEFT_TA(label_name, reg1, reg2, reg3);

	#define COPYCONFIGREGISTERTOMEM_3D(label_name, ConfigReg, reg1, reg2, reg3, reg4)	\
		mov				reg2, HSH(SGX_MP_CORE_SELECT(ConfigReg, 0)) >> 2;				\
		MK_LOAD_STATE_CORE_COUNT_3D(reg3, drc0);										\
		MK_WAIT_FOR_STATE_LOAD(drc0);													\
		COPYCONFIGREGISTERTOMEM_PERCORE(label_name, reg1, reg2, reg3, reg4);			\
		ADDR_INCREMENT_BYCORES_LEFT_3D(label_name, reg1, reg2, reg3);
	#else /* defined(SGX_FEATURE_MP_PLUS) */

	/* macros for equal TA/3D core count */
	#define COPYCONFIGREGISTERTOMEM2(label_name, ConfigReg, reg1, reg2, reg3, reg4)		\
		mov				reg2, HSH(SGX_MP_CORE_SELECT(ConfigReg, 0)) >> 2;				\
		MK_LOAD_STATE(reg3, ui32CoresEnabled, drc0);									\
		MK_WAIT_FOR_STATE_LOAD(drc0);													\
		COPYCONFIGREGISTERTOMEM_PERCORE(label_name, reg1, reg2, reg3, reg4);			\
	ADDR_INCREMENT_BYCORES_LEFT(label_name, reg1, reg2, reg3);
	#define COPYCONFIGREGISTERTOMEM_TA(label_name, ConfigReg, reg1, reg2, reg3, reg4)	\
		COPYCONFIGREGISTERTOMEM2(label_name, ConfigReg, reg1, reg2, reg3, reg4);
	#define COPYCONFIGREGISTERTOMEM_3D(label_name, ConfigReg, reg1, reg2, reg3, reg4)	\
		COPYCONFIGREGISTERTOMEM2(label_name, ConfigReg, reg1, reg2, reg3, reg4);
	#endif /* defined(SGX_FEATURE_MP_PLUS) */	
#else
#define COPYCONFIGREGISTERTOMEM(label_name, ConfigReg, reg1, reg2, reg3, reg4)		\
	ldr				reg4, HSH(ConfigReg) >> 2, drc0;								\
	wdf				drc0;															\
	MK_MEM_ACCESS(stad)	[reg1], reg4;
#define COPYCONFIGREGISTERTOMEM_TA(label_name, ConfigReg, reg1, reg2, reg3, reg4)	\
	COPYCONFIGREGISTERTOMEM(label_name, ConfigReg, reg1, reg2, reg3, reg4);
#define COPYCONFIGREGISTERTOMEM_3D(label_name, ConfigReg, reg1, reg2, reg3, reg4)	\
	COPYCONFIGREGISTERTOMEM(label_name, ConfigReg, reg1, reg2, reg3, reg4);
#endif /* SGX_FEATURE_MP */

#if defined(SGX_FEATURE_MP)
/*****************************************************************************
 WRITEMASTERCONFIG
 	write argument data to master config register bank

 arguments:	Core - Index of the core
			ConfigReg - EUR_CR_xxx
			Data - Data to be written
 temps:	reg1
*****************************************************************************/
#define WRITEMASTERCONFIG(ConfigReg, Data, reg1)	\
	mov		reg1, ConfigReg;						\
	str		reg1, Data;

/*****************************************************************************
 READMASTERCONFIG
 	read master config register

 arguments:	Core - Index of the core
			ConfigReg - EUR_CR_xxx
			Data - Data to be written
 temps:	reg1
*****************************************************************************/
#define READMASTERCONFIG(Dest, ConfigReg, Channel)	\
	mov		Dest, ConfigReg;						\
	ldr		Dest, Dest, Channel;
#endif

/*****************************************************************************
 WRITECORECONFIG
 	write argument data to per-core config register

 arguments:	Core - Index of the core
			ConfigReg - EUR_CR_xxx
			Data - Data to be written
 temps:	reg1
*****************************************************************************/
#if defined(SGX_FEATURE_MP)
#define WRITECORECONFIG(Core, ConfigReg, Data, reg1)		\
	iaddu32	reg1, Core.low, HSH(SGX_REG_BANK_BASE_INDEX);	\
	shl		reg1, reg1, HSH(SGX_REG_BANK_SHIFT-2);			\
	or		reg1, reg1, ConfigReg;							\
	str		reg1, Data;	
#else
#define WRITECORECONFIG(Core, ConfigReg, Data, reg1)	\
	str		ConfigReg, Data;
#endif
/*****************************************************************************
 READCORECONFIG
 	read per-core config register

 arguments:	Core - Index of the core
			ConfigReg - EUR_CR_xxx
			Data - Data to be written
 temps:	reg1
*****************************************************************************/
#if defined(SGX_FEATURE_MP)
#define READCORECONFIG(DestReg, Core, ConfigReg, Channel)		\
	iaddu32	DestReg, Core.low, HSH(SGX_REG_BANK_BASE_INDEX);	\
	shl		DestReg, DestReg, HSH(SGX_REG_BANK_SHIFT-2);			\
	or		DestReg, DestReg, ConfigReg;						\
	ldr		DestReg, DestReg, Channel;
#else
#define READCORECONFIG(DestReg, Core, ConfigReg, Channel)		\
	ldr		DestReg, ConfigReg, Channel;
#endif /* SGX_FEATURE_MP */

/*****************************************************************************
 WRITEISPRGNBASE
 	Copy data from memory to per-core config register.

 arguments:	label_name - unique name
			ReadAddr - Base address of data in memory
 temps:		reg1, reg3, reg4
*****************************************************************************/
#if defined(SGX_FEATURE_MP)
#define WRITEISPRGNBASE(label_name, ReadAddr, DestReg, CoreCnt, TmpReg)	\
	MK_MEM_ACCESS_BPCACHE(ldad)	TmpReg, [ReadAddr, HSH(1)++], drc0;		\
	wdf				drc0;												\
	str				HSH(EUR_CR_ISP_RGN_BASE) >> 2, TmpReg;				\
	mov				DestReg, HSH(EUR_CR_MASTER_ISP_RGN_BASE1) >> 2;		\
	MK_LOAD_STATE_CORE_COUNT_TA(CoreCnt, drc0);							\
	MK_WAIT_FOR_STATE_LOAD(drc0);										\
	label_name##Loop:;													\
	{;																	\
		isub16.testz	CoreCnt, p2, CoreCnt, HSH(1);					\
		p2	ba			label_name##Loaded;								\
		MK_MEM_ACCESS_BPCACHE(ldad)	TmpReg, [ReadAddr, HSH(1)++], drc0;	\
		wdf				drc0;											\
		str				DestReg, TmpReg;								\
		mov				TmpReg, HSH(1);									\
		iaddu32			DestReg, TmpReg.low, DestReg;					\
		ba				label_name##Loop;								\
	};																	\
	label_name##Loaded:;												\
	ADDR_INCREMENT_BYCORES_LEFT_TA(label_name, ReadAddr, DestReg, TmpReg);
#else
#define WRITEISPRGNBASE(label_name, ReadAddr, reg1, reg2, reg3)			\
	MK_MEM_ACCESS_BPCACHE(ldad)	reg1, [ReadAddr, HSH(1)++], drc0;		\
	wdf				drc0;												\
	str				HSH(EUR_CR_ISP_RGN_BASE) >> 2, reg1;
#endif /* SGX_FEATURE_MP */

#if defined(PVRSRV_USSE_EDM_BREAKPOINTS)
#define	SGXMK_BREAKPOINT(label_name, Pred, TmpReg)															\
	MK_MEM_ACCESS_BPCACHE(ldad)	TmpReg, [R_HostCtl, HSH(DOFFSET(SGXMKIF_HOST_CTL.ui32BreakpointDisable))], drc0;	\
	wdf		drc0;																							\
	and.testnz	Pred, TmpReg, TmpReg;																		\
	Pred	ba	label_name##Resume;																			\
	label_name##Pause:;																						\
	MK_MEM_ACCESS_BPCACHE(ldad)	TmpReg, [R_HostCtl, HSH(DOFFSET(SGXMKIF_HOST_CTL.ui32Continue))], drc0;			\
	wdf		drc0;																							\
	and.testz	Pred, TmpReg, TmpReg;																		\
	Pred	ba	label_name##Pause;																			\
	label_name##Resume:;																					\
	MK_MEM_ACCESS_BPCACHE(stad)	[R_HostCtl, HSH(DOFFSET(SGXMKIF_HOST_CTL.ui32Continue))], HSH(0);					\
	idf		drc0, st;																						\
	wdf		drc0;
#endif


#if defined(SGX_FEATURE_36BIT_MMU)
/*****************************************************************************
 SGXMK_HWCONTEXT_GETDEVPADDR
 	extracts the PD DevPAddr from the HW context

 arguments:	devpaddr - devpaddr to return (also used as a tmp)
			hwrc - address of HW context
 temps:		tmp_reg
 			drc_id - drc counter to use for wdf
*****************************************************************************/
#define SGXMK_HWCONTEXT_GETDEVPADDR(devpaddr, hwrc, tmp_reg, drc_id)																			\
		MK_MEM_ACCESS(ldad)	devpaddr, [hwrc, HSH(DOFFSET(SGXMKIF_HWCONTEXT.sPDDevPAddr.uiAddr))], drc_id;	\
		MK_MEM_ACCESS(ldad)	tmp_reg, [hwrc, HSH(DOFFSET(SGXMKIF_HWCONTEXT.sPDDevPAddr.uiHighAddr))], drc_id;\
		wdf		drc_id;																						\
		shr		devpaddr, devpaddr, HSH(4);																	\
		shl		tmp_reg, tmp_reg, HSH(28);																	\
		or		devpaddr, devpaddr, tmp_reg;
/*****************************************************************************
 SGXMK_HWCONTEXT_GETDEVPADDR
 	extracts the PD DevPAddr from the TA3DCtrl

 arguments:	devpaddr - devpaddr to return (also used as a tmp)
 temps:		tmp_reg
 			drc_id - drc counter to use for wdf
*****************************************************************************/
#define SGXMK_TA3DCTRL_GETDEVPADDR(devpaddr, tmp_reg, drc_id)														\
		MK_MEM_ACCESS_BPCACHE(ldad)	devpaddr, [R_TA3DCtl, HSH(DOFFSET(SGXMK_TA3D_CTL.sKernelPDDevPAddr.uiAddr))], drc_id;		\
		MK_MEM_ACCESS_BPCACHE(ldad)	tmp_reg, [R_TA3DCtl, HSH(DOFFSET(SGXMK_TA3D_CTL.sKernelPDDevPAddr.uiHighAddr))], drc_id;	\
		wdf		drc_id;																								\
		shr		devpaddr, devpaddr, HSH(4);																			\
		shl		tmp_reg, tmp_reg, HSH(28);																			\
		or		devpaddr, devpaddr, tmp_reg;
#else
/*****************************************************************************
 SGXMK_HWCONTEXT_GETDEVPADDR
 	extracts the PD DevPAddr from the HW context

 arguments:	devpaddr - devpaddr to return (also used as a tmp)
			hwrc - address of HW context
 temps:		tmp_reg (unused)
 			drc_id - drc counter to use for wdf
*****************************************************************************/
#define SGXMK_HWCONTEXT_GETDEVPADDR(devpaddr, hwrc, tmp_reg, drc_id)										\
		MK_MEM_ACCESS(ldad)	devpaddr, [hwrc, HSH(DOFFSET(SGXMKIF_HWCONTEXT.sPDDevPAddr.uiAddr))], drc_id;	\
		wdf		drc_id;
/*****************************************************************************
 SGXMK_HWCONTEXT_GETDEVPADDR
 	extracts the PD DevPAddr from the TA3DCtrl

 arguments:	devpaddr - devpaddr to return (also used as a tmp)
 temps:		tmp_reg (unused)
 			drc_id - drc counter to use for wdf
*****************************************************************************/
#define SGXMK_TA3DCTRL_GETDEVPADDR(devpaddr, tmp_reg, drc_id)													\
		MK_MEM_ACCESS_BPCACHE(ldad)	devpaddr, [R_TA3DCtl, HSH(DOFFSET(SGXMK_TA3D_CTL.sKernelPDDevPAddr.uiAddr))], drc_id;	\
		wdf		drc_id;
#endif

/*****************************************************************************
 SGXMK_SPRV_WRITE_REG / SGXMK_SPRV_UTILS_WRITE_REG
 	Supervisor register writes
 	two versions: a normal one and one for calls from utils

 arguments:	
 	reg_offset - offset of supervisor controlled register
 	reg_value - value of supervisor controlled register
 temps:		r24 if SGX_FEATURE_MK_SUPERVISOR
 			r25 if SGX_FEATURE_MK_SUPERVISOR
 			r26, r27, r28, r29			
*****************************************************************************/
#if defined(SGX_FEATURE_MK_SUPERVISOR)
#define SGXMK_SPRV_WRITE_REG(reg_offset, reg_value)			\
		mov		r24, HSH(SGX_UKERNEL_SPRV_WRITE_REG);		\
		mov		r25, reg_offset;							\
		mov		r26, reg_value;								\
		PVRSRV_SGXUTILS_CALL(JumpToSupervisor);
#define SGXMK_SPRV_UTILS_WRITE_REG(reg_offset, reg_value)	\
		mov		r24, HSH(SGX_UKERNEL_SPRV_WRITE_REG);		\
		mov		r25, reg_offset;							\
		mov		r26, reg_value;								\
		mov		r28, pclink;								\
		bal		JumpToSupervisor;							\
		mov		pclink, r28;
#else
#define SGXMK_SPRV_WRITE_REG(reg_offset, reg_value)	\
		str		reg_offset, reg_value;
#define SGXMK_SPRV_UTILS_WRITE_REG		SGXMK_SPRV_WRITE_REG
#endif/* #if defined(SGX_FEATURE_MK_SUPERVISOR) */

#if (defined(FIX_HW_BRN_30089) || defined(FIX_HW_BRN_29504) || defined(FIX_HW_BRN_33753)) && defined(SGX_FEATURE_MP)
#define	ADDLASTRGN(rtdata, mt_idx)							\
		.import	BRN30089AddLastRgn;							\
		mov		r16, rtdata;								\
		mov		r17, mt_idx;								\
		PVRSRV_SGXUTILS_CALL(BRN30089AddLastRgn);
		
#define RESTORERGNHDR(rtdata, mt_idx)						\
		.import	BRN30089RestoreRgnHdr;						\
		mov		r16, rtdata;								\
		mov		r17, mt_idx;								\
		PVRSRV_SGXUTILS_CALL(BRN30089RestoreRgnHdr);
#endif

#if defined(SGX_FEATURE_EXTENDED_PERF_COUNTERS)
/*****************************************************************************
 SET_PERF_GROUP
 Sets a hardware performance counter group register.
 arguments:	regA, regB, regC, regD - temps
 			index1 - index into aui32PerfGroup and aui32PerfBit
			index2 - EUR_CR_PERF register index
			*NOTE - (regA, regB) and (regC, regD) must be contiguous pairs
 temps: drc0, drc1
*****************************************************************************/
#define SET_PERF_GROUP(regA, regB, regC, regD, index1, index2)										\
	MK_MEM_ACCESS_BPCACHE(ldad.f2)	regA, [R_HostCtl, HSH(DOFFSET(SGXMKIF_HOST_CTL.aui32PerfGroup[index1]))-1], drc0;\
	MK_MEM_ACCESS_BPCACHE(ldad.f2)	regC, [R_HostCtl, HSH(DOFFSET(SGXMKIF_HOST_CTL.aui32PerfBit[index1]))-1], drc1;	\
	wdf drc0;																						\
	shl	regA, regA, HSH(EUR_CR_PERF0_GROUP_SELECT_A0_SHIFT);										\
	shl	regB, regB, HSH(EUR_CR_PERF0_GROUP_SELECT_A1_SHIFT);										\
	or	regA, regA, regB;																			\
	wdf drc1;																						\
	shl	regC, regC, HSH(EUR_CR_PERF0_COUNTER_SELECT_A0_SHIFT);										\
	shl	regD, regD, HSH(EUR_CR_PERF0_COUNTER_SELECT_A1_SHIFT);										\
	or	regA, regA, regC;																			\
	or	regA, regA, regD;																			\
	str		HSH(EUR_CR_PERF##index2) >> 2, regA;
#endif /* SGX_FEATURE_EXTENDED_PERF_COUNTERS */


/*****************************************************************************
 WRITE_SIGNATURES
 Dumps TA or 3D signature register samples to memory
 arguments:	regA, regB, regC, regD - temps
 			type - TA or 3D
 temps: drc0
*****************************************************************************/
#if !defined(DEBUG)
#define WRITE_SIGNATURES(type, regA, regB, regC, regD)
#else
#define WRITE_SIGNATURES(type, regA, regB, regC, regD)													\
	MK_MEM_ACCESS_BPCACHE(ldad)	regA, [R_TA3DCtl, HSH(DOFFSET(SGXMK_TA3D_CTL.s##type##SigBufferDevVAddr))], drc0;\
	wdf drc0;																							\
	MK_MEM_ACCESS_BPCACHE(ldad)	regC, [regA, HSH(DOFFSET(SGXMKIF_##type##SIG_BUFFER.ui32NumSamples))], drc0;\
	/* regB - address of data to write. */																\
	mov		regD, HSH(SIZEOF(SGXMKIF_##type##SIG_BUFFER.ui32Signature[0]));								\
	wdf 	drc0;																						\
	mov		regB, HSH(OFFSET(SGXMKIF_##type##SIG_BUFFER.ui32Signature));								\
	imae	regB, regC.low, regD.low, regB, u32;														\
	iaddu32	regB, regB.low, regA;																		\
	/* If the buffer is not full, increment CurrentSample. */											\
	/* Otherwise, continue to write values to the last sample position. */								\
	xor.testz	p0, regC, HSH(SGXMKIF_##type##SIG_BUFFER_SIZE - 1);										\
	p0	ba	WS_##type##_buffer_full;																	\
	{;																									\
		REG_INCREMENT(regC, p0);																		\
		MK_MEM_ACCESS_BPCACHE(stad)	[regA, HSH(DOFFSET(SGXMKIF_##type##SIG_BUFFER.ui32NumSamples))], regC;\
	};																									\
	WS_##type##_buffer_full:;																			\
	/* regA - address of signature registers' read offsets. */											\
	mov		regC, HSH(OFFSET(SGXMKIF_##type##SIG_BUFFER.ui32RegisterOffset));							\
	iaddu32	regA, regC.low, regA;																		\
	/* regD - loop counter. */																			\
	WS_##type##_start_loop:;																			\
	{;																									\
		/* Load signature value and store it out. */													\
		MK_MEM_ACCESS_BPCACHE(ldad)	regC, [regA, HSH(1)++], drc0;										\
		wdf 	drc0;																					\
		shr		regC, regC, HSH(2);																		\
		ldr		regC, regC, drc0;																		\
		wdf 	drc0;																					\
		MK_MEM_ACCESS_BPCACHE(stad)	[regB, HSH(1)++], regC;												\
		/* Decrement regD and loop back if necessary. */												\
		isub16.testnz	regD, p0, regD, HSH(SIZEOF(SGXMKIF_##type##SIG_BUFFER.ui32Signature[0][0]));	\
		p0	ba	WS_##type##_start_loop;																	\
	};
#endif /* DEBUG */

#if defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
#if 1 // SGX_SUPPORT_MASTER_ONLY_SWITCHING
/*	regNumSABlocks = number of 16-dword SA blocks (input, read-only), MAX 16
 *	reg2 = DMA burst size in dwords
 *	reg3 = burst size (BSIZE field)
 *	reg3 = stride (BSTRIDE field)
 *	reg3 = DMA burst number of lines
 *	regDOUTD1 = DOUTD1 control word (output)
 */
#if (EURASIA_PDS_DOUTD1_MAXBURST != EURASIA_PDS_DOUTD1_BSIZE_MAX)
#define PATCH_PDS_DMA_CONTROL(regDOUTD1, regNumSABlocks, reg2, reg3)							\
	mov		reg2, HSH(SGX_UKERNEL_SA_BURST_SIZE);												\
	mov		regDOUTD1, HSH(EURASIA_PDS_DOUTD1_INSTR_NORMAL) << EURASIA_PDS_DOUTD1_INSTR_SHIFT;	\
	isub32	reg2, reg2, HSH(1);																	\
	shl		reg3, reg2, HSH(EURASIA_PDS_DOUTD1_BSIZE_SHIFT);									\
	or		regDOUTD1, regDOUTD1, reg3;															\
	shl		reg3, reg2, HSH(EURASIA_PDS_DOUTD1_STRIDE_SHIFT);									\
	or		regDOUTD1, regDOUTD1, reg3;															\
	isub32	reg3, regNumSABlocks, HSH(1);														\
	or		regDOUTD1, regDOUTD1, reg3;
#else
#define PATCH_PDS_DMA_CONTROL(regDOUTD1, regNumSABlocks, reg2, reg3)							\
	shl		reg2, regNumSABlocks, HSH(SGX_31425_DMA_NLINES_SHIFT);								\
	mov		regDOUTD1, HSH(EURASIA_PDS_DOUTD1_INSTR_NORMAL) << EURASIA_PDS_DOUTD1_INSTR_SHIFT;	\
	isub32	reg2, reg2, HSH(1);																	\
	shl		reg3, reg2, HSH(EURASIA_PDS_DOUTD1_BSIZE_SHIFT);									\
	or		regDOUTD1, regDOUTD1, reg3;
#endif /* (EURASIA_PDS_DOUTD1_MAXBURST != EURASIA_PDS_DOUTD1_BSIZE_MAX) */

#endif /* SGX_SUPPORT_MASTER_ONLY_SWITCHING */
#endif

#if defined(SGX_FEATURE_MP) && defined(FIX_HW_BRN_31989)
/* collate named register across all slave cores and returned ORed result */
/*	label = unique location in code
	config_reg = named register to read (input)
	regA = ORed value of slave core regs (output)
*/
#define READ_COLLATE_TA_REG_OR(label, regA, config_reg, reg_corenum, regB)		\
	MK_LOAD_STATE_CORE_COUNT_TA(reg_corenum, drc0);								\
	mov		regA, HSH(0);														\
	MK_WAIT_FOR_STATE_LOAD(drc0);												\
	BRN31989_##label##_NextCore:;												\
	{;																			\
		isub32		reg_corenum, reg_corenum, HSH(1);							\
		READCORECONFIG(regB, reg_corenum, config_reg, drc0);					\
		wdf			drc0;														\
		or			regA, regA, regB;											\
		and.testnz	p0, reg_corenum, reg_corenum;								\
		p0			ba	BRN31989_##label##_NextCore;							\
	};
#endif /* FIX_HW_BRN_31989 */

#if defined(SGX_FEATURE_MP)
/* collate named register across all slave cores and returned ORed result */
/*	label = unique location in code
	config_reg = named register to read (input)
	regA = ORed value of slave core regs (output)
*/
#define READ_COLLATE_3D_REG_OR(label, regA, config_reg, reg_corenum, regB)		\
	MK_LOAD_STATE_CORE_COUNT_3D(reg_corenum, drc0);								\
	mov		regA, HSH(0);														\
	MK_WAIT_FOR_STATE_LOAD(drc0);												\
	_##label##_NextCore:;														\
	{;																			\
		isub32		reg_corenum, reg_corenum, HSH(1);							\
		READCORECONFIG(regB, reg_corenum, config_reg, drc0);					\
		wdf			drc0;														\
		or			regA, regA, regB;											\
		and.testnz	p0, reg_corenum, reg_corenum;								\
		p0			ba	_##label##_NextCore;									\
	};
#endif

 	
#if defined(FIX_HW_BRN_31542) || defined(FIX_HW_BRN_36513)
/* (EURASIA_TAIL_POINTER_SIZE * 32 * 32) */
#define SGX_CLIPPERBUG_PERCORE_TPC_SIZE	 8192
/* (EURASIA_REGIONHEADER_SIZE * 32 * 32) */
#define SGX_CLIPPERBUG_PERCORE_PSGRGN_SIZE   3*4096
#define SGX_CLIPPERBUG_PARAMMEM_NUM_PAGES 20
#endif

#if defined(FIX_HW_BRN_32302) || defined(SUPPORT_SGX_DOUBLE_PIXEL_PARTITIONS)
#define SGX_THRESHOLD_REDUCTION_FOR_VPCHANGE 0x100
#endif

#endif /* #ifndef __USEDEFS_H__ */
/******************************************************************************
 End of file (usedefs.h)
******************************************************************************/
