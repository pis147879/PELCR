/* Constants and
 
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



#include <sys/times.h>
#include <time.h>
#include <pthread.h>
#include <mpi.h>
#include "mydefs.h"
#include "symbolic.h"
#include "graph.h"
#include "combustion.h"
//#include "y.tab.h"
#include "io.h"
#include "dvm.h"
#include "distribution.h"
#include "lambdastar.h"
#include "buildgraph.h"

/* HashTable*BookTable[MAXNPROCESS]; */
#ifndef var_h
#define var_h
/* ANTO */
/* pointer to xfunction from dynamically loaded library */
extern void *handle;
/* path of dynamically loaded library */
extern char Path[MAXNAMELEN];
extern const char *error;
/* ANTO */

extern long maxloop,maxfires,maxbips;

extern long timestamp,outtimestamp;
extern long aggregation_cumulate,  num_receives;

extern long global_physical_msgs;

extern int lightprocess ;
extern int schedule ;
extern int pflag ;

extern int traceflag, inflag, outflag,verflag;
extern long maxloo,maxfir,bip,bip2,bip3,bip4,fam_counter;
extern int locf_counter;
extern long fires,loops,prnsteps,ones;
extern int unaddtest,uneottest;
extern int laddtest,leottest;
extern long idle,lidle,lbip2;
extern int temp;
extern int nhot,ncold;
extern int scount;
extern long francesco,lastloop;

extern int npozzi ;

extern int fra_hot;

extern struct tms smtime;

extern FILE *firfile,*tempfile,*coldfile,*anamfile;
extern FILE*mawfile[MAXNPROCESS],*maxwinfile[MAXNPROCESS];
extern FILE*trivfile,*hotfile,*nofile,*logfile;
extern char infile[MAXNAMELEN];
extern char directoryname[MAXNAMELEN];
extern char outfile[MAXNAMELEN];
//char rline[MAXLENWEIGHT];
extern node*environment;

extern int rank,size;
extern int TableProcess[MAXNPROCESS];
extern int number_of_processes;
extern int OutCounter[MAXNPROCESS],InCounter[MAXNPROCESS];
extern int ending,end_computation,check_passed;

extern MPI_Status status;
//MPI_Request send_add_request,send_eot_request[MAXNPROCESS];//,send_die_request;

extern int dataflag,saddflag,addflag,createflag,eotflag,dieflag,flag,h;
//int sebuf[MAXLENWEIGHT];
//int reotbuf[10],raddbuf[MAXLENWEIGHT+7*sizeof(long)],rcreatebuf[10],rdiebuf[10];

/*struct mbuffer incoming;*/
extern struct mbuffer incoming[MINPRIORITY]; /* array of buffers of incoming messages */
extern int maxubound;
extern int local_pending;
extern int edges_counter;

/*outgoing,*outbuffer= &outgoing;*/

extern int newval;

/********** Carlo *******************************************/
extern char buf[MAXNPROCESS][sizeof(long) + (1+MAXAWIN)*sizeof(struct messaggio)];
extern char rbuf[sizeof(int) + (1+MAXAWIN)*sizeof(struct messaggio)];

/************************************************************/

extern int OutTerminationStatus[MAXNPROCESS][MAXNPROCESS];
extern int InTerminationStatus[MAXNPROCESS][MAXNPROCESS];
extern int outcontrol[MAXNPROCESS];
extern int tickcontrol[MAXNPROCESS];
//struct messaggio msg;

extern float last_rate[MAXNPROCESS];
//float current_rate;

extern graph G;

/******************* Carlo ***********************************/

extern int nrFisicMsg[MAXNPROCESS];
extern int nrApplMsg[MAXNPROCESS];
extern float aggregationRatio[MAXNPROCESS];

extern int aggregationWindow[MAXNPROCESS];
extern int maxTick[MAXNPROCESS];

//time_t oss1, oss2;/* ,t;*/
extern long nTickSend; /* Quante volte spedisco allo scadere del tick */
extern long nFullSend; /* Quante volte spedisco perche il messaggio fisico e' pieno */


extern time_t inittime,finaltime,tim;

/****************************************************/
/*            per i thread                          */

extern pthread_mutex_t mutex;

/****************************************************/


/********************************* Marco 06032001 ***********************/

/**** integro l'ampiezza della finestra di timeout **********************/
extern long cumulate_timeout[MAXNPROCESS];
extern long physical_msgs[MAXNPROCESS];
extern long applicative_msgs[MAXNPROCESS];
/***********************************************************************/

/* per lo scheduler */
extern int contatore_combustioni_f;

extern HashTable*BookTable[MAXNPROCESS];


extern struct messaggio pozzi[MAXCUTNODES]; /*maximal number of cut-nodes in the parsed input*/

extern int lift;
extern int varname;
extern symTbl *symbolTable;

extern int kindex;
extern int findex;
extern int fcounter;

extern node*principal ;

#endif
