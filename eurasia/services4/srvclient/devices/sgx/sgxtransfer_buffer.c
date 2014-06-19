/*!****************************************************************************
@File           sgxtransfer_buffer.c

@Title         	Transfer queue circular buffer management

@Author         Imagination Technologies

@date          	06/10/09

@Copyright      Copyright 2009 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either
                material or conceptual may be copied or distributed,
                transmitted, transcribed, stored in a retrieval system
                or translated into any human or computer language in any
                form by any means, electronic, mechanical, manual or
                other-wise, or disclosed to third parties without the
                express written permission of Imagination Technologies
                Limited, Unit 8, HomePark Industrial Estate,
                King's Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       Generic

@Description   	Transfer queue circular buffer management

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-
$Log: sgxtransfer_buffer.c $
*****************************************************************************/


#include "sgxtransfer_buffer.h"
#include "sgxmmu.h"
#include "pvr_debug.h"
#include "pdump_um.h"
#include "servicesint.h"
#include "osfunc_client.h"
#include <stddef.h>
#include "img_types.h"
#include "services.h"
#include "servicesext.h"
#include "sgxapi.h"
#include "pvr_bridge.h"
#include "sgxinfo.h"
#include "sgxinfo_client.h"
#include "sgxtransfer_client.h"

/* these macros are for internal use only */
#define	SGXTQ_ROUNDEDSIZE(psCB, size)									\
	(((size) + (((psCB)->ui32Alignment) - 1)) & ~((psCB)->ui32Alignment - 1))

#define SGXTQ_CBPACKETOFFNEXT(off)	(((off) + 1) & (SGXTQ_MAX_QUEUED_BLITS - 1))

/*****************************************************************************
 * Function Name		:	SGXTQ_CreateCB
 * Inputs				:	psDevData		- the device data
 *							ui32CBSize		- buffer size in bytes
 *							ui32Alignment	- in buffer allocation alignment (byte)
 *							bAllowPageBr	- allow buffer allocations to cross page boundaries
 *							bAllowWrite		- set to IMG_FALSE for buffers never written by the hw
 *							hDevMemHeap		- heap handle
 *							pbyLabel	- only in PDUMP builds : small label for PDUMP comments
 * Outputs				:	ppsCB			- pointer to the created CB
 * Returns				:	error code on error
 * Description			:	Creates a packet based circular buffer.
******************************************************************************/
IMG_INTERNAL PVRSRV_ERROR SGXTQ_CreateCB(PVRSRV_DEV_DATA	*psDevData,
										 IMG_HANDLE hTQContext,
										 IMG_UINT32			ui32CBSize,
										IMG_UINT32			ui32Alignment,
										IMG_BOOL			bAllowPageBr,
										IMG_BOOL			bAllowWrite,
#if defined (SUPPORT_SID_INTERFACE)
										IMG_SID				hDevMemHeap,
#else
										IMG_HANDLE			hDevMemHeap,
#endif
										IMG_CHAR			*pbyLabel,
										SGXTQ_CB			**ppsCB)
{
	SGXTQ_CB		*psCB;

	if (IMG_NULL == (psCB = PVRSRVAllocUserModeMem(sizeof(SGXTQ_CB))))
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	/* Setup CB struct */
	PVRSRVMemSet(psCB, IMG_NULL, sizeof(SGXTQ_CB));

	PVR_ASSERT(((ui32CBSize & (ui32Alignment - 1)) == 0) &&
				(((ui32Alignment - 1) & ui32Alignment) == 0));

	/* Alignment must be between 0 and SGX_MMU_PAGE_SIZE */
	PVR_ASSERT(ui32Alignment <= SGX_MMU_PAGE_SIZE);

	psCB->ui32Alignment = ui32Alignment;
	psCB->ui32BufferSize = ui32CBSize;
	psCB->bAllowPageBr = bAllowPageBr;
	psCB->psDevData = psDevData;
	psCB->pbyLabel = pbyLabel;

#if defined(PDUMP)
	/* allocate the CB */
	PDUMPCOMMENTWITHFLAGSF(psDevData->psConnection,
		PDUMP_FLAGS_CONTINUOUS,
		"Allocate TQ %s CB",
		psCB->pbyLabel);
#endif

	if (ALLOCDEVICEMEM(hTQContext,
					   hDevMemHeap,
					   PVRSRV_MEM_READ						|
					   (bAllowWrite ? PVRSRV_MEM_WRITE : 0)|
					   PVRSRV_MEM_NO_SYNCOBJ				|
					   PVRSRV_MEM_CACHE_CONSISTENT,
					   ui32CBSize,
					   SGX_MMU_PAGE_SIZE,
					   &psCB->psBufferMemInfo) != PVRSRV_OK)
	{
		PVRSRVFreeUserModeMem(psCB);
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	/* Save reference to newly created CB */
	*ppsCB = psCB;
	return PVRSRV_OK;
}


/*****************************************************************************
 * Function Name		:	SGXTQ_DestroyCB
 * Inputs				:	psCB		- pointer to the CB
 * Description			:	Deletes a packet based circular buffer.
******************************************************************************/
IMG_INTERNAL IMG_VOID SGXTQ_DestroyCB(IMG_HANDLE hTQContext,
									  SGXTQ_CB *psCB)
{
	FREEDEVICEMEM(hTQContext, psCB->psBufferMemInfo);
	PVRSRVFreeUserModeMem(psCB);
}

static IMG_VOID SGXTQ_PrintBufferState(SGXTQ_CB * psCB, IMG_UINT32 ui32SampledFence)
{
	IMG_UINT32 i;

	PVR_UNREFERENCED_PARAMETER(ui32SampledFence);

	PVR_DPF((PVR_DBG_ERROR, "TQ %s state @ fence %8.8x", psCB->pbyLabel, ui32SampledFence));
	PVR_DPF((PVR_DBG_ERROR, "Woff : %8.8x NCWoff : %8.8x Roff : %8.8x",
			psCB->ui32Woff, psCB->ui32NCWoff, psCB->ui32Roff));

	PVR_DPF((PVR_DBG_ERROR, "PWoff : %d PNCWoff : %d PRoff : %d",
			psCB->ui32PacketWoff, psCB->ui32PacketNCWoff, psCB->ui32PacketRoff));

	for (i = psCB->ui32PacketRoff; i < psCB->ui32PacketWoff; i = SGXTQ_CBPACKETOFFNEXT(i))
	{
		PVR_DPF((PVR_DBG_ERROR, "[Commited] Fenced : %8.8x Roff : %8.8x",
			psCB->asCBPackets[i].ui32FenceID, psCB->asCBPackets[i].ui32Roff));
	}
	for (i = psCB->ui32PacketWoff; i < psCB->ui32PacketNCWoff; i = SGXTQ_CBPACKETOFFNEXT(i))
	{
		PVR_DPF((PVR_DBG_ERROR, "[Non-Commited] Fenced : %8.8x Roff : %8.8x",
			psCB->asCBPackets[i].ui32FenceID, psCB->asCBPackets[i].ui32Roff));
	}
}


/*****************************************************************************
 * Function Name		:	SGXTQ_QueryCB
 * Inputs				:	pui32FenceID		- the fence ID meminfo. any packets with
 *												  fenceid less than the pointed value are considered
 *												  as ready to be freed
 *							hOSevent			- if not IMG_NULL this is an Event object handle
 *							psCB				- pointer to the CB
 *							ui32Size			- allocation request size in bytes
 *							bPreparedForWait	- is the calling code prepared for waiting
 * Returns				:	true if the buffer has *ui32Size* bytes
 * Description			:	this is meant to be internal (static)
******************************************************************************/
static IMG_BOOL SGXTQ_QueryCB(PVRSRV_CLIENT_MEM_INFO	* psFenceIDMemInfo,
#if defined (SUPPORT_SID_INTERFACE)
									IMG_EVENTSID		hOSEvent,
#else
									IMG_HANDLE			hOSEvent,
#endif
									SGXTQ_CB			* psCB,
									IMG_UINT32			ui32Size,
									IMG_BOOL			bPreparedForWait,
									IMG_BOOL			bPDumpContinuous)
{
	IMG_BOOL	bFound = IMG_FALSE;
	IMG_UINT32	ui32WaitCount = WAIT_TRY_COUNT;
	IMG_BOOL	bErrorReported = IMG_FALSE;

	/* use up one granuality more so the Woff can't end up on the top of the Roff
	 * making the full buffer empty */
	ui32Size = SGXTQ_ROUNDEDSIZE(psCB, ui32Size) + psCB->ui32Alignment;

	while (ui32WaitCount > 0)
	{
		/* we have to sample it and use the value from the stack to avoid invalid
		 * calculations when the pointed value changes
		 */
		IMG_UINT32 ui32SampledFence = * ((volatile IMG_UINT32*)psFenceIDMemInfo->pvLinAddr);

		ui32WaitCount--;

		/* there is at least 1 packet */
		while (psCB->ui32PacketNCWoff != psCB->ui32PacketRoff)
		{
			SGXTQ_CB_PACKET* psPacket = & psCB->asCBPackets[psCB->ui32PacketRoff];

			if (psPacket->ui32FenceID < ui32SampledFence ||
				psPacket->ui32FenceID - ui32SampledFence > 0x80000000)
			{
				psCB->ui32Roff = psPacket->ui32Roff;
				psCB->ui32PacketRoff = SGXTQ_CBPACKETOFFNEXT(psCB->ui32PacketRoff);
#if defined(PDUMP)
				if (psPacket->bWasDumped)
				{
					PVRSRV_DEV_DATA			*psDevData = psCB->psDevData;
					const PVRSRV_CONNECTION *psConnection = psDevData->psConnection;
					IMG_UINT32				ui32PDumpFlags = (bPDumpContinuous ? PDUMP_FLAGS_CONTINUOUS : 0);

					/* poll the ssim host til this allocation is being used by previous blits*/

					PDUMPCOMMENTWITHFLAGSF(psConnection,
							ui32PDumpFlags,
							"Poll on TQ %s CB offs 0x%x 0x%x bytes to be released - fenced %x",
							psCB->pbyLabel,
							psPacket->ui32Offset,
							psPacket->ui32PacketSize,
							psPacket->ui32FenceID);
					/* this doesn't work if the 32 bit fence id has wrapped , but that's not a
					 * real limit in PDUMPing. Pol until
					 * psPacket->ui32FenceID < * psFenceIDMemInfo->pvLinAddr
					 */
#if defined (SUPPORT_SID_INTERFACE)
					PDUMPMEMPOLWITHOP(psConnection,
							psFenceIDMemInfo->hKernelMemInfo,
							0,
							psPacket->ui32FenceID, 
							0xffffffff,
							PDUMP_POLL_OPERATOR_GREATER, 
							ui32PDumpFlags);
#else
					PDUMPMEMPOLWITHOP(psConnection,
							psFenceIDMemInfo,
							0,
							psPacket->ui32FenceID, 
							0xffffffff,
							PDUMP_POLL_OPERATOR_GREATER, 
							ui32PDumpFlags);
#endif
				}
#else
				PVR_UNREFERENCED_PARAMETER(bPDumpContinuous);
#endif
			}
			else
			{
				break; /* nothing to read*/
			}
		}

		/* check if (NCwoff + 1) is free to go*/
		if (! bFound && (SGXTQ_CBPACKETOFFNEXT(psCB->ui32PacketNCWoff) != psCB->ui32PacketRoff))
		{
			bFound = IMG_TRUE;
		}

		if (bFound)
		{
			/* we have a packet, do we have the space?! */
			IMG_UINT32 ui32SpaceAvailable;

			if ( (psCB->ui32BufferSize - psCB->ui32NCWoff < ui32Size) &&
                 (psCB->ui32Roff > ui32Size) &&
                 (psCB->ui32Roff <= psCB->ui32Woff) )
			{
				/* it fits to the front (buffer may have allocations at the end)*/
				psCB->ui32NCWoff = 0;
			}
			else if(psCB->ui32Roff == psCB->ui32Woff && psCB->ui32Roff == psCB->ui32NCWoff)
			{
				/* buffer is empty*/
				psCB->ui32NCWoff = 0;
				psCB->ui32Woff = 0;
				psCB->ui32Roff = 0;
			}


			if (! psCB->bAllowPageBr)
			{
				/* if we would cross a page boundary go up to the start of the next page,
				 * but don't step out of the Buffer (buffer base must be page aligned)*/
				if ((psCB->ui32NCWoff & ~SGX_MMU_PAGE_MASK) != ((psCB->ui32NCWoff + ui32Size) & ~SGX_MMU_PAGE_MASK))
				{
					IMG_UINT32 ui32NextPage = (psCB->ui32NCWoff + SGX_MMU_PAGE_MASK) & ~SGX_MMU_PAGE_MASK;
					if (ui32NextPage < psCB->ui32BufferSize)
					{
						psCB->ui32NCWoff = ui32NextPage;
					}
					else if (psCB->ui32Roff > ui32Size)
					{
						psCB->ui32NCWoff = 0;
					}
					else
					{
						if (bPreparedForWait)
						{
							/* keep looping don't accept this one even if it fits*/
							continue;
						}
						else
						{
							/* this would always return false if ui32Size >= SGX_MMU_PAGE_SIZE*/
							PVR_ASSERT(ui32Size < SGX_MMU_PAGE_SIZE);
							return IMG_FALSE;
						}
					}
				}
			}

			if (psCB->ui32Roff <= psCB->ui32NCWoff)
			{
				ui32SpaceAvailable = psCB->ui32BufferSize - psCB->ui32NCWoff;
			}
			else
			{
				ui32SpaceAvailable = psCB->ui32Roff - psCB->ui32NCWoff;
			}

			if (ui32SpaceAvailable >= ui32Size)
			{
				return IMG_TRUE;
			}
			else if (! bPreparedForWait)
			{
				return IMG_FALSE;
			}
		}
		if ((WAIT_TRY_COUNT - ui32WaitCount > 100) && ! bErrorReported)
		{
			bErrorReported = IMG_TRUE;
			PVR_DPF((PVR_DBG_ERROR, "TQ %s stuck. Printing state", psCB->pbyLabel));
			SGXTQ_PrintBufferState(psCB, ui32SampledFence);
		}
		/* we failed in this round, wait some time and try again*/
#if defined (SUPPORT_SID_INTERFACE)
		if (hOSEvent != 0)
#else
		if (hOSEvent != IMG_NULL)
#endif
		{
			if (PVRSRVEventObjectWait(psCB->psDevData->psConnection,
						hOSEvent) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_MESSAGE, "SGXTQ_AcquireCB: PVRSRVEventObjectWait failed"));
			}
		}
		else
		{
			PVRSRVWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
		}
	}
	return IMG_FALSE;
}


/*****************************************************************************
 * Function Name		:	SGXTQ_AcquireCB
 * Inputs				:	psFenceIDMemInfo	- the fence ID mem info. any packets with
 *												  fenceid less than the pointed value are considered
 *												  as ready to be freed
 *							ui32CurrentFence	- the value to place in new packets on write
 *							hOSevent			- if not IMG_NULL this is an Event object handle
 *							psCB				- pointer to the CB
 *							ui32Size			- allocation request size in bytes
 *							bPreparedForWait	- is the calling code prepared for waiting
 * Outputs				:	ppvLinAddr			- if not IMG_NULL pointed value will be set to
 *												  the allocation ptr (in um linear space)
 *							pui32DevVAddr		- if not IMG_NULL pointed value will be set to
 *												  the allocation ptr (in device virtual space)
 * Returns				:	true if the buffer has *ui32Size* bytes
 * Description			:	if the buffer has enough space, this allocates the
 *							reuired amount otherwise returns false with the buffer
 *							untouched
******************************************************************************/
IMG_INTERNAL IMG_BOOL SGXTQ_AcquireCB(PVRSRV_CLIENT_MEM_INFO	* psFenceIDMemInfo,
									IMG_UINT32					ui32CurrentFence,
#if defined (SUPPORT_SID_INTERFACE)
									IMG_EVENTSID				hOSevent,
#else
									IMG_HANDLE					hOSevent,
#endif
									SGXTQ_CB					* psCB,
									IMG_UINT32					ui32Size,
									IMG_BOOL					bPreparedForWait,
									IMG_VOID**					ppvLinAddr,
									IMG_PUINT32					pui32DevVAddr,
									IMG_BOOL					bPDumpContinuous)
{
	IMG_BOOL		bHaveSpace;
	SGXTQ_CB_PACKET	* psCBPacket;

	bHaveSpace = SGXTQ_QueryCB(psFenceIDMemInfo,
			hOSevent,
			psCB,
			ui32Size,
			bPreparedForWait,
			bPDumpContinuous);

	if (! bHaveSpace)
		return IMG_FALSE;


	psCBPacket = &psCB->asCBPackets[psCB->ui32PacketNCWoff];
	psCBPacket->ui32FenceID = ui32CurrentFence;

#if defined(PDUMP)
	psCBPacket->ui32Offset = psCB->ui32NCWoff;
	psCBPacket->ui32PacketSize = ui32Size;
#endif

	/* get the information for the client that it needs about the allocation*/
	if (ppvLinAddr != IMG_NULL)
	{
		(*ppvLinAddr) = (IMG_VOID*)(((IMG_PBYTE)psCB->psBufferMemInfo->pvLinAddr) +
				psCB->ui32NCWoff);
	}
	if (pui32DevVAddr != IMG_NULL)
	{
		(*pui32DevVAddr) = (psCB->psBufferMemInfo->sDevVAddr.uiAddr + psCB->ui32NCWoff);
	}

	/* and now advance the NC offsets*/
	psCB->ui32NCWoff += SGXTQ_ROUNDEDSIZE(psCB, ui32Size);
	psCB->ui32PacketNCWoff = SGXTQ_CBPACKETOFFNEXT(psCB->ui32PacketNCWoff);

	/* and save where to update the Roff when the packets fence ID is passed*/
	psCBPacket->ui32Roff = psCB->ui32NCWoff;

	return IMG_TRUE;
}


/*****************************************************************************
 * Function Name		:	SGXTQ_BeginCB
 * Inputs				:	ppsCB			- pointer to the CB
 * Description			:	this resets the state of the buffer dropping all allocation
 *							since the last SGXTQ_FlushCB()
******************************************************************************/
IMG_INTERNAL IMG_VOID SGXTQ_BeginCB(SGXTQ_CB *psCB)
{
	/* clear the non commited*/
	psCB->ui32NCWoff = psCB->ui32Woff;
	psCB->ui32PacketNCWoff = psCB->ui32PacketWoff;
}


/*****************************************************************************
 * Function Name		:	SGXTQ_FlushCB
 * Inputs				:	psCB				- pointer to the CB
 *							bPDumpContinuous	- when PDUMPing set the continous flag
 * Description			:	this function flushes the buffer, by commiting - and PDUMPing -
 *							all allocations since the last SGXTQ_BeginCB.
 *							Once the buffer is flushed it's given to the hw, so the client
 *							must make sure that the Fence ID associted with the outstanding
 *							allocation inside the buffer will be passed.
******************************************************************************/
IMG_INTERNAL IMG_VOID SGXTQ_FlushCB(SGXTQ_CB	*psCB,
									IMG_BOOL	bPDumpContinuous)
{
#if ! defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(bPDumpContinuous);
#else
	IMG_UINT32 		i = psCB->ui32PacketWoff;
	PVRSRV_DEV_DATA *psDevData = psCB->psDevData;

	while (i != psCB->ui32PacketNCWoff)
	{
		SGXTQ_CB_PACKET * psPacket = & psCB->asCBPackets[i];
		IMG_UINT32	ui32PDumpFlags = (bPDumpContinuous ? PDUMP_FLAGS_CONTINUOUS : 0);
		const PVRSRV_CONNECTION *psConnection = psDevData->psConnection;

		psPacket->bWasDumped = (PDUMPISCAPTURINGTEST(psConnection)
										|| bPDumpContinuous) ? IMG_TRUE : IMG_FALSE;

		PDUMPCOMMENTWITHFLAGSF(psConnection, ui32PDumpFlags, "TQ %s CB allocation",
				psCB->pbyLabel);
		PDUMPCOMMENTWITHFLAGSF(psConnection, ui32PDumpFlags, "  DevVAddr == 0x%08x, buffer size == 0x%x",
				psCB->psBufferMemInfo->sDevVAddr.uiAddr,
				psCB->psBufferMemInfo->uAllocSize);
		PDUMPCOMMENTWITHFLAGSF(psConnection, ui32PDumpFlags, "  PacketOffset == 0x%x",
				psPacket->ui32Offset);
		PDUMPCOMMENTWITHFLAGSF(psConnection, ui32PDumpFlags, "  PacketSize == 0x%x",
				psPacket->ui32PacketSize);
		PDUMPMEM(psConnection,
				IMG_NULL,
				psCB->psBufferMemInfo,
				psPacket->ui32Offset,
				psPacket->ui32PacketSize,
				ui32PDumpFlags);

		i = SGXTQ_CBPACKETOFFNEXT(i);
		/* if we step on the top of the Roff then something went wrong*/
		PVR_ASSERT(i != psCB->ui32PacketRoff);
	}
#endif
	/* commit */
	psCB->ui32PacketWoff = psCB->ui32PacketNCWoff;
	psCB->ui32Woff = psCB->ui32NCWoff;
}

/******************************************************************************
 End of file (sgxtransfer_buffer.c)
******************************************************************************/
