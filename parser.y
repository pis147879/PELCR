/* Grammar for the PELCR language
 
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
%{
#include <stddef.h> 
#include <unistd.h>
#include <math.h>

#include <dlfcn.h>

#ifdef SOLARIS
#include <sys/systeminfo.h>
#endif
#include <sys/types.h>
#include <sys/times.h>
#ifdef MPE_GRAPH
#include <mpe.h>
#endif
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
//#include <mpi.h>

//#include "mydefs.h"
//#include "symbolic.h"
//#include "graph.h"
//#include "combustion.h"
//#include "y.tab.h"
//#include "io.h"
//#include "dvm.h"
//#include "distribution.h"
//#include "lambdastar.h"
//#include "buildgraph.h"
#include "var.h"

int yylex();
int yyerror(char *s);
%}


%union {
  char string[500];
  int number;
  int intero;
  struct termGraph *grafo;
};

%token <intero> HELP TRACEF VERBOSEF USELIB QUIT WRITE SETDIR SETLOOPS STATUS SYMBOL REC DEF IF THEN ELSE GMLGRAPH
%token <number> NL EQ LAMBDA 
%token <string> OP1 OP2 ID LOP1 LOP2 NUM BOOLEAN F1ARG F2ARG XFUNCTION LET INN PATH XCALL

%type <grafo> lterm 
%type <intero> stmt
%right EQ
%left OP1 LOP1  
%left OP2 LOP2 
%right F1ARG
%start lines

%%
lines : lines line | line ;
line: lterm NL {
		      varname=0;
		      /*MostraGrafo($2);
		       */		     

		      InitializeIncoming((termGraph *)$1);
		      DeallocTermGraph($1);
		      /*MostraTabelle();*/
		      
		      /*
			findex=-1;
			kindex=-1;
		      */
		      /*Print($2->root,0);*/
		      return kindex;		      
		    }
| stmt NL     {
                      switch($1) {
		    
                      case QUIT: return MAXNUMCOST+1; break;
                      case SYMBOL: ShowTable(); break;
                      case TRACEF: {
                          if (traceflag==1) {
                              traceflag=0;
                              printf("\n Trace Mode Off\n ");
                          }
                          else
                          if (traceflag==0) {
                              traceflag=1;
                              printf("\n Trace Mode On\n ");
                          }
                      }; break;
                      
                      case VERBOSEF: {
                          if (verflag==1) {
                              verflag=0;
                              printf("\n Verbose Mode Off\n ");
                          }
                          else
                          if (verflag==0) {
                              verflag=1;
                              printf("\n Verbose Mode On\n ");
                          }
                      }; break;
                      };
		    printf("\n PELCR 10.0> "); fflush(stdout);
        }
| error        {  yyerrok; printf("\n PELCR 10.0 > ");}
;

stmt	: QUIT { $$=QUIT; }
| SYMBOL                        { $$=SYMBOL; }
| TRACEF                        { $$=TRACEF;}
| VERBOSEF                      { $$=VERBOSEF;}
| SETLOOPS NUM					{
									maxloop = atol($2) ;
									printf("\n(%d) setting final idle loops to %ld",rank,maxloop);
									/* this is not well implemented since parsing is only on rank 0 process 
									 thus PEs with different rank do not receive this value and run with 
									 the default setting of maxloop */
								}
| WRITE PATH                    { SetOutputFile($2) ; }
| USELIB PATH                   { LoadLib($2);}
| XCALL '(' ID ')' '(' PATH ')' { CallXF1ArgChar($3,$6); }
| XCALL '(' ID ')' '(' NUM ')'  { CallXF1ArgUser($3,$6); }
| DEF ID EQ lterm               { PutSymbol($2,$4); }
| SETDIR PATH                   { SetDirectory($2) ; } ;
| STATUS                        { WriteStatus() ; };
| HELP							{ WriteHelp() ; };
| GMLGRAPH						{ return kindex ;}

lterm	: LAMBDA ID '.' lterm
            {
		      $$=BuildLambda($2,$4);
		      /*  printf("\n\t'astrazione su %s'",$2);*/ 
		    } 
| '(' lterm ')' lterm
		    { 
		      /*lift=0;*/
		      $$=BuildApplication($2,$4);
		      /* printf("\n\t 'applicazione'"); */
		    } 
| ID 		
		    { 
		      termGraph *t;
		      t=IsThere($1);
		      if (t) $$=t;
		      else  $$=BuildVar($1);
		      
		      /* printf("\n\t 'ID' %s ",$1); */
		    } 
| NUM		
                    {   $$=BuildNumber($1); } 		
| BOOLEAN	
                    {   $$=BuildBool($1);}
| F1ARG '(' lterm ')'
                    {  $$=BuildF1Arg($1,$3);}
| F2ARG '(' lterm ',' lterm ')'   
                    {  $$=BuildF2Arg($1,$3,$5);}
| XFUNCTION '(' ID ')' '(' lterm ')'
                    {  $$=BuildXF1Arg($3,$6); }
| XFUNCTION '(' ID ')' '(' lterm ',' lterm ')'
                    {  $$=BuildXF2Arg($3,$6,$8); }
| IF lterm THEN lterm ELSE lterm
                    { $$=BuildITE($2,$4,$6);}
| LET ID EQ lterm INN lterm
                    { $$=BuildLet($2,$4,$6);} 
| REC ID INN lterm 
                    { $$=BuildRec($2,$4);}


%%
/* An optional but friendlier yyerror function... */
 int yyerror(char *s)
 {
	extern int yylineno;  // defined and maintained in lex
	extern char *yytext;  // defined and maintained in lex
	fprintf(stderr, "ERROR: %s at symbol '%s' on line %d\n", s,
	yytext, yylineno);
	return -1;
 }

