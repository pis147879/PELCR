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
#include "weight.h"
#include "pelcr_value.h"
#include "symbolic.h"
#include "graph.h"
#include "combustion.h"
#include "message.h"
#include "io.h"
#include "dvm.h"
#include "distribution.h"
#include "lambdastar.h"
#include "buildgraph.h"
#include "rev.h"

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

PELCR_EXTERN long aggregation_cumulate, num_receives;

PELCR_EXTERN long global_physical_msgs;

PELCR_EXTERN int lightprocess;
PELCR_EXTERN int schedule;
PELCR_EXTERN int pflag;

PELCR_EXTERN int traceflag, inflag, outflag, verflag, dumpflag;
PELCR_EXTERN long failed_compositions, processed_actions, edge_compositions, family_reductions;
PELCR_EXTERN int local_family_reductions;
PELCR_EXTERN long fires, loops, prnsteps, one_optimizations;
PELCR_EXTERN int laddtest, leottest;
PELCR_EXTERN long idle, lidle, lbip2;
PELCR_EXTERN int graph_nodes, nhot;
PELCR_EXTERN long hot_nodes, cold_nodes;
PELCR_EXTERN long computing_loops, lastloop;

PELCR_EXTERN int cut_node_count;

/* PELCR_EXTERN int fra_hot; */
PELCR_EXTERN int incoming_actions_snapshot;

PELCR_EXTERN struct tms smtime;

PELCR_EXTERN FILE *writefile, *firfile, *tempfile, *coldfile, *anamfile, *statsfile;
PELCR_EXTERN FILE *mawfile[MAXNPROCESS], *maxwinfile[MAXNPROCESS];
PELCR_EXTERN FILE *trivfile, *hotfile, *nofile, *logfile;
PELCR_EXTERN char infile[MAXNAMELEN], outfile[MAXNAMELEN], directoryname[MAXNAMELEN];
PELCR_EXTERN char readbackfile[MAXNAMELEN]; /* -readback <file>: JSON dump of the final graph */
PELCR_EXTERN char finalgmlfile[MAXNAMELEN]; /* -gml <file>: the final graph in GML */
/* free variables of the term (boundary var nodes of the graph builder), for the read-back */
#define MAXFREEVARS 64
PELCR_EXTERN int nfreevars;
PELCR_EXTERN char freevar_name[MAXFREEVARS][64];
PELCR_EXTERN long freevar_key[MAXFREEVARS];

PELCR_EXTERN int rank, size;
PELCR_EXTERN int TempProcess[MAXNPROCESS], TableProcess[MAXNPROCESS], number_of_processes;
PELCR_EXTERN int OutCounter[MAXNPROCESS], InCounter[MAXNPROCESS];
PELCR_EXTERN int end_computation;

PELCR_EXTERN MPI_Status status;

PELCR_EXTERN int dataflag, dieflag;

/*struct mbuffer incoming;*/
PELCR_EXTERN struct mbuffer incoming[MINPRIORITY]; /* array of buffers of incoming messages */
PELCR_EXTERN int maxubound;
PELCR_EXTERN int local_pending;
PELCR_EXTERN int pending_actions;
PELCR_EXTERN long graph_edges;
PELCR_EXTERN long eot_alone;     /* EOTs sent as messages of their own (EOT_TAG) */
PELCR_EXTERN long eot_with_edge; /* EOTs carried by the last edge to a seminode (EOTADD_TAG) */

/*outgoing,*outbuffer= &outgoing;*/

PELCR_EXTERN int newval;

/********** Carlo *******************************************/
/* the aggregation buffers (h/message.h) */
PELCR_EXTERN char buf[MAXNPROCESS][MESSAGE_BUFFER];
PELCR_EXTERN char rbuf[MESSAGE_BUFFER];
PELCR_EXTERN char sendbuffer[150 * MESSAGE_BUFFER];

/************************************************************/

PELCR_EXTERN int OutTerminationStatus[MAXNPROCESS][MAXNPROCESS];
PELCR_EXTERN int InTerminationStatus[MAXNPROCESS][MAXNPROCESS];
PELCR_EXTERN int outcontrol[MAXNPROCESS]; /* messages in the aggregation buffer of each process */
PELCR_EXTERN int outbytes[MAXNPROCESS];   /* bytes of their records (h/message.h) */
PELCR_EXTERN int tickcontrol[MAXNPROCESS];
PELCR_EXTERN struct messaggio msg;

PELCR_EXTERN float last_rate[MAXNPROCESS];
PELCR_EXTERN float current_rate;

PELCR_EXTERN graph G;

/******************* Carlo ***********************************/

PELCR_EXTERN int aggregationWindow[MAXNPROCESS];
PELCR_EXTERN int maxTick[MAXNPROCESS];

PELCR_EXTERN long nTickSend; /* Quante volte spedisco allo scadere del tick */
PELCR_EXTERN long nFullSend; /* Quante volte spedisco perche il messaggio fisico e' pieno */

PELCR_EXTERN time_t inittime, finaltime;

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

PELCR_EXTERN int varname;
PELCR_EXTERN symTbl *symbolTable;

PELCR_EXTERN int kindex;
PELCR_EXTERN int findex;
PELCR_EXTERN int fcounter;

PELCR_EXTERN node *principal;

#undef PELCR_EXTERN
#endif
