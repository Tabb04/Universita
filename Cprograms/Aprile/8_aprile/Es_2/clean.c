#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>


int main(){

    sem_unlink("/primo_sem");
    sem_unlink("/secondo_sem");

    return 0;
}