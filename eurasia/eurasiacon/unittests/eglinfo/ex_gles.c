/***************************************************************************
 * Name:          ex_gles.c
 *
 * Author       : Yuan Wang
 * Created      : 06/05/2008
 *
 * Copyright    : 2004-2009 by Imagination Technologies Limited. All rights reserved.
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
 * $Log: ex_gles.c $
 *
 **************************************************************************/

#include <GLES/gl.h>
#include <GLES/glext.h>
#include "ex_gles.h"

#if defined(__linux__)
#define GLES_LIB_PATH		"libGLES_CM.so"
#define GLES_V1_LIB_PATH	"libGLESv1_CM.so"
#else
	#error "Platform lib name for OpenGLES1 required"
#endif

extern int giMode;

static void *gLibHandle = NULL;

typedef const GLubyte* (GL_APIENTRYP PFNGLGETSTRINGPROC) (GLenum name);
static PFNGLGETSTRINGPROC glGetStringTest;

typedef void (GL_APIENTRYP PFNGLGETINTEGERVPROC) (GLenum pname, GLint* params);
static PFNGLGETINTEGERVPROC glGetIntegervTest;

typedef void (GL_APIENTRYP PFNGLGETFLOATVPROC) (GLenum pname, GLfloat* params);
static PFNGLGETFLOATVPROC glGetFloatvTest;

static const struct glesStringInfoRec
{
	char *name;
	GLenum pname;
}
glesStringInfo[] =
{
	ATTRIB(GL_VENDOR),
	ATTRIB(GL_RENDERER),
	ATTRIB(GL_VERSION),
	ATTRIB(GL_EXTENSIONS),
};

static const struct glesIntegerInfoRec
{
	GLuint count;
	char *name;
	GLenum pname;
}
glesIntegerInfo[] =
{
	ATTRIBMORE(1, GL_MAX_LIGHTS),
	ATTRIBMORE(1, GL_MAX_CLIP_PLANES),
	ATTRIBMORE(1, GL_MAX_MODELVIEW_STACK_DEPTH),
	ATTRIBMORE(1, GL_MAX_PROJECTION_STACK_DEPTH),
	ATTRIBMORE(1, GL_MAX_TEXTURE_STACK_DEPTH),
	ATTRIBMORE(1, GL_SUBPIXEL_BITS),
	ATTRIBMORE(1, GL_MAX_TEXTURE_SIZE),
	ATTRIBMORE(2, GL_MAX_VIEWPORT_DIMS),
	ATTRIBMORE(1, GL_MAX_TEXTURE_UNITS),
	ATTRIBMORE(1, GL_NUM_COMPRESSED_TEXTURE_FORMATS),
	/* the following information is shared with egl configs
	ATTRIBMORE(1, GL_SAMPLE_BUFFERS),
	ATTRIBMORE(1, GL_SAMPLES),
	ATTRIBMORE(1, GL_RED_BITS),
	ATTRIBMORE(1, GL_GREEN_BITS),
	ATTRIBMORE(1, GL_BLUE_BITS),
	ATTRIBMORE(1, GL_ALPHA_BITS),
	ATTRIBMORE(1, GL_DEPTH_BITS),
	ATTRIBMORE(1, GL_STENCIL_BITS),*/
};

static const struct glesFloatInfoRec
{
	GLuint count;
	char *name;
	GLenum pname;
}
glesFloatInfo[] =
{
	ATTRIBMORE(2, GL_ALIASED_POINT_SIZE_RANGE),
	ATTRIBMORE(2, GL_SMOOTH_POINT_SIZE_RANGE),
	ATTRIBMORE(2, GL_ALIASED_LINE_WIDTH_RANGE),
	ATTRIBMORE(2, GL_SMOOTH_LINE_WIDTH_RANGE),
};

__internal void PrintOGLES1APIInfo(void)
{
	GLuint i;

	struct glesStringInfoRec const *glesStringInfoP, *glesStringInfoEnd;
	char *stringvalue;

	struct glesIntegerInfoRec const *glesIntegerInfoP, *glesIntegerInfoEnd;
	GLint intvalue[4];

	struct glesFloatInfoRec const *glesFloatInfoP, *glesFloatInfoEnd;
	GLfloat fvalue[16];

	if (!(giMode & XINFO_ES1))
		return;

	INFO("\nOpenGL ES 1 Information:\n");

	/************** print out gles string info *****************************/

	INFO("\nGLES String Information :\n");

	glesStringInfoP = glesStringInfo;
	glesStringInfoEnd = glesStringInfoP + (sizeof(glesStringInfo)) / sizeof(struct glesStringInfoRec);

	while (glesStringInfoP < glesStringInfoEnd)
	{
		stringvalue = (char *)glGetStringTest(glesStringInfoP->pname);

		INFO(" %s = %s \n", glesStringInfoP->name, stringvalue);

		glesStringInfoP++;
	}


	/****************** print out gles integer info **********************************/

	INFO("\nGLES Integer Information :\n");

	glesIntegerInfoP = glesIntegerInfo;
	glesIntegerInfoEnd = glesIntegerInfoP + (sizeof(glesIntegerInfo)) / sizeof(struct glesIntegerInfoRec);

	while (glesIntegerInfoP < glesIntegerInfoEnd)
	{
		glGetIntegervTest(glesIntegerInfoP->pname, intvalue);

		INFO(" %s = ", glesIntegerInfoP->name);

		for (i=0; i < glesIntegerInfoP->count; i++)
		{
			INFO(" 0x%x ", intvalue[i]);
		}

		INFO("\n");

		glesIntegerInfoP++;
	}


	/***************** print out gles float info *******************************/

	INFO("\nGLES Float Information :\n");

	glesFloatInfoP = glesFloatInfo;
	glesFloatInfoEnd = glesFloatInfoP + (sizeof(glesFloatInfo)) / sizeof(struct glesFloatInfoRec);

	while (glesFloatInfoP < glesFloatInfoEnd)
	{
		glGetFloatvTest(glesFloatInfoP->pname, fvalue);

		INFO(" %s = ", glesFloatInfoP->name); 

		for (i=0; i < glesFloatInfoP->count; i++)
		{
			INFO(" %f ", fvalue[i]);
		}

		INFO("\n");

		glesFloatInfoP++;
	}
}

__internal int TryLoadOGLES1Library(void)
{
	if(gLibHandle)
		return 1;

	/* Try the non-V1 library first */
	gLibHandle = exLoadLibrary(GLES_LIB_PATH);
	if(!gLibHandle)
	{
		/* Non-V1 library load failed; try the V1 library */
		gLibHandle = exLoadLibrary(GLES_V1_LIB_PATH);

		/* No point in continuing if there's no GL.. */
		if(!gLibHandle)
			return 0;
	}

	if(!exGetLibFuncAddr(gLibHandle, "glGetString", (GENERICPF *)&glGetStringTest))
		return 0;

	if(!exGetLibFuncAddr(gLibHandle, "glGetIntegerv", (GENERICPF *)&glGetIntegervTest))
		return 0;

	if(!exGetLibFuncAddr(gLibHandle, "glGetFloatv", (GENERICPF *)&glGetFloatvTest))
		return 0;

	return 1;
}

__internal void UnloadOGLES1Library(void)
{
	if(gLibHandle)
		exUnloadLibrary(gLibHandle);
}

__internal const char *GetOGLES1VendorString(void)
{
	if(!glGetStringTest)
		return NULL;
	return (const char *)glGetStringTest(GL_VENDOR);
}
