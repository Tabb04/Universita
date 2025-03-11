#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>

#define M_PRODUTTORI 3
#define N_CONSUMATORI 4
#define K_BUFFER 5
#define ITERAZIONI 5

int buffer = 0;  // Numero di bibite nel distributore
mtx_t mutex;     //una mutex e due condition variables
cnd_t non_pieno;    
cnd_t non_vuoto;

int Funzione_produttore(void * arg){
    int id =*(int*)arg;
    int iterzioni_thread = 0;

    while(iterzioni_thread < ITERAZIONI){
        thrd_sleep(&(struct timespec){0, (rand() % 3 + 1) * 500000000L}, NULL);

        mtx_lock(&mutex);
        while(buffer == K_BUFFER){
            printf("Produttore %d si mette a dormire\n", id);
            cnd_wait(&non_pieno, &mutex);
        }
        buffer++;
        printf("Prodotture %d ha aggiunto, totale: %d\n",id, buffer);
        printf("Iterazione produttore %d num %d\n\n", id, iterzioni_thread);
        
        iterzioni_thread++;
        cnd_broadcast(&non_vuoto);
        mtx_unlock(&mutex);
    }
    return 0;
}

int Funzione_consumatore(void * arg){
    int id =*(int*)arg;
    int iterzioni_thread = 0;

    while(iterzioni_thread < ITERAZIONI){
        thrd_sleep(&(struct timespec){0, (rand() % 3 + 1) * 500000000L}, NULL);

        mtx_lock(&mutex);
        while(buffer == 0){
            printf("Consumatore %d si mette a dormire\n", id);
            cnd_wait(&non_vuoto, &mutex);
        }
        buffer--;
        printf("Consumatore %d ha consumato, totale: %d\n",id, buffer);
        printf("Iterazione consumatore %d num %d\n\n", id, iterzioni_thread);
        iterzioni_thread++;
        cnd_broadcast(&non_pieno);
        mtx_unlock(&mutex);
    }
    return 0;
}


int main(){
    srand(time(NULL));
    thrd_t Produttori[M_PRODUTTORI];
    int id_prod[M_PRODUTTORI];
    thrd_t Consumatori[N_CONSUMATORI];
    int id_cons[N_CONSUMATORI];

    mtx_init(&mutex, mtx_plain);
    cnd_init(&non_pieno);
    cnd_init(&non_vuoto);

    for(int i = 0; i<M_PRODUTTORI; i++){
        id_prod[i] = i+1;
        thrd_create(&Produttori[i], Funzione_produttore, &id_prod[i]);
    }

    for(int i = 0; i<N_CONSUMATORI; i++){
        id_cons[i] = i+1;
        thrd_create(&Consumatori[i], Funzione_consumatore, &id_cons[i]);
    }

    for(int i = 0; i<M_PRODUTTORI; i++){
        thrd_join(Produttori[i], NULL);
    }

    for(int i = 0; i<N_CONSUMATORI; i++){
        thrd_join(Consumatori[i], NULL);
    }

    mtx_destroy(&mutex);
    cnd_destroy(&non_pieno);
    cnd_destroy(&non_vuoto);

    return 0;

}