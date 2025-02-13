/*
programma che legge da tastiera una sequenza di numeri interi e ne stampi la somma

input deve esserere una sequenza di n numeri interi, un intero per riga
il programma stampa in output la somma degli interi*/

#include <stdio.h>

int main (void){

    int somma = 0, n, attuale; //così dichiaro 3 variabili e ne inizializzo una

    scanf("Numero di addendi: %d\n", &n); //si aspetta un numero, lo scrive nella variabile n
    //ciò che viene letto viene letto come un intero

    for(int i = 0; i<n; i++){ //fuori dal ciclo for quella i non esiste
        scanf("%d", &attuale);
        somma +=attuale;
    }

    printf("%d\n", somma);
    return 0;

};




//NON FUNZIONA, PROF SPIEGHERÀ...
//COMMENTATO IL PRIMO, ORA ES 2

/*

//cerca elementi comuni tra 2 array

#include <stdio.h>
#include <stdlib.h>

int main (void){
    int i, j, lenx, leny, num_el_comuni=0;
    scanf("Immetti lunghezza del primo array: "%d\n", &lenx);




    scanf("Immetti lunghezza del secondo arrray: "%d\n", &leny)


    //porcoddio va veloce
}
*/
