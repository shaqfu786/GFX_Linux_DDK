%{
/*************************************************************************
 * Name         : cparser.y
 * Title        : USE assembler
 * Author       : David Welch
 * Created      : Jan 2002
 *
 * Copyright    : 2002-2006 by Imagination Technologies Limited. All rights reserved.
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
 *
 * Modifications:-
 * $Log: cparser.y $
 **************************************************************************/

#if defined(_MSC_VER)
#pragma warning (disable:4131)
#pragma warning (disable:4244)
#pragma warning (disable:4701)
#pragma warning (disable:4127)
#pragma warning (disable:4102)
#pragma warning (disable:4702)
#pragma warning (disable:4706)

/* Specify the malloc and free, to avoid problems with redeclarations on Windows */
#define YYMALLOC malloc
#define YYFREE free

#endif /* defined(_MSC_VER) */

#if defined(_MSC_VER)
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <img_types.h>

#include "ctree.h"

#define YYDEBUG 1
#define YYERROR_VERBOSE

static void yzerror(const char *fmt);

int yzlex(void);

IMG_PCHAR g_pszYzSourceFile;
IMG_UINT32 g_uYzSourceLine;

IMG_UINT32 g_uStructAlignOverride = 8;
IMG_UINT32 g_uDefaultStructAlignOverride = 8;

static void yzerror(const char *fmt)
{
	fprintf(stderr, "%s(%u): error: ", g_pszYzSourceFile, g_uYzSourceLine);
	fputs(fmt, stderr);
	fputs(".\n", stderr);

	g_bCCodeError = IMG_TRUE;
}

static YYLTYPE PickNonNullLocation(YYLTYPE sL1, YYLTYPE sL2)
{
	if (sL1.pszFilename != NULL)
	{
		return sL1;
	}
	assert(sL2.pszFilename != NULL);
	return sL2;
}

#define YYLTYPE YYLTYPE

# define YYLLOC_DEFAULT(Current, Rhs, N) \
	Current.pszFilename = (Rhs)->pszFilename; \
	Current.uLine = (Rhs)->uLine;

const char* yyvalue(int yychar, int yytype);

%}

%locations

%token SWITCH
%token SIGNED
%token MSVC__W64
%token VOID
%token SIZEOF
%token WHILE
%token UNSIGNED
%token DO
%token EXTERN
%token DOUBLE
%token VOLATILE
%token MSVC_PTR32
%token MSVC_PTR64
%token STATIC
%token ENUM
%token CONST
%token AUTO
%token UNION
%token FLOAT
%token RESTRICT
%token RETURN
%token TYPEDEF
%token IF
%token CONTINUE
%token CHAR
%token DEFAULT
%token USEASM_INLINE
%token CASE
%token STRUCT
%token BREAK
%token SHORT
%token INT
%token LONG
%token FOR
%token ELSE
%token GOTO
%token REGISTER
%token STRING_LITERAL
%token INTEGER_NUMBER
%token IDENTIFIER
%token FLOAT_NUMBER
%token TYPEDEF_NAME
%token MSVC__INT8;
%token MSVC__INT16;
%token MSVC__INT32;
%token MSVC__INT64;
%token INCREMENT
%token DECREMENT
%token STRUCTPTRACCESS
%token MULTIPLYASSIGNMENT
%token DIVIDEASSIGNMENT
%token MODULUSASSIGNMENT
%token ADDASSIGNMENT
%token SUBASSIGNMENT
%token SHIFTLEFTASSIGNMENT
%token SHIFTRIGHTASSIGNMENT
%token BITWISEANDASSIGNMENT
%token BITWISEXORASSIGNMENT
%token BITWISEORASSIGNMENT
%token LOGICALOR
%token LOGICALAND
%token LEFTSHIFT
%token RIGHTSHIFT
%token EQUALITY
%token LESSTHANOREQUAL
%token GREATERTHANOREQUAL
%token NOTEQUAL
%token MSVC__CDECL
%token MSVC__STDCALL
%token __INLINE
%token __FORCEINLINE
%token MSVC_FASTCALL
%token __ASM
%token _INLINE
%token MSVC__DECLSPEC
%token MSVC__DECLSPEC_ALLOCATE
%token MSVC__DECLSPEC_DLLIMPORT
%token MSVC__DECLSPEC_DLLEXPORT
%token MSVC__DECLSPEC_NAKED
%token MSVC__DECLSPEC_NORETURN
%token MSVC__DECLSPEC_NOTHROW
%token MSVC__DECLSPEC_NOVTABLE
%token MSVC__DECLSPEC_PROPERTY
%token MSVC__DECLSPEC_SELECTANY
%token MSVC__DECLSPEC_THREAD
%token MSVC__DECLSPEC_UUID
%token MSVC__DECLSPEC_PROPERTY_GET
%token MSVC__DECLSPEC_PROPERTY_PUT
%token MSVC__DECLSPEC_ALIGN
%token MSVC__DECLSPEC_DEPRECATED
%token MSVC__DECLSPEC_RESTRICT
%token MSVC__DECLSPEC_NOALIAS
%token MSVC_CDECL
%token GCC__ATTRIBUTE__
%token GCC__BUILTIN_VA_LIST
%token BOOL

%union
{
	IMG_UINT32					n;
	IMG_PCHAR					pszString;
	YZDECLARATION*				psDeclaration;
	YZDECLSPECIFIER*			psDeclSpecifier;
	YZINITDECLARATOR*			psInitDeclarator;
	YZDECLARATOR*				psDeclarator;
	YZSTRUCT*					psStruct;
	YZENUM*						psEnum;
	IMG_PVOID					pvVoid;
	IMG_BOOL					bBool;
	YZSTRUCTDECLARATION*		psStructDeclaration;
	YZSTRUCTDECLARATOR*			psStructDeclarator;
	YZENUMERATOR*				psEnumerator;
	YZPOINTER*					psPointer;
	YZEXPRESSION*				psExpression;
	YZTYPENAME*					psTypeName;
}

%printer    { YYFPRINTF (yyoutput, "%s", yyvaluep->pszString); } IDENTIFIER

%type <n>						INTEGER_NUMBER
%type <pszString>				FLOAT_NUMBER
%type <pszString>				STRING_LITERAL
%type <pszString>				IDENTIFIER
%type <pszString>				TYPEDEF_NAME

%type <pszString>				identifier_or_typedef_name
%type <pszString>				string_literal
%type <psDeclaration>			declaration
%type <psDeclSpecifier>			declaration_specifiers
%type <psInitDeclarator>		init_declarator_list
%type <psInitDeclarator>		init_declarator
%type <psDeclSpecifier>			declaration_specifier
%type <psDeclSpecifier>			storage_class_specifier
%type <psDeclSpecifier>			type_specifier
%type <psDeclSpecifier>			type_qualifier
%type <psDeclSpecifier>			function_specifier
%type <psDeclarator>			declarator
%type <pvVoid>					initializer
%type <psStruct>				struct_or_union_specifier
%type <psEnum>					enum_specifier
%type <bBool>					struct_or_union
%type <psStructDeclaration>		struct_declaration_list
%type <psStructDeclaration>		struct_declaration
%type <psDeclSpecifier>			specifier_qualifier_list
%type <psDeclSpecifier>			specifier_qualifier_list_opt
%type <psStructDeclarator>		struct_declarator_list
%type <psStructDeclarator>		struct_declarator
%type <psEnumerator>			enumerator_list
%type <psEnumerator>			enumerator
%type <psDeclarator>			direct_declarator
%type <psPointer>				pointer_opt
%type <psPointer>				pointer
%type <psDeclSpecifier>			type_qualifier_list_opt
%type <psDeclSpecifier>			type_qualifier_list
%type <psDeclSpecifier>			microsoft_modifier
%type <psDeclSpecifier>			declspec
%type <psStructDeclarator>		struct_declarator_list_opt

%type <n>						unary_operator
%type <n>						assignment_operator

%type <psExpression>			primary_expression
%type <psExpression>			postfix_expression
%type <psExpression>			argument_expression_list_opt
%type <psExpression>			argument_expression_list
%type <psExpression>			unary_expression
%type <psExpression>			cast_expression
%type <psExpression>			multiplicative_expression
%type <psExpression>			additive_expression
%type <psExpression>			shift_expression
%type <psExpression>			relational_expression
%type <psExpression>			equality_expression
%type <psExpression>			and_expression
%type <psExpression>			exclusive_or_expression
%type <psExpression>			inclusive_or_expression
%type <psExpression>			logical_and_expression
%type <psExpression>			logical_or_expression
%type <psExpression>			conditional_expression
%type <psExpression>			assignment_expression
%type <psExpression>			direct_abstract_declaractor_array_size
%type <psExpression>			expression
%type <psExpression>			expression_opt
%type <psExpression>			constant_expression

%type <psTypeName>				type_name

%type <psDeclarator>			abstract_declarator
%type <psDeclarator>			abstract_declarator_opt

%type <psDeclarator>			direct_abstract_declarator
%type <psDeclarator>			direct_abstract_declarator_opt

%left TYPEDEF_NAME

%start program
%%

program:
			/* Nothing */
		|	program extern_declaration
		;

extern_declaration:
			function_definition
		{
			/* Ignore function definitions. */
		}
		|	declaration									
		{
				CTreeAddExternDecl($1);
		}
		|	';'
		{
			/* Extra semi-colon outside of a function. Both VisualC and gcc allow this. */
		}
		;

function_definition:
		declaration_specifiers declarator declaration_list compound_statement
		;

declaration_list:
			/* Nothing */
		| declaration_list declaration
		;

declaration:
		declaration_specifiers init_declarator_list ';'
		{
			$$ = CTreeMakeDeclaration(@2);
			$$->psDeclSpecifierList = $1;
			$$->psInitDeclaratorList = $2;
		}
		;

declaration_specifiers:
			declaration_specifier
		{
			$$ = $1;
			@$ = @1;
		}
		|	declaration_specifier declaration_specifiers
		{
			$$ = CTreeAddToDeclSpecifierList($1, $2);
			@$ = @1;
		}
		;

declaration_specifier:
			storage_class_specifier	
		{
			$$ = $1;
			@$ = @1;
		}
		|	type_specifier
		{
			$$ = $1;
			@$ = @1;
		}
		|	type_qualifier
		{
			$$ = $1;
			@$ = @1;
		}
		|	function_specifier
		{
			$$ = $1;
			@$ = @1;
		}
		;

init_declarator_list:
			init_declarator
		{
			$$ = $1;
		}
		|	init_declarator_list ',' init_declarator
		{
			$$ = CTreeAddToInitDeclaratorList($1, $3);
		}
		;

init_declarator:
			/* Nothing */
		{	
			$$ = NULL; 
		}
		|	declarator
		{
			$$ = CTreeMakeInitDeclarator(@1);
			$$->psDeclarator = $1;
			$$->psInitializer = NULL;
		}
		|	declarator '=' initializer
		{
			$$ = CTreeMakeInitDeclarator(@1);
			$$->psDeclarator = $1;
			$$->psInitializer = $3;
		}
		;

storage_class_specifier:
			TYPEDEF
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_TYPEDEF;
		}
		|	EXTERN
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_EXTERN;
		}
		|	STATIC
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_STATIC;
		}
		|	AUTO
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_AUTO;
		}
		|	REGISTER
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_REGISTER;
		}
		;

type_specifier:
			VOID
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_VOID;
		}
		|	CHAR
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_CHAR;
		}
		|	SHORT
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_SHORT;
		}
		|	INT
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_INT;
		}
		|	LONG
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_LONG;
		}
		|	FLOAT
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_FLOAT;
		}
		|	DOUBLE
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_DOUBLE;
		}
		|	MSVC__INT8
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_MSVC_INT8;
		}
		|	MSVC__INT16
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_MSVC_INT16;
		}
		|	MSVC__INT32
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_MSVC_INT32;
		}
		|	MSVC__INT64
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_MSVC_INT64;
		}
		|	SIGNED
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_SIGNED;
		}
		|	MSVC__W64
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_MSVC__W64;
		}
		|	UNSIGNED
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_UNSIGNED;
		}
		|	struct_or_union_specifier
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_STRUCT;
			$$->psStruct = $1;
		}
		|	enum_specifier
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_ENUM;
			$$->psEnum = $1;
		}
		|	TYPEDEF_NAME
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_TYPEDEF_NAME;
			$$->pszIdentifier = $1;
		}
		|	GCC__BUILTIN_VA_LIST
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_GCC_BUILTIN_VA_LIST;
		}
		|	BOOL
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_BOOL;
		}
		;

struct_or_union_specifier:
			struct_or_union	'{' struct_declaration_list '}'
		{
			$$ = CTreeMakeStruct(@1);
			$$->bStruct = $1;
			$$->pszName = NULL;
			$$->psMemberList = $3;
			$$->uAlignOverride = g_uStructAlignOverride;
		}
		|	struct_or_union identifier_or_typedef_name '{' struct_declaration_list '}'
		{
			$$ = CTreeMakeStruct(@1);
			$$->bStruct = $1;
			$$->pszName = $2;
			$$->psMemberList = $4;
			$$->uAlignOverride = g_uStructAlignOverride;
		}
		|	struct_or_union declspec identifier_or_typedef_name '{' struct_declaration_list '}'
		{
			$$ = CTreeMakeStruct(@1);
			$$->bStruct = $1;
			$$->pszName = $3;
			$$->psMemberList = $5;
			$$->uAlignOverride = g_uStructAlignOverride;
		}
		|	struct_or_union identifier_or_typedef_name
		{
			$$ = CTreeMakeStruct(@1);
			$$->bStruct = $1;
			$$->pszName = $2;
			$$->psMemberList = NULL;
		}
		;

identifier_or_typedef_name:
			IDENTIFIER
		{
			$$ = $1;
		}
		|	TYPEDEF_NAME
		{
			$$ = $1;
		}
		;

struct_or_union:
			STRUCT												
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine; 
			$$ = IMG_TRUE; 
		}
		|	UNION												
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine; 
			$$ = IMG_FALSE; 
		}
		;

struct_declaration_list:
			struct_declaration
		{
			$$ = $1;
		}
		|	struct_declaration_list struct_declaration
		{
			$$ = CTreeAddToStructDeclarationList($1, $2);
		}
		;

struct_declaration:
		  specifier_qualifier_list struct_declarator_list_opt ';'
		{
			$$ = CTreeMakeStructDeclaration(PickNonNullLocation(@2, @1));
			$$->psSpecifierList = $1;
			$$->psDeclaratorList = $2;
		}
		;

specifier_qualifier_list_opt:
			/* Nothing */
		{
			$$ = NULL;
			@$.pszFilename = NULL;
			@$.uLine = (IMG_UINT32)-1;
		}
		| specifier_qualifier_list
		{
			$$ = $1;
		}
		;

specifier_qualifier_list:
			type_specifier specifier_qualifier_list_opt
		{
			$$ = CTreeAddToDeclSpecifierList($1, $2);
			@$ = @1;
		}
		|	type_qualifier specifier_qualifier_list_opt
		{
			$$ = CTreeAddToDeclSpecifierList($1, $2);
			@$ = @1;
		}
		;

struct_declarator_list_opt:
			/* Nothing */
		{
			$$ = NULL;
			@$.pszFilename = NULL;
			@$.uLine = (IMG_UINT32)-1;
		}
		|	struct_declarator_list
		{
			$$ = $1;
		}
		;

struct_declarator_list:
			struct_declarator
		{
			$$ = $1;
			
		}
		|	struct_declarator_list ',' struct_declarator
		{
			$$ = CTreeAddToStructDeclaratorList($1, $3);
		}
		;

struct_declarator:
			declarator
		{
			$$ = CTreeMakeStructDeclarator(@1);
			$$->psDeclarator = $1;
		}
		|	declarator ':' constant_expression
		{
			$$ = CTreeMakeStructDeclarator(@1);
			$$->psDeclarator = $1;
			$$->bBitField = IMG_TRUE;
			$$->psBitFieldSize = $3;
		}
		|	':' constant_expression
		{
			$$ = CTreeMakeStructDeclarator(@2);
			$$->psDeclarator = NULL;
			$$->bBitField = IMG_TRUE;
			$$->psBitFieldSize = $2;
		}
		;

enum_specifier:
			ENUM '{' enumerator_list '}'
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeEnum(@$);
			$$->pszIdentifier = NULL;
			$$->psEnumeratorList = $3;
		}
		|	ENUM IDENTIFIER '{' enumerator_list '}'
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeEnum(@$);
			$$->pszIdentifier = $2;
			$$->psEnumeratorList = $4;
		}
		|	ENUM '{' enumerator_list ',' '}'
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeEnum(@$);
			$$->pszIdentifier = NULL;
			$$->psEnumeratorList = $3;
		}
		|	ENUM IDENTIFIER '{' enumerator_list ',' '}'
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeEnum(@$);
			$$->pszIdentifier = $2;
			$$->psEnumeratorList = $4;
		}
		|	ENUM IDENTIFIER
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeEnum(@$);
			$$->bPartial = IMG_TRUE;
			$$->pszIdentifier = $2;
		}
		;

enumerator_list:
			enumerator
		{
			$$ = $1;
		}
		|	enumerator_list ',' enumerator
		{
			$$ = CTreeAddToEnumeratorList($1, $3);
		}
		;

enumerator:
			IDENTIFIER
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeEnumerator(@$);
			$$->pszIdentifier = $1;
			$$->psValue = NULL;
		}
		|	IDENTIFIER '=' constant_expression
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeEnumerator(@$);
			$$->pszIdentifier = $1;
			$$->psValue = $3;
		}
		;

type_qualifier:
			CONST
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine; 
			$$ = CTreeMakeDeclSpecifier(@$); 
			$$->uType = YZDECLSPECIFIER_CONST; 
		}
		|	RESTRICT
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine; 
			$$ = CTreeMakeDeclSpecifier(@$); 
			$$->uType = YZDECLSPECIFIER_RESTRICT; 
		}
		|	VOLATILE
		{ 
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$); 
			$$->uType = YZDECLSPECIFIER_VOLATILE; 
		}
		| microsoft_modifier
		;

function_specifier:
			USEASM_INLINE
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_INLINE;
		}
		|	__INLINE
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_INLINE;
		}
		|	_INLINE
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_INLINE;
		}
		|	__FORCEINLINE
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_FORCEINLINE;
		}
		;


microsoft_modifier:
			MSVC__CDECL
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_CDECL;
		}
		|	MSVC_CDECL
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_CDECL;
		}
		|	MSVC__STDCALL
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_STDCALL;
		}
		|	MSVC_FASTCALL
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER__FASTCALL;
		}
		|   MSVC_PTR32
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_MSVC_PTR32;
		}
		|   MSVC_PTR64
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_MSVC_PTR64;
		}
		|	declspec
		{
			@$ = @1;
			$$ = $1;
		}
		;
		
declspec:
		MSVC__DECLSPEC '(' microsoft_declspec ')'
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclSpecifier(@$);
			$$->uType = YZDECLSPECIFIER_DECLSPEC;
		}
		;

microsoft_declspec:
			MSVC__DECLSPEC_ALLOCATE '(' string_literal ')'
		|	MSVC__DECLSPEC_DLLIMPORT
		|	MSVC__DECLSPEC_DLLEXPORT
		|	MSVC__DECLSPEC_NAKED
		|	MSVC__DECLSPEC_NORETURN
		|	MSVC__DECLSPEC_NOTHROW
		|	MSVC__DECLSPEC_NOVTABLE
		|	MSVC__DECLSPEC_PROPERTY '(' microsoft_declspec_property_list ')'
		|	MSVC__DECLSPEC_SELECTANY
		|	MSVC__DECLSPEC_THREAD
		|	MSVC__DECLSPEC_UUID '(' string_literal ')'
		|	MSVC__DECLSPEC_ALIGN '(' assignment_expression ')'
		|	MSVC__DECLSPEC_DEPRECATED
		|	MSVC__DECLSPEC_DEPRECATED '(' string_literal ')'
		|	MSVC__DECLSPEC_RESTRICT
		|	MSVC__DECLSPEC_NOALIAS
		;

microsoft_declspec_property_list:
			microsoft_declspec_property
		|	microsoft_declspec_property_list microsoft_declspec_property
		;

microsoft_declspec_property:
			MSVC__DECLSPEC_PROPERTY_PUT '=' IDENTIFIER
		|	MSVC__DECLSPEC_PROPERTY_GET '=' IDENTIFIER
		;

declarator:
			pointer_opt direct_declarator attribute_opt
		{	
			$$ = $2;
			$$->psPointer = $1;
			@$ = @2;
		}
		;
		
gcc_attribute:	
			GCC__ATTRIBUTE__ '(' '(' attribute_specifier_list ')' ')'
		{
		};
		
gcc_attribute_list:	
			gcc_attribute
		|	gcc_attribute gcc_attribute_list
		;
		
attribute_opt:
			/* Nothing */
		{
		}
		| gcc_attribute_list
		{
		}
		;
		
attribute_specifier_list:
			attribute_specifier
		|	attribute_specifier_list attribute_specifier
		;
		
attribute_specifier:
			IDENTIFIER
		|	IDENTIFIER '(' IDENTIFIER attribute_specifier_parameter_list_opt ')'
		;
		
attribute_specifier_parameter_list_opt:
			/* Nothing */
		|	',' attribute_specifier_parameter_list
		;
		
attribute_specifier_parameter_list:
			assignment_expression
		|	attribute_specifier_parameter_list ',' assignment_expression
		;

direct_declarator:
			IDENTIFIER
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclarator(@$);
			$$->uType = YZDECLARATOR_IDENTIFIER;
			$$->pszIdentifier = $1;
		}
		|	TYPEDEF_NAME
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakeDeclarator(@$);
			$$->uType = YZDECLARATOR_IDENTIFIER;
			$$->pszIdentifier = $1;
		}
		|	'(' type_qualifier_list_opt declarator ')'
		{
			$$ = CTreeMakeDeclarator(@3);
			$$->uType = YZDECLARATOR_BRACKETED;
			$$->psSubDeclarator = $3;
			@$ = @3;
		}
		|	direct_declarator '[' ']'
		{
			$$ = CTreeMakeDeclarator(@1);
			$$->uType = YZDECLARATOR_VARIABLELENGTHARRAY;
			$$->psSubDeclarator = $1;
			@$ = @1;
		}
		|	direct_declarator '[' assignment_expression ']'
		{
			$$ = CTreeMakeDeclarator(@1);
			$$->uType = YZDECLARATOR_FIXEDLENGTHARRAY;
			$$->psSubDeclarator = $1;
			$$->psArraySize = $3;
			@$ = @1;
		}
		|	direct_declarator '[' '*' ']'
		{
			$$ = CTreeMakeDeclarator(@1);
			$$->uType = YZDECLARATOR_VARIABLELENGTHARRAY;
			$$->psSubDeclarator = $1;
			@$ = @1;
		}
		|	direct_declarator '(' parameter_type_list ')'
		{
			$$ = CTreeMakeDeclarator(@1);
			$$->uType = YZDECLARATOR_FUNCTION;
			$$->psSubDeclarator = $1;
			@$ = @1;
		}
		|	direct_declarator '(' ')'
		{
			$$ = CTreeMakeDeclarator(@1);
			$$->uType = YZDECLARATOR_FUNCTION;
			$$->psSubDeclarator = $1;
			@$ = @1;
		}
		|	direct_declarator '(' identifier_list ')'
		{
			$$ = CTreeMakeDeclarator(@1);
			$$->uType = YZDECLARATOR_FUNCTION;
			$$->psSubDeclarator = $1;
			@$ = @1;
		}
		;

pointer_opt:
			/* Nothing */
		{
			$$ = NULL;
		}
		|	pointer
		{
			$$ = $1;
		}
		;

pointer:
			'*' type_qualifier_list_opt 
		{
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			$$ = CTreeMakePointer(@$); 
			$$->psQualifierList = $2;
		}
		|	'*' type_qualifier_list_opt pointer 
		{
			YZPOINTER*	psRet;
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
			psRet = CTreeMakePointer(@$);
			psRet->psQualifierList = $2;
			psRet->psNext = $3;
			$$ = psRet;
		}
		;

type_qualifier_list_opt:
			/* Nothing */
		{ 
			$$ = NULL;
		}
		|	type_qualifier_list
		{ 
			$$ = $1; 
		}
		;

type_qualifier_list:
			type_qualifier
		{ 
			$$ = $1;
		}
		|	type_qualifier_list type_qualifier
		{ 
			$$ = CTreeAddToDeclSpecifierList($1, $2);
		}
		;

parameter_type_list_opt:
			/* Nothing */
		|	parameter_type_list
		;

parameter_type_list:
			parameter_list
		|	parameter_list ',' '.' '.' '.'
		;

parameter_list:
			parameter_declaration
		|	parameter_list ',' parameter_declaration
		;

parameter_declaration:
			declaration_specifiers declarator
		|	declaration_specifiers abstract_declarator_opt
		;

identifier_list:
			IDENTIFIER
		|	identifier_list ',' IDENTIFIER
		;

type_name:
			specifier_qualifier_list abstract_declarator_opt
		{
			$$ = CTreeMakeTypeName(@2, $1, $2);
			@$ = @1;
		}	
		;

abstract_declarator_opt:
			/* Nothing */
		{
			$$ = NULL;
		}
		|	abstract_declarator
		{
			$$ = $1;
		}
		;

abstract_declarator:
			pointer
		{
			$$ = CTreeMakeDeclarator(@1);
			$$->uType = YZDECLARATOR_ABSTRACTPOINTER;
			$$->psPointer = $1;
			$$->psSubDeclarator = NULL;
		}
		|	pointer_opt direct_abstract_declarator
		{
			if ($1 == NULL)
			{
				$$ = $2;
			}
			else
			{
				$$ = CTreeMakeDeclarator(@1);
				$$->uType = YZDECLARATOR_ABSTRACTPOINTER;
				$$->psPointer = $1;
				$$->psSubDeclarator = $2;
			}
		}
		;
		
direct_abstract_declaractor_array_size:
			/* Nothing */
		{
			$$ = NULL;
		}
		|	'*'
		{
			$$ = NULL;
		}
		|	assignment_expression
		{
			$$ = $1;
		}
		;

direct_abstract_declarator:
			'(' type_qualifier_list_opt abstract_declarator ')'
		{
			$$ = CTreeMakeDeclarator(@3);
			$$->uType = YZDECLARATOR_BRACKETED;
			$$->psQualifierList = $2;
			$$->psSubDeclarator = $3;
		}
		|	direct_abstract_declarator_opt '[' direct_abstract_declaractor_array_size ']'
		{
			$$ = CTreeMakeDeclarator(@3);
			if ($3 != NULL)
			{
				$$->uType = YZDECLARATOR_FIXEDLENGTHARRAY;
			}
			else
			{
				$$->uType = YZDECLARATOR_VARIABLELENGTHARRAY;
			}
			$$->psSubDeclarator = $1;
			$$->psArraySize = $3;
		}
		|	direct_abstract_declarator_opt '(' parameter_type_list_opt ')'
		{
			$$ = CTreeMakeDeclarator(@3);
			$$->uType = YZDECLARATOR_FUNCTION;
			$$->psSubDeclarator = $1;
		}
		;

direct_abstract_declarator_opt:
			/* Nothing */
		{
			$$ = NULL;
		}
		| direct_abstract_declarator
		{
			$$ = $1;
		}
		;

initializer:
			assignment_expression
		{ $$ = NULL; }
		|	'{' initializer_list '}'
		{ $$ = NULL; }
		|	'{' initializer_list ',' '}'
		{ $$ = NULL; }
		;

initializer_list:
			designation_opt initializer
		|	initializer_list ',' designation_opt initializer
		;

designation_opt:
			/* Nothing */
		|	designation
		;

designation:
			designator_list '='
		;

designator_list:
			designator
		|	designator_list designator
		;

designator:
			'[' constant_expression ']'
			'.' IDENTIFIER
		;

statement:
			labeled_statement
		|	compound_statement
		|	expression_statement
		|	selection_statement
		|	iteration_statement
		|	jump_statement
		;

labeled_statement:
			IDENTIFIER ':' statement
		|	CASE constant_expression ':' statement
		|	DEFAULT ':' statement
		;

compound_statement:
			'{' block_item_list_opt '}'
		;

block_item_list_opt:
			/* Nothing */
		|	block_item_list
		;

block_item_list:
			block_item
		|	block_item_list block_item
		;

block_item:
			declaration
		|	statement
		;

expression_statement:
			';'
		|	expression ';'
		;

selection_statement:
			IF '(' expression ')' statement
		|	IF '(' expression ')' statement ELSE statement
		|	SWITCH '(' expression ')' statement
		;

iteration_statement:
			WHILE '(' expression ')' statement
		|	DO statement WHILE '(' expression ')' ';'
		|	FOR '(' expression_opt ';' expression_opt ';' expression_opt ')' statement
		|	FOR	'(' declaration expression_opt ';' expression_opt ')' statement
		|	__ASM
		;

jump_statement:
			GOTO IDENTIFIER ';'
		|	CONTINUE ';'
		|	BREAK ';'
		|	RETURN expression_opt ';'
		;

primary_expression:
			IDENTIFIER
		{ 
				@$.pszFilename = g_pszYzSourceFile;
				@$.uLine = g_uYzSourceLine;
				$$ = CTreeMakeExpression(YZEXPRESSION_IDENTIFIER, @$); 
				$$->pszString = $1;
		}
		|	INTEGER_NUMBER
		{ 
				@$.pszFilename = g_pszYzSourceFile;
				@$.uLine = g_uYzSourceLine;
				$$ = CTreeMakeExpression(YZEXPRESSION_INTEGER_NUMBER, @$); 
				$$->uNumber = $1;
		}
		|	FLOAT_NUMBER
		{ 
				@$.pszFilename = g_pszYzSourceFile;
				@$.uLine = g_uYzSourceLine;
				$$ = CTreeMakeExpression(YZEXPRESSION_FLOAT_NUMBER, @$); 
				$$->pszString = $1;
		}
		|	string_literal
		{ 
				@$.pszFilename = g_pszYzSourceFile;
				@$.uLine = g_uYzSourceLine;
				$$ = CTreeMakeExpression(YZEXPRESSION_STRING_LITERAL, @$); 
				$$->pszString = $1;
		}
		|	'(' expression ')'
		{ 
				$$ = $2;
		}
		;

postfix_expression:
			primary_expression
		{ $$ = $1; }
		|	postfix_expression '[' expression ']'
		{ 
			$$ = CTreeMakeExpression(YZEXPRESSION_ARRAYACCESS, $1->sLocation); 
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		|	postfix_expression	'(' argument_expression_list_opt ')'
		{ 
			$$ = CTreeMakeExpression(YZEXPRESSION_FUNCTIONCALL,  $1->sLocation); 
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		|	postfix_expression '.' identifier_or_typedef_name
		{ 
			$$ = CTreeMakeExpression(YZEXPRESSION_STRUCTACCESS,  $1->sLocation); 
			$$->psSubExpr1 = $1;
			$$->pszString = $3;
		}
		|	postfix_expression STRUCTPTRACCESS identifier_or_typedef_name
		{ 
			$$ = CTreeMakeExpression(YZEXPRESSION_STRUCTPTRACCESS,  $1->sLocation); 
			$$->psSubExpr1 = $1;
			$$->pszString = $3;
		}
		|	postfix_expression INCREMENT
		{ 
			$$ = CTreeMakeExpression(YZEXPRESSION_POSTINCREMENT,  $1->sLocation); 
			$$->psSubExpr1 = $1;
		}
		|	postfix_expression DECREMENT
		{ 
			$$ = CTreeMakeExpression(YZEXPRESSION_POSTDECREMENT,  $1->sLocation); 
			$$->psSubExpr1 = $1;
		}
		|	'(' type_name ')' '{' initializer_list '}'
		{ 
			$$ = CTreeMakeExpression(YZEXPRESSION_INITIALIZER,  @2);
		}
		|	'(' type_name ')' '{' initializer_list ',' '}'
		{ 
			$$ = CTreeMakeExpression(YZEXPRESSION_INITIALIZER, @2);
		}
		;

argument_expression_list_opt:
			/* Nothing */
		{ $$ = NULL; }
		|	argument_expression_list
		{ $$ = $1; }
		;

argument_expression_list:
			assignment_expression
		{ $$ = $1; }
		|	argument_expression_list ',' assignment_expression
		{ $$ = CTreeAddToExpressionList($1, $3); }
		;

unary_expression:
			postfix_expression
		{ 
			$$ = $1; 
		}
		|	INCREMENT unary_expression
		{ 
			$$ = CTreeMakeExpression(YZEXPRESSION_PREINCREMENT, $2->sLocation); 
			$$->psSubExpr1 = $2;
		}
		|	DECREMENT unary_expression
		{ 
			$$ = CTreeMakeExpression(YZEXPRESSION_PREINCREMENT, $2->sLocation); 
			$$->psSubExpr1 = $2;
		}
		|	unary_operator cast_expression
		{ 
			$$ = CTreeMakeExpression($1, $2->sLocation); 
			$$->psSubExpr1 = $2;
		}
		|	SIZEOF unary_expression
		{ 
			$$ = CTreeMakeExpression(YZEXPRESSION_SIZEOFEXPRESSION, $2->sLocation); 
			$$->psSubExpr1 = $2;
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
		}
		|	SIZEOF '(' type_name ')'
		{ 
			$$ = CTreeMakeExpression(YZEXPRESSION_SIZEOFTYPE, @3);
			$$->psSizeofTypeName = $3;
			@$.pszFilename = g_pszYzSourceFile;
			@$.uLine = g_uYzSourceLine;
		}
		;

unary_operator:
			'&'
		{ $$ = YZEXPRESSION_ADDRESSOF; }
		|	'*'
		{ $$ = YZEXPRESSION_DEREFERENCE; }
		|	'+'
		{ $$ = YZEXPRESSION_POSITIVE; }
		|	'-'
		{ $$ = YZEXPRESSION_NEGATE; }
		|	'~'
		{ $$ = YZEXPRESSION_BITWISENOT; }
		|	'!'
		{ $$ = YZEXPRESSION_LOGICALNOT; }
		;

cast_expression:
			unary_expression
		{
			$$ = $1;
		}
		|	'(' type_name ')' cast_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_CAST, $4->sLocation);
			$$->psSubExpr1 = $4;
		}
		;

multiplicative_expression:
			cast_expression
		{
			$$ = $1;
		}
		|	multiplicative_expression '*' cast_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_MULTIPLY, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		|	multiplicative_expression '/' cast_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_DIVIDE, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		|	multiplicative_expression '%' cast_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_MODULUS, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		;

additive_expression:
			multiplicative_expression
		{
			$$ = $1;
		}
		|	additive_expression '+' multiplicative_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_ADD, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		|	additive_expression '-' multiplicative_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_SUB, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		;

shift_expression:
			additive_expression
		{ 
			$$ = $1; 
		}
		|	shift_expression LEFTSHIFT additive_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_SHIFTLEFT, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		|	shift_expression RIGHTSHIFT additive_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_SHIFTRIGHT, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		;

relational_expression:
			shift_expression
		{ 
			$$ = $1; 
		}
		|	relational_expression '<' shift_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_LT, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		|	relational_expression '>' shift_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_GT, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		|	relational_expression LESSTHANOREQUAL shift_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_LTE, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		|	relational_expression GREATERTHANOREQUAL shift_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_GTE, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		;

equality_expression:
			relational_expression
		{ 
			$$ = $1; 
		}
		|	equality_expression EQUALITY relational_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_EQ, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		|	equality_expression NOTEQUAL relational_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_NEQ, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		;

and_expression:
			equality_expression
		{ 
			$$ = $1; 
		}
		|	and_expression '&' equality_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_BITWISEAND, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		;

exclusive_or_expression:
			and_expression
		{ 
			$$ = $1; 
		}
		|	exclusive_or_expression '^' and_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_BITWISEXOR, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		;

inclusive_or_expression:
			exclusive_or_expression
		{ 
			$$ = $1; 
		}
		|	inclusive_or_expression '|' exclusive_or_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_BITWISEOR, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		;

logical_and_expression:
			inclusive_or_expression
		{ 
			$$ = $1; 
		}
		|	logical_and_expression LOGICALAND inclusive_or_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_LOGICALAND, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		;

logical_or_expression:
			logical_and_expression
		{ 
			$$ = $1; 
		}
		|	logical_or_expression LOGICALOR logical_and_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_LOGICALOR, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		;

conditional_expression:
			logical_or_expression
		{ 
			$$ = $1; 
		}
		|	logical_or_expression '?' expression ':' conditional_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_CONDITIONAL, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
			$$->psSubExpr3 = $5;
		}
		;

assignment_expression:
			conditional_expression
		{ 
			$$ = $1; 
		}
		|	unary_expression assignment_operator assignment_expression
		{ 
			$$ = CTreeMakeExpression($2, $1->sLocation); 
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		;

assignment_operator:
			'='
		{ $$ = YZEXPRESSION_ASSIGNMENT; }
		|	MULTIPLYASSIGNMENT
		{ $$ = YZEXPRESSION_MULTIPLYASSIGNMENT; }
		|	DIVIDEASSIGNMENT
		{ $$ = YZEXPRESSION_DIVIDEASSIGNMENT; }
		|	MODULUSASSIGNMENT
		{ $$ = YZEXPRESSION_MODULUSASSIGNMENT; }
		|	ADDASSIGNMENT
		{ $$ = YZEXPRESSION_ADDASSIGNMENT; }
		|	SUBASSIGNMENT
		{ $$ = YZEXPRESSION_SUBASSIGNMENT; }
		|	SHIFTLEFTASSIGNMENT
		{ $$ = YZEXPRESSION_SHIFTLEFTASSIGNMENT; }
		|	SHIFTRIGHTASSIGNMENT
		{ $$ = YZEXPRESSION_SHIFTRIGHTASSIGNMENT; }
		|	BITWISEANDASSIGNMENT
		{ $$ = YZEXPRESSION_BITWISEANDASSIGNMENT; }
		|	BITWISEXORASSIGNMENT
		{ $$ = YZEXPRESSION_BITWISEXORASSIGNMENT; }
		|	BITWISEORASSIGNMENT
		{ $$ = YZEXPRESSION_BITWISEORASSIGNMENT; }
		;

expression:
			assignment_expression
		{
			$$ = $1;
		}
		|	expression ',' assignment_expression
		{
			$$ = CTreeMakeExpression(YZEXPRESSION_COMMA, $1->sLocation);
			$$->psSubExpr1 = $1;
			$$->psSubExpr2 = $3;
		}
		;

expression_opt:
			/* Nothing */
		{ $$ = NULL; }
		|	expression
		{ $$ = $1; }
		;

constant_expression:
			conditional_expression
		{ $$ = $1; }
		;
		
string_literal:	
			STRING_LITERAL
		|	string_literal STRING_LITERAL
		;

%%

const char* yyvalue(int yychar, int yytype)
{
	if (yychar == IDENTIFIER)
	{
		static char temp[512];

		sprintf(temp, "identifier '%s'", yylval.pszString);
		return temp;
	}
	else
	{
		return yytname[yytype];
	}
}
