/*!****************************************************************************
@File           gralloc_defer.c

@Title          Graphics Allocator (gralloc) HAL component

@Author         Imagination Technologies

@Date           2011/08/01

@Copyright      Copyright (C) Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       linux

******************************************************************************/

#include <sys/types.h>

#include <pthread.h>
#include <errno.h>

#include "gralloc_defer.h"
#include "gralloc.h"

typedef struct _IMG_defer_flush_op_
{
	struct _IMG_defer_flush_op_	*psPrev;
	struct _IMG_defer_flush_op_ *psNext;
	IMG_defer_flush_op_pfn		pfnFlushOp;
	IMG_mapper_meminfo_t		*psMapperMemInfo;
	int							aiFd[MAX_SUB_ALLOCS];
}
IMG_defer_flush_op;

static pthread_t gpsDeferFlushOpThread;

static IMG_defer_flush_op *gpsDeferList;

static void *DeferFlushOpWorker(void *pvPriv)
{
	IMG_UINT32 ui32TriesLeft = GRALLOC_DEFAULT_WAIT_RETRIES;
	IMG_private_data_t *psPrivateData = pvPriv;

	while(1)
	{
		IMG_defer_flush_op *psEntry, *psNext = IMG_NULL;
		IMG_BOOL bDeferListEmpty = IMG_FALSE;

		PVRSRVLockMutex(psPrivateData->hMutex);

		for(psEntry = gpsDeferList; psEntry; psEntry = psNext)
		{
			int err;

			/* psEntry may be freed, so back-up psNext now */
			psNext = psEntry->psNext;

			if(!OpsFlushed(psEntry->psMapperMemInfo->apsMemInfo))
				continue;

			/* This function should not block, so we can hold the lock. */
			err = psEntry->pfnFlushOp(psPrivateData, psEntry->psMapperMemInfo,
									  psEntry->aiFd);
			if (err)
			{
				/* If we got -EAGAIN, it means that even though operations
				 * were flushed on the buffer, for some other reason (probably
				 * a workaround) we can't free/unmap it right now and should
				 * try again later.
				 */
				if(err == -EAGAIN)
					continue;

				PVR_DPF((PVR_DBG_ERROR, "%s: DeferFlushOp failed. "
										"Allocation will leak.", __func__));
			}

			/* Pass or fail, we need to remove it from the defer list */

			if(psEntry->psNext)
				psEntry->psNext->psPrev = psEntry->psPrev;
			if(psEntry->psPrev)
				psEntry->psPrev->psNext = psEntry->psNext;
			if(psEntry == gpsDeferList)
				gpsDeferList = psEntry->psNext;

			free(psEntry);
		}

		/* Check the list inside the lock */
		if(!gpsDeferList)
			bDeferListEmpty = IMG_TRUE;

		PVRSRVUnlockMutex(psPrivateData->hMutex);

		/* If the list was empty when checked, the workaround thread is
		 * probably no longer needed and can die. There is a race here --
		 * if another thread adds a new entry, we will spawn a new thread
		 * before this one dies -- but since we have committed to
		 * terminating this thread, the race should not matter.
		 *
		 * This worker will be re-spawned as required.
		 */
		if(bDeferListEmpty)
			break;

		if(!ui32TriesLeft)
		{
#if 0
			IMG_gralloc_module_t *psGrallocModule =
				container(psPrivateData, IMG_gralloc_module_t, sPrivateData);

			/* We can't trigger kernel dumps every time this happens
			 * because an app may unregister() a buffer that the display
			 * is still using. If the display doesn't update for 5+
			 * seconds, we'd time out (but nothing is actually wrong).
			 */
			PVRSRVClientEvent(PVRSRV_CLIENT_EVENT_HWTIMEOUT,
							  &psPrivateData->sDevData, psGrallocModule);
#endif

			PVR_DPF((PVR_DBG_ERROR, "%s: Timed out waiting for ops to flush",
									__func__));

			/* Re-arm the timer and go again. There's nothing sane we can
			 * do in this situation. Before the workaround, we would just
			 * leak the memory. At least now we will inform the user every
			 * GRALLOC_DEFAULT_WAIT_RETRIES that there is a serious problem.
			 */
			ui32TriesLeft = GRALLOC_DEFAULT_WAIT_RETRIES;
		}

		if(PVRSRVEventObjectWait(psPrivateData->sDevData.psConnection,
			psPrivateData->sSGXInfo.sMiscInfo.hOSGlobalEvent) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_MESSAGE, "%s: Timeout waiting for ops to flush",
									  __func__));
			ui32TriesLeft--;
		}
	}

	return IMG_NULL;
}

IMG_INTERNAL
int CheckDeferFlushOp(IMG_private_data_t *psPrivateData,
					  IMG_defer_flush_op_pfn pfnFlushOp,
					  IMG_mapper_meminfo_t *psMapperMemInfo,
					  int aiFd[MAX_SUB_ALLOCS])
{
	IMG_defer_flush_op *psEntry;
	int err, i;

	ENTERED();

	/* See if the ops flushed already. If they didn't, wait for one
	 * event object timeout (<100ms) synchronously before spawning
	 * a thread to wait for longer (asynchronously).
	 */

	if(OpsFlushed(psMapperMemInfo->apsMemInfo))
	{
		err = pfnFlushOp(psPrivateData, psMapperMemInfo, aiFd);

		/* See above for the meaning of -EAGAIN */
		if(err != -EAGAIN)
			goto err_out;
	}

	PVRSRVEventObjectWait(psPrivateData->sDevData.psConnection,
						  psPrivateData->sSGXInfo.sMiscInfo.hOSGlobalEvent);

	if(OpsFlushed(psMapperMemInfo->apsMemInfo))
	{
		err = pfnFlushOp(psPrivateData, psMapperMemInfo, aiFd);

		/* See above for the meaning of -EAGAIN */
		if(err != -EAGAIN)
			goto err_out;
	}

	/* The private data lock has been taken by the caller, so we should
	 * not race here because only one thread can call us at a time.
	 */

	if(!gpsDeferList)
	{
		/* No list? Spawn a thread. The worker will be held off processing
		 * (by the mutex) until we add the first list entry.
		 */
		err = pthread_create(&gpsDeferFlushOpThread, IMG_NULL,
							 DeferFlushOpWorker, psPrivateData);
		if(err)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to spawn DeferFlushOp"
									"worker thread", __func__));
			goto err_out;
		}
	}

	psEntry = calloc(1, sizeof(IMG_defer_flush_op));

	psEntry->pfnFlushOp      = pfnFlushOp;
	psEntry->psMapperMemInfo = psMapperMemInfo;
	for(i = 0; i < MAX_SUB_ALLOCS; i++)
		psEntry->aiFd[i] = aiFd[i];

	if(gpsDeferList)
	{
		psEntry->psNext = gpsDeferList;
		gpsDeferList->psPrev = psEntry;
	}

	gpsDeferList = psEntry;
	err = 0;
err_out:
	EXITED();
	return err;
}
