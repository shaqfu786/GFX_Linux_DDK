/*!****************************************************************************
@File           sgxkick_client.h

@Title          sgx services structures/functions

@Author         Imagination Technologies

@Date           06 / 12 / 07

@Copyright      Copyright 2007-2008 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed to
                third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       Generic

@Description    SGXKickTA interface

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-
$Log: sgxkick_client.h $
*****************************************************************************/
#ifndef __SGXKICK_CLIENT_H__
#define __SGXKICK_CLIENT_H__

IMG_EXPORT
PVRSRV_ERROR SGXKickTAUM(PVRSRV_DEV_DATA *psDevData,
						 SGX_KICKTA *psKickTA,
						 SGX_KICKTA_OUTPUT		*psKickOutput,
						 IMG_PVOID					pvKickPDUMP,
						 PSGX_KICKTA_SUBMIT	psKickSubmit);

#endif /* __SGXKICK_CLIENT_H__ */

