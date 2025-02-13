/*
//scanf serve per leggere gli input
//int scanf(char* format, ...)
//non si usa come stampa

//scanf("%d", ...) --> richiede un valore intero
//slide 29 per altri tipi


ES.


int main(void){
    int a, b;
    scanf("%d %d", &a, &b);
    printf("a=%d,b=%d\n", a, b);
    return 0;
}

OPPURE

int main (void){
    int a, b;
    int *aPtr=&a, *bPtr=&b;
    scanf("%d %d", aPrt, bPtr);
    printf("a=%d, b=%d\n", a, b);
    printf("aPtr=%p, bPtr=%p\n", aPtr, bPtr)
}


//posso usare i puntatori per scambiare variabili


void swap(int*, int*);

int main (void){
    int a = 5, b=10;
    
    printf("a=%d, b=%d\n", a, b);

    swap(&a, &b);

    printf("a=%d, b=%d\n", a, b);
}

void swap(int* x, int* y){
    int var=*x;
    *x=*y;
    *y = var;
}



//un array è un puntatore al primo elemento dell'array stesso

//un parametro di tipo puntatore può essere usato come array

int primo_elemento ( int* b) {
    return b[0];
}

int main ( void ){
    int d[10];
    return primo_elemento( d );
}


//gli array possono essere bidimensionali
//se sono passati come parametri deve essere specificata la seconda dimensione

es, int a[3][3]

int f([][3]){}

ES.

int main(void){
    int a[3][2] = {{1, 2}, {3, 4}, {5, 6}};

    printf("%p\n", a);
    printf("%p, %p, %p\n", a[0], a[1], a[2]) --> restituisce indirizzi

}



//si possono passare parametri al main (parametri  linea di comando)
come ./main 7 5 --> 7

int main (int argc, char* argv[])
    argc --> numero di parametri
    
    argv --> parametri stessi di stringhe
    funziona con puntatori

    es argv[0] è il nome dell parametro, argv[1] il primo parametro, argv[2] il secondo
    poi posso convertirli come voglio

    gli spazi non vengono considerati come tra main e 7





//NUOVE SLIDE





se definisco l'array a, a stesso è un puntantore
quindi a[10]

a == &(a[0]) --> true
invece &(a[1]) == ?, semplicemente l'indirizzo dell'elemento 1.

&(a[1]) == (a+1)
&(a[i]) == (a+i)

//sommare +1 ad un puntatore significa avanzare alla prossima cella di memoria
//varia in base al tipo del dato, esempio con interi sono 4 byte quindi avanza di 4 in memoria

a[1+1] = *(a+2) //stessa esatta cosa

//int a alloca spazio per un intero (sizeof(int))

ma int * aPtr; alloca spazio per un puntatore (sizeof(int*)) //4/8 byte in base ad architettura
dentro ha salvato l'indirizzo di a, aPtr = &a

un puntatore è un intero come valore, esadecimale

aPtr = (int*)malloc(sizeof(int));; assegna tot byte di memoria in ordine
//RIGUARDA..





*/

/*
#include<stdio.h>
#include <stdlib.h>

int main (int argc, char* argv){
    int i, j, lenx, leny, num_el_comuni=0;
    scanf("Immetti lunghezza del primo array: %d\n", &lenx);

    int primo_array[lenx];

    for (i=0; i<lenx; i++){
        scanf("Immetti un elemento del primo array: %d\n", );
    };
}

*/