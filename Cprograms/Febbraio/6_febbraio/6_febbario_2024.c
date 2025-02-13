#include <stdio.h>
#include <threads.h>
//possibile dover utilizzare docker container

#define N 5

int thread_func(void *arg){
    thrd_t id_ricevuto = *(thrd_t *)arg; //recupera l'id del thread
    thrd_t id_calcolato = thrd_current();
    printf("Thread ID ric:  %lu\n", id_ricevuto); //stampa l'id del thread
    printf("Thread ID cal:  %lu\n", id_ricevuto); //stampa l'id del thread

}

int main(){

    thrd_t threads[N];  //array descrittori del thread

    for(int i = 0; i<N; i++){
        thrd_create(        //crea un thread
            &threads[i],    //id del thread
            thread_func,    //funzione del thread
            &threads[i]     //argomento della funzione
        );
    }

    for (int i = 0; i<N; i++){
        thrd_t ret = thrd_join(threads[i], NULL); //attende terminazione del thread 1
        if (ret == thrd_success)
            printf("Thread con ID: %lu terminato correttamente.\n", threads[i]);
        else
            printf("Erorre nel join del thread %d\n", i);
    }
    return 0;
}


