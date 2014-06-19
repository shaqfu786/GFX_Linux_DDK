/************************************************************************
 * Name:          ex_gles2.c
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
 * $Log: ex_gles2.c $
 *
 **************************************************************************/

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "ex_gles2.h"

#if defined(__linux__)
#define GLES2_LIB_PATH "libGLESv2.so"
#else
	#error "Platform lib name for OpenGLES2 required"
#endif

extern int giMode;

static void *gLibHandle = NULL;

typedef const GLubyte* (GL_APIENTRYP PFNGLGETSTRINGPROC) (GLenum name);
static PFNGLGETSTRINGPROC glGetStringTest;

typedef void (GL_APIENTRYP PFNGLGETINTEGERVPROC) (GLenum pname, GLint* params);
static PFNGLGETINTEGERVPROC gl2GetIntegervTest;

typedef void (GL_APIENTRYP PFNGLGETBOOLEANVPROC) (GLenum pname, GLboolean* params);
static PFNGLGETBOOLEANVPROC glGetBooleanvTest;

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
	ATTRIB(GL_SHADING_LANGUAGE_VERSION),
	ATTRIB(GL_EXTENSIONS),
};

static const struct glesBooleanInfoRec
{
	GLuint count;
	char *name;
	GLenum pname;
}
glesBooleanInfo[] =
{
	ATTRIBMORE(1, GL_SHADER_COMPILER),
	ATTRIBMORE(4, GL_COLOR_WRITEMASK),
	ATTRIBMORE(4, GL_DEPTH_WRITEMASK),
};

static const struct glesIntegerInfoRec
{
	GLuint count;
	char *name;
	GLenum pname;
}
glesIntegerInfo[] =
{
	ATTRIBMORE(1, GL_SUBPIXEL_BITS),
	ATTRIBMORE(1, GL_MAX_TEXTURE_SIZE),
	ATTRIBMORE(1, GL_MAX_CUBE_MAP_TEXTURE_SIZE),
	ATTRIBMORE(2, GL_MAX_VIEWPORT_DIMS),
	ATTRIBMORE(1, GL_NUM_COMPRESSED_TEXTURE_FORMATS),
	/*  ATTRIBMORE(5, GL_COMPRESSED_TEXTURE_FORMATS), */
	ATTRIBMORE(1, GL_NUM_SHADER_BINARY_FORMATS),
	/*  ATTRIBMORE(5, GL_SHADER_BINARY_FORMATS),*/
	ATTRIBMORE(1, GL_MAX_VERTEX_ATTRIBS),
	ATTRIBMORE(1, GL_MAX_VERTEX_UNIFORM_VECTORS),
	ATTRIBMORE(1, GL_MAX_VARYING_VECTORS),
	ATTRIBMORE(1, GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS),
	ATTRIBMORE(1, GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS),
	ATTRIBMORE(1, GL_MAX_TEXTURE_IMAGE_UNITS),
	ATTRIBMORE(1, GL_MAX_FRAGMENT_UNIFORM_VECTORS),
	ATTRIBMORE(1, GL_MAX_RENDERBUFFER_SIZE),
	ATTRIBMORE(1, GL_IMPLEMENTATION_COLOR_READ_TYPE),
	ATTRIBMORE(1, GL_IMPLEMENTATION_COLOR_READ_FORMAT),
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

static const struct glesFloatInfoRec
{
	GLuint count;
	char *name;
	GLenum pname;
}
glesFloatInfo[] =
{
	ATTRIBMORE(2, GL_ALIASED_POINT_SIZE_RANGE),
	ATTRIBMORE(2, GL_ALIASED_LINE_WIDTH_RANGE),
};

__internal void PrintOGLES2APIInfo(void)
{
	GLuint i = 0;

	struct glesStringInfoRec const *glesStringInfoP, *glesStringInfoEnd;
	char *stringvalue;

	struct glesIntegerInfoRec const *glesIntegerInfoP, *glesIntegerInfoEnd;
	GLint intvalue[4];

	struct glesBooleanInfoRec const *glesBooleanInfoP, *glesBooleanInfoEnd;
	GLboolean bvalue[4];
	
	struct glesFloatInfoRec const *glesFloatInfoP, *glesFloatInfoEnd;
	GLfloat fvalue[4];

	if (!(giMode & XINFO_ES2))
		return;

	INFO("\nOpenGL ES 2 Information:\n");

	/************** print out gles 2 string info *****************************/

	INFO("\nGLES 2 String Information :\n");

	glesStringInfoP = glesStringInfo;
	glesStringInfoEnd = glesStringInfoP + (sizeof(glesStringInfo)) / sizeof(struct glesStringInfoRec);

	while (glesStringInfoP < glesStringInfoEnd)
	{
		stringvalue = (char *)glGetStringTest(glesStringInfoP->pname);

		INFO(" %s = %s \n", glesStringInfoP->name, stringvalue);

		glesStringInfoP++;
	}


	/****************** print out gles 2 boolean info **********************************/

	INFO("\nGLES 2 Boolean Information :\n");

	glesBooleanInfoP = glesBooleanInfo;
	glesBooleanInfoEnd = glesBooleanInfoP + sizeof(glesBooleanInfo) / sizeof(struct glesBooleanInfoRec);

	while (glesBooleanInfoP < glesBooleanInfoEnd)
	{
		glGetBooleanvTest(glesBooleanInfoP->pname, bvalue);

		INFO(" %s = ", glesBooleanInfoP->name);

		for (i=0; i < glesBooleanInfoP->count; i++)
		{
			INFO(" 0x%x ", bvalue[i]);
		}

		INFO("\n");

		glesBooleanInfoP++;
	}


	/****************** print out gles 2 integer info **********************************/

	INFO("\nGLES 2 Integer Information :\n");

	glesIntegerInfoP = glesIntegerInfo;
	glesIntegerInfoEnd = glesIntegerInfoP + (sizeof(glesIntegerInfo)) / sizeof(struct glesIntegerInfoRec);

	while (glesIntegerInfoP < glesIntegerInfoEnd)
	{
		gl2GetIntegervTest(glesIntegerInfoP->pname, intvalue);

		INFO(" %s = ", glesIntegerInfoP->name);

		for (i=0; i < glesIntegerInfoP->count; i++)
		{
			INFO(" 0x%x ", intvalue[i]);
		}

		INFO("\n");

		glesIntegerInfoP++;
	}


	/***************** print out gles 2 float info *******************************/

	INFO("\nGLES 2 Float Information :\n");

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

__internal int TryLoadOGLES2Library(void)
{
	if(gLibHandle)
		return 1;

	gLibHandle = exLoadLibrary(GLES2_LIB_PATH);
	if(!gLibHandle)
	{
		/* No point in continuing if there's no GL.. */
		return 0;
	}

	if (!exGetLibFuncAddr(gLibHandle, "glGetString", (GENERICPF *)&glGetStringTest))
		return 0;

	if (!exGetLibFuncAddr(gLibHandle, "glGetBooleanv", (GENERICPF *)&glGetBooleanvTest))
		return 0;

	if (!exGetLibFuncAddr(gLibHandle, "glGetIntegerv", (GENERICPF *)&gl2GetIntegervTest))
		return 0;

	if (!exGetLibFuncAddr(gLibHandle, "glGetFloatv", (GENERICPF *)&glGetFloatvTest))
		return 0;

	return 1;
}

__internal void UnloadOGLES2Library(void)
{
	if(gLibHandle)
		exUnloadLibrary(gLibHandle);
}

__internal const char *GetOGLES2VendorString(void)
{
	if(!glGetStringTest)
		return NULL;
	return (const char *)glGetStringTest(GL_VENDOR);
}
