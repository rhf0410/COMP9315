// query.c ... query scan functions
// part of Multi-attribute Linear-hashed Files
// Manage creating and using Query objects
// Last modified by John Shepherd, July 2019

#include "defs.h"
#include "query.h"
#include "reln.h"
#include "tuple.h"
#include "hash.h"

// A suggestion ... you can change however you like

struct QueryRep
{
	Reln rel;			// need to remember Relation info
	Bits known;			// the hash value from MAH
	Bits unknown;		// the unknown bits from MAH
	PageID curpage;		// current page in scan
	int is_ovflow;		// are we in the overflow pages?
	Offset curtup;		// offset of current tuple within page
						//TODO
	Count nqueryTuples; //How many tuples we have examed?
	Tuple query;
	PageID lastbucket; // the last/current bucket we are scaning.
};

int checkQuery(Reln r, char *q)
{
	if (*q == '\0')
		return 0;
	char *c;
	int nattr = 1;
	for (c = q; *c != '\0'; c++)
		if (*c == ',')
			nattr++;
	return (nattr == nattrs(r));
}

Bool validQueryBucket(PageID bucketID, Bits known, Bits unknown, Reln r);

// take a query string (e.g. "1234,?,abc,?")
// set up a QueryRep object for the scan

Query startQuery(Reln r, char *q)
{
	if (!checkQuery(r, q))
	{
		return NULL;
	}
	Query new = malloc(sizeof(struct QueryRep));
	assert(new != NULL);
	// TODO
	// Partial algorithm:
	// form known bits from known attributes
	// form unknown bits from '?' attributes
	// compute PageID of first page
	//   using known bits and first "unknown" value
	// set all values in QueryRep object

	Count nvals = nattrs(r);
	char **vals = malloc(nvals * sizeof(char *));
	tupleVals(q, vals);
	new->known = 0x0;
	Bits hashAttrs[nvals];
	ChVecItem *cv = chvec(r);
	for (int i = 0; i < nvals; i++)
	{
		// we set all the bits unknown in `known` to `0`.
		Bits temp = 0x0;
		if (vals[i][0] == '?')
		{
			hashAttrs[i] = 0x0;
		}
		else
		{
			temp = hash_any((unsigned char *)vals[i], strlen(vals[i]));
			hashAttrs[i] = temp;
		}
	}
	for (int i = 0; i < MAXCHVEC; i++)
	{
		Byte att = (cv + i)->att;
		Byte bit = (cv + i)->bit;
		if (bitIsSet(hashAttrs[att], bit))
		{
			new->known = setBit(new->known, i);
		}
		if (hashAttrs[att] == 0x0)
		{
			new->unknown = setBit(new->unknown, i);
		}
	}

	Bits p;
	if (depth(r) == 0)
		p = 1;
	else
	{
		p = getLower(new->known, depth(r));
		if (p < splitp(r))
			p = getLower(new->known, depth(r) + 1);
	}
	new->rel = r;
	new->curpage = p;
	new->is_ovflow = FALSE;
	new->curtup = 0u;
	new->nqueryTuples = 0u;
	new->query = q;
	new->lastbucket = p;
	freeVals(vals, nvals);
	return new;
}

// get next tuple during a scan

Tuple getNextTuple(Query q)
{
	// TODO
	// Partial algorithm:
	// if (more tuples in current page)
	//    get next matching tuple from current page
	// else if (current page has overflow)
	//    move to overflow page
	//    grab first matching tuple from page
	// else
	//    move to "next" bucket
	//    grab first matching tuple from data page
	// endif
	// if (current page has no matching tuples)
	//    go to next page (try again)
	// endif
	Page pg;
try_again:
	if (!q->is_ovflow)
	{
		pg = getPage(dataFile(q->rel), q->curpage);
	}
	else
	{
		pg = getPage(ovflowFile(q->rel), q->curpage);
	}
	while (q->nqueryTuples < pageNTuples(pg))
	{
		Tuple t1 = q->query;
		Tuple t2 = (char *)(pageData(pg) + q->curtup);
		if (tupleMatch(q->rel, t1, t2))
		{
			q->nqueryTuples++;
			q->curtup += tupLength(t2) + 1;
			return t2;
		}
		q->nqueryTuples++;
		q->curtup += tupLength(t2) + 1;
	}

	while (pageOvflow(pg) != NO_PAGE)
	{
		//goto ovflowFile
		q->is_ovflow = TRUE;
		q->nqueryTuples = 0u;
		q->curpage = pageOvflow(pg);
		q->curtup = 0u;

		//
		pg = getPage(ovflowFile(q->rel), q->curpage);
		while (q->nqueryTuples < pageNTuples(pg))
		{
			Tuple t1 = q->query;
			Tuple t2 = (char *)(pageData(pg) + q->curtup);
			if (tupleMatch(q->rel, t1, t2))
			{
				q->nqueryTuples++;
				q->curtup += tupLength(t2) + 1;
				return t2;
			}
			q->nqueryTuples++;
			q->curtup += tupLength(t2) + 1;
		}
	}

	//goto next bucket.
	q->is_ovflow = FALSE;
	q->nqueryTuples = 0u;
	// q->curpage [TODO]
	q->curtup = 0u;
	PageID nextBucket = q->lastbucket + 1;
	while (TRUE)
	{
		// npages(q->rel) is the number of bucket.
		if (nextBucket >= npages(q->rel))
		{
			return NULL;
		}
		if (validQueryBucket(nextBucket, q->known, q->unknown, q->rel))
		{
			q->lastbucket = nextBucket;
			q->curpage = nextBucket;
			goto try_again;
		}
		else
		{
			nextBucket += 1;
		}
	}
	free(pg);
	return NULL;
}
// typedef unsigned int BucketID;
// bucketID is less than bucket number.
// 0b1011011  known
// 0b0000100  unknown
// 0b1011011
// 0b1011111
Bool validQueryBucket(PageID bucketID, Bits known, Bits unknown, Reln r)
{
	Bits mask = bucketID & (~unknown);
	unsigned int ovsp = 0u;
	PageID p = getLower(bucketID, depth(r));
	if (p < splitp(r))
	{
		ovsp = 1u;
	}
	return getLower(known, depth(r) + ovsp) == mask;
}

// clean up a QueryRep object and associated data

void closeQuery(Query q)
{
	free(q);
}
