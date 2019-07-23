/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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
     HELP = 258,
     TRACEF = 259,
     VERBOSEF = 260,
     USELIB = 261,
     QUIT = 262,
     WRITE = 263,
     SETDIR = 264,
     SETLOOPS = 265,
     SETFIRES = 266,
     STATUS = 267,
     SYMBOL = 268,
     REC = 269,
     DEF = 270,
     IF = 271,
     THEN = 272,
     ELSE = 273,
     GMLGRAPH = 274,
     NL = 275,
     EQ = 276,
     LAMBDA = 277,
     OP1 = 278,
     OP2 = 279,
     ID = 280,
     LOP1 = 281,
     LOP2 = 282,
     NUM = 283,
     BOOLEAN = 284,
     F1ARG = 285,
     F2ARG = 286,
     XFUNCTION = 287,
     LET = 288,
     INN = 289,
     PATH = 290,
     XCALL = 291
   };
#endif
/* Tokens.  */
#define HELP 258
#define TRACEF 259
#define VERBOSEF 260
#define USELIB 261
#define QUIT 262
#define WRITE 263
#define SETDIR 264
#define SETLOOPS 265
#define SETFIRES 266
#define STATUS 267
#define SYMBOL 268
#define REC 269
#define DEF 270
#define IF 271
#define THEN 272
#define ELSE 273
#define GMLGRAPH 274
#define NL 275
#define EQ 276
#define LAMBDA 277
#define OP1 278
#define OP2 279
#define ID 280
#define LOP1 281
#define LOP2 282
#define NUM 283
#define BOOLEAN 284
#define F1ARG 285
#define F2ARG 286
#define XFUNCTION 287
#define LET 288
#define INN 289
#define PATH 290
#define XCALL 291




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 59 "PELCR/parser.y"
{
  char string[500];
  int number;
  int intero;
  struct termGraph *grafo;
}
/* Line 1529 of yacc.c.  */
#line 128 "parser.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

