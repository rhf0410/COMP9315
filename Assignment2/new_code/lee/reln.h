// reln.h ... interface to functions on Relations
// part of Multi-attribute Linear-hashed Files
// See reln.c for details on Reln type and functions
// Written by John Shepherd, April 2016

#ifndef RELN_H
#define RELN_H 1

typedef struct RelnRep *Reln;

#include "defs.h"
#include "tuple.h"
#include "chvec.h"
#include "page.h"

Status newRelation(char *name, Count nattr, Count npages, Count d, char *cv);
Reln openRelation(char *name, char *mode);
void closeRelation(Reln r);
Bool existsRelation(char *name);
PageID addToRelation(Reln r, Tuple t);
void splitRelation(Reln r);
Status insertintoPage(Reln r, Tuple t, PageID pid);
Count nattrs(Reln r);
Count npages(Reln r);
Count depth(Reln r);
Count splitp(Reln r);
ChVecItem *chvec(Reln r);
FILE *fdata(Reln r);
FILE *fovflow(Reln r);
FILE *finfo(Reln r);
void relationStats(Reln r);

#endif
