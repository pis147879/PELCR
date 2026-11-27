/* Frontend and backend of PELCR: the channel between them

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

/* The backend (-serve <port>: all the ranks of an MPI job) keeps only the hot graph and is always
   available: it never ends.  Its rank 0 is the gateway: it accepts a channel (TCP) from a
   frontend, injects the actions that arrive on it and writes on it the messages to the frontend.

   The frontend (-frontend <host:port>: one process) parses the terms and builds their graphs as
   the rank 0 of a run does, keeps the cold nodes, sends the other actions on the channel, receives
   the edges toward its cold nodes and makes the read-back: the whole final graph is there, for any
   number of ranks of the backend.

   Addresses.  size is the number of ranks of the backend (also in the frontend, after the
   handshake) and it is also the creator of the keys of the graph builder.  The nodes of the
   frontend have as rankpuit the rank of its channel, between size + 1 and size + CH_SLOTS, a new one
   for each channel the gateway opens (so the messages of a term whose channel closed are recognized
   and dropped), and so is the rank of the frontend process: the
   cold initial nodes, among them the cold sources of the initial edges; the hot initial nodes go
   to the ranks of the backend in round robin (ChannelInitialRank).  A
   message to a node of the frontend goes on the channel from the gateway, or to the gateway from
   the other ranks: the order of the messages of each rank is kept.  A cold node created by a
   composition in the backend lives in the frontend of its cold edge: the rank that creates it
   only books a key (SendCreateNewNode), and the frontend creates it when it is first named.

   The end of a term is an invariant of the step, kept by the frontend (no global quiescence).
   Every side of a node of the frontend whose edge PLUS (the one that owes the EOT to that side) is
   held by a node of the backend waits for one EOT.  The frontend counts these debts side by side:
   +1 for an initial edge sent to the backend, and for the other side of a cold node created by the
   backend when its other edge goes to the backend; -1 for each EOT received, and when an
   optimization of one moves the edge PLUS of a side into the frontend.  The term is over when no
   side is unbalanced.  The flags below travel in vtarget.sign of the messages to the frontend
   (the sign of a target is not used in a message). */

#ifndef channel_h
#define channel_h

#define PELCR_ALONE 0    /* parser and evaluation in one MPI job, as always */
#define PELCR_BACKEND 1  /* -serve <port> */
#define PELCR_FRONTEND 2 /* -frontend <host:port> */

#define CH_GATEWAY 0 /* the rank of the backend that holds the channel */
#define CH_SLOTS 64 /* the ranks of the channels: size + 1 ... size + CH_SLOTS */

#define CH_CREATED 1       /* the edge leaves a node created by the composition that sends it */
#define CH_OTHER_BACKEND 2 /* ... and the other edge of that node goes to the backend */

#define CHANNEL_STATS_TAG 20 /* MPI tag: the gateway asks the counters of a term to the other ranks */

extern int pelcr_role;
extern int principal_rank; /* the process that sets principal (the root of the term) */

/* the process that keeps an initial node (InitializeIncoming): this one; in a frontend, the
   frontend itself for a cold node, the ranks of the backend in round robin for a hot one */
void ChannelInitialReset(void);
int ChannelInitialRank(node *n);

/* the process where a cold node created by the composition of XI and XJ lives */
int ColdNodeRank(edge *XI, edge *XJ);

/* the flags of a message to the frontend that carries an edge of a node just created, whose other
   edge goes to the source of other */
void ChannelMarkCreated(struct messaggio *m, edge *other);

/* the process where PushMessage sends m (its dest is m->vtarget.rankpuit), or -1 if m went on the
   channel */
int ChannelRoute(struct messaggio *m, int dest);

/* the gateway: a message received from another rank for a frontend goes on the channel (1) */
int ChannelForward(struct messaggio *m);

void ServeLoop(int port);
void FrontendMain(const char *address);

#endif
