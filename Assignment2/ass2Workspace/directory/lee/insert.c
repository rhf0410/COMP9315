// insert.c ... add tuples to a relation
// part of Multi-attribute linear-hashed files
// Reads tuples from stdin and inserts into Reln
// Usage:  ./insert  [-v]  RelName
// Written by John Shepherd, April 2016

#include "defs.h"
#include "reln.h"
#include "tuple.h"

#define USAGE "./insert  [-v]  RelName"

// Main ... process args, read/insert tuples

int main(int argc, char **argv)
{
	Reln r;  // handle on the open relation
	Tuple t;  // tuple buffer
	char err[MAXERRMSG];  // buffer for error messages
	char tup[MAXTUPLEN];  // buffer for printable tuples
	int verbose;  // show extra info on query progress
	char *rname;  // name of table/file

	// process command-line args

	if (argc < 2) fatal(USAGE, "");
    if (strcmp(argv[1], "-v") == 0)
		{ verbose = 1; rname = argv[2]; }
	else
		{ verbose = 0; rname = argv[1]; }


	// set up relation for writing

	if ((r = openRelation(rname,"r+")) == NULL) {
		sprintf(err, "Can't open relation: %s",argv[1]);
		fatal("", err);
	}

	// read stdin and insert tuples

	while ((t = readTuple(r,stdin)) != NULL) {
		PageID pid;
		showTuple(t,tup); // printable version
		printf("%s\n", tup);
		pid = addToRelation(r,t);
		if (pid == NO_PAGE) {
			//sprintf(err, "Insert of %s failed\n", tup);
			fatal("",err);
		}
		if (verbose) printf("%s -> %d\n",tup,pid);
		free(t);
	}

	// clean up

	closeRelation(r);

	return 0;
}
