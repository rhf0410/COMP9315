// tuple.c ... functions on tuples
// part of Multi-attribute Linear-hashed Files
// Last modified by John Shepherd, July 2019

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
	return copyString(line); // needs to be free'd sometime
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
			vals[i++] = copyString(c0);
			int n = strlen(vals[i-1]);
			if(strlen(vals[i-1]) > 1){
				char *des = malloc(strlen(vals[i-1])*sizeof(char));
				strncpy(des, vals[i-1], n - 1);
				vals[i-1] = des;
			}
			break;
		}
		else {
			// end of next field; add to vals
			*c = '\0';
			vals[i++] = copyString(c0);
			*c = ',';
			c++; c0 = c;
		}
	}
}

void tupleVals2(Tuple t, char **vals)
{
	char *c = t, *c0 = t;
	int i = 0;
	for (;;) {
		while (*c != ',' && *c != '\0') c++;
		if (*c == '\0') {
			// end of tuple; add last field to vals
			vals[i++] = copyString(c0);
			break;
		}
		else {
			// end of next field; add to vals
			*c = '\0';
			vals[i++] = copyString(c0);
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
// TODO: actually use the choice vector to make the hash

Bits tupleHash(Reln r, Tuple t)
{
	int i;
	char buf[MAXBITS+1];
	Count nvals = nattrs(r);
	char **vals = malloc(nvals*sizeof(char *));
	assert(vals != NULL);
	tupleVals(t, vals);

	// Bits hash = hash_any((unsigned char *)vals[0],strlen(vals[0]));
  Bits hash = 0;
	Bits val_hash[MAXBITS+1];
	for(i=0;i<nvals;i++){
			val_hash[i] = hash_any((unsigned char *)vals[i], strlen(vals[i]));
	}

	Byte att;
	Byte bit;
	ChVecItem *c;
	c = chvec(r);
	for (i=0;i<MAXCHVEC;i++){
	  att = c[i].att;
	  bit = c[i].bit;
	  if (bitIsSet(val_hash[att],bit)){
	    hash = setBit(hash,i);
	  }else{
			hash = unsetBit(hash,i);
		}
	}
	bitsString(hash,buf);
	//Show info
	// printf("hash(");
	// for(i=0;i<nvals;i++){
	// 	if(i == nvals - 1){
	// 		printf("%s)  = %s\n", vals[i], buf);
	// 	}else{
	// 		printf("%s,", vals[i]);
	// 	}
	// }
	//printf("%s\n", t);
	return hash;
}

// compare two tuples (allowing for "unknown" values)

Bool tupleMatch(Reln r, Tuple t1, Tuple t2)
{
	Count na = nattrs(r);
	char **v1 = malloc(na*sizeof(char *));
	tupleVals2(t1, v1);
	char **v2 = malloc(na*sizeof(char *));
	tupleVals2(t2, v2);
	Bool match = TRUE;
	int i;
	for (i = 0; i < na; i++) {
		// assumes no real attribute values start with '?'
		if (v1[i][0] == '?' || v2[i][0] == '?') continue;
		if (strcmp(v1[i],v2[i]) == 0) continue;
		match = FALSE;
	}
	freeVals(v1,na);
	freeVals(v2,na);
	return match;
}

Tuple nextTuple(FILE *f, PageID pid, Offset curtup){
	char tuple[MAXTUPLEN];
	Offset base = pid * PAGESIZE + 2*sizeof(Offset) + sizeof(Count);
	fseek(f, base + curtup, SEEK_SET);
	fgets(tuple, MAXTUPLEN-1, f);
	return copyString(tuple);
}

// puts printable version of tuple in user-supplied buffer

void tupleString(Tuple t, char *buf)
{
	strcpy(buf,t);
}
