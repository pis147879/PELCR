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

#ifndef vm_h
#define vm_h


typedef struct HashEntry
{
struct HashEntry*nptr,*pptr;
unsigned long key;
node*address;
struct HashEntry*next;
}HashEntry;

typedef struct HashTable{
int elements;
int size;
HashEntry**table;
HashEntry*last;
}HashTable;

HashTable*table_init(HashTable*table);

unsigned long hash_ikey(unsigned long key);
void rebuild_itable(HashTable*table);
HashEntry*delete_fromilist(HashTable*table,HashEntry*entry,unsigned long key);

void table_iput(HashTable*table,unsigned long key,node*address);
int table_iget (HashTable*table,unsigned long key,node**address);

void table_idelete(HashTable*table,unsigned long key,node**address);
void table_idestroy(HashTable*table);
node*BookedAddress(int rk,long ord);
 node*StoreBookedAddress(int rk,long ord,int sto);
void*safemalloc(int length);

#endif

