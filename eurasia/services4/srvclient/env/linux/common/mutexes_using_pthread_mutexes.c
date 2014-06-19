/******************************************************************************
* Name         : mutexes_using_pthread_mutexes.c
* Title        : Mutexes implemented using POSIX thread mutexes.
*
* Copyright    : 2008 by Imagination Technologies Limited.
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
* Description  : Mutexes implemented using POSIX thread mutexes.
*
* Platform     : Linux
*
* Modifications:-
* $Log: mutexes_using_pthread_mutexes.c $
******************************************************************************/
#if defined(PVR_MUTEXES_USING_PTHREAD_MUTEXES)

/******************************************************************************
 Function Name      : PVRSRVCreateMutex
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVCreateMutex(PVRSRV_MUTEX_HANDLE *phMutex)
{
	pthread_mutex_t *psMutex;
	IMG_INT iError;
#ifdef	DEBUG
	pthread_mutexattr_t sAttr;
	pthread_mutexattr_t *psAttr = &sAttr;
#else
	pthread_mutexattr_t *psAttr = NULL;
#endif

	psMutex = malloc(sizeof(*psMutex));

	if (psMutex == NULL)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

#ifdef	DEBUG
	/*
	 * For the debug driver, enable error checking on mutexes.  The non-debug
	 * driver uses the default mutex attributes.
	 */
	iError = pthread_mutexattr_init(psAttr);
	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: pthread_mutexattr_init failed (%d)", __FUNCTION__, iError));
		goto error_free;
	}

	iError = pthread_mutexattr_settype(psAttr, PTHREAD_MUTEX_ERRORCHECK);
	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: pthread_mutexattr_settype failed (%d)", __FUNCTION__, iError));
		goto error_attr;
	}
#endif
	iError = pthread_mutex_init(psMutex, psAttr);

	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: pthread_mutex_init failed (%d)", __FUNCTION__, iError));
		goto error_attr;
	}

#ifdef	DEBUG
	iError = pthread_mutexattr_destroy(psAttr);
	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: pthread_mutexattr_destroy failed (%d)", __FUNCTION__, iError));
	}
#endif

	*phMutex = (PVRSRV_MUTEX_HANDLE)psMutex;

	return PVRSRV_OK;

error_attr:
#ifdef	DEBUG
	iError = pthread_mutexattr_destroy(psAttr);
	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: pthread_mutexattr_destroy failed (%d)", __FUNCTION__, iError));
	}
error_free:
#endif
	free(psMutex);

	return PVRSRV_ERROR_INIT_FAILURE;
}


/******************************************************************************
 Function Name      : PVRSRVDestroyMutex
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVDestroyMutex(PVRSRV_MUTEX_HANDLE hMutex)
{
	pthread_mutex_t *psMutex = (pthread_mutex_t *)hMutex;
	IMG_INT iError;

	iError = pthread_mutex_destroy(psMutex);

	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: pthread_mutex_destroy failed (%d)", __FUNCTION__, iError));
		return PVRSRV_ERROR_MUTEX_DESTROY_FAILED;
	}

	free(psMutex);

	return PVRSRV_OK;
}


/******************************************************************************
 Function Name      : PVRSRVLockMutex
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT IMG_VOID IMG_CALLCONV PVRSRVLockMutex(PVRSRV_MUTEX_HANDLE hMutex)
{
	IMG_INT iError = pthread_mutex_lock((pthread_mutex_t *)hMutex);

	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: pthread_mutex_lock failed (%d)", __FUNCTION__, iError));
		abort();
	}
}


/*****************************************************************************
 Function Name      : PVRSRVUnlockMutex
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT IMG_VOID IMG_CALLCONV PVRSRVUnlockMutex(PVRSRV_MUTEX_HANDLE hMutex)
{
	IMG_INT iError = pthread_mutex_unlock((pthread_mutex_t *)hMutex);

	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: pthread_mutex_unlock failed (%d)", __FUNCTION__, iError));
		abort();
	}
}
#endif	/* defined(PVR_MUTEXES_USING_PTHREAD_MUTEXES) */
