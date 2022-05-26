/* Variables and struct definitions
 
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


#ifndef mydefs_h
#define mydefs_h

#define NODEBUG if (OFF)

#ifdef _DEBUG
#define DEBUG if (TRACE)
#define DEBUG_EOT DEBUG
#define DEBUG_PRINT DEBUG
#define DEBUG_MEM DEBUG
#define DEBUG_IO DEBUG
#define DEBUG_LAMBDA DEBUG
#define DEBUG_PARSER DEBUG
#define DEBUG_WEIGHT DEBUG
#define DEBUG_AGGREGATION DEBUG
#define DEBUG_DISTRIBUTION DEBUG
#define DEBUG_XFUNC DEBUG
#define DEBUG_XDATA DEBUG
#define DEBUG_HASH  DEBUG
#else
#define DEBUG_EOT OFF
#endif

#define OUTPUT  if (PRINT)
#define OUTPUT2  if (PRINT2)
#define OUTPUTFILE if ((outflag==1))

#define TRACE   traceflag
#define PRINT   ON
#define PRINT2  verflag
#define TRACING if (TRACE)

#define ON      1
#define OFF     0

#define THREAD OFF

#define TRUE     1
#define FALSE    0

/*nodo IN: appartiene alla forma normale; nodo OUT:non appartiene alla forma normale.......o viceversa?*/
#define IN      1
#define OUT     0

#define STICKY  2
#define PLUS    1
#define MINUS   0

/*seminodo*/
#define LEFT 1
#define RIGHT 0

#define MAXLENWEIGHT 100
#define MAXNPROCESS 1000
#define MAXNUMARG 2
/* ANTO */
#define MAXNAMELEN 1000
/* ANTO */


#define WARMING (rank>0)
#define INITIALIZER (rank==0)


#define STORAGEPROCESS 0
#define MANAGERPROCESS 0
#define INITIALIZERPROCESS 0

#define ADD_REV_TAG 0
#define ADD_TAG     1
#define CREATE_TAG  2
#define EOT_TAG     3
#define DIE_TAG     4
#define COMMAND_TAG 5
#define DATA_TAG    6
#define ASK_TAG     7
#define PROCESS_TAG 8
#define START_TAG   9
/* ANTO */
#define LOADLIB_TAG 10
#define LOADFUN_TAG 11
/* ANTO */



/* passi per forzare la send */
/* #define CHECKTICKS 1000 8PE */

#define CHECKTICKS 1000

#define BURST 10


/* #define MAXTICKS 100*/

#define LENBUF 20000

/***********************************************************************/
/*  FREQUENZA di STAMPA delle STATISTICHE (misurata in numero di loop) */
/***********************************************************************/

#define FREQ 150

/*******************************************88***************************/
/* PARAMETRI di SOGLIA                                                  */
/*        (a che percentuale si deve trovare il buffer per far scattare */
/*           una modifica dei parametri di aggregazione)                */
/************************************************************************/


#define TRHLEFT 0.80
#define TRHRIGHT 0.80

/******************************************************************************
Parametri per l'aggregazione dinamica variando la dimensione del buffer
*******************************************************************************/

/* risoluzione temporale per la valutazione della politica di aggregazione */
/* ovvero ogni quanto fare il controllo sulla aggregationWindow            */
#define OBSERVATIONWINDOW 200

/* valore iniziale della dimensione della finestra di aggregazione */
#define AGGREGATIONWINDOW 45

/* MIN e, sotto, MAX della aggregationWindow                               */
#define MAXAWIN 200
#define MINAWIN 20


/*di quanto diminuisco la dim dell'aggregatioWindow*/
/*di quanto  aumento la dim dell'aggregatioWindow*/
#define ALPHA -2.0
#define BETA 2.0

/*
tenere conto della dimensione del buffer di MPI nella scelta
della dimensione massima del messaggio fisico a inviare
*/

/***************************************************************************/
/* Parametri per l'aggregazione dinamica variando il maxtick               */
/***************************************************************************/
/* il timeout per il buffer contenente i messaggi da aggregare destinati al processo  (pr)*/
/* e' mantenuto nell'array  maxTick[pr] */


/* Buffer TIMEOUT iniziale (maxtick di partenza) */
#define TICK 60
#define TICK_MOVE 1


/* RANGE per la variazione del Buffer TIMEOUT (maxtick massimo e minimo) */
#define MAXTICK 500
#define MINTICK 5


/* incrementi per il maxtick */
#define ALPHATICK 10.0
#define BETATICK  -10.0



/* code di priorita' incoming  6/10/2001 */
/*
    min priority is associated to EOT in order to guarantee that
    an edge cannot arrive to its target node after that node has been
    removed.
 */
#define MAXPRIORITY 0
#define MINPRIORITY 1


/* numbero of cut nodes in a parsed term */
#define MAXCUTNODES 100
#endif
