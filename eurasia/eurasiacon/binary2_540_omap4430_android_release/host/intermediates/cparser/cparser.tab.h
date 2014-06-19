
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     SWITCH = 258,
     SIGNED = 259,
     MSVC__W64 = 260,
     VOID = 261,
     SIZEOF = 262,
     WHILE = 263,
     UNSIGNED = 264,
     DO = 265,
     EXTERN = 266,
     DOUBLE = 267,
     VOLATILE = 268,
     MSVC_PTR32 = 269,
     MSVC_PTR64 = 270,
     STATIC = 271,
     ENUM = 272,
     CONST = 273,
     AUTO = 274,
     UNION = 275,
     FLOAT = 276,
     RESTRICT = 277,
     RETURN = 278,
     TYPEDEF = 279,
     IF = 280,
     CONTINUE = 281,
     CHAR = 282,
     DEFAULT = 283,
     USEASM_INLINE = 284,
     CASE = 285,
     STRUCT = 286,
     BREAK = 287,
     SHORT = 288,
     INT = 289,
     LONG = 290,
     FOR = 291,
     ELSE = 292,
     GOTO = 293,
     REGISTER = 294,
     STRING_LITERAL = 295,
     INTEGER_NUMBER = 296,
     IDENTIFIER = 297,
     FLOAT_NUMBER = 298,
     TYPEDEF_NAME = 299,
     MSVC__INT8 = 300,
     MSVC__INT16 = 301,
     MSVC__INT32 = 302,
     MSVC__INT64 = 303,
     INCREMENT = 304,
     DECREMENT = 305,
     STRUCTPTRACCESS = 306,
     MULTIPLYASSIGNMENT = 307,
     DIVIDEASSIGNMENT = 308,
     MODULUSASSIGNMENT = 309,
     ADDASSIGNMENT = 310,
     SUBASSIGNMENT = 311,
     SHIFTLEFTASSIGNMENT = 312,
     SHIFTRIGHTASSIGNMENT = 313,
     BITWISEANDASSIGNMENT = 314,
     BITWISEXORASSIGNMENT = 315,
     BITWISEORASSIGNMENT = 316,
     LOGICALOR = 317,
     LOGICALAND = 318,
     LEFTSHIFT = 319,
     RIGHTSHIFT = 320,
     EQUALITY = 321,
     LESSTHANOREQUAL = 322,
     GREATERTHANOREQUAL = 323,
     NOTEQUAL = 324,
     MSVC__CDECL = 325,
     MSVC__STDCALL = 326,
     __INLINE = 327,
     __FORCEINLINE = 328,
     MSVC_FASTCALL = 329,
     __ASM = 330,
     _INLINE = 331,
     MSVC__DECLSPEC = 332,
     MSVC__DECLSPEC_ALLOCATE = 333,
     MSVC__DECLSPEC_DLLIMPORT = 334,
     MSVC__DECLSPEC_DLLEXPORT = 335,
     MSVC__DECLSPEC_NAKED = 336,
     MSVC__DECLSPEC_NORETURN = 337,
     MSVC__DECLSPEC_NOTHROW = 338,
     MSVC__DECLSPEC_NOVTABLE = 339,
     MSVC__DECLSPEC_PROPERTY = 340,
     MSVC__DECLSPEC_SELECTANY = 341,
     MSVC__DECLSPEC_THREAD = 342,
     MSVC__DECLSPEC_UUID = 343,
     MSVC__DECLSPEC_PROPERTY_GET = 344,
     MSVC__DECLSPEC_PROPERTY_PUT = 345,
     MSVC__DECLSPEC_ALIGN = 346,
     MSVC__DECLSPEC_DEPRECATED = 347,
     MSVC__DECLSPEC_RESTRICT = 348,
     MSVC__DECLSPEC_NOALIAS = 349,
     MSVC_CDECL = 350,
     GCC__ATTRIBUTE__ = 351,
     GCC__BUILTIN_VA_LIST = 352,
     BOOL = 353
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 195 "tools/intern/useasm/cparser.y"

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



/* Line 1676 of yacc.c  */
#line 171 "eurasiacon/binary2_540_omap4430_android_release/host/intermediates/cparser/cparser.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yzlval;

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif

extern YYLTYPE yzlloc;

