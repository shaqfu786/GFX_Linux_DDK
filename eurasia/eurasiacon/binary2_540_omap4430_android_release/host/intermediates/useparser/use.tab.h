
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
     REGISTER = 258,
     OPCODE_FLAG1 = 259,
     OPCODE_FLAG2 = 260,
     OPCODE_FLAG3 = 261,
     ARGUMENT_FLAG = 262,
     MASK = 263,
     NUMBER = 264,
     TEST_TYPE = 265,
     TEST_CHANSEL = 266,
     TEMP_REGISTER = 267,
     REPEAT_FLAG = 268,
     TEST_MASK = 269,
     C10_FLAG = 270,
     F16_FLAG = 271,
     U8_FLAG = 272,
     FLOAT_NUMBER = 273,
     OPCODE = 274,
     COISSUE_OPCODE = 275,
     LD_OPCODE = 276,
     ST_OPCODE = 277,
     ELD_OPCODE = 278,
     BRANCH_OPCODE = 279,
     EFO_OPCODE = 280,
     LAPC_OPCODE = 281,
     WDF_OPCODE = 282,
     EXT_OPCODE = 283,
     LOCKRELEASE_OPCODE = 284,
     IDF_OPCODE = 285,
     FIR_OPCODE = 286,
     ASOP2_OPCODE = 287,
     ONEARG_OPCODE = 288,
     PTOFF_OPCODE = 289,
     CALL_OPCODE = 290,
     NOP_OPCODE = 291,
     EMITVTX_OPCODE = 292,
     CFI_OPCODE = 293,
     INPUT = 294,
     OUTPUT = 295,
     OPEN_SQBRACKET = 296,
     CLOSE_SQBRACKET = 297,
     PLUS = 298,
     MINUS = 299,
     TIMES = 300,
     DIVIDE = 301,
     LSHIFT = 302,
     RSHIFT = 303,
     HASH = 304,
     BANG = 305,
     AND = 306,
     OR = 307,
     PLUSPLUS = 308,
     MINUSMINUS = 309,
     EQUALS = 310,
     NOT = 311,
     XOR = 312,
     OPEN_BRACKET = 313,
     CLOSE_BRACKET = 314,
     INSTRUCTION_DELIMITER = 315,
     COMMA = 316,
     PRED0 = 317,
     PRED1 = 318,
     PRED2 = 319,
     PRED3 = 320,
     PREDN = 321,
     SCHEDON = 322,
     SCHEDOFF = 323,
     SKIPINVON = 324,
     SKIPINVOFF = 325,
     REPEATOFF = 326,
     FORCE_ALIGN = 327,
     MISALIGN = 328,
     IMPORT = 329,
     EXPORT = 330,
     MODULEALIGN = 331,
     I0 = 332,
     I1 = 333,
     A0 = 334,
     A1 = 335,
     M0 = 336,
     M1 = 337,
     SRC0 = 338,
     SRC1 = 339,
     SRC2 = 340,
     DIRECT_IMMEDIATE = 341,
     ADDRESS_MODE = 342,
     SWIZZLE = 343,
     DATAFORMAT = 344,
     INTSRCSEL = 345,
     ABS = 346,
     ABS_NODOT = 347,
     UNEXPECTED_CHARACTER = 348,
     INDEXLOW = 349,
     INDEXHIGH = 350,
     INDEXBOTH = 351,
     PCLINK = 352,
     LABEL_ADDRESS = 353,
     IDF_ST = 354,
     IDF_PIXELBE = 355,
     NOSCHED_FLAG = 356,
     SKIPINV_FLAG = 357,
     TARGET_SPECIFIER = 358,
     TARGET = 359,
     TEMP_REG_DEF = 360,
     OPEN_CURLY_BRACKET = 361,
     CLOSE_CURLY_BRACKET = 362,
     PROC = 363,
     SCOPE_NAME = 364,
     IDENTIFIER = 365,
     NAMED_REGS_RANGE = 366,
     RENAME_REG = 367,
     COLON = 368,
     COLON_PLUS_DELIMITER = 369,
     MODULUS = 370
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 2083 "tools/intern/useasm/use.y"

    IMG_UINT32 n;
    float f;
    USE_REGISTER sRegister;
    SOURCE_ARGUMENTS sArgs;
    USE_INST sInst;
    PUSE_INST psInst;
    OPCODE_AND_LINE sOp;
    LDST_ARGUMENT sLdStArg;
    INSTRUCTION_MODIFIER sMod;
    USEASM_PARSER_REG_LIST* psRegList;



/* Line 1676 of yacc.c  */
#line 182 "eurasiacon/binary2_540_omap4430_android_release/host/intermediates/useparser/use.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


