/******************************************************************************
 * Name         : scopeman.c
 * Title        : Scope Management
 * Author       : Rana-Iftikhar Ahmad
 * Created      : Nov 2007
 *
 * Copyright    : 2002-2007 by Imagination Technologies Limited.
 *              : All rights reserved. No part of this software, either
 *              : material or conceptual may be copied or distributed,
 *              : transmitted, transcribed, stored in a retrieval system or
 *              : translated into any human or computer language in any form
 *              : by any means, electronic, mechanical, manual or otherwise,
 *              : or disclosed to third parties without the express written
 *              : permission of Imagination Technologies Limited, Home Park
 *              : Estate, Kings Langley, Hertfordshire, WD4 8LZ, U.K.
 *
 * Modifications:-
 * $Log: scopeman.c $
 *****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "scopeman.h"
#include "useasm.h"
#include "sgxdefs.h"
#include "osglue.h"

extern IMG_UINT32		g_uSourceLine;
extern IMG_PCHAR		g_pszInFileName;

typedef struct _TEMP_REG
{
	IMG_PCHAR			pszName;
	IMG_UINT32			uHwRegNum;
	IMG_UINT32			uArrayElementsCount;
	IMG_PCHAR			pszSourceFile;
	IMG_UINT32			uSourceLine;
	IMG_UINT32			uLinkToModableNamdRegsTable;
	struct _TEMP_REG*	psNext;
} TEMP_REG, *PTEMP_REG;

typedef struct _CALL_RECORD
{	
	struct _SCOPE*			psDestScope;
	IMG_UINT32				uNamedRegsCount;
	struct _CALL_RECORD*	psNext;
} CALL_RECORD, *PCALL_RECORD;

typedef struct _SCOPES_LIST
{
	struct _SCOPE*			psScope;
	IMG_UINT32				uNamedRegsCount;
	struct _SCOPES_LIST*	psNext;
}SCOPES_LIST, *PSCOPES_LIST;

typedef struct _SCOPE
{
	IMG_PCHAR		pszScopeName;
	PTEMP_REG		psTempRegs;	/* Table of named registers in this scope */
	struct _SCOPE*	psParentScope;
	struct _SCOPE*	psChildScopes;
	struct _SCOPE*	psPreviousSiblingScope;
	struct _SCOPE*	psNextSiblingScope;
	struct _SCOPE*	psUndefinedBranchDests;
	struct _SCOPE*	psUndefinedProcDests;
	struct _SCOPE*	psUndefinedExportDests;
	struct _SCOPE*	psUndefinedMovDests;
	IMG_UINT32		uLinkToLabelsTable;
	IMG_UINT32		uLinkToModifiableLabelsTable;
	IMG_UINT32		uSourceLine;
	IMG_PCHAR		pszSourceFile;
	PTEMP_REG		psUndefinedTempRegs;	/* Table of undefined temporary 
	registers in this scope */	
	IMG_BOOL		bImported;
	IMG_BOOL		bExport;
	IMG_BOOL		bProc;
	PCALL_RECORD	psCalleesRecord;
	PCALL_RECORD	psCallsRecord;	
	IMG_UINT32		uMaxNamedRegsTransByCall;
	struct _SCOPE*	psNamedRegsAllocParent;
	PSCOPES_LIST	psNamedRegsAllocChildren;
	IMG_UINT32		uCurrentCountOfNamedRegsInScope; /* Current Count of named 
	register defined in scope */
	IMG_BOOL		bDummyScope;
} SCOPE, *PSCOPE;

/* To store scope management enable/disable status. */
static IMG_BOOL g_bScopeManagementEnable = IMG_FALSE;

IMG_INTERNAL IMG_VOID EnableScopeManagement(IMG_VOID)
/*****************************************************************************
 FUNCTION	: EnableScopeManagement
    
 PURPOSE	: Enables scope management feature.

 PARAMETERS	: None.
			  
 RETURNS	: Nothing.
*****************************************************************************/
{
	g_bScopeManagementEnable = IMG_TRUE;
}

IMG_INTERNAL IMG_BOOL IsScopeManagementEnabled(IMG_VOID)
/*****************************************************************************
 FUNCTION	: IsScopeManagementEnabled
    
 PURPOSE	: Tells whether scope management is enabled or not.

 PARAMETERS	: None.
			  
 RETURNS	: IMG_TRUE	- If scope management is enabled.
			  IMG_FALSE	- If scope management is disabled.
*****************************************************************************/
{
	return g_bScopeManagementEnable;
}

/* utility routines */
static IMG_VOID TestMemoryAllocation(const IMG_VOID* pvMemory)
/*****************************************************************************
 FUNCTION	: IsScopeManagementEnabled
    
 PURPOSE	: Tests whether requested memory is allocated or not.

 PARAMETERS	: pvMemory	- Pointer to allocated memory.
			  
 RETURNS	: Nothing.
*****************************************************************************/
{
	if(pvMemory == NULL)
	{
		fprintf(stderr, "Fatal error: Not enough memory available\n");
		exit(1);
	}
}

IMG_INTERNAL IMG_BOOL IsAllDigitsString(const IMG_CHAR* pszString)
/*****************************************************************************
 FUNCTION	: IsAllDigitsString
    
 PURPOSE	: Tests whether character string is composed of digits only.

 PARAMETERS	: pszString	- Pointer to '\0' terminated character string.
			  
 RETURNS	: IMG_TRUE	- If string is composed of digits only.
			  IMG_FALSE	- If string is not composed of digits only.
*****************************************************************************/
{
	const IMG_CHAR* pszStringIntern =  pszString;
	if(pszString == NULL)
	{
		return IMG_FALSE;
	}
	while(*pszStringIntern != '\0')
	{
		if(!isdigit(*pszStringIntern))
		{
			return IMG_FALSE;		
		}
		pszStringIntern++;
	}
	if(pszStringIntern == pszString)
	{
		return IMG_FALSE;
	}
	else
	{
		return IMG_TRUE;
	}	
}

static IMG_PCHAR StrdupOrNull(const IMG_CHAR* pszString)
/*****************************************************************************
 FUNCTION	: StrdupOrNull
    
 PURPOSE	: Duplicates a string including NULL pointers.

 PARAMETERS	: pszString	- Pointer to '\0' terminated character string or NULL.
			  
 RETURNS	: Pointer to duplicated string.
*****************************************************************************/
{
	return pszString != NULL ? strdup(pszString) : NULL;
}


/* Scope management support routines interfaces */
static IMG_VOID DeleteScope(PSCOPE psScope);

static PSCOPE UseExistingUndefinedScope(const IMG_CHAR* pszScopeName, 
										IMG_BOOL bProc, 
										IMG_BOOL bImported, 
										const IMG_CHAR* pszSourceFile, 
										IMG_UINT32 uSourceLine);

/* Named register management support routines interfaces */
static IMG_VOID DeleteTempReg(PTEMP_REG psTempReg);

/* Modifiable labels table interface */
static IMG_VOID InitModifiableLabelsTable(IMG_VOID);

static IMG_UINT32 GetNextFreeModifiableLabelsTableEntry(IMG_VOID);

static IMG_UINT32 GetModifiableLabelsTableEntryValue(
	IMG_UINT32 uLinkToModifiableLabelsTable);

static IMG_VOID SetModifiableLabelsTableEntryValue(
	IMG_UINT32 uLinkToModifiableLabelsTable, IMG_UINT32 uLinkToLabelsTable);

/* Modifiable named registers table interface */
static IMG_VOID InitModifiableNamedRegsTable(IMG_VOID);

static IMG_UINT32 GetNextFreeModifiableNamedRegsTableEntry(IMG_VOID);

static IMG_PVOID GetModifiableNamedRegsTableEntryValue(
	IMG_UINT32 uLinkToModifiableNamedRegsTable);

static IMG_VOID SetModifiableNamedRegsTableEntryValue(
	IMG_UINT32 uLinkToModifiableNamedRegsTable, IMG_PVOID pvNamedTempRegHandle);

static IMG_PVOID GetErrorNamedRegisterEntryValue(IMG_VOID);


static IMG_UINT32 GetHwRegForNamedReg(IMG_UINT32 uNamedRegLink);

static IMG_UINT32 GetHwRegForNamedRegArrayElement(
	IMG_UINT32 uNamedRegLink, 
	IMG_UINT32 uArrayElementIndex);

/* Table for temporary registers in assembler program */
static PSCOPE g_psCurrentScope;

static IMG_BOOL LookupChildSiblingScopes(const SCOPE* psScope, 
										 const IMG_CHAR* pszName, 
										 IMG_HVOID ppvHandle);

static IMG_VOID DeleteTempReg(PTEMP_REG psTempReg)
/*****************************************************************************
 FUNCTION	: DeleteTempReg
    
 PURPOSE	: Frees memory allocated to temporary register definition.

 PARAMETERS	: psTempReg	- Pointer to temporary register's memory.
			  
 RETURNS	: Nothing.
*****************************************************************************/
{
	UseAsm_Free(psTempReg->pszName);
	UseAsm_Free(psTempReg->pszSourceFile);
	UseAsm_Free(psTempReg);
}

static IMG_VOID GiveErrorsAndDefineUndefinedNamedTempRegs(TEMP_REG* psTempReg)
/*****************************************************************************
 FUNCTION	: GiveErrorsAndDefineUndefinedNamedTempRegs

 PURPOSE	: Defines all undefined named temporary registers with the 
			  specified name  and gives error if reference does not match the 
			  defintion.

 PARAMETERS	: psTempReg	- Pointer to temporary register containing definition.

 RETURNS	: Nothing.
*****************************************************************************/
{
	PTEMP_REG psUndefinedTempRegsItrator = 
		g_psCurrentScope->psUndefinedTempRegs;
	PTEMP_REG psPrevUndefinedTempReg = NULL;
	PTEMP_REG psTempRegToDelete = NULL;
	while(psUndefinedTempRegsItrator != NULL)		
	{		
		if(strcmp(psTempReg->pszName, psUndefinedTempRegsItrator->pszName) == 0)
		{
			if(psTempReg->uArrayElementsCount == 0)
			{
				if(psUndefinedTempRegsItrator->uArrayElementsCount > 0)
				{
					ParseErrorAt(psUndefinedTempRegsItrator->pszSourceFile, 
						psUndefinedTempRegsItrator->uSourceLine, "Can not reference '%s' as an array", 
						psUndefinedTempRegsItrator->pszName);
				}
			}
			else
			{
				if(psUndefinedTempRegsItrator->uArrayElementsCount == 0)
				{
					ParseErrorAt(psUndefinedTempRegsItrator->pszSourceFile, 
						psUndefinedTempRegsItrator->uSourceLine, "Can not reference '%s' as a non array", 
						psUndefinedTempRegsItrator->pszName);
				}
				if(psUndefinedTempRegsItrator->uArrayElementsCount > 
					psTempReg->uArrayElementsCount)
				{
					ParseErrorAt(psUndefinedTempRegsItrator->pszSourceFile, 
						psUndefinedTempRegsItrator->uSourceLine, "Can not index element %u on an array '%s' of %u elements", 
						(psUndefinedTempRegsItrator->uArrayElementsCount - 1), 
						psTempReg->pszName,
						psTempReg->uArrayElementsCount);
				}
			}
			SetModifiableNamedRegsTableEntryValue(
				psUndefinedTempRegsItrator->uLinkToModableNamdRegsTable, 
				psTempReg);
			if(psPrevUndefinedTempReg == NULL)
			{
				g_psCurrentScope->psUndefinedTempRegs = 
					psUndefinedTempRegsItrator->psNext;				
				psTempRegToDelete = psUndefinedTempRegsItrator;
				psUndefinedTempRegsItrator = psUndefinedTempRegsItrator->psNext;
				psTempRegToDelete->psNext = NULL;				
			}
			else
			{
				psPrevUndefinedTempReg->psNext = 
					psUndefinedTempRegsItrator->psNext;
				psTempRegToDelete = psUndefinedTempRegsItrator;
				psUndefinedTempRegsItrator = psUndefinedTempRegsItrator->psNext;
				psTempRegToDelete->psNext = NULL;				
			}
			DeleteTempReg(psTempRegToDelete);
		}
		else
		{
			psPrevUndefinedTempReg = psUndefinedTempRegsItrator;
			psUndefinedTempRegsItrator = psUndefinedTempRegsItrator->psNext;
		}
	}	
	return;
}

static IMG_BOOL g_bNamedRegsUsed = IMG_FALSE;

IMG_INTERNAL IMG_VOID AddNamedRegister(const IMG_CHAR* pszName, 
						  const IMG_CHAR* pszSourceFile, 
						  IMG_UINT32 uSourceLine)
/*****************************************************************************
 FUNCTION	: AddNamedRegister
    
 PURPOSE	: Adds a named register definition.

 PARAMETERS	: pszName	- Name of the register to add.
			  pszSourceFile	- Name of the source file.
			  uSourceLine	- Source code line number.

 RETURNS	: Nothing.
*****************************************************************************/
{	
	PTEMP_REG	psTempRegItrator = g_psCurrentScope->psTempRegs;
	PTEMP_REG	psLastTempReg = psTempRegItrator;
	g_bNamedRegsUsed = IMG_TRUE;
	for(;
		psTempRegItrator != NULL;
		psTempRegItrator = psTempRegItrator->psNext)
	{
		if (strcmp(psTempRegItrator->pszName, pszName) == 0)
		{
			ParseError("Temporary Register Name '%s' is already defined at this point", pszName);
			ParseErrorAt(psTempRegItrator->pszSourceFile, 
				psTempRegItrator->uSourceLine, 
				"Is the location of previous definition");			
			return;
		}
		psLastTempReg = psTempRegItrator;
	}
	{
		PTEMP_REG psNewTempReg;
		psNewTempReg = (PTEMP_REG)UseAsm_Malloc( sizeof( TEMP_REG ) );
		TestMemoryAllocation(psNewTempReg);
		psNewTempReg->psNext = NULL;
		psNewTempReg->pszName = StrdupOrNull(pszName);
		psNewTempReg->pszSourceFile = StrdupOrNull(pszSourceFile);
		psNewTempReg->uSourceLine = uSourceLine;
		psNewTempReg->uArrayElementsCount = 0;
		psNewTempReg->uHwRegNum = 0;		
		psNewTempReg->uLinkToModableNamdRegsTable = 
			GetNextFreeModifiableNamedRegsTableEntry();
		SetModifiableNamedRegsTableEntryValue(
			psNewTempReg->uLinkToModableNamdRegsTable, psNewTempReg);
		GiveErrorsAndDefineUndefinedNamedTempRegs(psNewTempReg);
		if(psLastTempReg == NULL)
		{
			g_psCurrentScope->psTempRegs = psNewTempReg;
		}
		else
		{
			psLastTempReg->psNext = psNewTempReg;
		}
		(g_psCurrentScope->uCurrentCountOfNamedRegsInScope)++;
	}	
	return;
}

IMG_INTERNAL IMG_VOID AddNamedRegistersArray(const IMG_CHAR* pszName, 
											 IMG_UINT32 uArrayElementsCount, 
											 const IMG_CHAR* pszSourceFile, 
											 IMG_UINT32 uSourceLine)
/*****************************************************************************
 FUNCTION	: AddNamedRegistersArray
    
 PURPOSE	: Adds a named registers array definition.

 PARAMETERS	: pszName	- Name of the named registers array.
			  uArrayElementsCount	- Array elements count.
			  pszSourceFile	- Name of the source file.
			  uSourceLine	- Source code line number.
			  
 RETURNS	: Nothing.
*****************************************************************************/
{	
	PTEMP_REG	psTempRegItrator = g_psCurrentScope->psTempRegs;
	PTEMP_REG	psLastTempReg = psTempRegItrator;
	g_bNamedRegsUsed = IMG_TRUE;
	for(;
		psTempRegItrator != NULL;
		psTempRegItrator = psTempRegItrator->psNext)
	{
		if (strcmp(psTempRegItrator->pszName, pszName) == 0)
		{
			ParseError("Temporary Register Name '%s' is already defined at this point", 
				pszName);			
			return;
		}
		psLastTempReg = psTempRegItrator;
	}
	{
		PTEMP_REG	psNewTempReg;
		psNewTempReg = (PTEMP_REG)UseAsm_Malloc( sizeof( TEMP_REG ) );
		TestMemoryAllocation(psNewTempReg);		
		psNewTempReg->psNext = NULL;
		psNewTempReg->pszName = StrdupOrNull(pszName);
		psNewTempReg->pszSourceFile = StrdupOrNull(pszSourceFile);
		psNewTempReg->uSourceLine = uSourceLine;
		psNewTempReg->uArrayElementsCount = uArrayElementsCount;
		psNewTempReg->uHwRegNum = 0;
		psNewTempReg->uLinkToModableNamdRegsTable = 
			GetNextFreeModifiableNamedRegsTableEntry();
		SetModifiableNamedRegsTableEntryValue(
			psNewTempReg->uLinkToModableNamdRegsTable, psNewTempReg);
		GiveErrorsAndDefineUndefinedNamedTempRegs(psNewTempReg);
		if(psLastTempReg == NULL)
		{
			g_psCurrentScope->psTempRegs = psNewTempReg;
		}
		else
		{
			psLastTempReg->psNext = psNewTempReg;
		}
		(g_psCurrentScope->uCurrentCountOfNamedRegsInScope)+=
			uArrayElementsCount;
	}	
	return;
}

IMG_INTERNAL IMG_BOOL LookupNamedRegister(const IMG_CHAR* pszName, 
										  IMG_PUINT32 puNamedRegLink)
/*****************************************************************************
 FUNCTION	: LookupNamedRegister
    
 PURPOSE	: Looks up the definition of named register.

 PARAMETERS	: pszName	- Name of register.
			  puNamedRegLink	- Pointer to link to named register definition.

 RETURNS	: IMG_TRUE	- If the register is already defined.
			  puNamedRegLink	- Will contain link to register definition.
			  IMG_FALSE	- If the register defintion is not found.
*****************************************************************************/
{	
	PSCOPE psParentScopesIterator = NULL;	
	for(psParentScopesIterator = g_psCurrentScope;
		psParentScopesIterator != NULL;
		psParentScopesIterator = psParentScopesIterator->psParentScope)
	{
		if(LookupTemporaryRegisterInScope(
			(const IMG_VOID*)(psParentScopesIterator), pszName, puNamedRegLink))
		{
			return IMG_TRUE;
		}
	}
	*puNamedRegLink = 0;
	return IMG_FALSE;
}

static PSCOPE LookupNamedRegisterAndItsScope(const IMG_CHAR* pszName, 
											 TEMP_REG** ppsNamedReg)
/*****************************************************************************
 FUNCTION	: LookupNamedRegisterAndItsScope
    
 PURPOSE	: Searches the definition of named register and also gives the 
			  scope in which it is defined.

 PARAMETERS	: pszName	- Name of register to search.
			  ppsNamedReg	- Pointer to memory to store pointer to named 
							  register defintion.

 RETURNS	: Pointer to scope in which named register is found.
				ppsNamedReg	- Will contain pointer to named register 
							  definition.
			  NULL - If named register defintion is not found.
*****************************************************************************/
{	
	PSCOPE psParentScopesIterator = NULL;	
	for(psParentScopesIterator = g_psCurrentScope;
		psParentScopesIterator != NULL;
		psParentScopesIterator = psParentScopesIterator->psParentScope)
	{
		IMG_UINT32 uNamedRegLink;
		if(LookupTemporaryRegisterInScope(
			(const IMG_VOID*)(psParentScopesIterator), pszName, &uNamedRegLink) 
			== IMG_TRUE)
		{
			*ppsNamedReg = (PTEMP_REG) GetModifiableNamedRegsTableEntryValue(
				uNamedRegLink);
			return psParentScopesIterator;
		}
	}
	*ppsNamedReg = NULL;
	return NULL;
}

IMG_INTERNAL IMG_VOID RenameNamedRegister(const IMG_CHAR* pszCurrentName, 
										  const IMG_CHAR* pszCurrSourceFile, 
										  IMG_UINT32 uCurrSourceLine, 
										  const IMG_CHAR* pszNewName, 
										  const IMG_CHAR* pszNewSourceFile, 
										  IMG_UINT32 uNewSourceLine)
/*****************************************************************************
 FUNCTION	: RenameNamedRegister
    
 PURPOSE	: Renames already defined named register.

 PARAMETERS	: pszCurrentName	- Name of named register to modify.
			  pszCurrSourceFile	- Source file in which renaming is needed.
			  uCurrSourceLine	- Source line at which renaming is needed.
			  pszNewName	- New name to give to named register.
			  pszNewSourceFile	- Source file in which new name is given.
			  uNewSourceLine	- Source line at which new name is given.
			  
 RETURNS	: Nothing.
*****************************************************************************/
{	
	PTEMP_REG psNamedReg;
	PSCOPE psScope = NULL;
	if((psScope = LookupNamedRegisterAndItsScope(pszCurrentName, &psNamedReg)) 
		!= NULL)
	{
		IMG_UINT32 uNamedRegLink;
		if(LookupTemporaryRegisterInScope(
			(const IMG_VOID*)(psScope), pszNewName, &uNamedRegLink) == 
			IMG_FALSE)
		{
			if(psScope->bProc == IMG_FALSE)
			{
				UseAsm_Free(psNamedReg->pszName);
				psNamedReg->pszName = StrdupOrNull(pszNewName);
				UseAsm_Free(psNamedReg->pszSourceFile);
				psNamedReg->pszSourceFile = StrdupOrNull(pszNewSourceFile);
				psNamedReg->uSourceLine = uNewSourceLine;
			}
			else
			{
				ParseErrorAt(pszCurrSourceFile, uCurrSourceLine, "Can not rename parameter '%s' of a scope", 
					pszCurrentName);
				ParseErrorAt(psNamedReg->pszSourceFile, psNamedReg->uSourceLine, "Is the location where '%s' is defined as parameter", 
					psNamedReg->pszName);
			}
		}
		else
		{
			ParseErrorAt(pszNewSourceFile, uNewSourceLine, "Can not use '%s' to rename named register '%s'", 
				pszNewName, pszCurrentName);
			psNamedReg = (PTEMP_REG) GetModifiableNamedRegsTableEntryValue(
				uNamedRegLink);
			ParseErrorAt(psNamedReg->pszSourceFile, psNamedReg->uSourceLine, "Is the location where '%s' is already used as a name of named register", 
				psNamedReg->pszName);
		}
	}
	else
	{
		ParseErrorAt(pszCurrSourceFile, uCurrSourceLine, 
			"Can not rename '%s', it is not defined", pszCurrentName);
	}
}

static IMG_UINT32 GetHwRegForNamedReg(IMG_UINT32 uNamedRegLink)
/*****************************************************************************
 FUNCTION	: GetHwRegForNamedReg
    
 PURPOSE	: Gets hardware register allocated to named register.

 PARAMETERS	: uNamedRegLink - Link to named register definition.
			  
 RETURNS	: IMG_UINT32	- Hardware register number allocated to named 
							  register.
*****************************************************************************/
{
	const TEMP_REG* psTempReg = (const TEMP_REG*)
		(GetModifiableNamedRegsTableEntryValue(uNamedRegLink));	
	return psTempReg->uHwRegNum;
}

static IMG_UINT32 GetHwRegForNamedRegArrayElement(
	IMG_UINT32 uNamedRegLink, 
	IMG_UINT32 uArrayElementIndex)
/*****************************************************************************
 FUNCTION	: GetHwRegForNamedRegArrayElement
    
 PURPOSE	: Gets hardware register allocated to named register array element.

 PARAMETERS	: uNamedRegLink	- Link to named register array definition.
			  uArrayElementIndex	- Array element index.
			  
 RETURNS	:  IMG_UINT32	- Hardware register number allocated to named 
							  register array element.
*****************************************************************************/
{
	const TEMP_REG* psTempReg = (const TEMP_REG*)
		(GetModifiableNamedRegsTableEntryValue(uNamedRegLink));
	if(uArrayElementIndex < psTempReg->uArrayElementsCount)
	{
		return ((psTempReg->uHwRegNum)+ uArrayElementIndex);
	}
	else
	{
		return 
			(((const TEMP_REG*) GetErrorNamedRegisterEntryValue())->uHwRegNum);	
	}
}

IMG_INTERNAL IMG_UINT32 GetArrayElementsCount(IMG_UINT32 uNamedRegLink)
/*****************************************************************************
 FUNCTION	: GetArrayElementsCount
    
 PURPOSE	: Gives array elements count associated with array definition.

 PARAMETERS	: uNamedRegLink	- Link to named register array definition.

 RETURNS	: IMG_UINT32	- Elements count associated with array definition.
*****************************************************************************/
{	
	const TEMP_REG* psTempReg = (const TEMP_REG*)
		(GetModifiableNamedRegsTableEntryValue(uNamedRegLink));
	return psTempReg->uArrayElementsCount;
}

IMG_INTERNAL IMG_BOOL IsArray(IMG_UINT32 uNamedRegLink)
/*****************************************************************************
 FUNCTION	: IsArray
    
 PURPOSE	: Tells whether the given named register definition is array or 
			  not.

 PARAMETERS	: uNamedRegLink	- Link to named register definition.

 RETURNS	: IMG_TRUE	- If named register definition is an array.
			  IMG_FALSE - Not an array.
*****************************************************************************/
{	
	const TEMP_REG* psTempReg = (const TEMP_REG*)
		(GetModifiableNamedRegsTableEntryValue(uNamedRegLink));
	if((psTempReg->uArrayElementsCount) > 0)
	{
		return IMG_TRUE;
	}
	else
	{
		return IMG_FALSE;
	}
}


IMG_INTERNAL const IMG_CHAR* GetNamedTempRegName(IMG_UINT32 uNamedRegLink)
/*****************************************************************************
 FUNCTION	: GetNamedTempRegName
    
 PURPOSE	: Gives name of named temporary register.

 PARAMETERS	: uNamedRegLink	- Link to named register definition.
			  
 RETURNS	: const IMG_CHAR*	- pointer to constant null terminated string 
								  containing register name.
*****************************************************************************/
{
	const TEMP_REG* psTempReg = (const TEMP_REG*)
		(GetModifiableNamedRegsTableEntryValue(uNamedRegLink));
	return (const IMG_CHAR*)(psTempReg->pszName);
}

static IMG_VOID GetNextNumericLabelName(IMG_PCHAR pszNumLabelName)
{
	static IMG_UINT32 uNumLabelNamsCtr;
	snprintf(pszNumLabelName, 10, "%u", uNumLabelNamsCtr);
	uNumLabelNamsCtr++;
	return;
}

IMG_INTERNAL IMG_VOID InitScopeManagementDataStructs(IMG_VOID)
/*****************************************************************************
 FUNCTION	: InitScopeManagementDataStructs
    
 PURPOSE	: Initializes scope management data structures.

 PARAMETERS	: None.
			  
 RETURNS	: Nothing.
*****************************************************************************/
{
	InitModifiableLabelsTable();
	InitModifiableNamedRegsTable();
	g_psCurrentScope = (PSCOPE)UseAsm_Malloc( sizeof( SCOPE ) );
	TestMemoryAllocation(g_psCurrentScope);
	g_psCurrentScope->pszScopeName = NULL;
	g_psCurrentScope->psTempRegs = NULL;
	g_psCurrentScope->psParentScope = NULL;
	g_psCurrentScope->psChildScopes = NULL;
	g_psCurrentScope->psPreviousSiblingScope = NULL;
	g_psCurrentScope->psNextSiblingScope = NULL;
	g_psCurrentScope->psUndefinedBranchDests = NULL;
	g_psCurrentScope->psUndefinedProcDests = NULL;
	g_psCurrentScope->psUndefinedExportDests = NULL;
	g_psCurrentScope->psUndefinedMovDests = NULL;
	g_psCurrentScope->pszSourceFile = StrdupOrNull(g_pszInFileName);
	g_psCurrentScope->uSourceLine = g_uSourceLine;
	{
		IMG_CHAR pszLabelInternName[11];
		GetNextNumericLabelName(pszLabelInternName);
		FindOrCreateLabel(pszLabelInternName, IMG_TRUE, g_pszInFileName, 
			g_uSourceLine);
	}
	g_psCurrentScope->psUndefinedTempRegs = NULL;
	g_psCurrentScope->bProc = IMG_FALSE;	
	g_psCurrentScope->bImported = IMG_FALSE;
	g_psCurrentScope->bExport = IMG_FALSE;
	g_psCurrentScope->psCalleesRecord = NULL;
	g_psCurrentScope->psCallsRecord = NULL;
	g_psCurrentScope->uMaxNamedRegsTransByCall = 0;
	g_psCurrentScope->psNamedRegsAllocParent = NULL;
	g_psCurrentScope->psNamedRegsAllocChildren = NULL;
	g_psCurrentScope->uCurrentCountOfNamedRegsInScope = 0;
	g_psCurrentScope->bDummyScope = IMG_FALSE;
	return;
}

static IMG_VOID AdjustCalled(PSCOPE psCalleeScope, 
							 PSCOPE psCalledScope, 
							 IMG_UINT32 uNamedRegsCountUsedBeforeCall);

IMG_INTERNAL IMG_UINT32 OpenScope(const IMG_CHAR* pszScopeName, 
								  IMG_BOOL bProc, 
								  IMG_BOOL bImported, 
								  const IMG_CHAR* pszSourceFile, 
								  IMG_UINT32 uSourceLine)
/*****************************************************************************
 FUNCTION	: OpenScope
    
 PURPOSE	: Opens a new scope.
 
 PARAMETERS	: pszScopeName	- Name of the scope.
			  bProc	- The scope is procedure or not.
			  bImported - The scope is imported or not.
			  pszSourceFile	- Source file name.
			  uSourceLine	- Source line number.
			  
 RETURNS	: Link to the scope definition.
*****************************************************************************/
{
	IMG_PCHAR pszScopeNameIntern = NULL;
	PSCOPE psLastSiblingScope = NULL;
	PSCOPE	psNewScope = NULL;
	if(pszScopeName != NULL)
	{
		IMG_PCHAR pszColon;
		pszScopeNameIntern = StrdupOrNull(pszScopeName);			
		if ((pszColon = strrchr(pszScopeNameIntern, ':')) != NULL)
		{
			*pszColon = 0;
		}
	}	
	if(bImported == IMG_FALSE)
	{
		if(pszScopeNameIntern != NULL)
		{
			IMG_BOOL bErrorAlreadyOccured = IMG_FALSE;	
			{
				/*first check, it should not have a name same as one of its 
				siblings*/				
				if(g_psCurrentScope->psChildScopes != NULL)
				{
					PSCOPE psSiblingScopesIterator = 
						g_psCurrentScope->psChildScopes;	
					for(;
						psSiblingScopesIterator != NULL;
						psSiblingScopesIterator = 
							psSiblingScopesIterator->psNextSiblingScope)
					{
						if((bErrorAlreadyOccured == IMG_FALSE) && 
							((psSiblingScopesIterator->pszScopeName)!=NULL) &&
							(strcmp(psSiblingScopesIterator->pszScopeName, 
								pszScopeNameIntern) == 0)
						)
						{
							ParseError("Can not create scope '%s' with name same as its siblings", 
								pszScopeNameIntern);
							ParseErrorAt(
								psSiblingScopesIterator->pszSourceFile, 
								psSiblingScopesIterator->uSourceLine, "Is the location of conflicting scope definition");
							bErrorAlreadyOccured = IMG_TRUE;
							pszScopeNameIntern = NULL;
						}
						psLastSiblingScope = psSiblingScopesIterator;
					}
				}					
			}
			if(bErrorAlreadyOccured == IMG_FALSE)
			{
				/*second check, it should not have a name same as one of its 
				parents or their siblings*/
				PSCOPE psParentScopesIterator = g_psCurrentScope->psParentScope;			
				while(psParentScopesIterator != NULL)
				{
					PSCOPE psScopeFound = NULL;				
					if(LookupChildSiblingScopes(psParentScopesIterator, 
							pszScopeNameIntern, (IMG_HVOID)(&psScopeFound)) 
						== IMG_TRUE
					)
					{
						ParseError("Can not create scope '%s' with name same as its parent or their sibling scopes", 
							pszScopeNameIntern);
						ParseErrorAt(
								psScopeFound->pszSourceFile, 
								psScopeFound->uSourceLine, "Is the location of conflicting scope definition");
						pszScopeNameIntern = NULL;
						psParentScopesIterator = NULL;
					}
					else
					{
						psParentScopesIterator = 
							psParentScopesIterator->psParentScope;					
					}
				}
			}
		}
		else
		{			
			{
				/*first check, it should not have a name same as one of its 
				siblings*/				
				if(g_psCurrentScope->psChildScopes != NULL)
				{
					PSCOPE psSiblingScopesIterator = 
						g_psCurrentScope->psChildScopes;	
					for(;
						psSiblingScopesIterator != NULL;
						psSiblingScopesIterator = 
							psSiblingScopesIterator->psNextSiblingScope)
					{						
						psLastSiblingScope = psSiblingScopesIterator;
					}
				}					
			}
		}
		{		
			if(pszScopeNameIntern != NULL)
			{
				psNewScope = UseExistingUndefinedScope(pszScopeNameIntern, 
					bProc, bImported, pszSourceFile, uSourceLine);
			}
			
			if(psNewScope == NULL)
			{
				psNewScope = (PSCOPE)UseAsm_Malloc( sizeof( SCOPE ) );
				TestMemoryAllocation(psNewScope);
				psNewScope->pszScopeName = StrdupOrNull(pszScopeNameIntern);
				psNewScope->uLinkToModifiableLabelsTable = 
					GetNextFreeModifiableLabelsTableEntry();
				if((IsCurrentScopeGlobal() == IMG_TRUE) && 
					(pszScopeNameIntern != NULL))
				{				
					psNewScope->uLinkToLabelsTable = FindOrCreateLabel(
						pszScopeNameIntern, IMG_TRUE, pszSourceFile, 
						uSourceLine);
					SetModifiableLabelsTableEntryValue(
						psNewScope->uLinkToModifiableLabelsTable, 
						psNewScope->uLinkToLabelsTable);
				}
				else
				{
					IMG_CHAR pszLabelInternName[11];
					GetNextNumericLabelName(pszLabelInternName);
					psNewScope->uLinkToLabelsTable = FindOrCreateLabel(
						pszLabelInternName, IMG_TRUE, pszSourceFile, 
						uSourceLine);
					SetModifiableLabelsTableEntryValue(
						psNewScope->uLinkToModifiableLabelsTable, 
						psNewScope->uLinkToLabelsTable);
				}
				psNewScope->psUndefinedTempRegs = NULL;
				psNewScope->bExport = IMG_FALSE;				
				psNewScope->bProc = bProc;
				psNewScope->psCalleesRecord = NULL;
				psNewScope->psCallsRecord = NULL;	
				psNewScope->uMaxNamedRegsTransByCall = 0;
				psNewScope->psNamedRegsAllocParent = NULL;
				psNewScope->psNamedRegsAllocChildren = NULL;
				psNewScope->uCurrentCountOfNamedRegsInScope = 0;
			}
			psNewScope->bImported = IMG_FALSE;
			psNewScope->bProc = bProc;
			psNewScope->psTempRegs = NULL;
			psNewScope->psChildScopes = NULL;
			psNewScope->psNextSiblingScope = NULL;
			psNewScope->psUndefinedBranchDests = NULL;
			psNewScope->psUndefinedProcDests = NULL;
			psNewScope->psUndefinedExportDests = NULL;
			psNewScope->psUndefinedMovDests = NULL;
			psNewScope->psParentScope = g_psCurrentScope;
			psNewScope->pszSourceFile = StrdupOrNull(pszSourceFile);
			psNewScope->uSourceLine = uSourceLine;
		}
	}
	else
	{
		/* Import case */
		PSCOPE psScopeFound = NULL;
		if(pszScopeNameIntern != NULL)
		{	
			/*first check, siblings*/				
			if(g_psCurrentScope->psChildScopes != NULL)
			{
				PSCOPE psSiblingScopesIterator = 
					g_psCurrentScope->psChildScopes;	
				for(;
					psSiblingScopesIterator != NULL; 
					psSiblingScopesIterator = 
						psSiblingScopesIterator->psNextSiblingScope)
				{
					if(((psSiblingScopesIterator->pszScopeName)!=NULL) &&
						(strcmp(psSiblingScopesIterator->pszScopeName, 
							pszScopeNameIntern) == 0)
					)
					{
						if(psScopeFound == NULL)
						{
							psScopeFound = psSiblingScopesIterator;
						}
					}
					psLastSiblingScope = psSiblingScopesIterator;
				}
			}					
			if(psScopeFound == NULL)
			{
				/*second check, parents or their siblings*/
				PSCOPE psParentScopesIterator = g_psCurrentScope->psParentScope;			
				while(psParentScopesIterator != NULL)
				{					
					if(LookupChildSiblingScopes(psParentScopesIterator, 
							pszScopeNameIntern, (IMG_HVOID)(&psScopeFound)) 
						== IMG_TRUE
					)
					{						
						psParentScopesIterator = NULL;
					}
					else
					{
						psParentScopesIterator = 
							psParentScopesIterator->psParentScope;					
					}
				}
			}
		}
		if(psScopeFound != NULL)
		{
			if(psScopeFound->bImported != IMG_TRUE)
			{
				IMG_CHAR pszLabelInternName[11];
				ParseError("Can not import label '%s' at this point", 
							pszScopeNameIntern);
				ParseErrorAt(psScopeFound->pszSourceFile, 
					psScopeFound->uSourceLine, "Is the location of conflicting scope definition");
				pszScopeNameIntern = NULL;
				psNewScope = (PSCOPE)UseAsm_Malloc( sizeof( SCOPE ) );
				TestMemoryAllocation(psNewScope);
				psNewScope->pszScopeName = NULL;
				GetNextNumericLabelName(pszLabelInternName);
				psNewScope->uLinkToLabelsTable = FindOrCreateLabel(
					pszLabelInternName, IMG_TRUE, pszSourceFile, uSourceLine);
				psNewScope->uLinkToModifiableLabelsTable = 
					GetNextFreeModifiableLabelsTableEntry();
				SetModifiableLabelsTableEntryValue(
					psNewScope->uLinkToModifiableLabelsTable, 
					psNewScope->uLinkToLabelsTable);
				psNewScope->psCalleesRecord = NULL;
				psNewScope->psCallsRecord = NULL;	
				psNewScope->uMaxNamedRegsTransByCall = 0;
				psNewScope->psNamedRegsAllocParent = NULL;
				psNewScope->psNamedRegsAllocChildren = NULL;
				psNewScope->uCurrentCountOfNamedRegsInScope = 0;
			}
			else
			{
				psNewScope = (PSCOPE)UseAsm_Malloc( sizeof( SCOPE ) );
				TestMemoryAllocation(psNewScope);
				psNewScope->pszScopeName = StrdupOrNull(pszScopeNameIntern);
				psNewScope->uLinkToModifiableLabelsTable = 
					GetNextFreeModifiableLabelsTableEntry();
				psNewScope->uLinkToLabelsTable = 
					psScopeFound->uLinkToLabelsTable;
				SetModifiableLabelsTableEntryValue(
					psNewScope->uLinkToModifiableLabelsTable, 
					psNewScope->uLinkToLabelsTable);	
				psNewScope->psCalleesRecord = NULL;
				psNewScope->psCallsRecord = NULL;	
				psNewScope->uMaxNamedRegsTransByCall = 0;
				psNewScope->psNamedRegsAllocParent = NULL;
				psNewScope->psNamedRegsAllocChildren = NULL;
				psNewScope->uCurrentCountOfNamedRegsInScope = 0;
			}
		}
		if(psNewScope == NULL)
		{		
			if(pszScopeNameIntern != NULL)
			{
				psNewScope = UseExistingUndefinedScope(pszScopeNameIntern, 
					bProc, bImported, pszSourceFile, uSourceLine);
			}		
			if(psNewScope == NULL)
			{
				psNewScope = (PSCOPE)UseAsm_Malloc( sizeof( SCOPE ) );
				TestMemoryAllocation(psNewScope);
				psNewScope->pszScopeName = StrdupOrNull(pszScopeNameIntern);
				psNewScope->uLinkToModifiableLabelsTable = 
					GetNextFreeModifiableLabelsTableEntry();
				psNewScope->uLinkToLabelsTable = FindOrCreateLabel(
					pszScopeNameIntern, IMG_FALSE, pszSourceFile, uSourceLine);
				SetModifiableLabelsTableEntryValue(
					psNewScope->uLinkToModifiableLabelsTable, 
					psNewScope->uLinkToLabelsTable);
			}
			psNewScope->psCalleesRecord = NULL;
			psNewScope->psCallsRecord = NULL;	
			psNewScope->uMaxNamedRegsTransByCall = 0;
			psNewScope->psNamedRegsAllocParent = NULL;
			psNewScope->psNamedRegsAllocChildren = NULL;
			psNewScope->uCurrentCountOfNamedRegsInScope = 0;
		}
		psNewScope->psTempRegs = NULL;
		psNewScope->psChildScopes = NULL;
		psNewScope->psNextSiblingScope = NULL;
		psNewScope->psUndefinedBranchDests = NULL;
		psNewScope->psUndefinedProcDests = NULL;
		psNewScope->psUndefinedExportDests = NULL;
		psNewScope->psUndefinedMovDests = NULL;
		psNewScope->psParentScope = g_psCurrentScope;
		psNewScope->pszSourceFile = StrdupOrNull(pszSourceFile);
		psNewScope->uSourceLine = uSourceLine;
		psNewScope->bExport = IMG_FALSE;
		psNewScope->bImported = IMG_TRUE;
		psNewScope->bProc = IMG_FALSE;
		psNewScope->psUndefinedTempRegs = NULL;
	}
	{
		psNewScope->bDummyScope = IMG_FALSE;
	}
	if(psLastSiblingScope != NULL)
	{
		psLastSiblingScope->psNextSiblingScope = psNewScope;
		psNewScope->psPreviousSiblingScope = psLastSiblingScope;			
	}
	else
	{
		psNewScope->psPreviousSiblingScope = NULL;
		g_psCurrentScope->psChildScopes = psNewScope;
	}
	AdjustCalled(g_psCurrentScope, psNewScope, 
		g_psCurrentScope->uCurrentCountOfNamedRegsInScope);
	g_psCurrentScope = psNewScope;
	if(pszScopeNameIntern != NULL)
	{
		UseAsm_Free(pszScopeNameIntern);
	}
	return psNewScope->uLinkToModifiableLabelsTable;	
}

IMG_INTERNAL IMG_UINT32 ReOpenLastScope(const IMG_CHAR* pszSourceFile, 
										IMG_UINT32 uSourceLine)
/*****************************************************************************
 FUNCTION	: ReOpenLastScope

 PURPOSE	: Reopens the last closed scope.
 
 PARAMETERS	: pszSourceFile	- Source file name.
			  uSourceLine	- Source line number.
			  
 RETURNS	: Link to the scope definition.
*****************************************************************************/
{

	PSCOPE psChildSiblingScopsItrator = g_psCurrentScope->psChildScopes;
	for(; psChildSiblingScopsItrator->psNextSiblingScope != NULL; 
		psChildSiblingScopsItrator = 
			psChildSiblingScopsItrator->psNextSiblingScope);
	UseAsm_Free(psChildSiblingScopsItrator->pszSourceFile);
	psChildSiblingScopsItrator->pszSourceFile = StrdupOrNull(pszSourceFile);
	psChildSiblingScopsItrator->uSourceLine = uSourceLine;	
	g_psCurrentScope = psChildSiblingScopsItrator;
	if(g_psCurrentScope->bProc == IMG_TRUE)
	{
		IMG_UINT32 uValueToReturn = OpenScope(NULL, IMG_FALSE, IMG_FALSE, 
			pszSourceFile, uSourceLine);
		g_psCurrentScope->bDummyScope = IMG_TRUE;
		return uValueToReturn;
	}
	else
	{
		return psChildSiblingScopsItrator->uLinkToModifiableLabelsTable;
	}
}

static IMG_VOID MovUndefinedChildsFromChildToParent(PSCOPE parentScope, 
													PSCOPE childScope)
/*****************************************************************************
 FUNCTION	: MovUndefinedChildsFromChildToParent
    
 PURPOSE	: Moves undefined scopes from child scope to parent scope.

 PARAMETERS	: parentScope	- Pointer to parent scope.
			  childScope	- Pointer to child scope.

 RETURNS	: Nothing.
*****************************************************************************/
{
	{
		PSCOPE parentUndefBranchDestsEnd = parentScope;
		while(parentUndefBranchDestsEnd->psUndefinedBranchDests != NULL)
		{
			parentUndefBranchDestsEnd = 
				parentUndefBranchDestsEnd->psUndefinedBranchDests;
		}
		parentUndefBranchDestsEnd->psUndefinedBranchDests = 
			childScope->psUndefinedBranchDests;
		childScope->psUndefinedBranchDests = NULL;
	}
	{
		PSCOPE parentUndefGlobalDestsEnd = parentScope;
		while(parentUndefGlobalDestsEnd->psUndefinedProcDests != NULL)
		{
			parentUndefGlobalDestsEnd = 
				parentUndefGlobalDestsEnd->psUndefinedProcDests;
		}
		parentUndefGlobalDestsEnd->psUndefinedProcDests = 
			childScope->psUndefinedProcDests;
		childScope->psUndefinedProcDests = NULL;
	}
	{
		PSCOPE parentUndefGlobalDestsEnd = parentScope;
		while(parentUndefGlobalDestsEnd->psUndefinedExportDests != NULL)
		{
			parentUndefGlobalDestsEnd = 
				parentUndefGlobalDestsEnd->psUndefinedExportDests;
		}
		parentUndefGlobalDestsEnd->psUndefinedExportDests = 
			childScope->psUndefinedExportDests;
		childScope->psUndefinedExportDests = NULL;
	}
	{
		PSCOPE parentUndefGlobalDestsEnd = parentScope;
		while(parentUndefGlobalDestsEnd->psUndefinedMovDests != NULL)
		{
			parentUndefGlobalDestsEnd = 
				parentUndefGlobalDestsEnd->psUndefinedMovDests;
		}
		parentUndefGlobalDestsEnd->psUndefinedMovDests = 
			childScope->psUndefinedMovDests;
		childScope->psUndefinedMovDests = NULL;
	}
	return;
}

static IMG_VOID CloseScopeIntern(IMG_VOID);

IMG_INTERNAL IMG_VOID CloseScope(IMG_VOID)
/*****************************************************************************
 FUNCTION	: CloseScope
    
 PURPOSE	: Closes scope and dummy scope.
 
 PARAMETERS	: None.
			  
 RETURNS	: Nothing.
*****************************************************************************/
{	
	if(g_psCurrentScope->bDummyScope == IMG_TRUE)
	{
		CloseScopeIntern();
	}
	CloseScopeIntern();
	return;
}

static IMG_VOID CloseScopeIntern(IMG_VOID)
/*****************************************************************************
 FUNCTION	: CloseScopeItern
    
 PURPOSE	: Closes just the current scope.
 
 PARAMETERS	: None.
			  
 RETURNS	: Nothing.
*****************************************************************************/
{	
	if(g_psCurrentScope->psParentScope != NULL)
	{
		MovUndefinedChildsFromChildToParent(g_psCurrentScope->psParentScope, 
			g_psCurrentScope);
		g_psCurrentScope = g_psCurrentScope->psParentScope;		
	}
	else
	{	
		ParseError("Extra scope end bracket present");
	}	
	return;
}

IMG_INTERNAL IMG_BOOL IsCurrentScopeLinkedToModifiableLabel(
	IMG_UINT32 uLinkToModifiableLabelsTable)
/*****************************************************************************
 FUNCTION	: IsCurrentScopeLinkedToModifiableLabel
    
 PURPOSE	: Tests that whether current scope is linked to specified 
			  modifiable label link.

 PARAMETERS	: uLinkToModifiableLabelsTable - Link to label entry in modifiable 
			  labels table.

 RETURNS	: IMG_TRUE - If label is linked to current scope.
			  IMG_FALSE - If label is not linked to current scope.
*****************************************************************************/
{
	if(g_psCurrentScope->uLinkToModifiableLabelsTable == 
		uLinkToModifiableLabelsTable)
	{
		return IMG_TRUE;
	}
	else
	{
		return IMG_FALSE;
	}
}

IMG_INTERNAL IMG_BOOL LookupParentScopes(const IMG_CHAR* pszName, 
										 IMG_HVOID ppvHandle)
/*****************************************************************************
 FUNCTION	: LookupParentScopes
    
 PURPOSE	: Searches scope definition in parent scopes of current scope, also 
			  includes current scope in look up.

 PARAMETERS	: pszName		- Name of the scope to search.
			  ppvHandle		- Pointer to void pointer containing scope 
							  definition handle.

 RETURNS	: IMG_TRUE	- If the scope exists.
				ppvHandle	- Will contain the scope definition handle.
			  IMG_FALSE	- If the scope does not exist.				
*****************************************************************************/
{	
	PSCOPE psParentScopesIterator = NULL;
	for(psParentScopesIterator = g_psCurrentScope;
		psParentScopesIterator != NULL;
		psParentScopesIterator = psParentScopesIterator->psParentScope)
	{
		if((psParentScopesIterator->pszScopeName != NULL) && 
			(strcmp(psParentScopesIterator->pszScopeName, pszName) == 0)
		)
		{
			*ppvHandle = (IMG_PVOID)(psParentScopesIterator);
			return IMG_TRUE;
		}
	}
	*ppvHandle = NULL;
	return IMG_FALSE;
}

static IMG_BOOL LookupChildSiblingScopes(const SCOPE* psScope, 
										 const IMG_CHAR* pszName, 
										 IMG_HVOID ppvHandle)
/*****************************************************************************
 FUNCTION	: LookupSiblingScopes
    
 PURPOSE	: Searches scope definition in child sibling scopes.

 PARAMETERS	: psScope	- Is the scope whose child siblings will be 
						  searched.
			  pszName	- Name of the scope to search.
			  ppvHandle	- pointer to void pointer containing scope 
						  definition handle.

 RETURNS	: IMG_TRUE	- If the scope exists.
				ppvHandle	- Will contain the scope definition handle.
			  IMG_FALSE	- If the scope does not exist.				
*****************************************************************************/
{	
	PSCOPE psChildSiblingScopsItrator = psScope->psChildScopes;
	for(;
		psChildSiblingScopsItrator != NULL;
		psChildSiblingScopsItrator = 
			psChildSiblingScopsItrator->psNextSiblingScope)
	{
		if((psChildSiblingScopsItrator->pszScopeName != NULL) && 
			(strcmp(psChildSiblingScopsItrator->pszScopeName, pszName) == 0)
		)
		{
			*ppvHandle = (IMG_PVOID)(psChildSiblingScopsItrator);
			return IMG_TRUE;
		}
	}
	*ppvHandle = NULL;
	return IMG_FALSE;
}

static PCALL_RECORD CheckIfCalledFromScope(PSCOPE psCurrentScope, 
										   PSCOPE psCalledScope)
/*****************************************************************************
 FUNCTION	: CheckIfCalledFromScope.
    
 PURPOSE	: Tests whether a specified scope is called from specific scope or 
			  not.

 PARAMETERS	: psCurrentScope - Is the pointer to calling scope.
			  psCalledScope	- Is the pointer to called scope.			  

 RETURNS	: Pointer to call record of scope, as it is already called.
			  NULL - If the call does not exist.
*****************************************************************************/
{
	PCALL_RECORD psCallRecordsIterator = psCurrentScope->psCallsRecord;
	while(psCallRecordsIterator != NULL)
	{
		if((psCallRecordsIterator->psDestScope) == psCalledScope)
		{
			return psCallRecordsIterator;
		}		
		psCallRecordsIterator = psCallRecordsIterator->psNext;
	}
	return NULL;
}

static PCALL_RECORD CheckIfCalleeInScope(PSCOPE psCurrentScope, 
										 PSCOPE psCalleeScope)
/*****************************************************************************
 FUNCTION	: CheckIfCalleeInScope
    
 PURPOSE	: Tests whether a specified scope is callee in specific scope or 
			  not.

 PARAMETERS	: psCurrentScope - Is the pointer to current scope.
			  psCalleeScope	- Is the pointer to callee scope.			  

 RETURNS	: Pointer to call record of scope, as it is already callee.
			  NULL - If the callee does not exist.
*****************************************************************************/
{
	PCALL_RECORD psCalleeRecordsIterator = psCurrentScope->psCalleesRecord;
	while(psCalleeRecordsIterator != NULL)
	{
		if((psCalleeRecordsIterator->psDestScope) == psCalleeScope)
		{
			return psCalleeRecordsIterator;
		}		
		psCalleeRecordsIterator = psCalleeRecordsIterator->psNext;
	}
	return NULL;
}

static PCALL_RECORD AddCalleeToScope(PSCOPE psCurrentScope, PSCOPE psCalleeScope)
/*****************************************************************************
 FUNCTION	: AddCalleeToScope
    
 PURPOSE	: Adds a callee to scope.

 PARAMETERS	: psCurrentScope - Is the pointer to current scope.
			  psCalleeScope	- Is the pointer to callee scope.			  

 RETURNS	: Pointer to call record of scope which is callee.			  
*****************************************************************************/
{
	PCALL_RECORD psNewCalleeRecord = (PCALL_RECORD) UseAsm_Malloc(sizeof(CALL_RECORD));
	TestMemoryAllocation(psNewCalleeRecord);
	psNewCalleeRecord->psDestScope = psCalleeScope;
	psNewCalleeRecord->psNext = NULL;
	if(psCurrentScope->psCalleesRecord == NULL)
	{
		psCurrentScope->psCalleesRecord = psNewCalleeRecord;
	}
	else
	{
		PCALL_RECORD psCalleeRecordsIterator = psCurrentScope->psCalleesRecord;
		while(psCalleeRecordsIterator->psNext != NULL)
		{			
			psCalleeRecordsIterator = psCalleeRecordsIterator->psNext;
		}
		psCalleeRecordsIterator->psNext = psNewCalleeRecord;
	}
	return psNewCalleeRecord;
}

static PCALL_RECORD GetLargestNamedRegsCountCallee(PSCOPE psScope)
/*****************************************************************************
 FUNCTION	: GetLargestNamedRegsCountCallee
    
 PURPOSE	: Gets a call record of callee transfering largest number of named 
			  registers count while calling.

 PARAMETERS	: psScope - Is the pointer to current scope.			  

 RETURNS	: Pointer to call record of callee transfering largest number of 
			  named registers count while calling.
*****************************************************************************/
{
	PCALL_RECORD psCalleeRecordsIterator = psScope->psCalleesRecord;
	PCALL_RECORD psLargestCalleeRecord = psCalleeRecordsIterator;
	while(psCalleeRecordsIterator != NULL)
	{
		if((psLargestCalleeRecord->uNamedRegsCount) < 
			(psCalleeRecordsIterator->uNamedRegsCount))
		{
			psLargestCalleeRecord = psCalleeRecordsIterator;					
		}
		psCalleeRecordsIterator = psCalleeRecordsIterator->psNext;
	}
	return psLargestCalleeRecord;
}

static IMG_VOID DeleteNamedRegAllocChild(PSCOPE psScope, PSCOPE psScopeToDelete)
/*****************************************************************************
 FUNCTION	: DeleteNamedRegAllocChild
    
 PURPOSE	: Deletes specified register allocation child from specified scope.

 PARAMETERS	: psScope - Is the pointer to current scope.
			  psScopeToDelete	- Scope to delete.

 RETURNS	: Nothing.
*****************************************************************************/
{
	PSCOPES_LIST psScopesListIterator = psScope->psNamedRegsAllocChildren;
	PSCOPES_LIST psPreviousScopesListIterator = NULL;	
	while(psScopesListIterator != NULL)
	{
		if(psScopesListIterator->psScope == psScopeToDelete)
		{
			PSCOPES_LIST psToDelete;			
			if(psPreviousScopesListIterator != NULL)
			{
				psPreviousScopesListIterator->psNext = psScopesListIterator->psNext;				
			}
			else
			{
				psScope->psNamedRegsAllocChildren = psScopesListIterator->psNext;
			}
			psToDelete = psScopesListIterator;
			psScopesListIterator = psScopesListIterator->psNext;
			UseAsm_Free(psToDelete);
		}
		else
		{
			psPreviousScopesListIterator = psScopesListIterator;
			psScopesListIterator = psScopesListIterator->psNext;
		}
	}		
}

static IMG_VOID AddNamedRegAllocChild(PSCOPE psScope, PSCOPE psScopeToAdd, 
									  IMG_UINT32 uCurrentNamedRegsCount)
/*****************************************************************************
 FUNCTION	: AddNamedRegAllocChild
    
 PURPOSE	: Adds specified register allocation child to specified scope.

 PARAMETERS	: psScope - Is the pointer to current scope.
			  psScopeToAdd	- Scope to add.
			  uCurrentNamedRegsCount - Is the number of named registers 
									   currently defined current scope.
 RETURNS	: Nothing.
*****************************************************************************/
{
	PSCOPES_LIST psNewScopeList = UseAsm_Malloc(sizeof(SCOPES_LIST));
	TestMemoryAllocation(psNewScopeList);
	psNewScopeList->psScope = psScopeToAdd;
	psNewScopeList->uNamedRegsCount = uCurrentNamedRegsCount;
	psNewScopeList->psNext = NULL;
	if(psScope->psNamedRegsAllocChildren == NULL)
	{
		psScope->psNamedRegsAllocChildren = psNewScopeList;			
	}
	else
	{
		PSCOPES_LIST psScopesListIterator = psScope->psNamedRegsAllocChildren;
		while(psScopesListIterator->psNext != NULL)
		{
			psScopesListIterator = psScopesListIterator->psNext;
		}
		psScopesListIterator->psNext = psNewScopeList;
	}
}

static IMG_VOID AdjustCalled(PSCOPE psCalleeScope, PSCOPE psCalledScope, 
							 IMG_UINT32 uNamedRegsCountUsedBeforeCall)
/*****************************************************************************
 FUNCTION	: AdjustCalled
    
 PURPOSE	: Adjusts all called records after the addition of new call from 
			  one scope to another scope, while adjusting caculate new maximum 
			  named registers counts transferd by each effecting call and 
			  adjusts register allocation children accordingly.

 PARAMETERS	: psCalleeScope - Is the pointer to calling scope.
			  psCalledScope	- Is the pointer to called scope.
			  uCurrentNamedRegsCount - Is the number of named registers 
									   currently defined calling scope.
 RETURNS	: Nothing.
*****************************************************************************/
{
	IMG_BOOL bProceedFurther = IMG_FALSE;
	IMG_BOOL bValueIncreased = IMG_FALSE;
	PCALL_RECORD psAlreadyCallee = CheckIfCalleeInScope(psCalledScope, 
		psCalleeScope);
	if(psAlreadyCallee == NULL)
	{
		PCALL_RECORD psTempCalleeRecord = AddCalleeToScope(psCalledScope, 
			psCalleeScope);
		psTempCalleeRecord->uNamedRegsCount = 
			psCalleeScope->uMaxNamedRegsTransByCall + 
			uNamedRegsCountUsedBeforeCall;
		bProceedFurther = IMG_TRUE;
	}
	else
	{
		if((psAlreadyCallee->uNamedRegsCount) < 
			((psCalleeScope->uMaxNamedRegsTransByCall) + 
			uNamedRegsCountUsedBeforeCall))
		{
			(psAlreadyCallee->uNamedRegsCount) = 
				((psCalleeScope->uMaxNamedRegsTransByCall) + 
				uNamedRegsCountUsedBeforeCall);
			bProceedFurther = IMG_TRUE;
			bValueIncreased = IMG_TRUE;
		}
	}
	if(bProceedFurther == IMG_TRUE)
	{
		PCALL_RECORD psLargestCallee = 
			GetLargestNamedRegsCountCallee(psCalledScope);
		if(psLargestCallee->psDestScope == 
			psCalledScope->psNamedRegsAllocParent)
		{
			if(bValueIncreased == IMG_FALSE)
			{
				return;
			}
		}
		{
			if(psCalledScope->psNamedRegsAllocParent == NULL)
			{
				psCalledScope->psNamedRegsAllocParent = 
					psLargestCallee->psDestScope;						
			}
			else
			{
				DeleteNamedRegAllocChild(psCalledScope->psNamedRegsAllocParent, 
					psCalledScope);
				psCalledScope->psNamedRegsAllocParent = 
					psLargestCallee->psDestScope;
			}
			AddNamedRegAllocChild(psCalledScope->psNamedRegsAllocParent, 
				psCalledScope, uNamedRegsCountUsedBeforeCall);
			psCalledScope->uMaxNamedRegsTransByCall = 
				psLargestCallee->uNamedRegsCount;
		}
		{
			PCALL_RECORD psCallRecordsIterator = psCalledScope->psCallsRecord;
			while(psCallRecordsIterator != NULL)
			{
				AdjustCalled(psCalledScope, psCallRecordsIterator->psDestScope, 
					psCallRecordsIterator->uNamedRegsCount);
				psCallRecordsIterator = psCallRecordsIterator->psNext;			
			}
		}
	}	
}

static PCALL_RECORD AddCallToScope(PSCOPE psCurrentScope, PSCOPE psCalledScope)
/*****************************************************************************
 FUNCTION	: AdjustCalled
    
 PURPOSE	: Adds a new call record to current scope, adds callee to called, 
			  Adjusts all called records after the addition of new call from 
			  one scope to another scope, while adjusting caculate new maximum 
			  named registers counts transferd by each effecting call and 
			  adjusts register allocation children accordingly.

 PARAMETERS	: psCurrentScope - Is the pointer to calling scope.
			  psCalledScope	- Is the pointer to called scope.

 RETURNS	: Nothing.
*****************************************************************************/
{
	PCALL_RECORD psNewCallRecord = (PCALL_RECORD) UseAsm_Malloc(sizeof(CALL_RECORD));
	TestMemoryAllocation(psNewCallRecord);
	psNewCallRecord->psDestScope = psCalledScope;
	psNewCallRecord->psNext = NULL;
	if(psCurrentScope->psCallsRecord == NULL)
	{
		psCurrentScope->psCallsRecord = psNewCallRecord;
	}
	else
	{
		PCALL_RECORD psCallRecordsIterator = psCurrentScope->psCallsRecord;
		while(psCallRecordsIterator->psNext != NULL)
		{			
			psCallRecordsIterator = psCallRecordsIterator->psNext;
		}
		psCallRecordsIterator->psNext = psNewCallRecord;
	}
	return psNewCallRecord;
}

static IMG_BOOL TestRecursion(PSCOPE psCurrentScope, PSCOPE psCalledScope)
/*****************************************************************************
 FUNCTION	: AdjustCalled
    
 PURPOSE	: Find presence of recursion in program.

 PARAMETERS	: psCurrentScope - Is the pointer to calling scope.
			  psCalledScope	- Is the pointer to called scope.

 RETURNS	: IMG_TRUE - Recursion is detected.
			  IMG_TRUE - No recursion found.
*****************************************************************************/
{
	IMG_BOOL bValueToReturn = IMG_FALSE;
	if(psCurrentScope == NULL)
	{
		return IMG_FALSE;	
	}
	if(psCurrentScope == psCalledScope)
	{
		ParseError("Recursive call of '%s'", psCalledScope->pszScopeName);
		bValueToReturn = IMG_TRUE;
	}
	{
		PCALL_RECORD psCalleeRecordIterator = psCurrentScope->psCalleesRecord;
		while(psCalleeRecordIterator != NULL)
		{
			if(TestRecursion(psCalleeRecordIterator->psDestScope, 
				psCalledScope) == IMG_TRUE)
			{
				bValueToReturn = IMG_TRUE;							
			}
			psCalleeRecordIterator = psCalleeRecordIterator->psNext;
		}
	}
	return bValueToReturn;
}

static IMG_VOID AddToCallsRecordOfScope(PSCOPE psCurrentScope, 
										PSCOPE psCalledScope, 
										IMG_UINT32 
											uNamedRegsCountUsedBeforeCall)
/*****************************************************************************
 FUNCTION	: AddToCallsRecordOfScope
    
 PURPOSE	: Adds call record to specified scope against specified call.

 PARAMETERS	: psCurrentScope - Is the pointer to scope to add call.
			  psCalledScope	- Is the pointer to called scope.
			  uNamedRegsCountUsedBeforeCall - Is the count of named registers 
											  defined in calling scope before 
											  making a call.

 RETURNS	: Nothing.
*****************************************************************************/
{
	if(TestRecursion(psCurrentScope, psCalledScope) == IMG_FALSE)
	{
		PCALL_RECORD psAlreadyCalled = CheckIfCalledFromScope(psCurrentScope, 
			psCalledScope);
		if(psAlreadyCalled != NULL)
		{
			if((psAlreadyCalled->uNamedRegsCount) < 
				uNamedRegsCountUsedBeforeCall)
			{
				(psAlreadyCalled->uNamedRegsCount) = 
					uNamedRegsCountUsedBeforeCall;
				AdjustCalled(psCurrentScope, psAlreadyCalled->psDestScope, 
					uNamedRegsCountUsedBeforeCall);
			}
		}
		else
		{
			psAlreadyCalled = AddCallToScope(psCurrentScope, psCalledScope);
			(psAlreadyCalled->uNamedRegsCount) = uNamedRegsCountUsedBeforeCall;
			AdjustCalled(psCurrentScope, psCalledScope, 
				uNamedRegsCountUsedBeforeCall);	
		}
	}
}

static IMG_VOID AddToCallsRecordOfEarlierScope(PSCOPE psEarlierScope, 
											   PSCOPE psCalledScope)
/*****************************************************************************
 FUNCTION	: AddToCallsRecordOfEarlierScope
    
 PURPOSE	: Adds call record to specified earlier scope against specified	
			  call left to be added due to forward reference in it.

 PARAMETERS	: psEarlierScope - Is the pointer to data containing earlier call 
			  information.
			  psCalledScope	- Is the pointer to called scope.

 RETURNS	: Nothing.
*****************************************************************************/
{
	AddToCallsRecordOfScope(psEarlierScope->psCallsRecord->psDestScope, 
		psCalledScope, 
		psEarlierScope->psCallsRecord->uNamedRegsCount);
	UseAsm_Free(psEarlierScope->psCallsRecord);
	psEarlierScope->psCallsRecord = NULL;
}

IMG_INTERNAL IMG_UINT32 FindScope(const IMG_CHAR* pszScopeName, 
								  SCOPE_TYPE eScopeType, 
								  const IMG_CHAR* pszSourceFile, 
								  IMG_UINT32 uSourceLine)
/*****************************************************************************
 FUNCTION	: FindScope
    
 PURPOSE	: Finds scope.

 PARAMETERS	: pszScopeName	- Name of the scope to search.
			  eScopeType - Type of the scope to search.			  
			  pszSourceFile	- Source file where search is required.
			  uSourceLine - Source line where searche is required.

 RETURNS	: Link to modifiable labels table.
*****************************************************************************/
{	
	if(eScopeType == PROCEDURE_SCOPE)
	{
		PSCOPE psParentScopesItrator = g_psCurrentScope;
		PSCOPE psScopeFound = NULL;
		for(;
			psParentScopesItrator != NULL;
			psParentScopesItrator = 
				psParentScopesItrator->psParentScope)
		{
			if(LookupChildSiblingScopes(psParentScopesItrator, pszScopeName, 
							(IMG_HVOID)(&psScopeFound)) == IMG_TRUE)
			{
				if(psScopeFound->bImported == IMG_TRUE)
				{
					AddToCallsRecordOfScope(g_psCurrentScope, psScopeFound, 
						g_psCurrentScope->uCurrentCountOfNamedRegsInScope);					
					return psScopeFound->uLinkToModifiableLabelsTable;
				}
			}
		}
	}
	if(eScopeType == EXPORT_SCOPE)
	{
		PSCOPE psParentScopesItrator = g_psCurrentScope;
		PSCOPE psScopeFound = NULL;
		for(;
			psParentScopesItrator != NULL;
			psParentScopesItrator = 
				psParentScopesItrator->psParentScope)
		{
			if(LookupChildSiblingScopes(psParentScopesItrator, pszScopeName, 
							(IMG_HVOID)(&psScopeFound)) == IMG_TRUE)
			{
				if(psScopeFound->bImported == IMG_TRUE)
				{
					ParseError("Can not export '%s' at this point", 
						pszScopeName);
					ParseErrorAt(psScopeFound->pszSourceFile, 
						psScopeFound->uSourceLine, 
						"Is the location where '%s' is already imported", 
						psScopeFound->pszScopeName);
					return psScopeFound->uLinkToModifiableLabelsTable;
				}
			}
		}
		psParentScopesItrator = g_psCurrentScope;
		for(;
			psParentScopesItrator->psParentScope != NULL;
			psParentScopesItrator = 
				psParentScopesItrator->psParentScope);
		if(LookupChildSiblingScopes(psParentScopesItrator, pszScopeName, 
			(IMG_HVOID)(&psScopeFound)) == IMG_TRUE)
		{
			if(psScopeFound->bExport != IMG_TRUE)
			{
				SetLabelExportImportState(psScopeFound->uLinkToLabelsTable, 
					pszSourceFile, uSourceLine, IMG_FALSE, IMG_TRUE);
				psScopeFound->bExport = IMG_TRUE;
			}			
			return psScopeFound->uLinkToModifiableLabelsTable;
		}
	}
	if(eScopeType == PROCEDURE_SCOPE)
	{
		PSCOPE psParentScopesItrator = g_psCurrentScope;
		PSCOPE psScopeFound = NULL;
		for(;
			psParentScopesItrator->psParentScope != NULL;
			psParentScopesItrator = 
				psParentScopesItrator->psParentScope);
		if(LookupChildSiblingScopes(psParentScopesItrator, pszScopeName, 
			(IMG_HVOID)(&psScopeFound)) == IMG_TRUE)
		{
			if(IsScopeManagementEnabled() == IMG_TRUE)
			{
				if(psScopeFound->bProc != IMG_TRUE)
				{
					ParseError("'%s' is not a procedure", 
						psScopeFound->pszScopeName);
				}
			}
			AddToCallsRecordOfScope(g_psCurrentScope, psScopeFound, 
				g_psCurrentScope->uCurrentCountOfNamedRegsInScope);
			return psScopeFound->uLinkToModifiableLabelsTable;
		}		
		{
			PSCOPE psLastUndefinedGlobalDest = g_psCurrentScope;
			PSCOPE psUndefinedGlobalDestsItrator = 
				g_psCurrentScope->psUndefinedProcDests;
			for(;
				psUndefinedGlobalDestsItrator != NULL;
				psUndefinedGlobalDestsItrator = 
				psUndefinedGlobalDestsItrator->psUndefinedProcDests)
			 {
				if(strcmp(psUndefinedGlobalDestsItrator->pszScopeName, 
					pszScopeName) == 0)
				{
					return psUndefinedGlobalDestsItrator->uLinkToModifiableLabelsTable;
				}
				psLastUndefinedGlobalDest = psUndefinedGlobalDestsItrator;
			}
			{
				PSCOPE psNewScope = NULL;
				psNewScope = (PSCOPE)UseAsm_Malloc( sizeof( SCOPE ) );
				TestMemoryAllocation(psNewScope);
				psNewScope->pszScopeName = StrdupOrNull(pszScopeName);
				psNewScope->pszSourceFile = StrdupOrNull(pszSourceFile);
				psNewScope->uSourceLine = uSourceLine;
				psNewScope->uLinkToModifiableLabelsTable = 
					GetNextFreeModifiableLabelsTableEntry();
				SetModifiableLabelsTableEntryValue(
					psNewScope->uLinkToModifiableLabelsTable, IMG_UINT32_MAX);
				psNewScope->bProc = IMG_TRUE;				
				psNewScope->bImported = IMG_FALSE;
				psNewScope->bExport = IMG_FALSE;				
				{
					psLastUndefinedGlobalDest->psUndefinedProcDests = 
						psNewScope;
					psNewScope->psUndefinedProcDests = NULL;
				}
				psNewScope->psCallsRecord = UseAsm_Malloc( sizeof(CALL_RECORD) );
				TestMemoryAllocation(psNewScope->psCallsRecord);
				psNewScope->psCallsRecord->uNamedRegsCount = 
					g_psCurrentScope->uCurrentCountOfNamedRegsInScope;
				psNewScope->psCallsRecord->psDestScope = g_psCurrentScope;
				return psNewScope->uLinkToModifiableLabelsTable;
			}
		}
	}
	else if(eScopeType == EXPORT_SCOPE)
	{
		PSCOPE psParentScopesItrator = g_psCurrentScope;
		PSCOPE psScopeFound = NULL;
		for(;
			psParentScopesItrator->psParentScope != NULL;
			psParentScopesItrator = 
				psParentScopesItrator->psParentScope);
		if(LookupChildSiblingScopes(psParentScopesItrator, pszScopeName, 
			(IMG_HVOID)(&psScopeFound)) == IMG_TRUE)
		{			
			return psScopeFound->uLinkToModifiableLabelsTable;
		}		
		{
			PSCOPE psLastUndefinedGlobalDest = g_psCurrentScope;
			PSCOPE psUndefinedGlobalDestsItrator = 
				g_psCurrentScope->psUndefinedExportDests;
			for(;
				psUndefinedGlobalDestsItrator != NULL;
				psUndefinedGlobalDestsItrator = 
				psUndefinedGlobalDestsItrator->psUndefinedExportDests)
			{
				if(strcmp(psUndefinedGlobalDestsItrator->pszScopeName, pszScopeName) == 0)
				{
					return psUndefinedGlobalDestsItrator->uLinkToModifiableLabelsTable;					
				}
				psLastUndefinedGlobalDest = psUndefinedGlobalDestsItrator;
			}
			{
				PSCOPE psNewScope = NULL;
				psNewScope = (PSCOPE)UseAsm_Malloc( sizeof( SCOPE ) );
				TestMemoryAllocation(psNewScope);				
				psNewScope->pszScopeName = StrdupOrNull(pszScopeName);
				psNewScope->pszSourceFile = StrdupOrNull(pszSourceFile);
				psNewScope->uSourceLine = uSourceLine;
				/*testing*/
				psNewScope->uLinkToLabelsTable = 
					FindOrCreateLabel(psNewScope->pszScopeName, IMG_FALSE, 
					psNewScope->pszSourceFile, psNewScope->uSourceLine);
				SetLabelExportImportState(psNewScope->uLinkToLabelsTable, 
					psNewScope->pszSourceFile, psNewScope->uSourceLine, 
					IMG_FALSE, IMG_TRUE);
				/**/
				psNewScope->uLinkToModifiableLabelsTable = 
					GetNextFreeModifiableLabelsTableEntry();
				SetModifiableLabelsTableEntryValue(
					psNewScope->uLinkToModifiableLabelsTable, IMG_UINT32_MAX);
				psNewScope->bProc = IMG_FALSE;				
				psNewScope->bImported = IMG_FALSE;
				psNewScope->bExport = IMG_TRUE;
				{
					psLastUndefinedGlobalDest->psUndefinedExportDests = 
						psNewScope;
					psNewScope->psUndefinedExportDests = NULL;
				}				
				return psNewScope->uLinkToModifiableLabelsTable;
			}
		}		
	}
	else
	{
		PSCOPE psParentScopesItrator = g_psCurrentScope;
		PSCOPE psScopeFound = NULL;
		for(;
			psParentScopesItrator != NULL;
			psParentScopesItrator = 
				psParentScopesItrator->psParentScope)
		{
			if(LookupChildSiblingScopes(psParentScopesItrator, pszScopeName, 
							(IMG_HVOID)(&psScopeFound)) == IMG_TRUE)
			{
				if(IsScopeManagementEnabled() == IMG_TRUE)
				{
					if(psScopeFound->bProc == IMG_TRUE)
					{
						ParseError("Can not branch to procedure '%s'", 
							psScopeFound->pszScopeName);
					}
				}
				return psScopeFound->uLinkToModifiableLabelsTable;
			}
		}		
		{
			PSCOPE psLastUndefinedBranchDest = g_psCurrentScope;
			PSCOPE psUndefinedBranchDestsItrator = 
				g_psCurrentScope->psUndefinedBranchDests;
			for(;
				psUndefinedBranchDestsItrator != NULL;
				psUndefinedBranchDestsItrator = 
				psUndefinedBranchDestsItrator->psUndefinedBranchDests)
			{
				if(strcmp(psUndefinedBranchDestsItrator->pszScopeName, 
					pszScopeName) == 0)
				{
					return psUndefinedBranchDestsItrator->
						uLinkToModifiableLabelsTable;
				}
				psLastUndefinedBranchDest = psUndefinedBranchDestsItrator;
			}
			{
				PSCOPE psNewScope = NULL;
				psNewScope = (PSCOPE)UseAsm_Malloc( sizeof( SCOPE ) );
				TestMemoryAllocation(psNewScope);
				psNewScope->pszScopeName = StrdupOrNull(pszScopeName);
				psNewScope->pszSourceFile = StrdupOrNull(pszSourceFile);
				psNewScope->uSourceLine = uSourceLine;
				psNewScope->uLinkToModifiableLabelsTable = 
					GetNextFreeModifiableLabelsTableEntry();
				SetModifiableLabelsTableEntryValue(
					psNewScope->uLinkToModifiableLabelsTable, IMG_UINT32_MAX);
				psNewScope->bProc = IMG_FALSE;				
				psNewScope->bImported = IMG_FALSE;
				psNewScope->bExport = IMG_FALSE;
				{
					psLastUndefinedBranchDest->psUndefinedBranchDests = 
						psNewScope;
					psNewScope->psUndefinedBranchDests = NULL;
				}
				return psNewScope->uLinkToModifiableLabelsTable;
			}
		}		
	}	
}

IMG_INTERNAL IMG_BOOL LookupProcScope(const IMG_CHAR* pszScopeName, 
									  const IMG_CHAR* pszSourceFile, 
									  IMG_UINT32 uSourceLine, 
									  IMG_HVOID ppvHandle)
/*****************************************************************************
 FUNCTION	: LookupProcScope
    
 PURPOSE	: Finds procedure.

 PARAMETERS	: pszScopeName	- Name of the procedure to search.			  
			  pszSourceFile	- Source file where search is required.
			  uSourceLine - Source line where searche is required.
			  ppvHandle - Pointer to memory to contain procedure definition 
			  handle.

 RETURNS	: IMG_TRUE - If procedure definition is found.
				ppvHandle - Will contain procedure definition handle.
			  IMG_FALSE - No specified procedure definition found.
*****************************************************************************/
{	
	PSCOPE psParentScopesIterator = NULL;
	PSCOPE psScopeFound;
	for(psParentScopesIterator = g_psCurrentScope;
		psParentScopesIterator->psParentScope != NULL;
		psParentScopesIterator = psParentScopesIterator->psParentScope);
	if(LookupChildSiblingScopes(psParentScopesIterator, pszScopeName, 
		(IMG_HVOID)(&psScopeFound)) == IMG_TRUE)
	{
		if(psScopeFound->bProc == IMG_FALSE)
		{
			ParseError(
				"Refering register of a non procedure global scope '%s'", 
				psScopeFound->pszScopeName);
		}
		*ppvHandle = (IMG_PVOID)(psScopeFound);
		return IMG_TRUE;
	}
	else
	{
		PSCOPE psLastUndefinedMovDest = g_psCurrentScope;
		PSCOPE psUndefinedMovDestsItrator = 
			g_psCurrentScope->psUndefinedMovDests;
		for(;
			psUndefinedMovDestsItrator != NULL;
			psUndefinedMovDestsItrator = 
			psUndefinedMovDestsItrator->psUndefinedMovDests)
		 {
			if(strcmp(psUndefinedMovDestsItrator->pszScopeName, pszScopeName) 
				== 0)
			{
				*ppvHandle =  (IMG_PVOID)(psUndefinedMovDestsItrator);
				return IMG_FALSE;
			}
			psLastUndefinedMovDest = psUndefinedMovDestsItrator;
		}
		{
			PSCOPE psNewScope = NULL;
			psNewScope = (PSCOPE)UseAsm_Malloc( sizeof( SCOPE ) );
			TestMemoryAllocation(psNewScope);			
			psNewScope->pszScopeName = StrdupOrNull(pszScopeName);
			psNewScope->pszSourceFile = StrdupOrNull(pszSourceFile);
			psNewScope->uSourceLine = uSourceLine;
			psNewScope->uLinkToModifiableLabelsTable = 
				GetNextFreeModifiableLabelsTableEntry();
			SetModifiableLabelsTableEntryValue(
				psNewScope->uLinkToModifiableLabelsTable, IMG_UINT32_MAX);
			psNewScope->psUndefinedTempRegs = NULL;
			psNewScope->bProc = IMG_TRUE;				
			psNewScope->bImported = IMG_FALSE;
			psNewScope->bExport = IMG_FALSE;
			{
				psLastUndefinedMovDest->psUndefinedMovDests = 
					psNewScope;
				psNewScope->psUndefinedMovDests = NULL;
			}				
			*ppvHandle =  (IMG_PVOID)(psNewScope);
			return IMG_FALSE;
		}
	}
}

IMG_INTERNAL IMG_VOID AddUndefinedNamedRegisterInScope(IMG_PVOID pvScopeHandle, 
													   const IMG_CHAR* pszName, 
													   const IMG_CHAR* pszSourceFile, 
													   IMG_UINT32 uSourceLine, 
													   IMG_PUINT32 puNamedRegLink)
/*****************************************************************************
 FUNCTION	: AddUndefinedNamedRegisterInScope
    
 PURPOSE	: Adds undefined named register in specified scope.

 PARAMETERS	: pvScopeHandle	- Handle of scope in which to add definition.
			  pszName	- Name of the named register.
			  pszSourceFile	- Source file where named register reference is 
							  found.
			  uSourceLine	- Source line where where named register reference 
							  is found.
			  puNamedRegLink	- Pointer to memory to store named register 
								  definition link.

 RETURNS	: Nothing.
				puNamedRegLink	- Will contain named register definition link.
*****************************************************************************/
{
	PTEMP_REG psUndefinedTempRegsItrator = 
		((PSCOPE)(pvScopeHandle))->psUndefinedTempRegs;
	PTEMP_REG psLastUndefinedTempReg = psUndefinedTempRegsItrator;
	for(;
		psUndefinedTempRegsItrator != NULL;
		psUndefinedTempRegsItrator = psUndefinedTempRegsItrator->psNext)
	{		
		psLastUndefinedTempReg = psUndefinedTempRegsItrator;
	}
	{
		PTEMP_REG	psNewTempReg;
		psNewTempReg = (PTEMP_REG)UseAsm_Malloc( sizeof( TEMP_REG ) );
		TestMemoryAllocation(psNewTempReg);
		psNewTempReg->psNext = NULL;
		psNewTempReg->pszName = StrdupOrNull(pszName);
		psNewTempReg->pszSourceFile = StrdupOrNull(pszSourceFile);
		psNewTempReg->uSourceLine = uSourceLine;
		psNewTempReg->uArrayElementsCount = 0;
		psNewTempReg->uHwRegNum = 0;
		psNewTempReg->uLinkToModableNamdRegsTable = 
			GetNextFreeModifiableNamedRegsTableEntry();
		SetModifiableNamedRegsTableEntryValue(
			psNewTempReg->uLinkToModableNamdRegsTable, psNewTempReg);
		psNewTempReg->psNext = NULL;
		if(psLastUndefinedTempReg == NULL)
		{
			((PSCOPE)(pvScopeHandle))->psUndefinedTempRegs = psNewTempReg;
		}
		else
		{
			psLastUndefinedTempReg->psNext = psNewTempReg;
		}
		*puNamedRegLink = psNewTempReg->uLinkToModableNamdRegsTable;
	}	
	return;
}

IMG_INTERNAL IMG_VOID AddUndefinedNamedRegsArrayInScope(IMG_PVOID pvScopeHandle, 
														const IMG_CHAR* pszName, 
														IMG_UINT32 uArrayElementsCount, 
														const IMG_CHAR* pszSourceFile, 
														IMG_UINT32 uSourceLine, 
														IMG_PUINT32 puNamedRegLink)
/*****************************************************************************
 FUNCTION	: AddUndefinedNamedRegsArrayInScope
    
 PURPOSE	: Adds undefined named registers array in specified scope.

 PARAMETERS	: pvScopeHandle	- Handle of scope in which to add definition.
			  pszName	- Name of the named registers array.
			  pszSourceFile	- Source file where named registers array's element 
							  reference is found.
			  uSourceLine	- Source line where where named registers array's 
							  element reference is found.
			  puNamedRegLink	- Pointer to memory to store named registers 
								  array definition link.

 RETURNS	: Nothing.
				puNamedRegLink	- Will contain named register array definition 
								  link.
*****************************************************************************/
{
	PTEMP_REG psUndefinedTempRegsItrator = 
		((PSCOPE)(pvScopeHandle))->psUndefinedTempRegs;
	PTEMP_REG psLastUndefinedTempReg = psUndefinedTempRegsItrator;
	for(;
		psUndefinedTempRegsItrator != NULL;
		psUndefinedTempRegsItrator = psUndefinedTempRegsItrator->psNext)
	{		
		psLastUndefinedTempReg = psUndefinedTempRegsItrator;
	}
	{
		PTEMP_REG	psNewTempReg;
		psNewTempReg = (PTEMP_REG)UseAsm_Malloc( sizeof( TEMP_REG ) );
		TestMemoryAllocation(psNewTempReg);
		psNewTempReg->psNext = NULL;
		psNewTempReg->pszName = StrdupOrNull(pszName);
		psNewTempReg->pszSourceFile = StrdupOrNull(pszSourceFile);
		psNewTempReg->uSourceLine = uSourceLine;
		psNewTempReg->uArrayElementsCount = (uArrayElementsCount + 1);
		psNewTempReg->uHwRegNum = 0;
		psNewTempReg->uLinkToModableNamdRegsTable = 
			GetNextFreeModifiableNamedRegsTableEntry();
		SetModifiableNamedRegsTableEntryValue(
			psNewTempReg->uLinkToModableNamdRegsTable, psNewTempReg);
		psNewTempReg->psNext = NULL;
		if(psLastUndefinedTempReg == NULL)
		{
			((PSCOPE)(pvScopeHandle))->psUndefinedTempRegs = psNewTempReg;
		}
		else
		{
			psLastUndefinedTempReg->psNext = psNewTempReg;
		}
		*puNamedRegLink = psNewTempReg->uLinkToModableNamdRegsTable;
	}	
	return;
}

IMG_INTERNAL IMG_BOOL LookupTemporaryRegisterInScope(const IMG_VOID* pvScopeHandle, 
													 const IMG_CHAR* pszName, 
													 IMG_PUINT32 puNamedRegLink)
/*****************************************************************************
 FUNCTION	: LookupTemporaryRegisterInScope
    
 PURPOSE	: Searches temporary register definition in the specified scope.

 PARAMETERS	: pvScope		- Handle of scope to search for named register.
			  pszName		- Name of the register
			  ppvHandle		- Pointer to memory to store register definition 
							  handle.

 RETURNS	: IMG_TRUE	- If the register definition is found.
				puNamedRegLink	- Will contain named register definition link.
			  IMG_FALSE	- If the register definition is not found.
*****************************************************************************/
{	
	const SCOPE* psScopeToSearch = (const SCOPE*)(pvScopeHandle);
	PTEMP_REG psTempRegItrator = psScopeToSearch->psTempRegs;
	for(;
		psTempRegItrator != NULL;
		psTempRegItrator = psTempRegItrator->psNext)
	{		
		if (strcmp((psTempRegItrator->pszName), pszName) == 0)
		{			
			*puNamedRegLink = psTempRegItrator->uLinkToModableNamdRegsTable;
			return IMG_TRUE;
		}		
	}	
	*puNamedRegLink = 0;
	return IMG_FALSE;
}

static IMG_VOID GiveErrorsAndDefineAllCurrentUndefinedBranchDests(IMG_VOID)
/*****************************************************************************
 FUNCTION	: GiveErrorsAndDefineAllCurrentUndefinedBranchDests
    
 PURPOSE	: Gives errors for all left undefined branches and define them to 
			  stop multiple similar error messages.

 PARAMETERS	: None.

 RETURNS	: Nothing.
*****************************************************************************/
{
	PSCOPE psCurrentScope = g_psCurrentScope->psUndefinedBranchDests;
	PSCOPE psNextToCurrentScope = NULL;	
	while(psCurrentScope != NULL)
	{			
		psCurrentScope->uLinkToLabelsTable = FindOrCreateLabel(
			psCurrentScope->pszScopeName, IMG_FALSE, 
			psCurrentScope->pszSourceFile, psCurrentScope->uSourceLine);
		SetModifiableLabelsTableEntryValue(
			psCurrentScope->uLinkToModifiableLabelsTable, 
			psCurrentScope->uLinkToLabelsTable);
		ParseErrorAt(psCurrentScope->pszSourceFile, 
			psCurrentScope->uSourceLine, "Can not branch to '%s', it is not defined", 
			psCurrentScope->pszScopeName);
		psNextToCurrentScope = psCurrentScope->psUndefinedBranchDests;
		DeleteScope(psCurrentScope);
		psCurrentScope = psNextToCurrentScope;	
	}
	g_psCurrentScope->psUndefinedBranchDests = NULL;
}

static IMG_VOID GiveErrorsAndDefineAllCurrentUndefinedProcDests(IMG_VOID)
/*****************************************************************************
 FUNCTION	: GiveErrorsAndDefineAllCurrentUndefinedProcDests
    
 PURPOSE	: Gives errors for all left undefined procedures and define them to 
			  stop multiple similar error messages.

 PARAMETERS	: None.

 RETURNS	: Nothing.
*****************************************************************************/
{
	PSCOPE psCurrentScope = g_psCurrentScope->psUndefinedProcDests;
	PSCOPE psNextToCurrentScope = NULL;	
	while(psCurrentScope != NULL)
	{			
		psCurrentScope->uLinkToLabelsTable = FindOrCreateLabel(
			psCurrentScope->pszScopeName, IMG_FALSE, 
			psCurrentScope->pszSourceFile, psCurrentScope->uSourceLine);		
		SetModifiableLabelsTableEntryValue(
			psCurrentScope->uLinkToModifiableLabelsTable, 
			psCurrentScope->uLinkToLabelsTable);
		ParseErrorAt(psCurrentScope->pszSourceFile, 
			psCurrentScope->uSourceLine, "Can not call '%s', it is not defined", 
			psCurrentScope->pszScopeName);
		psNextToCurrentScope = psCurrentScope->psUndefinedProcDests;
		UseAsm_Free(psCurrentScope->psCallsRecord);
		psCurrentScope->psCallsRecord = NULL;
		DeleteScope(psCurrentScope);
		psCurrentScope = psNextToCurrentScope;		
	}
	g_psCurrentScope->psUndefinedProcDests = NULL;
}

static IMG_VOID GiveErrorsAndDeleteAllCurrentUndefinedExportDests(IMG_VOID)
/*****************************************************************************
 FUNCTION	: GiveErrorsAndDeleteAllCurrentUndefinedExportDests
    
 PURPOSE	: Gives errors for all left undefined exports and define them to 
			  stop multiple similar error messages.

 PARAMETERS	: None.

 RETURNS	: Nothing.
*****************************************************************************/
{
	PSCOPE psCurrentScope = g_psCurrentScope->psUndefinedExportDests;
	PSCOPE psNextToCurrentScope = NULL;	
	while(psCurrentScope != NULL)
	{	
		ParseErrorAt(psCurrentScope->pszSourceFile, 
			psCurrentScope->uSourceLine, "Can not export '%s', it is not defined", psCurrentScope->pszScopeName);
		psNextToCurrentScope = psCurrentScope->psUndefinedExportDests;
		DeleteScope(psCurrentScope);
		psCurrentScope = psNextToCurrentScope;		
	}
	g_psCurrentScope->psUndefinedExportDests = NULL;
}

static IMG_VOID DeleteAllCurrentUndefinedNamedRegisters(PSCOPE psScope)
/*****************************************************************************
 FUNCTION	: DeleteAllCurrentUndefinedNamedRegisters
    
 PURPOSE	: Gives errors for all left undefined named registers in specified 
			  scope defines them with error register entry and delete the named 
			  register definition.

 PARAMETERS	: psScope	- Pointer to scope whose undefined named registers to 
						  delete.

 RETURNS	: Nothing.
*****************************************************************************/
{
	PTEMP_REG psUndefinedTempRegsItrator = psScope->psUndefinedTempRegs;	
	PTEMP_REG psTempRegToDelete = NULL;
	while(psUndefinedTempRegsItrator != NULL)
	{
		
		if(psUndefinedTempRegsItrator->uArrayElementsCount > 0)
		{
			ParseErrorAt(psUndefinedTempRegsItrator->pszSourceFile, 
				psUndefinedTempRegsItrator->uSourceLine, "Can not access element %u of named registers array '%s' of an undefined scope '%s'", 
				(psUndefinedTempRegsItrator->uArrayElementsCount)+1, 
				psUndefinedTempRegsItrator->pszName, 
				psScope->pszScopeName);
		}
		else		
		{
			ParseErrorAt(psUndefinedTempRegsItrator->pszSourceFile, 
				psUndefinedTempRegsItrator->uSourceLine, "Can not access named register '%s' of an  of an undefined scope '%s'", 
				psUndefinedTempRegsItrator->pszName, psScope->pszScopeName);
		}
		SetModifiableNamedRegsTableEntryValue(
			psUndefinedTempRegsItrator->uLinkToModableNamdRegsTable, 
			GetErrorNamedRegisterEntryValue());		
		{
			psTempRegToDelete = psUndefinedTempRegsItrator;
			psUndefinedTempRegsItrator = psUndefinedTempRegsItrator->psNext;
			DeleteTempReg(psTempRegToDelete);
		}
	}
	psScope->psUndefinedTempRegs = NULL;
	return;
}

static IMG_VOID GiveErrorsAndDeleteAllCurrentUndefinedMovDests(IMG_VOID)
/*****************************************************************************
 FUNCTION	: GiveErrorsAndDeleteAllCurrentUndefinedMovDests
    
 PURPOSE	: Gives errors for all left undefined move destination scopes, 
			  defines them to stop multiple similar error messages.

 PARAMETERS	: None.

 RETURNS	: Nothing.
*****************************************************************************/
{
	PSCOPE psCurrentScope = g_psCurrentScope->psUndefinedMovDests;
	PSCOPE psNextToCurrentScope = NULL;	
	while(psCurrentScope != NULL)
	{			
		DeleteAllCurrentUndefinedNamedRegisters(psCurrentScope);
		psNextToCurrentScope = psCurrentScope->psUndefinedMovDests;
		DeleteScope(psCurrentScope);
		psCurrentScope = psNextToCurrentScope;		
	}
	g_psCurrentScope->psUndefinedExportDests = NULL;
}

static IMG_VOID DeleteAllCurrentUndefinedNamedRegsOfScope(PSCOPE psScope)
/*****************************************************************************
 FUNCTION	: DeleteAllCurrentUndefinedNamedRegsOfScope
    
 PURPOSE	: Gives errors for all left undefined named registers in specified 
			  scope defines them with error register entry and delete the named 
			  register definition.

 PARAMETERS	: psScope	- Pointer to scope whose undefined named registers to 
						  delete.

 RETURNS	: Nothing.
*****************************************************************************/
{
	PTEMP_REG psUndefinedTempRegsItrator = psScope->psUndefinedTempRegs;	
	PTEMP_REG psTempRegToDelete = NULL;
	while(psUndefinedTempRegsItrator != NULL)
	{
		
		if(psUndefinedTempRegsItrator->uArrayElementsCount > 0)
		{
			ParseErrorAt(psUndefinedTempRegsItrator->pszSourceFile, 
				psUndefinedTempRegsItrator->uSourceLine, "No parameter array '%s' is defined in scope '%s'", 
				psUndefinedTempRegsItrator->pszName, 
				psScope->pszScopeName);
		}
		else
		{
			ParseErrorAt(psUndefinedTempRegsItrator->pszSourceFile, 
				psUndefinedTempRegsItrator->uSourceLine, "No parameter '%s' is defined in scope '%s'", 
				psUndefinedTempRegsItrator->pszName, psScope->pszScopeName);
		}
		SetModifiableNamedRegsTableEntryValue(
			psUndefinedTempRegsItrator->uLinkToModableNamdRegsTable, 
			GetErrorNamedRegisterEntryValue());
		{
			psTempRegToDelete = psUndefinedTempRegsItrator;
			psUndefinedTempRegsItrator = psUndefinedTempRegsItrator->psNext;
			DeleteTempReg(psTempRegToDelete);
		}
	}
	psScope->psUndefinedTempRegs = NULL;
	return;
}

static IMG_VOID GiveErrorsAndDeleteAllCurrentUndefinedRegsFromGlobals(IMG_VOID)
/*****************************************************************************
 FUNCTION	: GiveErrorsAndDeleteAllCurrentUndefinedRegsFromGlobals
    
 PURPOSE	: Gives errors for all left undefined named registers in all global 
			  scopes, defines them with error register entry and delete the 
			  named register definition.

 PARAMETERS	: None.

 RETURNS	: Nothing.
*****************************************************************************/
{
	PSCOPE psCurrentScope = g_psCurrentScope->psChildScopes;	
	while(psCurrentScope != NULL)
	{			
		DeleteAllCurrentUndefinedNamedRegsOfScope(psCurrentScope);		
		psCurrentScope = psCurrentScope->psNextSiblingScope;		
	}
}

IMG_INTERNAL IMG_VOID VerifyThatAllScopesAreEnded(IMG_VOID)
/*****************************************************************************
 FUNCTION	: VerifyThatAllScopesAreEnded
    
 PURPOSE	: Verifies that all scopes are ended properly with closing bracket.
 
 PARAMETERS	: None.
			  
 RETURNS	: Nothing.
*****************************************************************************/
{	
	PSCOPE psParentScopesIterator = g_psCurrentScope;	
	for(;
		psParentScopesIterator->psParentScope != NULL;
		psParentScopesIterator = psParentScopesIterator->psParentScope)
	{
		if(psParentScopesIterator->bDummyScope == IMG_FALSE)
		{
			if(psParentScopesIterator->pszScopeName != NULL)
			{
				ParseErrorAt(psParentScopesIterator->pszSourceFile, 
					psParentScopesIterator->uSourceLine, 
					"Scope '%s' does not have an closing bracket", 
					psParentScopesIterator->pszScopeName);
			}
			else
			{
				ParseErrorAt(psParentScopesIterator->pszSourceFile, 
					psParentScopesIterator->uSourceLine, 
					"Scope does not have an closing bracket");
			}
		}
		MovUndefinedChildsFromChildToParent(
			psParentScopesIterator->psParentScope, psParentScopesIterator);		
	}
	g_psCurrentScope = psParentScopesIterator;
	{
		GiveErrorsAndDefineAllCurrentUndefinedBranchDests();
		GiveErrorsAndDefineAllCurrentUndefinedProcDests();
		GiveErrorsAndDeleteAllCurrentUndefinedExportDests();
		GiveErrorsAndDeleteAllCurrentUndefinedRegsFromGlobals();
		GiveErrorsAndDeleteAllCurrentUndefinedMovDests();
	}
	return;
}

IMG_INTERNAL IMG_BOOL IsCurrentScopeGlobal(IMG_VOID)
/*****************************************************************************
 FUNCTION	: IsCurrentScopeGlobal
    
 PURPOSE	: To test that whether current scope is global or not.
 
 PARAMETERS	: None.
			  
 RETURNS	: IMG_TRUE - If current scope is global.
			  IMG_FALSE - Otherwise.
*****************************************************************************/
{	
	if(g_psCurrentScope->psParentScope != NULL)
	{
		return IMG_FALSE;	
	}
	else
	{
		return IMG_TRUE;
	}	
}

/* To store hardware registers allocation range start allocated to named 
registers */
static IMG_UINT32 g_uHwRegsAllocRangeStart = 1;
/* To store hardware registers allocation range end allocated to named 
registers */
static IMG_UINT32 g_uHwRegsAllocRangeEnd = 0;

static IMG_VOID AllocateHwRegsToAllScopes(PSCOPE psTopLevelScope, 
										  IMG_UINT32 uHwRegsRangeStart, 
										  IMG_UINT32 uHwRegsRangeEnd)
/*****************************************************************************
 FUNCTION	: AllocateHwRegsToAllScopes
    
 PURPOSE	: Allocates hardware registers to named temporary registers in 
			  all scopes.
 
 PARAMETERS	: psTopLevelScope	- Top level scope to start register 
								  allocation.
			  uHwRegsRangeStart	- Starting range of allowed hardware registers.
			  uHwRegsRangeEnd	- Ending range of allowed hardware registers.
			  
 RETURNS	: IMG_TRUE	- If current scope is global.
			  IMG_FALSE	- Otherwise.
*****************************************************************************/
{
	PTEMP_REG psTempRegItrator = psTopLevelScope->psTempRegs;
	IMG_UINT32 uHwRegsRangeStartIntern = uHwRegsRangeStart;
	for(;
		psTempRegItrator != NULL;
		psTempRegItrator = psTempRegItrator->psNext)
	{	
		psTempRegItrator->uHwRegNum = uHwRegsRangeStartIntern;
		if(psTempRegItrator->uArrayElementsCount != 0)
		{
			uHwRegsRangeStartIntern += (psTempRegItrator->uArrayElementsCount);			
		}
		else
		{
			uHwRegsRangeStartIntern++;
		}
		if((uHwRegsRangeStartIntern - 1) > uHwRegsRangeEnd)
		{
			if((uHwRegsRangeStartIntern - 1) > g_uHwRegsAllocRangeEnd)
			{
				g_uHwRegsAllocRangeEnd = uHwRegsRangeStartIntern - 1;
			}
			ParseError("Not enough hardware registes are available to allocate to named temporary registers");			
			return;
		}
	}
	if(
		(psTopLevelScope->psTempRegs != NULL) && 
		((uHwRegsRangeStartIntern - 1) > g_uHwRegsAllocRangeEnd)
	)
	{
		g_uHwRegsAllocRangeEnd = uHwRegsRangeStartIntern - 1;
	}
	{
		PSCOPES_LIST psChildrenScopesIterator;
		for(psChildrenScopesIterator = 
			psTopLevelScope->psNamedRegsAllocChildren;
			psChildrenScopesIterator != NULL;
			psChildrenScopesIterator = 
				psChildrenScopesIterator->psNext)
		{
			AllocateHwRegsToAllScopes(psChildrenScopesIterator->psScope, 
				uHwRegsRangeStart+(psChildrenScopesIterator->uNamedRegsCount), 
				uHwRegsRangeEnd);
		}	
	}	
	return;
}

static IMG_VOID DeleteScopesManagementData(IMG_VOID);

static IMG_UINT32 GetHwRegsAllocRangeStart(IMG_VOID);

static IMG_UINT32 GetHwRegsAllocRangeEnd(IMG_VOID);

IMG_INTERNAL IMG_VOID AllocHwRegsToNamdTempRegsAndSetLabelRefs(PUSE_INST psInst)
/*****************************************************************************
 FUNCTION	: AllocHwRegsToNamdTempRegsAndSetLabelRefs
    
 PURPOSE	: Allocates hardware registers to named temporary registers and 
			  set all labels references.
 
 PARAMETERS	: psInst	- Pointer to instruction defintion.
			  
 RETURNS	: IMG_TRUE	- If current scope is global.
			  IMG_FALSE	- Otherwise.
*****************************************************************************/
{	
	if(IsHwRegsAllocRangeSet() == IMG_TRUE)
	{
		if(g_bNamedRegsUsed == IMG_TRUE)
		{
			g_uHwRegsAllocRangeStart = GetHwRegsAllocRangeStart();
			AllocateHwRegsToAllScopes(g_psCurrentScope, 
				GetHwRegsAllocRangeStart(), GetHwRegsAllocRangeEnd());
		}
	}
	else
	{
		if(g_bNamedRegsUsed == IMG_TRUE)
		{
			ParseError("No range specified for h/w register allocation");
		}
	}
	for (; psInst != NULL; psInst = psInst->psNext)
	{
		IMG_UINT32 uInstArgsCount = OpcodeArgumentCount(psInst->uOpcode);
		IMG_UINT32 uArgItrator = 0;
		for(; uArgItrator < uInstArgsCount; 
			uArgItrator++)
		{
			if((psInst->asArg[uArgItrator].uType) == USEASM_REGTYPE_TEMP_NAMED)
			{
				psInst->asArg[uArgItrator].uType = USEASM_REGTYPE_TEMP;
				if((psInst->asArg[uArgItrator].uNumber) != 
					IMG_UINT32_MAX)
				{
					psInst->asArg[uArgItrator].uNumber = 
						GetHwRegForNamedRegArrayElement(
							psInst->asArg[uArgItrator].uNamedRegLink, 
							psInst->asArg[uArgItrator].uNumber);					
				}
				else
				{					
					psInst->asArg[uArgItrator].uNumber = 
						GetHwRegForNamedReg(
							psInst->asArg[uArgItrator].uNamedRegLink);
				}
			}
			else if((psInst->asArg[uArgItrator].uType) == USEASM_REGTYPE_LABEL)
			{
				psInst->asArg[uArgItrator].uNumber = 
					GetModifiableLabelsTableEntryValue(
						psInst->asArg[uArgItrator].uNumber);
			}
		}		
	}
	DeleteScopesManagementData();
	return;
}

IMG_INTERNAL IMG_UINT32 GetHwRegsAllocRangeStartForBinary(IMG_VOID)
/*****************************************************************************
 FUNCTION	: GetHwRegsAllocRangeStartForBinary
    
 PURPOSE	: Gives hardware registers allocation range start allocated to 
			  named registers for binary output.
 
 PARAMETERS	: None.
			  
 RETURNS	: Hardware registers allocation range start allocated to named 
			  registers.
*****************************************************************************/
{
	return g_uHwRegsAllocRangeStart;
}

IMG_INTERNAL IMG_UINT32 GetHwRegsAllocRangeEndForBinary(IMG_VOID)
/*****************************************************************************
 FUNCTION	: GetHwRegsAllocRangeEndForBinary
    
 PURPOSE	: Gives hardware registers allocation range end allocated to named 
			  registers for binary output.
 
 PARAMETERS	: None.
			  
 RETURNS	: Hardware registers allocation range end allocated to named 
			  registers.
*****************************************************************************/
{
	return g_uHwRegsAllocRangeEnd;
}

IMG_INTERNAL IMG_PCHAR GetScopeOrLabelName(const IMG_CHAR* pszName)
/*****************************************************************************
 FUNCTION	: GetScopeOrLabelName
    
 PURPOSE	: Transforms scope or label name followed by : in to a NULL 
			  terminated character string with null at first : character, 
			  also allocates new memory for the new string.
 
 PARAMETERS	: pszName	- Original name of the scope of label with :s present
						  in it.

 RETURNS	: Pointer to character string with NULL at first : character.
*****************************************************************************/
{
	IMG_PCHAR pszColon;
	IMG_PCHAR pszNameIntern = StrdupOrNull(pszName);
	if ((pszColon = strchr(pszNameIntern, ':')) != NULL)
	{
		*pszColon = 0;
	}
	return pszNameIntern;
}

IMG_INTERNAL IMG_VOID DeleteScopeOrLabelName(IMG_CHAR** ppszName)
/*****************************************************************************
 FUNCTION	: GetScopeOrLabelName
    
 PURPOSE	: Frees dynamically allocated memory given to Scope or Label name.
 
 PARAMETERS	: ppszName	- Pointer to pointer to NULL terminated character 
						  string containing name in it.

 RETURNS	: None.
*****************************************************************************/
{
	UseAsm_Free(*ppszName);
	*ppszName = NULL;
	return;
}

/* Scope management support routines */
static PSCOPE GetUdefProcScopWithName(const IMG_CHAR* pszScopeName)
/*****************************************************************************
 FUNCTION	: GetUdefProcScopWithName
    
 PURPOSE	: Gets specified undefined procedure from the current scope.
 
 PARAMETERS	: pszScopeName	- Name of the undefined scope to search.

 RETURNS	: Pointer to undefined procedure found in the scope.
			  NULL	- If no specified undefined procedure is found.
*****************************************************************************/
{
	PSCOPE psPrevUdefChildScop = g_psCurrentScope;		
	if(g_psCurrentScope->psUndefinedProcDests != NULL)
	{
		PSCOPE psUndefdChildScopsItrator = 
			g_psCurrentScope->psUndefinedProcDests;			
		for(;
			psUndefdChildScopsItrator != NULL;
			psUndefdChildScopsItrator = 
				psUndefdChildScopsItrator->psUndefinedProcDests
		)
		{
			if(strcmp(psUndefdChildScopsItrator->pszScopeName, 
				pszScopeName) == 0)
			{				
				psPrevUdefChildScop->psUndefinedProcDests = 
				psUndefdChildScopsItrator->psUndefinedProcDests;
				psUndefdChildScopsItrator->psUndefinedProcDests = NULL;
				return psUndefdChildScopsItrator;	
			}
			psPrevUdefChildScop = psUndefdChildScopsItrator;
		}		
		return NULL;
	}
	else
	{		
		return NULL;
	}
}

static PSCOPE GetUdefExportScopWithName(const IMG_CHAR* pszScopeName)
/*****************************************************************************
 FUNCTION	: GetUdefExportScopWithName
    
 PURPOSE	: Gets specified undefined export scope from the current scope.
 
 PARAMETERS	: pszScopeName	- Name of the undefined scope to search.

 RETURNS	: Pointer to undefined export scope found in the scope.
			  NULL	- If no specified undefined export procedure is found.
*****************************************************************************/
{
	PSCOPE psPrevUdefChildScop = g_psCurrentScope;		
	if(g_psCurrentScope->psUndefinedExportDests != NULL)
	{
		PSCOPE psUndefdChildScopsItrator = 
			g_psCurrentScope->psUndefinedExportDests;			
		for(;
			psUndefdChildScopsItrator != NULL;
			psUndefdChildScopsItrator = 
				psUndefdChildScopsItrator->psUndefinedExportDests
		)
		{
			if(strcmp(psUndefdChildScopsItrator->pszScopeName, 
				pszScopeName) == 0)
			{
				psPrevUdefChildScop->psUndefinedExportDests = 
				psUndefdChildScopsItrator->psUndefinedExportDests;
				psUndefdChildScopsItrator->psUndefinedExportDests = NULL;
				return psUndefdChildScopsItrator;				
			}
			psPrevUdefChildScop = psUndefdChildScopsItrator;
		}		
		return NULL;
	}
	else
	{		
		return NULL;
	}
}


static PSCOPE GetUdefBranchScopWithName(const IMG_CHAR* pszScopeName)
/*****************************************************************************
 FUNCTION	: GetUdefBranchScopWithName
    
 PURPOSE	: Gets specified undefined branch destination scope from the 
			  current scope.
 
 PARAMETERS	: pszScopeName	- Name of the undefined branch destination scope to 
							  search.

 RETURNS	: Pointer to undefined branch destination scope found in the scope.
			  NULL	- If no specified undefined branch destination procedure is 
					  found.
*****************************************************************************/
{
	PSCOPE psPrevUdefChildScop = g_psCurrentScope;		
	if(g_psCurrentScope->psUndefinedBranchDests != NULL)
	{
		PSCOPE psUndefdChildScopsItrator = 
			g_psCurrentScope->psUndefinedBranchDests;			
		for(;
			psUndefdChildScopsItrator != NULL;
			psUndefdChildScopsItrator = 
				psUndefdChildScopsItrator->psUndefinedBranchDests
		)
		{
			if(strcmp(psUndefdChildScopsItrator->pszScopeName, 
				pszScopeName) == 0)
			{
				psPrevUdefChildScop->psUndefinedBranchDests = 
				psUndefdChildScopsItrator->psUndefinedBranchDests;
				psUndefdChildScopsItrator->psUndefinedBranchDests = NULL;
				return psUndefdChildScopsItrator;										
			}
			psPrevUdefChildScop = psUndefdChildScopsItrator;
		}		
		return NULL;
	}
	else
	{		
		return NULL;
	}
}

static PSCOPE GetUdefMovScopWithName(const IMG_CHAR* pszScopeName)
/*****************************************************************************
 FUNCTION	: GetUdefMovScopWithName
    
 PURPOSE	: Gets specified undefined mov destination scope from the 
			  current scope.
 
 PARAMETERS	: pszScopeName	- Name of the undefined mov destination scope to 
							  search.

 RETURNS	: Pointer to undefined mov destination scope found in the scope.
			  NULL	- If no specified undefined mov destination procedure is 
					  found.
*****************************************************************************/
{
	PSCOPE psPrevUdefChildScop = g_psCurrentScope;		
	if(g_psCurrentScope->psUndefinedMovDests != NULL)
	{
		PSCOPE psUndefdChildScopsItrator = 
			g_psCurrentScope->psUndefinedMovDests;			
		for(;
			psUndefdChildScopsItrator != NULL;
			psUndefdChildScopsItrator = 
				psUndefdChildScopsItrator->psUndefinedMovDests
		)
		{
			if(strcmp(psUndefdChildScopsItrator->pszScopeName, 
				pszScopeName) == 0)
			{				
				psPrevUdefChildScop->psUndefinedMovDests = 
				psUndefdChildScopsItrator->psUndefinedMovDests;
				psUndefdChildScopsItrator->psUndefinedMovDests = NULL;
				return psUndefdChildScopsItrator;	
			}
			psPrevUdefChildScop = psUndefdChildScopsItrator;
		}		
		return NULL;
	}
	else
	{		
		return NULL;
	}
}

static PSCOPE CreateProcScope(const IMG_CHAR* pszScopeName, 
							  const IMG_CHAR* pszSourceFile, 
							  IMG_UINT32 uSourceLine)
/*****************************************************************************
 FUNCTION	: CreateProcScope
    
 PURPOSE	: Creats a new procedure scope.
 
 PARAMETERS	: pszScopeName	- Name of the scope.
			  pszSourceFile	- Source file of the scope.
			  uSourceLine	- Source line of the scope.

 RETURNS	: Pointer to newly created scope.
*****************************************************************************/
{
	PSCOPE psNewScope = NULL;	
	psNewScope = (PSCOPE)UseAsm_Malloc( sizeof( SCOPE ) );
	TestMemoryAllocation(psNewScope);
	psNewScope->pszScopeName = StrdupOrNull(pszScopeName);
	psNewScope->pszSourceFile = StrdupOrNull(pszSourceFile);
	psNewScope->uSourceLine = uSourceLine;
	psNewScope->uLinkToModifiableLabelsTable = 
		GetNextFreeModifiableLabelsTableEntry();	
	psNewScope->uLinkToLabelsTable = FindOrCreateLabel(pszScopeName, IMG_TRUE, 
		pszSourceFile, uSourceLine);
	SetModifiableLabelsTableEntryValue(
		psNewScope->uLinkToModifiableLabelsTable, 
		psNewScope->uLinkToLabelsTable);
	psNewScope->bExport = IMG_FALSE;
	psNewScope->bProc = IMG_TRUE;
	psNewScope->bImported = IMG_FALSE;
	psNewScope->psUndefinedTempRegs = NULL;
	psNewScope->psCalleesRecord = NULL;
	psNewScope->psCallsRecord = NULL;
	psNewScope->uMaxNamedRegsTransByCall = 0;
	psNewScope->psNamedRegsAllocParent = NULL;
	psNewScope->psNamedRegsAllocChildren = NULL;
	psNewScope->uCurrentCountOfNamedRegsInScope = 0;
	return psNewScope;
}

static PSCOPE CreateBranchScope(const IMG_CHAR* pszScopeName, 
								const IMG_CHAR* pszSourceFile, 
								IMG_UINT32 uSourceLine)
/*****************************************************************************
 FUNCTION	: CreateBranchScope
    
 PURPOSE	: Creats a new branch scope.
 
 PARAMETERS	: pszScopeName	- Name of the scope.
			  pszSourceFile	- Source file of the scope.
			  uSourceLine	- Source line of the scope.

 RETURNS	: Pointer to newly created scope.
*****************************************************************************/
{
	PSCOPE psNewScope = NULL;
	const IMG_CHAR* pszScopeNameForLabel = pszScopeName;
	IMG_CHAR pszLabelInternName[11];
	if(IsCurrentScopeGlobal() == IMG_FALSE)
	{	
		GetNextNumericLabelName(pszLabelInternName);
		pszScopeNameForLabel = pszLabelInternName;
	}	
	psNewScope = (PSCOPE)UseAsm_Malloc( sizeof( SCOPE ) );
	TestMemoryAllocation(psNewScope);
	psNewScope->pszScopeName = StrdupOrNull(pszScopeName);
	psNewScope->pszSourceFile = StrdupOrNull(pszSourceFile);
	psNewScope->uSourceLine = uSourceLine;
		
	psNewScope->uLinkToModifiableLabelsTable = 
		GetNextFreeModifiableLabelsTableEntry();	
	psNewScope->uLinkToLabelsTable = FindOrCreateLabel(pszScopeNameForLabel, 
		IMG_TRUE, pszSourceFile, uSourceLine);
	SetModifiableLabelsTableEntryValue(
		psNewScope->uLinkToModifiableLabelsTable, 
		psNewScope->uLinkToLabelsTable);
	psNewScope->bExport = IMG_FALSE;
	psNewScope->bProc = IMG_FALSE;
	psNewScope->bImported = IMG_FALSE;
	psNewScope->psUndefinedTempRegs = NULL;
	psNewScope->psCalleesRecord = NULL;
	psNewScope->psCallsRecord = NULL;
	psNewScope->uMaxNamedRegsTransByCall = 0;
	psNewScope->psNamedRegsAllocParent = NULL;
	psNewScope->psNamedRegsAllocChildren = NULL;
	psNewScope->uCurrentCountOfNamedRegsInScope = 0;
	return psNewScope;
}

static PSCOPE CreateExportScope(const IMG_CHAR* pszScopeName, 
								const IMG_CHAR* pszSourceFile, 
								IMG_UINT32 uSourceLine)
/*****************************************************************************
 FUNCTION	: CreateExportScope
    
 PURPOSE	: Creats a new export scope.
 
 PARAMETERS	: pszScopeName	- Name of the scope.
			  pszSourceFile	- Source file of the scope.
			  uSourceLine	- Source line of the scope.

 RETURNS	: Pointer to newly created scope.
*****************************************************************************/
{
	PSCOPE psNewScope = NULL;	
	psNewScope = (PSCOPE)UseAsm_Malloc( sizeof( SCOPE ) );
	TestMemoryAllocation(psNewScope);
	psNewScope->pszScopeName = StrdupOrNull(pszScopeName);
	psNewScope->pszSourceFile = StrdupOrNull(pszSourceFile);
	psNewScope->uSourceLine = uSourceLine;		
	psNewScope->uLinkToModifiableLabelsTable = 
		GetNextFreeModifiableLabelsTableEntry();	
	psNewScope->uLinkToLabelsTable = FindOrCreateLabel(pszScopeName, IMG_FALSE, 
		pszSourceFile, uSourceLine);
	SetLabelExportImportState(psNewScope->uLinkToLabelsTable, pszSourceFile, 
		uSourceLine, IMG_FALSE, IMG_TRUE);
	SetModifiableLabelsTableEntryValue(
		psNewScope->uLinkToModifiableLabelsTable, 
		psNewScope->uLinkToLabelsTable);
	psNewScope->bExport = IMG_TRUE;
	psNewScope->bProc = IMG_FALSE;
	psNewScope->bImported = IMG_FALSE;
	psNewScope->psUndefinedTempRegs = NULL;
	psNewScope->psCalleesRecord = NULL;
	psNewScope->psCallsRecord = NULL;
	psNewScope->uMaxNamedRegsTransByCall = 0;
	psNewScope->psNamedRegsAllocParent = NULL;
	psNewScope->psNamedRegsAllocChildren = NULL;
	psNewScope->uCurrentCountOfNamedRegsInScope = 0;
	return psNewScope;
}

static PSCOPE  CreateImportedScope(const IMG_CHAR* pszScopeName, 
								   const IMG_CHAR* pszSourceFile, 
								   IMG_UINT32 uSourceLine)
/*****************************************************************************
 FUNCTION	: CreateImportScope
    
 PURPOSE	: Creats a new imported scope.
 
 PARAMETERS	: pszScopeName	- Name of the scope.
			  pszSourceFile	- Source file of the scope.
			  uSourceLine	- Source line of the scope.

 RETURNS	: Pointer to newly created scope.
*****************************************************************************/
{
	PSCOPE psNewScope = NULL;	
	psNewScope = (PSCOPE)UseAsm_Malloc( sizeof( SCOPE ) );
	TestMemoryAllocation(psNewScope);
	psNewScope->pszScopeName = StrdupOrNull(pszScopeName);
	psNewScope->pszSourceFile = StrdupOrNull(pszSourceFile);
	psNewScope->uSourceLine = uSourceLine;		
	psNewScope->uLinkToModifiableLabelsTable = 
		GetNextFreeModifiableLabelsTableEntry();	
	psNewScope->uLinkToLabelsTable = FindOrCreateLabel(pszScopeName, IMG_FALSE, 
		pszSourceFile, uSourceLine);
	SetLabelExportImportState(psNewScope->uLinkToLabelsTable, pszSourceFile, 
		uSourceLine, IMG_TRUE, IMG_FALSE);
	SetModifiableLabelsTableEntryValue(
		psNewScope->uLinkToModifiableLabelsTable, 
		psNewScope->uLinkToLabelsTable);
	psNewScope->bExport = IMG_FALSE;
	psNewScope->bProc = IMG_TRUE;
	psNewScope->bImported = IMG_TRUE;
	psNewScope->psUndefinedTempRegs = NULL;
	psNewScope->psCalleesRecord = NULL;
	psNewScope->psCallsRecord = NULL;
	psNewScope->uMaxNamedRegsTransByCall = 0;
	psNewScope->psNamedRegsAllocParent = NULL;
	psNewScope->psNamedRegsAllocChildren = NULL;
	psNewScope->uCurrentCountOfNamedRegsInScope = 0;
	return psNewScope;
}

static IMG_VOID DeleteScope(PSCOPE psScope)
/*****************************************************************************
 FUNCTION	: DeleteScope
    
 PURPOSE	: Frees memory allocated to specified scope definition.
 
 PARAMETERS	: psScope	- Pointer to scope to delete.
			  
 RETURNS	: Nothing.
*****************************************************************************/
{
	if(psScope->pszScopeName != NULL)
	{
		UseAsm_Free(psScope->pszScopeName);
		psScope->pszScopeName = NULL;
	}
	UseAsm_Free(psScope->pszSourceFile);
	UseAsm_Free(psScope);
}

static PSCOPE DefineAllUndefinedBranchDests(PSCOPE psScope, 
											const IMG_CHAR* pszScopeName, 
											const IMG_CHAR* pszSourceFile, 
											IMG_UINT32 uSourceLine)
/*****************************************************************************
 FUNCTION	: DefineAllUndefinedBranchDests
    
 PURPOSE	: Defines all specified current undefined branch destinations.
 
 PARAMETERS	: psScope	- Pointer to scope to use for definition.
			  pszScopeName	- Name of scope to create, If psScope is NULL.
			  pszSourceFile	- Name of source file in which scope is opened, If 
							  psScope is NULL.
			  uSourceLine	- Source line at which scope is opened, If psScope 
							  is NULL.
			  
 RETURNS	: psScope - If psScope was provided.
			  Pointer to newly created scope. If psScope was NULL.
*****************************************************************************/
{
	if(psScope == NULL)
	{
		psScope = CreateBranchScope(pszScopeName, pszSourceFile, uSourceLine);	
	}
	{
		PSCOPE psCurrentScope = GetUdefBranchScopWithName(
			psScope->pszScopeName);
		while(psCurrentScope != NULL)
		{
			SetModifiableLabelsTableEntryValue(
				psCurrentScope->uLinkToModifiableLabelsTable, 
				psScope->uLinkToLabelsTable);
			DeleteScope(psCurrentScope);
			psCurrentScope = GetUdefBranchScopWithName(psScope->pszScopeName);
		}
	}
	return psScope;
}

static PSCOPE DefineAllUndefinedExportDests(PSCOPE psScope, 
											const IMG_CHAR* pszScopeName, 
											const IMG_CHAR* pszSourceFile, 
											IMG_UINT32 uSourceLine)
/*****************************************************************************
 FUNCTION	: DefineAllUndefinedExportDests
    
 PURPOSE	: Defines all specified current undefined export destinations.
 
 PARAMETERS	: psScope	- Pointer to scope to use for definition.
			  pszScopeName	- Name of scope to create, If psScope is NULL.
			  pszSourceFile	- Name of source file in which scope is opened, If 
							  psScope is NULL.
			  uSourceLine	- Source line at which scope is opened, If psScope 
							  is NULL.
			  
 RETURNS	: psScope - If psScope was provided.
			  Pointer to newly created scope. If psScope was NULL.
*****************************************************************************/
{
	if(psScope == NULL)
	{
		psScope = CreateExportScope(pszScopeName, pszSourceFile, uSourceLine);	
	}
	{
		PSCOPE psCurrentScope = GetUdefExportScopWithName(
			psScope->pszScopeName);
		IMG_BOOL bAlreadyExported = psScope->bExport;
		while(psCurrentScope != NULL)
		{
			SetModifiableLabelsTableEntryValue(
				psCurrentScope->uLinkToModifiableLabelsTable, 
				psScope->uLinkToLabelsTable);
			psCurrentScope->uLinkToLabelsTable = psScope->uLinkToLabelsTable; 
			if(bAlreadyExported == IMG_FALSE)
			{
				SetLabelExportImportState(psCurrentScope->uLinkToLabelsTable, 
					psCurrentScope->pszSourceFile, psCurrentScope->uSourceLine, 
					IMG_FALSE, IMG_TRUE);
				psScope->bExport = IMG_TRUE;
				bAlreadyExported = psScope->bExport;
			}
			DeleteScope(psCurrentScope);
			psCurrentScope = GetUdefExportScopWithName(psScope->pszScopeName);
		}
	}
	return psScope;
}

static PSCOPE DefineAllUndefinedProcDests(PSCOPE psScope, 
										  const IMG_CHAR* pszScopeName, 
										  const IMG_CHAR* pszSourceFile, 
										  IMG_UINT32 uSourceLine)
/*****************************************************************************
 FUNCTION	: DefineAllUndefinedProcDests
    
 PURPOSE	: Defines all specified current undefined procedure destinations.
 
 PARAMETERS	: psScope	- Pointer to scope to use for definition.
			  pszScopeName	- Name of scope to create, If psScope is NULL.
			  pszSourceFile	- Name of source file in which scope is opened, If 
							  psScope is NULL.
			  uSourceLine	- Source line at which scope is opened, If psScope 
							  is NULL.
			  
 RETURNS	: psScope - If psScope was provided.
			  Pointer to newly created scope. If psScope was NULL.
*****************************************************************************/
{
	if(psScope == NULL)
	{
		psScope = CreateProcScope(pszScopeName, pszSourceFile, uSourceLine);
	}
	{
		PSCOPE psCurrentScope = GetUdefProcScopWithName(psScope->pszScopeName);		
		while(psCurrentScope != NULL)
		{
			SetModifiableLabelsTableEntryValue(
				psCurrentScope->uLinkToModifiableLabelsTable, 
				psScope->uLinkToLabelsTable);
			AddToCallsRecordOfEarlierScope(psCurrentScope, psScope);			
			psCurrentScope->psParentScope = NULL;
			DeleteScope(psCurrentScope);
			psCurrentScope = GetUdefProcScopWithName(psScope->pszScopeName);
		}
	}
	return psScope;
}

static IMG_VOID MovUndefinedNamedRegs(PSCOPE psScopeDest, PSCOPE psScopeSrc)
/*****************************************************************************
 FUNCTION	: MovUndefinedNamedRegs
    
 PURPOSE	: Moves undefined named registers from one scope to another scope.
 
 PARAMETERS	: psScopeDest	- Pointer to destination scope.
			  psScopeSrc	- Pointer to source scope.

 RETURNS	: Nothing.
*****************************************************************************/
{
	if(psScopeDest->psUndefinedTempRegs == NULL)
	{
		psScopeDest->psUndefinedTempRegs = psScopeSrc->psUndefinedTempRegs;		
	}
	else
	{
		PTEMP_REG psUndefinedTempRegsEnd = psScopeDest->psUndefinedTempRegs;
		for(; psUndefinedTempRegsEnd->psNext != NULL; 
			psUndefinedTempRegsEnd = psUndefinedTempRegsEnd->psNext);		
		psUndefinedTempRegsEnd->psNext = psScopeSrc->psUndefinedTempRegs;		
	}
	psScopeSrc->psUndefinedTempRegs = NULL;
}

static PSCOPE DefineAllUndefinedMovDests(PSCOPE psScope, 
										 const IMG_CHAR* pszScopeName, 
										 const IMG_CHAR* pszSourceFile, 
										 IMG_UINT32 uSourceLine)
/*****************************************************************************
 FUNCTION	: DefineAllUndefinedMovDests
    
 PURPOSE	: Defines all specified current undefined mov destinations.
 
 PARAMETERS	: psScope	- Pointer to scope to use for definition.
			  pszScopeName	- Name of scope to create, If psScope is NULL.
			  pszSourceFile	- Name of source file in which scope is opened, If 
							  psScope is NULL.
			  uSourceLine	- Source line at which scope is opened, If psScope 
							  is NULL.
			  
 RETURNS	: psScope - If psScope was provided.
			  Pointer to newly created scope. If psScope was NULL.
*****************************************************************************/
{
	if(psScope == NULL)
	{
		psScope = CreateProcScope(pszScopeName, pszSourceFile, uSourceLine);
	}
	{
		PSCOPE psCurrentScope = GetUdefMovScopWithName(psScope->pszScopeName);		
		while(psCurrentScope != NULL)
		{
			SetModifiableLabelsTableEntryValue(
				psCurrentScope->uLinkToModifiableLabelsTable, 
				psScope->uLinkToLabelsTable);			
			MovUndefinedNamedRegs(psScope, psCurrentScope);
			DeleteScope(psCurrentScope);
			psCurrentScope = GetUdefMovScopWithName(psScope->pszScopeName);
		}
	}	
	return psScope;
}

static IMG_VOID GiveErrorsAndDefineAllUndefinedProcDests(PSCOPE psScope)
/*****************************************************************************
 FUNCTION	: GiveErrorsAndDefineAllUndefinedProcDests
    
 PURPOSE	: Gives errors and defines all specified current undefined 
			  procedure destinations to stop multiple similar error messages.
 
 PARAMETERS	: psScope	- Pointer to scope to use for definition.
			  
 RETURNS	: Nothing.
*****************************************************************************/
{
	if(IsScopeManagementEnabled() == IMG_TRUE)
	{
		PSCOPE psCurrentScope = GetUdefProcScopWithName(psScope->pszScopeName);
		while(psCurrentScope != NULL)
		{
			SetModifiableLabelsTableEntryValue(
				psCurrentScope->uLinkToModifiableLabelsTable, 
				psScope->uLinkToLabelsTable);
			ParseErrorAt(psCurrentScope->pszSourceFile, 
				psCurrentScope->uSourceLine, "'%s' is not a procedure", 
				psCurrentScope->pszScopeName);
			ParseErrorAt(psScope->pszSourceFile, psScope->uSourceLine, 
				"Is the location of conflicting definition");
			AddToCallsRecordOfEarlierScope(psCurrentScope, psScope);		
			psCurrentScope->psParentScope = NULL;
			DeleteScope(psCurrentScope);
			psCurrentScope = GetUdefProcScopWithName(psScope->pszScopeName);
		}
	}
	else
	{
		{
			PSCOPE psCurrentScope = 
				GetUdefProcScopWithName(psScope->pszScopeName);		
			while(psCurrentScope != NULL)
			{
				SetModifiableLabelsTableEntryValue(
					psCurrentScope->uLinkToModifiableLabelsTable, 
					psScope->uLinkToLabelsTable);
				AddToCallsRecordOfEarlierScope(psCurrentScope, psScope);			
				psCurrentScope->psParentScope = NULL;
				DeleteScope(psCurrentScope);
				psCurrentScope = GetUdefProcScopWithName(psScope->pszScopeName);
			}
		}	
	}
}

static IMG_VOID GiveErrorsAndDefineAllUndefinedBranchDests(const SCOPE* psScope)
/*****************************************************************************
 FUNCTION	: GiveErrorsAndDefineAllUndefinedBranchDests
    
 PURPOSE	: Gives errors and defines all specified current undefined 
			  branch destinations to stop multiple similar error messages.
 
 PARAMETERS	: psScope	- Pointer to scope to use for definition.
			  
 RETURNS	: Nothing.
*****************************************************************************/
{
	if(IsScopeManagementEnabled() == IMG_TRUE)
	{
		PSCOPE psCurrentScope = 
			GetUdefBranchScopWithName(psScope->pszScopeName);
		while(psCurrentScope != NULL)
		{
			SetModifiableLabelsTableEntryValue(
				psCurrentScope->uLinkToModifiableLabelsTable, 
				psScope->uLinkToLabelsTable);
			ParseErrorAt(psCurrentScope->pszSourceFile, 
				psCurrentScope->uSourceLine, "Can not branch to procedure '%s'", 
				psCurrentScope->pszScopeName);
			ParseErrorAt(psScope->pszSourceFile, psScope->uSourceLine, 
				"Is the location of conflicting definition");
			DeleteScope(psCurrentScope);
			psCurrentScope = GetUdefBranchScopWithName(psScope->pszScopeName);
		}
	}
	else
	{
		{
			PSCOPE psCurrentScope = GetUdefBranchScopWithName(
				psScope->pszScopeName);
			while(psCurrentScope != NULL)
			{
				SetModifiableLabelsTableEntryValue(
					psCurrentScope->uLinkToModifiableLabelsTable, 
					psScope->uLinkToLabelsTable);
				DeleteScope(psCurrentScope);
				psCurrentScope = GetUdefBranchScopWithName(psScope->pszScopeName);
			}
		}
	}
}

static IMG_VOID GiveErrorsAndDefineAllUndefinedMovDests(PSCOPE psScope)
/*****************************************************************************
 FUNCTION	: GiveErrorsAndDefineAllUndefinedMovDests
    
 PURPOSE	: Gives errors and defines all specified current undefined 
			  mov destinations to stop multiple similar error messages.
 
 PARAMETERS	: psScope	- Pointer to scope to use for definition.
			  
 RETURNS	: Nothing.
*****************************************************************************/
{
	PSCOPE psCurrentScope = GetUdefMovScopWithName(psScope->pszScopeName);
	while(psCurrentScope != NULL)
	{
		SetModifiableLabelsTableEntryValue(
			psCurrentScope->uLinkToModifiableLabelsTable, 
			psScope->uLinkToLabelsTable);
		ParseErrorAt(psCurrentScope->pszSourceFile, 
			psCurrentScope->uSourceLine, "Can not access named registers of a global non procedure '%s'", 
			psCurrentScope->pszScopeName);
		ParseErrorAt(psScope->pszSourceFile, psScope->uSourceLine, 
			"Is the location of conflicting definition");
		MovUndefinedNamedRegs(psScope, psCurrentScope);
		DeleteScope(psCurrentScope);
		psCurrentScope = GetUdefMovScopWithName(psScope->pszScopeName);
	}
}

static IMG_VOID GiveErrorsAndDeleteAllUndefinedExportDests(const SCOPE* psScope)
/*****************************************************************************
 FUNCTION	: GiveErrorsAndDeleteAllUndefinedExportDests
    
 PURPOSE	: Gives errors and deletes all specified current undefined 
			  export destinations to stop multiple similar error messages.
 
 PARAMETERS	: psScope	- Pointer to scope to use for error messages.
			  
 RETURNS	: Nothing.
*****************************************************************************/
{
	PSCOPE psCurrentScope = GetUdefExportScopWithName(psScope->pszScopeName);
	while(psCurrentScope != NULL)
	{
		SetModifiableLabelsTableEntryValue(
			psCurrentScope->uLinkToModifiableLabelsTable, 
			psScope->uLinkToLabelsTable);
		/*ParseErrorAt(psCurrentScope->pszSourceFile, 
			psCurrentScope->uSourceLine, "Can not export '%s' at this point", 
			psCurrentScope->pszScopeName);
		ParseErrorAt(psScope->pszSourceFile, psScope->uSourceLine, "Is the location where %s is imported", 
			psScope->pszScopeName);*/
		DeleteScope(psCurrentScope);
		psCurrentScope = GetUdefExportScopWithName(psScope->pszScopeName);
	}	
}

static IMG_VOID DeleteAllUndefinedNamedRegisters(PSCOPE psScope)
/*****************************************************************************
 FUNCTION	: DeleteAllUndefinedNamedRegisters
    
 PURPOSE	: Gives errors and deletes all current undefined named registers in 
			  a specified scope.
 
 PARAMETERS	: psScope	- Pointer to scope in which to delete undefined named 
						  registers.

 RETURNS	: Nothing.
*****************************************************************************/
{
	PTEMP_REG psUndefinedTempRegsItrator = psScope->psUndefinedTempRegs;	
	PTEMP_REG psTempRegToDelete = NULL;
	while(psUndefinedTempRegsItrator != NULL)
	{
		
		if(psUndefinedTempRegsItrator->uArrayElementsCount > 0)
		{
			ParseErrorAt(psUndefinedTempRegsItrator->pszSourceFile, 
				psUndefinedTempRegsItrator->uSourceLine, "Can not access element %u of named registers array '%s' of an imported global '%s'", 
				(psUndefinedTempRegsItrator->uArrayElementsCount)+1, 
				psUndefinedTempRegsItrator->pszName, 
				psScope->pszScopeName);
		}
		else		
		{
			ParseErrorAt(psUndefinedTempRegsItrator->pszSourceFile, 
				psUndefinedTempRegsItrator->uSourceLine, "Can not access named register '%s' of an imported global '%s'", 
				psUndefinedTempRegsItrator->pszName, psScope->pszScopeName);
		}
		SetModifiableNamedRegsTableEntryValue(
			psUndefinedTempRegsItrator->uLinkToModableNamdRegsTable, 
			GetErrorNamedRegisterEntryValue());
		{
			psTempRegToDelete = psUndefinedTempRegsItrator;
			psUndefinedTempRegsItrator = psUndefinedTempRegsItrator->psNext;
			DeleteTempReg(psTempRegToDelete);
		}
	}
	psScope->psUndefinedTempRegs = NULL;
	return;
}

static IMG_VOID GiveErrorsAndDeleteAllUndefinedMovDests(const SCOPE* psScope)
/*****************************************************************************
 FUNCTION	: GiveErrorsAndDeleteAllUndefinedMovDests
    
 PURPOSE	: Gives errors and deletes all specified current undefined 
			  mov destinations to stop multiple similar error messages.
 
 PARAMETERS	: psScope	- Pointer to scope to use for error messages.
			  
 RETURNS	: Nothing.
*****************************************************************************/
{
	PSCOPE psCurrentScope = GetUdefMovScopWithName(psScope->pszScopeName);
	while(psCurrentScope != NULL)
	{
		SetModifiableLabelsTableEntryValue(
			psCurrentScope->uLinkToModifiableLabelsTable, 
			psScope->uLinkToLabelsTable);
		DeleteAllUndefinedNamedRegisters(psCurrentScope);
		ParseErrorAt(psScope->pszSourceFile, psScope->uSourceLine, "Is the location where %s is imported", 
			psScope->pszScopeName);		
		DeleteScope(psCurrentScope);
		psCurrentScope = GetUdefExportScopWithName(psScope->pszScopeName);
	}	
}

static PSCOPE UseExistingUndefinedScope(const IMG_CHAR* pszScopeName, 
										IMG_BOOL bProc, 
										IMG_BOOL bImported, 
										const IMG_CHAR* pszSourceFile, 
										IMG_UINT32 uSourceLine)
/*****************************************************************************
 FUNCTION	: UseExistingUndefinedScope
    
 PURPOSE	: Gives existing undefined scope reference for a current reference 
			  to same scope.
 
 PARAMETERS	: pszScopeName	- Name of scope to search.
			  bProc	- Tells either the scope to search should be procedure or 
					  not.
			  bImported	- Tells either the scope to search should be imported 
						  or not.
			  pszSourceFile	- Source file in which reference is made.
			  uSourceLine	- Source line at which reference is made.

 RETURNS	: Nothing.
*****************************************************************************/
{
	PSCOPE psScopeToUse = NULL;
	if(IsCurrentScopeGlobal() == IMG_TRUE && bProc == IMG_FALSE && bImported == 
		IMG_FALSE)
	{
		psScopeToUse = DefineAllUndefinedBranchDests(psScopeToUse, 
			pszScopeName, pszSourceFile, uSourceLine);
		psScopeToUse = DefineAllUndefinedExportDests(psScopeToUse, 
			pszScopeName, pszSourceFile, uSourceLine);
		psScopeToUse = DefineAllUndefinedBranchDests(psScopeToUse, 
			pszScopeName, pszSourceFile, uSourceLine);		
		GiveErrorsAndDefineAllUndefinedProcDests(psScopeToUse);
		GiveErrorsAndDefineAllUndefinedMovDests(psScopeToUse);
	}
	else if(bProc == IMG_TRUE  && bImported == IMG_FALSE)
	{
		psScopeToUse = DefineAllUndefinedProcDests(psScopeToUse, pszScopeName, 
			pszSourceFile, uSourceLine);
		psScopeToUse = DefineAllUndefinedExportDests(psScopeToUse, 
			pszScopeName, pszSourceFile, uSourceLine);
		psScopeToUse = DefineAllUndefinedMovDests(psScopeToUse, pszScopeName, 
			pszSourceFile, uSourceLine);
		GiveErrorsAndDefineAllUndefinedBranchDests(psScopeToUse);
	}
	else if(bImported == IMG_FALSE)
	{
		psScopeToUse = DefineAllUndefinedBranchDests(psScopeToUse, 
			pszScopeName, pszSourceFile, uSourceLine);
	}
	else
	{
		psScopeToUse = CreateImportedScope(pszScopeName, pszSourceFile, 
			uSourceLine);
		psScopeToUse = DefineAllUndefinedBranchDests(psScopeToUse, 
			pszScopeName, pszSourceFile, uSourceLine);
		psScopeToUse = DefineAllUndefinedProcDests(psScopeToUse, pszScopeName, 
			pszSourceFile, uSourceLine);
		GiveErrorsAndDeleteAllUndefinedExportDests(psScopeToUse);
		GiveErrorsAndDeleteAllUndefinedMovDests(psScopeToUse);
	}
	if(psScopeToUse != NULL)
	{		
		UseAsm_Free(psScopeToUse->pszSourceFile);
	}
	return psScopeToUse;
}

/* Modifiable labels table, A table in which references to labels can be 
modified later */
static IMG_PUINT32 g_puModifiableLabelsTable;

static IMG_VOID InitModifiableLabelsTable(IMG_VOID)
/*****************************************************************************
 FUNCTION	: InitModifiableLabelsTable
    
 PURPOSE	: Initializes modifiable labels table.
 
 PARAMETERS	: None.

 RETURNS	: Nothing.
*****************************************************************************/
{	
	g_puModifiableLabelsTable = NULL;
}

static IMG_VOID DeleteModifiableLabelsTable(IMG_VOID)
/*****************************************************************************
 FUNCTION	: DeleteModifiableLabelsTable
    
 PURPOSE	: Deletes modifiable labels table.
 
 PARAMETERS	: None.

 RETURNS	: Nothing.
*****************************************************************************/
{
	if(g_puModifiableLabelsTable != NULL)
	{
		UseAsm_Free(g_puModifiableLabelsTable);
		g_puModifiableLabelsTable = NULL;
	}
}

static IMG_UINT32 GetNextFreeModifiableLabelsTableEntry(IMG_VOID)
/*****************************************************************************
 FUNCTION	: GetNextFreeModifiableLabelsTableEntry
    
 PURPOSE	: Gives next free entry in modifiable labels table.
 
 PARAMETERS	: None.

 RETURNS	: Link to newly reserved entry in table.
*****************************************************************************/
{
	static IMG_UINT32 uModifiableLabelsCount = 0;
	g_puModifiableLabelsTable = realloc(g_puModifiableLabelsTable, 
		(uModifiableLabelsCount + 1) * sizeof(g_puModifiableLabelsTable[0]));
	TestMemoryAllocation(g_puModifiableLabelsTable);
	return uModifiableLabelsCount++;
}

static IMG_UINT32 GetModifiableLabelsTableEntryValue(
	IMG_UINT32 uLinkToModifiableLabelsTable)
/*****************************************************************************
 FUNCTION	: GetModifiableLabelsTableEntryValue
    
 PURPOSE	: Gets value stored in specified entry of modifiable labels table.
 
 PARAMETERS	: uLinkToModifiableLabelsTable	- Link to entry in modifiable 
											  labels table.

 RETURNS	: Values stored in specified entry of modifiable labels table, 
			  value is a link to labels table.
*****************************************************************************/
{
	return g_puModifiableLabelsTable[uLinkToModifiableLabelsTable];
}

static IMG_VOID SetModifiableLabelsTableEntryValue(
	IMG_UINT32 uLinkToModifiableLabelsTable, IMG_UINT32 uLinkToLabelsTable)
/*****************************************************************************
 FUNCTION	: SetModifiableLabelsTableEntryValue
    
 PURPOSE	: Sets value of a specified entry in modifiable labels table.
 
 PARAMETERS	: uLinkToModifiableLabelsTable	- Link to entry in modifiable 
											  labels table.
			  uLinkToLabelsTable	- Link to labels table, a value to store.

 RETURNS	: Nothing.
*****************************************************************************/
{
	g_puModifiableLabelsTable[uLinkToModifiableLabelsTable] = 
		uLinkToLabelsTable;
	return;
}

/* Modifiable named registers table, A table in which references to named 
registers can be modified later */
static PTEMP_REG* g_puModableNamedRegsTable;
/* An error in entry in modifiable named registers table, to give to error 
cases to stop multiple similar error messages */
static IMG_UINT32 g_uErrorNamedRegisterEntry;

static IMG_VOID InitModifiableNamedRegsTable(IMG_VOID)
/*****************************************************************************
 FUNCTION	: InitModifiableNamedRegsTable
    
 PURPOSE	: Initializes modifiable named registers table.
 
 PARAMETERS	: None.

 RETURNS	: Nothing.
*****************************************************************************/
{
	PTEMP_REG psNewTempReg = NULL;
	g_puModableNamedRegsTable = NULL;
	g_uErrorNamedRegisterEntry = GetNextFreeModifiableNamedRegsTableEntry();
	psNewTempReg = (PTEMP_REG) UseAsm_Malloc(sizeof(TEMP_REG));
	TestMemoryAllocation(psNewTempReg);
	psNewTempReg->pszName = NULL;
	psNewTempReg->uArrayElementsCount = 0;
	psNewTempReg->uHwRegNum = 0;
	SetModifiableNamedRegsTableEntryValue(g_uErrorNamedRegisterEntry, 
		 (IMG_PVOID)psNewTempReg);
}

static IMG_VOID DeleteModifiableNamedRegsTable(IMG_VOID)
/*****************************************************************************
 FUNCTION	: DeleteModifiableNamedRegsTable
    
 PURPOSE	: Deletes modifiable named registers table.
 
 PARAMETERS	: None.

 RETURNS	: Nothing.
*****************************************************************************/
{	
	if(g_puModableNamedRegsTable != NULL)
	{
		if(g_puModableNamedRegsTable[g_uErrorNamedRegisterEntry] != NULL)
		{
			UseAsm_Free(g_puModableNamedRegsTable[g_uErrorNamedRegisterEntry]);
			g_puModableNamedRegsTable[g_uErrorNamedRegisterEntry] = NULL;	
		}
		UseAsm_Free(g_puModableNamedRegsTable);
		g_puModableNamedRegsTable = NULL;
	}
	
}

static IMG_UINT32 GetNextFreeModifiableNamedRegsTableEntry(IMG_VOID)
/*****************************************************************************
 FUNCTION	: GetNextFreeModifiableNamedRegsTableEntry
    
 PURPOSE	: Gets next free modifiable named registers table entry.
 
 PARAMETERS	: None.

 RETURNS	: Link to newly reserved entry in table.
*****************************************************************************/
{
	static IMG_UINT32 uModifiableNamedRegsCount = 0;
	g_puModableNamedRegsTable = realloc(g_puModableNamedRegsTable, 
		(uModifiableNamedRegsCount + 1) * sizeof(g_puModableNamedRegsTable[0]));
	TestMemoryAllocation(g_puModableNamedRegsTable);
	return uModifiableNamedRegsCount++;
}

static IMG_PVOID GetModifiableNamedRegsTableEntryValue(
	IMG_UINT32 uLinkToModifiableNamedRegsTable)
/*****************************************************************************
 FUNCTION	: GetModifiableNamedRegsTableEntryValue
    
 PURPOSE	: Gets value stored in specified entry of modifiable named 
			  registers table.
 
 PARAMETERS	: uLinkToModifiableLabelsTable	- Link to entry in modifiable 
											  named registers table.

 RETURNS	: Values stored in specified entry of modifiable named registers 
			  table, value is a handle to named register definition.
*****************************************************************************/
{
	return 
		(IMG_PVOID)(g_puModableNamedRegsTable[uLinkToModifiableNamedRegsTable]);
}

static IMG_VOID SetModifiableNamedRegsTableEntryValue(
	IMG_UINT32 uLinkToModifiableNamedRegsTable, IMG_PVOID pvNamedTempRegHandle)
/*****************************************************************************
 FUNCTION	: SetModifiableNamedRegsTableEntryValue
    
 PURPOSE	: Sets value of a specified entry in modifiable named registers 
			  table.
 
 PARAMETERS	: uLinkToModifiableNamedRegsTable	- Link to entry in modifiable 
												  named registers table.
			  uLinkToLabelsTable	- Handle of named register definition, a 
									  value to store.

 RETURNS	: Nothing.
*****************************************************************************/
{
	g_puModableNamedRegsTable[uLinkToModifiableNamedRegsTable] = 
		(PTEMP_REG)pvNamedTempRegHandle;
	return;
}

static IMG_PVOID GetErrorNamedRegisterEntryValue(IMG_VOID)
/*****************************************************************************
 FUNCTION	: GetErrorNamedRegisterEntryValue
    
 PURPOSE	: Gets value error register entry in modifiable named registers 
			  table, value is a handle of error named register definition.
 
 PARAMETERS	: None.

 RETURNS	: Handle of error named register definition.
*****************************************************************************/
{
	return 
		(IMG_PVOID)(g_puModableNamedRegsTable[g_uErrorNamedRegisterEntry]);
}

/* Structure to store hardware register allocation range for named registers */
typedef struct _HW_REGS_ALLOC_RANGE
{	
	IMG_UINT32	uRangeStart;
	IMG_UINT32	uRangeEnd;
	IMG_BOOL	bOverideFileRange;
} HW_REGS_ALLOC_RANGE, *PHW_REGS_ALLOC_RANGE;

/* Variable to store hardware register allocation range for named registers */
static HW_REGS_ALLOC_RANGE g_sHwRegsAllocRange;

IMG_INTERNAL IMG_VOID InitHwRegsAllocRange(IMG_VOID)
/*****************************************************************************
 FUNCTION	: InitHwRegsAllocRange
    
 PURPOSE	: Initializes hardware register allocation range for named 
			  registers.
 
 PARAMETERS	: None.

 RETURNS	: Nothing.
*****************************************************************************/
{
	g_sHwRegsAllocRange.uRangeStart = 1;
	g_sHwRegsAllocRange.uRangeEnd = 0;
	g_sHwRegsAllocRange.bOverideFileRange = IMG_FALSE;	
}

IMG_INTERNAL IMG_BOOL IsHwRegsAllocRangeSet(IMG_VOID)
/*****************************************************************************
 FUNCTION	: IsHwRegsAllocRangeSet
    
 PURPOSE	: Tells whether hardware registers allocation range is currently 
			  specified or not.

 PARAMETERS	: None.

 RETURNS	: IMG_TRUE	- If hardware register allocation range is specified.
			  IMG_FALSE	- If hardware register allocation range is not 
						  specified.
*****************************************************************************/
{
	if(g_sHwRegsAllocRange.uRangeStart > g_sHwRegsAllocRange.uRangeEnd)
	{
		return IMG_FALSE;
	}
	else
	{
		return IMG_TRUE;
	}
}

IMG_INTERNAL IMG_VOID SetHwRegsAllocRange(IMG_UINT32 uRangeStart, 
										  IMG_UINT32 uRangeEnd, 
										  IMG_BOOL bOverideFileRange)
/*****************************************************************************
 FUNCTION	: SetHwRegsAllocRange
    
 PURPOSE	: Sets hardware registers allocation range.

 PARAMETERS	: uRangeStart	- Start of hardware registers allocation range.
			  uRangeEnd	- End of hardware registers allocation range.
			  bOverideFileRange	- Tells whether to overide hardware register 
								  allocation range specified in source file.

 RETURNS	: Nothing.
*****************************************************************************/
{
	if(
		(g_sHwRegsAllocRange.bOverideFileRange == IMG_FALSE) 
		|| 
		(g_sHwRegsAllocRange.bOverideFileRange == IMG_TRUE && 
			bOverideFileRange == IMG_TRUE) 
	)
	{
		g_sHwRegsAllocRange.uRangeStart = uRangeStart;
		g_sHwRegsAllocRange.uRangeEnd = uRangeEnd;
		g_sHwRegsAllocRange.bOverideFileRange = bOverideFileRange;		
	}
}

IMG_INTERNAL IMG_VOID SetHwRegsAllocRangeFromFile(IMG_UINT32 uRangeStart, 
												  IMG_UINT32 uRangeEnd)
/*****************************************************************************
 FUNCTION	: SetHwRegsAllocRangeFromFile
    
 PURPOSE	: Sets hardware registers allocation range from source file.

 PARAMETERS	: uRangeStart	- Start of hardware registers allocation range.
			  uRangeEnd	- End of hardware registers allocation range.
			  
 RETURNS	: Nothing.
*****************************************************************************/
{
	static IMG_BOOL bAlreadySetFromFile = IMG_FALSE;
	if(bAlreadySetFromFile == IMG_FALSE)
	{
		if(g_sHwRegsAllocRange.bOverideFileRange == IMG_FALSE)
		{
			g_sHwRegsAllocRange.uRangeStart = uRangeStart;
			g_sHwRegsAllocRange.uRangeEnd = uRangeEnd;
		}
		bAlreadySetFromFile = IMG_TRUE;
	}
	else
	{
		ParseError("Can not set h/w regs alloc range more than once from file");
	}
}

static IMG_UINT32 GetHwRegsAllocRangeStart(IMG_VOID)
/*****************************************************************************
 FUNCTION	: GetHwRegsAllocRangeStart
    
 PURPOSE	: Gets hardware registers allocation range's start.

 PARAMETERS	: None.
			  
 RETURNS	: Hardware registers allocation range start.
*****************************************************************************/
{
	return g_sHwRegsAllocRange.uRangeStart;
}

static IMG_UINT32 GetHwRegsAllocRangeEnd(IMG_VOID)
/*****************************************************************************
 FUNCTION	: GetHwRegsAllocRangeEnd
    
 PURPOSE	: Gets hardware registers allocation range's end.

 PARAMETERS	: None.
			  
 RETURNS	: Hardware registers allocation range end.
*****************************************************************************/
{
	return g_sHwRegsAllocRange.uRangeEnd;
}


static IMG_VOID DeleteAllNamedRegsOfScope(PSCOPE psScope)
/*****************************************************************************
 FUNCTION	: DeleteAllNamedRegsOfScope
    
 PURPOSE	: Delete all named registers of a scope.

 PARAMETERS	: psScope	- Pointer to scope whose named registers to delete.

 RETURNS	: Nothing.
*****************************************************************************/
{
	PTEMP_REG psUndefinedTempRegsItrator = psScope->psTempRegs;	
	PTEMP_REG psTempRegToDelete = NULL;
	while(psUndefinedTempRegsItrator != NULL)
	{
		psTempRegToDelete = psUndefinedTempRegsItrator;
		psUndefinedTempRegsItrator = psUndefinedTempRegsItrator->psNext;
		DeleteTempReg(psTempRegToDelete);		
	}
	psScope->psTempRegs = NULL;
	return;
}

static IMG_VOID DeleteCalleesRecordOfScope(PSCOPE psScope)
/*****************************************************************************
 FUNCTION	: DeleteCalleesRecordOfScope
    
 PURPOSE	: Delete callees record of a scope.

 PARAMETERS	: psScope	- Pointer to scope whose data to delete.

 RETURNS	: Nothing.
*****************************************************************************/
{
	PCALL_RECORD psItrator = psScope->psCalleesRecord;	
	PCALL_RECORD psToDelete = NULL;
	while(psItrator != NULL)
	{
		psToDelete = psItrator;
		psItrator = psItrator->psNext;
		UseAsm_Free(psToDelete);
	}
	psScope->psCalleesRecord = NULL;
	return;
}

static IMG_VOID DeleteCallsRecordOfScope(PSCOPE psScope)
/*****************************************************************************
 FUNCTION	: DeleteCallsRecordOfScope
    
 PURPOSE	: Delete calls record of a scope.

 PARAMETERS	: psScope	- Pointer to scope whose data to delete.

 RETURNS	: Nothing.
*****************************************************************************/
{
	PCALL_RECORD psItrator = psScope->psCallsRecord;	
	PCALL_RECORD psToDelete = NULL;
	while(psItrator != NULL)
	{
		psToDelete = psItrator;
		psItrator = psItrator->psNext;
		UseAsm_Free(psToDelete);
	}
	psScope->psCallsRecord = NULL;
	return;
}

static IMG_VOID DeleteNamedRegAllocChildrenRecrod(PSCOPE psScope)
/*****************************************************************************
 FUNCTION	: DeleteNamedRegAllocChildrenRecrod
    
 PURPOSE	: Delete named registers allocation children scopes record of a 
			  scope.

 PARAMETERS	: psScope	- Pointer to scope whose data to delete.

 RETURNS	: Nothing.
*****************************************************************************/
{
	PSCOPES_LIST psItrator = psScope->psNamedRegsAllocChildren;	
	PSCOPES_LIST psToDelete = NULL;
	while(psItrator != NULL)
	{
		psToDelete = psItrator;
		psItrator = psItrator->psNext;
		UseAsm_Free(psToDelete);
	}
	psScope->psNamedRegsAllocChildren = NULL;
	return;
}

static IMG_VOID DeleteAllNamedRegsOfScope(PSCOPE psScope);
static IMG_VOID DeleteCalleesRecordOfScope(PSCOPE psScope);
static IMG_VOID DeleteCallsRecordOfScope(PSCOPE psScope);
static IMG_VOID DeleteNamedRegAllocChildrenRecrod(PSCOPE psScope);

static IMG_VOID DeleteScopesData(PSCOPE psTopLevelScope)
/*****************************************************************************
 FUNCTION	: DeleteScopesData
    
 PURPOSE	: Frees memory used by all scopes data.

 PARAMETERS	: psTopLevelScope	- Top level scope to free memory from.
			  
 RETURNS	: Nothing.
*****************************************************************************/
{	
	PSCOPE psChildScopesIterator = psTopLevelScope->psChildScopes;
	PSCOPE psScopeToDelete = NULL;
	while(psChildScopesIterator != NULL)
	{
		psScopeToDelete = psChildScopesIterator;
		psChildScopesIterator = psChildScopesIterator->psNextSiblingScope;
		psTopLevelScope->psChildScopes = psChildScopesIterator;
		psScopeToDelete->psNextSiblingScope = NULL;
		DeleteScopesData(psScopeToDelete);
	}	
	DeleteAllNamedRegsOfScope(psTopLevelScope);
	DeleteCalleesRecordOfScope(psTopLevelScope);
	DeleteCallsRecordOfScope(psTopLevelScope);
	DeleteNamedRegAllocChildrenRecrod(psTopLevelScope);
	DeleteScope(psTopLevelScope);
}

static IMG_VOID DeleteScopesData(PSCOPE psTopLevelScope);
static IMG_VOID DeleteModifiableLabelsTable(IMG_VOID);
static IMG_VOID DeleteModifiableNamedRegsTable(IMG_VOID);

static IMG_VOID DeleteScopesManagementData(IMG_VOID)
/*****************************************************************************
 FUNCTION	: DeleteScopesManagementData
    
 PURPOSE	: Frees memory used by all scopes management data.

 PARAMETERS	: None.
			  
 RETURNS	: Nothing.
*****************************************************************************/
{
	DeleteScopesData(g_psCurrentScope);
	g_psCurrentScope = NULL;
	DeleteModifiableLabelsTable();
	DeleteModifiableNamedRegsTable();
}

/******************************************************************************
 End of file (scopeman.c)
******************************************************************************/
