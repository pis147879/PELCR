/* Messages with weights of any length

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

/* In the queues of a process a message (h/combustion.h) keeps at most MSGLETTERS letters
   of its weight; a longer weight is allocated, and the message owns it until it is sent to
   another process (MessagePack), passed to a queue (PushMessage, PushIncomingMessage,
   PopMessage: the copy owns it) or processed (MessageFreeWeight).

   Between processes the messages are aggregated in a buffer (PushMessage, distribution.c):
   the number of messages (int) and then one record per message, as long as its weight:

       [length of the record (int)] [the message up to the side effects]
       [the weight: header and letters] [the side effects, only if the weight has an
       external letter X(FUNC,.) or X(KONST,.)]

   The aggregation window closes after aggregationWindow messages, or earlier when the
   buffer has no room for a record with MAXLETTERS letters. */

#ifndef message_h
#define message_h

/* the weight of a message */
#define MessageWeight(m) ((m)->wlong != NULL ? (m)->wlong : &(m)->weight.h)

#define MESSAGE_FIXED offsetof(struct messaggio, funWhich)
#define MESSAGE_FUN (offsetof(struct messaggio, wlong) - offsetof(struct messaggio, funWhich))
#define MESSAGE_MAXRECORD (sizeof(int) + MESSAGE_FIXED + sizeof(weight) + MAXLETTERS * sizeof(letter) + MESSAGE_FUN)

/* an aggregation buffer: the number of messages and their records (or, for the messages to
   the process itself, the messages: buf_flush) */
#define MESSAGE_BUFFER (sizeof(long) + (1 + MAXAWIN) * sizeof(struct messaggio))

_Static_assert(MESSAGE_BUFFER >= sizeof(long) + MESSAGE_MAXRECORD, "MESSAGE_BUFFER: room for the longest record");

extern long message_records;      /* records sent to other processes */
extern long message_record_bytes; /* their bytes */
extern long message_long_weights; /* weights longer than MSGLETTERS letters, allocated */

void MessageSetWeight(struct messaggio *m, const weight *w);
void MessageFreeWeight(struct messaggio *m);
int MessagePack(char *out, struct messaggio *m);
int MessageUnpack(struct messaggio *m, const char *in);
void PrintMessageStats(void);

#endif
