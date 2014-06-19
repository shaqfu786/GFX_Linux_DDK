/*!****************************************************************************
@File           hwc_private.h

@Title          Internal composer HAL decls

@Author         Imagination Technologies

@Date           2010/09/15

@Copyright      Copyright (C) Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       linux

******************************************************************************/

#ifndef HWC_PRIVATE_H
#define HWC_PRIVATE_H

#include "hal_public.h"

#ifdef HAL_ENTRYPOINT_DEBUG
#define ENTERED() \
	PVR_DPF((PVR_DBG_MESSAGE, "%s: entered function", __func__));
#define EXITED() \
	PVR_DPF((PVR_DBG_MESSAGE, "%s: left function", __func__));
#else
#define ENTERED()
#define EXITED()
#endif

typedef struct
{
	hwc_module_t base;
	IMG_framebuffer_device_public_t *psFrameBufferDevice;
	IMG_gralloc_module_public_t *psGrallocModule;
}
IMG_hwc_module_t;

#endif /* HWC_PRIVATE_H */
