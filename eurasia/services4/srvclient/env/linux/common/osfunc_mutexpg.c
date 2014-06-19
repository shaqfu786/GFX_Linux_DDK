/******************************************************************************
* Name         : osfunc_mutexpg.c
* Title        : User mode coarse-grained mutex implementation.
*
* Copyright    : 2011 by Imagination Technologies Limited.
*                All rights reserved.  No part of this software, either
*                material or conceptual may be copied or distributed,
*                transmitted, transcribed, stored in a retrieval system
*                or translated into any human or computer language in any
*                form by any means, electronic, mechanical, manual or
*                other-wise, or disclosed to third parties without the
*                express written permission of Imagination Technologies
*                Limited, Unit 8, HomePark Industrial Estate,
*                King's Langley, Hertfordshire, WD4 8LZ, U.K.
*
* Description  : Provides functionality to allow synchronization around a single
*                global-per-address-space instance.
*
* Platform     : Linux
*
* Modifications:-
* $Log: osfunc_mutexpg.c $
******************************************************************************/

#include <stdlib.h>
#include <pthread.h>

#include "services.h"
#include "pvr_debug.h"

/* This is what we are synchronizing with */
static pthread_mutex_t ProcessGlobalMutex = PTHREAD_MUTEX_INITIALIZER;

/******************************************************************************
 Function Name      : PVRSRVLockProcessGlobalMutex
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/

IMG_IMPORT IMG_VOID IMG_CALLCONV PVRSRVLockProcessGlobalMutex(void)
{
    IMG_INT iError = pthread_mutex_lock(&ProcessGlobalMutex);

    if (iError != 0)
    {
        PVR_DPF((PVR_DBG_ERROR, "%s: pthread_mutex_lock failed (%d)", __FUNCTION__, iError));
        abort();
    }
}


/******************************************************************************
 Function Name      : PVRSRVUnlockProcessGlobalMutex
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/

IMG_IMPORT IMG_VOID IMG_CALLCONV PVRSRVUnlockProcessGlobalMutex(void)
{
    IMG_INT iError = pthread_mutex_unlock(&ProcessGlobalMutex);

    if (iError != 0)
    {
        PVR_DPF((PVR_DBG_ERROR, "%s: pthread_mutex_unlock failed (%d)", __FUNCTION__, iError));
        abort();
    }
}
