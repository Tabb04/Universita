//voglio simulare una corsa tra conigli
//ogni coniglio è un thread che corre avanzando casualmenmte
//la funzione thrd_join permette di aspettare che tutti i conigli finiscano la corsa
//vince il coniglio che raggiunge per primo il traguardo

//devo fare in modo che tutti partano allo stesso tempo
//faccio partire tutti i thread e faccio leggere a tutti una variabile atomica

//dal main la cambio e lascio partire i thread
//non totalmente fair perchè la prima che la cambia parte teoricamente prima
//spinlock per cordinamento

//nel mio caso potrebbe esserci false sharing visto che i thread stanno in un vettore
//il compilatore da solo potrebbe aggiungere padding per evitare che le linee di cache
//prendano lo stesso spazio di memoria

//un coniglio potrebbe non avere vinto ma stampa per primo
//se uso una struttura per determinare chi vince la devo proteggere da accessi multipli

#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <threads.h>
#include <time.h>
#include <stdbool.h>

#define NUM_CONIGLI 5
#define TRAGUARDO 50

atomic_bool via = false;        //atomica per la partezna

typedef struct {
    int id;
    int distanza;
} Coniglio;

void gara_conigli(void* arg){               //potevo fare Coniglio * c
    Coniglio *c = (Coniglio *)arg;          //la funzione prende void ma mi serve tipo coniglio
    while(!via) {}      //busy wait

    //gara
    while (c->distanza < TRAGUARDO){
        int passo = rand()%5 + 1;       //passo tra 1 e 5
        c->distanza += passo;
        printf("Il coniglio %d ha corso fino a %d metri\n", c->id, c->distanza);
        thrd_sleep(&(struct timespec){0, 200000000L}, NULL); //pausa di 200ms
    }   
    printf("🐰 Coniglio %d ha tagliato il traguardo\n", c->id);
    return;
}


int main (void){
    srand(time(NULL));
    thrd_t threads[NUM_CONIGLI];
    Coniglio conigli[NUM_CONIGLI];

    //creo thread
    for (int i = 0; i<NUM_CONIGLI; i++){
        conigli[i].id = i;
        conigli[i].distanza = 0;
        thrd_create(&threads[i], gara_conigli, &conigli[i]);
    }

    thrd_sleep(&(struct timespec){0, 500000000L}, NULL); // Pausa 500ms
    printf("VIA! 🏁\n");
    via = true;         //operazione che la libreria atomics mi assicura sia fatta atomicamente
    

    for(int i = 0; i<NUM_CONIGLI; i++){
        thrd_join(threads[i], NULL);
    }

    printf("\n\n GARA FINITA!🎉\n");

    return 0;
}