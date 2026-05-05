/* Manager of global address space of memory for graph distributed allocation

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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef SOLARIS
#include <sys/systeminfo.h>
#endif
#include <mpi.h>
#include <sys/times.h>
#include <sys/types.h>
#ifdef MPE_GRAPH
#include <mpe.h>
#endif
#include <time.h>

#include <string.h>
//
#include "h/var.h"
#define INIT_TABLE_SIZE 239

#if FREE_HOT_BOOKTABLE_ENTRIES
static void
RememberBookedAddress(node *address, int rk, unsigned long ord) {
	if (address == NULL)
		return;

	address->has_booktable_entry = 1;
	address->booktable_rank = rk;
	address->booktable_key = ord;
}
#endif

void *
safemalloc(int length) {
	char *ptr;

	if (length <= 0)
		length = 1;

	ptr = (char *)malloc((size_t)length);
	if (ptr == (char *)0) {
		fprintf(stderr, "malloc of %d bytes failed. Exiting\n", length);
		exit(1);
	}
	return ptr;
}

HashTable *
table_init(HashTable *table) {
	table->elements = 0;
	table->size = INIT_TABLE_SIZE;
	table->table = safemalloc(INIT_TABLE_SIZE * sizeof(HashEntry *));
	table->last = NULL;
	memset(table->table, 0, INIT_TABLE_SIZE * sizeof(HashEntry *));

	return table;
}

unsigned long
hash_ikey(unsigned long key) {
	unsigned long h = 0, g;
	int i;
	char skey[sizeof(unsigned long)];

	memcpy(skey, &key, sizeof(unsigned long));
	for (i = 0; i < sizeof(unsigned long); i++) {
		h = (h << 4) + skey[i];
		if ((g = h & 0xf0000000))
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}

int
table_iget(HashTable *table, unsigned long key, node **address)
/* table_iget(table,key,address) */
/*      HashTable *table; */
/*      unsigned long key; */
/*      node**address; */
{
	unsigned long hkey;
	HashEntry *entry;

	hkey = hash_ikey(key) % table->size;
	entry = table->table[hkey];
	while (entry != NULL) {
		if (entry->key == key) {
			*address = entry->address;
			return 1;
		}
		entry = entry->next;
	}
	return 0;
}

void
rebuild_itable(HashTable *table)
/* rebuild_itable(table) */
/* HashTable*table; */
{
	HashTable newtable;
	HashEntry *entry;
	int i;

	newtable.last = NULL;
	newtable.elements = 0;
	newtable.size = table->size * 2;
	newtable.table = safemalloc(newtable.size * sizeof(HashEntry *));
	memset(newtable.table, 0, newtable.size * sizeof(HashEntry *));
	for (i = 0; i < table->size; i++) {
		entry = table->table[i];
		while (entry) {
			table_iput(&newtable, entry->key, entry->address);
			entry = entry->next;
		}
	}
	table_idestroy(table);
	table->elements = newtable.elements;
	table->size = newtable.size;
	table->table = newtable.table;
}

void
table_iput(HashTable *table, unsigned long key, node *address)
/* table_iput(table,key,address) */
/* HashTable*table; */
/* unsigned long key; */
/* node*address; */
{
	unsigned long hkey;
	HashEntry *nentry;

	nentry = safemalloc(sizeof(HashEntry));
	nentry->key = key;
	nentry->address = address;
	hkey = hash_ikey(key) % table->size;

	if (table->table[hkey] != NULL) {
		nentry->next = table->table[hkey];
		table->table[hkey] = nentry;
	} else {
		nentry->next = NULL;
		table->table[hkey] = nentry;
	}
	table->elements++;

	nentry->nptr = NULL;
	nentry->pptr = table->last;
	if (table->last)
		table->last->nptr = nentry;
	table->last = nentry;

	if (table->elements > (table->size * 3) / 2) {
#ifdef _DEBUG
		DEBUG_HASH printf("rebuilding hash table...\n");
#endif
		rebuild_itable(table);
	}
}
HashEntry *
delete_fromilist(HashTable *table, HashEntry *entry, unsigned long key, node **address, int *deleted)

/* HashEntry* */
/* delete_fromilist(table,entry,key) */
/* HashTable*table; */
/* HashEntry*entry; */
/* unsigned long key; */
{
	HashEntry *next;

	if (entry == NULL)
		return NULL;
	if (entry->key == key) {
		if (address != NULL)
			*address = entry->address;
		if (deleted != NULL)
			*deleted = 1;
		if (table->last == entry)
			table->last = entry->pptr;
		if (entry->nptr)
			entry->nptr->pptr = entry->pptr;
		if (entry->pptr)
			entry->pptr->nptr = entry->nptr;
		next = entry->next;
		free(entry);
		return next;
	}
	entry->next = delete_fromilist(table, entry->next, key, address, deleted);
	return entry;
}
void
table_idelete(HashTable *table, unsigned long key, node **address)

/* void */
/* table_idelete(table,key) */
/* HashTable*table; */
/* unsigned long key; */
{
	unsigned long hkey;
	int deleted = 0;

	hkey = hash_ikey(key) % table->size;
	table->table[hkey] = delete_fromilist(table, table->table[hkey], key, address, &deleted);
	if (deleted)
		table->elements--;
}

void
table_idestroy(HashTable *table) {
	HashEntry *entry, *previous;

	entry = table->last;
	while (entry != NULL) {
		previous = entry->pptr;
		free(entry);
		entry = previous;
	}
	free(table->table);
	table->elements = 0;
	table->size = 0;
	table->table = NULL;
	table->last = NULL;
}

#if FREE_HOT_BOOKTABLE_ENTRIES
void
ReleaseBookedAddress(node *address) {
	node *removed_address = NULL;

	if (address == NULL || !address->has_booktable_entry || address->sto == OUT)
		return;

	table_idelete(BookTable[address->booktable_rank], address->booktable_key, &removed_address);
	address->has_booktable_entry = 0;
	address->booktable_rank = -1;
	address->booktable_key = 0;
}
#endif

node *
BookedAddress(int rk, long ord) {
	HashTable *p;
	node *address;

#ifdef _DEBUG
	TRACING fprintf(logfile, "(%d) ROOT[%p] hash_table(%d) key:%ld(%p) ", rank, (void *)BookTable[rk], rk, ord,
	                (void *)ord);
#endif
	if (rk == rank) {
#ifdef _DEBUG
		TRACING fprintf(logfile, "\n");
#endif
		return (node *)ord;
	} else {
		p = BookTable[rk];
		if (table_iget(p, ord, &address) == 0) {
#ifdef _DEBUG
			TRACING fprintf(logfile, "it is not here\n");
#endif
			G.hot = CreateNewNode(G.hot);
			G.hot->sto = IN;
			table_iput(p, ord, G.hot);
#if FREE_HOT_BOOKTABLE_ENTRIES
			RememberBookedAddress(G.hot, rk, (unsigned long)ord);
#endif
			G.hot->printed = !pflag;
			address = G.hot;
		};
#ifdef _DEBUG
		TRACING fprintf(logfile, "\n");
#endif
		return address;
	};
}

node *
StoreBookedAddress(int rk, long ord, int sto) {
	HashTable *p;
	node *address;

	/*
	  DEBUG{
	  fprintf(logfile,"(%d) ROOT[%p] hash_table(%d) key:%d(%p) ",rank,BookTable[rk],rk,ord,(node*)ord);
	  fprintf(logfile," STO =%d",sto);
	  };
	  */

	if (rk == rank) {
		address = (node *)ord;
		if (sto == OUT)
			AttachNodeToCold(address);
		/* DEBUG fprintf(logfile,"\n");*/
		return address;
	} else {
		p = BookTable[rk];
		if (table_iget(p, ord, &address) == 0) {
			/*
			  DEBUG fprintf(logfile,"it is not here\n");
			  */
			if (sto == IN) {
				G.hot = CreateNewNode(G.hot);
				G.hot->sto = IN;
				table_iput(p, ord, G.hot);
#if FREE_HOT_BOOKTABLE_ENTRIES
				RememberBookedAddress(G.hot, rk, (unsigned long)ord);
#endif
				G.hot->printed = !pflag;
				address = G.hot;
			} else {
				G.cold = CreateNewNode(G.cold);
				G.cold->sto = OUT;
				table_iput(p, ord, G.cold);
#if FREE_HOT_BOOKTABLE_ENTRIES
				RememberBookedAddress(G.cold, rk, (unsigned long)ord);
#endif
				G.cold->printed = !pflag;
				address = G.cold;
			}
		} else if (sto == OUT) {
			AttachNodeToCold(address);
		};

		/*    DEBUG fprintf(logfile,"\n");*/
		return address;
	};
}
