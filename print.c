/* Manager of global address space of memory for graph distributed allocation
 
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
#include <unistd.h>
#include <string.h>


#ifdef SOLARIS
#include<sys/systeminfo.h>
#endif
#include <sys/types.h>
#include <sys/times.h>
#include <mpi.h>
#ifdef MPE_GRAPH
#include<mpe.h>
#endif
#include <time.h>
#include "var.h"
#include <assert.h>

static edge*e;
struct messaggio m;

void writelabel(wstream,e)
FILE *wstream;
edge *e;
{
	char symboliclabel[MAXLENWEIGHT];
	char PrintWeight[2*MAXLENWEIGHT];
	char *CurrentWeight = NULL;
	char *NewWeight = NULL;
	char *CopyWeight = NULL;
	int Index=0, Type=0;
	
#ifdef _DEBUG
	DEBUG_PRINT {
		fprintf(logfile,"(%d) SHOW label in writelabel \n",rank);
		ShowEdge(e);
	}
#endif
	
	memset(symboliclabel, 0, MAXLENWEIGHT);
	memset(PrintWeight, 0, 2*MAXLENWEIGHT);
	
	CopyWeight = strdup(e->weight); /*allocates sufficient memory for a copy of the
									 weight, does the copy, and returns a pointer to it*/
	
	CurrentWeight = CopyWeight;
	
	NewWeight = strstr(CurrentWeight,"X(");
	
	/* ANTO
  printf("PW>%s\n",PrintWeight);
  printf("CW>%s\n",CopyWeight);
  */
	
	while(NewWeight != NULL)
	{
		strncat(PrintWeight, CurrentWeight, NewWeight-CurrentWeight);
		
		Type = atoi(&NewWeight[2]);
		NewWeight = strstr(NewWeight,",");
		Index = atoi(&NewWeight[1]);
		NewWeight = strstr(NewWeight,")");
		
		switch (Type)
		{
				/*  TYPE */
			case 1:
				if(Index)
					sprintf(symboliclabel," NATout");
				else
					sprintf(symboliclabel," NATin");
				break;
				
				/*  KONST */
			case 2:
				sprintf(symboliclabel," (%lld)",k[Index][1]);
				break;
				
				/*  FUNC */
			case 3:
				sprintf(symboliclabel," %s",f_db[f[Index].fun_id].name);
				break;
				
				/*  SYNC */
			case 4:
				sprintf(symboliclabel," SYNC(%d)",Index);
				break;
				
				/*  ARG */
			case 5:
				sprintf(symboliclabel," ARG(%d)",Index);
				break;
				
				/*  ITE */
			case 6:
				sprintf(symboliclabel," ITE(%d)",Index);
				break;
				
				/*  ARGLAST */
			case 7:
				sprintf(symboliclabel," ARG(%d)",Index);
				break;
				
			default:
				break;
		};
		strcat(PrintWeight, symboliclabel);
		CurrentWeight = (char *) &NewWeight[1];
		NewWeight = strstr(CurrentWeight,"X(");
	};
	
	strcat(PrintWeight, CurrentWeight);
	
	/* ANTO
  printf("PW Result>%s\n",PrintWeight);
  printf("CW Result>%s\n",CopyWeight);
  */
	
	fprintf(wstream,"label \"%s\"  ]\n",PrintWeight);
	free(CopyWeight);
	return;
}

void writenode(wstream,p)
FILE *wstream;
node *p;
{
#ifdef _DEBUG
	DEBUG_PRINT { fprintf(logfile,"(%d) WRITENODE (function writenode) %p\n",rank,(void*)p);}
#endif
	fprintf(wstream,"node [ id %ld label \"%p\" ",((long)p),(void*)p);
	fprintf(wstream,"graphics [ w 6.0 h 6.0  type \"oval\" ");
	if((p->left.length==0)&&(p->right.length==0))
	{
		fprintf(wstream," fill  \"#FFFF%dF\"  outline \"#000000\" ]] \n",rank);
	}
	else
		if(((p->left.length==1)&&(p->left.eot==1))&&((p->right.length==1)&&(p->right.eot==1)))
		{
			fprintf(wstream," fill  \"#F%d0000\"  outline \"#000000\" ]] \n",rank);
		}
		else
			fprintf(wstream," fill  \"#AAAA00\"  outline \"#000000\" ]] \n");
			
			//   DEBUG_PRINT { printf("(%d) in writenode pflag = %d\n",rank,pflag); }
   p->printed=pflag ;
			//   DEBUG_PRINT { printf("(%d) in writenode printed = %d\n",rank,p->printed); }
			
   return;
}

void writedge(wstream,e,p,c)
FILE *wstream;
edge *e;
node *p;
int c;
{
#ifdef _DEBUG
	DEBUG_PRINT {
		fprintf(logfile,"(%d) SHOW edge in writedge\n",rank);
		ShowEdge(e);
	}
#endif
	if (p->printed!= pflag) { printf("(%d) ERROR TARGET NODE NOT YET PRINTED \n",rank); }
	
#ifdef _DEBUG
	DEBUG_PRINT {
		fprintf(logfile,"(%d) WRITENODE (function writedge) %p\n",rank,(void*)e->source);
	}
#endif
	if (e->rankpuit==rank)
	{
		node *pp ;
		pp = BookedAddress(e->creator,(long)e->source);
		if (pp->printed != pflag) writenode(wstream,pp);
	}
	else
		writenode(wstream, e->source);
		
		
	/*
		fprintf(wstream,"node [ id %d ] \n ",((int)e->source)*size+e->rankpuit);
	 */
		fprintf(wstream,"edge [ source ");
		
		if (e->rankpuit==rank)
			fprintf(wstream,"%ld ",(long)BookedAddress(e->creator,(long)e->source));
			else
				fprintf(wstream,"%ld ",(long)e->source);
				
				fprintf(wstream,"target %ld ",(long)p);
				switch (c)
   {
    case 0 : { fprintf(wstream," graphics [ fill \"#aa0000\" ") ;} break;
    case 1 : { fprintf(wstream," graphics [ fill  \"#0000aa\" ") ;} break;
		   /*    case 2 : { fprintf(wstream," graphics [ fill  \"#00aa00\" ] \n") ;} break;*/
    default : {
		float col;
		col =255-255*(float)(c-2)/(float)MINPRIORITY;
		printf("-----> %2.2x %d \n",(int)col,(int)col);
		fprintf(wstream," graphics [ fill  \"#00%2.2x00\" ",(int)col) ;}  break ;
   }
	fprintf(wstream,"targetArrow \"standard\" ] ");
	writelabel(wstream,e);
	return;
}

/*
 This function makes a dump of an edge encapsulated in a
 message in the incoming buffer into the opened stream
 wstream, in gml format.
 */

void mmwrite(wstream, mm, priority)
FILE *wstream;
struct messaggio *mm;
int priority;
{
	e=(edge*)InitReference((edge*)e);
	/*do nothing */
	/* step 1 : ask for the local address of the virtual edge */
	/* this implies an early creation of the node (before an incoming edge is combusted again this node) */
	
	switch(mm->tpy)
	{
		case EOT_TAG:
		{
			printf("EOT \n");
		}
			break ;
			
		case ADD_TAG:
		{
			node *target;
			node *source;
			
			printf("ADD %ld\n",bip3);
			target = BookedAddress((mm->vtarget).creator, (long) (mm->vtarget).source);
			source = BookedAddress((mm->vsource).creator, (long) (mm->vsource).source);
			
			if (target->printed != pflag) {
#ifdef _DEBUG
				DEBUG_PRINT { fprintf(logfile,"(%d) WRITENODE (function mmwrite) %p\n",rank,(void*)target);}
#endif
				writenode(wstream, target);
			}
			
			e->source = source;
			sprintf(e->weight,"%s", mm->weight);
			e->rankpuit = (mm->vsource).rankpuit;
			e->sto = (mm->vsource).sto;
			
			
#ifdef _DEBUG
			DEBUG_PRINT {
				fprintf(logfile,"(%d) SHOW edge before entering writedge\n",rank);
				ShowEdge(e);
			}
#endif
			writedge(wstream, e, target,2+priority);
		}
			break;
	}
	
 /*
  if(mm->vtarget.creator!=rank)
  {
  target= (node*)BookedAddress(mm->vtarget.creator,(int)mm->vtarget.source);
  DEBUG_PRINT fprintf(logfile,"(%d) ADDR TARGET BY HASHING [%p]\n",rank,target);
  }
  else
  {
  target= mm->vtarget.source;
  DEBUG_PRINT fprintf(logfile,"(%d) ADDR TARGET BY REFERENCE [%p]\n",rank,target);
  };
  
  */
	return ;
}

int write_buffer(wstream, b, priority)
FILE *wstream;
struct mbuffer *b;
int priority;
{
	int i;
#ifdef _DEBUG
	DEBUG_PRINT {
		fprintf(logfile,"(%d) dumping message buffer in write_buffer\n",rank);
	}
#endif
	BDump(b) ;
	i=b->first;
	while(i!=b->last) {
  memcpy(&m,&(b->stack[i]),sizeof(struct messaggio));
  ShowMessage(&m);
  mmwrite(wstream,&m,priority);
		i=(i+1)%MAXPENDING;
	}
	
	i=(b->last - b->first);
	if (i<0)  i=MAXPENDING+i;
		
  return i;
}

void Print(graph g, struct mbuffer *b, long sq)
{
	node*P;
	edge*e;
	int i;
	char name[MAXNAMELEN];
	
	pflag = !pflag ;
	if (sq==1000000) pflag=-1;
#ifdef _DEBUG
	DEBUG_PRINT {
		printf("(%d) sq = %ld -- pflag = %d\n",rank,sq,pflag);
	}
#endif
	sprintf(name,"%s%5d%5ld%3ld.gml",outfile,rank,loops,sq);
	if((writefile= fopen(name,"w"))==NULL)
	{
		fprintf(logfile,"Error opening output file\n");
		outflag= 0;
		fclose(writefile);
	};
	
	if((rank==0)&&(outflag))
	{
		fprintf(writefile,"graph [ id %ld directed 1 label \"\n    Input File: %s \n    ",loops,infile);
		fprintf(writefile,"Iteration  n.%ld\n    Successfull Compositions:%ld\n\"\n",loops,bip3);
	}
	
	
	/*
		Printing all the nodes
	 */
	
	/* Scan the hot part of the graph */
	P= g.hot;
	for(i= 0;P!=NULL;i++)
	{
		//		DEBUG_PRINT { printf("(%d) WRITENODE (function Print) node print flag : %d\n",rank,P->printed); }
		if (pflag!=P->printed)
		{
#ifdef _DEBUG
			DEBUG_PRINT
			fprintf(logfile,"(%d) WRITENODE (function Print) %p\n",rank,(void*)P);
#endif
			writenode(writefile,P);
		}
		//		DEBUG_PRINT { printf("(%d) WRITENODE (function Print) node print flag (after writenode) : %d\n",rank,P->printed);}
		P= P->nextpuit;
		//		DEBUG_PRINT { if (P!=NULL) printf("(%d) WRITENODE (function Print) node print flag (after writenode) : %d\n",rank,P->printed);}
	};
	
	fflush(writefile);
	
	/* Scan the cold part of the graph */
	P= g.cold;
	for(i= 0;P!=NULL;i++)
	{
#ifdef _DEBUG
		DEBUG_PRINT {
			fprintf(logfile,"(%d) WRITENODE (function Print) %p\n",rank,(void*)P);
		};
#endif
		//		printf("-------->file pointer [%p]\n",writefile);
		writenode(writefile,P);
		
		P= P->nextpuit;
	};
	
	/*
		Printing all the edges
	 */
	
	/* Rescan the hot part of the graph */
	P = g.hot;
	for(i= 0;P!=NULL;i++)
	{
		e = P->left.vector;
		for(;e!=NULL;e= e->vector)
		{
#ifdef _DEBUG
			DEBUG_PRINT {
				fprintf(logfile,"(%d) SHOW edge in LEFT scan-hot-graph before entering writedge\n",rank);
				ShowEdge(e);
			};
#endif
			
			writedge(writefile,e,P,0);
		};
		
		e= P->right.vector;
		for(;e!=NULL;e= e->vector)
		{
#ifdef _DEBUG
			DEBUG_PRINT {
				fprintf(logfile,"(%d) SHOW edge in RIGHT scan-hot-graph before entering writedge\n",rank);
				ShowEdge(e);
			};
#endif
			writedge(writefile,e,P,1);
		};
		
		P= P->nextpuit;
	};
	
	nhot= i;
	
	fflush(writefile);
	
	/* Rescan the cold part of the graph */
	P= g.cold;
	for(i= 0;P!=NULL;i++)
	{
		e = P->left.vector;
		for(;e!=NULL;e= e->vector)
		{
#ifdef _DEBUG
			DEBUG_PRINT {
				fprintf(logfile,"(%d) SHOW edge in LEFT scan-cold-graph before entering writedge\n",rank);
				ShowEdge(e);
			}
#endif
			writedge(writefile,e,P,0);
		};
		
		e= P->right.vector;
		for(;e!=NULL;e= e->vector)
		{
#ifdef _DEBUG
			DEBUG_PRINT {
				fprintf(logfile,"(%d) SHOW edge in RIGHT scan-cold-graph before entering writedge\n",rank);
				ShowEdge(e);
			}
#endif
			writedge(writefile,e,P,1);
		};
		
		P= P->nextpuit;
	};
	
	ncold= i;
	/* before to exit we have to scan the incoming buffer to print all the edges waiting to be
		processed */
	
	
	/* at this point we put the same while-do-cycle as in the buffer dump
		but here, for each message in the buffer, we call the write message
		function */
	
#ifdef _DEBUG
	DEBUG_PRINT fprintf(logfile,"(%d) dumping message buffer in Print\n",rank);
#endif
	for(i=0;i<MINPRIORITY;i++)
	{
		//#ifdef _DEBUG
		//        DEBUG_PRINT
		//#endif
		BDump(&(b[i])) ;
		write_buffer(writefile,&(b[i]),i);
	};
	
	OUTPUTFILE if(rank==(size-1))fprintf(writefile,"]\n\n");
	
	if(outflag==1)
	{
		fclose(writefile);
	};
	
	return;
}



void DebugPrint(g,sq)
graph g;
int sq;
{
	node*P;
	edge*e;
	int i;
	OUTPUTFILE
	{
		char name[MAXNAMELEN];
		
		sprintf(name,"%s%d.%ld.%d.gml",outfile,rank,loops,sq);
		if((writefile= fopen(name,"w"))==NULL)
 	{
		fprintf(logfile,"Error opening output file\n");
		outflag= 0;
		fclose(writefile);
	};
		if(rank==0)
 	{
		fprintf(writefile,"graph [ id %ld directed 1 label \"\n    Input File: %s \n    ",loops,infile);
		fprintf(writefile,"Iteration  n.%ld\n    Successfull Compositions:%ld\n\"\n",loops,bip3);};
	};
	
	
	P= g.hot; /* scan the hot part of the graph */
	for(i= 0;P!=NULL;i++)
	{
		OUTPUT2 printf("{%d} hot node n.%d [%p] has ",rank,i,(void*)P);
		OUTPUT2 printf("%d incident left edge(s)",P->left.length);
		OUTPUT2 printf("%d incident right edge(s)",P->right.length);
		OUTPUT2 printf("and EOT state %d.\n",P->left.eot);
		OUTPUTFILE{
			writenode(writefile,P);
			e = P->left.vector;
#ifdef _DEBUG
#ifdef MEMDEBUG
			fprintf(logfile,"(%d) LEFT  EDGES([%p]): ",rank,P);
#endif
#endif
			for(;e!=NULL;e= e->vector)
			{
#ifdef _DEBUG
#ifdef MEMDEBUG
				fprintf(logfile,"%s ",e->weight);
#endif
#endif
				writedge(writefile,e,P,0);
			};
#ifdef _DEBUG
#ifdef MEMDEBUG
			fprintf(logfile,"\n");
#endif
#endif
			e= P->right.vector;
			
#ifdef _DEBUG
#ifdef MEMDEBUG
			fprintf(logfile,"(%d) RIGHT EDGES([%p]): ",rank,P);
#endif
#endif
			for(;e!=NULL;e= e->vector)
			{
#ifdef _DEBUG
#ifdef MEMDEBUG
				fprintf(logfile,"%s ",e->weight);
#endif
#endif
				writedge(writefile,e,P,1);
			};
#ifdef _DEBUG
#ifdef MEMDEBUG
			fprintf(logfile,"\n");
#endif
#endif
		};
		
 	P= P->nextpuit;
 };
	
	OUTPUT printf("{%d} Hot Nodes:      %d\n",rank,i);
	nhot= i;
	fflush(writefile);
	P= G.cold;
	for(i= 0;P!=NULL;i++)
	{
 	OUTPUT2 printf("{%d} environment node n.%d [%p] has ",rank,i,(void*)P);
 	OUTPUT2 printf("%d incident edge(s) and EOT state %d.\n",P->left.length,P->left.eot);
 	OUTPUTFILE{
		fprintf(writefile,"node [ id %ld  ",((long)P)*size+rank);
		fprintf(writefile,"graphics [ w 6.0 h 6.0  type \"oval\" ");
		if(P->left.length==0)
		{fprintf(writefile," fill  \"#FFFFFF\"  outline \"#FF0000\" ]]");}
		else
			fprintf(writefile," fill  \"#FF0000\"  outline \"#FF0000\" ]]");
		e= P->left.vector;
		for(;e!=NULL;e= e->vector)
		{
			fprintf(writefile,"edge [ source %ld ",((long)e->source)*size+e->rankpuit);
			fprintf(writefile,"target %ld ",((long)P)*size+rank);
			fprintf(writefile,"label \"%s\" graphics [ fill \"#FF0000\"]  ]\n",e->weight);
		};
		e= P->right.vector;
		for(;e!=NULL;e= e->vector)
		{
			fprintf(writefile,"edge [ source %ld ",((long)e->source)*size+e->rankpuit);
			fprintf(writefile,"target %ld \n",((long)P)*size+rank);
			fprintf(writefile,"label \"%s\" graphics [ fill \"#FF0000\"]  ]\n",e->weight);
		};
	};
#ifdef _DEBUG
		DEBUG_PRINT {
			fprintf(logfile,"(%d) COLDLIST-NEXTPRINT([%p]) -> [%p]\n",rank,(void*)P,(void*)P->nextpuit);
			fflush(logfile);
		}
#endif
		P= P->nextpuit;
	};
	
	P= g.cold;
	for(i= 0;P!=NULL;i++)P= P->nextpuit;
		OUTPUT printf("{%d} Cold nodes:     %d\n",rank,i);
		ncold= i;
		OUTPUTFILE if(rank==(size-1))fprintf(writefile,"]\n\n");
			if(outflag==1)
			{
				fclose(writefile);
			}
}
