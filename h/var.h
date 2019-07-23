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
void *handle;
/* path of dynamically loaded library */
char Path[MAXNAMELEN];
const char *error;
/* ANTO */

long maxloop,maxfires,maxbips;

long timestamp,outtimestamp;
long aggregation_cumulate,  num_receives;

long global_physical_msgs;

int lightprocess ;
int schedule ;
int pflag ;

int traceflag, inflag, outflag,verflag;
long maxloo,maxfir,bip,bip2,bip3,bip4,fam_counter;
int locf_counter;
long fires,loops,prnsteps,ones;
int unaddtest,uneottest;
int laddtest,leottest;
long idle,lidle,lbip2;
int temp,nhot,ncold,scount;
long francesco,lastloop;

int npozzi=0 ;

int fra_hot;

struct tms smtime;

FILE*readfile,*writefile,*firfile,*tempfile,*coldfile,*anamfile;
FILE*mawfile[MAXNPROCESS],*maxwinfile[MAXNPROCESS];
FILE*trivfile,*hotfile,*nofile,*logfile;
char infile[MAXNAMELEN],outfile[MAXNAMELEN],directoryname[MAXNAMELEN];
char rline[MAXLENWEIGHT];
node*environment;

int comm,rank,size,ierr;
int TempProcess[MAXNPROCESS],TableProcess[MAXNPROCESS],number_of_processes;
int OutCounter[MAXNPROCESS],InCounter[MAXNPROCESS];
int combusted;
int ending,end_computation,check_passed;

MPI_Status status;
MPI_Request data_request,add_request,create_request,eot_request,die_request,request;
MPI_Request send_add_request,send_eot_request[MAXNPROCESS],send_die_request;

int dataflag,saddflag,addflag,createflag,eotflag,dieflag,flag,h;
int sebuf[MAXLENWEIGHT];
int sbuf[MAXLENWEIGHT];
long sbuflong[10];
//int reotbuf[10],raddbuf[MAXLENWEIGHT+7*sizeof(long)],rcreatebuf[10],rdiebuf[10];
int reotbuf[10],raddbuf[MAXLENWEIGHT],rcreatebuf[10],rdiebuf[10];

/*struct mbuffer incoming;*/
struct mbuffer incoming[MINPRIORITY]; /* array of buffers of incoming messages */
int maxubound;
int local_pending;
int edges_counter;

/*outgoing,*outbuffer= &outgoing;*/

int newval;

/********** Carlo *******************************************/
char buf[MAXNPROCESS][sizeof(long) + (1+MAXAWIN)*sizeof(struct messaggio)];
char rbuf[sizeof(int) + (1+MAXAWIN)*sizeof(struct messaggio)];
char sendbuffer[150*(sizeof(int) + (1+MAXAWIN)*sizeof(struct messaggio))];

/************************************************************/

int OutTerminationStatus[MAXNPROCESS][MAXNPROCESS];
int InTerminationStatus[MAXNPROCESS][MAXNPROCESS];
int outcontrol[MAXNPROCESS];
int tickcontrol[MAXNPROCESS];
struct messaggio msg;

float last_rate[MAXNPROCESS];
float current_rate;

graph G;

/******************* Carlo ***********************************/

int nrFisicMsg[MAXNPROCESS];
int nrApplMsg[MAXNPROCESS];
float aggregationRatio[MAXNPROCESS];

int aggregationWindow[MAXNPROCESS];
int maxTick[MAXNPROCESS];

time_t oss1, oss2;/* ,t;*/
long nTickSend; /* Quante volte spedisco allo scadere del tick */
long nFullSend; /* Quante volte spedisco perche il messaggio fisico e' pieno */


time_t inittime,finaltime,tim;

/****************************************************/
/*            per i thread                          */

pthread_mutex_t mutex;

/****************************************************/


/********************************* Marco 06032001 ***********************/

/**** integro l'ampiezza della finestra di timeout **********************/
long cumulate_timeout[MAXNPROCESS];
long physical_msgs[MAXNPROCESS];
long applicative_msgs[MAXNPROCESS];
/***********************************************************************/

/* per lo scheduler */
int contatore_combustioni_f;

HashTable*BookTable[MAXNPROCESS];


struct messaggio pozzi[MAXCUTNODES]; /*maximal number of cut-nodes in the parsed input*/

int lift;
int varname;
symTbl *symbolTable;

int kindex;
int findex;
int fcounter;

node*principal ;

#endif
