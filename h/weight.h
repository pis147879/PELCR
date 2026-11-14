/* Weights of the dynamic algebra as words of integer letters

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

/* A positive word of Lambda* is a sequence of letters {x, m} = !^m(x), as in the
   prototype VM-PELCR5.ma, with the indices m non-decreasing: the string form of
   the weights ("!2w(1,1)p", see symbolic.c) is the same word, where "!n" raises
   by n the index of all the letters that follow it.  One letter is a 64-bit
   integer:

       bits 0-2    kind: p, q, w (fan), c (choice), X (external algebra)
       bits 3-42   the index m of the exponential (up to 2^40 - 1: in dd4 it reaches 3069)
       bits 43-63  w(i,l), c(i,l): i in bits 43-55 (0..8191), the lift l in bits 56-63 (0..255)
                   X(i,l): i in bits 43-45 (1..7), l in bits 46-63 (-131072..131071)

   A weight is an element of Lambda*_nd = Lambda* (x) Lambda* (symbolic.c): the word
   u1 of component 1 and, when component 0 is not erase(u1), the word u0 of
   component 0.  The letters follow the header in memory:

       [weight: t1 n1 n0 one] [n1 letters of u1] [n0 letters of u0]

   t1 is the index of the exponential at the end of u1 (a trailing "!n" of the string
   form, e.g. "!2" = !^2(1)): the product of symbolic.c treats such a word as the
   string form does.  An edge allocates exactly the letters of its weight (graph.c);
   a message and the buffers of the product have room for MAXLETTERS letters (wbuf). */

#ifndef weight_h
#define weight_h

#include <stddef.h>

#ifndef MAXLETTERS
#define MAXLETTERS 16
#endif

typedef unsigned long long letter;
typedef long long wexp; /* an index of the exponential */

#define L_P 1
#define L_Q 2
#define L_W 3
#define L_C 4
#define L_X 5
#define L_BANG 6 /* "!n" of the string form, only inside the product (symbolic.c): n in the index field */

#define WEXP_MAX ((1LL << 40) - 1)
#define LETTER_KIND(x) ((int)((x)&7u))
#define LETTER_EXP(x) ((wexp)(((x) >> 3) & (letter)WEXP_MAX))
#define LETTER_PAYLOAD(x) ((x) >> 43)
#define LETTER_WITH_EXP(x, m) (((x) & ~((letter)WEXP_MAX << 3)) | ((letter)(m) << 3))
#define IS_GEN_LETTER(x) ((LETTER_KIND(x) == L_W) || (LETTER_KIND(x) == L_C))
#define GEN_INDEX(x) ((long)(LETTER_PAYLOAD(x) & 0x1FFFu))
#define GEN_LIFT(x) ((long)(LETTER_PAYLOAD(x) >> 13))
#define XL_OFFSET 131072L
#define XL_SUB(x) ((int)(LETTER_PAYLOAD(x) & 7u))
#define XL_VALUE(x) ((long)(LETTER_PAYLOAD(x) >> 3) - XL_OFFSET)
#define BANG_VALUE(x) LETTER_EXP(x)

letter MakePQ(int kind, wexp m);
letter MakeGen(int kind, long index, long lift, wexp m);
letter MakeX(long sub, long value, wexp m);
letter MakeBang(wexp n);
void WeightExpCheck(wexp m);

#define W_CLASSICAL 0xFF

typedef struct weight {
	unsigned long long t1 : 40; /* index of the exponential at the end of u1 */
	unsigned long long n1 : 8;  /* letters of component 1 */
	unsigned long long n0 : 8;  /* letters of component 0, or W_CLASSICAL when component 0 is erase(u1) */
	unsigned long long one : 8; /* the identity written "1" (the string form kept it), else 0 */
} weight;

/* a weight with room for MAXLETTERS letters */
typedef struct wbuf {
	weight h;
	letter l[MAXLETTERS];
} wbuf;

/* the letters of a wbuf follow its header, as those of an edge follow the edge */
_Static_assert(offsetof(wbuf, l) == sizeof(weight), "wbuf: letters right after the header");

#define WLETTERS(w) ((letter *)((w) + 1))
#define WLETTERS_C(w) ((const letter *)((w) + 1))
#define W_IS_PAIR(w) ((w)->n0 != W_CLASSICAL)
#define W_NLETTERS(w) ((int)(w)->n1 + (W_IS_PAIR(w) ? (int)(w)->n0 : 0))
#define W_BYTES(w) (sizeof(weight) + (size_t)W_NLETTERS(w) * sizeof(letter))

extern long max_weight_length; /* the largest number of letters of a weight (statistics) */

void WeightSetOne(weight *w);
void WeightCopy(weight *dst, const weight *src);
int WeightIsOne(const weight *w);
void WeightCheck(int nletters, const char *what);
void WeightParse(const char *s, weight *w);
char *WeightToString(const weight *w, char *buf, size_t size);
char *WordToString(const letter *l, int n, wexp t, char *buf, size_t size);
void WeightComponents(const weight *w, char *u1, size_t size1, char *u0, size_t size0);
int WeightFindX(const weight *w, int sub);
void WeightSetXAndTruncate(weight *w, int k, long value);
long WeightKonstIndex(const letter *l, int n);

#endif
