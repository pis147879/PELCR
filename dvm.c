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

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef SOLARIS
#include<sys/systeminfo.h>
#endif
#include <sys/types.h>
#include <sys/times.h>
#include <mpi.h>
#ifdef MPE_GRAPH
#include<mpe.h>
#endif
#include <time.h>

#include <string.h>
//
//#include "h/mydefs.h"
//#include "h/symbolic.h"
//#include "h/graph.h"
//#include "h/combustion.h"
//#include "h/io.h"
//#include "h/buildgraph.h"
//#include "h/dvm.h"
#include "h/var.h"
#define INIT_TABLE_SIZE 239

void*safemalloc(length)
int length;
{
  char*ptr;

  if(length<=0)
    length= 1;

  ptr= (char*)malloc((size_t)length);
  if(ptr==(char*)0)
    {
      fprintf(stderr,"malloc of %d bytes failed. Exiting\n",length);
      exit(1);
    }
  return ptr;
}

HashTable*table_init(HashTable *table)
{
  table->elements= 0;
  table->size= INIT_TABLE_SIZE;
  table->table= safemalloc(INIT_TABLE_SIZE*sizeof(HashEntry*));
  table->last= NULL;
  memset(table->table,0,INIT_TABLE_SIZE*sizeof(HashEntry*));

  return table;
}

unsigned long
hash_ikey(key)
unsigned long key;
{
  unsigned long h= 0,g;
  int i;
  char skey[sizeof(unsigned long)];

  memcpy(skey,&key,sizeof(unsigned long));
  for(i= 0;i<sizeof(unsigned long);i++){
    h= (h<<4)+skey[i];
    if((g= h&0xf0000000))
      h^= g>>24;
    h&= ~g;
  }
  return h;
}

int
table_iget (HashTable*table,unsigned long key,node**address)
/* table_iget(table,key,address) */
/*      HashTable *table; */
/*      unsigned long key; */
/*      node**address; */
{
  unsigned long hkey;
  HashEntry*entry;
  
  hkey= hash_ikey(key)%table->size;
  entry= table->table[hkey];
  while(entry!=NULL){
    if(entry->key==key){
      *address= entry->address;
      return 1;
    }
    entry= entry->next;
  }
  return 0;
}

void rebuild_itable(HashTable*table)
/* rebuild_itable(table) */
/* HashTable*table; */
{
  HashTable newtable;
  HashEntry*entry;
  int i;

  extern void table_iput();
  extern void table_idestroy();

  newtable.last= NULL;
  newtable.elements= 0;
  newtable.size= table->size*2;
  newtable.table= safemalloc(newtable.size*sizeof(HashEntry*));
  memset(newtable.table,0,newtable.size*sizeof(HashEntry*));
  for(i= 0;i<table->size;i++){
    entry= table->table[i];
    while(entry){
      table_iput(&newtable,entry->key,entry->address);
      entry= entry->next;
    }
  }
  table_idestroy(table);
  table->elements= newtable.elements;
  table->size= newtable.size;
  table->table= newtable.table;
}

void table_iput(HashTable*table,unsigned long key,node*address)
/* table_iput(table,key,address) */
/* HashTable*table; */
/* unsigned long key; */
/* node*address; */
{
  unsigned long hkey;
  HashEntry*nentry;

  nentry= safemalloc(sizeof(HashEntry));
  nentry->key= key;
  nentry->address= address;
  hkey= hash_ikey(key)%table->size;

  if(table->table[hkey]!=NULL){
    nentry->next= table->table[hkey];
    table->table[hkey]= nentry;
  }else{
    nentry->next= NULL;
    table->table[hkey]= nentry;
  }
  table->elements++;

  nentry->nptr= NULL;
  nentry->pptr= table->last;
  if(table->last)
    table->last->nptr= nentry;
  table->last= nentry;

  if(table->elements>(table->size*3)/2){
#ifdef _DEBUG 
      DEBUG_HASH printf("rebuilding hash table...\n");
#endif
    rebuild_itable(table);
  }
}
HashEntry*delete_fromilist(HashTable*table,HashEntry*entry,unsigned long key)

/* HashEntry* */
/* delete_fromilist(table,entry,key) */
/* HashTable*table; */
/* HashEntry*entry; */
/* unsigned long key; */
{
  HashEntry*next;

  if(entry==NULL)return NULL;
  if(entry->key==key){
    if(table->last==entry)
      table->last= entry->pptr;
    if(entry->nptr)
      entry->nptr->pptr= entry->pptr;
    if(entry->pptr)
      entry->pptr->nptr= entry->nptr;
    next= entry->next;
    free(entry);
    return next;
  }
  entry->next= delete_fromilist(table,entry->next,key);
  return entry;
}
void table_idelete(HashTable*table,unsigned long key,node**address)

/* void */
/* table_idelete(table,key) */
/* HashTable*table; */
/* unsigned long key; */
{
  unsigned long hkey;

  hkey= hash_ikey(key)%table->size;
  table->table[hkey]= delete_fromilist(table,table->table[hkey],key);
  table->elements--;
}

void
table_idestroy(HashTable *table)
{
  HashEntry*entry,*next;
  int i;

  for(i= 0;i<table->size;i++){
    entry= table->table[i];
    while(entry){
      next= entry->next;
      entry= next;
    }
  }
  free(table->table);
}

node*BookedAddress(rk,ord)
int rk;
long ord;
{
  HashTable*p;
  node*address;

#ifdef _DEBUG
    TRACING fprintf(logfile,"(%d) ROOT[%p] hash_table(%d) key:%ld(%p) ",rank,(void*)BookTable[rk],rk,ord,(void*)ord);
#endif
  if(rk==rank){
#ifdef _DEBUG
      TRACING fprintf(logfile,"\n");
#endif      
      return(node*)ord;
  }
  else
  {
      p= BookTable[rk];
      if(table_iget(p,ord,&address)==0) {
#ifdef _DEBUG
        TRACING fprintf(logfile,"it is not here\n");
#endif
          G.hot = CreateNewNode(G.hot);
          table_iput(p,ord,G.hot);
          G.hot->printed=!pflag;
          address = G.hot;
      };
#ifdef _DEBUG
        TRACING fprintf(logfile,"\n");
#endif
      return address;
  };
}

node*StoreBookedAddress(rk,ord,sto)
int rk;
long ord;
int sto;
{
  HashTable*p;
  node*address;

  /*
    DEBUG{
    fprintf(logfile,"(%d) ROOT[%p] hash_table(%d) key:%d(%p) ",rank,BookTable[rk],rk,ord,(node*)ord);
    fprintf(logfile," STO =%d",sto);
    };
    */

  if(rk==rank)
    {
      /* DEBUG fprintf(logfile,"\n");*/
      return(node*)ord;
    }
  else
    {
      p= BookTable[rk];
      if(table_iget(p,ord,&address)==0)
	{
	  /*
	    DEBUG fprintf(logfile,"it is not here\n");
	    */
	  if(sto==IN)
	    {
	      G.hot= CreateNewNode(G.hot);
	      table_iput(p,ord,G.hot);
	      G.hot->printed=!pflag;
	      address= G.hot;}
	  else
	    {
	      G.cold= CreateNewNode(G.cold);
	      table_iput(p,ord,G.cold);
	      G.cold->printed=!pflag;
	      address= G.cold;}
	}
      else 
	if (sto==OUT) {};

      /*    DEBUG fprintf(logfile,"\n");*/
      return address;
    };
}
