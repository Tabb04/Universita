#include <stdio.h>

int main (void){
    int primo_array[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    
    char nome[10] = "Luca"; //non uso parentesi perchè una stringa è già un array di caratteri
    
    //devo usare virgolette doppie non ''
    
};



//se dichiaro una struttura
struct Numeri{
    int a[10];
    float b;
};

//allora potevo fare
struct Numeri num = {{0, 1, 2, 3.7 /*ecc*/}, 2.5};

//se voglio accedere ad un campo della variabile num uso num.a
//posso anche assegnare ad uno dei campi con num.b = 2.4
//se faccio num = 5 non funziona


//posso cambiare Numeri usando typedef
//typedef unsigned Numeri Numeri; facendo così si chiama solo Numeri e non struct Numeri
