#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <fcntl.h>

#include "Syscalls_3.h"
#define NOME_SHM "/mem_PeF"
#define NUM_ITERAZIONI 10

typedef struct{
    sem_t mutex;
    int counter;
}dati_condivisi;


int main(){
    int fd_shm;
    int res;
    dati_condivisi *dati;
    int prima_volta;

    //SE FACCIO EXIT DESCRITTORI VENGONO CHIUSI, MEMORIA CONDIVISA RIMOSSA
    //MA SEMAFORI NON DISTRUTTI E MEMORIA NON UNLINKATA
    //QUINDI NON TUTTE LE SCALL ANDREBBERO BENE

    fd_shm = shm_open(NOME_SHM, O_RDWR | O_CREAT | O_EXCL, 0600);
    if(fd_shm == -1){
        if (errno = EEXIST){
            SCALL(fd_shm, shm_open(NOME_SHM, O_RDWR, 0600), "Errore shm_open");
        }else{
            perror("Errore shm_open");
            exit(1);
        }
    }else{
        SCALL(res, ftruncate(fd_shm, sizeof(dati_condivisi)), "Errore Ftruncate");
        prima_volta = 1;
    }
    
    SCALL_PERSONALIZED(dati, mmap(NULL, sizeof(dati_condivisi), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0), MAP_FAILED, "Errore mmap");
    if(prima_volta){
        SCALL(res, sem_init(&dati->mutex, 1, 1), "Errore sem_init");
    }

    pid_t pid1;
    SCALL(pid1, fork(), "Errore fork");
    
    for(int i = 0; i<NUM_ITERAZIONI; i++){
        SCALL(res, sem_wait(&dati->mutex), "Errore sem_wait");

        int val = dati->counter;
        printf("PID %d legge %d, incrementa a %d\n", getpid(), val, val+1);
        fflush(stdout);
        dati->counter++;

        SCALL(res, sem_post(&dati->mutex), "Errore sem_post");

        sleep(1);

    }
    
    SCALL(res, munmap(dati, sizeof(dati_condivisi)), "Errore munmap");
    SCALL(res, close(fd_shm), "Errore close");
    SCALL(res, sem_destroy(&dati->mutex), "Errore sem_destroy");
    SCALL(res, shm_unlink(NOME_SHM), "Errore shm_unlink");

    return 0;

}