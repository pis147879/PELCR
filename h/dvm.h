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

