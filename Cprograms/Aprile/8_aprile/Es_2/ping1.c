#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>


int main(){
    sem_t *sem1 = sem_open("/primo_sem", O_CREAT, 0600, 1);
    sem_t *sem2 = sem_open("/secondo_sem", O_CREAT, 0600, 0);

    for(int i = 0; i<5; i++){
        sleep(1);
        sem_wait(sem1);
        printf("Ciao!\n");
        fflush(stdout);
        sem_post(sem2);
        sleep(1);
    }

    sem_close(sem1);
    sem_close(sem2);
    return 0;
}