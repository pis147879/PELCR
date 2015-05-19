#ifndef symbolic_h
#define symbolic_h

typedef char term;


/* extern int k[MAXNUMCOST][2]; */
/* extern functionstack f[MAXFUNCTIONS]; */
/* extern int  t[5][2]; */

extern int kindex;
extern int newval;
extern int findex;
extern int fcounter;

/* extern double (*funcs[MAXFUNCTIONS])(double); */
/* extern void *vars[MAXDATAVARS]; */

void checkone(char*s);
int findType(int tipo);
int product(char*a,char*b,char*a1,char*b1);
int isone(char*a);

#endif

