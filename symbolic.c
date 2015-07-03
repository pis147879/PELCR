/* The implementation of Girard's dynamic algebra
 
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
//#include "mpi.h"
#ifdef MPE_GRAPH
#include<mpe.h>
#endif
#include <time.h>

#include <string.h>

//#include "h/mydefs.h"
//#include "h/symbolic.h"
//#include "h/graph.h"
//#include "h/buildgraph.h"
//
//#include "h/combustion.h"
//#include "h/io.h"
//#include "h/dvm.h"
#include "h/var.h"
#include "h/lambdastar.h"


 void checkone(s)
 char*s;
 {
   char*r,sx[MAXLENWEIGHT],*ss;
   long p= 0;
   ss= s;
 #ifdef WDEBUG
//   printf("%s %d\n",s,p);
 #endif
   while(s[0]=='!'){
     p= p+strtol(&s[1],&r,10);
     s= r;
 #ifdef WDEBUG
//     printf("%s %d\n",s,p);
 #endif
   };
   strcpy(sx,s);
   if(p!=0)sprintf(s,"!%ld%s",p,sx);
   sprintf(ss,"%s",s);
 #ifdef WDEBUG
//   printf("%s\n",s);
 #endif
   return;
 }

int isone(w)
 term*w;
 {
   if(w[0]=='\0')return 1;
   if(!strpbrk(w,"wpqX")){return 1;}
   else return 0;
 }

int product(a,b,a1,b1)
 char*a;
 char*b;
 char*a1;
 char*b1;
{
  long l1,l2,i1,i2,p1,p2;
  char*r;
  char*atmp=a,*btmp=b;
  if(atmp[0]=='\0') {
    sprintf(b1,"%s%s",b1,btmp);
#ifdef WDEBUG
//    printf("\n");
//    printf("A1:  %s %s\n",atmp,a1);
//   printf("B1:  %s %s\n",btmp,b1);
#endif
    checkone(a1);checkone(b1);
    return 1;
  }

  /*************************************/

  else if(btmp[0]=='\0') {
    sprintf(a1,"%s%s",a1,atmp);
#ifdef WDEBUG
//    printf("A2 %s %s\n",atmp,a1);
//    printf("B2 %s %s\n",btmp,b1);
#endif
    checkone(a1);checkone(b1);
    return 2;
  }

  /*************************************/

  else if(((atmp[0]=='p')&&(btmp[0]=='p'))||((atmp[0]=='q')&&(btmp[0]=='q'))) {
    atmp++;btmp++;
    if(product(atmp,btmp,a1,b1)) {
#ifdef WDEBUG
//      printf("A3:  %s %s\n",atmp,a1);
//      printf("B3:  %s %s\n",btmp,b1);
#endif
      checkone(a1);checkone(b1);
      locf_counter++;
      return 3;
    }
    else return 0;
  }

  /*************************************/

  else if((atmp[0]=='w')&&(btmp[0]=='w')) {
#ifdef WDEBUG
//    printf("%s\n",&atmp[2]);
//    printf("%s\n",&btmp[2]);
#endif
    i1= strtol(&atmp[2],&r,10);
    atmp= r+1;
    i2= strtol(&btmp[2],&r,10);
    btmp= r+1;
#ifdef WDEBUG
//    printf("branch: %d , %d\n",i1,i2);
#endif
    if(i1==i2) {
#ifdef WDEBUG
//      printf("%s\n",atmp);
//      printf("%s\n",btmp);
#endif
      strtol(atmp,&r,10);
      atmp = r+1;
      strtol(btmp,&r,10);
      btmp = r+1;
#ifdef WDEBUG
//      printf("A4:  %s %s\n",atmp,a1);
//      printf("B4:  %s %s\n",btmp,b1);
#endif
      if(product(atmp,btmp,a1,b1)) {
#ifdef WDEBUG
//	printf("A4b: %s %s\n",atmp,a1);
//	printf("B4b: %s %s\n",btmp,b1);
#endif
	checkone(a1);checkone(b1);
	return 4;
      }
      else return 0;
    }
    else return 0;
  }

  /*******************************************************/

  else if((atmp[0]=='!')&&(btmp[0]=='!')) {
    int propagaInv=0;
    p1= strtol(&atmp[1],&r,10);
    atmp = r;
    p2= strtol(&btmp[1],&r,10);
    btmp = r;
#ifdef WDEBUG
//    printf("\nlift: %d , %d\n",p1,p2);
#endif
    if(p1==p2) {
#ifdef WDEBUG
//      printf("!eq %s\n",atmp);
//      printf("!eq %s\n",btmp);
#endif
      propagaInv=product(atmp,btmp,a1,b1);
      if(propagaInv) {
#ifdef WDEBUG
//	printf("A5:  %s %s\n",atmp,a1);
//	printf("B5:  %s %s\n",btmp,b1);
#endif
	{
	  char ax[100];
	  char bx[100];
	  sprintf(ax,"%s",a1);
	  sprintf(bx,"%s",b1);
	  sprintf(a1,"!%ld%s",p1,ax);
	  sprintf(b1,"!%ld%s",p1,bx);
	}
	checkone(a1);
	checkone(b1);
	if ( (propagaInv==30)||(propagaInv==31) ||(propagaInv==32)||(propagaInv==33)) return propagaInv;
	else return 5;
      }
      else return 0;
    }
    else if(p1<p2) {
      char ax[100];
      char bx[100];
      sprintf(bx,"!%ld%s",p2-p1,btmp);
      if(product(atmp,bx,a1,b1)) {
	sprintf(ax,"%s",a1);
	sprintf(bx,"%s",b1);
	sprintf(a1,"!%ld%s",p1,ax);
	sprintf(b1,"!%ld%s",p1,bx);
	checkone(a1);checkone(b1);
	return 6;
      }
      else return 0;
    }
    else {
      char ax[100];
      char bx[100];
      sprintf(ax,"!%ld%s",p1-p2,atmp);
      if(product(ax,btmp,a1,b1)) {
	sprintf(ax,"%s",a1);
	sprintf(bx,"%s",b1);
	sprintf(a1,"!%ld%s",p2,ax);
	sprintf(b1,"!%ld%s",p2,bx);
	checkone(a1);checkone(b1);
	return 7;
      }
      else return 0;
    }
  }

  /*************************************/

  else if((atmp[0]=='!')&&(btmp[0]=='w')) {
    char ax[100];
    char bx[100];
    long px;
    char *Xstr=NULL;
    int konst_index = 0;    

    p1= strtol(&atmp[1],&r,10);
    atmp = r;
    i2= strtol(&btmp[2],&r,10);
    btmp = r+1;
    l2= strtol(btmp,&r,10);
    btmp = r+1;
#ifdef WDEBUG
//    printf("p1=%d,i2=%d,l2=%d\n",p1,i2,l2);
//    printf("A8:  %s %s\n",atmp,a1);
//    printf("B8:  %s %s\n",btmp,b1);
#endif
    px= p1+l2-1;
    if(px!=0) sprintf(ax,"!%ld%s",px,atmp); 
    else sprintf(ax,"%s",atmp);
    if(product(ax,btmp,a1,b1)) {
      sprintf(bx,"%s",b1);
      sprintf(b1,"w(%ld,%ld)%s",i2,l2,bx);
    }
    else return 0;
    checkone(a1);checkone(b1);
    
    Xstr=strstr(a1,"X(2");

    if (Xstr != NULL) 
    {
      konst_index=atoi(&Xstr[4]);
      k[konst_index][2]++;
    }
    return 8;
  }

  /*************************************/

  else if((atmp[0]=='w')&&(btmp[0]=='!')) {
    char ax[100];
    char bx[100];
    long px;
    char *Xstr=NULL;
    int konst_index = 0;    

    i1= strtol(&atmp[2],&r,10);
    atmp = r+1;
    l1= strtol(atmp,&r,10);
    atmp = r+1;
    p2= strtol(&btmp[1],&r,10);
    btmp = r;
#ifdef WDEBUG
//    printf("i1=%d,l1=%d, p2=%d\n",i1,l1,p2);
//    printf("A9:  %s %s\n",atmp,a1);
//    printf("B9:  %s %s\n",btmp,b1);
#endif
    px= p2+l1-1;
    if(px!=0) sprintf(bx,"!%ld%s",px,btmp);
    else sprintf(bx,"%s",btmp);
    if(product(atmp,bx,a1,b1)) {
      sprintf(ax,"%s",a1);
      sprintf(a1,"w(%ld,%ld)%s",i1,l1,ax);
    }
    else return 0;
    checkone(a1);checkone(b1);
    Xstr=strstr(b1,"X(2");

    if (Xstr != NULL) 
    {
      konst_index=atoi(&Xstr[4]);
      k[konst_index][2]++;
    }
    return 9;
  }

  /*************************************/

  else if(atmp[0]=='1') {
    atmp++;
    if(product(atmp,btmp,a1,b1)) {
      checkone(a1);
      checkone(b1);
      return 10;
    }
    else return 0;
  }

  /*************************************/

  else if(btmp[0]=='1') {
    btmp++;
    if(product(atmp,btmp,a1,b1)) {
      checkone(a1);checkone(b1);
      return 11;
    }
    else  return 0;
  }

  /***************** Algebra esterna ********************/

  else if((atmp[0]=='X')&&(btmp[0]=='X')) {

#ifdef WDEBUG
    printf("\nExtern Algebra --> %s\n",&atmp[2]);
    printf("\nExtern Algebra --> %s\n",&btmp[2]);
#endif
    i1= strtol(&atmp[2],&r,10);
    atmp= r+1;
    i2= strtol(&btmp[2],&r,10);
    btmp= r+1;
#ifdef WDEBUG
    printf("Extern Algebra --> branch: %d , %d\n",i1,i2);
#endif
    l1 = strtol(atmp,&r,10);
    atmp = r+1;
    l2 = strtol(btmp,&r,10);
    btmp = r+1;
#ifdef WDEBUG
    printf("Extern Algebra -->  %s %s\n",atmp,a1);
    printf("Extern Algebra -->  %s %s\n",btmp,b1);
#endif

  /*************************************/

    if ( (i1==TYPE)&&(i2==ARG) ) {     
#ifdef WDEBUG
      printf("\t%d) TYPE ARG\n",rank);
#endif
      if (product(atmp,btmp,a1,b1)) {
	char ax[100];  
	if(t[l1][1]==TIN) sprintf(ax,"X(%d,%ld)%s",SYNC,l2-1,a1);
	else sprintf(ax,"X(%d,%ld)%s",SYNC,l2,a1);
	
	sprintf(a1,"%s",ax);
	checkone(a1);
	checkone(b1);
	return 12;
      }
      else  return 0;
    }

  /*************************************/

    else if ( (i1==TYPE)&&(i2==ARGLAST) ) {     
#ifdef WDEBUG
      printf("\t%d) TYPE LASTARG\n",rank);
#endif
      if (product(atmp,btmp,a1,b1)) {
	char ax[100];  
	if(t[l1][1]==TIN) sprintf(ax,"X(%d,%ld)%s",SYNC,l2-1,a1);
	else sprintf(ax,"X(%d,%ld)%s",SYNC,l2,a1);
	
	sprintf(a1,"%s",ax);
	checkone(a1);
	checkone(b1);
	if(t[l1][1]==TOUT) return 40+l2%2;
	else return 13;
      }
      else  return 0;
    }

  /*************************************/

    else if ( (i1==ARG)&&(i2==TYPE) ) {
#ifdef WDEBUG
      printf("\t%d) ARG TYPE\n",rank);
#endif
      if (product(atmp,btmp,a1,b1)) {
	char bx[100];  
	if(t[l2][1]==TIN) 
	  sprintf(bx,"X(%d,%ld)%s",SYNC,l1-1,b1);
	else 
          sprintf(bx,"X(%d,%ld)%s",SYNC,l1,b1);
	
	sprintf(b1,"%s",bx);
	checkone(b1);
	checkone(a1);
	return 14;      
      }
      else  return 0;
    }

  /*************************************/

    else if ( (i1==ARGLAST)&&(i2==TYPE) ) {
#ifdef WDEBUG
      printf("\t%d) ARG TYPE\n",rank);
#endif
      if (product(atmp,btmp,a1,b1)) {
	char bx[100];  
	if(t[l2][1]==TIN) 
	  sprintf(bx,"X(%d,%ld)%s",SYNC,l1-1,b1);
	else 
          sprintf(bx,"X(%d,%ld)%s",SYNC,l1,b1);
	
	sprintf(b1,"%s",bx);
	checkone(b1);
	checkone(a1);
	if(t[l2][1]==TOUT) return 42+l1%2;
	else return 15;      
      }
      else  return 0;
    }

    /****************************************/

    else if ( (i1==SYNC)&&(i2==ARG) ) {
#ifdef WDEBUG
      printf("\t%d) SYNC ARG\n",rank);
#endif
      if (product(atmp,btmp,a1,b1)) {
	char ax[100];  
	sprintf(ax,"X(%d,%ld)%s",SYNC,l2,a1);

	sprintf(a1,"%s",ax);
	checkone(a1);
	checkone(b1);
	return 16;
      } 
      else  return 0;
    }

  /*************************************/

    else if ( (i1==ARG)&&(i2==SYNC) ) {
#ifdef WDEBUG
      printf("\t%d) ARG SYNC\n",rank);
#endif
      if (product(atmp,btmp,a1,b1)) {
	char bx[100];  
	sprintf(bx,"X(%d,%ld)%s",SYNC,l1,b1);
 
	sprintf(b1,"%s",bx);
	checkone(a1);
	checkone(b1);
	return 17;
      }
      else  return 0;
    }

    /******************************************/

    else if ( (i1==SYNC)&&(i2==FUNC) ) {
      char bcarlo[MAXLENWEIGHT];
#ifdef WDEBUG
      printf("\t%d) SYNC FUNC\n",rank);
#endif
      /*      if (f[l2].wait==1) {   fare lA COPIA della funzione 

	
	findex++;
	
	f[findex].type=f[l2].type;
	f[findex].narg=f[l2].narg;
	f[findex].wait=f[l2].wait;
	f[findex].fun=f[l2].fun;
	l2=findex;
	
      }
      */
      
      if (l1==0) {	    
	sprintf(bcarlo,"X(%d,%ld)",FUNC,l2);
	strcat(bcarlo,btmp);
	strcpy(btmp,bcarlo);
      }
      if (product(atmp,btmp,a1,b1)) {
	int ft;
	char ax[100];
	char bx[100];

	if (l1==0) {	
	  ft=findType(f[l2].type);
	  sprintf(ax,"X(%d,%d)%s",TYPE,ft,a1);
	  sprintf(a1,"%s",ax);

	  sprintf(bx,"%s",b1);
	  sprintf(b1,"%s",bx);
	  
	}
	else if (l1==f[l2].narg) {
	  ft=findType(f[l2].type);
	  sprintf(ax,"X(%d,%d)%s",TYPE,ft+1,a1);
	  sprintf(a1,"%s",ax);
	
	  sprintf(bx,"X(%d,%ld)%s",FUNC,l2,b1);
	  sprintf(b1,"%s",bx);
	}

	else return 0; 

	checkone(a1);
	checkone(b1);
 	return 18;
      }
      else return 0;
    }

  /*************************************/

    else if ( (i1==FUNC)&&(i2==SYNC) ) {
      char acarlo[MAXLENWEIGHT];
#ifdef WDEBUG
      printf("\t%d) FUNC SYNC\n",rank);
#endif

      /*
      if (f[l1].wait==1) {   fare lA COPIA della funzione 
	
	findex++;
	
	f[findex].type=f[l1].type;
	f[findex].narg=f[l1].narg;
	f[findex].wait=f[l1].wait;
	f[findex].fun=f[l1].fun;
	l1=findex;
	
      }
      */
      
      if (l2==0) {	    
	sprintf(acarlo,"X(%d,%ld)",FUNC,l1);
	strcat(acarlo,atmp);
	strcpy(atmp,acarlo);
      }
      if (product(atmp,btmp,a1,b1)) {
	int ft;
	char ax[100];
	char bx[100];
	
	if (l2==0) {	
	  ft=findType(f[l1].type);

	  sprintf(ax,"%s",a1);
	  sprintf(a1,"%s",ax);
 
	  sprintf(bx,"X(%d,%d)%s",TYPE,ft,b1);
	  sprintf(b1,"%s",bx);
	}
	else if (l2==f[l1].narg) {
	  ft=findType(f[l1].type);
	  sprintf(ax,"X(%d,%ld)%s",FUNC,l1,a1);
	  sprintf(a1,"%s",ax);
	
	  sprintf(bx,"X(%d,%d)%s",TYPE,ft+1,b1);
	  sprintf(b1,"%s",bx);
	}
	else return 0; 
	checkone(a1);
	checkone(b1);
	return 19;
      }
      else return 0;
    }

  /*************************************/

    else if ( (i1==KONST)&&(i2==FUNC) ) {
#ifdef WDEBUG
      printf("%d) CONST-FUNC WAIT %d, NARG %d\n",rank, f[l2].wait,f[l2].narg);
#endif
if ( f[l2].wait == f[l2].narg ) {
	if ( f[l2].narg == 1 ) {     /* valuto la funzione di 1 variabile*/
#ifdef WDEBUG
	  printf("%d) function evaluation (1)\n",rank);
	  printf("%d, %d %lld %lld\n",l1,l2, (f[l2].fun)(5), k[l1][1]);
#endif
        SetNextKIndex();
        k[kindex][2]++;
	  k[kindex][1]=(f[l2].fun)(k[l1][1]);
	}
	else if ( f[l2].narg == 2 ) {	 /* valuto la funzione di 2 variabili */
#ifdef WDEBUG
	  printf("%d) function evaluation (2)\n",rank);
	  printf("%d, %d %lld %lld\n",l1,l2, (f[l2].fun)(45,12), k[l1][1]);
#endif
        SetNextKIndex();
        k[kindex][2]++;
	  k[kindex][1]=(f[l2].fun)((f[l2].s)[0],k[l1][1]);
	}		   
	else if ( f[l2].narg == 3 ) {    /* valuto la funzione di 3 variabili */
#ifdef WDEBUG
	  printf("%d) function evaluation (3)\n",rank);
	  printf("%d, %d %lld %lld\n",l1,l2, (f[l2].fun)(45,12,33), k[l1][1]);
#endif
        SetNextKIndex();
        k[kindex][2]++;
	  k[kindex][1]=(f[l2].fun)((f[l2].s)[0],(f[l2].s)[1],k[l1][1]);
	}
	else  printf("EXCEPTION\n");
	
	newval=1;
	k[kindex][0]=f[l2].type;
	

	/** Avvertire tutti i processi della modifica */
	/*
printf("\n\n\t\t\t1) A-Il processo %d modifica le tabelle  f e  k (e i loro index)\n\n",rank);
	*/

#ifdef WDEBUG
	printf("--R-- %d %lld\n",kindex,k[kindex][1]);
	printf("%d) VALORE CALCOLATO %lld\n",rank,k[kindex][1]);
#endif
	if (product(atmp,btmp,a1,b1)) {
	  char ax[100];
	  char bx[100];
	  sprintf(ax,"X(%d,%d)%s",KONST,kindex,a1);
	  sprintf(bx,"X(%d,%ld)%s",FUNC,l2,b1);
	  sprintf(a1,"%s",ax);
	  sprintf(b1,"%s",bx);
	  checkone(a1);
	  checkone(b1);

	  return 20;
	}
	else return 0;

      }
      else {
	/* valuto parzialmente la funzione */
#ifdef WDEBUG
	printf("%d) PARTIAL EVAL FUNC\n",rank);
#endif
	(f[l2].s)[f[l2].wait-1] = k[l1][1] ;
	f[l2].wait ++ ;

	  /** Avvertire tutti i processi della modifica */
	/*
printf("\n\n\t\t\t2) A-Il processo %d modifica le tabelle  f e  k (e i loro index)\n\n",rank);
	*/

	if (product(atmp,btmp,a1,b1)) {
	  char bx[100];  
	  sprintf(bx,"X(%d,%ld)%s",FUNC,l2,b1);
	  sprintf(b1,"%s",bx);
	  checkone(a1);
	  checkone(b1);
	  return 21;
	}
	else return 0;
      } 


      /** Avvertire tutti i processi della modifica, forse al posto delle 2 precedenti*/
      /*
printf("\n\n\t\t\t3) Il processo %d modifica le tabelle  f e  k (e i loro index)\n\n",rank);
      */
    }  

  /*************************************/

    else if ( (i1==FUNC)&& (i2==KONST) ) {
#ifdef WDEBUG
      printf("%d) FUNC-CONST WAIT %d, NARG %d\n",rank,f[l1].wait,f[l1].narg);
#endif
      if ( f[l1].wait == f[l1].narg ) {
	if ( f[l1].narg == 1 ) {     /* valuto la funzione di 1 variabile*/
#ifdef WDEBUG
	  printf("%d) function evaluation (1)\n",rank);
	  printf("%d, %d %lld %lld\n",l2,l1, (f[0].fun)(45), k[l2][1]);
#endif
        SetNextKIndex();
        k[kindex][2]++;
	  k[kindex][1]=(f[l1].fun)(k[l2][1]);
	}
	else if ( f[l1].narg == 2 ) {	 /* valuto la funzione di 2 variabili */
#ifdef WDEBUG
	  printf("%d) function evaluation (2)\n",rank);
	  printf("%d, %d %lld %lld\n",l2,l1, (f[l1].fun)(45,12), k[l2][1]);
#endif
        SetNextKIndex();
        k[kindex][2]++;
	  k[kindex][1]=(f[l1].fun)((f[l1].s)[0],k[l2][1]);
	}		   
	else if ( f[l1].narg == 3 ) {    /* valuto la funzione di 2 variabili */
#ifdef WDEBUG
	  printf("%d) function evaluation (3)\n",rank);
	  printf("%d, %d %lld %lld\n",l2,l1, (f[l1].fun)(45,12,33), k[l2][1]);
#endif
        SetNextKIndex();
        k[kindex][2]++;
	  k[kindex][1]=(f[l1].fun)((f[l1].s)[0],(f[l1].s)[1],k[l2][1]);
	}

	else  printf("EXCEPTION\n");
	newval=1;
	k[kindex][0]=f[l1].type;

	/** Avvertire tutti i processi della modifica */
	/*
printf("\n\n\t\t\t1) B-Il processo %d modifica le tabelle  f e  k (e i loro index)\n\n",rank);
	*/
	
#ifdef WDEBUG
	printf("--R-- %d %lld\n",kindex,k[kindex][1]);
	printf("%d) VALORE CALCOLATO %lld\n",rank,k[kindex][1]);
#endif
	if (product(atmp,btmp,a1,b1)) {
	  char ax[100];
	  char bx[100];
	  sprintf(bx,"X(%d,%d)%s",KONST,kindex,b1);
	  sprintf(ax,"X(%d,%ld)%s",FUNC,l1,a1);
	  sprintf(a1,"%s",ax);
	  sprintf(b1,"%s",bx);
	  checkone(a1);
	  checkone(b1);

	  return 22;
	}
	else return 0;
      }
      else {
	/* valuto parzialmente la funzione */
#ifdef WDEBUG
	printf("%d) PARTIAL EVAL FUNC\n",rank);
#endif
	(f[l1].s)[f[l1].wait-1] = k[l2][1] ;
	f[l1].wait ++ ;


	/** Avvertire tutti i processi della modifica */
	/*
  printf("\n\n\t\t\t2) B-Il processo %d modifica le tabelle  f e  k (e i loro index)\n\n",rank);
	*/

	if (product(atmp,btmp,a1,b1)) {
	  char ax[100];  
	  sprintf(ax,"X(%d,%ld)%s",FUNC,l1,a1);
	  sprintf(a1,"%s",ax);
	  checkone(a1);
	  checkone(b1);
	  return 23;
	}
	else return 0;
      } 


    /** Avvertire tutti i processi della modifica, forse al posto delle 2 precedenti*/
      /*
printf("\n\n\t\t\t3) Il processo %d modifica le tabelle  f e  k (e i loro index)\n\n",rank);      
      */
    }

  /*************************************/

    else if ( (i1==FUNC)&&(i2==FUNC) ) {
#ifdef WDEBUG
      printf("\t%d) FUNC FUNC\n",rank);
#endif
      if((l1==l2)&&(product(atmp,btmp,a1,b1))) {
	checkone(a1);
	checkone(b1);
	
	return 24;
	/*Aggiungere un controllo sul nr. di argomenti (deve essere 0) */
      }
      else return 0;
    }

  /*************************************/

    else if (((i1==SYNC)&&(i2==SYNC))||((i1==TYPE)&&(i2==TYPE))) {
#ifdef WDEBUG
      printf("\t%d) SYNC SYNC\n",rank);
      printf("--> %s, %s\n",atmp,btmp); 
#endif
      {
	int res=product(atmp,btmp,a1,b1);
#ifdef WDEBUG
	printf("%d\n",res);
	printf("%d , %d\n",l1,l2);
#endif
	if((l1==l2)&&( res )) {
#ifdef WDEBUG
	  printf("%d) TYPE-TYPE\n",rank);
	  printf("%s, %s\n",a1,b1);
#endif
	  checkone(a1);
	  checkone(b1);
	  return 25;
	}
	else return 0;
      }
    }
    
    /********************* Interazioni relative all'if_then_else ***********/
    
    else if ( (i1==ITE)&&(i2==KONST) ) {
#ifdef WDEBUG
      printf("\t%d) ITE CONST\n",rank);
#endif
      if (product(atmp,btmp,a1,b1)) {
	char ax[100];  
	if(k[l2][1]==0) sprintf(ax,"X(%d,%d)%s",ITE,2,a1);
	else sprintf(ax,"X(%d,%d)%s",ITE,1,a1);
	
	sprintf(a1,"%s",ax);
	checkone(a1);
	checkone(b1);
	return 26;
      }
      else  return 0;
    }

  /*************************************/
    
    else if ( (i1==KONST)&&(i2==ITE) ) {
#ifdef WDEBUG
      printf("\t%d) KONST ITE\n",rank);
#endif
      if (product(atmp,btmp,a1,b1)) {
	char bx[100];  
	if(k[l1][1]==0) sprintf(bx,"X(%d,%d)%s",ITE,2,b1);
	else sprintf(bx,"X(%d,%d)%s",ITE,1,b1);
	
	sprintf(b1,"%s",bx);
	checkone(a1);
	checkone(b1);
	return 27;
      }
      else  return 0;
    }

  /*************************************/

    else if ( (i1==ITE)&&(i2==ITE) ) {
#ifdef WDEBUG
      printf("\t%d) ITE ITE\n",rank);
#endif
      if (l1==l2) {
	if (product(atmp,btmp,a1,b1)) {
	  checkone(a1);
	  checkone(b1);
	  return 28;
	}
	else return 0;
      }
      else  return 0;
    }

  /*************************************/

    /*
    else if ( (i1==ITE)&&(i2==ITE) ) {
#ifdef WDEBUG
      printf("\t%d) ITE ITE\n",rank);
#endif
      if (product(atmp,btmp,a1,b1)) {
	checkone(a1);
	checkone(b1);
	return 215;
      }
      else  return 0;
    }

    */

  /*************************************/

    else if ( (i1==ITE)&&(i2==SYNC) ) {
#ifdef WDEBUG
      printf("\t%d) ITE SYNC\n",rank);
#endif
      if (l2==0&&l1==0) {	    
	char acarlo[MAXLENWEIGHT];
	sprintf(acarlo,"X(%d,%d)",ITE,0);
	strcat(acarlo,atmp);
	strcpy(atmp,acarlo);
	if (product(atmp,btmp,a1,b1)) {
	  checkone(a1);
	  checkone(b1);
	  return 29;
	}
	else if( (l2==1)&&(l1==1||l1==2)) {
	  char ax[100];
	  sprintf(ax,"X(%d,%ld)%s",ITE,l1,a1);
	  
	  sprintf(a1,"%s",ax);
	  if (product(atmp,btmp,a1,b1)) {
	    checkone(a1);
	    checkone(b1);
	    return 30; 
	  }
	  else return 0;
	}
	else  return 0;
      }
      else  return 0;
    }

  /*************************************/
    
    else if ( (i1==SYNC)&&(i2==ITE) ) {
      
#ifdef WDEBUG
      printf("\t%d) SYNC ITE\n",rank);
#endif
      if (l1==0&&l2==0) {	    
	char bcarlo[MAXLENWEIGHT];
	sprintf(bcarlo,"X(%d,%d)",ITE,0);
	strcat(bcarlo,btmp);
	strcpy(btmp,bcarlo);
      }
      if (product(atmp,btmp,a1,b1)) {
	if (l1==0&&l2==0) { return 31;}	
	else if( (l1==1)&&(l2==1||l2==2)) {
	  char bx[100];  
	  sprintf(bx,"X(%d,%ld)%s",ITE,l2,b1);
	  
	  sprintf(b1,"%s",bx);
	  checkone(a1);
	  checkone(b1);
	  return 32;
	}
	else return 0;
      }
      else  return 0;
    }
    else return 0;
  }
  else return 0;
  /*  {
      printf("Illegal Interaction: (%s,%s)\nABORTING\nPlease, check your input graph!!\n",a,b);
      exit(-1);
      };
  */  
}


int findType(int tipo) {
  int i=0;
  while (i<=MAXTYPE) {
    if (t[i][0]==tipo) return i;
    i++;
  }
  printf("\n\tERRORE\n");
  return 0; 
}
