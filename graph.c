/* Functions concerning the implementation of dynamic graphs

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

#include "var.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/**/

graph *
NewGraph() {
	graph *nS;
	nS = (graph *)calloc(1, sizeof(graph));
	if (!nS) {
		fprintf(logfile, "graph allocation failure - ABORT");
		return 0;
	}
	return nS;
}

node *
NewNode() {
	node *nS;

	nS = (node *)calloc(1, sizeof(node));
	if (!nS) {
		printf("ALLOCATION FAILURE !!!!!!!!!\n\n");
		fprintf(logfile, "node allocation failure - ABORT");
		fflush(logfile);
		return 0;
	}
	return nS;
}

edge *
NewReference() {
	edge *nS;

	nS = (edge *)calloc(1, sizeof(edge));

	if (!nS) {
		fprintf(logfile, "edge allocation failure - ABORT");
		return 0;
	}
	return nS;
}

static void PropagateColdToSourceEndpoints(edge *P);
static void AttachSingleEotHotNodesToColdLocal(void);
static void QueueRemoteColdSource(int dest, int creator, long source);
static void ExchangeRemoteColdSources(void);

typedef struct cold_source_ref {
	int dest;
	int creator;
	long source;
} cold_source_ref;

static cold_source_ref *remote_cold_sources = NULL;
static int remote_cold_source_count = 0;
static int remote_cold_source_capacity = 0;
static int collect_remote_cold_sources = 0;

static void
UnlinkNodeFromList(node **head, node *P) {
	if (P == NULL || head == NULL || *head == NULL)
		return;

	if (*head == P)
		*head = P->nextpuit;
	if (P->prevpuit != NULL)
		P->prevpuit->nextpuit = P->nextpuit;
	if (P->nextpuit != NULL)
		P->nextpuit->prevpuit = P->prevpuit;
	P->prevpuit = NULL;
	P->nextpuit = NULL;
}

void
AttachNodeToCold(node *P) {
	int was_hot;

	if (P == NULL)
		return;

	if (P->sto == OUT)
		return;

	was_hot = (P->sto != OUT);

	UnlinkNodeFromList(&G.hot, P);

	P->sto = OUT;
	P->prevpuit = NULL;
	P->nextpuit = G.cold;
	if (G.cold != NULL)
		G.cold->prevpuit = P;
	G.cold = P;

	if (was_hot) {
		PropagateColdToSourceEndpoints(P->left.vector);
		PropagateColdToSourceEndpoints(P->right.vector);
	}
}

static void
PropagateColdToSourceEndpoints(edge *P) {
	for (; P != NULL; P = P->vector) {
		P->sto = OUT;
		if (P->rankpuit == rank)
			StoreBookedAddress(P->creator, (long)P->source, OUT);
		else if (collect_remote_cold_sources)
			QueueRemoteColdSource(P->rankpuit, P->creator, (long)P->source);
	}
}

void
AttachSingleEotHotNodesToCold(void) {
	int previous_collect_remote_cold_sources;
	long local_remote_source_count;
	long global_remote_source_count;

	previous_collect_remote_cold_sources = collect_remote_cold_sources;
	collect_remote_cold_sources = (size > 1);

	AttachSingleEotHotNodesToColdLocal();

	if (size > 1) {
		do {
			local_remote_source_count = remote_cold_source_count;
			MPI_Allreduce(&local_remote_source_count, &global_remote_source_count, 1, MPI_LONG, MPI_SUM,
			              MPI_COMM_WORLD);
			if (global_remote_source_count == 0)
				break;
			ExchangeRemoteColdSources();
		} while (1);
	}

	collect_remote_cold_sources = previous_collect_remote_cold_sources;
}

static void
AttachSingleEotHotNodesToColdLocal(void) {
	int changed;

	do {
		node *cursor;

		changed = 0;
		for (cursor = G.hot; cursor != NULL; cursor = cursor->nextpuit) {
			if ((cursor->left.eot + cursor->right.eot) == 1) {
				AttachNodeToCold(cursor);
				changed = 1;
				break;
			}
		}
	} while (changed);
}

static void
QueueRemoteColdSource(int dest, int creator, long source) {
	cold_source_ref *new_refs;

	if (dest < 0 || dest >= size || dest == rank)
		return;

	if (remote_cold_source_count == remote_cold_source_capacity) {
		remote_cold_source_capacity = remote_cold_source_capacity ? 2 * remote_cold_source_capacity : 1024;
		new_refs = (cold_source_ref *)realloc(remote_cold_sources,
		                                      remote_cold_source_capacity * sizeof(cold_source_ref));
		if (new_refs == NULL) {
			fprintf(logfile, "remote cold source allocation failure - ABORT");
			exit(-1);
		}
		remote_cold_sources = new_refs;
	}

	remote_cold_sources[remote_cold_source_count].dest = dest;
	remote_cold_sources[remote_cold_source_count].creator = creator;
	remote_cold_sources[remote_cold_source_count].source = source;
	remote_cold_source_count++;
}

static void
ExchangeRemoteColdSources(void) {
	int *send_counts;
	int *recv_counts;
	int *send_displacements;
	int *recv_displacements;
	int *fill_offsets;
	long *send_buffer;
	long *recv_buffer;
	int total_send;
	int total_recv;
	int i;

	send_counts = (int *)calloc(size, sizeof(int));
	recv_counts = (int *)calloc(size, sizeof(int));
	send_displacements = (int *)calloc(size, sizeof(int));
	recv_displacements = (int *)calloc(size, sizeof(int));
	fill_offsets = (int *)calloc(size, sizeof(int));
	if (send_counts == NULL || recv_counts == NULL || send_displacements == NULL || recv_displacements == NULL
	    || fill_offsets == NULL) {
		fprintf(logfile, "remote cold exchange allocation failure - ABORT");
		exit(-1);
	}

	for (i = 0; i < remote_cold_source_count; i++)
		send_counts[remote_cold_sources[i].dest] += 2;

	MPI_Alltoall(send_counts, 1, MPI_INT, recv_counts, 1, MPI_INT, MPI_COMM_WORLD);

	total_send = 0;
	total_recv = 0;
	for (i = 0; i < size; i++) {
		send_displacements[i] = total_send;
		recv_displacements[i] = total_recv;
		fill_offsets[i] = total_send;
		total_send += send_counts[i];
		total_recv += recv_counts[i];
	}

	send_buffer = total_send ? (long *)calloc(total_send, sizeof(long)) : NULL;
	recv_buffer = total_recv ? (long *)calloc(total_recv, sizeof(long)) : NULL;
	if ((total_send && send_buffer == NULL) || (total_recv && recv_buffer == NULL)) {
		fprintf(logfile, "remote cold exchange buffer allocation failure - ABORT");
		exit(-1);
	}

	for (i = 0; i < remote_cold_source_count; i++) {
		int dest;
		int offset;

		dest = remote_cold_sources[i].dest;
		offset = fill_offsets[dest];
		send_buffer[offset] = remote_cold_sources[i].creator;
		send_buffer[offset + 1] = remote_cold_sources[i].source;
		fill_offsets[dest] += 2;
	}

	remote_cold_source_count = 0;
	MPI_Alltoallv(send_buffer, send_counts, send_displacements, MPI_LONG, recv_buffer, recv_counts,
	              recv_displacements, MPI_LONG, MPI_COMM_WORLD);

	for (i = 0; i + 1 < total_recv; i += 2)
		StoreBookedAddress((int)recv_buffer[i], recv_buffer[i + 1], OUT);

	free(send_counts);
	free(recv_counts);
	free(send_displacements);
	free(recv_displacements);
	free(fill_offsets);
	free(send_buffer);
	free(recv_buffer);
}

void
LiberaV(edge *P) {
	struct messaggio m;

	if (P != NULL) {
		if (P->vector != NULL)
			LiberaV(P->vector);
		if (P->sign == PLUS) {
			StoreMessage(&m, P, P, (char *)"*", P->sto, P->side);
			m.tpy = EOT_TAG;
			PushMessage(&m);
		};

		char *Xstr = NULL;
		int konst_index = 0;

		Xstr = strstr(P->weight, "X(2");

		if (Xstr != NULL) {
			konst_index = atoi(&Xstr[4]);
			k[konst_index][2]--;
		}

		free(P);
		graph_edges--;
	};
#ifdef _DEBUG
	DEBUG_MEM fprintf(logfile, "(%d) Exit Free Memory Procedure\n", rank);
#endif

	return;
}

void
AddEdge(node *S, int rk, node *nS, int sto, term *w, int c, int polarity, int side) {
	edge *p;
	seminode *v;

	if (sto == OUT) {
		if (rk == rank)
			StoreBookedAddress(c, (long)nS, OUT);
	}

	p = NewReference();
	if (p == NULL) {
		/*
		DEBUG fprintf(logfile,"(%d) REFERENCE ALLOCATION FAILURE - ABORTING\n",rank);
		fflush(logfile);
		*/
		exit(-1);
	} else {
		strcpy(p->weight, w);
		if (polarity == LEFT) {
			v = &(S->left);
		} else
			v = &(S->right);

		p->vector = v->vector;
		p->rankpuit = rk;
		p->sto = sto;
		p->side = side;
		p->sign = PLUS;
		p->source = nS;
		p->creator = c;
		v->vector = p;
		if (v->length != -1)
			v->length = (v->length) + 1;
		graph_edges++;

#ifdef _DEBUG
		DEBUG_MEM {
			ShowEdge(p);
			fprintf(logfile, "(%d) LENNODE[%p]: %d\n", rank, (void *)S, v->length);
			fflush(logfile);
		};
#endif

		combusted = 0;
		return;
	};
}

#ifdef NEWVERSION

/*
 print details for the weight, specially for external symbols
 new representation for weights = words of handlers
  one handle is
      1) bytecode          = code for the action
      2) exception handler = address for the symbol
      3) code handler      = address to the library code (obtained by the header)
  bytecode = <depth,name,address,lift> [d,n,a,l]
*/

void
ShowWeight(char *s) {
	switch ((int)s) {
		case P:;
			break;
		case Q:;
			break;
		case W:;
			break;
		case X:;
			break;
	}
	return;
}

#endif

void
ShowEdge(edge *e) {
	fprintf(logfile, "(%d) target-hosting-process (%d), the has in/out flag %d\n", rank, e->rankpuit, e->sto);
	fprintf(logfile, "          creator (%d)\n", e->creator);

	fprintf(logfile, "          on side %d of the source (%ld)[%p]\n", e->side, (long)e->source, e->source);

	fprintf(logfile, "          weight |%s| and", e->weight);
	fprintf(logfile, " status one-opt. sign %d\n", e->sign);
	fprintf(logfile, "          next edge on the same node [%p] \n", e->vector);
	fflush(logfile);
}

node *
CreateNewNode(node *S) {
	node *nS;
#ifdef _DEBUG
	DEBUG_MEM {
		fprintf(logfile, "IN routine creazione \n");
		fflush(logfile);
	};
#endif

	nS = NewNode();

#ifdef _DEBUG
	DEBUG_MEM {
		fprintf(logfile, "(%d) Insert new node [%p] in S = [%p]\n", rank, nS, S);
		fflush(logfile);
	};
#endif

	nS->left.eot = 0;
	nS->left.weot = 1;
	nS->left.length = 0;
	nS->left.vector = NULL;
	nS->left.dejavu = NULL;

	nS->right.eot = 0;
	nS->right.weot = 1;
	nS->right.length = 0;
	nS->right.vector = NULL;
	nS->right.dejavu = NULL;

	nS->nextpuit = S;
	nS->sto = IN;
#if FREE_HOT_BOOKTABLE_ENTRIES
	nS->has_booktable_entry = 0;
	nS->booktable_rank = -1;
	nS->booktable_key = 0;
#endif
	nS->families = 0;
	nS->prevpuit = NULL;
	if (S != NULL)
		S->prevpuit = nS;
	graph_nodes++;
#ifdef _DEBUG
	DEBUG_MEM {
		fprintf(logfile, "(%d) NEXT([%p])=[%p]\n", rank, nS, nS->nextpuit);
		fprintf(logfile, "(%d) PREV([%p])=[%p]\n", rank, nS, nS->prevpuit);
		if (S != NULL) {
			fprintf(logfile, "(%d) NEXT([%p])=[%p]\n", rank, S, S->nextpuit);
			fprintf(logfile, "(%d) PREV([%p])=[%p]\n", rank, S, S->prevpuit);
		};
	};
#endif
	return nS;
}

node *
CreateNewBoundary(node *S) {
	node *nS;

	nS = NewNode();
	nS->sto = OUT;
#if FREE_HOT_BOOKTABLE_ENTRIES
	nS->has_booktable_entry = 0;
	nS->booktable_rank = -1;
	nS->booktable_key = 0;
#endif
	nS->left.length = -1;
	nS->left.vector = NULL;
	nS->left.dejavu = NULL;
	nS->nextpuit = S;
	S->prevpuit = nS;
	return nS;
}

node *
SinkRemove(node *P) {
	node *father_of_P = P->prevpuit;

#ifdef _DEBUG
	DEBUG_MEM fprintf(logfile, "(%d) REMOVE NODE P=[%p]\n", rank, P);
#endif
	family_reductions += P->families;

	if ((P->left.vector != NULL) || (P->right.vector != NULL)) {
#ifdef _DEBUG
		DEBUG_MEM {
			fprintf(logfile, "(%d) Now I  clean the ghost node 'cause", rank);
			fprintf(logfile, " its  combustion is completely exausted\n");
		}
#endif
		LiberaV(P->left.vector);
		LiberaV(P->right.vector);
	} else {
		/*
		DEBUG fprintf(logfile,"(%d) disconnected point checked\n",rank);
		*/
	};
	if (G.hot == P) {
		G.hot = P->nextpuit;
		if (G.hot != NULL)
			G.hot->prevpuit = NULL;
#ifdef _DEBUG
		DEBUG_MEM {
			fprintf(logfile, "(%d) ok 1\n", rank);
			fprintf(logfile, "(%d) Graph HOT=[%p]\n", rank, G.hot);
		};
#endif
	} else {
#ifdef _DEBUG
		DEBUG_MEM {
			father_of_P = P->prevpuit;
			fprintf(logfile, "(%d) ok 2\n", rank);
			fprintf(logfile, "(%d) NEXT(FATHER(P=[%p])=[%p])", rank, P, father_of_P);
			if (father_of_P != NULL)
				fprintf(logfile, "= [%p]\n", father_of_P->nextpuit);
			else
				fprintf(logfile, "= NULL FATHER\n");
			fflush(logfile);
		};
#endif
		P->prevpuit->nextpuit = P->nextpuit;
		if (P->nextpuit != NULL)
			P->nextpuit->prevpuit = P->prevpuit;
	};
#if FREE_HOT_BOOKTABLE_ENTRIES
	ReleaseBookedAddress(P);
#endif
	free(P);
	graph_nodes--;
#ifdef _DEBUG
	DEBUG_MEM {
		fprintf(logfile, "REMOVED NODE\n");
		fprintf(logfile, "(%d) ok 3\n", rank);
		fprintf(logfile, "(%d) ok 4 [%p]\n", rank, father_of_P);
		fflush(logfile);
	}
#endif
	return father_of_P;
}

edge *
EdgeRemove(edge *e, node *P) {
	edge *p, *pp;

	if (P->left.vector == NULL) {
		return 0;
	} else {
		pp = NULL;
		p = P->left.vector;
		for (; ((p != e) && (p != NULL));) {
			pp = p;
			p = p->vector;
		};
		if ((p != NULL) && (pp != NULL)) {
			pp->vector = p->vector;
			pp = P->left.vector;
		} else if ((p != NULL) && (pp == NULL)) {
			pp = p->vector;
		} else {
			fprintf(logfile, "ERROR!\n ");
			return 0;
		};

		free(e);
		graph_edges--;
		P->left.length = P->left.length - 1;
		return pp;
	};
}
