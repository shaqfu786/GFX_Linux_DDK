%{
/*************************************************************************
 * Name         : clexer.l
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
 * $Log: clexer.l $
 **************************************************************************/

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <img_types.h>

#include "ctree.h"
#include "osglue.h"

#include "cparser.tab.h"

#if defined(_MSC_VER)
#pragma warning (disable:4131)
#pragma warning (disable:4127)
#pragma warning (disable:4244)
#pragma warning (disable:4505)
#endif /* defined(_MSC_VER) */

#define YY_SKIP_YYWRAP 1

int yzlex(void);

extern int yzdebug;

static int g_dwYzDeclSpecBracketCount = 0;
static int g_dwYzPragmaBracketCount = 0;

static void yzeatasmblock(void);
static void yzeatcomment(void);

static int yzwrap(void)
{
	return 1;
}

%}
%option never-interactive
%x DECLSPEC
%x PRAGMA
%%

"auto"					{ return AUTO; }
"break"					{ return BREAK; }
"case"					{ return CASE; }
"char"					{ return CHAR; }
"const"					{ return CONST; }
"__const"				{ return CONST; }
"__ptr32"				{ return MSVC_PTR32; }
"__ptr64"				{ return MSVC_PTR64; }
"continue"				{ return CONTINUE; }
"default"				{ return DEFAULT; }
"do"					{ return DO; }
"double"				{ return DOUBLE; }
"else"					{ return ELSE; }
"enum"					{ return ENUM; }
"extern"				{ return EXTERN; }
"float"					{ return FLOAT; }
"for"					{ return FOR; }
"goto"					{ return GOTO; }
"if"					{ return IF; }
"inline"				{ return USEASM_INLINE; }
"int"					{ return INT; }
"long"					{ return LONG; }
"register"				{ return REGISTER; }
"restrict"				{ return RESTRICT; }
"__restrict"			{ return RESTRICT; }
"return"				{ return RETURN; }
"short"					{ return SHORT; }
"signed"				{ return SIGNED; }
"sizeof"				{ return SIZEOF; }
"static"				{ return STATIC; }
"struct"				{ return STRUCT; }
"switch"				{ return SWITCH; }
"typedef"				{ return TYPEDEF; }
"union"					{ return UNION; }
"unsigned"				{ return UNSIGNED; }
"void"					{ return VOID; }
"volatile"				{ return VOLATILE; }
"while"					{ return WHILE; }
"_Bool"					{ return BOOL; }
"bool"					{ return BOOL; }
"__int8"				{ return MSVC__INT8; }
"__int16"				{ return MSVC__INT16; }
"__int32"				{ return MSVC__INT32; }
"__int64"				{ return MSVC__INT64; }
"__cdecl"				{ return MSVC__CDECL; }
"__stdcall"				{ return MSVC__STDCALL; }
"__inline"				{ return __INLINE; }
"__forceinline"			{ return __FORCEINLINE; }
"_fastcall"				{ return MSVC_FASTCALL; }
"__w64"					{ return MSVC__W64; }
"__asm"					{ yzeatasmblock(); return __ASM; }
"_asm"					{ yzeatasmblock(); return __ASM; }
"_inline"				{ return _INLINE; }
"_cdecl"				{ return MSVC_CDECL; }
"__extension__"			{ /* GCC extension marker: can be safely ignored. */ }
"__declspec"			{ BEGIN(DECLSPEC); return MSVC__DECLSPEC; }
"__attribute__"			{ return GCC__ATTRIBUTE__; }
"__builtin_va_list"		{ return GCC__BUILTIN_VA_LIST; }

<DECLSPEC>{
"("						{ g_dwYzDeclSpecBracketCount++; return yztext[0]; }
")"						{ g_dwYzDeclSpecBracketCount--; 
						  if (g_dwYzDeclSpecBracketCount == 0) 
						  {
							BEGIN(INITIAL);
						  }
						  return yztext[0]; }
"allocate"				{ return MSVC__DECLSPEC_ALLOCATE; }
"dllimport"				{ return MSVC__DECLSPEC_DLLIMPORT; }
"dllexport"				{ return MSVC__DECLSPEC_DLLEXPORT; }
"naked"					{ return MSVC__DECLSPEC_NAKED; }
"noreturn"				{ return MSVC__DECLSPEC_NORETURN; }
"nothrow"				{ return MSVC__DECLSPEC_NOTHROW; }
"novtable"				{ return MSVC__DECLSPEC_NOVTABLE; }
"property"				{ return MSVC__DECLSPEC_PROPERTY; }
"selectany"				{ return MSVC__DECLSPEC_SELECTANY; }
"thread"				{ return MSVC__DECLSPEC_THREAD; }
"uuid"					{ return MSVC__DECLSPEC_UUID; }
"put"					{ return MSVC__DECLSPEC_PROPERTY_PUT; }
"get"					{ return MSVC__DECLSPEC_PROPERTY_GET; }
"align"					{ return MSVC__DECLSPEC_ALIGN; }
"deprecated"			{ return MSVC__DECLSPEC_DEPRECATED; }
"restrict"				{ return MSVC__DECLSPEC_RESTRICT; }
"noalias"				{ return MSVC__DECLSPEC_NOALIAS; }

"0x"[0-9A-Fa-f]+U?L?L?		{ yzlval.n = strtoul(yztext + 2, NULL, 16); return INTEGER_NUMBER; }
"0"[0-7]+				{ yzlval.n = strtoul(yztext + 1, NULL, 8); return INTEGER_NUMBER; }
[1-9][0-9]*U?L?L?		{ yzlval.n = strtoul(yztext, NULL, 10); return INTEGER_NUMBER; }
"0"						{ yzlval.n = 0; return INTEGER_NUMBER; }
"\""[^\"]*"\""			{ return STRING_LITERAL; }

[ \n\f\t\r]				{ }
.						{ return yztext[0]; }
}

"__pragma"				{ BEGIN(PRAGMA); }

<PRAGMA>{
"("						{ g_dwYzPragmaBracketCount++; }
")"						{ g_dwYzPragmaBracketCount--; 
						  if (g_dwYzPragmaBracketCount == 0) 
						  {
							BEGIN(INITIAL);
						  }
						}
[ \n\f\t\r]				{ }
.						{ }
}

"--"					{ return DECREMENT; }
"++"					{ return INCREMENT; }
"->"					{ return STRUCTPTRACCESS; }
"*="					{ return MULTIPLYASSIGNMENT; }
"/="					{ return DIVIDEASSIGNMENT; }
"%="					{ return MODULUSASSIGNMENT; }
"+="					{ return ADDASSIGNMENT; }
"-="					{ return SUBASSIGNMENT; }
"<<="					{ return SHIFTLEFTASSIGNMENT; }
">>="					{ return SHIFTRIGHTASSIGNMENT; }
"&="					{ return BITWISEANDASSIGNMENT; }
"^="					{ return BITWISEXORASSIGNMENT; }
"|="					{ return BITWISEORASSIGNMENT; }
"&&"					{ return LOGICALAND; }
"||"					{ return LOGICALOR; }
"<<"					{ return LEFTSHIFT; }
">>"					{ return RIGHTSHIFT; }
"=="					{ return EQUALITY; }
"<="					{ return LESSTHANOREQUAL; }
">="					{ return GREATERTHANOREQUAL; }
"!="					{ return NOTEQUAL; }

"0x"[0-9A-Fa-f]+U?L?L?		{ yzlval.n = strtoul(yztext + 2, NULL, 16); return INTEGER_NUMBER; }
"0"[0-7]+				{ yzlval.n = strtoul(yztext + 1, NULL, 8); return INTEGER_NUMBER; }
[1-9][0-9]*U?L?L?		{ yzlval.n = strtoul(yztext, NULL, 10); return INTEGER_NUMBER; }
"0"						{ yzlval.n = 0; return INTEGER_NUMBER; }

"\""(.|\n)*"\""			{ return STRING_LITERAL; }

"/*"					{ yzeatcomment();  }
"//".*"\n"				{ g_uYzSourceLine++; }

[A-Za-z_][A-Za-z_0-9]*	{ 
							yzlval.pszString = UseAsm_Malloc(yzleng + 1);
							UseAsm_MemCopy(yzlval.pszString, yztext, yzleng);
							yzlval.pszString[yzleng] = '\0';
							if (yzdebug)
							{
								fprintf(stderr, "Reading identifier: <%s> at line %u", yzlval.pszString, g_uYzSourceLine);
							}
							if (CTreeIsTypedefName(yzlval.pszString)) 
							{
								return TYPEDEF_NAME;
							}
							else 
							{
								return IDENTIFIER; 
							}
						}

[ \r\f\t]				{ }
"\n"					{ g_uYzSourceLine++; }

^[ \f\t]*"#".*"\n"		{ ParseHashLine(yytext, yyleng); }

.						{ return yztext[0]; }

%%

static void yzeatasmblock(void)
{
	char c;
	int block_count = 0;
	int nonwhitespace = 0;

	for (;;)
	{
		c = input();
		if (c == '{')
		{
			block_count++;
		}
		else if (c == '}')
		{
			if (block_count == 0)
			{
				unput(c);
				return;
			}
			block_count--;
			if (block_count == 0)
			{
				return;
			}
		}
		else if (c == '\n')
		{
			g_uYzSourceLine++;
			if (block_count == 0 && nonwhitespace)
			{
				return;
			}
		}
		else if (!isspace(c))
		{
			nonwhitespace = 1;
		}
	}
}

static void yzeatcomment(void)
{
	char c, c1;

loop:
	while ((c = input()) != '*' && c != 0)
	{
		if (c == '\n')
		{
			g_uYzSourceLine++;
		}
	}

	if ((c1 = input()) != '/' && c != 0)
	{
		unput(c1);
		goto loop;
	}
}
