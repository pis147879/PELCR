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

/* the node denoted by the source of an edge, NULL if it cannot be resolved */
static node *
source_node(edge *e) {
	node *address = NULL;
	HashTable *table;

	if (e->creator == rank)
		return e->source;
	table = (e->creator >= 0 && e->creator <= size) ? BookTable[e->creator] : NULL;
	if (table != NULL && table->size > 0 && table_iget(table, (unsigned long)e->source, &address))
		return address;
	return NULL;
}

static void
source_id(char *buf, size_t n, edge *e) {
	node *address = source_node(e);

	if (address != NULL)
		node_id(buf, n, address);
	else
		snprintf(buf, n, "%d:key:%lx", e->creator, (unsigned long)e->source);
}

/* the node of the k-th free variable, NULL if it is not in the final graph */
static node *
free_var_node(int k) {
	node *address = NULL;

	if (BookTable[size] == NULL || BookTable[size]->size <= 0
	    || !table_iget(BookTable[size], (unsigned long)freevar_key[k], &address))
		return NULL;
	return address;
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
			address = free_var_node(k);
			if (address == NULL)
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

/* ---------------------------------------------------------------------------
   The final graph in GML (-gml <file>, or PELCR_FINAL_GML).  Same conventions
   as the snapshots of Print (print.c), so that the tools in agents/gml read it,
   but complete and readable by any GML tool:
     - nodes are numbered 0, 1, ...; the label is "root", the name of a free
       variable, or the address; fill #FFFF0F without edges, #AAAA00 with edges,
       #00AA00 for the root, #00AAAA for the free variables;
     - an edge goes from its source to its target and is stored on the LEFT
       (#aa0000) or RIGHT (#0000aa) side of the target; its sources are resolved
       to nodes as in the JSON dump; sourceSide / targetSide are 1 (LEFT) or 0
       (RIGHT); label is the weight of Lambda*_nd, weight1 and weight0 its two
       components.
   --------------------------------------------------------------------------- */

static int
compare_nodes(const void *a, const void *b) {
	node *x = *(node *const *)a, *y = *(node *const *)b;
	return (x > y) - (x < y);
}

static long
node_index(node **sorted, long n, node *v) {
	node **hit = (v == NULL) ? NULL : (node **)bsearch(&v, sorted, (size_t)n, sizeof(node *), compare_nodes);
	return (hit == NULL) ? -1 : (long)(hit - sorted);
}

static void
gml_string(FILE *f, const char *s) {
	fputc('"', f);
	for (; *s != '\0'; s++)
		if (*s != '"' && *s != '&')
			fputc(*s, f);
	fputc('"', f);
}

static void
write_final_gml(node *principal, const char *path) {
	FILE *f;
	char name[MAXNAMELEN + 16];
	node *v, **sorted;
	edge *e;
	long n = 0, i, nedges = 0, nextra = 0;
	long *order; /* node index -> position in the lists (ids follow the lists) */
	int side, k;

	if (size == 1)
		snprintf(name, sizeof(name), "%s", path);
	else
		snprintf(name, sizeof(name), "%s.%d", path, rank);
	f = fopen(name, "w");
	if (f == NULL) {
		printf("(%d) gml: cannot write %s\n", rank, name);
		return;
	}
	for (v = G.hot; v != NULL; v = v->nextpuit)
		n++;
	for (v = G.cold; v != NULL; v = v->nextpuit)
		n++;
	sorted = (node **)malloc((size_t)(n + 1) * sizeof(node *));
	order = (long *)malloc((size_t)(n + 1) * sizeof(long));
	i = 0;
	for (v = G.hot; v != NULL; v = v->nextpuit)
		sorted[i++] = v;
	for (v = G.cold; v != NULL; v = v->nextpuit)
		sorted[i++] = v;
	qsort(sorted, (size_t)n, sizeof(node *), compare_nodes);
	/* ids in the order of the lists, hot first */
	i = 0;
	for (v = G.hot; v != NULL; v = v->nextpuit)
		order[node_index(sorted, n, v)] = i++;
	for (v = G.cold; v != NULL; v = v->nextpuit)
		order[node_index(sorted, n, v)] = i++;

	fprintf(f, "graph [\n  id 0\n  directed 1\n  label ");
	{
		char label[MAXNAMELEN + 128];
		snprintf(label, sizeof(label), "PELCR final graph: %s, rank %d, %ld compositions, %ld nodes", infile, rank,
		         edge_compositions, n);
		gml_string(f, label);
	}
	fprintf(f, "\n");
	for (side = 0; side < 2; side++) {
		for (v = (side == 0) ? G.hot : G.cold; v != NULL; v = v->nextpuit) {
			char label[96], address[64];
			const char *fill, *role = "node";
			snprintf(address, sizeof(address), "%p", (void *)v);
			snprintf(label, sizeof(label), "%s", address);
			fill = (v->left.vector != NULL || v->right.vector != NULL) ? "#AAAA00" : "#FFFF0F";
			if (v == principal) {
				snprintf(label, sizeof(label), "root");
				fill = "#00AA00";
				role = "root";
			}
			for (k = 0; k < nfreevars; k++)
				if (free_var_node(k) == v) {
					snprintf(label, sizeof(label), "%s", freevar_name[k]);
					fill = "#00AAAA";
					role = "var";
				}
			fprintf(f, "  node [ id %ld label ", order[node_index(sorted, n, v)]);
			gml_string(f, label);
			fprintf(f, " graphics [ w 6.0 h 6.0 type \"oval\" fill \"%s\" outline \"#000000\" ]", fill);
			fprintf(f, " role \"%s\" address \"%s\" part \"%s\" ]\n", role, address, (side == 0) ? "hot" : "cold");
		}
	}
	for (side = 0; side < 2; side++) {
		for (v = (side == 0) ? G.hot : G.cold; v != NULL; v = v->nextpuit) {
			int s2;
			for (s2 = 0; s2 < 2; s2++) {
				for (e = (s2 == 0) ? v->left.vector : v->right.vector; e != NULL; e = e->vector) {
					char u1[MAXLENWEIGHT], u0[MAXLENWEIGHT];
					long src = node_index(sorted, n, source_node(e));
					if (src < 0) {
						/* unresolved source: a node of its own */
						src = n + nextra++;
						fprintf(f,
						        "  node [ id %ld label \"%d:key:%lx\" graphics [ w 6.0 h 6.0 type \"oval\" fill "
						        "\"#888888\" outline \"#000000\" ] role \"unresolved\" ]\n",
						        src, e->creator, (unsigned long)e->source);
					} else
						src = order[src];
					nd_split(e->weight, u1, u0);
					fprintf(f,
					        "  edge [ id %ld source %ld target %ld graphics [ fill \"%s\" targetArrow \"standard\" ]"
					        " targetSide %d sourceSide %d weight1 ",
					        nedges, src, order[node_index(sorted, n, v)], (s2 == 0) ? "#aa0000" : "#0000aa",
					        (s2 == 0) ? LEFT : RIGHT, e->side);
					gml_string(f, u1);
					fprintf(f, " weight0 ");
					gml_string(f, u0);
					fprintf(f, " label ");
					gml_string(f, e->weight);
					fprintf(f, " ]\n");
					nedges++;
				}
			}
		}
	}
	fprintf(f, "]\n");
	fclose(f);
	free(sorted);
	free(order);
	printf("(%d) gml: final graph (%ld nodes, %ld edges%s) written to %s\n", rank, n + nextra, nedges,
	       nextra ? ", with unresolved sources" : "", name);
}

void
read_back(node *principal) {
	const char *env = getenv("PELCR_READBACK_JSON");

	printf("(%d) ********** principal port address %p\n", rank, (void *)principal);
	const char *genv = getenv("PELCR_FINAL_GML");

	if (readbackfile[0] != '\0')
		write_readback_dump(principal, readbackfile);
	else if (env != NULL && env[0] != '\0')
		write_readback_dump(principal, env);
	if (finalgmlfile[0] != '\0')
		write_final_gml(principal, finalgmlfile);
	else if (genv != NULL && genv[0] != '\0')
		write_final_gml(principal, genv);
}
