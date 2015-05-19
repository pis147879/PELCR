#ifndef distribution_h
#define distribution_h

void InitTable();/**/

void buf_flush();/**/

void SendAddEdge(edge*N,edge*E,char*sw,int sto);/**/
int SendLightProcess(void);/**/
int LightProcess(int dest);/**/
int LightProcess2(int dest);/**/
int LightProcess3(int dest);/**/
int LightProcess5(int dest);/**/
int LightProcess6(int dest);/**/
int LightProcess7(int dest);/**/
int LightProcess8(int dest);/**/

void SendCreateNewNode(int dest,int sto,edge*nS);/**/
int NouvelleReservation(int dest);/**/
void PopMessage(struct messaggio *m,struct mbuffer *l);
void PushMessage(struct messaggio *m);

void AggregationControl1();
void AggregationControl2();
void AggregationControl3();
void AggregationControl4();
void AggregationControl5();

void *ThreadReceiveMsgs();
void *ThreadInteraction();

#endif

