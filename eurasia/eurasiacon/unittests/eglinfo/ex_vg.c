/***************************************************************************
 * Name:   ex_vg.c
 *
* Author       : Yuan Wang
 * Created      : 07/05/2008
 *
 * Copyright    : 2004-2008 by Imagination Technologies Limited. All rights reserved.
 *              : No part of this software, either material or conceptual 
 *              : may be copied or distributed, transmitted, transcribed,
 *              : stored in a retrieval system or translated into any 
 *              : human or computer language in any form by any means,
 *              : electronic, mechanical, manual or other-wise, or 
 *              : disclosed to third parties without the express written
 *              : permission of Imagination Technologies Limited, Unit 8, HomePark
 *              : Industrial Estate, King's Langley, Hertfordshire,
 *              : WD4 8LZ, U.K.
 *
 * Platform     : ANSI
 * $Log: ex_vg.c $
 * 
 *  --- Revision Logs Removed --- 
 *
 **************************************************************************/

#include <VG/openvg.h>
#include "ex_vg.h"

#ifndef VG_APIENTRYP
#define VG_APIENTRYP VG_APIENTRY*
#endif

#if defined(__linux__)
#define OVG_LIB_PATH "libOpenVG.so"
#else
	#error "Platform lib name for OpenVG required"
#endif

extern int giMode;

static void *gLibHandle;

typedef const VGubyte* (VG_APIENTRYP PFNVGGETSTRING) (VGStringID name);
static PFNVGGETSTRING vgGetStringTest;

typedef VGint (VG_APIENTRYP PFNVGGETI) (VGParamType type);
static PFNVGGETI vgGetiTest;

typedef VGfloat (VG_APIENTRYP PFNVGGETF) (VGParamType type);
static PFNVGGETF vgGetfTest;

static const struct vgStringInfoRec
{
	char *name;
	VGStringID vgname;
}
vgStringInfo[] =
{
	ATTRIB(VG_VENDOR),
	ATTRIB(VG_RENDERER),
	ATTRIB(VG_VERSION),
	ATTRIB(VG_EXTENSIONS),
};
 
static const struct vgIntegerInfoRec
{
	char *name;
	VGParamType type;
}
vgIntegerInfo[] =
{
	ATTRIB(VG_MAX_SCISSOR_RECTS),
	ATTRIB(VG_MAX_DASH_COUNT),
	ATTRIB(VG_MAX_KERNEL_SIZE),
	ATTRIB(VG_MAX_SEPARABLE_KERNEL_SIZE),
	ATTRIB(VG_MAX_COLOR_RAMP_STOPS),
	ATTRIB(VG_MAX_IMAGE_WIDTH),
	ATTRIB(VG_MAX_IMAGE_HEIGHT),
	ATTRIB(VG_MAX_IMAGE_PIXELS),
	ATTRIB(VG_MAX_IMAGE_BYTES),
	ATTRIB(VG_MAX_GAUSSIAN_STD_DEVIATION),
};

static const struct vgFloatInfoRec
{
	char *name;
	VGParamType type;
}
vgFloatInfo[] =
{
	ATTRIB(VG_MAX_FLOAT),
};

__internal void PrintOVGAPIInfo(void)
{
	struct vgStringInfoRec const *vgStringInfoP, *vgStringInfoEnd;
	char *stringvalue;

	struct vgIntegerInfoRec const *vgIntegerInfoP, *vgIntegerInfoEnd;
	VGint intvalue;

	struct vgFloatInfoRec const *vgFloatInfoP, *vgFloatInfoEnd;
	VGfloat floatvalue;

    if (!(giMode & XINFO_VG))
		return;

	INFO("\nOpen VG Information:\n");

	/********************** print out OpenVG String Parameter Info ********************************/

	INFO("\nVG API String Information :\n");

	vgStringInfoP = vgStringInfo;
	vgStringInfoEnd = vgStringInfoP + (sizeof(vgStringInfo)) / sizeof(struct vgStringInfoRec);

	while (vgStringInfoP < vgStringInfoEnd)
	{
		stringvalue = (char *)vgGetStringTest(vgStringInfoP->vgname);

		INFO(" %s = %s \n", vgStringInfoP->name, stringvalue);

		vgStringInfoP++;
	}


	/****************** print out OpenVG integer info **********************************/

	INFO("\nVG Integer Information :\n");

	vgIntegerInfoP = vgIntegerInfo;
	vgIntegerInfoEnd = vgIntegerInfoP + (sizeof(vgIntegerInfo)) / sizeof(struct vgIntegerInfoRec);

	while (vgIntegerInfoP < vgIntegerInfoEnd)
	{
		intvalue = vgGetiTest(vgIntegerInfoP->type);

		INFO(" %s = 0x%x \n", vgIntegerInfoP->name, intvalue);

		vgIntegerInfoP++;
	}


	/********************** print out OpenVG API Float Parameter Info ********************************/

	INFO("\nVG API Float Parameter Information :\n");

	vgFloatInfoP = vgFloatInfo;
	vgFloatInfoEnd = vgFloatInfoP + (sizeof(vgFloatInfo)) / sizeof(struct vgFloatInfoRec);

	while (vgFloatInfoP < vgFloatInfoEnd)
	{
		floatvalue = vgGetfTest(vgFloatInfoP->type);

		INFO(" %s = %E \n", vgFloatInfoP->name, floatvalue);

		vgFloatInfoP++;
	}
}

__internal int TryLoadOVGLibrary(void)
{
	if(gLibHandle)
		return 1;

	gLibHandle = exLoadLibrary(OVG_LIB_PATH);
	if(!gLibHandle)
	{
		/* No point in continuing if there's no VG library.. */
		return 0;
	}

	if(!exGetLibFuncAddr(gLibHandle, "vgGetString", (GENERICPF *)&vgGetStringTest))
		return 0;

	if(!exGetLibFuncAddr(gLibHandle, "vgGeti", (GENERICPF *)&vgGetiTest))
		return 0;

	if(!exGetLibFuncAddr(gLibHandle, "vgGetf", (GENERICPF *)&vgGetfTest))
		return 0;

	return 1;
}

__internal void UnloadOVGLibrary(void)
{
	if(gLibHandle)
		exUnloadLibrary(gLibHandle);
}

__internal const char *GetOVGVendorString(void)
{
	if(!vgGetStringTest)
		return NULL;
	return (const char *)vgGetStringTest(VG_VENDOR);
}
