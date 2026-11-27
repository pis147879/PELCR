/* Manager for distribution of workload

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

#include <math.h>
#include <stddef.h>
#include <unistd.h>
#ifdef SOLARIS
#include <sys/systeminfo.h>
#endif
#include <sys/times.h>
#include <sys/types.h>
#ifdef MPE_GRAPH
#include <mpe.h>
#endif
#include "var.h"
#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int KnownProcessLoad[MAXNPROCESS];

void
ResetLightProcessPolicy(void) {
	int h;

	for (h = 0; h < MAXNPROCESS; h++) {
		KnownProcessLoad[h] = 0;
		TempProcess[h] = 0;
	}
}

static int
CurrentProcessLoad() {
	return incoming_actions_snapshot;
}

static void
RememberProcessLoad(int process, int load) {
	if ((process < 0) || (process >= size))
		return;

	TempProcess[process] = load;
	KnownProcessLoad[process] = 1;
}

static int
DrainModeIsGeneralized(int local_load) {
	int h;
	int known = 0;
	int low = 0;

	for (h = 0; h < size; h++) {
		int load;
		int known_process = KnownProcessLoad[h];

		if (h == rank) {
			known_process = 1;
			load = local_load;
		} else {
			load = TempProcess[h];
		}

		if (!known_process)
			continue;

		known++;
		if (load < DRAIN_LOAD_THRESHOLD)
			low++;
	}

	return (known * 2 >= size) && (low * 2 >= size)
	    && (low * DRAIN_LOW_FRACTION_DEN >= known * DRAIN_LOW_FRACTION_NUM);
}

static int
RankZeroCanDrain(int local_load) {
	if (rank == 0)
		return local_load < DRAIN_LOAD_THRESHOLD;

	return KnownProcessLoad[0] && (TempProcess[0] < DRAIN_LOAD_THRESHOLD);
}

int
NouvelleReservation(int dest) {
	TableProcess[dest] = TableProcess[dest] + 1;
	return TableProcess[dest];
}

void
SendCreateNewNode(int dest, int sto, edge *nS) {
	if ((dest == rank) && (sto == IN)) {
		nS->source = CreateNewNode(G.hot);
		G.hot = nS->source;
		hot_nodes++;
		nS->source->sto = IN;
		nS->rankpuit = rank;
		nS->creator = rank;
		nS->sto = IN;
#ifdef _DEBUG
		DEBUG_DISTRIBUTION fprintf(logfile, "IN) AutoREF: [%p] created\n", nS->source);
#endif
	} else if ((dest == rank) && (sto == OUT)) {
		nS->source = CreateNewNode(G.cold);
		G.cold = nS->source;
		cold_nodes++;
		nS->source->sto = OUT;
		nS->rankpuit = rank;
		nS->creator = rank;
		nS->sto = OUT;
#ifdef _DEBUG
		DEBUG_DISTRIBUTION fprintf(logfile, "OUT) AutoREF: [%p] created\n", nS->source);
#endif
	} else {
		nS->rankpuit = dest;
		nS->creator = rank;
		nS->sto = sto;
		nS->source = (node *)(long)NouvelleReservation(dest);
#ifdef _DEBUG
		DEBUG_DISTRIBUTION {
			if (sto == OUT)
				fprintf(logfile, "OUT)");
			else
				fprintf(logfile, "IN)");
			fprintf(logfile, " BOOK((%d),(%d", dest, rank);
			fprintf(logfile, " %ld[%p]))\n", (long)nS->source, nS->source);
		}
#endif
	}
	return;
}

/*
int
SendLightProcess() {
	return LightProcess(rank);
}

int
LightProcess(int dest) {
	static int min = 1;

	min++;
	if (TempProcess[min % size] >= graph_nodes) {
		return rank;
	} else {
		return min % size;
	}
}

int
LightProcess2(int dest) {
	static int min = 1;

	min++;

	if (TempProcess[min % size] >= graph_nodes) {
		return rank;
	} else {
		return min % size;
	};
}

int
LightProcess3(int dest) {
	static int min = 1;

	min++;

	if (TempProcess[min % size] >= nhot) {
		return rank;
	} else {
		return min % size;
	};
}
*/

/*
  int LightProcess4(dest)
  int dest;
  {
  if(TempProcess[lightprocess]>=nhot)
  {return rank;}
  else
  {
  return lightprocess;
  };
  }
*/

/*
int
LightProcess5(int dest) {
	static int index = 0;
	int lightprocess;

	lightprocess = index;
	if ((++index) >= size)
		index = 0;

	return lightprocess;
}

int
LightProcess6(int dest) {
	static int min = 1;

	min++;

	return min % size;
}
*/

int
LightProcess7(int dest) {
	static int min = 1;
	int candidate;
	int local_load;
	int drain_mode;

	min++;
	candidate = min % size;
	local_load = CurrentProcessLoad();
	RememberProcessLoad(rank, local_load);
	drain_mode = DrainModeIsGeneralized(local_load);

	if (drain_mode && RankZeroCanDrain(local_load)) {
		if (rank == 0) {
			if (local_load < DRAIN_LOAD_THRESHOLD)
				return rank;
		} else if (local_load < DRAIN_LOAD_THRESHOLD) {
			return 0;
		}
	}

	/*  if(((float)TempProcess[min%size])>=((float)1.1*nhot)) */

	/* if (TempProcess[min % size] >= (fra_hot - 20)) { */
	if (TempProcess[candidate] >= (local_load - DRAIN_LOAD_MARGIN)) {
		return rank;
	} else {
		return candidate;
	};
}

/*
int
LightProcess8(int dest) {
	static int min = 1;
	static int flag = 0;
	int selected;

	if (flag < BURST) {
		flag++;
		return min % size;
	} else {
		flag = 0;
		selected = min;
		min++;
		return selected % size;
	};
}
*/

void
buf_flush() {
	/* qui viene replicata l'analisi sulla priorita' del messaggio*/
	int i;
#if MINPRIORITY > 1
	int priority, ub;
#endif
	struct messaggio *m;

	for (i = 0; i < local_pending; i++) {
		m = (struct messaggio *)(buf[rank] + (i * sizeof(struct messaggio)));
#if MINPRIORITY > 1
		ub = UpperBound(m);
		priority = floor((MINPRIORITY - 1) * (1 - (float)ub / (float)maxubound));

		//      DEBUG      printf("(%d) upperbound function %d ; max =%d ",rank ,ub,maxubound);
		//      DEBUG      printf("(%d):: priority %d\n", rank, priority);

		PushIncomingMessage(priority, m);
#else
		PushIncomingMessage(0, m);
#endif

		/*
	  printf("STACK DUMP(%d-%d)\n",l->first,l->last);
	  fflush(stdout);
		*/

#ifdef _DEBUG
		DEBUG_DISTRIBUTION {
			//    printf("\nSTACK DUMP(%d-%d)\n",l->first,l->last);
			fflush(stdout);
		};
#endif
	}

	local_pending = 0;
}

/* the first message of a queue, taken out without being processed: a run stopped by -fires on
   several processes drains its channels before the end (StopComputation in combustion.c), and
   its queued messages stay unprocessed as in a run on one process */
void
DropMessage(struct mbuffer *l) {
	if (l->first == l->last)
		return;
	InCounter[(l->stack[l->first]).sender]++;
	MessageFreeWeight(&l->stack[l->first]);
	l->first = (l->first + 1) % l->capacity;
	pending_actions--;
}

void
PopMessage(struct messaggio *m, struct mbuffer *l) {
	int i, k;

	if (l->first == l->last) {
		printf("Pop from empty buffer\n Abort\n");
		exit(-1);
	}
	i = l->first;
	memcpy((char *)m, (char *)(&(l->stack[i])), sizeof(struct messaggio));
	/*
	  if(m->timestamp!= (timestamp+1)) {
	  printf("sequence error\n");
	  exit(-1);
	  }
	*/
	InCounter[m->sender]++;
	if (m->sender != rank) {
		RememberProcessLoad(m->sender, m->sender_load);
		if (m->sender_load < TempProcess[lightprocess])
			lightprocess = m->sender;
	}
	l->first = (l->first + 1) % l->capacity;

	/* the external letters: the string form wrote the new index with sprintf(&X[4], "%d)", ..),
	   which also cut the weight after the letter (WeightSetXAndTruncate) */
	if ((k = WeightFindX(MessageWeight(m), FUNC)) >= 0) {
		int indice = (int)XL_VALUE(WLETTERS(MessageWeight(m))[k]), i;

		if (m->funWhich == EXT) {
			WeightSetXAndTruncate(MessageWeight(m), k, XAddFun(indice, m->funWait));

			for (i = 0; i < m->funWait - 1; i++)
				pelcr_value_from_wire(&f[fcounter].s[i], m->funArgs[i]);
		} else {
			if (indice > findex)
				findex = indice;
			f[indice].wait = m->funWait;
			f[indice].narg = m->funNarg;
			f[indice].type = m->funType;
			f[indice].which = m->funWhich;
			for (i = 0; i < m->funWait - 1; i++)
				pelcr_value_from_wire(&f[indice].s[i], m->funArgs[i]);
			switch (m->funWhich) {
				case SUCC:
					f[indice].fun = pelcr_succ;
					break;
				case ADD:
					f[indice].fun = pelcr_add;
					break;
				case PRED:
					f[indice].fun = pelcr_pred;
					break;
				case MULT:
					f[indice].fun = pelcr_mult;
					break;
				case AND:
					f[indice].fun = pelcr_and;
					break;
				case NOT:
					f[indice].fun = pelcr_not;
					break;
				case ISZERO:
					f[indice].fun = pelcr_iszero;
					break;
				case GREATER:
					f[indice].fun = pelcr_great;
					break;
			}
		}
	} else if ((k = WeightFindX(MessageWeight(m), KONST)) >= 0) {
		SetNextKIndex();
		WeightSetXAndTruncate(MessageWeight(m), k, kindex);
		k_type[kindex] = m->funType;
		pelcr_value_from_wire(&k_value[kindex], m->funValue);
		k_refs[kindex]++;
	}

	pending_actions--;
	return;
}

/* the message goes to its process: to the queue of this process, or as a record (h/message.h)
   in the aggregation buffer of another one.  A message whose weight is allocated is pushed once:
   the copy in the queue, or the record, takes its weight */
void
PushMessage(struct messaggio *m) {
	char *lout;
	// int h,packet_action_count;
	int packet_action_count; /* the receiver reads an int (FunReceiveMessages) */
	int dest;
	int nc;

	dest = m->vtarget.rankpuit;

	m->sender = rank;
	OutCounter[dest]++;

	/*
	    if (dest!=rank)
	    {
	    printf("\n dest=%d <------ rank=%d",dest,rank);
	    read(0,&c,1);
	    }
	 */

	if (dest == rank) {
		int offset;

		offset = local_pending * sizeof(struct messaggio);
		lout = buf[rank];
		lout = lout + offset;

		memcpy((char *)lout, (char *)m, sizeof(struct messaggio));
		local_pending++;
		/*******************************
		              se local_pending supera la taglia di buf[rank] allora bisogna svuotare il buffer
		              buf_flush()
		**********************/

		if (local_pending == MAXAWIN) {
#ifdef _DEBUG
			DEBUG_DISTRIBUTION {
				printf("(%d) local pending= %d\n", rank, local_pending);
				fflush(stdout);
			};
#endif
			buf_flush();
		}
#ifdef WITHTHREAD
		if (THREAD)
			pthread_mutex_unlock(&mutex);
#endif
		return;
	} else {
		/* the record of the message, as long as its weight (h/message.h) */
		lout = buf[dest] + sizeof(int) + outbytes[dest];
		outbytes[dest] += MessagePack(lout, m);
		outcontrol[dest]++;

		/*
		  packet_action_count = outcontrol[dest];

		  memcpy((char*)buf[dest],(char*)&packet_action_count,sizeof(int));
		*/

		/* the window closes after aggregationWindow messages, or when the buffer has no room
		   for one more record with the longest weight */
		if ((outcontrol[dest] >= aggregationWindow[dest])
		    || (sizeof(int) + (size_t)outbytes[dest] + MESSAGE_MAXRECORD > MESSAGE_BUFFER)) {

			/*
		  int nc;
			*/

			packet_action_count = outcontrol[dest];
			memcpy((char *)buf[dest], (char *)&packet_action_count, sizeof(int));

			nc = sizeof(int) + outbytes[dest];
			/***************** Carlo ***************************************/
			nFullSend++;

			/*MARCO*/
			cumulate_timeout[dest] += maxTick[dest];
			applicative_msgs[dest] += outcontrol[dest];
			physical_msgs[dest]++;
			global_physical_msgs++;

			/****** SendNum 1 : SEND su FULLBUFFER **************************/

#ifdef _DEBUG
			DEBUG_AGGREGATION {
				printf("(%d) FULL BUFFER SEND - destination = %d - ncc = %d\n", rank, dest, nc);
				fflush(stdout);
			};
#endif

			MPI_Bsend(buf[dest], nc, MPI_CHAR, dest, DATA_TAG, MPI_COMM_WORLD);

			/*
		  FunReceiveMessages() ;
		  MPI_Wait(&ireq,&stsq);
			*/

			/*
		  {
		  float now;
		  times(&smtime);

		  now = (smtime.tms_utime+smtime.tms_stime)/60.0;

		  OUTPUT {
		  printf("___<______________________><________________");
		  fflush(stdout);
		  fprintf(maxwinfile[dest],"%f %d\n",now,aggregationWindow[dest]);
		  fprintf(mawfile[dest],"%f %f\n",now,(float)((float)nrApplMsg[dest]/(float)nrFisicMsg[dest]));
		  fflush(maxwinfile[dest]);
		  fflush(mawfile[dest]);
		  }

		  }
			*/
			outcontrol[dest] = 0;
			outbytes[dest] = 0;
			tickcontrol[dest] = 0;
		}
	}

	/*
   {  if((0)){
	for(h = 0; h < size; h++) tickcontrol[h]++;
	for(h = 0; h < size; h++)
	  if( (outcontrol[h]>0) && (tickcontrol[h]>=maxTick[h]) ) {
	int ncc;

	ncc = sizeof(int) + outcontrol[h]*sizeof(struct messaggio);


	//  printf("(%d) TIMEOUT SEND - destination = %d  - ncc = %d\n",rank,h,ncc);
	//  fflush(stdout);


	packet_action_count = outcontrol[h];

	memcpy((char*)buf[h],(char*)&packet_action_count,sizeof(int));
	nTickSend++;
	nrApplMsg[h]+=outcontrol[h];
	nrFisicMsg[h]++;
	cumulate_timeout[h] +=  maxTick[h] ;
	applicative_msgs[dest] += outcontrol[dest];
	physical_msgs[dest]++ ;
	global_physical_msgs++ ;

	DEBUG{
	  printf("(%d) TIMEOUT SEND - destination = %d - ncc = %d\n",rank,h,ncc);
	  fflush(stdout);
	};

	MPI_Bsend(buf[h],ncc,MPI_CHAR,h,DATA_TAG,MPI_COMM_WORLD);

	outcontrol[h]= 0;
	tickcontrol[h] = 0;
	  }
	}}
*/
}

#ifdef WITHTHREAD
void *
ThreadReceiveMsgs() {

	int received_action_count, i;
	if (size != 1) {
		dataflag = 1;
		MPI_Iprobe(MPI_ANY_SOURCE, DATA_TAG, MPI_COMM_WORLD, &dataflag, &status);
		while (dataflag) {
			int maxdim = MESSAGE_BUFFER;
			char *position;

			MPI_Recv(rbuf, maxdim, MPI_CHAR, MPI_ANY_SOURCE, DATA_TAG, MPI_COMM_WORLD, &status);
			position = rbuf + sizeof(int);

			memcpy((char *)&received_action_count, (char *)rbuf, sizeof(int));
			aggregation_cumulate += received_action_count;
			num_receives++;

			if ((tempfile != NULL) && (!(num_receives % 40))) {
				float now;
				times(&smtime);

				now = (smtime.tms_utime + smtime.tms_stime) / 60.0;
				OUTPUT {
					fprintf(coldfile, "%f %d\n", now, received_action_count);
					fprintf(anamfile, "%f %ld\n", now, aggregation_cumulate / num_receives);
					fflush(coldfile);
					fflush(anamfile);
				}
			}

			/***************************************************
			    questo pezzo di codice deve realizzare lo scheduling,
			       l'aggregato e puntato da "position", va ora disaggregato
			       inserendo nel buffer incoming ogni messaggio, in relazione alla
			       euristica.
			********************************/
			/*printf("(%d) RCVS %d, TOTAGG %d, AGGMSG %d\n",
		  rank,num_receives,aggregation_cumulate,received_action_count);
		  printf("arrivato messaggio (taglia %d)\n",received_action_count);
			*/
			/*  for(i= 0;i<AGGREGATIONWINDOW;i++)  */

			for (i = 0; i < received_action_count; i++) {
				struct messaggio rm;
#if MINPRIORITY > 1
				int priority;
				int ub;
#endif

				pthread_mutex_lock(&mutex);

				position = position + MessageUnpack(&rm, position);
#if MINPRIORITY > 1
				/*SCHEDULER*/
				ub = UpperBound(&rm);
				priority = floor((MINPRIORITY - 1) * (1 - (float)ub / (float)maxubound));
				/*
				  printf("ub = %d maxub = %d - priority = %d\n",ub,maxubound,priority);
				*/
				/* FINE SCHEDULER */

				PushIncomingMessage(priority, &rm);
#else
				PushIncomingMessage(0, &rm);
#endif
				pthread_mutex_unlock(&mutex);
			}
			MPI_Iprobe(MPI_ANY_SOURCE, DATA_TAG, MPI_COMM_WORLD, &dataflag, &status);
		}
	}

	/* scheduler*/
	buf_flush();
	batch_processed_actions = 0;
	schedule = 0;
	/* fine scheduler */

	pthread_exit(0);
}

void *
ThreadInteraction() {
	int batch_processed_actions = 0;
	int nc1, packet_action_count;
	int h;
	MPI_Request ireq;
	MPI_Status stsq;

	/*
	  while( (!EmptyBuffer(&incoming)) && (batch_processed_actions < CHECKTICKS) ) {
	  lidle = idle;
	  idle += loops-1;
	  loops = 0;
	  nhot=BDump(&incoming[schedule]);
	  processed_actions++;
	  batch_processed_actions++;
	  pthread_mutex_lock(&mutex);
	  PopMessage(&msg,&incoming);
	  pthread_mutex_unlock(&mutex);
	*/

	while ((pending_actions > 0) && (batch_processed_actions < CHECKTICKS)) {
		lidle = idle;
		idle += loops - 1;
		loops = 0;
#ifdef _DEBUG
		DEBUG_DISTRIBUTION fprintf(logfile, "(%d) ...seeking a non-empty incoming buffer\n", rank);
#endif

#if MINPRIORITY > 1
		while (((schedule < MINPRIORITY) && (!(nhot = BDumpS(&incoming[schedule]))))) {
#ifdef _DEBUG
			DEBUG_DISTRIBUTION fprintf(logfile, "(%d) BUFFER %d ", rank, schedule);
#endif
			schedule++;
		};
#else
		schedule = 0;
		nhot = BDumpS(&incoming[0]);
		if (!nhot)
			schedule = MINPRIORITY;
#endif
#ifdef _DEBUG
		DEBUG_DISTRIBUTION fprintf(logfile, "(%d) %d is the first non empty \n", rank, schedule);
#endif
		if (schedule < MINPRIORITY) {
			processed_actions++;
			batch_processed_actions++;
			DEBUG fprintf(logfile, "(%d) POP(%d) \n", rank, schedule);

			pthread_mutex_lock(&mutex);
			PopMessage(&msg, &incoming[schedule]);
			pthread_mutex_unlock(&mutex);

			DEBUG {
				char ws[MAXLENWEIGHT];
				fprintf(logfile, "-> %s store:", WeightToString(MessageWeight(&msg), ws, sizeof(ws)));
				fprintf(logfile, "%d sign:%d side:%d\n", msg.vsource.sto, msg.vsource.side, msg.side);
			}

			DEBUG {
				char ws[MAXLENWEIGHT];
				fprintf(logfile, "-> %s store:", WeightToString(MessageWeight(&msg), ws, sizeof(ws)));
				fprintf(logfile, "%d sign:%d side:%d\n", msg.vsource.sto, msg.vsource.side, msg.side);
			}

			switch (msg.tpy) {
				case EOT_TAG: {
					node *sourceaddress;
					edge e;

					e = msg.vsource;
					sourceaddress = StoreBookedAddress(e.creator, (long)e.source, e.sto);
					EotArrival(sourceaddress, msg.side);
				}

				break;
				case COLD_TAG:
					StoreBookedAddress((msg.vtarget).creator, (long)(msg.vtarget).source, OUT);
					break;
				case ADD_TAG:
				case EOTADD_TAG: {
					node *targetaddress;
					edge e;

					targetaddress
					    = StoreBookedAddress((msg.vtarget).creator, (long)(msg.vtarget).source, msg.vtarget.sto);
					e = msg.vsource;
					AddEdge(targetaddress, e.rankpuit, e.source, e.sto, MessageWeight(&msg), e.creator, msg.side, e.side);

					EdgeArrival(targetaddress, &msg);
					/*            DEBUG Print(G,&incoming,edge_compositions); */
					time(&finaltime);
				} break;
			} /* END OF SWITCH */
			MessageFreeWeight(&msg);

			OUTPUT WriteStats();

			/*   schedule=0;
		     while (((schedule<MINPRIORITY)&&(!(nhot=BDumpS(&incoming[schedule]))))) schedule++;
		     schedule++;*/

			for (h = 0; h < size; h++) {
				tickcontrol[h]++;
				if ((outcontrol[h] > 0) && (tickcontrol[h] >= maxTick[h])) {
					nc1 = sizeof(int) + outbytes[h];
					packet_action_count = outcontrol[h];

					memcpy((char *)buf[h], (char *)&packet_action_count, sizeof(int));
					/***************** Carlo *******************************/
					nTickSend++;

					/*MARCO*/
					cumulate_timeout[h] += maxTick[h];
					applicative_msgs[h] += outcontrol[h];
					physical_msgs[h]++;

					global_physical_msgs++;

					/*** SendNum 3 : SEND per TIMEOUT **********************************/

					MPI_Bsend(buf[h], nc1, MPI_CHAR, h, DATA_TAG, MPI_COMM_WORLD);

					/*
					  FunReceiveMessages() ;
					  MPI_Wait(&ireq,&stsq) ;
					*/
					outcontrol[h] = 0;
					outbytes[h] = 0;
					tickcontrol[h] = 0;
				}
			}
		} /* END WHILE SCHEDULER */
	} /*  END of WHILE */

	for (h = 0; h < size; h++) {
		tickcontrol[h]++;
		if ((outcontrol[h] > 0) && (tickcontrol[h] >= maxTick[h])) {
			nc1 = sizeof(int) + outbytes[h];
			packet_action_count = outcontrol[h];

			memcpy((char *)buf[h], (char *)&packet_action_count, sizeof(int));
			/***************** Carlo ***************************************/
			nTickSend++;

			/* MARCO */
			cumulate_timeout[h] += maxTick[h];
			applicative_msgs[h] += outcontrol[h];
			physical_msgs[h]++;
			global_physical_msgs++;

			/*********** Sendnum 4 : SEND per TIMEOUT ************************/
			MPI_Bsend(buf[h], nc1, MPI_CHAR, h, DATA_TAG, MPI_COMM_WORLD);

			/*
			  FunReceiveMessages() ;
			  MPI_Wait(&ireq,&stsq);
			*/
			outcontrol[h] = 0;
			outbytes[h] = 0;
			tickcontrol[h] = 0;
		}
	} /* END FOR */

	loops++;
	pthread_exit(0);
}
#endif
