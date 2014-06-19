/******************************************************************************
* Name         : mutexes_cond_using_pthread_condvars.c
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
* Description  : Mutexes using POSIX thread condition variables, on
		 condition multithread operation is detected.
*
* Platform     : Linux
*
* Modifications:-
* $Log: mutexes_cond_using_pthread_condvars.c $
******************************************************************************/
#if defined(PVR_MUTEXES_COND_USING_PTHREAD_CONDVARS)

/*
 * Using posix thread (PT) mutexes seems to result in uneven thread scheduling
 * on some platforms.  This implementation of PVR mutexes tries to avoid
 * using PT mutexes and condvars for non-multithreaded environments.
 * When resource locks or PT mutexes are used, they are only held within
 * the scope of the PVR mutex lock and unlock functions.
 */

#define	LIKELY(x) __builtin_expect((x), 1)
#define	UNLIKELY(x) __builtin_expect((x), 0)

/* Time to wait before retrying the resource lock */
#define PVR_MUTEX_SLEEP_US (10 * 1000)

struct pvr_mutex {
	RESOURCE_LOCK sResourceLock;
	pthread_mutex_t sPTMutex;
	pthread_cond_t sPTCond;
	IMG_BOOL bLocked;
	IMG_UINT32 ui32NumWaiting;
	IMG_BOOL bMultiThread;
	IMG_BOOL bGoingMultiThread;
};

static inline IMG_VOID PT_mutex_lock(pthread_mutex_t *psPTMutex)
{
	IMG_INT iError = pthread_mutex_lock(psPTMutex);
	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "PT_mutex_lock: pthread_mutex_lock failed (%d)", iError));
		abort();
	}
}

static inline IMG_VOID PT_mutex_unlock(pthread_mutex_t *psPTMutex)
{
	IMG_INT iError = pthread_mutex_unlock(psPTMutex);
	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "PT_mutex_unlock: pthread_mutex_unlock failed (%d)", iError));
		abort();
	}
}

static inline IMG_VOID PT_cond_wait(pthread_cond_t *psPTCond, pthread_mutex_t *psPTMutex)
{
	IMG_INT iError = pthread_cond_wait(psPTCond, psPTMutex);
	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "PT_cond_wait: pthread_mutex_cond failed (%d)", iError));
		abort();
	}
}

static inline IMG_VOID PT_cond_signal(pthread_cond_t *psPTCond)
{
	IMG_INT iError = pthread_cond_signal(psPTCond);
	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "PT_cond_signal: pthread_mutex_cond failed (%d)", iError));
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
		 * lock when it is already held.
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

	psPVRMutex = malloc(sizeof(*psPVRMutex));

	if (psPVRMutex == NULL)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	memset(psPVRMutex, 0, sizeof(*psPVRMutex));

	iError = pthread_mutex_init(&psPVRMutex->sPTMutex, NULL);

	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVCreateMutex: pthread_mutex_init failed (%d)", iError));
		goto error_free;
	}

	iError = pthread_cond_init(&psPVRMutex->sPTCond, NULL);

	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVCreateMutex: pthread_cond_init failed (%d)", iError));
		goto error_mutex;
	}

	*phMutex = (PVRSRV_MUTEX_HANDLE)psPVRMutex;

	return PVRSRV_OK;

error_mutex:
	iError = pthread_mutex_destroy(&psPVRMutex->sPTMutex);
	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVCreateMutex: pthread_mutex_destroy failed (%d)", iError));
	}
error_free:
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

	PVR_ASSERT(!psPVRMutex->bLocked);
	PVR_ASSERT(psPVRMutex->ui32NumWaiting == 0);

	iError = pthread_mutex_destroy(&psPVRMutex->sPTMutex);
	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVDestroyMutex: pthread_mutex_destroy failed (%d)", iError));
		return PVRSRV_ERROR_MUTEX_DESTROY_FAILED;
	}

	iError = pthread_cond_destroy(&psPVRMutex->sPTCond);
	if (iError != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVDestroyMutex: pthread_cond_destroy failed (%d)", iError));
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
IMG_EXPORT IMG_VOID IMG_CALLCONV PVRSRVLockMutex(PVRSRV_MUTEX_HANDLE hMutex)
{
	struct pvr_mutex *psPVRMutex = (struct pvr_mutex *)hMutex;
	IMG_BOOL bWaiting = IMG_FALSE;

	if (LIKELY(!psPVRMutex->bMultiThread))
	{
		if (LIKELY(TryLockResource(&psPVRMutex->sResourceLock)))
		{
			IMG_BOOL bLockTaken = IMG_FALSE;

			/*
			 * To avoid a race condition, we need to check
			 * bMultiThread again after taking the resource lock.
			 */
			if (LIKELY(!psPVRMutex->bMultiThread))
			{
				if (LIKELY(!psPVRMutex->bLocked))
				{
					/*
					 * There has been no lock contention,
					 * so take the fast path.
					 */
					psPVRMutex->bLocked = IMG_TRUE;
					bLockTaken = IMG_TRUE;
				}
			}

			if (UNLIKELY(!bLockTaken))
			{
				/*
				 * Try and get the fast path to yield when
				 * we switch over to multithreaded
				 * operation.
				 */
				psPVRMutex->bGoingMultiThread = IMG_TRUE;
				psPVRMutex->bMultiThread = IMG_TRUE;
			}

			UnlockResource(&psPVRMutex->sResourceLock);

			if (LIKELY(bLockTaken))
			{
				return;
			}
		}
	}

	/*
	 * We can set bGoingMultiThread without holding a lock, since it
	 * is only a hint to the unlock code that we are switching over
	 * to multithreaded operation.
	 */
	psPVRMutex->bGoingMultiThread = IMG_TRUE;

	PT_mutex_lock(&psPVRMutex->sPTMutex);

	for(;;)
	{
		LockResource(&psPVRMutex->sResourceLock);

		psPVRMutex->bMultiThread = IMG_TRUE;

		/*
		 * We need to wait if the mutex is held by another
		 * thread, or there are other threads waiting.
		 * In the latter case, we could just take the lock now,
		 * but then we would be jumping the queue.
		 */
		PVR_ASSERT(!(psPVRMutex->ui32NumWaiting == 0 && bWaiting));

		if (!psPVRMutex->bLocked &&
			(psPVRMutex->ui32NumWaiting == 0 || bWaiting))
		{
			psPVRMutex->bLocked = IMG_TRUE;
			if (bWaiting)
			{
				psPVRMutex->ui32NumWaiting--;
			}

			UnlockResource(&psPVRMutex->sResourceLock);
			break;
		}
		
		if (!bWaiting)
		{
			psPVRMutex->ui32NumWaiting++;
			bWaiting = IMG_TRUE;
		}

		UnlockResource(&psPVRMutex->sResourceLock);

		/*
		 * The mutex is automatically released when waiting on
		 * the condition variable, and retaken when the wait is
		 * over.
		 */
		PT_cond_wait(&psPVRMutex->sPTCond, &psPVRMutex->sPTMutex);
	}

	PT_mutex_unlock(&psPVRMutex->sPTMutex);

	PVR_ASSERT(psPVRMutex->bLocked);
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
	struct pvr_mutex *psPVRMutex = (struct pvr_mutex *)hMutex;
	IMG_BOOL bWaiters;

	PVR_ASSERT(psPVRMutex->bLocked);

	/*
	 * Don't assume bGoingMultiThread and bMultiThread are observed
	 * being set in the same order as they were actually set.
	 */
	if (LIKELY(!psPVRMutex->bGoingMultiThread && !psPVRMutex->bMultiThread))
	{
		if (LIKELY(TryLockResource(&psPVRMutex->sResourceLock)))
		{
			IMG_BOOL bMultiThread = psPVRMutex->bMultiThread;

			PVR_ASSERT(bMultiThread || psPVRMutex->ui32NumWaiting == 0);
			/*
			 * Once bMultithread is set, there may be threads
			 * waiting on the condition variable.
			 */
			if (LIKELY(!bMultiThread))
			{
				psPVRMutex->bLocked = IMG_FALSE;
			}

			UnlockResource(&psPVRMutex->sResourceLock);

			if (LIKELY(!bMultiThread))
			{
				return;
			}
		}
	}

	PT_mutex_lock(&psPVRMutex->sPTMutex);
	LockResource(&psPVRMutex->sResourceLock);

	psPVRMutex->bLocked = IMG_FALSE;
	bWaiters = (psPVRMutex->ui32NumWaiting != 0);

	UnlockResource(&psPVRMutex->sResourceLock);

	if (bWaiters != 0)
	{
		PT_cond_signal(&psPVRMutex->sPTCond);
	}

	PT_mutex_unlock(&psPVRMutex->sPTMutex);
}
#endif	/* defined(PVR_MUTEXES_COND_USING_PTHREAD_CONDVARS) */
