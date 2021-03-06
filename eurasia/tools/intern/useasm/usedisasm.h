/******************************************************************************
 * Name         : usedisasm.h
 * Title        : Pixel Shader Compiler
 * Author       : John Russell
 * Created      : Jan 2002
 *
 * Copyright    : 2002-2010 by Imagination Technologies Limited.
 *              : All rights reserved. No part of this software, either
 *              : material or conceptual may be copied or distributed,
 *              : transmitted, transcribed, stored in a retrieval system or
 *              : translated into any human or computer language in any form
 *              : by any means,electronic, mechanical, manual or otherwise,
 *              : or disclosed to third parties without the express written
 *              : permission of Imagination Technologies Limited,
 *              : Home Park Estate, Kings Langley, Hertfordshire,
 *              : WD4 8LZ, U.K.
 *
 * Modifications:-
 * $Log: usedisasm.h $
 *****************************************************************************/

#if !defined(__USEASM_USEDISASM_H)
#define __USEASM_USEDISASM_H

#include "pvr_debug.h"
#include "img_defs.h"
#include "img_types.h"

typedef enum
{
	USEDIS_FORMAT_CONTROL_STATE_UNKNOWN,
	USEDIS_FORMAT_CONTROL_STATE_ON,
	USEDIS_FORMAT_CONTROL_STATE_OFF
} USEDIS_FORMAT_CONTROL_STATE;

typedef struct _USE_DISASSEMBLE_RUNTIME_STATE
{
	USEDIS_FORMAT_CONTROL_STATE	eColourFormatControl;
	USEDIS_FORMAT_CONTROL_STATE	eEFOFormatControl;	
} USEDIS_RUNTIME_STATE, *PUSEDIS_RUNTIME_STATE;

typedef USEDIS_RUNTIME_STATE const* PCUSEDIS_RUNTIME_STATE;

typedef enum
{
	USEDISASM_OK = 0,
	USEDISASM_ERROR_INVALID_INSTRUCTION,
	USEDISASM_ERROR_INVALID_IDXSCW_DESTINATION_BANK,
	USEDISASM_ERROR_INVALID_IDXSCR_SOURCE_BANK,
	USEDISASM_ERROR_INVALID_LDST_DATA_SIZE,
	USEDISASM_ERROR_INVALID_FLOWCONTROL_OP2,
	USEDISASM_ERROR_INVALID_PHAS_RATE,
	USEDISASM_ERROR_INVALID_PHAS_WAIT_COND,
	USEDISASM_ERROR_INVALID_CFI_MODE,
	USEDISASM_ERROR_INVALID_OTHER2_OP2,
	USEDISASM_ERROR_INVALID_FNRM_SWIZZLE,
	USEDISASM_ERROR_INVALID_MOE_OP2,
	USEDISASM_ERROR_INVALID_FDUAL_MOV_SRC,
	USEDISASM_ERROR_INVALID_COMPLEX_OP2,
	USEDISASM_ERROR_INVALID_COMPLEX_DATA_TYPE,
	USEDISASM_ERROR_INVALID_FLOAT_OP2,
	USEDISASM_ERROR_INVALID_MOVC_DATA_TYPE,
	USEDISASM_ERROR_INVALID_DVEC_OP1,
	USEDISASM_ERROR_INVALID_DVEC_OP2,
	USEDISASM_ERROR_INVALID_DVEC_SRCCFG,
	USEDISASM_ERROR_INVALID_VECCOMPLEX_DEST_DATA_TYPE,
	USEDISASM_ERROR_INVALID_VECCOMPLEX_SRC_DATA_TYPE,
	USEDISASM_ERROR_INVALID_VECMOV_TYPE,
	USEDISASM_ERROR_INVALID_VECMOV_DATA_TYPE,
	USEDISASM_ERROR_INVALID_VPCK_FORMAT_COMBINATION,
	USEDISASM_ERROR_INVALID_SMP_LOD_MODE,
	USEDISASM_ERROR_INVALID_SMP_SRD_MODE,
	USEDISASM_ERROR_INVALID_SMP_PCF_MODE,
	USEDISASM_ERROR_INVALID_SMP_DATA_TYPE,
	USEDISASM_ERROR_INVALID_SMP_FORMAT_CONVERSION,
	USEDISASM_ERROR_INVALID_FIRH_SOURCE_FORMAT,
	USEDISASM_ERROR_INVALID_PCK_FORMAT_COMBINATION,
	USEDISASM_ERROR_INVALID_BITWISE_OP2,
	USEDISASM_ERROR_INVALID_TEST_ALUOP,
	USEDISASM_ERROR_INVALID_TEST_SIGN_TEST,
	USEDISASM_ERROR_INVALID_TEST_ZERO_TEST,
	USEDISASM_ERROR_INVALID_TEST_MASK_TYPE,
	USEDISASM_ERROR_INVALID_SOP2_CSEL1,
	USEDISASM_ERROR_INVALID_SOP2_CSEL2,
	USEDISASM_ERROR_INVALID_SOPWM_SEL1,
	USEDISASM_ERROR_INVALID_SOPWM_SEL2,
	USEDISASM_ERROR_INVALID_IMAE_SRC2_TYPE,
	USEDISASM_ERROR_INVALID_FIRV_FLAG,
	USEDISASM_ERROR_INVALID_SSUM16_ROUND_MODE,
	USEDISASM_ERROR_INVALID_ADIFFIRVBILIN_OPSEL,
	USEDISASM_ERROR_INVALID_BILIN_SOURCE_FORMAT,
	USEDISASM_ERROR_INVALID_OPCODE,
	USEDISASM_ERROR_INVALID_IMA32_STEP,
	USEDISASM_ERROR_INVALID_MUTEX_NUMBER,
	USEDISASM_ERROR_INVALID_EMIT_MTE_CONTROL,
	USEDISASM_ERROR_INVALID_MONITOR_NUMBER,
	USEDISASM_ERROR_INVALID_OTHER_OP2,
	USEDISASM_ERROR_INVALID_VISTEST_OP2,
	USEDISASM_ERROR_INVALID_SPECIAL_OPCAT,
	USEDISASM_ERROR_INVALID_EMIT_TARGET,
	USEDISASM_ERROR_INVALID_CND_OP2,
	USEDISASM_ERROR_INVALID_CND_INVALID_PCND,
	USEDISASM_ERROR_INVALID_LDST_ATOMIC_OP
} USEDISASM_ERROR;

IMG_BOOL IMG_CALLCONV UseDisassembleInstruction(PCSGX_CORE_DESC psTarget,
												IMG_UINT32 uInst0,
												IMG_UINT32 uInst1,
												IMG_PCHAR pszInst);

IMG_VOID IMG_CALLCONV UseDisassembler(PCSGX_CORE_DESC psTarget,
									  IMG_UINT32 uInstCount,
									  IMG_PUINT32 puInstructions);

USEDISASM_ERROR IMG_CALLCONV UseDecodeInstruction(PCSGX_CORE_DESC			psTarget,
												  IMG_UINT32				uInst0,
												  IMG_UINT32				uInst1,
												  PCUSEDIS_RUNTIME_STATE	psRuntimeState,
												  PUSE_INST					psInst);

#if defined(SGX543) || defined(SGX544) || defined(SGX554) || defined(SUPPORT_SGX543) || defined(SUPPORT_SGX544) || defined(SUPPORT_SGX554)
USEDISASM_ERROR IMG_CALLCONV UseGetVecDualIssueUSSource(IMG_UINT32	uInst0,
													    IMG_UINT32	uInst1,
													    IMG_PUINT32	puOp1USSrcIdx,
														IMG_PUINT32	puOp2USSrcIdx);
#endif /* defined(SGX543) || defined(SGX544) || defined(SUPPORT_SGX543) || defined(SUPPORT_SGX544) */


#endif /* __USEASM_USEDISASM_H */

/******************************************************************************
 End of file (use.h)
******************************************************************************/
