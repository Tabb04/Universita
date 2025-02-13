#include <stdio.h>

//array multidimensionali
int main(void){
int n = 12345;
int cifre[10]; //non molto bello
int dim = 0; //numero elementi array

for(;n!=0; dim++, n/10){
    cifre[dim] = n%10;
}

for(int i=0; i<dim; i++){
    printf("%d\n", cifre[i]);
}


return 0;
}

//si possono definire tipi di dato custom
//strutture dati non omogenee usando parola chiave struct
/*
esempio

#define LEN 20
Struct Studente{
    char nome[LEN];
    int età;
};

e si inizializza come Struct Studente s = {"Antonio", 15}



un array occupa tanta memoria quanto la sua dimensione * dimensione del tipo



guarda file 2 inizializzazioni ------------------


//se ad esempio assergno s = {15, "Antonio"} da un errore di puntatore


//posso fare size of di Studente perchè è un tipo



//typedef può essere usato per rinominare i tipi, principalmente struct
typedef struct{...} Studente
ad esempio posso cambiare un tipo lungo come typedef unsigned long int ulint;

guarda file 2


//esercizi in file 3




//lo scoping delle funzioni è definito da dove sono dichiarate in poi


int max(int, int);

int main(void){
    ...
    prinf("%d\n", max(...))
}

int max(int a, int b){
    ...
}

//senza la prima riga la main non troverebbe la funzione max che ho dichiarato
//prima riga si chiama "Dichiarazione di funzione", diverso da definizione

//prototipo informa il compilatore sul numero di parametri e del loro tipo e del ritorno
//possono essere anche su file diversi le funzioni, ma vanno dichiarate


//in C i parametri sono passati per valore e non indirizzo


//prinf e scanf hanno una stringa di formattazione, primo valore, e in base a cosa c'è dopo la
percentuale influenza che tipo è
*/