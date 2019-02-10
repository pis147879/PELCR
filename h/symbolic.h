/* Header for the implementation of Girard's dynamic algebra
 
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


#ifndef symbolic_h
#define symbolic_h

typedef char term;


/* extern int k[MAXNUMCOST][2]; */
/* extern functionstack f[MAXFUNCTIONS]; */
/* extern int  t[5][2]; */

extern int kindex;
extern int newval;
extern int findex;
extern int fcounter;

/* extern double (*funcs[MAXFUNCTIONS])(double); */
/* extern void *vars[MAXDATAVARS]; */

void checkone(char*s);
int findType(int tipo);
int product(char*a,char*b,char*a1,char*b1);
int isone(char*a);

#endif

