// page.h ... interface to functions on Pages
// part of Multi-attribute Linear-hashed Files
// See pages.c for descriptions of Page type and functions
// Written by John Shepherd, April 2016

#ifndef PAGE_H
#define PAGE_H 1

#include "defs.h"
#include "tuple.h"

typedef struct PageRep *Page;

Page newPage();
PageID addPage(FILE *);
Page getPage(FILE *, PageID);
Status putPage(FILE *, PageID, Page);
Status addToPage(Page, Tuple);
Count pageTuples(Page);
Offset pageOvflow(Page);
void pageSetOvflow(Page, PageID);
Count pageFreeSpace(Page);

#endif
