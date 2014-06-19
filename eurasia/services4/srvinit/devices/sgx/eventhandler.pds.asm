/*****************************************************************************
 Name		: eventhandler.pds.asm

 Title		: PDS Assembly Code

 Author 	: Imagination Technologies

 Created  	: 01/01/2008

 Copyright	: 2007 by Imagination Technologies Limited. All rights reserved.
		  No part of this software, either material or conceptual
		  may be copied or distributed, transmitted, transcribed,
		  stored in a retrieval system or translated into any
		  human or computer language in any form by any means,
		  electronic, mechanical, manual or other-wise, or
		  disclosed to third parties without the express written
		  permission of Imagination Technologies Limited, HomePark
		  Industrial Estate, King's Langley, Hertfordshire,
		  WD4 8LZ, U.K.

 Description 	: PDS program for handling all events

 Program Type	: PDS assembly language

 Version 	: $Revision: 1.16 $

 Modifications	:

 $Log: eventhandler.pds.asm $

 ******************************************************************************/

#include "sgxdefs.h"

#if defined(SGX_FEATURE_MP)
#define EVENTS_3D_IR0	(EURASIA_PDS_IR0_EDM_EVENT_MPPIXENDRENDER | \
						 EURASIA_PDS_IR0_EDM_EVENT_MP3DMEMFREE)

#define EVENTS_3D_IR1	(EURASIA_PDS_IR1_EDM_EVENT_ISPBREAKPOINT)
#define EVENTS_SPM (EURASIA_PDS_IR0_EDM_EVENT_MPGBLOUTOFMEM | \
					EURASIA_PDS_IR0_EDM_EVENT_MPMTOUTOFMEM | \
					EURASIA_PDS_IR0_EDM_EVENT_MPTATERMINATE)
#define EVENTS_TA		(EURASIA_PDS_IR0_EDM_EVENT_MPTAFINISHED)
#else
#define EVENTS_3D_IR0	(0)
#define EVENTS_3D_IR1	(EURASIA_PDS_IR1_EDM_EVENT_PIXENDRENDER | \
						 EURASIA_PDS_IR1_EDM_EVENT_ISPBREAKPOINT | \
						 EURASIA_PDS_IR1_EDM_EVENT_3DMEMFREE)
#define EVENTS_SPM (EURASIA_PDS_IR1_EDM_EVENT_GBLOUTOFMEM | \
					EURASIA_PDS_IR1_EDM_EVENT_MTOUTOFMEM | \
					EURASIA_PDS_IR1_EDM_EVENT_TATERMINATE)
#define EVENTS_TA		(EURASIA_PDS_IR1_EDM_EVENT_TAFINISHED)
#endif


/*****************************************************************************
* Register declarations
******************************************************************************/

/* Constants */
data dword INPUT_IR0_PA_DEST;
data dword INPUT_IR1_PA_DEST;
data dword INPUT_TIMESTAMP_PA_DEST;
data dword INPUT_DOUTU1;
data dword INPUT_DOUTU2;
data dword INPUT_GENERIC_DOUTU0;

data dword INPUT_3D_DOUTU0;
data dword INPUT_TA_DOUTU0;
#if defined(SGX_FEATURE_2D_HARDWARE)
data dword INPUT_2D_DOUTU0;
#endif
data dword INPUT_SPM_DOUTU0;

#if !defined(FIX_HW_BRN_25339) && !defined(FIX_HW_BRN_27330)
/* Temp temps */
temp dword tmp0;
temp dword tmp1;
temp dword eventreg1;

/*****************************************************************************
* Setup the primary attributes
******************************************************************************/
/* Emit ir0 and ir1 */
movs	douta,	ir0,	INPUT_IR0_PA_DEST;
movs	douta,	ir1,	INPUT_IR1_PA_DEST;

#if defined(SUPPORT_SGX_HWPERF)
/* Save the time stamp */
movs	douta,	tim, INPUT_TIMESTAMP_PA_DEST;
#endif


#if defined(FIX_HW_BRN_31474)
and		tmp0, ir0, 	EURASIA_PDS_IR0_EDM_EVENT_SW_EVENT2;
tstz	p0, tmp0;
p0	bra	NotCacheClearRequest;
	/* The ukernel has requested the PDS cache be cleared */
	bra	ClearLine2;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine2:
	bra	ClearLine3;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine3:
	bra	ClearLine4;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine4:
	bra	ClearLine5;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine5:
	bra	ClearLine6;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine6:
	bra	ClearLine7;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine7:
	bra	ClearLine8;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine8:
	/* Use the generic handler in case there was any events set */
	bra	UseGenericEventHandler;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
NotCacheClearRequest:
#endif

/* extRact the event bits from ir1 */
and		eventreg1, ir1, EURASIA_PDS_IR1_EDM_EVENT_KICKPTR_CLRMSK;

/* Check if the SW event bit is set */
and		tmp0,	eventreg1,	EURASIA_PDS_IR1_EDM_EVENT_SWEVENT;
/* Test whether any of the bits are set */
tstz	p0,	tmp0;
p0	bra	NotHostKick;
	/* There is a SW event so go via Generic event handler */
	bra	UseGenericEventHandler;
NotHostKick:

#if defined(SGX_FEATURE_2D_HARDWARE)
/******************************************************************************
* Check if it is a loopback for the 2D Module
******************************************************************************/
/* extract the 2D Bits from IR0 */
#if defined(SGX_FEATURE_PTLA)
and		tmp0,	ir0,	EURASIA_PDS_IR0_EDM_EVENT_PTLACOMPLETE;
#else
and		tmp0,	ir0,	EURASIA_PDS_IR0_EDM_EVENT_TWODCOMPLETE;
#endif

/* Test whether any of the bits are set */
tstz	p0,	tmp0;
p0	bra	Not2DModule;
	/* Check whether there are events not associated with this module */
	tstz	p0, eventreg1;
	p0 xor	tmp0, ir0, tmp0;
	p0 tstz	p0, tmp0;
	p0	bra		Use2DEventHandler;
	bra	UseGenericEventHandler;
	Use2DEventHandler:
	movs		doutu,		INPUT_2D_DOUTU0,	INPUT_DOUTU1,	INPUT_DOUTU2;
	halt;
Not2DModule:
#endif

/******************************************************************************
* Check if it is an event for the 3D Module
******************************************************************************/
/* extract the 3D Bits from IR1 */
and		tmp0,	eventreg1,	EVENTS_3D_IR1;
/* Test whether any of the bits are set */
tstz	p0,	tmp0;
#if defined(SGX_FEATURE_MP)
p0	and		tmp1, ir0,	EVENTS_3D_IR0;
p0	tstz	p0, tmp1;
#endif
p0	bra	Not3DModule;
	/* Check whether there are events not associated with this module */
	xor		tmp0, eventreg1, tmp0;
	tstz	p0, tmp0;
#if defined(SGX_FEATURE_MP)
	p0	xor	tmp1, ir0, tmp1;
	p0	tstz	p0, tmp1;
#else
	p0	tstz	p0, ir0;
#endif
	p0	bra		Use3DEventHandler;
	/* Not just 3D events */
	bra	UseGenericEventHandler;
	Use3DEventHandler:
	movs		doutu,		INPUT_3D_DOUTU0,	INPUT_DOUTU1,	INPUT_DOUTU2;
	halt;
Not3DModule:

/******************************************************************************
* Check if it is an event for the TA Module
******************************************************************************/
/* extract the TA Bits from IR0 */
#if defined(SGX_FEATURE_MP)
and		tmp0, 	ir0,	EVENTS_TA;
#else
and		tmp0,	eventreg1,	EVENTS_TA;
#endif
/* Test whether any of the bits are set */
tstz	p0, tmp0;
p0	bra	NotTAModule;
	/* Check whether there are events not associated with this module */
#if defined(SGX_FEATURE_MP)
	tstz	p0, eventreg1;
	p0	xor		tmp0, ir0, tmp0;
#else
	tstz	p0, ir0;
	p0	xor	tmp0, eventreg1, tmp0;
#endif
	p0	tstz	p0, tmp0;
	p0	bra		UseTAEventHandler;
	bra	UseGenericEventHandler;
	UseTAEventHandler:
	movs		doutu,		INPUT_TA_DOUTU0,	INPUT_DOUTU1,	INPUT_DOUTU2;
	halt;
NotTAModule:

/******************************************************************************
* Check if it is an event for the SPM Module
******************************************************************************/
/* extract the SPM Bits */
#if defined(SGX_FEATURE_MP)
and		tmp0,	ir0,	EVENTS_SPM;
#else
and		tmp0,	eventreg1,	EVENTS_SPM;
#endif
/* Test whether any of the bits are set */
tstz	p0,	tmp0;
p0	bra	UseGenericEventHandler;
	/* Check whether there are events not associated with this module */
#if defined(SGX_FEATURE_MP)
	tstz	p0, eventreg1;
	p0 xor	tmp0, ir0, tmp0;
#else
	tstz	p0, ir0;
	p0 xor	tmp0, eventreg1, tmp0;
#endif
	p0	tstz	p0, tmp0;
	p0	bra		UseSPMEventHandler;
	bra	UseGenericEventHandler;
	UseSPMEventHandler:
	movs		doutu,		INPUT_SPM_DOUTU0,	INPUT_DOUTU1,	INPUT_DOUTU2;
	halt;
UseGenericEventHandler:
/******************************************************************************
* Use Generic Event handler code as the events are not restricted to one module
* or it is a host kick event
******************************************************************************/

/* Issue USE task and exit */
movs		doutu,		INPUT_GENERIC_DOUTU0,	INPUT_DOUTU1,	INPUT_DOUTU2;
halt;

#else /* !(FIX_HW_BRN_25339) && !(FIX_HW_BRN_27330) */

/* Temp temps */
temp dword tmp0 = ds1[48];
temp dword tmp1 = ds1[49];
temp dword eventreg1;

/*****************************************************************************
* Setup the primary attributes
******************************************************************************/
#if defined(SUPPORT_SGX_HWPERF)
	/* Save the time stamp */
	mov32		tmp0, INPUT_TIMESTAMP_PA_DEST;
	movs	douta,	tim, tmp0;
#endif

/* Emit ir0 and ir1 */
mov32	tmp0, INPUT_IR0_PA_DEST;
movs	douta,	ir0,	tmp0;

mov32	tmp0, INPUT_IR1_PA_DEST;
movs	douta,	ir1,	tmp0;
#if defined(FIX_HW_BRN_31988)
movs	douta,	ir1,	tmp0;
movs	douta,	ir1,	tmp0;
#endif

#if defined(FIX_HW_BRN_31474)
mov32	tmp0, EURASIA_PDS_IR0_EDM_EVENT_SW_EVENT2;
and		tmp0, ir0, 	tmp0;
tstz	p0, tmp0;
p0	bra	NotCacheClearRequest;
	/* The ukernel has requested the PDS cache be cleared */
	bra	ClearLine2;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine2:
	bra	ClearLine3;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine3:
	bra	ClearLine4;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine4:
	bra	ClearLine5;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine5:
	bra	ClearLine6;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine6:
	bra	ClearLine7;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine7:
	bra	ClearLine8;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
	ClearLine8:
	/* Use the generic handler in case there was any events set */
	bra	UseGenericEventHandler;
	nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;
NotCacheClearRequest:
#endif

/* extract the event bits from ir1 */
mov32		tmp0, EURASIA_PDS_IR1_EDM_EVENT_KICKPTR_CLRMSK;
and		eventreg1, ir1, tmp0;

/* Check if the SW event bit is set */
mov32		tmp0, EURASIA_PDS_IR1_EDM_EVENT_SWEVENT;
and		tmp0,	eventreg1, tmp0;
/* Test whether any of the bits are set */
tstz	p0,	tmp0;
p0	bra	NotHostKick;
	bra	UseGenericEventHandler;
NotHostKick:

#if defined(SGX_FEATURE_2D_HARDWARE)
	/******************************************************************************
	* Check if it is a loopback for the 2D Module
	******************************************************************************/
	/* extract the 2D Bits from IR0 */
	mov32		tmp0, EURASIA_PDS_IR0_EDM_EVENT_TWODCOMPLETE;
	and		tmp0,	ir0,	tmp0;
	/* Test whether any of the bits are set */
	tstz	p0,	tmp0;
	p0	bra	Not2DModule;
		/* Check whether there are events not associated with this module */
		xor		tmp0, ir0, tmp0;
		tstz	p0, tmp0;
		p0	tstz	p0, eventreg1;
		p0	bra		Use2DEventHandler;
		bra	UseGenericEventHandler;
		mov32		tmp0, INPUT_DOUTU2;
		movs		doutu,		INPUT_2D_DOUTU0,	INPUT_DOUTU1,	tmp0;
		halt;
	Not2DModule:
#endif

/******************************************************************************
* Check if it is an event for the 3D Module
******************************************************************************/
/* extract the 3D Bits */
mov32		tmp0, EVENTS_3D_IR1;
and		tmp0,	eventreg1,	tmp0;
/* Test whether any of the bits are set */
tstz	p0,	tmp0;
#if defined(SGX_FEATURE_MP)
	p0	mov32		tmp1, EVENTS_3D_IR0;
	p0	and		tmp1, ir0, tmp1;
	p0	tstz	p0, tmp1;
#endif
p0	bra	Not3DModule;
	/* Check whether there are events not associated with this module */
	xor		tmp0, eventreg1, tmp0;
	tstz	p0, tmp0;
#if defined(SGX_FEATURE_MP)
	p0	xor	tmp1, ir0, tmp1;
	p0	tstz	p0, tmp1;
#else
	p0	tstz	p0, ir0;
#endif
	p0	bra		Use3DEventHandler;
	/* Not just 3D events */
	bra	UseGenericEventHandler;
	Use3DEventHandler:
	mov32		tmp0, INPUT_DOUTU2;
	movs		doutu,		INPUT_3D_DOUTU0,	INPUT_DOUTU1,	tmp0;
	halt;
Not3DModule:

/******************************************************************************
* Check if it is an event for the TA Module
******************************************************************************/
/* extract the TA Bits from IR0 */
mov32		tmp0, EVENTS_TA;
#if defined(SGX_FEATURE_MP)
and		tmp0, ir0, tmp0;
#else
and		tmp0, eventreg1, tmp0;
#endif
/* Test whether any of the bits are set */
tstz	p0, tmp0;
p0	bra	NotTAModule;
	/* Check whether there are events not associated with this module */
#if defined(SGX_FEATURE_MP)
	tstz	p0, eventreg1;
	p0	xor		tmp0, ir0, tmp0;
#else
	tstz	p0, ir0;
	p0	xor	tmp0, eventreg1, tmp0;
#endif
	p0 	tstz	p0, tmp0;
	p0	bra		UseTAEventHandler;
	bra	UseGenericEventHandler;
	UseTAEventHandler:
	mov32		tmp0, INPUT_DOUTU2;
	movs		doutu,		INPUT_TA_DOUTU0,	INPUT_DOUTU1,	tmp0;
	halt;
NotTAModule:

/******************************************************************************
* Check if it is an event for the SPM Module
******************************************************************************/
/* extract the SPM Bits from IR0 */
mov32		tmp0,	EVENTS_SPM;
#if defined(SGX_FEATURE_MP)
and		tmp0,	ir0,	tmp0;
#else
and		tmp0,	eventreg1,	tmp0;
#endif
/* Test whether any of the bits are set */
tstz	p0,	tmp0;
p0	bra	UseGenericEventHandler;
	/* Check whether there are events not associated with this module */
#if defined(SGX_FEATURE_MP)
	tstz	p0, eventreg1;
	p0 xor	tmp0, ir0, tmp0;
#else
	tstz	p0, ir0;
	p0 xor	tmp0, eventreg1, tmp0;
#endif
	tstz	p0, tmp0;
	p0	bra		UseSPMEventHandler;
	bra	UseGenericEventHandler;
	UseSPMEventHandler:
	mov32		tmp0, INPUT_DOUTU2;
	movs		doutu,		INPUT_SPM_DOUTU0,	INPUT_DOUTU1,	tmp0;
	halt;
UseGenericEventHandler:
/******************************************************************************
* Use Generic Event handler code as the events are not restricted to one module
* or it is a host kick event
******************************************************************************/
	
/* Issue USE task and exit */
mov32		tmp0, INPUT_DOUTU2;
movs		doutu,		INPUT_GENERIC_DOUTU0,	INPUT_DOUTU1,	tmp0;
halt;

#endif /* !FIX_HW_BRN_25339 && !(FIX_HW_BRN_27330) */


/******************************************************************************
 End of file (eventhandler.pds.asm)
******************************************************************************/
