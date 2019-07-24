/* Syntax interpretation function used in the parser
 
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

#ifndef bgraph_h
#define bgraph_h


#include <dlfcn.h>

typedef struct termGraph {
  char nameTerm[160000];  
  struct node *root;
  struct table *listOfVar;
} termGraph;

typedef struct table {
  char *name;
  struct node *ptrNode;
  struct table *next;
} table;

typedef struct symTbl {
  char *symName;
  struct termGraph *symTerm;
  struct symTbl *next;
} symTbl;

void InitializeIncoming(termGraph*t);

termGraph *BuildVar(char*);
termGraph *BuildLambda(char*,termGraph*);
termGraph *BuildApplication(termGraph*,termGraph*);
termGraph *BuildBool(char*);
termGraph *BuildNumber(char*);
termGraph *BuildF1Arg(char *namef, termGraph *arg);
termGraph *BuildF2Arg(char *namef, termGraph *arg1, termGraph *arg2);
termGraph *BuildXF1Arg(char *namef, termGraph *arg);
termGraph *BuildXF2Arg(char *namef, termGraph *arg1, termGraph *arg2);
termGraph *BuildITE(termGraph *arg1, termGraph *arg2, termGraph *arg3);
termGraph *BuildLet(char *arg1, termGraph *arg2, termGraph *arg3);
termGraph *BuildRec(char *x, termGraph *M);
/* ANTO */
void CallXF1ArgChar(char *, char *);
void CallXF1ArgUser(char *, char *);
int AddEntryInk(int,USERTYPE);
void SetNextKIndex(void);
/* ANTO */
int AddEntryInf(char*);
int XAddEntryInf(char*,int);
int XAddFun(int, USERTYPE);

edge *AddAnEdge(edge*,char*,node *,int);
void Lift(termGraph*t);
void LiftEdges(edge *e);
void P_Q(node *,char*);
void FreeNode(node*n);
termGraph *MergeNodes(termGraph *newTerm, termGraph *t1, termGraph *t2);
void MeltNodes(node *,node *);
termGraph *Duplicate(termGraph*);
void Weightcpy(char newW[],char oldW[]);
node *CopyNode(node*);
table *RemoveFromList(table*,char*);

void PutSymbol(char *symbol, termGraph *t);
void DeallocTermGraph(termGraph *t);
termGraph *IsThere(char*);
void ShowTable();

void MostraNodo(node *n);
void MostraArchi(edge*e);
void MostraListaDiNodi(node *n);
void MostraGrafo(termGraph *t);
void MostraTabelle();
/* ANTO */
void LoadLib(char *path);
void BCastLoad(void);
void CloseLib(void);



USERTYPE pelcr_not(USERTYPE);
USERTYPE pelcr_pred(USERTYPE);
USERTYPE pelcr_succ(USERTYPE);
USERTYPE pelcr_iszero(USERTYPE);
USERTYPE pelcr_add(USERTYPE, USERTYPE);
USERTYPE pelcr_and(USERTYPE,USERTYPE);
USERTYPE pelcr_mult(USERTYPE, USERTYPE);
USERTYPE pelcr_great(USERTYPE,USERTYPE);
/* ANTO */

#endif
