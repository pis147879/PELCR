/* Manager for distribution of workload 
 
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
#include <unistd.h>
#include <math.h>
#ifdef SOLARIS
#include <sys/systeminfo.h>
#endif
#include <sys/types.h>
#include <sys/times.h>
#ifdef MPE_GRAPH
#include <mpe.h>
#endif
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "var.h"


extern int fra_hot;

int NouvelleReservation(dest)
     int dest;
{
  TableProcess[dest]= TableProcess[dest]+1;
  return TableProcess[dest];
}

void SendAddEdge(N,E,sw,sto)
     edge*N;
     edge*E;
     char*sw;
     int sto;
{
    node*target;


#ifdef _DEBUG  
    DEBUG_DISTRIBUTION{
    fprintf(logfile,"SAD %d %d\n",saddflag,scount);
    fflush(logfile);
    fprintf(logfile,"(%d) STRING to SEND: %s to %d\n",rank,sw,N->rankpuit);
    fflush(logfile);
  };
#endif
  
    if(N->rankpuit==rank) {
#ifdef _DEBUG
        DEBUG_DISTRIBUTION {
            fprintf(logfile,"(%d) CREATOR %d\n",rank,N->creator);
            fprintf(logfile,"(%d) GET ADDR\n",rank);
        };
#endif
            if(N->creator!=rank) {
                target = (node*)BookedAddress(N->creator,(long)N->source);
#ifdef _DEBUG
        DEBUG_DISTRIBUTION
                fprintf(logfile,"(%d) ADDR TARGET BY HASHING [%p]\n",rank,target);
#endif
            }
            else
            {
                target= N->source;
#ifdef _DEBUG
        DEBUG_DISTRIBUTION
                fprintf(logfile,"(%d) ADDR TARGET BY REFERENCE [%p]\n",rank,target);
#endif
            };

#ifdef _DEBUG
        DEBUG_DISTRIBUTION {
            fprintf(logfile,"(%d) SELFSENDTO[(%d),(%d,",rank,N->rankpuit,N->creator);
            fprintf(logfile,"%p)]->[%p] Reference: [(%d),(%d,",N->source,target,E->rankpuit,E->creator);
            fprintf(logfile,"%ld[%p])], Storage: %d Weight: %s\n",(long)E->source,E->source,E->sto,sw);
            fflush(logfile);
        };
#endif

            AddEdge(target,E->rankpuit,E->source,sto,sw,E->creator,LEFT,LEFT);
            return;
    }
    else
    {
            int tr;
            if(size!=1){
                {
                    int localh= h-1;
                    while(localh!=h)
                        {
                            localh= h;
                            bip2++;
                            MPI_Test(&request,&flag,&status);
                            if(flag) {
                                char swgt[MAXLENWEIGHT];
                                node*pS;
                                TempProcess[status.MPI_SOURCE]= raddbuf[0];
#ifdef _DEBUG
            TRACING {
                                fprintf(logfile,"(%d) RCVADD(%d) (%d,",rank,status.MPI_SOURCE,raddbuf[6]);
                                fprintf(logfile,"%d[%p] on %d)->",raddbuf[4],(node*)(long)raddbuf[4],raddbuf[3]);
                                fprintf(logfile,"(%d,%d[%p]) ",raddbuf[1],raddbuf[2],(node*)(long)raddbuf[2]);
                                fprintf(logfile,"Weight: %s",(char*)raddbuf+7*sizeof(int));
            };
#endif
                                strcpy(swgt,"");
                                strcpy(swgt,(char*)(raddbuf+7*sizeof(int)));
                                pS = (node*)BookedAddress(raddbuf[1],raddbuf[2]);
#ifdef _DEBUG
            TRACING {
                                fprintf(logfile,"(%d) ADDRCONV(%d,%d",rank,raddbuf[1],raddbuf[2]);
                                fprintf(logfile,"[%p]) Address:[%p]\n",(node*)(long)raddbuf[2],pS);
            };
#endif
                                AddEdge((node*)pS,raddbuf[3],(node*)(long)raddbuf[4],raddbuf[5],swgt,raddbuf[6],LEFT,LEFT);
                                NodeCombustion(pS,LEFT);

#ifdef _DEBUG
            TRACING {
                                fprintf(logfile,"(%d) COMM AddEdge(",rank);
                                fprintf(logfile,"%p,((%d),",pS,raddbuf[3]);
                                fprintf(logfile,"(%d,%p)),%s)\n",status.MPI_SOURCE,(node*)(long)raddbuf[4],swgt);
            };
#endif
            
#ifdef _DEBUG
            DEBUG_MEM {
                                node*P;
                                edge*e;
                                P = G.hot;
                                for(;P!=NULL;)  {
                                    e= P->left.vector;
                                    fprintf(logfile,"(%d) EDGES([%p]): ",rank,P);
                                    for(;e!=NULL;e = e->vector) fprintf(logfile,"[%p] %s ",e->weight,e->weight);
                                    fprintf(logfile,"\n");
                                    P= P->nextpuit;
                                };
            };
#endif

                                addflag= 0;
                                h= h+1;
                                MPI_Irecv(raddbuf,MAXLENWEIGHT,MPI_INT,MPI_ANY_SOURCE,ADD_TAG,MPI_COMM_WORLD,&add_request);
		}
                            else
                                unaddtest++;
#ifdef _DEBUG
            DEBUG_DISTRIBUTION {
                            fprintf(logfile,"(%d) TESTO EOT %d %d",rank,eotflag,reotbuf[1]);
                            fprintf(logfile," %d[%p]\n",reotbuf[0],(node*)(long)reotbuf[0]);
            };
#endif

                            MPI_Test(&eot_request,&eotflag,&status);

#ifdef _DEBUG
            DEBUG_DISTRIBUTION {
                            fprintf(logfile,"(%d) RI-TESTO EOT %d %d %d\n",rank,eotflag,reotbuf[1],reotbuf[0]);
                            fflush(logfile);
	        };
#endif
                            if(eotflag) {
                                    node*pS;

                                    pS = (node*)BookedAddress(reotbuf[1],reotbuf[0]);

#ifdef _DEBUG
              DEBUG_DISTRIBUTION {
                                    fprintf(logfile,"(%d) RCVEOT(%d) Address:",rank,status.MPI_SOURCE);
                                    fprintf(logfile,"%d,%d[%p]\n",reotbuf[1],reotbuf[0],pS);
              };
#endif
                                    pS->left.eot= pS->left.eot+1;
#ifdef _DEBUG
              DEBUG_DISTRIBUTION {
                                    fprintf(logfile,"(%d) NEOTS",rank);
                                    fprintf(logfile,"[%p]: %d\n",pS,pS->left.eot);
              };

              DEBUG_MEM {
                                    node*P;
                                    edge*e;
                                    P = pS;
                                    e = P->left.vector;
                                    fprintf(logfile,"(%d) EDGES([%p]): ",rank,P);
                                    for(;e!=NULL;e= e->vector) fprintf(logfile,"[%p] %s ",e->weight,e->weight);
              };
                                    fflush(logfile);
#endif
                                    h = h+1;
                                    eotflag= 0;

                                    MPI_Irecv(reotbuf,2,MPI_INT,MPI_ANY_SOURCE,EOT_TAG,MPI_COMM_WORLD,&eot_request);
                            }
                            else
                                    uneottest++;
                        }
                }
           }
            sbuflong[1]= N->creator;
            sbuflong[2]= (long)N->source;
            sbuflong[3]= E->rankpuit;
            sbuflong[4]= (long)E->source;
            sbuflong[5]= E->sto;
            sbuflong[6]= E->creator;
            sbuflong[0]= temp;
        strcpy((char*)sbuf,(char*)sbuflong);
        strcpy((char*)(sbuf+7*sizeof(long)),sw);
        
#ifdef _DEBUG
        DEBUG_DISTRIBUTION {
            fprintf(logfile,"(%d) SENDTO[(%d),(%ld,",rank,N->rankpuit,sbuflong[1]);
            fprintf(logfile,"%p)]     Reference: [(%ld),(%d,",(node*)(long)sbuflong[2],sbuflong[3],rank);
            fprintf(logfile,"%p)], Storage: %ld ",(node*)(long)sbuflong[4],sbuflong[5]);
            fprintf(logfile,"Weight: %s\n",(char*)sbuf+7*sizeof(long));
            };
#endif

            tr = N->rankpuit;
            saddflag= 0;
            scount++;
            MPI_Send(sbuf,MAXLENWEIGHT,MPI_INT,tr,ADD_TAG,MPI_COMM_WORLD);
            
            return;
    };
}


void SendCreateNewNode(dest,sto,nS)
     int dest;
     int sto;
     edge*nS;
{
  if((dest==rank)&&(sto==IN))
    {
      nS->source= CreateNewNode(G.hot);
      G.hot= nS->source;
      nS->rankpuit= rank;
      nS->creator= rank;
      nS->sto= IN;
#ifdef _DEBUG
        DEBUG_DISTRIBUTION  fprintf(logfile,"IN) AutoREF: [%p] created\n",nS->source);
#endif
    }
  else
    if((dest==rank)&&(sto==OUT))
      {
          nS->source= CreateNewNode(G.cold);
          G.cold= nS->source;
          nS->rankpuit= rank;
          nS->creator= rank;
          nS->sto= OUT;
#ifdef _DEBUG
          DEBUG_DISTRIBUTION  fprintf(logfile,"OUT) AutoREF: [%p] created\n",nS->source);
#endif
      }
    else
      {
          nS->rankpuit= dest;
          nS->creator= rank;
          nS->sto= sto;
          nS->source= (node*)(long)NouvelleReservation(dest);
#ifdef _DEBUG
          DEBUG_DISTRIBUTION {
              if(sto==OUT)
                  fprintf(logfile,"OUT)");
              else
                  fprintf(logfile,"IN)");
              fprintf(logfile," BOOK((%d),(%d",dest,rank);
              fprintf(logfile," %ld[%p]))\n",(long)nS->source,nS->source);
          }
#endif
      }
  return;
}

int SendLightProcess()
{
  return LightProcess(rank);
}

int LightProcess(dest)
     int dest;
{
  static int min= 1;

  min++;
  if(TempProcess[min%size]>=temp){return rank;}
  else
    {
      return min%size;
    }

}

int LightProcess2(dest)
     int dest;
{
  static int min= 1;

  min++;

  /*
    if (temp > 500000) return 0;
  */
  if(TempProcess[min%size]>=temp)
    {return rank;}
  else
    {
      return min%size;
    };
}

int LightProcess3(dest)
     int dest;
{
  static int min= 1;

  min++;

  if(TempProcess[min%size]>=nhot)
    {return rank;}
  else
    {
      return min%size;
    };
}

/*
  int LightProcess4(dest)
  int dest;
  {
  if(TempProcess[lightprocess]>=nhot)
  {return rank;}
  else
  {
  return lightprocess;
  };
  }
*/


int LightProcess5(dest)
     int dest;
{
  static int index = 0;
  int lightprocess;

  lightprocess = index;
  if((++index) >= size) index = 0; 

  return lightprocess;
  
}


int LightProcess6(dest)
     int dest;
{
  static int min= 1;

  min++;

  return min%size;

}



int LightProcess7(dest)
     int dest;
{
  static int min = 1;

  min++;

  /*  if(((float)TempProcess[min%size])>=((float)1.1*nhot)) */

  
  if(TempProcess[min%size]>=(fra_hot-20))
    {return rank;}
  else
    {
      return min%size;
    };
  
}


int LightProcess8(dest)
     int dest;
{
  static int min = 1;
  static int flag = 0;
  int temp;
  /*  if(((float)TempProcess[min%size])>=((float)1.1*nhot)) */

  
  if(flag < BURST)
    {
      flag++;
      return min%size;
    }
  else
    {
      flag = 0;
      temp = min;
      min++;
      return temp%size;
    };
  
}

void buf_flush()
{
  /* qui viene replicata l'analisi sulla priorita' del messaggio*/
  int priority,i,ub;
  struct mbuffer*l;
  struct messaggio *m;

     
  for(i=0; i<local_pending; i++)
    {
      m=(struct messaggio *)(buf[rank]+(i*sizeof(struct messaggio)));
      ub = UpperBound(m);
      priority = floor((MINPRIORITY-1)*(1-(float)ub/(float)maxubound)) ;
					
//      DEBUG      printf("(%d) upperbound function %d ; max =%d ",rank ,ub,maxubound);
//      DEBUG      printf("(%d):: priority %d\n", rank, priority);
       
      l= &incoming[priority];

      
      /*
	printf("STACK DUMP(%d-%d)\n",l->first,l->last);
	fflush(stdout);
      */

      if((l->last + 1)%MAXPENDING == l->first)
	{
	  printf("Exceeded size of incoming buffer\nAbort\n");
	  exit(-1);
	}
      memcpy((char*)(&(l->stack[l->last])),(char*)m,sizeof(struct messaggio));
      l->last= (l->last+1)%MAXPENDING;
      
      
#ifdef _DEBUG
        DEBUG_DISTRIBUTION {
            //	printf("\nSTACK DUMP(%d-%d)\n",l->first,l->last);
            fflush(stdout);
        };
#endif
      
      edges_counter++;
    }

  local_pending = 0;
}


void PopMessage(struct messaggio *m,struct mbuffer *l) 
{
  int i;

  if(l->first==l->last) {
    printf("Pop from empty buffer\n Abort\n");
    exit(-1);
  }
  i= l->first;
  memcpy((char*)m,(char*)(&(l->stack[i])),sizeof(struct messaggio));
  /*
    if(m->timestamp!= (timestamp+1)) {
    printf("sequence error\n");
    exit(-1);
    }
  */
  timestamp++;
  InCounter[m->sender]++;
  if(m->sender!=rank) {
    TempProcess[m->sender]= m->temp;
    if (m->temp < TempProcess[lightprocess]) lightprocess=m->sender;
  }
  l->first= (l->first+1)%MAXPENDING;
  
  if (strstr(m->weight,"X(3")) {
    char *atmp;
    int indice=0,i;    
    atmp=strstr(m->weight,"X(3");
    indice=atoi(&atmp[4]);

    if(m->funWhich == EXT)
    {
      sprintf(&atmp[4],"%d)",XAddFun(indice, m->funWait));

      for(i=0;i < m->funWait-1;i++) f[fcounter].s[i]=m->funArgs[i];
    }
    else
    {
      if (indice>findex) findex=indice;
      f[indice].wait=m->funWait;
      f[indice].narg=m->funNarg;
      f[indice].type=m->funType;
      f[indice].which=m->funWhich;
      for(i=0;i < m->funWait-1;i++) f[indice].s[i]=m->funArgs[i];
      switch (m->funWhich) {
      case SUCC   : f[indice].fun= pelcr_succ   ; break;
      case ADD    : f[indice].fun= pelcr_add    ; break;
      case PRED   : f[indice].fun= pelcr_pred   ; break;
      case MULT   : f[indice].fun= pelcr_mult   ; break;
      case AND    : f[indice].fun= pelcr_and    ; break;
      case NOT    : f[indice].fun= pelcr_not    ; break;
      case ISZERO : f[indice].fun= pelcr_iszero ; break;
      }
    }
  }
  else if (strstr(m->weight,"X(2")) {
    char *atmp;
    atmp=strstr(m->weight,"X(2");

    SetNextKIndex();          
    sprintf(&atmp[4],"%d)",kindex);
    k[kindex][0]=m->funType;
    k[kindex][1]=m->funWait;
    k[kindex][2]++;    
  }

  edges_counter--;
  return;
}

void PushMessage(m)
     struct messaggio *m;
{
  char*lout;
  //int h,temporaneo;
  long temporaneo;
  int dest ;   
  int nc;

  dest = m->vtarget.rankpuit;

  m->timestamp=outtimestamp;
  outtimestamp++;

  m->sender= rank;
  OutCounter[dest]++;


  /*
      if (dest!=rank) 
      {
      printf("\n dest=%d <------ rank=%d",dest,rank);
      read(0,&c,1);
      }
   */



  if(dest == rank)
    {
      int temp; 
	
      temp = local_pending * sizeof(struct messaggio);
      lout = buf[rank];
      lout = lout + temp;

      memcpy((char*)lout,(char*)m,sizeof(struct messaggio));
      local_pending++;
      /*******************************
					se local_pending supera la taglia di buf[rank] allora bisogna svuotare il buffer
					buf_flush()
      **********************/

      if (local_pending == MAXAWIN) {
#ifdef _DEBUG
          DEBUG_DISTRIBUTION  {
              printf("(%d) local pending= %d\n",rank,local_pending);
              fflush(stdout);
          };
#endif
          buf_flush();
      } 
#ifdef WITHTHREAD
        if (THREAD) pthread_mutex_unlock(&mutex);
#endif
        return;
    }
  else {
    lout = buf[dest] + sizeof(int);
    lout = lout + outcontrol[dest]*sizeof(struct messaggio);

    memcpy((char*)lout,(char*)m,sizeof(struct messaggio));
    outcontrol[dest]++;
    
    /*
      temporaneo = outcontrol[dest];
    
      memcpy((char*)buf[dest],(char*)&temporaneo,sizeof(int));
    */



    if(outcontrol[dest]>=aggregationWindow[dest]) {

      /*
	int nc;
      */

      temporaneo = outcontrol[dest];
      memcpy((char*)buf[dest],(char*)&temporaneo,sizeof(long));

      nc = sizeof(long) + outcontrol[dest]*sizeof(struct messaggio);
      /***************** Carlo ***************************************/
      nFullSend++;
      nrApplMsg[dest]+=outcontrol[dest];
      nrFisicMsg[dest]++;
      
      /*MARCO*/
      cumulate_timeout[dest] += maxTick[dest] ;
      applicative_msgs[dest] += outcontrol[dest];
      physical_msgs[dest]++ ;
      global_physical_msgs++ ; 
      
      /****** SendNum 1 : SEND su FULLBUFFER **************************/

#ifdef _DEBUG
        DEBUG_AGGREGATION {
            printf("(%d) FULL BUFFER SEND - destination = %d - ncc = %d\n",rank,dest,nc);
            fflush(stdout);
        };
#endif

      MPI_Bsend(buf[dest],nc,MPI_CHAR,dest,DATA_TAG,MPI_COMM_WORLD);


      /*
	FunReceiveMessages() ;
	MPI_Wait(&ireq,&stsq);
      */


      /*
	{
	float now;
	times(&smtime);
	
	now = (smtime.tms_utime+smtime.tms_stime)/60.0;
	  
	OUTPUT {
	printf("___<______________________><________________");
	fflush(stdout);
	fprintf(maxwinfile[dest],"%f %d\n",now,aggregationWindow[dest]);
	fprintf(mawfile[dest],"%f %f\n",now,(float)((float)nrApplMsg[dest]/(float)nrFisicMsg[dest]));
	fflush(maxwinfile[dest]);
	fflush(mawfile[dest]);
	} 
	  
	}
      */
      outcontrol[dest]= 0;
      tickcontrol[dest] = 0;
    }
  }
  
    /*
   {  if((0)){
    for(h = 0; h < size; h++) tickcontrol[h]++;
    for(h = 0; h < size; h++) 
      if( (outcontrol[h]>0) && (tickcontrol[h]>=maxTick[h]) ) {
	int ncc;

	ncc = sizeof(int) + outcontrol[h]*sizeof(struct messaggio);

	
	//  printf("(%d) TIMEOUT SEND - destination = %d  - ncc = %d\n",rank,h,ncc);
	//  fflush(stdout);
	

	temporaneo = outcontrol[h];
      
	memcpy((char*)buf[h],(char*)&temporaneo,sizeof(int));
	nTickSend++;
	nrApplMsg[h]+=outcontrol[h];
	nrFisicMsg[h]++;
	cumulate_timeout[h] +=  maxTick[h] ; 
	applicative_msgs[dest] += outcontrol[dest];
	physical_msgs[dest]++ ;
	global_physical_msgs++ ; 

	DEBUG{
	  printf("(%d) TIMEOUT SEND - destination = %d - ncc = %d\n",rank,h,ncc);
	  fflush(stdout);
	};	

	MPI_Bsend(buf[h],ncc,MPI_CHAR,h,DATA_TAG,MPI_COMM_WORLD);

	outcontrol[h]= 0;
	tickcontrol[h] = 0;
      }
    }}
*/
}


void AggregationControl1() {/*copiata*/
  /* Varia solo la dim della finestra d'aggregazione */
  
  int pr;
  float aggregationRatio[MAXNPROCESS]; 
  
  for(pr=0;pr<size;pr++)
    if ((pr != rank)&&(nrFisicMsg[pr]!=0)) {
      aggregationRatio[pr] = (((float)nrApplMsg[pr])/((float)nrFisicMsg[pr]))/(float)aggregationWindow[pr];
      /*
	printf("(%d) %d ticks",rank,bip3);
	printf("\t WDIM[%d]=%d",pr,aggregationWindow[pr]);
	printf("\tFis=%d\tApp=%d\tRatio=%4.2f\n",nrFisicMsg[pr], nrApplMsg[pr],aggregationRatio[pr]);
      */
      if (aggregationRatio[pr]>=TRHRIGHT) {
	/* aggrego molto: aumentare la dim del buffer*/
	aggregationWindow[pr]=(int)((float)aggregationWindow[pr]+BETA);
	if (aggregationWindow[pr]>MAXAWIN)
	  aggregationWindow[pr]=MAXAWIN;
      }
      else if (aggregationRatio[pr]<=TRHLEFT) {
	/* diminuire */
	aggregationWindow[pr]=(int)((float)aggregationWindow[pr]+ALPHA);
	if (aggregationWindow[pr]<MINAWIN)
	  aggregationWindow[pr]=MINAWIN;
      }    
      /* se volessi calcolare aggregationRatio solo in base all comportamento durante
	 la finestra d'osservazione precedente dovrei azzerare nrApplMsg e nrFisicMsg */
      nrFisicMsg[pr]=0;
      nrApplMsg[pr]=0; 
    }
}

void AggregationControl2() {/*copiata*/
  /* varia solo il maxTick */
  int pr;
  float aggregationRatio[MAXNPROCESS];
  /*  float gainLossRatio[MAXNPROCESS];*/
  
  for(pr=0;pr<size;pr++)
    if ((pr != rank)&&(nrFisicMsg[pr]!=0)) {
      aggregationRatio[pr] = 
	(((float)nrApplMsg[pr])/((float)nrFisicMsg[pr]))/(float)aggregationWindow[pr];
      /*
	printf("(%d) %d ticks",rank,bip3);
	printf("\t WDIM[%d]=%d",pr,aggregationWindow[pr]);
	printf("\tFis=%d\tApp=%d\tRatio=%4.2f",nrFisicMsg[pr],nrApplMsg[pr],aggregationRatio[pr]);
      */
      if (aggregationRatio[pr]>=TRHRIGHT) {
	/* aggrego molto e quindi riempio prima il buffer,
	   aumento  il maxtick nella 
	   speranza di dare piu tempo di aggregare tanto
	*/
	maxTick[pr]=(int)((float)maxTick[pr]+BETATICK);
	if (maxTick[pr]>MAXTICK)
	  maxTick[pr]=MAXTICK;
      }
      else if (aggregationRatio[pr]<=TRHLEFT) {
	/* diminuire */
	maxTick[pr]=(int)((float)maxTick[pr]+ALPHATICK);
	if (maxTick[pr]<MINTICK)
	  maxTick[pr]=MINTICK;
      }    
      /*      printf("\t(%d) maxTick[%d]=%d\n",rank,pr,maxTick[pr]);*/
      /* se volessi calcolare aggregationRatio solo in base all comportamento durante
	 la finestra d'osservazione precedente dovrei azzerare nrApplMsg e nrFisicMsg */
      nrFisicMsg[pr]=0; 
      nrApplMsg[pr]=0; 
    }
}


void AggregationControl3() {/*copiata*/
  /* varia sia la dim della finestra di aggregazione che il maxTick */
  int pr;
  float aggregationRatio[MAXNPROCESS]; 
  /*float gainLossRatio[MAXNPROCESS];*/
  
  for(pr=0;pr<size;pr++)
    if ((pr != rank)&&(nrFisicMsg[pr]!=0)) {
      aggregationRatio[pr] =
	(((float)nrApplMsg[pr])/((float)nrFisicMsg[pr]))/(float)aggregationWindow[pr];
      /*  printf("(%d) %d ticks",rank,bip3);
	  printf("\t WDIM[%d]=%d",pr,aggregationWindow[pr]);
	  printf("\tFis=%d\tApp=%d\tRatio=%4.2f",nrFisicMsg[pr],nrApplMsg[pr],aggregationRatio[pr]); */
      if (aggregationRatio[pr]>=TRHRIGHT) {
	/* aggrego molto  */
	aggregationWindow[pr]=(int)((float)aggregationWindow[pr]+BETA);
	if (aggregationWindow[pr]>MAXAWIN)  aggregationWindow[pr]=MAXAWIN;
	maxTick[pr]=(int)((float)maxTick[pr]+BETATICK);
	if (maxTick[pr]>MAXTICK) maxTick[pr]=MAXTICK;
      }
      else if (aggregationRatio[pr]<TRHLEFT) {
	/* aggrego poco  */
	aggregationWindow[pr]=(int)((float)aggregationWindow[pr]+ALPHA);
	if (aggregationWindow[pr]<MINAWIN) aggregationWindow[pr]=MINAWIN;
	maxTick[pr]=(int)((float)maxTick[pr]+ALPHATICK);
	if (maxTick[pr]<MINTICK) maxTick[pr]=MINTICK;
      }
      /* printf("\t(%d) maxTick[%d]=%d\n",rank,pr,maxTick[pr]); */
      
      nrFisicMsg[pr]=0; 
      nrApplMsg[pr]=0; 
    }
}


void AggregationControl4() {/*copiata*/
  /* variano entrambi perÃ² in controtendenza */
  int pr;
  float aggregationRatio[MAXNPROCESS]; 
  
  for(pr=0;pr<size;pr++)
    if ((pr != rank)&&(nrFisicMsg[pr]!=0)) {
      aggregationRatio[pr]=(((float)nrApplMsg[pr])/((float)nrFisicMsg[pr]))/(float)aggregationWindow[pr];
      /*
	printf("(%d) %d ticks",rank,bip3);
	printf("\tFis=%d\tApp=%d\tRatio=%4.2f",nrFisicMsg[pr],nrApplMsg[pr],aggregationRatio[pr]);
	printf("\t WDIM[%d]=%d",pr,aggregationWindow[pr]);
      */
      if (aggregationRatio[pr]>BETA) {
	/* aggrego molto  */
	aggregationWindow[pr]=(int)((float)aggregationWindow[pr]-BETA);
	if (aggregationWindow[pr]<MINAWIN)
	  aggregationWindow[pr]=MINAWIN;
	maxTick[pr]=(int)((float)maxTick[pr]+BETATICK);
	if (maxTick[pr]>MAXTICK)
	  maxTick[pr]=MAXTICK;
      }
      else if (aggregationRatio[pr]<=ALPHA) {
	/* aggrego poco  */
	aggregationWindow[pr]=(int)((float)aggregationWindow[pr]-ALPHA);
	if (aggregationWindow[pr]>MAXAWIN)
	  aggregationWindow[pr]=MAXAWIN;
	maxTick[pr]=(int)((float)maxTick[pr]+ALPHATICK);
	if (maxTick[pr]<MINTICK)
	  maxTick[pr]=MINTICK;
      }
      printf("\t maxTick[%d]=%d\n",pr,maxTick[pr]);
      
      /* se volessi calcolare aggregationRatio solo in base all comportamento durante
	 la finestra d'osservazione precedente dovrei azzerare nrApplMsg e nrFisicMsg */
      nrFisicMsg[pr]=0; 
      nrApplMsg[pr]=0; 
    }
}


void AggregationControl5() {/*copiata*/
  /* questa predice che la prossima finestra Ã¨ come la media precedente 
     in controtendenza */
  int pr;
  float aggregationRatio[MAXNPROCESS]; 
  
  for(pr=0;pr<size;pr++)
    if ((pr != rank)&&(nrFisicMsg[pr]!=0)) {
      aggregationRatio[pr]=(int)((float)nrApplMsg[pr])/((float)nrFisicMsg[pr]);
      
      if (aggregationRatio[pr]>(BETA*aggregationWindow[pr])) {
	aggregationWindow[pr]=aggregationRatio[pr]+BETA;
	if (aggregationWindow[pr]>MAXAWIN)
	  aggregationWindow[pr]=MAXAWIN;	
	maxTick[pr]=(int)((float)maxTick[pr]+ALPHATICK);
	if (maxTick[pr]<MINTICK)
	  maxTick[pr]=MINTICK;
      }
      else if (aggregationRatio[pr]<=(ALPHA*aggregationWindow[pr])) {
	aggregationWindow[pr]=aggregationRatio[pr]+ALPHA;
	if (aggregationWindow[pr]<MINAWIN)
	  aggregationWindow[pr]=MINAWIN;
	maxTick[pr]=(int)((float)maxTick[pr]+BETATICK);
	if (maxTick[pr]>MAXTICK)
	  maxTick[pr]=MAXTICK;
	
      }
      /*
	printf("(%d) %d ticks",rank,bip3);
	printf("\tFis=%d\tApp=%d\tRatio=%4.2f",nrFisicMsg[pr],nrApplMsg[pr],aggregationRatio[pr]);
	printf("\t WDIM[%d]=%d",pr,aggregationWindow[pr]);
	printf("\t maxTick[%d]=%d\n",pr,maxTick[pr]);
      */
      /* se volessi calcolare aggregationRatio solo in base all comportamento durante
	 la finestra d'osservazione precedente dovrei azzerare nrApplMsg e nrFisicMsg */
      nrFisicMsg[pr]=0; 
      nrApplMsg[pr]=0; 
    }
}

#ifdef WITHTHREAD
void *ThreadReceiveMsgs() {
  
  int temporaneo1,i;
  if(size!=1) {
    dataflag= 1;
    MPI_Iprobe(MPI_ANY_SOURCE,DATA_TAG,MPI_COMM_WORLD,&dataflag,&status);
    while(dataflag) {
      int maxdim= sizeof(int) + MAXAWIN*sizeof(struct messaggio);
      char*position;
      
      check_passed= FALSE;
      MPI_Recv(rbuf,maxdim,MPI_CHAR,MPI_ANY_SOURCE,DATA_TAG,MPI_COMM_WORLD,&status);
      position= rbuf + sizeof(int);
      
      memcpy((char*)&temporaneo1,(char*)rbuf,sizeof(int));
      aggregation_cumulate += temporaneo1;
      num_receives++;
      
      if((tempfile!=NULL)&&(!(num_receives%40))) {
	float now;
	times(&smtime);
	
	now = (smtime.tms_utime+smtime.tms_stime)/60.0;
	OUTPUT
	{
	  fprintf(coldfile,"%f %d\n",now,temporaneo1); 
	  fprintf(anamfile,"%f %ld\n",now, aggregation_cumulate/num_receives);
	  fflush(coldfile);
	  fflush(anamfile);
	}
      }

      /***************************************************
    	  questo pezzo di codice deve realizzare lo scheduling,
       	  l'aggregato e puntato da "position", va ora disaggregato
       	  inserendo nel buffer incoming ogni messaggio, in relazione alla
       	  euristica.	
      ********************************/
      /*printf("(%d) RCVS %d, TOTAGG %d, AGGMSG %d\n",
	rank,num_receives,aggregation_cumulate,temporaneo1);
	printf("arrivato messaggio (taglia %d)\n",temporaneo1);
      */
      /*  for(i= 0;i<AGGREGATIONWINDOW;i++)  */



      for(i= 0;i<temporaneo1;i++) {
	int priority ;
        int ub;
	
	pthread_mutex_lock(&mutex);

	/*SCHEDULER*/
	ub = UpperBound((struct messaggio*)position);
        priority = floor((MINPRIORITY-1)*(1-(float)ub/(float)maxubound)) ;
	/*
	  printf("ub = %d maxub = %d - priority = %d\n",ub,maxubound,priority); 
	*/
        /* FINE SCHEDULER */

	PushIncomingMessage(priority,(struct messaggio*)position);
	position= position+sizeof(struct messaggio);
	pthread_mutex_unlock(&mutex);
      }
      MPI_Iprobe(MPI_ANY_SOURCE,DATA_TAG,MPI_COMM_WORLD,&dataflag,&status);
    }
  }	  

  /* scheduler*/  
  buf_flush();
  contatore_combustioni_f = 0;
  schedule = 0 ;
  /* fine scheduler */

  pthread_exit(0);
}

void *ThreadInteraction() {
  int contatore_combustioni_f = 0;
  int nc1, temporaneo2;  
  int h;
  MPI_Request ireq;
  MPI_Status stsq;

  /*
    while( (!EmptyBuffer(&incoming)) && (contatore_combustioni_f < CHECKTICKS) ) {
    lidle = idle;
    idle += loops-1;
    loops = 0;
    nhot=BDump(&incoming[schedule]);
    bip2++;
    contatore_combustioni_f++;
    pthread_mutex_lock(&mutex);
    PopMessage(&msg,&incoming);
    pthread_mutex_unlock(&mutex);
  */

  while( (edges_counter>0) && (contatore_combustioni_f < CHECKTICKS) )
    {	
      lidle = idle;
      idle += loops-1;
      loops = 0;
#ifdef _DEBUG
        DEBUG_DISTRIBUTION fprintf(logfile,"(%d) ...seeking a non-empty incoming buffer\n",rank);
#endif
        
      while (((schedule<MINPRIORITY)&&(!(nhot=BDumpS(&incoming[schedule])))))
        {
#ifdef _DEBUG
            DEBUG_DISTRIBUTION fprintf(logfile,"(%d) BUFFER %d ",rank,schedule);
#endif
            schedule++;
        };
#ifdef _DEBUG
        DEBUG_DISTRIBUTION fprintf(logfile,"(%d) %d is the first non empty \n",rank,schedule);
#endif
      if (schedule < MINPRIORITY)
	{
	  bip2++;
	  contatore_combustioni_f++;
	  DEBUG	fprintf(logfile,"(%d) POP(%d) \n", rank, schedule);

	  pthread_mutex_lock(&mutex);
	  PopMessage(&msg,&incoming[schedule]);
	  pthread_mutex_unlock(&mutex);

	  DEBUG{
	    fprintf(logfile,"-> %s store:",msg.weight);
	    fprintf(logfile,"%d sign:%d side:%d\n",msg.vsource.sto,msg.vsource.side,msg.side);
	  }
		    
	  DEBUG{
	    fprintf(logfile,"-> %s store:",msg.weight);
	    fprintf(logfile,"%d sign:%d side:%d\n",msg.vsource.sto,msg.vsource.side,msg.side);
	  }
    
    
	  switch(msg.tpy)
	    {
	    case EOT_TAG:
	      {
		node*sourceaddress;
		edge e;

		e = msg.vsource;
		sourceaddress= StoreBookedAddress(e.creator,(long)e.source,e.sto);

      		if(msg.side==LEFT)
		  sourceaddress->left.eot++;
		else
		  sourceaddress->right.eot++;

		if((e.sto!=OUT)&&(sourceaddress->left.eot==1)&&(sourceaddress->right.eot==1))
		  sourceaddress= SinkRemove(sourceaddress);
	      }

	      break;
	    case ADD_TAG:
	      {
		node*targetaddress;
		edge e;

		targetaddress= StoreBookedAddress((msg.vtarget).creator,
						  (long)(msg.vtarget).source,msg.vtarget.sto);
		e = msg.vsource;
		AddEdge(targetaddress,e.rankpuit,e.source,e.sto,msg.weight,e.creator,msg.side,e.side);

		NodeCombustion(targetaddress,msg.side);
		/*		    DEBUG Print(G,&incoming,bip3); */
		tim = time(&finaltime);
	      }
	      break;
	    } /* END OF SWITCH */

	  OUTPUT WriteStats();
	      
	  /*   schedule=0;	
	       while (((schedule<MINPRIORITY)&&(!(nhot=BDumpS(&incoming[schedule]))))) schedule++;
	       schedule++;*/

	  for(h = 0; h < size; h++) 
	    {
	      tickcontrol[h]++;
	      if((outcontrol[h]>0) && (tickcontrol[h]>=maxTick[h]))
		{
		  nc1 = sizeof(int) + outcontrol[h]*sizeof(struct messaggio);
		  temporaneo2 = outcontrol[h];
		      
		  memcpy((char*)buf[h],(char*)&temporaneo2,sizeof(int));
		  /***************** Carlo *******************************/
		  nTickSend++;
		  nrApplMsg[h]+=outcontrol[h];
		  nrFisicMsg[h]++;
		      
		  /*MARCO*/
		  cumulate_timeout[h] += maxTick[h] ;
		  applicative_msgs[h] += outcontrol[h]; 
		  physical_msgs[h]++ ;
		
		  global_physical_msgs++ ; 
		      

		  /*** SendNum 3 : SEND per TIMEOUT **********************************/
		    
		  MPI_Bsend(buf[h],nc1,MPI_CHAR,h,DATA_TAG,MPI_COMM_WORLD);



		  /*
		    FunReceiveMessages() ;
		    MPI_Wait(&ireq,&stsq) ;
		  */
		  outcontrol[h]= 0;
		  tickcontrol[h] = 0;
		}
	    }
	} /* END WHILE SCHEDULER */
    }   /*  END of WHILE */
	

  for(h = 0; h < size; h++) 
    {
      tickcontrol[h]++;
      if((outcontrol[h]>0) && (tickcontrol[h]>=maxTick[h]))
	{
	  nc1 = sizeof(int) + outcontrol[h]*sizeof(struct messaggio);
	  temporaneo2 = outcontrol[h];
		  
	  memcpy((char*)buf[h],(char*)&temporaneo2,sizeof(int));
	  /***************** Carlo ***************************************/
	  nTickSend++;
	  nrApplMsg[h]+=outcontrol[h];
	  nrFisicMsg[h]++;
		  
	  /* MARCO */
	  cumulate_timeout[h] +=  maxTick[h] ;
	  applicative_msgs[h] += outcontrol[h]; 
	  physical_msgs[h]++ ;
	  global_physical_msgs++ ; 
		  
		  
	  /*********** Sendnum 4 : SEND per TIMEOUT ************************/
	  MPI_Bsend(buf[h],nc1,MPI_CHAR,h,DATA_TAG,MPI_COMM_WORLD);

	  /*
	    FunReceiveMessages() ;
	    MPI_Wait(&ireq,&stsq);
	  */
	  outcontrol[h]= 0;
	  tickcontrol[h] = 0;
	}
    }   /* END FOR */

  loops++;
  pthread_exit(0);
}
#endif 

