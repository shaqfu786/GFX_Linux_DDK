/*!****************************************************************************
@File           registry.h

@Title          Registry functions

@Author         Imagination Technologies

@Date           23rd May 2002

@Copyright      Copyright 2002-2006 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       Generic

@Description    Registry API

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-
$Log: registry.h $
******************************************************************************/
#ifndef __REGISTRY_H__
#define __REGISTRY_H__

#if defined (__cplusplus)
extern "C" {
#endif

#if defined(__linux__)
static INLINE IMG_BOOL OSReadRegistryString( IMG_UINT32 ui32DevCookie, IMG_CHAR *pszKey, IMG_CHAR *pszValue, IMG_CHAR *pszOutBuf, IMG_UINT32 ui32OutBufSize )
{
	PVR_UNREFERENCED_PARAMETER(ui32DevCookie);
	PVR_UNREFERENCED_PARAMETER(pszKey);
	PVR_UNREFERENCED_PARAMETER(pszValue);
	PVR_UNREFERENCED_PARAMETER(pszOutBuf);
	PVR_UNREFERENCED_PARAMETER(ui32OutBufSize);
	return IMG_FALSE;
};
static INLINE IMG_BOOL OSReadRegistryBinary( IMG_UINT32 ui32DevCookie, IMG_CHAR *pszKey, IMG_CHAR *pszValue, IMG_VOID *pszOutBuf, IMG_UINT32 *pui32OutBufSize )
{
	PVR_UNREFERENCED_PARAMETER(ui32DevCookie);
	PVR_UNREFERENCED_PARAMETER(pszKey);
	PVR_UNREFERENCED_PARAMETER(pszValue);
	PVR_UNREFERENCED_PARAMETER(pszOutBuf);
	PVR_UNREFERENCED_PARAMETER(pui32OutBufSize);
	return IMG_FALSE;
};
static INLINE IMG_BOOL OSReadRegistryInt( IMG_UINT32 ui32DevCookie, IMG_CHAR *pszKey, IMG_CHAR *pszValue, IMG_UINT32 *pui32Data )
{
	PVR_UNREFERENCED_PARAMETER(ui32DevCookie);
	PVR_UNREFERENCED_PARAMETER(pszKey);
	PVR_UNREFERENCED_PARAMETER(pszValue);
	PVR_UNREFERENCED_PARAMETER(pui32Data);
	return IMG_FALSE;
};
static INLINE IMG_BOOL OSWriteRegistryString( IMG_UINT32 ui32DevCookie, IMG_CHAR *pszKey, IMG_CHAR *pszValue, IMG_CHAR *pszInBuf )
{
	PVR_UNREFERENCED_PARAMETER(ui32DevCookie);
	PVR_UNREFERENCED_PARAMETER(pszKey);
	PVR_UNREFERENCED_PARAMETER(pszValue);
	PVR_UNREFERENCED_PARAMETER(pszInBuf);
	return IMG_FALSE;
};
static INLINE IMG_BOOL OSWriteRegistryBinary( IMG_UINT32 ui32DevCookie, IMG_CHAR *pszKey, IMG_CHAR *pszValue, IMG_CHAR *pszInBuf, IMG_UINT32 ui32InBufSize )
{
	PVR_UNREFERENCED_PARAMETER(ui32DevCookie);
	PVR_UNREFERENCED_PARAMETER(pszKey);
	PVR_UNREFERENCED_PARAMETER(pszValue);
	PVR_UNREFERENCED_PARAMETER(pszInBuf);
	PVR_UNREFERENCED_PARAMETER(ui32InBufSize);
	return IMG_FALSE;
};
static INLINE IMG_BOOL OSReadRegistryDWORDFromString(IMG_UINT32 ui32DevCookie, IMG_CHAR *pszKey, IMG_CHAR *pszValueName, IMG_UINT32 *pui32Data )
{
	PVR_UNREFERENCED_PARAMETER(ui32DevCookie);
	PVR_UNREFERENCED_PARAMETER(pszKey);
	PVR_UNREFERENCED_PARAMETER(pszValueName);
	PVR_UNREFERENCED_PARAMETER(pui32Data);
	return IMG_FALSE;
};
#else
IMG_BOOL OSReadRegistryString( IMG_UINT32 ui32DevCookie, IMG_CHAR *pszKey, IMG_CHAR *pszValue, IMG_CHAR *pszOutBuf, IMG_UINT32 ui32OutBufSize );
IMG_BOOL OSReadRegistryBinary( IMG_UINT32 ui32DevCookie, IMG_CHAR *pszKey, IMG_CHAR *pszValue, IMG_VOID *pszOutBuf, IMG_UINT32 *pui32OutBufSize );
IMG_BOOL OSReadRegistryInt( IMG_UINT32 ui32DevCookie, IMG_CHAR *pszKey, IMG_CHAR *pszValue, IMG_UINT32 *pui32Data );
IMG_BOOL OSWriteRegistryString( IMG_UINT32 ui32DevCookie, IMG_CHAR *pszKey, IMG_CHAR *pszValue, IMG_CHAR *pszInBuf );
IMG_BOOL OSWriteRegistryBinary( IMG_UINT32 ui32DevCookie, IMG_CHAR *pszKey, IMG_CHAR *pszValue, IMG_CHAR *pszInBuf, IMG_UINT32 ui32InBufSize );
IMG_BOOL OSReadRegistryDWORDFromString(IMG_UINT32 ui32DevCookie, IMG_CHAR *pszKey, IMG_CHAR *pszValueName, IMG_UINT32 *pui32Data );
#endif


#if defined (__cplusplus)
}
#endif

#endif /* __REGISTRY_H__ */

/******************************************************************************
 End of file (registry.h)
******************************************************************************/

