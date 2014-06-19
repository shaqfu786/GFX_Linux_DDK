/******************************************************************************
* Name         : resource_lock.h
* Title        : Resource locks.
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
* Description  : Resource locks (spin locks).
*
* Platform     : Linux
*
* Modifications:-
* $Log: resource_lock.h $
******************************************************************************/

#if defined(__arm__) || defined(__i386__)

typedef IMG_UINT32 RESOURCE_LOCK;

static inline IMG_BOOL TryLockResource(RESOURCE_LOCK *pResource)
{
	IMG_UINT32 ui32Work;
	IMG_UINT32 ui32Lock = 2;

	volatile IMG_UINT32 *pui32Access = (volatile IMG_UINT32 *)pResource;

#if defined(__i386__)
	asm volatile
		("	pause				\n\t"
		 "	mov $0, %0			\n\t"
		 "	lock cmpxchgl %1,(%2)		\n\t"
		 : "=&a" (ui32Work)
		 : "r" (ui32Lock), "r" (pui32Access)
		 : "memory", "cc");
#else
	#if defined(__arm__)
	/*
	 * On ARM, the strexeq instruction returns 1 if it couldn't perform
	 * the store, because the destination has been accessed after the
	 * ldrex instruction has executed. The value of ui2Lock must be
	 * different from 1, so we can distinguish a failure to acquire the
	 * lock due to a strexeq failure, as opposed to another thread
	 * having acquired the lock.
	 */
	PVR_ASSERT(ui32Lock != 1);

	/*
	 * Spin until the lock is acquired, either by this thread or another
	 * one.
	 */
	do {
		asm volatile
			("	ldrex %0, [%2]		\n\t"
			"	cmp %0, #0		\n\t"
			"	strexeq %0, %1, [%2]	\n\t"
			: "=&r"(ui32Work)
			: "r" (ui32Lock), "r"(pui32Access)
			: "memory", "cc");
	} while (ui32Work == 1);
	#else 
		#error "TryLockResource - Unknown architecture."
	#endif
#endif
	return ui32Work == 0;
}

static inline IMG_VOID UnlockResource(RESOURCE_LOCK *pResource)
{
	volatile IMG_UINT32 *pui32Access = (volatile IMG_UINT32 *)pResource;

#if !defined(PVR_PLATFORM_NOT_SMP)
#if defined(__i386__)
		/*
		 * Memory fence.  Ensure all reads and writes
		 * have completed on this CPU before dropping the lock.
		 */
		asm volatile
			("mfence"
			:
			:
			: "memory");
#else
	#if defined(__arm__)
		/*
		 * Data memory barrier.  Ensure all reads and writes
		 * have completed on this CPU before dropping the lock.
		 */
		asm volatile
			("dmb"
			:
			:
			: "memory");
	#endif
#endif
#endif	/* !defined(PVR_PLATFORM_NOT_SMP) */
#ifdef	DEBUG
	if(!*pui32Access)
	{
		PVR_DPF((PVR_DBG_ERROR,"UnlockResource: Resource not locked"));
		abort();
	}
#endif
	*pui32Access = 0;
}

#endif	/* defined(__arm__) || defined(__i386__) */
