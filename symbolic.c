/* The implementation of Girard's dynamic algebra

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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef SOLARIS
#include <sys/systeminfo.h>
#endif
#include <sys/times.h>
#include <sys/types.h>
// #include "mpi.h"
#ifdef MPE_GRAPH
#include <mpe.h>
#endif
#include <time.h>

#include <string.h>

#include "h/var.h"

/* ---------------------------------------------------------------------------
   Girard's dynamic algebra on words of integer letters (h/weight.h).

   The product follows the rules of the string form of the weights, where "!n"
   raises by n the exponential of all the letters that follow it ("!2w(1,1)p"):
   product1 reads the two words as sequences of tokens, letters and "!n", exactly
   as the string form did, and writes the results in the same token form; the
   tokens are only internal (a word is stored as letters {x, m}, see weight.c).
   --------------------------------------------------------------------------- */

#define MAXTOK (2 * MAXLETTERS + 8)

/* a result of product1, written from the end (the rules add letters on the left) */
typedef struct tokbuf {
	int s; /* the result is t[s .. MAXTOK) */
	letter t[MAXTOK];
} tokbuf;

static void
StoreComputedValue(int type, USERTYPE result) {
	SetNextKIndex();
	k_refs[kindex]++;
	pelcr_value_set(&k_value[kindex], &result);
	pelcr_value_clear(&result);
	k_type[kindex] = type;
	newval = 1;
}

static void
tok_overflow(void) {
	WeightCheck(MAXLETTERS + 1, "product");
}

static void
tb_init(tokbuf *b) {
	b->s = MAXTOK;
}

/* x b, a letter or "!n"; "!n!m" is merged into "!(n+m)" as checkone did */
static void
tb_prefix(tokbuf *b, letter x) {
	if (LETTER_KIND(x) == L_BANG && b->s < MAXTOK && LETTER_KIND(b->t[b->s]) == L_BANG) {
		b->t[b->s] = MakeBang(BANG_VALUE(b->t[b->s]) + BANG_VALUE(x));
		return;
	}
	if (b->s == 0)
		tok_overflow();
	b->t[--b->s] = x;
}

/* b r */
static void
tb_append(tokbuf *b, const letter *r, int n) {
	int len = MAXTOK - b->s;
	if (n == 0)
		return;
	if (b->s < n)
		tok_overflow();
	memmove(b->t + b->s - n, b->t + b->s, (size_t)len * sizeof(letter));
	memcpy(b->t + MAXTOK - n, r, (size_t)n * sizeof(letter));
	b->s -= n;
}

/* the first X(KONST,.) of a result (-1 if none) */
static long
tb_konst(const tokbuf *b) {
	return WeightKonstIndex(b->t + b->s, MAXTOK - b->s);
}

/* x r: a token in front of the rest of an operand */
static int
with_head(letter x, const letter *r, int n, letter *out) {
	if (n + 1 > MAXTOK)
		tok_overflow();
	out[0] = x;
	memcpy(out + 1, r, (size_t)n * sizeof(letter));
	return n + 1;
}

#ifdef WDEBUG
/* the string form of tokens, from its character k (the debugging prints of the string form) */
static const char *
tok_string(const letter *tok, int n, int k) {
	static char buf[4][32 * MAXLETTERS + 64];
	static int i = 0;
	char *s;
	i = (i + 1) % 4;
	s = buf[i];
	WordToString(tok, n, 0, s, sizeof(buf[0]));
	return ((size_t)k <= strlen(s)) ? s + k : s + strlen(s);
}
#define TOKS(tok, n, k) tok_string((tok), (n), (k))
#define TBS(r) tok_string((r)->t + (r)->s, MAXTOK - (r)->s, 0)
#endif

static int product1(const letter *a, int na, const letter *b, int nb, tokbuf *a1, tokbuf *b1);

/* the value of a function on the constant k (it was evaluated when it had all its arguments) */
static void
EvaluateFunction(long fi, long k) {
	if (f[fi].narg == 1) { /* valuto la funzione di 1 variabile*/
#ifdef WDEBUG
		printf("%d) function evaluation (1)\n", rank);
#endif
		StoreComputedValue(f[fi].type, ((USERTYPE(*)(USERTYPE))f[fi].fun)(k_value[k]));
	} else if (f[fi].narg == 2) { /* valuto la funzione di 2 variabili */
#ifdef WDEBUG
		printf("%d) function evaluation (2)\n", rank);
#endif
		StoreComputedValue(f[fi].type, ((USERTYPE(*)(USERTYPE, USERTYPE))f[fi].fun)((f[fi].s)[0], k_value[k]));
	} else if (f[fi].narg == 3) { /* valuto la funzione di 3 variabili */
#ifdef WDEBUG
		printf("%d) function evaluation (3)\n", rank);
#endif
		StoreComputedValue(f[fi].type, ((USERTYPE(*)(USERTYPE, USERTYPE, USERTYPE))f[fi].fun)(
		                                   (f[fi].s)[0], (f[fi].s)[1], k_value[k]));
	} else
		printf("EXCEPTION\n");
#ifdef WDEBUG
	printf("--R-- %d ", kindex);
	pelcr_value_print(stdout, &k_value[kindex]);
	printf("\n%d) VALORE CALCOLATO ", rank);
	pelcr_value_print(stdout, &k_value[kindex]);
	printf("\n");
#endif
}

/***************** Algebra esterna ********************/
/* the two heads are external letters X(i1,l1), X(i2,l2) */
static int
product_external(const letter *a, int na, const letter *b, int nb, tokbuf *a1, tokbuf *b1) {
	long i1 = XL_SUB(a[0]), l1 = XL_VALUE(a[0]);
	long i2 = XL_SUB(b[0]), l2 = XL_VALUE(b[0]);
	const letter *ar = a + 1, *br = b + 1;
	int nar = na - 1, nbr = nb - 1;
	letter tmp[MAXTOK];
	int ft;

#ifdef WDEBUG
	printf("\nExtern Algebra --> %s\n", TOKS(a, na, 2));
	printf("\nExtern Algebra --> %s\n", TOKS(b, nb, 2));
	printf("Extern Algebra --> branch: %d , %d\n", (int)i1, (int)i2);
	printf("Extern Algebra -->  %s %s\n", TOKS(ar, nar, 0), TBS(a1));
	printf("Extern Algebra -->  %s %s\n", TOKS(br, nbr, 0), TBS(b1));
#endif

	if ((i1 == TYPE) && (i2 == ARG)) {
#ifdef WDEBUG
		printf("\t%d) TYPE ARG\n", rank);
#endif
		if (!product1(ar, nar, br, nbr, a1, b1))
			return 0;
		tb_prefix(a1, MakeX(SYNC, (t[l1][1] == TIN) ? l2 - 1 : l2, 0));
		return 12;
	}

	else if ((i1 == TYPE) && (i2 == ARGLAST)) {
#ifdef WDEBUG
		printf("\t%d) TYPE LASTARG\n", rank);
#endif
		if (!product1(ar, nar, br, nbr, a1, b1))
			return 0;
		tb_prefix(a1, MakeX(SYNC, (t[l1][1] == TIN) ? l2 - 1 : l2, 0));
		if (t[l1][1] == TOUT)
			return 40 + (int)(l2 % 2);
		else
			return 13;
	}

	else if ((i1 == ARG) && (i2 == TYPE)) {
#ifdef WDEBUG
		printf("\t%d) ARG TYPE\n", rank);
#endif
		if (!product1(ar, nar, br, nbr, a1, b1))
			return 0;
		tb_prefix(b1, MakeX(SYNC, (t[l2][1] == TIN) ? l1 - 1 : l1, 0));
		return 14;
	}

	else if ((i1 == ARGLAST) && (i2 == TYPE)) {
#ifdef WDEBUG
		printf("\t%d) ARG TYPE\n", rank);
#endif
		if (!product1(ar, nar, br, nbr, a1, b1))
			return 0;
		tb_prefix(b1, MakeX(SYNC, (t[l2][1] == TIN) ? l1 - 1 : l1, 0));
		if (t[l2][1] == TOUT)
			return 42 + (int)(l1 % 2);
		else
			return 15;
	}

	else if ((i1 == SYNC) && (i2 == ARG)) {
#ifdef WDEBUG
		printf("\t%d) SYNC ARG\n", rank);
#endif
		if (!product1(ar, nar, br, nbr, a1, b1))
			return 0;
		tb_prefix(a1, MakeX(SYNC, l2, 0));
		return 16;
	}

	else if ((i1 == ARG) && (i2 == SYNC)) {
#ifdef WDEBUG
		printf("\t%d) ARG SYNC\n", rank);
#endif
		if (!product1(ar, nar, br, nbr, a1, b1))
			return 0;
		tb_prefix(b1, MakeX(SYNC, l1, 0));
		return 17;
	}

	else if ((i1 == SYNC) && (i2 == FUNC)) {
#ifdef WDEBUG
		printf("\t%d) SYNC FUNC\n", rank);
#endif
		if (l1 == 0) {
			nbr = with_head(MakeX(FUNC, l2, 0), br, nbr, tmp);
			br = tmp;
		}
		if (!product1(ar, nar, br, nbr, a1, b1))
			return 0;
		if (l1 == 0) {
			ft = findType(f[l2].type);
			tb_prefix(a1, MakeX(TYPE, ft, 0));
		} else if (l1 == f[l2].narg) {
			ft = findType(f[l2].type);
			tb_prefix(a1, MakeX(TYPE, ft + 1, 0));
			tb_prefix(b1, MakeX(FUNC, l2, 0));
		} else
			return 0;
		return 18;
	}

	else if ((i1 == FUNC) && (i2 == SYNC)) {
#ifdef WDEBUG
		printf("\t%d) FUNC SYNC\n", rank);
#endif
		if (l2 == 0) {
			nar = with_head(MakeX(FUNC, l1, 0), ar, nar, tmp);
			ar = tmp;
		}
		if (!product1(ar, nar, br, nbr, a1, b1))
			return 0;
		if (l2 == 0) {
			ft = findType(f[l1].type);
			tb_prefix(b1, MakeX(TYPE, ft, 0));
		} else if (l2 == f[l1].narg) {
			ft = findType(f[l1].type);
			tb_prefix(a1, MakeX(FUNC, l1, 0));
			tb_prefix(b1, MakeX(TYPE, ft + 1, 0));
		} else
			return 0;
		return 19;
	}

	else if ((i1 == KONST) && (i2 == FUNC)) {
#ifdef WDEBUG
		printf("%d) CONST-FUNC WAIT %d, NARG %d\n", rank, f[l2].wait, f[l2].narg);
#endif
		if (f[l2].wait == f[l2].narg) {
			EvaluateFunction(l2, l1);
			if (!product1(ar, nar, br, nbr, a1, b1))
				return 0;
			tb_prefix(a1, MakeX(KONST, kindex, 0));
			tb_prefix(b1, MakeX(FUNC, l2, 0));
			return 20;
		} else {
			/* valuto parzialmente la funzione */
#ifdef WDEBUG
			printf("%d) PARTIAL EVAL FUNC\n", rank);
#endif
			pelcr_value_set(&(f[l2].s)[f[l2].wait - 1], &k_value[l1]);
			f[l2].wait++;
			if (!product1(ar, nar, br, nbr, a1, b1))
				return 0;
			tb_prefix(b1, MakeX(FUNC, l2, 0));
			return 21;
		}
	}

	else if ((i1 == FUNC) && (i2 == KONST)) {
#ifdef WDEBUG
		printf("%d) FUNC-CONST WAIT %d, NARG %d\n", rank, f[l1].wait, f[l1].narg);
#endif
		if (f[l1].wait == f[l1].narg) {
			EvaluateFunction(l1, l2);
			if (!product1(ar, nar, br, nbr, a1, b1))
				return 0;
			tb_prefix(b1, MakeX(KONST, kindex, 0));
			tb_prefix(a1, MakeX(FUNC, l1, 0));
			return 22;
		} else {
			/* valuto parzialmente la funzione */
#ifdef WDEBUG
			printf("%d) PARTIAL EVAL FUNC\n", rank);
#endif
			pelcr_value_set(&(f[l1].s)[f[l1].wait - 1], &k_value[l2]);
			f[l1].wait++;
			if (!product1(ar, nar, br, nbr, a1, b1))
				return 0;
			tb_prefix(a1, MakeX(FUNC, l1, 0));
			return 23;
		}
	}

	else if ((i1 == FUNC) && (i2 == FUNC)) {
#ifdef WDEBUG
		printf("\t%d) FUNC FUNC\n", rank);
#endif
		if ((l1 == l2) && (product1(ar, nar, br, nbr, a1, b1)))
			return 24;
		else
			return 0;
	}

	else if (((i1 == SYNC) && (i2 == SYNC)) || ((i1 == TYPE) && (i2 == TYPE))) {
		int res;
#ifdef WDEBUG
		printf("\t%d) SYNC SYNC\n", rank);
		printf("--> %s, %s\n", TOKS(ar, nar, 0), TOKS(br, nbr, 0));
#endif
		res = product1(ar, nar, br, nbr, a1, b1);
#ifdef WDEBUG
		printf("%d\n", res);
		printf("%d , %d\n", (int)l1, (int)l2);
#endif
		if ((l1 == l2) && (res)) {
#ifdef WDEBUG
			printf("%d) TYPE-TYPE\n", rank);
			printf("%s, %s\n", TBS(a1), TBS(b1));
#endif
			return 25;
		} else
			return 0;
	}

	/********************* Interazioni relative all'if_then_else ***********/

	else if ((i1 == ITE) && (i2 == KONST)) {
#ifdef WDEBUG
		printf("\t%d) ITE CONST\n", rank);
#endif
		if (!product1(ar, nar, br, nbr, a1, b1))
			return 0;
		tb_prefix(a1, MakeX(ITE, pelcr_value_is_zero(&k_value[l2]) ? 2 : 1, 0));
		return 26;
	}

	else if ((i1 == KONST) && (i2 == ITE)) {
#ifdef WDEBUG
		printf("\t%d) KONST ITE\n", rank);
#endif
		if (!product1(ar, nar, br, nbr, a1, b1))
			return 0;
		tb_prefix(b1, MakeX(ITE, pelcr_value_is_zero(&k_value[l1]) ? 2 : 1, 0));
		return 27;
	}

	else if ((i1 == ITE) && (i2 == ITE)) {
#ifdef WDEBUG
		printf("\t%d) ITE ITE\n", rank);
#endif
		if (l1 == l2)
			return product1(ar, nar, br, nbr, a1, b1) ? 28 : 0;
		else
			return 0;
	}

	else if ((i1 == ITE) && (i2 == SYNC)) {
#ifdef WDEBUG
		printf("\t%d) ITE SYNC\n", rank);
#endif
		/* (the string form also had a case l2 == 1 here, under l2 == 0: never taken) */
		if (l2 == 0 && l1 == 0) {
			nar = with_head(MakeX(ITE, 0, 0), ar, nar, tmp);
			return product1(tmp, nar, br, nbr, a1, b1) ? 29 : 0;
		} else
			return 0;
	}

	else if ((i1 == SYNC) && (i2 == ITE)) {
#ifdef WDEBUG
		printf("\t%d) SYNC ITE\n", rank);
#endif
		if (l1 == 0 && l2 == 0) {
			nbr = with_head(MakeX(ITE, 0, 0), br, nbr, tmp);
			br = tmp;
		}
		if (!product1(ar, nar, br, nbr, a1, b1))
			return 0;
		if (l1 == 0 && l2 == 0)
			return 31;
		else if ((l1 == 1) && (l2 == 1 || l2 == 2)) {
			tb_prefix(b1, MakeX(ITE, l2, 0));
			return 32;
		} else
			return 0;
	}

	return 0;
}

/* a^* b = b1 a1^* on two words in token form; 0 if a^* b = 0.  The return value tells
   which rule applied (40, 42: change of polarity, see NodeCombustion). */
static int
product1(const letter *a, int na, const letter *b, int nb, tokbuf *a1, tokbuf *b1) {
	letter ha, hb, tmp[MAXTOK];
	int ka, kb, n, r;
	wexp px;
	long k;

	if (na == 0) {
		tb_append(b1, b, nb);
		return 1;
	}
	if (nb == 0) {
		tb_append(a1, a, na);
		return 2;
	}
	ha = a[0];
	hb = b[0];
	ka = LETTER_KIND(ha);
	kb = LETTER_KIND(hb);

	if (((ka == L_P) && (kb == L_P)) || ((ka == L_Q) && (kb == L_Q))) {
		if (product1(a + 1, na - 1, b + 1, nb - 1, a1, b1)) {
			local_family_reductions++;
			return 3;
		} else
			return 0;
	}

	else if (IS_GEN_LETTER(ha) && IS_GEN_LETTER(hb)) {
		/* w(i,l): fan generator; c(h,1): choice generator (component 0 of Lambda*_nd only).
		   x^* y = 1 if x = y, 0 otherwise (ANNIHILATION) */
		if (ka != kb)
			return 0;
		if (GEN_INDEX(ha) == GEN_INDEX(hb))
			return product1(a + 1, na - 1, b + 1, nb - 1, a1, b1) ? 4 : 0;
		else
			return 0;
	}

	else if ((ka == L_BANG) && (kb == L_BANG)) {
		wexp p1 = BANG_VALUE(ha), p2 = BANG_VALUE(hb);
		if (p1 == p2) {
			r = product1(a + 1, na - 1, b + 1, nb - 1, a1, b1);
			if (!r)
				return 0;
			tb_prefix(a1, MakeBang(p1));
			tb_prefix(b1, MakeBang(p1));
			if ((r == 30) || (r == 31) || (r == 32) || (r == 33))
				return r;
			else
				return 5;
		} else if (p1 < p2) {
			n = with_head(MakeBang(p2 - p1), b + 1, nb - 1, tmp);
			if (!product1(a + 1, na - 1, tmp, n, a1, b1))
				return 0;
			tb_prefix(a1, MakeBang(p1));
			tb_prefix(b1, MakeBang(p1));
			return 6;
		} else {
			n = with_head(MakeBang(p1 - p2), a + 1, na - 1, tmp);
			if (!product1(tmp, n, b + 1, nb - 1, a1, b1))
				return 0;
			tb_prefix(a1, MakeBang(p2));
			tb_prefix(b1, MakeBang(p2));
			return 7;
		}
	}

	else if ((ka == L_BANG) && IS_GEN_LETTER(hb)) {
		/* !(u)^* x = x !^{e(x)}(u)^* (COMMUTATION, x = w or c of lift e(x)) */
		px = BANG_VALUE(ha) + GEN_LIFT(hb) - 1;
		if (px != 0) {
			n = with_head(MakeBang(px), a + 1, na - 1, tmp);
			r = product1(tmp, n, b + 1, nb - 1, a1, b1);
		} else
			r = product1(a + 1, na - 1, b + 1, nb - 1, a1, b1);
		if (!r)
			return 0;
		tb_prefix(b1, hb);
		k = tb_konst(a1);
		if (k >= 0)
			k_refs[k]++;
		return 8;
	}

	else if (IS_GEN_LETTER(ha) && (kb == L_BANG)) {
		px = BANG_VALUE(hb) + GEN_LIFT(ha) - 1;
		if (px != 0) {
			n = with_head(MakeBang(px), b + 1, nb - 1, tmp);
			r = product1(a + 1, na - 1, tmp, n, a1, b1);
		} else
			r = product1(a + 1, na - 1, b + 1, nb - 1, a1, b1);
		if (!r)
			return 0;
		tb_prefix(a1, ha);
		k = tb_konst(b1);
		if (k >= 0)
			k_refs[k]++;
		return 9;
	}

	else if ((ka == L_BANG) && ((kb == L_P) || (kb == L_Q))) {
		/* !(u)^* x = x !(u)^* for x = p, q of lift 1 (COMMUTATION) */
		if (!product1(a, na, b + 1, nb - 1, a1, b1))
			return 0;
		tb_prefix(b1, hb);
		return 50;
	}

	else if (((ka == L_P) || (ka == L_Q)) && (kb == L_BANG)) {
		/* x^* !(u) = !(u) x^* for x = p, q of lift 1 (COMMUTATION) */
		if (!product1(a + 1, na - 1, b, nb, a1, b1))
			return 0;
		tb_prefix(a1, ha);
		return 51;
	}

	/* (the string form skipped the letter 1 here: the integer form has no such letter) */

	else if ((ka == L_X) && (kb == L_X))
		return product_external(a, na, b, nb, a1, b1);

	return 0;
}

/* ---------------------------------------------------------------------------
   The non-deterministic dynamic algebra Lambda*_nd = Lambda* (x) Lambda*
   (reduced square, Definition ndexgoi2 of the ND-PELCR article):
       P = (1,p),  Q = (1,q),  W_j = (w_j,w_j),  C_h = (w_h,1) with e(w_h) = 1;
   products, involution and lift are componentwise, and a pair with a zero
   component is 0.

   A weight is a positive element: the word u1 of component 1 and, when component 0
   is not erase(u1), the word u0 of component 0 (h/weight.h), where erase(u1)
   deletes p, q and the external letters X(..).  Without u0 the weight is exact for
   every product of P, Q, W_j and lifts (component 0 of such a product is the same
   word without p and q), so choice-free computations keep the classical
   representation and the classical product.  Choice generators c(h,1) occur only in
   component 0; fan generators w(i,l) in both.
   --------------------------------------------------------------------------- */

/* a word in token form: "!n" before a letter whose index exceeds the previous one, and
   at the end up to t */
static int
to_tokens(const letter *l, int n, wexp tend, letter *out) {
	int k, j = 0;
	wexp cur = 0;
	for (k = 0; k < n; k++) {
		wexp m = LETTER_EXP(l[k]);
		if (m > cur) {
			out[j++] = MakeBang(m - cur);
			cur = m;
		}
		out[j++] = LETTER_WITH_EXP(l[k], 0);
	}
	if (tend > cur)
		out[j++] = MakeBang(tend - cur);
	return j;
}

/* tokens -> letters {x, m}; *t receives the index at the end */
static int
from_tokens(const letter *tok, int n, letter *out, int room, wexp *tend) {
	int k, j = 0;
	wexp cur = 0;
	for (k = 0; k < n; k++) {
		if (LETTER_KIND(tok[k]) == L_BANG)
			cur += BANG_VALUE(tok[k]);
		else {
			WeightExpCheck(cur);
			if (j >= room)
				WeightCheck(MAXLETTERS + 1, "product");
			out[j++] = LETTER_WITH_EXP(tok[k], cur);
		}
	}
	WeightExpCheck(cur);
	*tend = cur;
	return j;
}

/* erase: the generators w, c of a word, with their indices */
static int
erase_word(const letter *l, int n, letter *out) {
	int k, j = 0;
	for (k = 0; k < n; k++)
		if (IS_GEN_LETTER(l[k]))
			out[j++] = l[k];
	return j;
}

/* component 0 of a weight */
static int
component0(const weight *w, letter *out) {
	const letter *l = WLETTERS_C(w);
	if (!W_IS_PAIR(w))
		return erase_word(l, w->n1, out);
	memcpy(out, l + w->n1, (size_t)w->n0 * sizeof(letter));
	return w->n0;
}

/* the pair (u0,u1) into w, in the classical form when u0 = erase(u1) (nd_join): u1 as it
   is with its index at the end t1, u0 by its generators; in the pair form u1 without
   letters is the identity 1 */
static void
join(weight *w, const letter *u1, int n1, wexp t1, const letter *u0, int n0) {
	letter e1[MAXTOK], e0[MAXTOK];
	int ne1 = erase_word(u1, n1, e1);
	int ne0 = erase_word(u0, n0, e0);
	letter *l = WLETTERS(w);

	if (ne1 == ne0 && memcmp(e1, e0, (size_t)ne1 * sizeof(letter)) == 0) {
		WeightCheck(n1, "nd_join");
		memmove(l, u1, (size_t)n1 * sizeof(letter));
		w->n1 = n1;
		w->n0 = W_CLASSICAL;
		w->t1 = t1;
		w->one = 0;
	} else {
		WeightCheck(n1 + ne0, "nd_join");
		memmove(l, u1, (size_t)n1 * sizeof(letter));
		memcpy(l + n1, e0, (size_t)ne0 * sizeof(letter));
		w->n1 = n1;
		w->n0 = ne0;
		w->t1 = ((n1 == 0) ? 0 : t1);
		w->one = 0;
	}
}

static void
join_tokens(weight *w, const tokbuf *r1, const tokbuf *r0) {
	letter u1[MAXTOK], u0[MAXTOK];
	wexp t1, t0;
	int n1 = from_tokens(r1->t + r1->s, MAXTOK - r1->s, u1, MAXTOK, &t1);
	int n0 = from_tokens(r0->t + r0->s, MAXTOK - r0->s, u0, MAXTOK, &t0);
	join(w, u1, n1, t1, u0, n0);
}

static void
classical_tokens(weight *w, const tokbuf *r) {
	wexp t1;
	int n1 = from_tokens(r->t + r->s, MAXTOK - r->s, WLETTERS(w), MAXLETTERS, &t1);
	w->n1 = n1;
	w->n0 = W_CLASSICAL;
	w->t1 = t1;
	w->one = 0;
}

/* stable form in Lambda*_nd: a^* b = b1 a1^* (0 if a^* b = 0); a1 and b1 have room for
   MAXLETTERS letters */
int
product(const weight *a, const weight *b, weight *a1, weight *b1) {
	letter ta[MAXTOK], tb[MAXTOK], c[MAXTOK];
	tokbuf r1, s1, r0, s0;
	int na, nb, nc, out;

	if (!W_IS_PAIR(a) && !W_IS_PAIR(b)) {
		na = to_tokens(WLETTERS_C(a), a->n1, a->t1, ta);
		nb = to_tokens(WLETTERS_C(b), b->n1, b->t1, tb);
		tb_init(&r1);
		tb_init(&s1);
		out = product1(ta, na, tb, nb, &r1, &s1);
		if (out) {
			classical_tokens(a1, &r1);
			classical_tokens(b1, &s1);
		}
		return out;
	}

	/* component 0 first: it has no external letters, hence no side effects */
	nc = component0(a, c);
	na = to_tokens(c, nc, 0, ta);
	nc = component0(b, c);
	nb = to_tokens(c, nc, 0, tb);
	tb_init(&r0);
	tb_init(&s0);
	if (!product1(ta, na, tb, nb, &r0, &s0))
		return 0;
	na = to_tokens(WLETTERS_C(a), a->n1, a->t1, ta);
	nb = to_tokens(WLETTERS_C(b), b->n1, b->t1, tb);
	tb_init(&r1);
	tb_init(&s1);
	out = product1(ta, na, tb, nb, &r1, &s1);
	if (!out)
		return 0;
	join_tokens(a1, &r1, &r0);
	join_tokens(b1, &s1, &s0);
	return out;
}

int
isone(const weight *w) {
	return WeightIsOne(w);
}

/* ---- concatenation (ADDREV, rev.c) ---- */

static wexp
letter_lift(letter x) {
	return IS_GEN_LETTER(x) ? (wexp)GEN_LIFT(x) : 1;
}

/* a word of letters {x, m} in canonical order: indices non-decreasing, by the commutation
   y_n x_m = x_m y_{n+e(x)-1} (n > m) of positive letters */
static int
canon_word(letter *l, int n) {
	int i, moved = 1;
	while (moved) {
		moved = 0;
		for (i = 0; i + 1 < n; i++) {
			wexp a = LETTER_EXP(l[i]), b = LETTER_EXP(l[i + 1]);
			if (a > b) {
				letter y = l[i], x = l[i + 1];
				wexp m = a + letter_lift(x) - 1;
				WeightExpCheck(m);
				l[i] = x;
				l[i + 1] = LETTER_WITH_EXP(y, m);
				moved = 1;
			}
		}
	}
	return n;
}

/* out = b a, the product of two positive weights (both components of Lambda*_nd): the path
   that crosses first the edge of weight b, then the edge of weight a */
void
WeightConcat(const weight *b, const weight *a, weight *out) {
	letter u1[2 * MAXTOK], u0[2 * MAXTOK], c[MAXTOK];
	int n1, n0, nb0, na0;
	wexp t1;

	n1 = b->n1 + a->n1;
	if (n1 > 2 * MAXTOK)
		tok_overflow();
	memcpy(u1, WLETTERS_C(b), (size_t)b->n1 * sizeof(letter));
	memcpy(u1 + b->n1, WLETTERS_C(a), (size_t)a->n1 * sizeof(letter));
	canon_word(u1, n1);
	t1 = (n1 > 0) ? LETTER_EXP(u1[n1 - 1]) : 0;
	WeightCheck(n1, "WeightConcat");
	if (!W_IS_PAIR(a) && !W_IS_PAIR(b)) {
		memcpy(WLETTERS(out), u1, (size_t)n1 * sizeof(letter));
		out->n1 = n1;
		out->n0 = W_CLASSICAL;
		out->t1 = t1;
		out->one = 0;
		return;
	}
	nb0 = component0(b, c);
	memcpy(u0, c, (size_t)nb0 * sizeof(letter));
	na0 = component0(a, c);
	memcpy(u0 + nb0, c, (size_t)na0 * sizeof(letter));
	n0 = canon_word(u0, nb0 + na0);
	join(out, u1, n1, t1, u0, n0);
}

/* ---- lifts and prefixes used by the graph builder (in place: the edges of the graph
   builder have room for MAXLETTERS letters) ---- */

/* !(u) on a word whose first letter is not the head of a free-variable edge; the
   identity stays as it is */
static void
lift_word(letter *l, int n, wexp *tend) {
	int k;
	if (n == 0 && *tend == 0)
		return;
	for (k = 0; k < n; k++) {
		wexp m = LETTER_EXP(l[k]) + 1;
		WeightExpCheck(m);
		l[k] = LETTER_WITH_EXP(l[k], m);
	}
	WeightExpCheck(*tend + 1);
	(*tend)++;
}

/* the edge of a free variable starts with its fan generator w(i,l): a lift of the
   term raises the lift l of that generator (the variable is one box deeper) */
static void
lift_fan_head(letter *l) {
	*l = MakeGen(L_W, GEN_INDEX(*l), GEN_LIFT(*l) + 1, LETTER_EXP(*l));
}

static int
fan_head(const letter *l, int n) {
	return n > 0 && LETTER_KIND(l[0]) == L_W && LETTER_EXP(l[0]) == 0;
}

void
LiftWeight(weight *w) {
	letter *l = WLETTERS(w);
	letter u1[MAXTOK], u0[MAXTOK];
	int n1 = w->n1, n0, fh;
	wexp t1 = w->t1, t0 = 0;

	if (!W_IS_PAIR(w)) {
		if (fan_head(l, n1))
			lift_fan_head(l);
		else {
			lift_word(l, n1, &t1);
			w->t1 = t1;
		}
		WeightCheck(W_NLETTERS(w), "LiftWeight");
		return;
	}
	n0 = w->n0;
	memcpy(u1, l, (size_t)n1 * sizeof(letter));
	memcpy(u0, l + n1, (size_t)n0 * sizeof(letter));
	fh = fan_head(u1, n1);
	if (fh)
		lift_fan_head(u1);
	else
		lift_word(u1, n1, &t1);
	if (fh && fan_head(u0, n0))
		lift_fan_head(u0);
	else
		lift_word(u0, n0, &t0);
	join(w, u1, n1, t1, u0, n0);
	WeightCheck(W_NLETTERS(w), "LiftWeight");
}

/* left multiplication by a multiplicative letter (p or q): component 1 only */
void
PrefixWeight1(weight *w, int kind) {
	letter *l = WLETTERS(w);
	letter u1[MAXTOK], u0[MAXTOK];
	int n1 = w->n1, n0;

	if (!W_IS_PAIR(w)) {
		WeightCheck(n1 + 1, "PrefixWeight1");
		memmove(l + 1, l, (size_t)n1 * sizeof(letter));
		l[0] = MakePQ(kind, 0);
		w->n1 = (n1 + 1);
		w->one = 0;
		return;
	}
	n0 = w->n0;
	u1[0] = MakePQ(kind, 0);
	memcpy(u1 + 1, l, (size_t)n1 * sizeof(letter));
	memcpy(u0, l + n1, (size_t)n0 * sizeof(letter));
	join(w, u1, n1 + 1, w->t1, u0, n0);
	WeightCheck(W_NLETTERS(w), "PrefixWeight1");
}

/* left multiplication by the choice generator C_h = (c_h, 1): component 0 only */
void
PrefixChoice(weight *w, long h) {
	letter *l = WLETTERS(w);
	letter u0[MAXTOK];
	int n1 = w->n1, n0 = component0(w, u0);

	WeightCheck(n1 + n0 + 1, "PrefixChoice");
	memcpy(l + n1 + 1, u0, (size_t)n0 * sizeof(letter));
	l[n1] = MakeGen(L_C, h, 1, 0);
	w->n0 = (n0 + 1);
}

/* renames the choice generators of a weight: copies of a #def symbol get fresh
   choice codes, so that the codes stay injective on the choice branches of the
   term.  The two branches of a choice have the codes 2i (left) and 2i+1 (right):
   a family i is mapped to a fresh family i' (from/to hold family indices) and the
   branch parity is kept, as the read-back orders the branches by their codes. */
void
RenameChoices(weight *w, long *from, long *to, int *n, int max, long *counter) {
	letter *l = WLETTERS(w);
	long h, fam;
	int j, k;

	if (!W_IS_PAIR(w))
		return;
	for (j = w->n1; j < w->n1 + w->n0; j++) {
		if (LETTER_KIND(l[j]) != L_C)
			continue;
		h = GEN_INDEX(l[j]);
		fam = h / 2;
		for (k = 0; k < *n && from[k] != fam; k++)
			;
		if (k == *n) {
			if (*n >= max) {
				fprintf(stderr, "(%d) PELCR: too many choices in a copied symbol\n", rank);
				exit(-1);
			}
			if (*counter % 2 != 0)
				(*counter)++;
			from[k] = fam;
			to[k] = *counter / 2;
			*counter += 2;
			(*n)++;
		}
		l[j] = MakeGen(L_C, 2 * to[k] + h % 2, GEN_LIFT(l[j]), LETTER_EXP(l[j]));
	}
	WeightCheck(W_NLETTERS(w), "RenameChoices");
}

int
findType(int tipo) {
	int i = 0;
	while (i <= MAXTYPE) {
		if (t[i][0] == tipo)
			return i;
		i++;
	}
	printf("\n\tERRORE\n");
	return 0;
}
