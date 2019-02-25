/* Header for the processor of the workload
 
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


#ifndef combustion_h
#define combustion_h

/*#define MAXPENDING 320000*/

/*#define MAXPENDING 920000*/

#define MAXPENDING 520000

struct messaggio{
/* debugging */
  long timestamp;                    /* debugging information to check non-overlapping of messages */
  int temp;                          /* piggybacking of the workload of the hosting process */

/* symbolic and graph information */
  int tpy;                           /* the type EOT or ADD_TAG */
  int sender;                        /* the process sending this message */
  int side;                          /* the side of the edge on the target */
  edge vsource;                      /* the structure storing the "source" of the edge carried by the message*/
  edge vtarget;                      /* the structure storing the "target" of the edge carried by the message*/
  char weight[MAXLENWEIGHT];         /* weight string */

/* side effects */
  int funWhich;                      /* this message carries a functional symbol: this is symbol table entry */
  USERTYPE funWait;                 /* the function is in the stack with a certain number of arguments */
/* ANTO
  Attenzione !!! Modificato temporaneamente per abilitare il 
  passaggio di costanti long long. 
  Va definita una migliore stategia per il trasferimento delle
  costanti, magari utilizzando funArgs...
   ANTO */                                    
  int funNarg;                       /* this is the arity of the functional symbol */
  int funType;                       /* this is a pointer to the function (output) type */
  /* ANTO */
  USERTYPE funArgs[MAXNUMARG];  /* ??? Carlo che hai fatto, a che serve questo ? */
  /* ANTO */
};

struct mbuffer{
  int first;
  int last;
  struct messaggio stack[MAXPENDING+1];
};

struct termination_msg{
  int sender;
  int OutCounter[MAXNPROCESS];
  int InCounter[MAXNPROCESS];
};

void ComputeResult();
void PrintResult();

int BdumpS(struct mbuffer*);
int BDump(struct mbuffer*);

void NodeCombustion(node*n,int polarity);
void StoreMessage(struct messaggio*m,edge*t,edge*s,char*w,int sto,int p);
void ShowMessage(struct messaggio*m);

int UpperBound(struct messaggio*m);
edge*InitReference(edge*aux);

void FunReceiveMessages();
void FunInteraction();

void WriteStats();
void PushIncomingMessage(int priority, struct messaggio *m);
int EmptyBuffer(struct mbuffer*l);

void Finally();
#endif
