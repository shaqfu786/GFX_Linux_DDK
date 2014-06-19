/*************************************************************************/ /*!
@Title          Read application specific hints in Linux.
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Strictly Confidential.
*/ /**************************************************************************/

/* PRQA S 3332 3 */ /* ignore warning about using undefined macros */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

#include <cutils/properties.h> /* Android header */

#include "pvr_debug.h"
#include "img_defs.h"
#include "services.h"

#define MAX_TOKEN_LENGTH 256

typedef struct _PVR_APPHINT_STATE_
{
	IMG_MODULE_ID eModuleID;

	const IMG_CHAR *pszAppName;

} PVR_APPHINT_STATE;

enum ParseState {
	PARSE_STATE_START,             /* Start parsing a line */
	PARSE_STATE_SKIP,              /* Skip to end of the line */
	PARSE_STATE_SECTION,           /* Parsing a [section] header */
	PARSE_STATE_HINT_NAME_START,   /* Looking for the start of the hint. */
	PARSE_STATE_HINT_NAME,         /* Reading the hint name. */
	PARSE_STATE_EQUALS,            /* Looking for the equals. */
	PARSE_STATE_HINT_VALUE_START,  /* Looking for the start of the hint value. */
	PARSE_STATE_HINT_VALUE,        /* Reading a numerical hint value. */
	PARSE_STATE_HINT_VALUE_STRING  /* Reading a string hint value. */
};

static const char *variant_keys[] = {
	"ro.board.platform", /* e.g. omap4 */
	"ro.product.processor", /* TI specific property, e.g. omap4470 */
	"ro.hardware", /* e.g. omapblazeboard */
	"ro.product.board", /* e.g. blaze_tablet */
};

static const int HAL_VARIANT_KEYS_COUNT =
    (sizeof(variant_keys)/sizeof(variant_keys[0]));

/***********************************************************************************
 Function Name      : GetApplicationName
 Inputs             : None
 Outputs            : pszApplicationName
 Returns            : Success/Failure boolean
 Description        : Gets application name
************************************************************************************/
static IMG_BOOL GetApplicationName(IMG_CHAR *pszApplicationName)
{
	FILE *f;
	int c;
	int off;

	f = fopen("/proc/self/cmdline", "r");

	if (!f) {
		PVR_DPF((PVR_DBG_ERROR, "GetApplicationName : Can't open cmdline pseudofile: %s", strerror(errno)));
		return IMG_FALSE;
	}

	off = 0;
	while ((c = getc(f)) != EOF && c != '\0' && off < (MAX_TOKEN_LENGTH - 1)) {
		if (c == '/') {
			/* We are only interested in the basename, so start scanning again. */
			off = 0;
			continue;
		}
		pszApplicationName[off] = c;
		off++;
	}
	pszApplicationName[off] = '\0';
	if (ferror(f)) {
		PVR_DPF((PVR_DBG_ERROR, "GetApplicationName : Read of cmdline pseudofile failed"));
		return IMG_FALSE;
	}
		
	fclose(f);

	return IMG_TRUE;
}


static IMG_BOOL convertValue(IMG_CHAR* pszToken, const IMG_CHAR* pszHintName, IMG_DATA_TYPE eDataType, IMG_VOID *pReturn)
{
	PVR_UNREFERENCED_PARAMETER(pszHintName);
	/* Convert the token to the requested data type */
	switch(eDataType) {
	case IMG_FLOAT_TYPE:
		*(IMG_FLOAT*)pReturn = (IMG_FLOAT) atof(pszToken);
		PVR_TRACE(("Hint: Setting %s to %f",pszHintName, *(IMG_FLOAT*)pReturn));
		return IMG_TRUE;
	case IMG_UINT_TYPE:
	case IMG_FLAG_TYPE:
		*(IMG_UINT32*)pReturn = (IMG_UINT32) atoi(pszToken);
		PVR_TRACE(("Hint: Setting %s to %u",pszHintName, *(IMG_UINT32*)pReturn));
		return IMG_TRUE;
	case IMG_INT_TYPE:
		*(IMG_INT32*)pReturn = (IMG_INT32) atoi(pszToken);
		PVR_TRACE(("Hint: Setting %s to %d",pszHintName, *(IMG_INT32*)pReturn));
		return IMG_TRUE;
	default:
		PVR_DPF((PVR_DBG_ERROR, "FindAppHintInFile : Bad eDataType"));
		return IMG_FALSE;
	}
}

/***********************************************************************************
 Function Name      : FindAppHintInFile
 Inputs             : pszFileName
 					  pszAppName
 					  pszHintName
					  eDataType
 Outputs            : pReturn
 Returns            : Success/Failure boolean
 Description        : Searches for an application hint in a file and returns the
					  value if the hint is found.
************************************************************************************/
static IMG_BOOL FindAppHintInFile(IMG_CHAR *pszFileName, const IMG_CHAR *pszAppName,
								  const IMG_CHAR *pszHintName, IMG_VOID *pReturn,
								  IMG_DATA_TYPE eDataType)
{
	FILE *regFile;
	IMG_BOOL bFound = IMG_FALSE;

	PVR_UNREFERENCED_PARAMETER(pszAppName);

	regFile = fopen(pszFileName, "r");

	if (regFile)
	{
		IMG_CHAR pszToken[MAX_TOKEN_LENGTH], pszApplicationName[MAX_TOKEN_LENGTH];
		IMG_INT iLineNumber;
		IMG_BOOL bUseThisSection, bInAppSpecificSection;
		enum ParseState s;
		int tokOff;
		int c;
		IMG_BOOL quitEarly = IMG_FALSE;

		/* Get the name of the application to build the section name */
		if (!GetApplicationName(pszApplicationName)) {
			fclose(regFile);
			return IMG_FALSE;
		}

		bUseThisSection = IMG_FALSE;
		bInAppSpecificSection = IMG_FALSE;
		s = PARSE_STATE_START;
		iLineNumber = 1;
		tokOff = 0;
		while ((c = getc(regFile)) != EOF) {
			if (s == PARSE_STATE_HINT_NAME_START ||
			    s == PARSE_STATE_EQUALS     ||
			    s == PARSE_STATE_HINT_VALUE_START) {
				if (isspace(c) && c != '\n') { /* Skip whitespace */
					continue;
				}
			}
			switch (s) {
			case PARSE_STATE_SKIP:
				if (c == '\n') {
					s = PARSE_STATE_START;
				}
				break;
					
			case PARSE_STATE_START:
				if (c == '\n') { /* Blank line */
					break;
				}
				if (c == '[') { /* At start of section header.*/
					s = PARSE_STATE_SECTION;
					bUseThisSection = IMG_FALSE;
					tokOff = 0;
					break;
				}
				if (!bUseThisSection) {
					s = PARSE_STATE_SKIP;
					break;
				}
				ungetc(c, regFile);
				s = PARSE_STATE_HINT_NAME_START;
				break;

			case PARSE_STATE_SECTION:
				if (c == '\n') { /* Premature end of section header. */
					PVR_DPF((PVR_DBG_ERROR, "FindAppHintInFile : Premature end of section definition in %s line %u", pszFileName, iLineNumber));
					break;
				}
				if (c == ']') { /* At end of section header.*/
					pszToken[tokOff] = '\0';
					bInAppSpecificSection = (strcmp(pszApplicationName, pszToken) == 0);
					bUseThisSection = bInAppSpecificSection || (strcmp("default", pszToken) == 0);
					s = PARSE_STATE_SKIP;
					break;					
				}
				if (tokOff > MAX_TOKEN_LENGTH - 1) {
					PVR_DPF((PVR_DBG_ERROR, "FindAppHintInFile : Application name too long in %s line %u", pszFileName, iLineNumber));
					s = PARSE_STATE_SKIP;
					break;
				}
				pszToken[tokOff] = c;
				tokOff++;
				break;
					
			case PARSE_STATE_HINT_NAME_START:
				if (c == '\n') { /* blank line */
					break;
				}
				if (!isalnum(c)) { /* Treat anything that starts with a non alnum character as a comment. */
					s = PARSE_STATE_SKIP;
					break;
				}
				ungetc(c, regFile);
				tokOff = 0;
				s = PARSE_STATE_HINT_NAME;
				break;
			case PARSE_STATE_HINT_NAME:
				if (c == '\n') { /* blank line */
					PVR_DPF((PVR_DBG_ERROR, "FindAppHintInFile : Hint has no value at %s line %u", pszFileName, iLineNumber));
					break;
				}
				if (isspace(c) || c == '=') {
					pszToken[tokOff] = '\0';
					if (strcmp(pszHintName, pszToken) == 0) {
						ungetc(c, regFile);
						s = PARSE_STATE_EQUALS;
					}
					else { /* Uninteresting hint. */
						s = PARSE_STATE_SKIP;
					}
					break;
				}
				if (!isalnum(c)) {
					PVR_DPF((PVR_DBG_ERROR, "FindAppHintInFile : Invalid hint name at %s line %u", pszFileName, iLineNumber));
					s = PARSE_STATE_SKIP;
					break;
				}
				if (tokOff > MAX_TOKEN_LENGTH - 1) {
					PVR_DPF((PVR_DBG_ERROR, "FindAppHintInFile : Hint name too long in %s line %u", pszFileName, iLineNumber));
					s = PARSE_STATE_SKIP;
					break;
				}
				pszToken[tokOff] = c;
				tokOff++;
				break;
				
			case PARSE_STATE_EQUALS:
				if (c == '\n') { /* Hint has no value */
					PVR_DPF((PVR_DBG_ERROR, "FindAppHintInFile : Hint has no value at %s line %u", pszFileName, iLineNumber));
					break;
				}
				if (c == '=') {
					tokOff = 0;
					if (eDataType == IMG_STRING_TYPE)
						s = PARSE_STATE_HINT_VALUE_STRING;
					else 
						s = PARSE_STATE_HINT_VALUE_START;

					break;
				}
				PVR_DPF((PVR_DBG_ERROR, "FindAppHintInFile : Couldn't find '=' in line in %s line %u", pszFileName, iLineNumber));
				s = PARSE_STATE_SKIP;
				break;

			case PARSE_STATE_HINT_VALUE_START:
				if (c == '\n') { /* Hint has no value */
					PVR_DPF((PVR_DBG_ERROR, "FindAppHintInFile : Hint has no value at %s line %u", pszFileName, iLineNumber));
					break;
				}
				ungetc(c, regFile);
				tokOff = 0;
				s = PARSE_STATE_HINT_VALUE;
				break;

			case PARSE_STATE_HINT_VALUE:
				if (isspace(c)) {
					pszToken[tokOff] = '\0';
					bFound = convertValue(pszToken, pszHintName, eDataType, pReturn);
					if (bFound && bInAppSpecificSection)
						quitEarly = IMG_TRUE;
					s = PARSE_STATE_SKIP;
					break;
				}
				if (tokOff > MAX_TOKEN_LENGTH - 1) {
					PVR_DPF((PVR_DBG_ERROR, "FindAppHintInFile : Hint value too long in %s line %u", pszFileName, iLineNumber));
					s = PARSE_STATE_SKIP;
					break;
				}
				pszToken[tokOff] = c;
				tokOff++;
				break;

			case PARSE_STATE_HINT_VALUE_STRING:
				if (c == '\n' || tokOff > APPHINT_MAX_STRING_SIZE - 1) {
					if (tokOff > APPHINT_MAX_STRING_SIZE - 1) {
						PVR_DPF((PVR_DBG_ERROR, "FindAppHintInFile : Hint value too long at %s line %u", pszFileName, iLineNumber));
					}
					((IMG_CHAR*)pReturn)[tokOff] = '\0';
					PVR_TRACE(("Hint: Setting %s to %s",pszHintName, (IMG_CHAR*)pReturn));
					bFound = IMG_TRUE;
					if (bInAppSpecificSection)
						quitEarly = IMG_TRUE;
					s = PARSE_STATE_SKIP;
					break;
				}
				((IMG_CHAR*)pReturn)[tokOff] = c;
				tokOff++;
				break;
			}

			if (c == '\n') {
				/* Start of a new line. */
				iLineNumber++;
				s = PARSE_STATE_START;
			}
			if (quitEarly)
				break;
		}
		
		if (!bFound && s != PARSE_STATE_START && s != PARSE_STATE_SKIP) {
			PVR_DPF((PVR_DBG_ERROR, "FindAppHintInFile : Reached end of file too early in %s line %u", pszFileName, iLineNumber));
			if (s == PARSE_STATE_HINT_VALUE_STRING) {
				((IMG_CHAR*)pReturn)[tokOff] = '\0';
				PVR_TRACE(("Hint: Setting %s to %s",pszHintName, (IMG_CHAR*)pReturn));
				bFound = IMG_TRUE;
			}
			else if (s == PARSE_STATE_HINT_VALUE) {
				pszToken[tokOff] = '\0';
				if (strlen(pszToken) > 0) {
					bFound = convertValue(pszToken, pszHintName, eDataType, pReturn);
				}
			}
		}

		fclose(regFile);
	}

	return bFound;
}
/***********************************************************************************
 Function Name      : FindAppHintVariant
 Inputs             : pszAppName
 					  pszHintName
 					  eDataType
 Outputs            : pFound
 Returns            : Success/Failure boolean
 Description        : Searches for hardware specific versions of powervr.ini by
					  getting the values of various Android properties.
************************************************************************************/
static IMG_BOOL FindAppHintVariant(const IMG_CHAR *pszAppName,
							const IMG_CHAR *pszHintName, IMG_VOID *pReturn,
							IMG_DATA_TYPE eDataType)
{
	int i;
	char prop[30];
	char path[256];

	IMG_BOOL bFound = IMG_FALSE;
	for (i=0 ; i<HAL_VARIANT_KEYS_COUNT+1 ; i++) {
		if (i < HAL_VARIANT_KEYS_COUNT) {
			if (property_get(variant_keys[i], prop, NULL) == 0) {
				continue;
			}
			snprintf(path, sizeof(path), "/etc/powervr.%s.ini", prop);
			if (access(path, R_OK) == 0) {
				if(FindAppHintInFile(path, pszAppName, pszHintName, pReturn, eDataType))
				{
					bFound = IMG_TRUE;
				}
			}
		}
	}
	return bFound;
}

/***********************************************************************************
 Function Name      : FindAppHint
 Inputs             : eModuleID
 					  pszAppName
 					  pszHintName
 					  eDataType
 Outputs            : pReturn
 Returns            : Success/Failure boolean
 Description        : Calls FindAppHintInFile to find an apphint
************************************************************************************/
static IMG_BOOL FindAppHint(IMG_MODULE_ID eModuleID, const IMG_CHAR *pszAppName,
							const IMG_CHAR *pszHintName, IMG_VOID *pReturn,
							IMG_DATA_TYPE eDataType)
{
	IMG_BOOL bFound = IMG_FALSE;

	switch (eModuleID)
	{
		case IMG_EGL:
		case IMG_OPENGLES1:
		case IMG_OPENGLES2:
		case IMG_OPENVG:
		case IMG_SRV_UM:
		case IMG_SRVCLIENT:
		case IMG_OPENGL:
#if defined(SUPPORT_GRAPHICS_HAL) || defined(SUPPORT_COMPOSER_HAL)
		case IMG_ANDROID_HAL:
#endif
#if defined(SUPPORT_OPENCL)
	    case IMG_OPENCL:
#endif
		{
			/* Try main hint file first */
			if(FindAppHintInFile("/etc/powervr.ini", pszAppName, pszHintName, pReturn, eDataType))
			{
				bFound = IMG_TRUE;
			}

			if (FindAppHintVariant(pszAppName, pszHintName, pReturn, eDataType))
			{
				bFound = IMG_TRUE;
			}

			/* Try local hint file, if present */
			if(FindAppHintInFile("powervr.ini", pszAppName, pszHintName, pReturn, eDataType))
			{
				bFound = IMG_TRUE;
			}

			break;
		}
		default:
		{
		    PVR_DPF((PVR_DBG_WARNING, "FindAppHint: Unsupported eModuleID"));

			break;
		}
	}

	return bFound;
}


/******************************************************************************
 Function Name      : PVRSRVCreateAppHintState
 Inputs             : eModuleID, *pszAppName
 Outputs            : *ppvState
 Returns            : none
 Description        : Create app hint state
******************************************************************************/
IMG_EXPORT IMG_VOID PVRSRVCreateAppHintState(IMG_MODULE_ID eModuleID,
										   const IMG_CHAR *pszAppName,
										   IMG_VOID **ppvState)
{
	PVR_APPHINT_STATE* psHintState;
	
	/* Note we dont use OSAllocUserModeMem() here because an
	 * implementation used for debugging allocations may want
	 * app hints, which would cause recursion.
	 */
	psHintState = malloc(sizeof(PVR_APPHINT_STATE));

	if(!psHintState)
	{
		PVR_DPF((PVR_DBG_ERROR, "OSCreateAppHintState: Failed"));	
	}
	else
	{
		psHintState->eModuleID = eModuleID;
		psHintState->pszAppName = pszAppName;
	}

	*ppvState = (IMG_VOID*)psHintState;
}


/******************************************************************************
 Function Name      : PVRSRVFreeAppHintState
 Inputs             : eModuleID, *pvState
 Outputs            : none
 Returns            : none
 Description        : Free the app hint state, if it was created
******************************************************************************/
IMG_EXPORT IMG_VOID PVRSRVFreeAppHintState(IMG_MODULE_ID eModuleID,
										 IMG_VOID *pvHintState)
{
	PVR_UNREFERENCED_PARAMETER(eModuleID);	/* PRQA S 3358 */ /* override enum warning */

	if (pvHintState)
	{
		PVR_APPHINT_STATE* psHintState = (PVR_APPHINT_STATE*)pvHintState;

		free(psHintState);
	}
}


/******************************************************************************
 Function Name      : PVRSRVGetAppHint
 Inputs             : *pvHintState, *pszHintName, eDataType, *pvDefault
 Outputs            : *pvReturn
 Returns            : Boolean - True if hint read, False if used default
 Description        : Return the value of this hint from state or use default
******************************************************************************/
IMG_EXPORT IMG_BOOL PVRSRVGetAppHint(IMG_VOID		*pvHintState,
								   const IMG_CHAR	*pszHintName,
								   IMG_DATA_TYPE	eDataType,
								   const IMG_VOID	*pvDefault,
								   IMG_VOID		*pvReturn)
{
	IMG_BOOL bFound = IMG_FALSE;
	typedef union
	{
		IMG_VOID    *pvData;
		IMG_UINT32  *pui32Data;
		IMG_CHAR    *pcData;
		IMG_INT32   *pi32Data;
	} PTRXLATE;

	typedef union
	{
		const IMG_VOID    *pvData;
		const IMG_UINT32  *pui32Data;
		const IMG_CHAR    *pcData;
		const IMG_INT32   *pi32Data;
	} CONSTPTRXLATE;

	CONSTPTRXLATE uDefault;
	PTRXLATE uReturn;

	/* Debug sanity check - should be called with known strings */
	PVR_ASSERT(pszHintName);

	if (pvHintState)
	{
		PVR_APPHINT_STATE* psHintState = (PVR_APPHINT_STATE*)pvHintState;

		bFound = FindAppHint(psHintState->eModuleID,
							 psHintState->pszAppName,
							 pszHintName,
							 pvReturn,
							 eDataType);
	}

	if (!bFound)
	{
		uDefault.pvData = pvDefault;
		uReturn.pvData = pvReturn;

		switch(eDataType)
		{
			case IMG_UINT_TYPE:
			{
				*uReturn.pui32Data = *uDefault.pui32Data;

				break;
			}
			case IMG_STRING_TYPE:
			{
				strcpy(uReturn.pcData, uDefault.pcData);

				break;
			}
			case IMG_INT_TYPE:
			default:
			{
				*uReturn.pi32Data = *uDefault.pi32Data;

				break;
			}
		}
	}

	return bFound;
}

