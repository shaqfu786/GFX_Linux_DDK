/*!****************************************************************************
@File			os_srvclient_config.h

@Title			Code overlay configuration

@Author			Imagination Technologies

@Copyright     	Copyright 2008 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either
                material or conceptual may be copied or distributed,
                transmitted, transcribed, stored in a retrieval system
                or translated into any human or computer language in any
                form by any means, electronic, mechanical, manual or
                other-wise, or disclosed to third parties without the
                express written permission of Imagination Technologies
                Limited, Unit 8, HomePark Industrial Estate,
                King's Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform		generic

@Description	Configures overlay of bridged/XYZ and env/OS/XYZ code

@DoxygenVer		

Modifications :-

$Log: os_srvclient_config.h $

*/

#ifndef __OS_SRVCLIENT_CONFIG_H_
#define __OS_SRVCLIENT_CONFIG_H_

#include "pvrmmap.h"

#if !defined(SUPPORT_PVRSRV_GET_DC_SYSTEM_BUFFER)
#define OS_PVRSRV_GET_DC_SYSTEM_BUFFER_UM 1
#endif

#endif /* __OS_SRVCLIENTUM_CONFIG_H_ */

