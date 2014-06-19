/***************************************************************************
* Name		   : memchk.c
*
* Copyright    : 2006 by Imagination Technologies. All rights reserved.
*			   : No part of this software, either material or conceptual 
*			   : may be copied or distributed, transmitted, transcribed,
*			   : stored in a retrieval system or translated into any 
*			   : human or computer language in any form by any means,
*			   : electronic, mechanical, manual or other-wise, or 
*			   : disclosed to third parties without the express written
*			   : permission of Imagination Technologies, Unit 8, HomePark
*			   : Industrial Estate, King's Langley, Hertfordshire,
*			   : WD4 8LZ, U.K.
*
* Platform	   : Linux
*
* $Log: memchk.c $
*
*  --- Revision Logs Removed --- 
****************************************************************************/
#ifdef DEBUG

#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>

#include "img_defs.h"
#include "pvr_debug.h"
#include "services.h"
#include "memchk.h"

#define MAX_MEMCHK_FILENAME_LENGTH 50

/*
// TRUE = check for overruns, FALSE = check for underruns
*/
static IMG_BOOL bCheckForOverRuns;

/*
// Controls use of guard page or normal malloc
*/
static IMG_BOOL bUseGuardPage;

/*
// Have we read the apphints yet?
*/
static IMG_BOOL bGotMemTrackingApphints = IMG_FALSE;

static pthread_mutex_t sAllocListMutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct _sAlloc_Struct_
{
	IMG_UINT32	ui32Size;

	IMG_VOID  	*pvTrueAddress;							/* Address of real allocation (including guard page etc) */
	IMG_VOID  	*pvReturnedAddress;						/* Address returned by the alloc function */

	IMG_CHAR   	szFileName[MAX_MEMCHK_FILENAME_LENGTH];	/* Filename containing the call to alloc/realloc */
	IMG_INT32  	ui32LineNumber;							/* Line number of the alloc/realloc call in file 'pszFileName' */

	struct	_sAlloc_Struct_	*psNext;
	struct	_sAlloc_Struct_	*psPrevious;

} sAlloc_Struct;


static sAlloc_Struct *psAllocList = IMG_NULL;

static IMG_UINT32 ui32MemCurrentUsage	= 0;	/* Amount of memory currently allocated */
static IMG_UINT32 ui32MemHighWaterMark	= 0;	/* High water mark of allocations */
static IMG_UINT32 ui32NumAllocs			= 0;	/* Current number of allocations */


static IMG_VOID LockAllocList(IMG_VOID)
{
	IMG_INT error;

	if ((error = pthread_mutex_lock(&sAllocListMutex)) != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "LockAllocList: Couldn't lock mutex (%d)", error));
		abort();
	}
}

static IMG_VOID UnlockAllocList(IMG_VOID)
{
	IMG_INT error;

	if ((error = pthread_mutex_unlock(&sAllocListMutex)) != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "UnlockAllocList: Couldn't unlock mutex (%d)", error));
		abort();
	}
}

/***********************************************************************************
 Function Name      : GetMemTrackingApphints
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : 
                      
************************************************************************************/
static IMG_VOID GetMemTrackingApphints(IMG_VOID)
{
	IMG_BOOL bDefault;
	IMG_VOID* pvHintState;
	
	bDefault = IMG_FALSE;

	PVRSRVCreateAppHintState(IMG_SRV_UM, 0, &pvHintState);

	PVRSRVGetAppHint(pvHintState, "UseGuardPage", IMG_UINT_TYPE, &bDefault, &bUseGuardPage);

	if (bUseGuardPage)
	{
		bDefault = IMG_TRUE;

		PVRSRVGetAppHint(pvHintState, "CheckForOverRuns", IMG_UINT_TYPE, &bDefault, &bCheckForOverRuns);

		PVR_DPF((PVR_DBG_WARNING,"Guard page code enabled - checking for %s",bCheckForOverRuns?"OVERruns":"UNDERruns"));
	}	

	PVRSRVFreeAppHintState(IMG_SRV_UM, pvHintState);

	bGotMemTrackingApphints = IMG_TRUE;
}	


/***********************************************************************************
 Function Name      : AddToAllocList
 Inputs             : ui32Size, pvTrueAddress, pReturnedAddress, pszFileName,
 					  ui32LineNumber
 Outputs            : 
 Returns            : 
 Description        : Adds a new allocation to the list of all allocations
************************************************************************************/
static IMG_VOID AddToAllocList(IMG_UINT32 ui32Size, IMG_VOID *pvTrueAddress, IMG_VOID *pvReturnedAddress, IMG_CHAR *pszFileName, IMG_UINT32 ui32LineNumber)
{
	sAlloc_Struct *psNewItem;

	psNewItem = malloc(sizeof(sAlloc_Struct));

	if (!psNewItem)
	{
		PVR_DPF((PVR_DBG_ERROR,"AddToAllocList: Failed to allocate memory for alloc structure"));
		return;
	}

	psNewItem->ui32Size 			= ui32Size;
	psNewItem->pvTrueAddress 		= pvTrueAddress;
	psNewItem->pvReturnedAddress 	= pvReturnedAddress;
	psNewItem->ui32LineNumber 		= ui32LineNumber;
	
	strncpy(&psNewItem->szFileName[0], pszFileName, MAX_MEMCHK_FILENAME_LENGTH);

	psNewItem->psNext = psAllocList;
	psNewItem->psPrevious = IMG_NULL;

	if (psAllocList)
	{
		psAllocList->psPrevious = psNewItem;
	}

	/* Insert new item at head of list */
	psAllocList = psNewItem;

	/*
	// Update stats
	*/
	ui32NumAllocs++;

	ui32MemCurrentUsage += ui32Size;

	if (ui32MemCurrentUsage > ui32MemHighWaterMark)
	{
		ui32MemHighWaterMark = ui32MemCurrentUsage;
	}
}


/***********************************************************************************
 Function Name      : RemoveFromAllocList
 Inputs             : psAllocStruct
 Outputs            : 
 Returns            : 
 Description        : Removes the specified allocation structure from
 					  the global list
************************************************************************************/
static IMG_VOID RemoveFromAllocList(sAlloc_Struct *psAllocStruct)
{
	sAlloc_Struct *psItem;

	psItem = psAllocList;

	while (psItem)
	{
		if (psItem == psAllocStruct)
		{
			break;
		}

		psItem = psItem->psNext;
	}

	if (!psItem)
	{
		PVR_DPF((PVR_DBG_ERROR,"RemoveFromAllocList: Item not found in list"));
		return;
	}

	if (psItem->psPrevious)
	{
		psItem->psPrevious->psNext = psItem->psNext;
	}
	
	if (psItem->psNext)
	{
		psItem->psNext->psPrevious = psItem->psPrevious;
	}

	if (psItem==psAllocList)
	{
		psAllocList = psItem->psNext;
	}

	/*
	// Update stats
	*/
	ui32NumAllocs--;

	ui32MemCurrentUsage -= psItem->ui32Size;

	free(psItem);
}


/***********************************************************************************
 Function Name      : FindItemInAllocList
 Inputs             : pvReturnedAddress
 Outputs            : 
 Returns            : 
 Description        : Searches the global allocation list for an entry that
					  contains the specified address
************************************************************************************/
static sAlloc_Struct *FindItemInAllocList(IMG_VOID *pvReturnedAddress)
{
	sAlloc_Struct *psItem;

	psItem = psAllocList;

	while (psItem)
	{
		if (psItem->pvReturnedAddress == pvReturnedAddress)
		{
			break;
		}

		psItem = psItem->psNext;
	}

	if (!psItem)
	{
		PVR_DPF((PVR_DBG_ERROR,"FindItemInAllocList: Item not found in list"));
		return IMG_NULL;
	}

	return psItem;
}


/***********************************************************************************
 Function Name      : OutputMemoryStats
 Inputs             : 
 Outputs            : 
 Returns            : 
 Description        : Displays memory high water mark and any outstanding allocations
************************************************************************************/
IMG_INTERNAL IMG_VOID OutputMemoryStats(IMG_VOID)
{
	PVR_TRACE((""));
	PVR_TRACE(("Memory Stats"));
	PVR_TRACE(("------------"));
	PVR_TRACE((""));
	PVR_TRACE(("High Water Mark = %d bytes", ui32MemHighWaterMark));

	LockAllocList();

	if (psAllocList)
	{
		IMG_UINT32 ui32AllocNumber;
		sAlloc_Struct *psItem;

		psItem = psAllocList;

		PVR_TRACE((""));
		PVR_TRACE(("%u bytes still allocated in %u allocations", ui32MemCurrentUsage, ui32NumAllocs));
		PVR_TRACE((""));

		ui32AllocNumber = 0;

		while (psItem)
		{
			ui32AllocNumber++;

			PVR_TRACE(("%-3u - %5d bytes at %p - %s:%d", ui32AllocNumber, psItem->ui32Size, psItem->pvReturnedAddress, psItem->szFileName, psItem->ui32LineNumber));

			psItem = psItem->psNext;
		}

		PVR_TRACE((""));
	}

	UnlockAllocList();
}	


/**************************************************************************
 * Function Name  : GuardPageAlloc
 * Inputs         : bZeroMemory, ui32SizeInBytes, pszFileName,
 					ui32LineNumber
 * Outputs        : 
 * Returns        : 
 * Description    : Allocates memory and then places a non-readable/writable
 					guard page before/after the allocation
**************************************************************************/
static IMG_PVOID GuardPageAlloc(IMG_BOOL bZeroMemory, IMG_UINT32 ui32SizeInBytes, IMG_CHAR *pszFileName, IMG_UINT32 ui32LineNumber)
{
	IMG_UINT32 ui32SizeToReserve, ui32SizeToCommit;
	IMG_UINT8 *pui8Reserved, *pui8AlignedReserved, *pui8ReturnAddress, *pui8Protected;
	const int iPageSize = sysconf(_SC_PAGESIZE);

	if (!bGotMemTrackingApphints)
	{
		GetMemTrackingApphints();
	}

	if (bUseGuardPage)
	{
		/*
		// Round the allocation up to a multiple of full pages so
		// that the allocation can be moved to the start or end page boundary
		*/
		ui32SizeToCommit = ((ui32SizeInBytes + (iPageSize-1)) & ~(iPageSize-1));

		/*
		// Add one page for the guard page and one page to align the
		// address returned by the malloc() function
		*/
		ui32SizeToReserve = ui32SizeToCommit + (2 * iPageSize);

		pui8Reserved = (IMG_UINT8 *)malloc(ui32SizeToReserve);

		if (!pui8Reserved)
		{
			PVR_DPF((PVR_DBG_ERROR,"GuardPageAlloc: Failed to reserve linear range for alloc"));

			return IMG_NULL;
		}

		/*
		// Deal with aligned pages - this makes things easier as mprotect()
		// requires aligned addresses
		*/
		pui8AlignedReserved = (IMG_UINT8 *)(((IMG_UINT32)pui8Reserved + (iPageSize-1)) & ~(iPageSize-1));

		if (bCheckForOverRuns)
		{
			/*
			// Place the allocation at the end of a page, and set
			// the guard page to be the next page
			*/
			pui8Protected = pui8AlignedReserved + ui32SizeToCommit;

			pui8ReturnAddress = pui8Protected - ui32SizeInBytes;
		}
		else
		{
			/*
			// Place the allocation at the start of a page, and set
			// the guard page to be the previous page
			*/
			pui8Protected = pui8AlignedReserved;

			pui8ReturnAddress = pui8AlignedReserved + iPageSize;
		}

		/*
		// Make the guard page non-readable/writable.  This will cause
		// a seg fault if we read or write outside of our allocation
		*/
		if (mprotect(pui8Protected, iPageSize, PROT_NONE)) 
		{
			PVR_DPF((PVR_DBG_ERROR,"GuardPageAlloc: Couldn't mprotect guard page"));

			free(pui8Reserved);			

			return IMG_NULL;
		}
	}
	else
	{
		/*
		// Just do a normal malloc() if we're not using guard pages
		*/
		pui8Reserved = (IMG_UINT8 *)malloc(ui32SizeInBytes);
		if(!pui8Reserved)
		{
			PVR_DPF((PVR_DBG_ERROR,"GuardPageAlloc: Failed to reserve linear range for alloc"));
			
			return IMG_NULL;
		}
		pui8ReturnAddress = pui8Reserved;
	}


	/* Zero the memory for calloc() calls */
	if (bZeroMemory)
	{
		memset(pui8ReturnAddress, 0, ui32SizeInBytes);
	}

	AddToAllocList(ui32SizeInBytes, pui8Reserved, pui8ReturnAddress, pszFileName, ui32LineNumber);

	return (IMG_VOID *)pui8ReturnAddress;	
}


/**************************************************************************
 * Function Name  : GuardPageFree
 * Inputs         : pvMem
 * Outputs        : 
 * Returns        : 
 * Description    : Searches for the specified address in the global
					allocation list and, if found, deallocates it and
					removes it from the list
**************************************************************************/
static IMG_VOID GuardPageFree(IMG_VOID *pvMem)
{
	const int iPageSize = sysconf(_SC_PAGESIZE);
	sAlloc_Struct *psAllocStruct;

	if(!pvMem)
	{
		/* The standard says that NULL is ignored. Avoid iterating the list */
		return;
	}

	psAllocStruct=FindItemInAllocList(pvMem);

	if (!psAllocStruct)
	{
		PVR_DPF((PVR_DBG_ERROR,"GuardPageFree: Failed"));
		return;
	}

	if (bUseGuardPage)
	{
		IMG_UINT8 *pui8Protected;

		if (bCheckForOverRuns)
		{
			/*
			// Allocation at the end of a page, 
			// the guard page at the next page
			*/
			pui8Protected = (IMG_UINT8 *)psAllocStruct->pvReturnedAddress + psAllocStruct->ui32Size;
		}
		else
		{
			/*
			// Allocation at the start of a page, 
			// the guard page at the previous page
			*/
			pui8Protected  = (IMG_UINT8 *)psAllocStruct->pvReturnedAddress - iPageSize;
		}
		if (mprotect(pui8Protected, iPageSize, PROT_WRITE|PROT_READ|PROT_EXEC)) 
		{
			PVR_DPF((PVR_DBG_ERROR,"GuardPageFree: Couldn't un mprotect guard page"));
			return;
		}
	}

	free(psAllocStruct->pvTrueAddress);
	RemoveFromAllocList(psAllocStruct);
}


/**************************************************************************
 * Function Name  : GuardPageReAlloc
 * Inputs         : pvMem, ui32SizeInBytes, pszFileName, ui32LineNumber
 * Outputs        : 
 * Returns        : 
 * Description    : Equivalent functionality to the system realloc()
					function but makes use of the GuardPageAlloc and 
					GuardPageFree functions.
**************************************************************************/
static IMG_PVOID GuardPageReAlloc(IMG_VOID *pvMem, IMG_UINT32 ui32SizeInBytes, IMG_CHAR *pszFileName, IMG_UINT32 ui32LineNumber)
{
	sAlloc_Struct *psAllocStruct;

	IMG_UINT8 *pui8NewAlloc;

	/* ReAlloc of 0 acts like a free */
	if((ui32SizeInBytes == 0) && (pvMem != IMG_NULL))
	{
		GuardPageFree(pvMem);

		return IMG_NULL;
	}

	pui8NewAlloc = (IMG_UINT8 *)GuardPageAlloc(IMG_FALSE, ui32SizeInBytes, pszFileName, ui32LineNumber);

	if (!pui8NewAlloc)
	{
		PVR_DPF((PVR_DBG_ERROR,"GuardPageReAlloc: Failed to create new buffer"));

		return IMG_NULL;
	}

	/* If an old allocation exists - copy the contents to the new allocation */
	if (pvMem)
	{
		psAllocStruct=FindItemInAllocList(pvMem);

		if (psAllocStruct)
		{
			if (psAllocStruct->ui32Size > ui32SizeInBytes)
			{
				memcpy(pui8NewAlloc, pvMem, ui32SizeInBytes);
			}
			else
			{
				memcpy(pui8NewAlloc, pvMem, psAllocStruct->ui32Size);
			}

			GuardPageFree(pvMem);
		}
	}

	return (IMG_VOID *)pui8NewAlloc;
}


/**************************************************************************
 * Function Name  : PVRSRVAllocUserModeMemTracking
 * Inputs         : ui32Size, pszFileName, ui32LineNumber
 * Outputs        : 
 * Returns        : 
 * Description    : Wrapper function for malloc()
**************************************************************************/
IMG_EXPORT IMG_PVOID PVRSRVAllocUserModeMemTracking(IMG_UINT32 ui32Size, IMG_CHAR *pszFileName, IMG_UINT32 ui32LineNumber)
{
	IMG_PVOID pvRes;

	LockAllocList();

	pvRes = GuardPageAlloc(IMG_FALSE, ui32Size, pszFileName, ui32LineNumber);

	UnlockAllocList();

	return pvRes;
}


/**************************************************************************
 * Function Name  : PVRSRVCallocUserModeMemTracking
 * Inputs         : ui32Size, pszFileName, ui32LineNumber
 * Outputs        : 
 * Returns        : 
 * Description    : Wrapper function for calloc()
**************************************************************************/
IMG_EXPORT IMG_PVOID PVRSRVCallocUserModeMemTracking(IMG_UINT32 ui32Size, IMG_CHAR *pszFileName, IMG_UINT32 ui32LineNumber)
{	
	IMG_PVOID pvRes;

	LockAllocList();

	pvRes = GuardPageAlloc(IMG_TRUE, ui32Size, pszFileName, ui32LineNumber);

	UnlockAllocList();

	return pvRes;
}


/**************************************************************************
 * Function Name  : PVRSRVFreeUserModeMemTracking
 * Inputs         : pvMem
 * Outputs        : 
 * Returns        : 
 * Description    : Wrapper function for free()
**************************************************************************/
IMG_EXPORT IMG_VOID PVRSRVFreeUserModeMemTracking(IMG_VOID *pvMem)
{
	LockAllocList();

	GuardPageFree(pvMem);

	UnlockAllocList();
}


/**************************************************************************
 * Function Name  : PVRSRVReallocUserModeMemTracking
 * Inputs         : pvMem, ui32NewSize, pszFileName, ui32LineNumber
 * Outputs        : 
 * Returns        : 
 * Description    : Wrapper function for realloc()
**************************************************************************/
IMG_EXPORT IMG_PVOID PVRSRVReallocUserModeMemTracking(IMG_VOID *pvMem, IMG_UINT32 ui32NewSize, IMG_CHAR *pszFileName, IMG_UINT32 ui32LineNumber)
{
	IMG_PVOID pvRes;

	LockAllocList();

	pvRes = GuardPageReAlloc(pvMem, ui32NewSize, pszFileName, ui32LineNumber);

	UnlockAllocList();

	return pvRes;
}

#endif /* DEBUG */
