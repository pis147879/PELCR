/* Header for Input and output functions: graph dumping, readback, graph reader
 
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

#ifndef io_h
#define io_h

void OpenFileInitStruct();/**/

void Print(graph g,struct mbuffer *b, long int sq);
int SinkList(
	     struct messaggio*m,
	     struct messaggio*l,
	     int length);
void read_back( node*);
void SetOutputFile(char *name);
void SetDirectory(char *name) ;
void WriteStatus(void);
void WriteHelp(void);
#endif

