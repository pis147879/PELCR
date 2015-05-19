#ifndef LAMBDASTAR_H
#define LAMBDASTAR_H



#define  TYPE 1
#define  KONST 2
#define  FUNC 3
#define  SYNC 4
#define  ARG 5
#define  ITE 6
#define  ARGLAST 7

#define  BOOL 1
#define  NAT 2

#define  TIN 0
#define  TOUT 1

/* ANTO */
#define EXT  0
/* ANTO */
#define SUCC 1
#define ADD 2
#define PRED 3
#define MULT 4
#define AND 5
#define NOT 6
#define ISZERO 7
#define OR 8
#define GREATER 9


#define MAXNUMCOST 10000 /*era 1000000*/
#define MAXTYPE 20  /*era 10*/
#define MAXFUNCTIONS 50  /*era 50*/
#define MAXDATAVARS 20  /*era 10*/

typedef struct functionstack 
{
  int which;
  int type;
  int narg;
  long wait;
  USERTYPE (*fun)();
  USERTYPE s[MAXNUMARG];
/* ANTO */
  char name[MAXNAMELEN];
  int fun_id;
/* ANTO */
} functionstack ;

/* ANTO */
USERTYPE k[MAXNUMCOST][3];
/* ANTO */

functionstack f[MAXFUNCTIONS];
functionstack f_db[MAXFUNCTIONS];
int  t[5][2];
#endif
 
