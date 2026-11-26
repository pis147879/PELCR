/* ADDREV: the contraction of the chains of unary edges

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

/* An edge B (v -> n) that arrives at n and is unary is not combusted with its weight: it stays at n
   with weight 1, and (B, ADDREV) is sent to its source v.  The edge of weight 1 composes at n with
   the counter-edges C of B, and the optimization of one moves them to v (c -> v, weight C), on the
   side where the residuals of B would arrive; the true residuals of B do not exist, since B is not
   combusted.  At v, the composition of (B, ADDREV) with an edge A of the other side (present or
   future) only changes the weight of A into BA (first B, then A), and A goes on normally: if it is
   unary too, it stays at v with weight 1 and (BA, ADDREV) goes on to its source.  The counter-edges
   moved to v compose there with BA.  ADDREV reaches v before the counter-edges moved by the edge of
   weight 1 (the messages from a process to another keep their order, as for the EOT).

   The mechanism is not applied when the source or the target of B is cold (the source is then a
   node of the final graph, where the paths between its two out-edges must keep the weight B), to
   the edges whose source is a node of the graph builder, to the residuals that leave their node on
   the RIGHT side (a node holds at most one ADDREV), and to weights longer than PELCR_REV_CAP
   letters (at most MAXLETTERS/2, so that BA fits): the reduction stays the same without it.  Unary:
   B is the only edge of its side of n, which it closes (EOTADD_TAG: no other edge will arrive).
   (B, ADDREV) is kept at v as an edge of its seminode with sign REVSIGN: it sends no EOT, it is
   freed with v, and the read-back, the prints and the cold pass skip it.  PELCR_REV=1 enables it. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "h/var.h"

long rev_sent = 0;       /* ADDREV sent */
long rev_applied = 0;    /* weights changed into BA */
long rev_forwarded = 0;  /* edges A that went on with (BA, ADDREV) */
long rev_cold = 0;       /* ADDREV that found its node already cold */
long rev_late_cold = 0;  /* nodes holding an ADDREV that become cold afterwards */
long rev_maxletters = 0; /* the longest weight carried by an ADDREV */
long rev_maxsemi = 0;    /* the most ADDREV held by a seminode */
long rev_toolong = 0;    /* unary edges not optimized: their weight is longer than the limit */

static int rev_enabled = -1;
static int rev_cap = 0;

int
RevEnabled(void) {
	if (rev_enabled < 0) {
		const char *e = getenv("PELCR_REV");
		const char *c = getenv("PELCR_REV_CAP");
		rev_enabled = (e != NULL && e[0] != '\0' && e[0] != '0');
		/* the limit to the accumulation of the unary edges: a weight already longer than
		   REVLETTERS letters is no longer optimized (BA must also fit in MAXLETTERS) */
		rev_cap = (c != NULL) ? atoi(c) : REVLETTERS;
		if (rev_cap > MAXLETTERS / 2)
			rev_cap = MAXLETTERS / 2;
	}
	return rev_enabled;
}

/* the edges PLUS of a seminode, not counting the ADDREV it holds */
static int
rev_edges(const edge *e) {
	int k = 0;
	for (; e != NULL; e = e->vector)
		k += (e->sign != REVSIGN);
	return k;
}

static int
rev_eligible(int creator, int sourceside, int sourcesto, const node *n, const weight *w, int unary) {
	if (!(unary && (creator != size) && (sourceside == LEFT) && (sourcesto != OUT) && (n->sto != OUT)
	      && !WeightIsOne(w)))
		return 0;
	if (W_NLETTERS(w) > rev_cap) { /* the weight is already too long: no more optimization */
		rev_toolong++;
		return 0;
	}
	return 1;
}

/* the ADDREV of an edge (source given by its fields, target n on side tside) with weight w: to
   the source of the edge, where it arrives on the side the edge leaves from */
static void
rev_send(int rk, int creator, node *source, int sto, int sside, const struct messaggio *mt, const node *n,
         int tside, const weight *w) {
	struct messaggio r;

	memset(&r, 0, sizeof(r));
	r.tpy = ADD_REV_TAG;
	r.sender_load = incoming_actions_snapshot;
	r.vtarget.rankpuit = rk;
	r.vtarget.creator = creator;
	r.vtarget.source = source;
	r.vtarget.sto = sto;
	r.side = sside;
	r.vsource.rankpuit = rank;
	r.vsource.creator = mt->vtarget.creator;
	r.vsource.source = mt->vtarget.source;
	r.vsource.sto = n->sto;
	r.vsource.side = tside;
	MessageSetWeight(&r, w); /* r.wlong is NULL (memset) */
	PushMessage(&r);
	rev_sent++;
	if (W_NLETTERS(w) > rev_maxletters)
		rev_maxletters = W_NLETTERS(w);
}

/* an edge arrives at n (ADD_TAG), before it is stored: the ADDREV of n on the other side change its
   weight (in their order of arrival: the list keeps the last one first), then a unary edge stays
   with weight 1 and its ADDREV goes to its source */
void
RevFilterAdd(struct messaggio *m, node *n) {
	edge *revs[64], *e;
	int k, nr = 0;
	wbuf t;

	if (!RevEnabled())
		return;
	for (e = (m->side == LEFT) ? n->right.vector : n->left.vector; e != NULL; e = e->vector)
		if (e->sign == REVSIGN) {
			if (nr == 64) {
				fprintf(stderr, "(%d) PELCR: more than 64 ADDREV on a seminode\n", rank);
				exit(-1);
			}
			revs[nr++] = e;
		}
	for (k = nr - 1; k >= 0; k--) {
		WeightConcat(&revs[k]->weight, MessageWeight(m), &t.h);
		MessageSetWeight(m, &t.h);
		rev_applied++;
	}
	/* the present storage of the source, when it is a node of this process created here (its
	   address is then a pointer: StoreBookedAddress), else the one the message carries */
	/* unary: the edge is the last one of its side (EOTADD_TAG) and the side has no other edge */
	if (rev_eligible(m->vsource.creator, m->vsource.side,
	                 (m->vsource.creator == rank && m->vsource.rankpuit == rank) ? ((node *)m->vsource.source)->sto
	                                                                           : m->vsource.sto,
	                 n, MessageWeight(m),
	                 (m->tpy == EOTADD_TAG) && (rev_edges((m->side == LEFT) ? n->left.vector : n->right.vector) == 0))) {
		rev_send(m->vsource.rankpuit, m->vsource.creator, m->vsource.source, m->vsource.sto, m->vsource.side, m, n,
		         m->side, MessageWeight(m));
		WeightSetOne(&t.h);
		MessageSetWeight(m, &t.h);
	}
}

/* the edge p of the list *pp gets the weight w (its letters follow the edge: a new edge) */
static edge *
rev_reweight(edge **pp, edge *p, const weight *w) {
	edge *q;
	for (; *pp != NULL && *pp != p; pp = &(*pp)->vector)
		;
	if (*pp == NULL)
		return p;
	q = NewReferenceLetters(W_NLETTERS(w));
	q->vector = p->vector;
	q->source = p->source;
	q->sto = p->sto;
	q->sign = p->sign;
	q->side = p->side;
	q->rankpuit = p->rankpuit;
	q->creator = p->creator;
	WeightCopy(&q->weight, w);
	*pp = q;
	free(p);
	return q;
}

/* (B, ADDREV) arrives at v: it becomes an edge of v (sign REVSIGN) on the side B leaves from, the
   side of the residuals of B; its composition with the edges A of the other side changes their
   weight into BA, and A goes on: if it is unary it stays with weight 1 and (BA, ADDREV) goes on */
void
RevArrive(struct messaggio *m) {
	node *v = StoreBookedAddress(m->vtarget.creator, (long)m->vtarget.source, m->vtarget.sto);
	seminode *sv = (m->side == LEFT) ? &v->left : &v->right;
	seminode *so = (m->side == LEFT) ? &v->right : &v->left;
	edge *r, *p;

	if (v->sto == OUT)
		rev_cold++;
	r = NewReferenceLetters(W_NLETTERS(MessageWeight(m)));
	WeightCopy(&r->weight, MessageWeight(m));
	r->source = m->vsource.source;
	r->creator = m->vsource.creator;
	r->rankpuit = m->vsource.rankpuit;
	r->sto = m->vsource.sto;
	r->side = m->vsource.side;
	r->sign = REVSIGN;
	r->vector = sv->vector;
	sv->vector = r;
	graph_edges++;
	{
		long k = 0;
		for (p = sv->vector; p != NULL; p = p->vector)
			k += (p->sign == REVSIGN);
		if (k > rev_maxsemi)
			rev_maxsemi = k;
	}

	for (p = so->vector; p != NULL; p = p->vector) {
		wbuf t;
		if (p->sign == REVSIGN)
			continue;
		WeightConcat(&r->weight, &p->weight, &t.h);
		rev_applied++;
		/* unary: the side of p is complete and has only p; p not discharged yet (it can still be
		   composed: its EOT has not been sent) */
		if (rev_eligible(p->creator, p->side, p->sto, v, &t.h,
		                 (p->sign == PLUS) && (so->eot == 1) && (rev_edges(so->vector) == 1))) {
			struct messaggio a;
			memset(&a, 0, sizeof(a));
			a.vtarget.creator = rank;
			a.vtarget.source = v;
			rev_send(p->rankpuit, p->creator, p->source, p->sto, p->side, &a, v, (m->side == LEFT) ? RIGHT : LEFT, &t.h);
			rev_forwarded++;
			WeightSetOne(&t.h);
		}
		p = rev_reweight(&so->vector, p, &t.h);
	}
}

/* AttachNodeToCold: a node that holds an ADDREV becomes cold (statistics) */
void
RevNoteCold(node *v) {
	edge *e;
	for (e = v->left.vector; e != NULL; e = e->vector)
		if (e->sign == REVSIGN) {
			rev_late_cold++;
			return;
		}
	for (e = v->right.vector; e != NULL; e = e->vector)
		if (e->sign == REVSIGN) {
			rev_late_cold++;
			return;
		}
}

void
RevPrintStats(void) {
	if (RevEnabled())
		printf("(%d) ADDREV              :  %ld sent, %ld weights changed, %ld forwarded, %ld at cold nodes, "
		       "%ld become cold later, longest %ld letters, at most %ld on a seminode, %ld not done (weight longer "
		       "than %d letters)\n",
		       rank, rev_sent, rev_applied, rev_forwarded, rev_cold, rev_late_cold, rev_maxletters, rev_maxsemi,
		       rev_toolong, rev_cap);
}
