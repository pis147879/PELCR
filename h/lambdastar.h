/* Data structures required to manage external functions and C-native data types 
 Copyright (C) 1997-2015 Marco Pedicini
 
 This file is part of PELCR.
 
 PELCR is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 PELCR is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with PELCR.  If not, see <http://www.gnu.org/licenses/>.  */


#ifndef LAMBDASTAR_H
#define LAMBDASTAR_H



#define  TYPE 1
#define  KONST 2
#define  FUNC 3
#define  SYNC 4
#define  ARG 5
#define  ITE 6
#define  ARGLAST 7

#define  BOOL 1
#define  NAT 2

#define  TIN 0
#define  TOUT 1

/* ANTO */
#define EXT  0
/* ANTO */
#define SUCC 1
#define ADD 2
#define PRED 3
#define MULT 4
#define AND 5
#define NOT 6
#define ISZERO 7
#define OR 8
#define GREATER 9


#define MAXNUMCOST 10000 /*era 1000000*/
#define MAXTYPE 20  /*era 10*/
#define MAXFUNCTIONS 50  /*era 50*/
#define MAXDATAVARS 20  /*era 10*/

typedef struct functionstack 
{
  int which;
  int type;
  int narg;
  long wait;
  USERTYPE (*fun)();
  USERTYPE s[MAXNUMARG];
/* ANTO */
  char name[MAXNAMELEN];
  int fun_id;
/* ANTO */
} functionstack ;

/* ANTO */
USERTYPE k[MAXNUMCOST][3];
/* ANTO */

functionstack f[MAXFUNCTIONS];
functionstack f_db[MAXFUNCTIONS];
int  t[5][2];
#endif
 
