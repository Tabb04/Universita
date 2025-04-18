#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOME_SHM "/memoria_condivisa"
#define DIM_SHM 1024

int main(){
    int fd = shm_open(NOME_SHM, O_RDWR, 0666); //presuppongo esista già

    void* ptr = mmap(NULL, DIM_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    //controllo se map failed

    printf("Messaggio ricevuto: %s\n", (char*)ptr);

    munmap(ptr, DIM_SHM);
    close(fd);

    return 0;
}

