#include <stdio.h>
#include "tuple.h"
#include "tuple.c"
#include "reln.h"
#include "reln.c"
#include "chvec.h"
#include "chvec.c"
#include "page.h"
#include "page.c"
#include "hash.h"
#include "hash.c"
#include "bits.h"
#include "bits.c"

int main() {
    char *chs = "0,0:0,1:1,0:1,1:2,0:3,0";
    newRelation("test2", 4, 6, 3, chs);
    printf("Hello World\n");
//    Bool flag = existsRelation("abc");
//    printf("%d\n", flag);
//    Reln reln = openRelation("abc", "w");
//    printf("%d\n", reln->nattrs);
    return 0;
}