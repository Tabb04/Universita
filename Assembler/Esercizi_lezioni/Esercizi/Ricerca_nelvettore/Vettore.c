#include <stdio.h>
#include <stdlib.h>

extern int cerca(int x, int* v, int n);

int main(int argc, char** argv){
    int n = atoi(argv[1]);  //lunghezza vettore
    int x = atoi(argv[2]);  //elemento da cercare
    int* v = (int*)malloc(sizeof(int) * n);
    int i, res;

    srand(123); //inizializzazione
    for(i=0; i<n; i++){
        v[i] = rand()%n;
    }
    printf("Cerco %d nel vettore: \n", x);
    for(i=0; i<n; i++){
        printf("%d ", v[i]);
    }
    printf("\n");

    res = cerca(x, v, n);
    if(res!=-1){
        printf("%d trovato nella posizione %d\n", x, res);
    }

    return (0);


}

