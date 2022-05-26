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


#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//#ifdef SOLARIS
//    #include<sys/systeminfo.h>
//#endif
#include <sys/types.h>
#include <sys/times.h>
//#include <mpi.h>
//#ifdef MPE_GRAPH
//#include<mpe.h>
//#endif
#include <time.h>
#include "var.h"

/**/

graph*NewGraph()
{
  graph*nS;
  nS= (graph*)calloc(1,sizeof(graph));
  if(!nS)
    {fprintf(logfile,"graph allocation failure - ABORT");return 0;}
  return nS;
}


node*NewNode()
{
  node*nS;

  nS= (node*)calloc(1,sizeof(node));
  if(!nS)
    {
      printf("ALLOCATION FAILURE !!!!!!!!!\n\n");
      fprintf(logfile,"node allocation failure - ABORT");
      fflush(logfile);
      return 0;}
  return nS;
}

edge*NewReference()
{
  edge*nS; 
 
  nS= (edge*)calloc(1,sizeof(edge));
 
  if(!nS)
    {fprintf(logfile,"edge allocation failure - ABORT");return 0;}
  return nS;
}

void LiberaV(P)
edge*P;
{
  struct messaggio m;

  if(P!=NULL)
    {
      /*
	#ifdef MEMDEBUG
	fprintf(logfile,"(%d) Free Memory Routine Entered\n",rank);
	fprintf(logfile,"(%d) Freeing Address [%p]\n",rank,P);
	fprintf(logfile,"(%d) Destination Node [%p]\n",rank,P->source);
	#endif
	*/
      if(P->vector!=NULL)LiberaV(P->vector);
      if(P->sign==PLUS)
	{
	  StoreMessage(&m,P,P,(char*)"*",P->sto,P->side);
	  m.tpy= EOT_TAG;
	  PushMessage(&m);
	};

      char *Xstr=NULL;
      int konst_index = 0;    
    
      Xstr=strstr(P->weight,"X(2");

      if (Xstr != NULL) 
      {
        konst_index=atoi(&Xstr[4]);
        k[konst_index][2]--;
      }
      
      free(P);
    };
#ifdef _DEBUG
    DEBUG_MEM fprintf(logfile,"(%d) Exit Free Memory Procedure\n",rank);
#endif

  return;
}

void AddEdge(S,rk,nS,sto,w,c,polarity,side)
node*S;
int rk;
node*nS;
int sto;
term*w;
int c;
int polarity;
int side;
{
  edge*p;
  seminode*v;

  p= NewReference();
  if(p==NULL)
    {
      /*
      DEBUG fprintf(logfile,"(%d) REFERENCE ALLOCATION FAILURE - ABORTING\n",rank);
      fflush(logfile);
      */
      exit(-1);
    }
  else
    {
      strcpy(p->weight,w);
      if(polarity==LEFT)
           {
          	v= &(S->left);
           }
      else v= &(S->right);

      p->vector   = v->vector;
      p->rankpuit = rk;
      p->sto      = sto;
      p->side     = side;
      p->sign     = PLUS;
      p->source   = nS;
      p->creator  = c;
      v->vector   = p;
      if(v->length!=-1) v->length= (v->length)+1;
      
#ifdef _DEBUG
        DEBUG_MEM {
            ShowEdge(p);
            fprintf(logfile,"(%d) LENNODE[%p]: %d\n",rank,(void*)S,v->length);
            fflush(logfile);
        };
#endif
	
      combusted= 0;
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

void ShowWeight(s)
char *s;
{
  switch ((int) s) 
    {
    case P: ; break;
    case Q: ; break;
    case W: ; break;
    case X: ; break;
    }
  return ;
}

#endif

void ShowEdge(e)
     edge *e;
{
 fprintf(logfile,"(%d) target-hosting-process (%d), the has in/out flag %d\n",rank,e->rankpuit,e->sto);
 fprintf(logfile,"          creator (%d)\n", e->creator);

 fprintf(logfile,"          on side %d of the source (%ld)[%p]\n",e->side,(long)e->source,e->source);

 fprintf(logfile,"          weight |%s| and",e->weight) ;
 fprintf(logfile," status one-opt. sign %d\n",e->sign) ;
 fprintf(logfile,"          next edge on the same node [%p] \n",e->vector);
 fflush(logfile);   
}

node*CreateNewNode(S)
node*S;
{
  node*nS;
#ifdef _DEBUG
    DEBUG_MEM {
    fprintf(logfile,"IN routine creazione \n");
    fflush(logfile);
  };
#endif

  nS= NewNode();
  
#ifdef _DEBUG
    DEBUG_MEM {
        fprintf(logfile,"(%d) Insert new node [%p] in S = [%p]\n",rank,nS,S);
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
  nS->families = 0;
  nS->prevpuit = NULL;
  if(S!=NULL)S->prevpuit= nS;
  temp++;
#ifdef _DEBUG
    DEBUG_MEM {
        fprintf(logfile,"(%d) NEXT([%p])=[%p]\n",rank,nS,nS->nextpuit);
        fprintf(logfile,"(%d) PREV([%p])=[%p]\n",rank,nS,nS->prevpuit);
        if(S!=NULL) {
            fprintf(logfile,"(%d) NEXT([%p])=[%p]\n",rank,S,S->nextpuit);
            fprintf(logfile,"(%d) PREV([%p])=[%p]\n",rank,S,S->prevpuit);
        };
    };
#endif
  return nS;
}


node*CreateNewBoundary(S)
node*S;
{
  node*nS;

  nS= NewNode();
  nS->left.length= -1;
  nS->left.vector= NULL;
  nS->left.dejavu= NULL;
  nS->nextpuit= S;
  S->prevpuit= nS;
  return nS;
}

node*SinkRemove(P)
node*P;
{
  node*father_of_P= P->prevpuit;

#ifdef _DEBUG
    DEBUG_MEM fprintf(logfile,"(%d) REMOVE NODE P=[%p]\n",rank,P);
#endif
  fam_counter += P->families ;
  
  if((P->left.vector!=NULL)||(P->right.vector!=NULL))
    {
#ifdef _DEBUG
        DEBUG_MEM {
            fprintf(logfile,"(%d) Now I  clean the ghost node 'cause",rank);
            fprintf(logfile," its  combustion is completely exausted\n");
        }
#endif
      LiberaV(P->left.vector);
      LiberaV(P->right.vector);
    }
  else
    {
      /*
      DEBUG fprintf(logfile,"(%d) disconnected point checked\n",rank);
      */
    };
  if(G.hot==P)
    {
      G.hot= P->nextpuit;
      if(G.hot!=NULL)G.hot->prevpuit= NULL;
#ifdef _DEBUG
        DEBUG_MEM {
            fprintf(logfile,"(%d) ok 1\n",rank);
            fprintf(logfile,"(%d) Graph HOT=[%p]\n",rank,G.hot);
        };
#endif
    }
  else
    {
#ifdef _DEBUG
        DEBUG_MEM {
            father_of_P= P->prevpuit;
            fprintf(logfile,"(%d) ok 2\n",rank);
            fprintf(logfile,"(%d) NEXT(FATHER(P=[%p])=[%p])",rank,P,father_of_P);
            if(father_of_P!=NULL)
                fprintf(logfile,"= [%p]\n",father_of_P->nextpuit);
            else
                fprintf(logfile,"= NULL FATHER\n");
            fflush(logfile);
        };
#endif
      P->prevpuit->nextpuit= P->nextpuit;
      if(P->nextpuit!=NULL)P->nextpuit->prevpuit= P->prevpuit;
    };
  free(P);
  temp--;
#ifdef _DEBUG
    DEBUG_MEM {
        fprintf(logfile,"REMOVED NODE\n");
        fprintf(logfile,"(%d) ok 3\n",rank);
        fprintf(logfile,"(%d) ok 4 [%p]\n",rank,father_of_P);
        fflush(logfile);
    }
#endif
  return father_of_P;
}

edge*EdgeRemove(e,P)
edge*e;
node*P;
{
  edge*p,*pp;

  if(P->left.vector==NULL){return 0;}
  else
    {
      pp= NULL;
      p= P->left.vector;
      for(;((p!=e)&&(p!=NULL));)
	{
	  pp= p;
	  p= p->vector;
	};
      if((p!=NULL)&&(pp!=NULL))
	{
	  pp->vector= p->vector;
	  pp= P->left.vector;}
      else
	if((p!=NULL)&&(pp==NULL))
	  {pp= p->vector;}
	else
	  {fprintf(logfile,"ERROR!\n ");return 0;};

      free(e);
      P->left.length= P->left.length-1;
      return pp;

    };
}


