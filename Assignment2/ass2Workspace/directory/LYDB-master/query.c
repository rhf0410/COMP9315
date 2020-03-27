// query.c ... query scan functions
// part of Multi-attribute Linear-hashed Files
// Manage creating and using Query objects
// Written by John Shepherd, April 2016

#include "defs.h"
#include "query.h"
#include "reln.h"
#include "tuple.h"

#include "bits.h"
#include "hash.h"

// A suggestion ... you can change however you like

struct QueryRep {
	Reln    rel;       // need to remember Relation info
	Bits    known;     // the hash value from MAH
	Bits    unknown;   // the unknown bits from MAH
	PageID  curpage;   // current page in scan
	int     is_ovflow; // are we in the overflow pages?
	Offset  curtup;    // offset of current tuple within page
	//TODO
	Tuple   qry_tuple;  // to query tuple
	Bits    curhash;    // current hash value
};

// take a query string (e.g. "1234,?,abc,?")
// set up a QueryRep object for the scan

Query startQuery(Reln r, char *q)
{
    int i;

    Tuple t = q;

	Query new = malloc(sizeof(struct QueryRep));
	assert(new != NULL);

    Count nvals = nattrs(r);
	char **vals = malloc(nvals * sizeof(char *));
	assert(vals != NULL);

	tupleVals(t, vals);

	Bits known, unknown, val_hash[40];
	known = unknown = 0;

	for (i = 0; i < nvals; i++) {
	    if (strcmp(vals[i], "?") == 0) {
	        val_hash[i] = 0;
        } else {
            val_hash[i] = hash_any((unsigned char *)vals[i], strlen(vals[i]));
        }
    }

    ChVecItem *cv = chvec(r);

    for (i = 0; i < MAXCHVEC; i++) {
        if (bitIsSet(val_hash[cv[i].att], cv[i].bit)) {
            known = setBit(known, i);
        }

        if (strcmp(vals[cv[i].att], "?") == 0) {
            unknown = setBit(unknown, i); /* mask */
        }
    }

    Bits p;
    p = getLower(known, depth(r));
    if (p < splitp(r)) p = getLower(known, depth(r)+1);

    new->rel = r;
    new->known = known;
    new->unknown = unknown;
    new->curpage = p; /* set PageID */
    new->is_ovflow = 0;
    new->curtup = 0;
    new->qry_tuple = q;
    new->curhash = known;

    freeVals(vals, nvals);

	// TODO
	// Partial algorithm:
	// form known bits from known attributes
	// form unknown bits from '?' attributes
	// compute PageID of first page
	//   using known bits and first "unknown" value
	// set all values in QueryRep object

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

AGAIN:

	if (q->is_ovflow == 0) {
	    pg = getPage(datafile(q->rel), q->curpage);
    } else {
        pg = getPage(flowfile(q->rel), q->curpage);
    }

	if (q->curtup < pageFreeOffset(pg)) {
	    while (q->curtup < pageFreeOffset(pg)) {

	        Tuple t = getPageTuple(pg, q->curtup);

	        if (tupleMatch(t, q->qry_tuple, nattrs(q->rel)) == TRUE) {
	            q->curtup += tupLength(t)+1;
	            Tuple ret = strdup(t);
	            free(pg);
	            return ret;
            }

            q->curtup += tupLength(t)+1;
        }

        free(pg);
        goto AGAIN;

    } else if (pageOvflow(pg) != NO_PAGE) {
        q->curpage = pageOvflow(pg);
        q->curtup  = 0;
        q->is_ovflow = 1;
        //printf("in overflow\n");
        free(pg);
        goto AGAIN;
    } else {

        free(pg);
        pg = NULL;

        int dep = depth(q->rel);
        if (splitp(q->rel) > 0) dep = dep + 1;

        Bits mask = (1<<dep) - 1;

        if ((mask & q->unknown) != 0) { /* should have uncentain bit */

            q->curhash = q->curhash & mask;
            q->known = q->known & mask;
            q->unknown = q->unknown & mask;

            if (q->curhash != (q->known | q->unknown)) {
                Bits nextbit;

                for (nextbit = q->curhash+1;
                       nextbit <= (q->known|q->unknown);
                       nextbit++) {

                    /*
                    char buf[40];
                    showBits(nextbit, buf);
                    printf("nextbit hash = %s\n", buf);
                    */

                    Bits nbit = (nextbit & q->unknown) | q->known;
                    if (nbit != q->curhash) {
                        /* next bucket */

                        q->curhash = nbit;
                        q->curtup = 0;
                        q->is_ovflow = 0;

                        Bits p;
                        p = getLower(nbit, depth(q->rel));
                        if (p < splitp(q->rel))
                            p = getLower(nbit, depth(q->rel)+1);

                        if (p != q->curpage) {
                            //printf("page=%d\n", p);
                            q->curpage = p;
                            goto AGAIN;
                        }
                    }
                }
            }
        }
    }

    if (pg != NULL)
        free(pg);

    return NULL;
}

// clean up a QueryRep object and associated data

void closeQuery(Query q)
{
	// TODO
	free(q);
}
