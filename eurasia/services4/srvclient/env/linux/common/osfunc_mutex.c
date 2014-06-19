/******************************************************************************
* Name         : osfunc_mutex.c
* Title        : User mode mutex implementation.
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
* Description  : User mode mutex implementation.
*
* Platform     : Linux
*
* Modifications:-
* $Log: osfunc_mutex.c $
******************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "services.h"
#include "pvr_debug.h"

#if !defined (QAC_ANALYSE)
#include "resource_lock.h"
#endif

/*
 * We support three implementations of mutexes.
 *
 * 1. PVR_MUTEXES_USING_PTHREAD_MUTEXES
 * If the above is defined, mutexes are implemented using POSIX
 * thread mutexes.
 *
 * 2. PVR_MUTEXES_COND_USING_PTHREAD_MUTEXES
 * If the above is defined, mutexes are implemented using either
 * resource locks or POSIX thread mutexes, with the latter being
 * used only if the application is detected as being multithreaded.
 * This implementation is only supported on x86 and ARM systems.
 * 
 * 3. PVR_MUTEXES_COND_USING_PTHREAD_CONDVARS
 * If the above is defined, mutexes are implemented using either
 * resource locks or POSIX thread condition variables, with the latter
 * being used only if the application is detected as being multithreaded.
 * This implementation is only supported on x86 and ARM systems.
 *
 * Selection of which implementation to use is done within this file.
 * For ARM systems, the default implementation is 
 * PVR_MUTEXES_COND_USING_PTHREAD_CONDVARS.  For all other systems,
 * the default is PVR_MUTEXES_USING_PTHREAD_MUTEXES.
 * The default implementation is different for ARM because 
 * PVR_MUTEXES_USING_PTHREAD_MUTEXES can result in poor single
 * threaded application performance on some systems, whilst
 * PVR_MUTEXES_COND_USING_PTHREAD_MUTEXES can result in uneven
 * thread scheduling in multithreaded applications.
 */

#if !(defined(PVR_MUTEXES_USING_PTHREAD_MUTEXES) ||		\
	defined(PVR_MUTEXES_COND_USING_PTHREAD_MUTEXES) ||	\
	defined(PVR_MUTEXES_COND_USING_PTHREAD_CONDVARS))
#if defined(__arm__)
#define	PVR_MUTEXES_COND_USING_PTHREAD_CONDVARS
#else
#define	PVR_MUTEXES_USING_PTHREAD_MUTEXES
#endif
#endif

/*
 * Each implementation has its own source file.  We pull in all three
 * source files, so that they get touched by the build system, allowing
 * the implementation to be changed for drivers built from source
 * that has been packaged up by the build system.
 * The header files are included at the top of this source file, to
 * ensure all the headers touched.
 */
#include "mutexes_using_pthread_mutexes.c"
#include "mutexes_cond_using_pthread_mutexes.c"
#include "mutexes_cond_using_pthread_condvars.c"
