#include<stdio.h>

int max(int, int);

int main(void){
    
    int a = 5, b = 10;

    printf("%d\n", max(a, b));

}

int max(int a, int b){
    if(a>b) return a;
    return b;
}


/*
//ogni volta che invoco una funzione viene allocato uno stack memoria, poi viene cancellata
//variabili non saranno visibili fuori

//i due parametri nel frame di attivazione della funzione main vengono copiati nel frame di
attivazione del frame della funzione max. Quindi posso cambiare a e b senza cambiare quelli
dentro max


//allocazione auto, fa il compilatore e calcola lo spazio necessario (non serve scrivere)
//sono tutte allocate nello stack del main

//extern sono nello scope globale, definite fuori dalle funzioni
//include variabili in altri file

se faccio
#include<...>
#include "max.h" //file usato come libreria

extern int countA;

...max(a,b)

//va compilato insieme a max.h
//va a cercare nel file, se non trova cerca in max.h
//la funzione max in max.h è automaticamente extern (per qualche motivo)
//se non trova cerca in max.c (per qualche motivo)


//se in max.C uso static per countA sono globali ma solo visibili alla funzione max


esempio
se faccio
prinf(..., max(a, b))

(in max.c) uso static per countA e countB

entrambe non vengono riallocate ogni volta, quindi ogni invocazione di max può accedere a quei
valori con quelli che aveva la chiamate precedente


SE INVECE utilizzo static per countA e countB in max.c (non dentro la funzione), non potrà
essere visualizzato dal main.c (non funziona extern int countA)



//quando passo variabile ai parametri di una funzione, viene copiata e vive fino alla fine della
funzione

//il tipo del puntatore indica il tipo della variabile a cui punta

*/