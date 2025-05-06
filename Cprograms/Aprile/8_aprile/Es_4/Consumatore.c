#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h> 


#define SHM_NOME "/mem_prodcons"
#include "Syscalls.h"
#define NUM_ITERAZIONI 10

typedef struct{
    sem_t mutex;
    sem_t pieno;
    sem_t vuoto;
    int valore;
} sincronizzazione;


int main(){
    int ris;
    int fd_smh;
    sincronizzazione *dati;

    SCALL(fd_smh, shm_open(SHM_NOME, O_RDWR, 0600), "Errore shm_open");
    SCALL_PERSONALIZED(dati, mmap(NULL, sizeof(sincronizzazione), PROT_READ | PROT_WRITE, MAP_SHARED, fd_smh, 0), MAP_FAILED, "Errore mmap");

    for (int i = 0; i<NUM_ITERAZIONI; i++){
        sem_wait(&dati->pieno);
        sem_wait(&dati->mutex);

        printf("Consumatore consuma prodotto: %d\n", dati->valore);
        fflush(stdout);
        
        sem_post(&dati->mutex);
        sem_post(&dati->vuoto);
        sleep(1);
    }

    munmap(dati, sizeof(sincronizzazione));
    close(fd_smh);
    shm_unlink(SHM_NOME);

    return 0;
}