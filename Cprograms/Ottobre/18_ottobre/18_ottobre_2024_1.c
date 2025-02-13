/*
scrivere un programma che calcoli il prodotto scalare do in array di interi A, la cui dimensione e leemnti devono essere chiesti
in input all'utente. Il programma deve invertire l'array senza utilizzare un array di appoggio, ossia
scambiare il contenuto della prime e ultima e così via. Infine stampare a video ogni singolo
elemento dell'array modificato
*/
/*
int main(){

    int i, j, scambio;
    int A[10];


    //non scanf ma statico qui
    for (i=0; i<10; i++){
        A[i]=i;
    };
    //scorro array fino a metà per invertire

    for(i=0; i<10/2; i++){
        j=(10-1)-i;
        scambio = A[j];
        A[j]=A[i];
        A[i] = scambio;
    };

    //output
    for (i=0; i<10; i++){
        printf("%d\n", A[i]);
    };
    return 0;
}
*/

/*scrivere un programma che accetti in input due array di interi distinti e restituisca in output
il numero di elementi comuni ad entrambi gli array. Si assuma che la lunghezza di ogni array sia fornita 
prima dell'immissione degli elementi. presupponi che nel primo array siano tutti valori diversi

L'input è formato da:
-dimensione del primo array
-lista dei valori distinti del primo array
-dimensione del secondo array
-lista dei valori distinti del secondo array
l'unica riga dell'output contiene il numeri di elementi in comune tra il primo e e il secondo array
s
*/
#include<stdio.h>
int main(){

    int A[]={0, 1, 2, 3, 4};
    int B[]={0, 0, 3, 10, 10, 15, 15};
    int i, j, count;

    for(i=0; i<5; i++){
        for(j=0; j<7; j++){
            if (A[i]==B[j]) {count++;}
        };
    };
    return 0;
}
