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

int main(){
    shm_unlink(SHM_NOME);
    return 0;
}