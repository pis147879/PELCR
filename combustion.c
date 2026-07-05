/* Processor of the workload

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
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DIE_NOTICE_ACTIVE 0
#define DIE_NOTICE_IDLE 1
#define DIE_NOTICE_FINAL 2

static int die_idle_notice_sent = 0;
static int die_idle_rank[MAXNPROCESS];
static int die_max_pending_load = 0;
static long pending_buffer_slots_hwm = 0;
static long pending_buffer_max_slots_hwm = 0;
static long pending_buffer_capacity_hwm = 0;
static int die_sendbuf[2 + 2 * MAXNPROCESS];
static int die_recvbuf[2 + 2 * MAXNPROCESS];
static int die_last_out_counter[MAXNPROCESS];
static int die_last_in_counter[MAXNPROCESS];

static int
StatsLogsDisabled(void) {
	const char *value;

	value = getenv("PELCR_DISABLE_STATS_LOGS");
	return value != NULL && value[0] != '\0' && strcmp(value, "0") != 0;
}

static long
EnvLongValue(const char *name, long fallback, long min_value, long max_value) {
	const char *value;
	char *end;
	long parsed;

	value = getenv(name);
	parsed = fallback;
	if (value != NULL && value[0] != '\0') {
		long env_value = strtol(value, &end, 10);
		if (end != value)
			parsed = env_value;
	}
	if (parsed < min_value)
		parsed = min_value;
	if (parsed > max_value)
		parsed = max_value;
	return parsed;
}

static unsigned long long
MachineRamBytes(void) {
#if defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
	long pages;
	long page_size;

	pages = sysconf(_SC_PHYS_PAGES);
	page_size = sysconf(_SC_PAGESIZE);
	if ((pages <= 0) || (page_size <= 0))
		return 0;
	return (unsigned long long)pages * (unsigned long long)page_size;
#else
	return 0;
#endif
}

static int
ClampPendingCapacity(long value) {
	if (value < 2)
		value = 2;
	if (value > INT_MAX)
		value = INT_MAX;
	return (int)value;
}

static int
PendingBufferCount(const struct mbuffer *l) {
	int count;

	if ((l == NULL) || (l->capacity <= 0))
		return 0;

	count = l->last - l->first;
	if (count < 0)
		count += l->capacity;
	return count;
}

static long
PendingBufferTotalCapacity(void) {
	int h;
	long total = 0;

	for (h = 0; h < MINPRIORITY; h++)
		total += incoming[h].capacity;
	return total;
}

static long
PendingBufferTotalMaxCapacity(void) {
	int h;
	long total = 0;

	for (h = 0; h < MINPRIORITY; h++)
		total += incoming[h].max_capacity;
	return total;
}

static long
PendingBufferTotalGrowCount(void) {
	int h;
	long total = 0;

	for (h = 0; h < MINPRIORITY; h++)
		total += incoming[h].grow_count;
	return total;
}

static long
PendingBufferTotalShrinkCount(void) {
	int h;
	long total = 0;

	for (h = 0; h < MINPRIORITY; h++)
		total += incoming[h].shrink_count;
	return total;
}

static void
RecordPendingBufferCapacity(void) {
	long capacity = PendingBufferTotalCapacity();

	if (capacity > pending_buffer_capacity_hwm)
		pending_buffer_capacity_hwm = capacity;
}

static void
ResizePendingBuffer(struct mbuffer *l, int new_capacity) {
	struct messaggio *new_stack;
	int count;
	int i;

	count = PendingBufferCount(l);
	if (new_capacity <= count)
		new_capacity = count + 1;
	if (new_capacity < l->min_capacity)
		new_capacity = l->min_capacity;
	if (new_capacity > l->max_capacity)
		new_capacity = l->max_capacity;
	if (new_capacity <= count) {
		printf("Pending buffer cannot resize: count=%d requested=%d max=%d\n", count, new_capacity, l->max_capacity);
		exit(-1);
	}

	new_stack = (struct messaggio *)malloc((size_t)new_capacity * sizeof(struct messaggio));
	if (new_stack == NULL) {
		printf("Pending buffer allocation failed: capacity=%d message_size=%lu\n", new_capacity, (unsigned long)sizeof(struct messaggio));
		exit(-1);
	}

	for (i = 0; i < count; i++)
		memcpy(&new_stack[i], &l->stack[(l->first + i) % l->capacity], sizeof(struct messaggio));

	free(l->stack);
	l->stack = new_stack;
	l->capacity = new_capacity;
	l->first = 0;
	l->last = count;
	if (l->capacity > l->capacity_hwm)
		l->capacity_hwm = l->capacity;
	RecordPendingBufferCapacity();
}

static void
GrowPendingBuffer(struct mbuffer *l) {
	int count;
	int new_capacity;

	count = PendingBufferCount(l);
	if (count < l->capacity - 1)
		return;
	if (l->capacity >= l->max_capacity) {
		printf("Exceeded size of incoming buffer: count=%d capacity=%d max=%d\n", count, l->capacity, l->max_capacity);
		exit(-1);
	}

	new_capacity = l->capacity * 2;
	if (new_capacity < l->capacity)
		new_capacity = l->max_capacity;
	if (new_capacity > l->max_capacity)
		new_capacity = l->max_capacity;

	ResizePendingBuffer(l, new_capacity);
	l->grow_count++;
	l->shrink_countdown = PELCR_PENDING_SHRINK_GRACE;
}

static void
MaybeShrinkPendingBuffer(struct mbuffer *l) {
	int count;
	int new_capacity;

	if ((l == NULL) || (l->stack == NULL) || (l->capacity <= l->min_capacity))
		return;

	count = PendingBufferCount(l);
	if (count >= (l->capacity / 8)) {
		l->shrink_countdown = PELCR_PENDING_SHRINK_GRACE;
		return;
	}

	if (l->shrink_countdown > 0) {
		l->shrink_countdown--;
		return;
	}

	new_capacity = l->capacity / 2;
	if (new_capacity < l->min_capacity)
		new_capacity = l->min_capacity;
	if (new_capacity <= count)
		new_capacity = count + 1;
	if (new_capacity >= l->capacity)
		return;

	ResizePendingBuffer(l, new_capacity);
	l->shrink_count++;
	l->shrink_countdown = PELCR_PENDING_SHRINK_GRACE;
}

static void
MaybeShrinkPendingBuffers(void) {
	int h;

	for (h = 0; h < MINPRIORITY; h++)
		MaybeShrinkPendingBuffer(&incoming[h]);
}

void
InitPendingBuffers(void) {
	int h;
	int hard_max;
	int min_capacity;
	int initial_capacity;
	int ram_percent;
	unsigned long long ram_bytes;
	unsigned long long budget_bytes;
	unsigned long long per_buffer_budget;
	long ram_capacity;

	hard_max = ClampPendingCapacity(EnvLongValue("PELCR_PENDING_MAX", MAXPENDING, 2, INT_MAX));
	min_capacity = ClampPendingCapacity(EnvLongValue("PELCR_PENDING_MIN", PELCR_PENDING_MIN_CAPACITY, 2, hard_max));
	ram_percent = (int)EnvLongValue("PELCR_PENDING_RAM_PERCENT", PELCR_PENDING_RAM_PERCENT, 0, 100);

	ram_bytes = MachineRamBytes();
	if ((ram_bytes > 0) && (ram_percent > 0) && (size > 0)) {
		budget_bytes = (ram_bytes / 100ULL) * (unsigned long long)ram_percent;
		per_buffer_budget = budget_bytes / (unsigned long long)size / (unsigned long long)MINPRIORITY;
		ram_capacity = (long)(per_buffer_budget / (unsigned long long)sizeof(struct messaggio));
		if (ram_capacity < min_capacity)
			ram_capacity = min_capacity;
		if (ram_capacity < hard_max)
			hard_max = ClampPendingCapacity(ram_capacity);
	}

	initial_capacity = ClampPendingCapacity(EnvLongValue("PELCR_PENDING_INITIAL", PELCR_PENDING_INITIAL_CAPACITY, min_capacity, hard_max));
	if (initial_capacity < min_capacity)
		initial_capacity = min_capacity;
	if (initial_capacity > hard_max)
		initial_capacity = hard_max;

	for (h = 0; h < MINPRIORITY; h++) {
		incoming[h].first = 0;
		incoming[h].last = 0;
		incoming[h].capacity = initial_capacity;
		incoming[h].min_capacity = min_capacity;
		incoming[h].max_capacity = hard_max;
		incoming[h].capacity_hwm = initial_capacity;
		incoming[h].shrink_countdown = PELCR_PENDING_SHRINK_GRACE;
		incoming[h].grow_count = 0;
		incoming[h].shrink_count = 0;
		incoming[h].stack = (struct messaggio *)malloc((size_t)initial_capacity * sizeof(struct messaggio));
		if (incoming[h].stack == NULL) {
			printf("Pending buffer allocation failed: capacity=%d message_size=%lu\n", initial_capacity, (unsigned long)sizeof(struct messaggio));
			exit(-1);
		}
	}
	RecordPendingBufferCapacity();

	if (rank == 0) {
		printf("Pending buffer: initial=%d min=%d max=%d hard_max=%d ram_percent=%d message_size=%lu\n",
		       initial_capacity, min_capacity, hard_max, MAXPENDING, ram_percent, (unsigned long)sizeof(struct messaggio));
	}
}

void
ResetPendingBuffers(void) {
	int h;

	for (h = 0; h < MINPRIORITY; h++) {
		incoming[h].first = 0;
		incoming[h].last = 0;
		if ((incoming[h].stack != NULL) && (incoming[h].capacity > incoming[h].min_capacity)) {
			ResizePendingBuffer(&incoming[h], incoming[h].min_capacity);
			incoming[h].shrink_count++;
		}
		incoming[h].shrink_countdown = PELCR_PENDING_SHRINK_GRACE;
	}
}

void
FreePendingBuffers(void) {
	int h;

	for (h = 0; h < MINPRIORITY; h++) {
		free(incoming[h].stack);
		incoming[h].stack = NULL;
		incoming[h].capacity = 0;
		incoming[h].first = 0;
		incoming[h].last = 0;
	}
}

static void
ResetDieProtocol() {
	int h;

	die_idle_notice_sent = 0;
	die_max_pending_load = 0;
	for (h = 0; h < MAXNPROCESS; h++)
		die_idle_rank[h] = 0;
	for (h = 0; h < MAXNPROCESS; h++) {
		die_last_out_counter[h] = -1;
		die_last_in_counter[h] = -1;
	}
}

static int
OutgoingPendingCount() {
	int h;
	int count = local_pending;

	for (h = 0; h < size; h++)
		count += outcontrol[h];

	return count;
}

static int
IncomingPendingCount() {
	int h;
	int count = 0;

#if MINPRIORITY > 1
	for (h = 0; h < MINPRIORITY; h++)
		count += BDumpS(&incoming[h]);
#else
	(void)h;
	count = BDumpS(&incoming[0]);
#endif

	return count;
}

static void
IncomingPendingStats(long *total_slots, long *max_slots) {
	int h;

	*total_slots = 0;
	*max_slots = 0;
	for (h = 0; h < MINPRIORITY; h++) {
		long slots = BDumpS(&incoming[h]);

		*total_slots += slots;
		if (slots > *max_slots)
			*max_slots = slots;
	}
}

void
ResetPendingBufferStats(void) {
	pending_buffer_slots_hwm = 0;
	pending_buffer_max_slots_hwm = 0;
	pending_buffer_capacity_hwm = PendingBufferTotalCapacity();
}

void
RecordPendingBufferLoad(void) {
	long total_slots;
	long max_slots;

	IncomingPendingStats(&total_slots, &max_slots);
	if (total_slots > pending_buffer_slots_hwm)
		pending_buffer_slots_hwm = total_slots;
	if (max_slots > pending_buffer_max_slots_hwm)
		pending_buffer_max_slots_hwm = max_slots;
	RecordPendingBufferCapacity();
}

static int
CurrentPendingLoad() {
	return pending_actions + IncomingPendingCount() + OutgoingPendingCount();
}

static void
UpdateDiePendingLoad() {
	int load = CurrentPendingLoad();

	if (load > die_max_pending_load)
		die_max_pending_load = load;
}

static int
DieProtocolIsArmed() {
	return die_max_pending_load >= DRAIN_LOAD_THRESHOLD;
}

static int
LocalWorkIsDrained() {
	return CurrentPendingLoad() == 0;
}

static void
PackDieMessage(int *dbuf, int state) {
	int h;

	dbuf[0] = rank;
	dbuf[1] = state;
	for (h = 0; h < size; h++) {
		dbuf[2 + h] = OutCounter[h];
		dbuf[2 + size + h] = InCounter[h];
	}
}

static void
RememberDieCounters(int source, int *dbuf) {
	int h;

	for (h = 0; h < size; h++) {
		OutTerminationStatus[source][h] = dbuf[2 + h];
		InTerminationStatus[source][h] = dbuf[2 + size + h];
	}
}

static void
RememberLastSentDieCounters() {
	int h;

	for (h = 0; h < size; h++) {
		die_last_out_counter[h] = OutCounter[h];
		die_last_in_counter[h] = InCounter[h];
	}
}

static int
DieCountersChangedSinceNotice() {
	int h;

	for (h = 0; h < size; h++)
		if ((die_last_out_counter[h] != OutCounter[h]) || (die_last_in_counter[h] != InCounter[h]))
			return TRUE;

	return FALSE;
}

static void
SendDieMessage(int dest, int state) {
	PackDieMessage(die_sendbuf, state);
	MPI_Send(die_sendbuf, 2 + 2 * size, MPI_INT, dest, DIE_TAG, MPI_COMM_WORLD);
	if (dest == 0)
		RememberLastSentDieCounters();
}

static void
PollDieMessages() {
	MPI_Status die_status;

	if (size == 1)
		return;

	MPI_Iprobe(MPI_ANY_SOURCE, DIE_TAG, MPI_COMM_WORLD, &dieflag, &die_status);
	while (dieflag) {
		MPI_Recv(die_recvbuf, 2 + 2 * size, MPI_INT, die_status.MPI_SOURCE, DIE_TAG, MPI_COMM_WORLD, &die_status);

		if (rank == 0) {
			if ((die_status.MPI_SOURCE > 0) && (die_status.MPI_SOURCE < size)) {
				RememberDieCounters(die_status.MPI_SOURCE, die_recvbuf);
				die_idle_rank[die_status.MPI_SOURCE] = (die_recvbuf[1] == DIE_NOTICE_IDLE);
			}
		} else if ((die_status.MPI_SOURCE == 0) && (die_recvbuf[1] == DIE_NOTICE_FINAL)) {
			end_computation = 1;
		}

		MPI_Iprobe(MPI_ANY_SOURCE, DIE_TAG, MPI_COMM_WORLD, &dieflag, &die_status);
	}
}

static int
AllRanksIdleAtRankZero() {
	int h;

	for (h = 1; h < size; h++)
		if (!die_idle_rank[h])
			return FALSE;

	return TRUE;
}

static int
DieOutCounter(int source, int dest) {
	if (source == rank)
		return OutCounter[dest];

	return OutTerminationStatus[source][dest];
}

static int
DieInCounter(int dest, int source) {
	if (dest == rank)
		return InCounter[source];

	return InTerminationStatus[dest][source];
}

static int
AllChannelsDrainedAtRankZero() {
	int source, dest;

	for (source = 0; source < size; source++)
		for (dest = 0; dest < size; dest++)
			if (DieOutCounter(source, dest) != DieInCounter(dest, source))
				return FALSE;

	return TRUE;
}

static void
BroadcastFinalDie() {
	int h;

	for (h = 1; h < size; h++)
		SendDieMessage(h, DIE_NOTICE_FINAL);
}

static void
UpdateDieProtocol() {
	int local_idle;

	PollDieMessages();
	UpdateDiePendingLoad();

	local_idle = LocalWorkIsDrained();
	if (size == 1) {
		if (local_idle)
			end_computation = 1;
		return;
	}

	if (rank == 0) {
		die_idle_rank[0] = local_idle;
		if (local_idle && AllRanksIdleAtRankZero() && AllChannelsDrainedAtRankZero()) {
			BroadcastFinalDie();
			end_computation = 1;
		}
		return;
	}

	if (!DieProtocolIsArmed() && !local_idle && !die_idle_notice_sent)
		return;

	if (local_idle) {
		if (!die_idle_notice_sent || DieCountersChangedSinceNotice()) {
			SendDieMessage(0, DIE_NOTICE_IDLE);
			die_idle_notice_sent = 1;
		}
	} else if (die_idle_notice_sent) {
		SendDieMessage(0, DIE_NOTICE_ACTIVE);
		die_idle_notice_sent = 0;
	}
}

edge *
InitReference(edge *aux) {
	if (aux == NULL)
		aux = NewReference();
	aux->vector = NULL;
	aux->source = NULL;
	aux->sto = IN;
	aux->side = LEFT;
	aux->sign = PLUS;
	aux->rankpuit = rank;
	strcpy(aux->weight, "");

	return aux;
}

static void
OpenStatsFile(void) {
	char basename[MAXNAMELEN];
	char statsname[MAXNAMELEN];
	const char *start;
	const char *dot;
	size_t len;
	size_t i;

	if (statsfile != NULL)
		return;

	if (StatsLogsDisabled())
		return;

	start = strrchr(infile, '/');
	start = (start == NULL) ? infile : start + 1;
	if (*start == '\0')
		start = "pelcr";

	dot = strrchr(start, '.');
	len = (dot == NULL) ? strlen(start) : (size_t)(dot - start);
	if (len >= sizeof(basename))
		len = sizeof(basename) - 1;
	memcpy(basename, start, len);
	basename[len] = '\0';

	for (i = 0; basename[i] != '\0'; i++)
		if (!isalnum((unsigned char)basename[i]) && basename[i] != '-' && basename[i] != '_')
			basename[i] = '_';

	if (basename[0] == '\0')
		strcpy(basename, "pelcr");

	if (size == 1)
		snprintf(statsname, sizeof(statsname), "LOGS/%s-np=%d-stats.log", basename, size);
	else
		snprintf(statsname, sizeof(statsname), "LOGS/%s-np=%d-rank=%d-stats.log", basename, size, rank);

	statsfile = fopen(statsname, "w");
	if (statsfile != NULL) {
		/* The old fra_hot pre-pop load snapshot is now incoming_actions_snapshot. */
		fprintf(statsfile,
		        "# wall_epoch time rank loops processed_actions edge_compositions fires one_optimizations failed_compositions graph_nodes hot_nodes cold_nodes nhot pending_actions "
		        "graph_edges local_pending incoming_pending incoming_buffer_capacity incoming_buffer_pct incoming_buffer_hwm incoming_buffer_hwm_pct "
		        "incoming_buffer_max_slots incoming_buffer_max_hwm outgoing_pending global_physical_msgs nTickSend nFullSend "
		        "incoming_buffer_capacity_hwm incoming_buffer_grow_count incoming_buffer_shrink_count incoming_buffer_max_capacity\n");
		fflush(statsfile);
	}
}

void
WriteStats() {
	int h;
	long incoming_pending;
	long incoming_buffer_capacity;
	long incoming_buffer_max_slots;
	long incoming_buffer_max_capacity;
	long incoming_buffer_grow_count;
	long incoming_buffer_shrink_count;
	double incoming_buffer_pct;
	double incoming_buffer_hwm_pct;
	long outgoing_pending = local_pending;

	if (processed_actions % FREQ)
		return;

	IncomingPendingStats(&incoming_pending, &incoming_buffer_max_slots);
	RecordPendingBufferLoad();
	MaybeShrinkPendingBuffers();
	incoming_buffer_capacity = PendingBufferTotalCapacity();
	incoming_buffer_max_capacity = PendingBufferTotalMaxCapacity();
	incoming_buffer_grow_count = PendingBufferTotalGrowCount();
	incoming_buffer_shrink_count = PendingBufferTotalShrinkCount();
	if (incoming_buffer_capacity > 0) {
		incoming_buffer_pct = (double)incoming_pending / (double)incoming_buffer_capacity;
		incoming_buffer_hwm_pct = (double)pending_buffer_slots_hwm / (double)incoming_buffer_capacity;
	} else {
		incoming_buffer_pct = 0.0;
		incoming_buffer_hwm_pct = 0.0;
	}

	for (h = 0; h < size; h++)
		outgoing_pending += outcontrol[h];

	OpenStatsFile();

	if ((tempfile != NULL) || (statsfile != NULL)) {
		double now;
		time_t wall_epoch;

		wall_epoch = time(NULL);
		/* Match the elapsed time printed in PrintResult: wall-clock seconds since evaluation start. */
		now = (inittime == 0) ? 0.0 : difftime(wall_epoch, inittime);

		if (tempfile != NULL) {
			fprintf(firfile, "%f %ld\n", now, fires);
			fprintf(tempfile, "%f %d\n", now, graph_nodes);
			/*fprintf(coldfile,"%f %d\n",now,received_action_count); */
			fprintf(hotfile, "%f %d\n", now, nhot);
			fprintf(trivfile, "%f %ld\n", now, one_optimizations);
			fprintf(nofile, "%f %ld\n", now, failed_compositions);
			fflush(firfile);
			fflush(hotfile);
			fflush(tempfile);
		}

		if (statsfile != NULL) {
			fprintf(statsfile, "%ld %f %d %ld %ld %ld %ld %ld %ld %d %ld %ld %d %d %ld %d %ld %ld %.6f %ld %.6f %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld\n",
			        (long)wall_epoch, now, rank, loops, processed_actions, edge_compositions, fires, one_optimizations, failed_compositions, graph_nodes, hot_nodes, cold_nodes, nhot, pending_actions,
			        graph_edges, local_pending, incoming_pending, incoming_buffer_capacity, incoming_buffer_pct, pending_buffer_slots_hwm, incoming_buffer_hwm_pct,
			        incoming_buffer_max_slots, pending_buffer_max_slots_hwm, outgoing_pending, global_physical_msgs, nTickSend, nFullSend,
			        pending_buffer_capacity_hwm, incoming_buffer_grow_count, incoming_buffer_shrink_count, incoming_buffer_max_capacity);
			fflush(statsfile);
		}
	}
}
/*END WRITE STATS*/

void
NodeCombustion(node *n, int polarity) {
	seminode v, vstar;
	edge *XI, *XJ;
	int counter;
	char a[MAXLENWEIGHT], b[MAXLENWEIGHT];
	char pos[MAXLENWEIGHT], neg[MAXLENWEIGHT];
	int proc, sto, outp;
	static edge *aux;
	struct messaggio m;

	aux = InitReference(aux);
	if (polarity == LEFT) {
		v = n->left;
		vstar = n->right;
	} else {
		v = n->right;
		vstar = n->left;
	}
	XJ = v.vector;
	XI = vstar.vector;
	counter = 0;
	while (XI != NULL) {
		counter++;

		/*
		 printf("----------> test XJ->weight = %s\n",XJ->weight);
		 printf("---- %d -->",counter);fflush(stdout);
		 printf(" test XI->weight = %s\n",XI->weight);
		*/

		strcpy(a, XJ->weight);
		strcpy(b, XI->weight);

		TRACING {
			fprintf(logfile, "(%d) FIRING: %d-th pair\n", rank, counter);
			fprintf(logfile, "(%d) COMPUTE(", rank);
			fprintf(logfile, "%s,%s", a, b);
			fprintf(logfile, ") Result:");
		}

		strcpy(pos, "");
		strcpy(neg, "");
		edge_compositions++;

		local_family_reductions = 0;
		outp = product(a, b, pos, neg);
		//    DEBUG    printf("(%d) number of family reductions %ld (node families %d)\n", rank,
		//    local_family_reductions,n->families);
		if (local_family_reductions > n->families)
			n->families = local_family_reductions;

		if (!outp) {
			TRACING fprintf(logfile, "NULL(%d)\n", outp);
			failed_compositions++;
		} else if (isone(pos) && (XJ->sto == IN)) {
			TRACING {
				Print(G, incoming, edge_compositions);
				fprintf(logfile, "(%d) verify one-optimization \n", rank);
				fprintf(logfile, "     OPT=%s STO=%d CHKONE=%d\n", pos, XJ->sto, isone(pos));
				fprintf(logfile, "     OPT=%s STO=%d CHKONE=%d\n", neg, XI->sto, isone(neg));
				fprintf(logfile, "OPT %s\n", neg);
				fprintf(logfile, "(%d)-", rank);
			}

			one_optimizations++;
			if ((outp == 40) || (outp == 42)) {
				TRACING fprintf(logfile, "\n\t\t CAMBIO DI POLARITA' da %d ", XJ->side);
				XJ->side = !XJ->side;
				TRACING fprintf(logfile, "a %d\n\n", XJ->side);
				StoreMessage(&m, XJ, XI, neg, XI->sto, XI->side);
				XJ->side = !XJ->side;
			} else
				StoreMessage(&m, XJ, XI, neg, XI->sto, XI->side);
			PushMessage(&m);
			XI->sign = MINUS;
		} else if (isone(neg) && (XI->sto == IN)) {
			// DEBUG Print(G,edge_compositions);
			TRACING {
				fprintf(logfile, "OPT %s\n", pos);
				fprintf(logfile, "(%d)+", rank);
			}
			one_optimizations++;
			if ((outp == 40) || (outp == 42)) {
				TRACING fprintf(logfile, "\n\t\t CAMBIO DI POLARITA' da %d", XI->side);
				XI->side = !XI->side;
				TRACING fprintf(logfile, "a %d\n\n", XI->side);
				StoreMessage(&m, XI, XJ, pos, XJ->sto, XJ->side);
				XI->side = !XI->side;
			} else
				StoreMessage(&m, XI, XJ, pos, XJ->sto, XJ->side);
			PushMessage(&m);
			XJ->sign = MINUS;
		} else {
			fires++;
			if ((XI->sto == OUT) || ((XJ->sto == OUT))) {
				proc = rank;
				sto = OUT;
				TRACING {
					fprintf(logfile, "%d%d%d+%s, %d%d%d-%s\n", sto, RIGHT, XI->side, pos, sto, LEFT, XJ->side, neg);
					fprintf(logfile, " ISONE+ %d ISONE- %d\n", isone(pos), isone(neg));
					fprintf(logfile, "(%d) SENDCOLD", rank);
				}
			} else {
				//	        proc= LightProcess3(rank);
				proc = LightProcess7(rank);
				sto = IN;
				TRACING {
					fprintf(logfile, "%d%d%d+%s, %d%d%d-%s\n", sto, RIGHT, XI->side, pos, sto, LEFT, XJ->side, neg);
					fprintf(logfile, " ISONE+ %d ISONE- %d\n", isone(pos), isone(neg));
					fprintf(logfile, "(%d) SENDHOT(%d,", rank, proc);
				}
			}
			TRACING fflush(logfile);
			/*DEBUG Print(G,edge_compositions);*/
			SendCreateNewNode(proc, sto, aux);
			TRACING fprintf(logfile, "(%d)- sto:%d pol:%d side:%d\n", rank, sto, LEFT, XJ->side);
			StoreMessage(&m, XJ, aux, neg, sto, LEFT);
			PushMessage(&m);
			TRACING fprintf(logfile, "(%d)+ sto:%d pol:%d side:%d", rank, sto, RIGHT, XI->side);
			StoreMessage(&m, XI, aux, pos, sto, RIGHT);
			PushMessage(&m);
		}
		XI = XI->vector;
	}
}

void
PushIncomingMessage(int priority, struct messaggio *m) {
	struct mbuffer *l;
	/*char c;*/

	// l= &incoming[priority];
	/*
	 printf("priority=%d\n",priority);fflush(stdout);
	 read(0,&c,1);
	 */

	l = &incoming[priority];

	GrowPendingBuffer(l);
	memcpy((char *)(&l->stack[l->last]), (char *)m, sizeof(struct messaggio));
	l->last = (l->last + 1) % l->capacity;

	pending_actions++;
	RecordPendingBufferLoad();
	return;
}

int
EmptyBuffer(struct mbuffer *l) {
	if (l->first == l->last)
		return 1;
	else
		return 0;
}

void
ShowMessage(struct messaggio *m) {
	fprintf(logfile, "(%d) %20s : %d[(%d).(%d)%p]->%d[(%d).(%d)%p]\n", rank, m->weight, m->vsource.side,
	        m->vsource.rankpuit, m->vsource.creator, (void *)m->vsource.source, m->side, m->vtarget.rankpuit,
	        m->vtarget.creator, (void *)m->vtarget.source);
	return;
}

void
StoreMessage(struct messaggio *m, edge *target, edge *source, char *weight, int storeclass, int pol) {
	int i;

	m->tpy = ADD_TAG;
	pelcr_wire_clear(m->funValue);
	for (i = 0; i < MAXNUMARG; i++)
		pelcr_wire_clear(m->funArgs[i]);
	if (target->sto == OUT)
		storeclass = OUT;
	/* m->sender_load = fra_hot; */
	m->sender_load = incoming_actions_snapshot;
	/*m->sender_load= nhot; graph_nodes;*/
	m->side = target->side;

	m->vsource.rankpuit = source->rankpuit;
	m->vsource.creator = source->creator;
	m->vsource.source = source->source;
	m->vsource.sto = storeclass;
	m->vsource.side = pol;

	m->vtarget.rankpuit = target->rankpuit;
	m->vtarget.creator = target->creator;
	m->vtarget.source = target->source;

	m->vtarget.sto = target->sto;
	if ((pol != LEFT) && (pol != RIGHT)) {
		/*printf("(%d) SIDE NON LEGALE\n");*/
		fflush(stdout);
	};
	m->vtarget.side = pol;

	if (strstr(weight, "X(3")) {
		char *atmp;
		int indice;
		atmp = strstr(weight, "X(3");
		indice = atoi(&atmp[4]);

		if (fcounter > -1) {
			m->funWhich = f[indice].which; // Va eliminata successivamente

			sprintf(&atmp[4], "%d)", f[indice].fun_id);
			m->funWait = f[indice].wait;
			for (i = 0; i < f[indice].wait - 1; i++)
				pelcr_value_to_wire(&f[indice].s[i], m->funArgs[i]);
		} else {
			m->funWhich = f_db[indice].which;
			m->funWait = f_db[indice].wait;
		}

	} else if (strstr(weight, "X(2")) {
		char *atmp;
		int indice;
		atmp = strstr(weight, "X(2");
		indice = atoi(&atmp[4]);

		pelcr_value_to_wire(&k_value[indice], m->funValue);
		m->funType = k_type[indice];
	}
	strcpy(m->weight, weight);
}

int
BDump(struct mbuffer *b) {
	int i;

	i = b->first;
	TRACING fprintf(logfile, "\nSTACK DUMP(%d-%d) VVVV: ", b->first, b->last);

	while (i != b->last) {
		TRACING fprintf(logfile, "%s ", (b->stack[i]).weight);
		i = (i + 1) % b->capacity;
	}

	TRACING fprintf(logfile, "\n");

	return PendingBufferCount(b);
}

int
BDumpS(struct mbuffer *b) {
	return PendingBufferCount(b);
}

void
FunReceiveMessages() {

	int received_action_count, i;

	char *position;
	int maxdim;

	int priority;
	int ub;

	if (size != 1) {
		/*  int received_action_count=0; */
		dataflag = 1;
		MPI_Iprobe(MPI_ANY_SOURCE, DATA_TAG, MPI_COMM_WORLD, &dataflag, &status);
		while (dataflag) {
			maxdim = sizeof(int) + (1 + MAXAWIN) * sizeof(struct messaggio);

			check_passed = FALSE;
			MPI_Recv(rbuf, maxdim, MPI_CHAR, MPI_ANY_SOURCE, DATA_TAG, MPI_COMM_WORLD, &status);
			position = rbuf + sizeof(int);

			memcpy((char *)&received_action_count, (char *)rbuf, sizeof(int));
			aggregation_cumulate += received_action_count;
			num_receives++;

			/*
			 if((tempfile!=NULL)&&(!(num_receives%40)))
			 {
			 float now;
			 times(&smtime);

			 now = (smtime.tms_utime+smtime.tms_stime)/60.0;
			 OUTPUT
			 {
			 fprintf(coldfile,"%f %d\n",now,received_action_count);

			 fprintf(anamfile,"%f %ld\n",now,
			 aggregation_cumulate/num_receives);

			 fflush(coldfile);
			 fflush(anamfile);
			 }
			 }

			 */

			/***************************************************
			 questo pezzo di codice deve realizzare lo scheduling,
			 l'aggregato e puntato da "position", va ora disaggregato
			 inserendo nel buffer incoming ogni messaggio, in relazione alla
			 euristica.
			 ********************************/

			/*
			 printf("(%d) RCVS %d, TOTAGG %d, AGGMSG %d\n",
			 rank,num_receives,aggregation_cumulate,received_action_count);
			 */

#ifdef _DEBUG
			DEBUG_AGGREGATION {
				printf("(%d) incoming message (of size %d)\n", rank, received_action_count);
				fflush(stdout);
			}
#endif
			/*  for(i= 0;i<AGGREGATIONWINDOW;i++)	   */

			for (i = 0; i < received_action_count; i++) {

#if MINPRIORITY > 1
				ub = UpperBound((struct messaggio *)position);

				priority = floor((MINPRIORITY - 1) * (1 - (float)ub / (float)maxubound));

				/*
				 ShowMessage((struct messaggio*)position);
				 */

#ifdef _DEBUG
				DEBUG_AGGREGATION {
					printf("(%d) ub = %d maxub = %d - priority = %d\n", rank, ub, maxubound, priority);
					fflush(stdout);
				}
#endif

				PushIncomingMessage(priority, (struct messaggio *)position);
#else
				PushIncomingMessage(0, (struct messaggio *)position);
#endif
				position = position + sizeof(struct messaggio);
			}
			MPI_Iprobe(MPI_ANY_SOURCE, DATA_TAG, MPI_COMM_WORLD, &dataflag, &status);
		}
	}

	buf_flush();

	batch_processed_actions = 0;
	schedule = 0;

	TRACING {
		//    printf("(%d) exiting receive phase (loops= %ld)\n",rank,loops);
		fflush(stdout);
	}
}

void
FunInteraction() {
	int batch_processed_actions = 0;
	int nc1, packet_action_count;
	int h;
#if MINPRIORITY > 1
	int z;
#endif

	TRACING { // printf("(%d) before while (pending_actions = %d)\n",rank,pending_actions);
		fflush(stdout);
	};

	while ((pending_actions > 0) && (batch_processed_actions < CHECKTICKS)) {

		lidle = idle;
		idle += loops - 1;

		loops = 0;

		TRACING fprintf(logfile, "(%d) ...seeking a non-empty incoming buffer\n", rank);
		//      DEBUG	printf("(%d) *",rank);

		/* fra_hot = 0; */
#if MINPRIORITY > 1
		incoming_actions_snapshot = 0;
		/* for (z = 0; z < MINPRIORITY; z++) */
		for (z = 0; z < MINPRIORITY; z++)
		/* 	fra_hot += BDumpS(&incoming[z]); */
			incoming_actions_snapshot += BDumpS(&incoming[z]);
#else
		incoming_actions_snapshot = BDumpS(&incoming[0]);
#endif

#if MINPRIORITY > 1
		while (((schedule < MINPRIORITY) && (!(nhot = BDump(&incoming[schedule]))))) {
			TRACING fprintf(logfile, "(%d) BUFFER %d ", rank, schedule);
			schedule++;
			TRACING {
				//	    printf(".");
				fflush(stdout);
			};
		};
#else
		schedule = 0;
		nhot = BDump(&incoming[0]);
		if (!nhot)
			schedule = MINPRIORITY;
#endif

		//      DEBUG printf("\n*\n");
		TRACING {
			fflush(stdout);
			fprintf(logfile, "(%d) %d is the first non empty \n", rank, schedule);
			if (schedule >= MINPRIORITY) {
				printf("(%d) no work\n", rank);
				fflush(stdout);
			}
		}

		if (schedule < MINPRIORITY) {
			processed_actions++;
			batch_processed_actions++;
			TRACING fprintf(logfile, "(%d) POP(%d) \n", rank, schedule);
			PopMessage(&msg, &incoming[schedule]);

			TRACING {
				fprintf(logfile, "-> %s store:", msg.weight);
				fprintf(logfile, "%d sign:%d side:%d\n", msg.vsource.sto, msg.vsource.side, msg.side);
				//  printf("(%d) before switch\n",rank);
				fflush(stdout);
			}

			/* processing the message */
			switch (msg.tpy) {
				case EOT_TAG: {
					node *sourceaddress;
					edge e;
					e = msg.vsource;
					sourceaddress = StoreBookedAddress(e.creator, (long)e.source, e.sto);
					if (msg.side == LEFT)
						sourceaddress->left.eot++;
					else
						sourceaddress->right.eot++;

					if ((e.sto != OUT) && (sourceaddress->left.eot == 1) && (sourceaddress->right.eot == 1)) {
						// sourceaddress=
						SinkRemove(sourceaddress);
					}
				} break;

				case ADD_TAG: {
					node *targetaddress;
					edge e;

					/* the first edge on the rank 0 process correspond to
					 the principal conclusion of the proof net, so it is the
					 starting point of the read_back procedure */

					targetaddress
					    = StoreBookedAddress((msg.vtarget).creator, (long)(msg.vtarget).source, msg.vtarget.sto);
					if ((rank == 0) && (principal == 0)) {
						principal = targetaddress;
						TRACING printf("(%d) ********** principal port address %p\n", rank, principal);
					};

					e = msg.vsource;

					AddEdge(targetaddress, e.rankpuit, e.source, e.sto, msg.weight, e.creator, msg.side, e.side);

					//		DEBUG       printf("(%d) before combustion\n",rank);
					//		DEBUG       fflush(stdout);
					NodeCombustion(targetaddress, msg.side);
					//		DEBUG       printf("(%d) after combustion\n",rank);
					//		DEBUG       fflush(stdout);
					/*		DEBUG Print(G,incoming,edge_compositions);*/
					/*  tim = time(&finaltime); */
				} break;
			} /* END OF SWITCH */

			WriteStats();

			//  DEBUG  printf("(%d) after switch\n",rank);
			//	  DEBUG  fflush(stdout);

			/*   schedule=0;
			 while (((schedule<MINPRIORITY)&&(!(nhot=BDumpS(&incoming[schedule]))))) schedule++;
			 schedule++;
			 */

			tim = time(&finaltime);

			for (h = 0; h < size; h++) {

				/*	            if(outcontrol[h] > 0)  tickcontrol[h]++; */

				tickcontrol[h]++;

				if ((outcontrol[h] > 0) && (tickcontrol[h] >= maxTick[h])) {
					nc1 = sizeof(int) + outcontrol[h] * sizeof(struct messaggio);
					packet_action_count = outcontrol[h];

					memcpy((char *)buf[h], (char *)&packet_action_count, sizeof(int));
					/***************** Carlo *******************************/
					nTickSend++;
					nrApplMsg[h] += outcontrol[h];
					nrFisicMsg[h]++;

					/* VAB */

					/*
					 current_rate = (float)outcontrol[h]/(float)maxTick[h];
					 if(last_rate[h] >= current_rate){
					 maxTick[h] -= TICK_MOVE;
					 }
					 else{
					 maxTick[h] += TICK_MOVE;
					 }

					 if(maxTick[h] <= 0) maxTick[h] =1;

					 last_rate[h] = current_rate;

					 */

					current_rate = (float)outcontrol[h] / (float)maxTick[h];

					if ((1.1 * last_rate[h]) <= current_rate) {
						maxTick[h] += TICK_MOVE;
					} else {
						if ((0.9 * last_rate[h]) >= current_rate) {
							maxTick[h] -= TICK_MOVE;
						}
					}

					if (maxTick[h] <= 0)
						maxTick[h] = 1;

					last_rate[h] = current_rate;

					/*MARCO*/
					cumulate_timeout[h] += maxTick[h];
					applicative_msgs[h] += outcontrol[h];
					physical_msgs[h]++;
					global_physical_msgs++;

					/*** SendNum 3 : SEND per TIMEOUT **********************************/
#ifdef _DEBUG
					DEBUG_DISTRIBUTION {
						printf("(%d) before send to %d (messages %d - taglia %d) \n", rank, h, outcontrol[h], nc1);
						fflush(stdout);
					}
#endif

					/* tentative: force a receive step before starting a send */
					/*FunReceiveMessages() ;*/
					MPI_Bsend(buf[h], nc1, MPI_CHAR, h, DATA_TAG, MPI_COMM_WORLD);

#ifdef _DEBUG
					DEBUG_DISTRIBUTION {
						printf("(%d) after send\n", rank);
						fflush(stdout);
					};
#endif
					outcontrol[h] = 0;
					tickcontrol[h] = 0;
				}
				/*
				 if(outcontrol[h] == 0) tickcontrol[h]=0;
				 */

			} /* end for */
		} /* END IF SCHEDULE */
	} /*  END of WHILE */

	//  DEBUG  printf("(%d) after while\n",rank);
#ifdef _DEBUG
	fflush(stdout);
#endif

	for (h = 0; h < size; h++) {
		/* tickcontrol[h]++;
		 if((outcontrol[h]>0) && (tickcontrol[h]>=maxTick[h])) */

		if (outcontrol[h] > 0) {
			nc1 = sizeof(int) + outcontrol[h] * sizeof(struct messaggio);
			packet_action_count = outcontrol[h];

			memcpy((char *)buf[h], (char *)&packet_action_count, sizeof(int));
			/***************** Carlo ***************************************/
			nTickSend++;
			nrApplMsg[h] += outcontrol[h];
			nrFisicMsg[h]++;

			/* MARCO */
			cumulate_timeout[h] += maxTick[h];
			applicative_msgs[h] += outcontrol[h];
			physical_msgs[h]++;
			global_physical_msgs++;

			/*********** SendNum 4 : SEND per TIMEOUT ************************/
			/*FunReceiveMessages() ;*/

			MPI_Bsend(buf[h], nc1, MPI_CHAR, h, DATA_TAG, MPI_COMM_WORLD);

			outcontrol[h] = 0;
			tickcontrol[h] = 0;
		}
	}

	loops++;
}

void
ComputeResult() {
	printf("(%d) running...\n", rank);
	ResetDieProtocol();
	while ((!end_computation) && ((maxfires == 0) || edge_compositions < maxfires)) {
		maxubound = 1;
		if (!THREAD) {
			UpdateDiePendingLoad();
			FunReceiveMessages();
			UpdateDiePendingLoad();
			FunInteraction();
			UpdateDieProtocol();
		}
		if (THREAD) {
			exit(-1);
			/*
			 pthread_t threadID1, threadID2;
			 pthread_setconcurrency(2);
			 pthread_create(&threadID1, NULL, ThreadReceiveMsgs, NULL);
			 pthread_create(&threadID2, NULL, ThreadInteraction, NULL);

			 pthread_join(threadID1, NULL);
			 pthread_join(threadID2, NULL);
			 */
		}
	};

	printf("(%d) ...ending\n", rank);
	printf("(%d) read back procedure call \n", rank);
	read_back(principal);
	/* Temporarily skip the final graph dump while validating multi-rank runs. */
	/* OUTPUTFILE Print(G, incoming, 1000000); */
	printf("(%d) starting finalize \n", rank);
}

static int
CountNodeList(node *head) {
	int count = 0;
	node *cursor;

	for (cursor = head; cursor != NULL; cursor = cursor->nextpuit)
		count++;
	return count;
}

void
PrintResult() {
	long hot_node_count;
	long cold_node_count;

	AttachSingleEotHotNodesToCold();

	hot_node_count = hot_nodes;
	cold_node_count = cold_nodes;

#ifdef SOLARIS
	int lh;
	char *hostname[256], domainname[256];
	char *uid;
	time_t day;
	int h;

	uid = getlogin();
	lh = 30;
	gethostname(hostname, lh);
	lh = 60;
	getdomainname(domainname, lh);
	day = time(NULL);
	printf("user   : %s\n", uid);
	printf("host   : %s\n", hostname);
	printf("domain : %s\n", domainname);
	printf("date   : %s\n", ctime(&day));

#ifdef _DEBUG
	TRACING {
		fprintf(logfile, "user   : %s\n", uid);
		fprintf(logfile, "host   : %s\n", hostname);
		fprintf(logfile, "domain : %s\n", domainname);
		fprintf(logfile, "date   : %s\n", ctime(&day));
#endif
#endif

#ifdef LINUX
		int lh;
		char *hostname[256], domainname[256];
		char *uid;
		time_t day;
		int h;
		uid = getlogin();

		lh = 30;
		gethostname(hostname, (size_t)lh);
		lh = 60;
		getdomainname(domainname, lh);
		day = time(NULL);
		printf("user   : %s\n", uid);
		printf("host   : %s\n", hostname);
		printf("domain : %s\n", domainname);
		printf("date   : %s\n", ctime(&day));

		TRACING {
			fprintf(logfile, "user   : %s\n", uid);
			fprintf(logfile, "host   : %s\n", hostname);
			fprintf(logfile, "domain : %s\n", domainname);
			fprintf(logfile, "date   : %s\n", ctime(&day));
		}
#endif

		printf("(%d) elapsed time        :  %d\n", rank, ((int)finaltime - (int)inittime));
		printf("(%d) final nodes         :  %d\n", rank, graph_nodes);
		printf("(%d) hot nodes           :  %ld\n", rank, hot_node_count);
		printf("(%d) cold nodes          :  %ld\n", rank, cold_node_count);
		printf("(%d) edge compositions   :  %ld\n", rank, edge_compositions);
		printf("(%d) fires               :  %ld\n", rank, fires);
		printf("(%d) trivial             :  %ld optimized\n", rank, one_optimizations);
		printf("(%d) family reductions   :  %ld\n", rank, family_reductions);
		printf("(%d) loops               :  %ld\n", rank, lastloop);
		printf("(%d) computing loops     :  %ld\n", rank, computing_loops);
		printf("(%d) idle loops          :  %ld (%ld)\n", rank, lidle, idle);
		printf("(%d) listen loops        :  %ld\n", rank, lbip2);
		printf("(%d) received messages   :  %ld\n", rank, num_receives);
		printf("(%d) unsuccessful tests  :  (eot %d), (add %d)\n", rank, leottest, laddtest);
		printf("(%d) dieflag             :  %d\n", rank, dieflag);

		/* print to stdout all main values on aggragation */

		printf(" (%d) AVERAGE AGGREGATION in RECEIVED MESSAGES: %06.1f\n", rank,
		       (float)((float)aggregation_cumulate / (float)num_receives));
		printf(" (%d) TIMEOUT(max,min,delta) : %06d %06d %06.1f %06.1f\n", rank, MAXTICK, MINTICK, BETATICK, ALPHATICK);
		printf(" (%d) WINDOW(max,min,delta)  : %06d %06d %06.1f %06.1f\n", rank, MAXAWIN, MINAWIN, BETA, ALPHA);
		printf(" (%d) THRESHOLD(left,right)  : %06.1f %06.1f\n", rank, TRHLEFT, TRHRIGHT);
		printf(" (%d) FULLNESS COEFFICIENT   : %06.1f (full physical messages %ld)\n", rank,
		       (float)(nFullSend / (float)global_physical_msgs), nFullSend);

		for (h = 0; h < size; h++) {
			if (h != rank) {
				printf("SEND TO %d : average timeout = %06.1f, average window = %06.1f ,", h,
				       (float)((float)cumulate_timeout[h] / (float)physical_msgs[h]),
				       (float)((float)applicative_msgs[h] / (float)physical_msgs[h]));
				printf("n. physical messages = %ld, n. applicative messages = %ld \n", physical_msgs[h],
				       applicative_msgs[h]);
			};
		};

		TRACING {
			fprintf(logfile, "AVERAGE AGGREGATION in RECEIVED MESSAGES: %f\n\n",
			        (float)((float)aggregation_cumulate / (float)num_receives));

			fprintf(logfile, "TIMEOUT(max,min,delta) : %d %d %f %f\n", MAXTICK, MINTICK, BETATICK, ALPHATICK);
			fprintf(logfile, "WINDOW(max,min,delta) : %d %d %f %f\n", MAXAWIN, MINAWIN, BETA, ALPHA);
			fprintf(logfile, "THRESHOLD(left,right) : %f %f\n", TRHLEFT, TRHRIGHT);
			fprintf(logfile, "FULLNESS COEFFICIENT : %f (full physical messages %ld)\n\n",
			        (float)(nFullSend / (float)global_physical_msgs), nFullSend);

			for (h = 0; h < size; h++) {
				if (h != rank) {
					fprintf(logfile, "SEND TO %d : average timeout = %f, average window = %f ,", h,
					        (float)((float)cumulate_timeout[h] / (float)physical_msgs[h]),
					        (float)((float)applicative_msgs[h] / (float)physical_msgs[h]));
					fprintf(logfile, "n. physical messages = %ld, n. applicative messages = %ld \n", physical_msgs[h],
					        applicative_msgs[h]);
				}
			}

			fprintf(logfile, "(%d) elapsed time         :  %d\n", rank, ((int)finaltime - (int)inittime));
			fprintf(logfile, "(%d) final nodes         :  %d\n", rank, graph_nodes);
			fprintf(logfile, "(%d) hot nodes           :  %ld\n", rank, hot_node_count);
			fprintf(logfile, "(%d) cold nodes          :  %ld\n", rank, cold_node_count);
			fprintf(logfile, "(%d) edge compositions   :  %ld\n", rank, edge_compositions);
			fprintf(logfile, "(%d) fires                :  %ld\n", rank, fires);
			fprintf(logfile, "(%d) loops                :  %ld\n", rank, lastloop);
			fprintf(logfile, "(%d) computing Loops      :  %ld\n", rank, computing_loops);
			fprintf(logfile, "(%d) idle Loops           :  %ld (%ld)\n", rank, lidle, idle);
			fprintf(logfile, "(%d) listen Loops         :  %ld\n", rank, lbip2);
			fprintf(logfile, "(%d) received messages    :  %d\n", rank, h);
			fprintf(logfile, "(%d) unsuccessful Tests   :  (eot %d), (add %d)\n\n", rank, leottest, laddtest);

			fprintf(logfile, "(%d) logging out -> enter mpi_finalize\n", rank);
			fprintf(logfile, "termination task: rank %d\n", rank);
		}
	}

	void Finally() {

		printf("(%d) logging out -> enter mpi_finalize\n", rank);
		MPI_Finalize();
		printf("(%d) [OK] out from MPI\n", rank);
#ifdef MAIL
		if (rank == 0) {
			system("cat LOGS/run.*.log > LOGS/run.log");
			{
				char command[200];
#ifdef LINUX
				sprintfx(command, "mail -s %s.np%d@%s marco@localhost < LOGS/run.log", infile, size, hostname);
#endif
#ifdef SOLARIS
				sprintf(command, "mailx -s %s.np%d@%s marco@iac.rm.cnr.it < LOGS/run.log", infile, size, hostname);
#endif
				system(command);
				printf("Sent Aknowledgement: %s\n", command);
			};
		};
#endif
		printf("EXIT ...\n\n");
	}

	/**************** Scheduler 6/10/2001 *****************************/
	int UpperBound(struct messaggio * m) {
		/* questa funzione calcola una approssimazione del carico esterno
		 prodotto da un certo arco, il valore associato ad un arco e' dato
		 dal numero di archi incidenti con sorgente esterna se l'arco stesso
		 ha sorgente interna ed e' pari al doppio del numero di archi con
		 sorgente esterna se la sorgente dell'arco in arrivo e' esterna */

		int euristic = 0;
		node *targetaddress = NULL;
		edge e;

		targetaddress = StoreBookedAddress((m->vtarget).creator, (long)(m->vtarget).source, m->vtarget.sto);
		e = m->vsource;

		TRACING {
			fprintf(logfile, "(%d) ****** target address = %ld [%p]\n", rank, (long int)targetaddress,
			        (void *)targetaddress);

			//    printf("(%d) target address = %ld [%p]\n", rank,(long int)targetaddress,targetaddress);
			fflush(stdout);
			fflush(logfile);
		}
		if (m->tpy == EOT_TAG) {
			euristic = 0;
			/*printf("eot\n");*/
			/* poiche' il risultato di questa funzione e' normalizzato
			 questo corrisponde ad assegnare la minima priorita',
			 garantisce  l'ordine di processamento tra archi ed eot */
		} else {
			int coeff_locality;

			coeff_locality = !((e.rankpuit) == rank);

			/* l'arco da inserire ha sorgente locale */

			euristic = targetaddress->mu_remote + coeff_locality * targetaddress->mu_local;
			/* calcolo dell'euristica  descritta in Pedicini - Quaglia 2001 */
			/* euristica = (2 r + l) se remoto ed = (r) se locale */

			if (coeff_locality) {

				targetaddress->mu_remote++;
			}

			else
				targetaddress->mu_local++;
			/* incremento delle misure sul nodo target, in questo modo */
			/* vengono conteggiati anche gli archi che sono nelle code e */
			/* che arriveranno prima dell'arco */
		};

		if (maxubound < euristic)
			maxubound = euristic;

		return euristic;
	}
