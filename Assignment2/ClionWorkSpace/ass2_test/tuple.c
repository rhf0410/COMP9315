// tuple.c ... functions on tuples
// part of Multi-attribute Linear-hashed Files
// Written by John Shepherd, April 2016

#include "defs.h"
#include "tuple.h"
#include "reln.h"
#include "hash.h"
#include "chvec.h"
#include "bits.h"

// return number of bytes/chars in a tuple

int tupLength(Tuple t)
{
	return strlen(t);
}

// reads/parses next tuple in input

Tuple readTuple(Reln r, FILE *in)
{
	char line[MAXTUPLEN];
	if (fgets(line, MAXTUPLEN-1, in) == NULL)
		return NULL;
	line[strlen(line)-1] = '\0';
	// count fields
	// cheap'n'nasty parsing
	char *c; int nf = 1;
	for (c = line; *c != '\0'; c++)
		if (*c == ',') nf++;
	// invalid tuple
	if (nf != nattrs(r)) return NULL;
	return strdup(line); // needs to be free'd sometime
}

// extract values into an array of strings

void tupleVals(Tuple t, char **vals)
{
	char *c = t, *c0 = t;
	int i = 0;
	for (;;) {
		while (*c != ',' && *c != '\0') c++;
		if (*c == '\0') {
			// end of tuple; add last field to vals
			vals[i++] = strdup(c0);
			break;
		}
		else {
			// end of next field; add to vals
			*c = '\0';
			vals[i++] = strdup(c0);
			*c = ',';
			c++; c0 = c;
		}
	}
}

// release memory used for separate attirubte values

void freeVals(char **vals, int nattrs)
{
	int i;
	for (i = 0; i < nattrs; i++) free(vals[i]);
}

// hash a tuple using the choice vector
Bits tupleHash(Reln r, Tuple t)
{
	int i;
	ChVecItem *cv;
	char buf[40];
	Count nvals = nattrs(r);
	char **vals = malloc(nvals*sizeof(char *));
	assert(vals != NULL);
	tupleVals(t, vals);

	//Bits hash = hash_any((unsigned char *)vals[0],strlen(vals[0]));
	Bits hash = 0;
	Bits val_hash[40];
	Byte a, b;

	for (i = 0; i < nvals; i++) {
		val_hash[i] = hash_any((unsigned char *)vals[i],strlen(vals[i]));
	}

	cv = chvec(r);

	for (i = 0; i < MAXCHVEC; i++) {
		a = cv[i].att;
		b = cv[i].bit;

		if (bitIsSet(val_hash[a], b)) {
			hash = setBit(hash, i);
        }
	}

	showBits(hash,buf);
	printf("hash(%s) = %s\n", vals[0], buf);

	return hash;
}

// compare two tuples (allowing for "unknown" values)

Bool tupleMatch(Tuple t1, Tuple t2, int nattrs)
{
	char **v1 = malloc(nattrs*sizeof(char *));
	tupleVals(t1, v1);
	char **v2 = malloc(nattrs*sizeof(char *));
	tupleVals(t2, v2);
	Bool match = TRUE;
	int i;
	for (i = 0; i < nattrs; i++) {
		if (v1[i][0] == '?' || v2[i][0] == '?') continue;
		if (strcmp(v1[i],v2[i]) == 0) continue;
		match = FALSE;
	}
	freeVals(v1,nattrs); freeVals(v2,nattrs);
	return match;
}

// puts printable version of tuple in user-supplied buffer

void showTuple(Tuple t, char *buf)
{
	strcpy(buf,t);
}
