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

#ifdef PELCR_DEFINE_GLOBALS
#define PELCR_EXTERN
#else
#define PELCR_EXTERN extern
#endif

#define TYPE 1
#define KONST 2
#define FUNC 3
#define SYNC 4
#define ARG 5
#define ITE 6
#define ARGLAST 7

#define BOOL 1
#define NAT 2

#define TIN 0
#define TOUT 1

/* ANTO */
#define EXT 0
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

/* the constants: a table that grows (KonstTable, buildgraph.c) from KONST_INITIAL entries (the
   former fixed size, so that the entries are chosen as before) up to MAXNUMCOST, the room of the
   index in an external letter X(KONST, index) (XL_OFFSET in h/weight.h); the parser also returns
   MAXNUMCOST + 1 for #quit, above every index */
#define KONST_INITIAL 10000
#define MAXNUMCOST 131072
#define MAXTYPE 20       /*era 10*/
#define MAXFUNCTIONS 50  /*era 50*/
#define MAXDATAVARS 20   /*era 10*/

typedef struct functionstack {
	int which;
	int type;
	int narg;
	int wait;
	USERTYPE (*fun)();
	USERTYPE s[MAXNUMARG];
	/* ANTO */
	char name[MAXNAMELEN];
	int fun_id;
	/* ANTO */
} functionstack;

/* ANTO */
PELCR_EXTERN int *k_type;      /* k_capacity entries (KonstTable) */
PELCR_EXTERN USERTYPE *k_value;
PELCR_EXTERN int *k_refs;
PELCR_EXTERN int k_capacity;
/* ANTO */

PELCR_EXTERN functionstack f[MAXFUNCTIONS];
PELCR_EXTERN functionstack f_db[MAXFUNCTIONS];
PELCR_EXTERN int t[5][2];

#undef PELCR_EXTERN
#endif
