/*************************************************************************/ /*!
@File
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Strictly Confidential.
*/ /**************************************************************************/

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "ex_global.h"
#include "ex_gl.h"
#include "ex_gles.h"
#include "ex_gles2.h"

#if defined(SUPPORT_ANDROID_PLATFORM)
/* FIXME: Hack */
int EglMain(EGLNativeDisplayType eglDisplay, EGLNativeWindowType eglWindow);
void ResizeWindow(unsigned int width, unsigned int height);
void RequestQuit(void);
#endif

static EGLNativeWindowType eglWindow;

__internal /*extern*/ int giMode;

enum
{
	XINFO_NO_ERROR = 0,
	XINFO_ERR_CMD_LINE,
	XINFO_ERR_CANT_INIT,
	XINFO_ERR_BAD_CONFIGS,
	XINFO_ERR_BAD_MALLOC,
	XINFO_ERR_BAD_ATTRIB,
	XINFO_ERR_BAD_CONTEXT,
	XINFO_ERR_BAD_SURFACE,
	XINFO_ERR_FAILED_CURRENT,
	XINFO_ERR_HANDLE
};

static const char *gapcErrorStrings[] =
{
	"EGL_SUCCESS",
	"EGL_NOT_INITIALIZED",
	"EGL_BAD_ACCESS",
	"EGL_BAD_ALLOC",
	"EGL_BAD_ATTRIBUTE",    
	"EGL_BAD_CONFIG",
	"EGL_BAD_CONTEXT", 
	"EGL_BAD_CURRENT_SURFACE",
	"EGL_BAD_DISPLAY",
	"EGL_BAD_MATCH",
	"EGL_BAD_NATIVE_PIXMAP",
	"EGL_BAD_NATIVE_WINDOW",
	"EGL_BAD_PARAMETER",
	"EGL_BAD_SURFACE",
	"EGL_CONTEXT_LOST"
};

static void handle_egl_error(void)
{
	EGLint errorCode = eglGetError();
	ERROR("    egl error '%s' (0x%x)\n",
		  gapcErrorStrings[errorCode - EGL_SUCCESS], errorCode);
}

/* EGL Config, Surface and Context Attribute Structures */

static const struct ConfigAttribCheckRec
{
    char *name;
    EGLint value;
}
ConfigAttribCheck[] =
{
    ATTRIB(EGL_CONFIG_ID ),
    ATTRIB(EGL_BUFFER_SIZE ),         
    ATTRIB(EGL_RED_SIZE ),            
    ATTRIB(EGL_GREEN_SIZE ),          
    ATTRIB(EGL_BLUE_SIZE ),           
    ATTRIB(EGL_LUMINANCE_SIZE),       
    ATTRIB(EGL_ALPHA_SIZE ),          
    ATTRIB(EGL_ALPHA_MASK_SIZE),      
    ATTRIB(EGL_BIND_TO_TEXTURE_RGB),  
    ATTRIB(EGL_BIND_TO_TEXTURE_RGBA), 
    ATTRIB(EGL_COLOR_BUFFER_TYPE),    
    ATTRIB(EGL_CONFIG_CAVEAT ),
    ATTRIB(EGL_CONFORMANT),
    ATTRIB(EGL_DEPTH_SIZE ),          
    ATTRIB(EGL_LEVEL ),
    ATTRIB(EGL_MAX_PBUFFER_WIDTH ),
    ATTRIB(EGL_MAX_PBUFFER_HEIGHT ),
    ATTRIB(EGL_MAX_PBUFFER_PIXELS ),
    ATTRIB(EGL_MAX_SWAP_INTERVAL),
    ATTRIB(EGL_MIN_SWAP_INTERVAL),
    ATTRIB(EGL_NATIVE_RENDERABLE ),   
    ATTRIB(EGL_NATIVE_VISUAL_ID ),    
    ATTRIB(EGL_NATIVE_VISUAL_TYPE ),  
    ATTRIB(EGL_RENDERABLE_TYPE),      
    ATTRIB(EGL_SAMPLE_BUFFERS ),      
    ATTRIB(EGL_SAMPLES ),             
    ATTRIB(EGL_STENCIL_SIZE ),        
    ATTRIB(EGL_SURFACE_TYPE ),        
    ATTRIB(EGL_TRANSPARENT_TYPE ),
    ATTRIB(EGL_TRANSPARENT_RED_VALUE),
    ATTRIB(EGL_TRANSPARENT_GREEN_VALUE ),
    ATTRIB(EGL_TRANSPARENT_BLUE_VALUE ),
#if defined(EGL_EXTENSION_ANDROID_RECORDABLE)
	ATTRIB(EGL_RECORDABLE_ANDROID),
#endif
};

static const struct SurfaceAttribCheckRec
{
	char *name;
	EGLint value;
}
SurfaceAttribCheck[] =
{
	ATTRIB(EGL_CONFIG_ID),
	ATTRIB(EGL_VG_ALPHA_FORMAT),
	ATTRIB(EGL_VG_COLORSPACE),
	ATTRIB(EGL_WIDTH),
	ATTRIB(EGL_HEIGHT),
	ATTRIB(EGL_HORIZONTAL_RESOLUTION),
	ATTRIB(EGL_LARGEST_PBUFFER),
	ATTRIB(EGL_MIPMAP_TEXTURE),
	ATTRIB(EGL_MIPMAP_LEVEL),
	ATTRIB(EGL_PIXEL_ASPECT_RATIO),
	ATTRIB(EGL_RENDER_BUFFER),
	ATTRIB(EGL_SWAP_BEHAVIOR),
	ATTRIB(EGL_TEXTURE_FORMAT),
	ATTRIB(EGL_TEXTURE_TARGET),
	ATTRIB(EGL_VERTICAL_RESOLUTION),
};

static const struct ContextAttribCheckRec
{
	char *name;
	EGLint value;
}
ContextAttribCheck[] =
{
	ATTRIB(EGL_CONFIG_ID),
	ATTRIB(EGL_CONTEXT_CLIENT_TYPE),
	ATTRIB(EGL_CONTEXT_CLIENT_VERSION),
	ATTRIB(EGL_RENDER_BUFFER),
};  

/* Context creation helpers */

static EGLContext CreateOGLES1Context(EGLDisplay display, EGLConfig config)
{
	EGLint contextAttrib[] = { EGL_CONTEXT_CLIENT_VERSION, 1, EGL_NONE };
	if (giMode & XINFO_EGL)
		INFO("\nCreating OpenGL ES 1 context..\n");
	eglBindAPI(EGL_OPENGL_ES_API);
	return eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttrib);
}

static EGLContext CreateOGLES2Context(EGLDisplay display, EGLConfig config)
{
	EGLint contextAttrib[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	if (giMode & XINFO_EGL)
		INFO("\nCreating OpenGL ES 2 context..\n");
	eglBindAPI(EGL_OPENGL_ES_API);
	return eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttrib);
}

static EGLContext CreateOGLContext(EGLDisplay display, EGLConfig config)
{
	if (giMode & XINFO_EGL)
		INFO("\nCreating OpenGL context..\n");
	eglBindAPI(EGL_OPENGL_API);
	return eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);
}

/* Each API has a bit, and associated Context helper and Info helper */

static struct PerApiBitRec
{
	const EGLint apiBit;
	EGLContext (*const CreateContext)(EGLDisplay display, EGLConfig config);
	void (*const PrintAPIInfo)(void);
	int (*const TryLoadLibrary)(void);
	void (*const UnloadLibrary)(void);
	const char *(*const GetVendorString)(void);
}
PerApiBit[] =
{
 	{
		EGL_OPENGL_ES_BIT,
		CreateOGLES1Context,
		PrintOGLES1APIInfo,
		TryLoadOGLES1Library,
		UnloadOGLES1Library,
		GetOGLES1VendorString
	},
	{
		EGL_OPENGL_ES2_BIT,
		CreateOGLES2Context,
		PrintOGLES2APIInfo,
		TryLoadOGLES2Library,
		UnloadOGLES2Library,
		GetOGLES2VendorString
	},
	{
		EGL_OPENGL_BIT,
		CreateOGLContext,
		PrintOGLAPIInfo,
		TryLoadOGLLibrary,
		UnloadOGLLibrary,
		GetOGLVendorString
	}
};

/* Surface creation helpers */

static EGLSurface CreateWindowSurface(EGLDisplay display, EGLConfig config,
									  EGLint width, EGLint height)
{
	/* FIXME: Use width/height constructively */
	width = width;
	height = height;

	if (giMode & XINFO_EGL)
		INFO("\nCreating Window surface..\n");

	return eglCreateWindowSurface(display, config, eglWindow, NULL);
}

static EGLSurface CreatePbufferSurface(EGLDisplay display, EGLConfig config,
									   EGLint width, EGLint height)
{
	EGLint pBuffAttrib[] = {
		EGL_WIDTH,				0,
		EGL_HEIGHT,				0,
		EGL_LARGEST_PBUFFER,	EGL_TRUE,
		EGL_NONE
	};

	pBuffAttrib[1] = width;
	pBuffAttrib[3] = height;

	if (giMode & XINFO_EGL)
		INFO("\nCreating Pbuffer surface..\n");

	return eglCreatePbufferSurface(display, config, pBuffAttrib); 
}

#if 0
/* FIXME: Add pixmap surface support */
static EGLSurface CreatePixmapSurface(EGLDisplay display, EGLConfig config,
									  EGLint width, EGLint height)
{
	if (giMode & XINFO_EGL)
		INFO("\nCreating Pixmap surface..\n");
	return EGL_NO_SURFACE;
}
#endif

static const struct PerSurfaceTypeBitRec
{
	EGLint surfaceTypeBit;
	EGLSurface (*CreateSurface)(EGLDisplay display, EGLConfig config,
								EGLint width, EGLint height);
}
PerSurfaceTypeBit[] =
{
 	{
		EGL_WINDOW_BIT,
		CreateWindowSurface,
	},
	{
		EGL_PBUFFER_BIT,
		CreatePbufferSurface,
	},
#if 0
	{
		EGL_PIXMAP_BIT,
		CreatePbufferSurface,
	},
#endif
};

/* Code to help enumerate "interesting" configs for API info dumping
 * at the end of the program.
 */

struct UniqueApiRec
{
	struct PerApiBitRec *perApiBit;
	const char *pcVendorString;
	EGLint configNum;

	struct UniqueApiRec *psNext;
};

static struct UniqueApiRec *gpsUniqueApiList = NULL;

static void checkAddUniqueApiConfig(struct PerApiBitRec *perBitP,
                                    EGLint surfaceType, EGLint configNum)
{
	struct UniqueApiRec *psEntry;
	const char *pcVendorString;

	/* FIXME: Only support Window configs */
	if(surfaceType != EGL_WINDOW_BIT)
		return;

	/* Get the vendor string for this config */
	pcVendorString = perBitP->GetVendorString();

	/* Check for duplicates */
	for(psEntry = gpsUniqueApiList; psEntry; psEntry = psEntry->psNext)
	{
		if((psEntry->perApiBit == perBitP) &&
		   (strcmp(psEntry->pcVendorString, pcVendorString) == 0))
		{
			/* Already have this kind of config enumerated; abort */
			return;
		}
	}

	/* Not seen it, add it to the list */
	psEntry = (struct UniqueApiRec *)malloc(sizeof(struct UniqueApiRec));

	psEntry->perApiBit      = perBitP;
	psEntry->pcVendorString	= pcVendorString;
	psEntry->configNum      = configNum;
	psEntry->psNext         = gpsUniqueApiList;

	gpsUniqueApiList = psEntry;
}

/* EGL info helpers (output can be suppressed) */

static void ProcessEGLSurfaceInfo(EGLDisplay display, EGLSurface surface)
{
	const struct SurfaceAttribCheckRec *surfaceP, *surfaceEnd;
	EGLint value;

	if(giMode & XINFO_EGL)
		INFO("\nEGL surface Attributes:\n");

	surfaceP = SurfaceAttribCheck;
	surfaceEnd = surfaceP + (sizeof(SurfaceAttribCheck) / sizeof(struct SurfaceAttribCheckRec));

	while (surfaceP < surfaceEnd) 
	{
		if (eglQuerySurface(display, surface, surfaceP->value, &value))
		{
			if(giMode & XINFO_EGL)
				INFO(" %s = 0x%x\n", surfaceP->name, value);
		}
		else
		{
			ERROR(" -> Failed to query %s attribute\n", surfaceP->name);
			handle_egl_error();
		}

		surfaceP++;
	}
}

static void ProcessEGLContextInfo(EGLDisplay display, EGLContext context)
{
	const struct ContextAttribCheckRec *contextP, *contextEnd;
	EGLint value;

	if(giMode & XINFO_EGL)
		INFO("\nEGL context Attributes:\n");

	contextP = ContextAttribCheck;
	contextEnd = contextP + (sizeof(ContextAttribCheck) / sizeof(struct ContextAttribCheckRec));

	while (contextP < contextEnd) 
	{
		if (eglQueryContext(display, context, contextP->value, &value))
		{
			if(giMode & XINFO_EGL)
				INFO(" %s = 0x%x\n", contextP->name, value);
		}
		else
		{
			ERROR(" -> Failed to query %s attribute\n", contextP->name);
			handle_egl_error();
		}

		contextP++;
	}
}

static void ProcessEGLConfigInfo(EGLDisplay display, EGLConfig config,
								 EGLint configNum)
{
	const char *pcEglVendor, *pcEglVersion, *pcEglExtensions, *pcEglClientAPIs;
	const struct ConfigAttribCheckRec *configP, *configEnd;
	EGLint value;

	pcEglVendor = eglQueryString(display, EGL_VENDOR);
	pcEglVersion = eglQueryString(display, EGL_VERSION);
	pcEglExtensions = eglQueryString(display, EGL_EXTENSIONS);
	pcEglClientAPIs = eglQueryString(display, EGL_CLIENT_APIS);

	if (giMode & XINFO_EGL)
	{
		INFO("\n**********************************\n");
		INFO("EGL config number: %d\n", configNum);
		INFO("EGL vendor string: %s\n", pcEglVendor);
		INFO("EGL version string: %s\n", pcEglVersion);
		INFO("EGL extensions: %s\n", pcEglExtensions);
		INFO("EGL client APIs are: %s\n", pcEglClientAPIs);

		INFO("\nEGL config Attributes:\n");
	}

	configP = ConfigAttribCheck;
	configEnd = configP + (sizeof(ConfigAttribCheck) / sizeof(struct ConfigAttribCheckRec));

	while (configP < configEnd) 
	{
		if (!eglGetConfigAttrib(display, config, configP->value, &value))
		{
			ERROR(" -> Failed to get %s attribute\n", configP->name);
			handle_egl_error();
			goto skip_attrib;
		}

		if(!(giMode & XINFO_EGL))
			goto skip_attrib;

		INFO(" %s = 0x%x ", configP->name, value);

		switch (configP->value)
		{
			case EGL_SURFACE_TYPE:
			{
				if (value & EGL_PBUFFER_BIT)
					INFO(" EGL_PBUFFER_BIT ");

				if (value & EGL_PIXMAP_BIT)
					INFO(" EGL_PIXMAP_BIT ");

				if (value & EGL_WINDOW_BIT)
					INFO(" EGL_WINDOW_BIT ");

				if (value & EGL_VG_COLORSPACE_LINEAR_BIT)
					INFO(" EGL_VG_COLORSPACE_LINEAR_BIT ");

				if (value & EGL_VG_ALPHA_FORMAT_PRE_BIT)
					INFO(" EGL_VG_ALPHA_FORMAT_PRE_BIT");

				break;
			}

			case EGL_RENDERABLE_TYPE:
			{
				if (value & EGL_OPENGL_ES2_BIT)
					INFO(" EGL_OPENGL_ES2_BIT ");

				if (value & EGL_OPENGL_ES_BIT)
					INFO(" EGL_OPENGL_ES_BIT ");

				if (value & EGL_OPENGL_BIT)
					INFO(" EGL_OPENGL_BIT ");

				break;
			}
		}

		INFO("\n");

skip_attrib:
		configP++;
	}
}

static int CreateContextsWithSurface(EGLDisplay display, EGLConfig config,
									 EGLSurface surface, EGLint surfaceType,
									 EGLint contextTypes, EGLint configNum)
{
	struct PerApiBitRec *perBitP, *perBitEnd;
	EGLContext context;

	perBitP = PerApiBit;
	perBitEnd = perBitP + (sizeof(PerApiBit) / sizeof(struct PerApiBitRec));

	while (perBitP < perBitEnd)
	{
		/* Skip any unsupported APIs on this config */
		if (!(contextTypes & perBitP->apiBit))
			goto skip_api;

		/* Create a context with the supported API */
		context = perBitP->CreateContext(display, config);

		/* Check whether the context was created */
		if (context == EGL_NO_CONTEXT) 
		{
			ERROR("Unable to create a context\n");
			handle_egl_error();
			return XINFO_ERR_BAD_CONTEXT;
		}

		ProcessEGLContextInfo(display, context);

		/* Bind the selected context to the surface */
		if (!eglMakeCurrent(display, surface, surface, context)) 
		{
			ERROR("Unable to make current context\n");
			handle_egl_error();
			return XINFO_ERR_FAILED_CURRENT;
		}

		checkAddUniqueApiConfig(perBitP, surfaceType, configNum);

		/* Release the current surface */
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

		/* Destroy the context */
		eglDestroyContext(display, context);

skip_api:
		perBitP++;
	}

	return XINFO_NO_ERROR;
}

static int CreateConfigSurfaces(EGLDisplay display, EGLConfig config,
								EGLint contextTypes, EGLint surfaceTypes,
								EGLint configNum, EGLint width, EGLint height)
{
	const struct PerSurfaceTypeBitRec *perBitP, *perBitEnd;
	EGLSurface surface;
	int err;

	perBitP = PerSurfaceTypeBit;
	perBitEnd = perBitP + (sizeof(PerSurfaceTypeBit) / sizeof(struct PerSurfaceTypeBitRec));

	while (perBitP < perBitEnd)
	{
		/* Skip any unsupported surface types on this config */
		if (!(surfaceTypes & perBitP->surfaceTypeBit))
			goto skip_surface_type;

		/* Create a surface for the supported type */
		surface = perBitP->CreateSurface(display, config, width, height);

#if defined(ANDROID)
		/* On Android we can expect EGL_BAD_MATCH if we're doing eglinfo
		 * to the framebuffer directly. This is because configs are
		 * returned that are invalid for the framebuffer window.
		 */
		if (eglGetError() == EGL_BAD_MATCH)
		{
			ERROR("Config and Window pixel formats do not match\n");
			goto skip_surface_type;
		}
#endif

		if (surface == EGL_NO_SURFACE) 
		{
			ERROR("Unable to create surface\n");
			handle_egl_error();
			return XINFO_ERR_BAD_SURFACE;
		}

		ProcessEGLSurfaceInfo(display, surface);

		/* Now create all possible contexts on this surface */
		err = CreateContextsWithSurface(display, config, surface,
										perBitP->surfaceTypeBit,
										contextTypes, configNum);
		if (err != XINFO_NO_ERROR)
			return err;

		/* Destroy the surface */
		eglDestroySurface(display, surface);

skip_surface_type:
		perBitP++;
	}

	return XINFO_NO_ERROR;
}

static int ProcessConfig(EGLDisplay display, EGLConfig config, EGLint configNum)
{
	EGLint surfaceWidth = 0, surfaceHeight = 0;
	EGLint contextTypes, surfaceTypes;

#if defined(ANDROID)
	{
		EGLint conformant, configCaveat;

		if (!eglGetConfigAttrib(display, config, EGL_CONFORMANT, &conformant))
		{
			ERROR(" -> Failed to get EGL_CONFORMANT attribute\n");
			handle_egl_error();
			return XINFO_ERR_BAD_ATTRIB;
		}

		if (!eglGetConfigAttrib(display, config, EGL_CONFIG_CAVEAT, &configCaveat))
		{
			ERROR(" -> Failed to get EGL_CONFIG_CAVEAT attribute\n");
			handle_egl_error();
			return XINFO_ERR_BAD_ATTRIB;
		}

		if(!conformant && configCaveat == EGL_SLOW_CONFIG)
		{
			ERROR(" -> Detected software config (%d); skipping\n", configNum);
			return XINFO_NO_ERROR;
		}
	}
#endif /* defined(ANDROID) */

	/* This will process the config attributes and print out error
	 * messages if attributes are missing or fail to query.
	 */
	ProcessEGLConfigInfo(display, config, configNum);

	/* We now re-query some mandatory attributes, and fail hard if
	 * they cannot be read.
	 */

	if (!eglGetConfigAttrib(display, config, EGL_SURFACE_TYPE, &surfaceTypes))
	{
		ERROR(" -> Failed to get EGL_SURFACE_TYPE attribute\n");
		handle_egl_error();
		return XINFO_ERR_BAD_ATTRIB;
	}

	if(surfaceTypes & EGL_PBUFFER_BIT)
	{
		if (!eglGetConfigAttrib(display, config, EGL_MAX_PBUFFER_WIDTH, &surfaceWidth))
		{
			ERROR(" -> Failed to get EGL_MAX_PBUFFER_WIDTH attribute\n");
			handle_egl_error();
			return XINFO_ERR_BAD_ATTRIB;
		}

		if (!eglGetConfigAttrib(display, config, EGL_MAX_PBUFFER_HEIGHT, &surfaceHeight))
		{
			ERROR(" -> Failed to get EGL_MAX_PBUFFER_HEIGHT attribute\n");
			handle_egl_error();
			return XINFO_ERR_BAD_ATTRIB;
		}
	}

	if (!eglGetConfigAttrib(display, config, EGL_RENDERABLE_TYPE, &contextTypes))
	{
		ERROR(" -> Failed to get EGL_RENDERABLE_TYPE attribute\n");
		handle_egl_error();
		return XINFO_ERR_BAD_ATTRIB;
	}

	/* Now create all surface types for this config */
	return CreateConfigSurfaces(display, config, contextTypes,
								surfaceTypes, configNum,
								surfaceWidth, surfaceHeight);
}

static int PrintAPIInfo(EGLDisplay display, EGLConfig *pConfigs)
{
	struct UniqueApiRec *psEntry, *psNextEntry = NULL;
	EGLSurface surface;
	EGLContext context;
	EGLConfig config;

	for(psEntry = gpsUniqueApiList; psEntry; psEntry = psNextEntry)
	{
		/* Get the matching config from the config list */
		config = pConfigs[psEntry->configNum];

		/* Create a Window surface with this config */
		surface = CreateWindowSurface(display, config, 0, 0);

		if (surface == EGL_NO_SURFACE) 
		{
			ERROR("Unable to create surface\n");
			handle_egl_error();
			return XINFO_ERR_BAD_SURFACE;
		}

		/* Create API context with this config/surface */
		context = psEntry->perApiBit->CreateContext(display, config);

		/* Check whether the context was created */
		if (context == EGL_NO_CONTEXT) 
		{
			ERROR("Unable to create a context\n");
			handle_egl_error();
			return XINFO_ERR_BAD_CONTEXT;
		}

        /* Bind the selected context to the surface */
		if (!eglMakeCurrent(display, surface, surface, context)) 
		{
			ERROR("Unable to make current context\n");
			handle_egl_error();
	        return XINFO_ERR_FAILED_CURRENT;
		}

		/* May not actually print anything if giMode disallows */
		psEntry->perApiBit->PrintAPIInfo();

		/* Release the current surface */
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

		/* Destroy the context */
		eglDestroyContext(display, context);

		/* Destroy the surface */
		eglDestroySurface(display, surface);

		/* Free the entry in the unique list */
		psNextEntry = psEntry->psNext;
		free(psEntry);
	}
	gpsUniqueApiList = NULL;

	return XINFO_NO_ERROR;
}

static void LoadAPIsMaskModes(void)
{
	struct PerApiBitRec *perBitP, *perBitEnd;

	perBitP = PerApiBit;
	perBitEnd = perBitP + (sizeof(PerApiBit) / sizeof(struct PerApiBitRec));

	while (perBitP < perBitEnd)
	{
		if(!perBitP->TryLoadLibrary())
		{
			/* Disable this API's messages */
			giMode &= ~perBitP->apiBit;
		}

		perBitP++;
	}
}

static void UnloadAPIs(void)
{
	struct PerApiBitRec *perBitP, *perBitEnd;

	perBitP = PerApiBit;
	perBitEnd = perBitP + (sizeof(PerApiBit) / sizeof(struct PerApiBitRec));

	while (perBitP < perBitEnd)
	{
		perBitP->UnloadLibrary();
		perBitP++;
	}
}

#if !defined(SUPPORT_ANDROID_PLATFORM)
static
#endif
int EglMain(EGLNativeDisplayType eglDisplay, EGLNativeWindowType _eglWindow)
{
	EGLint majorVersion, minorVersion, numConfigs, i;
	int iError = XINFO_NO_ERROR;
	EGLConfig *pConfigs;
	EGLDisplay display;

#if defined(SUPPORT_ANDROID_PLATFORM)
	/* Enable all messages, because on Android the command-line arguments
	   aren't looked at, and there won't be any anyway */
	giMode = XINFO_ALL;
#endif

	LoadAPIsMaskModes();

	eglWindow = _eglWindow;
	display = eglGetDisplay(eglDisplay);

	if (display == EGL_NO_DISPLAY)
	{
		ERROR("Unable to get egl display\n");
		handle_egl_error();
		iError = XINFO_ERR_CANT_INIT;
		goto err_out;
	}

	if (!eglInitialize(display, &majorVersion, &minorVersion))
	{
		ERROR("Unable to initialise egl\n");
		handle_egl_error();
		iError = XINFO_ERR_CANT_INIT;
		goto err_terminate;
	}

	if (giMode & XINFO_GETCONFIGS)
	{
		INFO("\neglGetConfigs():\n");
		INFO("======================================================================\n");
		if (!eglGetConfigs(display, NULL, 0, &numConfigs))
		{
			ERROR("Failed to get config count\n");
			handle_egl_error();
			iError = XINFO_ERR_BAD_CONFIGS;
			goto err_terminate;
		}

		pConfigs = (EGLConfig *)malloc(sizeof(EGLConfig) * numConfigs);

		if (!pConfigs)
		{
			iError = XINFO_ERR_BAD_MALLOC;
			goto err_terminate;
		}

		if (!eglGetConfigs(display, pConfigs, numConfigs, &numConfigs))
		{
			ERROR("Failed to get configs\n");
			handle_egl_error();
			iError = XINFO_ERR_BAD_CONFIGS;
			goto err_free_configs;
		}

		for (i = 0; i < numConfigs; i++)
		{
			iError = ProcessConfig(display, pConfigs[i], i);
			if (iError != XINFO_NO_ERROR)
				goto err_free_configs;
		}

		/* If we have any non-EGL info bit set, print API info */
		if (giMode & (XINFO_ALL & ~XINFO_EGL))
		{
			INFO("\nPrinting API info..\n");
			iError = PrintAPIInfo(display, pConfigs);
		}
		free(pConfigs);
		pConfigs = NULL;
	}

	if (giMode & XINFO_CHOOSECONFIG)
	{
		EGLint attributes[10] = { EGL_RENDERABLE_TYPE, 0, EGL_NONE };
		
		if (giMode & XINFO_ES1) attributes[1] |= EGL_OPENGL_ES_BIT;
		if (giMode & XINFO_ES2) attributes[1] |= EGL_OPENGL_ES2_BIT;
		if (giMode & XINFO_GL) 	attributes[1] |= EGL_OPENGL_BIT;
		
		/* If no API is selected, process all APIs */
		if (attributes[1] == 0)
		{
			giMode |= XINFO_ES1 | XINFO_VG | XINFO_ES2 | XINFO_GL;
		}
		
		INFO("\neglChooseConfig():");
		INFO("\n======================================================================\n");
		if (!eglChooseConfig(display, attributes, NULL, 0, &numConfigs))
		{
			ERROR("Failed to get config count\n");
			handle_egl_error();
			iError = XINFO_ERR_BAD_CONFIGS;
			goto err_terminate;
		}
		
		pConfigs = (EGLConfig *)malloc(sizeof(EGLConfig) * numConfigs);

		if (!pConfigs)
		{
			iError = XINFO_ERR_BAD_MALLOC;
			goto err_terminate;
		}
		
		if (!eglChooseConfig(display, attributes, pConfigs, numConfigs, &numConfigs))
		{
			ERROR("Failed to get configs\n");
			handle_egl_error();
			iError = XINFO_ERR_BAD_CONFIGS;
			goto err_free_configs;
		}

		for (i = 0; i < numConfigs; i++)
		{
			iError = ProcessConfig(display, pConfigs[i], i);
			if (iError != XINFO_NO_ERROR)
				goto err_free_configs;
		}

		/* If we have any non-EGL info bit set, print API info */
		if (giMode & (XINFO_ALL & ~XINFO_EGL))
		{
			INFO("\nPrinting API info..\n");
			iError = PrintAPIInfo(display, pConfigs);
		}
	}
	
err_free_configs:
	free(pConfigs);
err_terminate:
	eglTerminate(display);
err_out:
	UnloadAPIs();
	return iError;
}

#if !defined(SUPPORT_ANDROID_PLATFORM)

static void Usage(void)
{
	INFO("Usage: eglinfo [-h] [-a] [-e] [-1] [-2] [-G] [-C]\n");
	INFO("\t-h: Help.\n");
	INFO("\t-a: Print all the possible EGL, OpenGL, OpenGL ES 1, OpenGL ES 2.\n");
	INFO("\t-e: Print EGL info.\n");
	INFO("\t-g: Print only OpenGL info.\n");
	INFO("\t-1: Print only OpenGL ES 1 info.\n");
	INFO("\t-2: Print only OpenGL ES 2 info.\n");
	INFO("\t-G: Use eglGetConfigs\n");
	INFO("\t-C: Use eglChooseConfig\n");
}

static int ProcessOptions(int argc, char *argv[])
{
    int iRetVal = XINFO_NO_ERROR;
    int i;


    if (argc == 1)
    {
    	giMode |= (XINFO_ALL & ~XINFO_CHOOSECONFIG);
		return iRetVal;
    }

	for (i = 1; i < argc; i++) 
	{
		if (strcmp(argv[i], "-a") == 0) 
		{
			giMode |= XINFO_ALL;
		}
		else if (strcmp(argv[i], "-e") == 0) 
		{
			giMode |= XINFO_EGL;
		}
		else if (strcmp(argv[i], "-v") == 0) 
		{
			giMode |= XINFO_VG;
		}
		else if (strcmp(argv[i], "-g") == 0)
		{
			giMode |= XINFO_GL;
		}
		else if (strcmp(argv[i], "-1") == 0)
		{
			giMode |= XINFO_ES1;
		}
		else if (strcmp(argv[i], "-2") == 0)
		{
			giMode |= XINFO_ES2;
		}
		else if (strcmp(argv[i], "-G") == 0)
		{
			giMode |= XINFO_GETCONFIGS;
		}
		else if (strcmp(argv[i], "-C") == 0)
		{
			giMode |= XINFO_CHOOSECONFIG;
		}
		else if (strcmp(argv[i], "-h") == 0) 
		{
			iRetVal = XINFO_ERR_CMD_LINE;
			Usage();
			break;
		}
		else
		{
			ERROR("Unknown option `%s'\n", argv[i]);
			iRetVal = XINFO_ERR_CMD_LINE;
			Usage();
			break;
		}
	}

    return iRetVal;
}


#if defined(SUPPORT_XORG)

#include <X11/Xlib.h>

int main(int argc, char *argv[])
{
	int iError, iBlackColour;
	Display *dpy;
	Window win;

	dpy = XOpenDisplay(":0");
	if(!dpy)
	{
		ERROR("Error: couldn't open display\n");
		return EXIT_FAILURE;
	}

	iBlackColour = BlackPixel(dpy, DefaultScreen(dpy));

	win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1,
							  0, iBlackColour, iBlackColour);
	XMapWindow(dpy, win);
	XFlush(dpy);

	iError = ProcessOptions(argc, argv);
	if (iError != XINFO_NO_ERROR)
		return iError;

	return EglMain((EGLNativeDisplayType)dpy, (EGLNativeWindowType)win);
}

#else /* defined(SUPPORT_XORG) */

/* NULLWS */

int main(int argc, char** argv)
{
	int iError;

	iError = ProcessOptions(argc, argv);
	if (iError != XINFO_NO_ERROR)
		return iError;

	return EglMain(EGL_DEFAULT_DISPLAY, 0);
}

#endif /* defined(SUPPORT_XORG) */


#else /* !defined(SUPPORT_ANDROID_PLATFORM) */

/* Stubbed */

void ResizeWindow(unsigned int width, unsigned int height)
{
	width = width;
	height = height;
}

void RequestQuit(void)
{
}

#endif /* !defined(SUPPORT_ANDROID_PLATFORM) */
