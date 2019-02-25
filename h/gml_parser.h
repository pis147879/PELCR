/* Parser of graphs in GML format
 
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

#ifndef gml_parser_h
#define gml_parser_h

#include "mydefs.h"
#include "var.h"
#include "symbolic.h"
#include "graph.h"
#include "combustion.h"
#include "gml_scanner.h"

union GML_pair_val {
    long integer;
    double floating;
    char* string;
    struct GML_pair* list;
};

struct GML_pair {
    char* key;
    GML_value kind;
    union GML_pair_val value;
    struct GML_pair* next;
};

struct GML_list_elem {
    char* key;
    struct GML_list_elem* next;
};

struct GML_stat {
    struct GML_error err;
    struct GML_list_elem* key_list;
};

/*
 * returns list of KEY - VALUE pairs. Errors and a pointer to a list
 * of key-names are returned in GML_stat
 */

struct GML_pair* GML_parser (FILE*, struct GML_stat*, int);

/*
 * free memory used in a list of GML_pair
 */

void GML_free_list (struct GML_pair*, struct GML_list_elem*);


/*
 * debugging 
 */

void GML_print_list (struct GML_pair*, int);

int GML_edge_det(struct GML_pair*list, struct messaggio*pozzi, int npozzi);

//int SinkList(struct messaggio *,struct messaggio *,int);

#endif


