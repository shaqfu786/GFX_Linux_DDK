/******************************************************************************
* Name         : mutexes_cond_using_pthread_mutexes.c
* Title        : Mutexes implemented using POSIX thread condition variables.
*
* Copyright    : 2010 by Imagination Technologies Limited.
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
* Description  : Mutexes using pthread mutexes, on condition multithread
*		 operation is detected.
*
* Platform     : Linux
*
* Modifications:-
* $Log: mutexes_cond_using_pthread_mutexes.c $
******************************************************************************/
#if defined(PVR_MUTEXES_COND_USING_PTHREAD_MUTEXES)

/*
 * Taking posix thread (PT) mutexes seems to be more exspensive than expected
 * on some platforms.  This implementation of PVR mutexes tries to avoid
 * taking PT mutexes for non-multithreaded environments.
 */

#define	LIKELY(x) __builtin_expect((x), 1)
#define	UNLIKELY(x) __builtin_expect((x), 0)

/* Time to wait before retrying the resource lock */
#define PVR_MUTEX_SLEEP_US (10 * 1000)

/* The locking order is sPTMutex then sResourceLock */
struct pvr_mutex {
	RESOURCE_LOCK sResourceLock;
	pthread_mutex_t sPTMutex;
	IMG_BOOL bGoingMultiThreaded;
	IMG_BOOL bMultiThread;
};

static inline void PT_mutex_lock(pthread_mutex_t *psPTMutex)
{
	IMG_INT iError = pthread_mutex_lock(psPTMutex);
	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: pthread_mutex_lock failed (%d)", __FUNCTION__, iError));
		abort();
	}
}

static inline void PT_mutex_unlock(pthread_mutex_t *psPTMutex)
{
	IMG_INT iError = pthread_mutex_unlock(psPTMutex);
	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: pthread_mutex_unlock failed (%d)", __FUNCTION__, iError));
		abort();
	}
}

static inline void LockResource(RESOURCE_LOCK *psResourceLock)
{
	for (;;)
	{
		if (TryLockResource(psResourceLock))
		{
			break;
		}
		/*
		 * If we couldn't get the resource lock, sleep for a
		 * while and try again.  The sleep is to try and reduce
		 * problems due to livelock; when we try to take the
		 * lock on a CPU that already holds the lock.
		 * Calling sched_yield, as an alternative to
		 * sleeping, would not yield to another thread in
		 * all cases.
		 */
		PVRSRVWaitus(PVR_MUTEX_SLEEP_US);
	}
}

/******************************************************************************
 Function Name      : PVRSRVCreateMutex
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
******************************************************************************/
IMG_EXPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVCreateMutex(PVRSRV_MUTEX_HANDLE *phMutex)
{
	struct pvr_mutex *psPVRMutex;
	IMG_INT iError;
#if defined(DEBUG)
	pthread_mutexattr_t sPTAttr;
	pthread_mutexattr_t *psPTAttr = &sPTAttr;
#else
	pthread_mutexattr_t *psPTAttr = NULL;
#endif

	psPVRMutex = malloc(sizeof(*psPVRMutex));

	if (psPVRMutex == NULL)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	memset(psPVRMutex, 0, sizeof(*psPVRMutex));

#if defined(DEBUG)
	/*
	 * For the debug driver, enable error checking on mutexes.
	 * The non-debug driver uses the default mutex attributes.
	 */
	iError = pthread_mutexattr_init(psPTAttr);
	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: pthread_mutexattr_init failed (%d)", __FUNCTION__, iError));
		goto error_free;
	}

	iError = pthread_mutexattr_settype(psPTAttr, PTHREAD_MUTEX_ERRORCHECK);
	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: pthread_mutexattr_settype failed (%d)", __FUNCTION__, iError));
		goto error_attr;
	}
#endif
	iError = pthread_mutex_init(&psPVRMutex->sPTMutex, psPTAttr);

	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: pthread_mutex_init failed (%d)", __FUNCTION__, iError));
		goto error_attr;
	}

#if defined(DEBUG)
	iError = pthread_mutexattr_destroy(psPTAttr);
	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: pthread_mutexattr_destroy failed (%d)", __FUNCTION__, iError));
	}
#endif

	*phMutex = (PVRSRV_MUTEX_HANDLE)psPVRMutex;

	return PVRSRV_OK;

error_attr:
#if defined(DEBUG)
	iError = pthread_mutexattr_destroy(psPTAttr);
	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: pthread_mutexattr_destroy failed (%d)", __FUNCTION__, iError));
	}
error_free:
#endif
	free(psPVRMutex);

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
	struct pvr_mutex *psPVRMutex = (struct pvr_mutex *)hMutex;
	IMG_INT iError;

	iError = pthread_mutex_destroy(&psPVRMutex->sPTMutex);
	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: pthread_mutex_destroy failed (%d)", __FUNCTION__, iError));
		return PVRSRV_ERROR_MUTEX_DESTROY_FAILED;
	}

	free(psPVRMutex);

	return PVRSRV_OK;
}

/******************************************************************************
 Function Name      : PVRSRVLockMutex
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT void IMG_CALLCONV PVRSRVLockMutex(PVRSRV_MUTEX_HANDLE hMutex)
{
	struct pvr_mutex *psPVRMutex = (struct pvr_mutex *)hMutex;

	/*
	 * Once bMultiThread is set, it stays set.  If it is unset, we
	 * will need to take the resource lock, and check the flag again,
	 * to avoid a race condition.
	 */
	if (UNLIKELY(psPVRMutex->bMultiThread))
	{
		PT_mutex_lock(&psPVRMutex->sPTMutex);
	}
	else
	{
		if (LIKELY(TryLockResource(&psPVRMutex->sResourceLock)))
		{
			/*
			 * Check bMultiThread again with the resource lock
			 * held, to avoid a race condition.
			 */
			if (UNLIKELY(psPVRMutex->bMultiThread))
			{
				UnlockResource(&psPVRMutex->sResourceLock);
				PT_mutex_lock(&psPVRMutex->sPTMutex);
			}
			/*
			 * If bMultiThread is not set, we exit the
			 * function with the resource lock held, and
			 * the PT mutex unlocked.  This is the fast
			 * path, for when there has been no contention
			 * for the PVR mutex.
			 */
		}
		else
		{
			/*
			 * Each thread will go through this path at most
			 * once.
			 */

			/*
			 * Try to get the fast path to yield when
			 * the mutex is unlocked.
			 */
			psPVRMutex->bGoingMultiThreaded = IMG_TRUE;

			PT_mutex_lock(&psPVRMutex->sPTMutex);

			/*
			 * If another thread has been through this path,
			 * it will already have set bMultiThread, so we
			 * don't need to set it, avoiding the need to take
			 * the resource lock.
			 */
			if (!psPVRMutex->bMultiThread)
			{
				/*
				 * If we reach this point, we are waiting
				 * for another thread that has taken the
				 * fast path.  We should reach this point
				 * at most once for each PVR mutex.
				 */
				LockResource(&psPVRMutex->sResourceLock);
				psPVRMutex->bMultiThread = IMG_TRUE;
				UnlockResource(&psPVRMutex->sResourceLock);
			}
		}
	}
}

/*****************************************************************************
 Function Name      : PVRSRVUnlockMutex
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
******************************************************************************/
IMG_EXPORT void IMG_CALLCONV PVRSRVUnlockMutex(PVRSRV_MUTEX_HANDLE hMutex)
{
	struct pvr_mutex *psPVRMutex = (struct pvr_mutex *)hMutex;

	/*
	 * Since both the PT mutex and the resource lock are held when
	 * bMultiThread is set, we know that the flag cannot change value
	 * whilst the PVR mutex is held, since we are holding either the
	 * resource lock, or the PT mutex.
	 */
	if (UNLIKELY(psPVRMutex->bMultiThread))
	{
		PT_mutex_unlock(&psPVRMutex->sPTMutex);
	}
	else
	{
		UnlockResource(&psPVRMutex->sResourceLock);
		if (UNLIKELY(psPVRMutex->bGoingMultiThreaded))
		{
			/*
			 * Give other threads a chance to run if there
			 * is mutex contention.  Without the yield,
			 * the fast path code would have an advantage
			 * when competing for the resource lock, since
			 * the slow path code sleeps between attempts
			 * to take the resource lock.
			 */
#if defined(_POSIX_PRIORITY_SCHEDULING)
			sched_yield();
#else
			PVRSRVWaitus(PVR_MUTEX_SLEEP_US);
#endif
		}
	}
}
#endif	/* defined(PVR_MUTEXES_COND_USING_PTHREAD_MUTEXES) */
