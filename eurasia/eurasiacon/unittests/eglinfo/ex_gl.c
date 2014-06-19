/***************************************************************************
 * Name:          ex_gl.c
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
 * $Log: ex_gl.c $
 *
 **************************************************************************/


#include "ex_gl.h"
#include <GL/gl.h>

#ifndef APIENTRYP
#define APIENTRYP APIENTRY*
#endif

#if defined(__linux__)
#define GL_LIB_PATH "libGL.so"
#else
	#error "Platform lib name for OpenGL required"
#endif

extern int giMode;

static void *gLibHandle;

typedef const /*WINGDIAPI*/ GLubyte* (APIENTRYP PFNGLGETSTRINGPROC) (GLenum name);
static PFNGLGETSTRINGPROC glGetStringTest;

typedef /*WINGDIAPI*/ void (APIENTRYP PFNGLGETINTEGERVPROC) (GLenum pname, GLint* params);
static PFNGLGETINTEGERVPROC glGetIntegervTest;

typedef /*WINGDIAPI*/ void (APIENTRYP PFNGLGETFLOATVPROC) (GLenum pname, GLfloat* params);
static PFNGLGETFLOATVPROC glGetFloatvTest;

static const struct glStringInfoRec
{
	char *name;
	GLenum pname;
}
glStringInfo[] =
{
	ATTRIB(GL_VENDOR),
	ATTRIB(GL_RENDERER),
	ATTRIB(GL_VERSION),
	ATTRIB(GL_EXTENSIONS),
};

static const struct glIntegerInfoRec
{
	GLuint count;
	char *name;
	GLenum pname;

}
glIntegerInfo[] =
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
	ATTRIBMORE(1, GL_STENCIL_BITS), */
};

static const struct glFloatInfoRec
{
	GLuint count;
	char *name;
	GLenum pname;
}
glFloatInfo[] =
{
	ATTRIBMORE(2, GL_ALIASED_POINT_SIZE_RANGE),
	ATTRIBMORE(2, GL_SMOOTH_POINT_SIZE_RANGE),
	ATTRIBMORE(2, GL_ALIASED_LINE_WIDTH_RANGE),
	ATTRIBMORE(2, GL_SMOOTH_LINE_WIDTH_RANGE),
};

__internal void PrintOGLAPIInfo(void)
{
	GLuint i;

	struct glStringInfoRec const *glStringInfoP, *glStringInfoEnd;
	char *stringvalue;

	struct glIntegerInfoRec const *glIntegerInfoP, *glIntegerInfoEnd;
	GLint intvalue[4];

	struct glFloatInfoRec const *glFloatInfoP, *glFloatInfoEnd;
	GLfloat fvalue[16];

    if (!(giMode & XINFO_ES2))
		return;

	INFO("\nOpenGL Information: \n");

	/************** print out gl string info *****************************/

	INFO("\nGL String Information :\n");

	glStringInfoP = glStringInfo;
	glStringInfoEnd = glStringInfoP + (sizeof(glStringInfo)) / sizeof(struct glStringInfoRec);

	while (glStringInfoP < glStringInfoEnd)
	{
		stringvalue = (char *)glGetStringTest(glStringInfoP->pname);

		INFO(" %s = %s \n", glStringInfoP->name, stringvalue);

		glStringInfoP++;
	}


	/****************** print out gl integer info **********************************/

	INFO("\nGL Integer Information :\n");

	glIntegerInfoP = glIntegerInfo;
	glIntegerInfoEnd = glIntegerInfoP + (sizeof(glIntegerInfo)) / sizeof(struct glIntegerInfoRec);

	while (glIntegerInfoP < glIntegerInfoEnd)
	{
		glGetIntegervTest(glIntegerInfoP->pname, intvalue);

		INFO(" %s = ", glIntegerInfoP->name);

		for (i=0; i < glIntegerInfoP->count; i++)
		{
			INFO(" 0x%x ", intvalue[i]);
		}

		INFO("\n");

		glIntegerInfoP++;
	}


	/***************** print out gl float info *******************************/

	INFO("\nGL Float Information :\n");

	glFloatInfoP = glFloatInfo;
	glFloatInfoEnd = glFloatInfoP + (sizeof(glFloatInfo)) / sizeof(struct glFloatInfoRec);

	while (glFloatInfoP < glFloatInfoEnd)
	{
		glGetFloatvTest(glFloatInfoP->pname, fvalue);

		INFO(" %s = ", glFloatInfoP->name); 

		for (i=0; i < glFloatInfoP->count; i++)
		{
			INFO(" %f ", fvalue[i]);
		}

		INFO("\n");

		glFloatInfoP++;
	}
}

__internal int TryLoadOGLLibrary(void)
{
	if(gLibHandle)
		return 1;

	gLibHandle = exLoadLibrary(GL_LIB_PATH);
	if(!gLibHandle)
	{
		/* No GL, no point in continuing */
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

__internal void UnloadOGLLibrary(void)
{
	if(gLibHandle)
		exUnloadLibrary(gLibHandle);
}

__internal const char *GetOGLVendorString(void)
{
	if(!glGetStringTest)
		return NULL;
	return (const char *)glGetStringTest(GL_VENDOR);
}
