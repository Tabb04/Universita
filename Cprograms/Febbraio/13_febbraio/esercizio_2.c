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

// ... struct coniglio
