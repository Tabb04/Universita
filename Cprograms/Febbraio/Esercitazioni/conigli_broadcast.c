#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <threads.h>
#include <time.h>
#include <stdbool.h>

#define NUM_CONIGLI 5
#define LUNG_GARA 20

cnd_t partenza;

typedef struct {
    mtx_t mutex;
    int id;
    int distanza;
} Dati;

int corsa(void * arg){
    Dati *d = (Dati*)arg;
    cnd_wait(&partenza, &d->mutex);
    //barrier_wait(&barriera); // Aspetta che tutti i thread siano pronti

    while(d->distanza < LUNG_GARA){
        int passo = rand() % 5 + 1;
        d->distanza+=passo;
        printf("Coniglio %d è a %d metri!\n", d->id, d->distanza);
        thrd_sleep(&(struct timespec){0, 200000000L}, NULL); //pausa di 200ms
    }
    printf("🐰 Coniglio %d ha tagliato il traguardo\n", d->id);
    mtx_unlock(&d->mutex);
    mtx_destroy(&d->mutex);
    return 0;
}

int main(){
    thrd_t Conigli[NUM_CONIGLI];
    Dati parametri[NUM_CONIGLI];
    cnd_init(&partenza);
    srand(time(NULL));
    //barrier_init(&barriera, NUM_CONIGLI + 1); // Inizializza la barriera con il numero di thread + 1 (main thread)
    
    for(int i = 0; i<NUM_CONIGLI; i++){
        mtx_init(&parametri[i].mutex, mtx_plain);
        parametri[i].id = i+1;
        parametri[i].distanza = 0;
        thrd_create(&Conigli[i], corsa, &parametri[i]);
    }

    thrd_sleep(&(struct timespec){0, 500000000L}, NULL); // Pausa 500ms
    printf("VIA! 🏁\n");
    cnd_broadcast(&partenza);
    //barrier_wait(&barriera); // Rilascia tutti i thread

    for(int i = 0; i<NUM_CONIGLI; i++){
        thrd_join(Conigli[i], NULL);
    }
    printf("FINITO!\n");
    cnd_destroy(&partenza);
    //  barrier_destroy(&barriera);
    return 0;
 
}