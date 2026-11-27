/* Frontend and backend of PELCR: the channel between them (see h/channel.h)

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

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "var.h"

#ifndef MSG_NOSIGNAL /* macOS: SIGPIPE is ignored instead (ch_nonblocking) */
#define MSG_NOSIGNAL 0
#endif

extern int yyparse(void);

int pelcr_role = PELCR_ALONE;
int principal_rank = INITIALIZERPROCESS;

/* ------------------------------------------------------------------------------------------- */
/* blocks on the channel: a header and, for CH_ACTIONS, the records of h/message.h */

#define CH_HELLO 1    /* backend -> frontend: count = the ranks of the backend */
#define CH_ACTIONS 2  /* both ways: count records */
#define CH_TERM_END 3 /* frontend -> backend: the term is over */
#define CH_STATS 4    /* backend -> frontend: the counters of the term (count longs) */
#define CH_BYE 5      /* frontend -> backend: the channel closes */

typedef struct ch_header {
	int type;
	int count;
	long bytes; /* of what follows the header */
} ch_header;

static int ch_fd = -1;     /* the channel */
static int ch_listen = -1; /* the gateway: the socket that accepts channels */

/* output: complete blocks not yet written (from out_sent), then the block being filled */
static char *out_buf = NULL;
static size_t out_len = 0, out_cap = 0, out_sent = 0;
static size_t blk_start = 0;
static int blk_count = 0, blk_open = 0;

/* input of the gateway: bytes read and not yet a complete block */
static char *in_buf = NULL;
static size_t in_len = 0, in_cap = 0;

static long ch_records_out = 0, ch_records_in = 0;

static void
ch_reserve(char **b, size_t *cap, size_t need) {
	if (need <= *cap)
		return;
	size_t c = (*cap == 0) ? 65536 : *cap;
	while (c < need)
		c *= 2;
	char *p = (char *)realloc(*b, c);
	if (p == NULL) {
		fprintf(stderr, "(%d) PELCR channel: no memory for %zu bytes\n", rank, c);
		exit(-1);
	}
	*b = p;
	*cap = c;
}

/* the record of m in the block being filled; m no longer owns its weight (MessagePack) */
static void
ch_write_record(struct messaggio *m) {
	if (!blk_open) {
		ch_reserve(&out_buf, &out_cap, out_len + sizeof(ch_header));
		blk_start = out_len;
		out_len += sizeof(ch_header);
		blk_count = 0;
		blk_open = 1;
	}
	ch_reserve(&out_buf, &out_cap,
	           out_len + sizeof(int) + MESSAGE_FIXED + W_BYTES(MessageWeight(m)) + MESSAGE_FUN);
	out_len += MessagePack(out_buf + out_len, m);
	blk_count++;
	ch_records_out++;
}

static void
ch_close_block(void) {
	ch_header h;

	if (!blk_open)
		return;
	h.type = CH_ACTIONS;
	h.count = blk_count;
	h.bytes = (long)(out_len - blk_start - sizeof(ch_header));
	memcpy(out_buf + blk_start, &h, sizeof(h));
	blk_open = 0;
}

/* a block without records, or with count longs */
static void
ch_put_block(int type, int count, const void *payload, long bytes) {
	ch_header h;

	ch_close_block();
	h.type = type;
	h.count = count;
	h.bytes = bytes;
	ch_reserve(&out_buf, &out_cap, out_len + sizeof(h) + (size_t)bytes);
	memcpy(out_buf + out_len, &h, sizeof(h));
	out_len += sizeof(h);
	if (bytes > 0) {
		memcpy(out_buf + out_len, payload, (size_t)bytes);
		out_len += (size_t)bytes;
	}
}

/* write the complete blocks: all of them (blocking), or what the socket takes now */
static void
ch_flush(int blocking) {
	ch_close_block();
	while ((ch_fd >= 0) && (out_sent < out_len)) {
		ssize_t n = send(ch_fd, out_buf + out_sent, out_len - out_sent, MSG_NOSIGNAL);
		if (n > 0) {
			out_sent += (size_t)n;
			continue;
		}
		if ((n < 0) && (errno == EINTR))
			continue;
		if ((n < 0) && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
			struct pollfd p;
			if (!blocking)
				return;
			p.fd = ch_fd;
			p.events = POLLOUT;
			poll(&p, 1, 1000);
			continue;
		}
		fprintf(stderr, "(%d) PELCR channel: write failed (%s)\n", rank, strerror(errno));
		close(ch_fd);
		ch_fd = -1;
		break;
	}
	if (out_sent == out_len)
		out_sent = out_len = 0;
}

/* blocking read of n bytes; 0 if the channel closed */
static int
ch_read_all(void *dst, size_t n) {
	char *p = (char *)dst;

	while (n > 0) {
		ssize_t r = recv(ch_fd, p, n, 0);
		if (r > 0) {
			p += r;
			n -= (size_t)r;
		} else if ((r < 0) && (errno == EINTR))
			continue;
		else if ((r < 0) && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
			struct pollfd q;
			q.fd = ch_fd;
			q.events = POLLIN;
			poll(&q, 1, 1000);
		} else
			return 0;
	}
	return 1;
}

static void
ch_nonblocking(int fd) {
	int one = 1;

	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
	signal(SIGPIPE, SIG_IGN); /* a channel that closes is seen by send (EPIPE) */
}

/* ------------------------------------------------------------------------------------------- */
/* where the nodes are */

/* the hot initial nodes of a frontend go to the ranks of the backend in round robin, in the order
   in which InitializeIncoming meets them; every message names a node with the same rank */
static node **initial_node = NULL;
static int *initial_rank = NULL;
static long initial_cap = 0, initial_used = 0;
static int initial_next = 0;

void
ChannelInitialReset(void) {
	free(initial_node);
	free(initial_rank);
	initial_node = NULL;
	initial_rank = NULL;
	initial_cap = initial_used = 0;
	initial_next = 0;
}

static unsigned long
pointer_hash(const void *p) {
	unsigned long z = (unsigned long)p;

	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9UL;
	z = (z ^ (z >> 27)) * 0x94d049bb133111ebUL;
	return z ^ (z >> 31);
}

static int
initial_round_robin(node *n) {
	unsigned long i;

	if (2 * (initial_used + 1) > initial_cap) {
		node **on = initial_node;
		int *orr = initial_rank;
		long oc = initial_cap, j;

		initial_cap = (initial_cap == 0) ? 1024 : 2 * initial_cap;
		initial_node = (node **)calloc((size_t)initial_cap, sizeof(node *));
		initial_rank = (int *)calloc((size_t)initial_cap, sizeof(int));
		if ((initial_node == NULL) || (initial_rank == NULL)) {
			fprintf(stderr, "(%d) PELCR frontend: no memory for the initial nodes\n", rank);
			exit(-1);
		}
		for (j = 0; j < oc; j++)
			if (on[j] != NULL) {
				for (i = pointer_hash(on[j]) % (unsigned long)initial_cap; initial_node[i] != NULL;
				     i = (i + 1) % (unsigned long)initial_cap)
					;
				initial_node[i] = on[j];
				initial_rank[i] = orr[j];
			}
		free(on);
		free(orr);
	}
	for (i = pointer_hash(n) % (unsigned long)initial_cap; initial_node[i] != NULL; i = (i + 1) % (unsigned long)initial_cap)
		if (initial_node[i] == n)
			return initial_rank[i];
	initial_node[i] = n;
	initial_rank[i] = initial_next;
	initial_next = (initial_next + 1) % size;
	initial_used++;
	return initial_rank[i];
}

int
ChannelInitialRank(node *n) {
	if (pelcr_role == PELCR_FRONTEND)
		return (n->sto == OUT) ? rank : initial_round_robin(n);
	return rank;
}

int
ColdNodeRank(edge *XI, edge *XJ) {
	if (pelcr_role == PELCR_BACKEND)
		return (XI->sto == OUT) ? XI->rankpuit : XJ->rankpuit;
	return rank;
}

void
ChannelMarkCreated(struct messaggio *m, edge *other) {
	if ((pelcr_role == PELCR_BACKEND) && (m->vtarget.rankpuit >= size))
		m->vtarget.sign = CH_CREATED | ((other->rankpuit < size) ? CH_OTHER_BACKEND : 0);
}

/* ------------------------------------------------------------------------------------------- */
/* the debts of the frontend: the EOTs that the sides of its nodes wait from the backend */

typedef struct debt {
	long key; /* the node: its key for its creator (an address, or a booking) */
	int creator;
	int side;
	int balance;
	int used;
} debt;

static debt *debts = NULL;
static long debts_cap = 0, debts_used = 0;
static long unbalanced = 0; /* sides with balance != 0 */
static long debt_initial = 0, debt_created = 0, debt_moved = 0, debt_eot = 0;

static unsigned long
debt_hash(int creator, long key, int side) {
	unsigned long z = (unsigned long)key ^ ((unsigned long)creator << 48) ^ ((unsigned long)side << 63);

	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9UL;
	z = (z ^ (z >> 27)) * 0x94d049bb133111ebUL;
	return z ^ (z >> 31);
}

static debt *
debt_find(int creator, long key, int side) {
	unsigned long i;

	if (2 * (debts_used + 1) > debts_cap) {
		debt *old = debts;
		long oldcap = debts_cap, j;

		debts_cap = (debts_cap == 0) ? 4096 : 2 * debts_cap;
		debts = (debt *)calloc((size_t)debts_cap, sizeof(debt));
		if (debts == NULL) {
			fprintf(stderr, "(%d) PELCR frontend: no memory for the debts\n", rank);
			exit(-1);
		}
		debts_used = 0;
		for (j = 0; j < oldcap; j++)
			if (old[j].used) {
				debt *d = debt_find(old[j].creator, old[j].key, old[j].side);
				d->balance = old[j].balance;
			}
		free(old);
	}
	for (i = debt_hash(creator, key, side) % (unsigned long)debts_cap;; i = (i + 1) % (unsigned long)debts_cap) {
		debt *d = &debts[i];
		if (!d->used) {
			d->used = 1;
			d->creator = creator;
			d->key = key;
			d->side = side;
			d->balance = 0;
			debts_used++;
			return d;
		}
		if ((d->creator == creator) && (d->key == key) && (d->side == side))
			return d;
	}
}

static void
debt_add(int creator, long key, int side, int delta) {
	debt *d = debt_find(creator, key, side);
	int before = d->balance;

	d->balance += delta;
	if ((before == 0) && (d->balance != 0))
		unbalanced++;
	else if ((before != 0) && (d->balance == 0))
		unbalanced--;
}

static void
debts_reset(void) {
	free(debts);
	debts = NULL;
	debts_cap = debts_used = unbalanced = 0;
	debt_initial = debt_created = debt_moved = debt_eot = 0;
}

/* a message that the frontend receives from the backend */
static void
frontend_account(struct messaggio *m) {
	switch (m->tpy) {
		case EOT_TAG:
			debt_add(m->vsource.creator, (long)m->vsource.source, m->side, -1);
			debt_eot++;
			break;
		case ADD_TAG:
		case EOTADD_TAG:
			if (m->vtarget.sign & CH_CREATED) {
				if (m->vtarget.sign & CH_OTHER_BACKEND) {
					debt_add(m->vsource.creator, (long)m->vsource.source, !m->vsource.side, +1);
					debt_created++;
				}
			} else {
				/* an optimization of one: the edge PLUS of the source side is now here */
				debt_add(m->vsource.creator, (long)m->vsource.source, m->vsource.side, -1);
				debt_moved++;
			}
			if (m->tpy == EOTADD_TAG) {
				debt_add(m->vtarget.creator, (long)m->vtarget.source, m->side, -1);
				debt_eot++;
			}
			break;
	}
}

/* ------------------------------------------------------------------------------------------- */
/* routing */

int
ChannelRoute(struct messaggio *m, int dest) {
	m->sender = rank;
	if (pelcr_role == PELCR_FRONTEND) {
		if (dest == rank)
			return dest;
		/* an edge from a node of the frontend held by the backend: its side waits for the EOT */
		if (((m->tpy == ADD_TAG) || (m->tpy == EOTADD_TAG)) && (m->vsource.rankpuit == rank)) {
			debt_add(m->vsource.creator, (long)m->vsource.source, m->vsource.side, +1);
			debt_initial++;
		}
		ch_write_record(m);
		return -1;
	}
	if ((pelcr_role == PELCR_BACKEND) && (dest >= size)) {
		if (rank != CH_GATEWAY)
			return CH_GATEWAY;
		ch_write_record(m);
		return -1;
	}
	return dest;
}

int
ChannelForward(struct messaggio *m) {
	if ((pelcr_role != PELCR_BACKEND) || (rank != CH_GATEWAY) || (m->vtarget.rankpuit < size))
		return 0;
	ch_write_record(m);
	return 1;
}

/* ------------------------------------------------------------------------------------------- */
/* the backend */

static void
serve_listen(int port) {
	struct sockaddr_in a;
	int one = 1;

	ch_listen = socket(AF_INET, SOCK_STREAM, 0);
	if (ch_listen < 0) {
		perror("PELCR backend: socket");
		exit(-1);
	}
	setsockopt(ch_listen, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	memset(&a, 0, sizeof(a));
	a.sin_family = AF_INET;
	a.sin_addr.s_addr = htonl(INADDR_ANY);
	a.sin_port = htons((unsigned short)port);
	if ((bind(ch_listen, (struct sockaddr *)&a, sizeof(a)) < 0) || (listen(ch_listen, 8) < 0)) {
		perror("PELCR backend: bind/listen");
		exit(-1);
	}
	fcntl(ch_listen, F_SETFL, fcntl(ch_listen, F_GETFL, 0) | O_NONBLOCK);
	printf("(%d) backend of %d ranks: waiting for frontends on port %d\n", rank, size, port);
	fflush(stdout);
}

/* the counters of a term on this rank, then zero */
static void
serve_counters(long *c) {
	c[0] = edge_compositions;
	c[1] = fires;
	c[2] = one_optimizations;
	c[3] = family_reductions;
	c[4] = hot_nodes;
	c[5] = processed_actions;
	edge_compositions = fires = one_optimizations = family_reductions = processed_actions = 0;
	/* the keys of the graph builder are addresses of the frontend: the next term reuses them */
	table_idestroy(BookTable[size]);
	table_init(BookTable[size]);
}

#define SERVE_COUNTERS 6

static void
serve_term_end(void) {
	long total[SERVE_COUNTERS], c[SERVE_COUNTERS];
	int r, k;
	MPI_Status st;

	serve_counters(total);
	for (r = 0; r < size; r++)
		if (r != CH_GATEWAY) {
			int ask = 1;
			MPI_Send(&ask, 1, MPI_INT, r, CHANNEL_STATS_TAG, MPI_COMM_WORLD);
			MPI_Recv(c, SERVE_COUNTERS, MPI_LONG, r, CHANNEL_STATS_TAG, MPI_COMM_WORLD, &st);
			for (k = 0; k < SERVE_COUNTERS; k++)
				total[k] += c[k];
		}
	ch_put_block(CH_STATS, SERVE_COUNTERS, total, (long)sizeof(total));
	printf("(%d) term over: %ld compositions, %ld family reductions, %ld hot nodes left\n", rank, total[0],
	       total[3], total[4]);
	fflush(stdout);
}

static void
serve_close(const char *why) {
	printf("(%d) channel closed (%s): %ld records in, %ld out\n", rank, why, ch_records_in, ch_records_out);
	fflush(stdout);
	close(ch_fd);
	ch_fd = -1;
	in_len = out_len = out_sent = 0;
	blk_open = 0;
	ch_records_in = ch_records_out = 0;
}

static void
serve_blocks(void) {
	size_t pos = 0;

	while (in_len - pos >= sizeof(ch_header)) {
		ch_header h;
		memcpy(&h, in_buf + pos, sizeof(h));
		if (in_len - pos - sizeof(h) < (size_t)h.bytes)
			break;
		pos += sizeof(h);
		if (h.type == CH_ACTIONS) {
			char *p = in_buf + pos;
			int i;
			for (i = 0; i < h.count; i++) {
				struct messaggio m;
				p += MessageUnpack(&m, p);
				ch_records_in++;
				PushMessage(&m);
			}
		} else if (h.type == CH_TERM_END)
			serve_term_end();
		else if (h.type == CH_BYE) {
			serve_close("bye");
			return;
		}
		pos += (size_t)h.bytes;
	}
	memmove(in_buf, in_buf + pos, in_len - pos);
	in_len -= pos;
}

static void
serve_poll(void) {
	if (ch_fd < 0) {
		int fd = accept(ch_listen, NULL, NULL);
		int n = size;
		if (fd < 0)
			return;
		ch_fd = fd;
		ch_nonblocking(ch_fd);
		ch_put_block(CH_HELLO, n, NULL, 0);
		printf("(%d) channel open\n", rank);
		fflush(stdout);
	}
	for (;;) {
		ssize_t r;
		ch_reserve(&in_buf, &in_cap, in_len + 65536);
		r = recv(ch_fd, in_buf + in_len, in_cap - in_len, 0);
		if (r > 0) {
			in_len += (size_t)r;
			continue;
		}
		if ((r < 0) && ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)))
			break;
		serve_blocks();
		if (ch_fd >= 0)
			serve_close("the frontend went away");
		return;
	}
	serve_blocks();
}

void
ServeLoop(int port) {
	principal_rank = -1; /* the root of a term is in its frontend */
	OpenFileInitStruct();
	if (rank == CH_GATEWAY)
		serve_listen(port);
	for (;;) {
		if (rank == CH_GATEWAY)
			serve_poll();
		else {
			int flag = 0;
			MPI_Status st;
			MPI_Iprobe(CH_GATEWAY, CHANNEL_STATS_TAG, MPI_COMM_WORLD, &flag, &st);
			if (flag) {
				int ask;
				long c[SERVE_COUNTERS];
				MPI_Recv(&ask, 1, MPI_INT, CH_GATEWAY, CHANNEL_STATS_TAG, MPI_COMM_WORLD, &st);
				serve_counters(c);
				MPI_Send(c, SERVE_COUNTERS, MPI_LONG, CH_GATEWAY, CHANNEL_STATS_TAG, MPI_COMM_WORLD);
			}
		}
		FunReceiveMessages();
		FunInteraction();
		if (rank == CH_GATEWAY)
			ch_flush(0);
		if ((pending_actions == 0) && (local_pending == 0))
			usleep(20); /* nothing to do: do not burn the core */
	}
}

/* ------------------------------------------------------------------------------------------- */
/* the frontend */

static void
frontend_connect(const char *address) {
	char host[256];
	const char *colon = strrchr(address, ':');
	struct addrinfo hints, *res, *p;

	if (colon == NULL) {
		fprintf(stderr, "PELCR frontend: the backend is host:port, not %s\n", address);
		exit(-1);
	}
	snprintf(host, sizeof(host), "%.*s", (int)(colon - address), address);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(host, colon + 1, &hints, &res) != 0) {
		fprintf(stderr, "PELCR frontend: unknown backend %s\n", address);
		exit(-1);
	}
	for (p = res; p != NULL; p = p->ai_next) {
		ch_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (ch_fd < 0)
			continue;
		if (connect(ch_fd, p->ai_addr, p->ai_addrlen) == 0)
			break;
		close(ch_fd);
		ch_fd = -1;
	}
	freeaddrinfo(res);
	if (ch_fd < 0) {
		fprintf(stderr, "PELCR frontend: no backend at %s\n", address);
		exit(-1);
	}
	ch_nonblocking(ch_fd);
}

static long backend_counters[SERVE_COUNTERS];
static int backend_counters_in = 0;

/* one block from the backend */
static void
frontend_read_block(void) {
	static char *payload = NULL;
	static size_t payload_cap = 0;
	ch_header h;

	if (!ch_read_all(&h, sizeof(h))) {
		fprintf(stderr, "(%d) PELCR frontend: the backend closed the channel\n", rank);
		exit(-1);
	}
	ch_reserve(&payload, &payload_cap, (size_t)h.bytes + 1);
	if ((h.bytes > 0) && !ch_read_all(payload, (size_t)h.bytes)) {
		fprintf(stderr, "(%d) PELCR frontend: the backend closed the channel\n", rank);
		exit(-1);
	}
	if (h.type == CH_ACTIONS) {
		char *p = payload;
		int i;
		for (i = 0; i < h.count; i++) {
			struct messaggio m;
			p += MessageUnpack(&m, p);
			ch_records_in++;
			frontend_account(&m);
			PushIncomingMessage(0, &m);
		}
	} else if (h.type == CH_STATS) {
		memcpy(backend_counters, payload, sizeof(backend_counters));
		backend_counters_in = 1;
	}
}

static double
now_seconds(void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double)tv.tv_sec + 1e-6 * (double)tv.tv_usec;
}

/* the evaluation of the term just parsed (its actions are in the buffers) */
static void
frontend_term(void) {
	double t0 = now_seconds();
	long blocks = 0;

	ch_flush(1);
	buf_flush();
	for (;;) {
		while (pending_actions > 0) {
			FunInteraction();
			buf_flush();
		}
		ch_flush(1);
		if (unbalanced == 0)
			break;
		frontend_read_block();
		blocks++;
	}
	ch_put_block(CH_TERM_END, 0, NULL, 0);
	ch_flush(1);
	backend_counters_in = 0;
	while (!backend_counters_in)
		frontend_read_block();

	printf("(%d) finished - collecting results\n", rank);
	printf("(%d) elapsed time        :  %.3f s\n", rank, now_seconds() - t0);
	printf("(%d) final nodes         :  %d\n", rank, graph_nodes);
	printf("(%d) cold nodes          :  %ld\n", rank, cold_nodes);
	printf("(%d) edge compositions   :  %ld (backend)\n", rank, backend_counters[0]);
	printf("(%d) fires               :  %ld (backend)\n", rank, backend_counters[1]);
	printf("(%d) trivial             :  %ld optimized (backend)\n", rank, backend_counters[2]);
	printf("(%d) family reductions   :  %ld (backend)\n", rank, backend_counters[3]);
	printf("(%d) hot nodes left      :  %ld (backend)\n", rank, backend_counters[4]);
	printf("(%d) channel             :  %ld records sent, %ld received in %ld blocks\n", rank, ch_records_out,
	       ch_records_in, blocks);
	printf("(%d) debts of EOT        :  %ld initial, %ld from created nodes, %ld moved here by one, %ld EOT "
	       "received\n",
	       rank, debt_initial, debt_created, debt_moved, debt_eot);
	fflush(stdout);
	read_back(principal);
	debts_reset();
	ch_records_in = ch_records_out = 0;
}

void
FrontendMain(const char *address) {
	int parse_status = 0;
	ch_header h;

	frontend_connect(address);
	if (!ch_read_all(&h, sizeof(h)) || (h.type != CH_HELLO)) {
		fprintf(stderr, "PELCR frontend: %s is not a PELCR backend\n", address);
		exit(-1);
	}
	size = h.count;
	rank = FRONTEND_RANK;
	principal_rank = rank;
	printf("(%d) frontend of a backend of %d ranks at %s\n", rank, size, address);
	fflush(stdout);

	while (parse_status != MAXNUMCOST + 1) {
		OpenFileInitStruct();
		principal = NULL;
		printf("(%d) PELCR 10.0> ", rank);
		fflush(stdout);
		parse_status = yyparse();
		if (parse_status != MAXNUMCOST + 1)
			frontend_term();
	}
	ch_put_block(CH_BYE, 0, NULL, 0);
	ch_flush(1);
	close(ch_fd);
	ch_fd = -1;
}
