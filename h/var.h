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

#ifndef var_h
#define var_h

#include <pthread.h>
#include <stdio.h>
#include <sys/times.h>
#include <time.h>
#include <mpi.h>

#include "mydefs.h"
#include "pelcr_value.h"
#include "symbolic.h"
#include "graph.h"
#include "combustion.h"
#include "io.h"
#include "dvm.h"
#include "distribution.h"
#include "lambdastar.h"
#include "buildgraph.h"

#ifdef PELCR_DEFINE_GLOBALS
#define PELCR_EXTERN
#else
#define PELCR_EXTERN extern
#endif

/* HashTable*BookTable[MAXNPROCESS]; */
/* ANTO */
/* pointer to xfunction from dynamically loaded library */
PELCR_EXTERN void *handle;
/* path of dynamically loaded library */
PELCR_EXTERN char Path[MAXNAMELEN];
PELCR_EXTERN const char *error;
/* ANTO */

PELCR_EXTERN long maxloop, maxfires;

PELCR_EXTERN long timestamp, outtimestamp;
PELCR_EXTERN long aggregation_cumulate, num_receives;

PELCR_EXTERN long global_physical_msgs;

PELCR_EXTERN int lightprocess;
PELCR_EXTERN int schedule;
PELCR_EXTERN int pflag;

PELCR_EXTERN int traceflag, inflag, outflag, verflag, dumpflag;
PELCR_EXTERN long failed_compositions, processed_actions, edge_compositions, bip4, family_reductions;
PELCR_EXTERN int local_family_reductions;
PELCR_EXTERN long fires, loops, prnsteps, one_optimizations;
PELCR_EXTERN int unaddtest, uneottest;
PELCR_EXTERN int laddtest, leottest;
PELCR_EXTERN long idle, lidle, lbip2;
PELCR_EXTERN int graph_nodes, nhot, ncold, scount;
PELCR_EXTERN long computing_loops, lastloop;

PELCR_EXTERN int cut_node_count;

/* PELCR_EXTERN int fra_hot; */
PELCR_EXTERN int incoming_actions_snapshot;

PELCR_EXTERN struct tms smtime;

PELCR_EXTERN FILE *readfile, *writefile, *firfile, *tempfile, *coldfile, *anamfile, *statsfile;
PELCR_EXTERN FILE *mawfile[MAXNPROCESS], *maxwinfile[MAXNPROCESS];
PELCR_EXTERN FILE *trivfile, *hotfile, *nofile, *logfile;
PELCR_EXTERN char infile[MAXNAMELEN], outfile[MAXNAMELEN], directoryname[MAXNAMELEN];
PELCR_EXTERN char rline[MAXLENWEIGHT];
PELCR_EXTERN node *environment;

PELCR_EXTERN int comm, rank, size, ierr;
PELCR_EXTERN int TempProcess[MAXNPROCESS], TableProcess[MAXNPROCESS], number_of_processes;
PELCR_EXTERN int OutCounter[MAXNPROCESS], InCounter[MAXNPROCESS];
PELCR_EXTERN int combusted;
PELCR_EXTERN int ending, end_computation, check_passed;

PELCR_EXTERN MPI_Status status;
PELCR_EXTERN MPI_Request data_request, add_request, create_request, eot_request, die_request, request;
PELCR_EXTERN MPI_Request send_add_request, send_eot_request[MAXNPROCESS], send_die_request;

PELCR_EXTERN int dataflag, saddflag, addflag, createflag, eotflag, dieflag, flag, h;
PELCR_EXTERN int sebuf[MAXLENWEIGHT];
PELCR_EXTERN int sbuf[MAXLENWEIGHT];
PELCR_EXTERN long sbuflong[10];
// int reotbuf[10],raddbuf[MAXLENWEIGHT+7*sizeof(long)],rcreatebuf[10],rdiebuf[10];
PELCR_EXTERN int reotbuf[10], raddbuf[MAXLENWEIGHT], rcreatebuf[10], rdiebuf[10];

/*struct mbuffer incoming;*/
PELCR_EXTERN struct mbuffer incoming[MINPRIORITY]; /* array of buffers of incoming messages */
PELCR_EXTERN int maxubound;
PELCR_EXTERN int local_pending;
PELCR_EXTERN int pending_actions;
PELCR_EXTERN long graph_edges;

/*outgoing,*outbuffer= &outgoing;*/

PELCR_EXTERN int newval;

/********** Carlo *******************************************/
PELCR_EXTERN char buf[MAXNPROCESS][sizeof(long) + (1 + MAXAWIN) * sizeof(struct messaggio)];
PELCR_EXTERN char rbuf[sizeof(int) + (1 + MAXAWIN) * sizeof(struct messaggio)];
PELCR_EXTERN char sendbuffer[150 * (sizeof(int) + (1 + MAXAWIN) * sizeof(struct messaggio))];

/************************************************************/

PELCR_EXTERN int OutTerminationStatus[MAXNPROCESS][MAXNPROCESS];
PELCR_EXTERN int InTerminationStatus[MAXNPROCESS][MAXNPROCESS];
PELCR_EXTERN int outcontrol[MAXNPROCESS];
PELCR_EXTERN int tickcontrol[MAXNPROCESS];
PELCR_EXTERN struct messaggio msg;

PELCR_EXTERN float last_rate[MAXNPROCESS];
PELCR_EXTERN float current_rate;

PELCR_EXTERN graph G;

/******************* Carlo ***********************************/

PELCR_EXTERN int nrFisicMsg[MAXNPROCESS];
PELCR_EXTERN int nrApplMsg[MAXNPROCESS];
PELCR_EXTERN float aggregationRatio[MAXNPROCESS];

PELCR_EXTERN int aggregationWindow[MAXNPROCESS];
PELCR_EXTERN int maxTick[MAXNPROCESS];

PELCR_EXTERN long nTickSend; /* Quante volte spedisco allo scadere del tick */
PELCR_EXTERN long nFullSend; /* Quante volte spedisco perche il messaggio fisico e' pieno */

PELCR_EXTERN time_t inittime, finaltime, tim;

/****************************************************/
/*            per i thread                          */

PELCR_EXTERN pthread_mutex_t mutex;

/****************************************************/

/********************************* Marco 06032001 ***********************/

/**** integro l'ampiezza della finestra di timeout **********************/
PELCR_EXTERN long cumulate_timeout[MAXNPROCESS];
PELCR_EXTERN long physical_msgs[MAXNPROCESS];
PELCR_EXTERN long applicative_msgs[MAXNPROCESS];
/***********************************************************************/

/* per lo scheduler */
PELCR_EXTERN int batch_processed_actions;

PELCR_EXTERN HashTable *BookTable[MAXNPROCESS];

PELCR_EXTERN struct messaggio cut_node_messages[MAXCUTNODES]; /*maximal number of cut-nodes in the parsed input*/

PELCR_EXTERN int lift;
PELCR_EXTERN int varname;
PELCR_EXTERN symTbl *symbolTable;

PELCR_EXTERN int kindex;
PELCR_EXTERN int findex;
PELCR_EXTERN int fcounter;

PELCR_EXTERN node *principal;

#undef PELCR_EXTERN
#endif
