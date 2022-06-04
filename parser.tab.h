/* A Bison parser, made by GNU Bison 3.6.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_PARSER_TAB_H_INCLUDED
# define YY_YY_PARSER_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    HELP = 258,                    /* HELP  */
    TRACEF = 259,                  /* TRACEF  */
    VERBOSEF = 260,                /* VERBOSEF  */
    USELIB = 261,                  /* USELIB  */
    QUIT = 262,                    /* QUIT  */
    WRITE = 263,                   /* WRITE  */
    SETDIR = 264,                  /* SETDIR  */
    SETLOOPS = 265,                /* SETLOOPS  */
    SETFIRES = 266,                /* SETFIRES  */
    STATUS = 267,                  /* STATUS  */
    SYMBOL = 268,                  /* SYMBOL  */
    REC = 269,                     /* REC  */
    DEF = 270,                     /* DEF  */
    IF = 271,                      /* IF  */
    THEN = 272,                    /* THEN  */
    ELSE = 273,                    /* ELSE  */
    GMLGRAPH = 274,                /* GMLGRAPH  */
    NL = 275,                      /* NL  */
    EQ = 276,                      /* EQ  */
    LAMBDA = 277,                  /* LAMBDA  */
    OP1 = 278,                     /* OP1  */
    OP2 = 279,                     /* OP2  */
    ID = 280,                      /* ID  */
    LOP1 = 281,                    /* LOP1  */
    LOP2 = 282,                    /* LOP2  */
    NUM = 283,                     /* NUM  */
    BOOLEAN = 284,                 /* BOOLEAN  */
    F1ARG = 285,                   /* F1ARG  */
    F2ARG = 286,                   /* F2ARG  */
    XFUNCTION = 287,               /* XFUNCTION  */
    LET = 288,                     /* LET  */
    INN = 289,                     /* INN  */
    PATH = 290,                    /* PATH  */
    XCALL = 291                    /* XCALL  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 60 "parser.y"

  char string[500];
  int number;
  int intero;
  struct termGraph *grafo;

#line 107 "parser.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_PARSER_TAB_H_INCLUDED  */
