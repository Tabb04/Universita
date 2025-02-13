#include <stdio.h>
#include <stdbool.h>
#include <threads.h>
#include <stdlib.h>

#define N 5 //numero threads

//IN QUESTO PROGRAMMA CREO 5 THREADS CHE POI FACCIO DETATCHED.
//DOPO PROVO A JOINARE MA DA ERRORE 
//DOVREBBE ASPETTARE CHE TUTTI FINISCANO MA TERMINA PRIMA
//LA JOIN SERVE AD ASPETTARE CHE TUTTI I THREADS ABBIANO TERMINATO
//Commentando la detatch funziona correttamente



int thread_func(void * arg){
    thrd_t id = thrd_current();
    int tempo_rnd = 1+(rand() % 4);

    printf("Inizio thread %lu, resta attivo per %d secondi\n", id, tempo_rnd);
    struct timespec ts = { .tv_sec = tempo_rnd, .tv_nsec = 0 };
    thrd_sleep(&ts, NULL);
    
    printf("Fine thread %lu\n", id);
    return thrd_success;
}

int main (){
    thrd_t threads[N];  //array di threads

    for (int i = 0; i<N; i++){
        thrd_create(
            &threads[i],    //dove viene memorizzato l'identificatore del thread
            thread_func,
            NULL            //dati passati alla funzione
        );
        printf("Thread %lu creato.\n", threads[i]);

        fflush(stdout);
        //thrd_detach(threads[i]);    //fa il threads detatched, sarà gestito dal sistema operativo
                                    //non deve esplicitamente essere terminato dunque
    }

    for(int i = 0; i<N; i++){

        int ret = thrd_join(threads[i], NULL);
        if (ret == thrd_success){
            printf("Thread %lu terminato correttamente.\n", threads[i]);
        }else{
            printf("Errroe sul join del thread %lu\n", threads[i]);
        }
    }

    printf("Termine programma.\n");
    return 0;

}