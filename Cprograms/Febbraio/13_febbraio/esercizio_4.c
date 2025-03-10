//frigo con 5 bottiglie e 4 consumatori?
//non si può mettere se pieno e consumare se vuoti
//serve una mutex (accesso esclusivo al frigo)
//due condition variables per evitare di mettere quando è pieno e prendere se vuoto
//quando riempono dicono ai consumatori di consumare e viceversa


#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>

#define NUM_PRODUTTORI 5
#define NUM_CONSUMATORI 5
#define DIM_BUFFER 5
#define MAX_INTERAZIONI 10

int buffer = 0;
mtx_t mutex;
cnd_t non_pieno;
cnd_t non_vuoto;

int produzione(void *arg){
    
    int id = *(int *)arg;
    int num = 0;
    
    while(num < MAX_INTERAZIONI){
        
        thrd_sleep(&(struct timespec){0, (rand() % 3 + 1) * 500000000L}, NULL);
        mtx_lock(&mutex);
        while(buffer == DIM_BUFFER){
            printf("💤 Produttore %d si mette a dormire 💤!\n", id);
            cnd_wait(&non_pieno, &mutex);
        }
        buffer++;
        printf("🥤 Produttore %d mette una bibita! Totale: %d\n", id, buffer);
        num+=1;
        cnd_broadcast(&non_vuoto);
        mtx_unlock(&mutex);
    }
    
    return 0;
}

int consumo(void* arg){
    int id = *(int *)arg;
    int num = 0;
    
    while(num < MAX_INTERAZIONI){
        
        thrd_sleep(&(struct timespec){0, (rand() % 3 + 1) * 500000000L}, NULL);
        mtx_lock(&mutex);
        while(buffer == 0){
            printf("💤 Consumatore %d si mette a dormire 💤!\n", id);
            cnd_wait(&non_vuoto, &mutex);
        }
        buffer--;
        printf("🥤 Consumatore %d prende una bibita! Totale: %d\n", id, buffer);
        num+=1;
        cnd_broadcast(&non_pieno);
        mtx_unlock(&mutex);
    }
    
    return 0;
}



int main(){
    thrd_t Produttori[NUM_PRODUTTORI];
    thrd_t Consumatori[NUM_CONSUMATORI];
    int id_prod[NUM_PRODUTTORI];
    int id_cons[NUM_CONSUMATORI];

    mtx_init(&mutex, mtx_plain);
    cnd_init(&non_pieno);
    cnd_init(&non_vuoto);

    for (int i = 0; i<NUM_PRODUTTORI; i++){
        id_prod[i] = i+1;
        thrd_create(&Produttori[i], produzione, &id_prod[i]);
    }

    for (int i = 0; i<NUM_CONSUMATORI; i++){
        id_cons[i] = i+1;
        thrd_create(&Consumatori[i], consumo, &id_cons[i]);
    }

    for(int i = 0; i<NUM_PRODUTTORI; i++){
        thrd_join(Produttori[i], NULL);
    }

    for(int i = 0; i<NUM_CONSUMATORI; i++){
        thrd_join(Consumatori[i], NULL);
    }

    mtx_destroy(&mutex);
    cnd_destroy(&non_pieno);
    cnd_destroy(&non_vuoto);
    
    printf("FINITO!\n");
    return 0;
}