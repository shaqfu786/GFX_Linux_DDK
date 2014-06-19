#include <stdlib.h>
#include <stdio.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include "pvr_debug.h"
#include "srvinit.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "IMGSRV"
#endif

#define COMMAND_MAX	256

int main(int argc, char** argv) {
	char command[COMMAND_MAX];
	struct board_properties props = {0};
	int ret = -1;
	char value[PROPERTY_VALUE_MAX];

	/* quiet the compiler */
	PVR_UNREFERENCED_PARAMETER(argc);
	PVR_UNREFERENCED_PARAMETER(argv);

	/* check if pvr is built into kernel to maintain backwards compatability */
	FILE *pvrInKernel = fopen("/proc/pvr/version", "r");

	if(!DetectPlatform(&props)) {
		LOGE("Could not detect platform");
		goto err_out;
	}

	if(pvrInKernel) {
		/* skip module loading */
		fclose(pvrInKernel);
	}
	else {
		/* load pvrsrvkm module */
		if(!LoadModules(props.sgx_core, props.sgx_rev)) {
			LOGE("Cannot load gfx modules!");
			goto err_out;
		}
		LOGV("Succesfully loaded gfx modules");
	}

	/* set property so libhardware can load correct gralloc */
	snprintf(command, COMMAND_MAX, "omap%d", props.chip_family);
	property_set("ro.product.processor", command);

	/* set property for surfaceflinger to initialize its client buffers.
	 * if the property is already defined, re-set props.num_bufs */
	if (property_get("surfaceflingerclient.numbuffers", value, NULL))
		props.num_bufs = atoi(value);

	if (props.num_bufs) {
		snprintf(command, COMMAND_MAX, "%d", props.num_bufs);
		property_set("surfaceflingerclient.numbuffers", command);
	}

	/* remount system */
//	system("mount -o remount rw /system");

	/* symlink libPVRScopeServices */
//	snprintf(command, COMMAND_MAX, "ln -s /vendor/lib/libPVRScopeServices_SGX%d_%d.so /vendor/lib/libPVRScopeServices.so", props.sgx_core, props.sgx_rev);
//	system(command);

	/* load pvrsrvinit */
	snprintf(command, COMMAND_MAX, "/vendor/bin/pvrsrvinit_SGX%d_%d", props.sgx_core, props.sgx_rev);
	system(command);

	/* mount as ro */
//	system("mount -o remount ro /system");

	/* success */
	ret = 0;

	LOGV("pvrsrvinit complete");
err_out:
	if(ret == -1)
		LOGE("pvrsrvinit failed to initialize graphics modules!!!");

	return ret;
}
