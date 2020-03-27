// query.c ... query scan functions
// part of Multi-attribute Linear-hashed Files
// Manage creating and using Query objects
// Written by John Shepherd, April 2016

#include "defs.h"
#include "query.h"
#include "reln.h"
#include "tuple.h"
#include "hash.h"

// A suggestion ... you can change however you like

struct QueryRep
{
	Reln rel;       // need to remember Relation info
	Bits known;     // the hash value from MAH
	Bits unknown;   // the unknown bits from MAH
	Bits stbucket;   // start bucket value
	Tuple qstring;
	int depth;
	Count count;      //count how many unknown bits in the certain depth

	PageID curpage;   // current page in scan
	int is_ovflow; // are we in the overflow pages?
	Offset curtup;    // offset of current tuple within page
	Count ntup;     // number of tuples scanned in curpage
	Bits option;    // cur combination of unknown bits
//TODO
};

PageID getpage(Query q) { return q->curpage; }
Offset gettup(Query q) { return q->curtup; }

// take a query string (e.g. "1234,?,abc,?")
// set up a QueryRep object for the scan

Query startQuery(Reln r, char *q)
{
	Query new = malloc(sizeof(struct QueryRep));
	assert(new != NULL);
	new->rel = r;
	new->is_ovflow = 0;

	Count nvals = nattrs(r);
	//char *qu = strdup(q);
	new->qstring = strdup(q);
	//char *de = ",";
	char *vals[nvals];
	Bits hash[nvals];
	int i;
	int attrknow[nvals];
	char buf[35]; // for debug

	tupleVals(q,vals);
	for (i = 0; i < nvals; i++)
	{
		//vals[i] = strsep(&qu, de);
		if (vals[i] == NULL)
		{
			fatal("", "Wrong number of attribute");
		}

		attrknow[i] = strcmp(vals[i], "?");
		if (attrknow[i])
		{
			hash[i] = hash_any((unsigned char *) vals[i], strlen(vals[i]));
		} else
		{
			hash[i] = 0x00000000;
		}

		showBits(hash[i],buf);
		printf("hash\"%s\" is %s\n",vals[i],buf);
	}
	//free(qu);

	Bits qhash = 0xFFFFFFFF;
	Bits nknow = 0x00000000;
	ChVecItem *cv = chvec(r);
	Byte bit;
	Byte att;
	Bits mask = 0x00000000;
	//Not tested
	for (i = 0; i < 32; ++i)
	{
		mask = 0x00000000;
		att = cv[i].att;
		bit = cv[i].bit;
		if (!attrknow[att])
			nknow = setBit(nknow, i);
		mask = setBit(mask, bit);
		if ((hash[att] & mask) == 0)
			qhash = unsetBit(qhash, i);
	}

	new->known = qhash;
	showBits(qhash,buf);
	printf("qhash is %s\n",buf);

	new->unknown = nknow;
	//showBits(nknow,buf);
	//printf("nknow is %s\n",buf);

	mask = 0x00000000;
	int d = depth(r);
	for (i = 0; i < d; ++i)
	{
		mask = setBit(mask, i);
	}

	PageID id = qhash & mask;
	new->depth = depth(r);
	if (id < splitp(r))
	{
		mask = setBit(mask, d);
		id = qhash & mask;
		new->depth++;
	}

	int count = 0;
	for (i = 0; i < 31; i++)
	{
		if (i >= new->depth)
			break;
		if (nknow & (0x00000001 << i))
			count++;
	}


	new->count = count;
	new->stbucket = id;
	new->curpage = id;
	new->curtup = 0;
	new->ntup = 0;
	new->option = 0x00000000;

	//debug
//	showBits(id, buf);
//	printf("ID        %s\n", buf);
//	printf("start q OK\n");
	return new;
}

// get next tuple during a scan

Tuple getNextTuple(Query q)
{

	Reln r = q->rel;
	Count nvals = nattrs(r);
	while (1)
	{
		PageID pid = q->curpage;
		Page p;

		FILE *file;
		if (q->is_ovflow)
		{
			file = fovflow(r);
		} else
		{
			file = fdata(r);
		}

		p = getPage(file, pid);
		//scan the cur page until there is no left tuples
		//return if find match
		while (q->ntup < pageTuples(p))
		{
			Tuple tmp = nextTuple(file,q->curpage,q->curtup);
			q->ntup++;
			q->curtup = q->curtup + strlen(tmp) + 1;
			//printf("all tuples : %s   %d\n", tmp, strlen(tmp));	//debug
			if (tupleMatch(q->qstring, tmp, nvals))
				return tmp;
			free(tmp);
		}

		//switch to next page or overflow
		Offset ovflw = pageOvflow(p);
		if (ovflw != -1)
		{
			q->curpage = ovflw;
			q->ntup = 0;
			q->curtup = 0;
			q->is_ovflow = 1;
			continue;
		} else
		{
			Bits uknow = q->unknown;
			Bits op = q->option;
			op++;
			int i;
			int count = q->count;

			if (op >= (0x00000001 << count))
			{
				// including three cases:
				// 1) op from 0x11111111 to 0x00000000, count == 8
				// 2) op from 0x00000000 to 0x00000001, count == 0 (no unknown bits)
				// 3) op from 0x0000000F to 0x00000010, count == 4 (normal case)
				break;
			}
			q->option = op;

			int offset = 0;
			Bits mask = 0;
			for (i = 0; i < count; ++i)
			{
				while (!(uknow & (setBit(0, offset))))
					offset++;

				if (op & setBit(0, i))
					mask = setBit(mask, offset);

				offset++;
			}
			PageID id = q->stbucket | mask;

			if(id > npages(r)-1)
				break;
			//debug
//			char buf[35];
//			showBits(q->curpage, buf);
//			printf("from page\n    %s\n", buf);
//			showBits(id, buf);
//			printf("     to\n    %s\n", buf);

//
//			showBits(q->stbucket, buf);
//			printf("stbucket\n    %s\n ", buf);
//			showBits(mask, buf);
//			printf("mask\n    %s\n   op = %d    count = %d   depth = %d\n", buf,op,count,q->depth);

			q->curpage = id;
			q->ntup = 0;
			q->curtup = 0;
			q->is_ovflow = 0;

		}
	}

	return NULL;
}

// clean up a QueryRep object and associated data

void closeQuery(Query q)
{
	// TODO
	free(q);
}
