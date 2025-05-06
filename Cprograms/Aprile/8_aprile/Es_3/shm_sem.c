#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#define MULTITHREAD_VERSION
//#undef MULTITHREAD_VERSION

#ifdef MULTITHREAD_VERSION
#include <threads.h>
#define NUM_THREADS 5
#endif

#include "Syscalls.h"
#define SHM_NOME "/mem_condivisa1"
#define NUM_ITERAZIONI 5




typedef struct{
    sem_t sem1;
    int counter;
}shared_data;

#ifdef MULTITHREAD_VERSION
typedef struct{
    shared_data *data;  //sarà la stessa passata a tutti i thread
    int id;
}thread_arg;

int thread_func(void *arg){
    thread_arg *args = (thread_arg *)arg;
    shared_data *dati = args->data;
    int thread_id = args->id;

    for(int i = 0; i<NUM_ITERAZIONI; i++){
        sem_wait(&dati->sem1);
        printf("Thread %d legge %d, incrementa a %d\n", thread_id, dati->counter, (dati->counter) + 1);
        dati->counter++;    //dati è un puntatore a args->data quindi modifico la memoria condivisa
        sem_post(&dati->sem1);
        sleep(1);
    }

    return 0;
}
#endif

int main(){
    int fd_shm;
    shared_data *dati;
    int prima_volta = 0;
    int ris = 0;

    fd_shm = shm_open(SHM_NOME, O_CREAT | O_RDWR | O_EXCL, 0600);
    if((fd_shm == -1) ){
        if((errno = EEXIST)){
            fd_shm = shm_open(SHM_NOME, O_RDWR, 0600);
        }else{
            perror("SHM_OPEN");
            exit(1);
        }
    }else{
        SCALL(ris, ftruncate(fd_shm, sizeof(shared_data)), "Errore ftruncate");
        ris = 0;
        prima_volta++; 
    }

    SCALL_PERSONALIZED(dati, mmap(NULL, sizeof(shared_data), PROT_WRITE | PROT_READ, MAP_SHARED, fd_shm, 0), MAP_FAILED, "Errore mmap");

    if(prima_volta){
        SCALL(ris, sem_init(&dati->sem1, 1, 1), "Errore sem_init");
        ris = 0;
        dati->counter = 0;
    }

    #ifdef MULTITHREAD_VERSION
    thrd_t threads[NUM_THREADS];
    thread_arg thrd_args[NUM_THREADS];

    for(int i = 0; i<NUM_THREADS; i++){
        thrd_args[i].data = dati;
        thrd_args[i].id = i + 1;

        thrd_create(
            &threads[i],
            thread_func,
            &thrd_args[i]            //devo tiparlo a void?
        );
        //forse devo fare (void *)&thrd_args[i]

    }

    for(int i = 0; i<NUM_THREADS; i++){
        thrd_join(threads[i], NULL);
    }

    #else

    for(int i = 0; i<NUM_ITERAZIONI; i++){
        sem_wait(&dati->sem1);
        //int val = dati->counter;
        printf("PID %d legge %d, incrementa a %d\n", getpid(), dati->counter, (dati->counter)+1);
        dati->counter++;
        sem_post(&dati->sem1);
        sleep(1);
    }    

    #endif

    sem_destroy(&dati->sem1);
    munmap(dati, sizeof(shared_data));
    shm_unlink(SHM_NOME);
    close(fd_shm);

    return 0;
}