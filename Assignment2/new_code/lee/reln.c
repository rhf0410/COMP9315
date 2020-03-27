// reln.c ... functions on Relations
// part of Multi-attribute Linear-hashed Files
// Written by John Shepherd, April 2016

#include "defs.h"
#include "reln.h"
#include "page.h"
#include "tuple.h"
#include "chvec.h"
#include "bits.h"
#include "hash.h"

#define HEADERSIZE (3*sizeof(Count)+sizeof(Offset))

struct RelnRep
{
	Count nattrs; // number of attributes
	Count depth;  // depth of main data file
	Offset sp;     // split pointer
	Count npages; // number of main data pages
	Count ntups;  // total number of tuples
	ChVec cv;     // choice vector
	char mode;   // open for read/write
	FILE *info;   // handle on info file
	FILE *data;   // handle on data file
	FILE *ovflow; // handle on ovflow file
};

// create a new relation (three files)

Status newRelation(char *name, Count nattrs, Count npages, Count d, char *cv)
{
	char fname[MAXFILENAME];
	Reln r = malloc(sizeof(struct RelnRep));
	r->nattrs = nattrs;
	r->depth = d;
	r->sp = 0;
	r->npages = npages;
	r->ntups = 0;
	r->mode = 'w';
	assert(r != NULL);
	if (parseChVec(r, cv, r->cv) != OK)
		return ~OK;
	sprintf(fname, "%s.info", name);
	r->info = fopen(fname, "w");
	assert(r->info != NULL);
	sprintf(fname, "%s.data", name);
	r->data = fopen(fname, "w");
	assert(r->data != NULL);
	sprintf(fname, "%s.ovflow", name);
	r->ovflow = fopen(fname, "w");
	assert(r->ovflow != NULL);
	int i;
	for (i = 0; i < npages; i++)
		addPage(r->data);
	closeRelation(r);
	return 0;
}

// check whether a relation already exists

Bool existsRelation(char *name)
{
	char fname[MAXFILENAME];
	sprintf(fname, "%s.info", name);
	FILE *f = fopen(fname, "r");
	if (f == NULL)
		return FALSE;
	else
	{
		fclose(f);
		return TRUE;
	}
}

// set up a relation descriptor from relation name
// open files, reads information from rel.info

Reln openRelation(char *name, char *mode)
{
	Reln r;
	r = malloc(sizeof(struct RelnRep));
	assert(r != NULL);
	char fname[MAXFILENAME];
	sprintf(fname, "%s.info", name);
	r->info = fopen(fname, mode);
	assert(r->info != NULL);
	sprintf(fname, "%s.data", name);
	r->data = fopen(fname, mode);
	assert(r->data != NULL);
	sprintf(fname, "%s.ovflow", name);
	r->ovflow = fopen(fname, mode);
	assert(r->ovflow != NULL);
	// BAD: assumes Count and Offset are the same size
	int n = fread(r, sizeof(Count), 5, r->info);
	assert(n == 5);
	n = fread(r->cv, sizeof(ChVecItem), MAXCHVEC, r->info);
	assert(n == MAXCHVEC);
	r->mode = (mode[0] == 'w' || mode[1] == '+') ? 'w' : 'r';
	return r;
}

// release files and descriptor for an open relation
// copy latest information to .info file
// note: we don't write ChoiceVector since it doesn't change

void closeRelation(Reln r)
{
	// make sure updated global data is put in info
	// BAD: assumes Count and Offset are the same size
	if (r->mode == 'w')
	{
		fseek(r->info, 0, SEEK_SET);
		// write out core relation info (#attr,#pages,d,sp)
		int n = fwrite(r, sizeof(Count), 5, r->info);
		assert(n == 5);
		// write out choice vector
		n = fwrite(r->cv, sizeof(ChVecItem), MAXCHVEC, r->info);
		assert(n == MAXCHVEC);
	}
	fclose(r->info);
	fclose(r->data);
	fclose(r->ovflow);
	free(r);
}

// insert a new tuple into a relation
// returns bucket where inserted
//   (may be a primary data page or overflow page)
// returns NO_PAGE if insert fails completely
// TODO: include splitting and file expansion

PageID addToRelation(Reln r, Tuple t)
{
	int nt = r->ntups + 1, na = r->nattrs;
	if (nt % (1024 / (10 * na)) == 0)
	{
		//Start spliting
		splitRelation(r);
	}

	Bits h, p;
	 char buf[40];
	h = tupleHash(r, t);
	p = getLower(h, r->depth);
	if (p < r->sp)
		p = getLower(h, r->depth + 1);
	 showBits(h,buf); printf("hash = %s\n",buf);
	// showBits(p,buf); printf("page = %s\n",buf);
	Page pg = getPage(r->data, p);
	if (addToPage(pg, t) == OK)
	{
		putPage(r->data, p, pg);
		r->ntups++;
		return p;
	}
	// primary data page full
	if (pageOvflow(pg) == NO_PAGE)
	{
		// add first overflow page in chain
		PageID newp = addPage(r->ovflow);
		pageSetOvflow(pg, newp);
		putPage(r->data, p, pg);
		Page newpg = getPage(r->ovflow, newp);
		// can't add to a new page; we have a problem
		if (addToPage(newpg, t) != OK)
			return NO_PAGE;
		putPage(r->ovflow, newp, newpg);
		r->ntups++;
		return p;
	} else
	{
		// scan overflow chain until we find space
		// worst case: add new ovflow page at end of chain
		Page ovpg, prevpg = NULL;
		PageID ovp, prevp = NO_PAGE;
		ovp = pageOvflow(pg);
		while (ovp != NO_PAGE)
		{
			ovpg = getPage(r->ovflow, ovp);
			if (addToPage(ovpg, t) != OK)
			{
				prevp = ovp;
				prevpg = ovpg;
				ovp = pageOvflow(ovpg);
			} else
			{
				if (prevpg != NULL)
					free(prevpg);
				putPage(r->ovflow, ovp, ovpg);
				r->ntups++;
				return p;
			}
		}
		// all overflow pages are full; add another to chain
		// at this point, there *must* be a prevpg
		assert(prevpg != NULL);
		// make new ovflow page
		PageID newp = addPage(r->ovflow);
		// insert tuple into new page
		Page newpg = getPage(r->ovflow, newp);
		if (addToPage(newpg, t) != OK)
			return NO_PAGE;
		putPage(r->ovflow, newp, newpg);
		// link to existing overflow chain
		pageSetOvflow(prevpg, newp);
		putPage(r->ovflow, prevp, prevpg);
		r->ntups++;
		return p;
	}

	return NO_PAGE;
}

void splitRelation(Reln r)
{
	addPage(r->data);
	r->npages++;
	//PageID addid = pid | setBit(0,r->depth);

	//Dummy approach to store all the tups stay in original page
	Tuple *tups_stay = malloc(1024 * sizeof(Tuple));

	PageID pid = r->sp;
	FILE * file = r->data;
	int index = 0;
	Offset curtup = 0;
	Count ntups = 0;

	while (pid != NO_PAGE)
	{
		Page pg = getPage(file, pid);

		if(pageTuples(pg) == 0){
			break;
		}
		Tuple tmp = nextTuple(file, pid, curtup);
		Bits hash, newid;

		// newid is always a data page id
		hash = tupleHash(r, tmp);
		newid = getLower(hash, r->depth + 1);
		//printf("tuple: [%s] ; newpid: [%d] ; pid: [%d]\n", tmp, newid, pid); //debug

		//if tuple should stay in original page
		if (newid == pid)
		{
			//printf("Put [%s] into tups_stay[%d]",tmp,index);  //debug
			tups_stay[index++] = strdup(tmp);
			//printf(" >>> finish\n"); //debug
		} else
		{
			//insert tuple in new page
			if (insertintoPage(r, tmp, newid) == NO_PAGE)
				printf("Alarm!!!!!  Insert [%s] to NEW page %d go wrong\n", tmp,
						newid);
			//printf("  >>>  finish\n");//debug
		}
		curtup += strlen(tmp)+1;
		ntups++;

		// cur page has no more tuples
		if (ntups >= pageTuples(pg))
		{

			//use a new empty page to cover the old one
			Page cover = newPage();
			putPage(file, pid, cover);

			pid = pageOvflow(pg);
			ntups = 0;
			curtup = 0;
			file = r->ovflow;
			printf("Move to overflow %d\n",pid);
		}
		free(tmp);

	}

	int i;
	for (i = 0; i < index; ++i)
	{
		//printf("insert tups_stays[%d]\n",index);//debug
		Tuple tmp = tups_stay[i];
		if (insertintoPage(r, tmp, r->sp) == NO_PAGE)
			printf("Alarm!!!!!  Inserting [%s] to page %d go wrong\n", tmp, r->sp);
		//printf("  >>>  finish\n");//debug
		free(tmp);
	}

	free(tups_stay);
	if (getLower(r->sp + 1, r->depth) != 0)
	{
		r->sp++;
	} else
	{
		r->depth++;
		r->sp = 0;
	}
	//printf("Split OK/n");   //debug

}

Status insertintoPage(Reln r, Tuple t, PageID pid)
{
	//debug
	//printf("Start Inserting [%s] to page %d", t, pid);
	Page pg = getPage(r->data,pid);

	if (addToPage(pg, t) == OK)
	{
		putPage(r->data, pid, pg);
		return pid;
	}
	// primary data page full
	if (pageOvflow(pg) == NO_PAGE)
	{
		// add first overflow page in chain
		PageID newp = addPage(r->ovflow);
		pageSetOvflow(pg, newp);
		putPage(r->data, pid, pg);
		Page newpg = getPage(r->ovflow, newp);
		// can't add to a new page; we have a problem
		if (addToPage(newpg, t) != OK)
			return NO_PAGE;
		putPage(r->ovflow, newp, newpg);
		return pid;
	} else
	{
		// scan overflow chain until we find space
		// worst case: add new ovflow page at end of chain
		Page ovpg, prevpg = NULL;
		PageID ovp, prevp = NO_PAGE;
		ovp = pageOvflow(pg);
		while (ovp != NO_PAGE)
		{
			ovpg = getPage(r->ovflow, ovp);
			if (addToPage(ovpg, t) != OK)
			{
				prevp = ovp;
				prevpg = ovpg;
				ovp = pageOvflow(ovpg);
			} else
			{
				if (prevpg != NULL)
					free(prevpg);
				putPage(r->ovflow, ovp, ovpg);
				return pid;
			}
		}
		// all overflow pages are full; add another to chain
		// at this point, there *must* be a prevpg
		assert(prevpg != NULL);
		// make new ovflow page
		PageID newp = addPage(r->ovflow);
		// insert tuple into new page
		Page newpg = getPage(r->ovflow, newp);
		if (addToPage(newpg, t) != OK)
			return NO_PAGE;
		putPage(r->ovflow, newp, newpg);
		// link to existing overflow chain
		pageSetOvflow(prevpg, newp);
		putPage(r->ovflow, prevp, prevpg);
		return pid;
	}
}

// external interfaces for Reln data

Count nattrs(Reln r)
{
	return r->nattrs;
}
Count npages(Reln r)
{
	return r->npages;
}
Count ntuples(Reln r)
{
	return r->ntups;
}
Count depth(Reln r)
{
	return r->depth;
}
Count splitp(Reln r)
{
	return r->sp;
}
ChVecItem *chvec(Reln r)
{
	return r->cv;
}
FILE *fdata(Reln r)
{
	return r->data;
}
FILE *fovflow(Reln r)
{
	return r->ovflow;
}
FILE *finfo(Reln r)
{
	return r->info;
}

// displays info about open Reln

void relationStats(Reln r)
{
	printf("Global Info:\n");
	printf("#attrs:%d  #pages:%d  #tuples:%d  d:%d  sp:%d\n", r->nattrs,
			r->npages, r->ntups, r->depth, r->sp);
	printf("Choice vector\n");
	printChVec(r->cv);
	printf("Bucket Info:\n");
	printf("%-3s %s\n", "#", "Info on pages in bucket");
	printf("%-3s %s\n", "", "(pageID,#tuples,freebytes,ovflow)");
	Offset pid;
	for (pid = 0; pid < r->npages; pid++)
	{
		printf("%-3d ", pid);
		Count tups, space, ntups = 0;
		Page p = getPage(r->data, pid);
		tups = pageTuples(p);
		ntups += tups;
		space = pageFreeSpace(p);
		Offset ovid = pageOvflow(p);
		printf("(d%d,%d,%d,%d)", pid, tups, space, ovid);
		free(p);
		while (ovid != NO_PAGE)
		{
			Offset curid = ovid;
			p = getPage(r->ovflow, ovid);
			tups = pageTuples(p);
			ntups += tups;
			space = pageFreeSpace(p);
			ovid = pageOvflow(p);
			printf(" -> (ov%d,%d,%d,%d)", curid, tups, space, ovid);
			free(p);
		}
		putchar('\n');
	}
}
