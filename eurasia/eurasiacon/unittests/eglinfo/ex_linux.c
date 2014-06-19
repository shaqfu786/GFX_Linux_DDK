/****************************************************************************
 * Name         : ex_linux.c
 *
 * Author       : Yuan Wang
 * Created      : 06/05/2008
 *
 * Copyright    : 2004-2008 by Imagination Technologies Limited. All rights reserved.
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
 * Platform     : ANSI
 * $Log: ex_linux.c $
 *
 **************************************************************************/

#include "ex_global.h"

__internal void *exLoadLibrary(char *exLibraryName)
{
    void *hLib;
    
    hLib = dlopen(exLibraryName, RTLD_LAZY);
    if (!hLib)
    {
		ERROR("Couldn't load library %s: %s", exLibraryName, dlerror());
		return NULL;
    }

    return hLib;
}

__internal int exUnloadLibrary(void *exhLib)
{
    if(!exhLib)
	{
		ERROR("invalid library handle\n");
		return 0;
	}

    if (dlclose(exhLib))
	{
		ERROR("dlclose failed %s\n", dlerror());
		return 0;
	}

	return 1;
}

__internal int exGetLibFuncAddr(void *exhLib, char *exFuncName,
								GENERICPF *ppfFuncAddr)
{
    *ppfFuncAddr = (GENERICPF)dlsym(exhLib, exFuncName);
    return *ppfFuncAddr != NULL;
}
