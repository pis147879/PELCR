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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef SOLARIS
#include <sys/systeminfo.h>
#endif
#include <sys/times.h>
#include <sys/types.h>
// #include "mpi.h"
#ifdef MPE_GRAPH
#include <mpe.h>
#endif
#include <time.h>

#include <string.h>

#include "h/var.h"

/* letters of exponential generators: fans w(i,l) and choices c(h,1) */
#define IS_GENERATOR(ch) (((ch) == 'w') || ((ch) == 'c'))

static void
StoreComputedValue(int type, USERTYPE result) {
	SetNextKIndex();
	k_refs[kindex]++;
	pelcr_value_set(&k_value[kindex], &result);
	pelcr_value_clear(&result);
	k_type[kindex] = type;
	newval = 1;
}

void
checkone(char *s) {
	char *r, sx[MAXLENWEIGHT], *ss;
	long p = 0;
	ss = s;
#ifdef WDEBUG
	//   printf("%s %d\n",s,p);
#endif
	while (s[0] == '!') {
		p = p + strtol(&s[1], &r, 10);
		s = r;
#ifdef WDEBUG
		//     printf("%s %d\n",s,p);
#endif
	};
	strcpy(sx, s);
	if (p != 0)
		sprintf(s, "!%ld%s", p, sx);
	sprintf(ss, "%s", s);
#ifdef WDEBUG
	//   printf("%s\n",s);
#endif
	return;
}

int
isone(term *w) {
	if (w[0] == '\0')
		return 1;
	if (!strpbrk(w, "wpqXc")) {
		return 1;
	} else
		return 0;
}

static int
product1(char *a, char *b, char *a1, char *b1) {
	long l1, l2, i1, i2, p1, p2;
	char *r;
	char *atmp = a, *btmp = b;
	if (atmp[0] == '\0') {
		sprintf(b1, "%s%s", b1, btmp);
#ifdef WDEBUG
//    printf("\n");
//    printf("A1:  %s %s\n",atmp,a1);
//   printf("B1:  %s %s\n",btmp,b1);
#endif
		checkone(a1);
		checkone(b1);
		return 1;
	}

	/*************************************/

	else if (btmp[0] == '\0') {
		sprintf(a1, "%s%s", a1, atmp);
#ifdef WDEBUG
//    printf("A2 %s %s\n",atmp,a1);
//    printf("B2 %s %s\n",btmp,b1);
#endif
		checkone(a1);
		checkone(b1);
		return 2;
	}

	/*************************************/

	else if (((atmp[0] == 'p') && (btmp[0] == 'p')) || ((atmp[0] == 'q') && (btmp[0] == 'q'))) {
		atmp++;
		btmp++;
		if (product1(atmp, btmp, a1, b1)) {
#ifdef WDEBUG
//      printf("A3:  %s %s\n",atmp,a1);
//      printf("B3:  %s %s\n",btmp,b1);
#endif
			checkone(a1);
			checkone(b1);
			local_family_reductions++;
			return 3;
		} else
			return 0;
	}

	/*************************************/

	else if (IS_GENERATOR(atmp[0]) && IS_GENERATOR(btmp[0])) {
		/* w(i,l): fan generator; c(h,1): choice generator (component 0 of Lambda*_nd only).
		   x^* y = 1 if x = y, 0 otherwise (ANNIHILATION) */
		if (atmp[0] != btmp[0])
			return 0;
#ifdef WDEBUG
//    printf("%s\n",&atmp[2]);
//    printf("%s\n",&btmp[2]);
#endif
		i1 = strtol(&atmp[2], &r, 10);
		atmp = r + 1;
		i2 = strtol(&btmp[2], &r, 10);
		btmp = r + 1;
#ifdef WDEBUG
//    printf("branch: %d , %d\n",i1,i2);
#endif
		if (i1 == i2) {
#ifdef WDEBUG
//      printf("%s\n",atmp);
//      printf("%s\n",btmp);
#endif
			strtol(atmp, &r, 10);
			atmp = r + 1;
			strtol(btmp, &r, 10);
			btmp = r + 1;
#ifdef WDEBUG
//      printf("A4:  %s %s\n",atmp,a1);
//      printf("B4:  %s %s\n",btmp,b1);
#endif
			if (product1(atmp, btmp, a1, b1)) {
#ifdef WDEBUG
//	printf("A4b: %s %s\n",atmp,a1);
//	printf("B4b: %s %s\n",btmp,b1);
#endif
				checkone(a1);
				checkone(b1);
				return 4;
			} else
				return 0;
		} else
			return 0;
	}

	/*******************************************************/

	else if ((atmp[0] == '!') && (btmp[0] == '!')) {
		int propagaInv = 0;
		p1 = strtol(&atmp[1], &r, 10);
		atmp = r;
		p2 = strtol(&btmp[1], &r, 10);
		btmp = r;
#ifdef WDEBUG
//    printf("\nlift: %d , %d\n",p1,p2);
#endif
		if (p1 == p2) {
#ifdef WDEBUG
//      printf("!eq %s\n",atmp);
//      printf("!eq %s\n",btmp);
#endif
			propagaInv = product1(atmp, btmp, a1, b1);
			if (propagaInv) {
#ifdef WDEBUG
//	printf("A5:  %s %s\n",atmp,a1);
//	printf("B5:  %s %s\n",btmp,b1);
#endif
				{
					char ax[MAXLENWEIGHT];
					char bx[MAXLENWEIGHT];
					sprintf(ax, "%s", a1);
					sprintf(bx, "%s", b1);
					sprintf(a1, "!%ld%s", p1, ax);
					sprintf(b1, "!%ld%s", p1, bx);
				}
				checkone(a1);
				checkone(b1);
				if ((propagaInv == 30) || (propagaInv == 31) || (propagaInv == 32) || (propagaInv == 33))
					return propagaInv;
				else
					return 5;
			} else
				return 0;
		} else if (p1 < p2) {
			char ax[MAXLENWEIGHT];
			char bx[MAXLENWEIGHT];
			sprintf(bx, "!%ld%s", p2 - p1, btmp);
			if (product1(atmp, bx, a1, b1)) {
				sprintf(ax, "%s", a1);
				sprintf(bx, "%s", b1);
				sprintf(a1, "!%ld%s", p1, ax);
				sprintf(b1, "!%ld%s", p1, bx);
				checkone(a1);
				checkone(b1);
				return 6;
			} else
				return 0;
		} else {
			char ax[MAXLENWEIGHT];
			char bx[MAXLENWEIGHT];
			sprintf(ax, "!%ld%s", p1 - p2, atmp);
			if (product1(ax, btmp, a1, b1)) {
				sprintf(ax, "%s", a1);
				sprintf(bx, "%s", b1);
				sprintf(a1, "!%ld%s", p2, ax);
				sprintf(b1, "!%ld%s", p2, bx);
				checkone(a1);
				checkone(b1);
				return 7;
			} else
				return 0;
		}
	}

	/*************************************/

	else if ((atmp[0] == '!') && IS_GENERATOR(btmp[0])) {
		/* !(u)^* x = x !^{e(x)}(u)^* (COMMUTATION, x = w or c of lift e(x)) */
		char letter = btmp[0];
		char ax[MAXLENWEIGHT];
		char bx[MAXLENWEIGHT];
		long px;
		char *Xstr = NULL;
		int konst_index = 0;

		p1 = strtol(&atmp[1], &r, 10);
		atmp = r;
		i2 = strtol(&btmp[2], &r, 10);
		btmp = r + 1;
		l2 = strtol(btmp, &r, 10);
		btmp = r + 1;
#ifdef WDEBUG
//    printf("p1=%d,i2=%d,l2=%d\n",p1,i2,l2);
//    printf("A8:  %s %s\n",atmp,a1);
//    printf("B8:  %s %s\n",btmp,b1);
#endif
		px = p1 + l2 - 1;
		if (px != 0)
			sprintf(ax, "!%ld%s", px, atmp);
		else
			sprintf(ax, "%s", atmp);
		if (product1(ax, btmp, a1, b1)) {
			sprintf(bx, "%s", b1);
			sprintf(b1, "%c(%ld,%ld)%s", letter, i2, l2, bx);
		} else
			return 0;
		checkone(a1);
		checkone(b1);

		Xstr = strstr(a1, "X(2");

		if (Xstr != NULL) {
			konst_index = atoi(&Xstr[4]);
			k_refs[konst_index]++;
		}
		return 8;
	}

	/*************************************/

	else if (IS_GENERATOR(atmp[0]) && (btmp[0] == '!')) {
		char letter = atmp[0];
		char ax[MAXLENWEIGHT];
		char bx[MAXLENWEIGHT];
		long px;
		char *Xstr = NULL;
		int konst_index = 0;

		i1 = strtol(&atmp[2], &r, 10);
		atmp = r + 1;
		l1 = strtol(atmp, &r, 10);
		atmp = r + 1;
		p2 = strtol(&btmp[1], &r, 10);
		btmp = r;
#ifdef WDEBUG
//    printf("i1=%d,l1=%d, p2=%d\n",i1,l1,p2);
//    printf("A9:  %s %s\n",atmp,a1);
//    printf("B9:  %s %s\n",btmp,b1);
#endif
		px = p2 + l1 - 1;
		if (px != 0)
			sprintf(bx, "!%ld%s", px, btmp);
		else
			sprintf(bx, "%s", btmp);
		if (product1(atmp, bx, a1, b1)) {
			sprintf(ax, "%s", a1);
			sprintf(a1, "%c(%ld,%ld)%s", letter, i1, l1, ax);
		} else
			return 0;
		checkone(a1);
		checkone(b1);
		Xstr = strstr(b1, "X(2");

		if (Xstr != NULL) {
			konst_index = atoi(&Xstr[4]);
			k_refs[konst_index]++;
		}
		return 9;
	}

	/*************************************/

	else if ((atmp[0] == '!') && ((btmp[0] == 'p') || (btmp[0] == 'q'))) {
		/* !(u)^* x = x !(u)^* for x = p, q of lift 1 (COMMUTATION) */
		char letter = btmp[0];
		char bx[MAXLENWEIGHT];
		btmp++;
		if (product1(atmp, btmp, a1, b1)) {
			sprintf(bx, "%s", b1);
			sprintf(b1, "%c%s", letter, bx);
			checkone(a1);
			checkone(b1);
			return 50;
		} else
			return 0;
	}

	/*************************************/

	else if (((atmp[0] == 'p') || (atmp[0] == 'q')) && (btmp[0] == '!')) {
		/* x^* !(u) = !(u) x^* for x = p, q of lift 1 (COMMUTATION) */
		char letter = atmp[0];
		char ax[MAXLENWEIGHT];
		atmp++;
		if (product1(atmp, btmp, a1, b1)) {
			sprintf(ax, "%s", a1);
			sprintf(a1, "%c%s", letter, ax);
			checkone(a1);
			checkone(b1);
			return 51;
		} else
			return 0;
	}

	/*************************************/

	else if (atmp[0] == '1') {
		atmp++;
		if (product1(atmp, btmp, a1, b1)) {
			checkone(a1);
			checkone(b1);
			return 10;
		} else
			return 0;
	}

	/*************************************/

	else if (btmp[0] == '1') {
		btmp++;
		if (product1(atmp, btmp, a1, b1)) {
			checkone(a1);
			checkone(b1);
			return 11;
		} else
			return 0;
	}

	/***************** Algebra esterna ********************/

	else if ((atmp[0] == 'X') && (btmp[0] == 'X')) {

#ifdef WDEBUG
		printf("\nExtern Algebra --> %s\n", &atmp[2]);
		printf("\nExtern Algebra --> %s\n", &btmp[2]);
#endif
		i1 = strtol(&atmp[2], &r, 10);
		atmp = r + 1;
		i2 = strtol(&btmp[2], &r, 10);
		btmp = r + 1;
#ifdef WDEBUG
		printf("Extern Algebra --> branch: %d , %d\n", i1, i2);
#endif
		l1 = strtol(atmp, &r, 10);
		atmp = r + 1;
		l2 = strtol(btmp, &r, 10);
		btmp = r + 1;
#ifdef WDEBUG
		printf("Extern Algebra -->  %s %s\n", atmp, a1);
		printf("Extern Algebra -->  %s %s\n", btmp, b1);
#endif

		/*************************************/

		if ((i1 == TYPE) && (i2 == ARG)) {
#ifdef WDEBUG
			printf("\t%d) TYPE ARG\n", rank);
#endif
			if (product1(atmp, btmp, a1, b1)) {
				char ax[MAXLENWEIGHT];
				if (t[l1][1] == TIN)
					sprintf(ax, "X(%d,%ld)%s", SYNC, l2 - 1, a1);
				else
					sprintf(ax, "X(%d,%ld)%s", SYNC, l2, a1);

				sprintf(a1, "%s", ax);
				checkone(a1);
				checkone(b1);
				return 12;
			} else
				return 0;
		}

		/*************************************/

		else if ((i1 == TYPE) && (i2 == ARGLAST)) {
#ifdef WDEBUG
			printf("\t%d) TYPE LASTARG\n", rank);
#endif
			if (product1(atmp, btmp, a1, b1)) {
				char ax[MAXLENWEIGHT];
				if (t[l1][1] == TIN)
					sprintf(ax, "X(%d,%ld)%s", SYNC, l2 - 1, a1);
				else
					sprintf(ax, "X(%d,%ld)%s", SYNC, l2, a1);

				sprintf(a1, "%s", ax);
				checkone(a1);
				checkone(b1);
				if (t[l1][1] == TOUT)
					return 40 + l2 % 2;
				else
					return 13;
			} else
				return 0;
		}

		/*************************************/

		else if ((i1 == ARG) && (i2 == TYPE)) {
#ifdef WDEBUG
			printf("\t%d) ARG TYPE\n", rank);
#endif
			if (product1(atmp, btmp, a1, b1)) {
				char bx[MAXLENWEIGHT];
				if (t[l2][1] == TIN)
					sprintf(bx, "X(%d,%ld)%s", SYNC, l1 - 1, b1);
				else
					sprintf(bx, "X(%d,%ld)%s", SYNC, l1, b1);

				sprintf(b1, "%s", bx);
				checkone(b1);
				checkone(a1);
				return 14;
			} else
				return 0;
		}

		/*************************************/

		else if ((i1 == ARGLAST) && (i2 == TYPE)) {
#ifdef WDEBUG
			printf("\t%d) ARG TYPE\n", rank);
#endif
			if (product1(atmp, btmp, a1, b1)) {
				char bx[MAXLENWEIGHT];
				if (t[l2][1] == TIN)
					sprintf(bx, "X(%d,%ld)%s", SYNC, l1 - 1, b1);
				else
					sprintf(bx, "X(%d,%ld)%s", SYNC, l1, b1);

				sprintf(b1, "%s", bx);
				checkone(b1);
				checkone(a1);
				if (t[l2][1] == TOUT)
					return 42 + l1 % 2;
				else
					return 15;
			} else
				return 0;
		}

		/****************************************/

		else if ((i1 == SYNC) && (i2 == ARG)) {
#ifdef WDEBUG
			printf("\t%d) SYNC ARG\n", rank);
#endif
			if (product1(atmp, btmp, a1, b1)) {
				char ax[MAXLENWEIGHT];
				sprintf(ax, "X(%d,%ld)%s", SYNC, l2, a1);

				sprintf(a1, "%s", ax);
				checkone(a1);
				checkone(b1);
				return 16;
			} else
				return 0;
		}

		/*************************************/

		else if ((i1 == ARG) && (i2 == SYNC)) {
#ifdef WDEBUG
			printf("\t%d) ARG SYNC\n", rank);
#endif
			if (product1(atmp, btmp, a1, b1)) {
				char bx[MAXLENWEIGHT];
				sprintf(bx, "X(%d,%ld)%s", SYNC, l1, b1);

				sprintf(b1, "%s", bx);
				checkone(a1);
				checkone(b1);
				return 17;
			} else
				return 0;
		}

		/******************************************/

		else if ((i1 == SYNC) && (i2 == FUNC)) {
			char bcarlo[MAXLENWEIGHT];
#ifdef WDEBUG
			printf("\t%d) SYNC FUNC\n", rank);
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

			if (l1 == 0) {
				sprintf(bcarlo, "X(%d,%ld)", FUNC, l2);
				strcat(bcarlo, btmp);
				strcpy(btmp, bcarlo);
			}
			if (product1(atmp, btmp, a1, b1)) {
				int ft;
				char ax[MAXLENWEIGHT];
				char bx[MAXLENWEIGHT];

				if (l1 == 0) {
					ft = findType(f[l2].type);
					sprintf(ax, "X(%d,%d)%s", TYPE, ft, a1);
					sprintf(a1, "%s", ax);

					sprintf(bx, "%s", b1);
					sprintf(b1, "%s", bx);

				} else if (l1 == f[l2].narg) {
					ft = findType(f[l2].type);
					sprintf(ax, "X(%d,%d)%s", TYPE, ft + 1, a1);
					sprintf(a1, "%s", ax);

					sprintf(bx, "X(%d,%ld)%s", FUNC, l2, b1);
					sprintf(b1, "%s", bx);
				}

				else
					return 0;

				checkone(a1);
				checkone(b1);
				return 18;
			} else
				return 0;
		}

		/*************************************/

		else if ((i1 == FUNC) && (i2 == SYNC)) {
			char acarlo[MAXLENWEIGHT];
#ifdef WDEBUG
			printf("\t%d) FUNC SYNC\n", rank);
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

			if (l2 == 0) {
				sprintf(acarlo, "X(%d,%ld)", FUNC, l1);
				strcat(acarlo, atmp);
				strcpy(atmp, acarlo);
			}
			if (product1(atmp, btmp, a1, b1)) {
				int ft;
				char ax[MAXLENWEIGHT];
				char bx[MAXLENWEIGHT];

				if (l2 == 0) {
					ft = findType(f[l1].type);

					sprintf(ax, "%s", a1);
					sprintf(a1, "%s", ax);

					sprintf(bx, "X(%d,%d)%s", TYPE, ft, b1);
					sprintf(b1, "%s", bx);
				} else if (l2 == f[l1].narg) {
					ft = findType(f[l1].type);
					sprintf(ax, "X(%d,%ld)%s", FUNC, l1, a1);
					sprintf(a1, "%s", ax);

					sprintf(bx, "X(%d,%d)%s", TYPE, ft + 1, b1);
					sprintf(b1, "%s", bx);
				} else
					return 0;
				checkone(a1);
				checkone(b1);
				return 19;
			} else
				return 0;
		}

		/*************************************/

		else if ((i1 == KONST) && (i2 == FUNC)) {
#ifdef WDEBUG
			printf("%d) CONST-FUNC WAIT %d, NARG %d\n", rank, f[l2].wait, f[l2].narg);
#endif
			if (f[l2].wait == f[l2].narg) {
				if (f[l2].narg == 1) { /* valuto la funzione di 1 variabile*/
#ifdef WDEBUG
					printf("%d) function evaluation (1)\n", rank);
#endif
					StoreComputedValue(f[l2].type, ((USERTYPE(*)(USERTYPE))f[l2].fun)(k_value[l1]));
				} else if (f[l2].narg == 2) { /* valuto la funzione di 2 variabili */
#ifdef WDEBUG
					printf("%d) function evaluation (2)\n", rank);
#endif
					StoreComputedValue(
					    f[l2].type,
					    ((USERTYPE(*)(USERTYPE, USERTYPE))f[l2].fun)((f[l2].s)[0], k_value[l1]));
				} else if (f[l2].narg == 3) { /* valuto la funzione di 3 variabili */
#ifdef WDEBUG
					printf("%d) function evaluation (3)\n", rank);
#endif
					StoreComputedValue(
					    f[l2].type,
					    ((USERTYPE(*)(USERTYPE, USERTYPE, USERTYPE))f[l2].fun)(
					        (f[l2].s)[0], (f[l2].s)[1], k_value[l1]));
				} else
					printf("EXCEPTION\n");

				/** Avvertire tutti i processi della modifica */
				/*
			printf("\n\n\t\t\t1) A-Il processo %d modifica le tabelle  f e  k (e i loro index)\n\n",rank);
				*/

#ifdef WDEBUG
				printf("--R-- %d ", kindex);
				pelcr_value_print(stdout, &k_value[kindex]);
				printf("\n%d) VALORE CALCOLATO ", rank);
				pelcr_value_print(stdout, &k_value[kindex]);
				printf("\n");
#endif
				if (product1(atmp, btmp, a1, b1)) {
					char ax[MAXLENWEIGHT];
					char bx[MAXLENWEIGHT];
					sprintf(ax, "X(%d,%d)%s", KONST, kindex, a1);
					sprintf(bx, "X(%d,%ld)%s", FUNC, l2, b1);
					sprintf(a1, "%s", ax);
					sprintf(b1, "%s", bx);
					checkone(a1);
					checkone(b1);

					return 20;
				} else
					return 0;

			} else {
				/* valuto parzialmente la funzione */
#ifdef WDEBUG
				printf("%d) PARTIAL EVAL FUNC\n", rank);
#endif
				pelcr_value_set(&(f[l2].s)[f[l2].wait - 1], &k_value[l1]);
				f[l2].wait++;

				/** Avvertire tutti i processi della modifica */
				/*
			printf("\n\n\t\t\t2) A-Il processo %d modifica le tabelle  f e  k (e i loro index)\n\n",rank);
				*/

				if (product1(atmp, btmp, a1, b1)) {
					char bx[MAXLENWEIGHT];
					sprintf(bx, "X(%d,%ld)%s", FUNC, l2, b1);
					sprintf(b1, "%s", bx);
					checkone(a1);
					checkone(b1);
					return 21;
				} else
					return 0;
			}

			/** Avvertire tutti i processi della modifica, forse al posto delle 2 precedenti*/
			/*
	  printf("\n\n\t\t\t3) Il processo %d modifica le tabelle  f e  k (e i loro index)\n\n",rank);
			*/
		}

		/*************************************/

		else if ((i1 == FUNC) && (i2 == KONST)) {
#ifdef WDEBUG
			printf("%d) FUNC-CONST WAIT %d, NARG %d\n", rank, f[l1].wait, f[l1].narg);
#endif
			if (f[l1].wait == f[l1].narg) {
				if (f[l1].narg == 1) { /* valuto la funzione di 1 variabile*/
#ifdef WDEBUG
					printf("%d) function evaluation (1)\n", rank);
#endif
					StoreComputedValue(f[l1].type, ((USERTYPE(*)(USERTYPE))f[l1].fun)(k_value[l2]));
				} else if (f[l1].narg == 2) { /* valuto la funzione di 2 variabili */
#ifdef WDEBUG
					printf("%d) function evaluation (2)\n", rank);
#endif
					StoreComputedValue(
					    f[l1].type,
					    ((USERTYPE(*)(USERTYPE, USERTYPE))f[l1].fun)((f[l1].s)[0], k_value[l2]));
				} else if (f[l1].narg == 3) { /* valuto la funzione di 2 variabili */
#ifdef WDEBUG
					printf("%d) function evaluation (3)\n", rank);
#endif
					StoreComputedValue(
					    f[l1].type,
					    ((USERTYPE(*)(USERTYPE, USERTYPE, USERTYPE))f[l1].fun)(
					        (f[l1].s)[0], (f[l1].s)[1], k_value[l2]));
				}

				else
					printf("EXCEPTION\n");

				/** Avvertire tutti i processi della modifica */
				/*
			printf("\n\n\t\t\t1) B-Il processo %d modifica le tabelle  f e  k (e i loro index)\n\n",rank);
				*/

#ifdef WDEBUG
				printf("--R-- %d ", kindex);
				pelcr_value_print(stdout, &k_value[kindex]);
				printf("\n%d) VALORE CALCOLATO ", rank);
				pelcr_value_print(stdout, &k_value[kindex]);
				printf("\n");
#endif
				if (product1(atmp, btmp, a1, b1)) {
					char ax[MAXLENWEIGHT];
					char bx[MAXLENWEIGHT];
					sprintf(bx, "X(%d,%d)%s", KONST, kindex, b1);
					sprintf(ax, "X(%d,%ld)%s", FUNC, l1, a1);
					sprintf(a1, "%s", ax);
					sprintf(b1, "%s", bx);
					checkone(a1);
					checkone(b1);

					return 22;
				} else
					return 0;
			} else {
				/* valuto parzialmente la funzione */
#ifdef WDEBUG
				printf("%d) PARTIAL EVAL FUNC\n", rank);
#endif
				pelcr_value_set(&(f[l1].s)[f[l1].wait - 1], &k_value[l2]);
				f[l1].wait++;

				/** Avvertire tutti i processi della modifica */
				/*
			  printf("\n\n\t\t\t2) B-Il processo %d modifica le tabelle  f e  k (e i loro index)\n\n",rank);
				*/

				if (product1(atmp, btmp, a1, b1)) {
					char ax[MAXLENWEIGHT];
					sprintf(ax, "X(%d,%ld)%s", FUNC, l1, a1);
					sprintf(a1, "%s", ax);
					checkone(a1);
					checkone(b1);
					return 23;
				} else
					return 0;
			}

			/** Avvertire tutti i processi della modifica, forse al posto delle 2 precedenti*/
			/*
	  printf("\n\n\t\t\t3) Il processo %d modifica le tabelle  f e  k (e i loro index)\n\n",rank);
			*/
		}

		/*************************************/

		else if ((i1 == FUNC) && (i2 == FUNC)) {
#ifdef WDEBUG
			printf("\t%d) FUNC FUNC\n", rank);
#endif
			if ((l1 == l2) && (product1(atmp, btmp, a1, b1))) {
				checkone(a1);
				checkone(b1);

				return 24;
				/*Aggiungere un controllo sul nr. di argomenti (deve essere 0) */
			} else
				return 0;
		}

		/*************************************/

		else if (((i1 == SYNC) && (i2 == SYNC)) || ((i1 == TYPE) && (i2 == TYPE))) {
#ifdef WDEBUG
			printf("\t%d) SYNC SYNC\n", rank);
			printf("--> %s, %s\n", atmp, btmp);
#endif
			{
				int res = product1(atmp, btmp, a1, b1);
#ifdef WDEBUG
				printf("%d\n", res);
				printf("%d , %d\n", l1, l2);
#endif
				if ((l1 == l2) && (res)) {
#ifdef WDEBUG
					printf("%d) TYPE-TYPE\n", rank);
					printf("%s, %s\n", a1, b1);
#endif
					checkone(a1);
					checkone(b1);
					return 25;
				} else
					return 0;
			}
		}

		/********************* Interazioni relative all'if_then_else ***********/

		else if ((i1 == ITE) && (i2 == KONST)) {
#ifdef WDEBUG
			printf("\t%d) ITE CONST\n", rank);
#endif
			if (product1(atmp, btmp, a1, b1)) {
				char ax[MAXLENWEIGHT];
				if (pelcr_value_is_zero(&k_value[l2]))
					sprintf(ax, "X(%d,%d)%s", ITE, 2, a1);
				else
					sprintf(ax, "X(%d,%d)%s", ITE, 1, a1);

				sprintf(a1, "%s", ax);
				checkone(a1);
				checkone(b1);
				return 26;
			} else
				return 0;
		}

		/*************************************/

		else if ((i1 == KONST) && (i2 == ITE)) {
#ifdef WDEBUG
			printf("\t%d) KONST ITE\n", rank);
#endif
			if (product1(atmp, btmp, a1, b1)) {
				char bx[MAXLENWEIGHT];
				if (pelcr_value_is_zero(&k_value[l1]))
					sprintf(bx, "X(%d,%d)%s", ITE, 2, b1);
				else
					sprintf(bx, "X(%d,%d)%s", ITE, 1, b1);

				sprintf(b1, "%s", bx);
				checkone(a1);
				checkone(b1);
				return 27;
			} else
				return 0;
		}

		/*************************************/

		else if ((i1 == ITE) && (i2 == ITE)) {
#ifdef WDEBUG
			printf("\t%d) ITE ITE\n", rank);
#endif
			if (l1 == l2) {
				if (product1(atmp, btmp, a1, b1)) {
					checkone(a1);
					checkone(b1);
					return 28;
				} else
					return 0;
			} else
				return 0;
		}

		/*************************************/

		/*
		else if ( (i1==ITE)&&(i2==ITE) ) {
	#ifdef WDEBUG
		  printf("\t%d) ITE ITE\n",rank);
	#endif
		  if (product1(atmp,btmp,a1,b1)) {
		checkone(a1);
		checkone(b1);
		return 215;
		  }
		  else  return 0;
		}

		*/

		/*************************************/

		else if ((i1 == ITE) && (i2 == SYNC)) {
#ifdef WDEBUG
			printf("\t%d) ITE SYNC\n", rank);
#endif
			if (l2 == 0 && l1 == 0) {
				char acarlo[MAXLENWEIGHT];
				sprintf(acarlo, "X(%d,%d)", ITE, 0);
				strcat(acarlo, atmp);
				strcpy(atmp, acarlo);
				if (product1(atmp, btmp, a1, b1)) {
					checkone(a1);
					checkone(b1);
					return 29;
				} else if ((l2 == 1) && (l1 == 1 || l1 == 2)) {
					char ax[MAXLENWEIGHT];
					sprintf(ax, "X(%d,%ld)%s", ITE, l1, a1);

					sprintf(a1, "%s", ax);
					if (product1(atmp, btmp, a1, b1)) {
						checkone(a1);
						checkone(b1);
						return 30;
					} else
						return 0;
				} else
					return 0;
			} else
				return 0;
		}

		/*************************************/

		else if ((i1 == SYNC) && (i2 == ITE)) {

#ifdef WDEBUG
			printf("\t%d) SYNC ITE\n", rank);
#endif
			if (l1 == 0 && l2 == 0) {
				char bcarlo[MAXLENWEIGHT];
				sprintf(bcarlo, "X(%d,%d)", ITE, 0);
				strcat(bcarlo, btmp);
				strcpy(btmp, bcarlo);
			}
			if (product1(atmp, btmp, a1, b1)) {
				if (l1 == 0 && l2 == 0) {
					return 31;
				} else if ((l1 == 1) && (l2 == 1 || l2 == 2)) {
					char bx[MAXLENWEIGHT];
					sprintf(bx, "X(%d,%ld)%s", ITE, l2, b1);

					sprintf(b1, "%s", bx);
					checkone(a1);
					checkone(b1);
					return 32;
				} else
					return 0;
			} else
				return 0;
		} else
			return 0;
	} else
		return 0;
	/*  {
	    printf("Illegal Interaction: (%s,%s)\nABORTING\nPlease, check your input graph!!\n",a,b);
	    exit(-1);
	    };
	*/
}

/* ---------------------------------------------------------------------------
   The non-deterministic dynamic algebra Lambda*_nd = Lambda* (x) Lambda*
   (reduced square, Definition ndexgoi2 of the ND-PELCR article):
       P = (1,p),  Q = (1,q),  W_j = (w_j,w_j),  C_h = (w_h,1) with e(w_h) = 1;
   products, involution and lift are componentwise, and a pair with a zero
   component is 0.

   A weight is a positive element.  It is written either
       "u1|u0"   (component 1, then component 0), or
       "u1"      for the pair (erase(u1), u1),
   where erase(u1) deletes p, q and the external letters X(..).  The second form
   is exact for every weight built from P, Q, W_j and lifts (component 0 of such a
   product is the same word without p and q), so choice-free computations keep
   the classical representation and the classical product.  Choice generators
   c(h,1) occur only in component 0; fan generators w(i,l) in both.
   --------------------------------------------------------------------------- */

long max_weight_length = 0;

static void
nd_overflow(const char *what, const char *w) {
	fprintf(stderr, "(%d) PELCR: weight too long in %s (MAXLENWEIGHT=%d): %.60s...\n", rank, what, MAXLENWEIGHT, w);
	fprintf(stderr, "(%d) PELCR: rebuild with a larger MAXLENWEIGHT (make MAXLENWEIGHT=...)\n", rank);
	exit(-1);
}

void
CheckWeightLength(const char *w, const char *what) {
	long n = (long)strlen(w);
	if (n > max_weight_length)
		max_weight_length = n;
	if (n >= MAXLENWEIGHT - 1)
		nd_overflow(what, w);
}

int
nd_is_pair(const char *w) {
	return strchr(w, '|') != NULL;
}

/* component 0 of a classical word: p, q, 1 and X(..) deleted, consecutive lifts merged */
void
nd_erase(const char *u1, char *u0) {
	long bang = 0;
	char *out = u0;
	const char *s = u1;
	char *r;

	*out = '\0';
	while (*s != '\0' && *s != '|') {
		if (*s == '!') {
			bang += strtol(s + 1, &r, 10);
			s = r;
		} else if (*s == 'X') {
			const char *e = strchr(s, ')');
			s = (e != NULL) ? e + 1 : s + strlen(s);
		} else if (IS_GENERATOR(*s)) {
			const char *e = strchr(s, ')');
			size_t n = (e != NULL) ? (size_t)(e - s + 1) : strlen(s);
			if ((size_t)(out - u0) + n + 24 >= MAXLENWEIGHT)
				nd_overflow("nd_erase", u1);
			if (bang != 0) {
				out += sprintf(out, "!%ld", bang);
				bang = 0;
			}
			memcpy(out, s, n);
			out += n;
			*out = '\0';
			s += n;
		} else
			s++; /* p, q, 1, and the EOT mark * */
	}
}

/* the two components of a weight */
void
nd_split(const char *w, char *u1, char *u0) {
	const char *bar = strchr(w, '|');
	if (bar == NULL) {
		strcpy(u1, w);
		nd_erase(w, u0);
	} else {
		size_t n = (size_t)(bar - w);
		memcpy(u1, w, n);
		u1[n] = '\0';
		nd_erase(bar + 1, u0); /* normal form of component 0 */
	}
}

/* writes the pair (u0,u1) into w, in the classical form when u0 = erase(u1) */
void
nd_join(char *w, const char *u1, const char *u0) {
	char e0[MAXLENWEIGHT], n0[MAXLENWEIGHT];
	nd_erase(u1, e0);
	nd_erase(u0, n0);
	if (strlen(u1) + strlen(n0) + 2 >= MAXLENWEIGHT)
		nd_overflow("nd_join", u1);
	if (strcmp(e0, n0) == 0)
		strcpy(w, u1);
	else
		/* a component without generators is the identity: written 1 (e.g. "!2" = !^2(1) = 1) */
		sprintf(w, "%s|%s", isone((char *)u1) ? "1" : u1, (n0[0] == '\0') ? "1" : n0);
}

/* stable form in Lambda*_nd: a^* b = b1 a1^* (0 if a^* b = 0); same convention as product1 */
int
product(char *a, char *b, char *a1, char *b1) {
	char a_1[MAXLENWEIGHT], a_0[MAXLENWEIGHT], b_1[MAXLENWEIGHT], b_0[MAXLENWEIGHT];
	char p1[MAXLENWEIGHT], n1[MAXLENWEIGHT], p0[MAXLENWEIGHT], n0[MAXLENWEIGHT];
	int out;

	if (!nd_is_pair(a) && !nd_is_pair(b))
		return product1(a, b, a1, b1);

	nd_split(a, a_1, a_0);
	nd_split(b, b_1, b_0);
	/* component 0 first: it has no external letters, hence no side effects */
	p0[0] = n0[0] = '\0';
	if (!product1(a_0, b_0, p0, n0))
		return 0;
	p1[0] = n1[0] = '\0';
	out = product1(a_1, b_1, p1, n1);
	if (!out)
		return 0;
	nd_join(a1, p1, p0);
	nd_join(b1, n1, n0);
	return out;
}

/* ---- lifts and prefixes used by the graph builder ---- */

/* !(u) on a word whose first letter is not the head of a free-variable edge */
static void
lift_word(char *u) {
	char tmp[MAXLENWEIGHT];
	char *r;
	long n;

	if (u[0] == '\0' || strcmp(u, "1") == 0)
		return;
	if (u[0] == '!') {
		n = strtol(u + 1, &r, 10);
		snprintf(tmp, sizeof(tmp), "!%ld%s", n + 1, r);
	} else
		snprintf(tmp, sizeof(tmp), "!1%s", u);
	strcpy(u, tmp);
}

/* the edge of a free variable starts with its fan generator w(i,l): a lift of the
   term raises the lift l of that generator (the variable is one box deeper) */
static void
lift_fan_head(char *u) {
	char tmp[MAXLENWEIGHT];
	char *r;
	long i, l;

	i = strtol(u + 2, &r, 10);
	l = strtol(r + 1, &r, 10);
	snprintf(tmp, sizeof(tmp), "%c(%ld,%ld)%s", u[0], i, l + 1, r + 1);
	strcpy(u, tmp);
}

void
LiftWeight(char *w) {
	char u1[MAXLENWEIGHT], u0[MAXLENWEIGHT];
	int fan_head;

	if (!nd_is_pair(w)) {
		if (w[0] == 'w')
			lift_fan_head(w);
		else
			lift_word(w);
		CheckWeightLength(w, "LiftWeight");
		return;
	}
	nd_split(w, u1, u0);
	fan_head = (u1[0] == 'w');
	if (fan_head)
		lift_fan_head(u1);
	else
		lift_word(u1);
	if (fan_head && u0[0] == 'w')
		lift_fan_head(u0);
	else
		lift_word(u0);
	nd_join(w, u1, u0);
	CheckWeightLength(w, "LiftWeight");
}

/* left multiplication by a multiplicative letter (p or q): component 1 only */
void
PrefixWeight1(char *w, const char *x) {
	char u1[MAXLENWEIGHT], u0[MAXLENWEIGHT], tmp[MAXLENWEIGHT];

	if (!nd_is_pair(w)) {
		if (strcmp(w, "1") == 0 || w[0] == '\0')
			snprintf(tmp, sizeof(tmp), "%s", x);
		else
			snprintf(tmp, sizeof(tmp), "%s%s", x, w);
		strcpy(w, tmp);
	} else {
		nd_split(w, u1, u0);
		if (strcmp(u1, "1") == 0 || u1[0] == '\0')
			snprintf(tmp, sizeof(tmp), "%s", x);
		else
			snprintf(tmp, sizeof(tmp), "%s%s", x, u1);
		nd_join(w, tmp, u0);
	}
	CheckWeightLength(w, "PrefixWeight1");
}

/* left multiplication by the choice generator C_h = (c_h, 1): component 0 only */
void
PrefixChoice(char *w, long h) {
	char u1[MAXLENWEIGHT], u0[MAXLENWEIGHT], tmp[MAXLENWEIGHT];

	nd_split(w, u1, u0);
	snprintf(tmp, sizeof(tmp), "c(%ld,1)%s", h, u0);
	if (u1[0] == '\0')
		strcpy(u1, "1");
	if (strlen(u1) + strlen(tmp) + 2 >= MAXLENWEIGHT)
		nd_overflow("PrefixChoice", w);
	sprintf(w, "%s|%s", u1, tmp);
	CheckWeightLength(w, "PrefixChoice");
}

/* renames the choice generators of a weight: copies of a #def symbol get fresh
   choice codes, so that the codes stay injective on the choice branches of the
   term.  The two branches of a choice have the codes 2i (left) and 2i+1 (right):
   a family i is mapped to a fresh family i' (from/to hold family indices) and the
   branch parity is kept, as the read-back orders the branches by their codes. */
void
RenameChoices(char *w, long *from, long *to, int *n, int max, long *counter) {
	char out[MAXLENWEIGHT];
	char *bar = strchr(w, '|');
	char *s, *r, *o;
	long h, fam;
	int k;

	if (bar == NULL || strstr(bar, "c(") == NULL)
		return;
	o = out;
	for (s = w; s <= bar; s++)
		*o++ = *s;
	while (*s != '\0') {
		if (s[0] == 'c' && s[1] == '(') {
			h = strtol(s + 2, &r, 10);
			fam = h / 2;
			for (k = 0; k < *n && from[k] != fam; k++)
				;
			if (k == *n) {
				if (*n >= max) {
					fprintf(stderr, "(%d) PELCR: too many choices in a copied symbol\n", rank);
					exit(-1);
				}
				if (*counter % 2 != 0)
					(*counter)++;
				from[k] = fam;
				to[k] = *counter / 2;
				*counter += 2;
				(*n)++;
			}
			o += sprintf(o, "c(%ld", 2 * to[k] + h % 2);
			s = r;
		} else
			*o++ = *s++;
	}
	*o = '\0';
	CheckWeightLength(out, "RenameChoices");
	strcpy(w, out);
}

int
findType(int tipo) {
	int i = 0;
	while (i <= MAXTYPE) {
		if (t[i][0] == tipo)
			return i;
		i++;
	}
	printf("\n\tERRORE\n");
	return 0;
}
