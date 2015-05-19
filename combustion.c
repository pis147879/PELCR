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
//#include <mpi.h>
//
//#include "mydefs.h"
//#include "symbolic.h"
//#include "graph.h"
//#include "combustion.h"
////#include "parser.h"
//#include "io.h"
//#include "dvm.h"
//#include "distribution.h"
//#include "lambdastar.h"
//#include "buildgraph.h"
#include "var.h"

edge*InitReference(aux)
edge*aux;
{
    if(aux==NULL)aux= NewReference();
        aux->vector= NULL;
        aux->source= NULL;
        aux->sto=IN;
        aux->side= LEFT;
        aux->sign= PLUS;
        aux->rankpuit= rank;
        strcpy(aux->weight,"");
        
        return aux;
}


void WriteStats()
{
    /*
     printf("(%d) Loop:           %d\n(%d) Fires:          %d\n",rank,loops,rank,fires);
     printf("(%d) Trivial Fires   %d\n",rank,ones);
     printf("(%d) No Fires        %d\n",rank,bip);
     printf("(%d) Linearity       %d\n",rank,bip4);
     printf("(%d) Temperature     %d\n",rank,temp);
     */
    
    if((tempfile!=NULL)&&(!(bip2%FREQ)))
    {
        float now;
        times(&smtime);
        
        /* printf("%d\n",bip2);*/
        now = (smtime.tms_utime+smtime.tms_stime)/60.0;
        /*printf("%d\n",nhot);*/
        fprintf(firfile,"%f %ld\n",now,fires);
        fprintf(tempfile,"%f %d\n",now,temp);
        /*fprintf(coldfile,"%f %d\n",now,temporaneo1); */
        fprintf(hotfile,"%f %d\n",now,nhot);
        fprintf(trivfile,"%f %ld\n",now,ones);
        fprintf(nofile,"%f %ld\n",now,bip);
        fflush(firfile);
        fflush(hotfile);
        fflush(tempfile);
    }
}
/*END WRITE STATS*/


void NodeCombustion(n,polarity)
node*n;
int polarity;
{
	seminode v,vstar;
	edge*XI,*XJ;
	int counter;
	char a[MAXLENWEIGHT],b[MAXLENWEIGHT];
	char pos[MAXLENWEIGHT],neg[MAXLENWEIGHT];
	int proc,sto,outp;
	static edge*aux;
	struct messaggio m;
	
	aux = InitReference(aux);
	if(polarity==LEFT) {
		v= n->left;
		vstar= n->right;
	}
	else {
		v= n->right;
		vstar= n->left;
	}
	XJ= v.vector;
	XI= vstar.vector;
	counter= 0;
	while(XI!=NULL) {
		counter++;
		
		
		/*
		 printf("----------> test XJ->weight = %s\n",XJ->weight);
		 printf("---- %d -->",counter);fflush(stdout);
		 printf(" test XI->weight = %s\n",XI->weight);
		*/
		
		strcpy(a,XJ->weight);
		strcpy(b,XI->weight);
		
		TRACING {
		 fprintf(logfile,"(%d) FIRING: %d-th pair\n",rank,counter);
		 fprintf(logfile,"(%d) COMPUTE(",rank);
		 fprintf(logfile,"%s,%s",a,b);
		 fprintf(logfile,") Result:");
		}
		
		strcpy(pos,"");
		strcpy(neg,"");
		bip3++;
		
		/*
		 if ((bip3 % OBSERVATIONWINDOW)==0)     AggregationControl3();
		 */
		/*
		 if ((bip3 % OBSERVATIONWINDOW)==0)     AggregationControl2();
		 */
		locf_counter = 0 ;
		outp=product(a,b,pos,neg);
		//    DEBUG    printf("(%d) number of family reductions %ld (node families %d)\n", rank, locf_counter,n->families);
		if (locf_counter > n->families) n->families = locf_counter ;

		
			if(!outp) {
				TRACING fprintf(logfile,"NULL(%d)\n",outp);
				bip++;
			}
			else if(isone(pos)&&(XJ->sto==IN)) {
				TRACING {
					Print(G,incoming,bip3);
					fprintf(logfile,"(%d) verify one-optimization \n",rank);
					fprintf(logfile,"     OPT=%s STO=%d CHKONE=%d\n",pos,XJ->sto,isone(pos));
					fprintf(logfile,"     OPT=%s STO=%d CHKONE=%d\n",neg,XI->sto,isone(neg));
					fprintf(logfile,"OPT %s\n",neg);
					fprintf(logfile,"(%d)-",rank);
				}
				
				ones++;
				if((outp==40)||(outp==42)) {
					TRACING fprintf(logfile,"\n\t\t CAMBIO DI POLARITA' da %d ",XJ->side);
					XJ->side=!XJ->side;
					TRACING fprintf(logfile,"a %d\n\n",XJ->side);
					StoreMessage(&m,XJ,XI,neg,XI->sto,XI->side);
					XJ->side=!XJ->side;
				} else StoreMessage(&m,XJ,XI,neg,XI->sto,XI->side);
				PushMessage(&m);
				XI->sign= MINUS;
			}
			else if(isone(neg)&&(XI->sto==IN)) {
				//DEBUG Print(G,bip3);
				TRACING {
					fprintf(logfile,"OPT %s\n",pos);
					fprintf(logfile,"(%d)+",rank);
				}
				ones++;
				if((outp==40)||(outp==42)) {
					TRACING fprintf(logfile,"\n\t\t CAMBIO DI POLARITA' da %d",XI->side);
					XI->side=!XI->side;
					TRACING fprintf(logfile,"a %d\n\n",XI->side);
					StoreMessage(&m,XI,XJ,pos,XJ->sto,XJ->side);
					XI->side=!XI->side;
				}
				else StoreMessage(&m,XI,XJ,pos,XJ->sto,XJ->side);
				PushMessage(&m);
				XJ->sign= MINUS;
			}
			else {
				fires++;
				if((XI->sto==OUT)||((XJ->sto==OUT))) {
					proc = rank;
					sto= OUT;
					TRACING {
						fprintf(logfile,"%d%d%d+%s, %d%d%d-%s\n",sto,RIGHT,XI->side,pos,sto,LEFT,XJ->side,neg);
						fprintf(logfile," ISONE+ %d ISONE- %d\n",isone(pos),isone(neg));
						fprintf(logfile,"(%d) SENDCOLD",rank);
					}
				}
				else
				{
					//	        proc= LightProcess3(rank);
					proc = LightProcess7(rank);
					sto= IN;
					TRACING {
						fprintf(logfile,"%d%d%d+%s, %d%d%d-%s\n",sto,RIGHT,XI->side,pos,sto,LEFT,XJ->side,neg);
						fprintf(logfile," ISONE+ %d ISONE- %d\n",isone(pos),isone(neg));
						fprintf(logfile,"(%d) SENDHOT(%d,",rank,proc);
					}
				}
				TRACING fflush(logfile);
				/*DEBUG Print(G,bip3);*/
				SendCreateNewNode(proc,sto,aux);
				TRACING fprintf(logfile,"(%d)- sto:%d pol:%d side:%d\n",rank,sto,LEFT,XJ->side);
				StoreMessage(&m,XJ,aux,neg,sto,LEFT);
				PushMessage(&m);
				TRACING fprintf(logfile,"(%d)+ sto:%d pol:%d side:%d",rank,sto,RIGHT,XI->side);
				StoreMessage(&m,XI,aux,pos,sto,RIGHT);
				PushMessage(&m);
			}
		XI= XI->vector;
	}
/*  return;*/
}

void PushIncomingMessage(priority,m)
int priority;         /* buffer di archi a priorita' */
struct messaggio*m;
{
    struct mbuffer*l;
    /*char c;*/
	
    //l= &incoming[priority];
    /*
     printf("priority=%d\n",priority);fflush(stdout);
     read(0,&c,1);
     */
	
    l= &incoming[priority];
	
    if((l->last+1)%MAXPENDING==l->first)
    {
        printf("Exceeded size of incoming buffer\n Abort\n");
        exit(-1);
    }
    memcpy((char*)(&l->stack[l->last]),(char*)m,sizeof(struct messaggio));
    l->last= (l->last+1)%MAXPENDING;
	
    edges_counter++;
    return;
}

int EmptyBuffer(l)
struct mbuffer*l;
{
    if(l->first==l->last)
        return 1;
    else return 0;
}

void ShowMessage(m)
struct messaggio*m;
{
    fprintf(logfile,"(%d) %20s : %d[(%d).(%d)%p]->%d[(%d).(%d)%p]\n", rank, m->weight,
            m->vsource.side, m->vsource.rankpuit, m->vsource.creator, m->vsource.source,
            m->side, m->vtarget.rankpuit, m->vtarget.creator, m->vtarget.source);
    return ;
}


void StoreMessage(m,target,source,weight,storeclass,pol)
struct messaggio*m;
edge*target;
edge*source;
char*weight;
int storeclass;
int pol;
{
    m->tpy= ADD_TAG;
    m->temp= fra_hot;
    /*m->temp= nhot; temp;*/
    m->side= target->side;
    
    m->vsource.rankpuit= source->rankpuit;
    m->vsource.creator= source->creator;
    m->vsource.source= source->source;
    m->vsource.sto= storeclass;
    m->vsource.side= pol;
    
    m->vtarget.rankpuit= target->rankpuit;
    m->vtarget.creator= target->creator;
    m->vtarget.source= target->source;
    
    m->vtarget.sto= target->sto;
    if ((pol!=LEFT)&&(pol!=RIGHT))
    {
        /*printf("(%d) SIDE NON LEGALE\n");*/
        fflush(stdout);
    };
    m->vtarget.side= pol;
    
    if (strstr(weight,"X(3")) {
        char *atmp;
        int indice,i;
        atmp=strstr(weight,"X(3");
        indice=atoi(&atmp[4]);
        
        
        if (fcounter > -1)
        {
            m->funWhich=f[indice].which; // Va eliminata successivamente
            
            sprintf(&atmp[4],"%d)",f[indice].fun_id);
            m->funWait=f[indice].wait;
            for (i=0;i < f[indice].wait-1;i++) m->funArgs[i]=f[indice].s[i];
        }
        else
        {
            m->funWhich=f_db[indice].which;
            m->funWait=f_db[indice].wait;
        }
        
    }
    else if (strstr(weight,"X(2")) {
        char *atmp;
        int indice;
        atmp=strstr(weight,"X(2");
        indice=atoi(&atmp[4]);
        
        m->funWait=k[indice][1];
        m->funType=(int)k[indice][0];
    }
    strcpy(m->weight,weight);
    
    /*  return;*/
}

int BDump(struct mbuffer* b) {
    int i;
    
    i=b->first;
    TRACING  fprintf(logfile,"\nSTACK DUMP(%d-%d) VVVV: ",b->first,b->last);
    
    while(i!=b->last) {
        TRACING    fprintf(logfile,"%s ",(b->stack[i]).weight);
        i=(i+1)%MAXPENDING;
    }
    
    
    TRACING  fprintf(logfile,"\n");
    
    i=(b->last - b->first);
    if (i<0)  i=MAXPENDING+i;
    
    return i;
}

int BDumpS(b)
struct mbuffer*b;
{
    int i;
    
    //  i=b->first;
    i=(b->last - b->first);
    if (i<0)  i=MAXPENDING+i;
        
        return i;
}



void FunReceiveMessages() {
    
    int temporaneo1,i;
    
    char*position;
    int maxdim;
    
    int priority ;
    int ub;
    
    if(size!=1)
    {
        /*  int temporaneo1=0; */
        dataflag= 1;
        MPI_Iprobe(MPI_ANY_SOURCE,DATA_TAG,MPI_COMM_WORLD,&dataflag,&status);
        while(dataflag)
        {
            maxdim = sizeof(int) + (1 + MAXAWIN)*sizeof(struct messaggio);
            
			check_passed= FALSE;
            MPI_Recv(rbuf,maxdim,MPI_CHAR,MPI_ANY_SOURCE,DATA_TAG,MPI_COMM_WORLD,&status);
            position= rbuf + sizeof(int);
            
            memcpy((char*)&temporaneo1,(char*)rbuf,sizeof(int));
            aggregation_cumulate += temporaneo1;
            num_receives++;
            
            /*
             if((tempfile!=NULL)&&(!(num_receives%40)))
             {
             float now;
             times(&smtime);
             
             now = (smtime.tms_utime+smtime.tms_stime)/60.0;
             OUTPUT
             {
             fprintf(coldfile,"%f %d\n",now,temporaneo1);
             
             fprintf(anamfile,"%f %ld\n",now,
             aggregation_cumulate/num_receives);
             
             fflush(coldfile);
             fflush(anamfile);
             }
             }
             
             */
            
            
            
            /***************************************************
             questo pezzo di codice deve realizzare lo scheduling,
             l'aggregato e puntato da "position", va ora disaggregato
             inserendo nel buffer incoming ogni messaggio, in relazione alla
             euristica.
             ********************************/
            
            /*
             printf("(%d) RCVS %d, TOTAGG %d, AGGMSG %d\n",
             rank,num_receives,aggregation_cumulate,temporaneo1);
             */
            
#ifdef _DEBUG
            DEBUG_AGGREGATION {
                printf("(%d) incoming message (of size %d)\n",rank,temporaneo1);
                fflush(stdout);
            }
#endif
            /*  for(i= 0;i<AGGREGATIONWINDOW;i++)	   */
            
            for(i= 0;i<temporaneo1;i++)
            {
                
                ub = UpperBound((struct messaggio*)position);
                
                priority = floor((MINPRIORITY-1)*(1-(float)ub/(float)maxubound)) ;
                
                /*
                 ShowMessage((struct messaggio*)position);
                 */
                
#ifdef _DEBUG
                DEBUG_AGGREGATION {
                    printf("(%d) ub = %d maxub = %d - priority = %d\n",rank,ub,maxubound,priority);
                    fflush(stdout);
                }
#endif
                
                PushIncomingMessage(priority,(struct messaggio*)position);
                position = position + sizeof(struct messaggio);
            }
            MPI_Iprobe(MPI_ANY_SOURCE,DATA_TAG,MPI_COMM_WORLD,&dataflag,&status);
        }
    }
    
    buf_flush();
    
    contatore_combustioni_f = 0;
    schedule = 0 ;
    
    TRACING {
        //    printf("(%d) exiting receive phase (loops= %ld)\n",rank,loops);
        fflush(stdout);
    }
}


void FunInteraction() {
    int contatore_combustioni_f = 0;
    int nc1, temporaneo2;
    int h;
    int z;
    
    TRACING {  //printf("(%d) before while (edges = %d)\n",rank,edges_counter);
        fflush(stdout);
    };
    
    while( (edges_counter>0) && (contatore_combustioni_f < CHECKTICKS) )
    {
        
        lidle = idle;
        idle += loops-1;
        
        loops = 0;
        
        TRACING  	fprintf(logfile,"(%d) ...seeking a non-empty incoming buffer\n",rank);
        //      DEBUG	printf("(%d) *",rank);
        
        fra_hot = 0;
        for(z=0;z<MINPRIORITY;z++)  fra_hot += BDumpS(&incoming[z]);
        
        
        while (((schedule<MINPRIORITY)&&(!(nhot=BDump(&incoming[schedule])))))
        {
            TRACING fprintf(logfile,"(%d) BUFFER %d ",rank,schedule);
            schedule++;
            TRACING {
                //	    printf(".");
                fflush(stdout);
            };
        };
        
        //      DEBUG printf("\n*\n");
        TRACING {
            fflush(stdout);
            fprintf(logfile,"(%d) %d is the first non empty \n",rank,schedule);
            if (schedule >= MINPRIORITY) {
                printf("(%d) no work\n",rank);
                fflush(stdout);
            }
        }
        
        if (schedule < MINPRIORITY) {
            bip2++;
            contatore_combustioni_f++;
            TRACING	      fprintf(logfile,"(%d) POP(%d) \n", rank, schedule);
            PopMessage(&msg,&incoming[schedule]);
            
            TRACING {
                fprintf(logfile,"-> %s store:",msg.weight);
                fprintf(logfile,"%d sign:%d side:%d\n",msg.vsource.sto,msg.vsource.side,msg.side);
                //  printf("(%d) before switch\n",rank);
                fflush(stdout);
            }
            
            /* processing the message */
            switch(msg.tpy)
            {
                case EOT_TAG:
                {
                    node*sourceaddress;
                    edge e;
                    e= msg.vsource;
                    sourceaddress= StoreBookedAddress(e.creator,(long)e.source,e.sto);
                    if(msg.side==LEFT)
                        sourceaddress->left.eot++;
                    else
                        sourceaddress->right.eot++;
                    
                    if((e.sto!=OUT)&&(sourceaddress->left.eot==1)&&(sourceaddress->right.eot==1))
                    {
                        //sourceaddress=
                        SinkRemove(sourceaddress);
                    }
                }
                    break;
                    
                case ADD_TAG:
                {
                    node*targetaddress;
                    edge e;
                    
                    /* the first edge on the rank 0 process correspond to
                     the principal conclusion of the proof net, so it is the
                     starting point of the read_back procedure */
                    
                    
                    
                    targetaddress= StoreBookedAddress((msg.vtarget).creator,
                                                      (long)(msg.vtarget).source,msg.vtarget.sto);
                    if ((rank == 0) && (principal== 0)) {
                        principal = targetaddress ;
                        TRACING printf("(%d) ********** principal port address %p\n" , rank, principal);
                    };
                    
                    
                    e = msg.vsource;
                    
                    AddEdge(targetaddress,e.rankpuit,e.source,e.sto,msg.weight,e.creator,msg.side,e.side);
                    
                    //		DEBUG       printf("(%d) before combustion\n",rank);
                    //		DEBUG       fflush(stdout);
                    NodeCombustion(targetaddress,msg.side);
                    //		DEBUG       printf("(%d) after combustion\n",rank);
                    //		DEBUG       fflush(stdout);
                    /*		DEBUG Print(G,incoming,bip3);*/
                    /*  tim = time(&finaltime); */
                }
                    break;
            } /* END OF SWITCH */
            
            /*     OUTPUT WriteStats();  */
            
            //  DEBUG  printf("(%d) after switch\n",rank);
            //	  DEBUG  fflush(stdout);
            
            /*   schedule=0;
             while (((schedule<MINPRIORITY)&&(!(nhot=BDumpS(&incoming[schedule]))))) schedule++;
             schedule++;
             */
            
            tim = time(&finaltime);
            
            for(h = 0; h < size; h++)
            {
                
                /*	            if(outcontrol[h] > 0)  tickcontrol[h]++; */
                
                
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
                    
                    
                    
                    /* VAB */
                    
                    /*
                     current_rate = (float)outcontrol[h]/(float)maxTick[h];
                     if(last_rate[h] >= current_rate){
                     maxTick[h] -= TICK_MOVE;
                     }
                     else{
                     maxTick[h] += TICK_MOVE;
                     }
                     
                     if(maxTick[h] <= 0) maxTick[h] =1;
                     
                     last_rate[h] = current_rate;
                     
                     */
                    
                    current_rate = (float)outcontrol[h]/(float)maxTick[h];
                    
                    if( (1.1*last_rate[h]) <= current_rate){
                        maxTick[h] += TICK_MOVE;
                    }else{
                        if( (0.9*last_rate[h]) >= current_rate){
                            maxTick[h] -= TICK_MOVE;
                        }
                    }
                    
                    if(maxTick[h] <= 0) maxTick[h] =1;
                    
                    last_rate[h] = current_rate;
                    
                    
                    /*MARCO*/
                    cumulate_timeout[h] += maxTick[h] ;
                    applicative_msgs[h] += outcontrol[h];
                    physical_msgs[h]++ ;
                    global_physical_msgs++ ;
                    
                    /*** SendNum 3 : SEND per TIMEOUT **********************************/
#ifdef _DEBUG
                    DEBUG_DISTRIBUTION {
                        printf("(%d) before send to %d (messages %d - taglia %d) \n",rank,h,outcontrol[h],nc1);
                        fflush(stdout);
                    }
#endif
                    
                    /* tentative: force a receive step before starting a send */
                    /*FunReceiveMessages() ;*/
                    MPI_Bsend(buf[h],nc1,MPI_CHAR,h,DATA_TAG,MPI_COMM_WORLD);
                    
#ifdef _DEBUG
                    DEBUG_DISTRIBUTION {
                        printf("(%d) after send\n",rank);
                        fflush(stdout);
                    };
#endif
                    outcontrol[h]= 0;
                    tickcontrol[h] = 0;
                }
                /*
                 if(outcontrol[h] == 0) tickcontrol[h]=0;
                 */
                
            }/* end for */
        } /* END IF SCHEDULE */
    }     /*  END of WHILE */
    
    
    //  DEBUG  printf("(%d) after while\n",rank);
#ifdef _DEBUG
    fflush(stdout);
#endif
    
    for(h = 0; h < size; h++)
    {
        /* tickcontrol[h]++;
         if((outcontrol[h]>0) && (tickcontrol[h]>=maxTick[h])) */
        
        if(outcontrol[h]>0)
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
            
            
            
            /*********** SendNum 4 : SEND per TIMEOUT ************************/
            /*FunReceiveMessages() ;*/
            
            MPI_Bsend(buf[h],nc1,MPI_CHAR,h,DATA_TAG,MPI_COMM_WORLD);
            
            
            outcontrol[h]= 0;
            tickcontrol[h] = 0;
        }
    }
    
    loops++;
}

void ComputeResult() {
    printf("(%d) running...\n",rank);
    while((!end_computation)&&(loops<=maxloop)) {
        maxubound = 1;
        if (!THREAD) {
            FunReceiveMessages();
            FunInteraction();
        }
        if (THREAD) {
            exit(-1);
            /*
             pthread_t threadID1, threadID2;
             pthread_setconcurrency(2);
             pthread_create(&threadID1, NULL, ThreadReceiveMsgs, NULL);
             pthread_create(&threadID2, NULL, ThreadInteraction, NULL);
             
             pthread_join(threadID1, NULL);
             pthread_join(threadID2, NULL);
             */
        }
    }	;
    
    printf("(%d) ...ending\n",rank);
    printf("(%d) read back procedure call \n",rank);
    read_back(principal) ;
    OUTPUTFILE Print(G,incoming,1000000) ;
    printf("(%d) starting finalize \n",rank);
    
}



void PrintResult() {
    
#ifdef SOLARIS
    int lh;
    char*hostname[256],domainname[256];
    char*uid;
    time_t day;
    int h;
    
    uid= getlogin();
    lh= 30;
    gethostname(hostname,lh);
    lh= 60;
    getdomainname(domainname,lh);
    day= time(NULL);
    printf("user   : %s\n",uid);
    printf("host   : %s\n",hostname);
    printf("domain : %s\n",domainname);
    printf("date   : %s\n",ctime(&day));
    
#ifdef _DEBUG
    TRACING {
        fprintf(logfile,"user   : %s\n",uid);
        fprintf(logfile,"host   : %s\n",hostname);
        fprintf(logfile,"domain : %s\n",domainname);
        fprintf(logfile,"date   : %s\n",ctime(&day));
#endif
#endif
        
#ifdef LINUX
        int lh;
        char*hostname[256],domainname[256];
        char*uid;
        time_t day;
        int h;
        uid= getlogin();
        
        lh= 30;
        gethostname(hostname,(size_t)lh);
        lh= 60;
        getdomainname(domainname,lh);
        day= time(NULL);
        printf("user   : %s\n",uid);
        printf("host   : %s\n",hostname);
        printf("domain : %s\n",domainname);
        printf("date   : %s\n",ctime(&day));
        
        TRACING {
            fprintf(logfile,"user   : %s\n",uid);
            fprintf(logfile,"host   : %s\n",hostname);
            fprintf(logfile,"domain : %s\n",domainname);
            fprintf(logfile,"date   : %s\n",ctime(&day));
        }
#endif
        
        printf("(%d) elapsed time        :  %d\n",rank,((int)finaltime-(int)inittime));
        printf("(%d) final nodes         :  %d\n",rank,temp);
        printf("(%d) combusted nodes     :  %ld\n",rank,bip3);
        printf("(%d) fires               :  %ld\n",rank,fires);
		printf("(%d) trivial             :  %ld optimized\n",rank,ones);
        printf("(%d) family reductions   :  %ld\n",rank,fam_counter);
        printf("(%d) loops               :  %ld\n",rank,lastloop);
        printf("(%d) computing loops     :  %ld\n",rank,francesco);
        printf("(%d) idle loops          :  %ld (%ld)\n",rank,lidle,idle);
        printf("(%d) listen loops        :  %ld\n",rank,lbip2);
        printf("(%d) received messages   :  %ld\n",rank,num_receives);
        printf("(%d) unsuccessful tests  :  (eot %d), (add %d)\n",rank,leottest,laddtest);
        printf("(%d) dieflag             :  %d\n",rank,dieflag);
        
        /* print to stdout all main values on aggragation */
        
        printf(" (%d) AVERAGE AGGREGATION in RECEIVED MESSAGES: %06.1f\n",rank,
               (float)((float)aggregation_cumulate/(float)num_receives));
        printf(" (%d) TIMEOUT(max,min,delta) : %06d %06d %06.1f %06.1f\n",rank,MAXTICK,MINTICK,BETATICK,ALPHATICK);
        printf(" (%d) WINDOW(max,min,delta)  : %06d %06d %06.1f %06.1f\n",rank,MAXAWIN,MINAWIN,BETA,ALPHA);
        printf(" (%d) THRESHOLD(left,right)  : %06.1f %06.1f\n",rank,TRHLEFT,TRHRIGHT);
        printf(" (%d) FULLNESS COEFFICIENT   : %06.1f (full physical messages %ld)\n",rank,
               (float)(nFullSend/(float)global_physical_msgs),
               nFullSend);
        
        for(h = 0; h < size; h++) {
            if( h!=rank ) {
                printf("SEND TO %d : average timeout = %06.1f, average window = %06.1f ,",
                       h,(float)((float)cumulate_timeout[h]/(float)physical_msgs[h]),
                       (float)((float)applicative_msgs[h]/(float)physical_msgs[h]));
                printf("n. physical messages = %ld, n. applicative messages = %ld \n",
                       physical_msgs[h],applicative_msgs[h]);
            };
        };
        
        
        TRACING {
            fprintf(logfile,"AVERAGE AGGREGATION in RECEIVED MESSAGES: %f\n\n",
                    (float)((float)aggregation_cumulate/(float)num_receives));
            
            fprintf(logfile,"TIMEOUT(max,min,delta) : %d %d %f %f\n",MAXTICK,MINTICK,BETATICK,ALPHATICK);
            fprintf(logfile,"WINDOW(max,min,delta) : %d %d %f %f\n",MAXAWIN,MINAWIN,BETA,ALPHA);
            fprintf(logfile,"THRESHOLD(left,right) : %f %f\n",TRHLEFT,TRHRIGHT);
            fprintf(logfile,"FULLNESS COEFFICIENT : %f (full physical messages %ld)\n\n",
                    (float)(nFullSend/(float)global_physical_msgs),
                    nFullSend);
            
            for(h = 0; h < size; h++) {
                if( h!=rank ) {
                    fprintf(logfile,"SEND TO %d : average timeout = %f, average window = %f ,",
                            h,(float)((float)cumulate_timeout[h]/(float)physical_msgs[h]),
                            (float)((float)applicative_msgs[h]/(float)physical_msgs[h]));
                    fprintf(logfile,"n. physical messages = %ld, n. applicative messages = %ld \n",
                            physical_msgs[h],applicative_msgs[h]);
                }
            }
            
            fprintf(logfile,"(%d) elapsed time         :  %d\n",rank,((int)finaltime-(int)inittime));
            fprintf(logfile,"(%d) final nodes         :  %d\n",rank,temp);
            fprintf(logfile,"(%d) combusted nodes  :  %ld\n",rank,bip3);
            fprintf(logfile,"(%d) fires                :  %ld\n",rank,fires);
            fprintf(logfile,"(%d) loops                :  %ld\n",rank,lastloop);
            fprintf(logfile,"(%d) computing Loops      :  %ld\n",rank,francesco);
            fprintf(logfile,"(%d) idle Loops           :  %ld (%ld)\n",rank,lidle,idle);
            fprintf(logfile,"(%d) listen Loops         :  %ld\n",rank,lbip2);
            fprintf(logfile,"(%d) received messages    :  %d\n",rank,h);
            fprintf(logfile,"(%d) unsuccessful Tests   :  (eot %d), (add %d)\n\n",
                    rank,leottest,laddtest);
            
            fprintf(logfile,"(%d) logging out -> enter mpi_finalize\n",rank);
            fprintf(logfile,"termination task: rank %d\n",rank);
        }
    }
    
    void Finally() {
        
        printf("(%d) logging out -> enter mpi_finalize\n",rank);
        MPI_Finalize();
        printf("(%d) [OK] out from MPI\n",rank);
#ifdef MAIL
        if(rank==0) {
            system("cat run.*.log > run.log");
            {
                char command[100];
#ifdef LINUX
                sprintfx(command,"mail -s %s.np%d@%s marco@localhost < run.log",infile,size,hostname);
#endif
#ifdef SOLARIS
                sprintf(command,"mailx -s %s.np%d@%s marco@iac.rm.cnr.it < run.log",infile,size,hostname);
#endif
                system(command);
                printf("Sent Aknowledgement: %s\n",command);
            };
        };
#endif
        printf("EXIT ...\n\n");
    }
    
    
    /**************** Scheduler 6/10/2001 *****************************/
    int UpperBound(m)
    struct messaggio* m;
    {
        /* questa funzione calcola una approssimazione del carico esterno
         prodotto da un certo arco, il valore associato ad un arco e' dato
         dal numero di archi incidenti con sorgente esterna se l'arco stesso
         ha sorgente interna ed e' pari al doppio del numero di archi con
         sorgente esterna se la sorgente dell'arco in arrivo e' esterna */
        
        
        int euristic = 0 ;
        node *targetaddress=NULL;
        edge e;
        
        targetaddress = StoreBookedAddress((m->vtarget).creator, (long)(m->vtarget).source,m->vtarget.sto);
        e = m->vsource;
        
        TRACING {
            fprintf(logfile,"(%d) ****** target address = %ld [%p]\n", rank,(long int)targetaddress,targetaddress);
            
            //    printf("(%d) target address = %ld [%p]\n", rank,(long int)targetaddress,targetaddress);
            fflush(stdout);
            fflush(logfile);
        }
        if (m->tpy == EOT_TAG)
        {
            euristic = 0 ;
            /*printf("eot\n");*/
            /* poiche' il risultato di questa funzione e' normalizzato
             questo corrisponde ad assegnare la minima priorita',
             garantisce  l'ordine di processamento tra archi ed eot */
        }
        else
        {
            int coeff_locality ;
            
            coeff_locality  = !((e.rankpuit) == rank) ;
            
            /* l'arco da inserire ha sorgente locale */
            
            euristic = targetaddress->mu_remote + coeff_locality *  targetaddress->mu_local  ;
            /* calcolo dell'euristica  descritta in Pedicini - Quaglia 2001 */
            /* euristica = (2 r + l) se remoto ed = (r) se locale */
            
            if (coeff_locality)
            {
                
                targetaddress->mu_remote++;
            }
            
            else targetaddress->mu_local++ ;
            /* incremento delle misure sul nodo target, in questo modo */
            /* vengono conteggiati anche gli archi che sono nelle code e */
            /* che arriveranno prima dell'arco */
        };
        
        if (maxubound < euristic) maxubound = euristic ;
        
        return euristic ;
    }
    
