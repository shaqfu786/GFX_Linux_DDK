/*!****************************************************************************
@File           fdsocket.h

@Title          Graphics HAL (fd passing)

@Author         Imagination Technologies

@Date           2009/03/19

@Copyright      Copyright 2009 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       linux

@Description    Graphics HAL (fd passing)

******************************************************************************/
/******************************************************************************
Modifications :-
$Log: fdsocket.h $
******************************************************************************/

#ifndef FDSOCKET_H
#define FDSOCKET_H

#include "img_types.h"

#include <hardware/gralloc.h>

IMG_BOOL waitForClientSendHandle(native_handle_t *psNativeHandle);
IMG_BOOL connectServerRecvHandle(native_handle_t **ppsNativeHandle);

#endif /* FDSOCKET_H */
