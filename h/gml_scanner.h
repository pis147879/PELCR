/* Scanner of graphs in GML format
 
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

#ifndef gml_scanner_h
#define gml_scanner_h

#include <stdio.h>

/*
 * start-size of buffers for reading strings. If too small it will be enlarged
 * dynamically
 */

#define INITIAL_SIZE 1024

typedef enum {
    GML_KEY, GML_INT, GML_DOUBLE, GML_STRING, GML_L_BRACKET, 
    GML_R_BRACKET, GML_END, GML_LIST, GML_ERROR
} GML_value; 


typedef enum {
    GML_UNEXPECTED, GML_SYNTAX, GML_PREMATURE_EOF, GML_TOO_MANY_DIGITS,
    GML_OPEN_BRACKET, GML_TOO_MANY_BRACKETS, GML_OK
} GML_error_value;


struct GML_error {
    GML_error_value err_num;
    int line;
    int column;
};


union GML_tok_val {
    long integer;
    double floating;
    char* string;
    struct GML_error err;
};


struct GML_token { 
    GML_value kind;
    union GML_tok_val value;
};

/*
 * global variables
 */

extern unsigned int GML_line;
extern unsigned int GML_column;

/*
 * if you are interested in the position where an error occured it is a good
 * idea to set GML_line and GML_column back. 
 * This is what GML_init does.
 */
 
void GML_init ();

/*
 * returns the next token in file. If an error occured it will be stored in 
 * GML_token.
 */

struct GML_token GML_scanner (FILE*);

#endif







