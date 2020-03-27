// tuple.c ... functions on tuples
// part of Multi-attribute Linear-hashed Files
// Written by John Shepherd, April 2016

#include "defs.h"
#include "tuple.h"
#include "reln.h"
#include "hash.h"
#include "chvec.h"
#include "bits.h"
#include "query.h"
#include <math.h>
// return number of bytes/chars in a tuple

int tupLength(Tuple t)
{
	return strlen(t);
}

// reads/parses next tuple in input

Tuple readTuple(Reln r, FILE *in)
{
	char line[MAXTUPLEN];
	fgets(line, MAXTUPLEN - 1, in);
	line[strlen(line) - 1] = '\0';
	// count fields
	// cheap'n'nasty parsing
	char *c;
	int nf = 1;
	for (c = line; *c != '\0'; c++)
		if (*c == ',')
			nf++;

	// invalid tuple
	if (nf != nattrs(r))
		return NULL;

	return strdup(line); // needs to be free'd sometime
}

Tuple nextTuple(FILE *in,PageID pid,Offset curtup)
{
	char line[MAXTUPLEN];
	Offset base = PAGESIZE * pid + 2 * sizeof(Offset) + sizeof(Count);

	fseek(in, base + curtup, SEEK_SET);
	fgets(line, MAXTUPLEN - 1, in);
	// count fields
	// cheap'n'nasty parsing
	char *c;
	int nf = 1;
	for (c = line; *c != '\0'; c++)
		if (*c == ',')
			nf++;

	return strdup(line); // needs to be free'd sometime
}

// extract values into an array of strings

void tupleVals(Tuple t, char **vals)
{
	char *c = t, *c0 = t;
	int i = 0;
	for (;;)
	{
		while (*c != ',' && *c != '\0')
			c++;
		if (*c == '\0')
		{
			// end of tuple; add last field to vals
			vals[i++] = strdup(c0);
			break;
		} else
		{
			// end of next field; add to vals
			*c = '\0';
			vals[i++] = strdup(c0);
			*c = ',';
			c++;
			c0 = c;
		}
	}
}

// release memory used for separate attribute values

void freeVals(char **vals, int nattrs)
{
	int i;
	for (i = 0; i < nattrs; i++)
		free(vals[i]);
}

// hash a tuple using the choice vector

Bits tupleHash(Reln r, Tuple t)
{
	char buf[35];
	Count nvals = nattrs(r);
	char **vals = malloc(nvals * sizeof(char *));
	assert(vals != NULL);
	tupleVals(t, vals);
	Bits hash[nvals];
	int j;

	//Compute the hash for attrs, and save into hash[]
	for (j = 0; j < nvals; j++)
	{
		hash[j] = hash_any((unsigned char *) vals[j], strlen(vals[j]));
		showBits(hash[j], buf);
		printf("hash(%s) = %s\n", vals[j], buf);

	}

	//Insert bits from attrhash to the buffer according to the cv
	ChVecItem *cv = chvec(r);
	char buffer[35];
	for (j = 0; j < 32; j++)
	{
		Byte att = cv[j].att;
		Byte bit = cv[j].bit;
		showBits(hash[att], buf);
		int c = 0, i = 0;
		while (buf[c] != '\0')
		{
			if (buf[c] != ' ')
			{
				buf[i++] = buf[c];
			}
			c++;
		}
		//printf("%c\n", buf[31-bit]);
		buffer[31 - j] = buf[31 - bit];
	}

	//Transfer buffer array into Bits
	Bits result = 0xFFFFFFFF;
	for (j = 0; j < 32; j++)
	{
		if (buffer[j] == '0')
			result = unsetBit(result, 31 - j);

	}

	free(vals);
	//showBits(result,buf);
	//printf("the result is %s\n",buf);
	return result;
}

// compare two tuples (allowing for "unknown" values)

Bool tupleMatch(Tuple t1, Tuple t2, int nattrs)
{
	char **v1 = malloc(nattrs * sizeof(char *));
	tupleVals(t1, v1);
	char **v2 = malloc(nattrs * sizeof(char *));
	tupleVals(t2, v2);
	Bool match = TRUE;
	int i;
	for (i = 0; i < nattrs; i++)
	{
		if (v1[i][0] == '?' || v2[i][0] == '?')
			continue;
		if (strcmp(v1[i], v2[i]) == 0)
			continue;
		match = FALSE;
	}
	freeVals(v1, nattrs);
	freeVals(v2, nattrs);
	return match;
}

// puts printable version of tuple in user-supplied buffer

void showTuple(Tuple t, char *buf)
{
	strcpy(buf, t);
}
