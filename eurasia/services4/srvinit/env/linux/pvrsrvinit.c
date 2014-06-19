/*!****************************************************************************
@File           main.c

@Title          Services initialisation routines

@Author         Imagination Technologies

@Date           17/10/2007

@Copyright      Copyright 2005-2007 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       GCC

@Description    Device specific functions

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-
$Log: main.c $
******************************************************************************/

#include "img_defs.h"
#include "pvr_debug.h"
#include "srvinit.h"

IMG_INT main(IMG_INT unref__ argc, IMG_CHAR unref__ *argv[])
{
	PVRSRV_ERROR eError;

	PVR_UNREFERENCED_PARAMETER(argc);
	PVR_UNREFERENCED_PARAMETER(argv);

	eError = SrvInit();
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "main: SrvInit failed (%d)", eError));
	}

	return (eError == PVRSRV_OK) ? 0 : 1;
}

/******************************************************************************
 End of file (main.c)
*****************************************************************************/
