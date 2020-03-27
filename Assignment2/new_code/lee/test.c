#include<stdio.h>
#include<string.h>
#include<stdlib.h>

typedef char *Tuple;

int main(){
    Tuple a = strdup("abcdegf");
    a[7] = 'Q';
    printf("%c     %s\n",a[7],a);
    
    Tuple *tups=malloc(2*sizeof(Tuple));
//   char **tups = malloc(2*sizeof(char*));
    tups[0] = strdup("hahaha");
    tups[1] = strdup("hehehe");
    printf("%s    %s\n",tups[0],tups[1]);

    free(tups[0]);
    free(tups[1]);
    free(a);
    printf("over\n");
    return 1;
}
