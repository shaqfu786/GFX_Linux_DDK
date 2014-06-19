/*************************************************************************/ /*!
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Strictly Confidential.
*/ /**************************************************************************/

#define _GNU_SOURCE
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "img_defs.h"
#include "servicesext.h"
#include "pvr_debug.h"
#include "srvinit.h"

#if defined(SUPPORT_ANDROID_PLATFORM)
#define DPF(x...) printf(x)
#else
#define DPF(x...) fprintf(stderr, x)
#endif

#define MODULE_PATH_MAX	256
#define BUF_MAX		64

enum { INVALID, START, STOP, RESTART };

#if defined(BUILD_PVRSRVCTL)
static void usage(char *argv0)
{
	DPF("Usage: %s [OPTION]\n", argv0);
	DPF("Load and initialize (or unload) PowerVR services.\n\n");
	DPF("  --start\tstart PowerVR services (may load kernel module)\n");
	DPF("  --stop\tstop PowerVR services (may unload kernel module)\n");
	DPF("  --restart\tstops and starts PowerVR services\n");
	DPF("  --no-module\tsuppresses kernel module load/unload\n");
	DPF("  --help\tdisplay this help and exit\n");
}
#endif

IMG_BOOL DetectPlatform(struct board_properties *props)
{
	FILE *pFile;
	int bytes_read;
	char in[BUF_MAX];

	/* query kernel for OMAP family */
	pFile = fopen("/sys/board_properties/soc/family", "r");
	if(pFile) {
		bytes_read = fread(in, BUF_MAX, 1, pFile);
		if(bytes_read < 0) {
			DPF("Cannot read /sys/board_properties/soc/family\n");
			goto err_out;
		}
		if(sscanf(in, "OMAP%d", &props->chip_family) != 1) {
			DPF("sscanf chip_family failed\n");
			goto err_out;
		}
	}
	else {
		DPF("Cannot open /sys/board_properties/soc/family\n");
		goto err_out;
	}

	/* query kernel for OMAP revision */
	pFile = fopen("/sys/board_properties/soc/revision", "r");
	if(pFile) {
		bytes_read = fread(in, BUF_MAX, 1, pFile);
		if(bytes_read < 0) {
			DPF("Cannot read /sys/board_properties/soc/revision\n");
			goto err_out;
		}

		if(sscanf(in, "ES%d.%d", &props->chip_rev_maj, &props->chip_rev_min) != 2) {
			DPF("sscanf chip_rev failed\n");
			goto err_out;
		}
	}
	else {
		DPF("Cannot open /sys/board_properties/soc/revision\n");
		goto err_out;
	}

	DPF("Detected platform as OMAP%d ES%d.%d\n", props->chip_family, props->chip_rev_maj, props->chip_rev_min);

	/* we can add chip_rev to switch once we need to */
	switch(props->chip_family) {
		case 4430:
			props->sgx_core = 540;
			props->sgx_rev = 120;
			break;
		case 4460:
			props->sgx_core = 540;
			props->sgx_rev = 120;
			break;
		case 4470:
			props->sgx_core = 544;
			props->sgx_rev = 112;
			break;
		case 5430:
			props->sgx_core = 544;
			props->sgx_rev = 105;
			break;
		default:
			DPF("Unknown chipset...\n");
			goto err_out;
	}

	/* newer sgx cores (>=544) have a larger address space and thus can
	   handle triple buffering */
	if(props->sgx_core >= 544)
		props->num_bufs = 3;

	return IMG_TRUE;

err_out:
	fclose(pFile);
	return IMG_FALSE;
}

static IMG_BOOL LoadModule(const char *szPath, const char *szArgs)
{
	IMG_BOOL bStatus = IMG_FALSE;
	void *pvFileData;
	long lFileLen;
	int err;

	FILE *f = fopen(szPath, "r");
	if(!f)
		goto err_out;

	if(fseek(f, 0, SEEK_END))
		goto err_close;

	lFileLen = ftell(f);
	if(lFileLen < 0)
		goto err_close;
	rewind(f);

	pvFileData = malloc(lFileLen);
	if(!pvFileData)
		goto err_close;

	if(fread(pvFileData, lFileLen, 1, f) != 1)
		goto err_free;

	err = syscall(__NR_init_module, pvFileData, (unsigned long)lFileLen, szArgs);
	if(err && errno != EEXIST)
		goto err_free;

	bStatus = IMG_TRUE;
err_free:
	free(pvFileData);
err_close:
	fclose(f);
err_out:
	return bStatus;
}

static IMG_BOOL UnloadModule(const char *szModule)
{
	int i, iRet;

	for(i = 0; i < 10; i++)
	{
		iRet = syscall(__NR_delete_module, szModule, O_NONBLOCK | O_EXCL);
		if(iRet < 0 && errno == EAGAIN)
			usleep(500000);
		else
			break;
	}

	return iRet < 0 ? IMG_FALSE : IMG_TRUE;
}

#if defined(DISPLAY_CONTROLLER)
#define XSTR(s)		#s 
#define STR(s)		XSTR(s)
#define DC_MODNAME	STR(DISPLAY_CONTROLLER)
#endif

IMG_BOOL LoadModules(int sgx_core, int sgx_rev)
{
	char module_path[MODULE_PATH_MAX];

#if defined(PDUMP)
	snprintf(module_path, MODULE_PATH_MAX, PVRSRV_MODULE_BASEDIR "dbgdrv_sgx%d_%d.ko", sgx_core, sgx_rev);
	if(!LoadModule(module_path, ""))
	{
		DPF("Failed to load %s\n",module_path) ;
		return IMG_FALSE;
	}
#endif

	snprintf(module_path, MODULE_PATH_MAX, PVRSRV_MODULE_BASEDIR PVRSRV_MODNAME "_sgx%d_%d.ko", sgx_core, sgx_rev);
	if(!LoadModule(module_path, ""))
	{
		DPF("Failed to load %s\n",module_path) ;
		return IMG_FALSE;
	}

#if defined(DC_MODNAME)
	snprintf(module_path, MODULE_PATH_MAX, PVRSRV_MODULE_BASEDIR DC_MODNAME "_sgx%d_%d.ko", sgx_core, sgx_rev);
	if(!LoadModule(module_path, ""))
	{
		DPF("Failed to load %s\n",module_path) ;
		return IMG_FALSE;
	}
#endif

	return IMG_TRUE;
}

IMG_BOOL UnloadModules(int sgx_core, int sgx_rev)
{
	char module_path[MODULE_PATH_MAX];

#if defined(DC_MODNAME)
	snprintf(module_path, MODULE_PATH_MAX, DC_MODNAME "_sgx%d_%d.ko", sgx_core, sgx_rev);
	if(!UnloadModule(module_path))
	{
		DPF("Failed to unload %s\n", module_path);
		return IMG_FALSE;
	}
#endif

	snprintf(module_path, MODULE_PATH_MAX, PVRSRV_MODNAME "_sgx%d_%d.ko", sgx_core, sgx_rev);
	if(!UnloadModule(module_path))
	{
		DPF("Failed to unload %s\n", module_path);
		return IMG_FALSE;
	}

#if defined(PDUMP)
	snprintf(module_path, MODULE_PATH_MAX, "dbgdrv_sgx%d_%d.ko", sgx_core, sgx_rev);
	if(!UnloadModule(module_path))
	{
		DPF("Failed to unload %s\n", module_path);
		return IMG_FALSE;
	}
#endif

	return IMG_TRUE;
}

#if defined(BUILD_PVRSRVCTL)
int main(int argc, char *argv[])
{
	int err = 1, iMode = INVALID, iModule = 1;
	struct board_properties props = {0};

	const struct option acsOptions[] =
	{
		{ "start",		no_argument, &iMode, START },
		{ "stop",		no_argument, &iMode, STOP },
		{ "reload",		no_argument, &iMode, RESTART },
		{ "restart",	no_argument, &iMode, RESTART },
		{ "no-module",	no_argument, &iModule, 0 },
		{ "help",		no_argument, NULL, 'h' },
		{ NULL, 0, NULL, 0 },
	};

	if(!DetectPlatform(&props)) {
		DPF("Could not detect platform\n");
		goto err_out;
	}

	while(1)
	{
		int iRet = getopt_long(argc, argv, "", acsOptions, NULL);
		if(iRet == '?')
		{
			usage(argv[0]);
			goto err_out;
		}
		else if(iRet == 'h')
		{
			usage(argv[0]);
			goto exit_out;
		}
		else if(iRet == -1)
		{
			break;
		}
	}

	if(iMode == INVALID)
	{
		DPF("%s: must specify mode of operation\n", argv[0]);
		usage(argv[0]);
		goto err_out;
	}

	if(iModule && (iMode == RESTART || iMode == STOP))
	{
		if(!UnloadModules(props.sgx_core, props.sgx_rev))
			goto err_out;
	}

	if(iMode == RESTART || iMode == START)
	{
		PVRSRV_ERROR eError;

		if(iModule)
		{
			if(!LoadModules(props.sgx_core, props.sgx_rev))
				goto err_out;
		}

		eError = SrvInit();
		if (eError != PVRSRV_OK)
		{
			DPF("%s: SrvInit failed (already initialized?) (err=%d)\n",
				argv[0], eError);
			goto err_out;
		}
	}

exit_out:
	err = 0;
err_out:
	return err;
}
#endif
