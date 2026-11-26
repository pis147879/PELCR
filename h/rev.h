/* ADDREV: the contraction of the chains of unary edges (rev.c)

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

#ifndef rev_h
#define rev_h

/* the limit to the accumulation of the unary edges: the optimization is not done on a weight
   already longer than this (PELCR_REV_CAP) */
#ifndef REVLETTERS
#define REVLETTERS 8
#endif

int RevEnabled(void);
void RevFilterAdd(struct messaggio *m, node *n);
void RevArrive(struct messaggio *m);
void RevPrintStats(void);
void RevNoteCold(node *v);

#endif
