// query.c ... query scan functions
// part of Multi-attribute Linear-hashed Files
// Manage creating and using Query objects
// Last modified by John Shepherd, July 2019

#include "defs.h"
#include "query.h"
#include "reln.h"
#include "tuple.h"

// A suggestion ... you can change however you like

struct QueryRep {
	Reln    rel;       // need to remember Relation info
	Bits    known;     // the hash value from MAH
	Bits    unknown;   // the unknown bits from MAH
	PageID  curpage;   // current page in scan
	int     is_ovflow; // are we in the overflow pages?
	Offset  curtup;    // offset of current tuple within page
	//TODO
	Tuple query_tuple; //query tuple
	Bits current_hash; //current hash value
};

// take a query string (e.g. "1234,?,abc,?")
// set up a QueryRep object for the scan

Query startQuery(Reln r, char *q)
{
	Query new = malloc(sizeof(struct QueryRep));
	assert(new != NULL);
	// TODO
	// Partial algorithm:
	Tuple t = q;
	Count nvals = nattrs(r);
	char **vals = malloc(nvals*sizeof(char *));
	Bits composite_hashes[nvals];
	tupleVals(t, vals);

	int i;
	for(i=0;i<nvals;i++){
		//printf("%d: %s\n", i, vals[i]);
		if(strcmp(vals[i], "?") == 0){
			// form unknown bits from '?' attributes
			composite_hashes[i] = 0;
		}else{
			// form known bits from known attributes
			composite_hashes[i] = hash_any((unsigned char *)vals[i],strlen(vals[i]));
		}
	}
	//Setting known and unknown
	ChVecItem *cv = chvec(r);
	Bits known = 0;
	Bits unknown = 0;
	for(i=0;i<MAXCHVEC;i++){
		Byte a = cv[i].att;
		Byte b = cv[i].bit;

		if(bitIsSet(composite_hashes[a], b)){
			known = setBit(known, i);
		}
		if(strcmp(vals[a], "?") == 0){
			unknown = setBit(unknown, i);
		}
	}

	// compute PageID of first page
	//   using known bits and first "unknown" value
	Bits p = getLower(known, depth(r)) < splitp(r) ? getLower(known, depth(r) + 1) : getLower(known, depth(r));

	// set all values in QueryRep object
	new->rel = r;
	new->known = known;
	new->unknown = unknown;
	new->curpage = p;
	new->is_ovflow = 0;
	new->curtup = 0;
	new->query_tuple = q;
	new->current_hash = known;
	freeVals(vals, nvals);
	return new;
}

// get next tuple during a scan

Tuple getNextTuple(Query q)
{
	// TODO
	// Partial algorithm:
	Page page;
	while(TRUE){
		//Get current page.
		if(q->is_ovflow == 0){
			page = getPage(dataFile(q->rel), q->curpage);
		}else{
			page = getPage(ovflowFile(q->rel), q->curpage);
		}
		Offset offset = pageFreeOffset(page);
		if(q->curtup < offset){
			// if (more tuples in current page)
			//    get next matching tuple from current page
			while(q->curtup < offset){
				Tuple t = getPageTuple(page, q->curtup);
				if(tupleMatch(q->rel, t, q->query_tuple)){
					q->curtup += tupLength(t) + 1;
					Tuple result = malloc(strlen(t)*sizeof(Tuple));
					strcpy(result, t);
					free(page);
					return result;
				}
				q->curtup += tupLength(t) + 1;
			}
			free(page);
			continue;
		}
		if(pageOvflow(page) != NO_PAGE){
			// else if (current page has overflow)
			//    move to overflow page
			//    grab first matching tuple from page
			q->curpage = pageOvflow(page);
			q->curtup = 0;
			q->is_ovflow = 1;
			free(page);
			continue;
		}else{
			// else
			//    move to "next" bucket
			//    grab first matching tuple from data page
			free(page);
			page = NULL;

			int d = splitp(q->rel) > 0 ? depth(q->rel) + 1 :depth(q->rel);
			Bits mask = (1<<d) - 1;
			Bool flag = FALSE;
			if((mask & q->unknown) != 0){
				q->current_hash = q->current_hash & mask;
				q->known = q->known & mask;
				q->unknown = q->unknown & mask;

				if(q->current_hash != (q->known | q->unknown)){
					Bits bit;
					for(bit = q->current_hash + 1;bit<=(q->known | q->unknown);bit++){
						Bits new_bit = (bit & q->unknown) | q->known;
						if(new_bit != q->current_hash){
							q->current_hash = new_bit;
							q->curtup = 0;
							q->is_ovflow = 0;

							Bits p = getLower(new_bit, depth(q->rel));
							if(p<splitp(q->rel)){
								p = getLower(new_bit, depth(q->rel)+1);
							}
							if(p != q->curpage){
								q->curpage = p;
								flag = TRUE;
								break;
							}
						}
					}
				}
			}
			if(flag){
				continue;
			}else{
				break;
			}
		}
	}
	// if (current page has no matching tuples)
	//    go to next page (try again)
	// endif
	if(page != NULL) free(page);
	return NULL;
}

// clean up a QueryRep object and associated data

void closeQuery(Query q)
{
	// TODO
	free(q);
}
