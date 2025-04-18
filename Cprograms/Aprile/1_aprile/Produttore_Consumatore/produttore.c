#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define NOME_SM "/memoria_condivisa"
#define MEM_SIZE 1024

int main(){
    int fd = shm_open(NOME_SM, O_CREAT | O_RDWR, 0666);
    //controllo se -1

    ftruncate(fd, MEM_SIZE);

    void *ptr = mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    //controllo se MAP_FAILED

    strcpy((char *)ptr, "Ciao consumatore!");

    printf("Produttore: Messaggio scritto nella memoria condivisa\n");

    getchar(); //aspetta per terminare

    munmap(ptr, MEM_SIZE);
    close(fd);
    shm_unlink(NOME_SM);

    return 0;

}