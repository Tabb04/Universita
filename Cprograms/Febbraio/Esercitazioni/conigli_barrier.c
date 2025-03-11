#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <threads.h>
#include <time.h>
#include <stdbool.h>


#define NUM_CONIGLI 5
#define LUNG_GARA 20

mtx_t mutex;
cnd_t partenza;
int count = 0;

typedef struct{
    int id;
    int distanza;
}Conigli;

int barrier(){
    mtx_lock(&mutex);
    count++;
    if (count == NUM_CONIGLI){
        count = 0;
        cnd_broadcast(&partenza);
        printf("VIA!\n");
        mtx_unlock(&mutex);
        return 1;
    }else{
        cnd_wait(&partenza, &mutex);
        mtx_unlock(&mutex);
        return 0;
    }
}




int corsa(void * arg){
    Conigli *c = (Conigli*)arg;

    printf("Coniglio %d pronto!\n", c->id);
    barrier();
    do{
        int passo = rand() % 5 + 1;
        c->distanza+=passo;
        printf("Il coniglio %d è a %d metri!\n", c->id, c->distanza);
        thrd_sleep(&(struct timespec){0, 500000000L}, NULL);

    }while(c->distanza < LUNG_GARA);
    printf("Coniglio %d taglia il traguardo!\n", c->id);
    return 0;
}



int main(){
    
    thrd_t Threads[NUM_CONIGLI];
    Conigli Valori[NUM_CONIGLI];
    
    mtx_init(&mutex, mtx_plain);
    cnd_init(&partenza);

    for(int i = 0; i<NUM_CONIGLI; i++){
        Valori[i].id = i+1;
        Valori[i].distanza = 0;
        thrd_create(&Threads[i], corsa, &Valori[i]);
    }

    for(int i = 0; i<NUM_CONIGLI; i++){
        thrd_join(Threads[i], NULL);
    }
    mtx_destroy(&mutex);
    cnd_destroy(&partenza);
    return 0;
}
