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
#include <threads.h>  // Per le funzioni thrd_create e thrd_join
#include "Syscalls.h"

#define SHM_NOME "/mem_condivisa1"
#define NUM_ITERAZIONI 5
#define NUM_THREADS 4

typedef struct {
    sem_t sem1;
    int counter;
} shared_data;

// Struttura per passare dati ai thread
typedef struct {
    shared_data *dati;
    int thread_id;
} thread_arg;

// Funzione eseguita dai thread - notare il tipo di ritorno int per compatibilità con thrd_create
int thread_function(void *arg) {
    thread_arg *t_arg = (thread_arg *)arg;
    shared_data *dati = t_arg->dati;
    int thread_id = t_arg->thread_id;
    
    for(int i = 0; i < NUM_ITERAZIONI; i++) {
        sem_wait(&dati->sem1);
        printf("Thread %d (PID %d) legge %d, incrementa a %d\n", 
               thread_id, getpid(), dati->counter, (dati->counter) + 1);
        dati->counter++;
        sem_post(&dati->sem1);
        sleep(1);
    }
    
    return 0;  // Ritorna 0 per segnalare successo
}

int main() {
    int fd_shm;
    shared_data *dati;
    int prima_volta = 0;
    int ris = 0;
    
    // Apertura della memoria condivisa
    fd_shm = shm_open(SHM_NOME, O_CREAT | O_RDWR | O_EXCL, 0600);
    if((fd_shm == -1)) {
        if((errno == EEXIST)) {
            fd_shm = shm_open(SHM_NOME, O_RDWR, 0600);
        } else {
            perror("SHM_OPEN");
            exit(1);
        }
    } else {
        SCALL(ris, ftruncate(fd_shm, sizeof(shared_data)), "Errore ftruncate");
        ris = 0;
        prima_volta++;
    }
    
    SCALL_PERSONALIZED(dati, mmap(NULL, sizeof(shared_data), PROT_WRITE | PROT_READ, MAP_SHARED, fd_shm, 0), MAP_FAILED, "Errore mmap");
    
    if(prima_volta) {
        // Inizializzazione del semaforo
        SCALL(ris, sem_init(&dati->sem1, 1, 1), "Errore sem_init");
        ris = 0;
        dati->counter = 0;
    }
    
    // Creazione dei thread
    thrd_t threads[NUM_THREADS];
    thread_arg thread_args[NUM_THREADS];
    
    for(int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].dati = dati;
        thread_args[i].thread_id = i;
        
        int rc = thrd_create(&threads[i], thread_function, (void *)&thread_args[i]);
        if(rc != thrd_success) {
            printf("ERRORE: codice di ritorno da thrd_create() è %d\n", rc);
            exit(-1);
        }
    }
    
    // Attesa terminazione di tutti i thread
    for(int i = 0; i < NUM_THREADS; i++) {
        int result;
        thrd_join(threads[i], &result);
        printf("Thread %d completato con codice di uscita %d\n", i, result);
    }
    
    // Pulizia risorse
    sem_destroy(&dati->sem1);
    munmap(dati, sizeof(shared_data));
    shm_unlink(SHM_NOME);
    close(fd_shm);
    
    return 0;
}