/******************************************************************************
 * Name         : benchmark.c
 *
 * Copyright    : 2009 by Imagination Technologies Limited.
 *              : All rights reserved. No part of this software, either
 *              : material or conceptual may be copied or distributed,
 *              : transmitted, transcribed, stored in a retrieval system or
 *              : translated into any human or computer language in any form
 *              : by any means, electronic, mechanical, manual or otherwise,
 *              : or disclosed to third parties without the express written
 *              : permission of Imagination Technologies Limited, Home Park
 *              : Estate, Kings Langley, Hertfordshire, WD4 8LZ, U.K.
 *
 * Platform     : ANSI
 *
 * $Date: 2010/03/31 15:39:07 $ $Revision: 1.3 $
 * $Log: benchmark.c $
 ******************************************************************************/

/* To see the output, these apphints are needed:
     FlushBehaviour=2
     WindowSystem=libpvrPVR2D_FRONTWSEGL.so
     HALNumFrameBuffers=1
   or on Android:
     FlushBehaviour=2
     WindowSystem=libpvrANDROID_WSEGL.so
     HALNumFrameBuffers=1 */

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "img_types.h"

#if defined(SUPPORT_ANDROID_PLATFORM)
/* FIXME: Android's ui/FrameBufferNativeWindow.h doesn't compile in C mode */
extern EGLNativeWindowType android_createDisplaySurface(void);
#include "eglhelper.h"
#endif

static EGLSurface gSurface;
static EGLDisplay gDisplay;

/* single triangle */
static const GLfloat afVerticesST[] =
{
	-1.0f, -1.0f,
	 3.0f, -1.0f,
	-1.0f,  3.0f,
};

/* double triangle */
static const GLfloat afVerticesDT[] =
{
	-1.0f, -1.0f,
	 1.0f, -1.0f,
	-1.0f,  1.0f,
	 1.0f,  1.0f
};

static const GLfloat afColors[] =
{
	1.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 1.0f, 1.0f,
};

static const GLfloat afTexCoordsSTFull[] =
{
	0.0f, 0.0f,
	2.0f, 0.0f,
	0.0f, 2.0f,
};

static const GLfloat afTexCoordsDTFull[] =
{
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f
};

static const GLfloat afTexCoordsSTQuarter[] =
{
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
};

static const GLfloat afTexCoordsDTQuarter[] =
{
	0.0f, 0.0f,
	0.5f, 0.0f,
	0.0f, 0.5f,
	0.5f, 0.5f
};

static const GLfloat *afTexCoordsST;
static const GLfloat *afTexCoordsDT;

static EGLint iTexWidth, iTexHeight;
static GLubyte *pbyTexData565;
static GLubyte *pbyTexData8888;

static int uiOptions;

static EGLDisplay dpy;
static IMG_BOOL bTextureSetupDone = IMG_FALSE;

#define QUARTER_TEXTURE (1<<0)
#define LINEAR_FILTER   (1<<1)
#define FORMAT_565      (1<<2)
#define DOUBLE_TRIANGLE (1<<3)

static void PrintOptions(void)
{
	printf("Test #%d: ", uiOptions+1);
	if (uiOptions & DOUBLE_TRIANGLE)
		printf("Double triangle ");
	else
		printf("Single triangle ");

	if (uiOptions & FORMAT_565)
		printf("565 ");
	else
		printf("8888 ");

	if (uiOptions & LINEAR_FILTER)
		printf("linear filter ");
	else
		printf("point sampled ");

	if (uiOptions & QUARTER_TEXTURE)
		printf("quarter texture");
	else
		printf("full texture");
	printf("\n");
}

static void InitTexData565(void)
{
	GLubyte *pbyTex = malloc(iTexWidth*iTexHeight*2);
    EGLint i, j;

	pbyTexData565 = pbyTex;
	for (j = 0; j < iTexHeight; j++)
	{
		for (i = 0; i < iTexWidth; i++)
		{
			if ((i ^ j) & 0x8)
				pbyTex[0] = pbyTex[1] = 0x00;
			else
				pbyTex[0] = pbyTex[1] = 0xff;
			pbyTex += 2;
		}
	}
}

static void InitTexData8888(void)
{
	GLubyte *pbyTex = malloc(iTexWidth*iTexHeight*4);
    EGLint i, j;

	pbyTexData8888 = pbyTex;
	for (j = 0; j < iTexHeight; j++)
	{
		for (i = 0; i < iTexWidth; i++)
		{
			if ((i ^ j) & 0x8)
			{
				pbyTex[0] = pbyTex[1] = pbyTex[2] = 0x00;
				pbyTex[3] = 0xff;
			}
			else
				pbyTex[0] = pbyTex[1] = pbyTex[2] = pbyTex[3] = 0xff;
			pbyTex += 4;
		}
	}
}

static const char *pszVertexShader =
	"varying vec2 texcoord;\n"
	"attribute vec4 position;\n"
	"attribute vec2 inputtexcoord;\n"
	"\n"
	"void main(void)\n"
	"{\n"
	"	texcoord = inputtexcoord;\n"
	"	gl_Position = position;\n"
	"}\n";

static const char *pszFragmentShader =
	"varying highp vec2 texcoord;\n"
	"uniform sampler2D texture;\n"
	"\n"
	"void main(void)\n"
	"{\n"
	"	gl_FragColor = texture2D(texture, texcoord);\n"
	"}\n";

static void GLES2Test1(unsigned long ulFrames, struct timeval *psStartTime)
{
	GLuint hVertexShader, hFragmentShader, hProgram;
	GLint iStatus, iLength, iTexture;
	int mode, count;

	if (uiOptions & FORMAT_565)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, iTexWidth, iTexHeight,
					 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pbyTexData565);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iTexWidth, iTexHeight,
					 0, GL_RGBA, GL_UNSIGNED_BYTE, pbyTexData8888);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (uiOptions & LINEAR_FILTER)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	hVertexShader = glCreateShader(GL_VERTEX_SHADER);
	hFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	iLength = strlen(pszVertexShader);
	glShaderSource(hVertexShader, 1, &pszVertexShader, &iLength);
	glCompileShader(hVertexShader);
	glGetShaderiv(hVertexShader, GL_COMPILE_STATUS, &iStatus);
	if (iStatus != GL_TRUE)
	{
		char pszInfoLog[1024];
		glGetShaderInfoLog(hVertexShader, 1024, &iLength, pszInfoLog);
		printf("Failed to compile vertex shader:\n%s\n", pszInfoLog);
		abort();
	}

	iLength = strlen(pszFragmentShader);
	glShaderSource(hFragmentShader, 1, &pszFragmentShader, &iLength);
	glCompileShader(hFragmentShader);
	glGetShaderiv(hFragmentShader, GL_COMPILE_STATUS, &iStatus);
	if (iStatus != GL_TRUE)
	{
		char pszInfoLog[1024];
		glGetShaderInfoLog(hFragmentShader, 1024, &iLength, pszInfoLog);
		printf("Failed to compile fragment shader:\n%s\n", pszInfoLog);
		abort();
	}

	hProgram = glCreateProgram();
	glAttachShader(hProgram, hVertexShader);
	glAttachShader(hProgram, hFragmentShader);
	glBindAttribLocation(hProgram, 0, "position");
	glBindAttribLocation(hProgram, 1, "inputtexcoord");
	glLinkProgram(hProgram);

	glGetProgramiv(hProgram, GL_LINK_STATUS, &iStatus);
	if (iStatus != GL_TRUE)
	{
		char pszInfoLog[1024];
		glGetProgramInfoLog(hProgram, 1024, &iLength, pszInfoLog);
		printf("Failed to link program:\n%s\n", pszInfoLog);
		abort();
	}

	glValidateProgram(hProgram);
	glGetProgramiv(hProgram, GL_VALIDATE_STATUS, &iStatus);
	if (iStatus != GL_TRUE)
	{
		char pszInfoLog[1024];
		glGetProgramInfoLog(hProgram, 1024, &iLength, pszInfoLog);
		printf("Failed to validate program:\n%s\n", pszInfoLog);
		abort();
	}

	iTexture = glGetUniformLocation(hProgram, "texture");
	if (iTexture == -1) { printf("Couldn't find uniform 'texture'\n"); abort(); }

	glUseProgram(hProgram);

	if (uiOptions & QUARTER_TEXTURE)
	{
		afTexCoordsST = afTexCoordsSTQuarter;
		afTexCoordsDT = afTexCoordsDTQuarter;
	}
	else
	{
		afTexCoordsST = afTexCoordsSTFull;
		afTexCoordsDT = afTexCoordsDTFull;
	}

	if (uiOptions & DOUBLE_TRIANGLE)
	{
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, afVerticesDT);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, afTexCoordsDT);
	}
	else
	{
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, afVerticesST);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, afTexCoordsST);
	}
	glUniform1i(iTexture, 0);
	glClearColor(0.0, 1.0, 0.0, 1.0);

	if (uiOptions & DOUBLE_TRIANGLE)
	{
		mode = GL_TRIANGLE_STRIP;
		count = 4;
	}
	else
	{
		mode = GL_TRIANGLES;
		count = 3;
	}

	gettimeofday(psStartTime, NULL);
	while (ulFrames--)
	{
		glDrawArrays(mode, 0, count);
		glFlush();
	}
}

static void RunTest(EGLNativeWindowType window, unsigned long ulFrames)
{
	struct timeval sStartTime, sEndTime;
	EGLint major, minor;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;
	EGLBoolean bRet;
	unsigned long ms;

	EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	bRet = eglInitialize(dpy, &major, &minor);
	assert(bRet == EGL_TRUE);

#if defined(SUPPORT_ANDROID_PLATFORM)
	config = findMatchingWindowConfig(dpy,
									  EGL_OPENGL_ES2_BIT,
									  window);
#else
	{
#define MAX_CONFIGS 2
		EGLConfig aConfigs[MAX_CONFIGS];
		EGLint iNumConfigs;

		EGLint config_attribs[] = {
			EGL_RED_SIZE,       5,
			EGL_GREEN_SIZE,     6,
			EGL_BLUE_SIZE,      5,
			EGL_DEPTH_SIZE,     8,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
			EGL_NONE
		};

		eglChooseConfig(dpy, config_attribs, aConfigs, MAX_CONFIGS, &iNumConfigs);
		config = aConfigs[0];
	}
#endif

	surface = eglCreateWindowSurface(dpy, config, window, NULL);
	assert(surface != EGL_NO_SURFACE);

	if (!bTextureSetupDone)
	{
		if (!iTexWidth && !iTexHeight)
		{
			if (!(eglQuerySurface(dpy, surface, EGL_WIDTH, &iTexWidth) &&
				  eglQuerySurface(dpy, surface, EGL_HEIGHT, &iTexHeight)))
			{
				printf("eglQuerySurface failed\n");
				abort();
			}
		}
		printf("Texture size is %dx%d\n", iTexWidth, iTexHeight);
		InitTexData565();
		InitTexData8888();
		bTextureSetupDone = IMG_TRUE;
	}

	context = eglCreateContext(dpy, config, EGL_NO_CONTEXT, context_attribs);
	assert(context != EGL_NO_CONTEXT);

	bRet = eglMakeCurrent(dpy, surface, surface, context);
	assert(bRet == EGL_TRUE);

	/* Don't wait for swap */
	bRet = eglSwapInterval(dpy, 0);

	/* Can't test this due to Android bug */
	/*assert(bRet == EGL_TRUE);*/

	gDisplay = dpy;
	gSurface = surface;

	PrintOptions();

	GLES2Test1(ulFrames, &sStartTime);

	gettimeofday(&sEndTime, NULL);

	ms = (sEndTime.tv_sec - sStartTime.tv_sec) * 1000L +
		(sEndTime.tv_usec - sStartTime.tv_usec) / 1000L;
	printf("     #%d: ", uiOptions+1);
	printf("%lu frames in %ldms, %.2fms per frame, %.1f FPS\n", ulFrames,
		   ms, (double)ms/ulFrames, ulFrames/((double)ms/1000.0));

	bRet = eglDestroySurface(dpy, surface);
	assert(bRet == EGL_TRUE);

	bRet = eglDestroyContext(dpy, context);
	assert(bRet == EGL_TRUE);

	bRet = eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	assert(bRet == EGL_TRUE);
	
	bRet = eglTerminate(dpy);
	assert(bRet == EGL_TRUE);
}

static void usage(const char *argv0)
{
	printf("usage: %s framecount [texwidth texheight]\ne.g. %s 5000\n     %s 1000 2 2\n", argv0, argv0, argv0);
	exit(0);
}

int main(int argc, char *argv[])
{
	EGLNativeWindowType window;
	unsigned long ulFrames;

	if(argc < 2)
		usage(argv[0]);

	ulFrames = strtoul(argv[1], NULL, 10);

	if (argc > 2)
	{
		if (argc == 4)
		{
			iTexWidth = strtoul(argv[2], NULL, 10);
			iTexHeight = strtoul(argv[3], NULL, 10);
		}
		else
			usage(argv[0]);
	}

#if defined(SUPPORT_ANDROID_PLATFORM)
	window = android_createDisplaySurface();
	assert(window != NULL);
	window->common.incRef(&window->common);
#else
	window = 0;
#endif

	for (uiOptions = 0; (uiOptions & (1<<4)) == 0; uiOptions++)
		RunTest(window, ulFrames);

#if defined(SUPPORT_ANDROID_PLATFORM)
	window->common.decRef(&window->common);
#endif

	return 0;
}

/******************************************************************************
   End of file (benchmark.c)
 ******************************************************************************/
