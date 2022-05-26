/* Input and output functions: graph dumping, readback, graph reader
 
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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "h/gml_parser.h"

#ifdef SOLARIS
	#include<sys/systeminfo.h>
#endif
#include <sys/types.h>
#include <sys/times.h>

#ifdef MPE_GRAPH
	#include<mpe.h>
#endif
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include "var.h"

unsigned int GML_line;
unsigned int GML_column;

extern int npozzi;


void OpenFileInitStruct() {
	int i,j;
	char namefile[MAXNAMELEN];
	char command[5000];
	/*************************/
	timestamp=0;
	outtimestamp=1;
	aggregation_cumulate = 0;
	num_receives = 0;
	global_physical_msgs=0;
	
	bip= 0;
	bip2= 0;
	bip3= 0;
	bip4= 0;
	fires= 0;
	loops= 0;
	temp=0;
	ones= 0;
	
	idle= 0;
	lidle= 0;
	lbip2= 0;
	check_passed= TRUE;
	/*
	 Finita una computazione e prima di iniziarne un'altra inizializzo
	 le tabelle delle funzioni e delle costanti.
	 Questo e' pero' causa di un problema:
	 se ho fatto una #def perdo le funzioni e le costanti.
	 Quindi, per ora, e' meglio non riinizializzare ma mettere
	 MAXNUMCOST e MAXFUNCTIONS alti e fregarsene.
  
  */
	lift=0;
	newval=0;
	
#ifdef _DEBUG
	TRACING printf("(%d) boot\n",rank);
#endif
	lightprocess=(rank+1)%size;
	
#ifdef _DEBUG
	TRACING printf("(%d) opening log files\n",rank);
#endif
	
	OUTPUT
	{
		/*  if(inflag==1) { */
		
		sprintf(namefile,"DAT/%s.%d.temp.dat",infile,rank);
		tempfile= fopen(namefile,"w");
		sprintf(namefile,"DAT/%s.%d.nofires.dat",infile,rank);
		nofile= fopen(namefile,"w");
		sprintf(namefile,"DAT/%s.%d.hot.dat",infile,rank);
		hotfile= fopen(namefile,"w");
		sprintf(namefile,"DAT/%s.%d.cold.dat",infile,rank);
		coldfile= fopen(namefile,"w");
		sprintf(namefile,"DAT/%s.%d.nmm.dat",infile,rank);
		anamfile= fopen(namefile,"w");
		for(i=0;i<size;i++) {
			sprintf(namefile,"DAT/%s.%d.%d.aws.dat",infile,rank,i);
			maxwinfile[i]= fopen(namefile,"w");
			sprintf(namefile,"DAT/%s.%d.%d.maw.dat",infile,rank,i);
			mawfile[i]= fopen(namefile,"w");
		}
		sprintf(namefile,"DAT/%s.%d.trivial.dat",infile,rank);
		trivfile= fopen(namefile,"w");
		
		sprintf(namefile,"DAT/%s.%d.fires.dat",infile,rank);
		firfile= fopen(namefile,"w");
		
		
		//    printf("<---------------------------------\n\n");
		sprintf(command,"cp run.%d.log runold.%d.log",rank,rank);
		system(command);
		
		sprintf(command,"\\rm -f run.%d.log",rank);
		system(command);
		sprintf(namefile,"run.%d.log",rank);
		logfile= fopen(namefile,"w");
#ifdef _DEBUG
		TRACING{
			printf("(%d) opened files:\n",rank);
			if (tempfile) printf("(%d) temp = [%p]\n",rank,(void*)tempfile);
			if (nofile)   printf("(%d) no   = [%p]\n",rank,(void*)nofile);
			if (hotfile)  printf("(%d) hot  = [%p]\n",rank,(void*)hotfile);
			if (coldfile) printf("(%d) cold = [%p]\n",rank,(void*)coldfile);
			if (trivfile) printf("(%d) triv = [%p]\n",rank,(void*)trivfile);
			if (firfile)  printf("(%d) fire = [%p]\n",rank,(void*)firfile);
			for(i=0;i<size;i++){
				if (maxwinfile[i])
					printf("(%d) maxaggregation[%d]=[%p]\n",rank,i,(void*)maxwinfile[i]);
				if (mawfile[i])
					printf("(%d) meanaggregation[%d]=[%p]\n",rank,i,(void*)mawfile[i]);
				if (logfile)  printf("(%d) log  = [%p]\n",rank,(void*)logfile);
			}
		}
#endif
	}
#ifdef _DEBUG
	DEBUG_IO {
		fprintf(logfile,"(%d) ok!\n",rank);
		fflush(logfile);
	}
#endif
	for(i= 0;i<=size;i++) {
		HashTable*p;
		
		p = (HashTable*)safemalloc(sizeof(HashTable));
		table_init(p);
		BookTable[i]= p;
		TableProcess[i]= 0;
		outcontrol[i]= 0;
		last_rate[i] = 0.0;
		tickcontrol[i]= 0;
		OutCounter[i]= 0;
		InCounter[i]= 0;
		for(j= 0;j<=size;j++) {
			OutTerminationStatus[i][j]= 0;
			InTerminationStatus[i][j]= 0;
		}
		nrFisicMsg[i]=0;
		nrApplMsg[i]=0;
		cumulate_timeout[i]=0;
		physical_msgs[i]=0;
		applicative_msgs[i]=0;
		aggregationRatio[i]=(float) AGGREGATIONWINDOW/MAXAWIN;
		aggregationWindow[i]=AGGREGATIONWINDOW;
		maxTick[i]=TICK;
	}
	number_of_processes= 0;
	
#ifdef _DEBUG
	TRACING fprintf(logfile,"(%d) process table initialized.\n",rank);
#endif
	
	G.cold= NULL;
	G.hot= NULL;
}

void SetDirectory(char *name) {
	strcpy(directoryname,name);
	printf("(%d) directory was set to %s\n",rank,directoryname);
}

void WriteStatus() {
	printf("(%d) working directory: %s\n",rank,directoryname);
	printf("(%d) trace: %d\n",rank,traceflag);
	printf("(%d) verbose: %d\n",rank,verflag);
	if (inflag) printf("(%d) input file: %s\n",rank,infile);
	if (outflag) printf("(%d) output file: %s\n",rank,outfile);
	printf("(%d) loops: ",rank) ;if (loops) printf("%ld\n",loops); else printf(" (%ld) unlimited\n",loops);
	printf("(%d) maxfires: ",rank) ;if (maxfires) printf("%ld\n",maxfires); else printf(" (%ld) unlimited\n",maxfires);
	printf("(%d) print every %ld steps\n",rank,prnsteps);
	
}

void WriteHelp() {
	printf("(%d) PELCR 10.0 help message:\n",rank);
	printf("       <lambda-term>    : evaluates the lambda term\n");
	printf("       #open <filename> : import the file (pex: default directory)\n");
	printf("       #trace           : enable tracing of execution in log files\n");
	printf("       #setloops <num>  : set the number of idle loops before ending\n");
	printf("       #setfires <num>   : set the maximal number of combustion steps before forcing to end\n");
}

void SetOutputFile(char *name)
{
	sprintf(outfile,"%s/%s",directoryname,name);
	printf("Output File Set: %s\n",outfile);
	outflag= 1;
}


// This is the code corresponding to the GML-parser

struct GML_pair*GML_parser(FILE*source,struct GML_stat*stat,int open)
{
	struct GML_token token;
	struct GML_pair*pair;
	struct GML_pair*list;
	struct GML_pair*tmp= NULL;
	struct GML_list_elem*tmp_elem;
	
	struct messaggio pozzi[MAXCUTNODES];
	//int npozzi = 0;
	
	assert(stat);
	
	pair= (struct GML_pair*)malloc(sizeof(struct GML_pair));
	list= pair;
	
	for(;;){
		token= GML_scanner(source);
		
		if(token.kind==GML_END){
			if(open){
				stat->err.err_num= GML_OPEN_BRACKET;
				stat->err.line= GML_line;
				stat->err.column= GML_column;
				free(pair);
				
				if(tmp==NULL){
					return NULL;
				}else{
					tmp->next= NULL;
					return list;
				}
			}
			
			break;
			
		}else if(token.kind==GML_R_BRACKET){
			if(!open){
				stat->err.err_num= GML_TOO_MANY_BRACKETS;
				stat->err.line= GML_line;
				stat->err.column= GML_column;
				free(pair);
				
				if(tmp==NULL){
					return NULL;
				}else{
					tmp->next= NULL;
					return list;
				}
			}
			
			break;
			
		}else if(token.kind==GML_ERROR){
			stat->err.err_num= token.value.err.err_num;
			stat->err.line= token.value.err.line;
			stat->err.column= token.value.err.column;
			free(pair);
			
			if(tmp==NULL){
				return NULL;
			}else{
				tmp->next= NULL;
				return list;
			}
			
		}else if(token.kind!=GML_KEY){
			stat->err.err_num= GML_SYNTAX;
			stat->err.line= GML_line;
			stat->err.column= GML_column;
			free(pair);
			
			if(token.kind==GML_STRING){
				free(token.value.string);
			}
			
			if(tmp==NULL){
				return NULL;
			}else{
				tmp->next= NULL;
				return list;
			}
		}
		
		if(!stat->key_list){
			stat->key_list= (struct GML_list_elem*)
			malloc(sizeof(struct GML_list_elem));
			stat->key_list->next= NULL;
			stat->key_list->key= token.value.string;
			pair->key= token.value.string;
			
		}else{
			tmp_elem= stat->key_list;
			
			while(tmp_elem){
				if(!strcmp(tmp_elem->key,token.value.string)){
					free(token.value.string);
					pair->key= tmp_elem->key;
					break;
				}
				
				tmp_elem= tmp_elem->next;
			}
			
			if(!tmp_elem){
				tmp_elem= (struct GML_list_elem*)
				malloc(sizeof(struct GML_list_elem));
				tmp_elem->next= stat->key_list;
				stat->key_list= tmp_elem;
				tmp_elem->key= token.value.string;
				pair->key= token.value.string;
			}
		}
		
		token= GML_scanner(source);
		
		switch(token.kind){
			case GML_INT:
				pair->kind= GML_INT;
				pair->value.integer= token.value.integer;
				break;
				
			case GML_DOUBLE:
				pair->kind= GML_DOUBLE;
				pair->value.floating= token.value.floating;
				break;
				
			case GML_STRING:
				pair->kind= GML_STRING;
				pair->value.string= token.value.string;
				break;
				
			case GML_L_BRACKET:
				pair->kind= GML_LIST;
				pair->value.list= GML_parser(source,stat,1);
				
				if(stat->err.err_num!=GML_OK){
					return list;
				}
				
				break;
				
			case GML_ERROR:
				stat->err.err_num= token.value.err.err_num;
				stat->err.line= token.value.err.line;
				stat->err.column= token.value.err.column;
				free(pair);
				
				if(tmp==NULL){
					return NULL;
				}else{
					tmp->next= NULL;
					return list;
				}
				
			default:
				stat->err.line= GML_line;
				stat->err.column= GML_column;
				stat->err.err_num= GML_SYNTAX;
				free(pair);
				
				if(tmp==NULL){
					return NULL;
				}else{
					tmp->next= NULL;
					return list;
				}
		}
		
		tmp= pair;
		if(!strcmp(pair->key,"edge"))
		{
			npozzi= GML_edge_det(pair->value.list,pozzi,npozzi);
		};
		pair= (struct GML_pair*)malloc(sizeof(struct GML_pair));
		tmp->next= pair;
	}
	
	stat->err.err_num= GML_OK;
	free(pair);
	{int i;
		if(npozzi>0)
		{
			printf("\n\n\n");
			for(i= 0;i<npozzi;i++){
	//			printf("pozzo n.%d nodo %d[%p]\n",i,(int)pozzi[i].vtarget.source,(void*((pozzi[i].vtarget).source)));
				PushMessage(&pozzi[i]);
				pozzi[i].side= !(pozzi[i].side);
				PushMessage(&pozzi[i]);
			};
			printf("\n\n\n");};
	}
	
	
	if(tmp==NULL){
		
		return NULL;
	}else{
		tmp->next= NULL;
		return list;
	}
}


void GML_free_list(struct GML_pair*list,struct GML_list_elem*keys){
	
	struct GML_pair*tmp= list;
	struct GML_list_elem*tmp_elem;
	
	while(keys){
		free(keys->key);
		tmp_elem= keys;
		keys= keys->next;
		free(tmp_elem);
	}
	
	while(list){
		
		switch(list->kind){
			case GML_LIST:
				GML_free_list(list->value.list,NULL);
				break;
				
			case GML_STRING:
				free(list->value.string);
				break;
				
			default:
				break;
		}
		
		tmp= list;
		list= tmp->next;
		free(tmp);
	}
}



void GML_print_list(struct GML_pair*list,int level){
	
	struct GML_pair*tmp= list;
	int i;
	
	while(tmp){
		
		for(i= 0;i<level;i++){
			printf("    ");
		}
		
		printf("*KEY* : %s",tmp->key);
		
		switch(tmp->kind){
			case GML_INT:
				printf("  *VALUE* (long) : %ld \n",tmp->value.integer);
				break;
				
			case GML_DOUBLE:
				printf("  *VALUE* (double) : %f \n",tmp->value.floating);
				break;
				
			case GML_STRING:
				printf("  *VALUE* (string) : %s \n",tmp->value.string);
				break;
				
			case GML_LIST:
				printf("  *VALUE* (list) : \n");
				GML_print_list(tmp->value.list,level+1);
				break;
				
			default:
				break;
		}
		
		tmp= tmp->next;
	}
}

int GML_edge_det(struct GML_pair*list, struct messaggio*pozzi, int nnpozzi)
{
	edge*vsource,*vtarget;
	int storeclass,polarity;
	
	struct messaggio m;
	
	char weight[MAXLENWEIGHT];
	struct GML_pair*tmp= list;
	
	printf("AddEdge(%ld,",tmp->value.integer);
	printf("%ld,%s,",tmp->next->value.integer,tmp->next->next->value.string);
	if(tmp->next->next->next!=NULL){
		printf("OUT)\n");
		storeclass= OUT;
	}else
	{
		printf("IN)\n");
		storeclass= IN;
	};
	
	{
		vsource= NewReference();
		vtarget= NewReference();
		
		vsource->creator= size;
		vsource->rankpuit= rank;
		vsource->source= (node*)tmp->value.integer;
		
		vtarget->creator= size;
		vtarget->rankpuit= rank;
		vtarget->source= (node*)tmp->next->value.integer;
		//		if(size==1) outbuffer= &incoming;
		
		
		if(tmp->next->next->value.string[1]=='+'){
			polarity= LEFT;}
		else polarity= RIGHT;
		
		if(tmp->next->next->value.string[2]=='+'){
			vtarget->side= LEFT;}
		else vtarget->side= RIGHT;
		
		if(tmp->next->next->value.string[0]=='+'){
			storeclass= IN;}
		else storeclass= OUT;
		
		
		vsource->sto= storeclass;
		
		printf("%s\n",(tmp->next->next->value.string)+3);
		strcpy(weight,(tmp->next->next->value.string)+3);
		
		if(tmp->next->next->next!=NULL){
			vtarget->sto= OUT;
			vsource->rankpuit= rank;
		}else vtarget->sto= IN;
		
		{
			
			
			StoreMessage(&m,vtarget,vsource,weight,storeclass,polarity);
			PushMessage(&m);
			if(vtarget->sto==IN)nnpozzi= SinkList(&m,pozzi,nnpozzi);
			
		};
	};
	return nnpozzi;
}




/*
 * ISO8859-1 coding of chars >= 160
 */

char* GML_table[] = {
	"&nbsp;",     /* 160 */
	"&iexcl;",
	"&cent;",
	"&pound;",
	"&curren;",
	"&yen;",
	"&brvbar;",
	"&sect;",
	"&uml;",
	"&copy;",
	"&ordf;",     /* 170 */
	"&laquo;",
	"&not;",
	"&shy;",
	"&reg;",
	"&macr;",
	"&deg;",
	"&plusmn;",
	"&sup2;",
	"&sup3;",     /* 180 */
	"&acute;",
	"&micro;",
	"&para;",
	"&middot;",
	"&cedil;",
	"&sup1;",
	"&ordm;",
	"&raquo;",
	"&frac14;",
	"&frac12;",
	"&frac34;",   /* 190 */
	"&iquest;",
	"&Agrave;",
	"&Aacute;",
	"&Acirc;",
	"&Atilde;",
	"&Auml;",
	"&Aring;",
	"&AElig;",
	"&Ccedil;",
	"&Egrave;",   /* 200 */
	"&Eacute;",
	"&Ecirc;",
	"&Euml;",
	"&Igrave;",
	"&Iacute;",
	"&Icirc;",
	"&Iuml;",
	"&ETH;",
	"&Ntilde;",
	"&Ograve;",   /* 210 */
	"&Oacute;",
	"&Ocirc;",
	"&Otilde;",
	"&Ouml;",
	"&times;",
	"&Oslash;",
	"&Ugrave;",
	"&Uacute;",
	"&Ucirc;",
	"&Uuml;",     /* 220 */
	"&Yacute;",
	"&THORN;",
	"&szlig;",
	"&agrave;",
	"&aacute;",
	"&acirc;",
	"&atilde;",
	"&auml;",
	"&aring;",
	"&aelig;",    /* 230 */
	"&ccedil;",
	"&egrave;",
	"&eacute;",
	"&ecirc;",
	"&euml;",
	"&igrave;",
	"&iacute;",
	"&icirc;",
	"&iuml;",
	"&eth;",      /* 240 */
	"&ntilde;",
	"&ograve;",
	"&oacute;",
	"&ocirc;",
	"&otilde;",
	"&ouml;",
	"&divide;",
	"&oslash;",
	"&ugrave;",
	"&uacute;",   /* 250 */
	"&ucirc;",
	"&uuml;",
	"&yacute;",
	"&thorn;",
	"&yuml;"
};




void GML_init () {
	
	GML_line = 1;
	GML_column = 1;
}



struct GML_token GML_scanner (FILE* source) {
	
	int cur_max_size = INITIAL_SIZE;
	static char buffer[INITIAL_SIZE];
	char* tmp = buffer;
	char* ret = tmp;
	struct GML_token token;
	int is_float = 0;
	unsigned int count = 0;
	int next;
	
	assert (source != NULL);
	
	/*
	 * eliminate preceeding white spaces
	 */
	
	do {
		next = fgetc (source);
		GML_column++;
		
		if (next == '\n') {
			GML_line++;
			GML_column = 1;
		} else if (next == EOF) {
			token.kind = GML_END;
			return token;
		}
	} while (isspace (next));
	
	if (isdigit (next) || next == '.' || next == '+' || next == '-') {
		
		/*
		 * floating point or integer
		 */
		
		do {
			if (count == INITIAL_SIZE - 1) {
				token.value.err.err_num = GML_TOO_MANY_DIGITS;
				token.value.err.line = GML_line;
				token.value.err.column = GML_column + count;
				token.kind = GML_ERROR;
				return token;
			}
			
			if (next == '.' || next == 'E') {
				is_float = 1;
			}
			
			buffer[count] = next;
			count++;
			next = fgetc (source);
		} while (!isspace(next));
		
		buffer[count] = 0;
		
		if (next == '\n') {
			GML_line++;
			GML_column = 1;
		} else {
			GML_column += count;
		}
		
		if (is_float) {
			token.value.floating = atof (tmp);
			token.kind = GML_DOUBLE;
		} else {
			token.value.integer = atol (tmp);
			token.kind = GML_INT;
		}
		
		return token;
		
	} else if (isalpha (next) || next == '_') {
		
		/*
		 * key
		 */
		
		do {
			if (count == cur_max_size - 1) {
				*tmp = 0;
				tmp =  (char*) malloc(2 * cur_max_size * sizeof (char));
				strcpy (tmp, ret);
				
				if (cur_max_size > INITIAL_SIZE) {
					free (ret);
				}
				
				ret = tmp;
				tmp += count;
				cur_max_size *= 2;
			}
			
			if (!isalnum (next) && next != '_') {
				token.value.err.err_num = GML_UNEXPECTED;
				token.value.err.line = GML_line;
				token.value.err.column = GML_column + count;
				token.kind = GML_ERROR;
				
				if (cur_max_size > INITIAL_SIZE) {
					free (ret);
				}
				
				return token;
			}
			
			*tmp++ = next;
			count++;
			next = fgetc (source);
		} while (!isspace (next) && next != EOF);
		
		if (next == '\n') {
			GML_line++;
			GML_column = 1;
		} else {
			GML_column += count;
		}
		
		*tmp = 0;
		token.kind = GML_KEY;
		token.value.string = (char*) malloc((count+1) * sizeof (char));
		strcpy (token.value.string, ret);
		
		if (cur_max_size > INITIAL_SIZE) {
			free (ret);
		}
		
		return token;
		
	} else {
		/*
		 * comments, brackets and strings
		 */
		
		switch (next) {
			case '#':
				do {
					next = fgetc (source);
				} while (next != '\n' && next != EOF);
				
				GML_line++;
				GML_column = 1;
				return GML_scanner (source);
				
			case '[':
				token.kind = GML_L_BRACKET;
				return token;
				
			case ']':
				token.kind = GML_R_BRACKET;
				return token;
				
			case '"':
				next = fgetc (source);
				GML_column++;
				
				while (next != '"') {
					
					if (count >= cur_max_size - 8) {
						*tmp = 0;
						tmp = (char*) malloc (2 * cur_max_size * sizeof(char));
						strcpy (tmp, ret);
		    
						if (cur_max_size > INITIAL_SIZE) {
							free (ret);
						}
						
						ret = tmp;
						tmp += count;
						cur_max_size *= 2;
					}
	    
					if (next >= 160) {
						*tmp = 0;
						strcat (tmp, GML_table[next - 160]);
						count += strlen (GML_table[next - 160]);
						tmp = ret + count;
					} else {
						*tmp++ = next;
						count++;
						GML_column++;
					}
					
					next = fgetc (source);
					
					if (next == EOF) {
						token.value.err.err_num = GML_PREMATURE_EOF;
						token.value.err.line = GML_line;
						token.value.err.column = GML_column + count;
						token.kind = GML_ERROR;
		    
						if (cur_max_size > INITIAL_SIZE) {
							free (ret);
						}
						
						return token;
					}
					
					if (next == '\n') {
						GML_line++;
						GML_column = 1;
					}
				}
				
				*tmp = 0;
				token.kind = GML_STRING;
				token.value.string = (char*) malloc((count+1) * sizeof (char));
				strcpy (token.value.string, ret);
				
				if (cur_max_size > INITIAL_SIZE) {
					free (ret);
				}
				
				return token;
				
			default:
				token.value.err.err_num = GML_UNEXPECTED;
				token.value.err.line = GML_line;
				token.value.err.column = GML_column;
				token.kind = GML_ERROR;
				return token;
		}
	}
}



int SinkList(m,l,length)
struct messaggio*m;
struct messaggio*l;
int length;
{
	int i;
	
	/* printf("dentro sink\n"); */
	
	for(i= 0;i<=length;i++)
		if(m->vtarget.source==l[i].vtarget.source){return length;}
	
	StoreMessage(&l[length],&(m->vtarget),&(m->vtarget),(char*)"*",m->vtarget.sto,m->side);
#ifdef _DEBUG
	DEBUG_MEM ShowMessage(&l[length]);
#endif
	l[length].tpy= EOT_TAG;
	length++;
#ifdef WDEBUG
	printf("(%d) number of cut-nodes: %3d",rank,length);
#endif
	return length;
}

