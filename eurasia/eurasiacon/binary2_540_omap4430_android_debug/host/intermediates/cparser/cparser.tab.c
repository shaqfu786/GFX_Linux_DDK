
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 1

/* Substitute the variable and function names.  */
#define yyparse         yzparse
#define yylex           yzlex
#define yyerror         yzerror
#define yylval          yzlval
#define yychar          yzchar
#define yydebug         yzdebug
#define yynerrs         yznerrs
#define yylloc          yzlloc

/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 1 "tools/intern/useasm/cparser.y"

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



/* Line 189 of yacc.c  */
#line 176 "eurasiacon/binary2_540_omap4430_android_debug/host/intermediates/cparser/cparser.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


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

/* Line 214 of yacc.c  */
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



/* Line 214 of yacc.c  */
#line 331 "eurasiacon/binary2_540_omap4430_android_debug/host/intermediates/cparser/cparser.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

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


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 356 "eurasiacon/binary2_540_omap4430_android_debug/host/intermediates/cparser/cparser.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1715

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  123
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  96
/* YYNRULES -- Number of rules.  */
#define YYNRULES  289
/* YYNRULES -- Number of states.  */
#define YYNSTATES  470

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   353

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   115,     2,     2,     2,   117,   111,     2,
     105,   106,   109,   112,   100,   113,   110,   116,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   104,    99,
     118,   101,   119,   122,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   107,     2,   108,   120,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   102,   121,   103,   114,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     7,     9,    11,    13,    18,    19,
      22,    26,    28,    31,    33,    35,    37,    39,    41,    45,
      46,    48,    52,    54,    56,    58,    60,    62,    64,    66,
      68,    70,    72,    74,    76,    78,    80,    82,    84,    86,
      88,    90,    92,    94,    96,    98,   100,   105,   111,   118,
     121,   123,   125,   127,   129,   131,   134,   138,   139,   141,
     144,   147,   148,   150,   152,   156,   158,   162,   165,   170,
     176,   182,   189,   192,   194,   198,   200,   204,   206,   208,
     210,   212,   214,   216,   218,   220,   222,   224,   226,   228,
     230,   232,   234,   239,   244,   246,   248,   250,   252,   254,
     256,   261,   263,   265,   270,   275,   277,   282,   284,   286,
     288,   291,   295,   299,   303,   310,   312,   315,   316,   318,
     320,   323,   325,   331,   332,   335,   337,   341,   343,   345,
     350,   354,   359,   364,   369,   373,   378,   379,   381,   384,
     388,   389,   391,   393,   396,   397,   399,   401,   407,   409,
     413,   416,   419,   421,   425,   428,   429,   431,   433,   436,
     437,   439,   441,   446,   451,   456,   457,   459,   461,   465,
     470,   473,   478,   479,   481,   484,   486,   489,   495,   497,
     499,   501,   503,   505,   507,   511,   516,   520,   524,   525,
     527,   529,   532,   534,   536,   538,   541,   547,   555,   561,
     567,   575,   585,   594,   596,   600,   603,   606,   610,   612,
     614,   616,   618,   622,   624,   629,   634,   638,   642,   645,
     648,   655,   663,   664,   666,   668,   672,   674,   677,   680,
     683,   686,   691,   693,   695,   697,   699,   701,   703,   705,
     710,   712,   716,   720,   724,   726,   730,   734,   736,   740,
     744,   746,   750,   754,   758,   762,   764,   768,   772,   774,
     778,   780,   784,   786,   790,   792,   796,   798,   802,   804,
     810,   812,   816,   818,   820,   822,   824,   826,   828,   830,
     832,   834,   836,   838,   840,   844,   845,   847,   849,   851
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     124,     0,    -1,    -1,   124,   125,    -1,   126,    -1,   128,
      -1,    99,    -1,   129,   155,   127,   187,    -1,    -1,   127,
     128,    -1,   129,   131,    99,    -1,   130,    -1,   130,   129,
      -1,   133,    -1,   134,    -1,   148,    -1,   149,    -1,   132,
      -1,   131,   100,   132,    -1,    -1,   155,    -1,   155,   101,
     179,    -1,    24,    -1,    11,    -1,    16,    -1,    19,    -1,
      39,    -1,     6,    -1,    27,    -1,    33,    -1,    34,    -1,
      35,    -1,    21,    -1,    12,    -1,    45,    -1,    46,    -1,
      47,    -1,    48,    -1,     4,    -1,     5,    -1,     9,    -1,
     135,    -1,   145,    -1,    44,    -1,    97,    -1,    98,    -1,
     137,   102,   138,   103,    -1,   137,   136,   102,   138,   103,
      -1,   137,   151,   136,   102,   138,   103,    -1,   137,   136,
      -1,    42,    -1,    44,    -1,    31,    -1,    20,    -1,   139,
      -1,   138,   139,    -1,   141,   142,    99,    -1,    -1,   141,
      -1,   134,   140,    -1,   148,   140,    -1,    -1,   143,    -1,
     144,    -1,   143,   100,   144,    -1,   155,    -1,   155,   104,
     217,    -1,   104,   217,    -1,    17,   102,   146,   103,    -1,
      17,    42,   102,   146,   103,    -1,    17,   102,   146,   100,
     103,    -1,    17,    42,   102,   146,   100,   103,    -1,    17,
      42,    -1,   147,    -1,   146,   100,   147,    -1,    42,    -1,
      42,   101,   217,    -1,    18,    -1,    22,    -1,    13,    -1,
     150,    -1,    29,    -1,    72,    -1,    76,    -1,    73,    -1,
      70,    -1,    95,    -1,    71,    -1,    74,    -1,    14,    -1,
      15,    -1,   151,    -1,    77,   105,   152,   106,    -1,    78,
     105,   218,   106,    -1,    79,    -1,    80,    -1,    81,    -1,
      82,    -1,    83,    -1,    84,    -1,    85,   105,   153,   106,
      -1,    86,    -1,    87,    -1,    88,   105,   218,   106,    -1,
      91,   105,   213,   106,    -1,    92,    -1,    92,   105,   218,
     106,    -1,    93,    -1,    94,    -1,   154,    -1,   153,   154,
      -1,    90,   101,    42,    -1,    89,   101,    42,    -1,   164,
     163,   158,    -1,    96,   105,   105,   159,   106,   106,    -1,
     156,    -1,   156,   157,    -1,    -1,   157,    -1,   160,    -1,
     159,   160,    -1,    42,    -1,    42,   105,    42,   161,   106,
      -1,    -1,   100,   162,    -1,   213,    -1,   162,   100,   213,
      -1,    42,    -1,    44,    -1,   105,   166,   155,   106,    -1,
     163,   107,   108,    -1,   163,   107,   213,   108,    -1,   163,
     107,   109,   108,    -1,   163,   105,   169,   106,    -1,   163,
     105,   106,    -1,   163,   105,   172,   106,    -1,    -1,   165,
      -1,   109,   166,    -1,   109,   166,   165,    -1,    -1,   167,
      -1,   148,    -1,   167,   148,    -1,    -1,   169,    -1,   170,
      -1,   170,   100,   110,   110,   110,    -1,   171,    -1,   170,
     100,   171,    -1,   129,   155,    -1,   129,   174,    -1,    42,
      -1,   172,   100,    42,    -1,   141,   174,    -1,    -1,   175,
      -1,   165,    -1,   164,   177,    -1,    -1,   109,    -1,   213,
      -1,   105,   166,   175,   106,    -1,   178,   107,   176,   108,
      -1,   178,   105,   168,   106,    -1,    -1,   177,    -1,   213,
      -1,   102,   180,   103,    -1,   102,   180,   100,   103,    -1,
     181,   179,    -1,   180,   100,   181,   179,    -1,    -1,   182,
      -1,   183,   101,    -1,   184,    -1,   183,   184,    -1,   107,
     217,   108,   110,    42,    -1,   186,    -1,   187,    -1,   191,
      -1,   192,    -1,   193,    -1,   194,    -1,    42,   104,   185,
      -1,    30,   217,   104,   185,    -1,    28,   104,   185,    -1,
     102,   188,   103,    -1,    -1,   189,    -1,   190,    -1,   189,
     190,    -1,   128,    -1,   185,    -1,    99,    -1,   215,    99,
      -1,    25,   105,   215,   106,   185,    -1,    25,   105,   215,
     106,   185,    37,   185,    -1,     3,   105,   215,   106,   185,
      -1,     8,   105,   215,   106,   185,    -1,    10,   185,     8,
     105,   215,   106,    99,    -1,    36,   105,   216,    99,   216,
      99,   216,   106,   185,    -1,    36,   105,   128,   216,    99,
     216,   106,   185,    -1,    75,    -1,    38,    42,    99,    -1,
      26,    99,    -1,    32,    99,    -1,    23,   216,    99,    -1,
      42,    -1,    41,    -1,    43,    -1,   218,    -1,   105,   215,
     106,    -1,   195,    -1,   196,   107,   215,   108,    -1,   196,
     105,   197,   106,    -1,   196,   110,   136,    -1,   196,    51,
     136,    -1,   196,    49,    -1,   196,    50,    -1,   105,   173,
     106,   102,   180,   103,    -1,   105,   173,   106,   102,   180,
     100,   103,    -1,    -1,   198,    -1,   213,    -1,   198,   100,
     213,    -1,   196,    -1,    49,   199,    -1,    50,   199,    -1,
     200,   201,    -1,     7,   199,    -1,     7,   105,   173,   106,
      -1,   111,    -1,   109,    -1,   112,    -1,   113,    -1,   114,
      -1,   115,    -1,   199,    -1,   105,   173,   106,   201,    -1,
     201,    -1,   202,   109,   201,    -1,   202,   116,   201,    -1,
     202,   117,   201,    -1,   202,    -1,   203,   112,   202,    -1,
     203,   113,   202,    -1,   203,    -1,   204,    64,   203,    -1,
     204,    65,   203,    -1,   204,    -1,   205,   118,   204,    -1,
     205,   119,   204,    -1,   205,    67,   204,    -1,   205,    68,
     204,    -1,   205,    -1,   206,    66,   205,    -1,   206,    69,
     205,    -1,   206,    -1,   207,   111,   206,    -1,   207,    -1,
     208,   120,   207,    -1,   208,    -1,   209,   121,   208,    -1,
     209,    -1,   210,    63,   209,    -1,   210,    -1,   211,    62,
     210,    -1,   211,    -1,   211,   122,   215,   104,   212,    -1,
     212,    -1,   199,   214,   213,    -1,   101,    -1,    52,    -1,
      53,    -1,    54,    -1,    55,    -1,    56,    -1,    57,    -1,
      58,    -1,    59,    -1,    60,    -1,    61,    -1,   213,    -1,
     215,   100,   213,    -1,    -1,   215,    -1,   212,    -1,    40,
      -1,   218,    40,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   294,   294,   296,   300,   304,   308,   315,   318,   320,
     324,   333,   338,   346,   351,   356,   361,   369,   373,   381,
     384,   390,   399,   406,   413,   420,   427,   437,   444,   451,
     458,   465,   472,   479,   486,   493,   500,   507,   514,   521,
     528,   535,   543,   551,   559,   566,   576,   584,   592,   600,
     610,   614,   621,   627,   636,   640,   647,   657,   662,   669,
     674,   683,   688,   695,   700,   707,   712,   719,   729,   737,
     745,   753,   761,   772,   776,   783,   791,   802,   809,   816,
     823,   827,   834,   841,   848,   859,   866,   873,   880,   887,
     894,   901,   909,   919,   920,   921,   922,   923,   924,   925,
     926,   927,   928,   929,   930,   931,   932,   933,   934,   938,
     939,   943,   944,   948,   957,   962,   963,   968,   970,   976,
     977,   981,   982,   985,   987,   991,   992,   996,  1004,  1012,
    1019,  1026,  1034,  1041,  1048,  1055,  1066,  1069,  1076,  1083,
    1097,  1100,  1107,  1111,  1117,  1119,  1123,  1124,  1128,  1129,
    1133,  1134,  1138,  1139,  1143,  1152,  1155,  1162,  1169,  1187,
    1190,  1194,  1201,  1208,  1222,  1232,  1235,  1242,  1244,  1246,
    1251,  1252,  1255,  1257,  1261,  1265,  1266,  1270,  1275,  1276,
    1277,  1278,  1279,  1280,  1284,  1285,  1286,  1290,  1293,  1295,
    1299,  1300,  1304,  1305,  1309,  1310,  1314,  1315,  1316,  1320,
    1321,  1322,  1323,  1324,  1328,  1329,  1330,  1331,  1335,  1342,
    1349,  1356,  1363,  1370,  1372,  1378,  1384,  1390,  1396,  1401,
    1406,  1410,  1418,  1419,  1424,  1426,  1431,  1435,  1440,  1445,
    1450,  1457,  1467,  1469,  1471,  1473,  1475,  1477,  1482,  1486,
    1494,  1498,  1504,  1510,  1519,  1523,  1529,  1538,  1542,  1548,
    1557,  1561,  1567,  1573,  1579,  1588,  1592,  1598,  1607,  1611,
    1620,  1624,  1633,  1637,  1646,  1650,  1659,  1663,  1672,  1676,
    1686,  1690,  1699,  1701,  1703,  1705,  1707,  1709,  1711,  1713,
    1715,  1717,  1719,  1724,  1728,  1738,  1739,  1744,  1749,  1750
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "SWITCH", "SIGNED", "MSVC__W64", "VOID",
  "SIZEOF", "WHILE", "UNSIGNED", "DO", "EXTERN", "DOUBLE", "VOLATILE",
  "MSVC_PTR32", "MSVC_PTR64", "STATIC", "ENUM", "CONST", "AUTO", "UNION",
  "FLOAT", "RESTRICT", "RETURN", "TYPEDEF", "IF", "CONTINUE", "CHAR",
  "DEFAULT", "USEASM_INLINE", "CASE", "STRUCT", "BREAK", "SHORT", "INT",
  "LONG", "FOR", "ELSE", "GOTO", "REGISTER", "STRING_LITERAL",
  "INTEGER_NUMBER", "IDENTIFIER", "FLOAT_NUMBER", "TYPEDEF_NAME",
  "MSVC__INT8", "MSVC__INT16", "MSVC__INT32", "MSVC__INT64", "INCREMENT",
  "DECREMENT", "STRUCTPTRACCESS", "MULTIPLYASSIGNMENT", "DIVIDEASSIGNMENT",
  "MODULUSASSIGNMENT", "ADDASSIGNMENT", "SUBASSIGNMENT",
  "SHIFTLEFTASSIGNMENT", "SHIFTRIGHTASSIGNMENT", "BITWISEANDASSIGNMENT",
  "BITWISEXORASSIGNMENT", "BITWISEORASSIGNMENT", "LOGICALOR", "LOGICALAND",
  "LEFTSHIFT", "RIGHTSHIFT", "EQUALITY", "LESSTHANOREQUAL",
  "GREATERTHANOREQUAL", "NOTEQUAL", "MSVC__CDECL", "MSVC__STDCALL",
  "__INLINE", "__FORCEINLINE", "MSVC_FASTCALL", "__ASM", "_INLINE",
  "MSVC__DECLSPEC", "MSVC__DECLSPEC_ALLOCATE", "MSVC__DECLSPEC_DLLIMPORT",
  "MSVC__DECLSPEC_DLLEXPORT", "MSVC__DECLSPEC_NAKED",
  "MSVC__DECLSPEC_NORETURN", "MSVC__DECLSPEC_NOTHROW",
  "MSVC__DECLSPEC_NOVTABLE", "MSVC__DECLSPEC_PROPERTY",
  "MSVC__DECLSPEC_SELECTANY", "MSVC__DECLSPEC_THREAD",
  "MSVC__DECLSPEC_UUID", "MSVC__DECLSPEC_PROPERTY_GET",
  "MSVC__DECLSPEC_PROPERTY_PUT", "MSVC__DECLSPEC_ALIGN",
  "MSVC__DECLSPEC_DEPRECATED", "MSVC__DECLSPEC_RESTRICT",
  "MSVC__DECLSPEC_NOALIAS", "MSVC_CDECL", "GCC__ATTRIBUTE__",
  "GCC__BUILTIN_VA_LIST", "BOOL", "';'", "','", "'='", "'{'", "'}'", "':'",
  "'('", "')'", "'['", "']'", "'*'", "'.'", "'&'", "'+'", "'-'", "'~'",
  "'!'", "'/'", "'%'", "'<'", "'>'", "'^'", "'|'", "'?'", "$accept",
  "program", "extern_declaration", "function_definition",
  "declaration_list", "declaration", "declaration_specifiers",
  "declaration_specifier", "init_declarator_list", "init_declarator",
  "storage_class_specifier", "type_specifier", "struct_or_union_specifier",
  "identifier_or_typedef_name", "struct_or_union",
  "struct_declaration_list", "struct_declaration",
  "specifier_qualifier_list_opt", "specifier_qualifier_list",
  "struct_declarator_list_opt", "struct_declarator_list",
  "struct_declarator", "enum_specifier", "enumerator_list", "enumerator",
  "type_qualifier", "function_specifier", "microsoft_modifier", "declspec",
  "microsoft_declspec", "microsoft_declspec_property_list",
  "microsoft_declspec_property", "declarator", "gcc_attribute",
  "gcc_attribute_list", "attribute_opt", "attribute_specifier_list",
  "attribute_specifier", "attribute_specifier_parameter_list_opt",
  "attribute_specifier_parameter_list", "direct_declarator", "pointer_opt",
  "pointer", "type_qualifier_list_opt", "type_qualifier_list",
  "parameter_type_list_opt", "parameter_type_list", "parameter_list",
  "parameter_declaration", "identifier_list", "type_name",
  "abstract_declarator_opt", "abstract_declarator",
  "direct_abstract_declaractor_array_size", "direct_abstract_declarator",
  "direct_abstract_declarator_opt", "initializer", "initializer_list",
  "designation_opt", "designation", "designator_list", "designator",
  "statement", "labeled_statement", "compound_statement",
  "block_item_list_opt", "block_item_list", "block_item",
  "expression_statement", "selection_statement", "iteration_statement",
  "jump_statement", "primary_expression", "postfix_expression",
  "argument_expression_list_opt", "argument_expression_list",
  "unary_expression", "unary_operator", "cast_expression",
  "multiplicative_expression", "additive_expression", "shift_expression",
  "relational_expression", "equality_expression", "and_expression",
  "exclusive_or_expression", "inclusive_or_expression",
  "logical_and_expression", "logical_or_expression",
  "conditional_expression", "assignment_expression", "assignment_operator",
  "expression", "expression_opt", "constant_expression", "string_literal", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,    59,
      44,    61,   123,   125,    58,    40,    41,    91,    93,    42,
      46,    38,    43,    45,   126,    33,    47,    37,    60,    62,
      94,   124,    63
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   123,   124,   124,   125,   125,   125,   126,   127,   127,
     128,   129,   129,   130,   130,   130,   130,   131,   131,   132,
     132,   132,   133,   133,   133,   133,   133,   134,   134,   134,
     134,   134,   134,   134,   134,   134,   134,   134,   134,   134,
     134,   134,   134,   134,   134,   134,   135,   135,   135,   135,
     136,   136,   137,   137,   138,   138,   139,   140,   140,   141,
     141,   142,   142,   143,   143,   144,   144,   144,   145,   145,
     145,   145,   145,   146,   146,   147,   147,   148,   148,   148,
     148,   149,   149,   149,   149,   150,   150,   150,   150,   150,
     150,   150,   151,   152,   152,   152,   152,   152,   152,   152,
     152,   152,   152,   152,   152,   152,   152,   152,   152,   153,
     153,   154,   154,   155,   156,   157,   157,   158,   158,   159,
     159,   160,   160,   161,   161,   162,   162,   163,   163,   163,
     163,   163,   163,   163,   163,   163,   164,   164,   165,   165,
     166,   166,   167,   167,   168,   168,   169,   169,   170,   170,
     171,   171,   172,   172,   173,   174,   174,   175,   175,   176,
     176,   176,   177,   177,   177,   178,   178,   179,   179,   179,
     180,   180,   181,   181,   182,   183,   183,   184,   185,   185,
     185,   185,   185,   185,   186,   186,   186,   187,   188,   188,
     189,   189,   190,   190,   191,   191,   192,   192,   192,   193,
     193,   193,   193,   193,   194,   194,   194,   194,   195,   195,
     195,   195,   195,   196,   196,   196,   196,   196,   196,   196,
     196,   196,   197,   197,   198,   198,   199,   199,   199,   199,
     199,   199,   200,   200,   200,   200,   200,   200,   201,   201,
     202,   202,   202,   202,   203,   203,   203,   204,   204,   204,
     205,   205,   205,   205,   205,   206,   206,   206,   207,   207,
     208,   208,   209,   209,   210,   210,   211,   211,   212,   212,
     213,   213,   214,   214,   214,   214,   214,   214,   214,   214,
     214,   214,   214,   215,   215,   216,   216,   217,   218,   218
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     4,     0,     2,
       3,     1,     2,     1,     1,     1,     1,     1,     3,     0,
       1,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     4,     5,     6,     2,
       1,     1,     1,     1,     1,     2,     3,     0,     1,     2,
       2,     0,     1,     1,     3,     1,     3,     2,     4,     5,
       5,     6,     2,     1,     3,     1,     3,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     4,     4,     1,     1,     1,     1,     1,     1,
       4,     1,     1,     4,     4,     1,     4,     1,     1,     1,
       2,     3,     3,     3,     6,     1,     2,     0,     1,     1,
       2,     1,     5,     0,     2,     1,     3,     1,     1,     4,
       3,     4,     4,     4,     3,     4,     0,     1,     2,     3,
       0,     1,     1,     2,     0,     1,     1,     5,     1,     3,
       2,     2,     1,     3,     2,     0,     1,     1,     2,     0,
       1,     1,     4,     4,     4,     0,     1,     1,     3,     4,
       2,     4,     0,     1,     2,     1,     2,     5,     1,     1,
       1,     1,     1,     1,     3,     4,     3,     3,     0,     1,
       1,     2,     1,     1,     1,     2,     5,     7,     5,     5,
       7,     9,     8,     1,     3,     2,     2,     3,     1,     1,
       1,     1,     3,     1,     4,     4,     3,     3,     2,     2,
       6,     7,     0,     1,     1,     3,     1,     2,     2,     2,
       2,     4,     1,     1,     1,     1,     1,     1,     1,     4,
       1,     3,     3,     3,     1,     3,     3,     1,     3,     3,
       1,     3,     3,     3,     3,     1,     3,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     5,
       1,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     0,     1,     1,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       2,     0,     1,    38,    39,    27,    40,    23,    33,    79,
      89,    90,    24,     0,    77,    25,    53,    32,    78,    22,
      28,    81,    52,    29,    30,    31,    26,    43,    34,    35,
      36,    37,    85,    87,    82,    84,    88,    83,     0,    86,
      44,    45,     6,     3,     4,     5,   136,    11,    13,    14,
      41,     0,    42,    15,    16,    80,    91,    72,     0,     0,
     140,     0,    17,     8,     0,   137,    12,    50,    51,     0,
      49,     0,     0,    75,     0,    73,     0,    94,    95,    96,
      97,    98,    99,     0,   101,   102,     0,     0,   105,   107,
     108,     0,   142,   138,   141,    10,   136,     0,     0,   127,
     128,   140,   117,    57,     0,    54,   136,    57,     0,     0,
       0,     0,     0,    68,     0,     0,     0,     0,     0,    92,
     139,   143,    18,    20,     0,   288,   209,   208,   210,     0,
       0,   172,     0,   233,   232,   234,   235,   236,   237,    21,
     213,   226,   238,     0,   240,   244,   247,   250,   255,   258,
     260,   262,   264,   266,   268,   270,   167,   211,   188,     9,
     136,     7,   136,     0,     0,     0,   115,   118,   113,    59,
      58,    46,    55,     0,     0,    62,    63,    65,    60,     0,
       0,     0,    69,   238,   287,    76,    70,    74,     0,     0,
       0,     0,   109,     0,     0,     0,     0,   230,     0,   227,
     228,     0,     0,     0,   173,     0,   175,   136,     0,   283,
       0,   218,   219,     0,   222,     0,     0,   273,   274,   275,
     276,   277,   278,   279,   280,   281,   282,   272,     0,   229,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   289,
       0,     0,     0,   285,     0,     0,     0,     0,     0,     0,
       0,   208,   203,   194,   192,   193,   178,   179,     0,   189,
     190,   180,   181,   182,   183,     0,     0,     0,   152,   134,
     136,     0,   146,   148,     0,   130,   233,     0,   116,    67,
      56,   136,     0,    47,     0,    71,    93,     0,     0,   100,
     110,   103,   104,   106,     0,     0,     0,   172,   168,   170,
     174,   176,   165,   137,   154,   156,     0,     0,   212,   217,
       0,   223,   224,     0,   216,   271,   241,   242,   243,   245,
     246,   248,   249,   253,   254,   251,   252,   256,   257,   259,
     261,   263,   265,   267,     0,     0,     0,     0,   286,     0,
       0,   205,     0,     0,   206,   285,     0,     0,   187,   191,
     195,   129,     0,   150,   165,   151,   133,     0,     0,   135,
     132,   131,    64,    66,    48,   112,   111,   231,     0,     0,
     169,     0,   140,   158,     0,   172,   239,   284,   215,     0,
     214,     0,     0,     0,     0,   207,     0,   186,     0,   285,
       0,   204,   184,   121,     0,   119,   140,     0,   149,   153,
       0,   171,   136,   144,   159,     0,   225,   269,     0,     0,
       0,     0,   185,     0,   285,     0,     0,   120,   136,     0,
     177,     0,     0,   145,   233,     0,   161,   172,   220,   198,
     199,     0,   196,   285,     0,   123,   114,   147,   162,   164,
     163,   221,     0,     0,     0,   285,     0,     0,   200,   197,
       0,     0,   124,   125,   122,   202,     0,     0,   201,   126
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    43,    44,    98,   264,   160,    47,    61,    62,
      48,   103,    50,    70,    51,   104,   105,   169,   106,   174,
     175,   176,    52,    74,    75,   107,    54,    55,    56,    91,
     191,   192,   123,   166,   167,   168,   404,   405,   457,   462,
     102,    64,    65,    93,    94,   432,   281,   282,   283,   284,
     208,   314,   315,   435,   383,   384,   139,   202,   203,   204,
     205,   206,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   140,   141,   320,   321,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     209,   228,   275,   349,   185,   157
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -384
static const yytype_int16 yypact[] =
{
    -384,  1112,  -384,  -384,  -384,  -384,  -384,  -384,  -384,  -384,
    -384,  -384,  -384,     2,  -384,  -384,  -384,  -384,  -384,  -384,
    -384,  -384,  -384,  -384,  -384,  -384,  -384,  -384,  -384,  -384,
    -384,  -384,  -384,  -384,  -384,  -384,  -384,  -384,   -49,  -384,
    -384,  -384,  -384,  -384,  -384,  -384,    37,  1530,  -384,  -384,
    -384,    32,  -384,  -384,  -384,  -384,  -384,   -37,    46,   814,
     287,   135,  -384,   140,    -9,  -384,  -384,  -384,  -384,  1617,
      10,   -10,    46,   -22,   -23,  -384,    27,  -384,  -384,  -384,
    -384,  -384,  -384,    94,  -384,  -384,   101,   109,   117,  -384,
    -384,    36,  -384,    43,   287,  -384,    37,   652,  1435,  -384,
    -384,   287,    33,  1617,  1208,  -384,   -47,  1617,  1617,    86,
      66,   728,   -16,  -384,   187,   190,   187,   728,   187,  -384,
    -384,  -384,  -384,   153,   765,  -384,  -384,  -384,  -384,   776,
     776,   152,   636,  -384,  -384,  -384,  -384,  -384,  -384,  -384,
    -384,    17,   523,   728,  -384,     9,   169,   222,    83,   157,
     136,   142,   143,   203,   -44,  -384,  -384,   230,   412,  -384,
      37,  -384,    43,   167,  1016,   358,   193,  -384,  -384,  -384,
    -384,  -384,  -384,   728,   184,   194,  -384,   204,  -384,  1253,
    1617,    -2,  -384,  -384,  -384,  -384,  -384,  -384,   -24,   210,
     211,   -48,  -384,   -17,   207,   -12,   636,  -384,   636,  -384,
    -384,   728,   128,   652,  -384,   -70,  -384,   127,   209,  -384,
     -51,  -384,  -384,   -10,   728,   728,   -10,  -384,  -384,  -384,
    -384,  -384,  -384,  -384,  -384,  -384,  -384,  -384,   728,  -384,
     728,   728,   728,   728,   728,   728,   728,   728,   728,   728,
     728,   728,   728,   728,   728,   728,   728,   728,   728,  -384,
     212,   214,   235,   728,   215,   217,   218,   728,   224,   219,
     284,   223,  -384,  -384,  -384,  -384,  -384,  -384,   225,   412,
    -384,  -384,  -384,  -384,  -384,   192,   226,   228,  -384,  -384,
      47,   229,   236,  -384,   -46,  -384,   221,   231,  -384,  -384,
    -384,    87,   728,  -384,  1340,  -384,  -384,   296,   300,  -384,
    -384,  -384,  -384,  -384,   237,   239,   243,     8,  -384,  -384,
    -384,  -384,   247,    39,  -384,  -384,   680,   728,  -384,  -384,
     250,   253,  -384,   -78,  -384,  -384,  -384,  -384,  -384,     9,
       9,   169,   169,   222,   222,   222,   222,    83,    83,   157,
     136,   142,   143,   203,   104,   728,   728,   352,   262,   270,
     728,  -384,   235,   266,  -384,   524,   272,   235,  -384,  -384,
    -384,  -384,   330,  -384,    -6,  -384,  -384,   916,   331,  -384,
    -384,  -384,  -384,  -384,  -384,  -384,  -384,   273,   273,   267,
    -384,   652,   287,    26,   145,   152,  -384,  -384,  -384,   728,
    -384,   728,    49,    54,   271,  -384,    61,  -384,   235,   728,
     279,  -384,  -384,   274,   -15,  -384,   287,   276,  -384,  -384,
     338,  -384,    43,  1530,   804,   146,  -384,  -384,   235,   235,
     728,   235,  -384,   285,   728,   341,   281,  -384,    43,   278,
    -384,   283,   286,  -384,   282,   288,  -384,   108,  -384,  -384,
    -384,    62,   354,   728,   294,   295,  -384,  -384,  -384,  -384,
    -384,  -384,   298,   235,   297,   728,   728,   303,  -384,  -384,
     235,   304,   302,  -384,  -384,  -384,   235,   728,  -384,  -384
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -384,  -384,  -384,  -384,  -384,     4,     1,  -384,  -384,   308,
    -384,     0,  -384,   -18,  -384,   -88,   -94,   299,   -82,  -384,
    -384,   103,  -384,   339,   -97,    -1,  -384,  -384,   398,  -384,
    -384,   277,   -43,  -384,   309,  -384,  -384,    60,  -384,  -384,
    -384,  -199,   -87,   -92,  -384,  -384,    52,  -384,   107,  -384,
      57,   196,  -383,  -384,  -384,  -384,  -191,    92,  -293,  -384,
    -384,   275,  -209,  -384,   380,  -384,  -384,   227,  -384,  -384,
    -384,  -384,  -384,  -384,  -384,  -384,   -60,  -384,  -124,    64,
      68,   -21,    65,   238,   246,   234,   245,   248,  -384,   -98,
     -93,  -384,  -125,  -338,  -162,    89
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -167
static const yytype_int16 yytable[] =
{
      53,    49,    46,    63,   156,    45,   120,   210,   312,   162,
     172,   289,   309,   184,   381,   187,   249,   400,   247,   229,
     179,   170,   317,   249,   194,   170,    73,   403,   249,   431,
     390,   310,    67,    99,    68,   100,    99,   201,   100,   306,
      73,   189,   190,   347,    57,   431,    53,    49,    66,   317,
     207,   183,   -61,   109,   368,   318,    59,   173,   299,    92,
     369,   423,    60,   177,   197,    72,   211,   212,   213,   199,
     200,   210,   287,   210,    67,   184,    68,   112,   248,   111,
     113,   364,   296,   183,   187,   172,   444,   186,    73,   301,
     323,   426,   294,   121,   303,   353,   101,    53,    49,   406,
      92,   295,   159,   184,    58,   454,   326,   327,   328,    38,
     156,   380,   108,   183,   207,   201,   207,   461,   230,   276,
     313,   322,   214,   344,   215,   231,   232,   216,   348,   163,
     373,  -166,   114,  -166,    69,   325,   -19,   -19,   164,  -157,
     165,   183,   119,   397,   381,  -157,    60,  -155,   402,   317,
     237,   238,    60,  -155,   317,   418,    60,    53,    49,   184,
     419,   317,   317,    53,    49,   280,   181,   421,   452,   182,
     183,   183,   183,   183,   183,   183,   183,   183,   183,   183,
     183,   183,   183,   183,   183,   183,   183,   183,   180,   422,
     411,   173,   386,   313,   184,   319,    60,   183,   324,   115,
     172,   239,   240,   188,   317,   193,   116,   195,   391,   439,
     440,   451,   442,   312,   117,   201,   333,   334,   335,   336,
     392,   393,   118,   241,   387,   396,   242,   125,   307,   364,
     348,   308,   183,  -155,    95,    96,    60,   363,   250,   -20,
     -20,    97,   124,   251,   459,   252,   437,   243,   177,   438,
     413,   465,   414,   304,    97,   305,   183,   468,   253,   201,
     254,   255,   244,   256,   245,   257,   246,   258,    53,    49,
     249,   259,   277,   260,   348,   125,   126,   261,   128,   189,
     190,   233,   234,   290,   129,   130,   235,   236,   156,   163,
     412,   360,   317,   417,   291,   441,   416,   329,   330,   348,
       9,    10,    11,   331,   332,    14,   337,   338,   292,    18,
     262,   297,   298,   302,   428,   316,   351,   345,   348,   346,
     350,   436,   352,   354,   355,   313,   356,   357,   358,   370,
     348,   183,   361,   362,   263,   366,   367,   158,   375,   371,
     132,   313,   376,   377,   133,   378,   134,   135,   136,   137,
     138,   379,   382,   389,    53,    49,   388,    32,    33,   399,
     394,    36,   317,   463,    38,   124,    53,    49,   280,   395,
     398,   401,   403,   409,   469,   385,   420,   410,   424,   425,
     430,    92,    39,   445,   443,   276,   429,   446,   447,   448,
    -160,   453,   449,   455,   372,   456,   450,   458,   125,   126,
     127,   128,   467,   460,   122,    92,   178,   129,   130,   464,
     466,   110,    53,    49,   280,   250,     3,     4,     5,   124,
     251,     6,   252,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,   253,    19,   254,   255,    20,
     256,    21,   257,    22,   258,    23,    24,    25,   259,    71,
     260,    26,   125,   126,   261,   128,    27,    28,    29,    30,
      31,   129,   130,   132,   427,   433,   285,   286,   300,   134,
     135,   136,   137,   138,   408,   288,   365,   415,   161,   341,
     311,   339,    32,    33,    34,    35,    36,   262,    37,    38,
     340,   342,     0,     0,     0,   343,   359,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    39,     0,    40,
      41,   263,     0,     0,   158,     0,     0,   132,     0,     0,
       0,   133,     0,   134,   135,   136,   137,   138,     3,     4,
       5,   124,     0,     6,     0,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,     0,    19,     0,
       0,    20,     0,    21,     0,    22,     0,    23,    24,    25,
       0,     0,     0,    26,   125,   126,   127,   128,    27,    28,
      29,    30,    31,   129,   130,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    32,    33,    34,    35,    36,     0,
      37,    38,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    39,
       0,    40,    41,     0,   227,     0,     0,     0,     0,   132,
       0,     0,     0,   133,     0,   134,   135,   136,   137,   138,
       3,     4,     5,   124,     0,     6,     0,     0,     8,     9,
      10,    11,     0,    13,    14,     0,    16,    17,    18,   124,
       0,     0,     0,    20,     0,     0,     0,    22,     0,    23,
      24,    25,     0,     0,     0,     0,   125,   126,   127,   128,
      27,    28,    29,    30,    31,   129,   130,   124,     0,     0,
       0,     0,   125,   126,   127,   128,     0,     0,     0,     0,
       0,   129,   130,     0,     0,     0,    32,    33,     0,     0,
      36,     0,     0,    38,     0,     0,     0,     0,     0,     0,
     125,   126,   127,   128,     0,     0,     0,     0,     0,   129,
     130,    39,     0,    40,    41,   124,     0,     0,     0,     0,
       0,   132,     0,     0,     0,   133,     0,   134,   135,   136,
     137,   138,     0,     0,   131,     0,     0,   132,     0,     0,
       0,   133,     0,   134,   135,   136,   137,   138,   125,   126,
     127,   128,   124,     0,     0,     0,     0,   129,   130,     0,
       0,     0,   385,   124,     0,   132,     0,     0,     0,   133,
       0,   134,   135,   136,   137,   138,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   125,   126,   127,   128,     0,
       0,   124,     0,     0,   129,   130,   125,   126,   127,   128,
       0,     0,     0,     0,     0,   129,   130,     0,     0,     0,
       0,     0,     0,   132,     0,     0,     0,   133,     0,   134,
     135,   136,   137,   138,   125,   126,   127,   128,     0,     0,
       0,     0,     0,   129,   130,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     196,     0,     0,     0,   133,     0,   134,   135,   136,   137,
     138,   198,     0,     0,     0,   133,     0,   134,   135,   136,
     137,   138,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,     0,     0,    87,    88,    89,    90,   132,
       0,     0,     0,   434,     0,   134,   135,   136,   137,   138,
       3,     4,     5,     0,     0,     6,     0,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,     0,
      19,     0,     0,    20,     0,    21,     0,    22,     0,    23,
      24,    25,     0,     0,     0,    26,     0,     0,     0,     0,
      27,    28,    29,    30,    31,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    32,    33,    34,    35,
      36,     0,    37,    38,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    39,     0,    40,    41,     0,     0,     0,     0,     0,
       3,     4,     5,     0,     0,     6,   407,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,     0,
      19,     0,     0,    20,     0,    21,     0,    22,     0,    23,
      24,    25,     0,     0,     0,    26,     0,     0,   278,     0,
      27,    28,    29,    30,    31,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    32,    33,    34,    35,
      36,     0,    37,    38,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    39,     2,    40,    41,     0,     3,     4,     5,     0,
       0,     6,   279,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,     0,    19,     0,     0,    20,
       0,    21,     0,    22,     0,    23,    24,    25,     0,     0,
       0,    26,     0,     0,     0,     0,    27,    28,    29,    30,
      31,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    32,    33,    34,    35,    36,     0,    37,    38,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    39,     0,    40,
      41,    42,     3,     4,     5,     0,     0,     6,     0,     0,
       8,     9,    10,    11,     0,    13,    14,     0,    16,    17,
      18,     0,     0,     0,     0,    20,     0,     0,     0,    22,
       0,    23,    24,    25,     0,     0,     0,     0,     0,     0,
       0,     0,    27,    28,    29,    30,    31,     3,     4,     5,
       0,     0,     6,     0,     0,     8,     9,    10,    11,     0,
      13,    14,     0,    16,    17,    18,     0,     0,    32,    33,
      20,     0,    36,     0,    22,    38,    23,    24,    25,     0,
       0,     0,     0,     0,     0,     0,     0,    27,    28,    29,
      30,    31,     0,    39,     0,    40,    41,     0,     0,     0,
       0,   171,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    32,    33,     0,     0,    36,     0,     0,
      38,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     3,     4,     5,     0,    39,     6,
      40,    41,     8,     9,    10,    11,   293,    13,    14,     0,
      16,    17,    18,     0,     0,     0,     0,    20,     0,     0,
       0,    22,     0,    23,    24,    25,     0,     0,     0,     0,
       0,     0,     0,     0,    27,    28,    29,    30,    31,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      32,    33,     0,     0,    36,     0,     0,    38,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    39,     0,    40,    41,     3,
       4,     5,     0,   374,     6,     0,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,     0,    19,
       0,     0,    20,     0,    21,     0,    22,     0,    23,    24,
      25,     0,     0,     0,    26,     0,     0,     0,     0,    27,
      28,    29,    30,    31,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    32,    33,    34,    35,    36,
       0,    37,    38,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      39,     0,    40,    41,     3,     4,     5,   158,     0,     6,
       0,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,     0,    19,     0,     0,    20,     0,    21,
       0,    22,     0,    23,    24,    25,     0,     0,     0,    26,
       0,     0,     0,     0,    27,    28,    29,    30,    31,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      32,    33,    34,    35,    36,     0,    37,    38,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     3,     4,     5,     0,    39,     6,    40,    41,     8,
       9,    10,    11,     0,    13,    14,     0,    16,    17,    18,
       0,     0,     0,     0,    20,     0,     0,     0,    22,     0,
      23,    24,    25,     0,     0,     0,     0,     0,     0,     0,
       0,    27,    28,    29,    30,    31,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    32,    33,     0,
       0,    36,     0,     0,    38,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    39,     0,    40,    41
};

static const yytype_int16 yycheck[] =
{
       1,     1,     1,    46,    97,     1,    93,   132,   207,   101,
     104,   173,   203,   111,   307,   112,    40,   355,    62,   143,
     108,   103,   100,    40,   117,   107,    42,    42,    40,   412,
     108,   101,    42,    42,    44,    44,    42,   107,    44,   201,
      42,    89,    90,   252,    42,   428,    47,    47,    47,   100,
     132,   111,    99,    71,   100,   106,   105,   104,   106,    60,
     106,   399,   109,   106,   124,   102,    49,    50,    51,   129,
     130,   196,   165,   198,    42,   173,    44,   100,   122,   101,
     103,   280,   106,   143,   181,   179,   424,   103,    42,   106,
     215,   106,   180,    94,   106,   257,   105,    98,    98,   105,
     101,   103,    98,   201,   102,   443,   230,   231,   232,    77,
     203,   103,   102,   173,   196,   107,   198,   455,   109,   162,
     207,   214,   105,   248,   107,   116,   117,   110,   253,    96,
     292,   105,   105,   107,   102,   228,    99,   100,   105,   100,
     107,   201,   106,   352,   437,   106,   109,   100,   357,   100,
      67,    68,   109,   106,   100,   106,   109,   158,   158,   257,
     106,   100,   100,   164,   164,   164,   100,   106,   106,   103,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   243,   244,   245,   246,   247,   102,   398,
     381,   104,   316,   280,   292,   213,   109,   257,   216,   105,
     294,   118,   119,   114,   100,   116,   105,   118,   104,   418,
     419,   103,   421,   412,   105,   107,   237,   238,   239,   240,
     345,   346,   105,    66,   317,   350,    69,    40,   100,   428,
     355,   103,   292,   106,    99,   100,   109,   280,     3,    99,
     100,   101,     7,     8,   453,    10,   100,   111,   291,   103,
     105,   460,   107,   196,   101,   198,   316,   466,    23,   107,
      25,    26,   120,    28,   121,    30,    63,    32,   269,   269,
      40,    36,   105,    38,   399,    40,    41,    42,    43,    89,
      90,   112,   113,    99,    49,    50,    64,    65,   381,    96,
     382,    99,   100,   391,   100,   420,   389,   233,   234,   424,
      13,    14,    15,   235,   236,    18,   241,   242,   104,    22,
      75,   101,   101,   106,   406,   106,    99,   105,   443,   105,
     105,   414,   104,    99,   105,   412,    42,   104,   103,   108,
     455,   391,   106,   105,    99,   106,   100,   102,    42,   108,
     105,   428,    42,   106,   109,   106,   111,   112,   113,   114,
     115,   108,   105,   100,   355,   355,   106,    70,    71,   355,
       8,    74,   100,   456,    77,     7,   367,   367,   367,    99,
     104,    99,    42,    42,   467,   102,   105,   110,    99,   105,
      42,   382,    95,    42,    99,   428,   110,   106,   110,   106,
     108,    37,   106,    99,   291,   100,   108,    99,    40,    41,
      42,    43,   100,   106,    96,   406,   107,    49,    50,   106,
     106,    72,   413,   413,   413,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    51,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,   105,   404,   413,   108,   109,   191,   111,
     112,   113,   114,   115,   367,   166,   280,   385,    98,   245,
     205,   243,    70,    71,    72,    73,    74,    75,    76,    77,
     244,   246,    -1,    -1,    -1,   247,   269,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    95,    -1,    97,
      98,    99,    -1,    -1,   102,    -1,    -1,   105,    -1,    -1,
      -1,   109,    -1,   111,   112,   113,   114,   115,     4,     5,
       6,     7,    -1,     9,    -1,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    -1,    24,    -1,
      -1,    27,    -1,    29,    -1,    31,    -1,    33,    34,    35,
      -1,    -1,    -1,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    70,    71,    72,    73,    74,    -1,
      76,    77,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    95,
      -1,    97,    98,    -1,   101,    -1,    -1,    -1,    -1,   105,
      -1,    -1,    -1,   109,    -1,   111,   112,   113,   114,   115,
       4,     5,     6,     7,    -1,     9,    -1,    -1,    12,    13,
      14,    15,    -1,    17,    18,    -1,    20,    21,    22,     7,
      -1,    -1,    -1,    27,    -1,    -1,    -1,    31,    -1,    33,
      34,    35,    -1,    -1,    -1,    -1,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,     7,    -1,    -1,
      -1,    -1,    40,    41,    42,    43,    -1,    -1,    -1,    -1,
      -1,    49,    50,    -1,    -1,    -1,    70,    71,    -1,    -1,
      74,    -1,    -1,    77,    -1,    -1,    -1,    -1,    -1,    -1,
      40,    41,    42,    43,    -1,    -1,    -1,    -1,    -1,    49,
      50,    95,    -1,    97,    98,     7,    -1,    -1,    -1,    -1,
      -1,   105,    -1,    -1,    -1,   109,    -1,   111,   112,   113,
     114,   115,    -1,    -1,   102,    -1,    -1,   105,    -1,    -1,
      -1,   109,    -1,   111,   112,   113,   114,   115,    40,    41,
      42,    43,     7,    -1,    -1,    -1,    -1,    49,    50,    -1,
      -1,    -1,   102,     7,    -1,   105,    -1,    -1,    -1,   109,
      -1,   111,   112,   113,   114,   115,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    40,    41,    42,    43,    -1,
      -1,     7,    -1,    -1,    49,    50,    40,    41,    42,    43,
      -1,    -1,    -1,    -1,    -1,    49,    50,    -1,    -1,    -1,
      -1,    -1,    -1,   105,    -1,    -1,    -1,   109,    -1,   111,
     112,   113,   114,   115,    40,    41,    42,    43,    -1,    -1,
      -1,    -1,    -1,    49,    50,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     105,    -1,    -1,    -1,   109,    -1,   111,   112,   113,   114,
     115,   105,    -1,    -1,    -1,   109,    -1,   111,   112,   113,
     114,   115,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    -1,    -1,    91,    92,    93,    94,   105,
      -1,    -1,    -1,   109,    -1,   111,   112,   113,   114,   115,
       4,     5,     6,    -1,    -1,     9,    -1,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    -1,
      24,    -1,    -1,    27,    -1,    29,    -1,    31,    -1,    33,
      34,    35,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      44,    45,    46,    47,    48,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,    73,
      74,    -1,    76,    77,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    95,    -1,    97,    98,    -1,    -1,    -1,    -1,    -1,
       4,     5,     6,    -1,    -1,     9,   110,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    -1,
      24,    -1,    -1,    27,    -1,    29,    -1,    31,    -1,    33,
      34,    35,    -1,    -1,    -1,    39,    -1,    -1,    42,    -1,
      44,    45,    46,    47,    48,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,    73,
      74,    -1,    76,    77,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    95,     0,    97,    98,    -1,     4,     5,     6,    -1,
      -1,     9,   106,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    -1,    24,    -1,    -1,    27,
      -1,    29,    -1,    31,    -1,    33,    34,    35,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    44,    45,    46,    47,
      48,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    71,    72,    73,    74,    -1,    76,    77,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    95,    -1,    97,
      98,    99,     4,     5,     6,    -1,    -1,     9,    -1,    -1,
      12,    13,    14,    15,    -1,    17,    18,    -1,    20,    21,
      22,    -1,    -1,    -1,    -1,    27,    -1,    -1,    -1,    31,
      -1,    33,    34,    35,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    44,    45,    46,    47,    48,     4,     5,     6,
      -1,    -1,     9,    -1,    -1,    12,    13,    14,    15,    -1,
      17,    18,    -1,    20,    21,    22,    -1,    -1,    70,    71,
      27,    -1,    74,    -1,    31,    77,    33,    34,    35,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,    46,
      47,    48,    -1,    95,    -1,    97,    98,    -1,    -1,    -1,
      -1,   103,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    70,    71,    -1,    -1,    74,    -1,    -1,
      77,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     4,     5,     6,    -1,    95,     9,
      97,    98,    12,    13,    14,    15,   103,    17,    18,    -1,
      20,    21,    22,    -1,    -1,    -1,    -1,    27,    -1,    -1,
      -1,    31,    -1,    33,    34,    35,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    44,    45,    46,    47,    48,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      70,    71,    -1,    -1,    74,    -1,    -1,    77,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    95,    -1,    97,    98,     4,
       5,     6,    -1,   103,     9,    -1,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    -1,    24,
      -1,    -1,    27,    -1,    29,    -1,    31,    -1,    33,    34,
      35,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    44,
      45,    46,    47,    48,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    70,    71,    72,    73,    74,
      -1,    76,    77,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      95,    -1,    97,    98,     4,     5,     6,   102,    -1,     9,
      -1,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    -1,    24,    -1,    -1,    27,    -1,    29,
      -1,    31,    -1,    33,    34,    35,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    44,    45,    46,    47,    48,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      70,    71,    72,    73,    74,    -1,    76,    77,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     4,     5,     6,    -1,    95,     9,    97,    98,    12,
      13,    14,    15,    -1,    17,    18,    -1,    20,    21,    22,
      -1,    -1,    -1,    -1,    27,    -1,    -1,    -1,    31,    -1,
      33,    34,    35,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    44,    45,    46,    47,    48,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    -1,
      -1,    74,    -1,    -1,    77,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    95,    -1,    97,    98
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   124,     0,     4,     5,     6,     9,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    24,
      27,    29,    31,    33,    34,    35,    39,    44,    45,    46,
      47,    48,    70,    71,    72,    73,    74,    76,    77,    95,
      97,    98,    99,   125,   126,   128,   129,   130,   133,   134,
     135,   137,   145,   148,   149,   150,   151,    42,   102,   105,
     109,   131,   132,   155,   164,   165,   129,    42,    44,   102,
     136,   151,   102,    42,   146,   147,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    91,    92,    93,
      94,   152,   148,   166,   167,    99,   100,   101,   127,    42,
      44,   105,   163,   134,   138,   139,   141,   148,   102,   136,
     146,   101,   100,   103,   105,   105,   105,   105,   105,   106,
     165,   148,   132,   155,     7,    40,    41,    42,    43,    49,
      50,   102,   105,   109,   111,   112,   113,   114,   115,   179,
     195,   196,   199,   200,   201,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   218,   102,   128,
     129,   187,   166,    96,   105,   107,   156,   157,   158,   140,
     141,   103,   139,   104,   142,   143,   144,   155,   140,   138,
     102,   100,   103,   199,   212,   217,   103,   147,   218,    89,
      90,   153,   154,   218,   213,   218,   105,   199,   105,   199,
     199,   107,   180,   181,   182,   183,   184,   141,   173,   213,
     215,    49,    50,    51,   105,   107,   110,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,   101,   214,   201,
     109,   116,   117,   112,   113,    64,    65,    67,    68,   118,
     119,    66,    69,   111,   120,   121,    63,    62,   122,    40,
       3,     8,    10,    23,    25,    26,    28,    30,    32,    36,
      38,    42,    75,    99,   128,   185,   186,   187,   188,   189,
     190,   191,   192,   193,   194,   215,   155,   105,    42,   106,
     129,   169,   170,   171,   172,   108,   109,   213,   157,   217,
      99,   100,   104,   103,   138,   103,   106,   101,   101,   106,
     154,   106,   106,   106,   173,   173,   217,   100,   103,   179,
     101,   184,   164,   165,   174,   175,   106,   100,   106,   136,
     197,   198,   213,   215,   136,   213,   201,   201,   201,   202,
     202,   203,   203,   204,   204,   204,   204,   205,   205,   206,
     207,   208,   209,   210,   215,   105,   105,   185,   215,   216,
     105,    99,   104,   217,    99,   105,    42,   104,   103,   190,
      99,   106,   105,   155,   164,   174,   106,   100,   100,   106,
     108,   108,   144,   217,   103,    42,    42,   106,   106,   108,
     103,   181,   105,   177,   178,   102,   201,   213,   106,   100,
     108,   104,   215,   215,     8,    99,   215,   185,   104,   128,
     216,    99,   185,    42,   159,   160,   105,   110,   171,    42,
     110,   179,   166,   105,   107,   180,   213,   212,   106,   106,
     105,   106,   185,   216,    99,   105,   106,   160,   166,   110,
      42,   175,   168,   169,   109,   176,   213,   100,   103,   185,
     185,   215,   185,    99,   216,    42,   106,   110,   106,   106,
     108,   103,   106,    37,   216,    99,   100,   161,    99,   185,
     106,   216,   162,   213,   106,   185,   106,   100,   185,   213
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      case 42: /* "IDENTIFIER" */

/* Line 715 of yacc.c  */
#line 214 "tools/intern/useasm/cparser.y"
	{ YYFPRINTF (yyoutput, "%s", yyvaluep->pszString); };

/* Line 715 of yacc.c  */
#line 1652 "eurasiacon/binary2_540_omap4430_android_debug/host/intermediates/cparser/cparser.tab.c"
	break;
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Location data for the lookahead symbol.  */
YYLTYPE yylloc;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.
       `yyls': related to locations.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[2];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;

#if YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 1;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);

	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
	YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 4:

/* Line 1455 of yacc.c  */
#line 301 "tools/intern/useasm/cparser.y"
    {
			/* Ignore function definitions. */
		;}
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 305 "tools/intern/useasm/cparser.y"
    {
				CTreeAddExternDecl((yyvsp[(1) - (1)].psDeclaration));
		;}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 309 "tools/intern/useasm/cparser.y"
    {
			/* Extra semi-colon outside of a function. Both VisualC and gcc allow this. */
		;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 325 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclaration) = CTreeMakeDeclaration((yylsp[(2) - (3)]));
			(yyval.psDeclaration)->psDeclSpecifierList = (yyvsp[(1) - (3)].psDeclSpecifier);
			(yyval.psDeclaration)->psInitDeclaratorList = (yyvsp[(2) - (3)].psInitDeclarator);
		;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 334 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclSpecifier) = (yyvsp[(1) - (1)].psDeclSpecifier);
			(yyloc) = (yylsp[(1) - (1)]);
		;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 339 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclSpecifier) = CTreeAddToDeclSpecifierList((yyvsp[(1) - (2)].psDeclSpecifier), (yyvsp[(2) - (2)].psDeclSpecifier));
			(yyloc) = (yylsp[(1) - (2)]);
		;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 347 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclSpecifier) = (yyvsp[(1) - (1)].psDeclSpecifier);
			(yyloc) = (yylsp[(1) - (1)]);
		;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 352 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclSpecifier) = (yyvsp[(1) - (1)].psDeclSpecifier);
			(yyloc) = (yylsp[(1) - (1)]);
		;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 357 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclSpecifier) = (yyvsp[(1) - (1)].psDeclSpecifier);
			(yyloc) = (yylsp[(1) - (1)]);
		;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 362 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclSpecifier) = (yyvsp[(1) - (1)].psDeclSpecifier);
			(yyloc) = (yylsp[(1) - (1)]);
		;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 370 "tools/intern/useasm/cparser.y"
    {
			(yyval.psInitDeclarator) = (yyvsp[(1) - (1)].psInitDeclarator);
		;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 374 "tools/intern/useasm/cparser.y"
    {
			(yyval.psInitDeclarator) = CTreeAddToInitDeclaratorList((yyvsp[(1) - (3)].psInitDeclarator), (yyvsp[(3) - (3)].psInitDeclarator));
		;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 381 "tools/intern/useasm/cparser.y"
    {	
			(yyval.psInitDeclarator) = NULL; 
		;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 385 "tools/intern/useasm/cparser.y"
    {
			(yyval.psInitDeclarator) = CTreeMakeInitDeclarator((yylsp[(1) - (1)]));
			(yyval.psInitDeclarator)->psDeclarator = (yyvsp[(1) - (1)].psDeclarator);
			(yyval.psInitDeclarator)->psInitializer = NULL;
		;}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 391 "tools/intern/useasm/cparser.y"
    {
			(yyval.psInitDeclarator) = CTreeMakeInitDeclarator((yylsp[(1) - (3)]));
			(yyval.psInitDeclarator)->psDeclarator = (yyvsp[(1) - (3)].psDeclarator);
			(yyval.psInitDeclarator)->psInitializer = (yyvsp[(3) - (3)].pvVoid);
		;}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 400 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_TYPEDEF;
		;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 407 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_EXTERN;
		;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 414 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_STATIC;
		;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 421 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_AUTO;
		;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 428 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_REGISTER;
		;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 438 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_VOID;
		;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 445 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_CHAR;
		;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 452 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_SHORT;
		;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 459 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_INT;
		;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 466 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_LONG;
		;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 473 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_FLOAT;
		;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 480 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_DOUBLE;
		;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 487 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_MSVC_INT8;
		;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 494 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_MSVC_INT16;
		;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 501 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_MSVC_INT32;
		;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 508 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_MSVC_INT64;
		;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 515 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_SIGNED;
		;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 522 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_MSVC__W64;
		;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 529 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_UNSIGNED;
		;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 536 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_STRUCT;
			(yyval.psDeclSpecifier)->psStruct = (yyvsp[(1) - (1)].psStruct);
		;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 544 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_ENUM;
			(yyval.psDeclSpecifier)->psEnum = (yyvsp[(1) - (1)].psEnum);
		;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 552 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_TYPEDEF_NAME;
			(yyval.psDeclSpecifier)->pszIdentifier = (yyvsp[(1) - (1)].pszString);
		;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 560 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_GCC_BUILTIN_VA_LIST;
		;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 567 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_BOOL;
		;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 577 "tools/intern/useasm/cparser.y"
    {
			(yyval.psStruct) = CTreeMakeStruct((yylsp[(1) - (4)]));
			(yyval.psStruct)->bStruct = (yyvsp[(1) - (4)].bBool);
			(yyval.psStruct)->pszName = NULL;
			(yyval.psStruct)->psMemberList = (yyvsp[(3) - (4)].psStructDeclaration);
			(yyval.psStruct)->uAlignOverride = g_uStructAlignOverride;
		;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 585 "tools/intern/useasm/cparser.y"
    {
			(yyval.psStruct) = CTreeMakeStruct((yylsp[(1) - (5)]));
			(yyval.psStruct)->bStruct = (yyvsp[(1) - (5)].bBool);
			(yyval.psStruct)->pszName = (yyvsp[(2) - (5)].pszString);
			(yyval.psStruct)->psMemberList = (yyvsp[(4) - (5)].psStructDeclaration);
			(yyval.psStruct)->uAlignOverride = g_uStructAlignOverride;
		;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 593 "tools/intern/useasm/cparser.y"
    {
			(yyval.psStruct) = CTreeMakeStruct((yylsp[(1) - (6)]));
			(yyval.psStruct)->bStruct = (yyvsp[(1) - (6)].bBool);
			(yyval.psStruct)->pszName = (yyvsp[(3) - (6)].pszString);
			(yyval.psStruct)->psMemberList = (yyvsp[(5) - (6)].psStructDeclaration);
			(yyval.psStruct)->uAlignOverride = g_uStructAlignOverride;
		;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 601 "tools/intern/useasm/cparser.y"
    {
			(yyval.psStruct) = CTreeMakeStruct((yylsp[(1) - (2)]));
			(yyval.psStruct)->bStruct = (yyvsp[(1) - (2)].bBool);
			(yyval.psStruct)->pszName = (yyvsp[(2) - (2)].pszString);
			(yyval.psStruct)->psMemberList = NULL;
		;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 611 "tools/intern/useasm/cparser.y"
    {
			(yyval.pszString) = (yyvsp[(1) - (1)].pszString);
		;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 615 "tools/intern/useasm/cparser.y"
    {
			(yyval.pszString) = (yyvsp[(1) - (1)].pszString);
		;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 622 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine; 
			(yyval.bBool) = IMG_TRUE; 
		;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 628 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine; 
			(yyval.bBool) = IMG_FALSE; 
		;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 637 "tools/intern/useasm/cparser.y"
    {
			(yyval.psStructDeclaration) = (yyvsp[(1) - (1)].psStructDeclaration);
		;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 641 "tools/intern/useasm/cparser.y"
    {
			(yyval.psStructDeclaration) = CTreeAddToStructDeclarationList((yyvsp[(1) - (2)].psStructDeclaration), (yyvsp[(2) - (2)].psStructDeclaration));
		;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 648 "tools/intern/useasm/cparser.y"
    {
			(yyval.psStructDeclaration) = CTreeMakeStructDeclaration(PickNonNullLocation((yylsp[(2) - (3)]), (yylsp[(1) - (3)])));
			(yyval.psStructDeclaration)->psSpecifierList = (yyvsp[(1) - (3)].psDeclSpecifier);
			(yyval.psStructDeclaration)->psDeclaratorList = (yyvsp[(2) - (3)].psStructDeclarator);
		;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 657 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclSpecifier) = NULL;
			(yyloc).pszFilename = NULL;
			(yyloc).uLine = (IMG_UINT32)-1;
		;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 663 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclSpecifier) = (yyvsp[(1) - (1)].psDeclSpecifier);
		;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 670 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclSpecifier) = CTreeAddToDeclSpecifierList((yyvsp[(1) - (2)].psDeclSpecifier), (yyvsp[(2) - (2)].psDeclSpecifier));
			(yyloc) = (yylsp[(1) - (2)]);
		;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 675 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclSpecifier) = CTreeAddToDeclSpecifierList((yyvsp[(1) - (2)].psDeclSpecifier), (yyvsp[(2) - (2)].psDeclSpecifier));
			(yyloc) = (yylsp[(1) - (2)]);
		;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 683 "tools/intern/useasm/cparser.y"
    {
			(yyval.psStructDeclarator) = NULL;
			(yyloc).pszFilename = NULL;
			(yyloc).uLine = (IMG_UINT32)-1;
		;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 689 "tools/intern/useasm/cparser.y"
    {
			(yyval.psStructDeclarator) = (yyvsp[(1) - (1)].psStructDeclarator);
		;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 696 "tools/intern/useasm/cparser.y"
    {
			(yyval.psStructDeclarator) = (yyvsp[(1) - (1)].psStructDeclarator);
			
		;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 701 "tools/intern/useasm/cparser.y"
    {
			(yyval.psStructDeclarator) = CTreeAddToStructDeclaratorList((yyvsp[(1) - (3)].psStructDeclarator), (yyvsp[(3) - (3)].psStructDeclarator));
		;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 708 "tools/intern/useasm/cparser.y"
    {
			(yyval.psStructDeclarator) = CTreeMakeStructDeclarator((yylsp[(1) - (1)]));
			(yyval.psStructDeclarator)->psDeclarator = (yyvsp[(1) - (1)].psDeclarator);
		;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 713 "tools/intern/useasm/cparser.y"
    {
			(yyval.psStructDeclarator) = CTreeMakeStructDeclarator((yylsp[(1) - (3)]));
			(yyval.psStructDeclarator)->psDeclarator = (yyvsp[(1) - (3)].psDeclarator);
			(yyval.psStructDeclarator)->bBitField = IMG_TRUE;
			(yyval.psStructDeclarator)->psBitFieldSize = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 720 "tools/intern/useasm/cparser.y"
    {
			(yyval.psStructDeclarator) = CTreeMakeStructDeclarator((yylsp[(2) - (2)]));
			(yyval.psStructDeclarator)->psDeclarator = NULL;
			(yyval.psStructDeclarator)->bBitField = IMG_TRUE;
			(yyval.psStructDeclarator)->psBitFieldSize = (yyvsp[(2) - (2)].psExpression);
		;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 730 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psEnum) = CTreeMakeEnum((yyloc));
			(yyval.psEnum)->pszIdentifier = NULL;
			(yyval.psEnum)->psEnumeratorList = (yyvsp[(3) - (4)].psEnumerator);
		;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 738 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psEnum) = CTreeMakeEnum((yyloc));
			(yyval.psEnum)->pszIdentifier = (yyvsp[(2) - (5)].pszString);
			(yyval.psEnum)->psEnumeratorList = (yyvsp[(4) - (5)].psEnumerator);
		;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 746 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psEnum) = CTreeMakeEnum((yyloc));
			(yyval.psEnum)->pszIdentifier = NULL;
			(yyval.psEnum)->psEnumeratorList = (yyvsp[(3) - (5)].psEnumerator);
		;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 754 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psEnum) = CTreeMakeEnum((yyloc));
			(yyval.psEnum)->pszIdentifier = (yyvsp[(2) - (6)].pszString);
			(yyval.psEnum)->psEnumeratorList = (yyvsp[(4) - (6)].psEnumerator);
		;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 762 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psEnum) = CTreeMakeEnum((yyloc));
			(yyval.psEnum)->bPartial = IMG_TRUE;
			(yyval.psEnum)->pszIdentifier = (yyvsp[(2) - (2)].pszString);
		;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 773 "tools/intern/useasm/cparser.y"
    {
			(yyval.psEnumerator) = (yyvsp[(1) - (1)].psEnumerator);
		;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 777 "tools/intern/useasm/cparser.y"
    {
			(yyval.psEnumerator) = CTreeAddToEnumeratorList((yyvsp[(1) - (3)].psEnumerator), (yyvsp[(3) - (3)].psEnumerator));
		;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 784 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psEnumerator) = CTreeMakeEnumerator((yyloc));
			(yyval.psEnumerator)->pszIdentifier = (yyvsp[(1) - (1)].pszString);
			(yyval.psEnumerator)->psValue = NULL;
		;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 792 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psEnumerator) = CTreeMakeEnumerator((yyloc));
			(yyval.psEnumerator)->pszIdentifier = (yyvsp[(1) - (3)].pszString);
			(yyval.psEnumerator)->psValue = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 803 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine; 
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc)); 
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_CONST; 
		;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 810 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine; 
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc)); 
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_RESTRICT; 
		;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 817 "tools/intern/useasm/cparser.y"
    { 
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc)); 
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_VOLATILE; 
		;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 828 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_INLINE;
		;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 835 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_INLINE;
		;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 842 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_INLINE;
		;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 849 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_FORCEINLINE;
		;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 860 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_CDECL;
		;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 867 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_CDECL;
		;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 874 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_STDCALL;
		;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 881 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER__FASTCALL;
		;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 888 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_MSVC_PTR32;
		;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 895 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_MSVC_PTR64;
		;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 902 "tools/intern/useasm/cparser.y"
    {
			(yyloc) = (yylsp[(1) - (1)]);
			(yyval.psDeclSpecifier) = (yyvsp[(1) - (1)].psDeclSpecifier);
		;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 910 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclSpecifier) = CTreeMakeDeclSpecifier((yyloc));
			(yyval.psDeclSpecifier)->uType = YZDECLSPECIFIER_DECLSPEC;
		;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 949 "tools/intern/useasm/cparser.y"
    {	
			(yyval.psDeclarator) = (yyvsp[(2) - (3)].psDeclarator);
			(yyval.psDeclarator)->psPointer = (yyvsp[(1) - (3)].psPointer);
			(yyloc) = (yylsp[(2) - (3)]);
		;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 958 "tools/intern/useasm/cparser.y"
    {
		;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 968 "tools/intern/useasm/cparser.y"
    {
		;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 971 "tools/intern/useasm/cparser.y"
    {
		;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 997 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclarator) = CTreeMakeDeclarator((yyloc));
			(yyval.psDeclarator)->uType = YZDECLARATOR_IDENTIFIER;
			(yyval.psDeclarator)->pszIdentifier = (yyvsp[(1) - (1)].pszString);
		;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1005 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psDeclarator) = CTreeMakeDeclarator((yyloc));
			(yyval.psDeclarator)->uType = YZDECLARATOR_IDENTIFIER;
			(yyval.psDeclarator)->pszIdentifier = (yyvsp[(1) - (1)].pszString);
		;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1013 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclarator) = CTreeMakeDeclarator((yylsp[(3) - (4)]));
			(yyval.psDeclarator)->uType = YZDECLARATOR_BRACKETED;
			(yyval.psDeclarator)->psSubDeclarator = (yyvsp[(3) - (4)].psDeclarator);
			(yyloc) = (yylsp[(3) - (4)]);
		;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1020 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclarator) = CTreeMakeDeclarator((yylsp[(1) - (3)]));
			(yyval.psDeclarator)->uType = YZDECLARATOR_VARIABLELENGTHARRAY;
			(yyval.psDeclarator)->psSubDeclarator = (yyvsp[(1) - (3)].psDeclarator);
			(yyloc) = (yylsp[(1) - (3)]);
		;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1027 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclarator) = CTreeMakeDeclarator((yylsp[(1) - (4)]));
			(yyval.psDeclarator)->uType = YZDECLARATOR_FIXEDLENGTHARRAY;
			(yyval.psDeclarator)->psSubDeclarator = (yyvsp[(1) - (4)].psDeclarator);
			(yyval.psDeclarator)->psArraySize = (yyvsp[(3) - (4)].psExpression);
			(yyloc) = (yylsp[(1) - (4)]);
		;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1035 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclarator) = CTreeMakeDeclarator((yylsp[(1) - (4)]));
			(yyval.psDeclarator)->uType = YZDECLARATOR_VARIABLELENGTHARRAY;
			(yyval.psDeclarator)->psSubDeclarator = (yyvsp[(1) - (4)].psDeclarator);
			(yyloc) = (yylsp[(1) - (4)]);
		;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1042 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclarator) = CTreeMakeDeclarator((yylsp[(1) - (4)]));
			(yyval.psDeclarator)->uType = YZDECLARATOR_FUNCTION;
			(yyval.psDeclarator)->psSubDeclarator = (yyvsp[(1) - (4)].psDeclarator);
			(yyloc) = (yylsp[(1) - (4)]);
		;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1049 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclarator) = CTreeMakeDeclarator((yylsp[(1) - (3)]));
			(yyval.psDeclarator)->uType = YZDECLARATOR_FUNCTION;
			(yyval.psDeclarator)->psSubDeclarator = (yyvsp[(1) - (3)].psDeclarator);
			(yyloc) = (yylsp[(1) - (3)]);
		;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1056 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclarator) = CTreeMakeDeclarator((yylsp[(1) - (4)]));
			(yyval.psDeclarator)->uType = YZDECLARATOR_FUNCTION;
			(yyval.psDeclarator)->psSubDeclarator = (yyvsp[(1) - (4)].psDeclarator);
			(yyloc) = (yylsp[(1) - (4)]);
		;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1066 "tools/intern/useasm/cparser.y"
    {
			(yyval.psPointer) = NULL;
		;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1070 "tools/intern/useasm/cparser.y"
    {
			(yyval.psPointer) = (yyvsp[(1) - (1)].psPointer);
		;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1077 "tools/intern/useasm/cparser.y"
    {
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			(yyval.psPointer) = CTreeMakePointer((yyloc)); 
			(yyval.psPointer)->psQualifierList = (yyvsp[(2) - (2)].psDeclSpecifier);
		;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1084 "tools/intern/useasm/cparser.y"
    {
			YZPOINTER*	psRet;
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
			psRet = CTreeMakePointer((yyloc));
			psRet->psQualifierList = (yyvsp[(2) - (3)].psDeclSpecifier);
			psRet->psNext = (yyvsp[(3) - (3)].psPointer);
			(yyval.psPointer) = psRet;
		;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1097 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psDeclSpecifier) = NULL;
		;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1101 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psDeclSpecifier) = (yyvsp[(1) - (1)].psDeclSpecifier); 
		;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1108 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psDeclSpecifier) = (yyvsp[(1) - (1)].psDeclSpecifier);
		;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1112 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psDeclSpecifier) = CTreeAddToDeclSpecifierList((yyvsp[(1) - (2)].psDeclSpecifier), (yyvsp[(2) - (2)].psDeclSpecifier));
		;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 1144 "tools/intern/useasm/cparser.y"
    {
			(yyval.psTypeName) = CTreeMakeTypeName((yylsp[(2) - (2)]), (yyvsp[(1) - (2)].psDeclSpecifier), (yyvsp[(2) - (2)].psDeclarator));
			(yyloc) = (yylsp[(1) - (2)]);
		;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1152 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclarator) = NULL;
		;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1156 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclarator) = (yyvsp[(1) - (1)].psDeclarator);
		;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1163 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclarator) = CTreeMakeDeclarator((yylsp[(1) - (1)]));
			(yyval.psDeclarator)->uType = YZDECLARATOR_ABSTRACTPOINTER;
			(yyval.psDeclarator)->psPointer = (yyvsp[(1) - (1)].psPointer);
			(yyval.psDeclarator)->psSubDeclarator = NULL;
		;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1170 "tools/intern/useasm/cparser.y"
    {
			if ((yyvsp[(1) - (2)].psPointer) == NULL)
			{
				(yyval.psDeclarator) = (yyvsp[(2) - (2)].psDeclarator);
			}
			else
			{
				(yyval.psDeclarator) = CTreeMakeDeclarator((yylsp[(1) - (2)]));
				(yyval.psDeclarator)->uType = YZDECLARATOR_ABSTRACTPOINTER;
				(yyval.psDeclarator)->psPointer = (yyvsp[(1) - (2)].psPointer);
				(yyval.psDeclarator)->psSubDeclarator = (yyvsp[(2) - (2)].psDeclarator);
			}
		;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1187 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = NULL;
		;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 1191 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = NULL;
		;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1195 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = (yyvsp[(1) - (1)].psExpression);
		;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1202 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclarator) = CTreeMakeDeclarator((yylsp[(3) - (4)]));
			(yyval.psDeclarator)->uType = YZDECLARATOR_BRACKETED;
			(yyval.psDeclarator)->psQualifierList = (yyvsp[(2) - (4)].psDeclSpecifier);
			(yyval.psDeclarator)->psSubDeclarator = (yyvsp[(3) - (4)].psDeclarator);
		;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1209 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclarator) = CTreeMakeDeclarator((yylsp[(3) - (4)]));
			if ((yyvsp[(3) - (4)].psExpression) != NULL)
			{
				(yyval.psDeclarator)->uType = YZDECLARATOR_FIXEDLENGTHARRAY;
			}
			else
			{
				(yyval.psDeclarator)->uType = YZDECLARATOR_VARIABLELENGTHARRAY;
			}
			(yyval.psDeclarator)->psSubDeclarator = (yyvsp[(1) - (4)].psDeclarator);
			(yyval.psDeclarator)->psArraySize = (yyvsp[(3) - (4)].psExpression);
		;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1223 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclarator) = CTreeMakeDeclarator((yylsp[(3) - (4)]));
			(yyval.psDeclarator)->uType = YZDECLARATOR_FUNCTION;
			(yyval.psDeclarator)->psSubDeclarator = (yyvsp[(1) - (4)].psDeclarator);
		;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1232 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclarator) = NULL;
		;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1236 "tools/intern/useasm/cparser.y"
    {
			(yyval.psDeclarator) = (yyvsp[(1) - (1)].psDeclarator);
		;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1243 "tools/intern/useasm/cparser.y"
    { (yyval.pvVoid) = NULL; ;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1245 "tools/intern/useasm/cparser.y"
    { (yyval.pvVoid) = NULL; ;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1247 "tools/intern/useasm/cparser.y"
    { (yyval.pvVoid) = NULL; ;}
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 1336 "tools/intern/useasm/cparser.y"
    { 
				(yyloc).pszFilename = g_pszYzSourceFile;
				(yyloc).uLine = g_uYzSourceLine;
				(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_IDENTIFIER, (yyloc)); 
				(yyval.psExpression)->pszString = (yyvsp[(1) - (1)].pszString);
		;}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 1343 "tools/intern/useasm/cparser.y"
    { 
				(yyloc).pszFilename = g_pszYzSourceFile;
				(yyloc).uLine = g_uYzSourceLine;
				(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_INTEGER_NUMBER, (yyloc)); 
				(yyval.psExpression)->uNumber = (yyvsp[(1) - (1)].n);
		;}
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 1350 "tools/intern/useasm/cparser.y"
    { 
				(yyloc).pszFilename = g_pszYzSourceFile;
				(yyloc).uLine = g_uYzSourceLine;
				(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_FLOAT_NUMBER, (yyloc)); 
				(yyval.psExpression)->pszString = (yyvsp[(1) - (1)].pszString);
		;}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 1357 "tools/intern/useasm/cparser.y"
    { 
				(yyloc).pszFilename = g_pszYzSourceFile;
				(yyloc).uLine = g_uYzSourceLine;
				(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_STRING_LITERAL, (yyloc)); 
				(yyval.psExpression)->pszString = (yyvsp[(1) - (1)].pszString);
		;}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 1364 "tools/intern/useasm/cparser.y"
    { 
				(yyval.psExpression) = (yyvsp[(2) - (3)].psExpression);
		;}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 1371 "tools/intern/useasm/cparser.y"
    { (yyval.psExpression) = (yyvsp[(1) - (1)].psExpression); ;}
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 1373 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_ARRAYACCESS, (yyvsp[(1) - (4)].psExpression)->sLocation); 
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (4)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (4)].psExpression);
		;}
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 1379 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_FUNCTIONCALL,  (yyvsp[(1) - (4)].psExpression)->sLocation); 
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (4)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (4)].psExpression);
		;}
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 1385 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_STRUCTACCESS,  (yyvsp[(1) - (3)].psExpression)->sLocation); 
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->pszString = (yyvsp[(3) - (3)].pszString);
		;}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 1391 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_STRUCTPTRACCESS,  (yyvsp[(1) - (3)].psExpression)->sLocation); 
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->pszString = (yyvsp[(3) - (3)].pszString);
		;}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 1397 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_POSTINCREMENT,  (yyvsp[(1) - (2)].psExpression)->sLocation); 
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (2)].psExpression);
		;}
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 1402 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_POSTDECREMENT,  (yyvsp[(1) - (2)].psExpression)->sLocation); 
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (2)].psExpression);
		;}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 1407 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_INITIALIZER,  (yylsp[(2) - (6)]));
		;}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 1411 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_INITIALIZER, (yylsp[(2) - (7)]));
		;}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 1418 "tools/intern/useasm/cparser.y"
    { (yyval.psExpression) = NULL; ;}
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 1420 "tools/intern/useasm/cparser.y"
    { (yyval.psExpression) = (yyvsp[(1) - (1)].psExpression); ;}
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 1425 "tools/intern/useasm/cparser.y"
    { (yyval.psExpression) = (yyvsp[(1) - (1)].psExpression); ;}
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 1427 "tools/intern/useasm/cparser.y"
    { (yyval.psExpression) = CTreeAddToExpressionList((yyvsp[(1) - (3)].psExpression), (yyvsp[(3) - (3)].psExpression)); ;}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 1432 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = (yyvsp[(1) - (1)].psExpression); 
		;}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 1436 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_PREINCREMENT, (yyvsp[(2) - (2)].psExpression)->sLocation); 
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(2) - (2)].psExpression);
		;}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 1441 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_PREINCREMENT, (yyvsp[(2) - (2)].psExpression)->sLocation); 
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(2) - (2)].psExpression);
		;}
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 1446 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = CTreeMakeExpression((yyvsp[(1) - (2)].n), (yyvsp[(2) - (2)].psExpression)->sLocation); 
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(2) - (2)].psExpression);
		;}
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 1451 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_SIZEOFEXPRESSION, (yyvsp[(2) - (2)].psExpression)->sLocation); 
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(2) - (2)].psExpression);
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
		;}
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 1458 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_SIZEOFTYPE, (yylsp[(3) - (4)]));
			(yyval.psExpression)->psSizeofTypeName = (yyvsp[(3) - (4)].psTypeName);
			(yyloc).pszFilename = g_pszYzSourceFile;
			(yyloc).uLine = g_uYzSourceLine;
		;}
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 1468 "tools/intern/useasm/cparser.y"
    { (yyval.n) = YZEXPRESSION_ADDRESSOF; ;}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 1470 "tools/intern/useasm/cparser.y"
    { (yyval.n) = YZEXPRESSION_DEREFERENCE; ;}
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 1472 "tools/intern/useasm/cparser.y"
    { (yyval.n) = YZEXPRESSION_POSITIVE; ;}
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 1474 "tools/intern/useasm/cparser.y"
    { (yyval.n) = YZEXPRESSION_NEGATE; ;}
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 1476 "tools/intern/useasm/cparser.y"
    { (yyval.n) = YZEXPRESSION_BITWISENOT; ;}
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 1478 "tools/intern/useasm/cparser.y"
    { (yyval.n) = YZEXPRESSION_LOGICALNOT; ;}
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 1483 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = (yyvsp[(1) - (1)].psExpression);
		;}
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 1487 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_CAST, (yyvsp[(4) - (4)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(4) - (4)].psExpression);
		;}
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 1495 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = (yyvsp[(1) - (1)].psExpression);
		;}
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 1499 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_MULTIPLY, (yyvsp[(1) - (3)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 1505 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_DIVIDE, (yyvsp[(1) - (3)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 1511 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_MODULUS, (yyvsp[(1) - (3)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 1520 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = (yyvsp[(1) - (1)].psExpression);
		;}
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 1524 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_ADD, (yyvsp[(1) - (3)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 1530 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_SUB, (yyvsp[(1) - (3)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 1539 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = (yyvsp[(1) - (1)].psExpression); 
		;}
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 1543 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_SHIFTLEFT, (yyvsp[(1) - (3)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 1549 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_SHIFTRIGHT, (yyvsp[(1) - (3)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 1558 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = (yyvsp[(1) - (1)].psExpression); 
		;}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 1562 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_LT, (yyvsp[(1) - (3)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 1568 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_GT, (yyvsp[(1) - (3)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 1574 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_LTE, (yyvsp[(1) - (3)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 1580 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_GTE, (yyvsp[(1) - (3)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 1589 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = (yyvsp[(1) - (1)].psExpression); 
		;}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 1593 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_EQ, (yyvsp[(1) - (3)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 1599 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_NEQ, (yyvsp[(1) - (3)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 1608 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = (yyvsp[(1) - (1)].psExpression); 
		;}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 1612 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_BITWISEAND, (yyvsp[(1) - (3)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 1621 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = (yyvsp[(1) - (1)].psExpression); 
		;}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 1625 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_BITWISEXOR, (yyvsp[(1) - (3)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 1634 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = (yyvsp[(1) - (1)].psExpression); 
		;}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 1638 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_BITWISEOR, (yyvsp[(1) - (3)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 1647 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = (yyvsp[(1) - (1)].psExpression); 
		;}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 1651 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_LOGICALAND, (yyvsp[(1) - (3)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 1660 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = (yyvsp[(1) - (1)].psExpression); 
		;}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 1664 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_LOGICALOR, (yyvsp[(1) - (3)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 1673 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = (yyvsp[(1) - (1)].psExpression); 
		;}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 1677 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_CONDITIONAL, (yyvsp[(1) - (5)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (5)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (5)].psExpression);
			(yyval.psExpression)->psSubExpr3 = (yyvsp[(5) - (5)].psExpression);
		;}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 1687 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = (yyvsp[(1) - (1)].psExpression); 
		;}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 1691 "tools/intern/useasm/cparser.y"
    { 
			(yyval.psExpression) = CTreeMakeExpression((yyvsp[(2) - (3)].n), (yyvsp[(1) - (3)].psExpression)->sLocation); 
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 1700 "tools/intern/useasm/cparser.y"
    { (yyval.n) = YZEXPRESSION_ASSIGNMENT; ;}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 1702 "tools/intern/useasm/cparser.y"
    { (yyval.n) = YZEXPRESSION_MULTIPLYASSIGNMENT; ;}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 1704 "tools/intern/useasm/cparser.y"
    { (yyval.n) = YZEXPRESSION_DIVIDEASSIGNMENT; ;}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 1706 "tools/intern/useasm/cparser.y"
    { (yyval.n) = YZEXPRESSION_MODULUSASSIGNMENT; ;}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 1708 "tools/intern/useasm/cparser.y"
    { (yyval.n) = YZEXPRESSION_ADDASSIGNMENT; ;}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 1710 "tools/intern/useasm/cparser.y"
    { (yyval.n) = YZEXPRESSION_SUBASSIGNMENT; ;}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 1712 "tools/intern/useasm/cparser.y"
    { (yyval.n) = YZEXPRESSION_SHIFTLEFTASSIGNMENT; ;}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 1714 "tools/intern/useasm/cparser.y"
    { (yyval.n) = YZEXPRESSION_SHIFTRIGHTASSIGNMENT; ;}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 1716 "tools/intern/useasm/cparser.y"
    { (yyval.n) = YZEXPRESSION_BITWISEANDASSIGNMENT; ;}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 1718 "tools/intern/useasm/cparser.y"
    { (yyval.n) = YZEXPRESSION_BITWISEXORASSIGNMENT; ;}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 1720 "tools/intern/useasm/cparser.y"
    { (yyval.n) = YZEXPRESSION_BITWISEORASSIGNMENT; ;}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 1725 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = (yyvsp[(1) - (1)].psExpression);
		;}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 1729 "tools/intern/useasm/cparser.y"
    {
			(yyval.psExpression) = CTreeMakeExpression(YZEXPRESSION_COMMA, (yyvsp[(1) - (3)].psExpression)->sLocation);
			(yyval.psExpression)->psSubExpr1 = (yyvsp[(1) - (3)].psExpression);
			(yyval.psExpression)->psSubExpr2 = (yyvsp[(3) - (3)].psExpression);
		;}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 1738 "tools/intern/useasm/cparser.y"
    { (yyval.psExpression) = NULL; ;}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 1740 "tools/intern/useasm/cparser.y"
    { (yyval.psExpression) = (yyvsp[(1) - (1)].psExpression); ;}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 1745 "tools/intern/useasm/cparser.y"
    { (yyval.psExpression) = (yyvsp[(1) - (1)].psExpression); ;}
    break;



/* Line 1455 of yacc.c  */
#line 4452 "eurasiacon/binary2_540_omap4430_android_debug/host/intermediates/cparser/cparser.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, &yylloc);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 1753 "tools/intern/useasm/cparser.y"


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

