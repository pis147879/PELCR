
/* Header for the Manager for distribution of workload
 
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

