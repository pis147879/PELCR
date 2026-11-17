/* Messages with weights of any length (see h/message.h)

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "var.h"

long message_records = 0;
long message_record_bytes = 0;
long message_long_weights = 0;

/* a copy of the weight with header h whose bytes are at src (in a record: not aligned) */
static weight *
message_alloc(const weight *h, const void *src) {
	weight *p = (weight *)malloc(W_BYTES(h));
	if (p == NULL) {
		fprintf(stderr, "(%d) PELCR: no memory for the weight of a message (%d letters)\n", rank, W_NLETTERS(h));
		exit(-1);
	}
	memcpy(p, src, W_BYTES(h));
	message_long_weights++;
	return p;
}

/* the weight of m becomes w (w may be the weight of m) */
void
MessageSetWeight(struct messaggio *m, const weight *w) {
	weight *old = m->wlong;

	if (W_NLETTERS(w) <= MSGLETTERS) {
		WeightCopy(&m->weight.h, w);
		m->wlong = NULL;
	} else if (w != old)
		m->wlong = message_alloc(w, w);
	if (old != NULL && old != m->wlong)
		free(old);
}

void
MessageFreeWeight(struct messaggio *m) {
	if (m->wlong != NULL) {
		free(m->wlong);
		m->wlong = NULL;
	}
}

/* the side effects travel only with the weights that have an external letter (PopMessage) */
static int
message_has_fun(const weight *w) {
	return (WeightFindX(w, FUNC) >= 0) || (WeightFindX(w, KONST) >= 0);
}

/* the record of m in out (see h/message.h); m no longer owns its weight */
int
MessagePack(char *out, struct messaggio *m) {
	const weight *w = MessageWeight(m);
	int fun = message_has_fun(w);
	int len = (int)(sizeof(int) + MESSAGE_FIXED + W_BYTES(w) + (fun ? MESSAGE_FUN : 0));
	char *p = out;

	memcpy(p, &len, sizeof(int));
	p += sizeof(int);
	memcpy(p, m, MESSAGE_FIXED);
	p += MESSAGE_FIXED;
	memcpy(p, w, W_BYTES(w));
	p += W_BYTES(w);
	if (fun)
		memcpy(p, &m->funWhich, MESSAGE_FUN);
	MessageFreeWeight(m);
	message_records++;
	message_record_bytes += len;
	return len;
}

/* the message of the record in, and the length of the record */
int
MessageUnpack(struct messaggio *m, const char *in) {
	const char *p = in;
	weight h;
	int len;

	memcpy(&len, p, sizeof(int));
	p += sizeof(int);
	memcpy(m, p, MESSAGE_FIXED);
	p += MESSAGE_FIXED;
	memcpy(&h, p, sizeof(weight));
	if (W_NLETTERS(&h) <= MSGLETTERS) {
		memcpy(&m->weight, p, W_BYTES(&h));
		m->wlong = NULL;
	} else
		m->wlong = message_alloc(&h, p);
	p += W_BYTES(&h);
	if (p - in < len)
		memcpy(&m->funWhich, p, MESSAGE_FUN);
	else
		memset(&m->funWhich, 0, MESSAGE_FUN);
	return len;
}

void
PrintMessageStats(void) {
	printf("(%d) messages to others   :  %ld records, %ld bytes (%.1f per record, a message in the queues %lu)\n",
	       rank, message_records, message_record_bytes,
	       message_records ? (double)message_record_bytes / (double)message_records : 0.0,
	       (unsigned long)sizeof(struct messaggio));
	printf("(%d) long message weights :  %ld (more than MSGLETTERS = %d letters)\n", rank, message_long_weights,
	       MSGLETTERS);
}
