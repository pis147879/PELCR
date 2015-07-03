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

#ifndef graph_h
#define graph_h

typedef struct seminode {
  int eot; /* numero di eot ricevuto */
  int length;
  int weot;   /* numero di eot attesi */
  struct edge*vector;
  struct edge*dejavu;
} seminode;


typedef struct node {
  long pro;
  seminode left;
  seminode right;
	int mu_local; /* misura di localita' del nodo  = n. archi con sorgente locale*/
	int mu_remote; /* n. archi con sorgente remota */
	struct node*nextpuit;
  struct node*prevpuit;
  int printed; /* used in Print function to avoid duplication of the node*/
  int families ; /* the maximal number of family reductions in compositions of edges onto the node*/
} node;

typedef struct edge {
  struct edge*vector;
  node*source;
  int sto;
  int sign;
  int side;
  int rankpuit;
  int creator;
  char weight[MAXLENWEIGHT];
} edge;

typedef struct graph {
  node*hot;
  node*cold;
  
  node*border;
} graph;

graph*NewGraph(void);
node*NewNode(void);
edge*NewReference(void);
void LiberaV(edge*P);
void AddEdge(node*S,int rk,node*nS,int sto,term*w,int c,int polarity,int side);
node*CreateNewNode(node*S);
node*CreateNewBoundary(node*S);
node*SinkRemove(node*P);
edge*EdgeRemove(edge*e,node*P);
void ShowEdge(edge*);
void ShowWeight(char *s);
#endif
