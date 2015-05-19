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
//#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
//#include <mpi.h>
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


termGraph *BuildF1Arg(char *namef, termGraph *arg) {
    termGraph *newT=(termGraph*)malloc(sizeof(termGraph));
    node *root=(node *)malloc(sizeof(node));
    node *tmp=(node *)malloc(sizeof(node));
    node *tmp2=(node *)malloc(sizeof(node));
    edge *tmpe;
    //=(edge *)malloc(sizeof(edge));
    char *c=(char*)malloc(20);
    int findexTmp=0;
    
    strcpy(newT->nameTerm,namef);
    strcat(newT->nameTerm,"(");
    strcat(newT->nameTerm,arg->nameTerm);
    strcat(newT->nameTerm,")");
    
    newT->listOfVar=arg->listOfVar;
    
    root->pro=(long)root;
    root->nextpuit=tmp;
    root->prevpuit=NULL;
    
    (root->left).eot=0;
    (root->right).eot=0;
    
    (root->left).weot=1;
    (root->right).weot=1;
    
    (root->left).length=0;
    (root->left).vector=NULL;
    
    (root->right).length=1;
    (root->right).vector=NULL;
    (root->right).vector=AddAnEdge((root->right).vector,"1",tmp,LEFT);
    
    
    tmp->pro=(long)tmp;
    tmp->prevpuit=root;
    tmp->nextpuit=tmp2;
    
    (tmp->left).eot=0;
    (tmp->right).eot=0;
    
    (tmp->left).weot=1;
    (tmp->right).weot=1;
    
    (tmp->left).length=0;
    (tmp->left).vector=NULL;
    
    (tmp->right).length=0;
    (tmp->right).vector=NULL;
    
    tmp2->pro=(long)tmp2;
    tmp2->prevpuit=tmp;
    tmp2->nextpuit=NULL;
    
    (tmp2->left).eot=0;
    (tmp2->right).eot=0;
    
    (tmp2->left).weot=0;
    (tmp2->right).weot=1;
    
    (tmp2->left).length=1;
    (tmp2->left).vector=NULL;
    
    findexTmp=AddEntryInf(namef);
    sprintf(c,"X(%d,%d)",FUNC,findexTmp);
    
    ((tmp2)->left).vector=AddAnEdge(((tmp2)->left).vector,c,tmp,RIGHT);
    
    (tmp2->right).length=0;
    (tmp2->right).vector=NULL;
    
    
    sprintf(c,"X(%d,%d)",ARGLAST,1);
    ((arg->root)->left).length++;
    ((arg->root)->left).vector=AddAnEdge(((arg->root)->left).vector,c,tmp2,RIGHT);
    
    for(tmpe=((arg->root)->left).vector;tmpe!=NULL; tmpe=tmpe->vector)
        tmpe->sto=IN;
    for(tmpe=((arg->root)->right).vector;tmpe!=NULL; tmpe=tmpe->vector)
        tmpe->sto=IN;
    ((arg->root)->left).eot=1;
    ((arg->root)->right).eot=1;
    
    tmp2->nextpuit=arg->root;
    (arg->root)->prevpuit=tmp2;
    
    newT->root=root;
    
    return newT;
}


termGraph *BuildXF1Arg(char *namef, termGraph *arg) {
    termGraph *newT=(termGraph*)malloc(sizeof(termGraph));
    node *root=(node *)malloc(sizeof(node));
    node *tmp=(node *)malloc(sizeof(node));
    node *tmp2=(node *)malloc(sizeof(node));
    //edge *tmpe ; //(edge *)malloc(sizeof(edge));
    char *c=(char*)malloc(20);
    int findexTmp=0;
    
    strcpy(newT->nameTerm,namef);
    strcat(newT->nameTerm,"(");
    strcat(newT->nameTerm,arg->nameTerm);
    strcat(newT->nameTerm,")");
    
    newT->listOfVar=arg->listOfVar;
    
    root->pro=(long)root;
    root->nextpuit=tmp;
    root->prevpuit=NULL;
    
    (root->left).eot=0;
    (root->right).eot=0;
    
    (root->left).weot=1;
    (root->right).weot=1;
    
    (root->left).length=0;
    (root->left).vector=NULL;
    
    (root->right).length=1;
    (root->right).vector=NULL;
    (root->right).vector=AddAnEdge((root->right).vector,"1",tmp,LEFT);
    
    
    tmp->pro=(long)tmp;
    tmp->prevpuit=root;
    tmp->nextpuit=tmp2;
    
    (tmp->left).eot=0;
    (tmp->right).eot=0;
    
    (tmp->left).weot=1;
    (tmp->right).weot=1;
    
    (tmp->left).length=0;
    (tmp->left).vector=NULL;
    
    (tmp->right).length=0;
    (tmp->right).vector=NULL;
    
    tmp2->pro=(long)tmp2;
    tmp2->prevpuit=tmp;
    tmp2->nextpuit=NULL;
    
    (tmp2->left).eot=0;
    (tmp2->right).eot=0;
    
    (tmp2->left).weot=0;
    (tmp2->right).weot=1;
    
    (tmp2->left).length=1;
    (tmp2->left).vector=NULL;
    
    findexTmp=XAddEntryInf(namef, 1);
    sprintf(c,"X(%d,%d)",FUNC,findexTmp);
    
    ((tmp2)->left).vector=AddAnEdge(((tmp2)->left).vector,c,tmp,RIGHT);
    
    (tmp2->right).length=0;
    (tmp2->right).vector=NULL;
    
    
    sprintf(c,"X(%d,%d)",ARGLAST,1);
    ((arg->root)->left).length++;
    ((arg->root)->left).vector=AddAnEdge(((arg->root)->left).vector,c,tmp2,RIGHT);
    
    {
        edge *tmpe;
        for(tmpe=((arg->root)->left).vector;tmpe!=NULL; tmpe=tmpe->vector) tmpe->sto=IN;
        for(tmpe=((arg->root)->right).vector;tmpe!=NULL; tmpe=tmpe->vector) tmpe->sto=IN;
    }
    ((arg->root)->left).eot=1;
    ((arg->root)->right).eot=1;
    
    tmp2->nextpuit=arg->root;
    (arg->root)->prevpuit=tmp2;
    
    newT->root=root;
    
    return newT;
}


termGraph *BuildF2Arg(char *namef, termGraph *arg1, termGraph *arg2) {
    
    termGraph *newT=(termGraph*)malloc(sizeof(termGraph));
    node *root=(node *)malloc(sizeof(node));
    node *tmp ; //=(node *)malloc(sizeof(node));
    node *tmp1=(node *)malloc(sizeof(node));
    node *tmp2=(node *)malloc(sizeof(node));
    edge *tmpe ;//(edge *)malloc(sizeof(edge));
    
    char *c=(char*)malloc(20);
    int findexTmp=0;
    
    strcpy(newT->nameTerm,namef);
    strcat(newT->nameTerm,"(");
    strcat(newT->nameTerm,arg1->nameTerm);
    strcat(newT->nameTerm,",");
    strcat(newT->nameTerm,arg2->nameTerm);
    strcat(newT->nameTerm,")");
    
    
    root->pro=(long)root;
    root->nextpuit=tmp1;
    root->prevpuit=NULL;
    
    
    (root->left).eot=0;
    (root->right).eot=0;
    
    (root->left).weot=1;
    (root->right).weot=1;
    
    (root->left).length=0;
    (root->left).vector=NULL;
    
    (root->right).length=1;
    (root->right).vector=NULL;
    (root->right).vector=AddAnEdge((root->right).vector,"1",tmp1, LEFT);
    
    
    tmp1->pro=(long)tmp1;
    tmp1->prevpuit=root;
    tmp1->nextpuit=tmp2;
    
    (tmp1->left).eot=0;
    (tmp1->right).eot=0;
    
    (tmp1->left).weot=1;
    (tmp1->right).weot=1;
    
    (tmp1->left).length=0;
    (tmp1->left).vector=NULL;
    (tmp1->right).length=0;
    (tmp1->right).vector=NULL;
    
    findexTmp=AddEntryInf(namef);
    sprintf(c,"X(%d,%d)",FUNC,findexTmp);
    
    tmp2->pro=(long)tmp2;
    tmp2->prevpuit=tmp1;
    tmp2->nextpuit=NULL;
    
    (tmp2->left).eot=0;
    (tmp2->right).eot=0;
    
    /******************* BOH ******************************/
    (tmp2->left).weot=1;
    (tmp2->right).weot=1;
    /*****************************************************/
    
    (tmp2->left).length=1;
    (tmp2->left).vector=NULL;
    (tmp2->left).vector=AddAnEdge((tmp2->left).vector,c,tmp1,RIGHT);
    /*((tmp2->left).vector)->sto=IN;*/
    
    (tmp2->right).length=0;
    (tmp2->right).vector=NULL;
    
    sprintf(c,"X(%d,%d)",ARG,1);
    
    
    /******************* BOH ******************************/
    /*((arg1->root)->left).weot++;
     ((arg1->root)->right).weot++;
     ((arg2->root)->left).weot++;
     ((arg2->root)->right).weot++;
     */
    /*****************************************************/
    
    ((arg1->root)->left).length++;
    ((arg1->root)->left).vector=AddAnEdge(((arg1->root)->left).vector,c,tmp2,RIGHT);
    for(tmpe=((arg1->root)->left).vector;tmpe!=NULL; tmpe=tmpe->vector)
        tmpe->sto=IN;
    for(tmpe=((arg1->root)->right).vector;tmpe!=NULL; tmpe=tmpe->vector)
        tmpe->sto=IN;
    
    /******************* BOH ******************************/
    ((arg1->root)->left).eot=1;
    ((arg1->root)->right).eot=1;
    /*****************************************************/
    
    sprintf(c,"X(%d,%d)",ARGLAST,2);
    ((arg2->root)->left).length++;
    ((arg2->root)->left).vector=AddAnEdge(((arg2->root)->left).vector,c,tmp2,LEFT);
    
    for(tmpe=((arg2->root)->left).vector;tmpe!=NULL; tmpe=tmpe->vector)
        tmpe->sto=IN;
    for(tmpe=((arg2->root)->right).vector;tmpe!=NULL; tmpe=tmpe->vector)
        tmpe->sto=IN;
    
    /******************* BOH ******************************/
    ((arg2->root)->left).eot=1;
    ((arg2->root)->right).eot=1;
    /*****************************************************/
    
    tmp2->nextpuit=arg1->root;
    (arg1->root)->prevpuit=tmp2;
    
    for(tmp=(arg1)->root;tmp->nextpuit!=NULL;tmp=tmp->nextpuit)
        ;
    tmp->nextpuit=arg2->root;
    (arg2->root)->prevpuit=tmp;
    newT->root=root;
    
    newT=MergeNodes(newT, arg1, arg2);
    return newT;
}

termGraph *BuildXF2Arg(char *namef, termGraph *arg1, termGraph *arg2) {
    
    termGraph *newT=(termGraph*)malloc(sizeof(termGraph));
    node *root=(node *)malloc(sizeof(node));
    node *tmp ;//=(node *)malloc(sizeof(node));
    node *tmp1=(node *)malloc(sizeof(node));
    node *tmp2=(node *)malloc(sizeof(node));
    edge *tmpe; //(edge *)malloc(sizeof(edge));
    
    char *c=(char*)malloc(20);
    int findexTmp=0;
    
    strcpy(newT->nameTerm,namef);
    strcat(newT->nameTerm,"(");
    strcat(newT->nameTerm,arg1->nameTerm);
    strcat(newT->nameTerm,",");
    strcat(newT->nameTerm,arg2->nameTerm);
    strcat(newT->nameTerm,")");
    
    
    root->pro=(long)root;
    root->nextpuit=tmp1;
    root->prevpuit=NULL;
    
    
    (root->left).eot=0;
    (root->right).eot=0;
    
    (root->left).weot=1;
    (root->right).weot=1;
    
    (root->left).length=0;
    (root->left).vector=NULL;
    
    (root->right).length=1;
    (root->right).vector=NULL;
    (root->right).vector=AddAnEdge((root->right).vector,"1",tmp1, LEFT);
    
    
    tmp1->pro=(long)tmp1;
    tmp1->prevpuit=root;
    tmp1->nextpuit=tmp2;
    
    (tmp1->left).eot=0;
    (tmp1->right).eot=0;
    
    (tmp1->left).weot=1;
    (tmp1->right).weot=1;
    
    (tmp1->left).length=0;
    (tmp1->left).vector=NULL;
    (tmp1->right).length=0;
    (tmp1->right).vector=NULL;
    
    findexTmp=XAddEntryInf(namef, 2);
    sprintf(c,"X(%d,%d)",FUNC,findexTmp);
    
    tmp2->pro=(long)tmp2;
    tmp2->prevpuit=tmp1;
    tmp2->nextpuit=NULL;
    
    (tmp2->left).eot=0;
    (tmp2->right).eot=0;
    
    /******************* BOH ******************************/
    (tmp2->left).weot=1;
    (tmp2->right).weot=1;
    /*****************************************************/
    
    (tmp2->left).length=1;
    (tmp2->left).vector=NULL;
    (tmp2->left).vector=AddAnEdge((tmp2->left).vector,c,tmp1,RIGHT);
    /*((tmp2->left).vector)->sto=IN;*/
    
    (tmp2->right).length=0;
    (tmp2->right).vector=NULL;
    
    sprintf(c,"X(%d,%d)",ARG,1);
    
    
    /******************* BOH ******************************/
    /*((arg1->root)->left).weot++;
     ((arg1->root)->right).weot++;
     ((arg2->root)->left).weot++;
     ((arg2->root)->right).weot++;
     */
    /*****************************************************/
    
    ((arg1->root)->left).length++;
    ((arg1->root)->left).vector=AddAnEdge(((arg1->root)->left).vector,c,tmp2,RIGHT);
    for(tmpe=((arg1->root)->left).vector;tmpe!=NULL; tmpe=tmpe->vector)
        tmpe->sto=IN;
    for(tmpe=((arg1->root)->right).vector;tmpe!=NULL; tmpe=tmpe->vector)
        tmpe->sto=IN;
    
    /******************* BOH ******************************/
    ((arg1->root)->left).eot=1;
    ((arg1->root)->right).eot=1;
    /*****************************************************/
    
    sprintf(c,"X(%d,%d)",ARGLAST,2);
    ((arg2->root)->left).length++;
    ((arg2->root)->left).vector=AddAnEdge(((arg2->root)->left).vector,c,tmp2,LEFT);
    
    for(tmpe=((arg2->root)->left).vector;tmpe!=NULL; tmpe=tmpe->vector) tmpe->sto=IN;
    for(tmpe=((arg2->root)->right).vector;tmpe!=NULL; tmpe=tmpe->vector) tmpe->sto=IN;
    
    /******************* BOH ******************************/
    ((arg2->root)->left).eot=1;
    ((arg2->root)->right).eot=1;
    /*****************************************************/
    
    tmp2->nextpuit=arg1->root;
    (arg1->root)->prevpuit=tmp2;
    
    for(tmp=(arg1)->root;tmp->nextpuit!=NULL;tmp=tmp->nextpuit) ;
    tmp->nextpuit=arg2->root;
    (arg2->root)->prevpuit=tmp;
    newT->root=root;
    
    newT=MergeNodes(newT, arg1, arg2);
    return newT;
}

void CallXF1ArgChar(char *namef, char *arg) {
    USERTYPE (*fun)();
    fun=dlsym(handle, namef);
    
    if ((error = dlerror()) != NULL)  {
        fputs(error, stderr);
        exit(1);
    };
    
    (fun)(arg);
    return;
}

void CallXF1ArgUser(char *namef, char *arg) {
    
    USERTYPE n;
    USERTYPE (*fun)();
    
    n=atoll(arg);
    fun=dlsym(handle, namef);
    
    if ((error = dlerror()) != NULL)  {
        fputs(error, stderr);
        exit(1);
    };
    
    (fun)(n);
}


int XAddEntryInf(char *funname, int narg) {
    
    findex++;
    
    f_db[findex].which=EXT;
    f_db[findex].type=NAT;
    f_db[findex].fun=dlsym(handle, funname);
    if ((error = dlerror()) != NULL)  {
        fputs(error, stderr);
        exit(1);
    };
    f_db[findex].narg=narg;
    f_db[findex].wait=1;
    /* ANTO */
    strcpy(f_db[findex].name, funname);
    /* ANTO */
    
    return findex;
}

int XAddFun(int f_db_id, USERTYPE wait) {
    
    int index;
    
    for (index=0; index<=fcounter; index++)
        if ((f[index].fun_id == f_db_id) && ((f[index].narg == 1) ))
            return index;
    
    
    fcounter++;
    
    f[fcounter].narg=f_db[f_db_id].narg;
    f[fcounter].type=f_db[f_db_id].type;
    f[fcounter].fun =f_db[f_db_id].fun;
    f[fcounter].fun_id=f_db_id;
    f[fcounter].wait=wait;
    
    f[fcounter].which=f_db[f_db_id].which;
    
    return fcounter;
}


int AddEntryInf(char *funname) {
    findex++;
    
    if ((strcmp(funname,"not")==0)||(strcmp(funname,"Not")==0) ) {
        f_db[findex].which=NOT;
        f_db[findex].type=BOOL;
        f_db[findex].fun=pelcr_not;
        f_db[findex].narg=1;
        f_db[findex].wait=1;
    }
    else if ((strcmp(funname,"succ")==0)||(strcmp(funname,"Succ")==0) ) {
        f_db[findex].which=SUCC;
        f_db[findex].type=NAT;
        f_db[findex].fun=pelcr_succ;
        f_db[findex].narg=1;
        f_db[findex].wait=1;
    }
    else if ((strcmp(funname,"pred")==0)||(strcmp(funname,"Pred")==0) ) {
        f_db[findex].which=PRED;
        f_db[findex].type=NAT;
        f_db[findex].fun=pelcr_pred;
        f_db[findex].narg=1;
        f_db[findex].wait=1;
    }
    else if ((strcmp(funname,"iszero")==0)||(strcmp(funname,"Iszero")==0) ) {
        f_db[findex].which=ISZERO;
        f_db[findex].type=BOOL;
        f_db[findex].fun=pelcr_iszero;
        f_db[findex].narg=1;
        f_db[findex].wait=1;
    }
    else if ((strcmp(funname,"add")==0)||(strcmp(funname,"Add")==0) ) {
        f_db[findex].which=ADD;
        f_db[findex].type=NAT;
        f_db[findex].fun=pelcr_add;
        f_db[findex].narg=2;
        f_db[findex].wait=1;
    }
    else if ((strcmp(funname,"mult")==0)||(strcmp(funname,"Mult")==0) ) {
        f_db[findex].which=MULT;
        f_db[findex].type=NAT;
        f_db[findex].fun=pelcr_mult;
        f_db[findex].narg=2;
        f_db[findex].wait=1;
    }
    else if ((strcmp(funname,"and")==0)||(strcmp(funname,"And")==0) ) {
        f_db[findex].which=AND;
        f_db[findex].type=BOOL;
        f_db[findex].fun=pelcr_and;
        f_db[findex].narg=2;
        f_db[findex].wait=1;
    }
    else if ((strcmp(funname,">")==0) ) {
        f_db[findex].which=GREATER;
        f_db[findex].type=BOOL;
        f_db[findex].fun=pelcr_great;
        f_db[findex].narg=2;
        f_db[findex].wait=1;
    }
    /* ANTO */
    strcpy(f_db[findex].name, funname);
    /* ANTO */
    
    return findex;
}

termGraph *BuildITE(termGraph *arg1, termGraph *arg2, termGraph *arg3) {
    /* eliminare qualche nodo ??? */
    termGraph *newT=(termGraph*)malloc(sizeof(termGraph));
    node *root=(node *)malloc(sizeof(node));
    node *tmp1=(node *)malloc(sizeof(node));
    node *tmp2=(node *)malloc(sizeof(node));
    //  node *tmp;//(node *)malloc(sizeof(node));
    //   edge *tmpe;//(edge *)malloc(sizeof(edge));
    char *c=(char*)malloc(20);
    
    strcpy(newT->nameTerm,"IF ");
    strcat(newT->nameTerm,arg1->nameTerm);
    strcat(newT->nameTerm," THEN ");
    strcat(newT->nameTerm,arg2->nameTerm);
    strcat(newT->nameTerm," ELSE ");
    strcat(newT->nameTerm,arg3->nameTerm);
    
    root->pro=(long)root;
    root->nextpuit=tmp1;
    root->prevpuit=NULL;
    (root->left).eot=0;
    (root->left).length=0;
    (root->left).vector=NULL;
    (root->right).eot=0;
    (root->right).length=1;
    (root->right).vector=NULL;
    
    (root->right).vector=AddAnEdge((root->right).vector,"1",tmp1,LEFT);
    
    tmp1->pro=(long)tmp1;
    tmp1->prevpuit=root;
    tmp1->nextpuit=tmp2;
    (tmp1->left).eot=0;
    (tmp1->left).length=0;
    (tmp1->left).vector=NULL;
    
    (tmp1->right).eot=0;
    (tmp1->right).length=0;
    (tmp1->right).vector=NULL;
    
    tmp2->pro=(long)tmp2;
    tmp2->prevpuit=tmp1;
    tmp2->nextpuit=NULL;
    (tmp2->left).eot=0;
    (tmp2->left).length=0;
    (tmp2->left).vector=NULL;
    
    (tmp2->right).eot=0;
    (tmp2->right).length=3;
    (tmp2->right).vector=NULL;
    
    /* controllare i LEFT RIGHT */
    sprintf(c,"X(%d,%d)",ITE,0);
    (tmp2->right).vector=AddAnEdge((tmp2->right).vector,c,tmp1,RIGHT);
    
    
    sprintf(c,"X(%d,%d)",ARG,1);
    ((arg1->root)->left).length++;
    ((arg1->root)->left).vector=AddAnEdge(((arg1->root)->left).vector,c,tmp2,LEFT);
    {
        edge *tmpe;
        for(tmpe=((arg1->root)->left).vector;tmpe!=NULL; tmpe=tmpe->vector)tmpe->sto=IN;
        for(tmpe=((arg1->root)->right).vector;tmpe!=NULL; tmpe=tmpe->vector)tmpe->sto=IN;
    }
    /******************* BOH ******************************/
    ((arg1->root)->left).eot=1;
    ((arg1->root)->right).eot=1;
    /*****************************************************/
    
    sprintf(c,"X(%d,%d)",ITE,1);
    (tmp2->right).vector=AddAnEdge((tmp2->right).vector,c,arg2->root,RIGHT);
    
    sprintf(c,"X(%d,%d)",ITE,2);
    (tmp2->right).vector=AddAnEdge((tmp2->right).vector,c,arg3->root,RIGHT);
    
    {
        edge *tmpe;
        for(tmpe=((arg1->root)->left).vector;tmpe!=NULL; tmpe=tmpe->vector)tmpe->sto=IN;
        for(tmpe=((arg1->root)->right).vector;tmpe!=NULL; tmpe=tmpe->vector) tmpe->sto=IN;
    }
    ((arg1->root)->left).eot=1;
    ((arg1->root)->right).eot=1;
    
    {edge *tmpe;
        for(tmpe=((arg2->root)->left).vector;tmpe!=NULL; tmpe=tmpe->vector)tmpe->sto=IN;
        for(tmpe=((arg2->root)->right).vector;tmpe!=NULL; tmpe=tmpe->vector)tmpe->sto=IN;
    }
    ((arg2->root)->left).eot=0;
    ((arg2->root)->right).eot=0;
    
    {
        edge *tmpe;
        for(tmpe=((arg3->root)->left).vector;tmpe!=NULL; tmpe=tmpe->vector)tmpe->sto=IN;
        for(tmpe=((arg3->root)->right).vector;tmpe!=NULL; tmpe=tmpe->vector) tmpe->sto=IN;
    }
    ((arg3->root)->left).eot=0;
    ((arg3->root)->right).eot=0;
    
    tmp2->nextpuit=arg1->root;
    (arg1->root)->prevpuit=tmp2;
    
    {
        node *tmp;
        for(tmp=(arg1)->root;tmp->nextpuit!=NULL;tmp=tmp->nextpuit);
        tmp->nextpuit=arg2->root;
        (arg2->root)->prevpuit=tmp;
        for(tmp=(arg2)->root;tmp->nextpuit!=NULL;tmp=tmp->nextpuit);
        tmp->nextpuit=arg3->root;
        (arg3->root)->prevpuit=tmp;
    }
    newT->root=root;
    
    /* controllare: credo che NON devo fondere le 2 listOfVar
     oppure occorre rinominare*/
    /*  newT->listOfVar=arg->listOfVar;*/
    newT=MergeNodes(newT, arg1, arg2);
    newT=MergeNodes(newT, arg1, arg3);
    return newT;
}

termGraph *BuildNumber(char *i) {
    termGraph *newT=(termGraph*)malloc(sizeof(termGraph));
    node *root=(node *)malloc(sizeof(node));
    node *tmp=(node *)malloc(sizeof(node));
    char *c=(char*)malloc(20);
    
    int kindexTmp=0;
    /* ANTO */
    
    /* Old
     int n;
     */
    
    USERTYPE n;
    
    
    /* Old
     n=atoi(i);
     */
    
    n=atoll(i);
    /* ANTO */
    
    strcpy(newT->nameTerm,i);
    
    newT->listOfVar=NULL;
    
    root->pro=(long)root;
    root->nextpuit=tmp;
    root->prevpuit=NULL;
    
    (root->left).eot=0;
    (root->right).eot=0;
    
    (root->left).weot=1;
    (root->right).weot=1;
    
    (root->left).length=0;
    (root->left).vector=NULL;
    
    (root->right).length=2;
    (root->right).vector=NULL;
    
    kindexTmp=AddEntryInk(NAT,n);
    sprintf(c,"X(%d,%d)X(%d,%d)",TYPE,0,KONST,kindexTmp);
    (root->right).vector=AddAnEdge((root->right).vector,c,tmp,LEFT);
    
    sprintf(c,"X(%d,%d)",TYPE,1);
    (root->right).vector=AddAnEdge((root->right).vector,c,tmp,RIGHT);
    
    tmp->pro=(long)tmp;
    tmp->prevpuit=root;
    tmp->nextpuit=NULL;
    
    (tmp->left).eot=0;
    (tmp->right).eot=0;
    
    (tmp->left).weot=1;
    (tmp->right).weot=1;
    
    (tmp->left).length=0;
    (tmp->left).vector=NULL;
    
    (tmp->right).length=0;
    (tmp->right).vector=NULL;
    
    newT->root=root;
    
    return newT;
}

termGraph *BuildBool(char *b) {
    termGraph *newT=(termGraph*)malloc(sizeof(termGraph));
    node *root=(node *)malloc(sizeof(node));
    node *tmp=(node *)malloc(sizeof(node));
    char *c=(char*)malloc(20);
    int n;
    int kindexTmp=0;
    
    strcpy(newT->nameTerm,b);
    
    newT->listOfVar=NULL;
    
    root->pro=(long)root;
    root->nextpuit=tmp;
    root->prevpuit=NULL;
    
    (root->left).eot=0;
    (root->right).eot=0;
    
    (root->left).weot=1;
    (root->right).weot=1;
    
    
    (root->left).length=0;
    (root->left).vector=NULL;
    
    (root->right).length=2;
    (root->right).vector=NULL;
    
    if( (strcmp(b,"TRUE")==0)||(strcmp(b,"true")==0)||(strcmp(b,"True")==0) )
        n=1;
    else n=0;
    kindexTmp=AddEntryInk(BOOL,n);
    
    sprintf(c,"X(%d,%d)X(%d,%d)",TYPE,2,KONST,kindexTmp);
    (root->right).vector=AddAnEdge((root->right).vector,c,tmp,LEFT);
    
    sprintf(c,"X(%d,%d)",TYPE,3);
    (root->right).vector=AddAnEdge((root->right).vector,c,tmp,RIGHT);
    
    tmp->pro=(long)tmp;
    tmp->prevpuit=root;
    tmp->nextpuit=NULL;
    
    
    (tmp->left).eot=0;
    (tmp->right).eot=0;
    
    (tmp->left).weot=1;
    (tmp->right).weot=1;
    
    (tmp->left).length=0;
    (tmp->left).vector=NULL;
    (tmp->right).length=0;
    (tmp->right).vector=NULL;
    
    newT->root=root;
    
    return newT;
}

/* ANTO */
int AddEntryInk(int type, USERTYPE val) {
    /* ANTO */
    
    kindex++;
    if (kindex>MAXNUMCOST) {
        printf("\nTabella delle costanti piena\n");
        exit(0);
    }
    k[kindex][0]=type;
    k[kindex][1]=val;
    k[kindex][2]++;
    
    return kindex;
}

void SetNextKIndex(void)
{
    int index_reduced=0;
    
    while (k[kindex][2] != 0)
    {
        kindex++;
        if (kindex >= MAXNUMCOST)
        {
            if(index_reduced)
            {
                printf("\n>SetNextKIndex: Tabella delle costanti piena!!!\n");
                exit(0);
            }
            else
            {
                index_reduced=1;
                kindex=kindex % MAXNUMCOST;
            }
        }
    }
    return;
}

void MostraTabelle() {
    int indice=0, j;
    
    printf("\n(%d) show the local symbol table post term evaluation", rank);
    printf("\n(%d) \tkindex  =%3d values-table is",rank,kindex);
    if (kindex>0)
    {
        printf("\n");
        for(indice=0;indice<=kindex;indice++) {
            printf("\t\t%lld\t%lld\t%lld\n",k[indice][0],k[indice][1],k[indice][2]); }
    } else
    {
        printf(" empty\n");
    }
    
    
    printf("(%d) \tfcounter=%3d functions-table is",rank,findex);
    if (findex>0)
    {
        printf("\n");
        for(indice=0;indice<=fcounter;indice++)
        {
            printf("\t\t *%lld --> %s type=%d,narg=%d wait_vct= ",(USERTYPE)f[indice].fun,f[indice].name,f[indice].type,f[indice].narg);
            for(j=0;j<f[indice].narg;j++)
                printf(" %lld ",f[indice].s[j]);
            printf(" \n");
        }
    } else { printf(" empty\n"); }
    
    printf("(%d) \tfindex  =%3d fdb-table f_db is",rank,findex);
    if (findex>0) {
        printf("\n");
        for(indice=0;indice<=findex;indice++) {
            printf("\t\t*%lld --> %s type=%d,narg=%d wait_vct= ",(USERTYPE)f_db[indice].fun,f_db[indice].name,f_db[indice].type,f_db[indice].narg);
            for(j=0;j<f_db[indice].narg;j++)
                printf(" %lld ",f_db[indice].s[j]);
            printf(" \n");
        };} else { printf(" empty\n"); }
    
    printf("(%d) \tterm table is:\n",rank);
    for(indice=0;indice<4;indice++) {
        printf("\t\t%d\t%d\n",t[indice][0],t[indice][1]);
    }
    
}

termGraph *BuildVar(char *n) {
    char tmpnamevar[10];
    termGraph *termine=(termGraph*)malloc(sizeof(termGraph));
    
    node *root=(node *)malloc(sizeof(node));
    node *tmp1=(node *)malloc(sizeof(node));
    node *tmp2=(node *)malloc(sizeof(node));
    
    table *tab=(table*)malloc(sizeof(table));
    
    strcpy(termine->nameTerm,n);
    
    root->pro=(long)root;
    tmp1->pro=(long)tmp1;
    tmp2->pro=(long)tmp2;
    
    root->prevpuit=NULL;
    root->nextpuit=tmp1;
    tmp1->prevpuit=root;
    tmp1->nextpuit=tmp2;
    tmp2->prevpuit=tmp1;
    tmp2->nextpuit=NULL;
    
    (root->left).eot=0;
    (root->left).weot=1;
    
    (root->left).length=0;
    (root->left).vector=NULL;
    (root->right).eot=0;
    (root->right).weot=1;
    (root->right).length=1;
    (root->right).vector=NULL;
    (root->right).vector=AddAnEdge((root->right).vector,"1",tmp2,LEFT);
    
    (tmp1->left).eot=0;
    (tmp1->left).weot=1;
    (tmp1->left).length=0;
    (tmp1->left).vector=NULL;
    (tmp1->right).eot=0;
    (tmp1->right).weot=1;
    (tmp1->right).length=1;
    (tmp1->right).vector=NULL;
    sprintf(tmpnamevar,"%s%d%s","w(",varname++,",0)");
    (tmp1->right).vector=AddAnEdge((tmp1->right).vector,tmpnamevar,tmp2,RIGHT);
    
    (tmp2->left).eot=0;
    (tmp2->left).weot=1;
    (tmp2->left).length=0;
    (tmp2->left).vector=NULL;
    (tmp2->right).eot=0;
    (tmp2->right).weot=1;
    (tmp2->right).length=0;
    (tmp2->right).vector=NULL;
    
    termine->root=root;
    tab->name=(char*)malloc(strlen(n)+1);
    strcpy(tab->name,n);
    tab->next=NULL;
    tab->ptrNode=tmp1;
    
    termine->listOfVar=tab;
    /*MostraGrafo(termine);*/
    return termine;
}

edge *AddAnEdge(edge *old,char *peso,node *sorgente, int lato) {
    edge *new=(edge*)malloc(sizeof(edge));
    
    strcpy(new->weight,peso);
    new->sto=OUT;
    new->source=sorgente;
    new->side=lato;
    new->sign=PLUS;
    new->vector=old;
    return new;
}

termGraph *BuildLet(char *var, termGraph *t1, termGraph *t2) {
    termGraph *t;
    t=BuildApplication(BuildLambda(var,t2),t1);
    
    return t;
}

termGraph *BuildLambda(char *var, termGraph *ter) {
    table *varList=ter->listOfVar;
    short find=0;
    edge *tmp;
    char tmpch[160000];
    
    strcpy(tmpch,"\\");
    strcat(tmpch,var);
    strcat(tmpch,".");
    strcat(tmpch,ter->nameTerm);
    strcpy(ter->nameTerm,tmpch);
    while(varList!=NULL && (!find)) {
        if (strcmp(varList->name,var)==0) find=1;
        else varList=varList->next;
        
    }
    
    
    P_Q(ter->root,"q");
    if (find) {
        P_Q(varList->ptrNode,"p");
        
        for (tmp=((ter->root)->right).vector; tmp->vector!=NULL; tmp=tmp->vector)
            ;
        tmp->vector=((varList->ptrNode)->right).vector;
        (ter->root)->right.length+=((varList->ptrNode)->right).length;
        
        /* Chieder a Marco *******************************************************
         
         for (tmp=((ter->root)->left).vector;tmp->vector!=NULL;tmp=tmp->vector)
         ;
         tmp->next=((varList->ptrNode)->left).vector;
         
         ****************************************************************************/
        fprintf(logfile,"\n%s \n",ter->nameTerm);
        
        FreeNode(varList->ptrNode);
        ter->listOfVar=RemoveFromList(ter->listOfVar,var);
    }
    return ter;
}

termGraph *BuildRec(char *xname,termGraph *M){
    
    termGraph *newT=malloc(sizeof(termGraph)),*x;
    node *p;
    table *t;
    int find=0,i=0;
    seminode sn;
    
    strcpy(newT->nameTerm,"rec_");
    strcat(newT->nameTerm,xname);
    strcat(newT->nameTerm,"_in_");
    strcat(newT->nameTerm,M->nameTerm);
    TRACING printf("\n nome del termine %s\n", newT->nameTerm);
    
    Lift(M);
    lift++;
    
    /*il ciclo cerca la variabile x tra le variabili di M*/
    for(t=M->listOfVar;t!=NULL && !find;t=t->next){
        if (strcmp(t->name,xname)==0) {
            find=1;
            /*quando trova la var x sposta la root del grafo*/
            p=M->root;
            M->root=p->nextpuit;
            /*inverte i seminodi di p*/
            sn=p->left;
            p->left=p->right;
            p->right=sn;
            /*cut tra le due concl di M*//*rimane la var libera che ora pero' e' un cut*/
            MeltNodes(t->ptrNode, p);
        }
    }
    
    
    TRACING {
        printf("\n root del grafo M %p\n",M->root);
        printf("\nGRAFO M dopo meltnode***********************\n");
        MostraGrafo(M);
    }
    
    if (!find)
    {   fprintf(logfile,"\nATTENZIONE!!!!!!!!!!Il termine M non contiene la var libera x\n");
        newT=M;
    }
    else
    {
        
        x=BuildVar(xname);
        TRACING {
            printf("\nGRAFO X***********************\n");
            MostraGrafo(x);
        }
        /*at the beginning newt is x*/
        newT=x;
        
        
        /*contrazione su x*/
        newT=MergeNodes(newT, M, x);
        /*metto i nodi di M in newT*/
        for (p=newT->root; p->nextpuit!=NULL; p=p->nextpuit);
        M->root->prevpuit=p;
        p->nextpuit=M->root;
        
        TRACING {
            printf("\nGRAFO REC***********************\n");
            MostraGrafo(newT);
            for (p=newT->root; p!=NULL; p=p->nextpuit)
                printf("\nnodo %d del grafo finale= %p\n",i++,p);
        }
    }
    
    return newT;
}




table *RemoveFromList(table *list,char *var) {
    table *tmp=list;
    table *tmpPrev;
    
    if (strcmp(tmp->name,var)==0) {
        list=list->next;
        free(tmp);
        return list;
    }
    tmp=list->next;
    tmpPrev=list;
    while(tmp!=NULL) {
        if (strcmp(tmp->name,var)==0) {
            tmpPrev->next=tmp->next;
            return list;
        }
        tmp=tmp->next;
        tmpPrev=tmpPrev->next;
    }
    printf("ERRORE : Qui non dovrei mai esserci !!!");
    return NULL;
}


termGraph *BuildApplication(termGraph *t1,termGraph *t2) {
    termGraph *newTerm=(termGraph*)malloc(sizeof(termGraph));
    edge *tmpe;
    node *newNode=(node*)malloc(sizeof(node));
    node *tmp2;
    
    strcpy(newTerm->nameTerm,"(");
    strcat(newTerm->nameTerm,t1->nameTerm);
    strcat(newTerm->nameTerm,")");
    strcat(newTerm->nameTerm,t2->nameTerm);
    
    newNode->pro=(long)newNode;
    (newNode->right).vector=NULL;
    (newNode->left).vector=NULL;
    (newNode->right).eot=0;
    (newNode->right).weot=1;
    (newNode->left).eot=0;
    (newNode->left).weot=1;
    (newNode->right).length=0;
    (newNode->left).length=0;
    Lift(t2);
    lift++;
    P_Q(t2->root,"p");
    
    (t1->root)->left=(t2->root)->right;
    ((t1->root)->left).length++;
    ((t1->root)->left).vector=AddAnEdge(((t1->root)->left).vector,"q",newNode,LEFT);
    
    for(tmpe=((t1->root)->left).vector;tmpe!=NULL; tmpe=tmpe->vector)
        tmpe->sto=IN;
    for(tmpe=((t1->root)->right).vector;tmpe!=NULL; tmpe=tmpe->vector)
        tmpe->sto=IN;
    ((t1->root)->left).eot=1;
    ((t1->root)->right).eot=1;
    
    (newTerm->root)=(node*)malloc(sizeof(node));
    
    (newTerm->root)->pro=(long)(newTerm->root);
    ((newTerm->root)->left).eot=0;
    ((newTerm->root)->left).weot=1;
    ((newTerm->root)->right).eot=0;
    ((newTerm->root)->right).weot=1;
    ((newTerm->root)->left).vector=0;
    ((newTerm->root)->right).vector=0;
    ((newTerm->root)->right).length=0;
    
    ((newTerm->root)->right).length=1;
    
    ((newTerm->root)->right).vector=AddAnEdge(((newTerm->root)->right).vector,"1",newNode,RIGHT);
    
    (newTerm->root)->prevpuit=NULL;
    (newTerm->root)->nextpuit=newNode;
    newNode->prevpuit=newTerm->root;
    newNode->nextpuit=t1->root;
    (t1->root)->prevpuit=newNode;
    
    for(tmp2=t1->root; tmp2->nextpuit!=NULL; tmp2=tmp2->nextpuit)
        ;
    tmp2->nextpuit=(t2->root)->nextpuit;
    ((t2->root)->nextpuit)->prevpuit=tmp2;
    
    newTerm=MergeNodes(newTerm, t1, t2);
    free(t2->root);
    fprintf(logfile,"\n%s\n",newTerm->nameTerm);
    return newTerm;
}

void Lift(termGraph *t) {
    {
        node *tmp;
        tmp=t->root;
        for (;tmp!=NULL;tmp=tmp->nextpuit) {
            LiftEdges( (tmp->left).vector );
            LiftEdges( (tmp->right).vector );
        }
    }
    {
        node *tmp;
        tmp=(t->root)->prevpuit;
        for (;tmp!=NULL;tmp=tmp->prevpuit) {
            LiftEdges( (tmp->left).vector );
            LiftEdges( (tmp->right).vector );
        }
    }
}

void LiftEdges(edge *e) {
    edge *tmp;
    char new[MAXLENWEIGHT];
    char tmpc[MAXLENWEIGHT];
    int numero;
    int numero2;
    
    for(tmp=e;tmp!=NULL;tmp=tmp->vector) {
        //  DEBUG {
        //    printf("-*-------> %p\n",tmp);
        //    printf("-*-------> %s\n",tmp->weight);
        //  }
        if (strcmp(tmp->weight,"1")==0) ;
        else if (strncmp(tmp->weight,"!",1)==0)  {
            strcpy(tmpc,"");
            sscanf(tmp->weight,"!%d%s",&numero,tmpc);
            numero++;
            
            sprintf(new,"!%d%s",numero,tmpc);
            strcpy(tmp->weight,new);
        }
        else if (strncmp(tmp->weight,"w",1)==0) {
            strcpy(tmpc,"");
            sscanf(tmp->weight,"w(%d,%d)%s",&numero2,&numero,tmpc);
            numero++;
            
            sprintf(tmp->weight,"w(%d,%d)%s",numero2, numero, tmpc);
            
            //  DEBUG printf("www> %s\n",tmp->weight);
        }
        else {
            strcpy(new,"!1");
            strcat(new,tmp->weight);
            strcpy(tmp->weight,new);
        }
        //  DEBUG printf("---> %s\n",tmp->weight);
    }
}

void P_Q(node *n, char *ch) {
    edge *tmp;
    char s[MAXLENWEIGHT];
    
    //  tmp=n->right.vector;
    for(tmp=n->right.vector;tmp!=NULL;tmp=tmp->vector) {
        if (strcmp(tmp->weight,"1")==0)
            strcpy(tmp->weight,ch);
        else {
            strcpy(s,ch);
            strcat(s,tmp->weight);
            strcpy(tmp->weight,s);
        }
    }
    for(tmp=n->left.vector;tmp!=NULL;tmp=tmp->vector) {
        if (strcmp(tmp->weight,"1")==0)
            strcpy(tmp->weight,ch);
        else {
            strcpy(s,ch);
            strcat(s,tmp->weight);
            strcpy(tmp->weight,s);
        }
    }
}

void FreeNode(node *n) {
    /* non funziona se il nodo e' l'estremo della lista */
    if (n->nextpuit!=NULL && n->prevpuit!=NULL){
        (n->nextpuit)->prevpuit=n->prevpuit;
        (n->prevpuit)->nextpuit=n->nextpuit;
        free(n);
    }
    else{
        TRACING printf("\n il nodo PUNTA A NULL!!!\n");
        if (n->nextpuit==NULL)
        {
            if (n->prevpuit!=NULL)
            {
                (n->prevpuit)->nextpuit=NULL;
            }
        }
        else
        {
            (n->nextpuit)->prevpuit=NULL;
        }
        free(n);
        TRACING printf("exit of freenode <<<<<<<<<<<<<<<\n");
    }
    
}
/* void FreeNode(node *n) { */
/*  if (n->nextpuit==NULL){ */
/*  (n->prevpuit)->nextpuit=NULL; */
/*        free(n); */
/*  } */
/*  else if (n->prevpuit==NULL){ */
/* (n->nextpuit)->prevpuit=NULL; */
/*        free(n); */
/*  } */
/*  else { */
/*       (n->nextpuit)->prevpuit=n->prevpuit;  */
/*       (n->prevpuit)->nextpuit=n->nextpuit;  */
/*       free(n); */
/*  } */
/* }*/
termGraph *MergeNodes(termGraph *new, termGraph *t1, termGraph *t2) {
    table *newLast=new->listOfVar; /*che punta sempre all'ultimo creato*/
    table *newTmp=NULL;
    
    table *list1=t1->listOfVar;
    short find,primo=1;
    
    if (list1==NULL) new->listOfVar=t2->listOfVar;
    else {
        while (list1 !=NULL) {
            table *list2=t2->listOfVar;
            find=0;
            while ( (list2 != NULL) && (!find) ) {
                /*1.se le var sono uguali */
                if (strcmp(list1->name,list2->name)==0) {
                    find=1;
                    /* a. Fare un solo nodo dei due nodi*/
                    MeltNodes(list1->ptrNode,list2->ptrNode);
                    
                    /* b. Metto la variabile in listOfVar di new
                     */
                    
                    newTmp=(table*)malloc(sizeof(table));
                    newTmp->name=(char*)malloc(strlen(list1->name)+1);
                    strcpy(newTmp->name,list1->name);
                    newTmp->ptrNode=list1->ptrNode;
                    newTmp->next=NULL;
                    if (primo) {
                        primo=0;
                        new->listOfVar=newTmp;
                        //	    newLast=new->listOfVar;
                    }
                    else newLast->next=newTmp;
                    newLast=newTmp;
                    t2->listOfVar=RemoveFromList(t2->listOfVar,list2->name);
                }
                list2=list2->next;
            }
            /*
             end of while (list2 !=NULL)
             */
            
            /* 2. Se non uguali, la metto in listOfVar di new
             */
            if (!find) {
                newTmp=(table*)malloc(sizeof(table));
                newTmp->name=(char*)malloc(strlen(list1->name)+1);
                strcpy(newTmp->name,list1->name);
                newTmp->ptrNode=list1->ptrNode;
                newTmp->next=NULL;
                if (primo) {
                    primo=0;
                    new->listOfVar=newTmp;
                    //	  newLast=new->listOfVar;
                }
                else newLast->next=newTmp;
                newLast=newTmp;
            }
            list1=list1->next;
        }  /*end of while (list1 !=NULL)*/
        newTmp->next=t2->listOfVar;
    }
    return new;
}

void MeltNodes(node *n1,node *n2) {
    /* fondo i due nodi mettendo il risultato in n1 */
    edge *tmp2;
    /*mette i vector left di n2 in n1 left*/
    for (tmp2=(n2->left).vector;tmp2!=NULL;tmp2=tmp2->vector) {
        (n1->left).length++;
        (n1->left).vector=AddAnEdge((n1->left).vector, tmp2->weight, tmp2->source, tmp2->side);
    }
    /*mette i vector right di n2 in n1 right*/
    for (tmp2=(n2->right).vector;tmp2!=NULL;tmp2=tmp2->vector) {
        (n1->right).length++;
        (n1->right).vector=AddAnEdge((n1->right).vector,tmp2->weight,tmp2->source, tmp2->side);
    }
    FreeNode(n2);
}

void PutSymbol(char *symbol, termGraph *t) {
    symTbl *tmp;
    int trovato=0;
    
    tmp=symbolTable;
    while((tmp!=NULL)&&(trovato==0)) {
        if (strcmp(tmp->symName,symbol)==0) trovato=1;
        else  tmp=tmp->next;
    }
    if (trovato) {
        printf("\n\tOverloading a name in the symbol table\n ");
        DeallocTermGraph(tmp->symTerm);
        strcpy(tmp->symName,symbol);
        tmp->symTerm=t;
    }
    else {
        /**** Non c'e' e l'aggiungo ****/
        tmp=(symTbl *)malloc(sizeof(symTbl));
        tmp->symName=(char *)malloc(strlen(symbol)+1);
        strcpy(tmp->symName,symbol);
        tmp->symTerm=t;
        tmp->next=symbolTable;
        symbolTable=tmp;
    }
}

void DeallocTermGraph(termGraph *t) {
    
    table *tmp1T,*tmp2T;
    node *tmp1N,*tmp2N;
    edge *tmp1E,*tmp2E;
    
    tmp1T=t->listOfVar;
    while(tmp1T!=NULL) {
        tmp2T=tmp1T->next;
        free(tmp1T->name);
        free(tmp1T);
        tmp1T=tmp2T;
    }
    
    tmp1N=t->root;
    while(tmp1N!=NULL) {
        
        tmp2N=tmp1N->nextpuit;
        
        tmp1E=(tmp1N->left).vector;
        while(tmp1E!=NULL) {
            tmp2E=tmp1E->vector;
            free(tmp1E);
            tmp1E=tmp2E;
        }
        tmp1E=(tmp1N->right).vector;
        while(tmp1E!=NULL) {
            tmp2E=tmp1E->vector;
            free(tmp1E);
            tmp1E=tmp2E;
        }
        free(tmp1N);
        tmp1N=tmp2N;
    }
}


void ShowTable() {
    symTbl *tmp;
    /*
     termGraph *t;
     node *graph;
     */
    // printf("\n\t\tTABELLA DEI SIMBOLI");
    for (tmp=symbolTable;tmp!=NULL;tmp=tmp->next) {
        
        // printf("\n\t %s = %s",tmp->symName,(tmp->symTerm)->nameTerm);
        
        /*
         t=(tmp->symTerm);
         graph=t->root;
         printf("the symbol %s has as root %li\n",tmp->symName,(long)graph->pro);
         printf("and his graph is:\n");
         MostraGrafo(t);*/
        
        /*Print(t->root,0);*/
    }
}

termGraph *IsThere(char *s) {
    symTbl *tmp;
    termGraph *new;
    
    for (tmp=symbolTable;tmp!=NULL;tmp=tmp->next)
        if (strcmp(tmp->symName,s)==0) {
            new=Duplicate(tmp->symTerm);
            OUTPUTFILE printf("\n   found %s in the symbol-table",s);
            return new;
        }
    return 0;
}

termGraph *Duplicate(termGraph *old) {
    termGraph *new ;
    
    node *newNode1;
    node *newNode2;
    node *oldNode;
    
    node *oldTnode;
    node *newTnode;
    
    edge *edgeOld;
    edge *edgeNew;
    
    edge *anotherEdge=NULL;
    int trovato;
    int primo;
    new = (termGraph*)malloc(sizeof(termGraph));
    strcpy(new->nameTerm,old->nameTerm);
    new->listOfVar=NULL;
    newNode1=CopyNode(old->root);
    new->root=newNode1;
    
    for (oldNode=(old->root)->nextpuit; oldNode!=NULL;oldNode=oldNode->nextpuit) {
        newNode2=CopyNode(oldNode);
        newNode2->prevpuit=newNode1;
        newNode1->nextpuit=newNode2;
        newNode1=newNode2;
    }
    /* Qui ho il doppione della lista di nodi senza archi  */
    
    newNode1=new->root;
    for(oldNode=old->root;oldNode!=NULL;oldNode=oldNode->nextpuit) {
        primo=1;
        for(edgeOld=(oldNode->left).vector;edgeOld!=NULL; edgeOld=edgeOld->vector) {
            edgeNew=(edge *)malloc(sizeof(edge));
            edgeNew->vector=NULL;
            strcpy(edgeNew->weight,edgeOld->weight);
            edgeNew->sto=edgeOld->sto;
            edgeNew->sign=edgeOld->sign;
            edgeNew->side=edgeOld->side;
            
            if (primo)
            { (newNode1->left).vector=edgeNew; primo=0;}
            else
            { anotherEdge->vector=edgeNew; }
            
            trovato=0;
            newTnode=new->root;
            for(oldTnode=old->root;trovato!=1;oldTnode=oldTnode->nextpuit) {
                if((edgeOld->source)->pro == oldTnode->pro) {
                    trovato=1;
                    edgeNew->source=newTnode;
                }
                else
                { newTnode=newTnode->nextpuit;}
            }
            anotherEdge=edgeNew;
            
        }
        
        primo=1;
        for(edgeOld=(oldNode->right).vector;edgeOld!=NULL; edgeOld=edgeOld->vector) {
            edgeNew=(edge *)malloc(sizeof(edge));
            edgeNew->vector=NULL;
            strcpy(edgeNew->weight,edgeOld->weight);
            edgeNew->sto=edgeOld->sto;
            edgeNew->sign=edgeOld->sign;
            edgeNew->side=edgeOld->side;
            
            if (primo)
            { (newNode1->right).vector=edgeNew; primo=0;}
            else
            { anotherEdge->vector=edgeNew; }
            
            trovato=0;
            newTnode=new->root;
            for(oldTnode=old->root;trovato!=1;oldTnode=oldTnode->nextpuit) {
                if((edgeOld->source)->pro == oldTnode->pro) {
                    trovato=1;
                    edgeNew->source=newTnode;
                }
                else
                { newTnode=newTnode->nextpuit;}
            }
            anotherEdge=edgeNew;
            
        }
        
        newNode1=newNode1->nextpuit;
        
    }
    
    /* duplicare la listOfVar */
    
    /*
     printf("\nInizio vecchio grafo\n"); MostraGrafo(old);  printf("\nFine vecchio grafo\n");
     printf("\nInizio nuovo grafo\n");   MostraGrafo(new);  printf("\nFine nuovo grafo\n");
     */
    
    return new;
}





node *CopyNode(node *old) {
    node *new=(node*)malloc(sizeof(node));
    
    new->pro=(long)new;
    new->nextpuit=NULL;
    new->prevpuit=NULL;
    (new->left).eot=(old->left).eot;
    (new->left).length=(old->left).length;
    
    (new->left).vector=NULL;
    (new->right).vector=NULL;
    
    (new->right).eot=(old->right).eot;
    (new->right).length=(old->right).length;
    
    return new;
}


/*
 void MostraLista(table *l) {
 
 table *tmp;
 
 printf("\n\t\tLISTA");
 for (tmp=l;tmp!=NULL;tmp=tmp->next) {
 printf("\n\t\t%s ----> %li",tmp->name,(tmp->ptrNode)->pro);
 }
 printf("\n");
 }
 */

void InitializeIncoming(termGraph *t) {
    node *nodo;
    edge *arco;
    edge *vsource,*vtarget;
    int storeclass,polarity;
    struct messaggio m;
    char weight[MAXLENWEIGHT];
    int npozzi=0 ;
    
    for(nodo=t->root; nodo!=NULL;nodo=nodo->nextpuit) {
        for(arco=(nodo->left).vector; arco!=NULL; arco=arco->vector) {
            /* incapsula l'arco in un messaggio */
            BDump(&incoming[schedule]);
            polarity= arco->side;
            //  DEBUG printf("inc (%ld)\n",(long)arco);
            vsource= NewReference();
            vtarget= NewReference();
            
            vsource->creator  = size;
            vsource->rankpuit = rank;
            
            //  DEBUG printf("peso dell'arco da inserire %s\n",arco->weight);
            sprintf(weight,"%s",arco->weight);
            
            vsource->source   = arco->source ;
            /* carlo : BookedAddress(size,(long) arco->source) ;*/
            
            vtarget->creator  = size ;
            vtarget->rankpuit = rank ;
            vtarget->source   = nodo ;
            /* carlo: BookedAddress(size,(int) nodo) ;*/
            
            /* if(size==1)outbuffer= &incoming; */
            vsource->sto = IN;
            /*  vsource->side=PLUS;*/
            vtarget->sto=arco->sto;
            storeclass=arco->sto ;
            
            vtarget->side=LEFT;
            //  DEBUG printf("il peso %s\n",weight);
            StoreMessage(&m,vtarget,vsource,weight,storeclass,polarity);
            
            //  DEBUG printf("after store message\n");
            ShowMessage(&m);
            PushMessage(&m); /*<------fa MPI_Bsend*/  /* In initIncoming */
            //  DEBUG printf("after push message. sto target=%d\n",vtarget->sto);
            
            if(vtarget->sto==IN) npozzi = SinkList(&m,pozzi,npozzi);
            //  DEBUG printf("after sink\n");
        }
        
        for(arco=(nodo->right).vector; arco!=NULL; arco=arco->vector) {
            /* incapsula l'arco in un messaggio */
            BDump(&incoming[schedule]);
            
            polarity= arco->side;
            //  DEBUG printf("inc (%ld)\n",(long)arco);
            vsource= NewReference();
            vtarget= NewReference();
            
            vsource->creator  = size;
            vsource->rankpuit = rank;
            
            //  DEBUG printf("peso dell'arco da inserire %s\n",arco->weight);
            sprintf(weight,"%s",arco->weight);
            
            vsource->source   = arco->source ;
            /* carlo: BookedAddress(size,(long) arco->source) ;*/
            
            vtarget->creator  = size;
            vtarget->rankpuit = rank;
            vtarget->source   = nodo;
            /* carlo: BookedAddress(size,(int) nodo) ;*/
            
            
            /* if(size==1)outbuffer= &incoming; */
            vsource->sto=IN;
            /*  vsource->side=PLUS;*/
            vtarget->sto=arco->sto;
            storeclass = arco->sto ;
            
            vtarget->side=RIGHT;
            //  DEBUG printf("il peso %s\n",weight);
            StoreMessage(&m,vtarget,vsource,weight,storeclass,polarity);
            
            //  DEBUG printf("after store message\n");
            TRACING ShowMessage(&m);
            PushMessage(&m); /* In initIncoming */
            //  DEBUG printf("after push message\n");
            
            /*
             if(vtarget->sto==IN) npozzi = SinkList(&m,&pozzi[npozzi],npozzi);
             printf("after sink\n");
             */
        }
    }
    
    /* invia effettivamente gli EOT */
    if(npozzi>0) {
        printf("(%d) sending initial graph termination signals (eot)\n",rank);
        TRACING fprintf(logfile,"(%d) sending initial graph termination signals (eot)\n",rank);
    };
    
    
    {
        int i;
        
        for(i = 0;i<npozzi;i++) {
            //  DEBUG printf("pozzo n.%d nodo %d[%p]\n",i,(int)pozzi[i].vtarget.source,pozzi[i].vtarget.source);
            PushMessage(&pozzi[i]);   /* In initIncoming */
            pozzi[i].side= !(pozzi[i].side);
            PushMessage(&pozzi[i]);
            
            TRACING BDump(&incoming[schedule]);
        }
    }
    // printf("           [OK]\n");
    TRACING fprintf(logfile,"            [OK]\n");
    
    
    BDump(&incoming[schedule]);
    printf("(%d) exit from initialization procedure\n",rank);
    printf("(%d) message stack size (of schedule %d) is |%d-%d|=%d\n",rank,schedule,incoming[schedule].last,incoming[schedule].first,incoming[schedule].last-incoming[schedule].first);
	printf("(%d) number of target nodes %d\n",rank,npozzi);

	
}

void MostraGrafo(termGraph *t) {
    table *tmp;
    if (t==NULL) return;
    tmp=t->listOfVar;
    
    printf("\n*********************************************\n");
    printf("(%d) free variables are:\n",rank);
    while (tmp!=NULL) {
        printf(" %s  has pointer to %ld)\n",tmp->name,tmp->ptrNode->pro);
        tmp=tmp->next;
    }
    printf("(%d) graph is:\n",rank);
    MostraListaDiNodi(t->root);
    
}

void MostraListaDiNodi(node *n) {
    node *tmp;
    
    printf("(ROOT) ");
    MostraNodo(n);
    for(tmp=n->nextpuit;tmp!=NULL;tmp=tmp->nextpuit)
        MostraNodo(tmp);
    
    for(tmp=n->prevpuit;tmp!=NULL;tmp=tmp->prevpuit)
        MostraNodo(tmp);
    
}

void MostraNodo(node *n) {
    
    printf("Nodo %ld :\n",(long)n->pro);
    printf("\tLEFT) eot    = %d\n",(n->left).eot);
    printf("\tLEFT) weot    = %d\n",(n->left).weot);
    printf("\tLEFT) length = %d\n",(n->left).length);
    MostraArchi((n->left).vector);
    printf("\tRIGHT) eot    = %d\n",(n->right).eot);
    printf("\tRIGHT) weot    = %d\n",(n->right).weot);
    printf("\tRIGHT) length = %d\n",(n->right).length);
    MostraArchi((n->right).vector);
    printf("\tIl Next Ã¨ %p\n",n->nextpuit);
    printf("\tIl Prev Ã¨ %p\n",n->prevpuit);
}

void MostraArchi(edge *e) {
    
    edge *tmp;
    int i=1;
    
    for(tmp=e;tmp!=NULL;tmp=tmp->vector) {
        printf("\t\t edge%i = %s ",i++,tmp->weight);
        if (tmp->sto==IN)    printf("\t sto = IN ");
        else  printf("\t sto = OUT ");
        if (tmp->source!=NULL)
            printf(" source = %ld",tmp->source->pro);
        if (tmp->side==LEFT) printf("\t side = LEFT");
        else if (tmp->side==RIGHT) printf(" side = RIGHT");
        else printf(" side= azzo");
        printf("\n");
    }
}

/* ANTO */
void LoadLib(char *path)
{
    /* dynamic loading of a function library */
    memset(Path, 0, MAXNAMELEN);
    strcpy(Path, path);
    
    handle = dlopen (Path, RTLD_LAZY);
    
    if (!handle) {
        fputs (dlerror(), stderr);
        exit(1);
    }
    
}

void BCastLoad(void)
{
    /* broadcast dynamic loading for extern functions management */
    
    int mpi_size, rank;
    int i, j, fun_number=0, narg=0;
    MPI_Status status;
    char funname[MAXNAMELEN];
    
    memset(funname, 0, MAXNAMELEN);
    
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &mpi_size );
    
    //printf("\n CHECK 1 curr_rank=%d mpi_size=%d Path=%s\n",rank,mpi_size,Path);
    
    if (rank == 0)
    {
        for (i=1; i<mpi_size; i++)
        {
            
            /* broadcast dynamic loading of a function library */
            
            MPI_Send(Path, MAXNAMELEN, MPI_CHAR, i, LOADLIB_TAG, MPI_COMM_WORLD);
            
            /* broadcast dynamic loading of functions table */
            MPI_Send(&findex, 1, MPI_INT, i, LOADFUN_TAG, MPI_COMM_WORLD);
            
            for (j=0; j<findex+1; j++)
            {
                //printf("\n SEND curr_rank=%d dest_rank=%d funname=%s\n",rank,i,f_db[j].name);
                MPI_Send(&f_db[j].narg, 1, MPI_INT, i, LOADFUN_TAG, MPI_COMM_WORLD);
                MPI_Send(f_db[j].name, MAXNAMELEN, MPI_CHAR, i, LOADFUN_TAG, MPI_COMM_WORLD);
            }
            
        }
    }
    else {
        MPI_Recv(Path, MAXNAMELEN, MPI_CHAR, 0, LOADLIB_TAG, MPI_COMM_WORLD, &status );
        MPI_Recv(&fun_number, 1, MPI_INT, 0, LOADFUN_TAG, MPI_COMM_WORLD, &status );
        // ANTprintf("\n RECV2 curr_rank=%d funname=%s num = %d\n",rank,funname,fun_number);
        
        
        if (Path[0] !='\0')
        {
            handle = dlopen (Path, RTLD_LAZY);
            
            if (!handle) {
                fputs (dlerror(), stderr);
                exit(1);
            }
            
            for (j=0; j< fun_number+1; j++)
            {
                MPI_Recv(&narg, 1, MPI_INT, 0, LOADFUN_TAG, MPI_COMM_WORLD, &status );
                MPI_Recv(funname, MAXNAMELEN, MPI_CHAR, 0, LOADFUN_TAG, MPI_COMM_WORLD, &status );
                //printf("\n RECV curr_rank=%d funname=%s\n",rank,funname);
                if(funname[0] != '0')
                    XAddEntryInf(funname,narg);
                
                //printf("\n MATCHED curr_rank=%d funname=%s\n",rank,funname);
            }
        }
        
        
        /* ANTO
         for (j=0; j<findex+1; j++)
         {
         printf("\n CHECK 2 rank=%d mpi_size=%d funname=%s\n",rank,mpi_size,f_db[j].name);
         }
         ANTO */
        
    }
    
}


void CloseLib(void)
{
    /* dynamic unloading of a function library */
    
    if (handle != NULL)
        dlclose (handle);
    
}


USERTYPE pelcr_not(USERTYPE n) {
    if (n==0) return 1;
    else return 0;
}

USERTYPE pelcr_succ(USERTYPE n) {
    return n+1;
}

USERTYPE pelcr_pred(USERTYPE n) {
    return n-1;
}

USERTYPE pelcr_iszero(USERTYPE n) {
    if (n==0) return 1;
    else return 0;
}

USERTYPE pelcr_add(USERTYPE n, USERTYPE m) {
    return n+m;
}

USERTYPE pelcr_mult(USERTYPE n, USERTYPE m) {
    return n*m;
}

USERTYPE pelcr_and(USERTYPE n, USERTYPE m) {
    if (n*m==0) return 0;
    else return 1;
}

USERTYPE pelcr_great(USERTYPE n, USERTYPE m) {
    if (n>m) return 1;
    else return 0;
}

/* ANTO */

void InitTable() {
    
    t[0][0]=NAT;   t[0][1]=TIN;
    t[1][0]=NAT;   t[1][1]=TOUT;
    
    t[2][0]=BOOL;  t[2][1]=TIN;
    t[3][0]=BOOL;  t[3][1]=TOUT;
    
}

