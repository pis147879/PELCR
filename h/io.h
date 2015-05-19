#ifndef io_h
#define io_h

void OpenFileInitStruct();/**/

void Print(graph g,struct mbuffer *b, long int sq);
int SinkList(
	     struct messaggio*m,
	     struct messaggio*l,
	     int length);
void read_back( node*);
void SetOutputFile(char *name);
void SetDirectory(char *name) ;
void WriteStatus(void);
void WriteHelp(void);
#endif

