/* Main part of the source code: interpretation of commandline options,
 starting of execution, printing of messages.
 
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


#include <stddef.h>
#include <unistd.h>
#include <math.h>
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

extern int yyparse (void);

int main(int argc, char **argv) {
  int j;
  int fine=0;

  timestamp = 0;
  outtimestamp=1;
  aggregation_cumulate = 0;
  num_receives = 0;

  global_physical_msgs=0;

  pflag=0 ;

  traceflag=0;
  inflag=0;
  outflag=0;
  verflag=0;

  bip= 0;
  bip2= 0;
  bip3= 0;
  bip4= 0;
  fires= 0;
  loops= 0;
  prnsteps= 1;
  ones= 0;
  unaddtest= 0;
  uneottest= 0;
  laddtest= 0;
  leottest= 0;
  idle= 0;
  lidle= 0;
  lbip2= 0;
  francesco= 0;
  lastloop= 0;
    
  ending= 0;
  end_computation= 0;
  check_passed= TRUE;

  maxubound =1 ;
  local_pending = 0;
  edges_counter=0;
  inittime=0;
  finaltime=0;
  contatore_combustioni_f = 0;
  lift=0;
  varname=0;
  symbolTable=NULL;

  kindex=-1;
  findex=-1;
  fcounter=-1;

  for (j=0; j<MAXNUMCOST; j++)
  {
  	k[j][0]=0;
	k[j][1]=0;
	k[j][2]=0;
  }

  timestamp=0;
  outtimestamp=1;
    
  maxloop= 1000;
  maxfires= 0;

  fires= 0;
  scount= 0;
  saddflag= 1;
  
  sprintf(directoryname, "pelcrexamples");
/* ANTO */
  memset(Path, 0, MAXNAMELEN);
  handle = NULL;

  printf ("USER TYPE Size: %ld bit",8*sizeof(k[0][1]));
/* ANTO */

  MPI_Init(&argc,&argv);

/* print the list of arguments */
 for(j= 1;j<argc;j++) printf("%s ",argv[j]);
  printf("\n");
  fflush(stdout);

  for(j= 1;j<argc;j++) {
    if(strcmp(argv[j],"-v")==0) {
      printf("Verbose Mode On\n");
      verflag= 1;
    }

    if(strcmp(argv[j],"-t")==0) {
      printf("Trace Mode On\n");
      traceflag= 1;
    }

    if(strcmp(argv[j],"-I")==0) {
      strcpy(infile,argv[j+1]);
      printf("Input File : %s\n",infile);
      inflag= 1;
    }

    if(strcmp(argv[j],"-loop")==0) {
      maxloop= atoi(argv[j+1]);
      printf("Loop: %ld\n",maxloop);
      j= j+1;
    }

    if(strcmp(argv[j],"-fires")==0) {
      maxfires= atoi(argv[j+1]);
      printf("Fires: %ld\n",maxfires);
      j= j+1;
    }

    if(strcmp(argv[j],"-prnstep")==0) {
      prnsteps= atoi(argv[j+1]);
      printf("Print every %ld loops\n",prnsteps);
      j= j+1;
    }

    if(strcmp(argv[j],"-o")==0) SetOutputFile(argv[j+1]);
  }
  MPI_Comm_size(MPI_COMM_WORLD,&size);

  /*if(size==1)outbuffer= &incoming;*/
  {
  int i;

  for(i=0;i<MINPRIORITY;i++) incoming[i].first= 0;incoming[i].last= 0;
  }
  /*outgoing.first= 0;outgoing.last= 0;*/


  InitTable();

  /* provide a buffer for buffered MPI_Send */
  MPI_Buffer_attach(sendbuffer,150 * ( sizeof(int) + (1+MAXAWIN)*sizeof(struct messaggio)));

  /*******************************************/
  if((size<MAXNPROCESS)&&(size>=1)) {
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    fine=0;

    if INITIALIZER {
      printf("PELCR (Parallel Environment for Lambda Calculus Optimal Reduction)\n");
      printf("             C. Giuffrida, M. Pedicini, F. Quaglia \n");
      printf("                  IAC - CNR (Rome, 1996 - 2002)\n");
    };

/*** INTERPRETER MAIN LOOP ***/
      while (fine!=MAXNUMCOST+1) {
          printf("(%d) EVAL\n",rank);
                OpenFileInitStruct(); fflush(stdout);
                if INITIALIZER {
                    printf("(%d) PELCR 10.0> ",rank); fflush(stdout);
                    /* CALLING PARSER */
                    fine = yyparse();
				//	printf("(%d) loops %ld\n",rank,maxloop); fflush(stdout);

                    /* BROADCAST TO MPICOMMWORLD RENDEZ-VOUS POINT AFTER PARSING*/
                    MPI_Bcast(&fine,1,MPI_INT,0,MPI_COMM_WORLD);
                    if (fine!=MAXNUMCOST+1) {
                        BDump(&incoming[schedule]);
                        /*MPI_Barrier(MPI_COMM_WORLD);*/
                        number_of_processes=size;
	  OUTPUT printf("(%d) initial network created\n",rank);
	  OUTPUT printf("(%d) n. of activated processes %d\n",rank,number_of_processes);
                        environment= NULL;
	  TRACING fprintf(logfile,"(%d) COLDS=[%p] HOTS=[%p]\n",rank,G.cold,G.hot);
                        /*DEBUG Print(G,incoming,bip3);*/
                    }
                }
                if WARMING {
                    MPI_Bcast(&fine,1,MPI_INT,0,MPI_COMM_WORLD);
                    if (fine!=MAXNUMCOST+1) {
                            kindex=fine;
                            /*MPI_Barrier(MPI_COMM_WORLD);*/
                    }
                }

      MPI_Barrier(MPI_COMM_WORLD);
      if (fine!=MAXNUMCOST+1) {
          TRACING fprintf(logfile,"2nd Barrier passed - injecting the stream of actions corresponding to the parsed term\n");
          printf("(%d) 2nd barrier - injecting the stream of actions corresponding to the parsed term\n",rank);fflush(stdout);
          
          /* set to zero the counter for beta-reductions */
          /* number of LAMBDA-APP reductions (families in the Levy's sense) */
          fam_counter = 0 ;
          buf_flush(); /* scheduler*/

          if (size!=1) loops = 0;
          tim = time(&inittime);

          /* ANTO */
          BCastLoad();
          
          /* READY TO START EVALUATION */
          MPI_Barrier(MPI_COMM_WORLD);
          TRACING fprintf(logfile,"3nd Barrier passed - starting the distributed computing engine\n");
          printf("(%d) 3rd barrier - starting the distributed computing engine\n",rank);
          fflush(stdout);
          
          /* START */
          ComputeResult();
		  TRACING fprintf(logfile,"(%d) finished - collecting results\n",rank);
		  printf("(%d) finished - collecting results\n",rank);
          /* PRINT AFTER EVALUATING */
          PrintResult();

          if INITIALIZER
              if (newval) printf("Result:  %lld\n",k[kindex][1]);
              MostraTabelle(); fflush(stdout);
                #ifdef WDEBUG
                /*{long int f; for(f=0;f<500000000;f++);}*/
                #endif
         }
    }
  }
  printf("\n(%d) QUIT\n",rank);
  CloseLib();  
  Finally();
  fflush(stdout);
  return 0;
}

