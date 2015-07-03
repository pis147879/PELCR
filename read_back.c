/* Read-back functions
 
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
#include <stdlib.h>
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

/* read_back from a virtual net in normal form to a
   lambda term */




void read_back( node *principal )
{
//	DEBUG{
			printf("(%d) ********** principal port address %p\n" , rank, principal);
//   		};
	}
