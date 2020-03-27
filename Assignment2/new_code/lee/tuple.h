// tuple.h ... interface to functions on Tuples
// part of Multi-attribute Linear-hashed Files
// A Tuple is just a '\0'-terminated C string
// Consists of "val_1,val_2,val_3,...,val_n"
// See tuple.c for details on functions
// Written by John Shepherd, April 2016

#ifndef TUPLE_H
#define TUPLE_H 1

typedef char *Tuple;

#include "reln.h"
#include "bits.h"
#include "defs.h"

int tupLength(Tuple t);
Tuple readTuple(Reln r, FILE *in);
Tuple nextTuple(FILE *in,PageID pid,Offset curtup);
Bits tupleHash(Reln r, Tuple t);
void tupleVals(Tuple t, char **vals);
void freeVals(char **vals, int nattrs);
Bool tupleMatch(Tuple t1, Tuple t2, int nattrs);
void showTuple(Tuple t, char *buf);

#endif
