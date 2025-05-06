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

//Parte commentata serve nel caso non sia fatta una unlink e/o una shm con quel nome esista già

typedef struct{
    sem_t mutex;
    sem_t pieno;
    sem_t vuoto;
    int valore;
} sincronizzazione;

int main(){
    int fd_shm;
    sincronizzazione *dati;
    int ris = 0;

    /*
    int prima_volta = 0;
    fd_shm = shm_open(SHM_NOME, O_CREAT | O_RDWR | O_EXCL, 0600);
    if(fd_shm == -1){
        if(errno == EEXIST){
            shm_open(SHM_NOME, O_RDWR, 0600);
        }else{
            perror("Errore smh_open");
            exit(1);
        }
    }else{
        SCALL(ris, ftruncate(fd_shm, sizeof(sincronizzazione)), "Errore ftruncate");
        ris = 0;
        prima_volta = 1;
    }
    */

    SCALL(fd_shm, shm_open(SHM_NOME, O_CREAT | O_RDWR | O_EXCL, 0600), "Errore smh_open");
    SCALL(ris, ftruncate(fd_shm, sizeof(sincronizzazione)), "Errore ftruncate");
    ris = 0;

    SCALL_PERSONALIZED(dati, mmap(NULL, sizeof(sincronizzazione), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0), MAP_FAILED, "Errore mmap");

    /*
    if(prima_volta){
        SCALL(ris, sem_init(&dati->mutex, 1, 1), "Errore sem_init");
        SCALL(ris, sem_init(&dati->vuoto, 1, 1), "Errore sem_init");
        SCALL(ris, sem_init(&dati->pieno, 1, 0), "Errore sem_init");
        int valore = 0;
    }
    */

    SCALL(ris, sem_init(&dati->mutex, 1, 1), "Errore sem_init");
    SCALL(ris, sem_init(&dati->vuoto, 1, 1), "Errore sem_init");
    SCALL(ris, sem_init(&dati->pieno, 1, 0), "Errore sem_init");
    int valore = 0;

    for(int i = 0; i<NUM_ITERAZIONI; i++){
        sem_wait(&dati->vuoto);
        sem_wait(&dati->mutex);

        dati->valore = i;
        printf("Produttore produce prodotto: %d\n", i);
        fflush(stdout);

        sem_post(&dati->mutex);
        sem_post(&dati->pieno);
        sleep(1);
    } 

    munmap(dati, sizeof(sincronizzazione));
    close(fd_shm);
    return 0;

}