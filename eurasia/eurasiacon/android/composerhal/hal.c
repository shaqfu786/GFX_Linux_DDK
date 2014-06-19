/*!****************************************************************************
@File           hwcomposer.c

@Title          Hardware Composer (hwcomposer) HAL component

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

#include <hardware/hwcomposer.h>

#include "pvr_debug.h"
#include "services.h"

#include "hwc_private.h"
#include "hwc.h"

#include <string.h>
#include <errno.h>

static int
OpenGraphicsHAL(IMG_gralloc_module_public_t **ppsGrallocModule)
{
	IMG_gralloc_module_public_t *psGrallocModule;
	int err;

	ENTERED();

	err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID,
						(const hw_module_t**)&psGrallocModule);
	if(err)
		goto err_out;

	if(strcmp(psGrallocModule->base.common.author,
			  "Imagination Technologies"))
	{
		err = -EINVAL;
		goto err_out;
	}

	*ppsGrallocModule = psGrallocModule;

exit_out:
	EXITED();
	return err;

err_out:
	PVR_DPF((PVR_DBG_ERROR, "%s: Composer HAL failed to load "
							"compatible Graphics HAL", __func__));
	goto exit_out;
}

static int hal_open(const hw_module_t *module, const char *id,
					hw_device_t **device)
{
	IMG_framebuffer_device_public_t **ppsFrameBufferDevice =
		&((IMG_hwc_module_t *)module)->psFrameBufferDevice;
	IMG_gralloc_module_public_t **ppsGrallocModule =
		&((IMG_hwc_module_t *)module)->psGrallocModule;

	if(!(*ppsFrameBufferDevice))
	{
		int err;

		err = OpenGraphicsHAL(ppsGrallocModule);
		if(err)
			return err;

		*ppsFrameBufferDevice = (*ppsGrallocModule)->psFrameBufferDevice;

		if(!(*ppsFrameBufferDevice))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Framebuffer HAL not "
									"opened before HWC", __func__));
			return -EFAULT;
		}

		(*ppsFrameBufferDevice)->bBypassPost = IMG_TRUE;
	}

	if(strcmp(id, HWC_HARDWARE_COMPOSER))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid open ID '%s'", __func__, id));
		return -EFAULT;
	}

	return hwc_setup(module, device);
}

static hw_module_methods_t hal_module_methods =
{
	.open = hal_open,
};

IMG_EXPORT IMG_hwc_module_t HAL_MODULE_INFO_SYM =
{
	.base =
	{
		/* Must be first (Android casts to hw_module_t) */
		.common	=
		{
			.tag			= HARDWARE_MODULE_TAG,
			.version_major	= 1,
			.version_minor	= 0,
			.id				= HWC_HARDWARE_MODULE_ID,
			.name			= "IMG SGX Hardware Composer HAL",
			.author			= "Imagination Technologies",
			.methods		= &hal_module_methods,
		},
	},

	/* Private data implicitly initialised to NULL */
};

/* Called on first dlopen() */
static void __attribute__((constructor)) hal_init(void)
{
	PVR_DPF((PVR_DBG_MESSAGE, "%s: IMG Composer HAL loaded", __func__));
}

/* Called on dlclose() */
static void __attribute__((destructor)) hal_exit(void)
{
	PVR_DPF((PVR_DBG_MESSAGE, "%s: IMG Composer HAL unloaded", __func__));
}
