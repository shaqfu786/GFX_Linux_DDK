/*****************************************************************************
$Revision: 1.12 $
*****************************************************************************/

#ifndef _PVRSCOPESERVICES_H_
#define _PVRSCOPESERVICES_H_

#ifdef __cplusplus
extern "C" {
#endif


			#if defined(__linux__) || defined(ANDROID)

				#define IMG_CALLCONV
				#define IMG_EXPORT __attribute__((visibility("default")))
				#define IMG_IMPORT

			#else
					#error("define an OS")
			#endif

// 64-bit type
	#if (defined(__linux__) || defined(ANDROID))
		typedef unsigned long long		IMG_UINT64;
		typedef long long 				IMG_INT64;
	#else
		#error("define an OS")
	#endif

/****************************************************************************
** Structures
****************************************************************************/

// Internal implementation data
struct SPVRSSData;

/****************************************************************************
** Entry-point: Initialise
****************************************************************************/
typedef SPVRSSData *		(IMG_CALLCONV *PVRSSFNINIT)(void);

/****************************************************************************
** Entry-point: Deinitialise
****************************************************************************/
typedef void				(IMG_CALLCONV *PVRSSFNDEINIT)(
	SPVRSSData		**ppsData);

/****************************************************************************
** Entry-point: Read perf counters then set group 2
****************************************************************************/
struct SPVRSSPerfCountersReading
{
	unsigned int nSW_ACTIVE_GROUP;
	unsigned int nSW_TIME;
	unsigned int nSW_SGX_AWAKE;
	unsigned int nHW_TIME_WRAPS;
	unsigned int nKICKTA;
	unsigned int nKICKTARENDER;

	unsigned int nr0;
	unsigned int nr1;
	unsigned int nr2;
	unsigned int nr3;
	unsigned int nr4;
	unsigned int nr5;
	unsigned int nr6;
	unsigned int nr7;
	unsigned int nr8;

	unsigned int ntimer;
};

typedef bool				(IMG_CALLCONV *PVRSSFNREADPERFCOUNTERSTHENSETGROUP2)(
	SPVRSSData					* const psData,
	SPVRSSPerfCountersReading	* const psPerfCounters,
	const unsigned int			nGroup);

/****************************************************************************
** Entry-point: Timing enable
****************************************************************************/
typedef bool				(IMG_CALLCONV *PVRSSFNTIMINGENABLE)(
	SPVRSSData	* const psData,
	const bool	bEnable);

/****************************************************************************
** Entry-point: Timing enable 2
****************************************************************************/

/* Flags to control which events are generated */
#define PVRSSTIMINGENABLE2_PERIODIC	0x00000001	// Periodic samples
#define PVRSSTIMINGENABLE2_GRAPHICS	0x00000002	// Rendering tasks
#define PVRSSTIMINGENABLE2_MK		0x00000004	// uKernel tasks

typedef bool				(IMG_CALLCONV *PVRSSFNTIMINGENABLE2)(
	SPVRSSData	* const psData,
	const unsigned int	nEnableFlags);

/****************************************************************************
** Entry-point: Timing retrieve 2
****************************************************************************/
enum EPVRSSTimingType2
{
	ePVRSSTiming2Invalid,

	ePVRSSTiming2TransferBgn,
	ePVRSSTiming2TransferEnd,

	ePVRSSTiming2TABgn,
	ePVRSSTiming2TAEnd,

	ePVRSSTiming23DBgn,
	ePVRSSTiming23DEnd,

	ePVRSSTiming22DBgn,
	ePVRSSTiming22DEnd,

	ePVRSSTiming2TimeTypeCount	// used for array lengths
};

#define PVRSSTIMING2_COUNTER_NUM	(9)

struct SPVRSSTiming2
{
	unsigned int		nFrameNumber;
	EPVRSSTimingType2	eType;
	unsigned int		nOrdinal;
	unsigned int		nClocksx16;
	unsigned int		anCounters[PVRSSTIMING2_COUNTER_NUM];
};

typedef bool				(IMG_CALLCONV *PVRSSFNTIMINGRETRIEVE2)(
	SPVRSSData		* const psData,
	SPVRSSTiming2	* const psTimings,
	unsigned int	* const pnArraySize,
	unsigned int	* const pnTime,
	unsigned int	* const pnClockSpeed);

/****************************************************************************
** Entry-point: Timing retrieve 3
****************************************************************************/
enum EPVRSSTimingType3
{
	ePVRSSTiming3TransferBgn,
	ePVRSSTiming3TransferEnd,

	ePVRSSTiming3TABgn,
	ePVRSSTiming3TAEnd,

	ePVRSSTiming33DBgn,
	ePVRSSTiming33DEnd,

	ePVRSSTiming32DBgn,
	ePVRSSTiming32DEnd,

	ePVRSSTiming3PowerBgn,
	ePVRSSTiming3PowerEnd,

	ePVRSSTiming3Periodic,

	ePVRSSTiming3TimeTypeCount	// used for array lengths
};

struct SPVRSSTiming3
{
	unsigned int		nFrameNumber;
	EPVRSSTimingType3	eType;
	unsigned int		nOrdinal;
	unsigned int		nInfo;
	unsigned int		nClocksx16;
	unsigned int		*pnCounters;
};

typedef bool				(IMG_CALLCONV *PVRSSFNTIMINGRETRIEVE3)(
	SPVRSSData		* const psData,
	SPVRSSTiming3	* const psTimings,
	unsigned int	* const pnArraySize,
	unsigned int	* const pnTime,
	unsigned int	* const pnClockSpeed);

/****************************************************************************
** Entry-point: Timing retrieve 4
****************************************************************************/
enum EPVRSSTimingType4
{
	ePVRSSTiming4TransferBgn,
	ePVRSSTiming4TransferEnd,

	ePVRSSTiming4TABgn,
	ePVRSSTiming4TAEnd,

	ePVRSSTiming43DBgn,
	ePVRSSTiming43DEnd,

	ePVRSSTiming42DBgn,
	ePVRSSTiming42DEnd,

	ePVRSSTiming4PowerBgn,
	ePVRSSTiming4PowerEnd,

	ePVRSSTiming4Periodic,

	ePVRSSTiming43DSPMBgn,
	ePVRSSTiming43DSPMEnd,

	ePVRSSTiming4MKTransferDummyBgn,
	ePVRSSTiming4MKTransferDummyEnd,
	ePVRSSTiming4MKTADummyBgn,
	ePVRSSTiming4MKTADummyEnd,
	ePVRSSTiming4MK3DDummyBgn,
	ePVRSSTiming4MK3DDummyEnd,
	ePVRSSTiming4MK2DDummyBgn,
	ePVRSSTiming4MK2DDummyEnd,
	ePVRSSTiming4MKTALockup,
	ePVRSSTiming4MK3DLockup,
	ePVRSSTiming4MK2DLockup,
	ePVRSSTiming4MKEventBgn,
	ePVRSSTiming4MKEventEnd,
	ePVRSSTiming4MKTABgn,
	ePVRSSTiming4MKTAEnd,
	ePVRSSTiming4MK3DBgn,
	ePVRSSTiming4MK3DEnd,
	ePVRSSTiming4MK2DBgn,
	ePVRSSTiming4MK2DEnd,

	ePVRSSTiming4TimeTypeCount	// used for array lengths
};

struct SPVRSSTiming4
{
	unsigned int		nFrameNumber;
	unsigned int		nPID;
	unsigned int		nRTData;
	EPVRSSTimingType4	eType;
	unsigned int		nOrdinal;
	unsigned int		nInfo;
	unsigned int		nClocksx16;
	unsigned int		*pnCounters;
	unsigned int		*pnMiscCounters;
};

typedef bool				(IMG_CALLCONV *PVRSSFNTIMINGRETRIEVE4)(
	SPVRSSData		* const psData,
	SPVRSSTiming4	* const psTimings,
	unsigned int	* const pnArraySize,
	unsigned int	* const pnTime,
	unsigned int	* const pnClockSpeed);

/****************************************************************************
** Entry-point: Get info 2
****************************************************************************/
struct SPVRSSInfo2
{
	const char		*pszDriverDeviceName;
	unsigned int	nDriverDeviceRevision;
	const char		*pszDriverName;			/* n.nn.nn.nnnnn (unless branched?) */

	unsigned int	nHWDeviceRevision;		/* SGX Core revision */
	unsigned int	nClockSpeed;			/* speed at start-up, can vary */
};

typedef const SPVRSSInfo2 *	(IMG_CALLCONV *PVRSSFNGETINFO2)(
	SPVRSSData	* const psData);

/****************************************************************************
** Entry-point: Get info 3
****************************************************************************/
struct SPVRSSInfo3
{
	const char		*pszDriverDeviceName;
	unsigned int	nDriverDeviceRevision;
	const char		*pszDriverName;			/* n.nn.nn.nnnnn (unless branched?) */

	unsigned int	nHWDeviceRevision;		/* SGX Core revision */
	unsigned int	nHWCoreCount;
	unsigned int	nHWCounterNum;
	unsigned int	nClockSpeed;			/* speed at start-up, can vary */
};

typedef const SPVRSSInfo3 *	(IMG_CALLCONV *PVRSSFNGETINFO3)(
	SPVRSSData	* const psData);

/****************************************************************************
** Entry-point: Get info 4
****************************************************************************/

/* Only one of the PB flags can be set */
#define PVRSSINFO4_PERCONTEXT_PB	0x00000001
#define PVRSSINFO4_SHARED_PB		0x00000002
#define PVRSSINFO4_HYBRID_PB		0x00000004	/* Should never see this one */

struct SPVRSSInfo4
{
	const char		*pszDriverDeviceName;
	unsigned int	nDriverDeviceRevision;
	const char		*pszDriverName;			/* n.nn.nn.nnnnn (unless branched?) */

	unsigned int	nHWDeviceRevision;		/* SGX Core revision */
	unsigned int	nHWCoreCount;
	unsigned int	nHWCounterNum;
	unsigned int	nMiscCounterNum;
	unsigned int	nClockSpeed;			/* speed at start-up, can vary */

	unsigned int	nFlags;					/* PVRSSINFO4_ flags */
	unsigned int	nMaxHostTimeVal;		/* At what value do host time readings wrap? */
};

typedef const SPVRSSInfo4 *	(IMG_CALLCONV *PVRSSFNGETINFO4)(
	SPVRSSData	* const psData);

/****************************************************************************
** Entry-point: Set group
****************************************************************************/
typedef bool				(IMG_CALLCONV *PVRSSFNSETGROUP)(
	SPVRSSData					* const psData,
	const unsigned int			nGroup);

/****************************************************************************
** Exported entry-point: get proc address
****************************************************************************/

/*
	Values of this enum must never be changed, otherwise forwards and
	backwards compatibility will not endure.

	Add new entries to the end of the list.
*/
enum EPVRSSProcedure
{
	ePVRSSProcInit								= 0,
	ePVRSSProcDeInit							= 1,
	//ePVRSSProcGetInfo							= 2,
	//ePVRSSProcReadPerfCountersThenSetGroup	= 3,
	ePVRSSProcTimingEnable						= 4,
	//ePVRSSProcTimingRetrieve					= 5,
	ePVRSSProcTimingRetrieve2					= 6,
	ePVRSSProcGetInfo2							= 7,
	ePVRSSProcReadPerfCountersThenSetGroup2		= 10000,
	ePVRSSProcTimingRetrieve3					= 20000,
	ePVRSSProcTimingRetrieve4					= 20001,
	ePVRSSProcGetInfo3							= 30000,
	ePVRSSProcGetInfo4							= 30001,
	ePVRSSProcSetGroup							= 40000,
	ePVRSSProcTimingEnable2						= 50000,

	// Insert new entries above.
	ePVRSSProcMax32bit							= 0xffffffff
};

IMG_IMPORT
void * IMG_CALLCONV pvrssGetProcAddress(const EPVRSSProcedure eProc);


#ifdef __cplusplus
}
#endif

#endif /* _PVRSCOPESERVICES_H_ */

/*****************************************************************************
 End of file (PVRScopeServices.h)
*****************************************************************************/
