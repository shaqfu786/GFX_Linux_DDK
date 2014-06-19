/*!****************************************************************************
@File           sgxinit.c

@Title          Services initialisation routines

@Author         Imagination Technologies

@Date           17/10/2007

@Copyright      Copyright 2005-2010 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       Generic

@Description    Device specific functions

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-
$Log: sgxinit.c $
******************************************************************************/

#include <stddef.h>

#include "img_defs.h"
#include "srvinit.h"
#include "pvr_debug.h"
#include "sgxdefs.h"
#include "sgxpdsdefs.h"
#include "services.h"
#include "sgxapi.h"
#include "pdump_um.h"
#include "sgx_mkif.h"
#include "sgx_mkif_client.h" /* FIXME: this should not be required here */
#include "regpaths.h"
#include "registry.h"
#include "sgxinfo.h"

#include "sgx_options.h"

#include "servicesinit_um.h"
#include "sgxinit_um.h"
#include "sgxmmu.h"

#include "usecodegen.h"

#include "sgxinit_primary1_pds.h"
#include "sgxinit_secondary_pds.h"
#include "sgxinit_primary2_pds.h"
#include "eventhandler_pds.h"
#include "loopback_pds.h"
#include "ukernel.h"
#include "ukernel_labels.h"
#if defined(SGX_FEATURE_MP)
#include "slave_eventhandler_pds.h"
#include "slave_ukernel.h"
#include "slave_ukernel_labels.h"
#endif
#if !defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
#include "ukernelTA.h"
#include "ukernelTA_labels.h"
#include "ukernelSPM.h"
#include "ukernelSPM_labels.h"
#include "ukernel3D.h"
#include "ukernel3D_labels.h"
	#if defined(SGX_FEATURE_2D_HARDWARE)
#include "ukernel2D.h"
#include "ukernel2D_labels.h"
	#endif
#endif /* SGX_FEATURE_MONOLITHIC_UKERNEL */

#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH) \
	|| defined(SGX_FEATURE_ISP_CONTEXT_SWITCH) \
	|| defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	#if defined(ALLOW_EDM_PROFILES)
#include "ukernel1.h"
#include "ukernel_labels1.h"
#include "ukernel2.h"
#include "ukernel_labels2.h"
#include "ukernel3.h"
#include "ukernel_labels3.h"
		#if !defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
#include "ukernelTA1.h"
#include "ukernelTA_labels1.h"
#include "ukernelTA2.h"
#include "ukernelTA_labels2.h"
#include "ukernelTA3.h"
#include "ukernelTA_labels3.h"
#include "ukernel3D1.h"
#include "ukernel3D_labels1.h"
#include "ukernel3D2.h"
#include "ukernel3D_labels2.h"
#include "ukernel3D3.h"
#include "ukernel3D_labels3.h"
		#endif /* SGX_FEATURE_MONOLITHIC_UKERNEL */
	#endif /* ALLOW_EDM_PROFILES */

	#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
#include "vdmcontextstore_pds.h"
#include "vdmcontextstore_use.h"
#include "vdmcontextstore_use_labels.h"
#include "tastaterestore_pds.h"
#include "tastaterestore_use.h"
		#if defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
#include "tasarestore_pds.h"
#include "tasarestore_use.h"
#include "tasarestore_use_labels.h"
		#endif
		#if defined(SGX_FEATURE_MTE_STATE_FLUSH) && defined(SGX_FEATURE_COMPLEX_GEOMETRY_REV_2)
#include "cmplxstaterestore.h"
		#endif
	#endif
#endif

#define USE_DEFAULT_TEMP_REG_COUNT	EURASIA_USE_GLOBAL_TEMP_REG_LIMIT
#define USE_DEFAULT_TEMP_GRAN		EURASIA_USE_GLOBAL_TEMP_REG_GRAN

#define ALIGNFIELD(V, S)			(((V) + ((1UL << (S)) - 1)) >> (S))

#define	PDS_NUM_WORDS_PER_DWORD		2
#define	PDS_NUM_DWORDS_PER_REG		2
#if defined(SGX_FEATURE_PDS_DATA_INTERLEAVE_2DWORDS)
#define	PDS_NUM_DWORDS_PER_ROW		2
#else
#define	PDS_NUM_DWORDS_PER_ROW		8
#endif

#define	PDS_DS0_CONST_BLOCK_BASE	0
#define	PDS_DS1_CONST_BLOCK_BASE	0
#define	PDS_DS0_TEMP_BLOCK_BASE		EURASIA_PDS_DATASTORE_TEMPSTART
#define	PDS_DS1_TEMP_BLOCK_BASE		EURASIA_PDS_DATASTORE_TEMPSTART

#if defined(FIX_HW_BRN_31542) || defined(FIX_HW_BRN_36513)
/* Hand crafted VDM control stream */
static IMG_UINT32 aui32ClipperBugVDMStream[]=
{
	/*********************
	  Vertex Processing State 
	*********************/
	/* 0100 0000 0000 0000 0000 0000 0000 0000  - 
	 * first PDS program, so offset=zero, modified later 
	 * by adding base address in sgxinit.c */
	0x40000000,
	/* Vertex Processing State word2 */
	/* State-data size = 0x3 = 3 128 bit words 5-bits in SGX543 
	MTE_EMIT=1 (bit 17)
	SECONDARY_EXEC = 0
	SD(Sequential Dependency) = 1
	Pipe =2 (bit: 13,14)
	Bit12(partial) = 0
	Num Partitions=1 (reference: sgx_render_flip_test.c)
	Num Secs = 0
	Mte state 32-bit word count = 19 (20dw is nearest 128 bit aligned = 5 128bw)
	*/ 

	/*     0001 1010 0000 0010 0100 0010 0000 0101 */
	0x1a024205,
	
	/*********************
	  Index-list
	*********************/
	/* Block Header (index list 0)
	In| tri	| 67-pres?| 45-pres?|3-pres?|2-pres?| index-count
	10|0000| 0| 1| 0|0| 0000000000000000011000
	*/
	/* was 81400018*/
	0x81400018,
	/*  index list 1 patched later, primitive ID through FIFO set to zero*/
	0x0,
	/* index list 2 
	Vert |Cmplx | flat-shade | idx-size | idx-offset
	1000| 0 |00 |1 | 0000
	*/
	0x81000000,
	/* index list 4 
	Part-size| pds-base
	0011| 000 */
	/* This is patched with PDS program number 1's base address */
	0x30000000,  
	
	/* index list 5
	Vtx-size | vtx-adv|cmplx-only| use-attr-size|batch-num|core-lock|pds-data-size
	0000001 1000000000000001000000011 */
	0x03000803,


	/*********************
	  Vertex Processing State (Terminate)
	*********************/
	/* 0100 0000 0000 0000 0000 0000 0000 0000  - 
	 * first PDS program, so offset=zero, modified later 
	 * by adding base address in sgxinit.c */
	0x40000000,
	/* Vertex Processing State word2 */
	/* State-data size = 0x3 = 3 128 bit words 5-bits in SGX543 
	MTE_EMIT=1? (bit 17)
	SECONDARY_EXEC = 0
	SD(Sequential Dependency) = 1
	Pipe =2 (bit: 13,14)
	Bit12(partial) = 0
	Num Partitions=1 (reference: sgx_render_flip_test.c)
	Num Secs = 0
	Mte state 32-bit word count = 2 (4dw is nearest 128 bit aligned = 1 128bw)
	*/ 

	/*     0001 1010 0000 0010 0100 0010 0000 0101 */
	0x1a024201,
	

	/****************************
	 MTE terminate 
	****************************/
	/* Block header 1100. */
	0xC0000000
};

static IMG_UINT32 aui32ClipperBugPDSProgram[] =
{
	/* MTE state emit program */
	/* DMA0,1,2,3 */
	/* USE0, 1, 2 */
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,

	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,

	/* Code */	
	0x87000000, 0x90000006, 0x07000203, 0x87040000,
	0x90000006, 0x07040103, 0x07080185, 0xAF000000,
	
	/* Vertex loading task(same-as-above, no-dma-just if0 doutu, use does everything else */
	/* DMA0,1,2,3 */
	/* USE0, 1, 2 */
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,

	/* Code */	
	0x87000000, 0x90000006, 0x07000203, 0x87040000,
	0x90000006, 0x07040103, 0x03080185, 0xAF000000,

	/* MTE state emit(TERMINATE) program */
	/* DMA0,1,2,3 */
	/* USE0, 1, 2 */
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,

	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,

	/* Code */	
	0x87000000, 0x90000006, 0x07000203, 0x87040000,
	0x90000006, 0x07040103, 0x07080185, 0xAF000000,
	
};

static IMG_BYTE pbuClipperBugUSEProgram[] =
{
	/* 
		phas #0, #0, pixel, end, parallel;
		mov.skipinv o0, #0x00001F09; // Block header: VIEWPORT | WRAP | OUTSELECTS | WCLAMP | MTECTRL | FFISPA | BBISPA
		mov.skipinv o1, #0x00000000; // ISPA
		mov.skipinv o2, #0x00000000; // ISPA
		mov.skipinv o3, #0x00000000; // a0 = 0
		mov.skipinv o4, #0x3F800000; // m0 = 1
		mov.skipinv o5, #0x00000000; // a1 = 0
		mov.skipinv o6, #0x3F800000; // m1 = 1
		mov.skipinv o7, #0x00000000; // a2 = 0
		mov.skipinv o8, #0x3F800000; // m2 = 1
		mov.skipinv o9, #0x00000000; // Wrap = 0
		mov.skipinv o10, #0x04001000; // Output select
		mov.skipinv o11, #0x3727C5AC; // Wclamp = 0.00001f
		mov.skipinv o12, #0x00000000; // MTECTRL
		emitst.freep #0, #13;
		or.skipinv.end r0, r0, #0x00000000;     
	*/
	0x00,0x00,0x00,0x00,0x00,0x07,0x44,0xFA,
	0x09,0x1F,0x00,0x00,0x01,0x00,0xA0,0xFC,
	0x00,0x00,0x20,0x00,0x01,0x00,0xA0,0xFC,
	0x00,0x00,0x40,0x00,0x01,0x00,0xA0,0xFC,
	0x00,0x00,0x60,0x00,0x01,0x00,0xA0,0xFC,
	0x00,0x00,0x80,0x00,0xC1,0xF1,0xA0,0xFC,
	0x00,0x00,0xA0,0x00,0x01,0x00,0xA0,0xFC,
	0x00,0x00,0xC0,0x00,0xC1,0xF1,0xA0,0xFC,
	0x00,0x00,0xE0,0x00,0x01,0x00,0xA0,0xFC,
	0x00,0x00,0x00,0x01,0xC1,0xF1,0xA0,0xFC,
	0x00,0x00,0x20,0x01,0x01,0x00,0xA0,0xFC,
	0x00,0x10,0x40,0x01,0x01,0x10,0xA0,0xFC,
	0xAC,0xC5,0x67,0x01,0x91,0xD1,0xA0,0xFC,
	0x00,0x00,0x80,0x01,0x01,0x00,0xA0,0xFC,
	0x80,0x06,0x20,0xA0,0x00,0x40,0x23,0xFB,
	0x00,0x00,0x00,0x20,0x08,0x00,0x85,0x50,


	/* USE vertex shader program 
	phas #0, #0, pixel, end, parallel;
	or.skipinv r0, g25, #0x00000000;
	xor.skipinv.testz p0, r0, #0;
	p0 mov.skipinv o0, #0xBF800000; // -1
	p0 mov.skipinv o1, #0x3F800000; // 1
	p0 mov.skipinv o2, #0xB727C5AC; // -0.00001
	p0 mov.skipinv o3, #0x3F800000;
	xor.skipinv.testz p0, r0, #1;
	p0 mov.skipinv o0, #0x3F800000; // 1
	p0 mov.skipinv o1, #0x3F800000; // 1
	p0 mov.skipinv o2, #0x3727C5AC; // 0.00001
	p0 mov.skipinv o3, #0x3F800000;
	xor.skipinv.testz p0, r0, #2;
	p0 mov.skipinv o0, #0x3F800000; // 1
	p0 mov.skipinv o1, #0xBF800000; // -1
	p0 mov.skipinv o2, #0x3727C5AC; // 0.00001
	p0 mov.skipinv o3, #0x3F800000;
	emitvtx.freep #0, #0;
	or.end r0, r0, #0x00000000;
	*/	
	0x00,0x00,0x00,0x00,0x00,0x07,0x44,0xFA,
	0x80,0x2C,0x00,0x60,0x08,0x00,0x83,0x50,
	0x00,0x80,0x0C,0x20,0x81,0x01,0x89,0x48,
	0x00,0x00,0x00,0x00,0xC1,0xF3,0xA2,0xFC,
	0x00,0x00,0x20,0x00,0xC1,0xF3,0xA0,0xFC,
	0xAC,0xC5,0x47,0x00,0x91,0xD3,0xA2,0xFC,
	0x00,0x00,0x60,0x00,0xC1,0xF3,0xA0,0xFC,
	0x01,0x80,0x0C,0x20,0x81,0x01,0x89,0x48,
	0x00,0x00,0x00,0x00,0xC1,0xF3,0xA0,0xFC,
	0x00,0x00,0x20,0x00,0xC1,0xF3,0xA0,0xFC,
	0xAC,0xC5,0x47,0x00,0x91,0xD3,0xA0,0xFC,
	0x00,0x00,0x60,0x00,0xC1,0xF3,0xA0,0xFC,
	0x02,0x80,0x0C,0x20,0x81,0x01,0x89,0x48,
	0x00,0x00,0x00,0x00,0xC1,0xF3,0xA0,0xFC,
	0x00,0x00,0x20,0x00,0xC1,0xF3,0xA2,0xFC,
	0xAC,0xC5,0x47,0x00,0x91,0xD3,0xA0,0xFC,
	0x00,0x00,0x60,0x00,0xC1,0xF3,0xA0,0xFC,
	0x00,0x00,0x20,0xA0,0x00,0x50,0x23,0xFB,
	0x00,0x00,0x00,0x20,0x08,0x00,0x05,0x50,

	/* Terminate program */

	/* USE Terminate program 
	phas #0, #0, pixel, end, parallel;
	Mov.skipinv o0, #(1<<13);  Terminate present 
	Mov.skipinv o1, #0x00140014;  640 * 640 
	emitst.freep #0, #2
	mov.end r0, r0;
	*/

	0x00,0x00,0x00,0x00,0x00,0x07,0x44,0xFA,
	0x00,0x20,0x00,0x00,0x01,0x00,0xA0,0xFC,
	0x14,0x00,0x34,0x00,0x01,0x00,0xA0,0xFC,
	0x00,0x01,0x20,0xA0,0x00,0x40,0x23,0xFB,
	0x00,0x00,0x00,0x20,0x08,0x00,0x05,0x50,

};
#endif

typedef struct _SGX_SRVINIT_INIT_INFO_
{
	IMG_UINT32 ui32EventOtherPDSExec;
	IMG_UINT32 ui32EventOtherPDSData;
	IMG_UINT32 ui32EventOtherPDSInfo;
	IMG_UINT32 ui32EventPDSEnable;
	#if defined(SGX_FEATURE_MP)
	IMG_UINT32 ui32MasterEventPDSEnable;
	IMG_UINT32 ui32MasterEventPDSEnable2;
		#if defined(SGX_FEATURE_MP)
	IMG_UINT32	ui32SlaveEventOtherPDSExec;
	IMG_UINT32	ui32SlaveEventOtherPDSData;
	IMG_UINT32	ui32SlaveEventOtherPDSInfo;
		#endif
	#endif /* SGX_FEATURE_MP */
	
	SGXMK_TA3D_CTL  *psTA3DCtrl;
	IMG_UINT32				ui32DefaultISPDepthSort;
	IMG_UINT32				ui32DefaultDMSCtrl;
	#if defined(EUR_CR_PDS_VCB_PERF_CTRL)
	IMG_UINT32				ui32DefaultVCBPerfCtrl;
	#endif
} SGX_SRVINIT_INIT_INFO;

typedef struct _SGX_SCRIPT_BUILD
{
	IMG_UINT32 ui32MaxLen;
	IMG_UINT32 ui32CurrComm;
	IMG_BOOL bOutOfSpace;
	SGX_INIT_COMMAND *psCommands;
} SGX_SCRIPT_BUILD;

typedef struct _SGX_HEAP_INFO
{
	IMG_DEV_PHYADDR sPDDevPAddr;
	PVRSRV_HEAP_INFO *psKernelCodeHeap;
	PVRSRV_HEAP_INFO *psKernelDataHeap;
	PVRSRV_HEAP_INFO *psPixelShaderHeap;
	PVRSRV_HEAP_INFO *psVertexShaderHeap;
	PVRSRV_HEAP_INFO *psPDSPixelCodeDataHeap;
} SGX_HEAP_INFO;

static IMG_UINT32 ui32uKernelProgramSizes[] =
{
	sizeof(pbuKernelProgram),
#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH) \
	|| defined(SGX_FEATURE_ISP_CONTEXT_SWITCH) \
	|| defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
#if defined(ALLOW_EDM_PROFILES)
	sizeof(pbuKernelProgram1),
	sizeof(pbuKernelProgram2),
	sizeof(pbuKernelProgram3),
#endif /* ALLOW_EDM_PROFILES */
#endif
};

#if !defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
static IMG_UINT32 ui32uKernelTAProgramSizes[] =
{
	sizeof(pbuKernelTAProgram),
#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH) \
	|| defined(SGX_FEATURE_ISP_CONTEXT_SWITCH) \
	|| defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
#if defined(ALLOW_EDM_PROFILES)
	sizeof(pbuKernelTAProgram1),
	sizeof(pbuKernelTAProgram2),
	sizeof(pbuKernelTAProgram3),
#endif /* ALLOW_EDM_PROFILES */
#endif
};

static IMG_UINT32 ui32uKernel3DProgramSizes[] =
{
	sizeof(pbuKernel3DProgram),
#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH) \
	|| defined(SGX_FEATURE_ISP_CONTEXT_SWITCH) \
	|| defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
#if defined(ALLOW_EDM_PROFILES)
	sizeof(pbuKernel3DProgram1),
	sizeof(pbuKernel3DProgram2),
	sizeof(pbuKernel3DProgram3),
#endif /* ALLOW_EDM_PROFILES */
#endif
};
#endif /* SGX_FEATURE_MONOLITHIC_UKERNEL */

static const IMG_UINT8 * pbuKernelProgramBlocks[] =
{
	pbuKernelProgram,
#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH) \
	|| defined(SGX_FEATURE_ISP_CONTEXT_SWITCH) \
	|| defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
#if defined(ALLOW_EDM_PROFILES)
	pbuKernelProgram1,
	pbuKernelProgram2,
	pbuKernelProgram3,
#endif /* ALLOW_EDM_PROFILES */
#endif
};

#if !defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
static const IMG_UINT8 * pbuKernelTAProgramBlocks[] =
{
	pbuKernelTAProgram,
#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH) \
	|| defined(SGX_FEATURE_ISP_CONTEXT_SWITCH) \
	|| defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
#if defined(ALLOW_EDM_PROFILES)
	pbuKernelTAProgram1,
	pbuKernelTAProgram2,
	pbuKernelTAProgram3,
#endif /* ALLOW_EDM_PROFILES */
#endif
};

static const IMG_UINT8 * pbuKernel3DProgramBlocks[] =
{
	pbuKernel3DProgram,
#if defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH) \
	|| defined(SGX_FEATURE_ISP_CONTEXT_SWITCH) \
	|| defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
#if defined(ALLOW_EDM_PROFILES)
	pbuKernel3DProgram1,
	pbuKernel3DProgram2,
	pbuKernel3DProgram3,
#endif /* ALLOW_EDM_PROFILES */
#endif
};
#endif /* SGX_FEATURE_MONOLITHIC_UKERNEL */

#if (defined(SGX_FEATURE_SW_ISP_CONTEXT_SWITCH) \
	|| defined(SGX_FEATURE_ISP_CONTEXT_SWITCH) \
	|| defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)) \
	&& defined(ALLOW_EDM_PROFILES)

/* PRQA S 0881 8 */ /* ignore 'order of evaluation' warning */
#define DECLARE_OFFSET_TABLE(NAME) \
static IMG_UINT32 ui32uKernelProgramOffset##NAME[] = \
{ \
	UKERNEL_##NAME##_OFFSET, \
	UKERNEL1_##NAME##_OFFSET, \
	UKERNEL2_##NAME##_OFFSET, \
	UKERNEL3_##NAME##_OFFSET, \
};

#else /* ALLOW_EDM_PROFILES */

/* PRQA S 0881 5 */ /* ignore 'order of evaluation' warning */
#define DECLARE_OFFSET_TABLE(NAME) \
static IMG_UINT32 ui32uKernelProgramOffset##NAME[] = \
{ \
	UKERNEL_##NAME##_OFFSET, \
};

#endif /* ALLOW_EDM_PROFILES */

/* PRQA S 0881 18 */ /* ignore 'order of evaluation' warning */
#define DECLARE_OFFSET_TABLE_GETTER(NAME) \
DECLARE_OFFSET_TABLE(NAME)																			\
static IMG_UINT32 GetuKernelProgram##NAME##Offset(IMG_VOID)											\
{																									\
	IMG_UINT32	ui32CodeProfile;																	\
																									\
	ui32CodeProfile = 0;																			\
																									\
	/* Try and read which profile has been selected */												\
	OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "EDMTACodeProfile", &ui32CodeProfile);	\
																									\
	if(ui32CodeProfile >= (sizeof(ui32uKernelProgramOffset##NAME) / sizeof(IMG_UINT32)))			\
	{																								\
		ui32CodeProfile = 0;																		\
	}																								\
																									\
	return ui32uKernelProgramOffset##NAME[ui32CodeProfile];											\
}



#if defined(FIX_HW_BRN_31542) || defined(FIX_HW_BRN_36513)
DECLARE_OFFSET_TABLE_GETTER(IKP_LIMMUSECODEBASEADDR)
DECLARE_OFFSET_TABLE_GETTER(IKP_LIMMPSGRGNHDRADDR)
DECLARE_OFFSET_TABLE_GETTER(IKP_LIMMTPCBASEADDR)
DECLARE_OFFSET_TABLE_GETTER(IKP_LIMMPARAMMEMADDR)
DECLARE_OFFSET_TABLE_GETTER(IKP_LIMMDPMPTBASEADDR)
DECLARE_OFFSET_TABLE_GETTER(IKP_LIMMVDMSTREAMBASEADDR)
#endif

DECLARE_OFFSET_TABLE_GETTER(INITUKERNELPRIMARY1)
DECLARE_OFFSET_TABLE_GETTER(IUKP1_PDSCONSTSIZESEC)
DECLARE_OFFSET_TABLE_GETTER(IUKP1_PDSDEVADDRSEC)
DECLARE_OFFSET_TABLE_GETTER(IUKP1_TA3DCTL)
DECLARE_OFFSET_TABLE_GETTER(IUKP1_PDSCONSTSIZEPRIM2)
DECLARE_OFFSET_TABLE_GETTER(IUKP1_PDSDEVADDRPRIM2)
DECLARE_OFFSET_TABLE_GETTER(INITUKERNELSECONDARY)
DECLARE_OFFSET_TABLE_GETTER(INITUKERNELPRIMARY2)
DECLARE_OFFSET_TABLE_GETTER(IUKP2_PDSEXEC)
DECLARE_OFFSET_TABLE_GETTER(IUKP2_PDSDATA)
DECLARE_OFFSET_TABLE_GETTER(EVENTHANDLER)

DECLARE_OFFSET_TABLE_GETTER(HOSTKICKPOWER)
DECLARE_OFFSET_TABLE_GETTER(HOSTKICKCONTEXTSUSPEND)
#if defined(SGX_FEATURE_2D_HARDWARE)
DECLARE_OFFSET_TABLE_GETTER(HOSTKICK2D)
#endif /* SGX_FEATURE_2D_HARDWARE */
DECLARE_OFFSET_TABLE_GETTER(HOSTKICKTRANSFER)
DECLARE_OFFSET_TABLE_GETTER(HOSTKICKTA)
DECLARE_OFFSET_TABLE_GETTER(HOSTKICKCLEANUP)
DECLARE_OFFSET_TABLE_GETTER(HOSTKICKPROCESS_QUEUES)
DECLARE_OFFSET_TABLE_GETTER(HOSTKICKGETMISCINFO)
#if defined(SGX_FEATURE_DATA_BREAKPOINTS)
DECLARE_OFFSET_TABLE_GETTER(HOSTKICKDATABREAKPOINT)
#endif
DECLARE_OFFSET_TABLE_GETTER(HOSTKICKSETHWPERFSTATUS)
#if defined(FIX_HW_BRN_31620)
DECLARE_OFFSET_TABLE_GETTER(HOSTKICKFLUSHPDCACHE)
#endif

DECLARE_OFFSET_TABLE_GETTER(ELB_PDSCONSTSIZE)
DECLARE_OFFSET_TABLE_GETTER(ELB_PDSDEVADDR)

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
DECLARE_OFFSET_TABLE_GETTER(STACS_TERMSTATE0)
DECLARE_OFFSET_TABLE_GETTER(STACS_TERMSTATE1)
#endif

#if defined(SGX_FAST_DPM_INIT)
#if (SGX_FEATURE_NUM_USE_PIPES < 2)
#error ("sgxinit.c: SGX_FAST_DPM_INIT cannot be used on cores with < 2 USE pipes!")
#else
DECLARE_OFFSET_TABLE_GETTER(SDPT_PDSCONSTSIZE)
DECLARE_OFFSET_TABLE_GETTER(SDPT_PDSDEVADDR)
#endif
#endif

#if defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
DECLARE_OFFSET_TABLE_GETTER(TAHANDLER)
DECLARE_OFFSET_TABLE_GETTER(TAEVENTHANDLER)
#if defined(SGX_FEATURE_MK_SUPERVISOR)
DECLARE_OFFSET_TABLE_GETTER(SUPERVISOR)
DECLARE_OFFSET_TABLE_GETTER(SPRV_PHAS)
DECLARE_OFFSET_TABLE_GETTER(SPRVRETURN)
#endif

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
DECLARE_OFFSET_TABLE_GETTER(RTA_MTESTATERESTORE0)
DECLARE_OFFSET_TABLE_GETTER(RTA_MTESTATERESTORE1)
#if defined(SGX_FEATURE_MTE_STATE_FLUSH) && defined(SGX_FEATURE_COMPLEX_GEOMETRY_REV_2)
DECLARE_OFFSET_TABLE_GETTER(RTA_CMPLXSTATERESTORE0)
DECLARE_OFFSET_TABLE_GETTER(RTA_CMPLXSTATERESTORE1)
#endif
#if defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
DECLARE_OFFSET_TABLE_GETTER(RTA_SARESTORE0)
DECLARE_OFFSET_TABLE_GETTER(RTA_SARESTORE1)
#endif
#endif


DECLARE_OFFSET_TABLE_GETTER(THREEDHANDLER)
DECLARE_OFFSET_TABLE_GETTER(THREEDEVENTHANDLER)
#else
DECLARE_OFFSET_TABLE_GETTER(TA_TAHANDLER)
DECLARE_OFFSET_TABLE_GETTER(TA_TAEVENTHANDLER)

DECLARE_OFFSET_TABLE_GETTER(TA_ELB_PDSCONSTSIZE)
DECLARE_OFFSET_TABLE_GETTER(TA_ELB_PDSDEVADDR)

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
DECLARE_OFFSET_TABLE_GETTER(TA_STACS_TERMSTATE0)
DECLARE_OFFSET_TABLE_GETTER(TA_STACS_TERMSTATE1)
DECLARE_OFFSET_TABLE_GETTER(TA_RTA_MTESTATERESTORE0)
DECLARE_OFFSET_TABLE_GETTER(TA_RTA_MTESTATERESTORE1)
#if defined(SGX_FEATURE_MTE_STATE_FLUSH) && defined(SGX_FEATURE_COMPLEX_GEOMETRY_REV_2)
DECLARE_OFFSET_TABLE_GETTER(TA_RTA_CMPLXSTATERESTORE0)
DECLARE_OFFSET_TABLE_GETTER(TA_RTA_CMPLXSTATERESTORE1)
#endif
#if defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
DECLARE_OFFSET_TABLE_GETTER(TA_RTA_SASTATERESTORE0)
DECLARE_OFFSET_TABLE_GETTER(TA_RTA_SASTATERESTORE1)
#endif
#endif


#if defined(SGX_FAST_DPM_INIT) && (SGX_FEATURE_NUM_USE_PIPES > 1)
DECLARE_OFFSET_TABLE_GETTER(TA_SDPT_PDSCONSTSIZE)
DECLARE_OFFSET_TABLE_GETTER(TA_SDPT_PDSDEVADDR)
#endif

DECLARE_OFFSET_TABLE_GETTER(3D_THREEDHANDLER)
DECLARE_OFFSET_TABLE_GETTER(3D_THREEDEVENTHANDLER)

DECLARE_OFFSET_TABLE_GETTER(3D_ELB_PDSCONSTSIZE)
DECLARE_OFFSET_TABLE_GETTER(3D_ELB_PDSDEVADDR)
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
DECLARE_OFFSET_TABLE_GETTER(3D_STACS_TERMSTATE0)
DECLARE_OFFSET_TABLE_GETTER(3D_STACS_TERMSTATE1)
#endif
#endif /* SGX_FEATURE_MONOLITHIC_UKERNEL */

/* PRQA S 0881 3 */ /* ignore 'order of evaluation' warning */
#define INITHOSTKICKADDR(TYPE)							\
	psDevInfo->aui32HostKickAddr[SGXMKIF_CMD_##TYPE] =	\
		GetuKernelProgramHOSTKICK##TYPE##Offset() / EURASIA_USE_INSTRUCTION_SIZE


#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG) || defined(SUPPORT_SGX_HWPERF) || \
		(defined(PDUMP) && defined(EUR_CR_PERF_COUNTER_SELECT_SHIFT)) || defined(SGX_FEATURE_MP)

static IMG_VOID SGXInitGetAppHint(IMG_UINT32	ui32DefaultValue,
								  IMG_CHAR		*pszHintName,
								  IMG_UINT32	*pui32Result)
{
#if defined(LINUX)
	IMG_VOID	*pvHintState;
	PVRSRVCreateAppHintState(IMG_SRVCLIENT, 0, &pvHintState);
	PVRSRVGetAppHint(pvHintState, pszHintName, IMG_UINT_TYPE, &ui32DefaultValue, pui32Result);
	PVRSRVFreeAppHintState(IMG_SRVCLIENT, pvHintState);
#else
	PVR_UNREFERENCED_PARAMETER(pszHintName);
	*pui32Result = ui32DefaultValue;
#endif
}

#endif /* defined(PVRSRV_USSE_EDM_STATUS_DEBUG) || defined(PDUMP) || defined(SGX_FEATURE_MP) */

static IMG_BOOL OutOfScriptSpace(SGX_SCRIPT_BUILD *psScript)
{
	if (psScript->ui32CurrComm >= psScript->ui32MaxLen)
	{
		psScript->bOutOfSpace = IMG_TRUE;
	}

	return psScript->bOutOfSpace;
}

static SGX_INIT_COMMAND *NextScriptCommand(SGX_SCRIPT_BUILD *psScript)
{
	if  (OutOfScriptSpace(psScript))
	{
		return IMG_NULL;
	}

	return &psScript->psCommands[psScript->ui32CurrComm++];
}

static IMG_VOID ScriptWriteSGXReg(SGX_SCRIPT_BUILD	*psScript,
								  IMG_UINT32 		ui32Offset,
								  IMG_UINT32		ui32Value)
{
	SGX_INIT_COMMAND *psComm = NextScriptCommand(psScript);

	if (psComm != IMG_NULL)
	{
		psComm->sWriteHWReg.eOp = SGX_INIT_OP_WRITE_HW_REG;
		psComm->sWriteHWReg.ui32Offset = ui32Offset;
		psComm->sWriteHWReg.ui32Value = ui32Value;
	}

	return;
}

#if defined(PDUMP) && (defined(NO_HARDWARE) || defined(EMULATOR) || defined(EUR_CR_PERF_COUNTER_SELECT_SHIFT))
static IMG_VOID ScriptPDumpSGXReg(SGX_SCRIPT_BUILD	*psScript,
								  IMG_UINT32		ui32Offset,
								  IMG_UINT32		ui32Value)
{
	SGX_INIT_COMMAND *psComm = NextScriptCommand(psScript);

	if (psComm != IMG_NULL)
	{
		psComm->sPDumpHWReg.eOp = SGX_INIT_OP_PDUMP_HW_REG;
		psComm->sPDumpHWReg.ui32Offset = ui32Offset;
		psComm->sPDumpHWReg.ui32Value = ui32Value;
	}
	return;
}
#endif

static IMG_BOOL ScriptHalt(SGX_SCRIPT_BUILD *psScript)
{
	SGX_INIT_COMMAND *psComm = NextScriptCommand(psScript);

	if (psComm != IMG_NULL)
	{
		psComm->eOp = SGX_INIT_OP_HALT;
		return IMG_TRUE;
	}

	return IMG_FALSE;
}

static IMG_UINT32 GetuKernelProgramSize(IMG_VOID)
{
	IMG_UINT32	ui32CodeProfile;
	IMG_UINT32	ui32CodeSize;

	ui32CodeProfile = 0;

	/* Try and read which profile has been selected */
	OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "EDMTACodeProfile", &ui32CodeProfile);

	if(ui32CodeProfile >= (sizeof(ui32uKernelProgramSizes) / sizeof(IMG_UINT32)))
	{
		ui32CodeProfile = 0;
	}

	ui32CodeSize = ui32uKernelProgramSizes[ui32CodeProfile];

	#if !defined(SGX_FEATURE_USE_NO_INSTRUCTION_PAIRING)
	/* Round up code size to make sure it's alligned to an instruction pair */
	ui32CodeSize = (ui32CodeSize + EURASIA_USE_INSTRUCTION_SIZE) & ~((2UL * EURASIA_USE_INSTRUCTION_SIZE) - 1);
	#endif

	return ui32CodeSize;
}

static const IMG_UINT8 * GetuKernelProgramBuffer(IMG_VOID)
{
	IMG_UINT32	ui32CodeProfile;

	ui32CodeProfile = 0;

	/* Try and read which profile has been selected */
	OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "EDMTACodeProfile", &ui32CodeProfile);

	if(ui32CodeProfile >= (sizeof(pbuKernelProgramBlocks) / sizeof(IMG_PUINT8)))
	{
		ui32CodeProfile = 0;
	}

	return pbuKernelProgramBlocks[ui32CodeProfile];
}

#if !defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
#if defined(SGX_FEATURE_2D_HARDWARE)
static IMG_UINT32 GetuKernel2DProgramSize(IMG_VOID)
{
	IMG_UINT32	ui32CodeSize;
	ui32CodeSize = sizeof(pbuKernel2DProgram);

	#if !defined(SGX_FEATURE_USE_NO_INSTRUCTION_PAIRING)
	/* Round up code size to make sure it's alligned to an instruction pair */
	ui32CodeSize = (ui32CodeSize + EURASIA_USE_INSTRUCTION_SIZE) & ~((2 * EURASIA_USE_INSTRUCTION_SIZE) - 1);
	#endif

	return ui32CodeSize;
}
static const IMG_UINT8 *GetuKernel2DProgramBuffer(IMG_VOID)
{
	return pbuKernel2DProgram;
}
#endif /* SGX_FEATURE_2D_HARDWARE */

static IMG_UINT32 GetuKernelTAProgramSize(IMG_VOID)
{
	IMG_UINT32	ui32CodeProfile;
	IMG_UINT32	ui32CodeSize;

	ui32CodeProfile = 0;

	/* Try and read which profile has been selected */
	OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "EDMTACodeProfile", &ui32CodeProfile);

	if(ui32CodeProfile >= (sizeof(ui32uKernelTAProgramSizes) / sizeof(IMG_UINT32)))
	{
		ui32CodeProfile = 0;
	}

	ui32CodeSize = ui32uKernelTAProgramSizes[ui32CodeProfile];

	#if !defined(SGX_FEATURE_USE_NO_INSTRUCTION_PAIRING)
	/* Round up code size to make sure it's alligned to an instruction pair */
	ui32CodeSize = (ui32CodeSize + EURASIA_USE_INSTRUCTION_SIZE) & ~((2UL * EURASIA_USE_INSTRUCTION_SIZE) - 1);
	#endif

	return ui32CodeSize;
}

static const IMG_UINT8 * GetuKernelTAProgramBuffer(IMG_VOID)
{
	IMG_UINT32	ui32CodeProfile;

	ui32CodeProfile = 0;

	/* Try and read which profile has been selected */
	OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "EDMTACodeProfile", &ui32CodeProfile);

	if(ui32CodeProfile >= (sizeof(pbuKernelTAProgramBlocks) / sizeof(IMG_PUINT8)))
	{
		ui32CodeProfile = 0;
	}

	return pbuKernelTAProgramBlocks[ui32CodeProfile];
}

static IMG_UINT32 GetuKernelSPMProgramSize(IMG_VOID)
{
	IMG_UINT32	ui32CodeSize;
	ui32CodeSize = sizeof(pbuKernelSPMProgram);

	#if !defined(SGX_FEATURE_USE_NO_INSTRUCTION_PAIRING)
	/* Round up code size to make sure it's alligned to an instruction pair */
	ui32CodeSize = (ui32CodeSize + EURASIA_USE_INSTRUCTION_SIZE) & ~((2 * EURASIA_USE_INSTRUCTION_SIZE) - 1);
	#endif

	return ui32CodeSize;
}
static const IMG_UINT8 * GetuKernelSPMProgramBuffer(IMG_VOID)
{
	return pbuKernelSPMProgram;
}

static IMG_UINT32 GetuKernel3DProgramSize(IMG_VOID)
{
	IMG_UINT32	ui32CodeProfile;
	IMG_UINT32	ui32CodeSize;

	ui32CodeProfile = 0;

	/* Try and read which profile has been selected */
	OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "EDMTACodeProfile", &ui32CodeProfile);

	if(ui32CodeProfile >= (sizeof(ui32uKernelProgramSizes) / sizeof(IMG_UINT32)))
	{
		ui32CodeProfile = 0;
	}

	ui32CodeSize = ui32uKernel3DProgramSizes[ui32CodeProfile];

	#if !defined(SGX_FEATURE_USE_NO_INSTRUCTION_PAIRING)
	/* Round up code size to make sure it's alligned to an instruction pair */
	ui32CodeSize = (ui32CodeSize + EURASIA_USE_INSTRUCTION_SIZE) & ~((2 * EURASIA_USE_INSTRUCTION_SIZE) - 1);
	#endif

	return ui32CodeSize;
}

static const IMG_UINT8 * GetuKernel3DProgramBuffer(IMG_VOID)
{
	IMG_UINT32	ui32CodeProfile;

	ui32CodeProfile = 0;

	/* Try and read which profile has been selected */
	OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "EDMTACodeProfile", &ui32CodeProfile);

	if(ui32CodeProfile >= (sizeof(pbuKernelProgramBlocks) / sizeof(IMG_PUINT8)))
	{
		ui32CodeProfile = 0;
	}

	return pbuKernel3DProgramBlocks[ui32CodeProfile];
}
#endif /* SGX_FEATURE_MONOLITHIC_UKERNEL */

/*!
*******************************************************************************

 @Function	PatchLIMM

 @Description

 Patches a USE LIMM instruction.

 @Input psDevInfo

 @Return PVRSRV_ERROR

******************************************************************************/
static IMG_VOID PatchLIMM(IMG_UINT32 	*pui32Instruction,
							IMG_UINT32 	ui32Value)
{
	pui32Instruction[0]	&=	EURASIA_USE0_LIMM_IMML21_CLRMSK;
	pui32Instruction[0]	|= 	((ui32Value << EURASIA_USE0_LIMM_IMML21_SHIFT) & (~EURASIA_USE0_LIMM_IMML21_CLRMSK));
	pui32Instruction[1] &=	(EURASIA_USE1_LIMM_IMM2521_CLRMSK & EURASIA_USE1_LIMM_IMM3126_CLRMSK);
	pui32Instruction[1] |=	(((ui32Value >> 21) << EURASIA_USE1_LIMM_IMM2521_SHIFT) & (~EURASIA_USE1_LIMM_IMM2521_CLRMSK)) |
						 	(((ui32Value >> 26) << EURASIA_USE1_LIMM_IMM3126_SHIFT) & (~EURASIA_USE1_LIMM_IMM3126_CLRMSK));
}


#define SETSIGREGOFFSET(type, name)	\
			ps##type##SigBuffer->ui32RegisterOffset[PVRSRV_SGX_##type##SIG_##name] = \
			(EUR_CR_##name)
			
/*!
*******************************************************************************

 @Function	InitSignatureBuffers

 @Description

 Initialises the TA and 3D signature buffers for the microkernel

 @Input psDevData
 @Input psDevInfo

 @Return IMG_VOID

******************************************************************************/
static IMG_VOID InitSignatureBuffers(PVRSRV_DEV_DATA		*psDevData,
									 SGX_CLIENT_INIT_INFO	*psDevInfo)
{
	SGXMKIF_TASIG_BUFFER	*psTASigBuffer = psDevInfo->psTASigBufferMemInfo->pvLinAddr;
	SGXMKIF_3DSIG_BUFFER	*ps3DSigBuffer = psDevInfo->ps3DSigBufferMemInfo->pvLinAddr;

	#if !defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(psDevData);
	#endif

	psTASigBuffer->ui32NumSignatures = PVRSRV_SGX_TASIG_NUM;
	psTASigBuffer->ui32NumSamples = 0;
	ps3DSigBuffer->ui32NumSignatures = PVRSRV_SGX_3DSIG_NUM;
	ps3DSigBuffer->ui32NumSamples = 0;

	/* TA signatures. */
	#if defined(EUR_CR_TE_CHECKSUM)
		SETSIGREGOFFSET(TA, CLIP_CHECKSUM);
		SETSIGREGOFFSET(TA, MTE_MEM_CHECKSUM);
		SETSIGREGOFFSET(TA, MTE_TE_CHECKSUM);
		SETSIGREGOFFSET(TA, TE_CHECKSUM);
	#else
		SETSIGREGOFFSET(TA, CLIP_SIG1);
		SETSIGREGOFFSET(TA, MTE_SIG1);
		SETSIGREGOFFSET(TA, MTE_SIG2);
		#if defined(EUR_CR_TE1)
		SETSIGREGOFFSET(TA, TE1);
		SETSIGREGOFFSET(TA, TE2);
		#endif
		#if defined(EUR_CR_VDM_MTE)
		SETSIGREGOFFSET(TA, VDM_MTE);
		#endif
	#endif /* EUR_CR_TE_CHECKSUM */

	/* 3D signatures. */
	#if defined(EUR_CR_ISP_FPU_CHECKSUM)
		SETSIGREGOFFSET(3D, ISP_FPU_CHECKSUM);
		#if defined(EUR_CR_ISP2_PRECALC_CHECKSUM)
		SETSIGREGOFFSET(3D, ISP2_PRECALC_CHECKSUM);
		SETSIGREGOFFSET(3D, ISP2_EDGE_CHECKSUM);
		SETSIGREGOFFSET(3D, ISP2_TAGWRITE_CHECKSUM);
		SETSIGREGOFFSET(3D, ISP2_SPAN_CHECKSUM);
		SETSIGREGOFFSET(3D, PBE_CHECKSUM);
		#else
		SETSIGREGOFFSET(3D, ISP_PRECALC_CHECKSUM);
		SETSIGREGOFFSET(3D, ISP_EDGE_CHECKSUM);
		SETSIGREGOFFSET(3D, ISP_TAGWRITE_CHECKSUM);
		SETSIGREGOFFSET(3D, ISP_SPAN_CHECKSUM);
		SETSIGREGOFFSET(3D, PBE_PIXEL_CHECKSUM);
		SETSIGREGOFFSET(3D, PBE_NONPIXEL_CHECKSUM);
		#endif
	#else
		SETSIGREGOFFSET(3D, ISP_FPU);
		#if defined(EUR_CR_ISP_PIPE0_SIG1)
		SETSIGREGOFFSET(3D, ISP_PIPE0_SIG1);
		SETSIGREGOFFSET(3D, ISP_PIPE0_SIG2);
		SETSIGREGOFFSET(3D, ISP_PIPE0_SIG3);
		SETSIGREGOFFSET(3D, ISP_PIPE0_SIG4);
		SETSIGREGOFFSET(3D, ISP_PIPE1_SIG1);
		SETSIGREGOFFSET(3D, ISP_PIPE1_SIG2);
		SETSIGREGOFFSET(3D, ISP_PIPE1_SIG3);
		SETSIGREGOFFSET(3D, ISP_PIPE1_SIG4);
		#endif
		#if defined(EUR_CR_ISP_SIG1)
		SETSIGREGOFFSET(3D, ISP_SIG1);
		SETSIGREGOFFSET(3D, ISP_SIG2);
		SETSIGREGOFFSET(3D, ISP_SIG3);
		SETSIGREGOFFSET(3D, ISP_SIG4);
		#endif

		#if defined(EUR_CR_ISP2_ZLS_LOAD_DATA0)
		/* ZLS signature registers */
		SETSIGREGOFFSET(3D, ISP2_ZLS_LOAD_DATA0);
		SETSIGREGOFFSET(3D, ISP2_ZLS_LOAD_DATA1);
		SETSIGREGOFFSET(3D, ISP2_ZLS_LOAD_STENCIL0);
		SETSIGREGOFFSET(3D, ISP2_ZLS_LOAD_STENCIL1);
		SETSIGREGOFFSET(3D, ISP2_ZLS_STORE_DATA0);
		SETSIGREGOFFSET(3D, ISP2_ZLS_STORE_DATA1);
		SETSIGREGOFFSET(3D, ISP2_ZLS_STORE_STENCIL0);
		SETSIGREGOFFSET(3D, ISP2_ZLS_STORE_STENCIL1);
		SETSIGREGOFFSET(3D, ISP2_ZLS_STORE_BIF);
		#endif

		#if defined(EUR_CR_ITR_TAG0)
		/* Iterator TAG signature registers */
		SETSIGREGOFFSET(3D, ITR_TAG0);
		SETSIGREGOFFSET(3D, ITR_TAG1);
		#endif

		#if defined(EUR_CR_ITR_TAG00)
		/* Iterator TAG signature registers */
		SETSIGREGOFFSET(3D, ITR_TAG00);
		SETSIGREGOFFSET(3D, ITR_TAG01);
		SETSIGREGOFFSET(3D, ITR_TAG10);
		SETSIGREGOFFSET(3D, ITR_TAG11);
		#endif

		#if defined(EUR_CR_TF_SIG00)
		/* TF signature registers */
		SETSIGREGOFFSET(3D, TF_SIG00);
		SETSIGREGOFFSET(3D, TF_SIG01);
		SETSIGREGOFFSET(3D, TF_SIG02);
		SETSIGREGOFFSET(3D, TF_SIG03);
		#endif

		#if defined(EUR_CR_ITR_USE0)
		/* Iterator USE signature registers */
		SETSIGREGOFFSET(3D, ITR_USE0);
		SETSIGREGOFFSET(3D, ITR_USE1);
		SETSIGREGOFFSET(3D, ITR_USE2);
		SETSIGREGOFFSET(3D, ITR_USE3);
		#endif

		/* PIXELBE signature registers */
		#if defined(EUR_CR_PIXELBE_SIG01)
		SETSIGREGOFFSET(3D, PIXELBE_SIG01);
		SETSIGREGOFFSET(3D, PIXELBE_SIG02);
		#endif
		#if defined(EUR_CR_PIXELBE_SIG11)
		SETSIGREGOFFSET(3D, PIXELBE_SIG11);
		SETSIGREGOFFSET(3D, PIXELBE_SIG12);
		#endif
	#endif /* EUR_CR_ISP_FPU_CHECKSUM */

	#if defined(PDUMP)
	PDUMPCOMMENT(psDevData->psConnection, "Initialise the TA signature buffer", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psTASigBufferMemInfo,
			 0, sizeof(*psTASigBuffer), PDUMP_FLAGS_CONTINUOUS);
	PDUMPCOMMENT(psDevData->psConnection, "Initialise the 3D signature buffer", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->ps3DSigBufferMemInfo,
			 0, sizeof(*ps3DSigBuffer), PDUMP_FLAGS_CONTINUOUS);
	#endif /* PDUMP */
}


#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
/*****************************************************************************
 FUNCTION	: LoadCtxSwitchStateTermProgram

 PURPOSE	: Load the PDS and USE programs required to send the MTE state
 				terminate and therefore cause a VDM context switch

 PARAMETERS	:

 RETURNS	: PVRSRV_ERROR
*****************************************************************************/
static PVRSRV_ERROR LoadCtxSwitchStateTermProgram(PVRSRV_DEV_DATA *psDevData,
													SGX_HEAP_INFO *psHeapInfo,
													SGX_CLIENT_INIT_INFO *psDevInfo)
{
	PVRSRV_CLIENT_MEM_INFO	*psUSEMemInfo;
	PVRSRV_CLIENT_MEM_INFO	*psPDSMemInfo;
	IMG_UINT32				aui32USETaskControl[3];
	IMG_UINT32				ui32TATermState[2];
	IMG_UINT32				ui32AttributeSize;
	IMG_UINT32				ui32USENumTemps;
	

	/* Allocate memory for the USE program */
	PDUMPCOMMENT(psDevData->psConnection, "Allocate VDM Context Store Terminate USE Program", IMG_TRUE);
	if (PVRSRVAllocDeviceMem(psDevData,
							 psHeapInfo->psKernelCodeHeap->hDevMemHeap,
							 PVRSRV_MEM_READ | PVRSRV_MEM_NO_SYNCOBJ,
							 sizeof(pbUSEProgramVDMContextStore),
							 EURASIA_PDS_DOUTU_PHASE_START_ALIGN,
							 &psDevInfo->psUSECtxSwitchTermMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"LoadCtxSwitchStateTermProgram : Failed to allocate USE program"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	psUSEMemInfo = psDevInfo->psUSECtxSwitchTermMemInfo;

	/* Copy the USE code to the allocated memory */
	PVRSRVMemCopy((IMG_UINT8 *)psUSEMemInfo->pvLinAddr,
			  		pbUSEProgramVDMContextStore, sizeof(pbUSEProgramVDMContextStore));

#if defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSEMemInfo->pvLinAddr + TACS_TERM_SABUFFERBASEADDR_OFFSET),
				psDevInfo->psTA3DCtlMemInfo->sDevVAddr.uiAddr + offsetof(SGXMK_TA3D_CTL, sVDMSABufferDevAddr));
#endif
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) && \
	defined(FIX_HW_BRN_33657) && defined(SUPPORT_SECURE_33657_FIX)
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSEMemInfo->pvLinAddr + TACS_TERM_TERMINATEFLAGSBASEADDR_OFFSET),
				psDevInfo->psTA3DCtlMemInfo->sDevVAddr.uiAddr + offsetof(SGXMK_TA3D_CTL, ui32VDMCtxTerminateMode));
	PVR_DPF((PVR_DBG_MESSAGE, "LoadCtxSwitchStateTermProgram : Setting prog data at 0x%x to value 0x%x.",
				(IMG_UINT32)((IMG_PUINT8)psUSEMemInfo->pvLinAddr + TACS_TERM_TERMINATEFLAGSBASEADDR_OFFSET),
				psDevInfo->psTA3DCtlMemInfo->sDevVAddr.uiAddr + offsetof(SGXMK_TA3D_CTL, ui32VDMCtxTerminateMode) ));
#endif

	PDUMPCOMMENT(psDevData->psConnection, "VDM Context Store Terminate USE Program", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSEMemInfo,
				0, sizeof(pbUSEProgramVDMContextStore), PDUMP_FLAGS_CONTINUOUS);

	/* Set the temp register count */
#if defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
	ui32USENumTemps = ALIGNSIZE(3, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT);
#else
	ui32USENumTemps = 0;
#endif

	/* Setup the USE task control words */
	#if defined(EURASIA_PDS_DOUTU0_TRC_SHIFT)
	aui32USETaskControl[0]	= ((ALIGNFIELD(ui32USENumTemps, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT)
							<< EURASIA_PDS_DOUTU0_TRC_SHIFT) & ~EURASIA_PDS_DOUTU0_TRC_CLRMSK);
	aui32USETaskControl[1]	= EURASIA_PDS_DOUTU1_MODE_PARALLEL;
	aui32USETaskControl[2]	= 0;
	#else
	aui32USETaskControl[0]	= 0;
	aui32USETaskControl[1]	= EURASIA_PDS_DOUTU1_MODE_PARALLEL |
						  ((ALIGNFIELD(ui32USENumTemps, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT)
						  	<< EURASIA_PDS_DOUTU1_TRC_SHIFT) & ~EURASIA_PDS_DOUTU1_TRC_CLRMSK);
	aui32USETaskControl[2]	= ((ALIGNFIELD(ui32USENumTemps, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT)
							>> EURASIA_PDS_DOUTU2_TRC_INTERNALSHIFT) << EURASIA_PDS_DOUTU2_TRC_SHIFT);
	#endif
	
	SetUSEExecutionAddress(&aui32USETaskControl[0], 0, psUSEMemInfo->sDevVAddr,
							psHeapInfo->psKernelCodeHeap->sDevVAddrBase, SGX_KERNEL_USE_CODE_BASE_INDEX);

	/* Allocate memory for the PDS program */
	PDUMPCOMMENT(psDevData->psConnection, "Allocate VDM Context Store Terminate PDS Program", IMG_TRUE);
	if (PVRSRVAllocDeviceMem(psDevData,
							 psHeapInfo->psKernelCodeHeap->hDevMemHeap,
							 PVRSRV_MEM_READ | PVRSRV_MEM_NO_SYNCOBJ,
							 sizeof(g_pui32PDSVDMCONTEXT_STORE),
							 (1 << EUR_CR_EVENT_PIXEL_PDS_EXEC_ADDR_ALIGNSHIFT),
							 &psDevInfo->psPDSCtxSwitchTermMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"LoadCtxSwitchStateTermProgram : Failed to allocate PDS program"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	psPDSMemInfo = psDevInfo->psPDSCtxSwitchTermMemInfo;

	/* Copy the PDS program */
	PVRSRVMemCopy((IMG_UINT8 *)psPDSMemInfo->pvLinAddr,
					(IMG_PUINT8)g_pui32PDSVDMCONTEXT_STORE,
					sizeof(g_pui32PDSVDMCONTEXT_STORE));


	/* Fill in the contants */
	PDSVDMCONTEXT_STORESetINPUT_DOUTU1((IMG_PUINT32)psPDSMemInfo->pvLinAddr, aui32USETaskControl[1]);
	PDSVDMCONTEXT_STORESetINPUT_DOUTU2((IMG_PUINT32)psPDSMemInfo->pvLinAddr, aui32USETaskControl[2]);
	PDSVDMCONTEXT_STORESetINPUT_DOUTU0((IMG_PUINT32)psPDSMemInfo->pvLinAddr, aui32USETaskControl[0]);

	PDUMPCOMMENT(psDevData->psConnection, "VDM Context Store Terminate PDS Program", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psPDSMemInfo,
				0, sizeof(g_pui32PDSVDMCONTEXT_STORE), PDUMP_FLAGS_CONTINUOUS);

	
	#if defined(SGX_FEATURE_UNIFIED_TEMPS_AND_PAS) && defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
	ui32AttributeSize = ui32USENumTemps;
	ui32AttributeSize = ALIGNSIZE(ui32AttributeSize * sizeof(IMG_UINT32), EURASIA_TAPDSSTATE_USEATTRIBUTESIZE_ALIGNSHIFT);
	// convert into 128 Words (4 dwords)
	ui32AttributeSize = ui32AttributeSize >> EURASIA_TAPDSSTATE_USEATTRIBUTESIZE_ALIGNSHIFT;
	#else
	ui32AttributeSize = 0;
	#endif

	ui32TATermState[0] =  EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_TERMINATE_MASK |
	#if defined(SGX_FEATURE_EDM_VERTEX_PDSADDR_FULL_RANGE)
							(((psDevInfo->psPDSCtxSwitchTermMemInfo->sDevVAddr.uiAddr >> EURASIA_TAPDSSTATE_BASEADDR_ALIGNSHIFT) 
										<< EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_BASEADDR_SHIFT) 
											& EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_BASEADDR_MASK);
	#else
							((((psDevInfo->psPDSCtxSwitchTermMemInfo->sDevVAddr.uiAddr - psDevInfo->sDevVAddrPDSExecBase.uiAddr) 
									>> EURASIA_TAPDSSTATE_BASEADDR_ALIGNSHIFT) 
									<< EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_BASEADDR_SHIFT) & EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_BASEADDR_MASK);
	#endif

	ui32TATermState[1] = ((PDS_VDMCONTEXT_STORE_DATA_SEGMENT_SIZE >> EURASIA_TAPDSSTATE_DATASIZE_ALIGNSHIFT)
										<< EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_DATASIZE_SHIFT) |
										EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_MTE_EMIT_MASK |
										EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_SD_MASK |
										(SGX_FEATURE_NUM_USE_PIPES << EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_USEPIPE_SHIFT) |
	#if defined(SGX545)
										(2UL << EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_PARTITIONS_SHIFT) |
	#else
										(1UL << EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_PARTITIONS_SHIFT) |
	#endif
										(ui32AttributeSize << EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_USEATTRIBUTESIZE_SHIFT);
	
	PDUMPCOMMENT(psDevData->psConnection, "Patch Ctx Switch State Terminate words", IMG_TRUE);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelUSEMemInfo->pvLinAddr + GetuKernelProgramSTACS_TERMSTATE0Offset()), ui32TATermState[0]);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelUSEMemInfo->pvLinAddr + GetuKernelProgramSTACS_TERMSTATE1Offset()), ui32TATermState[1]);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelUSEMemInfo, 
				GetuKernelProgramSTACS_TERMSTATE0Offset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelUSEMemInfo, 
				GetuKernelProgramSTACS_TERMSTATE1Offset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	#if !defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelTAUSEMemInfo->pvLinAddr + GetuKernelProgramTA_STACS_TERMSTATE0Offset()), ui32TATermState[0]);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelTAUSEMemInfo->pvLinAddr + GetuKernelProgramTA_STACS_TERMSTATE1Offset()), ui32TATermState[1]);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernel3DUSEMemInfo->pvLinAddr + GetuKernelProgram3D_STACS_TERMSTATE0Offset()), ui32TATermState[0]);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernel3DUSEMemInfo->pvLinAddr + GetuKernelProgram3D_STACS_TERMSTATE1Offset()), ui32TATermState[1]);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelSPMUSEMemInfo->pvLinAddr + UKERNEL_SPM_STACS_TERMSTATE0_OFFSET), ui32TATermState[0]);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelSPMUSEMemInfo->pvLinAddr + UKERNEL_SPM_STACS_TERMSTATE0_OFFSET), ui32TATermState[1]);

	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelTAUSEMemInfo, GetuKernelProgramTA_STACS_TERMSTATE0Offset(), 
				EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelTAUSEMemInfo, GetuKernelProgramTA_STACS_TERMSTATE1Offset(), 
				EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernel3DUSEMemInfo, GetuKernelProgram3D_STACS_TERMSTATE0Offset(), 
				EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernel3DUSEMemInfo, GetuKernelProgram3D_STACS_TERMSTATE1Offset(), 
				EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelSPMUSEMemInfo, UKERNEL_SPM_STACS_TERMSTATE0_OFFSET, 
				EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelSPMUSEMemInfo, UKERNEL_SPM_STACS_TERMSTATE0_OFFSET, 
				EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
		#if defined(SGX_FEATURE_2D_HARDWARE)
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernel2DUSEMemInfo->pvLinAddr + UKERNEL_2D_STACS_TERMSTATE0_OFFSET), ui32TATermState[0]);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernel2DUSEMemInfo->pvLinAddr + UKERNEL_2D_STACS_TERMSTATE1_OFFSET), ui32TATermState[1]);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernel2DUSEMemInfo, UKERNEL_2D_STACS_TERMSTATE0_OFFSET, 
				EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernel2DUSEMemInfo, UKERNEL_2D_STACS_TERMSTATE1_OFFSET, 
				EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
		#endif
	#endif
	
	return PVRSRV_OK;
}

/*!
*******************************************************************************

 @Function	LoadTAStateRestoreProgram

 @Description

 Loads the TA state restore program

 @Input none

 @Return none

******************************************************************************/
static PVRSRV_ERROR LoadTAStateRestoreProgram(PVRSRV_DEV_DATA *psDevData,
												SGX_HEAP_INFO *psHeapInfo,
												SGX_CLIENT_INIT_INFO *psDevInfo)
{
	PVRSRV_CLIENT_MEM_INFO	*psUSEMemInfo;
	PVRSRV_CLIENT_MEM_INFO	*psPDSMemInfo;
	IMG_UINT32	ui32USEProgramSize;
	IMG_UINT32	ui32USENumTemps;
	IMG_UINT32	ui32AttributeSize;
	IMG_UINT32	aui32USETaskCtrl[3];
	IMG_UINT32	ui32PDSMTEStateRestore[2];
	
	/*********************************/
	/* MTE State Update PDS/USE Task */
	/*********************************/

	ui32USEProgramSize = sizeof(pbUSEProgramTAStateRestore);

	/* Allocate memory for the USE program */
	PDUMPCOMMENT(psDevData->psConnection, "Allocate TA State Restore USE Program", IMG_TRUE);
	if (PVRSRVAllocDeviceMem(psDevData,
								 psHeapInfo->psKernelCodeHeap->hDevMemHeap,
								 PVRSRV_MEM_READ | PVRSRV_MEM_NO_SYNCOBJ,
								 ui32USEProgramSize,
								 EURASIA_PDS_DOUTU_PHASE_START_ALIGN,
								 &psDevInfo->psUSETAStateMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"LoadTAStateRestoreProgram : Failed to allocate USE program"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	psUSEMemInfo = psDevInfo->psUSETAStateMemInfo;

	/* Copy the USE code to the allocated memory */
	PVRSRVMemCopy((IMG_UINT8 *) psUSEMemInfo->pvLinAddr,
					pbUSEProgramTAStateRestore,
					ui32USEProgramSize);

	/* Pdump the USE code */
	PDUMPCOMMENT(psDevData->psConnection, "USE code for TA state restore program", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSEMemInfo,
				0, ui32USEProgramSize, PDUMP_FLAGS_CONTINUOUS);

	/* Set the temp register count */
	ui32USENumTemps = MAX(SGX_USE_MINTEMPREGS, 0);
	ui32USENumTemps = ALIGNSIZE(ui32USENumTemps, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT);
	
	/* Setup the USE task control words */
	#if defined(EURASIA_PDS_DOUTU0_TRC_SHIFT)
	aui32USETaskCtrl[0]	= ((ALIGNFIELD(ui32USENumTemps, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT)
							<< EURASIA_PDS_DOUTU0_TRC_SHIFT) & ~EURASIA_PDS_DOUTU0_TRC_CLRMSK);
	aui32USETaskCtrl[1]	= EURASIA_PDS_DOUTU1_MODE_PERINSTANCE;
	aui32USETaskCtrl[2]	= 0;
	#else
	aui32USETaskCtrl[0]	= EURASIA_PDS_DOUTU0_PDSDMADEPENDENCY;
	aui32USETaskCtrl[1]	= EURASIA_PDS_DOUTU1_MODE_PERINSTANCE |
						  ((ALIGNFIELD(ui32USENumTemps, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT)
						  	<< EURASIA_PDS_DOUTU1_TRC_SHIFT) & ~EURASIA_PDS_DOUTU1_TRC_CLRMSK);
	aui32USETaskCtrl[2]	= ((ALIGNFIELD(ui32USENumTemps, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT)
							>> EURASIA_PDS_DOUTU2_TRC_INTERNALSHIFT) << EURASIA_PDS_DOUTU2_TRC_SHIFT);
	#endif

	/* Setup the USE code address */
	SetUSEExecutionAddress(&aui32USETaskCtrl[0], 0, psUSEMemInfo->sDevVAddr,
							psHeapInfo->psKernelCodeHeap->sDevVAddrBase, SGX_KERNEL_USE_CODE_BASE_INDEX);

	/* Allocate memory for the PDS program */
	PDUMPCOMMENT(psDevData->psConnection, "Allocate TA State Restore PDS Program", IMG_TRUE);
	if (PVRSRVAllocDeviceMem(psDevData,
								 psHeapInfo->psKernelCodeHeap->hDevMemHeap,
								 PVRSRV_MEM_READ | PVRSRV_MEM_NO_SYNCOBJ,
								 sizeof(g_pui32PDSTASTATERESTORE),
								 (1 << EUR_CR_EVENT_PIXEL_PDS_EXEC_ADDR_ALIGNSHIFT),
								 &psDevInfo->psPDSTAStateMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"LoadTAStateRestoreProgram : Failed to allocate PDS program"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	psPDSMemInfo = psDevInfo->psPDSTAStateMemInfo;

	/* Copy the PDS program */
	PVRSRVMemCopy((IMG_UINT8 *)psPDSMemInfo->pvLinAddr,
					(IMG_PUINT8)g_pui32PDSTASTATERESTORE,
					sizeof(g_pui32PDSTASTATERESTORE));

	/* Fill in the contants */
	PDSTASTATERESTORESetINPUT_TA3DCTRL_ADDR((IMG_PUINT32)psPDSMemInfo->pvLinAddr,
											psDevInfo->psTA3DCtlMemInfo->sDevVAddr.uiAddr);
	PDSTASTATERESTORESetINPUT_TA3DCTRL_PA_DEST((IMG_PUINT32)psPDSMemInfo->pvLinAddr,
												(0 << EURASIA_PDS_DOUTA1_AO_SHIFT));
	PDSTASTATERESTORESetINPUT_DOUTU0((IMG_PUINT32)psPDSMemInfo->pvLinAddr,
										aui32USETaskCtrl[0]);
	PDSTASTATERESTORESetINPUT_DOUTU1((IMG_PUINT32)psPDSMemInfo->pvLinAddr,
										aui32USETaskCtrl[1]);
	PDSTASTATERESTORESetINPUT_DOUTU2((IMG_PUINT32)psPDSMemInfo->pvLinAddr,
										aui32USETaskCtrl[2]);

	/* Pdump the PDS program */
	PDUMPCOMMENT(psDevData->psConnection, "PDS program for TA state restore", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psPDSMemInfo, 0, sizeof(g_pui32PDSTASTATERESTORE), PDUMP_FLAGS_CONTINUOUS);

	#if defined(SGX543) || defined(SGX544)
	// take into account extra space for load Address, header word and pds_batch_num
	ui32AttributeSize = EURASIA_TACTL_ALL_SIZEINDWORDS+3;
	#else
	ui32AttributeSize = EURASIA_TACTL_ALL_SIZEINDWORDS+2;
	#endif
	#if defined(SGX_FEATURE_UNIFIED_TEMPS_AND_PAS)
	ui32AttributeSize += ui32USENumTemps;
	#endif
	ui32AttributeSize = ALIGNSIZE(ui32AttributeSize * sizeof(IMG_UINT32), EURASIA_TAPDSSTATE_USEATTRIBUTESIZE_ALIGNSHIFT);
	// convert into 128 Words (4 dwords)
	ui32AttributeSize = ui32AttributeSize >> EURASIA_TAPDSSTATE_USEATTRIBUTESIZE_ALIGNSHIFT;
	

#if defined(SGX_FEATURE_EDM_VERTEX_PDSADDR_FULL_RANGE)
	ui32PDSMTEStateRestore[0] = (((psDevInfo->psPDSTAStateMemInfo->sDevVAddr.uiAddr >> EURASIA_TAPDSSTATE_BASEADDR_ALIGNSHIFT)
											<< EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_BASEADDR_SHIFT) & EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_BASEADDR_MASK);
#else
	ui32PDSMTEStateRestore[0] = ((((psDevInfo->psPDSTAStateMemInfo->sDevVAddr.uiAddr - psDevInfo->sDevVAddrPDSExecBase.uiAddr) >> EURASIA_TAPDSSTATE_BASEADDR_ALIGNSHIFT)
											<< EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_BASEADDR_SHIFT) & EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_BASEADDR_MASK);

#endif
	ui32PDSMTEStateRestore[1] = (((PDS_TASTATERESTORE_DATA_SEGMENT_SIZE >> EURASIA_TAPDSSTATE_DATASIZE_ALIGNSHIFT)
											<< EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_DATASIZE_SHIFT)
											& EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_DATASIZE_MASK) |
											EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_MTE_EMIT_MASK |
											EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_SD_MASK |
											(1UL << EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_USEPIPE_SHIFT) |
#if defined(SGX545)
											(3UL << EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_PARTITIONS_SHIFT) |
#else
											(1UL << EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_PARTITIONS_SHIFT) |
#endif
											(ui32AttributeSize << EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_USEATTRIBUTESIZE_SHIFT);
	
	PDUMPCOMMENT(psDevData->psConnection, "Patch Ctx Switch State Restore words", IMG_TRUE);
#if defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelUSEMemInfo->pvLinAddr + GetuKernelProgramRTA_MTESTATERESTORE0Offset()), ui32PDSMTEStateRestore[0]);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelUSEMemInfo->pvLinAddr + GetuKernelProgramRTA_MTESTATERESTORE1Offset()), ui32PDSMTEStateRestore[1]);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelUSEMemInfo, GetuKernelProgramRTA_MTESTATERESTORE0Offset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelUSEMemInfo, GetuKernelProgramRTA_MTESTATERESTORE1Offset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
#else
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelTAUSEMemInfo->pvLinAddr + GetuKernelProgramTA_RTA_MTESTATERESTORE0Offset()), ui32PDSMTEStateRestore[0]);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelTAUSEMemInfo->pvLinAddr + GetuKernelProgramTA_RTA_MTESTATERESTORE1Offset()), ui32PDSMTEStateRestore[1]);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelTAUSEMemInfo, GetuKernelProgramTA_RTA_MTESTATERESTORE0Offset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelTAUSEMemInfo, GetuKernelProgramTA_RTA_MTESTATERESTORE1Offset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
#endif

	return PVRSRV_OK;
}

#if defined(SGX_FEATURE_MTE_STATE_FLUSH) && defined(SGX_FEATURE_COMPLEX_GEOMETRY_REV_2)
/*!
*******************************************************************************

 @Function	LoadComplexGeometryStateRestoreProgram

 @Description

 Loads the Complex Geometry state restore program

 @Input none

 @Return none

******************************************************************************/
static PVRSRV_ERROR LoadComplexGeometryStateRestoreProgram(PVRSRV_DEV_DATA *psDevData,
															SGX_HEAP_INFO  *psHeapInfo,
															SGX_CLIENT_INIT_INFO *psDevInfo)
{
	PVRSRV_CLIENT_MEM_INFO	*psUSEMemInfo;
	PVRSRV_CLIENT_MEM_INFO	*psPDSMemInfo;
	IMG_UINT32	ui32USEProgramSize;
	IMG_UINT32	ui32USENumTemps;
	IMG_UINT32	aui32USETaskCtrl[3];
	IMG_UINT32	ui32AttributeSize, ui32PDSCmplxStateRestore[2];

	/*********************************/
	/* Complex Geometry State Update PDS/USE Task */
	/*********************************/
	ui32USEProgramSize = sizeof(pbUSEProgramCmplxStateRestore);

	/* Allocate memory for the USE program */
	PDUMPCOMMENT(psDevData->psConnection, "Allocate Complex geometry state restore USE Program", IMG_TRUE);
	if (PVRSRVAllocDeviceMem(psDevData,
								 psHeapInfo->psKernelCodeHeap->hDevMemHeap,
								 PVRSRV_MEM_READ | PVRSRV_MEM_NO_SYNCOBJ,
								 ui32USEProgramSize,
								 EURASIA_PDS_DOUTU_PHASE_START_ALIGN,
								 &psDevInfo->psUSECmplxTAStateMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"LoadComplexGeometryStateRestoreProgram : Failed to allocate USE program"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	psUSEMemInfo = psDevInfo->psUSECmplxTAStateMemInfo;

	/* Copy the USE code to the allocated memory */
	PVRSRVMemCopy((IMG_UINT8 *)psUSEMemInfo->pvLinAddr,
					pbUSEProgramCmplxStateRestore,
					ui32USEProgramSize);

	/* Pdump the USE code */
	PDUMPCOMMENT(psDevData->psConnection, "USE code for Complex Geometry state restore", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSEMemInfo,
				0, ui32USEProgramSize, PDUMP_FLAGS_CONTINUOUS);

	/* Set the temp register count */
	ui32USENumTemps = MAX(SGX_USE_MINTEMPREGS, 1);
	ui32USENumTemps = ALIGNSIZE(ui32USENumTemps, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT);

	/* Setup the USE task control words */
	#if defined(EURASIA_PDS_DOUTU0_TRC_SHIFT)
	aui32USETaskCtrl[0]	= ((ALIGNFIELD(ui32USENumTemps, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT)
							<< EURASIA_PDS_DOUTU0_TRC_SHIFT) & ~EURASIA_PDS_DOUTU0_TRC_CLRMSK);
	aui32USETaskCtrl[1]	= EURASIA_PDS_DOUTU1_MODE_PERINSTANCE;
	aui32USETaskCtrl[2]	= 0;
	#else
	aui32USETaskCtrl[0]	= EURASIA_PDS_DOUTU0_PDSDMADEPENDENCY;
	aui32USETaskCtrl[1]	= EURASIA_PDS_DOUTU1_MODE_PERINSTANCE |
						  ((ALIGNFIELD(ui32USENumTemps, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT)
						  	<< EURASIA_PDS_DOUTU1_TRC_SHIFT) & ~EURASIA_PDS_DOUTU1_TRC_CLRMSK);
	aui32USETaskCtrl[2]	= ((ALIGNFIELD(ui32USENumTemps, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT)
							>> EURASIA_PDS_DOUTU2_TRC_INTERNALSHIFT) << EURASIA_PDS_DOUTU2_TRC_SHIFT);
	#endif

	/* Setup the USE code address */
	SetUSEExecutionAddress(&aui32USETaskCtrl[0], 0, psUSEMemInfo->sDevVAddr,
							psHeapInfo->psKernelCodeHeap->sDevVAddrBase, SGX_KERNEL_USE_CODE_BASE_INDEX);

	/* Allocate memory for the PDS program */
	PDUMPCOMMENT(psDevData->psConnection, "Allocate Complex geometry state restore PDS Program", IMG_TRUE);
	if (PVRSRVAllocDeviceMem(psDevData,
								 psHeapInfo->psKernelCodeHeap->hDevMemHeap,
								 PVRSRV_MEM_READ | PVRSRV_MEM_NO_SYNCOBJ,
								 sizeof(g_pui32PDSTASTATERESTORE),
								 (1 << EUR_CR_EVENT_PIXEL_PDS_EXEC_ADDR_ALIGNSHIFT),
								 &psDevInfo->psPDSCmplxTAStateMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"LoadComplexGeometryStateRestoreProgram : Failed to allocate PDS program"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	psPDSMemInfo = psDevInfo->psPDSCmplxTAStateMemInfo;

	/* Copy the PDS program */
	PVRSRVMemCopy((IMG_UINT8 *)psPDSMemInfo->pvLinAddr,
					(IMG_PUINT8)g_pui32PDSTASTATERESTORE,
					sizeof(g_pui32PDSTASTATERESTORE));

	/* Fill in the contants */
	PDSTASTATERESTORESetINPUT_TA3DCTRL_ADDR((IMG_PUINT32)psPDSMemInfo->pvLinAddr,
											psDevInfo->psTA3DCtlMemInfo->sDevVAddr.uiAddr);
	PDSTASTATERESTORESetINPUT_TA3DCTRL_PA_DEST((IMG_PUINT32)psPDSMemInfo->pvLinAddr,
												(0 << EURASIA_PDS_DOUTA1_AO_SHIFT));
	PDSTASTATERESTORESetINPUT_DOUTU0((IMG_PUINT32)psPDSMemInfo->pvLinAddr, aui32USETaskCtrl[0]);
	PDSTASTATERESTORESetINPUT_DOUTU1((IMG_PUINT32)psPDSMemInfo->pvLinAddr, aui32USETaskCtrl[1]);
	PDSTASTATERESTORESetINPUT_DOUTU2((IMG_PUINT32)psPDSMemInfo->pvLinAddr, aui32USETaskCtrl[2]);

	/* Pdump the PDS program */
	PDUMPCOMMENT(psDevData->psConnection, "PDS program for Complex Geometry state restore", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psPDSMemInfo, 0, sizeof(g_pui32PDSTASTATERESTORE), PDUMP_FLAGS_CONTINUOUS);

	ui32AttributeSize = EURASIA_TACTL_CMPLX_ALL_SIZEINDWORDS;
	#if defined(SGX_FEATURE_UNIFIED_TEMPS_AND_PAS)
	ui32AttributeSize += ui32USENumTemps;
	#endif
	ui32AttributeSize = ALIGNSIZE(ui32AttributeSize * sizeof(IMG_UINT32), EURASIA_TAPDSSTATE_USEATTRIBUTESIZE_ALIGNSHIFT);
	/* convert into 128 Words (4 dwords) */
	ui32AttributeSize = ui32AttributeSize >> EURASIA_TAPDSSTATE_USEATTRIBUTESIZE_ALIGNSHIFT; 

#if defined(SGX_FEATURE_EDM_VERTEX_PDSADDR_FULL_RANGE)
	ui32PDSCmplxStateRestore[0] = (((psDevInfo->psPDSCmplxTAStateMemInfo->sDevVAddr.uiAddr >> EURASIA_TAPDSSTATE_BASEADDR_ALIGNSHIFT)
									<< EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_BASEADDR_SHIFT) & EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_BASEADDR_MASK) |
									EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_COMPLEX_MASK;
#else
	ui32PDSCmplxStateRestore[0] = ((((psDevInfo->psPDSCmplxTAStateMemInfo->sDevVAddr.uiAddr - psDevInfo->sDevVAddrPDSExecBase.uiAddr) >> EURASIA_TAPDSSTATE_BASEADDR_ALIGNSHIFT)
									<< EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_BASEADDR_SHIFT) & EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_BASEADDR_MASK) |
									EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_COMPLEX_MASK;

#endif
	ui32PDSCmplxStateRestore[1] = (((PDS_TASTATERESTORE_DATA_SEGMENT_SIZE >> EURASIA_TAPDSSTATE_DATASIZE_ALIGNSHIFT)
											<< EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_DATASIZE_SHIFT)
											& EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_DATASIZE_MASK) |
											EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_SD_MASK |
											EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_MTE_EMIT_MASK |
											(1UL << EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_USEPIPE_SHIFT) |
											(2UL << EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_PARTITIONS_SHIFT) |
											(ui32AttributeSize << EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_USEATTRIBUTESIZE_SHIFT);
	
	PDUMPCOMMENT(psDevData->psConnection, "Patch Ctx Switch Complex Geometry State Restore words", IMG_TRUE);
#if defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelUSEMemInfo->pvLinAddr + GetuKernelProgramRTA_CMPLXSTATERESTORE0Offset()), ui32PDSCmplxStateRestore[0]);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelUSEMemInfo->pvLinAddr + GetuKernelProgramRTA_CMPLXSTATERESTORE1Offset()), ui32PDSCmplxStateRestore[1]);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelUSEMemInfo, GetuKernelProgramRTA_CMPLXSTATERESTORE0Offset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelUSEMemInfo, GetuKernelProgramRTA_CMPLXSTATERESTORE1Offset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
#else
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelTAUSEMemInfo->pvLinAddr + GetuKernelProgramTA_RTA_CMPLXSTATERESTORE0Offset()), ui32PDSCmplxStateRestore[0]);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelTAUSEMemInfo->pvLinAddr + GetuKernelProgramTTA_RTA_CMPLXSTATERESTORE1Offset()), ui32PDSCmplxStateRestore[1]);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelTAUSEMemInfo, GetuKernelProgramTA_RTA_CMPLXSTATERESTORE0Offset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelTAUSEMemInfo, GetuKernelProgramTA_RTA_CMPLXSTATERESTORE1Offset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
#endif

	return PVRSRV_OK;
}
#endif

#if defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
/*!
*******************************************************************************

 @Function	LoadSARestoreProgram

 @Description

 Loads the SA restore program

 @Input none

 @Return none

******************************************************************************/
static PVRSRV_ERROR LoadSARestoreProgram(PVRSRV_DEV_DATA *psDevData,
												SGX_HEAP_INFO *psHeapInfo,
												SGX_CLIENT_INIT_INFO *psDevInfo,
												PVRSRV_CLIENT_MEM_INFO *psTA3DCtlMemInfo)
{
	PVRSRV_CLIENT_MEM_INFO	*psUSEMemInfo;
	PVRSRV_CLIENT_MEM_INFO	*psPDSMemInfo;
	IMG_UINT32	ui32USEProgramSize;
	IMG_UINT32	ui32USENumTemps;
	IMG_UINT32	aui32USETaskCtrl[3];
	IMG_UINT32	ui32PDSSARestore[2];
	IMG_UINT32	ui32AttributeSize;
	
	/*********************************/
	/* SA update Update PDS/USE Task */
	/*********************************/
	#if 0 // !SGX_SUPPORT_MASTER_ONLY_SWITCHING
	PVR_UNREFERENCED_PARAMETER(psTA3DCtlMemInfo);
	#endif
	ui32USEProgramSize = sizeof(pbUSEProgramTASARestore);

	PDUMPCOMMENT(psDevData->psConnection, "Allocate VDM Secondary Attribute USE Program", IMG_TRUE);

	/* Allocate memory for the USE program */
	if (PVRSRVAllocDeviceMem(psDevData,
								 psHeapInfo->psKernelCodeHeap->hDevMemHeap,
								 PVRSRV_MEM_READ | PVRSRV_MEM_NO_SYNCOBJ,
								 ui32USEProgramSize,
								 EURASIA_PDS_DOUTU_PHASE_START_ALIGN,
								 &psDevInfo->psUSESARestoreMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"LoadSARestoreProgram : Failed to allocate USE program"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	psUSEMemInfo = psDevInfo->psUSESARestoreMemInfo;

	/* Copy the USE code to the allocated memory */
	PVRSRVMemCopy((IMG_UINT8 *) psUSEMemInfo->pvLinAddr,
					pbUSEProgramTASARestore,
					ui32USEProgramSize);

	#if 0 // !SGX_SUPPORT_MASTER_ONLY_SWITCHING
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSEMemInfo->pvLinAddr + TASAR_SABUFFERBASEADDR_OFFSET),
				psDevInfo->psTA3DCtlMemInfo->sDevVAddr.uiAddr + offsetof(SGXMK_TA3D_CTL, sVDMSABufferDevAddr));
	#endif
	
	/* Pdump the USE code */
	PDUMPCOMMENT(psDevData->psConnection, "USE code for Secondary Atttibute restore program", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSEMemInfo,
				0, ui32USEProgramSize, PDUMP_FLAGS_CONTINUOUS);

	/* Set the temp register count */
	ui32USENumTemps = MAX(SGX_USE_MINTEMPREGS, 19);
	ui32USENumTemps = ALIGNSIZE(ui32USENumTemps, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT);

	/* Setup the USE task control words */
	#if defined(EURASIA_PDS_DOUTU0_TRC_SHIFT)
	aui32USETaskCtrl[0]	= ((ALIGNFIELD(ui32USENumTemps, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT)
							<< EURASIA_PDS_DOUTU0_TRC_SHIFT) & ~EURASIA_PDS_DOUTU0_TRC_CLRMSK);
	aui32USETaskCtrl[1]	= EURASIA_PDS_DOUTU1_MODE_PERINSTANCE | EURASIA_PDS_DOUTU1_SDSOFT;
	aui32USETaskCtrl[2]	= 0;
	#else
	aui32USETaskCtrl[0]	= EURASIA_PDS_DOUTU0_PDSDMADEPENDENCY;
	aui32USETaskCtrl[1]	= EURASIA_PDS_DOUTU1_MODE_PERINSTANCE |
						  ((ALIGNFIELD(ui32USENumTemps, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT)
						  	<< EURASIA_PDS_DOUTU1_TRC_SHIFT) & ~EURASIA_PDS_DOUTU1_TRC_CLRMSK);
	aui32USETaskCtrl[2]	= ((ALIGNFIELD(ui32USENumTemps, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT)
							>> EURASIA_PDS_DOUTU2_TRC_INTERNALSHIFT) << EURASIA_PDS_DOUTU2_TRC_SHIFT) |
							EURASIA_PDS_DOUTU2_SDSOFT;
	#endif

	/* Setup the USE code address */
	SetUSEExecutionAddress(&aui32USETaskCtrl[0], 0, psUSEMemInfo->sDevVAddr,
							psHeapInfo->psKernelCodeHeap->sDevVAddrBase, SGX_KERNEL_USE_CODE_BASE_INDEX);

	PDUMPCOMMENT(psDevData->psConnection, "Allocate VDM Secondary Attribute PDS Program", IMG_TRUE);
	/* Allocate memory for the PDS program */
	if (PVRSRVAllocDeviceMem(psDevData,
								 psHeapInfo->psKernelCodeHeap->hDevMemHeap,
								 PVRSRV_MEM_READ |
								 #if 1 // SGX_SUPPORT_MASTER_ONLY_SWITCHING
								 PVRSRV_MEM_WRITE | PVRSRV_MEM_EDM_PROTECT |
								 #endif
								 PVRSRV_MEM_NO_SYNCOBJ,
								 sizeof(g_pui32PDSTASARESTORE),
								 (1 << EUR_CR_EVENT_PIXEL_PDS_EXEC_ADDR_ALIGNSHIFT),
								 &psDevInfo->psPDSSARestoreMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"LoadSARestoreProgram : Failed to allocate PDS program"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	psPDSMemInfo = psDevInfo->psPDSSARestoreMemInfo;

	/* Copy the PDS program */
	PVRSRVMemCopy((IMG_UINT8 *)psPDSMemInfo->pvLinAddr,
					(IMG_PUINT8)g_pui32PDSTASARESTORE,
					sizeof(g_pui32PDSTASARESTORE));

	/* Fill in the contants */
	PDSTASARESTORESetINPUT_DOUTU0((IMG_PUINT32)psPDSMemInfo->pvLinAddr, aui32USETaskCtrl[0]);
	PDSTASARESTORESetINPUT_DOUTU1((IMG_PUINT32)psPDSMemInfo->pvLinAddr, aui32USETaskCtrl[1]);
	PDSTASARESTORESetINPUT_DOUTU2((IMG_PUINT32)psPDSMemInfo->pvLinAddr, aui32USETaskCtrl[2]);

	#if 1 // SGX_SUPPORT_MASTER_ONLY_SWITCHING

	/* Store the location of the PDS DOUTD data words */
	{
		IMG_UINT32	ui32DOUTD0_ByteOffset[] = PDS_TASARESTORE_INPUT_DOUTD0_LOCATIONS;
		IMG_UINT32	ui32DOUTD1_ByteOffset[] = PDS_TASARESTORE_INPUT_DOUTD1_LOCATIONS;
		IMG_UINT32	ui32DOUTD1_REMBLOCKS_ByteOffset[] = PDS_TASARESTORE_INPUT_DOUTD1_REMBLOCKS_LOCATIONS;
		IMG_UINT32	ui32DMABLOCKS_ByteOffset[] = PDS_TASARESTORE_INPUT_DMABLOCKS_LOCATIONS;
		
		SGXMK_TA3D_CTL *psTA3DCtl = psTA3DCtlMemInfo->pvLinAddr;
		psTA3DCtl->sDevAddrSAStateRestoreDMA_InputDOUTD0.uiAddr = psPDSMemInfo->sDevVAddr.uiAddr
			+ ui32DOUTD0_ByteOffset[0];
		psTA3DCtl->sDevAddrSAStateRestoreDMA_InputDOUTD1.uiAddr = psPDSMemInfo->sDevVAddr.uiAddr
			+ ui32DOUTD1_ByteOffset[0];
		psTA3DCtl->sDevAddrSAStateRestoreDMA_InputDOUTD1_REMBLOCKS.uiAddr = psPDSMemInfo->sDevVAddr.uiAddr
			+ ui32DOUTD1_REMBLOCKS_ByteOffset[0];
		psTA3DCtl->sDevAddrSAStateRestoreDMA_InputDMABLOCKS.uiAddr = psPDSMemInfo->sDevVAddr.uiAddr
			+ ui32DMABLOCKS_ByteOffset[0];
			
		PDUMPMEM(psDevData->psConnection, IMG_NULL, psTA3DCtlMemInfo, offsetof(SGXMK_TA3D_CTL, sDevAddrSAStateRestoreDMA_InputDOUTD0), sizeof(IMG_UINT32), PDUMP_FLAGS_CONTINUOUS);
		PDUMPMEM(psDevData->psConnection, IMG_NULL, psTA3DCtlMemInfo, offsetof(SGXMK_TA3D_CTL, sDevAddrSAStateRestoreDMA_InputDOUTD1), sizeof(IMG_UINT32), PDUMP_FLAGS_CONTINUOUS);
		PDUMPMEM(psDevData->psConnection, IMG_NULL, psTA3DCtlMemInfo, offsetof(SGXMK_TA3D_CTL, sDevAddrSAStateRestoreDMA_InputDOUTD1_REMBLOCKS), sizeof(IMG_UINT32), PDUMP_FLAGS_CONTINUOUS);
		PDUMPMEM(psDevData->psConnection, IMG_NULL, psTA3DCtlMemInfo, offsetof(SGXMK_TA3D_CTL, sDevAddrSAStateRestoreDMA_InputDMABLOCKS), sizeof(IMG_UINT32), PDUMP_FLAGS_CONTINUOUS);
	}
	#endif

	/* Pdump the PDS program */
	PDUMPCOMMENT(psDevData->psConnection, "PDS program for Secondary Attribute restore", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psPDSMemInfo, 0, sizeof(g_pui32PDSTASARESTORE), PDUMP_FLAGS_CONTINUOUS);

	#if defined(SGX_FEATURE_UNIFIED_TEMPS_AND_PAS)
	/* The SA count will be calculated in the uKernel, but we must setup any temps */
	ui32AttributeSize = ui32USENumTemps;
	ui32AttributeSize = ALIGNSIZE(ui32AttributeSize * sizeof(IMG_UINT32), EURASIA_TAPDSSTATE_USEATTRIBUTESIZE_ALIGNSHIFT);
	ui32AttributeSize = ui32AttributeSize >> EURASIA_TAPDSSTATE_USEATTRIBUTESIZE_ALIGNSHIFT;
	#else
	ui32AttributeSize = 0;
	#endif
	
	/* The number of Secondary Attributes to allocate will be calculated by the microkernel at runtime */
	ui32PDSSARestore[0] = (((psDevInfo->psPDSSARestoreMemInfo->sDevVAddr.uiAddr >> EURASIA_TAPDSSTATE_BASEADDR_ALIGNSHIFT)
									<< EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_BASEADDR_SHIFT) & EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_BASEADDR_MASK);
	ui32PDSSARestore[1] = (((PDS_TASARESTORE_DATA_SEGMENT_SIZE >> EURASIA_TAPDSSTATE_DATASIZE_ALIGNSHIFT)
											<< EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_DATASIZE_SHIFT)
											& EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_DATASIZE_MASK) |
	#if defined(EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_SEC_EXEC)
											EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_SEC_EXEC_MASK |
	#endif
											EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_SD_MASK |
											EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_USEPIPE_MASK |
											EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_SECONDARY_MASK |
											(ui32AttributeSize << EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_USEATTRIBUTESIZE_SHIFT);
											
	PDUMPCOMMENT(psDevData->psConnection, "Patch Ctx Switch Secondary Attribute Restore words", IMG_TRUE);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelUSEMemInfo->pvLinAddr + GetuKernelProgramRTA_SARESTORE0Offset()), ui32PDSSARestore[0]);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelUSEMemInfo->pvLinAddr + GetuKernelProgramRTA_SARESTORE1Offset()), ui32PDSSARestore[1]);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelUSEMemInfo, GetuKernelProgramRTA_SARESTORE0Offset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelUSEMemInfo, GetuKernelProgramRTA_SARESTORE1Offset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	
	return PVRSRV_OK;
}
#endif
#endif


/*!
*******************************************************************************

 @Function	LoaduKernelPDSInitPrograms

 @Description

 Loads the uKernel PDS init programs

 @Input psDevInfo

 @Return PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR LoaduKernelPDSInitPrograms(SGX_CLIENT_INIT_INFO		*psDevInfo,
											   PVRSRV_DEV_DATA			*psDevData,
#if defined (SUPPORT_SID_INTERFACE)
											   IMG_SID					hDevMemHeap,
#else
											   IMG_HANDLE				hDevMemHeap,
#endif
											   SGX_HEAP_INFO			*psHeapInfo,
											   PVRSRV_CLIENT_MEM_INFO	*psUSSEMemInfo)
{
	IMG_UINT32				aui32USETaskCtrl[3];
	IMG_UINT32				aui32SecDMAControl[2];
	IMG_DEV_VIRTADDR		sUSSEAddr;
	IMG_UINT32				ui32USETaskCtrlAddrInitPrim1 = 0;
	IMG_UINT32				ui32USETaskCtrlAddrInitSec = 0;
	IMG_UINT32				ui32USETaskCtrlAddrInitPrim2 = 0;
	IMG_UINT32				ui32NumTemps;
	IMG_PVOID				pvPDSProgLinAddr;

	/*
		Allocate memory for PDS primary init program 1
	*/
	PDUMPCOMMENT(psDevData->psConnection, "Allocate PDS primary program 1 for ukernel initialisation", IMG_TRUE);
	if (PVRSRVAllocDeviceMem(psDevData,
							 hDevMemHeap,
							 PVRSRV_MEM_READ | PVRSRV_MEM_NO_SYNCOBJ,
							 sizeof(g_pui32PDSUKERNEL_INIT_PRIM1),
							 (1 << EUR_CR_EVENT_OTHER_PDS_EXEC_ADDR_ALIGNSHIFT),
							 &psDevInfo->psMicrokernelPDSInitPrimary1MemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"LoaduKernelPDSInitPrograms : Failed to allocate PDS primary program 1 for ukernel initialisation"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	PVR_DPF((PVR_DBG_MESSAGE,"LoaduKernelPDSInitPrograms : uKernel PDS primary init program 1 Device Addr: 0x%x Size: 0x%x",
			psDevInfo->psMicrokernelPDSInitPrimary1MemInfo->sDevVAddr.uiAddr,
			psDevInfo->psMicrokernelPDSInitPrimary1MemInfo->uAllocSize));

	pvPDSProgLinAddr = psDevInfo->psMicrokernelPDSInitPrimary1MemInfo->pvLinAddr;

	/*
		Copy PDS primary init program 1
	*/
	PVRSRVMemCopy(pvPDSProgLinAddr, g_pui32PDSUKERNEL_INIT_PRIM1, sizeof(g_pui32PDSUKERNEL_INIT_PRIM1));

	/*
		Setup the USE code addresses for primary init program 1
	*/
	sUSSEAddr.uiAddr = psUSSEMemInfo->sDevVAddr.uiAddr + GetuKernelProgramINITUKERNELPRIMARY1Offset();
	SetUSEExecutionAddress(&ui32USETaskCtrlAddrInitPrim1, 0, sUSSEAddr,
						   psHeapInfo->psKernelCodeHeap->sDevVAddrBase, SGX_KERNEL_USE_CODE_BASE_INDEX);

	/*
		Setup the USE task control words and USE code address
	*/
	ui32NumTemps = ALIGNSIZE(SGX_UKERNEL_NUM_TEMPS, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT);
	
	#if defined(EURASIA_PDS_DOUTU0_TRC_SHIFT)
	aui32USETaskCtrl[0]	= (ALIGNFIELD(ui32NumTemps, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT) << EURASIA_PDS_DOUTU0_TRC_SHIFT) & ~EURASIA_PDS_DOUTU0_TRC_CLRMSK;
	aui32USETaskCtrl[1]	= EURASIA_PDS_DOUTU1_MODE_PERINSTANCE;
	aui32USETaskCtrl[2]	= 0;
	#else
	aui32USETaskCtrl[0]	= EURASIA_PDS_DOUTU0_PDSDMADEPENDENCY;
	aui32USETaskCtrl[1]	= EURASIA_PDS_DOUTU1_MODE_PERINSTANCE |
						  (ui32NumTemps << EURASIA_PDS_DOUTU1_TRC_SHIFT);
	aui32USETaskCtrl[2] = ((ui32NumTemps >> EURASIA_PDS_DOUTU2_TRC_INTERNALSHIFT) << EURASIA_PDS_DOUTU2_TRC_SHIFT)  & (~EURASIA_PDS_DOUTU2_TRC_CLRMSK);
	#endif
	aui32USETaskCtrl[0] |= ui32USETaskCtrlAddrInitPrim1;

	/*
		Fill in the contants
	*/
	PDSUKERNEL_INIT_PRIM1SetINPUT_DOUTU0(pvPDSProgLinAddr, aui32USETaskCtrl[0]);
	PDSUKERNEL_INIT_PRIM1SetINPUT_DOUTU1(pvPDSProgLinAddr, aui32USETaskCtrl[1]);
	PDSUKERNEL_INIT_PRIM1SetINPUT_DOUTU2(pvPDSProgLinAddr, aui32USETaskCtrl[2]);

	#if defined(PDUMP)
	/*
		Pdump PDS primary init program 1
	*/
	PDUMPCOMMENT(psDevData->psConnection, "PDS primary program 1 for ukernel initialisation", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelPDSInitPrimary1MemInfo,
			 0, (IMG_UINT32)psDevInfo->psMicrokernelPDSInitPrimary1MemInfo->uAllocSize, PDUMP_FLAGS_CONTINUOUS);
	#endif /* PDUMP */


	/*
		Allocate memory for the PDS secondary init program
	*/
	PDUMPCOMMENT(psDevData->psConnection, "Allocate PDS secondary program for ukernel initialisation", IMG_TRUE);
	if (PVRSRVAllocDeviceMem(psDevData,
							 hDevMemHeap,
							 PVRSRV_MEM_READ | PVRSRV_MEM_NO_SYNCOBJ,
							 sizeof(g_pui32PDSUKERNEL_INIT_SEC),
							 (1 << EUR_CR_EVENT_OTHER_PDS_EXEC_ADDR_ALIGNSHIFT),
							 &psDevInfo->psMicrokernelPDSInitSecondaryMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"LoaduKernelPDSInitPrograms : Failed to allocate PDS secondary program for ukernel initialisation"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	PVR_DPF((PVR_DBG_MESSAGE,"LoaduKernelPDSInitPrograms : uKernel PDS secondary init program Device Addr: 0x%x Size: 0x%x",
			psDevInfo->psMicrokernelPDSInitSecondaryMemInfo->sDevVAddr.uiAddr,
			psDevInfo->psMicrokernelPDSInitSecondaryMemInfo->uAllocSize));

	pvPDSProgLinAddr = psDevInfo->psMicrokernelPDSInitSecondaryMemInfo->pvLinAddr;
	
	/*
		Copy the PDS secondary init program
	*/
	PVRSRVMemCopy(pvPDSProgLinAddr, g_pui32PDSUKERNEL_INIT_SEC, sizeof(g_pui32PDSUKERNEL_INIT_SEC));

	/*
		Setup the USE code addresses for the secondary init program
	*/
	sUSSEAddr.uiAddr = psUSSEMemInfo->sDevVAddr.uiAddr + GetuKernelProgramINITUKERNELSECONDARYOffset();
	SetUSEExecutionAddress(&ui32USETaskCtrlAddrInitSec, 0, sUSSEAddr,
						   psHeapInfo->psKernelCodeHeap->sDevVAddrBase, SGX_KERNEL_USE_CODE_BASE_INDEX);

	/*
		Set up the DMA control words.
	*/
	{
		IMG_UINT32	ui32DMABurstSize, ui32DMATotalSize;

		ui32DMATotalSize = SGX_UKERNEL_NUM_SEC_ATTRIB;
		PVR_ASSERT(ui32DMATotalSize > 0);

		#if (EURASIA_PDS_DOUTD1_MAXBURST != EURASIA_PDS_DOUTD1_BSIZE_MAX)
		ui32DMABurstSize = SGX_UKERNEL_SA_BURST_SIZE;
		#else
		ui32DMABurstSize = ui32DMATotalSize;
		#endif
		
		PVR_ASSERT(ui32DMATotalSize <= EURASIA_PDS_DOUTD1_MAXBURST);
		PVR_ASSERT(ui32DMABurstSize <= EURASIA_PDS_DOUTD1_BSIZE_MAX);

		aui32SecDMAControl[0] = (psDevInfo->psTA3DCtlMemInfo->sDevVAddr.uiAddr << EURASIA_PDS_DOUTD0_SBASE_SHIFT);
		aui32SecDMAControl[1] = (EURASIA_PDS_DOUTD1_INSTR_NORMAL	<< EURASIA_PDS_DOUTD1_INSTR_SHIFT)		|
								 (0									<< EURASIA_PDS_DOUTD1_AO_SHIFT)			|
								 ((ui32DMABurstSize - 1)			<< EURASIA_PDS_DOUTD1_BSIZE_SHIFT);
		#if (EURASIA_PDS_DOUTD1_MAXBURST != EURASIA_PDS_DOUTD1_BSIZE_MAX)
		{
			IMG_UINT32	ui32DMABurstLines = ui32DMATotalSize / ui32DMABurstSize;
			PVR_ASSERT(ui32DMABurstLines <= EURASIA_PDS_DOUTD1_BLINES_MAX);

			aui32SecDMAControl[1] |= ((ui32DMABurstSize - 1)			<< EURASIA_PDS_DOUTD1_STRIDE_SHIFT)		|
									  ((ui32DMABurstLines - 1)			<< EURASIA_PDS_DOUTD1_BLINES_SHIFT);
		}
		#endif /* SGX543 */
	}
							 
	/*
		Setup the USE task control words and USE code address
	*/
	ui32NumTemps = ALIGNSIZE(0, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT);
	#if defined(EURASIA_PDS_DOUTU0_TRC_SHIFT)
	aui32USETaskCtrl[0]	= (ALIGNFIELD(ui32NumTemps, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT) << EURASIA_PDS_DOUTU0_TRC_SHIFT) & ~EURASIA_PDS_DOUTU0_TRC_CLRMSK;
	aui32USETaskCtrl[1]	= EURASIA_PDS_DOUTU1_MODE_PERINSTANCE;
	#else
	aui32USETaskCtrl[0]	= EURASIA_PDS_DOUTU0_PDSDMADEPENDENCY;
	aui32USETaskCtrl[1]	= EURASIA_PDS_DOUTU1_MODE_PERINSTANCE |
						  (ui32NumTemps << EURASIA_PDS_DOUTU1_TRC_SHIFT);
	#endif
	aui32USETaskCtrl[2]	= 0;
	aui32USETaskCtrl[0] |= ui32USETaskCtrlAddrInitSec;

	/* Fill in the contants */
	PDSUKERNEL_INIT_SECSetINPUT_DOUTD0(pvPDSProgLinAddr, aui32SecDMAControl[0]);
	PDSUKERNEL_INIT_SECSetINPUT_DOUTD1(pvPDSProgLinAddr, aui32SecDMAControl[1]);

	PDSUKERNEL_INIT_SECSetINPUT_DOUTU0(pvPDSProgLinAddr, aui32USETaskCtrl[0]);
	PDSUKERNEL_INIT_SECSetINPUT_DOUTU1(pvPDSProgLinAddr, aui32USETaskCtrl[1]);
	PDSUKERNEL_INIT_SECSetINPUT_DOUTU2(pvPDSProgLinAddr, aui32USETaskCtrl[2]);

	#if defined(PDUMP)
	/*
		Pdump the PDS secondary init program
	*/
	PDUMPCOMMENT(psDevData->psConnection, "PDS secondary program for ukernel initialisation", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelPDSInitSecondaryMemInfo,
			 0, (IMG_UINT32)psDevInfo->psMicrokernelPDSInitSecondaryMemInfo->uAllocSize, PDUMP_FLAGS_CONTINUOUS);
	#endif /* PDUMP */


	/*
		Allocate memory for PDS primary init program 2
	*/
	PDUMPCOMMENT(psDevData->psConnection, "Allocate PDS primary program 2 for ukernel initialisation", IMG_TRUE);
	if (PVRSRVAllocDeviceMem(psDevData,
							 hDevMemHeap,
							 PVRSRV_MEM_READ | PVRSRV_MEM_NO_SYNCOBJ,
							 sizeof(g_pui32PDSUKERNEL_INIT_PRIM2),
							 (1 << EUR_CR_EVENT_OTHER_PDS_EXEC_ADDR_ALIGNSHIFT),
							 &psDevInfo->psMicrokernelPDSInitPrimary2MemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"LoaduKernelPDSInitPrograms : Failed to allocate PDS primary program 2 for ukernel initialisation"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	PVR_DPF((PVR_DBG_MESSAGE,"LoaduKernelPDSInitPrograms : uKernel PDS primary init program 2 Device Addr: 0x%x Size: 0x%x",
			psDevInfo->psMicrokernelPDSInitPrimary2MemInfo->sDevVAddr.uiAddr,
			psDevInfo->psMicrokernelPDSInitPrimary2MemInfo->uAllocSize));

	pvPDSProgLinAddr = psDevInfo->psMicrokernelPDSInitPrimary2MemInfo->pvLinAddr;

	/*
		Copy PDS primary init program 2
	*/
	PVRSRVMemCopy(pvPDSProgLinAddr, g_pui32PDSUKERNEL_INIT_PRIM2, sizeof(g_pui32PDSUKERNEL_INIT_PRIM2));

	/*
		Setup the USE code addresses for primary init program 2
	*/
	sUSSEAddr.uiAddr = psUSSEMemInfo->sDevVAddr.uiAddr + GetuKernelProgramINITUKERNELPRIMARY2Offset();
	SetUSEExecutionAddress(&ui32USETaskCtrlAddrInitPrim2, 0, sUSSEAddr,
						   psHeapInfo->psKernelCodeHeap->sDevVAddrBase, SGX_KERNEL_USE_CODE_BASE_INDEX);

	/*
		Setup the USE task control words and USE code address
	*/
	ui32NumTemps = ALIGNSIZE(SGX_UKERNEL_NUM_TEMPS, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT);
	#if defined(EURASIA_PDS_DOUTU0_TRC_SHIFT)
	aui32USETaskCtrl[0]	= (ALIGNFIELD(ui32NumTemps, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT) << EURASIA_PDS_DOUTU0_TRC_SHIFT) & ~EURASIA_PDS_DOUTU0_TRC_CLRMSK;
	aui32USETaskCtrl[1]	= EURASIA_PDS_DOUTU1_MODE_PERINSTANCE;
	aui32USETaskCtrl[2]	= 0;
	#else
	aui32USETaskCtrl[0]	= EURASIA_PDS_DOUTU0_PDSDMADEPENDENCY;
	aui32USETaskCtrl[1]	= EURASIA_PDS_DOUTU1_MODE_PERINSTANCE |
						  ((ALIGNFIELD(ui32NumTemps, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT) << EURASIA_PDS_DOUTU1_TRC_SHIFT) & ~EURASIA_PDS_DOUTU1_TRC_CLRMSK);
	aui32USETaskCtrl[2] = ((ui32NumTemps >> EURASIA_PDS_DOUTU2_TRC_INTERNALSHIFT) << EURASIA_PDS_DOUTU2_TRC_SHIFT)  & (~EURASIA_PDS_DOUTU2_TRC_CLRMSK);
	
	#endif
	aui32USETaskCtrl[0] |= ui32USETaskCtrlAddrInitPrim2;

	/*
		Fill in the contants
	*/
	PDSUKERNEL_INIT_PRIM2SetINPUT_DOUTU0(pvPDSProgLinAddr, aui32USETaskCtrl[0]);
	PDSUKERNEL_INIT_PRIM2SetINPUT_DOUTU1(pvPDSProgLinAddr, aui32USETaskCtrl[1]);
	PDSUKERNEL_INIT_PRIM2SetINPUT_DOUTU2(pvPDSProgLinAddr, aui32USETaskCtrl[2]);

	#if defined(PDUMP)
	/*
		Pdump PDS primary init program 2
	*/
	PDUMPCOMMENT(psDevData->psConnection, "PDS primary program 2 for ukernel initialisation", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelPDSInitPrimary2MemInfo,
			 0, (IMG_UINT32)psDevInfo->psMicrokernelPDSInitPrimary2MemInfo->uAllocSize, PDUMP_FLAGS_CONTINUOUS);
	#endif /* PDUMP */


	return PVRSRV_OK;
}


/*!
*******************************************************************************

 @Function	LoaduKernelProgram

 @Description

 Loads a uKernel program

 @Input psDevInfo

 @Return PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR LoaduKernelProgram(PVRSRV_DEV_DATA			*psDevData,
#if defined (SUPPORT_SID_INTERFACE)
									   IMG_SID					hDevMemHeap,
#else
									   IMG_HANDLE				hDevMemHeap,
#endif
									   PVRSRV_CLIENT_MEM_INFO	**ppsUSEMemInfo,
									   const IMG_UINT8 				*pui8ProgramBuffer,
									   IMG_UINT32				ui32ProgramSize,
									   IMG_CHAR					*pszProgramName)
{
	PVRSRV_CLIENT_MEM_INFO	*psUSEMemInfo;

#if !defined(PDUMP) && !defined(DEBUG)
	PVR_UNREFERENCED_PARAMETER(pszProgramName);
#endif

	/*
		Allocate memory for the ukernel program. The USSE linker should have
		thrown an error if the maximum USSE program size is exceeded.
		FIXME: Alignment for monolithic ukernel is marked as
			EURASIA_PDS_DOUTU_PHASE_START_ALIGN, but really needs to be
			EURASIA_USE_CODE_PAGE_SIZE.
			Unfortunately, this would fail to allocate due to RA/heap constraints.
	*/
	PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "Allocate buffer for %s USE program", pszProgramName);
	if (PVRSRVAllocDeviceMem(psDevData,
								 hDevMemHeap,
								 PVRSRV_MEM_READ | PVRSRV_MEM_NO_SYNCOBJ,
								 ui32ProgramSize,
#if defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
								  EURASIA_PDS_DOUTU_PHASE_START_ALIGN,
#else
								  EURASIA_USE_CODE_PAGE_SIZE,
#endif
								 &psUSEMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupuKernel : Failed to allocate 0x%x bytes for %s USE program",
				ui32ProgramSize, pszProgramName));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	PVR_DPF((PVR_DBG_MESSAGE,"LoaduKernelProgram : %s program Device Addr: 0x%x Size: 0x%x",
			pszProgramName, psUSEMemInfo->sDevVAddr.uiAddr, ui32ProgramSize));
	PVR_ASSERT((psUSEMemInfo->sDevVAddr.uiAddr & (EURASIA_USE_CODE_PAGE_SIZE - 1)) == 0);

	/*
		Copy the USE code to the allocated memory
	*/
	PVRSRVMemCopy((IMG_UINT8 *) psUSEMemInfo->pvLinAddr,
			  		pui8ProgramBuffer, ui32ProgramSize);
			  		

#if defined(SGX_FEATURE_MK_SUPERVISOR)
	#if defined(SGX_FEATURE_MONOLITHIC_UKERNEL) 
	{
		IMG_UINT32 *pui32Tmp = (IMG_UINT32*)psUSEMemInfo->pvLinAddr;
		/* advance to instruction to patch */
		pui32Tmp += (GetuKernelProgramSPRV_PHASOffset() >> 2);
		
		/*
			patch the phas instruction return address to return 
			to the utils from the supervisor
		*/
		PVR_ASSERT((*pui32Tmp & ~EURASIA_USE0_OTHER2_PHAS_IMM_EXEADDR_CLRMSK) == 0);
		*pui32Tmp |= ~EURASIA_USE0_OTHER2_PHAS_IMM_EXEADDR_CLRMSK 
					& ((GetuKernelProgramSPRVRETURNOffset()
						/ EURASIA_USE_INSTRUCTION_SIZE)
					<< EURASIA_USE0_OTHER2_PHAS_IMM_EXEADDR_SHIFT);
	}
	#else
		#error ERROR: SGX_FEATURE_MK_SUPERVISOR only supported with SGX_FEATURE_MONOLITHIC_UKERNEL
	#endif
#endif

	/*
		Pdump the USE code
	*/
	PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "%s USE program", pszProgramName);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSEMemInfo, 0, ui32ProgramSize, PDUMP_FLAGS_CONTINUOUS);

	*ppsUSEMemInfo = psUSEMemInfo;

	return PVRSRV_OK;
}

#if defined(SGX_FEATURE_MP)
/*****************************************************************************
 FUNCTION	: LoadSlaveuKernel

 PURPOSE	: Load the PDS and USE programs required to run on core1-n on
 				an SGX-MP system.

 PARAMETERS	:

 RETURNS	: PVRSRV_ERROR
*****************************************************************************/
static PVRSRV_ERROR LoadSlaveuKernel(PVRSRV_DEV_DATA *psDevData,
										SGX_HEAP_INFO *psHeapInfo,
										SGX_CLIENT_INIT_INFO *psDevInfo,
										SGX_SRVINIT_INIT_INFO *psSrvInfo)
{
	PVRSRV_CLIENT_MEM_INFO	*psUSEMemInfo;
	PVRSRV_CLIENT_MEM_INFO	*psPDSMemInfo;
	IMG_UINT32				aui32USETaskControl[3];
	IMG_UINT32				*pui32Tmp;
	IMG_UINT32				ui32ReturnAddr;
		
	/* Allocate memory for the USE program */
	PDUMPCOMMENT(psDevData->psConnection, "Allocate Slave uKernel USE Program", IMG_TRUE);
	if (PVRSRVAllocDeviceMem(psDevData,
							 psHeapInfo->psKernelCodeHeap->hDevMemHeap,
							 PVRSRV_MEM_READ | PVRSRV_MEM_NO_SYNCOBJ,
							 sizeof(pbSlaveuKernelProgram),
							 EURASIA_PDS_DOUTU_PHASE_START_ALIGN,
							 &psDevInfo->psMicrokernelSlaveUSEMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"LoadSlaveuKernel : Failed to allocate USE program"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	psUSEMemInfo = psDevInfo->psMicrokernelSlaveUSEMemInfo;

	/* Copy the USE code to the allocated memory */
	PVRSRVMemCopy((IMG_UINT8 *)psUSEMemInfo->pvLinAddr,
			  		pbSlaveuKernelProgram, sizeof(pbSlaveuKernelProgram));
			  		
	/* patch the SSPRV_Exit phase instruction */
	pui32Tmp = (IMG_UINT32*)psUSEMemInfo->pvLinAddr + (SLAVE_UKERNEL_SSPRV_EXIT_OFFSET >> 2);
	/*
		patch the phas instruction return address to return
	*/
	ui32ReturnAddr = psUSEMemInfo->sDevVAddr.uiAddr 
						+ SLAVE_UKERNEL_SSPRV_RETURN_OFFSET
	 					- psHeapInfo->psKernelCodeHeap->sDevVAddrBase.uiAddr;
	 			
	PVR_ASSERT((*pui32Tmp & ~EURASIA_USE0_OTHER2_PHAS_IMM_EXEADDR_CLRMSK) == 0);
	*pui32Tmp |= ~EURASIA_USE0_OTHER2_PHAS_IMM_EXEADDR_CLRMSK
				& ((ui32ReturnAddr / EURASIA_USE_INSTRUCTION_SIZE) << EURASIA_USE0_OTHER2_PHAS_IMM_EXEADDR_SHIFT);

	PDUMPCOMMENT(psDevData->psConnection, "Slave uKernel USE Program", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSEMemInfo,
				0, sizeof(pbSlaveuKernelProgram), PDUMP_FLAGS_CONTINUOUS);

	/* Setup the USE task control words */
	#if defined(EURASIA_PDS_DOUTU0_TRC_SHIFT)
	aui32USETaskControl[0]	= (ALIGNFIELD(SGX_UKERNEL_NUM_TEMPS, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT) << EURASIA_PDS_DOUTU0_TRC_SHIFT) & ~EURASIA_PDS_DOUTU0_TRC_CLRMSK;
	aui32USETaskControl[1]	= EURASIA_PDS_DOUTU1_MODE_PERINSTANCE;
	aui32USETaskControl[2]	= 0;
	#else
	aui32USETaskControl[0]	= EURASIA_PDS_DOUTU0_PDSDMADEPENDENCY;
	aui32USETaskControl[1]	= EURASIA_PDS_DOUTU1_MODE_PERINSTANCE |
						  (SGX_UKERNEL_NUM_TEMPS << EURASIA_PDS_DOUTU1_TRC_SHIFT);
	aui32USETaskControl[2] = ((SGX_UKERNEL_NUM_TEMPS >> EURASIA_PDS_DOUTU2_TRC_INTERNALSHIFT) << EURASIA_PDS_DOUTU2_TRC_SHIFT)  & (~EURASIA_PDS_DOUTU2_TRC_CLRMSK);
	#endif
	
	SetUSEExecutionAddress(&aui32USETaskControl[0], 0, psUSEMemInfo->sDevVAddr,
							psHeapInfo->psKernelCodeHeap->sDevVAddrBase, SGX_KERNEL_USE_CODE_BASE_INDEX);

	/* Allocate memory for the PDS program */
	PDUMPCOMMENT(psDevData->psConnection, "Allocate Slave uKernel Event PDS Program", IMG_TRUE);
	if (PVRSRVAllocDeviceMem(psDevData,
							 psHeapInfo->psKernelCodeHeap->hDevMemHeap,
							 PVRSRV_MEM_READ | PVRSRV_MEM_NO_SYNCOBJ,
							 sizeof(g_pui32PDSSLAVE_UKERNEL_EVENTS),
							 (1 << EUR_CR_EVENT_PIXEL_PDS_EXEC_ADDR_ALIGNSHIFT),
							 &psDevInfo->psMicrokernelPDSEventSlaveMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"LoadSlaveuKernel : Failed to allocate PDS program"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	psPDSMemInfo = psDevInfo->psMicrokernelPDSEventSlaveMemInfo;

	/* Copy the PDS program */
	PVRSRVMemCopy((IMG_UINT8 *)psPDSMemInfo->pvLinAddr,
					(IMG_PUINT8)g_pui32PDSSLAVE_UKERNEL_EVENTS,
					sizeof(g_pui32PDSSLAVE_UKERNEL_EVENTS));

	/* Fill in the contants */
	PDSSLAVE_UKERNEL_EVENTSSetINPUT_DOUTU0((IMG_PUINT32)psPDSMemInfo->pvLinAddr, aui32USETaskControl[0]);
	PDSSLAVE_UKERNEL_EVENTSSetINPUT_DOUTU1((IMG_PUINT32)psPDSMemInfo->pvLinAddr, aui32USETaskControl[1]);
	PDSSLAVE_UKERNEL_EVENTSSetINPUT_DOUTU2((IMG_PUINT32)psPDSMemInfo->pvLinAddr, aui32USETaskControl[2]);
	
	PDSSLAVE_UKERNEL_EVENTSSetINPUT_IR0_PA_DEST((IMG_PUINT32)psPDSMemInfo->pvLinAddr, ((offsetof(PVRSRV_SGX_SLAVE_EDMPROG_PRIMATTR, ui32PDSIn0) / sizeof(IMG_UINT32)) << EURASIA_PDS_DOUTA1_AO_SHIFT));
	PDSSLAVE_UKERNEL_EVENTSSetINPUT_IR1_PA_DEST((IMG_PUINT32)psPDSMemInfo->pvLinAddr, ((offsetof(PVRSRV_SGX_SLAVE_EDMPROG_PRIMATTR, ui32PDSIn1) / sizeof(IMG_UINT32)) << EURASIA_PDS_DOUTA1_AO_SHIFT));
	PDSSLAVE_UKERNEL_EVENTSSetINPUT_TA3D_CTL_ADDR((IMG_PUINT32)psPDSMemInfo->pvLinAddr, psDevInfo->psTA3DCtlMemInfo->sDevVAddr.uiAddr);
	PDSSLAVE_UKERNEL_EVENTSSetINPUT_TA3D_CTL_PA_DEST((IMG_PUINT32)psPDSMemInfo->pvLinAddr, ((offsetof(PVRSRV_SGX_SLAVE_EDMPROG_PRIMATTR, sTA3DCtl) / sizeof(IMG_UINT32)) << EURASIA_PDS_DOUTA1_AO_SHIFT));

	PDUMPCOMMENT(psDevData->psConnection, "Slave uKernel PDS Program", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psPDSMemInfo,
				0, sizeof(g_pui32PDSSLAVE_UKERNEL_EVENTS), PDUMP_FLAGS_CONTINUOUS);

	/* Setup EDM PDS/USE registers */
	psSrvInfo->ui32SlaveEventOtherPDSExec	= (psPDSMemInfo->sDevVAddr.uiAddr >> EUR_CR_EVENT_OTHER_PDS_EXEC_ADDR_ALIGNSHIFT) << EUR_CR_EVENT_OTHER_PDS_EXEC_ADDR_SHIFT;
	psSrvInfo->ui32SlaveEventOtherPDSData	= (PDS_SLAVE_UKERNEL_EVENTS_DATA_SEGMENT_SIZE >> EUR_CR_EVENT_OTHER_PDS_DATA_SIZE_ALIGNSHIFT) << EUR_CR_EVENT_OTHER_PDS_DATA_SIZE_SHIFT;
	psSrvInfo->ui32SlaveEventOtherPDSInfo	= EUR_CR_EVENT_OTHER_PDS_INFO_SD_MASK |
												((((SGX_UKERNEL_SLAVE_NUM_PRIM_ATTRIB + SGX_UKERNEL_NUM_TEMPS) * sizeof(IMG_UINT32) + ((1 << EUR_CR_EVENT_OTHER_PDS_INFO_ATTRIBUTE_SIZE_ALIGNSHIFT) - 1)) 
												>> EUR_CR_EVENT_OTHER_PDS_INFO_ATTRIBUTE_SIZE_ALIGNSHIFT) << EUR_CR_EVENT_OTHER_PDS_INFO_ATTRIBUTE_SIZE_SHIFT);

	return PVRSRV_OK;
}
#endif

/*!
*******************************************************************************

 @Function	LoaduKernel

 @Description

 Loads the uKernel programs

 @Input psDevInfo

 @Return PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR LoaduKernel(PVRSRV_DEV_DATA *psDevData,
								SGX_HEAP_INFO *psHeapInfo,
								SGX_SRVINIT_INIT_INFO *psSrvInfo,
								SGX_CLIENT_INIT_INFO *psDevInfo)
{
	PVRSRV_ERROR eError;
	IMG_UINT32	aui32USETaskCtrl[3];
	IMG_UINT32	ui32USETaskCtrlAddrGeneric = 0;
	IMG_UINT32	ui32USETaskCtrlAddrSPM = 0;
	IMG_UINT32	ui32USETaskCtrlAddr3D = 0;
	IMG_UINT32	ui32USETaskCtrlAddrTA = 0;
#if defined(SGX_FEATURE_2D_HARDWARE)
	IMG_UINT32 ui32USETaskCtrlAddr2D = 0;
#endif
	IMG_UINT32	ui32PDSExecBase;
	IMG_UINT32	ui32USENumAttrs;
	IMG_DEV_VIRTADDR sUSSEAddr;
	PVRSRV_CLIENT_MEM_INFO	*psUSEMemInfo;
#if !defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
	PVRSRV_CLIENT_MEM_INFO	*psUSESPMMemInfo;
	PVRSRV_CLIENT_MEM_INFO	*psUSE3DMemInfo;
	PVRSRV_CLIENT_MEM_INFO	*psUSETAMemInfo;
#if defined(SGX_FEATURE_2D_HARDWARE)
	PVRSRV_CLIENT_MEM_INFO	*psUSE2DMemInfo;
#endif
#endif /* SGX_FEATURE_MONOLITHIC_UKERNEL */
	IMG_DEV_VIRTADDR		sLoopbackDevAddr;
	IMG_UINT32				ui32LoopbackConstSize;
	PVRSRV_CLIENT_MEM_INFO	*psLoopbackPDSMemInfo;
	PVRSRV_CLIENT_MEM_INFO	*psEventPDSMemInfo;
#if defined (SUPPORT_SID_INTERFACE)
	IMG_SID 				hDevKernelCodeHeap = psHeapInfo->psKernelCodeHeap->hDevMemHeap;
#else
	IMG_HANDLE 				hDevKernelCodeHeap = psHeapInfo->psKernelCodeHeap->hDevMemHeap;
#endif

	PVR_UNREFERENCED_PARAMETER(psSrvInfo);

	/*
		Allocate and load the ukernel USSE modules.
	*/

	eError = LoaduKernelProgram(psDevData, hDevKernelCodeHeap,
								&psDevInfo->psMicrokernelUSEMemInfo,
								GetuKernelProgramBuffer(), GetuKernelProgramSize(), "SGX ukernel");
	if (eError != PVRSRV_OK)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	psUSEMemInfo = psDevInfo->psMicrokernelUSEMemInfo;

#if !defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
	eError = LoaduKernelProgram(psDevData, hDevKernelCodeHeap,
								&psDevInfo->psMicrokernelTAUSEMemInfo,
								GetuKernelTAProgramBuffer(), GetuKernelTAProgramSize(), "SGX TA");
	if (eError != PVRSRV_OK)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	psUSETAMemInfo = psDevInfo->psMicrokernelTAUSEMemInfo;

	eError = LoaduKernelProgram(psDevData, hDevKernelCodeHeap,
								&psDevInfo->psMicrokernelSPMUSEMemInfo,
								GetuKernelSPMProgramBuffer(), GetuKernelSPMProgramSize(), "SGX SPM");
	if (eError != PVRSRV_OK)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	psUSESPMMemInfo = psDevInfo->psMicrokernelSPMUSEMemInfo;

	eError = LoaduKernelProgram(psDevData, hDevKernelCodeHeap,
								&psDevInfo->psMicrokernel3DUSEMemInfo,
								GetuKernel3DProgramBuffer(), GetuKernel3DProgramSize(), "SGX 3D");
	if (eError != PVRSRV_OK)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	psUSE3DMemInfo = psDevInfo->psMicrokernel3DUSEMemInfo;
#if defined(SGX_FEATURE_2D_HARDWARE)
	eError = LoaduKernelProgram(psDevData, hDevKernelCodeHeap,
								&psDevInfo->psMicrokernel2DUSEMemInfo,
								GetuKernel2DProgramBuffer(), GetuKernel2DProgramSize(), "SGX 2D");
	if (eError != PVRSRV_OK)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	psUSE2DMemInfo = psDevInfo->psMicrokernel2DUSEMemInfo;
#endif
#endif /* SGX_FEATURE_MONOLITHIC_UKERNEL */

#if defined(SGX_FEATURE_MP)
	eError = LoadSlaveuKernel(psDevData, psHeapInfo, psDevInfo, psSrvInfo);
	if (eError != PVRSRV_OK)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;	
	}
#endif

	/*
		Allocate and load the ukernel PDS initialisation modules.
	*/
	LoaduKernelPDSInitPrograms(psDevInfo, psDevData, hDevKernelCodeHeap,
							   psHeapInfo, psUSEMemInfo);

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH)
	/*
		Load the TA state terminate program which will be launched
		from the main EDM program via the register interface.
	*/
	if (LoadCtxSwitchStateTermProgram(psDevData, psHeapInfo, psDevInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupuKernel : Failed to load CtxSwitch Terminate programs"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	/*
		Load the TA state restore program which will be launched
		from the main EDM program via the register interface.
	*/
	if (LoadTAStateRestoreProgram(psDevData, psHeapInfo, psDevInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupuKernel : Failed to load MTE State Restore programs"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	
	#if defined(SGX_FEATURE_MTE_STATE_FLUSH) && defined(SGX_FEATURE_COMPLEX_GEOMETRY_REV_2)
	/* 
		Load the Complex Geometry state restore program which will be launched
		from the main EDM program via the register interface.
	*/
	if (LoadComplexGeometryStateRestoreProgram(psDevData, psHeapInfo, psDevInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupuKernel : Failed to load Complex Geomerty State Restore programs"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	#endif

	#if defined(SGX_FEATURE_USE_SA_COUNT_REGISTER)
	/* 
		Load the Secondary Attribute restore program which will be launched
		from the main EDM program via the register interface.
	*/
	if (LoadSARestoreProgram(psDevData, psHeapInfo, psDevInfo, psDevInfo->psTA3DCtlMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupuKernel : Failed to load Secondary Attribute Restore programs"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	#endif
#endif

	/*
		Setup the USE task control words and USE code address
	*/
	#if defined(EURASIA_PDS_DOUTU0_TRC_SHIFT)
	aui32USETaskCtrl[0]	= (ALIGNFIELD(SGX_UKERNEL_NUM_TEMPS, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT) << EURASIA_PDS_DOUTU0_TRC_SHIFT) & ~EURASIA_PDS_DOUTU0_TRC_CLRMSK;
	aui32USETaskCtrl[1]	= EURASIA_PDS_DOUTU1_MODE_PERINSTANCE;
	aui32USETaskCtrl[2]	= 0;
	#else
	aui32USETaskCtrl[0]	= EURASIA_PDS_DOUTU0_PDSDMADEPENDENCY;
	aui32USETaskCtrl[1]	= EURASIA_PDS_DOUTU1_MODE_PERINSTANCE |
						  (SGX_UKERNEL_NUM_TEMPS << EURASIA_PDS_DOUTU1_TRC_SHIFT);
	aui32USETaskCtrl[2] = ((SGX_UKERNEL_NUM_TEMPS >> EURASIA_PDS_DOUTU2_TRC_INTERNALSHIFT) << EURASIA_PDS_DOUTU2_TRC_SHIFT)  & (~EURASIA_PDS_DOUTU2_TRC_CLRMSK);
	#endif

	/*
		Allocate memory for the PDS event handler program
	*/
	PDUMPCOMMENT(psDevData->psConnection, "Allocate PDS program for EDM other events", IMG_TRUE);
	if (PVRSRVAllocDeviceMem(psDevData,
							 hDevKernelCodeHeap,
							 PVRSRV_MEM_READ | PVRSRV_MEM_NO_SYNCOBJ,
							 sizeof(g_pui32PDSUKERNEL_EVENTS),
							 (1 << EUR_CR_EVENT_OTHER_PDS_EXEC_ADDR_ALIGNSHIFT),
							 &psDevInfo->psMicrokernelPDSEventMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"LoaduKernelPrograms : Failed to allocate PDS program for EDM other events"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	psEventPDSMemInfo = psDevInfo->psMicrokernelPDSEventMemInfo;

	PVR_DPF((PVR_DBG_MESSAGE,"LoaduKernel : uKernel PDS Event handler program Device Addr: 0x%x Size: 0x%x",
			psEventPDSMemInfo->sDevVAddr.uiAddr, sizeof(g_pui32PDSUKERNEL_EVENTS)));

	/*
		Copy the PDS event handler program
	*/
	PVRSRVMemCopy((IMG_UINT8 *)psEventPDSMemInfo->pvLinAddr, (const IMG_UINT8 *)g_pui32PDSUKERNEL_EVENTS, sizeof(g_pui32PDSUKERNEL_EVENTS));

	/*
		Set up the USE code addresses for the event handler
	*/
	sUSSEAddr.uiAddr = psUSEMemInfo->sDevVAddr.uiAddr + GetuKernelProgramEVENTHANDLEROffset();
	SetUSEExecutionAddress(&ui32USETaskCtrlAddrGeneric, 0, sUSSEAddr,
							psHeapInfo->psKernelCodeHeap->sDevVAddrBase, SGX_KERNEL_USE_CODE_BASE_INDEX);

	/* Fill in the contants */
	PDSUKERNEL_EVENTSSetINPUT_IR0_PA_DEST ((IMG_PUINT32)psEventPDSMemInfo->pvLinAddr, ((offsetof(PVRSRV_SGX_EDMPROG_PRIMATTR, ui32PDSIn0) / sizeof(IMG_UINT32)) << EURASIA_PDS_DOUTA1_AO_SHIFT));
	PDSUKERNEL_EVENTSSetINPUT_IR1_PA_DEST ((IMG_PUINT32)psEventPDSMemInfo->pvLinAddr, ((offsetof(PVRSRV_SGX_EDMPROG_PRIMATTR, ui32PDSIn1) / sizeof(IMG_UINT32)) << EURASIA_PDS_DOUTA1_AO_SHIFT));
#if defined(SUPPORT_SGX_HWPERF)
	PDSUKERNEL_EVENTSSetINPUT_TIMESTAMP_PA_DEST((IMG_PUINT32)psEventPDSMemInfo->pvLinAddr, ((offsetof(PVRSRV_SGX_EDMPROG_PRIMATTR, ui32TimeStamp) / sizeof(IMG_UINT32)) << EURASIA_PDS_DOUTA1_AO_SHIFT));
#endif
	PDSUKERNEL_EVENTSSetINPUT_DOUTU1 ((IMG_PUINT32)psEventPDSMemInfo->pvLinAddr, aui32USETaskCtrl[1]);
	PDSUKERNEL_EVENTSSetINPUT_DOUTU2 ((IMG_PUINT32)psEventPDSMemInfo->pvLinAddr, aui32USETaskCtrl[2]);
	PDSUKERNEL_EVENTSSetINPUT_GENERIC_DOUTU0 ((IMG_PUINT32)psEventPDSMemInfo->pvLinAddr, aui32USETaskCtrl[0] | ui32USETaskCtrlAddrGeneric);

	#if defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
	sUSSEAddr.uiAddr = psUSEMemInfo->sDevVAddr.uiAddr + GetuKernelProgramTAEVENTHANDLEROffset();
	#else
	sUSSEAddr.uiAddr = psUSETAMemInfo->sDevVAddr.uiAddr + GetuKernelProgramTA_TAEVENTHANDLEROffset();
	#endif
	SetUSEExecutionAddress(&ui32USETaskCtrlAddrTA, 0, sUSSEAddr,
							psHeapInfo->psKernelCodeHeap->sDevVAddrBase, SGX_KERNEL_USE_CODE_BASE_INDEX);

	#if defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
	sUSSEAddr.uiAddr = psUSEMemInfo->sDevVAddr.uiAddr + UKERNEL_SPMEVENTHANDLER_OFFSET;
	#else
	sUSSEAddr.uiAddr = psUSESPMMemInfo->sDevVAddr.uiAddr + UKERNEL_SPM_SPMEVENTHANDLER_OFFSET;
	#endif
	SetUSEExecutionAddress(&ui32USETaskCtrlAddrSPM, 0, sUSSEAddr,
							psHeapInfo->psKernelCodeHeap->sDevVAddrBase, SGX_KERNEL_USE_CODE_BASE_INDEX);

	#if defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
	sUSSEAddr.uiAddr = psUSEMemInfo->sDevVAddr.uiAddr + GetuKernelProgramTHREEDEVENTHANDLEROffset();
	#else
	sUSSEAddr.uiAddr = psUSE3DMemInfo->sDevVAddr.uiAddr + GetuKernelProgram3D_THREEDEVENTHANDLEROffset();
	#endif
	SetUSEExecutionAddress(&ui32USETaskCtrlAddr3D, 0, sUSSEAddr,
							psHeapInfo->psKernelCodeHeap->sDevVAddrBase, SGX_KERNEL_USE_CODE_BASE_INDEX);

	#if defined(SGX_FEATURE_2D_HARDWARE)
	#if defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
	sUSSEAddr.uiAddr = psUSEMemInfo->sDevVAddr.uiAddr + UKERNEL_TWODEVENTHANDLER_OFFSET;
	#else
	sUSSEAddr.uiAddr = psUSE2DMemInfo->sDevVAddr.uiAddr + UKERNEL_2D_TWODEVENTHANDLER_OFFSET;	
	#endif
	SetUSEExecutionAddress(&ui32USETaskCtrlAddr2D, 0, sUSSEAddr,
							psHeapInfo->psKernelCodeHeap->sDevVAddrBase, SGX_KERNEL_USE_CODE_BASE_INDEX);
	#endif

	PDSUKERNEL_EVENTSSetINPUT_TA_DOUTU0((IMG_PUINT32)psEventPDSMemInfo->pvLinAddr, aui32USETaskCtrl[0] | ui32USETaskCtrlAddrTA);
	PDSUKERNEL_EVENTSSetINPUT_SPM_DOUTU0((IMG_PUINT32)psEventPDSMemInfo->pvLinAddr, aui32USETaskCtrl[0] | ui32USETaskCtrlAddrSPM);
	PDSUKERNEL_EVENTSSetINPUT_3D_DOUTU0((IMG_PUINT32)psEventPDSMemInfo->pvLinAddr, aui32USETaskCtrl[0] | ui32USETaskCtrlAddr3D);
	#if defined(SGX_FEATURE_2D_HARDWARE)
	PDSUKERNEL_EVENTSSetINPUT_2D_DOUTU0((IMG_PUINT32)psEventPDSMemInfo->pvLinAddr, aui32USETaskCtrl[0] | ui32USETaskCtrlAddr2D);
	#endif

	/*
		Pdump the PDS event handler program
	*/
	PDUMPCOMMENT(psDevData->psConnection, "PDS program for EDM other events", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psEventPDSMemInfo, 0, sizeof(g_pui32PDSUKERNEL_EVENTS), PDUMP_FLAGS_CONTINUOUS);

	/* Reset USE addresses */
	ui32USETaskCtrlAddrTA = ui32USETaskCtrlAddrSPM = ui32USETaskCtrlAddr3D = 0;
#if defined(SGX_FEATURE_2D_HARDWARE)
	ui32USETaskCtrlAddr2D = 0;
#endif

	/*
		Setup the USE code addresses for the loopback handlers
	*/
#if defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
	sUSSEAddr.uiAddr = psUSEMemInfo->sDevVAddr.uiAddr + GetuKernelProgramTAHANDLEROffset();
#else
	sUSSEAddr.uiAddr = psUSETAMemInfo->sDevVAddr.uiAddr + GetuKernelProgramTA_TAHANDLEROffset();
#endif
	SetUSEExecutionAddress(&ui32USETaskCtrlAddrTA, 0, sUSSEAddr,
							psHeapInfo->psKernelCodeHeap->sDevVAddrBase, SGX_KERNEL_USE_CODE_BASE_INDEX);

#if defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
	sUSSEAddr.uiAddr = psUSEMemInfo->sDevVAddr.uiAddr + UKERNEL_SPMHANDLER_OFFSET;
#else
	sUSSEAddr.uiAddr = psUSESPMMemInfo->sDevVAddr.uiAddr + UKERNEL_SPM_SPMHANDLER_OFFSET;
#endif
	SetUSEExecutionAddress(&ui32USETaskCtrlAddrSPM, 0, sUSSEAddr,
							psHeapInfo->psKernelCodeHeap->sDevVAddrBase, SGX_KERNEL_USE_CODE_BASE_INDEX);

#if defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
	sUSSEAddr.uiAddr = psUSEMemInfo->sDevVAddr.uiAddr + GetuKernelProgramTHREEDHANDLEROffset();
#else
	sUSSEAddr.uiAddr = psUSE3DMemInfo->sDevVAddr.uiAddr + GetuKernelProgram3D_THREEDHANDLEROffset();
#endif
	SetUSEExecutionAddress(&ui32USETaskCtrlAddr3D, 0, sUSSEAddr,
							psHeapInfo->psKernelCodeHeap->sDevVAddrBase, SGX_KERNEL_USE_CODE_BASE_INDEX);

#if defined(SGX_FEATURE_2D_HARDWARE)
#if defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
	sUSSEAddr.uiAddr = psUSEMemInfo->sDevVAddr.uiAddr + UKERNEL_TWODHANDLER_OFFSET;
#else
	sUSSEAddr.uiAddr = psUSE2DMemInfo->sDevVAddr.uiAddr + UKERNEL_2D_TWODHANDLER_OFFSET;
#endif
	SetUSEExecutionAddress(&ui32USETaskCtrlAddr2D, 0, sUSSEAddr,
							psHeapInfo->psKernelCodeHeap->sDevVAddrBase, SGX_KERNEL_USE_CODE_BASE_INDEX);
#endif

	/* Allocate memory for the Loopback PDS program */
	PDUMPCOMMENT(psDevData->psConnection, "Allocate PDS program for ukernel loopback events", IMG_TRUE);
	if (PVRSRVAllocDeviceMem(psDevData,
								 hDevKernelCodeHeap,
								 PVRSRV_MEM_READ | PVRSRV_MEM_NO_SYNCOBJ,
								 sizeof(g_pui32PDSUKERNEL_LB),
								 (1 << EURASIA_PDSSB1_PDSEXECADDR_ALIGNSHIFT),
								 &psDevInfo->psMicrokernelPDSLoopbackMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"LoaduKernelPrograms: Failed to allocate PDS program for uKernel loopback events"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	psLoopbackPDSMemInfo = psDevInfo->psMicrokernelPDSLoopbackMemInfo;

	PVR_DPF((PVR_DBG_MESSAGE,"LoaduKernel : uKernel loopback PDS program Device Addr: 0x%x Size: 0x%x",
			psLoopbackPDSMemInfo->sDevVAddr.uiAddr, sizeof(g_pui32PDSUKERNEL_LB)));

	/*
		Copy the loopback PDS program
	*/
	PVRSRVMemCopy((IMG_UINT8 *)psLoopbackPDSMemInfo->pvLinAddr, (const IMG_UINT8 *)g_pui32PDSUKERNEL_LB, sizeof(g_pui32PDSUKERNEL_LB));

	/* Fill in the contants */
	PDSUKERNEL_LBSetINPUT_IR0_PA_DEST((IMG_PUINT32)psLoopbackPDSMemInfo->pvLinAddr, ((offsetof(PVRSRV_SGX_EDMPROG_PRIMATTR, ui32PDSIn0) / sizeof(IMG_UINT32)) << EURASIA_PDS_DOUTA1_AO_SHIFT));
#if defined(SUPPORT_SGX_HWPERF)
	PDSUKERNEL_LBSetINPUT_TIMESTAMP_PA_DEST((IMG_PUINT32)psLoopbackPDSMemInfo->pvLinAddr, ((offsetof(PVRSRV_SGX_EDMPROG_PRIMATTR, ui32TimeStamp) / sizeof(IMG_UINT32)) << EURASIA_PDS_DOUTA1_AO_SHIFT));
#endif
	PDSUKERNEL_LBSetINPUT_TAEVENTS((IMG_PUINT32)psLoopbackPDSMemInfo->pvLinAddr, (USE_LOOPBACK_TAFINISHED |
	#if (SGX_FEATURE_NUM_USE_PIPES > 1)
																					USE_LOOPBACK_INITPB |
	#endif
																					USE_LOOPBACK_FINDTA));
	PDSUKERNEL_LBSetINPUT_3DEVENTS((IMG_PUINT32)psLoopbackPDSMemInfo->pvLinAddr, (USE_LOOPBACK_3DMEMFREE |USE_LOOPBACK_PIXENDRENDER |
																				USE_LOOPBACK_ISPHALT | USE_LOOPBACK_ISPBREAKPOINT |
																				USE_LOOPBACK_FIND3D));
	PDSUKERNEL_LBSetINPUT_3D_DOUTU0((IMG_PUINT32)psLoopbackPDSMemInfo->pvLinAddr, aui32USETaskCtrl[0] | ui32USETaskCtrlAddr3D);
	PDSUKERNEL_LBSetINPUT_TA_DOUTU0((IMG_PUINT32)psLoopbackPDSMemInfo->pvLinAddr, aui32USETaskCtrl[0] | ui32USETaskCtrlAddrTA);
	PDSUKERNEL_LBSetINPUT_SPM_DOUTU0((IMG_PUINT32)psLoopbackPDSMemInfo->pvLinAddr, aui32USETaskCtrl[0] | ui32USETaskCtrlAddrSPM);
	PDSUKERNEL_LBSetINPUT_DOUTU1((IMG_PUINT32)psLoopbackPDSMemInfo->pvLinAddr, aui32USETaskCtrl[1]);
	PDSUKERNEL_LBSetINPUT_DOUTU2((IMG_PUINT32)psLoopbackPDSMemInfo->pvLinAddr, aui32USETaskCtrl[2]);
#if defined(SGX_FEATURE_2D_HARDWARE)
	PDSUKERNEL_LBSetINPUT_2D_DOUTU0((IMG_PUINT32)psLoopbackPDSMemInfo->pvLinAddr, aui32USETaskCtrl[0] | ui32USETaskCtrlAddr2D);
	PDSUKERNEL_LBSetINPUT_2DEVENTS((IMG_PUINT32)psLoopbackPDSMemInfo->pvLinAddr, (USE_LOOPBACK_2DCOMPLETE | USE_LOOPBACK_FIND2D));

#endif

	/*
		Pdump the PDS program
	*/
	PDUMPCOMMENT(psDevData->psConnection, "PDS program for uKernel loopback events", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psLoopbackPDSMemInfo, 0, sizeof(g_pui32PDSUKERNEL_LB), PDUMP_FLAGS_CONTINUOUS);

	/*
		Derive the sideband words in the emitpds for the primary loopback.
	*/
	ui32PDSExecBase = psLoopbackPDSMemInfo->sDevVAddr.uiAddr;
#if !defined(SGX_FEATURE_EDM_VERTEX_PDSADDR_FULL_RANGE)
	ui32PDSExecBase -= psDevInfo->sDevVAddrPDSExecBase.uiAddr;
#endif /* defined(SGX_FEATURE_EDM_VERTEX_PDSADDR_FULL_RANGE) */
	sLoopbackDevAddr.uiAddr = (ui32PDSExecBase >> EURASIA_PDSSB1_PDSEXECADDR_ALIGNSHIFT) << EURASIA_PDSSB1_PDSEXECADDR_SHIFT;

#if defined(SGX_FEATURE_UNIFIED_TEMPS_AND_PAS)
	ui32USENumAttrs = SGX_UKERNEL_NUM_PRIM_ATTRIB + SGX_UKERNEL_NUM_TEMPS;
#else
	ui32USENumAttrs	= SGX_UKERNEL_NUM_PRIM_ATTRIB;
#endif

#if defined(EURASIA_PDSSB1_USEPIPE_PIPE0)
	sLoopbackDevAddr.uiAddr |= (EURASIA_PDSSB1_USEPIPE_PIPE0 << EURASIA_PDSSB1_USEPIPE_SHIFT);
	ui32LoopbackConstSize = ((PDS_UKERNEL_LB_DATA_SEGMENT_SIZE >> EURASIA_PDSSB0_PDSDATASIZE_ALIGNSHIFT) << EURASIA_PDSSB0_PDSDATASIZE_SHIFT) |
										 SGX_LOOPBACK_ATTRIB_SPACE(ui32USENumAttrs);
#else
	ui32LoopbackConstSize = ((PDS_UKERNEL_LB_DATA_SEGMENT_SIZE >> EURASIA_PDSSB0_PDSDATASIZE_ALIGNSHIFT) << EURASIA_PDSSB0_PDSDATASIZE_SHIFT) |
	#if defined(EURASIA_PDSSB0_USEPIPE_PIPE0)
										 (EURASIA_PDSSB0_USEPIPE_PIPE0 << EURASIA_PDSSB0_USEPIPE_SHIFT) |
	#endif
										 SGX_LOOPBACK_ATTRIB_SPACE(ui32USENumAttrs);
#endif
	PDUMPCOMMENT(psDevData->psConnection, "Patch PDS loopback vaddr and const size", IMG_TRUE);
	/* PRQA S 3305 END_ALIGN_OVERRIDE  */ /* override pointer alignment */
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSEMemInfo->pvLinAddr + GetuKernelProgramELB_PDSCONSTSIZEOffset()), ui32LoopbackConstSize);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSEMemInfo->pvLinAddr + GetuKernelProgramELB_PDSDEVADDROffset()), sLoopbackDevAddr.uiAddr);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSEMemInfo, GetuKernelProgramELB_PDSCONSTSIZEOffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSEMemInfo, GetuKernelProgramELB_PDSDEVADDROffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);

#if !defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSETAMemInfo->pvLinAddr + GetuKernelProgramTA_ELB_PDSCONSTSIZEOffset()), ui32LoopbackConstSize);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSETAMemInfo->pvLinAddr + GetuKernelProgramTA_ELB_PDSDEVADDROffset()), sLoopbackDevAddr.uiAddr);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSE3DMemInfo->pvLinAddr + GetuKernelProgram3D_ELB_PDSCONSTSIZEOffset()), ui32LoopbackConstSize);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSE3DMemInfo->pvLinAddr + GetuKernelProgram3D_ELB_PDSDEVADDROffset()), sLoopbackDevAddr.uiAddr);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSESPMMemInfo->pvLinAddr + UKERNEL_SPM_ELB_PDSCONSTSIZE_OFFSET), ui32LoopbackConstSize);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSESPMMemInfo->pvLinAddr + UKERNEL_SPM_ELB_PDSDEVADDR_OFFSET), sLoopbackDevAddr.uiAddr);

	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSETAMemInfo, GetuKernelProgramTA_ELB_PDSCONSTSIZEOffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSETAMemInfo, GetuKernelProgramTA_ELB_PDSDEVADDROffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSE3DMemInfo, GetuKernelProgram3D_ELB_PDSCONSTSIZEOffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSE3DMemInfo, GetuKernelProgram3D_ELB_PDSDEVADDROffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSESPMMemInfo, UKERNEL_SPM_ELB_PDSCONSTSIZE_OFFSET, EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSESPMMemInfo, UKERNEL_SPM_ELB_PDSDEVADDR_OFFSET, EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);

	#if defined(SGX_FEATURE_2D_HARDWARE)
	PDUMPCOMMENT(psDevData->psConnection, "Patch PDS loopback vaddr and const size for 2D loopback", IMG_TRUE);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSE2DMemInfo->pvLinAddr + UKERNEL_2D_ELB_PDSCONSTSIZE_OFFSET), ui32LoopbackConstSize);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSE2DMemInfo->pvLinAddr + UKERNEL_2D_ELB_PDSDEVADDR_OFFSET), sLoopbackDevAddr.uiAddr);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSE2DMemInfo, UKERNEL_2D_ELB_PDSCONSTSIZE_OFFSET, EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSE2DMemInfo, UKERNEL_2D_ELB_PDSDEVADDR_OFFSET, EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	#endif
#endif


#if defined(SGX_FAST_DPM_INIT) && (SGX_FEATURE_NUM_USE_PIPES > 1)
	/* make it emit to pipe 1 */
	#if defined(EURASIA_PDSSB1_USEPIPE_SHIFT)
	sLoopbackDevAddr.uiAddr &= EURASIA_PDSSB1_USEPIPE_CLRMSK;
	sLoopbackDevAddr.uiAddr |= (EURASIA_PDSSB1_USEPIPE_PIPE1 << EURASIA_PDSSB1_USEPIPE_SHIFT);
	#else
	#if defined(EURASIA_PDSSB0_USEPIPE_SHIFT)
	ui32LoopbackConstSize &= EURASIA_PDSSB0_USEPIPE_CLRMSK;
	ui32LoopbackConstSize |= (EURASIA_PDSSB0_USEPIPE_PIPE1 << EURASIA_PDSSB0_USEPIPE_SHIFT);
	#endif
	#endif

	PDUMPCOMMENT(psDevData->psConnection, "Patch PDS loopback vaddr and const size for DPM PT loopback", IMG_TRUE);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSEMemInfo->pvLinAddr + GetuKernelProgramSDPT_PDSCONSTSIZEOffset()), ui32LoopbackConstSize);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSEMemInfo->pvLinAddr + GetuKernelProgramSDPT_PDSDEVADDROffset()), sLoopbackDevAddr.uiAddr);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSEMemInfo, GetuKernelProgramSDPT_PDSCONSTSIZEOffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSEMemInfo, GetuKernelProgramSDPT_PDSDEVADDROffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);

	#if !defined(SGX_FEATURE_MONOLITHIC_UKERNEL)
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSETAMemInfo->pvLinAddr + GetuKernelProgramTA_SDPT_PDSCONSTSIZEOffset()), ui32LoopbackConstSize);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSETAMemInfo->pvLinAddr + GetuKernelProgramTA_SDPT_PDSDEVADDROffset()), sLoopbackDevAddr.uiAddr);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSETAMemInfo, GetuKernelProgramTA_SDPT_PDSCONSTSIZEOffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSETAMemInfo, GetuKernelProgramTA_SDPT_PDSDEVADDROffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	#endif
#endif

	/*
		Derive the sideband words in the emitpds for the secondary init loopback.
	*/
	ui32PDSExecBase = psDevInfo->psMicrokernelPDSInitSecondaryMemInfo->sDevVAddr.uiAddr;
#if !defined(SGX_FEATURE_EDM_VERTEX_PDSADDR_FULL_RANGE)
	ui32PDSExecBase -= psDevInfo->sDevVAddrPDSExecBase.uiAddr;
#endif /* defined(SGX_FEATURE_EDM_VERTEX_PDSADDR_FULL_RANGE) */
	sLoopbackDevAddr.uiAddr = (ui32PDSExecBase >> EURASIA_PDSSB1_PDSEXECADDR_ALIGNSHIFT) << EURASIA_PDSSB1_PDSEXECADDR_SHIFT;

#if defined(SGX_FEATURE_UNIFIED_TEMPS_AND_PAS)
	ui32USENumAttrs = SGX_UKERNEL_NUM_SEC_ATTRIB + 0;
#else
	ui32USENumAttrs	= SGX_UKERNEL_NUM_SEC_ATTRIB;
#endif

#if defined(EURASIA_PDSSB1_USEPIPE_PIPE0)
	sLoopbackDevAddr.uiAddr |= (EURASIA_PDSSB1_USEPIPE_PIPE0 << EURASIA_PDSSB1_USEPIPE_SHIFT);
	ui32LoopbackConstSize = ((PDS_UKERNEL_INIT_SEC_DATA_SEGMENT_SIZE >> EURASIA_PDSSB0_PDSDATASIZE_ALIGNSHIFT) << EURASIA_PDSSB0_PDSDATASIZE_SHIFT) |
										 SGX_LOOPBACK_ATTRIB_SPACE(ui32USENumAttrs);
#else
	ui32LoopbackConstSize = ((PDS_UKERNEL_INIT_SEC_DATA_SEGMENT_SIZE >> EURASIA_PDSSB0_PDSDATASIZE_ALIGNSHIFT) << EURASIA_PDSSB0_PDSDATASIZE_SHIFT) |
	#if defined(EURASIA_PDSSB0_USEPIPE_PIPE0)
										 (EURASIA_PDSSB0_USEPIPE_PIPE0 << EURASIA_PDSSB0_USEPIPE_SHIFT) |
	#endif
										 SGX_LOOPBACK_ATTRIB_SPACE(ui32USENumAttrs);
#endif

	PDUMPCOMMENT(psDevData->psConnection, "Patch Primary ukernel init program 1", IMG_TRUE);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSEMemInfo->pvLinAddr + GetuKernelProgramIUKP1_PDSCONSTSIZESECOffset()), ui32LoopbackConstSize);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSEMemInfo->pvLinAddr + GetuKernelProgramIUKP1_PDSDEVADDRSECOffset()), sLoopbackDevAddr.uiAddr);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSEMemInfo->pvLinAddr + GetuKernelProgramIUKP1_TA3DCTLOffset()), psDevInfo->psTA3DCtlMemInfo->sDevVAddr.uiAddr);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSEMemInfo, GetuKernelProgramIUKP1_PDSCONSTSIZESECOffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSEMemInfo, GetuKernelProgramIUKP1_PDSDEVADDRSECOffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSEMemInfo, GetuKernelProgramIUKP1_TA3DCTLOffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);

	/*
		Derive the sideband words in the emitpds for the primary init loopback.
	*/
	ui32PDSExecBase = psDevInfo->psMicrokernelPDSInitPrimary2MemInfo->sDevVAddr.uiAddr;
#if !defined(SGX_FEATURE_EDM_VERTEX_PDSADDR_FULL_RANGE)
	ui32PDSExecBase -= psDevInfo->sDevVAddrPDSExecBase.uiAddr;
#endif /* defined(SGX_FEATURE_EDM_VERTEX_PDSADDR_FULL_RANGE) */
	sLoopbackDevAddr.uiAddr = (ui32PDSExecBase >> EURASIA_PDSSB1_PDSEXECADDR_ALIGNSHIFT) << EURASIA_PDSSB1_PDSEXECADDR_SHIFT;

#if defined(SGX_FEATURE_UNIFIED_TEMPS_AND_PAS)
	ui32USENumAttrs = SGX_UKERNEL_NUM_PRIM_ATTRIB + SGX_UKERNEL_NUM_TEMPS;
#else
	ui32USENumAttrs	= SGX_UKERNEL_NUM_PRIM_ATTRIB;
#endif

#if defined(EURASIA_PDSSB1_USEPIPE_PIPE0)
	sLoopbackDevAddr.uiAddr |= (EURASIA_PDSSB1_USEPIPE_PIPE0 << EURASIA_PDSSB1_USEPIPE_SHIFT);
	ui32LoopbackConstSize = ((PDS_UKERNEL_INIT_PRIM2_DATA_SEGMENT_SIZE >> EURASIA_PDSSB0_PDSDATASIZE_ALIGNSHIFT) << EURASIA_PDSSB0_PDSDATASIZE_SHIFT) |
										 SGX_LOOPBACK_ATTRIB_SPACE(ui32USENumAttrs);
#else
	ui32LoopbackConstSize = ((PDS_UKERNEL_INIT_PRIM2_DATA_SEGMENT_SIZE >> EURASIA_PDSSB0_PDSDATASIZE_ALIGNSHIFT) << EURASIA_PDSSB0_PDSDATASIZE_SHIFT) |
	#if defined(EURASIA_PDSSB0_USEPIPE_PIPE0)
										 (EURASIA_PDSSB0_USEPIPE_PIPE0 << EURASIA_PDSSB0_USEPIPE_SHIFT) |
	#endif
										 SGX_LOOPBACK_ATTRIB_SPACE(ui32USENumAttrs);
#endif

	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSEMemInfo->pvLinAddr + GetuKernelProgramIUKP1_PDSCONSTSIZEPRIM2Offset()), ui32LoopbackConstSize);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSEMemInfo->pvLinAddr + GetuKernelProgramIUKP1_PDSDEVADDRPRIM2Offset()), sLoopbackDevAddr.uiAddr);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSEMemInfo, GetuKernelProgramIUKP1_PDSCONSTSIZEPRIM2Offset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSEMemInfo, GetuKernelProgramIUKP1_PDSDEVADDRPRIM2Offset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);

	/*
		Derive the values that will be patched into EUR_CR_EVENT_OTHER_PDS_EXEC and
		EUR_CR_EVENT_OTHER_PDS_DATA.
	*/
	PDUMPCOMMENT(psDevData->psConnection, "Patch Primary ukernel init program 2", IMG_TRUE);
	ui32PDSExecBase = psDevInfo->psMicrokernelPDSEventMemInfo->sDevVAddr.uiAddr;
#if !defined(SGX_FEATURE_EDM_VERTEX_PDSADDR_FULL_RANGE)
	ui32PDSExecBase -= psDevInfo->sDevVAddrPDSExecBase.uiAddr;
#endif /* defined(SGX_FEATURE_EDM_VERTEX_PDSADDR_FULL_RANGE) */
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSEMemInfo->pvLinAddr + GetuKernelProgramIUKP2_PDSEXECOffset()),
			  (ui32PDSExecBase >> EUR_CR_EVENT_OTHER_PDS_EXEC_ADDR_ALIGNSHIFT) << EUR_CR_EVENT_OTHER_PDS_EXEC_ADDR_SHIFT);
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psUSEMemInfo->pvLinAddr + GetuKernelProgramIUKP2_PDSDATAOffset()),
			  (PDS_UKERNEL_EVENTS_DATA_SEGMENT_SIZE >> EUR_CR_EVENT_OTHER_PDS_DATA_SIZE_ALIGNSHIFT) << EUR_CR_EVENT_OTHER_PDS_DATA_SIZE_SHIFT);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSEMemInfo, GetuKernelProgramIUKP2_PDSEXECOffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psUSEMemInfo, GetuKernelProgramIUKP2_PDSDATAOffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);


/* PRQA L:END_ALIGN_OVERRIDE */


	return PVRSRV_OK;
}

 
#if defined(FIX_HW_BRN_31542) || defined(FIX_HW_BRN_36513)

/*!
*******************************************************************************

 @Function	NonEDMProtectedAllocateuKernelBuffer

 @Description

 Allocates a buffer for the microkernel to populate with diagnostic data

 @Input psDevData
 @Input psHeapInfo
 @Input ui32Size
 @Input ui32Alignment
 @Output ppsMemInfo
 @Input pszBufferName

 @Return PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR NonEDMProtectedAllocateuKernelBuffer(PVRSRV_DEV_DATA			*psDevData,
										  SGX_HEAP_INFO				*psHeapInfo,
										  IMG_UINT32				ui32Size,
										  IMG_UINT32				ui32Alignment,
										  PVRSRV_CLIENT_MEM_INFO	**ppsMemInfo,
										  IMG_CHAR					*pszBufferName)
{
#if !defined(PDUMP) && !defined(DEBUG)
	PVR_UNREFERENCED_PARAMETER(pszBufferName);
#endif

	/*
		Allocate memory for the buffer.
	*/
	PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "Allocate memory for %s", pszBufferName);
	if (PVRSRVAllocDeviceMem(psDevData,
							 psHeapInfo->psKernelDataHeap->hDevMemHeap,
							 PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | 
								PVRSRV_MEM_NO_SYNCOBJ | PVRSRV_MEM_CACHE_CONSISTENT,
							 ui32Size,
							 ui32Alignment,
							 ppsMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupuKernel : Failed to allocate %s.", pszBufferName));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	
	/* Zero the Buffer. */
	PVRSRVMemSet((*ppsMemInfo)->pvLinAddr, 0, (*ppsMemInfo)->uAllocSize);
	PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "Zero the %s", pszBufferName);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, *ppsMemInfo, 0, (*ppsMemInfo)->uAllocSize, PDUMP_FLAGS_CONTINUOUS);

	return PVRSRV_OK;
}

static PVRSRV_ERROR SetClipperBugUSEExecutionAddress(IMG_PVOID pvPDSProgLinAddr, IMG_UINT32 ui32PDSProgNumber, IMG_UINT32 ui32USEProgAddress)
{
	IMG_UINT32 ui32USE0dword;
	IMG_UINT32 ui32ExecAddr;
	ui32USE0dword = (SGX_CLIPPERBUG_HWBRN_CODE_BASE_INDEX << EURASIA_PDS_DOUTU0_CBASE_SHIFT) & ~EURASIA_PDS_DOUTU0_CBASE_CLRMSK;
	ui32ExecAddr = ui32USEProgAddress - (ui32USEProgAddress & ~((1<<SGX_USE_CODE_SEGMENT_RANGE_BITS)-1));
#if defined(SGX_FEATURE_USE_UNLIMITED_PHASES)
	ui32USE0dword |= ((ui32ExecAddr >> EURASIA_PDS_DOUTU0_EXE_ALIGNSHIFT) << EURASIA_PDS_DOUTU0_EXE_SHIFT)& ~EURASIA_PDS_DOUTU0_EXE_CLRMSK;
	ui32USE0dword |= ( (SGX_CLIPPERBUG_NUMTEMPS << EURASIA_PDS_DOUTU0_TRC_SHIFT) | (1 << EURASIA_PDS_DOUTU0_TRC_SHIFT) );
#else
	ui32USE0dword |= ((ui32ExecAddr>>EURASIA_PDS_DOUTU0_COFF_ALIGNSHIFT) << EURASIA_PDS_DOUTU0_COFF_SHIFT) & ~ EURASIA_PDS_DOUTU0_COFF_CLRMSK;
	ui32USE0dword |= ((ui32ExecAddr >> EURASIA_PDS_DOUTU0_EXE_ALIGNSHIFT) << EURASIA_PDS_DOUTU0_EXE_SHIFT) & ~EURASIA_PDS_DOUTU0_EXE_CLRMSK;
#endif

	((IMG_UINT32 *)(pvPDSProgLinAddr))
	[(ui32PDSProgNumber * SGX_CLIPPERBUG_PDS_PROGRAM_SIZE + 
		SGX_CLIPPERBUG_PDSPROG_USE0OFFSET) / sizeof(IMG_UINT32)]  = ui32USE0dword;

	return PVRSRV_OK;
}

#endif
 

/*!
*******************************************************************************

 @Function	AllocateuKernelBuffer

 @Description

 Allocates a buffer for the microkernel to populate with diagnostic data

 @Input psDevData
 @Input psHeapInfo
 @Input ui32Size
 @Input ui32Alignment
 @Output ppsMemInfo
 @Input pszBufferName

 @Return PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR AllocateuKernelBuffer(PVRSRV_DEV_DATA			*psDevData,
										  SGX_HEAP_INFO				*psHeapInfo,
										  IMG_UINT32				ui32Size,
										  IMG_UINT32				ui32Alignment,
										  PVRSRV_CLIENT_MEM_INFO	**ppsMemInfo,
										  IMG_CHAR					*pszBufferName)
{
#if !defined(PDUMP) && !defined(DEBUG)
	PVR_UNREFERENCED_PARAMETER(pszBufferName);
#endif

	/*
		Allocate memory for the buffer.
	*/
	PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "Allocate memory for %s", pszBufferName);
	if (PVRSRVAllocDeviceMem(psDevData,
							 psHeapInfo->psKernelDataHeap->hDevMemHeap,
							 PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_EDM_PROTECT |
								PVRSRV_MEM_NO_SYNCOBJ | PVRSRV_MEM_CACHE_CONSISTENT,
							 ui32Size,
							 ui32Alignment,
							 ppsMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupuKernel : Failed to allocate %s.", pszBufferName));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	
	/* Zero the Buffer. */
	PVRSRVMemSet((*ppsMemInfo)->pvLinAddr, 0, (*ppsMemInfo)->uAllocSize);
	PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "Zero the %s", pszBufferName);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, *ppsMemInfo, 0, (IMG_UINT32)(*ppsMemInfo)->uAllocSize, PDUMP_FLAGS_CONTINUOUS);

	return PVRSRV_OK;
}


/*!
*******************************************************************************

 @Function	SetupuKernel

 @Description

 Setups all the uKernel related data

 @Input psDevInfo

 @Return PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR SetupuKernel(PVRSRV_DEV_DATA *psDevData,
	SGX_HEAP_INFO *psHeapInfo,
	SGX_SRVINIT_INIT_INFO *psSrvInfo,
	SGX_CLIENT_INIT_INFO *psDevInfo)
{
	PVRSRV_ERROR			eError;
	IMG_UINT32				ui32USENumAttrs;
	PVRSRV_CLIENT_MEM_INFO	*psHostCtlMemInfo;
	PVRSRV_CLIENT_MEM_INFO	*psTA3DCtlMemInfo;
	SGXMK_TA3D_CTL			*psTA3DCtrl;
	SGXMK_STATE				*psMKState;
	IMG_UINT32				ui32PDSExecBase;

#if defined (FIX_HW_BRN_31542) || defined(FIX_HW_BRN_36513)
	IMG_UINT32			*pui32TempLinAddr;
	IMG_UINT32			ui32Loop;
#endif
	/*
		Allocate memory for the new-style CCB and its control (this will reference further command data in non-shared CCBs)
	*/
	PDUMPCOMMENT(psDevData->psConnection, "Allocate memory for kernel CCB", IMG_TRUE);
	if (PVRSRVAllocDeviceMem(psDevData,
								 psHeapInfo->psKernelDataHeap->hDevMemHeap,
								 PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_EDM_PROTECT | PVRSRV_MEM_NO_SYNCOBJ | PVRSRV_MEM_CACHE_CONSISTENT,
								 sizeof(PVRSRV_SGX_KERNEL_CCB),
								 0x1000,
								 &psDevInfo->psKernelCCBMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupuKernel: Failed to allocate kernel CCB"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	psDevInfo->psKernelCCB = (PVRSRV_SGX_KERNEL_CCB *) psDevInfo->psKernelCCBMemInfo->pvLinAddr;

	/*
		Allocate memory for the kernel CCB and its control
			(this will reference further command data in non-shared CCBs)
	*/
	PDUMPCOMMENT(psDevData->psConnection, "Allocate memory for kernel CCB control", IMG_TRUE);
	if (PVRSRVAllocDeviceMem(psDevData,
			 psHeapInfo->psKernelDataHeap->hDevMemHeap,
			 PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_EDM_PROTECT | PVRSRV_MEM_NO_SYNCOBJ | PVRSRV_MEM_CACHE_CONSISTENT,
			 sizeof(PVRSRV_SGX_CCB_CTL),
			 0x1000,
			 &psDevInfo->psKernelCCBCtlMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupuKernel: Failed to allocate kernel CCB control"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	psDevInfo->psKernelCCBCtl = (PVRSRV_SGX_CCB_CTL *) psDevInfo->psKernelCCBCtlMemInfo->pvLinAddr;

	/*
		Allocate memory for the kernel CCB event kicker
	*/
	PDUMPCOMMENT(psDevData->psConnection, "Allocate memory for kernel CCB event kicker", IMG_TRUE);
	if (PVRSRVAllocDeviceMem(psDevData,
								 psHeapInfo->psKernelDataHeap->hDevMemHeap,
								 PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_EDM_PROTECT | PVRSRV_MEM_NO_SYNCOBJ | PVRSRV_MEM_CACHE_CONSISTENT,
								 sizeof(IMG_UINT32),
								 0x10,
								 &psDevInfo->psKernelCCBEventKickerMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupuKernel: Failed to allocate kernel CCB event kicker"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	/*
		Allocate memory for the TA3D control
	*/
	PDUMPCOMMENT(psDevData->psConnection, "Allocate memory for TA3D control", IMG_TRUE);
	if (PVRSRVAllocDeviceMem(psDevData,
			 psHeapInfo->psKernelDataHeap->hDevMemHeap,
			 PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_EDM_PROTECT | PVRSRV_MEM_NO_SYNCOBJ
#if defined(DISABLE_CACHECONSISTENT_TA3D_CTL)
			/* FIXME: need to investigate issue further */
#else
			 | PVRSRV_MEM_CACHE_CONSISTENT
#endif
             | PVRSRV_HAP_MULTI_PROCESS,
			 sizeof(SGXMK_TA3D_CTL),
			 32,
			 &psDevInfo->psTA3DCtlMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupuKernel : Failed to allocate TA/3D Control"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	/*
		Allocate memory for the Host Control
	*/
	PDUMPCOMMENT(psDevData->psConnection, "Allocate memory for host control", IMG_TRUE);
	if (PVRSRVAllocDeviceMem(psDevData,
			 psHeapInfo->psKernelDataHeap->hDevMemHeap,
			 PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_EDM_PROTECT | PVRSRV_MEM_NO_SYNCOBJ
#if defined(DISABLE_CACHECONSISTENT_TA3D_CTL)
			/* FIXME: need to investigate issue further */
#else
			 | PVRSRV_MEM_CACHE_CONSISTENT
#endif
             | PVRSRV_HAP_MULTI_PROCESS,
			 sizeof(SGXMKIF_HOST_CTL),
			 32,
			 &psDevInfo->psHostCtlMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupuKernel : Failed to allocate Host Control structure"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	/* setup TA3D_CTL details */
	psTA3DCtlMemInfo = psDevInfo->psTA3DCtlMemInfo;
	PVRSRVMemSet(psTA3DCtlMemInfo->pvLinAddr, 0, sizeof(SGXMK_TA3D_CTL));
	psTA3DCtrl = (SGXMK_TA3D_CTL*)psTA3DCtlMemInfo->pvLinAddr;
	psSrvInfo->psTA3DCtrl = psTA3DCtrl;
	
	psTA3DCtrl->sMKSecAttr.sTA3DCtl.uiAddr = psDevInfo->psTA3DCtlMemInfo->sDevVAddr.uiAddr;
	psTA3DCtrl->sMKSecAttr.sHostCtl.uiAddr = psDevInfo->psHostCtlMemInfo->sDevVAddr.uiAddr;
	psTA3DCtrl->sMKSecAttr.sCCBCtl.uiAddr  = psDevInfo->psKernelCCBCtlMemInfo->sDevVAddr.uiAddr;

#if defined(FIX_HW_BRN_31272) || defined(FIX_HW_BRN_31780) || defined(FIX_HW_BRN_33920)
	{
		PVRSRV_CLIENT_MEM_INFO ** ppsPTLAWriteBackMemInfo = & psDevInfo->psPTLAWriteBackMemInfo;

		if (PVRSRVAllocDeviceMem(psDevData,
					psHeapInfo->psKernelDataHeap->hDevMemHeap,
					PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ
					| PVRSRV_MEM_CACHE_CONSISTENT | PVRSRV_HAP_MULTI_PROCESS,
					4,
					32,
					ppsPTLAWriteBackMemInfo) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SetupuKernel : Failed to allocate psPTLAWriteBackMemInfo"));
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		}
		
		psTA3DCtrl->sPTLAWriteBackDevVAddr = (*ppsPTLAWriteBackMemInfo)->sDevVAddr;
		* (IMG_PUINT32) (*ppsPTLAWriteBackMemInfo)->pvLinAddr = 0; 
		PDUMPCOMMENT(psDevData->psConnection, "Initialize PTLA writeback address", IMG_TRUE);
		PDUMPMEM(psDevData->psConnection, IMG_NULL, *ppsPTLAWriteBackMemInfo, 0, 4, PDUMP_FLAGS_CONTINUOUS);
	}
#endif

#if !defined(SGX_DISABLE_UKERNEL_SECONDARY_STATE)
	psMKState = &psTA3DCtrl->sMKSecAttr.sMKState;
#else
	psMKState = &psTA3DCtrl->sMKState;
#endif /* SGX_DISABLE_UKERNEL_SECONDARY_STATE */

	/* Setup priority scheduling value */
	psMKState->ui32BlockedPriority = SGX_MAX_CONTEXT_PRIORITY-1;

	/* setup kernel CCB details */
	psTA3DCtrl->sKernelCCB = psDevInfo->psKernelCCBMemInfo->sDevVAddr;
	psTA3DCtrl->sKernelPDDevPAddr = psHeapInfo->sPDDevPAddr;

	/* setup HostCtl details */
	psHostCtlMemInfo = psDevInfo->psHostCtlMemInfo;
	PVRSRVMemSet(psHostCtlMemInfo->pvLinAddr, 0, sizeof(SGXMKIF_HOST_CTL));

	/*
	 * 	Allocate memory for the device-addressable buffer used in SGXGetMiscInfo
	 */
	PDUMPCOMMENT(psDevData->psConnection, "Allocate buffer for SGXGetMiscInfo", IMG_TRUE);
	if (PVRSRVAllocDeviceMem(psDevData,
			 psHeapInfo->psKernelDataHeap->hDevMemHeap,
			 PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_EDM_PROTECT | PVRSRV_MEM_NO_SYNCOBJ
			 | PVRSRV_MEM_CACHE_CONSISTENT
			 | PVRSRV_HAP_MULTI_PROCESS,
			 sizeof(PVRSRV_SGX_MISCINFO_INFO),
			 32,
			 &psDevInfo->psKernelSGXMiscMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupuKernel : Failed to allocate buffer for SGXGetMiscInfo"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	PVRSRVMemSet(psDevInfo->psKernelSGXMiscMemInfo->pvLinAddr, 0,
			sizeof(PVRSRV_SGX_MISCINFO_INFO)); /* ensure the heart-beat counter is initialised to zero */

#if defined(FIX_HW_BRN_29702)
	if (PVRSRVAllocDeviceMem(psDevData,
			 psHeapInfo->psKernelDataHeap->hDevMemHeap,
			 PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_EDM_PROTECT | PVRSRV_MEM_NO_SYNCOBJ
			 | PVRSRV_MEM_CACHE_CONSISTENT
			 | PVRSRV_HAP_MULTI_PROCESS,
			 sizeof(IMG_UINT32),
			 32,
			 &psDevInfo->psCFIMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupuKernel : Failed to allocate buffer for SGXGetMiscInfo"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
		
	psTA3DCtrl->sCFIMemDevAddr = psDevInfo->psCFIMemInfo->sDevVAddr;
#endif

#if defined(FIX_HW_BRN_29823)
	/* Allocate memory for the dummy terminate ctrl stream */
	eError = AllocateuKernelBuffer(psDevData, psHeapInfo,
								   (4 + EURASIA_VDM_CTRL_STREAM_BURST_SIZE),
								   (1 << EUR_CR_VDM_CTRL_STREAM_BASE_ADDR_SHIFT),
								   &psDevInfo->psDummyTermStreamMemInfo,
								   "Dummy Terminate Ctrl Stream");
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	*((IMG_UINT32*)psDevInfo->psDummyTermStreamMemInfo->pvLinAddr) = EURASIA_TAOBJTYPE_TERMINATE;
	
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psDummyTermStreamMemInfo, 0, (4 + EURASIA_VDM_CTRL_STREAM_BURST_SIZE), PDUMP_FLAGS_CONTINUOUS);

	psTA3DCtrl->sDummyTermDevAddr = psDevInfo->psDummyTermStreamMemInfo->sDevVAddr;
#endif /* PVRSRV_USSE_EDM_STATUS_DEBUG */

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) && defined(FIX_HW_BRN_31425)
	/* 
		Allocate memory for the dummy context switch snapshot buffer
	*/
	if (PVRSRVAllocDeviceMem(psDevData,
							 psHeapInfo->psKernelDataHeap->hDevMemHeap,
							 PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ,
							 EURASIA_DUMMY_VDM_SNAPSHOT_BUFFER_SIZE,
							 (1 << EURASIA_VDM_SNAPSHOT_BUFFER_ALIGNSHIFT),
							 &psDevInfo->psVDMSnapShotBufferMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupuKernel : Failed to allocate Context Switch Snapshot buffer."));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	PVRSRVMemSet(psDevInfo->psVDMSnapShotBufferMemInfo->pvLinAddr, 0, EURASIA_DUMMY_VDM_SNAPSHOT_BUFFER_SIZE );
	PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "Allocate memory for dummy Context Switch Snapshot buffer (ui32Size = 0x%8X)", EURASIA_DUMMY_VDM_SNAPSHOT_BUFFER_SIZE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psVDMSnapShotBufferMemInfo, 0, EURASIA_DUMMY_VDM_SNAPSHOT_BUFFER_SIZE, PDUMP_FLAGS_CONTINUOUS);

	psTA3DCtrl->sVDMSnapShotBufferDevAddr.uiAddr = psDevInfo->psVDMSnapShotBufferMemInfo->sDevVAddr.uiAddr
		>> EURASIA_VDM_SNAPSHOT_BUFFER_ALIGNSHIFT;
  	PVR_DPF((PVR_DBG_WARNING, "SetupuKernel : Allocate dummy context store buffer at 0x%8x, size 0x%x",
 			 psTA3DCtrl->sVDMSnapShotBufferDevAddr.uiAddr, EURASIA_DUMMY_VDM_SNAPSHOT_BUFFER_SIZE));

 	/* 
 		Allocate memory for the dummy TA control stream buffer
 	*/
	if (PVRSRVAllocDeviceMem(psDevData,
							 psHeapInfo->psKernelDataHeap->hDevMemHeap,
							 PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ,
							 EURASIA_DUMMY_VDM_CTRL_STREAM_BUFFER_SIZE,
							 (1 << EURASIA_VDM_SNAPSHOT_BUFFER_ALIGNSHIFT),
							 &psDevInfo->psVDMCtrlStreamBufferMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupuKernel : Failed to allocate Context Switch Snapshot buffer."));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	PVRSRVMemSet(psDevInfo->psVDMCtrlStreamBufferMemInfo->pvLinAddr, 0, EURASIA_DUMMY_VDM_CTRL_STREAM_BUFFER_SIZE );
	/* Setup the terminate marker in the control stream */
	{
		IMG_UINT32 *pui32CtrlStream = psDevInfo->psVDMCtrlStreamBufferMemInfo->pvLinAddr;
		*pui32CtrlStream = EURASIA_TAOBJTYPE_TERMINATE;
	}
	PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "Allocate memory for dummy Control Stream buffer (ui32Size = 0x%8X)", EURASIA_DUMMY_VDM_CTRL_STREAM_BUFFER_SIZE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psVDMCtrlStreamBufferMemInfo, 0, EURASIA_DUMMY_VDM_SNAPSHOT_BUFFER_SIZE, PDUMP_FLAGS_CONTINUOUS);

	psTA3DCtrl->sVDMCtrlStreamBufferDevAddr.uiAddr = psDevInfo->psVDMCtrlStreamBufferMemInfo->sDevVAddr.uiAddr;

	PVR_DPF((PVR_DBG_WARNING, "SetupuKernel : Allocate dummy control stream buffer at 0x%8x, size 0x%x",
			 psTA3DCtrl->sVDMCtrlStreamBufferDevAddr.uiAddr, EURASIA_DUMMY_VDM_CTRL_STREAM_BUFFER_SIZE));
#endif /* FIX_HW_BRN_31425 */

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) && \
	defined(FIX_HW_BRN_33657) && defined(SUPPORT_SECURE_33657_FIX)
 	/* 
 		Allocate memory for the TA control stream/dummy state update buffer
 	*/
	if (PVRSRVAllocDeviceMem(psDevData,
							 psHeapInfo->psKernelDataHeap->hDevMemHeap,
							 PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ,
							 EURASIA_DUMMY_VDM_CTRL_STREAM_BUFFER_SIZE,
							 (1 << EURASIA_VDM_SNAPSHOT_BUFFER_ALIGNSHIFT),
							 &psDevInfo->psVDMStateUpdateBufferMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupuKernel : Failed to allocate Context Switch state update buffer."));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	PVRSRVMemSet(psDevInfo->psVDMStateUpdateBufferMemInfo->pvLinAddr, 0, EURASIA_DUMMY_VDM_CTRL_STREAM_BUFFER_SIZE );
	{
		IMG_UINT32  ui32AttributeSize = ALIGNSIZE(3, EURASIA_PDS_DOUTU_TRC_ALIGNSHIFT);
		IMG_UINT32 *pui32CtrlStream = psDevInfo->psVDMStateUpdateBufferMemInfo->pvLinAddr;

		/* Align attribute size to blocks of 4 dwords */
		ui32AttributeSize = ALIGNSIZE(ui32AttributeSize * sizeof(IMG_UINT32), EURASIA_TAPDSSTATE_USEATTRIBUTESIZE_ALIGNSHIFT);
		// convert into 128 Words (4 dwords)
		ui32AttributeSize = ui32AttributeSize >> EURASIA_TAPDSSTATE_USEATTRIBUTESIZE_ALIGNSHIFT;

		/* Set up the VDM state update program (PDS code addr will be patched later) */
		*pui32CtrlStream++ = EURASIA_TAOBJTYPE_STATE;
		*pui32CtrlStream++ = ((PDS_VDMCONTEXT_STORE_DATA_SEGMENT_SIZE >> EURASIA_TAPDSSTATE_DATASIZE_ALIGNSHIFT)
								 << EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_DATASIZE_SHIFT) |
							 EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_MTE_EMIT_MASK |
							 EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_SD_MASK |
							 (SGX_FEATURE_NUM_USE_PIPES << EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_USEPIPE_SHIFT) |
	#if defined(SGX545)
							 (2UL << EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_PARTITIONS_SHIFT) |
	#else
							 (1UL << EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_PARTITIONS_SHIFT) |
	#endif
							 (ui32AttributeSize << EUR_CR_VDM_CONTEXT_STORE_STATE1_TAPDSSTATE_USEATTRIBUTESIZE_SHIFT);

		/* Setup the terminate marker in the control stream */
		*pui32CtrlStream++ = EURASIA_TAOBJTYPE_TERMINATE;
	}
	PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "Allocate memory for dummy State Update buffer (ui32Size = 0x%8X)", EURASIA_DUMMY_VDM_CTRL_STREAM_BUFFER_SIZE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psVDMStateUpdateBufferMemInfo, 0, EURASIA_DUMMY_VDM_SNAPSHOT_BUFFER_SIZE, PDUMP_FLAGS_CONTINUOUS);

	psTA3DCtrl->sVDMStateUpdateBufferDevAddr.uiAddr = psDevInfo->psVDMStateUpdateBufferMemInfo->sDevVAddr.uiAddr;

	PVR_DPF((PVR_DBG_WARNING, "SetupuKernel : Allocate dummy state update buffer at 0x%8x, size 0x%x",
			 psTA3DCtrl->sVDMStateUpdateBufferDevAddr.uiAddr, EURASIA_DUMMY_VDM_CTRL_STREAM_BUFFER_SIZE));
#endif

#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG)
	/*
		Allocate memory for the microkernel trace buffer.
	*/
	eError = AllocateuKernelBuffer(psDevData, psHeapInfo,
								   sizeof(SGXMK_TRACE_BUFFER),
								   0x10, // Required by logic analyser
								   &psDevInfo->psEDMStatusBufferMemInfo,
								   "SGX microkernel trace buffer");
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	psMKState->sMKTraceBuffer = psDevInfo->psEDMStatusBufferMemInfo->sDevVAddr;
	psMKState->ui32MKTBOffset = 0;
	{
		IMG_UINT32	ui32MKTraceProfile;

		SGXInitGetAppHint(MKTF_EV | MKTF_EHEV | MKTF_HW | MKTF_SCH | MKTF_POW | MKTF_SO
#if defined(USE_SUPPORT_STATUSVALS_DEBUG)
						  | MKTF_SV
#endif
#if defined(SUPPORT_SGX_LOW_LATENCY_SCHEDULING)
						  | MKTF_CSW
#endif
						  , "MKTraceProfile", &ui32MKTraceProfile);

		psMKState->ui32MKTraceProfile = ui32MKTraceProfile;
	}

	PVR_DPF((PVR_DBG_ERROR, "SetupuKernel : EDM status value DevVAddr: 0x%08X pvLinAddrKM: 0x%08X",
								psDevInfo->psEDMStatusBufferMemInfo->sDevVAddr.uiAddr,
								(IMG_UINTPTR_T)psDevInfo->psEDMStatusBufferMemInfo->pvLinAddrKM));
#endif /* PVRSRV_USSE_EDM_STATUS_DEBUG */

#if defined(SGX_SUPPORT_HWPROFILING)
	/*
		Allocate memory for the profiling structure
	*/
	PDUMPCOMMENTF(psDevData->psConnection, IMG_TRUE, "Allocate memory for HW profiling (ui32Size = 0x%8X)", sizeof(SGXMKIF_HWPROFILING));
	if (PVRSRVAllocDeviceMem(psDevData,
								 psHeapInfo->psKernelDataHeap->hDevMemHeap,
								 PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_EDM_PROTECT | PVRSRV_MEM_NO_SYNCOBJ,
								 sizeof(SGXMKIF_HWPROFILING),
								 32,
								 &psDevInfo->psHWProfilingMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupuKernel : Failed to allocate structure for HW profiling."));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psTA3DCtrl->sHWProfilingDevVAddr = psDevInfo->psHWProfilingMemInfo->sDevVAddr;

	PVRSRVMemSet(psDevInfo->psHWProfilingMemInfo->pvLinAddr, 0, sizeof(SGXMKIF_HWPROFILING));
	PDUMPCOMMENT(psDevData->psConnection,"HW Profiling block", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psHWProfilingMemInfo, 0, sizeof(SGXMKIF_HWPROFILING), PDUMP_FLAGS_CONTINUOUS);

	PVR_DPF((PVR_DBG_WARNING,"HWPROFILING INFO: Profiling block is at DevVAddr: 0x%8.8x",
								psDevInfo->psHWProfilingMemInfo->sDevVAddr.uiAddr));
#endif /* SGX_SUPPORT_HWPROFILING*/

#if defined(SUPPORT_SGX_HWPERF)

	/*
		Allocate memory for the Perf CB structure
	*/
	eError = AllocateuKernelBuffer(psDevData, psHeapInfo,
								   sizeof(SGXMKIF_HWPERF_CB), 32,
								   &psDevInfo->psHWPerfCBMemInfo,
								   "HW Perf Circular Buffer");
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	/* Pass the CB to the uKernel*/
	psTA3DCtrl->sHWPerfCBDevVAddr = psDevInfo->psHWPerfCBMemInfo->sDevVAddr;
	
	{
		IMG_UINT32	ui32HWPerfFlags;

		SGXInitGetAppHint(0, "HWPerfFlags", &ui32HWPerfFlags);

		psMKState->ui32HWPerfFlags = ui32HWPerfFlags;
	}	
#endif /* SUPPORT_SGX_HWPERF*/

	/*
		Allocate memory for the TA and 3D signature buffers
	*/
	eError = AllocateuKernelBuffer(psDevData, psHeapInfo,
								   sizeof(SGXMKIF_TASIG_BUFFER), 32,
								   &psDevInfo->psTASigBufferMemInfo,
								   "TA signature buffer");
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	eError = AllocateuKernelBuffer(psDevData, psHeapInfo,
								   sizeof(SGXMKIF_3DSIG_BUFFER), 32,
								   &psDevInfo->ps3DSigBufferMemInfo,
								   "3D signature buffer");
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	InitSignatureBuffers(psDevData, psDevInfo);

	/* Pass the signature buffer addresses to the uKernel */
	psTA3DCtrl->sTASigBufferDevVAddr = psDevInfo->psTASigBufferMemInfo->sDevVAddr;
	psTA3DCtrl->s3DSigBufferDevVAddr = psDevInfo->ps3DSigBufferMemInfo->sDevVAddr;

	/*
		Setup the USE code addresses for the host kick
	*/
	INITHOSTKICKADDR(TA);
	INITHOSTKICKADDR(TRANSFER);
	#if defined(SGX_FEATURE_2D_HARDWARE)
	INITHOSTKICKADDR(2D);
	#endif /* SGX_FEATURE_2D_HARDWARE */
	INITHOSTKICKADDR(POWER);
	INITHOSTKICKADDR(CONTEXTSUSPEND);
	INITHOSTKICKADDR(CLEANUP);
	INITHOSTKICKADDR(GETMISCINFO);
	INITHOSTKICKADDR(PROCESS_QUEUES);
#if defined(SGX_FEATURE_DATA_BREAKPOINTS)
	INITHOSTKICKADDR(DATABREAKPOINT);
#endif
	INITHOSTKICKADDR(SETHWPERFSTATUS);
#if defined(FIX_HW_BRN_31620)
	INITHOSTKICKADDR(FLUSHPDCACHE);
#endif

	/* Load the uKernel code modules */
	if (LoaduKernel(psDevData, psHeapInfo, psSrvInfo, psDevInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SetupuKernel : Failed to load uKernel programs"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

#if defined(FIX_HW_BRN_31542) || defined(FIX_HW_BRN_36513)
	eError = AllocateuKernelBuffer(psDevData, psHeapInfo,
								   (SGX_CLIPPERBUG_VDM_BUFFER_SIZE),
								   (SGX_128b_ALIGNED), 
								   &psDevInfo->psClearClipWAVDMStreamMemInfo,
								   "VDM control stream for clear clip WA");
	pui32TempLinAddr = psDevInfo->psClearClipWAVDMStreamMemInfo->pvLinAddr;
	PVRSRVMemCopy(pui32TempLinAddr,
			  		(IMG_BYTE *)aui32ClipperBugVDMStream, sizeof(aui32ClipperBugVDMStream));


	eError = AllocateuKernelBuffer(psDevData, psHeapInfo,
								   (SGX_CLIPPERBUG_INDEX_BUFFER_SIZE),
								   (SGX_128b_ALIGNED), 
								   &psDevInfo->psClearClipWAIndexStreamMemInfo,
								   "Index Buffer for clear clip WA");
	pui32TempLinAddr = psDevInfo->psClearClipWAIndexStreamMemInfo->pvLinAddr;
	for (ui32Loop =0; ui32Loop < SGX_CLIPPERBUG_INDEX_BUFFER_SIZE/INDEX_SIZE; ui32Loop++)
	{
		*(pui32TempLinAddr++) = ui32Loop;
		
	}

	eError = AllocateuKernelBuffer(psDevData, psHeapInfo,
								   (SGX_CLIPPERBUG_PDS_BUFFER_SIZE),
								   (SGX_128b_ALIGNED), 
								   &psDevInfo->psClearClipWAPDSMemInfo,
								   "PDS program for clear clip WA");
	PVRSRVMemCopy(psDevInfo->psClearClipWAPDSMemInfo->pvLinAddr,
			  		(IMG_BYTE *)aui32ClipperBugPDSProgram, sizeof(aui32ClipperBugPDSProgram));

	eError = AllocateuKernelBuffer(psDevData, psHeapInfo,
								   (SGX_CLIPPERBUG_USE_BUFFER_SIZE),
								   (SGX_128b_ALIGNED), 
								   &psDevInfo->psClearClipWAUSEMemInfo,
								   "USE program for clear clip WA");
	PVRSRVMemCopy(psDevInfo->psClearClipWAUSEMemInfo->pvLinAddr,
			  		(IMG_BYTE *)pbuClipperBugUSEProgram, sizeof(pbuClipperBugUSEProgram));

	eError = NonEDMProtectedAllocateuKernelBuffer(psDevData, psHeapInfo,
								   (SGX_CLIPPERBUG_PARAMMEM_BUFFER_SIZE),
								   (SGX_1M_ALIGNED), 
								   &psDevInfo->psClearClipWAParamMemInfo,
								   "Parameter Memory for clear clip WA");
	eError = NonEDMProtectedAllocateuKernelBuffer(psDevData, psHeapInfo,
								   (SGX_CLIPPERBUG_DPMPT_BUFFER_SIZE),
								   (SGX_SAFELY_PAGE_ALIGNED),
								   &psDevInfo->psClearClipWAPMPTMemInfo,
								   "DPM page-tables for clear clip WA");


	eError = NonEDMProtectedAllocateuKernelBuffer(psDevData, psHeapInfo,
								   (SGX_CLIPPERBUG_TPC_BUFFER_SIZE),
								   (SGX_SAFELY_PAGE_ALIGNED),
								   &psDevInfo->psClearClipWATPCMemInfo,
								   "TPC Base clear clip WA");

	eError = NonEDMProtectedAllocateuKernelBuffer(psDevData, psHeapInfo,
								   (SGX_CLIPPERBUG_PSGRGNHDR_BUFFER_SIZE),
								   (SGX_SAFELY_PAGE_ALIGNED),
								   &psDevInfo->psClearClipWAPSGRgnHdrMemInfo,
								   "PSGRegion Base for clear clip WA");

	/* Patch the VDM stream with MTE-state update PDS program base */
	((IMG_UINT32 *)(psDevInfo->psClearClipWAVDMStreamMemInfo->pvLinAddr))[SGX_CLIPPERBUG_MTE_UPDATE_PDSPROG_BASE_OFFSET] |= 
		(~EURASIA_TAPDSSTATE_BASEADDR_CLRMSK & (psDevInfo->psClearClipWAPDSMemInfo->sDevVAddr.uiAddr >> EURASIA_TAPDSSTATE_BASEADDR_ALIGNSHIFT));

	/* Patch the index-list with vertex-loading PDS program base */
	((IMG_UINT32 *)(psDevInfo->psClearClipWAVDMStreamMemInfo->pvLinAddr))[SGX_CLIPPERBUG_VTXLOAD_PDSPROG_BASE_OFFSET] |=
		(~EURASIA_VDMPDS_BASEADDR_CLRMSK & ((psDevInfo->psClearClipWAPDSMemInfo->sDevVAddr.uiAddr + 
		 	SGX_CLIPPERBUG_PDS_PROGRAM_SIZE) >>
				EURASIA_TAPDSSTATE_BASEADDR_ALIGNSHIFT));
	
	/* Patch the VDM stream(terminate) with PDS(terminate) program base */
	((IMG_UINT32 *)(psDevInfo->psClearClipWAVDMStreamMemInfo->pvLinAddr))[SGX_CLIPPERBUG_MTETERM_PDSPROG_BASE_OFFSET] |= 
		(~EURASIA_TAPDSSTATE_BASEADDR_CLRMSK & ((psDevInfo->psClearClipWAPDSMemInfo->sDevVAddr.uiAddr + 2*SGX_CLIPPERBUG_PDS_PROGRAM_SIZE) >> EURASIA_TAPDSSTATE_BASEADDR_ALIGNSHIFT));

	/* Patch the index-list with index buffer base */
	((IMG_UINT32 *)(psDevInfo->psClearClipWAVDMStreamMemInfo->pvLinAddr))[SGX_CLIPPERBUG_INDEX_BUFFER_BASE_OFFSET] |=
		(psDevInfo->psClearClipWAIndexStreamMemInfo->sDevVAddr.uiAddr);

	/* Patch the PDS program with USE program address */	
	SetClipperBugUSEExecutionAddress(psDevInfo->psClearClipWAPDSMemInfo->pvLinAddr, 0, 
		(psDevInfo->psClearClipWAUSEMemInfo->sDevVAddr.uiAddr));

	SetClipperBugUSEExecutionAddress(psDevInfo->psClearClipWAPDSMemInfo->pvLinAddr, 1, 
		psDevInfo->psClearClipWAUSEMemInfo->sDevVAddr.uiAddr + SGX_CLIPPERBUG_VTX_SHADER_USE_PROG_OFFSET 
		);

	SetClipperBugUSEExecutionAddress(psDevInfo->psClearClipWAPDSMemInfo->pvLinAddr, 2, 
		psDevInfo->psClearClipWAUSEMemInfo->sDevVAddr.uiAddr + SGX_CLIPPERBUG_TERM_USE_PROG_OFFSET 
		);

	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelUSEMemInfo->pvLinAddr + GetuKernelProgramIKP_LIMMUSECODEBASEADDROffset()), (psDevInfo->psClearClipWAUSEMemInfo->sDevVAddr.uiAddr & ~((1<<SGX_USE_CODE_SEGMENT_RANGE_BITS)-1)));
#if defined(PDUMP)
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelUSEMemInfo, 
				GetuKernelProgramIKP_LIMMUSECODEBASEADDROffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
#endif
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelUSEMemInfo->pvLinAddr + GetuKernelProgramIKP_LIMMPSGRGNHDRADDROffset()), psDevInfo->psClearClipWAPSGRgnHdrMemInfo->sDevVAddr.uiAddr);
#if defined(PDUMP)
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelUSEMemInfo, 
				GetuKernelProgramIKP_LIMMPSGRGNHDRADDROffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
#endif
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelUSEMemInfo->pvLinAddr + GetuKernelProgramIKP_LIMMTPCBASEADDROffset()), psDevInfo->psClearClipWATPCMemInfo->sDevVAddr.uiAddr);
#if defined(PDUMP)
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelUSEMemInfo, 
				GetuKernelProgramIKP_LIMMTPCBASEADDROffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
#endif
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelUSEMemInfo->pvLinAddr + GetuKernelProgramIKP_LIMMPARAMMEMADDROffset()), psDevInfo->psClearClipWAParamMemInfo->sDevVAddr.uiAddr);
#if defined(PDUMP)
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelUSEMemInfo, 
				GetuKernelProgramIKP_LIMMPARAMMEMADDROffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
#endif
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelUSEMemInfo->pvLinAddr + GetuKernelProgramIKP_LIMMDPMPTBASEADDROffset()), psDevInfo->psClearClipWAPMPTMemInfo->sDevVAddr.uiAddr);
#if defined(PDUMP)
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelUSEMemInfo, 
				GetuKernelProgramIKP_LIMMDPMPTBASEADDROffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
#endif
	PatchLIMM((IMG_UINT32*)((IMG_PUINT8)psDevInfo->psMicrokernelUSEMemInfo->pvLinAddr + GetuKernelProgramIKP_LIMMVDMSTREAMBASEADDROffset()), psDevInfo->psClearClipWAVDMStreamMemInfo->sDevVAddr.uiAddr);
#if defined(PDUMP)
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psMicrokernelUSEMemInfo, 
				GetuKernelProgramIKP_LIMMVDMSTREAMBASEADDROffset(), EURASIA_USE_INSTRUCTION_SIZE, PDUMP_FLAGS_CONTINUOUS);
#endif

#if defined(PDUMP)
	PDUMPCOMMENT(psDevData->psConnection, "HWBRN 31542 WA input parameters", IMG_TRUE);


	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psClearClipWAVDMStreamMemInfo, 0,
								   (SGX_CLIPPERBUG_VDM_BUFFER_SIZE),
								   (PDUMP_FLAGS_CONTINUOUS));

	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psClearClipWAIndexStreamMemInfo, 0,
								   (SGX_CLIPPERBUG_INDEX_BUFFER_SIZE),
								   (PDUMP_FLAGS_CONTINUOUS));

	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psClearClipWAPDSMemInfo, 0,
								   (SGX_CLIPPERBUG_PDS_BUFFER_SIZE),
								   (PDUMP_FLAGS_CONTINUOUS));

	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psClearClipWAUSEMemInfo, 0,
								   (SGX_CLIPPERBUG_USE_BUFFER_SIZE),
								   (PDUMP_FLAGS_CONTINUOUS));

	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psClearClipWAParamMemInfo, 0,
								   (SGX_CLIPPERBUG_PARAMMEM_BUFFER_SIZE),
								   (PDUMP_FLAGS_CONTINUOUS));
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psClearClipWAPMPTMemInfo, 0,
								   (SGX_CLIPPERBUG_DPMPT_BUFFER_SIZE),
								   (PDUMP_FLAGS_CONTINUOUS));

	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psClearClipWATPCMemInfo, 0,
								   (SGX_CLIPPERBUG_TPC_BUFFER_SIZE),
								   (PDUMP_FLAGS_CONTINUOUS));

	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psClearClipWAPSGRgnHdrMemInfo, 0,
								   (SGX_CLIPPERBUG_PSGRGNHDR_BUFFER_SIZE),
								   (PDUMP_FLAGS_CONTINUOUS));


#endif

#endif /* FIX_HW_BRN_31542 , FIX_HW_BRN_36513 */

#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) && \
	defined(FIX_HW_BRN_33657) && defined(SUPPORT_SECURE_33657_FIX)
	/* Patch the VDM state update with the PDS code base addr */
 	((IMG_UINT32 *)(psDevInfo->psVDMStateUpdateBufferMemInfo->pvLinAddr))[0] |=
	#if defined(SGX_FEATURE_EDM_VERTEX_PDSADDR_FULL_RANGE)
			(((psDevInfo->psPDSCtxSwitchTermMemInfo->sDevVAddr.uiAddr >> EURASIA_TAPDSSTATE_BASEADDR_ALIGNSHIFT) 
			<< EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_BASEADDR_SHIFT) 
			& EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_BASEADDR_MASK);
	#else
			((((psDevInfo->psPDSCtxSwitchTermMemInfo->sDevVAddr.uiAddr - psDevInfo->sDevVAddrPDSExecBase.uiAddr) 
			>> EURASIA_TAPDSSTATE_BASEADDR_ALIGNSHIFT) 
			<< EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_BASEADDR_SHIFT) & EUR_CR_VDM_CONTEXT_STORE_STATE0_TAPDSSTATE_BASEADDR_MASK);
	#endif

	/* Pdump the patched buffer */
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psVDMStateUpdateBufferMemInfo, 0,
								   (EURASIA_DUMMY_VDM_CTRL_STREAM_BUFFER_SIZE),
								   (PDUMP_FLAGS_CONTINUOUS));
#endif /* FIX_HW_BRN_33657 */

	/* Setup the number of USE primary attributes and temporaries */
#if !defined(SGX_FEATURE_UNIFIED_TEMPS_AND_PAS)
	ui32USENumAttrs	= SGX_UKERNEL_NUM_PRIM_ATTRIB;
#else
	ui32USENumAttrs =  SGX_UKERNEL_NUM_PRIM_ATTRIB + SGX_UKERNEL_NUM_TEMPS;
#endif

	/* Setup EDM PDS/USE registers */
	ui32PDSExecBase = psDevInfo->psMicrokernelPDSInitPrimary1MemInfo->sDevVAddr.uiAddr;
#if !defined(SGX_FEATURE_EDM_VERTEX_PDSADDR_FULL_RANGE)
	ui32PDSExecBase -= psDevInfo->sDevVAddrPDSExecBase.uiAddr;
#endif /* defined(SGX_FEATURE_EDM_VERTEX_PDSADDR_FULL_RANGE) */
	psSrvInfo->ui32EventOtherPDSExec	= (ui32PDSExecBase >> EUR_CR_EVENT_OTHER_PDS_EXEC_ADDR_ALIGNSHIFT) << EUR_CR_EVENT_OTHER_PDS_EXEC_ADDR_SHIFT;
	psSrvInfo->ui32EventOtherPDSData	= (PDS_UKERNEL_INIT_PRIM1_DATA_SEGMENT_SIZE >> EUR_CR_EVENT_OTHER_PDS_DATA_SIZE_ALIGNSHIFT) << EUR_CR_EVENT_OTHER_PDS_DATA_SIZE_SHIFT;
	psSrvInfo->ui32EventOtherPDSInfo	= 0 |
#if defined(EUR_CR_EVENT_OTHER_PDS_INFO_DM_EVENT)
										  EUR_CR_EVENT_OTHER_PDS_INFO_DM_EVENT |
										  EUR_CR_EVENT_OTHER_PDS_INFO_PNS_MASK |
										  EUR_CR_EVENT_OTHER_PDS_INFO_USE_PIPELINE_0 |
#endif
#if !defined(SGX_FEATURE_MULTITHREADED_UKERNEL)
										  EUR_CR_EVENT_OTHER_PDS_INFO_SD_MASK |
#endif
										  (((ui32USENumAttrs * sizeof(IMG_UINT32) + ((1 << EUR_CR_EVENT_OTHER_PDS_INFO_ATTRIBUTE_SIZE_ALIGNSHIFT) - 1)) >> EUR_CR_EVENT_OTHER_PDS_INFO_ATTRIBUTE_SIZE_ALIGNSHIFT) << EUR_CR_EVENT_OTHER_PDS_INFO_ATTRIBUTE_SIZE_SHIFT);

	/*
		Note: PDS_ENABLE_SW_EVENT is ignored by the PDS - host kick events always enabled.
		However, the software sim does not reflect this behaviour.
	*/
	psSrvInfo->ui32EventPDSEnable	= 0 |
									  EUR_CR_EVENT_PDS_ENABLE_BREAKPOINT_MASK |
									  EUR_CR_EVENT_PDS_ENABLE_DPM_OUT_OF_MEMORY_ZLS_MASK |
#if defined(EUR_CR_EVENT_PDS_ENABLE_SW_EVENT_MASK)
									  EUR_CR_EVENT_PDS_ENABLE_SW_EVENT_MASK |
#endif
									  EUR_CR_EVENT_PDS_ENABLE_TIMER_MASK;

#if defined(SGX_FEATURE_MP)
	psSrvInfo->ui32MasterEventPDSEnable = EUR_CR_MASTER_EVENT_PDS_ENABLE_TA_FINISHED_MASK |
										  EUR_CR_MASTER_EVENT_PDS_ENABLE_TA_TERMINATE_MASK |
										  EUR_CR_MASTER_EVENT_PDS_ENABLE_DPM_REACHED_MEM_THRESH_MASK |
										  EUR_CR_MASTER_EVENT_PDS_ENABLE_DPM_OUT_OF_MEMORY_GBL_MASK |
										  EUR_CR_MASTER_EVENT_PDS_ENABLE_DPM_OUT_OF_MEMORY_MT_MASK |
										  EUR_CR_MASTER_EVENT_PDS_ENABLE_DPM_3D_MEM_FREE_MASK |
										  EUR_CR_MASTER_EVENT_PDS_ENABLE_PIXELBE_END_RENDER_MASK;
	#if defined(SGX_FEATURE_PTLA)
	psSrvInfo->ui32MasterEventPDSEnable |= EUR_CR_MASTER_EVENT_PDS_ENABLE_TWOD_COMPLETE_MASK;
	#endif /* SGX_FEATURE_PTLA */
#else
	psSrvInfo->ui32EventPDSEnable |= EUR_CR_EVENT_PDS_ENABLE_TA_FINISHED_MASK |
									 EUR_CR_EVENT_PDS_ENABLE_TA_TERMINATE_MASK |
									 EUR_CR_EVENT_PDS_ENABLE_DPM_REACHED_MEM_THRESH_MASK |
									 EUR_CR_EVENT_PDS_ENABLE_DPM_OUT_OF_MEMORY_GBL_MASK |
									 EUR_CR_EVENT_PDS_ENABLE_DPM_OUT_OF_MEMORY_MT_MASK |
									 EUR_CR_EVENT_PDS_ENABLE_DPM_3D_MEM_FREE_MASK |
									 EUR_CR_EVENT_PDS_ENABLE_PIXELBE_END_RENDER_MASK;
#if defined(SGX_FEATURE_2D_HARDWARE)
	psSrvInfo->ui32EventPDSEnable |= EUR_CR_EVENT_PDS_ENABLE_TWOD_COMPLETE_MASK;
#endif /* SGX_FEATURE_2D_HARDWARE */
#endif /* SGX_FEATURE_MP */

	/* Pdump the TA/3D CCB control data */
	PDUMPCOMMENT(psDevData->psConnection, "TA/3D Control Structure", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psTA3DCtlMemInfo, 0, sizeof(SGXMK_TA3D_CTL), PDUMP_FLAGS_CONTINUOUS);

	/* Pdump the Host control data */
	PDUMPCOMMENT(psDevData->psConnection, "Host Control Structure", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psHostCtlMemInfo, 0, sizeof(SGXMKIF_HOST_CTL), PDUMP_FLAGS_CONTINUOUS);

	PDUMPCOMMENT(psDevData->psConnection, "Kernel page directory address in TA/3D control", IMG_TRUE);
	PDUMPPDDEVPADDR(psDevData->psConnection, psTA3DCtlMemInfo, offsetof(SGXMK_TA3D_CTL, sKernelPDDevPAddr), psTA3DCtrl->sKernelPDDevPAddr);

	return PVRSRV_OK;
}

#if defined(SGX_FEATURE_MP)
	/* By default enable all cores available */
	#if !defined(SGX_FEATURE_MP_PLUS)
	#define SGXGETCORECOUNT(cores)												\
		SGXInitGetAppHint(SGX_FEATURE_MP_CORE_COUNT, "CoresEnabled", &cores);	\
		PVR_ASSERT(cores > 0);													\
		PVR_ASSERT(cores <= SGX_FEATURE_MP_CORE_COUNT);
	#else
	#define SGXGETCORECOUNT(coresTA, cores3D)											\
		SGXInitGetAppHint(SGX_FEATURE_MP_CORE_COUNT_TA, "TACoresEnabled", &coresTA);	\
		SGXInitGetAppHint(SGX_FEATURE_MP_CORE_COUNT_3D, "3DCoresEnabled", &cores3D);	\
		PVR_ASSERT(coresTA > 0);														\
		PVR_ASSERT(coresTA <= SGX_FEATURE_MP_CORE_COUNT_TA);							\
		PVR_ASSERT(cores3D > 0);														\
		PVR_ASSERT(cores3D <= SGX_FEATURE_MP_CORE_COUNT_3D);
	#endif
#endif /* SGX_FEATURE_MP */

/*!
*******************************************************************************

 @Function	SGXInitialisePart1

 @Description

 (client invoked) chip-reset and initialisation. Part 1 runs before the SGX
 	soft reset.

 @Input psDevInfo - device
 @Output psScript - script for queuing the register writes

 @Return   PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR SGXInitialisePart1(SGX_SCRIPT_BUILD				*psScript)
{
#if defined(SGX_FEATURE_MP)
	{
		IMG_UINT32	ui32RegVal, ui32CoresEnabled;
		
#if !defined(SGX_FEATURE_MP_PLUS)
		SGXGETCORECOUNT(ui32CoresEnabled);
		ui32RegVal = (ui32CoresEnabled - 1) << EUR_CR_MASTER_CORE_ENABLE_SHIFT;
		PVR_ASSERT((ui32RegVal & ~EUR_CR_MASTER_CORE_ENABLE_MASK) == 0);
#else
		IMG_UINT32	ui323DCoresEnabled;

		SGXGETCORECOUNT(ui32CoresEnabled, ui323DCoresEnabled);
		ui32RegVal = (ui32CoresEnabled - 1) << EUR_CR_MASTER_CORE_ENABLE_SHIFT;
		ui32RegVal |= (ui323DCoresEnabled - 1) << EUR_CR_MASTER_CORE_ENABLE_3D_SHIFT;
#endif
	
		ScriptWriteSGXReg(psScript, EUR_CR_MASTER_CORE, ui32RegVal);
		
		/* Enable the clocks to the blocks in the master area. */
#if defined(SGX_FEATURE_AUTOCLOCKGATING)
		ui32RegVal = 
#if defined(FIX_HW_BRN_32845)
					 (EUR_CR_CLKGATECTL_ON << EUR_CR_MASTER_CLKGATECTL2_BIF_CLKG_SHIFT)   |
#else
					 (EUR_CR_CLKGATECTL_AUTO << EUR_CR_MASTER_CLKGATECTL2_BIF_CLKG_SHIFT) |
#endif
					 (EUR_CR_CLKGATECTL_AUTO << EUR_CR_MASTER_CLKGATECTL2_DPM_CLKG_SHIFT) |
					 (EUR_CR_CLKGATECTL_AUTO << EUR_CR_MASTER_CLKGATECTL2_IPF_CLKG_SHIFT) |
					 (EUR_CR_CLKGATECTL_AUTO << EUR_CR_MASTER_CLKGATECTL2_VDM_CLKG_SHIFT);
		#if defined(SGX_FEATURE_PTLA)
		ui32RegVal |= (EUR_CR_CLKGATECTL_AUTO << EUR_CR_MASTER_CLKGATECTL2_PTLA_CLKG_SHIFT);
		#endif /* SGX_FEATURE_PTLA */
#else
		ui32RegVal = (EUR_CR_CLKGATECTL_ON << EUR_CR_MASTER_CLKGATECTL2_BIF_CLKG_SHIFT) |
					 (EUR_CR_CLKGATECTL_ON << EUR_CR_MASTER_CLKGATECTL2_DPM_CLKG_SHIFT) |
					 (EUR_CR_CLKGATECTL_ON << EUR_CR_MASTER_CLKGATECTL2_IPF_CLKG_SHIFT) |
					 (EUR_CR_CLKGATECTL_ON << EUR_CR_MASTER_CLKGATECTL2_VDM_CLKG_SHIFT);
		#if defined(SGX_FEATURE_PTLA)
		ui32RegVal |= (EUR_CR_CLKGATECTL_ON << EUR_CR_MASTER_CLKGATECTL2_PTLA_CLKG_SHIFT);
		#endif /* SGX_FEATURE_PTLA */	
#endif
		ScriptWriteSGXReg(psScript, EUR_CR_MASTER_CLKGATECTL2, ui32RegVal);

		/* Enable the clocks to the blocks in each core. */
		{
			IMG_UINT32 ui32Core;
			IMG_UINT32 ui32ClockGateCtl = 0;
			IMG_UINT32 ui32ClockGateShifts[] = {
				EUR_CR_MASTER_CLKGATECTL_CORE0_CLKG_SHIFT,
				EUR_CR_MASTER_CLKGATECTL_CORE1_CLKG_SHIFT,
				EUR_CR_MASTER_CLKGATECTL_CORE2_CLKG_SHIFT,
				EUR_CR_MASTER_CLKGATECTL_CORE3_CLKG_SHIFT,
#if defined(SGX_FEATURE_MP_PLUS)
				EUR_CR_MASTER_CLKGATECTL_CORE4_CLKG_SHIFT,
				EUR_CR_MASTER_CLKGATECTL_CORE5_CLKG_SHIFT,
				EUR_CR_MASTER_CLKGATECTL_CORE6_CLKG_SHIFT,
				EUR_CR_MASTER_CLKGATECTL_CORE7_CLKG_SHIFT,
#endif /* SGX_FEATURE_MP_PLUS */
			};
			
#if defined(SGX_FEATURE_MP_PLUS)
			/* TA > 3D core count not currently supported */
			ui32CoresEnabled = SGX_FEATURE_MP_CORE_COUNT_3D;
#else
			ui32CoresEnabled = SGX_FEATURE_MP_CORE_COUNT;
#endif
			for(ui32Core=0; ui32Core<ui32CoresEnabled; ui32Core++)
			{
	#if defined(SGX_FEATURE_AUTOCLOCKGATING)
				ui32ClockGateCtl |= (EUR_CR_CLKGATECTL_AUTO << ui32ClockGateShifts[ui32Core]);
	#else
				ui32ClockGateCtl |= (EUR_CR_CLKGATECTL_ON << ui32ClockGateShifts[ui32Core]);
	#endif
			}
			ScriptWriteSGXReg(psScript, EUR_CR_MASTER_CLKGATECTL, ui32ClockGateCtl);
		}
	}
#else
	/* Make sure that ukernel events cannot happen during initialisation. */
	ScriptWriteSGXReg(psScript, EUR_CR_EVENT_PDS_ENABLE, 0);
	ScriptWriteSGXReg(psScript, EUR_CR_EVENT_TIMER, 0);
#endif /* SGX_FEATURE_MP */

	if (!ScriptHalt(psScript))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXInitialisePart1: Not enough space for SGX initialisation commands.  Increase SGX_MAX_INIT_COMMANDS."));
		return PVRSRV_ERROR_INSUFFICIENT_SPACE_FOR_COMMAND;
	}

	return PVRSRV_OK;
}


/*!
*******************************************************************************

 @Function	SGXInitialisePart2

 @Description

 (client invoked) chip-reset and initialisation. Part 2 runs after the SGX soft
 	reset.

 @Input pvDeviceNode - device info. structure

 @Return   PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR SGXInitialisePart2(PVRSRV_DEV_DATA *psDevData,
									   SGX_HEAP_INFO *psHeapInfo,
									   SGX_SRVINIT_INIT_INFO *psSrvInfo,
									   SGX_CLIENT_INIT_INFO *psDevInfo,
									   SGX_SCRIPT_BUILD *psScript)
{
	IMG_UINT32			ui32CacheCtrl;
	IMG_UINT32			ui32USSECodeBase, i;
	IMG_UINT32			ui32AttribRegStart;
	IMG_UINT32			ui32USECtrl;
	IMG_UINT32			ui32RegBoundReturnZero;
#if !defined(SGX_FEATURE_UNIFIED_TEMPS_AND_PAS)
	IMG_UINT32			ui32USETmpReg;
	IMG_UINT32			ui32TmpRegGran;
	IMG_UINT32			ui32TmpRegCount;
	IMG_UINT32			ui32TmpRegInit;
#endif /* #if defined(SGX_FEATURE_UNIFIED_TEMPS_AND_PAS) */
#if defined(SGX_FEATURE_TAG_YUV_TO_RGB)
	IMG_UINT32			ui32YUVCoefficient;
#endif

	PVR_UNREFERENCED_PARAMETER(psDevData);

#if defined(SGX_FEATURE_WRITEBACK_DCU)
	/*
		Setup the DCU...
	*/
#if defined(SGX_BYPASS_DCU)
	/* Bypass DCU for all requestors */
	ScriptWriteSGXReg(psScript, EUR_CR_DCU_BYPASS, EUR_CR_DCU_BYPASS_L0P0OFF_MASK |
												   EUR_CR_DCU_BYPASS_L0P1OFF_MASK |
												   EUR_CR_DCU_BYPASS_L0P2OFF_MASK |
												   EUR_CR_DCU_BYPASS_L0P3OFF_MASK |
												   EUR_CR_DCU_BYPASS_L0P4OFF_MASK |
												   EUR_CR_DCU_BYPASS_L0P5OFF_MASK |
												   EUR_CR_DCU_BYPASS_L0P6OFF_MASK |
												   EUR_CR_DCU_BYPASS_L0P7OFF_MASK | 
												   EUR_CR_DCU_BYPASS_L1P0OFF_MASK |
												   EUR_CR_DCU_BYPASS_L1P1OFF_MASK |
												   EUR_CR_DCU_BYPASS_L2OFF_MASK);

	/* Invalidate DCU */
	ScriptWriteSGXReg(psScript, EUR_CR_DCU_ICTRL, EUR_CR_DCU_ICTRL_INVALIDATE_MASK);
#endif

	ScriptWriteSGXReg(psScript, EUR_CR_DCU_PCTRL, EUR_CR_DCU_PCTRL_EDM_MASK |
													EUR_CR_DCU_PCTRL_PDM_MASK |
													EUR_CR_DCU_PCTRL_USE_VDM_MASK |
													EUR_CR_DCU_PCTRL_PDS_VDM_MASK);
#endif



	{
		IMG_UINT32 ui32EventEnable = 0;
		IMG_UINT32 ui32EventEnable2 = 0;

#if defined(SYS_USING_INTERRUPTS)

		ui32EventEnable |= EUR_CR_EVENT_HOST_ENABLE_SW_EVENT_MASK;

#if defined(SGX_FEATURE_DATA_BREAKPOINTS)
		ui32EventEnable2 |= EUR_CR_EVENT_HOST_ENABLE2_DATA_BREAKPOINT_TRAPPED_MASK;
		ui32EventEnable2 |= EUR_CR_EVENT_HOST_ENABLE2_DATA_BREAKPOINT_UNTRAPPED_MASK;
#endif /* defined(SGX_FEATURE_DATA_BREAKPOINTS) */

#endif /* #if defined(SYS_USING_INTERRUPTS) */

		/*
		 * Enable or disable host interrupts, depending on the
		 * value of ui32EventEnable.
		*/
		ScriptWriteSGXReg(psScript, EUR_CR_EVENT_HOST_ENABLE, ui32EventEnable);
		ScriptWriteSGXReg(psScript, EUR_CR_EVENT_HOST_ENABLE2, ui32EventEnable2);
	}

	ScriptWriteSGXReg(psScript, EUR_CR_PDS_CACHE_HOST_ENABLE, 0);

	/* Initialise USE global registers */
	ScriptWriteSGXReg(psScript, EUR_CR_USE_G0, 0);

	ScriptWriteSGXReg(psScript, EUR_CR_USE_G1, 0);

	/*
		Initialise the DMS to its default state - pixel and vertex partition settings are left to sgxkick
	*/
	ScriptWriteSGXReg(psScript, EUR_CR_DMS_CTRL, psSrvInfo->ui32DefaultDMSCtrl);

#if defined(EUR_CR_ISP_DEPTHSORT)
	/*
		Initialise the ISP depth sort to its default state.
	*/
	ScriptWriteSGXReg(psScript, EUR_CR_ISP_DEPTHSORT, psSrvInfo->ui32DefaultISPDepthSort);
#endif

	#if defined(EUR_CR_PDS_VCB_PERF_CTRL)
	/*
		Initialise the VCM perf control to it's default value...
	*/
	ScriptWriteSGXReg(psScript, EUR_CR_PDS_VCB_PERF_CTRL, psSrvInfo->ui32DefaultVCBPerfCtrl);
	#endif

	#if defined(EUR_CR_DMS_AGE)
	/*
		Disable data master aging.
	*/
	ScriptWriteSGXReg(psScript, EUR_CR_DMS_AGE, 0);
	#endif

#if !defined(SGX_FEATURE_UNIFIED_TEMPS_AND_PAS)
	/* FIXME: use the generic apphint infrastructure. */
	/*
		Read an apphint for the temporary register granularity.
	*/
	ui32TmpRegGran = USE_DEFAULT_TEMP_GRAN;
	OSReadRegistryDWORDFromString(0,
									PVRSRV_REGISTRY_ROOT,
									"USETmpRegGran",
									&ui32TmpRegGran);

	/*
		Read an apphint for the number of temporary registers.
	*/
	ui32TmpRegCount = USE_DEFAULT_TEMP_REG_COUNT;
	OSReadRegistryDWORDFromString(0,
									PVRSRV_REGISTRY_ROOT,
									"USETmpRegCount",
									&ui32TmpRegCount);

	/*
		Calculate the corresponding temporary register size.
	*/
	ui32TmpRegInit = ui32TmpRegCount / (1UL << ui32TmpRegGran);

	/*
		Fix out of range values.
	*/
	if (ui32TmpRegInit > 32)
	{
		ui32TmpRegInit = 32;
	}
	if (ui32TmpRegInit < 4)
	{
		ui32TmpRegInit = 4;
	}

	/*
		Report the number of temporary registers to other components.
	*/
	psDevInfo->ui32NumUSETemporaryRegisters = ui32TmpRegInit * (1UL << ui32TmpRegGran);

	/*
		Initialise the space allocated for USE temporaries
	*/
	ui32USETmpReg	= (ui32TmpRegGran << EUR_CR_USE_TMPREG_SIZE_SHIFT) |
					  ((32 - ui32TmpRegInit) << EUR_CR_USE_TMPREG_INIT_SHIFT);
	ScriptWriteSGXReg(psScript, EUR_CR_USE_TMPREG, ui32USETmpReg);

	/*
		Calculate the space allocated for USE attributes.
	*/
	ui32AttribRegStart	= (ui32TmpRegInit * (1UL << ui32TmpRegGran)) + (1UL << EURASIA_PDS_CHUNK_SIZE_SHIFT) - 1;
	ui32AttribRegStart	>>= EURASIA_PDS_CHUNK_SIZE_SHIFT;
#else /* #if !defined(SGX_FEATURE_UNIFIED_TEMPS_AND_PAS) */

	/*
		No seperate allocation for temporary registers.
	*/
	ui32AttribRegStart	= 0;

	/*
		Global temp register limit.
	*/
	psDevInfo->ui32NumUSETemporaryRegisters = USE_DEFAULT_TEMP_REG_COUNT;
#endif /* defined(SGX_FEATURE_UNIFIED_TEMPS_AND_PAS) */

#if defined(EUR_CR_PDS_DOUT_TIMEOUT_DISABLE_MASK)
	{
		IMG_UINT32	ui32PDS;

		/*
			Initialise the space allocated for USE attributes
		*/
		ui32PDS	= (ui32AttribRegStart << EUR_CR_PDS_ATTRIBUTE_CHUNK_START_SHIFT);

		/*
			Disable PDS timeouts.
		*/
		ui32PDS |= EUR_CR_PDS_DOUT_TIMEOUT_DISABLE_MASK;

		ScriptWriteSGXReg(psScript, EUR_CR_PDS, ui32PDS);
	}
#endif /* EUR_CR_PDS_DOUT_TIMEOUT_DISABLE_MASK */


	/*
		Report the number of attribute registers to other component.
	*/
	psDevInfo->ui32NumUSEAttributeRegisters = EURASIA_USE_NUM_UNIFIED_REGISTERS - EURASIA_USSE_NUM_OUTPUT_REGISTERS - SGX_UKERNEL_NUM_SEC_ATTRIB;
	psDevInfo->ui32NumUSEAttributeRegisters -= (ui32AttribRegStart << EURASIA_PDS_CHUNK_SIZE_SHIFT);

#if defined(EUR_CR_PDS_DMS_CTRL2)	
#if defined(SGX545)
	/*
	 *  This is here as it fixes hardware recovery.  It's not quite understood yet.
		Setup USSE output partition count.
		
		Awaiting 543 definition change to match 545 before the above #if can be based on the reg define.
	*/
	ScriptWriteSGXReg(psScript, EUR_CR_PDS_DMS_CTRL2, ((EURASIA_USSE_NUM_OUTPUT_REGISTERS/EURASIA_OUTPUT_PARTITION_SIZE + SGX_DEFAULT_NUM_EXTRA_OUTPUT_PARTITIONS) << EUR_CR_PDS_DMS_CTRL2_MAX_PARTITIONS_SHIFT));
#else
	ScriptWriteSGXReg(psScript, EUR_CR_PDS_DMS_CTRL2, (SGX_DEFAULT_NUM_EXTRA_OUTPUT_PARTITIONS) << EUR_CR_PDS_DMS_CTRL2_MAX_PARTITIONS_SHIFT);
#endif
#endif
	
#if defined(SGX_FEATURE_CEM_FACE_PACKING) || defined(EUR_CR_TSP_CONFIG_DADJUST_RANGE_MASK)
	{
		IMG_UINT32 ui32TSPDefaultValue = 0;

#if defined(SGX_FEATURE_CEM_FACE_PACKING)
		ui32TSPDefaultValue |= EUR_CR_TSP_CONFIG_CEM_FACE_PACKING_MASK;
#endif

#if defined(EUR_CR_TSP_CONFIG_DADJUST_RANGE_MASK)
		ui32TSPDefaultValue |= EUR_CR_TSP_CONFIG_DADJUST_RANGE_MASK;
#endif

		ScriptWriteSGXReg(psScript, EUR_CR_TSP_CONFIG, ui32TSPDefaultValue);
	}
#endif /* defined(SGX_FEATURE_CEM_FACE_PACKING) || defined(SGX_FEATURE_8BIT_DADJUST) */

	/*
		Initialise the cache control to its default state
	*/
#if defined(EUR_CR_CACHE_CTRL)
	ui32CacheCtrl = EUR_CR_CACHE_CTRL_PARTDM0_MASK |
					EUR_CR_CACHE_CTRL_PARTDM1_MASK |
					EUR_CR_CACHE_CTRL_PARTDM2_MASK |
					EUR_CR_CACHE_CTRL_PARTDM3_MASK;

	OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "CacheCtrl", &ui32CacheCtrl);
	ScriptWriteSGXReg(psScript, EUR_CR_CACHE_CTRL, ui32CacheCtrl);
#else

	ScriptWriteSGXReg(psScript, EUR_CR_DCU_CTRL, 0);

	#if defined(EUR_CR_TCU_CTRL_MAX_REQUEST_MASK) 
		#if defined(FIX_HW_BRN_30573)
	ScriptWriteSGXReg(psScript, EUR_CR_TCU_CTRL, 0x2F << EUR_CR_TCU_CTRL_MAX_REQUEST_SHIFT);
		#else
			#if defined(SGX543) && ( \
				(SGX_CORE_REV == 111) || \
				(SGX_CORE_REV == 112) || \
				(SGX_CORE_REV == 113) || \
				(SGX_CORE_REV == 211) || \
				(SGX_CORE_REV == 2111) || \
				(SGX_CORE_REV == 216) )
	ScriptWriteSGXReg(psScript, EUR_CR_TCU_CTRL, 0x2F << EUR_CR_TCU_CTRL_MAX_REQUEST_SHIFT);
			#else		
	/* Improves TCU performance */
	ScriptWriteSGXReg(psScript, EUR_CR_TCU_CTRL, 0x60 << EUR_CR_TCU_CTRL_MAX_REQUEST_SHIFT);
			#endif /* SGX543 */
		#endif
	#else
	ScriptWriteSGXReg(psScript, EUR_CR_TCU_CTRL, 0);
	#endif


	ui32CacheCtrl = EUR_CR_DCU_PCTRL_EDM_MASK |
					EUR_CR_DCU_PCTRL_PDM_MASK |
					EUR_CR_DCU_PCTRL_USE_VDM_MASK |
					EUR_CR_DCU_PCTRL_PDS_VDM_MASK;

	ScriptWriteSGXReg(psScript, EUR_CR_DCU_PCTRL, ui32CacheCtrl);
#endif

	/*
		Initialize the USSE contol register.
	*/
	ui32USECtrl = (31UL << EUR_CR_USE_CTRL_INSTLIMIT_SHIFT) |
				  EUR_CR_USE_CTRL_REGBOUND_ZERO_RETURN_ONE;
	if (OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "RegBoundReturnZero", &ui32RegBoundReturnZero))
	{
		if (ui32RegBoundReturnZero)
		{
			ui32USECtrl |= EUR_CR_USE_CTRL_REGBOUND_ZERO_RETURN_ZERO;
		}
	}
	ScriptWriteSGXReg(psScript, EUR_CR_USE_CTRL, ui32USECtrl);

	/* Pixel Shader Heap - 10 base exe regs */
	ui32USSECodeBase = psHeapInfo->psPixelShaderHeap->sDevVAddrBase.uiAddr;
	for(i=SGX_PIXSHADER_USE_CODE_BASE_INDEX; i < SGX_VTXSHADER_USE_CODE_BASE_INDEX; i++)
	{
		IMG_UINT32 ui32USSECodeReg;

		ui32USSECodeReg = (ui32USSECodeBase >> EUR_CR_USE_CODE_BASE_ADDR_ALIGNSHIFT) |
							(EURASIA_USECODEBASE_DATAMASTER_PIXEL << EUR_CR_USE_CODE_BASE_DM_SHIFT);
		ScriptWriteSGXReg(psScript, EUR_CR_USE_CODE_BASE(i), ui32USSECodeReg);

		ui32USSECodeBase += (1UL << SGX_USE_CODE_SEGMENT_RANGE_BITS);
	}

	/* Vertex Shader Heap - 4 base exe regs */
	ui32USSECodeBase = psHeapInfo->psVertexShaderHeap->sDevVAddrBase.uiAddr;
	/* Leave a gap before the kernel base reg for future use */
	for(i=SGX_VTXSHADER_USE_CODE_BASE_INDEX; i < SGX_KERNEL_USE_CODE_BASE_INDEX - 1; i++)
	{
		IMG_UINT32 ui32USSECodeReg;

		ui32USSECodeReg = (ui32USSECodeBase >> EUR_CR_USE_CODE_BASE_ADDR_ALIGNSHIFT) |
							(EURASIA_USECODEBASE_DATAMASTER_VERTEX << EUR_CR_USE_CODE_BASE_DM_SHIFT);
		ScriptWriteSGXReg(psScript, EUR_CR_USE_CODE_BASE(i), ui32USSECodeReg);

		ui32USSECodeBase += (1UL << SGX_USE_CODE_SEGMENT_RANGE_BITS);
	}

	/* kernel heap - 1 base exe reg */
	ui32USSECodeBase = (psHeapInfo->psKernelCodeHeap->sDevVAddrBase.uiAddr >> EUR_CR_USE_CODE_BASE_ADDR_ALIGNSHIFT) |
							(EURASIA_USECODEBASE_DATAMASTER_EDM << EUR_CR_USE_CODE_BASE_DM_SHIFT);
	ScriptWriteSGXReg(psScript, EUR_CR_USE_CODE_BASE(SGX_KERNEL_USE_CODE_BASE_INDEX), ui32USSECodeBase);

#if !defined(SGX_FEATURE_PIXEL_PDSADDR_FULL_RANGE)
	/*
		Initialise the PDS exec base.
		For cores with SGX_FEATURE_EDM_VERTEX_PDSADDR_FULL_RANGE the
			PDS executable base applies to PDS pixel programs only.
		For cores with SGX_FEATURE_PIXEL_PDSADDR_FULL_RANGE it is not present
		For all other cores it applies to all PDS programs.
	*/
	ScriptWriteSGXReg(psScript, EUR_CR_PDS_EXEC_BASE, psDevInfo->sDevVAddrPDSExecBase.uiAddr);
#endif

	/*
		Initialize the MOE format control register.
	*/
	{
		IMG_UINT32	ui32UsseMoe;

		ui32UsseMoe = 0;
		#if EURASIA_USE_MOE_INITIAL_EFO_FORMAT_CONTROL
		ui32UsseMoe |= EUR_CR_USE_MOE_FSFASEL_MASK;
		#endif /* EURASIA_USE_MOE_INITIAL_EFO_FORMAT_CONTROL */
		#if EURASIA_USE_MOE_INITIAL_COLOUR_FORMAT_CONTROL
		ui32UsseMoe |= EUR_CR_USE_MOE_CFASEL_MASK;
		#endif /* EURASIA_USE_MOE_INITIAL_COLOUR_FORMAT_CONTROL */

		ScriptWriteSGXReg(psScript, EUR_CR_USE_MOE, ui32UsseMoe);
	}

	ScriptWriteSGXReg(psScript, EUR_CR_EVENT_KICKER, psDevInfo->psKernelCCBEventKickerMemInfo->sDevVAddr.uiAddr);
	
#if defined(SGX_FEATURE_MP)
	ScriptWriteSGXReg(psScript, EUR_CR_EVENT_OTHER_PDS_EXEC, psSrvInfo->ui32SlaveEventOtherPDSExec);
	ScriptWriteSGXReg(psScript, EUR_CR_EVENT_OTHER_PDS_DATA, psSrvInfo->ui32SlaveEventOtherPDSData);
	ScriptWriteSGXReg(psScript, EUR_CR_EVENT_OTHER_PDS_INFO, psSrvInfo->ui32SlaveEventOtherPDSInfo);
#endif

	ScriptWriteSGXReg(psScript, SGX_MP_CORE_SELECT(EUR_CR_EVENT_OTHER_PDS_EXEC, 0), psSrvInfo->ui32EventOtherPDSExec);
	ScriptWriteSGXReg(psScript, SGX_MP_CORE_SELECT(EUR_CR_EVENT_OTHER_PDS_DATA, 0), psSrvInfo->ui32EventOtherPDSData);
	ScriptWriteSGXReg(psScript, SGX_MP_CORE_SELECT(EUR_CR_EVENT_OTHER_PDS_INFO, 0), psSrvInfo->ui32EventOtherPDSInfo);
	ScriptWriteSGXReg(psScript, SGX_MP_CORE_SELECT(EUR_CR_EVENT_PDS_ENABLE, 0), psSrvInfo->ui32EventPDSEnable);
#if defined(SGX_FEATURE_MP)
	ScriptWriteSGXReg(psScript, EUR_CR_MASTER_EVENT_PDS_ENABLE, psSrvInfo->ui32MasterEventPDSEnable);
#endif /* SGX_FEATURE_MP */
#if !defined(FIX_HW_BRN_23258) && defined(SGX_DMS_AGE_ENABLE)
	ScriptWriteSGXReg(psScript, EUR_CR_DMS_AGE, EUR_CR_DMS_AGE_ENABLE_MASK | PVRSRV_SGX_MAX_DMS_AGE);
#endif

#if defined(SGX_FEATURE_SYSTEM_CACHE)
	psDevInfo->ui32CacheControl = (SGXMKIF_CC_INVAL_BIF_PD | SGXMKIF_CC_INVAL_BIF_SL);
#else
	psDevInfo->ui32CacheControl = SGXMKIF_CC_INVAL_BIF_PD;
#endif

#if defined(PDUMP)
#if defined(EUR_CR_PERF_COUNTER_SELECT_SHIFT)
	/* Initialise performance counter selector. */
	{
		IMG_UINT32	ui32PerfCounterSelect;

		SGXInitGetAppHint(0, "PerfCounterSelect", &ui32PerfCounterSelect);

		ui32PerfCounterSelect = (ui32PerfCounterSelect << EUR_CR_PERF_COUNTER_SELECT_SHIFT) & EUR_CR_PERF_COUNTER_SELECT_MASK;
		for(i=0; i<SGX_FEATURE_MP_CORE_COUNT_3D; i++)
		{
			ScriptPDumpSGXReg(psScript, SGX_MP_CORE_SELECT(EUR_CR_PERF, i), ui32PerfCounterSelect);
		}
	}
#endif /* EUR_CR_PERF_COUNTER_SELECT_SHIFT */

	/*
		Clear the cycle counters...
	*/
	#if defined(NO_HARDWARE) || defined(EMULATOR)
	ScriptPDumpSGXReg(psScript, EUR_CR_EMU_CYCLE_COUNT, 1 << EUR_CR_EMU_CYCLE_COUNT_RESET_SHIFT);
	ScriptPDumpSGXReg(psScript, EUR_CR_EMU_CYCLE_COUNT, 0 << EUR_CR_EMU_CYCLE_COUNT_RESET_SHIFT);
	#endif
#endif /* PDUMP */

#if defined(SGX_FEATURE_MP)
	/* set MP specifics */
	{
		IMG_UINT32 ui32MPCoreAllocationMode;
		IMG_UINT32 ui32PrimitiveSplitThreshold;
	#if defined(EUR_CR_MASTER_DPM_PIMSHARE_RESERVE_PAGES)
		IMG_UINT32 ui32DPMPimShareReservePages;

		/* default to reserve oage size of 4*/
		SGXInitGetAppHint(4, "MPPimShareReservePages", &ui32DPMPimShareReservePages);
		ScriptWriteSGXReg(psScript, EUR_CR_MASTER_DPM_PIMSHARE_RESERVE_PAGES, ui32DPMPimShareReservePages);
	#endif

		/*
			- default to load balanced (mode 0)
			- HW validation requires override to strict round robin so HW and SW can match
			- default to a split point of 1000.
		*/
		SGXInitGetAppHint(0, "MPCoreAllocationMode", &ui32MPCoreAllocationMode);
	#if defined(FIX_HW_BRN_31054)
		SGXInitGetAppHint(23, "MPSplitThreshold", &ui32PrimitiveSplitThreshold);
	#else
		SGXInitGetAppHint(1000, "MPSplitThreshold", &ui32PrimitiveSplitThreshold);
	#endif
		/* check for out of range values */
		PVR_ASSERT((ui32MPCoreAllocationMode & ~EUR_CR_MASTER_MP_TILE_MODE_MASK) == 0);

	#if defined(FIX_HW_BRN_31109)
		ScriptWriteSGXReg(psScript,
						  EUR_CR_MASTER_MP_PRIMITIVE,
						  (ui32PrimitiveSplitThreshold << EUR_CR_MASTER_MP_PRIMITIVE_SPLIT_THRESHOLD_SHIFT)
							| EUR_CR_MASTER_MP_PRIMITIVE_MODE_MASK 
							| EUR_CR_MASTER_MP_PRIMITIVE_SPLIT_MODE_MASK
							| (2 << EUR_CR_MASTER_MP_PRIMITIVE_MAX_BLOCKS_SHIFT));
	#else
		ScriptWriteSGXReg(psScript,
						  EUR_CR_MASTER_MP_PRIMITIVE,
						  (ui32PrimitiveSplitThreshold << EUR_CR_MASTER_MP_PRIMITIVE_SPLIT_THRESHOLD_SHIFT)
						| (ui32MPCoreAllocationMode ? EUR_CR_MASTER_MP_PRIMITIVE_MODE_MASK : 0));
	#endif
		ScriptWriteSGXReg(psScript,
						  EUR_CR_MASTER_MP_TILE,
						  (ui32MPCoreAllocationMode & EUR_CR_MASTER_MP_TILE_MODE_MASK)
						  << EUR_CR_MASTER_MP_TILE_MODE_SHIFT);
	}
#endif

#if defined(SGX_FEATURE_MK_SUPERVISOR)
	{
		IMG_UINT32 ui32Tmp;
		
		#if defined(SGX_FEATURE_MP)
		/* calc the USSE code base relative byte address */
		ui32Tmp = psDevInfo->psMicrokernelSlaveUSEMemInfo->sDevVAddr.uiAddr 
				+ SLAVE_UKERNEL_SLAVESPRVMAIN_OFFSET
	 			- psHeapInfo->psKernelCodeHeap->sDevVAddrBase.uiAddr;
		/* convert to instruction address */
		ui32Tmp /= EURASIA_USE_INSTRUCTION_SIZE;
		
		for(i=1; i<SGX_FEATURE_MP_CORE_COUNT_3D; i++)
		{
			/* setup Supervisor vector register */
			ScriptWriteSGXReg(psScript,	SGX_MP_CORE_SELECT(EUR_CR_USE_SPRV, i), ui32Tmp);
		}
		#endif
		
		/* calc the USSE code base relative byte address */
		ui32Tmp = psDevInfo->psMicrokernelUSEMemInfo->sDevVAddr.uiAddr 
				+ GetuKernelProgramSUPERVISOROffset()
	 			- psHeapInfo->psKernelCodeHeap->sDevVAddrBase.uiAddr;
		/* convert to instruction address */
		ui32Tmp /= EURASIA_USE_INSTRUCTION_SIZE;
		/* setup Supervisor vector register */
		ScriptWriteSGXReg(psScript,	SGX_MP_CORE_SELECT(EUR_CR_USE_SPRV, 0), ui32Tmp);
		/* and enable security */
		ScriptWriteSGXReg(psScript,	EUR_CR_USE_SECURITY, 0);
	}
#endif

#if defined(SGX_FEATURE_TAG_YUV_TO_RGB)
	if(OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "YUVCoef00", &ui32YUVCoefficient))
		ScriptWriteSGXReg(psScript, EUR_CR_TPU_YUV00, ui32YUVCoefficient);

	if(OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "YUVCoef01", &ui32YUVCoefficient))
		ScriptWriteSGXReg(psScript, EUR_CR_TPU_YUV01, ui32YUVCoefficient);

	if(OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "YUVCoef02", &ui32YUVCoefficient))
		ScriptWriteSGXReg(psScript, EUR_CR_TPU_YUV02, ui32YUVCoefficient);

	if(OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "YUVCoef03", &ui32YUVCoefficient))
		ScriptWriteSGXReg(psScript, EUR_CR_TPU_YUV03, ui32YUVCoefficient);

	if(OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "YUVCoef04", &ui32YUVCoefficient))
		ScriptWriteSGXReg(psScript, EUR_CR_TPU_YUV04, ui32YUVCoefficient);

#if defined(SGX_FEATURE_TAG_YUV_TO_RGB_HIPREC)
	if(OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "YUVCoef05", &ui32YUVCoefficient))
		ScriptWriteSGXReg(psScript, EUR_CR_TPU_YUV05, ui32YUVCoefficient);
#endif /* defined(SGX_FEATURE_TAG_YUV_TO_RGB_HIPREC) */

	if(OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "YUVCoef10", &ui32YUVCoefficient))
		ScriptWriteSGXReg(psScript, EUR_CR_TPU_YUV10, ui32YUVCoefficient);

	if(OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "YUVCoef11", &ui32YUVCoefficient))
		ScriptWriteSGXReg(psScript, EUR_CR_TPU_YUV11, ui32YUVCoefficient);

	if(OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "YUVCoef12", &ui32YUVCoefficient))
		ScriptWriteSGXReg(psScript, EUR_CR_TPU_YUV12, ui32YUVCoefficient);

	if(OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "YUVCoef13", &ui32YUVCoefficient))
		ScriptWriteSGXReg(psScript, EUR_CR_TPU_YUV13, ui32YUVCoefficient);

	if(OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "YUVCoef14", &ui32YUVCoefficient))
		ScriptWriteSGXReg(psScript, EUR_CR_TPU_YUV14, ui32YUVCoefficient);

#if defined(SGX_FEATURE_TAG_YUV_TO_RGB_HIPREC)
	if(OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "YUVCoef15", &ui32YUVCoefficient))
		ScriptWriteSGXReg(psScript, EUR_CR_TPU_YUV15, ui32YUVCoefficient);
#endif /* defined(SGX_FEATURE_TAG_YUV_TO_RGB_HIPREC) */

#endif /* defined(SGX_FEATURE_TAG_YUV_TO_RGB) */

	if (!ScriptHalt(psScript))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXInit: Not enough space for SGX initialisation commands.  Increase SGX_MAX_INIT_COMMANDS."));
		return PVRSRV_ERROR_INSUFFICIENT_SPACE_FOR_COMMAND;
	}

	return PVRSRV_OK;
}

/*!
*******************************************************************************

 @Function	SGXDeinitialise

 @Description

 (client invoked) chip-reset and deinitialisation

 @Input hDevCookie - device info. handle

 @Return   PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR SGXDeinitialise(PVRSRV_DEV_DATA *psDevData, SGX_SCRIPT_BUILD *psScript)

{
	PVR_UNREFERENCED_PARAMETER(psDevData);

	/* Write to the EDM configuration registers */
	ScriptWriteSGXReg(psScript, EUR_CR_EVENT_PDS_ENABLE, 0);
	ScriptWriteSGXReg(psScript, EUR_CR_EVENT_OTHER_PDS_EXEC, 0);
	ScriptWriteSGXReg(psScript, EUR_CR_EVENT_OTHER_PDS_DATA, 0);
	ScriptWriteSGXReg(psScript, EUR_CR_EVENT_OTHER_PDS_INFO, 0);
	ScriptWriteSGXReg(psScript, EUR_CR_USE_CODE_BASE(SGX_KERNEL_USE_CODE_BASE_INDEX), 0);

	/* Disable interrupts */
	ScriptWriteSGXReg(psScript, EUR_CR_EVENT_HOST_ENABLE, 0);
	ScriptWriteSGXReg(psScript, EUR_CR_EVENT_HOST_ENABLE2, 0);

	/* Disable clocks */
	ScriptWriteSGXReg(psScript, EUR_CR_CLKGATECTL, 0);
#if defined(EUR_CR_CLKGATECTL2)
	ScriptWriteSGXReg(psScript, EUR_CR_CLKGATECTL2, 0);
#endif

	if (!ScriptHalt(psScript))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXInit: Not enough space for SGX Deinitialisation commands.  Increase SGX_MAX_DEINIT_COMMANDS."));
		return PVRSRV_ERROR_INSUFFICIENT_SPACE_FOR_COMMAND;
	}

	return PVRSRV_OK;
}

/*!
*******************************************************************************

 @Function	SGXMiscInit

 @Description

 Miscellaneous device initialisation

 @Input pvDeviceNode - device info. structure

 @Return  PVRSRV_ERROR

******************************************************************************/
static IMG_VOID SGXMiscInit(SGX_HEAP_INFO *psHeapInfo,
					SGX_SRVINIT_INIT_INFO *psSrvInfo,
					SGX_CLIENT_INIT_INFO *psDevInfo)
{
	IMG_UINT32			ui32MaxEDMTasks;
	IMG_UINT32			ui32MaxVertexTasks;
	IMG_UINT32			ui32MaxPixelTasks;
	IMG_UINT32			ui32ClkGateCtl = 0;
#if defined(EUR_CR_CLKGATECTL2)
	IMG_UINT32			ui32ClkGateCtl2 = 0;
#endif

	/*
		At least 33 EDM tasks is needed to fix BRN23632.
	*/
#if defined(FIX_HW_BRN_23632)
	ui32MaxEDMTasks = 63;
#else
	ui32MaxEDMTasks = 32;
#endif
	ui32MaxVertexTasks = 32;
	ui32MaxPixelTasks = 32;

	/* Check for an apphint indicating the max number of EDM tasks */
	OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "MaxEDMTasks", &ui32MaxEDMTasks);

	/*
	 * 	Store the client-side build flags for verification in KM
	 */
	psDevInfo->ui32ClientBuildOptions = (SGX_BUILD_OPTIONS);

	/*
	 * 	Store the client-side structure sizes
	 */
#if defined (SGX_FEATURE_2D_HARDWARE)
	psDevInfo->sSGXStructSizes.ui32Sizeof_2DCMD = sizeof(SGXMKIF_2DCMD);
	psDevInfo->sSGXStructSizes.ui32Sizeof_2DCMD_SHARED = sizeof(SGXMKIF_2DCMD_SHARED);
#endif
	psDevInfo->sSGXStructSizes.ui32Sizeof_CMDTA = sizeof(SGXMKIF_CMDTA);
	psDevInfo->sSGXStructSizes.ui32Sizeof_CMDTA_SHARED = sizeof(SGXMKIF_CMDTA_SHARED);
	psDevInfo->sSGXStructSizes.ui32Sizeof_TRANSFERCMD = sizeof(SGXMKIF_TRANSFERCMD);
	psDevInfo->sSGXStructSizes.ui32Sizeof_TRANSFERCMD_SHARED = sizeof(SGXMKIF_TRANSFERCMD_SHARED);

	psDevInfo->sSGXStructSizes.ui32Sizeof_3DREGISTERS = sizeof(SGXMKIF_3DREGISTERS);
	psDevInfo->sSGXStructSizes.ui32Sizeof_HWPBDESC = sizeof(SGXMKIF_HWPBDESC);
	psDevInfo->sSGXStructSizes.ui32Sizeof_HWRENDERCONTEXT = sizeof(SGXMKIF_HWRENDERCONTEXT);
	psDevInfo->sSGXStructSizes.ui32Sizeof_HWRENDERDETAILS = sizeof(SGXMKIF_HWRENDERDETAILS);
	psDevInfo->sSGXStructSizes.ui32Sizeof_HWRTDATA = sizeof(SGXMKIF_HWRTDATA);
	psDevInfo->sSGXStructSizes.ui32Sizeof_HWRTDATASET = sizeof(SGXMKIF_HWRTDATASET);
	psDevInfo->sSGXStructSizes.ui32Sizeof_HWTRANSFERCONTEXT = sizeof(SGXMKIF_HWTRANSFERCONTEXT);

	psDevInfo->sSGXStructSizes.ui32Sizeof_HOST_CTL = sizeof(SGXMKIF_HOST_CTL);
	psDevInfo->sSGXStructSizes.ui32Sizeof_COMMAND = sizeof(SGXMKIF_COMMAND);

	/*
		Initialise the default DMS ctrl value.
	*/
	psSrvInfo->ui32DefaultDMSCtrl = (ui32MaxEDMTasks << EUR_CR_DMS_CTRL_MAX_NUM_EDM_TASKS_SHIFT) |
									(ui32MaxVertexTasks << EUR_CR_DMS_CTRL_MAX_NUM_VERTEX_TASKS_SHIFT) |
									(ui32MaxPixelTasks << EUR_CR_DMS_CTRL_MAX_NUM_PIXEL_TASKS_SHIFT) |
									(0UL  << EUR_CR_DMS_CTRL_DISABLE_DM_SHIFT);

	/*
	  And, when using the transfer queue, do the same for the USE exec base.
	  Don't know why the heap of the PDS Exec base requires changed here
	 */
#if defined(SGX_FEATURE_PIXEL_PDSADDR_FULL_RANGE)
	psDevInfo->sDevVAddrPDSExecBase.uiAddr = 0;
#else
	psDevInfo->sDevVAddrPDSExecBase = psHeapInfo->psPDSPixelCodeDataHeap->sDevVAddrBase;
#endif

	psDevInfo->sDevVAddrUSEExecBase = psHeapInfo->psPixelShaderHeap->sDevVAddrBase;

#if !defined(SGX_FEATURE_PIXEL_PDSADDR_FULL_RANGE)
	PVR_ASSERT((psDevInfo->sDevVAddrPDSExecBase.uiAddr & ~EUR_CR_PDS_EXEC_BASE_ADDR_MASK) == 0);
#endif

	#if defined(EUR_CR_PDS_VCB_PERF_CTRL)
	/*
		VCB perf control setup...
	*/
	psSrvInfo->ui32DefaultVCBPerfCtrl = (EURASIA_PDS_RESERVED_PIXEL_CHUNKS << EUR_CR_PDS_VCB_PERF_CTRL_REDUCE_TA_MEM_SHIFT);
	#endif

	/*
		Secondary Alpha Iteration Feature Setup...
	*/
#if defined(SGX_FEATURE_ALPHATEST_SECONDARY) && !defined(SGX_FEATURE_ALPHATEST_SECONDARY_PERPRIMITIVE)
	{
		IMG_UINT32 ui32DisableSecondaryAlpha = 0;
		OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "DisableSecondaryAlpha", &ui32DisableSecondaryAlpha);

		if(ui32DisableSecondaryAlpha == 0)
		{
			#if defined(EUR_CR_DMS_CTRL_ENABLE_SECONDARY_FB_ABC_SHIFT)
			psSrvInfo->ui32DefaultDMSCtrl |= (1UL << EUR_CR_DMS_CTRL_ENABLE_SECONDARY_FB_ABC_SHIFT);
			#endif
		}
	}
#endif

	/*
		Alpha PCOEFF Coefficient Re-ordering...
	*/
#if defined(SGX_FEATURE_ALPHATEST_COEFREORDER) && defined(EUR_CR_ISP_DEPTHSORT_FB_ABC_ORDER_MASK)
	{
		IMG_UINT32 ui32DisableAlphaCoefficientReOrdering = 0;
		OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "DisableAlphaCoefficientReOrdering", &ui32DisableAlphaCoefficientReOrdering);

		if(ui32DisableAlphaCoefficientReOrdering == 0)
		{
			psSrvInfo->ui32DefaultISPDepthSort = EUR_CR_ISP_DEPTHSORT_FB_ABC_ORDER_MASK;
		}
		else
		{
			psSrvInfo->ui32DefaultISPDepthSort = 0;
		}
	}
#else
	psSrvInfo->ui32DefaultISPDepthSort = 0;
#endif

	/*
	 * EVM contexts will be switched on a render so initialise
	 * so 3D ID will switch to TA ID just used...
	*/
	psDevInfo->ui32EVMConfig = EUR_CR_DPM_STATE_CONTEXT_ID_DALLOC_MASK;

	/* SGX registers used for lockup detection in srvkm */
#if defined(EUR_CR_USE_SERV_EVENT)
	psDevInfo->ui32EDMTaskReg0 = EUR_CR_USE_SERV_EVENT;
	psDevInfo->ui32EDMTaskReg1 = 0;
#else
	psDevInfo->ui32EDMTaskReg0 = EUR_CR_USE0_SERV_EVENT;
	psDevInfo->ui32EDMTaskReg1 = EUR_CR_USE1_SERV_EVENT;
#endif


	/*
		Initialise clock gating register default values.
	*/
	{
#if defined(SGX_FEATURE_AUTOCLOCKGATING)
		IMG_UINT32 ui32DisableClockGating;

		ui32DisableClockGating = 0;
		OSReadRegistryDWORDFromString(0, PVRSRV_REGISTRY_ROOT, "DisableClockGating", &ui32DisableClockGating);
		if (!ui32DisableClockGating)
		{
			#if defined(EUR_CR_CLKGATECTL_ISP_CLKG_AUTO)
			#if defined(FIX_HW_BRN_22648)
			ui32ClkGateCtl	|= EUR_CR_CLKGATECTL_ISP_CLKG_ON;
			#else
			ui32ClkGateCtl	|= EUR_CR_CLKGATECTL_ISP_CLKG_AUTO;
			#endif
			#endif

			#if defined(EUR_CR_CLKGATECTL_ISP2_CLKG_AUTO)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_ISP2_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL_TSP_CLKG_AUTO)
			#if defined(FIX_HW_BRN_23765) || defined(FIX_HW_BRN_22610)
			ui32ClkGateCtl	|= EUR_CR_CLKGATECTL_TSP_CLKG_ON;
			#else
			ui32ClkGateCtl	|= EUR_CR_CLKGATECTL_TSP_CLKG_AUTO;
			#endif
			#endif

			#if defined(EUR_CR_CLKGATECTL_TE_CLKG_AUTO)
			#if defined(FIX_HW_BRN_26202) || defined(FIX_HW_BRN_28474)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_TE_CLKG_ON;
			#else
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_TE_CLKG_AUTO;
			#endif
			#endif

			#if defined(EUR_CR_CLKGATECTL_MTE_CLKG_AUTO)
			#if defined(FIX_HW_BRN_28474)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_MTE_CLKG_ON;
			#else
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_MTE_CLKG_AUTO;
			#endif			
			#endif

			#if defined(EUR_CR_CLKGATECTL_DPM_CLKG_AUTO)
			#if defined(FIX_HW_BRN_22563) || defined(FIX_HW_BRN_31543)
			ui32ClkGateCtl	|= EUR_CR_CLKGATECTL_DPM_CLKG_ON;
			#else
			ui32ClkGateCtl	|= EUR_CR_CLKGATECTL_DPM_CLKG_AUTO;
			#endif
			#endif

			#if defined(EUR_CR_CLKGATECTL_VDM_CLKG_AUTO)
			#if defined(FIX_HW_BRN_28474)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_VDM_CLKG_ON;
			#else
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_VDM_CLKG_AUTO;
			#endif
			#endif

			#if defined(EUR_CR_CLKGATECTL_PDS0_CLKG_AUTO)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_PDS0_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL_PDS_CLKG_AUTO)
			#if defined(FIX_HW_BRN_30852)
			/* Force PDS clocks always on */
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_PDS_CLKG_ON;
			#else
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_PDS_CLKG_AUTO;
			#endif /* FIX BRN30852 */
			#endif

			#if defined(EUR_CR_CLKGATECTL_IDXFIFO_CLKG_AUTO)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_IDXFIFO_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL_TA_CLKG_AUTO)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_TA_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL_SYSTEM_CLKG_AUTO)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_SYSTEM_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL_USE_CLKG_AUTO)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_USE_CLKG_AUTO;
			#endif

			#if defined(SGX_FEATURE_2D_HARDWARE)
			#if defined(EUR_CR_CLKGATECTL_2D_CLKG_AUTO)
			ui32ClkGateCtl	|= EUR_CR_CLKGATECTL_2D_CLKG_AUTO;
			#endif
			#endif

			#if defined(EUR_CR_CLKGATECTL_BIF_CORE_CLKG_AUTO)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_BIF_CORE_CLKG_AUTO;
			#endif

#if defined(EUR_CR_CLKGATECTL2)
			#if defined(EUR_CR_CLKGATECTL2_PBE_CLKG_AUTO)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_PBE_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_CACHEL2_CLKG_AUTO)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_CACHEL2_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_TCU_L2_CLKG_AUTO)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_TCU_L2_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_UCACHEL2_CLKG_AUTO)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_UCACHEL2_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_USE1_CLKG_AUTO)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_USE1_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_ITR1_CLKG_AUTO)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_ITR1_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_TEX1_CLKG_AUTO)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_TEX1_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_MADD1_CLKG_AUTO)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_MADD1_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_PDS1_CLKG_AUTO)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_PDS1_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_USE0_CLKG_AUTO)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_USE0_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_ITR0_CLKG_AUTO)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_ITR0_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_TEX0_CLKG_AUTO)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_TEX0_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_MADD0_CLKG_AUTO)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_MADD0_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_DCU_L2_CLKG_AUTO)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_DCU_L2_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_DCU1_L0L1_CLKG_AUTO)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_DCU1_L0L1_CLKG_AUTO;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_DCU0_L0L1_CLKG_AUTO)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_DCU0_L0L1_CLKG_AUTO;
			#endif

#endif
		}
		else
#endif /* defined(SGX_FEATURE_AUTOCLOCKGATING) */
		{
			#if defined(EUR_CR_CLKGATECTL_ISP_CLKG_ON)
			ui32ClkGateCtl	|= EUR_CR_CLKGATECTL_ISP_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL_ISP2_CLKG_ON)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_ISP2_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL_TSP_CLKG_ON)
			ui32ClkGateCtl	|= EUR_CR_CLKGATECTL_TSP_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL_TE_CLKG_ON)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_TE_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL_MTE_CLKG_ON)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_MTE_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL_DPM_CLKG_ON)
			ui32ClkGateCtl	|= EUR_CR_CLKGATECTL_DPM_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL_VDM_CLKG_ON)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_VDM_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL_PDS0_CLKG_ON)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_PDS0_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL_PDS_CLKG_ON)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_PDS_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL_IDXFIFO_CLKG_ON)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_IDXFIFO_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL_TA_CLKG_ON)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_TA_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL_SYSTEM_CLKG_ON)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_SYSTEM_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL_USE_CLKG_ON)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_USE_CLKG_ON;
			#endif

			#if defined(SGX_FEATURE_2D_HARDWARE)
			#if defined(EUR_CR_CLKGATECTL_2D_CLKG_ON)
			ui32ClkGateCtl	|= EUR_CR_CLKGATECTL_2D_CLKG_ON;
			#endif
			#endif

			#if defined(EUR_CR_CLKGATECTL_BIF_CORE_CLKG_ON)
			ui32ClkGateCtl |= EUR_CR_CLKGATECTL_BIF_CORE_CLKG_ON;
			#endif

#if defined(EUR_CR_CLKGATECTL2)
			#if defined(EUR_CR_CLKGATECTL2_PBE_CLKG_ON)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_PBE_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_CACHEL2_CLKG_ON)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_CACHEL2_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_TCU_L2_CLKG_ON)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_TCU_L2_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_UCACHEL2_CLKG_ON)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_UCACHEL2_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_USE1_CLKG_ON)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_USE1_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_ITR1_CLKG_ON)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_ITR1_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_TEX1_CLKG_ON)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_TEX1_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_MADD1_CLKG_ON)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_MADD1_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_PDS1_CLKG_ON)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_PDS1_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_USE0_CLKG_ON)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_USE0_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_ITR0_CLKG_ON)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_ITR0_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_TEX0_CLKG_ON)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_TEX0_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_MADD0_CLKG_ON)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_MADD0_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_DCU_L2_CLKG_ON)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_DCU_L2_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_DCU1_L0L1_CLKG_ON)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_DCU1_L0L1_CLKG_ON;
			#endif

			#if defined(EUR_CR_CLKGATECTL2_DCU0_L0L1_CLKG_ON)
			ui32ClkGateCtl2 |= EUR_CR_CLKGATECTL2_DCU0_L0L1_CLKG_ON;
			#endif
#endif
		}
	}

	psDevInfo->ui32ClkGateCtl = ui32ClkGateCtl;
#if defined(EUR_CR_CLKGATECTL2)
	psDevInfo->ui32ClkGateCtl2 = ui32ClkGateCtl2;
#endif

#if defined(SGX_FEATURE_AUTOCLOCKGATING)
	{
		IMG_UINT32			ui32ClockMask;
		/*
			When polling for clock gating in srvkm, only the bits which are
			set in ui32ClockMask will be checked.
		*/
		ui32ClockMask = 0xFFFFFFFF;

#if defined(FIX_HW_BRN_30852)
		ui32ClockMask &= ~EUR_CR_CLKGATESTATUS_PDS_CLKS_MASK;
#endif

#if defined(FIX_HW_BRN_23765) || defined(FIX_HW_BRN_22610)
		ui32ClockMask &= ~EUR_CR_CLKGATESTATUS_TSP_CLKS_MASK;
#endif

#if defined(FIX_HW_BRN_28474) || defined(FIX_HW_BRN_26202)
		ui32ClockMask &= ~EUR_CR_CLKGATESTATUS_TE_CLKS_MASK;
#endif

#if defined(FIX_HW_BRN_28474)
		ui32ClockMask &= ~EUR_CR_CLKGATESTATUS_MTE_CLKS_MASK;
		ui32ClockMask &= ~EUR_CR_CLKGATESTATUS_VDM_CLKS_MASK;
#endif

#if defined(FIX_HW_BRN_26202)
		ui32ClockMask &= ~EUR_CR_CLKGATESTATUS_TA_CLKS_MASK;
#endif

#if defined(FIX_HW_BRN_22563) || defined(FIX_HW_BRN_28474) || defined(FIX_HW_BRN_31543) || defined(FIX_HW_BRN_26202)
		ui32ClockMask &= ~EUR_CR_CLKGATESTATUS_DPM_CLKS_MASK;
#endif

#if defined(FIX_HW_BRN_22648)
		ui32ClockMask &= ~EUR_CR_CLKGATESTATUS_ISP_CLKS_MASK;
#endif

		psDevInfo->ui32ClkGateStatusMask = ui32ClockMask;
	}
#else
		psDevInfo->ui32ClkGateStatusMask = 0;
#endif	/* #if defined(SGX_FEATURE_AUTOCLOCKGATING) */

	psDevInfo->ui32ClkGateStatusReg = EUR_CR_CLKGATESTATUS;

#if defined(SGX_FEATURE_MP)
	psDevInfo->ui32MasterClkGateStatusReg = EUR_CR_MASTER_CLKGATESTATUS;
	/* EUR_CR_MASTER_CLKGATESTATUS_CORE*_CLKS_MASK */
	psDevInfo->ui32MasterClkGateStatusMask = 0;
#if !defined(FIX_HW_BRN_30852) && !defined(FIX_HW_BRN_31543)
	{
		IMG_UINT32	ui32CoresEnabled;
		
	#if !defined(SGX_FEATURE_MP_PLUS)
		SGXGETCORECOUNT(ui32CoresEnabled);
		psDevInfo->ui32MasterClkGateStatusMask |= (1 << ui32CoresEnabled) - 1;
	#else
		IMG_UINT32	ui323DCoresEnabled;

		SGXGETCORECOUNT(ui32CoresEnabled, ui323DCoresEnabled);
		psDevInfo->ui32MasterClkGateStatusMask |= (1 << ui323DCoresEnabled) - 1;
	#endif	
	}
#endif

	psDevInfo->ui32MasterClkGateStatus2Reg = EUR_CR_MASTER_CLKGATESTATUS2;
	psDevInfo->ui32MasterClkGateStatus2Mask = EUR_CR_MASTER_CLKGATESTATUS2_DPM_CLKS_MASK |
											  EUR_CR_MASTER_CLKGATESTATUS2_IPF_CLKS_MASK |
											  EUR_CR_MASTER_CLKGATESTATUS2_VDM_CLKS_MASK;
	
#if !defined(FIX_HW_BRN_32845)	
	psDevInfo->ui32MasterClkGateStatus2Mask |= EUR_CR_MASTER_CLKGATESTATUS2_BIF_CLKS_MASK;
#endif
	        
#if defined(SGX_FEATURE_PTLA)
	psDevInfo->ui32MasterClkGateStatus2Mask |= EUR_CR_MASTER_CLKGATESTATUS2_PTLA_CLKS_MASK;
#endif /* SGX_FEATURE_PTLA */
#endif /* SGX_FEATURE_MP */
}


#if defined(FIX_HW_BRN_23861)
static PVRSRV_ERROR
AllocateDumyStencilBuffers(PVRSRV_DEV_DATA *psDevData,
						   SGX_HEAP_INFO *psHeapInfo,
						   SGX_CLIENT_INIT_INFO *psDevInfo)
{
	SGXMK_TA3D_CTL	*psTA3DCtl =
		(SGXMK_TA3D_CTL *)psDevInfo->psTA3DCtlMemInfo->pvLinAddr;
	PVRSRV_ERROR eError;

	/*
	   first render context so create a dummy Z/stencil
	   - 2k x 2k 4x MSAA = 16M pixels @ 4bytes = 64Mb Depth
	   - 2k x 2k 4x MSAA = 16M pixels @ 1bytes = 16Mb Stencil
	   Z pixel offset = (Y x Xstride x 256) + (X x 16)
	   - this formula suggests one row of tiles is enough
	   for Z but it's really 2 rows because the advance in
	   memory of the previous rows exceeds the starting address
	   of the next row.
	   So for 2k x 2k:
	   Zvalues = 128 tiles x 2 rows x 256 Values per tile
	   for MSAA 4x:
	   ZValues = ZValues * 4(MSAA) * 4bpp = 0x100000 bytes
	   SValues = 0x40000 bytes
		Note: to avoid the bug we only need to do Z or Stencil LS
		therefore we'll only do stencil
	*/
	PDUMPCOMMENT(psDevData->psConnection,"Allocate dummy Z/Stencil load buffer", IMG_TRUE);
	if(PVRSRVAllocDeviceMem(psDevData,
							psHeapInfo->psKernelDataHeap->hDevMemHeap,
							PVRSRV_MEM_DUMMY | PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ
							| PVRSRV_MAP_NOUSERVIRTUAL,
							0x40000,
							SGX_MMU_PAGE_SIZE,
							&psDevInfo->psDummyStencilLoadMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"Failed to allocate a Dummy Stencil Load Buffer!"));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorExit;
	}

	PDUMPCOMMENT(psDevData->psConnection,"Allocate dummy Z/Stencil store buffer", IMG_TRUE);
	if(PVRSRVAllocDeviceMem(psDevData,
							psHeapInfo->psKernelDataHeap->hDevMemHeap,
							PVRSRV_MEM_DUMMY | PVRSRV_MEM_READ | PVRSRV_MEM_WRITE | PVRSRV_MEM_NO_SYNCOBJ
							| PVRSRV_MAP_NOUSERVIRTUAL,
							0x40000,
							SGX_MMU_PAGE_SIZE,
							&psDevInfo->psDummyStencilStoreMemInfo) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"Failed to allocate a Dummy Stencil Store Buffer!"));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorExit;
	}

	/* setup the microkernel structure accordingly */

	/* ZLS requestor base */
	psTA3DCtl->sDummyZLSReqBaseDevVAddr.uiAddr =
		psHeapInfo->psKernelDataHeap->sDevVAddrBase.uiAddr;

	/* and the Depth/stencil load and store */
	psTA3DCtl->sDummyStencilLoadDevVAddr.uiAddr =
		(psDevInfo->psDummyStencilLoadMemInfo->sDevVAddr.uiAddr
		 - psHeapInfo->psKernelDataHeap->sDevVAddrBase.uiAddr)
		| EUR_CR_ISP_STENCIL_LOAD_BASE_ENABLE_MASK;
	psTA3DCtl->sDummyStencilStoreDevVAddr.uiAddr =
		(psDevInfo->psDummyStencilStoreMemInfo->sDevVAddr.uiAddr
		 - psHeapInfo->psKernelDataHeap->sDevVAddrBase.uiAddr)
		| EUR_CR_ISP_STENCIL_STORE_BASE_ENABLE_MASK;

	PDUMPCOMMENT(psDevData->psConnection,"Dummy ZLS and stencil load/store buffers", IMG_TRUE);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psTA3DCtlMemInfo, offsetof(SGXMK_TA3D_CTL, sDummyZLSReqBaseDevVAddr), sizeof(IMG_DEV_VIRTADDR), PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psTA3DCtlMemInfo, offsetof(SGXMK_TA3D_CTL, sDummyStencilLoadDevVAddr), sizeof(IMG_DEV_VIRTADDR), PDUMP_FLAGS_CONTINUOUS);
	PDUMPMEM(psDevData->psConnection, IMG_NULL, psDevInfo->psTA3DCtlMemInfo, offsetof(SGXMK_TA3D_CTL, sDummyStencilStoreDevVAddr), sizeof(IMG_DEV_VIRTADDR), PDUMP_FLAGS_CONTINUOUS);

	return PVRSRV_OK;

ErrorExit:
	if(psDevInfo->psDummyStencilStoreMemInfo)
	{
		PVRSRVFreeDeviceMem(psDevData,
							psDevInfo->psDummyStencilStoreMemInfo);
	}
	if(psDevInfo->psDummyStencilLoadMemInfo)
	{
		PVRSRVFreeDeviceMem(psDevData,
							psDevInfo->psDummyStencilLoadMemInfo);
	}
	return eError;
}
#endif /* defined(FIX_HW_BRN_23861) */


/*!
*******************************************************************************

 @Function	SGXInit

 @Description

 SGX Initialisation

 @Input psServices

 @Return   PVRSRV_ERROR

******************************************************************************/
IMG_INTERNAL
PVRSRV_ERROR SGXInit(PVRSRV_CONNECTION *psServices, PVRSRV_DEVICE_IDENTIFIER *psDevID)
{
	PVRSRV_ERROR eError;
	PVRSRV_DEV_DATA sDevData;
	SGX_SRVINIT_INIT_INFO sSrvInfo;
	SGX_CLIENT_INIT_INFO *psClientInfo = IMG_NULL;
	SGX_BRIDGE_INFO_FOR_SRVINIT * psInitInfo = IMG_NULL;
	SGX_HEAP_INFO sHeapInfo;
	IMG_UINT32 i;
	SGX_SCRIPT_BUILD sInitScriptPart1 = {SGX_MAX_INIT_COMMANDS, 0, IMG_FALSE, IMG_NULL};
	SGX_SCRIPT_BUILD sInitScriptPart2 = {SGX_MAX_INIT_COMMANDS, 0, IMG_FALSE, IMG_NULL};
	SGX_SCRIPT_BUILD sDeinitScript  = {SGX_MAX_DEINIT_COMMANDS, 0, IMG_FALSE, IMG_NULL};

	PVR_ASSERT(psDevID->eDeviceClass == PVRSRV_DEVICE_CLASS_3D);
	PVR_ASSERT(psDevID->eDeviceType == PVRSRV_DEVICE_TYPE_SGX);

	PDUMPCOMMENT(psServices,"Acquire device data", IMG_TRUE);
	eError = PVRSRVAcquireDeviceData (psServices,
			psDevID->ui32DeviceIndex,
			&sDevData,
			PVRSRV_DEVICE_TYPE_UNKNOWN);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXInit: PVRSRVAcquireDeviceData failed (%d)", eError));
		goto cleanup;
	}

	/* Dynamically allocate psInitInfo as it is too large for the stack */
	psInitInfo = (SGX_BRIDGE_INFO_FOR_SRVINIT *)PVRSRVAllocUserModeMem(sizeof(SGX_BRIDGE_INFO_FOR_SRVINIT));
	if (!psInitInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXInit: Out of memory"));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto cleanup;
	}

	/* Get initialisation info */
	eError = SGXGetInfoForSrvInit(&sDevData, psInitInfo);
	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXInit: SGXGetInfoForSrvInit failed (%d)", eError));
		goto cleanup;
	}

	PVRSRVMemSet(&sHeapInfo, 0, sizeof(SGX_HEAP_INFO));

	sHeapInfo.sPDDevPAddr = psInitInfo->sPDDevPAddr;

	/* setup local heap info */
	for(i=0; i<PVRSRV_MAX_CLIENT_HEAPS; i++)
	{
		PVRSRV_HEAP_INFO * psHeapInfo = &(psInitInfo->asHeapInfo[i]);
		switch(HEAP_IDX(psInitInfo->asHeapInfo[i].ui32HeapID))
		{
			case SGX_KERNEL_CODE_HEAP_ID:
			{
				sHeapInfo.psKernelCodeHeap = psHeapInfo;
				break;
			}
			case SGX_KERNEL_DATA_HEAP_ID:
			{
				sHeapInfo.psKernelDataHeap = psHeapInfo;
				break;
			}
			case SGX_PIXELSHADER_HEAP_ID:
			{
				sHeapInfo.psPixelShaderHeap = psHeapInfo;
				break;
			}
			case SGX_VERTEXSHADER_HEAP_ID:
			{
				sHeapInfo.psVertexShaderHeap = psHeapInfo;
				break;
			}
			case SGX_PDSPIXEL_CODEDATA_HEAP_ID:
			{
				sHeapInfo.psPDSPixelCodeDataHeap = psHeapInfo;
				break;
			}
		}
	}

	/* check they've all been setup (non-zero devVaddr) */
	PVR_ASSERT(sHeapInfo.psKernelCodeHeap->sDevVAddrBase.uiAddr != 0);
	PVR_ASSERT(sHeapInfo.psKernelDataHeap->sDevVAddrBase.uiAddr != 0);
	PVR_ASSERT(sHeapInfo.psPixelShaderHeap->sDevVAddrBase.uiAddr != 0);
	PVR_ASSERT(sHeapInfo.psVertexShaderHeap->sDevVAddrBase.uiAddr != 0);
	PVR_ASSERT(sHeapInfo.psPDSPixelCodeDataHeap->sDevVAddrBase.uiAddr != 0);

	/* Some miscellaneous SGX initialisation */

	/* Dynamically allocate psClientInfo as it is too large for the stack */
	psClientInfo = (SGX_CLIENT_INIT_INFO *)PVRSRVAllocUserModeMem(sizeof(*psClientInfo));
	if (!psClientInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXInit: Out of memory"));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto cleanup;
	}

	sInitScriptPart1.psCommands = psClientInfo->sScripts.asInitCommandsPart1;
	sInitScriptPart2.psCommands = psClientInfo->sScripts.asInitCommandsPart2;
	sDeinitScript.psCommands = psClientInfo->sScripts.asDeinitCommands;

	SGXMiscInit(&sHeapInfo, &sSrvInfo, psClientInfo);

	eError = SetupuKernel(&sDevData, &sHeapInfo, &sSrvInfo, psClientInfo);
	if (eError != PVRSRV_OK)
	{
		goto cleanup;
	}

#if defined(FIX_HW_BRN_23861)
	AllocateDumyStencilBuffers(&sDevData, &sHeapInfo, psClientInfo);
#endif

	/* Build the SGX initialisation scripts */
	eError = SGXInitialisePart1(&sInitScriptPart1);
	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXInit: SGXInitialisePart1 failed (%d)", eError));
		goto cleanup;
	}

	eError = SGXInitialisePart2(&sDevData, &sHeapInfo, &sSrvInfo, psClientInfo, &sInitScriptPart2);
	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXInit: SGXInitialisePart2 failed (%d)", eError));
		goto cleanup;
	}

	/* Build the SGX deinitialisation script */
	eError = SGXDeinitialise(&sDevData, &sDeinitScript);
	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXInit: SGXDeinitialise failed (%d)", eError));
		goto cleanup;
	}

	/* Perform second stage of SGX initialisation */
	eError = SGXDevInitPart2(&sDevData, psClientInfo);
	if (eError != PVRSRV_OK)
	{
		goto cleanup;
	}

	eError = PVRSRV_OK;

cleanup:
	if (psClientInfo)
	{
		PVRSRVFreeUserModeMem(psClientInfo);
	}

	if (psInitInfo)
	{
	    PVRSRVFreeUserModeMem(psInitInfo);
	}
	
	return eError;
}

/******************************************************************************
 End of file (sgxinit.c)
******************************************************************************/
