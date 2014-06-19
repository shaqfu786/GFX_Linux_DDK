/*************************************************************************
 * Name         : main.c
 * Title        : Pixel Shader Compiler
 * Author       : John Russell
 * Created      : Jan 2002
 *
 * Copyright    : 2002-2010 by Imagination Technologies Limited. All rights reserved.
 *              : No part of this software, either material or conceptual 
 *              : may be copied or distributed, transmitted, transcribed,
 *              : stored in a retrieval system or translated into any 
 *              : human or computer language in any form by any means,
 *              : electronic, mechanical, manual or other-wise, or 
 *              : disclosed to third parties without the express written
 *              : permission of Imagination Technologies Limited, Unit 8, HomePark
 *              : Industrial Estate, King's Langley, Hertfordshire,
 *              : WD4 8LZ, U.K.
 *
 *
 * Modifications:-
 * $Log: userutils.c $
 **************************************************************************/

#include <string.h>
#include <stdio.h>

#include "use.h"
#include "coreutils.h"

#if defined(LINUX)
#include <strings.h>

#   define stricmp strcasecmp
#endif


/*
	Table of the cores supported by the current build of the compiler
*/
static const struct
{
	SGX_CORE_ID_TYPE	eType;
	IMG_PCHAR			pszName;
} g_asSupportedCores[] =
{
	#if defined(SUPPORT_SGX535)
	{SGX_CORE_ID_535, "sgx535"},
	#endif /* defined(SUPPORT_SGX535) */
	#if defined(SUPPORT_SGX520)
	{SGX_CORE_ID_520, "sgx520"},
	#endif /* defined(SUPPORT_SGX520) */
	#if defined(SUPPORT_SGX530)
	{SGX_CORE_ID_530, "sgx530"},
	#endif /* defined(SUPPORT_SGX530) */
	#if defined(SUPPORT_SGX531)
	{SGX_CORE_ID_531, "sgx531"},
	#endif /* defined(SUPPORT_SGX531) */
	#if defined(SUPPORT_SGX540)
	{SGX_CORE_ID_540, "sgx540"},
	#endif /* defined(SUPPORT_SGX540) */
	#if defined(SUPPORT_SGX541)
	{SGX_CORE_ID_541, "sgx541"},
	#endif /* defined(SUPPORT_SGX541) */
	#if defined(SUPPORT_SGX543)
	{SGX_CORE_ID_543, "sgx543"},
	#endif /* defined(SUPPORT_SGX543) */
	#if defined(SUPPORT_SGX544)
	{SGX_CORE_ID_544, "sgx544"},
	#endif /* defined(SUPPORT_SGX544) */
	#if defined(SUPPORT_SGX545)
	{SGX_CORE_ID_545, "sgx545"},
	#endif /* defined(SUPPORT_SGX545) */
	#if defined(SUPPORT_SGX554)
	{SGX_CORE_ID_554, "sgx554"},
	#endif /* defined(SUPPORT_SGX554) */
};

/*********************************************************************************
 Function		: GetDefaultCore

 Description	: Get the default core for this build of useasm.
 
 Parameters		: None.

 Return			: The default target core.
*********************************************************************************/
SGX_CORE_ID_TYPE GetDefaultCore(IMG_VOID)
{
	return g_asSupportedCores[0].eType;
}

/*********************************************************************************
 Function		: CheckCore

 Description	: Checks the compile target is supported.
 
 Parameters		: eCompileTarget		- Target to check.

 Return			: TRUE if supported; FALSE otherwise.
*********************************************************************************/
IMG_BOOL CheckCore(SGX_CORE_ID_TYPE	eCompileTarget)
{
	IMG_UINT32	uCoreIdx;

	for (uCoreIdx = 0; uCoreIdx < (sizeof(g_asSupportedCores) / sizeof(g_asSupportedCores[0])); uCoreIdx++)
	{
		if (g_asSupportedCores[uCoreIdx].eType == eCompileTarget)
		{
			return IMG_TRUE;
		}
	}
	return IMG_FALSE;
}

/*********************************************************************************
 Function		: ParseCoreSpecifier

 Description	: Converts a textual core specifier to an enumeration.
 
 Parameters		: pszTarget		- The target specifier to convert.

 Return			: The corresponding core type.
*********************************************************************************/
SGX_CORE_ID_TYPE ParseCoreSpecifier(IMG_PCHAR pszTarget)
{
	IMG_UINT32	uCoreIdx;

	for (uCoreIdx = 0; uCoreIdx < (sizeof(g_asSupportedCores) / sizeof(g_asSupportedCores[0])); uCoreIdx++)
	{
		if (stricmp(g_asSupportedCores[uCoreIdx].pszName, pszTarget) == 0)
		{
			return g_asSupportedCores[uCoreIdx].eType;
		}
	}
	return SGX_CORE_ID_INVALID;
}


/*********************************************************************************
 Function		: ParseRevisionSpecifier

 Description	: Converts a textual core specifier to an revision.
 
 Parameters		: pszTarget		- The target specifier to convert.

 Return			: The corresponding revision.
*********************************************************************************/
IMG_UINT32 ParseRevisionSpecifier(IMG_PCHAR pszTarget)
{
	IMG_UINT32 uiRev;
	return (sscanf(pszTarget, "%u", &uiRev) == 1) ? uiRev : 0;
}


