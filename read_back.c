/* Read-back functions

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Read-back.

   At the end of a run the final graph is the dump of the machine: its paths
   between boundary ports, followed by chains, form the observation Obs, and at
   quiescence Obs = EX(tau(t)) (ND-PELCR article, Section "PELCR execution and
   observation").  The normal form, choices included, is read back from Obs by the
   token decoder of the article (agents/readback/pelcr_c_readback.py, which uses
   pelcr_readback.py of ND-PELCR).

   read_back writes the final graph in JSON when a file was requested with
   -readback <file> (or the environment variable PELCR_READBACK_JSON):
     nodes: id ("rank:address"), part (hot/cold), sto, eot;
     edges: target, target_side (LEFT 1 / RIGHT 0), source, source_side, weight,
            with the weights of Lambda*_nd ("u1|u0", or "u1" for (erase(u1),u1)).
   The source of an edge is resolved to the address of the node it denotes: the
   address itself when the edge was created on this rank, otherwise the book table
   of its creator (keys of the graph builder have creator = size).  Single rank only. */

static void
node_id(char *buf, size_t n, node *v) {
	snprintf(buf, n, "%d:%lx", rank, (unsigned long)v);
}

static void
source_id(char *buf, size_t n, edge *e) {
	node *address = NULL;
	HashTable *table;

	if (e->creator == rank) {
		node_id(buf, n, e->source);
		return;
	}
	table = (e->creator >= 0 && e->creator <= size) ? BookTable[e->creator] : NULL;
	if (table != NULL && table->size > 0 && table_iget(table, (unsigned long)e->source, &address) && address != NULL)
		node_id(buf, n, address);
	else
		snprintf(buf, n, "%d:key:%lx", e->creator, (unsigned long)e->source);
}

static long
write_part(FILE *f, node *head, const char *part, int *first) {
	node *v;
	long count = 0;
	char id[64];

	for (v = head; v != NULL; v = v->nextpuit) {
		node_id(id, sizeof(id), v);
		fprintf(f, "%s\n  {\"id\": \"%s\", \"part\": \"%s\", \"sto\": %d, \"eot\": [%d, %d]}", *first ? "" : ",", id,
		        part, v->sto, v->left.eot, v->right.eot);
		*first = 0;
		count++;
	}
	return count;
}

static long
write_edges(FILE *f, node *head, int *first) {
	node *v;
	edge *e;
	long count = 0;
	char tid[64], sid[64];
	int side;

	for (v = head; v != NULL; v = v->nextpuit) {
		node_id(tid, sizeof(tid), v);
		for (side = 0; side < 2; side++) {
			e = (side == 0) ? v->left.vector : v->right.vector;
			for (; e != NULL; e = e->vector) {
				source_id(sid, sizeof(sid), e);
				fprintf(f,
				        "%s\n  {\"target\": \"%s\", \"target_side\": %d, \"source\": \"%s\", \"source_side\": %d, "
				        "\"sign\": %d, \"sto\": %d, \"rankpuit\": %d, \"creator\": %d, \"weight\": \"%s\"}",
				        *first ? "" : ",", tid, (side == 0) ? LEFT : RIGHT, sid, e->side, e->sign, e->sto, e->rankpuit,
				        e->creator, e->weight);
				*first = 0;
				count++;
			}
		}
	}
	return count;
}

static void
write_readback_dump(node *principal, const char *path) {
	FILE *f;
	char name[MAXNAMELEN + 16];
	char pid[64];
	long nnodes, nedges;
	int first;

	if (size == 1)
		snprintf(name, sizeof(name), "%s", path);
	else {
		snprintf(name, sizeof(name), "%s.%d", path, rank);
		if (rank == 0)
			printf("(%d) read-back: %d ranks, the dumps of the ranks are written separately (the decoder needs one "
			       "rank)\n",
			       rank, size);
	}
	f = fopen(name, "w");
	if (f == NULL) {
		printf("(%d) read-back: cannot write %s\n", rank, name);
		return;
	}
	if (principal != NULL)
		node_id(pid, sizeof(pid), principal);
	else
		strcpy(pid, "");
	fprintf(f, "{\"format\": \"pelcr-dump-1\", \"rank\": %d, \"size\": %d, \"principal\": \"%s\",", rank, size, pid);
	fprintf(f, " \"LEFT\": %d, \"RIGHT\": %d,", LEFT, RIGHT);
	fprintf(f,
	        "\n \"stats\": {\"edge_compositions\": %ld, \"fires\": %ld, \"one_optimizations\": %ld, "
	        "\"graph_edges\": %ld, \"max_weight_length\": %ld},",
	        edge_compositions, fires, one_optimizations, graph_edges, max_weight_length);
	/* free variables: the builder's var nodes, booked under creator = size */
	fprintf(f, "\n \"free\": [");
	first = 1;
	{
		int k;
		node *address;
		char vid[64];
		for (k = 0; k < nfreevars; k++) {
			address = NULL;
			if (BookTable[size] == NULL || BookTable[size]->size <= 0
			    || !table_iget(BookTable[size], (unsigned long)freevar_key[k], &address) || address == NULL)
				continue;
			node_id(vid, sizeof(vid), address);
			fprintf(f, "%s{\"id\": \"%s\", \"name\": \"%s\"}", first ? "" : ", ", vid, freevar_name[k]);
			first = 0;
		}
	}
	fprintf(f, "],");
	fprintf(f, "\n \"nodes\": [");
	first = 1;
	nnodes = write_part(f, G.hot, "hot", &first);
	nnodes += write_part(f, G.cold, "cold", &first);
	fprintf(f, "\n ],\n \"edges\": [");
	first = 1;
	nedges = write_edges(f, G.hot, &first);
	nedges += write_edges(f, G.cold, &first);
	fprintf(f, "\n ]\n}\n");
	fclose(f);
	printf("(%d) read-back: final graph (%ld nodes, %ld edges) written to %s\n", rank, nnodes, nedges, name);
}

void
read_back(node *principal) {
	const char *env = getenv("PELCR_READBACK_JSON");

	printf("(%d) ********** principal port address %p\n", rank, (void *)principal);
	if (readbackfile[0] != '\0')
		write_readback_dump(principal, readbackfile);
	else if (env != NULL && env[0] != '\0')
		write_readback_dump(principal, env);
}
