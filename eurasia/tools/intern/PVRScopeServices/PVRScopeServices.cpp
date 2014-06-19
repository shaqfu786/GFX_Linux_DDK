/*****************************************************************************
$Revision: 1.21 $
*****************************************************************************/

/****************************************************************************
** Build options
****************************************************************************/

/****************************************************************************
** Includes
****************************************************************************/
#include "PVRScopeServices.h"

#include "services.h"
#include "sgxapi.h"

#define _ASSERT(X)

/****************************************************************************
** Defines
****************************************************************************/

/* Size of the array to fill with device IDs */
#define MAX_NUM_DEVICE_IDS	(32)

/****************************************************************************
** Structures
****************************************************************************/

// Internal implementation data
struct SPVRSSData
{
	PVRSRV_CONNECTION			*psConnection;
	PVRSRV_DEV_DATA				s3DDevData;
	SPVRSSInfo2					sInfo2;
	SPVRSSInfo3					sInfo3;
	SPVRSSInfo4					sInfo4;

	PVRSRV_SGX_MISCINFO_SET_HWPERF_STATUS	sHWPerfStatus;

	PVRSRV_SGX_HWPERF_CB_ENTRY	asCBEntry[0x100];
	IMG_UINT32					ui32CBCount;
	IMG_UINT32					ui32CBClockSpeed, ui32CBHostTimeStamp;

	// Used to implement PVRSSReadPerfCountersThenSetGroup2
	IMG_UINT32					ui32CBSkippedPackets;
	SPVRSSPerfCountersReading	sFakePerfCountersReading;
};

/****************************************************************************
** Local code
****************************************************************************/
static
unsigned int StringLength(const char * const pszSrc, const unsigned int nMaxLen)
{
	unsigned int nLen, nLimit = nMaxLen - 1;

	for(nLen = 0; pszSrc[nLen] != '\0' && nLen < nLimit; ++nLen);
	++nLen;	// count the \0

	return nLen;
}

static
void StringDuplicate(const char *&pszDst, const char * const pszSrc, const unsigned int nLen)
{
	unsigned int	i;
	char			*pszCopy;

	pszCopy = new char[nLen];
	if(pszCopy)
	{
		for(i = 0; i < nLen; ++i)
		{
			pszCopy[i] = pszSrc[i];
		}
	}

	pszDst = pszCopy;
}

static bool TimingRetrieve(
	SPVRSSData		* const psData)
{
	const IMG_UINT32	ui32Space = sizeof(psData->asCBEntry) / sizeof(*psData->asCBEntry) - psData->ui32CBCount;
	IMG_UINT32			ui32DataCount, ui32ClockSpeed, ui32HostTimeStamp;

	/*
		Get more packets of data from Services, on the end of the existing list
	*/
	PVRSRV_ERROR eResult = SGXReadHWPerfCB(
		&psData->s3DDevData,
		ui32Space, &psData->asCBEntry[psData->ui32CBCount],
		&ui32DataCount, &ui32ClockSpeed, &ui32HostTimeStamp);

	if(eResult != PVRSRV_OK)
	{
		return false;
	}

	psData->ui32CBCount			+= ui32DataCount;
	psData->ui32CBClockSpeed	= ui32ClockSpeed;
	psData->ui32CBHostTimeStamp	= ui32HostTimeStamp;
	return true;
}

static unsigned int SumReadings(
	const IMG_UINT32 aaui32[SGX_FEATURE_MP_CORE_COUNT_3D][PVRSRV_SGX_HWPERF_NUM_COUNTERS],
	const IMG_UINT32 ui32Counter)
{
	unsigned int nCoreIdx, nRet = 0;

	for(nCoreIdx = 0; nCoreIdx < SGX_FEATURE_MP_CORE_COUNT_3D; ++nCoreIdx)
	{
		nRet += aaui32[nCoreIdx][ui32Counter];
	}
	return nRet;
}

static void TimingFakeCounterReading(
	SPVRSSData		* const psData)
{
	psData->sFakePerfCountersReading.nSW_TIME = psData->ui32CBHostTimeStamp;

	if(psData->ui32CBCount)
	{
		// Update with latest data from HW
		psData->sFakePerfCountersReading.nr0	= SumReadings(psData->asCBEntry[psData->ui32CBCount-1].ui32Counters, 0);
		psData->sFakePerfCountersReading.nr1	= SumReadings(psData->asCBEntry[psData->ui32CBCount-1].ui32Counters, 1);
		psData->sFakePerfCountersReading.nr2	= SumReadings(psData->asCBEntry[psData->ui32CBCount-1].ui32Counters, 2);
		psData->sFakePerfCountersReading.nr3	= SumReadings(psData->asCBEntry[psData->ui32CBCount-1].ui32Counters, 3);
		psData->sFakePerfCountersReading.nr4	= SumReadings(psData->asCBEntry[psData->ui32CBCount-1].ui32Counters, 4);
		psData->sFakePerfCountersReading.nr5	= SumReadings(psData->asCBEntry[psData->ui32CBCount-1].ui32Counters, 5);
		psData->sFakePerfCountersReading.nr6	= SumReadings(psData->asCBEntry[psData->ui32CBCount-1].ui32Counters, 6);
		psData->sFakePerfCountersReading.nr7	= SumReadings(psData->asCBEntry[psData->ui32CBCount-1].ui32Counters, 7);
		psData->sFakePerfCountersReading.nr8	= SumReadings(psData->asCBEntry[psData->ui32CBCount-1].ui32Counters, 8);
#ifdef EUR_CR_TIMER
		psData->sFakePerfCountersReading.ntimer	= psData->asCBEntry[psData->ui32CBCount-1].ui32Clocksx16;
#endif
	}
}

static bool SetHWPerfStatus(
	SPVRSSData		* const psData)
{
	SGX_MISC_INFO sSGXMiscInfo;

	_ASSERT(psData);

	// Prepare to call into Services
	sSGXMiscInfo.eRequest				= SGX_MISC_INFO_REQUEST_SET_HWPERF_STATUS;
	sSGXMiscInfo.uData.sSetHWPerfStatus	= psData->sHWPerfStatus;

	// Call into Services
	PVRSRV_ERROR eResult = SGXGetMiscInfo(&psData->s3DDevData, &sSGXMiscInfo);

	if(eResult == PVRSRV_OK)
	{
#if defined(SGX_FEATURE_EXTENDED_PERF_COUNTERS)
		/* FIXME */
#else
		psData->sFakePerfCountersReading.nSW_ACTIVE_GROUP = psData->sHWPerfStatus.ui32PerfGroup;
#endif /* SGX_FEATURE_EXTENDED_PERF_COUNTERS */
		return true;
	}
	return false;
}

/****************************************************************************
** Entry-point: Initialise
****************************************************************************/
static
SPVRSSData * IMG_CALLCONV PVRSSInit()
{
	PVRSRV_ERROR	eResult;
	SPVRSSData		*psData;

	psData = new SPVRSSData;
	if(!psData)
	{
		return 0;
	}

	/*
		Allocate and copy the device name
	*/
	{
		unsigned int	nLen;
		const char		*pszName;

		pszName = SGX_CORE_FRIENDLY_NAME;

		// Arbitrary max length here, as it shouldn't fail
		nLen = StringLength(pszName, 1024);

		StringDuplicate(psData->sInfo2.pszDriverDeviceName, pszName, nLen);
	}

	/*
		Allocate and copy the device revision
	*/
	psData->sInfo2.nDriverDeviceRevision = SGX_CORE_REV;

	/*
		Connect to Services
	*/
	{
		IMG_UINT32					uiNumDevices;
		PVRSRV_DEVICE_IDENTIFIER	asDevID[MAX_NUM_DEVICE_IDS];
		IMG_UINT32					i;
		PVRSRV_DEV_DATA				asDevData[MAX_NUM_DEVICE_IDS], *ps3DDevData;
//		PVRSRV_SGX_CLIENT_INFO		sSGXInfo;

		ps3DDevData = IMG_NULL;

		eResult = PVRSRVConnect(&psData->psConnection, 0);
		if(eResult != PVRSRV_OK)
		{
			delete psData;
			return 0;
		}

		// Find a device
		eResult = PVRSRVEnumerateDevices(psData->psConnection,
										&uiNumDevices,
										asDevID);
		if(eResult != PVRSRV_OK)
		{
			return 0;
		}

		/* Get each device... */
		for (i = 0; i < uiNumDevices; i++)
		{
			/*
				Only get services managed devices.
				Display Class API handles external display devices
			 */
			if (asDevID[i].eDeviceType != PVRSRV_DEVICE_TYPE_EXT)
			{
				PVRSRV_DEV_DATA *psDevData = asDevData + i;

				eResult = PVRSRVAcquireDeviceData ( psData->psConnection,
													asDevID[i].ui32DeviceIndex,
													psDevData,
													PVRSRV_DEVICE_TYPE_UNKNOWN);
				if(eResult != PVRSRV_OK)
				{
					return 0;
				}

				/*
					Print out details about the SGX device.
					At the enumeration stage you should get back the device info
					from which you match a devicetype with index, i.e. we should
					know what index SGX device is and test for it now.
				*/
				if (asDevID[i].eDeviceType == PVRSRV_DEVICE_TYPE_SGX)
				{
					if(ps3DDevData != IMG_NULL)
					{
						// Hmm, another SGX found.
					}

					/* save off 3d devdata */
					ps3DDevData = psDevData;

//					eResult = SGXGetClientInfo(psDevData, &sSGXInfo);
					if(eResult != PVRSRV_OK)
					{
						return 0;
					}
				}
			}
		}

		if(ps3DDevData == IMG_NULL)
		{
			return 0;
		}

		// Store the device data
		psData->s3DDevData = *ps3DDevData;
	}

	/*
		Allocate and copy the driver name
	*/
	{
		PVRSRV_MISC_INFO	sMiscInfo;
		char				aszDriverName[1024];

		*aszDriverName = 0;
		sMiscInfo.ui32StateRequest	= PVRSRV_MISC_INFO_DDKVERSION_PRESENT;
		sMiscInfo.pszMemoryStr		= aszDriverName;
		sMiscInfo.ui32MemoryStrLen	= sizeof(aszDriverName) / sizeof(*aszDriverName);
		PVRSRVGetMiscInfo(psData->psConnection, &sMiscInfo);

		// Copy the name somewhere safe
		if((sMiscInfo.ui32StatePresent & PVRSRV_MISC_INFO_DDKVERSION_PRESENT) && *aszDriverName)
		{
			unsigned int nLen = StringLength(aszDriverName, sizeof(aszDriverName) / sizeof(*aszDriverName));
			StringDuplicate(psData->sInfo2.pszDriverName, aszDriverName, nLen);
		}
		else
		{
			psData->sInfo2.pszDriverName = 0;
		}
	}

	{
		SGX_MISC_INFO sSGXMiscInfo;

		// Retrieve the clock speed, this can vary at run time
		sSGXMiscInfo.eRequest = SGX_MISC_INFO_REQUEST_CLOCKSPEED;
		SGXGetMiscInfo(&psData->s3DDevData, &sSGXMiscInfo);
		psData->sInfo2.nClockSpeed = sSGXMiscInfo.uData.ui32SGXClockSpeed;

		// Retrieve the HW core revision
		sSGXMiscInfo.eRequest = SGX_MISC_INFO_REQUEST_SGXREV;
		SGXGetMiscInfo(&psData->s3DDevData, &sSGXMiscInfo);
		psData->sInfo2.nHWDeviceRevision = sSGXMiscInfo.uData.sSGXFeatures.ui32CoreRev;
	}

	psData->sInfo3.pszDriverDeviceName		= psData->sInfo2.pszDriverDeviceName;
	psData->sInfo3.nDriverDeviceRevision	= psData->sInfo2.nDriverDeviceRevision;
	psData->sInfo3.pszDriverName			= psData->sInfo2.pszDriverName;
	psData->sInfo3.nHWDeviceRevision		= psData->sInfo2.nHWDeviceRevision;
	psData->sInfo3.nHWCoreCount				= SGX_FEATURE_MP_CORE_COUNT_3D;
	psData->sInfo3.nHWCounterNum			= PVRSRV_SGX_HWPERF_NUM_COUNTERS;
	psData->sInfo3.nClockSpeed				= psData->sInfo2.nClockSpeed;

	psData->sInfo4.pszDriverDeviceName		= psData->sInfo3.pszDriverDeviceName;
	psData->sInfo4.nDriverDeviceRevision	= psData->sInfo3.nDriverDeviceRevision;
	psData->sInfo4.pszDriverName			= psData->sInfo3.pszDriverName;
	psData->sInfo4.nHWDeviceRevision		= psData->sInfo3.nHWDeviceRevision;
	psData->sInfo4.nHWCoreCount				= psData->sInfo3.nHWCoreCount;
	psData->sInfo4.nHWCounterNum			= psData->sInfo3.nHWCounterNum;
	psData->sInfo4.nMiscCounterNum			= PVRSRV_SGX_HWPERF_NUM_MISC_COUNTERS;
	psData->sInfo4.nClockSpeed				= psData->sInfo3.nClockSpeed;
	psData->sInfo4.nFlags					=
#ifdef SUPPORT_PERCONTEXT_PB
		PVRSSINFO4_PERCONTEXT_PB |
#endif
#ifdef SUPPORT_SHARED_PB
		PVRSSINFO4_SHARED_PB |
#endif
#ifdef SUPPORT_HYBRID_PB
		PVRSSINFO4_HYBRID_PB |
#endif
		0;
	psData->sInfo4.nMaxHostTimeVal			= 0xffffffff;

	psData->ui32CBCount				= 0;
	psData->ui32CBClockSpeed		= 0;
	psData->ui32CBHostTimeStamp		= 0;
	psData->ui32CBSkippedPackets	= 0;

	psData->sFakePerfCountersReading.nSW_ACTIVE_GROUP	= 16;
	psData->sFakePerfCountersReading.nSW_TIME			= 0;
	psData->sFakePerfCountersReading.nSW_SGX_AWAKE		= 1;
	psData->sFakePerfCountersReading.nHW_TIME_WRAPS		= 0;
	psData->sFakePerfCountersReading.nKICKTA			= 0;
	psData->sFakePerfCountersReading.nKICKTARENDER		= 0;
	psData->sFakePerfCountersReading.nr0				= 0;
	psData->sFakePerfCountersReading.nr1				= 0;
	psData->sFakePerfCountersReading.nr2				= 0;
	psData->sFakePerfCountersReading.nr3				= 0;
	psData->sFakePerfCountersReading.nr4				= 0;
	psData->sFakePerfCountersReading.nr5				= 0;
	psData->sFakePerfCountersReading.nr6				= 0;
	psData->sFakePerfCountersReading.nr7				= 0;
	psData->sFakePerfCountersReading.nr8				= 0;
	psData->sFakePerfCountersReading.ntimer				= 0;

	return psData;
}

/****************************************************************************
** Entry-point: Deinitialise
****************************************************************************/
static
void IMG_CALLCONV PVRSSDeInit(
	SPVRSSData		**ppsData)
{
	SPVRSSData	*psData;

	// free resources
	psData = *ppsData;
	(*ppsData) = 0;

	_ASSERT(psData);
	PVRSRVDisconnect(psData->psConnection);

	delete [] psData->sInfo2.pszDriverDeviceName;
	psData->sInfo2.pszDriverDeviceName = 0;

	delete [] psData->sInfo2.pszDriverName;
	psData->sInfo2.pszDriverName = 0;

	delete psData;
	psData = 0;
}

/****************************************************************************
** Entry-point: Set group
****************************************************************************/
static
bool IMG_CALLCONV PVRSSSetGroup(
	SPVRSSData					* const psData,
	const unsigned int			nGroup)
{
	if(!psData)
	{
		_ASSERT(false);
		return false;
	}

#if defined(SGX_FEATURE_EXTENDED_PERF_COUNTERS)
		/* FIXME */
#else
	psData->sHWPerfStatus.ui32PerfGroup = nGroup;
#endif
	return SetHWPerfStatus(psData);
}

/****************************************************************************
** Entry-point: Read perf counters then set group 2
** Returns absolute values.
****************************************************************************/
static
bool IMG_CALLCONV PVRSSReadPerfCountersThenSetGroup2(
	SPVRSSData					* const psData,
	SPVRSSPerfCountersReading	* const psPerfCounters,
	const unsigned int			nGroup)
{
	_ASSERT(psData && psPerfCounters);

	if (!psData || !psPerfCounters)
	{
		return false;
	}

	// get more packets, and a host time
	if(!TimingRetrieve(psData))
	{
		return false;
	}
	TimingFakeCounterReading(psData);

	// Return the faked up some data
	*psPerfCounters = psData->sFakePerfCountersReading;

	if(nGroup != (unsigned int)0-1)
	{
		// Switch to the new group
		return PVRSSSetGroup(psData, nGroup);
	}
	return true;
}

/****************************************************************************
** Entry-point: Timing enable
****************************************************************************/
static
bool IMG_CALLCONV PVRSSTimingEnable(
	SPVRSSData	* const psData,
	const bool	bEnable)
{
	_ASSERT(psData);

	if(bEnable)
	{
		psData->sHWPerfStatus.ui32NewHWPerfStatus = PVRSRV_SGX_HWPERF_STATUS_GRAPHICS_ON | PVRSRV_SGX_HWPERF_STATUS_PERIODIC_ON;
	}
	else
	{
		psData->sHWPerfStatus.ui32NewHWPerfStatus = PVRSRV_SGX_HWPERF_STATUS_PERIODIC_ON;
	}
	return SetHWPerfStatus(psData);
}

/****************************************************************************
** Entry-point: Timing enable 2
****************************************************************************/
static
bool IMG_CALLCONV PVRSSTimingEnable2(
	SPVRSSData			* const psData,
	const unsigned int	nEnableFlags)
{
	IMG_UINT32 ui32NewStatus = 0;

	if(nEnableFlags & PVRSSTIMINGENABLE2_PERIODIC)
	{
		ui32NewStatus |= PVRSRV_SGX_HWPERF_STATUS_PERIODIC_ON;
	}

	if(nEnableFlags & PVRSSTIMINGENABLE2_GRAPHICS)
	{
		ui32NewStatus |= PVRSRV_SGX_HWPERF_STATUS_GRAPHICS_ON;
	}

	if(nEnableFlags & PVRSSTIMINGENABLE2_MK)
	{
		ui32NewStatus |= PVRSRV_SGX_HWPERF_STATUS_MK_EXECUTION_ON;
	}

	_ASSERT(psData);
	psData->sHWPerfStatus.ui32NewHWPerfStatus = ui32NewStatus;
	return SetHWPerfStatus(psData);
}

static EPVRSSTimingType2 ConvType2(const IMG_UINT32 ui32Type)
{
	switch(ui32Type)
	{
	case PVRSRV_SGX_HWPERF_TYPE_TRANSFER_START:	return ePVRSSTiming2TransferBgn;
	case PVRSRV_SGX_HWPERF_TYPE_TRANSFER_END:	return ePVRSSTiming2TransferEnd;
	case PVRSRV_SGX_HWPERF_TYPE_TA_START:		return ePVRSSTiming2TABgn;
	case PVRSRV_SGX_HWPERF_TYPE_TA_END:			return ePVRSSTiming2TAEnd;
	case PVRSRV_SGX_HWPERF_TYPE_3D_START:		return ePVRSSTiming23DBgn;
	case PVRSRV_SGX_HWPERF_TYPE_3D_END:			return ePVRSSTiming23DEnd;
	case PVRSRV_SGX_HWPERF_TYPE_2D_START:		return ePVRSSTiming22DBgn;
	case PVRSRV_SGX_HWPERF_TYPE_2D_END:			return ePVRSSTiming22DEnd;
	case PVRSRV_SGX_HWPERF_TYPE_POWER_START:
	case PVRSRV_SGX_HWPERF_TYPE_POWER_END:
	case PVRSRV_SGX_HWPERF_TYPE_PERIODIC:
	default:									return ePVRSSTiming2TimeTypeCount;
	}
}

static EPVRSSTimingType3 ConvType3(const IMG_UINT32 ui32Type)
{
	switch(ui32Type)
	{
	case PVRSRV_SGX_HWPERF_TYPE_TRANSFER_START:	return ePVRSSTiming3TransferBgn;
	case PVRSRV_SGX_HWPERF_TYPE_TRANSFER_END:	return ePVRSSTiming3TransferEnd;
	case PVRSRV_SGX_HWPERF_TYPE_TA_START:		return ePVRSSTiming3TABgn;
	case PVRSRV_SGX_HWPERF_TYPE_TA_END:			return ePVRSSTiming3TAEnd;
	case PVRSRV_SGX_HWPERF_TYPE_3D_START:		return ePVRSSTiming33DBgn;
	case PVRSRV_SGX_HWPERF_TYPE_3D_END:			return ePVRSSTiming33DEnd;
	case PVRSRV_SGX_HWPERF_TYPE_2D_START:		return ePVRSSTiming32DBgn;
	case PVRSRV_SGX_HWPERF_TYPE_2D_END:			return ePVRSSTiming32DEnd;
	case PVRSRV_SGX_HWPERF_TYPE_POWER_START:	return ePVRSSTiming3PowerBgn;
	case PVRSRV_SGX_HWPERF_TYPE_POWER_END:		return ePVRSSTiming3PowerEnd;
	case PVRSRV_SGX_HWPERF_TYPE_PERIODIC:		return ePVRSSTiming3Periodic;
	default:					_ASSERT(false);	return ePVRSSTiming3TimeTypeCount;
	}
}

static EPVRSSTimingType4 ConvType4(const IMG_UINT32 ui32Type)
{
	switch(ui32Type)
	{
	case PVRSRV_SGX_HWPERF_TYPE_TRANSFER_START:	return ePVRSSTiming4TransferBgn;
	case PVRSRV_SGX_HWPERF_TYPE_TRANSFER_END:	return ePVRSSTiming4TransferEnd;
	case PVRSRV_SGX_HWPERF_TYPE_TA_START:		return ePVRSSTiming4TABgn;
	case PVRSRV_SGX_HWPERF_TYPE_TA_END:			return ePVRSSTiming4TAEnd;
	case PVRSRV_SGX_HWPERF_TYPE_3D_START:		return ePVRSSTiming43DBgn;
	case PVRSRV_SGX_HWPERF_TYPE_3D_END:			return ePVRSSTiming43DEnd;
	case PVRSRV_SGX_HWPERF_TYPE_2D_START:		return ePVRSSTiming42DBgn;
	case PVRSRV_SGX_HWPERF_TYPE_2D_END:			return ePVRSSTiming42DEnd;
	case PVRSRV_SGX_HWPERF_TYPE_POWER_START:	return ePVRSSTiming4PowerBgn;
	case PVRSRV_SGX_HWPERF_TYPE_POWER_END:		return ePVRSSTiming4PowerEnd;
	case PVRSRV_SGX_HWPERF_TYPE_PERIODIC:		return ePVRSSTiming4Periodic;
	case PVRSRV_SGX_HWPERF_TYPE_3DSPM_START:	return ePVRSSTiming43DSPMBgn;
	case PVRSRV_SGX_HWPERF_TYPE_3DSPM_END:		return ePVRSSTiming43DSPMEnd;
	case PVRSRV_SGX_HWPERF_TYPE_MK_TRANSFER_DUMMY_START:	return ePVRSSTiming4MKTransferDummyBgn;
	case PVRSRV_SGX_HWPERF_TYPE_MK_TRANSFER_DUMMY_END:		return ePVRSSTiming4MKTransferDummyEnd;
	case PVRSRV_SGX_HWPERF_TYPE_MK_TA_DUMMY_START:			return ePVRSSTiming4MKTADummyBgn;
	case PVRSRV_SGX_HWPERF_TYPE_MK_TA_DUMMY_END:			return ePVRSSTiming4MKTADummyEnd;
	case PVRSRV_SGX_HWPERF_TYPE_MK_3D_DUMMY_START:			return ePVRSSTiming4MK3DDummyBgn;
	case PVRSRV_SGX_HWPERF_TYPE_MK_3D_DUMMY_END:			return ePVRSSTiming4MK3DDummyEnd;
	case PVRSRV_SGX_HWPERF_TYPE_MK_2D_DUMMY_START:			return ePVRSSTiming4MK2DDummyBgn;
	case PVRSRV_SGX_HWPERF_TYPE_MK_2D_DUMMY_END:			return ePVRSSTiming4MK2DDummyEnd;
	case PVRSRV_SGX_HWPERF_TYPE_MK_TA_LOCKUP:				return ePVRSSTiming4MKTALockup;
	case PVRSRV_SGX_HWPERF_TYPE_MK_3D_LOCKUP:				return ePVRSSTiming4MK3DLockup;
	case PVRSRV_SGX_HWPERF_TYPE_MK_2D_LOCKUP:				return ePVRSSTiming4MK2DLockup;
	case PVRSRV_SGX_HWPERF_TYPE_MK_EVENT_START:				return ePVRSSTiming4MKEventBgn;
	case PVRSRV_SGX_HWPERF_TYPE_MK_EVENT_END:				return ePVRSSTiming4MKEventEnd;
	case PVRSRV_SGX_HWPERF_TYPE_MK_TA_START:				return ePVRSSTiming4MKTABgn;
	case PVRSRV_SGX_HWPERF_TYPE_MK_TA_END:					return ePVRSSTiming4MKTAEnd;
	case PVRSRV_SGX_HWPERF_TYPE_MK_3D_START:				return ePVRSSTiming4MK3DBgn;
	case PVRSRV_SGX_HWPERF_TYPE_MK_3D_END:					return ePVRSSTiming4MK3DEnd;
	case PVRSRV_SGX_HWPERF_TYPE_MK_2D_START:				return ePVRSSTiming4MK2DBgn;
	case PVRSRV_SGX_HWPERF_TYPE_MK_2D_END:					return ePVRSSTiming4MK2DEnd;
	default:					_ASSERT(false);				return ePVRSSTiming4TimeTypeCount;
	}
}

/****************************************************************************
** Entry-point: Timing retrieve 2
****************************************************************************/
static
bool IMG_CALLCONV PVRSSTimingRetrieve2(
	SPVRSSData		* const psData,
	SPVRSSTiming2	* const psTimings,
	unsigned int	* const pnArraySize,
	unsigned int	* const pnTime,
	unsigned int	* const pnClockSpeed)
{
	unsigned int i, nOutputDataCount = 0, nCounterIdx;

	_ASSERT(psData && psTimings && pnArraySize && *pnArraySize);
	_ASSERT(PVRSRV_SGX_HWPERF_NUM_COUNTERS == (sizeof(psTimings->anCounters) / sizeof(*psTimings->anCounters)));

	/*
		Get more packets of data from Services
	*/
	if(!TimingRetrieve(psData))
	{
		return false;
	}
	TimingFakeCounterReading(psData);

	for(i = 0; i < psData->ui32CBCount; ++i)
	{
		SPVRSSTiming2						* const pDst = &psTimings[nOutputDataCount];
		const PVRSRV_SGX_HWPERF_CB_ENTRY	* const pSrc = &psData->asCBEntry[i];

		pDst->nFrameNumber	= pSrc->ui32FrameNo;
		pDst->eType			= ConvType2(pSrc->ui32Type);

		if(0xffffffff == pSrc->ui32Ordinal)
		{
			// timing enabled, reset
			psData->ui32CBSkippedPackets = 0;
		}

		// Don't output unknown packets
		if(pDst->eType == ePVRSSTiming2TimeTypeCount)
		{
			++psData->ui32CBSkippedPackets;
			continue;
		}

		// Deduct any skipped packets, so as not to introduce holes in the
		// numbering (which would suggest to code downstream that the
		// circular buffer became full)
		pDst->nOrdinal		= pSrc->ui32Ordinal - psData->ui32CBSkippedPackets;

		// at the time of writing, ui32Info is 0 except for power-on events where it is the host time the uk was powered up
		//pSrc->ui32Info;
		pDst->nClocksx16	= pSrc->ui32Clocksx16;

		// Generate counters by summing the values across the cores
		for(nCounterIdx = 0; nCounterIdx < PVRSRV_SGX_HWPERF_NUM_COUNTERS; ++nCounterIdx)
		{
			pDst->anCounters[nCounterIdx] = SumReadings(pSrc->ui32Counters, nCounterIdx);
		}

		++nOutputDataCount;
	}

	*pnArraySize	= nOutputDataCount;
	*pnTime			= psData->ui32CBHostTimeStamp;
	*pnClockSpeed	= psData->ui32CBClockSpeed;

	// All packets processed
	psData->ui32CBCount	= 0;
	return true;
}

/****************************************************************************
** Entry-point: Timing retrieve 3
****************************************************************************/
static
bool IMG_CALLCONV PVRSSTimingRetrieve3(
	SPVRSSData		* const psData,
	SPVRSSTiming3	* const psTimings,
	unsigned int	* const pnArraySize,
	unsigned int	* const pnTime,
	unsigned int	* const pnClockSpeed)
{
	unsigned int i;

	_ASSERT(psData && psTimings && pnArraySize && *pnArraySize);

	// Should not be any data in that array if this fn is in use
	if(psData->ui32CBCount)
	{
		_ASSERT(0);
		return false;
	}

	/*
		Get more packets of data from Services
	*/
	if(!TimingRetrieve(psData))
	{
		return false;
	}

	for(i = 0; i < psData->ui32CBCount; ++i)
	{
		psTimings[i].nFrameNumber	= psData->asCBEntry[i].ui32FrameNo;
		psTimings[i].eType			= ConvType3(psData->asCBEntry[i].ui32Type);
		psTimings[i].nOrdinal		= psData->asCBEntry[i].ui32Ordinal;
		// at the time of writing, ui32Info is 0 except for power-on events where it is the host time the uk was powered up
		psTimings[i].nInfo			= psData->asCBEntry[i].ui32Info;
		psTimings[i].nClocksx16		= psData->asCBEntry[i].ui32Clocksx16;
		psTimings[i].pnCounters		= &psData->asCBEntry[i].ui32Counters[0][0];
	}

	*pnArraySize	= psData->ui32CBCount;
	*pnTime			= psData->ui32CBHostTimeStamp;
	*pnClockSpeed	= psData->ui32CBClockSpeed;

	// All packets processed
	psData->ui32CBCount	= 0;
	return true;
}

/****************************************************************************
** Entry-point: Timing retrieve 4
****************************************************************************/
static
bool IMG_CALLCONV PVRSSTimingRetrieve4(
	SPVRSSData		* const psData,
	SPVRSSTiming4	* const psTimings,
	unsigned int	* const pnArraySize,
	unsigned int	* const pnTime,
	unsigned int	* const pnClockSpeed)
{
	unsigned int i;

	_ASSERT(psData && psTimings && pnArraySize && *pnArraySize);

	// Should not be any data in that array if this fn is in use
	if(psData->ui32CBCount)
	{
		_ASSERT(0);
		return false;
	}

	/*
		Get more packets of data from Services
	*/
	if(!TimingRetrieve(psData))
	{
		return false;
	}

	for(i = 0; i < psData->ui32CBCount; ++i)
	{
		psTimings[i].nFrameNumber	= psData->asCBEntry[i].ui32FrameNo;
		psTimings[i].nPID			= psData->asCBEntry[i].ui32PID;
		psTimings[i].nRTData		= psData->asCBEntry[i].ui32RTData;
		psTimings[i].eType			= ConvType4(psData->asCBEntry[i].ui32Type);
		psTimings[i].nOrdinal		= psData->asCBEntry[i].ui32Ordinal;
		// at the time of writing, ui32Info is 0 except for power-on events where it is the host time the uk was powered up
		psTimings[i].nInfo			= psData->asCBEntry[i].ui32Info;
		psTimings[i].nClocksx16		= psData->asCBEntry[i].ui32Clocksx16;
		psTimings[i].pnCounters		= &psData->asCBEntry[i].ui32Counters[0][0];
		psTimings[i].pnMiscCounters	= &psData->asCBEntry[i].ui32MiscCounters[0][0];
	}

	*pnArraySize	= psData->ui32CBCount;
	*pnTime			= psData->ui32CBHostTimeStamp;
	*pnClockSpeed	= psData->ui32CBClockSpeed;

	// All packets processed
	psData->ui32CBCount	= 0;
	return true;
}

/****************************************************************************
** Entry-point: Get info 2
****************************************************************************/
static
const SPVRSSInfo2 * IMG_CALLCONV PVRSSGetInfo2(
	SPVRSSData	* const psData)
{
	_ASSERT(psData);
	return &psData->sInfo2;
}

/****************************************************************************
** Entry-point: Get info 3
****************************************************************************/
static
const SPVRSSInfo3 * IMG_CALLCONV PVRSSGetInfo3(
	SPVRSSData	* const psData)
{
	_ASSERT(psData);
	return &psData->sInfo3;
}

/****************************************************************************
** Entry-point: Get info 4
****************************************************************************/
static
const SPVRSSInfo4 * IMG_CALLCONV PVRSSGetInfo4(
	SPVRSSData	* const psData)
{
	_ASSERT(psData);
	return &psData->sInfo4;
}

/****************************************************************************
** Exported entry-point: get proc address
****************************************************************************/
IMG_EXPORT
void * IMG_CALLCONV pvrssGetProcAddress(const EPVRSSProcedure eProc)
{
	switch(eProc)
	{
	case ePVRSSProcInit:
		return (void*)&PVRSSInit;

	case ePVRSSProcDeInit:
		return (void*)&PVRSSDeInit;

	case ePVRSSProcTimingEnable:
		return (void*)&PVRSSTimingEnable;

	case ePVRSSProcTimingRetrieve2:
		return (void*)&PVRSSTimingRetrieve2;

	case ePVRSSProcGetInfo2:
		return (void*)&PVRSSGetInfo2;

	case ePVRSSProcReadPerfCountersThenSetGroup2:
		return (void*)&PVRSSReadPerfCountersThenSetGroup2;

	case ePVRSSProcTimingRetrieve3:
		return (void*)&PVRSSTimingRetrieve3;

	case ePVRSSProcTimingRetrieve4:
		return (void*)&PVRSSTimingRetrieve4;

	case ePVRSSProcGetInfo3:
		return (void*)&PVRSSGetInfo3;

	case ePVRSSProcGetInfo4:
		return (void*)&PVRSSGetInfo4;

	case ePVRSSProcSetGroup:
		return (void*)&PVRSSSetGroup;

	case ePVRSSProcTimingEnable2:
		return (void*)&PVRSSTimingEnable2;

	default:
		// Unrecognised request
		return (void*)0;
	}
}

/*****************************************************************************
 End of file (PVRScopeServices.cpp)
*****************************************************************************/
