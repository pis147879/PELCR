/* Weights of the dynamic algebra as words of integer letters (see h/weight.h)

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "h/var.h"

long max_weight_length = 0;

static void
letter_overflow(const char *what, long long v) {
	fprintf(stderr, "(%d) PELCR: %s out of range in a letter of a weight: %lld (see h/weight.h)\n", rank, what, v);
	exit(-1);
}

void
WeightExpCheck(wexp m) {
	if (m < 0 || m > WEXP_MAX)
		letter_overflow("index of the exponential", m);
}

letter
MakePQ(int kind, wexp m) {
	WeightExpCheck(m);
	return (letter)kind | ((letter)m << 3);
}

letter
MakeGen(int kind, long index, long lift, wexp m) {
	WeightExpCheck(m);
	if (index < 0 || index > 0x1FFF)
		letter_overflow("index of a generator", index);
	if (lift < 0 || lift > 255)
		letter_overflow("lift of a generator", lift);
	return (letter)kind | ((letter)m << 3) | ((letter)index << 43) | ((letter)lift << 56);
}

letter
MakeX(long sub, long value, wexp m) {
	WeightExpCheck(m);
	if (sub < 0 || sub > 7)
		letter_overflow("kind of an external letter", sub);
	if (value < -XL_OFFSET || value >= XL_OFFSET)
		letter_overflow("value of an external letter", value);
	return (letter)L_X | ((letter)m << 3) | ((letter)sub << 43) | ((letter)(value + XL_OFFSET) << 46);
}

/* "!n" (a token of the product): n in the index field */
letter
MakeBang(wexp n) {
	WeightExpCheck(n);
	return (letter)L_BANG | ((letter)n << 3);
}

/* the number of letters of a weight: statistics, and the end of the run when it does
   not fit (MAXLETTERS) */
void
WeightCheck(int nletters, const char *what) {
	if (nletters > max_weight_length)
		max_weight_length = nletters;
	if (nletters > MAXLETTERS) {
		fprintf(stderr, "(%d) PELCR: weight too long in %s: more than %d letters\n", rank, what, MAXLETTERS);
		fprintf(stderr, "(%d) PELCR: rebuild with a larger MAXLETTERS (make MAXLETTERS=...)\n", rank);
		exit(-1);
	}
}

void
WeightSetOne(weight *w) {
	w->n1 = 0;
	w->n0 = W_CLASSICAL;
	w->t1 = 0;
	w->one = 0;
}

void
WeightCopy(weight *dst, const weight *src) {
	if (dst != src)
		memmove(dst, src, W_BYTES(src));
}

/* no letter p, q, w, c, X in either component */
int
WeightIsOne(const weight *w) {
	return (w->n1 == 0) && (!W_IS_PAIR(w) || w->n0 == 0);
}

/* ---- string form ---- */

static const char *
parse_letter(const char *s, letter *x, wexp m, const char *all) {
	char *r;
	long i, l;
	int kind = (s[0] == 'w') ? L_W : (s[0] == 'c') ? L_C : L_X;

	if (s[1] != '(')
		goto bad;
	i = strtol(s + 2, &r, 10);
	if (*r != ',')
		goto bad;
	l = strtol(r + 1, &r, 10);
	if (*r != ')')
		goto bad;
	*x = (kind == L_X) ? MakeX(i, l, m) : MakeGen(kind, i, l, m);
	return r + 1;
bad:
	fprintf(stderr, "(%d) PELCR: malformed weight \"%s\"\n", rank, all);
	exit(-1);
}

/* a weight from its string form: "u1" for the pair (erase(u1),u1), or "u1|u0" */
void
WeightParse(const char *s, weight *w) {
	letter *l = WLETTERS(w);
	const char *p = s;
	char *r;
	wexp m = 0;
	int n = 0;

	WeightSetOne(w);
	while (*p != '\0' && *p != '|') {
		if (*p == '!') {
			m += strtoll(p + 1, &r, 10);
			p = r;
		} else if (*p == 'p' || *p == 'q') {
			WeightCheck(n + 1, "WeightParse");
			l[n++] = MakePQ((*p == 'p') ? L_P : L_Q, m);
			p++;
		} else if (*p == 'w' || *p == 'c' || *p == 'X') {
			WeightCheck(n + 1, "WeightParse");
			p = parse_letter(p, &l[n++], m, s);
		} else if (*p == '1' || *p == '*') /* the identity, the mark of an EOT */
			p++;
		else {
			fprintf(stderr, "(%d) PELCR: malformed weight \"%s\"\n", rank, s);
			exit(-1);
		}
	}
	WeightExpCheck(m);
	w->n1 = n;
	w->t1 = m;
	w->one = (n == 0 && m == 0 && strcmp(s, "1") == 0);
	if (*p == '|') {
		/* component 0: its generators (erase) */
		int n0 = 0;
		p++;
		m = 0;
		while (*p != '\0') {
			letter x;
			if (*p == '!') {
				m += strtoll(p + 1, &r, 10);
				p = r;
			} else if (*p == 'w' || *p == 'c' || *p == 'X') {
				p = parse_letter(p, &x, m, s);
				if (LETTER_KIND(x) != L_X) {
					WeightCheck(n + n0 + 1, "WeightParse");
					l[n + n0++] = x;
				}
			} else
				p++;
		}
		w->n0 = n0;
	}
	WeightCheck(W_NLETTERS(w), "WeightParse");
}

static int
append(char *buf, size_t size, size_t *pos, const char *fmt, long long a, long long b) {
	int k;
	if (*pos >= size)
		return 0;
	k = snprintf(buf + *pos, size - *pos, fmt, a, b);
	if (k < 0 || (size_t)k >= size - *pos) {
		*pos = size;
		return 0;
	}
	*pos += (size_t)k;
	return 1;
}

/* the string form of a word: "!n" before a letter whose index exceeds the previous one,
   and at the end when t exceeds the index of the last letter */
char *
WordToString(const letter *l, int n, wexp t, char *buf, size_t size) {
	size_t pos = 0;
	wexp cur = 0;
	int k;

	buf[0] = '\0';
	for (k = 0; k < n; k++) {
		wexp m = LETTER_EXP(l[k]);
		if (LETTER_KIND(l[k]) != L_BANG && m > cur) {
			append(buf, size, &pos, "!%lld", m - cur, 0);
			cur = m;
		}
		switch (LETTER_KIND(l[k])) {
			case L_P:
				append(buf, size, &pos, "p", 0, 0);
				break;
			case L_Q:
				append(buf, size, &pos, "q", 0, 0);
				break;
			case L_W:
				append(buf, size, &pos, "w(%lld,%lld)", GEN_INDEX(l[k]), GEN_LIFT(l[k]));
				break;
			case L_C:
				append(buf, size, &pos, "c(%lld,%lld)", GEN_INDEX(l[k]), GEN_LIFT(l[k]));
				break;
			case L_X:
				append(buf, size, &pos, "X(%lld,%lld)", XL_SUB(l[k]), XL_VALUE(l[k]));
				break;
			case L_BANG:
				append(buf, size, &pos, "!%lld", BANG_VALUE(l[k]), 0);
				break;
			default:
				append(buf, size, &pos, "?", 0, 0);
		}
	}
	if (t > cur)
		append(buf, size, &pos, "!%lld", t - cur, 0);
	return buf;
}

/* the string form of a weight, as before the integer letters: "u1", or "u1|u0" with 1 for
   a component without letters */
char *
WeightToString(const weight *w, char *buf, size_t size) {
	const letter *l = WLETTERS_C(w);
	size_t pos;

	if (!W_IS_PAIR(w)) {
		if (w->one && w->n1 == 0 && w->t1 == 0) {
			snprintf(buf, size, "1");
			return buf;
		}
		return WordToString(l, w->n1, w->t1, buf, size);
	}
	if (w->n1 == 0 && w->t1 == 0)
		snprintf(buf, size, "1");
	else
		WordToString(l, w->n1, w->t1, buf, size);
	pos = strlen(buf);
	if (pos + 2 < size) {
		buf[pos++] = '|';
		buf[pos] = '\0';
		if (w->n0 == 0)
			snprintf(buf + pos, size - pos, "1");
		else
			WordToString(l + w->n1, w->n0, 0, buf + pos, size - pos);
	}
	return buf;
}

/* the string forms of the two components (u0: its generators), as nd_split gave them */
void
WeightComponents(const weight *w, char *u1, size_t size1, char *u0, size_t size0) {
	const letter *l = WLETTERS_C(w);
	letter e[MAXLETTERS];
	int k, n = 0;

	if (W_IS_PAIR(w) && w->n1 == 0 && w->t1 == 0)
		snprintf(u1, size1, "1");
	else
		WordToString(l, w->n1, w->t1, u1, size1);
	if (W_IS_PAIR(w)) {
		for (k = 0; k < w->n0; k++)
			e[n++] = l[w->n1 + k];
	} else
		for (k = 0; k < w->n1; k++)
			if (IS_GEN_LETTER(l[k]))
				e[n++] = l[k];
	WordToString(e, n, 0, u0, size0);
}

/* ---- external letters ---- */

/* the first external letter X(sub,.) of component 1 (-1 if none) */
int
WeightFindX(const weight *w, int sub) {
	const letter *l = WLETTERS_C(w);
	int k;
	for (k = 0; k < w->n1; k++)
		if (LETTER_KIND(l[k]) == L_X && XL_SUB(l[k]) == sub)
			return k;
	return -1;
}

/* writes the value of the external letter k and cuts the weight after it: the string
   form did sprintf(&X[4], "%d)", value) on the letter, which ends the string there */
void
WeightSetXAndTruncate(weight *w, int k, long value) {
	letter *l = WLETTERS(w);
	wexp m = LETTER_EXP(l[k]);
	l[k] = MakeX(XL_SUB(l[k]), value, m);
	w->n1 = k + 1;
	w->t1 = m;
	w->n0 = W_CLASSICAL;
}

/* the index of the constant of the first letter X(KONST,.) of a word (-1 if none) */
long
WeightKonstIndex(const letter *l, int n) {
	int k;
	for (k = 0; k < n; k++)
		if (LETTER_KIND(l[k]) == L_X && XL_SUB(l[k]) == KONST)
			return XL_VALUE(l[k]);
	return -1;
}
